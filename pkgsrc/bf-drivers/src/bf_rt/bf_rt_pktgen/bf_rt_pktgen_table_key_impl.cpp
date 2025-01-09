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


#include <bitset>
#include <arpa/inet.h>
#include <inttypes.h>

#include <bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_pktgen_table_key_impl.hpp"

namespace bfrt {
namespace {
template <class T>
bf_status_t getKeyIdxValue(const T &key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           uint64_t *value) {
  // Use getKeyFieldSafe to validate key type
  const BfRtTableKeyField *keyField;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &keyField, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  *value = key.getId();
  return BF_SUCCESS;
}

template <class T>
bf_status_t getKeyIdxValue(const T &key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           uint8_t *value,
                           const size_t &size) {
  // Use getKeyFieldSafe to validate key type
  const BfRtTableKeyField *keyField;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &keyField, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t local_val = htobe32(key.getId());
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return BF_SUCCESS;
}

template <class T>
bf_status_t setKeyIdxValue(T *key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           const uint64_t &value) {
  // Use getKeyFieldSafe to validate key type
  const BfRtTableKeyField *keyField;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &keyField, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  key->setId(value);
  return BF_SUCCESS;
}

template <class T>
bf_status_t setKeyIdxValue(T *key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           const uint8_t *value,
                           const size_t &size) {
  // Use getKeyFieldSafe to validate key type
  const BfRtTableKeyField *keyField;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &keyField, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  key->setId(val);
  return BF_SUCCESS;
}

}  // anonymous namespace

bf_status_t BfRtPktgenPortTableKey::setValue(const bf_rt_id_t &field_id,
                                             const uint64_t &value) {
  return setKeyIdxValue<BfRtPktgenPortTableKey>(this, *table_, field_id, value);
}

bf_status_t BfRtPktgenPortTableKey::setValue(const bf_rt_id_t &field_id,
                                             const uint8_t *value,
                                             const size_t &size) {
  return setKeyIdxValue<BfRtPktgenPortTableKey>(
      this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenPortTableKey::getValue(const bf_rt_id_t &field_id,
                                             uint64_t *value) const {
  return getKeyIdxValue<BfRtPktgenPortTableKey>(
      *this, *table_, field_id, value);
}

bf_status_t BfRtPktgenPortTableKey::getValue(const bf_rt_id_t &field_id,
                                             const size_t &size,
                                             uint8_t *value) const {
  return getKeyIdxValue<BfRtPktgenPortTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenAppTableKey::setValue(const bf_rt_id_t &field_id,
                                            const uint64_t &value) {
  return setKeyIdxValue<BfRtPktgenAppTableKey>(this, *table_, field_id, value);
}

bf_status_t BfRtPktgenAppTableKey::setValue(const bf_rt_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setKeyIdxValue<BfRtPktgenAppTableKey>(
      this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenAppTableKey::getValue(const bf_rt_id_t &field_id,
                                            uint64_t *value) const {
  return getKeyIdxValue<BfRtPktgenAppTableKey>(*this, *table_, field_id, value);
}

bf_status_t BfRtPktgenAppTableKey::getValue(const bf_rt_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getKeyIdxValue<BfRtPktgenAppTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenPortMaskTableKey::setValue(const bf_rt_id_t &field_id,
                                                 const uint64_t &value) {
  return setKeyIdxValue<BfRtPktgenPortMaskTableKey>(
      this, *table_, field_id, value);
}

bf_status_t BfRtPktgenPortMaskTableKey::setValue(const bf_rt_id_t &field_id,
                                                 const uint8_t *value,
                                                 const size_t &size) {
  return setKeyIdxValue<BfRtPktgenPortMaskTableKey>(
      this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenPortMaskTableKey::getValue(const bf_rt_id_t &field_id,
                                                 uint64_t *value) const {
  return getKeyIdxValue<BfRtPktgenPortMaskTableKey>(
      *this, *table_, field_id, value);
}

bf_status_t BfRtPktgenPortMaskTableKey::getValue(const bf_rt_id_t &field_id,
                                                 const size_t &size,
                                                 uint8_t *value) const {
  return getKeyIdxValue<BfRtPktgenPortMaskTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPktgenPktBufferTableKey::setValue(const bf_rt_id_t &field_id,
                                                  const uint64_t &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Get key field for field_id %d fail",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (value > static_cast<uint32_t>(-1)) {
    LOG_ERROR(
        "%s:%d %s ERROR Value size is larger than the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  if (key_field->getName() == "pkt_buffer_offset") {
    byte_buf_offset_ = static_cast<uint32_t>(value);
  } else if (key_field->getName() == "pkt_buffer_size") {
    byte_buf_size_ = static_cast<uint32_t>(value);
  } else {
    LOG_ERROR("%s:%d %s ERROR Invalid field, field id %d, name %s",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id,
              (key_field->getName().c_str()));
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPktBufferTableKey::setValue(const bf_rt_id_t &field_id,
                                                  const uint8_t *value,
                                                  const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Get key field for field_id %d fail",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  if (key_field->getName() == "pkt_buffer_offset") {
    byte_buf_offset_ = val;
  } else if (key_field->getName() == "pkt_buffer_size") {
    byte_buf_size_ = val;
  } else {
    LOG_ERROR("%s:%d %s ERROR Invalid field, field id %d, name %s",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id,
              (key_field->getName().c_str()));
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPktBufferTableKey::getValue(const bf_rt_id_t &field_id,
                                                  uint64_t *value) const {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Get key field for field_id %d fail",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  if (key_field->getName() == "pkt_buffer_offset") {
    *value = static_cast<uint64_t>(byte_buf_offset_);
  } else if (key_field->getName() == "pkt_buffer_size") {
    *value = static_cast<uint64_t>(byte_buf_size_);
  } else {
    LOG_ERROR("%s:%d %s ERROR Invalid field, field id %d, name %s",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id,
              (key_field->getName().c_str()));
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPktBufferTableKey::getValue(const bf_rt_id_t &field_id,
                                                  const size_t &size,
                                                  uint8_t *value) const {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Get key field for field_id %d fail",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t local_val = 0;
  if (key_field->getName() == "pkt_buffer_offset") {
    local_val = byte_buf_offset_;
  } else if (key_field->getName() == "pkt_buffer_size") {
    local_val = byte_buf_size_;
  } else {
    LOG_ERROR("%s:%d %s ERROR Invalid field, field id %d, name %s",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              field_id,
              (key_field->getName().c_str()));
    return BF_INVALID_ARG;
  }
  local_val = htobe32(local_val);
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return BF_SUCCESS;
}

}  // namespace bfrt
