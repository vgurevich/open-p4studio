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


#ifndef _TDI_P4_TABLE_KEY_IMPL_HPP
#define _TDI_P4_TABLE_KEY_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_key.hpp>

#include <tdi/arch/tna/tna_defs.h>

#include <tdi_common/tdi_context_info.hpp>

typedef uint32_t tdi_tof_act_mem_id_t;
typedef uint32_t tdi_tof_sel_grp_id_t;

namespace tdi {
namespace tna {
namespace tofino {

class MatchActionKey : public tdi::TableKey {
 public:
  MatchActionKey(const Table *table) : TableKey(table) {
    // Allocate the key array and mask array based on the key size
    setKeySz();
    partition_index = 0;
    priority = 0;
  }

  ~MatchActionKey() {
    if (key_array) {
      delete[] key_array;
    }
    if (mask_array) {
      delete[] mask_array;
    }
  }

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  virtual tdi_status_t reset() override;

  void setPriority(uint32_t pri) { priority = pri; }
  void setPartitionIndex(uint32_t p) { partition_index = p; }
  // Hidden
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);
  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint64_t &value,
                               const uint64_t &mask);
  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &size);
  tdi_status_t setValueRange(const tdi::KeyFieldInfo *key_field,
                             const uint64_t &start,
                             const uint64_t &end);
  tdi_status_t setValueRange(const tdi::KeyFieldInfo *key_field,
                             const uint8_t *start,
                             const uint8_t *end,
                             const size_t &size);
  tdi_status_t setValueLpm(const tdi::KeyFieldInfo *key_field,
                           const uint64_t &value,
                           const uint16_t &p_length);
  tdi_status_t setValueLpm(const tdi::KeyFieldInfo *key_field,
                           const uint8_t *value1,
                           const uint16_t &p_length,
                           const size_t &size);
  tdi_status_t setValueOptional(const tdi::KeyFieldInfo *key_field,
                                const uint64_t &value,
                                const bool &is_valid);
  tdi_status_t setValueOptional(const tdi::KeyFieldInfo *key_field,
                                const uint8_t *value,
                                const bool &is_valid,
                                const size_t &size);
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;
  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               uint64_t *value1,
                               uint64_t *value2) const;
  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               const size_t &size,
                               uint8_t *value1,
                               uint8_t *value2) const;
  tdi_status_t getValueRange(const tdi::KeyFieldInfo *key_field,
                             uint64_t *start,
                             uint64_t *end) const;
  tdi_status_t getValueRange(const tdi::KeyFieldInfo *key_field,
                             const size_t &size,
                             uint8_t *start,
                             uint8_t *end) const;
  tdi_status_t getValueLpm(const tdi::KeyFieldInfo *key_field,
                           uint64_t *start,
                           uint16_t *p_length) const;
  tdi_status_t getValueLpm(const tdi::KeyFieldInfo *key_field,
                           const size_t &size,
                           uint8_t *start,
                           uint16_t *p_length) const;
  tdi_status_t getValueOptional(const tdi::KeyFieldInfo *key_field,
                                uint64_t *value1,
                                bool *is_valid) const;
  tdi_status_t getValueOptional(const tdi::KeyFieldInfo *key_field,
                                const size_t &size,
                                uint8_t *value1,
                                bool *is_valid) const;

  void populate_match_spec(pipe_tbl_match_spec_t *pipe_match_spec) const;

  void populate_key_from_match_spec(
      const pipe_tbl_match_spec_t &pipe_match_spec);

  void set_key_from_match_spec_by_deepcopy(
      const pipe_tbl_match_spec_t *pipe_match_spec);

  static void packFieldIntoMatchSpecByteBuffer(
      const tdi::KeyFieldInfo &key_field,
      const size_t &size,
      const bool &do_masking,
      const uint8_t *field_buf,
      const uint8_t *field_mask_buf,
      uint8_t *match_spec_buf,
      uint8_t *match_mask_spec_buf,
      const tdi::Table *table);

 private:
  void setValueInternal(const tdi::KeyFieldInfo &key_field,
                        const uint8_t *value,
                        const size_t &num_bytes);
  void getValueInternal(const tdi::KeyFieldInfo &key_field,
                        uint8_t *value,
                        const size_t &num_bytes) const;
  void setValueAndMaskInternal(const tdi::KeyFieldInfo &key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &num_bytes,
                               bool do_masking);
  void getValueAndMaskInternal(const tdi::KeyFieldInfo &key_field,
                               uint8_t *value,
                               uint8_t *mask,
                               const size_t &num_bytes) const;
  void setKeySz();

  void unpackFieldFromMatchSpecByteBuffer(const tdi::KeyFieldInfo &key_field,
                                          const size_t &size,
                                          const uint8_t *match_spec_buf,
                                          uint8_t *field_buf) const;

  uint16_t num_valid_match_bits;
  uint16_t num_valid_match_bytes;
  uint8_t *key_array;
  uint8_t *mask_array;
  uint32_t partition_index;
  uint32_t priority;
};

class ActionProfileKey : public tdi::TableKey {
 public:
  ActionProfileKey(const Table *tbl_obj) : TableKey(tbl_obj), member_id(){};

  ~ActionProfileKey() = default;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const tdi::KeyFieldValue &field_value) override;
  tdi_status_t getValue(const tdi_id_t &field_id,
                        tdi::KeyFieldValue *field_value) const override;

  tdi_id_t getMemberId() const { return member_id; }
  void setMemberId(tdi_id_t mbr_id) { member_id = mbr_id; }

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_status_t reset() {
    member_id = 0;
    return TDI_SUCCESS;
  }

 private:
  tdi_tof_act_mem_id_t member_id;
};

class SelectorTableKey : public tdi::TableKey {
 public:
  SelectorTableKey(const Table *table) : tdi::TableKey(table), group_id(){};

  ~SelectorTableKey() = default;
  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  // Hidden
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_id_t getGroupId() const { return group_id; }

  void setGroupId(tdi_id_t grp_id) { group_id = grp_id; }

  virtual tdi_status_t reset() override {
    group_id = 0;
    return TDI_SUCCESS;
  };

 private:
  tdi_id_t group_id;
};

class CounterIndirectTableKey : public tdi::TableKey {
 public:
  CounterIndirectTableKey(const tdi::Table *table)
      : tdi::TableKey(table), counter_id(){};
  ~CounterIndirectTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  uint32_t getCounterId() const { return counter_id; }

  void setCounterId(uint32_t id) { counter_id = id; }

  void setIdxKey(uint32_t idx) { counter_id = idx; }

  uint32_t getIdxKey() const { return counter_id; }

  virtual tdi_status_t reset() override {
    counter_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t counter_id;
};

class MeterIndirectTableKey : public tdi::TableKey {
 public:
  MeterIndirectTableKey(const tdi::Table *table)
      : tdi::TableKey(table), meter_id(){};
  ~MeterIndirectTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

#if 0
  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;
#endif
  void setIdxKey(uint32_t idx) { meter_id = idx; }

  uint32_t getIdxKey() const { return meter_id; }

  virtual tdi_status_t reset() override {
    meter_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t meter_id;
};

// RegisterIndirect
class RegisterIndirectTableKey : public tdi::TableKey {
 public:
  RegisterIndirectTableKey(const tdi::Table *table)
      : tdi::TableKey(table), register_id(){};
  ~RegisterIndirectTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  void setIdxKey(uint32_t idx) { register_id = idx; }

  uint32_t getIdxKey() const { return register_id; }

  virtual tdi_status_t reset() override {
    register_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t register_id = 0;
};

class PVSTableKey : public tdi::TableKey {
 public:
  PVSTableKey(const tdi::Table *table) : tdi::TableKey(table){};
  ~PVSTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint64_t &value,
                               const uint64_t &mask);

  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &size);

  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               uint64_t *value1,
                               uint64_t *value2) const;

  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               const size_t &size,
                               uint8_t *value1,
                               uint8_t *value2) const;

  // Hidden
  void populate_match_spec(uint32_t *key_p, uint32_t *mask_p) const {
    // match spec in pvs_table is key and mask. It's not a struct include all
    // config as MatchAction table.
    *key_p = key_;
    *mask_p = mask_;
  };

  // be careful while using this function, may change the saved mask and key
  void populate_key_from_match_spec(uint32_t &key, uint32_t &mask) {
    key_ = key;
    mask_ = mask;
  };

  tdi_status_t reset() override {
    key_ = 0;
    mask_ = 0;
    return TDI_SUCCESS;
  }

 private:
  void setValueAndMaskInternal(const size_t &field_offset,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &num_bytes);

  void getValueAndMaskInternal(const size_t &field_offset,
                               uint8_t *value,
                               uint8_t *mask,
                               const size_t &num_bytes) const;

  uint32_t key_{0};
  uint32_t mask_{0};
};

#if 0
class TdiLPFTableKey : public TdiTableKeyObj {
 public:
  TdiLPFTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj), lpf_id(){};
  ~TdiLPFTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { lpf_id = idx; }

  uint32_t getIdxKey() const { return lpf_id; }

  tdi_status_t reset() {
    lpf_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t lpf_id;
};

class TdiWREDTableKey : public TdiTableKeyObj {
 public:
  TdiWREDTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj), wred_id(){};
  ~TdiWREDTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { wred_id = idx; }

  uint32_t getIdxKey() const { return wred_id; }

  tdi_status_t reset() {
    wred_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t wred_id;
};

class TdiRegisterTableKey : public TdiTableKeyObj {
 public:
  TdiRegisterTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj), register_id(){};
  ~TdiRegisterTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { register_id = idx; }

  uint32_t getIdxKey() const { return register_id; }

  tdi_status_t reset() {
    register_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t register_id;
};



class TdiSnapshotConfigTableKey : public TdiTableKeyObj {
 public:
  TdiSnapshotConfigTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj) {}

  ~TdiSnapshotConfigTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void get_stage_id(uint32_t *start_stage_p, uint32_t *end_stage_p) const;
  void set_stage_id(const uint32_t start_stage, const uint32_t end_stage);

 private:
  std::map<tdi_id_t, uint32_t> uint32_fields;
};

class TdiSnapshotDataTableKey : public TdiTableKeyObj {
 public:
  TdiSnapshotDataTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj) {}
  ~TdiSnapshotDataTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  tdi_status_t get_stage_id(uint32_t *stage) const;
  tdi_status_t set_stage_id(const uint32_t stage);

 private:
  std::map<tdi_id_t, uint32_t> uint32_fields;
};

class TdiSnapshotTriggerTableKey : public TdiTableKeyObj {
 public:
  TdiSnapshotTriggerTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj) {}
  ~TdiSnapshotTriggerTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  tdi_status_t get_stage_id(uint32_t *stage) const;
  tdi_status_t set_stage_id(const uint32_t stage);

 private:
  std::map<tdi_id_t, uint32_t> uint32_fields;
};

class TdiSnapshotLivenessTableKey : public TdiTableKeyObj {
 public:
  TdiSnapshotLivenessTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiSnapshotLivenessTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);

  tdi_status_t getValue(const tdi_id_t &field_id, std::string *value) const;

  tdi_status_t getFieldName(std::string *value) const;

 private:
  std::string field_name;
};

class TdiSnapshotPhvTableKey : public TdiTableKeyObj {
 public:
  TdiSnapshotPhvTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj), stage_id(0){};
  ~TdiSnapshotPhvTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);
  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  uint32_t getStageId() const { return stage_id; }
  void setStageId(uint32_t stage) { stage_id = stage; }

 private:
  uint32_t stage_id;
};

class TdiDynHashCfgTableKey : public TdiTableKeyObj {
 public:
  TdiDynHashCfgTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiDynHashCfgTableKey() = default;
};

class TdiDynHashAlgoTableKey : public TdiTableKeyObj {
 public:
  TdiDynHashAlgoTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiDynHashAlgoTableKey() = default;
};

class TdiTblDbgCntTableKey : public TdiTableKeyObj {
 public:
  TdiTblDbgCntTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiTblDbgCntTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::string &str_value);

  tdi_status_t getValue(const tdi_id_t &field_id, std::string *value) const;

  const std::string getTblName() const { return this->tbl_name; };
  void setTblName(const std::string &name) { this->tbl_name = name; };

 private:
  std::string tbl_name;
};

class TdiLogDbgCntTableKey : public TdiTableKeyObj {
 public:
  TdiLogDbgCntTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiLogDbgCntTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);
  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

 private:
  std::map<tdi_id_t, uint32_t> fields_;
};

class TdiDynHashComputeTableKey : public TdiTableKeyObj {
 public:
  TdiDynHashComputeTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiDynHashComputeTableKey() = default;
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

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

  // unexposed funcs
  tdi_status_t attrListGet(
      const TdiTable *cfg_tbl,
      std::vector<pipe_hash_calc_input_field_attribute_t> *attr_list) const;

 private:
  bool isConstant(const tdi_id_t &field_id, const TdiTable *cfg_tbl) const;
  std::map<tdi_id_t, uint64_t> uint64_fields;
};

class TdiSelectorGetMemberTableKey : public TdiTableKeyObj {
 public:
  TdiSelectorGetMemberTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiSelectorGetMemberTableKey() = default;
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

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

 private:
  std::map<tdi_id_t, uint64_t> uint64_fields;
};

// Wrapper Class for Pipe mgr match spec
class PipeMgrMatchSpec {
 public:
  PipeMgrMatchSpec(size_t num_bytes) {
    std::memset(&pipe_match_spec, 0, sizeof(pipe_match_spec));
    pipe_match_spec.match_value_bits = new uint8_t[num_bytes];
    pipe_match_spec.match_mask_bits = new uint8_t[num_bytes];
  }
  ~PipeMgrMatchSpec() {
    delete[] pipe_match_spec.match_value_bits;
    delete[] pipe_match_spec.match_mask_bits;
  }
  pipe_tbl_match_spec_t *getPipeMatchSpec();

 private:
  pipe_tbl_match_spec_t pipe_match_spec;
};
#endif

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_KEY_IMPL_HPP
