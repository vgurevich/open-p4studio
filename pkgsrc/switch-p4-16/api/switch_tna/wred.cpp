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
#include <tofino/pdfixed/pd_tm.h>
}

#include <math.h>

#include <utility>
#include <vector>
#include <memory>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

uint32_t switch_wred_bytes_to_cells(uint64_t bytes) {
  // p4_pd_status_t pd_status = 0;
  uint32_t cell_size = 0;
  p4_pd_tm_get_cell_size_in_bytes(0, &cell_size);
  return ceil(bytes / cell_size);
}

class wred_index : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_WRED_INDEX;
  static const switch_attr_id_t status_attr_id = SWITCH_WRED_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_WRED_INDEX_ATTR_PARENT_HANDLE;

 public:
  wred_index(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_WRED_INDEX,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t wred_profile_handle = {}, green_wred_handle = {},
                       yellow_wred_handle = {}, red_wred_handle = {},
                       port_handle = {};
    uint16_t dev_port = 0;
    switch_enum_t type = {};
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_ATTR_WRED_PROFILE_HANDLE, wred_profile_handle);
    if (wred_profile_handle.data != 0) {
      status |= switch_store::v_get(wred_profile_handle,
                                    SWITCH_WRED_PROFILE_ATTR_WRED_GREEN_HANDLE,
                                    green_wred_handle);
      status |= switch_store::v_get(wred_profile_handle,
                                    SWITCH_WRED_PROFILE_ATTR_WRED_YELLOW_HANDLE,
                                    yellow_wred_handle);
      status |= switch_store::v_get(wred_profile_handle,
                                    SWITCH_WRED_PROFILE_ATTR_WRED_RED_HANDLE,
                                    red_wred_handle);
    } else {
      return;
    }

    status |=
        switch_store::v_get(parent, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    if (port_handle.data == 0) return;
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    status |= switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_WRED_INDEX, false));

    auto it = match_action_list.begin();
    // green entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_WRED_INDEX),
                                           _ActionEntry(smi_id::T_WRED_INDEX)));
    status |= it->first.set_exact(smi_id::F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT,
                                  dev_port);
    status |= it->first.set_exact(smi_id::F_WRED_INDEX_QOS_MD_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= it->first.set_exact(
        smi_id::F_WRED_INDEX_QOS_MD_QID, parent, SWITCH_QUEUE_ATTR_QUEUE_ID);
    if (green_wred_handle.data != 0) {
      it->second.init_action_data(smi_id::A_WRED_INDEX_SET_WRED_INDEX);
      status |= it->second.set_arg(smi_id::D_SET_WRED_INDEX_WRED_INDEX,
                                   green_wred_handle);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }

    // yellow entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_WRED_INDEX),
                                           _ActionEntry(smi_id::T_WRED_INDEX)));
    status |= it->first.set_exact(smi_id::F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT,
                                  dev_port);
    status |=
        it->first.set_exact(smi_id::F_WRED_INDEX_QOS_MD_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= it->first.set_exact(
        smi_id::F_WRED_INDEX_QOS_MD_QID, parent, SWITCH_QUEUE_ATTR_QUEUE_ID);
    if (yellow_wred_handle.data != 0) {
      it->second.init_action_data(smi_id::A_WRED_INDEX_SET_WRED_INDEX);
      status |= it->second.set_arg(smi_id::D_SET_WRED_INDEX_WRED_INDEX,
                                   yellow_wred_handle);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }

    // red entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_WRED_INDEX),
                                           _ActionEntry(smi_id::T_WRED_INDEX)));
    status |= it->first.set_exact(smi_id::F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT,
                                  dev_port);
    status |= it->first.set_exact(smi_id::F_WRED_INDEX_QOS_MD_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= it->first.set_exact(
        smi_id::F_WRED_INDEX_QOS_MD_QID, parent, SWITCH_QUEUE_ATTR_QUEUE_ID);
    if (red_wred_handle.data != 0) {
      it->second.init_action_data(smi_id::A_WRED_INDEX_SET_WRED_INDEX);
      status |= it->second.set_arg(smi_id::D_SET_WRED_INDEX_WRED_INDEX,
                                   red_wred_handle);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }
  }
};

class v6_wred_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_V6_WRED_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_V6_WRED_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_V6_WRED_ACTION_ATTR_PARENT_HANDLE;

 public:
  v6_wred_action(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_V6_WRED_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint8_t not_ect = 0x0, ect_0 = 0x1, ect_1 = 0x2;
    bool enable = false, ecn_mark = false;

    status |= switch_store::v_get(parent, SWITCH_WRED_ATTR_ENABLE, enable);
    status |= switch_store::v_get(parent, SWITCH_WRED_ATTR_ECN_MARK, ecn_mark);

    auto it = match_action_list.begin();

    // v6 drop for ecn not-capable traffic
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V6_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V6_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V6_WRED_ACTION_INDEX, parent);
    status |=
        it->first.set_exact(smi_id::F_V6_WRED_ACTION_HDR_IPV6_ECN, not_ect);
    if (enable)
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);
    else
      it->second.init_action_data(smi_id::A_NO_ACTION);

    // v6 set ecn for ECN Codepoint=0x1
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V6_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V6_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V6_WRED_ACTION_INDEX, parent);
    status |= it->first.set_exact(smi_id::F_V6_WRED_ACTION_HDR_IPV6_ECN, ect_0);
    if (ecn_mark)
      it->second.init_action_data(smi_id::A_WRED_ACTION_SET_IPV6_ECN);
    else if (!enable)
      it->second.init_action_data(smi_id::A_NO_ACTION);
    else
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);

    // v6 set ecn for ECN Codepoint=0x2
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V6_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V6_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V6_WRED_ACTION_INDEX, parent);
    status |= it->first.set_exact(smi_id::F_V6_WRED_ACTION_HDR_IPV6_ECN, ect_1);
    if (ecn_mark)
      it->second.init_action_data(smi_id::A_WRED_ACTION_SET_IPV6_ECN);
    else if (!enable)
      it->second.init_action_data(smi_id::A_NO_ACTION);
    else
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);
  }
};

class v4_wred_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_V4_WRED_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_V4_WRED_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_V4_WRED_ACTION_ATTR_PARENT_HANDLE;

 public:
  v4_wred_action(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_V4_WRED_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint8_t not_ect = 0x0, ect_0 = 0x1, ect_1 = 0x2;
    bool enable = false, ecn_mark = false;

    status |= switch_store::v_get(parent, SWITCH_WRED_ATTR_ENABLE, enable);
    status |= switch_store::v_get(parent, SWITCH_WRED_ATTR_ECN_MARK, ecn_mark);

    auto it = match_action_list.begin();

    // v4 drop for ecn not-capable traffic
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V4_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V4_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V4_WRED_ACTION_INDEX, parent);
    status |=
        it->first.set_exact(smi_id::F_V4_WRED_ACTION_HDR_IPV4_ECN, not_ect);
    if (enable)
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);
    else
      it->second.init_action_data(smi_id::A_NO_ACTION);

    // v4 set ecn for ECN Codepoint=0x1
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V4_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V4_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V4_WRED_ACTION_INDEX, parent);
    status |= it->first.set_exact(smi_id::F_V4_WRED_ACTION_HDR_IPV4_ECN, ect_0);
    if (ecn_mark)
      it->second.init_action_data(smi_id::A_WRED_ACTION_SET_IPV4_ECN);
    else if (!enable)
      it->second.init_action_data(smi_id::A_NO_ACTION);
    else
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);

    // v4 set ecn for ECN Codepoint=0x2
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_V4_WRED_ACTION),
                                      _ActionEntry(smi_id::T_V4_WRED_ACTION)));
    status |= it->first.set_exact(smi_id::F_V4_WRED_ACTION_INDEX, parent);
    status |= it->first.set_exact(smi_id::F_V4_WRED_ACTION_HDR_IPV4_ECN, ect_1);
    if (ecn_mark)
      it->second.init_action_data(smi_id::A_WRED_ACTION_SET_IPV4_ECN);
    else if (!enable)
      it->second.init_action_data(smi_id::A_NO_ACTION);
    else
      it->second.init_action_data(smi_id::A_WRED_ACTION_DROP);
  }
};

switch_status_t add_delete_wred(const switch_object_id_t wred_handle,
                                bool add) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t time_constant = 0;
  uint32_t probability = 0;
  uint32_t min_thres = 0, max_thres = 0;

  status |= switch_store::v_get(
      wred_handle, SWITCH_WRED_ATTR_TIME_CONSTANT, time_constant);
  status |= switch_store::v_get(
      wred_handle, SWITCH_WRED_ATTR_PROBABILITY, probability);
  status |= switch_store::v_get(
      wred_handle, SWITCH_WRED_ATTR_MIN_THRESHOLD, min_thres);
  status |= switch_store::v_get(
      wred_handle, SWITCH_WRED_ATTR_MAX_THRESHOLD, max_thres);

  for (auto pipe : _Table(smi_id::T_WRED_SESSION).get_active_pipes()) {
    bf_rt_target_t local_dev_tgt = {.dev_id = 0, .pipe_id = pipe};
    _Table table(local_dev_tgt, get_bf_rt_info(), smi_id::T_WRED_SESSION);
    _MatchKey match_key(smi_id::T_WRED_SESSION);
    _ActionEntry action_entry(smi_id::T_WRED_SESSION);
    action_entry.init_indirect_data();
    status |= match_key.set_exact(smi_id::F_WRED_SESSION_INDEX, wred_handle);
    action_entry.init_indirect_data();

    if (add) {
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_TIME_CONSTANT,
                                     static_cast<float>(time_constant));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MIN_THRES,
                                     switch_wred_bytes_to_cells(min_thres));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MAX_THRES,
                                     switch_wred_bytes_to_cells(max_thres));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MAX_PROB,
                                     static_cast<float>(probability / 100.0));
    } else {
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_TIME_CONSTANT,
                                     static_cast<float>(0));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MIN_THRES,
                                     static_cast<uint64_t>(0));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MAX_THRES,
                                     static_cast<uint64_t>(0));
      status |= action_entry.set_arg(smi_id::D_WRED_SESSION_MAX_PROB,
                                     static_cast<float>(1));
    }
    status = table.entry_modify(match_key, action_entry);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_WRED,
                 "{}:{}: failed wred update status {} wred_handle {} "
                 "add {} pipe {}",
                 __func__,
                 __LINE__,
                 status,
                 wred_handle,
                 add,
                 pipe);
    }
  }

  return status;
}

class wred_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_WRED_SESSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_WRED_SESSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_WRED_SESSION_ATTR_PARENT_HANDLE;

 public:
  wred_session(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
  }
  switch_status_t create_update() {
    return add_delete_wred(get_parent(), true);
  }
  switch_status_t del() { return SWITCH_STATUS_SUCCESS; }
};

class egress_wred_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_WRED_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_WRED_STATS_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_WRED_STATS_ATTR_STATUS;

 public:
  egress_wred_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_WRED_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t wred_profile_handle = {}, port_handle = {};
    uint16_t dev_port = 0;
    switch_enum_t type = {};
    status |=
        switch_store::v_get(parent, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_ATTR_WRED_PROFILE_HANDLE, wred_profile_handle);
    if (wred_profile_handle.data == 0) return;

    if (port_handle.data == 0) return;
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    status |= switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_EGRESS_WRED_STATS, false));

    auto it = match_action_list.begin();
    std::vector<bool> flags = {false, true};
    std::vector<switch_pkt_color_t> colors = {
        SWITCH_PKT_COLOR_GREEN, SWITCH_PKT_COLOR_YELLOW, SWITCH_PKT_COLOR_RED};
    for (const auto &color : colors) {
      for (bool flag : flags) {
        it = match_action_list.insert(
            it,
            std::pair<_MatchKey, _ActionEntry>(
                _MatchKey(smi_id::T_EGRESS_WRED_STATS),
                _ActionEntry(smi_id::T_EGRESS_WRED_STATS)));
        status |=
            it->first.set_exact(smi_id::F_EGRESS_WRED_STATS_PORT, dev_port);
        status |= it->first.set_exact(smi_id::F_EGRESS_WRED_STATS_QID,
                                      parent,
                                      SWITCH_QUEUE_ATTR_QUEUE_ID);
        status |= it->first.set_exact(smi_id::F_EGRESS_WRED_STATS_COLOR,
                                      static_cast<uint8_t>(color));
        status |=
            it->first.set_exact(smi_id::F_EGRESS_WRED_STATS_WRED_DROP, flag);

        it->second.init_action_data(smi_id::A_EGRESS_WRED_STATS_COUNT);
      }
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    const std::vector<std::pair<uint16_t, std::vector<uint16_t>>>
        aggregate_counters = {
            {SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_PACKETS,
             {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS,
              SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS,
              SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_PACKETS}},
            {SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_BYTES,
             {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_BYTES,
              SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES,
              SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_BYTES}},
            {SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_PACKETS,
             {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS,
              SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS,
              SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS}},
            {SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_BYTES,
             {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_BYTES,
              SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_BYTES,
              SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES}}};
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t color = 0, flag = 0;
      entry.second.get_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_WRED_STATS_COUNT,
                           &bytes);
      entry.second.get_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_WRED_STATS_COUNT,
                           &pkts);
      entry.first.get_exact(smi_id::F_EGRESS_WRED_STATS_COLOR, &color);
      entry.first.get_exact(smi_id::F_EGRESS_WRED_STATS_WRED_DROP, &flag);
      switch_counter_t cntr_pkts;
      switch_counter_t cntr_bytes;
      if (flag) {
        switch (color) {
          case SWITCH_PKT_COLOR_GREEN:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS] =
                cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_BYTES] =
                cntr_bytes;
            break;
          case SWITCH_PKT_COLOR_YELLOW:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS] =
                cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES] =
                cntr_bytes;
            break;
          case SWITCH_PKT_COLOR_RED:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_PACKETS] = cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_BYTES] = cntr_bytes;
            break;
        }
      } else {
        switch (color) {
          case SWITCH_PKT_COLOR_GREEN:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS] =
                cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_BYTES] =
                cntr_bytes;
            break;
          case SWITCH_PKT_COLOR_YELLOW:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS] =
                cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_BYTES] =
                cntr_bytes;
            break;
          case SWITCH_PKT_COLOR_RED:
            cntr_pkts.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS;
            cntr_pkts.count = pkts;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS] =
                cntr_pkts;
            cntr_bytes.counter_id =
                SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES;
            cntr_bytes.count = bytes;
            cntrs[SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES] =
                cntr_bytes;
            break;
        }
      }
    }
    for (auto &aggregate_counter : aggregate_counters) {
      cntrs[aggregate_counter.first].counter_id = aggregate_counter.first;
      for (auto counter : aggregate_counter.second) {
        cntrs[aggregate_counter.first].count += cntrs[counter].count;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES,
                           value);
      entry.second.set_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t color = 0, flag = 0;
      entry.first.get_exact(smi_id::F_EGRESS_WRED_STATS_COLOR, &color);
      entry.first.get_exact(smi_id::F_EGRESS_WRED_STATS_WRED_DROP, &flag);

      for (auto id : cntr_ids) {
        switch (id) {
          case SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_PACKETS:
            if (flag && ((color == SWITCH_PKT_COLOR_GREEN) ||
                         (color == SWITCH_PKT_COLOR_YELLOW) ||
                         (color == SWITCH_PKT_COLOR_RED))) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_BYTES:
            if (flag && ((color == SWITCH_PKT_COLOR_GREEN) ||
                         (color == SWITCH_PKT_COLOR_YELLOW) ||
                         (color == SWITCH_PKT_COLOR_RED))) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS:
            if (flag && (color == SWITCH_PKT_COLOR_GREEN)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_BYTES:
            if (flag && (color == SWITCH_PKT_COLOR_GREEN)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS:
            if (flag && (color == SWITCH_PKT_COLOR_YELLOW)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES:
            if (flag && (color == SWITCH_PKT_COLOR_YELLOW)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_PACKETS:
            if (flag && (color == SWITCH_PKT_COLOR_RED)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_BYTES:
            if (flag && (color == SWITCH_PKT_COLOR_RED)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_PACKETS:
            if (!flag && ((color == SWITCH_PKT_COLOR_GREEN) ||
                          (color == SWITCH_PKT_COLOR_YELLOW) ||
                          (color == SWITCH_PKT_COLOR_RED))) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS:
            if (!flag && (color == SWITCH_PKT_COLOR_GREEN)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_BYTES:
            if (!flag && ((color == SWITCH_PKT_COLOR_GREEN) ||
                          (color == SWITCH_PKT_COLOR_YELLOW) ||
                          (color == SWITCH_PKT_COLOR_RED))) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_BYTES:
            if (!flag && (color == SWITCH_PKT_COLOR_GREEN)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS:
            if (!flag && (color == SWITCH_PKT_COLOR_YELLOW)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_BYTES:
            if (!flag && (color == SWITCH_PKT_COLOR_YELLOW)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS:
            if (!flag && (color == SWITCH_PKT_COLOR_RED)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES:
            if (!flag && (color == SWITCH_PKT_COLOR_RED)) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          default:
            break;
        }
      }
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_WRED_STATS_COUNT,
                           &ctr_bytes);
      entry.second.get_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_WRED_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_WRED_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(get_auto_oid(),
                                  SWITCH_EGRESS_WRED_STATS_ATTR_MAU_STATS_CACHE,
                                  ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_WRED,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_wred_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES,
                           ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_WRED,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_wred_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class port_ecn_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PORT_ECN_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_ECN_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_ECN_STATS_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> queue_handles;

 public:
  port_ecn_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = switch_store::v_get(
        parent, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_handles);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t queue_cntr_id = SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS;
    // get the ecn stats for all queues of this port
    for (const auto &queue_handle : queue_handles) {
      std::vector<switch_counter_t> queue_cntrs(SWITCH_QUEUE_COUNTER_ID_MAX);
      egress_wred_stats stats(queue_handle, status);
      stats.counters_get(queue_handle, queue_cntrs);
      queue_cntr_id = SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS;
      for (uint64_t cntr_id = SWITCH_PORT_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS;
           cntr_id <= SWITCH_PORT_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES;
           cntr_id++) {
        cntrs[cntr_id].counter_id = cntr_id;
        cntrs[cntr_id].count += queue_cntrs[queue_cntr_id].count;
        queue_cntr_id++;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    (void)cntr_ids;
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    return SWITCH_STATUS_SUCCESS;
  }
};

// wred stats for the queue will be 0'd after an update
switch_status_t before_queue_update(const switch_object_id_t handle,
                                    const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject, mobject_stats;

  if (attr.id_get() == SWITCH_QUEUE_ATTR_WRED_PROFILE_HANDLE) {
    mobject_stats = std::unique_ptr<egress_wred_stats>(
        new egress_wred_stats(handle, status));
    if (mobject_stats != NULL) {
      status = mobject_stats->del();
    }
    if (status == SWITCH_STATUS_SUCCESS) {
      mobject = std::unique_ptr<wred_index>(new wred_index(handle, status));
      if (mobject != NULL) {
        status = mobject->del();
      }
    }
  }

  return status;
}

switch_status_t after_queue_update(const switch_object_id_t handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject, mobject_stats;

  if (attr.id_get() == SWITCH_QUEUE_ATTR_WRED_PROFILE_HANDLE) {
    mobject = std::unique_ptr<wred_index>(new wred_index(handle, status));
    if (mobject != NULL) {
      status = mobject->create_update();
    }
    if (status == SWITCH_STATUS_SUCCESS) {
      mobject_stats = std::unique_ptr<egress_wred_stats>(
          new egress_wred_stats(handle, status));
      if (mobject_stats != NULL) {
        status = mobject_stats->create_update();
      }
    }
  }

  return status;
}

switch_status_t wred_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_QUEUE,
                                                  &before_queue_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_QUEUE,
                                                 &after_queue_update);

  REGISTER_OBJECT(v4_wred_action, SWITCH_OBJECT_TYPE_V4_WRED_ACTION);
  REGISTER_OBJECT(v6_wred_action, SWITCH_OBJECT_TYPE_V6_WRED_ACTION);
  REGISTER_OBJECT(wred_session, SWITCH_OBJECT_TYPE_WRED_SESSION);
  REGISTER_OBJECT(wred_index, SWITCH_OBJECT_TYPE_WRED_INDEX);
  REGISTER_OBJECT(egress_wred_stats, SWITCH_OBJECT_TYPE_EGRESS_WRED_STATS);
  REGISTER_OBJECT(port_ecn_stats, SWITCH_OBJECT_TYPE_PORT_ECN_STATS);

  return status;
}

switch_status_t wred_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
