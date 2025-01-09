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
#include <cstring>
#include <list>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_ROUTER_INTERFACE;
static switch_object_id_t device_handle = {0};

static const sai_to_switch_counters_map rif_to_switch_counter_mapping{
    {SAI_ROUTER_INTERFACE_STAT_IN_OCTETS,
     {SWITCH_RIF_COUNTER_ID_IN_UCAST_BYTES,
      SWITCH_RIF_COUNTER_ID_IN_MCAST_BYTES,
      SWITCH_RIF_COUNTER_ID_IN_BCAST_BYTES}},
    {SAI_ROUTER_INTERFACE_STAT_OUT_OCTETS,
     {SWITCH_RIF_COUNTER_ID_OUT_UCAST_BYTES,
      SWITCH_RIF_COUNTER_ID_OUT_MCAST_BYTES,
      SWITCH_RIF_COUNTER_ID_OUT_BCAST_BYTES}},
    {SAI_ROUTER_INTERFACE_STAT_IN_PACKETS,
     {SWITCH_RIF_COUNTER_ID_IN_UCAST_PKTS,
      SWITCH_RIF_COUNTER_ID_IN_MCAST_PKTS,
      SWITCH_RIF_COUNTER_ID_IN_BCAST_PKTS}},
    {SAI_ROUTER_INTERFACE_STAT_OUT_PACKETS,
     {SWITCH_RIF_COUNTER_ID_OUT_UCAST_PKTS,
      SWITCH_RIF_COUNTER_ID_OUT_MCAST_PKTS,
      SWITCH_RIF_COUNTER_ID_OUT_BCAST_PKTS}}};

sai_status_t sai_get_router_interface_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;
  static std::vector<sai_router_interface_type_t>
      supported_router_interface_types = {SAI_ROUTER_INTERFACE_TYPE_PORT,
                                          SAI_ROUTER_INTERFACE_TYPE_VLAN,
                                          SAI_ROUTER_INTERFACE_TYPE_LOOPBACK,
                                          SAI_ROUTER_INTERFACE_TYPE_SUB_PORT};
  if (bf_switch_is_feature_enabled(SWITCH_FEATURE_QINQ_RIF))
    supported_router_interface_types.push_back(
        SAI_ROUTER_INTERFACE_TYPE_QINQ_PORT);

  if (!enum_values_capability) return SAI_STATUS_INVALID_PARAMETER;

  if (attr_id == SAI_ROUTER_INTERFACE_ATTR_TYPE) {
    if (enum_values_capability->count >=
        supported_router_interface_types.size()) {
      for (const auto &type : supported_router_interface_types) {
        enum_values_capability->list[i] = type;
        i++;
      }
    } else {
      enum_values_capability->count = supported_router_interface_types.size();
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    return status;
  }
  enum_values_capability->count = i;
  return status;
}

sai_status_t query_rif_stats_capability(
    sai_stat_capability_list_t &stats_capability) {
  static const uint16_t supported_count =
      supported_counters_count(rif_to_switch_counter_mapping);
  return query_stats_capability_by_mapping(
      rif_to_switch_counter_mapping, stats_capability, supported_count);
}

/*
 * Routine Description:
 *    Create router interface.
 *
 * Arguments:
 *    [out] rif_id - router interface id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_router_interface(
    _Out_ sai_object_id_t *rif_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_RIF;
  sai_router_interface_type_t sai_intf_type;
  uint32_t index = 0;
  switch_object_id_t vrf_handle = {0};
  bool src_mac_found = false;

  if (!rif_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  attribute = sai_get_attr_from_list(
      SAI_ROUTER_INTERFACE_ATTR_TYPE, attr_list, attr_count);
  if (attribute == NULL) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("missing attribute %s %d",
                  sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                     SAI_ROUTER_INTERFACE_ATTR_TYPE),
                  status_switch_to_sai(status));
    return status;
  }
  sai_intf_type =
      static_cast<sai_router_interface_type_t>(attribute->value.s32);

  attribute = sai_get_attr_from_list(
      SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID, attr_list, attr_count);
  if (attribute == NULL) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "missing attribute %s %d",
        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                           SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID),
        status_switch_to_sai(status));
    return status;
  }
  vrf_handle.data = attribute->value.oid;

  switch_object_id_t rif_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  switch (sai_intf_type) {
    case SAI_ROUTER_INTERFACE_TYPE_PORT:
    case SAI_ROUTER_INTERFACE_TYPE_SUB_PORT: {
      uint16_t outer_vlan_id = 0;
      attribute = sai_get_attr_from_list(
          SAI_ROUTER_INTERFACE_ATTR_PORT_ID, attr_list, attr_count);
      if (attribute == NULL) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
        return status;
      }
      switch_object_id_t port_lag_handle = {.data = attribute->value.oid};
      switch_enum_t rif_type = {.enumdata = SWITCH_RIF_ATTR_TYPE_PORT};
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle));

      if (sai_intf_type == SAI_ROUTER_INTERFACE_TYPE_SUB_PORT) {
        attribute = sai_get_attr_from_list(
            SAI_ROUTER_INTERFACE_ATTR_OUTER_VLAN_ID, attr_list, attr_count);
        if (attribute == NULL) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR(
              "missing attribute %s, %d",
              sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                 SAI_ROUTER_INTERFACE_ATTR_OUTER_VLAN_ID),
              status_switch_to_sai(status));
          return status;
        }
        outer_vlan_id = attribute->value.u16;
        sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_OUTER_VLAN_ID, outer_vlan_id));
        rif_type.enumdata = SWITCH_RIF_ATTR_TYPE_SUB_PORT;
      }
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, rif_type));

      sai_object_type_t obj_type = sai_object_type_query(port_lag_handle.data);
      if (obj_type != SAI_OBJECT_TYPE_PORT && obj_type != SAI_OBJECT_TYPE_LAG) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("invalid attribute %s, %d",
                      sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                         SAI_ROUTER_INTERFACE_ATTR_PORT_ID),
                      status_switch_to_sai(status));
        return status;
      }

      const auto &rif_attr_ref_map = switch_store::get_object_references(
          port_lag_handle, SWITCH_OBJECT_TYPE_RIF);

      for (const auto &rif_attr_ref : rif_attr_ref_map) {
        attr_w anycast_mac_support_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
        attr_w is_virtual_attr(SWITCH_RIF_ATTR_IS_VIRTUAL);
        attr_w second_rif_type_attr(SWITCH_RIF_ATTR_TYPE);
        bool is_second_rif_virtual = false;
        bool is_second_rif_anycast = false;
        bool same_outer_vlan_ids = true;  // true in case of type PORT
        bool is_virtual = false;
        bool is_anycast = false;
        switch_enum_t second_rif_type;

        switch_status |= bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_TYPE, second_rif_type_attr);
        second_rif_type_attr.v_get(second_rif_type);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        if (rif_type.enumdata != second_rif_type.enumdata) continue;

        attribute = sai_get_attr_from_list(
            SAI_ROUTER_INTERFACE_ATTR_IS_VIRTUAL, attr_list, attr_count);
        if (attribute != NULL) is_virtual = attribute->value.booldata;
        switch_status |= bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual_attr);
        is_virtual_attr.v_get(is_second_rif_virtual);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to retrieve is_virtual attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        attribute = sai_get_attr_from_list(
            SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0, attr_list, attr_count);
        if (attribute != NULL) is_anycast = attribute->value.booldata;

        switch_status |=
            bf_switch_attribute_get(rif_attr_ref.oid,
                                    SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                                    anycast_mac_support_attr);
        anycast_mac_support_attr.v_get(is_second_rif_anycast);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to retrieve anycast_mac_support attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        if (sai_intf_type == SAI_ROUTER_INTERFACE_TYPE_SUB_PORT) {
          same_outer_vlan_ids = false;
          attr_w outer_vlan_id_attr(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
          uint16_t second_outer_vlan_id = 0;

          switch_status |=
              bf_switch_attribute_get(rif_attr_ref.oid,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      outer_vlan_id_attr);
          outer_vlan_id_attr.v_get(second_outer_vlan_id);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
          if (outer_vlan_id == second_outer_vlan_id) same_outer_vlan_ids = true;
        }

        if (is_anycast && !is_second_rif_virtual && same_outer_vlan_ids) {
          switch_mac_addr_t anycast_mac = {};

          attribute = sai_get_attr_from_list(
              SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS, attr_list, attr_count);
          if (attribute == NULL) {
            status = SAI_STATUS_INVALID_PARAMETER;
            SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
            return status;
          }
          std::memcpy(&anycast_mac, &attribute->value.mac, sizeof(sai_mac_t));

          attr_w anycast_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR,
                                  anycast_mac);
          switch_status |=
              bf_switch_attribute_set(rif_attr_ref.oid, anycast_mac_attr);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to create router interface: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
        } else if (!is_virtual && is_second_rif_anycast &&
                   same_outer_vlan_ids) {
          attr_w anycast_mac_attr(SWITCH_RIF_ATTR_SRC_MAC);
          switch_mac_addr_t anycast_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

          switch_status |= bf_switch_attribute_get(
              rif_attr_ref.oid, SWITCH_RIF_ATTR_SRC_MAC, anycast_mac_attr);
          anycast_mac_attr.v_get(anycast_mac);
          if (status_switch_to_sai(switch_status) == SAI_STATUS_SUCCESS) {
            sw_attrs.insert(
                attr_w(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, anycast_mac));
          }
        }
      }
    } break;
    case SAI_ROUTER_INTERFACE_TYPE_BRIDGE:
      break;
    case SAI_ROUTER_INTERFACE_TYPE_VLAN: {
      attribute = sai_get_attr_from_list(
          SAI_ROUTER_INTERFACE_ATTR_VLAN_ID, attr_list, attr_count);
      if (attribute == NULL) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
        return status;
      }
      switch_object_id_t vlan_object_handle = {.data = attribute->value.oid};
      switch_enum_t rif_type_vlan = {.enumdata = SWITCH_RIF_ATTR_TYPE_VLAN};
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, rif_type_vlan));
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_object_handle));

      sai_object_type_t obj_type =
          sai_object_type_query(vlan_object_handle.data);
      if (obj_type != SAI_OBJECT_TYPE_VLAN) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("invalid attribute %s, %d",
                      sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                         SAI_ROUTER_INTERFACE_ATTR_VLAN_ID),
                      status_switch_to_sai(status));
        return status;
      }

      const auto &rif_attr_ref_map = switch_store::get_object_references(
          vlan_object_handle, SWITCH_OBJECT_TYPE_RIF);
      for (const auto &rif_attr_ref : rif_attr_ref_map) {
        attr_w is_virtual_attr(SWITCH_RIF_ATTR_IS_VIRTUAL);
        attr_w anycast_mac_support_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
        bool is_second_rif_virtual = false;
        bool is_second_rif_anycast = false;
        bool is_virtual = false;
        bool is_anycast = false;

        attribute = sai_get_attr_from_list(
            SAI_ROUTER_INTERFACE_ATTR_IS_VIRTUAL, attr_list, attr_count);
        if (attribute != NULL) is_virtual = attribute->value.booldata;
        switch_status |= bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual_attr);
        is_virtual_attr.v_get(is_second_rif_virtual);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to retrieve is_virtual attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        attribute = sai_get_attr_from_list(
            SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0, attr_list, attr_count);
        if (attribute != NULL) is_anycast = attribute->value.booldata;

        switch_status |=
            bf_switch_attribute_get(rif_attr_ref.oid,
                                    SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                                    anycast_mac_support_attr);
        anycast_mac_support_attr.v_get(is_second_rif_anycast);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to retrieve anycast_mac_support attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        if (is_anycast && !is_second_rif_virtual) {
          switch_mac_addr_t anycast_mac = {};

          attribute = sai_get_attr_from_list(
              SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS, attr_list, attr_count);
          if (attribute == NULL) {
            status = SAI_STATUS_INVALID_PARAMETER;
            SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
            return status;
          }
          std::memcpy(&anycast_mac, &attribute->value.mac, sizeof(sai_mac_t));

          attr_w anycast_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR,
                                  anycast_mac);
          switch_status |=
              bf_switch_attribute_set(rif_attr_ref.oid, anycast_mac_attr);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to create router interface: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
        } else if (!is_virtual && is_second_rif_anycast) {
          attr_w anycast_mac_attr(SWITCH_RIF_ATTR_SRC_MAC);
          switch_mac_addr_t anycast_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

          switch_status |= bf_switch_attribute_get(
              rif_attr_ref.oid, SWITCH_RIF_ATTR_SRC_MAC, anycast_mac_attr);
          anycast_mac_attr.v_get(anycast_mac);
          if (status_switch_to_sai(switch_status) == SAI_STATUS_SUCCESS) {
            sw_attrs.insert(
                attr_w(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, anycast_mac));
          }
        }
      }
    } break;
    case SAI_ROUTER_INTERFACE_TYPE_LOOPBACK: {
      switch_object_id_t loopback_handle = {.data = SAI_NULL_OBJECT_ID};
      switch_enum_t rif_type_loopback = {.enumdata =
                                             SWITCH_RIF_ATTR_TYPE_LOOPBACK};
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, rif_type_loopback));
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, loopback_handle));
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_VLAN_HANDLE, loopback_handle));
    } break;
    case SAI_ROUTER_INTERFACE_TYPE_QINQ_PORT: {
      attribute = sai_get_attr_from_list(
          SAI_ROUTER_INTERFACE_ATTR_PORT_ID, attr_list, attr_count);
      if (attribute == NULL) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
        return status;
      }
      switch_object_id_t port_lag_object_handle = {.data =
                                                       attribute->value.oid};
      switch_enum_t rif_type_qinq_port = {.enumdata =
                                              SWITCH_RIF_ATTR_TYPE_QINQ_PORT};
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, rif_type_qinq_port));
      sw_attrs.insert(
          attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_object_handle));

      attribute = sai_get_attr_from_list(
          SAI_ROUTER_INTERFACE_ATTR_OUTER_VLAN_ID, attr_list, attr_count);
      if (attribute == 0) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
        return status;
      }
      sw_attrs.insert(
          attr_w(SWITCH_RIF_ATTR_OUTER_VLAN_ID, attribute->value.u16));

      attribute = sai_get_attr_from_list(
          SAI_ROUTER_INTERFACE_ATTR_INNER_VLAN_ID, attr_list, attr_count);
      if (attribute == 0) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("missing attribute %d", status_switch_to_sai(status));
        return status;
      }
      sw_attrs.insert(
          attr_w(SWITCH_RIF_ATTR_INNER_VLAN_ID, attribute->value.u16));
    } break;
    case SAI_ROUTER_INTERFACE_TYPE_MPLS_ROUTER: {
      switch_enum_t mpls_rif = {.enumdata = SWITCH_RIF_ATTR_TYPE_MPLS};
      sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, mpls_rif));
    } break;
    default:
      SAI_LOG_WARN("Unsupported intf type %d\n", sai_intf_type);
      return SAI_STATUS_NOT_SUPPORTED;
      break;
  }

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      /* skip cases */
      case SAI_ROUTER_INTERFACE_ATTR_TYPE:
      case SAI_ROUTER_INTERFACE_ATTR_PORT_ID:
      case SAI_ROUTER_INTERFACE_ATTR_VLAN_ID:
      case SAI_ROUTER_INTERFACE_ATTR_OUTER_VLAN_ID:
      case SAI_ROUTER_INTERFACE_ATTR_INNER_VLAN_ID:
      case SAI_ROUTER_INTERFACE_ATTR_INGRESS_ACL:
      case SAI_ROUTER_INTERFACE_ATTR_EGRESS_ACL:
      case SAI_ROUTER_INTERFACE_ATTR_NEIGHBOR_MISS_PACKET_ACTION:
        break;
      case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS: {
        switch_mac_addr_t mac = {};
        std::memcpy(&mac, &attribute->value.mac, sizeof(sai_mac_t));
        sw_attrs.insert(smi::attr_w(SWITCH_RIF_ATTR_SRC_MAC, mac));
        src_mac_found = true;
      } break;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0:  // customattr
        // = SAI_ROUTER_INTERFACE_ATTR_END
        sw_attrs.insert(attr_w(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                               attribute->value.booldata));
        break;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_5: {  // customattr
        switch_mac_addr_t mac = {};
        std::memcpy(&mac, &attribute->value.mac, sizeof(sai_mac_t));
        sw_attrs.insert(smi::attr_w(SWITCH_RIF_ATTR_PEER_SRC_MAC, mac));
      } break;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_1: {  // customattr
        switch_mac_addr_t mac = {};
        std::memcpy(&mac, &attribute->value.mac, sizeof(sai_mac_t));
        sw_attrs.insert(smi::attr_w(SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, mac));
      } break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_ROUTER_INTERFACE, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create router interface: %s, attribute: %s",
                        sai_metadata_get_status_name(status),
                        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                           attribute->id));
          return status;
        }
        break;
    }
  }

  if (!src_mac_found) {
    attr_w vrf_attr(SWITCH_VRF_ATTR_SRC_MAC);
    switch_status =
        bf_switch_attribute_get(vrf_handle, SWITCH_VRF_ATTR_SRC_MAC, vrf_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to create router interface: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    switch_mac_addr_t mac = {};
    vrf_attr.v_get(mac);
    sw_attrs.insert(smi::attr_w(SWITCH_RIF_ATTR_SRC_MAC, mac));
  }
  sw_attrs.insert(smi::attr_w(SWITCH_RIF_ATTR_DEVICE, device_handle));

  switch_status = bf_switch_object_create(ot, sw_attrs, rif_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    // TODO(bfn) support multiple rif of type loopback?
    if (sai_intf_type == SAI_ROUTER_INTERFACE_TYPE_LOOPBACK) {
      *rif_id = SAI_NULL_OBJECT_ID;
      return SAI_STATUS_SUCCESS;
    }
    SAI_LOG_ERROR("failed to create rif interface: %s",
                  sai_metadata_get_status_name(status));
  }

  *rif_id = rif_object_id.data;
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove router interface
 *
 * Arguments:
 *    [in] rif_id - router interface id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_router_interface(_In_ sai_object_id_t rif_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  attr_w anycast_mac_support_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
  bool anycast_mac_support = false;
  switch_object_id_t handle;
  switch_enum_t rif_type = {};
  attr_w rif_type_attr(SWITCH_RIF_ATTR_TYPE);

  if (sai_object_type_query(rif_id) != SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    SAI_LOG_ERROR("Invalid object type: 0x%" PRIx64, rif_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_rif_object_id = {.data = rif_id};

  switch_status = bf_switch_attribute_get(
      sw_rif_object_id, SWITCH_RIF_ATTR_TYPE, rif_type_attr);
  rif_type_attr.v_get(rif_type);

  switch_status |= bf_switch_attribute_get(sw_rif_object_id,
                                           SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                                           anycast_mac_support_attr);
  anycast_mac_support_attr.v_get(anycast_mac_support);
  if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to retrieve anycast_mac_support attribute: %s",
                  sai_metadata_get_status_name(status));
  }
  if (anycast_mac_support) {
    if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
         rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT)) {
      switch_object_id_t port_lag_handle;
      attr_w port_handle_attr(SWITCH_RIF_ATTR_PORT_HANDLE);

      switch_status = bf_switch_attribute_get(
          sw_rif_object_id, SWITCH_RIF_ATTR_PORT_HANDLE, port_handle_attr);
      port_handle_attr.v_get(port_lag_handle);
      handle = port_lag_handle;
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      switch_object_id_t vlan_handle;
      attr_w vlan_handle_attr(SWITCH_RIF_ATTR_VLAN_HANDLE);

      switch_status = bf_switch_attribute_get(
          sw_rif_object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle_attr);
      vlan_handle_attr.v_get(vlan_handle);
      handle = vlan_handle;
    }

    const auto &rif_attr_ref_map =
        switch_store::get_object_references(handle, SWITCH_OBJECT_TYPE_RIF);

    for (const auto &rif_attr_ref : rif_attr_ref_map) {
      attr_w second_anycast_mac_support_attr(
          SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
      bool second_anycast_mac_support = false;
      switch_mac_addr_t first_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
      attr_w first_mac_attr(SWITCH_RIF_ATTR_SRC_MAC);
      switch_mac_addr_t second_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
      attr_w second_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR);

      switch_status |=
          bf_switch_attribute_get(rif_attr_ref.oid,
                                  SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                                  second_anycast_mac_support_attr);
      second_anycast_mac_support_attr.v_get(second_anycast_mac_support);
      if ((status = status_switch_to_sai(switch_status)) !=
          SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to retrieve anycast_mac_support attribute: %s",
                      sai_metadata_get_status_name(status));
      }

      switch_status |= bf_switch_attribute_get(
          sw_rif_object_id, SWITCH_RIF_ATTR_SRC_MAC, first_mac_attr);
      first_mac_attr.v_get(first_mac);
      if ((status = status_switch_to_sai(switch_status)) !=
          SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to retrieve src_mac attribute: %s",
                      sai_metadata_get_status_name(status));
      }

      switch_status |= bf_switch_attribute_get(
          rif_attr_ref.oid, SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, second_mac_attr);
      second_mac_attr.v_get(second_mac);
      if ((status = status_switch_to_sai(switch_status)) !=
          SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to retrieve anycast_mac_addr attribute: %s",
                      sai_metadata_get_status_name(status));
      }

      if (!second_anycast_mac_support && (first_mac == second_mac)) {
        switch_mac_addr_t zeroed_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
        attr_w zeroed_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, zeroed_mac);
        switch_status |=
            bf_switch_attribute_set(rif_attr_ref.oid, zeroed_mac_attr);
        if ((status = status_switch_to_sai(switch_status)) !=
            SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to zero anycast_mac_addr: %s",
                        sai_metadata_get_status_name(status));
        }
      }
    }
  }

  switch_status |= bf_switch_object_delete(sw_rif_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove router interface: %s",
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set router interface attribute
 *
 * Arguments:
 *    [in] rif_id - router interface id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_router_interface_attribute(
    _In_ sai_object_id_t rif_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %d", status_switch_to_sai(status));
    return status;
  }

  if (sai_object_type_query(rif_id) != SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    SAI_LOG_ERROR("Invalid object type: 0x%" PRIx64, rif_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_rif_object_id = {.data = rif_id};

  switch (attr->id) {
    case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS: {
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ROUTER_INTERFACE, attr, sw_rif_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set attribute %s error: %s\n",
            sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }

      bool is_anycast = {};
      switch_enum_t rif_type = {};
      switch_object_id_t handle = {};
      attr_w rif_type_attr(SWITCH_RIF_ATTR_TYPE);
      attr_w is_anycast_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);

      switch_status = bf_switch_attribute_get(
          sw_rif_object_id, SWITCH_RIF_ATTR_TYPE, rif_type_attr);
      rif_type_attr.v_get(rif_type);

      switch_status =
          bf_switch_attribute_get(sw_rif_object_id,
                                  SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT,
                                  is_anycast_attr);
      is_anycast_attr.v_get(is_anycast);

      if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
           rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) &&
          is_anycast) {
        switch_object_id_t port_lag_handle;
        attr_w port_handle_attr(SWITCH_RIF_ATTR_PORT_HANDLE);

        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_PORT_HANDLE, port_handle_attr);
        port_handle_attr.v_get(port_lag_handle);
        handle = port_lag_handle;
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN && is_anycast) {
        switch_object_id_t vlan_handle;
        attr_w vlan_handle_attr(SWITCH_RIF_ATTR_VLAN_HANDLE);

        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle_attr);
        vlan_handle_attr.v_get(vlan_handle);
        handle = vlan_handle;
      }

      const auto &rif_attr_ref_map =
          switch_store::get_object_references(handle, SWITCH_OBJECT_TYPE_RIF);
      for (const auto &rif_attr_ref : rif_attr_ref_map) {
        attr_w is_virtual_attr(SWITCH_RIF_ATTR_IS_VIRTUAL);
        attr_w anycast_mac_support_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
        bool is_second_rif_virtual = false;
        bool same_outer_vlan_ids = true;

        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          same_outer_vlan_ids = false;
          attr_w second_outer_vlan_id_attr(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
          attr_w outer_vlan_id_attr(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
          uint16_t second_outer_vlan_id = 0;
          uint16_t outer_vlan_id = 0;

          switch_status |=
              bf_switch_attribute_get(sw_rif_object_id,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      outer_vlan_id_attr);
          outer_vlan_id_attr.v_get(outer_vlan_id);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }

          switch_status |=
              bf_switch_attribute_get(rif_attr_ref.oid,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      second_outer_vlan_id_attr);
          second_outer_vlan_id_attr.v_get(second_outer_vlan_id);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
          if (outer_vlan_id == second_outer_vlan_id) same_outer_vlan_ids = true;
        }

        switch_status = bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual_attr);
        is_virtual_attr.v_get(is_second_rif_virtual);

        if (!is_second_rif_virtual && same_outer_vlan_ids) {
          switch_mac_addr_t mac = {};
          std::memcpy(&mac, attr->value.mac, sizeof(sai_mac_t));
          attr_w sw_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, mac);
          switch_status = bf_switch_attribute_set(rif_attr_ref.oid, sw_attr);
          status = status_switch_to_sai(switch_status);
        }
      }
    } break;
    case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0: {  // customattr
      // = SAI_ROUTER_INTERFACE_ATTR_END
      attr_w sw_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT, attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_rif_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
      switch_object_id_t handle = {};

      switch_enum_t rif_type = {};
      attr_w rif_type_attr(SWITCH_RIF_ATTR_TYPE);

      switch_status = bf_switch_attribute_get(
          sw_rif_object_id, SWITCH_RIF_ATTR_TYPE, rif_type_attr);
      rif_type_attr.v_get(rif_type);

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        switch_object_id_t port_lag_handle;
        attr_w port_handle_attr(SWITCH_RIF_ATTR_PORT_HANDLE);

        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_PORT_HANDLE, port_handle_attr);
        port_handle_attr.v_get(port_lag_handle);
        handle = port_lag_handle;
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t vlan_handle;
        attr_w vlan_handle_attr(SWITCH_RIF_ATTR_VLAN_HANDLE);

        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle_attr);
        vlan_handle_attr.v_get(vlan_handle);

        handle = vlan_handle;
      }
      const auto &rif_attr_ref_map =
          switch_store::get_object_references(handle, SWITCH_OBJECT_TYPE_RIF);
      for (const auto &rif_attr_ref : rif_attr_ref_map) {
        switch_enum_t second_rif_type = {};
        attr_w second_rif_type_attr(SWITCH_RIF_ATTR_TYPE);
        bool same_outer_vlan_ids = true;

        switch_status = bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_TYPE, second_rif_type_attr);
        second_rif_type_attr.v_get(second_rif_type);
        if (rif_type.enumdata != second_rif_type.enumdata) continue;

        attr_w is_virtual_attr(SWITCH_RIF_ATTR_IS_VIRTUAL);
        attr_w anycast_mac_support_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
        bool is_second_rif_virtual = false;
        bool is_anycast = attr->value.booldata;

        switch_status = bf_switch_attribute_get(
            rif_attr_ref.oid, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual_attr);
        is_virtual_attr.v_get(is_second_rif_virtual);

        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          same_outer_vlan_ids = false;
          attr_w second_outer_vlan_id_attr(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
          attr_w outer_vlan_id_attr(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
          uint16_t second_outer_vlan_id = 0;
          uint16_t outer_vlan_id = 0;

          switch_status |=
              bf_switch_attribute_get(sw_rif_object_id,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      outer_vlan_id_attr);
          outer_vlan_id_attr.v_get(outer_vlan_id);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }

          switch_status |=
              bf_switch_attribute_get(rif_attr_ref.oid,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      second_outer_vlan_id_attr);
          second_outer_vlan_id_attr.v_get(second_outer_vlan_id);
          if ((status = status_switch_to_sai(switch_status)) !=
              SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to retrieve outer_vlan_id attribute: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
          if (outer_vlan_id == second_outer_vlan_id) same_outer_vlan_ids = true;
        }

        if (is_anycast && !is_second_rif_virtual && same_outer_vlan_ids) {
          attr_w src_mac_attr(SWITCH_RIF_ATTR_SRC_MAC);
          switch_mac_addr_t anycast_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

          switch_status = bf_switch_attribute_get(
              sw_rif_object_id, SWITCH_RIF_ATTR_SRC_MAC, src_mac_attr);
          src_mac_attr.v_get(anycast_mac);

          attr_w anycast_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR,
                                  anycast_mac);
          switch_status =
              bf_switch_attribute_set(rif_attr_ref.oid, anycast_mac_attr);
        } else if (!is_anycast && !is_second_rif_virtual &&
                   same_outer_vlan_ids) {
          switch_mac_addr_t zeroed_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
          attr_w zeroed_mac_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_ADDR, zeroed_mac);

          switch_status =
              bf_switch_attribute_set(rif_attr_ref.oid, zeroed_mac_attr);
        }
      }
    } break;
    case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_5: {  // customattr
      switch_mac_addr_t mac = {};
      std::memcpy(&mac, attr->value.mac, sizeof(sai_mac_t));
      attr_w sw_attr(SWITCH_RIF_ATTR_PEER_SRC_MAC, mac);
      switch_status = bf_switch_attribute_set(sw_rif_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
    } break;
    case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_1: {  // customattr
      switch_mac_addr_t mac = {};
      std::memcpy(&mac, attr->value.mac, sizeof(sai_mac_t));
      attr_w sw_attr(SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, mac);
      switch_status = bf_switch_attribute_set(sw_rif_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
    } break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_ROUTER_INTERFACE, attr, sw_rif_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set attribute %s error: %s\n",
            sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Get router interface attribute
 *
 * Arguments:
 *    [in] rif_id - router interface id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_router_interface_attribute(
    _In_ sai_object_id_t rif_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  uint32_t index;
  sai_attribute_t *attribute = NULL;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  if (sai_object_type_query(rif_id) != SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    SAI_LOG_ERROR("Invalid object type: 0x%" PRIx64, rif_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %d", status_switch_to_sai(status));
    return status;
  }

  const switch_object_id_t sw_rif_object_id = {.data = rif_id};
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_ROUTER_INTERFACE_ATTR_EGRESS_ACL:
        SAI_LOG_ERROR("RIF set unsupported attr %s\n",
                      sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                         attribute->id));
        return SAI_STATUS_NOT_SUPPORTED;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0: {  // customattr
        // = SAI_ROUTER_INTERFACE_ATTR_END
        attr_w sw_attr(SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT);
        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_ANYCAST_MAC_SUPPORT, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get custom attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                           attribute->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(attribute->value.booldata);
      } break;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_5: {  // customattr
        switch_mac_addr_t mac = {};
        attr_w sw_attr(SWITCH_RIF_ATTR_PEER_SRC_MAC);
        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_PEER_SRC_MAC, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get custom attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                           attribute->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(mac);
        memcpy(&attribute->value.mac, &mac, sizeof(mac));
      } break;
      case SAI_ROUTER_INTERFACE_ATTR_CUSTOM_1: {  // customattr
        switch_mac_addr_t mac = {};
        attr_w sw_attr(SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE);
        switch_status = bf_switch_attribute_get(
            sw_rif_object_id, SWITCH_RIF_ATTR_SRC_MAC_RIF_UPDATE, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get custom attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                           attribute->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(mac);
        memcpy(&attribute->value.mac, &mac, sizeof(mac));
      } break;

      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_ROUTER_INTERFACE, sw_rif_object_id, attribute);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_ROUTER_INTERFACE,
                                           attribute->id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get router interface statistics counters
 *
 * Arguments:
 *    [in] router_interface_id Router interface id
 *    [in] number_of_counters Number of counters in the array
 *    [in] counter_ids Specifies the array of counter ids
 *    [out] counters Array of resulting counter values.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_router_interface_stats(
    _In_ sai_object_id_t rif_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> rif_cntrs;

  if (sai_object_type_query(rif_id) != SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, rif_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_rif_object_id = {.data = rif_id};
  switch_status = bf_switch_counters_get(sw_rif_object_id, rif_cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get rif stats rif_object_id: %" PRIx64 ": %s",
                  rif_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < number_of_counters; i++) {
    counters[i] = get_counters_count_by_id(
        rif_to_switch_counter_mapping, counter_ids[i], rif_cntrs);
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Get router interface statistics counters extended.
 *
 * Arguments:
 *    [in] router_interface_id Router interface id
 *    [in] number_of_counters Number of counters in the array
 *    [in] counter_ids Specifies the array of counter ids
 *    [in] mode Statistics mode
 *    [out] counters Array of resulting counter values.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_router_interface_stats_ext(
    _In_ sai_object_id_t router_interface_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _In_ sai_stats_mode_t mode,
    _Out_ uint64_t *counters) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

/*
 * Routine Description:
 *    Clear router interface statistics counters.
 *
 *    [in] router_interface_id Router interface id
 *    [in] number_of_counters Number of counters in the array
 *    [in] counter_ids Specifies the array of counter ids
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_clear_router_interface_stats(
    _In_ sai_object_id_t router_interface_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::list<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("Rif stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(router_interface_id) !=
      SAI_OBJECT_TYPE_ROUTER_INTERFACE) {
    SAI_LOG_ERROR("Rif stats clear failed: invalid rif handle 0x%" PRIx64,
                  router_interface_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = router_interface_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    const auto counters_it = rif_to_switch_counter_mapping.find(counter_ids[i]);
    if (counters_it != rif_to_switch_counter_mapping.end() &&
        !counters_it->second.empty()) {
      cntr_ids.insert(cntr_ids.end(),
                      counters_it->second.begin(),
                      counters_it->second.end());
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(
        sw_object_id, std::vector<uint16_t>(cntr_ids.begin(), cntr_ids.end()));
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear rif 0x%" PRIx64 " stats, status %s",
                    router_interface_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routing interface methods table retrieved with sai_api_query()
 */
sai_router_interface_api_t rif_api = {
    .create_router_interface = sai_create_router_interface,
    .remove_router_interface = sai_remove_router_interface,
    .set_router_interface_attribute = sai_set_router_interface_attribute,
    .get_router_interface_attribute = sai_get_router_interface_attribute,
    .get_router_interface_stats = sai_get_router_interface_stats,
    .get_router_interface_stats_ext = sai_get_router_interface_stats_ext,
    .clear_router_interface_stats = sai_clear_router_interface_stats,
};

sai_router_interface_api_t *sai_router_interface_api_get() { return &rif_api; }

sai_status_t sai_router_interface_initialize() {
  SAI_LOG_DEBUG("Initializing router interface");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_ROUTER_INTERFACE);

  device_handle = sai_get_device_id(0);
  return SAI_STATUS_SUCCESS;
}
