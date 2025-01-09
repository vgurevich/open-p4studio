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


#include <unordered_map>

#include "switch_tna/utils.h"

namespace std {
template <>
struct hash<switch_device_attr_table> {
  inline size_t operator()(switch_device_attr_table const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};
}  // namespace std

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

std::unordered_map<switch_device_attr_table, bf_rt_table_id_t> table_map;

switch_status_t table_info_get(switch_device_attr_table table_id,
                               switch_table_info_t &table_info) {
  auto it = table_map.find(table_id);
  if (it == table_map.end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Invalid/unsupported table ID: {}",
               __func__,
               __LINE__,
               table_id);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  if (it->second == 0) {
    table_info.table_size = 0;
    table_info.table_usage = 0;
    return SWITCH_STATUS_SUCCESS;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), it->second);
  table.table_size_get(&table_info.table_size);
  table.table_usage_get(&table_info.table_usage);
  table_info.bf_rt_table_id = it->second;

  switch (table_id) {
    case SWITCH_DEVICE_ATTR_TABLE_NEXTHOP:
      // nexthop-id lookup key starts from 1, zero is reserved
      // so we will have to ignore max value as we cant use that
      // value as key [bit-size not enough]. Return max_entries-1
      table_info.table_size -= 1;
      break;

    case SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP:
    case SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP_MEMBERS:
      // 1 entry reserved for default ecmp entry internally
      // 1 entry reserved for default ecmp ap entry internally
      table_info.table_size -= 1;
      break;

    case SWITCH_DEVICE_ATTR_TABLE_NEIGHBOR:
      // nexthop-id will be used as lookup key in neighbor entry
      // nexthop-id starts from 1, nexthop entry 0 is reserved
      // Another two nexthop entries are reserved for glean, drop
      // so return max entries - 3
      table_info.table_size -= 3;
      break;

    default:
      break;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t tables_init() {
  table_map[SWITCH_DEVICE_ATTR_TABLE_DMAC] = smi_id::T_DMAC;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV4_HOST] = smi_id::T_IPV4_FIB_HOST;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV4_LPM] = smi_id::T_IPV4_FIB_LPM;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_HOST] = smi_id::T_IPV6_FIB_HOST;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_HOST64] = smi_id::T_IPV6_FIB_HOST64;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM] = smi_id::T_IPV6_FIB_LPM;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM64] = smi_id::T_IPV6_FIB_LPM64;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM_TCAM] =
      smi_id::T_IPV6_FIB_LPM_TCAM;
  table_map[SWITCH_DEVICE_ATTR_TABLE_NEXTHOP] = smi_id::T_NEXTHOP;
  table_map[SWITCH_DEVICE_ATTR_TABLE_LAG_GROUP] = smi_id::T_LAG;
  table_map[SWITCH_DEVICE_ATTR_TABLE_LAG_GROUP_MEMBERS] =
      smi_id::AP_LAG_SELECTOR;
  table_map[SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP] = smi_id::T_ECMP;
  table_map[SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP_MEMBERS] =
      smi_id::AP_ECMP_SELECTOR;
  table_map[SWITCH_DEVICE_ATTR_TABLE_OUTER_ECMP_GROUP] =
      smi_id::SG_OUTER_ECMP_SELECTOR_GROUP;
  table_map[SWITCH_DEVICE_ATTR_TABLE_OUTER_ECMP_GROUP_MEMBERS] =
      smi_id::AP_OUTER_ECMP_SELECTOR;
  table_map[SWITCH_DEVICE_ATTR_TABLE_PRE_INGRESS_ACL] =
      smi_id::T_PRE_INGRESS_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IP_ACL] = smi_id::T_INGRESS_IP_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV4_ACL] = smi_id::T_INGRESS_IPV4_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV6_ACL] = smi_id::T_INGRESS_IPV6_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_MAC_ACL] = smi_id::T_INGRESS_MAC_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IP_MIRROR_ACL] =
      smi_id::T_INGRESS_IP_MIRROR_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_DTEL_ACL] = smi_id::T_INGRESS_IP_DTEL_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_SYSTEM_ACL] = smi_id::T_INGRESS_SYSTEM_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV4_ACL] =
      smi_id::T_EGRESS_IPV4_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV6_ACL] =
      smi_id::T_EGRESS_IPV6_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_EGRESS_MAC_ACL] = smi_id::T_EGRESS_MAC_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_EGRESS_SYSTEM_ACL] =
      smi_id::T_EGRESS_SYSTEM_ACL;
  table_map[SWITCH_DEVICE_ATTR_TABLE_ECN_ACL] = smi_id::T_ECN;
  table_map[SWITCH_DEVICE_ATTR_TABLE_SNAT] = smi_id::T_INGRESS_NAT_SNAT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_DNAT] = smi_id::T_INGRESS_NAT_DEST_NAT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_FLOW_NAT] = smi_id::T_INGRESS_NAT_FLOW_NAT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_SNAPT] = smi_id::T_INGRESS_NAT_SNAPT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_DNAPT] = smi_id::T_INGRESS_NAT_DEST_NAPT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_FLOW_NAPT] =
      smi_id::T_INGRESS_NAT_FLOW_NAPT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_L4_SRC_PORT] =
      smi_id::T_INGRESS_L4_SRC_PORT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_L4_DST_PORT] =
      smi_id::T_INGRESS_L4_DST_PORT;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV4_MCAST_ROUTE_S_G] =
      smi_id::T_IPV4_MULTICAST_ROUTE_S_G;
  table_map[SWITCH_DEVICE_ATTR_TABLE_IPV4_MCAST_ROUTE_X_G] =
      smi_id::T_IPV4_MULTICAST_ROUTE_X_G;
  table_map[SWITCH_DEVICE_ATTR_TABLE_NEIGHBOR] = smi_id::T_NEIGHBOR;
  table_map[SWITCH_DEVICE_ATTR_TABLE_MPLS] = smi_id::T_MPLS_FIB;
  table_map[SWITCH_DEVICE_ATTR_TABLE_MY_SID] = smi_id::T_MY_SID;
  table_map[SWITCH_DEVICE_ATTR_TABLE_TUNNEL] =
      smi_id::T_TUNNEL_SRC_ADDR_REWRITE;

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t tables_clean() {
  table_map.clear();
  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
