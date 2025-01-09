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

#include "bf_rt_pktgen_table_data_impl.hpp"
#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

namespace bfrt {

const std::map<std::string, bf_pktgen_port_down_mode_t>
    BfRtPktgenPortDownReplayCfgTableData::string_to_portdown_reply_mode_map{
        {"REPLAY_NONE", BF_PKTGEN_PORT_DOWN_REPLAY_NONE},
        {"REPLAY_ALL", BF_PKTGEN_PORT_DOWN_REPLAY_ALL},
        {"REPLAY_MISSED", BF_PKTGEN_PORT_DOWN_REPLAY_MISSED}};

const std::map<bf_pktgen_port_down_mode_t, std::string>
    BfRtPktgenPortDownReplayCfgTableData::portdown_reply_mode_to_string_map{
        {BF_PKTGEN_PORT_DOWN_REPLAY_NONE, "REPLAY_NONE"},
        {BF_PKTGEN_PORT_DOWN_REPLAY_ALL, "REPLAY_ALL"},
        {BF_PKTGEN_PORT_DOWN_REPLAY_MISSED, "REPLAY_MISSED"}};

bf_status_t BfRtPktgenPortTableData::setValue(const bf_rt_id_t &field_id,
                                              const bool &value) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
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
  if (!this->allFieldsSet()) {
    auto elem1 = this->getActiveFields().find(field_id);
    if (elem1 == this->getActiveFields().end()) {
      LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
                __func__,
                __LINE__,
                field_id,
                this->table_->table_name_get().c_str());
      return BF_INVALID_ARG;
    }
  }
  boolFieldData[field_id] = value;
  return status;
}

bf_status_t BfRtPktgenPortTableData::getValue(const bf_rt_id_t &field_id,
                                              bool *value) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
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
  return status;
}

bf_status_t BfRtPktgenAppTableData::setValue(const bf_rt_id_t &field_id,
                                             const bool &value) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(
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
  if (!checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  boolFieldData[field_id] = value;
  return status;
}

bf_status_t BfRtPktgenAppTableData::setValueInternal(const bf_rt_id_t &field_id,
                                                     const uint64_t &value,
                                                     const uint8_t *value_ptr,
                                                     const size_t &s) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
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

  status = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }
  if (!checkFieldActive(field_id)) {
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
    std::copy(value_ptr, value_ptr + s, arrayFieldData[field_id].begin());
  }
  return status;
}

bf_status_t BfRtPktgenAppTableData::setValue(const bf_rt_id_t &field_id,
                                             const uint64_t &value) {
  return setValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPktgenAppTableData::setValue(const bf_rt_id_t &field_id,
                                             const uint8_t *value,
                                             const size_t &size) {
  return setValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtPktgenAppTableData::getValue(const bf_rt_id_t &field_id,
                                             bool *value) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(
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
  return status;
}

bf_status_t BfRtPktgenAppTableData::getValueInternal(const bf_rt_id_t &field_id,
                                                     uint64_t *value,
                                                     uint8_t *value_ptr,
                                                     const size_t &s) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(
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
    }
    if (s < pattern_size) return BF_INVALID_ARG;
    std::copy(elem1->second.begin(), elem1->second.end(), value_ptr);
  } else {
    uint64_t val = elem->second;
    if (value_ptr) {
      utils::BfRtTableFieldUtils::toNetworkOrderData(
          *tableDataField, val, value_ptr);
    } else {
      *value = val;
    }
  }
  return status;
}

bf_status_t BfRtPktgenAppTableData::getValue(const bf_rt_id_t &field_id,
                                             uint64_t *value) const {
  return getValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPktgenAppTableData::getValue(const bf_rt_id_t &field_id,
                                             const size_t &size,
                                             uint8_t *value) const {
  return getValueInternal(field_id, nullptr, value, size);
}

bf_status_t BfRtPktgenPortMaskTableData::setValue(const bf_rt_id_t &field_id,
                                                  const uint8_t *value,
                                                  const size_t &size) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, NULL, value, size);
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
  status = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, 0, value, size);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              tableDataField->getId());
    return status;
  }
  // Network order to host order
  std::reverse_copy(value, value + size, port_down_mask_.begin());
  return status;
}

bf_status_t BfRtPktgenPortMaskTableData::getValue(const bf_rt_id_t &field_id,
                                                  const size_t &size,
                                                  uint8_t *value) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, NULL, value, size);
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
  // Host order to network order conversion
  std::reverse_copy(port_down_mask_.begin(), port_down_mask_.end(), value);
  return status;
}

bf_status_t BfRtPktgenPktBufferTableData::setValue(const bf_rt_id_t &field_id,
                                                   const uint8_t *value,
                                                   const size_t &size) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  data_.insert(data_.end(), value, value + size);
  return status;
}

bf_status_t BfRtPktgenPktBufferTableData::setValue(
    const bf_rt_id_t &field_id, const std::vector<bf_rt_id_t> &arr) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  data_.insert(data_.end(), arr.begin(), arr.end());
  return status;
}

bf_status_t BfRtPktgenPktBufferTableData::getValue(const bf_rt_id_t &field_id,
                                                   const size_t &size,
                                                   uint8_t *value) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  if (size > data_.size()) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Requested data size %zu larger than valid data size "
        "%zu for field_id %d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        size,
        data_.size(),
        tableDataField->getId());
    return BF_INVALID_ARG;
  }
  std::copy(data_.begin(), data_.end(), value);
  return status;
}

bf_status_t BfRtPktgenPktBufferTableData::getValue(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  if (arr->size() > data_.size()) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Requested data size %zu larger than valid data size "
        "%zu for field_id %d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        arr->size(),
        data_.size(),
        tableDataField->getId());
    return BF_INVALID_ARG;
  }
  arr->insert(arr->end(), data_.begin(), data_.end());
  return status;
}

bf_status_t BfRtPktgenPortDownReplayCfgTableData::setValue(
    const bf_rt_id_t &field_id, const std::string &str) {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
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
  auto tmp = string_to_portdown_reply_mode_map.find(str);
  if (tmp == string_to_portdown_reply_mode_map.end()) {
    LOG_ERROR("%s:%d Invalid port down replay mode %s",
              __func__,
              __LINE__,
              str.c_str());
    return BF_NOT_SUPPORTED;
  }
  bf_mode_ = tmp->second;
  mode_ = str;
  return status;
}

bf_status_t BfRtPktgenPortDownReplayCfgTableData::getValue(
    const bf_rt_id_t &field_id, std::string *str) const {
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = this->table_->getDataField(field_id, &tableDataField);
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
  if (mode_.empty()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *str = mode_;
  return status;
}

void BfRtPktgenPortDownReplayCfgTableData::set_mode(
    bf_pktgen_port_down_mode_t mode) {
  bf_mode_ = mode;
  if (portdown_reply_mode_to_string_map.find(mode) !=
      portdown_reply_mode_to_string_map.end()) {
    mode_ = portdown_reply_mode_to_string_map.at(mode);
  } else {
    LOG_ERROR("ERROR: %s:%d %s : Invalid port down replay mode %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              static_cast<int>(mode));
  }
}
}  // namespace bfrt
