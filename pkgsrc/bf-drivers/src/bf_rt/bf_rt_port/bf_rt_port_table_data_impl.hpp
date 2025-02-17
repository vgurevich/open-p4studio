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


#ifndef _BF_RT_PORT_TABLE_DATA_IMPL_HPP
#define _BF_RT_PORT_TABLE_DATA_IMPL_HPP

#include <unordered_set>
#include "bf_rt_port_mgr_intf.hpp"
#include "../bf_rt_common/bf_rt_table_data_impl.hpp"
#include "../bf_rt_common/bf_rt_table_impl.hpp"

namespace bfrt {

class BfRtPortCfgTableData : public BfRtTableDataObj {
 public:
  BfRtPortCfgTableData(const BfRtTableObj *tbl_obj,
                       const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    set_active_fields(fields);
  }
  ~BfRtPortCfgTableData() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id, const int64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value);

  bf_status_t setValue(const bf_rt_id_t &field_id, const std::string &str);

  bf_status_t setValue(const bf_rt_id_t &field_id, const float &value);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, int64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *str) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, float *value) const;

  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;
  bf_status_t reset() override final;

  // unexposed API
  const std::unordered_set<bf_rt_id_t> &getActiveDataFields() const {
    return activeFields;
  }
  const std::unordered_map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<bf_rt_id_t, uint32_t> &getU32FieldDataMap() const {
    return u32FieldData;
  }
  const std::unordered_map<bf_rt_id_t, int32_t> &getI32FieldDataMap() const {
    return i32FieldData;
  }
  const std::unordered_map<bf_rt_id_t, uint64_t> &getU64FieldDataMap() const {
    return u64FieldData;
  }

  const std::unordered_map<bf_rt_id_t, std::string> &getStrFieldDataMap()
      const {
    return strFieldData;
  }
  const std::unordered_map<bf_rt_id_t, float> &getFloatFieldDataMap() const {
    return floatFieldData;
  }

 private:
  bf_status_t setU32ValueInternal(const bf_rt_id_t &field_id,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr,
                                  const size_t &s);
  bf_status_t getU32ValueInternal(const bf_rt_id_t &field_id,
                                  uint64_t *value,
                                  uint8_t *value_ptr,
                                  const size_t &s) const;
  bf_status_t set_active_fields(const std::vector<bf_rt_id_t> &fields);
  bool checkFieldActive(const bf_rt_id_t &field_id,
                        const DataType &dataType) const;
  bool all_fields_set;
  std::set<bf_rt_id_t> fieldPresent;
  std::unordered_map<bf_rt_id_t, bool> boolFieldData;
  std::unordered_map<bf_rt_id_t, uint32_t> u32FieldData;
  std::unordered_map<bf_rt_id_t, int32_t> i32FieldData;
  std::unordered_map<bf_rt_id_t, uint64_t> u64FieldData;
  std::unordered_map<bf_rt_id_t, std::string> strFieldData;
  std::unordered_map<bf_rt_id_t, float> floatFieldData;
  std::unordered_set<bf_rt_id_t> activeFields;
};

class BfRtPortStatTableData : public BfRtTableDataObj {
 public:
  BfRtPortStatTableData(const BfRtTableObj *tbl_obj,
                        const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    std::memset(u64FieldDataArray,
                0,
                (BF_NUM_RMON_COUNTERS + BF_NUM_PKT_RATE_COUNTERS +
                 BF_NUM_TIME_STAMP_COUNTERS) *
                    sizeof(uint64_t));
    set_active_fields(fields);
  }
  ~BfRtPortStatTableData() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;
  bf_status_t reset() override final;

  // unexposed API
  void setAllValues(const uint64_t *stats);
  const std::vector<bf_rt_id_t> &getActiveDataFields() const {
    return activeFields;
  }
  const uint64_t *getU64FieldData() const { return u64FieldDataArray; }
  const uint32_t &getAllStatsBoundry() const { return AllStatsBoundry; }

 private:
  bf_status_t set_active_fields(const std::vector<bf_rt_id_t> &fields);
  bf_status_t setU64ValueInternal(const bf_rt_id_t &field_id,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr,
                                  const size_t &s);
  bf_status_t getU64ValueInternal(const bf_rt_id_t &field_id,
                                  uint64_t *value,
                                  uint8_t *value_ptr,
                                  const size_t &s) const;
  bool all_fields_set;
  std::set<bf_rt_id_t> fieldPresent;
  uint64_t u64FieldDataArray[BF_NUM_RMON_COUNTERS + BF_NUM_PKT_RATE_COUNTERS +
                             BF_NUM_TIME_STAMP_COUNTERS];
  std::vector<bf_rt_id_t> activeFields;
  const uint32_t AllStatsBoundry = 20;
};

class BfRtPortHdlInfoTableData : public BfRtTableDataObj {
 public:
  BfRtPortHdlInfoTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj), dev_port_() {}
  ~BfRtPortHdlInfoTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset() override final;

  bf_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return BF_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

class BfRtPortFpIdxInfoTableData : public BfRtTableDataObj {
 public:
  BfRtPortFpIdxInfoTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj), dev_port_() {}
  ~BfRtPortFpIdxInfoTableData() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset() override final;

  bf_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return BF_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

class BfRtPortStrInfoTableData : public BfRtTableDataObj {
 public:
  BfRtPortStrInfoTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj), dev_port_() {}
  ~BfRtPortStrInfoTableData() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset() override final;
  bf_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return BF_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

}  // namespace bfrt
#endif  // _BF_RT_PORT_TABLE_DATA_IMPL_HPP
