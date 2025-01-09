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

static sai_api_t api_id = SAI_API_IPMC_GROUP;

/*
 * Routine Description:
 *    Create IP multicast group
 *
 * Arguments:
 *    [out] ipmc_group_id - IP multicast group ID
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_ipmc_group(_Out_ sai_object_id_t *ipmc_group_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;

  if ((attr_count && !attr_list) || !ipmc_group_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *ipmc_group_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t mc_group_object_id = {};
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_IPMC_GROUP, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for IPMC group, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_IPMC_GROUP_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_IPMC_GROUP, sw_attrs, mc_group_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create IPMC group, error: %s",
                  sai_metadata_get_status_name(status));
  }

  *ipmc_group_id = mc_group_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Remove IP multicast group
 * Arguments:
 *    [in] ipmc_group_id - IP multicast group ID
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_ipmc_group(_In_ sai_object_id_t ipmc_group_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t mc_group_object_id = {.data = ipmc_group_id};

  if (sai_object_type_query(ipmc_group_id) != SAI_OBJECT_TYPE_IPMC_GROUP) {
    SAI_LOG_ERROR(
        "IPMC group remove failed: invalid IPMC group handle 0x%" PRIx64,
        ipmc_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_status = bf_switch_object_delete(mc_group_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove IPMC group: 0x%" PRIx64 ", error: %s",
                  mc_group_object_id.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Set IP multicast group attribute value
 *
 * Arguments:
 *    [in] ipmc_group_id - IP multicast group ID
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_ipmc_group_attribute(_In_ sai_object_id_t ipmc_group_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_group_object_id = {.data = ipmc_group_id};

  if (sai_object_type_query(ipmc_group_id) != SAI_OBJECT_TYPE_IPMC_GROUP) {
    SAI_LOG_ERROR(
        "IPMC group attribute set failed: invalid IPMC group handle "
        "0x%" PRIx64,
        ipmc_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_IPMC_GROUP, attr, mc_group_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set IPMC group 0x%" PRIx64
                      " attribute %s, error: %s",
                      ipmc_group_id,
                      sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Get IP multicast group attribute value
 *
 * Arguments:
 *    [in] ipmc_group_id - IP multicast group ID
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_ipmc_group_attribute(_In_ sai_object_id_t ipmc_group_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_group_object_id = {.data = ipmc_group_id};

  if (sai_object_type_query(ipmc_group_id) != SAI_OBJECT_TYPE_IPMC_GROUP) {
    SAI_LOG_ERROR(
        "IPMC group attribute get failed: invalid IPMC group handle "
        "0x%" PRIx64,
        ipmc_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_IPMC_GROUP, mc_group_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get IPMC group: 0x%" PRIx64
                        " attribute %s, error: %s",
                        ipmc_group_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP,
                                           attr_list[index].id),
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
 * Routine Description:
 *    Create IP multicast group member
 *
 * Arguments:
 *    [out] ipmc_group_member_id - IP multicast group member ID
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_ipmc_group_member(
    _Out_ sai_object_id_t *ipmc_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> sw_attrs;

  if (!attr_list || !ipmc_group_member_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *ipmc_group_member_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t mc_group_member_object_id = {};
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for IPMC group member, error: "
              "%s",
              sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_IPMC_MEMBER_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_IPMC_MEMBER, sw_attrs, mc_group_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create IPMC group member, error: %s",
                  sai_metadata_get_status_name(status));
  }

  *ipmc_group_member_id = mc_group_member_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Remove IP multicast group member
 * Arguments:
 *    [in] ipmc_group_member_id - IP multicast group member ID
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_ipmc_group_member(
    _In_ sai_object_id_t ipmc_group_member_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t mc_group_member_object_id = {.data = ipmc_group_member_id};

  if (sai_object_type_query(ipmc_group_member_id) !=
      SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "IPMC group member remove failed: invalid IPMC group member handle "
        "0x%" PRIx64,
        ipmc_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_status = bf_switch_object_delete(mc_group_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove IPMC group member: 0x%" PRIx64
                  ", error: %s",
                  mc_group_member_object_id.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Set IP multicast group member attribute value
 *
 * Arguments:
 *    [in] ipmc_group_member_id - IP multicast group member ID
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_ipmc_group_member_attribute(
    _In_ sai_object_id_t ipmc_group_member_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_group_member_object_id = {.data = ipmc_group_member_id};

  if (sai_object_type_query(ipmc_group_member_id) !=
      SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "IPMC group member attribute set failed: invalid IPMC group member "
        "handle 0x%" PRIx64,
        ipmc_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER, attr, mc_group_member_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set IPMC group member 0x%" PRIx64
            " attribute %s, error: %s",
            ipmc_group_member_id,
            sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Get IP multicast group member attribute value
 *
 * Arguments:
 *    [in] ipmc_group_member_id - IP multicast group member ID
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_ipmc_group_member_attribute(
    _In_ sai_object_id_t ipmc_group_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_group_member_object_id = {.data = ipmc_group_member_id};

  if (sai_object_type_query(ipmc_group_member_id) !=
      SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "IPMC group member attribute get failed: invalid IPMC group member "
        "handle 0x%" PRIx64,
        ipmc_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER,
                                             mc_group_member_object_id,
                                             &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get IPMC group member: 0x%" PRIx64
                        " attribute %s, error: %s",
                        ipmc_group_member_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER,
                                           attr_list[index].id),
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
 * IP multicast group method table retrieved with sai_api_query()
 */
sai_ipmc_group_api_t ipmc_group_api = {
    .create_ipmc_group = sai_create_ipmc_group,
    .remove_ipmc_group = sai_remove_ipmc_group,
    .set_ipmc_group_attribute = sai_set_ipmc_group_attribute,
    .get_ipmc_group_attribute = sai_get_ipmc_group_attribute,
    .create_ipmc_group_member = sai_create_ipmc_group_member,
    .remove_ipmc_group_member = sai_remove_ipmc_group_member,
    .set_ipmc_group_member_attribute = sai_set_ipmc_group_member_attribute,
    .get_ipmc_group_member_attribute = sai_get_ipmc_group_member_attribute};

sai_ipmc_group_api_t *sai_ipmc_group_api_get() { return &ipmc_group_api; }

sai_status_t sai_ipmc_group_initialize() {
  SAI_LOG_DEBUG("Initializing ipmc group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_IPMC_GROUP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER);
  return SAI_STATUS_SUCCESS;
}
