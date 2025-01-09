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

#ifndef _TDI_ENTRY_GEN_HPP
#define _TDI_ENTRY_GEN_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <tdi/tdi.hpp>
#include <target-sys/bf_sal/bf_sys_assert.h>

namespace tdi {

class GenericKeyField {
 public:
  GenericKeyField(const TdiTable &table, const tdi_id_t &field_id);
  const std::vector<uint8_t> &getValue() const { return value_byte_; }
  template <typename T>
  void getValue(T *value) const;

  template <typename T>
  void setValue(const T &value);

  const std::vector<uint8_t> &getMask() const { return mask_byte_; }
  template <typename T>
  void getMask(T *mask) const;

  const uint16_t &getPrefixLen() const { return p_len_; };
  const size_t &sizeGet() const { return size_; };
  const tdi_id_t &fieldIdGet() const { return field_id_; };
  void fillKeyField(const TdiTable &table);

  // private:
  std::vector<uint8_t> value_byte_;
  std::vector<uint8_t> mask_byte_;
  uint16_t p_len_;
  size_t size_;       // bytes
  size_t size_bits_;  // bits
  tdi_id_t field_id_;
};

class GenericKey {
 public:
  GenericKey(const TdiTable &table);
  std::vector<tdi_id_t> fieldIdListGet() const;
  void regenerate();
  void regenerate(tdi_id_t field_id);
  const GenericKeyField &keyFieldGet(tdi_id_t field_id) const;
  const TdiTable &tableGet() const { return table_; }
  uint32_t getActionMbrId() const {
    bf_sys_assert(table_type_ == TdiTable::TableType::ACTION_PROFILE);
    bf_sys_assert(key_field_list_.size() == 1);
    const auto &key_field = key_field_list_[0];
    uint32_t mbr_id = 0;
    key_field.getValue<uint32_t>(&mbr_id);
    return mbr_id;
  }

  void setActionMbrId(const uint32_t &mbr_id) {
    bf_sys_assert(table_type_ == TdiTable::TableType::ACTION_PROFILE);
    bf_sys_assert(key_field_list_.size() == 1);
    auto &key_field = key_field_list_[0];
    key_field.setValue<uint32_t>(mbr_id);
  }
  // private:
  std::vector<GenericKeyField> key_field_list_;
  const TdiTable &table_;
  TdiTable::TableType table_type_;
};

// forward declaration
class GenericData;
class GenericDataField {
 public:
  GenericDataField(GenericData *gen_data,
                   const TdiTable &table,
                   const tdi_id_t &field_id,
                   tdi_id_t action_id = 0);
  const std::vector<uint8_t> &getValue() const { return value_byte_; }
  std::vector<uint32_t> getValue_intarr() const { return value_int_arr_; }
  std::vector<bool> getValue_boolarr() const { return value_bool_arr_; }
  template <typename T>
  void getValue(T *value) const;
  void getValue(std::string *val) const;
  const size_t &sizeGet() const { return size_; };
  const tdi_id_t &fieldIdGet() const { return field_id_; };
  void fillDataField(const TdiTable &table, const tdi_id_t &action_id);
  void fillDataFieldTTL(const TdiTable &table,
                        const tdi_id_t &action_id,
                        const uint64_t &min_ttl,
                        const uint64_t &max_ttl);
  std::string getName() const { return field_name_; }

  // private:
  std::vector<uint8_t> value_byte_;
  std::vector<uint32_t> value_int_arr_;
  std::vector<bool> value_bool_arr_;
  std::string value_string_;
  float value_float_;
  size_t size_;       // bytes
  size_t size_bits_;  // bits
  std::string field_name_;
  GenericData *parent_generic_data_{nullptr};
  tdi_id_t field_id_;

 private:
  template <typename T>
  void setValue(const T &value);
};

class GenericData {
 public:
  GenericData(const TdiTable &table);

  // Since we store the parent pointer in the data field, we need
  // define a copy constructor. Consider the foll case. We return a vector of
  // data entries to the user by value. This will implicitly incur copying
  // of the data objects. Thus the actual addresses of the parent data objs
  // will change. But the parent pointer in the DataField will be shallow
  // copied and thus will be still pointing to the old data obj address
  // Thus we need to explicitly define a copy constructor and overwrite
  // the parent pointer in the DataField objs
  GenericData(const GenericData &old) : table_(old.table_) {
    action_id_ = old.action_id_;
    max_group_size_ = old.max_group_size_;
    mbr_count_ = old.mbr_count_;
    table_type_ = old.table_type_;
    data_field_list_ = old.data_field_list_;
    for (uint32_t i = 0; i < old.data_field_list_.size(); i++) {
      // Update the parent_generic_data_ pointer in the DataField objs
      data_field_list_[i].parent_generic_data_ = this;
    }
  }

  std::vector<tdi_id_t> fieldIdListGet() const;
  const tdi_id_t &actionIdGet() const { return action_id_; };
  void regenerate();
  void regenerate(tdi_id_t field_id);
  void regenerateTTL(tdi_id_t field_id,
                     const uint64_t &min_ttl,
                     const uint64_t &max_ttl);
  const GenericDataField &dataFieldGet(tdi_id_t field_id) const;
  std::vector<GenericDataField> &dataFieldListGet() { return data_field_list_; }
  const TdiTable &tableGet() const { return table_; }
  uint32_t getMaxGroupSize() const {
    bf_sys_assert(table_type_ == TdiTable::TableType::SELECTOR);
    return max_group_size_;
  }
  uint32_t getGroupMbrCount() const {
    bf_sys_assert(table_type_ == TdiTable::TableType::SELECTOR);
    return mbr_count_;
  }
  std::vector<uint32_t> getGroupMbrs() const {
    bf_sys_assert(table_type_ == TdiTable::TableType::SELECTOR);
    for (const auto &data_field : data_field_list_) {
      const auto field_name = data_field.getName();
      if (field_name == "$ACTION_MEMBER_ID") {
        return data_field.getValue_intarr();
      }
    }
    return std::vector<uint32_t>{};
  }
  std::vector<bool> getGroupMbrSts() const {
    bf_sys_assert(table_type_ == TdiTable::TableType::SELECTOR);
    for (const auto &data_field : data_field_list_) {
      const auto field_name = data_field.getName();
      if (field_name == "$ACTION_MEMBER_STATUS") {
        return data_field.getValue_boolarr();
      }
    }
    return std::vector<bool>{};
  }

  // private:
  std::vector<GenericDataField> data_field_list_;
  tdi_id_t action_id_;
  uint32_t max_group_size_{0};  // max group size
  uint32_t mbr_count_{
      0xffffffff};  // Number of members(active + inactive) in the group
  const TdiTable &table_;
  TdiTable::TableType table_type_;
};

class TdiEntryGen {
 public:
  static std::unique_ptr<TdiEntryGen> makeGenerator(const TdiTable *table);
  virtual std::vector<GenericKey> getTableKeyEntries(
      uint32_t num_entries) const = 0;
  virtual std::vector<GenericData> getTableDataEntries(
      uint32_t num_entries) const = 0;

  static void setRandomSeed(const time_t &seed) { srand(seed); }

 protected:
  const TdiTable *table_;
  TdiEntryGen(const TdiTable *table) : table_(table){};
};

class TdiMatchActionTableGen : public TdiEntryGen {
 public:
  TdiMatchActionTableGen(const TdiTable *table) : TdiEntryGen(table){};
  std::vector<GenericKey> getTableKeyEntries(uint32_t num_entries) const;
  std::vector<GenericData> getTableDataEntries(uint32_t num_entries) const;
};

}  // namespace tdi

#endif  // _TDI_ENTRY_GEN_HPP
