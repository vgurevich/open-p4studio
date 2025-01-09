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

static sai_api_t api_id = SAI_API_NEXT_HOP_GROUP;

sai_status_t sai_get_next_hop_group_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;
  const std::vector<sai_next_hop_group_type_t> supported_next_hop_group_types =
      {SAI_NEXT_HOP_GROUP_TYPE_DYNAMIC_UNORDERED_ECMP,
       SAI_NEXT_HOP_GROUP_TYPE_DYNAMIC_ORDERED_ECMP,
       SAI_NEXT_HOP_GROUP_TYPE_FINE_GRAIN_ECMP};

  if (!enum_values_capability) return SAI_STATUS_INVALID_PARAMETER;

  if (attr_id == SAI_NEXT_HOP_GROUP_ATTR_TYPE) {
    if (enum_values_capability->count >=
        supported_next_hop_group_types.size()) {
      for (const auto &type : supported_next_hop_group_types) {
        enum_values_capability->list[i] = type;
        i++;
      }
    } else {
      enum_values_capability->count = supported_next_hop_group_types.size();
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    return status;
  }
  enum_values_capability->count = i;
  return status;
}

/*
 * Routine Description:
 *    Create next hop group
 *
 * Arguments:
 *    [out] next_hop_group_id - next hop group id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_next_hop_group_entry(
    _Out_ sai_object_id_t *next_hop_group_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status;
  const sai_attribute_t *attribute;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_ECMP;
  uint32_t index = 0;

  if (!next_hop_group_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *next_hop_group_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t ecmp_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NEXT_HOP_GROUP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create nexthop_group: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_ECMP_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(ot, sw_attrs, ecmp_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create next hop group : %s",
                  sai_metadata_get_status_name(status));
  }
  *next_hop_group_id = ecmp_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove next hop group
 *
 * Arguments:
 *    [in] next_hop_group_id - next hop group id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_next_hop_group_entry(
    _In_ sai_object_id_t next_hop_group_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(next_hop_group_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP);

  const switch_object_id_t sw_object_id = {.data = next_hop_group_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove nexthop_group 0x%" PRIx64 ": %s",
                  next_hop_group_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set Next Hop Group attribute
 *
 * Arguments:
 *    [in] sai_object_id_t - next_hop_group_id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_next_hop_group_entry_attribute(
    _In_ sai_object_id_t next_hop_group_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(next_hop_group_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP);

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get Next Hop Group attribute
 *
 * Arguments:
 *    [in] sai_object_id_t - next_hop_group_id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_next_hop_group_entry_attribute(
    _In_ sai_object_id_t next_hop_group_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;
  sai_attribute_t *attribute;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(next_hop_group_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP);

  const switch_object_id_t sw_object_id = {.data = next_hop_group_id};
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_COUNT: {
        smi::attr_w ecmp_members_attr(SWITCH_ECMP_ATTR_ECMP_MEMBERS);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP_GROUP, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
        std::vector<switch_object_id_t> ecmp_members;
        ecmp_members_attr.v_get(ecmp_members);
        attribute->value.u32 = static_cast<uint32_t>(ecmp_members.size());
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_NEXT_HOP_GROUP, sw_object_id, attribute);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP_GROUP, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Create next hop group member
 *
 * @param[out] next_hop_group_member_id - next hop group member id
 * @param[in] attr_count - number of attributes
 * @param[in] attr_list - array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_next_hop_group_member(
    _Out_ sai_object_id_t *next_hop_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_ECMP_MEMBER;
  const sai_attribute_t *attribute;
  uint32_t index = 0;
  *next_hop_group_member_id = SAI_NULL_OBJECT_ID;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t ecmp_member_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, attribute, sw_attrs);
        if ((status != SAI_STATUS_SUCCESS) &&
            (status != SAI_STATUS_NOT_SUPPORTED)) {
          SAI_LOG_ERROR("failed to create nexthop_group: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_ECMP_MEMBER_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(ot, sw_attrs, ecmp_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create next hop group : %s",
                  sai_metadata_get_status_name(status));
  }
  *next_hop_group_member_id = ecmp_member_object_id.data;

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/**
 * @brief Remove next hop group member
 *
 * @param[in] next_hop_group_member_id - next hop group member id
 *
 * @return SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_remove_next_hop_group_member(
    _In_ sai_object_id_t next_hop_group_member_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(next_hop_group_member_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER);

  const switch_object_id_t sw_object_id = {.data = next_hop_group_member_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove nexthop_group_member 0x%" PRIx64 ": %s",
                  next_hop_group_member_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/**
 * @brief Set Next Hop Group attribute
 *
 * @param[in] sai_object_id_t - next_hop_group_member_id
 * @param[in] attr - attribute
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_set_next_hop_group_member_attribute(
    _In_ sai_object_id_t next_hop_group_member_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(next_hop_group_member_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER);

  const switch_object_id_t sw_object_id = {.data = next_hop_group_member_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, attr->id),
        sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/**
 * @brief Get Next Hop Group attribute
 *
 * @param[in] sai_object_id_t - next_hop_group_member_id
 * @param[in] attr_count - number of attributes
 * @param[inout] attr_list - array of attributes
 *
 * @return SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_get_next_hop_group_member_attribute(
    _In_ sai_object_id_t next_hop_group_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;
  sai_attribute_t *attr = attr_list;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(next_hop_group_member_id) ==
             SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER);

  const switch_object_id_t sw_object_id = {.data = next_hop_group_member_id};
  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, sw_object_id, attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(
                            SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, attr->id),
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
 * Routine Description:
 *    Bulk Next Hop Group Members creation.
 *
 *    [in] switch_id - SAI Switch object id
 *    [in] object_count - number of objects to create
 *    [in] attr_count - list of attr_count.
 *    [in] attr_list - list of attributes for every object.
 *    [in] mode - bulk operation error handling mode.
 *    [out] object_id - list of object ids returned
 *    [out] object_statuses - list of status for every object. Caller needs
 *           to allocate the buffer.
 *
 *    #SAI_STATUS_SUCCESS on success when all objects are created or
 *    #SAI_STATUS_FAILURE when any of the objects fails to create.
 *     When there is failure, Caller is expected to go through the
 *     list of returned statuses to find out which fails and which succeeds.
 */
sai_status_t sai_create_next_hop_group_members(
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t object_count,
    _In_ const uint32_t *attr_count,
    _In_ const sai_attribute_t **attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_object_id_t *object_id,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!attr_count || !attr_list || !object_id || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, attr_count: %p, attr_list: "
        "%p, object_id: %p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        attr_count,
        attr_list,
        object_id,
        object_statuses);
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_create_next_hop_group_member(
        &object_id[it], switch_id, attr_count[it], attr_list[it]);

    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create next hop group member #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) break;
    }
  }
  bf_switch_end_batch();

  while (++it < object_count) {
    object_statuses[it] = SAI_STATUS_NOT_EXECUTED;
  }

  return status;
}

/**
 * Routine Description:
 *    Bulk Next Hop Group Members removal.
 *
 * Arguments:
 *    [in] object_count - number of objects to create
 *    [in] object_id - list of object ids
 *    [in] mode - bulk operation error handling mode.
 *    [out] object_statuses - list of status for every object.
 *
 * Return Values:
 *    #SAI_STATUS_SUCCESS on success when all objects are removed or
 *    #SAI_STATUS_FAILURE when any of the objects fails to remove. When
 *     there is failure, Caller is expected to go through the list of
 *     returned statuses to find out which fails and which succeeds.
 */
sai_status_t sai_remove_next_hop_group_members(
    _In_ uint32_t object_count,
    _In_ const sai_object_id_t *object_id,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!object_id || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, object_id: %p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        object_id,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_remove_next_hop_group_member(object_id[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove next hop group member #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) break;
    }
  }
  bf_switch_end_batch();

  while (++it < object_count) {
    object_statuses[it] = SAI_STATUS_NOT_EXECUTED;
  }

  return status;
}

/*
 *  Next Hop group methods table retrieved with sai_api_query()
 */
sai_next_hop_group_api_t nhop_group_api = {
    .create_next_hop_group = sai_create_next_hop_group_entry,
    .remove_next_hop_group = sai_remove_next_hop_group_entry,
    .set_next_hop_group_attribute = sai_set_next_hop_group_entry_attribute,
    .get_next_hop_group_attribute = sai_get_next_hop_group_entry_attribute,
    .create_next_hop_group_member = sai_create_next_hop_group_member,
    .remove_next_hop_group_member = sai_remove_next_hop_group_member,
    .set_next_hop_group_member_attribute =
        sai_set_next_hop_group_member_attribute,
    .get_next_hop_group_member_attribute =
        sai_get_next_hop_group_member_attribute,
    .create_next_hop_group_members = sai_create_next_hop_group_members,
    .remove_next_hop_group_members = sai_remove_next_hop_group_members};

sai_next_hop_group_api_t *sai_next_hop_group_api_get() {
  return &nhop_group_api;
}

sai_status_t sai_next_hop_group_initialize() {
  SAI_LOG_DEBUG("Initializing nexthop group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_NEXT_HOP_GROUP);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER);
  return SAI_STATUS_SUCCESS;
}
