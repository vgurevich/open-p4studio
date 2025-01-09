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

static sai_api_t api_id = SAI_API_WRED;
#define SAI_WRED_MAX_INSTANCES_IN_PROFILE 3

typedef enum _sai_wred_color_t {
  SAI_WRED_COLOR_GREEN = 0,
  SAI_WRED_COLOR_YELLOW,
  SAI_WRED_COLOR_RED
} sai_wred_color_t;

typedef struct _sai_wred_info_t {
  switch_object_id_t wred_id;
  std::set<smi::attr_w> sw_attrs;
} sai_wred_info_t;

typedef struct _sai_wred_profile_info_t {
  sai_wred_info_t wred_list[SAI_WRED_MAX_INSTANCES_IN_PROFILE];
} sai_wred_profile_info_t;

/*
 * Routine Description:
 *   Converts SAI WRED color to string
 *
 * Arguments:
 *  [in] color - color
 *
 * Return Values:
 *    String appearance of the color specified
 */
const char *sai_wred_color_to_str(_In_ sai_wred_color_t color) {
  switch (color) {
    case SAI_WRED_COLOR_GREEN:
      return "GREEN";
    case SAI_WRED_COLOR_YELLOW:
      return "YELLOW";
    case SAI_WRED_COLOR_RED:
      return "RED";
    default:
      break;
  }

  return "unknown";
}

/*
 * Routine Description:
 *   Converts SAI WRED ECN mode to
 *   WRED profile info
 *
 * Arguments:
 *  [in] ecn_mode - SAI ECN mode attribute
 *  [out] wr_info - WRED profile info
 *
 * Return Values:
 *    void
 */
void sai_wred_ecn_mark_mode_to_wred_profile_info(
    _In_ sai_ecn_mark_mode_t ecn_mode, _Out_ sai_wred_profile_info_t &wr_info) {
  auto &green_sw_list = wr_info.wred_list[SAI_WRED_COLOR_GREEN].sw_attrs;
  auto &yellow_sw_list = wr_info.wred_list[SAI_WRED_COLOR_YELLOW].sw_attrs;
  auto &red_sw_list = wr_info.wred_list[SAI_WRED_COLOR_RED].sw_attrs;

  switch (ecn_mode) {
    case SAI_ECN_MARK_MODE_NONE:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      break;
    case SAI_ECN_MARK_MODE_GREEN:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      break;
    case SAI_ECN_MARK_MODE_YELLOW:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      break;
    case SAI_ECN_MARK_MODE_RED:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      break;
    case SAI_ECN_MARK_MODE_GREEN_YELLOW:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      break;
    case SAI_ECN_MARK_MODE_GREEN_RED:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      break;
    case SAI_ECN_MARK_MODE_YELLOW_RED:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, false));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      break;
    case SAI_ECN_MARK_MODE_ALL:
      green_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      yellow_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      red_sw_list.insert(smi::attr_w(SWITCH_WRED_ATTR_ECN_MARK, true));
      break;
    default:
      break;
  }
}

/*
 * Routine Description:
 *   Converts SAI WRED attribute to
 *   WRED profile info
 *
 * Arguments:
 *  [in] attr - SAI WRED attribute
 *  [out] wr_info - WRED profile info
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_wred_attr_to_wred_profile_info(
    _In_ const sai_attribute_t &attr, _Out_ sai_wred_profile_info_t &wr_info) {
  auto &green_sw_list = wr_info.wred_list[SAI_WRED_COLOR_GREEN].sw_attrs;
  auto &yellow_sw_list = wr_info.wred_list[SAI_WRED_COLOR_YELLOW].sw_attrs;
  auto &red_sw_list = wr_info.wred_list[SAI_WRED_COLOR_RED].sw_attrs;

  switch (attr.id) {
    case SAI_WRED_ATTR_GREEN_ENABLE:
      green_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_ENABLE, attr.value.booldata));
      break;
    case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
      green_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MIN_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
      green_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MAX_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_GREEN_DROP_PROBABILITY:
      green_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_PROBABILITY, attr.value.u32));
      break;
    case SAI_WRED_ATTR_YELLOW_ENABLE:
      yellow_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_ENABLE, attr.value.booldata));
      break;
    case SAI_WRED_ATTR_YELLOW_MIN_THRESHOLD:
      yellow_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MIN_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_YELLOW_MAX_THRESHOLD:
      yellow_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MAX_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_YELLOW_DROP_PROBABILITY:
      yellow_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_PROBABILITY, attr.value.u32));
      break;
    case SAI_WRED_ATTR_RED_ENABLE:
      red_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_ENABLE, attr.value.booldata));
      break;
    case SAI_WRED_ATTR_RED_MIN_THRESHOLD:
      red_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MIN_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_RED_MAX_THRESHOLD:
      red_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_MAX_THRESHOLD, attr.value.u32));
      break;
    case SAI_WRED_ATTR_RED_DROP_PROBABILITY:
      red_sw_list.insert(
          smi::attr_w(SWITCH_WRED_ATTR_PROBABILITY, attr.value.u32));
      break;
    case SAI_WRED_ATTR_ECN_MARK_MODE:
      sai_wred_ecn_mark_mode_to_wred_profile_info(
          (sai_ecn_mark_mode_t)attr.value.u32, wr_info);
      break;
    case SAI_WRED_ATTR_WEIGHT:
      break;
    default:
      SAI_LOG_ERROR("SAI WRED attribute %s conversion failed - not supported",
                    sai_attribute_name(SAI_OBJECT_TYPE_WRED, attr.id));
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Given a WRED profile handle gets its WRED handles
 *   from switch.
 *
 * Arguments:
 *  [in] wred_profile - WRED profile handle
 *  [out] wr_info - WRED profile info
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_wred_handles_get_from_sw(
    _In_ switch_object_id_t wred_profile,
    _Out_ sai_wred_profile_info_t &wr_info) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  smi::attr_w green_wred(SWITCH_WRED_PROFILE_ATTR_WRED_GREEN_HANDLE);
  sw_status = bf_switch_attribute_get(
      wred_profile, SWITCH_WRED_PROFILE_ATTR_WRED_GREEN_HANDLE, green_wred);

  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get %s wred handle for wred profile 0x%" PRIx64
                  ": %s",
                  sai_wred_color_to_str(SAI_WRED_COLOR_GREEN),
                  wred_profile.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  green_wred.v_get(wr_info.wred_list[SAI_WRED_COLOR_GREEN].wred_id);

  smi::attr_w yellow_wred(SWITCH_WRED_PROFILE_ATTR_WRED_YELLOW_HANDLE);
  sw_status = bf_switch_attribute_get(
      wred_profile, SWITCH_WRED_PROFILE_ATTR_WRED_YELLOW_HANDLE, yellow_wred);

  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get %s wred handle for wred profile 0x%" PRIx64
                  ": %s",
                  sai_wred_color_to_str(SAI_WRED_COLOR_YELLOW),
                  wred_profile.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  yellow_wred.v_get(wr_info.wred_list[SAI_WRED_COLOR_YELLOW].wred_id);

  smi::attr_w red_wred(SWITCH_WRED_PROFILE_ATTR_WRED_RED_HANDLE);
  sw_status = bf_switch_attribute_get(
      wred_profile, SWITCH_WRED_PROFILE_ATTR_WRED_RED_HANDLE, red_wred);

  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get %s wred handle for wred profile 0x%" PRIx64
                  ": %s",
                  sai_wred_color_to_str(SAI_WRED_COLOR_RED),
                  wred_profile.data,
                  sai_metadata_get_status_name(status));
    return status;
  }
  red_wred.v_get(wr_info.wred_list[SAI_WRED_COLOR_RED].wred_id);

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Create WRED Profile
 *
 * Arguments:
 *    [out] wred_id - WRED profile Id
 *    [in] switch_id - switch Id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_wred(_Out_ sai_object_id_t *wred_id,
                             _In_ sai_object_id_t switch_id,
                             _In_ uint32_t attr_count,
                             _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_wred_profile_info_t wr_info;
  switch_object_id_t wred_profile = {};
  std::set<smi::attr_w> sw_attrs;  // wred profile attributes

  if (!attr_list || !wred_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *wred_id = SAI_NULL_OBJECT_ID;

  for (uint32_t index = 0; index < attr_count; index++) {
    status = sai_wred_attr_to_wred_profile_info(attr_list[index], wr_info);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create wred: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  // Create WREDs
  for (uint32_t i = SAI_WRED_COLOR_GREEN; i < SAI_WRED_MAX_INSTANCES_IN_PROFILE;
       i++) {
    sai_insert_device_attribute(
        0, SWITCH_WRED_ATTR_DEVICE, wr_info.wred_list[i].sw_attrs);

    auto iter = wr_info.wred_list[i].sw_attrs.find(
        smi::attr_w(SWITCH_WRED_ATTR_PROBABILITY));
    if (iter == wr_info.wred_list[i].sw_attrs.end()) {
      // Add default probability value in case it's missing in the attributes
      // list
      uint32_t prob = 100;
      wr_info.wred_list[i].sw_attrs.insert(
          smi::attr_w(SWITCH_WRED_ATTR_PROBABILITY, prob));
    }

    sw_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_WRED,
                                        wr_info.wred_list[i].sw_attrs,
                                        wr_info.wred_list[i].wred_id);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create %s wred: %s",
                    sai_wred_color_to_str((sai_wred_color_t)i),
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  // Create WRED profile
  sai_insert_device_attribute(0, SWITCH_WRED_PROFILE_ATTR_DEVICE, sw_attrs);
  sw_attrs.insert(smi::attr_w(SWITCH_WRED_PROFILE_ATTR_WRED_GREEN_HANDLE,
                              wr_info.wred_list[SAI_WRED_COLOR_GREEN].wred_id));
  sw_attrs.insert(
      smi::attr_w(SWITCH_WRED_PROFILE_ATTR_WRED_YELLOW_HANDLE,
                  wr_info.wred_list[SAI_WRED_COLOR_YELLOW].wred_id));
  sw_attrs.insert(smi::attr_w(SWITCH_WRED_PROFILE_ATTR_WRED_RED_HANDLE,
                              wr_info.wred_list[SAI_WRED_COLOR_RED].wred_id));

  sw_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_WRED_PROFILE, sw_attrs, wred_profile);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create wred profile: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *wred_id = wred_profile.data;

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Remove WRED Profile
 *
 * Arguments:
 *    [in] wred_id - WRED profile Id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_wred(_In_ sai_object_id_t wred_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_wred_profile_info_t wr_info;

  if (sai_object_type_query(wred_id) != SAI_OBJECT_TYPE_WRED) {
    SAI_LOG_ERROR(
        "Wred remove failed: invalid wred handle "
        "0x%" PRIx64 "\n",
        wred_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t wred_profile = {.data = wred_id};

  status = sai_wred_handles_get_from_sw(wred_profile, wr_info);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  // Remove WRED profile
  sw_status = bf_switch_object_delete(wred_profile);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove wred profile 0x%" PRIx64 ": %s",
                  wred_id,
                  sai_metadata_get_status_name(status));
  }

  // Remove WREDs
  for (uint32_t i = SAI_WRED_COLOR_GREEN; i < SAI_WRED_MAX_INSTANCES_IN_PROFILE;
       i++) {
    sw_status = bf_switch_object_delete(wr_info.wred_list[i].wred_id);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to remove %s wred 0x%" PRIx64 ": %s",
                    sai_wred_color_to_str((sai_wred_color_t)i),
                    wr_info.wred_list[i].wred_id.data,
                    sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Set attributes to WRED profile
 *
 * Arguments:
 *    [in] wred_id - WRED profile Id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_wred_attribute(_In_ sai_object_id_t wred_id,
                                    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_wred_profile_info_t wr_info;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(wred_id) != SAI_OBJECT_TYPE_WRED) {
    SAI_LOG_ERROR(
        "Wred attribute set failed: invalid wred handle "
        "0x%" PRIx64 "\n",
        wred_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t wred_profile = {.data = wred_id};

  status = sai_wred_handles_get_from_sw(wred_profile, wr_info);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  status = sai_wred_attr_to_wred_profile_info(*attr, wr_info);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set wred profile 0x%" PRIx64 ": attribute %s",
                  wred_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  // Remove WREDs
  for (uint32_t i = SAI_WRED_COLOR_GREEN; i < SAI_WRED_MAX_INSTANCES_IN_PROFILE;
       i++) {
    for (auto iter = wr_info.wred_list[i].sw_attrs.begin();
         iter != wr_info.wred_list[i].sw_attrs.end();
         ++iter) {
      sw_status = bf_switch_attribute_set(wr_info.wred_list[i].wred_id, *iter);
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set %s wred 0x%" PRIx64 " attribute: %s",
                      sai_wred_color_to_str((sai_wred_color_t)i),
                      wr_info.wred_list[i].wred_id.data,
                      sai_metadata_get_status_name(status));
      }
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Get WRED profile attribute
 *
 * Arguments:
 *    [in] wred_id - WRED Profile Id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_wred_attribute(_In_ sai_object_id_t wred_id,
                                    _In_ uint32_t attr_count,
                                    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  sai_wred_profile_info_t wr_info;
  smi::attr_w sw_attr(0);

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(wred_id) != SAI_OBJECT_TYPE_WRED) {
    SAI_LOG_ERROR(
        "Wred attribute get failed: invalid wred handle "
        "0x%" PRIx64 "\n",
        wred_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t wred_profile = {.data = wred_id};

  status = sai_wred_handles_get_from_sw(wred_profile, wr_info);
  if (status != SAI_STATUS_SUCCESS) {
    return status;
  }

  auto green_wr_id = wr_info.wred_list[SAI_WRED_COLOR_GREEN].wred_id;
  auto yellow_wr_id = wr_info.wred_list[SAI_WRED_COLOR_YELLOW].wred_id;
  auto red_wr_id = wr_info.wred_list[SAI_WRED_COLOR_RED].wred_id;

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_WRED_ATTR_GREEN_ENABLE:
        sw_status = bf_switch_attribute_get(
            green_wr_id, SWITCH_WRED_ATTR_ENABLE, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.booldata);
        }
        break;
      case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            green_wr_id, SWITCH_WRED_ATTR_MIN_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            green_wr_id, SWITCH_WRED_ATTR_MAX_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_GREEN_DROP_PROBABILITY:
        sw_status = bf_switch_attribute_get(
            green_wr_id, SWITCH_WRED_ATTR_PROBABILITY, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_YELLOW_ENABLE:
        sw_status = bf_switch_attribute_get(
            yellow_wr_id, SWITCH_WRED_ATTR_ENABLE, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.booldata);
        }
        break;
      case SAI_WRED_ATTR_YELLOW_MIN_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            yellow_wr_id, SWITCH_WRED_ATTR_MIN_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_YELLOW_MAX_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            yellow_wr_id, SWITCH_WRED_ATTR_MAX_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_YELLOW_DROP_PROBABILITY:
        sw_status = bf_switch_attribute_get(
            yellow_wr_id, SWITCH_WRED_ATTR_PROBABILITY, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_RED_ENABLE:
        sw_status = bf_switch_attribute_get(
            red_wr_id, SWITCH_WRED_ATTR_ENABLE, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.booldata);
        }
        break;
      case SAI_WRED_ATTR_RED_MIN_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            red_wr_id, SWITCH_WRED_ATTR_MIN_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_RED_MAX_THRESHOLD:
        sw_status = bf_switch_attribute_get(
            red_wr_id, SWITCH_WRED_ATTR_MAX_THRESHOLD, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_RED_DROP_PROBABILITY:
        sw_status = bf_switch_attribute_get(
            red_wr_id, SWITCH_WRED_ATTR_PROBABILITY, sw_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          sw_attr.v_get(attr_list[index].value.u32);
        }
        break;
      case SAI_WRED_ATTR_ECN_MARK_MODE:
        break;
      case SAI_WRED_ATTR_WEIGHT:
        break;
      default:
        SAI_LOG_ERROR(
            "SAI WRED attribute %s get failed - not supported",
            sai_attribute_name(SAI_OBJECT_TYPE_WRED, attr_list[index].id));
        return SAI_STATUS_NOT_SUPPORTED;
    }

    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get attribute %s error: %s\n",
          sai_attribute_name(SAI_OBJECT_TYPE_WRED, attr_list[index].id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 *  WRED methods table retrieved with sai_api_query()
 */
sai_wred_api_t wred_api = {
  create_wred : sai_create_wred,
  remove_wred : sai_remove_wred,
  set_wred_attribute : sai_set_wred_attribute,
  get_wred_attribute : sai_get_wred_attribute
};

sai_wred_api_t *sai_wred_api_get() { return &wred_api; }

sai_status_t sai_wred_initialize() {
  SAI_LOG_DEBUG("Initializing wred");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_WRED);
  return SAI_STATUS_SUCCESS;
}
