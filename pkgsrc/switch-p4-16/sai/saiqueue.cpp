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

#include <list>
#include <vector>

static sai_api_t api_id = SAI_API_QUEUE;
static switch_object_id_t device_handle = {0};

static const sai_to_switch_counters_map queue_to_switch_counter_mapping{
    {SAI_QUEUE_STAT_PACKETS, {SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS}},
    {SAI_QUEUE_STAT_BYTES, {SWITCH_QUEUE_COUNTER_ID_STAT_BYTES}},
    {SAI_QUEUE_STAT_DROPPED_PACKETS, {SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS}},
    {SAI_QUEUE_STAT_CURR_OCCUPANCY_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_CURR_OCCUPANCY_BYTES}},
    {SAI_QUEUE_STAT_WATERMARK_BYTES, {SWITCH_QUEUE_COUNTER_ID_WATERMARK_BYTES}},
    {SAI_QUEUE_STAT_SHARED_CURR_OCCUPANCY_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES}},
    {SAI_QUEUE_STAT_SHARED_WATERMARK_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_SHARED_WATERMARK_BYTES}},
    {SAI_QUEUE_STAT_GREEN_WRED_DROPPED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_PACKETS}},
    {SAI_QUEUE_STAT_YELLOW_WRED_DROPPED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_PACKETS}},
    {SAI_QUEUE_STAT_RED_WRED_DROPPED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_PACKETS}},
    {SAI_QUEUE_STAT_WRED_DROPPED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_PACKETS}},
    {SAI_QUEUE_STAT_GREEN_WRED_DROPPED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_DROPPED_BYTES}},
    {SAI_QUEUE_STAT_YELLOW_WRED_DROPPED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_DROPPED_BYTES}},
    {SAI_QUEUE_STAT_RED_WRED_DROPPED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_RED_DROPPED_BYTES}},
    {SAI_QUEUE_STAT_WRED_DROPPED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_DROPPED_BYTES}},
    {SAI_QUEUE_STAT_GREEN_WRED_ECN_MARKED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_PACKETS}},
    {SAI_QUEUE_STAT_YELLOW_WRED_ECN_MARKED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_PACKETS}},
    {SAI_QUEUE_STAT_RED_WRED_ECN_MARKED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_PACKETS}},
    {SAI_QUEUE_STAT_WRED_ECN_MARKED_PACKETS,
     {SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_PACKETS}},
    {SAI_QUEUE_STAT_GREEN_WRED_ECN_MARKED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_GREEN_ECN_MARKED_BYTES}},
    {SAI_QUEUE_STAT_YELLOW_WRED_ECN_MARKED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_YELLOW_ECN_MARKED_BYTES}},
    {SAI_QUEUE_STAT_RED_WRED_ECN_MARKED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_RED_ECN_MARKED_BYTES}},
    {SAI_QUEUE_STAT_WRED_ECN_MARKED_BYTES,
     {SWITCH_QUEUE_COUNTER_ID_WRED_ECN_MARKED_BYTES}},
    {SAI_QUEUE_STAT_DROPPED_BYTES, {}}  // Unsupported
};

sai_status_t query_queue_stats_capability(
    sai_stat_capability_list_t &stats_capability) {
  static const uint16_t supported_count =
      supported_counters_count(queue_to_switch_counter_mapping);
  return query_stats_capability_by_mapping(
      queue_to_switch_counter_mapping, stats_capability, supported_count);
}

/**
 * @brief Set attribute to Queue
 * @param[in] queue_id queue id to set the attribute
 * @param[in] attr attribute to set
 *
 * @return  SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_set_queue_attribute(_In_ sai_object_id_t queue_id,
                                     _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = queue_id};

  if (!attr) {
    SAI_LOG_ERROR("Queue attribute set failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  SAI_ASSERT(sai_object_type_query(queue_id) == SAI_OBJECT_TYPE_QUEUE);
  status =
      sai_to_switch_attribute_set(SAI_OBJECT_TYPE_QUEUE, attr, sw_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set queue attribute %s for 0x%" PRIx64 ": %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_QUEUE, attr->id),
                  queue_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/**
 * @brief Get attribute to Queue
 * @param[in] queue_id queue id to set the attribute
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return  SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_get_queue_attribute(_In_ sai_object_id_t queue_id,
                                     _In_ uint32_t attr_count,
                                     _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_attribute_t *attr = attr_list;
  uint32_t i = 0;
  switch_object_id_t sw_object_id = {.data = queue_id};

  if (!attr_list) {
    SAI_LOG_ERROR("Queue attribute get failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  SAI_ASSERT(sai_object_type_query(queue_id) == SAI_OBJECT_TYPE_QUEUE);

  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    memset(&(attr->value), 0, sizeof(attr->value));
    switch (attr->id) {
      case SAI_QUEUE_ATTR_TYPE:
        attr->value.s32 = SAI_QUEUE_TYPE_ALL;
        break;
      case SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE: {
        switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
        attr_w port_attr(SWITCH_QUEUE_ATTR_PORT_HANDLE);
        switch_object_id_t port_handle = {};
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get queue attribute object_id: 0x%" PRIx64
                        " attribute %s "
                        "error: %s",
                        queue_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_QUEUE, attr->id),
                        sai_metadata_get_status_name(status));
        }
        port_attr.v_get(port_handle);
        attr->value.oid = port_handle.data;
      } break;
      case SAI_QUEUE_ATTR_TAM_OBJECT:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
      case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
      case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_QUEUE, sw_object_id, attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get queue attribute object_id: 0x%" PRIx64
                        " attribute %s "
                        "error: %s",
                        queue_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_QUEUE, attr->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief   Clear queue statistics counters.
 *
 * @param[in] queue_id Queue id
 * @param[in] counter_ids specifies the array of counter ids
 * @param[in] number_of_counters number of counters in the array
 *
 * @return SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_clear_queue_stats(_In_ sai_object_id_t queue_id,
                                   _In_ uint32_t number_of_counters,
                                   _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::list<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("Queue stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(queue_id) != SAI_OBJECT_TYPE_QUEUE) {
    SAI_LOG_ERROR("Queue stats clear failed: invalid queue handle 0x%" PRIx64,
                  queue_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = queue_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    const auto counters_it =
        queue_to_switch_counter_mapping.find(counter_ids[i]);
    if (counters_it != queue_to_switch_counter_mapping.end() &&
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
      SAI_LOG_ERROR("Failed to clear queue 0x%" PRIx64 " stats, status %s",
                    queue_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief   Get queue statistics counters.
 *
 * @param[in] queue_id Queue id
 * @param[in] counter_ids specifies the array of counter ids
 * @param[in] number_of_counters number of counters in the array
 * @param[out] counters array of resulting counter values.
 *
 * @return SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_get_queue_stats(_In_ sai_object_id_t queue_id,
                                 _In_ uint32_t number_of_counters,
                                 _In_ const sai_stat_id_t *counter_ids,
                                 _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;
  switch_object_id_t sw_object_id = {.data = queue_id};

  if (!counter_ids || !counters) {
    SAI_LOG_ERROR("Queue stats get failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(queue_id) != SAI_OBJECT_TYPE_QUEUE) {
    SAI_LOG_ERROR("Queue stats get failed: invalid queue handle 0x%" PRIx64,
                  queue_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  sw_status = bf_switch_counters_get(sw_object_id, cntrs);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get queue 0x%" PRIx64 " stats: %s",
                  queue_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < number_of_counters; i++) {
    counters[i] = get_counters_count_by_id(
        queue_to_switch_counter_mapping, counter_ids[i], cntrs);
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * @brief   Get queue statistics counters extended.
 *
 * @param[in] queue_id Queue id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[in] mode Statistics mode
 * @param[out] counters Array of resulting counter values.
 *
 * @return SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_get_queue_stats_ext(_In_ sai_object_id_t queue_id,
                                     _In_ uint32_t number_of_counters,
                                     _In_ const sai_stat_id_t *counter_ids,
                                     _In_ sai_stats_mode_t mode,
                                     _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  sai_get_queue_stats(queue_id, number_of_counters, counter_ids, counters);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get queue stats: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (mode == SAI_STATS_MODE_READ_AND_CLEAR) {
    sai_clear_queue_stats(queue_id, number_of_counters, counter_ids);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear queue stats: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Create queue
 *
 * @param[out] queue_id Queue id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_queue(_Out_ sai_object_id_t *queue_id,
                              _In_ sai_object_id_t switch_id,
                              _In_ uint32_t attr_count,
                              _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;
  uint8_t index = UINT8_MAX;
  switch_object_id_t port_handle = {0};

  if (!attr_list) {
    SAI_LOG_ERROR("Queue create failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_QUEUE_ATTR_TYPE:
        if (attr_list[i].value.s32 != SAI_QUEUE_TYPE_ALL) {
          SAI_LOG_ERROR("Queue create failed: supports only queue type all");
          return SAI_STATUS_NOT_SUPPORTED;
        }
        break;
      case SAI_QUEUE_ATTR_PORT:
        port_handle.data = attr_list[i].value.oid;
        break;
      case SAI_QUEUE_ATTR_INDEX:
        index = attr_list[i].value.u8;
        break;
      case SAI_QUEUE_ATTR_TAM_OBJECT:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      default:
        break;
    }
  }

  if (index == UINT8_MAX) {
    SAI_LOG_ERROR("Queue create failed: invalid queue index");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!port_handle.data) {
    SAI_LOG_ERROR(
        "Queue create failed: mandatory attibute SAI_QUEUE_ATTR_PORT is "
        "missing");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  std::vector<switch_object_id_t> queues;
  attr_w sw_attr(SWITCH_PORT_ATTR_QUEUE_HANDLES);

  status = bf_switch_attribute_get(
      port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, sw_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status_switch_to_sai(status);
  }

  sw_attr.v_get(queues);

  for (auto queue : queues) {
    uint8_t qid = UINT8_MAX;
    attr_w qid_attr(SWITCH_QUEUE_ATTR_QUEUE_ID);

    status =
        bf_switch_attribute_get(queue, SWITCH_QUEUE_ATTR_QUEUE_ID, qid_attr);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status_switch_to_sai(status);
    }

    qid_attr.v_get(qid);
    if (qid == index) {
      return SAI_STATUS_ITEM_ALREADY_EXISTS;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Remove queue
 *
 * @param[in] queue_id Queue id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_queue(_In_ sai_object_id_t queue_id) {
  return SAI_STATUS_NOT_SUPPORTED;
}

/*
 *  Queue  methods table retrieved with sai_api_query()
 */
sai_queue_api_t queue_api = {
  create_queue : sai_create_queue,
  remove_queue : sai_remove_queue,
  set_queue_attribute : sai_set_queue_attribute,
  get_queue_attribute : sai_get_queue_attribute,
  get_queue_stats : sai_get_queue_stats,
  get_queue_stats_ext : sai_get_queue_stats_ext,
  clear_queue_stats : sai_clear_queue_stats
};

sai_queue_api_t *sai_queue_api_get() { return &queue_api; }

sai_status_t sai_queue_initialize() {
  device_handle = sai_get_device_id(0);
  SAI_LOG_DEBUG("Initializing queue map");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_QUEUE);
  return SAI_STATUS_SUCCESS;
}
