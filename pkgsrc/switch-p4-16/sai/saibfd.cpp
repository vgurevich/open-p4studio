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

static sai_api_t api_id = SAI_API_BFD;

/**
 * Routine Description:
 *     Create a BFD session
 *
 * Arguments:
 *     [out] bfd_session_id BFD SESSION ID
 *     [in] switch_id Switch id
 *     [in] attr_count Number of attributes
 *     [in] attr_list Array of attributes
 *
 * Return Values:
 *     SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_bfd_session(_Out_ sai_object_id_t *bfd_session_id,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_BFD_SESSION;
  switch_object_id_t sw_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  if (!bfd_session_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *bfd_session_id = SAI_NULL_OBJECT_ID;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_BFD_SESSION_ATTR_HW_LOOKUP_VALID:
        break;
      case SAI_BFD_SESSION_ATTR_UDP_SRC_PORT: {
        attr_w udp_port(SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT,
                        static_cast<uint16_t>(attr_list[i].value.u32));
        sw_attrs.insert(udp_port);
        break;
      }
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_BFD_SESSION, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create bfd session: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_BFD_SESSION_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(ot, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create bfd session: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *bfd_session_id = sw_object_id.data;
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove a BFD session
 *
 * Arguments:
 *    [in] sai_object_id_t bfd session id - a handle
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_bfd_session(_In_ sai_object_id_t bfd_session_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(bfd_session_id) != SAI_OBJECT_TYPE_BFD_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bfd_session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = bfd_session_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove bfd session %" PRIx64 ": %s",
                  bfd_session_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set BFD session attribute Value
 *
 * Arguments:
 *    [in] bfd_session_id - BFD_SESSION id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_bfd_session_attribute(_In_ sai_object_id_t bfd_session_id,
                                           _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(bfd_session_id) != SAI_OBJECT_TYPE_BFD_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bfd_session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = bfd_session_id};
  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_BFD_SESSION, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_BFD_SESSION, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get BFD_SESSION attribute Value
 *
 * Arguments:
 *    [in] bfd_session_id - BFD_SESSION id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_bfd_session_attribute(_In_ sai_object_id_t bfd_session_id,
                                           _In_ uint32_t attr_count,
                                           _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(bfd_session_id) != SAI_OBJECT_TYPE_BFD_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bfd_session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = bfd_session_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_BFD_SESSION_ATTR_HW_LOOKUP_VALID:
        attr_list[i].value.booldata = true;
        break;
      case SAI_BFD_SESSION_ATTR_UDP_SRC_PORT: {
        uint16_t udp_port;
        attr_w udp_port_attr(SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_BFD_SESSION_ATTR_UDP_SRC_PORT, udp_port_attr);
        status = status_switch_to_sai(switch_status);
        udp_port_attr.v_get(udp_port);
        attr_list[i].value.u32 = udp_port;
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_BFD_SESSION, sw_object_id, &attr_list[i]);
        break;
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get bfd session attribute object_id: 0x%" PRIx64
          "attribute %s "
          "error: %s",
          bfd_session_id,
          sai_attribute_name(SAI_OBJECT_TYPE_BFD_SESSION, attr_list[i].id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * Routine Description:
 *   @brief Clear bfd session statistics counters.
 *
 * Arguments:
 *    @param[in] bfd session - bfd session id
 *    @param[in] counter_ids - specifies the array of counter ids
 *    @param[in] number_of_counters - number of counters in the array
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t sai_clear_bfd_session_stats(
    _In_ sai_object_id_t bfd_session_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("bfd session stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(bfd_session_id) != SAI_OBJECT_TYPE_BFD_SESSION) {
    SAI_LOG_ERROR(
        "bfd session stats clear failed: invalid bfd handle 0x%" PRIx64,
        bfd_session_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = bfd_session_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    switch (counter_ids[i]) {
      default:
        break;
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(sw_object_id, cntr_ids);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear bfd session 0x%" PRIx64
                    " stats, status %s",
                    bfd_session_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *   Get bfd session statistics counters.
 *
 * Arguments:
 *    [in] bfd session - bfd session id
 *    [in] counter_ids - specifies the array of counter ids
 *    [in] number_of_counters - number of counters in the array
 *    [out] counters - array of resulting counter values.
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_bfd_session_stats(_In_ sai_object_id_t bfd_session_id,
                                       _In_ uint32_t number_of_counters,
                                       _In_ const sai_stat_id_t *counter_ids,
                                       _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> bfd_session_cntrs;

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(bfd_session_id) != SAI_OBJECT_TYPE_BFD_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bfd_session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = bfd_session_id};
  switch_status = bf_switch_counters_get(sw_object_id, bfd_session_cntrs);

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get bfd stats object_id: %" PRIx64 ": %s",
                  bfd_session_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

static void sai_bfd_notify_cb(const switch_bfd_event_data_t &data) {
  sai_bfd_session_state_notification_t n = {
      .bfd_session_id = data.bfd_session_handle.data,
  };
  switch (data.bfd_session_state) {
    case SWITCH_BFD_SESSION_STATE_ADMIN_DOWN:
      n.session_state = SAI_BFD_SESSION_STATE_ADMIN_DOWN;
      break;
    case SWITCH_BFD_SESSION_STATE_DOWN:
      n.session_state = SAI_BFD_SESSION_STATE_DOWN;
      break;
    case SWITCH_BFD_SESSION_STATE_INIT:
      n.session_state = SAI_BFD_SESSION_STATE_INIT;
      break;
    case SWITCH_BFD_SESSION_STATE_UP:
      n.session_state = SAI_BFD_SESSION_STATE_UP;
      break;
    default:
      return;
  }
  if (sai_switch_notifications.on_bfd_session_state_change) {
    sai_switch_notifications.on_bfd_session_state_change(1, &n);
  }
}
/*
 * BFD methods table retrieved with sai_api_query()
 */

sai_bfd_api_t bfd_api = {
  create_bfd_session : sai_create_bfd_session,
  remove_bfd_session : sai_remove_bfd_session,
  set_bfd_session_attribute : sai_set_bfd_session_attribute,
  get_bfd_session_attribute : sai_get_bfd_session_attribute,
  get_bfd_session_stats : sai_get_bfd_session_stats,
  get_bfd_session_stats_ext : NULL,
  clear_bfd_session_stats : sai_clear_bfd_session_stats
};

sai_bfd_api_t *sai_bfd_api_get() { return &bfd_api; }

sai_status_t sai_bfd_initialize() {
  sai_status_t status = SAI_STATUS_SUCCESS;

  bf_switch_event_register(SWITCH_BFD_EVENT,
                           (void *)&sai_bfd_notify_cb);  // NOLINT
  SAI_LOG_DEBUG("Initializing bfd");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_BFD_SESSION);

  return status;
}
