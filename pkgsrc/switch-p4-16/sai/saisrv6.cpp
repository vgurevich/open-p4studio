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
#include "s3/switch_store.h"

#if SAI_API_VERSION >= 10901

static sai_api_t api_id = SAI_API_SRV6;

static void sai_my_sid_entry_to_string(const sai_my_sid_entry_t *my_sid_entry,
                                       char *str) {
  int count = 0;
  int len = 0;
  count = snprintf(str,
                   SAI_MAX_ENTRY_STRING_LEN,
                   "my_sid: vrf 0x%" PRIx64 ", ip: ",
                   my_sid_entry->vr_id);
  sai_ipv6_to_string(
      my_sid_entry->sid, SAI_MAX_ENTRY_STRING_LEN - count, str + count, &len);
}

static void sai_my_sid_entry_parse(const sai_my_sid_entry_t *my_sid_entry,
                                   switch_ip_address_t &switch_address) {
  sai_ipv6_to_switch_ip_addr(my_sid_entry->sid, switch_address);
}

/**
 * @brief Create MY_SID entry
 *
 * @param[in] my_sid_entry Local SID entry
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_my_sid_entry(
    _In_ const sai_my_sid_entry_t *my_sid_entry,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  switch_ip_address_t sid_address;
  std::set<attr_w> my_sid_attrs;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t my_sid_handle = {};
  char my_sid_str[SAI_MAX_ENTRY_STRING_LEN];

  if (!my_sid_entry || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_my_sid_entry_parse(my_sid_entry, sid_address);

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_MY_SID_ENTRY, &attr_list[index], my_sid_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Unsupported attribute %d: %s",
                        attr_list[index].id,
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_MY_SID_ENTRY_ATTR_DEVICE, my_sid_attrs);
  my_sid_attrs.insert(attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID, sid_address));

  switch_object_id_t vrf_handle;
  vrf_handle.data = my_sid_entry->vr_id;
  my_sid_attrs.insert(
      attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID_VRF_HANDLE, vrf_handle));

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, my_sid_attrs, my_sid_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    sai_my_sid_entry_to_string(my_sid_entry, my_sid_str);
    SAI_LOG_ERROR("Failed to create my_sid_entry %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Remove MY_SID entry
 *
 * @param[in] my_sid_entry Local SID entry
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_my_sid_entry(
    _In_ const sai_my_sid_entry_t *my_sid_entry) {
  sai_status_t status;
  switch_status_t switch_status;
  switch_ip_address_t sid_address;
  std::set<attr_w> my_sid_attrs;
  char my_sid_str[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!my_sid_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Failed to remove my_sid_entry %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_my_sid_entry_to_string(my_sid_entry, my_sid_str);
  sai_my_sid_entry_parse(my_sid_entry, sid_address);

  my_sid_attrs.insert(attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID, sid_address));
  sai_insert_device_attribute(0, SWITCH_MY_SID_ENTRY_ATTR_DEVICE, my_sid_attrs);

  switch_object_id_t vrf_handle;
  vrf_handle.data = my_sid_entry->vr_id;
  my_sid_attrs.insert(
      attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID_VRF_HANDLE, vrf_handle));

  switch_object_id_t my_sid_object = {};
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, my_sid_attrs, my_sid_object);

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove my_sid_entry: object not found %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = bf_switch_object_delete(my_sid_object);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove my_sid_entry %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Set MY_SID entry attribute value
 *
 * @param[in] my_sid_entry Local SID entry
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_my_sid_entry_attribute(
    _In_ const sai_my_sid_entry_t *my_sid_entry,
    _In_ const sai_attribute_t *attr) {
  sai_status_t status;
  switch_status_t switch_status;
  switch_ip_address_t sid_address;
  std::set<attr_w> my_sid_attrs;
  char my_sid_str[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!my_sid_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Failed to set my_sid_entry %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_my_sid_entry_to_string(my_sid_entry, my_sid_str);
  sai_my_sid_entry_parse(my_sid_entry, sid_address);

  my_sid_attrs.insert(attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID, sid_address));
  sai_insert_device_attribute(0, SWITCH_MY_SID_ENTRY_ATTR_DEVICE, my_sid_attrs);

  switch_object_id_t vrf_handle;
  vrf_handle.data = my_sid_entry->vr_id;
  my_sid_attrs.insert(
      attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID_VRF_HANDLE, vrf_handle));

  switch_object_id_t my_sid_object = {};
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, my_sid_attrs, my_sid_object);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set my_sid_entry: object not found %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_MY_SID_ENTRY, attr, my_sid_object);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set attribute %s error %s for my_sid_entry %s",
            sai_attribute_name(SAI_OBJECT_TYPE_MY_SID_ENTRY, attr->id),
            sai_metadata_get_status_name(status),
            my_sid_str);
        return status;
      }
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get MY SID attribute entry
 *
 * @param[in] my_sid_entry Local SID entry
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_my_sid_entry_attribute(
    _In_ const sai_my_sid_entry_t *my_sid_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status;
  switch_status_t switch_status;
  switch_ip_address_t sid_address;
  std::set<attr_w> my_sid_attrs;
  char my_sid_str[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!my_sid_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Failed to get my_sid_entry %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_my_sid_entry_to_string(my_sid_entry, my_sid_str);
  sai_my_sid_entry_parse(my_sid_entry, sid_address);

  my_sid_attrs.insert(attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID, sid_address));
  sai_insert_device_attribute(0, SWITCH_MY_SID_ENTRY_ATTR_DEVICE, my_sid_attrs);

  switch_object_id_t vrf_handle;
  vrf_handle.data = my_sid_entry->vr_id;
  my_sid_attrs.insert(
      attr_w(SWITCH_MY_SID_ENTRY_ATTR_SID_VRF_HANDLE, vrf_handle));

  switch_object_id_t my_sid_object = {};
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, my_sid_attrs, my_sid_object);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get my_sid_entry: object not found %s: %s",
                  my_sid_str,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_MY_SID_ENTRY, my_sid_object, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get fdb entry for my_sid %s, attribute %s: %s",
              my_sid_str,
              sai_attribute_name(SAI_OBJECT_TYPE_MY_SID_ENTRY, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  SAI_LOG_EXIT()
  return SAI_STATUS_SUCCESS;
}

void sai_switch_segmentlist_convert(const sai_attribute_t attr,
                                    std::vector<switch_ip_address_t> &sidlist) {
  for (uint32_t j = 0; j < attr.value.segmentlist.count; j++) {
    switch_ip_address_t switch_ip = {};
    char ipstring[SAI_MAX_ENTRY_STRING_LEN];
    int k = 0;
    sai_ipv6_to_string(
        attr.value.segmentlist.list[j], SAI_MAX_ENTRY_STRING_LEN, ipstring, &k);
    sai_ipv6_to_switch_ip_addr(attr.value.segmentlist.list[j], switch_ip);
    sidlist.push_back(switch_ip);
  }
}

/**
 * @brief Create SRV6 Segment ID List
 *
 * @param[out] srv6_sidlist_id Segment ID List ID
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_srv6_sidlist(_Out_ sai_object_id_t *srv6_sidlist_id,
                                     _In_ sai_object_id_t switch_id,
                                     _In_ uint32_t attr_count,
                                     _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_SEGMENTROUTE_SIDLIST;

  if (!srv6_sidlist_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_segmentlist_object_id = {};
  std::set<smi::attr_w> sw_seg_attrs;
  sai_insert_device_attribute(
      0, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_DEVICE, sw_seg_attrs);

  std::vector<switch_ip_address_t> sidlist;
  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_SRV6_SIDLIST, &attr_list[i], sw_seg_attrs);
        break;
    }
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to create segmentlist object, convert attribute failed : %s",
        sai_metadata_get_status_name(status));
    return status;
  }

  switch_status =
      bf_switch_object_create(ot, sw_seg_attrs, switch_segmentlist_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create segmentlist object : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *srv6_sidlist_id = switch_segmentlist_object_id.data;
  return status;
}

/**
 * @brief Remove Segment ID List
 *
 * @param[in] srv6_sidlist_id Segment ID List ID
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_remove_srv6_sidlist(_In_ sai_object_id_t srv6_sidlist_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(srv6_sidlist_id) != SAI_OBJECT_TYPE_SRV6_SIDLIST) {
    SAI_LOG_ERROR(
        "Segmentroute sidlist object delete failed: invalid object handle "
        "0x%" PRIx64 "\n",
        srv6_sidlist_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t segmentroute_object_id = {.data = srv6_sidlist_id};
  switch_status = bf_switch_object_delete(segmentroute_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove segmentroute object 0x%" PRIx64 "\n",
                  srv6_sidlist_id);
    return status;
  }

  return status;
}

/**
 * @brief Set Segment ID List attribute value
 *
 * @param[in] srv6_sidlist_id Segment ID List ID
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_set_srv6_sidlist_attribute(
    _In_ sai_object_id_t srv6_sidlist_id, _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(srv6_sidlist_id) != SAI_OBJECT_TYPE_SRV6_SIDLIST) {
    SAI_LOG_ERROR(
        "Segmentroute sidlist object set failed: invalid object handle "
        "0x%" PRIx64 "\n",
        srv6_sidlist_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch_object_id_t segmentroute_object_id = {.data = srv6_sidlist_id};

  std::vector<switch_ip_address_t> sidlist;
  if (attr->id == SAI_SRV6_SIDLIST_ATTR_SEGMENT_LIST) {
    sai_switch_segmentlist_convert(*attr, sidlist);
  }

  smi::attr_w segmentlist_attr(SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST);
  segmentlist_attr.v_set(sidlist);

  switch_status =
      bf_switch_attribute_set(segmentroute_object_id, segmentlist_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set sidlist attribute to object 0x%" PRIx64 ": %s",
                  segmentroute_object_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }

  return status;
}

/**
 * @brief Get Segment ID List attribute value
 *
 * @param[in] srv6_sidlist_id Segment ID List ID
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_get_srv6_sidlist_attribute(
    _In_ sai_object_id_t srv6_sidlist_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (sai_object_type_query(srv6_sidlist_id) != SAI_OBJECT_TYPE_SRV6_SIDLIST) {
    SAI_LOG_ERROR(
        "Segmentroute sidlist object get failed: invalid object handle "
        "0x%" PRIx64 "\n",
        srv6_sidlist_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t segmentroute_object_id = {.data = srv6_sidlist_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(SAI_OBJECT_TYPE_SRV6_SIDLIST,
                                             segmentroute_object_id,
                                             &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s for segmentroute object 0x%" PRIx64
              ": %s",
              sai_attribute_name(SAI_OBJECT_TYPE_SRV6_SIDLIST,
                                 attr_list[index].id),
              srv6_sidlist_id,
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_create_srv6_sidlists(_In_ sai_object_id_t switch_id,
                                      _In_ uint32_t object_count,
                                      _In_ const uint32_t *attr_count,
                                      _In_ const sai_attribute_t **attr_list,
                                      _In_ sai_bulk_op_error_mode_t mode,
                                      _Out_ sai_object_id_t *object_id,
                                      _Out_ sai_status_t *object_statuses) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_remove_srv6_sidlists(_In_ uint32_t object_count,
                                      _In_ const sai_object_id_t *object_id,
                                      _In_ sai_bulk_op_error_mode_t mode,
                                      _Out_ sai_status_t *object_statuses) {
  return SAI_STATUS_SUCCESS;
}

sai_srv6_api_t srv6_api = {
  create_srv6_sidlist : sai_create_srv6_sidlist,
  remove_srv6_sidlist : sai_remove_srv6_sidlist,
  set_srv6_sidlist_attribute : sai_set_srv6_sidlist_attribute,
  get_srv6_sidlist_attribute : sai_get_srv6_sidlist_attribute,
  create_srv6_sidlists : sai_create_srv6_sidlists,
  remove_srv6_sidlists : sai_remove_srv6_sidlists,
  create_my_sid_entry : sai_create_my_sid_entry,
  remove_my_sid_entry : sai_remove_my_sid_entry,
  set_my_sid_entry_attribute : sai_set_my_sid_entry_attribute,
  get_my_sid_entry_attribute : sai_get_my_sid_entry_attribute,
};

sai_srv6_api_t *sai_srv6_api_get() { return &srv6_api; }

#endif

sai_status_t sai_segmentroute_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_SRV6_SIDLIST);
  return status;
}
