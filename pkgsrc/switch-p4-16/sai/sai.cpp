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

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include "s3/switch_store.h"

uint16_t device = 0;

static std::set<sai_object_type_t> bf_sai_supported_objects;
static std::unordered_map<sai_object_type_t, sai_to_sw_obj_metadata_t>
    object_forward_map;
static std::map<switch_object_type_t, sai_object_type_t> object_reverse_map;

void bf_sai_add_object_type_to_supported_list(sai_object_type_t object_type) {
  bf_sai_supported_objects.insert(object_type);
}

const std::set<sai_object_type_t> &bf_sai_get_supported_object_types() {
  return bf_sai_supported_objects;
}

sai_status_t sai_api_query(sai_api_t api_id, void **api_method_table) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!api_method_table) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null api method table: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (api_id) {
    case SAI_API_SWITCH:
      *(const sai_switch_api_t **)api_method_table = sai_switch_api_get();
      return status;
    case SAI_API_HASH:
      *(const sai_hash_api_t **)api_method_table = sai_hash_api_get();
      return status;
    case SAI_API_PORT:
      *(const sai_port_api_t **)api_method_table = sai_port_api_get();
      return status;
    case SAI_API_BRIDGE:
      *(const sai_bridge_api_t **)api_method_table = sai_bridge_api_get();
      return status;
    case SAI_API_LAG:
      *(const sai_lag_api_t **)api_method_table = sai_lag_api_get();
      return status;
    case SAI_API_MY_MAC:
      *(const sai_my_mac_api_t **)api_method_table = sai_my_mac_api_get();
      return status;
    case SAI_API_VLAN:
      *(const sai_vlan_api_t **)api_method_table = sai_vlan_api_get();
      return status;
    case SAI_API_BFD:
      *(const sai_bfd_api_t **)api_method_table = sai_bfd_api_get();
      return status;
    case SAI_API_FDB:
      *(const sai_fdb_api_t **)api_method_table = sai_fdb_api_get();
      return status;
    case SAI_API_ROUTE:
      *(const sai_route_api_t **)api_method_table = sai_route_api_get();
      return status;
    case SAI_API_ROUTER_INTERFACE:
      *(const sai_router_interface_api_t **)api_method_table =
          sai_router_interface_api_get();
      return status;
    case SAI_API_NEXT_HOP:
      *(const sai_next_hop_api_t **)api_method_table = sai_next_hop_api_get();
      return status;
    case SAI_API_NEXT_HOP_GROUP:
      *(const sai_next_hop_group_api_t **)api_method_table =
          sai_next_hop_group_api_get();
      return status;
    case SAI_API_NEIGHBOR:
      *(const sai_neighbor_api_t **)api_method_table = sai_neighbor_api_get();
      return status;
    case SAI_API_HOSTIF:
      *(const sai_hostif_api_t **)api_method_table = sai_hostif_api_get();
      return status;
    case SAI_API_BUFFER:
      *(const sai_buffer_api_t **)api_method_table = sai_buffer_api_get();
      return status;
    case SAI_API_ACL:
      *(const sai_acl_api_t **)api_method_table = sai_acl_api_get();
      return status;
    case SAI_API_QUEUE:
      *(const sai_queue_api_t **)api_method_table = sai_queue_api_get();
      return status;
    case SAI_API_VIRTUAL_ROUTER:
      *(const sai_virtual_router_api_t **)api_method_table =
          sai_virtual_router_api_get();
      return status;
    case SAI_API_POLICER:
      *(const sai_policer_api_t **)api_method_table = sai_policer_api_get();
      return status;
    case SAI_API_STP:
      *(const sai_stp_api_t **)api_method_table = sai_stp_api_get();
      return status;
    case SAI_API_DTEL:
      *(const sai_dtel_api_t **)api_method_table = sai_dtel_api_get();
      return status;
    case SAI_API_TUNNEL:
      *(const sai_tunnel_api_t **)api_method_table = sai_tunnel_api_get();
      return status;
    case SAI_API_SCHEDULER:
      *(const sai_scheduler_api_t **)api_method_table = sai_scheduler_api_get();
      return status;
    case SAI_API_SCHEDULER_GROUP:
      *(const sai_scheduler_group_api_t **)api_method_table =
          sai_scheduler_group_api_get();
      return status;
    case SAI_API_MIRROR:
      *(const sai_mirror_api_t **)api_method_table = sai_mirror_api_get();
      return status;
    case SAI_API_IPMC:
      *(const sai_ipmc_api_t **)api_method_table = sai_ipmc_api_get();
      return status;
    case SAI_API_IPMC_GROUP:
      *(const sai_ipmc_group_api_t **)api_method_table =
          sai_ipmc_group_api_get();
      return status;
    case SAI_API_L2MC:
      *(const sai_l2mc_api_t **)api_method_table = sai_l2mc_api_get();
      return status;
    case SAI_API_L2MC_GROUP:
      *(const sai_l2mc_group_api_t **)api_method_table =
          sai_l2mc_group_api_get();
      return status;
    case SAI_API_QOS_MAP:
      *(const sai_qos_map_api_t **)api_method_table = sai_qos_map_api_get();
      return status;
    case SAI_API_WRED:
      *(const sai_wred_api_t **)api_method_table = sai_wred_api_get();
      return status;
    case SAI_API_SAMPLEPACKET:
      *(const sai_samplepacket_api_t **)api_method_table =
          sai_samplepacket_api_get();
      return status;
    case SAI_API_DEBUG_COUNTER:
      *(const sai_debug_counter_api_t **)api_method_table =
          sai_debug_counter_api_get();
      return status;
    case SAI_API_NAT:
      *(const sai_nat_api_t **)api_method_table = sai_nat_api_get();
      return status;
    case SAI_API_MPLS:
      *(const sai_mpls_api_t **)api_method_table = sai_mpls_api_get();
      return status;
    case SAI_API_UDF:
      *(const sai_udf_api_t **)api_method_table = sai_udf_api_get();
      return status;
#if SAI_API_VERSION >= 10901
    case SAI_API_SRV6:
      *(const sai_srv6_api_t **)api_method_table = sai_srv6_api_get();
      return status;
#endif
    case SAI_API_ISOLATION_GROUP:
      *(const sai_isolation_group_api_t **)api_method_table =
          sai_isolation_group_api_get();
      return status;
    case SAI_API_COUNTER:
      *(const sai_counter_api_t **)api_method_table = sai_counter_api_get();
      return status;
    case SAI_API_RPF_GROUP:
      *(const sai_rpf_group_api_t **)api_method_table = sai_rpf_group_api_get();
      return status;
    default:
      SAI_LOG_INFO("Unsupported api query: %d", api_id);
      return SAI_STATUS_NOT_SUPPORTED;
  }
}

/*
 * Routine Description:
 *     Query sai object type.
 *
 * Arguments:
 *     [in] sai_object_id_t
 *
 * Return Values:
 *    Return SAI_OBJECT_TYPE_NULL when sai_object_id is not valid.
 *    Otherwise, return a valid sai object type SAI_OBJECT_TYPE_XXX
 */
sai_object_type_t sai_object_type_query(sai_object_id_t sai_object_id) {
  switch_object_id_t switch_api_object_id = {.data = sai_object_id};

  sai_object_type_t sai_object_type = SAI_OBJECT_TYPE_NULL;
  const auto switch_object_type =
      switch_store::object_type_query(switch_api_object_id);
  const auto it = object_reverse_map.find(switch_object_type);
  if (it != object_reverse_map.end()) sai_object_type = it->second;

  return sai_object_type;
}

std::string sai_object_name_query(sai_object_id_t sai_object_id) {
  switch_object_id_t switch_api_object_id = {.data = sai_object_id};
  ModelInfo *model_info = switch_store::switch_model_info_get();

  return model_info->get_object_name_from_type(
      switch_store::object_type_query(switch_api_object_id));
}

const sai_attribute_t *sai_get_attr_from_list(sai_attr_id_t attr_id,
                                              const sai_attribute_t *attr_list,
                                              uint32_t attr_count) {
  if (attr_list == NULL || attr_count == 0) {
    return NULL;
  }

  for (unsigned int index = 0; index < attr_count; index++) {
    if (attr_list[index].id == attr_id) {
      return &attr_list[index];
    }
  }

  return NULL;
}

/*
 * Get the correct switch attribute type and set it
 */
template <typename T>
void switch_attribute_set(const switch_object_type_t ot,
                          smi::attr_w &sw_attr,
                          T key) {
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *object_info = model_info->get_object_info(ot);
  auto attr_md = object_info->get_attr_metadata(sw_attr.id_get());

  if (!attr_md) {
    SAI_GENERIC_ERROR("Invalid attr ID:%" PRIu16 "", sw_attr.id_get());
    return;
  }

  switch (attr_md->type) {
    case SWITCH_TYPE_BOOL:
      sw_attr.v_set(static_cast<bool>(key));
      break;
    case SWITCH_TYPE_UINT8:
      sw_attr.v_set(static_cast<uint8_t>(key));
      break;
    case SWITCH_TYPE_UINT16:
      sw_attr.v_set(static_cast<uint16_t>(key));
      break;
    case SWITCH_TYPE_UINT32:
      sw_attr.v_set(static_cast<uint32_t>(key));
      break;
    case SWITCH_TYPE_UINT64:
      sw_attr.v_set(static_cast<uint64_t>(key));
      break;
    case SWITCH_TYPE_INT64:
      sw_attr.v_set(static_cast<int64_t>(key));
      break;
    // Some cases where SAI boolean is an enum in SMI
    case SWITCH_TYPE_ENUM: {
      switch_enum_t e = {.enumdata = static_cast<uint64_t>(key)};
      sw_attr.v_set(e);
      break;
    }
    default:
      break;
  }
}

int32_t sai_md_get_enum_name_value_from_enumsname(
    const sai_enum_metadata_t *enum_metadata,
    const char *enum_value_short_name) {
  if (enum_metadata == NULL) {
    return -1;
  }

  size_t i = 0;
  for (; i < enum_metadata->valuescount; ++i) {
    if (!strcmp(enum_metadata->valuesshortnames[i], enum_value_short_name)) {
      return enum_metadata->values[i];
    }
  }
  return -1;
}

const char *sai_metadata_get_enum_value_shortname(
    const sai_enum_metadata_t *metadata, int value) {
  if (metadata == NULL) {
    return NULL;
  }

  size_t i = 0;
  for (; i < metadata->valuescount; ++i) {
    if (metadata->values[i] == value) {
      return metadata->valuesshortnames[i];
    }
  }

  return NULL;
}

sai_status_t switch_to_sai_map_enum(const sai_attr_metadata_t *sai_attr_md,
                                    const switch_attr_id_t sw_attr_id,
                                    const switch_enum_t &sw_enum,
                                    sai_enum_data_t &sai_enum_value) {
  sai_status_t sai_status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = sai_attr_md->objecttype;
  const sai_attr_id_t sai_attr_id = sai_attr_md->attrid;

  auto obj_itr = object_forward_map.find(sai_ot);
  if (obj_itr == object_forward_map.end()) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum name for SAI: %s"
        ", sw_enum_value: %" PRIu64 "Unsupported SAI object_type: %s.",
        sai_attribute_name(sai_ot, sai_attr_id),
        sw_enum.enumdata,
        sai_metadata_get_object_type_name(sai_ot));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  auto attr_itr = obj_itr->second.obj_attrs_forward_map.find(sai_attr_id);
  if (attr_itr == obj_itr->second.obj_attrs_forward_map.end()) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum name for SAI: %s"
        ", sw_enum_value: %" PRIu64 "Unsupported SAI attribute: %s ",
        sai_attribute_name(sai_ot, sai_attr_id),
        sw_enum.enumdata,
        sai_metadata_get_object_type_name(sai_ot));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  auto &enum_map = attr_itr->second.sai_sw_enum_hashmap;
  auto attr_enum_map_itr = enum_map.reverse_enum_map.find(sw_enum.enumdata);
  if (attr_enum_map_itr != enum_map.reverse_enum_map.end()) {
    sai_enum_value = attr_enum_map_itr->second;
    return SAI_STATUS_SUCCESS;
  }

  /*
   * enum not found in obj_enum hashmap.chk
   * map the enum value using sai enum name.
   */
  const char *sw_enum_name = NULL;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  switch_object_type_t sw_ot = 0;

  sai_status = sai_to_sw_object_type(sai_ot, sw_ot);
  if (sai_status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("Unsupported SAI object type: %d", sai_ot);
    return sai_status;
  }
  const ObjectInfo *object_info = model_info->get_object_info(sw_ot);
  auto attr_md = object_info->get_attr_metadata(sw_attr_id);
  const ValueMetadata *value_md = attr_md->get_value_metadata();
  const std::vector<EnumMetadata> &enums = value_md->get_enum_metadata();
  for (const auto &sw_enum_vals : enums) {
    if (sw_enum_vals.enum_value == sw_enum.enumdata) {
      sw_enum_name = sw_enum_vals.enum_name.c_str();
      break;
    }
  }
  if (!sw_enum_name) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum name for SAI: %s"
        ", sw_enum_value: %" PRIu64 "not supported.",
        sai_attribute_name(sai_ot, sai_attr_id),
        sw_enum.enumdata);
    return SAI_STATUS_NOT_SUPPORTED;
  }

  sai_enum_value = sai_md_get_enum_name_value_from_enumsname(
      sai_attr_md->enummetadata, sw_enum_name);
  if (sai_enum_value == -1) {
    SAI_GENERIC_ERROR(
        "Failed to find sai enum value for , SAI attr: %s,"
        " sw_enum: name: (%s) value: %" PRIu64 "not supported.",
        sai_attribute_name(sai_ot, sai_attr_id),
        sw_enum_name,
        sw_enum.enumdata);
    return SAI_STATUS_NOT_SUPPORTED;
  }

  /*
   * enum mapping exists, update enum maps for next lookup,
   */
  enum_map.forward_enum_map[sai_enum_value] = sw_enum.enumdata;
  enum_map.reverse_enum_map[sw_enum.enumdata] = sai_enum_value;

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_to_switch_map_enum(const sai_attr_metadata_t *sai_attr_md,
                                    const sai_enum_data_t sai_enum_value,
                                    switch_enum_t &sw_enum) {
  sai_status_t sai_status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = sai_attr_md->objecttype;
  const sai_attr_id_t sai_attr_id = sai_attr_md->attrid;

  auto obj_itr = object_forward_map.find(sai_ot);
  if (obj_itr == object_forward_map.end()) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum for SAI: %s"
        ", Unsupported SAI object_type: %s.",
        sai_attribute_name(sai_ot, sai_attr_id),
        sai_metadata_get_object_type_name(sai_ot));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  auto attr_itr = obj_itr->second.obj_attrs_forward_map.find(sai_attr_id);
  if (attr_itr == obj_itr->second.obj_attrs_forward_map.end()) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum for SAI: %s"
        ", Unsupported SAI attribute.",
        sai_attribute_name(sai_ot, sai_attr_id));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  auto &enum_map = attr_itr->second.sai_sw_enum_hashmap;
  auto attr_enum_map_itr = enum_map.forward_enum_map.find(sai_enum_value);
  if (attr_enum_map_itr != enum_map.forward_enum_map.end()) {
    sw_enum.enumdata = attr_enum_map_itr->second;
    return SAI_STATUS_SUCCESS;
  }

  /*
   * enum not found in obj_enum hashmap.chk
   * if same enum name exists in sw and if so map enum value.
   */
  ModelInfo *model_info = switch_store::switch_model_info_get();
  switch_object_type_t sw_ot = 0;
  switch_attr_id_t sw_attr_id = 0;

  sai_status |= sai_to_sw_attr_id(sai_ot, sai_attr_id, sw_attr_id);
  sai_status |= sai_to_sw_object_type(sai_ot, sw_ot);

  if (sai_status != SAI_STATUS_SUCCESS) {
    return SAI_STATUS_NOT_SUPPORTED;
  }
  const char *sai_enum_short_name = sai_metadata_get_enum_value_shortname(
      sai_attr_md->enummetadata, sai_enum_value);
  if (!sai_enum_short_name) {
    return SAI_STATUS_NOT_SUPPORTED;
  }

  const ObjectInfo *object_info = model_info->get_object_info(sw_ot);
  auto attr_md = object_info->get_attr_metadata(sw_attr_id);
  const ValueMetadata *value_md = attr_md->get_value_metadata();
  const std::vector<EnumMetadata> &enums = value_md->get_enum_metadata();
  bool found = false;
  for (const auto &sw_enum_vals : enums) {
    if (sw_enum_vals.enum_name.compare(sai_enum_short_name) == 0) {
      sw_enum.enumdata = sw_enum_vals.enum_value;
      found = true;
      break;
    }
  }
  if (!found) {
    SAI_GENERIC_ERROR(
        "Failed to find switch enum for SAI: %s"
        ", sai_enum: name: (%s), value: (%d), enum mapping does not exists.",
        sai_attribute_name(sai_ot, sai_attr_id),
        sai_enum_short_name,
        sai_enum_value);
    return SAI_STATUS_NOT_SUPPORTED;
  }

  /*
   * enum mapping exists, update enum maps for next lookup,
   */
  enum_map.forward_enum_map[sai_enum_value] = sw_enum.enumdata;
  enum_map.reverse_enum_map[sw_enum.enumdata] = sai_enum_value;
  return SAI_STATUS_SUCCESS;
}

/*
 * Directly converting from sai to switch is not possible
 * ie. bool to bool does not always possible
 * SAI may have bool but the switch may have the same attribute as an enum
 */
sai_status_t sai_to_switch_value(const sai_object_type_t sai_ot,
                                 const sai_attribute_t *sai_attr,
                                 smi::attr_w &sw_attr) {
  sai_status_t status = SWITCH_STATUS_SUCCESS;
  const auto attr_md = sai_metadata_get_attr_metadata(sai_ot, sai_attr->id);
  switch_ip_address_t ip = {};
  switch_ip_prefix_t ip_prefix = {};
  switch_object_type_t ot = 0;
  size_t i = 0;

  if (!attr_md) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_GENERIC_ERROR("Invalid attr ID:%s, %s",
                      sai_attribute_name(sai_ot, sai_attr->id),
                      sai_metadata_get_status_name(status));
    return status;
  }

  status = sai_to_sw_object_type(sai_ot, ot);
  switch (attr_md->attrvaluetype) {
    case SAI_ATTR_VALUE_TYPE_BOOL:
      switch_attribute_set(ot, sw_attr, sai_attr->value.booldata);
      break;
    case SAI_ATTR_VALUE_TYPE_CHARDATA: {
      switch_string_t text;
      memcpy(text.text, sai_attr->value.chardata, SWITCH_MAX_STRING_LEN);
      sw_attr.v_set(text);
    } break;
    case SAI_ATTR_VALUE_TYPE_UINT8:
      sw_attr.v_set(sai_attr->value.u8);
      break;
    case SAI_ATTR_VALUE_TYPE_UINT16:
      sw_attr.v_set(sai_attr->value.u16);
      break;
    case SAI_ATTR_VALUE_TYPE_UINT32:
      sw_attr.v_set(sai_attr->value.u32);
      break;
    case SAI_ATTR_VALUE_TYPE_UINT64:
      sw_attr.v_set(sai_attr->value.u64);
      break;
    case SAI_ATTR_VALUE_TYPE_INT8:
      sw_attr.v_set(static_cast<int64_t>(sai_attr->value.s8));
      break;
    case SAI_ATTR_VALUE_TYPE_INT16:
      sw_attr.v_set(static_cast<int64_t>(sai_attr->value.s16));
      break;
    case SAI_ATTR_VALUE_TYPE_INT32: {
      /* check if it's enum type and do enum mapping  */
      if (attr_md->isenum) {
        switch_enum_t sw_enum = {};
        status = sai_to_switch_map_enum(attr_md, sai_attr->value.s32, sw_enum);
        if (status != SAI_STATUS_SUCCESS) {
          return SAI_STATUS_NOT_SUPPORTED;
        }
        sw_attr.v_set(sw_enum);
      } else {
        sw_attr.v_set(static_cast<int64_t>(sai_attr->value.s32));
      }
    } break;
    case SAI_ATTR_VALUE_TYPE_INT64:
      sw_attr.v_set(sai_attr->value.s64);
      break;
    case SAI_ATTR_VALUE_TYPE_MAC: {
      switch_mac_addr_t mac = {};
      memcpy(&mac.mac, &sai_attr->value.mac, sizeof(mac));
      sw_attr.v_set(mac);
    } break;
    case SAI_ATTR_VALUE_TYPE_IPV4:
      sai_ipv4_to_switch_ip_addr(sai_attr->value.ip4, ip);
      sw_attr.v_set(ip);
      break;
    case SAI_ATTR_VALUE_TYPE_IPV6:
      sai_ipv6_to_switch_ip_addr(sai_attr->value.ip6, ip);
      sw_attr.v_set(ip);
      break;
    case SAI_ATTR_VALUE_TYPE_IP_ADDRESS:
      sai_ip_addr_to_switch_ip_addr(&sai_attr->value.ipaddr, ip);
      sw_attr.v_set(ip);
      break;
    case SAI_ATTR_VALUE_TYPE_IP_PREFIX:
      sai_ip_prefix_to_switch_ip_prefix(&sai_attr->value.ipprefix, ip_prefix);
      sw_attr.v_set(ip_prefix);
      break;
    case SAI_ATTR_VALUE_TYPE_OBJECT_ID: {
      const switch_object_id_t oid = {.data = sai_attr->value.oid};
      sw_attr.v_set(oid);
    } break;
    case SAI_ATTR_VALUE_TYPE_OBJECT_LIST: {
      std::vector<switch_object_id_t> list;
      for (i = 0; i < sai_attr->value.objlist.count; i++) {
        switch_object_id_t oid = {.data = sai_attr->value.objlist.list[i]};
        list.push_back(oid);
      }
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_UINT8_LIST: {
      std::vector<uint8_t> list;
      for (i = 0; i < sai_attr->value.u8list.count; i++)
        list.push_back(sai_attr->value.u8list.list[i]);
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_UINT16_LIST: {
      std::vector<uint16_t> list;
      for (i = 0; i < sai_attr->value.u16list.count; i++)
        list.push_back(sai_attr->value.u16list.list[i]);
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_UINT32_LIST: {
      std::vector<uint32_t> list;
      for (i = 0; i < sai_attr->value.u32list.count; i++)
        list.push_back(sai_attr->value.u32list.list[i]);
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_INT8_LIST: {
      std::vector<uint8_t> list;
      for (i = 0; i < sai_attr->value.s8list.count; i++)
        list.push_back(static_cast<uint8_t>(sai_attr->value.s8list.list[i]));
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_INT16_LIST: {
      std::vector<uint16_t> list;
      for (i = 0; i < sai_attr->value.s16list.count; i++)
        list.push_back(static_cast<uint16_t>(sai_attr->value.s16list.list[i]));
      sw_attr.v_set(list);
      break;
    }
    case SAI_ATTR_VALUE_TYPE_INT32_LIST: {
      if (attr_md->isenumlist) {
        std::vector<switch_enum_t> list(sai_attr->value.s32list.count);
        auto sw_enum = list.begin();
        for (i = 0; i < sai_attr->value.s32list.count; i++) {
          status = sai_to_switch_map_enum(
              attr_md, sai_attr->value.s32list.list[i], *sw_enum++);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_GENERIC_ERROR(
                "Can not map enum id: %u to switch value attr: %s status: %s",
                sai_attr->value.s32list.list[i],
                sai_attribute_name(sai_ot, sai_attr->id),
                sai_metadata_get_status_name(status));
            return status;
          }
        }
        sw_attr.v_set(list);
      } else {
        std::vector<uint32_t> list;
        for (i = 0; i < sai_attr->value.s32list.count; i++)
          list.push_back(
              static_cast<uint32_t>(sai_attr->value.s32list.list[i]));
        sw_attr.v_set(list);
      }
    } break;
    case SAI_ATTR_VALUE_TYPE_IP_ADDRESS_LIST: {
      std::vector<switch_ip_address_t> list;
      for (i = 0; i < sai_attr->value.ipaddrlist.count; i++) {
        switch_ip_address_t _ip;
        sai_ip_addr_to_switch_ip_addr(&sai_attr->value.ipaddrlist.list[i], _ip);
        list.push_back(_ip);
      }
      sw_attr.v_set(list);
    } break;
    case SAI_ATTR_VALUE_TYPE_UINT32_RANGE: {
      switch_range_t range = {sai_attr->value.u32range.min,
                              sai_attr->value.u32range.max};
      sw_attr.v_set(range);
    } break;
    case SAI_ATTR_VALUE_TYPE_SEGMENT_LIST: {
      std::vector<switch_ip_address_t> list;
      for (i = 0; i < sai_attr->value.segmentlist.count; i++) {
        switch_ip_address_t switch_ip = {};
        // char ipstring[SAI_MAX_ENTRY_STRING_LEN];
        // int k = 0;
        // sai_ipv6_to_string(sai_attr->value.segmentlist.list[i],
        //                    SAI_MAX_ENTRY_STRING_LEN,
        //                    ipstring,
        //                    &k);
        sai_ipv6_to_switch_ip_addr(sai_attr->value.segmentlist.list[i],
                                   switch_ip);
        list.push_back(switch_ip);
      }
      sw_attr.v_set(list);
    } break;
    case SAI_ATTR_VALUE_TYPE_POINTER:
    case SAI_ATTR_VALUE_TYPE_INT32_RANGE:
    case SAI_ATTR_VALUE_TYPE_VLAN_LIST:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_BOOL:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT8:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT8:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT16:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT16:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT32:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT32:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_MAC:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV4:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV6:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_OBJECT_ID:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
    case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT8_LIST:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT8:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT8:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT16:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT16:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT32:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT32:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_MAC:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_IPV4:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_IPV6:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_OBJECT_ID:
    case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
    case SAI_ATTR_VALUE_TYPE_QOS_MAP_LIST:
    case SAI_ATTR_VALUE_TYPE_ACL_CAPABILITY:
    case SAI_ATTR_VALUE_TYPE_MAP_LIST:
    default:
      SAI_GENERIC_ERROR("Unimplemented: type: %d", attr_md->attrvaluetype);
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

/* As per SAI specification, if the list size is not large enough,
 * the callee will set the count member to the actual number of object id
 * and return SAI_STATUS_BUFFER_OVERFLOW. Once the caller gets such
 * return code, it should use the returned count member to re-allocate list
 * and retry
 */
sai_status_t switch_to_sai_value_list(const smi::attr_w &sw_attr,
                                      sai_attribute_t *sai_attr,
                                      const sai_attr_metadata_t *sai_attr_md) {
  sai_attr_value_type_t sai_attr_value_type = sai_attr_md->attrvaluetype;
  switch (sw_attr.list_type_get()) {
    case SWITCH_TYPE_UINT8: {
      std::vector<uint8_t> list;
      sw_attr.v_get(list);
      uint32_t count = 0;
      if (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT8) {
        count = sai_attr->value.u8list.count;
        sai_attr->value.u8list.count = list.size();
      } else {
        count = sai_attr->value.s8list.count;
        sai_attr->value.s8list.count = list.size();
      }

      if (count < list.size()) {
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        int i = 0;
        for (const auto item : list) {
          (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT8)
              ? sai_attr->value.u8list.list[i++] = item
              : sai_attr->value.s8list.list[i++] = static_cast<int8_t>(item);
        }
      }
    } break;
    case SWITCH_TYPE_UINT16: {
      std::vector<uint16_t> list;
      sw_attr.v_get(list);
      uint32_t count = 0;

      if (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT16) {
        count = sai_attr->value.u16list.count;
        sai_attr->value.u16list.count = list.size();
      } else {
        count = sai_attr->value.s16list.count;
        sai_attr->value.s16list.count = list.size();
      }

      if (count < list.size()) {
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        int i = 0;
        for (const auto item : list) {
          (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT16)
              ? sai_attr->value.u16list.list[i++] = item
              : sai_attr->value.s16list.list[i++] = static_cast<int16_t>(item);
        }
      }
    } break;
    case SWITCH_TYPE_UINT32: {
      std::vector<uint32_t> list;
      sw_attr.v_get(list);
      uint32_t count = 0;

      if (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT32) {
        count = sai_attr->value.u32list.count;
        sai_attr->value.u32list.count = list.size();
      } else {
        count = sai_attr->value.s32list.count;
        sai_attr->value.s32list.count = list.size();
      }

      if (count < list.size()) {
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        int i = 0;
        for (const auto item : list) {
          (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_UINT32)
              ? sai_attr->value.u32list.list[i++] = item
              : sai_attr->value.s32list.list[i++] = static_cast<int32_t>(item);
        }
      }
    } break;
    case SWITCH_TYPE_OBJECT_ID: {
      std::vector<switch_object_id_t> list;
      sw_attr.v_get(list);
      if (sai_attr->value.objlist.count < list.size()) {
        sai_attr->value.objlist.count = list.size();
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        int i = 0;
        for (const auto item : list)
          sai_attr->value.objlist.list[i++] = item.data;
        sai_attr->value.objlist.count = list.size();
      }
    } break;
    case SWITCH_TYPE_IP_ADDRESS: {
      std::vector<switch_ip_address_t> list;
      sw_attr.v_get(list);
      if (sai_attr->value.ipaddrlist.count < list.size()) {
        sai_attr->value.ipaddrlist.count = list.size();
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        int i = 0;
        for (const auto item : list) {
          if (item.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
            sai_attr->value.ipaddrlist.list[i].addr_family =
                SAI_IP_ADDR_FAMILY_IPV4;
            sai_attr->value.ipaddrlist.list[i].addr.ip4 = htonl(item.ip4);
          } else if (item.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
            if (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_SEGMENT_LIST) {
              memcpy(
                  sai_attr->value.segmentlist.list[i], item.ip6, IPV6_ADDR_LEN);
            } else {
              sai_attr->value.ipaddrlist.list[i].addr_family =
                  SAI_IP_ADDR_FAMILY_IPV6;
              memcpy(sai_attr->value.ipaddrlist.list[i].addr.ip6,
                     item.ip6,
                     IPV6_ADDR_LEN);
            }
          }
          i++;
        }
        if (sai_attr_value_type == SAI_ATTR_VALUE_TYPE_SEGMENT_LIST) {
          sai_attr->value.segmentlist.count = list.size();
        } else {
          sai_attr->value.ipaddrlist.count = list.size();
        }
      }
    } break;
    case SWITCH_TYPE_ENUM: {
      std::vector<switch_enum_t> list;
      sw_attr.v_get(list);
      if (sai_attr->value.s32list.count < list.size()) {
        sai_attr->value.s32list.count = list.size();
        return SAI_STATUS_BUFFER_OVERFLOW;
      } else {
        sai_attr->value.s32list.count = list.size();
        sai_enum_data_t *sai_enum = sai_attr->value.s32list.list;
        for (const auto &sw_enum : list) {
          sai_status_t status = switch_to_sai_map_enum(
              sai_attr_md, sw_attr.id_get(), sw_enum, *sai_enum++);
          if (status != SAI_STATUS_SUCCESS) {
            return status;
          }
        }
      }
    } break;
    default:
      SAI_GENERIC_ERROR("Unimplemented: list type: %d",
                        sw_attr.list_type_get());
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t switch_to_sai_value(const sai_object_type_t sai_ot,
                                 const smi::attr_w &sw_attr,
                                 sai_attribute_t *sai_attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attr_metadata_t *sai_attr_md =
      sai_metadata_get_attr_metadata(sai_ot, sai_attr->id);

  if (!sai_attr_md) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_GENERIC_ERROR("Invalid attr ID:%s, %s",
                      sai_attribute_name(sai_ot, sai_attr->id),
                      sai_metadata_get_status_name(status));
    return status;
  }

  switch (sw_attr.type_get()) {
    case SWITCH_TYPE_BOOL:
      sw_attr.v_get(sai_attr->value.booldata);
      break;
    case SWITCH_TYPE_UINT8: {
      uint8_t u8_val = 0;
      sw_attr.v_get(u8_val);
      (sai_attr_md->attrvaluetype == SAI_ATTR_VALUE_TYPE_UINT8)
          ? sai_attr->value.u8 = u8_val
          : sai_attr->value.s8 = static_cast<int8_t>(u8_val);
      break;
    }
    case SWITCH_TYPE_UINT16: {
      uint16_t u16_val = 0;
      sw_attr.v_get(u16_val);
      (sai_attr_md->attrvaluetype == SAI_ATTR_VALUE_TYPE_UINT16)
          ? sai_attr->value.u16 = u16_val
          : sai_attr->value.s16 = static_cast<int16_t>(u16_val);
      break;
    }
    case SWITCH_TYPE_UINT32: {
      uint32_t u32_val = 0;
      sw_attr.v_get(u32_val);
      (sai_attr_md->attrvaluetype == SAI_ATTR_VALUE_TYPE_UINT32)
          ? sai_attr->value.u32 = u32_val
          : sai_attr->value.s32 = static_cast<int32_t>(u32_val);
      break;
    }
    case SWITCH_TYPE_UINT64: {
      uint64_t u64_val = 0;
      sw_attr.v_get(u64_val);
      (sai_attr_md->attrvaluetype == SAI_ATTR_VALUE_TYPE_UINT64)
          ? sai_attr->value.u64 = u64_val
          : sai_attr->value.s64 = static_cast<int64_t>(u64_val);
      break;
    }
    case SWITCH_TYPE_INT64: {
      int64_t s64_val = 0;
      sw_attr.v_get(s64_val);
      switch (sai_attr_md->attrvaluetype) {
        case SAI_ATTR_VALUE_TYPE_INT8:
          sai_attr->value.s8 = static_cast<int8_t>(s64_val);
          break;
        case SAI_ATTR_VALUE_TYPE_INT16:
          sai_attr->value.s16 = static_cast<int16_t>(s64_val);
          break;
        case SAI_ATTR_VALUE_TYPE_INT32:
          sai_attr->value.s32 = static_cast<int32_t>(s64_val);
          break;
        case SAI_ATTR_VALUE_TYPE_INT64:
          sai_attr->value.s64 = s64_val;
          break;
        default:
          break;
      }
      break;
    }
    case SWITCH_TYPE_MAC: {
      switch_mac_addr_t mac = {};
      sw_attr.v_get(mac);
      memcpy(&sai_attr->value.mac, &mac, sizeof(mac));
      break;
    }
    case SWITCH_TYPE_STRING: {
      switch_string_t text;
      sw_attr.v_get(text);
      sai_strncpy(
          sai_attr->value.chardata,
          text.text,
          std::min(sizeof(sai_attr->value.chardata), sizeof(text.text)));
      break;
    }
    case SWITCH_TYPE_IP_ADDRESS: {
      switch_ip_address_t ip_addr;
      sw_attr.v_get(ip_addr);

      switch (sai_attr_md->attrvaluetype) {
        case SAI_ATTR_VALUE_TYPE_IPV4:
          sai_attr->value.ip4 = htonl(ip_addr.ip4);
          break;
        case SAI_ATTR_VALUE_TYPE_IPV6:
          memcpy(sai_attr->value.ip6, ip_addr.ip6, IPV6_ADDR_LEN);
          break;
        case SAI_ATTR_VALUE_TYPE_IP_ADDRESS:
          if (ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
            sai_attr->value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
            sai_attr->value.ipaddr.addr.ip4 = htonl(ip_addr.ip4);
          } else if (ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
            sai_attr->value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
            memcpy(sai_attr->value.ipaddr.addr.ip6, ip_addr.ip6, IPV6_ADDR_LEN);
          }
          break;
        default:
          SAI_GENERIC_ERROR("Unsupported");
          assert(0);
          break;
      }

      break;
    }
    case SWITCH_TYPE_IP_PREFIX: {
      switch_ip_prefix_t ipprefix;
      sw_attr.v_get(ipprefix);
      if (ipprefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        sai_attr->value.ipprefix.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
        sai_attr->value.ipprefix.addr.ip4 = htonl(ipprefix.addr.ip4);
        sai_attr->value.ipprefix.mask.ip4 = ~((1 << (32 - ipprefix.len)) - 1);
      } else if (ipprefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
        sai_attr->value.ipprefix.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
        memcpy(sai_attr->value.ipprefix.addr.ip6,
               ipprefix.addr.ip6,
               IPV6_ADDR_LEN);
      }
      break;
    }
    case SWITCH_TYPE_OBJECT_ID: {
      switch_object_id_t oid = {};
      sw_attr.v_get(oid);
      sai_attr->value.oid = oid.data;
      break;
    }
    case SWITCH_TYPE_ENUM: {
      switch_enum_t sw_enum = {0};
      sai_enum_data_t sai_enum;
      sw_attr.v_get(sw_enum);

      status = switch_to_sai_map_enum(
          sai_attr_md, sw_attr.id_get(), sw_enum, sai_enum);
      if (status != SAI_STATUS_SUCCESS) {
        return status;
      }
      switch (sai_attr_md->attrvaluetype) {
        case SAI_ATTR_VALUE_TYPE_INT32:
          sai_attr->value.s32 = sai_enum;
          break;
        default:
          SAI_GENERIC_ERROR("Unimplemented");
          assert(0);
          break;
      }
      break;
    }
    case SWITCH_TYPE_LIST:
      status = switch_to_sai_value_list(sw_attr, sai_attr, sai_attr_md);
      break;
    case SWITCH_TYPE_RANGE: {
      switch_range_t range = {};
      sw_attr.v_get(range);
      sai_attr->value.u32range.min = range.min;
      sai_attr->value.u32range.max = range.max;
    } break;
    default:
      SAI_GENERIC_ERROR("Unimplemented: type: %d", sw_attr.type_get());
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return status;
}

/**
 * Given a SAI attribute, do an equivalent attribute_set in SMI
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] sai_attr - Single SAI attribute
 *  [in] oid - SMI object ID
 *  [in] forward_map - SAI to SWITCH object mapping
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and attribute added to list
 *  SAI_STATUS_NOT_SUPPORTED if mapping not found
 *  SAI_STATUS_FAILURE if mapping is found but attribute not added to list
 */
sai_status_t sai_to_switch_attribute_set(const sai_object_type_t ot,
                                         const sai_attribute_t *sai_attr,
                                         const switch_object_id_t oid) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t sw_attr_id = 0;

  // map sai_attr_id to sw_attr_id
  status = sai_to_sw_attr_id(ot, sai_attr->id, sw_attr_id);
  if (status != SAI_STATUS_SUCCESS) {
    return SAI_STATUS_NOT_SUPPORTED;
  }

  if (oid.data == 0) return SAI_STATUS_FAILURE;

  // If entry is found, then do the conversion
  smi::attr_w sw_attr(sw_attr_id);
  status = sai_to_switch_value(ot, sai_attr, sw_attr);
  switch_status = bf_switch_attribute_set(oid, sw_attr);
  return status_switch_to_sai(switch_status);
}

/**
 * Given a list of SAI attributes, this API sweep and  add it to a set of switch
 *attributes.
 * This is useful for a case where there is no SAI parse logic required for a
 *SAI object and
 * need to sweep and create mapped switch attributes for an object_create call.
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] attr_count - SAI attribute count
 *  [in] attr_list - SAI attribute list
 *  [inout] sw_attr_list - Switch attribute list
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and attribute added to list
 *  SAI_STATUS_NOT_SUPPORTED if mapping is found, but fail to match value. this
 *is the case with unsupported enums.
 */
sai_status_t sai_to_switch_attribute_list(const sai_object_type_t ot,
                                          uint32_t attr_count,
                                          const sai_attribute_t *sai_attr_list,
                                          std::set<smi::attr_w> &sw_attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_attr_id_t sw_attr_id = 0;
  uint32_t index = 0;
  const sai_attribute_t *sai_attr;

  for (index = 0; index < attr_count; index++) {
    sai_attr = &sai_attr_list[index];
    // map sai_attr_id to sw_attr_id
    status = sai_to_sw_attr_id(ot, sai_attr->id, sw_attr_id);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR(
          "sai to switch mapping does not exists for SAI attribute: %s,"
          "(may be not supported in this release)",
          sai_attribute_name(ot, sai_attr->id));
      return SAI_STATUS_NOT_SUPPORTED;
    }

    // If entry is found, then do the conversion
    smi::attr_w sw_attr(sw_attr_id);
    status = sai_to_switch_value(ot, sai_attr, sw_attr);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR(
          "sai to switch mapping exists, but attribute fail to match "
          "value for SAI attribute: %s,",
          sai_attribute_name(ot, sai_attr->id));
      return status;
    }
    sw_attr_list.insert(sw_attr);
  }
  return SAI_STATUS_SUCCESS;
}

/**
 * Given a SAI attribute, add it to a set of switch attributes. This is useful
 * for creating list of attributes for an object_create call
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] sai_attr - Single SAI attribute
 *  [inout] sw_attr_list - Switch attribute list
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and attribute added to list
 *  SAI_STATUS_NOT_SUPPORTED if mapping not found
 *  SAI_STATUS_FAILURE if mapping is found but attribute not added to list
 */
sai_status_t sai_to_switch_attribute(const sai_object_type_t ot,
                                     const sai_attribute_t *sai_attr,
                                     std::set<smi::attr_w> &sw_attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_attr_id_t sw_attr_id;

  // map sai_attr_id to sw_attr_id
  status = sai_to_sw_attr_id(ot, sai_attr->id, sw_attr_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR(
        "sai to switch mapping does not exists for SAI attribute: %s,",
        sai_attribute_name(ot, sai_attr->id));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  // If entry is found, then do the conversion
  smi::attr_w sw_attr(sw_attr_id);
  status = sai_to_switch_value(ot, sai_attr, sw_attr);
  sw_attr_list.insert(sw_attr);
  return status;
}

/**
 * Given a SAI object_type, returns switch object_type.
 *
 * Arguments:
 *  [in] sai_ot - SAI object type
 *  [out] sw_ot - SWITCH object type
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping exists
 *  SAI_STATUS_NOT_SUPPORTED if mapping not found
 */

sai_status_t sai_to_sw_object_type(const sai_object_type_t sai_ot,
                                   switch_object_type_t &sw_ot) {
  const auto it = object_forward_map.find(sai_ot);
  if (it == object_forward_map.end()) {
    return SAI_STATUS_NOT_SUPPORTED;
  }
  sw_ot = it->second.sw_ot;
  return (SAI_STATUS_SUCCESS);
}

/**
 * Given a SAI attr_id, returns switch attr_id.
 *
 * Arguments:
 *  [in] sai_ot - SAI object type
 *  [in] sai_attr_id - SAI attribute id
 *  [out] sw_attr_id - SWITCH attribute id
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping exists
 *  SAI_STATUS_NOT_SUPPORTED if object or attribute not supported.
 */

sai_status_t sai_to_sw_attr_id(const sai_object_type_t sai_ot,
                               const sai_attr_id_t sai_attr_id,
                               switch_attr_id_t &sw_attr_id) {
  const auto obj_itr = object_forward_map.find(sai_ot);
  if (obj_itr == object_forward_map.end()) {
    return SAI_STATUS_NOT_SUPPORTED;
  }

  const auto &attrs_forward_map = obj_itr->second.obj_attrs_forward_map;
  const auto attr_itr = attrs_forward_map.find(sai_attr_id);
  if (attr_itr == attrs_forward_map.end()) {
    return SAI_STATUS_NOT_SUPPORTED;
  }
  sw_attr_id = attr_itr->second.sw_attr_id;
  return (SAI_STATUS_SUCCESS);
}

/**
 * Given a SAI attribute, do an equivalent attribute_get in SMI
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] oid - switch object ID
 *  [out] sai_attr - Single SAI attribute
 *  [in] forward_map - SAI to SWITCH object mapping
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and attribute added to list
 *  SAI_STATUS_NOT_SUPPORTED if mapping not found
 *  SAI_STATUS_FAILURE if mapping is found but attribute not added to list
 */
/**
 * Given a list of SAI attributes, this API sweep and  get corresponding switch
 *  attributes.
 * This is useful for a case where there is no per SAI attribute parse logic
 *required for a
 *  SAI object in caller
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] oid - switch object ID
 *  [inout] attr_list - SAI attribute list
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and switch attribute get success
 *  SAI_STATUS_NOT_SUPPORTED if sai to switch mapping is no found . this
 *                           indicates either sai attribute not supported or
 *                           enum not supported .
 */
sai_status_t sai_to_switch_attribute_list_get(const sai_object_type_t ot,
                                              const switch_object_id_t oid,
                                              uint32_t attr_count,
                                              sai_attribute_t *sai_attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t sw_attr_id;
  sai_attribute_t *sai_attr;
  uint32_t index = 0;

  for (index = 0; index < attr_count; index++) {
    sai_attr = &sai_attr_list[index];
    // Check if the attr is in the map
    status = sai_to_sw_attr_id(ot, sai_attr->id, sw_attr_id);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR(
          "Failed to find switch attribute for SAI: %s, error: %s",
          sai_attribute_name(ot, sai_attr->id),
          sai_metadata_get_status_name(status));
      /*
       * may be get not supported not added for this
       * return status as "success" even if we do't support get for this
       * attribute.
       * this is behaviour in old switchsai and matained as it is done for SONIC
       * to keep happy.
       */

      status = SWITCH_STATUS_SUCCESS;
      continue;
    }

    // If entry is found, then do the conversion
    smi::attr_w sw_attr(sw_attr_id);
    switch_status = bf_switch_attribute_get(oid, sw_attr_id, sw_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR(
          "Failed to fetch switch attribute for SAI: %s, error: %s",
          sai_attribute_name(ot, sai_attr->id),
          sai_metadata_get_status_name(status));
      return status;
    }
    status = switch_to_sai_value(ot, sw_attr, sai_attr);
    if (status != SAI_STATUS_SUCCESS) {
      if (status == SAI_STATUS_BUFFER_OVERFLOW) {
        SAI_GENERIC_DEBUG("Attribute: %s, %s",
                          sai_attribute_name(ot, sai_attr->id),
                          sai_metadata_get_status_name(status));
      } else {
        SAI_GENERIC_ERROR(
            "Failed to convert switch attribute for SAI: %s, error: %s",
            sai_attribute_name(ot, sai_attr->id),
            sai_metadata_get_status_name(status));
      }
      return status;
    }
  }
  return status;
}

/**
 * Given a SAI attribute, do an equivalent attribute_get in SMI
 *
 * Arguments:
 *  [in] ot - SAI object type
 *  [in] oid - SMI object ID
 *  [out] sai_attr - Single SAI attribute
 *  [in] forward_map - SAI to SWITCH object mapping
 *
 * Return values:
 *  SAI_STATUS_SUCCESS if mapping is found and attribute added to list
 *  SAI_STATUS_NOT_SUPPORTED if mapping not found
 *  SAI_STATUS_FAILURE if mapping is found but attribute not added to list
 */
sai_status_t sai_to_switch_attribute_get(const sai_object_type_t ot,
                                         const switch_object_id_t oid,
                                         sai_attribute_t *sai_attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t sw_attr_id;

  // Check if the attr is in the map
  status = sai_to_sw_attr_id(ot, sai_attr->id, sw_attr_id);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("Failed to find switch attribute for SAI: %s, error: %s",
                      sai_attribute_name(ot, sai_attr->id),
                      sai_metadata_get_status_name(status));
    return SAI_STATUS_NOT_SUPPORTED;
  }

  // If entry is found, then do the conversion
  smi::attr_w sw_attr(sw_attr_id);
  switch_status = bf_switch_attribute_get(oid, sw_attr_id, sw_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("Failed to fetch switch attribute for SAI: %s, error: %s",
                      sai_attribute_name(ot, sai_attr->id),
                      sai_metadata_get_status_name(status));
    return status;
  }
  status = switch_to_sai_value(ot, sw_attr, sai_attr);
  if (status != SAI_STATUS_SUCCESS) {
    if (status == SAI_STATUS_BUFFER_OVERFLOW) {
      SAI_GENERIC_DEBUG("Attribute: %s, %s",
                        sai_attribute_name(ot, sai_attr->id),
                        sai_metadata_get_status_name(status));
    } else {
      SAI_GENERIC_ERROR(
          "Failed to convert switch attribute for SAI: %s, error: %s",
          sai_attribute_name(ot, sai_attr->id),
          sai_metadata_get_status_name(status));
    }
    return status;
  }
  return status;
}

/* We maintain two maps.
   1. Forward Map: Maps sai object type to [switch object type & sai to switch
   attr mapping]
   2. Reverse Map: Maps switch object type to sai object type
   Both these mappings are generated via
   sai_generate_sai_sw_object_attributes_map()
   during init time by parsing sai/maps/sai.*json files.
   But not all objects have a map file. For example qos maps does not have
   associated
   sai mapping json files. So these mappings are not generated for qos map
   objects..
   This mapping along with other functions is used in sai_object_type_query.
   The below function helps in manuallys adding reverse mapping (switch -> sai
   OT) for objects
   which do not have a generated mapping.
*/
sai_status_t sai_insert_sw_sai_object_type_mapping(switch_object_type_t sw_ot,
                                                   sai_object_type_t sai_ot) {
  object_reverse_map[sw_ot] = sai_ot;
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_generate_sai_sw_object_attributes_map() {
  uint32_t num_sai_objs = sizeof(sai_sw_obj_mapping_md_list) /
                          (sizeof(sai_sw_obj_mapping_md_list[0]));
  sai_object_type_t sai_ot = {};
  switch_object_type_t sw_ot = 0;
  sai_attr_id_t sai_attr_id = 0;
  switch_attr_id_t swi_attr_id = 0;
  uint32_t obj_itr = 0, attr_itr = 0, enum_itr = 0;
  uint32_t num_attrs = 0;
  uint32_t num_enums = 0;
  const sai_to_sw_attr_mapping_md_t *attr_list;
  const sai_to_sw_enum_mapper_t *enum_mapper_list;
  const sai_to_sw_enum_mapper_t *enum_mapper;
  const sai_to_sw_object_mapping_metadata_t *obj_mapping_md_list =
      &sai_sw_obj_mapping_md_list[0];

  for (obj_itr = 0; obj_itr < num_sai_objs; ++obj_itr) {
    sai_ot = obj_mapping_md_list[obj_itr].sai_ot;
    sw_ot = obj_mapping_md_list[obj_itr].sw_ot;
    attr_list = obj_mapping_md_list[obj_itr].sai_sw_attr_mapper_list;
    num_attrs = obj_mapping_md_list[obj_itr].num_attrs;

    /* generate sai_to_sw and sw_to_sai object type hashmap */
    auto &obj_mapping_md = object_forward_map[sai_ot];
    obj_mapping_md.sw_ot = sw_ot;
    object_reverse_map[sw_ot] = sai_ot;
    auto &attrs_forward_map =
        obj_mapping_md.sai_sw_attrs_mapping_md.forward_attrs_hashmap;

    /* generate sai_to_sw and sw_to_sai attribute hashmap */
    for (attr_itr = 0; attr_itr < num_attrs; ++attr_itr) {
      sai_attr_id = attr_list[attr_itr].sai_attr_id;
      swi_attr_id = attr_list[attr_itr].swi_attr_id;
      enum_mapper_list = attr_list[attr_itr].enum_mapper_list;
      num_enums = attr_list[attr_itr].num_enums;

      auto &attr_mapping_md = attrs_forward_map[sai_attr_id];
      attr_mapping_md.sw_attr_id = swi_attr_id;
      auto &enum_map = attr_mapping_md.sai_sw_enum_hashmap;

      /* generate sai_to_sw and sw_to_sai enum hashmap */
      for (enum_itr = 0; enum_itr < num_enums; ++enum_itr) {
        enum_mapper = &enum_mapper_list[enum_itr];
        enum_map.forward_enum_map[enum_mapper->sai_enum] = enum_mapper->sw_enum;
        enum_map.reverse_enum_map[enum_mapper->sw_enum] = enum_mapper->sai_enum;
      }
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_initialize(bool warm_init) {
  unsigned api = 0;

  for (api = SAI_API_UNSPECIFIED; api < SAI_API_MAX; api++) {
    sai_log_set(static_cast<sai_api_t>(api), SAI_LOG_LEVEL_INFO);
  }

  sai_generate_sai_sw_object_attributes_map();
  sai_switch_initialize();
  sai_port_initialize();
  sai_bridge_initialize(warm_init);
  sai_lag_initialize();
  sai_vlan_initialize();
  sai_bfd_initialize();
  sai_fdb_initialize();
  sai_nat_initialize();
  sai_route_initialize();
  sai_router_interface_initialize();
  sai_next_hop_initialize();
  sai_next_hop_group_initialize();
  sai_neighbor_initialize();
  sai_hostif_initialize(warm_init);
  sai_buffer_initialize(warm_init);
  sai_queue_initialize();
  sai_virtual_router_initialize();
  sai_dtel_initialize();
  sai_scheduler_initialize();
  sai_scheduler_group_initialize();
  sai_mirror_initialize();
  sai_qos_map_initialize();
  sai_tunnel_initialize();
  sai_samplepacket_initialize();
  sai_hash_initialize(warm_init);
  sai_mpls_initialize(warm_init);
  sai_acl_initialize(warm_init);
  sai_udf_initialize();
  sai_counter_initialize(warm_init);
  sai_l2mc_initialize();
  sai_l2mc_group_initialize();
  sai_ipmc_initialize();
  sai_ipmc_group_initialize();
  sai_rpf_group_initialize();

  return SAI_STATUS_SUCCESS;
}

inline int sai_to_bf_log_level(sai_log_level_t log_level) {
  switch (log_level) {
    case SAI_LOG_LEVEL_DEBUG:
      return BF_LOG_DBG;
    case SAI_LOG_LEVEL_INFO:
      return BF_LOG_INFO;
    case SAI_LOG_LEVEL_NOTICE:
      return BF_LOG_INFO;
    case SAI_LOG_LEVEL_WARN:
      return BF_LOG_WARN;
    case SAI_LOG_LEVEL_ERROR:
      return BF_LOG_ERR;
    case SAI_LOG_LEVEL_CRITICAL:
      return BF_LOG_CRIT;
    default:
      return BF_LOG_NONE;
  }
  return BF_LOG_NONE;
}

int sai_log_levels[SAI_API_MAX] = {};
bool sai_log_is_enabled(int api_id, int bf_level) {
  return (bf_level <= sai_log_levels[api_id]);
}

sai_status_t sai_log_set(sai_api_t sai_api_id, sai_log_level_t log_level) {
  sai_log_levels[sai_api_id] = sai_to_bf_log_level(log_level);
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_dbg_generate_dump(const char *dump_file_name) {
  (void)dump_file_name;
  return SAI_STATUS_SUCCESS;
}

sai_object_id_t sai_switch_id_query(sai_object_id_t sai_object_id) {
  if (sai_object_id != SAI_NULL_OBJECT_ID) {
    return sai_get_device_id(0).data;
  }
  return SAI_NULL_OBJECT_ID;
}

sai_status_t sai_object_type_get_availability(sai_object_id_t switch_id,
                                              sai_object_type_t object_type,
                                              uint32_t attr_count,
                                              const sai_attribute_t *attr_list,
                                              uint64_t *count) {
  sai_api_t api_id = SAI_API_SWITCH;
  sai_status_t status;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint64_t table_id;
  switch_table_info_t table_info = {};

  *count = 0;

  switch (object_type) {
    case SAI_OBJECT_TYPE_DEBUG_COUNTER:
      status = sai_get_debug_counter_type_availability(
          switch_id, object_type, attr_count, attr_list, count);
      return status;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MAP:
      return SAI_STATUS_SUCCESS;
    case SAI_OBJECT_TYPE_FDB_ENTRY:
      table_id = SWITCH_DEVICE_ATTR_TABLE_DMAC;
      break;
    case SAI_OBJECT_TYPE_ROUTE_ENTRY:
      status = sai_route_entry_get_availability(
          switch_id, attr_count, attr_list, count);
      return status;
    case SAI_OBJECT_TYPE_NEXT_HOP:
      table_id = SWITCH_DEVICE_ATTR_TABLE_NEXTHOP;
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      table_id = SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP;
      break;
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      status = sai_neighbor_entry_get_availability(
          switch_id, attr_count, attr_list, count);
      return status;
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      table_id = SWITCH_DEVICE_ATTR_TABLE_MPLS;
      break;
#if SAI_API_VERSION >= 10901
    case SAI_OBJECT_TYPE_MY_SID_ENTRY:
      table_id = SWITCH_DEVICE_ATTR_TABLE_MY_SID;
      break;
#endif
    default:
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_WARN("Not supported object type: %s",
                   sai_metadata_get_object_type_name(object_type));
      return status;
  }

  switch_status = bf_switch_table_info_get(table_id, table_info);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get table info: %s",
                  sai_metadata_get_status_name(status));
    return SAI_STATUS_FAILURE;
  }
  *count = table_info.table_size - table_info.table_usage;

  return status;
}

sai_status_t sai_generate_attr_capability(_In_ sai_object_type_t object_type) {
  sai_attr_capability_t attr_capability = {0};
  sai_object_capability_map &object_capability_map =
      sai_get_object_capability_map();

  if (!sai_metadata_is_object_type_valid(object_type)) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const sai_attr_metadata_t *const *attr_meta =
      sai_metadata_attr_by_object_type[object_type];

  while (*attr_meta != NULL) {
    attr_capability = {true, false, false};

    if (SAI_HAS_FLAG_CREATE_ONLY((*attr_meta)->flags)) {
      attr_capability.create_implemented = true;
    } else if (SAI_HAS_FLAG_CREATE_AND_SET((*attr_meta)->flags)) {
      attr_capability.create_implemented = true;
      attr_capability.set_implemented = true;
    }

    object_capability_map[object_type][(*attr_meta)->attrid] = attr_capability;
    attr_meta++;
  }

  return SAI_STATUS_SUCCESS;
}
