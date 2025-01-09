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

static sai_api_t api_id = SAI_API_ISOLATION_GROUP;

/**
 * Routine Description:
 *     Create a isolation group
 *
 * Arguments:
 *     [out] isolation_group_id ISOLATION GROUP ID
 *     [in] switch_id Switch id
 *     [in] attr_count Number of attributes
 *     [in] attr_list Array of attributes
 *
 * Return Values:
 *     SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_isolation_group_entry(
    _Out_ sai_object_id_t *isolation_group_object_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_ISOLATION_GROUP;
  switch_object_id_t switch_isolation_group_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  if (!isolation_group_object_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *isolation_group_object_id = SAI_NULL_OBJECT_ID;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ISOLATION_GROUP, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create isolation group: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_ISOLATION_GROUP_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_isolation_group_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create isolation group: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *isolation_group_object_id = switch_isolation_group_object_id.data;
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove an isolation group
 *
 * Arguments:
 *    [in] sai_object_id_t isolation group id - a handle
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_isolation_group_entry(
    _In_ sai_object_id_t isolation_group_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(isolation_group_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = isolation_group_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove isolation group handle %" PRIx64 ": %s",
                  isolation_group_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set isolation group attribute Value
 *
 * Arguments:
 *    [in] isolation_group_id - isolation group id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_isolation_group_entry_attribute(
    _In_ sai_object_id_t isolation_group_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(isolation_group_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = isolation_group_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ISOLATION_GROUP, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_ISOLATION_GROUP, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get ISOLATION_GROUP attribute Value
 *
 * Arguments:
 *    [in] isolation_group_id - ISOLATION_GROUP id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_isolation_group_entry_attribute(
    _In_ sai_object_id_t isolation_group_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(isolation_group_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = isolation_group_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ISOLATION_GROUP, sw_object_id, &attr_list[i]);
        break;
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get isolation_group attribute object_id: 0x%" PRIx64
          "attribute %s "
          "error: %s",
          isolation_group_id,
          sai_attribute_name(SAI_OBJECT_TYPE_ISOLATION_GROUP, attr_list[i].id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
    \brief Create isolation group Member
    \param[out] isolation_group_member_id isolation group member ID
    \param[in] switch_id Switch id
    \param[in] attr_count number of attributes
    \param[in] attr_list array of attributes
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_create_isolation_group_member(
    _Out_ sai_object_id_t *isolation_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_ISOLATION_GROUP_MEMBER;
  switch_object_id_t isolation_group_member_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  if (!isolation_group_member_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *isolation_group_member_id = SAI_NULL_OBJECT_ID;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create isolation_group_member, attribute get failed: "
              "%s",
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(
      0, SWITCH_ISOLATION_GROUP_MEMBER_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_attrs, isolation_group_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create isolation_group_member: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *isolation_group_member_id = isolation_group_member_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Remove ISOLATION_GROUP Member
    \param[in] isolation_group_member_id ISOLATION_GROUP member ID
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_remove_isolation_group_member(
    _In_ sai_object_id_t isolation_group_member_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(isolation_group_member_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }
  const switch_object_id_t sw_object_id = {.data = isolation_group_member_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove isolation_group_member handle %" PRIx64
                  ": %s",
                  isolation_group_member_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Set ISOLATION_GROUP Member Attribute
    \param[in] isolation_group_member_id ISOLATION_GROUP member ID
    \param[in] attr attribute structure containing ID and value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_set_isolation_group_member_attribute(
    _In_ sai_object_id_t isolation_group_member_id,
    _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t isolation_group_member_handle = {
      .data = isolation_group_member_id};
  SAI_LOG_ENTER();

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(isolation_group_member_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  status = sai_to_switch_attribute_set(SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER,
                                       attr,
                                       isolation_group_member_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to set isolation_group member attribute %s, oid 0x%" PRIx64
        ": %s",
        sai_attribute_name(SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER, attr->id),
        isolation_group_member_id,
        sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Get ISOLATION_GROUP Member Attribute
    \param[in] isolation_group_member_id ISOLATION_GROUP member ID
    \param[in] attr_count number of attributes
    \param[in,out] attr_list list of attribute structures containing ID and
   value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_get_isolation_group_member_attribute(
    _In_ sai_object_id_t isolation_group_member_id,
    _In_ const uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(isolation_group_member_id) !=
      SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, isolation_group_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = isolation_group_member_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status =
            sai_to_switch_attribute_get(SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER,
                                        sw_object_id,
                                        &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get isolation_group member attribute object_id: "
              "0x%" PRIx64
              "attribute "
              "%s "
              "error: %s",
              isolation_group_member_id,
              sai_attribute_name(SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER,
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

/*
 * ISOLATION_GROUP methods table retrieved with sai_api_query()
 */

sai_isolation_group_api_t isolation_group_api = {
  create_isolation_group : sai_create_isolation_group_entry,
  remove_isolation_group : sai_remove_isolation_group_entry,
  set_isolation_group_attribute : sai_set_isolation_group_entry_attribute,
  get_isolation_group_attribute : sai_get_isolation_group_entry_attribute,
  create_isolation_group_member : sai_create_isolation_group_member,
  remove_isolation_group_member : sai_remove_isolation_group_member,
  set_isolation_group_member_attribute :
      sai_set_isolation_group_member_attribute,
  get_isolation_group_member_attribute :
      sai_get_isolation_group_member_attribute,
};

sai_isolation_group_api_t *sai_isolation_group_api_get() {
  return &isolation_group_api;
}

sai_status_t sai_isolation_group_initialize() {
  SAI_LOG_DEBUG("Initializing isolation_group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ISOLATION_GROUP);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_ISOLATION_GROUP_MEMBER);

  return SAI_STATUS_SUCCESS;
}
