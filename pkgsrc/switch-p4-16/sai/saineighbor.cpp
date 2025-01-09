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


#include <arpa/inet.h>
#include <saiinternal.h>

#include <set>
#include <vector>

#include "s3/switch_store.h"
#include "s3/attribute_util.h"

static sai_api_t api_id = SAI_API_NEIGHBOR;
static switch_object_id_t device_handle = {0};

static std::vector<sai_packet_action_t> supported_neighbor_entry_packet_actions{
    SAI_PACKET_ACTION_FORWARD};

sai_status_t sai_get_neighbor_entry_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;

  if (!enum_values_capability) return SAI_STATUS_INVALID_PARAMETER;

  if (attr_id == SAI_NEIGHBOR_ENTRY_ATTR_PACKET_ACTION) {
    if (enum_values_capability->count >=
        supported_neighbor_entry_packet_actions.size()) {
      for (const auto &packet_action :
           supported_neighbor_entry_packet_actions) {
        enum_values_capability->list[i] = packet_action;
        i++;
      }
    } else {
      enum_values_capability->count =
          supported_neighbor_entry_packet_actions.size();
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    return status;
  }
  enum_values_capability->count = i;
  return status;
}

static void sai_neighbor_entry_to_string(
    _In_ const sai_neighbor_entry_t *neighbor_entry, _Out_ char *entry_string) {
  int count = 0;
  int entry_length = 0;
  count = snprintf(entry_string,
                   SAI_MAX_ENTRY_STRING_LEN,
                   "neighbor:  rif_oid 0x%" PRIx64 ", ip ",
                   neighbor_entry->rif_id);
  sai_ipaddress_to_string(neighbor_entry->ip_address,
                          SAI_MAX_ENTRY_STRING_LEN - count,
                          entry_string + count,
                          &entry_length);
  return;
}

static sai_status_t sai_neighbor_entry_parse(
    const sai_neighbor_entry_t *neighbor_entry,
    std::set<smi::attr_w> &sw_attrs,
    bool &regular_rif) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  regular_rif = true;

  if (sai_object_type_query(neighbor_entry->rif_id) !=
      SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
  sw_attrs.insert(smi::attr_w(SWITCH_NEIGHBOR_ATTR_HANDLE, rif_handle));

  switch_ip_address_t ip = {};
  sai_ip_addr_to_switch_ip_addr(&neighbor_entry->ip_address, ip);
  sw_attrs.insert(smi::attr_w(SWITCH_NEIGHBOR_ATTR_DEST_IP, ip));

  // determine RIF object type
  sai_attribute_t rif_attribute{0};

  rif_attribute.id = SAI_ROUTER_INTERFACE_ATTR_TYPE;
  status = sai_to_switch_attribute_get(
      SAI_OBJECT_TYPE_ROUTER_INTERFACE, rif_handle, &rif_attribute);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to determine RIF type error: %s\n",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_router_interface_type_t sai_intf_type =
      static_cast<sai_router_interface_type_t>(rif_attribute.value.s32);
  if (sai_intf_type == SAI_ROUTER_INTERFACE_TYPE_MPLS_ROUTER) {
    regular_rif = false;
  }

  return SAI_STATUS_SUCCESS;
}

static sai_status_t sai_get_neighbor_entry_handle(
    const sai_neighbor_entry_t *neighbor_entry,
    switch_object_id_t &nbr_handle) {
  std::set<smi::attr_w> nbr_lookup_key_attrs;
  switch_status_t switch_status;
  sai_status_t status;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  if (sai_object_type_query(neighbor_entry->rif_id) !=
      SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
  switch_ip_address_t dest_ip = {};
  sai_ip_addr_to_switch_ip_addr(&neighbor_entry->ip_address, dest_ip);

  /* build nexthop object lookup key */
  nbr_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEIGHBOR_ATTR_DEVICE, device_handle));
  nbr_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEIGHBOR_ATTR_HANDLE, rif_handle));
  nbr_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEIGHBOR_ATTR_DEST_IP, dest_ip));

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_NEIGHBOR, nbr_lookup_key_attrs, nbr_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_neighbor_entry_to_string(neighbor_entry, entry_string);
    SAI_LOG_ERROR(
        "failed to get nbr handle for neighbor: %s : switch_status: %s",
        entry_string,
        sai_metadata_get_status_name(status));
  }

  return status;
}

static sai_status_t sai_neighbor_entry_nexthop_get(
    const sai_neighbor_entry_t *neighbor_entry,
    switch_object_id_t &nhop_handle,
    bool regular_rif) {
  std::set<smi::attr_w> nexthop_lookup_key_attrs;
  switch_status_t switch_status;
  sai_status_t status;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];
  uint32_t vni = 0;
  switch_enum_t label_op = {.enumdata = SWITCH_NEXTHOP_ATTR_LABELOP_NONE};
  std::vector<uint32_t> label_list;

  SAI_ASSERT(sai_object_type_query(neighbor_entry->rif_id) ==
             SAI_OBJECT_TYPE_ROUTER_INTERFACE);

  switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
  switch_ip_address_t dest_ip = {};
  sai_ip_addr_to_switch_ip_addr(&neighbor_entry->ip_address, dest_ip);
  switch_enum_t nhop_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_IP};
  switch_object_id_t sidlist_handle = {};

  /* build IP nexthop object lookup key */
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_DEVICE, device_handle));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_DEST_IP, dest_ip));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, vni));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELOP, label_op));
  nexthop_lookup_key_attrs.insert(
      smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list));

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_NEXTHOP, nexthop_lookup_key_attrs, nhop_handle);
  status = status_switch_to_sai(switch_status);

  if (switch_status != SWITCH_STATUS_ITEM_NOT_FOUND) {
    return status;
  } else {
    /* look for MPLS nexthop */
    nexthop_lookup_key_attrs.erase(
        smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));
    nhop_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_MPLS};
    nexthop_lookup_key_attrs.insert(
        smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));

    switch_status = bf_switch_object_get(
        SWITCH_OBJECT_TYPE_NEXTHOP, nexthop_lookup_key_attrs, nhop_handle);
    status = status_switch_to_sai(switch_status);
  }

  /* Create a nexthop if no nexthop is found and store it locally */
  if (regular_rif == true) {
    nexthop_lookup_key_attrs.erase(
        smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));
    nhop_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_IP};
    nexthop_lookup_key_attrs.insert(
        smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));
  }

  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    switch_status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_NEXTHOP, nexthop_lookup_key_attrs, nhop_handle);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_DEBUG(
          "failed to create internal nexthop handle for neighbor: %s : "
          "switch_status: %s",
          entry_string,
          sai_metadata_get_status_name(status));
      return status;
    }
  }
  return status;
}

/*
 * Routine Description:
 *    Create neighbor entry
 *
 * Arguments:
 *    [in] neighbor_entry - neighbor entry
 *    [in] attr_count - number of attributes
 *    [in] attrs - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 *
 * Note: IP address expected in Network Byte Order.
 */
sai_status_t sai_create_neighbor_entry(
    _In_ const sai_neighbor_entry_t *neighbor_entry,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_NEIGHBOR;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];
  bool set_host_route = true;
  bool regular_rif = true;
  uint32_t index;

  if (!neighbor_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_neighbor_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  status = sai_neighbor_entry_parse(neighbor_entry, sw_attrs, regular_rif);
  if (status != SAI_STATUS_SUCCESS) {
    sai_neighbor_entry_to_string(neighbor_entry, entry_string);
    SAI_LOG_ERROR(
        "failed to create neighbor entry %s : sai_neighbor_entry_parse "
        "failed, status: %s ",
        entry_string,
        sai_metadata_get_status_name(status));
    return status;
  }
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_NEIGHBOR_ENTRY_ATTR_NO_HOST_ROUTE:
        set_host_route = !attribute->value.booldata;
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, attribute, sw_attrs);
        if ((status != SAI_STATUS_SUCCESS) &&
            (status != SAI_STATUS_NOT_SUPPORTED)) {
          sai_neighbor_entry_to_string(neighbor_entry, entry_string);
          SAI_LOG_ERROR(
              "failed to create neighbor entry %s : sai_to_switch attr map "
              "failed, attribute: %s, status: %s ",
              entry_string,
              sai_attribute_name(SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  switch_object_id_t nexthop_handle = {};

  /* lookup and insert nexthop handle */
  status = sai_neighbor_entry_nexthop_get(
      neighbor_entry, nexthop_handle, regular_rif);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to create neighbor entry %s : failed to find nexthop "
        "status: %s ",
        entry_string,
        sai_metadata_get_status_name(status));
    return status;
  }
  sw_attrs.insert(
      smi::attr_w(SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE, nexthop_handle));

  /* insert device handle */
  sai_insert_device_attribute(0, SWITCH_NEIGHBOR_ATTR_DEVICE, sw_attrs);

  /* create neighbor object */
  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_neighbor_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_neighbor_entry_to_string(neighbor_entry, entry_string);
    SAI_LOG_ERROR("failed to create neighbor entry: %s: %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_ip_prefix_t prefix = {.len = 32};
  sai_ip_addr_to_switch_ip_addr(&neighbor_entry->ip_address, prefix.addr);
  if (neighbor_entry->ip_address.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    prefix.len = 128;
  }

  // If neighbor-ip is link local /128, then no host route
  if (switch_ipv6_prefix_link_local_host_ip(prefix)) {
    set_host_route = false;
  }

  if (set_host_route && nexthop_handle.data) {
    switch_object_id_t switch_route_handle = {};
    std::set<smi::attr_w> route_attrs;

    /* get vrf_handle */
    switch_object_id_t vrf_handle = {};
    attr_w vrf_handle_attr(SWITCH_RIF_ATTR_VRF_HANDLE);
    switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
    switch_status = bf_switch_attribute_get(
        rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle_attr);
    vrf_handle_attr.v_get(vrf_handle);

    /* build and create host route for nbr entry */
    sai_insert_device_attribute(0, SWITCH_ROUTE_ATTR_DEVICE, route_attrs);
    route_attrs.insert(smi::attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE, vrf_handle));
    route_attrs.insert(smi::attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));

    if (!(bf_switch_is_feature_enabled(SWITCH_FEATURE_IPV4_LOCAL_HOST))) {
      // when SWITCH_FEATURE_IPV4_LOCAL_HOST is disable
      // same route could be created in sai_create_route_entry and here
      // to handle this we use is_nbr_sourced and is_route_sourced flags
      // So we should search for route sourced before create new obj
      route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, false));
      switch_status = bf_switch_object_get(
          SWITCH_OBJECT_TYPE_ROUTE, route_attrs, switch_route_handle);

      if (switch_status == SWITCH_STATUS_SUCCESS) {
        // if there is route sourced object then update existing obj
        switch_status = bf_switch_attribute_set(
            switch_route_handle,
            attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, true));
        switch_status =
            bf_switch_attribute_set(switch_route_handle,
                                    attr_w(SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE,
                                           switch_neighbor_object_id));
        switch_status = bf_switch_attribute_set(
            switch_route_handle,
            attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nexthop_handle));

        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          sai_neighbor_entry_to_string(neighbor_entry, entry_string);
          SAI_LOG_ERROR(
              "failed to set is_nbr_sourced for host route for neighbor entry: "
              "%s : %s",
              entry_string,
              sai_metadata_get_status_name(status));
          return status;
        }
      }
    }

    if (switch_route_handle.data == 0) {
      // when SWITCH_FEATURE_IPV4_LOCAL_HOST is enabled
      // or same route did not exist then create new
      route_attrs.insert(
          attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nexthop_handle));
      route_attrs.erase(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED));
      route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, true));
      route_attrs.insert(
          attr_w(SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE, switch_neighbor_object_id));
      switch_status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_ROUTE, route_attrs, switch_route_handle);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        sai_neighbor_entry_to_string(neighbor_entry, entry_string);
        SAI_LOG_ERROR("failed to create host route for neighbor entry: %s : %s",
                      entry_string,
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  // If there are routes pointing to this RIF, update their nexthop
  switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
  attr_w rif_attr(SWITCH_RIF_ATTR_TYPE);
  switch_enum_t rif_type = {};
  switch_status =
      bf_switch_attribute_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_attr);
  rif_attr.v_get(rif_type);
  std::set<switch_object_id_t> ref_oids;
  switch_status = switch_store::referencing_set_get(
      rif_handle, SWITCH_OBJECT_TYPE_ROUTE, ref_oids);
  for (const auto route_handle : ref_oids) {
    // only update the routes for peer VRF routes
    switch_object_id_t route_vrf_handle = {}, rif_vrf_handle = {};
    switch_ip_prefix_t route_prefix = {};
    attr_w route_vrf_attr(SWITCH_ROUTE_ATTR_VRF_HANDLE);
    attr_w route_prefix_attr(SWITCH_ROUTE_ATTR_IP_PREFIX);
    attr_w rif_vrf_attr(SWITCH_RIF_ATTR_VRF_HANDLE);

    switch_status = bf_switch_attribute_get(
        route_handle, SWITCH_ROUTE_ATTR_VRF_HANDLE, route_vrf_attr);
    switch_status = bf_switch_attribute_get(
        route_handle, SWITCH_ROUTE_ATTR_IP_PREFIX, route_prefix_attr);
    switch_status = bf_switch_attribute_get(
        rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, rif_vrf_attr);
    route_vrf_attr.v_get(route_vrf_handle);
    route_prefix_attr.v_get(route_prefix);
    rif_vrf_attr.v_get(rif_vrf_handle);

    // If rif is SVI and rif vrf is not the same as route vrf then update the
    // route with this neighbor NH data in case it's not a directed broadcast
    // one. This is needed to pass SONiC vxlan CTs which
    // uses inter-vrf forwarding and expects that the packet forwarded will
    // be sent using the learned neighbor NH data instead of directed broadcast
    // NH.
    if (route_vrf_handle.data == rif_vrf_handle.data) continue;

    if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) &&
        attr_util::is_dir_bcast_addr(route_prefix.len, prefix.addr))
      continue;

    attr_w route_attr(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nexthop_handle);
    switch_status = bf_switch_attribute_set(route_handle, route_attr);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      status = status_switch_to_sai(switch_status);
      SAI_LOG_ERROR("Failed to set route handle: %" PRIx64
                    " nhop to neighbor: %" PRIx64 ": %s",
                    route_handle.data,
                    nexthop_handle.data,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove neighbor entry
 *
 * Arguments:
 *    [in] neighbor_entry - neighbor entry
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 *
 * Note: IP address expected in Network Byte Order.
 */
sai_status_t sai_remove_neighbor_entry(
    _In_ const sai_neighbor_entry_t *neighbor_entry) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  if (!neighbor_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_neighbor_entry_to_string(neighbor_entry, entry_string);

  switch_object_id_t switch_nbr_handle;
  status = sai_get_neighbor_entry_handle(neighbor_entry, switch_nbr_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return (SAI_STATUS_SUCCESS);
  }

  /*
    remove host route
        ignore error!
  */

  /* get vrf_handle */
  switch_object_id_t vrf_handle = {};
  attr_w vrf_handle_attr(SWITCH_RIF_ATTR_VRF_HANDLE);
  switch_object_id_t rif_handle = {.data = neighbor_entry->rif_id};
  switch_status = bf_switch_attribute_get(
      rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle_attr);
  vrf_handle_attr.v_get(vrf_handle);

  /*  nbr host route */
  switch_ip_prefix_t prefix = {.len = 32};
  sai_ip_addr_to_switch_ip_addr(&neighbor_entry->ip_address, prefix.addr);
  if (neighbor_entry->ip_address.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    prefix.len = 128;
  }

  /* find and delete host route */
  std::set<smi::attr_w> route_attrs;
  switch_object_id_t switch_route_handle;
  route_attrs.insert(smi::attr_w(SWITCH_ROUTE_ATTR_DEVICE, device_handle));
  route_attrs.insert(smi::attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE, (vrf_handle)));
  route_attrs.insert(smi::attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));
  route_attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, true));
  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_ROUTE, route_attrs, switch_route_handle);

  if (switch_status == SWITCH_STATUS_SUCCESS) {
    // if obj is_route_sourced then set is_nbr_sourced to false
    // and do not delete the obj. See sai_create_neighbor_entry for more info
    attr_w is_route_sourced_attr(SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED);
    switch_status = bf_switch_attribute_get(switch_route_handle,
                                            SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED,
                                            is_route_sourced_attr);

    bool is_route_sourced = false;
    is_route_sourced_attr.v_get(is_route_sourced);

    if (is_route_sourced) {
      switch_object_id_t nbr_handle = {SWITCH_NULL_OBJECT_ID};
      bf_switch_attribute_set(
          switch_route_handle,
          attr_w(SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE, nbr_handle));
      bf_switch_attribute_set(switch_route_handle,
                              attr_w(SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, false));
    } else {
      bf_switch_object_delete(switch_route_handle);
    }
  }

  /* delete nexthop */
  attr_w nhop_attr(SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE);
  switch_object_id_t nexthop_handle = {}, clear_handle = {};
  switch_status = bf_switch_attribute_get(
      switch_nbr_handle, SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE, nhop_attr);
  nhop_attr.v_get(nexthop_handle);
  if (nexthop_handle.data != 0) {
    /* clear the neighbor's nexthop attribute */
    attr_w clear_attr(SWITCH_NEIGHBOR_ATTR_NEXTHOP_HANDLE, clear_handle);
    switch_status = bf_switch_attribute_set(switch_nbr_handle, clear_attr);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to clear internal nexthop for neighbor: %s",
                    entry_string);
    }
    sai_route_set_glean_internal(nexthop_handle);
    /* nexthop all set for deletion. remove it */
    bool del = false;
    switch_status = switch_store::object_ready_for_delete(nexthop_handle, del);
    if (del == true) {
      switch_status = bf_switch_object_delete(nexthop_handle);
      status = status_switch_to_sai(switch_status);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to delete internal nexthop 0x%" PRIx64
                      " for neighbor: %s",
                      nexthop_handle.data,
                      entry_string);
      }
    }
  }

  /* delete neighbor */
  switch_status = bf_switch_object_delete(switch_nbr_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_neighbor_entry_to_string(neighbor_entry, entry_string);
    SAI_LOG_ERROR("failed to delete neighbor entry: %s : %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    status = SAI_STATUS_SUCCESS;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set neighbor attribute value
 *
 * Arguments:
 *    [in] neighbor_entry - neighbor entry
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_neighbor_entry_attribute(
    _In_ const sai_neighbor_entry_t *neighbor_entry,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!neighbor_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_nbr_handle;
  status = sai_get_neighbor_entry_handle(neighbor_entry, switch_nbr_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return (SAI_STATUS_SUCCESS);
  }

  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, attr, switch_nbr_handle);
  if ((status != SAI_STATUS_SUCCESS) && (status != SAI_STATUS_NOT_SUPPORTED)) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s\n",
                  sai_attribute_name(SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get neighbor attribute value
 *
 * Arguments:
 *    [in] neighbor_entry - neighbor entry
 *    [in] attr_count - number of attributes
 *    [inout] attrs - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_neighbor_entry_attribute(
    _In_ const sai_neighbor_entry_t *neighbor_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  if (!neighbor_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null neighbor entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_nbr_handle;
  status = sai_get_neighbor_entry_handle(neighbor_entry, switch_nbr_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to get neighbor entry, attribute get failed, status: %s",
        sai_metadata_get_status_name(status));
    return (SAI_STATUS_SUCCESS);
  }

  status = sai_to_switch_attribute_list_get(
      SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, switch_nbr_handle, attr_count, attr_list);
  if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
    sai_neighbor_entry_to_string(neighbor_entry, entry_string);
    SAI_LOG_ERROR("Failed to get nbr entry: %s sw_object_id: 0x%" PRIx64
                  ": %s\n",
                  entry_string,
                  switch_nbr_handle.data,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 *  Neighbor methods table retrieved with sai_api_query()
 */
sai_neighbor_api_t neighbor_api = {
    .create_neighbor_entry = sai_create_neighbor_entry,
    .remove_neighbor_entry = sai_remove_neighbor_entry,
    .set_neighbor_entry_attribute = sai_set_neighbor_entry_attribute,
    .get_neighbor_entry_attribute = sai_get_neighbor_entry_attribute};

sai_neighbor_api_t *sai_neighbor_api_get() { return &neighbor_api; }

sai_status_t sai_neighbor_initialize() {
  SAI_LOG_DEBUG("Initializing neighbor");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_NEIGHBOR_ENTRY);
  device_handle = sai_get_device_id(0);

  if (bf_switch_is_feature_enabled(SWITCH_FEATURE_UDT_TYPE_NEIGHBOR)) {
    supported_neighbor_entry_packet_actions.push_back(SAI_PACKET_ACTION_TRAP);
  }

  return SAI_STATUS_SUCCESS;
}
