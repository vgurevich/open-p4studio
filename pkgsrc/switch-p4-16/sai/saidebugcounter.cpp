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

#include <unordered_map>
#include <vector>
#include <set>
#include <list>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_DEBUG_COUNTER;

namespace std {
template <>
struct hash<sai_debug_counter_type_t> {
  inline size_t operator()(sai_debug_counter_type_t const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};

template <>
struct hash<sai_out_drop_reason_t> {
  inline size_t operator()(sai_out_drop_reason_t const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};

template <>
struct hash<sai_in_drop_reason_t> {
  inline size_t operator()(sai_in_drop_reason_t const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};

template <>
struct hash<switch_drop_reason_attr_type> {
  inline size_t operator()(switch_drop_reason_attr_type const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};
}  // namespace std

static const std::unordered_map<sai_debug_counter_type_t,
                                std::array<uint64_t, 2>>
    sai_debug_counter_type_to_switch_mapping = {
        {SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
         {SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_IN_DROP_REASONS,
          SWITCH_OBJECT_TYPE_DEBUG_COUNTER}},
        {SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
         {SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_OUT_DROP_REASONS,
          SWITCH_OBJECT_TYPE_DEBUG_COUNTER}},
        {SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS,
         {SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_IN_DROP_REASONS,
          SWITCH_OBJECT_TYPE_DEBUG_COUNTER}},
        {SAI_DEBUG_COUNTER_TYPE_SWITCH_OUT_DROP_REASONS,
         {SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_OUT_DROP_REASONS,
          SWITCH_OBJECT_TYPE_DEBUG_COUNTER}}};

/* Unsupported in drop reasons
 * SAI_IN_DROP_REASON_VLAN_TAG_NOT_ALLOWED // Unsupported
 * SAI_IN_DROP_REASON_L2_LOOPBACK_FILTER // Unsupported
 * SAI_IN_DROP_REASON_EXCEEDS_L2_MTU // Unsupported
 * SAI_IN_DROP_REASON_EXCEEDS_L3_MTU // Unsupported
 * SAI_IN_DROP_REASON_L3_LOOPBACK_FILTER // Unsupported
 * SAI_IN_DROP_REASON_NO_L3_HEADER // Unsupported
 * SAI_IN_DROP_REASON_SIP_EQUALS_DIP // Unsupported
 * SAI_IN_DROP_REASON_ERIF_DISABLED // Unsupported
 * SAI_IN_DROP_REASON_BLACKHOLE_ARP // Unsupported
 * SAI_IN_DROP_REASON_UNRESOLVED_NEXT_HOP // Unsupported
 * SAI_IN_DROP_REASON_L3_EGRESS_LINK_DOWN // Unsupported
 * SAI_IN_DROP_REASON_DECAP_ERROR // Unsupported
 * SAI_IN_DROP_REASON_ACL_ANY // Unsupported
 * SAI_IN_DROP_REASON_ACL_INGRESS_PORT // Unsupported
 * SAI_IN_DROP_REASON_ACL_INGRESS_LAG // Unsupported
 * SAI_IN_DROP_REASON_ACL_INGRESS_VLAN // Unsupported
 * SAI_IN_DROP_REASON_ACL_INGRESS_RIF // Unsupported
 * SAI_IN_DROP_REASON_ACL_INGRESS_SWITCH // Unsupported
 * SAI_IN_DROP_REASON_ACL_EGRESS_PORT // Unsupported
 * SAI_IN_DROP_REASON_ACL_EGRESS_LAG // Unsupported
 * SAI_IN_DROP_REASON_ACL_EGRESS_VLAN // Unsupported
 * SAI_IN_DROP_REASON_ACL_EGRESS_RIF // Unsupported
 * SAI_IN_DROP_REASON_ACL_EGRESS_SWITCH // Unsupported
 */
static const std::unordered_map<sai_in_drop_reason_t,
                                std::array<switch_drop_reason_attr_type, 1>>
    sai_in_dc_drop_reason_type_to_switch_attr_type_mapping = {
        /* TODO the list to be completed */
        {SAI_IN_DROP_REASON_INGRESS_VLAN_FILTER,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_INGRESS_VLAN_FILTER}},
        {SAI_IN_DROP_REASON_L2_ANY,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L2_ANY}},
        {SAI_IN_DROP_REASON_SMAC_MULTICAST,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SMAC_MULTICAST}},
        {SAI_IN_DROP_REASON_DMAC_RESERVED,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DMAC_RESERVED}},
        // support of SAI_IN_DROP_REASON_SMAC_EQUALS_DMAC may vary per profile
        {SAI_IN_DROP_REASON_SMAC_EQUALS_DMAC,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SMAC_EQUALS_DMAC}},
        {SAI_IN_DROP_REASON_TTL,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_TTL}},
        {SAI_IN_DROP_REASON_IP_HEADER_ERROR,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IP_HEADER_ERROR}},
        {SAI_IN_DROP_REASON_SIP_MC,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_MC}},
        {SAI_IN_DROP_REASON_IRIF_DISABLED,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_RIF_DISABLED_DISCARDS}},
        {SAI_IN_DROP_REASON_L3_ANY,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L3_ANY}},
        {SAI_IN_DROP_REASON_ACL_ANY,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_ACL_ANY}},
        {SAI_IN_DROP_REASON_DIP_LOOPBACK,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DIP_LOOPBACK}},
        {SAI_IN_DROP_REASON_SIP_LOOPBACK,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_LOOPBACK}},
        {SAI_IN_DROP_REASON_SIP_CLASS_E,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_CLASS_E}},
        {SAI_IN_DROP_REASON_DIP_LINK_LOCAL,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DIP_LINK_LOCAL}},
        {SAI_IN_DROP_REASON_SIP_LINK_LOCAL,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_LINK_LOCAL}},
        {SAI_IN_DROP_REASON_SIP_UNSPECIFIED,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_UNSPECIFIED}},
        {SAI_IN_DROP_REASON_NON_ROUTABLE,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_NON_ROUTABLE}},
#if SAI_API_VERSION >= 10901
        {SAI_IN_DROP_REASON_MPLS_MISS,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_MPLS_MISS}},
        {SAI_IN_DROP_REASON_SRV6_LOCAL_SID_DROP,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SRV6_MY_SID_DROP}},
#endif
        {SAI_IN_DROP_REASON_LPM4_MISS,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_LPM4_MISS}},
        {SAI_IN_DROP_REASON_LPM6_MISS,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_LPM6_MISS}},
        {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_BLACKHOLE_ROUTE}},
        {SAI_IN_DROP_REASON_UC_DIP_MC_DMAC,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_UC_DIP_MC_DMAC}},
        {SAI_IN_DROP_REASON_FDB_AND_BLACKHOLE_DISCARDS,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_AND_BLACKHOLE_DISCARDS}},
        {SAI_IN_DROP_REASON_FDB_UC_DISCARD,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_UC_DISCARD}},
        {SAI_IN_DROP_REASON_FDB_MC_DISCARD,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_MC_DISCARD}},
        {SAI_IN_DROP_REASON_SIP_BC,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_BC}},
        {SAI_IN_DROP_REASON_DIP_LOCAL,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_IP_DST_UNSPECIFIED}},
        {SAI_IN_DROP_REASON_IPV6_MC_SCOPE0,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IPV6_MC_SCOPE0}},
        {SAI_IN_DROP_REASON_IPV6_MC_SCOPE1,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IPV6_MC_SCOPE1}},
        {SAI_IN_DROP_REASON_EXCEEDS_L3_MTU,
         {SWITCH_DROP_REASON_ATTR_TYPE_OUT_MTU_CHECK_FAIL}},
        {SAI_IN_DROP_REASON_INGRESS_STP_FILTER,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_STP_FILTER}},
        {SAI_IN_DROP_REASON_MC_DMAC_MISMATCH,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_MC_DMAC_MISMATCH}},
        {SAI_IN_DROP_REASON_L3_EGRESS_LINK_DOWN,
         {SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L3_EGRESS_LINK_DOWN}}};
/* Unsupported out drop reasons
 * SAI_OUT_DROP_REASON_L2_ANY // Unsupported
 * SAI_OUT_DROP_REASON_EGRESS_VLAN_FILTER // Unsupported
 * SAI_OUT_DROP_REASON_L3_ANY // Unsupported
 * SAI_OUT_DROP_REASON_L3_EGRESS_LINK_DOWN // Unsupported
 * SAI_OUT_DROP_REASON_TUNNEL_LOOPBACK_PACKET_DROP // Unsupported
 */
static const std::unordered_map<sai_out_drop_reason_t,
                                std::array<switch_drop_reason_attr_type, 1>>
    sai_out_dc_drop_reason_type_to_switch_attr_type_mapping;

static const std::unordered_map<uint64_t, uint64_t>
    switch_dc_type_to_sai_mapping = {
        {SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_IN_DROP_REASONS,
         SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS},
        {SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_IN_DROP_REASONS,
         SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS},
        {SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_OUT_DROP_REASONS,
         SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS},
        {SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_OUT_DROP_REASONS,
         SAI_DEBUG_COUNTER_TYPE_SWITCH_OUT_DROP_REASONS}};

sai_status_t sai_get_debug_counter_type_availability(
    sai_object_id_t switch_id,
    sai_object_type_t object_type,
    uint32_t attr_count,
    const sai_attribute_t *attr_list,
    uint64_t *count) {
  const sai_attribute_t *attribute = NULL;
  if (object_type == SAI_OBJECT_TYPE_DEBUG_COUNTER) {
    for (uint32_t i = 0; i < attr_count; i++) {
      attribute = &attr_list[i];
      if (attribute->id == SAI_DEBUG_COUNTER_ATTR_TYPE) {
        switch (attribute->value.s32) {
          case SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS:
          case SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS:
          case SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS:
          case SAI_DEBUG_COUNTER_TYPE_SWITCH_OUT_DROP_REASONS:
            /* indexes for all dc types are shared therefore this formula */
            *count = ((SAI_PORT_STAT_IN_DROP_REASON_RANGE_END -
                       SAI_PORT_STAT_IN_DROP_REASON_RANGE_BASE) /
                      4) +
                     1;
            SAI_LOG_DEBUG("Query debug counter availability: %" PRIu64 "",
                          *count);
            return SAI_STATUS_SUCCESS;
          default:
            break;
        }
      }
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_debug_counter_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;
  size_t size = 0;

  if (attr_id == SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST) {
    size = sai_in_dc_drop_reason_type_to_switch_attr_type_mapping.size();
    if (size > enum_values_capability->count) {
      enum_values_capability->count = size;
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
    for (auto it : sai_in_dc_drop_reason_type_to_switch_attr_type_mapping) {
      if ((it.first == SAI_IN_DROP_REASON_SMAC_EQUALS_DMAC) &&
          !bf_switch_is_feature_enabled(SWITCH_FEATURE_SAME_MAC_CHECK)) {
        continue;
      }
      if ((it.first == SAI_IN_DROP_REASON_INGRESS_STP_FILTER) &&
          !bf_switch_is_feature_enabled(SWITCH_FEATURE_STP)) {
        continue;
      }
      if ((it.first == SAI_IN_DROP_REASON_MC_DMAC_MISMATCH) &&
          !bf_switch_is_feature_enabled(SWITCH_FEATURE_IPMC_DMAC_VALIDATION)) {
        continue;
      }
      enum_values_capability->list[i] = it.first;
      i++;
    }
  } else if (attr_id == SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST) {
    size = sai_out_dc_drop_reason_type_to_switch_attr_type_mapping.size();
    if (size > enum_values_capability->count) {
      enum_values_capability->count = size;
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
    for (auto it : sai_out_dc_drop_reason_type_to_switch_attr_type_mapping) {
      enum_values_capability->list[i] = it.first;
      i++;
    }
  } else if (attr_id == SAI_DEBUG_COUNTER_ATTR_TYPE) {
    size = sai_debug_counter_type_to_switch_mapping.size();
    if (size > enum_values_capability->count) {
      enum_values_capability->count = size;
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
    for (auto it : sai_debug_counter_type_to_switch_mapping) {
      enum_values_capability->list[i] = it.first;
      i++;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    return status;
  }
  enum_values_capability->count = i;
  return status;
}

static sai_status_t sai_in_drop_reason_to_switch_attr(
    sai_in_drop_reason_t in_drop_reason_type,
    switch_drop_reason_attr_type *switch_drop_reason) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;

  auto it = sai_in_dc_drop_reason_type_to_switch_attr_type_mapping.find(
      in_drop_reason_type);
  if (it != sai_in_dc_drop_reason_type_to_switch_attr_type_mapping.end()) {
    if ((in_drop_reason_type != SAI_IN_DROP_REASON_SMAC_EQUALS_DMAC) ||
        bf_switch_is_feature_enabled(SWITCH_FEATURE_SAME_MAC_CHECK)) {
      *switch_drop_reason = it->second[0];
      status = SAI_STATUS_SUCCESS;
    }
  }

  return status;
}

static sai_status_t sai_out_drop_reason_to_switch_attr(
    sai_out_drop_reason_t out_drop_reason_type,
    switch_drop_reason_attr_type *switch_drop_reason) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;

  auto it = sai_out_dc_drop_reason_type_to_switch_attr_type_mapping.find(
      out_drop_reason_type);
  if (it != sai_out_dc_drop_reason_type_to_switch_attr_type_mapping.end()) {
    *switch_drop_reason = it->second[0];
    status = SAI_STATUS_SUCCESS;
  }

  return status;
}

sai_status_t sw_out_drop_reason_to_switch_port_counter(
    switch_drop_reason_attr_type switch_drop_reason,
    std::vector<uint16_t> &cntr_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (switch_drop_reason) {
    default:
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
  }

  return status;
}

sai_status_t sw_in_drop_reason_to_switch_port_counter(
    switch_drop_reason_attr_type switch_drop_reason,
    std::vector<uint16_t> &cntr_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (switch_drop_reason) {
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SMAC_MULTICAST:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SMAC_MULTICAST_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SMAC_MULTICAST_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L2_ANY:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SMAC_ZERO_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SMAC_ZERO_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_DMAC_ZERO_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DMAC_ZERO_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SMAC_MULTICAST_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SMAC_MULTICAST_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SAME_MAC_CHECK_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_VLAN_DISCARDS);
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_IPMC_DMAC_VALIDATION)) {
        cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_MC_DMAC_MISMATCH);
      }
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SMAC_EQUALS_DMAC:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_SAME_MAC_CHECK_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_INGRESS_VLAN_FILTER:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_VLAN_DISCARDS);
      break;
    // L3 drop reasons
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_TTL:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_TTL_ZERO_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_TTL_ZERO_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_MC:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_SRC_MULTICAST_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_MULTICAST_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IP_HEADER_ERROR:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_IHL_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_IHL_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_CHECKSUM_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_CHECKSUM_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_VERSION_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_VERSION_INVALID_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_RIF_DISABLED_DISCARDS:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV4_DISABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV6_DISABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_MPLS_DISABLE_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_ACL_ANY:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_ACL_DENY_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_ACL_METER_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L3_ANY:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV4_DISABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L3_IPV6_DISABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_MPLS_DISABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_TTL_ZERO_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_TTL_ZERO_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_SRC_MULTICAST_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_MULTICAST_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_IHL_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_IHL_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_CHECKSUM_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_CHECKSUM_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_OUTER_IP_VERSION_INVALID_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_VERSION_INVALID_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SIP_LOOPBACK_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DIP_LOOPBACK_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_CLASS_E_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_DST_LINK_LOCAL_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_LINK_LOCAL_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DIP_UNSPECIFIED_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SIP_UNSPECIFIED_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_UC_DIP_MC_DMAC_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_BC_DIP_MC_DMAC_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_NON_ROUTABLE_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM4_MISS_DISCARDS);
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM6_MISS_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_BLACKHOLE_ROUTE_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_L3_PORT_RMAC_MISS_DISCARDS);
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_L3_EGRESS_LINK_DOWN_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_CLASS_E:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_CLASS_E_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DIP_LOOPBACK:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DIP_LOOPBACK_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_LOOPBACK:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SIP_LOOPBACK_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DIP_LINK_LOCAL:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_DST_LINK_LOCAL_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_LINK_LOCAL:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_SRC_LINK_LOCAL_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_UNSPECIFIED:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SIP_UNSPECIFIED_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_UC_DIP_MC_DMAC:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_UC_DIP_MC_DMAC_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_NON_ROUTABLE:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_NON_ROUTABLE_DISCARDS);
      break;

    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_MPLS_MISS:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_MPLS_LOOKUP_MISS_DISCARD);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SRV6_MY_SID_DROP:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SRV6_MY_SID_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_LPM4_MISS:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM4_MISS_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_LPM6_MISS:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IP_LPM6_MISS_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_BLACKHOLE_ROUTE:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_IP_BLACKHOLE_ROUTE_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L3_PORT_RMAC_MISS:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_L3_PORT_RMAC_MISS_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_AND_BLACKHOLE_DISCARDS:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_FDB_AND_BLACKHOLE_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_UC_DISCARD:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_UNICAST);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_FDB_MC_DISCARD:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_MULTICAST);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_DMAC_RESERVED:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DMAC_RESERVED_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_SIP_BC:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_SIP_BC_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_IP_DST_UNSPECIFIED:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_DIP_UNSPECIFIED_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IPV6_MC_SCOPE0:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IPV6_MC_SCOPE0_DISCARD);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_IPV6_MC_SCOPE1:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_IPV6_MC_SCOPE1_DISCARD);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_OUT_MTU_CHECK_FAIL:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_OUT_MTU_CHECK_FAIL_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_STP_FILTER:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_STP_STATE_BLOCKING_DISCARDS);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_MC_DMAC_MISMATCH:
      cntr_ids.push_back(SWITCH_PORT_COUNTER_ID_IF_IN_MC_DMAC_MISMATCH);
      break;
    case SWITCH_DROP_REASON_ATTR_TYPE_IN_DROP_REASON_L3_EGRESS_LINK_DOWN:
      cntr_ids.push_back(
          SWITCH_PORT_COUNTER_ID_IF_IN_L3_EGRESS_LINK_DOWN_DISCARDS);
      break;
    default:
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
  }

  return status;
}

sai_status_t sai_create_debug_counter(_Out_ sai_object_id_t *debug_counter_id,
                                      _In_ sai_object_id_t switch_id,
                                      _In_ uint32_t attr_count,
                                      _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  switch_object_type_t ot = SWITCH_OBJECT_TYPE_DEBUG_COUNTER;
  sai_debug_counter_type_t dc_type =
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS;
  sai_debug_counter_attr_t drop_list_type;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_dc_object_id = {};
  switch_attr_id_t dev_attr_id = 0;
  switch_drop_reason_attr_type sw_drop_reason;
  uint32_t index = 0;

  if (attr_count && !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  std::set<attr_w> sw_attrs;
  attribute = sai_get_attr_from_list(
      SAI_DEBUG_COUNTER_ATTR_TYPE, attr_list, attr_count);
  if (attribute == NULL) {
    SAI_LOG_ERROR(
        "DebugCounter attribute parse failed. "
        "SAI_DEBUG_COUNTER_ATTR_TYPE attribute is missing");
    return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
  }
  switch_enum_t sw_dc_type = {0};
  dc_type = static_cast<sai_debug_counter_type_t>(attribute->value.s32);
  auto it = sai_debug_counter_type_to_switch_mapping.find(dc_type);
  if (it != sai_debug_counter_type_to_switch_mapping.end()) {
    sw_dc_type.enumdata = it->second[0];
    ot = it->second[1];
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    SAI_LOG_ERROR("DebugCounter attribute parse failed %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  dev_attr_id = SWITCH_DEBUG_COUNTER_ATTR_DEVICE;
  switch (dc_type) {
    case SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS:
    case SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS:
      drop_list_type = SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST;
      break;
    case SAI_DEBUG_COUNTER_TYPE_SWITCH_OUT_DROP_REASONS:
    case SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS:
      drop_list_type = SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST;
      break;
    default:
      SAI_LOG_ERROR("Invalid SAI debug counter type\n");
      return SAI_STATUS_INVALID_PARAMETER;
  }

  attribute = sai_get_attr_from_list(drop_list_type, attr_list, attr_count);
  if (attribute == NULL) {
    SAI_LOG_ERROR(
        "DebugCounter attribute parse failed. "
        "Drop reason list is missing");
    return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
  }

  sai_insert_device_attribute(0, dev_attr_id, sw_attrs);
  sw_attrs.insert(attr_w(SWITCH_DEBUG_COUNTER_ATTR_TYPE, sw_dc_type));

  smi::attr_w drop_reasons_list_attr(
      SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST);
  smi::attr_w drop_counters_list_attr(
      SWITCH_DEBUG_COUNTER_ATTR_DROP_COUNTERS_LIST);
  std::vector<uint16_t> drop_reason_list;
  for (uint32_t j = 0; j < attribute->value.s32list.count; j++) {
    if (drop_list_type == SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST) {
      status = sai_in_drop_reason_to_switch_attr(
          sai_in_drop_reason_t(attribute->value.s32list.list[j]),
          &sw_drop_reason);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("DebugCounter unsupported SAI drop reason: %s",
                      sai_metadata_get_in_drop_reason_name(sai_in_drop_reason_t(
                          attribute->value.s32list.list[j])));
        return status;
      }
    } else {
      status = sai_out_drop_reason_to_switch_attr(
          sai_out_drop_reason_t(attribute->value.s32list.list[j]),
          &sw_drop_reason);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "DebugCounter unsupported SAI drop reason: %s",
            sai_metadata_get_out_drop_reason_name(
                sai_out_drop_reason_t(attribute->value.s32list.list[j])));
        return status;
      }
    }
    for (unsigned i = 0; i < drop_reason_list.size(); i++) {
      if (drop_reason_list[i] == sw_drop_reason) {
        SAI_LOG_ERROR("DebugCounter duplicate SAI drop reason in the list.");
        return SAI_STATUS_ITEM_ALREADY_EXISTS;
      }
    }
    drop_reason_list.push_back(sw_drop_reason);
  }

  // for each drop_reason find associated drop_counters
  std::vector<uint16_t> drop_counters_list;
  for (auto drop_reason : drop_reason_list) {
    std::vector<uint16_t> cntr_ids;
    switch_drop_reason_attr_type switch_drop_reason =
        static_cast<switch_drop_reason_attr_type>(drop_reason);
    if (drop_list_type == SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST) {
      status = sw_in_drop_reason_to_switch_port_counter(switch_drop_reason,
                                                        cntr_ids);
    } else {
      status = sw_out_drop_reason_to_switch_port_counter(switch_drop_reason,
                                                         cntr_ids);
    }
    if (status != SAI_STATUS_SUCCESS) break;

    for (unsigned j = 0; j < cntr_ids.size(); j++) {
      bool found = false;
      for (auto counter_id : drop_counters_list) {
        if (cntr_ids[j] == counter_id) {
          found = true;
          break;
        }
      }
      if (!found) drop_counters_list.push_back(cntr_ids[j]);
    }
  }

  drop_reasons_list_attr.v_set(drop_reason_list);
  // drop_counters_list is internal list only
  drop_counters_list_attr.v_set(drop_counters_list);
  sw_attrs.insert(drop_reasons_list_attr);
  sw_attrs.insert(drop_counters_list_attr);
  sw_attrs.insert(attr_w(SWITCH_DEBUG_COUNTER_ATTR_INDEX, index));

  switch_status = bf_switch_object_create(ot, sw_attrs, sw_dc_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create debug counter, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  /* update the debug counter index based on obj_id.
   * each debug counter is identified by obj_id although it can be searched
   * by an unique index.
   * The object type determines the index range accessed
   * by e.g. sai_get_port_stats_ext().
   */
  *debug_counter_id = sw_dc_object_id.data;
  index = switch_store::handle_to_id(sw_dc_object_id);
  attr_w sw_attr(SWITCH_DEBUG_COUNTER_ATTR_INDEX, index);
  switch_status = bf_switch_attribute_set(sw_dc_object_id, sw_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create the debug counter index, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  return status;
}

sai_status_t sai_remove_debug_counter(_In_ sai_object_id_t debug_counter_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = debug_counter_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);

  if (ot != SWITCH_OBJECT_TYPE_DEBUG_COUNTER) {
    SAI_LOG_ERROR("Invalid debug counter object id 0x%" PRIx64,
                  debug_counter_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }
  // remove the drop reasons list
  attr_w attr(SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST, attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get debug counters drop reason list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (attr.type_get() != SWITCH_TYPE_LIST) {
    SAI_LOG_ERROR("Invalid SMI attribute type %u", attr.type_get());
    return SAI_STATUS_FAILURE;
  }
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to delete debug counter : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  return status;
}

sai_status_t switch_dc_type_to_sai_dc_type(
    switch_enum_t sw_dc_type, sai_debug_counter_type_t *sai_dc_type) {
  auto it = switch_dc_type_to_sai_mapping.find(sw_dc_type.enumdata);
  if (it != switch_dc_type_to_sai_mapping.end()) {
    *sai_dc_type = static_cast<sai_debug_counter_type_t>(it->second);
    return SAI_STATUS_SUCCESS;
  }
  return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t switch_out_drop_reason_to_sai_drop_reason(
    switch_drop_reason_attr_type drop_reason,
    sai_out_drop_reason_t *sai_out_drop_reason) {
  for (auto it : sai_out_dc_drop_reason_type_to_switch_attr_type_mapping) {
    if (it.second[0] == drop_reason) {
      *sai_out_drop_reason = it.first;
      return SAI_STATUS_SUCCESS;
    }
  }
  return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t switch_in_drop_reason_to_sai_drop_reason(
    switch_drop_reason_attr_type drop_reason,
    sai_in_drop_reason_t *sai_in_drop_reason) {
  for (auto it : sai_in_dc_drop_reason_type_to_switch_attr_type_mapping) {
    if (it.second[0] == drop_reason) {
      *sai_in_drop_reason = it.first;
      return SAI_STATUS_SUCCESS;
    }
  }
  return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_get_debug_counter_attribute(
    _In_ sai_object_id_t debug_counter_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = debug_counter_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);
  sai_attribute_t *attribute = NULL;
  switch_enum_t sw_dc_type = {0};
  uint32_t dc_index;
  sai_debug_counter_type_t sai_dc_type;

  if (ot != SWITCH_OBJECT_TYPE_DEBUG_COUNTER) {
    SAI_LOG_ERROR("Invalid object type 0x%" PRIx64, debug_counter_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  attr_w type_attr(SWITCH_DEBUG_COUNTER_ATTR_TYPE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_DEBUG_COUNTER_ATTR_TYPE, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get debug counter : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  type_attr.v_get(sw_dc_type);
  status = switch_dc_type_to_sai_dc_type(sw_dc_type, &sai_dc_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to map switch debug counter type for handle 0x%" PRIx64 " : %s",
        debug_counter_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; i++) {
    attribute = &attr_list[i];
    switch (attribute->id) {
      case SAI_DEBUG_COUNTER_ATTR_TYPE: {
        attribute->value.s32 = sai_dc_type;
        break;
      }
      case SAI_DEBUG_COUNTER_ATTR_INDEX: {
        attr_w index_attr(SWITCH_DEBUG_COUNTER_ATTR_INDEX);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_DEBUG_COUNTER_ATTR_INDEX, index_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get debug counter index: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        if (index_attr.type_get() != SWITCH_TYPE_UINT32) {
          SAI_LOG_ERROR("Invalid SMI attribute type %u", index_attr.type_get());
          return SAI_STATUS_FAILURE;
        }
        index_attr.v_get(dc_index);
        attribute->value.u32 = dc_index;
        break;
      }
      case SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST:
      case SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST: {
        attr_w drop_reason_list_attr(
            SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST);
        std::vector<uint16_t> drop_reason_list;
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST,
                                    drop_reason_list_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get debug counter drop reason list: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        drop_reason_list_attr.v_get(drop_reason_list);

        std::list<int32_t> converted_drop_reasons;
        for (auto sw_drop_reason : drop_reason_list) {
          int32_t drop_reason = 0;
          if (attribute->id == SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST) {
            status = switch_in_drop_reason_to_sai_drop_reason(
                static_cast<switch_drop_reason_attr_type>(sw_drop_reason),
                reinterpret_cast<sai_in_drop_reason_t *>(&drop_reason));
          } else {
            status = switch_out_drop_reason_to_sai_drop_reason(
                static_cast<switch_drop_reason_attr_type>(sw_drop_reason),
                reinterpret_cast<sai_out_drop_reason_t *>(&drop_reason));
          }
          if (status == SAI_STATUS_SUCCESS) {
            converted_drop_reasons.push_back(static_cast<int32_t>(drop_reason));
          }
        }
        TRY_LIST_SET(attribute->value.s32list, converted_drop_reasons);
        break;
      }
      default:
        SAI_LOG_ERROR("Unsupported attribute");
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_debug_counter_attribute(
    _In_ sai_object_id_t debug_counter_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = debug_counter_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);
  switch_drop_reason_attr_type new_drop_reason;
  switch_enum_t sw_dc_type = {0};
  sai_debug_counter_type_t sai_dc_type;

  if (ot != SWITCH_OBJECT_TYPE_DEBUG_COUNTER) {
    SAI_LOG_ERROR("Invalid object type 0x%" PRIx64, debug_counter_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }
  attr_w type_attr(SWITCH_DEBUG_COUNTER_ATTR_TYPE);
  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_DEBUG_COUNTER_ATTR_TYPE, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get debug counter : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(sw_dc_type);
  status = switch_dc_type_to_sai_dc_type(sw_dc_type, &sai_dc_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to map switch debug counter type for handle 0x%" PRIx64 " : %s",
        debug_counter_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_DEBUG_COUNTER_ATTR_TYPE: {
      // read only attribute
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_ERROR("Debug Counter type change unsupported: %s",
                    sai_metadata_get_status_name(status));
      break;
    }
    case SAI_DEBUG_COUNTER_ATTR_INDEX: {
      // read only attribute
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_ERROR("Debug Counter index change unsupported: %s",
                    sai_metadata_get_status_name(status));
      break;
    }
    case SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST:
    case SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST: {
      attr_w drop_reason_list_attr(SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST);
      switch_status =
          bf_switch_attribute_get(sw_object_id,
                                  SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST,
                                  drop_reason_list_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get debug counter drop reason list: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      // check if input data corresponds to dc object type
      if (((sw_dc_type.enumdata ==
            SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_IN_DROP_REASONS) ||
           (sw_dc_type.enumdata ==
            SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_IN_DROP_REASONS)) &&
          (attr->id != SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST)) {
        status = SAI_STATUS_FAILURE;
        SAI_LOG_ERROR(
            "Unable to set debug counter attribute."
            "Expected list type SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST");
        return status;
      } else if (((sw_dc_type.enumdata ==
                   SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_OUT_DROP_REASONS) ||
                  (sw_dc_type.enumdata ==
                   SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_OUT_DROP_REASONS)) &&
                 (attr->id != SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST)) {
        status = SAI_STATUS_FAILURE;
        SAI_LOG_ERROR(
            "Unable to set debug counter attribute."
            "Expected list type SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST");
        return status;
      }

      std::vector<uint16_t> drop_reason_list;
      for (uint32_t j = 0; j < attr->value.s32list.count; j++) {
        if ((sw_dc_type.enumdata ==
             SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_IN_DROP_REASONS) ||
            (sw_dc_type.enumdata ==
             SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_IN_DROP_REASONS)) {
          status = sai_in_drop_reason_to_switch_attr(
              sai_in_drop_reason_t(attr->value.s32list.list[j]),
              &new_drop_reason);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "DebugCounter unsupported SAI drop reason: %s",
                sai_metadata_get_in_drop_reason_name(
                    sai_in_drop_reason_t(attr->value.s32list.list[j])));
            return status;
          }
        } else {
          status = sai_out_drop_reason_to_switch_attr(
              sai_out_drop_reason_t(attr->value.s32list.list[j]),
              &new_drop_reason);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "DebugCounter unsupported SAI drop reason: %s",
                sai_metadata_get_out_drop_reason_name(
                    sai_out_drop_reason_t(attr->value.s32list.list[j])));
            return status;
          }
        }

        for (auto drop_reason : drop_reason_list) {
          // check for duplicates
          if (drop_reason == new_drop_reason) {
            status = SAI_STATUS_ITEM_ALREADY_EXISTS;
            break;
          }
        }
        if (status == SAI_STATUS_ITEM_ALREADY_EXISTS) {
          SAI_LOG_ERROR("Detected duplicate drop_reason.");
          return status;
        }
        drop_reason_list.push_back(new_drop_reason);
      }

      // for each drop_reason find associated drop_counters
      std::vector<uint16_t> drop_counters_list;
      for (auto drop_reason : drop_reason_list) {
        std::vector<uint16_t> cntr_ids;
        switch_drop_reason_attr_type switch_drop_reason =
            static_cast<switch_drop_reason_attr_type>(drop_reason);
        if ((sw_dc_type.enumdata ==
             SWITCH_DEBUG_COUNTER_ATTR_TYPE_PORT_IN_DROP_REASONS) ||
            (sw_dc_type.enumdata ==
             SWITCH_DEBUG_COUNTER_ATTR_TYPE_SWITCH_IN_DROP_REASONS)) {
          status = sw_in_drop_reason_to_switch_port_counter(switch_drop_reason,
                                                            cntr_ids);
        } else {
          status = sw_out_drop_reason_to_switch_port_counter(switch_drop_reason,
                                                             cntr_ids);
        }
        if (status != SAI_STATUS_SUCCESS) break;

        for (unsigned j = 0; j < cntr_ids.size(); j++) {
          bool found = false;
          for (auto counter_id : drop_counters_list) {
            if (cntr_ids[j] == counter_id) {
              found = true;
              break;
            }
          }
          // check for duplicate drop counters
          if (!found) drop_counters_list.push_back(cntr_ids[j]);
        }
      }

      // set the drop reasons list
      attr_w drop_reasons_list_attr(
          SWITCH_DEBUG_COUNTER_ATTR_DROP_REASONS_LIST);
      drop_reasons_list_attr.v_set(drop_reason_list);
      switch_status =
          bf_switch_attribute_set(sw_object_id, drop_reasons_list_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set debug counter drop reason list: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      // set the drop counters list.
      // the counters_list is an internal only list of unique switch counters
      // associated  with drop_reasons assigned to debug counter.
      // It is used by sai_get_port_stats_ext() to decrease the debug counters
      // stats read time.
      attr_w drop_counters_list_attr(
          SWITCH_DEBUG_COUNTER_ATTR_DROP_COUNTERS_LIST);
      drop_counters_list_attr.v_set(drop_counters_list);
      switch_status =
          bf_switch_attribute_set(sw_object_id, drop_counters_list_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set debug counter drop reason list: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    default:
      SAI_LOG_ERROR("Unsupported attribute");
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * Debug counter methods table retrieved with sai_api_query()
 */
sai_debug_counter_api_t debug_counter_api = {
  create_debug_counter : sai_create_debug_counter,
  remove_debug_counter : sai_remove_debug_counter,
  set_debug_counter_attribute : sai_set_debug_counter_attribute,
  get_debug_counter_attribute : sai_get_debug_counter_attribute
};

sai_debug_counter_api_t *sai_debug_counter_api_get() {
  return &debug_counter_api;
}

sai_status_t sai_debug_counter_initialize() {
  SAI_LOG_DEBUG("Initializing debug counter");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DEBUG_COUNTER);

  return SAI_STATUS_SUCCESS;
}
