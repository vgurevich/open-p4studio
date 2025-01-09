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

#include <tdi/common/tdi_utils.hpp>
#include <tdi_common/tdi_table_field_utils.hpp>
#include "tdi_mirror_table_key_impl.hpp"

namespace tdi {
namespace tna {
namespace tofino {

tdi_status_t MirrorCfgTableKey::reset() {
  session_id_ = 0;
  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTableKey::setValue(
    const tdi::KeyFieldInfo * /*key_field*/, const uint64_t &value) {
  this->setId(value);
  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTableKey::setValue(const tdi::KeyFieldInfo *key_field,
                                         const uint8_t *value,
                                         const size_t &size) {
  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint16_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }

  // There is no common utils function for key to convert to host order,
  // use be16toh directly
  uint16_t val = *(reinterpret_cast<const uint16_t *>(value));
  val = be16toh(val);
  this->setId(val);

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTableKey::setValue(
    const tdi_id_t &field_id, const tdi::KeyFieldValue &field_value) {
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, &field_value, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value.matchTypeGet() !=
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    return TDI_INVALID_ARG;
  }

  if (field_value.is_pointer()) {
    auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint8_t *> *>(
        &field_value);
    return this->setValue(key_field, e_fv->value_, e_fv->size_);
  } else {
    auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint64_t> *>(
        &field_value);
    return this->setValue(key_field, e_fv->value_);
  }

  return TDI_UNEXPECTED;
}

tdi_status_t MirrorCfgTableKey::getValue(
    const tdi::KeyFieldInfo * /*key_field*/, uint64_t *value) const {
  *value = this->getId();
  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTableKey::getValue(const tdi::KeyFieldInfo *key_field,
                                         const size_t &size,
                                         uint8_t *value) const {
  if (size != sizeof(uint16_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint16_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }

  // There is no common utils function for key to convert to network order,
  // use htobe16 directly
  uint16_t local_val = htobe16(this->getId());
  std::memcpy(value, &local_val, sizeof(uint16_t));

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTableKey::getValue(
    const tdi_id_t &field_id, tdi::KeyFieldValue *field_value) const {
  if (!field_value) {
    LOG_ERROR("%s:%d %s input param passed null for key field_id %d, ",
              __func__,
              __LINE__,
              table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Get the key_field from the table
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, field_value, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value->matchTypeGet() !=
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    return TDI_INVALID_ARG;
  }

  if (field_value->is_pointer()) {
    auto e_fv = static_cast<tdi::KeyFieldValueExact<uint8_t *> *>(field_value);
    return getValue(key_field, e_fv->size_, e_fv->value_);
  } else {
    auto e_fv = static_cast<tdi::KeyFieldValueExact<uint64_t> *>(field_value);
    return getValue(key_field, &e_fv->value_);
  }

  return TDI_UNEXPECTED;
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
