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


#include "bf_rt_tm_table_data_impl.hpp"
#include <bf_rt_common/bf_rt_utils.hpp>

namespace bfrt {

bf_status_t BfRtTMTableData::getDataField(
    const bf_rt_id_t &field_id,
    const DataType val_type,
    const BfRtTableDataField **field_ptr) const {
  bf_status_t status = BF_SUCCESS;

  LOG_DBG("%s:%d %s field_id=%d acton_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          this->actionIdGet_());
  *field_ptr = nullptr;

  if (this->table_->actionIdApplicable()) {
    /* When the table is with actions. */
    status =
        this->table_->getDataField(field_id, this->actionIdGet_(), field_ptr);
  } else {
    status = this->table_->getDataField(field_id, field_ptr);
  }
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field_id=%d action_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id,
              this->actionIdGet_());
    return BF_INVALID_ARG;
  }
  if ((*field_ptr)->getDataType() != val_type) {
    LOG_ERROR("%s:%d %s Incompatible type %u for field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              static_cast<unsigned int>(val_type),
              field_id);
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::setIntValue(const bf_rt_id_t &field_id,
                                         const uint64_t &value,
                                         const uint8_t *value_ptr,
                                         const size_t &siz) {
  LOG_DBG("%s:%d %s field_id=%d size=%zu",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          siz);
  const BfRtTableDataField *tableDataField = nullptr;

  auto status = this->getDataField(field_id, DataType::UINT64, &tableDataField);
  if (status != BF_SUCCESS) {
    bf_status_t status_ =
        this->getDataField(field_id, DataType::BYTE_STREAM, &tableDataField);
    if (status_ == BF_SUCCESS) {
      status = BF_NOT_IMPLEMENTED;
    }
    LOG_ERROR("%s:%d %s Invalid field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Can't set inactive field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, siz);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Incompatible field type for field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  status = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, siz);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Bounds check failed for field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  uint64_t out_val = 0;

  if (value_ptr == nullptr) {
    out_val = value;
  } else {
    utils::BfRtTableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &out_val);
  }
  this->int_map[field_id] = out_val;
  this->addAssignedField(field_id);

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const uint64_t &value) {
  LOG_DBG("%s:%d %s set field_id=%d uint64_t",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  return this->setIntValue(field_id, value, nullptr, 0);
}

/* For  DataType::UINT64 or DataType::BYTE_STREAM */
bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const uint8_t *value,
                                      const size_t &size) {
  LOG_DBG("%s:%d %s set field_id=%d uint8_t[%zu]",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          size);
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }
  return this->setIntValue(field_id, 0, value, size);
}

/* For DataType::INT_ARR */
bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const std::vector<bf_rt_id_t> &arr) {
  LOG_DBG("%s:%d %s field_id=%d vector<bf_rd_id_t> with %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr.size());
  const BfRtTableDataField *tableDataField = nullptr;

  auto status =
      this->getDataField(field_id, DataType::INT_ARR, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Can't set inactive field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  this->vect_map[field_id] = arr;
  this->addAssignedField(field_id);

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const std::vector<std::string> &arr) {
  LOG_DBG("%s:%d %s field_id=%d vector<std::string> with %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr.size());
  const BfRtTableDataField *tableDataField = nullptr;

  auto status =
      this->getDataField(field_id, DataType::STRING_ARR, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Can't set inactive field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  this->vect_str_map[field_id] = arr;
  this->addAssignedField(field_id);

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const bool &value) {
  LOG_DBG("%s:%d %s set field_id=%d bool",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->getDataField(field_id, DataType::BOOL, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Can't set inactive field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  this->bool_map[field_id] = value;
  this->addAssignedField(field_id);

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::setValue(const bf_rt_id_t &field_id,
                                      const std::string &value) {
  LOG_DBG("%s:%d %s set field_id=%d string",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);

  const BfRtTableDataField *tableDataField = nullptr;

  auto status = this->getDataField(field_id, DataType::STRING, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Can't set inactive field_id=%d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  this->str_map[field_id] = value;
  this->addAssignedField(field_id);

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::getIntValue(const bf_rt_id_t &field_id,
                                         uint64_t *value,
                                         uint8_t *value_ptr,
                                         const size_t &size) const {
  LOG_DBG("%s:%d %s field_id=%d size=%zu",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          size);

  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->getDataField(field_id, DataType::UINT64, &tableDataField);
  if (status != BF_SUCCESS) {
    bf_status_t status_ =
        this->getDataField(field_id, DataType::BYTE_STREAM, &tableDataField);
    if (status_ == BF_SUCCESS) {
      status = BF_NOT_IMPLEMENTED;
    }
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Attempt to get from inactive field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Check the compatability of data
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Incompatible field type for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  auto item = this->int_map.find(field_id);
  if (item != this->int_map.end()) {
    uint64_t out_val = item->second;
    if (value_ptr) {
      utils::BfRtTableFieldUtils::toNetworkOrderData(
          *tableDataField, out_val, value_ptr);
    } else {
      *value = out_val;
    }
  } else {
    LOG_ERROR("%s:%d %s field ID %d has no data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      uint64_t *value) const {
  LOG_DBG("%s:%d %s get field_id=%d uint64_t",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }
  return this->getIntValue(field_id, value, nullptr, 0);
}

/* For  DataType::UINT64 or DataType::BYTE_STREAM */
bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      const size_t &size,
                                      uint8_t *value_ptr) const {
  LOG_DBG("%s:%d %s get field_id=%d uint8_t[%zu]",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          size);
  if (value_ptr == nullptr) {
    return BF_INVALID_ARG;
  }
  return this->getIntValue(field_id, 0, value_ptr, size);
}

bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      bool *value) const {
  LOG_DBG("%s:%d %s get field_id=%d bool",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->getDataField(field_id, DataType::BOOL, &tableDataField);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Attempt to get inactive field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  auto item = this->bool_map.find(field_id);
  if (item == this->bool_map.end()) {
    LOG_ERROR("%s:%d %s field ID %d has no data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *value = item->second;

  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      std::string *value) const {
  LOG_DBG("%s:%d %s set field_id=%d string",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->getDataField(field_id, DataType::STRING, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Attempt to get inactive field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  if (str_map.find(field_id) == str_map.end()) {
    LOG_ERROR("%s:%d %s field ID %d has no data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *value = this->str_map.at(field_id);
  return BF_SUCCESS;
}

/* For DataType::INT_ARR */
bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      std::vector<bf_rt_id_t> *arr) const {
  if (arr == nullptr) {
    return BF_INVALID_ARG;
  }
  LOG_DBG("%s:%d %s field_id=%d vector<bf_rd_id_t> with %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr->size());

  const BfRtTableDataField *tableDataField = nullptr;

  auto status =
      this->getDataField(field_id, DataType::INT_ARR, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Attempt to get inactive field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  auto a_item_ = this->vect_map.find(field_id);
  if (a_item_ == this->vect_map.end()) {
    LOG_ERROR("%s:%d %s Active field id %d has no data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *arr = a_item_->second;
  LOG_DBG("%s:%d %s got field id %d vector of %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr->size());
  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::getValue(const bf_rt_id_t &field_id,
                                      std::vector<std::string> *arr) const {
  if (arr == nullptr) {
    LOG_ERROR("%s:%d %s null input for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  LOG_DBG("%s:%d %s field_id=%d vector<std::string> with %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr->size());

  const BfRtTableDataField *tableDataField = nullptr;

  auto status =
      this->getDataField(field_id, DataType::STRING_ARR, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Attempt to get inactive field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  auto a_item_ = this->vect_str_map.find(field_id);
  if (a_item_ == this->vect_str_map.end()) {
    LOG_ERROR("%s:%d %s Active field id %d has no data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *arr = a_item_->second;
  LOG_DBG("%s:%d %s got field id %d vector of %zu items",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id,
          arr->size());
  return BF_SUCCESS;
}

bf_status_t BfRtTMTableData::reset(const bf_rt_id_t &action_id,
                                   const std::vector<bf_rt_id_t> &fields) {
  LOG_DBG("%s:%d %s action_id=%d fields size= %zu",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          action_id,
          fields.size());

  this->bool_map.clear();
  this->int_map.clear();
  this->str_map.clear();
  this->vect_map.clear();
  this->vect_str_map.clear();

  this->resetAssignedFields();

  this->actionIdSet(action_id);

  return this->setActiveFields(fields);
}

bf_status_t BfRtTMTableData::reset(const bf_rt_id_t &action_id) {
  LOG_DBG("%s:%d %s action_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          action_id);
  std::vector<bf_rt_id_t> emptyFields;
  return this->reset(action_id, emptyFields);
}

bf_status_t BfRtTMTableData::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  std::vector<bf_rt_id_t> emptyFields;
  return this->reset(0, emptyFields);
}

bf_status_t BfRtTMTableData::setValueByName(
    const std::string &field_name, const std::vector<bf_rt_id_t> &arr) {
  bf_rt_id_t field_id;
  auto status = this->table_->dataFieldIdGet(field_name, &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  }
  if (this->checkFieldActive(field_id)) {
    status = this->setValue(field_id, arr);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't set data object for %s",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMTableData::getValueByName(
    const std::string &field_name, std::vector<bf_rt_id_t> *arr) const {
  bf_rt_id_t field_id;
  auto status = this->table_->dataFieldIdGet(field_name, &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Inactive field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  } else {
    status = this->getValue(field_id, arr);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't get data object for %s",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMTableData::setValueByName(
    const std::string &field_name, const std::vector<std::string> &arr) {
  bf_rt_id_t field_id;
  auto status = this->table_->dataFieldIdGet(field_name, &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  }
  if (this->checkFieldActive(field_id)) {
    status = this->setValue(field_id, arr);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't set data object for %s",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMTableData::getValueByName(
    const std::string &field_name, std::vector<std::string> *arr) const {
  bf_rt_id_t field_id;
  auto status = this->table_->dataFieldIdGet(field_name, &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  }
  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("%s:%d %s Inactive field %s",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_name.c_str());
    return status;
  } else {
    status = this->getValue(field_id, arr);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't get data object for %s",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
  }
  return status;
}
}  // namespace bfrt
