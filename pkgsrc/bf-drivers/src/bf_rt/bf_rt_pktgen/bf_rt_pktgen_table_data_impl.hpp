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


#ifndef _BF_RT_PKTGEN_TABLE_DATA_IMPL_HPP
#define _BF_RT_PKTGEN_TABLE_DATA_IMPL_HPP

#include <array>
#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#define pattern_size 16
namespace bfrt {

class BfRtPktgenPortTableData : public BfRtTableDataObj {
 public:
  BfRtPktgenPortTableData(const BfRtTableObj *tbl_obj,
                          const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    this->setActiveFields(fields);
  }
  virtual ~BfRtPktgenPortTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value) override;

  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value) const override;
  // unexposed API

  const std::map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }

 private:
  std::map<bf_rt_id_t, bool> boolFieldData;
};

class BfRtPktgenAppTableData : public BfRtTableDataObj {
 public:
  BfRtPktgenAppTableData(const BfRtTableObj *tbl_obj,
                         const bf_rt_id_t &act_id,
                         const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->setActiveFields(fields);
  }
  ~BfRtPktgenAppTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  // unexposed API
  const std::map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::map<bf_rt_id_t, uint64_t> &getU64FieldDataMap() const {
    return u64FieldData;
  }
  const std::map<bf_rt_id_t, std::array<uint8_t, pattern_size>>
      &getArrayFieldDataMap() const {
    return arrayFieldData;
  }

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;
  std::map<bf_rt_id_t, uint64_t> u64FieldData;
  std::map<bf_rt_id_t, bool> boolFieldData;
  std::map<bf_rt_id_t, std::array<uint8_t, pattern_size>> arrayFieldData;
};

class BfRtPktgenPortMaskTableData : public BfRtTableDataObj {
 public:
  BfRtPktgenPortMaskTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj) {}
  ~BfRtPktgenPortMaskTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  // unexposed API
  void setMask(const uint8_t *value, const size_t &size) {
    std::memcpy(
        port_down_mask_.data(), value, std::min(size, port_down_mask_.size()));
  }
  const uint8_t *getMask() const { return port_down_mask_.data(); }

 private:
  // port down mask array size is 72 bits, fixed, defined in chip.
  std::array<uint8_t, 9> port_down_mask_;
};

class BfRtPktgenPktBufferTableData : public BfRtTableDataObj {
 public:
  BfRtPktgenPktBufferTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj) {}
  ~BfRtPktgenPktBufferTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override;

  // exposed API
  const std::vector<uint8_t> &getData() const { return data_; }

 private:
  std::vector<uint8_t> data_;
};

class BfRtPktgenPortDownReplayCfgTableData : public BfRtTableDataObj {
 public:
  BfRtPktgenPortDownReplayCfgTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj), bf_mode_(BF_PKTGEN_PORT_DOWN_REPLAY_NONE) {}
  ~BfRtPktgenPortDownReplayCfgTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override;

  // unexposed API
  bf_pktgen_port_down_mode_t get_mode() const { return bf_mode_; }
  void set_mode(bf_pktgen_port_down_mode_t mode);

 private:
  std::string mode_;
  bf_pktgen_port_down_mode_t bf_mode_;
  static const std::map<std::string, bf_pktgen_port_down_mode_t>
      string_to_portdown_reply_mode_map;
  static const std::map<bf_pktgen_port_down_mode_t, std::string>
      portdown_reply_mode_to_string_map;
};

}  // namespace bfrt
#endif  // _BF_RT_PKTGEN_TABLE_DATA_IMPL_HPP
