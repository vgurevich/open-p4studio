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
#include "tdi_port_table_impl.hpp"
#include "tdi_port_table_data_impl.hpp"
#include <tdi_tofino/tdi_common/tdi_table_field_utils.hpp>

namespace tdi {
namespace {
template <class T>
tdi_status_t getDevPortValue(const T &data,
                             const tdi::Table &table,
                             const tdi_id_t &field_id,
                             uint64_t *value,
                             uint8_t *value_ptr,
                             const size_t &size) {
  auto tableInfo = table.tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);

  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, value, value_ptr, size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  uint32_t dev_port;
  data.getDevPort(&dev_port);
  uint64_t val = static_cast<uint64_t>(dev_port);
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(*tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t setDevPortValue(T &data,
                             const Table &table,
                             const tdi_id_t &field_id,
                             const uint64_t &value,
                             const uint8_t *value_ptr,
                             const size_t &size) {
  auto tableInfo = table.tableInfoGet();
  const auto &tableDataField = tableInfo->dataFieldGet(field_id);

  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, &value, value_ptr, size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  sts = utils::TableFieldUtils::boundsCheck(
      table, *tableDataField, value, value_ptr, size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              tableDataField->idGet());
    return sts;
  }
  uint64_t local_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &local_val);
  } else {
    local_val = value;
  }
  const uint32_t dev_port = static_cast<uint32_t>(local_val);
  data.setDevPort(dev_port);
  return TDI_SUCCESS;
}
}  // anonymous namespace

// Port Cfg Table Data
tdi_status_t PortCfgTableData::resetDerived() {
  boolFieldData.clear();
  u32FieldData.clear();
  u64FieldData.clear();
  i32FieldData.clear();
  strFieldData.clear();
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::setU32ValueInternal(const tdi_id_t &field_id,
                                                   const uint64_t &value,
                                                   const uint8_t *value_ptr,
                                                   const size_t &s) {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
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
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(*tableDataField, value_ptr, &val);
  } else {
    val = value;
  }

  u32FieldData[field_id] = val;
  return TDI_SUCCESS;

#if 0
 std::string field_name;
 sts = this->table_->dataFieldNameGet(field_id, &field_name);
 //sts = tableInfo->dataFieldNameGet(field_id, &field_name);
 if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in getting field name for field %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return sts;
  }

  size_t field_size = 0;
  sts = tableInfo->dataFieldSizeGet(field_id, &field_size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Error : Failed to get the field size for %s",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_name.c_str());
    return sts;
  }

 field_size = (field_size + 7) / 8;
  if (sizeof(uint64_t) == field_size) {
    u64FieldData[field_id] = val;
  } else {
    u32FieldData[field_id] = val;
  }
#endif
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const uint64_t &value) {
  return setU32ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const uint8_t *value,
                                        const size_t &size) {
  return setU32ValueInternal(field_id, 0, value, size);
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const bool &value) {
  const DataFieldInfo *tableDataField = nullptr;
  tableDataField = (this->table_->tableInfoGet())->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        (this->table_->tableInfoGet())->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  boolFieldData[field_id] = value;
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const std::string &str) {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        tableInfo->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  strFieldData[field_id] = str;
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const int64_t &value) {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_INT64) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a int",
        __func__,
        __LINE__,
        field_id,
        tableInfo->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  i32FieldData[field_id] = value;
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::getU32ValueInternal(const tdi_id_t &field_id,
                                                   uint64_t *value,
                                                   uint8_t *value_ptr,
                                                   const size_t &s) const {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  auto elem = u32FieldData.find(field_id);
  if (elem == u32FieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  uint64_t val = elem->second;
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(*tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                        uint64_t *value) const {
  return getU32ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                        const size_t &size,
                                        uint8_t *value) const {
  return getU32ValueInternal(field_id, nullptr, value, size);
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                        bool *value) const {
  const DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        tableInfo->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  *value = elem->second;
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                        std::string *str) const {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        tableInfo->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  *str = elem->second;
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                        int64_t *value) const {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_INT64) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a int",
        __func__,
        __LINE__,
        field_id,
        tableInfo->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  auto elem = i32FieldData.find(field_id);
  if (elem == i32FieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  *value = elem->second;
  return TDI_SUCCESS;
}

tdi_status_t PortStatTableData::resetDerived() {
  std::memset(u64FieldDataArray, 0, sizeof(u64FieldDataArray));
  return TDI_SUCCESS;
}
void PortStatTableData::setAllValues(const uint64_t *stats) {
  if (stats == NULL) return;
  std::memcpy(
      u64FieldDataArray, stats, BF_NUM_RMON_COUNTERS * sizeof(uint64_t));
}

tdi_status_t PortStatTableData::setU64ValueInternal(const tdi_id_t &field_id,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr,
                                                    const size_t &s) {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
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
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(*tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  u64FieldDataArray[field_id - 1] = val;
  return TDI_SUCCESS;
}

tdi_status_t PortStatTableData::getU64ValueInternal(const tdi_id_t &field_id,
                                                    uint64_t *value,
                                                    uint8_t *value_ptr,
                                                    const size_t &s) const {
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (!tableDataField) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  bool is_active = false;
  if (this->isActive(field_id, &is_active) != TDI_SUCCESS || (!is_active)) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  uint64_t val = u64FieldDataArray[field_id - 1];
  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(*tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStatTableData::setValue(const tdi_id_t &field_id,
                                         const uint64_t &value) {
  return setU64ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortStatTableData::setValue(const tdi_id_t &field_id,
                                         const uint8_t *value,
                                         const size_t &size) {
  return setU64ValueInternal(field_id, 0, value, size);
}

tdi_status_t PortStatTableData::getValue(const tdi_id_t &field_id,
                                         uint64_t *value) const {
  return getU64ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortStatTableData::getValue(const tdi_id_t &field_id,
                                         const size_t &size,
                                         uint8_t *value) const {
  return getU64ValueInternal(field_id, nullptr, value, size);
}

// Port Str Info Table Data
tdi_status_t PortStrInfoTableData::resetDerived() {
  dev_port_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t PortStrInfoTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return setDevPortValue<PortStrInfoTableData>(
      *(this), *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortStrInfoTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setDevPortValue<PortStrInfoTableData>(
      *(this), *table_, field_id, 0, value, size);
}

tdi_status_t PortStrInfoTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return getDevPortValue<PortStrInfoTableData>(
      *this, *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortStrInfoTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getDevPortValue<PortStrInfoTableData>(
      *this, *table_, field_id, nullptr, value, size);
}

// Port Hdl Info Table Data
tdi_status_t PortHdlInfoTableData::resetDerived() {
  dev_port_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t PortHdlInfoTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return setDevPortValue<PortHdlInfoTableData>(
      *(this), *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortHdlInfoTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setDevPortValue<PortHdlInfoTableData>(
      *(this), *table_, field_id, 0, value, size);
}

tdi_status_t PortHdlInfoTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return getDevPortValue<PortHdlInfoTableData>(
      *this, *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortHdlInfoTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getDevPortValue<PortHdlInfoTableData>(
      *this, *table_, field_id, nullptr, value, size);
}

// Port Fp Idx Info Table Data
tdi_status_t PortFpIdxInfoTableData::resetDerived() {
  dev_port_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t PortFpIdxInfoTableData::setValue(const tdi_id_t &field_id,
                                              const uint64_t &value) {
  return setDevPortValue<PortFpIdxInfoTableData>(
      *(this), *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortFpIdxInfoTableData::setValue(const tdi_id_t &field_id,
                                              const uint8_t *value,
                                              const size_t &size) {
  return setDevPortValue<PortFpIdxInfoTableData>(
      *(this), *table_, field_id, 0, value, size);
}

tdi_status_t PortFpIdxInfoTableData::getValue(const tdi_id_t &field_id,
                                              uint64_t *value) const {
  return getDevPortValue<PortFpIdxInfoTableData>(
      *this, *table_, field_id, value, nullptr, 0);
}

tdi_status_t PortFpIdxInfoTableData::getValue(const tdi_id_t &field_id,
                                              const size_t &size,
                                              uint8_t *value) const {
  return getDevPortValue<PortFpIdxInfoTableData>(
      *this, *table_, field_id, nullptr, value, size);
}

}  // namespace tdi
