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
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <algorithm>

#include "switch_tna/utils.h"
extern "C" {
#include "tofino/bf_pal/pltfm_intf.h"
}

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

uint32_t compute_ecmp_selector_member_id(switch_object_id_t ecmp_member,
                                         uint32_t index) {
  (void)index;
  return static_cast<uint32_t>(switch_store::handle_to_id(ecmp_member));
}

static switch_status_t resolve_neighbor(switch_object_id_t &neighbor_handle,
                                        switch_object_id_t &nexthop_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> lookup_attrs;
  switch_ip_address_t dest_ip_addr = {};
  switch_object_id_t device_handle = {}, rif_handle = {};

  if (nexthop_handle != 0) {
    /* find if a matching neighbor exists */
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_DEST_IP, dest_ip_addr);
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_DEVICE, device_handle);
    lookup_attrs.insert(attr_w(SWITCH_NEIGHBOR_ATTR_HANDLE, rif_handle));
    lookup_attrs.insert(attr_w(SWITCH_NEIGHBOR_ATTR_DEVICE, device_handle));
    lookup_attrs.insert(attr_w(SWITCH_NEIGHBOR_ATTR_DEST_IP, dest_ip_addr));
    switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_NEIGHBOR, lookup_attrs, neighbor_handle);
  }
  return status;
}

static switch_status_t resolve_nexthop(
    std::vector<switch_object_id_t> &nexthop_handle_list,
    switch_object_id_t &neighbor_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> lookup_attrs;
  switch_ip_address_t dest_ip_addr = {};
  switch_object_id_t handle = {};
  switch_object_id_t device_handle = {};
  switch_enum_t e = {};
  uint32_t vni = 0;
  switch_object_id_t sidlist_id = {};
  switch_enum_t label_op = {.enumdata = SWITCH_NEXTHOP_ATTR_LABELOP_NONE};
  std::vector<uint32_t> label_list;
  switch_object_id_t nexthop_handle = {};

  if (neighbor_handle == 0) return status;

  /* find if a matching nexthop exists */
  status |=
      switch_store::v_get(neighbor_handle, SWITCH_NEIGHBOR_ATTR_HANDLE, handle);
  if (switch_store::object_type_query(handle) == SWITCH_OBJECT_TYPE_RIF) {
    e.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_IP;
  } else if (switch_store::object_type_query(handle) ==
             SWITCH_OBJECT_TYPE_TUNNEL) {
    e.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL;
  }
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_DEST_IP, dest_ip_addr);

  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEVICE, device_handle));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TYPE, e));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEST_IP, dest_ip_addr));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_HANDLE, handle));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, vni));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_id));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_LABELOP, label_op));
  lookup_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list));

  switch_status_t tmp_status = SWITCH_STATUS_SUCCESS;
  tmp_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_NEXTHOP, lookup_attrs, nexthop_handle);
  if (tmp_status == SWITCH_STATUS_SUCCESS) {
    nexthop_handle_list.push_back(nexthop_handle);
  }

  if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
    std::set<switch_object_id_t> nh_list;
    tmp_status |= switch_store::referencing_set_get(
        handle, SWITCH_OBJECT_TYPE_NEXTHOP, nh_list);
    /* MPLS Nexthops can have common IP, RIF, but different
     * label stack and label operation. So, iterate over all
     * the nexthops with same {IP, RIF}.
     */
    for (auto nh : nh_list) {
      switch_ip_address_t dip;
      switch_enum_t nh_type;
      tmp_status |= switch_store::v_get(nh, SWITCH_NEXTHOP_ATTR_TYPE, nh_type);
      tmp_status |= switch_store::v_get(nh, SWITCH_NEXTHOP_ATTR_DEST_IP, dip);
      if ((nh_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_MPLS) &&
          (dest_ip_addr == dip)) {
        nexthop_handle_list.push_back(nh);
      }
    }
  }
  return status;
}

static switch_status_t resolve_mac_entry(switch_object_id_t &mac_entry_handle,
                                         switch_object_id_t &neighbor_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> lookup_attrs;

  switch_enum_t packet_action{SWITCH_NEIGHBOR_ATTR_PACKET_ACTION_FORWARD};
  if (neighbor_handle == 0) return status;
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_PACKET_ACTION, packet_action);

  if (packet_action.enumdata == SWITCH_NEIGHBOR_ATTR_PACKET_ACTION_TRAP)
    return status;
  switch_object_id_t rif_handle = {}, vlan_handle = {}, device_handle = {};
  switch_mac_addr_t mac_addr = {};
  switch_enum_t rif_type = {};
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_HANDLE, rif_handle);
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(
      neighbor_handle, SWITCH_NEIGHBOR_ATTR_MAC_ADDRESS, mac_addr);
  status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
    status |= switch_store::v_get(
        rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
    lookup_attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_DEVICE, device_handle));
    lookup_attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle));
    lookup_attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac_addr));
    switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_MAC_ENTRY, lookup_attrs, mac_entry_handle);
  }
  return status;
}

/** @brief ECMP selector group
 * Subscribes to ECMP user object. Sets the group ID from the object handle
 * and size of the group as data.
 */
class ecmp_selector_group : public p4_object_selector_group {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ECMP_SELECTOR_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ECMP_SELECTOR_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECMP_SELECTOR_GROUP_ATTR_PARENT_HANDLE;

 public:
  ecmp_selector_group(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_selector_group(
            smi_id::SG_ECMP_SELECTOR_GROUP,
            status_attr_id,
            smi_id::P_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE,
            smi_id::P_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY,
            smi_id::P_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY,
            auto_ot,
            parent_attr_id,
            parent) {
    status |= match_key.set_exact(smi_id::F_ECMP_SELECTOR_GROUP_ID, parent);
    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(smi_id::P_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE,
                                   parent,
                                   SWITCH_ECMP_ATTR_CONFIGURED_SIZE);
  }
};

/** @brief ECMP table
 * Auto object of above ecmp selector object
 * Adds an entry for each port. Use the port__index as the key
 * and parent action member ID from above as the action parameter.
 * For port,
 * ECMP_TABLE : port_index -> member_id
 * ECMP_SELECTOR: member_id -> dev_port
 * For lag,
 * ECMP_TABLE : lag_index -> grp_id
 * ECMP_SEL_GRP: grp_id -> member_list -> ECMP_SELECTOR::member_id
 */
class ecmp_table : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ECMP_TABLE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECMP_TABLE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_ECMP_TABLE_ATTR_STATUS;

 public:
  ecmp_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_ECMP, status_attr_id, auto_ot, parent_attr_id, parent) {
    construct_ecmp_table(parent, status, 0);
  }

  ecmp_table(const switch_object_id_t parent,
             switch_status_t &status,
             size_t mbr_size)
      : p4_object_match_action(
            smi_id::T_ECMP, status_attr_id, auto_ot, parent_attr_id, parent) {
    construct_ecmp_table(parent, status, mbr_size);
  }

  void construct_ecmp_table(const switch_object_id_t parent,
                            switch_status_t &status,
                            size_t mbr_size) {
    const auto nhop_index = compute_nexthop_index(parent);
    status |= match_key.set_exact(smi_id::F_ECMP_LOCAL_MD_NEXTHOP, nhop_index);
    status |= action_entry.init_indirect_data();
    if (mbr_size == 0) {
      status |=
          action_entry.set_arg(smi_id::D_ECMP_ACTION_MEMBER_ID,
                               static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
    } else {
      status |= action_entry.set_arg(smi_id::D_ECMP_SELECTOR_GROUP_ID, parent);
    }

    // This value must be in sync with P4 size per profile [below]
    // Temp using the macro value SELECTOR_GROUP_MAX_GROUP_SIZE
    // WIP - replace below macro with bfrt API to fetch the actual value
    switch_store::v_set(
        parent,
        SWITCH_ECMP_ATTR_REAL_SIZE,
        p4_object_selector_group::SELECTOR_GROUP_MAX_GROUP_SIZE);
  }
};

/*************************************************************************
  changes to support using same nexthop in multiple ecmp-members
  [For features like FG-ECMP]
  Logic to support this changes - use lower 32 bits of ecmp-member hdl as
  action member id while programming the ecmp-members in ecmp_action_profile
  table so that action member id will be unique
  For WCMP: This class creates a list of action profile entries as a stop
  gap until driver allows using duplicate entries in the selector group.
  For WCMP, if weight is 10, then 10 action profiles entries are created.
***************************************************************************/

/** @brief ECMP selector.
 * This is similar to the nexthop_table class
 * - Subscribes to a nexthop_resolution object if nexthop resolution feature is
 * on
 * - Subscribes to nexthop[nexthop_table] object if nexthop resolution feature
 * is off
 */
#define ECMP_SELECTOR                                                     \
  std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::AP_ECMP_SELECTOR), \
                                     _ActionEntry(smi_id::AP_ECMP_SELECTOR))
class ecmp_selector : public p4_object_action_selector_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ECMP_SELECTOR;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ECMP_SELECTOR_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECMP_SELECTOR_ATTR_PARENT_HANDLE;

 public:
  ecmp_selector(const switch_object_id_t parent,
                switch_status_t &status,
                uint32_t weight)
      : p4_object_action_selector_list(smi_id::AP_ECMP_SELECTOR,
                                       smi_id::F_ECMP_SELECTOR_ACTION_MEMBER_ID,
                                       status_attr_id,
                                       auto_ot,
                                       parent_attr_id,
                                       parent) {
    switch_object_id_t nexthop_handle = {}, nexthop_res_handle = {},
                       mac_entry_handle = {}, rif_handle = {},
                       port_lag_handle = {}, bd_handle = {}, vlan_handle = {},
                       neighbor_handle{};
    switch_enum_t e = {0};
    switch_enum_t rif_type = {};
    bool nexthop_resolution_on = true;
    switch_object_id_t dev_hdl = {0};

    status |= switch_store::v_get(
        parent, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, nexthop_handle);
    status |= resolve_neighbor(neighbor_handle, nexthop_handle);

    auto it = match_action_list.begin();
    for (uint32_t i = 1; i <= weight; i++) {
      it = match_action_list.insert(it, ECMP_SELECTOR);
      status |= it->first.set_exact(smi_id::F_ECMP_SELECTOR_ACTION_MEMBER_ID,
                                    compute_ecmp_selector_member_id(parent, i));

      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_DEVICE, dev_hdl);
      nexthop_resolution_on = is_nexthop_resolution_feature_on(dev_hdl);

      status |=
          switch_store::v_get(nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, e);
      // tunnel nexthop
      if ((e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) ||
          (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
        if (nexthop_resolution_on) {
          it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_TUNNEL);
          status |=
              it->second.set_arg(smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_DIP_INDEX,
                                 nexthop_handle,
                                 SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_INDEX);
          status |= it->second.set_arg(
              smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_NEXTHOP_INDEX,
              compute_nexthop_index(nexthop_handle));
          continue;
        } else {
          // revisit for sal tunnel
        }
      }

      if (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_DROP) {
        it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_DROP);
      } else if (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_GLEAN) {
        it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_GLEAN);
      } else {
        // non-tunnel nexthop - IP
        if (neighbor_handle.data) {
          switch_enum_t packet_action{};
          status |= switch_store::v_get(neighbor_handle,
                                        SWITCH_NEIGHBOR_ATTR_PACKET_ACTION,
                                        packet_action);
          if (packet_action.enumdata ==
              SWITCH_NEIGHBOR_ATTR_PACKET_ACTION_TRAP) {
            switch_object_id_t hostif_udt_trap_handle{};
            status |= switch_store::v_get(
                neighbor_handle,
                SWITCH_NEIGHBOR_ATTR_USER_DEFINED_TRAP_HANDLE,
                hostif_udt_trap_handle);
            if (hostif_udt_trap_handle.data) {
              it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_GLEAN);
              it->second.set_arg(smi_id::P_SET_ECMP_PROPERTIES_GLEAN_TRAP_ID,
                                 hostif_udt_trap_handle);
            } else {
              it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_DROP);
            }
            return;
          }
        }

        // If nexthop resolution feature is off, get port_lag_hdl
        // from nexthop object
        if (nexthop_resolution_on == false) {
          status |= switch_store::v_get(nexthop_handle,
                                        SWITCH_NEXTHOP_ATTR_PORT_LAG_HANDLE,
                                        port_lag_handle);
        }

        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
        status |=
            switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);

        // If L3 interface, get bd from the RIF
        if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) ||
            (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) ||
            (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT)) {
          // If nexthop resolution feature is on, get port_lag_hdl from rif
          if (nexthop_resolution_on) {
            status |= switch_store::v_get(
                rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
          }

          status |= find_auto_oid(rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
          if (bd_handle.data == 0) {
            status = SWITCH_STATUS_FAILURE;
            return;
          }
        } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
          // for SVI case
          status |= switch_store::v_get(
              rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
          status |=
              find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
          assert(bd_handle.data != 0);

          if (nexthop_resolution_on) {
            status |= find_auto_oid(nexthop_handle,
                                    SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION,
                                    nexthop_res_handle);
            status |= switch_store::v_get(
                nexthop_res_handle,
                SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                mac_entry_handle);
          }

          // If nexthop resolution feature is on & mac_entry is valid,
          // fetch mac entry & get port_lag_hdl
          if (mac_entry_handle.data != 0 && nexthop_resolution_on) {
            // rif must be pointing at a vlan, if mac_entry exists, grab
            // interface
            status |=
                switch_store::v_get(mac_entry_handle,
                                    SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                    port_lag_handle);
            if (switch_store::object_type_query(port_lag_handle) ==
                SWITCH_OBJECT_TYPE_TUNNEL) {
              it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_TUNNEL);
              status |= it->second.set_arg(
                  smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_DIP_INDEX,
                  port_lag_handle,
                  SWITCH_TUNNEL_ATTR_DST_IP_INDEX);
              status |= it->second.set_arg(
                  smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_NEXTHOP_INDEX,
                  compute_nexthop_index(nexthop_handle));
              continue;
            }
            // TODO(bfn) : why not call reevaluate_tunnel_nexthops here
          } else if (port_lag_handle.data == 0) {  // If no port or lag
            // post routed flood
            // if mac_entry not found then flood
            // find flood group of the vlan, must exist
            // This feature is not supported since driver does not support
            // action
            // data change when the action profile is in a selector group
            it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES);
            status |= it->second.set_arg(smi_id::P_SET_ECMP_PROPERTIES_BD,
                                         compute_bd(bd_handle));
            continue;
          }
        }
      }

      // we have the nexthop resolved now, add the entry
      it->second.init_action_data(smi_id::A_SET_ECMP_PROPERTIES);
      if (port_lag_handle != 0) {
        status |=
            it->second.set_arg(smi_id::P_SET_ECMP_PROPERTIES_PORT_LAG_INDEX,
                               compute_port_lag_index(port_lag_handle));
        status |= it->second.set_arg(smi_id::P_SET_ECMP_PROPERTIES_BD,
                                     compute_bd(bd_handle));
        status |= it->second.set_arg(
            smi_id::P_SET_ECMP_PROPERTIES_NEXTHOP_INDEX, nexthop_handle);
      }
    }
  }
};

/** @brief ECMP membership handler
 * Manages adding and deleting to a ECMP group. Subscribes to updates from a
 * ECMP member.
 * BF-Runtime requires all members when adding or deleting members to the grp.
 * ECMP group holds the list of members
 * Fetch the member IDs from the ecmp_selector object
 * ECMP_SEL_GRP: grp_id -> member_list -> ECMP_SELECTOR::member_id
 */
class ecmp_membership : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ECMP_MEMBERSHIP;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECMP_MEMBERSHIP_ATTR_PARENT_HANDLE;
  switch_object_id_t ecmp_handle = {}, nhop_handle = {};
  bool enable = false;
  switch_ecmp_attr_type ecmp_type = SWITCH_ECMP_ATTR_TYPE_ECMP;

 public:
  ecmp_membership(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, nhop_handle);
    status |= switch_store::v_get(
        parent, SWITCH_ECMP_MEMBER_ATTR_ECMP_HANDLE, ecmp_handle);
    status |=
        switch_store::v_get(parent, SWITCH_ECMP_MEMBER_ATTR_ENABLE, enable);
  }

  // add to group
  // ACTION_MEMBER_ID is derived from nexthop_handle of the ecmp member
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (ecmp_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    std::vector<switch_object_id_t> ecmp_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    std::map<uint32_t, switch_object_id_t> idx_map;
    bool is_update = false;
    switch_enum_t e = {0};

    status |= switch_store::v_get(ecmp_handle, SWITCH_ECMP_ATTR_TYPE, e);
    ecmp_type = static_cast<switch_ecmp_attr_type>(e.enumdata);

    status |= switch_store::v_get(
        ecmp_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);

    // BF-Runtime requires all members when adding/updating/deleting group
    // members. We ensure here that all ecmp members including current member if
    // not already present are passed to the driver for succesful create/update,
    // while during delete we ensure that current member is not part of the
    // ecmp_members list. Detail: S3 infra create_membership is evaluated post
    // the creation of an ecmp member while delete_membership is evaluated
    // before deletion of ecmp_member. Thus when an ecmp member is created it is
    // already part of the ecmp members list and when it is deleted, it has
    // already been removed from the list by s3 infra.
    if (std::find(ecmp_members.begin(), ecmp_members.end(), get_parent()) !=
        ecmp_members.end()) {
      if (!switch_store::smiContext::context().in_warm_init()) {
        is_update = true;
      }
    } else {
      ecmp_members.push_back(get_parent());
    }

    if (ecmp_type == SWITCH_ECMP_ATTR_TYPE_FINE_GRAIN_ECMP ||
        ecmp_type == SWITCH_ECMP_ATTR_TYPE_ORDERED_ECMP) {
      const switch_attr_id_t attr_id =
          (ecmp_type == SWITCH_ECMP_ATTR_TYPE_FINE_GRAIN_ECMP)
              ? SWITCH_ECMP_MEMBER_ATTR_INDEX
              : SWITCH_ECMP_MEMBER_ATTR_SEQUENCE_ID;
      std::sort(ecmp_members.begin(),
                ecmp_members.end(),
                [attr_id, &status](const switch_object_id_t &lhs,
                                   const switch_object_id_t &rhs) {
                  uint32_t index1{}, index2{};
                  status |= switch_store::v_get(lhs, attr_id, index1);
                  status |= switch_store::v_get(rhs, attr_id, index2);
                  return (index1 < index2);
                });
    }
    for (auto const mbr : ecmp_members) {
      uint32_t weight = 1;
      bool mbr_st_temp = true;
      status |=
          switch_store::v_get(mbr, SWITCH_ECMP_MEMBER_ATTR_ENABLE, mbr_st_temp);
      status |=
          switch_store::v_get(mbr, SWITCH_ECMP_MEMBER_ATTR_WEIGHT, weight);
      for (uint32_t i = 1; i <= weight; i++) {
        mbrs.push_back(compute_ecmp_selector_member_id(mbr, i));
        mbr_status.push_back(mbr_st_temp);
      }
    }

    // add this member
    ecmp_selector ecmp_sel_member(get_parent(), status, 1);
    status = ecmp_sel_member.create_update();

    ecmp_selector_group ecmp_group_obj(ecmp_handle, status);
    status |= ecmp_group_obj.add_delete_member(mbrs, mbr_status);
    if (status) return SWITCH_STATUS_FAILURE;

    // this is an update on the ecmp member. nothing to modify beyond here
    if (is_update) return status;

    int member_count = ecmp_members.size();
    if (member_count == 1 ||
        (member_count > 0 &&
         switch_store::smiContext::context().in_warm_init())) {
      // add the ecmp table entry pointing to the selector group when first
      // member is added
      ecmp_table ecmp_table_obj(ecmp_handle, status, member_count);
      ecmp_table_obj.create_update();
    }

    status |= auto_object::create_update();
    return status;
  }

  // remove from group
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (ecmp_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    std::vector<switch_object_id_t> ecmp_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    status |= switch_store::v_get(
        ecmp_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);
    // At this point, this member is already removed from members list
    for (auto const mbr : ecmp_members) {
      bool mbr_st_temp = true;
      uint32_t weight = 1;
      status |=
          switch_store::v_get(mbr, SWITCH_ECMP_MEMBER_ATTR_ENABLE, mbr_st_temp);
      status |=
          switch_store::v_get(mbr, SWITCH_ECMP_MEMBER_ATTR_WEIGHT, weight);
      for (uint32_t i = 1; i <= weight; i++) {
        mbrs.push_back(compute_ecmp_selector_member_id(mbr, i));
        mbr_status.push_back(mbr_st_temp);
      }
    }

    if (ecmp_members.size() == 0) {
      // delete the ecmp table entry to the selector group
      ecmp_table ecmp_table_obj(ecmp_handle, status, mbrs.size());
      ecmp_table_obj.create_update();
    }

    ecmp_selector_group ecmp_group_obj(ecmp_handle, status);
    status |= ecmp_group_obj.add_delete_member(mbrs, mbr_status);
    if (status) return SWITCH_STATUS_FAILURE;

    uint32_t weight = 1;
    status |= switch_store::v_get(
        get_parent(), SWITCH_ECMP_MEMBER_ATTR_WEIGHT, weight);
    ecmp_selector ecmp_sel_member(get_parent(), status, 1);
    status = ecmp_sel_member.del();
    if (status) return SWITCH_STATUS_FAILURE;

    status |= auto_object::del();
    return status;
  }
};

/*
 * - Parent is nexthop_resolution object if nexthop resolution feature is on
 * - Parent is nexthop object if nexthop resolution feature is off
 */
class nexthop_table : public p4_object_match_action {
  friend class ecmp_membership;

 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_NEXTHOP_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_NEXTHOP_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_NEXTHOP_TABLE_ATTR_PARENT_HANDLE;

 public:
  nexthop_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_NEXTHOP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t neighbor_handle = {}, nexthop_handle = {},
                       mac_entry_handle = {}, rif_handle = {}, vlan_handle = {},
                       port_lag_handle = {}, bd_handle = {},
                       nr_parent_handle = {};
    switch_enum_t e = {};
    switch_enum_t rif_type = {};
    bool nexthop_resolution_on = true;
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);
    switch_object_id_t dev_hdl = {0};

    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                              neighbor_handle);
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                              mac_entry_handle);
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE,
                              nr_parent_handle);
      switch_object_type_t nr_parent_ot =
          switch_store::object_type_query(nr_parent_handle);
      if (nr_parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
        nexthop_handle = nr_parent_handle;
      } else if (nr_parent_ot == SWITCH_OBJECT_TYPE_TUNNEL) {
        status |= match_key.set_exact(smi_id::F_NEXTHOP_LOCAL_MD_NEXTHOP,
                                      compute_nexthop_index(parent));
        action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_TUNNEL);
        status |= action_entry.set_arg(
            smi_id::P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX,
            nr_parent_handle,
            SWITCH_TUNNEL_ATTR_DST_IP_INDEX);
        return;
      } else {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_NEXTHOP,
            "{}:{}: Invalid parent object {:#x} type {} of {} object {:#x}",
            __func__,
            __LINE__,
            nr_parent_handle.data,
            switch_store::object_name_get_from_type(nr_parent_ot),
            switch_store::object_name_get_from_type(parent_ot),
            parent.data);
        clear_attrs();
        return;
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      // comes here when nexthop resolution feature is off
      nexthop_handle = parent;
      status |= resolve_neighbor(neighbor_handle, nexthop_handle);
    } else {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NEXTHOP,
          "{}:{}: Invalid parent object {:#x} type {} of nexthop table object",
          __func__,
          __LINE__,
          parent.data,
          switch_store::object_name_get_from_type(parent_ot));
      clear_attrs();
      return;
    }

    status |= match_key.set_exact(smi_id::F_NEXTHOP_LOCAL_MD_NEXTHOP,
                                  compute_nexthop_index(parent));

    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_DEVICE, dev_hdl);
    nexthop_resolution_on = is_nexthop_resolution_feature_on(dev_hdl);

    status |= switch_store::v_get(nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, e);
    // tunnel nexthop
    if ((e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) ||
        (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
      if (nexthop_resolution_on) {
        action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_TUNNEL);
        status |= action_entry.set_arg(
            smi_id::P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX,
            nexthop_handle,
            SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_INDEX);
        return;
      } else {
        // revisit for sal tunnel
      }
    }

    if (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_DROP) {
      // Drop
      action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_DROP);
      status |= action_entry.set_arg(
          smi_id::P_SET_NEXTHOP_PROPERTIES_DROP_DROP_REASON,
          SWITCH_DROP_REASON_NEXTHOP);
      return;
    } else if (e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_GLEAN ||
               (neighbor_handle == 0 && nexthop_resolution_on)) {
      // Glean
      action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_GLEAN);
    } else {
      // non-tunnel nexthop i.e. IP and MPLS
      if (neighbor_handle.data) {
        switch_enum_t packet_action{};
        status |= switch_store::v_get(
            neighbor_handle, SWITCH_NEIGHBOR_ATTR_PACKET_ACTION, packet_action);
        if (packet_action.enumdata == SWITCH_NEIGHBOR_ATTR_PACKET_ACTION_TRAP) {
          switch_object_id_t hostif_udt_trap_handle{};
          status |=
              switch_store::v_get(neighbor_handle,
                                  SWITCH_NEIGHBOR_ATTR_USER_DEFINED_TRAP_HANDLE,
                                  hostif_udt_trap_handle);
          if (hostif_udt_trap_handle.data) {
            action_entry.init_action_data(
                smi_id::A_SET_NEXTHOP_PROPERTIES_GLEAN);
            action_entry.set_arg(smi_id::P_SET_NEXTHOP_PROPERTIES_GLEAN_TRAP_ID,
                                 hostif_udt_trap_handle);
          } else {
            action_entry.init_action_data(
                smi_id::A_SET_NEXTHOP_PROPERTIES_DROP);
          }
          return;
        }
      }

      // If nexthop resolution feature is off, get port_lag_hdl
      // from nexthop object
      if (nexthop_resolution_on == false) {
        status |= switch_store::v_get(nexthop_handle,
                                      SWITCH_NEXTHOP_ATTR_PORT_LAG_HANDLE,
                                      port_lag_handle);
      }

      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);

      // If L3 interface, get bd from the RIF
      if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) ||
          (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) ||
          (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT)) {
        // If nexthop resolution feature is on, get port_lag_hdl from rif
        if (nexthop_resolution_on) {
          status |= switch_store::v_get(
              rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
        }

        status |= find_auto_oid(rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        if (bd_handle.data == 0) {
          status = SWITCH_STATUS_FAILURE;
          return;
        }
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        uint8_t nat_zone = 0;
        switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_NAT_ZONE, nat_zone);

        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        if (bd_handle.data == 0) {
          status = SWITCH_STATUS_FAILURE;
          return;
        }

        // If nexthop resolution feature is on & mac_entry is valid,
        // fetch mac entry & get port_lag_hdl
        if (mac_entry_handle.data != 0 && nexthop_resolution_on) {
          // rif must be pointing at a vlan, if mac_entry exists, grab interface
          status |=
              switch_store::v_get(mac_entry_handle,
                                  SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                  port_lag_handle);
          reevaluate_tunnel_nexthops();
        } else if (neighbor_handle.data != 0) {
          switch_mac_addr_t mac_addr = {};
          switch_mac_addr_t bcast_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
          switch_store::v_get(
              neighbor_handle, SWITCH_NEIGHBOR_ATTR_MAC_ADDRESS, mac_addr);
          // post routed flood direct broadcast packets
          if (SWITCH_MAC_COMPARE(mac_addr, bcast_mac)) {
            action_entry.init_action_data(
                smi_id::A_SET_NEXTHOP_PROPERTIES_PR_FLOOD);
            status |= action_entry.set_arg(
                smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_BD,
                compute_bd(bd_handle));
            if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
              status |= action_entry.set_arg(
                  smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_NAT_ZONE, nat_zone);
            }
            status |= action_entry.set_arg(
                smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_MGID,
                compute_pre_mgid(vlan_handle));
            return;
          } else {
            action_entry.init_action_data(
                smi_id::A_SET_NEXTHOP_PROPERTIES_GLEAN);
            return;
          }
        } else if (port_lag_handle.data == 0) {  // If no port or lag
          action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_GLEAN);
          return;
        }
      }
    }

    if (port_lag_handle != 0) {
      // Will drop packets, if egress port oper_state is down
      // nexthop object has dependency on
      // "port" -> attribute: "oper_state"
      bool sw_model = false;
      bf_pal_pltfm_type_get(0, &sw_model);
      if (!sw_model && switch_store::object_type_query(port_lag_handle) ==
                           SWITCH_OBJECT_TYPE_PORT) {
        status |= switch_store::v_get(
            port_lag_handle, SWITCH_PORT_ATTR_OPER_STATE, e);
        if (e.enumdata != SWITCH_PORT_ATTR_OPER_STATE_UP) {
          action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_DROP);
          status |= action_entry.set_arg(
              smi_id::P_SET_NEXTHOP_PROPERTIES_DROP_DROP_REASON,
              SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN);
          return;
        }
      }

      if (nexthop_resolution_on && e.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP &&
          mac_entry_handle.data != 0 &&
          switch_store::object_type_query(port_lag_handle) ==
              SWITCH_OBJECT_TYPE_TUNNEL) {
        action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES_TUNNEL);
        status |= action_entry.set_arg(
            smi_id::P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX,
            port_lag_handle,
            SWITCH_TUNNEL_ATTR_DST_IP_INDEX);
        return;
      } else {
        action_entry.init_action_data(smi_id::A_SET_NEXTHOP_PROPERTIES);
        status |= action_entry.set_arg(
            smi_id::P_SET_NEXTHOP_PROPERTIES_PORT_LAG_INDEX,
            compute_port_lag_index(port_lag_handle));
        status |= action_entry.set_arg(smi_id::P_SET_NEXTHOP_PROPERTIES_BD,
                                       compute_bd(bd_handle));
        if (!nexthop_resolution_on) {
          // If nexthop resolution feature is off, update ecmp_members from here
          // Refresh the ecmp selector table
          std::set<switch_object_id_t> ecmp_member_handles;
          status |=
              switch_store::referencing_set_get(nexthop_handle,
                                                SWITCH_OBJECT_TYPE_ECMP_MEMBER,
                                                ecmp_member_handles);
          for (const auto ecmp_member : ecmp_member_handles) {
            ecmp_selector ecmp_sel(ecmp_member, status, 1);
            ecmp_sel.create_update();
          }
        }
      }

      uint8_t nat_zone = 0;
      switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_NAT_ZONE, nat_zone);
      if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
        status |= action_entry.set_arg(
            smi_id::P_SET_NEXTHOP_PROPERTIES_NAT_ZONE, nat_zone);
      }
    }
  }
};

/*
 * This object acts as a next hop resolver. Its complex because it handles
 * multiple objects that are associated but do not refer to each other.
 *  * Next hop
 *  * Tunnel (P2P)
 *  * Neighbor
 *  * Mac entry
 *
 * These objects are decoupled and can arrive in any order.
 * If we receive updates from Neighbor or Mac Entry object, we find nexthop
 * object and use. Just know that the nexthop_resolution is always an auto
 * object only for the Nexthop and Tunnel objects and the neighbor and mac
 * entry refer to this auto oid
 *
 * If we get deletes, the same as above applies.
 *
 */
class nexthop_resolution : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION;
  static const switch_attr_id_t neighbor_attr_id =
      SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE;
  static const switch_attr_id_t mac_entry_attr_id =
      SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE;

 public:
  std::vector<std::unique_ptr<nexthop_resolution>> nhop_overrides;
  std::vector<switch_object_id_t> nexthop_handles;
  // constructor
  nexthop_resolution(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t neighbor_handle = {0}, nexthop_handle = {0},
                       mac_entry_handle = {0};
    switch_object_type_t ot = switch_store::object_type_query(parent);
    bool p2p_tunnel = false;
    std::vector<switch_object_id_t> rif_handles;

    /* the object_id here might not be the parent (nexthop), we find the
     * nexthop and override */
    if (ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      nexthop_handle = parent;
      switch_store::object_try_lock(nexthop_handle);
      // parent is a nexthop, find neighbor and mac_entry
      status |= resolve_neighbor(neighbor_handle, nexthop_handle);
      if (neighbor_handle != 0) {
        status |= resolve_mac_entry(mac_entry_handle, neighbor_handle);
      }
      nexthop_handles.push_back(nexthop_handle);
    } else if (ot == SWITCH_OBJECT_TYPE_NEIGHBOR) {
      neighbor_handle = parent;
      // parent is a neighbor, find nexthop and mac_entry
      status |= resolve_nexthop(nexthop_handles, neighbor_handle);
      if (neighbor_handle != 0) {
        status |= resolve_mac_entry(mac_entry_handle, neighbor_handle);
      }
    } else if (ot == SWITCH_OBJECT_TYPE_MAC_ENTRY) {
      // no need to handle MAC as parent. Nexthop and neighbor will ensure it
      if (switch_store::smiContext::context().in_warm_init()) {
        attrs.clear();
        return;
      }
      mac_entry_handle = parent;
      // parent is a mac_entry, first get the rif_handle. If the rif is an SVI,
      // get all nexthops using this rif and update them
      switch_object_id_t vlan_handle = {};
      status |= switch_store::v_get(
          mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle);
      switch_object_type_t mac_network_ot =
          switch_store::object_type_query(vlan_handle);
      if (mac_network_ot == SWITCH_OBJECT_TYPE_VLAN) {
        status |= switch_store::v_get(
            vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
      }
      for (const auto rif_handle : rif_handles) {
        // find all neighbors on this SVI RIF and check if MAC matches
        std::set<switch_object_id_t> neighbor_handles;
        switch_mac_addr_t mac_addr = {};
        status |= switch_store::v_get(
            mac_entry_handle, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac_addr);
        status |= switch_store::referencing_set_get(
            rif_handle, SWITCH_OBJECT_TYPE_NEIGHBOR, neighbor_handles);
        for (auto tmp_neighbor_handle : neighbor_handles) {
          switch_mac_addr_t neigh_mac = {};
          status |= switch_store::v_get(
              tmp_neighbor_handle, SWITCH_NEIGHBOR_ATTR_MAC_ADDRESS, neigh_mac);
          // if MAC matches, update the port_lag_index for the nexthop
          if (SWITCH_MAC_COMPARE(mac_addr, neigh_mac)) {
            attrs.insert(attr_w(mac_entry_attr_id, mac_entry_handle));
            attrs.insert(attr_w(neighbor_attr_id, tmp_neighbor_handle));
            std::vector<switch_object_id_t> tmp_nhop_handle_list;
            status |=
                resolve_nexthop(tmp_nhop_handle_list, tmp_neighbor_handle);
            for (auto tmp_nhop_handle : tmp_nhop_handle_list) {
              if (tmp_nhop_handle != 0) {
                if (switch_store::object_try_lock(tmp_nhop_handle) == 0)
                  continue;
                nhop_overrides.push_back(std::unique_ptr<nexthop_resolution>(
                    new nexthop_resolution(tmp_nhop_handle, status)));
                nexthop_handles.push_back(tmp_nhop_handle);
              }
            }
          }
        }
      }
      attrs.clear();
      return;
    } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
      switch_enum_t peer_mode = {0};
      status |=
          switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode);
      if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN) ||
          peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
        attrs.clear();
        return;
      } else {
        p2p_tunnel = true;
      }
    }

    if (nexthop_handles.size() == 0 && !p2p_tunnel) {
      attrs.clear();
      return;
    }

    attrs.insert(attr_w(mac_entry_attr_id, mac_entry_handle));
    attrs.insert(attr_w(neighbor_attr_id, neighbor_handle));
    if (ot != SWITCH_OBJECT_TYPE_NEXTHOP && ot != SWITCH_OBJECT_TYPE_TUNNEL) {
      for (auto nh_handle : nexthop_handles) {
        nhop_overrides.push_back(std::unique_ptr<nexthop_resolution>(
            new nexthop_resolution(nh_handle, status)));
      }
    }
  }
  switch_status_t update_auto_objects_override(bool clear_mac,
                                               bool clear_neighbor) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    const switch_object_id_t empty_handle = {0};

    for (auto &&nhop_override : nhop_overrides) {
      status |= nhop_override->create_update();
      switch_object_id_t temp_oid = nhop_override->get_auto_oid();
      // This may need to happen for more tables
      if (temp_oid.data != 0) {
        if (clear_mac)
          switch_store::v_set(temp_oid, mac_entry_attr_id, empty_handle);
        if (clear_neighbor)
          switch_store::v_set(temp_oid, neighbor_attr_id, empty_handle);

        // Refresh the nexthop table
        nexthop_table nhop_table(temp_oid, status);
        nhop_table.create_update();

        // Refresh the ecmp selector table
        std::set<switch_object_id_t> ecmp_member_handles;
        status |=
            switch_store::referencing_set_get(nhop_override->get_parent(),
                                              SWITCH_OBJECT_TYPE_ECMP_MEMBER,
                                              ecmp_member_handles);
        for (const auto ecmp_member : ecmp_member_handles) {
          ecmp_selector ecmp_sel(ecmp_member, status, 1);
          ecmp_sel.create_update();
        }

        // Refresh the Neighbor Table
        std::unique_ptr<object> neighbor_rewrite(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_NEIGHBOR_REWRITE, temp_oid, status));
        neighbor_rewrite->create_update();

        // Refresh the Outer Nexthop Table
        std::unique_ptr<object> outer_nexthop(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_OUTER_NEXTHOP, temp_oid, status));
        outer_nexthop->create_update();

        if (feature::is_feature_set(SWITCH_FEATURE_IPV4_TUNNEL) ||
            feature::is_feature_set(SWITCH_FEATURE_IPV6_TUNNEL)) {
          // Refresh the outer ecmp selector table
          // one entry for nexthop and one each for every ecmp member
          std::unique_ptr<object> outer_ecmp_sel_nh(
              factory::get_instance().create(
                  SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR, temp_oid, status));
          outer_ecmp_sel_nh->create_update();
        }

        if (feature::is_feature_set(SWITCH_FEATURE_SRV6)) {
          // Refresh sid_rewrite Table
          std::unique_ptr<object> sid_rewrite(factory::get_instance().create(
              SWITCH_OBJECT_TYPE_SID_REWRITE, temp_oid, status));
          sid_rewrite->create_update();
        }
      }
    }
    return status;
  }
  switch_status_t create_update() {
    if (nhop_overrides.size() > 0) {
      return update_auto_objects_override(false, false);
    }

    // This only gets called when parent is nexthop or (p2p) tunnel
    return auto_object::create_update();
  }
  switch_status_t del() {
    bool clear_mac = false, clear_neighbor = false;

    if (nhop_overrides.size() > 0) {
      // this is messy. For the delete case, we want to know which parent is
      // being deleted and set the attributes correctly
      if (switch_store::object_type_query(get_parent()) ==
          SWITCH_OBJECT_TYPE_MAC_ENTRY) {
        clear_mac = true;
      }
      if (switch_store::object_type_query(get_parent()) ==
          SWITCH_OBJECT_TYPE_NEIGHBOR) {
        clear_neighbor = true;
      }
      return update_auto_objects_override(clear_mac, clear_neighbor);
    }

    // This only gets called when parent is nexthop or (p2p) tunnel
    if (get_auto_oid().data == 0) return SWITCH_STATUS_SUCCESS;
    return auto_object::del();
  }
  ~nexthop_resolution() {
    for (const auto nhop : nexthop_handles) {
      switch_store::object_unlock(nhop);
    }
  }
};

switch_status_t reevaluate_tunnel_nexthops_cb(
    switch_object_id_t tunnel_dest_ip_handle, switch_attribute_t attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  std::unique_ptr<object> outer_fib_table(factory::get_instance().create(
      SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE, tunnel_dest_ip_handle, status));
  outer_fib_table->create_update();

  return status;
}

// This is invoked in 2 cases
// 1 - Everytime a route is created or deleted
// 2 - When an ARP is resolved
switch_status_t reevaluate_tunnel_nexthops() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attribute_t attr = {};

  if ((feature::is_feature_set(SWITCH_FEATURE_IPV4_TUNNEL)) == false)
    return status;

  status = execute_cb_for_all(
      SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP, reevaluate_tunnel_nexthops_cb, attr);
  return status;
}

switch_status_t before_nexthop_create(const switch_object_type_t object_type,
                                      std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t nexthop_type{};
  switch_ip_address_t dst_ip{};
  std::vector<switch_ip_address_t> sidlist;
  switch_object_id_t device_handle{}, tunnel_dest_ip_handle{}, sidlist_handle{},
      tunnel_handle{}, rif_handle{}, uvrf_handle{};

  if (switch_store::smiContext::context().in_warm_init()) return status;
  auto it = attrs.find(static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_TYPE));
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
  it->v_get(nexthop_type);
  if ((nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
      (nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST))
    return status;
  if (nexthop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) {
    it = attrs.find(static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_DEST_IP));
    CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
    it->v_get(dst_ip);
  } else if (nexthop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST) {
    it = attrs.find(
        static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID));
    CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
    status = it->v_get(sidlist_handle);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    const auto sidlist_ot = switch_store::object_type_query(sidlist_handle);
    if (sidlist_ot == SWITCH_OBJECT_TYPE_SEGMENTROUTE_SIDLIST) {
      status |=
          switch_store::v_get(sidlist_handle,
                              SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST,
                              sidlist);
    }
    dst_ip = sidlist[0];
    if (sidlist.size() == 0) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          object_type,
          "{}:{}: Failed to find SID List for nexthop_type = srv6_sidlist",
          __func__,
          __LINE__);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
  }
  it = attrs.find(static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_DEVICE));
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= it->v_get(device_handle);

  it = attrs.find(static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_HANDLE));
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= it->v_get(tunnel_handle);
  switch_object_type_t ot = switch_store::object_type_query(tunnel_handle);
  CHECK_RET(ot != SWITCH_OBJECT_TYPE_TUNNEL, SWITCH_STATUS_INVALID_PARAMETER);
  status |= switch_store::v_get(
      tunnel_handle, SWITCH_TUNNEL_ATTR_UNDERLAY_RIF_HANDLE, rif_handle);
  status |=
      switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, uvrf_handle);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

  std::set<attr_w> lookup_attrs;
  lookup_attrs.insert(attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_DEVICE, device_handle));
  lookup_attrs.insert(
      attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_UNDERLAY_VRF_HANDLE, uvrf_handle));
  lookup_attrs.insert(attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP, dst_ip));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP, lookup_attrs, tunnel_dest_ip_handle);
  if (tunnel_dest_ip_handle.data == 0) {
    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP, lookup_attrs, tunnel_dest_ip_handle);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  }
  it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_HANDLE));
  if (it != attrs.end()) {
    attrs.erase(it);
  }
  attrs.insert(
      attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_HANDLE, tunnel_dest_ip_handle));
  attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_INDEX,
                      switch_store::handle_to_id(tunnel_dest_ip_handle)));
  return status;
}

switch_status_t after_nexthop_delete(const switch_object_type_t object_type,
                                     const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t nexthop_type{};
  switch_object_id_t tunnel_dest_ip_handle{}, sidlist_handle{};
  (void)object_type;
  if (switch_store::smiContext::context().in_warm_init()) return status;

  for (auto it = attrs.begin(); it != attrs.end(); it++) {
    switch (it->id_get()) {
      case SWITCH_NEXTHOP_ATTR_TYPE:
        status = it->v_get(nexthop_type);
        CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
        break;
      case SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_HANDLE:
        status = it->v_get(tunnel_dest_ip_handle);
        CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
        break;
      case SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID:
        status = it->v_get(sidlist_handle);
        CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
        break;
      default:
        break;
    }
  }

  if ((nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
      (nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST))
    return status;

  if (tunnel_dest_ip_handle.data == 0) return status;
  const auto &nexthops_set = switch_store::get_object_references(
      tunnel_dest_ip_handle, SWITCH_OBJECT_TYPE_NEXTHOP);
  auto const &tunnels_set = switch_store::get_object_references(
      tunnel_dest_ip_handle, SWITCH_OBJECT_TYPE_TUNNEL);
  if (nexthops_set.size() == 0 && tunnels_set.size() == 0) {
    status = switch_store::object_delete(tunnel_dest_ip_handle);
  }
  return status;
}

switch_status_t before_nexthop_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t nexthop_type{};
  switch_object_id_t sidlist_handle{}, stats_handle{};

  if (switch_store::smiContext::context().in_warm_init()) return status;
  status =
      switch_store::v_get(object_id, SWITCH_NEXTHOP_ATTR_TYPE, nexthop_type);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  if ((nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
      (nexthop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST))
    return status;

  // store sidlist nexthop counters
  if (nexthop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST) {
    std::vector<switch_counter_t> cntrs(
        SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_MAX);

    status = switch_store::v_get(
        object_id, SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

    const auto &stats = switch_store::get_object_references(
        sidlist_handle, SWITCH_OBJECT_TYPE_SIDLIST_STATS);
    // one and only one sidlist_stats can refer to segmentroute_sidlist
    stats_handle = stats[0].oid;

    std::unique_ptr<object> rewrite_object(factory::get_instance().create(
        SWITCH_OBJECT_TYPE_SID_REWRITE, object_id, status));
    rewrite_object->counters_get(object_id, cntrs);

    uint64_t pkts, bytes;
    pkts = cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS].count;
    bytes = cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES].count;

    std::vector<uint64_t> cntr_list;

    status = switch_store::v_get(
        stats_handle, SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS, cntr_list);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

    if (cntr_list.size() == SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_MAX) {
      pkts += cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS];
      bytes += cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES];
    }

    cntr_list.clear();

    cntr_list.push_back(static_cast<uint64_t>(pkts));
    cntr_list.push_back(static_cast<uint64_t>(bytes));

    attr_w stored_cntrs_attr(SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS);
    stored_cntrs_attr.v_set(cntr_list);
    status = switch_store::attribute_set(stats_handle, stored_cntrs_attr);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  }
  return status;
}

bool skip_auto_object_for_nexthop_ecmp(const switch_object_id_t object_id,
                                       const switch_object_type_t auto_ot) {
  bool skip = false;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t ot;
  switch_object_id_t dev_hdl = {};
  bool nexthop_resolution_on = true;

  if (object_id.data == 0) return false;
  if (switch_store::is_object_type_valid(auto_ot) == false) return false;
  ot = switch_store::object_type_query(object_id);

  /* For parent object - SWITCH_OBJECT_TYPE_NEXTHOP
      With global/local nexthop_resolution feature set to false: [SAL]
        Skip auto object:
          - nexthop_resolution
        Allow auto object:
          - nexthop_table
          - neighbor_rewrite
          - outer_nexthop
          - tunnel_nexthop

      With global/local nexthop_resolution attribute set to true: [SAI]
        Skip auto object:
          - nexthop_table
          - neighbor_rewrite
          - outer_nexthop
          - tunnel_nexthop
        Allow auto object:
          - nexthop_resolution
  */
  if (ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
    // Alternate option is to have the feature control in nexthop object
    // as well i.e. SWITCH_NEXTHOP_ATTR_NEXTHOP_RESOLUTION
    // currently checking the global flag for feature [nexthop resolution]
    status |=
        switch_store::v_get(object_id, SWITCH_NEXTHOP_ATTR_DEVICE, dev_hdl);
    nexthop_resolution_on = is_nexthop_resolution_feature_on(dev_hdl);

    switch (auto_ot) {
      case SWITCH_OBJECT_TYPE_NEXTHOP_TABLE:
      case SWITCH_OBJECT_TYPE_NEIGHBOR_REWRITE:
      case SWITCH_OBJECT_TYPE_OUTER_NEXTHOP:
      case SWITCH_OBJECT_TYPE_TUNNEL_NEXTHOP:
        if (nexthop_resolution_on == true) {
          skip = true;
        }
        break;

      case SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION:
        if (nexthop_resolution_on == false) {
          skip = true;
        }
        break;

      default:
        break;
    }
  }

  /*  For parent object - SWITCH_OBJECT_TYPE_ECMP
      With global/local nexthop_resolution feature set to false: [SAL]
        Skip auto object:
        - outer_ecmp_selector_group

      With global/local nexthop_resolution attribute set to true: [SAI]
        Allow auto object:
        - outer_ecmp_selector_group
  */
  if (ot == SWITCH_OBJECT_TYPE_ECMP) {
    // currently checking the global flag for feature [nexthop resolution]
    status |= switch_store::v_get(object_id, SWITCH_ECMP_ATTR_DEVICE, dev_hdl);
    nexthop_resolution_on = is_nexthop_resolution_feature_on(dev_hdl);

    switch (auto_ot) {
      case SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR_GROUP:
        if (nexthop_resolution_on == false) {
          skip = true;
        }
        break;

      default:
        break;
    }
  }

  /* For parent object - SWITCH_OBJECT_TYPE_ECMP_MEMBER
      With global/local nexthop_resolution feature set to false: [SAL]
        Skip auto object:
        - outer_ecmp_membership
        - outer_ecmp_selector

      With global/local nexthop_resolution attribute set to true: [SAI]
        Allow auto object:
        - outer_ecmp_membership
        - outer_ecmp_selector
  */
  if (ot == SWITCH_OBJECT_TYPE_ECMP_MEMBER) {
    // currently checking the global flag for feature [nexthop resolution]
    status |=
        switch_store::v_get(object_id, SWITCH_ECMP_MEMBER_ATTR_DEVICE, dev_hdl);
    nexthop_resolution_on = is_nexthop_resolution_feature_on(dev_hdl);

    switch (auto_ot) {
      case SWITCH_OBJECT_TYPE_OUTER_ECMP_MEMBERSHIP:
      case SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR:
        if (nexthop_resolution_on == false) {
          skip = true;
        }
        break;

      default:
        break;
    }
  }

  if (skip) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               ot,
               "{}:{}: Skip auto object {} for parent type {} hdl {}, nexthop "
               "resolution feature enabled - {}",
               __func__,
               __LINE__,
               switch_store::object_name_get_from_type(auto_ot),
               switch_store::object_name_get_from_type(ot),
               object_id,
               nexthop_resolution_on);
  }

  return skip;
}

switch_status_t nexthop_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(ecmp_selector_group, SWITCH_OBJECT_TYPE_ECMP_SELECTOR_GROUP);
  REGISTER_OBJECT(ecmp_membership, SWITCH_OBJECT_TYPE_ECMP_MEMBERSHIP);
  // REGISTER_OBJECT(ecmp_selector, SWITCH_OBJECT_TYPE_ECMP_SELECTOR);
  REGISTER_OBJECT(ecmp_table, SWITCH_OBJECT_TYPE_ECMP_TABLE);
  REGISTER_OBJECT(nexthop_resolution, SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION);
  REGISTER_OBJECT(nexthop_table, SWITCH_OBJECT_TYPE_NEXTHOP_TABLE);

  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_NEXTHOP,
                                                  &before_nexthop_create);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_NEXTHOP,
                                                 &after_nexthop_delete);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_NEXTHOP,
                                                  &before_nexthop_delete);

  status |= switch_store::reg_skip_auto_object_trigs(
      SWITCH_OBJECT_TYPE_NEXTHOP, &skip_auto_object_for_nexthop_ecmp);
  status |= switch_store::reg_skip_auto_object_trigs(
      SWITCH_OBJECT_TYPE_ECMP, &skip_auto_object_for_nexthop_ecmp);
  status |= switch_store::reg_skip_auto_object_trigs(
      SWITCH_OBJECT_TYPE_ECMP_MEMBER, &skip_auto_object_for_nexthop_ecmp);

  return status;
}

switch_status_t nexthop_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
