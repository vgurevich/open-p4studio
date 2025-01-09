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
#include <mc_mgr/mc_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_mc.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_tm.h>
}

#include <memory>
#include <utility>
#include <vector>
#include <set>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"
#include "s3/switch_packet.h"

namespace smi {
using ::smi::logging::switch_log;
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using namespace ::bfrt;      // NOLINT(build/namespaces)

const BfRtLearn *bfrtlearn = NULL;

thread_local bool digest_context = false;

class ingress_stp_group : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_STP_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_STP_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_STP_GROUP_ATTR_PARENT_HANDLE;

 public:
  ingress_stp_group(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_STP_GROUP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_lag_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE, port_lag_handle);

    status |= match_key.set_exact(smi_id::F_INGRESS_STP_GROUP_PORT_LAG_INDEX,
                                  compute_port_lag_index(port_lag_handle));
    status |= match_key.set_exact(
        smi_id::F_INGRESS_STP_GROUP, parent, SWITCH_STP_PORT_ATTR_STP_HANDLE);

    action_entry.init_action_data(smi_id::A_INGRESS_STP_SET_STP_STATE);
    action_entry.set_arg(smi_id::P_INGRESS_STP_SET_STP_STATE_STP_STATE,
                         parent,
                         SWITCH_STP_PORT_ATTR_STATE);
  }
};

class egress_stp_group : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_STP_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_STP_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_STP_GROUP_ATTR_PARENT_HANDLE;

 public:
  egress_stp_group(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_STP_GROUP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_lag_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE, port_lag_handle);

    status |= match_key.set_exact(smi_id::F_EGRESS_STP_GROUP_PORT_LAG_INDEX,
                                  compute_port_lag_index(port_lag_handle));
    status |= match_key.set_exact(
        smi_id::F_EGRESS_STP_GROUP, parent, SWITCH_STP_PORT_ATTR_STP_HANDLE);

    action_entry.init_action_data(smi_id::A_EGRESS_STP_SET_STP_STATE);
    action_entry.set_arg(smi_id::P_EGRESS_STP_SET_STP_STATE_STP_STATE,
                         parent,
                         SWITCH_STP_PORT_ATTR_STATE);
  }
};

// Keep in sync with stp_port schema
// Forwarding -> 00
// Blocking   -> 01
// Learning   -> 10
switch_status_t update_ingress_stp_check_register(uint16_t dev_port,
                                                  switch_object_id_t bd_handle,
                                                  uint8_t stp_state) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t register_index = 0;
  bf_rt_target_t dev_target = compute_dev_target_for_table(
      dev_port, smi_id::T_INGRESS_STP0_CHECK, true);
  uint8_t bit_0 = stp_state & 1U;
  uint8_t bit_1 = (stp_state >> 1) & 1U;

  register_index = (compute_bd(bd_handle) << 7) | (dev_port & 0x7f);

  {
    _Table table(dev_target, get_bf_rt_info(), smi_id::T_INGRESS_STP0_CHECK);
    _MatchKey register_key(smi_id::T_INGRESS_STP0_CHECK);
    _ActionEntry register_action(smi_id::T_INGRESS_STP0_CHECK);

    register_action.init_indirect_data();
    status |= register_key.set_exact(
        smi_id::F_INGRESS_STP0_CHECK_REGISTER_INDEX, register_index);
    status |= register_action.set_arg(
        smi_id::D_INGRESS_STP0_CHECK_REGISTER_DATA, bit_0);
    status = table.entry_modify(register_key, register_action);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_STP_PORT,
                 "{}:{}: failed to set ingress_stp0_check stateful register,"
                 " status {} dev_port {} vlan_id {} stp_state {}",
                 __func__,
                 __LINE__,
                 status,
                 dev_port,
                 compute_bd(bd_handle),
                 stp_state);
    }
  }

  {
    _Table table(dev_target, get_bf_rt_info(), smi_id::T_INGRESS_STP1_CHECK);
    _MatchKey register_key(smi_id::T_INGRESS_STP1_CHECK);
    _ActionEntry register_action(smi_id::T_INGRESS_STP1_CHECK);

    register_action.init_indirect_data();
    status |= register_key.set_exact(
        smi_id::F_INGRESS_STP1_CHECK_REGISTER_INDEX, register_index);
    status |= register_action.set_arg(
        smi_id::D_INGRESS_STP1_CHECK_REGISTER_DATA, bit_1);
    status = table.entry_modify(register_key, register_action);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_STP_PORT,
                 "{}:{}: failed to set ingress_stp1_check stateful register,"
                 " status {} dev_port {} vlan_id {} stp_state {}",
                 __func__,
                 __LINE__,
                 status,
                 dev_port,
                 compute_bd(bd_handle),
                 stp_state);
    }
  }

  return status;
}

switch_status_t update_ingress_stp_port(std::vector<uint16_t> &dev_port_list,
                                        switch_object_id_t vlan_handle,
                                        uint8_t stp_state) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bd_handle;

  status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
  for (auto const &dev_port : dev_port_list) {
    status |= update_ingress_stp_check_register(dev_port, bd_handle, stp_state);
  }

  return status;
}

class ingress_stp : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_INGRESS_STP;
  static const switch_attr_id_t status_attr_id = SWITCH_INGRESS_STP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_STP_ATTR_PARENT_HANDLE;

 public:
  ingress_stp(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t port_lag_handle = {0};
    std::set<switch_object_id_t> stp_vlan_members;

    uint8_t stp_state = SWITCH_STP_PORT_ATTR_STATE_FORWARDING;

    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE, port_lag_handle);

    switch_enum_t stp_state_enum;
    status |=
        switch_store::v_get(parent, SWITCH_STP_PORT_ATTR_STATE, stp_state_enum);
    stp_state = static_cast<uint8_t>(stp_state_enum.enumdata);

    switch_object_id_t stp_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_STP_HANDLE, stp_handle);

    std::vector<uint16_t> dev_port_list;
    switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      dev_port_list.push_back(dev_port);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      std::vector<switch_object_id_t> lag_members;
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
      dev_port_list.reserve(lag_members.size());
      for (auto const mbr : lag_members) {
        switch_object_id_t mbr_port_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
        status |= switch_store::v_get(
            mbr_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        dev_port_list.push_back(dev_port);
      }
    } else {
      status |= SWITCH_STATUS_FAILURE;
      return;
    }

    status |= switch_store::referencing_set_get(
        stp_handle, SWITCH_OBJECT_TYPE_VLAN, stp_vlan_members);
    for (auto const mbr : stp_vlan_members) {
      status |= update_ingress_stp_port(dev_port_list, mbr, stp_state);
    }
  }
};

switch_status_t update_egress_stp_check_register(uint16_t dev_port,
                                                 switch_object_id_t bd_handle,
                                                 uint8_t stp_state) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t register_index = 0;
  bf_rt_target_t dev_target =
      compute_dev_target_for_table(dev_port, smi_id::T_EGRESS_STP_CHECK, false);
  uint8_t bit_0 = stp_state == SWITCH_STP_PORT_ATTR_STATE_FORWARDING ? 0 : 1;

  _Table table(dev_target, get_bf_rt_info(), smi_id::T_EGRESS_STP_CHECK);
  register_index = (compute_bd(bd_handle) << 7) | (dev_port & 0x7f);
  _MatchKey register_key(smi_id::T_EGRESS_STP_CHECK);
  _ActionEntry register_action(smi_id::T_EGRESS_STP_CHECK);

  register_action.init_indirect_data();
  status |= register_key.set_exact(smi_id::F_EGRESS_STP_CHECK_REGISTER_INDEX,
                                   register_index);
  status |=
      register_action.set_arg(smi_id::D_EGRESS_STP_CHECK_REGISTER_DATA, bit_0);
  status = table.entry_modify(register_key, register_action);

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_STP_PORT,
               "{}:{}: failed to set egress_stp_check stateful register,"
               " status {} dev_port {} bd {} stp_state {}",
               __func__,
               __LINE__,
               status,
               dev_port,
               compute_bd(bd_handle),
               stp_state);
  }

  return status;
}

switch_status_t update_egress_stp_port(std::vector<uint16_t> &dev_port_list,
                                       switch_object_id_t vlan_handle,
                                       uint8_t stp_state) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bd_handle;

  status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
  for (auto const &dev_port : dev_port_list) {
    status |= update_egress_stp_check_register(dev_port, bd_handle, stp_state);
  }

  return status;
}

class egress_stp : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_EGRESS_STP;
  static const switch_attr_id_t status_attr_id = SWITCH_EGRESS_STP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_STP_ATTR_PARENT_HANDLE;

 public:
  egress_stp(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t port_lag_handle = {0};
    std::set<switch_object_id_t> stp_vlan_members;
    uint8_t stp_state = SWITCH_STP_PORT_ATTR_STATE_FORWARDING;

    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE, port_lag_handle);

    switch_enum_t stp_state_enum;
    status |=
        switch_store::v_get(parent, SWITCH_STP_PORT_ATTR_STATE, stp_state_enum);
    stp_state = static_cast<uint8_t>(stp_state_enum.enumdata);

    std::vector<uint16_t> dev_port_list;
    switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      dev_port_list.push_back(dev_port);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      std::vector<switch_object_id_t> lag_members;
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
      dev_port_list.reserve(lag_members.size());
      for (auto const mbr : lag_members) {
        switch_object_id_t mbr_port_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
        status |= switch_store::v_get(
            mbr_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        dev_port_list.push_back(dev_port);
      }
    } else {
      status |= SWITCH_STATUS_FAILURE;
      return;
    }

    switch_object_id_t stp_handle = {0};
    status |= switch_store::v_get(
        parent, SWITCH_STP_PORT_ATTR_STP_HANDLE, stp_handle);
    status |= switch_store::referencing_set_get(
        stp_handle, SWITCH_OBJECT_TYPE_VLAN, stp_vlan_members);
    for (auto const mbr : stp_vlan_members) {
      status |= update_egress_stp_port(dev_port_list, mbr, stp_state);
    }
  }
};

class stp_factory : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_STP_FACTORY;
  static const switch_attr_id_t status_attr_id = SWITCH_STP_FACTORY_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_STP_FACTORY_ATTR_PARENT_HANDLE;
  std::unique_ptr<object> ingress_object;
  std::unique_ptr<object> egress_object;

 public:
  stp_factory(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (feature::is_feature_set(SWITCH_FEATURE_MSTP)) {
      ingress_object = std::unique_ptr<ingress_stp_group>(
          new ingress_stp_group(parent, status));
    } else {
      ingress_object =
          std::unique_ptr<ingress_stp>(new ingress_stp(parent, status));
      egress_object =
          std::unique_ptr<egress_stp>(new egress_stp(parent, status));
    }
  }

  switch_status_t create_update() {
    if (ingress_object != NULL) {
      ingress_object->create_update();
    }
    if (egress_object != NULL) {
      egress_object->create_update();
    }
    return auto_object::create_update();
  }

  switch_status_t del() {
    if (ingress_object != NULL) {
      ingress_object->del();
    }
    if (egress_object != NULL) {
      egress_object->del();
    }
    return auto_object::del();
  }
};

class smac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SMAC;
  static const switch_attr_id_t status_attr_id = SWITCH_SMAC_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id = SWITCH_SMAC_ATTR_PARENT_HANDLE;
  bool age_out = false;

 public:
  smac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_SMAC, status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_id_t port_lag_handle = {}, vlan_handle = {}, bd_handle = {},
                       device_handle = {};
    bool learning = false;
    switch_enum_t e = {0};
    uint32_t aging_interval = 0;

    // if this MAC entry is being created/deleted via a driver callback, use the
    // l2 session
    if (digest_context) {
      device_session_set(SWITCH_CONTEXT.get_l2_session_ptr());
    }

    std::vector<std::reference_wrapper<const switch_attribute_t>> mac_attrs;
    mac_attrs.reserve(16);

    status = switch_store::attribute_get_all(parent, mac_attrs);
    for (const auto &ref_attr : mac_attrs) {
      const switch_attribute_t &mac_attr = ref_attr.get();
      switch (mac_attr.id) {
        case SWITCH_MAC_ENTRY_ATTR_DEVICE:
          device_handle = mac_attr.value.oid;
          break;
        case SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE:
          port_lag_handle = mac_attr.value.oid;
          break;
        case SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE:
          vlan_handle = mac_attr.value.oid;
          break;
        case SWITCH_MAC_ENTRY_ATTR_TYPE:
          e.enumdata = mac_attr.value.enumdata.enumdata;
          break;
        case SWITCH_MAC_ENTRY_ATTR_AGE_OUT:
          age_out = mac_attr.value.booldata;
          break;
        case SWITCH_MAC_ENTRY_ATTR_ALLOW_MOVE:
        case SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS:
        case SWITCH_MAC_ENTRY_ATTR_ACTION:
        case SWITCH_MAC_ENTRY_ATTR_DEST_IP:
        case SWITCH_MAC_ENTRY_ATTR_INTERNAL_OBJECT:
          break;
        default:
          switch_log(
              SWITCH_API_LEVEL_ERROR,
              SWITCH_OBJECT_TYPE_ROUTE,
              "{}:{}: attribute_get_all fail invalid attr id {} route.{}",
              __func__,
              __LINE__,
              mac_attr.id,
              switch_store::handle_to_id(parent));
          status = SWITCH_STATUS_FAILURE;
          return;
          break;
      }
    }

    status |=
        switch_store::v_get(vlan_handle, SWITCH_VLAN_ATTR_LEARNING, learning);
    if (e.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_DYNAMIC) {
      if (sai_mode()) {
        status |= switch_store::v_get(device_handle,
                                      SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL,
                                      aging_interval);
      } else {
        status |= switch_store::v_get(
            vlan_handle, SWITCH_VLAN_ATTR_AGING_INTERVAL, aging_interval);
      }
    }
    status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

    if (!learning) return;

    status |=
        match_key.set_exact(smi_id::F_SMAC_LOCAL_MD_BD, compute_bd(bd_handle));
    status |= match_key.set_exact(
        smi_id::F_SMAC_SRC_ADDR, parent, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS);

    action_entry.init_action_data(smi_id::A_SMAC_HIT);
    // When port_lag_handle refers to a tunnel object, we want any matching
    // outer source MAC to trigger a mac move (if allowed) by setting
    // src_move != 0. This code will program port_lag_index value 0, which
    // when compared to any non-zero local_md.ingress_port_lag_index, will
    // cause src_move to be set to a non-zero value.
    action_entry.set_arg(smi_id::P_SMAC_HIT_PORT_LAG_INDEX,
                         compute_port_lag_index(port_lag_handle));
    action_entry.set_arg(smi_id::D_SMAC_TTL, aging_interval);
  }

  switch_status_t del() {
    switch_mac_payload_t payload = {};
    switch_mac_event_data_t mac_data;
    switch_store::v_get(auto_obj.get_parent(),
                        SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                        payload.port_lag_handle);
    switch_store::v_get(
        auto_obj.get_parent(), SWITCH_MAC_ENTRY_ATTR_AGE_OUT, age_out);
    payload.mac_event =
        age_out ? SWITCH_MAC_EVENT_AGE : SWITCH_MAC_EVENT_DELETE;
    payload.mac_handle = auto_obj.get_parent();
    mac_data.payload.push_back(payload);
    smi::event::mac_event_notify(mac_data);
    return p4_object_match_action::del();
  }
};

class dmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DMAC;
  static const switch_attr_id_t status_attr_id = SWITCH_DMAC_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id = SWITCH_DMAC_ATTR_PARENT_HANDLE;

 public:
  dmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_DMAC, status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_id_t handle = {0}, vlan_handle = {0}, bd_handle = {0};
    switch_mac_addr_t match_mac = {0};
    switch_enum_t e = {};

    // if this MAC entry is being created/deleted via a driver callback, use the
    // l2 session
    if (digest_context) {
      device_session_set(SWITCH_CONTEXT.get_l2_session_ptr());
    }

    status |= switch_store::v_get(
        parent, SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle);
    status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    status |=
        match_key.set_exact(smi_id::F_DMAC_LOCAL_MD_BD, compute_bd(bd_handle));
    status |= switch_store::v_get(
        parent, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, match_mac);
    status |= match_key.set_exact(
        smi_id::F_DMAC_DST_ADDR, parent, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS);

    status |= switch_store::v_get(parent, SWITCH_MAC_ENTRY_ATTR_ACTION, e);

    status |= switch_store::v_get(
        parent, SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, handle);
    if ((switch_store::object_type_query(handle) == SWITCH_OBJECT_TYPE_PORT) ||
        (switch_store::object_type_query(handle) == SWITCH_OBJECT_TYPE_LAG)) {
      action_entry.init_action_data(smi_id::A_DMAC_HIT);
      action_entry.set_arg(smi_id::P_DMAC_HIT_PORT_LAG_INDEX,
                           compute_port_lag_index(handle));
    } else if (switch_store::object_type_query(handle) ==
               SWITCH_OBJECT_TYPE_NEXTHOP) {
      action_entry.init_action_data(smi_id::A_DMAC_REDIRECT);
      action_entry.set_arg(smi_id::P_DMAC_REDIRECT_NEXTHOP_INDEX,
                           compute_nexthop_index(handle));
    } else if (switch_store::object_type_query(handle) ==
               SWITCH_OBJECT_TYPE_TUNNEL) {
      switch_enum_t peer_mode;
      switch_object_id_t drop_nexthop = {}, device_handle = {};
      status |= switch_store::v_get(
          parent, SWITCH_MAC_ENTRY_ATTR_DEVICE, device_handle);
      status |=
          switch_store::v_get(handle, SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode);
      if (is_nexthop_resolution_feature_on(device_handle) &&
          feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN) &&
          peer_mode.enumdata == SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
        action_entry.init_action_data(smi_id::A_DMAC_REDIRECT);
        action_entry.set_arg(smi_id::P_DMAC_REDIRECT_NEXTHOP_INDEX,
                             compute_nexthop_index(handle));
      } else {
        // This combination is not supported, packet will be dropped
        status |= switch_store::v_get(device_handle,
                                      SWITCH_DEVICE_ATTR_DROP_NEXTHOP_HANDLE,
                                      drop_nexthop);
        action_entry.init_action_data(smi_id::A_DMAC_REDIRECT);
        action_entry.set_arg(smi_id::P_DMAC_REDIRECT_NEXTHOP_INDEX,
                             compute_nexthop_index(drop_nexthop));
      }
    }
  }
};

class egress_bd_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_BD_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_BD_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_BD_MAPPING_ATTR_PARENT_HANDLE;

 public:
  egress_bd_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_BD_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t egress_acl_handle{};
    switch_bd_label_t vlan_rif_label{};
    switch_object_id_t rif_handle = {0}, vrf_handle = {0}, bridge_handle = {0},
                       vlan_handle{};
    switch_mac_addr_t src_mac_rif_update = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t anycast_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    uint32_t mtu = 0;

    status |= match_key.set_exact(smi_id::F_EGRESS_BD_MAPPING_LOCAL_MD_BD,
                                  compute_bd(parent));

    action_entry.init_action_data(smi_id::A_EGRESS_SET_BD_PROPERTIES);
    status |= get_parent_of_bd(
        parent, vlan_handle, bridge_handle, rif_handle, vrf_handle);

    if (vrf_handle != 0) {
      status |= action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_SMAC,
                                     vrf_handle,
                                     SWITCH_VRF_ATTR_SRC_MAC);
    }

    if (vlan_handle != 0) {
      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_EGRESS_ACL_HANDLE, egress_acl_handle);
      if (egress_acl_handle.data != 0) {
        vlan_rif_label = compute_bind_label(vlan_handle, egress_acl_handle);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SET_BD_PROPERTIES_BD_LABEL, vlan_rif_label);
      } else {
        status |=
            action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_BD_LABEL,
                                 vlan_handle,
                                 SWITCH_VLAN_ATTR_EGRESS_VLAN_RIF_LABEL);
      }
    }

    if (rif_handle != 0) {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_EGRESS_ACL_HANDLE, egress_acl_handle);
      if (egress_acl_handle.data != 0) {
        vlan_rif_label = compute_bind_label(rif_handle, egress_acl_handle);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SET_BD_PROPERTIES_BD_LABEL, vlan_rif_label);
      } else {
        status |=
            action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_BD_LABEL,
                                 rif_handle,
                                 SWITCH_RIF_ATTR_EGRESS_VLAN_RIF_LABEL);
      }

      status = switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, src_mac_rif_update);
      status = switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, anycast_mac);

      if (SWITCH_MAC_VALID(anycast_mac)) {
        status |= action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_SMAC,
                                       rif_handle,
                                       SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR);
      } else if (SWITCH_MAC_VALID(src_mac_rif_update)) {
        status |= action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_SMAC,
                                       rif_handle,
                                       SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE);
      } else {
        status |= action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_SMAC,
                                       rif_handle,
                                       SWITCH_RIF_ATTR_SRC_MAC);
      }

      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_MTU, mtu);
      mtu += 1;
      status |=
          action_entry.set_arg(smi_id::P_EGRESS_SET_BD_PROPERTIES_MTU, mtu);
    }
  }
};

class vlan_decap : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_VLAN_DECAP;
  static const switch_attr_id_t status_attr_id = SWITCH_VLAN_DECAP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VLAN_DECAP_ATTR_PARENT_HANDLE;

 public:
  vlan_decap(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_VLAN_DECAP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    typedef struct rule_spec_ {
      uint8_t vlan_0_valid;
      uint8_t vlan_0_valid_mask;
      uint8_t vlan_1_valid;
      uint8_t vlan_1_valid_mask;
      bf_rt_action_id_t action_id;
    } rule_spec_t;

    rule_spec_t rules[] = {
        /* remove two vlan tags */
        {1, 1, 1, 1, smi_id::A_REMOVE_DOUBLE_TAG},
        /* remove a single vlan tag */
        {1, 1, 0, 1, smi_id::A_REMOVE_VLAN_TAG},
    };
    const size_t num_elements = sizeof(rules) / sizeof(rule_spec_t);

    auto it = match_action_list.begin();
    for (uint32_t i = 0; i < num_elements; i++) {
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_VLAN_DECAP),
                                        _ActionEntry(smi_id::T_VLAN_DECAP)));
      status |= it->first.set_exact(smi_id::F_VLAN_DECAP_PRIORITY,
                                    static_cast<uint32_t>(i));
      status |= it->first.set_ternary(smi_id::F_VLAN_DECAP_VLAN_0_VALID,
                                      rules[i].vlan_0_valid,
                                      rules[i].vlan_0_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VLAN_DECAP_VLAN_1_VALID,
                                      rules[i].vlan_1_valid,
                                      rules[i].vlan_1_valid_mask);
      it->second.init_action_data(rules[i].action_id);
    }
  }
};

class port_bd_to_vlan_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_BD_TO_VLAN_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_BD_TO_VLAN_MAPPING_ATTR_PARENT_HANDLE;

 public:
  port_bd_to_vlan_mapping(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_PORT_BD_TO_VLAN_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint16_t vlan_id = 0, outer_vlan_id = 0, inner_vlan_id = 0;
    switch_object_id_t bd_handle = {0}, port_lag_handle = {0}, handle = {0},
                       vlan_member_handle = {0}, rif_handle = {0},
                       bd_member_parent_handle;
    switch_enum_t rif_type = {0};
    switch_object_type_t bd_member_parent_type, bd_member_type;

    status |= get_bd_from_bd_member(parent, bd_handle);
    status |=
        switch_store::v_get(parent, SWITCH_BD_MEMBER_ATTR_HANDLE, handle.data);
    bd_member_type = switch_store::object_type_query(handle);
    if (bd_member_type != SWITCH_OBJECT_TYPE_PORT &&
        bd_member_type != SWITCH_OBJECT_TYPE_LAG) {
      clear_attrs();
      return;
    }
    status |= switch_store::v_get(
        parent, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, bd_member_parent_handle);
    bd_member_parent_type =
        switch_store::object_type_query(bd_member_parent_handle);
    if (bd_member_parent_type == SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      status |= switch_store::v_get(
          parent, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, vlan_member_handle);
    } else if (bd_member_parent_type == SWITCH_OBJECT_TYPE_RIF) {
      status |= switch_store::v_get(
          parent, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, rif_handle);
      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
    }

    // Get vlan
    port_lag_handle = handle;
    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT) {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_OUTER_VLAN_ID, outer_vlan_id);
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_INNER_VLAN_ID, inner_vlan_id);
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_OUTER_VLAN_ID, outer_vlan_id);
    } else {
      // Get vlan for port/lag
      status |=
          switch_store::v_get(parent, SWITCH_BD_MEMBER_ATTR_VLAN_ID, vlan_id);
    }

    status |=
        match_key.set_exact(smi_id::F_PORT_BD_TO_VLAN_MAPPING_PORT_LAG_INDEX,
                            compute_port_lag_index(port_lag_handle));
    status |= match_key.set_exact(smi_id::F_PORT_BD_TO_VLAN_MAPPING_BD,
                                  compute_bd(bd_handle));
    status |=
        switch_store::v_get(parent, SWITCH_BD_MEMBER_ATTR_VLAN_ID, vlan_id);

    if (switch_store::object_type_query(vlan_member_handle) ==
        SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      switch_enum_t tag_mode = {0};
      status |= switch_store::v_get(
          vlan_member_handle, SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE, tag_mode);
      if (tag_mode.enumdata == SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_UNTAGGED) {
        action_entry.init_action_data(
            smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED);
      } else if (tag_mode.enumdata ==
                 SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_PRIORITY_TAGGED) {
        action_entry.init_action_data(
            smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED);
      }
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT) {
      action_entry.init_action_data(
          smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED);
      status |= action_entry.set_arg(
          smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID0,
          outer_vlan_id);
      status |= action_entry.set_arg(
          smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID1,
          inner_vlan_id);
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      action_entry.init_action_data(
          smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED);
      status |= action_entry.set_arg(
          smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID, outer_vlan_id);
    }
  }
};

class bd_to_vlan_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_BD_TO_VLAN_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BD_TO_VLAN_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BD_TO_VLAN_MAPPING_ATTR_PARENT_HANDLE;

  uint16_t vlan_id = 0;
  uint16_t bd_id = 0;
  uint16_t dev_id = 0;

 public:
  bd_to_vlan_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_BD_TO_VLAN_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vlan_handle = {0};
    switch_object_id_t device_handle = {0};

    status |=
        switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, vlan_handle);
    if (switch_store::object_type_query(vlan_handle) != SWITCH_OBJECT_TYPE_VLAN)
      return;

    status |=
        switch_store::v_get(vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);
    status |= switch_store::v_get(
        vlan_handle, SWITCH_VLAN_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);

    bd_id = compute_bd(parent);

    status |= match_key.set_exact(smi_id::F_BD_TO_VLAN_MAPPING_BD, bd_id);
    action_entry.init_action_data(smi_id::A_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED);
    status |= action_entry.set_arg(
        smi_id::P_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID, vlan_id);

    status |= switch_pktdriver_bd_to_vlan_mapping_add(dev_id, bd_id, vlan_id);
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (bd_id) {
      status |=
          switch_pktdriver_bd_to_vlan_mapping_delete(dev_id, bd_id, vlan_id);
    }
    p4_object_match_action::del();
    return status;
  }
};

class ingress_bd_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_BD_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_BD_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_BD_STATS_ATTR_PARENT_HANDLE;

  switch_object_type_t bd_parent_type = {};

 public:
  ingress_bd_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_BD_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t bd_handle = {0};
    switch_object_id_t bd_parent_handle = {0};
    std::set<uint8_t> l2_pkt_types({SWITCH_PACKET_TYPE_UNICAST,
                                    SWITCH_PACKET_TYPE_MULTICAST,
                                    SWITCH_PACKET_TYPE_BROADCAST});
    std::vector<bf_rt_id_t> cntrs{smi_id::P_INGRESS_BD_STATS_BYTES,
                                  smi_id::P_INGRESS_BD_STATS_PKTS};

    if (switch_store::object_type_query(parent) == SWITCH_OBJECT_TYPE_BRIDGE) {
      switch_enum_t type = {0};
      status |= switch_store::v_get(parent, SWITCH_BRIDGE_ATTR_TYPE, type);
      if (type.enumdata == SWITCH_BRIDGE_ATTR_TYPE_DOT1Q) {
        clear_attrs();
        return;
      }
    }

    if (switch_store::object_type_query(parent) == SWITCH_OBJECT_TYPE_BD) {
      bd_handle = parent;
    } else {
      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
    }

    if (!bd_handle.data) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 auto_ot,
                 "{}.{}: Failed to get bd for parent object {}",
                 __func__,
                 __LINE__,
                 parent);
      status = SWITCH_STATUS_FAILURE;
      return;
    }

    status |= switch_store::v_get(
        bd_handle, SWITCH_BD_ATTR_PARENT_HANDLE, bd_parent_handle);
    bd_parent_type = switch_store::object_type_query(bd_parent_handle);

    auto it = match_action_list.begin();
    for (auto pkt_type : l2_pkt_types) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INGRESS_BD_STATS),
              _ActionEntry(smi_id::T_INGRESS_BD_STATS)));
      status |= it->first.set_exact(smi_id::F_INGRESS_BD_STATS_BD,
                                    compute_bd(bd_handle));
      status |=
          it->first.set_exact(smi_id::F_INGRESS_BD_STATS_PKT_TYPE, pkt_type);
      it->second.init_action_data(smi_id::A_INGRESS_BD_STATS_COUNT, cntrs);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_type = 0;
      entry.second.get_arg(smi_id::P_INGRESS_BD_STATS_BYTES,
                           smi_id::A_INGRESS_BD_STATS_COUNT,
                           &bytes);
      entry.second.get_arg(smi_id::P_INGRESS_BD_STATS_PKTS,
                           smi_id::A_INGRESS_BD_STATS_COUNT,
                           &pkts);
      entry.first.get_exact(smi_id::F_INGRESS_BD_STATS_PKT_TYPE, &pkt_type);
      switch_counter_t cntr_pkts;
      switch_counter_t cntr_bytes;
      switch (static_cast<uint8_t>(pkt_type)) {
        case SWITCH_PACKET_TYPE_UNICAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_IN_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_IN_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_UCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_IN_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_UCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        case SWITCH_PACKET_TYPE_MULTICAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_IN_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_IN_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_MCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_IN_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_MCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        case SWITCH_PACKET_TYPE_BROADCAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_IN_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_IN_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_IN_BCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_IN_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_IN_BCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        default:
          cntr_pkts.counter_id = 0;
          cntr_bytes.counter_id = 0;
          break;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_BYTES, value);
      entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t pkt_type = 0;
      entry.first.get_exact(smi_id::F_INGRESS_BD_STATS_PKT_TYPE, &pkt_type);

      for (auto cntr_id : cntr_ids) {
        switch (static_cast<uint8_t>(pkt_type)) {
          case SWITCH_PACKET_TYPE_UNICAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_IN_UCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_IN_UCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_UCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_BYTES, value);
            }
            break;
          case SWITCH_PACKET_TYPE_MULTICAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_IN_MCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_IN_MCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_MCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_BYTES, value);
            }
            break;
          case SWITCH_PACKET_TYPE_BROADCAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_IN_BCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_IN_BCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_IN_BCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_BYTES, value);
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
      entry.second.get_arg(smi_id::P_INGRESS_BD_STATS_BYTES,
                           smi_id::A_INGRESS_BD_STATS_COUNT,
                           &ctr_bytes);
      entry.second.get_arg(smi_id::P_INGRESS_BD_STATS_PKTS,
                           smi_id::A_INGRESS_BD_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_BD_STATS_ATTR_MAU_STATS_CACHE);
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
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_INGRESS_BD_STATS_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_bd_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_BYTES, ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::P_INGRESS_BD_STATS_PKTS, ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_bd_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_bd_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_BD_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_BD_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_BD_STATS_ATTR_PARENT_HANDLE;

  switch_object_type_t bd_parent_type = {};

 public:
  egress_bd_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_BD_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t bd_handle = {0};
    switch_object_id_t bd_parent_handle = {0};
    std::set<uint8_t> l2_pkt_types({SWITCH_PACKET_TYPE_UNICAST,
                                    SWITCH_PACKET_TYPE_MULTICAST,
                                    SWITCH_PACKET_TYPE_BROADCAST});
    std::vector<bf_rt_id_t> cntrs{smi_id::P_EGRESS_BD_STATS_BYTES,
                                  smi_id::P_EGRESS_BD_STATS_PKTS};

    if (switch_store::object_type_query(parent) == SWITCH_OBJECT_TYPE_BRIDGE) {
      switch_enum_t type = {0};
      status |= switch_store::v_get(parent, SWITCH_BRIDGE_ATTR_TYPE, type);
      if (type.enumdata == SWITCH_BRIDGE_ATTR_TYPE_DOT1Q) {
        clear_attrs();
        return;
      }
    }

    if (switch_store::object_type_query(parent) == SWITCH_OBJECT_TYPE_BD) {
      bd_handle = parent;
    } else {
      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
    }

    if (!bd_handle.data) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 auto_ot,
                 "{}.{}: Failed to get bd for parent object {}",
                 __func__,
                 __LINE__,
                 parent);
      status = SWITCH_STATUS_FAILURE;
      return;
    }

    status |= switch_store::v_get(
        bd_handle, SWITCH_BD_ATTR_PARENT_HANDLE, bd_parent_handle);
    bd_parent_type = switch_store::object_type_query(bd_parent_handle);

    auto it = match_action_list.begin();
    for (auto pkt_type : l2_pkt_types) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_EGRESS_BD_STATS),
              _ActionEntry(smi_id::T_EGRESS_BD_STATS)));
      status |= it->first.set_exact(smi_id::F_EGRESS_BD_STATS_LOCAL_MD_BD,
                                    compute_bd(bd_handle));
      status |= it->first.set_exact(smi_id::F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE,
                                    pkt_type);
      it->second.init_action_data(smi_id::A_EGRESS_BD_STATS_COUNT, cntrs);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      uint64_t pkt_type = 0;
      entry.second.get_arg(smi_id::P_EGRESS_BD_STATS_BYTES,
                           smi_id::A_EGRESS_BD_STATS_COUNT,
                           &bytes);
      entry.second.get_arg(smi_id::P_EGRESS_BD_STATS_PKTS,
                           smi_id::A_EGRESS_BD_STATS_COUNT,
                           &pkts);
      switch_counter_t cntr_pkts;
      switch_counter_t cntr_bytes;
      entry.first.get_exact(smi_id::F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE,
                            &pkt_type);
      switch (static_cast<uint8_t>(pkt_type)) {
        case SWITCH_PACKET_TYPE_UNICAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_OUT_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_OUT_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_UCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_UCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        case SWITCH_PACKET_TYPE_MULTICAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_OUT_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_OUT_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_MCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_MCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        case SWITCH_PACKET_TYPE_BROADCAST:
          switch (bd_parent_type) {
            case SWITCH_OBJECT_TYPE_RIF:
              cntr_pkts.counter_id = SWITCH_RIF_COUNTER_ID_OUT_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_RIF_COUNTER_ID_OUT_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_RIF_COUNTER_ID_OUT_BCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
              cntr_pkts.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_VLAN_COUNTER_ID_OUT_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_VLAN_COUNTER_ID_OUT_BCAST_BYTES] = cntr_bytes;
              break;
            case SWITCH_OBJECT_TYPE_BRIDGE:
              cntr_pkts.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_PKTS;
              cntr_pkts.count = pkts;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_PKTS] = cntr_pkts;
              cntr_bytes.counter_id = SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_BYTES;
              cntr_bytes.count = bytes;
              cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_BYTES] = cntr_bytes;
              break;
            default:
              break;
          }
          break;
        default:
          cntr_pkts.counter_id = 0;
          cntr_bytes.counter_id = 0;
          break;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_BYTES, value);
      entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t pkt_type = 0;
      entry.first.get_exact(smi_id::F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE,
                            &pkt_type);

      for (auto cntr_id : cntr_ids) {
        switch (static_cast<uint8_t>(pkt_type)) {
          case SWITCH_PACKET_TYPE_UNICAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_UCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_UCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_UCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id ==
                         SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_BYTES, value);
            }
            break;
          case SWITCH_PACKET_TYPE_MULTICAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_MCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_MCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_MCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id ==
                         SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_BYTES, value);
            }
            break;
          case SWITCH_PACKET_TYPE_BROADCAST:
            if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_BCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                 (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS)) ||
                ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                 (cntr_id == SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_PKTS))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_PKTS, value);
            } else if (((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id == SWITCH_RIF_COUNTER_ID_OUT_BCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_VLAN) &&
                        (cntr_id == SWITCH_VLAN_COUNTER_ID_OUT_BCAST_BYTES)) ||
                       ((bd_parent_type == SWITCH_OBJECT_TYPE_RIF) &&
                        (cntr_id ==
                         SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_BYTES))) {
              entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_BYTES, value);
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
      entry.second.get_arg(smi_id::P_EGRESS_BD_STATS_BYTES,
                           smi_id::A_EGRESS_BD_STATS_COUNT,
                           &ctr_bytes);
      entry.second.get_arg(smi_id::P_EGRESS_BD_STATS_PKTS,
                           smi_id::A_EGRESS_BD_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_BD_STATS_ATTR_MAU_STATS_CACHE);
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
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_EGRESS_BD_STATS_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_bd_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_BYTES, ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::P_EGRESS_BD_STATS_PKTS, ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_bd_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class rif_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_RIF_STATS;
  static const switch_attr_id_t status_attr_id = SWITCH_RIF_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_RIF_STATS_ATTR_PARENT_HANDLE;
  switch_object_id_t bd_handle = {};
  bool get_stats = true;

 public:
  rif_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = get_bd_for_object(parent, bd_handle);
    if (bd_handle == 0) get_stats = false;
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (get_stats == false) return status;

    ingress_bd_stats ing_stats(bd_handle, status);
    ing_stats.counters_get(bd_handle, cntrs);
    egress_bd_stats eg_stats(bd_handle, status);
    eg_stats.counters_get(bd_handle, cntrs);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (get_stats == false) return status;

    ingress_bd_stats ing_stats(bd_handle, status);
    ing_stats.counters_set(bd_handle);
    egress_bd_stats eg_stats(bd_handle, status);
    eg_stats.counters_set(bd_handle);
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (get_stats == false) return status;

    ingress_bd_stats ing_stats(bd_handle, status);
    ing_stats.counters_set(bd_handle, cntr_ids);
    egress_bd_stats eg_stats(bd_handle, status);
    eg_stats.counters_set(bd_handle, cntr_ids);
    return status;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (get_stats == false) return status;

    ingress_bd_stats ing_stats(bd_handle, status);
    ing_stats.counters_save(bd_handle);
    egress_bd_stats eg_stats(bd_handle, status);
    eg_stats.counters_save(bd_handle);
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (get_stats == false) return status;

    ingress_bd_stats ing_stats(bd_handle, status);
    ing_stats.counters_restore(bd_handle);
    egress_bd_stats eg_stats(bd_handle, status);
    eg_stats.counters_restore(bd_handle);
    return status;
  }
};

static bf_rt_field_id_t LEARN_BD = 0;
static bf_rt_field_id_t LEARN_PORT_LAG_INDEX = 0;
static bf_rt_field_id_t LEARN_SRC_ADDR = 0;

bf_status_t bf_rt_mac_learn_notify_cb(
    const bf_rt_target_t &bf_rt_tgt,
    const std::shared_ptr<BfRtSession> session,
    std::vector<std::unique_ptr<BfRtLearnData>> data,
    bf_rt_learn_msg_hdl *const learn_msg_hdl,
    const void *client_data) {
  (void)client_data;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_status_t bf_status = BF_SUCCESS;
  switch_mac_event_data_t mac_data;

  if (!bfrtlearn) return SWITCH_STATUS_FAILURE;

  std::lock_guard<std::mutex> lk_fdb{
      switch_store::smiContext::context().fdb_lock};

  digest_context = true;

  for (auto const &digest : data) {
    uint64_t bd = 0, port_lag_index = 0;
    switch_mac_addr_t mac = {0};
    switch_object_id_t device_handle = {0};
    switch_object_id_t port_lag_handle = {0};
    switch_object_id_t vlan_handle = {0};
    switch_object_id_t mac_entry_handle = {0};
    bool program_smac = true;
    uint16_t dev_id = bf_rt_tgt.dev_id;
    switch_mac_payload_t payload = {};

    digest->getValue(LEARN_BD, &bd);
    digest->getValue(LEARN_PORT_LAG_INDEX, &port_lag_index);
    digest->getValue(LEARN_SRC_ADDR, ETH_LEN, (unsigned char *)mac.mac);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_MAC_ENTRY,
               "Learn: bd {} port_lag_index {} mac {}",
               bd,
               port_lag_index,
               mac);

    // Ignoring dummy learn digests
    if (bd == 0 && port_lag_index == 0) continue;

    std::set<attr_w> attrs_dev;
    attrs_dev.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_DEVICE, attrs_dev, device_handle);

    // use to get vlan handle since bd == vlan_id
    std::set<attr_w> attrs_vlan;
    attrs_vlan.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device_handle));
    attrs_vlan.insert(
        attr_w(SWITCH_VLAN_ATTR_VLAN_ID, static_cast<uint16_t>(bd)));
    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_VLAN, attrs_vlan, vlan_handle);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_MAC_ENTRY,
          "{}:{}: Failed to get vlan: vlan_id {} port_lag_index {} mac {}",
          __func__,
          __LINE__,
          bd,
          port_lag_index,
          mac);
      goto send_ack;
    }

    std::set<attr_w> attrs_key;
    attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_DEVICE, device_handle));
    attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac));
    attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle));
    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_MAC_ENTRY, attrs_key, mac_entry_handle);

    // Ignoring learn digests for current vlan if max learn attribute more
    // or equal than count mac entries in fdb table for this vlan.
    // if max learn equals 0 therefore learning turn on for this vlan
    if (mac_entry_handle == 0) {
      uint32_t max_learned = 0;
      switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_MAX_LEARNED_ADDRESSES, max_learned);

      if (max_learned != 0) {
        const auto &vlan_attr_ref_map = switch_store::get_object_references(
            vlan_handle, SWITCH_OBJECT_TYPE_MAC_ENTRY);
        if (vlan_attr_ref_map.size() >= max_learned) {
          continue;
        }
      }
    }

    // convert port_lag_index to port_lag_handle
    if (port_lag_index >> PORT_LAG_INDEX_WIDTH == 1) {
      port_lag_handle = switch_store::id_to_handle(
          SWITCH_OBJECT_TYPE_LAG, port_lag_index ^ (1 << PORT_LAG_INDEX_WIDTH));
    } else {
      port_lag_handle =
          switch_store::id_to_handle(SWITCH_OBJECT_TYPE_PORT, port_lag_index);
    }
    if (port_lag_handle.data == 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}:{}: Invalid learn port_lag_index: vlan_id {} "
                 "port_lag_index {} mac {}",
                 __func__,
                 __LINE__,
                 bd,
                 port_lag_index,
                 mac);
      goto send_ack;
    }

    /**
     * Do not program the mac table if program smac flag is not set. Notify
     * the learn to application through callback.
     * TODO: learn notify callback
     */
    status = switch_store::v_get(
        device_handle, SWITCH_DEVICE_ATTR_PROGRAM_SMAC, program_smac);
    if (!program_smac) continue;

    /**
     * search the switch store for the (vlan,mac). If the mac handle returned
     * is 0, create one since it does not exist in store. Update the
     * entry otherwise.
     */
    if (mac_entry_handle == 0 ||
        !switch_store::object_try_lock(mac_entry_handle)) {
      /* The mac is new */
      switch_enum_t e = {0};
      e.enumdata = static_cast<uint64_t>(SWITCH_MAC_ENTRY_ATTR_TYPE_DYNAMIC);
      attrs_key.insert(
          attr_w(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, port_lag_handle));
      attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_TYPE, e));

      const switch_object_type_t mac_entry_ot = SWITCH_OBJECT_TYPE_MAC_ENTRY;
      status = switch_store::object_create(
          mac_entry_ot, attrs_key, mac_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_MAC_ENTRY,
                   "{}:{}: failed to add dynamic MAC status={} bd {} "
                   "port_lag_index {} mac {} l2 session {}, shared session {}",
                   __func__,
                   __LINE__,
                   status,
                   bd,
                   port_lag_index,
                   mac,
                   SWITCH_CONTEXT.get_l2_session_ptr()->sessHandleGet(),
                   get_bf_rt_session_ptr()->sessHandleGet());
        goto send_ack;
      }
      payload.mac_event = SWITCH_MAC_EVENT_LEARN;
      payload.mac_handle = mac_entry_handle;
      payload.port_lag_handle = port_lag_handle;
    } else {
      /* The mac entry exists, now it's locked and is going to be moved */
      switch_enum_t e = {0};
      status =
          switch_store::v_get(mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_TYPE, e);
      if (e.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC) {
        bool allow_move = false;
        status = switch_store::v_get(
            mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_ALLOW_MOVE, allow_move);
        if (!allow_move) {
          switch_store::object_unlock(mac_entry_handle);
          switch_log(SWITCH_API_LEVEL_DEBUG,
                     SWITCH_OBJECT_TYPE_MAC_ENTRY,
                     "Mac-learn cant move the static mac to port_lag_idx {}, "
                     "bd {}  mac {}",
                     port_lag_index,
                     bd,
                     mac);
          goto send_ack;
        } else {
          // Change type to dynamic so that aging applies to this entry
          switch_enum_t mac_entry_type = {
              .enumdata = SWITCH_MAC_ENTRY_ATTR_TYPE_DYNAMIC};
          status = switch_store::v_set(
              mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_TYPE, mac_entry_type);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_store::object_unlock(mac_entry_handle);
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_MAC_ENTRY,
                       "{}:{}: failed to set type to dynamic status={} bd {} "
                       "mac {} l2 session {}, shared session {}",
                       __func__,
                       __LINE__,
                       status,
                       bd,
                       mac,
                       SWITCH_CONTEXT.get_l2_session_ptr()->sessHandleGet(),
                       get_bf_rt_session_ptr()->sessHandleGet());
            goto send_ack;
          }
        }
      }

      status = switch_store::v_set(mac_entry_handle,
                                   SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                   port_lag_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_store::object_unlock(mac_entry_handle);
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_MAC_ENTRY,
                   "{}:{}: failed to set port_lag_handle status={} bd {} "
                   "port_lag_index {} mac {} l2 session {}, shared session {}",
                   __func__,
                   __LINE__,
                   status,
                   bd,
                   port_lag_index,
                   mac,
                   SWITCH_CONTEXT.get_l2_session_ptr()->sessHandleGet(),
                   get_bf_rt_session_ptr()->sessHandleGet());
        goto send_ack;
      }
      if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
        // clear dest_ip in case mac is moving from remote vtep
        switch_ip_address_t zero_ip_addr = {};
        memset(&zero_ip_addr, 0x00, sizeof(switch_ip_address_t));
        zero_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_NONE;
        status = switch_store::v_set(
            mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_DEST_IP, zero_ip_addr);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_store::object_unlock(mac_entry_handle);
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MAC_ENTRY,
                     "{}:{}: failed to clear dest ip status={} bd {} "
                     "mac {} l2 session {}, shared session {}",
                     __func__,
                     __LINE__,
                     status,
                     bd,
                     mac,
                     SWITCH_CONTEXT.get_l2_session_ptr()->sessHandleGet(),
                     get_bf_rt_session_ptr()->sessHandleGet());
          goto send_ack;
        }
      }
      payload.mac_event = SWITCH_MAC_EVENT_MOVE;
      payload.mac_handle = mac_entry_handle;
      payload.port_lag_handle = port_lag_handle;
    }
    mac_data.payload.push_back(payload);
  }

  smi::event::mac_event_notify(mac_data);

/**
 * TODO:
 * Sequence to follow
 * 1) add the mac entries to create and update vectors
 * 2) enable batching and program the mac entries
 * 3) flush entries to hardware
 * 4) acknowledge driver
 */

/** Send Ack to driver */
send_ack:
  /* unlock moved entries */
  for (auto m : mac_data.payload) {
    if (m.mac_event == SWITCH_MAC_EVENT_MOVE) {
      switch_store::object_unlock(m.mac_handle);
    }
  }
  digest_context = false;
  bfrtlearn->bfRtLearnNotifyAck(session, learn_msg_hdl);

  return bf_status;
}

switch_status_t mac_learn_callback_register(switch_object_id_t device_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_status_t bf_status = BF_SUCCESS;
  const bf_rt_target_t dev_tgt = get_dev_tgt();

  ENTER();
  (void)device_handle;

  if (!bfrtlearn) return SWITCH_STATUS_FAILURE;

  bf_status =
      bfrtlearn->bfRtLearnCallbackRegister(SWITCH_CONTEXT.get_l2_session_ptr(),
                                           dev_tgt,
                                           bf_rt_mac_learn_notify_cb,
                                           NULL);
  if (bf_status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: failed to register mac learn callback status={}",
               __func__,
               bf_status);
    status = SWITCH_STATUS_FAILURE;
  }

  EXIT();

  return status;
}

bf_status_t bf_rt_mac_aging_nofity_cb(const bf_rt_target_t &dev_tgt,
                                      const BfRtTableKey *match_spec,
                                      void *client_data) {
  (void)client_data;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t bd = 0;
  switch_mac_addr_t mac = {0};
  switch_object_id_t device_handle = {0};
  switch_object_id_t mac_entry_handle = {0};
  switch_object_id_t vlan_handle = {0};
  uint16_t dev_id = dev_tgt.dev_id;

  std::lock_guard<std::mutex> lk_fdb{
      switch_store::smiContext::context().fdb_lock};

  match_spec->getValue(smi_id::F_SMAC_LOCAL_MD_BD, &bd);
  match_spec->getValue(
      smi_id::F_SMAC_SRC_ADDR, ETH_LEN, (unsigned char *)mac.mac);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_MAC_ENTRY,
             "Aging: bd {} mac {}",
             bd,
             mac);

  std::set<attr_w> attrs_dev;
  attrs_dev.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_DEVICE, attrs_dev, device_handle);

  /** use to get vlan handle since bd == vlan_id */
  std::set<attr_w> attrs_vlan;
  attrs_vlan.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device_handle));
  attrs_vlan.insert(
      attr_w(SWITCH_VLAN_ATTR_VLAN_ID, static_cast<uint16_t>(bd)));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_VLAN, attrs_vlan, vlan_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  std::set<attr_w> attrs_key;
  attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_DEVICE, device_handle));
  attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac));
  attrs_key.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MAC_ENTRY, attrs_key, mac_entry_handle);
  if (mac_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_MAC_ENTRY,
               "{}.{}: failed to find mac in aging callback bd {} mac {}",
               __func__,
               __LINE__,
               bd,
               mac);
    return BF_SUCCESS;
  }

  status = switch_store::v_set(
      mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_AGE_OUT, true);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_MAC_ENTRY,
               "{}.{}: failed to age out mac entry {} handle {}",
               __func__,
               __LINE__,
               mac,
               mac_entry_handle);
  }

  status = switch_store::object_delete(mac_entry_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_MAC_ENTRY,
               "{}.{}: failed to delete mac handle in aging callback bd {} mac "
               "{} handle {}",
               __func__,
               __LINE__,
               bd,
               mac,
               mac_entry_handle);
  }
  return BF_SUCCESS;
}

switch_status_t before_mac_entry_create(const switch_object_type_t object_type,
                                        std::set<attr_w> &attrs) {
  uint32_t max_learned = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t vlan_handle = {};
  const auto attr_dev = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE));
  status = (*attr_dev).v_get(vlan_handle);

  switch_store::v_get(
      vlan_handle, SWITCH_VLAN_ATTR_MAX_LEARNED_ADDRESSES, max_learned);

  if (max_learned != 0) {
    const auto &vlan_attr_ref_map =
        switch_store::get_object_references(vlan_handle, object_type);
    if (vlan_attr_ref_map.size() >= max_learned) {
      status = SWITCH_STATUS_NO_MEMORY;
    }
  }

  return status;
}

switch_status_t after_port_bd_to_vlan_entry_update(
    const switch_object_id_t object_id, const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bd_member_handle = {};
  switch_object_type_t bd_member_parent_type, bd_member_type;
  switch_object_id_t bd_handle = {0}, handle = {0}, vlan_member_handle = {0},
                     bd_member_parent_handle;
  bool bf_rt_status_local = false;
  (void)attr;

  /**
   * When add port into vlan_member as untagged mode, SDE add one entry into
   * port_bd_to_vlan_mapping as untag action and another entry added into
   * bd_to_vlan_mapping as tag action.
   * When set vlan member tag mode by CLI command "set vlan_member handle 1
   * attribute tagging_mode TAGGED",this callback function delete existed entry
   * from port_bd_to_vlan_mapping
   * P4 table, keep the entry in bd_to_vlan_mapping and make vlan_member untag
   * mode works.
   */

  /*check whether related port_bd_to_vlan_mapping entry is inserted or not*/
  status |= switch_store::v_get(object_id,
                                SWITCH_PORT_BD_TO_VLAN_MAPPING_ATTR_STATUS,
                                bf_rt_status_local);
  if (!bf_rt_status_local) {
    return status;
  }

  /*get bd_member object by current port_bd_to_vlan object*/
  status |= get_bd_member_from_port_bd_to_vlan(object_id, bd_member_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING,
               "{}.{}: failed to get bd_member object {} from "
               "port_bd_to_vlan_mapping handle {}",
               __func__,
               __LINE__,
               object_id,
               bd_member_handle);
  }
  /*get bd object from bd_member*/
  status |= get_bd_from_bd_member(bd_member_handle, bd_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING,
        "{}.{}: failed to get bd object {} from bd_member_handle handle {}",
        __func__,
        __LINE__,
        bd_member_handle,
        bd_handle);
  }
  /*check bd_member type,only handle port/lag type*/
  status |= switch_store::v_get(
      bd_member_handle, SWITCH_BD_MEMBER_ATTR_HANDLE, handle.data);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING,
        "{}.{}: failed to get bd object {} from bd_member_handle handle {}",
        __func__,
        __LINE__);
  }
  bd_member_type = switch_store::object_type_query(handle);
  if (bd_member_type != SWITCH_OBJECT_TYPE_PORT &&
      bd_member_type != SWITCH_OBJECT_TYPE_LAG) {
    // clear_attrs();
    return status;
  }
  /*check whether parent object is vlan_member*/
  status |= switch_store::v_get(bd_member_handle,
                                SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE,
                                bd_member_parent_handle);
  bd_member_parent_type =
      switch_store::object_type_query(bd_member_parent_handle);
  if (bd_member_parent_type == SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
    status |= switch_store::v_get(bd_member_handle,
                                  SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE,
                                  vlan_member_handle);
  } else {
    return status;
  }

  /*instantiate a port_bd_vlan_mapping object and delete related p4 entry*/
  port_bd_to_vlan_mapping p_bd_to_vlan(bd_member_handle, status);
  if (switch_store::object_type_query(vlan_member_handle) ==
      SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
    switch_enum_t tag_mode = {0};
    status |= switch_store::v_get(
        vlan_member_handle, SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE, tag_mode);
    if (tag_mode.enumdata == SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED) {
      status |= p_bd_to_vlan.p4_object_match_action::del_pi_only();
      if (status == SWITCH_STATUS_SUCCESS) {
        status |= switch_store::v_set(
            object_id, SWITCH_PORT_BD_TO_VLAN_MAPPING_ATTR_STATUS, false);
      }
    }
  }
  return status;
}

switch_status_t mac_aging_callback_register(switch_object_id_t device_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_status_t rc = BF_SUCCESS;
  const BfRtInfo *bfrtinfo = get_bf_rt_info();
  const bf_rt_target_t dev_tgt = get_dev_tgt();
  const BfRtTable *table = NULL;
  ENTER();

  uint32_t query_interval = 0;
  uint32_t default_aging_time = 0;
  uint32_t max_aging_time = 0;

  std::unique_ptr<BfRtTableAttributes> table_attributes;
  rc = bfrtinfo->bfrtTableFromIdGet(smi_id::T_SMAC, &table);
  if (!table) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  rc = table->attributeAllocate(TableAttributesType::IDLE_TABLE_RUNTIME,
                                TableAttributesIdleTableMode::NOTIFY_MODE,
                                &table_attributes);
  if (!table_attributes) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  status |= switch_store::v_get(
      device_handle, SWITCH_DEVICE_ATTR_QUERY_INTERVAL, query_interval);
  status |= switch_store::v_get(device_handle,
                                SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL,
                                default_aging_time);
  status |= switch_store::v_get(
      device_handle, SWITCH_DEVICE_ATTR_MAX_AGING_TIME, max_aging_time);

  // FIXME(bfn): compute better value
  if (!default_aging_time) default_aging_time = 5000;
  rc = table_attributes->idleTableNotifyModeSet(true,
                                                bf_rt_mac_aging_nofity_cb,
                                                query_interval,
                                                max_aging_time,
                                                default_aging_time,
                                                NULL);
  if (rc != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register mac aging callback status={}",
               __func__,
               __LINE__,
               rc);
    return SWITCH_STATUS_FAILURE;
  }

  rc = table->tableAttributesSet(
      SWITCH_CONTEXT.get_l2_session(), dev_tgt, *table_attributes);
  if (rc != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to set table attributes",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  EXIT();
  return status;
}

bool skip_auto_object_for_mac_entry(const switch_object_id_t object_id,
                                    const switch_object_type_t auto_ot) {
  bool skip = false;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t ot;
  switch_object_id_t dev_hdl = {};

  if (object_id.data == 0) return false;
  if (switch_store::is_object_type_valid(auto_ot) == false) return false;
  ot = switch_store::object_type_query(object_id);

  /* For parent object - SWITCH_OBJECT_TYPE_MAC_ENTRY
      With global/local nexthop_resolution feature set to false: [SAL]
        Skip auto object:
          - nexthop_resolution

      With global/local nexthop_resolution feature set to true: [SAI]
        Allow auto object:
          - nexthop_resolution
  */
  if (ot == SWITCH_OBJECT_TYPE_MAC_ENTRY) {
    bool resolve_nexthop = true;

    // Alternate option is to have the per object feature control i.e. in mac
    // object like SWITCH_MAC_ENTRY_ATTR_NEXTHOP_RESOLUTION
    // currently checking the global flag for feature [nexthop resolution]
    status |=
        switch_store::v_get(object_id, SWITCH_MAC_ENTRY_ATTR_DEVICE, dev_hdl);
    resolve_nexthop = is_nexthop_resolution_feature_on(dev_hdl);

    switch (auto_ot) {
      case SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION:
        if (resolve_nexthop == false) {
          skip = true;
        }
        break;

      default:
        break;
    }

    if (skip) {
      switch_log(
          SWITCH_API_LEVEL_DEBUG,
          ot,
          "{}:{}: Skip auto object {} for parent type {} hdl {}, nexthop "
          "resolution feature enabled - {}",
          __func__,
          __LINE__,
          switch_store::object_name_get_from_type(auto_ot),
          switch_store::object_name_get_from_type(ot),
          object_id,
          resolve_nexthop);
    }
  }
  return skip;
}

switch_status_t l2_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  const BfRtInfo *bfrtinfo = get_bf_rt_info();
  if (!bfrtinfo) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_init: failed to get bf-runtime info");
    return SWITCH_STATUS_FAILURE;
  }

  bfrtinfo->bfrtLearnFromNameGet("learning_digest", &bfrtlearn);
  if (!bfrtlearn) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_init: failed to get bf-runtime learn object");
    return SWITCH_STATUS_FAILURE;
  }

  bfrtlearn->learnFieldIdGet("bd", &LEARN_BD);
  bfrtlearn->learnFieldIdGet("port_lag_index", &LEARN_PORT_LAG_INDEX);
  bfrtlearn->learnFieldIdGet("src_addr", &LEARN_SRC_ADDR);

  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_MAC_ENTRY,
                                                  &before_mac_entry_create);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING,
      &after_port_bd_to_vlan_entry_update);

  REGISTER_OBJECT(stp_factory, SWITCH_OBJECT_TYPE_STP_FACTORY);
  REGISTER_OBJECT(smac, SWITCH_OBJECT_TYPE_SMAC);
  REGISTER_OBJECT(dmac, SWITCH_OBJECT_TYPE_DMAC);
  REGISTER_OBJECT(egress_bd_mapping, SWITCH_OBJECT_TYPE_EGRESS_BD_MAPPING);
  REGISTER_OBJECT(vlan_decap, SWITCH_OBJECT_TYPE_VLAN_DECAP);
  REGISTER_OBJECT(port_bd_to_vlan_mapping,
                  SWITCH_OBJECT_TYPE_PORT_BD_TO_VLAN_MAPPING);
  REGISTER_OBJECT(bd_to_vlan_mapping, SWITCH_OBJECT_TYPE_BD_TO_VLAN_MAPPING);
  REGISTER_OBJECT(ingress_bd_stats, SWITCH_OBJECT_TYPE_INGRESS_BD_STATS);
  REGISTER_OBJECT(egress_bd_stats, SWITCH_OBJECT_TYPE_EGRESS_BD_STATS);
  REGISTER_OBJECT(rif_stats, SWITCH_OBJECT_TYPE_RIF_STATS);

  status |= switch_store::reg_skip_auto_object_trigs(
      SWITCH_OBJECT_TYPE_MAC_ENTRY, &skip_auto_object_for_mac_entry);

  return status;
}

switch_status_t l2_clean() {
  bf_status_t bf_status = BF_SUCCESS;
  const BfRtTable *table = NULL;
  uint64_t val = 0;

  const BfRtInfo *bfrtinfo = get_bf_rt_info();
  if (!bfrtinfo) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_clean: failed to get bf-runtime info");
    return SWITCH_STATUS_FAILURE;
  }

  bfrtinfo->bfrtLearnFromNameGet("learning_digest", &bfrtlearn);
  if (!bfrtlearn) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_clean: failed to get bf-runtime learn object");
    return SWITCH_STATUS_FAILURE;
  }

  bf_status = bfrtlearn->bfRtLearnCallbackDeregister(
      SWITCH_CONTEXT.get_l2_session_ptr(), get_dev_tgt());
  if (bf_status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_clean: failed to deregister mac learn callback status={}",
               bf_status);
    return SWITCH_STATUS_FAILURE;
  }

  std::unique_ptr<BfRtTableAttributes> table_attributes;
  bf_status = bfrtinfo->bfrtTableFromIdGet(smi_id::T_SMAC, &table);
  if (!table) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  bf_status =
      table->attributeAllocate(TableAttributesType::IDLE_TABLE_RUNTIME,
                               TableAttributesIdleTableMode::NOTIFY_MODE,
                               &table_attributes);
  if (!table_attributes) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  bf_status = table_attributes->idleTableNotifyModeSet(
      false, NULL, val, val, val, NULL);
  if (bf_status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to deregister mac aging callback status={}",
               __func__,
               __LINE__,
               bf_status);
    return SWITCH_STATUS_FAILURE;
  }

  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
