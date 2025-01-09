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
#include <list>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_STP;

/*
 * Routine Description:
 *    Create stp instance with default port state as forwarding.
 *
 * Arguments:
 *    [out] stp_id - stp instance id
 *    [in] switch_id - Switch id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Value of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_create_stp_entry(_Out_ sai_object_id_t *stp_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!stp_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *stp_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t stp_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status =
            sai_to_switch_attribute(SAI_OBJECT_TYPE_STP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create stp: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_STP_ATTR_DEVICE, sw_attrs);

  switch_status =
      bf_switch_object_create(SWITCH_OBJECT_TYPE_STP, sw_attrs, stp_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create stp: %s",
                  sai_metadata_get_status_name(status));
  }
  *stp_id = stp_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove stp instance.
 *
 * Arguments:
 *    [in] stp_id - stp instance id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_remove_stp_entry(_In_ sai_object_id_t stp_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(stp_id) != SAI_OBJECT_TYPE_STP) {
    SAI_LOG_ERROR("stp remove failed: invalid stp handle %" PRIx64 "\n",
                  stp_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = stp_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove stp %" PRIx64 ": %s",
                  stp_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set stp instance attribute.
 *
 * Arguments:
 *    [in] stp_id - stp instance id
 *    [in] attr - attribute to be set
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_set_stp_entry_attribute(_In_ sai_object_id_t stp_id,
                                         _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(stp_id) == SAI_OBJECT_TYPE_STP);

  const switch_object_id_t sw_object_id = {.data = stp_id};
  switch (attr->id) {
    default:
      status =
          sai_to_switch_attribute_set(SAI_OBJECT_TYPE_STP, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_STP, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Retrieve stp entry attribute.
 *
 * Arguments:
 *    [in] stp_id - stp instance id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_get_stp_entry_attribute(_In_ sai_object_id_t stp_id,
                                         _In_ uint32_t attr_count,
                                         _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  unsigned int i = 0;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(stp_id) == SAI_OBJECT_TYPE_STP);

  const switch_object_id_t sw_object_id = {.data = stp_id};
  uint32_t index;

  for (index = 0; index < attr_count; index++) {
    sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_STP_ATTR_VLAN_LIST: {
        std::set<switch_object_id_t> list;
        switch_status = switch_store::referencing_set_get(
            sw_object_id, SWITCH_OBJECT_TYPE_VLAN, list);
        status = status_switch_to_sai(switch_status);
        std::list<uint16_t> vlan_list;
        for (const auto item : list) {
          uint16_t vlan_id = 0;
          smi::attr_w sw_attr(SWITCH_VLAN_ATTR_VLAN_ID);
          switch_status =
              bf_switch_attribute_get(item, SWITCH_VLAN_ATTR_VLAN_ID, sw_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            return status;
          }
          sw_attr.v_get(vlan_id);
          vlan_list.push_back(vlan_id);
        }
        TRY_LIST_SET(attribute->value.vlanlist, vlan_list);
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_STP, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_STP, attribute->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Create stp port object.
 *
 * Arguments:
 *    [out] stp_port_id - stp port id
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_create_stp_port(_Out_ sai_object_id_t *stp_port_id,
                                 _In_ sai_object_id_t switch_id,
                                 _In_ uint32_t attr_count,
                                 _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!stp_port_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *stp_port_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t stp_port_oid = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_STP_PORT_ATTR_BRIDGE_PORT: {
        switch_object_id_t port_lag_handle = {0};
        status = sai_get_port_from_bridge_port(attribute->value.oid,
                                               port_lag_handle);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port_handle: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_w stp_port_attr(SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE,
                             port_lag_handle);
        sw_attrs.insert(stp_port_attr);
        break;
      }
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_STP_PORT, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create stp port: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_STP_PORT_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_STP_PORT, sw_attrs, stp_port_oid);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create stp port: %s",
                  sai_metadata_get_status_name(status));
  }
  *stp_port_id = stp_port_oid.data;

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Remove stp port object.
 *
 * Arguments:
 *    [in] stp_port_id - stp port object id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_remove_stp_port(_In_ sai_object_id_t stp_port_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(stp_port_id) != SAI_OBJECT_TYPE_STP_PORT) {
    SAI_LOG_ERROR("stp remove failed: invalid stp port handle %" PRIx64 "\n",
                  stp_port_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = stp_port_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove stp port %" PRIx64 ": %s",
                  stp_port_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Set the attribute of STP port.
 *
 * Arguments:
 *    [in] stp_port_id - stp port id
 *    [in] attr - attribute to be set
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_set_stp_port_attribute(_In_ sai_object_id_t stp_port_id,
                                        _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(stp_port_id) == SAI_OBJECT_TYPE_STP_PORT);

  const switch_object_id_t sw_object_id = {.data = stp_port_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_STP_PORT, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_STP_PORT, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get the attribute of STP port.
 *
 * Arguments:
 *    [in] stp_port_id - stp port id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - list of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *    error code is returned.
 */
sai_status_t sai_get_stp_port_attribute(_In_ sai_object_id_t stp_port_id,
                                        _In_ uint32_t attr_count,
                                        _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(stp_port_id) == SAI_OBJECT_TYPE_STP_PORT);

  const switch_object_id_t sw_object_id = {.data = stp_port_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_STP_PORT_ATTR_BRIDGE_PORT: {
        switch_object_id_t port_lag_handle = {};
        smi::attr_w sw_attr(SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_STP_PORT_ATTR_PORT_LAG_HANDLE, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_STP_PORT, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(port_lag_handle);
        status = sai_get_port_to_bridge_port(port_lag_handle,
                                             attr_list[i].value.oid);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get bridge port by stp port object_id: 0x%" PRIx64
              "attribute %s "
              "error: %s",
              stp_port_id,
              sai_attribute_name(SAI_OBJECT_TYPE_STP_PORT, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_STP_PORT, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get stp port attribute object_id: 0x%" PRIx64
              "attribute "
              "%s "
              "error: %s",
              stp_port_id,
              sai_attribute_name(SAI_OBJECT_TYPE_STP_PORT, attr_list[i].id),
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
 * @brief STP method table retrieved with sai_api_query()
 */
sai_stp_api_t stp_api = {.create_stp = sai_create_stp_entry,
                         .remove_stp = sai_remove_stp_entry,
                         .set_stp_attribute = sai_set_stp_entry_attribute,
                         .get_stp_attribute = sai_get_stp_entry_attribute,
                         .create_stp_port = sai_create_stp_port,
                         .remove_stp_port = sai_remove_stp_port,
                         .set_stp_port_attribute = sai_set_stp_port_attribute,
                         .get_stp_port_attribute = sai_get_stp_port_attribute};

sai_stp_api_t *sai_stp_api_get() { return &stp_api; }

sai_status_t sai_stp_initialize() {
  SAI_LOG_DEBUG("Initializing stp");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_STP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_STP_PORT);

  return SAI_STATUS_SUCCESS;
}
