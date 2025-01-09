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


#include <saiinternal.h>

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_ACL;

namespace std {
template <>
struct hash<switch_acl_table_attr_type> {
  inline size_t operator()(switch_acl_table_attr_type const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};
}  // namespace std

bool sai_acl_using_acl2_profile() {
  // For now, using below checks to figure out if acl2.p4 is in use
  //  - actions transit/deny
  if (bf_switch_is_feature_enabled(
          SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION) ||
      bf_switch_is_feature_enabled(
          SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION)) {
    return true;
  }
  return false;
}

// Verify the unsupported fields below
// We want the IP acl to be processed last because this is basically a super set
// of other IP ACLs.
// For X1 profile, table type resolves to IPv4 or IPv6.
// For X4 profile, table type will miss and end up with IP for PINS use case.
// For regular Sonic path, they will still resolve to IPv4 or IPv6, but they are
// changed to IP if SHARED_IP_ACL is enabled.
typedef std::pair<switch_acl_table_attr_type, std::set<sai_attr_id_t>>
    switch_acl_table_type_to_fields;
const std::vector<switch_acl_table_type_to_fields> sai_acl_tbl_patterns = {
    // clang-format off
    {SWITCH_ACL_TABLE_ATTR_TYPE_IPV4,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_ENTRY_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_IPV6,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_FLOW_LABEL,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_ENTRY_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_MAC,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_DST_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_PRI,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_CFI,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_DTEL,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_TUNNEL_VNI,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_ENTRY_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_DST_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_CFI,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_PRI}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_ENTRY_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_DST_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_CFI,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_PRI}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD,
     {SAI_ACL_TABLE_ATTR_FIELD_TC,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_ECN,
      {SAI_ACL_TABLE_ATTR_FIELD_DSCP,
       SAI_ACL_TABLE_ATTR_FIELD_ECN}},
    // clang-format on
    {SWITCH_ACL_TABLE_ATTR_TYPE_EGRESS_SYSTEM,
     {SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_IP,
     {SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER,
      SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_TABLE_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_IPV6_FLOW_LABEL,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE,
      SAI_ACL_TABLE_ATTR_FIELD_DST_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0,
      SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,
      SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_CFI,
      SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_PRI}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS,
     {SAI_ACL_TABLE_ATTR_FIELD_DST_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC,
      SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_TABLE_ATTR_FIELD_ECN,
      SAI_ACL_TABLE_ATTR_FIELD_TOS,
      SAI_ACL_TABLE_ATTR_FIELD_IN_PORT}},
    {SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR, {SAI_ACL_TABLE_ATTR_FIELD_DSCP}}};

const std::set<sai_attr_id_t> unsupported_attr_ids = {
    SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,       // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_IP_FLAGS,          // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META,     // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_TUNNEL_VNI,        // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_INNER_ETHER_TYPE,  // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_INNER_SRC_IP,      // Unsupported
    SAI_ACL_TABLE_ATTR_FIELD_INNER_DST_IP,      // Unsupported
    SAI_ACL_TABLE_ATTR_SIZE,                    // Unsupported
};

/*
 * Routine Description:
 *   Converts switch ACL action type to SAI ACL action type
 *   the switch ACL table type.
 *
 * Arguments:
 *  [in] action - switch action type
 *  [in] stage - SAI ACL stage
 *
 * Return Values:
 *    sai_acl_action_type_t
 */
sai_acl_action_type_t sai_switch_acl_action_type_to_sai(
    _In_ uint8_t action, _In_ sai_acl_stage_t stage) {
  switch (action) {
    case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT:
      return SAI_ACL_ACTION_TYPE_PACKET_ACTION;

    case SWITCH_ACL_ENTRY_ATTR_REDIRECT:
      return SAI_ACL_ACTION_TYPE_REDIRECT;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE:
      return SAI_ACL_ACTION_TYPE_MIRROR_INGRESS;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE:
      return SAI_ACL_ACTION_TYPE_MIRROR_EGRESS;

    case SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA:
      return SAI_ACL_ACTION_TYPE_SET_ACL_META_DATA;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE:
      return SAI_ACL_ACTION_TYPE_SET_POLICER;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC:
      return SAI_ACL_ACTION_TYPE_SET_TC;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR:
      return SAI_ACL_ACTION_TYPE_SET_PACKET_COLOR;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE:
      return SAI_ACL_ACTION_TYPE_COUNTER;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_ALL_PACKETS:
      return SAI_ACL_ACTION_TYPE_DTEL_REPORT_ALL_PACKETS;

    case SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE:
      return SAI_ACL_ACTION_TYPE_INGRESS_SAMPLEPACKET_ENABLE;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_DISABLE_NAT:
      return SAI_ACL_ACTION_TYPE_NO_NAT;

    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_ECN:
      return SAI_ACL_ACTION_TYPE_SET_ECN;

    default:
      break;
  }

  return SAI_ACL_ACTION_TYPE_PACKET_ACTION;
}

/*
 * Routine Description:
 *   Gets SAI ACL supported actions.
 *
 * Arguments:
 *  [in] device - device handle
 *  [in] stage - ACL stage
 *  [out] sai_actions_list - List of ACL actions per stage
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_acl_get_supported_actions(
    _In_ switch_object_id_t device,
    _In_ sai_acl_stage_t stage,
    _Out_ std::set<sai_acl_action_type_t> &sai_actions_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  smi::attr_w ingress_actions_attr(SWITCH_DEVICE_ATTR_INGRESS_ACL_ACTIONS);
  smi::attr_w egress_actions_attr(SWITCH_DEVICE_ATTR_EGRESS_ACL_ACTIONS);
  std::vector<uint8_t> ingress_actions_list;
  std::vector<uint8_t> egress_actions_list;

  if (stage == SAI_ACL_STAGE_INGRESS) {
    switch_status = bf_switch_attribute_get(
        device, SWITCH_DEVICE_ATTR_INGRESS_ACL_ACTIONS, ingress_actions_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get list of supported ingress ACL actions, error: %s\n",
          sai_metadata_get_status_name(status));
      return status;
    }
    ingress_actions_attr.v_get(ingress_actions_list);

    for (auto switch_action : ingress_actions_list) {
      auto sai_action = sai_switch_acl_action_type_to_sai(
          switch_action, SAI_ACL_STAGE_INGRESS);
      if (sai_actions_list.find(sai_action) == sai_actions_list.end()) {
        sai_actions_list.insert(sai_action);
      }
    }
    // For DTEL, add more
    if (sai_actions_list.find(SAI_ACL_ACTION_TYPE_DTEL_REPORT_ALL_PACKETS) !=
        sai_actions_list.end()) {
      sai_actions_list.insert(SAI_ACL_ACTION_TYPE_ACL_DTEL_FLOW_OP);
      sai_actions_list.insert(SAI_ACL_ACTION_TYPE_DTEL_DROP_REPORT_ENABLE);
      sai_actions_list.insert(SAI_ACL_ACTION_TYPE_DTEL_TAIL_DROP_REPORT_ENABLE);
    }
  }

  if (stage == SAI_ACL_STAGE_EGRESS) {
    switch_status = bf_switch_attribute_get(
        device, SWITCH_DEVICE_ATTR_EGRESS_ACL_ACTIONS, egress_actions_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get list of supported egress ACL actions, error: %s\n",
          sai_metadata_get_status_name(status));
      return status;
    }
    egress_actions_attr.v_get(egress_actions_list);

    for (auto switch_action : egress_actions_list) {
      auto sai_action = sai_switch_acl_action_type_to_sai(switch_action,
                                                          SAI_ACL_STAGE_EGRESS);
      if (sai_actions_list.find(sai_action) == sai_actions_list.end()) {
        sai_actions_list.insert(sai_action);
      }
    }
  }

  return status;
}

/*
 * Routine Description:
 *   Using the SAI ACL table attributes gets
 *   the switch ACL table type.
 *
 * Arguments:
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *  [out] acl_type - switch ACL table type
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl_tbl_type_get_from_attributes(
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list,
    _Out_ switch_acl_table_attr_type &acl_type) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_acl_table_attr_type acl_type_tmp = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  bool matched = true;
  bool mirror = false, dtel = false;
  bool ingress = true;
  bool pre_ingress = false;
  bool user_meta = false, set_user_meta = false;
  bool bp_switch = false;
  bool set_tc = false;
  bool set_policer = false;
  uint32_t index = 0;
  bool user_defined_fields = false;
  // bool redirect = false;
  bool match_dscp = false;
  uint32_t acl_table_field_count = 0;

  if (!attr_list) {
    SAI_LOG_ERROR("Null attribute list parameter passed");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (auto tbl_iter = sai_acl_tbl_patterns.begin();
       tbl_iter != sai_acl_tbl_patterns.end();
       ++tbl_iter) {
    matched = true;
    acl_type_tmp = tbl_iter->first;
    SAI_LOG_DEBUG("ACL table type tmp %d", acl_type_tmp);
    for (index = 0; index < attr_count; index++) {
      SAI_LOG_DEBUG(
          "ACL attrib %s",
          sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE, attr_list[index].id));
      if (attr_list[index].id >= SAI_ACL_TABLE_ATTR_FIELD_START &&
          attr_list[index].id < SAI_ACL_TABLE_ATTR_FIELD_END) {
        acl_table_field_count++;
      }
      // skip ports and VLAN attributes on check
      switch (attr_list[index].id) {
        case SAI_ACL_TABLE_ATTR_ACL_STAGE: {
          if (attr_list[index].value.s32 == SAI_ACL_STAGE_EGRESS) {
            ingress = false;
          }
          if (attr_list[index].value.s32 == SAI_ACL_STAGE_PRE_INGRESS) {
            pre_ingress = true;
          }
        } break;
        case SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META: {
          if (attr_list[index].value.booldata) {
            user_meta = true;
          }
        } break;
        case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST: {
          if (attr_list[index].value.s32list.list[0] ==
              SAI_ACL_BIND_POINT_TYPE_SWITCH) {
            bp_switch = true;
          }
        } break;
        case SAI_ACL_TABLE_ATTR_SIZE:
          matched = true;
          break;
        case SAI_ACL_TABLE_ATTR_FIELD_DSCP:
          match_dscp = true;
          break;
        case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST: {
          for (uint32_t i = 0; i < attr_list[index].value.s32list.count; i++) {
            switch (attr_list[index].value.s32list.list[i]) {
              case SAI_ACL_ACTION_TYPE_MIRROR_INGRESS:
              case SAI_ACL_ACTION_TYPE_MIRROR_EGRESS:
                mirror = true;
                break;
              case SAI_ACL_ACTION_TYPE_REDIRECT:
                // redirect = true;
                break;
              case SAI_ACL_ACTION_TYPE_DTEL_DROP_REPORT_ENABLE:
              case SAI_ACL_ACTION_TYPE_DTEL_TAIL_DROP_REPORT_ENABLE:
              case SAI_ACL_ACTION_TYPE_ACL_DTEL_FLOW_OP:
              case SAI_ACL_ACTION_TYPE_DTEL_INT_SESSION:
              case SAI_ACL_ACTION_TYPE_DTEL_FLOW_SAMPLE_PERCENT:
              case SAI_ACL_ACTION_TYPE_DTEL_REPORT_ALL_PACKETS:
                dtel = true;
                break;
              case SAI_ACL_ACTION_TYPE_SET_ACL_META_DATA:
                set_user_meta = true;
                break;
              case SAI_ACL_ACTION_TYPE_SET_POLICER:
                set_policer = true;
                break;
              case SAI_ACL_ACTION_TYPE_SET_TC:
                set_tc = true;
                break;
              default:
                break;
            }
          }
          if (dtel && acl_type_tmp != SWITCH_ACL_TABLE_ATTR_TYPE_DTEL) {
            matched = false;
          }
        } break;
        // ignore above for matching fields
        default:
          if (attr_list[index].id >=
                  SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MIN &&
              attr_list[index].id <=
                  SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MAX) {
            // Check if the match field is in the table pattern qualifiers list
            user_defined_fields = true;
            matched = true;
            break;
          }

          if (unsupported_attr_ids.find(attr_list[index].id) !=
              unsupported_attr_ids.end()) {
            // Handle unsupported attributes we silently ignore
            matched = true;
            break;
          }

          if (attr_list[index].id >= SAI_ACL_TABLE_ATTR_FIELD_START &&
              attr_list[index].id <= SAI_ACL_TABLE_ATTR_FIELD_END) {
            // Check if the match field is in the table pattern qualifiers list
            if (tbl_iter->second.find(attr_list[index].id) ==
                tbl_iter->second.end()) {
              matched = false;
            }
          }
          break;
      }
      // this pattern doesn't have this field, try next
      if (!matched) break;
    }

    /* User metadata can be an acl key field only in egress*/
    if (user_meta && ingress) {
      matched = false;
      SAI_LOG_DEBUG(
          "User metadata is allowed as ACL key only in Egress stage.\n");
    }
    /* ACL Action set user meta is supported only in ingress*/
    if (set_user_meta && !ingress) {
      matched = false;
      SAI_LOG_ERROR(
          "ACL Action set user meta is supported only in ingress stage.\n");
    }

    // The assumption for ingress acl is,
    // if shared_ip_acl, then acl type for IPv4 and IPv6 will be set to IP
    // if shared_ip_acl, then acl type for IP_MIRROR is
    //   - IP_MIRROR if feature SWITCH_FEATURE_INGRESS_MIRROR_ACL is true
    //   - else IP
    bool shared_ip_acl_feature =
        bf_switch_is_feature_enabled(SWITCH_FEATURE_SHARED_INGRESS_IP_ACL);
    bool ingress_ip_mirror_acl_feature =
        bf_switch_is_feature_enabled(SWITCH_FEATURE_INGRESS_MIRROR_ACL);

    // We have gone through all entries in this pattern
    if (matched && (index == attr_count)) {
      acl_type = acl_type_tmp;
      if (pre_ingress) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_PREINGRESS;
      } else if (mirror) {
        // IP_MIRROR
        //  - If stage is ingress && SWITCH_FEATURE_INGRESS_MIRROR_ACL is true
        //  - If stage is egress  [Will calculate exact acl table in SMI]
        // IP
        //  - If stage is ingress && SWITCH_FEATURE_INGRESS_MIRROR_ACL is false
        if (ingress) {
          if (ingress_ip_mirror_acl_feature) {
            acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
          } else {
            acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
          }
        } else {
          acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
        }
      } else if (dtel) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_DTEL;
        if (acl_type_tmp != SWITCH_ACL_TABLE_ATTR_TYPE_DTEL) {
          SAI_LOG_WARN("Incorrect ACL type macth. Please check it");
        }
      } else if (user_defined_fields) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
      } else if (ingress && bp_switch && set_policer && set_tc) {
        // This should be etrap
        if (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4) {
          acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_ETRAP;
        } else if (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6) {
          acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_ETRAP;
        }
      }
      if (ingress && shared_ip_acl_feature &&
          (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4 ||
           acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6)) {
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
      }

      if (match_dscp && acl_table_field_count == 1) {
        // select DSCP_MIRROR acl_table type only when the acl table
        // contains only DSCP match field.
        // SONiC creates ACL table with only DSCP field for DSCP_MIRROR table.
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR;
      }
      SAI_LOG_DEBUG("ACL type %d", acl_type);
      return SAI_STATUS_SUCCESS;
    }
  }

  return status;
}

/*
 * Routine Description:
 *   Using the SAI ACL2 table attributes gets
 *   the switch ACL table type just for acl2.p4 profile
 *
 * Arguments:
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *  [out] acl_type - switch ACL table type
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl2_tbl_type_get_from_attributes(
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list,
    _Out_ switch_acl_table_attr_type &acl_type) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_acl_table_attr_type acl_type_tmp = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  bool matched = true;
  bool mirror = false;
  bool ingress = true;
  bool bp_switch = false;
  bool qos_actions = false;
  bool set_policer = false;
  uint32_t idx = 0;
  bool user_def_fields = false;
  bool redirect = false;
  uint32_t acl_table_attrib_count = 0;
  // bool user_acl_pkt_actions = false;

  if (!attr_list) {
    SAI_LOG_ERROR("Null attribute list parameter passed");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (auto tbl_iter = sai_acl_tbl_patterns.begin();
       tbl_iter != sai_acl_tbl_patterns.end();
       ++tbl_iter) {
    matched = true;
    acl_type_tmp = tbl_iter->first;
    SAI_LOG_DEBUG("Checking ACL2 type tmp %d", acl_type_tmp);
    for (idx = 0; idx < attr_count; idx++) {
      SAI_LOG_DEBUG(
          "  For ACL2 attrib %s .....",
          sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE, attr_list[idx].id));
      if (attr_list[idx].id >= SAI_ACL_TABLE_ATTR_FIELD_START &&
          attr_list[idx].id < SAI_ACL_TABLE_ATTR_FIELD_END) {
        acl_table_attrib_count++;
      }

      // skip some config attributes & ignore them while checking fields
      // continue matching for other fields
      switch (attr_list[idx].id) {
        case SAI_ACL_TABLE_ATTR_ACL_STAGE: {
          if (attr_list[idx].value.s32 == SAI_ACL_STAGE_EGRESS) {
            ingress = false;
          }
        } break;
        case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST: {
          if (attr_list[idx].value.s32list.list[0] ==
              SAI_ACL_BIND_POINT_TYPE_SWITCH) {
            bp_switch = true;
          }
        } break;
        case SAI_ACL_TABLE_ATTR_SIZE:
          break;
        // check for certain actions to figure out table type
        case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST: {
          for (uint32_t i = 0; i < attr_list[idx].value.s32list.count; i++) {
            switch (attr_list[idx].value.s32list.list[i]) {
              case SAI_ACL_ACTION_TYPE_MIRROR_INGRESS:
              case SAI_ACL_ACTION_TYPE_MIRROR_EGRESS:
                mirror = true;
                break;
              case SAI_ACL_ACTION_TYPE_REDIRECT:
                redirect = true;
                break;
              case SAI_ACL_ACTION_TYPE_SET_POLICER:
                set_policer = true;
                break;
              case SAI_ACL_ACTION_TYPE_SET_TC:
              case SAI_ACL_ACTION_TYPE_SET_DSCP:
              case SAI_ACL_ACTION_TYPE_SET_ECN:
              case SAI_ACL_ACTION_TYPE_SET_OUTER_VLAN_PRI:
              case SAI_ACL_ACTION_TYPE_SET_PACKET_COLOR:
                qos_actions = true;
                break;
              case SAI_ACL_ACTION_TYPE_PACKET_ACTION:
                // user_acl_pkt_actions = true;
                break;
              default:
                break;
            }
          }
        } break;
        // ignore above for matching fields
        default:
          if (attr_list[idx].id >=
                  SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MIN &&
              attr_list[idx].id <=
                  SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MAX) {
            // Check if the match field is in the table pattern qualifiers list
            user_def_fields = true;
            break;
          }

          if (unsupported_attr_ids.find(attr_list[idx].id) !=
              unsupported_attr_ids.end()) {
            // unsupported attributes we silently ignore
            break;
          }

          if (attr_list[idx].id >= SAI_ACL_TABLE_ATTR_FIELD_START &&
              attr_list[idx].id <= SAI_ACL_TABLE_ATTR_FIELD_END) {
            // Check if the match field is in the table pattern qualifiers list
            // if exists - continue matching other fields
            if (tbl_iter->second.find(attr_list[idx].id) ==
                tbl_iter->second.end()) {
              matched = false;
            }
          }
          break;
      }
      // Table pattern doesn't have this field, try next table
      if (!matched) {
        break;
      }
    }

    bool ip_ingress_pbr =
        bf_switch_is_feature_enabled(SWITCH_FEATURE_SHARED_INGRESS_IP_ACL);
    bool ingress_ip_mirror_acl =
        bf_switch_is_feature_enabled(SWITCH_FEATURE_INGRESS_MIRROR_ACL);
    // We have gone through all entries in this pattern
    if (matched && (idx == attr_count)) {
      acl_type = acl_type_tmp;
      if (mirror) {
        if (ingress) {
          if (ingress_ip_mirror_acl) {
            acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
          } else {
            // setting none for now, so that table prog fails
            acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
          }
        } else {
          acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
        }
      } else if (redirect && (ip_ingress_pbr || user_def_fields)) {
        // Assumption for ingress pbr acl
        //   - if shared_ip_acl, then acl type is IP
        //   - redirect nhop/port objects are passed using user  def fields
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
      } else if (set_policer || qos_actions) {
        // qos acl
        acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS;
      }

      SAI_LOG_DEBUG("ACL2 table type  %d", acl_type);
      SAI_LOG_DEBUG(
          "ACL2 tables in this profile: ingress_ip_mirror_acl  %d, "
          "ip_ingress_pbr  %d",
          ingress_ip_mirror_acl,
          ip_ingress_pbr);
      SAI_LOG_DEBUG(
          "ACL2 config attributesa or actions from user: "
          " user fields  %d, redirect  %d, mirror %d, qos_actions %d ",
          user_def_fields,
          redirect,
          mirror,
          qos_actions);
      if (bp_switch == false) {
        SAI_LOG_WARN(" Warning: ACL2 table bpoint is not switch  ");
      }
      return SAI_STATUS_SUCCESS;
    }
  }

  return status;
}

/*
 * Routine Description:
 *   Converts SAI packet color to switch color.
 *
 * Arguments:
 *  [in] sai_color - SAI packet color
 *  [out] sw_color - switch color
 *
 * Return Values:
 *    Void
 */
static void sai_color_to_switch(
    _In_ sai_packet_color_t sai_color,
    _Out_ switch_acl_entry_attr_action_set_color &sw_color) {
  switch (sai_color) {
    case SAI_PACKET_COLOR_GREEN:
      sw_color = SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_GREEN;
      break;
    case SAI_PACKET_COLOR_YELLOW:
      sw_color = SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_YELLOW;
      break;
    case SAI_PACKET_COLOR_RED:
      sw_color = SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_RED;
      break;
    default:
      SAI_LOG_ERROR("Invalid sai color");
      sw_color = SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_MAX;
      break;
  }
}

/*
 * Routine Description:
 *   Converts switch packet color to SAI color.
 *
 * Arguments:
 *  [in] sai_color - SAI packet color
 *  [out] sw_color - switch color
 *
 * Return Values:
 *    Void
 */
static void sai_switch_color_to_sai(_In_ switch_acl_entry_attr_action_set_color
                                        sw_color,
                                    _Out_ sai_packet_color_t &sai_color) {
  switch (sw_color) {
    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_GREEN:
      sai_color = SAI_PACKET_COLOR_GREEN;
      break;
    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_YELLOW:
      sai_color = SAI_PACKET_COLOR_YELLOW;
      break;
    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_RED:
      sai_color = SAI_PACKET_COLOR_RED;
      break;
    default:
      SAI_LOG_ERROR("Invalid switch color");
      sai_color = SAI_PACKET_COLOR_GREEN;
      break;
  }
}

/*
 * Routine Description:
 *   Using the SAI ACL entry attributes updates
 *   the switch ACL table type - for acl.p4
 *
 * Arguments:
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl_table_type_update(
    _In_ uint32_t attr_count, _In_ const sai_attribute_t *attr_list) {
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_object_id_t acl_table_id = SAI_NULL_OBJECT_ID;
  bool mirror_acl = false, dtel = false;
  bool set_policer = false;
  bool set_tc = false;
  bool user_defined_fields = false;

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_ACL_ENTRY_ATTR_TABLE_ID:
        acl_table_id = attr_list[index].value.oid;
        break;

      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS:
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS:
        mirror_acl = true;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_ACL_DTEL_FLOW_OP:
      case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_DROP_REPORT_ENABLE:
      case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_TAIL_DROP_REPORT_ENABLE:
      case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_REPORT_ALL_PACKETS:
        dtel = true;
        break;

      case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_INT_SESSION:          // Unsupported
      case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_FLOW_SAMPLE_PERCENT:  // Unsupported
        break;

      case SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER:
        set_policer = true;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
        set_tc = true;
        break;
      default:
        if (attr_list[index].id >=
                SAI_ACL_ENTRY_ATTR_USER_DEFINED_FIELD_GROUP_MIN &&
            attr_list[index].id <=
                SAI_ACL_ENTRY_ATTR_USER_DEFINED_FIELD_GROUP_MAX) {
          user_defined_fields = true;
          break;
        }
        break;
    }
  }

  if (acl_table_id == SAI_NULL_OBJECT_ID) {
    SAI_LOG_ERROR("Failed to update acl type. acl table id is null\n");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t sw_object_id = {.data = acl_table_id};
  smi::attr_w sw_type_attr(SWITCH_ACL_TABLE_ATTR_TYPE);
  switch_enum_t sw_curr_type = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t sw_new_type = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  smi::attr_w sw_direction_attr(SWITCH_ACL_TABLE_ATTR_DIRECTION);
  switch_enum_t sw_direction_enum = {.enumdata =
                                         SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_DIRECTION, sw_direction_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL table direction, error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }
  sw_direction_attr.v_get(sw_direction_enum);

  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_TYPE, sw_type_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL table type, error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }
  sw_type_attr.v_get(sw_curr_type);
  sw_new_type = sw_curr_type;

  switch_enum_t tbl_bp_enum = {};
  smi::attr_w tbl_bp_attr(SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE);
  std::vector<switch_enum_t> tbl_bp_list;
  sw_status =
      bf_switch_attribute_get(sw_object_id, tbl_bp_attr.id_get(), tbl_bp_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL table bind point type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  tbl_bp_attr.v_get(tbl_bp_list);
  tbl_bp_enum = tbl_bp_list.back();

  // During table creation the action attributes may not be passed.
  // In such case a table of IP type will be created instead of
  // mirror/etrap/dtel
  // one as mirror/etrap/dtel table has the same qualifiers and can be
  // identified
  // by the action attribute only. When an ACL entry is created we may
  // finally identify the mirror/etrap/dtel table type and update it.

  bool shared_ip_acl_feature =
      bf_switch_is_feature_enabled(SWITCH_FEATURE_SHARED_INGRESS_IP_ACL);
  bool ingress_ip_mirror_acl_feature =
      bf_switch_is_feature_enabled(SWITCH_FEATURE_INGRESS_MIRROR_ACL);

  // IP_MIRROR
  //  - If stage is ingress && SWITCH_FEATURE_INGRESS_MIRROR_ACL is true
  //  - If stage is egress  [Will calculate exact acl table in SMI]
  // IP
  //  - If stage is ingress && SWITCH_FEATURE_INGRESS_MIRROR_ACL is false
  if (mirror_acl &&
      sw_curr_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_TOS_MIRROR) {
    if (sw_direction_enum.enumdata == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
      if (ingress_ip_mirror_acl_feature) {
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
      } else {
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
      }
    } else {
      sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
    }
  } else if (user_defined_fields) {
    sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
  } else if (set_policer && set_tc) {
    // Figure out whether this is etrap table
    // Etrap can work on switch level and ingress stage only
    if ((tbl_bp_enum.enumdata ==
         SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_SWITCH) &&
        (sw_direction_enum.enumdata ==
         SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS)) {
      if (sw_curr_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4) {
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_ETRAP;
      } else if (sw_curr_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6) {
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_ETRAP;
      }
    }
  }
  if (sw_direction_enum.enumdata == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS &&
      shared_ip_acl_feature &&
      (sw_new_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4 ||
       sw_new_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6)) {
    sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
  }

  if (sw_curr_type.enumdata != sw_new_type.enumdata) {
    smi::attr_w acl_tab_attr(SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES);
    std::vector<switch_object_id_t> acl_entry_list;

    status = bf_switch_attribute_get(
        sw_object_id, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, acl_tab_attr);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Fail to get ACL table entries list, error: %s\n",
                    sai_metadata_get_status_name(status));
      return status;
    }
    acl_tab_attr.v_get(acl_entry_list);
    // reset the table type to the new one only if the acl table has no entries
    if (acl_entry_list.size() > 0) {
      SAI_LOG_ERROR(
          "ACL entries list in the acl table is not empty, "
          "cant update the acl able type 0x%" PRIx64,
          sw_new_type.enumdata);
      return SAI_STATUS_SUCCESS;
    }
  }

  if (dtel) {
    sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_DTEL;
  }

  if (sw_curr_type.enumdata != sw_new_type.enumdata) {
    sw_type_attr.v_set(sw_new_type);
    sw_status = bf_switch_attribute_set(sw_object_id, sw_type_attr);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set ACL table type, error: %s\n",
                    sai_metadata_get_status_name(status));
      return status;
    }

    SAI_LOG_DEBUG("ACL current table type 0x%" PRIx64
                  ", new table type 0x%" PRIx64,
                  sw_curr_type.enumdata,
                  sw_new_type.enumdata);
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Using the SAI ACL2 entry attributes updates
 *   the switch ACL table type - for acl2.p4
 *
 * Arguments:
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl2_table_type_update(
    _In_ uint32_t attr_count, _In_ const sai_attribute_t *attr_list) {
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_object_id_t acl_table_id = SAI_NULL_OBJECT_ID;
  bool mirror = false;
  bool set_policer = false;
  bool qos_actions = false;
  bool user_def_fields = false;
  bool redirect = false;

  for (uint32_t idx = 0; idx < attr_count; idx++) {
    switch (attr_list[idx].id) {
      case SAI_ACL_ENTRY_ATTR_TABLE_ID:
        acl_table_id = attr_list[idx].value.oid;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS:
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS:
        mirror = true;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT:
        redirect = true;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER:
        set_policer = true;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP:
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_ECN:
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI:
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_PACKET_COLOR:
        qos_actions = true;
        break;
      default:
        if (attr_list[idx].id >=
                SAI_ACL_ENTRY_ATTR_USER_DEFINED_FIELD_GROUP_MIN &&
            attr_list[idx].id <=
                SAI_ACL_ENTRY_ATTR_USER_DEFINED_FIELD_GROUP_MAX) {
          user_def_fields = true;
          break;
        }
        break;
    }
  }

  if (acl_table_id == SAI_NULL_OBJECT_ID) {
    SAI_LOG_ERROR("Failed to update acl2 table type. acl table id is null\n");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t sw_object_id = {.data = acl_table_id};
  smi::attr_w sw_type_attr(SWITCH_ACL_TABLE_ATTR_TYPE);
  switch_enum_t sw_curr_type = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t sw_new_type = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  smi::attr_w sw_direction_attr(SWITCH_ACL_TABLE_ATTR_DIRECTION);
  switch_enum_t sw_direction_enum = {.enumdata =
                                         SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_DIRECTION, sw_direction_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL2 table direction, error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }
  sw_direction_attr.v_get(sw_direction_enum);

  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_TYPE, sw_type_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL2 table type, error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }
  sw_type_attr.v_get(sw_curr_type);
  sw_new_type = sw_curr_type;

  switch_enum_t tbl_bp_enum = {};
  smi::attr_w tbl_bp_attr(SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE);
  std::vector<switch_enum_t> tbl_bp_list;
  sw_status =
      bf_switch_attribute_get(sw_object_id, tbl_bp_attr.id_get(), tbl_bp_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL2 table bpoint type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  tbl_bp_attr.v_get(tbl_bp_list);
  tbl_bp_enum = tbl_bp_list.back();

  // During table creation the action attributes may not be passed.
  // In such case, a table of IP type will be alloc to start with
  // mirror/pbr table has the same qualifiers and can figure out only
  // by the action attribute. When an ACL entry is prog, we may
  // finally calculate the mirror/pbr table type and update it.

  bool ip_ingress_pbr =
      bf_switch_is_feature_enabled(SWITCH_FEATURE_SHARED_INGRESS_IP_ACL);
  bool ingress_ip_mirror_acl =
      bf_switch_is_feature_enabled(SWITCH_FEATURE_INGRESS_MIRROR_ACL);

  // Assunption for IP_MIRROR Acl
  //  - If stage is ingress && SWITCH_FEATURE_INGRESS_MIRROR_ACL is true
  //  - If stage is egress  [Will calculate exact acl table in SMI]
  if (mirror) {
    if (sw_direction_enum.enumdata == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
      if (ingress_ip_mirror_acl) {
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
      } else {
        // setting none so that table prog fails
        sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
      }
    } else {
      sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR;
    }
  } else if (redirect && (ip_ingress_pbr || user_def_fields)) {
    // Assumption for ingress pbr acl
    //   - if shared_ip_acl, then acl type is IP
    //   - redirect nhop/port objects are passed using user  def fields
    sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP;
  } else if (set_policer || qos_actions) {
    // qos acl
    sw_new_type.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IP_QOS;
  }

  if (sw_curr_type.enumdata != sw_new_type.enumdata) {
    smi::attr_w acl_tab_attr(SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES);
    std::vector<switch_object_id_t> acl_entry_list;

    status = bf_switch_attribute_get(
        sw_object_id, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, acl_tab_attr);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Fail to get ACL2 table entries list, error: %s\n",
                    sai_metadata_get_status_name(status));
      return status;
    }
    acl_tab_attr.v_get(acl_entry_list);
    // reset the table type to the new one only if the acl table has no entries
    if (acl_entry_list.size() > 0) {
      SAI_LOG_ERROR(
          "ACL entries list in the acl2 table is not empty, "
          "cant update the acl2 table type to 0x%" PRIx64,
          sw_new_type.enumdata);
      return SAI_STATUS_SUCCESS;
    }
  }

  if (sw_curr_type.enumdata != sw_new_type.enumdata) {
    sw_type_attr.v_set(sw_new_type);
    sw_status = bf_switch_attribute_set(sw_object_id, sw_type_attr);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set ACL table type, error: %s\n",
                    sai_metadata_get_status_name(status));
      return status;
    }

    SAI_LOG_DEBUG("ACL2 current table type 0x%" PRIx64
                  ", new table type 0x%" PRIx64,
                  sw_curr_type.enumdata,
                  sw_new_type.enumdata);
    SAI_LOG_DEBUG(
        "ACL2 tables in this profile: ingress_ip_mirror_acl  %d, "
        "ip_ingress_pbr  %d",
        ingress_ip_mirror_acl,
        ip_ingress_pbr);
    SAI_LOG_DEBUG(
        "ACL2 config attributes or actions from user: "
        " user fields  %d, redirect  %d, mirror %d, qos_actions %d ",
        user_def_fields,
        redirect,
        mirror,
        qos_actions);
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Converts SAI ACL entry range list to
 *   switch attributes
 *
 * Arguments:
 *  [in] range_list - range list
 *  [out] sw_attrs - switch ACL entry attributes list
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl_entry_range_list_to_switch(
    _In_ const sai_object_list_t &range_list,
    _Out_ std::set<smi::attr_w> &sw_attrs) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  for (uint32_t index = 0; index < range_list.count; index++) {
    switch_object_id_t sw_object = {.data = range_list.list[index]};
    smi::attr_w sw_attr(SWITCH_ACL_RANGE_ATTR_TYPE);
    switch_enum_t range_type = {.enumdata = SWITCH_ACL_RANGE_ATTR_TYPE_MAX};

    if (range_list.list[index] == SAI_NULL_OBJECT_ID) {
      SAI_LOG_ERROR("Null ACL range object id specified");
      return SAI_STATUS_INVALID_OBJECT_ID;
    }

    sw_status =
        bf_switch_attribute_get(sw_object, SWITCH_ACL_RANGE_ATTR_TYPE, sw_attr);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get ACL range type, error: %s\n",
                    sai_metadata_get_status_name(status));
      return status;
    }
    sw_attr.v_get(range_type);

    switch (range_type.enumdata) {
      case SWITCH_ACL_RANGE_ATTR_TYPE_SRC_PORT:
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_SRC_PORT_RANGE_ID, sw_object));
        break;
      case SWITCH_ACL_RANGE_ATTR_TYPE_DST_PORT:
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DST_PORT_RANGE_ID, sw_object));
        break;
      default:
        SAI_LOG_ERROR("Failed to set sai ACL entry range 0x%" PRIx64
                      ". Range type %" PRIu64
                      "is not "
                      "supported.",
                      sw_object.data,
                      range_type.enumdata);
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Converts SAI ACL entry IP field to switch attribute.
 *
 * Arguments:
 *  [in] ip_field - SAI ACL entry IP field
 *  [in] is_src - flag signalyzing that this is source IP
 *  [in] is_ipv6 - flag signalyzing that this is IPv6 IP
 *  [out] sw_attrs - switch ACL entry attributes list
 *
 * Return Values:
 *    Void
 */
static void sai_acl_entry_ip_field_to_switch(
    _In_ sai_acl_field_data_t ip_field,
    _In_ bool is_src,
    _In_ bool is_ipv6,
    _Out_ std::set<smi::attr_w> &sw_attrs) {
  switch_ip_address_t ip = {};
  switch_ip_address_t mask = {};
  switch_attr_id_t ip_attr_id = SWITCH_ACL_ENTRY_ATTR_SRC_IP;
  switch_attr_id_t mask_attr_id = SWITCH_ACL_ENTRY_ATTR_SRC_IP_MASK;

  if (!is_src) {
    ip_attr_id = SWITCH_ACL_ENTRY_ATTR_DST_IP;
    mask_attr_id = SWITCH_ACL_ENTRY_ATTR_DST_IP_MASK;
  }

  if (is_ipv6) {
    sai_ipv6_to_switch_ip_addr(ip_field.data.ip6, ip);
    sai_ipv6_to_switch_ip_addr(ip_field.mask.ip6, mask);
  } else {
    sai_ipv4_to_switch_ip_addr(ip_field.data.ip4, ip);
    sai_ipv4_to_switch_ip_addr(ip_field.mask.ip4, mask);
  }

  sw_attrs.insert(smi::attr_w(ip_attr_id, ip));
  sw_attrs.insert(smi::attr_w(mask_attr_id, mask));
}

static void sai_acl_entry_ip6_to_word(_In_ sai_acl_field_data_t acl_field,
                                      _In_ switch_attr_id_t attr_id,
                                      _In_ switch_attr_id_t mask_id,
                                      _In_ int word_pos,
                                      _Out_ std::set<smi::attr_w> &sw_attrs) {
  uint32_t word;

  if (word_pos == 3) {
    memcpy(&word, &acl_field.data.ip6[0], sizeof(word));
    word = ntohl(word);
    sw_attrs.insert(smi::attr_w(attr_id, word));
    memcpy(&word, &acl_field.mask.ip6[0], sizeof(word));
    word = ntohl(word);
    sw_attrs.insert(smi::attr_w(mask_id, word));
  } else if (word_pos == 2) {
    memcpy(&word, &acl_field.data.ip6[4], sizeof(word));
    word = ntohl(word);
    sw_attrs.insert(smi::attr_w(attr_id, word));
    memcpy(&word, &acl_field.mask.ip6[4], sizeof(word));
    word = ntohl(word);
    sw_attrs.insert(smi::attr_w(mask_id, word));
  }
}

static sai_status_t sai_acl_entry_attr_to_switch_action_dtel_flow_op(
    _In_ const sai_attribute_t &attr, _Out_ std::set<smi::attr_w> &sw_attrs) {
  switch_enum_t sw_enum = {};
  switch (attr.value.aclaction.parameter.s32) {
    case SAI_ACL_DTEL_FLOW_OP_NOP:
      if (attr.value.aclaction.enable) {
        auto it = sw_attrs.find(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE));
        if (it != sw_attrs.end()) {
          uint8_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
          it->v_get(report_type);
          if (report_type == SWITCH_DTEL_REPORT_TYPE_NONE ||
              report_type == SWITCH_DTEL_REPORT_TYPE_DROP) {
            // In case it's already set just do nothing.
            break;
          } else {
            // We support only flow and drop report types set within one
            // action
            SAI_LOG_ERROR(
                "Too many dtel report types specified. Only FLOW and DROP "
                "can be simultaneously set for the same action.");
            return SAI_STATUS_NOT_SUPPORTED;
          }
        } else {
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                          static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_NONE)));
        }
        it = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE));
        if (it != sw_attrs.end()) {
          switch_enum_t sw_action = {
              .enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_NOP};
          it->v_get(sw_action);
          if (sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE ||
              sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT) {
            sw_enum.enumdata =
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT;
          } else {
            sw_enum.enumdata =
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
          }
          sw_attrs.erase(it);
        } else {
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
        }
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, sw_enum));
      }
      break;
    case SAI_ACL_DTEL_FLOW_OP_INT:
      break;
    case SAI_ACL_DTEL_FLOW_OP_POSTCARD:
      if (attr.value.aclaction.enable) {
        auto it = sw_attrs.find(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE));
        if (it != sw_attrs.end()) {
          uint8_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
          it->v_get(report_type);
          if (report_type == SWITCH_DTEL_REPORT_TYPE_NONE) {
            sw_attrs.erase(it);
            sw_attrs.insert(smi::attr_w(
                SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_FLOW)));
          } else if (report_type == SWITCH_DTEL_REPORT_TYPE_DROP) {
            sw_attrs.erase(it);
            sw_attrs.insert(smi::attr_w(
                SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_FLOW |
                                     SWITCH_DTEL_REPORT_TYPE_DROP)));
          } else {
            // We support only flow and drop report types set within one
            // action
            SAI_LOG_ERROR(
                "Too many dtel report types specified. Only FLOW and DROP "
                "can be simultaneously set for the same action.");
            return SAI_STATUS_NOT_SUPPORTED;
          }
        } else {
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                          static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_FLOW)));
        }
        it = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE));
        if (it != sw_attrs.end()) {
          switch_enum_t sw_action = {
              .enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_NOP};
          it->v_get(sw_action);
          if (sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE ||
              sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT) {
            sw_enum.enumdata =
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT;
          } else {
            sw_enum.enumdata =
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
          }
          sw_attrs.erase(it);
        } else {
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
        }
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, sw_enum));
      }
      break;
    default:
      SAI_LOG_WARN("Unknown DTEL flow operation %d.",
                   attr.value.aclaction.parameter.s32);
      break;
  }
  return SAI_STATUS_SUCCESS;
}

static sai_status_t sai_acl_entry_attr_to_switch_action_dtel_drop_report_ena(
    _In_ const sai_attribute_t &attr, _Out_ std::set<smi::attr_w> &sw_attrs) {
  switch_enum_t sw_enum = {};
  if (!attr.value.aclaction.enable) {
    return SAI_STATUS_SUCCESS;
  }

  auto it =
      sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE));
  if (it != sw_attrs.end()) {
    uint8_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    it->v_get(report_type);
    if (attr.value.aclaction.parameter.booldata) {
      if (report_type & SWITCH_DTEL_REPORT_TYPE_DROP) {
        // Drop report can be set twice for drop and tail_drop cases
        // So, in case it's already set just do nothing.
        return SAI_STATUS_SUCCESS;
      } else if (report_type == SWITCH_DTEL_REPORT_TYPE_NONE) {
        sw_attrs.erase(it);
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                        static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_DROP)));
        return SAI_STATUS_SUCCESS;
      } else if (report_type == SWITCH_DTEL_REPORT_TYPE_FLOW) {
        sw_attrs.erase(it);
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                        static_cast<uint8_t>(SWITCH_DTEL_REPORT_TYPE_FLOW |
                                             SWITCH_DTEL_REPORT_TYPE_DROP)));
        return SAI_STATUS_SUCCESS;
      }
    } else {
      // booldata is false - install entry and not generate the report
      if (report_type == SWITCH_DTEL_REPORT_TYPE_NONE ||
          report_type == SWITCH_DTEL_REPORT_TYPE_FLOW) {
        // None report can be set for drop, tail_drop (when booldata is
        // false) and flow nop cases
        // So, in case it's already set just do nothing.
        return SAI_STATUS_SUCCESS;
      }
    }

    // We support only flow and drop report types set within one action
    SAI_LOG_ERROR(
        "Too many dtel report types specified. Only FLOW and DROP can "
        "be simultaneously set for the same action.");
    return SAI_STATUS_NOT_SUPPORTED;
  } else {
    uint8_t report_type = (attr.value.aclaction.parameter.booldata)
                              ? SWITCH_DTEL_REPORT_TYPE_DROP
                              : SWITCH_DTEL_REPORT_TYPE_NONE;
    sw_attrs.insert(
        smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE, report_type));
  }
  it = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE));
  if (it != sw_attrs.end()) {
    switch_enum_t sw_action = {.enumdata =
                                   SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_NOP};
    it->v_get(sw_action);
    if (sw_action.enumdata ==
            SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE ||
        sw_action.enumdata ==
            SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT) {
      sw_enum.enumdata =
          SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT;
    } else {
      sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
    }
    sw_attrs.erase(it);
  } else {
    sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT;
  }
  sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, sw_enum));
  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Converts SAI ACL entry attribute to
 *   switch attributes
 *
 * Arguments:
 *  [in] attr - SAI ACL entry field attribute
 *  [out] sw_attrs - switch ACL entry attributes list
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl_entry_attr_to_switch(
    _In_ const sai_attribute_t &attr, _Out_ std::set<smi::attr_w> &sw_attrs) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_enum_t sw_enum = {};

  SAI_LOG_DEBUG("Acl Entry attr %s",
                sai_attribute_name(SAI_OBJECT_TYPE_ACL_ENTRY, attr.id));
  switch (attr.id) {
    case SAI_ACL_ENTRY_ATTR_PRIORITY: {
      uint32_t priority = sai_acl_priority_to_switch_priority(attr.value.u32);
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PRIORITY, priority));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6:
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP:
      sai_acl_entry_ip_field_to_switch(
          attr.value.aclfield,
          true,
          (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6),
          sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6_WORD3:
      sai_acl_entry_ip6_to_word(attr.value.aclfield,
                                SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD3,
                                SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD3,
                                3,  // pos
                                sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6_WORD2:
      sai_acl_entry_ip6_to_word(attr.value.aclfield,
                                SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD2,
                                SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD2,
                                2,  // pos
                                sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6:
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IP:
      sai_acl_entry_ip_field_to_switch(
          attr.value.aclfield,
          false,
          (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6),
          sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6_WORD3:
      sai_acl_entry_ip6_to_word(attr.value.aclfield,
                                SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD3,
                                SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD3,
                                3,  // pos
                                sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6_WORD2:
      sai_acl_entry_ip6_to_word(attr.value.aclfield,
                                SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD2,
                                SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD2,
                                2,  // pos
                                sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_MAC: {
      switch_mac_addr_t mac = {};
      memcpy(mac.mac, attr.value.aclfield.data.mac, sizeof(mac));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_SRC_MAC, mac));
      memcpy(mac.mac, attr.value.aclfield.mask.mac, sizeof(mac));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK, mac));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC: {
      switch_mac_addr_t mac = {};
      memcpy(mac.mac, attr.value.aclfield.data.mac, sizeof(mac));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DST_MAC, mac));
      memcpy(mac.mac, attr.value.aclfield.mask.mac, sizeof(mac));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK, mac));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT: {
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT,
                                  attr.value.aclfield.data.u16));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT_MASK,
                                  attr.value.aclfield.mask.u16));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT: {
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT,
                                  attr.value.aclfield.data.u16));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT_MASK,
                                  attr.value.aclfield.mask.u16));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE,
                                  attr.value.aclfield.data.u16));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK,
                                  attr.value.aclfield.mask.u16));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL:
    case SAI_ACL_ENTRY_ATTR_FIELD_IPV6_NEXT_HEADER:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_PROTO,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_PROTO_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DSCP:
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_DSCP,
                      (uint8_t)(attr.value.aclfield.data.u8 & 0x3F)));
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_DSCP_MASK,
                      (uint8_t)(attr.value.aclfield.mask.u8 & 0x3F)));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ECN:
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_ECN,
                      (uint8_t)(attr.value.aclfield.data.u8 & 0x3)));
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_ECN_MASK,
                      (uint8_t)(attr.value.aclfield.mask.u8 & 0x3)));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TTL:
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TTL, attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TTL_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_FRAG: {
      switch_enum_t frag = {
          .enumdata = static_cast<uint64_t>(attr.value.aclfield.data.s32)};
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_FRAG, frag));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMP_CODE,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMP_CODE_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE:
      switch ((sai_acl_ip_type_t)attr.value.aclfield.data.s32) {
        case SAI_ACL_IP_TYPE_IPV4ANY:
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, (uint16_t)0x0800));
          sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK,
                                      (uint16_t)0xFFFF));
          break;
        case SAI_ACL_IP_TYPE_IPV6ANY:
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE, (uint16_t)0x86DD));
          sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK,
                                      (uint16_t)0xFFFF));
          break;
        case SAI_ACL_IP_TYPE_ANY:
          break;
        case SAI_ACL_IP_TYPE_IP:
          /* ignored */
          break;
        default:
          SAI_LOG_ERROR(
              "Unsupported SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE value: %d",
              attr.value.aclfield.data.s32);
          status = SAI_STATUS_NOT_SUPPORTED;
          break;
      }
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_RANGE_TYPE:
      status = sai_acl_entry_range_list_to_switch(
          attr.value.aclfield.data.objlist, sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TC:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID,
                                  attr.value.aclfield.data.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS: {
      std::vector<switch_object_id_t> port_handle_list;
      if (attr.value.aclfield.data.objlist.count == 0) {
        SAI_LOG_ERROR("In ports list is empty");
        return SAI_STATUS_INVALID_PARAMETER;
      }
      for (unsigned i = 0; i < attr.value.aclfield.data.objlist.count; i++) {
        switch_object_id_t oid = {.data =
                                      attr.value.aclfield.data.objlist.list[i]};
        port_handle_list.push_back(oid);
      }
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IN_PORTS, port_handle_list));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS: {
      std::vector<switch_object_id_t> port_handle_list;
      if (attr.value.aclfield.data.objlist.count == 0) {
        SAI_LOG_ERROR("Out ports list is empty");
        return SAI_STATUS_INVALID_PARAMETER;
      }
      for (unsigned i = 0; i < attr.value.aclfield.data.objlist.count; i++) {
        switch_object_id_t oid = {.data =
                                      attr.value.aclfield.data.objlist.list[i]};
        port_handle_list.push_back(oid);
      }
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_OUT_PORTS, port_handle_list));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_INNER_VLAN_ID:  // Unsupported
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN,
                                  attr.value.aclfield.data.u16));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN_MASK,
                                  attr.value.aclfield.mask.u16));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_PRI:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_PRI,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_PRI_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_CFI:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_CFI,
                                  attr.value.aclfield.data.u8));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_OUTER_VLAN_CFI_MASK,
                                  attr.value.aclfield.mask.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_IN_PORT: {
      switch_object_id_t in_port_id = {.data = attr.value.aclfield.data.oid};
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INGRESS_PORT_LAG_HANDLE,
                                  in_port_id));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT: {
      switch_object_id_t out_port_id = {.data = attr.value.aclfield.data.oid};
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_EGRESS_PORT_LAG_HANDLE,
                                  out_port_id));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_CUSTOM_0: {
      switch_object_id_t ingress_rif = {.data = attr.value.aclfield.data.oid};
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INGRESS_RIF, ingress_rif));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_CUSTOM_1: {
      switch_object_id_t egress_rif = {.data = attr.value.aclfield.data.oid};
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_EGRESS_RIF, egress_rif));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT: {
      sai_object_type_t ot =
          sai_object_type_query(attr.value.aclaction.parameter.oid);
      switch_object_id_t oid = {.data = attr.value.aclaction.parameter.oid};
      if (ot == SAI_OBJECT_TYPE_PORT || ot == SAI_OBJECT_TYPE_LAG ||
          ot == SAI_OBJECT_TYPE_NEXT_HOP || ot == SAI_OBJECT_TYPE_NULL ||
          ot == SAI_OBJECT_TYPE_NEXT_HOP_GROUP) {
        sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_REDIRECT, oid));
      } else {
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR(
            "ACL action redirect unsupported for parameter %s:0x%" PRIx64 "\n",
            sai_metadata_get_object_type_name(ot),
            attr.value.aclaction.parameter.oid);
        return status;
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION: {
      sai_packet_action_t packet_action =
          static_cast<sai_packet_action_t>(attr.value.aclaction.parameter.s32);
      switch (packet_action) {
        case SAI_PACKET_ACTION_DROP:
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        case SAI_PACKET_ACTION_DENY:
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DENY;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        case SAI_PACKET_ACTION_TRANSIT:
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_TRANSIT;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        case SAI_PACKET_ACTION_TRAP:
          sw_enum.enumdata =
              SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        case SAI_PACKET_ACTION_COPY:
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        case SAI_PACKET_ACTION_FORWARD:
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT;
          sw_attrs.insert(
              smi::attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_enum));
          break;
        default:
          SAI_LOG_ERROR("Unsupported ACL entry packet action: %s",
                        sai_metadata_get_packet_action_name(packet_action));
          break;
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ROUTE_DST_USER_META: {
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_FIB_LABEL,
                                  attr.value.aclfield.data.u32));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_FIB_LABEL_MASK,
                                  attr.value.aclfield.mask.u32));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA: {
      uint32_t metadata = attr.value.aclaction.parameter.u32;
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA, metadata));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_FLOOD:  // Unsupported
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS: {
      if (attr.value.aclaction.parameter.objlist.count > 0) {
        switch_object_id_t session = {
            .data = attr.value.aclaction.parameter.objlist.list[0]};
        sw_attrs.insert(smi::attr_w(
            SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE, session));
        if (attr.value.aclaction.parameter.objlist.count > 1) {
          SAI_LOG_DEBUG("Only one mirror handle action is supported, given %d",
                        attr.value.aclaction.parameter.objlist.count);
          return SAI_STATUS_NOT_SUPPORTED;
        }
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS: {
      if (attr.value.aclaction.parameter.objlist.count > 0) {
        switch_object_id_t session = {
            .data = attr.value.aclaction.parameter.objlist.list[0]};
        sw_attrs.insert(smi::attr_w(
            SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE, session));
        if (attr.value.aclaction.parameter.objlist.count > 1) {
          SAI_LOG_DEBUG("Only one mirror handle action is supported, given %d",
                        attr.value.aclaction.parameter.objlist.count);
          return SAI_STATUS_NOT_SUPPORTED;
        }
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER: {
      switch_object_id_t meter = {.data = attr.value.aclaction.parameter.oid};
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE, meter));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_COUNTER: {
      switch_object_id_t counter = {.data = attr.value.aclaction.parameter.oid};
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE, counter));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_PACKET_COLOR: {
      switch_acl_entry_attr_action_set_color color =
          SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_GREEN;
      sai_color_to_switch(
          (sai_packet_color_t)attr.value.aclaction.parameter.s32, color);
      sw_enum.enumdata = color;
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR, sw_enum));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC,
                                  attr.value.aclaction.parameter.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_DSCP,
                                  attr.value.aclaction.parameter.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_ECN:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_ECN,
                                  attr.value.aclaction.parameter.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI:
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI,
                      attr.value.aclaction.parameter.u8));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_ACL_DTEL_FLOW_OP:
      status = sai_acl_entry_attr_to_switch_action_dtel_flow_op(attr, sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_DROP_REPORT_ENABLE:
    case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_TAIL_DROP_REPORT_ENABLE:
      status = sai_acl_entry_attr_to_switch_action_dtel_drop_report_ena(
          attr, sw_attrs);
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_REPORT_ALL_PACKETS:
      if (attr.value.aclaction.enable) {
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_ALL_PACKETS,
                        attr.value.aclaction.parameter.booldata));
      }
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_INT_SESSION:          // Unsupported
    case SAI_ACL_ENTRY_ATTR_ACTION_DTEL_FLOW_SAMPLE_PERCENT:  // Unsupported
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_INGRESS_SAMPLEPACKET_ENABLE: {
      if (attr.value.aclaction.enable) {
        switch_object_id_t session = {.data =
                                          attr.value.aclaction.parameter.oid};
        auto it =
            sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE));
        if (it != sw_attrs.end()) {
          switch_enum_t sw_action = {
              .enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_NOP};
          it->v_get(sw_action);
          if (sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_DTEL_REPORT ||
              sw_action.enumdata ==
                  SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT) {
            sw_enum.enumdata =
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT;
          } else {
            sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE;
          }
          sw_attrs.erase(it);
        } else {
          sw_enum.enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE;
        }
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, sw_enum));
        sw_attrs.insert(
            smi::attr_w(SWITCH_ACL_ENTRY_ATTR_SAMPLE_SESSION_HANDLE, session));
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_USER_META:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_USER_METADATA,
                                  (uint32_t)(attr.value.aclfield.data.u32)));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_USER_METADATA_MASK,
                                  (uint32_t)(attr.value.aclfield.mask.u32)));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_NO_NAT:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_DISABLE_NAT,
                                  attr.value.aclaction.parameter.booldata));
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_USER_TRAP_ID: {
      switch_object_id_t trap_id = {.data = attr.value.aclaction.parameter.oid};
      sw_attrs.insert(smi::attr_w(
          SWITCH_ACL_ENTRY_ATTR_ACTION_HOSTIF_USER_DEFINED_TRAP_HANDLE,
          trap_id));
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_VRF: {
      switch_object_id_t vrf_id = {.data = attr.value.aclaction.parameter.oid};
      sw_attrs.insert(
          smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_VRF_HANDLE, vrf_id));
    } break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TUNNEL_VNI:
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TUNNEL_VNI,
                                  attr.value.aclfield.data.u32));
      sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TUNNEL_VNI_MASK,
                                  attr.value.aclfield.mask.u32));
      break;
    default:
      status =
          sai_to_switch_attribute(SAI_OBJECT_TYPE_ACL_ENTRY, &attr, sw_attrs);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to convert SAI acl_entry attribute to switch: %s",
                      sai_metadata_get_status_name(status));
      }
      break;
  }

  return status;
}

/*
 * Routine Description:
 *   Gets the switch attribute and its mask
 *   and converts them to SAI ACL entry field.
 *
 * Arguments:
 *  [in] entry_id - id ow switch ACL entry attribute
 *  [in] attr_id - id of switch attribute
 *  [in] mask_id - id of switch mask attribute
 *  [in] value_type - type of SAI ACL fieal value
 *  [out] aclfield - SAI ACL field
 *
 * Return Values:
 *    SWITCH_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static switch_status_t sai_acl_entry_get_field(
    _In_ switch_object_id_t entry_id,
    _In_ switch_attr_id_t attr_id,
    _In_ switch_attr_id_t mask_id,
    _In_ sai_attr_value_type_t value_type,
    _Out_ sai_acl_field_data_t &aclfield) {
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  smi::attr_w sw_attr(attr_id);
  smi::attr_w sw_attr_mask(mask_id);

  sw_status = bf_switch_attribute_get(entry_id, attr_id, sw_attr);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return sw_status;
  }

  sw_status = bf_switch_attribute_get(entry_id, mask_id, sw_attr_mask);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return sw_status;
  }

  switch (value_type) {
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT8:
      sw_attr.v_get(aclfield.data.u8);
      sw_attr_mask.v_get(aclfield.mask.u8);
      break;
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT16:
      sw_attr.v_get(aclfield.data.u16);
      sw_attr_mask.v_get(aclfield.mask.u16);
      break;
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_MAC: {
      switch_mac_addr_t mac = {};
      switch_mac_addr_t mask = {};
      sw_attr.v_get(mac);
      sw_attr_mask.v_get(mask);
      memcpy(aclfield.data.mac, mac.mac, sizeof(aclfield.data.mac));
      memcpy(aclfield.mask.mac, mask.mac, sizeof(aclfield.mask.mac));
      break;
    }
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV4: {
      switch_ip_address_t ip = {};
      switch_ip_address_t mask = {};
      sw_attr.v_get(ip);
      sw_attr_mask.v_get(mask);
      aclfield.data.ip4 = htonl(ip.ip4);
      aclfield.mask.ip4 = htonl(mask.ip4);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV6: {
      switch_ip_address_t ip = {};
      switch_ip_address_t mask = {};
      sw_attr.v_get(ip);
      sw_attr_mask.v_get(mask);
      memcpy(aclfield.data.ip6, ip.ip6, IPV6_ADDR_LEN);
      memcpy(aclfield.mask.ip6, mask.ip6, IPV6_ADDR_LEN);
      break;
    }

    default:
      return SWITCH_STATUS_NOT_SUPPORTED;
  }

  return SWITCH_STATUS_SUCCESS;
}

static switch_status_t sai_acl_entry_word_to_ip6(
    _In_ switch_object_id_t entry_id,
    _In_ switch_attr_id_t attr_id,
    _In_ switch_attr_id_t mask_id,
    _In_ int word_pos,
    _Out_ sai_acl_field_data_t &aclfield) {
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  smi::attr_w sw_attr(attr_id);
  smi::attr_w sw_attr_mask(mask_id);
  uint32_t word;

  sw_status = bf_switch_attribute_get(entry_id, attr_id, sw_attr);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return sw_status;
  }

  sw_status = bf_switch_attribute_get(entry_id, mask_id, sw_attr_mask);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return sw_status;
  }

  if (word_pos == 3) {
    sw_attr.v_get(word);
    word = htonl(word);
    memcpy(&aclfield.data.ip6[0], &word, sizeof(word));
    sw_attr_mask.v_get(word);
    word = htonl(word);
    memcpy(&aclfield.mask.ip6[0], &word, sizeof(word));
  } else if (word_pos == 2) {
    sw_attr.v_get(word);
    word = htonl(word);
    memcpy(&aclfield.data.ip6[4], &word, sizeof(word));
    sw_attr_mask.v_get(word);
    word = htonl(word);
    memcpy(&aclfield.mask.ip6[4], &word, sizeof(word));
  }

  return SWITCH_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get ACL entry attribute
 *
 * Arguments:
 *    [in] acl_entry_id - the acl entry id
 *    [inout] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_acl_entry_get_attr(_In_ sai_object_id_t acl_entry_id,
                                           _Inout_ sai_attribute_t &attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = acl_entry_id};
  const auto attr_md =
      sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_ACL_ENTRY, attr.id);
  if (!attr_md) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Invalid attribute ID: %d, %s",
                  attr.id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr.id) {
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6:
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_SRC_IP,
                                          SWITCH_ACL_ENTRY_ATTR_SRC_IP_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6_WORD3:
      sw_status =
          sai_acl_entry_word_to_ip6(sw_object_id,
                                    SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD3,
                                    SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD3,
                                    3,  // pos
                                    attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6_WORD2:
      sw_status =
          sai_acl_entry_word_to_ip6(sw_object_id,
                                    SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_WORD2,
                                    SWITCH_ACL_ENTRY_ATTR_SRC_IPV6_MASK_WORD2,
                                    2,  // pos
                                    attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6:
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IP:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_DST_IP,
                                          SWITCH_ACL_ENTRY_ATTR_DST_IP_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6_WORD3:
      sw_status =
          sai_acl_entry_word_to_ip6(sw_object_id,
                                    SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD3,
                                    SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD3,
                                    3,  // pos
                                    attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6_WORD2:
      sw_status =
          sai_acl_entry_word_to_ip6(sw_object_id,
                                    SWITCH_ACL_ENTRY_ATTR_DST_IPV6_WORD2,
                                    SWITCH_ACL_ENTRY_ATTR_DST_IPV6_MASK_WORD2,
                                    2,  // pos
                                    attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_MAC:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_SRC_MAC,
                                          SWITCH_ACL_ENTRY_ATTR_SRC_MAC_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_DST_MAC,
                                          SWITCH_ACL_ENTRY_ATTR_DST_MAC_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT: {  // Unsupported
      break;
    }
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT: {
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT,
                                  SWITCH_ACL_ENTRY_ATTR_L4_SRC_PORT_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
      break;
    }
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT: {
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT,
                                  SWITCH_ACL_ENTRY_ATTR_L4_DST_PORT_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
      break;
    }
    case SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_ETH_TYPE,
                                          SWITCH_ACL_ENTRY_ATTR_ETH_TYPE_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL:
    case SAI_ACL_ENTRY_ATTR_FIELD_IPV6_NEXT_HEADER:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_IP_PROTO,
                                          SWITCH_ACL_ENTRY_ATTR_IP_PROTO_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DSCP:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_IP_DSCP,
                                          SWITCH_ACL_ENTRY_ATTR_IP_DSCP_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ECN:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_IP_ECN,
                                          SWITCH_ACL_ENTRY_ATTR_IP_ECN_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TTL:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_TTL,
                                          SWITCH_ACL_ENTRY_ATTR_TTL_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE,
                                          SWITCH_ACL_ENTRY_ATTR_ICMP_TYPE_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_ICMP_CODE,
                                          SWITCH_ACL_ENTRY_ATTR_ICMP_CODE_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE:
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE,
                                  SWITCH_ACL_ENTRY_ATTR_ICMPV6_TYPE_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE:
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE,
                                  SWITCH_ACL_ENTRY_ATTR_ICMPV6_CODE_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS:
      sw_status = sai_acl_entry_get_field(sw_object_id,
                                          SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS,
                                          SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS_MASK,
                                          attr_md->attrvaluetype,
                                          attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_RANGE_TYPE:  // Unsupported
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TC: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID, sw_attr);
      if (sw_status != SWITCH_STATUS_SUCCESS) {
        return sw_status;
      }
      sw_attr.v_get(attr.value.aclfield.data.u8);
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_REDIRECT);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_REDIRECT, sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_object_id_t oid = {};
        sw_attr.v_get(oid);
        attr.value.aclaction.parameter.oid = oid.data;
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_enum_t sw_action = {
            .enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT};
        sw_attr.v_get(sw_action);
        if (sw_action.enumdata == SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP) {
          attr.value.aclaction.parameter.s32 = SAI_PACKET_ACTION_DROP;
        } else if (sw_action.enumdata ==
                   SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT) {
          attr.value.aclaction.parameter.s32 = SAI_PACKET_ACTION_FORWARD;
        } else if (sw_action.enumdata ==
                   SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DENY) {
          attr.value.aclaction.parameter.s32 = SAI_PACKET_ACTION_DENY;
        } else if (sw_action.enumdata ==
                   SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_TRANSIT) {
          attr.value.aclaction.parameter.s32 = SAI_PACKET_ACTION_TRANSIT;
        }
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_FLOOD:  // Unsupported
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE);
      sw_status = bf_switch_attribute_get(
          sw_object_id,
          SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE,
          sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_object_id_t session = {};
        sw_attr.v_get(session);
        TRY_LIST_SET(attr.value.aclaction.parameter.objlist,
                     std::vector<sai_object_id_t>{session.data});
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE);
      sw_status = bf_switch_attribute_get(
          sw_object_id,
          SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE,
          sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_object_id_t session = {};
        sw_attr.v_get(session);
        TRY_LIST_SET(attr.value.aclaction.parameter.objlist,
                     std::vector<sai_object_id_t>{session.data});
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE, sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_object_id_t meter = {};
        sw_attr.v_get(meter);
        attr.value.aclaction.parameter.oid = meter.data;
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_COUNTER: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE, sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_object_id_t counter = {};
        sw_attr.v_get(counter);
        attr.value.aclaction.parameter.oid = counter.data;
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_PACKET_COLOR: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_enum_t sw_color = {
            .enumdata = SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR_GREEN};
        sai_packet_color_t sai_color = SAI_PACKET_COLOR_GREEN;

        sw_attr.v_get(sw_color);
        sai_switch_color_to_sai(
            (switch_acl_entry_attr_action_set_color)sw_color.enumdata,
            sai_color);
        attr.value.aclaction.parameter.s32 = sai_color;
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        sw_attr.v_get(attr.value.aclaction.parameter.u8);
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI);

      sw_status = bf_switch_attribute_get(
          sw_object_id,
          SWITCH_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_PRI,
          sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        sw_attr.v_get(attr.value.aclaction.parameter.u8);
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_DSCP);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_DSCP, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        sw_attr.v_get(attr.value.aclaction.parameter.u8);
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_ECN: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_ECN);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_ECN, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        sw_attr.v_get(attr.value.aclaction.parameter.u8);
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_USER_META: {
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_USER_METADATA,
                                  SWITCH_ACL_ENTRY_ATTR_USER_METADATA_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA);

      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA, sw_attr);

      if (sw_status == SWITCH_STATUS_SUCCESS) {
        sw_attr.v_get(attr.value.aclaction.parameter.u32);
      }
    } break;
    case SAI_ACL_ENTRY_ATTR_ACTION_INGRESS_SAMPLEPACKET_ENABLE: {
      attr.value.aclaction.parameter.oid = SAI_NULL_OBJECT_ID;
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE, sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        switch_enum_t sw_action = {
            .enumdata = SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_NOP};
        sw_attr.v_get(sw_action);

        if (sw_action.enumdata ==
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE ||
            sw_action.enumdata ==
                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE_IFA_CLONE_AND_DTEL_REPORT) {
          smi::attr_w sw_attr_sess(SWITCH_ACL_ENTRY_ATTR_SAMPLE_SESSION_HANDLE);
          sw_status = bf_switch_attribute_get(
              sw_object_id,
              SWITCH_ACL_ENTRY_ATTR_SAMPLE_SESSION_HANDLE,
              sw_attr_sess);
          if (sw_status == SWITCH_STATUS_SUCCESS) {
            switch_object_id_t session = {};
            sw_attr_sess.v_get(session);
            attr.value.aclaction.parameter.oid = session.data;
          }
        }
      }
      break;
    }
    case SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID:
      sw_status =
          sai_acl_entry_get_field(sw_object_id,
                                  SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN,
                                  SWITCH_ACL_ENTRY_ATTR_INTERNAL_VLAN_MASK,
                                  attr_md->attrvaluetype,
                                  attr.value.aclfield);
      break;
    case SAI_ACL_ENTRY_ATTR_PRIORITY: {
      smi::attr_w sw_attr(SWITCH_ACL_ENTRY_ATTR_PRIORITY);
      sw_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_ACL_ENTRY_ATTR_PRIORITY, sw_attr);
      if (sw_status == SWITCH_STATUS_SUCCESS) {
        uint32_t priority;
        sw_attr.v_get(priority);
        attr.value.u32 = switch_priority_to_sai_acl_priority(priority);
      }
      break;
    }
    default:
      status = sai_to_switch_attribute_get(
          SAI_OBJECT_TYPE_ACL_ENTRY, sw_object_id, &attr);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                      sai_attribute_name(SAI_OBJECT_TYPE_ACL_ENTRY, attr.id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                  sai_attribute_name(SAI_OBJECT_TYPE_ACL_ENTRY, attr.id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  return status;
}

/*
 * Routine Description:
 *   Create an ACL table group
 *
 * Arguments:
 *  [out] acl_group_id - the the acl table group id
 *  [in] switch_id  Switch Object id
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_acl_group(_Out_ sai_object_id_t *acl_group_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t acl_object_id = {};

  if (!attr_list || !acl_group_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_group_id = SAI_NULL_OBJECT_ID;

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_STAGE:  // Unsupported
        // No such attribute in SMI so far.
        break;
      case SAI_ACL_TABLE_GROUP_ATTR_TYPE:
        if (attr_list[index].value.s32 == SAI_ACL_TABLE_GROUP_TYPE_SEQUENTIAL) {
          return SAI_STATUS_NOT_SUPPORTED;
        }
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ACL_TABLE_GROUP, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to create ACL group - the conversion of attribute %s "
              "failed, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE_GROUP,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_ACL_GROUP_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_GROUP, sw_attrs, acl_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL table group: %s",
                  sai_metadata_get_status_name(status));
  }
  *acl_group_id = acl_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *  Delete an ACL table group
 *
 * Arguments:
 *  [out] acl_group_id - the the acl table group id
 *
 * Return Values:
 *  SAI_STATUS_SUCCESS on success
 *  Failure status code on error
 */
sai_status_t sai_remove_acl_group(_In_ sai_object_id_t acl_group_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(acl_group_id) != SAI_OBJECT_TYPE_ACL_TABLE_GROUP) {
    SAI_LOG_ERROR(
        "ACL table group remove failed: invalid ACL table group handle "
        "0x%" PRIx64 "\n",
        acl_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_group_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove ACL table group 0x%" PRIx64 ": %s",
                  acl_group_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *  Set ACL tabl group attribute
 *
 * Arguments:
 *  [in] acl_group_id - the acl table group id
 *  [in] attr - attribute
 *
 * Return Values:
 *  SAI_STATUS_SUCCESS on success
 *  Failure status code on error
 */
sai_status_t sai_set_acl_table_group_attribute(
    _In_ sai_object_id_t acl_group_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_group_id) != SAI_OBJECT_TYPE_ACL_TABLE_GROUP) {
    SAI_LOG_ERROR(
        "ACL table group set failed: invalid ACL table group handle 0x%" PRIx64
        "\n",
        acl_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_group_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ACL_TABLE_GROUP, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set attribute %s error: %s",
            sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE_GROUP, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}
/*
 * Routine Description:
 *  Get ACL table group attribute
 *
 * Arguments:
 *  [in] acl_group_id - acl table group id
 *  [in] attr_count - number of attributes
 *  [out] attr_list - array of attributes
 *
 * Return Values:
 *  SAI_STATUS_SUCCESS on success
 *  Failure status code on error
 */
sai_status_t sai_get_acl_table_group_attribute(
    _In_ sai_object_id_t acl_group_id,
    _In_ uint32_t attr_count,
    _Out_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_group_id) != SAI_OBJECT_TYPE_ACL_TABLE_GROUP) {
    SAI_LOG_ERROR(
        "ACL table group get failed: invalid ACL table group handle 0x%" PRIx64
        "\n",
        acl_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_group_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ACL_TABLE_GROUP, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE_GROUP,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

// clang-format off
static sai_status_t stage_to_direction(int32_t stage, uint64_t *dir) {
  if (stage == SAI_ACL_STAGE_EGRESS) {
    *dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS;
  } else if (stage == SAI_ACL_STAGE_INGRESS) {
    *dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS;
  } else if (stage == SAI_ACL_STAGE_PRE_INGRESS) {
    *dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_PREINGRESS;
  } else {
    SAI_LOG_ERROR("Unsupported stage: %" PRId32, stage);
    return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}
// clang-format on
/*
 * Routine Description:
 *   Create an ACL table
 *
 * Arguments:
 *  [out] acl_table_id - the the acl table id
 * [in] switch_id  Switch Object id
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_acl_table(_Out_ sai_object_id_t *acl_table_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_acl_table_attr_type acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t acl_object_id = {};

  if (!attr_list || !acl_table_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_table_id = SAI_NULL_OBJECT_ID;

  if (sai_acl_using_acl2_profile()) {
    status =
        sai_acl2_tbl_type_get_from_attributes(attr_count, attr_list, acl_type);
  } else {
    status =
        sai_acl_tbl_type_get_from_attributes(attr_count, attr_list, acl_type);
  }
  if (status != SAI_STATUS_SUCCESS) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Failed to find match table: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_enum_t sw_enum = {.enumdata = acl_type};
  sw_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_TYPE, sw_enum));

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_ACL_TABLE_ATTR_ACL_STAGE: {
        status |=
            stage_to_direction(attr_list[index].value.s32, &sw_enum.enumdata);
        sw_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_DIRECTION, sw_enum));
        break;
      }
      case SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0:
      case SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1:
      case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST:
        break;  // Ignore it
      default: {
        if (unsupported_attr_ids.find(attr_list[index].id) !=
            unsupported_attr_ids.end()) {
          // Handle unsupported attributes we silently ignore
          break;
        }

        if (attr_list[index].id >=
                SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MIN &&
            attr_list[index].id <=
                SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MAX) {
          // unsupported
          break;
        }

        // Check if the table field is present in a table pattern.
        // If it is let's not try to convert it to the switch
        // attribute as in switch we use table type instead of fields list.
        if ((attr_list[index].id >= SAI_ACL_TABLE_ATTR_FIELD_START) &&
            (attr_list[index].id <= SAI_ACL_TABLE_ATTR_FIELD_END)) {
          switch_acl_table_attr_type acl_search_type = acl_type;

          auto tbl_iter =
              std::find_if(sai_acl_tbl_patterns.begin(),
                           sai_acl_tbl_patterns.end(),
                           [&](const switch_acl_table_type_to_fields &element) {
                             return element.first == acl_search_type;
                           });
          if (tbl_iter != sai_acl_tbl_patterns.end()) {
            if (tbl_iter->second.find(attr_list[index].id) !=
                tbl_iter->second.end()) {
              break;  // Field is in the table quilifiers list - just ignore it
            } else {
              SAI_LOG_ERROR(
                  "Failed to create ACL table - the attribute %s is not "
                  "supported in pattern %d",
                  sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE,
                                     attr_list[index].id),
                  acl_search_type);
              return SAI_STATUS_NOT_SUPPORTED;
            }
          } else {
            SAI_LOG_ERROR(
                "Failed to create ACL table: could not find table pattern of "
                "type %d",
                acl_search_type);
            return SAI_STATUS_ITEM_NOT_FOUND;
          }
        }

        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ACL_TABLE, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to create ACL table - the conversion of attribute %s "
              "failed, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
      }
    }
  }

  sai_insert_device_attribute(0, SWITCH_ACL_TABLE_ATTR_DEVICE, sw_attrs);

  if (bf_switch_is_feature_enabled(SWITCH_FEATURE_INNER_DTEL) &&
      acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_DTEL) {
    // Create inner IPv4 DTEL ACL table
    std::set<smi::attr_w> ipv4_attrs(sw_attrs);
    std::set<smi::attr_w>::iterator ipv4_it = std::find_if(
        ipv4_attrs.begin(), ipv4_attrs.end(), [](const smi::attr_w &attr) {
          return attr.id_get() == SWITCH_ACL_TABLE_ATTR_TYPE;
        });
    if (ipv4_it != ipv4_attrs.end()) {
      ipv4_attrs.erase(ipv4_it);
    }
    switch_enum_t ipv4_type = {.enumdata =
                                   SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV4};
    ipv4_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_TYPE, ipv4_type));
    switch_object_id_t ipv4_object_id = {};
    switch_status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_ACL_TABLE, ipv4_attrs, ipv4_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create inner IPv4 ACL table: %s",
                    sai_metadata_get_status_name(status));
    }
    sw_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE,
                                ipv4_object_id));

    // Create inner IPv6 DTEL ACL table
    std::set<smi::attr_w> ipv6_attrs(sw_attrs);
    std::set<smi::attr_w>::iterator ipv6_it = std::find_if(
        ipv6_attrs.begin(), ipv6_attrs.end(), [](const smi::attr_w &attr) {
          return attr.id_get() == SWITCH_ACL_TABLE_ATTR_TYPE;
        });
    if (ipv6_it != ipv6_attrs.end()) {
      ipv6_attrs.erase(ipv6_it);
    }
    switch_enum_t ipv6_type = {.enumdata =
                                   SWITCH_ACL_TABLE_ATTR_TYPE_INNER_DTEL_IPV6};
    ipv6_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_TYPE, ipv6_type));
    switch_object_id_t ipv6_object_id = {};
    switch_status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_ACL_TABLE, ipv6_attrs, ipv6_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create inner IPv6 ACL table: %s",
                    sai_metadata_get_status_name(status));
    }
    sw_attrs.insert(smi::attr_w(SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE,
                                ipv6_object_id));
  }

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_TABLE, sw_attrs, acl_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL table: %s",
                  sai_metadata_get_status_name(status));
  }
  *acl_table_id = acl_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Delete an ACL table
 *
 * Arguments:
 *   [in] acl_table_id - the acl table id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_acl_table(_In_ sai_object_id_t acl_table_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = acl_table_id};

  if (sai_object_type_query(acl_table_id) != SAI_OBJECT_TYPE_ACL_TABLE) {
    SAI_LOG_ERROR("ACL table remove failed: invalid acl table handle 0x%" PRIx64
                  "\n",
                  acl_table_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  // Get inner IPv4 DTEL ACL table handle
  smi::attr_w ipv4_attr(SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE, ipv4_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv4 ACL table 0x%" PRIx64 ": %s",
                  sw_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv4_object_id = {};
  ipv4_attr.v_get(ipv4_object_id);

  // Get inner IPv6 DTEL ACL table handle
  smi::attr_w ipv6_attr(SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE, ipv6_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv6 ACL table 0x%" PRIx64 ": %s",
                  sw_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv6_object_id = {};
  ipv6_attr.v_get(ipv6_object_id);

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove acl table 0x%" PRIx64 ": %s",
                  acl_table_id,
                  sai_metadata_get_status_name(status));
  }

  // Remove inner IPv6 DTEL ACL table if exists
  if (ipv6_object_id.data != 0) {
    switch_status = bf_switch_object_delete(ipv6_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove inner IPv6 acl table 0x%" PRIx64 ": %s",
                    ipv6_object_id.data,
                    sai_metadata_get_status_name(status));
    }
  }

  // Remove inner IPv4 DTEL ACL table if exists
  if (ipv4_object_id.data != 0) {
    switch_status = bf_switch_object_delete(ipv4_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove inner IPv4 acl table 0x%" PRIx64 ": %s",
                    ipv4_object_id.data,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

sai_status_t sai_acl_table_size_get(switch_object_id_t object_id,
                                    uint32_t *table_size) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  smi::attr_w type_attr(SWITCH_ACL_TABLE_ATTR_TYPE),
      dir_attr(SWITCH_ACL_TABLE_ATTR_DIRECTION);
  switch_enum_t type = {0}, direction = {0};
  uint64_t table_id = 0;

  sw_status =
      bf_switch_attribute_get(object_id, SWITCH_ACL_TABLE_ATTR_TYPE, type_attr);
  type_attr.v_get(type);
  sw_status = bf_switch_attribute_get(
      object_id, SWITCH_ACL_TABLE_ATTR_DIRECTION, dir_attr);
  dir_attr.v_get(direction);

  switch (direction.enumdata) {
    case SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS:
      switch (type.enumdata) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          table_id = SWITCH_DEVICE_ATTR_TABLE_IPV4_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          table_id = SWITCH_DEVICE_ATTR_TABLE_IPV6_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
          table_id = SWITCH_DEVICE_ATTR_TABLE_MAC_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR:
          table_id = SWITCH_DEVICE_ATTR_TABLE_IP_MIRROR_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_ECN:
          table_id = SWITCH_DEVICE_ATTR_TABLE_ECN_ACL;
          break;
        default:
          *table_size = 0;
          return status;
      }
      break;
    case SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS:
      switch (type.enumdata) {
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV4:
          table_id = SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV4_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_IPV6:
          table_id = SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV6_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_MAC:
          table_id = SWITCH_DEVICE_ATTR_TABLE_EGRESS_MAC_ACL;
          break;
        case SWITCH_ACL_TABLE_ATTR_TYPE_SYSTEM:
          table_id = SWITCH_DEVICE_ATTR_TABLE_EGRESS_SYSTEM_ACL;
          break;
        default:
          *table_size = 0;
          return status;
      }
      break;
    default:
      *table_size = 0;
      return status;
  }

  switch_table_info_t table_info = {};
  sw_status = bf_switch_table_info_get(table_id, table_info);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get table size: %s table id: %" PRIx64 "\n",
                  sai_metadata_get_status_name(status),
                  table_id);
    return status;
  }
  *table_size += (table_info.table_size - table_info.table_usage);
  return status;
}

/*
 * Routine Description:
 *   Get ACL table attribute
 *
 * Arguments:
 *    [in] acl_table_id - the acl table id
 *    [in] attr_count - attribute count
 *    [inout] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_acl_table_attribute(_In_ sai_object_id_t acl_table_id,
                                         _In_ uint32_t attr_count,
                                         _Out_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_table_id) != SAI_OBJECT_TYPE_ACL_TABLE) {
    SAI_LOG_ERROR("ACL table get failed: invalid ACL table handle 0x%" PRIx64
                  "\n",
                  acl_table_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_table_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_ACL_TABLE_ATTR_AVAILABLE_ACL_ENTRY:
      case SAI_ACL_TABLE_ATTR_AVAILABLE_ACL_COUNTER: {
        uint32_t table_size = 0;
        status = sai_acl_table_size_get(sw_object_id, &table_size);
        attr_list[index].value.u32 = table_size;
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ACL_TABLE, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Create an ACL tabel group member
 *
 * Arguments:
 *   [out] acl_table_group_member_id - the acl table group member id
 *   [in] switch_id The Switch Object id
 *   [in] attr_count - number of attributes
 *   [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_acl_group_member(
    _Out_ sai_object_id_t *acl_table_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;
  switch_object_id_t group_id = {}, table_id = {};

  if (!acl_table_group_member_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_table_group_member_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t group_member_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_PRIORITY:
        break;  // SONiC uses this attribute so just ignore it as we don't
                // support this
      default:
        if (attr_list[index].id ==
            SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_ID) {
          table_id.data = (uint64_t)attr_list[index].value.oid;
        } else if (attr_list[index].id ==
                   SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_GROUP_ID) {
          group_id.data = (uint64_t)attr_list[index].value.oid;
        }

        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to create ACL group member - the conversion of attribute "
              "%s failed, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_ACL_GROUP_MEMBER_ATTR_DEVICE, sw_attrs);

  smi::attr_w grp_bp_attr(SWITCH_ACL_GROUP_ATTR_BIND_POINT_TYPE);
  std::vector<switch_enum_t> grp_bp_list;
  switch_status =
      bf_switch_attribute_get(group_id, grp_bp_attr.id_get(), grp_bp_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL group bind point type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  grp_bp_attr.v_get(grp_bp_list);

  smi::attr_w tbl_bp_attr(SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE);
  std::vector<switch_enum_t> tbl_bp_list;
  switch_status =
      bf_switch_attribute_get(table_id, tbl_bp_attr.id_get(), tbl_bp_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ACL table bind point type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  tbl_bp_attr.v_get(tbl_bp_list);

  /* Update ACL table with missing bind point types */
  size_t tbl_bp_list_size = tbl_bp_list.size();
  for (auto grp_bp_enum : grp_bp_list) {
    std::vector<switch_enum_t>::iterator it = tbl_bp_list.begin();
    for (; it != tbl_bp_list.end(); ++it) {
      if (grp_bp_enum.enumdata == it->enumdata) break;
    }
    if (it == tbl_bp_list.end()) {
      tbl_bp_list.push_back(grp_bp_enum);
    }
  }

  /* get number of entries in the acl table */
  smi::attr_w acl_tab_attr(SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES);
  std::vector<switch_object_id_t> acl_entry_list;

  switch_status = bf_switch_attribute_get(
      table_id, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, acl_tab_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Fail to get ACL table entries list, error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }
  acl_tab_attr.v_get(acl_entry_list);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, sw_attrs, group_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL group member: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  /* set bind point type only if the acl table has no entries */
  if (tbl_bp_list_size < tbl_bp_list.size()) {
    if (!acl_entry_list.size()) {
      tbl_bp_attr.v_set(tbl_bp_list);
      switch_status = switch_store::attribute_set(table_id, tbl_bp_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set ACL table bind point type: %s",
                      sai_metadata_get_status_name(status));
        bf_switch_object_delete(group_member_object_id);
        return status;
      }
    } else {
      SAI_LOG_ERROR(
          "Failed to set ACL table bind point type: Cannot change bind point "
          "type for non-empty ACL table");
    }
  }

  *acl_table_group_member_id = group_member_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Delete an ACL table group member
 *
 * Arguments:
 *   [in] acl_group_member_id - the acl table group member id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_acl_group_member(
    _In_ sai_object_id_t acl_group_member_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(acl_group_member_id) !=
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "ACL table group member remove failed: invalid acl table group member "
        "handle 0x%" PRIx64 "\n",
        acl_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_group_member_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove acl table group member 0x%" PRIx64 ": %s",
                  acl_group_member_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Set ACL table group member attribute
 *
 * Arguments:
 *    [in] acl_group_member_id - the acl table group member id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_NOT_SUPPORTED
 */
sai_status_t sai_set_acl_table_group_member_attribute(
    _In_ sai_object_id_t acl_group_member_id,
    _In_ const sai_attribute_t *attr) {
  return SAI_STATUS_NOT_SUPPORTED;
}

/*
 * Routine Description:
 *   Get ACL table group member attribute
 *
 * Arguments:
 *    [in] acl_group_member_id - the acl table group member id
 *    [in] attr_count - attribute count
 *    [inout] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_NOT_SUPPORTED
 */
sai_status_t sai_get_acl_table_group_member_attribute(
    _In_ sai_object_id_t acl_id,
    _In_ uint32_t attr_count,
    _Out_ sai_attribute_t *attr_list) {
  return SAI_STATUS_NOT_SUPPORTED;
}

/*
 * Routine Description:
 *   Create an ACL entry
 *
 * Arguments:
 *   [out] acl_entry_id - the acl entry id
 *   [in] switch_id The Switch Object id
 *   [in] attr_count - number of attributes
 *   [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_acl_entry(_Out_ sai_object_id_t *acl_entry_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t acl_object_id = {};

  if (!acl_entry_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_entry_id = SAI_NULL_OBJECT_ID;

  // During ACL table creation actions may be missing which means
  // that the incorrect table type can be chosen. Now when we have
  // the action specified in the entry we can update the table type
  // to the correct one.
  if (sai_acl_using_acl2_profile()) {
    status = sai_acl2_table_type_update(attr_count, attr_list);
  } else {
    status = sai_acl_table_type_update(attr_count, attr_list);
  }
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    status = sai_acl_entry_attr_to_switch(attr_list[index], sw_attrs);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to convert SAI ACL entry attribute %s to switch: %s",
          sai_attribute_name(SAI_OBJECT_TYPE_ACL_ENTRY, attr_list[index].id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  auto it3 = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TCP_FLAGS));
  auto it4 = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_PROTO));

  // Add IP_PROTO attribute if ACL entry is created with TCP_FLAGS parameters
  if (it3 != sw_attrs.end() && it4 == sw_attrs.end()) {
    sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_PROTO,
                                (uint8_t)0x06));  //  IP_PROTO_TCP
    sw_attrs.insert(
        smi::attr_w(SWITCH_ACL_ENTRY_ATTR_IP_PROTO_MASK, (uint8_t)0xFF));
  }

  switch_object_id_t table_handle;
  auto it5 = sw_attrs.find(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE));
  if (it5 == sw_attrs.end()) {
    SAI_LOG_ERROR(
        "Failed to create SAI ACL entry, "
        "absent mandatory attr: SAI_ACL_ENTRY_ATTR_TABLE_ID");
    return SAI_STATUS_INVALID_PARAMETER;
  }
  it5->v_get(table_handle);

  switch_enum_t table_type = {};
  smi::attr_w table_type_attr(SWITCH_ACL_TABLE_ATTR_TYPE);
  status = bf_switch_attribute_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type_attr);
  table_type_attr.v_get(table_type);

  sai_insert_device_attribute(0, SWITCH_ACL_ENTRY_ATTR_DEVICE, sw_attrs);

  // Check if match field is source/destination IPv4 or EtherType is IPv4
  bool is_ipv4 =
      std::find_if(
          attr_list, attr_list + attr_count, [](const sai_attribute_t &attr) {
            return (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP &&
                    attr.value.aclfield.enable) ||
                   (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_DST_IP &&
                    attr.value.aclfield.enable) ||
                   (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE &&
                    attr.value.aclfield.enable &&
                    attr.value.aclfield.data.u16 == 0x0800);
          }) != attr_list + attr_count;

  // Check if match field is source/destination IPv6 or EtherType is IPv6
  bool is_ipv6 =
      std::find_if(
          attr_list, attr_list + attr_count, [](const sai_attribute_t &attr) {
            return (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6 &&
                    attr.value.aclfield.enable) ||
                   (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6 &&
                    attr.value.aclfield.enable) ||
                   (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE &&
                    attr.value.aclfield.enable &&
                    attr.value.aclfield.data.u16 == 0x86DD);
          }) != attr_list + attr_count;

  // Check if EtherType is specified
  bool is_ether =
      std::find_if(
          attr_list, attr_list + attr_count, [](const sai_attribute_t &attr) {
            return (attr.id == SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE &&
                    attr.value.aclfield.enable);
          }) != attr_list + attr_count;

  // Check if inner IPv4 DTEL ACL table exists and entry should be created
  smi::attr_w ipv4_table_attr(SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE);
  switch_status =
      bf_switch_attribute_get(table_handle,
                              SWITCH_ACL_TABLE_ATTR_INNER_IPV4_TABLE_HANDLE,
                              ipv4_table_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv4 ACL table 0x%" PRIx64 ": %s",
                  table_handle.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv4_table_handle = {};
  ipv4_table_attr.v_get(ipv4_table_handle);
  if (ipv4_table_handle.data != 0 && (is_ipv4 || !(is_ipv6 || is_ether))) {
    // Create inner IPv4 DTEL ACL entry
    std::set<smi::attr_w> ipv4_attrs(sw_attrs);
    std::set<smi::attr_w>::iterator ipv4_it = std::find_if(
        ipv4_attrs.begin(), ipv4_attrs.end(), [](const smi::attr_w &attr) {
          return attr.id_get() == SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE;
        });
    if (ipv4_it != ipv4_attrs.end()) {
      ipv4_attrs.erase(ipv4_it);
    }
    ipv4_attrs.insert(
        smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, ipv4_table_handle));
    switch_object_id_t ipv4_object_id = {};
    switch_status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_ACL_ENTRY, ipv4_attrs, ipv4_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create inner IPv4 acl entry: %s",
                    sai_metadata_get_status_name(status));
    }
    sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INNER_IPV4_ENTRY_HANDLE,
                                ipv4_object_id));
  }

  // Check if inner IPv6 DTEL ACL table exists and entry should be created
  smi::attr_w ipv6_table_attr(SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE);
  switch_status =
      bf_switch_attribute_get(table_handle,
                              SWITCH_ACL_TABLE_ATTR_INNER_IPV6_TABLE_HANDLE,
                              ipv6_table_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv6 ACL table 0x%" PRIx64 ": %s",
                  table_handle.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv6_table_handle = {};
  ipv6_table_attr.v_get(ipv6_table_handle);
  if (ipv6_table_handle.data != 0 && (is_ipv6 || !(is_ipv4 || is_ether))) {
    // Create inner IPv6 DTEL ACL entry
    std::set<smi::attr_w> ipv6_attrs(sw_attrs);
    std::set<smi::attr_w>::iterator ipv6_it = std::find_if(
        ipv6_attrs.begin(), ipv6_attrs.end(), [](const smi::attr_w &attr) {
          return attr.id_get() == SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE;
        });
    if (ipv6_it != ipv6_attrs.end()) {
      ipv6_attrs.erase(ipv6_it);
    }
    ipv6_attrs.insert(
        smi::attr_w(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, ipv6_table_handle));
    switch_object_id_t ipv6_object_id = {};
    switch_status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_ACL_ENTRY, ipv6_attrs, ipv6_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create inner IPv6 acl entry: %s",
                    sai_metadata_get_status_name(status));
    }
    sw_attrs.insert(smi::attr_w(SWITCH_ACL_ENTRY_ATTR_INNER_IPV6_ENTRY_HANDLE,
                                ipv6_object_id));
  }

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_ENTRY, sw_attrs, acl_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL entry: %s",
                  sai_metadata_get_status_name(status));
  }

  *acl_entry_id = acl_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Delete an ACL entry
 *
 * Arguments:
 *  [in] acl_entry_id - the acl entry id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_acl_entry(_In_ sai_object_id_t acl_entry_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = acl_entry_id};

  if (sai_object_type_query(acl_entry_id) != SAI_OBJECT_TYPE_ACL_ENTRY) {
    SAI_LOG_ERROR("ACL entry remove failed: invalid acl entry handle 0x%" PRIx64
                  "\n",
                  acl_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  // Get inner IPv4 DTEL ACL entry handle
  smi::attr_w ipv4_attr(SWITCH_ACL_ENTRY_ATTR_INNER_IPV4_ENTRY_HANDLE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_ENTRY_ATTR_INNER_IPV4_ENTRY_HANDLE, ipv4_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv4 acl entry 0x%" PRIx64 ": %s",
                  sw_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv4_object_id = {};
  ipv4_attr.v_get(ipv4_object_id);

  // Get inner IPv6 DTEL ACL entry handle
  smi::attr_w ipv6_attr(SWITCH_ACL_ENTRY_ATTR_INNER_IPV6_ENTRY_HANDLE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_ACL_ENTRY_ATTR_INNER_IPV6_ENTRY_HANDLE, ipv6_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv6 acl entry 0x%" PRIx64 ": %s",
                  sw_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv6_object_id = {};
  ipv6_attr.v_get(ipv6_object_id);

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove acl entry 0x%" PRIx64 ": %s",
                  acl_entry_id,
                  sai_metadata_get_status_name(status));
  }

  // Remove inner IPv6 DTEL ACL entry if exists
  if (ipv6_object_id.data != 0) {
    switch_status = bf_switch_object_delete(ipv6_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove inner IPv6 acl entry 0x%" PRIx64 ": %s",
                    ipv6_object_id.data,
                    sai_metadata_get_status_name(status));
    }
  }

  // Remove inner IPv4 DTEL ACL entry if exists
  if (ipv4_object_id.data != 0) {
    switch_status = bf_switch_object_delete(ipv4_object_id);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove inner IPv4 acl entry 0x%" PRIx64 ": %s",
                    ipv4_object_id.data,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Set ACL entry attribute
 *
 * Arguments:
 *    [in] acl_entry_id - the acl entry id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_acl_entry_attribute(_In_ sai_object_id_t acl_entry_id,
                                         _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t acl_object_id = {.data = acl_entry_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_entry_id) != SAI_OBJECT_TYPE_ACL_ENTRY) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("ACL entry set failed: invalid acl entry handle 0x%" PRIx64
                  ": %s\n",
                  acl_entry_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (attr->id >= SAI_ACL_ENTRY_ATTR_FIELD_START &&
      attr->id <= SAI_ACL_ENTRY_ATTR_FIELD_END) {
    status = SAI_STATUS_NOT_SUPPORTED;
    SAI_LOG_ERROR("Modifying acl_entry field attributes is not supported %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  // For DTEL ACLs, insert previous report_type and action_type in
  // sw_attrs so that existing values of other DTEL ACL action attributes
  // are considered
  if (attr->id == SAI_ACL_ENTRY_ATTR_ACTION_ACL_DTEL_FLOW_OP ||
      attr->id == SAI_ACL_ENTRY_ATTR_ACTION_DTEL_DROP_REPORT_ENABLE ||
      attr->id == SAI_ACL_ENTRY_ATTR_ACTION_DTEL_TAIL_DROP_REPORT_ENABLE ||
      attr->id == SAI_ACL_ENTRY_ATTR_ACTION_INGRESS_SAMPLEPACKET_ENABLE) {
    smi::attr_w report_type_attr(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE);
    switch_status =
        bf_switch_attribute_get(acl_object_id,
                                SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE,
                                report_type_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get previous dtel_acl report_type 0x%" PRIx64
                    ": %s",
                    acl_entry_id,
                    sai_metadata_get_status_name(status));
    }
    // Clear bit corresponding to attr
    uint8_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    report_type_attr.v_get(report_type);
    if (attr->id == SAI_ACL_ENTRY_ATTR_ACTION_ACL_DTEL_FLOW_OP) {
      report_type &= ~(SWITCH_DTEL_REPORT_TYPE_FLOW);
    } else if (attr->id == SAI_ACL_ENTRY_ATTR_ACTION_DTEL_DROP_REPORT_ENABLE ||
               attr->id ==
                   SAI_ACL_ENTRY_ATTR_ACTION_DTEL_TAIL_DROP_REPORT_ENABLE) {
      report_type &= ~(SWITCH_DTEL_REPORT_TYPE_DROP);
    }
    sw_attrs.insert(
        smi::attr_w(SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_TYPE, report_type));

    smi::attr_w action_type_attr(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE);
    switch_status =
        bf_switch_attribute_get(acl_object_id,
                                SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE,
                                action_type_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get previous dtel_acl action_type 0x%" PRIx64
                    ": %s",
                    acl_entry_id,
                    sai_metadata_get_status_name(status));
    }
    sw_attrs.insert(action_type_attr);
  }

  status = sai_acl_entry_attr_to_switch(*attr, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to convert SAI ACL entry attribute %s to switch: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_ACL_ENTRY, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t table_handle;
  smi::attr_w table_handle_attr(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE);
  status |= bf_switch_attribute_get(
      acl_object_id, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle_attr);
  table_handle_attr.v_get(table_handle);

  switch_enum_t table_type = {};
  smi::attr_w table_type_attr(SWITCH_ACL_TABLE_ATTR_TYPE);
  status = bf_switch_attribute_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type_attr);
  table_type_attr.v_get(table_type);

  // Check if inner IPv4 DTEL ACL entry exists
  smi::attr_w ipv4_attr(SWITCH_ACL_ENTRY_ATTR_INNER_IPV4_ENTRY_HANDLE);
  switch_status = bf_switch_attribute_get(
      acl_object_id, SWITCH_ACL_ENTRY_ATTR_INNER_IPV4_ENTRY_HANDLE, ipv4_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv4 acl entry 0x%" PRIx64 ": %s",
                  acl_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv4_object_id = {};
  ipv4_attr.v_get(ipv4_object_id);
  if (ipv4_object_id.data != 0) {
    // Set inner IPv4 DTEL ACL entry attributes
    for (auto iter = sw_attrs.begin(); iter != sw_attrs.end(); ++iter) {
      switch_status = bf_switch_attribute_set(ipv4_object_id, *iter);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set acl entry 0x%" PRIx64 ": %s",
                      ipv4_object_id.data,
                      sai_metadata_get_status_name(status));
      }
    }
  }

  // Check if inner IPv6 DTEL ACL entry exists
  smi::attr_w ipv6_attr(SWITCH_ACL_ENTRY_ATTR_INNER_IPV6_ENTRY_HANDLE);
  switch_status = bf_switch_attribute_get(
      acl_object_id, SWITCH_ACL_ENTRY_ATTR_INNER_IPV6_ENTRY_HANDLE, ipv6_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get inner IPv6 acl entry 0x%" PRIx64 ": %s",
                  acl_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t ipv6_object_id = {};
  ipv6_attr.v_get(ipv6_object_id);
  if (ipv6_object_id.data != 0) {
    // Set inner IPv6 DTEL ACL entry attributes
    for (auto iter = sw_attrs.begin(); iter != sw_attrs.end(); iter++) {
      switch_status = bf_switch_attribute_set(ipv6_object_id, *iter);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set acl entry 0x%" PRIx64 ": %s",
                      ipv6_object_id.data,
                      sai_metadata_get_status_name(status));
      }
    }
  }

  for (auto iter = sw_attrs.begin(); iter != sw_attrs.end(); ++iter) {
    switch_status = bf_switch_attribute_set(acl_object_id, *iter);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set acl entry 0x%" PRIx64 ": %s",
                    acl_entry_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Get ACL entry attribute
 *
 * Arguments:
 *    [in] acl_entry_id - the acl entry id
 *    [in] attr_count - attribute count
 *    [inout] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_acl_entry_attribute(_In_ sai_object_id_t acl_entry_id,
                                         _In_ uint32_t attr_count,
                                         _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_entry_id) != SAI_OBJECT_TYPE_ACL_ENTRY) {
    SAI_LOG_ERROR("ACL entry get failed: invalid ACL entry handle 0x%" PRIx64
                  "\n",
                  acl_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    status = sai_acl_entry_get_attr(acl_entry_id, attr_list[index]);
    if (status != SAI_STATUS_SUCCESS) {
      return status;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *   @brief Create an ACL counter
 *
 * Arguments:
 *   @param[out] acl_counter_id - the acl counter id
 *   @param[in] switch_id The switch Object id
 *   @param[in] attr_count - number of attributes
 *   @param[in] attr_list - array of attributes
 *
 * Return Values:
 *    @return  SAI_STATUS_SUCCESS on success
 *             Failure status code on error
 */
sai_status_t sai_create_acl_counter(_Out_ sai_object_id_t *acl_counter_id,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!acl_counter_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_counter_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t counter_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_ACL_COUNTER_ATTR_TABLE_ID:
      case SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT:
      case SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT:
        break;  // Just ignore these attributes
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ACL_RANGE, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create ACL counter: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_ACL_COUNTER_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_COUNTER, sw_attrs, counter_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL counter: %s",
                  sai_metadata_get_status_name(status));
  }
  *acl_counter_id = counter_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *   @brief Delete an ACL counter
 *
 * Arguments:
 *  @param[in] acl_counter_id - the acl counter id
 *
 * Return Values:
 *    @return  SAI_STATUS_SUCCESS on success
 *             Failure status code on error
 */
sai_status_t sai_remove_acl_counter(_In_ sai_object_id_t acl_counter_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(acl_counter_id) != SAI_OBJECT_TYPE_ACL_COUNTER) {
    SAI_LOG_ERROR(
        "ACL counter remove failed: invalid ACL counter handle 0x%" PRIx64 "\n",
        acl_counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_counter_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove ACL counter 0x%" PRIx64 ": %s",
                  acl_counter_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *   @brief Set ACL counter attribute
 *
 * Arguments:
 *    @param[in] acl_counter_id - the acl counter id
 *    @param[in] attr - attribute
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t sai_set_acl_counter_attribute(_In_ sai_object_id_t acl_counter_id,
                                           _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_counter_id) != SAI_OBJECT_TYPE_ACL_COUNTER) {
    SAI_LOG_ERROR(
        "ACL counter set failed: invalid ACL counter handle 0x%" PRIx64 "\n",
        acl_counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_counter_id};
  switch (attr->id) {
    case SAI_ACL_COUNTER_ATTR_PACKETS:
    case SAI_ACL_COUNTER_ATTR_BYTES: {
      if (attr->value.u64 != 0) {
        SAI_LOG_ERROR("Failed to set counter attribute %s value to: %" PRIu64
                      " Only 0 is supported",
                      sai_attribute_name(SAI_OBJECT_TYPE_ACL_COUNTER, attr->id),
                      attr->value.u64);
        return SAI_STATUS_NOT_SUPPORTED;
      }

      uint16_t cntr_id = (attr->id == SAI_ACL_COUNTER_ATTR_PACKETS)
                             ? SWITCH_ACL_ENTRY_COUNTER_ID_PKTS
                             : SWITCH_ACL_ENTRY_COUNTER_ID_BYTES;
      std::vector<uint16_t> cntr_ids = {cntr_id};
      sw_status = bf_switch_counters_clear(sw_object_id, cntr_ids);
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to clear ACL counter 0x%" PRIx64 ": %s",
                      acl_counter_id,
                      sai_metadata_get_status_name(status));
      }
      break;
    }
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ACL_COUNTER, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_ACL_COUNTER, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *   @brief Get ACL counter attribute
 *
 * Arguments:
 *    @param[in] acl_counter_id - acl counter id
 *    @param[in] attr_count - number of attributes
 *    @param[out] attr_list - array of attributes
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t sai_get_acl_counter_attribute(_In_ sai_object_id_t acl_counter_id,
                                           _In_ uint32_t attr_count,
                                           _Out_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_counter_id) != SAI_OBJECT_TYPE_ACL_COUNTER) {
    SAI_LOG_ERROR(
        "ACL counter get failed: invalid ACL counter handle 0x%" PRIx64 "\n",
        acl_counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_counter_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_ACL_COUNTER_ATTR_PACKETS:
      case SAI_ACL_COUNTER_ATTR_BYTES: {
        attr_list[index].value.u64 = 0;
        std::set<switch_object_id_t> acl_entry_oids;
        switch_counter_t cntr = {.counter_id = 0, .count = 0};
        std::vector<switch_counter_t> sw_acl_cntrs;
        sw_acl_cntrs.push_back(cntr);
        sw_acl_cntrs.push_back(cntr);
        sw_status = switch_store::referencing_set_get(
            sw_object_id, SWITCH_OBJECT_TYPE_ACL_ENTRY, acl_entry_oids);
        for (const auto oid : acl_entry_oids) {
          std::vector<switch_counter_t> acl_entry_cntrs;
          sw_status = bf_switch_counters_get(oid, acl_entry_cntrs);
          status = status_switch_to_sai(sw_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("Failed to get ACL counter 0x%" PRIx64 ": %s",
                          acl_counter_id,
                          sai_metadata_get_status_name(status));
          }
          sw_acl_cntrs[0].count += acl_entry_cntrs[0].count;
          sw_acl_cntrs[1].count += acl_entry_cntrs[1].count;
        }

        if (sw_acl_cntrs.size() > 0) {
          attr_list[index].value.u64 =
              (attr_list[index].id == SAI_ACL_COUNTER_ATTR_PACKETS)
                  ? sw_acl_cntrs[0].count
                  : sw_acl_cntrs[1].count;
        }
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ACL_COUNTER, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ACL_COUNTER,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 *  Routine Description:
 *    Create an ACL Range
 *
 *  Arguments:
 *    [out] acl_range_id - the acl range id
 *    [in] switch_id The switch Object id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_acl_range(_Out_ sai_object_id_t *acl_range_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!acl_range_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *acl_range_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t range_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ACL_RANGE, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create ACL range: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_RANGE, sw_attrs, range_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ACL range: %s",
                  sai_metadata_get_status_name(status));
  }
  *acl_range_id = range_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  Routine Description:
 *    Remove an ACL Range
 *
 *  Arguments:
 *    [in] acl_range_id - the acl range id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_acl_range(_In_ sai_object_id_t acl_range_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(acl_range_id) != SAI_OBJECT_TYPE_ACL_RANGE) {
    SAI_LOG_ERROR("ACL range remove failed: invalid ACL range handle 0x%" PRIx64
                  "\n",
                  acl_range_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_range_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove ACL range 0x%" PRIx64 ": %s",
                  acl_range_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Set ACL range attribute
 *
 * Arguments:
 *   [in] acl_range_id - the acl range id
 *   [in] attr - attribute
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_set_acl_range_attribute(_In_ sai_object_id_t acl_range_id,
                                         _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_range_id) != SAI_OBJECT_TYPE_ACL_RANGE) {
    SAI_LOG_ERROR("ACL range set failed: invalid ACL range handle 0x%" PRIx64
                  "\n",
                  acl_range_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_range_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ACL_RANGE, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_ACL_RANGE, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Get ACL range attribute
 *
 * Arguments:
 *   [in] acl_range_id - acl range id
 *   [in] attr_count - number of attributes
 *   [out] attr_list - array of attributes
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_get_acl_range_attribute(_In_ sai_object_id_t acl_range_id,
                                         _In_ uint32_t attr_count,
                                         _Out_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(acl_range_id) != SAI_OBJECT_TYPE_ACL_RANGE) {
    SAI_LOG_ERROR("ACL range get failed: invalid ACL range handle 0x%" PRIx64
                  "\n",
                  acl_range_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = acl_range_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ACL_RANGE, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ACL_RANGE,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 *  ACL methods table retrieved with sai_api_query()
 */
sai_acl_api_t acl_api = {
  create_acl_table : sai_create_acl_table,
  remove_acl_table : sai_remove_acl_table,
  set_acl_table_attribute : NULL,
  get_acl_table_attribute : sai_get_acl_table_attribute,
  create_acl_entry : sai_create_acl_entry,
  remove_acl_entry : sai_remove_acl_entry,
  set_acl_entry_attribute : sai_set_acl_entry_attribute,
  get_acl_entry_attribute : sai_get_acl_entry_attribute,
  create_acl_counter : sai_create_acl_counter,
  remove_acl_counter : sai_remove_acl_counter,
  set_acl_counter_attribute : sai_set_acl_counter_attribute,
  get_acl_counter_attribute : sai_get_acl_counter_attribute,
  create_acl_range : sai_create_acl_range,
  remove_acl_range : sai_remove_acl_range,
  set_acl_range_attribute : sai_set_acl_range_attribute,
  get_acl_range_attribute : sai_get_acl_range_attribute,
  create_acl_table_group : sai_create_acl_group,
  remove_acl_table_group : sai_remove_acl_group,
  set_acl_table_group_attribute : sai_set_acl_table_group_attribute,
  get_acl_table_group_attribute : sai_get_acl_table_group_attribute,
  create_acl_table_group_member : sai_create_acl_group_member,
  remove_acl_table_group_member : sai_remove_acl_group_member,
  set_acl_table_group_member_attribute :
      sai_set_acl_table_group_member_attribute,
  get_acl_table_group_member_attribute :
      sai_get_acl_table_group_member_attribute
};

sai_acl_api_t *sai_acl_api_get() { return &acl_api; }

sai_status_t sai_acl_initialize(bool warm_init) {
  SAI_LOG_DEBUG("Initializing acl");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ACL_TABLE);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ACL_ENTRY);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ACL_COUNTER);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ACL_RANGE);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ACL_TABLE_GROUP);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER);
  return SAI_STATUS_SUCCESS;
}
