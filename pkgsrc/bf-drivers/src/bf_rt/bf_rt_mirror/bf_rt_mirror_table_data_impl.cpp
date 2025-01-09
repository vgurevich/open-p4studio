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
#include <array>

#include "bf_rt_mirror_table_data_impl.hpp"
#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

namespace bfrt {

bf_status_t BfRtMirrorCfgTableData::reset(
    const bf_rt_id_t &action_id, const std::vector<bf_rt_id_t> &fields) {
  u64FieldData.clear();
  boolFieldData.clear();
  strFieldData.clear();
  arrayFieldData.clear();
  this->actionIdSet(action_id);
  return this->setActiveFields(fields);
}

bf_status_t BfRtMirrorCfgTableData::reset(const bf_rt_id_t &action_id) {
  std::vector<bf_rt_id_t> emptyfield;
  return this->reset(action_id, emptyfield);
}

bf_status_t BfRtMirrorCfgTableData::reset() {
  std::vector<bf_rt_id_t> emptyfield;
  return this->reset(0, emptyfield);
}

bf_status_t BfRtMirrorCfgTableData::setValue(const bf_rt_id_t &field_id,
                                             const bool &value) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  boolFieldData[field_id] = value;

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableData::setValueInternal(const bf_rt_id_t &field_id,
                                                     const uint64_t &value,
                                                     const uint8_t *value_ptr,
                                                     const size_t &s) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do bounds checking using the utility functions
  auto sts = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  sts = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  if (s <= sizeof(uint64_t)) {
    uint64_t val = 0;
    if (value_ptr) {
      utils::BfRtTableFieldUtils::toHostOrderData(
          *tableDataField, value_ptr, &val);
    } else {
      val = value;
    }
    u64FieldData[field_id] = val;
  } else {
    std::array<uint8_t, BF_RT_MIRROR_INT_HDR_SIZE> tmp = {0};
    for (size_t i = 0; i < s; i++) {
      // network endian, reverse in pipe_mgr before configure into registers.
      tmp[i] = value_ptr[i];
    }
    arrayFieldData[field_id] = tmp;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableData::setValue(const bf_rt_id_t &field_id,
                                             const uint64_t &value) {
  return setValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtMirrorCfgTableData::setValue(const bf_rt_id_t &field_id,
                                             const uint8_t *value,
                                             const size_t &size) {
  return setValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtMirrorCfgTableData::setValue(const bf_rt_id_t &field_id,
                                             const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  strFieldData[field_id] = str;

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableData::getValue(const bf_rt_id_t &field_id,
                                             bool *value) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *value = elem->second;

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableData::getValueInternal(const bf_rt_id_t &field_id,
                                                     uint64_t *value,
                                                     uint8_t *value_ptr,
                                                     const size_t &s) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }

  auto elem = u64FieldData.find(field_id);
  if (elem == u64FieldData.end()) {
    // u64 and int array type fields share the same setter and getter.
    auto elem1 = arrayFieldData.find(field_id);
    if (elem1 == arrayFieldData.end()) {
      LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    } else {
      if (s < BF_RT_MIRROR_INT_HDR_SIZE) return BF_INVALID_ARG;
      for (size_t i = 0; i < s; i++) {
        // would reverse endian in pipe_mgr.
        value_ptr[i] = (elem1->second)[i];
      }
    }
  } else {
    uint64_t val = elem->second;
    if (value_ptr) {
      utils::BfRtTableFieldUtils::toNetworkOrderData(
          *tableDataField, val, value_ptr);
    } else {
      *value = val;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableData::getValue(const bf_rt_id_t &field_id,
                                             uint64_t *value) const {
  return getValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtMirrorCfgTableData::getValue(const bf_rt_id_t &field_id,
                                             const size_t &size,
                                             uint8_t *value) const {
  return getValueInternal(field_id, nullptr, value, size);
}

bf_status_t BfRtMirrorCfgTableData::getValue(const bf_rt_id_t &field_id,
                                             std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *str = elem->second;

  return BF_SUCCESS;
}

}  // namespace bfrt
