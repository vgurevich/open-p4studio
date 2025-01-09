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


#include "switch_tna/acl.h"
#include <arpa/inet.h>
#include <memory>
#include <vector>
#include <utility>
#include <set>
#include <map>
#include <unordered_map>
#include "common/hostif.h"
#include "./utils.h"
#include "./p4_16_types.h"

namespace smi {
using ::smi::logging::switch_log;

static inline bf_rt_id_t get_smi_id(uint64_t acl_type,
                                    uint64_t dir,
                                    int acl_entry_attr) {
  bf_rt_id_t smi_id = IdMap::instance()->get_id(acl_type, dir, acl_entry_attr);
  return smi_id;
}

bool acl_using_acl2_profile() {
  // For now, using below checks to figure out if acl2.p4 is in use
  //  - actions transit/deny
  if (feature::is_feature_set(
          SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION) ||
      feature::is_feature_set(SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION)) {
    return true;
  }
  return false;
}

// Map of 16-bit ether_type to a 4-bit etype_label
// Multiple ACL entries can share the same ether type
// So a refcount is kept as second in pair to keep track.
std::unordered_map<uint16_t, std::pair<uint8_t, uint16_t>> eth_type_label_map;

switch_status_t etype_label_allocate(const uint16_t eth_type,
                                     uint8_t &etype_label) {
  auto it = eth_type_label_map.find(eth_type);
  if (it == eth_type_label_map.end()) {
    etype_label = SWITCH_CONTEXT.etype_index_allocate();
    // refcount = 1
    eth_type_label_map[eth_type] = std::make_pair(etype_label, 1);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Allocated eth_type {} label {}",
               __func__,
               __LINE__,
               eth_type,
               etype_label);

  } else {
    // increment refcount
    it->second.second++;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Incremented eth_type {} refcount {}",
               __func__,
               __LINE__,
               eth_type,
               it->second.second);
    etype_label = eth_type_label_map[eth_type].first;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t etype_label_reserve(const uint16_t eth_type,
                                    const uint8_t etype_label) {
  auto it = eth_type_label_map.find(eth_type);
  if (it == eth_type_label_map.end()) {
    SWITCH_CONTEXT.etype_index_reserve(etype_label);
    // refcount = 1
    eth_type_label_map[eth_type] = std::make_pair(etype_label, 1);
  } else {
    // increment refcount
    it->second.second++;
  }
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Reserved eth_type {} label {}",
             __func__,
             __LINE__,
             eth_type,
             etype_label);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t etype_label_release(const uint8_t etype_label) {
  for (auto it = eth_type_label_map.begin(); it != eth_type_label_map.end();
       it++) {
    if (it->second.first == etype_label) {
      it->second.second--;
      if (it->second.second == 0) {
        SWITCH_CONTEXT.etype_index_release(etype_label);
        eth_type_label_map.erase(it);
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}:{}: Released label {}",
                   __func__,
                   __LINE__,
                   etype_label);
      }
      return SWITCH_STATUS_SUCCESS;
    }
  }
  // Trying to release a label that does not exist
  return SWITCH_STATUS_ITEM_NOT_FOUND;
}

switch_status_t etype_label_get_refcount(const uint16_t eth_type,
                                         uint16_t &refcount) {
  auto it = eth_type_label_map.find(eth_type);
  if (it == eth_type_label_map.end()) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
  refcount = it->second.second;
  return SWITCH_STATUS_SUCCESS;
}

/**
 * @brief get the actual table type
 *
 * @param[in] parent this is switch_object_id_t of acl_entry type
 * @param[in] acl_type acl table type (switch_acl_table_attr_type)
 * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
 *
 * @return table_type
 */
uint64_t get_smi_acl_type(switch_object_id_t parent,
                          uint64_t acl_type,
                          uint64_t dir) {
  bool mirrorv6 = true;
  uint8_t icmp_mask = 0;
  uint16_t eth_type = 0;
  uint16_t eth_type_mask = 0;
  switch_ip_address_t ip_addr;

  /* Currently, shared P4 code supports combined IPv4/IPv6 mirror ACL table
   * on ingress but two separate IPv4 and IPv6 tables on egress.
   * From the application perspective (SONiC) it's expected that
   * the tables on ingress and egress paths are identical - either combined or
   * separated. In case the application works in the combined mode
   * (SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR), the following logic selects either
   * IPv4 or IPv6 egress table based on the provided ACL rule's match key.
   */
  if (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR &&
      dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS &&
      !acl_using_acl2_profile()) {
    do {
      switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, eth_type);
      switch_store::v_get(
          parent, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK, eth_type_mask);
      if (eth_type_mask == 0xFFFF && eth_type == 0x86DD) break;

      switch_store::v_get(
          parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE_MASK, icmp_mask);
      if (icmp_mask) break;

      switch_store::v_get(
          parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE_MASK, icmp_mask);
      if (icmp_mask) break;

      switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_SRC_IP, ip_addr);
      if (ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) break;

      switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DST_IP, ip_addr);
      if (ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) break;

      mirrorv6 = false;
    } while (0);

    // Use ip mirror acl tables if exists in a profile, else use ip acl tables
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_MIRROR_ACL)) {
      acl_type = (mirrorv6 ? SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR
                           : SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR);
    } else {
      acl_type = (mirrorv6 ? SWITCH_ACL_TABLE_ATTR_TYPE_IPV6
                           : SWITCH_ACL_TABLE_ATTR_TYPE_IPV4);
    }
  }
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_TABLE,
             "{}:{} acl table type: {}",
             __func__,
             __LINE__,
             acl_type);
  return acl_type;
}

const uint8_t ACL_SAMPLE_NULL_ID = 0xFF;

// These will be combined to a uint16 map with dst in MSB
std::map<switch_acl_range_attr_type, std::bitset<8>> global_acl_range_label = {
    {SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT, std::bitset<8>{}},
    {SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT, std::bitset<8>{}}};
std::vector<uint16_t> l4_ingress_src_port_entries(65536, 0);
std::vector<uint16_t> l4_ingress_dst_port_entries(65536, 0);
std::vector<uint16_t> l4_egress_src_port_entries(65536, 0);
std::vector<uint16_t> l4_egress_dst_port_entries(65536, 0);

// Create port_group_index for the first rule.
switch_status_t acl_entry_compute_port_group(
    const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  std::set<switch_object_id_t> group_member_handles;
  std::set<switch_object_id_t> port_handles;
  std::set<switch_object_id_t> lag_handles;
  std::set<switch_object_id_t> port_lag_handles;

  uint64_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  std::vector<switch_object_id_t> acl_entry_handles;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, acl_entry_handles);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Number of ACL entries bound to ACL table {}",
             acl_entry_handles.size());
  if (acl_entry_handles.size() > 0) {
    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_ENTRY,
        "Not First rule: Dont compute port_group_index for the ACL table");
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "First rule: Compute port_group_index for the ACL table");
  switch_enum_t enum_type;
  status =
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;

  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  switch_store::referencing_set_get(
      table_handle, SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, group_member_handles);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Number of ACL group members bound to ACL table {}",
             group_member_handles.size());

  for (auto group_member : group_member_handles) {
    switch_object_id_t group_handle = {0};
    std::set<switch_object_id_t> group_port_handles;
    std::set<switch_object_id_t> group_lag_handles;

    status = switch_store::v_get(group_member,
                                 SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                 group_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_ENTRY,
        "compute_port_Group: Number of port handles bound to ACL table {}",
        group_port_handles.size());

    for (auto port : group_port_handles) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Port handles bound to acl {}",
                 port);
      port_handles.insert(port);
      port_lag_handles.insert(port);
    }
    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_LAG, group_lag_handles);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Number of LAG handles bound to ACL table {}",
               group_lag_handles.size());

    for (auto lag : group_lag_handles) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "LAG handles bound to acl {}",
                 lag);
      lag_handles.insert(lag);
      port_lag_handles.insert(lag);
    }
  }

  uint8_t group_index = 0;

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Total port/LAG handles' size {}",
             port_lag_handles.size());

  // When port_lag_handles size is 1, need to use port_lag_index in the rule.
  if (port_lag_handles.size() > 1) {
    PortGroupManager::instance()->port_group_allocate(
        acl_type, dir, port_lag_handles, group_index);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "port group index {}",
               group_index);

    if (group_index > SWITCH_ACL_PORT_GROUP_MAX) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Port group index more than 8, release the index");
      PortGroupManager::instance()->port_group_release(
          acl_type, dir, group_index);
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }
    status |= switch_store::v_set(
        table_handle, SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX, true);
    status |= switch_store::v_set(
        table_handle, SWITCH_ACL_TABLE_ATTR_PORT_GROUP_INDEX, group_index);
  } else {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Port group compute ignored, use port_lag_index");
    status |= switch_store::v_set(
        table_handle, SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX, false);
    if (port_lag_handles.size() == 1) {
      switch_object_id_t port_handle = *std::next(port_lag_handles.begin(), 0);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Use port/lag handle {}",
                 port_handle);
      status |= switch_store::v_set(
          table_handle, SWITCH_ACL_TABLE_ATTR_PORT_LAG_HANDLE, port_handle);
    }
  }

  return status;
}

switch_status_t update_lag_members_port_group_index(
    const switch_object_id_t lag, const uint32_t port_group_value) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  std::vector<switch_object_id_t> lag_members;
  status |= switch_store::v_get(lag, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
  switch_log(
      SWITCH_API_LEVEL_DEBUG,
      SWITCH_OBJECT_TYPE_ACL_ENTRY,
      "update LAG_mbrs {} Program port in hardware with Port group index ",
      lag);
  for (auto const mbr : lag_members) {
    switch_object_id_t mbr_port_handle = {0};
    status |= switch_store::v_get(
        mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
    status |= switch_store::v_set(mbr_port_handle,
                                  SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                  port_group_value);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "port_group_index set failed for LAG mbr {}",
                 mbr_port_handle);
    }
  }
  return status;
}

// When ACL group member is deleted, update the PORT/LAG with the new
// port_group_index.
switch_status_t update_acl_port_group_index(
    const switch_object_id_t acl_group_member_handle,
    const uint64_t acl_type,
    const uint64_t dir) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t group_handle = {0};
  switch_object_id_t table_handle = {0};
  std::set<switch_object_id_t> group_port_handles;
  std::set<switch_object_id_t> group_lag_handles;
  uint32_t port_group_value = 0;
  bool use_port_group_index = false;

  switch_store::v_get(acl_group_member_handle,
                      SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                      table_handle);

  status |= switch_store::v_get(table_handle,
                                SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                                use_port_group_index);
  if (use_port_group_index == false) return status;

  uint32_t ref_count = 0;
  uint8_t group_index = 0;
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_PORT_GROUP_INDEX, group_index);
  status |= PortGroupManager::instance()->port_group_count(
      acl_type, dir, group_index, ref_count);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Delete member - acl_type {}, group_index {}, ref_count {}",
             acl_type,
             group_index,
             ref_count);

  if (ref_count > 1) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "port_group index is referenced by other tables, dont update "
               "bind point");
    return status;
  }

  switch_store::v_get(table_handle,
                      SWITCH_ACL_TABLE_ATTR_ACL_TYPE_PORT_GROUP_VALUE,
                      port_group_value);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Delete member - port label value from table {}",
             port_group_value);

  status = switch_store::v_get(acl_group_member_handle,
                               SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                               group_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  switch_store::referencing_set_get(
      group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Delete member - Number of port handles bound to ACL table {}",
             group_port_handles.size());
  for (auto port : group_port_handles) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Delete member - Port handles bound to acl {}",
               port);
    // Retrieve the existing port group index and update with the latest one.
    uint32_t current_port_group_value = 0;
    status |= switch_store::v_get(port,
                                  SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                  current_port_group_value);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Delete member - Current port label value {}",
               current_port_group_value);
    uint32_t new_port_group_value =
        current_port_group_value & ~port_group_value;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Delete member - New port label value {}",
               new_port_group_value);
    status |= switch_store::v_set(
        port, SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX, new_port_group_value);
  }

  switch_store::referencing_set_get(
      group_handle, SWITCH_OBJECT_TYPE_LAG, group_lag_handles);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "Delete - Number of LAG handles bound to ACL table {}",
             group_lag_handles.size());
  for (auto lag : group_lag_handles) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "LAG handles bound to acl {}",
               lag);
    // Retrieve the existing port group index and update with the latest one.
    uint32_t current_port_group_value = 0;
    status |= switch_store::v_get(lag,
                                  SWITCH_LAG_ATTR_INGRESS_PORT_GROUP_INDEX,
                                  current_port_group_value);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Current LAG label value {}",
               current_port_group_value);
    uint32_t new_port_group_value =
        current_port_group_value & ~port_group_value;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "New port label value {}",
               new_port_group_value);
    status |= switch_store::v_set(
        lag, SWITCH_LAG_ATTR_INGRESS_PORT_GROUP_INDEX, new_port_group_value);
    status |= update_lag_members_port_group_index(lag, new_port_group_value);
  }
  return status;
}

switch_status_t switch_acl_update_port_group_index(
    const switch_object_id_t acl_group_member_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle;
  uint32_t port_group_value = 0;
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;
  bool use_port_group_index = false;

  switch_store::v_get(acl_group_member_handle,
                      SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                      table_handle);

  status |= switch_store::v_get(table_handle,
                                SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                                use_port_group_index);
  if (use_port_group_index == false) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "switch_acl_update_port_group_index, port_group index not used, "
               "skip update");
    return status;
  }

  switch_enum_t enum_type;
  status =
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;

  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  // Currently port group index is only for ingress direction and V4/V6/Mirror
  // ACL types.
  if ((dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) &&
      ((acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4) ||
       (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_MAC) ||
       (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6) ||
       (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR) ||
       (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR) ||
       (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR))) {
    switch_object_id_t group_handle = {0};
    std::set<switch_object_id_t> group_port_handles;
    std::set<switch_object_id_t> group_lag_handles;

    status = switch_store::v_get(acl_group_member_handle,
                                 SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                 group_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "switch_acl_update_port_group_index: Number of port handles "
               "bound to ACL table {}",
               group_port_handles.size());

    switch_store::v_get(table_handle,
                        SWITCH_ACL_TABLE_ATTR_ACL_TYPE_PORT_GROUP_VALUE,
                        port_group_value);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "port label value from table {}",
               port_group_value);
    for (auto port : group_port_handles) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Port handles bound to acl {}",
                 port);
      // Retrieve the existing port group index and update with the latest one.
      uint32_t current_port_group_value = 0;
      status |= switch_store::v_get(port,
                                    SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                    current_port_group_value);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Current port label value {}",
                 current_port_group_value);
      uint32_t new_port_group_value =
          current_port_group_value | port_group_value;
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "New port label value {}",
                 new_port_group_value);
      status |= switch_store::v_set(port,
                                    SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                    new_port_group_value);
      uint32_t check_port_group_value = 0;
      status |= switch_store::v_get(port,
                                    SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                    check_port_group_value);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Port {} Double check New port label value {}",
                 port,
                 check_port_group_value);
    }
    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_LAG, group_lag_handles);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "Number of LAG handles bound to ACL table {}",
               group_lag_handles.size());

    for (auto lag : group_lag_handles) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "LAG handles bound to acl {}",
                 lag);
      // Retrieve the existing port group index and update with the latest one.
      uint32_t current_port_group_value = 0;
      status |= switch_store::v_get(lag,
                                    SWITCH_LAG_ATTR_INGRESS_PORT_GROUP_INDEX,
                                    current_port_group_value);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Current LAG port_group_index value {} to lag {}",
                 current_port_group_value,
                 lag);
      uint32_t new_port_group_value =
          current_port_group_value | port_group_value;
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "New LAG port_group_index value {} to lag {}",
                 new_port_group_value,
                 lag);
      status |= switch_store::v_set(
          lag, SWITCH_LAG_ATTR_INGRESS_PORT_GROUP_INDEX, new_port_group_value);
      status |= update_lag_members_port_group_index(lag, new_port_group_value);
    }
  }
  return status;
}

switch_status_t release_acl_table_port_group(
    const switch_object_id_t acl_table) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;
  uint8_t group_index = 0;
  bool use_port_group_index = false;

  status |= switch_store::v_get(acl_table,
                                SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                                use_port_group_index);

  if (use_port_group_index == false) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "release_acl_table_port_group, port_group index not used, skip "
               "release");
    return status;
  }

  switch_enum_t enum_type;
  status =
      switch_store::v_get(acl_table, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;

  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_PORT_GROUP_INDEX, group_index);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "port group index release {}",
             group_index);
  PortGroupManager::instance()->port_group_release(acl_type, dir, group_index);
  return status;
}

switch_status_t get_meter_attrs_per_acl_entry(
    const switch_object_id_t meter_handle,
    uint64_t &cir_bps,
    uint64_t &pir_bps,
    uint64_t &cburst_bytes,
    uint64_t &pburst_bytes) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_CIR, cir_bps);
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_PIR, pir_bps);
  status |=
      switch_store::v_get(meter_handle, SWITCH_METER_ATTR_CBS, cburst_bytes);
  status |=
      switch_store::v_get(meter_handle, SWITCH_METER_ATTR_PBS, pburst_bytes);

  return status;
}

class acl_entry_object {
 public:
  explicit acl_entry_object(const switch_object_id_t parent);
  ~acl_entry_object() {}

  uint16_t eth_type;               // IP, MAC, MIRROR, MAC_QOS
  uint16_t eth_type_mask;          // IP, MAC, MIRROR, MAC_QOS
  switch_mac_addr_t src_mac;       // MAC, MAC_QOS
  switch_mac_addr_t src_mac_mask;  // MAC, MAC_QOS
  switch_mac_addr_t dst_mac;       // MAC
  switch_mac_addr_t dst_mac_mask;  // MAC
  switch_ip_address_t src_ip;      // IP, RACL, MIRROR, IP_QOS
  uint32_t src_ipv6_word3;
  uint32_t src_ipv6_word2;
  switch_ip_address_t src_ip_mask;  // IP, RACL, MIRROR, IP_QOS
  uint32_t src_ip_mask_word3;
  uint32_t src_ip_mask_word2;
  switch_ip_address_t dst_ip;  // IP, RACL, MIRROR, IP_QOS
  uint32_t dst_ipv6_word3;
  uint32_t dst_ipv6_word2;
  switch_ip_address_t dst_ip_mask;  // IP, RACL, MIRROR, IP_QOS
  uint32_t dst_ip_mask_word3;
  uint32_t dst_ip_mask_word2;
  uint16_t l4_src_port;       // IP, RACL, MIRROR
  uint16_t l4_src_port_mask;  // IP, RACL, MIRROR
  uint16_t l4_dst_port;       // IP, RACL, MIRROR
  uint16_t l4_dst_port_mask;  // IP, RACL, MIRROR
  uint8_t ip_proto;           // IP, RACL, MIRROR, IP_QOS
  uint8_t ip_proto_mask;      // IP, RACL, MIRROR, IP_QOS
  uint8_t ip_tos;             // IP, RACL, MIRROR, IP_QOS
  uint8_t ip_tos_mask;        // IP, RACL, MIRROR, IP_QOS
  switch_ig_port_lag_label_t
      port_lag_label;  // IP, MAC, RACL, MIRROR, MAC_QOS, IP_QOS, DTEL
  switch_ig_port_lag_label_t
      port_lag_label_mask;     // IP, MAC, RACL, MIRROR, MAC_QOS, IP_QOS, DTEL
  switch_bd_label_t bd_label;  // IP, MAC, RACL
  switch_bd_label_t bd_label_mask;  // IP, MAC, RACL
  switch_in_ports_label_t in_ports_group_label = 0;
  switch_in_ports_label_t in_ports_group_label_mask = 0;
  switch_out_ports_label_t out_ports_group_label = 0;
  switch_out_ports_label_t out_ports_group_label_mask = 0;
  bool inout_ports_config;
  uint16_t arp_opcode;
  uint16_t arp_opcode_mask;
  bool port_vlan_miss;
  bool port_vlan_miss_mask;
  bool acl_deny;
  bool acl_deny_mask;
  bool racl_deny;
  bool racl_deny_mask;
  bool rmac_hit;       // IP
  bool rmac_hit_mask;  // IP
  switch_myip_type_t myip;
  switch_myip_type_t myip_mask;
  uint16_t same_if_check;
  uint16_t same_if_check_mask;
  uint16_t same_bd_check;
  uint16_t same_bd_check_mask;
  uint16_t stp_state;
  uint16_t stp_state_mask;
  bool ipv4_unicast_enable;
  bool ipv4_unicast_enable_mask;
  bool ipv6_unicast_enable;
  bool ipv6_unicast_enable_mask;
  uint16_t drop_reason;
  uint16_t cpu_redirect_reason_code;
  // Acl user metadata is an action data field for ingress acls
  // and match key field for egress acls
  uint32_t user_metadata;
  uint32_t user_metadata_mask;
  uint32_t set_user_metadata;
  // Field value range for user metadata
  switch_range_t acl_user_meta_range = {};
  uint8_t src_port_label, dst_port_label;
  uint8_t src_port_label_mask, dst_port_label_mask;

  // TODO(bfn): Following fields have to added to schema
  // ROCEV2
  // uint8_t ip_flag;       // IPv4
  // uint8_t ip_flag_mask;  // IPv4
  // uint32_t flow_label;   // IPv6
  // uint32_t flow_label_mask;  // IPv6
  uint8_t ip_ttl;          // IP, RACL
  uint8_t ip_ttl_mask;     // IP, RACL
  uint8_t tcp_flags;       // IP, RACL, MIRROR, IP_QOS
  uint8_t tcp_flags_mask;  // IP, RACL. MIRROR, IP_QOS
  uint8_t ip_frag;         // IP
  uint8_t ip_frag_mask;    // IP
  uint8_t pkt_type;
  uint8_t pkt_type_mask;
  switch_object_id_t src_port_range_id;  // IP, RACL, MIRROR, IP_QOS
  switch_object_id_t dst_port_range_id;  // IP, RACL, MIRROR, IP_QOS
  uint8_t icmp_type;
  uint8_t icmp_type_mask;
  uint8_t icmp_code;
  uint8_t icmp_code_mask;
  uint16_t l3_mtu;
  uint16_t l3_mtu_mask;
  bool routed;
  bool routed_mask;
  bool sample_packet;
  bool sample_packet_mask;
  switch_object_id_t ingress_port_lag_handle;
  switch_object_id_t egress_port_lag_handle;
  switch_object_id_t ingress_rif;
  switch_object_id_t egress_rif;
  switch_object_id_t meter_handle;
  switch_object_id_t counter_handle;
  switch_object_id_t redirect_handle;
  switch_object_id_t action_ingress_mirror_handle = {0};
  switch_object_id_t action_egress_mirror_handle = {0};
  switch_object_id_t action_vrf_handle;
  switch_object_id_t action_hostif_udt_trap_id;
  uint8_t action_tc;
  uint8_t action_outer_vlan_pri;
  uint8_t action_dscp;
  uint8_t action_ecn;
  uint64_t action_color;
  uint8_t qid;

  switch_object_id_t device_handle;
  switch_object_id_t table_handle;
  switch_enum_t packet_action;
  uint64_t table_type;
  uint64_t direction;
  uint64_t dtel_action_type;
  uint64_t bp_type;
  uint32_t acl_priority;
  uint8_t report_type;
  bool report_all_packets;
  switch_object_id_t sample_session_handle = {0};
  uint8_t sample_session_id = ACL_SAMPLE_NULL_ID;
  bool use_port_group_index;
  uint8_t port_group_index;
  uint32_t port_group_value;
  bool action_disable_nat;
  uint8_t nat_miss_type;
  uint8_t nat_miss_type_mask;
  uint32_t fib_label;
  uint32_t fib_label_mask;
  uint16_t internal_vlan;
  uint16_t internal_vlan_mask;
  uint32_t tunnel_vni;
  uint32_t tunnel_vni_mask;
  uint8_t etype_label;
  uint8_t etype_label_mask = 0x0F;  // 4-bit
  uint8_t macaddr_label;
  uint8_t macaddr_label_mask = 0xFF;  // 8-bit
  uint8_t outer_vlan_pri;
  uint8_t outer_vlan_pri_mask;
  uint8_t outer_vlan_cfi;
  uint8_t outer_vlan_cfi_mask;

  static uint64_t entry_to_switch_pkt_color(switch_enum_t color_enum) {
    switch (color_enum.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_GREEN:
        return SWITCH_PKT_COLOR_GREEN;
      case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_YELLOW:
        return SWITCH_PKT_COLOR_YELLOW;
      case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_RED:
        return SWITCH_PKT_COLOR_RED;
      default:
        break;
    }

    return SWITCH_PKT_COLOR_GREEN;
  }
};

switch_status_t etype_acl1_create(const uint16_t eth_type,
                                  const uint8_t etype_label) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool bf_rt_status = false;
  bf_rt_id_t table_smi_id = smi_id::T_INGRESS_ACL_ETYPE1;
  _Table table(get_dev_tgt(), get_bf_rt_info(), table_smi_id);
  _MatchKey match_key(table_smi_id);
  _ActionEntry action_entry(table_smi_id);

  status |= match_key.set_exact(
      smi_id::F_INGRESS_ACL_ETYPE1_LOCAL_MD_LKP_MAC_TYPE, eth_type);
  action_entry.init_action_data(smi_id::A_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL);
  status |= action_entry.set_arg(smi_id::D_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL,
                                 etype_label);
  status |= table.entry_add(match_key, action_entry, bf_rt_status);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to create etype_acl1 entry {}",
               __func__,
               __LINE__,
               eth_type);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Created etype_acl1 entry",
             __func__,
             __LINE__);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t etype_acl1_delete(const uint16_t eth_type) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_rt_id_t table_smi_id = smi_id::T_INGRESS_ACL_ETYPE1;
  _Table table(get_dev_tgt(), get_bf_rt_info(), table_smi_id);
  _MatchKey match_key(table_smi_id);

  status |= match_key.set_exact(
      smi_id::F_INGRESS_ACL_ETYPE1_LOCAL_MD_LKP_MAC_TYPE, eth_type);
  status |= table.entry_delete(match_key);

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to delete etype_acl1 entry {}",
               __func__,
               __LINE__,
               eth_type);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Deleted etype_acl1 entry",
             __func__,
             __LINE__);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t etype_acl2_create(const uint16_t eth_type,
                                  const uint8_t etype_label) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool bf_rt_status = false;
  bf_rt_id_t table_smi_id = smi_id::T_INGRESS_ACL_ETYPE2;
  _Table table(get_dev_tgt(), get_bf_rt_info(), table_smi_id);
  _MatchKey match_key(table_smi_id);
  _ActionEntry action_entry(table_smi_id);

  status |= match_key.set_exact(
      smi_id::F_INGRESS_ACL_ETYPE2_LOCAL_MD_LKP_MAC_TYPE, eth_type);
  action_entry.init_action_data(smi_id::A_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL);
  status |= action_entry.set_arg(smi_id::D_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL,
                                 etype_label);
  status |= table.entry_add(match_key, action_entry, bf_rt_status);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to create etype_acl2 entry {}",
               __func__,
               __LINE__,
               eth_type);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Created etype_acl2 entry",
             __func__,
             __LINE__);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t etype_acl2_delete(const uint16_t eth_type) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_rt_id_t table_smi_id = smi_id::T_INGRESS_ACL_ETYPE2;
  _Table table(get_dev_tgt(), get_bf_rt_info(), table_smi_id);
  _MatchKey match_key(table_smi_id);

  status |= match_key.set_exact(
      smi_id::F_INGRESS_ACL_ETYPE2_LOCAL_MD_LKP_MAC_TYPE, eth_type);
  status |= table.entry_delete(match_key);

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to delete etype_acl2 entry {}",
               __func__,
               __LINE__,
               eth_type);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Deleted etype_acl2 entry",
             __func__,
             __LINE__);
  return SWITCH_STATUS_SUCCESS;
}

class macaddr_acl : public p4_object_match_action, acl_entry_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MACADDR_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MACADDR_ACL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_MACADDR_ACL_ATTR_STATUS;

 public:
  macaddr_acl(const switch_object_id_t parent,
              uint64_t _table_type,
              uint64_t _direction,
              switch_status_t &status)
      : p4_object_match_action(
            get_smi_id(_table_type,
                       _direction,
                       ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE),
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent),
        acl_entry_object(parent) {
    switch_object_id_t port_lag_handle =
        _direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS
            ? ingress_port_lag_handle
            : egress_port_lag_handle;

    if (port_lag_handle.data != 0) {
      uint16_t port_lag_index_mask = 0x3FF;
      uint16_t port_lag_index = compute_port_lag_index(port_lag_handle);

      status |= match_key.set_ternary(
          get_smi_id(table_type,
                     direction,
                     ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX),
          port_lag_index,
          port_lag_index_mask);
    }
    status |= match_key.set_ternary(
        get_smi_id(
            table_type, direction, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC),
        src_mac,
        src_mac_mask);
    status |= match_key.set_ternary(
        get_smi_id(
            table_type, direction, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC),
        dst_mac,
        dst_mac_mask);
    status |= match_key.set_exact(
        get_smi_id(
            table_type, direction, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY),
        acl_priority);

    action_entry.init_action_data(get_smi_id(
        table_type, direction, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL));
    status |= action_entry.set_arg(
        get_smi_id(table_type,
                   direction,
                   ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL),
        macaddr_label);
  }
};

switch_status_t macaddr_acl_entry_create(const switch_object_id_t parent,
                                         uint64_t acl_type,
                                         uint64_t acl_dir) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject = std::unique_ptr<macaddr_acl>(
      new macaddr_acl(parent, acl_type, acl_dir, status));
  if (!mobject) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to create macaddr_acl",
               __func__,
               __LINE__);
    return status;
  }

  status = mobject->create_update();
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to create_update macaddr_acl",
               __func__,
               __LINE__);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Created macaddr_acl entry",
             __func__,
             __LINE__);

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t macaddr_acl_entry_delete(const switch_object_id_t parent,
                                         uint64_t acl_type,
                                         uint64_t acl_dir) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject = std::unique_ptr<macaddr_acl>(
      new macaddr_acl(parent, acl_type, acl_dir, status));
  if (!mobject) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to delete macaddr_acl",
               __func__,
               __LINE__);
    return status;
  }

  status = mobject->del();
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to del macaddr_acl",
               __func__,
               __LINE__);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: Deleted macaddr_acl entry",
             __func__,
             __LINE__);

  return SWITCH_STATUS_SUCCESS;
}

void switch_compute_acl_rule_port_group_value(const uint64_t table_type,
                                              const uint8_t port_group_index,
                                              uint32_t &port_group_value) {
  switch (table_type) {
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      port_group_value = SWITCH_ACL_FEATURE_PORT_GROUP_VALUE(
          port_group_index, SWITCH_ACL_DATA_IPV4_ACL_PG_POS);
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
      port_group_value = SWITCH_ACL_FEATURE_PORT_GROUP_VALUE(
          port_group_index, SWITCH_ACL_DATA_MAC_ACL_PG_POS);
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      port_group_value = SWITCH_ACL_FEATURE_PORT_GROUP_VALUE(
          port_group_index, SWITCH_ACL_DATA_IPV6_ACL_PG_POS);
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
      port_group_value = SWITCH_ACL_FEATURE_PORT_GROUP_VALUE(
          port_group_index, SWITCH_ACL_MIRROR_ACL_PG_POS);
      break;
    default:
      break;
  }
}

acl_entry_object::acl_entry_object(const switch_object_id_t parent) {
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, eth_type);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK, eth_type_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ETYPE_LABEL, etype_label);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_MACADDR_LABEL, macaddr_label);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_SRC_MAC, src_mac);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK, src_mac_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DST_MAC, dst_mac);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK, dst_mac_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_SRC_IP, src_ip);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD3, src_ipv6_word3);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD2, src_ipv6_word2);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_SRC_IP_MASK, src_ip_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD3, src_ip_mask_word3);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD2, src_ip_mask_word2);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DST_IP, dst_ip);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD3, dst_ipv6_word3);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD2, dst_ipv6_word2);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DST_IP_MASK, dst_ip_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD3, dst_ip_mask_word3);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD2, dst_ip_mask_word2);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_PROTO, ip_proto);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_IP_PROTO_MASK, ip_proto_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT, l4_src_port);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT_MASK, l4_src_port_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT, l4_dst_port);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT_MASK, l4_dst_port_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_USER_METADATA, user_metadata);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_USER_METADATA_MASK, user_metadata_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA, set_user_metadata);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DEVICE, device_handle);
  switch_store::v_get(device_handle,
                      SWITCH_DEVICE_ATTR_ACL_USER_METADATA_RANGE,
                      acl_user_meta_range);

  icmp_type = icmp_type_mask = 0;
  icmp_code = icmp_code_mask = 0;
  if (l4_src_port_mask == 0) {
    switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE, icmp_type);
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE_MASK, icmp_type_mask);
    switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ICMP_CODE, icmp_code);
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_ICMP_CODE_MASK, icmp_code_mask);
    l4_src_port = (((uint16_t)icmp_code) << 8) + icmp_type;
    l4_src_port_mask = (((uint16_t)icmp_code_mask) << 8) + icmp_type_mask;
  }

  if (l4_src_port_mask == 0) {
    switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE, icmp_type);
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE_MASK, icmp_type_mask);
    switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE, icmp_code);
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE_MASK, icmp_code_mask);
    l4_src_port = (((uint16_t)icmp_code) << 8) + icmp_type;
    l4_src_port_mask = (((uint16_t)icmp_code_mask) << 8) + icmp_type_mask;
  }

  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PORT_LAG_LABEL, port_lag_label);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PORT_LAG_LABEL_MASK, port_lag_label_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_BD_LABEL, bd_label);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_BD_LABEL_MASK, bd_label_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN, internal_vlan);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN_MASK, internal_vlan_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_PRI, outer_vlan_pri);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_PRI_MASK, outer_vlan_pri_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_CFI, outer_vlan_cfi);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_CFI_MASK, outer_vlan_cfi_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ARP_OPCODE, arp_opcode);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ARP_OPCODE_MASK, arp_opcode_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PORT_VLAN_MISS, port_vlan_miss);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PORT_VLAN_MISS_MASK, port_vlan_miss_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ACL_DENY, acl_deny);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACL_DENY_MASK, acl_deny_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_RACL_DENY, racl_deny);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_RACL_DENY_MASK, racl_deny_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_RMAC_HIT, rmac_hit);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_RMAC_HIT_MASK, rmac_hit_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_MYIP, myip);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_MYIP_MASK, myip_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAME_IF_CHECK, same_if_check);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAME_IF_CHECK_MASK, same_if_check_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAME_BD_CHECK, same_bd_check);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAME_BD_CHECK_MASK, same_bd_check_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_STP_STATE, stp_state);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_STP_STATE_MASK, stp_state_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_IPV4_UNICAST_ENABLE, ipv4_unicast_enable);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_IPV4_UNICAST_ENABLE_MASK,
                      ipv4_unicast_enable_mask);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_IPV6_UNICAST_ENABLE, ipv6_unicast_enable);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_IPV6_UNICAST_ENABLE_MASK,
                      ipv6_unicast_enable_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_DROP_REASON, drop_reason);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_CPU_REDIRECT_REASON_CODE,
                      cpu_redirect_reason_code);

  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_TTL, ip_ttl);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_TTL_MASK, ip_ttl_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_L3_MTU, l3_mtu);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_L3_MTU_MASK, l3_mtu_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS, tcp_flags);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS_MASK, tcp_flags_mask);
  switch_enum_t frag = {};
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_FRAG, frag);
  switch (frag.enumdata) {
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_ANY:
      ip_frag = SWITCH_IP_FRAG_HEAD | SWITCH_IP_FRAG_NON_HEAD;
      ip_frag_mask = 0x2;
      break;
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_NON_FRAG:
      ip_frag = SWITCH_IP_FRAG_NON_FRAG;
      ip_frag_mask = 0x3;
      break;
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_NON_FRAG_OR_HEAD:
      ip_frag = SWITCH_IP_FRAG_NON_FRAG | SWITCH_IP_FRAG_HEAD;
      ip_frag_mask = 0x1;
      break;
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_HEAD:
      ip_frag = SWITCH_IP_FRAG_HEAD;
      ip_frag_mask = 0x3;
      break;
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_NON_HEAD:
      ip_frag = SWITCH_IP_FRAG_NON_HEAD;
      ip_frag_mask = 0x3;
      break;
    case SWITCH_ACL_ENTRY_ATTR_IP_FRAG_IGNORE:
    default:
      ip_frag = 0;
      ip_frag_mask = 0;
      break;
  }
  uint8_t ip_dscp = 0;
  uint8_t ip_dscp_mask = 0;
  uint8_t ip_ecn = 0;
  uint8_t ip_ecn_mask = 0;
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_DSCP, ip_dscp);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_DSCP_MASK, ip_dscp_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_ECN, ip_ecn);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_IP_ECN_MASK, ip_ecn_mask);
  ip_tos = (ip_dscp << 2);
  ip_tos_mask = (ip_dscp_mask << 2) & 0xFC;
  ip_tos |= (ip_ecn & 0x3);
  ip_tos_mask |= (ip_ecn_mask & 0x3);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_PKT_TYPE, pkt_type);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PKT_TYPE_MASK, pkt_type_mask);

  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_TUNNEL_VNI, tunnel_vni);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_TUNNEL_VNI_MASK, tunnel_vni_mask);

  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SRC_PORT_RANGE_ID, src_port_range_id);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DST_PORT_RANGE_ID, dst_port_range_id);

  src_port_label = 0, dst_port_label = 0;
  src_port_label_mask = 0, dst_port_label_mask = 0;
  if (src_port_range_id.data) {
    switch_store::v_get(
        src_port_range_id, SWITCH_ACL_RANGE_ATTR_LABEL, src_port_label);
    src_port_label_mask = src_port_label;
  }
  if (dst_port_range_id.data) {
    switch_store::v_get(
        dst_port_range_id, SWITCH_ACL_RANGE_ATTR_LABEL, dst_port_label);
    dst_port_label_mask = dst_port_label;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "L4 src port label/mask: {}/{}",
             src_port_label,
             src_port_label_mask);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "L4 dst port label/mask: {}/{}",
             dst_port_label,
             dst_port_label_mask);

  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAMPLE_PACKET, sample_packet);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_SAMPLE_PACKET_MASK, sample_packet_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ROUTED, routed);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ROUTED_MASK, routed_mask);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE,
                      ingress_port_lag_handle);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE,
                      egress_port_lag_handle);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_INGRESS_RIF, ingress_rif);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_EGRESS_RIF, egress_rif);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE, meter_handle);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE, counter_handle);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_REDIRECT, redirect_handle);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE,
                      action_ingress_mirror_handle);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE,
                      action_egress_mirror_handle);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_VRF_HANDLE, action_vrf_handle);
  switch_store::v_get(
      parent,
      SWITCH_ACL_ENTRY_ATTR_ACTION_HOSTIF_USER_DEFINED_TRAP_HANDLE,
      action_hostif_udt_trap_id);

  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC, action_tc);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI,
                      action_outer_vlan_pri);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_DSCP, action_dscp);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_ECN, action_ecn);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_QUEUE_ID, qid);
  switch_enum_t color_enum = {0};
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR, color_enum);
  action_color = acl_entry_object::entry_to_switch_pkt_color(color_enum);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE, report_type);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_ALL_PACKETS,
                      report_all_packets);
  switch_store::v_get(parent,
                      SWITCH_ACL_ENTRY_ATTR_SAMPLE_SESSION_HANDLE,
                      sample_session_handle);
  if (sample_session_handle == 0) {
    sample_session_id = ACL_SAMPLE_NULL_ID;
  } else {
    // Initially only support shared ACL samplers
    switch_enum_t _mode = {};
    switch_store::v_get(
        sample_session_handle, SWITCH_SFLOW_SESSION_ATTR_MODE, _mode);
    if (_mode.enumdata == SWITCH_SFLOW_SESSION_ATTR_MODE_EXCLUSIVE) {
      // Initially only support shared ACL samplers
      sample_session_id = ACL_SAMPLE_NULL_ID;
    } else {
      sample_session_id = switch_store::handle_to_id(sample_session_handle);
    }
  }

  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, packet_action);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_ACTION_DISABLE_NAT, action_disable_nat);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_NAT_MISS_TYPE, nat_miss_type);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_NAT_MISS_TYPE_MASK, nat_miss_type_mask);
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_FIB_LABEL, fib_label);
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_FIB_LABEL_MASK, fib_label_mask);
  switch_enum_t direction_enum = {0};
  switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, direction_enum);
  direction = direction_enum.enumdata;
  switch_enum_t table_enum = {0};
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_enum);
  table_type = get_smi_acl_type(parent, table_enum.enumdata, direction);

  inout_ports_config = true;
  // Extracting in_ports list
  std::vector<switch_object_id_t> in_port_handles_list;
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_IN_PORTS, in_port_handles_list);

  // Extracting out_ports list
  std::vector<switch_object_id_t> out_port_handles_list;
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_OUT_PORTS, out_port_handles_list);

  if (in_port_handles_list.empty() && out_port_handles_list.empty()) {
    inout_ports_config = false;
  }

  if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    switch_store::v_get(parent,
                        SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL,
                        in_ports_group_label);
    switch_store::v_get(parent,
                        SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL_MASK,
                        in_ports_group_label_mask);
  } else if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
    switch_store::v_get(parent,
                        SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL,
                        out_ports_group_label);
    switch_store::v_get(parent,
                        SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL_MASK,
                        out_ports_group_label_mask);
  }

  std::vector<switch_enum_t> bp_type_list;
  switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE, bp_type_list);
  bp_type = SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH;
  for (auto bp_type_enum : bp_type_list) {
    // TODO(bfn): should be fixed for multiple bind points
    //       for now, just take the last one
    bp_type = bp_type_enum.enumdata;
  }

  // std::set<switch_object_id_t> acl_grp_member_list;
  // switch_store::referencing_set_get(
  //     table_handle, SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER,
  //     acl_grp_member_list);
  // for (auto acl_member : acl_grp_member_list) {
  //   compute_acl_group_label(acl_member, false, true);
  // }
  // overwrite port and bd label if bp_type is not NONE
  if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
      bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
    switch_acl_label_t port_lag_label_from_acl, port_lag_label_mask_from_acl;
    switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, port_lag_label_from_acl);
    switch_store::v_get(table_handle,
                        SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK,
                        port_lag_label_mask_from_acl);
    port_lag_label = port_lag_label_from_acl;
    port_lag_label_mask = port_lag_label_mask_from_acl;
  } else if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_VLAN ||
             bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_RIF) {
    switch_acl_label_t bd_label_from_acl, bd_label_mask_from_acl;
    switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, bd_label_from_acl);
    switch_store::v_get(table_handle,
                        SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK,
                        bd_label_mask_from_acl);
    bd_label = bd_label_from_acl;
    bd_label_mask = bd_label_mask_from_acl;
  } else {
    // if bp_type is not set or SWITCH
    // if port label mask is non zero, then overwrite it to use all bits
    if (port_lag_label_mask != 0) {
      if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
        if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_TOS_MIRROR_ACL)) {
          port_lag_label_mask = 0xFFFFF;
        } else {
          port_lag_label_mask = 0xFFFF;
        }
      } else if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
        // When NAT feature is enabled, port_lag_label is 24-bits
        port_lag_label_mask = 0xFFFFFF;
      } else {
        port_lag_label_mask = 0xFFFFFFFF;
      }
    }
    // if bd label mask is non zero, then overwrite it to use all bits
    if (bd_label_mask != 0) {
      bd_label_mask = 0xFFFF;
    }
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "acl table {} b-point type {} acl entry {}",
             table_handle,
             bp_type,
             parent);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "port-lag label value {} mask {}, bd label value {} mask {} "
             "internal vlan value {} mask {}",
             port_lag_label,
             port_lag_label_mask,
             bd_label,
             bd_label_mask,
             internal_vlan,
             internal_vlan_mask);

  switch_enum_t action_type_enum = {0};
  switch_store::v_get(
      parent, SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, action_type_enum);
  dtel_action_type = action_type_enum.enumdata;

  // get acl_priorty
  switch_store::v_get(parent, SWITCH_ACL_ENTRY_ATTR_PRIORITY, acl_priority);

  // for port_group_index
  switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_PORT_GROUP_INDEX, port_group_index);
  switch_store::v_get(table_handle,
                      SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                      use_port_group_index);
  port_group_value = 0;
  if (use_port_group_index) {
    switch_compute_acl_rule_port_group_value(
        table_type, port_group_index, port_group_value);
  }
  switch_store::v_set(table_handle,
                      SWITCH_ACL_TABLE_ATTR_ACL_TYPE_PORT_GROUP_VALUE,
                      port_group_value);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "acl_entry_object: Port group value : {}",
             port_group_value);
}

class acl_hw_entry : public p4_object_match_action, acl_entry_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ACL_HW_ENTRY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ACL_HW_ENTRY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ACL_HW_ENTRY_ATTR_STATUS;
  void compute_ipv6_words(const switch_ip_address_t ipaddr,
                          uint32_t &word3,
                          uint32_t &word2,
                          uint32_t &word1,
                          uint32_t &word0) {
    memcpy(&word3, &ipaddr.ip6[0], sizeof(word3));
    word3 = ntohl(word3);

    memcpy(&word2, &ipaddr.ip6[4], sizeof(word2));
    word2 = ntohl(word2);

    memcpy(&word1, &ipaddr.ip6[8], sizeof(word1));
    word1 = ntohl(word1);

    memcpy(&word0, &ipaddr.ip6[12], sizeof(word0));
    word0 = ntohl(word0);
  }

  switch_status_t program_acl2_egress_ipaddress() {
    uint32_t sip_word3 = 0, sip_word2 = 0, sip_word1 = 0, sip_word0 = 0;
    uint32_t sip_mask_word3 = 0, sip_mask_word2 = 0, sip_mask_word1 = 0,
             sip_mask_word0 = 0;
    uint32_t dip_word3 = 0, dip_word2 = 0, dip_word1 = 0, dip_word0 = 0;
    uint32_t dip_mask_word3 = 0, dip_mask_word2 = 0, dip_mask_word1 = 0,
             dip_mask_word0 = 0;

    uint64_t sip_word10 = 0;
    uint64_t sip_mask_word10 = 0;
    uint64_t dip_word10 = 0;
    uint64_t dip_mask_word10 = 0;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      compute_ipv6_words(src_ip, sip_word3, sip_word2, sip_word1, sip_word0);
      compute_ipv6_words(src_ip_mask,
                         sip_mask_word3,
                         sip_mask_word2,
                         sip_mask_word1,
                         sip_mask_word0);
      sip_word10 = ((uint64_t)sip_word1 << 32) | (sip_word0);
      sip_mask_word10 = ((uint64_t)sip_mask_word1 << 32) | (sip_mask_word0);
    }

    if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      compute_ipv6_words(dst_ip, dip_word3, dip_word2, dip_word1, dip_word0);
      compute_ipv6_words(dst_ip_mask,
                         dip_mask_word3,
                         dip_mask_word2,
                         dip_mask_word1,
                         dip_mask_word0);

      dip_word10 = ((uint64_t)dip_word1 << 32) | (dip_word0);
      dip_mask_word10 = ((uint64_t)dip_mask_word1 << 32) | (dip_mask_word0);
    }

    if (src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3),
          sip_word3,
          sip_mask_word3);
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2),
          sip_word2,
          sip_mask_word2);
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD10),
          sip_word10,
          sip_mask_word10);
    } else {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2),
          src_ip,
          src_ip_mask);
    }

    if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3),
          dip_word3,
          dip_mask_word3);
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2),
          dip_word2,
          dip_mask_word2);
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD10),
          dip_word10,
          dip_mask_word10);
    } else {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2),
          dst_ip,
          dst_ip_mask);
    }
    return status;
  }

 public:
  acl_hw_entry(const switch_object_id_t parent,
               switch_status_t &status,
               uint64_t _table_type,
               uint64_t _direction)
      : p4_object_match_action(
            get_smi_id(_table_type, _direction, ACL_HW_ENTRY_ATTR_TABLE_TYPE),
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent),
        acl_entry_object(parent) {
    bool use_ingress_port_group = false;
    uint16_t in_dev_port = 0;
    uint16_t mirror_session_id = 0;

    if (feature::is_feature_set(SWITCH_FEATURE_IN_PORTS_IN_DATA) ||
        feature::is_feature_set(SWITCH_FEATURE_IN_PORTS_IN_MIRROR) ||
        feature::is_feature_set(SWITCH_FEATURE_OUT_PORTS)) {
      if (inout_ports_config) {
        if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
          status |= match_key.set_ternary(
              get_smi_id(table_type,
                         direction,
                         ACL_HW_ENTRY_ATTR_IN_PORTS_GROUP_LABEL),
              in_ports_group_label,
              in_ports_group_label_mask);
          switch_log(SWITCH_API_LEVEL_DEBUG,
                     SWITCH_OBJECT_TYPE_ACL_ENTRY,
                     "{}:{} in_ports:  {}, in_ports_mask: {}, table type: {}",
                     __func__,
                     __LINE__,
                     in_ports_group_label,
                     in_ports_group_label_mask,
                     table_type);
        } else if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
          status |= match_key.set_ternary(
              get_smi_id(table_type,
                         direction,
                         ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL),
              out_ports_group_label,
              out_ports_group_label_mask);
          switch_log(SWITCH_API_LEVEL_DEBUG,
                     SWITCH_OBJECT_TYPE_ACL_ENTRY,
                     "{}:{} out_ports:  {}, out_ports_mask: {}, table_type: {}",
                     __func__,
                     __LINE__,
                     out_ports_group_label,
                     out_ports_group_label_mask,
                     table_type);
        }
      }
    }

    // configure ingress or egress port_lag_label
    if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
      status |=
          switch_store::v_get(device_handle,
                              SWITCH_DEVICE_ATTR_INGRESS_ACL_PORT_GROUP_ENABLE,
                              use_ingress_port_group);
      if (use_ingress_port_group == false) {
        if (port_lag_label == 0) {
          if (ingress_port_lag_handle.data != 0) {
            uint16_t port_lag_index;
            uint16_t port_lag_index_mask = 0x3FF;
            port_lag_index = compute_port_lag_index(ingress_port_lag_handle);
            status |= match_key.set_ternary(
                get_smi_id(table_type,
                           direction,
                           ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX),
                port_lag_index,
                port_lag_index_mask);
          }
        } else {
          status |= match_key.set_ternary(
              get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL),
              port_lag_label,
              port_lag_label_mask);
          // applies when IN_PORT_IN_IP_ACL_KEY_ENABLE
          if (ingress_port_lag_handle.data != 0) {
            status |= switch_store::v_get(ingress_port_lag_handle,
                                          SWITCH_PORT_ATTR_DEV_PORT,
                                          in_dev_port);
            status |= match_key.set_ternary(
                get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_INGRESS_PORT),
                in_dev_port,
                static_cast<uint16_t>(0x1FF));
          }
        }
      } else {
        // If port_lag_handle from ACL rule is valid, use it in TCAM key.
        // else, if only one port bound to ACL table, use that port_lag_handle
        // from ACL table.
        // else, use computed port_group_index.
        uint16_t port_lag_index;
        uint16_t port_lag_index_mask = 0x3FF;
        if (ingress_port_lag_handle.data != 0) {
          port_lag_index = compute_port_lag_index(ingress_port_lag_handle);
          status |= match_key.set_ternary(
              get_smi_id(table_type,
                         direction,
                         ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX),
              port_lag_index,
              port_lag_index_mask);
        } else {
          switch_object_id_t tbl_port_lag_handle = {0};
          switch_store::v_get(table_handle,
                              SWITCH_ACL_TABLE_ATTR_PORT_LAG_HANDLE,
                              tbl_port_lag_handle);
          switch_store::v_get(table_handle,
                              SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                              use_port_group_index);
          if (tbl_port_lag_handle.data != 0) {
            port_lag_index = compute_port_lag_index(tbl_port_lag_handle);
            status |= match_key.set_ternary(
                get_smi_id(table_type,
                           direction,
                           ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX),
                port_lag_index,
                port_lag_index_mask);
          } else if (use_port_group_index) {
            switch_store::v_get(table_handle,
                                SWITCH_ACL_TABLE_ATTR_PORT_GROUP_INDEX,
                                port_group_index);
            switch_log(SWITCH_API_LEVEL_DEBUG,
                       SWITCH_OBJECT_TYPE_ACL_ENTRY,
                       "ipv4_acl: program port_group_index {} to ACL",
                       port_group_index);
            status |= match_key.set_ternary(
                get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_PORT_GROUP_INDEX),
                port_group_index,
                port_group_index);
          }
        }
      }
    } else if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS) {
      if (ingress_port_lag_handle.data != 0) {
        status |= switch_store::v_get(
            ingress_port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, in_dev_port);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_INGRESS_PORT),
            in_dev_port,
            static_cast<uint16_t>(0x1FF));
      }
    } else {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL),
          static_cast<switch_eg_port_lag_label_t>(port_lag_label),
          static_cast<switch_eg_port_lag_label_t>(port_lag_label_mask));
    }

    if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
      uint16_t eg_port_lag_index;
      uint16_t eg_port_lag_index_mask = 0x3FF;
      if (egress_port_lag_handle.data != 0) {
        eg_port_lag_index = compute_port_lag_index(egress_port_lag_handle);
        status |= match_key.set_ternary(
            get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX),
            eg_port_lag_index,
            eg_port_lag_index_mask);
      }
    }

    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_BD_LABEL),
        bd_label,
        bd_label_mask);
    if (internal_vlan != 0) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_BD),
          internal_vlan,
          internal_vlan_mask);
    }
    if (outer_vlan_pri_mask != 0) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY),
          outer_vlan_pri,
          outer_vlan_pri_mask);

      // Outer vlan priority is valid only
      // when vlan header is valid.
      status |= match_key.set_ternary(
          get_smi_id(
              table_type, direction, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID),
          true,
          true);
    }
    if (outer_vlan_cfi_mask != 0) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_VLAN_CFI),
          outer_vlan_cfi,
          outer_vlan_cfi_mask);

      status |= match_key.set_ternary(
          get_smi_id(
              table_type, direction, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID),
          true,
          true);
    }
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_FIB_LABEL),
        static_cast<uint8_t>(fib_label),
        static_cast<uint8_t>(fib_label_mask));
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID),
        src_port_label,
        src_port_label_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID),
        dst_port_label,
        dst_port_label_mask);

    auto acl_metadata_id =
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_USER_METADATA);
    if (acl_metadata_id) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_USER_METADATA),
          user_metadata,
          user_metadata_mask);
    }

    if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4 ||
        table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR ||
        table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4 ||
        table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IP),
          src_ip.ip4,
          src_ip_mask.ip4);
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IP),
          dst_ip.ip4,
          dst_ip_mask.ip4);
    } else if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_DTEL ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) {
      uint16_t ether_type_mask = 0xFFFF;
      uint16_t ether_type_value = 0;

      if ((src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) ||
          (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6)) {
        ether_type_value = SWITCH_ETHERTYPE_IPV6;
      } else if ((src_ip.ip4 != 0 &&
                  src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) ||
                 (dst_ip.ip4 != 0 &&
                  dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)) {
        ether_type_value = SWITCH_ETHERTYPE_IPV4;
      }
      if (ether_type_value != 0) {
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ETH_TYPE),
            ether_type_value,
            ether_type_mask);
      }
      if (feature::is_feature_set(SWITCH_FEATURE_IPV6_ACL_UPPER64)) {
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3),
            src_ipv6_word3,
            src_ip_mask_word3);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2),
            src_ipv6_word2,
            src_ip_mask_word2);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3),
            dst_ipv6_word3,
            dst_ip_mask_word3);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2),
            dst_ipv6_word2,
            dst_ip_mask_word2);
      } else {
        if ((direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) &&
            acl_using_acl2_profile()) {
          status |= program_acl2_egress_ipaddress();
        } else {
          status |= match_key.set_ip_unified_ternary(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IP),
              src_ip,
              src_ip_mask);
          status |= match_key.set_ip_unified_ternary(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IP),
              dst_ip,
              dst_ip_mask);
        }
      }
    } else {
      if (feature::is_feature_set(SWITCH_FEATURE_IPV6_ACL_UPPER64)) {
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3),
            src_ipv6_word3,
            src_ip_mask_word3);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2),
            src_ipv6_word2,
            src_ip_mask_word2);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3),
            dst_ipv6_word3,
            dst_ip_mask_word3);
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2),
            dst_ipv6_word2,
            dst_ip_mask_word2);
      } else {
        if ((direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) &&
            acl_using_acl2_profile()) {
          status |= program_acl2_egress_ipaddress();
        } else {
          src_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
          dst_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
          src_ip_mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
          dst_ip_mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
          status |= match_key.set_ternary(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_IP),
              src_ip,
              src_ip_mask);
          status |= match_key.set_ternary(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_IP),
              dst_ip,
              dst_ip_mask);
        }
      }
    }

    // Set ip_proto variable if ACl entry includes tcp_flags field
    if (tcp_flags_mask && !ip_proto_mask) {
      ip_proto = (uint8_t)0x06;  // IP_PROTO_TCP
      ip_proto_mask = (uint8_t)0xFF;
    }

    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_IP_PROTO),
        ip_proto,
        ip_proto_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_IP_TOS),
        ip_tos,
        ip_tos_mask);

    // ip_proto must be specified to match L4 ports in inner dtel acls
    acl_hw_entry_attr l4_src_port_attr, l4_dst_port_attr;
    if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4 ||
         table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6) &&
        ip_proto == 0x06) {  // IP_PROTO_TCP
      l4_src_port_attr = ACL_HW_ENTRY_ATTR_TCP_SRC_PORT;
      l4_dst_port_attr = ACL_HW_ENTRY_ATTR_TCP_DST_PORT;
    } else if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4 ||
                table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6) &&
               ip_proto == 0x11) {  // IP_PROTO_UDP
      l4_src_port_attr = ACL_HW_ENTRY_ATTR_UDP_SRC_PORT;
      l4_dst_port_attr = ACL_HW_ENTRY_ATTR_UDP_DST_PORT;
    } else {
      l4_src_port_attr = ACL_HW_ENTRY_ATTR_L4_SRC_PORT;
      l4_dst_port_attr = ACL_HW_ENTRY_ATTR_L4_DST_PORT;
    }
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, l4_src_port_attr),
        l4_src_port,
        l4_src_port_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, l4_dst_port_attr),
        l4_dst_port,
        l4_dst_port_mask);

    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_TTL),
        ip_ttl,
        ip_ttl_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_IP_FRAG),
        ip_frag,
        ip_frag_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_TCP_FLAGS),
        tcp_flags,
        tcp_flags_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SRC_MAC),
        src_mac,
        src_mac_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DST_MAC),
        dst_mac,
        dst_mac_mask);
    status |= match_key.set_ternary(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_VNI),
        tunnel_vni,
        tunnel_vni_mask);

    if ((eth_type != 0) && !acl_using_acl2_profile()) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ETH_TYPE),
          eth_type,
          eth_type_mask);
    }

    if (etype_label != 0) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ETYPE_LABEL),
          etype_label,
          etype_label_mask);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: set_ternary etype_label {} status {}",
                 __func__,
                 __LINE__,
                 etype_label,
                 status);
    }

    if (macaddr_label != 0) {
      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_MACADDR_LABEL),
          macaddr_label,
          macaddr_label_mask);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: set_ternary macaddr_label {} status {}",
                 __func__,
                 __LINE__,
                 macaddr_label,
                 status);
    }

    if (ingress_rif.data != 0 || egress_rif.data != 0) {
      switch_object_id_t rif_handle =
          direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS ? ingress_rif
                                                               : egress_rif;
      if (switch_store::object_type_query(rif_handle) !=
          SWITCH_OBJECT_TYPE_RIF) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}:{}: Incorrect object type for rif_handle {}",
                   __func__,
                   __LINE__,
                   rif_handle);
        status = SWITCH_STATUS_INVALID_PARAMETER;
        return;
      }

      switch_object_id_t port_lag_handle = {}, bd_handle = {};
      switch_enum_t rif_type = {};

      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
        status |= find_auto_oid(rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t vlan_handle = {};
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      }

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        // Both ingress RIF and Port cannot be used together
        if (ingress_rif.data != 0 && ingress_port_lag_handle.data != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_ACL_ENTRY,
                     "{}:{}: Ingress port cannot be used with ingress RIF",
                     __func__,
                     __LINE__);
          status = SWITCH_STATUS_INVALID_PARAMETER;
          return;
        }
        // Both egress RIF and Port cannot be used together
        if (egress_rif.data != 0 && egress_port_lag_handle.data != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_ACL_ENTRY,
                     "{}:{}: Egress port cannot be used with egress RIF",
                     __func__,
                     __LINE__);
          status = SWITCH_STATUS_INVALID_PARAMETER;
          return;
        }
      }

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        // Both VLANs cannot be used together
        if (internal_vlan != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_ACL_ENTRY,
                     "{}:{}: VLAN cannot be used with RIF",
                     __func__,
                     __LINE__);
          status = SWITCH_STATUS_INVALID_PARAMETER;
          return;
        }
      }

      if (port_lag_handle.data != 0) {
        uint16_t port_lag_index;
        uint16_t port_lag_index_mask = 0x3FF;
        port_lag_index = compute_port_lag_index(port_lag_handle);
        status |= match_key.set_ternary(
            get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX),
            port_lag_index,
            port_lag_index_mask);
      }

      if (bd_handle.data != 0) {
        uint16_t bd = compute_bd(bd_handle);
        uint16_t bd_mask = 0x1FFF;
        status |= match_key.set_ternary(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_BD),
            bd,
            bd_mask);
      }

      status |= match_key.set_ternary(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ROUTED),
          true,
          true);
    }

    status |= match_key.set_exact(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PRIORITY),
        acl_priority);

    /*
     * Action merge scenarios
     * -----------------------------------------------------------------------
     * Packet Action  Action Type
     * -----------------------------------------------------------------------
     * Permit         Meter, Mirror, User trap, tc, color, Vrf, nexthop, port
     * Deny           Mirror  // possible, not implemented
     * Redirect_cpu   Meter, User trap
     * Copy_cpu       Meter, User trap
     *
     */
    switch (packet_action.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT: {
        // set_vrf
        if (action_vrf_handle != 0) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SET_VRF));
          status |= action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_VRF),
              compute_vrf(action_vrf_handle));
          return;
        }

        if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) &&
            (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS)) {
          if ((action_tc != 0 && meter_handle != 0 && action_color != 0) ||
              (action_tc != 0 && meter_handle != 0) ||
              (action_tc != 0 && action_color != 0) ||
              (action_color != 0 && meter_handle != 0)) {
            action_entry.init_action_data(get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_SET_QOS_PARAMS));
            if (action_tc != 0)
              status |= action_entry.set_arg(
                  get_smi_id(
                      table_type, direction, ACL_HW_ENTRY_ATTR_QOS_PARAMS_TC),
                  action_tc);
            if (action_color != 0)
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_QOS_PARAMS_COLOR),
                  action_color);
            if (meter_handle != 0) {
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_QOS_PARAMS_METER_INDEX),
                  meter_handle);
              uint64_t cir_bps = 0, pir_bps = 0, cburst_bytes = 0,
                       pburst_bytes = 0;
              status |= get_meter_attrs_per_acl_entry(
                  meter_handle, cir_bps, pir_bps, cburst_bytes, pburst_bytes);
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CIR),
                  switch_meter_bytes_to_kbps(cir_bps));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PIR),
                  switch_meter_bytes_to_kbps(pir_bps));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CBS),
                  switch_meter_bytes_to_kbps(cburst_bytes));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PBS),
                  switch_meter_bytes_to_kbps(pburst_bytes));
            }
            return;
          }
        }

        // set_tc
        if (action_tc != 0) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SET_TC));
          status |= action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_TC),
              action_tc);

          if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) &&
              (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) &&
              meter_handle != 0) {
            status |= action_entry.set_arg(
                get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_TC_METER_INDEX),
                meter_handle);
          }
          return;
        }

        // Egress IP QoS ACL actions
        if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) &&
            (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS)) {
          // set both tos and outer_vlan_pri
          uint8_t action_tos = 0;
          if ((action_outer_vlan_pri != QOS_ACL_ACTION_PCP_DEFAULT_VALUE) &&
              (action_dscp != QOS_ACL_ACTION_TOS_DEFAULT_VALUE)) {
            if (action_ecn != QOS_ACL_ACTION_TOS_DEFAULT_VALUE) {
              action_tos = action_dscp << 2 | action_ecn;
            } else {
              action_tos = action_dscp << 2;
            }
            if ((src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) ||
                (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6)) {
              switch_log(SWITCH_API_LEVEL_DEBUG,
                         SWITCH_OBJECT_TYPE_ACL_ENTRY,
                         "{}.{}: Setting PCP:{} and IPv6 TOS:{} actions/args",
                         __func__,
                         __LINE__,
                         action_outer_vlan_pri,
                         action_tos);
              action_entry.init_action_data(get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_SET_PCP_IPV6_TOS));
              status |= action_entry.set_arg(
                  get_smi_id(
                      table_type, direction, ACL_HW_ENTRY_ATTR_PCP_IPV6_TOS),
                  action_tos);
              status |= action_entry.set_arg(
                  get_smi_id(
                      table_type, direction, ACL_HW_ENTRY_ATTR_PCP_IPV6_PCP),
                  action_outer_vlan_pri);
              if (meter_handle != 0) {
                status |= action_entry.set_arg(
                    get_smi_id(table_type,
                               direction,
                               ACL_HW_ENTRY_ATTR_PCP_IPV6_METER_INDEX),
                    meter_handle);
              }
              return;

            } else {
              switch_log(SWITCH_API_LEVEL_DEBUG,
                         SWITCH_OBJECT_TYPE_ACL_ENTRY,
                         "{}.{}: Setting PCP:{} and IPV4 TOS:{} actions/args",
                         __func__,
                         __LINE__,
                         action_outer_vlan_pri,
                         action_tos);
              action_entry.init_action_data(get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_SET_PCP_IPV4_TOS));
              status |= action_entry.set_arg(
                  get_smi_id(
                      table_type, direction, ACL_HW_ENTRY_ATTR_PCP_IPV4_TOS),
                  action_tos);
              status |= action_entry.set_arg(
                  get_smi_id(
                      table_type, direction, ACL_HW_ENTRY_ATTR_PCP_IPV4_PCP),
                  action_outer_vlan_pri);
              if (meter_handle != 0) {
                status |= action_entry.set_arg(
                    get_smi_id(table_type,
                               direction,
                               ACL_HW_ENTRY_ATTR_PCP_IPV4_METER_INDEX),
                    meter_handle);
              }
              return;
            }

          } else {
            // set outer_vlan_pri
            if (action_outer_vlan_pri != QOS_ACL_ACTION_PCP_DEFAULT_VALUE) {
              switch_log(SWITCH_API_LEVEL_DEBUG,
                         SWITCH_OBJECT_TYPE_ACL_ENTRY,
                         "{}.{}: Setting PCP:{} actions/args",
                         __func__,
                         __LINE__,
                         action_outer_vlan_pri);
              action_entry.init_action_data(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SET_PCP));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PCP),
                  action_outer_vlan_pri);
              if (meter_handle != 0) {
                status |= action_entry.set_arg(
                    get_smi_id(table_type,
                               direction,
                               ACL_HW_ENTRY_ATTR_PCP_METER_INDEX),
                    meter_handle);
              }
              return;
            }

            // set tos
            if (action_dscp != QOS_ACL_ACTION_TOS_DEFAULT_VALUE ||
                action_ecn != QOS_ACL_ACTION_TOS_DEFAULT_VALUE) {
              if (action_dscp != QOS_ACL_ACTION_TOS_DEFAULT_VALUE &&
                  action_ecn != QOS_ACL_ACTION_TOS_DEFAULT_VALUE) {
                action_tos = action_dscp << 2 | action_ecn;
              } else if (action_dscp == QOS_ACL_ACTION_TOS_DEFAULT_VALUE &&
                         action_ecn != QOS_ACL_ACTION_TOS_DEFAULT_VALUE) {
                action_tos = action_ecn;
              } else if (action_dscp != QOS_ACL_ACTION_TOS_DEFAULT_VALUE &&
                         action_ecn == QOS_ACL_ACTION_TOS_DEFAULT_VALUE) {
                action_tos = action_dscp << 2;
              }
              if ((src_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) ||
                  (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6)) {
                switch_log(SWITCH_API_LEVEL_DEBUG,
                           SWITCH_OBJECT_TYPE_ACL_ENTRY,
                           "{}.{}: Setting IPv6 TOS:{} actions/args",
                           __func__,
                           __LINE__,
                           action_tos);
                action_entry.init_action_data(get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_SET_IPV6_TOS));
                status |= action_entry.set_arg(
                    get_smi_id(
                        table_type, direction, ACL_HW_ENTRY_ATTR_IPV6_TOS),
                    action_tos);
                if (meter_handle != 0) {
                  status |= action_entry.set_arg(
                      get_smi_id(table_type,
                                 direction,
                                 ACL_HW_ENTRY_ATTR_IPV6_TOS_METER_INDEX),
                      meter_handle);
                }
              } else {
                switch_log(SWITCH_API_LEVEL_DEBUG,
                           SWITCH_OBJECT_TYPE_ACL_ENTRY,
                           "{}.{}: Setting IPv4 TOS:{} actions/args",
                           __func__,
                           __LINE__,
                           action_tos);
                action_entry.init_action_data(get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_SET_IPV4_TOS));
                status |= action_entry.set_arg(
                    get_smi_id(
                        table_type, direction, ACL_HW_ENTRY_ATTR_IPV4_TOS),
                    action_tos);
                if (meter_handle != 0) {
                  status |= action_entry.set_arg(
                      get_smi_id(table_type,
                                 direction,
                                 ACL_HW_ENTRY_ATTR_IPV4_TOS_METER_INDEX),
                      meter_handle);
                }
              }
              return;
            }
          }
        }

        // set_color
        if (action_color != 0) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SET_COLOR));
          status |= action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_COLOR),
              action_color);
          if ((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) &&
              (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) &&
              meter_handle != 0) {
            status |= action_entry.set_arg(
                get_smi_id(
                    table_type, direction, ACL_HW_ENTRY_ATTR_COLOR_METER_INDEX),
                meter_handle);
          }
          return;
        }

        // redirect
        if (redirect_handle != 0) {
          switch (switch_store::object_type_query(redirect_handle)) {
            case SWITCH_OBJECT_TYPE_NEXTHOP:
            case SWITCH_OBJECT_TYPE_ECMP:
              action_entry.init_action_data(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP));
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE),
                  compute_nexthop_index(redirect_handle));
              status = action_set_user_metadata(
                  ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA);
              return;
            case SWITCH_OBJECT_TYPE_PORT:
            case SWITCH_OBJECT_TYPE_LAG:
              action_entry.init_action_data(get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT));
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX),
                  compute_port_lag_index(redirect_handle));
              status = action_set_user_metadata(
                  ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA);
              return;
            default:
              switch_log(
                  SWITCH_API_LEVEL_ERROR,
                  SWITCH_OBJECT_TYPE_ACL_ENTRY,
                  "{}:{}: Unsupported object for redirect action {} parent {}",
                  __func__,
                  __LINE__,
                  parent,
                  redirect_handle);
              return;
          }
        }

        // disable nat
        if (action_disable_nat) {
          if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
            action_entry.init_action_data(get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_ACL_NO_NAT));
            bf_rt_id_t disable_nat_smi_id =
                get_smi_id(table_type,
                           direction,
                           ACL_HW_ENTRY_ATTR_ACL_NO_NAT_DISABLE_NAT);
            status |=
                action_entry.set_arg(disable_nat_smi_id, action_disable_nat);
            return;
          }
        }

        // mirror - ingress/egress
        if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS ||
            direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
          switch_object_id_t mirror_handle = {0};
          bool ingress_mirror_action = true;

          if (action_ingress_mirror_handle != 0) {
            mirror_handle = action_ingress_mirror_handle;
          } else if (action_egress_mirror_handle != 0) {
            mirror_handle = action_egress_mirror_handle;
            ingress_mirror_action = false;
          }
          if (mirror_handle != 0) {
            switch_object_id_t mirror_meter_handle = {};
            status |= switch_store::v_get(mirror_handle,
                                          SWITCH_MIRROR_ATTR_SESSION_ID,
                                          mirror_session_id);
            status |= switch_store::v_get(mirror_handle,
                                          SWITCH_MIRROR_ATTR_METER_HANDLE,
                                          mirror_meter_handle);

            if (mirror_meter_handle.data != 0) {
              switch_enum_t target_type = {0};
              if (acl_using_acl2_profile()) {
                target_type.enumdata =
                    ((direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS)
                         ? SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_MIRROR_ACL
                         : SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_MIRROR_ACL);

              } else {
                target_type.enumdata =
                    ((direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS)
                         ? SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR
                         : SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR);
              }
              status |= switch_store::v_set(mirror_meter_handle,
                                            SWITCH_METER_ATTR_TARGET_TYPE,
                                            target_type);
            }

            if (ingress_mirror_action) {
              action_entry.init_action_data(get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN));
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE),
                  mirror_session_id);
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE),
                  mirror_meter_handle);
            } else {
              action_entry.init_action_data(get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT));
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE),
                  mirror_session_id);
              status |= action_entry.set_arg(
                  get_smi_id(table_type,
                             direction,
                             ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE),
                  mirror_meter_handle);
            }
            if (mirror_meter_handle != 0) {
              uint64_t cir_bps = 0, pir_bps = 0, cburst_bytes = 0,
                       pburst_bytes = 0;
              status |= get_meter_attrs_per_acl_entry(mirror_meter_handle,
                                                      cir_bps,
                                                      pir_bps,
                                                      cburst_bytes,
                                                      pburst_bytes);
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CIR),
                  switch_meter_bytes_to_kbps(cir_bps));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PIR),
                  switch_meter_bytes_to_kbps(pir_bps));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CBS),
                  switch_meter_bytes_to_kbps(cburst_bytes));
              status |= action_entry.set_arg(
                  get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PBS),
                  switch_meter_bytes_to_kbps(pburst_bytes));
            }
            return;
          }
        }

        if ((meter_handle != 0) &&
            (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS)) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SET_METER));
          status |= action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_METER_INDEX),
              meter_handle);
        }

        // this is it, nothing else left, so permit
        if (meter_handle != 0) action_set_acl_meter(meter_handle);
        if (get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_PERMIT)) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_PERMIT));
          status =
              action_set_user_metadata(ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA);
          status |= action_entry.set_arg(
              get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX),
              meter_handle);
          status |= action_entry.set_arg(
              get_smi_id(
                  table_type, direction, ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID),
              action_hostif_udt_trap_id);
        } else {
          action_entry.init_action_data(get_smi_id(
              table_type, direction, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION));
          if (((table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS) ||
               (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR)) &&
              ((direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) ||
               (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS)) &&
              meter_handle != 0) {
            status |= action_entry.set_arg(
                get_smi_id(table_type,
                           direction,
                           ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX),
                meter_handle);
            uint64_t cir_bps = 0, pir_bps = 0, cburst_bytes = 0,
                     pburst_bytes = 0;
            status |= get_meter_attrs_per_acl_entry(
                meter_handle, cir_bps, pir_bps, cburst_bytes, pburst_bytes);
            status |= action_entry.set_arg(
                get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CIR),
                switch_meter_bytes_to_kbps(cir_bps));
            status |= action_entry.set_arg(
                get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PIR),
                switch_meter_bytes_to_kbps(pir_bps));
            status |= action_entry.set_arg(
                get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_CBS),
                switch_meter_bytes_to_kbps(cburst_bytes));
            status |= action_entry.set_arg(
                get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_PBS),
                switch_meter_bytes_to_kbps(pburst_bytes));
          }
        }
      } break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP:
        action_entry.init_action_data(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_DROP));
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DENY:
        if (feature::is_feature_set(
                SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION)) {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_DENY));
        } else {
          action_entry.init_action_data(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_DROP));
        }
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_TRANSIT:
        action_entry.init_action_data(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_TRANSIT));
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
        if (meter_handle != 0) action_set_acl_meter(meter_handle);
        action_entry.init_action_data(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_TRAP));
        status |= action_entry.set_arg(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID),
            action_hostif_udt_trap_id);
        status |= action_entry.set_arg(
            get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX),
            meter_handle);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU:
        if (meter_handle != 0) action_set_acl_meter(meter_handle);
        action_entry.init_action_data(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_COPY));
        status |= action_entry.set_arg(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_COPY_TRAP_ID),
            action_hostif_udt_trap_id);
        status |= action_entry.set_arg(
            get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_COPY_METER_INDEX),
            meter_handle);
        break;
      default:
        break;
    }

    // action data maybe initialized above, gets overwritten below
    switch (dtel_action_type) {
      case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT:
        action_entry.init_action_data(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DTEL_REPORT));
        if (report_all_packets) report_type |= SWITCH_DTEL_SUPPRESS_REPORT;
        status |= action_entry.set_arg(
            get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_REPORT_TYPE),
            report_type);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE:
        action_entry.init_action_data(get_smi_id(
            table_type, direction, ACL_HW_ENTRY_ATTR_IFA_CLONE_SAMPLE));
        status |= action_entry.set_arg(
            get_smi_id(
                table_type, direction, ACL_HW_ENTRY_ATTR_IFA_CLONE_SESSION_ID),
            sample_session_id);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT:
        action_entry.init_action_data(
            get_smi_id(table_type,
                       direction,
                       ACL_HW_ENTRY_ATTR_IFA_CLONE_AND_DTEL_REPORT));
        status |= action_entry.set_arg(
            get_smi_id(table_type,
                       direction,
                       ACL_HW_ENTRY_ATTR_IFA_SESSION_ID_WITH_TYPE),
            sample_session_id);
        if (report_all_packets) report_type |= SWITCH_DTEL_SUPPRESS_REPORT;
        status |= action_entry.set_arg(
            get_smi_id(table_type,
                       direction,
                       ACL_HW_ENTRY_ATTR_REPORT_TYPE_WITH_CLONE),
            report_type);
        break;
      default:
        break;
    }
  }
  void action_set_acl_meter(switch_object_id_t m_handle) {
    switch_attr_id_t target_type_attr_id = SWITCH_METER_ATTR_TARGET_TYPE;
    if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
      switch_enum_t new_target_type = {0};
      if (acl_using_acl2_profile()) {
        new_target_type.enumdata =
            SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_IP_QOS_ACL;
      } else {
        new_target_type.enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_ACL;
      }
      switch_enum_t old_target_type = {};
      switch_store::v_get(m_handle, target_type_attr_id, old_target_type);
      if (old_target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_NONE) {
        switch_store::v_set(m_handle, target_type_attr_id, new_target_type);
      }
    } else {
      switch_enum_t new_target_type = {0};
      if (acl_using_acl2_profile()) {
        new_target_type.enumdata =
            SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_IP_QOS_ACL;
      } else {
        new_target_type.enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_ACL;
      }
      switch_enum_t old_target_type = {};
      switch_store::v_get(m_handle, target_type_attr_id, old_target_type);
      if (old_target_type.enumdata == SWITCH_METER_ATTR_TARGET_TYPE_NONE) {
        switch_store::v_set(m_handle, target_type_attr_id, new_target_type);
      }
    }
  }
  switch_status_t action_set_user_metadata(int set_user_metadata_attr_id) {
    switch_status_t mstatus = SWITCH_STATUS_SUCCESS;
    bf_rt_id_t user_meta_smi_id =
        get_smi_id(table_type, direction, set_user_metadata_attr_id);
    if (user_meta_smi_id) {
      if (set_user_metadata >= acl_user_meta_range.min &&
          set_user_metadata <= acl_user_meta_range.max) {
        mstatus |= action_entry.set_arg(user_meta_smi_id, set_user_metadata);
      } else {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}:{}: ACL user metadata {} not within permitted range "
                   "Min:{} Max:{}",
                   __func__,
                   __LINE__,
                   set_user_metadata,
                   acl_user_meta_range.min,
                   acl_user_meta_range.max);
        mstatus |= SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return mstatus;
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0, bytes = 0;
    status = p4_object_match_action::data_get();
    // dscp_mirror acl doesn't have permit/deny action, use mirror action to
    // retrieve stats.
    if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR) {
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT),
          &pkts);
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT),
          &bytes);
    } else if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_DTEL ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4 ||
               table_type == SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6) {
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DTEL_REPORT),
          &pkts);
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_DTEL_REPORT),
          &bytes);
    } else {
      // at first glance this is confusing as to why the NO_ACTION action name
      // is used to retreive counters. The ACL counters are direct and any
      // action name can be used to retreive them. This is just a quirk of BFRT
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION),
          &pkts);
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION),
          &bytes);
    }
    switch_counter_t cntr_pkts;
    cntr_pkts.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_PKTS;
    cntr_pkts.count = pkts;
    cntrs[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS] = cntr_pkts;
    switch_counter_t cntr_bytes;
    cntr_bytes.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_BYTES;
    cntr_bytes.count = bytes;
    cntrs[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES] = cntr_bytes;

    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
        value);
    action_entry.set_arg(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS), value);
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action::data_get();

    for (auto id : cntr_ids) {
      switch (id) {
        case SWITCH_ACL_ENTRY_COUNTER_ID_PKTS:
          action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
              value);
          break;
        case SWITCH_ACL_ENTRY_COUNTER_ID_BYTES:
          action_entry.set_arg(
              get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
              value);
          break;
        default:
          break;
      }
    }
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      ctr_bytes = 0, ctr_pkt = 0;
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_DROP),
          &ctr_bytes);
      action_entry.get_arg(
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
          get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_ACL_DROP),
          &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));

      attr_w ctr_attr_list(SWITCH_ACL_HW_ENTRY_ATTR_MAU_STATS_CACHE);
      ctr_attr_list.v_set(ctr_list);
      switch_store::attribute_set(get_auto_oid(), ctr_attr_list);
    }

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_ACL_HW_ENTRY_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: No stat cache to restore mau stats, "
                 "acl_entry cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (ctr_list.size() == SWITCH_ACL_ENTRY_COUNTER_ID_MAX) {
      ctr_pkt = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS];
      ctr_bytes = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES];
    }

    action_entry.set_arg(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_BYTES),
        ctr_bytes);
    action_entry.set_arg(
        get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_STATS_PKTS),
        ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "acl_entry status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

switch_status_t add_sample_session(uint32_t session_id,
                                   uint32_t rate,
                                   uint64_t table_type,
                                   uint64_t direction) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  bf_rt_id_t table_smi_id =
      get_smi_id(table_type, direction, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_TABLE);
  _Table table(get_dev_tgt(), get_bf_rt_info(), table_smi_id);
  _MatchKey register_key(table_smi_id);
  _ActionEntry register_action(table_smi_id);
  register_action.init_indirect_data();
  status |= register_key.set_exact(
      get_smi_id(
          table_type, direction, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_INDEX),
      session_id);
  status |= register_action.set_arg(
      get_smi_id(
          table_type, direction, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_CURRENT),
      rate);
  status |= register_action.set_arg(
      get_smi_id(
          table_type, direction, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_RATE),
      rate);
  status = table.entry_modify(register_key, register_action);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_ACL_ENTRY,
        "{}:{}: failed add sample session status {} session_id {} rate {}",
        __func__,
        __LINE__,
        status,
        session_id,
        rate);
  }

  return status;
}

class acl_sample_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ACL_SAMPLE_SESSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ACL_SAMPLE_SESSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ACL_SAMPLE_SESSION_ATTR_PARENT_HANDLE;

 public:
  acl_sample_session(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t sample_session_handle = {0};
    uint8_t sample_session_id = ACL_SAMPLE_NULL_ID;
    switch_enum_t _mode = {};
    uint32_t rate = 0;
    switch_enum_t action_type_enum = {0};
    uint64_t dtel_action_type;

    switch_object_id_t table_handle = {0};
    uint64_t table_type = 0, direction = 0;
    switch_enum_t table_enum = {0}, direction_enum = {0};
    IdMap::instance()->Dummy();
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
    switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, direction_enum);
    direction = direction_enum.enumdata;
    switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_enum);
    table_type = get_smi_acl_type(parent, table_enum.enumdata, direction);
    status |= switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, action_type_enum);
    dtel_action_type = action_type_enum.enumdata;

    switch (dtel_action_type) {
      case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE:
      case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT:
        status |=
            switch_store::v_get(parent,
                                SWITCH_ACL_ENTRY_ATTR_SAMPLE_SESSION_HANDLE,
                                sample_session_handle);
        if (sample_session_handle != 0) {
          status |= switch_store::v_get(
              sample_session_handle, SWITCH_SFLOW_SESSION_ATTR_MODE, _mode);
          status |= switch_store::v_get(sample_session_handle,
                                        SWITCH_SFLOW_SESSION_ATTR_SAMPLE_RATE,
                                        rate);
          if (_mode.enumdata == SWITCH_SFLOW_SESSION_ATTR_MODE_EXCLUSIVE) {
            // Initially only support shared ACL samplers
            status |= add_sample_session(
                ACL_SAMPLE_NULL_ID, 0, table_type, direction);
          } else {
            sample_session_id =
                switch_store::handle_to_id(sample_session_handle);
            status |= add_sample_session(
                sample_session_id, rate, table_type, direction);
          }
        } else {
          status |=
              add_sample_session(ACL_SAMPLE_NULL_ID, 0, table_type, direction);
        }
        break;
      default:
        break;
    }
  }
};

class l4_port_range {
 public:
  uint8_t range_bit_label = 0;
  uint8_t alloc_bit = 0xFF;
  uint16_t i = 0;
  switch_range_t range = {};
  switch_acl_range_attr_type type = SWITCH_ACL_RANGE_ATTR_TYPE_MAX;
  switch_object_id_t parent = {};
  std::vector<bf_rt_table_id_t> table_id;
  std::vector<bf_rt_field_id_t> field;
  std::vector<bf_rt_action_id_t> action;
  std::vector<bf_rt_field_id_t> action_prm;

  l4_port_range(const switch_object_id_t p) {
    parent = p;
    switch_enum_t t = {};

    switch_store::v_get(parent, SWITCH_ACL_RANGE_ATTR_RANGE, range);
    switch_store::v_get(parent, SWITCH_ACL_RANGE_ATTR_LABEL, range_bit_label);
    switch_store::v_get(parent, SWITCH_ACL_RANGE_ATTR_TYPE, t);

    type = static_cast<switch_acl_range_attr_type>(t.enumdata);

    // if acl_range has no pre-alloc bit label, then allocate
    if (range_bit_label == 0) {
      auto it = global_acl_range_label.find(type);
      if (it == global_acl_range_label.end()) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_RANGE,
                   "{}.{}: Invalid Acl L4 port range type",
                   __func__,
                   __LINE__);
        return;
      }
      auto &range_labels = it->second;
      for (i = 0; i < range_labels.size(); i++) {
        if (range_labels.test(i)) {
          continue;
        } else {
          alloc_bit = i;
          break;
        }
      }
      if (alloc_bit != 0xFF) {
        range_bit_label = 1U << alloc_bit;
        switch_store::v_set(
            parent, SWITCH_ACL_RANGE_ATTR_LABEL, range_bit_label);
        range_labels.set(alloc_bit, true);
      }
    }
  }

  switch_status_t program_l4_port_entries(bool ingress, bool set) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    std::vector<uint16_t> *l4_port_entries = NULL;
    uint8_t calc_label = 0;
    uint32_t port = 0;
    bool add = false;

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_RANGE,
               "{}.{}: {} Range type {} Min {} Max {} - {} bit label {}",
               __func__,
               __LINE__,
               ingress ? "Ingress" : "Egress",
               type,
               range.min,
               range.max,
               set ? "set" : "unset",
               range_bit_label);

    if (type == SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) {
      if (ingress) {
        l4_port_entries = &l4_ingress_src_port_entries;
      } else {
        l4_port_entries = &l4_egress_src_port_entries;
      }
    } else if (type == SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) {
      if (ingress) {
        l4_port_entries = &l4_ingress_dst_port_entries;
      } else {
        l4_port_entries = &l4_egress_dst_port_entries;
      }
    } else {
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
    for (port = range.min; port <= range.max; port++) {
      if (set) {
        calc_label = (uint8_t)(*l4_port_entries)[port] | range_bit_label;
      } else {
        calc_label = (uint8_t)(*l4_port_entries)[port] & ~range_bit_label;
      }
      int tables = table_id.size();
      for (int j = 0; j < tables; j++) {
        if (table_id[j] == 0) continue;
        _Table table(get_dev_tgt(), get_bf_rt_info(), table_id[j]);
        _ActionEntry action_entry(table_id[j]);
        _MatchKey match_key(table_id[j]);

        status |= match_key.set_exact(field[j], static_cast<uint16_t>(port));
        action_entry.init_action_data(action[j]);
        status |= action_entry.set_arg(action_prm[j], calc_label);

        // if not new entry & unsetting the label value to zero
        if ((*l4_port_entries)[port] || (set == false)) {
          status |= table.entry_modify(match_key, action_entry);
          add = false;
        } else {
          bool ret_status = false;
          status |= table.entry_add(match_key, action_entry, ret_status);
          add = true;
        }
      }
      (*l4_port_entries)[port] = calc_label;
      // set the highest bit i.e. 15th bit i.e. entry  added
      // with this flag set, entry will only be modified
      (*l4_port_entries)[port] |= 1U << 15;

      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_RANGE,
                 "{}.{}: {} Range port {} entry {} - set new label {} "
                 "[higher bit set -> entry prog]",
                 __func__,
                 __LINE__,
                 ingress ? "Ingress" : "Egress",
                 port,
                 add ? "add" : "modify",
                 (*l4_port_entries)[port]);
    }
    return status;
  }

  switch_status_t check_label() {
    // If WR/FR replay going on, ignore this check
    if (!switch_store::smiContext::context().in_warm_init()) {
      auto it = global_acl_range_label.find(type);
      if (it == global_acl_range_label.end()) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_RANGE,
                   "{}.{}: Invalid Acl L4 port range type",
                   __func__,
                   __LINE__);
        return SWITCH_STATUS_INVALID_PARAMETER;
      }
      auto &range_labels = it->second;
      if (range_bit_label == 0 && range_labels.all()) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_RANGE,
                   "{}.{}: ACL L4 port range labels unavailable for type {}",
                   __func__,
                   __LINE__,
                   type);
        return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  void del() {
    uint8_t free_bit = 0;
    uint8_t temp_bit_label = range_bit_label;
    if (temp_bit_label != 0) {
      while ((temp_bit_label & 1U) != 1U) {
        free_bit++;
        temp_bit_label >>= 1;
      }
      auto it = global_acl_range_label.find(type);
      if (it == global_acl_range_label.end()) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_RANGE,
                   "{}.{}: Invalid Acl L4 port range type",
                   __func__,
                   __LINE__);
        return;
      }
      it->second.set(free_bit, false);
    }
  }
};

class ingress_l4_src_port : public auto_object, l4_port_range {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_L4_SRC_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_L4_SRC_PORT_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_L4_SRC_PORT_ATTR_STATUS;

 public:
  ingress_l4_src_port(const switch_object_id_t p, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, p), l4_port_range(p) {
    status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) return;

    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_RANGE,
        "{}.{} parent {} ingress_l4_src_port label {} for l4 port range {}",
        __func__,
        __LINE__,
        parent,
        range_bit_label,
        range);
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      table_id.push_back(smi_id::T_INGRESS_QOS_ACL_L4_SRC_PORT);
      table_id.push_back(smi_id::T_INGRESS_IP_ACL_L4_SRC_PORT);

      field.push_back(smi_id::F_INGRESS_QOS_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT);
      field.push_back(smi_id::F_INGRESS_IP_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT);

      action.push_back(smi_id::A_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL);
      action.push_back(smi_id::A_INGRESS_IP_ACL_SET_SRC_PORT_LABEL);

      action_prm.push_back(smi_id::P_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL_LABEL);
      action_prm.push_back(smi_id::P_INGRESS_IP_ACL_SET_SRC_PORT_LABEL_LABEL);
    } else {
      table_id.push_back(smi_id::T_INGRESS_L4_SRC_PORT);
      field.push_back(smi_id::F_INGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT);
      action.push_back(smi_id::A_INGRESS_SET_SRC_PORT_LABEL);
      action_prm.push_back(smi_id::P_INGRESS_SET_SRC_PORT_LABEL_LABEL);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) {
      return status;
    }

    // If WR/FR replay going on, ignore this check
    status = check_label();
    if (status != SWITCH_STATUS_SUCCESS) {
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = auto_object::create_update();
    status = program_l4_port_entries(true, true);
    return status;
  }

  switch_status_t del() {
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) {
      return SWITCH_STATUS_SUCCESS;
    }

    l4_port_range::del();
    program_l4_port_entries(true, false);
    return auto_object::del();
  }
};

class ingress_l4_dst_port : public auto_object, l4_port_range {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_L4_DST_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_L4_DST_PORT_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_L4_DST_PORT_ATTR_STATUS;

 public:
  ingress_l4_dst_port(const switch_object_id_t p, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, p), l4_port_range(p) {
    status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) return;

    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_RANGE,
        "{}.{} parent {} ingress_l4_dst_port label {} for l4 port range {}",
        __func__,
        __LINE__,
        parent,
        range_bit_label,
        range);
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      table_id.push_back(smi_id::T_INGRESS_QOS_ACL_L4_DST_PORT);
      table_id.push_back(smi_id::T_INGRESS_IP_ACL_L4_DST_PORT);

      field.push_back(smi_id::F_INGRESS_QOS_ACL_L4_DST_PORT_LKP_L4_DST_PORT);
      field.push_back(smi_id::F_INGRESS_IP_ACL_L4_DST_PORT_LKP_L4_DST_PORT);

      action.push_back(smi_id::A_INGRESS_QOS_ACL_SET_DST_PORT_LABEL);
      action.push_back(smi_id::A_INGRESS_IP_ACL_SET_DST_PORT_LABEL);

      action_prm.push_back(smi_id::P_INGRESS_QOS_ACL_SET_DST_PORT_LABEL_LABEL);
      action_prm.push_back(smi_id::P_INGRESS_IP_ACL_SET_DST_PORT_LABEL_LABEL);
    } else {
      table_id.push_back(smi_id::T_INGRESS_L4_DST_PORT);
      field.push_back(smi_id::F_INGRESS_L4_DST_PORT_LKP_L4_DST_PORT);
      action.push_back(smi_id::A_INGRESS_SET_DST_PORT_LABEL);
      action_prm.push_back(smi_id::P_INGRESS_SET_DST_PORT_LABEL_LABEL);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) {
      return status;
    }

    // If WR/FR replay going on, ignore this check
    status = check_label();
    if (status != SWITCH_STATUS_SUCCESS) {
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = auto_object::create_update();
    status = program_l4_port_entries(true, true);
    return status;
  }

  switch_status_t del() {
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) {
      return SWITCH_STATUS_SUCCESS;
    }

    l4_port_range::del();
    program_l4_port_entries(true, false);
    return auto_object::del();
  }
};

class egress_l4_src_port : public auto_object, l4_port_range {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_L4_SRC_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_L4_SRC_PORT_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_L4_SRC_PORT_ATTR_STATUS;

 public:
  egress_l4_src_port(const switch_object_id_t p, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, p), l4_port_range(p) {
    status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) return;

    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_RANGE,
        "{}.{} parent {} egress_l4_src_port label {} for l4 port range {}",
        __func__,
        __LINE__,
        parent,
        range_bit_label,
        range);
    table_id.push_back(smi_id::T_EGRESS_L4_SRC_PORT);
    field.push_back(smi_id::F_EGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT);
    action.push_back(smi_id::A_EGRESS_SET_SRC_PORT_LABEL);
    action_prm.push_back(smi_id::P_EGRESS_SET_SRC_PORT_LABEL_LABEL);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) {
      return status;
    }

    // If WR/FR replay going on, ignore this check
    status = check_label();
    if (status != SWITCH_STATUS_SUCCESS) {
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = auto_object::create_update();
    status = program_l4_port_entries(false, true);
    return status;
  }

  switch_status_t del() {
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT) {
      return SWITCH_STATUS_SUCCESS;
    }

    l4_port_range::del();
    program_l4_port_entries(false, false);
    return auto_object::del();
  }
};

class egress_l4_dst_port : public auto_object, l4_port_range {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_L4_DST_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_L4_DST_PORT_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_L4_DST_PORT_ATTR_STATUS;

 public:
  egress_l4_dst_port(const switch_object_id_t p, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, p), l4_port_range(p) {
    status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) return;

    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_RANGE,
        "{}.{} parent {} egress_l4_dst_port label {} for l4 port range {}",
        __func__,
        __LINE__,
        parent,
        range_bit_label,
        range);
    table_id.push_back(smi_id::T_EGRESS_L4_DST_PORT);
    field.push_back(smi_id::F_EGRESS_L4_DST_PORT_LKP_L4_DST_PORT);
    action.push_back(smi_id::A_EGRESS_SET_DST_PORT_LABEL);
    action_prm.push_back(smi_id::P_EGRESS_SET_DST_PORT_LABEL_LABEL);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) {
      return status;
    }

    // If WR/FR replay going on, ignore this check
    status = check_label();
    if (status != SWITCH_STATUS_SUCCESS) {
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = auto_object::create_update();
    status = program_l4_port_entries(false, true);
    return status;
  }

  switch_status_t del() {
    if (type != SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT) {
      return SWITCH_STATUS_SUCCESS;
    }

    l4_port_range::del();
    program_l4_port_entries(false, false);
    return auto_object::del();
  }
};

class ingress_system_acl : public p4_object_match_action, acl_entry_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_SYSTEM_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_SYSTEM_ACL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_SYSTEM_ACL_ATTR_STATUS;

 public:
  ingress_system_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent),
        acl_entry_object(parent) {
    if (table_type != SWITCH_ACL_TABLE_ATTR_TYPE_SYSTEM) return;
    if (direction != SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) return;

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL,
        port_lag_label,
        port_lag_label_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_BD_LABEL, bd_label, bd_label_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, dst_mac, dst_mac_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        static_cast<uint8_t>(port_vlan_miss),
        static_cast<uint8_t>(port_vlan_miss_mask));
    status |=
        match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY,
                              static_cast<uint8_t>(acl_deny),
                              static_cast<uint8_t>(acl_deny_mask));
    status |=
        match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                              static_cast<uint8_t>(rmac_hit),
                              static_cast<uint8_t>(rmac_hit_mask));
    status |= match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP,
                                    static_cast<uint8_t>(myip),
                                    static_cast<uint8_t>(myip_mask));
    status |=
        match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD,
                              same_bd_check,
                              same_bd_check_mask);
    status |=
        match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF,
                              same_if_check,
                              same_if_check_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE, stp_state, stp_state_mask);

    if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
      status |= match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_HIT,
                                      nat_miss_type,
                                      nat_miss_type_mask);
    }

    status |= match_key.set_ip_unified_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, dst_ip, dst_ip_mask);
    if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      status |= match_key.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(3));
    } else if (dst_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      status |= match_key.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(2),
                                      static_cast<uint8_t>(3));
    }

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_TTL, ip_ttl, ip_ttl_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_src_port, l4_src_port_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, l4_dst_port, l4_dst_port_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_opcode, arp_opcode_mask);
    status |= match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                    static_cast<uint8_t>(routed),
                                    static_cast<uint8_t>(routed_mask));
    status |=
        match_key.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_SAMPLE_PACKET,
                              static_cast<uint8_t>(sample_packet),
                              static_cast<uint8_t>(sample_packet_mask));

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        static_cast<uint8_t>(ipv4_unicast_enable),
        static_cast<uint8_t>(ipv4_unicast_enable_mask));
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
        static_cast<uint8_t>(ipv6_unicast_enable),
        static_cast<uint8_t>(ipv6_unicast_enable_mask));

    status |= match_key.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, acl_priority);

    switch (packet_action.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT:
        action_entry.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP:
        action_entry.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= action_entry.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                       drop_reason);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_SFLOW_TO_CPU:
        action_entry.init_action_data(
            smi_id::A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_QID, qid);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_DISABLE_LEARNING, false);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
        action_entry.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |=
            action_entry.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID, qid);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING, false);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU:
        action_entry.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
        status |=
            action_entry.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
                                 cpu_redirect_reason_code);
        status |=
            action_entry.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_QID, qid);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_SFLOW_TO_CPU:
        action_entry.init_action_data(smi_id::A_SYSTEM_ACL_COPY_SFLOW_TO_CPU);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_QID, qid);
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        status |= action_entry.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_OVERWRITE_QID, true);
        break;
      default:
        break;
    }
  }
};

class egress_system_acl : public p4_object_match_action, acl_entry_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_SYSTEM_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_SYSTEM_ACL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_SYSTEM_ACL_ATTR_STATUS;

 public:
  egress_system_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent),
        acl_entry_object(parent) {
    if (table_type != SWITCH_ACL_TABLE_ATTR_TYPE_SYSTEM) return;
    if (direction != SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) return;
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS)) return;

    // The following comment is not correct since port and port_lag_label
    // are not the same thing. Leaving it in as a hint for now.
    //
    // status |= match_key.set_ternary<uint16_t>(
    //     smi_id::F_EGRESS_SYSTEM_ACL_EG_INTR_MD_EGRESS_PORT,
    //     static_cast<switch_eg_port_lag_label_t>(port_lag_label),
    //     static_cast<switch_eg_port_lag_label_t>(port_lag_label_mask));
    status |= match_key.set_ternary(
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU, l3_mtu, l3_mtu_mask);
    status |=
        match_key.set_exact(smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, acl_priority);

    switch (packet_action.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT:
        action_entry.init_action_data(smi_id::A_NO_ACTION);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP:
        action_entry.init_action_data(smi_id::A_EGRESS_SYSTEM_ACL_DROP);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE, drop_reason);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU:
        action_entry.init_action_data(smi_id::A_EGRESS_SYSTEM_ACL_COPY_TO_CPU);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
        action_entry.init_action_data(
            smi_id::A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= action_entry.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
            switch_enum_t target_type = {
                .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP};
            status |= switch_store::v_set(
                meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
          }
        }
        break;
      default:
        break;
    }
  }
};

#define TOS_MIRROR_ACL_ENTRY                      \
  std::pair<_MatchKey, _ActionEntry>(             \
      _MatchKey(smi_id::T_EGRESS_TOS_MIRROR_ACL), \
      _ActionEntry(smi_id::T_EGRESS_TOS_MIRROR_ACL))

class egress_tos_mirror_acl : public p4_object_match_action_list,
                              acl_entry_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_TOS_MIRROR_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_TOS_MIRROR_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_TOS_MIRROR_ACL_ATTR_PARENT_HANDLE;

 public:
  egress_tos_mirror_acl(const switch_object_id_t parent,
                        switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_TOS_MIRROR_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent),
        acl_entry_object(parent) {
    switch_object_id_t mirror_meter_handle = {};
    uint16_t mirror_sess_id = 0;
    bool ingress_mirror_action = true;
    auto it = match_action_list.begin();

    if (table_type != SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR) return;
    if (direction != SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) return;

    if (action_ingress_mirror_handle != 0) {
      status |= switch_store::v_get(action_ingress_mirror_handle,
                                    SWITCH_MIRROR_ATTR_SESSION_ID,
                                    mirror_sess_id);
      status |= switch_store::v_get(action_ingress_mirror_handle,
                                    SWITCH_MIRROR_ATTR_METER_HANDLE,
                                    mirror_meter_handle);
    } else if (action_egress_mirror_handle != 0) {
      status |= switch_store::v_get(action_egress_mirror_handle,
                                    SWITCH_MIRROR_ATTR_SESSION_ID,
                                    mirror_sess_id);
      status |= switch_store::v_get(action_egress_mirror_handle,
                                    SWITCH_MIRROR_ATTR_METER_HANDLE,
                                    mirror_meter_handle);
      ingress_mirror_action = false;
    }

    if (mirror_meter_handle.data != 0) {
      switch_enum_t target_type = {
          .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR};
      status |= switch_store::v_set(
          mirror_meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    }

    // rule for IPv4 packets
    {
      it = match_action_list.insert(it, TOS_MIRROR_ACL_ENTRY);
      status |= it->first.set_exact(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY, acl_priority);

      status |= it->first.set_ternary(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL,
          static_cast<switch_eg_port_lag_label_t>(port_lag_label),
          static_cast<switch_eg_port_lag_label_t>(port_lag_label_mask));

      status |=
          it->first.set_ternary(smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_VALID,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));

      status |= it->first.set_ternary(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_DIFFSERV,
          ip_tos,
          ip_tos_mask);

      if (ingress_mirror_action) {
        it->second.init_action_data(smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_SESSION_ID,
            mirror_sess_id);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_METER_INDEX,
            mirror_meter_handle);
      } else {
        it->second.init_action_data(smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID, mirror_sess_id);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX,
            mirror_meter_handle);
      }
    }

    // rule for IPv6 packets
    {
      it = match_action_list.insert(it, TOS_MIRROR_ACL_ENTRY);
      status |= it->first.set_exact(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY, acl_priority);

      status |= it->first.set_ternary(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL,
          static_cast<switch_eg_port_lag_label_t>(port_lag_label),
          static_cast<switch_eg_port_lag_label_t>(port_lag_label_mask));

      status |=
          it->first.set_ternary(smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_VALID,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));

      status |= it->first.set_ternary(
          smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS,
          ip_tos,
          ip_tos_mask);

      if (ingress_mirror_action) {
        it->second.init_action_data(smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_SESSION_ID,
            mirror_sess_id);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_METER_INDEX,
            mirror_meter_handle);
      } else {
        it->second.init_action_data(smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID, mirror_sess_id);
        status |= it->second.set_arg(
            smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX,
            mirror_meter_handle);
      }
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0, bytes = 0;
    status = p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      uint64_t pkts_num = 0, bytes_num = 0;
      entry.second.get_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                           &pkts_num);
      entry.second.get_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                           &bytes_num);
      pkts += pkts_num;
      bytes += bytes_num;
    }

    switch_counter_t cntr_pkts;
    cntr_pkts.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_PKTS;
    cntr_pkts.count = pkts;
    cntrs[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS] = cntr_pkts;
    switch_counter_t cntr_bytes;
    cntr_bytes.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_BYTES;
    cntr_bytes.count = bytes;
    cntrs[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES] = cntr_bytes;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;

    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES,
                           value);
      entry.second.set_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS,
                           value);
    }

    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action_list::data_get();

    for (auto entry : match_action_list) {
      for (auto id : cntr_ids) {
        switch (id) {
          case SWITCH_ACL_ENTRY_COUNTER_ID_PKTS:
            entry.second.set_arg(
                smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS, value);
            break;
          case SWITCH_ACL_ENTRY_COUNTER_ID_BYTES:
            entry.second.set_arg(
                smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES, value);
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
    status = p4_object_match_action_list::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      ctr_bytes = 0, ctr_pkt = 0;

      for (auto &entry : match_action_list) {
        uint64_t pkts_num = 0, bytes_num = 0;
        entry.second.get_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES,
                             smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                             &bytes_num);
        entry.second.get_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS,
                             smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                             &pkts_num);
        ctr_bytes += bytes_num;
        ctr_pkt += pkts_num;
      }

      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      attr_w ctr_attr_list(SWITCH_EGRESS_TOS_MIRROR_ACL_ATTR_MAU_STATS_CACHE);
      ctr_attr_list.v_set(ctr_list);
      switch_store::attribute_set(get_auto_oid(), ctr_attr_list);
    }

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_EGRESS_TOS_MIRROR_ACL_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: No stat cache to restore mau stats, "
                 "acl_entry cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (ctr_list.size() == SWITCH_ACL_ENTRY_COUNTER_ID_MAX) {
      ctr_pkt = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS];
      ctr_bytes = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES];
    }

    for (auto entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES,
                           ctr_bytes);
      entry.second.set_arg(smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS,
                           ctr_pkt);
    }

    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "acl_entry status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ecn_acl : public p4_object_match_action, acl_entry_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ECN_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECN_ACL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_ECN_ACL_ATTR_STATUS;

 public:
  ecn_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_ECN, status_attr_id, auto_ot, parent_attr_id, parent),
        acl_entry_object(parent) {
    if (table_type != SWITCH_ACL_TABLE_ATTR_TYPE_ECN) return;
    if (direction != SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) return;

    status |=
        match_key.set_ternary(smi_id::F_ECN_LOCAL_MD_INGRESS_PORT_LAG_LABEL,
                              port_lag_label,
                              port_lag_label_mask);
    status |=
        match_key.set_ternary(smi_id::F_ECN_LKP_IP_TOS, ip_tos, ip_tos_mask);
    status |= match_key.set_ternary(
        smi_id::F_ECN_LKP_TCP_FLAGS, tcp_flags, tcp_flags_mask);

    action_entry.init_action_data(smi_id::A_ECN_SET_INGRESS_COLOR);
    status |= action_entry.set_arg(smi_id::D_ECN_SET_INGRESS_COLOR_COLOR,
                                   action_color);
  }
};

/****************************************************************
 *
 * returns true/false depending on acl_table
 * bind_point_attach flag values
 *
 ***************************************************************/

bool validate_acl_table_bind_point_attach(
    const switch_object_id_t acl_table_handle) {
  bool acl_table_attach_flag = false;

  switch_store::v_get(acl_table_handle,
                      SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH,
                      acl_table_attach_flag);

  return acl_table_attach_flag;
}

class acl_factory : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ACL_FACTORY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ACL_FACTORY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_ACL_FACTORY_ATTR_STATUS;
  std::unique_ptr<object> mobject;
  switch_object_id_t table_handle = {0};

 public:
  acl_factory(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint64_t table_type = 0, direction = 0;
    switch_enum_t table_enum = {0}, direction_enum = {0};
    IdMap::instance()->Dummy();
    switch_store::v_get(
        parent, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
    switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, direction_enum);
    direction = direction_enum.enumdata;
    switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_enum);
    table_type = get_smi_acl_type(parent, table_enum.enumdata, direction);

    if (feature::is_feature_set(SWITCH_FEATURE_INGRESS_ACL) ||
        feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL)) {
      if (!validate_acl_table_bind_point_attach(table_handle)) {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "Couldn't program ACL entry, as bind_point is not attached");
        return;
      }
    }

    if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
      if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_SYSTEM) {
        mobject = std::unique_ptr<ingress_system_acl>(
            new ingress_system_acl(parent, status));
      } else if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_ECN) {
        mobject = std::unique_ptr<ecn_acl>(new ecn_acl(parent, status));
      } else if ((table_type != SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_ETRAP) &&
                 (table_type != SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_ETRAP)) {
        mobject = std::unique_ptr<acl_hw_entry>(
            new acl_hw_entry(parent, status, table_type, direction));
      }
    } else if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
      if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_SYSTEM) {
        mobject = std::unique_ptr<egress_system_acl>(
            new egress_system_acl(parent, status));
      } else if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR) {
        mobject = std::unique_ptr<egress_tos_mirror_acl>(
            new egress_tos_mirror_acl(parent, status));
      } else {
        mobject = std::unique_ptr<acl_hw_entry>(
            new acl_hw_entry(parent, status, table_type, direction));
      }
    } else if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS) {
      if (table_type == SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS) {
        mobject = std::unique_ptr<acl_hw_entry>(
            new acl_hw_entry(parent, status, table_type, direction));
      }
    }
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->create_update();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Failed to add ACL entry, status:{}",
                   "acl_factory",
                   __LINE__,
                   status);
        return status;
      }
      std::vector<switch_object_id_t> acl_entry_handles;
      status = switch_store::v_get(table_handle,
                                   SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES,
                                   acl_entry_handles);
      // update port/bd label for first entry
      if (acl_entry_handles.size() == 0) {
        std::set<switch_object_id_t> acl_grp_member_list;
        switch_store::referencing_set_get(table_handle,
                                          SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER,
                                          acl_grp_member_list);
        for (auto acl_member : acl_grp_member_list) {
          compute_acl_group_label(acl_member, false, true);
          switch_acl_update_port_group_index(acl_member);
        }
      }
    } else {
      std::vector<switch_object_id_t> acl_entry_handles;
      status = switch_store::v_get(table_handle,
                                   SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES,
                                   acl_entry_handles);
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}.{} acl_entry:{}",
                 __func__,
                 __LINE__,
                 acl_entry_handles.size());
      if (acl_entry_handles.size() == 0) {
        std::set<switch_object_id_t> acl_grp_member_list;
        switch_store::referencing_set_get(table_handle,
                                          SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER,
                                          acl_grp_member_list);
        for (auto acl_member : acl_grp_member_list) {
          compute_acl_group_label(acl_member, false, true);
          switch_acl_update_port_group_index(acl_member);
        }
      }
    }
    return auto_object::create_update();
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->del();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Failed to delete ACL entry, status:{}",
                   "acl_factory",
                   __LINE__,
                   status);
        return status;
      }
      std::vector<switch_object_id_t> acl_entry_handles;
      status = switch_store::v_get(table_handle,
                                   SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES,
                                   acl_entry_handles);
      // update port/bd label for last entry
      if (acl_entry_handles.size() == 0) {
        std::set<switch_object_id_t> acl_grp_member_list;
        switch_store::referencing_set_get(table_handle,
                                          SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER,
                                          acl_grp_member_list);
        for (auto acl_member : acl_grp_member_list) {
          compute_acl_group_label(acl_member, true, false);
        }
      }
    }
    return auto_object::del();
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->counters_get(handle, cntrs);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Failed counters_get for ACL entry, status:{}",
                   "acl_factory",
                   __LINE__,
                   status);
        return status;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->counters_set(handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Failed counters_set for ACL entry, status:{}",
                   "acl_factory",
                   __LINE__,
                   status);
        return status;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->counters_set(handle, cntr_ids);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Failed counters_set for ACL entry, status:{}",
                   "acl_factory",
                   __LINE__,
                   status);
        return status;
      }
    }
    return status;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->counters_save(parent);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Fail counters save for ACL entry, status:{}",
                   "acl_factory parent {:#x}",
                   __LINE__,
                   status,
                   parent.data);
        return status;
      }
    }
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (mobject != NULL) {
      status = mobject->counters_restore(parent);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ACL_ENTRY,
                   "{}.{}: Fail counters restore for ACL entry, status:{}",
                   "acl_factory parent {:#x}",
                   __LINE__,
                   status,
                   parent.data);
        return status;
      }
    }
    return status;
  }
};

switch_status_t get_acl_table_type_dir(switch_object_id_t acl_entry,
                                       switch_enum_t &table_type,
                                       switch_enum_t &dir) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};

  status |= switch_store::v_get(
      acl_entry, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, dir);
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type);
  table_type.enumdata =
      get_smi_acl_type(acl_entry, table_type.enumdata, dir.enumdata);
  return status;
}

class device_pre_ingress_acl : public auto_object {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_PRE_INGRESS_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_PRE_INGRESS_ACL_ATTR_PARENT_HANDLE;
  switch_object_id_t pre_ingress_acl = {};

  switch_status_t set_acl_status(bool enabled) {
    _Table table(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_PRE_INGRESS_DEIVCE_TO_ACL);
    _ActionEntry action_entry(smi_id::T_PRE_INGRESS_DEIVCE_TO_ACL);
    action_entry.init_action_data(smi_id::A_PRE_INGRESS_SET_ACL_STATUS);
    action_entry.set_arg(smi_id::D_PRE_INGRESS_SET_ACL_STATUS_ENABLED, enabled);
    return table.default_entry_set(action_entry, false);
  }

 public:
  device_pre_ingress_acl(const switch_object_id_t parent,
                         switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (!feature::is_feature_set(SWITCH_FEATURE_PRE_INGRESS_ACL)) return;

    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_PRE_INGRESS_ACL, pre_ingress_acl);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (feature::is_feature_set(SWITCH_FEATURE_PRE_INGRESS_ACL)) {
      status = set_acl_status(pre_ingress_acl.data != 0);
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    return auto_object::create_update();
  }

  switch_status_t del() {
    if (feature::is_feature_set(SWITCH_FEATURE_PRE_INGRESS_ACL))
      set_acl_status(false);
    return auto_object::del();
  }
};

/**
 * Why is this needed here?
 * There is no drop_flag in p4_16 unlike p4_14. So we need one entry per
 * drop_reason installed. We can revisit this if we get a drop_flag
 */
switch_drop_reason_t malformed_ip_drop_reasons[] = {
    SWITCH_DROP_REASON_NEXTHOP,
    SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO,
    SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_IP_TTL_ZERO,
    SWITCH_DROP_REASON_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_IP_IHL_INVALID,
    SWITCH_DROP_REASON_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN};

switch_drop_reason_t malformed_l2_drop_reasons[] = {
    SWITCH_DROP_REASON_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_DST_MAC_ZERO,
    SWITCH_DROP_REASON_SRC_MAC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST};

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

// Remember to add newer entries in the order in which SAI has them
// These are the highest priority entries starting with priority 1
#define SYSTEM_ACL_ENTRY                       \
  std::pair<_MatchKey, _ActionEntry>(          \
      _MatchKey(smi_id::T_INGRESS_SYSTEM_ACL), \
      _ActionEntry(smi_id::T_INGRESS_SYSTEM_ACL))
class default_ingress_system_acl : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFAULT_INGRESS_SYSTEM_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFAULT_INGRESS_SYSTEM_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFAULT_INGRESS_SYSTEM_ACL_ATTR_PARENT_HANDLE;

 public:
  default_ingress_system_acl(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_enum_t ing_target_type = {
        .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
    struct ip_addr_with_mask {
      switch_ip_type_t ip_type;
      switch_ip_address_t ip;
      switch_ip_address_t ip_mask;
      bf_rt_id_t v4_v6_enable;
    };

    switch_object_id_t cpu_handle = {0};
    const std::vector<uint16_t> l3_bd_labels = {0x1000};

    status =
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_CPU_PORT, cpu_handle);
    if (cpu_handle == 0) return;

    switch_object_id_t dflt_trap_grp = {};
    switch_object_id_t dflt_trap_grp_meter{};
    uint8_t dflt_trap_grp_qid = 0;
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP, dflt_trap_grp);
    get_trap_group_cfg(dflt_trap_grp, dflt_trap_grp_meter, dflt_trap_grp_qid);

    auto it = match_action_list.begin();

    // the default entries all have low priorities
    uint32_t prio = SWITCH_INTERNAL_ACL_LOW_PRIO_START;

    /**************************************************************************
     * Entries for actions deny - must be highest priority
     *************************************************************************/

    // ACL deny
    if (feature::is_feature_set(
            SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION)) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(
          smi_id::F_SYSTEM_ACL_PRIORITY,
          static_cast<uint32_t>(SWITCH_INTERNAL_ACL_DENY_HIGH_PRIO));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_COPY_CANCEL,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DENY);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DENY_DROP_REASON,
                                   SWITCH_DROP_REASON_ACL_DENY);
    }

    /**************************************************************************
     * Begin L2 drop reasons
     *************************************************************************/

    // l2 drop reason entries
    {
      for (uint8_t i = 0;
           i < sizeof(malformed_l2_drop_reasons) / sizeof(switch_drop_reason_t);
           i++) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        if (smi_id::F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON) {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON,
              static_cast<uint8_t>(malformed_l2_drop_reasons[i]),
              static_cast<uint8_t>(0xFF));
        } else {
          // Profiles with a combined drop_reason field
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
              static_cast<uint8_t>(malformed_l2_drop_reasons[i]),
              static_cast<uint8_t>(0xFF));
        }
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     malformed_l2_drop_reasons[i]);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING,
                                     true);
      }
    }

    // same mac check
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK),
          static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(0),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    // Reserved DMAC(01-80-C2-00-00-0x) and !trap , drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      if (smi_id::F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_DMAC_RESERVED),
            static_cast<uint8_t>(0xFF));
      } else {
        // Profiles with a combined drop_reason field
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_DMAC_RESERVED),
            static_cast<uint8_t>(0xFF));
      }
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_DMAC_RESERVED));
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    // port vlan mapping miss, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      if (smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
            static_cast<uint8_t>(1),
            static_cast<uint8_t>(1));
      } else {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS),
            static_cast<uint8_t>(0xFF));
      }
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    if (feature::is_feature_set(SWITCH_FEATURE_STP)) {
      const uint8_t ingress_stp_state_mask = 0x3;
      if (smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE) {
        // STP state == blocked (01), drop and don't learn
        {
          it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
          status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE,
              static_cast<uint8_t>(SWITCH_STP_PORT_ATTR_STATE_BLOCKING),
              ingress_stp_state_mask);
          it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
          status |=
              it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                 SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING);
          status |= it->second.set_arg(
              smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
        }
        // STP state == learning (10), drop and learn
        {
          it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
          status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE,
              static_cast<uint8_t>(SWITCH_STP_PORT_ATTR_STATE_LEARNING),
              ingress_stp_state_mask);
          it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
          status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                       SWITCH_DROP_REASON_STP_STATE_LEARNING);
        }
      } else {
        // STP state == blocked (01), drop and don't learn
        {
          it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
          status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
              static_cast<uint8_t>(
                  SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING),
              static_cast<uint8_t>(0xFF));
          it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
          status |=
              it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                 SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING);
          status |= it->second.set_arg(
              smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
        }
        // STP state == learning (10), drop and learn
        {
          it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
          status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
              static_cast<uint8_t>(SWITCH_DROP_REASON_STP_STATE_LEARNING),
              static_cast<uint8_t>(0xFF));
          it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
          status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                       SWITCH_DROP_REASON_STP_STATE_LEARNING);
        }
      }
    }

    // L2 unicast self fwd check, !routed && egress_port == ingress_port
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
                                static_cast<uint8_t>(SWITCH_PKT_TYPE_UNICAST),
                                static_cast<uint8_t>(0x3));
      if (smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF) {
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF,
                                  static_cast<uint16_t>(0),
                                  static_cast<uint16_t>(PORT_LAG_INDEX_MASK));
      } else {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_SAME_IFINDEX),
            static_cast<uint8_t>(0xFF));
      }
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(0),
                                static_cast<uint8_t>(1));

      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_DECAP)) {
        // The case when tunnel is terminated and the inner packet is forwarded
        // back to the same port is OK.
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_TUNNEL_TERMINATE,
            static_cast<uint8_t>(0),
            static_cast<uint8_t>(1));
      }
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_SAME_IFINDEX);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    /**************************************************************************
     * Begin L3 drop reasons
     *************************************************************************/

    // ip drop reason entries
    {
      for (uint8_t i = 0;
           i < sizeof(malformed_ip_drop_reasons) / sizeof(switch_drop_reason_t);
           i++) {
        // ipv4 enable + rmac_hit
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(malformed_ip_drop_reasons[i]),
            static_cast<uint8_t>(0xFF));
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
            static_cast<uint8_t>(1),
            static_cast<uint8_t>(1));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                  static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                  static_cast<uint8_t>(3));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     malformed_ip_drop_reasons[i]);

        // ipv6 enable + rmac_hit
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(malformed_ip_drop_reasons[i]),
            static_cast<uint8_t>(0xFF));
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
            static_cast<uint8_t>(1),
            static_cast<uint8_t>(1));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                  static_cast<uint8_t>(SWITCH_IP_TYPE_IPV6),
                                  static_cast<uint8_t>(3));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     malformed_ip_drop_reasons[i]);
      }
    }

    // ipv4 unicast disable
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(0),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_L3_IPV4_DISABLE);
    }

    // ipv6 unicast disable
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(0),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV6),
                                      static_cast<uint8_t>(3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_L3_IPV6_DISABLE);
    }

    // ipmc dmac does not contain encoded ip
    if (feature::is_feature_set(SWITCH_FEATURE_IPMC_DMAC_VALIDATION)) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH),
          static_cast<uint8_t>(0xFF));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                             SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH);
    }

    // blackhole route drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_DROP,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    /* ip_options == valid, vrf_ip_options_action == redirect_to_cpu */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION,
          static_cast<uint8_t>(SWITCH_PACKET_ACTION_TRAP),
          static_cast<uint8_t>(0x3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_IP_OPTIONS));
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID,
                                   dflt_trap_grp_qid);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, dflt_trap_grp_meter);
      if (dflt_trap_grp_meter.data != 0) {
        status |= switch_store::v_set(dflt_trap_grp_meter,
                                      SWITCH_METER_ATTR_TARGET_TYPE,
                                      ing_target_type);
      }
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING, false);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
    }

    /* ip_options == valid, vrf_ip_options_action == copy_to_cpu */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION,
          static_cast<uint8_t>(SWITCH_PACKET_ACTION_COPY),
          static_cast<uint8_t>(0x3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_IP_OPTIONS));
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_QID,
                                   dflt_trap_grp_qid);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_METER_ID,
                                   dflt_trap_grp_meter);
      if (dflt_trap_grp_meter.data != 0) {
        status |= switch_store::v_set(dflt_trap_grp_meter,
                                      SWITCH_METER_ATTR_TARGET_TYPE,
                                      ing_target_type);
      }
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID, true);
    }

    /* ip_options == valid, vrf_ip_options_action == drop */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION,
          static_cast<uint8_t>(SWITCH_PACKET_ACTION_DROP),
          static_cast<uint8_t>(0x3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                             static_cast<uint8_t>(SWITCH_DROP_REASON_UNKNOWN));
    }

    /* ip_options == valid, vrf_ip_options_action == permit */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION,
          static_cast<uint8_t>(SWITCH_PACKET_ACTION_PERMIT),
          static_cast<uint8_t>(0x3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    }

    // rmac_hit, non IP
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(0),
                                      static_cast<uint8_t>(3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_NON_IP_ROUTER_MAC);
    }

    // routed, ttl == 1, in_port == cpu, permit
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TTL,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          compute_port_lag_index(cpu_handle),
          static_cast<uint16_t>(PORT_LAG_INDEX_MASK));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    }

    // routed, ttl == 1, redirect to cpu
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TTL,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(SWITCH_HOSTIF_TRAP_ATTR_TYPE_TTL_ERROR));
    }

    // link local src ipv4 and !routed, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);

      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));

      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));

      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));

      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL);
    }

    // link_local dst ipv4 and routed, drop
    {
      switch_ip_address_t link_local_ip = {SWITCH_IP_ADDR_FAMILY_IPV4,
                                           0xA9FE0000};
      switch_ip_address_t link_local_ip_mask = {SWITCH_IP_ADDR_FAMILY_IPV4,
                                                0xFFFF0000};

      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);

      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));

      status |=
          it->first.set_ip_unified_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
                                           link_local_ip,
                                           link_local_ip_mask);

      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));

      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_DST_LINK_LOCAL);
    }

    // routed, ipv6_src_is_link_local == 1, in_port == cpu, permit
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          compute_port_lag_index(cpu_handle),
          static_cast<uint16_t>(PORT_LAG_INDEX_MASK));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    }

    // routed, ipv6_src_is_link_local == 1, redirect to cpu
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_SRC_IS_LINK_LOCAL));
    }

    // myip host, redirect to cpu
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP,
          static_cast<uint8_t>(SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST),
          static_cast<uint8_t>(SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP));
    }

    // myip subnet, redirect to cpu
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP,
          static_cast<uint8_t>(SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET),
          static_cast<uint8_t>(SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP_SUBNET));
    }

    // glean, redirect to cpu
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_GLEAN,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_GLEAN));
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, false);
    }

    // ip dst addr is loopback, drop
    {
      // clang-format off
      const ip_addr_with_mask loopback_ip_addrs[] = {
        // IP=127.0.0.0/8
        {SWITCH_IP_TYPE_IPV4, {SWITCH_IP_ADDR_FAMILY_IPV4, 0x7F000000}, {SWITCH_IP_ADDR_FAMILY_IPV4, 0xFF000000}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE},
        // IP=::1/128
        {SWITCH_IP_TYPE_IPV6, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1}}}, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE},
        // IP=0:0:0:0:0:ffff:7f00:0/104
        {SWITCH_IP_TYPE_IPV6, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0x7F, 0x0, 0x0, 0x0}}}, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x0}}}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE}
      };
      // clang-format on

      for (uint8_t i = 0;
           i < sizeof(loopback_ip_addrs) / sizeof(ip_addr_with_mask);
           i++) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
            static_cast<uint8_t>(loopback_ip_addrs[i].ip_type),
            static_cast<uint8_t>(3));
        status |= it->first.set_ip_unified_ternary(
            smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
            loopback_ip_addrs[i].ip,
            loopback_ip_addrs[i].ip_mask);
        status |= it->first.set_ternary(loopback_ip_addrs[i].v4_v6_enable,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK);
      }
    }

    // Multicast dst IPV6 scope 0 reserved
    {
      // clang-format off
      const ip_addr_with_mask mc_ip_addrs = {
        SWITCH_IP_TYPE_IPV6, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}}, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF,  0xFF,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0}}}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE};
      // clang-format on
      // IP=ff:x0/16
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(mc_ip_addrs.ip_type),
                                      static_cast<uint8_t>(3));
      status |=
          it->first.set_ip_unified_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
                                           mc_ip_addrs.ip,
                                           mc_ip_addrs.ip_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV6),
                                      static_cast<uint8_t>(3));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IPV6_MC_SCOPE0);
    }

    // Multicast dst IPV6 scope 1 reserved
    {
      // clang-format off
      const ip_addr_with_mask mc_ip_addrs_1 = {
        SWITCH_IP_TYPE_IPV6, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF,  0x01,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0}}}, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF,  0xFF,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0}}}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE};
      // IP=ff:x1/16
      // clang-format on
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                static_cast<uint8_t>(mc_ip_addrs_1.ip_type),
                                static_cast<uint8_t>(3));
      status |=
          it->first.set_ip_unified_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
                                           mc_ip_addrs_1.ip,
                                           mc_ip_addrs_1.ip_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV6),
                                      static_cast<uint8_t>(3));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IPV6_MC_SCOPE1);
    }

    // unspecified dst ip, drop
    {
      // clang-format off
      const ip_addr_with_mask zero_ip_addrs[] = {
        // IP=0.0.0.0/32
        {SWITCH_IP_TYPE_IPV4, {SWITCH_IP_ADDR_FAMILY_IPV4, 0x0}, {SWITCH_IP_ADDR_FAMILY_IPV4, 0xFFFFFFFF}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE},
        // IP=::0/128
        {SWITCH_IP_TYPE_IPV6, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0x0}}}, {SWITCH_IP_ADDR_FAMILY_IPV6, {.ip6 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}}, smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE}
      };
      // clang-format on

      for (uint8_t i = 0; i < sizeof(zero_ip_addrs) / sizeof(ip_addr_with_mask);
           i++) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
            static_cast<uint8_t>(zero_ip_addrs[i].ip_type),
            static_cast<uint8_t>(3));
        status |= it->first.set_ternary(zero_ip_addrs[i].v4_v6_enable,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        status |= it->first.set_ip_unified_ternary(
            smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
            zero_ip_addrs[i].ip,
            zero_ip_addrs[i].ip_mask);
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_IP_DST_UNSPECIFIED);
      }
    }

    // unspecified src ip, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED),
          static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED);
    }

    // lpm4 miss, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_UNKNOWN),
          static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_LPM_MISS,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_LPM4_MISS);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    // lpm6 miss, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_UNKNOWN),
          static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_LPM_MISS,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV6),
                                      static_cast<uint8_t>(3));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_LPM6_MISS);
      status |=
          it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
    }

    // L3 self fwd check, routed & ingress bd == egress bd, default allow
    if (feature::is_feature_set(SWITCH_FEATURE_L3_UNICAST_SELF_FWD_CHECK)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
                                  static_cast<uint8_t>(SWITCH_PKT_TYPE_UNICAST),
                                  static_cast<uint8_t>(0x3));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        if (smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF) {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF,
              static_cast<uint16_t>(0),
              static_cast<uint16_t>(PORT_LAG_INDEX_MASK));
        } else {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
              static_cast<uint8_t>(SWITCH_DROP_REASON_SAME_IFINDEX),
              static_cast<uint8_t>(0xFF));
        }
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD,
                                  static_cast<uint16_t>(0),
                                  static_cast<uint16_t>(0xFFFF));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
        /*
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
            static_cast<uint16_t>(SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICMP_REDIRECT));
        */
      }
    }

    // src ipv4 == broadcast, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);

      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
                                static_cast<uint8_t>(SWITCH_DROP_REASON_SIP_BC),
                                static_cast<uint8_t>(0xFF));

      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_SIP_BC);
    }

    // src ipv4 is class e and pkt_type == unicast, drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);

      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(SWITCH_IP_TYPE_IPV4),
                                      static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E),
          static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
                                static_cast<uint8_t>(SWITCH_PKT_TYPE_UNICAST),
                                static_cast<uint8_t>(0x3));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_IP_SRC_CLASS_E);
    }

    // MPLS
    if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
      // mpls disable
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_ENABLE,
                                      static_cast<uint8_t>(0),
                                      static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_MPLS_DISABLE);
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);

      // MPLS trap
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_TRAP,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_MPLS_TRAP));

      // MPLS drop
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_MPLS_LABEL_DROP),
          static_cast<uint8_t>(0xFF));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(
          smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_MPLS_LABEL_DROP));
    }

    // SRv6
    if (feature::is_feature_set(SWITCH_FEATURE_SRV6)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        // SRV6 trap
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_SRV6_TRAP,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            static_cast<uint16_t>(
                SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_SRV6_TRAP));

        // SRV6 drop
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_SRV6_MY_SID_DROP),
            static_cast<uint8_t>(0xFF));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_SRV6_MY_SID_DROP));
      }
    }

    // IGMP + non_routable, drop
    {
      for (auto bd_label : l3_bd_labels) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LKP_IP_PROTO,
            static_cast<uint8_t>(SWITCH_HOSTIF_IP_PROTO_IGMP),
            static_cast<uint8_t>(0xFF));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_NON_ROUTABLE));
      }
    }

    if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
      auto vrf_unknown_mcast_match = [&](switch_ip_type_t ip_type, bool trap) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
            static_cast<uint8_t>(SWITCH_PKT_TYPE_MULTICAST),
            static_cast<uint8_t>(0x3));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_MULTICAST_HIT,
                                  static_cast<uint8_t>(0x0),
                                  static_cast<uint8_t>(0x1));
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                        static_cast<uint8_t>(ip_type),
                                        static_cast<uint8_t>(0x3));
        if (ip_type == SWITCH_IP_TYPE_IPV4) {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_ENABLE,
              static_cast<uint8_t>(0x1),
              static_cast<uint8_t>(0x1));
        } else {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_ENABLE,
              static_cast<uint8_t>(0x1),
              static_cast<uint8_t>(0x1));
        }
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_UNKNOWN_L3_MULTICAST_TRAP,
            static_cast<uint8_t>(trap),
            static_cast<uint8_t>(0x1));
      };

      for (uint16_t bd_label : l3_bd_labels) {
        /* pkt_type == multicast, ip_type == ipv4, ipv4_multicast_enabled == 1,
         * multicast_hit == 0, vrf_unknown_l3_multicast_action == trap */
        vrf_unknown_mcast_match(SWITCH_IP_TYPE_IPV4, true);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            static_cast<uint16_t>(
                SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_UNKNOWN_L3_MULTICAST));
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID,
                                     dflt_trap_grp_qid);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, dflt_trap_grp_meter);
        if (dflt_trap_grp_meter.data != 0) {
          status |= switch_store::v_set(dflt_trap_grp_meter,
                                        SWITCH_METER_ATTR_TARGET_TYPE,
                                        ing_target_type);
        }
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING, false);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
      }

      for (uint16_t bd_label : l3_bd_labels) {
        /* pkt_type == multicast, ip_type == ipv4, ipv4_multicast_enabled == 1,
         * multicast_hit == 0, vrf_unknown_l3_multicast_action == drop */
        vrf_unknown_mcast_match(SWITCH_IP_TYPE_IPV4, false);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
            static_cast<uint8_t>(
                SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_UNKNOWN_L3_MULTICAST));
      }

      for (uint16_t bd_label : l3_bd_labels) {
        /* pkt_type == multicast, ip_type == ipv6, ipv6_multicast_enabled == 1,
         * multicast_hit == 0, vrf_unknown_l3_multicast_action == trap */
        vrf_unknown_mcast_match(SWITCH_IP_TYPE_IPV6, true);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            static_cast<uint16_t>(
                SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_UNKNOWN_L3_MULTICAST));
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID,
                                     dflt_trap_grp_qid);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, dflt_trap_grp_meter);
        if (dflt_trap_grp_meter.data != 0) {
          status |= switch_store::v_set(dflt_trap_grp_meter,
                                        SWITCH_METER_ATTR_TARGET_TYPE,
                                        ing_target_type);
        }
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING, false);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
      }

      for (uint16_t bd_label : l3_bd_labels) {
        /* pkt_type == multicast, ip_type == ipv6, ipv6_multicast_enabled == 1,
         * multicast_hit == 0, vrf_unknown_l3_multicast_action == drop */
        vrf_unknown_mcast_match(SWITCH_IP_TYPE_IPV6, false);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
            static_cast<uint8_t>(
                SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_UNKNOWN_L3_MULTICAST));
      }
    }

    // L3 port, rmac miss, drop
    {
      for (auto bd_label : l3_bd_labels) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                  static_cast<uint8_t>(0),
                                  static_cast<uint8_t>(1));
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
                                  static_cast<uint8_t>(SWITCH_PKT_TYPE_UNICAST),
                                  static_cast<uint8_t>(0x3));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_L3_PORT_RMAC_MISS));
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING,
                                     true);
      }
    }

    // ucast/bcast dip, mc mac, drop
    /*
     * Drop packets that have mismatched IP/MAC addresses received on L3
     * interfaces. Pure L3 interface can detected base on BD values.
     * Current BD width is 16-bit and pure L3 BDs are after 4096.
     * Any packets from pure L2 interface will have BD value 1 - 4095
     * and below ACL entries will not match for those packets.
     */
    {
      if (sai_mode()) {
        typedef struct {
          switch_ip_addr_family_t addr_family;
          union {
            switch_ip4_t ip4_addr;
            switch_ip6_t ip6_addr;
          };
          union {
            switch_ip4_t ip4_mask;
            switch_ip6_t ip6_mask;
          };
          switch_ip_address_t get_addr() const {
            switch_ip_address_t addr{.addr_family = addr_family};
            if (addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
              addr.ip4 = ip4_addr;
            else
              memcpy(addr.ip6, ip6_addr, sizeof(switch_ip6_t));
            return addr;
          }
          switch_ip_address_t get_mask() const {
            switch_ip_address_t addr{.addr_family = addr_family};
            if (addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
              addr.ip4 = ip4_mask;
            else
              memcpy(addr.ip6, ip6_mask, sizeof(switch_ip6_t));
            return addr;
          }
        } mcast_adress_and_mask_t;

        const std::vector<mcast_adress_and_mask_t> mcast_adress_and_mask_array{
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
             {.ip4_addr = 0},
             {.ip4_mask = 0x80000000}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
             {.ip4_addr = 0},
             {.ip4_mask = 0x40000000}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
             {.ip4_addr = 0},
             {.ip4_mask = 0x20000000}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
             {.ip4_addr = 0x10000000},
             {.ip4_mask = 0x10000000}},  // for bcast
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x80}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x40}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x20}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x10}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x08}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x04}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x02}}},
            {.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
             {.ip6_addr = {0}},
             {.ip6_mask = {0x01}}},
        };
        const uint32_t pkt_types[]{SWITCH_PKT_TYPE_MULTICAST,
                                   SWITCH_PKT_TYPE_BROADCAST};

        for (const auto bd_label : l3_bd_labels) {
          for (const auto &mcast_adress_and_mask :
               mcast_adress_and_mask_array) {
            for (const auto pkt_type : pkt_types) {
              it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
              status |=
                  it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
              status |= it->first.set_ternary(
                  smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                  static_cast<uint8_t>((mcast_adress_and_mask.addr_family ==
                                        SWITCH_IP_ADDR_FAMILY_IPV4)
                                           ? SWITCH_IP_TYPE_IPV4
                                           : SWITCH_IP_TYPE_IPV6),
                  static_cast<uint8_t>(3));
              status |= it->first.set_ternary(
                  smi_id::F_SYSTEM_ACL_LOCAL_MD_BD, bd_label, bd_label);
              status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE,
                                              static_cast<uint8_t>(pkt_type),
                                              static_cast<uint8_t>(0x3));
              status |= it->first.set_ip_unified_ternary(
                  smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR,
                  mcast_adress_and_mask.get_addr(),
                  mcast_adress_and_mask.get_mask());
              it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);

              if ((mcast_adress_and_mask.addr_family ==
                   SWITCH_IP_ADDR_FAMILY_IPV4) &&
                  (mcast_adress_and_mask.ip4_addr == 0x10000000)) {
                status |= it->second.set_arg(
                    smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                    static_cast<uint8_t>(
                        SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST));
              } else {
                status |= it->second.set_arg(
                    smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                    static_cast<uint8_t>(
                        SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST));
              }

              status |= it->second.set_arg(
                  smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING, true);
            }
          }
        }
      }
    }

    /**************************************************************************
     * Begin ACL, QoS and rest
     *************************************************************************/

    // ACL drop
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_HOSTIF_TRAP_ID,
                                static_cast<uint8_t>(0),
                                static_cast<uint8_t>(0xFF));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                   SWITCH_DROP_REASON_ACL_DROP);
    }

    // RACL deny, drop
    if (feature::is_feature_set(SWITCH_FEATURE_INGRESS_RACL)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RACL_DENY,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_RACL_DENY);
      }
    }

    // Ingress ACL meter drop
    if (feature::is_feature_set(SWITCH_FEATURE_INGRESS_ACL_METER) &&
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION,
            SWITCH_PACKET_ACTION_DROP,
            static_cast<uint8_t>(0x3));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_INGRESS_ACL_METER);
      }
    }

    // Ingress IP QoS ACL meter drop
    if (feature::is_feature_set(SWITCH_FEATURE_INGRESS_ACL_METER) &&
        smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_DROP_REASON_INGRESS_ACL_METER),
            static_cast<uint8_t>(0xFF));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_INGRESS_ACL_METER);
      }
    }

    // Ingress Port meter drop
    if (feature::is_feature_set(SWITCH_FEATURE_INGRESS_PORT_METER)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP,
            static_cast<uint8_t>(1),
            static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_INGRESS_PORT_METER);
      }
    }

    // storm_control_color yellow, drop
    // storm_control_color red, drop
    if (feature::is_feature_set(SWITCH_FEATURE_STORM_CONTROL)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_STORM_CONTROL_COLOR,
            static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW),
            static_cast<uint8_t>(0x1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |=
            it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                               SWITCH_DROP_REASON_STORM_CONTROL_COLOR_RED);
      }
    }

    // pfc_wd drop
    if (feature::is_feature_set(SWITCH_FEATURE_PFC_WD)) {
      {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, ++prio);
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP,
            static_cast<uint8_t>(1),
            static_cast<uint8_t>(1));
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP);
      }
    }
  }
};

#define MATCH_ACTION_ITERATOR \
  std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator

class default_egress_system_acl_object {
 public:
  ~default_egress_system_acl_object() {}

  struct field_spec {
    field_spec(bf_rt_field_id_t f_id,
               uint64_t k,
               uint64_t m = {0xffffffffffffffff}) {
      field_id = f_id;
      key = k;
      mask = m;
    }

    bf_rt_field_id_t field_id{0};
    uint64_t key;
    uint64_t mask;
  };

  struct rule_spec {
    std::vector<field_spec> fields{};
    uint8_t drop_reason;
    bf_rt_action_id_t action_id{smi_id::A_EGRESS_SYSTEM_ACL_DROP};
  };

  std::vector<rule_spec> rules = {};

  switch_status_t add_match_action(MATCH_ACTION_ITERATOR it,
                                   rule_spec rule,
                                   std::vector<bf_rt_id_t> cntrs = {}) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (auto field : rule.fields) {
      if (field.field_id == smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY) {
        status |= it->first.set_exact(field.field_id, field.key);
        continue;
      }
      status |= it->first.set_ternary(field.field_id, field.key, field.mask);
    }

    if (cntrs.empty()) {
      it->second.init_action_data(rule.action_id);
    } else {
      it->second.init_action_data(rule.action_id, cntrs);
    }

    status |= it->second.set_arg(smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE,
                                 rule.drop_reason);

    return status;
  }

  explicit default_egress_system_acl_object() {
    auto rule = rules.begin();
    uint32_t prio = SWITCH_INTERNAL_ACL_LOW_PRIO_START;

    // l3 mtu check fail
    {
      rule = rules.insert(rule, rule_spec{});
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU, 0, 0xFFFF});
      rule->drop_reason = SWITCH_DROP_REASON_MTU_CHECK_FAIL;
    }

    // STP state == blocked, drop
    if (feature::is_feature_set(SWITCH_FEATURE_STP)) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_STP,
                     SWITCH_STP_PORT_ATTR_STATE_BLOCKING,
                     0x1});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_STP_STATE_BLOCKING;
    }

    // acl_deny drop
    {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY, 1, 1});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_DROP;
    }

    // wred_drop
    if (feature::is_feature_set(SWITCH_FEATURE_WRED)) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_WRED_DROP, 1, 1});
      rule->drop_reason = SWITCH_DROP_REASON_WRED;
    }

    // pfc_wd_drop
    if (feature::is_feature_set(SWITCH_FEATURE_PFC_WD)) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP, 1, 1});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_PFC_WD_DROP;
    }

    // Egress ACL meter drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL_METER) &&
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION,
          SWITCH_PACKET_ACTION_DROP,
          0x3});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_METER;
    }

    // Egress IP QOS ACL meter yellow drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL_METER) &&
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR,
                     static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW),
                     static_cast<uint8_t>(0x3)});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_METER;
    }

    // Egress IP QOS ACL meter red drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL_METER) &&
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR,
                     static_cast<uint8_t>(SWITCH_PKT_COLOR_RED),
                     static_cast<uint8_t>(0x3)});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_METER;
    }

    // Egress IP Mirror ACL meter yellow drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL_METER) &&
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR,
          static_cast<uint8_t>(SWITCH_PKT_COLOR_YELLOW),
          static_cast<uint8_t>(0x3)});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_METER;
    }

    // Egress IP Mirror ACL meter red drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_ACL_METER) &&
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR,
          static_cast<uint8_t>(SWITCH_PKT_COLOR_RED),
          static_cast<uint8_t>(0x3)});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_ACL_METER;
    }

    // Egress Port meter drop
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_PORT_METER)) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP, 1, 1});
      rule->drop_reason = SWITCH_DROP_REASON_EGRESS_PORT_METER;
    }

    // isolation_packet_drop
    if (feature::is_feature_set(SWITCH_FEATURE_PORT_ISOLATION)) {
      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_ISOLATION_PACKET_DROP,
          1,
          1});
      rule->drop_reason = SWITCH_DROP_REASON_PORT_ISOLATION_DROP;

      rule = rules.insert(rule, rule_spec());
      rule->fields.push_back(
          field_spec{smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY, ++prio});
      rule->fields.push_back(field_spec{
          smi_id::
              F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_BPORT_ISOLATION_PACKET_DROP,
          1,
          1});
      rule->drop_reason = SWITCH_DROP_REASON_PORT_ISOLATION_DROP;
    }
  }
};

#define EGRESS_SYSTEM_ACL_ENTRY               \
  std::pair<_MatchKey, _ActionEntry>(         \
      _MatchKey(smi_id::T_EGRESS_SYSTEM_ACL), \
      _ActionEntry(smi_id::T_EGRESS_SYSTEM_ACL))
class default_egress_system_acl : public p4_object_match_action_list,
                                  default_egress_system_acl_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFAULT_EGRESS_SYSTEM_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFAULT_EGRESS_SYSTEM_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFAULT_EGRESS_SYSTEM_ACL_ATTR_PARENT_HANDLE;

 public:
  default_egress_system_acl(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent),
        default_egress_system_acl_object() {
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS)) {
      clear_attrs();
      return;
    }

    auto it = match_action_list.begin();
    for (auto rule : rules) {
      it = match_action_list.insert(it, EGRESS_SYSTEM_ACL_ENTRY);
      status |= add_match_action(it, rule);
    }
  }
};

class default_egress_system_acl2 : public p4_object_match_action_list,
                                   default_egress_system_acl_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFAULT_EGRESS_SYSTEM_ACL2;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFAULT_EGRESS_SYSTEM_ACL2_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFAULT_EGRESS_SYSTEM_ACL2_ATTR_PARENT_HANDLE;

  // in acl2.p4 the smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE
  // if optimized out by p4c so wee need to tack reason code locally
  std::map<MATCH_ACTION_ITERATOR, uint8_t> entry_dropreason_map = {};

 public:
  default_egress_system_acl2(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent),
        default_egress_system_acl_object() {
    if (!feature::is_feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS)) {
      clear_attrs();
      return;
    }

    uint16_t dev_port = 0;
    uint16_t dev_port_mask = 0x1FF;
    switch_enum_t port_type = {};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_EGRESS_SYSTEM_ACL, false));

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    std::vector<bf_rt_id_t> cntrs{smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS};

    auto it = match_action_list.begin();
    for (auto rule : rules) {
      it = match_action_list.insert(it, EGRESS_SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_EGRESS_SYSTEM_ACL_EG_INTR_MD_EGRESS_PORT,
          dev_port,
          dev_port_mask);
      status |= add_match_action(it, rule, cntrs);
      entry_dropreason_map[it] = rule.drop_reason;
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    p4_object_match_action_list::data_get();
    cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].counter_id =
        SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS;
    for (auto it = match_action_list.begin(); it != match_action_list.end();
         it++) {
      uint64_t pkts = 0, drop_reason = 0;
      uint16_t counter_id;
      it->second.get_arg(smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS,
                         smi_id::A_EGRESS_SYSTEM_ACL_DROP,
                         &pkts);
      cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].count += pkts;

      drop_reason = entry_dropreason_map[it];
      // check if drop_reason falls into another counter
      if (port_drop_reason_to_switch_counter(drop_reason, &counter_id) ==
          SWITCH_STATUS_SUCCESS) {
        cntrs[counter_id].counter_id = counter_id;
        cntrs[counter_id].count += pkts;
      }
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      entry.second.get_arg(smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE,
                           smi_id::A_EGRESS_SYSTEM_ACL_DROP,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }

    attr_w ctr_attr_list(
        SWITCH_DEFAULT_EGRESS_SYSTEM_ACL2_ATTR_MAU_STATS_CACHE);
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
        get_auto_oid(),
        SWITCH_DEFAULT_EGRESS_SYSTEM_ACL2_ATTR_MAU_STATS_CACHE,
        ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_drop_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_drop_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

void IdMap::updateIngressAclIds() {
  // clang-format off
  if (acl_using_acl2_profile()) {
    /*
     * Update the BFRT ids of Ingress ACLs for folded pipeline.
     */
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_MAC, smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_MAC, smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_DST_ADDR);
  }
  // clang-format on
}
void IdMap::updateEgressAclIds() {
  // clang-format off
  if (smi_id::T_EGRESS_PKT_VALIDATION) {
    /*
     * Update the BFRT ids of Egress ACLs for x7/Y8 profiles.
     */
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD10, smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD10);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2, smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD10, smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD10);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS, smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP, smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP, smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO, smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO);
  }
  // clang-format on
}

void IdMap::updateMacLabelAclIds() {
  // clang-format off
  if (acl_using_acl2_profile()) {
    /*
     * Update the BFRT ids of MAC Label ACLs for folded pipeline.
     */
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE, smi_id::T_INGRESS_ACL_QOS_MACADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX, smi_id::F_INGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC, smi_id::F_INGRESS_ACL_QOS_MACADDR_SMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC, smi_id::F_INGRESS_ACL_QOS_MACADDR_DMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL, smi_id::A_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL, smi_id::D_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY, smi_id::F_INGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY);

    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE, smi_id::T_INGRESS_ACL_MIRROR_MACADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX, smi_id::F_INGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC, smi_id::F_INGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC, smi_id::F_INGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL, smi_id::A_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL, smi_id::D_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY, smi_id::F_INGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY);

    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE, smi_id::T_INGRESS_ACL_PBR_MACADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX, smi_id::F_INGRESS_ACL_PBR_MACADDR_PORT_LAG_INDEX);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC, smi_id::F_INGRESS_ACL_PBR_MACADDR_SMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC, smi_id::F_INGRESS_ACL_PBR_MACADDR_DMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL, smi_id::A_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL, smi_id::D_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY, smi_id::F_INGRESS_ACL_PBR_MACADDR_MATCH_PRIORITY);

    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE, smi_id::T_EGRESS_ACL_QOS_MACADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX, smi_id::F_EGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC, smi_id::F_EGRESS_ACL_QOS_MACADDR_SMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC, smi_id::F_EGRESS_ACL_QOS_MACADDR_DMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL, smi_id::A_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL, smi_id::D_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY, smi_id::F_EGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY);

    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE, smi_id::T_EGRESS_ACL_MIRROR_MACADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX, smi_id::F_EGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC, smi_id::F_EGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC, smi_id::F_EGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL, smi_id::A_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL, smi_id::D_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL);
    set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY, smi_id::F_EGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY);
  }
  // clang-format on
}

IdMap::IdMap() {
  // If you are editing this code, STOP. Clear all distractions, now edit the
  // code.
  // clang-format off
  // Ingress MAC ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_MAC_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_MAC,                   smi_id::F_INGRESS_MAC_ACL_HDR_MAC_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_MAC,                   smi_id::F_INGRESS_MAC_ACL_HDR_MAC_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_INGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_MAC_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_MAC_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_MAC_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DENY,                  smi_id::A_INGRESS_MAC_ACL_DENY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_TRANSIT,               smi_id::A_INGRESS_MAC_ACL_TRANSIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_MAC_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA,      smi_id::P_INGRESS_MAC_ACL_PERMIT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_INGRESS_MAC_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID,            smi_id::P_INGRESS_MAC_ACL_PERMIT_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT,         smi_id::A_INGRESS_MAC_ACL_REDIRECT_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA,  smi_id::P_INGRESS_MAC_ACL_REDIRECT_PORT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX,   smi_id::P_INGRESS_MAC_ACL_REDIRECT_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA,  smi_id::P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_MAC_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_MAC_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_MAC_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_MAC_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_MAC_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_MAC_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_MAC_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP,                      smi_id::A_INGRESS_MAC_ACL_TRAP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID,              smi_id::P_INGRESS_MAC_ACL_TRAP_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX,          smi_id::P_INGRESS_MAC_ACL_TRAP_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY,                      smi_id::A_INGRESS_MAC_ACL_COPY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_TRAP_ID,              smi_id::P_INGRESS_MAC_ACL_COPY_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_METER_INDEX,          smi_id::P_INGRESS_MAC_ACL_COPY_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,               smi_id::A_INGRESS_MAC_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,               smi_id::P_INGRESS_MAC_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_MAC_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_MAC_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT,                smi_id::A_INGRESS_MAC_ACL_NO_NAT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT_DISABLE_NAT,    smi_id::P_INGRESS_MAC_ACL_NO_NAT_DISABLE_NAT);

  // pre ingress ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,       smi_id::T_PRE_INGRESS_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_SRC_MAC,          smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_DST_MAC,          smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,         smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_INGRESS_PORT,     smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_INGRESS_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,           smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,     smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,     smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_DST_IP,           smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,     smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,     smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,           smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,         smi_id::F_PRE_INGRESS_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,    smi_id::A_PRE_INGRESS_ACL_NO_ACTION);
  // There is no permit action for pre-ingress, using no_action instead
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,       smi_id::A_PRE_INGRESS_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,         smi_id::A_PRE_INGRESS_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_SET_VRF,          smi_id::A_PRE_INGRESS_ACL_SET_VRF);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_VRF,              smi_id::D_PRE_INGRESS_ACL_SET_VRF_VRF);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,      smi_id::D_PRE_INGRESS_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS, SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,       smi_id::D_PRE_INGRESS_ACL_COUNTER_SPEC_PKTS);

  // Ingress IP ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IP_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_INGRESS_PORT,              smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,              smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,              smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,              smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,              smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,                       smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_FRAG,                   smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_FRAG);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_INGRESS_IP_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_FIB_LABEL,                 smi_id::F_INGRESS_IP_ACL_LOCAL_MD_FIB_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_IP_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_IP_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_MAC,                   smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_MAC,                   smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IP_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_IP_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IP_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_IP_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_IP_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA,      smi_id::P_INGRESS_IP_ACL_PERMIT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_INGRESS_IP_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID,            smi_id::P_INGRESS_IP_ACL_PERMIT_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_IP_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA,
        smi_id::P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT,
        smi_id::A_INGRESS_IP_ACL_REDIRECT_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX,
        smi_id::P_INGRESS_IP_ACL_REDIRECT_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA,
        smi_id::P_INGRESS_IP_ACL_REDIRECT_PORT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_IP_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_IP_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_IP_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IP_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IP_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IP_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IP_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP,                      smi_id::A_INGRESS_IP_ACL_TRAP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID,              smi_id::P_INGRESS_IP_ACL_TRAP_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX,          smi_id::P_INGRESS_IP_ACL_TRAP_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY,                      smi_id::A_INGRESS_IP_ACL_COPY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_TRAP_ID,              smi_id::P_INGRESS_IP_ACL_COPY_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_METER_INDEX,          smi_id::P_INGRESS_IP_ACL_COPY_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IP_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IP_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETYPE_LABEL,               smi_id::F_INGRESS_IP_ACL_LOCAL_MD_ETYPE_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_LABEL,             smi_id::F_INGRESS_IP_ACL_LOCAL_MD_PBR_MAC_LABEL);

  // Ingress IPv4 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IPV4_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_INGRESS_PORT,              smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,                       smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_FRAG,                   smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_FRAG);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_FIB_LABEL,                 smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_FIB_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IN_PORTS_GROUP_LABEL,      smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IPV4_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IPV4_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_IPV4_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DENY,                  smi_id::A_INGRESS_IPV4_ACL_DENY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_TRANSIT,               smi_id::A_INGRESS_IPV4_ACL_TRANSIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_IPV4_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA,      smi_id::P_INGRESS_IPV4_ACL_PERMIT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_INGRESS_IPV4_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID,            smi_id::P_INGRESS_IPV4_ACL_PERMIT_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA,
        smi_id::P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT,
        smi_id::A_INGRESS_IPV4_ACL_REDIRECT_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX,
        smi_id::P_INGRESS_IPV4_ACL_REDIRECT_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA,
        smi_id::P_INGRESS_IPV4_ACL_REDIRECT_PORT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_IPV4_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_IPV4_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_IPV4_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IPV4_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IPV4_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IPV4_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IPV4_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,               smi_id::A_INGRESS_IPV4_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,               smi_id::P_INGRESS_IPV4_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP,                      smi_id::A_INGRESS_IPV4_ACL_TRAP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID,              smi_id::P_INGRESS_IPV4_ACL_TRAP_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX,          smi_id::P_INGRESS_IPV4_ACL_TRAP_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY,                      smi_id::A_INGRESS_IPV4_ACL_COPY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_TRAP_ID,              smi_id::P_INGRESS_IPV4_ACL_COPY_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_METER_INDEX,          smi_id::P_INGRESS_IPV4_ACL_COPY_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT,                smi_id::A_INGRESS_IPV4_ACL_NO_NAT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT_DISABLE_NAT,    smi_id::P_INGRESS_IPV4_ACL_NO_NAT_DISABLE_NAT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IPV4_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IPV4_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);

  // Ingress IPv6 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IPV6_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_INGRESS_PORT,              smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,              smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,              smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,              smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,              smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,                       smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_FRAG,                   smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_FRAG);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_FIB_LABEL,                 smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_FIB_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IN_PORTS_GROUP_LABEL,      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IPV6_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IPV6_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_IPV6_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DENY,                  smi_id::A_INGRESS_IPV6_ACL_DENY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_TRANSIT,               smi_id::A_INGRESS_IPV6_ACL_TRANSIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_IPV6_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA,      smi_id::P_INGRESS_IPV6_ACL_PERMIT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_INGRESS_IPV6_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID,            smi_id::P_INGRESS_IPV6_ACL_PERMIT_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA,
         smi_id::P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT,
         smi_id::A_INGRESS_IPV6_ACL_REDIRECT_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX,
         smi_id::P_INGRESS_IPV6_ACL_REDIRECT_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA,
         smi_id::P_INGRESS_IPV6_ACL_REDIRECT_PORT_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_IPV6_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_IPV6_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_IPV6_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IPV6_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IPV6_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IPV6_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IPV6_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,               smi_id::A_INGRESS_IPV6_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,               smi_id::P_INGRESS_IPV6_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP,                      smi_id::A_INGRESS_IPV6_ACL_TRAP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID,              smi_id::P_INGRESS_IPV6_ACL_TRAP_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX,          smi_id::P_INGRESS_IPV6_ACL_TRAP_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY,                      smi_id::A_INGRESS_IPV6_ACL_COPY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_TRAP_ID,              smi_id::P_INGRESS_IPV6_ACL_COPY_TRAP_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COPY_METER_INDEX,          smi_id::P_INGRESS_IPV6_ACL_COPY_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IPV6_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IPV6_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT,                smi_id::A_INGRESS_IPV6_ACL_NO_NAT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_NAT_DISABLE_NAT,    smi_id::P_INGRESS_IPV6_ACL_NO_NAT_DISABLE_NAT);

  // Ingress IP Mirror ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IP_MIRROR_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,                       smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_FRAG,                   smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_FRAG);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_INGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IP_MIRROR_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IP_MIRROR_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX,     smi_id::P_INGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_IP_MIRROR_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_IP_MIRROR_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IP_MIRROR_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IP_MIRROR_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IP_MIRROR_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IP_MIRROR_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,               smi_id::A_INGRESS_IP_MIRROR_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,               smi_id::P_INGRESS_IP_MIRROR_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_GROUP_INDEX,          smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_23_16);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IN_PORTS_GROUP_LABEL,      smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETYPE_LABEL,               smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_ETYPE_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_LABEL,             smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_CIR,                       smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PIR,                       smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_CBS,                       smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PBS,                       smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS);

  // Ingress IP QOS ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IP_QOS_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_INGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IP_QOS_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                    smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_RMAC_HIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IP_QOS_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX,     smi_id::P_INGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IP_QOS_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IP_QOS_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC_METER_INDEX,            smi_id::P_INGRESS_IP_QOS_ACL_SET_TC_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_QOS_PARAMS,            smi_id::A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_QOS_PARAMS_TC,             smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_QOS_PARAMS_COLOR,          smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_QOS_PARAMS_METER_INDEX,    smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IP_QOS_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR_METER_INDEX,         smi_id::P_INGRESS_IP_QOS_ACL_SET_COLOR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IP_QOS_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_METER,                 smi_id::A_INGRESS_IP_QOS_ACL_SET_METER);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_METER_INDEX,               smi_id::P_INGRESS_IP_QOS_ACL_SET_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,      smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETYPE_LABEL,               smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_ETYPE_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MACADDR_LABEL,             smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_CIR,                       smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PIR,                       smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_CBS,                       smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PBS,                       smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS);

  // Ingress IP DTEL ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_IP_DTEL_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,              smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,              smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,              smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,              smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_PROTO);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_MAC_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,                       smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_FRAG,                   smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_FRAG);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_IP_DTEL_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_INGRESS_IP_DTEL_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_INGRESS_IP_DTEL_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_INGRESS_IP_DTEL_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,      smi_id::A_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,   smi_id::P_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_IP_DTEL_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_IP_DTEL_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_IP_DTEL_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_TC,                    smi_id::A_INGRESS_IP_DTEL_ACL_SET_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TC,                        smi_id::P_INGRESS_IP_DTEL_ACL_SET_TC_TC);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SET_COLOR,                 smi_id::A_INGRESS_IP_DTEL_ACL_SET_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_COLOR,                     smi_id::P_INGRESS_IP_DTEL_ACL_SET_COLOR_COLOR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,               smi_id::A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,               smi_id::P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IFA_CLONE_SAMPLE,          smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IFA_CLONE_SESSION_ID,      smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IFA_CLONE_AND_DTEL_REPORT, smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IFA_SESSION_ID_WITH_TYPE,  smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID_WITH_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE_WITH_CLONE,    smi_id::P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE_WITH_CLONE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_TABLE,      smi_id::T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_INDEX,  smi_id::F_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REGISTER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_CURRENT, smi_id::D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_CURRENT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_RATE,   smi_id::D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_RATE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_DTEL, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_PKTS);

  // Ingress DSCP Mirror ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_INGRESS_TOS_MIRROR_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_INGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_INGRESS_TOS_MIRROR_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS);

  // Ingress Inner IPv4 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,     smi_id::T_INGRESS_INNER_DTEL_IPV4_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,         smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,         smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,       smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_PROTOCOL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,         smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DIFFSERV);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_SRC_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_DST_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_UDP_SRC_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_UDP_DST_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,            smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_TTL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,      smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL, smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_IG_MD_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VNI,            smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_LOCAL_MD_TUNNEL_VNI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,    smi_id::A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,    smi_id::P_INGRESS_INNER_DTEL_IPV4_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,    smi_id::D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,     smi_id::D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_PKTS);

  // Ingress Inner dtel IPv6 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,     smi_id::T_INGRESS_INNER_DTEL_IPV6_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,         smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DST_IP,         smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,       smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_NEXT_HDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,         smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_TRAFFIC_CLASS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_SRC_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_DST_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_UDP_SRC_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_UDP_DST_PORT,   smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TTL,            smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_HOP_LIMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,      smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL, smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_IG_MD_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_VNI,            smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_LOCAL_MD_TUNNEL_VNI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_DTEL_REPORT,    smi_id::A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_REPORT_TYPE,    smi_id::P_INGRESS_INNER_DTEL_IPV6_ACL_DTEL_REPORT_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,    smi_id::D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,     smi_id::D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_PKTS);

  // Egress MAC ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                 smi_id::T_EGRESS_MAC_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_MAC,                    smi_id::F_EGRESS_MAC_ACL_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_MAC,                    smi_id::F_EGRESS_MAC_ACL_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,             smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                   smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                         smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                   smi_id::F_EGRESS_MAC_ACL_ETHER_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_USER_METADATA,              smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                   smi_id::F_EGRESS_MAC_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                   smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_FLAGS_ROUTED);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,      smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,              smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                   smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,          smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,              smi_id::A_EGRESS_MAC_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                   smi_id::A_EGRESS_MAC_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                 smi_id::A_EGRESS_MAC_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,         smi_id::P_EGRESS_MAC_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,             smi_id::A_EGRESS_MAC_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,    smi_id::P_EGRESS_MAC_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,          smi_id::P_EGRESS_MAC_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,                smi_id::D_EGRESS_MAC_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_MAC, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                 smi_id::D_EGRESS_MAC_ACL_COUNTER_SPEC_PKTS);

  // Egress IPv4 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IPV4_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_PROTOCOL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_DIFFSERV);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_USER_METADATA,             smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_EGRESS_IPV4_ACL_ETHER_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IPV4_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                  smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_FLAGS_ROUTED);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,     smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IPV4_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_EGRESS_IPV4_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_EGRESS_IPV4_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_EGRESS_IPV4_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_EGRESS_IPV4_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_EGRESS_IPV4_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_EGRESS_IPV4_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_EGRESS_IPV4_ACL_MIRROR_IN);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_EGRESS_IPV4_ACL_MIRROR_IN_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_EGRESS_IPV4_ACL_MIRROR_IN_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IPV4_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IPV4_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL,     smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL);

  // Egress IPv6 ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IPV6_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,            smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,            smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,            smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,            smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_NEXT_HDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_TRAFFIC_CLASS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_USER_METADATA,             smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_USER_METADATA);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_EGRESS_IPV6_ACL_ETHER_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IPV6_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                  smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_FLAGS_ROUTED);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,     smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IPV6_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_EGRESS_IPV6_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_EGRESS_IPV6_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_EGRESS_IPV6_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_EGRESS_IPV6_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_EGRESS_IPV6_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_EGRESS_IPV6_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_EGRESS_IPV6_ACL_MIRROR_IN);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_EGRESS_IPV6_ACL_MIRROR_IN_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_EGRESS_IPV6_ACL_MIRROR_IN_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IPV6_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IPV6_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL,     smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL);

  //  start acl2.p4 egress
  // Egress IP Mirror ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IP_MIRROR_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTOCOL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,            smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2, smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD10, smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_SRC_ADDR_WORD10);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,            smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2, smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD10, smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_DST_ADDR_WORD10);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,     smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IP_MIRROR_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                  smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_ROUTED);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IP_MIRROR_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX,     smi_id::P_EGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_EGRESS_IP_MIRROR_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_EGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_EGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_LABEL,             smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_CIR,                       smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PIR,                       smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_CBS,                       smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PBS,                       smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS);

  // Egress IP Qos ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IP_QOS_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,            smi_id::F_EGRESS_IP_QOS_ACL_HDR_IP_SRC_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2, smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD10, smi_id::F_EGRESS_IP_QOS_ACL_HDR_IPV6_SRC_ADDR_WORD10);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,            smi_id::F_EGRESS_IP_QOS_ACL_HDR_IP_DST_ADDR_WORD3);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2, smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IPV6_WORD10, smi_id::F_EGRESS_IP_QOS_ACL_HDR_IPV6_DST_ADDR_WORD10);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTOCOL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,     smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IP_QOS_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ROUTED,                  smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_ROUTED);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,             smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_CFI,                  smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_DEI);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,         smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IP_QOS_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX,     smi_id::P_EGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SET_PCP,                   smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP,                       smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_METER_INDEX,           smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SET_IPV4_TOS,              smi_id::A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IPV4_TOS,                  smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IPV4_TOS_METER_INDEX,      smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SET_IPV6_TOS,              smi_id::A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IPV6_TOS,                  smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IPV6_TOS_METER_INDEX,      smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SET_PCP_IPV4_TOS,          smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV4_TOS,              smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV4_PCP,              smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV4_METER_INDEX,      smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SET_PCP_IPV6_TOS,          smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV6_TOS,              smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_TOS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV6_PCP,              smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_PCP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PCP_IPV6_METER_INDEX,      smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MACADDR_LABEL,             smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_CIR,                       smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PIR,                       smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_CBS,                       smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PBS,                       smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS);
  //  acl2.p4 egress complete

  // Egress IPv4 Mirror ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IPV4_MIRROR_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_PROTOCOL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DIFFSERV);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_EGRESS_IPV4_MIRROR_ACL_ETHER_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IPV4_MIRROR_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IPV4_MIRROR_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_EGRESS_IPV4_MIRROR_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_EGRESS_IPV4_MIRROR_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_EGRESS_IPV4_MIRROR_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL,      smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL);

  // Egress IPv6 Mirror ACL
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TABLE_TYPE,                smi_id::T_EGRESS_IPV6_MIRROR_ACL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_IP,                    smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_SRC_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_IP,                    smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_DST_ADDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_PROTO,                  smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_NEXT_HDR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_IP_TOS,                    smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ETH_TYPE,                  smi_id::F_EGRESS_IPV6_MIRROR_ACL_ETHER_TYPE);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_SRC_PORT,               smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_L4_DST_PORT,               smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_TCP_FLAGS,                 smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,            smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD_LABEL,                  smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_BD,                        smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,         smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PRIORITY,                  smi_id::F_EGRESS_IPV6_MIRROR_ACL_MATCH_PRIORITY);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,             smi_id::A_EGRESS_IPV6_MIRROR_ACL_NO_ACTION);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_DROP,                  smi_id::A_EGRESS_IPV6_MIRROR_ACL_DROP);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_PERMIT,                smi_id::A_EGRESS_IPV6_MIRROR_ACL_PERMIT);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,        smi_id::P_EGRESS_IPV6_MIRROR_ACL_PERMIT_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,            smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,   smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,         smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,             smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,    smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_METER_INDEX);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,          smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_SESSION_ID);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_BYTES,               smi_id::D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_BYTES);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_STATS_PKTS,                smi_id::D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_PKTS);
  set_id(SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR, SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS, ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL,      smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL);
  updateIngressAclIds();
  updateEgressAclIds();
  updateMacLabelAclIds();
  // clang-format on
}  // NOLINT(readability/fn_size)

LabelManager::LabelManager() {
  uint32_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX;
  if_qos_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_data_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_mac_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_racl_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_mirror_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_ipv4_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_ipv6_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_dtel_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_qos_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_data_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_racl_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_mirror_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_ipv4_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_ipv6_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  bd_dtel_labels_ = std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
  if_tos_mirror_labels_ =
      std::unique_ptr<idAllocator[]>(new idAllocator[dir]());
}

bool LabelManager::label_supp_for_racl_mac_qos_acl() {
  // NOTE: if updating this logic, go update ACLFieldComplexityTest PTF
  // If these slices are supported, then data ACLs use shared label space
  if ((feature::is_feature_set(SWITCH_FEATURE_INGRESS_MAC_ACL)) ||
      (feature::is_feature_set(SWITCH_FEATURE_INGRESS_IP_QOS_ACL)) ||
      (feature::is_feature_set(SWITCH_FEATURE_INGRESS_IPV4_RACL)) ||
      (feature::is_feature_set(SWITCH_FEATURE_INGRESS_IPV6_RACL)) ||
      (feature::is_feature_set(SWITCH_FEATURE_EGRESS_MAC_ACL))) {
    return true;
  }
  // all profiles without above slices use unique label space for v4/v6
  return false;
}

uint32_t LabelManager::label_position(uint64_t acl_type,
                                      uint64_t dir,
                                      uint64_t bp_type) {
  if (label_supp_for_racl_mac_qos_acl()) {
    if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
        bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
      if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_DATA_IPV4_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
            return SWITCH_ACL_MAC_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_DATA_IPV6_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
            return SWITCH_ACL_QOS_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
            return SWITCH_ACL_RACL_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
            return SWITCH_ACL_DTEL_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_POS;

          default:
            return 0;
        }
      } else {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_DATA_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
            return SWITCH_ACL_QOS_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
            return SWITCH_ACL_RACL_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_POS;

          default:
            return 0;
        }
      }
    } else {
      switch (acl_type) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          return SWITCH_ACL_DATA_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
          return SWITCH_ACL_QOS_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
          return SWITCH_ACL_RACL_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
          return SWITCH_ACL_DTEL_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
          return SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_POS;

        default:
          return 0;
      }
    }
  } else {
    if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
        bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
      if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_IPV4_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_IPV6_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
            return SWITCH_ACL_DTEL_IFA_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
            return SWITCH_ACL_TOS_MIRROR_ACL_IG_PORT_LABEL_POS;

          default:
            return 0;
        }
      } else {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_IPV4_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_IPV6_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_POS;

          case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
            return SWITCH_ACL_TOS_MIRROR_ACL_EG_PORT_LABEL_POS;

          default:
            return 0;
        }
      }
    } else {
      switch (acl_type) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          return SWITCH_ACL_IPV4_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          return SWITCH_ACL_IPV6_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
          return SWITCH_ACL_DTEL_IFA_ACL_BD_LABEL_POS;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
          return SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_POS;

        default:
          return 0;
      }
    }
  }
  return 0;
}

uint32_t LabelManager::label_width(uint64_t acl_type,
                                   uint64_t dir,
                                   uint64_t bp_type) {
  if (label_supp_for_racl_mac_qos_acl()) {
    if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
        bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
      if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_DATA_IPV4_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
            return SWITCH_ACL_MAC_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_DATA_IPV6_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
            return SWITCH_ACL_QOS_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
            return SWITCH_ACL_RACL_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
            return SWITCH_ACL_DTEL_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_WIDTH;

          default:
            return 0;
        }
      } else {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_DATA_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
            return SWITCH_ACL_QOS_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
            return SWITCH_ACL_RACL_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_WIDTH;

          default:
            return 0;
        }
      }
    } else {
      switch (acl_type) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          return SWITCH_ACL_DATA_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
          return SWITCH_ACL_QOS_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
          return SWITCH_ACL_RACL_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
          return SWITCH_ACL_DTEL_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
          return SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_WIDTH;

        default:
          return 0;
      }
    }
  } else {
    if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
        bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
      if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
            return SWITCH_ACL_IPV4_ACL_IG_PORT_LABEL_WIDTH +
                   SWITCH_ACL_IPV6_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_IPV4_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_IPV6_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
            return SWITCH_ACL_DTEL_IFA_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
            return SWITCH_ACL_TOS_MIRROR_ACL_IG_PORT_LABEL_WIDTH;

          default:
            return 0;
        }
      } else {
        switch (acl_type) {
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
            return SWITCH_ACL_IPV4_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
            return SWITCH_ACL_IPV6_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
          case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
            return SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_WIDTH;

          case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
            return SWITCH_ACL_TOS_MIRROR_ACL_EG_PORT_LABEL_WIDTH;

          default:
            return 0;
        }
      }
    } else {
      switch (acl_type) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
          return SWITCH_ACL_IPV4_ACL_BD_LABEL_WIDTH +
                 SWITCH_ACL_IPV6_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          return SWITCH_ACL_IPV4_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          return SWITCH_ACL_IPV6_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
          return SWITCH_ACL_DTEL_IFA_ACL_BD_LABEL_WIDTH;

        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
          return SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_WIDTH;

        default:
          return 0;
      }
    }
  }
  return 0;
}

void LabelManager::label_feature_value(switch_acl_label_t label_index,
                                       uint64_t acl_type,
                                       uint64_t dir,
                                       uint64_t bp_type,
                                       switch_acl_label_t &label_value,
                                       switch_acl_label_t &label_mask) {
  uint32_t label_pos = 0;
  uint32_t width = 0;

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Label index : {}",
             label_index);
  label_pos = label_position(acl_type, dir, bp_type);
  label_value = SWITCH_ACL_FEATURE_LABEL_VALUE(label_index, label_pos);
  width = label_width(acl_type, dir, bp_type);
  label_mask = SWITCH_ACL_FEATURE_LABEL_MASK(label_pos, width);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Label value/mask : {}/{}",
             label_value,
             label_mask);
}

switch_status_t LabelManager::label_allocate(uint64_t acl_type,
                                             uint64_t dir,
                                             uint64_t bp_type,
                                             switch_acl_label_t &label_value,
                                             switch_acl_label_t &label_mask) {
  uint32_t lbl_index = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (acl_type) {
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_ipv6_labels_[dir].allocate();
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          lbl_index = bd_data_labels_[dir].allocate();
        } else {
          lbl_index = bd_ipv6_labels_[dir].allocate();
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_mac_labels_[dir].allocate();
      } else {
        lbl_index = bd_data_labels_[dir].allocate();
      }
      break;

    // use the same label allocator for shared IP
    case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_ipv4_labels_[dir].allocate();
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          lbl_index = bd_data_labels_[dir].allocate();
        } else {
          lbl_index = bd_ipv4_labels_[dir].allocate();
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_qos_labels_[dir].allocate();
      } else {
        lbl_index = bd_qos_labels_[dir].allocate();
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_mirror_labels_[dir].allocate();
      } else {
        lbl_index = bd_mirror_labels_[dir].allocate();
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_dtel_labels_[dir].allocate();
      } else {
        lbl_index = bd_dtel_labels_[dir].allocate();
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_racl_labels_[dir].allocate();
      } else {
        lbl_index = bd_racl_labels_[dir].allocate();
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        lbl_index = if_tos_mirror_labels_[dir].allocate();
      } else {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_ACL_TABLE,
                   "DSCP Mirror ACL table is supported only for port/lag");
        return SWITCH_STATUS_SUCCESS;
      }
      break;
    case SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD:
      label_value = 0;
      label_mask = 0;
      // Label is not used for PFC WD
      return SWITCH_STATUS_SUCCESS;

    default:
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Acl type {} not supported",
                 acl_type);
      return SWITCH_STATUS_NOT_SUPPORTED;
  }
  label_feature_value(static_cast<switch_acl_label_t>(lbl_index),
                      acl_type,
                      dir,
                      bp_type,
                      label_value,
                      label_mask);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Label allocate for Acl table type {} bp type {} "
             "dir {} - Label idx {} value {} mask {}",
             acl_type,
             bp_type,
             dir,
             lbl_index,
             label_value,
             label_mask);

  return status;
}

switch_status_t LabelManager::label_reserve(uint64_t acl_type,
                                            uint64_t dir,
                                            uint64_t bp_type,
                                            switch_acl_label_t label_value) {
  uint32_t lbl_idx = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t label_pos = 0;

  label_pos = label_position(acl_type, dir, bp_type);
  lbl_idx = SWITCH_ACL_LABEL_VALUE(label_value, label_pos);

  switch (acl_type) {
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_ipv6_labels_[dir].reserve(lbl_idx);
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          bd_data_labels_[dir].reserve(lbl_idx);
        } else {
          bd_ipv6_labels_[dir].reserve(lbl_idx);
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_mac_labels_[dir].reserve(lbl_idx);
      } else {
        bd_data_labels_[dir].reserve(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_ipv4_labels_[dir].reserve(lbl_idx);
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          bd_data_labels_[dir].reserve(lbl_idx);
        } else {
          bd_ipv4_labels_[dir].reserve(lbl_idx);
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_qos_labels_[dir].reserve(lbl_idx);
      } else {
        bd_qos_labels_[dir].reserve(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_mirror_labels_[dir].reserve(lbl_idx);
      } else {
        bd_mirror_labels_[dir].reserve(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_dtel_labels_[dir].reserve(lbl_idx);
      } else {
        bd_dtel_labels_[dir].reserve(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_racl_labels_[dir].reserve(lbl_idx);
      } else {
        bd_racl_labels_[dir].reserve(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_tos_mirror_labels_[dir].reserve(lbl_idx);
      }
      break;

    default:
      break;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Label reserve for Acl table type {} bp type {} "
             "dir {} - Label idx {} value {}",
             acl_type,
             bp_type,
             dir,
             lbl_idx,
             label_value);

  return status;
}

switch_status_t LabelManager::label_release(uint64_t acl_type,
                                            uint64_t dir,
                                            uint64_t bp_type,
                                            switch_acl_label_t label_value) {
  uint32_t lbl_idx = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t label_pos = 0;

  label_pos = label_position(acl_type, dir, bp_type);
  lbl_idx = SWITCH_ACL_LABEL_VALUE(label_value, label_pos);

  switch (acl_type) {
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_ipv6_labels_[dir].release(lbl_idx);
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          bd_data_labels_[dir].release(lbl_idx);
        } else {
          bd_ipv6_labels_[dir].release(lbl_idx);
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_mac_labels_[dir].release(lbl_idx);
      } else {
        bd_data_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_ipv4_labels_[dir].release(lbl_idx);
      } else {
        if (label_supp_for_racl_mac_qos_acl()) {
          bd_data_labels_[dir].release(lbl_idx);
        } else {
          bd_ipv4_labels_[dir].release(lbl_idx);
        }
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_MAC_QOS:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_qos_labels_[dir].release(lbl_idx);
      } else {
        bd_qos_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_mirror_labels_[dir].release(lbl_idx);
      } else {
        bd_mirror_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_DTEL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_dtel_labels_[dir].release(lbl_idx);
      } else {
        bd_dtel_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_RACL:
    case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_RACL:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_racl_labels_[dir].release(lbl_idx);
      } else {
        bd_racl_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR:
      if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
          bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
        if_tos_mirror_labels_[dir].release(lbl_idx);
      }
      break;

    case SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD:
      // Label is not in use for PFC wd feature
      return SWITCH_STATUS_SUCCESS;

    default:
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "Acl type {} not supported",
                 acl_type);
      return SWITCH_STATUS_NOT_SUPPORTED;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Label release for Acl table type {} bp type {} "
             "dir {} - Label idx {} value {}",
             acl_type,
             bp_type,
             dir,
             lbl_idx,
             label_value);
  return status;
}

switch_status_t compute_acl_group_label(
    const switch_object_id_t acl_group_member,
    bool delete_mbr,
    bool adding_rule) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t acl_group = {0}, acl_table = {0};
  switch_acl_label_t this_acl_label = 0;
  switch_acl_label_t this_acl_label_mask = 0;
  switch_acl_label_t group_acl_label = 0;
  switch_acl_label_t group_acl_label_mask = 0;
  std::vector<switch_object_id_t> acl_group_members;

  // Get the acl table and group. Then fetch the acl_label from table and
  // save
  // it to group
  status |= switch_store::v_get(acl_group_member,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                acl_group);
  // Get the acl label for this member.
  status |= switch_store::v_get(acl_group_member,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                acl_table);

  std::vector<switch_object_id_t> acl_entry_handles;
  status |= switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, acl_entry_handles);
  if (acl_entry_handles.size() == 0 && !delete_mbr && !adding_rule)
    return status;

  status |= switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, this_acl_label);
  status |= switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK, this_acl_label_mask);

  // create-member: Add label from group_member's table.
  // delete-member: Mask member's label from the group's label.
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "Current ACL-group member table label {}, mask {} from table {}",
             this_acl_label,
             this_acl_label_mask,
             acl_table);

  // Get the group acl label
  status |= switch_store::v_get(
      acl_group, SWITCH_ACL_GROUP_ATTR_ACL_LABEL, group_acl_label);
  status |= switch_store::v_get(
      acl_group, SWITCH_ACL_GROUP_ATTR_ACL_LABEL_MASK, group_acl_label_mask);

  status |= switch_store::v_get(
      acl_group, SWITCH_ACL_GROUP_ATTR_ACL_GROUP_MEMBERS, acl_group_members);
  // this member is already off this list
  for (const auto mbr : acl_group_members) {
    if (mbr.data == acl_group_member.data) continue;

    switch_object_id_t temp_acl_table = {};
    switch_acl_label_t temp_acl_label = 0;
    switch_acl_label_t temp_acl_label_mask = 0;

    status |= switch_store::v_get(
        mbr, SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE, temp_acl_table);
    std::vector<switch_object_id_t> tmp_acl_entry_handles;
    status |= switch_store::v_get(temp_acl_table,
                                  SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES,
                                  tmp_acl_entry_handles);
    if (tmp_acl_entry_handles.size() == 0) continue;

    status |= switch_store::v_get(
        temp_acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, temp_acl_label);
    status |= switch_store::v_get(temp_acl_table,
                                  SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK,
                                  temp_acl_label_mask);
    group_acl_label |= temp_acl_label;
    group_acl_label_mask |= temp_acl_label_mask;
  }

  // now update the group acl label for this new group member
  if (delete_mbr == false) {
    group_acl_label |= this_acl_label;
    group_acl_label_mask |= this_acl_label_mask;
  } else {
    group_acl_label &= ~this_acl_label;
    group_acl_label_mask &= ~this_acl_label_mask;
  }

  switch_enum_t direction_enum = {0};
  switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_DIRECTION, direction_enum);
  uint64_t direction = direction_enum.enumdata;

  uint64_t bp_type = SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH;
  std::vector<switch_enum_t> bp_type_list;
  switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE, bp_type_list);
  for (auto bp_type_enum : bp_type_list) {
    // TODO(bfn): should be fixed for multiple bind points
    //       for now, just take the last one
    bp_type = bp_type_enum.enumdata;
  }

  status |= switch_store::v_set(
      acl_group, SWITCH_ACL_GROUP_ATTR_ACL_LABEL, group_acl_label);

  status |= switch_store::v_set(
      acl_group, SWITCH_ACL_GROUP_ATTR_ACL_LABEL_MASK, group_acl_label_mask);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "ACL group label {}, mask {}",
             group_acl_label,
             group_acl_label_mask);
  if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT ||
      bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_LAG) {
    switch_log(
        SWITCH_API_LEVEL_DETAIL, SWITCH_OBJECT_TYPE_NONE, "PORT/LAG ACL");
    switch_ig_port_lag_label_t group_acl_label_ipl = group_acl_label;
    switch_eg_port_lag_label_t group_acl_label_epl = group_acl_label;
    std::set<switch_object_id_t> port_handle_list;
    status |= switch_store::referencing_set_get(
        acl_group, SWITCH_OBJECT_TYPE_PORT, port_handle_list);
    for (auto port_oid : port_handle_list) {
      switch_log(SWITCH_API_LEVEL_DETAIL,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Set lag_label to port objects: {}, label {}",
                 port_oid,
                 group_acl_label_ipl);
      if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        status |= switch_store::v_set(port_oid,
                                      SWITCH_PORT_ATTR_INGRESS_PORT_LAG_LABEL,
                                      group_acl_label_ipl);
      } else {
        status |= switch_store::v_set(port_oid,
                                      SWITCH_PORT_ATTR_EGRESS_PORT_LAG_LABEL,
                                      group_acl_label_epl);
      }
    }

    std::set<switch_object_id_t> lag_handle_list;
    status |= switch_store::referencing_set_get(
        acl_group, SWITCH_OBJECT_TYPE_LAG, lag_handle_list);
    for (auto lag_oid : lag_handle_list) {
      switch_log(SWITCH_API_LEVEL_DETAIL,
                 SWITCH_OBJECT_TYPE_NONE,
                 "LAG objects: {}",
                 lag_oid);
      if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        status |= switch_store::v_set(lag_oid,
                                      SWITCH_LAG_ATTR_INGRESS_PORT_LAG_LABEL,
                                      group_acl_label_ipl);
      } else {
        status |= switch_store::v_set(lag_oid,
                                      SWITCH_LAG_ATTR_EGRESS_PORT_LAG_LABEL,
                                      group_acl_label_epl);
      }
    }
  }

  if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_VLAN ||
      bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_RIF) {
    switch_log(
        SWITCH_API_LEVEL_DETAIL, SWITCH_OBJECT_TYPE_NONE, "VLAN/RIF ACL");
    switch_bd_label_t group_acl_label_bd = group_acl_label;
    std::set<switch_object_id_t> vlan_handle_list;
    status |= switch_store::referencing_set_get(
        acl_group, SWITCH_OBJECT_TYPE_VLAN, vlan_handle_list);
    for (auto vlan_oid : vlan_handle_list) {
      switch_log(SWITCH_API_LEVEL_DETAIL,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Vlan objects: {}",
                 vlan_oid);
      if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        status |= switch_store::v_set(vlan_oid,
                                      SWITCH_VLAN_ATTR_INGRESS_VLAN_RIF_LABEL,
                                      group_acl_label_bd);
      } else {
        status |= switch_store::v_set(vlan_oid,
                                      SWITCH_VLAN_ATTR_EGRESS_VLAN_RIF_LABEL,
                                      group_acl_label_bd);
      }
    }

    std::set<switch_object_id_t> rif_handle_list;
    status |= switch_store::referencing_set_get(
        acl_group, SWITCH_OBJECT_TYPE_RIF, rif_handle_list);
    for (auto rif_oid : rif_handle_list) {
      switch_log(SWITCH_API_LEVEL_DETAIL,
                 SWITCH_OBJECT_TYPE_NONE,
                 "RIF objects: {}",
                 rif_oid);
      if (direction == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
        status |= switch_store::v_set(rif_oid,
                                      SWITCH_RIF_ATTR_INGRESS_VLAN_RIF_LABEL,
                                      group_acl_label_bd);
      } else {
        status |= switch_store::v_set(
            rif_oid, SWITCH_RIF_ATTR_EGRESS_VLAN_RIF_LABEL, group_acl_label_bd);
      }
    }
  }
  return status;
}

switch_status_t release_acl_table_label(const switch_object_id_t acl_table) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_acl_label_t label_value = 0, label_mask = 0;
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t bp_type = SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  switch_enum_t enum_type;
  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  status =
      switch_store::v_get(acl_table, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = get_smi_acl_type(acl_table, enum_type.enumdata, dir);

  std::vector<switch_enum_t> bp_type_list;
  switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE, bp_type_list);
  for (auto bp_type_enum : bp_type_list) {
    // TODO(bfn): should be fixed for multiple bind points
    //       for now, just take the last one
    bp_type = bp_type_enum.enumdata;
  }

  if (bp_type == SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, label_value);
  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK, label_mask);
  if (label_value != 0) {
    LabelManager::instance()->label_release(
        acl_type, dir, bp_type, label_value);
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_TABLE,
             "release label value {} label mask {} for acl table {}, acl type "
             "{} bp type {}",
             label_value,
             label_mask,
             acl_table,
             acl_type,
             bp_type);

  // reset back to zero
  label_value = 0;
  label_mask = 0;
  status = switch_store::v_set(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, label_value);
  status = switch_store::v_set(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK, label_mask);

  bool use_port_group_index = false;
  switch_store::v_get(acl_table,
                      SWITCH_ACL_TABLE_ATTR_USE_PORT_GROUP_INDEX,
                      use_port_group_index);
  if (use_port_group_index) {
    status |= release_acl_table_port_group(acl_table);
  }
  return status;
}

switch_status_t compute_acl_table_label(const switch_object_id_t acl_table) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_acl_label_t label_value = 0, label_mask = 0;
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t bp_type = SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  switch_enum_t enum_type;
  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  status =
      switch_store::v_get(acl_table, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = get_smi_acl_type(acl_table, enum_type.enumdata, dir);

  std::vector<switch_enum_t> bp_type_list;
  switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE, bp_type_list);
  for (auto bp_type_enum : bp_type_list) {
    // TODO(bfn): should be fixed for multiple bind points
    //       for now, just take the last one
    bp_type = bp_type_enum.enumdata;
  }

  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, label_value);
  status = switch_store::v_get(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK, label_mask);

  // bp type - port, lag, vlan, rif
  if (bp_type != SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH) {
    if (label_value == 0) {
      LabelManager::instance()->label_allocate(
          acl_type, dir, bp_type, label_value, label_mask);
    } else {
      LabelManager::instance()->label_reserve(
          acl_type, dir, bp_type, label_value);

      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_TABLE,
                 "Label exists for acl table {} acl type{} - reserve label {}, "
                 "label mask {}",
                 acl_table,
                 acl_type,
                 label_value,
                 label_mask);
      return status;
    }
  }

  // Currently only one table can exist in every acl type label space
  status |= switch_store::v_set(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL, label_value);
  status |= switch_store::v_set(
      acl_table, SWITCH_ACL_TABLE_ATTR_ACL_LABEL_MASK, label_mask);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_TABLE,
             "compute label value {} label mask {} for acl table {}, acl type "
             "{} bp type {}",
             label_value,
             label_mask,
             acl_table,
             acl_type,
             bp_type);

  return status;
}

switch_status_t release_inout_ports_label(
    const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  std::set<switch_object_id_t> port_handles;

  uint64_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  // IN_PORTS HANDLE LIST to update
  std::vector<switch_object_id_t> in_port_handles_list;
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_IN_PORTS, in_port_handles_list);

  // OUT_PORTS HANDLE LIST to update
  std::vector<switch_object_id_t> out_port_handles_list;
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_OUT_PORTS, out_port_handles_list);

  if (in_port_handles_list.empty() && out_port_handles_list.empty()) {
    return SWITCH_STATUS_SUCCESS;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  // ACL direction
  switch_enum_t dir_enum;
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, dir_enum);
  dir = dir_enum.enumdata;

  // ACL type
  switch_enum_t enum_type;
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = get_smi_acl_type(acl_entry_handle, enum_type.enumdata, dir);

  // in out group index
  uint8_t inout_group_index = 0;
  status |= switch_store::v_get(acl_entry_handle,
                                SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL,
                                inout_group_index);

  // releasing the label
  PortGroupLabel::instance()->label_release(acl_type, dir, inout_group_index);

  // Update and Get ref_count
  uint32_t ref_count = 0;
  ref_count =
      PortGroupLabel::instance()->label_count(acl_type, dir, inout_group_index);

  // If the in-ports / out-ports still has positive refer count return
  if (ref_count |= 0) {
    return status;
  }

  // calculate the port_inout_group_label
  uint32_t port_label = 0;
  PortGroupLabel::instance()->get_label_value(
      inout_group_index, acl_type, dir, port_label);

  for (auto port : in_port_handles_list) {
    uint32_t current_in_ports_label = 0;
    status |= switch_store::v_get(
        port, SWITCH_PORT_ATTR_IN_PORTS_GROUP_LABEL, current_in_ports_label);
    uint32_t new_in_ports_label = current_in_ports_label & (~port_label);
    status |= switch_store::v_set(
        port, SWITCH_PORT_ATTR_IN_PORTS_GROUP_LABEL, new_in_ports_label);
    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_ENTRY,
        "{}:{} Current IN PORTS label value {} to port {}, new_label: {}",
        __func__,
        __LINE__,
        current_in_ports_label,
        port,
        new_in_ports_label);
  }

  for (auto port : out_port_handles_list) {
    uint16_t current_out_ports_label = 0;
    status |= switch_store::v_get(
        port, SWITCH_PORT_ATTR_OUT_PORTS_GROUP_LABEL, current_out_ports_label);
    uint16_t new_out_ports_label = current_out_ports_label & (~port_label);
    status |= switch_store::v_set(
        port, SWITCH_PORT_ATTR_OUT_PORTS_GROUP_LABEL, new_out_ports_label);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} Current OUT PORTS label value {} to port {}, value: {}",
               __func__,
               __LINE__,
               current_out_ports_label,
               port,
               new_out_ports_label);
  }

  return status;
}

switch_status_t compute_inout_ports_label(std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  std::set<switch_object_id_t> port_handles;

  uint64_t s3_acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  // Retrieving IN_PORTS
  std::vector<switch_object_id_t> in_port_handles_list;
  auto it_in_ports = attrs.find(SWITCH_ACL_ENTRY_ATTR_IN_PORTS);
  if (it_in_ports != attrs.end()) {
    status |= it_in_ports->v_get(in_port_handles_list);
  }

  // Retrieving OUT_PORTS
  std::vector<switch_object_id_t> out_port_handles_list;
  auto it_out_ports = attrs.find(SWITCH_ACL_ENTRY_ATTR_OUT_PORTS);
  if (it_out_ports != attrs.end()) {
    status |= it_out_ports->v_get(out_port_handles_list);
  }

  // check if we are setting in_ports / out_ports
  if (in_port_handles_list.empty() && out_port_handles_list.empty()) {
    return SWITCH_STATUS_SUCCESS;
  }

  // Retrieving acl_table id
  auto it_acl_table = attrs.find(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE);
  CHECK_RET(it_acl_table == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= it_acl_table->v_get(table_handle);

  // Retrieving ACL type
  switch_enum_t enum_type;
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  s3_acl_type = enum_type.enumdata;

  // Retrieving direction
  switch_enum_t dir_enum;
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, dir_enum);
  dir = dir_enum.enumdata;

  // calculate the egress acl_table
  switch_ip_address_t dest_ip_addr = {};
  auto it_dest_ip = attrs.find(SWITCH_ACL_ENTRY_ATTR_DST_IP);
  if (it_dest_ip != attrs.end()) it_dest_ip->v_get(dest_ip_addr);

  switch_ip_address_t src_ip_addr = {};
  auto it_src_ip = attrs.find(SWITCH_ACL_ENTRY_ATTR_SRC_IP);
  if (it_src_ip != attrs.end()) it_src_ip->v_get(src_ip_addr);

  // If received ACL entry is of IP Mirror Type on Egress direction
  // use ip mirror acl tables if exists in a profile, else use ip acl tables
  // Else received ACL type
  if (s3_acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR &&
      dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_MIRROR_ACL)) {
      if (dest_ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6 ||
          src_ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR;
      } else {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR;
      }
    } else {
      if (dest_ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6 ||
          src_ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV6;
      } else {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV4;
      }
    }
  } else {
    acl_type = s3_acl_type;
  }

  std::set<switch_object_id_t> in_ports;
  for (auto port : in_port_handles_list) {
    in_ports.insert(port);
  }

  std::set<switch_object_id_t> out_ports;
  for (auto port : out_port_handles_list) {
    out_ports.insert(port);
  }

  uint8_t inout_group_index = 0;

  if (in_port_handles_list.size() > 0) {
    status = PortGroupLabel::instance()->label_allocate(
        acl_type, dir, in_ports, inout_group_index);

    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} in port label {}",
               __func__,
               __LINE__,
               inout_group_index);
  }

  if (out_port_handles_list.size() > 0) {
    status = PortGroupLabel::instance()->label_allocate(
        acl_type, dir, out_ports, inout_group_index);

    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} out ports label {}",
               __func__,
               __LINE__,
               inout_group_index);
  }

  const auto in_port_set_ret = attrs.insert(attr_w(
      SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL, inout_group_index));
  CHECK_RET(in_port_set_ret.second == false, SWITCH_STATUS_FAILURE);

  const auto out_port_set_ret = attrs.insert(attr_w(
      SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL_MASK, inout_group_index));
  CHECK_RET(out_port_set_ret.second == false, SWITCH_STATUS_FAILURE);

  return status;
}

switch_status_t update_inout_ports_label(
    const switch_object_id_t acl_entry_handle) {
  switch_object_id_t table_handle = {0};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  uint64_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  // Extracting in_ports list
  std::vector<switch_object_id_t> in_port_handles_list;
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_IN_PORTS, in_port_handles_list);

  // Extracting out_ports list
  std::vector<switch_object_id_t> out_port_handles_list;
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_OUT_PORTS, out_port_handles_list);

  if (in_port_handles_list.empty() && out_port_handles_list.empty()) {
    return SWITCH_STATUS_SUCCESS;
  }

  // ACL table handle
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  // ACL direction
  switch_enum_t dir_enum;
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, dir_enum);
  dir = dir_enum.enumdata;

  // ACL type
  switch_enum_t enum_type;
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = get_smi_acl_type(acl_entry_handle, enum_type.enumdata, dir);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{} --> acl_type: {}, s3_acl_type: {}",
             __func__,
             __LINE__,
             acl_type,
             enum_type.enumdata);

  uint32_t port_label = 0;
  uint8_t group_index = 0;
  switch_store::v_get(acl_entry_handle,
                      SWITCH_ACL_ENTRY_ATTR_IN_OUT_PORTS_GROUP_LABEL,
                      group_index);

  // Calculare the port_group_index
  PortGroupLabel::instance()->get_label_value(
      group_index, acl_type, dir, port_label);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}: {} Inside update_inout_ports_label acl_entry_handle : {}"
             ", group_index: {}, port_label: {}",
             __func__,
             __LINE__,
             acl_entry_handle.data,
             group_index,
             port_label);

  for (auto port : in_port_handles_list) {
    uint32_t current_in_ports_label = 0;
    status |= switch_store::v_get(
        port, SWITCH_PORT_ATTR_IN_PORTS_GROUP_LABEL, current_in_ports_label);

    uint32_t new_in_ports_label = current_in_ports_label | port_label;

    status |= switch_store::v_set(
        port, SWITCH_PORT_ATTR_IN_PORTS_GROUP_LABEL, new_in_ports_label);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} Current IN PORTS label {}, new label: {} to port {}",
               __func__,
               __LINE__,
               current_in_ports_label,
               new_in_ports_label,
               port);
  }

  for (auto port : out_port_handles_list) {
    uint16_t current_out_ports_label = 0;
    status |= switch_store::v_get(
        port, SWITCH_PORT_ATTR_OUT_PORTS_GROUP_LABEL, current_out_ports_label);

    uint16_t new_out_ports_label = current_out_ports_label | port_label;

    status |= switch_store::v_set(
        port, SWITCH_PORT_ATTR_OUT_PORTS_GROUP_LABEL, new_out_ports_label);

    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} Current OUT PORTS label {}, new label: {} to port {}",
               __func__,
               __LINE__,
               current_out_ports_label,
               new_out_ports_label,
               port);
  }

  return status;
}

uint32_t PortGroupLabel::label_position(uint64_t acl_type, uint64_t dir) {
  if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    switch (acl_type) {
      case SWITCH_ACL_TABLE_ATTR_TYPE_IP:
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
        return SWITCH_ACL_IPV4_ACL_IN_PORTS_LABEL_POS;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
        return SWITCH_ACL_IPV6_ACL_IN_PORTS_LABEL_POS;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        return SWITCH_ACL_IP_MIRROR_ACL_IN_PORTS_LABEL_POS;

      default:
        return 0;
    }
  } else {
    switch (acl_type) {
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        return SWITCH_ACL_IPV4_ACL_OUT_PORTS_LABEL_POS;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
        return SWITCH_ACL_IPV6_ACL_OUT_PORTS_LABEL_POS;

      default:
        return 0;
    }
  }
  return 0;
}

uint32_t PortGroupLabel::label_width(uint64_t acl_type, uint64_t dir) {
  if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    switch (acl_type) {
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
        return SWITCH_ACL_IPV4_ACL_IN_PORTS_LABEL_WIDTH;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
        return SWITCH_ACL_IPV6_ACL_IN_PORTS_LABEL_WIDTH;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
        return SWITCH_ACL_IP_MIRROR_ACL_IN_PORTS_LABEL_WIDTH;

      default:
        return 0;
    }
  } else {
    switch (acl_type) {
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR:
        return SWITCH_ACL_IPV4_ACL_OUT_PORTS_LABEL_WIDTH;

      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
      case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR:
        return SWITCH_ACL_IPV6_ACL_OUT_PORTS_LABEL_WIDTH;

      default:
        return 0;
    }
  }
  return 0;
}

void PortGroupLabel::get_label_value(uint8_t group_index,
                                     uint64_t acl_type,
                                     uint64_t dir,
                                     uint32_t &label_value) {
  uint32_t interim_label = group_index;
  uint32_t label_pos = label_position(acl_type, dir);
  label_value = SWITCH_ACL_FEATURE_LABEL_VALUE(interim_label, label_pos);
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{} Label value : {}",
             __func__,
             __LINE__,
             label_value);
}

// This is getting usef for Inports and Outports Label
PortGroupLabel::PortGroupLabel() {
  uint32_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX;
  for (uint32_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
       acl_type < SWITCH_ACL_TABLE_ATTR_TYPE_MAX;
       acl_type++) {
    for (dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS;
         dir < SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX;
         dir++) {
      acl_port_group_[acl_type][dir] =
          new idAllocator(SWITCH_ACL_PORT_GROUP_MAX);
    }
  }
}

switch_status_t PortGroupLabel::label_allocate(
    const uint64_t acl_type,
    const uint64_t dir,
    const std::set<switch_object_id_t> &port_handles_list,
    uint8_t &group_index) {
  uint8_t group_index_pos = 0;
  // allocate a new port_group_index, if not present
  if (acl_port_group_map_[acl_type][dir].find(port_handles_list) ==
      acl_port_group_map_[acl_type][dir].end()) {
    // get the group_index_pos
    group_index_pos =
        static_cast<uint8_t>(acl_port_group_[acl_type][dir]->allocate());

    if (group_index_pos > SWITCH_ACL_INOUT_PORTS_GROUP_MAX) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{}:{} in/out port group index exceeds maximum count of:  8 "
                 "for acl_type: {} and direction: {}",
                 __func__,
                 __LINE__,
                 acl_type,
                 dir);
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    }

    // calculating the group_index
    group_index = 0x0001;
    if (group_index_pos > 1) {
      group_index = 0x0001 << (group_index_pos - 1);
    }

    acl_port_group_map_[acl_type][dir][port_handles_list] = group_index;
    acl_port_group_count_[acl_type][dir][group_index] = 1;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} ACL {}: port_group index {}, value: {}",
               __func__,
               __LINE__,
               acl_type,
               group_index,
               acl_port_group_count_[acl_type][dir][group_index]);
  } else {
    group_index = acl_port_group_map_[acl_type][dir][port_handles_list];
    acl_port_group_count_[acl_type][dir][group_index] += 1;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} ACL {}: existing port_group index {}, value: {}",
               __func__,
               __LINE__,
               acl_type,
               group_index,
               acl_port_group_count_[acl_type][dir][group_index]);
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t PortGroupLabel::label_release(const uint64_t acl_type,
                                              const uint64_t dir,
                                              const uint8_t group_index) {
  if (acl_port_group_count_[acl_type][dir][group_index] == 0) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{} in out port group index is not present",
               __func__,
               __LINE__);
    return SWITCH_STATUS_SUCCESS;
  }

  acl_port_group_count_[acl_type][dir][group_index] -= 1;
  if (acl_port_group_count_[acl_type][dir][group_index] == 0) {
    // calculate the group_index_pos
    uint8_t group_index_pos = 1;
    uint8_t bit_flag = 1;
    for (; group_index_pos <= SWITCH_ACL_INOUT_PORTS_GROUP_MAX;
         ++group_index_pos) {
      if ((group_index & bit_flag) == group_index) {
        break;
      }
      bit_flag = bit_flag << 1;
    }
    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_ACL_ENTRY,
        "{}:{} ACL {}: Dir: {}, port_group_index: {}, port_group_index_pos: {}",
        __func__,
        __LINE__,
        acl_type,
        dir,
        group_index,
        group_index_pos);
    acl_port_group_[acl_type][dir]->release(group_index_pos);

    auto iter = acl_port_group_map_[acl_type][dir].begin();
    while (iter != acl_port_group_map_[acl_type][dir].end()) {
      if (iter->second == group_index) {
        acl_port_group_map_[acl_type][dir].erase(iter);
        break;
      }
      iter++;
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

uint32_t PortGroupLabel::label_count_update(const uint64_t acl_type,
                                            const uint64_t dir,
                                            const uint8_t group_index,
                                            bool is_reduce) {
  if (acl_port_group_count_[acl_type][dir][group_index] == 0) {
    return acl_port_group_count_[acl_type][dir][group_index];
  }

  if (is_reduce) {
    acl_port_group_count_[acl_type][dir][group_index] -= 1;
  } else {
    acl_port_group_count_[acl_type][dir][group_index] += 1;
  }
  return acl_port_group_count_[acl_type][dir][group_index];
}

uint32_t PortGroupLabel::label_count(const uint64_t acl_type,
                                     const uint64_t dir,
                                     const uint8_t group_index) {
  return acl_port_group_count_[acl_type][dir][group_index];
}

PortGroupManager::PortGroupManager() {
  uint32_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX;
  for (uint32_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
       acl_type < SWITCH_ACL_TABLE_ATTR_TYPE_MAX;
       acl_type++) {
    for (dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS;
         dir < SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX;
         dir++) {
      acl_port_group_[acl_type][dir] =
          new idAllocator(SWITCH_ACL_PORT_GROUP_MAX);
    }
  }
}

switch_status_t PortGroupManager::port_group_allocate(
    const uint64_t acl_type,
    const uint64_t dir,
    const std::set<switch_object_id_t> port_handles_list,
    uint8_t &group_value) {
  uint32_t group_index = 0;

  if (acl_port_group_map_[acl_type][dir].find(port_handles_list) ==
      acl_port_group_map_[acl_type][dir].end()) {
    // allocate a new port_group_index
    group_index = acl_port_group_[acl_type][dir]->allocate();
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_NONE,
               "ACL {}: create new port_group index {}",
               acl_type,
               group_index);
    acl_port_group_map_[acl_type][dir][port_handles_list] = group_index;
    acl_port_group_count_[acl_type][dir][group_index] = 0;
  } else {
    group_index = acl_port_group_map_[acl_type][dir][port_handles_list];
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_NONE,
               "ACL {}: port_group index exists for the map {}",
               acl_type,
               group_index);
  }
  acl_port_group_count_[acl_type][dir][group_index]++;
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "ACL {}: Allocate port_group index ref_count {}",
             acl_type,
             acl_port_group_count_[acl_type][dir][group_index]);
  group_value = static_cast<uint8_t>(group_index);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t PortGroupManager::port_group_count(const uint64_t acl_type,
                                                   const uint64_t dir,
                                                   const uint8_t group_index,
                                                   uint32_t &count) {
  count = acl_port_group_count_[acl_type][dir][group_index];
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t PortGroupManager::port_group_release(
    const uint64_t acl_type, const uint64_t dir, const uint8_t group_index) {
  acl_port_group_count_[acl_type][dir][group_index]--;
  if (acl_port_group_count_[acl_type][dir][group_index] == 0) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_NONE,
               "ACL {}: Release: port_group index {} release",
               acl_type,
               group_index);
    acl_port_group_[acl_type][dir]->release(group_index);
    auto iter = acl_port_group_map_[acl_type][dir].begin();
    while (iter != acl_port_group_map_[acl_type][dir].end()) {
      if (iter->second == group_index) {
        acl_port_group_map_[acl_type][dir].erase(iter);
        break;
      }
      ++iter;
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_hostif_trap_group_update(const switch_object_id_t handle,
                                               const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle{};
  switch_object_id_t dflt_trap_grp{};

  status = switch_store::v_get(
      handle, SWITCH_HOSTIF_TRAP_GROUP_ATTR_DEVICE, device_handle);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  status = switch_store::v_get(device_handle,
                               SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP,
                               dflt_trap_grp);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (dflt_trap_grp == handle) {
    std::unique_ptr<object> mobject =
        std::unique_ptr<default_ingress_system_acl>(
            new default_ingress_system_acl(device_handle, status));

    if (mobject) {
      mobject->create_update();
    }
  }

  return status;
}

switch_status_t before_lag_update(const switch_object_id_t object_id,
                                  const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id = attr.id_get();
  switch (attr_id) {
    case SWITCH_LAG_ATTR_INGRESS_ACL_HANDLE:
    case SWITCH_LAG_ATTR_EGRESS_ACL_HANDLE:
      return update_bind_point_flag(object_id, attr);
    default:
      break;
  }
  return status;
}

switch_status_t before_vlan_update(const switch_object_id_t object_id,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id = attr.id_get();
  switch (attr_id) {
    case SWITCH_VLAN_ATTR_INGRESS_ACL_HANDLE:
    case SWITCH_VLAN_ATTR_EGRESS_ACL_HANDLE:
      return update_bind_point_flag(object_id, attr);
    default:
      break;
  }
  return status;
}

switch_status_t before_rif_update(const switch_object_id_t object_id,
                                  const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id = attr.id_get();
  switch (attr_id) {
    case SWITCH_RIF_ATTR_INGRESS_ACL_HANDLE:
    case SWITCH_RIF_ATTR_EGRESS_ACL_HANDLE:
      return update_bind_point_flag(object_id, attr);
    default:
      break;
  }
  return status;
}

switch_status_t before_acl_group_update(const switch_object_id_t object_id,
                                        const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id = attr.id_get();

  if (attr_id == SWITCH_ACL_GROUP_ATTR_BIND_POINT_ATTACH) {
    bool _acl_group_attach_flag = {}, acl_group_attach_flag = {};

    attr.v_get(_acl_group_attach_flag);
    status |= switch_store::v_get(object_id,
                                  SWITCH_ACL_GROUP_ATTR_BIND_POINT_ATTACH,
                                  acl_group_attach_flag);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_TABLE,
               "{}.{} Flag: {}.{}",
               __func__,
               __LINE__,
               acl_group_attach_flag,
               _acl_group_attach_flag);
    if (acl_group_attach_flag == false && _acl_group_attach_flag == true) {
      // get all the tables of the group and mark them to true
      std::vector<switch_object_id_t> acl_group_members;
      status |= switch_store::v_get(object_id,
                                    SWITCH_ACL_GROUP_ATTR_ACL_GROUP_MEMBERS,
                                    acl_group_members);
      for (const auto mbr : acl_group_members) {
        switch_object_id_t acl_table_handle = {0};
        status |=
            switch_store::v_get(mbr,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                acl_table_handle);
        status |= set_bind_point_attach_flag(acl_table_handle, true);
      }
    }
  }
  return status;
}

switch_status_t before_acl_table_update2(const switch_object_id_t object_id,
                                         const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id = attr.id_get();

  if (attr_id == SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH) {
    bool _acl_table_attach_flag = {}, acl_table_attach_flag = {};

    attr.v_get(_acl_table_attach_flag);
    status |= switch_store::v_get(object_id,
                                  SWITCH_ACL_TABLE_ATTR_BIND_POINT_ATTACH,
                                  acl_table_attach_flag);
    if (acl_table_attach_flag == true && _acl_table_attach_flag == false) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_ACL_TABLE,
                 "{}.{} Delete ACL entries from HW",
                 __func__,
                 __LINE__);
      switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
      switch_enum_t acl_dir = {SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};

      std::vector<switch_object_id_t> acl_entry_handles;
      status = switch_store::v_get(object_id,
                                   SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES,
                                   acl_entry_handles);
      for (const auto &acl_entry_handle : acl_entry_handles) {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_ACL_TABLE,
                   "{}.{} Getting into deletion code...",
                   __func__,
                   __LINE__);
        std::unique_ptr<object> mobject;
        status = get_acl_table_type_dir(acl_entry_handle, acl_type, acl_dir);
        mobject = std::unique_ptr<acl_hw_entry>(new acl_hw_entry(
            acl_entry_handle, status, acl_type.enumdata, acl_dir.enumdata));
        if (mobject != NULL) {
          mobject->del();
        }
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

bool is_etype_label_supported(uint64_t acl_type, uint64_t acl_dir) {
  auto table_types_supported = {SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS,
                                SWITCH_ACL_TABLE_ATTR_TYPE_IP,  // PBR
                                SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR};

  if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) return false;

  for (auto tt : table_types_supported) {
    if (acl_type == tt) {
      return true;
    }
  }
  return false;
}

switch_status_t etype_label_allocate_or_reserve(std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle;
  uint64_t acl_type;
  uint64_t acl_dir;
  uint16_t eth_type = 0;
  uint16_t eth_type_mask = 0;
  uint8_t etype_label = 0;  // 4-bit label

  const auto hdl_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE));
  CHECK_RET(hdl_it == attrs.end(), SWITCH_STATUS_FAILURE);
  hdl_it->v_get(table_handle);

  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_etype_label_supported(acl_type, acl_dir)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not generating label for table type {} dir {}",
               __func__,
               __LINE__,
               acl_type,
               acl_dir);
    return SWITCH_STATUS_SUCCESS;
  }

  const auto attr_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE));
  if (attr_it == attrs.end()) {
    // Eth type not present for this ACL entry, label not required
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type not present",
               __func__,
               __LINE__);
    return SWITCH_STATUS_SUCCESS;
  }

  attr_it->v_get(eth_type);
  if (eth_type == 0) {
    // By default, eth_type is set to 0, this is not an error
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type not set",
               __func__,
               __LINE__);
    return SWITCH_STATUS_SUCCESS;
  }

  const auto mask_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK));
  if (mask_attr_it == attrs.end()) {
    // Eth type present but not the mask
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type mask not present",
               __func__,
               __LINE__);
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  mask_attr_it->v_get(eth_type_mask);
  if (eth_type_mask == 0) {
    // Eth type is set, but not the mask
    // Etype label is supported for mask 0xFFFF only
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type invalid mask {}",
               __func__,
               __LINE__,
               eth_type_mask);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  const auto index_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_ETYPE_LABEL));
  if (index_attr_it == attrs.end()) {
    status = etype_label_allocate(eth_type, etype_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{};{}: Failed to allocate label for eth_type {}",
                 __func__,
                 __LINE__,
                 eth_type);
      return status;
    }

    attrs.insert(attr_w(SWITCH_ACL_ENTRY_ATTR_ETYPE_LABEL, etype_label));
  } else {
    status = (*index_attr_it).v_get(etype_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{};{}: Failed to get allocated etype_label {}",
                 __func__,
                 __LINE__,
                 etype_label);
      return status;
    }

    status = etype_label_reserve(eth_type, etype_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{};{}: Failed to reserve label {} for eth_type {}",
                 __func__,
                 __LINE__,
                 etype_label,
                 eth_type);
      return status;
    }
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Allocated etype_label {} for eth_type {}",
             __func__,
             __LINE__,
             etype_label,
             eth_type);
  return status;
}

switch_status_t etype_acl_create(const switch_object_id_t object_id,
                                 const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t eth_type = 0;
  uint8_t etype_label = 0;
  uint16_t refcount;

  switch_object_id_t table_handle;
  uint64_t acl_type;
  uint64_t acl_dir;
  const auto hdl_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE));
  CHECK_RET(hdl_it == attrs.end(), SWITCH_STATUS_FAILURE);
  hdl_it->v_get(table_handle);

  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_etype_label_supported(acl_type, acl_dir)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not creating etype entry for table type {}",
               __func__,
               __LINE__,
               acl_type);
    return SWITCH_STATUS_SUCCESS;
  }

  status =
      switch_store::v_get(object_id, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, eth_type);
  if (eth_type == 0) {
    // By default, eth_type is set to 0
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type not set",
               __func__,
               __LINE__);
    return SWITCH_STATUS_SUCCESS;
  }

  status = switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_ETYPE_LABEL, etype_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Failed to retrieve etype_label from object {}",
               __func__,
               __LINE__,
               object_id);
    return status;
  }

  status = etype_label_get_refcount(eth_type, refcount);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to get refcount eth_type {} status {}",
               __func__,
               __LINE__,
               eth_type,
               status);
    return SWITCH_STATUS_FAILURE;
  }
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: eth_type {} refcount {}",
             __func__,
             __LINE__,
             eth_type,
             refcount);

  // Create etype_acl entries only when creating first acl entry
  if (refcount == 1) {
    status |= etype_acl1_create(eth_type, etype_label);
    status |= etype_acl2_create(eth_type, etype_label);
  }

  return status;
}

switch_status_t etype_acl_cleanup(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t eth_type = 0;
  uint8_t etype_label = 0;
  uint16_t refcount;

  switch_object_id_t table_handle;
  uint64_t acl_type;
  uint64_t acl_dir;
  status |= switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_etype_label_supported(acl_type, acl_dir)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not releasing etype entry for table type {}",
               __func__,
               __LINE__,
               acl_type);
    return SWITCH_STATUS_SUCCESS;
  }

  status =
      switch_store::v_get(object_id, SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, eth_type);
  if (eth_type == 0) {
    // By default, eth_type is set to 0
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Eth type not set",
               __func__,
               __LINE__);
    return SWITCH_STATUS_SUCCESS;
  }

  status = etype_label_get_refcount(eth_type, refcount);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to get refcount eth_type {} status {}",
               __func__,
               __LINE__,
               eth_type,
               status);
    return SWITCH_STATUS_FAILURE;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: eth_type {} refcount {}",
             __func__,
             __LINE__,
             eth_type,
             refcount);

  // Delete etype_acl entries when matching ACL entries are gone
  if (refcount == 1) {
    status |= etype_acl1_delete(eth_type);
    status |= etype_acl2_delete(eth_type);
  }

  status = switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_ETYPE_LABEL, etype_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Failed to retrieve etype_label from object {}",
               __func__,
               __LINE__,
               object_id);
    return status;
  }

  status = etype_label_release(etype_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Failed to release etype_label from object {}",
               __func__,
               __LINE__,
               object_id);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Released allocated etype_label {}",
             __func__,
             __LINE__,
             etype_label);

  return status;
}

switch_status_t MacAddrLabel::allocate(switch_object_id_t port_lag_handle,
                                       switch_mac_addr_t src_mac,
                                       switch_mac_addr_t src_mac_mask,
                                       switch_mac_addr_t dst_mac,
                                       switch_mac_addr_t dst_mac_mask,
                                       uint8_t &macaddr_label) {
  macaddr_compress_key_t key = {
      port_lag_handle, src_mac, src_mac_mask, dst_mac, dst_mac_mask};

  auto it = macaddr_compr_map.find(key);
  if (it == macaddr_compr_map.end()) {
    macaddr_label = idalloc.allocate();
    macaddr_compr_map[key] = std::make_pair(macaddr_label, 1);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Allocated macaddr_label {}",
               __func__,
               __LINE__,
               macaddr_label);
  } else {
    it->second.second++;
    macaddr_label = macaddr_compr_map[key].first;
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Incremented macaddr_label {} refcount {}",
               __func__,
               __LINE__,
               macaddr_label,
               it->second.second);
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t MacAddrLabel::reserve(switch_object_id_t port_lag_handle,
                                      switch_mac_addr_t src_mac,
                                      switch_mac_addr_t src_mac_mask,
                                      switch_mac_addr_t dst_mac,
                                      switch_mac_addr_t dst_mac_mask,
                                      uint8_t macaddr_label) {
  macaddr_compress_key_t key = {
      port_lag_handle, src_mac, src_mac_mask, dst_mac, dst_mac_mask};
  auto it = macaddr_compr_map.find(key);
  if (it == macaddr_compr_map.end()) {
    macaddr_label = idalloc.reserve(macaddr_label);
    macaddr_compr_map[key] = std::make_pair(macaddr_label, 1);
  } else {
    it->second.second++;
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t MacAddrLabel::release(switch_object_id_t port_lag_handle,
                                      switch_mac_addr_t src_mac,
                                      switch_mac_addr_t src_mac_mask,
                                      switch_mac_addr_t dst_mac,
                                      switch_mac_addr_t dst_mac_mask,
                                      uint8_t macaddr_label) {
  macaddr_compress_key_t key = {
      port_lag_handle, src_mac, src_mac_mask, dst_mac, dst_mac_mask};

  auto it = macaddr_compr_map.find(key);
  if (it == macaddr_compr_map.end()) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  if (it->second.first != macaddr_label) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  it->second.second--;
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Decremented macaddr_label {} refcount {}",
             __func__,
             __LINE__,
             macaddr_label,
             it->second.second);

  if (it->second.second == 0) {
    idalloc.release(macaddr_label);
    macaddr_compr_map.erase(key);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Released macaddr_label {}",
               __func__,
               __LINE__,
               macaddr_label);
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t MacAddrLabel::get_refcount(switch_object_id_t port_lag_handle,
                                           switch_mac_addr_t src_mac,
                                           switch_mac_addr_t src_mac_mask,
                                           switch_mac_addr_t dst_mac,
                                           switch_mac_addr_t dst_mac_mask,
                                           uint16_t &refcount) {
  macaddr_compress_key_t key = {
      port_lag_handle, src_mac, src_mac_mask, dst_mac, dst_mac_mask};
  auto it = macaddr_compr_map.find(key);
  if (it == macaddr_compr_map.end()) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
  refcount = it->second.second;

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t MacAddrLabelManager::allocate(
    uint64_t acl_type,
    uint64_t acl_dir,
    switch_object_id_t port_lag_handle,
    switch_mac_addr_t src_mac,
    switch_mac_addr_t src_mac_mask,
    switch_mac_addr_t dst_mac,
    switch_mac_addr_t dst_mac_mask,
    uint8_t &macaddr_label) {
  auto key = std::make_pair(acl_type, acl_dir);
  if (mac_addr_labels[key] == NULL) {
    mac_addr_labels[key] = new MacAddrLabel;
  }
  return mac_addr_labels[key]->allocate(port_lag_handle,
                                        src_mac,
                                        src_mac_mask,
                                        dst_mac,
                                        dst_mac_mask,
                                        macaddr_label);
}

switch_status_t MacAddrLabelManager::reserve(uint64_t acl_type,
                                             uint64_t acl_dir,
                                             switch_object_id_t port_lag_handle,
                                             switch_mac_addr_t src_mac,
                                             switch_mac_addr_t src_mac_mask,
                                             switch_mac_addr_t dst_mac,
                                             switch_mac_addr_t dst_mac_mask,
                                             uint8_t macaddr_label) {
  auto key = std::make_pair(acl_type, acl_dir);
  if (mac_addr_labels[key] == NULL) {
    mac_addr_labels[key] = new MacAddrLabel;
  }
  return mac_addr_labels[key]->reserve(port_lag_handle,
                                       src_mac,
                                       src_mac_mask,
                                       dst_mac,
                                       dst_mac_mask,
                                       macaddr_label);
}

switch_status_t MacAddrLabelManager::release(uint64_t acl_type,
                                             uint64_t acl_dir,
                                             switch_object_id_t port_lag_handle,
                                             switch_mac_addr_t src_mac,
                                             switch_mac_addr_t src_mac_mask,
                                             switch_mac_addr_t dst_mac,
                                             switch_mac_addr_t dst_mac_mask,
                                             uint8_t macaddr_label) {
  switch_status_t status;

  auto key = std::make_pair(acl_type, acl_dir);
  if (mac_addr_labels[key] == NULL) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
  status = mac_addr_labels[key]->release(port_lag_handle,
                                         src_mac,
                                         src_mac_mask,
                                         dst_mac,
                                         dst_mac_mask,
                                         macaddr_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  if (mac_addr_labels[key]->is_empty()) {
    delete mac_addr_labels[key];
    mac_addr_labels.erase(key);
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t MacAddrLabelManager::get_refcount(
    uint64_t acl_type,
    uint64_t acl_dir,
    switch_object_id_t port_lag_handle,
    switch_mac_addr_t src_mac,
    switch_mac_addr_t src_mac_mask,
    switch_mac_addr_t dst_mac,
    switch_mac_addr_t dst_mac_mask,
    uint16_t &refcount) {
  auto key = std::make_pair(acl_type, acl_dir);
  if (mac_addr_labels[key] == NULL) {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
  return mac_addr_labels[key]->get_refcount(
      port_lag_handle, src_mac, src_mac_mask, dst_mac, dst_mac_mask, refcount);
}

bool is_macaddr_label_supported(uint64_t acl_type) {
  auto table_types_supported = {SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS,
                                SWITCH_ACL_TABLE_ATTR_TYPE_IP,  // PBR
                                SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR};
  for (auto tt : table_types_supported) {
    if (acl_type == tt) {
      return true;
    }
  }
  return false;
}

switch_status_t macaddr_label_allocate_or_reserve(std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t acl_type;
  uint64_t acl_dir;
  switch_object_id_t table_handle = {0};
  switch_object_id_t ingress_port_lag_handle = {0};
  switch_object_id_t egress_port_lag_handle = {0};
  switch_object_id_t port_lag_handle = {0};
  switch_mac_addr_t src_mac = {0};
  switch_mac_addr_t src_mac_mask = {0};
  switch_mac_addr_t dst_mac = {0};
  switch_mac_addr_t dst_mac_mask = {0};
  switch_mac_addr_t zero_mac_mask = {0};
  uint8_t macaddr_label = 0;

  auto label_attrs = {SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE,
                      SWITCH_ACL_ENTRY_ATTR_SRC_MAC,
                      SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK,
                      SWITCH_ACL_ENTRY_ATTR_DST_MAC,
                      SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK,
                      SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE,
                      SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE};

  for (auto label_attr : label_attrs) {
    const auto attr_it = attrs.find(static_cast<switch_attr_id_t>(label_attr));
    if (attr_it == attrs.end()) continue;
    switch (label_attr) {
      case SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE:
        attr_it->v_get(table_handle);
        break;
      case SWITCH_ACL_ENTRY_ATTR_SRC_MAC:
        attr_it->v_get(src_mac);
        break;
      case SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK:
        attr_it->v_get(src_mac_mask);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DST_MAC:
        attr_it->v_get(dst_mac);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK:
        attr_it->v_get(dst_mac_mask);
        break;
      case SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE:
        attr_it->v_get(ingress_port_lag_handle);
        break;
      case SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE:
        attr_it->v_get(egress_port_lag_handle);
        break;
    }
  }
  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_macaddr_label_supported(acl_type)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not generating label for table type {}",
               __func__,
               __LINE__,
               acl_type);
    return SWITCH_STATUS_SUCCESS;
  }

  // Generate label only if one of the mac mask is set
  if (src_mac_mask == zero_mac_mask && dst_mac_mask == zero_mac_mask) {
    return SWITCH_STATUS_SUCCESS;
  }

  if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    port_lag_handle = ingress_port_lag_handle;
  } else if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
    port_lag_handle = egress_port_lag_handle;
  }

  const auto index_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_ACL_ENTRY_ATTR_MACADDR_LABEL));
  if (index_attr_it == attrs.end()) {
    status = MacAddrLabelManager::instance()->allocate(acl_type,
                                                       acl_dir,
                                                       port_lag_handle,
                                                       src_mac,
                                                       src_mac_mask,
                                                       dst_mac,
                                                       dst_mac_mask,
                                                       macaddr_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_ACL_ENTRY,
          "{};{}: Failed to allocate macaddr_label for acl type {} dir {}",
          __func__,
          __LINE__,
          acl_type,
          acl_dir);
      return status;
    }
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{};{}: Allocated macaddr_label {} for acl type {} dir {}",
               __func__,
               __LINE__,
               macaddr_label,
               acl_type,
               acl_dir);

    attrs.insert(attr_w(SWITCH_ACL_ENTRY_ATTR_MACADDR_LABEL, macaddr_label));
  } else {
    status = (*index_attr_it).v_get(macaddr_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{};{}: Failed to get allocated macaddr_label {}",
                 __func__,
                 __LINE__,
                 macaddr_label);
      return status;
    }

    status = MacAddrLabelManager::instance()->reserve(acl_type,
                                                      acl_dir,
                                                      port_lag_handle,
                                                      src_mac,
                                                      src_mac_mask,
                                                      dst_mac,
                                                      dst_mac_mask,
                                                      macaddr_label);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ACL_ENTRY,
                 "{};{}: Failed to reserve label {}",
                 __func__,
                 __LINE__,
                 macaddr_label);
      return status;
    }
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Allocated macaddr_label {}",
             __func__,
             __LINE__,
             macaddr_label);

  return status;
}

switch_status_t macaddr_acl_create(const switch_object_id_t object_id,
                                   const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t acl_type;
  uint64_t acl_dir;
  switch_object_id_t table_handle = {0};
  switch_object_id_t ingress_port_lag_handle = {0};
  switch_object_id_t egress_port_lag_handle = {0};
  switch_object_id_t port_lag_handle = {0};
  switch_mac_addr_t src_mac = {0};
  switch_mac_addr_t src_mac_mask = {0};
  switch_mac_addr_t dst_mac = {0};
  switch_mac_addr_t dst_mac_mask = {0};
  switch_mac_addr_t zero_mac_mask = {0};
  uint16_t refcount = 0;

  auto label_attrs = {SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE,
                      SWITCH_ACL_ENTRY_ATTR_SRC_MAC,
                      SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK,
                      SWITCH_ACL_ENTRY_ATTR_DST_MAC,
                      SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK,
                      SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE,
                      SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE};

  for (auto label_attr : label_attrs) {
    const auto attr_it = attrs.find(static_cast<switch_attr_id_t>(label_attr));
    if (attr_it == attrs.end()) continue;
    switch (label_attr) {
      case SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE:
        attr_it->v_get(table_handle);
        break;
      case SWITCH_ACL_ENTRY_ATTR_SRC_MAC:
        attr_it->v_get(src_mac);
        break;
      case SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK:
        attr_it->v_get(src_mac_mask);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DST_MAC:
        attr_it->v_get(dst_mac);
        break;
      case SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK:
        attr_it->v_get(dst_mac_mask);
        break;
      case SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE:
        attr_it->v_get(ingress_port_lag_handle);
        break;
      case SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE:
        attr_it->v_get(egress_port_lag_handle);
        break;
    }
  }
  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_macaddr_label_supported(acl_type)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not creating macaddr acl entry, table type {}",
               __func__,
               __LINE__,
               acl_type);
    return SWITCH_STATUS_SUCCESS;
  }

  // Generate label only if one of the mac mask is set
  if (src_mac_mask == zero_mac_mask && dst_mac_mask == zero_mac_mask) {
    return SWITCH_STATUS_SUCCESS;
  }

  if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    port_lag_handle = ingress_port_lag_handle;
  } else if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
    port_lag_handle = egress_port_lag_handle;
  }

  status = MacAddrLabelManager::instance()->get_refcount(acl_type,
                                                         acl_dir,
                                                         port_lag_handle,
                                                         src_mac,
                                                         src_mac_mask,
                                                         dst_mac,
                                                         dst_mac_mask,
                                                         refcount);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to get refcount status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: refcount {}",
             __func__,
             __LINE__,
             refcount);

  // Create macaddr_acl entries only when creating first acl entry
  status = macaddr_acl_entry_create(object_id, acl_type, acl_dir);

  return status;
}

switch_status_t macaddr_acl_cleanup(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t acl_type;
  uint64_t acl_dir;
  switch_object_id_t table_handle, ingress_port_lag_handle,
      egress_port_lag_handle, port_lag_handle = {0};
  switch_mac_addr_t src_mac;
  switch_mac_addr_t src_mac_mask;
  switch_mac_addr_t dst_mac;
  switch_mac_addr_t dst_mac_mask;
  switch_mac_addr_t zero_mac_mask = {0};
  uint8_t macaddr_label = 0;
  uint16_t refcount = 0;

  status |= switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
  status |=
      switch_store::v_get(object_id, SWITCH_ACL_ENTRY_ATTR_SRC_MAC, src_mac);
  status |= switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK, src_mac_mask);
  status |=
      switch_store::v_get(object_id, SWITCH_ACL_ENTRY_ATTR_DST_MAC, dst_mac);
  status |= switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK, dst_mac_mask);
  status |= switch_store::v_get(object_id,
                                SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE,
                                ingress_port_lag_handle);
  status |= switch_store::v_get(object_id,
                                SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE,
                                egress_port_lag_handle);

  switch_enum_t enum_type;
  switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;
  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  acl_dir = enum_type.enumdata;

  if (!is_macaddr_label_supported(acl_type)) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Not releasing label for table type {}",
               __func__,
               __LINE__,
               acl_type);
    return SWITCH_STATUS_SUCCESS;
  }

  // Label not released when both MAC masks are 0, so just return
  if (src_mac_mask == zero_mac_mask && dst_mac_mask == zero_mac_mask) {
    return SWITCH_STATUS_SUCCESS;
  }

  if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    port_lag_handle = ingress_port_lag_handle;
  } else if (acl_dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS) {
    port_lag_handle = egress_port_lag_handle;
  }

  status = MacAddrLabelManager::instance()->get_refcount(acl_type,
                                                         acl_dir,
                                                         port_lag_handle,
                                                         src_mac,
                                                         src_mac_mask,
                                                         dst_mac,
                                                         dst_mac_mask,
                                                         refcount);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}.{}: Failed to get refcount status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}.{}: refcount {}",
             __func__,
             __LINE__,
             refcount);

  // Delete macaddr_acl entries when matching ACL entries are gone
  status |= macaddr_acl_entry_delete(object_id, acl_type, acl_dir);

  status = switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_MACADDR_LABEL, macaddr_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Failed to retrieve macaddr_label from object {}",
               __func__,
               __LINE__,
               object_id);
    return status;
  }

  status = MacAddrLabelManager::instance()->release(acl_type,
                                                    acl_dir,
                                                    port_lag_handle,
                                                    src_mac,
                                                    src_mac_mask,
                                                    dst_mac,
                                                    dst_mac_mask,
                                                    macaddr_label);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Failed to release macaddr_label from object {}",
               __func__,
               __LINE__,
               object_id);
    return status;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_ENTRY,
             "{}:{}: Released allocated macaddr_label {}",
             __func__,
             __LINE__,
             macaddr_label);

  return status;
}

// Allocate/reserve etype_label and macaddr_label for ACL2 profile
switch_status_t before_acl_entry_create2(const switch_object_type_t object_type,
                                         std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (object_type != SWITCH_OBJECT_TYPE_ACL_ENTRY) {
    return SWITCH_STATUS_FAILURE;
  }

  if (!acl_using_acl2_profile()) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = etype_label_allocate_or_reserve(attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Etype label allocation failed",
               __func__,
               __LINE__);
  }

  status = macaddr_label_allocate_or_reserve(attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Macaddr label allocation failed",
               __func__,
               __LINE__);
  }

  return status;
}

// Create etype_acl and macaddr_acl entry if there is none correspnding
// to the eth_type and mac_addresses for this ACL entry
switch_status_t after_acl_entry_create2(const switch_object_id_t object_id,
                                        const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!acl_using_acl2_profile()) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = etype_acl_create(object_id, attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Etype acl create failed",
               __func__,
               __LINE__);
  }

  status = macaddr_acl_create(object_id, attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Mac addr acl create failed",
               __func__,
               __LINE__);
  }

  return status;
}

// Cleanup etype and macaddr acl entres and labels
switch_status_t before_acl_entry_delete2(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!acl_using_acl2_profile()) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = etype_acl_cleanup(object_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Etype acl cleanup failed",
               __func__,
               __LINE__);
  }

  status = macaddr_acl_cleanup(object_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ACL_ENTRY,
               "{}:{}: Macaddr acl cleanup failed",
               __func__,
               __LINE__);
  }

  return status;
}

LabelManager *LabelManager::instance_ = NULL;
PortGroupManager *PortGroupManager::instance_ = NULL;
PortGroupLabel *PortGroupLabel::instance_ = NULL;
MacAddrLabelManager *MacAddrLabelManager::instance_ = NULL;
IdMap *IdMap::instance_ = NULL;

switch_status_t acl_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(acl_factory, SWITCH_OBJECT_TYPE_ACL_FACTORY);
  REGISTER_OBJECT(default_ingress_system_acl,
                  SWITCH_OBJECT_TYPE_DEFAULT_INGRESS_SYSTEM_ACL);
  REGISTER_OBJECT(default_egress_system_acl,
                  SWITCH_OBJECT_TYPE_DEFAULT_EGRESS_SYSTEM_ACL);
  REGISTER_OBJECT(default_egress_system_acl2,
                  SWITCH_OBJECT_TYPE_DEFAULT_EGRESS_SYSTEM_ACL2);
  REGISTER_OBJECT(ingress_l4_src_port, SWITCH_OBJECT_TYPE_INGRESS_L4_SRC_PORT);
  REGISTER_OBJECT(ingress_l4_dst_port, SWITCH_OBJECT_TYPE_INGRESS_L4_DST_PORT);
  REGISTER_OBJECT(egress_l4_src_port, SWITCH_OBJECT_TYPE_EGRESS_L4_SRC_PORT);
  REGISTER_OBJECT(egress_l4_dst_port, SWITCH_OBJECT_TYPE_EGRESS_L4_DST_PORT);
  REGISTER_OBJECT(egress_tos_mirror_acl,
                  SWITCH_OBJECT_TYPE_EGRESS_TOS_MIRROR_ACL);
  REGISTER_OBJECT(acl_sample_session, SWITCH_OBJECT_TYPE_ACL_SAMPLE_SESSION);
  REGISTER_OBJECT(device_pre_ingress_acl,
                  SWITCH_OBJECT_TYPE_DEVICE_PRE_INGRESS_ACL);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_HOSTIF_TRAP_GROUP, &after_hostif_trap_group_update);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_ACL_GROUP,
                                                  &before_acl_group_update);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_ACL_TABLE,
                                                  &before_acl_table_update2);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_LAG,
                                                  &before_lag_update);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_VLAN,
                                                  &before_vlan_update);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_RIF,
                                                  &before_rif_update);
  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                  &before_acl_entry_create2);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                 &after_acl_entry_create2);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                  &before_acl_entry_delete2);

  return status;
}

switch_status_t acl_clean() { return SWITCH_STATUS_SUCCESS; }
}  // namespace smi
