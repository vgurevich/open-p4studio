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


#include <arpa/inet.h>
#include <inttypes.h>

#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table.hpp>

#include "tdi_p4_table_impl.hpp"
#include "tdi_p4_table_data_impl.hpp"
#include <tdi_common/tdi_table_field_utils.hpp>

#include <sstream>

namespace tdi {
namespace tna {
namespace tofino {

const std::map<std::string, pipe_lpf_type_e>
    DataFieldStringMapper::string_to_lpf_spec_type_map{
        {"RATE", LPF_TYPE_RATE}, {"SAMPLE", LPF_TYPE_SAMPLE}};
const std::map<pipe_lpf_type_e, std::string>
    DataFieldStringMapper::lpf_spec_type_to_string_map{
        {LPF_TYPE_RATE, "RATE"}, {LPF_TYPE_SAMPLE, "SAMPLE"}};

// Map of allowed string values for pipe_idle_time_hit_state_e
const std::map<pipe_idle_time_hit_state_e, std::string>
    DataFieldStringMapper::idle_time_hit_state_to_string_map{
        {ENTRY_ACTIVE, "ENTRY_ACTIVE"}, {ENTRY_IDLE, "ENTRY_IDLE"}};

namespace {

/*
 * - Get Data Field from the table
 * - Check if the data dield type matched with any of the allowed field types
 * - If the allowed_field_types set is an empty set, that means that the getter
 *   can be used for any field type
 * - Check if the data field size is compatible with the passed-in field size
 * - Check if the value passed-in is within the bounds of the data field type
 * - If the valued is being set using a byte array, then convert it to host
 *   order
 * - Set the field type which can be reused by the function calling this
 */
tdi_status_t indirectResourceSetValueHelper(
    const tdi::Table &table,
    const tdi_id_t &field_id,
    const std::set<DataFieldType> &allowed_field_types,
    const size_t &size,
    const uint64_t &value,
    const uint8_t *value_ptr,
    uint64_t *out_val,
    DataFieldType *field_type) {
  // Get the data_field from the table
  auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_type = *(static_cast<const TofinoDataFieldContextInfo *>(
                      tableDataField->dataFieldContextInfoGet())
                      ->typesGet()
                      .begin());
  if (allowed_field_types.size() &&
      allowed_field_types.find(*field_type) == allowed_field_types.end()) {
    // This indicates that this particular setter can only be used for a
    // specific set of DataFieldTypes and the fieldtype of the field
    // corresponding to the passed in field id does not match with any of
    // them
    LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      table, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }

  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, out_val);
  } else {
    *out_val = value;
  }
  return TDI_SUCCESS;
}

/*
 * - Get Data Field from the table
 * - Check if the data dield type matched with any of the allowed field types
 * - If the allowed_field_types set is an empty set, that means that the getter
 *   can be used for any field type
 * - Check if the data field size is compatible with the passed-in field size
 * - Set the field type which can be reused by the function calling this
 */
tdi_status_t indirectResourceGetValueHelper(
    const tdi::Table &table,
    const tdi_id_t &field_id,
    const std::set<DataFieldType> &allowed_field_types,
    const size_t &size,
    uint64_t *value,
    uint8_t *value_ptr,
    DataFieldType *field_type) {
  auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_type = *(static_cast<const TofinoDataFieldContextInfo *>(
                      tableDataField->dataFieldContextInfoGet())
                      ->typesGet()
                      .begin());
  if (allowed_field_types.size() &&
      allowed_field_types.find(*field_type) == allowed_field_types.end()) {
    // This indicates that this particular getter can only be used for a
    // specific set of DataFieldTypes and the fieldtype of the field
    // corresponding to the passed in field id does not match with any of
    // them
    LOG_ERROR("%s:%d %s ERROR : This getter cannot be used for field id %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Output param compatibility check failed for field "
        "id "
        "%d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }
  return TDI_SUCCESS;
}

/* This is a very light function whose only responsibility is to set the input
   to the output depending on wether the output is a 64 bit value or a byte
   array. The functions calling this, need to ensure the sanity of the params
 */

void setInputValToOutputVal(const tdi::Table &table,
                            const tdi_id_t &field_id,
                            const uint64_t &input_val,
                            uint64_t *value,
                            uint8_t *value_ptr) {
  if (value_ptr) {
    auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, input_val, value_ptr);
  } else {
    *value = input_val;
  }
}

bool is_indirect_resource(const DataFieldType &type) {
  switch (type) {
    case DataFieldType::COUNTER_INDEX:
    case DataFieldType::REGISTER_INDEX:
    case DataFieldType::METER_INDEX:
    case DataFieldType::LPF_INDEX:
    case DataFieldType::WRED_INDEX:
      return true;
    default:
      return false;
  }
  return false;
}

// Initialization is required only for direct/indirect resources. For
// ACTION_PARAM type, we don't need to initialize, since they start off a
// zero-initialized byte-arrays.
bool initialization_required(const DataFieldType &type) {
  switch (type) {
    case DataFieldType::COUNTER_INDEX:
    case DataFieldType::METER_INDEX:
    case DataFieldType::REGISTER_INDEX:
    case DataFieldType::LPF_INDEX:
    case DataFieldType::WRED_INDEX:
    case DataFieldType::COUNTER_SPEC_BYTES:
    case DataFieldType::COUNTER_SPEC_PACKETS:
    case DataFieldType::METER_SPEC_CIR_PPS:
    case DataFieldType::METER_SPEC_PIR_PPS:
    case DataFieldType::METER_SPEC_CBS_PKTS:
    case DataFieldType::METER_SPEC_PBS_PKTS:
    case DataFieldType::METER_SPEC_CIR_KBPS:
    case DataFieldType::METER_SPEC_PIR_KBPS:
    case DataFieldType::METER_SPEC_CBS_KBITS:
    case DataFieldType::METER_SPEC_PBS_KBITS:
      return true;
    default:
      return false;
  }
  return false;
}

template <class TableData>
tdi_status_t reset_action_data(const tdi_id_t &new_act_id,
                               TableData *table_data) {
  const auto &action_id = table_data->actionIdGet();
  const Table *table = nullptr;
  tdi_status_t sts = table_data->getParent(&table);

  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d ERROR in getting parent for data object", __func__, __LINE__);
    TDI_DBGCHK(0);
    return sts;
  }
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());

  // Following are the cases that are handled
  // 1. new action_id = 0
  //    Allocate pipe_action_spec with max data size possible
  // 2. new action_id != old action id
  //    Existing object's action id is different, then a new PipeActionSpec
  //    is allocated and the existing PipeActionSpec, which is managed by a
  //    unique_ptr underneath, is reset with the newly allocated object.
  //
  // 3.  new action id == old action id
  //    Existing object's action_id is same as what is passed, then we just
  //     need to zero out the PipeActionSpec

  if (!new_act_id) {
    PipeActionSpec *action_spec = nullptr;
    action_spec = new PipeActionSpec(mat_context_info->maxDataSzGet(),
                                     mat_context_info->maxDataSzBitsGet(),
                                     PIPE_ACTION_DATA_TYPE);
    table_data->actionSpecSet(action_spec);
  } else if (new_act_id != action_id) {
    PipeActionSpec *action_spec = nullptr;
    auto action = table->tableInfoGet()->actionGet(new_act_id);
    if (!action || !action->actionContextInfoGet()) {
      TDI_DBGCHK(0);
    }
    auto tofino_action_context = static_cast<const TofinoActionContextInfo *>(
        action->actionContextInfoGet());
    action_spec = new PipeActionSpec(tofino_action_context->dataSzGet(),
                                     tofino_action_context->dataSzBitsGet(),
                                     PIPE_ACTION_DATA_TYPE);
    table_data->actionSpecSet(action_spec);
  } else {
    // Just ZERO out the pipe action spec
    table_data->actionSpecSet(nullptr);
  }

  return TDI_SUCCESS;
}
}  // Anonymous namespace

// PipeActionSpec class
tdi_status_t PipeActionSpec::setValueActionParam(
    const tdi::DataFieldInfo &field,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  size_t field_size = field.sizeGet();
  size_t field_offset = static_cast<const TofinoDataFieldContextInfo *>(
                            field.dataFieldContextInfoGet())
                            ->offsetGet();

  auto size = (field_size + 7) / 8;
  if (value_ptr) {
    std::memcpy(
        action_spec.act_data.action_data_bits + field_offset, value_ptr, size);
  } else {
    utils::TableFieldUtils::toNetworkOrderData(
        field, value, action_spec.act_data.action_data_bits + field_offset);
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueActionParam(
    const tdi::DataFieldInfo &field,
    uint64_t *value,
    uint8_t *value_ptr) const {
  size_t field_size = field.sizeGet();
  size_t field_offset = static_cast<const TofinoDataFieldContextInfo *>(
                            field.dataFieldContextInfoGet())
                            ->offsetGet();

  auto size = (field_size + 7) / 8;
  if (value_ptr) {
    std::memcpy(
        value_ptr, action_spec.act_data.action_data_bits + field_offset, size);
  } else {
    utils::TableFieldUtils::toHostOrderData(
        field, action_spec.act_data.action_data_bits + field_offset, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::setValueResourceIndex(
    const tdi::DataFieldInfo &field,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t resource_idx = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &resource_idx);
  } else {
    resource_idx = value;
  }

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, false, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tbl_idx = static_cast<pipe_res_idx_t>(resource_idx);
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueResourceIndex(
    const tdi::DataFieldInfo &field,
    const pipe_tbl_hdl_t &tbl_hdl,
    uint64_t *value,
    uint8_t *value_ptr) const {
  const pipe_res_spec_t *res_spec = getResourceSpec(tbl_hdl);
  if (!res_spec) {
    LOG_ERROR("%s:%d ERROR : Resource for hdl %d does not exist for the table",
              __func__,
              __LINE__,
              tbl_hdl);
    return TDI_INVALID_ARG;
  }
  uint64_t local_val = res_spec->tbl_idx;

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, local_val, value_ptr);
  } else {
    *value = local_val;
  }
  return TDI_SUCCESS;
  ;
}

// This variant is used by tableEntryGet function to populate the counter spec
// in the data object with the counter spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueCounterSpec(
    const pipe_stat_data_t &counter) {
  // populate the internal counter object
  counter_spec.setCounterDataFromCounterSpec(counter);

  return TDI_SUCCESS;
}

// This variant is used to set the counter spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueCounterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType &field_type,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t counter_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &counter_val);
  } else {
    counter_val = value;
  }

  // populate the internal counter spec
  counter_spec.setCounterDataFromValue(field_type, counter_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      res_spec->data.counter.bytes = counter_val;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      res_spec->data.counter.packets = counter_val;
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "counter data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueCounterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType &field_type,
    uint64_t *value,
    uint8_t *value_ptr) const {
  uint64_t counter_val = 0;

  // populate the value from the counter spec
  counter_spec.getCounterData(field_type, &counter_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, counter_val, value_ptr);
  } else {
    *value = counter_val;
  }

  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the register spec
// in the data object with the register spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueRegisterSpec(
    const std::vector<pipe_stful_mem_spec_t> &register_data) {
  register_spec.populateDataFromStfulSpec(
      register_data, static_cast<uint32_t>(register_data.size()));

  return TDI_SUCCESS;
}

// This variant is used to set the register spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueRegisterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType & /*field_type*/,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t & /*field_size*/) {
  uint64_t register_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &register_val);
  } else {
    register_val = value;
  }

  // Populate the reg spec data obj
  register_spec.addRegisterToVec(field, register_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // Copy the reg spec into the action spec
  register_spec.populateStfulSpecFromData(&res_spec->data.stful);

  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueRegisterSpec(
    const tdi::DataFieldInfo &field, std::vector<uint64_t> *value) const {
  const auto &tdi_registers = register_spec.getRegisterVec();

  for (const auto &tdi_register_data : tdi_registers) {
    uint64_t reg_val = tdi_register_data.getData(field);
    value->push_back(reg_val);
  }

  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the meter spec
// in the data object with the meter spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueMeterSpec(const pipe_meter_spec_t &meter) {
  // populate the internal meter spec object
  meter_spec.setMeterDataFromMeterSpec(meter);

  return TDI_SUCCESS;
}

// This variant is used to set the meter spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueMeterSpec(const tdi::DataFieldInfo &field,
                                               const DataFieldType &field_type,
                                               const pipe_tbl_hdl_t &tbl_hdl,
                                               const uint64_t &value,
                                               const uint8_t *value_ptr) {
  uint64_t meter_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &meter_val);
  } else {
    meter_val = value;
  }

  // populate the meter obj
  meter_spec.setMeterDataFromValue(field_type, meter_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // populate the pipe action spec
  switch (field_type) {
    case METER_SPEC_CIR_PPS:
      res_spec->data.meter.cir.value.pps = meter_val;
      res_spec->data.meter.cir.type = METER_RATE_TYPE_PPS;
      break;
    case METER_SPEC_PIR_PPS:
      res_spec->data.meter.pir.value.pps = meter_val;
      res_spec->data.meter.pir.type = METER_RATE_TYPE_PPS;
      break;
    case METER_SPEC_CBS_PKTS:
      res_spec->data.meter.cburst = meter_val;
      break;
    case METER_SPEC_PBS_PKTS:
      res_spec->data.meter.pburst = meter_val;
      break;
    case METER_SPEC_CIR_KBPS:
      res_spec->data.meter.cir.value.kbps = meter_val;
      res_spec->data.meter.cir.type = METER_RATE_TYPE_KBPS;
      break;
    case METER_SPEC_PIR_KBPS:
      res_spec->data.meter.pir.value.kbps = meter_val;
      res_spec->data.meter.pir.type = METER_RATE_TYPE_KBPS;
      break;
    case METER_SPEC_CBS_KBITS:
      res_spec->data.meter.cburst = meter_val;
      break;
    case METER_SPEC_PBS_KBITS:
      res_spec->data.meter.pburst = meter_val;
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "meter data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueMeterSpec(const tdi::DataFieldInfo &field,
                                               const DataFieldType &field_type,
                                               uint64_t *value,
                                               uint8_t *value_ptr) const {
  uint64_t meter_val = 0;
  // populate the value from the meter spec
  meter_spec.getMeterDataFromValue(field_type, &meter_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, meter_val, value_ptr);
  } else {
    *value = meter_val;
  }

  return TDI_SUCCESS;
}

template <typename T>
tdi_status_t PipeActionSpec::setValueLPFSpecHelper(
    const DataFieldType &field_type,
    const pipe_tbl_hdl_t &tbl_hdl,
    const T &value) {
  // populate the lpf spec
  lpf_spec.setLPFDataFromValue<T>(field_type, value);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // populate the pipe action spec
  switch (field_type) {
    case DataFieldType::LPF_SPEC_TYPE:
      if (value != LPF_TYPE_RATE && value != LPF_TYPE_SAMPLE) {
        LOG_ERROR(
            "%s:%d ERROR : Enumvalue passed is not applicable for "
            "LPF_TPYE ENUM value %" PRIu64,
            __func__,
            __LINE__,
            static_cast<uint64_t>(value));
        return TDI_INVALID_ARG;
      }
      res_spec->data.lpf.lpf_type = static_cast<pipe_lpf_type_e>(value);
      break;
    case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR:
      res_spec->data.lpf.output_scale_down_factor =
          static_cast<uint32_t>(value);
      break;
    case DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT:
      res_spec->data.lpf.gain_time_constant = static_cast<float>(value);
      if (res_spec->data.lpf.gain_time_constant !=
          res_spec->data.lpf.decay_time_constant) {
        res_spec->data.lpf.gain_decay_separate_time_constant = true;
      } else {
        res_spec->data.lpf.gain_decay_separate_time_constant = false;
        res_spec->data.lpf.time_constant = static_cast<float>(value);
      }
      break;
    case DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT:
      res_spec->data.lpf.decay_time_constant = static_cast<float>(value);
      if (res_spec->data.lpf.gain_time_constant !=
          res_spec->data.lpf.decay_time_constant) {
        res_spec->data.lpf.gain_decay_separate_time_constant = true;
      } else {
        res_spec->data.lpf.gain_decay_separate_time_constant = false;
        res_spec->data.lpf.time_constant = static_cast<float>(value);
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "lpf data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the lpf spec
// in the data object with the lpf spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueLPFSpec(const pipe_lpf_spec_t &lpf) {
  // populate the internal lpf spec object
  lpf_spec.setLPFDataFromLPFSpec(&lpf);

  return TDI_SUCCESS;
}

// This variant is used to set the lpf spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueLPFSpec(const tdi::DataFieldInfo &field,
                                             const DataFieldType &field_type,
                                             const pipe_tbl_hdl_t &tbl_hdl,
                                             const uint64_t &value,
                                             const uint8_t *value_ptr) {
  uint64_t lpf_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &lpf_val);
  } else {
    lpf_val = value;
  }

  return setValueLPFSpecHelper<uint64_t>(field_type, tbl_hdl, lpf_val);
}

// This variant is used to set the lpf spec in the data object by the user
// and also in the pipe action spec
template <typename T>
tdi_status_t PipeActionSpec::setValueLPFSpec(const DataFieldType &field_type,
                                             const pipe_tbl_hdl_t &tbl_hdl,
                                             const T &value) {
  return setValueLPFSpecHelper<T>(field_type, tbl_hdl, value);
}

tdi_status_t PipeActionSpec::getValueLPFSpec(const tdi::DataFieldInfo &field,
                                             const DataFieldType &field_type,
                                             uint64_t *value,
                                             uint8_t *value_ptr) const {
  uint64_t lpf_val;
  // populate the value from lpf spec
  lpf_spec.getLPFDataFromValue<uint64_t>(field_type, &lpf_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, lpf_val, value_ptr);
  } else {
    *value = lpf_val;
  }

  return TDI_SUCCESS;
}

template <typename T>
tdi_status_t PipeActionSpec::getValueLPFSpec(const DataFieldType &field_type,
                                             T *value) const {
  return lpf_spec.getLPFDataFromValue<T>(field_type, value);
}

template <typename T>
tdi_status_t PipeActionSpec::setValueWREDSpecHelper(
    const DataFieldType &field_type,
    const pipe_tbl_hdl_t &tbl_hdl,
    const T &value) {
  // populate the wred spec
  wred_spec.setWREDDataFromValue<T>(field_type, value);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // populate the pipe action spec
  switch (field_type) {
    case WRED_SPEC_MIN_THRESHOLD:
      res_spec->data.red.red_min_threshold = static_cast<uint32_t>(value);
      break;
    case WRED_SPEC_MAX_THRESHOLD:
      res_spec->data.red.red_max_threshold = static_cast<uint32_t>(value);
      break;
    case WRED_SPEC_TIME_CONSTANT:
      res_spec->data.red.time_constant = static_cast<float>(value);
      break;
    case WRED_SPEC_MAX_PROBABILITY:
      res_spec->data.red.max_probability = static_cast<float>(value);
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "wred data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the wred spec
// in the data object with the wred spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueWREDSpec(const pipe_wred_spec_t &wred) {
  // populate the internal wred spec
  wred_spec.setWREDDataFromWREDSpec(&wred);

  return TDI_SUCCESS;
}

// This variant is used to set the wred spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueWREDSpec(const tdi::DataFieldInfo &field,
                                              const DataFieldType &field_type,
                                              const pipe_tbl_hdl_t &tbl_hdl,
                                              const uint64_t &value,
                                              const uint8_t *value_ptr) {
  uint64_t wred_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &wred_val);
  } else {
    wred_val = value;
  }

  return setValueWREDSpecHelper<uint64_t>(field_type, tbl_hdl, wred_val);
}

// This variant is used to set the wred spec in the data object by the user
// and also in the pipe action spec
template <typename T>
tdi_status_t PipeActionSpec::setValueWREDSpec(const DataFieldType &field_type,
                                              const pipe_tbl_hdl_t &tbl_hdl,
                                              const T &value) {
  return setValueWREDSpecHelper<T>(field_type, tbl_hdl, value);
}

tdi_status_t PipeActionSpec::getValueWREDSpec(const tdi::DataFieldInfo &field,
                                              const DataFieldType &field_type,
                                              uint64_t *value,
                                              uint8_t *value_ptr) const {
  uint64_t wred_val = 0;
  // populate the value from wred spec
  wred_spec.getWREDDataFromValue<uint64_t>(field_type, &wred_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, wred_val, value_ptr);
  } else {
    *value = wred_val;
  }
  return TDI_SUCCESS;
}

template <typename T>
tdi_status_t PipeActionSpec::getValueWREDSpec(const DataFieldType &field_type,
                                              T *value) const {
  return wred_spec.getWREDDataFromValue<T>(field_type, value);
}

tdi_status_t PipeActionSpec::setValueActionDataHdlType() {
  action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::setValueSelectorGroupHdlType() {
  action_spec.pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
  return TDI_SUCCESS;
}

// This function will return a non null resource_spec if we have already
// inserted the said resource spec handle in the action spec. Thus all
// the functions trying to just query the resource spec for a given resource
// handle should always expect a non null resource spec and should error
// otherwise. On the other hand, when trying to add a new resource handle in
// the action spec, resource spec returned should be null. Thus all the
// functions trying to insert a resource handle in the action spec should
// expect a null resource spec returned from this function and then insert
// the resource handle in the next unused location
pipe_res_spec_t *PipeActionSpec::getResourceSpec(
    const pipe_tbl_hdl_t &tbl_hdl) {
  pipe_res_spec_t *res_spec = NULL;
  for (auto i = 0; i < action_spec.resource_count; i++) {
    res_spec = &action_spec.resources[i];
    if (res_spec->tbl_hdl == tbl_hdl) {
      break;
    } else {
      res_spec = NULL;
    }
  }
  return res_spec;
}

const pipe_res_spec_t *PipeActionSpec::getResourceSpec(
    const pipe_tbl_hdl_t &tbl_hdl) const {
  const pipe_res_spec_t *res_spec = NULL;
  for (auto i = 0; i < action_spec.resource_count; i++) {
    res_spec = &action_spec.resources[i];
    if (res_spec->tbl_hdl == tbl_hdl) {
      break;
    } else {
      res_spec = NULL;
    }
  }
  return res_spec;
}

void PipeActionSpec::updateResourceSpec(const pipe_tbl_hdl_t &tbl_hdl,
                                        const bool &&is_direct_resource,
                                        pipe_res_spec_t **res_spec) {
  *res_spec = getResourceSpec(tbl_hdl);
  if (!(*res_spec)) {
    *res_spec = &action_spec.resources[action_spec.resource_count++];
    if (is_direct_resource) {
      this->num_direct_resources++;
    } else {
      this->num_indirect_resources++;
    }
  }
}

// RESOURCE SPEC DATA
// REGISTER TABLE
tdi_status_t RegisterSpec::setDataFromStfulSpec(
    const pipe_stful_mem_spec_t &stful_spec) {
  memcpy(&stful, &stful_spec, sizeof(stful));
  return TDI_SUCCESS;
}

tdi_status_t RegisterSpec::getStfulSpecFromData(
    pipe_stful_mem_spec_t *stful_spec) const {
  memcpy(stful_spec, &stful, sizeof(*stful_spec));
  return TDI_SUCCESS;
}

tdi_status_t RegisterSpec::setData(const tdi::DataFieldInfo &tableDataField,
                                   uint64_t value) {
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField.dataFieldContextInfoGet())
                        ->typesGet();
  size_t field_size = tableDataField.sizeGet();
  // Register table data fields have to be byte alighned except for 1 bit mode
  if (field_size != 1) {
    TDI_ASSERT(field_size % 8 == 0);
  }

  bool hi_value = false;
  bool lo_value = false;

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::REGISTER_SPEC_HI:
        hi_value = true;
        break;
      case DataFieldType::REGISTER_SPEC_LO:
        lo_value = true;
        break;
      case DataFieldType::REGISTER_SPEC:
        break;
      default:
        LOG_ERROR("%s:%d ERROR : Invalid data field type for field id %d",
                  __func__,
                  __LINE__,
                  tableDataField.idGet());
        TDI_ASSERT(0);
    }
  }
  switch (field_size) {
    case 1:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      stful.bit = (value & 0x1);
      break;
    case 64:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      stful.dbl_word.hi = value >> 32;
      stful.dbl_word.lo = value & 0xffffffff;
      break;
    case 8:
      if (hi_value) {
        stful.dbl_byte.hi = (value & 0xff);
      } else if (lo_value) {
        stful.dbl_byte.lo = (value & 0xff);
      } else {
        stful.byte = (value & 0xff);
      }
      break;
    case 16:
      if (hi_value) {
        stful.dbl_half.hi = (value & 0xffff);
      } else if (lo_value) {
        stful.dbl_half.lo = (value & 0xffff);
      } else {
        stful.half = (value & 0xffff);
      }
      break;
    case 32:
      if (hi_value) {
        stful.dbl_word.hi = (value & 0xffffffff);
      } else if (lo_value) {
        stful.dbl_word.lo = (value & 0xffffffff);
      } else {
        stful.word = (value & 0xffffffff);
      }
      break;
    default:
      LOG_ERROR("%s:%d ERROR : Invalid field size encountered for field id %d",
                __func__,
                __LINE__,
                tableDataField.idGet());
      TDI_ASSERT(0);
  }
  return TDI_SUCCESS;
}

uint64_t RegisterSpec::getData(const tdi::DataFieldInfo &tableDataField) const {
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField.dataFieldContextInfoGet())
                        ->typesGet();
  size_t field_size = tableDataField.sizeGet();
  // Register table data fields have to be byte alighned except for 1 bit mode
  if (field_size != 1) {
    TDI_ASSERT(field_size % 8 == 0);
  }

  bool hi_value = false;
  bool lo_value = false;

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::REGISTER_SPEC_HI:
        hi_value = true;
        break;
      case DataFieldType::REGISTER_SPEC_LO:
        lo_value = true;
        break;
      case DataFieldType::REGISTER_SPEC:
        break;
      default:
        LOG_ERROR("%s:%d ERROR :Invalid data field type for field id %d",
                  __func__,
                  __LINE__,
                  tableDataField.idGet());
        TDI_ASSERT(0);
    }
  }

  switch (field_size) {
    case 1:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      return stful.bit;
    case 8:
      if (hi_value) {
        return stful.dbl_byte.hi;
      } else if (lo_value) {
        return stful.dbl_byte.lo;
      } else {
        return stful.byte;
      }
    case 16:
      if (hi_value) {
        return stful.dbl_half.hi;
      } else if (lo_value) {
        return stful.dbl_half.lo;
      } else {
        return stful.half;
      }
    case 32:
      if (hi_value) {
        return stful.dbl_word.hi;
      } else if (lo_value) {
        return stful.dbl_word.lo;
      } else {
        return stful.word;
      }
    case 64: {
      return ((uint64_t)stful.dbl_word.hi << 32) | stful.dbl_word.lo;
    }
    default:
      LOG_ERROR("%s:%d ERROR : Invalid field size encountered for field id %d",
                __func__,
                __LINE__,
                tableDataField.idGet());
      TDI_ASSERT(0);
  }
  return 0;
}

// This function is used to set a register and add it to the vector. This
// register will then be retrieved by populateStfulSpecFromData to set the
// set the stful mem spec while adding entry into the table.
void RegisterSpecData::addRegisterToVec(
    const tdi::DataFieldInfo &tableDataField, const uint64_t &value) {
  if (registers_.size()) {
    auto &tdi_register = registers_[0];
    tdi_register.setData(tableDataField, value);
  } else {
    RegisterSpec tdi_register;
    tdi_register.setData(tableDataField, value);
    registers_.push_back(tdi_register);
  }
}

void RegisterSpecData::setFirstRegister(
    const tdi::DataFieldInfo &tableDataField, const uint64_t &value) {
  if (registers_.empty()) {
    RegisterSpec tdi_register;
    tdi_register.setData(tableDataField, value);
    registers_.push_back(tdi_register);
  } else {
    auto &tdi_register = registers_[0];
    tdi_register.setData(tableDataField, value);
  }
}

void RegisterSpecData::populateDataFromStfulSpec(
    const std::vector<pipe_stful_mem_spec_t> &stful_spec, uint32_t size) {
  for (uint32_t i = 0; i < size && i < stful_spec.size(); i++) {
    RegisterSpec tdi_register;
    tdi_register.setDataFromStfulSpec(stful_spec[i]);
    registers_.push_back(tdi_register);
  }
}

void RegisterSpecData::populateStfulSpecFromData(
    pipe_stful_mem_spec_t *stful_spec) const {
  if (!registers_.size()) {
    LOG_ERROR("%s:%d Trying to populate stful spec from an empty register data",
              __func__,
              __LINE__);
    TDI_ASSERT(0);
  }

  // This function is called when we want to populate the stful spec so as to
  // program the pipe mgr during tableEntryAdd. As a result it only makes
  // sense to have one register in the vector. Otherwise which one should be
  // picked to program the pipe mgr? Thus add safety assert here
  if (registers_.size() != 1) {
    LOG_ERROR(
        "%s:%d Cannot have multiple register populated in the vector during "
        "entry add",
        __func__,
        __LINE__);
    TDI_ASSERT(0);
  }
  const auto &register_data = registers_[0];
  register_data.getStfulSpecFromData(stful_spec);
}

void CounterSpecData::setCounterDataFromCounterSpec(
    const pipe_stat_data_t &counter) {
  std::memcpy(&counter_data, &counter, sizeof(counter_data));
}

void CounterSpecData::setCounterDataFromValue(const DataFieldType &field_type,
                                              const uint64_t &value) {
  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      counter_data.bytes = value;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      counter_data.packets = value;
      break;
    default:
      LOG_ERROR("%s:%d Invalid Field type %d for counter data",
                __func__,
                __LINE__,
                static_cast<int>(field_type));
      TDI_ASSERT(0);
  }
}

void CounterSpecData::getCounterData(const DataFieldType &field_type,
                                     uint64_t *value) const {
  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      *value = counter_data.bytes;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      *value = counter_data.packets;
      break;
    default:
      LOG_ERROR("%s:%d Invalid Field type %d for counter data",
                __func__,
                __LINE__,
                static_cast<int>(field_type));
      TDI_ASSERT(0);
  }
}

void MeterSpecData::setMeterDataFromValue(const DataFieldType &fieldType,
                                          const uint64_t &value) {
  switch (fieldType) {
    case METER_SPEC_CIR_PPS:
      setCIRPps(value);
      break;
    case METER_SPEC_PIR_PPS:
      setPIRPps(value);
      break;
    case METER_SPEC_CBS_PKTS:
      setCBSPkts(value);
      break;
    case METER_SPEC_PBS_PKTS:
      setPBSPkts(value);
      break;
    case METER_SPEC_CIR_KBPS:
      setCIRKbps(value);
      break;
    case METER_SPEC_PIR_KBPS:
      setPIRKbps(value);
      break;
    case METER_SPEC_CBS_KBITS:
      setCBSKbits(value);
      break;
    case METER_SPEC_PBS_KBITS:
      setPBSKbits(value);
      break;
    default:
      TDI_ASSERT(0);
      break;
  }
}

void MeterSpecData::setMeterDataFromMeterSpec(const pipe_meter_spec_t &mspec) {
  if (mspec.cir.type == METER_RATE_TYPE_KBPS) {
    TDI_ASSERT(mspec.pir.type == METER_RATE_TYPE_KBPS);
    setCIRKbps(mspec.cir.value.kbps);
    setPIRKbps(mspec.pir.value.kbps);
    setCBSKbits(mspec.cburst);
    setPBSKbits(mspec.pburst);
  } else if (mspec.cir.type == METER_RATE_TYPE_PPS) {
    TDI_ASSERT(mspec.pir.type == METER_RATE_TYPE_PPS);
    setCIRPps(mspec.cir.value.pps);
    setPIRPps(mspec.pir.value.pps);
    setCBSPkts(mspec.cburst);
    setPBSPkts(mspec.pburst);
  } else {
    TDI_ASSERT(0);
  }
}

void MeterSpecData::getMeterDataFromValue(const DataFieldType &fieldType,
                                          uint64_t *value) const {
  switch (fieldType) {
    case METER_SPEC_CIR_PPS:
      *value = pipe_meter_spec.cir.value.pps;
      break;
    case METER_SPEC_PIR_PPS:
      *value = pipe_meter_spec.pir.value.pps;
      break;
    case METER_SPEC_CBS_PKTS:
      *value = pipe_meter_spec.cburst;
      break;
    case METER_SPEC_PBS_PKTS:
      *value = pipe_meter_spec.pburst;
      break;
    case METER_SPEC_CIR_KBPS:
      *value = pipe_meter_spec.cir.value.kbps;
      break;
    case METER_SPEC_PIR_KBPS:
      *value = pipe_meter_spec.pir.value.kbps;
      break;
    case METER_SPEC_CBS_KBITS:
      *value = pipe_meter_spec.cburst;
      break;
    case METER_SPEC_PBS_KBITS:
      *value = pipe_meter_spec.pburst;
      break;
    default:
      TDI_ASSERT(0);
      break;
  }
}

template <class T>
tdi_status_t LPFSpecData::setLPFDataFromValue(const DataFieldType &fieldType,
                                              const T &value) {
  switch (fieldType) {
    case DataFieldType::LPF_SPEC_TYPE:
      if (value == LPF_TYPE_RATE) {
        setRateEnable(true);
      } else if (value == LPF_TYPE_SAMPLE) {
        setRateEnable(false);
      } else {
        LOG_ERROR(
            "%s:%d ERROR : Enumvalue passed is not applicable for "
            "LPF_TPYE ENUM value %" PRIu64,
            __func__,
            __LINE__,
            static_cast<uint64_t>(value));
        return TDI_INVALID_ARG;
      }
      break;
    case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR:
      setOutputScaleDownFactor(static_cast<uint32_t>(value));
      break;
    case DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT:
      setGainTimeConstant(static_cast<float>(value));
      break;
    case DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT:
      setDecayTimeConstant(static_cast<float>(value));
      break;
    default:
      LOG_ERROR("%s:%d ERROR : This setter cannot be used for field type %d",
                __func__,
                __LINE__,
                static_cast<int>(fieldType));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

void LPFSpecData::setLPFDataFromLPFSpec(const pipe_lpf_spec_t *lpf_spec) {
  bool rateEnable = (lpf_spec->lpf_type == LPF_TYPE_RATE) ? true : false;
  setRateEnable(rateEnable);
  float gainTime_constant = 0;
  float decayTime_constant = 0;
  if (lpf_spec->gain_decay_separate_time_constant) {
    gainTime_constant = lpf_spec->gain_time_constant;
    decayTime_constant = lpf_spec->decay_time_constant;
  } else {
    gainTime_constant = lpf_spec->time_constant;
    decayTime_constant = lpf_spec->time_constant;
  }
  setGainTimeConstant(gainTime_constant);
  setDecayTimeConstant(decayTime_constant);
  setOutputScaleDownFactor(lpf_spec->output_scale_down_factor);
}

// Dangerous function. Incompatible dataFieldType and T cannot be checked here.
template <class T>
tdi_status_t LPFSpecData::getLPFDataFromValue(const DataFieldType &fieldType,
                                              T *value) const {
  switch (fieldType) {
    case DataFieldType::LPF_SPEC_TYPE:
      if (pipe_lpf_spec.lpf_type == LPF_TYPE_RATE) {
        *value = LPF_TYPE_RATE;
      } else {
        *value = LPF_TYPE_SAMPLE;
      }
      break;
    case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR:
      *reinterpret_cast<uint32_t *>(value) =
          pipe_lpf_spec.output_scale_down_factor;
      break;
    case DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT:
      *reinterpret_cast<float *>(value) = pipe_lpf_spec.gain_time_constant;
      break;
    case DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT:
      *reinterpret_cast<float *>(value) = pipe_lpf_spec.decay_time_constant;
      break;
    default:
      LOG_ERROR("%s:%d ERROR : Invalid getter for field type %d",
                __func__,
                __LINE__,
                static_cast<int>(fieldType));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t WREDSpecData::setWREDDataFromValue(const DataFieldType &fieldType,
                                                const T &value) {
  switch (fieldType) {
    case WRED_SPEC_MIN_THRESHOLD:
      setMinThreshold(value);
      break;
    case WRED_SPEC_MAX_THRESHOLD:
      setMaxThreshold(value);
      break;
    case WRED_SPEC_TIME_CONSTANT:
      setTimeConstant(value);
      break;
    case WRED_SPEC_MAX_PROBABILITY:
      setMaxProbability(value);
      break;
    default:
      LOG_ERROR("%s:%d ERROR : This setter cannot be used for field type %d",
                __func__,
                __LINE__,
                static_cast<int>(fieldType));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

void WREDSpecData::setWREDDataFromWREDSpec(const pipe_wred_spec_t *wred_spec) {
  setTimeConstant(wred_spec->time_constant);
  setMinThreshold(wred_spec->red_min_threshold);
  setMaxThreshold(wred_spec->red_max_threshold);
  setMaxProbability(wred_spec->max_probability);
}

template <class T>
tdi_status_t WREDSpecData::getWREDDataFromValue(const DataFieldType &fieldType,
                                                T *value) const {
  switch (fieldType) {
    case WRED_SPEC_MIN_THRESHOLD:
      *value = pipe_wred_spec.red_min_threshold;
      break;
    case WRED_SPEC_MAX_THRESHOLD:
      *value = pipe_wred_spec.red_max_threshold;
      break;
    case WRED_SPEC_TIME_CONSTANT:
      *value = pipe_wred_spec.time_constant;
      break;
    case WRED_SPEC_MAX_PROBABILITY:
      *value = pipe_wred_spec.max_probability;
      break;
    default:
      LOG_ERROR("%s:%d ERROR : Invalid getter for field type %d",
                __func__,
                __LINE__,
                fieldType);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

// MATCH ACTION TABLE DATA

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *ptr,
                                            const size_t &s) {
  return this->setValueInternal(field_id, 0, ptr, s);
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const std::string &value) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  const auto &fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                               tableDataField->dataFieldContextInfoGet())
                               ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::LPF_SPEC_TYPE: {
        pipe_lpf_type_e lpf_val;
        tdi_status_t sts =
            DataFieldStringMapper::lpfSpecTypeFromStringGet(value, &lpf_val);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s ERROR : Trying to get lpf spec type value from invalid "
              "string %s for field %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              value.c_str(),
              field_id);
          return sts;
        }
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueLPFSpec<pipe_lpf_type_e>(
            fieldType, res_hdl, lpf_val);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s ERROR : Unable to set LPF spec type for field %d",
                    __func__,
                    __LINE__,
                    this->table_->tableInfoGet()->nameGet().c_str(),
                    field_id);
          return sts;
        }
        break;
      }
      case DataFieldType::ENTRY_HIT_STATE: {
        tdi_status_t sts =
            DataFieldStringMapper::idleHitStateFromString(value, &hit_state);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s ERROR : Trying to get hit state type value from "
              "invalid "
              "string %s for field %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              value.c_str(),
              field_id);
          return sts;
        }
        break;
      }
      default: {
        LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_INVALID_ARG;
      }
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const float &value) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT:
      case DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        return getPipeActionSpecObj().setValueLPFSpec(
            fieldType, res_hdl, value);
      }
      case DataFieldType::WRED_SPEC_TIME_CONSTANT:
      case DataFieldType::WRED_SPEC_MAX_PROBABILITY: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        return getPipeActionSpecObj().setValueWREDSpec(
            fieldType, res_hdl, value);
      }
      default: {
        LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_UNEXPECTED;
      }
    }
  }
  return TDI_SUCCESS;
}

// Register values are returned one instance per pipe and the stage the table
// lives in. Thus if one wants to query a register for a table which is in
// 4 pipes and 1 stage in each pipe, then the returned vector will have 4
// elements.
tdi_status_t MatchActionTableData::getValue(
    const tdi_id_t &field_id, std::vector<uint64_t> *value) const {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  size_t field_size = tableDataField->sizeGet();
  if (field_size > 32) {
    LOG_ERROR(
        "ERROR %s:%d %s ERROR : This getter cannot be used since field size of "
        "%zu "
        "is > 32 bits, for field id %d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        field_size,
        field_id);
    return TDI_INVALID_ARG;
  }

  getPipeActionSpecObj().getValueRegisterSpec(*tableDataField, value);

  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::getValueInternal(const tdi_id_t &field_id,
                                                    uint64_t *value,
                                                    uint8_t *value_ptr,
                                                    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  auto mat_table = static_cast<const MatchActionDirect *>(this->table_);
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  uint64_t val = 0;
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::METER_SPEC_CIR_PPS:
      case DataFieldType::METER_SPEC_PIR_PPS:
      case DataFieldType::METER_SPEC_CBS_PKTS:
      case DataFieldType::METER_SPEC_PBS_PKTS:
      case DataFieldType::METER_SPEC_CIR_KBPS:
      case DataFieldType::METER_SPEC_PIR_KBPS:
      case DataFieldType::METER_SPEC_CBS_KBITS:
      case DataFieldType::METER_SPEC_PBS_KBITS: {
        return getPipeActionSpecObj().getValueMeterSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR: {
        return getPipeActionSpecObj().getValueLPFSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::WRED_SPEC_MIN_THRESHOLD:
      case DataFieldType::WRED_SPEC_MAX_THRESHOLD: {
        return getPipeActionSpecObj().getValueWREDSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::COUNTER_SPEC_BYTES:
      case DataFieldType::COUNTER_SPEC_PACKETS: {
        return getPipeActionSpecObj().getValueCounterSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::TTL: {
        if (mat_table->idleTablePollMode()) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Idle table is in Poll mode. TTL get is not"
              " applicable. Use ENTRY_HIT_STATE field instead",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
          return TDI_NOT_SUPPORTED;
        }
        val = ttl;
        if (value_ptr) {
          utils::TableFieldUtils::toNetworkOrderData(
              *tableDataField, val, value_ptr);
        } else {
          *value = val;
        }
        return TDI_SUCCESS;
      }
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_PARAM: {
        return getPipeActionSpecObj().getValueActionParam(
            *tableDataField, value, value_ptr);
      }
      case DataFieldType::COUNTER_INDEX:
      case DataFieldType::REGISTER_INDEX:
      case DataFieldType::METER_INDEX:
      case DataFieldType::LPF_INDEX:
      case DataFieldType::WRED_INDEX: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        return getPipeActionSpecObj().getValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
      }
      case DataFieldType::ACTION_MEMBER_ID:
        val = get_action_member_id();
        if (value_ptr) {
          utils::TableFieldUtils::toNetworkOrderData(
              *tableDataField, val, value_ptr);
        } else {
          *value = val;
        }
        return TDI_SUCCESS;
      case DataFieldType::SELECTOR_GROUP_ID:
        val = get_selector_group_id();
        if (value_ptr) {
          utils::TableFieldUtils::toNetworkOrderData(
              *tableDataField, val, value_ptr);
        } else {
          *value = val;
        }
        return TDI_SUCCESS;
      case DataFieldType::REGISTER_SPEC:
      case DataFieldType::REGISTER_SPEC_HI:
      case DataFieldType::REGISTER_SPEC_LO:
        LOG_ERROR("%s:%d This getter cannot be used for register fields",
                  __func__,
                  __LINE__);
        return TDI_INVALID_ARG;
      default:
        LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
        return TDI_NOT_SUPPORTED;
    }
  }
  return TDI_OBJECT_NOT_FOUND;
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return this->getValueInternal(field_id, nullptr, value, size);
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            std::string *value) const {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto mat_table = static_cast<const MatchActionDirect *>(this->table_);
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::LPF_SPEC_TYPE: {
        pipe_lpf_type_e lpf_val;
        tdi_status_t sts =
            getPipeActionSpecObj().getValueLPFSpec<pipe_lpf_type_e>(fieldType,
                                                                    &lpf_val);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s ERROR : Unable to get value for field %d",
                    __func__,
                    __LINE__,
                    this->table_->tableInfoGet()->nameGet().c_str(),
                    field_id);
          return sts;
        }
        sts = DataFieldStringMapper::lpfStringFromSpecTypeGet(lpf_val, value);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s ERROR : Trying to get String from invalid lpf spec "
              "type value %d for field %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              lpf_val,
              field_id);
          return sts;
        }
        break;
      }
      case DataFieldType::ENTRY_HIT_STATE: {
        if (!mat_table->idleTablePollMode()) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Idle table is not in Poll mode. ENTRY HIT "
              "STATE get is not"
              "applicable. Use ENTRY_TTL field instead",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
          return TDI_NOT_SUPPORTED;
        }
        // Derive the hit state from member pipe_idle_time_hit_state_e
        tdi_status_t sts =
            DataFieldStringMapper::idleHitStateToString(hit_state, value);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s ERROR : Trying to get String from invalid entry hit "
              "state value %d for field %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              hit_state,
              field_id);
          return sts;
        }
        break;
      }
      default: {
        LOG_ERROR("%s:%d %s ERROR : This getter cannot be used for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_UNEXPECTED;
      }
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            float *value) const {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT:
      case DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT: {
        return getPipeActionSpecObj().getValueLPFSpec<float>(fieldType, value);
      }
      case DataFieldType::WRED_SPEC_TIME_CONSTANT:
      case DataFieldType::WRED_SPEC_MAX_PROBABILITY: {
        return getPipeActionSpecObj().getValueWREDSpec<float>(fieldType, value);
      }
      default: {
        LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_UNEXPECTED;
      }
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::setValueInternal(const tdi_id_t &field_id,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr,
                                                    const size_t &s) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto mat_table = static_cast<const MatchActionDirect *>(this->table_);
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  auto types_vec = static_cast<const TofinoDataFieldContextInfo *>(
                       tableDataField->dataFieldContextInfoGet())
                       ->typesGet();
  size_t field_size = tableDataField->sizeGet();

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }

  sts = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d "
        "action id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet(),
        this->actionIdGet());
    return sts;
  }
  // const auto &oneof_siblings = tableDataField->oneofSiblingsGet();

  uint64_t val = 0;
  for (const auto &fieldType : types_vec) {
    switch (fieldType) {
      case (DataFieldType::ACTION_PARAM): {
        // Set the action param
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, value, value_ptr);
        break;
      }
      case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
        // When the action param is optimized out from the context json,
        // we need to just send 0s to the pipe mgr.
        LOG_WARN(
            "WARNING: %s:%d %s : Trying to set value for an optimized out "
            "field with id %d action id %d; Ignoring the user value and "
            "setting the field to zeros",
            __func__,
            __LINE__,
            this->table_->tableInfoGet()->nameGet().c_str(),
            tableDataField->idGet(),
            this->actionIdGet());
        std::vector<uint8_t> data_arr((tableDataField->sizeGet() + 7) / 8, 0);
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, 0, data_arr.data());
        break;
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX):
      case (DataFieldType::LPF_INDEX):
      case (DataFieldType::WRED_INDEX): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
        break;
      }
      case (DataFieldType::COUNTER_SPEC_BYTES):
      case (DataFieldType::COUNTER_SPEC_PACKETS): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueCounterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }

      case (DataFieldType::REGISTER_SPEC):
      case (DataFieldType::REGISTER_SPEC_HI):
      case (DataFieldType::REGISTER_SPEC_LO): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueRegisterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr, field_size);
        break;
      }
      case (DataFieldType::METER_SPEC_CIR_PPS):
      case (DataFieldType::METER_SPEC_PIR_PPS):
      case (DataFieldType::METER_SPEC_CBS_PKTS):
      case (DataFieldType::METER_SPEC_PBS_PKTS):
      case (DataFieldType::METER_SPEC_CIR_KBPS):
      case (DataFieldType::METER_SPEC_PIR_KBPS):
      case (DataFieldType::METER_SPEC_CBS_KBITS):
      case (DataFieldType::METER_SPEC_PBS_KBITS): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueMeterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }
      case DataFieldType::ACTION_MEMBER_ID: {
        auto act_data_type_bitmap =
            getPipeActionSpecObj().getPipeActionDatatypeBitmap();
        if (act_data_type_bitmap == PIPE_SEL_GRP_HDL_TYPE) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Groupr ID is set probably as %d"
              ". Cannot set Action member ID",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              this->group_id);
          return TDI_INVALID_ARG;
        }
        sts = getPipeActionSpecObj().setValueActionDataHdlType();
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &val);
        } else {
          val = value;
        }
        set_action_member_id(val);
        // Remove oneof sibling from active fields
        // this->removeActiveFields(oneof_siblings);
        break;
      }
      case DataFieldType::SELECTOR_GROUP_ID: {
        auto act_data_type_bitmap =
            getPipeActionSpecObj().getPipeActionDatatypeBitmap();
        if (act_data_type_bitmap == PIPE_ACTION_DATA_HDL_TYPE) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Action Member ID is set probably as %d"
              ". Cannot set Group ID",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              this->action_mbr_id);
          return TDI_INVALID_ARG;
        }
        sts = getPipeActionSpecObj().setValueSelectorGroupHdlType();
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &val);
        } else {
          val = value;
        }
        set_selector_group_id(val);
        // Remove oneof sibling from active fields
        // this->removeActiveFields(oneof_siblings);
        break;
      }
      case DataFieldType::TTL: {
        if (mat_table->idleTablePollMode()) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Idle table is in Poll mode. TTL set is not"
              " applicable",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
          return TDI_NOT_SUPPORTED;
        }
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &val);
        } else {
          val = value;
        }
        set_ttl(val);
        break;
      }
      case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueLPFSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }
      case DataFieldType::WRED_SPEC_MIN_THRESHOLD:
      case DataFieldType::WRED_SPEC_MAX_THRESHOLD: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueWREDSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }
      default:
        LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
        return TDI_INVALID_ARG;
    }
  }

  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to set data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
    return sts;
  }
  return TDI_SUCCESS;
}

void MatchActionTableData::set_action_member_id(uint64_t val) {
  action_mbr_id = (tdi_id_t)val;
}

void MatchActionTableData::set_selector_group_id(uint64_t val) {
  group_id = val;
}

void MatchActionTableData::set_ttl(uint64_t val) { ttl = val; }

void MatchActionTableData::initializeDataFields() {
  const auto &all_fields =
      this->table_->tableInfoGet()->dataFieldIdListGet(this->actionIdGet());

  // Count the number of action only fields. This is equal to
  // num(all_fields) - num(common_fields)
  this->num_action_only_fields =
      all_fields.size() - this->table_->tableInfoGet()->name_data_map_.size();

  // Count the number of direct and indirect resources assoicated with
  // this data object for all fields because we do not want to miss out
  // on checking any fields.
  this->getIndirectResourceCounts(all_fields);

  for (const auto &each_field : all_fields) {
    auto *tableDataField = this->table_->tableInfoGet()->dataFieldGet(
        each_field, this->actionIdGet());
    if (!tableDataField) {
      LOG_ERROR("%s:%d %s ERROR in getting data field info  for action id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                this->actionIdGet());
      TDI_DBGCHK(0);
      return;
    }

    auto types_vec = static_cast<const TofinoDataFieldContextInfo *>(
                         tableDataField->dataFieldContextInfoGet())
                         ->typesGet();
    for (const auto &fieldType : types_vec) {
      if (initialization_required(fieldType)) {
        // initialize_data_field(each_field, *table_, this->actionIdGet(),
        // this);
      }
    }
  }
  return;
}

void MatchActionTableData::getIndirectResourceCounts(
    const std::vector<tdi_id_t> field_list) {
  for (auto each_field : field_list) {
    auto *tableDataField = this->table_->tableInfoGet()->dataFieldGet(
        each_field, this->actionIdGet());
    if (!tableDataField) {
      LOG_ERROR("%s:%d %s ERROR in getting data field info  for action id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                this->actionIdGet());
      TDI_DBGCHK(0);
      return;
    }
    auto types_vec = static_cast<const TofinoDataFieldContextInfo *>(
                         tableDataField->dataFieldContextInfoGet())
                         ->typesGet();
    this->num_indirect_resource_count +=
        std::count_if(types_vec.begin(), types_vec.end(), is_indirect_resource);
  }
}

pipe_act_fn_hdl_t MatchActionTableData::getActFnHdl() const {
  auto action = this->table_->tableInfoGet()->actionGet(this->actionIdGet());
  if (!action || !action->actionContextInfoGet()) return 0;
  auto tofino_action_context = static_cast<const TofinoActionContextInfo *>(
      action->actionContextInfoGet());
  return tofino_action_context->actionFnHdlGet();
}

tdi_status_t MatchActionTableData::reset(const tdi_id_t &act_id,
                                         const tdi_id_t & /*container_id*/,
                                         const std::vector<tdi_id_t> &fields) {
  auto status = reset_action_data<MatchActionTableData>(act_id, this);
  if (status) return status;
  return tdi::TableData::reset(act_id, 0, fields);
}

// ACTION TABLE DATA

tdi_status_t ActionProfileData::setValue(const tdi_id_t &field_id,
                                         const uint64_t &value) {
  auto status = this->setValueInternal(field_id, value, nullptr, 0);

  return status;
}

tdi_status_t ActionProfileData::setValue(const tdi_id_t &field_id,
                                         const uint8_t *ptr,
                                         const size_t &size) {
  auto status = this->setValueInternal(field_id, 0, ptr, size);

  return status;
}

tdi_status_t ActionProfileData::getValueInternal(const tdi_id_t &field_id,
                                                 uint64_t *value,
                                                 uint8_t *value_ptr,
                                                 const size_t &s) const {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  auto adt_context_info = static_cast<const ActionProfileContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_PARAM: {
        return getPipeActionSpecObj().getValueActionParam(
            *tableDataField, value, value_ptr);
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX):
      case (DataFieldType::LPF_INDEX):
      case (DataFieldType::WRED_INDEX): {
        pipe_tbl_hdl_t res_hdl = adt_context_info->resourceHdlGet(fieldType);
        return getPipeActionSpecObj().getValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
      }
      default:
        LOG_ERROR("ERROR: %s:%d %s : This API is not supported for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_INVALID_ARG;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionProfileData::getValue(const tdi_id_t &field_id,
                                         uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t ActionProfileData::getValue(const tdi_id_t &field_id,
                                         const size_t &size,
                                         uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t ActionProfileData::setValueInternal(const tdi_id_t &field_id,
                                                 const uint64_t &value,
                                                 const uint8_t *value_ptr,
                                                 const size_t &s) {
  auto tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  sts = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Input Param bounds check failed for field id %d action id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet(),
        this->actionIdGet());
    return sts;
  }

  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  auto adt_context_info = static_cast<const ActionProfileContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case (DataFieldType::ACTION_PARAM): {
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, value, value_ptr);
        break;
      }
      case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
        // When the action param is optimized out from the context json,
        // we need to just send 0s to the pipe mgr.
        LOG_WARN(
            "WARNING: %s:%d %s : Trying to set value for an optimized out "
            "field with id %d action id %d; Ignoring the user value and "
            "setting the field to zeros",
            __func__,
            __LINE__,
            this->table_->tableInfoGet()->nameGet().c_str(),
            tableDataField->idGet(),
            this->actionIdGet());
        std::vector<uint8_t> data_arr((tableDataField->sizeGet() + 7) / 8, 0);
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, 0, data_arr.data());
        break;
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX):
      case (DataFieldType::LPF_INDEX):
      case (DataFieldType::WRED_INDEX): {
        pipe_tbl_hdl_t res_hdl = adt_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s : Unable set resource index for field with id %d",
                    __func__,
                    __LINE__,
                    this->table_->tableInfoGet()->nameGet().c_str(),
                    tableDataField->idGet());
          return sts;
        }
        // This is an indirect resource. Add this to the indirect resource Map.
        // This map is retrieved during MatchActionIndirect table entry add to
        // retrieve the indirect resources, since its required during match
        // entry add
        uint64_t resource_idx = 0;
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &resource_idx);
        } else {
          resource_idx = value;
        }
        auto elem = resource_map.find(fieldType);
        if (elem != resource_map.end()) {
          // This should not happen. This means that there are two action
          // parameters for the same indirect resource. NOT supported.
          TDI_ASSERT(0);
        }
        resource_map[fieldType] = resource_idx;
        break;
      }
      default:
        LOG_ERROR("%s:%d %s ERROR : This API is not supported for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_INVALID_ARG;
    }
  }
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to set data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
    return sts;
  }
  return TDI_SUCCESS;
}

pipe_act_fn_hdl_t ActionProfileData::getActFnHdl() const {
  auto action = this->table_->tableInfoGet()->actionGet(this->actionIdGet());
  if (!action || !action->actionContextInfoGet()) return 0;
  auto tofino_action_context = static_cast<const TofinoActionContextInfo *>(
      action->actionContextInfoGet());
  return tofino_action_context->actionFnHdlGet();
}

tdi_status_t ActionProfileData::reset(const tdi_id_t &act_id,
                                      const tdi_id_t & /*container_id*/,
                                      const std::vector<tdi_id_t> &fields) {
  auto status = reset_action_data<ActionProfileData>(act_id, this);
  if (status) return status;
  return tdi::TableData::reset(act_id, 0, fields);
}

// SELECTOR TABLE DATA
SelectorTableData::SelectorTableData(const tdi::Table *table,
                                     const std::vector<tdi_id_t> &fields)
    : tdi::TableData(table), act_fn_hdl_() {
  const auto max_grp_size_field =
      table->tableInfoGet()->dataFieldGet("$MAX_GROUP_SIZE");
  if (max_grp_size_field == nullptr) {
    LOG_ERROR("%s:%d ERROR Field Id for \"$MAX_GROUP_SIZE\" field not found",
              __func__,
              __LINE__);
    TDI_DBGCHK(0);
  }
  auto max_grp_size_id = max_grp_size_field->idGet();

  if (fields.empty() ||
      (std::find(fields.begin(), fields.end(), max_grp_size_id) !=
       fields.end())) {
    // Set the max group size to the default value parsed from tdi json
    // We don't need to worry about the members and member sts arrays because
    // they will be empty anyway by default
    uint64_t default_value = max_grp_size_field->defaultValueGet();
    auto sts =
        this->setValueInternal(max_grp_size_id, 0, default_value, nullptr);
    if (sts != TDI_SUCCESS) {
      // Unable to set the default value
      LOG_ERROR("%s:%d ERROR Unable to set the default value for field %d",
                __func__,
                __LINE__,
                max_grp_size_id);
      TDI_DBGCHK(0);
    }
  }
}

std::vector<tdi_id_t> SelectorTableData::get_members_from_array(
    const uint8_t *value, const size_t &size) {
  std::vector<tdi_id_t> grp_members;
  for (unsigned i = 0; i < size; i += sizeof(tdi_id_t)) {
    tdi_id_t this_member = *(reinterpret_cast<const tdi_id_t *>(value + i));
    this_member = ntohl(this_member);
    grp_members.push_back(this_member);
  }
  return grp_members;
}

tdi_status_t SelectorTableData::setValueInternal(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 const uint64_t &value,
                                                 const uint8_t *value_ptr) {
  uint64_t val = 0;
  std::set<DataFieldType> allowed_field_types = {DataFieldType::MAX_GROUP_SIZE};
  DataFieldType field_type;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  max_grp_size_ = static_cast<uint32_t>(val);
  return TDI_SUCCESS;
}

tdi_status_t SelectorTableData::setValue(const tdi_id_t &field_id,
                                         const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t SelectorTableData::setValue(const tdi_id_t &field_id,
                                         const uint8_t *value_ptr,
                                         const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t SelectorTableData::setValue(const tdi_id_t &field_id,
                                         const std::vector<tdi_id_t> &arr) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_INT_ARR) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  members_ = arr;
  return TDI_SUCCESS;
}

tdi_status_t SelectorTableData::setValue(const tdi_id_t &field_id,
                                         const std::vector<bool> &arr) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL_ARR) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool array",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  member_status_ = arr;

  return TDI_SUCCESS;
}

tdi_status_t SelectorTableData::getValueInternal(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 uint64_t *value,
                                                 uint8_t *value_ptr) const {
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {DataFieldType::MAX_GROUP_SIZE};
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  uint64_t grp_size = 0;
  grp_size = max_grp_size_;
  setInputValToOutputVal(*table_, field_id, grp_size, value, value_ptr);
  return TDI_SUCCESS;
}

tdi_status_t SelectorTableData::getValue(const tdi_id_t &field_id,
                                         uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t SelectorTableData::getValue(const tdi_id_t &field_id,
                                         const size_t &size,
                                         uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t SelectorTableData::getValue(const tdi_id_t &field_id,
                                         std::vector<tdi_id_t> *arr) const {
  tdi_status_t status = TDI_SUCCESS;
  // Get the data_field from the table
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType == DataFieldType::SELECTOR_MEMBERS &&
        tableDataField->dataTypeGet() == TDI_FIELD_DATA_TYPE_INT_ARR) {
      *arr = members_;
      status = TDI_SUCCESS;
    } else {
      LOG_ERROR(
          "%s:%d %s Field type other than SELECTOR_MEMBERS"
          " Not supported. Field type received %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          int(fieldType));
      status = TDI_NOT_SUPPORTED;
    }
  }
  return status;
}

tdi_status_t SelectorTableData::getValue(const tdi_id_t &field_id,
                                         std::vector<bool> *arr) const {
  tdi_status_t status = TDI_SUCCESS;
  // Get the data_field from the table
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType == DataFieldType::ACTION_MEMBER_STATUS &&
        tableDataField->dataTypeGet() == TDI_FIELD_DATA_TYPE_BOOL_ARR) {
      *arr = member_status_;
      status = TDI_SUCCESS;
    } else {
      LOG_ERROR(
          "%s:%d %s Field type other than ACTION_MEMBER_STATUS"
          " Not supported. Field type received %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          int(fieldType));
      status = TDI_NOT_SUPPORTED;
    }
  }
  return status;
}

tdi_status_t SelectorTableData::resetDerived() {
  act_fn_hdl_ = 0;
  members_.clear();
  member_status_.clear();
  max_grp_size_ = 0;
  return TDI_SUCCESS;
}

// Counter Table Data

tdi_status_t CounterIndirectTableData::setValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // of counter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  getCounterSpecObj().setCounterDataFromValue(field_type, val);
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirectTableData::setValue(const tdi_id_t &field_id,
                                                const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t CounterIndirectTableData::setValue(const tdi_id_t &field_id,
                                                const uint8_t *value_ptr,
                                                const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t CounterIndirectTableData::getValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    uint64_t *value,
    uint8_t *value_ptr) const {
  uint64_t counter = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // of counter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  getCounterSpecObj().getCounterData(field_type, &counter);
  setInputValToOutputVal(*table_, field_id, counter, value, value_ptr);

  return TDI_SUCCESS;
}

tdi_status_t CounterIndirectTableData::getValue(const tdi_id_t &field_id,
                                                uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t CounterIndirectTableData::getValue(const tdi_id_t &field_id,
                                                const size_t &size,
                                                uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t CounterIndirectTableData::resetDerived() {
  counter_spec_.reset();
  return TDI_SUCCESS;
}

// METER TABLE DATA

tdi_status_t MeterIndirectTableData::setValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under meter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  meter_spec_.setMeterDataFromValue(field_type, val);
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirectTableData::setValue(const tdi_id_t &field_id,
                                              const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t MeterIndirectTableData::setValue(const tdi_id_t &field_id,
                                              const uint8_t *value_ptr,
                                              const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t MeterIndirectTableData::getValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    uint64_t *value,
    uint8_t *value_ptr) const {
  uint64_t meter = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under meter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  meter_spec_.getMeterDataFromValue(field_type, &meter);
  setInputValToOutputVal(*table_, field_id, meter, value, value_ptr);
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirectTableData::getValue(const tdi_id_t &field_id,
                                              uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t MeterIndirectTableData::getValue(const tdi_id_t &field_id,
                                              const size_t &size,
                                              uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t MeterIndirectTableData::resetDerived() {
  meter_spec_.reset();
  return TDI_SUCCESS;
}

// RegisterIndirect
tdi_status_t RegisterIndirectTableData::setValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under register table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);
  getRegisterSpecObj().setFirstRegister(*tableDataField, val);
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirectTableData::setValue(const tdi_id_t &field_id,
                                                 const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t RegisterIndirectTableData::setValue(const tdi_id_t &field_id,
                                                 const uint8_t *value_ptr,
                                                 const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t RegisterIndirectTableData::getValue(
    const tdi_id_t &field_id, std::vector<uint64_t> *value) const {
  // Get the data_field from the table
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  const auto &register_obj = getRegisterSpecObj();
  const auto &tdi_registers = register_obj.getRegisterVec();
  for (const auto &tdi_register_data : tdi_registers) {
    value->push_back(tdi_register_data.getData(*tableDataField));
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirectTableData::resetDerived() {
  register_spec_.reset();
  return TDI_SUCCESS;
}

#if 0
// LPF TABLE DATA
tdi_status_t TdiLPFTableData::setValueInternal(const tdi_id_t &field_id,
                                               const size_t &size,
                                               const uint64_t &value,
                                               const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {
      DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR};
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  lpf_spec_.setLPFDataFromValue<uint64_t>(field_type, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::setValue(const tdi_id_t &field_id,
                                       const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiLPFTableData::setValue(const tdi_id_t &field_id,
                                       const uint8_t *value_ptr,
                                       const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiLPFTableData::setValue(const tdi_id_t &field_id,
                                       const float &value) {
  // Get the data_field from the table

  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT &&
        fieldType != DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_UNEXPECTED;
    }
    lpf_spec_.setLPFDataFromValue<float>(fieldType, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::setValue(const tdi_id_t &field_id,
                                       const std::string &value) {
  // Get the data_field from the table

  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::LPF_SPEC_TYPE) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_UNEXPECTED;
    }
    pipe_lpf_type_e lpf_val;
    tdi_status_t sts =
        DataFieldStringMapper::lpfSpecTypeFromStringGet(value, &lpf_val);
    if (sts != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR : Trying to get lpf spec type value from invalid "
          "string %s for field %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          value.c_str(),
          field_id);
      return sts;
    }
    sts = lpf_spec_.setLPFDataFromValue<pipe_lpf_type_e>(fieldType, lpf_val);
    if (sts != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR : Unable to set LPF spec type for field %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                field_id);
      return sts;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::getValue(const tdi_id_t &field_id,
                                       float *value) const {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT &&
        fieldType != DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_UNEXPECTED;
    }
    lpf_spec_.getLPFDataFromValue<float>(fieldType, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::getValue(const tdi_id_t &field_id,
                                       std::string *value) const {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::LPF_SPEC_TYPE) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_UNEXPECTED;
    }
    pipe_lpf_type_e lpf_val;
    tdi_status_t sts =
        lpf_spec_.getLPFDataFromValue<pipe_lpf_type_e>(fieldType, &lpf_val);
    if (sts != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR : Unable to get value for field %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                field_id);
      return sts;
    }
    sts = DataFieldStringMapper::lpfStringFromSpecTypeGet(lpf_val, value);
    if (sts != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR : Trying to get String from invalid lpf spec type "
          "value %d for field %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          lpf_val,
          field_id);
      return sts;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::getValueInternal(const tdi_id_t &field_id,
                                               const size_t &size,
                                               uint64_t *value,
                                               uint8_t *value_ptr) const {
  uint64_t lpf = 0;
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {
      DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR};
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  lpf_spec_.getLPFDataFromValue<uint64_t>(field_type, &lpf);
  setInputValToOutputVal(*table_, field_id, lpf, value, value_ptr);

  return TDI_SUCCESS;
}

tdi_status_t TdiLPFTableData::getValue(const tdi_id_t &field_id,
                                       uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiLPFTableData::getValue(const tdi_id_t &field_id,
                                       const size_t &size,
                                       uint8_t *value) const {
  return this->getValueInternal(field_id, size, nullptr, value);
}

tdi_status_t TdiLPFTableData::reset() {
  lpf_spec_.reset();
  return TDI_SUCCESS;
}

// WRED TABLE DATA
tdi_status_t TdiWREDTableData::setValueInternal(const tdi_id_t &field_id,
                                                const size_t &size,
                                                const uint64_t &value,
                                                const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {
      DataFieldType::WRED_SPEC_MIN_THRESHOLD,
      DataFieldType::WRED_SPEC_MAX_THRESHOLD};
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  return wred_spec_.setWREDDataFromValue<uint64_t>(field_type, val);
}

tdi_status_t TdiWREDTableData::setValue(const tdi_id_t &field_id,
                                        const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiWREDTableData::setValue(const tdi_id_t &field_id,
                                        const uint8_t *value_ptr,
                                        const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiWREDTableData::setValue(const tdi_id_t &field_id,
                                        const float &value) {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != WRED_SPEC_TIME_CONSTANT &&
        fieldType != WRED_SPEC_MAX_PROBABILITY) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_INVALID_ARG;
    }
    return wred_spec_.setWREDDataFromValue<float>(fieldType, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiWREDTableData::getValue(const tdi_id_t &field_id,
                                        float *value) const {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType != WRED_SPEC_TIME_CONSTANT &&
        fieldType != WRED_SPEC_MAX_PROBABILITY) {
      LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                tableDataField->idGet());
      return TDI_INVALID_ARG;
    }
    return wred_spec_.getWREDDataFromValue<float>(fieldType, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiWREDTableData::getValueInternal(const tdi_id_t &field_id,
                                                const size_t &size,
                                                uint64_t *value,
                                                uint8_t *value_ptr) const {
  uint64_t wred_val = 0;
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {
      DataFieldType::WRED_SPEC_MIN_THRESHOLD,
      DataFieldType::WRED_SPEC_MAX_THRESHOLD};
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  wred_spec_.getWREDDataFromValue<uint64_t>(field_type, &wred_val);
  setInputValToOutputVal(*table_, field_id, wred_val, value, value_ptr);
  return TDI_SUCCESS;
}

tdi_status_t TdiWREDTableData::getValue(const tdi_id_t &field_id,
                                        uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiWREDTableData::getValue(const tdi_id_t &field_id,
                                        const size_t &size,
                                        uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t TdiWREDTableData::reset() {
  wred_spec_.reset();
  return TDI_SUCCESS;
}

// REGISTER TABLE DATA
tdi_status_t TdiRegisterTableData::setValueInternal(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under reggister table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  const tdi::DataFieldInfo *tableDataField = nullptr;
  this->table_->getDataField(field_id, &tableDataField);
  getRegisterSpecObj().setFirstRegister(*tableDataField, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiRegisterTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value_ptr,
                                            const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiRegisterTableData::getValue(
    const tdi_id_t &field_id, std::vector<uint64_t> *value) const {
  // Get the data_field from the table
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  const auto &register_obj = getRegisterSpecObj();
  const auto &tdi_registers = register_obj.getRegisterVec();
  for (const auto &tdi_register_data : tdi_registers) {
    value->push_back(tdi_register_data.getData(*tableDataField));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterTableData::reset() {
  register_spec_.reset();
  return TDI_SUCCESS;
}

tdi_status_t TdiPhase0TableData::setValueInternal(const tdi_id_t &field_id,
                                                  const uint64_t &value,
                                                  const uint8_t *value_ptr,
                                                  const size_t &size) {

  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR Input Param bounds check failed for field id %d "
        "action id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet(),
        this->actionIdGet());
    return status;
  }
  auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : types_vec) {
    switch (fieldType) {
      case (DataFieldType::ACTION_PARAM):
        // Set the action param
        status = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, value, value_ptr);
        break;
      case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
        // When the action param is optimized out from the context json,
        // we need to just send 0s to the pipe mgr.
        LOG_WARN(
            "WARNING: %s:%d %s : Trying to set value for an optimized out "
            "field with id %d action id %d; Ignoring the user value and "
            "setting the field to zeros",
            __func__,
            __LINE__,
            this->table_->tableInfoGet()->nameGet().c_str(),
            tableDataField->idGet(),
            this->actionIdGet());
        std::vector<uint8_t> data_arr((tableDataField->sizeGet() + 7) / 8, 0);
        status = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, 0, data_arr.data());
        break;
      }
      default:
        LOG_ERROR("%s:%d %s ERROR Invalid field type encountered",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
    }
  }
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Unable to set data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiPhase0TableData::setValue(const tdi_id_t &field_id,
                                          const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiPhase0TableData::setValue(const tdi_id_t &field_id,
                                          const uint8_t *value_ptr,
                                          const size_t &size) {
  return this->setValueInternal(field_id, 0, value_ptr, size);
}

tdi_status_t TdiPhase0TableData::getValueInternal(const tdi_id_t &field_id,
                                                  uint64_t *value,
                                                  uint8_t *value_ptr,
                                                  const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  // Even though we have cached in a fake action id in the data obj, we need
  // to get the tabledatafield without action id. Because the data for phase0
  // tables is published as a common data field in tdi json
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s Invalid field id %d, action id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id,
              this->actionIdGet());
    return TDI_INVALID_ARG;
  }

  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  for (const auto &fieldType : tableDataField->getTypes()) {
    switch (fieldType) {
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_PARAM:
        return getPipeActionSpecObj().getValueActionParam(
            *tableDataField, value, value_ptr);
      default:
        LOG_ERROR("%s:%d %s ERROR Invalid field type encountered",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiPhase0TableData::getValue(const tdi_id_t &field_id,
                                          uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiPhase0TableData::getValue(const tdi_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size);
}

pipe_act_fn_hdl_t TdiPhase0TableData::getActFnHdl() const {
  return this->table_->getActFnHdl(this->actionIdGet());
}

tdi_status_t TdiPhase0TableData::reset(const tdi_id_t &act_id) {
  auto status reset_action_data<TdiPhase0TableData>(act_id, this);
  if (status) return status;
  return tdi::tabledata::reset(act_id);
}

// SnapshotConfig
tdi_status_t TdiSnapshotConfigTableData::setValue(const tdi_id_t &field_id,
                                                  const uint64_t &value_int) {
  return this->setValueInternal(
      field_id, value_int, nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotConfigTableData::setValue(const tdi_id_t &field_id,
                                                  const uint8_t *value_ptr,
                                                  const size_t &size) {
  return this->setValueInternal(field_id, 0, value_ptr, size, std::string(""));
}

tdi_status_t TdiSnapshotConfigTableData::setValue(const tdi_id_t &field_id,
                                                  const bool &value_int) {
  return this->setValueInternal(
      field_id, static_cast<uint64_t>(value_int), nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotConfigTableData::setValue(
    const tdi_id_t &field_id, const std::string &value_str) {
  return this->setValueInternal(field_id, 0, nullptr, 0, value_str);
}

tdi_status_t TdiSnapshotConfigTableData::setValue(
    const tdi_id_t &field_id, const std::vector<tdi_id_t> &arr) {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  this->capture_pipes[field_id] = arr;
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotConfigTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value_int,
    const uint8_t *value_ptr,
    const size_t &size,
    const std::string &value_str) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::UINT64: {
      uint64_t val = 0;
      if (value_ptr) {
        utils::TableFieldUtils::toHostOrderData(
            *tableDataField, value_ptr, &val);
      } else {
        val = value_int;
      }
      this->uint32_fields[field_id] = static_cast<uint32_t>(val);
      break;
    }
    case DataType::BOOL:
      this->uint32_fields[field_id] = static_cast<uint32_t>(value_int);
      break;
    case DataType::STRING: {
      if (value_str.empty()) {
        LOG_ERROR("Field data type is string, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      std::vector<std::reference_wrapper<const std::string>> allowed;
      status = this->table_->dataFieldAllowedChoicesGet(field_id, &allowed);
      if (status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_INVALID_ARG;
      }
      // Verify if input is correct, cast to set makes it easier to verify
      std::set<std::string> s(allowed.begin(), allowed.end());
      if (s.find(value_str) == s.end()) {
        LOG_ERROR("%s:%d %s ERROR Invalid field value %s",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  value_str.c_str());
        return TDI_INVALID_ARG;
      }
      this->str_fields[field_id] = value_str;
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotConfigTableData::getValue(const tdi_id_t &field_id,
                                                  uint64_t *value_int) const {
  return this->getValueInternal(field_id, value_int, nullptr, 0, nullptr);
}

tdi_status_t TdiSnapshotConfigTableData::getValue(const tdi_id_t &field_id,
                                                  const size_t &size,
                                                  uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size, nullptr);
}

tdi_status_t TdiSnapshotConfigTableData::getValue(const tdi_id_t &field_id,
                                                  bool *value_bool) const {
  uint64_t value_int = 0;
  auto status =
      this->getValueInternal(field_id, &value_int, nullptr, 0, nullptr);

  *value_bool = static_cast<bool>(value_int);
  return status;
}

tdi_status_t TdiSnapshotConfigTableData::getValue(const tdi_id_t &field_id,
                                                  std::string *str) const {
  return this->getValueInternal(field_id, nullptr, nullptr, 0, str);
}

tdi_status_t TdiSnapshotConfigTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value_int,
    uint8_t *value_ptr,
    const size_t &size,
    std::string *value_str) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case tdi::DataType::UINT64:
    /* Fallthrough */
    case tdi::DataType::BOOL: {
      uint32_t val = 0;
      auto elem = this->uint32_fields.find(field_id);
      if (elem != this->uint32_fields.end()) {
        val = this->uint32_fields.at(field_id);
      } else {
        LOG_ERROR("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        TDI_DBGCHK(0);
        return TDI_UNEXPECTED;
      }
      if (value_ptr) {
        utils::TableFieldUtils::toNetworkOrderData(
            *tableDataField, val, value_ptr);
      } else {
        *value_int = val;
      }
      break;
    }
    case tdi::DataType::STRING: {
      if (value_str == nullptr) {
        LOG_ERROR(
            "Field data type is string, but provided output param is not.");
        return TDI_INVALID_ARG;
      }
      auto elem = this->str_fields.find(field_id);
      if (elem != this->str_fields.end()) {
        *value_str = this->str_fields.at(field_id);
      } else {
        LOG_ERROR("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        TDI_DBGCHK(0);
        return TDI_UNEXPECTED;
      }
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotConfigTableData::getValue(
    const tdi_id_t &field_id, std::vector<tdi_id_t> *value) const {
  // Get the data_field from the table
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto elem = this->capture_pipes.find(field_id);
  if (elem != this->capture_pipes.end()) {
    for (const auto &pipe : this->capture_pipes.at(field_id)) {
      value->push_back(pipe);
    }
  }

  return TDI_SUCCESS;
}

// SnapshotTrigger
tdi_status_t TdiSnapshotTriggerTableData::setValue(const tdi_id_t &field_id,
                                                   const uint64_t &value_int) {
  return this->setValueInternal(
      field_id, value_int, nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotTriggerTableData::setValue(const tdi_id_t &field_id,
                                                   const uint8_t *value_ptr,
                                                   const size_t &size) {
  return this->setValueInternal(field_id, 0, value_ptr, size, std::string(""));
}

tdi_status_t TdiSnapshotTriggerTableData::setValue(const tdi_id_t &field_id,
                                                   const bool &value_int) {
  return this->setValueInternal(
      field_id, static_cast<uint64_t>(value_int), nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotTriggerTableData::setValue(
    const tdi_id_t &field_id, const std::string &value_str) {
  return this->setValueInternal(field_id, 0, nullptr, 0, value_str);
}

tdi_status_t TdiSnapshotTriggerTableData::setValue(
    const tdi_id_t &field_id, const std::vector<std::string> &arr) {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  std::vector<std::reference_wrapper<const std::string>> allowed;
  status = this->table_->dataFieldAllowedChoicesGet(field_id, &allowed);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  // Verify if input is correct, cast to set to it is easier to verify
  std::set<std::string> s(allowed.begin(), allowed.end());
  for (auto const &item : arr) {
    if (s.find(item) == s.end()) {
      LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_INVALID_ARG;
    }
  }

  this->str_arr_fields[field_id] = arr;
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotTriggerTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value_int,
    const uint8_t *value_ptr,
    const size_t &size,
    const std::string &value_str) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::UINT64: {
      uint64_t val = 0;
      if (value_ptr) {
        utils::TableFieldUtils::toHostOrderData(
            *tableDataField, value_ptr, &val);
      } else {
        val = value_int;
      }
      this->uint32_fields[field_id] = static_cast<uint32_t>(val);
      break;
    }
    case DataType::BOOL:
      this->uint32_fields[field_id] = static_cast<uint32_t>(value_int);
      break;
    case DataType::STRING:
      if (value_str.empty()) {
        LOG_ERROR("Field data type is string, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      this->str_fields[field_id] = value_str;
      break;
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      std::vector<uint8_t> tmp(size);
      memcpy(tmp.data(), value_ptr, size);
      this->bs_fields[field_id] = tmp;
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotTriggerTableData::getValue(
    const tdi_id_t &field_id, std::vector<std::string> *arr) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;

  // Verify field_id
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  auto elem = this->str_arr_fields.find(field_id);
  if (elem != this->str_arr_fields.end()) {
    *arr = elem->second;
  } else {
    LOG_TRACE("%s:%d %s ERROR : Data value for field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotTriggerTableData::getValue(const tdi_id_t &field_id,
                                                   uint64_t *value_int) const {
  return this->getValueInternal(field_id, value_int, nullptr, 0, nullptr);
}

tdi_status_t TdiSnapshotTriggerTableData::getValue(const tdi_id_t &field_id,
                                                   const size_t &size,
                                                   uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size, nullptr);
}

tdi_status_t TdiSnapshotTriggerTableData::getValue(const tdi_id_t &field_id,
                                                   bool *value_bool) const {
  uint64_t value_int = 0;
  auto status =
      this->getValueInternal(field_id, &value_int, nullptr, 0, nullptr);

  *value_bool = static_cast<bool>(value_int);
  return status;
}

tdi_status_t TdiSnapshotTriggerTableData::getValue(const tdi_id_t &field_id,
                                                   std::string *str) const {
  return this->getValueInternal(field_id, nullptr, nullptr, 0, str);
}

tdi_status_t TdiSnapshotTriggerTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value_int,
    uint8_t *value_ptr,
    const size_t &size,
    std::string *value_str) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case tdi::DataType::UINT64:
    /* Fallthrough */
    case tdi::DataType::BOOL: {
      uint64_t val = 0;
      auto elem = this->uint32_fields.find(field_id);
      if (elem != this->uint32_fields.end()) {
        val = this->uint32_fields.at(field_id);
      } else {
        val = 0;
      }
      if (value_ptr) {
        utils::TableFieldUtils::toNetworkOrderData(
            *tableDataField, val, value_ptr);
      } else {
        *value_int = val;
      }
      break;
    }
    case tdi::DataType::STRING: {
      if (value_str == nullptr) {
        LOG_ERROR(
            "Field data type is string, but provided output param is not.");
        return TDI_INVALID_ARG;
      }
      auto elem = this->str_fields.find(field_id);
      if (elem != this->str_fields.end()) {
        *value_str = this->str_fields.at(field_id);
      } else {
        LOG_TRACE("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_OBJECT_NOT_FOUND;
      }
      break;
    }
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }

      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      auto elem = this->bs_fields.find(field_id);
      if (elem != this->bs_fields.end()) {
        std::memcpy(value_ptr, elem->second.data(), size);
      } else {
        // This will happen during scanning for fields with populated values
        return TDI_OBJECT_NOT_FOUND;
      }
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

// SnapshotData
tdi_status_t TdiSnapshotDataTableData::setValue(const tdi_id_t &field_id,
                                                const uint64_t &value_int) {
  return this->setValueInternal(
      field_id, value_int, nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotDataTableData::setValue(const tdi_id_t &field_id,
                                                const uint8_t *value_ptr,
                                                const size_t &size) {
  return this->setValueInternal(field_id, 0, value_ptr, size, std::string(""));
}

tdi_status_t TdiSnapshotDataTableData::setValue(const tdi_id_t &field_id,
                                                const bool &value_int) {
  return this->setValueInternal(
      field_id, static_cast<uint64_t>(value_int), nullptr, 0, std::string(""));
}

tdi_status_t TdiSnapshotDataTableData::setValue(const tdi_id_t &field_id,
                                                const std::string &value_str) {
  return this->setValueInternal(field_id, 0, nullptr, 0, value_str);
}

tdi_status_t TdiSnapshotDataTableData::setValue(
    const tdi_id_t &field_id, const std::vector<std::string> &arr) {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  std::vector<std::reference_wrapper<const std::string>> allowed;
  status = this->table_->dataFieldAllowedChoicesGet(field_id, &allowed);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  if (allowed.size() > 0) {
    // Verify if input is correct, cast to set to it is easier to verify
    std::set<std::string> s(allowed.begin(), allowed.end());
    for (auto const &item : arr) {
      if (s.find(item) == s.end()) {
        LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_INVALID_ARG;
      }
    }
  }
  this->str_arr_fields[field_id] = arr;
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotDataTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value_int,
    const uint8_t *value_ptr,
    const size_t &size,
    const std::string &value_str) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::UINT64: {
      uint64_t val = 0;
      if (value_ptr) {
        utils::TableFieldUtils::toHostOrderData(
            *tableDataField, value_ptr, &val);
      } else {
        val = value_int;
      }
      this->uint32_fields[field_id] = static_cast<uint32_t>(val);
      break;
    }
    case DataType::BOOL:
      this->uint32_fields[field_id] = static_cast<uint32_t>(value_int);
      break;
    case DataType::STRING:
      if (value_str.empty()) {
        LOG_ERROR("Field data type is string, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      this->str_fields[field_id] = value_str;
      break;
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }

      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      std::vector<uint8_t> tmp(size);
      memcpy(tmp.data(), value_ptr, size);
      this->bs_fields[field_id] = tmp;
      break;
    }
    case DataType::CONTAINER: {
      std::unique_ptr<TdiSnapshotDataTableData> obj(
          new TdiSnapshotDataTableData(tdi_table_obj));
      obj->setContainerValid(false);
      std::set<DataFieldType> types = tableDataField->getTypes();
      // Get the first type
      for (auto it_set = types.begin(); it_set != types.end(); ++it_set) {
        obj->setDataFieldType(*it_set);
        break;
      }
      c_fields.insert(std::make_pair(field_id, std::move(obj)));
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotDataTableData::setValue(
    const tdi_id_t &field_id,
    std::unique_ptr<TdiSnapshotDataTableData> tableDataObj) {
  // Container object set
  auto it = this->c_fields.find(field_id);
  /* Insert the field if it does not exist */
  if (it == this->c_fields.end()) {
    // Field does not exist, so create a dummy integer object and
    // later convert it to a container object.
    uint64_t temp_int = 0;
    tdi_status_t status = this->setValue(field_id, temp_int);
    if (status != TDI_SUCCESS) {
      return status;
    }
    it = this->c_fields.find(field_id);
    if (it == c_fields.end()) {
      LOG_ERROR("%s:%d ERROR : Unable to insert field %d",
                __func__,
                __LINE__,
                field_id);
      return TDI_INVALID_ARG;
    }
  }
  auto &obj = it->second;
  // Set Container valid
  obj->setContainerValid(true);
  obj->setContainer(std::move(tableDataObj));
  return TDI_SUCCESS;
}

// Internal function
tdi_status_t TdiSnapshotDataTableData::setContainer(
    std::unique_ptr<TdiSnapshotDataTableData> tableDataObj) {
  container_items.push_back(std::move(tableDataObj));
  return TDI_SUCCESS;
}

// Return all container items
tdi_status_t TdiSnapshotDataTableData::getValue(
    const tdi_id_t &field_id, std::vector<TdiTableData *> *ret_vec) const {
  auto it = this->c_fields.find(field_id);
  if (it == this->c_fields.end()) {
    LOG_ERROR(
        "%s:%d ERROR : Field %d is not found", __func__, __LINE__, field_id);
    return TDI_INVALID_ARG;
  }
  auto &obj = it->second;
  if (!(obj->isContainerValid())) {
    // Wrong get API called, use api which returns object
    LOG_ERROR("%s:%d ERROR : Field %d is not a container object",
              __func__,
              __LINE__,
              field_id);
    return TDI_INVALID_ARG;
  }
  for (auto it_c = obj->container_items.begin();
       it_c != obj->container_items.end();
       ++it_c) {
    auto &item = *it_c;
    (*ret_vec).push_back((&item)->get());
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotDataTableData::getValue(
    const tdi_id_t &field_id, std::vector<std::string> *arr) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;

  // Verify field_id
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  auto elem = this->str_arr_fields.find(field_id);
  if (elem != this->str_arr_fields.end()) {
    *arr = elem->second;
  } else {
    LOG_TRACE("%s:%d %s ERROR : Data value for field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotDataTableData::getValue(const tdi_id_t &field_id,
                                                uint64_t *value_int) const {
  return this->getValueInternal(field_id, value_int, nullptr, 0, nullptr);
}

tdi_status_t TdiSnapshotDataTableData::getValue(const tdi_id_t &field_id,
                                                const size_t &size,
                                                uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size, nullptr);
}

tdi_status_t TdiSnapshotDataTableData::getValue(const tdi_id_t &field_id,
                                                bool *value_bool) const {
  uint64_t value_int = 0;
  auto status =
      this->getValueInternal(field_id, &value_int, nullptr, 0, nullptr);

  *value_bool = static_cast<bool>(value_int);
  return status;
}

tdi_status_t TdiSnapshotDataTableData::getValue(const tdi_id_t &field_id,
                                                std::string *str) const {
  return this->getValueInternal(field_id, nullptr, nullptr, 0, str);
}

tdi_status_t TdiSnapshotDataTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value_int,
    uint8_t *value_ptr,
    const size_t &size,
    std::string *value_str) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case tdi::DataType::UINT64:
    /* Fallthrough */
    case tdi::DataType::BOOL: {
      uint64_t val = 0;
      auto elem = this->uint32_fields.find(field_id);
      if (elem != this->uint32_fields.end()) {
        val = this->uint32_fields.at(field_id);
      } else {
        LOG_TRACE("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_OBJECT_NOT_FOUND;
      }
      if (value_ptr) {
        utils::TableFieldUtils::toNetworkOrderData(
            *tableDataField, val, value_ptr);
      } else {
        *value_int = val;
      }
      break;
    }
    case tdi::DataType::STRING: {
      if (value_str == nullptr) {
        LOG_ERROR(
            "Field data type is string, but provided output param is not.");
        return TDI_INVALID_ARG;
      }
      auto elem = this->str_fields.find(field_id);
      if (elem != this->str_fields.end()) {
        *value_str = this->str_fields.at(field_id);
      } else {
        LOG_TRACE("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_OBJECT_NOT_FOUND;
      }
      break;
    }
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }

      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      auto elem = this->bs_fields.find(field_id);
      if (elem != this->bs_fields.end()) {
        std::memcpy(value_ptr, elem->second.data(), size);
      } else {
        LOG_TRACE("%s:%d %s ERROR : Data value for field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_OBJECT_NOT_FOUND;
      }
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

// Snapshot Liveness
tdi_status_t TdiSnapshotLivenessTableData::setValueInternal(
    const tdi_id_t &field_id,
    const size_t &size,
    const uint64_t &v,
    const uint8_t *value_ptr) {
  uint64_t stage_val = 0;

  // Get the data_field from the table
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &v, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, v, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &stage_val);
  } else {
    stage_val = v;
  }

  stages.push_back(stage_val);

  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotLivenessTableData::setValue(const tdi_id_t &field_id,
                                                    const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiSnapshotLivenessTableData::setValue(const tdi_id_t &field_id,
                                                    const uint8_t *value_ptr,
                                                    const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiSnapshotLivenessTableData::getValue(
    const tdi_id_t &field_id, std::vector<tdi_id_t> *value) const {
  // Get the data_field from the table
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  for (const auto &stage_val : stages) {
    value->push_back(stage_val);
  }
  return TDI_SUCCESS;
}

/* Snapshot PHV table */
tdi_status_t TdiSnapshotPhvTableData::reset(
    const std::vector<tdi_id_t> &fields) {
  this->fields_.clear();
  return this->setActiveFields(fields);
}

tdi_status_t TdiSnapshotPhvTableData::reset() {
  std::vector<tdi_id_t> emptyfield;
  return this->reset(emptyfield);
}

tdi_status_t TdiSnapshotPhvTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }

  this->fields_[field_id] = static_cast<uint32_t>(val);
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotPhvTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  uint32_t val;
  auto elem = this->fields_.find(field_id);
  if (elem != this->fields_.end()) {
    val = this->fields_.at(field_id);
  } else {
    val = 0;
  }

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiSnapshotPhvTableData::setValue(const tdi_id_t &field_id,
                                               const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiSnapshotPhvTableData::getValue(const tdi_id_t &field_id,
                                               uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiSnapshotPhvTableData::getValue(const tdi_id_t &field_id,
                                               const size_t &size,
                                               uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiSnapshotPhvTableData::setValue(const tdi_id_t &field_id,
                                               const uint8_t *value,
                                               const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

/* Dynamic Hashing config table */

std::ostream &operator<<(std::ostream &os, const DynHashField &dt) {
  os << "(" << dt.hash_field_name_ << " start_bit:" << dt.start_bit_
     << " length:" << dt.length_ << " ) Order:" << dt.order_;
  return os;
}

tdi_status_t DynHashField::preProcess(const uint64_t &default_length) {
  // If start_bit is -1 (wasn't set), then set it to 0 (default)
  if (this->start_bit_ == -1) {
    this->start_bit_ = 0;
  }
  // if start_bit exceeds default_length, then error
  if (this->start_bit_ > static_cast<int>(default_length - 1)) {
    LOG_ERROR("ERROR: %s:%d start bit %d cannot exceed length-1: %" PRIu64 "",
              __func__,
              __LINE__,
              this->start_bit_,
              default_length - 1);
    return TDI_INVALID_ARG;
  }
  // If length is -1 (wasn't set), or if the provided value exceeds
  // right border of slice, then set it to right border.
  if (this->length_ == -1 ||
      this->start_bit_ + this->length_ > static_cast<int>(default_length)) {
    this->length_ = default_length - this->start_bit_;
  }
  // If length is explicitly given 0, then user wants to turn the slice off
  // which is the same as keeping order 0
  if (this->length_ == 0) {
    this->order_ = 0;
  }
  return TDI_SUCCESS;
}

tdi_status_t DynHashFieldSliceList::addSlice(const DynHashField &lhs) {
  std::ostringstream os;
  tdi_status_t status = TDI_SUCCESS;
  // If order is -1, return error straight away
  if (lhs.orderGet() == -1) {
    os << lhs << " Invalid Order " << std::endl;
    status = TDI_INVALID_ARG;
    goto cleanup;
  }
  for (const auto &rhs : this->list_) {
    bool sts = DynHashField::dynHashCompareOverlap(lhs, rhs);
    if (sts) {
      os << lhs << " overlaps over " << rhs << std::endl;
      status = TDI_INVALID_ARG;
      goto cleanup;
    }

    // Only check for symmetric overlap if order != 0
    if (lhs.orderGet() != 0) {
      sts = DynHashField::dynHashCompareSymmetricOrderOverlap(lhs, rhs);
      if (sts) {
        os << lhs << " has order conflict with " << rhs << std::endl;
        status = TDI_INVALID_ARG;
        goto cleanup;
      }
    }
    // Check for count for symmetric
    if (rhs.orderGet() == lhs.orderGet() && order_count_[rhs.orderGet()] == 2) {
      os << "Unable to add " << lhs
         << " 2 elements already exist with same order " << rhs << std::endl;
      status = TDI_INVALID_ARG;
      goto cleanup;
    }
  }
  // 0 Order means we want to skip tracking order count since many of them
  // can have order 0
  if (lhs.orderGet() != 0) {
    if (this->order_count_.find(lhs.orderGet()) != this->order_count_.end()) {
      this->order_count_[lhs.orderGet()]++;
    } else {
      this->order_count_[lhs.orderGet()] = 1;
    }
  }
  this->list_.push_back(lhs);
  return status;
cleanup:
  std::string s = os.str();
  LOG_ERROR("%s:%d Failed to add slice. %s", __func__, __LINE__, s.c_str());
  return status;
}

void DynHashFieldSliceList::clear() {
  this->list_.clear();
  this->order_count_.clear();
}

tdi_status_t DynHashFieldSliceList::removeSlice(const DynHashField &lhs) {
  auto i =
      find_if(list_.begin(),
              list_.end(),
              std::bind(DynHashField::isEqual, std::placeholders::_1, lhs));
  if (i != list_.end()) {
    this->list_.erase(i);
    this->order_count_[lhs.orderGet()]--;
    if (this->order_count_[lhs.orderGet()] == 0) {
      this->order_count_.erase(lhs.orderGet());
    }
  } else {
    std::ostringstream os;
    os << "Unable to find " << lhs << std::endl;
    std::string s = os.str();
    LOG_ERROR("%s:%d %s", __func__, __LINE__, s.c_str());
    return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

void DynHashFieldSliceList::sort() {
  std::sort(this->list_.begin(),
            this->list_.end(),
            [](const DynHashField &a, const DynHashField &b) {
              return a.orderGet() < b.orderGet();
            });
}

tdi_status_t TdiDynHashCfgTableData::attrListGet(
    std::vector<pipe_hash_calc_input_field_attribute_t> *attr_list) const {
  tdi_status_t sts = TDI_SUCCESS;
  for (const auto &df : this->slice_list_.listGet()) {
    pipe_hash_calc_input_field_attribute_t attr{0};
    // if order is 0, then just make it excluded
    if (df.orderGet() == 0) {
      attr.value.mask = INPUT_FIELD_EXCLUDED;
    } else {
      attr.value.mask = INPUT_FIELD_INCLUDED;
    }
    attr.input_field = this->name_id_map_.at(df.hash_field_name_) - 1;
    attr.slice_start_bit = df.start_bit_;
    attr.slice_length = df.length_;
    attr.order = df.order_;
    attr.type = INPUT_FIELD_ATTR_TYPE_MASK;
    attr_list->push_back(attr);
  }
  return sts;
}

tdi_status_t TdiDynHashCfgTableData::setValue(const tdi_id_t &field_id,
                                              const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashCfgTableData::setValue(const tdi_id_t &field_id,
                                              const uint8_t *value,
                                              const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiDynHashCfgTableData::setValue(
    const tdi_id_t &field_id,
    std::vector<std::unique_ptr<TdiTableData>> container_v) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  if (!tableDataField->isContainerValid()) {
    LOG_ERROR("ERROR: %s:%d Field ID %d is not container for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // We need to clear this data's state since we are overwriting for this
  // field_id
  if (this->con_obj_map_.find(field_id) != this->con_obj_map_.end() &&
      !this->con_obj_map_[field_id].empty()) {
    // For every data object in this list, remove from sliceList
    for (const auto &dt : this->con_obj_map_[field_id]) {
      auto inner_data = static_cast<TdiDynHashCfgTableData *>(dt.get());
      this->slice_list_.removeSlice(inner_data->dynHashFieldGet());
    }
    // empty this list now
    this->con_obj_map_[field_id].clear();
  }
  this->con_obj_map_[field_id] = std::vector<std::unique_ptr<TdiTableData>>{};

  // For all leaf data objects passed in
  //    check length.
  //      If -1; then default value needs to be used since this
  //        wasn't set on the leaf object.
  //      or If start_bit + length > field_width; then length needs
  //        to be shortened so that start_bit + length == bit_width
  //      If 0; then order needs to be made 0 and this slice
  //        needs to be excluded
  int index = 0;
  for (auto &l : container_v) {
    auto leaf = static_cast<TdiDynHashCfgTableData *>(l.get());
    if (leaf->dyn_hash_field_.hash_field_name_ == "" ||
        leaf->dyn_hash_field_.hash_field_name_ != tableDataField->getName()) {
      // Something is wrong here. User hasn't passed
      // correct data object. Log and return error
      LOG_ERROR(
          "ERROR: %s:%d table %s Error while parsing %dth data object "
          "Expecting %s Received %s",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          index,
          tableDataField->getName().c_str(),
          leaf->dyn_hash_field_.hash_field_name_.c_str());
      return TDI_INVALID_ARG;
    }
    // Process length
    std::string field_name = "length";
    const tdi::DataFieldInfo *length_data_field;
    std::vector<tdi_id_t> container_id_vec;
    container_id_vec.push_back(leaf->containerIdGet_());
    auto sts = this->table_->getDataField(field_name,
                                          0,  // act_id
                                          container_id_vec,
                                          &length_data_field);
    if (sts) return sts;
    int default_length = static_cast<int>(length_data_field->defaultValueGet());
    sts = leaf->dyn_hash_field_.preProcess(default_length);
    if (sts) {
      return sts;
    }
    // Start_bit and order get processed during slice add
    // Add leaf slice to top data
    sts = this->slice_list_.addSlice(leaf->dyn_hash_field_);
    if (sts) {
      LOG_ERROR("ERROR: %s:%d table %s Error while parsing %dth data object",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                index);
      return sts;
    }
    // Add item to name_id_map
    this->name_id_map_[tableDataField->getName()] = leaf->containerIdGet_();
    // Add the leaf obj to container obj map
    this->con_obj_map_[field_id].push_back(std::move(l));
    index++;
  }
  container_v.clear();
  this->slice_list_.sort();
  return TDI_SUCCESS;
}

tdi_status_t TdiDynHashCfgTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;
  std::vector<tdi_id_t> container_id_v;
  tdi_id_t container_id = this->containerIdGet_();
  container_id_v.push_back(container_id);
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), container_id_v, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }
  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  // figure out type and set it accordingly
  auto type_set = tableDataField->getTypes();
  auto field_type = *type_set.begin();
  if (field_type == DataFieldType::DYN_HASH_CFG_START_BIT) {
    this->dyn_hash_field_.start_bit_ = val;
  } else if (field_type == DataFieldType::DYN_HASH_CFG_LENGTH) {
    this->dyn_hash_field_.length_ = val;
  } else if (field_type == DataFieldType::DYN_HASH_CFG_ORDER) {
    this->dyn_hash_field_.order_ = val;
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiDynHashCfgTableData::getValue(const tdi_id_t &field_id,
                                              uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashCfgTableData::getValue(const tdi_id_t &field_id,
                                              const size_t &size,
                                              uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiDynHashCfgTableData::getValue(
    const tdi_id_t &field_id,
    std::vector<TdiTableData *> *container_v) const {
  // We need to return all slices for a hash field in this func
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  if (!tableDataField->isContainerValid()) {
    LOG_ERROR("ERROR: %s:%d Field ID %d is not container for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  if (this->con_obj_map_.find(field_id) == this->con_obj_map_.end()) {
    LOG_TRACE(" %s:%d Field ID %d is not set in this obj for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  } else {
    for (const auto &data : this->con_obj_map_.at(field_id))
      container_v->push_back(data.get());
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiDynHashCfgTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  std::vector<tdi_id_t> container_id_v;
  tdi_id_t container_id = this->containerIdGet_();
  container_id_v.push_back(container_id);
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), container_id_v, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  uint64_t val = 0;
  // figure out type and set it accordingly
  auto type_set = tableDataField->getTypes();
  auto field_type = *type_set.begin();
  if (field_type == DataFieldType::DYN_HASH_CFG_START_BIT) {
    val = this->dyn_hash_field_.start_bit_;
  } else if (field_type == DataFieldType::DYN_HASH_CFG_LENGTH) {
    val = this->dyn_hash_field_.length_;
  } else if (field_type == DataFieldType::DYN_HASH_CFG_ORDER) {
    val = this->dyn_hash_field_.order_;
  } else {
    LOG_ERROR("ERROR: %s:%d Invalid field type %d for table %s",
              __func__,
              __LINE__,
              field_type,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return status;
}

/* Dynamic Hashing algo table */
tdi_status_t TdiDynHashAlgoTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }
  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  auto field_name = tableDataField->getName();
  if (field_name == "seed") {
    seed = val;
  } else if (field_name == "extend") {
    extend = val;
  } else if (field_name == "msb") {
    msb = val;
  } else if (field_name == "reverse") {
    user.reverse = val;
  } else if (field_name == "polynomial") {
    user.polynomial = val;
  } else if (field_name == "init") {
    user.init = val;
  } else if (field_name == "final_xor") {
    user.final_xor = val;
  } else if (field_name == "hash_bit_width") {
    user.hash_bit_width = val;
  } else {
    LOG_ERROR("%s:%d %s : Cannot use this function to set field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiDynHashAlgoTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  uint64_t val;
  auto field_name = tableDataField->getName();
  if (field_name == "seed") {
    val = seed;
  } else if (field_name == "extend") {
    val = extend;
  } else if (field_name == "msb") {
    val = msb;
  } else if (field_name == "reverse") {
    val = user.reverse;
  } else if (field_name == "polynomial") {
    val = user.polynomial;
  } else if (field_name == "init") {
    val = user.init;
  } else if (field_name == "final_xor") {
    val = user.final_xor;
  } else if (field_name == "hash_bit_width") {
    val = user.hash_bit_width;
  } else {
    val = 0;
  }
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return status;
}

tdi_status_t TdiDynHashAlgoTableData::setValue(const tdi_id_t &field_id,
                                               const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashAlgoTableData::getValue(const tdi_id_t &field_id,
                                               uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashAlgoTableData::getValue(const tdi_id_t &field_id,
                                               const size_t &size,
                                               uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiDynHashAlgoTableData::setValue(const tdi_id_t &field_id,
                                               const uint8_t *value,
                                               const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiDynHashAlgoTableData::setValue(const tdi_id_t &field_id,
                                               const bool &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashAlgoTableData::getValue(const tdi_id_t &field_id,
                                               bool *value) const {
  uint64_t temp_ptr = 0;
  auto sts = this->getValueInternal(field_id, &temp_ptr, nullptr, 0);
  *value = temp_ptr;
  return sts;
}

tdi_status_t TdiDynHashAlgoTableData::setValue(const tdi_id_t &field_id,
                                               const std::string &value) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  this->algo_crc_name = value;
  return status;
}

tdi_status_t TdiDynHashAlgoTableData::getValue(const tdi_id_t &field_id,
                                               std::string *value) const {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet(), &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  *value = this->algo_crc_name;
  return status;
}

tdi_status_t TdiDynHashComputeTableData::setValue(const tdi_id_t &field_id,
                                                  const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashComputeTableData::setValue(const tdi_id_t &field_id,
                                                  const uint8_t *value,
                                                  const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiDynHashComputeTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  this->hash_value.resize((tableDataField->sizeGet() + 7) / 8, 0);
  if (value_ptr) {
    std::memcpy(this->hash_value.data(), value_ptr, size);
  } else {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, value, this->hash_value.data());
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiDynHashComputeTableData::getValue(const tdi_id_t &field_id,
                                                  uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiDynHashComputeTableData::getValue(const tdi_id_t &field_id,
                                                  const size_t &size,
                                                  uint8_t *value) const {
  return this->getValueInternal(field_id, nullptr, value, size);
}

tdi_status_t TdiDynHashComputeTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  if (value_ptr) {
    std::memcpy(value_ptr, this->hash_value.data(), this->hash_value.size());
  } else {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, this->hash_value.data(), value);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorGetMemberTableData::setValue(const tdi_id_t &field_id,
                                                     const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiSelectorGetMemberTableData::setValue(const tdi_id_t &field_id,
                                                     const uint8_t *value,
                                                     const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiSelectorGetMemberTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }

  this->act_mbr_id = val;
  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorGetMemberTableData::getValue(const tdi_id_t &field_id,
                                                     uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiSelectorGetMemberTableData::getValue(const tdi_id_t &field_id,
                                                     const size_t &size,
                                                     uint8_t *value) const {
  return this->getValueInternal(field_id, nullptr, value, size);
}

tdi_status_t TdiSelectorGetMemberTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  uint64_t val = this->act_mbr_id;
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}

/* Debug Counter table */
tdi_status_t TdiTblDbgCntTableData::reset(
    const std::vector<tdi_id_t> &fields) {
  this->fields_.clear();
  return this->setActiveFields(fields);
}

tdi_status_t TdiTblDbgCntTableData::reset() {
  std::vector<tdi_id_t> emptyfield;
  return this->reset(emptyfield);
}

tdi_status_t TdiTblDbgCntTableData::setValue(const tdi_id_t &field_id,
                                             const uint64_t &value_int) {
  return this->setValueInternal(
      field_id, value_int, nullptr, 0, std::string(""));
}

tdi_status_t TdiTblDbgCntTableData::setValue(const tdi_id_t &field_id,
                                             const uint8_t *value_ptr,
                                             const size_t &size) {
  return this->setValueInternal(field_id, 0, value_ptr, size, std::string(""));
}

tdi_status_t TdiTblDbgCntTableData::setValue(const tdi_id_t &field_id,
                                             const std::string &value_str) {
  return this->setValueInternal(field_id, 0, nullptr, 0, value_str);
}

tdi_status_t TdiTblDbgCntTableData::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value_int,
    const uint8_t *value_ptr,
    const size_t &size,
    const std::string &value_str) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value_int, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::UINT64: {
      uint64_t val = 0;
      if (value_ptr) {
        utils::TableFieldUtils::toHostOrderData(
            *tableDataField, value_ptr, &val);
      } else {
        val = value_int;
      }
      this->fields_[field_id] = val;
      break;
    }
    case DataType::STRING: {
      if (value_str.empty()) {
        LOG_ERROR("Field data type is string, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      std::vector<std::reference_wrapper<const std::string>> allowed;
      status = this->table_->dataFieldAllowedChoicesGet(field_id, &allowed);
      if (status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return TDI_INVALID_ARG;
      }
      // Verify if input is correct, cast to set makes it easier to verify
      std::set<std::string> s(allowed.begin(), allowed.end());
      if (s.find(value_str) == s.end()) {
        LOG_ERROR("%s:%d %s ERROR Invalid field value %s",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  value_str.c_str());
        return TDI_INVALID_ARG;
      }
      this->str_fields_[field_id] = value_str;
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiTblDbgCntTableData::getValue(const tdi_id_t &field_id,
                                             uint64_t *value_int) const {
  return this->getValueInternal(field_id, value_int, nullptr, 0, nullptr);
}

tdi_status_t TdiTblDbgCntTableData::getValue(const tdi_id_t &field_id,
                                             const size_t &size,
                                             uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size, nullptr);
}

tdi_status_t TdiTblDbgCntTableData::getValue(const tdi_id_t &field_id,
                                             std::string *str) const {
  return this->getValueInternal(field_id, nullptr, nullptr, 0, str);
}

tdi_status_t TdiTblDbgCntTableData::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value_int,
    uint8_t *value_ptr,
    const size_t &size,
    std::string *value_str) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case tdi::DataType::UINT64: {
      uint64_t val = 0;
      auto elem = this->fields_.find(field_id);
      if (elem != this->fields_.end()) {
        val = this->fields_.at(field_id);
      } else {
        LOG_ERROR("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        TDI_DBGCHK(0);
        return TDI_UNEXPECTED;
      }
      if (value_ptr) {
        utils::TableFieldUtils::toNetworkOrderData(
            *tableDataField, val, value_ptr);
      } else {
        *value_int = val;
      }
      break;
    }
    case tdi::DataType::STRING: {
      if (value_str == nullptr) {
        LOG_ERROR(
            "Field data type is string, but provided output param is not.");
        return TDI_INVALID_ARG;
      }
      auto elem = this->str_fields_.find(field_id);
      if (elem != this->str_fields_.end()) {
        *value_str = this->str_fields_.at(field_id);
      } else {
        LOG_ERROR("%s:%d %s ERROR : Value for data field id %d not found",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  field_id);
        TDI_DBGCHK(0);
        return TDI_UNEXPECTED;
      }
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

// RegisterParam
tdi_status_t TdiRegisterParamTableData::reset() {
  this->value = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterParamTableData::setValue(const tdi_id_t &field_id,
                                                 const uint8_t *value_ptr,
                                                 const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, nullptr, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      utils::TableFieldUtils::toHostOrderData(
          *tableDataField,
          value_ptr,
          reinterpret_cast<uint64_t *>(&this->value));
      // Fix the sign
      uint8_t shift = (sizeof(this->value) - size) * 8;
      this->value = (this->value << shift) >> shift;
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterParamTableData::getValue(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 uint8_t *value_ptr) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }

      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      uint64_t temp;
      std::memcpy(&temp, &this->value, sizeof(temp));
      utils::TableFieldUtils::toNetworkOrderData(
          *tableDataField, temp, value_ptr);
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}
#endif

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
