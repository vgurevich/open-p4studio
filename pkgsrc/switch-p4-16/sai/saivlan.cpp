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
#include <vector>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_VLAN;
static switch_object_id_t default_stp = {};

/**
 * Routine Description:
 *     Retrieve VLAN OID
 *
 * Arguments:
 *     [out] vlan_oid SAI VLAN OID
 *     [in] vlan_id VLAN ID
 *
 * Return Values:
 *     SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_get_vlan_oid_by_vlan_id(_Out_ sai_object_id_t *vlan_oid,
                                         _In_ sai_uint16_t vlan_id) {
  switch_object_id_t vlan_handle = {};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  std::set<attr_w> vlan_attrs;

  if (!vlan_oid) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_VLAN_ATTR_DEVICE, vlan_attrs);
  vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));

  switch_status =
      bf_switch_object_get(SWITCH_OBJECT_TYPE_VLAN, vlan_attrs, vlan_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to find vlan %u : %s",
                  vlan_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  *vlan_oid = vlan_handle.data;
  return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *     Create a VLAN
 *
 * Arguments:
 *     [out] vlan_id VLAN ID
 *     [in] switch_id Switch id
 *     [in] attr_count Number of attributes
 *     [in] attr_list Array of attributes
 *
 * Return Values:
 *     SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_vlan_entry(_Out_ sai_object_id_t *vlan_object_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_VLAN;
  switch_object_id_t switch_vlan_object_id = {}, stp_handle = {};
  std::set<smi::attr_w> sw_attrs;

  if (!vlan_object_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vlan_object_id = SAI_NULL_OBJECT_ID;
  stp_handle = default_stp;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_VLAN_ATTR_LEARN_DISABLE: {
        bool learning = (attr_list[i].value.booldata ? false : true);
        attr_w vlan_attr(SWITCH_VLAN_ATTR_LEARNING, learning);
        sw_attrs.insert(vlan_attr);
        break;
      }
      case SAI_VLAN_ATTR_CUSTOM_IGMP_SNOOPING_ENABLE:
        sw_attrs.insert(attr_w(SWITCH_VLAN_ATTR_IGMP_SNOOPING,
                               attr_list[i].value.booldata));
        sw_attrs.insert(
            attr_w(SWITCH_VLAN_ATTR_MLD_SNOOPING, attr_list[i].value.booldata));
        break;
      case SAI_VLAN_ATTR_CUSTOM_0:  // customattr for arp suppression
        // = SAI_VLAN_ATTR_END
        sw_attrs.insert(attr_w(SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE,
                               attr_list[i].value.booldata));
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_VLAN, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create vlan: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_VLAN_ATTR_DEVICE, sw_attrs);
  sw_attrs.insert(attr_w(SWITCH_VLAN_ATTR_STP_HANDLE, stp_handle));
  switch_status = bf_switch_object_create(ot, sw_attrs, switch_vlan_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create vlan: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vlan_object_id = switch_vlan_object_id.data;
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove a VLAN
 *
 * Arguments:
 *    [in] sai_object_id_t vlan id - a handle
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_vlan_entry(_In_ sai_object_id_t vlan_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(vlan_id) != SAI_OBJECT_TYPE_VLAN) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  switch_object_id_t switch_id = sai_get_device_id(0);
  sai_attribute_t attr;
  attr.id = SAI_FDB_FLUSH_ATTR_BV_ID;
  attr.value.oid = vlan_id;
  status = sai_flush_fdb_entries(switch_id.data, 1, &attr);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_WARN("failed to flush FDB entries by vlan handle 0x%" PRIx64 ": %s",
                 vlan_id,
                 sai_metadata_get_status_name(status));
  }

  const switch_object_id_t sw_object_id = {.data = vlan_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove vlan handle %" PRIx64 ": %s",
                  vlan_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set VLAN attribute Value
 *
 * Arguments:
 *    [in] vlan_id - VLAN id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_vlan_entry_attribute(_In_ sai_object_id_t vlan_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(vlan_id) != SAI_OBJECT_TYPE_VLAN) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = vlan_id};
  switch (attr->id) {
    case SAI_VLAN_ATTR_LEARN_DISABLE: {
      bool learning = (attr->value.booldata ? false : true);
      attr_w vlan_attr(SWITCH_VLAN_ATTR_LEARNING, learning);
      switch_status = bf_switch_attribute_set(sw_object_id, vlan_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_VLAN_ATTR_CUSTOM_IGMP_SNOOPING_ENABLE: {
      attr_w igmp_attr(SWITCH_VLAN_ATTR_IGMP_SNOOPING, attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_object_id, igmp_attr);
      status = status_switch_to_sai(switch_status);
      attr_w mld_attr(SWITCH_VLAN_ATTR_MLD_SNOOPING, attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_object_id, mld_attr);
      status |= status_switch_to_sai(switch_status);
      break;
    }
    case SAI_VLAN_ATTR_CUSTOM_0: {
      attr_w arp_sup_attr(SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE,
                          attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_object_id, arp_sup_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    default:
      status =
          sai_to_switch_attribute_set(SAI_OBJECT_TYPE_VLAN, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_VLAN, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get VLAN attribute Value
 *
 * Arguments:
 *    [in] vlan_id - VLAN id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_vlan_entry_attribute(_In_ sai_object_id_t vlan_id,
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

  if (sai_object_type_query(vlan_id) != SAI_OBJECT_TYPE_VLAN) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = vlan_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_VLAN_ATTR_UNKNOWN_NON_IP_MCAST_OUTPUT_GROUP_ID:  // Unsupported
      case SAI_VLAN_ATTR_UNKNOWN_IPV4_MCAST_OUTPUT_GROUP_ID:    // Unsupported
      case SAI_VLAN_ATTR_UNKNOWN_IPV6_MCAST_OUTPUT_GROUP_ID:    // Unsupported
      case SAI_VLAN_ATTR_UNKNOWN_LINKLOCAL_MCAST_OUTPUT_GROUP_ID:  // Unsupported
      case SAI_VLAN_ATTR_UNKNOWN_UNICAST_FLOOD_GROUP:    // Unsupported
      case SAI_VLAN_ATTR_UNKNOWN_MULTICAST_FLOOD_GROUP:  // Unsupported
      case SAI_VLAN_ATTR_BROADCAST_FLOOD_GROUP:          // Unsupported
      case SAI_VLAN_ATTR_TAM_OBJECT:                     // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_VLAN_ATTR_LEARN_DISABLE: {
        smi::attr_w sw_attr(SWITCH_VLAN_ATTR_LEARNING);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_VLAN_ATTR_LEARNING, sw_attr);
        status = status_switch_to_sai(switch_status);

        sw_attr.v_get(attr_list[i].value.booldata);
        attr_list[i].value.booldata =
            (attr_list[i].value.booldata ? false : true);
        break;
      }
      case SAI_VLAN_ATTR_CUSTOM_0: {
        smi::attr_w sw_attr(SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE, sw_attr);
        status = status_switch_to_sai(switch_status);

        sw_attr.v_get(attr_list[i].value.booldata);
        break;
      }
      case SAI_VLAN_ATTR_MEMBER_LIST: {
        const auto &sai_vlan_members_objects =
            switch_store::get_object_references(sw_object_id,
                                                SWITCH_OBJECT_TYPE_VLAN_MEMBER);
        std::vector<sai_object_id_t> sai_vlan_members;
        for (const auto item : sai_vlan_members_objects) {
          sai_vlan_members.push_back(item.oid.data);
        }
        TRY_LIST_SET(attr_list[i].value.objlist, sai_vlan_members);
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_VLAN, sw_object_id, &attr_list[i]);
        break;
    }
    if (attr_list[i].id == SAI_VLAN_ATTR_MEMBER_LIST &&
        status == SAI_STATUS_BUFFER_OVERFLOW) {
      // Not an issue. The caller should re-allocate the list and retry.
      return status;
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get vlan attribute object_id: 0x%" PRIx64
                    "attribute %s "
                    "error: %s",
                    vlan_id,
                    sai_attribute_name(SAI_OBJECT_TYPE_VLAN, attr_list[i].id),
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *    Remove VLAN configuration (remove all VLANs).
 *
 * Arguments:
 *    None
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_all_vlans(void) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Create VLAN Member
    \param[out] vlan_member_id VLAN member ID
    \param[in] switch_id Switch id
    \param[in] attr_count number of attributes
    \param[in] attr_list array of attributes
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_create_vlan_member(_Out_ sai_object_id_t *vlan_member_id,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_VLAN_MEMBER;
  switch_object_id_t member_handle = {0};
  switch_object_id_t switch_vlan_member_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  if (!vlan_member_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vlan_member_id = SAI_NULL_OBJECT_ID;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID: {
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

        if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
          status = sai_get_port_from_bridge_port(attr_list[i].value.oid,
                                                 member_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "failed to get port_handle for bridge port: 0x%" PRIx64 ", %s",
                attr_list[i].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
          status = sai_get_tunnel_from_bridge_port(attr_list[i].value.oid,
                                                   member_handle);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "failed to get tunnel_handle for bridge port: 0x%" PRIx64
                ", %s",
                attr_list[i].value.oid,
                sai_metadata_get_status_name(status));
            return status;
          }
        }

        attr_w member_attr(SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE,
                           member_handle);
        sw_attrs.insert(member_attr);
      } break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_VLAN_MEMBER, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create vlan_member, attribute get failed: %s",
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_VLAN_MEMBER_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_vlan_member_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create vlan_member: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *vlan_member_id = switch_vlan_member_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Remove VLAN Member
    \param[in] vlan_member_id VLAN member ID
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_remove_vlan_member(_In_ sai_object_id_t vlan_member_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(vlan_member_id) != SAI_OBJECT_TYPE_VLAN_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }
  const switch_object_id_t sw_object_id = {.data = vlan_member_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove vlan_member handle %" PRIx64 ": %s",
                  vlan_member_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Set VLAN Member Attribute
    \param[in] vlan_member_id VLAN member ID
    \param[in] attr attribute structure containing ID and value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_set_vlan_member_attribute(_In_ sai_object_id_t vlan_member_id,
                                           _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_object_id_t vlan_member_handle = {.data = vlan_member_id};
  SAI_LOG_ENTER();

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(vlan_member_id) != SAI_OBJECT_TYPE_VLAN_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_VLAN_MEMBER, attr, vlan_member_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set vlan member attribute %s, oid 0x%" PRIx64
                  ": %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER, attr->id),
                  vlan_member_id,
                  sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Get VLAN Member Attribute
    \param[in] vlan_member_id VLAN member ID
    \param[in] attr_count number of attributes
    \param[in,out] attr_list list of attribute structures containing ID and
   value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_get_vlan_member_attribute(_In_ sai_object_id_t vlan_member_id,
                                           _In_ const uint32_t attr_count,
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

  if (sai_object_type_query(vlan_member_id) != SAI_OBJECT_TYPE_VLAN_MEMBER) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_member_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = vlan_member_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID: {
        switch_object_id_t member_handle = {};
        smi::attr_w sw_attr(SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(member_handle);
        switch_object_type_t ot =
            switch_store::object_type_query(member_handle);
        if (ot == SWITCH_OBJECT_TYPE_PORT || ot == SWITCH_OBJECT_TYPE_LAG) {
          status = sai_get_port_to_bridge_port(member_handle,
                                               attr_list[i].value.oid);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get attribute %s from vlan member object_id: "
                "0x%" PRIx64 "mapping from port, error: %s",
                sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER,
                                   attr_list[i].id),
                vlan_member_id,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
          status = sai_get_tunnel_to_bridge_port(member_handle,
                                                 attr_list[i].value.oid);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "Failed to get attribute %s from vlan member object_id: "
                "0x%" PRIx64 "mapping from tunnel, error: %s",
                sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER,
                                   attr_list[i].id),
                vlan_member_id,
                sai_metadata_get_status_name(status));
            return status;
          }
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
          SAI_LOG_ERROR(
              "Failed to get attribute %s from vlan member object_id: "
              "0x%" PRIx64 "due to unsupported member type, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER, attr_list[i].id),
              vlan_member_id,
              sai_metadata_get_status_name(status));
          return status;
        }
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_VLAN_MEMBER, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get vlan member attribute object_id: 0x%" PRIx64
              "attribute "
              "%s "
              "error: %s",
              vlan_member_id,
              sai_attribute_name(SAI_OBJECT_TYPE_VLAN_MEMBER, attr_list[i].id),
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
 *   @brief Clear vlan statistics counters.
 *
 * Arguments:
 *    @param[in] vlan_id - vlan id
 *    @param[in] counter_ids - specifies the array of counter ids
 *    @param[in] number_of_counters - number of counters in the array
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t sai_clear_vlan_stats(_In_ sai_object_id_t vlan_id,
                                  _In_ uint32_t number_of_counters,
                                  _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("Vlan stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(vlan_id) != SAI_OBJECT_TYPE_VLAN) {
    SAI_LOG_ERROR("Vlan stats clear failed: invalid vlan handle 0x%" PRIx64,
                  vlan_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = vlan_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    switch (counter_ids[i]) {
      case SAI_VLAN_STAT_IN_OCTETS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_UCAST_BYTES);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_MCAST_BYTES);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_BCAST_BYTES);
        break;
      case SAI_VLAN_STAT_OUT_OCTETS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_UCAST_BYTES);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_MCAST_BYTES);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_BCAST_BYTES);
        break;
      case SAI_VLAN_STAT_IN_PACKETS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS);
        break;
      case SAI_VLAN_STAT_IN_UCAST_PKTS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS);
        break;
      case SAI_VLAN_STAT_IN_NON_UCAST_PKTS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS);
        break;
      case SAI_VLAN_STAT_OUT_PACKETS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS);
        break;
      case SAI_VLAN_STAT_OUT_UCAST_PKTS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS);
        break;
      case SAI_VLAN_STAT_OUT_NON_UCAST_PKTS:
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS);
        break;
      default:
        break;
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(sw_object_id, cntr_ids);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear vlan 0x%" PRIx64 " stats, status %s",
                    vlan_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

sai_status_t switch_vlan_counters_to_sai_vlan_counters(
    _In_ std::vector<switch_counter_t> &vlan_cntrs,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _Out_ uint64_t *counters) {
  uint32_t index = 0;
  sai_status_t status = SAI_STATUS_SUCCESS;

  std::vector<switch_counter_t> sw_vlan_id_cntrs(SWITCH_VLAN_COUNTER_ID_MAX,
                                                 {0, 0});

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (auto const &cntr : vlan_cntrs) {
    if (cntr.counter_id >= SWITCH_VLAN_COUNTER_ID_MAX) {
      SAI_LOG_ERROR("Unsupported vlan counter id: %d", cntr.counter_id);
      continue;
    }
    sw_vlan_id_cntrs[cntr.counter_id] = cntr;
  }

  for (index = 0; index < number_of_counters; index++) {
    switch (counter_ids[index]) {
      case SAI_VLAN_STAT_IN_OCTETS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_UCAST_BYTES].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_MCAST_BYTES].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_BCAST_BYTES].count;
        break;
      case SAI_VLAN_STAT_IN_UCAST_PKTS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS].count;
        break;
      case SAI_VLAN_STAT_IN_NON_UCAST_PKTS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS].count;
        break;
      case SAI_VLAN_STAT_IN_DISCARDS:        // Unsupported
      case SAI_VLAN_STAT_IN_ERRORS:          // Unsupported
      case SAI_VLAN_STAT_IN_UNKNOWN_PROTOS:  // Unsupported
        counters[index] = 0;
        break;
      case SAI_VLAN_STAT_OUT_OCTETS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_UCAST_BYTES].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_MCAST_BYTES].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_BCAST_BYTES].count;
        break;
      case SAI_VLAN_STAT_OUT_UCAST_PKTS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS].count;
        break;
      case SAI_VLAN_STAT_OUT_NON_UCAST_PKTS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS].count;
        break;
      case SAI_VLAN_STAT_OUT_DISCARDS:  // Unsupported
      case SAI_VLAN_STAT_OUT_ERRORS:    // Unsupported
      case SAI_VLAN_STAT_OUT_QLEN:      // Unsupported
        counters[index] = 0;
        break;
      case SAI_VLAN_STAT_IN_PACKETS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_UCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_MCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_IN_BCAST_PKTS].count;
        break;
      case SAI_VLAN_STAT_OUT_PACKETS:
        counters[index] =
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_UCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_MCAST_PKTS].count +
            sw_vlan_id_cntrs[SWITCH_VLAN_COUNTER_ID_OUT_BCAST_PKTS].count;
        break;
    }
  }
  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get vlan statistics counters.
 *
 * Arguments:
 *    [in] vlan_id - VLAN id
 *    [in] counter_ids - specifies the array of counter ids
 *    [in] number_of_counters - number of counters in the array
 *    [out] counters - array of resulting counter values.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_vlan_stats(_In_ sai_object_id_t vlan_id,
                                _In_ uint32_t number_of_counters,
                                _In_ const sai_stat_id_t *counter_ids,
                                _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> vlan_cntrs;

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(vlan_id) != SAI_OBJECT_TYPE_VLAN) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, vlan_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_vlan_object_id = {.data = vlan_id};
  switch_status = bf_switch_counters_get(sw_vlan_object_id, vlan_cntrs);

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get vlan stats vlan_object_id: %" PRIx64 ": %s",
                  vlan_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_vlan_counters_to_sai_vlan_counters(
      vlan_cntrs, number_of_counters, counter_ids, counters);

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Bulk set attribute on VLAN entry
 *
 * Arguments:
 *
 *    [in] object_count Number of objects to create
 *    [in] object_id List of object to create
 *    [in] attr_count List of attr_count.
 *    [in] attr_list List of attributes for every object.
 *    [in] mode Bulk operation error handling mode.
 *    [out] object_statuses List of status for every object.
 *
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success when all objects are set.
 *    SAI_STATUS_FAILURE when any of the objects fails to set.
 */

sai_status_t sai_create_vlan_members(_In_ sai_object_id_t switch_id,
                                     _In_ uint32_t object_count,
                                     _In_ const uint32_t *attr_count,
                                     _In_ const sai_attribute_t **attr_list,
                                     _In_ sai_bulk_op_error_mode_t mode,
                                     _Out_ sai_object_id_t *object_id,
                                     _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!object_id || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (object_id = %p, attr_count = %p, attr_list = "
        "%p, object_statuses = %p): %s",
        object_id,
        attr_count,
        attr_list,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_create_vlan_member(
        &object_id[it], switch_id, attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create vlan member #%d", it);
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
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Bulk remove VLAN entry
 *
 * Arguments:
 *    [in] object_count Number of objects to remove
 *    [in] object_id List of objects to remove
 *    [in] mode Bulk operation error handling mode.
 *    [out] object_statuses List of status for every object.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success when all objects are removed
 *    SAI_STATUS_FAILURE when any of the objects fails to remove.
 */

sai_status_t sai_remove_vlan_members(_In_ uint32_t object_count,
                                     _In_ const sai_object_id_t *object_id,
                                     _In_ sai_bulk_op_error_mode_t mode,
                                     _Out_ sai_status_t *object_statuses) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  SAI_LOG_ENTER();
  if (!object_id || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "Null argument passed (vlan_entry = %p, object_statuses = %p): %s",
        object_id,
        object_statuses,
        sai_metadata_get_status_name(status));
    return status;
  }
  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_remove_vlan_member(object_id[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove vlan member #%d", it);
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
  return (sai_status_t)status;
}

/*
 * VLAN methods table retrieved with sai_api_query()
 */

sai_vlan_api_t vlan_api = {
  create_vlan : sai_create_vlan_entry,
  remove_vlan : sai_remove_vlan_entry,
  set_vlan_attribute : sai_set_vlan_entry_attribute,
  get_vlan_attribute : sai_get_vlan_entry_attribute,
  create_vlan_member : sai_create_vlan_member,
  remove_vlan_member : sai_remove_vlan_member,
  set_vlan_member_attribute : sai_set_vlan_member_attribute,
  get_vlan_member_attribute : sai_get_vlan_member_attribute,
  create_vlan_members : sai_create_vlan_members,
  remove_vlan_members : sai_remove_vlan_members,
  get_vlan_stats : sai_get_vlan_stats,
  get_vlan_stats_ext : NULL,
  clear_vlan_stats : sai_clear_vlan_stats
};

sai_vlan_api_t *sai_vlan_api_get() { return &vlan_api; }

sai_status_t sai_vlan_initialize() {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_DEBUG("Initializing vlan");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_VLAN);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_VLAN_MEMBER);

  switch_object_id_t device_handle = {0};
  device_handle = sai_get_device_id(0);
  smi::attr_w device_stp_attr(SWITCH_DEVICE_ATTR_DEFAULT_STP);
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_DEFAULT_STP, device_stp_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to fetch switch default stp, error: %s",
                  sai_metadata_get_status_name(status));
  }
  device_stp_attr.v_get(default_stp);

  return status;
}
