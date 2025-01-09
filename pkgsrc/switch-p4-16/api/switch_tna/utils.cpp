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


#include "switch_tna/utils.h"

#include <arpa/inet.h>

extern "C" {
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_mc.h>
#include <lld/bf_lld_if.h>
#include <lld/lld_sku.h>
}

#include <bitset>
#include <vector>
#include <set>
#include <mutex>  // NOLINT(build/c++11)

#include "common/multicast.h"
#include "switch_tna/acl.h"
#include "../../s3/switch_lpm_int.h"

#define MASK_32BIT 0xFFFFFFFF

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

bool sai_api_mode = false;

void sai_mode_set(bool set) { sai_api_mode = set; }

bool sai_mode() { return sai_api_mode; }

uint16_t compute_port_lag_index(switch_object_id_t handle) {
  const switch_object_type_t object_type =
      switch_store::object_type_query(handle);
  bool peer_link = false;
  int port_lag_type;
  if (object_type == SWITCH_OBJECT_TYPE_PORT) {
    port_lag_type = 0;
  } else if (object_type == SWITCH_OBJECT_TYPE_LAG) {
    switch_store::v_get(handle, SWITCH_LAG_ATTR_IS_PEER_LINK, peer_link);
    if (peer_link) return MLAG_PEER_LINK_PORT_LAG_INDEX;
    port_lag_type = 1;
  } else {
    return 0;
  }
  return (switch_store::handle_to_id(handle) |
          (port_lag_type << PORT_LAG_INDEX_WIDTH));
}

switch_status_t get_parent_of_bd(switch_object_id_t bd_handle,
                                 switch_object_id_t &vlan_handle,
                                 switch_object_id_t &bridge_handle,
                                 switch_object_id_t &rif_handle,
                                 switch_object_id_t &vrf_handle) {
  switch_object_id_t bd_parent_handle = {};

  vlan_handle.data = 0;
  bridge_handle.data = 0;
  rif_handle.data = 0;

  auto status = switch_store::v_get(
      bd_handle, SWITCH_BD_ATTR_PARENT_HANDLE, bd_parent_handle);
  const auto bd_parent_type = switch_store::object_type_query(bd_parent_handle);
  switch (bd_parent_type) {
    case SWITCH_OBJECT_TYPE_RIF:
      rif_handle = bd_parent_handle;
      break;
    case SWITCH_OBJECT_TYPE_VLAN: {
      vlan_handle = bd_parent_handle;
      std::vector<switch_object_id_t> rif_handles;
      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
      if (rif_handles.size() > 0) {
        rif_handle = rif_handles[0];
      }
    } break;
    case SWITCH_OBJECT_TYPE_VRF:
      vrf_handle = bd_parent_handle;
      break;
    default:
      switch_log(
          SWITCH_API_LEVEL_ERROR, bd_parent_type, "unexpected parameter");
      status |= SWITCH_STATUS_INVALID_PARAMETER;
  }
  return status;
}

switch_status_t get_bd_from_bd_member(switch_object_id_t bd_member_handle,
                                      switch_object_id_t &bd_handle) {
  switch_object_id_t parent_handle = {}, handle = {};
  auto status = switch_store::v_get(
      bd_member_handle, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, parent_handle);
  const auto parent_type = switch_store::object_type_query(parent_handle);
  switch (parent_type) {
    case SWITCH_OBJECT_TYPE_VLAN_MEMBER:
      status |= switch_store::v_get(
          parent_handle, SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE, handle);
      status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      break;
    case SWITCH_OBJECT_TYPE_RIF:
      status |= find_auto_oid(parent_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      break;
    default:
      switch_log(SWITCH_API_LEVEL_ERROR, parent_type, "unexpected parameter");
      status |= SWITCH_STATUS_INVALID_PARAMETER;
  }
  return status;
}

switch_status_t get_bd_member_from_port_bd_to_vlan(
    switch_object_id_t port_bd_to_vlan_handle,
    switch_object_id_t &bd_member_handle) {
  auto status =
      switch_store::v_get(port_bd_to_vlan_handle,
                          SWITCH_PORT_BD_TO_VLAN_MAPPING_ATTR_PARENT_HANDLE,
                          bd_member_handle);
  const auto parent_type = switch_store::object_type_query(bd_member_handle);
  if (parent_type != SWITCH_OBJECT_TYPE_BD_MEMBER) {
    switch_log(SWITCH_API_LEVEL_ERROR, parent_type, "unexpected object type");
  }
  return status;
}

switch_status_t get_bd_for_object(switch_object_id_t oid,
                                  switch_object_id_t &bd_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t rif_type = {};
  switch_object_id_t vlan_handle = {0};
  const auto parent_type = switch_store::object_type_query(oid);
  switch (parent_type) {
    case SWITCH_OBJECT_TYPE_VLAN:
      status |= find_auto_oid(oid, SWITCH_OBJECT_TYPE_BD, bd_handle);
      break;
    case SWITCH_OBJECT_TYPE_RIF:
      status |= switch_store::v_get(oid, SWITCH_RIF_ATTR_TYPE, rif_type);
      status |=
          switch_store::v_get(oid, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      } else {
        status |= find_auto_oid(oid, SWITCH_OBJECT_TYPE_BD, bd_handle);
      }
      break;
    case SWITCH_OBJECT_TYPE_VRF:
      status |= find_auto_oid(oid, SWITCH_OBJECT_TYPE_BD, bd_handle);
      break;
  }
  return status;
}

bf_rt_target_t compute_dev_target_for_table(uint16_t dev_port,
                                            bf_rt_id_t table_id,
                                            bool dir) {
  bf_dev_pipe_t pipe = 0;
  if (dir)
    pipe = INGRESS_DEV_PORT_TO_PIPE(dev_port);
  else
    pipe = EGRESS_DEV_PORT_TO_PIPE(dev_port);

  pipe = SHIFT_PIPE_IF_FOLDED(pipe, _Table(table_id).get_active_pipes());
  bf_rt_target_t dev_target = {.dev_id = 0, .pipe_id = pipe};
  return dev_target;
}

uint16_t compute_nexthop_index(const switch_object_id_t handle) {
  switch_object_id_t obj_handle = {}, dev_hdl = {};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t object_type = switch_store::object_type_query(handle);

  obj_handle = handle;

  if (object_type == SWITCH_OBJECT_TYPE_NEXTHOP) {
    status = switch_store::v_get(handle, SWITCH_NEXTHOP_ATTR_DEVICE, dev_hdl);
  } else if (object_type == SWITCH_OBJECT_TYPE_TUNNEL) {
    status = switch_store::v_get(handle, SWITCH_TUNNEL_ATTR_DEVICE, dev_hdl);
  }
  if (status != SWITCH_STATUS_SUCCESS) return 0;

  if (object_type == SWITCH_OBJECT_TYPE_NEXTHOP ||
      object_type == SWITCH_OBJECT_TYPE_TUNNEL) {
    if (is_nexthop_resolution_feature_on(dev_hdl)) {
      switch_object_id_t nexthop_resolution;
      status |= find_auto_oid(
          handle, SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION, nexthop_resolution);
      if (status != SWITCH_STATUS_SUCCESS || nexthop_resolution.data == 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION,
                   "{}.{}: Fail to get nexthop resolution for nexthop: {}",
                   __func__,
                   __LINE__,
                   switch_error_to_string(SWITCH_STATUS_FAILURE));
        return 0;
      }
      obj_handle = nexthop_resolution;
    }
  }

  const auto id = switch_store::handle_to_id(obj_handle);
  assert(id >> smi_id::F_NEXTHOP_INDEX_WIDTH == 0);
  object_type = switch_store::object_type_query(obj_handle);

  // nexthop index starts at 1. ecmp from oppposite end.
  // this way we can maintain a single namespace for nexthop and ecmp indices
  if (object_type == SWITCH_OBJECT_TYPE_NEXTHOP ||
      object_type == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
    return id;
  } else if (object_type == SWITCH_OBJECT_TYPE_ECMP) {
    return ((1 << smi_id::F_NEXTHOP_INDEX_WIDTH) - id);
  } else {
    return 0;
  }
}

uint16_t compute_vrf(const switch_object_id_t vrf_handle) {
  return (switch_store::handle_to_id(vrf_handle));
}

/**
 * @brief Return vlan_id as bd value if parent of bd is vlan
 *        For all other cases, return (bd_id + 4096)
 *
 * @param oid
 * @return uint16_t BD value
 */
uint16_t compute_bd(const switch_object_id_t bd_handle) {
  uint16_t bd = 0, vlan_id = 0;
  switch_object_id_t parent = {0};
  switch_object_type_t object_type = 0;

  object_type = switch_store::object_type_query(bd_handle);
  if (object_type != SWITCH_OBJECT_TYPE_BD) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}.{}: Incorrect object type {}",
               __func__,
               __LINE__,
               switch_store::object_name_get_from_type(object_type));
    return 0;
  }

  switch_store::v_get(bd_handle, SWITCH_BD_ATTR_PARENT_HANDLE, parent);

  object_type = switch_store::object_type_query(parent);
  if (object_type == SWITCH_OBJECT_TYPE_VLAN) {
    switch_store::v_get(parent, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);
    bd = vlan_id;
  } else {
    bd = static_cast<uint16_t>(bd_handle.data) + SWITCH_MAX_VLANS;
  }
  return bd;
}

switch_status_t update_bind_point_flag(const switch_object_id_t object_id,
                                       const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t new_acl_handle = {0}, curr_acl_handle = {0};
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_id);
  switch_attr_id_t attr_id = attr.id_get();

  attr.v_get(new_acl_handle);
  status |= switch_store::v_get(object_id, attr_id, curr_acl_handle);
  // case 1: bind_point attachment happens, set flag to true
  if ((curr_acl_handle == 0) && (new_acl_handle != 0)) {
    status |= set_bind_point_attach_flag(new_acl_handle, true);
  }
  // case 2: bind_point detachment happens, set flag to flase
  if ((curr_acl_handle != 0) && (new_acl_handle == 0)) {
    std::set<switch_object_id_t> acl_handles;
    switch_store::referencing_set_get(
        curr_acl_handle, object_type, acl_handles);
    if (acl_handles.size() == 1) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{} Set bind_point_attach_flag:FALSE for ACL Handle:{}",
                 __func__,
                 curr_acl_handle);
      status |= set_bind_point_attach_flag(curr_acl_handle, false);
    }
  }
  // case 3: bind_point changes from one acl_handle to another
  if ((curr_acl_handle != 0) && (new_acl_handle != 0)) {
    std::set<switch_object_id_t> acl_handles;
    switch_store::referencing_set_get(
        curr_acl_handle, object_type, acl_handles);
    if (acl_handles.size() == 1) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{} Set bind_point_attach_flag:FALSE for ACL Handle:{}",
                 __func__,
                 curr_acl_handle);
      status |= set_bind_point_attach_flag(curr_acl_handle, false);
    }
    status |= set_bind_point_attach_flag(new_acl_handle, true);
  }
  return status;
}

switch_status_t set_bind_point_attach_flag(const switch_object_id_t acl_handle,
                                           bool new_attach_flag) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t object_type = 0;
  bool curr_attach_flag;

  object_type = switch_store::object_type_query(acl_handle);

  if (object_type == SWITCH_OBJECT_TYPE_ACL_GROUP) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_PORT,
               "bind_point_attach_flag:{} for ACL Group:{}",
               new_attach_flag,
               acl_handle);
    status |= switch_store::v_get(
        acl_handle, SWITCH_ACL_GROUP_ATTR_BIND_POINT_ATTACH, curr_attach_flag);
    if (curr_attach_flag ^ new_attach_flag) {
      status |= switch_store::v_set(
          acl_handle, SWITCH_ACL_GROUP_ATTR_BIND_POINT_ATTACH, new_attach_flag);
      // get all the tables of the group and set the attach flag
      std::vector<switch_object_id_t> acl_group_members;
      status |= switch_store::v_get(acl_handle,
                                    SWITCH_ACL_GROUP_ATTR_ACL_GROUP_MEMBERS,
                                    acl_group_members);
      for (const auto mbr : acl_group_members) {
        switch_object_id_t acl_table_handle = {0};
        status |=
            switch_store::v_get(mbr,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                acl_table_handle);
        std::set<switch_object_id_t> acl_handles;
        switch_store::referencing_set_get(
            acl_table_handle, object_type, acl_handles);
        if (new_attach_flag == false && acl_handles.size() == 0) {
          status |= switch_store::v_set(acl_table_handle,
                                        SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH,
                                        new_attach_flag);
        } else {
          if (new_attach_flag == true) {
            status |=
                switch_store::v_set(acl_table_handle,
                                    SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH,
                                    new_attach_flag);
          }
        }
      }
    }
  } else if (object_type == SWITCH_OBJECT_TYPE_ACL_TABLE) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_PORT,
               "bind_point_attach_flag:{} for ACL Table:{}",
               new_attach_flag,
               acl_handle);
    status |= switch_store::v_get(
        acl_handle, SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH, curr_attach_flag);
    if (curr_attach_flag ^ new_attach_flag) {
      uint64_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
      switch_enum_t enum_type;
      status |= switch_store::v_get(
          acl_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
      acl_type = enum_type.enumdata;
      if (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_DTEL) {
        switch_object_id_t ipv4_table_handle = {0};
        switch_object_id_t ipv6_table_handle = {0};
        // get the inner ipv4 an dinner ipv6 handles and mark the flag
        status |=
            switch_store::v_get(acl_handle,
                                SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE,
                                ipv4_table_handle);
        status |=
            switch_store::v_get(acl_handle,
                                SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE,
                                ipv6_table_handle);
        status |=
            set_bind_point_attach_flag(ipv4_table_handle, new_attach_flag);
        status |=
            set_bind_point_attach_flag(ipv6_table_handle, new_attach_flag);
      }
      status |= switch_store::v_set(
          acl_handle, SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH, new_attach_flag);
    }
  }

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_PORT,
        "{} Setting bind_point_attach flag failed for acl handle {} status {}",
        __func__,
        acl_handle,
        status);
  }
  return status;
}

switch_acl_label_t compute_bind_label(const switch_object_id_t handle,
                                      const switch_object_id_t acl_handle) {
  (void)handle;
  switch_acl_label_t label = 0;
  switch_object_type_t ot = 0;

  if (acl_handle.data == 0) return 0;

  ot = switch_store::object_type_query(acl_handle);

  if (ot == SWITCH_OBJECT_TYPE_ACL_GROUP) {
    switch_store::v_get(acl_handle, SWITCH_ACL_GROUP_ATTR_ACL_LABEL, label);
  } else if (ot == SWITCH_OBJECT_TYPE_ACL_TABLE) {
    switch_store::v_get(acl_handle, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, label);
  }
  return label;
}

switch_status_t lpm_trie_lookup(switch_object_id_t vrf_handle,
                                const switch_ip_address_t &addr,
                                switch_object_id_t &route_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_lpm_trie_t *lpm_trie;
  const uint8_t *prefix_ptr = NULL;
  switch_ip4_t ip4_addr;

  if (vrf_handle.data == 0) return SWITCH_STATUS_INVALID_PARAMETER;

  if (addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    lpm_trie = SWITCH_CONTEXT.ipv4_tries[vrf_handle.data];
    ip4_addr = htonl(addr.ip4);
    prefix_ptr = reinterpret_cast<uint8_t *>(&ip4_addr);
  } else if (addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    lpm_trie = SWITCH_CONTEXT.ipv6_tries[vrf_handle.data];
    prefix_ptr = reinterpret_cast<const uint8_t *>(addr.ip6);
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}:{}: Invalid IP address family {}",
               __func__,
               __LINE__,
               addr.addr_family);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status = switch_lpm_trie_lookup(lpm_trie, prefix_ptr, &(route_handle.data));
  if ((status != SWITCH_STATUS_SUCCESS) &&
      (status != SWITCH_STATUS_ITEM_NOT_FOUND)) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "route trie lookup failed "
               "lpm trie lookup failed {}",
               switch_error_to_string(status));
    return status;
  }

  return status;
}

switch_status_t compute_outer_nexthop(
    switch_object_id_t tunnel_dest_ip_handle,
    switch_object_id_t &outer_nexthop_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {0}, uvrf_handle = {0}, route_handle = {0};
  switch_ip_prefix_t ip_prefix = {};

  status |= switch_store::v_get(
      tunnel_dest_ip_handle, SWITCH_TUNNEL_DEST_IP_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(tunnel_dest_ip_handle,
                                SWITCH_TUNNEL_DEST_IP_ATTR_UNDERLAY_VRF_HANDLE,
                                uvrf_handle);
  status |= switch_store::v_get(tunnel_dest_ip_handle,
                                SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP,
                                ip_prefix.addr);
  if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    ip_prefix.len = sizeof(ip_prefix.addr.ip4) * 8;
  } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    ip_prefix.len = sizeof(ip_prefix.addr.ip6) * 8;
  } else {
    status |= SWITCH_STATUS_INVALID_PARAMETER;
  }

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP,
               "{}.{}: failed to lookup tunnel_dest_ip attributes for handle "
               "{:#x} : {}",
               __func__,
               __LINE__,
               tunnel_dest_ip_handle.data,
               switch_error_to_string(status));
    return status;
  }

  // lookup the host route
  std::set<attr_w> lookup_attrs;
  lookup_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_DEVICE, device_handle));
  lookup_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE, uvrf_handle));
  lookup_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix));
  bool nbr_installed_route = true;
  lookup_attrs.insert(
      smi::attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, nbr_installed_route));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_ROUTE, lookup_attrs, route_handle);
  // if not found, lookup the lpm route
  if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    status = lpm_trie_lookup(uvrf_handle, ip_prefix.addr, route_handle);
  }
  if (status == SWITCH_STATUS_SUCCESS && route_handle.data == 0) {
    status = SWITCH_STATUS_ITEM_NOT_FOUND;
  }
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP,
        "{}:{}: failed to find route for tunnel dest ip {} in vrf {:#x} : {}",
        __func__,
        __LINE__,
        ip_prefix.addr,
        uvrf_handle.data,
        switch_error_to_string(status));
    return status;
  }

  // From the nexthop_handle of the route, get the port_lag_index
  status |= switch_store::v_get(
      route_handle, SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, outer_nexthop_handle);
  return status;
}

uint32_t system_acl_priority(system_acl_default_internal_types_t acl_type) {
  uint32_t acl_high_priority_base = SWITCH_INTERNAL_ACL_HIGH_PRIO_START;
  uint32_t acl_lo_priority_base = SWITCH_INTERNAL_ACL_LOW_PRIO_START;
  bool high_prio = false;
  uint32_t final_priority = 0;

  switch (acl_type) {
    // High priority ACLs
    case SYSTEM_ACL_TYPE_HOSTIF_TRAP:
      high_prio = true;
      break;
    default:
      high_prio = false;
      break;
  }

  final_priority = high_prio ? (acl_high_priority_base + acl_type)
                             : (acl_lo_priority_base + acl_type);

  switch_log(SWITCH_API_LEVEL_INFO,
             SWITCH_OBJECT_TYPE_NONE,
             "Final priority for internal ACL {} is {}",
             acl_type,
             final_priority);

  return final_priority;
}

uint32_t port_mirror_metadata_len_get(bool is_direction_egress) {
  if (bf_lld_dev_is_tof1(0)) {
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      if (is_direction_egress) {
        // 4 bytes of timestamp + 1 byte of pkt_src +
        // + 1 byte of mirror_type + 1 byte of session_id
        // + 2 bytes of ingress_port
        return 9;
      } else {
        // 4 bytes of timestamp + 1 byte of pkt_src +
        // + 1 byte of mirror_type + 1 byte of session_id +
        // + 2 bytes of ingress_port
        // + 17 byte o fp_bridged_md
        // 9 + 17 = 26
        return 26;
      }
    } else {
      // 4 bytes of timestamp + 1 byte of pkt_src +
      // + 1 byte of mirror_type + 2 byte of session_id +
      // + 2 bytes of ingress_port
      // 4 + 1 + 1 + 2 + 2 = 10
      return 10;
    }

  } else {
    if ((feature::is_feature_set(SWITCH_FEATURE_INT_V2)) == true) {
      return 11;
    } else {
      return 9;
    }
  }
}

// checks if nexthop resolution feature in on globally
bool is_nexthop_resolution_feature_on(switch_object_id_t dev_hdl) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool feature_on = true;
  status |= switch_store::v_get(
      dev_hdl, SWITCH_DEVICE_ATTR_NEXTHOP_RESOLUTION, feature_on);
  return feature_on;
}

// return port handle of all valid vlan members
std::vector<switch_object_id_t> get_untagged_vlan_member_ports(
    switch_object_id_t vlan_handle) {
  std::vector<switch_object_id_t> ret;
  if (vlan_handle.data != 0) {
    std::vector<switch_object_id_t> vlan_members;
    const auto &vlan_members_objects = switch_store::get_object_references(
        vlan_handle, SWITCH_OBJECT_TYPE_VLAN_MEMBER);
    for (const auto &item : vlan_members_objects) {
      vlan_members.push_back(item.oid);
    }
    for (const auto &vlan_member : vlan_members) {
      switch_enum_t tag_mode = {};
      switch_store::v_get(
          vlan_member, SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE, tag_mode);
      if (tag_mode.enumdata == SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_UNTAGGED) {
        switch_object_id_t port_lag_handle = {};
        switch_store::v_get(vlan_member,
                            SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE,
                            port_lag_handle);
        const switch_object_type_t ot =
            switch_store::object_type_query(port_lag_handle);
        if (ot == SWITCH_OBJECT_TYPE_PORT || ot == SWITCH_OBJECT_TYPE_LAG) {
          ret.push_back(port_lag_handle);
        }
      }
    }
  }
  return ret;
}

uint16_t compute_pre_mgid(const switch_object_id_t oid) {
  uint16_t mgid = switch_store::handle_to_id(oid);
  switch_object_type_t ot = switch_store::object_type_query(oid);

  if (ot == SWITCH_OBJECT_TYPE_VLAN) {
    return mgid;
  } else if (ot == SWITCH_OBJECT_TYPE_IPMC_GROUP) {
    return (mgid | (1 << TOFINO_MGID_WIDTH));
  } else if (ot == SWITCH_OBJECT_TYPE_L2MC_BRIDGE) {
    return (mgid | (1 << (TOFINO_MGID_WIDTH - 8)));
  }
  return 0;
}

/**
 * @brief Compute RID value for PRE programing
 *        in case of VLAN or RIF take BD number
 *        in other cases allocate unique values by rid_manager (note that
 *        rid_allocate() takes into account the numbers reserved for BDs)
 *
 * @param oid
 * @return uint16_t RID value
 */
uint16_t compute_rid(const switch_object_id_t oid) {
  uint16_t rid = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t ot = switch_store::object_type_query(oid);

  // for VLAN RID must be equal to BD which is equal to VLAN ID
  if (ot == SWITCH_OBJECT_TYPE_VLAN) {
    uint16_t vlan_id;
    switch_store::v_get(oid, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);

    return vlan_id;
  }

  // for RIF RID must be equal to BD
  if (ot == SWITCH_OBJECT_TYPE_RIF) {
    switch_enum_t rif_type;
    switch_object_id_t bd_handle = {};

    switch_store::v_get(oid, SWITCH_RIF_ATTR_TYPE, rif_type);
    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
        rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      status |= find_auto_oid(oid, SWITCH_OBJECT_TYPE_BD, bd_handle);

    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      switch_object_id_t vlan_handle = {};
      status |=
          switch_store::v_get(oid, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

    } else {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                 "{}.{}: Unsupported RIF type",
                 __func__,
                 __LINE__);
      return 0;
    }
    return compute_bd(bd_handle);
  }

  // if the object is just created allocate RID value for it
  if (oid.data == 0) {
    return SWITCH_CONTEXT.rid_allocate();
  }

  // if the object exists read rid attribute of a proper object
  switch (ot) {
    case SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION: {
      status = switch_store::v_get(
          oid, SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_RID, rid);
    } break;
    case SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL: {
      status = switch_store::v_get(
          oid, SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_RID, rid);
    } break;
    default: {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ot,
                 "{}.{}: Object of type {} not supported for RID computation",
                 __func__,
                 __LINE__,
                 switch_store::object_name_get_from_type(ot));
      status = SWITCH_STATUS_FEATURE_NOT_SUPPORTED;
    }
  }

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               ot,
               "Cannot find rid for object: {} oid: {}",
               switch_store::object_name_get_from_type(ot),
               oid.data);
  } else {
    if (switch_store::smiContext::context().in_warm_init()) {
      SWITCH_CONTEXT.rid_reserve(rid);
    }
  }

  return rid;
}

switch_status_t vrf_ttl_action_to_switch_packet_action(
    switch_enum_t vrf_ttl_action,
    switch_packet_action_t &packet_action,
    bool &is_valid) {
  is_valid = true;
  switch (vrf_ttl_action.enumdata) {
    case SWITCH_VRF_ATTR_TTL_ACTION_FORWARD:
    case SWITCH_VRF_ATTR_TTL_ACTION_TRANSIT:
      packet_action = SWITCH_PACKET_ACTION_PERMIT;
      break;
    case SWITCH_VRF_ATTR_TTL_ACTION_DROP:
    case SWITCH_VRF_ATTR_TTL_ACTION_DENY:
      packet_action = SWITCH_PACKET_ACTION_DROP;
      break;
    case SWITCH_VRF_ATTR_TTL_ACTION_TRAP:
      packet_action = SWITCH_PACKET_ACTION_TRAP;
      break;
    case SWITCH_VRF_ATTR_TTL_ACTION_COPY:
    case SWITCH_VRF_ATTR_TTL_ACTION_LOG:
      packet_action = SWITCH_PACKET_ACTION_COPY;
      break;
    case SWITCH_VRF_ATTR_TTL_ACTION_NONE:
      is_valid = false;
      break;
    default:
      return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t vrf_ip_options_action_to_switch_packet_action(
    switch_enum_t vrf_ip_options_action,
    switch_packet_action_t &packet_action) {
  switch (vrf_ip_options_action.enumdata) {
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_FORWARD:
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_TRANSIT:
      packet_action = SWITCH_PACKET_ACTION_PERMIT;
      break;
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_DROP:
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_DENY:
      packet_action = SWITCH_PACKET_ACTION_DROP;
      break;
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_TRAP:
      packet_action = SWITCH_PACKET_ACTION_TRAP;
      break;
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_COPY:
    case SWITCH_VRF_ATTR_IP_OPTIONS_ACTION_LOG:
      packet_action = SWITCH_PACKET_ACTION_COPY;
      break;
    default:
      return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t vrf_unknown_l3_mcast_action_to_switch_packet_action(
    switch_enum_t vrf_unknown_l3_mcast_action,
    switch_packet_action_t &packet_action) {
  switch (vrf_unknown_l3_mcast_action.enumdata) {
    case SWITCH_VRF_ATTR_UNKNOWN_L3_MCAST_ACTION_DROP:
    case SWITCH_VRF_ATTR_UNKNOWN_L3_MCAST_ACTION_DENY:
      packet_action = SWITCH_PACKET_ACTION_DROP;
      break;
    case SWITCH_VRF_ATTR_UNKNOWN_L3_MCAST_ACTION_TRAP:
      packet_action = SWITCH_PACKET_ACTION_TRAP;
      break;
    default:
      return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t port_drop_reason_to_switch_counter(
    switch_drop_reason_t drop_reason, uint16_t *counter_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  // It is required that drop_reason belongs to only one SWITCH counter
  // Therefore the SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS and
  // SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS counter may not be handled here.
  switch (drop_reason) {
    case SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_VLAN_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SMAC_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_SRC_MAC_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SMAC_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_DMAC_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_DST_MAC_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_DMAC_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SMAC_MULTICAST_DISCARDS;
      break;
    case SWITCH_DROP_REASON_SRC_MAC_MULTICAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SMAC_MULTICAST_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SAME_MAC_CHECK_DISCARDS;
      break;
    case SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_UC_DIP_MC_DMAC_DISCARDS;
      break;
    case SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_BC_DIP_MC_DMAC_DISCARDS;
      break;
    case SWITCH_DROP_REASON_SAME_IFINDEX:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SAME_IFINDEX_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST:
      *counter_id =
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_SRC_MULTICAST_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_SRC_MULTICAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_MULTICAST_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_TTL_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_TTL_ZERO:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_TTL_ZERO_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SIP_LOOPBACK_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_DIP_LOOPBACK_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_IHL_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_IHL_INVALID:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_IHL_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID:
      *counter_id =
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_VERSION_INVALID_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_VERSION_INVALID:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_VERSION_INVALID_DISCARDS;
      break;
    case SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM:
      *counter_id =
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_CHECKSUM_INVALID_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_INVALID_CHECKSUM:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_CHECKSUM_INVALID_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_DST_LINK_LOCAL:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_DST_LINK_LOCAL_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_LINK_LOCAL_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_SRC_CLASS_E:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_CLASS_E_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SIP_UNSPECIFIED_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_DST_UNSPECIFIED:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_DIP_UNSPECIFIED_DISCARDS;
      break;
    case SWITCH_DROP_REASON_NON_IP_ROUTER_MAC:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_NON_IP_ROUTER_MAC_DISCARDS;
      break;
    case SWITCH_DROP_REASON_NEXTHOP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_NEXTHOP_DISCARDS;
      break;
    case SWITCH_DROP_REASON_MPLS_LABEL_DROP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_MPLS_LOOKUP_MISS_DISCARD;
      break;
    case SWITCH_DROP_REASON_SRV6_MY_SID_DROP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SRV6_MY_SID_DISCARDS;
      break;
    case SWITCH_DROP_REASON_ACL_DENY:
    case SWITCH_DROP_REASON_ACL_DROP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_ACL_DENY_DISCARDS;
      break;
    case SWITCH_DROP_REASON_RACL_DENY:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_RACL_DENY_DISCARDS;
      break;
    case SWITCH_DROP_REASON_INGRESS_ACL_METER:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_ACL_METER_DISCARDS;
      break;
    case SWITCH_DROP_REASON_INGRESS_PORT_METER:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_PORT_METER_DISCARDS;
      break;
    case SWITCH_DROP_REASON_L3_IPV4_DISABLE:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV4_DISABLE_DISCARDS;
      break;
    case SWITCH_DROP_REASON_L3_IPV6_DISABLE:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV6_DISABLE_DISCARDS;
      break;
    case SWITCH_DROP_REASON_MPLS_DISABLE:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_MPLS_DISABLE_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_LPM4_MISS:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM4_MISS_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_LPM6_MISS:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM6_MISS_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IP_BLACKHOLE_ROUTE_DISCARDS;
      break;
    case SWITCH_DROP_REASON_L3_PORT_RMAC_MISS:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L3_PORT_RMAC_MISS_DISCARDS;
      break;
    case SWITCH_DROP_REASON_DMAC_RESERVED:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_DMAC_RESERVED_DISCARDS;
      break;
    case SWITCH_DROP_REASON_NON_ROUTABLE:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_NON_ROUTABLE_DISCARDS;
      break;
    case SWITCH_DROP_REASON_L2_MISS_UNICAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_UNICAST;
      break;
    case SWITCH_DROP_REASON_L2_MISS_MULTICAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_MULTICAST;
      break;
    case SWITCH_DROP_REASON_L2_MISS_BROADCAST:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_BROADCAST;
      break;
    case SWITCH_DROP_REASON_SIP_BC:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_SIP_BC_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IPV6_MC_SCOPE0:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IPV6_MC_SCOPE0_DISCARD;
      break;
    case SWITCH_DROP_REASON_IPV6_MC_SCOPE1:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_IPV6_MC_SCOPE1_DISCARD;
      break;
    case SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_STP_STATE_BLOCKING_DISCARDS;
      break;
    case SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_MC_DMAC_MISMATCH;
      break;
    case SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_IN_L3_EGRESS_LINK_DOWN_DISCARDS;
      break;

    // EGRESS
    case SWITCH_DROP_REASON_EGRESS_STP_STATE_BLOCKING:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_STP_STATE_BLOCKING_DISCARDS;
      break;
    case SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_STORM_PFC_WD_DISCARDS;
      break;
    case SWITCH_DROP_REASON_MTU_CHECK_FAIL:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_MTU_CHECK_FAIL_DISCARDS;
      break;
    case SWITCH_DROP_REASON_EGRESS_ACL_DROP:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_EGRESS_ACL_DENY_DISCARDS;
      break;
    case SWITCH_DROP_REASON_EGRESS_ACL_METER:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_EGRESS_ACL_METER_DISCARDS;
      break;
    case SWITCH_DROP_REASON_EGRESS_PORT_METER:
      *counter_id = SWITCH_PORT_COUNTER_ID_IF_OUT_EGRESS_PORT_METER_DISCARDS;
      break;
    default:
      status = SWITCH_STATUS_INVALID_PARAMETER;
      break;
  }

  return status;
}

uint8_t compute_sflow_id(switch_object_id_t port_handle) {
  uint16_t dev_id = 0;
  switch_object_id_t device_handle = {0};
  switch_object_id_t sflow_handle = {0};
  uint16_t local_port_start = SWITCH_MAX_RECIRC_PORTS;

  switch_store::v_get(
      port_handle, SWITCH_PORT_ATTR_INGRESS_SFLOW_HANDLE, sflow_handle);

  if (sflow_handle == 0) {
    return SWITCH_SFLOW_INVALID_ID;
  }

  switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEVICE, device_handle);
  switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);

  bf_dev_family_t dev_family =
      bf_dev_type_to_family(lld_sku_get_dev_type(dev_id));

  if (dev_family == BF_DEV_FAMILY_TOFINO) {
    local_port_start = 0;
  }

  switch_enum_t mode = {};
  switch_store::v_get(sflow_handle, SWITCH_SFLOW_SESSION_ATTR_MODE, mode);
  // first 64 are reserved for exclusive and shared >= 64
  if (mode.enumdata == SWITCH_SFLOW_SESSION_ATTR_MODE_EXCLUSIVE) {
    uint16_t dev_port = 0;
    switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    return DEV_PORT_TO_LOCAL_PORT(dev_port) - local_port_start;
  } else {
    // values [0-63] are reserved for exclusive mode,
    // so fo shared mode values we have [64-254(max)]
    // For valid handle handle_to_id return > 0,
    // so as offset 63 should be used to not skip value 64
    return SWITCH_MAX_PORT_PER_PIPE + switch_store::handle_to_id(sflow_handle) -
           1;
  }
}

}  // namespace smi
