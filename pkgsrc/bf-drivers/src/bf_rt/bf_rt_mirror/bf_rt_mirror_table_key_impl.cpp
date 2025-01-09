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
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include "bf_rt_mirror_table_key_impl.hpp"

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

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return BF_INVALID_ARG;
  }

  // There is no common utils function for key to convert to network order,
  // use htobe16 directly
  uint16_t local_val = htobe16(key.getId());
  std::memcpy(value, &local_val, sizeof(uint16_t));

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

  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        size,
        sizeof(uint16_t),
        field_id);
    return BF_INVALID_ARG;
  }

  // There is no common utils function for key to convert to host order,
  // use be16toh directly
  uint16_t val = *(reinterpret_cast<const uint16_t *>(value));
  val = be16toh(val);
  key->setId(val);

  return BF_SUCCESS;
}

}  // anonymous namespace

bf_status_t BfRtMirrorCfgTableKey::reset() {
  session_id_ = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTableKey::setValue(const bf_rt_id_t &field_id,
                                            const uint64_t &value) {
  return setKeyIdxValue<BfRtMirrorCfgTableKey>(this, *table_, field_id, value);
}

bf_status_t BfRtMirrorCfgTableKey::setValue(const bf_rt_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setKeyIdxValue<BfRtMirrorCfgTableKey>(
      this, *table_, field_id, value, size);
}

bf_status_t BfRtMirrorCfgTableKey::getValue(const bf_rt_id_t &field_id,
                                            uint64_t *value) const {
  return getKeyIdxValue<BfRtMirrorCfgTableKey>(*this, *table_, field_id, value);
}

bf_status_t BfRtMirrorCfgTableKey::getValue(const bf_rt_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getKeyIdxValue<BfRtMirrorCfgTableKey>(
      *this, *table_, field_id, value, size);
}

}  // namespace bfrt
