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

static sai_api_t api_id = SAI_API_SCHEDULER;

/*
 * Routine Description:
 *   Converts SAI scheduler rate from bytes per second to
 *   bits per second
 *
 * Arguments:
 *  [in] rate_in_bytes - rate in bytes per second
 *
 * Return Values:
 *    Converted rate value
 */
static uint64_t sai_convert_rate_to_bps(uint64_t rate_in_bytes) {
  return rate_in_bytes * 8;
}

/*
 * Routine Description:
 *   Converts SAI scheduler rate from bits per second to
 *   bytes per second
 *
 * Arguments:
 *  [in] rate_in_bps - rate in bits per second
 *
 * Return Values:
 *    Converted rate value
 */
static uint64_t sai_convert_rate_to_bytes(uint64_t rate_in_bps) {
  return rate_in_bps / 8;
}

/*
 * Routine Description:
 *   Converts SAI scheduler attribute to
 *   switch attributes
 *
 * Arguments:
 *  [in] attr - SAI scheduler attribute
 *  [in] is_pps - flag signalizing that meter is
 *                configured in packets per second
 *  [out] sw_attrs - switch scheduler attributes list
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t sai_scheduler_attr_to_switch(
    _In_ const sai_attribute_t &attr,
    bool is_pps,
    _Out_ std::set<smi::attr_w> &sw_attrs) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (attr.id) {
    case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT:
      sw_attrs.insert(
          smi::attr_w(SWITCH_SCHEDULER_ATTR_WEIGHT, (uint16_t)attr.value.u8));
      break;
    case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE: {
      uint64_t rate =
          is_pps ? attr.value.u64 : sai_convert_rate_to_bps(attr.value.u64);
      sw_attrs.insert(smi::attr_w(SWITCH_SCHEDULER_ATTR_MIN_RATE, rate));
      break;
    }
    case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE: {
      uint64_t rate =
          is_pps ? attr.value.u64 : sai_convert_rate_to_bps(attr.value.u64);
      sw_attrs.insert(smi::attr_w(SWITCH_SCHEDULER_ATTR_MAX_RATE, rate));
      break;
    }
    default:
      status =
          sai_to_switch_attribute(SAI_OBJECT_TYPE_SCHEDULER, &attr, sw_attrs);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to conver SAI attribute %s to switch, error: %s\n",
            sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER, attr.id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  return status;
}

/*
 *  Routine Description:
 *    Create Scheduler Profile
 *
 *  Arguments:
 *    [out] scheduler_id - scheduler id
 *    [in] switch_id - switch Object id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_scheduler(_Out_ sai_object_id_t *scheduler_id,
                                  _In_ sai_object_id_t switch_id,
                                  _In_ uint32_t attr_count,
                                  _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;
  sai_uint32_t shced_meter_type = 0;

  if (!scheduler_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *scheduler_id = SAI_NULL_OBJECT_ID;

  std::set<smi::attr_w> sw_attrs;
  const sai_attribute_t *scheduler_type = sai_get_attr_from_list(
      SAI_SCHEDULER_ATTR_METER_TYPE, attr_list, attr_count);
  if (!scheduler_type) {
    // If meter type is not passed as an attribute, set the default type
    shced_meter_type = SAI_METER_TYPE_BYTES;
    switch_enum_t shaper_type = {.enumdata =
                                     SWITCH_SCHEDULER_ATTR_SHAPER_TYPE_BPS};
    sw_attrs.insert(
        smi::attr_w(SWITCH_SCHEDULER_ATTR_SHAPER_TYPE, shaper_type));
  } else {
    shced_meter_type = scheduler_type->value.s32;
  }

  bool is_pps = (shced_meter_type == SAI_METER_TYPE_PACKETS);

  switch_object_id_t scheduler_object_id = {};
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      default:
        status = sai_scheduler_attr_to_switch(*attribute, is_pps, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create scheduler: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_SCHEDULER_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_SCHEDULER, sw_attrs, scheduler_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create scheduler: %s",
                  sai_metadata_get_status_name(status));
  }
  *scheduler_id = scheduler_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  Routine Description:
 *    Remove Scheduler profile
 *
 *  Arguments:
 *    [in] scheduler_id - scheduler id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_scheduler(_In_ sai_object_id_t scheduler_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(scheduler_id) != SAI_OBJECT_TYPE_SCHEDULER) {
    SAI_LOG_ERROR("Scheduler remove failed: invalid scheduler handle 0x%" PRIx64
                  "\n",
                  scheduler_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = scheduler_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove scheduler 0x%" PRIx64 ": %s",
                  scheduler_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Set Scheduler Attribute
 *
 * Arguments:
 *   [in] scheduler_id - scheduler id
 *   [in] attr - attribute
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_set_scheduler_attribute(_In_ sai_object_id_t scheduler_id,
                                         _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = scheduler_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(scheduler_id) != SAI_OBJECT_TYPE_SCHEDULER) {
    SAI_LOG_ERROR("Scheduler set failed: invalid scheduler handle 0x%" PRIx64
                  "\n",
                  scheduler_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  smi::attr_w sw_attr(SWITCH_SCHEDULER_ATTR_SHAPER_TYPE);

  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_SCHEDULER_ATTR_SHAPER_TYPE, sw_attr);

  if (sw_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(sw_status);
    SAI_LOG_ERROR("Failed to get scheduler 0x%" PRIx64 " shaper type: %s",
                  scheduler_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  std::set<smi::attr_w> sw_attrs;
  switch_enum_t scheduler_type = {.enumdata =
                                      SWITCH_SCHEDULER_ATTR_SHAPER_TYPE_PPS};
  sw_attr.v_get(scheduler_type);
  bool is_pps =
      (scheduler_type.enumdata == SWITCH_SCHEDULER_ATTR_SHAPER_TYPE_PPS);

  status = sai_scheduler_attr_to_switch(*attr, is_pps, sw_attrs);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to det SAI scheduler attribute: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (auto iter = sw_attrs.begin(); iter != sw_attrs.end(); ++iter) {
    sw_status = bf_switch_attribute_set(sw_object_id, *iter);
    status = status_switch_to_sai(sw_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to set SAI scheduler attribute 0x%" PRIx64 ": %s",
                    scheduler_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Get Scheduler attribute
 *
 * Arguments:
 *   [in] scheduler_id - scheduler id
 *   [in] attr_count - number of attributes
 *   [out] attr_list - array of attributes
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_get_scheduler_attribute(_In_ sai_object_id_t scheduler_id,
                                         _In_ uint32_t attr_count,
                                         _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const switch_object_id_t sw_object_id = {.data = scheduler_id};

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(scheduler_id) != SAI_OBJECT_TYPE_SCHEDULER) {
    SAI_LOG_ERROR("Scheduler get failed: invalid scheduler handle 0x%" PRIx64
                  "\n",
                  scheduler_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  smi::attr_w sw_attr(SWITCH_SCHEDULER_ATTR_SHAPER_TYPE);

  sw_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_SCHEDULER_ATTR_SHAPER_TYPE, sw_attr);

  if (sw_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(sw_status);
    SAI_LOG_ERROR("Failed to get scheduler 0x%" PRIx64 " shaper type: %s",
                  scheduler_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_enum_t scheduler_type = {.enumdata =
                                      SWITCH_SCHEDULER_ATTR_SHAPER_TYPE_PPS};
  sw_attr.v_get(scheduler_type);
  bool is_pps =
      (scheduler_type.enumdata == SWITCH_SCHEDULER_ATTR_SHAPER_TYPE_PPS);

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT: {
        smi::attr_w weight_attr(SWITCH_SCHEDULER_ATTR_WEIGHT);
        sw_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_SCHEDULER_ATTR_WEIGHT, weight_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          uint16_t weight = 0;
          weight_attr.v_get(weight);
          attr_list[index].value.u8 = weight;
        }
        break;
      }
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE:
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE: {
        switch_attr_id_t id =
            (attr_list[index].id == SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE)
                ? SWITCH_SCHEDULER_ATTR_MIN_RATE
                : SWITCH_SCHEDULER_ATTR_MAX_RATE;

        smi::attr_w rate_attr(id);
        sw_status = bf_switch_attribute_get(sw_object_id, id, rate_attr);
        if (sw_status == SWITCH_STATUS_SUCCESS) {
          uint64_t rate = 0;
          rate_attr.v_get(rate);

          attr_list[index].value.u64 =
              is_pps ? rate : sai_convert_rate_to_bytes(rate);
        }
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_SCHEDULER, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }

    if (sw_status != SWITCH_STATUS_SUCCESS) {
      status = status_switch_to_sai(sw_status);
      SAI_LOG_ERROR(
          "Failed to get scheduler 0x%" PRIx64 " attribute: %s, error: %s",
          scheduler_id,
          sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER, attr_list[index].id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Scheduler methods table retrieved with sai_api_query()
 */
sai_scheduler_api_t scheduler_api = {
    .create_scheduler = sai_create_scheduler,
    .remove_scheduler = sai_remove_scheduler,
    .set_scheduler_attribute = sai_set_scheduler_attribute,
    .get_scheduler_attribute = sai_get_scheduler_attribute,
};

sai_scheduler_api_t *sai_scheduler_api_get() { return &scheduler_api; }

sai_status_t sai_scheduler_initialize() {
  SAI_LOG_DEBUG("Initializing scheduler");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_SCHEDULER);
  return SAI_STATUS_SUCCESS;
}
