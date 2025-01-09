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

#include <sstream>
#include <string>
#include <set>

static sai_api_t api_id = SAI_API_L2MC;

/**
 * Routine Description:
 *    Converts SAI L2 multicast entry to string
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *    [out entry_string - entry string
 *
 * Return Values:
 *    void
 */
static void sai_l2mc_entry_to_string(_In_ const sai_l2mc_entry_t &l2mc_entry,
                                     _Out_ std::string &entry_string) {
  char buf[SAI_MAX_ENTRY_STRING_LEN] = {"\0"};
  std::stringstream str_stream;
  int len = 0;

  str_stream << "l2mc_entry: vlan 0x" << std::hex << l2mc_entry.bv_id << ", ";
  str_stream << "type: "
             << ((l2mc_entry.type == SAI_L2MC_ENTRY_TYPE_SG) ? "(S, G), "
                                                             : "(*, G), ");

  sai_ipaddress_to_string(
      l2mc_entry.destination, SAI_MAX_ENTRY_STRING_LEN, buf, &len);
  str_stream << "dst IP: " << buf;

  if (l2mc_entry.type == SAI_L2MC_ENTRY_TYPE_SG) {
    sai_ipaddress_to_string(
        l2mc_entry.source, SAI_MAX_ENTRY_STRING_LEN, buf, &len);
    str_stream << ", "
               << "src IP: " << buf;
  }

  entry_string = str_stream.str();
}

/**
 * Routine Description:
 *    Converts SAI L2 multicast entry to switch attributes list
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *    [out sw_attrs - switch attributes list
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_l2mc_entry_to_switch(
    _In_ const sai_l2mc_entry_t &l2mc_entry,
    _Out_ std::set<smi::attr_w> &sw_attrs) {
  switch_ip_prefix_t dst_ip = {}, src_ip = {};
  switch_object_id_t vlan_object = {};
  const uint8_t IPV6_PREFIX_LEN = 128;
  const uint8_t IPV4_PREFIX_LEN = 32;

  if (sai_object_type_query(l2mc_entry.bv_id) != SAI_OBJECT_TYPE_VLAN) {
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  vlan_object = {.data = l2mc_entry.bv_id};
  sw_attrs.insert(
      smi::attr_w(SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_object));

  sai_ip_addr_to_switch_ip_addr(&l2mc_entry.destination, dst_ip.addr);
  dst_ip.len = (dst_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
                   ? IPV4_PREFIX_LEN
                   : IPV6_PREFIX_LEN;
  sw_attrs.insert(smi::attr_w(SWITCH_L2MC_BRIDGE_ATTR_GRP_IP, dst_ip));

  if (l2mc_entry.type == SAI_L2MC_ENTRY_TYPE_SG) {
    sai_ip_addr_to_switch_ip_addr(&l2mc_entry.source, src_ip.addr);
    dst_ip.len = (dst_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
                     ? IPV4_PREFIX_LEN
                     : IPV6_PREFIX_LEN;
  } else {
    // For (*, G) zero source IP prefix has to be passed
    memset(&src_ip, 0, sizeof(switch_ip_prefix_t));
  }
  sw_attrs.insert(smi::attr_w(SWITCH_L2MC_BRIDGE_ATTR_SRC_IP, src_ip));

  return SWITCH_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *    Gets l2mc_bridge switch object for SAI L2 multicast entry
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *    [out] bridge_object - l2mc_bridge switch object
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_l2mc_entry_get_switch_object(
    _In_ const sai_l2mc_entry_t &l2mc_entry,
    _Out_ switch_object_id_t &bridge_object) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  std::string entry_str;

  sai_l2mc_entry_to_string(l2mc_entry, entry_str);

  status = sai_l2mc_entry_to_switch(l2mc_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to convert L2MC entry %s to switch object, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_L2MC_BRIDGE_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_L2MC_BRIDGE, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to find switch object for L2C entry: %s, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
    return status;
  }

  bridge_object = sw_object_id;

  return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *    Create L2 multicast entry
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_l2mc_entry(_In_ const sai_l2mc_entry_t *l2mc_entry,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t bridge_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  std::string entry_str;

  if (!attr_list || !l2mc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_l2mc_entry_to_string(*l2mc_entry, entry_str);

  status = sai_l2mc_entry_to_switch(*l2mc_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to create L2MC entry %s, entry conversion failed, error: %s",
        entry_str.c_str(),
        sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_L2MC_ENTRY_ATTR_PACKET_ACTION:  // Unsupported
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_L2MC_ENTRY, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for L2MC entry: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_L2MC_ENTRY,
                                 attr_list[index].id),
              entry_str.c_str(),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_L2MC_BRIDGE_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_L2MC_BRIDGE, sw_attrs, bridge_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create L2MC entry: %s, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Remove L2 multicast entry
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_l2mc_entry(_In_ const sai_l2mc_entry_t *l2mc_entry) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t bridge_object_id = {};

  if (!l2mc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null L2MC entry passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_l2mc_entry_get_switch_object(*l2mc_entry, bridge_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get l2mc_bridge switch object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_object_delete(bridge_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    std::string entry_str;
    sai_l2mc_entry_to_string(*l2mc_entry, entry_str);
    SAI_LOG_ERROR("Failed to remove L2MC entry %s, bridge_object_id: 0x%" PRIx64
                  ", error: %s",
                  entry_str.c_str(),
                  bridge_object_id.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *    Set L2 multicast entry attribute value
 *
 * Arguments:
 *    [in] L2 multicast - L2 multicast entry
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_l2mc_entry_attribute(
    _In_ const sai_l2mc_entry_t *l2mc_entry, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t bridge_object_id = {};

  if (!attr || !l2mc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_l2mc_entry_get_switch_object(*l2mc_entry, bridge_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get l2mc_bridge switch object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_L2MC_ENTRY, attr, bridge_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        std::string entry_str;
        sai_l2mc_entry_to_string(*l2mc_entry, entry_str);
        SAI_LOG_ERROR("Failed to set L2MC entry: %s attribute %s, error: %s",
                      entry_str.c_str(),
                      sai_attribute_name(SAI_OBJECT_TYPE_L2MC_ENTRY, attr->id),
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
 *    Get L2 multicast entry attribute value
 *
 * Arguments:
 *    [in] l2mc_entry - L2 multicast entry
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_l2mc_entry_attribute(
    _In_ const sai_l2mc_entry_t *l2mc_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t bridge_object_id = {};

  if (!attr_list || !l2mc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_l2mc_entry_get_switch_object(*l2mc_entry, bridge_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get l2mc_bridge switch object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_L2MC_ENTRY, bridge_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          std::string entry_str;
          sai_l2mc_entry_to_string(*l2mc_entry, entry_str);
          SAI_LOG_ERROR("Failed to get IP MC entry: %s attribute %s, error: %s",
                        entry_str.c_str(),
                        sai_attribute_name(SAI_OBJECT_TYPE_L2MC_ENTRY,
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
 * L2 multicast method table retrieved with sai_api_query()
 */
sai_l2mc_api_t l2mc_api = {
    .create_l2mc_entry = sai_create_l2mc_entry,
    .remove_l2mc_entry = sai_remove_l2mc_entry,
    .set_l2mc_entry_attribute = sai_set_l2mc_entry_attribute,
    .get_l2mc_entry_attribute = sai_get_l2mc_entry_attribute,
};

sai_l2mc_api_t *sai_l2mc_api_get() { return &l2mc_api; }

sai_status_t sai_l2mc_initialize() {
  SAI_LOG_DEBUG("Initializing l2mc");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_L2MC_ENTRY);
  return SAI_STATUS_SUCCESS;
}
