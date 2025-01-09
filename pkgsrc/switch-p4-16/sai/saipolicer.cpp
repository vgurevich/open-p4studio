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

static sai_api_t api_id = SAI_API_POLICER;

/**
 * @brief Create Policer
 *
 * @param[out] policer_id - the policer id
 * @param[in] switch_id Switch id
 * @param[in] attr_count - number of attributes
 * @param[in] attr_list - array of attributes
 *
 * @return SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_create_policer(_Out_ sai_object_id_t *policer_id,
                                _In_ sai_object_id_t switch_id,
                                _In_ uint32_t attr_count,
                                _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_METER;

  if (!policer_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *policer_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_meter_object_id = {};
  std::set<smi::attr_w> sw_meter_attrs;
  bool single_rate_mode = false;
  bool storm_control_mode = false;
  uint64_t cir = 0;
  uint64_t cbs = 0;
  uint64_t pir = 0;
  uint64_t pbs = 0;

  for (uint32_t index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_POLICER_ATTR_MODE: {
        if (SAI_POLICER_MODE_SR_TCM == attribute->value.s32) {
          single_rate_mode = true;
        } else if (SAI_POLICER_MODE_STORM_CONTROL == attribute->value.s32) {
          storm_control_mode = true;
        }
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_POLICER, attribute, sw_meter_attrs);
        break;
      }
      case SAI_POLICER_ATTR_PIR:
        pir = attribute->value.u64;
        break;
      case SAI_POLICER_ATTR_PBS:
        pbs = attribute->value.u64;
        break;
      case SAI_POLICER_ATTR_CIR:
        cir = attribute->value.u64;
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_POLICER, attribute, sw_meter_attrs);
        break;
      case SAI_POLICER_ATTR_CBS:
        cbs = attribute->value.u64;
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_POLICER, attribute, sw_meter_attrs);
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_POLICER, attribute, sw_meter_attrs);
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "failed to create policer: %s : sai_to_switch attr map failed, ",
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  smi::attr_w pir_attr(SWITCH_METER_ATTR_PIR);
  smi::attr_w pbs_attr(SWITCH_METER_ATTR_PBS);
  // if SR_TCM or STORM_CONTROL, pretend like TR_TCM
  if (single_rate_mode) {
    pir = cir;
    if (pbs == 0) {
      pbs = cbs;
    }
  } else if (storm_control_mode) {
    pir = cir;
    pbs = cbs;
  }
  // set pir and pbs as we skipped above
  pir_attr.v_set(pir);
  pbs_attr.v_set(pbs);
  sw_meter_attrs.insert(pir_attr);
  sw_meter_attrs.insert(pbs_attr);

  sai_insert_device_attribute(0, SWITCH_METER_ATTR_DEVICE, sw_meter_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_meter_attrs, switch_meter_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS &&
      status != SAI_STATUS_ITEM_ALREADY_EXISTS) {
    SAI_LOG_ERROR("failed to create policer: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *policer_id = switch_meter_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Delete policer
 *
 * @param[in] policer_id - Policer id
 *
 * @return  SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_remove_policer(_In_ sai_object_id_t policer_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(policer_id) == SAI_OBJECT_TYPE_POLICER);
  const switch_object_id_t sw_object_id = {.data = policer_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to delete policer %" PRIx64 ": %s",
                  policer_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief  Set Policer attribute
 *
 * @param[in] policer_id - Policer id
 * @param[in] attr - attribute
 *
 * @return SAI_STATUS_SUCCESS on success
 *        Failure status code on error
 */
sai_status_t sai_set_policer_attribute(_In_ sai_object_id_t policer_id,
                                       _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(policer_id) == SAI_OBJECT_TYPE_POLICER);

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t oid = {.data = policer_id};

  switch (attr->id) {
    case SAI_POLICER_ATTR_PIR:
      switch_status |= bf_switch_attribute_set(
          oid, attr_w(SWITCH_METER_ATTR_PIR, attr->value.u64));
      break;
    case SAI_POLICER_ATTR_CIR:
      switch_status |= bf_switch_attribute_set(
          oid, attr_w(SWITCH_METER_ATTR_CIR, attr->value.u64));
      break;
    case SAI_POLICER_ATTR_PBS:
      switch_status |= bf_switch_attribute_set(
          oid, attr_w(SWITCH_METER_ATTR_PBS, attr->value.u64));
      break;
    case SAI_POLICER_ATTR_CBS:
      switch_status |= bf_switch_attribute_set(
          oid, attr_w(SWITCH_METER_ATTR_CBS, attr->value.u64));
      break;
    case SAI_POLICER_ATTR_MODE:
      return SAI_STATUS_INVALID_ATTRIBUTE_0;
    default:
      status = sai_to_switch_attribute_set(SAI_OBJECT_TYPE_POLICER, attr, oid);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Set policer attribute failed to convert attribute: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  bool single_rate_mode = false;
  bool storm_control_mode = false;
  smi::attr_w mod_attr(SWITCH_METER_ATTR_MODE);
  switch_enum_t mode = {.enumdata = SWITCH_METER_ATTR_MODE_NONE};
  switch_status |=
      bf_switch_attribute_get(oid, SWITCH_METER_ATTR_MODE, mod_attr);
  mod_attr.v_get(mode);
  if (mode.enumdata == SWITCH_METER_ATTR_MODE_SINGLE_RATE_THREE_COLOR) {
    single_rate_mode = true;
  } else if (mode.enumdata == SWITCH_METER_ATTR_MODE_STORM_CONTROL) {
    storm_control_mode = true;
  }

  if (single_rate_mode || storm_control_mode) {
    smi::attr_w cir_attr(SWITCH_METER_ATTR_CIR);
    smi::attr_w pir_attr(SWITCH_METER_ATTR_PIR);

    switch_status |=
        bf_switch_attribute_get(oid, SWITCH_METER_ATTR_CIR, cir_attr);

    uint64_t value = 0;
    cir_attr.v_get(value);
    pir_attr.v_set(value);

    switch_status |= bf_switch_attribute_set(oid, pir_attr);
  }

  if (storm_control_mode) {
    smi::attr_w cbs_attr(SWITCH_METER_ATTR_CBS);
    smi::attr_w pbs_attr(SWITCH_METER_ATTR_PBS);

    switch_status |=
        bf_switch_attribute_get(oid, SWITCH_METER_ATTR_CBS, cbs_attr);

    uint64_t value = 0;
    cbs_attr.v_get(value);
    pbs_attr.v_set(value);

    switch_status |= bf_switch_attribute_set(oid, pbs_attr);
  }

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set policer 0x%" PRIx64 "attribute %s: %s",
                  policer_id,
                  sai_attribute_name(SAI_OBJECT_TYPE_POLICER, attr->id),
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/**
 * @brief  Get Policer attribute
 *
 * @param[in] policer_id - policer id
 * @param[in] attr_count - number of attributes
 * @param[inout] attr_list - array of attributes
 *
 * @return SAI_STATUS_SUCCESS on success
 *        Failure status code on error
 */
sai_status_t sai_get_policer_attribute(_In_ sai_object_id_t policer_id,
                                       _In_ uint32_t attr_count,
                                       _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_attribute_t *attr = attr_list;
  unsigned int i = 0;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(policer_id) == SAI_OBJECT_TYPE_POLICER);
  const switch_object_id_t sw_object_id = {.data = policer_id};

  for (i = 0, attr = attr_list; i < attr_count; i++, attr++) {
    switch (attr->id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_POLICER, sw_object_id, attr);
        if ((status != SAI_STATUS_SUCCESS) &&
            (status != SAI_STATUS_NOT_SUPPORTED)) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_POLICER, attr->id),
                        sai_metadata_get_status_name(status));
        }
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

static sai_status_t switch_meter_counters_to_sai_meter_counters(
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids,
    _In_ std::vector<switch_counter_t> &sw_meter_cntrs,
    _Out_ uint64_t *counters) {
  std::vector<switch_counter_t> sw_meter_id_cntrs(SWITCH_METER_COUNTER_ID_MAX,
                                                  {0, 0});

  for (auto const &cntr : sw_meter_cntrs) {
    if (cntr.counter_id >= SWITCH_METER_COUNTER_ID_MAX) {
      SAI_LOG_ERROR("Unsupported meter counter id: %d", cntr.counter_id);
      continue;
    }
    sw_meter_id_cntrs[cntr.counter_id] = cntr;
  }

  uint32_t index = 0;
  for (index = 0; index < number_of_counters; index++) {
    switch (counter_ids[index]) {
      case SAI_POLICER_STAT_PACKETS:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count +
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count +
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count;
        break;
      case SAI_POLICER_STAT_GREEN_PACKETS:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_GREEN_PACKETS].count;
        break;
      case SAI_POLICER_STAT_YELLOW_PACKETS:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_YELLOW_PACKETS].count;
        break;
      case SAI_POLICER_STAT_RED_PACKETS:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_RED_PACKETS].count;
        break;
      case SAI_POLICER_STAT_ATTR_BYTES:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count +
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count +
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count;
        break;
      case SAI_POLICER_STAT_GREEN_BYTES:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_GREEN_BYTES].count;
        break;
      case SAI_POLICER_STAT_YELLOW_BYTES:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_YELLOW_BYTES].count;
        break;
      case SAI_POLICER_STAT_RED_BYTES:
        counters[index] =
            sw_meter_id_cntrs[SWITCH_METER_COUNTER_ID_RED_BYTES].count;
        break;
      default:
        break;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_policer_statistics(_In_ sai_object_id_t policer_id,
                                        _In_ uint32_t number_of_counters,
                                        _In_ const sai_stat_id_t *counter_ids,
                                        _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  const switch_object_id_t sw_meter_object_id = {.data = policer_id};
  std::vector<switch_counter_t> sw_meter_cntrs;

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_counters_get(sw_meter_object_id, sw_meter_cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    status = SAI_STATUS_NO_MEMORY;
    SAI_LOG_ERROR("failed to get meter stats meter_object_id: %" PRIx64 ": %s",
                  policer_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_meter_counters_to_sai_meter_counters(
      number_of_counters, counter_ids, sw_meter_cntrs, counters);

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief   Clear policer statistics counters.
 *
 * @param[in] policer_id policer id
 * @param[in] number_of_counters number of counters in the array
 * @param[in] counter_ids specifies the array of counter ids
 *
 * @return SAI_STATUS_SUCCESS on success
 *         Failure status code on error
 */
sai_status_t sai_clear_policer_stats(_In_ sai_object_id_t policer_id,
                                     _In_ uint32_t number_of_counters,
                                     _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("Policer stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(policer_id) != SAI_OBJECT_TYPE_POLICER) {
    SAI_LOG_ERROR("Policer stats clear failed: invalid handle 0x%" PRIx64,
                  policer_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = policer_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    switch (counter_ids[i]) {
      case SAI_POLICER_STAT_PACKETS:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_PACKETS);
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_YELLOW_PACKETS);
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_PACKETS);
        break;
      case SAI_POLICER_STAT_GREEN_PACKETS:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_PACKETS);
        break;
      case SAI_POLICER_STAT_YELLOW_PACKETS:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_YELLOW_PACKETS);
        break;
      case SAI_POLICER_STAT_RED_PACKETS:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_PACKETS);
        break;
      case SAI_POLICER_STAT_ATTR_BYTES:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_BYTES);
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_YELLOW_BYTES);
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_BYTES);
        break;
      case SAI_POLICER_STAT_GREEN_BYTES:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_GREEN_BYTES);
        break;
      case SAI_POLICER_STAT_YELLOW_BYTES:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_YELLOW_BYTES);
        break;
      case SAI_POLICER_STAT_RED_BYTES:
        cntr_ids.push_back(SWITCH_METER_COUNTER_ID_RED_BYTES);
        break;
      default:
        break;
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(sw_object_id, cntr_ids);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear policer 0x%" PRIx64 " stats, status %s",
                    policer_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Policer methods table retrieved with sai_api_query()
 */
sai_policer_api_t policer_api = {
    .create_policer = sai_create_policer,
    .remove_policer = sai_remove_policer,
    .set_policer_attribute = sai_set_policer_attribute,
    .get_policer_attribute = sai_get_policer_attribute,
    .get_policer_stats = sai_get_policer_statistics,
    .get_policer_stats_ext = NULL,
    .clear_policer_stats = sai_clear_policer_stats};

sai_policer_api_t *sai_policer_api_get() { return &policer_api; }

sai_status_t sai_policer_initialize() {
  SAI_LOG_DEBUG("Initializing policer interface");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_POLICER);
  return SAI_STATUS_SUCCESS;
}
