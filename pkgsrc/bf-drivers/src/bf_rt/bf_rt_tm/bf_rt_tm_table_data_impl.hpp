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


#ifndef _BF_RT_TM_TABLE_DATA_IMPL_HPP
#define _BF_RT_TM_TABLE_DATA_IMPL_HPP

#include <map>
#include <unordered_map>

#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

namespace bfrt {

class BfRtTMTableData : public BfRtTableDataObj {
 public:
  BfRtTMTableData(const BfRtTableObj *tbl_obj) : BfRtTableDataObj(tbl_obj) {}

  BfRtTMTableData(const BfRtTableObj *tbl_obj,
                  const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    this->setActiveFields(fields);
  }

  BfRtTMTableData(const BfRtTableObj *tbl_obj,
                  const bf_rt_id_t &act_id,
                  const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->setActiveFields(fields);
  }

  ~BfRtTMTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<std::string> &arr) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<std::string> *arr) const override final;

  // Override it if the table data has data-dependent action id.
  // bf_status_t actionIdGet(bf_rt_id_t *act_id) const override;

  bf_status_t reset() override final;
  bf_status_t reset(const bf_rt_id_t &action_id) override final;
  bf_status_t reset(const bf_rt_id_t &action_id,
                    const std::vector<bf_rt_id_t> &fields) override final;

  const std::unordered_map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return this->bool_map;
  }
  const std::unordered_map<bf_rt_id_t, uint64_t> &getIntFieldDataMap() const {
    return this->int_map;
  }
  const std::unordered_map<bf_rt_id_t, std::string> &getStrFieldDataMap()
      const {
    return this->str_map;
  }
  const std::unordered_map<bf_rt_id_t, std::vector<bf_rt_id_t>>
      &getVectorDataMap() const {
    return this->vect_map;
  }

  bool hasValue(bf_rt_id_t field_id_) const {
    return (this->checkFieldActive(field_id_) &&
            (this->is_set.find(field_id_) != this->is_set.end()));
  }

  std::size_t hasValues() const { return this->is_set.size(); }

  const std::set<bf_rt_id_t> &getAssignedFields() const { return this->is_set; }

  bf_status_t setValueByName(const std::string &field_name,
                             const std::vector<bf_rt_id_t> &arr);

  bf_status_t getValueByName(const std::string &field_name,
                             std::vector<bf_rt_id_t> *arr) const;

  bf_status_t setValueByName(const std::string &field_name,
                             const std::vector<std::string> &arr);

  bf_status_t getValueByName(const std::string &field_name,
                             std::vector<std::string> *arr) const;

 protected:
  void addAssignedField(const bf_rt_id_t &field_id) {
    this->is_set.insert(field_id);
  }

  void resetAssignedFields() { this->is_set.clear(); }

 private:
  bf_status_t getDataField(const bf_rt_id_t &field_id,
                           const DataType val_type,
                           const BfRtTableDataField **field_ptr) const;
  bf_status_t setIntValue(const bf_rt_id_t &field_id,
                          const uint64_t &value,
                          const uint8_t *value_ptr,
                          const size_t &s);
  bf_status_t getIntValue(const bf_rt_id_t &field_id,
                          uint64_t *value,
                          uint8_t *value_ptr,
                          const size_t &s) const;

  std::set<bf_rt_id_t> is_set;

  std::unordered_map<bf_rt_id_t, bool> bool_map;
  std::unordered_map<bf_rt_id_t, uint64_t> int_map;
  std::unordered_map<bf_rt_id_t, std::string> str_map;
  std::unordered_map<bf_rt_id_t, std::vector<bf_rt_id_t>> vect_map;
  std::unordered_map<bf_rt_id_t, std::vector<std::string>> vect_str_map;
};
}  // namespace bfrt
#endif
