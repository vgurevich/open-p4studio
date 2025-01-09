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


#include "bf_rt_tm_table_key_impl.hpp"

namespace bfrt {

namespace {

inline bf_status_t keyValueBoundsCheck(const uint64_t &value,
                                       const size_t &size) {
  bf_status_t status = BF_SUCCESS;

  // Check if the value is within the limit based on the key field size
  auto limit = (1ULL << size * 8) - 1;
  if (value > limit) {
    return BF_INVALID_ARG;
  }

  return status;
}
}  // anonymous namespace

//-------------------------------------------------

namespace {
inline bf_status_t checkKeyField(const bf_rt_id_t &field_id,
                                 const uint64_t &value,
                                 const BfRtTableObj *table) {
  if (table == nullptr) {
    return BF_INVALID_ARG;
  }
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          table->table_name_get().c_str(),
          field_id);
  // Get the key_field object from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table);
  if (status != BF_SUCCESS) {
    return status;
  }
  // Check if the key field value is valid based on size of the key field
  size_t size = (key_field->getSize() + 7) / 8;
  status = keyValueBoundsCheck(value, size);
  if (status != BF_SUCCESS)
    LOG_ERROR("%s:%d %s : Value %" PRIu64
              " is greater than what field size %zu "
              "of field id %d allows",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              value,
              size,
              field_id);
  return status;
}

inline bf_status_t checkKeyField(const bf_rt_id_t &field_id,
                                 const BfRtTableObj *table,
                                 const size_t &s) {
  if (table == nullptr) {
    return BF_INVALID_ARG;
  }
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          table->table_name_get().c_str(),
          field_id);
  // Get the key_field object from the table
  const BfRtTableKeyField *key_field;
  auto status = utils::BfRtTableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, KeyFieldType::EXACT, table);
  if (status != BF_SUCCESS) {
    return status;
  }
  // Check if the key field value is valid based on size of the key field
  size_t size = (key_field->getSize() + 7) / 8;
  if (size != s) {
    LOG_ERROR(
        "%s:%d %s Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table->table_name_get().c_str(),
        size,
        s,
        field_id);
    status = BF_INVALID_ARG;
  }
  return status;
}
}  // namespace

//-------------- TM PPG Table Key --------------------------------------

bf_status_t BfRtTMPpgTableKey::setValue(const bf_rt_id_t &field_id,
                                        const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_id_field && sizeof(uint64_t) >= sizeof(bf_tm_ppg_id_t)) {
    this->ppg_id = static_cast<bf_tm_ppg_id_t>(value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableKey::setValue(const bf_rt_id_t &field_id,
                                        const uint8_t *value,
                                        const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == ppg_id_field && sizeof(bf_tm_ppg_id_t) == sizeof(uint8_t)) {
    this->ppg_id = static_cast<bf_tm_ppg_id_t>(*value);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableKey::getValue(const bf_rt_id_t &field_id,
                                        uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_id_field && sizeof(uint64_t) >= sizeof(bf_tm_ppg_id_t)) {
    *value = static_cast<uint64_t>(this->ppg_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableKey::getValue(const bf_rt_id_t &field_id,
                                        const size_t &s,
                                        uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_id_field && sizeof(bf_tm_ppg_id_t) == sizeof(uint8_t)) {
    *value = static_cast<uint8_t>(this->ppg_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableKey::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  this->ppg_id = 0;
  return BF_SUCCESS;
}

//-------------- TM PortGroup Table Key --------------------------------------

bf_status_t BfRtTMPortGroupTableKey::setValue(const bf_rt_id_t &field_id,
                                              const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field && sizeof(uint64_t) >= sizeof(bf_tm_pg_t)) {
    this->pg_id = static_cast<bf_tm_pg_t>(value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupTableKey::setValue(const bf_rt_id_t &field_id,
                                              const uint8_t *value,
                                              const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == pg_id_field && sizeof(bf_tm_pg_t) == sizeof(uint8_t)) {
    this->pg_id = static_cast<bf_tm_pg_t>(*value);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupTableKey::getValue(const bf_rt_id_t &field_id,
                                              uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field && sizeof(uint64_t) >= sizeof(bf_tm_pg_t)) {
    *value = static_cast<uint64_t>(this->pg_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupTableKey::getValue(const bf_rt_id_t &field_id,
                                              const size_t &s,
                                              uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field && sizeof(bf_tm_pg_t) == sizeof(uint8_t)) {
    *value = static_cast<uint8_t>(this->pg_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupTableKey::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  this->pg_id = 0;
  return BF_SUCCESS;
}

//-------------- TM Queue Table Key ------------------------------------------

bf_status_t BfRtTMQueueTableKey::setValue(const bf_rt_id_t &field_id,
                                          const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field) {
    this->pg_id = static_cast<bf_tm_pg_t>(value);
  } else if (field_id == pg_queue_field) {
    this->pg_queue = static_cast<uint8_t>(value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableKey::setValue(const bf_rt_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == pg_id_field) {
    this->pg_id = static_cast<bf_tm_pg_t>(*value);
  } else if (field_id == pg_queue_field) {
    this->pg_queue = static_cast<uint8_t>(*value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableKey::getValue(const bf_rt_id_t &field_id,
                                          uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field) {
    *value = static_cast<uint64_t>(this->pg_id);
  } else if (field_id == pg_queue_field) {
    *value = static_cast<uint64_t>(this->pg_queue);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableKey::getValue(const bf_rt_id_t &field_id,
                                          const size_t &s,
                                          uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == pg_id_field) {
    *value = static_cast<uint8_t>(this->pg_id);
  } else if (field_id == pg_queue_field) {
    *value = static_cast<uint8_t>(this->pg_queue);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableKey::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  this->pg_id = 0;
  this->pg_queue = 0;
  return BF_SUCCESS;
}

//-------------- TM L1 Node Table Key ------------------------------------------

bf_status_t BfRtTML1NodeTableKey::setValue(const bf_rt_id_t &field_id,
                                           const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == this->pg_id_field) {
    this->pg_id = static_cast<bf_tm_pg_t>(value);
  } else if (field_id == this->pg_l1_node_field) {
    this->pg_l1_node = static_cast<bf_tm_l1_node_t>(value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableKey::setValue(const bf_rt_id_t &field_id,
                                           const uint8_t *value,
                                           const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == this->pg_id_field) {
    this->pg_id = static_cast<bf_tm_pg_t>(*value);
  } else if (field_id == this->pg_l1_node_field) {
    this->pg_l1_node = static_cast<bf_tm_l1_node_t>(*value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableKey::getValue(const bf_rt_id_t &field_id,
                                           uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == this->pg_id_field) {
    *value = static_cast<uint64_t>(this->pg_id);
  } else if (field_id == this->pg_l1_node_field) {
    *value = static_cast<uint64_t>(this->pg_l1_node);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableKey::getValue(const bf_rt_id_t &field_id,
                                           const size_t &s,
                                           uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == this->pg_id_field) {
    *value = static_cast<uint8_t>(this->pg_id);
  } else if (field_id == this->pg_l1_node_field) {
    *value = static_cast<uint8_t>(this->pg_l1_node);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableKey::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  this->pg_id = 0;
  this->pg_l1_node = 0;
  return BF_SUCCESS;
}

/** Pool Config Table **/

bf_status_t BfRtTMPoolTableKey::setValue(const bf_rt_id_t &field_id,
                                         const std::string &value) {
  bf_rt_id_t pool_field_id;
  auto status = this->table_->keyFieldIdGet("pool", &pool_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == pool_field_id) {
    this->pool_ = value;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolTableKey::getValue(const bf_rt_id_t &field_id,
                                         std::string *value) const {
  bf_rt_id_t pool_field_id;
  auto status = this->table_->keyFieldIdGet("pool", &pool_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == pool_field_id) {
    *value = this->pool_;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolTableKey::reset() {
  this->pool_ = "";
  return BF_SUCCESS;
}

/** App pool Config Table **/
bf_status_t BfRtTMPoolAppTableKey::setValue(const bf_rt_id_t &field_id,
                                            const std::string &value) {
  bf_rt_id_t pool_field_id;
  auto status = this->table_->keyFieldIdGet("pool", &pool_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == pool_field_id) {
    this->pool_ = value;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolAppTableKey::getValue(const bf_rt_id_t &field_id,
                                            std::string *value) const {
  bf_rt_id_t pool_field_id;
  auto status = this->table_->keyFieldIdGet("pool", &pool_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == pool_field_id) {
    *value = this->pool_;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolAppTableKey::reset() {
  this->pool_ = "";
  return BF_SUCCESS;
}

/** Pool Color Config Table **/
bf_status_t BfRtTMPoolColorTableKey::setValue(const bf_rt_id_t &field_id,
                                              const std::string &value) {
  bf_rt_id_t color_field_id;
  auto status = this->table_->keyFieldIdGet("color", &color_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == color_field_id) {
    // Validate the color value first
    std::vector<std::reference_wrapper<const std::string>> color_choices;
    status = this->table_->keyFieldAllowedChoicesGet(field_id, &color_choices);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get string choices for key field Id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return status;
    }
    std::vector<std::string> valid_colors(color_choices.begin(),
                                          color_choices.end());
    if (find(valid_colors.begin(), valid_colors.end(), value) ==
        valid_colors.end()) {
      LOG_ERROR("%s:%d %s Invalid value for key field Id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
    this->color_ = value;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolColorTableKey::getValue(const bf_rt_id_t &field_id,
                                              std::string *value) const {
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_rt_id_t color_field_id;
  auto status = this->table_->keyFieldIdGet("color", &color_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return status;
  }

  if (field_id == color_field_id) {
    *value = this->color_;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolColorTableKey::reset() {
  this->color_ = "";
  return BF_SUCCESS;
}

/** App Pool PFC Config Table **/
bf_status_t BfRtTMAppPoolPfcTableKey::setValue(const bf_rt_id_t &field_id,
                                               const std::string &value) {
  bf_status_t status = BF_SUCCESS;
  if (field_id == this->pool_field_id_) {
    // Validate the pool value first
    std::vector<std::reference_wrapper<const std::string>> pool_choices;
    status = this->table_->keyFieldAllowedChoicesGet(field_id, &pool_choices);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get string choices for key field Id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return status;
    }
    std::vector<std::string> valid_pools(pool_choices.begin(),
                                         pool_choices.end());
    if (find(valid_pools.begin(), valid_pools.end(), value) ==
        valid_pools.end()) {
      LOG_ERROR("%s:%d %s Invalid value for key field Id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
    this->pool_ = value;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::getValue(const bf_rt_id_t &field_id,
                                               std::string *value) const {
  if (value == nullptr) {
    return BF_INVALID_ARG;
  }

  if (field_id == pool_field_id_) {
    *value = this->pool_;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::setValue(const bf_rt_id_t &field_id,
                                               const uint8_t *value,
                                               const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == this->cos_field_id_ && value != nullptr &&
      s == sizeof(uint8_t)) {
    this->cos_ = static_cast<uint8_t>(*value);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::getValue(const bf_rt_id_t &field_id,
                                               const size_t &s,
                                               uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == cos_field_id_ && value != nullptr) {
    *value = this->cos_;
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::setValue(const bf_rt_id_t &field_id,
                                               const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == this->cos_field_id_) {
    this->cos_ = static_cast<uint8_t>(value);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::getValue(const bf_rt_id_t &field_id,
                                               uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == cos_field_id_ && value != nullptr) {
    *value = static_cast<uint64_t>(this->cos_);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMAppPoolPfcTableKey::reset() {
  this->pool_ = "";
  this->cos_ = std::numeric_limits<uint8_t>::max();
  return BF_SUCCESS;
}

/** Dev port key **/
bf_status_t BfRtTMDevPortKey::setValue(const bf_rt_id_t &field_id,
                                       const uint8_t *value,
                                       const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == this->dev_port_field_id_ && value != nullptr &&
      s == sizeof(uint32_t)) {
    uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
    this->dev_port_ = be32toh(val);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMDevPortKey::getValue(const bf_rt_id_t &field_id,
                                       const size_t &s,
                                       uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == dev_port_field_id_ && value != nullptr) {
    uint32_t local_val = htobe32(this->dev_port_);
    std::memcpy(value, &local_val, sizeof(uint32_t));
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMDevPortKey::setValue(const bf_rt_id_t &field_id,
                                       const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == this->dev_port_field_id_) {
    this->dev_port_ = static_cast<uint32_t>(value);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMDevPortKey::getValue(const bf_rt_id_t &field_id,
                                       uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == dev_port_field_id_ && value != nullptr) {
    *value = static_cast<uint64_t>(this->dev_port_);
  } else {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMDevPortKey::reset() {
  this->dev_port_ = 0;
  return BF_SUCCESS;
}

//-------------- TM PPG Counter Table Key --------------------------------------

bf_status_t BfRtTMPpgCounterTableKey::setValue(const bf_rt_id_t &field_id,
                                               const uint64_t &value) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_counter_id_field &&
      sizeof(uint64_t) >= sizeof(bf_tm_ppg_id_t)) {
    this->ppg_counter_id = static_cast<bf_tm_ppg_id_t>(value);
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgCounterTableKey::setValue(const bf_rt_id_t &field_id,
                                               const uint8_t *value,
                                               const size_t &s) {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Set the data
  if (field_id == ppg_counter_id_field &&
      sizeof(bf_tm_ppg_id_t) == sizeof(uint8_t)) {
    this->ppg_counter_id =
        static_cast<bf_tm_ppg_id_t>(*value);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgCounterTableKey::getValue(const bf_rt_id_t &field_id,
                                               uint64_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, *value, this->table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_counter_id_field &&
      sizeof(uint64_t) >= sizeof(bf_tm_ppg_id_t)) {
    *value = static_cast<uint64_t>(this->ppg_counter_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgCounterTableKey::getValue(const bf_rt_id_t &field_id,
                                               const size_t &s,
                                               uint8_t *value) const {
  LOG_DBG("%s:%d %s key field_id=%d",
          __func__,
          __LINE__,
          this->table_->table_name_get().c_str(),
          field_id);
  auto status = checkKeyField(field_id, this->table_, s);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (field_id == ppg_counter_id_field &&
      sizeof(bf_tm_ppg_id_t) == sizeof(uint8_t)) {
    *value = static_cast<uint8_t>(this->ppg_counter_id);  // it is uint8_t
  } else {
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgCounterTableKey::reset() {
  LOG_DBG(
      "%s:%d %s", __func__, __LINE__, this->table_->table_name_get().c_str());
  this->ppg_counter_id = 0;
  return BF_SUCCESS;
}
}  // namespace bfrt
