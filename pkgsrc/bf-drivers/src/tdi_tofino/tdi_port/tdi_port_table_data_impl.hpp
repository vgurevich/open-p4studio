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


#ifndef _TDI_PORT_TABLE_DATA_IMPL_HPP
#define _TDI_PORT_TABLE_DATA_IMPL_HPP

#include <unordered_set>
//#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table_data.hpp>
#include <tdi/common/tdi_table.hpp>
#include "tdi_port_mgr_intf.hpp"

namespace tdi {

class PortCfgTableData : public tdi::TableData {
 public:
  PortCfgTableData(const tdi::Table *tbl_obj,
                   const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, fields) {}
  ~PortCfgTableData() = default;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const int64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;

  tdi_status_t setValue(const tdi_id_t &field_id, const bool &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const std::string &str) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        int64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id, bool *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::string *str) const override;

  tdi_status_t resetDerived() override final;

  // unexposed API
  const std::unordered_map<tdi_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<tdi_id_t, uint32_t> &getU32FieldDataMap() const {
    return u32FieldData;
  }
  const std::unordered_map<tdi_id_t, int32_t> &getI32FieldDataMap() const {
    return i32FieldData;
  }
  const std::unordered_map<tdi_id_t, std::string> &getStrFieldDataMap() const {
    return strFieldData;
  }

 private:
  tdi_status_t setU32ValueInternal(const tdi_id_t &field_id,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr,
                                   const size_t &s);
  tdi_status_t getU32ValueInternal(const tdi_id_t &field_id,
                                   uint64_t *value,
                                   uint8_t *value_ptr,
                                   const size_t &s) const;

  std::unordered_map<tdi_id_t, bool> boolFieldData;
  std::unordered_map<tdi_id_t, uint32_t> u32FieldData;
  std::unordered_map<tdi_id_t, uint64_t> u64FieldData;
  std::unordered_map<tdi_id_t, int32_t> i32FieldData;
  std::unordered_map<tdi_id_t, std::string> strFieldData;
};

class PortStatTableData : public tdi::TableData {
 public:
  PortStatTableData(const tdi::Table *tbl_obj,
                    const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, fields) {
    std::memset(u64FieldDataArray,
                0,
                (BF_NUM_RMON_COUNTERS + BF_NUM_PKT_RATE_COUNTERS +
                 BF_NUM_TIME_STAMP_COUNTERS) *
                    sizeof(uint64_t));
  }
  ~PortStatTableData() = default;
  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_status_t resetDerived() override final;

  // unexposed API
  void setAllValues(const uint64_t *stats);
  const uint64_t *getU64FieldData() const { return u64FieldDataArray; }
  const uint32_t &getAllStatsBoundry() const { return AllStatsBoundry; }

 private:
  tdi_status_t setU64ValueInternal(const tdi_id_t &field_id,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr,
                                   const size_t &s);
  tdi_status_t getU64ValueInternal(const tdi_id_t &field_id,
                                   uint64_t *value,
                                   uint8_t *value_ptr,
                                   const size_t &s) const;

  uint64_t u64FieldDataArray[BF_NUM_RMON_COUNTERS + BF_NUM_PKT_RATE_COUNTERS +
                             BF_NUM_TIME_STAMP_COUNTERS];
  const uint32_t AllStatsBoundry = 20;
};

class PortStrInfoTableData : public tdi::TableData {
 public:
  PortStrInfoTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj), dev_port_() {}
  ~PortStrInfoTableData() = default;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;
  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  tdi_status_t resetDerived() override final;
  tdi_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return TDI_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

class PortHdlInfoTableData : public tdi::TableData {
 public:
  PortHdlInfoTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj), dev_port_() {}
  ~PortHdlInfoTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;
  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  tdi_status_t resetDerived() override final;

  tdi_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return TDI_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

class PortFpIdxInfoTableData : public tdi::TableData {
 public:
  PortFpIdxInfoTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj), dev_port_() {}
  ~PortFpIdxInfoTableData() = default;
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

  tdi_status_t resetDerived() override final;

  tdi_status_t getDevPort(uint32_t *id) const {
    *id = dev_port_;
    return TDI_SUCCESS;
  };
  void setDevPort(const uint32_t &id) { dev_port_ = id; }

 private:
  uint32_t dev_port_;
};

}  // namespace tdi
#endif  // _BF_RT_PORT_TABLE_DATA_IMPL_HPP
