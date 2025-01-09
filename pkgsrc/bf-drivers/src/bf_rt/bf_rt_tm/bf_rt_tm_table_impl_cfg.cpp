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


#include "bf_rt_tm_table_impl_cfg.hpp"

namespace bfrt {

#define BFRT_TM_CFG_SET_DATA_FIELD_BYTES(a_field_name_, p_data_, a_val_) \
  {                                                                      \
    bf_rt_id_t a_field_id_;                                              \
    status = this->dataFieldIdGet((a_field_name_), &a_field_id_);        \
    if (status != BF_SUCCESS) {                                          \
      LOG_ERROR("%s:%d %s has no field %s",                              \
                __func__,                                                \
                __LINE__,                                                \
                this->table_name_get().c_str(),                          \
                (a_field_name_));                                        \
      return status;                                                     \
    }                                                                    \
    if (p_data_->checkFieldActive(a_field_id_)) {                        \
      status = p_data_->setValue(a_field_id_, &a_val_, sizeof(a_val_));  \
      if (status != BF_SUCCESS) {                                        \
        LOG_ERROR("%s:%d %s Can't set data object for %s",               \
                  __func__,                                              \
                  __LINE__,                                              \
                  this->table_name_get().c_str(),                        \
                  (a_field_name_));                                      \
        return status;                                                   \
      }                                                                  \
    }                                                                    \
  }

#define BFRT_TM_CFG_SET_DATA_FIELD_BOOL(a_field_name_, p_data_, a_val_) \
  {                                                                     \
    bf_rt_id_t a_field_id_;                                             \
    status = this->dataFieldIdGet((a_field_name_), &a_field_id_);       \
    if (status != BF_SUCCESS) {                                         \
      LOG_ERROR("%s:%d %s has no field %s",                             \
                __func__,                                               \
                __LINE__,                                               \
                this->table_name_get().c_str(),                         \
                (a_field_name_));                                       \
      return status;                                                    \
    }                                                                   \
    if (p_data_->checkFieldActive(a_field_id_)) {                       \
      status = p_data_->setValue(a_field_id_, a_val_);                  \
      if (status != BF_SUCCESS) {                                       \
        LOG_ERROR("%s:%d %s Can't set data object for %s",              \
                  __func__,                                             \
                  __LINE__,                                             \
                  this->table_name_get().c_str(),                       \
                  (a_field_name_));                                     \
        return status;                                                  \
      }                                                                 \
    }                                                                   \
  }

#define BFRT_TM_CFG_SET_DATA_FIELD_INT(a_field_name_, p_data_, a_val_)        \
  {                                                                           \
    bf_rt_id_t a_field_id_;                                                   \
    status = this->dataFieldIdGet((a_field_name_), &a_field_id_);             \
    if (status != BF_SUCCESS) {                                               \
      LOG_ERROR("%s:%d %s has no field %s",                                   \
                __func__,                                                     \
                __LINE__,                                                     \
                this->table_name_get().c_str(),                               \
                (a_field_name_));                                             \
      return status;                                                          \
    }                                                                         \
    if (p_data_->checkFieldActive(a_field_id_)) {                             \
      status = p_data_->setValue(a_field_id_, static_cast<uint64_t>(a_val_)); \
      if (status != BF_SUCCESS) {                                             \
        LOG_ERROR("%s:%d %s Can't set data object for %s",                    \
                  __func__,                                                   \
                  __LINE__,                                                   \
                  this->table_name_get().c_str(),                             \
                  (a_field_name_));                                           \
        return status;                                                        \
      }                                                                       \
    }                                                                         \
  }

#define BFRT_TM_CFG_DATA_GET_BYTES(a_field_name_, p_data_, a_field_val_)      \
  {                                                                           \
    bf_rt_id_t a_field_id_ = 0;                                               \
    status = this->dataFieldIdGet(a_field_name_, &a_field_id_);               \
    if (status != BF_SUCCESS) {                                               \
      LOG_ERROR("%s:%d %s Error in getting fieldId for %s",                   \
                __func__,                                                     \
                __LINE__,                                                     \
                table_name_get().c_str(),                                     \
                a_field_name_);                                               \
      return status;                                                          \
    }                                                                         \
    if (p_data_.checkFieldActive(a_field_id_)) {                              \
      status =                                                                \
          p_data_.getValue(a_field_id_, sizeof(a_field_val_), &a_field_val_); \
      if (status != BF_SUCCESS) {                                             \
        LOG_ERROR("%s:%d %s Error in getting data value for Id %d",           \
                  __func__,                                                   \
                  __LINE__,                                                   \
                  table_name_get().c_str(),                                   \
                  a_field_id_);                                               \
        return status;                                                        \
      }                                                                       \
    }                                                                         \
  }

#define BFRT_TM_CFG_DATA_GET_INT(a_field_name_, p_data_, a_field_val_)     \
  {                                                                        \
    bf_rt_id_t a_field_id_ = 0;                                            \
    status = this->dataFieldIdGet(a_field_name_, &a_field_id_);            \
    if (status != BF_SUCCESS) {                                            \
      LOG_ERROR("%s:%d %s Error in getting fieldId for %s",                \
                __func__,                                                  \
                __LINE__,                                                  \
                table_name_get().c_str(),                                  \
                a_field_name_);                                            \
      return status;                                                       \
    }                                                                      \
    if (p_data_.checkFieldActive(a_field_id_)) {                           \
      status = p_data_.getValue(a_field_id_, (uint64_t *)(&a_field_val_)); \
      if (status != BF_SUCCESS) {                                          \
        LOG_ERROR("%s:%d %s Error in getting data value for Id %d",        \
                  __func__,                                                \
                  __LINE__,                                                \
                  table_name_get().c_str(),                                \
                  a_field_id_);                                            \
        return status;                                                     \
      }                                                                    \
    }                                                                      \
  }

#define BFRT_TM_CFG_DATA_GET_BOOL(a_field_name_, p_data_, a_field_val_) \
  {                                                                     \
    bf_rt_id_t a_field_id_ = 0;                                         \
    status = this->dataFieldIdGet(a_field_name_, &a_field_id_);         \
    if (status != BF_SUCCESS) {                                         \
      LOG_ERROR("%s:%d %s Error in getting fieldId for %s",             \
                __func__,                                               \
                __LINE__,                                               \
                table_name_get().c_str(),                               \
                a_field_name_);                                         \
      return status;                                                    \
    }                                                                   \
    if (p_data_.checkFieldActive(a_field_id_)) {                        \
      status = p_data_.getValue(a_field_id_, &a_field_val_);            \
      if (status != BF_SUCCESS) {                                       \
        LOG_ERROR("%s:%d %s Error in getting data value for Id %d",     \
                  __func__,                                             \
                  __LINE__,                                             \
                  table_name_get().c_str(),                             \
                  a_field_id_);                                         \
        return status;                                                  \
      }                                                                 \
    }                                                                   \
  }

#define BFRT_TM_CFG_IS_FIELD_AVAILABLE(a_field_name_, p_data_, is_avail) \
  {                                                                      \
    bf_rt_id_t a_field_id_ = 0;                                          \
    is_avail = false;                                                    \
    status = this->dataFieldIdGet(a_field_name_, &a_field_id_);          \
    if (BF_SUCCESS == status) {                                          \
      is_avail = p_data_.hasValue(a_field_id_);                          \
    }                                                                    \
    status = BF_SUCCESS;                                                 \
  }

bf_status_t BfRtTMCfgTable::tableDefaultEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Fetch the fields and set it in data
  BfRtTMTableData *tmCfgData = static_cast<BfRtTMTableData *>(data);

  // Time stamp shift bits
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint8_t timestamp_shift_bits = 0;
  auto status =
      trafficMgr->bfTmTimestampShiftGet(dev_tgt.dev_id, &timestamp_shift_bits);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get timestamp_shift_bits",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_CFG_SET_DATA_FIELD_BYTES(
      "timestamp_shift_bits", tmCfgData, timestamp_shift_bits);

  // Cell size in bytes
  uint32_t cell_size_bytes = 0;
  status = trafficMgr->bfTmCellSizeInBytesGet(dev_tgt.dev_id, &cell_size_bytes);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get cell_size_bytes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_CFG_SET_DATA_FIELD_INT("cell_size_bytes", tmCfgData, cell_size_bytes);

  // Total number of cells
  uint32_t total_cells = 0;
  status = trafficMgr->bfTmTotalCellCountGet(dev_tgt.dev_id, &total_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get total_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_CFG_SET_DATA_FIELD_INT("total_cells", tmCfgData, total_cells);

  // Ingress buffer limit
  uint32_t ig_buffer_limit_cells = 0;
  status = trafficMgr->bfTmIngressBufferLimitGet(dev_tgt.dev_id,
                                                 &ig_buffer_limit_cells);
  if ((status != BF_SUCCESS) && (status != BF_NOT_SUPPORTED)) {
    LOG_ERROR("%s:%d %s Failed to get ig_buffer_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  if (status != BF_NOT_SUPPORTED) {
    BFRT_TM_CFG_SET_DATA_FIELD_INT(
        "ig_buffer_limit_cells", tmCfgData, ig_buffer_limit_cells);
  }

  // Ingress buffer limit enable
  bool ig_buffer_limit_enable = false;
  status = trafficMgr->bfTmIngressBufferLimitStateGet(dev_tgt.dev_id,
                                                      &ig_buffer_limit_enable);
  if ((status != BF_SUCCESS) && (status != BF_NOT_SUPPORTED)) {
    LOG_ERROR("%s:%d %s Failed to get ig_buffer_limit_enable",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  if (status != BF_NOT_SUPPORTED) {
    BFRT_TM_CFG_SET_DATA_FIELD_BOOL(
        "ig_buffer_limit_enable", tmCfgData, ig_buffer_limit_enable);
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMCfgTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Fetch the fields and set it in data
  const BfRtTMTableData &tmCfgData = static_cast<const BfRtTMTableData &>(data);
  bool is_field_avail = false;
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // Fetch the data fields and modify
  BFRT_TM_CFG_IS_FIELD_AVAILABLE(
      "timestamp_shift_bits", tmCfgData, is_field_avail);

  if (is_field_avail) {
    // Time stamp shift bits
    uint8_t timestamp_shift_bits;
    BFRT_TM_CFG_DATA_GET_BYTES(
        "timestamp_shift_bits", tmCfgData, timestamp_shift_bits);

    status =
        trafficMgr->bfTmTimestampShiftSet(dev_tgt.dev_id, timestamp_shift_bits);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set timestamp_shift_bits",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  }

  BFRT_TM_CFG_IS_FIELD_AVAILABLE(
      "ig_buffer_limit_cells", tmCfgData, is_field_avail);

  if (is_field_avail) {
    // Ingress buffer limit
    uint32_t ig_buffer_limit_cells = 0;
    BFRT_TM_CFG_DATA_GET_INT(
        "ig_buffer_limit_cells", tmCfgData, ig_buffer_limit_cells);

    status = trafficMgr->bfTmIngressBufferLimitSet(dev_tgt.dev_id,
                                                   ig_buffer_limit_cells);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set ig_buffer_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  }

  BFRT_TM_CFG_IS_FIELD_AVAILABLE(
      "ig_buffer_limit_enable", tmCfgData, is_field_avail);

  if (is_field_avail) {
    // Ingress buffer limit enable
    bool state = false;
    BFRT_TM_CFG_DATA_GET_BOOL("ig_buffer_limit_enable", tmCfgData, state);

    if (state) {
      status = trafficMgr->bfTmIngressBufferLimitEnable(dev_tgt.dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to enable ig_buffer_limit_enable",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
      }
    } else {
      status = trafficMgr->bfTmIngressBufferLimitDisable(dev_tgt.dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to disable ig_buffer_limit_enable",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
      }
    }
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMCfgTable::tableDefaultEntryReset(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;

  // Fetch the default values for the data fields and set it
  // Time stamp shift bits
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint8_t default_timestamp_shift_bits;
  status = trafficMgr->bfTmTimestampShiftGetDefault(
      dev_tgt.dev_id, &default_timestamp_shift_bits);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Failed to fetch the default value for timestamp_shift_bits",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }

  status = trafficMgr->bfTmTimestampShiftSet(dev_tgt.dev_id,
                                             default_timestamp_shift_bits);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to reset timestamp_shift_bits",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Ingress Buffer Limit
  uint32_t default_ig_buffer_limit_cells;
  status = trafficMgr->bfTmIngressBufferLimitGetDefault(
      dev_tgt.dev_id, &default_ig_buffer_limit_cells);
  if ((status != BF_SUCCESS) && (status != BF_NOT_SUPPORTED)) {
    LOG_ERROR(
        "%s:%d %s Failed to fetch the default value for "
        "ig_buffer_limit_cells",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  } else if (status != BF_NOT_SUPPORTED) {
    status = trafficMgr->bfTmIngressBufferLimitSet(
        dev_tgt.dev_id, default_ig_buffer_limit_cells);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to reset ig_buffer_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  }

  // Ingress Buffer Limit State
  bool default_ig_buffer_limit_state;
  status = trafficMgr->bfTmIngressBufferLimitStateGetDefault(
      dev_tgt.dev_id, &default_ig_buffer_limit_state);
  if ((status != BF_SUCCESS) && (status != BF_NOT_SUPPORTED)) {
    LOG_ERROR(
        "%s:%d %s Failed to fetch the default value for "
        "ig_buffer_limit_enable",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  } else if (status != BF_NOT_SUPPORTED) {
    if (default_ig_buffer_limit_state) {
      status = trafficMgr->bfTmIngressBufferLimitEnable(dev_tgt.dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to enable ingress buffer limit",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      }
    } else {
      // Default is Enable state for Ingress Buffer Limit
      status = trafficMgr->bfTmIngressBufferLimitDisable(dev_tgt.dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to disable ingress buffer limit",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      }
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTMCfgTable::tableClear(const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}
}  // namespace bfrt
