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

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_L2MC_GROUP;

/**
 * Routine Description:
 *    Create L2 multicast group
 *
 * Arguments:
 *    [out] l2mc_group_id - L2 multicast group ID
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_l2mc_group(_Out_ sai_object_id_t *l2mc_group_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t l2mc_group_oid = {};
  std::set<smi::attr_w> sw_attrs;

  if ((attr_count && !attr_list) || !l2mc_group_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *l2mc_group_id = SAI_NULL_OBJECT_ID;

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_L2MC_GROUP, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for L2MC group, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_L2MC_GROUP_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_L2MC_GROUP, sw_attrs, l2mc_group_oid);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create L2MC group, error: %s",
                  sai_metadata_get_status_name(status));
  }

  *l2mc_group_id = l2mc_group_oid.data;

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Remove L2 multicast group
 * Arguments:
 *    [in] l2mc_group_id - L2 multicast group ID
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_l2mc_group(_In_ sai_object_id_t l2mc_group_id) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_oid = {.data = l2mc_group_id};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_id) != SAI_OBJECT_TYPE_L2MC_GROUP) {
    SAI_LOG_ERROR(
        "L2MC group remove failed: invalid L2MC group handle 0x%" PRIx64,
        l2mc_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_status = bf_switch_object_delete(l2mc_group_oid);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove L2MC group: 0x%" PRIx64 ", error: %s",
                  l2mc_group_oid.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Set L2 multicast group attribute value
 *
 * Arguments:
 *    [in] l2mc_group_id - L2 multicast group ID
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_l2mc_group_attribute(_In_ sai_object_id_t l2mc_group_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_oid = {.data = l2mc_group_id};
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_id) != SAI_OBJECT_TYPE_L2MC_GROUP) {
    SAI_LOG_ERROR(
        "L2MC group attribute set failed: invalid L2MC group handle "
        "0x%" PRIx64,
        l2mc_group_id);
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
          SAI_OBJECT_TYPE_L2MC_GROUP, attr, l2mc_group_oid);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set L2MC group 0x%" PRIx64
                      " attribute %s, error: %s",
                      l2mc_group_id,
                      sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Get L2 multicast group attribute value
 *
 * Arguments:
 *    [in] l2mc_group_id - L2 multicast group ID
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_l2mc_group_attribute(_In_ sai_object_id_t l2mc_group_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_oid = {.data = l2mc_group_id};
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_id) != SAI_OBJECT_TYPE_L2MC_GROUP) {
    SAI_LOG_ERROR(
        "L2MC group attribute get failed: invalid L2MC group handle "
        "0x%" PRIx64,
        l2mc_group_id);
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
            SAI_OBJECT_TYPE_L2MC_GROUP, l2mc_group_oid, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get L2MC group: 0x%" PRIx64
                        " attribute %s, error: %s",
                        l2mc_group_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP,
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

/**
 * Routine Description:
 *    Create L2 multicast group member
 *
 * Arguments:
 *    [out] l2mc_group_member_id - L2 multicast group member ID
 *    [in] switch_id - switch id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_l2mc_group_member(
    _Out_ sai_object_id_t *l2mc_group_member_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t l2mc_group_member_oid = {};
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_handle = {};
  std::set<smi::attr_w> sw_attrs;
  switch_enum_t iface_type;

  if (!attr_list || !l2mc_group_member_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *l2mc_group_member_id = SAI_NULL_OBJECT_ID;
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_L2MC_GROUP_MEMBER_ATTR_L2MC_OUTPUT_ID: {
        if (sai_object_type_query(attr_list[index].value.oid) !=
            SAI_OBJECT_TYPE_BRIDGE_PORT) {
          SAI_LOG_ERROR(
              "L2MC group member create failed: invalid L2MC output type, must "
              "be SAI_OBJECT_TYPE_BRIDGE_PORT");
          return SAI_STATUS_INVALID_PARAMETER;
        }

        status = sai_get_bridge_port_interface_type(attr_list[index].value.oid,
                                                    iface_type);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get interface type for bridge port: 0x%" PRIx64 ", %s",
              attr_list[index].value.oid,
              sai_metadata_get_status_name(status));
          return status;
        }
        if (iface_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
          status = sai_get_port_from_bridge_port(attr_list[index].value.oid,
                                                 sw_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get port handle for bridge port: 0x%" PRIx64 ", %s",
                attr_list[index].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (iface_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
          status = sai_get_tunnel_from_bridge_port(attr_list[index].value.oid,
                                                   sw_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get tunnel handle for bridge port: 0x%" PRIx64
                ", %s",
                attr_list[index].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
          SAI_LOG_ERROR("Fail, bridge_port type not supported: 0x%" PRIx64
                        ", %s",
                        attr_list[index].value.oid,
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_w output_id_attr(SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, sw_handle);
        sw_attrs.insert(output_id_attr);
      } break;
      default: {
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for L2MC group member, error: "
              "%s",
              sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
      }
    }
  }

  sai_insert_device_attribute(0, SWITCH_L2MC_MEMBER_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_L2MC_MEMBER, sw_attrs, l2mc_group_member_oid);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create L2MC group member, error: %s",
                  sai_metadata_get_status_name(status));
  }

  *l2mc_group_member_id = l2mc_group_member_oid.data;

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Remove L2 multicast group member
 * Arguments:
 *    [in] l2mc_group_member_id - L2 multicast group member ID
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_l2mc_group_member(
    _In_ sai_object_id_t l2mc_group_member_id) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_member_oid = {.data = l2mc_group_member_id};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_member_id) !=
      SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "L2MC group member remove failed: invalid L2MC group member handle "
        "0x%" PRIx64,
        l2mc_group_member_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_status = bf_switch_object_delete(l2mc_group_member_oid);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove L2MC group member: 0x%" PRIx64
                  ", error: %s",
                  l2mc_group_member_oid.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Set L2 multicast group member attribute value
 *
 * Arguments:
 *    [in] l2mc_group_member_id - L2 multicast group member ID
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_l2mc_group_member_attribute(
    _In_ sai_object_id_t l2mc_group_member_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_member_oid = {.data = l2mc_group_member_id};
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_member_id) !=
      SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "L2MC group member attribute set failed: invalid L2MC group member "
        "handle 0x%" PRIx64,
        l2mc_group_member_id);
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
          SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER, attr, l2mc_group_member_oid);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set L2MC group member 0x%" PRIx64
            " attribute %s, error: %s",
            l2mc_group_member_id,
            sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER, attr->id),
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
 *    Get L2 multicast group member attribute value
 *
 * Arguments:
 *    [in] l2mc_group_member_id - L2 multicast group member ID
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_l2mc_group_member_attribute(
    _In_ sai_object_id_t l2mc_group_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_object_id_t l2mc_group_member_oid = {.data = l2mc_group_member_id};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(l2mc_group_member_id) !=
      SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER) {
    SAI_LOG_ERROR(
        "L2MC group member attribute get failed: invalid L2MC group member "
        "handle 0x%" PRIx64,
        l2mc_group_member_id);
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
      case SAI_L2MC_GROUP_MEMBER_ATTR_L2MC_OUTPUT_ID: {
        switch_object_id_t output_id = {};
        smi::attr_w sw_attr(SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE);

        switch_status =
            bf_switch_attribute_get(l2mc_group_member_oid,
                                    SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE,
                                    sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                 attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(output_id);

        switch_object_type_t ot = switch_store::object_type_query(output_id);
        if (ot == SWITCH_OBJECT_TYPE_PORT || ot == SWITCH_OBJECT_TYPE_LAG) {
          status = sai_get_port_to_bridge_port(output_id,
                                               attr_list[index].value.oid);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get attribute %s from l2mc_group_member object_id: "
                "0x%" PRIx64 "mapping from port, error: %s",
                sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                   attr_list[index].id),
                l2mc_group_member_id,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
          status = sai_get_tunnel_to_bridge_port(output_id,
                                                 attr_list[index].value.oid);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get attribute %s from l2mc_group_member object_id: "
                "0x%" PRIx64 "mapping from tunnel, error: %s",
                sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                   attr_list[index].id),
                l2mc_group_member_id,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
          SAI_LOG_ERROR(
              "Failed to get attribute %s from l2mc_group_member object_id: "
              "0x%" PRIx64 "due to unsupported member type, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                 attr_list[index].id),
              l2mc_group_member_id,
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
      default:
        status = sai_to_switch_attribute_get(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
                                             l2mc_group_member_oid,
                                             &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get L2MC group member: 0x%" PRIx64
                        " attribute %s, error: %s",
                        l2mc_group_member_id,
                        sai_attribute_name(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER,
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

/**
 * L2 multicast group method table retrieved with sai_api_query()
 */
sai_l2mc_group_api_t l2mc_group_api = {
    .create_l2mc_group = sai_create_l2mc_group,
    .remove_l2mc_group = sai_remove_l2mc_group,
    .set_l2mc_group_attribute = sai_set_l2mc_group_attribute,
    .get_l2mc_group_attribute = sai_get_l2mc_group_attribute,
    .create_l2mc_group_member = sai_create_l2mc_group_member,
    .remove_l2mc_group_member = sai_remove_l2mc_group_member,
    .set_l2mc_group_member_attribute = sai_set_l2mc_group_member_attribute,
    .get_l2mc_group_member_attribute = sai_get_l2mc_group_member_attribute};

sai_l2mc_group_api_t *sai_l2mc_group_api_get() { return &l2mc_group_api; }

sai_status_t sai_l2mc_group_initialize() {
  SAI_LOG_DEBUG("Initializing l2mc group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_L2MC_GROUP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_L2MC_GROUP_MEMBER);
  return SAI_STATUS_SUCCESS;
}
