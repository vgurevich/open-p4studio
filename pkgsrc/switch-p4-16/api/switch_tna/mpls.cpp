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
#include <lld/bf_lld_if.h>
}

#include <vector>
#include <utility>
#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class mpls_label : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MPLS_LABEL;
  static const switch_attr_id_t status_attr_id = SWITCH_MPLS_LABEL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MPLS_LABEL_ATTR_PARENT_HANDLE;

 public:
  mpls_label(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_MPLS_LABEL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t nexthop_handle = {0};
    uint8_t num_labels = 0;
    switch_enum_t labelop = {0}, nhop_type = {0};
    std::vector<uint32_t> label_list;

    status |= switch_store::v_get(parent, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
    if (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_MPLS) return;

    nexthop_handle = parent;

    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_LABELOP, labelop);
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list);
    num_labels = label_list.size();
    if (num_labels == 0) return;

    match_key.set_exact(smi_id::F_MPLS_LABEL_LOCAL_MD_TUNNEL_NEXTHOP,
                        compute_nexthop_index(nexthop_handle));

    if (labelop.enumdata == SWITCH_NEXTHOP_ATTR_LABELOP_SWAP) {
      action_entry.init_action_data(smi_id::A_MPLS_SWAP_LABEL);
      action_entry.set_arg(smi_id::P_MPLS_SWAP_LABEL0, label_list[0] & 0xFFFFF);
      return;
    }
    switch (num_labels) {
      case 1:
        action_entry.init_action_data(smi_id::A_MPLS_PUSH_1_LABEL);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_1_LABEL0,
                             label_list[0] & 0xFFFFF);
        break;
      case 2:
        action_entry.init_action_data(smi_id::A_MPLS_PUSH_2_LABEL);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_2_LABEL0,
                             label_list[0] & 0xFFFFF);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_2_LABEL1,
                             label_list[1] & 0xFFFFF);
        break;
      case 3:
        action_entry.init_action_data(smi_id::A_MPLS_PUSH_3_LABEL);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_3_LABEL0,
                             label_list[0] & 0xFFFFF);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_3_LABEL1,
                             label_list[1] & 0xFFFFF);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_3_LABEL2,
                             label_list[2] & 0xFFFFF);
        break;
    }
  }
};

#define MPLS_EXP_REWRITE_ENTRY                                              \
  std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_MPLS_EXP_REWRITE), \
                                     _ActionEntry(smi_id::T_MPLS_EXP_REWRITE))

class mpls_fib : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MPLS_FIB;
  static const switch_attr_id_t status_attr_id = SWITCH_MPLS_FIB_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MPLS_FIB_ATTR_PARENT_HANDLE;
  switch_tunnel_mode_t switch_mpls_pop_ttl_mode_to_tunnel_mode(
      switch_enum_t switch_tunnel_mode) {
    switch (switch_tunnel_mode.enumdata) {
      case SWITCH_MPLS_ATTR_POP_TTL_MODE_UNIFORM_MODEL:
        return SWITCH_TUNNEL_MODE_UNIFORM;
        break;
      case SWITCH_MPLS_ATTR_POP_TTL_MODE_PIPE_MODEL:
        return SWITCH_TUNNEL_MODE_PIPE;
        break;
      default:
        return SWITCH_TUNNEL_MODE_UNIFORM;
    }
  }
  switch_tunnel_mode_t switch_mpls_pop_qos_mode_to_tunnel_mode(
      switch_enum_t switch_tunnel_mode) {
    switch (switch_tunnel_mode.enumdata) {
      case SWITCH_MPLS_ATTR_POP_QOS_MODE_UNIFORM_MODEL:
        return SWITCH_TUNNEL_MODE_UNIFORM;
        break;
      case SWITCH_MPLS_ATTR_POP_QOS_MODE_PIPE_MODEL:
        return SWITCH_TUNNEL_MODE_PIPE;
        break;
      default:
        return SWITCH_TUNNEL_MODE_UNIFORM;
    }
  }

 public:
  mpls_fib(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_MPLS_FIB,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t packet_action = {0}, ttl_mode = {0}, qos_mode = {0};
    status |= switch_store::v_get(
        parent, SWITCH_MPLS_ATTR_PACKET_ACTION, packet_action);
    switch_object_id_t nhop_rif_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_MPLS_ATTR_NEXTHOP_RIF_HANDLE, nhop_rif_handle);
    switch_object_type_t ot = switch_store::object_type_query(nhop_rif_handle);
    status |=
        switch_store::v_get(parent, SWITCH_MPLS_ATTR_POP_QOS_MODE, qos_mode);
    match_key.set_exact(
        smi_id::F_MPLS_FIB_LOOKUP_LABEL, parent, SWITCH_MPLS_ATTR_LABEL);
    if (packet_action.enumdata == SWITCH_MPLS_ATTR_PACKET_ACTION_TRAP) {
      action_entry.init_action_data(smi_id::A_MPLS_FIB_MPLS_TRAP);
      return;
    } else if (packet_action.enumdata == SWITCH_MPLS_ATTR_PACKET_ACTION_DROP) {
      action_entry.init_action_data(smi_id::A_MPLS_FIB_MPLS_DROP);
      return;
    }
    if (ot == SWITCH_OBJECT_TYPE_RIF) {
      switch_object_id_t vrf_handle = {};
      action_entry.init_action_data(smi_id::A_MPLS_FIB_MPLS_TERM);
      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_POP_COUNT,
                           parent,
                           SWITCH_MPLS_ATTR_NUM_POP);

      switch_store::v_get(
          nhop_rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle);
      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_VRF,
                           compute_vrf(vrf_handle));

      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_V4_UNICAST_EN,
                           nhop_rif_handle,
                           SWITCH_RIF_ATTR_IPV4_UNICAST);
      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_V6_UNICAST_EN,
                           nhop_rif_handle,
                           SWITCH_RIF_ATTR_IPV6_UNICAST);
      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_BD_LABEL,
                           nhop_rif_handle,
                           SWITCH_RIF_ATTR_INGRESS_VLAN_RIF_LABEL);

      switch_object_id_t bd_handle = {};
      status |=
          find_auto_oid(nhop_rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_TERM_BD,
                           compute_bd(bd_handle));

      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= switch_store::v_get(
            parent, SWITCH_MPLS_ATTR_POP_TTL_MODE, ttl_mode);
        status |= action_entry.set_arg(
            smi_id::P_MPLS_FIB_MPLS_TERM_TTL_MODE,
            switch_mpls_pop_ttl_mode_to_tunnel_mode(ttl_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_MPLS_FIB_MPLS_TERM_QOS_MODE,
            switch_mpls_pop_qos_mode_to_tunnel_mode(qos_mode));
      }
    }

    if (ot == SWITCH_OBJECT_TYPE_NEXTHOP || ot == SWITCH_OBJECT_TYPE_ECMP) {
      uint8_t pop_count = 0;
      switch_store::v_get(parent, SWITCH_MPLS_ATTR_NUM_POP, pop_count);

      if (ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
        switch_enum_t nhop_type = {0};
        switch_store::v_get(
            nhop_rif_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
        if (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_MPLS) {
          switch_enum_t labelop = {0};
          switch_store::v_get(
              nhop_rif_handle, SWITCH_NEXTHOP_ATTR_LABELOP, labelop);
          // In the Swap case with single MPLS label, SONiC may set num_pop = 1
          // (even when pop is not needed). We will override this to avoid pop
          // and just do swap instead.
          if (pop_count == 1 &&
              labelop.enumdata == SWITCH_NEXTHOP_ATTR_LABELOP_SWAP) {
            pop_count = 0;
          }
        }
      } else {
        // In the case of ECMP, we can have both IP and MPLS nexthops.
        // We currently support only MPLS nexthops and not mix of both.
        std::vector<switch_object_id_t> ecmp_members;
        switch_store::v_get(
            nhop_rif_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);
        for (auto const &mbr : ecmp_members) {
          switch_object_id_t mbr_nhop_handle = {0};
          switch_store::v_get(
              mbr, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, mbr_nhop_handle);
          switch_enum_t mbr_nhop_type = {0};
          switch_store::v_get(
              mbr_nhop_handle, SWITCH_NEXTHOP_ATTR_TYPE, mbr_nhop_type);

          if (mbr_nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_MPLS) {
            switch_enum_t labelop = {0};
            switch_store::v_get(
                mbr_nhop_handle, SWITCH_NEXTHOP_ATTR_LABELOP, labelop);

            // If any of the MPLS member nexthops are performing SWAP, we will
            // override pop_count. Mix of swap/push cases are not supported.
            if (pop_count == 1 &&
                labelop.enumdata == SWITCH_NEXTHOP_ATTR_LABELOP_SWAP) {
              pop_count = 0;
              break;
            }
          }
        }
      }

      if (pop_count != 0) {
        action_entry.init_action_data(smi_id::A_MPLS_FIB_MPLS_PHP);
        action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_PHP_POP_COUNT, pop_count);
        action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_PHP_NEXTHOP,
                             compute_nexthop_index(nhop_rif_handle));
        if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          status |= switch_store::v_get(
              parent, SWITCH_MPLS_ATTR_POP_TTL_MODE, ttl_mode);
          status |= action_entry.set_arg(
              smi_id::P_MPLS_FIB_MPLS_PHP_TTL_MODE,
              switch_mpls_pop_ttl_mode_to_tunnel_mode(ttl_mode));
        }
        if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          status |= switch_store::v_get(
              parent, SWITCH_MPLS_ATTR_POP_QOS_MODE, qos_mode);
          status |= action_entry.set_arg(
              smi_id::P_MPLS_FIB_MPLS_PHP_QOS_MODE,
              switch_mpls_pop_qos_mode_to_tunnel_mode(qos_mode));
        }
      }
      if (pop_count == 0) {
        action_entry.init_action_data(smi_id::A_MPLS_FIB_MPLS_SWAP);
        action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_SWAP_POP_COUNT, pop_count);
        action_entry.set_arg(smi_id::P_MPLS_FIB_MPLS_SWAP_NEXTHOP,
                             compute_nexthop_index(nhop_rif_handle));
      }
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0, bytes = 0;
    switch_object_id_t nhop_rif_handle = {0};
    switch_enum_t packet_action = {0};

    status = p4_object_match_action::data_get();

    status |= switch_store::v_get(
        handle, SWITCH_MPLS_ATTR_PACKET_ACTION, packet_action);
    status |= switch_store::v_get(
        handle, SWITCH_MPLS_ATTR_NEXTHOP_RIF_HANDLE, nhop_rif_handle);
    switch_object_type_t ot = switch_store::object_type_query(nhop_rif_handle);

    if (packet_action.enumdata == SWITCH_MPLS_ATTR_PACKET_ACTION_TRAP) {
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                           smi_id::A_MPLS_FIB_MPLS_TRAP,
                           &pkts);
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                           smi_id::A_MPLS_FIB_MPLS_TRAP,
                           &bytes);
    } else if (packet_action.enumdata == SWITCH_MPLS_ATTR_PACKET_ACTION_DROP) {
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                           smi_id::A_MPLS_FIB_MPLS_DROP,
                           &pkts);
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                           smi_id::A_MPLS_FIB_MPLS_DROP,
                           &bytes);
    } else if (ot == SWITCH_OBJECT_TYPE_RIF) {
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                           smi_id::A_MPLS_FIB_MPLS_TERM,
                           &pkts);
      action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                           smi_id::A_MPLS_FIB_MPLS_TERM,
                           &bytes);
    } else if (ot == SWITCH_OBJECT_TYPE_NEXTHOP ||
               ot == SWITCH_OBJECT_TYPE_ECMP) {
      uint8_t pop_count = 0;
      switch_store::v_get(handle, SWITCH_MPLS_ATTR_NUM_POP, pop_count);
      if (pop_count != 0) {
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                             smi_id::A_MPLS_FIB_MPLS_PHP,
                             &pkts);
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                             smi_id::A_MPLS_FIB_MPLS_PHP,
                             &bytes);
      } else {
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                             smi_id::A_MPLS_FIB_MPLS_SWAP,
                             &pkts);
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                             smi_id::A_MPLS_FIB_MPLS_SWAP,
                             &bytes);
      }
    }
    switch_counter_t cntr_pkts;
    cntr_pkts.counter_id = SWITCH_MPLS_COUNTER_ID_PKTS;
    cntr_pkts.count = pkts;
    cntrs[SWITCH_MPLS_COUNTER_ID_PKTS] = cntr_pkts;
    switch_counter_t cntr_bytes;
    cntr_bytes.counter_id = SWITCH_MPLS_COUNTER_ID_BYTES;
    cntr_bytes.count = bytes;
    cntrs[SWITCH_MPLS_COUNTER_ID_BYTES] = cntr_bytes;
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS, value);
    action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES, value);

    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action::data_get();

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_MPLS_COUNTER_ID_PKTS:
          action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                               value);
          break;
        case SWITCH_MPLS_COUNTER_ID_BYTES:
          action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                               value);
          break;
        default:
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MPLS,
                     "{}.{}: Invalid counter ID",
                     __func__,
                     __LINE__);
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t cntr_pkts = 0, cntr_bytes = 0;
    switch_object_id_t nhop_rif_handle = {0};
    switch_enum_t packet_action = {0};

    if (get_auto_oid() == 0) return status;

    status |= switch_store::v_get(
        parent, SWITCH_MPLS_ATTR_PACKET_ACTION, packet_action);
    status |= switch_store::v_get(
        parent, SWITCH_MPLS_ATTR_NEXTHOP_RIF_HANDLE, nhop_rif_handle);
    switch_object_type_t ot = switch_store::object_type_query(nhop_rif_handle);

    // MAU counter
    std::vector<uint64_t> cntr_list;
    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      if (packet_action.enumdata == SWITCH_MPLS_ATTR_PACKET_ACTION_TRAP) {
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                             smi_id::A_MPLS_FIB_MPLS_TRAP,
                             &cntr_pkts);
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                             smi_id::A_MPLS_FIB_MPLS_TRAP,
                             &cntr_bytes);
      } else if (packet_action.enumdata ==
                 SWITCH_MPLS_ATTR_PACKET_ACTION_DROP) {
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                             smi_id::A_MPLS_FIB_MPLS_DROP,
                             &cntr_pkts);
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                             smi_id::A_MPLS_FIB_MPLS_DROP,
                             &cntr_bytes);
      } else if (ot == SWITCH_OBJECT_TYPE_RIF) {
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                             smi_id::A_MPLS_FIB_MPLS_TERM,
                             &cntr_pkts);
        action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                             smi_id::A_MPLS_FIB_MPLS_TERM,
                             &cntr_bytes);
      } else if (ot == SWITCH_OBJECT_TYPE_NEXTHOP ||
                 ot == SWITCH_OBJECT_TYPE_ECMP) {
        uint8_t pop_count = 0;
        switch_store::v_get(parent, SWITCH_MPLS_ATTR_NUM_POP, pop_count);
        if (pop_count != 0) {
          action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                               smi_id::A_MPLS_FIB_MPLS_PHP,
                               &cntr_pkts);
          action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                               smi_id::A_MPLS_FIB_MPLS_PHP,
                               &cntr_bytes);
        } else {
          action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS,
                               smi_id::A_MPLS_FIB_MPLS_SWAP,
                               &cntr_pkts);
          action_entry.get_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                               smi_id::A_MPLS_FIB_MPLS_SWAP,
                               &cntr_bytes);
        }
      }

      cntr_list.push_back(static_cast<uint64_t>(cntr_pkts));
      cntr_list.push_back(static_cast<uint64_t>(cntr_bytes));

      attr_w cntr_attr_list(SWITCH_MPLS_FIB_ATTR_MAU_STATS_CACHE);
      cntr_attr_list.v_set(cntr_list);
      switch_store::attribute_set(get_auto_oid(), cntr_attr_list);
    }

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t cntr_pkts = 0, cntr_bytes = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> cntr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_MPLS_FIB_ATTR_MAU_STATS_CACHE, cntr_list);
    if (cntr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MPLS_FIB,
                 "{}.{}: No stat cache to restore mau stats, "
                 "mpls_fib cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (cntr_list.size() == SWITCH_MPLS_COUNTER_ID_MAX) {
      cntr_pkts = cntr_list[SWITCH_MPLS_COUNTER_ID_PKTS];
      cntr_bytes = cntr_list[SWITCH_MPLS_COUNTER_ID_BYTES];
    }

    action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS, cntr_pkts);
    action_entry.set_arg(smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES,
                         cntr_bytes);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "mpls_fib status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

switch_status_t mpls_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  REGISTER_OBJECT(mpls_fib, SWITCH_OBJECT_TYPE_MPLS_FIB);
  REGISTER_OBJECT(mpls_label, SWITCH_OBJECT_TYPE_MPLS_LABEL);
  return status;
}

} /* namespace smi */
