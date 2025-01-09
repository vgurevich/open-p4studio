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
#include <sainat.h>

#include <memory>
#include <set>
#include <vector>

static sai_api_t api_id = SAI_API_NAT;
static switch_object_id_t device_handle = {0};

sai_status_t sai_switch_nat_type(sai_nat_type_t sai_type,
                                 switch_enum_t &nat_type,
                                 bool l4_sport,
                                 bool l4_dport) {
  switch (sai_type) {
    case SAI_NAT_TYPE_SOURCE_NAT:
      nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT;
      if (l4_sport) {
        nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT;
      }
      break;
    case SAI_NAT_TYPE_DESTINATION_NAT:
      nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT;
      if (l4_dport) {
        nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT;
      }
      break;
    case SAI_NAT_TYPE_DOUBLE_NAT:
      nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT;
      if (l4_sport && l4_dport) {
        nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT;
      }
      break;
    case SAI_NAT_TYPE_DESTINATION_NAT_POOL:
      nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

/*
 * Convert from switch nat_type to sai nat_type
 */
sai_status_t sai_switch_to_sai_nat_type(switch_enum_t nat_type,
                                        sai_nat_type_t &sai_type) {
  switch (nat_type.enumdata) {
    case SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT:
    case SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT:
      sai_type = SAI_NAT_TYPE_SOURCE_NAT;
      break;
    case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT:
    case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT:
      sai_type = SAI_NAT_TYPE_DESTINATION_NAT;
      break;
    case SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT:
    case SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT:
      sai_type = SAI_NAT_TYPE_DOUBLE_NAT;
      break;
    case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL:
      sai_type = SAI_NAT_TYPE_DESTINATION_NAT_POOL;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

void sai_print_nat_entry(bool create, const sai_nat_entry_t *nat_entry) {
  char sip_string[SAI_MAX_ENTRY_STRING_LEN];
  char dip_string[SAI_MAX_ENTRY_STRING_LEN];
  int len = 0;
  sai_ipv4_to_string(
      nat_entry->data.key.src_ip, SAI_MAX_ENTRY_STRING_LEN, sip_string, &len);
  sai_ipv4_to_string(
      nat_entry->data.key.dst_ip, SAI_MAX_ENTRY_STRING_LEN, dip_string, &len);

  SAI_LOG_DEBUG("%s Nat entry:key - vr_id 0x%" PRIx64
                " type %d sip %s, dip %s, proto %d, L4 sport %d, L4 dport %d\n",
                create ? "Create" : "Remove",
                nat_entry->vr_id,
                nat_entry->nat_type,
                sip_string,
                dip_string,
                nat_entry->data.key.proto,
                nat_entry->data.key.l4_src_port,
                nat_entry->data.key.l4_dst_port);
}

sai_status_t sai_nat_entry_parse(const sai_nat_entry_t *nat_entry,
                                 std::set<smi::attr_w> &nat_attrs) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_ip_address_t ip_address;
  switch_enum_t nat_type;
  bool l4_sport = false, l4_dport = false;
  sai_object_type_t obj_type;
  obj_type = sai_object_type_query(nat_entry->vr_id);

  if (obj_type == SAI_OBJECT_TYPE_VIRTUAL_ROUTER) {
    switch_object_id_t vr_handle = {.data = nat_entry->vr_id};
    nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_VR_HANDLE, vr_handle));
  } else {
    SAI_LOG_ERROR("VR object not found for nat_entry");
  }

  sai_ipv4_to_switch_ip_addr(nat_entry->data.key.src_ip, ip_address);
  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY, ip_address));

  sai_ipv4_to_switch_ip_addr(nat_entry->data.mask.src_ip, ip_address);
  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_SRC_IP_MASK, ip_address));

  sai_ipv4_to_switch_ip_addr(nat_entry->data.key.dst_ip, ip_address);
  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY, ip_address));

  sai_ipv4_to_switch_ip_addr(nat_entry->data.mask.dst_ip, ip_address);
  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_DST_IP_MASK, ip_address));

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY,
                               nat_entry->data.key.proto));

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_MASK,
                               nat_entry->data.mask.proto));

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY,
                               nat_entry->data.key.l4_src_port));
  nat_type.enumdata = SWITCH_NAT_ENTRY_ATTR_TYPE_NONE;
  if (nat_entry->data.key.l4_src_port != 0) {
    l4_sport = true;
  }

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_MASK,
                               nat_entry->data.mask.l4_src_port));

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY,
                               nat_entry->data.key.l4_dst_port));
  if (nat_entry->data.key.l4_dst_port != 0) {
    l4_dport = true;
  }

  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_MASK,
                               nat_entry->data.mask.l4_dst_port));

  SAI_LOG_DEBUG("L4sport %d, L4dport %d", l4_sport, l4_dport);
  // Use nat_entry->nat_type after updating SAI refpoint
  status =
      sai_switch_nat_type(nat_entry->nat_type, nat_type, l4_sport, l4_dport);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to parse sai nat type %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  nat_attrs.insert(smi::attr_w(SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type));

  return status;
}

sai_status_t sai_create_nat_entry(_In_ const sai_nat_entry_t *nat_entry,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  std::set<smi::attr_w> nat_attrs;
  switch_object_id_t nat_object_id = {};

  status = sai_nat_entry_parse(nat_entry, nat_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Create NAT entry failed: failed to parse nat entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_print_nat_entry(true, nat_entry);

  for (unsigned int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_NAT_ENTRY_ATTR_SRC_IP:
      case SAI_NAT_ENTRY_ATTR_DST_IP:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NAT_ENTRY, &attr_list[i], nat_attrs);
        break;
      case SAI_NAT_ENTRY_ATTR_L4_SRC_PORT:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NAT_ENTRY, &attr_list[i], nat_attrs);
        break;
      case SAI_NAT_ENTRY_ATTR_L4_DST_PORT:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NAT_ENTRY, &attr_list[i], nat_attrs);
        break;
      case SAI_NAT_ENTRY_ATTR_AGING_TIME:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NAT_ENTRY, &attr_list[i], nat_attrs);
        break;
      default:
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_NAT_ENTRY_ATTR_DEVICE, nat_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create nat entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  return status;
}

sai_status_t sai_remove_nat_entry(_In_ const sai_nat_entry_t *nat_entry) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> nat_attrs;
  switch_object_id_t nat_object_id = {};

  sai_print_nat_entry(false, nat_entry);
  status = sai_nat_entry_parse(nat_entry, nat_attrs);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Remove NAT entry failed: failed to parse nat entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_NAT_ENTRY_ATTR_DEVICE, nat_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove NAT entry: key_eror, object_get failed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (nat_object_id.data == 0) {
    return SAI_STATUS_SUCCESS;
  }
  switch_status = bf_switch_object_delete(nat_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove NAT entry, object_id: %" PRIx64 ": %s",
                  nat_object_id.data,
                  sai_metadata_get_status_name(status));
  }

  return status;
}

sai_status_t sai_set_nat_entry(_In_ const sai_nat_entry_t *nat_entry,
                               _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> nat_attrs;
  switch_object_id_t nat_object_id = {};

  status = sai_nat_entry_parse(nat_entry, nat_attrs);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Set NAT entry failed: failed to parse nat entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_NAT_ENTRY_ATTR_DEVICE, nat_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set NAT entry: key_eror, object_get failed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (nat_object_id.data == 0) {
    return SAI_STATUS_SUCCESS;
  }
  std::vector<uint16_t> cntr_ids;
  switch (attr->id) {
    case SAI_NAT_ENTRY_ATTR_PACKET_COUNT: {
      cntr_ids = {SWITCH_NAT_ENTRY_COUNTER_ID_PKTS};
      switch_status = bf_switch_counters_clear(nat_object_id, cntr_ids);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to clear NAT packet counters for object %" PRIx64,
                      nat_object_id.data);
        return status;
      }
      // After clearing NAT counters, set the cached counter value to 0.
      uint64_t zero_counter = 0;
      attr_w zero_counter_set(SWITCH_NAT_ENTRY_ATTR_CACHED_COUNTER,
                              zero_counter);
      switch_status = bf_switch_attribute_set(nat_object_id, zero_counter_set);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set NAT cachedcounters to zero for object %" PRIx64,
            nat_object_id.data);
        return status;
      }
    } break;
    case SAI_NAT_ENTRY_ATTR_BYTE_COUNT:
      cntr_ids = {SWITCH_NAT_ENTRY_COUNTER_ID_BYTES};
      switch_status = bf_switch_counters_clear(nat_object_id, cntr_ids);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to clear NAT byte counters for object %" PRIx64,
                      nat_object_id.data);
        return status;
      }
      break;
    default:
      break;
  }

  return status;
}

sai_status_t sai_get_nat_entry(_In_ const sai_nat_entry_t *nat_entry,
                               _In_ uint32_t attr_count,
                               _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<smi::attr_w> nat_attrs;
  switch_object_id_t nat_object_id = {};

  status = sai_nat_entry_parse(nat_entry, nat_attrs);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Get NAT entry failed: failed to parse nat entry: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sai_insert_device_attribute(0, SWITCH_NAT_ENTRY_ATTR_DEVICE, nat_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get NAT entry: key_eror, object_get failed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (nat_object_id.data == 0) {
    return SAI_STATUS_SUCCESS;
  }
  std::vector<switch_counter_t> nat_counters;
  for (unsigned int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_NAT_ENTRY_ATTR_PACKET_COUNT:
      case SAI_NAT_ENTRY_ATTR_BYTE_COUNT:
        switch_status = bf_switch_counters_get(nat_object_id, nat_counters);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get NAT counters for object %" PRIx64,
                        nat_object_id.data);
          return status;
        }
        SAI_LOG_DEBUG("Packet count %" PRIu64 ", Byte count %" PRIu64 "\n",
                      nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_PKTS].count,
                      nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_BYTES].count);
        attr_list[i].value.u64 =
            (attr_list[i].id == SAI_NAT_ENTRY_ATTR_PACKET_COUNT)
                ? nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_PKTS].count
                : nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_BYTES].count;
        break;
      case SAI_NAT_ENTRY_ATTR_HIT_BIT_COR:
        break;
      case SAI_NAT_ENTRY_ATTR_HIT_BIT: {
        uint64_t cache_counter = 0;
        smi::attr_w counter_val(SWITCH_NAT_ENTRY_ATTR_CACHED_COUNTER);
        switch_status = bf_switch_attribute_get(
            nat_object_id, SWITCH_NAT_ENTRY_ATTR_CACHED_COUNTER, counter_val);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get NAT cached counters for object %" PRIx64,
                        nat_object_id.data);
          return status;
        }
        counter_val.v_get(cache_counter);
        switch_status = bf_switch_counters_get(nat_object_id, nat_counters);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get NAT counters for object %" PRIx64,
                        nat_object_id.data);
          return status;
        }
        if (nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_PKTS].count >
            cache_counter) {
          attr_list[i].value.booldata = true;
        } else {
          attr_list[i].value.booldata = false;
        }
        SAI_LOG_DEBUG("Stats %" PRIu64 ", CAched counter %" PRIu64
                      ", hit bit %s\n",
                      nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_PKTS].count,
                      cache_counter,
                      attr_list[i].value.booldata == true ? "Yes" : "No");
        smi::attr_w counter_val_set(
            SWITCH_NAT_ENTRY_ATTR_CACHED_COUNTER,
            nat_counters[SWITCH_NAT_ENTRY_COUNTER_ID_PKTS].count);
        switch_status = bf_switch_attribute_set(nat_object_id, counter_val_set);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to set NAT cachedcounters for object %" PRIx64,
                        nat_object_id.data);
          return status;
        }
      } break;
      default:
        break;
    }
  }

  return status;
}

sai_status_t sai_create_nat_entries(_In_ uint32_t object_count,
                                    _In_ const sai_nat_entry_t *nat_entry,
                                    _In_ const uint32_t *attr_count,
                                    _In_ const sai_attribute_t **attr_list,
                                    _In_ sai_bulk_op_error_mode_t mode,
                                    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!nat_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, nat_entry: %p, attr_count: %p, attr_list: "
        "%p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        nat_entry,
        attr_count,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_create_nat_entry(&nat_entry[it], attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create nat entry #%d", it);
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

sai_status_t sai_remove_nat_entries(_In_ uint32_t object_count,
                                    _In_ const sai_nat_entry_t *nat_entry,
                                    _In_ sai_bulk_op_error_mode_t mode,
                                    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!nat_entry || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, nat_entry: %p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        nat_entry,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_remove_nat_entry(&nat_entry[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove nat entry #%d", it);
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

sai_status_t sai_set_nat_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_nat_entry_t *nat_entry,
    _In_ const sai_attribute_t *attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!nat_entry || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, nat_entry: %p, attr_list: %p, "
        "object_statuses: %p",
        sai_metadata_get_status_name(status),
        nat_entry,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] = sai_set_nat_entry(&nat_entry[it], &attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set attribute in nat entry #%d", it);
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

sai_status_t sai_get_nat_entries_attribute(
    _In_ uint32_t object_count,
    _In_ const sai_nat_entry_t *nat_entry,
    _In_ const uint32_t *attr_count,
    _Inout_ sai_attribute_t **attr_list,
    _In_ sai_bulk_op_error_mode_t mode,
    _Out_ sai_status_t *object_statuses) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t it = 0;

  if (!nat_entry || !attr_count || !attr_list || !object_statuses) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR(
        "null argument passed: %s, nat_entry: %p, attr_count: %p, attr_list: "
        "%p, object_statuses: %p",
        sai_metadata_get_status_name(status),
        nat_entry,
        attr_count,
        attr_list,
        object_statuses);
    return status;
  }

  bf_switch_start_batch();
  for (it = 0; it < object_count; it++) {
    object_statuses[it] =
        sai_get_nat_entry(&nat_entry[it], attr_count[it], attr_list[it]);
    if (object_statuses[it] != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to get attribute in nat entry #%d", it);
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

sai_nat_api_t nat_api = {
  create_nat_entry : sai_create_nat_entry,
  remove_nat_entry : sai_remove_nat_entry,
  set_nat_entry_attribute : sai_set_nat_entry,
  get_nat_entry_attribute : sai_get_nat_entry,
  create_nat_entries : sai_create_nat_entries,
  remove_nat_entries : sai_remove_nat_entries,
  set_nat_entries_attribute : sai_set_nat_entries_attribute,
  get_nat_entries_attribute : sai_get_nat_entries_attribute,
};

sai_nat_api_t *sai_nat_api_get() { return &nat_api; }

static sai_nat_event_t switch_nat_event_to_sai_nat_event(
    switch_nat_event_t nat_event) {
  switch (nat_event) {
    case SWITCH_NAT_EVENT_AGED:
      return SAI_NAT_EVENT_AGED;
    default:
      return SAI_NAT_EVENT_NONE;
  }
}

void sai_switch_ip_addr_to_sai_ipv4(sai_ip4_t &ip4,
                                    const switch_ip_address_t &switch_ip_addr) {
  ip4 = htonl(switch_ip_addr.ip4);
}

void sai_nat_notify_cb(const switch_nat_event_data_t &data) {
  SAI_LOG_ENTER();
  uint16_t num_entries = data.payload.size();
  uint16_t entry = 0;

  if (num_entries == 0) {
    SAI_LOG_ERROR("sai nat notify callback with 0 entries");
    return;
  }
  SAI_LOG_DEBUG("Num nat events: %d", num_entries);

  std::unique_ptr<sai_nat_event_notification_data_t[]> nat_event(
      new sai_nat_event_notification_data_t[num_entries]());

  for (const switch_nat_payload_t payload : data.payload) {
    switch_object_id_t nat_handle = payload.nat_handle;
    sai_nat_event_t nat_ev_type =
        switch_nat_event_to_sai_nat_event(payload.nat_event);
    nat_event[entry].event_type = nat_ev_type;

    switch_enum_t nat_type;
    attr_w attr_type(SWITCH_NAT_ENTRY_ATTR_TYPE);
    bf_switch_attribute_get(nat_handle, SWITCH_NAT_ENTRY_ATTR_TYPE, attr_type);
    attr_type.v_get(nat_type);
    sai_switch_to_sai_nat_type(nat_type, nat_event[entry].nat_entry.nat_type);

    switch_ip_address_t ip_address;
    attr_w attr_src_ip_key(SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY, attr_src_ip_key);
    attr_src_ip_key.v_get(ip_address);
    sai_switch_ip_addr_to_sai_ipv4(nat_event[entry].nat_entry.data.key.src_ip,
                                   ip_address);

    attr_w attr_src_ip_mask(SWITCH_NAT_ENTRY_ATTR_SRC_IP_MASK);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_SRC_IP_MASK, attr_src_ip_mask);
    attr_src_ip_mask.v_get(ip_address);
    sai_switch_ip_addr_to_sai_ipv4(nat_event[entry].nat_entry.data.mask.src_ip,
                                   ip_address);

    attr_w attr_dst_ip_key(SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY, attr_dst_ip_key);
    attr_dst_ip_key.v_get(ip_address);
    sai_switch_ip_addr_to_sai_ipv4(nat_event[entry].nat_entry.data.key.dst_ip,
                                   ip_address);

    attr_w attr_dst_ip_mask(SWITCH_NAT_ENTRY_ATTR_DST_IP_MASK);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_DST_IP_MASK, attr_dst_ip_mask);
    attr_dst_ip_mask.v_get(ip_address);
    sai_switch_ip_addr_to_sai_ipv4(nat_event[entry].nat_entry.data.mask.dst_ip,
                                   ip_address);

    attr_w attr_ip_proto_key(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY, attr_ip_proto_key);
    attr_ip_proto_key.v_get(nat_event[entry].nat_entry.data.key.proto);

    attr_w attr_ip_proto_mask(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_MASK);
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_IP_PROTO_MASK, attr_ip_proto_mask);
    attr_ip_proto_mask.v_get(nat_event[entry].nat_entry.data.mask.proto);

    attr_w attr_l4_src_port_key(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY);
    bf_switch_attribute_get(nat_handle,
                            SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY,
                            attr_l4_src_port_key);
    attr_l4_src_port_key.v_get(nat_event[entry].nat_entry.data.key.l4_src_port);

    attr_w attr_l4_src_port_mask(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_MASK);
    bf_switch_attribute_get(nat_handle,
                            SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_MASK,
                            attr_l4_src_port_mask);
    attr_l4_src_port_mask.v_get(
        nat_event[entry].nat_entry.data.mask.l4_src_port);

    attr_w attr_l4_dst_port_key(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY);
    bf_switch_attribute_get(nat_handle,
                            SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY,
                            attr_l4_dst_port_key);
    attr_l4_dst_port_key.v_get(nat_event[entry].nat_entry.data.key.l4_dst_port);

    attr_w attr_l4_dst_port_mask(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_MASK);
    bf_switch_attribute_get(nat_handle,
                            SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_MASK,
                            attr_l4_dst_port_mask);
    attr_l4_dst_port_mask.v_get(
        nat_event[entry].nat_entry.data.mask.l4_dst_port);

    nat_event[entry].nat_entry.switch_id = device_handle.data;

    attr_w attr_vr_hdl(SWITCH_NAT_ENTRY_ATTR_VR_HANDLE);
    switch_object_id_t vr_handle = {0};
    bf_switch_attribute_get(
        nat_handle, SWITCH_NAT_ENTRY_ATTR_VR_HANDLE, attr_vr_hdl);
    attr_vr_hdl.v_get(vr_handle);
    nat_event[entry].nat_entry.vr_id = vr_handle.data;

    // sai_print_nat_entry(false, &nat_event[entry].nat_entry);

    entry++;
  }

  if (sai_switch_notifications.on_nat_event) {
    sai_switch_notifications.on_nat_event(num_entries, nat_event.get());
  }

  SAI_LOG_EXIT();
  return;
}

sai_status_t sai_nat_initialize() {
  SAI_LOG_DEBUG("Initializing nat");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_NAT_ENTRY);
  device_handle = sai_get_device_id(0);

  bf_switch_event_register(SWITCH_NAT_EVENT,
                           (void *)&sai_nat_notify_cb);  // NOLINT

  return SAI_STATUS_SUCCESS;
}
