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

static sai_api_t api_id = SAI_API_SAMPLEPACKET;

/*
 *  Routine Description:
 *    Creates samplepacket
 *
 *  Arguments:
 *    [out] samplepacket_id - samplepacket id
 *    [in] switch_id - Switch Id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_samplepacket(_Out_ sai_object_id_t *samplepacket_id,
                                     _In_ sai_object_id_t switch_id,
                                     _In_ uint32_t attr_count,
                                     _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;
  const sai_attribute_t *attribute = NULL;

  if (!samplepacket_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *samplepacket_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    status = SAI_STATUS_SUCCESS;
    attribute = &attr_list[index];

    switch (attribute->id) {
      case SAI_SAMPLEPACKET_ATTR_TYPE:
        if (((sai_samplepacket_type_t)attribute->value.s32) !=
            SAI_SAMPLEPACKET_TYPE_SLOW_PATH) {
          status = SAI_STATUS_NOT_SUPPORTED;
        }
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_SAMPLEPACKET, attribute, sw_attrs);
    }

    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create samplepacket: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  sai_insert_device_attribute(0, SWITCH_SFLOW_SESSION_ATTR_DEVICE, sw_attrs);

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_SFLOW_SESSION, sw_attrs, object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create samplepacket: %s",
                  sai_metadata_get_status_name(status));
  }
  *samplepacket_id = object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Removes samplepacket
 *
 *  Arguments:
 *    [in] samplepacket_id - Samplepacket id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_samplepacket(_In_ sai_object_id_t samplepacket_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(samplepacket_id) != SAI_OBJECT_TYPE_SAMPLEPACKET) {
    SAI_LOG_ERROR(
        "Samplepacket remove failed: invalid samplepacket handle 0x%" PRIx64
        "\n",
        samplepacket_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = samplepacket_id};
  sw_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove samplepacket 0x%" PRIx64 ": %s",
                  samplepacket_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Sets samplepacket attribute
 *
 *  Arguments:
 *    [in] samplepacket_id - Samplepacket id
 *    [in] attr - Attribute
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_samplepacket_attribute(
    _In_ sai_object_id_t samplepacket_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(samplepacket_id) != SAI_OBJECT_TYPE_SAMPLEPACKET) {
    SAI_LOG_ERROR(
        "Samplepacket set failed: invalid samplepacket handle 0x%" PRIx64 "\n",
        samplepacket_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = samplepacket_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_SAMPLEPACKET, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_SAMPLEPACKET, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Gets samplepacket attributes
 *
 *  Arguments:
 *    [in] samplepacket_id - Samplepacket id
 *    [in] attr_count - Number of attributes
 *    [inout] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_samplepacket_attribute(
    _In_ sai_object_id_t samplepacket_id,
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

  if (sai_object_type_query(samplepacket_id) != SAI_OBJECT_TYPE_SAMPLEPACKET) {
    SAI_LOG_ERROR(
        "Samplepacket get failed: invalid samplepacket handle 0x%" PRIx64 "\n",
        samplepacket_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = samplepacket_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_SAMPLEPACKET_ATTR_TYPE:
        attr_list[index].value.s32 = SAI_SAMPLEPACKET_TYPE_SLOW_PATH;
        break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_SAMPLEPACKET, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_SAMPLEPACKET,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

// SAMPLEPACKET method table retrieved with sai_api_query()
sai_samplepacket_api_t samplepacket_api = {
    .create_samplepacket = sai_create_samplepacket,
    .remove_samplepacket = sai_remove_samplepacket,
    .set_samplepacket_attribute = sai_set_samplepacket_attribute,
    .get_samplepacket_attribute = sai_get_samplepacket_attribute};

sai_samplepacket_api_t *sai_samplepacket_api_get() { return &samplepacket_api; }
sai_status_t sai_samplepacket_initialize() {
  SAI_LOG_DEBUG("Initializing samplepacket");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_SAMPLEPACKET);
  return SAI_STATUS_SUCCESS;
}
