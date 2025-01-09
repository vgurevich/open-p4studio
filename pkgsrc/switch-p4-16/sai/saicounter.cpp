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

#include <set>
#include <vector>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_COUNTER;

/**
 * @brief Create counter
 *
 * @param[out] counter_id Counter id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success
 */
sai_status_t sai_create_counter(_Out_ sai_object_id_t *counter_id,
                                _In_ sai_object_id_t switch_id,
                                _In_ uint32_t attr_count,
                                _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!counter_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("No counter ID passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *counter_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t counter_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_COUNTER, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create counter: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_GENERIC_COUNTER_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_GENERIC_COUNTER, sw_attrs, counter_object_id);

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create counter: %s",
                  sai_metadata_get_status_name(status));
  }
  *counter_id = counter_object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove counter
 *
 * @param[in] counter_id Counter id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_counter(_In_ sai_object_id_t counter_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(counter_id) != SAI_OBJECT_TYPE_COUNTER) {
    SAI_LOG_ERROR("Counter remove failed: invalid counter handle 0x%" PRIx64
                  "\n",
                  counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = counter_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove counter 0x%" PRIx64 ": %s",
                  counter_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set counter attribute Value
 *
 * @param[in] counter_id Counter id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_counter_attribute(_In_ sai_object_id_t counter_id,
                                       _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(counter_id) != SAI_OBJECT_TYPE_COUNTER) {
    SAI_LOG_ERROR(
        "Counter attribute set failed: invalid counter handle 0x%" PRIx64 "\n",
        counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = counter_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_COUNTER, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_COUNTER, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get counter attribute Value
 *
 * @param[in] counter_id Counter id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_counter_attribute(_In_ sai_object_id_t counter_id,
                                       _In_ uint32_t attr_count,
                                       _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_count || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(counter_id) != SAI_OBJECT_TYPE_COUNTER) {
    SAI_LOG_ERROR(
        "Counter attribute set failed: invalid counter handle 0x%" PRIx64 "\n",
        counter_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = counter_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_COUNTER, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_COUNTER, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  return status;
}

/**
 * @brief Get switch packets counter id for given object type
 */
// Here to add mapping for counters supported by both switch and SAI
// possible in SAI: fdb, hostif, neighbor, nexthop, nexthop group member,
// route
uint64_t sai_ot_to_sw_pkts_counter_id(sai_object_id_t oid) {
  sai_object_type_t ot = sai_object_type_query(oid);
  switch (ot) {
    case SAI_OBJECT_TYPE_HOSTIF:
      return SWITCH_HOSTIF_COUNTER_ID_RX_PKT;
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      return SWITCH_MPLS_COUNTER_ID_PKTS;
#if SAI_API_VERSION >= 10901
    case SAI_OBJECT_TYPE_MY_SID_ENTRY:
      return SWITCH_MY_SID_ENTRY_COUNTER_ID_PKTS;
#endif
    default:
      SAI_LOG_ERROR("Unknown mapping from object type %s to packets counter ID",
                    sai_object_name_query(oid).c_str());
      return 0;
  }
}

/**
 * @brief Get switch bytes counter id for given object type
 */
// Here to add mapping for counters supported by both switch and SAI
// possible in SAI: fdb, hostif, neighbor, nexthop, nexthop group member,
// route
uint64_t sai_ot_to_sw_bytes_counter_id(sai_object_id_t oid) {
  sai_object_type_t ot = sai_object_type_query(oid);
  switch (ot) {
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      return SWITCH_MPLS_COUNTER_ID_BYTES;
#if SAI_API_VERSION >= 10901
    case SAI_OBJECT_TYPE_MY_SID_ENTRY:
      return SWITCH_MY_SID_ENTRY_COUNTER_ID_BYTES;
#endif
    default:
      SAI_LOG_ERROR("Unknown mapping from object type %s to bytes counter ID",
                    sai_object_name_query(oid).c_str());
      return 0;
  }
}

/**
 * @brief Get a list of objects the counter refers to
 *
 * @param counter_id SAI counter oid
 * @return std::vector<sai_object_id_t> list of object the counter refers to
 */
std::vector<sai_object_id_t> sai_get_counter_references(
    sai_object_id_t counter_id) {
  std::vector<sai_object_id_t> references;
  switch_object_id_t sw_counter_id = {.data = counter_id};

  auto const &mpls_set = switch_store::get_object_references(
      sw_counter_id, SWITCH_OBJECT_TYPE_MPLS);

  for (const auto item : mpls_set) {
    references.push_back(item.oid.data);
  }

  auto const &my_sid_set = switch_store::get_object_references(
      sw_counter_id, SWITCH_OBJECT_TYPE_MY_SID_ENTRY);

  for (const auto item : my_sid_set) {
    references.push_back(item.oid.data);
  }

  // Here to add retrieving of other supported object types
  // (getting references and adding them to the list)

  return references;
}

/**
 * @brief Get counter statistics counters. Deprecated for backward
 * compatibility.
 *
 * @param[in] counter_id Counter id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_counter_stats(_In_ sai_object_id_t counter_id,
                                   _In_ uint32_t number_of_counters,
                                   _In_ const sai_stat_id_t *counter_ids,
                                   _Out_ uint64_t *counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(counter_id) != SAI_OBJECT_TYPE_COUNTER) {
    SAI_LOG_ERROR("Invalid object type 0x%" PRIx64, counter_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  // retrieve objects for which statistics are counted
  std::vector<sai_object_id_t> cntr_refs =
      sai_get_counter_references(counter_id);

  for (uint32_t i = 0; i < number_of_counters; i++) {
    counters[i] = 0;
  }

  for (const auto oid : cntr_refs) {
    uint64_t pkts_cntr_id = 0xFF, bytes_cntr_id = 0xFF;
    std::vector<switch_counter_t> object_cntrs;
    switch_object_id_t sw_object_id = {.data = oid};

    pkts_cntr_id = sai_ot_to_sw_pkts_counter_id(oid);
    bytes_cntr_id = sai_ot_to_sw_bytes_counter_id(oid);

    switch_status = bf_switch_counters_get(sw_object_id, object_cntrs);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get switch counters for %s",
                    sai_object_name_query(oid).c_str());
      return status;
    }

    for (uint32_t i = 0; i < number_of_counters; i++) {
      switch (counter_ids[i]) {
        case SAI_COUNTER_STAT_PACKETS:
          if (pkts_cntr_id != 0xFF) {
            counters[i] += object_cntrs[pkts_cntr_id].count;
          } else {
            status = SAI_STATUS_NOT_SUPPORTED;
            SAI_LOG_WARN("Packets stats for %s not supported",
                         sai_object_name_query(oid).c_str());
          }
          break;
        case SAI_COUNTER_STAT_BYTES:
          if (bytes_cntr_id != 0xFF) {
            counters[i] += object_cntrs[bytes_cntr_id].count;
          } else {
            status = SAI_STATUS_NOT_SUPPORTED;
            SAI_LOG_WARN("Bytes stats for %s not supported",
                         sai_object_name_query(oid).c_str());
          }
          break;
        case SAI_COUNTER_STAT_CUSTOM_RANGE_BASE:
          break;  // just ignore this type
        default:
          SAI_LOG_ERROR("Invalid counter ID");
          return SAI_STATUS_INVALID_OBJECT_ID;
      }
    }
  }

  return status;
}

/**
 * @brief Get counter statistics counters extended.
 *
 * @param[in] counter_id Counter id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[in] mode Statistics mode
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_counter_stats_ext(_In_ sai_object_id_t counter_id,
                                       _In_ uint32_t number_of_counters,
                                       _In_ const sai_stat_id_t *counter_ids,
                                       _In_ sai_stats_mode_t mode,
                                       _Out_ uint64_t *counters) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Clear counter statistics counters.
 *
 * @param[in] counter_id Counter id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_clear_counter_stats(_In_ sai_object_id_t counter_id,
                                     _In_ uint32_t number_of_counters,
                                     _In_ const sai_stat_id_t *counter_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!number_of_counters || !counter_ids) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(counter_id) != SAI_OBJECT_TYPE_COUNTER) {
    SAI_LOG_ERROR("Invalid object type 0x%" PRIx64, counter_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  // retrieve objects for which statistics are counted
  std::vector<sai_object_id_t> cntr_refs =
      sai_get_counter_references(counter_id);

  for (const auto oid : cntr_refs) {
    uint64_t pkts_cntr_id = 0xFF, bytes_cntr_id = 0xFF;
    switch_object_id_t sw_object_id = {.data = oid};
    std::vector<uint16_t> sw_cntr_ids;

    pkts_cntr_id = sai_ot_to_sw_pkts_counter_id(oid);
    bytes_cntr_id = sai_ot_to_sw_bytes_counter_id(oid);

    for (uint32_t i = 0; i < number_of_counters; i++) {
      switch (counter_ids[i]) {
        case SAI_COUNTER_STAT_PACKETS:
          if (pkts_cntr_id != 0xFF) {
            sw_cntr_ids.push_back(pkts_cntr_id);
          } else {
            status = SAI_STATUS_NOT_SUPPORTED;
            SAI_LOG_WARN("Packets stats for %s not supported",
                         sai_object_name_query(oid).c_str());
          }
          break;
        case SAI_COUNTER_STAT_BYTES:
          if (bytes_cntr_id != 0xFF) {
            sw_cntr_ids.push_back(bytes_cntr_id);
          } else {
            status = SAI_STATUS_NOT_SUPPORTED;
            SAI_LOG_WARN("Bytes stats for %s not supported",
                         sai_object_name_query(oid).c_str());
          }
          break;
        case SAI_COUNTER_STAT_CUSTOM_RANGE_BASE:
          break;  // just ignore this type
        default:
          SAI_LOG_ERROR("Invalid counter ID");
          return SAI_STATUS_INVALID_OBJECT_ID;
      }
    }

    if (sw_cntr_ids.size()) {
      switch_status = bf_switch_counters_clear(sw_object_id, sw_cntr_ids);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to clear counters for %s",
                      sai_object_name_query(oid).c_str());
        return status;
      }
    }
  }

  return status;
}

/*
 * Counters methods table retrieved with sai_api_query()
 */
sai_counter_api_t counter_api = {
  create_counter : sai_create_counter,
  remove_counter : sai_remove_counter,
  set_counter_attribute : sai_set_counter_attribute,
  get_counter_attribute : sai_get_counter_attribute,
  get_counter_stats : sai_get_counter_stats,
  get_counter_stats_ext : sai_get_counter_stats_ext,
  clear_counter_stats : sai_clear_counter_stats
};

sai_counter_api_t *sai_counter_api_get() { return &counter_api; }

sai_status_t sai_counter_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_COUNTER);
  return status;
}
