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

static sai_object_capability_map object_capability;

sai_status_t sai_get_maximum_attribute_count(_In_ sai_object_id_t switch_id,
                                             _In_ sai_object_type_t object_type,
                                             _Inout_ uint32_t *count) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_object_count(_In_ sai_object_id_t switch_id,
                                  _In_ sai_object_type_t object_type,
                                  _Inout_ uint32_t *count) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_object_key(_In_ sai_object_id_t switch_id,
                                _In_ sai_object_type_t object_type,
                                _Inout_ uint32_t *object_count,
                                _Inout_ sai_object_key_t *object_list) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bulk_get_attribute(_In_ sai_object_id_t switch_id,
                                    _In_ sai_object_type_t object_type,
                                    _In_ uint32_t object_count,
                                    _In_ const sai_object_key_t *object_key,
                                    _Inout_ uint32_t *attr_count,
                                    _Inout_ sai_attribute_t **attrs,
                                    _Inout_ sai_status_t *object_statuses) {
  return SAI_STATUS_SUCCESS;
}

sai_object_capability_map &sai_get_object_capability_map() {
  return object_capability;
}

sai_status_t sai_query_attribute_capability(
    _In_ sai_object_id_t switch_id,
    _In_ sai_object_type_t object_type,
    _In_ sai_attr_id_t attr_id,
    _Out_ sai_attr_capability_t *attr_capability) {
  const sai_object_type_info_t *object_type_info =
      sai_metadata_get_object_type_info(object_type);

  if (!attr_capability || !object_type_info ||
      attr_id < object_type_info->attridstart ||
      attr_id >= object_type_info->attridend) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *attr_capability = {false, false, false};
  sai_object_capability_map::iterator capability =
      object_capability.find(object_type);

  if (capability != object_capability.end()) {
    if (capability->second.find(attr_id) != capability->second.end()) {
      *attr_capability = object_capability[object_type][attr_id];
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_query_attribute_enum_values_capability(
    _In_ sai_object_id_t switch_id,
    _In_ sai_object_type_t object_type,
    _In_ sai_attr_id_t attr_id,
    _Inout_ sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (object_type) {
    case SAI_OBJECT_TYPE_DEBUG_COUNTER:
      status = sai_get_debug_counter_enum_capabilities(attr_id,
                                                       enum_values_capability);
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      status = sai_get_next_hop_group_enum_capabilities(attr_id,
                                                        enum_values_capability);
      break;
    case SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP:
      status = sai_get_hostif_user_defined_trap_enum_capabilities(
          attr_id, enum_values_capability);
      break;
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      status = sai_get_neighbor_entry_enum_capabilities(attr_id,
                                                        enum_values_capability);
      break;
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      status = sai_get_router_interface_enum_capabilities(
          attr_id, enum_values_capability);
      break;
    default:
      status = SAI_STATUS_NOT_IMPLEMENTED;
      break;
  }
  return status;
}

sai_status_t sai_query_stats_capability(
    _In_ sai_object_id_t switch_id,
    _In_ sai_object_type_t object_type,
    _Inout_ sai_stat_capability_list_t *stats_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  if (stats_capability == nullptr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_GENERIC_ERROR(
        "Failed to query stats capability: stats_capability is nullptr.");
    return status;
  }
  switch (object_type) {
    case SAI_OBJECT_TYPE_PORT:
      status = query_port_stats_capability(*stats_capability);
      break;
    case SAI_OBJECT_TYPE_QUEUE:
      status = query_queue_stats_capability(*stats_capability);
      break;
    case SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP:
      status = query_ingress_priority_group_stats_capability(*stats_capability);
      break;
    case SAI_OBJECT_TYPE_BUFFER_POOL:
      status = query_buffer_pool_stats_capability(*stats_capability);
      break;
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      status = query_rif_stats_capability(*stats_capability);
      break;
    default:
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_GENERIC_ERROR("Query stats capability for %s is not supported.",
                        sai_metadata_get_object_type_name(object_type));
  }
  return status;
}
