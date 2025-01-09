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

#include <map>
#include <set>
#include <functional>
#include <unordered_map>
#include <vector>
#include <list>
#include <utility>

#include "s3/switch_store.h"

extern sai_status_t sai_get_qos_map_attribute(
    _In_ sai_object_id_t qos_map_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list);

static const sai_api_t api_id = SAI_API_PORT;
const sai_object_type_t sai_ot = SAI_OBJECT_TYPE_PORT;
static switch_object_id_t device_handle = {0};
static const switch_object_id_t null_id = {0};

static const sai_to_switch_counters_map port_to_switch_counter_mapping{
    {SAI_PORT_STAT_IF_IN_OCTETS, {SWITCH_PORT_COUNTER_ID_IN_ALL_OCTETS}},
    {SAI_PORT_STAT_IF_IN_UCAST_PKTS, {SWITCH_PORT_COUNTER_ID_IN_UCAST_PKTS}},
    {SAI_PORT_STAT_IF_IN_ERRORS, {SWITCH_PORT_COUNTER_ID_IN_ERROR_PKTS}},
    {SAI_PORT_STAT_IF_IN_BROADCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IN_BCAST_PKTS}},
    {SAI_PORT_STAT_IF_IN_MULTICAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IN_MCAST_PKTS}},

// This a customer specific parameters.
// Later will move this to a patch.
#ifdef PORT_RATE_STAT
    {SAI_PORT_STAT_IF_IN_OCTETS_RATE, {SWITCH_PORT_COUNTER_ID_IN_OCTETS_RATE}},
    {SAI_PORT_STAT_IF_IN_PKTS_RATE, {SWITCH_PORT_COUNTER_ID_IN_PKTS_RATE}},
#endif

    {SAI_PORT_STAT_IF_OUT_OCTETS, {SWITCH_PORT_COUNTER_ID_OUT_ALL_OCTETS}},
    {SAI_PORT_STAT_IF_OUT_UCAST_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_UCAST_PKTS}},
    {SAI_PORT_STAT_IF_OUT_ERRORS, {SWITCH_PORT_COUNTER_ID_OUT_ERROR_PKTS}},
    {SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_OUT_BCAST_PKTS}},
    {SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_OUT_MCAST_PKTS}},
    {SAI_PORT_STAT_IF_IN_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IN_NON_UCAST_PKTS}},

// This a customer specific parameters.
// Later will move this to a patch.
#ifdef PORT_RATE_STAT
    {SAI_PORT_STAT_IF_OUT_OCTETS_RATE,
     {SWITCH_PORT_COUNTER_ID_OUT_OCTETS_RATE}},
    {SAI_PORT_STAT_IF_OUT_PKTS_RATE, {SWITCH_PORT_COUNTER_ID_OUT_PKTS_RATE}},
#endif

    {SAI_PORT_STAT_IF_IN_UNKNOWN_PROTOS, {}},  // Unsupported
    {SAI_PORT_STAT_IF_IN_VLAN_DISCARDS,
     {SWITCH_PORT_COUNTER_ID_IF_IN_VLAN_DISCARDS}},
    {SAI_PORT_STAT_IF_OUT_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_OUT_NON_UCAST_PKTS}},
    {SAI_PORT_STAT_IF_OUT_QLEN, {}},                 // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_DROP_EVENTS, {}},     // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_MULTICAST_PKTS, {}},  // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_BROADCAST_PKTS, {}},  // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_FRAGMENTS,
     {SWITCH_PORT_COUNTER_ID_IN_FRAGMENTS}},
    {SAI_PORT_STAT_ETHER_STATS_PKTS_64_OCTETS, {}},             // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_65_TO_127_OCTETS, {}},      // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_128_TO_255_OCTETS, {}},     // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_256_TO_511_OCTETS, {}},     // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_512_TO_1023_OCTETS, {}},    // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_1024_TO_1518_OCTETS, {}},   // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_1519_TO_2047_OCTETS, {}},   // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_2048_TO_4095_OCTETS, {}},   // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_4096_TO_9216_OCTETS, {}},   // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_PKTS_9217_TO_16383_OCTETS, {}},  // Unsupported
    {SAI_PORT_STAT_ETHER_RX_OVERSIZE_PKTS,
     {SWITCH_PORT_COUNTER_ID_IN_OVER_SIZED_PKTS}},
    {SAI_PORT_STAT_ETHER_TX_OVERSIZE_PKTS,
     {SWITCH_PORT_COUNTER_ID_OUT_OVER_SIZED_PKTS}},
    {SAI_PORT_STAT_ETHER_STATS_JABBERS, {SWITCH_PORT_COUNTER_ID_IN_JABBERS}},
    {SAI_PORT_STAT_ETHER_STATS_OCTETS, {SWITCH_PORT_COUNTER_ID_OCTETS}},
    {SAI_PORT_STAT_ETHER_STATS_PKTS, {SWITCH_PORT_COUNTER_ID_PKTS}},
    {SAI_PORT_STAT_ETHER_STATS_COLLISIONS, {}},  // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_CRC_ALIGN_ERRORS,
     {SWITCH_PORT_COUNTER_ID_IN_CRC_ERRORS}},
    {SAI_PORT_STAT_ETHER_STATS_TX_NO_ERRORS,
     {SWITCH_PORT_COUNTER_ID_OUT_GOOD_PKTS}},
    {SAI_PORT_STAT_ETHER_STATS_RX_NO_ERRORS,
     {SWITCH_PORT_COUNTER_ID_IN_GOOD_PKTS}},
    {SAI_PORT_STAT_IP_IN_RECEIVES, {SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES}},
    {SAI_PORT_STAT_IP_IN_OCTETS, {SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS}},
    {SAI_PORT_STAT_IP_IN_UCAST_PKTS, {SWITCH_PORT_COUNTER_ID_IP_IN_UCAST_PKTS}},
    {SAI_PORT_STAT_IP_IN_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IP_IN_NON_UCAST_PKTS}},
    {SAI_PORT_STAT_IP_IN_DISCARDS, {SWITCH_PORT_COUNTER_ID_IP_IN_DISCARDS}},
    {SAI_PORT_STAT_IP_OUT_OCTETS, {SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS}},
    {SAI_PORT_STAT_IP_OUT_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IP_OUT_UCAST_PKTS}},
    {SAI_PORT_STAT_IP_OUT_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IP_OUT_NON_UCAST_PKTS}},
    {SAI_PORT_STAT_IP_OUT_DISCARDS, {SWITCH_PORT_COUNTER_ID_IP_OUT_DISCARDS}},
    {SAI_PORT_STAT_IPV6_IN_RECEIVES, {SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES}},
    {SAI_PORT_STAT_IPV6_IN_OCTETS, {SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS}},
    {SAI_PORT_STAT_IPV6_IN_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_IN_UCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_IN_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_IN_NON_UCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_IN_MCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_IN_MCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_IN_DISCARDS, {SWITCH_PORT_COUNTER_ID_IPV6_IN_DISCARDS}},
    {SAI_PORT_STAT_IPV6_OUT_OCTETS, {SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS}},
    {SAI_PORT_STAT_IPV6_OUT_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_OUT_UCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_OUT_NON_UCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_OUT_NON_UCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_OUT_MCAST_PKTS,
     {SWITCH_PORT_COUNTER_ID_IPV6_OUT_MCAST_PKTS}},
    {SAI_PORT_STAT_IPV6_OUT_DISCARDS,
     {SWITCH_PORT_COUNTER_ID_IPV6_OUT_DISCARDS}},
    {SAI_PORT_STAT_ETHER_STATS_OVERSIZE_PKTS, {}},  // Unsupported
    {SAI_PORT_STAT_ETHER_STATS_UNDERSIZE_PKTS,
     {SWITCH_PORT_COUNTER_ID_IN_UNDER_SIZED_PKTS}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_64_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_EQ_64}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_65_TO_127_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_65_TO_127}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_128_TO_255_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_128_TO_255}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_256_TO_511_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_256_TO_511}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_512_TO_1023}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_1024_TO_1518_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_1024_TO_1518}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_1519_TO_2047_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_1519_TO_2047}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_2048_TO_4095_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_2048_TO_4095}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_4096_TO_9216_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_4096_TO_8191,
      SWITCH_PORT_COUNTER_ID_IN_PKTS_8192_TO_9215}},
    {SAI_PORT_STAT_ETHER_IN_PKTS_9217_TO_16383_OCTETS,
     {SWITCH_PORT_COUNTER_ID_IN_PKTS_9216}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_64_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_EQ_64}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_65_TO_127_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_65_TO_127}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_128_TO_255_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_128_TO_255}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_256_TO_511_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_256_TO_511}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_512_TO_1023}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_1024_TO_1518_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_1024_TO_1518}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_1519_TO_2047_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_1519_TO_2047}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_2048_TO_4095_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_2048_TO_4095}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_4096_TO_9216_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_4096_TO_8191,
      SWITCH_PORT_COUNTER_ID_OUT_PKTS_8192_TO_9215}},
    {SAI_PORT_STAT_ETHER_OUT_PKTS_9217_TO_16383_OCTETS,
     {SWITCH_PORT_COUNTER_ID_OUT_PKTS_9216}},
    {SAI_PORT_STAT_IN_WATERMARK_BYTES, {}},               // Unsupported
    {SAI_PORT_STAT_IN_SHARED_CURR_OCCUPANCY_BYTES, {}},   // Unsupported
    {SAI_PORT_STAT_IN_SHARED_WATERMARK_BYTES, {}},        // Unsupported
    {SAI_PORT_STAT_OUT_WATERMARK_BYTES, {}},              // Unsupported
    {SAI_PORT_STAT_OUT_SHARED_CURR_OCCUPANCY_BYTES, {}},  // Unsupported
    {SAI_PORT_STAT_OUT_SHARED_WATERMARK_BYTES, {}},       // Unsupported
    {SAI_PORT_STAT_PAUSE_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PAUSE_PKTS}},
    {SAI_PORT_STAT_PAUSE_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PAUSE_PKTS}},
    {SAI_PORT_STAT_PFC_0_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_0_PKTS}},
    {SAI_PORT_STAT_PFC_0_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_0_PKTS}},
    {SAI_PORT_STAT_PFC_1_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_1_PKTS}},
    {SAI_PORT_STAT_PFC_1_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_1_PKTS}},
    {SAI_PORT_STAT_PFC_2_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_2_PKTS}},
    {SAI_PORT_STAT_PFC_2_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_2_PKTS}},
    {SAI_PORT_STAT_PFC_3_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_3_PKTS}},
    {SAI_PORT_STAT_PFC_3_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_3_PKTS}},
    {SAI_PORT_STAT_PFC_4_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_4_PKTS}},
    {SAI_PORT_STAT_PFC_4_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_4_PKTS}},
    {SAI_PORT_STAT_PFC_5_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_5_PKTS}},
    {SAI_PORT_STAT_PFC_5_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_5_PKTS}},
    {SAI_PORT_STAT_PFC_6_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_6_PKTS}},
    {SAI_PORT_STAT_PFC_6_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_6_PKTS}},
    {SAI_PORT_STAT_PFC_7_RX_PKTS, {SWITCH_PORT_COUNTER_ID_IN_PFC_7_PKTS}},
    {SAI_PORT_STAT_PFC_7_TX_PKTS, {SWITCH_PORT_COUNTER_ID_OUT_PFC_7_PKTS}},
    {SAI_PORT_STAT_PFC_0_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_0_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_1_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_1_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_2_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_2_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_3_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_3_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_4_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_4_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_5_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_5_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_6_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_6_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_7_RX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_7_RX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_0_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_0_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_1_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_1_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_2_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_2_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_3_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_3_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_4_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_4_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_5_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_5_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_6_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_6_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_PFC_7_TX_PAUSE_DURATION,
     {SWITCH_PORT_COUNTER_ID_IN_PFC_7_TX_PAUSE_DURATION}},
    {SAI_PORT_STAT_IF_IN_DISCARDS, {SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS}},
    {SAI_PORT_STAT_IF_OUT_DISCARDS, {SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS}},
    {SAI_PORT_STAT_IN_CURR_OCCUPANCY_BYTES,
     {SWITCH_PORT_COUNTER_ID_IN_CURR_OCCUPANCY_BYTES}},
    {SAI_PORT_STAT_OUT_CURR_OCCUPANCY_BYTES,
     {SWITCH_PORT_COUNTER_ID_OUT_CURR_OCCUPANCY_BYTES}},
    {SAI_PORT_STAT_IN_DROPPED_PKTS,
     {SWITCH_PORT_COUNTER_ID_INGRESS_TM_DISCARDS}},
    {SAI_PORT_STAT_OUT_DROPPED_PKTS,
     {SWITCH_PORT_COUNTER_ID_EGRESS_TM_DISCARDS}},
    {SAI_PORT_STAT_GREEN_WRED_DROPPED_PACKETS,
     {SWITCH_PORT_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS}},
    {SAI_PORT_STAT_GREEN_WRED_DROPPED_BYTES,
     {SWITCH_PORT_COUNTER_ID_WRED_GREEN_DROPPED_BYTES}},
    {SAI_PORT_STAT_YELLOW_WRED_DROPPED_PACKETS,
     {SWITCH_PORT_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS}},
    {SAI_PORT_STAT_YELLOW_WRED_DROPPED_BYTES,
     {SWITCH_PORT_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES}},
    {SAI_PORT_STAT_RED_WRED_DROPPED_PACKETS,
     {SWITCH_PORT_COUNTER_ID_WRED_RED_DROPPED_PACKETS}},
    {SAI_PORT_STAT_RED_WRED_DROPPED_BYTES,
     {SWITCH_PORT_COUNTER_ID_WRED_RED_DROPPED_BYTES}},
    {SAI_PORT_STAT_DOT3_STATS_FCS_ERRORS,
     {SWITCH_PORT_COUNTER_ID_IN_FCS_ERRORS}},
    {SAI_PORT_STAT_EEE_TX_EVENT_COUNT, {}},  // Unsupported
    {SAI_PORT_STAT_EEE_RX_EVENT_COUNT, {}},  // Unsupported
    {SAI_PORT_STAT_EEE_TX_DURATION, {}},     // Unsupported
    {SAI_PORT_STAT_EEE_RX_DURATION, {}},     // Unsupported
    {SAI_PORT_STAT_ECN_MARKED_PACKETS,
     {SWITCH_PORT_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS,
      SWITCH_PORT_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS,
      SWITCH_PORT_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS}},
    {SAI_PORT_STAT_WRED_DROPPED_PACKETS,
     {SWITCH_PORT_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS,
      SWITCH_PORT_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS,
      SWITCH_PORT_COUNTER_ID_WRED_RED_DROPPED_PACKETS}},
    {SAI_PORT_STAT_WRED_DROPPED_BYTES,
     {SWITCH_PORT_COUNTER_ID_WRED_GREEN_DROPPED_BYTES,
      SWITCH_PORT_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES,
      SWITCH_PORT_COUNTER_ID_WRED_RED_DROPPED_BYTES}}};

sai_status_t query_port_stats_capability(
    sai_stat_capability_list_t &stats_capability) {
  static const uint16_t supported_count =
      supported_counters_count(port_to_switch_counter_mapping);
  return query_stats_capability_by_mapping(
      port_to_switch_counter_mapping, stats_capability, supported_count);
}

static const std::unordered_map<int32_t, int32_t> port_to_qos_mapping = {
    {SAI_PORT_ATTR_QOS_DOT1P_TO_TC_MAP, SAI_QOS_MAP_TYPE_DOT1P_TO_TC},
    {SAI_PORT_ATTR_QOS_DOT1P_TO_COLOR_MAP, SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR},
    {SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP, SAI_QOS_MAP_TYPE_DSCP_TO_TC},
    {SAI_PORT_ATTR_QOS_DSCP_TO_COLOR_MAP, SAI_QOS_MAP_TYPE_DSCP_TO_COLOR},
    {SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP, SAI_QOS_MAP_TYPE_TC_TO_QUEUE},
    {SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP,
     SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP},
    {SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP,
     SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP},
    {SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP,
     SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP},
    {SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP,
     SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P},
    {SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP,
     SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE},
};

static sai_status_t port_qos_map_get(sai_object_id_t port_id,
                                     sai_attribute_t &attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = port_id};
  switch_attr_id_t qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP;

  attr.value.oid = SAI_NULL_OBJECT_ID;

  switch (attr.id) {
    case SAI_PORT_ATTR_QOS_DOT1P_TO_COLOR_MAP:
    case SAI_PORT_ATTR_QOS_DOT1P_TO_TC_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
    case SAI_PORT_ATTR_QOS_DSCP_TO_COLOR_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
    case SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_TC_QOS_MAP_HANDLE;
      break;
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_ICOS_PPG_QOS_MAP_HANDLE;
      break;
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_PFC_PRIORITY_QUEUE_QOS_MAP_HANDLE;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }

  attr_w qos_attr(qos_attr_id);
  switch_status = bf_switch_attribute_get(sw_object_id, qos_attr_id, qos_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map OID: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_id_t oid = {};
  qos_attr.v_get(oid);

  if (oid.data == SAI_NULL_OBJECT_ID) {
    return SAI_STATUS_SUCCESS;
  }

  sai_object_id_t qos_map_id = oid.data;
  sai_attribute_t qos_map_attr = {.id = SAI_QOS_MAP_ATTR_TYPE};
  status = sai_get_qos_map_attribute(qos_map_id, 1, &qos_map_attr);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  auto it = port_to_qos_mapping.find(attr.id);
  if (it != port_to_qos_mapping.end()) {
    if (qos_map_attr.value.s32 == it->second) {
      attr.value.oid = oid.data;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    SAI_LOG_ERROR("Port attribute parse failed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  return SAI_STATUS_SUCCESS;
}

static sai_status_t port_qos_map_update(sai_object_id_t port_id,
                                        const sai_attribute_t &attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = port_id};
  sai_attribute_t port_qos_attr = {.id = attr.id};
  switch_attr_id_t qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP;

  if (attr.value.oid != SAI_NULL_OBJECT_ID) {
    status = port_qos_map_get(port_id, port_qos_attr);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set QoS map on Port: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    if (port_qos_attr.value.oid != SAI_NULL_OBJECT_ID) {
      sai_object_id_t qos_map_id = port_qos_attr.value.oid;
      sai_attribute_t qos_map_attr = {.id = SAI_QOS_MAP_ATTR_TYPE};
      status = sai_get_qos_map_attribute(qos_map_id, 1, &qos_map_attr);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get QoS map type: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }

      auto it = port_to_qos_mapping.find(attr.id);
      if (it != port_to_qos_mapping.end()) {
        if (qos_map_attr.value.s32 != it->second) {
          status = SAI_STATUS_INSUFFICIENT_RESOURCES;
          SAI_LOG_ERROR("Port QoS mapping already set: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
      } else {
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR("Port attribute parse failed: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  switch (attr.id) {
    case SAI_PORT_ATTR_QOS_DOT1P_TO_COLOR_MAP:
    case SAI_PORT_ATTR_QOS_DOT1P_TO_TC_MAP: {
      qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP;
      break;
    }
    case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
    case SAI_PORT_ATTR_QOS_DSCP_TO_COLOR_MAP: {
      qos_attr_id = SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP;
      break;
    }
    case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
    case SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_TC_QOS_MAP_HANDLE;
      break;
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_ICOS_PPG_QOS_MAP_HANDLE;
      break;
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP;
      break;
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP:
      qos_attr_id = SWITCH_PORT_ATTR_PFC_PRIORITY_QUEUE_QOS_MAP_HANDLE;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t qos_group = {.data = attr.value.oid};
  switch_status =
      bf_switch_attribute_set(sw_object_id, attr_w(qos_attr_id, qos_group));
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set QoS for port 0x%" PRIx64 ": %s",
                  port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  return SAI_STATUS_SUCCESS;
}

static void port_serdes_object_get(switch_object_id_t sw_port_id,
                                   switch_object_id_t &serdes_id) {
  switch_status_t sw_status;
  std::set<switch_object_id_t> serdes;

  sw_status = switch_store::referencing_set_get(
      sw_port_id, SWITCH_OBJECT_TYPE_PORT_SERDES, serdes);
  if (sw_status != SWITCH_STATUS_SUCCESS) return;

  if (serdes.size() > 0) {
    serdes_id = *(serdes.begin());
  }
}

sai_status_t port_attr_autoneg_apply(const sai_attribute_t *attr,
                                     sai_object_id_t port_id) {
  switch_enum_t autoneg = {0};
  switch_object_id_t oid = {.data = port_id};
  autoneg.enumdata = attr->value.booldata ? SWITCH_PORT_ATTR_AUTONEG_ENABLED
                                          : SWITCH_PORT_ATTR_AUTONEG_DISABLED;
  switch_status_t switch_status =
      bf_switch_attribute_set(oid, attr_w(SWITCH_PORT_ATTR_AUTONEG, autoneg));
  sai_status_t status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "autoneg %u : %s",
                  port_id,
                  attr->value.booldata,
                  sai_metadata_get_status_name(status));
  }
  return status;
}

/*
 * Routine Description:
 *   Set port attribute value.
 *
 * Arguments:
 *    [in] port_id - port id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_port_attribute(sai_object_id_t port_id,
                                    const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;

  switch_object_id_t oid = {.data = port_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_PORT_ATTR_MTU: {
      uint32_t mtu = attr->value.u32;
      switch_status =
          bf_switch_attribute_set(oid, attr_w(SWITCH_PORT_ATTR_RX_MTU, mtu));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "Rx MTU %u: %s",
                      port_id,
                      mtu,
                      sai_metadata_get_status_name(status));
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      switch_status =
          bf_switch_attribute_set(oid, attr_w(SWITCH_PORT_ATTR_TX_MTU, mtu));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "Tx MTU %u: %s",
                      port_id,
                      mtu,
                      sai_metadata_get_status_name(status));
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      break;
    }
    case SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL: {
      switch_enum_t pfc = {.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_NONE};
      switch_enum_t pfc_mode = {.enumdata = SWITCH_PORT_ATTR_PFC_MODE_COMBINED};
      attr_w pfc_mode_attr(SWITCH_PORT_ATTR_PFC_MODE);
      uint32_t pfc_map = attr->value.u8;

      switch_status = bf_switch_attribute_get(
          oid, SWITCH_PORT_ATTR_PFC_MODE, pfc_mode_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "pfc mode: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }

      pfc_mode_attr.v_get(pfc_mode);
      if (pfc_mode.enumdata != SWITCH_PORT_ATTR_PFC_MODE_COMBINED) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64
                      "flow control: %s, pfc mode is not combined",
                      port_id,
                      sai_metadata_get_status_name(status));
        return SAI_STATUS_NOT_SUPPORTED;
      }

      if (pfc_map) {
        pfc.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_PFC;
      }

      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_ATTR_FLOW_CONTROL, pfc));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "flow control: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }

      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_ATTR_PFC_MAP, pfc_map));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "pfc map: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      switch_status = bf_switch_attribute_set(
          oid,
          attr_w(SWITCH_PORT_ATTR_PFC_COS_MAP, static_cast<uint8_t>(pfc_map)));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "pfc cos map: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_RX:
    case SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_TX: {
      switch_enum_t pfc = {.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_NONE};
      switch_enum_t pfc_mode = {.enumdata = SWITCH_PORT_ATTR_PFC_MODE_COMBINED};
      attr_w pfc_mode_attr(SWITCH_PORT_ATTR_PFC_MODE);
      uint32_t pfc_map = attr->value.u8;
      switch_attr_id_t attr_id =
          (attr->id == SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_RX)
              ? SWITCH_PORT_ATTR_RX_PFC_MAP
              : SWITCH_PORT_ATTR_TX_PFC_MAP;

      switch_status = bf_switch_attribute_get(
          oid, SWITCH_PORT_ATTR_PFC_MODE, pfc_mode_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "pfc mode: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }

      pfc_mode_attr.v_get(pfc_mode);
      if (pfc_mode.enumdata != SWITCH_PORT_ATTR_PFC_MODE_SEPARATE) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64
                      "rx/tx flow control: %s, pfc mode is not separate",
                      port_id,
                      sai_metadata_get_status_name(status));
        return SAI_STATUS_NOT_SUPPORTED;
      }

      if (pfc_map) {
        pfc.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_PFC;
      }

      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_ATTR_FLOW_CONTROL, pfc));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "flow control: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }

      switch_status = bf_switch_attribute_set(oid, attr_w(attr_id, pfc_map));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "rx/tx pfc map: %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_ATTR_INGRESS_MIRROR_SESSION:
    case SAI_PORT_ATTR_EGRESS_MIRROR_SESSION: {
      if (attr->value.objlist.count > 1) {
        SAI_LOG_ERROR("unsupported number of mirror sessions %u",
                      attr->value.objlist.count);
        return SAI_STATUS_INSUFFICIENT_RESOURCES;
      }
      uint64_t session_type = (attr->id == SAI_PORT_ATTR_INGRESS_MIRROR_SESSION)
                                  ? SWITCH_PORT_ATTR_INGRESS_MIRROR_HANDLE
                                  : SWITCH_PORT_ATTR_EGRESS_MIRROR_HANDLE;
      switch_object_id_t mirror_handle = {
          .data = (attr->value.objlist.count > 0) ? attr->value.objlist.list[0]
                                                  : SAI_NULL_OBJECT_ID};
      switch_status =
          bf_switch_attribute_set(oid, attr_w(session_type, mirror_handle));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set mirror session for port 0x%" PRIx64 ": %s",
                      port_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_ATTR_QOS_DOT1P_TO_COLOR_MAP:
    case SAI_PORT_ATTR_QOS_DOT1P_TO_TC_MAP:
    case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
    case SAI_PORT_ATTR_QOS_DSCP_TO_COLOR_MAP:
    case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP:
    case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP:
    case SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP:
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP:
    case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP:
      status = port_qos_map_update(port_id, *attr);
      break;
    case SAI_PORT_ATTR_AUTO_NEG_MODE: {
      status = port_attr_autoneg_apply(attr, port_id);
      break;
    }
    case SAI_PORT_ATTR_ADVERTISED_MEDIA_TYPE:  // Unsupported
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
    case SAI_PORT_ATTR_SERDES_PREEMPHASIS:  // Unsupported required for blue
      break;
    case SAI_PORT_ATTR_PORT_VLAN_ID:
    case SAI_PORT_ATTR_DROP_TAGGED:
    case SAI_PORT_ATTR_DROP_UNTAGGED: {
      std::set<switch_object_id_t> lag_members;

      status = switch_store::referencing_set_get(
          oid, SWITCH_OBJECT_TYPE_LAG_MEMBER, lag_members);

      if (lag_members.size()) {
        status = SAI_STATUS_NOT_EXECUTED;
        SAI_LOG_ERROR(
            "Failed to set port attribute when port is lag member %s error: "
            "%s",
            sai_attribute_name(sai_ot, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      status = sai_to_switch_attribute_set(sai_ot, attr, oid);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(sai_ot, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_ATTR_ADVERTISED_SPEED:
    case SAI_PORT_ATTR_SPEED:
    case SAI_PORT_ATTR_ADVERTISED_FEC_MODE:
    default:
      status = sai_to_switch_attribute_set(sai_ot, attr, oid);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(sai_ot, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }
  return status;
}

/*
 * Routine Description:
 *   Get port list-attribute elements.
 *
 * Arguments:
 *    [in] port_id - port id
 *    [in] attr_id - switch attribute ID
 *    [out] oids - list elements
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_get_port_list_attr(
    switch_object_id_t port_id,
    switch_attr_id_t attr_id,
    std::vector<switch_object_id_t> &oids) {
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  attr_w sw_attr(attr_id);

  sw_status = bf_switch_attribute_get(port_id, attr_id, sw_attr);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return status_switch_to_sai(sw_status);
  }

  sw_attr.v_get(oids);

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get port list-attribute elements count.
 *
 * Arguments:
 *    [in] port_id - port id
 *    [in] attr_id - switch attribute ID
 *    [out] count - count of list elements
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_get_port_list_attr_count(switch_object_id_t port_id,
                                                 switch_attr_id_t attr_id,
                                                 uint32_t &count) {
  std::vector<switch_object_id_t> oids;
  sai_status_t status = sai_get_port_list_attr(port_id, attr_id, oids);

  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  count = oids.size();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get list port priority groups.
 *
 * Arguments:
 *    [in] port_id - port id
 *    [out] ppgs - list of PPGs
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_get_port_ppg_list(
    switch_object_id_t port_id, std::vector<switch_object_id_t> &ppgs) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> oids;
  switch_object_id_t default_ppg = {};
  attr_w sw_attr(SWITCH_PORT_ATTR_DEFAULT_PPG);

  status = sai_get_port_list_attr(
      port_id, SWITCH_PORT_ATTR_PORT_PRIORITY_GROUPS, oids);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  sw_status =
      bf_switch_attribute_get(port_id, SWITCH_PORT_ATTR_DEFAULT_PPG, sw_attr);
  if (sw_status != SWITCH_STATUS_SUCCESS) {
    return status_switch_to_sai(sw_status);
  }

  sw_attr.v_get(default_ppg.data);
  ppgs.push_back(default_ppg);

  // PPG list includes both default and non-default PPGs.
  // Sonic uses PPG 0 in the list to query for stats when there is no default
  // TC to PPG mapping. PPG 0 will be always default PPG.
  for (auto ppg : oids) {
    if (ppg.data != default_ppg.data) {
      ppgs.push_back(ppg);
    }
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get port attribute value.
 *
 * Arguments:
 *    [in] port_id - port id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_port_attribute(sai_object_id_t port_id,
                                    uint32_t attr_count,
                                    sai_attribute_t *attr_list) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  attr_w lane_attr(SWITCH_PORT_ATTR_LANE_LIST);
  size_t cnt = 0;

  SAI_LOG_ENTER();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  const switch_object_id_t sw_object_id = {.data = port_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_PORT_ATTR_CURRENT_BREAKOUT_MODE_TYPE:
      case SAI_PORT_ATTR_HW_LANE_LIST: {
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_LANE_LIST, lane_attr);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port %" PRIu64 "lane list", port_id);
          return status;
        }
        std::vector<uint16_t> lane_list;
        lane_attr.v_get(lane_list);
        cnt = lane_list.size();
        if (attr_list[i].id == SAI_PORT_ATTR_HW_LANE_LIST) {
          TRY_LIST_SET(attr_list[i].value.u32list, lane_list);
        } else {
          if (cnt == 1) {
            attr_list[i].value.s32 = SAI_PORT_BREAKOUT_MODE_TYPE_1_LANE;
          } else if (cnt == 2) {
            attr_list[i].value.s32 = SAI_PORT_BREAKOUT_MODE_TYPE_2_LANE;
          } else {
            attr_list[i].value.s32 = SAI_PORT_BREAKOUT_MODE_TYPE_4_LANE;
          }
        }
        break;
      }
      case SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL: {
        switch_enum_t pfc = {.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_NONE};
        attr_w flow_ctrl_attr(SWITCH_PORT_ATTR_FLOW_CONTROL);
        uint32_t pfc_map = 0;

        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_FLOW_CONTROL, flow_ctrl_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "flow control: %s",
                        port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }

        flow_ctrl_attr.v_get(pfc);
        if (pfc.enumdata == SWITCH_PORT_ATTR_FLOW_CONTROL_PFC) {
          attr_w pfc_attr(SWITCH_PORT_ATTR_PFC_MAP);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_PORT_ATTR_PFC_MAP, pfc_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "pfc map: %s",
                          port_id,
                          sai_metadata_get_status_name(status));
            return status;
          }
          pfc_attr.v_get(pfc_map);
        }

        attr_list[i].value.u8 = (uint8_t)pfc_map;
        break;
      }
      case SAI_PORT_ATTR_SUPPORTED_BREAKOUT_MODE_TYPE: {
        static const std::vector<int32_t> list{
            SAI_PORT_BREAKOUT_MODE_TYPE_4_LANE,
            SAI_PORT_BREAKOUT_MODE_TYPE_2_LANE,
            SAI_PORT_BREAKOUT_MODE_TYPE_1_LANE};
        TRY_LIST_SET(attr_list[i].value.s32list, list);
        break;
      }
      case SAI_PORT_ATTR_MTU: {
        uint32_t rx_mtu = 0;
        uint32_t tx_mtu = 0;
        attr_w rx_mtu_attr(SWITCH_PORT_ATTR_RX_MTU);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_RX_MTU, rx_mtu_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "Rx MTU: %s",
                        port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_w tx_mtu_attr(SWITCH_PORT_ATTR_TX_MTU);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_TX_MTU, tx_mtu_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "Tx MTU: %s",
                        port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        rx_mtu_attr.v_get(rx_mtu);
        tx_mtu_attr.v_get(tx_mtu);

        attr_list[i].value.u32 = (rx_mtu < tx_mtu) ? rx_mtu : tx_mtu;
        break;
      }
      case SAI_PORT_ATTR_INGRESS_MIRROR_SESSION:
      case SAI_PORT_ATTR_EGRESS_MIRROR_SESSION: {
        uint64_t session_type =
            (attr_list[i].id == SAI_PORT_ATTR_INGRESS_MIRROR_SESSION)
                ? SWITCH_PORT_ATTR_INGRESS_MIRROR_HANDLE
                : SWITCH_PORT_ATTR_EGRESS_MIRROR_HANDLE;
        attr_w mirror_attr(session_type);
        switch_object_id_t mirror_handle = {0};
        switch_status =
            bf_switch_attribute_get(sw_object_id, session_type, mirror_attr);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get mirror session handle: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        mirror_attr.v_get(mirror_handle);
        std::vector<sai_object_id_t> list;
        if (mirror_handle.data != SAI_NULL_OBJECT_ID) {
          list.push_back(mirror_handle.data);
        }
        TRY_LIST_SET(attr_list[i].value.objlist, list);
        break;
      }
      case SAI_PORT_ATTR_NUMBER_OF_INGRESS_PRIORITY_GROUPS: {
        std::vector<switch_object_id_t> ppgs;
        status = sai_get_port_ppg_list(sw_object_id, ppgs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port %" PRIu64 " port priority groups",
                        port_id);
          return status;
        }
        attr_list[i].value.u32 = ppgs.size();
        break;
      }
      case SAI_PORT_ATTR_INGRESS_PRIORITY_GROUP_LIST: {
        std::vector<switch_object_id_t> ppgs;
        status = sai_get_port_ppg_list(sw_object_id, ppgs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port %" PRIu64 " port priority groups",
                        port_id);
          return status;
        }
        TRY_LIST_SET(attr_list[i].value.objlist, ppgs);
        break;
      }
      case SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES:
        status = sai_get_port_list_attr_count(sw_object_id,
                                              SWITCH_PORT_ATTR_QUEUE_HANDLES,
                                              attr_list[i].value.u32);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port %" PRIu64 " QOS queue handles",
                        port_id);
          return status;
        }
        break;
      case SAI_PORT_ATTR_QOS_NUMBER_OF_SCHEDULER_GROUPS:
        status = sai_get_port_list_attr_count(
            sw_object_id,
            SWITCH_PORT_ATTR_PORT_QUEUE_SCHEDULER_GROUP_HANDLES,
            attr_list[i].value.u32);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port %" PRIu64
                        " scheduler group handles",
                        port_id);
          return status;
        }
        break;
      case SAI_PORT_ATTR_QOS_INGRESS_BUFFER_PROFILE_LIST:
      case SAI_PORT_ATTR_QOS_EGRESS_BUFFER_PROFILE_LIST:
        attr_list[i].value.objlist.count = 0;
        break;
      case SAI_PORT_ATTR_EGRESS_SAMPLEPACKET_ENABLE:
        attr_list[i].value.oid = 0;
        break;
      case SAI_PORT_ATTR_QOS_DOT1P_TO_COLOR_MAP:
      case SAI_PORT_ATTR_QOS_DOT1P_TO_TC_MAP:
      case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
      case SAI_PORT_ATTR_QOS_DSCP_TO_COLOR_MAP:
      case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
      case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP:
      case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP:
      case SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP:
      case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP:
      case SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP:
        status = port_qos_map_get(port_id, attr_list[i]);
        break;
      case SAI_PORT_ATTR_POLICER_ID:                            // Unsupported
      case SAI_PORT_ATTR_PORT_POOL_LIST:                        // Unsupported
      case SAI_PORT_ATTR_EGRESS_BLOCK_PORT_LIST:                // Unsupported
      case SAI_PORT_ATTR_TAM_OBJECT:                            // Unsupported
      case SAI_PORT_ATTR_INGRESS_MACSEC_ACL:                    // Unsupported
      case SAI_PORT_ATTR_EGRESS_MACSEC_ACL:                     // Unsupported
      case SAI_PORT_ATTR_MACSEC_PORT_LIST:                      // Unsupported
      case SAI_PORT_ATTR_INGRESS_SAMPLE_MIRROR_SESSION:         // Unsupported
      case SAI_PORT_ATTR_EGRESS_SAMPLE_MIRROR_SESSION:          // Unsupported
      case SAI_PORT_ATTR_QOS_MPLS_EXP_TO_TC_MAP:                // Unsupported
      case SAI_PORT_ATTR_QOS_MPLS_EXP_TO_COLOR_MAP:             // Unsupported
      case SAI_PORT_ATTR_QOS_TC_AND_COLOR_TO_MPLS_EXP_MAP:      // Unsupported
      case SAI_PORT_ATTR_SYSTEM_PORT:                           // Unsupported
      case SAI_PORT_ATTR_QOS_DSCP_TO_FORWARDING_CLASS_MAP:      // Unsupported
      case SAI_PORT_ATTR_QOS_MPLS_EXP_TO_FORWARDING_CLASS_MAP:  // Unsupported
      case SAI_PORT_ATTR_IPSEC_PORT:                            // Unsupported
      case SAI_PORT_ATTR_REMOTE_ADVERTISED_SPEED:               // Unsupported
      case SAI_PORT_ATTR_ADVERTISED_MEDIA_TYPE:                 // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_PORT_ATTR_SUPPORTED_AUTO_NEG_MODE: {
        switch_enum_t port_type = {.enumdata = SWITCH_PORT_ATTR_TYPE_MAX};
        attr_w port_attr_type(SWITCH_PORT_ATTR_TYPE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_TYPE, port_attr_type);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "port type : %s",
                        port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        port_attr_type.v_get(port_type);
        attr_list[i].value.booldata =
            (port_type.enumdata == SWITCH_PORT_ATTR_TYPE_NORMAL);
        break;
      }
      case SAI_PORT_ATTR_AUTO_NEG_MODE: {
        switch_enum_t autoneg = {.enumdata = SWITCH_PORT_ATTR_AUTONEG_DISABLED};
        attr_w port_attr(SWITCH_PORT_ATTR_AUTONEG);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_PORT_ATTR_AUTONEG, port_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "autoneg : %s",
                        port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        port_attr.v_get(autoneg);
        attr_list[i].value.booldata =
            (autoneg.enumdata == SWITCH_PORT_ATTR_AUTONEG_DISABLED) ? false
                                                                    : true;
        break;
      }
      case SAI_PORT_ATTR_PORT_SERDES_ID: {
        switch_object_id_t serdes_id = {0};
        port_serdes_object_get(sw_object_id, serdes_id);
        attr_list[i].value.oid = (sai_object_id_t)serdes_id.data;
      } break;
      case SAI_PORT_ATTR_ADVERTISED_SPEED:
      case SAI_PORT_ATTR_SUPPORTED_SPEED:
      case SAI_PORT_ATTR_SPEED:
      default:
        status =
            sai_to_switch_attribute_get(sai_ot, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS &&
            status != SAI_STATUS_BUFFER_OVERFLOW) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s",
                        sai_attribute_name(sai_ot, attr_list[i].id),
                        sai_metadata_get_status_name(status));
        }
        break;
    }
  }

  return status;
}

sai_status_t sai_create_port(sai_object_id_t *port_id,
                             sai_object_id_t switch_id,
                             uint32_t attr_count,
                             const sai_attribute_t *attr_list) {
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_PORT;

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bool admin_state = false;
  switch_enum_t port_type = {0};
  switch_object_id_t sw_object_id = {};
  std::set<attr_w> sw_attrs;
  const sai_attribute_t *attribute;

  if (!port_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *port_id = SAI_NULL_OBJECT_ID;

  attribute =
      sai_get_attr_from_list(SAI_PORT_ATTR_HW_LANE_LIST, attr_list, attr_count);
  if (attribute == NULL) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("missing attribute %s", sai_metadata_get_status_name(status));
    return status;
  }

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL:
        if (attr_list[i].value.u8) {
          switch_enum_t pfc = {.enumdata = SWITCH_PORT_ATTR_FLOW_CONTROL_PFC};
          uint32_t pfc_map = attr_list[i].value.u8;
          sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_FLOW_CONTROL, pfc));
          sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_PFC_MAP, pfc_map));
          sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_PFC_COS_MAP,
                                 static_cast<uint8_t>(pfc_map)));
        }
        break;
      case SAI_PORT_ATTR_MTU:
        sw_attrs.insert(
            attr_w(SWITCH_PORT_ATTR_RX_MTU, attr_list[i].value.u32));
        sw_attrs.insert(
            attr_w(SWITCH_PORT_ATTR_TX_MTU, attr_list[i].value.u32));
        break;
      case SAI_PORT_ATTR_INGRESS_MIRROR_SESSION:
      case SAI_PORT_ATTR_EGRESS_MIRROR_SESSION: {
        if (attr_list[i].value.objlist.count > 1) {
          SAI_LOG_ERROR("Unsupported number of mirror sessions %u",
                        attr_list[i].value.objlist.count);
          return SAI_STATUS_INSUFFICIENT_RESOURCES;
        }
        uint64_t session_type =
            (attr_list[i].id == SAI_PORT_ATTR_INGRESS_MIRROR_SESSION)
                ? SWITCH_PORT_ATTR_INGRESS_MIRROR_HANDLE
                : SWITCH_PORT_ATTR_EGRESS_MIRROR_HANDLE;
        if (attr_list[i].value.objlist.count > 0) {
          switch_object_id_t mirror_handle = {
              .data = attr_list[i].value.objlist.list[0]};
          sw_attrs.insert(attr_w(session_type, mirror_handle));
        }
        break;
      }
      case SAI_PORT_ATTR_ADMIN_STATE:
        admin_state = attr_list[i].value.booldata;
        break;
      case SAI_PORT_ATTR_AUTO_NEG_MODE: {
        switch_enum_t autoneg = {0};
        autoneg.enumdata = attr_list[i].value.booldata
                               ? SWITCH_PORT_ATTR_AUTONEG_ENABLED
                               : SWITCH_PORT_ATTR_AUTONEG_DISABLED;
        sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_AUTONEG, autoneg));
        break;
      }
      case SAI_PORT_ATTR_ADVERTISED_MEDIA_TYPE:
        SAI_LOG_WARN(
            "SAI_PORT_ATTR_ADVERTISED_MEDIA_TYPE not supported - ignoring");
        break;
      case SAI_PORT_ATTR_SPEED:
      case SAI_PORT_ATTR_HW_LANE_LIST:
      default:
        status = sai_to_switch_attribute(sai_ot, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to convert SAI attribute: %s, status %s",
                        sai_attribute_name(sai_ot, attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  port_type.enumdata = SWITCH_PORT_ATTR_TYPE_NORMAL;
  sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_TYPE, port_type));

  sai_insert_device_attribute(0, SWITCH_PORT_ATTR_DEVICE, sw_attrs);
  sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_ADMIN_STATE, admin_state));

  // set the non-def ppg counts
  attr_w non_def_ppg_attr(SWITCH_DEVICE_ATTR_NUM_NON_DEFAULT_PPGS);
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_NUM_NON_DEFAULT_PPGS, non_def_ppg_attr);
  uint32_t non_def_ppgs = 0;
  non_def_ppg_attr.v_get(non_def_ppgs);
  sw_attrs.insert(attr_w(SWITCH_PORT_ATTR_NON_DEF_PPGS, non_def_ppgs));

  // set port QOS config precedence to true so that the port QOS config will be
  // used when port becomes LAG member
  bool qos_config_precedence = true;
  sw_attrs.insert(
      attr_w(SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, qos_config_precedence));

  switch_status = bf_switch_object_create(ot, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create port, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  attr_w attr(SWITCH_DEVICE_ATTR_NUM_PORTS);
  uint32_t num_ports = 0;
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_NUM_PORTS, attr);
  attr.v_get(num_ports);
  num_ports++;
  attr.v_set(num_ports);
  switch_status = bf_switch_attribute_set(device_handle, attr);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to update switch port count, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *port_id = sw_object_id.data;
  return status;
}

sai_status_t sai_remove_port(sai_object_id_t port_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  const switch_object_id_t sw_object_id = {.data = port_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove port 0x%" PRIx64 ", status %s",
                  port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  attr_w attr(SWITCH_DEVICE_ATTR_NUM_PORTS);
  uint32_t num_ports = 0;
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_NUM_PORTS, attr);
  attr.v_get(num_ports);
  num_ports--;
  attr.v_set(num_ports);
  switch_status = bf_switch_attribute_set(device_handle, attr);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to update switch port count, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  return status;
}

sai_status_t sai_get_port_stats(sai_object_id_t port_id,
                                uint32_t number_of_counters,
                                const sai_stat_id_t *counter_ids,
                                uint64_t *counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_object_id = {.data = port_id};
  switch_status = bf_switch_counters_get(sw_object_id, cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get port stats 0x%" PRIx64 ", status %s",
                  port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!cntrs.size()) {
    memset(counters, 0, number_of_counters * sizeof(uint64_t));
    return SAI_STATUS_SUCCESS;
  }

  for (uint32_t i = 0; i < number_of_counters; i++) {
    counters[i] = get_counters_count_by_id(
        port_to_switch_counter_mapping, counter_ids[i], cntrs);
  }

  return status;
}

sai_status_t sai_clear_port_all_stats(sai_object_id_t port_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;

  const switch_object_id_t sw_object_id = {.data = port_id};

  switch_status = bf_switch_counters_clear_all(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to clear port all stats 0x%" PRIx64 ", status %s",
                  port_id,
                  sai_metadata_get_status_name(status));
  }

  return status;
}

sai_status_t sai_clear_port_stats(sai_object_id_t port_id,
                                  uint32_t number_of_counters,
                                  const sai_stat_id_t *counter_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::list<uint16_t> cntr_ids;

  if (!counter_ids) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(port_id) != SAI_OBJECT_TYPE_PORT) {
    SAI_LOG_ERROR("Port stats clear failed: invalid port handle 0x%" PRIx64,
                  port_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = port_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    const auto counters_it =
        port_to_switch_counter_mapping.find(counter_ids[i]);
    if (counters_it != port_to_switch_counter_mapping.end() &&
        !counters_it->second.empty()) {
      cntr_ids.insert(cntr_ids.end(),
                      counters_it->second.begin(),
                      counters_it->second.end());
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(
        sw_object_id, std::vector<uint16_t>(cntr_ids.begin(), cntr_ids.end()));
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear port 0x%" PRIx64 " stats, status %s",
                    port_id,
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_port_stats_ext(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t *counter_ids,  // list of counters
    sai_stats_mode_t mode,             // supports READ only
    uint64_t *counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = port_id};
  std::vector<switch_counter_t> cntrs;
  switch_object_id_t debug_counter_handle;
  switch_enum_t sw_dc_type;
  uint32_t index = 0;

  switch_status = bf_switch_counters_get(sw_object_id, cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get port stats 0x%" PRIx64 ", status %s",
                  port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!cntrs.size()) {
    memset(counters, 0, number_of_counters * sizeof(uint64_t));
    return SAI_STATUS_SUCCESS;
  }
  std::set<switch_object_id_t> debug_counter_list;
  for (uint32_t i = 0; i < number_of_counters; i++) {
    if (counter_ids[i] < SAI_PORT_STAT_IN_DROP_REASON_RANGE_BASE) {
      // process non Debug Counter
      counters[i] = get_counters_count_by_id(
          port_to_switch_counter_mapping, counter_ids[i], cntrs);

      // for non DebugCounter further processing not needed
      continue;

    } else if (counter_ids[i] < SAI_PORT_STAT_IN_DROP_REASON_RANGE_END) {
      index = counter_ids[i] - SAI_PORT_STAT_IN_DROP_REASON_RANGE_BASE;
    } else if (counter_ids[i] >= SAI_PORT_STAT_OUT_DROP_REASON_RANGE_BASE &&
               counter_ids[i] < SAI_PORT_STAT_OUT_DROP_REASON_RANGE_END) {
      index = counter_ids[i] - SAI_PORT_STAT_OUT_DROP_REASON_RANGE_BASE;
    } else if (counter_ids[i] >= SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE &&
               counter_ids[i] < SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_END) {
      index = counter_ids[i] - SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE;
    } else if (counter_ids[i] >= SAI_SWITCH_STAT_OUT_DROP_REASON_RANGE_BASE &&
               counter_ids[i] < SAI_SWITCH_STAT_OUT_DROP_REASON_RANGE_END) {
      index = counter_ids[i] - SAI_SWITCH_STAT_OUT_DROP_REASON_RANGE_BASE;
    } else {
      SAI_LOG_ERROR("port stats ext invalid index value %d", index);
      return SAI_STATUS_INVALID_ATTR_VALUE_0;
    }
    // find debug counter object for given index
    debug_counter_handle =
        switch_store::id_to_handle(SWITCH_OBJECT_TYPE_DEBUG_COUNTER, index);
    switch_object_type_t ot =
        switch_store::object_type_query(debug_counter_handle);
    if (ot != SWITCH_OBJECT_TYPE_DEBUG_COUNTER) {
      SAI_LOG_ERROR("Invalid Debug Counter object id 0x%" PRIx64,
                    debug_counter_handle.data);
      return SAI_STATUS_INVALID_OBJECT_TYPE;
    }
    attr_w type_attr(SWITCH_DEBUG_COUNTER_ATTR_TYPE);
    switch_status = bf_switch_attribute_get(
        debug_counter_handle, SWITCH_DEBUG_COUNTER_ATTR_TYPE, type_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get debug counter : %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    type_attr.v_get(sw_dc_type);
    smi::attr_w drop_counters_attr(
        SWITCH_DEBUG_COUNTER_ATTR_DROP_COUNTERS_LIST);
    switch_status =
        bf_switch_attribute_get(debug_counter_handle,
                                SWITCH_DEBUG_COUNTER_ATTR_DROP_COUNTERS_LIST,
                                drop_counters_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get Debug Counter list: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    if (drop_counters_attr.type_get() != SWITCH_TYPE_LIST) {
      SAI_LOG_ERROR("Invalid SMI attribute type %u",
                    drop_counters_attr.type_get());
      return SAI_STATUS_FAILURE;
    }
    std::vector<uint16_t> unique_cntr_ids;
    drop_counters_attr.v_get(unique_cntr_ids);
    counters[i] = 0;

    for (auto counter_id : unique_cntr_ids) {
      if (cntrs[counter_id].count) {
        // count each drop reason only once per debug counter
        counters[i] += cntrs[counter_id].count;
      }
    }

    if (mode == SAI_STATS_MODE_READ_AND_CLEAR && unique_cntr_ids.size()) {
      switch_status = bf_switch_counters_clear(sw_object_id, unique_cntr_ids);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to clear port 0x%" PRIx64 " stats, status %s",
                      port_id,
                      sai_metadata_get_status_name(status));
      }
    }
  }  // end of for (uint32_t i = 0; i < number_of_counters; i++) {

  return status;
}
sai_status_t sai_create_port_serdes(sai_object_id_t *port_serdes_id,
                                    sai_object_id_t switch_id,
                                    uint32_t attr_count,
                                    const sai_attribute_t *attr_list) {
  const sai_object_type_t sai_ot_ps = SAI_OBJECT_TYPE_PORT_SERDES;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_PORT_SERDES;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> sw_attrs;
  std::vector<int64_t> port_lane_values;
  const sai_attribute_t *attribute = NULL;
  switch_object_id_t sw_object_id = {};
  if (!port_serdes_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *port_serdes_id = SAI_NULL_OBJECT_ID;
  for (unsigned i = 0; i < attr_count; i++) {
    attribute = &attr_list[i];
    port_lane_values.clear();
    switch (attribute->id) {
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE1, port_lane_values));
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE2, port_lane_values));
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_MAIN, port_lane_values));
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST1: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST1, port_lane_values));
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST2: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST2, port_lane_values));
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_ATTN: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int64_t v = attribute->value.s32list.list[idx];
          port_lane_values.push_back(v);
        }
        sw_attrs.insert(
            attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_ATTN, port_lane_values));
        break;
      }
      default:
        status = sai_to_switch_attribute(sai_ot_ps, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to convert SAI attribute: %s, status %s",
                        sai_attribute_name(sai_ot_ps, attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }
  switch_status = bf_switch_object_create(ot, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create port, status %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  *port_serdes_id = sw_object_id.data;
  return status;
}
sai_status_t sai_remove_port_serdes(sai_object_id_t port_serdes_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  const switch_object_id_t sw_object_id = {.data = port_serdes_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove port_serdes 0x%" PRIx64 ", status %s",
                  port_serdes_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  return status;
}

sai_status_t sai_set_port_serdes_attribute(sai_object_id_t port_serdes_id,
                                           const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  const sai_object_type_t sai_ot_ps = SAI_OBJECT_TYPE_PORT_SERDES;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;

  switch_object_id_t oid = {.data = port_serdes_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE1, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_PRE1 %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE2, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_PRE2 %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_MAIN, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_MAIN %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_SERDES_ATTR_TX_FIR_POST1: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST1, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_POST1 %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_SERDES_ATTR_TX_FIR_POST2: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST2, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_POST2 %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_PORT_SERDES_ATTR_TX_FIR_ATTN: {
      std::vector<int64_t> port_lane_values;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int64_t v = attr->value.s32list.list[idx];
        port_lane_values.push_back(v);
      }
      switch_status = bf_switch_attribute_set(
          oid, attr_w(SWITCH_PORT_SERDES_ATTR_TX_FIR_ATTN, port_lane_values));
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set port 0x%" PRIx64 "TX_FIR_ATTN %s",
                      port_serdes_id,
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    default:
      status = sai_to_switch_attribute_set(sai_ot_ps, attr, oid);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(sai_ot_ps, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }
  return status;
}
sai_status_t sai_get_port_serdes_attribute(sai_object_id_t port_serdes_id,
                                           uint32_t attr_count,
                                           sai_attribute_t *attr_list) {
  const sai_object_type_t sai_ot_ps = SAI_OBJECT_TYPE_PORT_SERDES;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  const switch_object_id_t sw_object_id = {.data = port_serdes_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_pre1_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE1);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE1,
                                    tx_fir_pre1_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_PRE1: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_pre1_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_pre2_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE2);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_PRE2,
                                    tx_fir_pre2_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_PRE2: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_pre2_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_main_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_MAIN);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_MAIN,
                                    tx_fir_main_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_MAIN: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_main_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST1: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_post1_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST1);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_POST1,
                                    tx_fir_post1_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_POST1: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_post1_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST2: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_post2_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_POST2);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_POST2,
                                    tx_fir_post2_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_POST2: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_post2_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      case SAI_PORT_SERDES_ATTR_TX_FIR_ATTN: {
        std::vector<int64_t> port_lane_values;
        attr_w tx_fir_attn_attr(SWITCH_PORT_SERDES_ATTR_TX_FIR_ATTN);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_PORT_SERDES_ATTR_TX_FIR_ATTN,
                                    tx_fir_attn_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port 0x%" PRIx64 "TX_FIR_ATTN: %s",
                        port_serdes_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        tx_fir_attn_attr.v_get(port_lane_values);
        TRY_LIST_SET(attr_list[i].value.s32list, port_lane_values);
        break;
      }
      default:
        status =
            sai_to_switch_attribute_get(sai_ot_ps, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS &&
            status != SAI_STATUS_BUFFER_OVERFLOW) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s",
                        sai_attribute_name(sai_ot_ps, attr_list[i].id),
                        sai_metadata_get_status_name(status));
        }
        break;
    }
  }
  return status;
}

sai_status_t sai_create_port_pool(sai_object_id_t *port_pool_id,
                                  sai_object_id_t switch_id,
                                  uint32_t attr_count,
                                  const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_remove_port_pool(sai_object_id_t port_pool_id) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_set_port_pool_attribute(sai_object_id_t port_pool_id,
                                         const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_get_port_pool_attribute(sai_object_id_t port_pool_id,
                                         uint32_t attr_count,
                                         sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_get_port_pool_stats(sai_object_id_t port_pool_id,
                                     uint32_t number_of_counters,
                                     const sai_stat_id_t *counter_ids,
                                     uint64_t *counters) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_get_port_pool_stats_ext(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t *counter_ids,  // list of counters
    sai_stats_mode_t mode,             // supports READ only
    uint64_t *counters) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_clear_port_pool_stats(sai_object_id_t port_id,
                                       uint32_t number_of_counters,
                                       const sai_stat_id_t *counter_ids) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_create_port_connector(sai_object_id_t *port_connector_id,
                                       sai_object_id_t switch_id,
                                       uint32_t attr_count,
                                       const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_remove_port_connector(sai_object_id_t port_connector_id) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_set_port_connector_attribute(sai_object_id_t port_connector_id,
                                              const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}

sai_status_t sai_get_port_connector_attribute(sai_object_id_t port_connector_id,
                                              uint32_t attr_count,
                                              sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  return status;
}
/*
 * Port methods table retrieved with sai_api_query()
 */
sai_port_api_t port_api = {
  create_port : sai_create_port,
  remove_port : sai_remove_port,
  set_port_attribute : sai_set_port_attribute,
  get_port_attribute : sai_get_port_attribute,
  get_port_stats : sai_get_port_stats,
  get_port_stats_ext : sai_get_port_stats_ext,
  clear_port_stats : sai_clear_port_stats,
  clear_port_all_stats : sai_clear_port_all_stats,
  create_port_pool : sai_create_port_pool,
  remove_port_pool : sai_remove_port_pool,
  set_port_pool_attribute : sai_set_port_pool_attribute,
  get_port_pool_attribute : sai_get_port_pool_attribute,
  get_port_pool_stats : sai_get_port_pool_stats,
  get_port_pool_stats_ext : sai_get_port_pool_stats_ext,
  clear_port_pool_stats : sai_clear_port_pool_stats,
  create_port_connector : sai_create_port_connector,
  remove_port_connector : sai_remove_port_connector,
  set_port_connector_attribute : sai_set_port_connector_attribute,
  get_port_connector_attribute : sai_get_port_connector_attribute,
  create_port_serdes : sai_create_port_serdes,
  remove_port_serdes : sai_remove_port_serdes,
  set_port_serdes_attribute : sai_set_port_serdes_attribute,
  get_port_serdes_attribute : sai_get_port_serdes_attribute,
};

sai_port_api_t *sai_port_api_get() { return &port_api; }

void sai_port_initialize() {
  device_handle = sai_get_device_id(0);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_PORT);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_PORT_SERDES);
}
