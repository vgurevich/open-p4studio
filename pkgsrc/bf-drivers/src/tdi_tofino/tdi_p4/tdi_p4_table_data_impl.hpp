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


#ifndef _TDI_P4_TABLE_DATA_IMPL_HPP
#define _TDI_P4_TABLE_DATA_IMPL_HPP

#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>

#include <tdi_common/tdi_context_info.hpp>

namespace tdi {
namespace tna {
namespace tofino {

namespace {
#if 0
void initialize_data_field(const tdi_id_t &field_id,
                           const tdi::Table &table,
                           const tdi_id_t &action_id,
                           tdi::TableData * /*data_obj*/) {

  // No need to initialize mandatory fields, as those will not have
  // default value in JSON.
  const tdi::DataFieldInfo *field_obj =
      table.tableInfoGet()->dataFieldGet(field_id, action_id);
  if (!field_obj) {
    LOG_ERROR(
        "%s:%d %s ERROR in getting data field object for field id %d, "
        "action id %d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id,
        action_id);
    TDI_DBGCHK(0);
    return;
  }
  if (field_obj->mandatoryGet()) return;

  uint64_t default_value;
  std::string default_str_value;
  float default_fl_value;
  switch (field_obj->dataTypeGet()) {
    case TDI_FIELD_DATA_TYPE_UINT64:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, default_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_FLOAT:
      default_fl_value = field_obj->defaultFlValueGet();
      status = data_obj->setValue(field_id, default_fl_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_BOOL:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, static_cast<bool>(default_value));
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_STRING:
      default_str_value = field_obj->defaultStrValueGet();
      status = data_obj->setValue(field_id, default_str_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    default:
      break;
  }
  return;
}
#endif
}  // namespace

class DataFieldStringMapper {
 public:
  // Map of allowed string values for pipe_lpf_type_e
  static const std::map<std::string, pipe_lpf_type_e>
      string_to_lpf_spec_type_map;
  static const std::map<pipe_lpf_type_e, std::string>
      lpf_spec_type_to_string_map;

  // Map of allowed string values for pipe_idle_time_hit_state_e
  static const std::map<pipe_idle_time_hit_state_e, std::string>
      idle_time_hit_state_to_string_map;

  static tdi_status_t lpfSpecTypeFromStringGet(const std::string &val,
                                               pipe_lpf_type_e *lpf_val) {
    const auto kv = string_to_lpf_spec_type_map.find(val);
    if (kv == string_to_lpf_spec_type_map.end()) {
      return TDI_INVALID_ARG;
    }
    *lpf_val = kv->second;
    return TDI_SUCCESS;
  }

  static tdi_status_t lpfStringFromSpecTypeGet(const pipe_lpf_type_e &lpf_val,
                                               std::string *val) {
    const auto kv = lpf_spec_type_to_string_map.find(lpf_val);
    if (kv == lpf_spec_type_to_string_map.end()) {
      return TDI_INVALID_ARG;
    }
    *val = kv->second;
    return TDI_SUCCESS;
  }

  static tdi_status_t idleHitStateToString(const pipe_idle_time_hit_state_e &hs,
                                           std::string *val) {
    const auto kv = idle_time_hit_state_to_string_map.find(hs);
    if (kv == idle_time_hit_state_to_string_map.end()) {
      return TDI_INVALID_ARG;
    }
    *val = kv->second;
    return TDI_SUCCESS;
  }

  static tdi_status_t idleHitStateFromString(const std::string &val,
                                             pipe_idle_time_hit_state_e *hs) {
    auto map = idle_time_hit_state_to_string_map;
    for (auto kv = map.begin(); kv != map.end(); kv++) {
      if (kv->second == val) {
        *hs = kv->first;
        return TDI_SUCCESS;
      }
    }
    return TDI_INVALID_ARG;
  }
};  // DataFieldStringMapper

// This class is a software representation of how a register data field
// can be programmed in hardware. A hardware 'register' can be of size
// 1, 8, 16, 32 or 64. When in 1 size mode, the register acts in non dual
// mode and thus has only one instance published in the tdi json. When
// in 8, 16, 32 size mode, it can operate in non dual or dual mode.
// When in 64 size mode, the register always acts in dual mode.
//
// In non dual, there is only one instance published in the json while
// in dual mode there are two instances published in the json (each
// of the 8, 16 or 32 sizes respectively)
class RegisterSpec {
 public:
  tdi_status_t setDataFromStfulSpec(const pipe_stful_mem_spec_t &stful_spec);
  tdi_status_t getStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;

  tdi_status_t setData(const tdi::DataFieldInfo &tableDataField,
                       uint64_t value);
  uint64_t getData(const tdi::DataFieldInfo &tableDataField) const;

 private:
  pipe_stful_mem_spec_t stful = {};
};

class RegisterSpecData {
 public:
  void populateDataFromStfulSpec(
      const std::vector<pipe_stful_mem_spec_t> &stful_spec, uint32_t size);
  void reset() { registers_.clear(); }
  void populateStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;
  void setFirstRegister(const tdi::DataFieldInfo &tableDataField,
                        const uint64_t &value);
  void addRegisterToVec(const tdi::DataFieldInfo &tableDataField,
                        const uint64_t &value);
  const std::vector<RegisterSpec> &getRegisterVec() const { return registers_; }

 private:
  std::vector<RegisterSpec> registers_;
};

class CounterSpecData {
 public:
  CounterSpecData() { counter_data = {}; };
  void setCounterDataFromCounterSpec(const pipe_stat_data_t &counter);
  void setCounterDataFromValue(const DataFieldType &field_type,
                               const uint64_t &count);

  void getCounterData(const DataFieldType &field_type, uint64_t *count) const;

  const pipe_stat_data_t *getPipeCounterSpec() const { return &counter_data; }
  pipe_stat_data_t *getPipeCounterSpec() { return &counter_data; }
  void reset() { counter_data = {0}; }

 private:
  pipe_stat_data_t counter_data;
};

class MeterSpecData {
 public:
  MeterSpecData() { pipe_meter_spec = {}; };
  void reset() { pipe_meter_spec = {}; }

  void setCIRKbps(uint64_t value) {
    pipe_meter_spec.cir.value.kbps = value;
    pipe_meter_spec.cir.type = METER_RATE_TYPE_KBPS;
  }
  void setPIRKbps(uint64_t value) {
    pipe_meter_spec.pir.value.kbps = value;
    pipe_meter_spec.pir.type = METER_RATE_TYPE_KBPS;
  }
  void setCBSKbits(uint64_t value) { pipe_meter_spec.cburst = value; }
  void setPBSKbits(uint64_t value) { pipe_meter_spec.pburst = value; }
  void setCIRPps(uint64_t value) {
    pipe_meter_spec.cir.value.pps = value;
    pipe_meter_spec.cir.type = METER_RATE_TYPE_PPS;
  }
  void setPIRPps(uint64_t value) {
    pipe_meter_spec.pir.value.pps = value;
    pipe_meter_spec.pir.type = METER_RATE_TYPE_PPS;
  }
  void setCBSPkts(uint64_t value) { pipe_meter_spec.cburst = value; }
  void setPBSPkts(uint64_t value) { pipe_meter_spec.pburst = value; }

  const pipe_meter_spec_t *getPipeMeterSpec() const { return &pipe_meter_spec; }

  void setMeterDataFromValue(const DataFieldType &type, const uint64_t &value);
  void setMeterDataFromMeterSpec(const pipe_meter_spec_t &mspec);

  void getMeterDataFromValue(const DataFieldType &type, uint64_t *value) const;

 private:
  pipe_meter_spec_t pipe_meter_spec{};
};
class LPFSpecData {
 public:
  LPFSpecData() { pipe_lpf_spec = {}; };
  void reset() { pipe_lpf_spec = {}; }

  void setGainTimeConstant(float value) {
    pipe_lpf_spec.gain_time_constant = value;
    if (pipe_lpf_spec.gain_time_constant != pipe_lpf_spec.decay_time_constant) {
      pipe_lpf_spec.gain_decay_separate_time_constant = true;
    } else {
      pipe_lpf_spec.gain_decay_separate_time_constant = false;
      pipe_lpf_spec.time_constant = value;
    }
  }
  void setDecayTimeConstant(float value) {
    pipe_lpf_spec.decay_time_constant = value;
    if (pipe_lpf_spec.gain_time_constant != pipe_lpf_spec.decay_time_constant) {
      pipe_lpf_spec.gain_decay_separate_time_constant = true;
    } else {
      pipe_lpf_spec.gain_decay_separate_time_constant = false;
      pipe_lpf_spec.time_constant = value;
    }
  }
  void setRateEnable(bool enable) {
    pipe_lpf_spec.lpf_type = enable ? LPF_TYPE_RATE : LPF_TYPE_SAMPLE;
  }
  void setOutputScaleDownFactor(uint32_t value) {
    pipe_lpf_spec.output_scale_down_factor = value;
  }

  const pipe_lpf_spec_t *getPipeLPFSpec() const { return &pipe_lpf_spec; }
  pipe_lpf_spec_t *getPipeLPFSpec() { return &pipe_lpf_spec; }

  template <class T>
  tdi_status_t setLPFDataFromValue(const DataFieldType &type, const T &value);
  void setLPFDataFromLPFSpec(const pipe_lpf_spec_t *lpf_spec);

  template <class T>
  tdi_status_t getLPFDataFromValue(const DataFieldType &type, T *value) const;

 private:
  pipe_lpf_spec_t pipe_lpf_spec;
};

class WREDSpecData {
 public:
  WREDSpecData() { pipe_wred_spec = {}; };
  void reset() { pipe_wred_spec = {}; }
  void setTimeConstant(float value) { pipe_wred_spec.time_constant = value; }
  void setMinThreshold(uint32_t value) {
    pipe_wred_spec.red_min_threshold = value;
  }
  void setMaxThreshold(uint32_t value) {
    pipe_wred_spec.red_max_threshold = value;
  }
  void setMaxProbability(float value) {
    pipe_wred_spec.max_probability = value;
  }

  const pipe_wred_spec_t *getPipeWREDSpec() const { return &pipe_wred_spec; };
  pipe_wred_spec_t *getPipeWREDSpec() { return &pipe_wred_spec; };
  template <class T>
  tdi_status_t setWREDDataFromValue(const DataFieldType &type, const T &value);
  void setWREDDataFromWREDSpec(const pipe_wred_spec_t *wred_spec);

  template <class T>
  tdi_status_t getWREDDataFromValue(const DataFieldType &type, T *value) const;

 private:
  pipe_wred_spec_t pipe_wred_spec;
};

// This class manages the pipe action spec. The responsibilities of
// this class include
// 1. Given a DataField, set/get the value in/from the the action spec
// 2. Expose APIs to set/get the action spec through a variety of possibilities
// 3. This class internally has a few sub-composite classes (one for each
//    type of resource)
class PipeActionSpec {
 public:
  PipeActionSpec(const size_t &data_sz,
                 const size_t &data_sz_bits,
                 const uint8_t &pipe_action_datatype_bmap) {
    std::memset(&action_spec, 0, sizeof(action_spec));
    std::memset(&action_spec.act_data, 0, sizeof(action_spec.act_data));
    if (data_sz) {
      action_data_bits.reset(new uint8_t[data_sz]());
      action_spec.act_data.action_data_bits = action_data_bits.get();
      action_spec.act_data.num_action_data_bytes = data_sz;
      action_spec.act_data.num_valid_action_data_bits = data_sz_bits;
    } else {
      action_data_bits = nullptr;
      action_spec.act_data.action_data_bits = nullptr;
      action_spec.act_data.num_action_data_bytes = 0;
      action_spec.act_data.num_valid_action_data_bits = 0;
    }
    action_spec.pipe_action_datatype_bmap = pipe_action_datatype_bmap;
  }

  void resetPipeActionSpec() {
    if (action_spec.act_data.action_data_bits) {
      std::memset(action_spec.act_data.action_data_bits,
                  0,
                  action_spec.act_data.num_action_data_bytes);
    }
    std::memset(action_spec.resources,
                0,
                sizeof(pipe_res_spec_t) * PIPE_NUM_TBL_RESOURCES);
    action_spec.resource_count = 0;
    action_spec.sel_grp_hdl = 0;
    action_spec.adt_ent_hdl = 0;
    counter_spec.reset();
    meter_spec.reset();
    lpf_spec.reset();
    wred_spec.reset();
    register_spec.reset();
    num_direct_resources = 0;
    num_indirect_resources = 0;
    action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }

  ~PipeActionSpec() {}

  // Action Param
  tdi_status_t setValueActionParam(const tdi::DataFieldInfo &field,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr);
  tdi_status_t getValueActionParam(const tdi::DataFieldInfo &field,
                                   uint64_t *value,
                                   uint8_t *value_ptr) const;

  // Resource Index
  tdi_status_t setValueResourceIndex(const tdi::DataFieldInfo &field,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     const uint64_t &value,
                                     const uint8_t *value_ptr);
  tdi_status_t getValueResourceIndex(const tdi::DataFieldInfo &field,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     uint64_t *value,
                                     uint8_t *value_ptr) const;

  // Counter
  tdi_status_t setValueCounterSpec(const pipe_stat_data_t &counter);
  tdi_status_t setValueCounterSpec(const tdi::DataFieldInfo &field,
                                   const DataFieldType &field_type,
                                   const pipe_tbl_hdl_t &tbl_hdl,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr);
  tdi_status_t getValueCounterSpec(const tdi::DataFieldInfo &field,
                                   const DataFieldType &field_type,
                                   uint64_t *value,
                                   uint8_t *value_ptr) const;

  // Register
  tdi_status_t setValueRegisterSpec(
      const std::vector<pipe_stful_mem_spec_t> &register_data);
  tdi_status_t setValueRegisterSpec(const tdi::DataFieldInfo &field,
                                    const DataFieldType &field_type,
                                    const pipe_tbl_hdl_t &tbl_hdl,
                                    const uint64_t &value,
                                    const uint8_t *value_ptr,
                                    const size_t &field_size);
  tdi_status_t getValueRegisterSpec(const tdi::DataFieldInfo &field,
                                    std::vector<uint64_t> *value) const;

  // Meter
  tdi_status_t setValueMeterSpec(const pipe_meter_spec_t &meter);
  tdi_status_t setValueMeterSpec(const tdi::DataFieldInfo &field,
                                 const DataFieldType &field_type,
                                 const pipe_tbl_hdl_t &tbl_hdl,
                                 const uint64_t &value,
                                 const uint8_t *value_ptr);
  tdi_status_t getValueMeterSpec(const tdi::DataFieldInfo &field,
                                 const DataFieldType &field_type,
                                 uint64_t *value,
                                 uint8_t *value_ptr) const;

  // LPF
  tdi_status_t setValueLPFSpec(const pipe_lpf_spec_t &lpf);
  tdi_status_t setValueLPFSpec(const tdi::DataFieldInfo &field,
                               const DataFieldType &field_type,
                               const pipe_tbl_hdl_t &tbl_hdl,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  template <typename T>
  tdi_status_t setValueLPFSpec(const DataFieldType &field_type,
                               const pipe_tbl_hdl_t &tbl_hdl,
                               const T &value);
  tdi_status_t getValueLPFSpec(const tdi::DataFieldInfo &field,
                               const DataFieldType &field_type,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  template <typename T>
  tdi_status_t getValueLPFSpec(const DataFieldType &field_type, T *value) const;

  // WRED
  tdi_status_t setValueWREDSpec(const pipe_wred_spec_t &wred);
  tdi_status_t setValueWREDSpec(const tdi::DataFieldInfo &field,
                                const DataFieldType &field_type,
                                const pipe_tbl_hdl_t &tbl_hdl,
                                const uint64_t &value,
                                const uint8_t *value_ptr);
  template <typename T>
  tdi_status_t setValueWREDSpec(const DataFieldType &field_type,
                                const pipe_tbl_hdl_t &tbl_hdl,
                                const T &value);
  tdi_status_t getValueWREDSpec(const tdi::DataFieldInfo &field,
                                const DataFieldType &field_type,
                                uint64_t *value,
                                uint8_t *value_ptr) const;
  template <typename T>
  tdi_status_t getValueWREDSpec(const DataFieldType &field_type,
                                T *value) const;

  // Action member
  tdi_status_t setValueActionDataHdlType();

  // Selector Group
  tdi_status_t setValueSelectorGroupHdlType();

  uint8_t getPipeActionDatatypeBitmap() const {
    return action_spec.pipe_action_datatype_bmap;
  }

  pipe_action_spec_t *getPipeActionSpec() { return &action_spec; }

  const pipe_action_spec_t *getPipeActionSpec() const { return &action_spec; }

  const CounterSpecData &getCounterSpecObj() const { return counter_spec; }
  CounterSpecData &getCounterSpecObj() { return counter_spec; }
  const MeterSpecData &getMeterSpecObj() const { return meter_spec; };
  MeterSpecData &getMeterSpecObj() { return meter_spec; };
  const LPFSpecData &getLPFSpecObj() const { return lpf_spec; };
  LPFSpecData &getLPFSpecObj() { return lpf_spec; };
  const WREDSpecData &getWREDSpecObj() const { return wred_spec; };
  WREDSpecData &getWREDSpecObj() { return wred_spec; };
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec; }
  const uint32_t &directResCountGet() { return num_direct_resources; };
  const uint32_t &indirectResCountGet() { return num_indirect_resources; };

 private:
  pipe_res_spec_t *getResourceSpec(const pipe_tbl_hdl_t &tbl_hdl);
  const pipe_res_spec_t *getResourceSpec(const pipe_tbl_hdl_t &tbl_hdl) const;

  void updateResourceSpec(const pipe_tbl_hdl_t &tbl_hdl,
                          const bool &&is_direct_resource,
                          pipe_res_spec_t **res_spec);

  template <typename T>
  tdi_status_t setValueLPFSpecHelper(const DataFieldType &field_type,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     const T &value);
  template <typename T>
  tdi_status_t setValueWREDSpecHelper(const DataFieldType &field_type,
                                      const pipe_tbl_hdl_t &tbl_hdl,
                                      const T &value);

  CounterSpecData counter_spec;
  MeterSpecData meter_spec;
  LPFSpecData lpf_spec;
  WREDSpecData wred_spec;
  RegisterSpecData register_spec;

  pipe_action_spec_t action_spec{0};
  std::unique_ptr<uint8_t[]> action_data_bits{nullptr};
  uint32_t num_direct_resources = 0;
  uint32_t num_indirect_resources = 0;
};

class MatchActionTableData : public tdi::TableData {
 public:
  MatchActionTableData(const tdi::Table *table,
                       tdi_id_t act_id,
                       const std::vector<tdi_id_t> &fields)
      : tdi::TableData(table, act_id, fields) {
    size_t data_sz = 0;
    size_t data_sz_bits = 0;
    if (act_id) {
      data_sz = static_cast<const TofinoActionContextInfo *>(
                    this->table_->tableInfoGet()
                        ->actionGet(act_id)
                        ->actionContextInfoGet())
                    ->dataSzGet();
      data_sz_bits = static_cast<const TofinoActionContextInfo *>(
                         this->table_->tableInfoGet()
                             ->actionGet(act_id)
                             ->actionContextInfoGet())
                         ->dataSzBitsGet();
    } else {
      data_sz = static_cast<const TofinoTableContextInfo *>(
                    this->table_->tableInfoGet()->tableContextInfoGet())
                    ->maxDataSzGet();
      data_sz_bits = static_cast<const TofinoTableContextInfo *>(
                         this->table_->tableInfoGet()->tableContextInfoGet())
                         ->maxDataSzBitsGet();
    }

    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
    this->initializeDataFields();
  }

  virtual ~MatchActionTableData() {}

  MatchActionTableData(const tdi::Table *tbl_obj,
                       const std::vector<tdi_id_t> &fields)
      : MatchActionTableData(tbl_obj, 0, fields) {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const std::string &value) override;
  tdi_status_t setValue(const tdi_id_t &field_id, const float &value) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::string *value) const override;
  tdi_status_t getValue(const tdi_id_t &field_id, float *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::vector<uint64_t> *value) const override;

  // This gives out a copy of action spec. This is needed for the indirect Match
  // tables to fill in the action member ID or selector group ID based
  // information in the action spec.
  // Note that for match indirect tables, the copy of action spec is not too
  // expensive since the byte array representing the action parameters does not
  // exist
  virtual void copy_pipe_action_spec(pipe_action_spec_t *act_spec) const {
    std::memcpy(act_spec,
                getPipeActionSpecObj().getPipeActionSpec(),
                sizeof(*act_spec));
    return;
  }

  virtual const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  virtual pipe_action_spec_t *get_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  uint32_t get_ttl() const { return ttl; }

  pipe_idle_time_hit_state_e get_entry_hit_state() const { return hit_state; }

  void set_ttl_from_read(uint32_t value) { ttl = value; }

  void set_entry_hit_state(pipe_idle_time_hit_state_e state) {
    hit_state = state;
  }

  const uint32_t &indirectResCountGet() const {
    return num_indirect_resource_count;
  }

  PipeActionSpec &getPipeActionSpecObj() {
    return *(action_spec_wrapper.get());
  }
  const PipeActionSpec &getPipeActionSpecObj() const {
    return *(action_spec_wrapper.get());
  }

  virtual tdi_status_t reset(const tdi_id_t &action_id,
                             const tdi_id_t & /*container_id*/,
                             const std::vector<tdi_id_t> &fields) override;

  // A public setter for action_spec_wrapper
  void actionSpecSet(PipeActionSpec *action_spec) {
    if (action_spec == nullptr) {
      action_spec_wrapper->resetPipeActionSpec();
    } else {
      action_spec_wrapper.reset(action_spec);
    }
    return;
  }

  const uint32_t &numActionOnlyFieldsGet() const {
    return num_action_only_fields;
  }
  pipe_act_fn_hdl_t getActFnHdl() const;

 protected:
  tdi_id_t action_mbr_id{0};
  tdi_id_t group_id{0};
  std::unique_ptr<PipeActionSpec> action_spec_wrapper;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const uint64_t &value,
                                const uint8_t *value_ptr,
                                const size_t &s);

  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                uint64_t *value,
                                uint8_t *value_ptr,
                                const size_t &size) const;

  void set_action_member_id(uint64_t val);
  tdi_id_t get_action_member_id() const { return action_mbr_id; }
  tdi_id_t get_selector_group_id() const { return group_id; }
  void set_selector_group_id(uint64_t val);
  void set_ttl(uint64_t val);
  void initializeDataFields();
  void getIndirectResourceCounts(const std::vector<tdi_id_t> field_list);

  uint32_t ttl = 0;
  pipe_idle_time_hit_state_e hit_state = ENTRY_IDLE;
  uint32_t num_indirect_resource_count = 0;
  uint32_t num_action_only_fields = 0;
};

class MatchActionIndirectTableData : public MatchActionTableData {
 public:
  MatchActionIndirectTableData(const tdi::Table *tbl_obj,
                               const std::vector<tdi_id_t> &fields)
      : MatchActionTableData(tbl_obj, fields){};

  tdi_id_t getActionMbrId() const {
    return MatchActionTableData::action_mbr_id;
  }

  void setActionMbrId(tdi_id_t act_mbr_id) {
    MatchActionTableData::action_mbr_id = act_mbr_id;
  }

  tdi_id_t getGroupId() const { return MatchActionTableData::group_id; }

  void setGroupId(tdi_id_t sel_grp_id) {
    MatchActionTableData::group_id = sel_grp_id;
  }

  bool isGroup() const {
    return (MatchActionTableData::getPipeActionSpecObj()
                .getPipeActionDatatypeBitmap() == PIPE_SEL_GRP_HDL_TYPE);
  }

  static const tdi_id_t invalid_group = 0xdeadbeef;
  static const tdi_id_t invalid_action_entry_hdl = 0xdeadbeef;
};

class ActionProfileData : public tdi::TableData {
 public:
  ActionProfileData(const tdi::Table *tbl_obj, tdi_id_t act_id)
      : tdi::TableData(tbl_obj, act_id) {
    size_t data_sz = 0;
    size_t data_sz_bits = 0;
    if (act_id) {
      data_sz = static_cast<const TofinoActionContextInfo *>(
                    this->table_->tableInfoGet()
                        ->actionGet(act_id)
                        ->actionContextInfoGet())
                    ->dataSzGet();
      data_sz_bits = static_cast<const TofinoActionContextInfo *>(
                         this->table_->tableInfoGet()
                             ->actionGet(act_id)
                             ->actionContextInfoGet())
                         ->dataSzBitsGet();
    } else {
      data_sz = static_cast<const TofinoTableContextInfo *>(
                    this->table_->tableInfoGet()->tableContextInfoGet())
                    ->maxDataSzGet();
      data_sz_bits = static_cast<const TofinoTableContextInfo *>(
                         this->table_->tableInfoGet()->tableContextInfoGet())
                         ->maxDataSzBitsGet();
    }

    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
    this->initializeDataFields();
  }

  ActionProfileData(const tdi::Table *tbl_obj)
      : ActionProfileData(tbl_obj, 0) {}

  tdi_status_t reset(const tdi_id_t &action_id,
                     const tdi_id_t & /*container_id*/,
                     const std::vector<tdi_id_t> &fields) override;

  virtual ~ActionProfileData(){};

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const;

  void initializeDataFields(){};
  const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  pipe_action_spec_t *mutable_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  const std::map<DataFieldType, tdi_id_t> &get_res_map() const {
    return resource_map;
  }

  // A public setter for action_spec_wrapper
  void actionSpecSet(PipeActionSpec *action_spec) {
    if (action_spec == nullptr) {
      action_spec_wrapper->resetPipeActionSpec();
    } else {
      action_spec_wrapper.reset(action_spec);
    }
    return;
  }

  pipe_act_fn_hdl_t getActFnHdl() const;

 private:
  PipeActionSpec &getPipeActionSpecObj() {
    return *(action_spec_wrapper.get());
  }
  const PipeActionSpec &getPipeActionSpecObj() const {
    return *(action_spec_wrapper.get());
  }

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const uint64_t &value,
                                const uint8_t *value_ptr,
                                const size_t &s);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                uint64_t *value,
                                uint8_t *value_ptr,
                                const size_t &s) const;

  std::unique_ptr<PipeActionSpec> action_spec_wrapper;
  // A map of indirect resource type to the resource index
  // Used to maintain state required for action profile management
  std::map<DataFieldType, tdi_id_t> resource_map;
};

class SelectorTableData : public tdi::TableData {
 public:
  SelectorTableData(const tdi::Table *tbl_obj,
                    const std::vector<tdi_id_t> &fields);

  ~SelectorTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const std::vector<tdi_id_t> &arr);

  tdi_status_t setValue(const tdi_id_t &field_id, const std::vector<bool> &arr);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::vector<tdi_id_t> *arr) const;

  tdi_status_t getValue(const tdi_id_t &field_id, std::vector<bool> *arr) const;

  pipe_act_fn_hdl_t get_pipe_act_fn_hdl() const { return act_fn_hdl_; }
  uint32_t get_max_grp_size() const { return max_grp_size_; }

  const std::vector<uint32_t> &getMembers() const { return members_; }
  const std::vector<bool> &getMemberStatus() const { return member_status_; }

  void setMembers(std::vector<uint32_t> &members) { members_ = members; }
  void setMemberStatus(std::vector<bool> &member_status) {
    member_status_ = member_status;
  }
  void setMaxGrpSize(const uint32_t &max_size) { max_grp_size_ = max_size; }

  tdi_status_t resetDerived() override;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                const uint64_t &value,
                                const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                uint64_t *value,
                                uint8_t *value_ptr) const;
  std::vector<tdi_id_t> get_members_from_array(const uint8_t *value,
                                               const size_t &size);
  pipe_act_fn_hdl_t act_fn_hdl_;
  std::vector<uint32_t> members_;
  std::vector<bool> member_status_;
  uint32_t max_grp_size_{0};
};

class CounterIndirectTableData : public tdi::TableData {
 public:
  CounterIndirectTableData(const tdi::Table *table) : tdi::TableData(table){};
  ~CounterIndirectTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  // Functions not exposed
  const CounterSpecData &getCounterSpecObj() const { return counter_spec_; }
  CounterSpecData &getCounterSpecObj() { return counter_spec_; }

  tdi_status_t resetDerived() override;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                const uint64_t &value,
                                const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                uint64_t *value,
                                uint8_t *value_ptr) const;
  CounterSpecData counter_spec_;
};

class MeterIndirectTableData : public tdi::TableData {
 public:
  MeterIndirectTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~MeterIndirectTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  // Functions not exposed
  const MeterSpecData &getMeterSpecObj() const { return meter_spec_; };
  MeterSpecData &getMeterSpecObj() { return meter_spec_; };

  tdi_status_t resetDerived() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                const uint64_t &value,
                                const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                uint64_t *value,
                                uint8_t *value_ptr) const;

  MeterSpecData meter_spec_;
};

// RegisterIndirect
class RegisterIndirectTableData : public tdi::TableData {
 public:
  RegisterIndirectTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~RegisterIndirectTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::vector<uint64_t> *value) const;

  // Unexposed APIs
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec_; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec_; }

  tdi_status_t resetDerived() override;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const size_t &size,
                                const uint64_t &value,
                                const uint8_t *value_ptr);

  RegisterSpecData register_spec_;
};

#if 0
class LPFTableData : public tdi::TableData {
 public:
  LPFTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~LPFTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id, const float &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id, float *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::string *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  // Functions not exposed
  const LPFSpecData &getLPFSpecObj() const { return lpf_spec_; };
  LPFSpecData &getLPFSpecObj() { return lpf_spec_; };

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  LPFSpecData lpf_spec_;
};

class WREDTableData : public tdi::TableData {
 public:
  WREDTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~WREDTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id, const float &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id, float *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;
  // Functions not exposed
  const WREDSpecData &getWREDSpecObj() const { return wred_spec_; };
  WREDSpecData &getWREDSpecObj() { return wred_spec_; };

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  uint64_t wred = 0;
  WREDSpecData wred_spec_;
};

class RegisterTableData : public tdi::TableData {
 public:
  RegisterTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~RegisterTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<uint64_t> *value) const;

  // Unexposed APIs
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec_; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec_; }

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);

  RegisterSpecData register_spec_;
};

class EmptyTableData : public tdi::TableData {
 public:
  EmptyTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~EmptyTableData() = default;
};

class Phase0TableData : public tdi::TableData {
 public:
  Phase0TableData(const tdi::Table *tbl_obj, const tdi_id_t &act_id)
      : tdi::TableData(tbl_obj) {
    this->actionIdSet(act_id);
    // Prime the pipe mgr action spec structure
    size_t data_sz = getdataSz(act_id);
    size_t data_sz_bits = getdataSzbits(act_id);
    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
  }
  ~Phase0TableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Unexposed APIs
  const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  pipe_action_spec_t *get_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  tdi_status_t reset(const tdi_id_t &act_id) override final;

  // A public setter for action_spec_wrapper
  void actionSpecSet(PipeActionSpec *action_spec) {
    if (action_spec == nullptr) {
      action_spec_wrapper->resetPipeActionSpec();
    } else {
      action_spec_wrapper.reset(action_spec);
    }
    return;
  }

  pipe_act_fn_hdl_t getActFnHdl() const;

 private:
  PipeActionSpec &getPipeActionSpecObj() {
    return *(action_spec_wrapper.get());
  }
  const PipeActionSpec &getPipeActionSpecObj() const {
    return *(action_spec_wrapper.get());
  }

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

  std::unique_ptr<PipeActionSpec> action_spec_wrapper;
};

class DynHashField {
 public:
  DynHashField(std::string hash_field_name,
               int start_bit,
               int length,
               int order,
               int field_width)
      : hash_field_name_(hash_field_name),
        start_bit_(start_bit),
        length_(length),
        order_(order),
        field_width_(field_width){};
  DynHashField() : DynHashField("", -1, -1, -1, -1){};
  static bool isEqual(const DynHashField &lhs, const DynHashField &rhs) {
    return (lhs.hash_field_name_ == rhs.hash_field_name_ &&
            lhs.start_bit_ == rhs.start_bit_ && lhs.length_ == rhs.length_);
  }
  static bool dynHashCompareOverlap(const DynHashField &lhs,
                                    const DynHashField &rhs) {
    // overlap condition 1: if rhs start_bit is inside lhs
    if (rhs.hash_field_name_ == lhs.hash_field_name_ &&
        rhs.start_bit_ < lhs.start_bit_ + lhs.length_ &&
        rhs.start_bit_ >= lhs.start_bit_)
      return true;
    // overlap condition 2:  if lhs start_bit_ is inside rhs
    if (lhs.hash_field_name_ == rhs.hash_field_name_ &&
        lhs.start_bit_ < rhs.start_bit_ + rhs.length_ &&
        lhs.start_bit_ >= rhs.start_bit_)
      return true;
    return false;
  }
  static bool dynHashCompareSymmetricOrderOverlap(const DynHashField &lhs,
                                                  const DynHashField &rhs) {
    if (lhs.order_ == rhs.order_ && (lhs.length_ != rhs.length_)) return true;
    return false;
  }
  const int &orderGet() const { return order_; }
  void hashFieldNameSet(const std::string &name) {
    if (name == "") return;
    this->hash_field_name_ = name;
  };
  tdi_status_t preProcess(const uint64_t &default_length);

  friend std::ostream &operator<<(std::ostream &os, const DynHashField &dt);
  friend class DynHashCfgTableData;

 private:
  std::string hash_field_name_;
  int start_bit_ = -1;
  int length_ = -1;
  int order_ = -1;
  int field_width_ = -1;
};

class DynHashFieldSliceList {
 public:
  tdi_status_t addSlice(const DynHashField &lhs);
  tdi_status_t removeSlice(const DynHashField &lhs);

  void clear();
  void sort();
  const std::vector<DynHashField> &listGet() const { return this->list_; };

 private:
  std::vector<DynHashField> list_;
  // Count of each order
  std::unordered_map<int, int> order_count_;
};

class DynHashCfgTableData : public tdi::TableData {
 public:
  DynHashCfgTableData(const tdi::Table *tbl_obj,
                          tdi_id_t container_id,
                          const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, container_id, 0 /*act_id*/) {
    this->containerIdSet(container_id);
    this->setActiveFields(fields);
  }
  DynHashCfgTableData(const tdi::Table *tbl_obj,
                          const std::vector<tdi_id_t> &fields)
      : DynHashCfgTableData(tbl_obj, 0, fields) {}
  ~DynHashCfgTableData() {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  tdi_status_t setValue(
      const tdi_id_t &field_id,
      std::vector<std::unique_ptr<tdi::TableData>> container_v) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  tdi_status_t getValue(
      const tdi_id_t &field_id,
      std::vector<tdi::TableData *> *container_v) const override final;

  // unexposed funcs
  void setDynHashFieldName(const std::string &name) {
    this->dyn_hash_field_.hashFieldNameSet(name);
  }
  const DynHashField &dynHashFieldGet() { return this->dyn_hash_field_; }
  tdi_status_t attrListGet(
      std::vector<pipe_hash_calc_input_field_attribute_t> *attr_list) const;

 private:
  // These internal functions are only for leaf data objects
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  // Top object = data object containing info
  // for all hash fields
  // leaf object = data object containing info of only one container
  // i.e, one container object

  // This contains list of all slices. This is constructed when
  // called setValue with the vec of Data objects. Only applicable
  // for top data object
  DynHashFieldSliceList slice_list_;
  // This contains info about a single slice. Applicable only to
  // leaf data objects
  DynHashField dyn_hash_field_;
  // This map of lists is applicable for top data object. This keeps
  // ownership of leaf data objects. During get, driver creates these
  // data objects and moves into this vec. During set, driver takes
  // over ownership of the passed-in leaf data objects and keeps them
  // here.
  std::map<tdi_id_t, std::vector<std::unique_ptr<tdi::TableData>>>
      con_obj_map_;
  // This map contains hash field name to field ID (container fieldID)
  std::map<std::string, tdi_id_t> name_id_map_;
};

class DynHashAlgoTableData : public tdi::TableData {
 public:
  DynHashAlgoTableData(const tdi::Table *tbl_obj,
                           tdi_id_t act_id,
                           const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  DynHashAlgoTableData(const tdi::Table *tbl_obj,
                           const std::vector<tdi_id_t> &fields)
      : DynHashAlgoTableData(tbl_obj, 0, fields) {}
  ~DynHashAlgoTableData() {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const bool &value) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::string *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       bool *value) const override final;

 private:
  struct user_defined_s {
    bool reverse = false;
    uint64_t polynomial = 0;
    uint64_t init = 0;
    uint64_t final_xor = 0;
    uint64_t hash_bit_width = 0;
  };

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::string algo_crc_name = "";
  user_defined_s user;
  bool msb = false;
  bool extend = false;
  uint64_t seed = 0;
};

class DynHashComputeTableData : public tdi::TableData {
 public:
  DynHashComputeTableData(const tdi::Table *tbl_obj,
                              tdi_id_t act_id,
                              const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  DynHashComputeTableData(const tdi::Table *tbl_obj,
                              const std::vector<tdi_id_t> &fields)
      : DynHashComputeTableData(tbl_obj, 0, fields) {}
  ~DynHashComputeTableData() {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::vector<uint8_t> hash_value;
};

class SelectorGetMemberTableData : public tdi::TableData {
 public:
  SelectorGetMemberTableData(const tdi::Table *tbl_obj,
                                 tdi_id_t act_id,
                                 const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  SelectorGetMemberTableData(const tdi::Table *tbl_obj,
                                 const std::vector<tdi_id_t> &fields)
      : SelectorGetMemberTableData(tbl_obj, 0, fields) {}
  ~SelectorGetMemberTableData() {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  uint32_t act_mbr_id;
};

class SnapshotConfigTableData : public tdi::TableData {
 public:
  SnapshotConfigTableData(const tdi::Table *tbl_obj,
                              const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj) {
    std::vector<tdi_id_t> field_list = fields;
    if (field_list.empty()) {
      tbl_obj->dataFieldIdListGet(&field_list);
    }
    for (auto it = field_list.begin(); it != field_list.end(); it++) {
      initialize_data_field(*it, *table_, 0, this);
    }
    this->setActiveFields(fields);
  }

  ~SnapshotConfigTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value_int);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::vector<tdi_id_t> &arr);
  tdi_status_t setValue(const tdi_id_t &field_id, const bool &value_int);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id, bool *value_bool) const;
  tdi_status_t getValue(const tdi_id_t &field_id, std::string *str) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<tdi_id_t> *value) const;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;
  std::map<tdi_id_t, uint32_t> uint32_fields;
  std::map<tdi_id_t, std::string> str_fields;
  std::map<tdi_id_t, std::vector<uint32_t>> capture_pipes;
};

class SnapshotTriggerTableData : public tdi::TableData {
 public:
  SnapshotTriggerTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~SnapshotTriggerTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value_int);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  tdi_status_t setValue(const tdi_id_t &field_id, const bool &value_bool);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::vector<std::string> &arr);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id, bool *value_bool) const;
  tdi_status_t getValue(const tdi_id_t &field_id, std::string *str) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<std::string> *arr) const;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  std::map<tdi_id_t, std::vector<uint8_t>> bs_fields;
  std::map<tdi_id_t, uint32_t> uint32_fields;
  std::map<tdi_id_t, std::string> str_fields;
  std::map<tdi_id_t, std::vector<std::string>> str_arr_fields;
};

class SnapshotDataTableData : public tdi::TableData {
 public:
  SnapshotDataTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj), container_valid(), field_type() {
    tdi_table_obj = tbl_obj;
  }
  ~SnapshotDataTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value_int);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  tdi_status_t setValue(const tdi_id_t &field_id, const bool &value_bool);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::vector<std::string> &arr);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       std::unique_ptr<SnapshotDataTableData> tableDataObj);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  tdi_status_t getValue(const tdi_id_t &field_id, bool *value_bool) const;
  tdi_status_t getValue(const tdi_id_t &field_id, std::string *str) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<std::string> *arr) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<tdi::TableData *> *ret_vec) const;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);

  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  // Container fields
  void setContainerValid(bool v) { this->container_valid = v; };
  bool isContainerValid() const { return container_valid; };
  tdi_status_t setContainer(
      std::unique_ptr<SnapshotDataTableData> tableDataObj);
  bool container_valid;
  std::vector<std::unique_ptr<SnapshotDataTableData>> container_items;
  std::map<tdi_id_t, std::unique_ptr<SnapshotDataTableData>> c_fields;

  // Field type
  DataFieldType getDataFieldType() const { return this->field_type; };
  void setDataFieldType(DataFieldType d) { this->field_type = d; };
  DataFieldType field_type;
  const tdi::Table *tdi_table_obj;

  // Standard fields
  std::map<tdi_id_t, std::vector<uint8_t>> bs_fields;
  std::map<tdi_id_t, uint32_t> uint32_fields;
  std::map<tdi_id_t, std::string> str_fields;
  std::map<tdi_id_t, std::vector<std::string>> str_arr_fields;
};

class SnapshotLivenessTableData : public tdi::TableData {
 public:
  SnapshotLivenessTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~SnapshotLivenessTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<tdi_id_t> *value) const;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  std::vector<uint32_t> stages;
};

class SnapshotPhvTableData : public tdi::TableData {
 public:
  SnapshotPhvTableData(const tdi::Table *tbl_obj,
                           const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj) {
    this->setActiveFields(fields);
  };
  SnapshotPhvTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};

  ~SnapshotPhvTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  tdi_status_t reset() override final;
  tdi_status_t reset(const std::vector<tdi_id_t> &fields) override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::map<tdi_id_t, uint32_t> fields_;
};

class TblDbgCntTableData : public tdi::TableData {
 public:
  TblDbgCntTableData(const tdi::Table *tbl_obj,
                         const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj) {
    std::vector<tdi_id_t> field_list = fields;
    if (field_list.empty()) {
      tbl_obj->dataFieldIdListGet(&field_list);
    }
    for (auto it = field_list.begin(); it != field_list.end(); it++) {
      initialize_data_field(*it, *table_, 0, this);
    }
    this->setActiveFields(fields);
  };
  TblDbgCntTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};

  ~TblDbgCntTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id, std::string *str) const;

  tdi_status_t reset() override final;
  tdi_status_t reset(const std::vector<tdi_id_t> &fields) override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);

  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  std::map<tdi_id_t, uint64_t> fields_;
  std::map<tdi_id_t, std::string> str_fields_;
};

class RegisterParamTableData : public tdi::TableData {
 public:
  RegisterParamTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~RegisterParamTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;

  tdi_status_t reset() override final;

  // Unpublished
  int64_t value;
};
#endif

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_DATA_IMPL_HPP
