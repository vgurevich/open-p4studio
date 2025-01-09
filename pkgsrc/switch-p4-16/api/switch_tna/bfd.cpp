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
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/bf_pal/pltfm_intf.h>
#include <pipe_mgr/pktgen_intf.h>
}
#include <utility>
#include <vector>
#include <set>
#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"
#include "s3/switch_packet.h"
#include "s3/switch_bfdd.h"

#include "../../s3/switch_utils.h"
#include "../../s3/bfdd.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

#define PKTGEN_APP_ID_BFD_V4 0
#define PKTGEN_APP_ID_BFD_V6 1

/* pktgen buffer supports 16B aligned writes/reads */
#define PKTGEN_PKTBUF_BFD_V4_OFF 0
#define PKTGEN_PKTBUF_BFD_V4_LEN 74
#define PKTGEN_PKTBUF_BFD_V6_OFF 80
#define PKTGEN_PKTBUF_BFD_V6_LEN 94

#define BFD_BYPASS_TX (uint16_t)(1u << 15)

static uint32_t timer_denominator_usec = 250000;
static uint16_t max_sessions = 4096;

struct PACKED bfd_pktgen_payload_v4_t {
  uint8_t eth_src[6];
  switch_packet_header_t cpu;
  uint16_t ether_type;
  switch_ip_header_t ip;
  switch_udp_header_t udp;
  switch_bfd_header_t bfd;
};

struct PACKED bfd_pktgen_payload_v6_t {
  uint8_t eth_src[6];
  switch_packet_header_t cpu;
  uint16_t ether_type;
  switch_ip6_header_t ip;
  switch_udp_header_t udp;
  switch_bfd_header_t bfd;
};

static uint32_t bfd_max_pipes;

static void get_trap_group_cfg(const switch_object_id_t trap_group_handle,
                               switch_object_id_t &meter_handle,
                               uint8_t &qid) {
  switch_object_id_t cpu_queue_handle = {};

  if (trap_group_handle.data) {
    switch_store::v_get(trap_group_handle,
                        SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE,
                        cpu_queue_handle);
    switch_store::v_get(trap_group_handle,
                        SWITCH_HOSTIF_TRAP_GROUP_ATTR_POLICER_HANDLE,
                        meter_handle);
  }
  if (cpu_queue_handle.data)
    switch_store::v_get(cpu_queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
}

static switch_status_t update_rx_timer(uint16_t dev_id,
                                       uint8_t pipe,
                                       uint32_t session_id,
                                       uint8_t value) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = pipe};
  _Table table(tgt, get_bf_rt_info(), smi_id::T_BFD_RX_TIMER_REG);
  _MatchKey register_key(smi_id::T_BFD_RX_TIMER_REG);
  _ActionEntry register_action(smi_id::T_BFD_RX_TIMER_REG);

  register_action.init_indirect_data();
  status |=
      register_key.set_exact(smi_id::F_BFD_RX_TIMER_REG_INDEX, session_id);
  status |= register_action.set_arg(smi_id::D_BFD_RX_TIMER_REG_DATA, value);
  status |= table.entry_modify(register_key, register_action);

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to reset bfd rx timer,"
               " status {} dev_id {} pipe {} session_id {} value {}",
               __func__,
               __LINE__,
               status,
               dev_id,
               pipe,
               session_id,
               value);
  }

  return status;
}

struct bfd_session_common {
  bfd_session_common(switch_object_id_t parent, switch_status_t &status) {
    session_id = switch_store::handle_to_id(parent);
    if (session_id >= (1u << BFD_SESSION_ID_WIDTH)) {
      status |= SWITCH_STATUS_FAILURE;
      return;
    }
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_IS_OFFLOADED, is_offloaded);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_DESIGNATED_PIPE, designated_pipe);
    status |=
        switch_store::v_get(parent,
                            SWITCH_BFD_SESSION_ATTR_DESIGNATED_RECIRC_PORT,
                            designated_recirc_port);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_SRC_IP_ADDRESS, src_ip);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT, udp_src_port);
  }

  uint64_t session_id{};
  uint16_t dev_id{};
  uint8_t designated_pipe{};
  uint16_t designated_recirc_port{};
  switch_object_id_t device_handle{};
  switch_ip_address_t src_ip{};
  uint16_t udp_src_port{};
  bool is_offloaded{};
};

class bfd_tx_session : public p4_object_match_action, bfd_session_common {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BFD_TX_SESSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_TX_SESSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_TX_SESSION_ATTR_PARENT_HANDLE;

 public:
  bfd_tx_session(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_BFD_TX_SESSION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent),
        bfd_session_common(parent, status) {
    uint32_t tx_mult = 0, negotiated_tx = 0;
    uint16_t udp_dst_port = SWITCH_BFD_UDP_DSTPORT;
    switch_object_id_t vrf_handle = {};
    uint16_t vrf = 0;
    switch_ip_address_t dst_ip = {};
    bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = designated_pipe};
    device_tgt_set(tgt);

    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_VRF_HANDLE, vrf_handle);
    vrf = compute_vrf(vrf_handle);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_DST_IP_ADDRESS, dst_ip);

    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_NEGOTIATED_TX, negotiated_tx);

    tx_mult = (negotiated_tx / timer_denominator_usec);
    /* XXX see pktgen jitter */
    tx_mult = (tx_mult * BFD_TX_JITTER);
    tx_mult = (tx_mult > 255 ? 255 : tx_mult);

    status |=
        match_key.set_exact(smi_id::F_BFD_TX_SESSION_ID, (session_id - 1));
    if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      action_entry.init_action_data(smi_id::A_BFD_TX_SESSION_V4);

      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_SESSION_ID,
                                     session_id);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_VRF, vrf);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_SIP, src_ip);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_DIP, dst_ip);
      status |=
          action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_TX_MULT, tx_mult);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_SPORT,
                                     parent,
                                     SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT);
      status |=
          action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V4_DPORT, udp_dst_port);
    } else if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      action_entry.init_action_data(smi_id::A_BFD_TX_SESSION_V6);

      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_SESSION_ID,
                                     session_id);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_VRF, vrf);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_SIP, src_ip);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_DIP, dst_ip);
      status |=
          action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_TX_MULT, tx_mult);
      status |= action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_SPORT,
                                     parent,
                                     SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT);
      status |=
          action_entry.set_arg(smi_id::P_BFD_TX_SESSION_V6_DPORT, udp_dst_port);
    }
  }
  switch_status_t create_update() {
    if (!is_offloaded) {
      return SWITCH_STATUS_SUCCESS;
    }
    return p4_object_match_action::create_update();
  }
};

class bfd_rx_session : public p4_object_match_action_list, bfd_session_common {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BFD_RX_SESSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_RX_SESSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_RX_SESSION_ATTR_PARENT_HANDLE;
  uint32_t rx_mult = 0;

 public:
  bfd_rx_session(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_BFD_RX_SESSION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent),
        bfd_session_common(parent, status) {
    uint8_t bfd_version = SWITCH_BFD_VERSION;
    uint32_t negotiated_rx = 0;
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_NEGOTIATED_RX, negotiated_rx);
    rx_mult = (negotiated_rx / timer_denominator_usec);
    rx_mult = (rx_mult > 255 ? 255 : rx_mult);

    auto it = match_action_list.begin();
    auto add_entry = [&](uint8_t flags) {
      it =
          match_action_list.insert(it,
                                   std::pair<_MatchKey, _ActionEntry>(
                                       _MatchKey(smi_id::T_BFD_RX_SESSION),
                                       _ActionEntry(smi_id::T_BFD_RX_SESSION)));
      status |=
          it->first.set_exact(smi_id::F_BFD_RX_SESSION_HDR_BFD_MY_DISCRIMINATOR,
                              parent,
                              SWITCH_BFD_SESSION_ATTR_REMOTE_DISCRIMINATOR);
      status |= it->first.set_exact(
          smi_id::F_BFD_RX_SESSION_HDR_BFD_YOUR_DISCRIMINATOR,
          parent,
          SWITCH_BFD_SESSION_ATTR_LOCAL_DISCRIMINATOR);
      status |= it->first.set_exact(smi_id::F_BFD_RX_SESSION_HDR_BFD_VERSION,
                                    bfd_version);
      status |=
          it->first.set_exact(smi_id::F_BFD_RX_SESSION_HDR_BFD_FLAGS, flags);
      status |= it->first.set_exact(
          smi_id::F_BFD_RX_SESSION_HDR_BFD_DESIRED_MIN_TX_INTERVAL,
          parent,
          SWITCH_BFD_SESSION_ATTR_REMOTE_MIN_TX);
      status |= it->first.set_exact(
          smi_id::F_BFD_RX_SESSION_HDR_BFD_REQ_MIN_RX_INTERVAL,
          parent,
          SWITCH_BFD_SESSION_ATTR_REMOTE_MIN_RX);

      it->second.init_action_data(smi_id::A_BFD_RX_SESSION_INFO);
      status |=
          it->second.set_arg(smi_id::P_BFD_RX_SESSION_INFO_RX_MULT, rx_mult);
      status |= it->second.set_arg(smi_id::P_BFD_RX_SESSION_INFO_SESSION_ID,
                                   session_id);

      status |= it->second.set_arg(smi_id::P_BFD_RX_SESSION_INFO_PKTGEN_PIPE,
                                   designated_pipe);
      status |= it->second.set_arg(smi_id::P_BFD_RX_SESSION_INFO_RECIRC_PORT,
                                   designated_recirc_port);
    };
    /* flags is the second byte of bfd ctl pkt, RFC 5880 section 4.1:
     *   [7:6]=Status
     *   [5]=Poll
     *   [4]=Final
     *   [3]=Control Plane Independent
     *   [2]=Authentication present
     *   [1]=Demand
     *   [0]=Multipoint (reserved)
     */
    /* state up */
    add_entry(0b11000000);
  }
  switch_status_t create_update() {
    if (!is_offloaded) {
      return SWITCH_STATUS_SUCCESS;
    }
    update_rx_timer(dev_id, designated_pipe, session_id, rx_mult);
    return p4_object_match_action_list::create_update();
  }
  switch_status_t del() {
    switch_status_t status = p4_object_match_action_list::del();
    update_rx_timer(dev_id, designated_pipe, session_id, 0);
    return status;
  }
};

class bfd_tx_timer : public p4_object_match_action, bfd_session_common {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BFD_TX_TIMER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_TX_TIMER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_TX_TIMER_ATTR_PARENT_HANDLE;

 public:
  bfd_tx_timer(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_BFD_TX_TIMER,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent),
        bfd_session_common(parent, status) {
    status |= match_key.set_exact(
        smi_id::F_BFD_TX_TIMER_LOCAL_MD_BFD_SESSION_ID, (session_id - 1));

    action_entry.init_action_data(smi_id::A_BFD_TX_TIMER_CHECK);
    status |= action_entry.set_arg(smi_id::P_BFD_TX_TIMER_CHECK_SESSION_ID,
                                   session_id);
    status |= action_entry.set_arg(smi_id::P_BFD_TX_TIMER_CHECK_DETECT_MULTI,
                                   parent,
                                   SWITCH_BFD_SESSION_ATTR_MULTIPLIER);
    status |=
        action_entry.set_arg(smi_id::P_BFD_TX_TIMER_CHECK_MY_DISCRIMINATOR,
                             parent,
                             SWITCH_BFD_SESSION_ATTR_LOCAL_DISCRIMINATOR);
    status |=
        action_entry.set_arg(smi_id::P_BFD_TX_TIMER_CHECK_YOUR_DISCRIMINATOR,
                             parent,
                             SWITCH_BFD_SESSION_ATTR_REMOTE_DISCRIMINATOR);
    status |= action_entry.set_arg(
        smi_id::P_BFD_TX_TIMER_CHECK_DESIRED_MIN_TX_INTERVAL,
        parent,
        SWITCH_BFD_SESSION_ATTR_MIN_TX);
    status |=
        action_entry.set_arg(smi_id::P_BFD_TX_TIMER_CHECK_REQ_MIN_RX_INTERVAL,
                             parent,
                             SWITCH_BFD_SESSION_ATTR_MIN_RX);
  }

  switch_status_t create_update() {
    if (!is_offloaded) {
      return SWITCH_STATUS_SUCCESS;
    }
    return p4_object_match_action::create_update();
  }
};

class bfd_rx_timer : public p4_object_match_action_list, bfd_session_common {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BFD_RX_TIMER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_RX_TIMER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_RX_TIMER_ATTR_PARENT_HANDLE;

 public:
  bfd_rx_timer(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_BFD_RX_TIMER,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent),
        bfd_session_common(parent, status) {
    bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = designated_pipe};
    device_tgt_set(tgt);

    auto it = match_action_list.begin();
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_BFD_RX_TIMER),
                                      _ActionEntry(smi_id::T_BFD_RX_TIMER)));
    status |= it->first.set_exact(smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_PKT_TX,
                                  static_cast<uint8_t>(0));
    status |= it->first.set_exact(
        smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_SESSION_ID, session_id);
    it->second.init_action_data(smi_id::A_BFD_RX_TIMER_RESET);
    status |=
        it->second.set_arg(smi_id::P_BFD_RX_TIMER_RESET_SESSION_ID, session_id);

    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_BFD_RX_TIMER),
                                      _ActionEntry(smi_id::T_BFD_RX_TIMER)));
    status |= it->first.set_exact(smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_PKT_TX,
                                  static_cast<uint8_t>(1));
    status |= it->first.set_exact(
        smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_SESSION_ID, session_id);
    it->second.init_action_data(smi_id::A_BFD_RX_TIMER_CHECK);
    status |=
        it->second.set_arg(smi_id::P_BFD_RX_TIMER_CHECK_SESSION_ID, session_id);
  }

  switch_status_t create_update() {
    if (!is_offloaded) {
      return SWITCH_STATUS_SUCCESS;
    }
    return p4_object_match_action_list::create_update();
  }
};

class bfd_pkt_action : public auto_object {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BFD_PKT_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_PKT_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_PKT_ACTION_ATTR_PARENT_HANDLE;

  using ma_list = std::vector<std::pair<_MatchKey, _ActionEntry>>;
  uint16_t dev_id = {0};

  switch_status_t entry_list_create(uint16_t pipe, ma_list &match_action_list) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    ma_list::iterator it;
    auto add_entry = [&](uint8_t pkt_tx,
                         uint8_t pkt_action,
                         uint8_t pktgen_pipe,
                         uint8_t pktgen_pipe_mask,
                         uint32_t prio) {
      it =
          match_action_list.insert(match_action_list.end(),
                                   std::pair<_MatchKey, _ActionEntry>(
                                       _MatchKey(smi_id::T_BFD_PKT_ACTION),
                                       _ActionEntry(smi_id::T_BFD_PKT_ACTION)));
      status |= it->first.set_exact(smi_id::F_BFD_PKT_ACTION_PRIORITY, prio);
      status |= it->first.set_exact(
          smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_TX, pkt_tx);
      status |= it->first.set_exact(
          smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_ACTION, pkt_action);
      status |= it->first.set_ternary(
          smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKTGEN_PIPE,
          pktgen_pipe,
          pktgen_pipe_mask);
    };

    add_entry(0, BFD_PKT_ACTION_DROP, pipe, 0xf, 0);
    it->second.init_action_data(smi_id::A_BFD_PKT_ACTION_BFD_DROP_PKT);

    add_entry(0, BFD_PKT_ACTION_DROP, 0, 0, 1);
    it->second.init_action_data(
        smi_id::A_BFD_PKT_ACTION_BFD_RECIRC_TO_PKTGEN_PIPE);

    add_entry(1, BFD_PKT_ACTION_NORMAL, 0, 0, 1);
    it->second.init_action_data(smi_id::A_BFD_PKT_ACTION_BFD_TX_PKT);

    add_entry(1, BFD_PKT_ACTION_TIMEOUT, 0, 0, 1);
    it->second.init_action_data(smi_id::A_BFD_PKT_ACTION_BFD_PKT_TO_CPU);

    return status;
  }

  switch_status_t table_entry_update(bool is_add,
                                     _Table &table,
                                     _MatchKey &match_key,
                                     _ActionEntry &action_entry) {
    bool bf_rt_status{};
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (is_add) {
      status = table.entry_add(match_key, action_entry, bf_rt_status, false);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_BFD_SESSION,
                   "{}:{}: failed table.entry_add status {}",
                   __func__,
                   __LINE__,
                   status);
      }
    } else {
      status = table.entry_modify(match_key, action_entry, false);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_BFD_SESSION,
                   "{}:{}: failed table.entry_modify status {}",
                   __func__,
                   __LINE__,
                   status);
      }
    }

    return status;
  }

 public:
  bfd_pkt_action(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(parent, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool is_add = get_auto_oid() == 0;
    for (auto pipe : get_active_pipes()) {
      ma_list match_action_list;
      bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = pipe};
      _Table tbl{tgt, get_bf_rt_info(), smi_id::T_BFD_PKT_ACTION};

      status |= entry_list_create(pipe, match_action_list);
      for (auto &ma : match_action_list) {
        status |= table_entry_update(is_add, tbl, ma.first, ma.second);
      }
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (auto pipe : get_active_pipes()) {
      ma_list match_action_list;
      bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = pipe};
      _Table tbl{tgt, get_bf_rt_info(), smi_id::T_BFD_PKT_ACTION};

      status |= entry_list_create(pipe, match_action_list);
      for (auto &ma : match_action_list) {
        status |= tbl.entry_delete(ma.first);
      }
    }
    status |= auto_object::del();
    return status;
  }
};

class bfd_offload_trap : public p4_object_match_action {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_BFD_OFFLOAD_TRAP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BFD_OFFLOAD_TRAP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_OFFLOAD_TRAP_ATTR_PARENT_HANDLE;

 public:
  bfd_offload_trap(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t ing_target_type = {
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
    switch_object_id_t dflt_trap_grp = {};
    switch_object_id_t dflt_trap_grp_meter{};
    uint8_t dflt_trap_grp_qid = 0;
    uint8_t bfd_to_cpu = 1;
    uint8_t bfd_to_cpu_mask = 1;
    uint32_t prio = 0;

    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP, dflt_trap_grp);
    get_trap_group_cfg(dflt_trap_grp, dflt_trap_grp_meter, dflt_trap_grp_qid);

    /* internal mechanism, so putting highest prio TODO */
    /* TODO put into high prio acl? to have priorities predictable */
    status |= match_key.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, prio);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU, bfd_to_cpu, bfd_to_cpu_mask);
    action_entry.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU);
    status |= action_entry.set_arg(
        smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_REASON_CODE,
        static_cast<uint16_t>(SWITCH_BFD_REASON_CODE));
    status |= action_entry.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_QID,
                                   dflt_trap_grp_qid);
    status |= action_entry.set_arg(
        smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_METER_ID, dflt_trap_grp_meter);
    if (dflt_trap_grp_meter.data != 0) {
      status |= switch_store::v_set(
          dflt_trap_grp_meter, SWITCH_METER_ATTR_TARGET_TYPE, ing_target_type);
    }
    /* TODO false ?? */
    status |= action_entry.set_arg(
        smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_DISABLE_LEARNING, false);
  }
};

class bfd_session_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_BFD_SESSION_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BFD_SESSION_HELPER_ATTR_PARENT_HANDLE;
  switch_ip_address_t src_ip = {}, dst_ip = {};
  uint32_t local_discriminator = 0, min_tx = 0, min_rx = 0;
  uint8_t multiplier = 0;
  switch_enum_t session_type = {};
  uint16_t udp_src_port = 0;

 public:
  bfd_session_helper(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t vrf_handle = {};

    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_VRF_HANDLE, vrf_handle);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_DST_IP_ADDRESS, dst_ip);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_SRC_IP_ADDRESS, src_ip);
    status |= switch_store::v_get(parent,
                                  SWITCH_BFD_SESSION_ATTR_LOCAL_DISCRIMINATOR,
                                  local_discriminator);
    status |=
        switch_store::v_get(parent, SWITCH_BFD_SESSION_ATTR_MIN_RX, min_rx);
    status |=
        switch_store::v_get(parent, SWITCH_BFD_SESSION_ATTR_MIN_TX, min_tx);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_MULTIPLIER, multiplier);
    status |=
        switch_store::v_get(parent, SWITCH_BFD_SESSION_ATTR_TYPE, session_type);
    status |= switch_store::v_get(
        parent, SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT, udp_src_port);
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status = switch_bfdd_session_create(
        get_parent().data,
        static_cast<switch_bfd_session_type_t>(session_type.enumdata),
        local_discriminator,
        min_tx,
        min_rx,
        multiplier,
        udp_src_port,
        src_ip,
        dst_ip);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}.{}: Failed to create BFD session dst_ip {}, local_disc {} "
                 "type {}, status {}",
                 __func__,
                 __LINE__,
                 dst_ip,
                 local_discriminator,
                 session_type.enumdata,
                 status);
      return status;
    }
    return auto_object::create_update();
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status = switch_bfdd_session_delete(local_discriminator, src_ip, dst_ip);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}.{}: Failed to create BFD session dst_ip {}, local_disc {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 dst_ip,
                 local_discriminator,
                 status);
      return status;
    }
    return auto_object::del();
  }
};

static switch_status_t bfd_session_count(bf_dev_id_t dev_id,
                                         bf_dev_pipe_t pipe,
                                         uint32_t &count) {
  uint32_t c = 0;
  bf_rt_target_t tgt{.dev_id = dev_id, .pipe_id = pipe};
  _Table tbl{tgt, get_bf_rt_info(), smi_id::T_BFD_TX_SESSION};
  switch_status_t status = tbl.table_usage_get(&c);
  if (status == SWITCH_STATUS_SUCCESS) {
    count = c;
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }
  return status;
}

static void bfd_pktgen_payload_v4_init(bfd_pktgen_payload_v4_t &p) {
  uint8_t marker_mac[6] = {0, 0, 0xBB, 0xFF, 0xDD, 0};
  memset(&p, 0, sizeof p);
  memcpy(&p.eth_src, marker_mac, 6);
  p.cpu.cpu_header.reason_code = htons(BFD_BYPASS_TX);
  p.ether_type = htons(0x0800);
  p.ip.version = 0x4;
  p.ip.ihl = 0x5;
  p.ip.tot_len = htons(52);
  p.ip.id = 0x1;
  p.ip.protocol = 0x11;
  p.udp.dst_port = htons(SWITCH_BFD_UDP_DSTPORT);
  p.udp.len = htons(32);
  p.bfd.state = 0b11000000;
  p.bfd.length = SWITCH_BFD_PKT_LEN;
  SWITCH_BFD_SET_VER(p.bfd.ver_diag, SWITCH_BFD_VERSION);
}

static void bfd_pktgen_payload_v6_init(bfd_pktgen_payload_v6_t &p) {
  uint8_t marker_mac[6] = {0, 0, 0xBB, 0xFF, 0xDD, 0};
  memset(&p, 0, sizeof p);
  memcpy(&p.eth_src, marker_mac, 6);
  p.cpu.cpu_header.reason_code = htons(BFD_BYPASS_TX);
  p.ether_type = htons(0x86DD);
  p.ip.version = 0x6;
  p.ip.nexthdr = 0x11;
  p.udp.dst_port = htons(SWITCH_BFD_UDP_DSTPORT);
  p.bfd.state = 0b11000000;
}

static switch_status_t bfd_pktgen_configure(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port) {
  bf_status_t sts;
  switch_status_t status;
  bfd_pktgen_payload_v4_t payload4;
  bfd_pktgen_payload_v6_t payload6;
  bf_dev_pipe_t pipe = DEV_PORT_TO_PIPE(dev_port);
  bf_dev_port_t local_port = DEV_PORT_TO_LOCAL_PORT(dev_port);
  bf_dev_target_t tgt{.device_id = dev_id, .dev_pipe_id = pipe};
  bf_pktgen_app_cfg_t cfg;

  memset(&cfg, 0, sizeof cfg);
  cfg.trigger_type = BF_PKTGEN_TRIGGER_TIMER_PERIODIC;
  cfg.batch_count = 0; /* 1 batch */
  cfg.packets_per_batch =
      (uint16_t)(max_sessions - 1); /* max bfd session count */
  cfg.u.timer_nanosec = timer_denominator_usec * 1000;
  cfg.pipe_local_source_port = local_port;
  cfg.tof2.assigned_chnl_id = (uint8_t)local_port;
  cfg.pkt_buffer_offset = PKTGEN_PKTBUF_BFD_V4_OFF;
  cfg.length = PKTGEN_PKTBUF_BFD_V4_LEN;

  bfd_pktgen_payload_v4_init(payload4);
  sts = bf_pktgen_write_pkt_buffer(get_bf_rt_session().sessHandleGet(),
                                   tgt,
                                   PKTGEN_PKTBUF_BFD_V4_OFF,
                                   sizeof payload4,
                                   reinterpret_cast<uint8_t *>(&payload4));
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to write pkt buffer dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }

  sts = bf_pktgen_cfg_app(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V4, &cfg);
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }

  cfg.pkt_buffer_offset = PKTGEN_PKTBUF_BFD_V6_OFF;
  cfg.length = PKTGEN_PKTBUF_BFD_V6_LEN;
  bfd_pktgen_payload_v6_init(payload6);
  sts = bf_pktgen_write_pkt_buffer(get_bf_rt_session().sessHandleGet(),
                                   tgt,
                                   PKTGEN_PKTBUF_BFD_V6_OFF,
                                   sizeof payload6,
                                   reinterpret_cast<uint8_t *>(&payload6));
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to write pkt buffer dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }

  sts = bf_pktgen_cfg_app(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V6, &cfg);
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }
  return status;
}

static switch_status_t bfd_pktgen_enable(bf_dev_id_t dev_id,
                                         bf_dev_pipe_t pipe) {
  bf_dev_target_t tgt{.device_id = dev_id, .dev_pipe_id = pipe};
  bf_status_t sts = bf_pktgen_app_enable(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V4);
  switch_status_t status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to enable v4 pktgen dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
    return status;
  }
  /*
  sts = bf_pktgen_app_enable(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V6);
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to enable v6 pktgen dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }
  */
  return status;
}

static switch_status_t bfd_pktgen_disable(bf_dev_id_t dev_id,
                                          bf_dev_pipe_t pipe) {
  bf_dev_target_t tgt{.device_id = dev_id, .dev_pipe_id = pipe};
  bf_status_t sts = bf_pktgen_app_disable(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V4);
  switch_status_t status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to disable v4 pktgen dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
    return status;
  }
  /*
  sts = bf_pktgen_app_disable(
      get_bf_rt_session().sessHandleGet(), tgt, PKTGEN_APP_ID_BFD_V6);
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to disable v6 pktgen dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);

  }
  */
  return status;
}

/* NOTE currently assume all session share sip */
static void bfd_pktgen_payload_set(uint16_t dev_id,
                                   uint8_t pipe,
                                   switch_ip_address_t ip,
                                   uint16_t src_port) {
  bf_status_t sts;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_dev_target_t tgt{.device_id = dev_id, .dev_pipe_id = pipe};

  if (ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    bfd_pktgen_payload_v4_t payload4;
    bfd_pktgen_payload_v4_init(payload4);
    payload4.ip.src_addr = htonl(ip.ip4);
    payload4.udp.src_port = htons(src_port);
    sts = bf_pktgen_write_pkt_buffer(get_bf_rt_session().sessHandleGet(),
                                     tgt,
                                     PKTGEN_PKTBUF_BFD_V4_OFF,
                                     sizeof payload4,
                                     reinterpret_cast<uint8_t *>(&payload4));
  } else {
    bfd_pktgen_payload_v6_t payload6;
    bfd_pktgen_payload_v6_init(payload6);
    payload6.udp.src_port = htons(src_port);
    memcpy(payload6.ip.src_addr, &ip.ip6, 16);
    sts = bf_pktgen_write_pkt_buffer(get_bf_rt_session().sessHandleGet(),
                                     tgt,
                                     PKTGEN_PKTBUF_BFD_V6_OFF,
                                     sizeof payload6,
                                     reinterpret_cast<uint8_t *>(&payload6));
  }
  status = pal_status_xlate(sts);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to write pkt buffer dev_id {} pipe {} sts {}",
               __func__,
               __LINE__,
               dev_id,
               pipe,
               status);
  }
}

static switch_status_t bfd_tables_create_update(
    const switch_object_id_t handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bfd_tx_session tx_session{handle, status};
  bfd_rx_session rx_session{handle, status};
  bfd_tx_timer tx_timer{handle, status};
  bfd_rx_timer rx_timer{handle, status};
  if (status == SWITCH_STATUS_SUCCESS) {
    status |= tx_session.create_update();
    status |= rx_session.create_update();
    status |= tx_timer.create_update();
    status |= rx_timer.create_update();
  }
  return status;
}

static switch_status_t bfd_tables_del(const switch_object_id_t handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bfd_tx_session tx_session{handle, status};
  bfd_rx_session rx_session{handle, status};
  bfd_tx_timer tx_timer{handle, status};
  bfd_rx_timer rx_timer{handle, status};
  if (status == SWITCH_STATUS_SUCCESS) {
    status |= tx_session.del();
    status |= rx_session.del();
    status |= tx_timer.del();
    status |= rx_timer.del();
  }
  return status;
}

static switch_status_t before_bfd_session_update(
    const switch_object_id_t handle, const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attr.id_get() == SWITCH_BFD_SESSION_ATTR_IS_OFFLOADED) {
    bool is_offloaded;
    uint32_t sess_count = 0;

    attr.v_get(is_offloaded);
    if (is_offloaded) {
      /* onload -> offload */
      return SWITCH_STATUS_SUCCESS;
    }

    bfd_session_common sess{handle, status};
    status |= bfd_session_count(sess.dev_id, sess.designated_pipe, sess_count);
    if (sess_count == 1) {
      /* if it's the last session in hw on the pipe,
       * turn pktgen app off there */
      status |= bfd_pktgen_disable(sess.dev_id, sess.designated_pipe);
    }
    status |= bfd_tables_del(handle);
    return status;
  }

  return status;
}

static switch_status_t after_bfd_session_update(const switch_object_id_t handle,
                                                const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attr.id_get() == SWITCH_BFD_SESSION_ATTR_IS_OFFLOADED) {
    bool is_offloaded;
    attr.v_get(is_offloaded);

    if (!is_offloaded) {
      /* offload -> onload */
      return SWITCH_STATUS_SUCCESS;
    }

    status = bfd_tables_create_update(handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      bfd_session_common sess{handle, status};
      bfd_pktgen_payload_set(
          sess.dev_id, sess.designated_pipe, sess.src_ip, sess.udp_src_port);
      status = bfd_pktgen_enable(sess.dev_id, sess.designated_pipe);
    }

    return status;
  }

  return status;
}

static switch_status_t after_bfd_session_create(const switch_object_id_t handle,
                                                const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)attrs;
  bfd_session_common sess{handle, status};
  if (sess.is_offloaded) {
    bfd_pktgen_payload_set(
        sess.dev_id, sess.designated_pipe, sess.src_ip, sess.udp_src_port);
    status |= bfd_pktgen_enable(sess.dev_id, sess.designated_pipe);
  }
  return status;
}

switch_status_t before_bfd_session_delete(const switch_object_id_t handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t sess_count = 0;
  bfd_session_common sess{handle, status};
  if (sess.is_offloaded) {
    status |= bfd_session_count(sess.dev_id, sess.designated_pipe, sess_count);
    if (sess_count == 1) {
      status |= bfd_pktgen_disable(sess.dev_id, sess.designated_pipe);
    }
  }
  return status;
}

static switch_status_t after_recirc_port_create(const switch_object_id_t handle,
                                                const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t port_type{};
  switch_object_id_t device_handle{};
  uint16_t dev_id{};
  uint16_t dev_port{};
  bf_dev_pipe_t pipe{};
  (void)attrs;

  status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
  if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_RECIRC) {
    return status;
  }

  status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_DEVICE, device_handle);
  status |=
      switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
  status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

  pipe = (bf_dev_pipe_t)DEV_PORT_TO_PIPE(dev_port);
  auto pipes = _Table(smi_id::T_BFD_TX_SESSION).get_active_pipes();
  if (pipes.find(pipe) == pipes.end()) {
    return status;
  }

  status |= bfd_pktgen_configure(dev_id, dev_port);

  return status;
}

static void bfd_update_session_params(switch_object_id_t handle,
                                      switch_bfdd_session_params_t params) {
  switch_store::v_set(handle,
                      SWITCH_BFD_SESSION_ATTR_REMOTE_DISCRIMINATOR,
                      params.remote_discriminator);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_NEGOTIATED_RX, params.negotiated_rx);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_NEGOTIATED_TX, params.negotiated_tx);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_REMOTE_MIN_RX, params.remote_min_rx);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_REMOTE_MIN_TX, params.remote_min_tx);
  switch_store::v_set(handle,
                      SWITCH_BFD_SESSION_ATTR_REMOTE_MULTIPLIER,
                      params.remote_multiplier);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_LOCAL_DIAG, params.local_diag);
  switch_store::v_set(
      handle, SWITCH_BFD_SESSION_ATTR_REMOTE_DIAG, params.remote_diag);
}

static void bfdd_session_state_cb(uint64_t session_id,
                                  switch_bfdd_session_params_t params) {
  switch_object_id_t device_handle = {};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t handle =
      switch_store::id_to_handle(SWITCH_OBJECT_TYPE_BFD_SESSION, session_id);
  switch_bfd_event_data_t ev{
      .bfd_session_state = params.state,
      .bfd_session_handle = handle,
  };

  status = switch_store::v_get(
      handle, SWITCH_BFD_SESSION_ATTR_DEVICE, device_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BFD_SESSION,
               "{}:{}: failed to get device handle, session_id bfd.{} state {}",
               __func__,
               __LINE__,
               session_id & 0xFFFF,
               params.state);
    return;
  }

  switch_log(
      SWITCH_API_LEVEL_DEBUG,
      SWITCH_OBJECT_TYPE_BFD_SESSION,
      "{}:{} session_id {} state {} remote_discriminator {} negotiated_rx {} "
      "negotiated_tx {} remote_min_rx {} remote_min_tx {} local_diag "
      "{} remote_diag {}",
      __func__,
      __LINE__,
      session_id,
      params.state,
      params.remote_discriminator,
      params.negotiated_rx,
      params.negotiated_tx,
      params.remote_min_rx,
      params.remote_min_tx,
      params.local_diag,
      params.remote_diag);

  if (params.state == SWITCH_BFD_SESSION_STATE_ADMIN_DOWN) {
    switch_store::v_set(
        handle,
        SWITCH_BFD_SESSION_ATTR_STATE,
        switch_enum_t{SWITCH_BFD_SESSION_ATTR_STATE_ADMIN_DOWN});
    smi::event::bfd_event_notify(ev);
    return;
  }
  if (params.state == SWITCH_BFD_SESSION_STATE_DOWN) {
    switch_store::v_set(handle,
                        SWITCH_BFD_SESSION_ATTR_STATE,
                        switch_enum_t{SWITCH_BFD_SESSION_ATTR_STATE_DOWN});
    switch_store::v_set(handle, SWITCH_BFD_SESSION_ATTR_IS_OFFLOADED, false);
    bfd_update_session_params(handle, params);
    smi::event::bfd_event_notify(ev);
    return;
  }
  if (params.state == SWITCH_BFD_SESSION_STATE_INIT) {
    switch_store::v_set(handle,
                        SWITCH_BFD_SESSION_ATTR_STATE,
                        switch_enum_t{SWITCH_BFD_SESSION_ATTR_STATE_INIT});
    smi::event::bfd_event_notify(ev);
    return;
  }
  if (params.state == SWITCH_BFD_SESSION_STATE_UP) {
    uint16_t dev_port = 0;
    uint8_t pipe = (uint8_t)(session_id % bfd_max_pipes);

    status = bfd_tables_del(handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}:{}: failed bfd_tables_del, session_id {} "
                 "state {} status {}",
                 __func__,
                 __LINE__,
                 session_id,
                 params.state,
                 status);
    }

    bfd_update_session_params(handle, params);

    status = bfd_tables_create_update(handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}:{}: failed to bfd_tables_create_update, session_id {} "
                 "state {} status {}",
                 __func__,
                 __LINE__,
                 session_id,
                 params.state,
                 status);
    }

    // NOTE assume each recirc is pktge
    status =
        get_recirc_port_in_pipe(device_handle, (bf_dev_pipe_t)pipe, dev_port);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}:{}: failed to get recirc port for pipe, session_id {} "
                 "state {} status {}",
                 __func__,
                 __LINE__,
                 session_id,
                 params.state,
                 status);
      return;
    }

    switch_store::v_set(handle, SWITCH_BFD_SESSION_ATTR_DESIGNATED_PIPE, pipe);
    switch_store::v_set(
        handle, SWITCH_BFD_SESSION_ATTR_DESIGNATED_RECIRC_PORT, dev_port);
    status =
        switch_store::v_set(handle, SWITCH_BFD_SESSION_ATTR_IS_OFFLOADED, true);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BFD_SESSION,
                 "{}:{}: failed to offload, session_id {} state {} status {}",
                 __func__,
                 __LINE__,
                 session_id,
                 params.state,
                 status);
      return;
    }

    switch_store::v_set(handle,
                        SWITCH_BFD_SESSION_ATTR_STATE,
                        switch_enum_t{SWITCH_BFD_SESSION_ATTR_STATE_UP});
    smi::event::bfd_event_notify(ev);
    return;
  }
}

switch_status_t bfd_init() {
  bf_status_t sts;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!feature::is_feature_set(SWITCH_FEATURE_BFD_OFFLOAD)) {
    return SWITCH_STATUS_SUCCESS;
  }

  REGISTER_OBJECT(bfd_rx_session, SWITCH_OBJECT_TYPE_BFD_RX_SESSION);
  REGISTER_OBJECT(bfd_tx_session, SWITCH_OBJECT_TYPE_BFD_TX_SESSION);
  REGISTER_OBJECT(bfd_rx_timer, SWITCH_OBJECT_TYPE_BFD_RX_TIMER);
  REGISTER_OBJECT(bfd_tx_timer, SWITCH_OBJECT_TYPE_BFD_TX_TIMER);
  REGISTER_OBJECT(bfd_pkt_action, SWITCH_OBJECT_TYPE_BFD_PKT_ACTION);
  REGISTER_OBJECT(bfd_offload_trap, SWITCH_OBJECT_TYPE_BFD_OFFLOAD_TRAP);
  REGISTER_OBJECT(bfd_session_helper, SWITCH_OBJECT_TYPE_BFD_SESSION_HELPER);

  bool sw_model = false;
  bf_pal_pltfm_type_get(0, &sw_model);
  if (sw_model) {
    /* reduce scale on model */
    timer_denominator_usec = 1000000;
    max_sessions = 1;
  }

  sts = bf_pal_num_pipes_get(0, &bfd_max_pipes);
  if (sts != BF_SUCCESS) {
    return pal_status_xlate(sts);
  }
  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_BFD_SESSION, &before_bfd_session_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_BFD_SESSION,
                                                 &after_bfd_session_update);
  status |= switch_store::reg_delete_trigs_before(
      SWITCH_OBJECT_TYPE_BFD_SESSION, &before_bfd_session_delete);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_BFD_SESSION,
                                                 &after_bfd_session_create);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_PORT,
                                                 &after_recirc_port_create);

  start_bf_switch_bfdd(SWITCH_HOSTIF_TRAP_ATTR_TYPE_BFD);
  switch_register_bfdd_session_state_cb(bfdd_session_state_cb);

  return status;
}

switch_status_t bfd_clean() {
  switch_register_bfdd_session_state_cb(0);
  return SWITCH_STATUS_SUCCESS;
}

} /* namespace smi */
