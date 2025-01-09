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

#include "tdi/common/tdi_defs.h"
#include "tdi_mirror_table_data_impl.hpp"
#include <tdi_common/tdi_table_field_utils.hpp>
#include <tdi/common/tdi_utils.hpp>

namespace tdi {
namespace tna {
namespace tofino {

tdi_status_t MirrorCfgTableData::resetDerived() {
  u64FieldData.clear();
  boolFieldData.clear();
  strFieldData.clear();
  arrayFieldData.clear();
  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::setValue(const tdi_id_t &field_id,
                                          const bool &value) {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  bool is_active = false;
  this->isActive(field_id, &is_active);
  if (!is_active) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  boolFieldData[field_id] = value;

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::setValue(const tdi_id_t &field_id,
                                          const std::string &str) {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  bool is_active = false;
  this->isActive(field_id, &is_active);
  if (!is_active) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  strFieldData[field_id] = str;

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::setValueInternal(const tdi_id_t &field_id,
                                                  const uint64_t &value,
                                                  const uint8_t *value_ptr,
                                                  const size_t &s) {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Do bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }

  sts = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }

  bool is_active = false;
  this->isActive(field_id, &is_active);
  if (!is_active) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  if (s <= sizeof(uint64_t)) {
    uint64_t val = 0;
    if (value_ptr) {
      utils::TableFieldUtils::toHostOrderData(*tableDataField, value_ptr, &val);
    } else {
      val = value;
    }
    u64FieldData[field_id] = val;
  } else {
    std::array<uint8_t, TDI_MIRROR_INT_HDR_SIZE> tmp = {0};
    for (size_t i = 0; i < s; i++) {
      // network endian, reverse in pipe_mgr before configure into registers.
      tmp[i] = value_ptr[i];
    }
    arrayFieldData[field_id] = tmp;
  }

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::setValue(const tdi_id_t &field_id,
                                          const uint64_t &value) {
  return setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MirrorCfgTableData::setValue(const tdi_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &size) {
  return setValueInternal(field_id, 0, value, size);
}

tdi_status_t MirrorCfgTableData::getValue(const tdi_id_t &field_id,
                                          bool *value) const {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  *value = elem->second;

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::getValue(const tdi_id_t &field_id,
                                          std::string *str) const {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  *str = elem->second;

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::getValueInternal(const tdi_id_t &field_id,
                                                  uint64_t *value,
                                                  uint8_t *value_ptr,
                                                  const size_t &s) const {
  const auto *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, this->actionIdGet());
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Do bounds checking using the utility functions
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
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
                this->table_->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_INVALID_ARG;
    } else {
      if (s < TDI_MIRROR_INT_HDR_SIZE) return TDI_INVALID_ARG;
      for (size_t i = 0; i < s; i++) {
        // would reverse endian in pipe_mgr.
        value_ptr[i] = (elem1->second)[i];
      }
    }
  } else {
    uint64_t val = elem->second;
    if (value_ptr) {
      utils::TableFieldUtils::toNetworkOrderData(
          *tableDataField, val, value_ptr);
    } else {
      *value = val;
    }
  }

  return TDI_SUCCESS;
}

tdi_status_t MirrorCfgTableData::getValue(const tdi_id_t &field_id,
                                          uint64_t *value) const {
  return getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MirrorCfgTableData::getValue(const tdi_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  return getValueInternal(field_id, nullptr, value, size);
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
