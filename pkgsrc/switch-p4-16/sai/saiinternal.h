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


#ifndef SAI_SAIINTERNAL_H_
#define SAI_SAIINTERNAL_H_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <set>
#include <utility>
#include <type_traits>

#include "bf_switch/bf_switch_types.h"
#include "bf_switch/bf_event.h"
#include "bf_switch/bf_switch.h"
#include "model.h"
#include "../s3/log.h"

extern "C" {
#include "sai.h"
#include "saitypes.h"
#include "saiversion.h"
#include "saimetadatatypes.h"
#include "saimetadatautils.h"
#include "saimetadata.h"
#include "sai_map.h"
#include "target-sys/bf_sal/bf_sys_intf.h"
#include "target-sys/bf_sal/bf_sys_log.h"
#include "target-sys/bf_sal/bf_sys_assert.h"
#include "target-sys/bf_sal/bf_sys_mem.h"
}

using namespace smi;
using namespace bf_switch;

/* macros */

#define SAI_MAX_ENTRY_STRING_LEN 200
#define IPV6_ADDR_LEN 16

/* typedefs */
typedef struct _sai_switch_notification_t {
  sai_switch_state_change_notification_fn on_switch_state_change;
  sai_fdb_event_notification_fn on_fdb_event;
  sai_nat_event_notification_fn on_nat_event;
  sai_port_state_change_notification_fn on_port_state_change;
  sai_switch_shutdown_request_notification_fn on_switch_shutdown_request;
  sai_packet_event_notification_fn on_packet_event;
  sai_bfd_session_state_change_notification_fn on_bfd_session_state_change;
} sai_switch_notification_t;

extern sai_switch_notification_t sai_switch_notifications;

typedef std::unordered_map<sai_enum_data_t, sw_enum_data_t> sai_to_sw_enum_map;

typedef std::unordered_map<sw_enum_data_t, sai_enum_data_t> sw_to_sai_enum_map;

struct sai_to_sw_enum_metadata_t {
  sai_to_sw_enum_map forward_enum_map;
  sw_to_sai_enum_map reverse_enum_map;
};

struct sai_to_sw_attr_mapping_metadata_t {
  switch_attr_id_t sw_attr_id;
  sai_to_sw_enum_metadata_t
      sai_sw_enum_hashmap; /* valid for attribute type enum */
};

typedef std::unordered_map<sai_attr_id_t, sai_to_sw_attr_mapping_metadata_t>
    sai_to_switch_attributes_map_t;

struct sai_to_sw_attributes_mapping_metadata_t {
  sai_to_switch_attributes_map_t forward_attrs_hashmap;
};

struct sai_to_sw_obj_metadata_t {
  switch_object_type_t sw_ot;
  sai_to_sw_attributes_mapping_metadata_t sai_sw_attrs_mapping_md;
#define obj_attrs_forward_map sai_sw_attrs_mapping_md.forward_attrs_hashmap
};

typedef std::unordered_map<sai_attr_id_t, sai_attr_capability_t>
    sai_attr_capability_map;

namespace std {
template <>
struct hash<sai_object_type_t> {
  inline size_t operator()(sai_object_type_t const &object_type) const {
    return std::hash<uint64_t>{}(object_type);
  }
};
}  // namespace std
typedef std::unordered_map<sai_object_type_t, sai_attr_capability_map>
    sai_object_capability_map;

/* macros */
#define SWITCH_SAI_APP_ID 0x1
#define UNUSED(x) (void)x;

#define SAI_ASSERT(x) bf_sys_assert(x)

sai_switch_api_t *sai_switch_api_get();
sai_port_api_t *sai_port_api_get();
sai_bridge_api_t *sai_bridge_api_get();

sai_acl_api_t *sai_acl_api_get();
sai_buffer_api_t *sai_buffer_api_get();
sai_fdb_api_t *sai_fdb_api_get();
sai_hash_api_t *sai_hash_api_get();
sai_hostif_api_t *sai_hostif_api_get();
sai_ipmc_api_t *sai_ipmc_api_get();
sai_ipmc_group_api_t *sai_ipmc_group_api_get();
sai_l2mc_api_t *sai_l2mc_api_get();
sai_l2mc_group_api_t *sai_l2mc_group_api_get();
sai_lag_api_t *sai_lag_api_get();
sai_my_mac_api_t *sai_my_mac_api_get();
sai_mirror_api_t *sai_mirror_api_get();
sai_neighbor_api_t *sai_neighbor_api_get();
sai_next_hop_group_api_t *sai_next_hop_group_api_get();
sai_next_hop_api_t *sai_next_hop_api_get();
sai_policer_api_t *sai_policer_api_get();
sai_qos_map_api_t *sai_qos_map_api_get();
sai_route_api_t *sai_route_api_get();
sai_router_interface_api_t *sai_router_interface_api_get();
sai_scheduler_group_api_t *sai_scheduler_group_api_get();
sai_scheduler_api_t *sai_scheduler_api_get();
sai_stp_api_t *sai_stp_api_get();
sai_dtel_api_t *sai_dtel_api_get();
sai_tunnel_api_t *sai_tunnel_api_get();
sai_udf_api_t *sai_udf_api_get();
sai_virtual_router_api_t *sai_virtual_router_api_get();
sai_vlan_api_t *sai_vlan_api_get();
sai_bfd_api_t *sai_bfd_api_get();
sai_queue_api_t *sai_queue_api_get();
sai_wred_api_t *sai_wred_api_get();
sai_debug_counter_api_t *sai_debug_counter_api_get();
sai_samplepacket_api_t *sai_samplepacket_api_get();
sai_nat_api_t *sai_nat_api_get();
sai_mpls_api_t *sai_mpls_api_get();
#if SAI_API_VERSION >= 10901
sai_srv6_api_t *sai_srv6_api_get();
#endif
sai_isolation_group_api_t *sai_isolation_group_api_get();
sai_counter_api_t *sai_counter_api_get();
sai_rpf_group_api_t *sai_rpf_group_api_get();

sai_status_t sai_initialize(bool warm_init);

void sai_switch_initialize();
void sai_port_initialize();
sai_status_t sai_hash_initialize(bool warm_init);
sai_status_t sai_bridge_initialize(bool warm_init);
sai_status_t sai_hostif_initialize(bool warm_init);
sai_status_t sai_lag_initialize();
sai_status_t sai_my_mac_initialize();
sai_status_t sai_bfd_initialize();
sai_status_t sai_vlan_initialize();
sai_status_t sai_fdb_initialize();
sai_status_t sai_nat_initialize();
sai_status_t sai_router_interface_initialize();
sai_status_t sai_next_hop_initialize();
sai_status_t sai_route_initialize();
sai_status_t sai_buffer_initialize(bool warm_init);
sai_status_t sai_queue_initialize();
sai_status_t sai_next_hop_group_initialize();
sai_status_t sai_neighbor_initialize();
sai_status_t sai_virtual_router_initialize();
sai_status_t sai_acl_initialize(bool warm_init);
sai_status_t sai_policer_initialize();
sai_status_t sai_dtel_initialize();
sai_status_t sai_scheduler_initialize();
sai_status_t sai_scheduler_group_initialize();
sai_status_t sai_mirror_initialize();
sai_status_t sai_qos_map_initialize();
sai_status_t sai_tunnel_initialize();
sai_status_t sai_samplepacket_initialize();
sai_status_t sai_mpls_initialize(bool warm_init);
sai_status_t sai_udf_initialize();
sai_status_t sai_counter_initialize(bool warm_init);
sai_status_t sai_l2mc_initialize();
sai_status_t sai_l2mc_group_initialize();
sai_status_t sai_ipmc_initialize();
sai_status_t sai_ipmc_group_initialize();
sai_status_t sai_rpf_group_initialize();

// L2 learn timeout in milliseconds
#define SAI_L2_LEARN_TIMEOUT 100

inline sai_status_t status_switch_to_sai(const switch_status_t status) {
  switch (status) {
    case SWITCH_STATUS_SUCCESS:
      return SAI_STATUS_SUCCESS;
    case SWITCH_STATUS_FAILURE:
      return SAI_STATUS_FAILURE;
    case SWITCH_STATUS_INVALID_PARAMETER:
      return SAI_STATUS_INVALID_PARAMETER;
    case SWITCH_STATUS_NO_MEMORY:
      return SAI_STATUS_NO_MEMORY;
    case SWITCH_STATUS_ITEM_ALREADY_EXISTS:
      return SAI_STATUS_ITEM_ALREADY_EXISTS;
    case SWITCH_STATUS_ITEM_NOT_FOUND:
      return SAI_STATUS_ITEM_NOT_FOUND;
    // case SWITCH_STATUS_TABLE_FULL:
    //  return SAI_STATUS_TABLE_FULL;
    case SWITCH_STATUS_NOT_SUPPORTED:
      return SAI_STATUS_NOT_SUPPORTED;
    case SWITCH_STATUS_RESOURCE_IN_USE:
      return SAI_STATUS_OBJECT_IN_USE;
    case SWITCH_STATUS_INSUFFICIENT_RESOURCES:
      return SAI_STATUS_INSUFFICIENT_RESOURCES;
    default:
      return SAI_STATUS_FAILURE;
  }
}

bool sai_log_is_enabled(int api_id, int bf_level);

#define SAI_LOG_ENTER()                                                   \
  do {                                                                    \
    if (sai_log_is_enabled(api_id, BF_LOG_DBG)) {                         \
      bf_sys_log_and_trace(BF_MOD_SAI, BF_LOG_DBG, "Enter:%s", __func__); \
    }                                                                     \
  } while (0);

#define SAI_LOG_EXIT()                                                    \
  do {                                                                    \
    if (sai_log_is_enabled(api_id, BF_LOG_DBG)) {                         \
      bf_sys_log_and_trace(BF_MOD_SAI, BF_LOG_DBG, "Exit :%s", __func__); \
    }                                                                     \
  } while (0);

#define SAI_LOG_DEBUG(fmt, arg...)                                           \
  do {                                                                       \
    if (sai_log_is_enabled(api_id, BF_LOG_DBG)) {                            \
      bf_sys_log_and_trace(                                                  \
          BF_MOD_SAI, BF_LOG_DBG, "%s:%d: " fmt, __func__, __LINE__, ##arg); \
    }                                                                        \
  } while (0);

#define SAI_LOG_INFO(fmt, arg...)                                             \
  do {                                                                        \
    if (sai_log_is_enabled(api_id, BF_LOG_INFO)) {                            \
      bf_sys_log_and_trace(                                                   \
          BF_MOD_SAI, BF_LOG_INFO, "%s:%d: " fmt, __func__, __LINE__, ##arg); \
    }                                                                         \
  } while (0);

#define SAI_LOG_WARN(fmt, arg...)                                             \
  do {                                                                        \
    if (sai_log_is_enabled(api_id, BF_LOG_WARN)) {                            \
      bf_sys_log_and_trace(                                                   \
          BF_MOD_SAI, BF_LOG_WARN, "%s:%d: " fmt, __func__, __LINE__, ##arg); \
    }                                                                         \
  } while (0);

#define SAI_LOG_ERROR(fmt, arg...)                                           \
  do {                                                                       \
    if (sai_log_is_enabled(api_id, BF_LOG_ERR)) {                            \
      bf_sys_log_and_trace(                                                  \
          BF_MOD_SAI, BF_LOG_ERR, "%s:%d: " fmt, __func__, __LINE__, ##arg); \
    }                                                                        \
  } while (0);

#define SAI_LOG_CRITICAL(fmt, arg...)                                         \
  do {                                                                        \
    if (sai_log_is_enabled(api_id, BF_LOG_CRIT)) {                            \
      bf_sys_log_and_trace(                                                   \
          BF_MOD_SAI, BF_LOG_CRIT, "%s:%d: " fmt, __func__, __LINE__, ##arg); \
    }                                                                         \
  } while (0);

#define SAI_GENERIC_DEBUG(fmt, arg...) \
  bf_sys_log_and_trace(                \
      BF_MOD_SAI, BF_LOG_DBG, "%s:%d: " fmt, __func__, __LINE__, ##arg)

#define SAI_GENERIC_ERROR(fmt, arg...) \
  bf_sys_log_and_trace(                \
      BF_MOD_SAI, BF_LOG_ERR, "%s:%d: " fmt, __func__, __LINE__, ##arg)

// stats capability

typedef std::unordered_map<sai_stat_id_t, std::vector<uint16_t>>
    sai_to_switch_counters_map;

inline uint16_t supported_counters_count(
    const sai_to_switch_counters_map &counters_map) {
  uint16_t count = 0;
  for (const auto &item : counters_map) {
    if (!item.second.empty()) {
      count++;
    }
  }
  return count;
}

inline sai_status_t query_stats_capability_by_mapping(
    const sai_to_switch_counters_map &mapping,
    sai_stat_capability_list_t &stats_capability,
    const uint16_t supported_count) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  if (stats_capability.count < supported_count) {
    stats_capability.count = supported_count;
    status = SAI_STATUS_BUFFER_OVERFLOW;
  } else if (stats_capability.list == nullptr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_GENERIC_ERROR("stats_capability list is nullptr.");
  } else {
    stats_capability.count = supported_count;
    sai_stat_capability_t *iter = stats_capability.list;
    for (const auto &item : mapping) {
      if (!item.second.empty()) {
        iter->stat_enum = item.first;
        iter->stat_modes = SAI_STATS_MODE_READ | SAI_STATS_MODE_READ_AND_CLEAR;
        iter++;
      }
    }
  }
  return status;
}

inline uint64_t get_counters_count_by_id(
    const sai_to_switch_counters_map &mapping,
    const sai_stat_id_t counter_id,
    const std::vector<switch_counter_t> &cntrs) {
  uint64_t result = 0;
  const auto counters_it = mapping.find(counter_id);
  if (counters_it != mapping.end() && !counters_it->second.empty()) {
    for (const auto id : counters_it->second) {
      result += cntrs[id].count;
    }
  }
  return result;
}

sai_status_t query_port_stats_capability(
    sai_stat_capability_list_t &stats_capability);
sai_status_t query_queue_stats_capability(
    sai_stat_capability_list_t &stats_capability);
sai_status_t query_ingress_priority_group_stats_capability(
    sai_stat_capability_list_t &stats_capability);
sai_status_t query_buffer_pool_stats_capability(
    sai_stat_capability_list_t &stats_capability);
sai_status_t query_rif_stats_capability(
    sai_stat_capability_list_t &stats_capability);

// end of stats capability

const sai_attribute_t *sai_get_attr_from_list(sai_attr_id_t attr_id,
                                              const sai_attribute_t *attr_list,
                                              uint32_t attr_count);
sai_status_t sai_to_switch_value(const sai_object_type_t sai_ot,
                                 const sai_attribute_t &sai_attr,
                                 smi::attr_w &sw_attr);
sai_status_t switch_to_sai_value(const smi::attr_w &sw_attr,
                                 sai_attribute_t &sai_attr);
sai_status_t sai_to_switch_attribute_set(const sai_object_type_t ot,
                                         const sai_attribute_t *sai_attr,
                                         const switch_object_id_t oid);
sai_status_t sai_to_sw_object_type(const sai_object_type_t sai_ot,
                                   switch_object_type_t &sw_ot);

sai_status_t sai_to_sw_attr_id(const sai_object_type_t sai_ot,
                               const sai_attr_id_t sai_attr_id,
                               switch_attr_id_t &sw_attr_id);

sai_status_t sai_to_switch_attribute_list(const sai_object_type_t ot,
                                          uint32_t attr_count,
                                          const sai_attribute_t *sai_attr_list,
                                          std::set<smi::attr_w> &sw_attr_list);

sai_status_t sai_to_switch_attribute(const sai_object_type_t ot,
                                     const sai_attribute_t *sai_attr,
                                     std::set<smi::attr_w> &sw_attr_list);
sai_status_t sai_to_switch_attribute_get(const sai_object_type_t ot,
                                         const switch_object_id_t oid,
                                         sai_attribute_t *sai_attr);

sai_status_t sai_to_switch_attribute_list_get(const sai_object_type_t ot,
                                              const switch_object_id_t oid,
                                              uint32_t attr_count,
                                              sai_attribute_t *sai_attr_list);

const char *sai_attribute_name(sai_object_type_t sai_ot,
                               const sai_attr_id_t attr_id);

switch_object_id_t sai_get_device_id(const uint16_t dev_id);

void sai_insert_device_attribute(const uint16_t dev_id,
                                 const switch_attr_id_t sw_dev_attr_id,
                                 std::set<smi::attr_w> &sw_attr_list);

sai_status_t sai_get_port_from_bridge_port(const sai_object_id_t bridge_port_id,
                                           switch_object_id_t &port_lag_handle);
sai_status_t sai_get_port_to_bridge_port(const switch_object_id_t port_handle,
                                         sai_object_id_t &bridge_port_id);
sai_status_t sai_get_tunnel_from_bridge_port(
    const sai_object_id_t bridge_port_id, switch_object_id_t &port_lag_handle);
sai_status_t sai_get_tunnel_to_bridge_port(
    const switch_object_id_t tunnel_handle, sai_object_id_t &bridge_port_id);
sai_status_t sai_get_bridge_port_interface_type(
    const sai_object_id_t bridge_port_id, switch_enum_t &intf_type);

void sai_port_oper_state_update(void);

sai_status_t sai_ipv4_to_string(_In_ sai_ip4_t ip4,
                                _In_ uint32_t max_length,
                                _Out_ char *entry_string,
                                _Out_ int *entry_length);

sai_status_t sai_ipv6_to_string(_In_ const sai_ip6_t &ip6,
                                _In_ uint32_t max_length,
                                _Out_ char *entry_string,
                                _Out_ int *entry_length);

sai_status_t sai_ipaddress_to_string(_In_ sai_ip_address_t ip_addr,
                                     _In_ uint32_t max_length,
                                     _Out_ char *entry_string,
                                     _Out_ int *entry_length);

sai_status_t sai_ipprefix_to_string(_In_ sai_ip_prefix_t ip_prefix,
                                    _In_ uint32_t max_length,
                                    _Out_ char *entry_string,
                                    _Out_ int *entry_length);

void switch_ip_addr_to_sai_ipv4(sai_ip4_t &ip4,
                                const switch_ip_address_t &switch_ip_addr);

void switch_ip_addr_to_sai_ipv6(sai_ip6_t &ip6,
                                const switch_ip_address_t &switch_ip_addr);

void sai_ipv4_to_switch_ip_addr(const sai_ip4_t ip4,
                                switch_ip_address_t &switch_ip_addr);

void sai_ipv6_to_switch_ip_addr(const sai_ip6_t ip6,
                                switch_ip_address_t &switch_ip_addr);

void sai_ip_addr_to_switch_ip_addr(const sai_ip_address_t *sai_ip_addr,
                                   switch_ip_address_t &switch_ip_addr);

bool switch_ipv6_prefix_link_local_host_ip(const switch_ip_prefix_t &ip_prefix);

void sai_ip_prefix_to_switch_ip_prefix(const sai_ip_prefix_t *sai_ip_prefix,
                                       switch_ip_prefix_t &switch_ip_prefix);

sai_status_t sai_get_vlan_oid_by_vlan_id(_Out_ sai_object_id_t *vlan_oid,
                                         _In_ sai_uint16_t vlan_id);

sai_status_t sai_flush_fdb_entries(_In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list);

void sai_ipv4_prefix_length(sai_ip4_t ip4, uint16_t *prefix_length);

void sai_ipv6_prefix_length(const sai_ip6_t ip6, uint16_t *prefix_length);

uint32_t sai_acl_priority_to_switch_priority(sai_uint32_t sai_acl_priority);

sai_uint32_t switch_priority_to_sai_acl_priority(
    const uint32_t switch_priority);

uint32_t sai_hostif_priority_to_switch_hostif_priority(
    sai_uint32_t sai_hostif_priority);
sai_uint32_t switch_hostif_priority_to_sai_hostif_priority(
    uint32_t switch_hostif_priority);

switch_status_t sai_init_global_pcp_tc_qosmap(uint16_t dev_id);

sai_status_t sai_route_set_glean_internal(const switch_object_id_t nhop_handle);

sai_status_t sai_switch_set_learn_notif_timeout(const switch_object_id_t device,
                                                uint32_t timeout);

std::string sai_object_name_query(sai_object_id_t sai_object_id);
sai_status_t sai_acl_get_supported_actions(
    _In_ switch_object_id_t device,
    _In_ sai_acl_stage_t stage,
    _Out_ std::set<sai_acl_action_type_t> &sai_actions_list);
sai_status_t sai_insert_sw_sai_object_type_mapping(switch_object_type_t sw_ot,
                                                   sai_object_type_t sai_ot);

sai_status_t sai_get_port_stats_ext(sai_object_id_t port_id,
                                    uint32_t number_of_counters,
                                    const sai_stat_id_t *counter_ids,
                                    sai_stats_mode_t mode,
                                    uint64_t *counters);

sai_status_t sai_get_debug_counter_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability);
sai_status_t sai_get_next_hop_group_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability);
sai_status_t sai_get_hostif_user_defined_trap_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability);
sai_status_t sai_get_neighbor_entry_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability);
sai_status_t sai_get_router_interface_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability);
sai_status_t sai_get_debug_counter_type_availability(
    sai_object_id_t switch_id,
    sai_object_type_t object_type,
    uint32_t attr_count,
    const sai_attribute_t *attr_list,
    uint64_t *count);

sai_status_t sai_generate_attr_capability(_In_ sai_object_type_t object_type);

sai_object_capability_map &sai_get_object_capability_map();

char *sai_strncpy(char *dest, const char *src, size_t n);

sai_status_t sai_route_entry_get_availability(sai_object_id_t switch_id,
                                              uint32_t attr_count,
                                              const sai_attribute_t *attr_list,
                                              uint64_t *count);

sai_status_t sai_neighbor_entry_get_availability(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t *attr_list,
    uint64_t *count);

void bf_sai_add_object_type_to_supported_list(sai_object_type_t object_type);
const std::set<sai_object_type_t> &bf_sai_get_supported_object_types();

inline std::string serialize_sai_attribute(
    _In_ const sai_attr_metadata_t *meta,
    _In_ const sai_attribute_t *attribute) {
  char buff[256]{};
  sai_serialize_attribute(buff, meta, attribute);
  return std::string(buff);
}

inline std::string serialize_switch_attribute(
    _In_ const smi::attr_w &attribute) {
  std::stringstream ss;
  ss << attribute;
  return ss.str();
}

// clang-format off

// Copy from std container to sai list with convertor sai_list_element_type
// convertor(std_containar_element_type)
template <typename List,
          typename Element,
          template <class... Args> typename Container>
sai_status_t sai_list_set_value(
    List &sai_list,
    const Container<Element> &list,
    typename std::remove_pointer<decltype(List::list)>::type (*convertor)(
        Element)) {
  if (sai_list.count < list.size()) {
    sai_list.count = list.size();
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  sai_list.count = list.size();
  auto *iterator = sai_list.list;
  for (const auto &from : list) {
    *iterator++ = convertor(from);
  }
  return SAI_STATUS_SUCCESS;
}

// Copy from std container to sai list if list element type and container
// element type are same or convertible
template <
    typename List,
    typename Element,
    template <class... Args> typename Container,
    typename std::enable_if<
      std::is_convertible<
        typename std::remove_pointer<
          decltype(List::list)>::type,
          Element
        >::value, bool>::type = true>
sai_status_t sai_list_set_value(List &sai_list,
                                const Container<Element> &list) {
  if (sai_list.count < list.size()) {
    sai_list.count = list.size();
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  sai_list.count = list.size();
  decltype(List::list) it = sai_list.list;
  std::for_each(list.begin(), list.end(), [&it](const Element &value) {
    *it++ =
        static_cast<typename std::remove_reference<decltype(*it)>::type>(value);
  });
  return SAI_STATUS_SUCCESS;
}

// Copy from std container of enums to sai s32list by value
template <
    typename Element,
    template <class... Args> typename Container,
    typename std::enable_if<std::is_enum<Element>::value, bool>::type = true>
sai_status_t sai_list_set_value(sai_s32_list_t &sai_list,
                                const Container<Element> &list) {
  if (sai_list.count < list.size()) {
    sai_list.count = list.size();
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  sai_list.count = list.size();
  int32_t *it = sai_list.list;
  std::for_each(list.begin(), list.end(), [&it](const Element &value) {
    *it++ = static_cast<int32_t>(value);
  });
  return SAI_STATUS_SUCCESS;
}

// Copy from std container to sai list if list element type is sai_object_id_t
// and container element type is switch_object_id_t
template <template <class... Args> typename Container>
sai_status_t sai_list_set_value(sai_object_list_t &sai_list,
                                const Container<switch_object_id_t> &list) {
  if (sai_list.count < list.size()) {
    sai_list.count = list.size();
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  sai_list.count = list.size();
  sai_object_id_t *it = sai_list.list;
  std::for_each(list.begin(), list.end(), [&it](const switch_object_id_t &oid) {
    *it++ = oid.data;
  });
  return SAI_STATUS_SUCCESS;
}

// clang-format on

// macros to copy std container(list, vector, set) to sai list with correct sai
// list processing using convertor functor
//    @__sai_list  - reference to sai list where values will be copied
//    @__list      - const reference to vector, list or set of values wich will
//    be copied to sai list
//    @__convertor - pointer to function with argument type same as __list
//    element type and witch return __sai_list element type
// For exsample
// TRY_LIST_SET_WITH_CONVERTOR(attribute.s32list,
//                             std::vector<uint32_t>,
//                             [](uint32_t in) -> int32_t {
//                                /* some values mapping */
//                             });

#define TRY_LIST_SET_WITH_CONVERTOR(__sai_list, __list, __convertor) \
  {                                                                  \
    sai_status_t __status =                                          \
        sai_list_set_value(__sai_list, __list, __convertor);         \
    if (__status) return __status;                                   \
  }

// macros to copy std container(list, vector, set) to sai list with correct sai
// list processing
//    @__sai_list - reference to sai list where values will be copied
//    @__list     - const reference to vector, list or set of values wich will
//    be copied to sai list
// macros support:
//  - lists with same element type e.g sai_u32_list_t and std::vector<uint32_t>
//  - lists with convertible element types e.g. sai_u32_list_t and
//  std::list<uint16_t>
//  - sai_s32_list_t and list of enum values (mapping enum by value) e.g.
//  sai_s32_list_t and std::set<sai_acl_action_type_t>
//  - sai_object_list_t and list of switch_object_id_t e.g. sai_object_list_t
//  and std::vector<switch_object_id_t>
// if macros will be used with unsupported types - code will not compile
#define TRY_LIST_SET(__sai_list, __list)                            \
  {                                                                 \
    sai_status_t __status = sai_list_set_value(__sai_list, __list); \
    if (__status) return __status;                                  \
  }

#endif  // SAI_SAIINTERNAL_H_
