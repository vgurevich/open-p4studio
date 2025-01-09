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

#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>

static sai_api_t api_id = SAI_API_SWITCH;
static switch_object_id_t device_handle = {0};
sai_switch_notification_t sai_switch_notifications;

sai_object_id_t hash_id =
    static_cast<uint64_t>(SWITCH_OBJECT_TYPE_HASH) << 48 | 0x1;

switch_status_t sai_to_switch_hash_algo_attr(int32_t sai_hash_attr,
                                             uint32_t &switch_hash_attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch (sai_hash_attr) {
    case SAI_HASH_ALGORITHM_CRC:
    case SAI_HASH_ALGORITHM_CRC_32LO:  // Unsupported
      switch_hash_attr = SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_CRC_32;
      break;
    case SAI_HASH_ALGORITHM_XOR:
      switch_hash_attr = SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_XOR;
      break;
    case SAI_HASH_ALGORITHM_RANDOM:
      switch_hash_attr = SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_RANDOM;
      break;
    default:
      status = SWITCH_STATUS_FAILURE;
      break;
  }
  return status;
}

int32_t switch_to_sai_hash_algo_attr(uint32_t switch_hash_attr) {
  int32_t sai_hash_attr = 0;
  switch (switch_hash_attr) {
    case SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_CRC_32:
      sai_hash_attr = SAI_HASH_ALGORITHM_CRC;
      break;
    case SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_XOR:
      sai_hash_attr = SAI_HASH_ALGORITHM_XOR;
      break;
    case SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM_RANDOM:
      sai_hash_attr = SAI_HASH_ALGORITHM_RANDOM;
      break;
    default:
      break;
  }
  return sai_hash_attr;
}

switch_attr_id_t sai_to_switch_fdb_action(sai_attr_id_t attr_id) {
  switch (attr_id) {
    case SAI_SWITCH_ATTR_FDB_UNICAST_MISS_PACKET_ACTION:
      return SWITCH_DEVICE_ATTR_UNICAST_MISS_PACKET_ACTION;
    case SAI_SWITCH_ATTR_FDB_MULTICAST_MISS_PACKET_ACTION:
      return SWITCH_DEVICE_ATTR_MULTICAST_MISS_PACKET_ACTION;
    case SAI_SWITCH_ATTR_FDB_BROADCAST_MISS_PACKET_ACTION:
      return SWITCH_DEVICE_ATTR_BROADCAST_MISS_PACKET_ACTION;
  }
  return 0;
}

/**
 * @brief default switch operational state change notification
 * @notes do nothing
 */
static void sai_switch_state_change_dummy(_In_ sai_object_id_t switch_id,
                                          _In_ sai_switch_oper_status_t
                                              switch_oper_status) {
  return;
}

/**
 * @brief default FDB notifications handler
 * @notes do nothing
 */
static void sai_fdb_event_dummy(
    _In_ uint32_t count, _In_ const sai_fdb_event_notification_data_t *data) {
  return;
}

/**
 * @brief default port state change notification
 * @notes do nothing
 */
static void sai_port_state_change_dummy(
    _In_ uint32_t count, _In_ const sai_port_oper_status_notification_t *data) {
  return;
}

/**
 * @brief default Switch shutdown request callback.
 * @notes do nothing
 */
static void sai_switch_shutdown_request_dummy(_In_ sai_object_id_t switch_id) {
  return;
}

/**
 * @brief default Switch BFD state change notification callback.
 * @notes do nothing
 */
static void sai_switch_bfd_session_state_change_dummy(
    _In_ uint32_t count,
    _In_ const sai_bfd_session_state_notification_t *data) {
  return;
}

/*
 * Routine Description:
 *    Set switch attribute value
 *
 * Arguments:
 *   [in] switch_id Switch id
 *    [in] attr - switch attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_switch_attribute(_In_ sai_object_id_t switch_id,
                                      _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint64_t flags = 0;

  (void)flags;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_SWITCH_ATTR_INIT_SWITCH:
      break;
    case SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY:
      sai_switch_notifications.on_fdb_event =
          (sai_fdb_event_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_NAT_EVENT_NOTIFY:
      sai_switch_notifications.on_nat_event =
          (sai_nat_event_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY:
      sai_switch_notifications.on_port_state_change =
          (sai_port_state_change_notification_fn)attr->value.ptr;
      // sai_port_oper_state_update();
      break;
    case SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY:
      sai_switch_notifications.on_packet_event =
          (sai_packet_event_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY:
      sai_switch_notifications.on_switch_state_change =
          (sai_switch_state_change_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_SHUTDOWN_REQUEST_NOTIFY:
      sai_switch_notifications.on_switch_shutdown_request =
          (sai_switch_shutdown_request_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_BFD_SESSION_STATE_CHANGE_NOTIFY:
      sai_switch_notifications.on_bfd_session_state_change =
          (sai_bfd_session_state_change_notification_fn)attr->value.ptr;
      break;
    case SAI_SWITCH_ATTR_FDB_AGING_TIME: {
      uint32_t aging_interval = attr->value.u32 * 1000;
      attr_w age_attr(SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL,
                      aging_interval);
      switch_status = bf_switch_attribute_set(device_handle, age_attr);
      break;
    }
    case SAI_SWITCH_ATTR_QUEUE_PFC_DEADLOCK_NOTIFY:  // Unsupported
      break;
    case SAI_SWITCH_ATTR_FDB_UNICAST_MISS_PACKET_ACTION:
    case SAI_SWITCH_ATTR_FDB_BROADCAST_MISS_PACKET_ACTION:
    case SAI_SWITCH_ATTR_FDB_MULTICAST_MISS_PACKET_ACTION: {
      switch_attr_id_t sw_attr_id = sai_to_switch_fdb_action(attr->id);
      uint8_t action = 1;  // default action is forward
      switch (attr->value.s32) {
        case SAI_PACKET_ACTION_DROP:
          action = 0;
          break;
        case SAI_PACKET_ACTION_FORWARD:
          action = 1;
          break;
        case SAI_PACKET_ACTION_TRAP:
          action = 2;
          break;
        case SAI_PACKET_ACTION_COPY:
          action = 3;
          break;
        default:
          break;
      }
      attr_w ac_attr(sw_attr_id, action);
      switch_status = bf_switch_attribute_set(device_handle, ac_attr);
      break;
    }
    case SAI_SWITCH_ATTR_CUSTOM_5:  // customattr for ecmp v4 symmetric hash
                                    // = SAI_SWITCH_ATTR_END + 5
      /* fall through */
    case SAI_SWITCH_ATTR_CUSTOM_6:  // customattr for ecmp v6 symmetric hash
                                    // = SAI_SWITCH_ATTR_END + 6
      /* fall through */
    case SAI_SWITCH_ATTR_ECMP_DEFAULT_SYMMETRIC_HASH:
      if (attr->id == SAI_SWITCH_ATTR_CUSTOM_5 ||
          attr->id == SAI_SWITCH_ATTR_ECMP_DEFAULT_SYMMETRIC_HASH) {
        switch_object_id_t ecmp_v4_hash_oid;
        attr_w v4_hash(SWITCH_DEVICE_ATTR_ECMP_IPV4_HASH);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_ECMP_IPV4_HASH, v4_hash);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get default ecmp ipv4 hash handle");
          break;
        }
        v4_hash.v_get(ecmp_v4_hash_oid);
        if (ecmp_v4_hash_oid.data == 0) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR("Failed device ecmp ipv4 hash is not configured");
        } else {
          attr_w sym_attr(SWITCH_HASH_ATTR_SYMMETRIC_HASH,
                          attr->value.booldata);
          switch_status = bf_switch_attribute_set(ecmp_v4_hash_oid, sym_attr);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR(
                "Failed to set ECMP v4 symmetric hash attribute, error: %s",
                sai_metadata_get_status_name(status));
            break;
          }
        }
      }
      if (attr->id == SAI_SWITCH_ATTR_CUSTOM_6 ||
          attr->id == SAI_SWITCH_ATTR_ECMP_DEFAULT_SYMMETRIC_HASH) {
        switch_object_id_t ecmp_v6_hash_oid;
        attr_w v6_hash(SWITCH_DEVICE_ATTR_ECMP_IPV6_HASH);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_ECMP_IPV6_HASH, v6_hash);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get default ecmp ipv6 hash handle");
          break;
        }
        v6_hash.v_get(ecmp_v6_hash_oid);
        if (ecmp_v6_hash_oid.data == 0) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR("Failed device ecmp ipv6 hash is not configured");
          break;
        } else {
          attr_w sym_attr(SWITCH_HASH_ATTR_SYMMETRIC_HASH,
                          attr->value.booldata);
          switch_status = bf_switch_attribute_set(ecmp_v6_hash_oid, sym_attr);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR(
                "Failed to set ECMP v6 symmetric hash attribute, error: %s",
                sai_metadata_get_status_name(status));
            break;
          }
        }
      }
      if (attr->id == SAI_SWITCH_ATTR_ECMP_DEFAULT_SYMMETRIC_HASH) {
        attr_w ecmp_sym_hash(SWITCH_DEVICE_ATTR_ECMP_SYMMETRIC_HASH,
                             attr->value.booldata);
        switch_status = bf_switch_attribute_set(device_handle, ecmp_sym_hash);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to set device ecmp symmetric hash");
          break;
        }
      }
      break;
    case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM: {
      uint32_t switch_hash_attr = 0;
      switch_status =
          sai_to_switch_hash_algo_attr(attr->value.s32, switch_hash_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Un Supported LAG default hash algo type: %d",
                      attr->value.s32);
        break;
      }
      switch_object_id_t old_algo_oid{}, algo_oid{};
      attr_w old_algo(SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO);
      switch_status = bf_switch_attribute_get(
          device_handle, SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO, old_algo);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get previously configured Hash Algorithm");
        break;
      }
      old_algo.v_get(old_algo_oid);
      switch_enum_t algorithm = {.enumdata = switch_hash_attr};
      switch_enum_t algo_type = {
          .enumdata = SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED};
      std::set<smi::attr_w> attrs;
      attrs.insert(attr_w(SWITCH_HASH_ALGORITHM_ATTR_TYPE, algo_type));
      attrs.insert(attr_w(SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algorithm));
      switch_status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_HASH_ALGORITHM, attrs, algo_oid);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to create new Hash Algorithm");
        break;
      }
      attr_w algo_attr(SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO, algo_oid);
      switch_status = bf_switch_attribute_set(device_handle, algo_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        status = status_switch_to_sai(switch_status);
        SAI_LOG_ERROR("Failed to set LAG default hash algo, error: %s",
                      sai_metadata_get_status_name(status));
      }
      if (old_algo_oid.data) {
        switch_status = bf_switch_object_delete(old_algo_oid);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to delete previously configured Hash Algorithm "
              "0x%" PRIx64,
              old_algo_oid.data);
          break;
        }
      }
      break;
    }
    case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM: {
      uint32_t switch_hash_attr = 0;
      switch_status =
          sai_to_switch_hash_algo_attr(attr->value.s32, switch_hash_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Un Supported ECMP default hash algo type: %d",
                      attr->value.s32);
        break;
      }
      switch_object_id_t old_algo_oid = {};
      switch_object_id_t algo_oid = {};
      attr_w old_algo(SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO);
      switch_status = bf_switch_attribute_get(
          device_handle, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO, old_algo);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get previously configure Hash Algorithm");
        break;
      }
      old_algo.v_get(old_algo_oid);
      std::set<smi::attr_w> attrs;
      switch_enum_t algorithm = {.enumdata = switch_hash_attr};
      switch_enum_t algo_type = {
          .enumdata = SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED};
      attrs.insert(attr_w(SWITCH_HASH_ALGORITHM_ATTR_TYPE, algo_type));
      attrs.insert(attr_w(SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algorithm));
      switch_status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_HASH_ALGORITHM, attrs, algo_oid);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to create new Hash Algorithm");
        break;
      }
      attr_w algo_attr(SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO, algo_oid);
      switch_status = bf_switch_attribute_set(device_handle, algo_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) {
        status = status_switch_to_sai(switch_status);
        SAI_LOG_ERROR("Failed to set ECMP default hash algo, error: %s",
                      sai_metadata_get_status_name(status));
      }
      if (old_algo_oid.data) {
        switch_status = bf_switch_object_delete(old_algo_oid);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to delete previously configured Hash Algorithm "
              "0x%" PRIx64,
              old_algo_oid.data);
          break;
        }
      }
      break;
    }
    case SAI_SWITCH_ATTR_RESTART_WARM: {
      SAI_LOG_WARN("Requested %s reboot",
                   (attr->value.booldata == true) ? "WARM" : "COLD");
      attr_w warm_attr(SWITCH_DEVICE_ATTR_WARM_SHUT, attr->value.booldata);
      switch_status = bf_switch_attribute_set(device_handle, warm_attr);
      break;
    }
    case SAI_SWITCH_ATTR_PRE_SHUTDOWN:
      if (true == attr->value.booldata) {
        if (NULL != sai_switch_notifications.on_fdb_event)
          sai_switch_notifications.on_fdb_event = sai_fdb_event_dummy;
        if (NULL != sai_switch_notifications.on_port_state_change)
          sai_switch_notifications.on_port_state_change =
              sai_port_state_change_dummy;
        if (NULL != sai_switch_notifications.on_switch_state_change)
          sai_switch_notifications.on_switch_state_change =
              sai_switch_state_change_dummy;
        if (NULL != sai_switch_notifications.on_switch_shutdown_request)
          sai_switch_notifications.on_switch_shutdown_request =
              sai_switch_shutdown_request_dummy;
        if (NULL != sai_switch_notifications.on_bfd_session_state_change) {
          sai_switch_notifications.on_bfd_session_state_change =
              sai_switch_bfd_session_state_change_dummy;
        }
      }
      break;
    case SAI_SWITCH_ATTR_NAT_ENABLE:
      if (!attr->value.booldata !=
          !bf_switch_is_feature_enabled(SWITCH_FEATURE_NAT)) {
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_WARN(
            "Cannot set switch attribute %s to %s on this profile, return "
            "status %s",
            sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
            attr->value.booldata ? "on" : "off",
            sai_metadata_get_status_name(status));
      }
      break;
    // Stubs
    case SAI_SWITCH_ATTR_FAST_API_ENABLE: {  // Unsupported
      SAI_LOG_WARN(
          "Set switch attribute %s is not implemented, return status %s",
          sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
          sai_metadata_get_status_name(status));
    } break;
    case SAI_SWITCH_ATTR_LAG_HASH: {  // In-valid parameter
      status = SAI_STATUS_INVALID_PARAMETER;
      SAI_LOG_ERROR(
          "Set switch attribute %s is a read only attribute, return status %s",
          sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
          sai_metadata_get_status_name(status));
    } break;
    case SAI_SWITCH_ATTR_LAG_HASH_IPV4_IN_IPV4: {  // Unsupported
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_WARN(
          "Set switch attribute %s is not implemented, return status %s",
          sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
          sai_metadata_get_status_name(status));
    } break;
    case SAI_SWITCH_ATTR_ECMP_HASH_IPV4_IN_IPV4: {  // Unsupported
      status = SAI_STATUS_NOT_SUPPORTED;
      SAI_LOG_WARN(
          "Set switch attribute %s is not implemented, return status %s",
          sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
          sai_metadata_get_status_name(status));
    } break;
    case SAI_SWITCH_ATTR_TYPE:
      if (attr->value.s32 != SAI_SWITCH_TYPE_NPU) {
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR(
            "Cannot set switch attribute %s to %d, return "
            "status %s",
            sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
            attr->value.s32,
            sai_metadata_get_status_name(status));
      }
      break;
    case SAI_SWITCH_ATTR_PRE_INGRESS_ACL: {
      switch_object_id_t device_id = {.data = switch_id};
      switch_object_id_t acl_id = {.data = attr->value.oid};
      attr_w acl_id_attr(SWITCH_DEVICE_ATTR_PRE_INGRESS_ACL, acl_id);
      switch_status = bf_switch_attribute_set(device_id, acl_id_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set switch pre ingress acl, error: "
            "%s\n",
            sai_metadata_get_status_name(status));
        return status;
      }
    } break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_SWITCH, attr, device_handle);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set switch attribute %s: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set switch attribute %s: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *     Get switch-api table-id mapping from SAI table ids.
 *
 * Arguments:
 *  [in] sai table-id
 *
 * Return value:
 *     switch-api table-id.
 *
 */
uint64_t sai_get_switchapi_table_id(int sai_table_id) {
  switch (sai_table_id) {
    case SAI_SWITCH_ATTR_FDB_TABLE_SIZE:
    case SAI_SWITCH_ATTR_AVAILABLE_FDB_ENTRY:
      return SWITCH_DEVICE_ATTR_TABLE_DMAC;

    case SAI_SWITCH_ATTR_NUMBER_OF_LAGS:
      return SWITCH_DEVICE_ATTR_TABLE_LAG_GROUP;

    case SAI_SWITCH_ATTR_NUMBER_OF_ECMP_GROUPS:
    case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_ENTRY:
      return SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP;

    case SAI_SWITCH_ATTR_L3_NEIGHBOR_TABLE_SIZE:
      return SWITCH_DEVICE_ATTR_TABLE_NEXTHOP;

    case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEXTHOP_ENTRY:
    case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEXTHOP_ENTRY:
      return SWITCH_DEVICE_ATTR_TABLE_NEXTHOP;

    case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_MEMBER_ENTRY:
      return SWITCH_DEVICE_ATTR_TABLE_ECMP_GROUP_MEMBERS;

#if SAI_API_VERSION >= 10901
    case SAI_SWITCH_ATTR_AVAILABLE_MY_SID_ENTRY:
      return SWITCH_DEVICE_ATTR_TABLE_MY_SID;
#endif

    case SAI_SWITCH_ATTR_MAX_ACL_RANGE_COUNT:
      return SWITCH_DEVICE_ATTR_TABLE_L4_SRC_PORT;

    default:
      return SWITCH_DEVICE_ATTR_TABLE_MAX;
  }
}

#define LPM_SCALE_FACTOR 0.85
switch_status_t sai_get_available_size_from_table_list(
    std::vector<uint64_t> tables, uint32_t &avail_tbl_size) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  avail_tbl_size = 0;
  for (uint64_t i = 0; i < tables.size(); i++) {
    switch_table_info_t table_info = {};
    switch_status = bf_switch_table_info_get(tables[i], table_info);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get table size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    if (tables[i] == SWITCH_DEVICE_ATTR_TABLE_IPV4_LPM ||
        tables[i] == SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM ||
        tables[i] == SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM64 ||
        tables[i] == SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM_TCAM) {
      avail_tbl_size +=
          ((table_info.table_size * LPM_SCALE_FACTOR) - table_info.table_usage);
    } else {
      avail_tbl_size += (table_info.table_size - table_info.table_usage);
    }
  }
  return status;
}

sai_status_t sai_route_entry_get_availability(sai_object_id_t switch_id,
                                              uint32_t attr_count,
                                              const sai_attribute_t *attr_list,
                                              uint64_t *count) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t route_tbl_size = 0;
  bool ipv4_routes_avail = true;
  bool ipv6_routes_avail = true;

  if (count == NULL) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (uint32_t i = 0; i < attr_count && attr_list; i++) {
    switch (attr_list[i].id) {
      case SAI_ROUTE_ENTRY_ATTR_IP_ADDR_FAMILY:
        if (attr_list[i].value.s32 == SAI_IP_ADDR_FAMILY_IPV4) {
          ipv6_routes_avail = false;
        } else {
          ipv4_routes_avail = false;
        }
        break;
      default:
        break;
    }
  }

  *count = 0;
  if (ipv4_routes_avail) {
    std::vector<uint64_t> tables = {SWITCH_DEVICE_ATTR_TABLE_IPV4_HOST,
                                    SWITCH_DEVICE_ATTR_TABLE_IPV4_LPM};
    status = sai_get_available_size_from_table_list(tables, route_tbl_size);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get ipv4 route table avail size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    *count += route_tbl_size;
  }
  if (ipv6_routes_avail) {
    std::vector<uint64_t> tables = {SWITCH_DEVICE_ATTR_TABLE_IPV6_HOST,
                                    SWITCH_DEVICE_ATTR_TABLE_IPV6_HOST64,
                                    SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM,
                                    SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM64,
                                    SWITCH_DEVICE_ATTR_TABLE_IPV6_LPM_TCAM};
    status = sai_get_available_size_from_table_list(tables, route_tbl_size);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get ipv6 route table avail size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    *count += route_tbl_size;
  }
  return status;
}

sai_status_t sai_neighbor_entry_get_availability(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t *attr_list,
    uint64_t *count) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bool ipv4_nbr_avail = true;
  bool ipv6_nbr_avail = true;
  uint32_t nbrs = 0;
  uint32_t ipv4_hosts = 0;
  uint32_t ipv6_hosts = 0;
  switch_table_info_t table_info = {};

  if (count == NULL) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  for (uint32_t i = 0; i < attr_count && attr_list; i++) {
    switch (attr_list[i].id) {
      case SAI_NEIGHBOR_ENTRY_ATTR_IP_ADDR_FAMILY:
        if (attr_list[i].value.s32 == SAI_IP_ADDR_FAMILY_IPV4) {
          ipv6_nbr_avail = false;
        } else {
          ipv4_nbr_avail = false;
        }
        break;
      default:
        break;
    }
  }

  *count = 0;

  // Neighbor entries gets programmed in both host and nhop tables.
  // Return the smaller of available numbers from these two tables.

  if (ipv4_nbr_avail) {
    switch_status = bf_switch_table_info_get(SWITCH_DEVICE_ATTR_TABLE_IPV4_HOST,
                                             table_info);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get table size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    ipv4_hosts = table_info.table_size - table_info.table_usage;
  }
  if (ipv6_nbr_avail) {
    // TODO(aagrawa5) : Account for Host64 entries
    switch_status = bf_switch_table_info_get(SWITCH_DEVICE_ATTR_TABLE_IPV6_HOST,
                                             table_info);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get table size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    ipv6_hosts = table_info.table_size - table_info.table_usage;
  }
  if (ipv4_nbr_avail || ipv6_nbr_avail) {
    switch_status =
        bf_switch_table_info_get(SWITCH_DEVICE_ATTR_TABLE_NEIGHBOR, table_info);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to get table size: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    nbrs = table_info.table_size - table_info.table_usage;
    *count =
        nbrs < (ipv4_hosts + ipv6_hosts) ? nbrs : (ipv4_hosts + ipv6_hosts);
  }
  return status;
}

/*
 * Routine Description:
 *    Get switch attribute value
 *
 * Arguments:
 *    [in] attr_count - number of switch attributes
 *   [in] switch_id Switch id
 *    [inout] attr_list - array of switch attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_switch_attribute(_In_ sai_object_id_t switch_id,
                                      _In_ uint32_t attr_count,
                                      _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  // uint64_t flags = 0;
  // uint32_t max_queues = 0;
  // uint8_t max_pools = 0;
  // uint32_t cntr_refresh_interval = 0;
  // uint64_t buffer_size = 0;
  // uint32_t max_tcs = 0;
  // uint64_t seed = 0;

  // switch_acl_action_t switch_packet_action;
  // switch_packet_type_t switch_packet_type = SWITCH_PACKET_TYPE_UNICAST;
  // sai_packet_action_t sai_packet_action;
  // bool cut_through = false;

  (void)switch_status;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_object_id = {.data = switch_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_SWITCH_ATTR_PORT_LIST: {
        switch_object_id_t first_handle = {0};
        std::vector<switch_object_id_t> handles_list;
        attr_w attr(0);
        uint32_t num_ports = 0;
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_NUM_PORTS, attr);
        attr.v_get(num_ports);

        // skip CPU port
        switch_status =
            bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
        switch_status = bf_switch_get_next_handles(
            first_handle, 512, handles_list, num_ports);
        std::list<sai_object_id_t> port_list;
        for (const auto &handle : handles_list) {
          switch_enum_t type = {};
          attr_w port_type(SWITCH_PORT_ATTR_TYPE);
          switch_status =
              bf_switch_attribute_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
          port_type.v_get(type);
          if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;
          // For FP bf_switch_get_next_handles also return the internal ports
          // which should not be shown to the user
          switch_enum_t loopback = {};
          attr_w loopback_mode(SWITCH_PORT_ATTR_LOOPBACK_MODE);
          switch_status = bf_switch_attribute_get(
              handle, SWITCH_PORT_ATTR_LOOPBACK_MODE, loopback_mode);
          loopback_mode.v_get(loopback);
          if (loopback.enumdata == SWITCH_PORT_ATTR_LOOPBACK_MODE_MAC_NEAR) {
            continue;
          }
          port_list.push_back(handle.data);
        }
        TRY_LIST_SET(attr_list[index].value.objlist, port_list);
        break;
      }
      case SAI_SWITCH_ATTR_CUSTOM_5: {
        switch_object_id_t ecmp_v4_hash_oid;
        attr_w v4_hash(SWITCH_DEVICE_ATTR_ECMP_IPV4_HASH);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_ECMP_IPV4_HASH, v4_hash);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get default ecmp ipv4 hash handle");
          break;
        }
        v4_hash.v_get(ecmp_v4_hash_oid);
        if (ecmp_v4_hash_oid.data == 0) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR("Failed device ecmp ipv4 hash is not configured");
        } else {
          attr_w sym_attr(SWITCH_HASH_ATTR_SYMMETRIC_HASH);
          switch_status = bf_switch_attribute_get(
              ecmp_v4_hash_oid, SWITCH_HASH_ATTR_SYMMETRIC_HASH, sym_attr);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR(
                "Failed to get ECMP v4 symmetric hash attribute, error: %s",
                sai_metadata_get_status_name(status));
            break;
          }
          sym_attr.v_get(attr_list[index].value.booldata);
        }
        break;
      }
      case SAI_SWITCH_ATTR_CUSTOM_6: {
        switch_object_id_t ecmp_v6_hash_oid;
        attr_w v6_hash(SWITCH_DEVICE_ATTR_ECMP_IPV6_HASH);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_ECMP_IPV6_HASH, v6_hash);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get default ecmp ipv6 hash handle");
          break;
        }
        v6_hash.v_get(ecmp_v6_hash_oid);
        if (ecmp_v6_hash_oid.data == 0) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR("Failed device ecmp ipv6 hash is not configured");
        } else {
          attr_w sym_attr(SWITCH_HASH_ATTR_SYMMETRIC_HASH);
          switch_status = bf_switch_attribute_get(
              ecmp_v6_hash_oid, SWITCH_HASH_ATTR_SYMMETRIC_HASH, sym_attr);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR(
                "Failed to get ECMP v6 symmetric hash attribute, error: %s",
                sai_metadata_get_status_name(status));
            break;
          }
          sym_attr.v_get(attr_list[index].value.booldata);
        }
        break;
      }
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM: {
        uint32_t hash_attr = 0;
        switch_enum_t algorithm = {};
        switch_object_id_t hash_algo_obj{};
        attr_w attr(SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO, attr);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get ECMP default hash algo, error: %s",
                        sai_metadata_get_status_name(status));
        }
        attr.v_get(hash_algo_obj);
        if (hash_algo_obj.data) {
          attr_w algo(SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO);
          switch_status = bf_switch_attribute_get(
              hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algo);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR("Failed to get ECMP default hash algo, error: %s",
                          sai_metadata_get_status_name(status));
          }
          algo.v_get(algorithm);
          hash_attr = algorithm.enumdata;
        } else {
          attr_w algo(SWITCH_DEVICE_ATTR_ECMP_HASH_ALGO_CACHE);
          switch_status = bf_switch_attribute_get(
              device_handle, SWITCH_DEVICE_ATTR_ECMP_HASH_ALGO_CACHE, algo);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR("Failed to get ECMP default hash algo, error: %s",
                          sai_metadata_get_status_name(status));
          }
          algo.v_get(hash_attr);
        }
        int32_t sai_val = switch_to_sai_hash_algo_attr(hash_attr);
        attr_list[index].value.s32 = sai_val;
        break;
      }
      case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM: {
        uint32_t hash_attr = 0;
        switch_enum_t algorithm = {};
        switch_object_id_t hash_algo_obj{};
        attr_w attr(SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO, attr);
        if (switch_status != SWITCH_STATUS_SUCCESS) {
          status = status_switch_to_sai(switch_status);
          SAI_LOG_ERROR("Failed to get LAG default hash algo, error: %s",
                        sai_metadata_get_status_name(status));
        }
        attr.v_get(hash_algo_obj);
        if (hash_algo_obj.data) {
          attr_w algo(SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO);
          switch_status = bf_switch_attribute_get(
              hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algo);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR("Failed to get LAG default hash algo, error: %s",
                          sai_metadata_get_status_name(status));
          }
          algo.v_get(algorithm);
          hash_attr = algorithm.enumdata;
        } else {
          attr_w algo(SWITCH_DEVICE_ATTR_LAG_HASH_ALGO_CACHE);
          switch_status = bf_switch_attribute_get(
              device_handle, SWITCH_DEVICE_ATTR_LAG_HASH_ALGO_CACHE, algo);
          if (switch_status != SWITCH_STATUS_SUCCESS) {
            status = status_switch_to_sai(switch_status);
            SAI_LOG_ERROR("Failed to get LAG default hash algo, error: %s",
                          sai_metadata_get_status_name(status));
          }
          algo.v_get(hash_attr);
        }
        int32_t sai_val = switch_to_sai_hash_algo_attr(hash_attr);
        attr_list[index].value.s32 = sai_val;
        break;
      }
      case SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY:
        attr_list[index].value.ptr =
            (void *)sai_switch_notifications.on_fdb_event;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY:
        attr_list[index].value.ptr =
            (void *)sai_switch_notifications.on_port_state_change;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY:
        attr_list[index].value.ptr =
            (void *)sai_switch_notifications.on_packet_event;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY:
        attr_list[index].value.ptr =
            (void *)sai_switch_notifications.on_switch_state_change;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_SHUTDOWN_REQUEST_NOTIFY:
        attr_list[index].value.ptr =
            (void *)                                              // NOLINT
            sai_switch_notifications.on_switch_shutdown_request;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_BFD_SESSION_STATE_CHANGE_NOTIFY:
        attr_list[index].value.ptr =
            (void *)                                               // NOLINT
            sai_switch_notifications.on_bfd_session_state_change;  // NOLINT
        break;
      case SAI_SWITCH_ATTR_QOS_MAX_NUMBER_OF_SCHEDULER_GROUP_HIERARCHY_LEVELS:
      case SAI_SWITCH_ATTR_QOS_MAX_NUMBER_OF_SCHEDULER_GROUPS_PER_HIERARCHY_LEVEL:
        attr_list[index].value.u32 = 1;
        break;
      case SAI_SWITCH_ATTR_ECMP_HASH: {  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_WARN(
            "Get switch attribute %s is not supported, return status %s",
            sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr_list[index].id),
            sai_metadata_get_status_name(status));
      } break;
      case SAI_SWITCH_ATTR_FDB_UNICAST_MISS_PACKET_ACTION:
      case SAI_SWITCH_ATTR_FDB_BROADCAST_MISS_PACKET_ACTION:
      case SAI_SWITCH_ATTR_FDB_MULTICAST_MISS_PACKET_ACTION: {
        switch_attr_id_t sw_attr_id =
            sai_to_switch_fdb_action(attr_list[index].id);
        attr_w ac_attr(sw_attr_id);
        uint8_t action = 0;
        switch_status =
            bf_switch_attribute_get(device_handle, sw_attr_id, ac_attr);
        ac_attr.v_get(action);
        switch (action) {
          case 0:
            attr_list[index].value.s32 = SAI_PACKET_ACTION_DROP;
            break;
          case 1:
            attr_list[index].value.s32 = SAI_PACKET_ACTION_FORWARD;
            break;
          case 2:
            attr_list[index].value.s32 = SAI_PACKET_ACTION_TRAP;
            break;
          case 3:
            attr_list[index].value.s32 = SAI_PACKET_ACTION_COPY;
            action = 3;
            break;
          default:
            attr_list[index].value.s32 = SAI_PACKET_ACTION_FORWARD;
            break;
        }
      } break;
      case SAI_SWITCH_ATTR_MAX_ACL_RANGE_COUNT: {
        uint64_t table_id = sai_get_switchapi_table_id(attr_list[index].id);
        if (table_id == SWITCH_DEVICE_ATTR_TABLE_MAX) {
          SAI_LOG_ERROR("Failed to get proper table mapping");
          return SAI_STATUS_FAILURE;
        }
        switch_table_info_t table_info = {};
        switch_status = bf_switch_table_info_get(table_id, table_info);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get max acl range count: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = table_info.table_size;
      } break;
      case SAI_SWITCH_ATTR_NUMBER_OF_ECMP_GROUPS:
      case SAI_SWITCH_ATTR_FDB_TABLE_SIZE:
      case SAI_SWITCH_ATTR_NUMBER_OF_LAGS:
      case SAI_SWITCH_ATTR_L3_NEIGHBOR_TABLE_SIZE: {
        uint64_t table_id = sai_get_switchapi_table_id(attr_list[index].id);
        if (table_id == SWITCH_DEVICE_ATTR_TABLE_MAX) {
          SAI_LOG_ERROR("Failed to get proper table mapping");
          return SAI_STATUS_FAILURE;
        }
        switch_table_info_t table_info = {};
        switch_status = bf_switch_table_info_get(table_id, table_info);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get table size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = table_info.table_size;
      } break;
      // Return sum of all fib tables
      case SAI_SWITCH_ATTR_L3_ROUTE_TABLE_SIZE: {
        uint64_t route_tbl_size = 0;
        status = sai_route_entry_get_availability(
            switch_id, 0, NULL, &route_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get l3 route table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = (uint32_t)route_tbl_size;
      } break;
      // Return sum of all ipv4 fib tables
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_ROUTE_ENTRY: {
        uint64_t route_tbl_size = 0;
        sai_attribute_t attr;
        attr.id = SAI_ROUTE_ENTRY_ATTR_IP_ADDR_FAMILY;
        attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV4;
        status = sai_route_entry_get_availability(
            switch_id, 1, &attr, &route_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get ipv4 route table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = (uint32_t)route_tbl_size;
      } break;
      // Return sum of all ipv6 fib tables
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_ROUTE_ENTRY: {
        uint64_t route_tbl_size = 0;
        sai_attribute_t attr;
        attr.id = SAI_ROUTE_ENTRY_ATTR_IP_ADDR_FAMILY;
        attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV6;
        status = sai_route_entry_get_availability(
            switch_id, 1, &attr, &route_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get ipv6 route table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = (uint32_t)route_tbl_size;
      } break;

      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_MEMBER_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_FDB_ENTRY: {
        uint64_t table_id = sai_get_switchapi_table_id(attr_list[index].id);
        if (table_id == SWITCH_DEVICE_ATTR_TABLE_MAX) {
          SAI_LOG_ERROR("Failed to get proper table mapping");
          return SAI_STATUS_FAILURE;
        }
        switch_table_info_t table_info = {};
        switch_status = bf_switch_table_info_get(table_id, table_info);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get table size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 =
            (table_info.table_size - table_info.table_usage);
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEIGHBOR_ENTRY: {
        uint64_t nbrs = 0;
        sai_attribute_t attr;
        attr.id = SAI_NEIGHBOR_ENTRY_ATTR_IP_ADDR_FAMILY;
        attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV4;
        status =
            sai_neighbor_entry_get_availability(switch_id, 1, &attr, &nbrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get ipv4 neigh table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = (uint32_t)nbrs;
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEIGHBOR_ENTRY: {
        uint64_t nbrs = 0;
        sai_attribute_t attr;
        attr.id = SAI_NEIGHBOR_ENTRY_ATTR_IP_ADDR_FAMILY;
        attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV6;
        status =
            sai_neighbor_entry_get_availability(switch_id, 1, &attr, &nbrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get ipv6 neigh table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = (uint32_t)nbrs;
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_ACL_TABLE:
      case SAI_SWITCH_ATTR_AVAILABLE_ACL_TABLE_GROUP: {
        uint32_t count = 0, aclresource_size = 0;
        uint32_t ingress_avail = 0, egress_avail = 0;
        uint32_t racl_avail = 0, system_i_avail = 0, system_e_avail = 0;
        uint32_t aclresource_count = 0;

        aclresource_count = 2 * (SAI_ACL_BIND_POINT_TYPE_SWITCH + 1);
        if (aclresource_count > attr_list[index].value.aclresource.count) {
          attr_list[index].value.aclresource.count = aclresource_count;
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr_list[index].value.aclresource.count = aclresource_count;

        if (attr_list[index].value.aclresource.list == NULL) {
          SAI_LOG_ERROR("unimitialized aclresource list");
          return SAI_STATUS_INVALID_ATTR_VALUE_0 + index;
        }

        // same value for many stage-bind point combination
        std::vector<uint64_t> ingress_table = {
            SWITCH_DEVICE_ATTR_TABLE_IP_ACL,
            SWITCH_DEVICE_ATTR_TABLE_IPV4_ACL,
            SWITCH_DEVICE_ATTR_TABLE_IPV6_ACL,
            SWITCH_DEVICE_ATTR_TABLE_MAC_ACL,
            SWITCH_DEVICE_ATTR_TABLE_ECN_ACL,
            SWITCH_DEVICE_ATTR_TABLE_IP_MIRROR_ACL};
        std::vector<uint64_t> egress_table = {
            SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV4_ACL,
            SWITCH_DEVICE_ATTR_TABLE_EGRESS_IPV6_ACL,
            SWITCH_DEVICE_ATTR_TABLE_EGRESS_MAC_ACL};
        status = sai_get_available_size_from_table_list(ingress_table,
                                                        ingress_avail);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get ingress table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        status =
            sai_get_available_size_from_table_list(egress_table, egress_avail);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get egress table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }

        // ingress system
        {
          switch_table_info_t table_info = {};
          switch_status = bf_switch_table_info_get(
              SWITCH_DEVICE_ATTR_TABLE_SYSTEM_ACL, table_info);
          system_i_avail = (table_info.table_size - table_info.table_usage);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to get table available count %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
        }

        // egress system
        {
          switch_table_info_t table_info = {};
          switch_status = bf_switch_table_info_get(
              SWITCH_DEVICE_ATTR_TABLE_EGRESS_SYSTEM_ACL, table_info);
          system_e_avail = (table_info.table_size - table_info.table_usage);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to get table available count %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
        }

        count = 0;
        aclresource_size = sizeof(sai_acl_resource_t) * aclresource_count;
        memset(attr_list[index].value.aclresource.list, 0, aclresource_size);
        for (uint64_t bpt = SAI_ACL_BIND_POINT_TYPE_PORT;
             bpt <= SAI_ACL_BIND_POINT_TYPE_SWITCH;
             bpt++) {
          for (uint64_t stage = SAI_ACL_STAGE_INGRESS;
               stage <= SAI_ACL_STAGE_EGRESS;
               stage++) {
            attr_list[index].value.aclresource.list[count].bind_point =
                static_cast<sai_acl_bind_point_type_t>(bpt);
            attr_list[index].value.aclresource.list[count].stage =
                static_cast<sai_acl_stage_t>(stage);
            if (bpt == SAI_ACL_BIND_POINT_TYPE_ROUTER_INTERFACE) {
              attr_list[index].value.aclresource.list[count].avail_num =
                  racl_avail;
            } else if (bpt == SAI_ACL_BIND_POINT_TYPE_SWITCH) {
              if (stage == SAI_ACL_STAGE_INGRESS)
                attr_list[index].value.aclresource.list[count].avail_num =
                    system_i_avail;
              else
                attr_list[index].value.aclresource.list[count].avail_num =
                    system_e_avail;
            } else {
              attr_list[index].value.aclresource.list[count].avail_num =
                  stage == SAI_ACL_STAGE_INGRESS ? ingress_avail : egress_avail;
            }
            count++;
          }
        }
      } break;
      case SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO:                  // Unsupported
      case SAI_SWITCH_ATTR_ECMP_HASH_IPV4_IN_IPV4:                // Unsupported
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_SYMMETRIC_HASH:           // Unsupported
      case SAI_SWITCH_ATTR_LAG_HASH_IPV4_IN_IPV4:                 // Unsupported
      case SAI_SWITCH_ATTR_QOS_DOT1P_TO_TC_MAP:                   // Unsupported
      case SAI_SWITCH_ATTR_QOS_DOT1P_TO_COLOR_MAP:                // Unsupported
      case SAI_SWITCH_ATTR_QOS_DSCP_TO_TC_MAP:                    // Unsupported
      case SAI_SWITCH_ATTR_QOS_DSCP_TO_COLOR_MAP:                 // Unsupported
      case SAI_SWITCH_ATTR_QOS_TC_TO_QUEUE_MAP:                   // Unsupported
      case SAI_SWITCH_ATTR_QOS_TC_AND_COLOR_TO_DOT1P_MAP:         // Unsupported
      case SAI_SWITCH_ATTR_QOS_TC_AND_COLOR_TO_DSCP_MAP:          // Unsupported
      case SAI_SWITCH_ATTR_TAM_OBJECT_ID:                         // Unsupported
      case SAI_SWITCH_ATTR_NAT_ZONE_COUNTER_OBJECT_ID:            // Unsupported
      case SAI_SWITCH_ATTR_PORT_CONNECTOR_LIST:                   // Unsupported
      case SAI_SWITCH_ATTR_MACSEC_OBJECT_LIST:                    // Unsupported
      case SAI_SWITCH_ATTR_QOS_MPLS_EXP_TO_TC_MAP:                // Unsupported
      case SAI_SWITCH_ATTR_QOS_MPLS_EXP_TO_COLOR_MAP:             // Unsupported
      case SAI_SWITCH_ATTR_NUMBER_OF_SYSTEM_PORTS:                // Unsupported
      case SAI_SWITCH_ATTR_SYSTEM_PORT_LIST:                      // Unsupported
      case SAI_SWITCH_ATTR_FABRIC_PORT_LIST:                      // Unsupported
      case SAI_SWITCH_ATTR_TUNNEL_OBJECTS_LIST:                   // Unsupported
      case SAI_SWITCH_ATTR_MY_MAC_LIST:                           // Unsupported
      case SAI_SWITCH_ATTR_IPSEC_OBJECT_ID:                       // Unsupported
      case SAI_SWITCH_ATTR_QOS_DSCP_TO_FORWARDING_CLASS_MAP:      // Unsupported
      case SAI_SWITCH_ATTR_QOS_TC_AND_COLOR_TO_MPLS_EXP_MAP:      // Unsupported
      case SAI_SWITCH_ATTR_DEFAULT_OVERRIDE_VIRTUAL_ROUTER_ID:    // Unsupported
      case SAI_SWITCH_ATTR_QOS_MPLS_EXP_TO_FORWARDING_CLASS_MAP:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_SWITCH_ATTR_MAX_LEARNED_ADDRESSES:
        attr_list[index].value.u32 = 0;
        break;
      case SAI_SWITCH_ATTR_FDB_AGING_TIME: {
        uint32_t aging_interval = 0;
        attr_w age_attr(SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL, age_attr);
        age_attr.v_get(aging_interval);
        attr_list[index].value.u32 = aging_interval / 1000;
      } break;
      case SAI_SWITCH_ATTR_MAX_ACL_ACTION_COUNT: {
        std::set<sai_acl_action_type_t> ingress_actions_list;
        std::set<sai_acl_action_type_t> egress_actions_list;
        status = sai_acl_get_supported_actions(
            device_handle, SAI_ACL_STAGE_INGRESS, ingress_actions_list);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get number of supported ingress ACL actions, error: "
              "%s\n",
              sai_metadata_get_status_name(status));
        }
        status = sai_acl_get_supported_actions(
            device_handle, SAI_ACL_STAGE_EGRESS, egress_actions_list);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get number of supported egress ACL actions, error: "
              "%s\n",
              sai_metadata_get_status_name(status));
        }
        attr_list[index].value.u32 =
            ingress_actions_list.size() + egress_actions_list.size();
      } break;
      case SAI_SWITCH_ATTR_ACL_STAGE_INGRESS:
      case SAI_SWITCH_ATTR_ACL_STAGE_EGRESS: {
        std::set<sai_acl_action_type_t> actions_list;

        status = sai_acl_get_supported_actions(
            device_handle,
            attr_list[index].id == SAI_SWITCH_ATTR_ACL_STAGE_INGRESS
                ? SAI_ACL_STAGE_INGRESS
                : SAI_ACL_STAGE_EGRESS,
            actions_list);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get number of supported %s ACL actions, error: "
              "%s\n",
              attr_list[index].id == SAI_SWITCH_ATTR_ACL_STAGE_INGRESS
                  ? "ingress"
                  : "egress",
              sai_metadata_get_status_name(status));
          return status;
        }
        TRY_LIST_SET(attr_list[index].value.aclcapability.action_list,
                     actions_list);
        attr_list[index].value.aclcapability.is_action_list_mandatory = false;
      } break;
      case SAI_SWITCH_ATTR_NAT_ENABLE: {
        attr_list[index].value.booldata =
            bf_switch_is_feature_enabled(SWITCH_FEATURE_NAT) ? true : false;
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_SNAT_ENTRY: {
        std::vector<uint64_t> tables = {SWITCH_DEVICE_ATTR_TABLE_SNAT,
                                        SWITCH_DEVICE_ATTR_TABLE_SNAPT};
        uint32_t snat_tbl_size = 0;
        status = sai_get_available_size_from_table_list(tables, snat_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get SNAT table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = snat_tbl_size;
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_DNAT_ENTRY: {
        std::vector<uint64_t> tables = {SWITCH_DEVICE_ATTR_TABLE_DNAT,
                                        SWITCH_DEVICE_ATTR_TABLE_DNAPT};
        uint32_t dnat_tbl_size = 0;
        status = sai_get_available_size_from_table_list(tables, dnat_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get DNAT table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = dnat_tbl_size;
      } break;
      case SAI_SWITCH_ATTR_AVAILABLE_DOUBLE_NAT_ENTRY: {
        std::vector<uint64_t> tables = {SWITCH_DEVICE_ATTR_TABLE_FLOW_NAT,
                                        SWITCH_DEVICE_ATTR_TABLE_FLOW_NAPT};
        uint32_t double_nat_tbl_size = 0;
        status =
            sai_get_available_size_from_table_list(tables, double_nat_tbl_size);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get double NAT table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = double_nat_tbl_size;
      } break;
      case SAI_SWITCH_ATTR_TYPE:
        attr_list[index].value.s32 = SAI_SWITCH_TYPE_NPU;
        break;
      case SAI_SWITCH_ATTR_AVAILABLE_IPMC_ENTRY: {
        std::vector<uint64_t> tables = {
            SWITCH_DEVICE_ATTR_TABLE_IPV4_MCAST_ROUTE_S_G,
            SWITCH_DEVICE_ATTR_TABLE_IPV4_MCAST_ROUTE_X_G,
        };
        uint32_t avail = 0;
        status = sai_get_available_size_from_table_list(tables, avail);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get IPMC table avail size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 = avail;
      } break;
#if SAI_API_VERSION >= 10901
      case SAI_SWITCH_ATTR_AVAILABLE_MY_SID_ENTRY: {
        uint64_t table_id = sai_get_switchapi_table_id(attr_list[index].id);
        if (table_id == SWITCH_DEVICE_ATTR_TABLE_MAX) {
          SAI_LOG_ERROR("Failed to get proper table mapping");
          return SAI_STATUS_FAILURE;
        }
        switch_table_info_t table_info = {};
        switch_status = bf_switch_table_info_get(table_id, table_info);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get table size: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        attr_list[index].value.u32 =
            (table_info.table_size - table_info.table_usage);
      } break;
#endif
      case SAI_SWITCH_ATTR_PRE_INGRESS_ACL: {
        switch_object_id_t acl_id;
        switch_object_id_t device_id = {.data = switch_id};
        attr_w acl_id_attr(SWITCH_DEVICE_ATTR_PRE_INGRESS_ACL);
        switch_status = bf_switch_attribute_get(
            device_id, SWITCH_DEVICE_ATTR_PRE_INGRESS_ACL, acl_id_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get switch pre ingress acl, error: "
              "%s\n",
              sai_metadata_get_status_name(status));
          return status;
        }
        acl_id_attr.v_get(acl_id);
        attr_list[index].value.oid = acl_id.data;
      } break;
      case SAI_SWITCH_ATTR_SUPPORTED_OBJECT_TYPE_LIST: {
        TRY_LIST_SET(attr_list[index].value.s32list,
                     bf_sai_get_supported_object_types());
      } break;
      case SAI_SWITCH_ATTR_MAX_NUMBER_OF_TEMP_SENSORS:
      case SAI_SWITCH_ATTR_TEMP_LIST:
      case SAI_SWITCH_ATTR_MAX_TEMP:
      case SAI_SWITCH_ATTR_AVERAGE_TEMP: {
        attr_w temp_list_attr(SWITCH_DEVICE_ATTR_TEMP_LIST);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_TEMP_LIST, temp_list_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get temperature list attribute: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        std::vector<int64_t> temp_list = {};
        temp_list_attr.v_get(temp_list);
        switch (attr_list[index].id) {
          case SAI_SWITCH_ATTR_MAX_NUMBER_OF_TEMP_SENSORS:
            attr_list[index].value.u8 = temp_list.size();
            break;
          case SAI_SWITCH_ATTR_TEMP_LIST: {
            TRY_LIST_SET(attr_list[index].value.s32list, temp_list);
          } break;
          case SAI_SWITCH_ATTR_MAX_TEMP: {
            if (temp_list.size() == 0) return SAI_STATUS_NOT_SUPPORTED;
            auto temp = *std::max_element(temp_list.begin(), temp_list.end());
            attr_list[index].value.s32 = static_cast<int32_t>(temp);
          } break;
          case SAI_SWITCH_ATTR_AVERAGE_TEMP: {
            if (temp_list.size() == 0) return SAI_STATUS_NOT_SUPPORTED;
            auto temp =
                std::accumulate(temp_list.begin(), temp_list.end(), 0.0) /
                temp_list.size();
            attr_list[index].value.s32 = static_cast<int32_t>(temp);
          } break;
          default:
            break;
        }
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_SWITCH, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attr_list[index].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}  // NOLINT

sai_status_t sai_create_switch(_Out_ sai_object_id_t *switch_id,
                               _In_ uint32_t attr_count,
                               _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  const sai_attribute_t *attribute;
  sai_status_t status = SAI_STATUS_SUCCESS;
  unsigned index = 0;

  if (!switch_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *switch_id = device_handle.data;

  // switch_api should be initialized here..
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      // mandatory create attributes ..
      default:
        status = sai_set_switch_attribute(*switch_id, attribute);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to set switch attribute at create %s: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_SWITCH, attribute->id),
              sai_metadata_get_status_name(status));
        }
    }
  }
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

sai_status_t sai_remove_switch(_In_ sai_object_id_t switch_id) {
  (void)switch_id;
  return SAI_STATUS_SUCCESS;
}

inline sai_port_oper_status_t switch_oper_state_to_sai_oper_state(
    switch_port_oper_status_event_t oper_status) {
  switch (oper_status) {
    case SWITCH_PORT_OPER_STATUS_UP:
      return SAI_PORT_OPER_STATUS_UP;
    case SWITCH_PORT_OPER_STATUS_DOWN:
      return SAI_PORT_OPER_STATUS_DOWN;
    case SWITCH_PORT_OPER_STATUS_UNKNOWN:
    default:
      return SAI_PORT_OPER_STATUS_UNKNOWN;
  }
}

void sai_port_state_batch_change(uint32_t count,
                                 switch_port_oper_status_event_data_t *data) {
  std::vector<sai_port_oper_status_notification_t> notifs;

  if (!sai_switch_notifications.on_port_state_change) return;
  if (count == 0) return;
  if (!data) {
    SAI_LOG_ERROR("data == NULL");
    return;
  }

  notifs.reserve(count);

  for (uint32_t i = 0; i < count; ++i) {
    sai_port_oper_status_notification_t n = {};

    SAI_LOG_INFO("BFN SDK link status: 0x%" PRIx64 " %d",
                 data[i].object_id.data,
                 data[i].port_status_event);

    n.port_id = data[i].object_id.data;
    n.port_state =
        switch_oper_state_to_sai_oper_state(data[i].port_status_event);

    notifs.push_back(n);
  }

  sai_switch_notifications.on_port_state_change(notifs.size(), notifs.data());
}

void sai_port_state_change(const switch_port_oper_status_event_data_t data) {
  sai_port_oper_status_notification_t port_sc_notif;

  memset(&port_sc_notif, 0x0, sizeof(port_sc_notif));
  port_sc_notif.port_id = data.object_id.data;
  port_sc_notif.port_state =
      switch_oper_state_to_sai_oper_state(data.port_status_event);

  if (sai_switch_notifications.on_port_state_change) {
    // this greatly helps in debugging Sonic link issues
    SAI_LOG_INFO("BFN SDK link status: 0x%" PRIx64 " %d",
                 data.object_id.data,
                 data.port_status_event);
    sai_switch_notifications.on_port_state_change(0x1, &port_sc_notif);
  }

  return;
}

sai_status_t sai_switch_set_learn_notif_timeout(const switch_object_id_t device,
                                                uint32_t timeout) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  attr_w sattr(SWITCH_DEVICE_ATTR_LEARN_NOTIF_TIMEOUT, timeout);
  switch_status = bf_switch_attribute_set(device, sattr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_ERROR("Failed to set device learn timeout, error: %s",
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

sai_status_t sai_get_switch_stats(_In_ sai_object_id_t switch_id,
                                  _In_ uint32_t number_of_counters,
                                  _In_ const sai_stat_id_t *counter_ids,
                                  _Out_ uint64_t *counters) {
  SAI_LOG_WARN("API not implemented");

  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t sai_get_switch_stats_ext(_In_ sai_object_id_t switch_id,
                                      _In_ uint32_t number_of_counters,
                                      _In_ const sai_stat_id_t *counter_ids,
                                      _In_ sai_stats_mode_t mode,
                                      _Out_ uint64_t *counters) {
  sai_status_t status = SAI_STATUS_NOT_SUPPORTED;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t first_handle = {0};
  std::vector<switch_object_id_t> handles_list;
  uint32_t num_ports = 0;

  SAI_LOG_ENTER();

  // skip CPU port
  switch_status =
      bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
  switch_status =
      bf_switch_get_next_handles(first_handle, 512, handles_list, num_ports);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    SAI_LOG_WARN("Failed to get port handles, %s",
                 sai_metadata_get_status_name(status));
    return SAI_STATUS_INVALID_PARAMETER;
  }

  uint64_t total_sum = 0;
  for (const auto &handle : handles_list) {
    switch_enum_t type = {};
    attr_w port_type(SWITCH_PORT_ATTR_TYPE);
    switch_status =
        bf_switch_attribute_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    port_type.v_get(type);
    if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;

    sai_object_id_t port_id = handle.data;
    // passes debug counter indexes
    status = sai_get_port_stats_ext(
        port_id, number_of_counters, counter_ids, mode, counters);
    total_sum += *counters;
  }
  *counters = total_sum;

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Switch method table retrieved with sai_api_query()
 */
sai_switch_api_t switch_api = {
    .create_switch = sai_create_switch,
    .remove_switch = sai_remove_switch,
    .set_switch_attribute = sai_set_switch_attribute,
    .get_switch_attribute = sai_get_switch_attribute,
    .get_switch_stats = sai_get_switch_stats,
    .get_switch_stats_ext = sai_get_switch_stats_ext};

sai_switch_api_t *sai_switch_api_get() { return &switch_api; }

static sai_switch_oper_status_t switch_state_event_to_sai_oper_event(
    switch_device_oper_status_event_t switch_event) {
  switch (switch_event) {
    case SWITCH_DEVICE_OPER_STATUS_FAILED:
      return SAI_SWITCH_OPER_STATUS_FAILED;
    default:
      return SAI_SWITCH_OPER_STATUS_UNKNOWN;
  }
}

void switch_device_notify_cb(const switch_device_event_data_t data) {
  SAI_LOG_ENTER();

  sai_switch_oper_status_t switch_ev_type =
      switch_state_event_to_sai_oper_event(data.device_status_event);
  sai_object_id_t sai_sw_id = data.device_handle;
  if (sai_switch_notifications.on_switch_state_change) {
    sai_switch_notifications.on_switch_state_change(sai_sw_id, switch_ev_type);
  }

  SAI_LOG_EXIT();
  return;
}

void sai_switch_initialize() {
  SAI_LOG_DEBUG("Initializing switch");
  device_handle = sai_get_device_id(0);
  bf_switch_event_register(SWITCH_PORT_OPER_STATUS_EVENT,
                           (void *)&sai_port_state_change);  // NOLINT
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_SWITCH);
  sai_generate_attr_capability(SAI_OBJECT_TYPE_SWITCH);

  SAI_LOG_DEBUG("Initializing switch device oper status event");
  device_handle = sai_get_device_id(0);
  bf_switch_event_register(SWITCH_DEVICE_EVENT,
                           reinterpret_cast<void *>(&switch_device_notify_cb));
  return;
}
