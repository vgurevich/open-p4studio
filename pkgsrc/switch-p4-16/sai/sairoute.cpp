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

static sai_api_t api_id = SAI_API_ROUTE;
static switch_object_id_t device_handle = {0};
switch_object_id_t hostif_nhop_handle = {0};

static char *sai_route_entry_to_string(
    _In_ const sai_route_entry_t *route_entry,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list,
    _Out_ char *entry_string) {
  int count = 0;
  int len = 0;

  if (!route_entry || !entry_string) {
    return entry_string;
  }

  count = snprintf(entry_string,
                   SAI_MAX_ENTRY_STRING_LEN,
                   "route: vrf_oid 0x%" PRIx64 ", ip ",
                   route_entry->vr_id);
  sai_ipprefix_to_string(route_entry->destination,
                         SAI_MAX_ENTRY_STRING_LEN - count,
                         entry_string + count,
                         &len);
  count += len;

  if (!attr_list || !attr_count) {
    return entry_string;
  }

  for (uint32_t index = 0; index < attr_count; index++) {
    const sai_attribute_t *attr = &attr_list[index];
    switch (attr->id) {
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        len = snprintf(entry_string + count,
                       SAI_MAX_ENTRY_STRING_LEN - count,
                       " nh_oid: 0x%" PRIx64 ", nh_oid_type %s",
                       attr->value.oid,
                       sai_object_name_query(attr->value.oid).c_str());
        count += len;
        break;
      case SAI_ROUTE_ENTRY_ATTR_USER_TRAP_ID:  // Unsupported
        break;
      case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
        len = snprintf(entry_string + count,
                       SAI_MAX_ENTRY_STRING_LEN - count,
                       " action: %d",
                       attr->value.s32);
        count += len;
        break;
      default:
        break;
    }
  }
  return entry_string;
}

/*
 * Helper function to set all routes using this nhop to glean since nhop is
 * being removed
 */
sai_status_t sai_route_set_glean_internal(
    const switch_object_id_t nhop_handle) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  std::set<switch_object_id_t> ref_oids;

  switch_status |= switch_store::referencing_set_get(
      nhop_handle, SWITCH_OBJECT_TYPE_ROUTE, ref_oids);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_ERROR("Failed to get route handles from nhop handle: %" PRIx64
                  ": %s",
                  nhop_handle.data,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (const auto route_handle : ref_oids) {
    attr_w route_attr(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, hostif_nhop_handle);
    switch_status = bf_switch_attribute_set(route_handle, route_attr);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      status = status_switch_to_sai(switch_status);
      SAI_LOG_ERROR("Failed to set route handle nhop to glean: %" PRIx64 ": %s",
                    route_handle.data,
                    sai_metadata_get_status_name(status));
    }
  }
  return status;
}

static void sai_route_entry_parse(_In_ const sai_route_entry_t *route_entry,
                                  _Out_ switch_object_id_t &vrf_handle,
                                  _Out_ switch_ip_prefix_t &ip_prefix) {
  const sai_ip_prefix_t *sai_ip_prefix;

  if (sai_object_type_query(route_entry->vr_id) !=
      SAI_OBJECT_TYPE_VIRTUAL_ROUTER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, route_entry->vr_id);
    vrf_handle.data = SAI_NULL_OBJECT_ID;
    return;
  }

  vrf_handle.data = route_entry->vr_id;
  sai_ip_prefix = &route_entry->destination;
  sai_ip_prefix_to_switch_ip_prefix(sai_ip_prefix, ip_prefix);
}

static inline sai_status_t sai_to_switch_packet_action(
    const sai_packet_action_t sai_action, switch_enum_t &switch_action) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (sai_action) {
    case SAI_PACKET_ACTION_DROP:
      switch_action.enumdata = SWITCH_ROUTE_ATTR_PACKET_ACTION_DROP;
      break;
    case SAI_PACKET_ACTION_FORWARD:
      switch_action.enumdata = SWITCH_ROUTE_ATTR_PACKET_ACTION_FORWARD;
      break;
    case SAI_PACKET_ACTION_TRAP:
      switch_action.enumdata = SWITCH_ROUTE_ATTR_PACKET_ACTION_TRAP;
      break;
    case SAI_PACKET_ACTION_DENY:
      switch_action.enumdata = SWITCH_ROUTE_ATTR_PACKET_ACTION_DENY;
      break;
    case SAI_PACKET_ACTION_TRANSIT:
      switch_action.enumdata = SWITCH_ROUTE_ATTR_PACKET_ACTION_TRANSIT;
      break;
    case SAI_PACKET_ACTION_COPY_CANCEL:
    case SAI_PACKET_ACTION_COPY:
    case SAI_PACKET_ACTION_LOG:
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
    default:
      status = SAI_STATUS_INVALID_PARAMETER;
  }

  return status;
}

static sai_status_t sai_route_entry_attribute_parse(
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list,
    _Out_ switch_object_id_t &nhop_handle,
    _Out_ switch_enum_t &action,
    _Out_ uint32_t *meta_data) {
  const sai_attribute_t *attribute;
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t index = 0;
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        nhop_handle.data = attribute->value.oid;
        break;
      case SAI_ROUTE_ENTRY_ATTR_USER_TRAP_ID:  // Unsupported
        break;
      case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
        status = sai_to_switch_packet_action(
            (sai_packet_action_t)attribute->value.s32, action);
        break;
      case SAI_ROUTE_ENTRY_ATTR_META_DATA:
        *meta_data = attribute->value.u32;
        break;
    }
  }

  return status;
}

bool sai_route_entry_host_route(switch_object_id_t nexthop_handle) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t cpu_port_handle = {0};
  attr_w attr(0);

  switch_status = bf_switch_attribute_get(
      sai_get_device_id(0), SWITCH_DEVICE_ATTR_CPU_PORT, attr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_DEBUG("Failed to get cpu port handle: %s",
                  sai_metadata_get_status_name(status));
    return false;
  }
  attr.v_get(cpu_port_handle);

  if (nexthop_handle.data == cpu_port_handle.data) {
    return true;
  }

  return false;
}

static sai_status_t sai_route_entry_find_route_handle(
    const switch_object_id_t vrf_handle,
    const switch_ip_prefix_t ip_prefix,
    switch_object_id_t &route_handle) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  std::set<attr_w> route_attrs;
  sai_insert_device_attribute(0, SWITCH_ROUTE_ATTR_DEVICE, route_attrs);
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE, vrf_handle));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix));

  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, false));
  switch_status =
      bf_switch_object_get(SWITCH_OBJECT_TYPE_ROUTE, route_attrs, route_handle);

  if (switch_status != SWITCH_STATUS_SUCCESS &&
      !(bf_switch_is_feature_enabled(SWITCH_FEATURE_IPV4_LOCAL_HOST))) {
    // when SWITCH_FEATURE_IPV4_LOCAL_HOST is disable
    // same route could be created in sai_create_neighbor_entry and here
    // to handle this we use is_nbr_sourced and is_route_sourced flags
    // So we should search for nbr sourced before create new obj
    route_attrs.erase(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED));
    route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, true));
    switch_status = bf_switch_object_get(
        SWITCH_OBJECT_TYPE_ROUTE, route_attrs, route_handle);
  }

  if (switch_status != SWITCH_STATUS_SUCCESS) {
    route_handle.data = SAI_NULL_OBJECT_ID;
  }

  return status_switch_to_sai(switch_status);
}

static bool sai_route_entry_is_route_sourced(
    const switch_object_id_t route_handle) {
  bool is_route_sourced = false;

  attr_w is_route_sourced_attr(SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED);
  bf_switch_attribute_get(
      route_handle, SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED, is_route_sourced_attr);
  is_route_sourced_attr.v_get(is_route_sourced);

  return is_route_sourced;
}

static bool sai_route_entry_is_nbr_sourced(
    const switch_object_id_t route_handle) {
  bool is_nbr_sourced = false;

  attr_w is_nbr_sourced_attr(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED);
  bf_switch_attribute_get(
      route_handle, SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, is_nbr_sourced_attr);
  is_nbr_sourced_attr.v_get(is_nbr_sourced);

  return is_nbr_sourced;
}

/*
 * Routine Description:
 *    Create Route
 *
 * Arguments:
 *    [in] route_entry - route entry
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 *
 * Note: IP prefix/mask expected in Network Byte Order.
 *
 */
sai_status_t sai_create_route_entry(_In_ const sai_route_entry_t *route_entry,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  switch_ip_prefix_t ip_prefix = {};
  switch_object_id_t nhop_handle = {0};
  switch_object_id_t vrf_handle = {0};
  switch_object_id_t route_handle = {0};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_enum_t action = {SWITCH_ROUTE_ATTR_PACKET_ACTION_FORWARD};
  std::set<attr_w> route_attrs;
  sai_status_t status = SAI_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];
  uint8_t myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE;
  uint32_t meta_data = 0;

  if (!route_entry || (!attr_list && attr_count)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_parse(route_entry, vrf_handle, ip_prefix);
  if (vrf_handle.data == SAI_NULL_OBJECT_ID) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Invalid VRF handle: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_route_entry_attribute_parse(
      attr_count, attr_list, nhop_handle, action, &meta_data);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Route entry create failed for route entry %s: %s",
                  sai_route_entry_to_string(
                      route_entry, attr_count, attr_list, entry_string),
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_find_route_handle(vrf_handle, ip_prefix, route_handle);

  // if this is a host route, set glean and set myip
  if (sai_route_entry_host_route(nhop_handle)) {
    myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
#ifdef SAI_TRAP_TYPE_IP2ME_SUBNET  // customer patch
    if (!switch_ip_prefix_is_host_ip(ip_prefix)) {
      myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET;
    }
#endif
    nhop_handle.data = hostif_nhop_handle.data;
  }

  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nhop_handle));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED, true));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_PACKET_ACTION, action));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_HOST_MYIP, myip_type));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_FIB_LABEL, meta_data));

  // if route exists, update nexthop and return
  if (route_handle.data != SAI_NULL_OBJECT_ID) {
    for (attr_w route_attr : route_attrs) {
      switch_status |= bf_switch_attribute_set(route_handle, route_attr);
    }
  } else {
    sai_insert_device_attribute(0, SWITCH_ROUTE_ATTR_DEVICE, route_attrs);
    route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE, vrf_handle));
    route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix));
    route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, false));
    // why not use bf_switch_object_create?
    // we have all attributes correctly setup, no need to waste cycles checking
    // for flags and mandatory attributes
    switch_status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_ROUTE, route_attrs, route_handle);
  }

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Route entry create failed for route entry %s: %s",
                  sai_route_entry_to_string(
                      route_entry, attr_count, attr_list, entry_string),
                  sai_metadata_get_status_name(status));
  }

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove Route
 *
 * Arguments:
 *    [in] route_entry - route entry
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 *
 * Note: IP prefix/mask expected in Network Byte Order.
 */
sai_status_t sai_remove_route_entry(_In_ const sai_route_entry_t *route_entry) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_ip_prefix_t ip_prefix = {};
  switch_object_id_t vrf_handle = {0};
  switch_object_id_t route_handle = {0};
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  if (!route_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null unicast entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_parse(route_entry, vrf_handle, ip_prefix);
  if (vrf_handle.data == SAI_NULL_OBJECT_ID) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Invalid VRF handle: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_find_route_handle(vrf_handle, ip_prefix, route_handle);

  if (route_handle.data == SAI_NULL_OBJECT_ID ||
      !sai_route_entry_is_route_sourced(route_handle)) {
    status = SAI_STATUS_ITEM_NOT_FOUND;
    SAI_LOG_ERROR("Failed to find route entry %s : %s",
                  sai_route_entry_to_string(route_entry, 0, NULL, entry_string),
                  sai_metadata_get_status_name(status));
    return status;
  }

  // obj is_nbr_sourced so set is_route_sourced to false
  // and do not delete the obj.
  if (sai_route_entry_is_nbr_sourced(route_handle)) {
    switch_object_id_t nbr_handle = {0};
    attr_w route_attr(SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE);
    switch_status = bf_switch_attribute_get(
        route_handle, SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE, route_attr);
    route_attr.v_get(nbr_handle);

    switch_object_id_t nexthop_handle = {};
    attr_w nbr_attr(SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE);
    switch_status |= bf_switch_attribute_get(
        nbr_handle, SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE, nbr_attr);
    nbr_attr.v_get(nexthop_handle);

    switch_status |= bf_switch_attribute_set(
        route_handle, attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nexthop_handle));
    switch_status |= bf_switch_attribute_set(
        route_handle, attr_w(SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED, false));
    switch_status |= switch_store::attribute_set(
        route_handle,
        attr_w(SWITCH_ROUTE_ATTR_RIF_HANDLE,
               switch_object_id_t{SWITCH_NULL_OBJECT_ID}));
  } else {
    switch_status = bf_switch_object_delete(route_handle);
  }

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove route entry: %s",
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set route attribute value
 *
 * Arguments:
 *    [in] route_entry - route entry
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_route_entry_attribute(
    _In_ const sai_route_entry_t *route_entry,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];
  switch_enum_t packet_action;
  switch_ip_prefix_t ip_prefix = {};
  switch_object_id_t vrf_handle = {0};
  switch_object_id_t route_handle = {0};
  switch_object_id_t nhop_handle = {0};
  uint8_t myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE;

  if (!route_entry || !attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_parse(route_entry, vrf_handle, ip_prefix);
  if (vrf_handle.data == SAI_NULL_OBJECT_ID) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Invalid VRF handle: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_find_route_handle(vrf_handle, ip_prefix, route_handle);

  if (route_handle.data == SAI_NULL_OBJECT_ID) {
    status = SAI_STATUS_ITEM_NOT_FOUND;
    SAI_LOG_ERROR("Failed to find route entry %s : %s",
                  sai_route_entry_to_string(route_entry, 0, NULL, entry_string),
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
      nhop_handle.data = attr->value.oid;

      // if this is a host route, set glean and set myip
      if (sai_route_entry_host_route(nhop_handle)) {
        myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
#ifdef SAI_TRAP_TYPE_IP2ME_SUBNET  // customer patch
        if (!switch_ip_prefix_is_host_ip(ip_prefix)) {
          myip_type = SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET;
        }
#endif
        nhop_handle.data = hostif_nhop_handle.data;
      }

      switch_status |= bf_switch_attribute_set(
          route_handle, attr_w(SWITCH_ROUTE_ATTR_IS_HOST_MYIP, myip_type));
      switch_status |= bf_switch_attribute_set(
          route_handle, attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nhop_handle));
      status = status_switch_to_sai(switch_status);
      break;
    case SAI_ROUTE_ENTRY_ATTR_USER_TRAP_ID:  // Unsupported
      break;
    case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
      status = sai_to_switch_packet_action((sai_packet_action_t)attr->value.s32,
                                           packet_action);
      if (status == SAI_STATUS_SUCCESS) {
        switch_status = bf_switch_attribute_set(
            route_handle,
            attr_w(SWITCH_ROUTE_ATTR_PACKET_ACTION, packet_action));
        status = status_switch_to_sai(switch_status);
      }
      break;
    default:
      status = SAI_STATUS_INVALID_PARAMETER;
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute for route %s : %s",
                  sai_route_entry_to_string(route_entry, 1, attr, entry_string),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get route attribute value
 *
 * Arguments:
 *    [in] route_entry - route entry
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_route_entry_attribute(
    _In_ const sai_route_entry_t *route_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  unsigned int i = 0;
  switch_ip_prefix_t ip_prefix;
  switch_object_id_t nhop_handle = {0};
  switch_object_id_t vrf_handle = {0};
  switch_object_id_t route_handle = {0};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];
  sai_attribute_t *attr = attr_list;
  switch_enum_t packet_action;

  if (!route_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null unicast entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_parse(route_entry, vrf_handle, ip_prefix);
  if (vrf_handle.data == SAI_NULL_OBJECT_ID) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Invalid VRF handle: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_route_entry_find_route_handle(vrf_handle, ip_prefix, route_handle);

  if (route_handle.data == SAI_NULL_OBJECT_ID) {
    SAI_LOG_ERROR("Failed to get nexthop for route entry %s : %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return SAI_STATUS_ITEM_NOT_FOUND;
  }

  attr_w nhop_attr(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE);
  switch_status |= bf_switch_attribute_get(
      route_handle, SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nhop_attr);
  nhop_attr.v_get(nhop_handle);

  attr_w action_attr(SWITCH_ROUTE_ATTR_PACKET_ACTION);
  switch_status |= bf_switch_attribute_get(
      route_handle, SWITCH_ROUTE_ATTR_PACKET_ACTION, action_attr);
  action_attr.v_get(packet_action);
  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        attr->value.oid =
            (nhop_handle.data == 0) ? SAI_NULL_OBJECT_ID : nhop_handle.data;
        break;
      case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
        switch (packet_action.enumdata) {
          case SWITCH_ROUTE_ATTR_PACKET_ACTION_DROP:
            attr->value.s32 = SAI_PACKET_ACTION_DROP;
            break;
          case SWITCH_ROUTE_ATTR_PACKET_ACTION_FORWARD:
            attr->value.s32 = SAI_PACKET_ACTION_FORWARD;
            break;
          case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRAP:
            attr->value.s32 = SAI_PACKET_ACTION_TRAP;
            break;
          case SWITCH_ROUTE_ATTR_PACKET_ACTION_DENY:
            attr->value.s32 = SAI_PACKET_ACTION_DENY;
            break;
          case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRANSIT:
            attr->value.s32 = SAI_PACKET_ACTION_TRANSIT;
            break;
          default:
            attr->value.s32 = packet_action.enumdata;
            break;
        }
        break;
      default:
        break;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Bulk create route entry
 *
 * Arguments:
 *    [in] object_count - Number of objects to create
 *    [in] route_entry - List of object to create
 *    [in] attr_count - List of attr_count. Caller passes the number
 *            of attribute for each object to create.
 *    [in] attr_list - List of attributes for every object.
 *    [in] mode-  Bulk operation error handling mode.
 *    [out] object_statuses - List of status for every object. Caller needs to
 *            allocate the buffer
 *
 * Return Values:
 *    #SAI_STATUS_SUCCESS on success when all objects are created or
 *    #SAI_STATUS_FAILURE when any of the objects fails to create. When there is
 *      failure, Caller is expected to go through the list of returned statuses
 *      to find out which fails and which succeeds.
 */
sai_status_t sai_create_route_entries(_In_ uint32_t object_count,
                                      _In_ const sai_route_entry_t *route_entry,
                                      _In_ const uint32_t *attr_count,
                                      _In_ const sai_attribute_t **attr_list,
                                      _In_ sai_bulk_op_error_mode_t mode,
                                      _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!route_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, route_entry: %p, attr_count: %p, attr_list: "
        "%p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        route_entry,
        attr_count,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_create_route_entry(&route_entry[it], attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create route entry #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) break;
    }
  }
  bf_switch_end_batch();

  while (++it < object_count) {
    object_statuses[it] = SAI_STATUS_NOT_EXECUTED;
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 * Routine Description:
 *    Bulk remove route entry
 *
 * Arguments:
 *    [in] object_count - Number of objects to remove
 *    [in] route_entry - List of objects to remove
 *    [in] mode - Bulk operation error handling mode.
 *    [in] object_statuses - List of status for every object. Caller needs to
 *            allocate the buffer
 *
 * Return Values:
 *    #SAI_STATUS_SUCCESS on success when all objects are removed or
 *    #SAI_STATUS_FAILURE when any of the objects fails to remove. When there is
 *     failure, Caller is expected to go through the list of returned statuses
 * to find out which fails and which succeeds.
 */
sai_status_t sai_remove_route_entries(_In_ uint32_t object_count,
                                      _In_ const sai_route_entry_t *route_entry,
                                      _In_ sai_bulk_op_error_mode_t mode,
                                      _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!route_entry || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, route_entry: %p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        route_entry,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_remove_route_entry(&route_entry[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove route entry #%d", it);
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
 * Routine Description:
 *    Bulk set attribute on route entry
 *
 * Arguments:
 *    [in] object_count - Number of objects to set attribute
 *    [in] route_entry - List of objects to set attribute
 *    [in] attr_list - List of attributes to set on objects, one attribute
 *             per object
 *    [in] mode - Bulk operation error handling mode.
 *    [out] object_statuses - List of status for every object. Caller needs to
 *             allocate the buffer
 *
 * Return Values:
 *    #SAI_STATUS_SUCCESS on success when all objects are removed or
 *    #SAI_STATUS_FAILURE when any of the objects fails to remove. When there is
 *     failure, Caller is expected to go through the list of returned statuses
 * to find out which fails and which succeeds.
 */
sai_status_t sai_set_route_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_route_entry_t *route_entry,
    _In_ const sai_attribute_t *attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!route_entry || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, route_entry: %p, attr_list: %p, "
        "object_statuses: %p",
        sai_metadata_get_status_name(status),
        route_entry,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_set_route_entry_attribute(&route_entry[it], &attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set attribute in route entry #%d", it);
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
 * Routine Description:
 *    Bulk get attribute on route entry
 *
 * Arguments:
 *    [in] object_count - Number of objects to set attribute
 *    [in] route_entry - List of objects to set attribute
 *    [in] attr_count - List of attr_count. Caller passes the number
 *           of attribute for each object to get
 *    [inout] attr_list - List of attributes to set on objects, one attribute
 *           per object
 *    [in] mode - Bulk operation error handling mode
 *    [out] object_statuses - List of status for every object. Caller needs to
 *          allocate the buffer
 *
 * Return Values:
 *    #SAI_STATUS_SUCCESS on success when all objects are removed or
 *    #SAI_STATUS_FAILURE when any of the objects fails to remove. When there is
 *     failure, Caller is expected to go through the list of returned statuses
 * to find out which fails and which succeeds.
 */
sai_status_t sai_get_route_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_route_entry_t *route_entry,
    _In_ const uint32_t *attr_count,
    _Inout_ sai_attribute_t **attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!route_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, route_entry: %p, attr_count: %p, attr_list: "
        "%p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        route_entry,
        attr_count,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_get_route_entry_attribute(
        &route_entry[it], attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get attribute in route entry #%d", it);
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
 *  Router entry methods table retrieved with sai_api_query()
 */
sai_route_api_t route_api = {
  create_route_entry : sai_create_route_entry,
  remove_route_entry : sai_remove_route_entry,
  set_route_entry_attribute : sai_set_route_entry_attribute,
  get_route_entry_attribute : sai_get_route_entry_attribute,
  create_route_entries : sai_create_route_entries,
  remove_route_entries : sai_remove_route_entries,
  set_route_entries_attribute : sai_set_route_entries_attribute,
  get_route_entries_attribute : sai_get_route_entries_attribute,
};

sai_route_api_t *sai_route_api_get() { return &route_api; }

sai_status_t sai_route_initialize() {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_DEBUG("Initializing route");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ROUTE_ENTRY);
  device_handle = sai_get_device_id(0);

  attr_w hostif_nhop_attr(SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE);
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE, hostif_nhop_attr);
  hostif_nhop_attr.v_get(hostif_nhop_handle);

  return status_switch_to_sai(switch_status);
}
