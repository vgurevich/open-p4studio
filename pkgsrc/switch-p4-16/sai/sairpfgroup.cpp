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

static sai_api_t api_id = SAI_API_RPF_GROUP;

/*
 * Routine Description:
 *    Create RPF interface group
 *
 * Arguments:
 *    [out] rpf_group_id RPF - interface group id
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_rpf_group(_Out_ sai_object_id_t *rpf_group_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t rpf_handle = {};
  std::set<smi::attr_w> sw_attrs;

  if (!rpf_group_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *rpf_group_id = SAI_NULL_OBJECT_ID;

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default: {
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_RPF_GROUP, &attr_list[i], sw_attrs);
        if (status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to create RPF group, attr %s: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
    }
  }

  // use default (the only supported) PIM mode SM
  uint64_t pim_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
  sw_attrs.insert(smi::attr_w(SWITCH_RPF_GROUP_ATTR_PIM_MODE, pim_mode));

  sai_insert_device_attribute(0, SWITCH_RPF_GROUP_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_RPF_GROUP, sw_attrs, rpf_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create RPF group: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *rpf_group_id = rpf_handle.data;
  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Remove RPF interface group
 *
 * Arguments:
 *    [in] rpf_group_id - RPF interface group id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_rpf_group(_In_ sai_object_id_t rpf_group_id) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(rpf_group_id) != SAI_OBJECT_TYPE_RPF_GROUP) {
    SAI_LOG_ERROR(
        "Removing RPF group failed: invalid RPF group handle 0x%" PRIx64 "\n",
        rpf_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  const switch_object_id_t sw_object_id = {.data = rpf_group_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove RPF group %" PRIx64 ": %s",
                  rpf_group_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 * Routine Description:
 *    Set RPF interface group attribute
 *
 * Arguments:
 *    [in] rpf_group_id - RPF interface group id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_rpf_group_attribute(_In_ sai_object_id_t rpf_group_id,
                                         _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(rpf_group_id) != SAI_OBJECT_TYPE_RPF_GROUP) {
    SAI_LOG_ERROR(
        "Setting RPF group attribute failed: invalid RPF group handle "
        "0x%" PRIx64 "\n",
        rpf_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = rpf_group_id};

  switch (attr->id) {
    default: {
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_RPF_GROUP, attr, sw_object_id);
    } break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set RPF group attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Get RPF interface group attribute
 *
 * Arguments:
 *    [in] rpf_group_id - RPF interface group id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_rpf_group_attribute(_In_ sai_object_id_t rpf_group_id,
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

  if (sai_object_type_query(rpf_group_id) != SAI_OBJECT_TYPE_RPF_GROUP) {
    SAI_LOG_ERROR(
        "Getting RPF group attribute failed: invalid RPF group handle "
        "0x%" PRIx64 "\n",
        rpf_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  const switch_object_id_t sw_object_id = {.data = rpf_group_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default: {
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_RPF_GROUP, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get RPF group attribute %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Create RPF interface group member
 *
 * Arguments:
 *    [out] rpf_group_member_id RPF - interface group member id
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_rpf_group_member(
    _Out_ sai_object_id_t *rpf_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t rpf_member_handle = {};
  std::set<smi::attr_w> sw_attrs;

  if (!rpf_group_member_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *rpf_group_member_id = SAI_NULL_OBJECT_ID;

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default: {
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_RPF_GROUP_MEMBER, &attr_list[i], sw_attrs);
        if (status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create RPF group member, attr %s: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP_MEMBER,
                                           attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
      } break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_RPF_MEMBER_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_RPF_MEMBER, sw_attrs, rpf_member_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create RPF group member: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *rpf_group_member_id = rpf_member_handle.data;
  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Remove RPF interface group member
 *
 * Arguments:
 *    [in] rpf_group_member_id - RPF interface group member id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_rpf_group_member(
    _In_ sai_object_id_t rpf_group_member_id) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(rpf_group_member_id) !=
      SAI_OBJECT_TYPE_RPF_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "Removing RPF group failed: invalid RPF group member handle 0x%" PRIx64
        "\n",
        rpf_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  const switch_object_id_t sw_object_id = {.data = rpf_group_member_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove RPF group member %" PRIx64 ": %s",
                  rpf_group_member_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 * Routine Description:
 *    Set RPF interface group member attribute
 *
 * Arguments:
 *    [in] rpf_group_member_id - RPF interface group member id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_rpf_group_member_attribute(
    _In_ sai_object_id_t rpf_group_member_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(rpf_group_member_id) !=
      SAI_OBJECT_TYPE_RPF_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "Setting RPF group member attribute failed: invalid RPF group member "
        "handle "
        "0x%" PRIx64 "\n",
        rpf_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = rpf_group_member_id};

  switch (attr->id) {
    default: {
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_RPF_GROUP_MEMBER, attr, sw_object_id);
    } break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set RPF group member attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP_MEMBER, attr->id),
        sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Get RPF interface group member attribute
 *
 * Arguments:
 *    [in] rpf_group_member_id - RPF interface group member id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_rpf_group_member_attribute(
    _In_ sai_object_id_t rpf_group_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_EXIT();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(rpf_group_member_id) !=
      SAI_OBJECT_TYPE_RPF_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "Getting RPF group member attribute failed: invalid RPF group handle "
        "0x%" PRIx64 "\n",
        rpf_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  const switch_object_id_t sw_object_id = {.data = rpf_group_member_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default: {
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_RPF_GROUP_MEMBER, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get RPF group member attribute %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_RPF_GROUP_MEMBER,
                                 attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
    }
  }

  return status;
}

/*
 *  RPF group methods table retrieved with sai_api_query()
 */
sai_rpf_group_api_t rpf_api = {
  create_rpf_group : sai_create_rpf_group,
  remove_rpf_group : sai_remove_rpf_group,
  set_rpf_group_attribute : sai_set_rpf_group_attribute,
  get_rpf_group_attribute : sai_get_rpf_group_attribute,
  create_rpf_group_member : sai_create_rpf_group_member,
  remove_rpf_group_member : sai_remove_rpf_group_member,
  set_rpf_group_member_attribute : sai_set_rpf_group_member_attribute,
  get_rpf_group_member_attribute : sai_get_rpf_group_member_attribute
};

sai_rpf_group_api_t *sai_rpf_group_api_get() { return &rpf_api; }

sai_status_t sai_rpf_group_initialize() {
  SAI_LOG_DEBUG("Initializing RPF group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_RPF_GROUP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_RPF_GROUP_MEMBER);
  return SAI_STATUS_SUCCESS;
}
