/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <lld/bf_lld_if.h>
}

#include <unordered_map>
#include <tuple>
#include <array>
#include <functional>
#include <vector>
#include <set>
#include <utility>
#include <memory>

#include "switch_tna/utils.h"
#include "common/hostif.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

p4_pd_sess_hdl_t sess_hdl;
p4_pd_dev_target_t pd_device;

#pragma pack(1)
/* This is an intenal header used when coal mirror frame is processed
 * This header is defined in P4 program and is extracted by egress
 * parser
 * This header is programmed in the hw as a set of 4 32 bit registers,
 * where first two bytes are fixed as - num_samples, compiler_defined_indication
 * the rest of the bytes are P4 program defined as -
 * +-------------+-------------+-------------+-------------+
 * | num_samples | compiler_def|       mirror_id           | Word 0
 * +-------------+-------------+-------------+-------------+
 * +-------------+-------------+-------------+-------------+ Word 1-3 (unused)
 * +-------------+-------------+-------------+-------------+
 * +-------------+-------------+-------------+-------------+
 */
typedef struct switch_coal_pkt_hdr_ {
  uint32_t reg_hdr0;
} switch_coal_pkt_hdr_t;

#pragma pack()

/**
 * MAX mirroring sessions supported
 */
#define SWITCH_MAX_MIRROR_SESSIONS 1024
/**
 * ID for cpu mirror session
 */
#define SWITCH_CPU_MIRROR_SESSION_ID 250
#define SWITCH_FOLDED_CPU_MIRROR_SESSION_ID 63

/**
 * Platform Id for Traffic Manager
 */
#define SWITCH_PLATFORM_ID_TM 60

/**
 * Platform Id for Egress pipeline
 */
#define SWITCH_PLATFORM_ID_EGRESS 62

/*
 * Base ID for coalesced mirror session. A total of 8 ids from the base are
 * reserved for coalescing mirroring. Only these IDs can be used for coalesing.
 */

#define SWITCH_COALESCED_MIRROR_BASE_SESSION_ID 1016  // BF_MIRROR_COAL_BASE_SID
#define SWITCH_MAX_COALESCED_MIRROR_SESSIONS 8  // BF_MIRROR_COAL_SESSION_MAX
#define SWITCH_COALESCED_MIRROR_MAX_SESSION_ID \
  (SWITCH_COALESCED_MIRROR_BASE_SESSION_ID +   \
   SWITCH_MAX_COALESCED_MIRROR_SESSIONS - 1)

#define SWITCH_MIRROR_SESSION_ID_COALESCING(_session_id)       \
  ((_session_id >= SWITCH_COALESCED_MIRROR_BASE_SESSION_ID) && \
   (_session_id <= SWITCH_COALESCED_MIRROR_MAX_SESSION_ID))    \
      ? true                                                   \
      : false

p4_pd_mirror_type_e switch_pd_p4_pd_mirror_type(uint64_t type) {
  switch (type) {
    case SWITCH_MIRROR_ATTR_SESSION_TYPE_SIMPLE:
      return PD_MIRROR_TYPE_NORM;
    case SWITCH_MIRROR_ATTR_SESSION_TYPE_COALESCE:
      return PD_MIRROR_TYPE_COAL;
    case SWITCH_MIRROR_ATTR_SESSION_TYPE_TRUNCATE:
    default:
      return PD_MIRROR_TYPE_MAX;
  }
}

p4_pd_direction_t switch_pd_p4_pd_direction(uint64_t direction) {
  switch (direction) {
    case SWITCH_MIRROR_ATTR_DIRECTION_BOTH:
      return PD_DIR_BOTH;
    case SWITCH_MIRROR_ATTR_DIRECTION_INGRESS:
      return PD_DIR_INGRESS;
    case SWITCH_MIRROR_ATTR_DIRECTION_EGRESS:
      return PD_DIR_EGRESS;
    default:
      return PD_DIR_NONE;
  }
}

class dtel_mirror_rewrite : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_MIRROR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_MIRROR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_MIRROR_REWRITE_ATTR_PARENT_HANDLE;
  uint16_t dod_session_id = 0;
  uint8_t tos = 0;
  uint8_t ttl = 0;
  bool udp_src_port_entropy = false;
  uint16_t max_pkt_len = 0;

 public:
  dtel_mirror_rewrite(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_MIRROR_REWRITE,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    auto it = match_action_list.begin();

    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_TRUNCATE_SIZE, max_pkt_len);
    if (bf_lld_dev_is_tof1(0)) {
      if ((feature::is_feature_set(SWITCH_FEATURE_INT_V2)) == true) {
        // Max report_length based on truncate size
        if (max_pkt_len == 0) {
          max_pkt_len = 0xffff;
        } else {
          max_pkt_len =
              (max_pkt_len + DTEL_REPORT_V2_DTEL_HEADERS_DROP_MAX_LENGTH) >> 2;
        }
      }
    } else {
      // Increase truncate size by max length of outer headers + DTEL headers
      if ((feature::is_feature_set(SWITCH_FEATURE_INT_V2)) == true) {
        max_pkt_len += DTEL_REPORT_V2_OUTER_HEADERS_LENGTH +
                       DTEL_REPORT_V2_DTEL_HEADERS_MAX_LENGTH;
      } else {
        max_pkt_len += DTEL_REPORT_V0_5_OUTER_HEADERS_LENGTH +
                       DTEL_REPORT_V0_5_DTEL_HEADERS_MAX_LENGTH;
      }
    }

    std::vector<uint16_t> session_ids;
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);

    std::vector<switch_ip_address_t> ip_list;
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DST_IP_LIST, ip_list);
    if (ip_list.size() != session_ids.size()) {
      status |= SWITCH_STATUS_FAILURE;
      return;
    }

    switch_ip_address_t src_ip;
    status |=
        switch_store::v_get(parent, SWITCH_REPORT_SESSION_ATTR_SRC_IP, src_ip);
    status |= switch_store::v_get(parent, SWITCH_REPORT_SESSION_ATTR_TTL, ttl);
    status |= switch_store::v_get(parent, SWITCH_REPORT_SESSION_ATTR_TOS, tos);
    status |=
        switch_store::v_get(parent,
                            SWITCH_REPORT_SESSION_ATTR_UDP_SRC_PORT_ENTROPY,
                            udp_src_port_entropy);
    if (bf_lld_dev_is_tof1(0)) {
      status |=
          switch_store::v_get(parent,
                              SWITCH_REPORT_SESSION_ATTR_DOD_MIRROR_SESSION_ID,
                              dod_session_id);
    }
    switch_object_id_t device_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DEVICE, device_handle);

    switch_mac_addr_t rmac = {0};
    switch_mac_addr_t smac = {0x00, 0x00, 0x00, 0x01, 0x02, 0x03};
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_SRC_MAC, rmac);

    std::vector<uint16_t>::iterator session_id = session_ids.begin();
    for (auto dst_ip : ip_list) {
      it =
          match_action_list.insert(it,
                                   std::pair<_MatchKey, _ActionEntry>(
                                       _MatchKey(smi_id::T_MIRROR_REWRITE),
                                       _ActionEntry(smi_id::T_MIRROR_REWRITE)));
      status |= it->first.set_exact(
          smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID, *session_id);
      if (udp_src_port_entropy) {
        it->second.init_action_data(smi_id::A_REWRITE_DTEL_REPORT_ENTROPY);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SMAC,
                                     reinterpret_cast<const char *>(smac.mac),
                                     sizeof(smac.mac));
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_DMAC,
                                     reinterpret_cast<const char *>(rmac.mac),
                                     sizeof(rmac.mac));
        status |= it->second.set_arg(
            smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_UDP_DST_PORT,
            parent,
            SWITCH_REPORT_SESSION_ATTR_UDP_DST_PORT);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_DIP,
                                     dst_ip);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SIP,
                                     src_ip);
        status |=
            it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_TTL, ttl);
        status |=
            it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_TOS, tos);
        if (bf_lld_dev_is_tof1(0)) {
          status |= it->second.set_arg(
              smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SESSION_ID, dod_session_id);
        }
        status |= it->second.set_arg(
            smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_MAX_PKT_LEN, max_pkt_len);
      } else {
        it->second.init_action_data(smi_id::A_REWRITE_DTEL_REPORT);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_SMAC,
                                     reinterpret_cast<const char *>(smac.mac),
                                     sizeof(smac.mac));
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_DMAC,
                                     reinterpret_cast<const char *>(rmac.mac),
                                     sizeof(rmac.mac));
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_UDP_DST_PORT,
                                     parent,
                                     SWITCH_REPORT_SESSION_ATTR_UDP_DST_PORT);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_UDP_SRC_PORT,
                                     parent,
                                     SWITCH_REPORT_SESSION_ATTR_UDP_SRC_PORT);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_DIP, dst_ip);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_SIP, src_ip);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_TTL, ttl);
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_TOS, tos);
        if (bf_lld_dev_is_tof1(0)) {
          status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_SESSION_ID,
                                       dod_session_id);
        }
        status |= it->second.set_arg(smi_id::P_REWRITE_DTEL_REPORT_MAX_PKT_LEN,
                                     max_pkt_len);
      }
      it++;
      session_id++;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it =
          match_action_list.insert(it,
                                   std::pair<_MatchKey, _ActionEntry>(
                                       _MatchKey(smi_id::T_MIRROR_REWRITE),
                                       _ActionEntry(smi_id::T_MIRROR_REWRITE)));
      status |= it->first.set_exact(
          smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID, dod_session_id);
      it->second.init_action_data(smi_id::A_REWRITE_IP_UDP_LENGTHS);
    }
  }
};

class dtel_clone_mirror_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_CLONE_MIRROR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_CLONE_MIRROR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_CLONE_MIRROR_REWRITE_ATTR_PARENT_HANDLE;
  uint16_t session_id = 0;

 public:
  dtel_clone_mirror_rewrite(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_MIRROR_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    if (((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) == false) ||
        !bf_lld_dev_is_tof1(0)) {
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID, session_id);
    if (session_id == 0) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Failed to create clone mirror rewrite for DTEL IFA."
                 "status {}",
                 __func__,
                 __LINE__,
                 status);
      return;
    }

    status |= match_key.set_exact(smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID,
                                  session_id);
    action_entry.init_action_data(smi_id::A_REWRITE_DTEL_IFA_CLONE);
  }
  switch_status_t create_update() {
    if (((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) == false) ||
        !bf_lld_dev_is_tof1(0)) {
      return SWITCH_STATUS_SUCCESS;
    }
    return p4_object_match_action::create_update();
  }
  switch_status_t del() {
    if (((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) == false) ||
        !bf_lld_dev_is_tof1(0)) {
      return SWITCH_STATUS_SUCCESS;
    }
    return p4_object_match_action::del();
  }
};

struct enum_hash_t {
  template <typename T>
  size_t operator()(T const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};

class mirror_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MIRROR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MIRROR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MIRROR_REWRITE_ATTR_PARENT_HANDLE;
  static const uint16_t default_tpid = SWITCH_ETHERTYPE_DOT1Q;
  enum rw_action_t {
    ERSPAN_2 = 0,
    ERSPAN_2_VLAN,
    ERSPAN_3,
    ERSPAN_3_VLAN,
    ERSPAN_3_PLAT,
    ERSPAN_3_PLAT_VLAN
  };
  enum idx_t {
    QID = 0,
    SMAC,
    DMAC,
    SIP,
    DIP,
    TOS,
    TTL,
    TPID,
    PCP,
    VID,
    NUM_FIELDS
  };
  using fields_t = std::array<bf_rt_field_id_t *, idx_t::NUM_FIELDS>;
  using m_action_t = std::tuple<bf_rt_action_id_t *, fields_t>;
  using objs_map_t = std::unordered_map<rw_action_t, m_action_t, enum_hash_t>;
  static const objs_map_t erspan_objects;
  uint16_t session_id = 0;

 public:
  mirror_rewrite(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_MIRROR_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {}, port_type = {};
    switch_object_id_t port_lag_handle = {};

    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_SESSION_ID, session_id);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_EGRESS_PORT_HANDLE, port_lag_handle);

    if (switch_store::object_type_query(port_lag_handle) ==
        SWITCH_OBJECT_TYPE_PORT) {
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_PORT_ATTR_TYPE, port_type);
      if (port_type.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) {
        clear_attrs();
        return;
      }
    }

    /*
     * Return if mirror type is not erspan
     */
    status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_TYPE, e);
    switch (e.enumdata) {
      case SWITCH_MIRROR_ATTR_TYPE_ENHANCED_REMOTE: {
        bool platform_hdr = true;
        switch_enum_t format = {};
        switch_enum_t rspan = {};

        status |=
            switch_store::v_get(parent, SWITCH_MIRROR_ATTR_RSPAN_TYPE, rspan);
        status |=
            switch_store::v_get(parent, SWITCH_MIRROR_ATTR_ERSPAN_TYPE, format);
        status |= switch_store::v_get(
            parent, SWITCH_MIRROR_ATTR_PLATFORM_INFO, platform_hdr);

        rw_action_t act_type = rw_action_t::ERSPAN_2;
        if (SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_2 == format.enumdata) {
          act_type = (SWITCH_MIRROR_ATTR_RSPAN_TYPE_NONE == rspan.enumdata)
                         ? rw_action_t::ERSPAN_2
                         : rw_action_t::ERSPAN_2_VLAN;
        } else if (SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_3 == format.enumdata) {
          if (SWITCH_MIRROR_ATTR_RSPAN_TYPE_NONE == rspan.enumdata) {
            act_type = (platform_hdr) ? rw_action_t::ERSPAN_3_PLAT
                                      : rw_action_t::ERSPAN_3;
          } else {
            act_type = (platform_hdr) ? rw_action_t::ERSPAN_3_PLAT_VLAN
                                      : rw_action_t::ERSPAN_3_VLAN;
          }
        }

        auto &objs_tbl = erspan_objects.at(act_type);
        status |= erspan_mirror_rewrite(parent, objs_tbl, rspan);
      } break;
      case SWITCH_MIRROR_ATTR_TYPE_REMOTE:
        status |= rspan_mirror_rewrite(parent);
        break;
      case SWITCH_MIRROR_ATTR_TYPE_LOCAL:
        status |= local_span_mirror_rewrite(parent);
      default:
        break;
    }
    return;
  }

  switch_status_t local_span_mirror_rewrite(switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status |= match_key.set_exact(smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID,
                                  session_id);
    action_entry.init_action_data(smi_id::A_REWRITE_DUMMY);
    status |= action_entry.set_arg(
        smi_id::P_REWRITE_DUMMY_QUEUE_ID, parent, SWITCH_MIRROR_ATTR_QUEUE_ID);
    return status;
  }

  switch_status_t erspan_mirror_rewrite(switch_object_id_t parent,
                                        const m_action_t &objs,
                                        switch_enum_t &vlan_mode) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status |= match_key.set_exact(smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID,
                                  session_id);
    action_entry.init_action_data(*std::get<0>(objs));

    /*
     * Erspan mirror attributes
     */
    fields_t fields = std::get<1>(objs);
    status |= action_entry.set_arg(
        *fields[idx_t::QID], parent, SWITCH_MIRROR_ATTR_QUEUE_ID);
    status |= action_entry.set_arg(
        *fields[idx_t::SIP], parent, SWITCH_MIRROR_ATTR_SRC_IP);
    status |= action_entry.set_arg(
        *fields[idx_t::DIP], parent, SWITCH_MIRROR_ATTR_DEST_IP);
    status |= action_entry.set_arg(
        *fields[idx_t::SMAC], parent, SWITCH_MIRROR_ATTR_SRC_MAC_ADDRESS);
    status |= action_entry.set_arg(
        *fields[idx_t::DMAC], parent, SWITCH_MIRROR_ATTR_DEST_MAC_ADDRESS);
    status |= action_entry.set_arg(
        *fields[idx_t::TOS], parent, SWITCH_MIRROR_ATTR_TOS);
    status |= action_entry.set_arg(
        *fields[idx_t::TTL], parent, SWITCH_MIRROR_ATTR_TTL);

    if (SWITCH_MIRROR_ATTR_RSPAN_TYPE_NONE != vlan_mode.enumdata) {
      uint16_t tpid = 0;

      status |= action_entry.set_arg(
          *fields[idx_t::PCP], parent, SWITCH_MIRROR_ATTR_VLAN_PRI);

      status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_VLAN_TPID, tpid);
      if (0 == tpid) {
        tpid = default_tpid;
      }
      status |= action_entry.set_arg(*fields[idx_t::TPID], tpid);

      if (SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_ID == vlan_mode.enumdata) {
        status |= action_entry.set_arg(
            *fields[idx_t::VID], parent, SWITCH_MIRROR_ATTR_VLAN_ID);
      } else {
        switch_object_id_t vlan_handle = {};
        uint16_t vlan_id = 0;

        status |= switch_store::v_get(
            parent, SWITCH_MIRROR_ATTR_VLAN_HANDLE, vlan_handle);
        if (vlan_handle.data == 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_NONE,
                     "{}:{}: Invalid VLAN handle. for rspan type: {}",
                     __func__,
                     __LINE__,
                     "RSPAN_TYPE_VLAN_HANDLE");
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
        status |=
            switch_store::v_get(vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);
        status |= action_entry.set_arg(*fields[idx_t::VID], vlan_id);
      }
    }
    return status;
  }

  switch_status_t rspan_mirror_rewrite(switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t vlan_handle = {};
    switch_enum_t rspan_type;

    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_RSPAN_TYPE, rspan_type);
    if (rspan_type.enumdata == SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_HANDLE) {
      status |= switch_store::v_get(
          parent, SWITCH_MIRROR_ATTR_VLAN_HANDLE, vlan_handle);
      if (vlan_handle.data == 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_NONE,
                   "{}:{}: Invalid VLAN handle. for rspan type: {}",
                   __func__,
                   __LINE__,
                   "RSPAN_TYPE_VLAN_HANDLE");
        return SWITCH_STATUS_INVALID_PARAMETER;
      }
    } else {
      uint16_t vlan_id = 0;
      status |=
          switch_store::v_get(parent, SWITCH_MIRROR_ATTR_VLAN_ID, vlan_id);
      if (vlan_id == 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_NONE,
                   "{}:{}: Invalid VLAN ID. for rspan type: {}",
                   __func__,
                   __LINE__,
                   "RSPAN_TYPE_VLAN_ID");
        return SWITCH_STATUS_INVALID_PARAMETER;
      }
      std::set<attr_w> lookup_attrs;
      switch_object_id_t device;
      status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_DEVICE, device);
      lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device));
      lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));
      switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
      if (vlan_handle.data == 0) {
        // create vlan
        status |= switch_store::object_create(
            SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
        status |= switch_store::v_set(
            vlan_handle, SWITCH_VLAN_ATTR_INTERNAL_OBJECT, true);
      }
    }

    switch_object_id_t bd_handle;
    status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    uint16_t bd = compute_bd(bd_handle);

    /*
     * rspan mirror attributes
     */
    status |= match_key.set_exact(smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID,
                                  session_id);
    action_entry.init_action_data(smi_id::A_REWRITE_RSPAN);
    status |= action_entry.set_arg(
        smi_id::P_REWRITE_RSPAN_QUEUE_ID, parent, SWITCH_MIRROR_ATTR_QUEUE_ID);
    status |= action_entry.set_arg(smi_id::P_REWRITE_RSPAN_VID, bd);

    return status;
  }
};

class dtel_mirror_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_MIRROR_SESSION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_MIRROR_SESSION_ATTR_STATUS;
  std::vector<p4_pd_mirror_session_info_t> mirr_sess_infos;
  switch_enum_t direction = {0};

 public:
  dtel_mirror_session(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint16_t max_pkt_len = 0;

    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_TRUNCATE_SIZE, max_pkt_len);
    /* Increase truncate size by len(switch_dtel_switch_local_mirror_metadata_h)
     * For tofino2, also add 3 so that truncate rounds up to the nearest
     * multiple of 4 rather than down. */
    if ((feature::is_feature_set(SWITCH_FEATURE_INT_V2)) == true) {
      if (bf_lld_dev_is_tof1(0)) {
        max_pkt_len += len_of_t1_switch_dtel_v2_switch_local_mirror_metadata_h;
      } else {
        max_pkt_len +=
            len_of_t2_switch_dtel_v2_switch_local_mirror_metadata_h + 3;
      }
    } else {
      if (bf_lld_dev_is_tof1(0)) {
        max_pkt_len += len_of_t1_switch_dtel_switch_local_mirror_metadata_h;
      } else {
        max_pkt_len += len_of_t2_switch_dtel_switch_local_mirror_metadata_h + 3;
      }
    }
    switch_object_id_t device_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DEVICE, device_handle);
    std::vector<uint16_t> recirc_port_list;
    status = switch_store::v_get(device_handle,
                                 SWITCH_DEVICE_ATTR_RECIRC_DEV_PORT_LIST,
                                 recirc_port_list);
    direction.enumdata = SWITCH_MIRROR_ATTR_DIRECTION_BOTH;
    if (!recirc_port_list.size()) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Failed to create mirror session for DTEL Report "
                 "Session. Failed to set egress port for mirror session to  "
                 "recirc port(s)"
                 "{} status {}",
                 __func__,
                 __LINE__,
                 device_handle,
                 status);
      return;
    }

    std::vector<uint16_t> session_ids;
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);

    p4_pd_mirror_session_info_t mirr_sess_info;
    for (uint16_t session_id : session_ids) {
      // If we reached here, then there is sufficient data for a mirror session
      memset(&mirr_sess_info, 0x0, sizeof(mirr_sess_info));
      mirr_sess_info.dir = switch_pd_p4_pd_direction(direction.enumdata);
      mirr_sess_info.id = session_id;
      mirr_sess_info.max_pkt_len = max_pkt_len;
      mirr_sess_info.c2c = false;
      mirr_sess_info.mcast_grp_a = 0;
      mirr_sess_info.mcast_grp_a_v = false;
      mirr_sess_info.egr_port =
          recirc_port_list[mirr_sess_info.id % recirc_port_list.size()];
      mirr_sess_info.mcast_grp_b = 0;
      mirr_sess_info.mcast_grp_b_v = false;
      mirr_sess_info.level1_mcast_hash = 0;
      mirr_sess_info.level2_mcast_hash = 0;
      mirr_sess_info.egr_port_v = true;
      mirr_sess_infos.push_back(mirr_sess_info);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    auto_object::create_update();

    for (auto mirr_sess_info : mirr_sess_infos) {
      pd_status = p4_pd_mirror_session_update(
          sess_hdl, pd_device, &mirr_sess_info, true);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
    }
    p4_pd_complete_operations(sess_hdl);

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    for (auto mirr_sess_info : mirr_sess_infos) {
      pd_status =
          p4_pd_mirror_session_delete(sess_hdl, pd_device, mirr_sess_info.id);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
    }
    p4_pd_complete_operations(sess_hdl);

    auto_object::del();

    return status;
  }
};

class dtel_dod_mirror_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_DOD_MIRROR_SESSION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_DOD_MIRROR_SESSION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_DOD_MIRROR_SESSION_ATTR_STATUS;
  uint16_t session_id = 0;
  p4_pd_mirror_session_info_t mirr_sess_info;

 public:
  dtel_dod_mirror_session(const switch_object_id_t parent,
                          switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint16_t max_pkt_len = 0;

    memset(&mirr_sess_info, 0x0, sizeof(mirr_sess_info));

    if (!bf_lld_dev_is_tof1(0)) {
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_TRUNCATE_SIZE, max_pkt_len);
    // Increase truncate size by max length of outer headers + DTEL headers
    if ((feature::is_feature_set(SWITCH_FEATURE_INT_V2)) == true) {
      max_pkt_len += DTEL_REPORT_V2_OUTER_HEADERS_LENGTH +
                     DTEL_REPORT_V2_DTEL_HEADERS_DROP_MAX_LENGTH;
    } else {
      max_pkt_len += DTEL_REPORT_V0_5_OUTER_HEADERS_LENGTH +
                     DTEL_REPORT_V0_5_DTEL_HEADERS_DROP_LENGTH;
    }
    switch_object_id_t device_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DEVICE, device_handle);
    std::vector<uint16_t> recirc_port_list;
    status = switch_store::v_get(device_handle,
                                 SWITCH_DEVICE_ATTR_RECIRC_DEV_PORT_LIST,
                                 recirc_port_list);
    if (!recirc_port_list.size()) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Failed to create DoD mirror session for DTEL Report "
                 "Session. Failed to set egress port for DoD mirror session "
                 "to recirc port(s)"
                 "{} status {}",
                 __func__,
                 __LINE__,
                 device_handle,
                 status);
      return;
    }
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DOD_MIRROR_SESSION_ID, session_id);
    if (session_id == 0) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Failed to create DoD mirror session for DTEL Report "
                 "Session."
                 "{} status {}",
                 __func__,
                 __LINE__,
                 device_handle,
                 status);
      return;
    }

    // If we reached here, then there is sufficient data for a mirror session
    mirr_sess_info.dir =
        switch_pd_p4_pd_direction(SWITCH_MIRROR_ATTR_DIRECTION_EGRESS);
    mirr_sess_info.id = session_id;
    mirr_sess_info.max_pkt_len = max_pkt_len;
    mirr_sess_info.c2c = false;
    mirr_sess_info.mcast_grp_a = 0;
    mirr_sess_info.mcast_grp_a_v = false;
    mirr_sess_info.egr_port = recirc_port_list[1];
    mirr_sess_info.mcast_grp_b = 0;
    mirr_sess_info.mcast_grp_b_v = false;
    mirr_sess_info.level1_mcast_hash = 0;
    mirr_sess_info.level2_mcast_hash = 0;
    mirr_sess_info.egr_port_v = true;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    auto_object::create_update();

    if (bf_lld_dev_is_tof1(0)) {
      pd_status = p4_pd_mirror_session_update(
          sess_hdl, pd_device, &mirr_sess_info, true);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      p4_pd_complete_operations(sess_hdl);
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    if (bf_lld_dev_is_tof1(0)) {
      pd_status =
          p4_pd_mirror_session_delete(sess_hdl, pd_device, mirr_sess_info.id);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      p4_pd_complete_operations(sess_hdl);
    }

    auto_object::del();

    return status;
  }
};

class dtel_clone_mirror_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_CLONE_MIRROR_SESSION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_CLONE_MIRROR_SESSION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_CLONE_MIRROR_SESSION_ATTR_STATUS;
  uint16_t session_id = 0;
  p4_pd_mirror_session_info_t mirr_sess_info;

 public:
  dtel_clone_mirror_session(const switch_object_id_t parent,
                            switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint16_t dev_port = 0;

    memset(&mirr_sess_info, 0x0, sizeof(mirr_sess_info));

    if (((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) == false) ||
        !bf_lld_dev_is_tof1(0)) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID, session_id);
    if (session_id == 0) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Failed to create clone mirror session for DTEL IFA."
                 "status {}",
                 __func__,
                 __LINE__,
                 status);
      return;
    }

    // If we reached here, then there is sufficient data for a mirror session
    mirr_sess_info.dir =
        switch_pd_p4_pd_direction(SWITCH_MIRROR_ATTR_DIRECTION_EGRESS);
    mirr_sess_info.id = session_id;
    mirr_sess_info.max_pkt_len = 0;
    mirr_sess_info.c2c = false;
    mirr_sess_info.mcast_grp_a = 0;
    mirr_sess_info.mcast_grp_a_v = false;
    mirr_sess_info.egr_port = dev_port;
    mirr_sess_info.mcast_grp_b = 0;
    mirr_sess_info.mcast_grp_b_v = false;
    mirr_sess_info.level1_mcast_hash = 0;
    mirr_sess_info.level2_mcast_hash = 0;
    mirr_sess_info.egr_port_v = true;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    auto_object::create_update();

    if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
        bf_lld_dev_is_tof1(0)) {
      pd_status = p4_pd_mirror_session_update(
          sess_hdl, pd_device, &mirr_sess_info, true);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      p4_pd_complete_operations(sess_hdl);
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;

    if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
        bf_lld_dev_is_tof1(0)) {
      pd_status =
          p4_pd_mirror_session_delete(sess_hdl, pd_device, mirr_sess_info.id);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      p4_pd_complete_operations(sess_hdl);
    }

    auto_object::del();

    return status;
  }
};

const mirror_rewrite::objs_map_t mirror_rewrite::erspan_objects = {
    {mirror_rewrite::rw_action_t::ERSPAN_2,
     mirror_rewrite::m_action_t(&smi_id::A_REWRITE_ERSPAN_TYPE2,
                                {&smi_id::P_REWRITE_ERSPAN_TYPE2_QUEUE_ID,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_SMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_DMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_SIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_DIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_TOS,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE2_TTL})},
    {mirror_rewrite::rw_action_t::ERSPAN_2_VLAN,
     mirror_rewrite::m_action_t(
         &smi_id::A_REWRITE_ERSPAN_TYPE2_VLAN,
         {&smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_QUEUE_ID,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_SMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_DMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_SIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_DIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_TOS,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_TTL,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_ETHER_TYPE,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_PCP,
          &smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_VID})},
    {mirror_rewrite::rw_action_t::ERSPAN_3,
     mirror_rewrite::m_action_t(&smi_id::A_REWRITE_ERSPAN_TYPE3,
                                {&smi_id::P_REWRITE_ERSPAN_TYPE3_QUEUE_ID,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_SMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_DMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_SIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_DIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_TOS,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_TTL})},
    {mirror_rewrite::rw_action_t::ERSPAN_3_VLAN,
     mirror_rewrite::m_action_t(
         &smi_id::A_REWRITE_ERSPAN_TYPE3_VLAN,
         {&smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_QUEUE_ID,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_SMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_DMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_SIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_DIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_TOS,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_TTL,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_ETHER_TYPE,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_PCP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_VID})},
    {mirror_rewrite::rw_action_t::ERSPAN_3_PLAT,
     mirror_rewrite::m_action_t(&smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT,
                                {&smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_QUEUE_ID,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_SMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_DMAC,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_SIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_DIP,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_TOS,
                                 &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_TTL})},
    {mirror_rewrite::rw_action_t::ERSPAN_3_PLAT_VLAN,
     mirror_rewrite::m_action_t(
         &smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN,
         {&smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_QUEUE_ID,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DMAC,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DIP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TOS,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TTL,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_ETHER_TYPE,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_PCP,
          &smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_VID})}};

class mirror_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MIRROR_SESSION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MIRROR_SESSION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t vlan_member_attr_id =
      SWITCH_MIRROR_SESSION_ATTR_VLAN_MEMBER_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MIRROR_SESSION_ATTR_STATUS;
  static const switch_attr_id_t hw_programmed_attr_id =
      SWITCH_MIRROR_SESSION_ATTR_HW_PROGRAMMED;
  p4_pd_mirror_session_info_t mirr_sess_info;
  switch_enum_t mirror_type = {0}, session_type = {0}, direction = {0},
                rspan_type = {0}, type = {0};
  switch_object_id_t vlan_handle = {0}, port_lag_handle = {0},
                     port_handle = {0}, device = {0};
  switch_object_type_t monitor_port_ot = SWITCH_OBJECT_TYPE_PORT;

  uint16_t vlan_id = 0;
  uint16_t session_id = 0;
  switch_coal_pkt_hdr_t int_coal_pkt_hdr = {};

 public:
  mirror_session(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint32_t extract_length = 0, max_pkt_len = 0, timeout_usec = 0;

    switch_object_id_t mgid_handle = {0};
    uint32_t l1_hash = 0;
    uint32_t l2_hash = 0;
    uint16_t dev_port = 0;
    uint8_t tos = 0;
    uint8_t queue_id = 0;

    memset(&mirr_sess_info, 0x0, sizeof(mirr_sess_info));

    status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_DEVICE, device);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_MIRROR_TYPE, mirror_type);
    status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_TYPE, type);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_SESSION_TYPE, session_type);
    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_DIRECTION, direction);
    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_RSPAN_TYPE, rspan_type);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_TIMEOUT_USEC, timeout_usec);
    status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_TOS, tos);
    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_QUEUE_ID, queue_id);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_EXTRACT_LENGTH, extract_length);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_MAX_PKT_LEN, max_pkt_len);
    max_pkt_len += port_mirror_metadata_len_get(
        SWITCH_MIRROR_ATTR_DIRECTION_EGRESS == direction.enumdata);
    /* Increase truncate size by len(switch_port_mirror_metadata_h).
     * For tofino2, also add 3 so that truncate rounds up to the nearest
     * multiple of 4 rather than down. */
    if (!bf_lld_dev_is_tof1(0)) {
      max_pkt_len += 3;
    }
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_VLAN_HANDLE, vlan_handle);
    status |= switch_store::v_get(parent, SWITCH_MIRROR_ATTR_VLAN_ID, vlan_id);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_EGRESS_PORT_HANDLE, port_lag_handle);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_MULTICAST_MGID_HANDLE, mgid_handle);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_MULTICAST_L1_HASH, l1_hash);
    status |= switch_store::v_get(
        parent, SWITCH_MIRROR_ATTR_MULTICAST_L2_HASH, l2_hash);

    if (port_lag_handle.data == 0) {
      attrs.clear();
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    } else {
      monitor_port_ot = switch_store::object_type_query(port_lag_handle);
      if (monitor_port_ot == SWITCH_OBJECT_TYPE_PORT) {
        port_handle = port_lag_handle;
        status |= switch_store::v_get(
            port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      } else {
        assert(monitor_port_ot == SWITCH_OBJECT_TYPE_LAG);
        switch_object_id_t designated_lag_member = {0};
        status |= switch_store::v_get(port_lag_handle,
                                      SWITCH_LAG_ATTR_DESIGNATED_LAG_MEMBER,
                                      designated_lag_member.data);
        if (designated_lag_member.data != 0) {
          status |= switch_store::v_get(designated_lag_member,
                                        SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE,
                                        port_handle);
          status |= switch_store::v_get(
              port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        }
      }
    }

    status |=
        switch_store::v_get(parent, SWITCH_MIRROR_ATTR_SESSION_ID, session_id);

    int_coal_pkt_hdr.reg_hdr0 = session_id;

    if (session_type.enumdata == SWITCH_MIRROR_ATTR_SESSION_TYPE_COALESCE) {
      if (extract_length > 80 || extract_length & 0x3 ||
          direction.enumdata == SWITCH_MIRROR_ATTR_DIRECTION_INGRESS ||
          !SWITCH_MIRROR_SESSION_ID_COALESCING(session_id)) {
        attrs.clear();
        status = SWITCH_STATUS_INVALID_PARAMETER;
        return;
      }
    } else if (SWITCH_MIRROR_SESSION_ID_COALESCING(session_id)) {
      attrs.clear();
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    if (session_type.enumdata != SWITCH_MIRROR_ATTR_SESSION_TYPE_TRUNCATE)
      max_pkt_len = 0;

    // If we reached here, then there is sufficient data for a mirror session
    memset(&mirr_sess_info, 0x0, sizeof(mirr_sess_info));
    mirr_sess_info.dir = switch_pd_p4_pd_direction(direction.enumdata);
    mirr_sess_info.id = session_id;
    mirr_sess_info.max_pkt_len = max_pkt_len;
    mirr_sess_info.cos = tos;
    mirr_sess_info.egr_port_queue = queue_id;
    mirr_sess_info.c2c = false;
    mirr_sess_info.extract_len = extract_length;
    mirr_sess_info.timeout_usec = timeout_usec;
    mirr_sess_info.int_hdr =
        (uint32_t *)&int_coal_pkt_hdr;  // NOLINT(readability/casting)
    mirr_sess_info.int_hdr_len = (sizeof(switch_coal_pkt_hdr_t) + 3) / 4;
    mirr_sess_info.mcast_grp_a = 0;
    mirr_sess_info.mcast_grp_a_v = false;
    if (mirror_type.enumdata == SWITCH_MIRROR_ATTR_MIRROR_TYPE_PORT) {
      mirr_sess_info.egr_port = dev_port;
      mirr_sess_info.mcast_grp_b = 0;
      mirr_sess_info.mcast_grp_b_v = false;
      mirr_sess_info.level1_mcast_hash = 0;
      mirr_sess_info.level2_mcast_hash = 0;
      mirr_sess_info.egr_port_v = true;
    } else {
      mirr_sess_info.egr_port = 0;
      mirr_sess_info.mcast_grp_b = switch_store::handle_to_id(mgid_handle);
      mirr_sess_info.mcast_grp_b_v = true;
      mirr_sess_info.level1_mcast_hash = l1_hash;
      mirr_sess_info.level2_mcast_hash = l2_hash;
      mirr_sess_info.egr_port_v = false;
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;
    switch_object_id_t vlan_member_handle = {0};
    bool hw_programmed = false;

    status = add_vlan_and_member(vlan_member_handle);
    if (status != 0) return SWITCH_STATUS_FAILURE;

    auto_object::create_update();
    status = switch_store::v_set(
        get_auto_oid(), vlan_member_attr_id, vlan_member_handle.data);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    status = switch_store::v_get(
        get_auto_oid(), hw_programmed_attr_id, hw_programmed);

    // If mirror session points to Lag, and lag does not contain
    // any lag members, then delete the mirror session in hardware
    // We do not delete the auto object here as that will be used
    // later on when updates to the mirror session user object are done
    // Finally when this mirror session points to a valid egress port
    // the hardware mirror session will be created
    if (monitor_port_ot == SWITCH_OBJECT_TYPE_LAG && port_handle.data == 0) {
      if (hw_programmed) {
        pd_status =
            p4_pd_mirror_session_delete(sess_hdl, pd_device, mirr_sess_info.id);
        p4_pd_complete_operations(sess_hdl);
        CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
        status =
            switch_store::v_set(get_auto_oid(), hw_programmed_attr_id, false);
      }
      return status;
    }

    pd_status =
        p4_pd_mirror_session_update(sess_hdl, pd_device, &mirr_sess_info, true);
    p4_pd_complete_operations(sess_hdl);
    CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);

    if (!hw_programmed) {
      status = switch_store::v_set(get_auto_oid(), hw_programmed_attr_id, true);
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)pd_status;
    switch_object_id_t vlan_member_handle = {0}, clear_handle = {0};
    bool hw_programmed = false;
    status = switch_store::v_get(
        get_auto_oid(), hw_programmed_attr_id, hw_programmed);

    if (hw_programmed) {
      pd_status =
          p4_pd_mirror_session_delete(sess_hdl, pd_device, mirr_sess_info.id);
      p4_pd_complete_operations(sess_hdl);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
    }

    status = switch_store::v_get(
        get_auto_oid(), vlan_member_attr_id, vlan_member_handle.data);
    status |= del_vlan_and_member(vlan_member_handle);
    status |= switch_store::v_set(
        get_auto_oid(), vlan_member_attr_id, clear_handle.data);
    status |= switch_store::v_set(get_auto_oid(), hw_programmed_attr_id, false);
    auto_object::del();

    return status;
  }

  switch_status_t add_vlan_and_member(switch_object_id_t &vlan_member_handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type.enumdata == SWITCH_MIRROR_ATTR_TYPE_REMOTE) {
      if (rspan_type.enumdata == SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_HANDLE) {
        if (vlan_handle.data == 0) {
          attrs.clear();
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
      } else {
        if (vlan_id == 0) {
          attrs.clear();
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
        std::set<attr_w> lookup_attrs;
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device));
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));
        switch_store::object_id_get_wkey(
            SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
        if (vlan_handle.data == 0) {
          // create vlan
          status |= switch_store::object_create(
              SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
          status |= switch_store::v_set(
              vlan_handle, SWITCH_VLAN_ATTR_INTERNAL_OBJECT, true);
        }
      }

      std::set<attr_w> vlan_mem_attrs;
      vlan_mem_attrs.insert(attr_w(SWITCH_VLAN_MEMBER_ATTR_DEVICE, device));
      vlan_mem_attrs.insert(
          attr_w(SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE, vlan_handle));
      vlan_mem_attrs.insert(
          attr_w(SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, port_handle));
      if (switch_store::smiContext::context().in_warm_init()) {
        switch_store::object_id_get_wkey(
            SWITCH_OBJECT_TYPE_VLAN_MEMBER, vlan_mem_attrs, vlan_member_handle);
      } else {
        switch_enum_t tag_mode = {
            .enumdata = SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED};
        vlan_mem_attrs.insert(
            attr_w(SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE, tag_mode));
        status |= switch_store::object_create(
            SWITCH_OBJECT_TYPE_VLAN_MEMBER, vlan_mem_attrs, vlan_member_handle);
        status |= switch_store::v_set(
            vlan_member_handle, SWITCH_VLAN_MEMBER_ATTR_INTERNAL_OBJECT, true);
      }
    }
    return status;
  }

  switch_status_t del_vlan_and_member(switch_object_id_t vlan_member_handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type.enumdata == SWITCH_MIRROR_ATTR_TYPE_REMOTE) {
      if (rspan_type.enumdata == SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_HANDLE) {
        if (vlan_handle.data == 0) {
          attrs.clear();
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
      } else {
        if (vlan_id == 0) {
          attrs.clear();
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
        std::set<attr_w> lookup_attrs;
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device));
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));
        switch_store::object_id_get_wkey(
            SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
        if (vlan_handle.data == 0) {
          attrs.clear();
          return SWITCH_STATUS_INVALID_PARAMETER;
        }
      }
      // delete vlan member and interface
      status |= switch_store::object_delete(vlan_member_handle);

      // Delete the vlan handle for vlan_id type
      if (rspan_type.enumdata == SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_ID) {
        status |= switch_store::object_delete(vlan_handle);
      }
    }
    return status;
  }
};

switch_status_t dtel_mirror_session_ids_allocate(
    size_t num_ips, std::vector<uint16_t> &session_ids) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t alloc_sess_id = 0;

  for (uint32_t i = 0; i < num_ips; i++) {
    alloc_sess_id = SWITCH_CONTEXT.mirror_session_id_allocate();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_REPORT_SESSION,
                 "{}:{}: Failed to allocate session_id: error: {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
    session_ids.push_back(static_cast<uint16_t>(alloc_sess_id));
  }
  return status;
}

switch_status_t dtel_mirror_session_ids_release(
    const std::vector<uint16_t> &session_ids) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  for (uint16_t alloc_sess_id : session_ids) {
    status |= SWITCH_CONTEXT.mirror_session_id_release(alloc_sess_id);
  }
  return status;
}

switch_status_t before_report_session_create(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t alloc_sess_id = 0;

  attr_w mirror_id_list(SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID);
  std::vector<uint16_t> session_ids;
  uint16_t dod_session_id = 0;

  size_t num_ips = 0;
  std::vector<switch_ip_address_t> ip_list;
  const auto attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_REPORT_SESSION_ATTR_DST_IP_LIST));
  if (attr_it != attrs.end()) {
    attr_it->v_get(ip_list);
    num_ips = ip_list.size();
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               object_type,
               "{}:{}: Failed to find Dst IP List for dtel mirror session",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  if (num_ips) {
    const auto session_id_attr_it = attrs.find(static_cast<switch_attr_id_t>(
        SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID));

    if (session_id_attr_it == attrs.end()) {
      dtel_mirror_session_ids_allocate(num_ips, session_ids);

      mirror_id_list.v_set(session_ids);
      const auto insert_ret = attrs.insert(mirror_id_list);
      CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
    } else {
      status = (*session_id_attr_it).v_get(session_ids);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Pre report session create task failed. Failed to "
                   "fetch dtel report, mirror session ids: error: {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
      for (uint16_t session_id : session_ids) {
        SWITCH_CONTEXT.mirror_session_id_reserve(session_id);
      }
    }
  }
  if (bf_lld_dev_is_tof1(0)) {
    const auto session_id_attr_it = attrs.find(static_cast<switch_attr_id_t>(
        SWITCH_REPORT_SESSION_ATTR_DOD_MIRROR_SESSION_ID));

    if (session_id_attr_it == attrs.end()) {
      alloc_sess_id = SWITCH_CONTEXT.mirror_session_id_allocate();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Failed to allocate session_id: error: {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
      dod_session_id = static_cast<uint16_t>(alloc_sess_id);
      const auto insert_ret = attrs.insert(attr_w(
          SWITCH_REPORT_SESSION_ATTR_DOD_MIRROR_SESSION_ID, dod_session_id));
      CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
    } else {
      status = (*session_id_attr_it).v_get(dod_session_id);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Pre report session create task failed. Failed to "
                   "fetch dod mirror session id: error: {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
      SWITCH_CONTEXT.mirror_session_id_reserve(dod_session_id);
    }
  }
  return status;
}

switch_status_t after_report_session_delete(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> session_ids;
  uint16_t dod_session_id = 0;
  (void)object_type;
  for (auto it = attrs.begin(); it != attrs.end(); it++) {
    switch (it->id_get()) {
      case SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID:
        status = it->v_get(session_ids);
        if (status != SWITCH_STATUS_SUCCESS) return status;
        break;
      case SWITCH_REPORT_SESSION_ATTR_DOD_MIRROR_SESSION_ID:
        status = it->v_get(dod_session_id);
        if (status != SWITCH_STATUS_SUCCESS) return status;
        break;
      default:
        break;
    }
  }

  status = dtel_mirror_session_ids_release(session_ids);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (bf_lld_dev_is_tof1(0)) {
    status |= SWITCH_CONTEXT.mirror_session_id_release(dod_session_id);
  }
  return status;
}

switch_status_t before_report_session_update(const switch_object_id_t object_id,
                                             const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject_session, mobject_rewrite;
  std::vector<uint16_t> session_ids;

  if (attr.id_get() == SWITCH_REPORT_SESSION_ATTR_DST_IP_LIST) {
    // Delete dtel_mirror_session since number of mirror sessions may change
    mobject_session = std::unique_ptr<dtel_mirror_session>(
        new dtel_mirror_session(object_id, status));
    if (mobject_session != NULL) {
      status = mobject_session->del();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    std::unique_ptr<object> dtel_mirror_session_table(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION_TABLE, object_id, status));
    if (dtel_mirror_session_table != NULL) {
      status = dtel_mirror_session_table->del();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    std::unique_ptr<object> dtel_session_selector_group(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_SESSION_SELECTOR_GROUP, object_id, status));
    if (dtel_session_selector_group != NULL) {
      status = dtel_session_selector_group->del();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    std::unique_ptr<object> dtel_session_selector(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_SESSION_SELECTOR, object_id, status));
    if (dtel_session_selector != NULL) {
      status = dtel_session_selector->del();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    // Delete dtel_mirror_rewrite
    mobject_rewrite = std::unique_ptr<dtel_mirror_rewrite>(
        new dtel_mirror_rewrite(object_id, status));
    if (mobject_rewrite != NULL) {
      status = mobject_rewrite->del();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    // Clear mirror session IDs, will be reallocated after update
    status = switch_store::v_get(
        object_id, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);
    if (status == SWITCH_STATUS_SUCCESS && !session_ids.empty()) {
      status = dtel_mirror_session_ids_release(session_ids);
      if (status != SWITCH_STATUS_SUCCESS) return status;
      session_ids.clear();
      status = switch_store::v_set(
          object_id, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);
    } else if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
      status = SWITCH_STATUS_SUCCESS;
    }
  }
  return status;
}

switch_status_t after_report_session_update(const switch_object_id_t object_id,
                                            const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject_session, mobject_rewrite;
  std::vector<uint16_t> session_ids;
  size_t num_ips = 0;
  std::vector<switch_ip_address_t> ip_list;

  if (attr.id_get() == SWITCH_REPORT_SESSION_ATTR_DST_IP_LIST) {
    // Re-allocate mirror session IDs after update
    status = switch_store::v_get(
        object_id, SWITCH_REPORT_SESSION_ATTR_DST_IP_LIST, ip_list);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    num_ips = ip_list.size();

    if (num_ips) {
      dtel_mirror_session_ids_allocate(num_ips, session_ids);

      status = switch_store::v_set(
          object_id, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    // Re-create dtel_mirror_rewrite object
    mobject_rewrite = std::unique_ptr<dtel_mirror_rewrite>(
        new dtel_mirror_rewrite(object_id, status));
    if (mobject_rewrite != NULL) {
      status = mobject_rewrite->create_update();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    // Re-create dtel_mirror_session object
    mobject_session = std::unique_ptr<dtel_mirror_session>(
        new dtel_mirror_session(object_id, status));
    if (mobject_session != NULL) {
      status = mobject_session->create_update();
    }

    std::unique_ptr<object> dtel_session_selector(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_SESSION_SELECTOR, object_id, status));
    if (dtel_session_selector != NULL) {
      status = dtel_session_selector->create_update();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    std::unique_ptr<object> dtel_session_selector_group(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_SESSION_SELECTOR_GROUP, object_id, status));
    if (dtel_session_selector_group != NULL) {
      status = dtel_session_selector_group->create_update();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;

    std::unique_ptr<object> dtel_mirror_session_table(
        factory::get_instance().create(
            SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION_TABLE, object_id, status));
    if (dtel_mirror_session_table != NULL) {
      status = dtel_mirror_session_table->create_update();
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }
  return status;
}

switch_status_t before_port_create_2(const switch_object_type_t object_type,
                                     std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t alloc_sess_id = 0;
  uint16_t clone_session_id = 0;

  if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
      bf_lld_dev_is_tof1(0)) {
    const auto session_id_attr_it = attrs.find(static_cast<switch_attr_id_t>(
        SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID));
    if (session_id_attr_it == attrs.end()) {
      alloc_sess_id = SWITCH_CONTEXT.mirror_session_id_allocate();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Failed to allocate clone_session_id: error: {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
      clone_session_id = static_cast<uint16_t>(alloc_sess_id);
      const auto insert_ret = attrs.insert(
          attr_w(SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID, clone_session_id));
      CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
    } else {
      status = (*session_id_attr_it).v_get(clone_session_id);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Pre port create task failed. Failed to fetch "
                   "clone_session_id: error: {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
      SWITCH_CONTEXT.mirror_session_id_reserve(clone_session_id);
    }
  }
  return status;
}

switch_status_t before_port_delete_2(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t clone_session_id;

  if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
      bf_lld_dev_is_tof1(0)) {
    status = switch_store::v_get(
        object_id, SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID, clone_session_id);
    status |= SWITCH_CONTEXT.mirror_session_id_release(clone_session_id);
  }
  return status;
}

switch_status_t before_mirror_create(const switch_object_type_t object_type,
                                     std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t port_handle = {};
  switch_enum_t port_type;
  uint32_t alloc_sess_id = 0;
  uint16_t session_id = 0;

  const auto attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_MIRROR_ATTR_EGRESS_PORT_HANDLE));
  CHECK_RET(attr_it == attrs.end(), SWITCH_STATUS_FAILURE);
  status = (*attr_it).v_get(port_handle);
  const auto monitor_port_ot = switch_store::object_type_query(port_handle);

  if (monitor_port_ot == SWITCH_OBJECT_TYPE_PORT) {
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
  }

  bool internal_obj = false;
  const auto internal_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_MIRROR_ATTR_INTERNAL_OBJECT));
  if (internal_attr_it != attrs.end()) (*internal_attr_it).v_get(internal_obj);

  const auto session_id_attr_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_MIRROR_ATTR_SESSION_ID));

  if (session_id_attr_it == attrs.end()) {
    if (monitor_port_ot == SWITCH_OBJECT_TYPE_PORT &&
        port_type.enumdata == SWITCH_PORT_ATTR_TYPE_CPU && internal_obj) {
      if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
        session_id = SWITCH_FOLDED_CPU_MIRROR_SESSION_ID;
      } else {
        session_id = SWITCH_CPU_MIRROR_SESSION_ID;
      }
    } else {
      alloc_sess_id = SWITCH_CONTEXT.mirror_session_id_allocate();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}:{}: Failed to allocate session_id: error: {}",
                   __func__,
                   __LINE__,
                   status);
      }
      session_id = static_cast<uint16_t>(alloc_sess_id);
    }

    // allocate mirror session ID left shifted by 2 to allow for 8 bit mirror
    // session IDs.
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      session_id = session_id << 2;
    }
    const auto insert_ret =
        attrs.insert(attr_w(SWITCH_MIRROR_ATTR_SESSION_ID, session_id));
    CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
  } else {
    status |= (*session_id_attr_it).v_get(session_id);
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      session_id = session_id >> 2;
    }
    SWITCH_CONTEXT.mirror_session_id_reserve(session_id);
  }
  return status;
}

switch_status_t after_mirror_delete(const switch_object_type_t object_type,
                                    const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t port_handle = {};
  switch_enum_t port_type;
  uint16_t alloc_sess_id = 0;
  (void)object_type;

  for (auto it = attrs.begin(); it != attrs.end(); it++) {
    switch (it->id_get()) {
      case SWITCH_MIRROR_ATTR_EGRESS_PORT_HANDLE:
        status |= it->v_get(port_handle);
        break;
      case SWITCH_MIRROR_ATTR_SESSION_ID:
        status |= it->v_get(alloc_sess_id);
        break;
      default:
        break;
    }
  }
  const auto monitor_port_ot = switch_store::object_type_query(port_handle);

  if (monitor_port_ot == SWITCH_OBJECT_TYPE_PORT) {
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
  }

  if (((monitor_port_ot == SWITCH_OBJECT_TYPE_PORT) &&
       port_type.enumdata != SWITCH_PORT_ATTR_TYPE_CPU) ||
      monitor_port_ot == SWITCH_OBJECT_TYPE_LAG) {
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      alloc_sess_id = alloc_sess_id >> 2;
    }
    status |= SWITCH_CONTEXT.mirror_session_id_release(alloc_sess_id);
  }

  return status;
}

switch_status_t mirror_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  p4_pd_status_t pd_status = 0;

  SWITCH_CONTEXT.mirror_session_id_reserve(SWITCH_CPU_MIRROR_SESSION_ID);

  const bf_rt_target_t dev_tgt = get_dev_tgt();
  pd_device.device_id = dev_tgt.dev_id;
  pd_device.dev_pipe_id = dev_tgt.pipe_id;

  pd_status = p4_pd_client_init(&sess_hdl);
  CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);

  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_MIRROR,
                                                  &before_mirror_create);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_MIRROR,
                                                 &after_mirror_delete);
  status |= switch_store::reg_create_trigs_before(
      SWITCH_OBJECT_TYPE_REPORT_SESSION, &before_report_session_create);
  status |= switch_store::reg_delete_trigs_after(
      SWITCH_OBJECT_TYPE_REPORT_SESSION, &after_report_session_delete);
  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_REPORT_SESSION, &before_report_session_update);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_REPORT_SESSION, &after_report_session_update);
  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_PORT,
                                                  &before_port_create_2);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_PORT,
                                                  &before_port_delete_2);

  REGISTER_OBJECT(mirror_session, SWITCH_OBJECT_TYPE_MIRROR_SESSION);
  REGISTER_OBJECT(mirror_rewrite, SWITCH_OBJECT_TYPE_MIRROR_REWRITE);
  REGISTER_OBJECT(dtel_mirror_session, SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION);
  REGISTER_OBJECT(dtel_dod_mirror_session,
                  SWITCH_OBJECT_TYPE_DTEL_DOD_MIRROR_SESSION);
  REGISTER_OBJECT(dtel_clone_mirror_session,
                  SWITCH_OBJECT_TYPE_DTEL_CLONE_MIRROR_SESSION);
  REGISTER_OBJECT(dtel_mirror_rewrite, SWITCH_OBJECT_TYPE_DTEL_MIRROR_REWRITE);
  REGISTER_OBJECT(dtel_clone_mirror_rewrite,
                  SWITCH_OBJECT_TYPE_DTEL_CLONE_MIRROR_REWRITE);

  return status;
}

switch_status_t mirror_clean() {
  p4_pd_status_t pd_status = 0;
  pd_status = p4_pd_client_cleanup(sess_hdl);
  CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);

  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
