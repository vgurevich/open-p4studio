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
#include <ios>
#include <string>
#include <set>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_IPMC;

/*
 * Routine Description:
 *    Converts SAI IP multicast entry to string
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *    [out entry_string - entry string
 *
 * Return Values:
 *    void
 */
static void sai_ipmc_entry_to_string(_In_ const sai_ipmc_entry_t &ipmc_entry,
                                     _Out_ std::string &entry_string) {
  char buf[SAI_MAX_ENTRY_STRING_LEN] = {"\0"};
  int len = 0;
  std::stringstream str_stream;

  str_stream << "ipmc entry: vrf 0x" << std::hex << ipmc_entry.vr_id << ", ";
  str_stream << "type "
             << ((ipmc_entry.type == SAI_IPMC_ENTRY_TYPE_SG) ? "(S,G), "
                                                             : "(*,G), ");

  sai_ipaddress_to_string(
      ipmc_entry.destination, SAI_MAX_ENTRY_STRING_LEN, buf, &len);
  str_stream << "DST IP " << buf << ", ";

  if (ipmc_entry.type == SAI_IPMC_ENTRY_TYPE_SG) {
    sai_ipaddress_to_string(
        ipmc_entry.source, SAI_MAX_ENTRY_STRING_LEN, buf, &len);
    str_stream << "SRC IP " << buf;
  }

  entry_string = str_stream.str();
}

/*
 * Routine Description:
 *    Converts SAI IP multicast entry to switch attributes list
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *    [out sw_attrs - switch attributes list
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_ipmc_entry_to_switch(
    _In_ const sai_ipmc_entry_t &ipmc_entry,
    _Out_ std::set<smi::attr_w> &sw_attrs) {
  switch_ip_prefix_t dst_ip = {}, src_ip = {};
  switch_object_id_t vrf_object = {};
  const uint8_t IPV6_PREFIX_LEN = 128;
  const uint8_t IPV4_PREFIX_LEN = 32;

  if (sai_object_type_query(ipmc_entry.vr_id) !=
      SAI_OBJECT_TYPE_VIRTUAL_ROUTER) {
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  vrf_object = {.data = ipmc_entry.vr_id};
  sw_attrs.insert(smi::attr_w(SWITCH_IPMC_ROUTE_ATTR_VRF_HANDLE, vrf_object));

  sai_ip_addr_to_switch_ip_addr(&ipmc_entry.destination, dst_ip.addr);
  dst_ip.len = (dst_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
                   ? IPV4_PREFIX_LEN
                   : IPV6_PREFIX_LEN;
  sw_attrs.insert(smi::attr_w(SWITCH_IPMC_ROUTE_ATTR_GRP_IP, dst_ip));

  if (ipmc_entry.type == SAI_IPMC_ENTRY_TYPE_SG) {
    sai_ip_addr_to_switch_ip_addr(&ipmc_entry.source, src_ip.addr);
    src_ip.len = (src_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6)
                     ? IPV6_PREFIX_LEN
                     : IPV4_PREFIX_LEN;
  } else {
    // For (*,G) zero source IP prefix has to be passed
    memset(&src_ip, 0, sizeof(switch_ip_prefix_t));
  }

  sw_attrs.insert(smi::attr_w(SWITCH_IPMC_ROUTE_ATTR_SRC_IP, src_ip));

  return SWITCH_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Gets route entry switch object for SAI IP multicast entry
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *    [out] mc_route_object - route entry switch object
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_ipmc_entry_get_switch_object(
    _In_ const sai_ipmc_entry_t &ipmc_entry,
    _Out_ switch_object_id_t &mc_route_object) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::string entry_str;
  std::set<smi::attr_w> sw_attrs;

  sai_ipmc_entry_to_string(ipmc_entry, entry_str);

  status = sai_ipmc_entry_to_switch(ipmc_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to convert IPMC entry %s to switch, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t sw_object_id = {};
  sai_insert_device_attribute(0, SWITCH_IPMC_ROUTE_ATTR_DEVICE, sw_attrs);

  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_IPMC_ROUTE, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to find switch object for IPMC entry: %s, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
    return status;
  }

  mc_route_object = sw_object_id;

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Create IP multicast entry
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_ipmc_entry(_In_ const sai_ipmc_entry_t *ipmc_entry,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::string entry_str;
  std::set<smi::attr_w> sw_attrs;

  if (!attr_list || !ipmc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_ipmc_entry_to_string(*ipmc_entry, entry_str);

  status = sai_ipmc_entry_to_switch(*ipmc_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to create IPMC entry %s, entry conversion failed, error: %s",
        entry_str.c_str(),
        sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t mc_route_object_id = {};
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_IPMC_ENTRY_ATTR_PACKET_ACTION:  // Unsupported
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_IPMC_ENTRY, &attr_list[index], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to convert attribute: %s for IPMC entry: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_IPMC_ENTRY,
                                 attr_list[index].id),
              entry_str.c_str(),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_IPMC_ROUTE_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_IPMC_ROUTE, sw_attrs, mc_route_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create IPMC entry: %s, error: %s",
                  entry_str.c_str(),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Remove IP multicast entry
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_ipmc_entry(_In_ const sai_ipmc_entry_t *ipmc_entry) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t mc_route_object_id = {};

  if (!ipmc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null IPMC entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_ipmc_entry_get_switch_object(*ipmc_entry, mc_route_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  switch_status = bf_switch_object_delete(mc_route_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    std::string entry_str;
    sai_ipmc_entry_to_string(*ipmc_entry, entry_str);
    SAI_LOG_ERROR(
        "Failed to remove IPMC entry %s, mc_route_object_id: 0x%" PRIx64
        ", error: %s",
        entry_str.c_str(),
        mc_route_object_id.data,
        sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *    Set IP multicast entry attribute value
 *
 * Arguments:
 *    [in] IP multicast - IP multicast entry
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_ipmc_entry_attribute(
    _In_ const sai_ipmc_entry_t *ipmc_entry, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_route_object_id = {};

  if (!attr || !ipmc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_ipmc_entry_get_switch_object(*ipmc_entry, mc_route_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_IPMC_ENTRY, attr, mc_route_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        std::string entry_str;
        sai_ipmc_entry_to_string(*ipmc_entry, entry_str);
        SAI_LOG_ERROR("Failed to set IPMC entry: %s attribute %s, error: %s",
                      entry_str.c_str(),
                      sai_attribute_name(SAI_OBJECT_TYPE_IPMC_ENTRY, attr->id),
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
 *    Get IP multicast entry attribute value
 *
 * Arguments:
 *    [in] ipmc_entry - IP multicast entry
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_ipmc_entry_attribute(
    _In_ const sai_ipmc_entry_t *ipmc_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t mc_route_object_id = {};

  if (!attr_list || !ipmc_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_ipmc_entry_get_switch_object(*ipmc_entry, mc_route_object_id);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_IPMC_ENTRY, mc_route_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          std::string entry_str;
          sai_ipmc_entry_to_string(*ipmc_entry, entry_str);
          SAI_LOG_ERROR("Failed to get IPMC entry: %s attribute %s, error: %s",
                        entry_str.c_str(),
                        sai_attribute_name(SAI_OBJECT_TYPE_IPMC_ENTRY,
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
 * IP multicast method table retrieved with sai_api_query()
 */
sai_ipmc_api_t ipmc_api = {
    .create_ipmc_entry = sai_create_ipmc_entry,
    .remove_ipmc_entry = sai_remove_ipmc_entry,
    .set_ipmc_entry_attribute = sai_set_ipmc_entry_attribute,
    .get_ipmc_entry_attribute = sai_get_ipmc_entry_attribute,
};

sai_ipmc_api_t *sai_ipmc_api_get() { return &ipmc_api; }

sai_status_t sai_ipmc_initialize() {
  SAI_LOG_DEBUG("Initializing ipmc");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_IPMC_ENTRY);
  return SAI_STATUS_SUCCESS;
}
