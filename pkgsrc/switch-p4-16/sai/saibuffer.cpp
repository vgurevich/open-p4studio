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
#include <list>

static sai_api_t api_id = SAI_API_BUFFER;
static switch_object_id_t device_handle = {0};

static const sai_to_switch_counters_map
    ingress_priority_group_to_switch_counter_mapping{
        {SAI_INGRESS_PRIORITY_GROUP_STAT_PACKETS,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_PACKETS}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_BYTES}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_DROPPED_PACKETS,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_WATERMARK_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_WATERMARK_BYTES}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_CURR_OCCUPANCY_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_CURR_OCCUPANCY_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_CURR_OCCUPANCY_BYTES}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_XOFF_ROOM_CURR_OCCUPANCY_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SKID_CURR_OCCUPANCY_BYTES}},
        {SAI_INGRESS_PRIORITY_GROUP_STAT_XOFF_ROOM_WATERMARK_BYTES,
         {}},  // Unsupported
        {SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_WATERMARK_BYTES,
         {SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_WATERMARK_BYTES}}};

static const sai_to_switch_counters_map buffer_pool_to_switch_counter_mapping{
    {SAI_BUFFER_POOL_STAT_CURR_OCCUPANCY_BYTES,
     {SWITCH_BUFFER_POOL_COUNTER_ID_CURR_OCCUPANCY_BYTES}},
    {SAI_BUFFER_POOL_STAT_WATERMARK_BYTES,
     {SWITCH_BUFFER_POOL_COUNTER_ID_WATERMARK_BYTES}},
    {SAI_BUFFER_POOL_STAT_DROPPED_PACKETS, {}}  // Unsupported
};

sai_status_t query_ingress_priority_group_stats_capability(
    sai_stat_capability_list_t &stats_capability) {
  static const uint16_t supported_count = supported_counters_count(
      ingress_priority_group_to_switch_counter_mapping);
  return query_stats_capability_by_mapping(
      ingress_priority_group_to_switch_counter_mapping,
      stats_capability,
      supported_count);
}

sai_status_t query_buffer_pool_stats_capability(
    sai_stat_capability_list_t &stats_capability) {
  static const uint16_t supported_count =
      supported_counters_count(buffer_pool_to_switch_counter_mapping);
  return query_stats_capability_by_mapping(
      buffer_pool_to_switch_counter_mapping, stats_capability, supported_count);
}

/**
 * @brief Create ingress priority group
 *
 * @param[out] ingress_pg_id Ingress priority group
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_ingress_priority_group(
    _Out_ sai_object_id_t *ingress_pg_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  const sai_attribute_t *sai_attr;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t i = 0;
  std::set<smi::attr_w> sw_attr_list;

  switch_object_id_t port_handle = {0};
  switch_object_id_t ppg_handle = {0};

  if (!ingress_pg_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (i = 0, sai_attr = attr_list; i < attr_count; i++, sai_attr++) {
    switch (sai_attr->id) {
      case SAI_INGRESS_PRIORITY_GROUP_ATTR_PORT:
        port_handle.data = sai_attr->value.oid;
        break;
      default:
        break;
    }
  }

  if (port_handle.data == 0) {
    SAI_LOG_ERROR("Port handle is invalid");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  status = sai_to_switch_attribute_list(SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
                                        attr_count,
                                        attr_list,
                                        sw_attr_list);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to convert attributes: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(
      0, SWITCH_PORT_PRIORITY_GROUP_ATTR_DEVICE, sw_attr_list);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP, sw_attr_list, ppg_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create ingress priority group: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *ingress_pg_id = ppg_handle.data;
  return status;
}

/**
 * @brief Remove ingress priority group
 *
 * @param[in] ingress_pg_id Ingress priority group
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_remove_ingress_priority_group(
    _In_ sai_object_id_t ingress_pg_id) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t ppg_handle = {.data = ingress_pg_id};

  switch_status = bf_switch_object_delete(ppg_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to delete ingress priority group 0x%" PRIx64 ": %s",
                  ingress_pg_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  return status;
}

/**
 * @brief Set ingress priority group attribute
 * @param[in] ingress_pg_id ingress priority group id
 * @param[in] attr attribute to set
 *
 * @return  SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_set_ingress_priority_group_attribute(
    _In_ sai_object_id_t ingress_pg_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  switch_object_id_t ppg_handle = {.data = ingress_pg_id};

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(ingress_pg_id) ==
             SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP);

  if (attr->id == SAI_INGRESS_PRIORITY_GROUP_ATTR_BUFFER_PROFILE) {
    // When buffer profile handle is invalid, SDK programs the device default
    // buffer profile to the PPG.
    status = sai_to_switch_attribute_set(
        SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP, attr, ppg_handle);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "failed to set pg buffer profile attribute %s: %s",
          sai_attribute_name(SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP, attr->id),
          sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get ingress priority group attributes
 * @param[in] ingress_pg_id ingress priority group id
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list array of attributes
 *
 * @return  SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_get_ingress_priority_group_attribute(
    _In_ sai_object_id_t ingress_pg_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(ingress_pg_id) ==
             SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP);

  const switch_object_id_t sw_object_id = {.data = ingress_pg_id};

  for (i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_INGRESS_PRIORITY_GROUP_ATTR_TAM:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      default:
        status =
            sai_to_switch_attribute_get(SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
                                        sw_object_id,
                                        &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get priority group attribute object_id: 0x%" PRIx64
              " attribute %s "
              "error: %s",
              ingress_pg_id,
              sai_attribute_name(SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
                                 attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Clear ingress priority group statistics counters.
 *
 * @param[in] ingress_priority_group_id Ingress priority group id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_clear_ingress_priority_group_stats(
    _In_ sai_object_id_t ingress_priority_group_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::list<uint16_t> cntr_ids;

  if (!counter_ids) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Ingress PPG stats clear failed - null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(ingress_priority_group_id) !=
      SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP) {
    SAI_LOG_ERROR(
        "Ingress PPG stats clear failed - invalid PPG handle 0x%" PRIx64,
        ingress_priority_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = ingress_priority_group_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    const auto counters_it =
        ingress_priority_group_to_switch_counter_mapping.find(counter_ids[i]);
    if (counters_it != ingress_priority_group_to_switch_counter_mapping.end() &&
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
      SAI_LOG_ERROR("Failed to clear PPG 0x%" PRIx64 " stats, status %s",
                    ingress_priority_group_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get ingress priority group statistics counters.
 *
 * @param[in] ingress_priority_group_id Ingress priority group id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_ingress_priority_group_stats(
    _In_ sai_object_id_t ingress_pg_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;
  switch_object_id_t sw_object_id = {.data = ingress_pg_id};

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(ingress_pg_id) !=
      SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP) {
    SAI_LOG_ERROR(
        "Ingress priority group stats get failed: invalid priority group "
        "handle 0x%" PRIx64,
        ingress_pg_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  sw_status = bf_switch_counters_get(sw_object_id, cntrs);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get priority group 0x%" PRIx64 " stats: %s",
                  ingress_pg_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  for (uint32_t i = 0; i < number_of_counters; i++) {
    counters[i] = get_counters_count_by_id(
        ingress_priority_group_to_switch_counter_mapping,
        counter_ids[i],
        cntrs);
  }
  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get ingress priority group statistics counters extended.
 *
 * @param[in] ingress_priority_group_id Ingress priority group id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[in] mode Statistics mode
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_ingress_priority_group_stats_ext(
    _In_ sai_object_id_t ingress_priority_group_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _In_ sai_stats_mode_t mode,
    _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  status = sai_get_ingress_priority_group_stats(
      ingress_priority_group_id, number_of_counters, counter_ids, counters);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get ingress priority group stats: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (mode == SAI_STATS_MODE_READ_AND_CLEAR) {
    status = sai_clear_ingress_priority_group_stats(
        ingress_priority_group_id, number_of_counters, counter_ids);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear ingress priority group stats: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Create buffer pool
 * @param[out] pool_id buffer pool id
 * @param[in] switch_id Switch id
 * @param[in] attr_count number of attributes
 * @param[in] attr_list array of attributes
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_create_buffer_pool(_Out_ sai_object_id_t *pool_id,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t buffer_pool = {0};
  std::set<attr_w> sw_attr_list;

  if (!pool_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *pool_id = SAI_NULL_OBJECT_ID;
  for (uint32_t i = 0; i < attr_count; i++) {
    const sai_attribute_t *attribute = &attr_list[i];
    status = sai_to_switch_attribute(
        SAI_OBJECT_TYPE_BUFFER_POOL, attribute, sw_attr_list);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to create buffer pool, attribute %s: %s ",
          sai_attribute_name(SAI_OBJECT_TYPE_BUFFER_POOL, attribute->id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  sai_insert_device_attribute(0, SWITCH_BUFFER_POOL_ATTR_DEVICE, sw_attr_list);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_BUFFER_POOL, sw_attr_list, buffer_pool);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create buffer pool: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *pool_id = buffer_pool.data;
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove buffer pool
 * @param[in] pool_id buffer pool id
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_remove_buffer_pool(_In_ sai_object_id_t pool_id) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t buffer_pool = {.data = pool_id};

  SAI_ASSERT(sai_object_type_query(pool_id) == SAI_OBJECT_TYPE_BUFFER_POOL);

  switch_status = bf_switch_object_delete(buffer_pool);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to delete buffer pool: %s",
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set buffer pool attribute
 * @param[in] pool_id buffer pool id
 * @param[in] attr attribute
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_set_buffer_pool_attribute(_In_ sai_object_id_t pool_id,
                                           _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = pool_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(pool_id) == SAI_OBJECT_TYPE_BUFFER_POOL);

  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_BUFFER_POOL, attr, sw_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set buffer pool attribute %s: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_BUFFER_POOL, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get buffer pool attributes
 * @param[in] pool_id buffer pool id
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list array of attributes
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_get_buffer_pool_attribute(_In_ sai_object_id_t pool_id,
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

  SAI_ASSERT(sai_object_type_query(pool_id) == SAI_OBJECT_TYPE_BUFFER_POOL);
  const switch_object_id_t sw_object_id = {.data = pool_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_BUFFER_POOL_ATTR_WRED_PROFILE_ID:  // Unsupported
      case SAI_BUFFER_POOL_ATTR_TAM:              // Unsupported
        memset(&attr_list[i].value, 0, sizeof(sai_attribute_value_t));
        break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_BUFFER_POOL, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS &&
            status != SAI_STATUS_NOT_SUPPORTED) {
          SAI_LOG_ERROR(
              "Failed to get buffer pool  attributes: %" PRIx64
              " attr: %s error: %s\n",
              pool_id,
              sai_attribute_name(SAI_OBJECT_TYPE_BUFFER_POOL, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get buffer pool statistics counters.
 *
 * @param[in] buffer_pool_id Buffer pool id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_buffer_pool_stats(_In_ sai_object_id_t buffer_pool_id,
                                       _In_ uint32_t number_of_counters,
                                       _In_ const sai_stat_id_t *counter_ids,
                                       _Out_ uint64_t *counters) {
  uint32_t index = 0;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;
  switch_object_id_t sw_object_id = {.data = buffer_pool_id};

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(buffer_pool_id) ==
             SAI_OBJECT_TYPE_BUFFER_POOL);

  switch_status = bf_switch_counters_get(sw_object_id, cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get buffer pool 0x%" PRIx64 " stats: %s",
                  buffer_pool_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (index = 0; index < number_of_counters; index++) {
    counters[index] = get_counters_count_by_id(
        buffer_pool_to_switch_counter_mapping, counter_ids[index], cntrs);
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

// FIXME - move to schema
#define SWITCH_API_BUFFER_DYNAMIC_MIN_THRESHOLD 0
#define SWITCH_API_BUFFER_DYNAMIC_MAX_THRESHOLD 255
#define SWITCH_API_BUFFER_DYNAMIC_THRESHOLD_FACTOR 32

uint64_t sai_buffer_threshold_to_switch_threshold(uint8_t threshold) {
  if (threshold == 0) {
    return SWITCH_API_BUFFER_DYNAMIC_MIN_THRESHOLD;
  }

  if (threshold >= 8) {
    return SWITCH_API_BUFFER_DYNAMIC_MAX_THRESHOLD;
  }
  return (threshold * SWITCH_API_BUFFER_DYNAMIC_THRESHOLD_FACTOR);
}

/**
 * @brief Clear buffer pool statistics counters.
 *
 * @param[in] buffer_pool_id Buffer pool id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_clear_buffer_pool_stats(
    _In_ sai_object_id_t buffer_pool_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::list<uint16_t> cntr_ids;
  const switch_object_id_t sw_object_id = {.data = buffer_pool_id};

  if (!counter_ids) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Buffer pool stats clear failed - null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(buffer_pool_id) != SAI_OBJECT_TYPE_BUFFER_POOL) {
    SAI_LOG_ERROR(
        "Buffer pool stats clear failed - invalid buffer pool handle "
        "0x%" PRIx64,
        buffer_pool_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (uint32_t i = 0; i < number_of_counters; i++) {
    const auto counters_it =
        buffer_pool_to_switch_counter_mapping.find(counter_ids[i]);
    if (counters_it != buffer_pool_to_switch_counter_mapping.end() &&
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
      SAI_LOG_ERROR("Failed to clear buffer pool 0x%" PRIx64
                    " stats, status %s",
                    buffer_pool_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Create buffer profile
 * @param[out] buffer_profile_id buffer profile id
 * @param[in] switch_id Switch id
 * @param[in] attr_count number of attributes
 * @param[in] attr_list array of attributes
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_create_buffer_profile(_Out_ sai_object_id_t *buffer_profile_id,
                                       _In_ sai_object_id_t switch_id,
                                       _In_ uint32_t attr_count,
                                       _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  switch_enum_t mode = {0};
  std::set<attr_w> sw_attrs;
  switch_object_id_t sw_object_id = {0};

  if (!buffer_profile_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *buffer_profile_id = SAI_NULL_OBJECT_ID;

  for (uint32_t i = 0; i < attr_count; i++) {
    attribute = &attr_list[i];
    switch (attribute->id) {
      case SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH:
        mode.enumdata = SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE_DYNAMIC;
        sw_attrs.insert(
            attr_w(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE, mode));
        sw_attrs.insert(attr_w(
            SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD,
            sai_buffer_threshold_to_switch_threshold(attribute->value.u8)));
        break;
      case SAI_BUFFER_PROFILE_ATTR_THRESHOLD_MODE:
        break;
      case SAI_BUFFER_PROFILE_ATTR_SHARED_STATIC_TH:
        mode.enumdata = SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE_STATIC;
        sw_attrs.insert(
            attr_w(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE, mode));
        sw_attrs.insert(
            attr_w(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD, attribute->value.u64));
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_BUFFER_PROFILE, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create buffer profile, attribute %s: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_BUFFER_PROFILE, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_BUFFER_PROFILE_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_BUFFER_PROFILE, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create buffer profile: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *buffer_profile_id = sw_object_id.data;
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove buffer profile
 * @param[in] buffer_profile_id buffer profile id
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_remove_buffer_profile(_In_ sai_object_id_t buffer_profile_id) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = buffer_profile_id};

  SAI_ASSERT(sai_object_type_query(buffer_profile_id) ==
             SAI_OBJECT_TYPE_BUFFER_PROFILE);

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove buffer profile: %s",
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set buffer profile attribute
 * @param[in] buffer_profile_id buffer profile id
 * @param[in] attr attribute
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_set_buffer_profile_attribute(
    _In_ sai_object_id_t buffer_profile_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = buffer_profile_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(buffer_profile_id) ==
             SAI_OBJECT_TYPE_BUFFER_PROFILE);

  switch (attr->id) {
    case SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH: {
      attr_w bp_attr(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD,
                     sai_buffer_threshold_to_switch_threshold(attr->value.u8));
      switch_status = bf_switch_attribute_set(sw_object_id, bp_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_BUFFER_PROFILE_ATTR_SHARED_STATIC_TH: {
      attr_w bp_attr(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD, attr->value.u64);
      switch_status = bf_switch_attribute_set(sw_object_id, bp_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_BUFFER_PROFILE, attr, sw_object_id);
      break;
  }
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to set buffer profile attribute %s for handle %" PRIx64 ": %s",
        sai_attribute_name(SAI_OBJECT_TYPE_BUFFER_PROFILE, attr->id),
        buffer_profile_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return status;
}

uint8_t sai_switch_buffer_threshold_to_sai_threshold(uint64_t threshold) {
  if (threshold == 0) {
    return 0;
  }
  if (threshold >= SWITCH_API_BUFFER_DYNAMIC_MAX_THRESHOLD) {
    return 8;
  }
  return threshold / SWITCH_API_BUFFER_DYNAMIC_THRESHOLD_FACTOR;
}

/**
 * @brief Get buffer profile attributes
 * @param[in] buffer_profile_id buffer profile id
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list array of attributes
 * @return SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_get_buffer_profile_attribute(
    _In_ sai_object_id_t buffer_profile_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = buffer_profile_id};
  uint32_t i = 0;
  sai_attribute_t *sai_attr;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(buffer_profile_id) ==
             SAI_OBJECT_TYPE_BUFFER_PROFILE);

  for (i = 0, sai_attr = attr_list; i < attr_count; i++, sai_attr++) {
    switch (sai_attr->id) {
      case SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH:
      case SAI_BUFFER_PROFILE_ATTR_SHARED_STATIC_TH: {
        uint64_t threshold = 0;
        attr_w sw_attr(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD, sw_attr);
        status = status_switch_to_sai(switch_status);
        sw_attr.v_get(threshold);
        if (sai_attr->id == SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH)
          sai_attr->value.u8 =
              sai_switch_buffer_threshold_to_sai_threshold(threshold);
        else
          sai_attr->value.u64 = threshold;
        break;
      }
      case SAI_BUFFER_PROFILE_ATTR_THRESHOLD_MODE: {
        switch_enum_t mode = {0};
        attr_w sw_attr(SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_BUFFER_PROFILE_ATTR_THRESHOLD_MODE, sw_attr);
        status = status_switch_to_sai(switch_status);
        sw_attr.v_get(mode);
        sai_attr->value.u32 = static_cast<uint32_t>(mode.enumdata);
        break;
      }

      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_BUFFER_PROFILE, sw_object_id, sai_attr);
        break;
    }
  }
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to get buffer profile info for profile handle 0x%" PRIx64
        ": %s",
        buffer_profile_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return status;
}

sai_status_t sai_buffer_profiles_set(switch_object_id_t port_handle,
                                     switch_object_id_t ingress_profile_handle,
                                     switch_object_id_t egress_profile_handle) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  std::vector<switch_object_id_t> queue_handles;
  attr_w queue_list(SWITCH_PORT_ATTR_QUEUE_HANDLES);
  status = bf_switch_attribute_get(
      port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_list);
  queue_list.v_get(queue_handles);
  attr_w q_attr(SWITCH_QUEUE_ATTR_BUFFER_PROFILE_HANDLE, egress_profile_handle);
  for (auto oid : queue_handles) {
    switch_status = bf_switch_attribute_set(oid, q_attr);
    if (switch_status != SWITCH_STATUS_SUCCESS)
      return status_switch_to_sai(switch_status);
  }

  /*
  switch_status = switch_api_port_max_ppg_get(device, port_handle, &num_ppgs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get port's max PPG for port 0x%lx: %s",
                  port_handle,
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (num_ppgs == 0) {
    SAI_LOG_ERROR("Num ppg is zero, may be internal ports");
    return status;
  }

  ppg_handles =
      (switch_handle_t *)SAI_MALLOC(num_ppgs * sizeof(switch_handle_t));
  switch_status =
      switch_api_port_ppg_get(device, port_handle, &num_ppgs, ppg_handles);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get port's PPG handles for port 0x%lx: %s",
                  port_handle,
                  sai_metadata_get_status_name(status));
    SAI_FREE(ppg_handles);
    return status;
  }
  for (index = 0; index < num_ppgs; index++) {
    status = switch_api_priority_group_buffer_profile_set(
        device, ppg_handles[index], ingress_profile_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set buffer profile to PPG 0x%lx",
                    ppg_handles[index]);
      SAI_FREE(ppg_handles);
      return status;
    }
  }
  SAI_FREE(ppg_handles);
  */
  return status;
}

/*
 *  Buffer methods table retrieved with sai_api_query()
 */
sai_buffer_api_t buffer_api = {
  create_buffer_pool : sai_create_buffer_pool,
  remove_buffer_pool : sai_remove_buffer_pool,
  set_buffer_pool_attribute : sai_set_buffer_pool_attribute,
  get_buffer_pool_attribute : sai_get_buffer_pool_attribute,
  get_buffer_pool_stats : sai_get_buffer_pool_stats,
  get_buffer_pool_stats_ext : NULL,
  clear_buffer_pool_stats : sai_clear_buffer_pool_stats,
  create_ingress_priority_group : sai_create_ingress_priority_group,
  remove_ingress_priority_group : sai_remove_ingress_priority_group,
  set_ingress_priority_group_attribute :
      sai_set_ingress_priority_group_attribute,
  get_ingress_priority_group_attribute :
      sai_get_ingress_priority_group_attribute,
  get_ingress_priority_group_stats : sai_get_ingress_priority_group_stats,
  get_ingress_priority_group_stats_ext :
      sai_get_ingress_priority_group_stats_ext,
  clear_ingress_priority_group_stats : sai_clear_ingress_priority_group_stats,
  create_buffer_profile : sai_create_buffer_profile,
  remove_buffer_profile : sai_remove_buffer_profile,
  set_buffer_profile_attribute : sai_set_buffer_profile_attribute,
  get_buffer_profile_attribute : sai_get_buffer_profile_attribute
};

sai_buffer_api_t *sai_buffer_api_get() { return &buffer_api; }

sai_status_t sai_buffer_initialize(bool warm_init) {
  if (warm_init) return SAI_STATUS_SUCCESS;

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  device_handle = sai_get_device_id(0);
  switch_object_id_t ingress_buffer_pool = {0}, egress_buffer_pool = {0};
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint64_t pool_size = 0;

  SAI_LOG_DEBUG("Initializing buffer");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_BUFFER_POOL);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_BUFFER_PROFILE);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP);

  // fetch the default buffer pool size from the device object
  attr_w pool_size_attr(SWITCH_DEVICE_ATTR_DEFAULT_BUFFER_POOL_SIZE);
  switch_status =
      bf_switch_attribute_get(device_handle,
                              SWITCH_DEVICE_ATTR_DEFAULT_BUFFER_POOL_SIZE,
                              pool_size_attr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to retrieve device default buffer pool size");
    return status_switch_to_sai(switch_status);
  }
  pool_size_attr.v_get(pool_size);

  for (int i = 0; i < 2; i++) {
    std::set<attr_w> buffer_pool_attrs;
    switch_enum_t th_mode = {
        .enumdata = SWITCH_BUFFER_POOL_ATTR_THRESHOLD_MODE_DYNAMIC};

    buffer_pool_attrs.insert(
        attr_w(SWITCH_BUFFER_POOL_ATTR_DEVICE, device_handle));
    buffer_pool_attrs.insert(
        attr_w(SWITCH_BUFFER_POOL_ATTR_THRESHOLD_MODE, th_mode));
    buffer_pool_attrs.insert(
        attr_w(SWITCH_BUFFER_POOL_ATTR_POOL_SIZE, pool_size));
    if (i == 0) {
      switch_enum_t dir = {.enumdata =
                               SWITCH_BUFFER_POOL_ATTR_DIRECTION_INGRESS};
      buffer_pool_attrs.insert(attr_w(SWITCH_BUFFER_POOL_ATTR_DIRECTION, dir));
      switch_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_BUFFER_POOL,
                                              buffer_pool_attrs,
                                              ingress_buffer_pool);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Default ingress buffer pool create failed");
        return status_switch_to_sai(switch_status);
      }
    } else {
      // pool size comes from json
      switch_enum_t dir = {.enumdata =
                               SWITCH_BUFFER_POOL_ATTR_DIRECTION_EGRESS};
      buffer_pool_attrs.insert(attr_w(SWITCH_BUFFER_POOL_ATTR_DIRECTION, dir));
      switch_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_BUFFER_POOL,
                                              buffer_pool_attrs,
                                              egress_buffer_pool);
      if (status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Default egress buffer pool create failed");
        return status_switch_to_sai(switch_status);
      }
    }
  }
  return SAI_STATUS_SUCCESS;
}
