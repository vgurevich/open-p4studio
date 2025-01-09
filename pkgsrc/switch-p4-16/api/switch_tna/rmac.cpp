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


#include <memory>
#include <utility>
#include <set>
#include <vector>

#include "switch_tna/utils.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

// tunnel_mapper_entry is parent
class vxlan_rmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_VXLAN_RMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VXLAN_RMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_VXLAN_RMAC_ATTR_STATUS;

 public:
  vxlan_rmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle = {};
    switch_enum_t type = {0};

    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE)
      return;

    status |= match_key.set_exact(smi_id::F_TUNNEL_RMAC_LOCAL_MD_TUNNEL_VNI,
                                  parent,
                                  SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, vrf_handle);
    status |=
        match_key.set_exact(smi_id::F_TUNNEL_RMAC_HDR_INNER_ETHERNET_DST_ADDR,
                            vrf_handle,
                            SWITCH_VRF_ATTR_SRC_MAC);

    action_entry.init_action_data(smi_id::A_TUNNEL_RMAC_HIT);
  }
};

// rif of type PORT/SUB_PORT is parent
// for port type, (port, *, dmac) -> rmac_hit
// for sub-port, (port, vlan, dmac) -> rmac_hit
class ingress_pv_rmac_for_rif : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_RIF;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_RIF_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_RIF_ATTR_STATUS;

 public:
  ingress_pv_rmac_for_rif(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_PV_RMAC,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_enum_t rif_type = {};
    switch_object_id_t port_handle = {0};
    switch_mac_addr_t rmac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t peer_src_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t src_mac_rif_update = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t rmac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    std::vector<switch_mac_addr_t> rmacs;

    status = switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);
    status = switch_store::v_get(parent, SWITCH_RIF_ATTR_SRC_MAC, rmac);
    rmacs.push_back(rmac);

    status =
        switch_store::v_get(parent, SWITCH_RIF_ATTR_PEER_SRC_MAC, peer_src_mac);
    if (SWITCH_MAC_VALID(peer_src_mac)) {
      if (std::find(rmacs.begin(), rmacs.end(), peer_src_mac) == rmacs.end()) {
        rmacs.push_back(peer_src_mac);
      }
    }

    status = switch_store::v_get(
        parent, SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, src_mac_rif_update);
    if (SWITCH_MAC_VALID(src_mac_rif_update)) {
      if (std::find(rmacs.begin(), rmacs.end(), src_mac_rif_update) ==
          rmacs.end()) {
        rmacs.push_back(src_mac_rif_update);
      }
    }

    if (rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_PORT &&
        rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_SUB_PORT)
      return;

    auto it = match_action_list.begin();

    status |=
        switch_store::v_get(parent, SWITCH_RIF_ATTR_PORT_HANDLE, port_handle);
    uint16_t port_lag_index = compute_port_lag_index(port_handle);

    for (auto mac : rmacs) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INGRESS_PV_RMAC),
              _ActionEntry(smi_id::T_INGRESS_PV_RMAC)));
      status |= it->first.set_ternary(
          smi_id::F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          port_lag_index,
          static_cast<uint16_t>(0x3FF));
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= it->first.set_ternary(
            smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID, true, true);
        uint16_t outer_vlan_id = 0;
        status |= switch_store::v_get(
            parent, SWITCH_RIF_ATTR_OUTER_VLAN_ID, outer_vlan_id);
        status |=
            it->first.set_ternary(smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID,
                                  outer_vlan_id,
                                  static_cast<uint16_t>(0xFFF));
      }
      status |= it->first.set_ternary(
          smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR, mac, rmac_mask);
      it->second.init_action_data(smi_id::A_RMAC_HIT);
    }
  }
};

// rif of type VLAN is parent
class ingress_vlan_rmac : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_VLAN_RMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_VLAN_RMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_VLAN_RMAC_ATTR_STATUS;
  switch_object_id_t vlan_handle = {0};

 public:
  ingress_vlan_rmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_VLAN_RMAC,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_mac_addr_t src_mac_rif_update = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t peer_src_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t rmac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    std::vector<switch_mac_addr_t> rmacs;
    switch_enum_t rif_type = {};
    status = switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);

    if (rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_VLAN) {
      clear_attrs();
      return;
    }

    status = switch_store::v_get(parent, SWITCH_RIF_ATTR_SRC_MAC, rmac);
    rmacs.push_back(rmac);

    status =
        switch_store::v_get(parent, SWITCH_RIF_ATTR_PEER_SRC_MAC, peer_src_mac);
    if (SWITCH_MAC_VALID(peer_src_mac)) {
      if (std::find(rmacs.begin(), rmacs.end(), peer_src_mac) == rmacs.end()) {
        rmacs.push_back(peer_src_mac);
      }
    }

    status = switch_store::v_get(
        parent, SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, src_mac_rif_update);
    if (SWITCH_MAC_VALID(src_mac_rif_update)) {
      if (std::find(rmacs.begin(), rmacs.end(), src_mac_rif_update) ==
          rmacs.end()) {
        rmacs.push_back(src_mac_rif_update);
      }
    }

    auto it = match_action_list.begin();

    status |=
        switch_store::v_get(parent, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);

    for (auto mac : rmacs) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INGRESS_VLAN_RMAC),
              _ActionEntry(smi_id::T_INGRESS_VLAN_RMAC)));
      status |=
          it->first.set_exact(smi_id::F_INGRESS_VLAN_RMAC_HDR_VLAN_TAG0_VID,
                              vlan_handle,
                              SWITCH_VLAN_ATTR_VLAN_ID);
      status |= it->first.set_exact(
          smi_id::F_INGRESS_VLAN_RMAC_HDR_ETHERNET_DST_ADDR, mac);

      it->second.init_action_data(smi_id::A_RMAC_HIT);
    }
  }
};

// my_mac is parent
class ingress_pv_rmac_for_my_mac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_MY_MAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_MY_MAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_MY_MAC_ATTR_STATUS;

 public:
  ingress_pv_rmac_for_my_mac(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PV_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_lag_handle = {0};
    uint16_t vlan_id = 0, port_lag_index = 0;
    switch_mac_addr_t my_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t my_mac_mask = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    status |= switch_store::v_get(
        parent, SWITCH_MY_MAC_ATTR_PORT_LAG_INDEX, port_lag_handle);
    status |=
        switch_store::v_get(parent, SWITCH_MY_MAC_ATTR_MAC_ADDRESS, my_mac);
    status |= switch_store::v_get(
        parent, SWITCH_MY_MAC_ATTR_MAC_ADDRESS_MASK, my_mac_mask);
    status |= switch_store::v_get(parent, SWITCH_MY_MAC_ATTR_VLAN_ID, vlan_id);

    if (port_lag_handle != 0) {
      port_lag_index = compute_port_lag_index(port_lag_handle);

      status |= match_key.set_ternary(
          smi_id::F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          port_lag_index,
          static_cast<uint16_t>(0x3FF));
    }
    if (vlan_id != 0) {
      status |= match_key.set_ternary(
          smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID, true, true);

      status |=
          match_key.set_ternary(smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID,
                                vlan_id,
                                static_cast<uint16_t>(0xFFF));
    }
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR, my_mac, my_mac_mask);

    action_entry.init_action_data(smi_id::A_RMAC_HIT);
  }
};

// port is parent
// re-evaluated whenever port_vlan_id attribute is set
// Entry uses the rmac of the SVI for the port_vlan_id
// If SVI absent, (port, *, *) -> rmac_miss
// iF SVI present, (port, *, dmac) -> rmac_hit
class ingress_pv_rmac_for_port : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_PORT_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PV_RMAC_FOR_PORT_ATTR_STATUS;

 public:
  ingress_pv_rmac_for_port(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_PV_RMAC,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_type_t ot = switch_store::object_type_query(parent);
    switch_object_id_t device_handle = {};
    uint16_t port_vlan_id = 0;
    bf_rt_action_id_t rmac_action_id = smi_id::A_RMAC_MISS;
    uint16_t port_lag_index = compute_port_lag_index(parent);
    // this is lower priority compared to the RIF entry
    uint32_t prio = 10;
    switch_mac_addr_t src_mac_rif_update = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t peer_src_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t rmac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    std::vector<switch_mac_addr_t> rmacs;
    switch_mac_addr_t rmac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      status |= switch_store::v_get(
          parent, SWITCH_PORT_ATTR_PORT_VLAN_ID, port_vlan_id);
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    } else {
      status |= switch_store::v_get(
          parent, SWITCH_LAG_ATTR_PORT_VLAN_ID, port_vlan_id);
      status |=
          switch_store::v_get(parent, SWITCH_LAG_ATTR_DEVICE, device_handle);
    }

    if (port_vlan_id != 0) {
      // find an SVI for this vlan ID
      switch_object_id_t vlan_handle = {};
      std::vector<switch_object_id_t> rif_handles;
      std::set<attr_w> vlan_attrs;
      bool is_vlan_deletes = false;
      vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device_handle));
      vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, port_vlan_id));
      status = switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_VLAN, vlan_attrs, vlan_handle);
      if (vlan_handle.data != 0) {
        switch_object_id_t bd_handle = {0};
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        if (bd_handle.data != 0) {
          status |= switch_store::v_get(
              bd_handle, SWITCH_BD_ATTR_IS_DELETING, is_vlan_deletes);
        }
      }
      if (status == SWITCH_STATUS_SUCCESS && !is_vlan_deletes) {
        status |= switch_store::v_get(
            vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
        for (const auto &rif_handle : rif_handles) {
          status |=
              switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_SRC_MAC, rmac);
          rmacs.push_back(rmac);
          rmac_action_id = smi_id::A_RMAC_HIT;

          status = switch_store::v_get(
              rif_handle, SWITCH_RIF_ATTR_PEER_SRC_MAC, peer_src_mac);
          if (SWITCH_MAC_VALID(peer_src_mac)) {
            if (std::find(rmacs.begin(), rmacs.end(), peer_src_mac) ==
                rmacs.end()) {
              rmacs.push_back(peer_src_mac);
            }
          }
          status = switch_store::v_get(rif_handle,
                                       SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE,
                                       src_mac_rif_update);
          if (SWITCH_MAC_VALID(src_mac_rif_update)) {
            if (std::find(rmacs.begin(), rmacs.end(), src_mac_rif_update) ==
                rmacs.end()) {
              rmacs.push_back(src_mac_rif_update);
            }
          }
        }
      } else {
        rmacs.push_back(rmac);
        status = SWITCH_STATUS_SUCCESS;
      }
    }

    typedef struct entries_ {
      uint16_t vlan_id;
      uint16_t vlan_id_mask;
      uint8_t valid;
      uint8_t valid_mask;
    } entries;

    std::vector<entries> rules = {
        {0, 0, 0, 1},     // port, *, 0
        {0, 0xFFF, 1, 1}  // port, 0, valid
    };

    auto it = match_action_list.begin();
    for (const auto entry : rules) {
      for (const auto mac : rmacs) {
        it = match_action_list.insert(
            it,
            std::pair<_MatchKey, _ActionEntry>(
                _MatchKey(smi_id::T_INGRESS_PV_RMAC),
                _ActionEntry(smi_id::T_INGRESS_PV_RMAC)));
        status |= it->first.set_ternary(
            smi_id::F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
            port_lag_index,
            static_cast<uint16_t>(0x3FF));
        status |=
            it->first.set_exact(smi_id::F_INGRESS_PV_RMAC_MATCH_PRIORITY, prio);

        status |= it->first.set_ternary(
            smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR, mac, rmac_mask);

        status |=
            it->first.set_ternary(smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID,
                                  entry.vlan_id,
                                  entry.vlan_id_mask);
        status |=
            it->first.set_ternary(smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID,
                                  entry.valid,
                                  entry.valid_mask);
        it->second.init_action_data(rmac_action_id);
      }
    }
  }
};

switch_status_t before_rif_update2(const switch_object_id_t handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t vlan_handle = {};
  switch_enum_t rif_type = {};

  status = switch_store::v_get(handle, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC ||
      attr.id_get() == SWITCH_RIF_ATTR_PEER_SRC_MAC ||
      attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE) {
    ingress_pv_rmac_for_rif pv_rmac(handle, status);
    pv_rmac.del();
    ingress_vlan_rmac vlan_rmac(handle, status);
    vlan_rmac.del();

    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      status |=
          switch_store::v_get(handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      for (const auto &phandle : get_untagged_vlan_member_ports(vlan_handle)) {
        ingress_pv_rmac_for_port pv_rmac_for_port(phandle, status);
        pv_rmac_for_port.del();
      }
    }
  }
  return status;
}

switch_status_t after_rif_update2(const switch_object_id_t handle,
                                  const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t vlan_handle = {};
  switch_enum_t rif_type = {};

  status = switch_store::v_get(handle, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC ||
      attr.id_get() == SWITCH_RIF_ATTR_PEER_SRC_MAC ||
      attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE) {
    ingress_pv_rmac_for_rif pv_rmac(handle, status);
    pv_rmac.create_update();
    ingress_vlan_rmac vlan_rmac(handle, status);
    vlan_rmac.create_update();

    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      status |=
          switch_store::v_get(handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      for (const auto &phandle : get_untagged_vlan_member_ports(vlan_handle)) {
        ingress_pv_rmac_for_port pv_rmac_for_port(phandle, status);
        pv_rmac_for_port.create_update();
      }
    }
  }
  return status;
}

switch_status_t before_port_update3(const switch_object_id_t handle,
                                    const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (attr.id_get() == SWITCH_PORT_ATTR_PORT_VLAN_ID ||
      attr.id_get() == SWITCH_LAG_ATTR_PORT_VLAN_ID) {
    ingress_pv_rmac_for_port pv_rmac(handle, status);
    pv_rmac.del();
  }
  return status;
}

switch_status_t after_port_update3(const switch_object_id_t handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (attr.id_get() == SWITCH_PORT_ATTR_PORT_VLAN_ID ||
      attr.id_get() == SWITCH_LAG_ATTR_PORT_VLAN_ID) {
    ingress_pv_rmac_for_port pv_rmac(handle, status);
    pv_rmac.create_update();
  }
  return status;
}

switch_status_t rmac_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_RIF,
                                                  &before_rif_update2);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_RIF,
                                                 &after_rif_update2);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_PORT,
                                                  &before_port_update3);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_PORT,
                                                 &after_port_update3);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_LAG,
                                                  &before_port_update3);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_LAG,
                                                 &after_port_update3);

  REGISTER_OBJECT(vxlan_rmac, SWITCH_OBJECT_TYPE_VXLAN_RMAC);
  REGISTER_OBJECT(ingress_pv_rmac_for_rif,
                  SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_RIF);
  REGISTER_OBJECT(ingress_pv_rmac_for_port,
                  SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_PORT);
  REGISTER_OBJECT(ingress_pv_rmac_for_my_mac,
                  SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_MY_MAC);
  REGISTER_OBJECT(ingress_vlan_rmac, SWITCH_OBJECT_TYPE_INGRESS_VLAN_RMAC);

  return status;
}

switch_status_t rmac_clean() { return SWITCH_STATUS_SUCCESS; }

} /* namespace smi */
