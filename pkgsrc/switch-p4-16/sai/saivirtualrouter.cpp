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
#include <cstring>

static sai_api_t api_id = SAI_API_VIRTUAL_ROUTER;
static switch_object_id_t device_handle = {0};

/*
 * Routine Description:
 *    Create virtual router
 *
 * Arguments:
 *    [out] vr_id - virtual router id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *  - SAI_STATUS_SUCCESS on success
 *  - SAI_STATUS_ADDR_NOT_FOUND if neither SAI_SWITCH_ATTR_SRC_MAC_ADDRESS nor
 *    SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS is set.
 */
sai_status_t sai_create_virtual_router_entry(
    _Out_ sai_object_id_t *vr_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bool src_mac_found = false;

  if (!vr_id || (attr_count > 0 && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vr_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t vrf_handle = {};
  std::set<smi::attr_w> sw_attrs;

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS: {
        switch_mac_addr_t mac = {};
        std::memcpy(&mac, &attr_list[i].value.mac, sizeof(sai_mac_t));
        sw_attrs.insert(smi::attr_w(SWITCH_VRF_ATTR_SRC_MAC, mac));
        src_mac_found = true;
      } break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_VIRTUAL_ROUTER, &attr_list[i], sw_attrs);
        if (status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create virtual router, attr %s: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
                                           attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  if (!src_mac_found) {
    attr_w dev_attr(SWITCH_DEVICE_ATTR_SRC_MAC);
    switch_status = bf_switch_attribute_get(
        device_handle, SWITCH_DEVICE_ATTR_SRC_MAC, dev_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to create virtual router: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    switch_mac_addr_t mac = {};
    dev_attr.v_get(mac);
    sw_attrs.insert(smi::attr_w(SWITCH_VRF_ATTR_SRC_MAC, mac));
  }

  sai_insert_device_attribute(0, SWITCH_VRF_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(SWITCH_OBJECT_TYPE_VRF, sw_attrs, vrf_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create virtual router: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vr_id = vrf_handle.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove virtual router
 *
 * Arguments:
 *    [in] vr_id - virtual router id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_virtual_router_entry(_In_ sai_object_id_t vr_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(vr_id) == SAI_OBJECT_TYPE_VIRTUAL_ROUTER);
  const switch_object_id_t sw_object_id = {.data = vr_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove virtual router entry %" PRIx64 ": %s",
                  vr_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set virtual router attribute Value
 *
 * Arguments:
 *    [in] vr_id - virtual router id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_virtual_router_entry_attribute(
    _In_ sai_object_id_t vr_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(vr_id) != SAI_OBJECT_TYPE_VIRTUAL_ROUTER) {
    SAI_LOG_ERROR("vrf get failed: invalid vrf handle 0x%" PRIx64 "\n", vr_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = vr_id};

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_VIRTUAL_ROUTER, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_VIRTUAL_ROUTER, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get virtual router attribute Value
 *
 * Arguments:
 *    [in] vr_id - virtual router id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_virtual_router_entry_attribute(
    _In_ sai_object_id_t vr_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(vr_id) != SAI_OBJECT_TYPE_VIRTUAL_ROUTER) {
    SAI_LOG_ERROR("vrf get failed: invalid vrf handle 0x%" PRIx64 "\n", vr_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = vr_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    status = SAI_STATUS_SUCCESS;

    switch (attr_list[index].id) {
      case SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_TTL1_PACKET_ACTION: {
        switch_enum_t packet_action = {};
        smi::attr_w sw_attr(SWITCH_VRF_ATTR_TTL_ACTION);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_VRF_ATTR_TTL_ACTION, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) break;
        sw_attr.v_get(packet_action);
        if (packet_action.enumdata == SWITCH_VRF_ATTR_TTL_ACTION_NONE) {
          attr_list[index].value.s32 = SAI_PACKET_ACTION_TRAP;
          break;
        }
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_VIRTUAL_ROUTER, sw_object_id, &attr_list[index]);
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_VIRTUAL_ROUTER, sw_object_id, &attr_list[index]);
        break;
    }

    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                    sai_attribute_name(SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
                                       attr_list[index].id),
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  Virtual router methods table retrieved with sai_api_query()
 */
sai_virtual_router_api_t vr_api = {
  create_virtual_router : sai_create_virtual_router_entry,
  remove_virtual_router : sai_remove_virtual_router_entry,
  set_virtual_router_attribute : sai_set_virtual_router_entry_attribute,
  get_virtual_router_attribute : sai_get_virtual_router_entry_attribute
};

sai_virtual_router_api_t *sai_virtual_router_api_get() { return &vr_api; }

sai_status_t sai_virtual_router_initialize() {
  SAI_LOG_DEBUG("Initializing virtual router");
  device_handle = sai_get_device_id(0);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_VIRTUAL_ROUTER);
  return SAI_STATUS_SUCCESS;
}
