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

static sai_api_t api_id = SAI_API_TUNNEL;

/*
 *  Routine Description:
 *    Creates tunnel Map
 *
 *  Arguments:
 *    [out] tunnel_map_id - Tunnel Map Id
 *    [in] switch_id - Switch Id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_tunnel_map(_Out_ sai_object_id_t *tunnel_map_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!tunnel_map_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *tunnel_map_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t map_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL_MAP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create tunnel map: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, sw_attrs, map_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create tunnel map: %s",
                  sai_metadata_get_status_name(status));
  }
  *tunnel_map_id = map_object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Removes tunnel Map
 *
 *  Arguments:
 *    [in] tunnel_map_id - Tunnel Map id to be removed
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_tunnel_map(_In_ sai_object_id_t tunnel_map_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(tunnel_map_id) != SAI_OBJECT_TYPE_TUNNEL_MAP) {
    SAI_LOG_ERROR(
        "Tunnel map remove failed: invalid tunnel map handle 0x%" PRIx64 "\n",
        tunnel_map_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_map_id};
  sw_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove tunnel map 0x%" PRIx64 ": %s",
                  tunnel_map_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Set attributes for tunnel map
 *
 *  Arguments:
 *    [in] tunnel_map_id - Tunnel Map Id
 *    [in] attr - Attribute to set
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_tunnel_map_attribute(_In_ sai_object_id_t tunnel_map_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_map_id) != SAI_OBJECT_TYPE_TUNNEL_MAP) {
    SAI_LOG_ERROR("Tunnel map set failed: invalid tunnel map handle 0x%" PRIx64
                  "\n",
                  tunnel_map_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_map_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_TUNNEL_MAP, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_MAP, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Gets attributes of tunnel map
 *
 *  Arguments:
 *    [in] tunnel_map_id - Tunnel map id
 *    [in] attr_count - Number of attributes
 *    [inout] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_tunnel_map_attribute(_In_ sai_object_id_t tunnel_map_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_map_id) != SAI_OBJECT_TYPE_TUNNEL_MAP) {
    SAI_LOG_ERROR("Tunnel map get failed: invalid tunnel map handle 0x%" PRIx64
                  "\n",
                  tunnel_map_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_map_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_TUNNEL_MAP, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_MAP,
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

/*
 *  Routine Description:
 *    Programs a reverse entry for given encap map
 *
 *  Arguments:
 *    decap_mapper_reverse - Reverse decap mapper
 *    encap_mapper_reverse - Reverse encap mapper
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t program_map_entry(switch_object_id_t decap_mapper_reverse,
                               switch_object_id_t encap_mapper_reverse) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  smi::attr_w reverse_map(SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID,
                          decap_mapper_reverse);
  sw_status = bf_switch_attribute_set(encap_mapper_reverse, reverse_map);
  status |= status_switch_to_sai(sw_status);

  const auto &decap_map_entries = switch_store::get_object_references(
      decap_mapper_reverse, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY);
  for (const auto &decap_map_entry : decap_map_entries) {
    switch_object_id_t map_entry_object_id = {};
    auto decap_map_entry_id = decap_map_entry.oid;
    std::set<smi::attr_w> sw_attrs_reverse_entry;

    uint32_t tunnel_vni;
    smi::attr_w tunnel_vni_attr(SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI);
    sw_status =
        bf_switch_attribute_get(decap_map_entry_id,
                                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                                tunnel_vni_attr);
    tunnel_vni_attr.v_get(tunnel_vni);
    sw_attrs_reverse_entry.insert(
        smi::attr_w(SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI, tunnel_vni));

    switch_object_id_t network_handle;
    smi::attr_w network_handle_attr(
        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE);
    sw_status =
        bf_switch_attribute_get(decap_map_entry_id,
                                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
                                network_handle_attr);
    network_handle_attr.v_get(network_handle);
    sw_attrs_reverse_entry.insert(smi::attr_w(
        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle));

    switch_enum_t encap_map_type = {
        .enumdata = static_cast<uint64_t>(
            SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI)};
    sw_attrs_reverse_entry.insert(
        smi::attr_w(SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, encap_map_type));

    sw_attrs_reverse_entry.insert(
        smi::attr_w(SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_MAPPER_HANDLE,
                    encap_mapper_reverse));

    sai_insert_device_attribute(
        0, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_DEVICE, sw_attrs_reverse_entry);

    sw_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                                        sw_attrs_reverse_entry,
                                        map_entry_object_id);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                    sai_metadata_get_status_name(status));
    }
  }
  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Creates tunnel
 *
 *  Arguments:
 *    [out] tunnel_id - Tunnel id
 *    [in] switch_id - Switch Id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_tunnel(_Out_ sai_object_id_t *tunnel_id,
                               _In_ sai_object_id_t switch_id,
                               _In_ uint32_t attr_count,
                               _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_object_id_t encap_mapper_reverse = {}, decap_mapper_reverse = {};
  bool program_reverse_map = false, verify_reverse_map = false;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  switch_object_id_t reverse_mapper_id;
  uint32_t index = 0;

  if (!tunnel_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *tunnel_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t tun_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_TUNNEL_ATTR_ENCAP_MAPPERS: {
        std::vector<switch_object_id_t> list;
        for (uint32_t i = 0; i < attribute->value.objlist.count; i++) {
          switch_object_id_t encap_mapper = {
              .data = attribute->value.objlist.list[i]};
          list.push_back(encap_mapper);

          switch_enum_t map_type;
          smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ATTR_TYPE);
          sw_status = bf_switch_attribute_get(
              encap_mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, map_type_attr);
          map_type_attr.v_get(map_type);
          status |= status_switch_to_sai(sw_status);

          const auto &encap_map_entries = switch_store::get_object_references(
              encap_mapper, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY);

          if (map_type.enumdata ==
                  SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI &&
              encap_map_entries.size() == 0) {
            smi::attr_w reverse_mapper_id_attr(
                SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID);
            sw_status = bf_switch_attribute_get(
                encap_mapper,
                SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID,
                reverse_mapper_id_attr);
            reverse_mapper_id_attr.v_get(reverse_mapper_id);
            status |= status_switch_to_sai(sw_status);

            if (reverse_mapper_id.data == 0) {
              encap_mapper_reverse = encap_mapper;
              program_reverse_map = true;
            } else {
              verify_reverse_map = true;
            }
          }
        }
        sw_attrs.insert(
            smi::attr_w(SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES, list));
        break;
      }
      case SAI_TUNNEL_ATTR_DECAP_MAPPERS: {
        std::vector<switch_object_id_t> list;
        for (uint32_t i = 0; i < attribute->value.objlist.count; i++) {
          switch_object_id_t decap_mapper = {
              .data = attribute->value.objlist.list[i]};
          list.push_back(decap_mapper);

          switch_enum_t map_type;
          smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ATTR_TYPE);
          sw_status = bf_switch_attribute_get(
              decap_mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, map_type_attr);
          map_type_attr.v_get(map_type);
          status |= status_switch_to_sai(sw_status);

          if (map_type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VNI_TO_VLAN_HANDLE) {
            decap_mapper_reverse = decap_mapper;
          }
        }
        sw_attrs.insert(
            smi::attr_w(SWITCH_TUNNEL_ATTR_EGRESS_MAPPER_HANDLES, list));
        break;
      }
      case SAI_TUNNEL_ATTR_ENCAP_DSCP_MODE:
      case SAI_TUNNEL_ATTR_DECAP_DSCP_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          status = sai_to_switch_attribute(
              SAI_OBJECT_TYPE_TUNNEL, attribute, sw_attrs);
        } else {
          if (attribute->value.s32 == SAI_TUNNEL_DSCP_MODE_PIPE_MODEL) {
            SAI_LOG_ERROR(
                "Set %s to pipe is not supported",
                sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attribute->id));
          }
        }
        break;
      case SAI_TUNNEL_ATTR_ENCAP_TTL_MODE:
      case SAI_TUNNEL_ATTR_DECAP_TTL_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          status = sai_to_switch_attribute(
              SAI_OBJECT_TYPE_TUNNEL, attribute, sw_attrs);
        } else {
          if (attribute->value.s32 == SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL) {
            SAI_LOG_ERROR(
                "Set %s to uniform is not supported",
                sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attribute->id));
          }
        }
        break;
      case SAI_TUNNEL_ATTR_DECAP_ECN_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_ECN_RFC_6040)) {
          status = sai_to_switch_attribute(
              SAI_OBJECT_TYPE_TUNNEL, attribute, sw_attrs);
        } else {
          if (attribute->value.s32 !=
              SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER) {
            SAI_LOG_ERROR(
                "Setting %s to %d is not supported, only the "
                "COPY_FROM_OUTER value is supported",
                sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attribute->id),

                attribute->value.s32);
          }
        }
        break;
      case SAI_TUNNEL_ATTR_ENCAP_ECN_MODE:
        if (attribute->value.s32 != SAI_TUNNEL_ENCAP_ECN_MODE_STANDARD) {
          SAI_LOG_ERROR(
              "Setting %s to %d is not supported, only the STANDARD "
              "value is supported",
              sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attribute->id),
              attribute->value.s32);
        }
        break;
      // Just ignore the attributes above
      case SAI_TUNNEL_ATTR_ENCAP_GRE_KEY_VALID:
      case SAI_TUNNEL_ATTR_LOOPBACK_PACKET_ACTION:
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL, attribute, sw_attrs);
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create tunnel: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  if (verify_reverse_map) {
    if (reverse_mapper_id != decap_mapper_reverse) {
      SAI_LOG_DEBUG("Reverse mapper attribute in encap mapper 0x%" PRIx64
                    " does not match the decap mapper 0x%" PRIx64 " ",
                    reverse_mapper_id.data,
                    decap_mapper_reverse.data);
    }
  }

  if (program_reverse_map) {
    program_map_entry(decap_mapper_reverse, encap_mapper_reverse);
  }

  sai_insert_device_attribute(0, SWITCH_TUNNEL_ATTR_DEVICE, sw_attrs);

  attribute = sai_get_attr_from_list(
      SAI_TUNNEL_ATTR_ENCAP_SRC_IP, attr_list, attr_count);
  if (attribute == NULL) {
    // @default 0.0.0.0
    switch_ip_address_t src_ip_addr = {};
    src_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    src_ip_addr.ip4 = 0;
    sw_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_SRC_IP, src_ip_addr));
  }
  attribute =
      sai_get_attr_from_list(SAI_TUNNEL_ATTR_PEER_MODE, attr_list, attr_count);
  // When peer_mode == P2MP, add default value of dst_ip attribute.
  // This is needed so that corresponding P2P tunnels can find this P2MP tunnel,
  // for BMAI programming of tunnel_index in tunnel_nexthop.
  if (attribute == NULL || attribute->value.s32 == SAI_TUNNEL_PEER_MODE_P2MP) {
    // @default 0.0.0.0
    switch_ip_address_t dst_ip_addr = {};
    dst_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dst_ip_addr.ip4 = 0;
    sw_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_DST_IP, dst_ip_addr));
  }

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_TUNNEL, sw_attrs, tun_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create tunnel: %s",
                  sai_metadata_get_status_name(status));
  }
  *tunnel_id = tun_object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Sets reverse mapper id to 0 and flushes its entries
 *
 *  Arguments:
 *    object_id - Object id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t flush_reverse_mapper_entries(switch_object_id_t object_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  /* Setting reverse_mapper_id to 0 */
  switch_object_id_t zeroed_mapper = {.data = 0};
  smi::attr_w zeroed_reversed_mapper_attr(
      SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID, zeroed_mapper);
  sw_status = bf_switch_attribute_set(object_id, zeroed_reversed_mapper_attr);
  status |= status_switch_to_sai(sw_status);

  /* Flushing all entries from this tunnel map */
  const auto &encap_map_entries = switch_store::get_object_references(
      object_id, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY);
  for (const auto &encap_map_entry : encap_map_entries) {
    sw_status |= bf_switch_object_delete(encap_map_entry.oid);
    status |= status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove tunnel map entry: %s",
                    sai_metadata_get_status_name(status));
    }
  }
  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Verifies if tunnel is last one that
 *    connects encap and decap map
 *
 *  Arguments:
 *    decap_mapper - Decap mapper
 *    encap_mapper - Encap mapper
 *
 *  Return Values:
 *    true (1) if it is the last connection
 *    false (0) if it is not the last connection
 */
bool verify_encap_decap_mappers_connection(switch_object_id_t decap_mapper,
                                           switch_object_id_t encap_mapper) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  int encap_decap_connections = 0;

  const auto &tunnels = switch_store::get_object_references(
      decap_mapper, SWITCH_OBJECT_TYPE_TUNNEL);
  for (const auto &tunnel : tunnels) {
    auto tunnel_oid = tunnel.oid;
    switch_object_id_t referenced_encap_mapper = {};
    std::vector<switch_object_id_t> referenced_encap_mappers;
    smi::attr_w referenced_encap_mappers_attr(
        SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES);
    sw_status =
        bf_switch_attribute_get(tunnel_oid,
                                SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES,
                                referenced_encap_mappers_attr);
    referenced_encap_mappers_attr.v_get(referenced_encap_mappers);
    status |= status_switch_to_sai(sw_status);
    for (auto &mapper : referenced_encap_mappers) {
      switch_enum_t map_type;
      smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ATTR_TYPE);
      sw_status = bf_switch_attribute_get(
          mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, map_type_attr);
      map_type_attr.v_get(map_type);
      status |= status_switch_to_sai(sw_status);
      if (map_type.enumdata ==
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
        referenced_encap_mapper = mapper;
        break;
      }
    }

    if (encap_mapper == referenced_encap_mapper) encap_decap_connections++;
  }

  SAI_LOG_EXIT();

  if (encap_decap_connections == 1) return true;

  return false;
}

/*
 *  Routine Description:
 *    Removes tunnel
 *
 *  Arguments:
 *    [in] tunnel_id - Tunnel id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_tunnel(_In_ sai_object_id_t tunnel_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t decap_mapper = {}, encap_mapper = {};

  if (sai_object_type_query(tunnel_id) != SAI_OBJECT_TYPE_TUNNEL) {
    SAI_LOG_ERROR("Tunnel remove failed: invalid tunnel handle 0x%" PRIx64 "\n",
                  tunnel_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_id};
  std::vector<switch_object_id_t> decap_mappers;
  smi::attr_w decap_mappers_attr(SWITCH_TUNNEL_ATTR_EGRESS_MAPPER_HANDLES);
  sw_status = bf_switch_attribute_get(sw_object_id,
                                      SWITCH_TUNNEL_ATTR_EGRESS_MAPPER_HANDLES,
                                      decap_mappers_attr);
  decap_mappers_attr.v_get(decap_mappers);
  status |= status_switch_to_sai(sw_status);
  if (!decap_mappers.empty()) {
    for (auto &mapper : decap_mappers) {
      switch_enum_t map_type;
      smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ATTR_TYPE);
      sw_status = bf_switch_attribute_get(
          mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, map_type_attr);
      map_type_attr.v_get(map_type);
      status |= status_switch_to_sai(sw_status);
      if (map_type.enumdata ==
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE) {
        decap_mapper = mapper;
        break;
      }
    }

    std::vector<switch_object_id_t> encap_mappers;
    smi::attr_w encap_mappers_attr(SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES);
    sw_status =
        bf_switch_attribute_get(sw_object_id,
                                SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES,
                                encap_mappers_attr);
    encap_mappers_attr.v_get(encap_mappers);
    status |= status_switch_to_sai(sw_status);
    if (!encap_mappers.empty()) {
      for (auto &mapper : encap_mappers) {
        switch_enum_t map_type;
        smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ATTR_TYPE);
        sw_status = bf_switch_attribute_get(
            mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, map_type_attr);
        map_type_attr.v_get(map_type);
        status |= status_switch_to_sai(sw_status);
        if (map_type.enumdata ==
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
          encap_mapper = mapper;
          break;
        }
      }

      if (encap_mapper.data != 0) {
        bool last_connection =
            verify_encap_decap_mappers_connection(decap_mapper, encap_mapper);

        switch_object_id_t reverse_mapper_id = {};
        smi::attr_w reverse_mapper_id_attr(
            SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID);
        sw_status =
            bf_switch_attribute_get(encap_mapper,
                                    SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID,
                                    reverse_mapper_id_attr);
        reverse_mapper_id_attr.v_get(reverse_mapper_id);
        status |= status_switch_to_sai(sw_status);

        if (last_connection && reverse_mapper_id.data != 0) {
          /* This is the last tunnel that connects decap and encap map */
          flush_reverse_mapper_entries(encap_mapper);
        }
      }
    }
  }

  sw_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove tunnel 0x%" PRIx64 ": %s",
                  tunnel_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Sets tunnel attribute
 *
 *  Arguments:
 *    [in] tunnel_id - Tunnel id
 *    [in] attr - Attribute
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_tunnel_attribute(_In_ sai_object_id_t tunnel_id,
                                      _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_id) != SAI_OBJECT_TYPE_TUNNEL) {
    SAI_LOG_ERROR("Tunnel set failed: invalid tunnel handle 0x%" PRIx64 "\n",
                  tunnel_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_id};
  switch (attr->id) {
    case SAI_TUNNEL_ATTR_ENCAP_DSCP_MODE:
    case SAI_TUNNEL_ATTR_DECAP_DSCP_MODE:
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status = sai_to_switch_attribute_set(
            SAI_OBJECT_TYPE_TUNNEL, attr, sw_object_id);
      } else {
        if (attr->value.s32 == SAI_TUNNEL_DSCP_MODE_PIPE_MODEL) {
          SAI_LOG_ERROR("Set %s to pipe is not supported",
                        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attr->id));
        }
      }
      break;
    case SAI_TUNNEL_ATTR_ENCAP_TTL_MODE:
    case SAI_TUNNEL_ATTR_DECAP_TTL_MODE:
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status = sai_to_switch_attribute_set(
            SAI_OBJECT_TYPE_TUNNEL, attr, sw_object_id);
      } else {
        if (attr->value.s32 == SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL) {
          SAI_LOG_ERROR("Set %s to uniform is not supported",
                        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attr->id));
        }
      }
      break;
    case SAI_TUNNEL_ATTR_DECAP_ECN_MODE:
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_ECN_RFC_6040)) {
        status = sai_to_switch_attribute_set(
            SAI_OBJECT_TYPE_TUNNEL, attr, sw_object_id);
      } else {
        if (attr->value.s32 != SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER) {
          SAI_LOG_ERROR(
              "Setting %s to %d is not supported, only the "
              "COPY_FROM_OUTER value is supported",
              sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attr->id),
              attr->value.s32);
        }
      }
      break;
    case SAI_TUNNEL_ATTR_LOOPBACK_PACKET_ACTION:
      break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_TUNNEL, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Gets tunnel attributes
 *
 *  Arguments:
 *    [in] tunnel_id - Tunnel id
 *    [in] attr_count - Number of attributes
 *    [inout] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_tunnel_attribute(_In_ sai_object_id_t tunnel_id,
                                      _In_ uint32_t attr_count,
                                      _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_id) != SAI_OBJECT_TYPE_TUNNEL) {
    SAI_LOG_ERROR("Tunnel get failed: invalid tunnel handle 0x%" PRIx64 "\n",
                  tunnel_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_TUNNEL, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL, attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

#if 0
/*
*  Routine Description:
*    Gets tunnel statistics counters.
*
*  Arguments:
*    [in] tunnel_id - Tunnel id
*    [in] number_of_counters - Number of counters in the array
*    [in] counter_ids - Specifies the array of counter ids
*    [out] counters - Array of resulting counter values.
*
*  Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_get_tunnel_stats(_In_ sai_object_id_t tunnel_id,
                                   _In_ uint32_t number_of_counters,
                                   _In_ const sai_tunnel_stat_t *counter_ids,
                                   _Out_ uint64_t *counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  return status;
}

/*
*  Routine Description:
*    Clears tunnel statistics counters.
*
*  Arguments:
*    [in] tunnel_id - Tunnel id
*    [in] number_of_counters - Number of counters in the array
*    [in] counter_ids - Specifies the array of counter ids
*
*  Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_clear_tunnel_stats(
    _In_ sai_object_id_t tunnel_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_tunnel_stat_t *counter_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  return status;
}
#endif

/*
 *  Routine Description:
 *    Creates tunnel termination table entry
 *
 *  Arguments:
 *    [out] tunnel_term_table_entry_id - Tunnel termination table entry id
 *    [in] switch_id - Switch Id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_tunnel_term_table_entry(
    _Out_ sai_object_id_t *tunnel_term_table_entry_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!tunnel_term_table_entry_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *tunnel_term_table_entry_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t term_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create tunnel termination table entry: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_TUNNEL_TERM_ATTR_DEVICE, sw_attrs);

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_TUNNEL_TERM, sw_attrs, term_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create tunnel termination table entry: %s",
                  sai_metadata_get_status_name(status));
  }
  *tunnel_term_table_entry_id = term_object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Removes tunnel termination table entry
 *
 *  Arguments:
 *    [in] tunnel_term_table_entry_id - Tunnel termination table entry id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_tunnel_term_table_entry(
    _In_ sai_object_id_t tunnel_term_table_entry_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(tunnel_term_table_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY) {
    SAI_LOG_ERROR(
        "Tunnel termination table remove failed: invalid handle 0x%" PRIx64
        "\n",
        tunnel_term_table_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_term_table_entry_id};
  sw_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove tunnel termination table entry 0x%" PRIx64
                  ": %s",
                  tunnel_term_table_entry_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Sets tunnel termination table entry attribute
 *
 *  Arguments:
 *    [in] tunnel_term_table_entry_id - Tunnel termination table entry id
 *    [in] attr - Attribute
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_tunnel_term_table_entry_attribute(
    _In_ sai_object_id_t tunnel_term_table_entry_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_term_table_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY) {
    SAI_LOG_ERROR(
        "Tunnel termination table entry set failed: invalid handle 0x%" PRIx64
        "\n",
        tunnel_term_table_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_term_table_entry_id};
  switch (attr->id) {
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID:
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE:
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP:
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP:
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TUNNEL_TYPE:
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_ACTION_TUNNEL_ID: {
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_ERROR("Set operation not permitted for create-only %s attribute",
                    sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
                                       attr->id));
      break;
    }
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY, attr->id),
        sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Gets tunnel termination table entry attributes
 *
 *  Arguments:
 *    [in] tunnel_term_table_entry_id - Tunnel termination table entry id
 *    [in] attr_count - Number of attributes
 *    [inout] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_tunnel_term_table_entry_attribute(
    _In_ sai_object_id_t tunnel_term_table_entry_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_term_table_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY) {
    SAI_LOG_ERROR("Tunnel get failed: invalid tunnel handle 0x%" PRIx64 "\n",
                  tunnel_term_table_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_term_table_entry_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status =
            sai_to_switch_attribute_get(SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
                                        sw_object_id,
                                        &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
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

/*
 *  Routine Description:
 *    Creates tunnel map entry
 *
 *  Arguments:
 *    [out] tunnel_map_entry_id - Tunnel map entry id
 *    [in] switch_id - Switch Id
 *    [in] attr_count - Number of attributes
 *    [in] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_tunnel_map_entry(
    _Out_ sai_object_id_t *tunnel_map_entry_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  bool program_reverse_entry = false;
  bool check_reverse_entry = false;
  uint16_t tunnel_map = 0;
  uint32_t index = 0;

  if (!tunnel_map_entry_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *tunnel_map_entry_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t map_object_id = {};
  std::set<smi::attr_w> sw_attrs_reverse;
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_TUNNEL_MAP_ENTRY_ATTR_VLAN_ID_KEY:
      case SAI_TUNNEL_MAP_ENTRY_ATTR_VLAN_ID_VALUE: {
        sai_object_id_t vlan_oid = SAI_NULL_OBJECT_ID;
        status = sai_get_vlan_oid_by_vlan_id(&vlan_oid, attribute->value.u16);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get vlan OID by ID: %u, %s",
                        attribute->value.u16,
                        sai_metadata_get_status_name(status));
          return status;
        }
        switch_object_id_t network_handle = {.data = vlan_oid};
        sw_attrs.insert(smi::attr_w(
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle));
        if (program_reverse_entry) {
          sw_attrs_reverse.insert(smi::attr_w(
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle));
        }
        break;
      }
      case SAI_TUNNEL_MAP_ENTRY_ATTR_VIRTUAL_ROUTER_ID_KEY:
      case SAI_TUNNEL_MAP_ENTRY_ATTR_VIRTUAL_ROUTER_ID_VALUE: {
        switch_object_id_t network_handle = {.data = attribute->value.oid};
        sw_attrs.insert(smi::attr_w(
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle));
        break;
      }
      case SAI_TUNNEL_MAP_ENTRY_ATTR_TUNNEL_MAP_TYPE:
        if (attribute->value.s32 == SAI_TUNNEL_MAP_TYPE_VNI_TO_VLAN_ID) {
          /* For every VNI_TO_VLAN entry we create a reverse VLAN_TO_VNI entry
             for every encap_map that has a reference to this decap_map. */
          program_reverse_entry = true;

          switch_enum_t encap_map_type = {
              .enumdata = static_cast<uint64_t>(
                  SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI)};
          sw_attrs_reverse.insert(smi::attr_w(
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, encap_map_type));
        } else if (attribute->value.s32 == SAI_TUNNEL_MAP_TYPE_VLAN_ID_TO_VNI) {
          /* When entry is of type VLAN_TO_VNI we check if the map has a
             reference for any decap_map if yes: we remove all entry from the
             map and remove the reference, only after that we create an entry
             if no: we just create an entry. */
          check_reverse_entry = true;
        }
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
      case SAI_TUNNEL_MAP_ENTRY_ATTR_TUNNEL_MAP:
        tunnel_map = attribute->value.u16;
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        if (program_reverse_entry) {
          if (attribute->id != SAI_TUNNEL_MAP_ENTRY_ATTR_TUNNEL_MAP) {
            status = sai_to_switch_attribute(
                SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attribute, sw_attrs_reverse);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                            sai_metadata_get_status_name(status));
              return status;
            }
          }
        }
    }
  }

  if (check_reverse_entry) {
    switch_object_id_t tunnel_map_handle = switch_store::id_to_handle(
        SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, tunnel_map);
    switch_object_id_t reverse_mapper_handle;
    smi::attr_w reverse_mapper_handle_attr(
        SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID);
    sw_status =
        bf_switch_attribute_get(tunnel_map_handle,
                                SWITCH_TUNNEL_MAPPER_ATTR_REVERSE_MAPPER_ID,
                                reverse_mapper_handle_attr);
    reverse_mapper_handle_attr.v_get(reverse_mapper_handle);
    status |= status_switch_to_sai(sw_status);

    if (reverse_mapper_handle.data != 0) {
      flush_reverse_mapper_entries(tunnel_map_handle);
    }
  }

  sai_insert_device_attribute(
      0, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_DEVICE, sw_attrs);

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY, sw_attrs, map_object_id);
  status |= status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                  sai_metadata_get_status_name(status));
  }
  *tunnel_map_entry_id = map_object_id.data;

  if (program_reverse_entry) {
    switch_object_id_t tunnel_map_handle = switch_store::id_to_handle(
        SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, tunnel_map);

    const auto &decap_map_references = switch_store::get_object_references(
        tunnel_map_handle, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER);
    for (const auto &encap_map : decap_map_references) {
      std::set<smi::attr_w> sw_attrs_reverse_entry;
      sw_attrs_reverse_entry = sw_attrs_reverse;

      auto map = encap_map.oid;
      sw_attrs_reverse_entry.insert(smi::attr_w(
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_MAPPER_HANDLE, map));

      sai_insert_device_attribute(
          0, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_DEVICE, sw_attrs_reverse_entry);

      sw_status =
          bf_switch_object_create(SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                                  sw_attrs_reverse_entry,
                                  map_object_id);
      status |= status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to create tunnel map entry: %s",
                      sai_metadata_get_status_name(status));
      }
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Removes reverse entry for given decap entry
 *
 *  Arguments:
 *    decap_map_entry - Decap map entry
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t remove_reverse_encap_entry(switch_object_id_t decap_map_entry) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  switch_object_id_t tunnel_mapper_handle;
  smi::attr_w tunnel_mapper_handle_attr(
      SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_MAPPER_HANDLE);
  sw_status = bf_switch_attribute_get(
      decap_map_entry,
      SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_MAPPER_HANDLE,
      tunnel_mapper_handle_attr);
  tunnel_mapper_handle_attr.v_get(tunnel_mapper_handle);
  status |= status_switch_to_sai(sw_status);

  const auto &encap_maps = switch_store::get_object_references(
      tunnel_mapper_handle, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER);

  for (const auto &encap_map : encap_maps) {
    auto encap_map_id = encap_map.oid;
    const auto &encap_map_entries = switch_store::get_object_references(
        encap_map_id, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY);

    for (const auto &encap_map_entry : encap_map_entries) {
      auto encap_map_entry_id = encap_map_entry.oid;

      switch_object_id_t encap_network_handle;
      smi::attr_w encap_network_handle_attr(
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE);
      sw_status = bf_switch_attribute_get(
          encap_map_entry_id,
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
          encap_network_handle_attr);
      encap_network_handle_attr.v_get(encap_network_handle);
      status |= status_switch_to_sai(sw_status);

      switch_object_id_t decap_network_handle;
      smi::attr_w decap_network_handle_attr(
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE);
      sw_status = bf_switch_attribute_get(
          decap_map_entry,
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
          decap_network_handle_attr);
      decap_network_handle_attr.v_get(decap_network_handle);
      status |= status_switch_to_sai(sw_status);

      uint32_t encap_tunnel_vni;
      smi::attr_w encap_tunnel_vni_attr(
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI);
      sw_status =
          bf_switch_attribute_get(encap_map_entry_id,
                                  SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                                  encap_tunnel_vni_attr);
      encap_tunnel_vni_attr.v_get(encap_tunnel_vni);
      status |= status_switch_to_sai(sw_status);

      uint32_t decap_tunnel_vni;
      smi::attr_w decap_tunnel_vni_attr(
          SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI);
      sw_status =
          bf_switch_attribute_get(decap_map_entry,
                                  SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                                  decap_tunnel_vni_attr);
      decap_tunnel_vni_attr.v_get(decap_tunnel_vni);
      status |= status_switch_to_sai(sw_status);

      if (encap_network_handle == decap_network_handle &&
          encap_tunnel_vni == decap_tunnel_vni) {
        sw_status = bf_switch_object_delete(encap_map_entry_id);
        status |= status_switch_to_sai(sw_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to remove tunnel map entry 0x%" PRIx64 ": %s",
                        encap_map_entry_id.data,
                        sai_metadata_get_status_name(status));
        }
        break;
      }
    }
  }
  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Removes tunnel map entry
 *
 *  Arguments:
 *    [in] tunnel_map_entry_id - Tunnel map entry id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_tunnel_map_entry(
    _In_ sai_object_id_t tunnel_map_entry_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(tunnel_map_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY) {
    SAI_LOG_ERROR(
        "Tunnel map entry remove failed: invalid tunnel map entry handle "
        "0x%" PRIx64 "\n",
        tunnel_map_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t sw_object_id = {.data = tunnel_map_entry_id};
  switch_enum_t map_type;
  smi::attr_w map_type_attr(SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE);
  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, map_type_attr);
  map_type_attr.v_get(map_type);
  status |= status_switch_to_sai(sw_status);

  /* Removing reverse entries */
  if (map_type.enumdata ==
      SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE) {
    remove_reverse_encap_entry(sw_object_id);
  }

  sw_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove tunnel map entry 0x%" PRIx64 ": %s",
                  tunnel_map_entry_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Sets tunnel map entry attribute
 *
 *  Arguments:
 *    [in] tunnel_map_entry_id - Tunnel map entry id
 *    [in] attr - Attribute
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_tunnel_map_entry_attribute(
    _In_ sai_object_id_t tunnel_map_entry_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_map_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY) {
    SAI_LOG_ERROR(
        "Tunnel map entry set failed: invalid tunnel map entry handle "
        "0x%" PRIx64 "\n",
        tunnel_map_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_map_entry_id};
  switch (attr->id) {
    case SAI_TUNNEL_MAP_ENTRY_ATTR_TUNNEL_MAP_TYPE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_TUNNEL_MAP:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_OECN_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_OECN_VALUE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_UECN_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_UECN_VALUE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VLAN_ID_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VLAN_ID_VALUE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VNI_ID_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VNI_ID_VALUE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_BRIDGE_ID_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_BRIDGE_ID_VALUE:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VIRTUAL_ROUTER_ID_KEY:
    case SAI_TUNNEL_MAP_ENTRY_ATTR_VIRTUAL_ROUTER_ID_VALUE: {
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_ERROR(
          "Set operation not permitted for create-only %s attribute",
          sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attr->id));
      break;
    }
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, attr->id),
        sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Routine Description:
 *    Gets tunnel map entry attributes
 *
 *  Arguments:
 *    [in] tunnel_map_entry_id - Tunnel map entry id
 *    [in] attr_count - Number of attributes
 *    [inout] attr_list - Array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_tunnel_map_entry_attribute(
    _In_ sai_object_id_t tunnel_map_entry_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(tunnel_map_entry_id) !=
      SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY) {
    SAI_LOG_ERROR(
        "Tunnel map entry get failed: invalid tunnel map entry handle "
        "0x%" PRIx64 "\n",
        tunnel_map_entry_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = tunnel_map_entry_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY,
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

/*
 *    TUNNEL method table retrieved with sai_api_query()
 */
sai_tunnel_api_t tunnel_api = {
    .create_tunnel_map = sai_create_tunnel_map,
    .remove_tunnel_map = sai_remove_tunnel_map,
    .set_tunnel_map_attribute = sai_set_tunnel_map_attribute,
    .get_tunnel_map_attribute = sai_get_tunnel_map_attribute,
    .create_tunnel = sai_create_tunnel,
    .remove_tunnel = sai_remove_tunnel,
    .set_tunnel_attribute = sai_set_tunnel_attribute,
    .get_tunnel_attribute = sai_get_tunnel_attribute,
    .get_tunnel_stats = NULL,
    .get_tunnel_stats_ext = NULL,
    .clear_tunnel_stats = NULL,
    .create_tunnel_term_table_entry = sai_create_tunnel_term_table_entry,
    .remove_tunnel_term_table_entry = sai_remove_tunnel_term_table_entry,
    .set_tunnel_term_table_entry_attribute =
        sai_set_tunnel_term_table_entry_attribute,
    .get_tunnel_term_table_entry_attribute =
        sai_get_tunnel_term_table_entry_attribute,
    .create_tunnel_map_entry = sai_create_tunnel_map_entry,
    .remove_tunnel_map_entry = sai_remove_tunnel_map_entry,
    .set_tunnel_map_entry_attribute = sai_set_tunnel_map_entry_attribute,
    .get_tunnel_map_entry_attribute = sai_get_tunnel_map_entry_attribute};

sai_tunnel_api_t *sai_tunnel_api_get() { return &tunnel_api; }
sai_status_t sai_tunnel_initialize() {
  SAI_LOG_DEBUG("Initializing tunnel");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_TUNNEL);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_TUNNEL_MAP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_TUNNEL_MAP_ENTRY);
  return SAI_STATUS_SUCCESS;
}
