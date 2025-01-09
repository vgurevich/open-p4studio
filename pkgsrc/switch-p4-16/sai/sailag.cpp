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

static sai_api_t api_id = SAI_API_LAG;

sai_status_t sai_create_lag_entry(_Out_ sai_object_id_t *lag_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list);

sai_status_t sai_remove_lag_entry(_In_ sai_object_id_t lag_id);

sai_status_t sai_set_lag_entry_attribute(_In_ sai_object_id_t lag_id,
                                         _In_ const sai_attribute_t *attr);

sai_status_t sai_add_ports_to_lag(_In_ sai_object_id_t lag_id,
                                  _In_ const sai_object_list_t *port_list);

/*
    \brief Create LAG
    \param[out] lag_id LAG id
    \param[in] attr_count number of attributes
    \param[in] attr_list array of attributes
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_create_lag_entry(_Out_ sai_object_id_t *lag_id,
                                  _In_ sai_object_id_t switch_id,

                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_LAG;

  if (!lag_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *lag_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_lag_object_id = {};
  std::set<smi::attr_w> sw_lag_attrs;

  sai_insert_device_attribute(0, SWITCH_LAG_ATTR_DEVICE, sw_lag_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_lag_attrs, switch_lag_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create lag: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *lag_id = switch_lag_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Remove LAG
    \param[in] lag_id LAG id
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_remove_lag_entry(_In_ sai_object_id_t lag_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(lag_id) == SAI_OBJECT_TYPE_LAG);

  const switch_object_id_t sw_lag_object_id = {.data = lag_id};
  switch_status = bf_switch_object_delete(sw_lag_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove lag %" PRIx64 ": %s",
                  lag_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Set LAG Attribute
    \param[in] lag_id LAG id
    \param[in] attr Structure containing ID and value to be set
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_set_lag_attribute(_In_ sai_object_id_t lag_id,
                                   _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(lag_id) == SAI_OBJECT_TYPE_LAG);
  switch (attr->id) {
    default: {
      const switch_object_id_t sw_lag_object_id = {.data = lag_id};
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_LAG, attr, sw_lag_object_id);
      if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_LAG, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Get LAG Attribute
    \param[in] lag_id LAG id
    \param[in] attr_count Number of attributes to be get
    \param[in,out] attr_list List of structures containing ID and value to be
   get
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_get_lag_attribute(_In_ sai_object_id_t lag_id,
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

  SAI_ASSERT(sai_object_type_query(lag_id) == SAI_OBJECT_TYPE_LAG);

  const switch_object_id_t sw_lag_object_id = {.data = lag_id};
  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      case SAI_LAG_ATTR_INGRESS_ACL:
      case SAI_LAG_ATTR_PORT_LIST:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_LAG, sw_lag_object_id, attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s for lag_id: %" PRIx64
                        "error: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_LAG, attr->id),
                        (lag_id & 0xFFFF),
                        sai_metadata_get_status_name(status));
        }
        break;
      case SAI_LAG_ATTR_EGRESS_ACL:     // Unsupported
      case SAI_LAG_ATTR_DROP_UNTAGGED:  // Unsupported
      case SAI_LAG_ATTR_DROP_TAGGED:    // Unsupported
        break;
      default:
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Create LAG Member
    \param[out] lag_member_id LAG Member id
    \param[in] attr_count number of attributes
    \param[in] attr_list array of attributes
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_create_lag_member(_Out_ sai_object_id_t *lag_member_id,
                                   _In_ sai_object_id_t switch_id,

                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_LAG_MEMBER;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!lag_member_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *lag_member_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t sw_lag_member_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  sai_insert_device_attribute(0, SWITCH_LAG_MEMBER_ATTR_DEVICE, sw_attrs);
  status = sai_to_switch_attribute_list(
      SAI_OBJECT_TYPE_LAG_MEMBER, attr_count, attr_list, sw_attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  switch_status =
      bf_switch_object_create(ot, sw_attrs, sw_lag_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create lag_member: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *lag_member_id = sw_lag_member_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Remove LAG Member
    \param[in] lag_member_id LAG Member id
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_remove_lag_member(_In_ sai_object_id_t lag_member_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(lag_member_id) ==
             SAI_OBJECT_TYPE_LAG_MEMBER);
  const switch_object_id_t sw_lag_member_object_id = {.data = lag_member_id};
  switch_status = bf_switch_object_delete(sw_lag_member_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove lag_member %" PRIx64 ": %s",
                  lag_member_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Set LAG Member Attribute
    \param[in] lag_member_id LAG Member id
    \param[in] attr Structure containing ID and value to be set
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/
sai_status_t sai_set_lag_member_attribute(_In_ sai_object_id_t lag_member_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(lag_member_id) ==
             SAI_OBJECT_TYPE_LAG_MEMBER);

  const switch_object_id_t sw_lag_mem_object_id = {.data = lag_member_id};
  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_LAG_MEMBER, attr, sw_lag_mem_object_id);
  if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_LAG_MEMBER, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Get LAG Member Attribute
    \param[in] lag_member_id LAG Member id
    \param[in] attr_count Number of attributes to be get
    \param[in,out] attr_list List of structures containing ID and value to be
   get
    \return Success: SAI_STATUS_SUCCESS
            Failure: Failure status code on error
*/

sai_status_t sai_get_lag_member_attribute(_In_ sai_object_id_t lag_member_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_attribute_t *attr = attr_list;

  unsigned int i = 0;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(lag_member_id) ==
             SAI_OBJECT_TYPE_LAG_MEMBER);
  const switch_object_id_t sw_lag_member_object_id = {.data = lag_member_id};

  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      case SAI_LAG_MEMBER_ATTR_LAG_ID:
      case SAI_LAG_MEMBER_ATTR_PORT_ID:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_LAG_MEMBER, sw_lag_member_object_id, attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s for lag_member_id: %" PRIx64
                        "error: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_LAG, attr->id),
                        (lag_member_id & 0xFFFF),
                        sai_metadata_get_status_name(status));
        }
        break;
      default:
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  LAG methods table retrieved with sai_api_query()
 */
sai_lag_api_t lag_api = {
    .create_lag = sai_create_lag_entry,
    .remove_lag = sai_remove_lag_entry,
    .set_lag_attribute = sai_set_lag_attribute,
    .get_lag_attribute = sai_get_lag_attribute,
    .create_lag_member = sai_create_lag_member,
    .remove_lag_member = sai_remove_lag_member,
    .set_lag_member_attribute = sai_set_lag_member_attribute,
    .get_lag_member_attribute = sai_get_lag_member_attribute,
};

sai_lag_api_t *sai_lag_api_get() { return &lag_api; }

sai_status_t sai_lag_initialize() {
  SAI_LOG_DEBUG("Initializing lag");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_LAG);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_LAG_MEMBER);
  return SAI_STATUS_SUCCESS;
}
