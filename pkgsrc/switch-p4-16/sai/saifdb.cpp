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


#include <linux/if_ether.h>
#include <saiinternal.h>

#include <memory>
#include <set>
#include <vector>
#include <mutex>  // NOLINT(build/c++11)

#include "s3/switch_store.h"

#define SAI_FDB_ENTRY_TYPE_STATIC_MACMOVE (SAI_FDB_ENTRY_TYPE_STATIC + 1)

static sai_api_t api_id = SAI_API_FDB;
static switch_object_id_t device_handle = {0};

static void sai_fdb_entry_to_string(_In_ const sai_fdb_entry_t *fdb_entry,
                                    _Out_ char *entry_string) {
  snprintf(entry_string,
           SAI_MAX_ENTRY_STRING_LEN,
           "fdb entry mac [%02x:%02x:%02x:%02x:%02x:%02x]",
           fdb_entry->mac_address[0],
           fdb_entry->mac_address[1],
           fdb_entry->mac_address[2],
           fdb_entry->mac_address[3],
           fdb_entry->mac_address[4],
           fdb_entry->mac_address[5]);
}

static sai_status_t sai_fdb_entry_parse(const sai_fdb_entry_t *fdb_entry,
                                        std::set<smi::attr_w> &sw_attrs) {
  sai_object_type_t obj_type;
  obj_type = sai_object_type_query(fdb_entry->bv_id);

  /* parse fdb vlan id or bridge-id */
  if (obj_type == SAI_OBJECT_TYPE_BRIDGE || obj_type == SAI_OBJECT_TYPE_VLAN) {
    switch_object_id_t vlan_handle = {.data = fdb_entry->bv_id};
    sw_attrs.insert(
        smi::attr_w(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_handle));
  } else {
    return SWITCH_STATUS_NOT_SUPPORTED;
  }

  switch_mac_addr_t sw_mac;
  memcpy(sw_mac.mac, fdb_entry->mac_address, ETH_ALEN);
  sw_attrs.insert(smi::attr_w(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, sw_mac));
  return SWITCH_STATUS_SUCCESS;
}

static bool flush_entry_type_match(sai_fdb_flush_entry_type_t flush_type,
                                   switch_enum_t mac_type) {
  if (flush_type == SAI_FDB_FLUSH_ENTRY_TYPE_ALL) {
    return true;
  }
  if (flush_type == SAI_FDB_FLUSH_ENTRY_TYPE_DYNAMIC) {
    return mac_type.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_DYNAMIC;
  }
  if (flush_type == SAI_FDB_FLUSH_ENTRY_TYPE_STATIC) {
    return mac_type.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC;
  }
  return false;
}

static void mac_entry_info_get(switch_object_id_t mac_handle,
                               switch_mac_addr_t &mac,
                               switch_object_id_t &vlan_handle,
                               switch_object_id_t &dest_handle,
                               sai_object_id_t &bridge_port_id) {
  attr_w mac_attr(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS);
  bf_switch_attribute_get(
      mac_handle, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac_attr);
  mac_attr.v_get(mac);

  if (!vlan_handle.data) {
    attr_w vlan_attr(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE);
    bf_switch_attribute_get(
        mac_handle, SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_attr);
    vlan_attr.v_get(vlan_handle);
  }

  if (!dest_handle.data) {
    attr_w dest_attr(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE);
    bf_switch_attribute_get(
        mac_handle, SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, dest_attr);
    dest_attr.v_get(dest_handle);
  }

  if (bridge_port_id == SAI_NULL_OBJECT_ID) {
    switch_object_type_t ot = switch_store::object_type_query(dest_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT || ot == SWITCH_OBJECT_TYPE_LAG) {
      sai_get_port_to_bridge_port(dest_handle, bridge_port_id);
    } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
      sai_get_tunnel_to_bridge_port(dest_handle, bridge_port_id);
    }
  }
}

/*
 * Routine Description:
 *    Create FDB entry
 *
 * Arguments:
 *    [in] fdb_entry - fdb entry
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_fdb_entry(_In_ const sai_fdb_entry_t *fdb_entry,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_MAC_ENTRY;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!fdb_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null fdb entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_mac_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  status = sai_fdb_entry_parse(fdb_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create fdb entry, key parsing failed : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID: {
        switch_enum_t intf_type = {0};
        status = sai_get_bridge_port_interface_type(attr_list[i].value.oid,
                                                    intf_type);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to get interface type for bridge port: 0x%" PRIx64 ", %s",
              attr_list[i].value.oid,
              sai_metadata_get_status_name(status));
          return status;
        }

        switch_object_id_t sw_handle = {0};
        if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
          status =
              sai_get_port_from_bridge_port(attr_list[i].value.oid, sw_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "failed to get port_handle for bridge port: 0x%" PRIx64 ", %s",
                attr_list[i].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
          status = sai_get_tunnel_from_bridge_port(attr_list[i].value.oid,
                                                   sw_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "failed to get tunnel_handle for bridge port: 0x%" PRIx64
                ", %s",
                attr_list[i].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
          SAI_LOG_ERROR("failed, bridge_port type not supported: 0x%" PRIx64
                        ", %s",
                        attr_list[i].value.oid,
                        sai_metadata_get_status_name(status));
          return status;
        }

        attr_w mac_attr(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, sw_handle);
        sw_attrs.insert(mac_attr);
        break;
      }
      case SAI_FDB_ENTRY_ATTR_TYPE:
        if (attr_list[i].value.s32 == SAI_FDB_ENTRY_TYPE_STATIC_MACMOVE) {
          switch_enum_t type_enum = {0};
          type_enum.enumdata = SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC;
          attr_w type_attr(SWITCH_MAC_ENTRY_ATTR_TYPE, type_enum);
          sw_attrs.insert(type_attr);

          attr_w allow_move(SWITCH_MAC_ENTRY_ATTR_ALLOW_MOVE, true);
          sw_attrs.insert(allow_move);
          break;
        }
        [[fallthrough]];
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_FDB_ENTRY, &attr_list[i], sw_attrs);
        if ((status != SAI_STATUS_SUCCESS) &&
            (status != SAI_STATUS_NOT_SUPPORTED)) {
          sai_fdb_entry_to_string(fdb_entry, entry_string);
          SAI_LOG_ERROR(
              "failed to create fdb entry %s : sai_to_switch attr map failed, "
              "%s",
              entry_string,
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_MAC_ENTRY_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(ot, sw_attrs, switch_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_fdb_entry_to_string(fdb_entry, entry_string);
    SAI_LOG_ERROR("failed to create fdb entry %s : %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Remove FDB entry
 *
 * Arguments:
 *    [in] fdb_entry - fdb entry
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_fdb_entry(_In_ const sai_fdb_entry_t *fdb_entry) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!fdb_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null fdb entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_mac_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  status = sai_fdb_entry_parse(fdb_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove fdb entry, key parsing failed : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_MAC_ENTRY_ATTR_DEVICE, sw_attrs);
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MAC_ENTRY, sw_attrs, switch_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_fdb_entry_to_string(fdb_entry, entry_string);
    SAI_LOG_ERROR("failed to remove fdb entry: %s, key_eror: %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_object_delete(switch_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_fdb_entry_to_string(fdb_entry, entry_string);
    SAI_LOG_ERROR(
        "failed to remove fdb_entry %s, sw_mac_entry_object_id: %" PRIx64
        ": %s",
        entry_string,
        switch_mac_object_id.data,
        sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set fdb entry attribute value
 *
 * Arguments:
 *    [in] fdb_entry - fdb entry
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_fdb_entry_attribute(_In_ const sai_fdb_entry_t *fdb_entry,
                                         _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!fdb_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null fdb entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_mac_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  status = sai_fdb_entry_parse(fdb_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set fdb entry, key parsing failed : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_MAC_ENTRY_ATTR_DEVICE, sw_attrs);
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MAC_ENTRY, sw_attrs, switch_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_fdb_entry_to_string(fdb_entry, entry_string);
    SAI_LOG_ERROR("failed to set fdb entry: %s, key_eror: %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID: {
      switch_enum_t intf_type = {0};
      status = sai_get_bridge_port_interface_type(attr->value.oid, intf_type);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to get interface type for bridge port: 0x%" PRIx64
                      ", %s",
                      attr->value.oid,
                      sai_metadata_get_status_name(status));
        return status;
      }

      switch_object_id_t dest_handle = {0};
      if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
        status = sai_get_port_from_bridge_port(attr->value.oid, dest_handle);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get port_handle for bridge port: 0x%" PRIx64
                        ", %s",
                        attr->value.oid,
                        sai_metadata_get_status_name(status));
          return status;
        }
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_L2_VXLAN)) {
          // clear endpoint_ip if moving from remote vtep
          switch_ip_address_t zero_ip_addr = {};
          memset(&zero_ip_addr, 0x00, sizeof(switch_ip_address_t));
          zero_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_NONE;
          attr_w mac_attr_ip(SWITCH_MAC_ENTRY_ATTR_DEST_IP, zero_ip_addr);
          switch_status =
              bf_switch_attribute_set(switch_mac_object_id, mac_attr_ip);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to set attribute %s error: %s",
                sai_attribute_name(SAI_OBJECT_TYPE_FDB_ENTRY, attr->id),
                sai_metadata_get_status_name(status));
            return status;
          }
        }
      } else if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
        status = sai_get_tunnel_from_bridge_port(attr->value.oid, dest_handle);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to get tunnel_handle for bridge port: 0x%" PRIx64 ", %s",
              attr->value.oid,
              sai_metadata_get_status_name(status));
          return status;
        }
      } else {
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR("failed: bridge_port type not supported: 0x%" PRIx64
                      ", %s",
                      attr->value.oid,
                      sai_metadata_get_status_name(status));
        return status;
      }

      attr_w mac_attr(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, dest_handle);
      switch_status = bf_switch_attribute_set(switch_mac_object_id, mac_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_FDB_ENTRY, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
    } break;
    case SAI_FDB_ENTRY_ATTR_TYPE:
      if (attr->value.s32 == SAI_FDB_ENTRY_TYPE_STATIC_MACMOVE) {
        switch_enum_t type_enum = {0};
        type_enum.enumdata = SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC;
        attr_w type_attr(SWITCH_MAC_ENTRY_ATTR_TYPE, type_enum);
        switch_status =
            bf_switch_attribute_set(switch_mac_object_id, type_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                        sai_attribute_name(SAI_OBJECT_TYPE_FDB_ENTRY, attr->id),
                        sai_metadata_get_status_name(status));
          return status;
        }

        attr_w allow_move(SWITCH_MAC_ENTRY_ATTR_ALLOW_MOVE, true);
        switch_status =
            bf_switch_attribute_set(switch_mac_object_id, allow_move);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                        "SAI_FDB_ENTRY_ATTR_ALLOW_MAC_MOVE",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
      }
      [[fallthrough]];
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_FDB_ENTRY, attr, switch_mac_object_id);
      if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_FDB_ENTRY, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Get fdb entry attribute value
 *
 * Arguments:
 *    [in] fdb_entry - fdb entry
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_fdb_entry_attribute(_In_ const sai_fdb_entry_t *fdb_entry,
                                         _In_ uint32_t attr_count,
                                         _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  char entry_string[SAI_MAX_ENTRY_STRING_LEN];

  SAI_LOG_ENTER();

  if (!fdb_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null fdb entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_object_id_t switch_mac_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  status = sai_fdb_entry_parse(fdb_entry, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set fdb entry, key parsing failed : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_MAC_ENTRY_ATTR_DEVICE, sw_attrs);
  switch_status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_MAC_ENTRY, sw_attrs, switch_mac_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    sai_fdb_entry_to_string(fdb_entry, entry_string);
    SAI_LOG_ERROR("failed to set fdb entry: %s, %s",
                  entry_string,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID: {
        sai_object_id_t bridge_port_id = 0;
        switch_object_id_t dest_handle = {};
        attr_w dest_attr(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE);
        switch_status =
            bf_switch_attribute_get(switch_mac_object_id,
                                    SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                    dest_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          sai_fdb_entry_to_string(fdb_entry, entry_string);
          SAI_LOG_ERROR("failed to get fdb entry, dest_handle: %s, %s",
                        entry_string,
                        sai_metadata_get_status_name(status));
          return status;
        }
        dest_attr.v_get(dest_handle);
        switch_object_type_t ot = switch_store::object_type_query(dest_handle);
        if (ot == SWITCH_OBJECT_TYPE_PORT || ot == SWITCH_OBJECT_TYPE_LAG) {
          status = sai_get_port_to_bridge_port(dest_handle, bridge_port_id);
          if (status != SAI_STATUS_SUCCESS) {
            sai_fdb_entry_to_string(fdb_entry, entry_string);
            SAI_LOG_ERROR(
                "Failed to get fdb entry bridge_port_id: %s mac_object_id: "
                "%" PRIx64 " port_lag_handle: %" PRIx64 ": %s",
                entry_string,
                switch_mac_object_id.data,
                dest_handle.data,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
          status = sai_get_tunnel_to_bridge_port(dest_handle, bridge_port_id);
          if (status != SAI_STATUS_SUCCESS) {
            sai_fdb_entry_to_string(fdb_entry, entry_string);
            SAI_LOG_ERROR(
                "Failed to get fdb entry bridge_port_id: %s mac_object_id: "
                "%" PRIx64 " tunnel_handle: %" PRIx64 ": %s",
                entry_string,
                switch_mac_object_id.data,
                dest_handle.data,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
          SAI_LOG_ERROR("failed: bridge_port type not supported: 0x%" PRIx64
                        ", %s",
                        dest_handle.data,
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[i].value.oid = bridge_port_id;
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_FDB_ENTRY, switch_mac_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS &&
            status != SAI_STATUS_NOT_SUPPORTED) {
          sai_fdb_entry_to_string(fdb_entry, entry_string);
          SAI_LOG_ERROR("Failed to get fdb entry: %s sw_object_id: %" PRIx64
                        ": %s",
                        entry_string,
                        switch_mac_object_id.data,
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

void sai_get_fdb_data(uint32_t attr_count,
                      const sai_attribute_t *attr_list,
                      sai_fdb_flush_entry_type_t &type,
                      switch_object_id_t &bridge_port_oid,
                      switch_object_id_t &vlan_oid) {
  if (attr_count == 0) return;
  if (attr_list == NULL) return;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_FDB_FLUSH_ATTR_ENTRY_TYPE:
        type = static_cast<sai_fdb_flush_entry_type_t>(attr_list[i].value.s32);
        break;
      case SAI_FDB_FLUSH_ATTR_BRIDGE_PORT_ID:
        bridge_port_oid.data = attr_list[i].value.oid;
        break;
      case SAI_FDB_FLUSH_ATTR_BV_ID:
        vlan_oid.data = attr_list[i].value.oid;
        break;
      default:
        break;
    }
  }
  return;
}

/*
 * Routine Description:
 *    Remove all FDB entries by attribute set in sai_fdb_flush_attr
 *
 * Arguments:
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_flush_fdb_entries(_In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t first_handle = {0}, bridge_port_handle = {0},
                     vlan_handle = {0}, dest_handle = {0};
  bool get_more = false;
  sai_fdb_flush_entry_type_t type = {};
  std::vector<switch_object_id_t> handles_list;
  attr_w port_attr(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE);
  attr_w tunnel_attr(SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE);

  SAI_LOG_ENTER();

  sai_get_fdb_data(
      attr_count, attr_list, type, bridge_port_handle, vlan_handle);

  if (bridge_port_handle.data) {
    switch_enum_t intf_type = {0};
    status =
        sai_get_bridge_port_interface_type(bridge_port_handle.data, intf_type);
    if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
      switch_status =
          bf_switch_attribute_get(bridge_port_handle,
                                  SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE,
                                  port_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        status = status_switch_to_sai(switch_status);
        SAI_LOG_ERROR("Invalid bridge port handle :%" PRIx64 " %s",
                      bridge_port_handle.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
      port_attr.v_get(dest_handle);
    } else if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
      switch_status =
          bf_switch_attribute_get(bridge_port_handle,
                                  SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE,
                                  tunnel_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        status = status_switch_to_sai(switch_status);
        SAI_LOG_ERROR("Invalid bridge port handle :%" PRIx64 " %s",
                      bridge_port_handle.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
      tunnel_attr.v_get(dest_handle);
    }
  }

  std::lock_guard<std::mutex> lk_fdb{
      switch_store::smiContext::context().fdb_lock};

  switch_status =
      bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_MAC_ENTRY, first_handle);
  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    // no FDB entries found
    return SAI_STATUS_SUCCESS;
  }
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_ERROR("failed to retrieve first fdb handle : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  handles_list.push_back(first_handle);

  do {
    get_more = false;
    uint32_t num_handles = 0, count = 2048;
    std::vector<switch_object_id_t> local_handles_list;

    switch_status = bf_switch_get_next_handles(
        first_handle, count, local_handles_list, num_handles);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      status = status_switch_to_sai(switch_status);
      SAI_LOG_ERROR("failed to retrieve fdb handles : %s",
                    sai_metadata_get_status_name(status));
      return status;
    }

    // copy temp list to big list
    for (const auto &handle : local_handles_list)
      handles_list.push_back(handle);

    if (local_handles_list.size() == count) {
      get_more = true;
      first_handle = local_handles_list.back();
    }
  } while (get_more == true);

  for (const auto &mac_handle : handles_list) {
    switch_object_id_t vlan_t_handle = {}, dest_t_handle = {};
    sai_object_id_t bridge_port_id = SAI_NULL_OBJECT_ID;

    if (!switch_store::object_try_lock(mac_handle)) {
      /* skip if the object does not exist by the time we process it */
      continue;
    }

    if (dest_handle.data) {
      attr_w dest_attr(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE);
      bf_switch_attribute_get(
          mac_handle, SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE, dest_attr);
      dest_attr.v_get(dest_t_handle);
    }

    if (vlan_handle.data) {
      attr_w vlan_attr(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE);
      bf_switch_attribute_get(
          mac_handle, SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_attr);
      vlan_attr.v_get(vlan_t_handle);
    }

    if (dest_handle != dest_t_handle || vlan_handle != vlan_t_handle) {
      switch_store::object_unlock(mac_handle);
      continue;
    }

    switch_enum_t mac_type = {};
    attr_w mac_type_attr(SWITCH_MAC_ENTRY_ATTR_TYPE);
    bf_switch_attribute_get(
        mac_handle, SWITCH_MAC_ENTRY_ATTR_TYPE, mac_type_attr);
    mac_type_attr.v_get(mac_type);

    if (!flush_entry_type_match(type, mac_type)) {
      switch_store::object_unlock(mac_handle);
      continue;
    }

    if (sai_switch_notifications.on_fdb_event) {
      sai_fdb_event_notification_data_t ev = {};
      switch_mac_addr_t mac = {};
      sai_attribute_t attr[3];

      mac_entry_info_get(
          mac_handle, mac, vlan_t_handle, dest_t_handle, bridge_port_id);

      attr[0].id = SAI_FDB_ENTRY_ATTR_TYPE;
      if (mac_type.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC) {
        attr[0].value.s32 = SAI_FDB_ENTRY_TYPE_STATIC;
      } else {
        attr[0].value.s32 = SAI_FDB_ENTRY_TYPE_DYNAMIC;
      }
      attr[1].id = SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID;
      attr[1].value.oid = bridge_port_id;
      attr[2].id = SAI_FDB_ENTRY_ATTR_PACKET_ACTION;
      attr[2].value.s32 = SAI_PACKET_ACTION_FORWARD;

      /* TODO: needs to be FLUSH, possibly consolidated */
      ev.event_type = SAI_FDB_EVENT_AGED;
      ev.attr = attr;
      ev.attr_count = 3;
      ev.fdb_entry.switch_id = device_handle.data;
      ev.fdb_entry.bv_id = vlan_t_handle.data;
      memcpy(ev.fdb_entry.mac_address, mac.mac, ETH_ALEN);

      sai_switch_notifications.on_fdb_event(1, &ev);
    }

    switch_status = bf_switch_object_delete(mac_handle);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      switch_store::object_unlock(mac_handle);
      status = status_switch_to_sai(switch_status);
      SAI_LOG_ERROR("failed to delete fdb handle 0x%" PRIx64 ": %s",
                    mac_handle.data,
                    sai_metadata_get_status_name(status));
      // continue to try and delete other handles
    }
  }

  SAI_LOG_EXIT();
  return status_switch_to_sai(switch_status);
}

/*
 * Routine Description:
 *    Bulk create FDB entry
 *
 * Arguments:
 *    [in] object_count Number of objects to create
 *    [in] fdb_entry List of object to create
 *    [in] attr_count List of attr_count.
 *    [in] attr_list List of attributes for every object.
 *    [in] mode Bulk operation error handling mode.
 *    [out] object_statuses List of status for every object.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    SAI_STATUS_FAILURE when any of the objects fails to create.
 */
sai_status_t sai_create_fdb_entries(_In_ uint32_t object_count,
                                    _In_ const sai_fdb_entry_t *fdb_entry,
                                    _In_ const uint32_t *attr_count,
                                    _In_ const sai_attribute_t **attr_list,
                                    _In_ sai_bulk_op_error_mode_t mode,
                                    _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!fdb_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (fdb_entry = %p, attr_count = %p, attr_list = "
        "%p, object_statuses = %p): %s",
        fdb_entry,
        attr_count,
        attr_list,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_create_fdb_entry(fdb_entry++, attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create fdb entry #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) {
        break;
      }
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
 *    Bulk remove FDB entry
 *
 * Arguments:
 *    [in] object_count Number of objects to remove
 *    [in] fdb_entry List of objects to remove
 *    [in] mode Bulk operation error handling mode.
 *    [out] object_statuses List of status for every object.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success when all objects are removed
 *    SAI_STATUS_FAILURE when any of the objects fails to remove.
 */
sai_status_t sai_remove_fdb_entries(_In_ uint32_t object_count,
                                    _In_ const sai_fdb_entry_t *fdb_entry,
                                    _In_ sai_bulk_op_error_mode_t mode,
                                    _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!fdb_entry || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (fdb_entry = %p, object_statuses = %p): %s",
        fdb_entry,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_remove_fdb_entry(fdb_entry++);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove fdb entry #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) {
        break;
      }
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
 *    Bulk set attribute on FDB entry
 *
 * Arguments:
 *    [in] object_count Number of objects to set attribute
 *    [in] fdb_entry List of objects to set attribute
 *    [in] attr_list List of attributes to set on objects, one attribute per
 * object
 *    [in] mode Bulk operation error handling mode.
 *    [out] object_statuses List of status for every object.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success when all objects are set.
 *    SAI_STATUS_FAILURE when any of the objects fails to set.
 */
sai_status_t sai_set_fdb_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_fdb_entry_t *fdb_entry,
    _In_ const sai_attribute_t *attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!fdb_entry || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (fdb_entry = %p, attr_list = %p, object_statuses "
        "= %p): %s",
        fdb_entry,
        attr_list,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_set_fdb_entry_attribute(fdb_entry++, attr_list++);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set fdb entry #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) {
        break;
      }
    }
  }
  bf_switch_start_batch();
  while (++it < object_count) {
    object_statuses[it] = SAI_STATUS_NOT_EXECUTED;
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 * Routine Description:
 *    Bulk get attribute on FDB entry
 *
 * Arguments:
 *    [in] object_count Number of objects to get attribute
 *    [in] fdb_entry List of objects to get attribute
 *    [in] attr_count List of attr_count.
 *    [inout] attr_list List of attributes to get on objects, one attribute per
 * object
 *    [in] mode Bulk operation error handling mode
 *    [out] object_statuses List of status for every object.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success when all objects are get.
 *    SAI_STATUS_FAILURE when any of the objects fails to get.
 */
sai_status_t sai_get_fdb_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_fdb_entry_t *fdb_entry,
    _In_ const uint32_t *attr_count,
    _Inout_ sai_attribute_t **attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!fdb_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (fdb_entry = %p, attr_count = %p, attr_list = "
        "%p, object_statuses = %p): %s",
        fdb_entry,
        attr_count,
        attr_list,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_get_fdb_entry_attribute(fdb_entry++, attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get fdb entry #%d", it);
      status = SAI_STATUS_FAILURE;
      if (mode == SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR) {
        break;
      }
    }
  }
  bf_switch_end_batch();
  while (++it < object_count) {
    object_statuses[it] = SAI_STATUS_NOT_EXECUTED;
  }

  SAI_LOG_EXIT();
  return status;
}

static sai_fdb_event_t switch_mac_event_to_sai_fdb_event(
    switch_mac_event_t mac_event) {
  switch (mac_event) {
    case SWITCH_MAC_EVENT_MOVE:
      return SAI_FDB_EVENT_MOVE;
    case SWITCH_MAC_EVENT_DELETE:
      return SAI_FDB_EVENT_FLUSHED;
    case SWITCH_MAC_EVENT_AGE:
      return SAI_FDB_EVENT_AGED;
    case SWITCH_MAC_EVENT_CREATE:
    case SWITCH_MAC_EVENT_LEARN:
    default:
      return SAI_FDB_EVENT_LEARNED;
  }
}

void sai_mac_notify_cb(const switch_mac_event_data_t &data) {
  SAI_LOG_ENTER();
  uint16_t num_entries = data.payload.size();
  uint16_t entry = 0;

  if (num_entries == 0) {
    SAI_LOG_DEBUG("sai mac notify callback with null entries");
    return;
  }

  std::unique_ptr<sai_fdb_event_notification_data_t[]> fdb_event(
      new sai_fdb_event_notification_data_t[num_entries]());
  std::unique_ptr<sai_attribute_t[][3]> attr_lists(
      new sai_attribute_t[num_entries][3]());

  SAI_LOG_DEBUG("Num FDB events: %d", num_entries);
  for (const switch_mac_payload_t payload : data.payload) {
    if (payload.mac_event == SWITCH_MAC_EVENT_DELETE) {
      continue;
    }

    sai_fdb_event_t fdb_type =
        switch_mac_event_to_sai_fdb_event(payload.mac_event);
    SAI_LOG_DEBUG("Event type: %s", sai_metadata_get_fdb_event_name(fdb_type));
    fdb_event[entry].event_type = fdb_type;

    attr_w mac_attr(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS);
    switch_mac_addr_t mac = {0};
    bf_switch_attribute_get(
        payload.mac_handle, SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, mac_attr);
    mac_attr.v_get(mac);
    attr_w vlan_attr(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE);
    switch_object_id_t vlan_handle = {0};
    bf_switch_attribute_get(
        payload.mac_handle, SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE, vlan_attr);
    vlan_attr.v_get(vlan_handle);
    sai_object_id_t bridge_port_id = 0;
    sai_get_port_to_bridge_port(payload.port_lag_handle, bridge_port_id);
    if (bridge_port_id == 0) {
      SAI_LOG_ERROR("Bridge port not found for port 0x%" PRIx64
                    " , %s, %02x:%02x:%02x:%02x:%02x:%02x",
                    payload.port_lag_handle.data,
                    sai_metadata_get_fdb_event_name(fdb_type),
                    mac.mac[0],
                    mac.mac[1],
                    mac.mac[2],
                    mac.mac[3],
                    mac.mac[4],
                    mac.mac[5]);
      return;
    }

    memcpy(fdb_event[entry].fdb_entry.mac_address, mac.mac, ETH_ALEN);
    fdb_event[entry].fdb_entry.switch_id = device_handle.data;
    fdb_event[entry].fdb_entry.bv_id = vlan_handle.data;

    attr_lists[entry][0].id = SAI_FDB_ENTRY_ATTR_TYPE;
    attr_lists[entry][0].value.s32 = SAI_FDB_ENTRY_TYPE_DYNAMIC;
    attr_lists[entry][1].id = SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID;
    attr_lists[entry][1].value.oid = bridge_port_id;
    attr_lists[entry][2].id = SAI_FDB_ENTRY_ATTR_PACKET_ACTION;
    attr_lists[entry][2].value.s32 = SAI_PACKET_ACTION_FORWARD;
    fdb_event[entry].attr_count = 3;
    fdb_event[entry].attr = attr_lists[entry];
    entry++;
  }

  if (sai_switch_notifications.on_fdb_event && entry)
    sai_switch_notifications.on_fdb_event(entry, fdb_event.get());

  SAI_LOG_EXIT();
  return;
}

/*
 *  FDB methods table retrieved with sai_api_query()
 */
sai_fdb_api_t fdb_api = {
  create_fdb_entry : sai_create_fdb_entry,
  remove_fdb_entry : sai_remove_fdb_entry,
  set_fdb_entry_attribute : sai_set_fdb_entry_attribute,
  get_fdb_entry_attribute : sai_get_fdb_entry_attribute,
  flush_fdb_entries : sai_flush_fdb_entries,
  create_fdb_entries : sai_create_fdb_entries,
  remove_fdb_entries : sai_remove_fdb_entries,
  set_fdb_entries_attribute : sai_set_fdb_entries_attribute,
  get_fdb_entries_attribute : sai_get_fdb_entries_attribute
};

sai_fdb_api_t *sai_fdb_api_get() { return &fdb_api; }

sai_status_t sai_fdb_initialize() {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_DEBUG("Initializing fdb");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_FDB_ENTRY);
  device_handle = sai_get_device_id(0);

  bf_switch_event_register(SWITCH_MAC_EVENT,
                           (void *)&sai_mac_notify_cb);  // NOLINT

  (void)sai_switch_set_learn_notif_timeout(device_handle, SAI_L2_LEARN_TIMEOUT);

  // By default, dynamic FDB entry aging must be zero - means aging is disabled.
  uint32_t aging_interval = 0;
  attr_w age_attr(SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL, aging_interval);
  switch_status = bf_switch_attribute_set(device_handle, age_attr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_ERROR("Failed to set FDB entry aging time, error: %s",
                  sai_metadata_get_status_name(status));
  }
  return SAI_STATUS_SUCCESS;
}
