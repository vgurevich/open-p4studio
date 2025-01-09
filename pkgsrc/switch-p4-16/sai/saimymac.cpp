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


#include <set>

#include "./saiinternal.h"

static sai_api_t api_id = SAI_API_MY_MAC;

/**
 * @brief Create My MAC entry.
 *
 * @param[out] my_mac_id My MAC id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_my_mac(_Out_ sai_object_id_t *my_mac_id,
                               _In_ sai_object_id_t switch_id,

                               _In_ uint32_t attr_count,
                               _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_MY_MAC;
  uint32_t index = 0;

  if (!my_mac_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *my_mac_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_my_mac_object_id = {};
  std::set<smi::attr_w> sw_my_mac_attrs;

  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_MY_MAC, attribute, sw_my_mac_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create my mac: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_MY_MAC_ATTR_DEVICE, sw_my_mac_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_my_mac_attrs, switch_my_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create mymac: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *my_mac_id = switch_my_mac_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Remove My MAC entry
 *
 * @param[in] my_mac_id My MAC Id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_my_mac(_In_ sai_object_id_t my_mac_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(my_mac_id) == SAI_OBJECT_TYPE_MY_MAC);

  const switch_object_id_t switch_my_mac_object_id = {.data = my_mac_id};
  switch_status = bf_switch_object_delete(switch_my_mac_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove mymac %" PRIx64 ": %s",
                  my_mac_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Set My MAC entry attribute
 *
 * @param[in] my_mac_id My MAC id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_my_mac_attribute(_In_ sai_object_id_t my_mac_id,
                                      _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(my_mac_id) == SAI_OBJECT_TYPE_MY_MAC);
  switch (attr->id) {
    default: {
      const switch_object_id_t sw_my_mac_object_id = {.data = my_mac_id};
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_MY_MAC, attr, sw_my_mac_object_id);
      if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_MY_MAC, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Get My MAC entry attribute
 *
 * @param[in] my_mac_id My MAC id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_my_mac_attribute(_In_ sai_object_id_t my_mac_id,
                                      _In_ uint32_t attr_count,
                                      _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_attribute_t *attr = attr_list;
  uint32_t i;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(my_mac_id) == SAI_OBJECT_TYPE_MY_MAC);

  const switch_object_id_t sw_my_mac_object_id = {.data = my_mac_id};
  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_MY_MAC, sw_my_mac_object_id, attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s for my_mac_id: %" PRIx64
                        "error: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_MY_MAC, attr->id),
                        (my_mac_id & 0xFFFF),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  my_mac methods table retrieved with sai_api_query()
 */
sai_my_mac_api_t my_mac_api = {
    .create_my_mac = sai_create_my_mac,
    .remove_my_mac = sai_remove_my_mac,
    .set_my_mac_attribute = sai_set_my_mac_attribute,
    .get_my_mac_attribute = sai_get_my_mac_attribute,
};

sai_my_mac_api_t *sai_my_mac_api_get() { return &my_mac_api; }

sai_status_t sai_my_mac_initialize() {
  SAI_LOG_DEBUG("Initializing mymac");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_MY_MAC);
  return SAI_STATUS_SUCCESS;
}
