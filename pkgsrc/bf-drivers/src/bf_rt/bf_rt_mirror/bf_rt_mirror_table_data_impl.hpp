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


#ifndef _BF_RT_MIRROR_TABLE_DATA_IMPL_HPP
#define _BF_RT_MIRROR_TABLE_DATA_IMPL_HPP

#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#define BF_RT_MIRROR_INT_HDR_SIZE 16
namespace bfrt {

class BfRtMirrorCfgTableData : public BfRtTableDataObj {
 public:
  BfRtMirrorCfgTableData(const BfRtTableObj *tbl_obj,
                         const bf_rt_id_t &act_id,
                         const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->setActiveFields(fields);
  }

  ~BfRtMirrorCfgTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;

  bf_status_t reset() override final;
  bf_status_t reset(const bf_rt_id_t &action_id) override final;
  bf_status_t reset(const bf_rt_id_t &action_id,
                    const std::vector<bf_rt_id_t> &fields) override final;

  const std::unordered_map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<bf_rt_id_t, uint64_t> &getU64FieldDataMap() const {
    return u64FieldData;
  }

  const std::unordered_map<bf_rt_id_t, std::string> &getStrFieldDataMap()
      const {
    return strFieldData;
  }

  const std::unordered_map<bf_rt_id_t,
                           std::array<uint8_t, BF_RT_MIRROR_INT_HDR_SIZE>>
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
  std::unordered_map<bf_rt_id_t, uint64_t> u64FieldData;
  std::unordered_map<bf_rt_id_t, bool> boolFieldData;
  std::unordered_map<bf_rt_id_t, std::string> strFieldData;
  std::unordered_map<bf_rt_id_t, std::array<uint8_t, BF_RT_MIRROR_INT_HDR_SIZE>>
      arrayFieldData;
};

}  // namespace bfrt
#endif  // _BF_RT_MIRROR_TABLE_DATA_IMPL_HPP
