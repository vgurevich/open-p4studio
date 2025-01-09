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


#include <math.h>

#include <tuple>
#include <utility>
#include <vector>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

uint64_t switch_meter_bytes_to_kbps(uint64_t bytes) {
  return ceil((static_cast<double>(bytes * 8)) / 1000);
}

switch_packet_action_t switch_meter_color_action_to_packet_action(
    switch_enum_t color_action) {
  switch (color_action.enumdata) {
    case SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP:
      return SWITCH_PACKET_ACTION_DROP;
    case SWITCH_METER_ATTR_GREEN_PACKET_ACTION_PERMIT:
      return SWITCH_PACKET_ACTION_PERMIT;
    case SWITCH_METER_ATTR_GREEN_PACKET_ACTION_TRAP:
      return SWITCH_PACKET_ACTION_TRAP;
    case SWITCH_METER_ATTR_GREEN_PACKET_ACTION_COPY:
      return SWITCH_PACKET_ACTION_COPY;
    default:
      return SWITCH_PACKET_ACTION_PERMIT;
  }
  return SWITCH_PACKET_ACTION_PERMIT;
}

class storm_control : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_STORM_CONTROL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_STORM_CONTROL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_STORM_CONTROL_ATTR_PARENT_HANDLE;

 public:
  storm_control(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_STORM_CONTROL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t meter_handle = parent;
    switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL)
      return;
    const auto &port_attr_ref_map =
        switch_store::get_object_references(parent, SWITCH_OBJECT_TYPE_PORT);
    // One and only one port can refer to a single storm control meter. So we
    // break out of the loop after first
    // reference
    switch_object_id_t port_hdl{};
    switch_attr_id_t port_attr{};
    bool found = false;
    for (const auto &port_attr_ref : port_attr_ref_map) {
      switch_enum_t port_type{};
      switch_store::v_get(port_attr_ref.oid, SWITCH_PORT_ATTR_TYPE, port_type);
      if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;
      found = true;
      port_hdl = port_attr_ref.oid;
      // For a given port a single Storm Control Meter can only be referred once
      // by one of the following port attrs
      // MC/BC/UC storm control
      port_attr = port_attr_ref.attr_id;
      break;
    }
    if (!found) {
      status |= SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    uint8_t mask = 0x3;
    uint16_t dev_port = 0;
    status |=
        switch_store::v_get(port_hdl, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_STORM_CONTROL, true));

    switch_packet_type_t pkt_type{SWITCH_PACKET_TYPE_UNICAST};

    switch (port_attr) {
      case SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_BROADCAST;
        break;
      case SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_MULTICAST;
        break;
      case SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_UNICAST;
        break;
      default:
        status |= SWITCH_STATUS_INVALID_PARAMETER;
        return;
    }

    status |= match_key.set_exact(smi_id::F_STORM_CONTROL_LOCAL_MD_INGRESS_PORT,
                                  dev_port);
    status |= match_key.set_ternary(
        smi_id::F_STORM_CONTROL_PKT_TYPE, static_cast<uint8_t>(pkt_type), mask);
    status |= match_key.set_ternary(
        smi_id::F_STORM_CONTROL_DMAC_MISS,
        static_cast<uint8_t>((pkt_type == SWITCH_PACKET_TYPE_UNICAST) ? 1 : 0),
        static_cast<uint8_t>((pkt_type == SWITCH_PACKET_TYPE_UNICAST) ? 1 : 0));
    status |= match_key.set_ternary(
        smi_id::F_STORM_CONTROL_MULTICAST_HIT,
        static_cast<uint8_t>(0),
        static_cast<uint8_t>((pkt_type == SWITCH_PACKET_TYPE_MULTICAST) ? 1
                                                                        : 0));
    action_entry.init_action_data(smi_id::A_STORM_CONTROL_SET_METER);
    status |= action_entry.set_arg(smi_id::P_STORM_CONTROL_SET_METER_INDEX,
                                   meter_handle);
  }
};

class storm_control_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_STORM_CONTROL_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_STORM_CONTROL_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_STORM_CONTROL_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;

  switch_status_t program_storm_control_meter(switch_object_id_t meter,
                                              uint64_t cir_bps,
                                              uint64_t pir_bps,
                                              uint64_t cburst_bytes,
                                              uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (bf_dev_pipe_t tpipe :
         _Table(smi_id::T_STORM_CONTROL_METER).get_active_pipes()) {
      bf_rt_target_t dev_tgt = {.dev_id = 0, .pipe_id = tpipe};
      _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_STORM_CONTROL_METER);
      _MatchKey meter_key(smi_id::T_STORM_CONTROL_METER);
      _ActionEntry meter_action(smi_id::T_STORM_CONTROL_METER);
      status |=
          meter_key.set_exact(smi_id::F_STORM_CONTROL_METER_METER_INDEX, meter);
      status |= meter_action.init_indirect_data();
      status |= meter_action.set_arg(
          smi_id::D_STORM_CONTROL_METER_METER_SPEC_CIR_KBPS,
          switch_meter_bytes_to_kbps(cir_bps));
      status |= meter_action.set_arg(
          smi_id::D_STORM_CONTROL_METER_METER_SPEC_PIR_KBPS,
          switch_meter_bytes_to_kbps(pir_bps));
      status |= meter_action.set_arg(
          smi_id::D_STORM_CONTROL_METER_METER_SPEC_CBS_KBITS,
          switch_meter_bytes_to_kbps(cburst_bytes));
      status |= meter_action.set_arg(
          smi_id::D_STORM_CONTROL_METER_METER_SPEC_PBS_KBITS,
          switch_meter_bytes_to_kbps(pburst_bytes));
      status |= table.entry_modify(meter_key, meter_action);
    }
    return status;
  }

 public:
  storm_control_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
    switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_BYTES) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: storm control meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status = program_storm_control_meter(
        get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    status = auto_object::create_update();
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status = program_storm_control_meter(get_parent(), 0, 0, 0, 0);
    status = auto_object::del();
    return status;
  }
};

class storm_control_port_meter_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_SC_PORT_METER_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SC_PORT_METER_STATS_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_SC_PORT_METER_STATS_ATTR_STATUS;
  std::vector<std::tuple<switch_packet_type_t, switch_object_id_t>>
      pkt_meter_list;
  switch_enum_t target_type{};

 public:
  storm_control_port_meter_stats(const switch_object_id_t parent,
                                 switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t meter_hdl{};
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL, meter_hdl);
    if (meter_hdl.data != 0) {
      pkt_meter_list.push_back(
          std::make_tuple(SWITCH_PACKET_TYPE_BROADCAST, meter_hdl));
      meter_hdl.data = 0;
    }
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL, meter_hdl);
    if (meter_hdl.data != 0) {
      pkt_meter_list.push_back(
          std::make_tuple(SWITCH_PACKET_TYPE_MULTICAST, meter_hdl));
      meter_hdl.data = 0;
    }
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL, meter_hdl);
    if (meter_hdl.data != 0) {
      pkt_meter_list.push_back(
          std::make_tuple(SWITCH_PACKET_TYPE_UNICAST, meter_hdl));
      meter_hdl.data = 0;
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    auto_object::create_update();
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    auto_object::del();
    return status;
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (auto pkt_meter : pkt_meter_list) {
      auto pkt_type = std::get<0>(pkt_meter);
      std::vector<switch_counter_t> meter_cntrs;
      status |= (factory::get_instance())
                    .get_counters_handler(std::get<1>(pkt_meter), meter_cntrs);
      for (auto cntr : meter_cntrs) {
        switch (pkt_type) {
          case SWITCH_PACKET_TYPE_UNICAST: {
            switch (cntr.counter_id) {
              case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_UCAST_GREEN_PACKETS]
                    .counter_id = SWITCH_PORT_COUNTER_ID_SC_UCAST_GREEN_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_UCAST_GREEN_PACKETS].count +=
                    cntr.count;
                break;
              case SWITCH_METER_COUNTER_ID_RED_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_UCAST_RED_PACKETS].counter_id =
                    SWITCH_PORT_COUNTER_ID_SC_UCAST_RED_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_UCAST_RED_PACKETS].count +=
                    cntr.count;
                break;
              default:
                break;
            }
          } break;
          case SWITCH_PACKET_TYPE_MULTICAST: {
            switch (cntr.counter_id) {
              case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_MCAST_GREEN_PACKETS]
                    .counter_id = SWITCH_PORT_COUNTER_ID_SC_MCAST_GREEN_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_MCAST_GREEN_PACKETS].count +=
                    cntr.count;
                break;
              case SWITCH_METER_COUNTER_ID_RED_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_MCAST_RED_PACKETS].counter_id =
                    SWITCH_PORT_COUNTER_ID_SC_MCAST_RED_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_MCAST_RED_PACKETS].count +=
                    cntr.count;
                break;
              default:
                break;
            }
          } break;
          case SWITCH_PACKET_TYPE_BROADCAST: {
            switch (cntr.counter_id) {
              case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_BCAST_GREEN_PACKETS]
                    .counter_id = SWITCH_PORT_COUNTER_ID_SC_BCAST_GREEN_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_BCAST_GREEN_PACKETS].count +=
                    cntr.count;
                break;
              case SWITCH_METER_COUNTER_ID_RED_PACKETS:
                cntrs[SWITCH_PORT_COUNTER_ID_SC_BCAST_RED_PACKETS].counter_id =
                    SWITCH_PORT_COUNTER_ID_SC_BCAST_RED_PACKETS;
                cntrs[SWITCH_PORT_COUNTER_ID_SC_BCAST_RED_PACKETS].count +=
                    cntr.count;
                break;
              default:
                break;
            }
          } break;
          default:
            break;
        }
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (auto pkt_meter : pkt_meter_list) {
      status |= (factory::get_instance())
                    .set_all_counters_handler(std::get<1>(pkt_meter));
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (auto pkt_meter : pkt_meter_list) {
      std::vector<uint16_t> meter_cntr_ids;
      auto pkt_type = std::get<0>(pkt_meter);
      auto meter_hdl = std::get<1>(pkt_meter);
      for (auto id : cntr_ids) {
        switch (id) {
          case SWITCH_PORT_COUNTER_ID_SC_UCAST_GREEN_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_UNICAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_PACKETS);
            break;
          case SWITCH_PORT_COUNTER_ID_SC_UCAST_RED_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_UNICAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_PACKETS);
            break;
          case SWITCH_PORT_COUNTER_ID_SC_MCAST_GREEN_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_MULTICAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_PACKETS);
            break;
          case SWITCH_PORT_COUNTER_ID_SC_MCAST_RED_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_MULTICAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_PACKETS);
            break;
          case SWITCH_PORT_COUNTER_ID_SC_BCAST_GREEN_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_BROADCAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_PACKETS);
            break;
          case SWITCH_PORT_COUNTER_ID_SC_BCAST_RED_PACKETS:
            if (pkt_type == SWITCH_PACKET_TYPE_BROADCAST)
              meter_cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_PACKETS);
            break;
          default:
            break;
        }
      }
      status |= (factory::get_instance())
                    .set_counters_handler(meter_hdl, meter_cntr_ids);
    }
    return status;
  }
};

class storm_control_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SC_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SC_STATS_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_SC_STATS_ATTR_STATUS;
  switch_enum_t target_type = {0};

 public:
  storm_control_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_STORM_CONTROL_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL)
      return;
    const auto &port_attr_ref_map =
        switch_store::get_object_references(parent, SWITCH_OBJECT_TYPE_PORT);
    // One and only one port can refer to a single storm control meter. So we
    // break out of the loop after first
    // reference
    switch_object_id_t port_hdl{};
    switch_attr_id_t port_attr{};
    bool found = false;
    for (const auto &port_attr_ref : port_attr_ref_map) {
      switch_enum_t port_type{};
      switch_store::v_get(port_attr_ref.oid, SWITCH_PORT_ATTR_TYPE, port_type);
      if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;
      found = true;
      port_hdl = port_attr_ref.oid;
      // For a given port a single Storm Control Meter can only be referred once
      // by one of the following port attrs
      // MC/BC/UC storm control
      port_attr = port_attr_ref.attr_id;
      break;
    }
    if (!found) {
      status |= SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    uint16_t dev_port = 0;
    status |=
        switch_store::v_get(port_hdl, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_STORM_CONTROL_STATS, true));
    switch_packet_type_t pkt_type{SWITCH_PACKET_TYPE_UNICAST};

    switch (port_attr) {
      case SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_BROADCAST;
        break;
      case SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_MULTICAST;
        break;
      case SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL:
        pkt_type = SWITCH_PACKET_TYPE_UNICAST;
        break;
      default:
        status |= SWITCH_STATUS_INVALID_PARAMETER;
        return;
    }

    auto it = match_action_list.begin();
    std::vector<bool> flags = {false, true};
    std::vector<bf_rt_id_t> storm_control_ctrs{
        smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS,
        smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES};
    for (auto color : {SWITCH_PKT_COLOR_GREEN,
                       SWITCH_PKT_COLOR_YELLOW,
                       SWITCH_PKT_COLOR_RED}) {
      uint8_t dmac_miss = (pkt_type == SWITCH_PACKET_TYPE_UNICAST) ? 1 : 0;
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_STORM_CONTROL_STATS),
              _ActionEntry(smi_id::T_STORM_CONTROL_STATS)));
      status |= it->first.set_exact(
          smi_id::F_STORM_CONTROL_STATS_LOCAL_MD_INGRESS_PORT, dev_port);
      status |= it->first.set_exact(smi_id::F_STORM_CONTROL_STATS_COLOR,
                                    static_cast<uint8_t>(color));
      status |= it->first.set_ternary(smi_id::F_STORM_CONTROL_STATS_PKT_TYPE,
                                      static_cast<uint8_t>(pkt_type),
                                      static_cast<uint8_t>(0x3));
      status |= it->first.set_ternary(
          smi_id::F_STORM_CONTROL_STATS_DMAC_MISS, dmac_miss, dmac_miss);
      status |= it->first.set_ternary(
          smi_id::F_STORM_CONTROL_STATS_MULTICAST_HIT,
          static_cast<uint8_t>(0),
          static_cast<uint8_t>((pkt_type == SWITCH_PACKET_TYPE_MULTICAST) ? 1
                                                                          : 0));

      it->second.init_action_data(smi_id::A_STORM_CONTROL_STATS_COUNT,
                                  storm_control_ctrs);
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_object_match_action_list::data_get();

    for (auto const &entry : match_action_list) {
      uint64_t pkts = 0;
      uint64_t bytes = 0;
      uint64_t color = 0;

      entry.second.get_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_STORM_CONTROL_STATS_COUNT,
                           &pkts);
      entry.second.get_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_STORM_CONTROL_STATS_COUNT,
                           &bytes);
      entry.first.get_exact(smi_id::F_STORM_CONTROL_STATS_COLOR, &color);

      switch (color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count += bytes;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count += bytes;
          break;
        // Driver doesn't allow CBS and PBS values to be programmed as zero. As
        // a result we cannot truly implement Single Rate Two Color Meters. To
        // work around this issue, SC Meters as programmed with PIR and PBS
        // values, while programming the Yellow SC action as drop. This mimics
        // the Single Rate Two color behavior. Hence we club Yellow color
        // packets here with Red packets.
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count += bytes;
          break;
        default:
          break;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t color = 0;
      entry.first.get_exact(smi_id::F_STORM_CONTROL_STATS_COLOR, &color);

      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            // YELLOW is also counted as RED
            if (color != SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            // YELLOW is also counted as RED
            if (color != SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES, value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_STORM_CONTROL_STATS_COUNT,
                           &ctr_bytes);
      entry.second.get_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_STORM_CONTROL_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_SC_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_SC_STATS_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "storm control stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES,
                           ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "storm control stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_copp_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_COPP_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_COPP_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_COPP_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_copp_meter(switch_object_id_t meter,
                                     uint64_t cir_bps,
                                     uint64_t pir_bps,
                                     uint64_t cburst_bytes,
                                     uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_COPP_METER);
    _MatchKey meter_key(smi_id::T_COPP_METER);
    _ActionEntry meter_action(smi_id::T_COPP_METER);

    status |= meter_key.set_exact(smi_id::F_COPP_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |=
        meter_action.set_arg(smi_id::D_COPP_METER_METER_SPEC_CIR_PPS, cir_bps);
    status |=
        meter_action.set_arg(smi_id::D_COPP_METER_METER_SPEC_PIR_PPS, pir_bps);
    status |= meter_action.set_arg(smi_id::D_COPP_METER_METER_SPEC_CBS_PKTS,
                                   cburst_bytes);
    status |= meter_action.set_arg(smi_id::D_COPP_METER_METER_SPEC_PBS_PKTS,
                                   pburst_bytes);
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  ingress_copp_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      status |= program_copp_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      status |= program_copp_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class ingress_copp_table : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_COPP_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_COPP_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_COPP_TABLE_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};
  switch_enum_t packet_action = {0};

 public:
  ingress_copp_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_COPP, status_attr_id, auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return;
    }
    std::vector<bf_rt_id_t> copp_ctrs{smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS};
    uint64_t pkts = 0;

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_COPP),
                                           _ActionEntry(smi_id::T_COPP)));
    uint8_t meter_index_mask = 0xFF;
    uint8_t color_mask = 0x03;
    status |= it->first.set_ternary(smi_id::F_COPP_COPP_METER_ID,
                                    static_cast<uint8_t>(parent.data),
                                    meter_index_mask);

    status |=
        it->first.set_ternary(smi_id::F_COPP_PACKET_COLOR,
                              static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN),
                              color_mask);
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS, pkts);

    /* set match/action for color - yello */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_COPP),
                                           _ActionEntry(smi_id::T_COPP)));

    status |= it->first.set_ternary(smi_id::F_COPP_COPP_METER_ID,
                                    static_cast<uint8_t>(parent.data),
                                    meter_index_mask);
    status |=
        it->first.set_ternary(smi_id::F_COPP_PACKET_COLOR,
                              static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW),
                              color_mask);
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS, pkts);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_COPP),
                                           _ActionEntry(smi_id::T_COPP)));

    status |= it->first.set_ternary(smi_id::F_COPP_COPP_METER_ID,
                                    static_cast<uint8_t>(parent.data),
                                    meter_index_mask);
    status |= it->first.set_ternary(smi_id::F_COPP_PACKET_COLOR,
                                    static_cast<uint8_t>(SWITCH_PKT_COLOR_RED),
                                    color_mask);
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS, pkts);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint8_t pkt_color = 0;
      uint8_t color_mask = 0x03;
      entry.second.get_arg(
          smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS, smi_id::A_COPP_DROP, &pkts);
      entry.first.get_ternary(
          smi_id::F_COPP_PACKET_COLOR, &pkt_color, &color_mask);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint8_t pkt_color = 0;
      uint8_t color_mask = 0x03;
      entry.first.get_ternary(
          smi_id::F_COPP_PACKET_COLOR, &pkt_color, &color_mask);

      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS,
                                   value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS,
                                   value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS,
                                   value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_COPP_DROP,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_COPP_TABLE_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_INGRESS_COPP_TABLE_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_copp_table cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_copp_table status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_copp_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_COPP_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_COPP_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_COPP_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_copp_meter(switch_object_id_t meter,
                                     uint64_t cir_bps,
                                     uint64_t pir_bps,
                                     uint64_t cburst_bytes,
                                     uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_EGRESS_COPP_METER);
    _MatchKey meter_key(smi_id::T_EGRESS_COPP_METER);
    _ActionEntry meter_action(smi_id::T_EGRESS_COPP_METER);

    status |=
        meter_key.set_exact(smi_id::F_EGRESS_COPP_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_COPP_METER_METER_SPEC_CIR_PPS, cir_bps);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_COPP_METER_METER_SPEC_PIR_PPS, pir_bps);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_COPP_METER_METER_SPEC_CBS_PKTS, cburst_bytes);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_COPP_METER_METER_SPEC_PBS_PKTS, pburst_bytes);
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  egress_copp_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      status |= program_copp_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      status |= program_copp_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class egress_copp_table : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_COPP_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_COPP_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_COPP_TABLE_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  egress_copp_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_COPP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return;
    }
    std::vector<bf_rt_id_t> copp_ctrs{
        smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS};
    uint64_t pkts = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_EGRESS_COPP),
                                      _ActionEntry(smi_id::T_EGRESS_COPP)));
    status |= it->first.set_exact(smi_id::F_EGRESS_COPP_COPP_METER_ID, parent);

    status |= it->first.set_exact(smi_id::F_EGRESS_COPP_PACKET_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, pkts);

    /* set match/action for color - yellow */
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_EGRESS_COPP),
                                      _ActionEntry(smi_id::T_EGRESS_COPP)));

    status |= it->first.set_exact(smi_id::F_EGRESS_COPP_COPP_METER_ID, parent);
    status |=
        it->first.set_exact(smi_id::F_EGRESS_COPP_PACKET_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, pkts);

    /* set match/action for color - red */
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_EGRESS_COPP),
                                      _ActionEntry(smi_id::T_EGRESS_COPP)));

    status |= it->first.set_exact(smi_id::F_EGRESS_COPP_COPP_METER_ID, parent);
    status |= it->first.set_exact(smi_id::F_EGRESS_COPP_PACKET_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_DROP, copp_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_COPP_PERMIT, copp_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, pkts);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_COPP_DROP,
                           &pkts);
      entry.first.get_exact(smi_id::F_EGRESS_COPP_PACKET_COLOR, &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count += pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_COPP_PACKET_COLOR, &pkt_color);

      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS, value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_COPP_DROP,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_COPP_TABLE_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(get_auto_oid(),
                                  SWITCH_EGRESS_COPP_TABLE_ATTR_MAU_STATS_CACHE,
                                  ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_copp_table cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_copp_table status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_mirror_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_MIRROR_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_MIRROR_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
  switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};

 public:
  switch_status_t program_ingress_mirror_meter(switch_object_id_t meter,
                                               uint64_t cir_bps,
                                               uint64_t pir_bps,
                                               uint64_t cburst_bytes,
                                               uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_MIRROR_METER);
    _MatchKey meter_key(smi_id::T_INGRESS_MIRROR_METER);
    _ActionEntry meter_action(smi_id::T_INGRESS_MIRROR_METER);

    status |=
        meter_key.set_exact(smi_id::F_INGRESS_MIRROR_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |= meter_action.set_arg(
        smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_CIR_PPS, cir_bps);
    status |= meter_action.set_arg(
        smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_PIR_PPS, pir_bps);
    status |= meter_action.set_arg(
        smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS, cburst_bytes);
    status |= meter_action.set_arg(
        smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS, pburst_bytes);
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  ingress_mirror_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      status |= program_ingress_mirror_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      status |= program_ingress_mirror_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class ingress_mirror_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_MIRROR_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_MIRROR_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  ingress_mirror_meter_index(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_MIRROR_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
    switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_INGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX,
        parent);
    action_entry.init_action_data(smi_id::A_SET_INGRESS_MIRROR_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_INGRESS_MIRROR_METER_INDEX, parent);
  }
};

class ingress_mirror_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_MIRROR_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_MIRROR_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
  switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};

 public:
  ingress_mirror_meter_action(const switch_object_id_t parent,
                              switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_MIRROR_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    std::vector<bf_rt_id_t> ingress_mirror_meter_ctrs{
        smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          ingress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &pkts);
      entry.second.get_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS, value);
      entry.second.set_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &ctr_bytes);
      entry.second.get_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(
        SWITCH_INGRESS_MIRROR_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_INGRESS_MIRROR_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_mirror_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(
          smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_mirror_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_mirror_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_MIRROR_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_MIRROR_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
  switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};

 public:
  switch_status_t program_egress_mirror_meter(switch_object_id_t meter,
                                              uint64_t cir_bps,
                                              uint64_t pir_bps,
                                              uint64_t cburst_bytes,
                                              uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_EGRESS_MIRROR_METER);
    _MatchKey meter_key(smi_id::T_EGRESS_MIRROR_METER);
    _ActionEntry meter_action(smi_id::T_EGRESS_MIRROR_METER);

    status |=
        meter_key.set_exact(smi_id::F_EGRESS_MIRROR_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_CIR_PPS, cir_bps);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_PIR_PPS, pir_bps);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS, cburst_bytes);
    status |= meter_action.set_arg(
        smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS, pburst_bytes);
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  egress_mirror_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      status |= program_egress_mirror_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      status |= program_egress_mirror_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class egress_mirror_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_MIRROR_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_MIRROR_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  egress_mirror_meter_index(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_MIRROR_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
    switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_EGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX,
        parent);
    action_entry.init_action_data(smi_id::A_SET_EGRESS_MIRROR_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_EGRESS_MIRROR_METER_INDEX, parent);
  }
};

class egress_mirror_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_MIRROR_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_MIRROR_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {SWITCH_METER_ATTR_TARGET_TYPE_NONE};
  switch_enum_t meter_type = {SWITCH_METER_ATTR_TYPE_NONE};

 public:
  egress_mirror_meter_action(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_MIRROR_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_TYPE, meter_type);
    if (meter_type.enumdata != SWITCH_METER_ATTR_TYPE_PACKETS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}:{}: meter type {} is not supported",
                 __func__,
                 __LINE__,
                 meter_type.enumdata);
      status |= SWITCH_STATUS_NOT_SUPPORTED;
      return;
    }

    std::vector<bf_rt_id_t> egress_mirror_meter_ctrs{
        smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_MIRROR_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_MIRROR_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          egress_mirror_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &pkts);
      entry.second.get_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS, value);
      entry.second.set_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &ctr_bytes);
      entry.second.get_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
          &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(
        SWITCH_EGRESS_MIRROR_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_EGRESS_MIRROR_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_mirror_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(
          smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_mirror_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_acl_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_ACL_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_ACL_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_ACL_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_ingress_acl_meter(switch_object_id_t meter,
                                            uint64_t cir_bps,
                                            uint64_t pir_bps,
                                            uint64_t cburst_bytes,
                                            uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_ACL_METER);
    _MatchKey meter_key(smi_id::T_INGRESS_ACL_METER);
    _ActionEntry meter_action(smi_id::T_INGRESS_ACL_METER);

    status |=
        meter_key.set_exact(smi_id::F_INGRESS_ACL_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_ACL_METER_METER_SPEC_CIR_KBPS,
                             switch_meter_bytes_to_kbps(cir_bps));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_ACL_METER_METER_SPEC_PIR_KBPS,
                             switch_meter_bytes_to_kbps(pir_bps));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_ACL_METER_METER_SPEC_CBS_KBITS,
                             switch_meter_bytes_to_kbps(cburst_bytes));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_ACL_METER_METER_SPEC_PBS_KBITS,
                             switch_meter_bytes_to_kbps(pburst_bytes));
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  ingress_acl_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      status |= program_ingress_acl_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      status |= program_ingress_acl_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class ingress_acl_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_ACL_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_ACL_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_ACL_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  ingress_acl_meter_index(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_ACL_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {0};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_INGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX, parent);
    action_entry.init_action_data(smi_id::A_SET_INGRESS_ACL_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_INGRESS_ACL_METER_INDEX, parent);
  }
};

class ingress_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  ingress_acl_meter_action(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_ACL_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> ingress_acl_meter_ctrs{
        smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_ACL_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_INGRESS_ACL_METER_COUNT,
                                ingress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_INGRESS_ACL_METER_COUNT,
                                ingress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_ACL_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_INGRESS_ACL_METER_COUNT,
                                ingress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_INGRESS_ACL_METER_COUNT,
                           &pkts);
      entry.second.get_arg(
          smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_ACL_METER_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_INGRESS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           value);
      entry.second.set_arg(
          smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_ACL_METER_COUNT,
          &ctr_bytes);
      entry.second.get_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_INGRESS_ACL_METER_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_INGRESS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_acl_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_ip_qos_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_IP_QOS_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_IP_QOS_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_IP_QOS_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  ingress_ip_qos_acl_meter_action(const switch_object_id_t parent,
                                  switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> ingress_ip_qos_acl_meter_ctrs{
        smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    }
    it->second.set_arg(
        smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    }
    it->second.set_arg(
        smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    } else {
      it->second.init_action_data(
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          ingress_ip_qos_acl_meter_ctrs);
    }
    it->second.set_arg(
        smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0;
    switch_enum_t packet_action = {0};
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          status |= switch_store::v_get(
              handle, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
          if (packet_action.enumdata ==
              SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
                &bytes);
          } else {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                &bytes);
          }
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          status |= switch_store::v_get(
              handle, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
          if (packet_action.enumdata ==
              SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
                &bytes);
          } else {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                &bytes);
          }
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          status |= switch_store::v_get(
              handle, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
          if (packet_action.enumdata ==
              SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT,
                &bytes);
          } else {
            entry.second.get_arg(
                smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                &bytes);
          }
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          &ctr_bytes);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
    }
    attr_w ctr_attr_list(
        SWITCH_INGRESS_IP_QOS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_INGRESS_IP_QOS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_ip_qos_acl_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_ip_qos_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_ip_mirror_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_IP_MIRROR_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  ingress_ip_mirror_acl_meter_action(const switch_object_id_t parent,
                                     switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION,
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> ingress_ip_mirror_acl_meter_ctrs{
        smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t bytes = 0;

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    it->second.init_action_data(
        smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
        ingress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    it->second.init_action_data(
        smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT,
        ingress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    it->second.init_action_data(
        smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT,
        ingress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
        entry.second.get_arg(
            smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
            smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
            &bytes);
      } else {
        entry.second.get_arg(
            smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
            smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT,
            &bytes);
      }
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::
                      D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::
                      D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::
                      D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            &pkt_color);

      ctr_bytes = 0;
      if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
        entry.second.get_arg(
            smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
            smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
            &ctr_bytes);
      } else {
        entry.second.get_arg(
            smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
            smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT,
            &ctr_bytes);
      }
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
    }
    attr_w ctr_attr_list(
        SWITCH_INGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_INGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(
          SWITCH_API_LEVEL_DEBUG,
          SWITCH_OBJECT_TYPE_METER,
          "{}.{}: No stat cache to restore mau stats, "
          "ingress_ip_mirror_acl_meter_action cache list empty, status {}",
          __func__,
          __LINE__,
          status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_ip_mirror_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_ip_qos_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_IP_QOS_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_IP_QOS_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_IP_QOS_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  egress_ip_qos_acl_meter_action(const switch_object_id_t parent,
                                 switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> egress_ip_qos_acl_meter_ctrs{
        smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t bytes = 0;

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    it->second.init_action_data(smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                                egress_ip_qos_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    it->second.init_action_data(smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                                egress_ip_qos_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    it->second.init_action_data(smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                                egress_ip_qos_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
          &ctr_bytes);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
    }
    attr_w ctr_attr_list(
        SWITCH_EGRESS_IP_QOS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_EGRESS_IP_QOS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_ip_qos_acl_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_ip_qos_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_ip_mirror_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_IP_MIRROR_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  egress_ip_mirror_acl_meter_action(const switch_object_id_t parent,
                                    switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> egress_ip_mirror_acl_meter_ctrs{
        smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t bytes = 0;

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    it->second.init_action_data(
        smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
        egress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    it->second.init_action_data(
        smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
        egress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION)));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX,
                            static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    it->second.init_action_data(
        smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
        egress_ip_mirror_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES, bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::
                      D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::
                      D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::
                      D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
          &ctr_bytes);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
    }
    attr_w ctr_attr_list(
        SWITCH_EGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata !=
        SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_EGRESS_IP_MIRROR_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(
          SWITCH_API_LEVEL_DEBUG,
          SWITCH_OBJECT_TYPE_METER,
          "{}.{}: No stat cache to restore mau stats, "
          "egress_ip_mirror_acl_meter_action cache list empty, status {}",
          __func__,
          __LINE__,
          status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_ip_mirror_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_acl_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_ACL_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_ACL_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_ACL_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_egress_acl_meter(switch_object_id_t meter,
                                           uint64_t cir_bps,
                                           uint64_t pir_bps,
                                           uint64_t cburst_bytes,
                                           uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_EGRESS_ACL_METER);
    _MatchKey meter_key(smi_id::T_EGRESS_ACL_METER);
    _ActionEntry meter_action(smi_id::T_EGRESS_ACL_METER);

    status |=
        meter_key.set_exact(smi_id::F_EGRESS_ACL_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_ACL_METER_METER_SPEC_CIR_KBPS,
                             switch_meter_bytes_to_kbps(cir_bps));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_ACL_METER_METER_SPEC_PIR_KBPS,
                             switch_meter_bytes_to_kbps(pir_bps));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_ACL_METER_METER_SPEC_CBS_KBITS,
                             switch_meter_bytes_to_kbps(cburst_bytes));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_ACL_METER_METER_SPEC_PBS_KBITS,
                             switch_meter_bytes_to_kbps(pburst_bytes));
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  egress_acl_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      status |= program_egress_acl_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      status |= program_egress_acl_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class egress_acl_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_ACL_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_ACL_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_ACL_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  egress_acl_meter_index(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_ACL_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {0};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_EGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX, parent);
    action_entry.init_action_data(smi_id::A_SET_EGRESS_ACL_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_EGRESS_ACL_METER_INDEX, parent);
  }
};

class egress_acl_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_ACL_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_ACL_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_ACL_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};
  switch_enum_t packet_action = {0};

 public:
  egress_acl_meter_action(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_ACL_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return;
    }
    std::vector<bf_rt_id_t> egress_acl_meter_ctrs{
        smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_ACL_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_EGRESS_ACL_METER_COUNT,
                                egress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_ACL_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_EGRESS_ACL_METER_COUNT,
                                egress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_ACL_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_ACL_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_ACL_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    it->second.init_action_data(smi_id::A_EGRESS_ACL_METER_COUNT,
                                egress_acl_meter_ctrs);
    it->second.set_arg(
        smi_id::D_EGRESS_ACL_METER_COUNT_PACKET_ACTION,
        switch_meter_color_action_to_packet_action(packet_action));
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_ACL_METER_COUNT,
                           &pkts);
      entry.second.get_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_ACL_METER_COUNT,
                           &bytes);
      entry.first.get_exact(smi_id::F_EGRESS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           value);
      entry.second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_ACL_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES, value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_ACL_METER_COUNT,
                           &ctr_bytes);
      entry.second.get_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_ACL_METER_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_EGRESS_ACL_METER_ACTION_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_acl_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES,
                           ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_acl_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ingress_port_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_ingress_port_meter(switch_object_id_t meter,
                                             uint64_t cir_bps,
                                             uint64_t pir_bps,
                                             uint64_t cburst_bytes,
                                             uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_PORT_METER);
    _MatchKey meter_key(smi_id::T_INGRESS_PORT_METER);
    _ActionEntry meter_action(smi_id::T_INGRESS_PORT_METER);

    status |=
        meter_key.set_exact(smi_id::F_INGRESS_PORT_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_PORT_METER_METER_SPEC_CIR_KBPS,
                             switch_meter_bytes_to_kbps(cir_bps));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_PORT_METER_METER_SPEC_PIR_KBPS,
                             switch_meter_bytes_to_kbps(pir_bps));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_PORT_METER_METER_SPEC_CBS_KBITS,
                             switch_meter_bytes_to_kbps(cburst_bytes));
    status |=
        meter_action.set_arg(smi_id::D_INGRESS_PORT_METER_METER_SPEC_PBS_KBITS,
                             switch_meter_bytes_to_kbps(pburst_bytes));
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  ingress_port_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      status |= program_ingress_port_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      status |= program_ingress_port_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class ingress_port_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  ingress_port_meter_index(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {0};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_INGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX,
        parent);
    action_entry.init_action_data(smi_id::A_SET_INGRESS_PORT_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_INGRESS_PORT_METER_INDEX, parent);
  }
};

class ingress_port_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  ingress_port_meter_action(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_PORT_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return;
    }
    std::vector<bf_rt_id_t> ingress_port_meter_ctrs{
        smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_PORT_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_DROP_AND_COUNT,
                                  ingress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_COUNT,
                                  ingress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_INGRESS_PORT_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_DROP_AND_COUNT,
                                  ingress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_COUNT,
                                  ingress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_INGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_INGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_INGRESS_PORT_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_DROP_AND_COUNT,
                                  ingress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_INGRESS_PORT_METER_COUNT,
                                  ingress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return status;
    }
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_INGRESS_PORT_METER_COUNT,
          &pkts);
      entry.second.get_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_PORT_METER_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_INGRESS_PORT_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
      entry.second.set_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_INGRESS_PORT_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                  value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_PORT_METER_COUNT,
          &ctr_bytes);
      entry.second.get_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
          smi_id::A_INGRESS_PORT_METER_COUNT,
          &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_PORT_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_PORT) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_INGRESS_PORT_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_port_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(
          smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
          ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_port_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_port_meter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_METER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_METER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_METER_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0, _pir_bps = 0, _cburst_bytes = 0, _pburst_bytes = 0;
  switch_enum_t target_type = {0};

 public:
  switch_status_t program_egress_port_meter(switch_object_id_t meter,
                                            uint64_t cir_bps,
                                            uint64_t pir_bps,
                                            uint64_t cburst_bytes,
                                            uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_EGRESS_PORT_METER);
    _MatchKey meter_key(smi_id::T_EGRESS_PORT_METER);
    _ActionEntry meter_action(smi_id::T_EGRESS_PORT_METER);

    status |=
        meter_key.set_exact(smi_id::F_EGRESS_PORT_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_PORT_METER_METER_SPEC_CIR_KBPS,
                             switch_meter_bytes_to_kbps(cir_bps));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_PORT_METER_METER_SPEC_PIR_KBPS,
                             switch_meter_bytes_to_kbps(pir_bps));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_PORT_METER_METER_SPEC_CBS_KBITS,
                             switch_meter_bytes_to_kbps(cburst_bytes));
    status |=
        meter_action.set_arg(smi_id::D_EGRESS_PORT_METER_METER_SPEC_PBS_KBITS,
                             switch_meter_bytes_to_kbps(pburst_bytes));
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  egress_port_meter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CIR, _cir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PIR, _pir_bps);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_CBS, _cburst_bytes);
    status |= switch_store::v_get(parent, SWITCH_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR) {
      status |= program_egress_port_meter(
          get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      status |= program_egress_port_meter(get_parent(), 0, 0, 0, 0);
    }

    status |= auto_object::del();
    return status;
  }
};

class egress_port_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  egress_port_meter_index(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PORT_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t target_type = {0};
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_EGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX,
        parent);
    action_entry.init_action_data(smi_id::A_SET_EGRESS_PORT_METER);
    status |=
        action_entry.set_arg(smi_id::D_SET_EGRESS_PORT_METER_INDEX, parent);
  }
};

class egress_port_meter_action : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_METER_ACTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_METER_ACTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_METER_ACTION_ATTR_PARENT_HANDLE;
  switch_enum_t target_type = {0};

 public:
  egress_port_meter_action(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_PORT_METER_ACTION,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return;
    }
    std::vector<bf_rt_id_t> egress_port_meter_ctrs{
        smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
        smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    switch_enum_t packet_action = {0};

    auto it = match_action_list.begin();
    /* set match/action for color - green */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_PORT_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_GREEN));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_GREEN_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_DROP_AND_COUNT,
                                  egress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_COUNT,
                                  egress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - yellow */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |=
        it->first.set_exact(smi_id::F_EGRESS_PORT_METER_ACTION_COLOR,
                            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_YELLOW_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_YELLOW_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_DROP_AND_COUNT,
                                  egress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_COUNT,
                                  egress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);

    /* set match/action for color - red */
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_EGRESS_PORT_METER_ACTION),
            _ActionEntry(smi_id::T_EGRESS_PORT_METER_ACTION)));
    status |= it->first.set_exact(
        smi_id::F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX,
        static_cast<uint8_t>(parent.data));
    status |= it->first.set_exact(smi_id::F_EGRESS_PORT_METER_ACTION_COLOR,
                                  static_cast<uint8_t>(SWITCH_PKT_COLOR_RED));
    status |= switch_store::v_get(
        parent, SWITCH_METER_ATTR_RED_PACKET_ACTION, packet_action);
    if (packet_action.enumdata == SWITCH_METER_ATTR_RED_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_DROP_AND_COUNT,
                                  egress_port_meter_ctrs);
    } else {
      it->second.init_action_data(smi_id::A_EGRESS_PORT_METER_COUNT,
                                  egress_port_meter_ctrs);
    }
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                       pkts);
    it->second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
                       bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return status;
    }

    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_color = 0;
      entry.second.get_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_PORT_METER_COUNT,
                           &pkts);
      entry.second.get_arg(
          smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_PORT_METER_COUNT,
          &bytes);
      entry.first.get_exact(smi_id::F_EGRESS_PORT_METER_ACTION_COLOR,
                            &pkt_color);
      switch (pkt_color) {
        case SWITCH_PKT_COLOR_GREEN:
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_GREEN_BYTES;
          break;
        case SWITCH_PKT_COLOR_YELLOW:
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_YELLOW_BYTES;
          break;
        case SWITCH_PKT_COLOR_RED:
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count = pkts;
          cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].counter_id =
              SWITCH_METER_COUNTER_ID_RED_PACKETS;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count = bytes;
          cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].counter_id =
              SWITCH_METER_COUNTER_ID_RED_BYTES;
          break;
        default:
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return status;
    }
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                           value);
      entry.second.set_arg(
          smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status |=
        switch_store::v_get(handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      uint64_t pkt_color = 0;
      entry.first.get_exact(smi_id::F_EGRESS_PORT_METER_ACTION_COLOR,
                            &pkt_color);
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_METER_COUNTER_ID_GREEN_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_GREEN_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_GREEN) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_YELLOW_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_YELLOW) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_PACKETS:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS, value);
            }
            break;
          case SWITCH_METER_COUNTER_ID_RED_BYTES:
            if (pkt_color == SWITCH_PKT_COLOR_RED) {
              entry.second.set_arg(
                  smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES, value);
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
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          smi_id::A_EGRESS_PORT_METER_COUNT,
          &ctr_bytes);
      entry.second.get_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_PORT_METER_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_PORT_METER_ACTION_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    status |=
        switch_store::v_get(parent, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    if (target_type.enumdata != SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_PORT) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_EGRESS_PORT_METER_ACTION_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_port_meter_action cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(
          smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES,
          ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_METER,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_port_meter_action status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

switch_status_t meter_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(storm_control, SWITCH_OBJECT_TYPE_STORM_CONTROL);
  REGISTER_OBJECT(storm_control_port_meter_stats,
                  SWITCH_OBJECT_TYPE_SC_PORT_METER_STATS);
  REGISTER_OBJECT(storm_control_stats, SWITCH_OBJECT_TYPE_SC_STATS);
  REGISTER_OBJECT(storm_control_meter, SWITCH_OBJECT_TYPE_STORM_CONTROL_METER);
  REGISTER_OBJECT(ingress_copp_meter, SWITCH_OBJECT_TYPE_INGRESS_COPP_METER);
  REGISTER_OBJECT(ingress_copp_table, SWITCH_OBJECT_TYPE_INGRESS_COPP_TABLE);
  REGISTER_OBJECT(egress_copp_meter, SWITCH_OBJECT_TYPE_EGRESS_COPP_METER);
  REGISTER_OBJECT(egress_copp_table, SWITCH_OBJECT_TYPE_EGRESS_COPP_TABLE);
  REGISTER_OBJECT(ingress_mirror_meter,
                  SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER);
  REGISTER_OBJECT(ingress_mirror_meter_action,
                  SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER_ACTION);
  REGISTER_OBJECT(ingress_mirror_meter_index,
                  SWITCH_OBJECT_TYPE_INGRESS_MIRROR_METER_INDEX);
  REGISTER_OBJECT(egress_mirror_meter, SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER);
  REGISTER_OBJECT(egress_mirror_meter_action,
                  SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER_ACTION);
  REGISTER_OBJECT(egress_mirror_meter_index,
                  SWITCH_OBJECT_TYPE_EGRESS_MIRROR_METER_INDEX);
  REGISTER_OBJECT(ingress_acl_meter, SWITCH_OBJECT_TYPE_INGRESS_ACL_METER);
  REGISTER_OBJECT(ingress_acl_meter_action,
                  SWITCH_OBJECT_TYPE_INGRESS_ACL_METER_ACTION);
  REGISTER_OBJECT(ingress_ip_qos_acl_meter_action,
                  SWITCH_OBJECT_TYPE_INGRESS_IP_QOS_ACL_METER_ACTION);
  REGISTER_OBJECT(ingress_ip_mirror_acl_meter_action,
                  SWITCH_OBJECT_TYPE_INGRESS_IP_MIRROR_ACL_METER_ACTION);
  REGISTER_OBJECT(ingress_acl_meter_index,
                  SWITCH_OBJECT_TYPE_INGRESS_ACL_METER_INDEX);
  REGISTER_OBJECT(egress_acl_meter, SWITCH_OBJECT_TYPE_EGRESS_ACL_METER);
  REGISTER_OBJECT(egress_acl_meter_action,
                  SWITCH_OBJECT_TYPE_EGRESS_ACL_METER_ACTION);
  REGISTER_OBJECT(egress_ip_qos_acl_meter_action,
                  SWITCH_OBJECT_TYPE_EGRESS_IP_QOS_ACL_METER_ACTION);
  REGISTER_OBJECT(egress_ip_mirror_acl_meter_action,
                  SWITCH_OBJECT_TYPE_EGRESS_IP_MIRROR_ACL_METER_ACTION);
  REGISTER_OBJECT(egress_acl_meter_index,
                  SWITCH_OBJECT_TYPE_EGRESS_ACL_METER_INDEX);
  REGISTER_OBJECT(ingress_port_meter, SWITCH_OBJECT_TYPE_INGRESS_PORT_METER);
  REGISTER_OBJECT(ingress_port_meter_action,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_METER_ACTION);
  REGISTER_OBJECT(ingress_port_meter_index,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_METER_INDEX);
  REGISTER_OBJECT(egress_port_meter, SWITCH_OBJECT_TYPE_EGRESS_PORT_METER);
  REGISTER_OBJECT(egress_port_meter_action,
                  SWITCH_OBJECT_TYPE_EGRESS_PORT_METER_ACTION);
  REGISTER_OBJECT(egress_port_meter_index,
                  SWITCH_OBJECT_TYPE_EGRESS_PORT_METER_INDEX);
  return status;
}

switch_status_t meter_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
