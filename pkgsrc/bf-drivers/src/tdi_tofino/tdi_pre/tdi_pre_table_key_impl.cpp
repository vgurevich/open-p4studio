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


#ifdef __cplusplus
extern "C" {
#endif
#include <arpa/inet.h>
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <tdi/tdi_table_key.hpp>

#include "tdi_pre_table_impl.hpp"
#include "tdi_pre_table_key_impl.hpp"

namespace tdi {

namespace {

template <class T>
tdi_status_t getKeyIdxValue(const T &key,
                            const TdiTableObj &table,
                            const tdi_id_t &field_id,
                            uint64_t *value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, &table);
  if (status != TDI_SUCCESS) {
    return status;
  }

  *value = key.getId();

  return TDI_SUCCESS;
}

inline tdi_status_t keyValueBoundsCheck(const uint64_t &value,
                                        const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  // Check if the value is within the limit based on the key field size
  auto limit = (1ULL << size) - 1;
  if (value > limit) {
    return TDI_INVALID_ARG;
  }

  return status;
}
}  // anonymous namespace

tdi_status_t TdiPREMGIDTableKey::setValue(const tdi_id_t &field_id,
                                          const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  mgid_ = value;
  return TDI_SUCCESS;
}

tdi_status_t TdiPREMGIDTableKey::setValue(const tdi_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  uint16_t val = *(reinterpret_cast<const uint16_t *>(value));
  mgid_ = be16toh(val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMGIDTableKey::getValue(const tdi_id_t &field_id,
                                          uint64_t *value) const {
  return getKeyIdxValue<TdiPREMGIDTableKey>(*this, *table_, field_id, value);
}

tdi_status_t TdiPREMGIDTableKey::getValue(const tdi_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  uint16_t local_val = htobe16(this->getId());
  std::memcpy(value, &local_val, sizeof(uint16_t));

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMGIDTableKey::reset() {
  mgid_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastNodeTableKey::setValue(const tdi_id_t &field_id,
                                                   const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  multicast_node_id_ = value;

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastNodeTableKey::setValue(const tdi_id_t &field_id,
                                                   const uint8_t *value,
                                                   const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  multicast_node_id_ = be32toh(val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastNodeTableKey::getValue(const tdi_id_t &field_id,
                                                   uint64_t *value) const {
  return getKeyIdxValue<TdiPREMulticastNodeTableKey>(
      *this, *table_, field_id, value);
}

tdi_status_t TdiPREMulticastNodeTableKey::getValue(const tdi_id_t &field_id,
                                                   const size_t &size,
                                                   uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t local_val = htobe32(this->getId());
  std::memcpy(value, &local_val, sizeof(uint32_t));

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastNodeTableKey::reset() {
  multicast_node_id_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiPREECMPTableKey::setValue(const tdi_id_t &field_id,
                                          const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  multicast_ecmp_id_ = value;

  return TDI_SUCCESS;
}

tdi_status_t TdiPREECMPTableKey::setValue(const tdi_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  multicast_ecmp_id_ = be32toh(val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREECMPTableKey::getValue(const tdi_id_t &field_id,
                                          uint64_t *value) const {
  return getKeyIdxValue<TdiPREECMPTableKey>(*this, *table_, field_id, value);
}

tdi_status_t TdiPREECMPTableKey::getValue(const tdi_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t local_val = htobe32(this->getId());
  std::memcpy(value, &local_val, sizeof(uint32_t));

  return TDI_SUCCESS;
}

tdi_status_t TdiPREECMPTableKey::reset() {
  multicast_ecmp_id_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiPRELAGTableKey::setValue(const tdi_id_t &field_id,
                                         const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  multicast_lag_id_ = value;

  return TDI_SUCCESS;
}

tdi_status_t TdiPRELAGTableKey::setValue(const tdi_id_t &field_id,
                                         const uint8_t *value,
                                         const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint8_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint8_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  uint8_t val = *(reinterpret_cast<const uint8_t *>(value));
  multicast_lag_id_ = val;

  return TDI_SUCCESS;
}

tdi_status_t TdiPRELAGTableKey::getValue(const tdi_id_t &field_id,
                                         uint64_t *value) const {
  return getKeyIdxValue<TdiPRELAGTableKey>(*this, *table_, field_id, value);
}

tdi_status_t TdiPRELAGTableKey::getValue(const tdi_id_t &field_id,
                                         const size_t &size,
                                         uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint8_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint8_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  *value = this->getId();

  return TDI_SUCCESS;
}

tdi_status_t TdiPRELAGTableKey::reset() {
  multicast_lag_id_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPruneTableKey::setValue(const tdi_id_t &field_id,
                                                    const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  multicast_l2_xid_ = value;

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPruneTableKey::setValue(const tdi_id_t &field_id,
                                                    const uint8_t *value,
                                                    const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  uint16_t val = *(reinterpret_cast<const uint16_t *>(value));
  multicast_l2_xid_ = be16toh(val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPruneTableKey::getValue(const tdi_id_t &field_id,
                                                    uint64_t *value) const {
  return getKeyIdxValue<TdiPREMulticastPruneTableKey>(
      *this, *table_, field_id, value);
}

tdi_status_t TdiPREMulticastPruneTableKey::getValue(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return TDI_INVALID_ARG;
  }

  uint16_t local_val = htobe16(this->getId());
  std::memcpy(value, &local_val, sizeof(uint16_t));

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPruneTableKey::reset() {
  multicast_l2_xid_ = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPortTableKey::setValue(const tdi_id_t &field_id,
                                                   const uint64_t &value) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  // Check if the key field value is valid based on size of the key field
  // We don't have common utils function for key bounds check, so
  // use anonymous namesapce function for PRE.
  size_t size = key_field->getSize();
  status = keyValueBoundsCheck(value, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu for "
              "field id %d allows",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              value,
              size,
              field_id);
    return status;
  }

  dev_port_ = value;

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPortTableKey::setValue(const tdi_id_t &field_id,
                                                   const uint8_t *value,
                                                   const size_t &size) {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  dev_port_ = be32toh(val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPortTableKey::getValue(const tdi_id_t &field_id,
                                                   uint64_t *value) const {
  return getKeyIdxValue<TdiPREMulticastPortTableKey>(
      *this, *table_, field_id, value);
}

tdi_status_t TdiPREMulticastPortTableKey::getValue(const tdi_id_t &field_id,
                                                   const size_t &size,
                                                   uint8_t *value) const {
  // Get the key_field from the table
  const TdiTableKeyField *key_field;
  auto status = utils::TdiTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != TDI_SUCCESS) {
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
    return TDI_INVALID_ARG;
  }

  uint32_t local_val = htobe32(this->getId());
  std::memcpy(value, &local_val, sizeof local_val);

  return TDI_SUCCESS;
}

tdi_status_t TdiPREMulticastPortTableKey::reset() {
  dev_port_ = 0;
  return TDI_SUCCESS;
}

}  // namespace tdi
