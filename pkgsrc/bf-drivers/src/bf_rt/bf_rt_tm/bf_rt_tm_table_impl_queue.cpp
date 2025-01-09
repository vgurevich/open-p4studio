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


#include <sstream>

#include "bf_rt_port/bf_rt_port_mgr_intf.hpp"
#include "bf_rt_tm_table_helper_sched.hpp"
#include "bf_rt_tm_table_impl_queue.hpp"

namespace bfrt {

const std::map<std::string, bf_tm_app_pool_t>
    BfRtTMQueueBufferTable::str_to_pool{
        {"EG_APP_POOL_0", BF_TM_EG_APP_POOL_0},
        {"EG_APP_POOL_1", BF_TM_EG_APP_POOL_1},
        {"EG_APP_POOL_2", BF_TM_EG_APP_POOL_2},
        {"EG_APP_POOL_3", BF_TM_EG_APP_POOL_3},
    };

const std::map<bf_tm_app_pool_t, std::string>
    BfRtTMQueueBufferTable::pool_to_str{
        {BF_TM_EG_APP_POOL_0, "EG_APP_POOL_0"},
        {BF_TM_EG_APP_POOL_1, "EG_APP_POOL_1"},
        {BF_TM_EG_APP_POOL_2, "EG_APP_POOL_2"},
        {BF_TM_EG_APP_POOL_3, "EG_APP_POOL_3"},
    };

const std::map<std::string, bf_tm_queue_baf_t>
    BfRtTMQueueBufferTable::str_to_qbaf{{"1.5%", BF_TM_Q_BAF_1_POINT_5_PERCENT},
                                        {"3%", BF_TM_Q_BAF_3_PERCENT},
                                        {"6%", BF_TM_Q_BAF_6_PERCENT},
                                        {"11%", BF_TM_Q_BAF_11_PERCENT},
                                        {"20%", BF_TM_Q_BAF_20_PERCENT},
                                        {"33%", BF_TM_Q_BAF_33_PERCENT},
                                        {"50%", BF_TM_Q_BAF_50_PERCENT},
                                        {"66%", BF_TM_Q_BAF_66_PERCENT},
                                        {"80%", BF_TM_Q_BAF_80_PERCENT},
                                        {"DISABLE", BF_TM_Q_BAF_DISABLE}};

const std::map<bf_tm_queue_baf_t, std::string>
    BfRtTMQueueBufferTable::qbaf_to_str{{BF_TM_Q_BAF_1_POINT_5_PERCENT, "1.5%"},
                                        {BF_TM_Q_BAF_3_PERCENT, "3%"},
                                        {BF_TM_Q_BAF_6_PERCENT, "6%"},
                                        {BF_TM_Q_BAF_11_PERCENT, "11%"},
                                        {BF_TM_Q_BAF_20_PERCENT, "20%"},
                                        {BF_TM_Q_BAF_33_PERCENT, "33%"},
                                        {BF_TM_Q_BAF_50_PERCENT, "50%"},
                                        {BF_TM_Q_BAF_66_PERCENT, "66%"},
                                        {BF_TM_Q_BAF_80_PERCENT, "80%"},
                                        {BF_TM_Q_BAF_DISABLE, "DISABLE"}};

const std::map<std::string, bf_tm_queue_color_limit_t>
    BfRtTMQueueColorTable::str_to_color_limit{
        {"12.5%", BF_TM_Q_COLOR_LIMIT_12_POINT_5_PERCENT},
        {"25%", BF_TM_Q_COLOR_LIMIT_25_PERCENT},
        {"37.5%", BF_TM_Q_COLOR_LIMIT_37_POINT_5_PERCENT},
        {"50%", BF_TM_Q_COLOR_LIMIT_50_PERCENT},
        {"62.5%", BF_TM_Q_COLOR_LIMIT_62_POINT_5_PERCENT},
        {"75%", BF_TM_Q_COLOR_LIMIT_75_PERCENT},
        {"87.5%", BF_TM_Q_COLOR_LIMIT_87_POINT_5_PERCENT},
        {"100%", BF_TM_Q_COLOR_LIMIT_100_PERCENT}};

const std::map<bf_tm_queue_color_limit_t, std::string>
    BfRtTMQueueColorTable::color_limit_to_str{
        {BF_TM_Q_COLOR_LIMIT_12_POINT_5_PERCENT, "12.5%"},
        {BF_TM_Q_COLOR_LIMIT_25_PERCENT, "25%"},
        {BF_TM_Q_COLOR_LIMIT_37_POINT_5_PERCENT, "37.5%"},
        {BF_TM_Q_COLOR_LIMIT_50_PERCENT, "50%"},
        {BF_TM_Q_COLOR_LIMIT_62_POINT_5_PERCENT, "62.5%"},
        {BF_TM_Q_COLOR_LIMIT_75_PERCENT, "75%"},
        {BF_TM_Q_COLOR_LIMIT_87_POINT_5_PERCENT, "87.5%"},
        {BF_TM_Q_COLOR_LIMIT_100_PERCENT, "100%"}};

//----------------------------------------------------------------------------
#define BFRT_TM_SET_Q_DATA_FIELD(a_field_name_, p_data_, a_val_)               \
  {                                                                            \
    bf_rt_id_t a_field_id_;                                                    \
                                                                               \
    bf_status_t status_ = this->dataFieldIdGet((a_field_name_), &a_field_id_); \
    if (status_ != BF_SUCCESS) {                                               \
      LOG_ERROR("%s:%d %s has no field %s",                                    \
                __func__,                                                      \
                __LINE__,                                                      \
                this->table_name_get().c_str(),                                \
                (a_field_name_));                                              \
      return status_;                                                          \
    }                                                                          \
    if (p_data_->checkFieldActive(a_field_id_)) {                              \
      LOG_DBG(                                                                 \
          "%s:%d %s dev_id=%d pipe_id=%d "                                     \
          "pg_id=%d pg_queue=%d field_id=%d(%s)",                              \
          __func__,                                                            \
          __LINE__,                                                            \
          this->table_name_get().c_str(),                                      \
          dev_tgt.dev_id,                                                      \
          dev_tgt.pipe_id,                                                     \
          pg_id,                                                               \
          pg_queue,                                                            \
          a_field_id_,                                                         \
          (a_field_name_));                                                    \
      status_ = p_data_->setValue(a_field_id_, (a_val_));                      \
      if (status_) {                                                           \
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",                     \
                  __func__,                                                    \
                  __LINE__,                                                    \
                  this->table_name_get().c_str(),                              \
                  (a_field_name_));                                            \
        return status_;                                                        \
      }                                                                        \
      status_get = BF_SUCCESS;                                                 \
    }                                                                          \
  }

#define BFRT_TM_SET_Q_DFT_FIELD(a_field_name_, p_data_, a_val_)                \
  {                                                                            \
    bf_rt_id_t a_field_id_;                                                    \
                                                                               \
    bf_status_t status_ = this->dataFieldIdGet((a_field_name_), &a_field_id_); \
    if (status_ != BF_SUCCESS) {                                               \
      LOG_ERROR("%s:%d %s has no field %s",                                    \
                __func__,                                                      \
                __LINE__,                                                      \
                this->table_name_get().c_str(),                                \
                (a_field_name_));                                              \
      return status_;                                                          \
    }                                                                          \
    if (p_data_->checkFieldActive(a_field_id_)) {                              \
      LOG_DBG(                                                                 \
          "%s:%d %s dev_id=%d pipe_id=%d "                                     \
          "default entry field_id=%d(%s)",                                     \
          __func__,                                                            \
          __LINE__,                                                            \
          this->table_name_get().c_str(),                                      \
          dev_tgt.dev_id,                                                      \
          dev_tgt.pipe_id,                                                     \
          a_field_id_,                                                         \
          (a_field_name_));                                                    \
      status_ = p_data_->setValue(a_field_id_, (a_val_));                      \
      if (status_) {                                                           \
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",                     \
                  __func__,                                                    \
                  __LINE__,                                                    \
                  this->table_name_get().c_str(),                              \
                  (a_field_name_));                                            \
        return status_;                                                        \
      }                                                                        \
      status_get = BF_SUCCESS;                                                 \
    }                                                                          \
  }

#define BFRT_TM_SET_Q_ACT_FIELD(a_field_name_, a_act_id_, p_data_, a_val_) \
  {                                                                        \
    bf_rt_id_t a_field_id_;                                                \
                                                                           \
    bf_status_t status_ =                                                  \
        this->dataFieldIdGet((a_field_name_), (a_act_id_), &a_field_id_);  \
    if (status_ != BF_SUCCESS) {                                           \
      LOG_ERROR("%s:%d %s has no field %s",                                \
                __func__,                                                  \
                __LINE__,                                                  \
                this->table_name_get().c_str(),                            \
                (a_field_name_));                                          \
      return status_;                                                      \
    }                                                                      \
    if (p_data_->checkFieldActive(a_field_id_)) {                          \
      LOG_DBG(                                                             \
          "%s:%d %s dev_id=%d pipe_id=%d action_id=%d"                     \
          " default entry field_id=%d(%s)",                                \
          __func__,                                                        \
          __LINE__,                                                        \
          this->table_name_get().c_str(),                                  \
          dev_tgt.dev_id,                                                  \
          dev_tgt.pipe_id,                                                 \
          (a_act_id_),                                                     \
          a_field_id_,                                                     \
          (a_field_name_));                                                \
      status_ = p_data_->setValue(a_field_id_, (a_val_));                  \
      if (status_) {                                                       \
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",                 \
                  __func__,                                                \
                  __LINE__,                                                \
                  this->table_name_get().c_str(),                          \
                  (a_field_name_));                                        \
        return status_;                                                    \
      }                                                                    \
      status_get = BF_SUCCESS;                                             \
    }                                                                      \
  }

//---------------------------------------------------------------
bf_status_t BfRtTMQueueTableIntf::getDeviceTMQueuesCount(
    const bf_rt_target_t &dev_tgt, uint32_t *count) const {
  LOG_DBG("%s:%d", __func__, __LINE__);

  bf_status_t status = BF_SUCCESS;

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  *count = 0;

  uint8_t mau_pipes = 0;
  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, &mau_pipes, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  uint32_t count_ = (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) ? mau_pipes : 1;
  count_ *= queues_per_pg;
  count_ *= pg_per_pipe;
  *count = count_;

  return status;
}

bf_status_t BfRtTMQueueTableIntf::tableUsageGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  return getDeviceTMQueuesCount(dev_tgt, count);
}

bf_status_t BfRtTMQueueTableIntf::tableSizeGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    size_t *size) const {
  uint32_t size_ = 0;

  auto status = this->getDeviceTMQueuesCount(dev_tgt, &size_);
  *size = size_;

  return status;
}

bf_status_t BfRtTMQueueTableIntf::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMQueueTableKey(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableIntf::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtTMQueueTableKey *>(key))->reset();
}

// Resets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMQueueTableIntf::tableResetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableIntf::tableResetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMQueueTableIntf::tableResetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (q_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  // Make a copy of the data field ids to work with.
  // The virtual reset methods will set 'default' values
  // and remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(q_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      active_fields_cnt,
      q_data->hasValues());

  // Get block of related fields at once if this mode is defined for the table.
  // In the latter case the list of field ids should shrink.
  status = this->tableResetFields(
      dev_tgt, pg_id, pg_queue, port_id, queue_nr, q_data, workDataFields);
  if (status == BF_EAGAIN) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "reset block of fields failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "need fields %zu of %zu; got block of %zu values",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      workDataFields.size(),
      active_fields_cnt,
      q_data->hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
    bool f_read_only = false;
    status = this->dataFieldReadOnlyGet(data_id, &f_read_only);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get r/o attribute of field_id=%d, rc=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id,
                status);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    if (f_read_only) {
      LOG_DBG("%s:%d %s skip r/o field_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
      continue;
    }
    status = this->tableResetField(
        dev_tgt, data_id, pg_id, pg_queue, port_id, queue_nr, q_data);
    if (status) {
      break;
    }
  }  // for (field_id)

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "reset %zu values for %zu fields",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      q_data->hasValues(),
      active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
  }
  return status;
}

//----
bf_status_t BfRtTMQueueTableIntf::tableReset(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  bf_status_t status = BF_UNEXPECTED;

  if (dev_wrk.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_wrk.dev_id, &(dev_wrk.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id);
      return status;
    }
  }
  status = this->singlePipe_validate(dev_wrk);
  if (BF_SUCCESS != status) {
    return status;
  }

  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  BfRtTMTableData reset_data(this);

  std::lock_guard<std::mutex> lock(this->entry_lock);

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d clear with default entry",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);

    for (bf_tm_pg_t pg_id = 0; pg_id < pg_per_pipe && BF_SUCCESS == status;
         pg_id++) {
      // TODO: Acquire the Queue mapping state lock for the pg_id.
      for (uint8_t pg_queue = 0; pg_queue < queues_per_pg; pg_queue++) {
        status = this->tableResetEntry(dev_wrk, pg_id, pg_queue, &reset_data);
        if (status == BF_EAGAIN) {
          LOG_TRACE(
              "%s:%d %s Skip reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_queue=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_queue);
          status = BF_SUCCESS;
          continue;
        }
        if (status != BF_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s Can't prepare reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_queue=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_queue);
          break;
        }
        status = this->tableSetEntry(dev_wrk, reset_data, pg_id, pg_queue);
        if (status != BF_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s Can't reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_queue=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_queue);
          break;
        }
      }
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // Is called for a single pipe reset.
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more pipes on this device
    }
  } while (BF_SUCCESS == status);

  return status;
}

bf_status_t BfRtTMQueueTableIntf::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                                const BfRtTMTableData &q_data,
                                                bf_tm_pg_t pg_id,
                                                uint8_t pg_queue) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  if (!is_mapped) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d is not mapped to a port",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return BF_NOT_READY;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(q_data.getAssignedFields());
  auto active_fields_cnt = workDataFields.size();

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      active_fields_cnt,
      q_data.hasValues());

  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.
  status = this->tableSetFields(
      dev_tgt, pg_id, pg_queue, port_id, queue_nr, q_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "set block of fields failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "need fields %zu of %zu; got block of %zu values",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      workDataFields.size(),
      active_fields_cnt,
      q_data.hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
    bool f_read_only = false;
    status = this->dataFieldReadOnlyGet(data_id, &f_read_only);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get r/o attribute of field_id=%d, rc=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id,
                status);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    if (f_read_only) {
      LOG_DBG("%s:%d %s skip r/o field_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
      //-----------
      // Should we raise error on read-only fields in the mod() request ?
      continue;
    }
    status = this->tableSetField(
        dev_tgt, pg_id, pg_queue, port_id, queue_nr, q_data, data_id);
    if (status) {
      break;
    }
  }  // for (field_id)

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "set %zu values for %zu fields",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      q_data.hasValues(),
      active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
  }
  return status;
}

// Sets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMQueueTableIntf::tableSetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    const BfRtTMTableData & /* q_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMQueueTableIntf::tableSetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    const BfRtTMTableData & /* q_data */,
    bf_rt_id_t data_id) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMQueueTableIntf::tableGetEntry(const bf_rt_target_t &dev_tgt,
                                                bf_tm_pg_t pg_id,
                                                uint8_t pg_queue,
                                                BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (q_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(q_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      active_fields_cnt,
      q_data->hasValues());

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  status = this->tableGetFields(
      dev_tgt, pg_id, pg_queue, port_id, queue_nr, q_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "get block of fields failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "need fields %zu of %zu; got block of %zu values",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      workDataFields.size(),
      active_fields_cnt,
      q_data->hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
    status = this->tableGetField(
        dev_tgt, data_id, pg_id, pg_queue, port_id, queue_nr, q_data);
    if (status != BF_SUCCESS) {
      break;
    }
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "got %zu values for %zu fields",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      q_data->hasValues(),
      active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        status,
        bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTMQueueTableIntf::tableGetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    BfRtTMTableData * /* q_data */) const {
  LOG_ERROR("%s:%d %s not implemented field_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMQueueTableIntf::tableGetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t /* port_id */,
    bf_tm_queue_t /* queue_nr */,
    BfRtTMTableData * /* q_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

//----
bf_status_t BfRtTMQueueTableIntf::tableClear(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  bf_status_t status = BF_UNEXPECTED;

  if (dev_wrk.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_wrk.dev_id, &(dev_wrk.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id);
      return status;
    }
  }
  status = this->singlePipe_validate(dev_wrk);
  if (BF_SUCCESS != status) {
    return status;
  }

  BfRtTMTableData dft_data(this);

  status = tableDefaultEntryGet(session, dev_tgt, flags, &dft_data);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get default entry to use it for table clear",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  std::lock_guard<std::mutex> lock(this->entry_lock);

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d clear with default entry",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);

    for (bf_tm_pg_t pg_id = 0; pg_id < pg_per_pipe && BF_SUCCESS == status;
         pg_id++) {
      // TODO: Acquire the Queue mapping state lock for the pg_id.
      for (uint8_t pg_queue = 0; pg_queue < queues_per_pg; pg_queue++) {
        status = this->tableSetEntry(dev_wrk, dft_data, pg_id, pg_queue);
        if (status != BF_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s Can't clear entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_queue=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_queue);
          break;
        }
      }
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // Is called for a single pipe reset.
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more pipes on this device
    }
  } while (BF_SUCCESS == status);

  return status;
}

bf_status_t BfRtTMQueueTableIntf::tableEntryGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  BfRtTMTableData *q_data = static_cast<BfRtTMTableData *>(data);

  // Table with actions should proceed with get using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && q_data->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s No active data fields to get",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    return BF_SUCCESS;
  }

  const BfRtTMQueueTableKey &qKey =
      static_cast<const BfRtTMQueueTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;

  status = qKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TODO: Acquire the Queue mapping state lock for the pg_id.

  if (this->isVolatile()) {
    status = this->tableGetEntry(dev_tgt, pg_id, pg_queue, q_data);
  } else {
    std::lock_guard<std::mutex> lock(this->entry_lock);
    status = this->tableGetEntry(dev_tgt, pg_id, pg_queue, q_data);
  }

  return status;
}

bf_status_t BfRtTMQueueTableIntf::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  const BfRtTMTableData &q_data = static_cast<const BfRtTMTableData &>(data);

  // Table with actions should proceed with mod using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && q_data.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const BfRtTMQueueTableKey &qKey =
      static_cast<const BfRtTMQueueTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;

  status = qKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TODO: Acquire the Queue mapping state lock for the pg_id.

  std::lock_guard<std::mutex> lock(this->entry_lock);

  return this->tableSetEntry(dev_tgt, q_data, pg_id, pg_queue);
}

bf_status_t BfRtTMQueueTableIntf::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }
  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  BfRtTMQueueTableKey *qKey = static_cast<BfRtTMQueueTableKey *>(key);
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;

  qKey->setId(pg_id, pg_queue);

  return this->tableEntryGet(session, dev_tgt, flags, *qKey, data);
}

//---
bf_status_t BfRtTMQueueTableIntf::tableEntryGetNext_n(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    const uint32_t &cnt,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  LOG_DBG("%s:%d %s for cnt=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          cnt);

  if (key_data_pairs == nullptr || num_returned == nullptr) {
    return BF_INVALID_ARG;
  }

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  const BfRtTMQueueTableKey &qKey =
      static_cast<const BfRtTMQueueTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;

  status = qKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  if (pg_id >= pg_per_pipe || pg_queue >= queues_per_pg) {
    LOG_ERROR(
        "%s:%d %s Invalid key "
        " dev_id=%d pipe_id=%d pg_id=%d, pg_queue=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue);
    return BF_INVALID_ARG;
  }

  LOG_DBG("%s:%d %s cnt=%d dev_id=%d pipe_id=%d pg_id=%d, pg_queue=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          cnt,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue);

  // Continue from the next entry on the same pipe.
  if (++pg_queue >= queues_per_pg) {
    pg_queue = 0;
    pg_id++;
  }

  uint32_t i = 0;

  for (; pg_id < pg_per_pipe && i < cnt && BF_SUCCESS == status; pg_id++) {
    for (; pg_queue < queues_per_pg && i < cnt && BF_SUCCESS == status;
         i++, pg_queue++) {
      auto curr_key =
          static_cast<BfRtTMQueueTableKey *>((*key_data_pairs)[i].first);
      auto curr_data =
          static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

      curr_key->setId(pg_id, pg_queue);

      // TODO: Acquire the Queue mapping state lock for the pg_id.

      if (this->isVolatile()) {
        status = this->tableGetEntry(dev_tgt, pg_id, pg_queue, curr_data);
      } else {
        std::lock_guard<std::mutex> lock(this->entry_lock);
        status = this->tableGetEntry(dev_tgt, pg_id, pg_queue, curr_data);
      }

      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s Can't get next entry for "
            " dev_id=%d pipe_id=%d pg_id=%d, pg_queue=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue);
        // BF_OBJECT_NOT_FOUND also is strange at the fixed TM tables
        if (BF_OBJECT_NOT_FOUND == status) {
          BF_RT_DBGCHK(0);
          status = BF_UNEXPECTED;
        }
      }
    }  // for (pg_queue)

    pg_queue = 0;
  }  // for (pg_id)

  *num_returned = i;
  return status;
}

//----------------------------------------------------------------------------
bf_status_t BfRtTMQueueCfgTable::tableGetField(const bf_rt_target_t &dev_tgt,
                                               bf_rt_id_t data_id,
                                               bf_tm_pg_t pg_id,
                                               uint8_t pg_queue,
                                               bf_dev_port_t port_id,
                                               bf_tm_queue_t queue_nr,
                                               BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_SUCCESS;
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t action_id = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("mirror_drop_destination", action_id, data_id)) {
    bool mirror_drop_destination = false;
    bf_dev_port_t drop_port = 0;
    bf_tm_queue_t drop_queue = 0;

    status_api = trafficMgr->bfTMPipeMirrorOnDropDestGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, &drop_port, &drop_queue);
    if (BF_SUCCESS == status_api) {
      mirror_drop_destination =
          ((drop_port == port_id) && (drop_queue == queue_nr));

      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d does %s mirror drop to "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          (mirror_drop_destination) ? "" : "NOT",
          pg_id,
          pg_queue,
          drop_port,
          drop_queue);

      status = q_data->setValue(data_id, mirror_drop_destination);
    }
  } else if (this->dataFieldDeclared("pfc_cos", action_id, data_id)) {
    uint8_t pfc_cos = 0;
    status_api = trafficMgr->bfTMQueuePfcCosGet(
        dev_tgt.dev_id, port_id, queue_nr, &pfc_cos);
    if (BF_SUCCESS == status_api) {
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d port_id=%d queue_nr=%d "
          "PFC CoS=0x%02x",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          port_id,
          queue_nr,
          pfc_cos);

      status = q_data->setValue(data_id, &pfc_cos, sizeof(uint8_t));
    }
  } else if (this->dataFieldDeclared(
                 "depth_report_enable", action_id, data_id)) {
    bool depth_report_enable = false;

    status_api = trafficMgr->bfTMQueueVisibleGet(
        dev_tgt.dev_id, port_id, queue_nr, &depth_report_enable);
    if (BF_SUCCESS == status_api) {
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "QSTAT depth report %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          (depth_report_enable) ? "ENABLED" : "DISABLED");

      status = q_data->setValue(data_id, depth_report_enable);
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s field_id=%d set value failed, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              status);
  }
  if (BF_SUCCESS != status_api) {
    LOG_ERROR(
        "%s:%d %s field_id=%d dev_id=%d pipe_id=%d pg_id=%d pg_queue=%d "
        "(port_id=%d queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        data_id,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status_api,
        bf_err_str(status_api));
    status = status_api;
  }

  return status;
}

bf_status_t BfRtTMQueueCfgTable::tableSetField(const bf_rt_target_t &dev_tgt,
                                               bf_tm_pg_t pg_id,
                                               uint8_t pg_queue,
                                               bf_dev_port_t port_id,
                                               bf_tm_queue_t queue_nr,
                                               const BfRtTMTableData &q_data,
                                               bf_rt_id_t data_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_SUCCESS;
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t action_id = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("pfc_cos", action_id, data_id)) {
    uint8_t pfc_cos = 0;

    status = q_data.getValue(data_id, sizeof(uint8_t), &pfc_cos);

    if (BF_SUCCESS == status) {
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d pg_id=%d pg_queue=%d (port_id=%d "
          "queue_nr=%d) PFC CoS=0x%02x",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          pfc_cos);
      status_api = trafficMgr->bfTMQueuePfcCosSet(
          dev_tgt.dev_id, port_id, queue_nr, pfc_cos);
    }
  } else if (this->dataFieldDeclared(
                 "depth_report_enable", action_id, data_id)) {
    bool depth_report_enable = false;

    status = q_data.getValue(data_id, &depth_report_enable);

    if (BF_SUCCESS == status) {
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d pg_id=%d pg_queue=%d (port_id=%d "
          "queue_nr=%d) QSTAT depth report %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          (depth_report_enable) ? "ENABLED" : "DISABLED");
      status_api = trafficMgr->bfTMQueueVisibleSet(
          dev_tgt.dev_id, port_id, queue_nr, depth_report_enable);
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s field_id=%d get value failed ,rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              status);
  }
  if (BF_SUCCESS != status_api) {
    LOG_ERROR(
        "%s:%d %s field_id=%d dev_id=%d pipe_id=%d pg_id=%d pg_queue=%d "
        "(port_id=%d queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        data_id,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status_api,
        bf_err_str(status_api));
    status = status_api;
  }

  return status;
}

bf_status_t BfRtTMQueueCfgTable::tableDefaultEntryGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status;

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *q_data = static_cast<BfRtTMTableData *>(data);
  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();

  if (activeDataFields.empty()) {
    LOG_DBG("%s:%d %s reset to default active fields",
            __func__,
            __LINE__,
            this->table_name_get().c_str());

    status = q_data->reset();
    if (status) {
      LOG_ERROR("%s:%d %s Can't reset the data given",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_status_t status_get = BF_INVALID_ARG;

  // mirror_drop_destination depends on a queue number which we do not have.
  BFRT_TM_SET_Q_DFT_FIELD("mirror_drop_destination", q_data, false);

  //---
  uint8_t pfc_cos = 0;
  status = trafficMgr->bfTMQueueCfgDefaultsGet(dev_tgt.dev_id, &pfc_cos);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't get TM defaults for pfc_cos, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
    return status;
  }
  BFRT_TM_SET_Q_DFT_FIELD("pfc_cos", q_data, static_cast<uint64_t>(pfc_cos));

  //---
  bool depth_report_enable = false;
  status = trafficMgr->bfTMQueueVisibleDefaultsGet(dev_tgt.dev_id,
                                                   &depth_report_enable);
  if (BF_NOT_SUPPORTED != status) {
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s Can't get TM defaults for depth_report_enable, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          status,
          bf_err_str(status));
      return status;
    }
    BFRT_TM_SET_Q_DFT_FIELD("depth_report_enable", q_data, depth_report_enable);
  }

  return status_get;
}

//----------------------------------------------------------------------------

bf_status_t BfRtTMQueueMapTable::tableGetEntry(const bf_rt_target_t &dev_tgt,
                                               bf_tm_pg_t pg_id,
                                               uint8_t pg_queue,
                                               BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (q_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();

  // Convert the port group queue to the port queue number
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      activeDataFields.size(),
      q_data->hasValues());

  bf_status_t status_get = BF_UNEXPECTED;
  std::string field_name = "";

  //--- Process common data fields
  uint8_t queues_per_pg = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, nullptr, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  //---
  BFRT_TM_SET_Q_DATA_FIELD("dev_port", q_data, static_cast<uint64_t>(port_id));

  BFRT_TM_SET_Q_DATA_FIELD("queue_nr", q_data, static_cast<uint64_t>(queue_nr));

  BFRT_TM_SET_Q_DATA_FIELD(
      "ingress_qid_max", q_data, static_cast<uint64_t>(queues_per_pg));

  {  //--- Related fields
    bf_rt_id_t f_ingress_qid_count = 0;
    bf_rt_id_t f_ingress_qid_list = 0;

    status = this->dataFieldIdGet("ingress_qid_count", &f_ingress_qid_count);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s no field_id for ingress_qid_count",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
    if (!(q_data->checkFieldActive(f_ingress_qid_count))) {
      f_ingress_qid_count = 0;
    }

    status = this->dataFieldIdGet("ingress_qid_list", &f_ingress_qid_list);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s no field_id for ingress_qid_list",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
    if (!(q_data->checkFieldActive(f_ingress_qid_list))) {
      f_ingress_qid_list = 0;
    }

    if (f_ingress_qid_count || f_ingress_qid_list) {
      uint8_t q_map_count = 0;

      std::unique_ptr<uint8_t[]> q_buffer(new uint8_t[queues_per_pg]);
      uint8_t *q_mapping = q_buffer.get();
      if (q_mapping == nullptr) {
        LOG_ERROR("%s:%d %s Failed to allocate buffer",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str());
        return BF_NO_SYS_RESOURCES;
      }

      status = trafficMgr->bfTMPortQMappingGet(
          dev_tgt.dev_id, port_id, &q_map_count, q_mapping);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d get queue mappings failed",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  pg_id,
                  pg_queue);
        return status;
      }
      if (q_map_count > queues_per_pg) {
        LOG_ERROR(
            "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
            "incorrect number of queues mapped %d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            pg_id,
            pg_queue,
            port_id,
            queue_nr,
            q_map_count);
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
      }

      /* Inverse the port queue map array: its index is an inbount Queue ID,
       * and an item is the port queue number less than the number of
       * the Port group queues carved to this port.
       * The resulting list will keep indexes of items with the same value
       * of the queue_nr a.k.a pg_queue.
       * For example:
       * [0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7]
       * gives [1, 9, 17, 25] for queue_nr==1
       */
      uint8_t num_q = 0;
      std::vector<bf_rt_id_t> qid_list;
      bool f_dbg_m = BFRT_TM_DEBUG;
      std::stringstream debug_map;
      debug_map.str("");

      if (q_map_count) {
        /* Assuming it is possible for a port to have 0 mappings. */
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          if (f_dbg_m) {
            debug_map << (static_cast<int>(q_mapping[q])) << " ";
          }
          if (q_mapping[q] >= q_map_count) {
            LOG_ERROR(
                "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
                "incorrect mapping of queue_nr=%d to QID=%d > %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                pg_id,
                pg_queue,
                port_id,
                queue_nr,
                q_mapping[q],
                q,
                q_map_count);
            BF_RT_DBGCHK(0);
            return BF_UNEXPECTED;
          }

          if (q_mapping[q] == queue_nr) {
            // if we'll need a byte array data field as well.
            // q_mapping[num_q] = q;
            num_q++;
            qid_list.push_back(static_cast<bf_rt_id_t>(q));
          }
        }
      }

      LOG_DBG(
          "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "q_cnt=%d map=[%s]",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          q_map_count,
          debug_map.str().c_str());

      if (f_ingress_qid_count) {
        status = q_data->setValue(f_ingress_qid_count, (uint64_t)num_q);
        if (status != BF_SUCCESS) {
          LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d field_id=%d set failed",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    pg_id,
                    pg_queue,
                    f_ingress_qid_count);
          return status;
        }
      }
      if (f_ingress_qid_list) {
        status = q_data->setValue(f_ingress_qid_list, qid_list);
        if (status != BF_SUCCESS) {
          LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d field_id=%d set failed",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    pg_id,
                    pg_queue,
                    f_ingress_qid_list);
          return status;
        }
      }
    }  //- Set ingress_qid_* fields
  }    //-- Related data fields

  return status_get;
}

//----------------------------------------------------------------------------

bf_status_t BfRtTMQueueColorTable::tableGetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (q_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      activeDataFields.size(),
      q_data->hasValues());

  bf_status_t status_get = BF_UNEXPECTED;
  std::string field_name = "";

  for (bf_rt_id_t data_id : activeDataFields) {
    status = this->dataFieldNameGet(data_id, &field_name);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get name for field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id);
      return BF_INVALID_ARG;
    }

    if ("drop_enable" == field_name) {
      bool drop_enabled = false;

      status = trafficMgr->bfTMQueueColorDropGet(
          dev_tgt.dev_id, port_id, queue_nr, &drop_enabled);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get color drop mode for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "color drop %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          (drop_enabled) ? "ENABLED" : "DISABLED");

      status = q_data->setValue(data_id, drop_enabled);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;

    } else if ("drop_limit_yellow" == field_name) {
      bf_tm_queue_color_limit_t yellow_drop_limit =
          BF_TM_Q_COLOR_LIMIT_75_PERCENT;

      status = trafficMgr->bfTMQueueColorDropLimitGet(dev_tgt.dev_id,
                                                      port_id,
                                                      queue_nr,
                                                      BF_TM_COLOR_YELLOW,
                                                      &yellow_drop_limit);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get yellow drop limit settings "
            " dev_id=%d pipe_id=%d port_id=%d queue_nr=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            port_id,
            queue_nr);
        return status;
      }
      std::string yellow_pct;
      try {
        yellow_pct = color_limit_to_str.at(yellow_drop_limit);
      } catch (std::out_of_range &) {
        return BF_UNEXPECTED;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "yellow drop limit %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          yellow_pct.c_str());

      status = q_data->setValue(data_id, yellow_pct);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;

    } else if ("drop_limit_red" == field_name) {
      bf_tm_queue_color_limit_t red_drop_limit = BF_TM_Q_COLOR_LIMIT_75_PERCENT;

      status = trafficMgr->bfTMQueueColorDropLimitGet(
          dev_tgt.dev_id, port_id, queue_nr, BF_TM_COLOR_RED, &red_drop_limit);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get red drop limit settings "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      std::string red_pct;
      try {
        red_pct = color_limit_to_str.at(red_drop_limit);
      } catch (std::out_of_range &) {
        return BF_UNEXPECTED;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "red drop limit %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          red_pct.c_str());

      status = q_data->setValue(data_id, red_pct);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;

    } else if ("hysteresis_yellow_cells" == field_name) {
      bf_tm_thres_t hyst_yellow_cells = 0;

      status = trafficMgr->bfTMQueueColorHysteresisGet(dev_tgt.dev_id,
                                                       port_id,
                                                       queue_nr,
                                                       BF_TM_COLOR_YELLOW,
                                                       &hyst_yellow_cells);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get yellow hysteresis for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "yellow hysteresis %d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          hyst_yellow_cells);

      status =
          q_data->setValue(data_id, static_cast<uint64_t>(hyst_yellow_cells));
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;

    } else if ("hysteresis_red_cells" == field_name) {
      bf_tm_thres_t hyst_red_cells = 0;

      status = trafficMgr->bfTMQueueColorHysteresisGet(
          dev_tgt.dev_id, port_id, queue_nr, BF_TM_COLOR_RED, &hyst_red_cells);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get red hysteresis for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "red hysteresis %d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          hyst_red_cells);

      status = q_data->setValue(data_id, static_cast<uint64_t>(hyst_red_cells));
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;

    } else {
      LOG_ERROR("%s:%d %s field_id=%d(%s) is not implemented",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id,
                field_name.c_str());
      return BF_NOT_IMPLEMENTED;
    }
  }  // for (field_id)

  if (activeDataFields.size() != q_data->hasValues()) {
    LOG_TRACE(
        "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
        "active_fields=%zu != with_values=%zu",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        (is_mapped) ? "" : "un-",
        activeDataFields.size(),
        q_data->hasValues());
    status_get = BF_UNEXPECTED;
  }

  return status_get;
}

bf_status_t BfRtTMQueueColorTable::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                                 const BfRtTMTableData &q_data,
                                                 bf_tm_pg_t pg_id,
                                                 uint8_t pg_queue) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const std::set<bf_rt_id_t> &assignedDataFields = q_data.getAssignedFields();

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  bf_status_t status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  if (!is_mapped) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d is not mapped to a port",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return BF_NOT_READY;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      q_data.getActiveFields().size(),
      q_data.hasValues());

  bf_status_t status_set = BF_UNEXPECTED;
  std::string field_name = "";

  for (bf_rt_id_t data_id : assignedDataFields) {
    // Successful field_id to name resolution also validates that
    // a platform-specific field is allowed for the table.
    status = this->dataFieldNameGet(data_id, &field_name);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get name for field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id);
      return BF_INVALID_ARG;
    }

    if ("drop_enable" == field_name) {
      bool drop_enable = false;

      status = q_data.getValue(data_id, &drop_enable);
      if (status) {
        LOG_ERROR("%s:%d %s Can't get Data Object value for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "color drop %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          (drop_enable) ? "ENABLE" : "DISABLE");

      status = trafficMgr->bfTMQueueColorDropSet(
          dev_tgt.dev_id, port_id, queue_nr, drop_enable);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set color drop mode for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      status_set = BF_SUCCESS;

    } else if ("drop_limit_yellow" == field_name) {
      bf_tm_queue_color_limit_t drop_limit_yellow =
          BF_TM_Q_COLOR_LIMIT_75_PERCENT;
      std::string drop_pct_yellow = "";

      status = q_data.getValue(data_id, &drop_pct_yellow);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "yellow drop limit %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          drop_pct_yellow.c_str());
      try {
        drop_limit_yellow = str_to_color_limit.at(drop_pct_yellow);
      } catch (std::out_of_range &) {
        LOG_ERROR("%s:%d %s Invalid value %s for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  drop_pct_yellow.c_str(),
                  field_name.c_str());
        return BF_INVALID_ARG;
      }
      status = trafficMgr->bfTMQueueColorDropLimitSet(dev_tgt.dev_id,
                                                      port_id,
                                                      queue_nr,
                                                      BF_TM_COLOR_YELLOW,
                                                      drop_limit_yellow);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set yellow drop limit for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }

      status_set = BF_SUCCESS;

    } else if ("drop_limit_red" == field_name) {
      bf_tm_queue_color_limit_t drop_limit_red = BF_TM_Q_COLOR_LIMIT_75_PERCENT;
      std::string drop_pct_red = "";

      status = q_data.getValue(data_id, &drop_pct_red);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "red drop limit %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          drop_pct_red.c_str());
      try {
        drop_limit_red = str_to_color_limit.at(drop_pct_red);
      } catch (std::out_of_range &) {
        LOG_ERROR("%s:%d %s Invalid value %s for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  drop_pct_red.c_str(),
                  field_name.c_str());
        return BF_INVALID_ARG;
      }
      status = trafficMgr->bfTMQueueColorDropLimitSet(
          dev_tgt.dev_id, port_id, queue_nr, BF_TM_COLOR_RED, drop_limit_red);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set red drop limit for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }

      status_set = BF_SUCCESS;

    } else if ("hysteresis_yellow_cells" == field_name) {
      uint64_t hysteresis_yellow_cells = 0;

      status = q_data.getValue(data_id, &hysteresis_yellow_cells);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "yellow hysteresis %" PRIu64,
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          hysteresis_yellow_cells);
      status = trafficMgr->bfTMQueueColorHysteresisSet(
          dev_tgt.dev_id,
          port_id,
          queue_nr,
          BF_TM_COLOR_YELLOW,
          static_cast<bf_tm_thres_t>(hysteresis_yellow_cells));
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set yellow hysteresis for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      status_set = BF_SUCCESS;

    } else if ("hysteresis_red_cells" == field_name) {
      uint64_t hysteresis_red_cells = 0;

      status = q_data.getValue(data_id, &hysteresis_red_cells);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "red hysteresis %" PRIu64,
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          hysteresis_red_cells);
      status = trafficMgr->bfTMQueueColorHysteresisSet(
          dev_tgt.dev_id,
          port_id,
          queue_nr,
          BF_TM_COLOR_RED,
          static_cast<bf_tm_thres_t>(hysteresis_red_cells));
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set red hysteresis for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      status_set = BF_SUCCESS;

    } else {
      LOG_ERROR("%s:%d %s field_id=%d(%s) is not implemented",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id,
                field_name.c_str());
      return BF_NOT_IMPLEMENTED;
    }
  }  // for (field_id)

  return status_set;
}

bf_status_t BfRtTMQueueColorTable::tableDefaultEntryGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status;

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *q_data = static_cast<BfRtTMTableData *>(data);
  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();

  if (activeDataFields.empty()) {
    LOG_DBG("%s:%d %s reset to default active fields",
            __func__,
            __LINE__,
            this->table_name_get().c_str());

    status = q_data->reset();
    if (status) {
      LOG_ERROR("%s:%d %s Can't reset the data given",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
  }

  bf_tm_queue_color_limit_t yellow_drop_limit = BF_TM_Q_COLOR_LIMIT_75_PERCENT;
  bf_tm_queue_color_limit_t red_drop_limit = BF_TM_Q_COLOR_LIMIT_75_PERCENT;
  bf_tm_thres_t hyst_yellow_cells = 0;
  bf_tm_thres_t hyst_red_cells = 0;
  bool drop_enable = false;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMQueueColorDefaultsGet(dev_tgt.dev_id,
                                                 &drop_enable,
                                                 &yellow_drop_limit,
                                                 &hyst_yellow_cells,
                                                 &red_drop_limit,
                                                 &hyst_red_cells);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM defaults %s(%d)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_err_str(status),
              status);
    return status;
  }

  std::string yellow_drop_limit_str = "";
  std::string red_drop_limit_str = "";

  try {
    yellow_drop_limit_str = color_limit_to_str.at(yellow_drop_limit);
    red_drop_limit_str = color_limit_to_str.at(red_drop_limit);
  } catch (std::out_of_range &) {
    LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
    return BF_UNEXPECTED;
  }

  bf_status_t status_get = BF_INVALID_ARG;

  BFRT_TM_SET_Q_DFT_FIELD("drop_enable", q_data, drop_enable);

  BFRT_TM_SET_Q_DFT_FIELD("drop_limit_yellow", q_data, yellow_drop_limit_str);

  uint64_t hyst_yellow_val = static_cast<uint64_t>(hyst_yellow_cells);
  BFRT_TM_SET_Q_DFT_FIELD("hysteresis_yellow_cells", q_data, hyst_yellow_val);

  BFRT_TM_SET_Q_DFT_FIELD("drop_limit_red", q_data, red_drop_limit_str);

  uint64_t hyst_red_val = static_cast<uint64_t>(hyst_red_cells);
  BFRT_TM_SET_Q_DFT_FIELD("hysteresis_red_cells", q_data, hyst_red_val);

  return status_get;
}

//-----------------------
#define BFRT_TM_Q_EXPECT_FIELD_SET(field_id_, a_val_)      \
  {                                                        \
    if (field_id_) {                                       \
      status = q_data->setValue((field_id_), (a_val_));    \
      if (status) {                                        \
        LOG_ERROR(                                         \
            "%s:%d %s Can't set Data Object for "          \
            "action_id=%d field_id=%d",                    \
            __func__,                                      \
            __LINE__,                                      \
            this->table_name_get().c_str(),                \
            data_action_id,                                \
            (field_id_));                                  \
        return status;                                     \
      }                                                    \
      LOG_DBG(                                             \
          "%s:%d %s dev_id=%d pipe_id=%d "                 \
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) " \
          "action_id=%d field_id=%d",                      \
          __func__,                                        \
          __LINE__,                                        \
          this->table_name_get().c_str(),                  \
          dev_tgt.dev_id,                                  \
          dev_tgt.pipe_id,                                 \
          pg_id,                                           \
          pg_queue,                                        \
          port_id,                                         \
          queue_nr,                                        \
          data_action_id,                                  \
          (field_id_));                                    \
      status_get = BF_SUCCESS;                             \
    }                                                      \
  }

bf_status_t BfRtTMQueueBufferTable::tableGetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    BfRtTMTableData *q_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (q_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();

  std::string action_name("shared_pool");
  bf_rt_id_t data_action_id = 0;
  bf_rt_id_t do_action_id = 0;

  bf_status_t status = q_data->actionIdGet(&data_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (data_action_id == 0 && activeDataFields.empty()) {
    // The bfrt_python makes get() request without action_id.
    LOG_DBG("%s:%d %s pg_id=%d pg_queue=%d unfold with action '%s'",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            pg_id,
            pg_queue,
            action_name.c_str());

    status = this->actionIdGet(action_name, &data_action_id);
    if (status) {
      LOG_ERROR("%s:%d %s No '%s' action to use with empty entry",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                action_name.c_str());
      BF_RT_DBGCHK(0);
      return status;
    }
    q_data->actionIdSet(data_action_id);

    std::vector<bf_rt_id_t> empty_fields;
    q_data->setActiveFields(empty_fields);
  } else {
    status = this->actionNameGet(data_action_id, &action_name);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Invalid action_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_action_id);
      return status;
    }
  }
  // Convert the port group queue to the port queue number
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      activeDataFields.size(),
      q_data->hasValues());

  bf_status_t status_get = BF_UNEXPECTED;
  std::string field_name = "";
  bf_rt_id_t data_id = 0;

  //---- Get data fields from HW

  std::string dynamic_baf_str("");
  bf_tm_queue_baf_t dynamic_baf;

  uint32_t hysteresis_cells = 0;
  uint32_t pool_max_cells = 0;

  std::string pool_id_str("");
  bf_tm_app_pool_t pool_id;

  //---- Select fields according to the action and TM APIs
  bf_rt_id_t f_pool_id = 0;
  bf_rt_id_t f_pool_max_cells = 0;
  bf_rt_id_t f_dynamic_baf = 0;
  bf_rt_id_t f_hysteresis_cells = 0;

  // This field might be read from HW in two different ways
  // depending on the action chosen.
  status = this->dataFieldIdGet(
      "hysteresis_cells", data_action_id, &f_hysteresis_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s action_id=%d has no field hysteresis_cells",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id);
    return status;
  }
  if (!(q_data->checkFieldActive(f_hysteresis_cells))) {
    f_hysteresis_cells = 0;
  }

  //---- Action-specific data retrieval from HW

  if (action_name == "shared_pool") {
    do_action_id = data_action_id;

    status = this->dataFieldIdGet("pool_id", do_action_id, &f_pool_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field pool_id",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data->checkFieldActive(f_pool_id))) {
      f_pool_id = 0;
    }

    status =
        this->dataFieldIdGet("pool_max_cells", do_action_id, &f_pool_max_cells);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field pool_max_calls",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data->checkFieldActive(f_pool_max_cells))) {
      f_pool_max_cells = 0;
    }

    status = this->dataFieldIdGet("dynamic_baf", do_action_id, &f_dynamic_baf);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field dynamic_baf",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data->checkFieldActive(f_dynamic_baf))) {
      f_dynamic_baf = 0;
    }

  } else if (action_name == "buffer_only") {
    do_action_id = data_action_id;

  } else {
    LOG_ERROR("%s:%d %s unknown action_id=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id,
              action_name.c_str());
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  //--- Get data from HW depending on what set of fields is requested.

  if (f_pool_id || f_pool_max_cells || f_dynamic_baf) {
    status = trafficMgr->bfTMQueueAppPoolGet(dev_tgt.dev_id,
                                             port_id,
                                             queue_nr,
                                             &pool_id,
                                             &pool_max_cells,
                                             &dynamic_baf,
                                             &hysteresis_cells);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get queue app pool settings for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    try {
      dynamic_baf_str = qbaf_to_str.at(dynamic_baf);
      pool_id_str = pool_to_str.at(pool_id);
    } catch (std::out_of_range &) {
      LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
      return BF_UNEXPECTED;
    }
    status_get = BF_SUCCESS;

  } else if (f_hysteresis_cells) {
    status = trafficMgr->bfTMQueueHysteresisGet(
        dev_tgt.dev_id, port_id, queue_nr, &hysteresis_cells);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get hysteresis cells for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    status_get = BF_SUCCESS;
  }

  //-- Assign retrieved values to the data fields.
  BFRT_TM_Q_EXPECT_FIELD_SET(f_hysteresis_cells,
                             static_cast<uint64_t>(hysteresis_cells));

  BFRT_TM_Q_EXPECT_FIELD_SET(f_pool_id, pool_id_str);

  BFRT_TM_Q_EXPECT_FIELD_SET(f_pool_max_cells,
                             static_cast<uint64_t>(pool_max_cells));

  BFRT_TM_Q_EXPECT_FIELD_SET(f_dynamic_baf, dynamic_baf_str);

  //--- Process common data fields
  //---
  {
    uint32_t field_val = 0;
    field_name.assign("guaranteed_cells");

    status = this->dataFieldIdGet(field_name, data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (q_data->checkFieldActive(data_id)) {
      status = trafficMgr->bfTMQueueGuaranteedCellsGet(
          dev_tgt.dev_id, port_id, queue_nr, &field_val);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get field_id=%d for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id,
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }

      BFRT_TM_Q_EXPECT_FIELD_SET(data_id, static_cast<uint64_t>(field_val));
    }
  }
  //---
  {
    bool field_val = false;
    field_name.assign("tail_drop_enable");

    status = this->dataFieldIdGet(field_name, data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (q_data->checkFieldActive(data_id)) {
      status = trafficMgr->bfTMQueueTailDropGet(
          dev_tgt.dev_id, port_id, queue_nr, &field_val);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get field_id=%d for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id,
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }

      BFRT_TM_Q_EXPECT_FIELD_SET(data_id, field_val);
    }
  }

  //--- Update action_id depending on the current buffering mode.
  if (action_name == "buffer_only") {
    uint32_t ap_cells = 0;
    status = trafficMgr->bfTMQueueAppPoolLimitGet(
        dev_tgt.dev_id, port_id, queue_nr, &ap_cells);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get shared pool usage limit for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    if (ap_cells != 0) {
      status = this->actionIdGet("shared_pool", &do_action_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Can't get action_id for 'shared_pool'",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str());
        BF_RT_DBGCHK(0);
        return status;
      }
      q_data->actionIdSet(do_action_id);
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
          "uses %d cells from a shared pool",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          ap_cells);
    }
    status_get = BF_SUCCESS;
  }

  return status_get;
}

bf_status_t BfRtTMQueueBufferTable::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                                  const BfRtTMTableData &q_data,
                                                  bf_tm_pg_t pg_id,
                                                  uint8_t pg_queue) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const std::set<bf_rt_id_t> &activeDataFields = q_data.getActiveFields();

  bf_status_t status_set = BF_INVALID_ARG;
  std::string field_name = "";
  bf_rt_id_t data_id = 0;

  bf_rt_id_t data_action_id = 0;
  bf_rt_id_t do_action_id = 0;

  bf_status_t status = q_data.actionIdGet(&data_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  std::string action_name;
  status = this->actionNameGet(data_action_id, &action_name);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid action_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id);
    return status;
  }

  // Convert the port group queue to the port queue number
  bf_dev_port_t port_id = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &port_id, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d %smapped) "
      "active_fields=%zu with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_queue,
      port_id,
      queue_nr,
      (is_mapped) ? "" : "un-",
      activeDataFields.size(),
      q_data.hasValues());

  if (!is_mapped) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d is not mapped to a port",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return BF_NOT_READY;
  }

  //--- Get some related settings.
  std::string dynamic_baf_str("");
  bf_tm_queue_baf_t dynamic_baf = BF_TM_Q_BAF_80_PERCENT;

  uint64_t hysteresis_cells = 0;
  uint64_t pool_max_cells = 0;

  std::string pool_id_str("");
  bf_tm_app_pool_t pool_id = BF_TM_EG_APP_POOL_0;

  bf_rt_id_t f_pool_id = 0;
  bf_rt_id_t f_pool_max_cells = 0;
  bf_rt_id_t f_dynamic_baf = 0;
  bf_rt_id_t f_hysteresis_cells = 0;

  //---- Select what to set to HW.

  status = this->dataFieldIdGet(
      "hysteresis_cells", data_action_id, &f_hysteresis_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s action_id=%d has no field hysteresis_cells",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id);
    return status;
  }
  if (!(q_data.hasValue(f_hysteresis_cells))) {
    f_hysteresis_cells = 0;
  }

  //----------------
  if (action_name == "shared_pool") {
    do_action_id = data_action_id;

    status = this->dataFieldIdGet("pool_id", do_action_id, &f_pool_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field pool_id",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data.hasValue(f_pool_id))) {
      f_pool_id = 0;
    }

    status =
        this->dataFieldIdGet("pool_max_cells", do_action_id, &f_pool_max_cells);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field pool_max_calls",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data.hasValue(f_pool_max_cells))) {
      f_pool_max_cells = 0;
    }

    status = this->dataFieldIdGet("dynamic_baf", do_action_id, &f_dynamic_baf);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field dynamic_baf",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id);
      return status;
    }
    if (!(q_data.hasValue(f_dynamic_baf))) {
      f_dynamic_baf = 0;
    }
  } else if (action_name == "buffer_only") {
    do_action_id = data_action_id;
  } else {
    LOG_ERROR("%s:%d %s unknown action_id=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id,
              action_name.c_str());
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  //-- Get the data fields.

  if (f_hysteresis_cells) {
    status = q_data.getValue(f_hysteresis_cells, &hysteresis_cells);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get Data Object for "
          "action_id=%d field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          do_action_id,
          f_hysteresis_cells);
      return status;
    }
    LOG_DBG(
        "%s:%d %s dev_id=%d pipe_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
        "action_id=%d field_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        do_action_id,
        f_hysteresis_cells);
  }
  if (f_pool_id) {
    status = q_data.getValue(f_pool_id, &pool_id_str);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get Data Object for "
          "action_id=%d field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          do_action_id,
          f_pool_id);
      return status;
    }
    LOG_DBG(
        "%s:%d %s dev_id=%d pipe_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
        "action_id=%d field_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        do_action_id,
        f_pool_id);
    try {
      pool_id = str_to_pool.at(pool_id_str);
    } catch (std::out_of_range &) {
      LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
      return BF_INVALID_ARG;
    }
  }
  if (f_pool_max_cells) {
    status = q_data.getValue(f_pool_max_cells, &pool_max_cells);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't get Data Object for "
          "action_id=%d field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          do_action_id,
          f_pool_max_cells);
      return status;
    }
    LOG_DBG(
        "%s:%d %s dev_id=%d pipe_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
        "action_id=%d field_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        do_action_id,
        f_pool_max_cells);
  }
  if (f_dynamic_baf) {
    status = q_data.getValue(f_dynamic_baf, &dynamic_baf_str);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't set Data Object for "
          "action_id=%d field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          do_action_id,
          f_dynamic_baf);
      return status;
    }
    LOG_DBG(
        "%s:%d %s dev_id=%d pipe_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) "
        "action_id=%d field_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        do_action_id,
        f_dynamic_baf);
    try {
      dynamic_baf = str_to_qbaf.at(dynamic_baf_str);
    } catch (std::out_of_range &) {
      LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
      return BF_INVALID_ARG;
    }
  }

  //--- Set data to HW

  //--- Read from HW values missed in the Data.
  if (f_pool_id || f_pool_max_cells || f_dynamic_baf) {
    // We need to set at least one of these three.
    if (!(f_pool_id && f_pool_max_cells && f_dynamic_baf &&
          f_hysteresis_cells)) {
      // And at least one of these four is missed.
      bf_tm_queue_baf_t dynamic_baf_ = BF_TM_Q_BAF_80_PERCENT;
      bf_tm_app_pool_t pool_id_ = BF_TM_EG_APP_POOL_0;
      uint32_t hysteresis_cells_ = 0;
      uint32_t pool_max_cells_ = 0;

      status = trafficMgr->bfTMQueueAppPoolGet(dev_tgt.dev_id,
                                               port_id,
                                               queue_nr,
                                               &pool_id_,
                                               &pool_max_cells_,
                                               &dynamic_baf_,
                                               &hysteresis_cells_);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't get queue app pool settings for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      // Take only these missed from the data.
      if (!f_dynamic_baf) {
        dynamic_baf = dynamic_baf_;
      }
      if (!f_hysteresis_cells) {
        hysteresis_cells = static_cast<uint64_t>(hysteresis_cells_);
      }
      if (!f_pool_max_cells) {
        pool_max_cells = static_cast<uint64_t>(pool_max_cells_);
      }
      if (!f_pool_id) {
        pool_id = pool_id_;
      }
    }
    // All four is ready for write to HW.
    status = trafficMgr->bfTMQueueAppPoolSet(
        dev_tgt.dev_id,
        port_id,
        queue_nr,
        pool_id,
        static_cast<uint32_t>(pool_max_cells),
        dynamic_baf,
        static_cast<uint32_t>(hysteresis_cells));
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't set queue app pool settings for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    status_set = BF_SUCCESS;
  } else if (f_hysteresis_cells) {
    // In case of only this value to set.
    status = trafficMgr->bfTMQueueHysteresisSet(
        dev_tgt.dev_id,
        port_id,
        queue_nr,
        static_cast<uint32_t>(hysteresis_cells));
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't set hysteresis cells for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    status_set = BF_SUCCESS;
  }

  //--- Process common data fields

  //---
  {
    uint64_t field_val = 0;
    field_name.assign("guaranteed_cells");
    const char *field_note = "queue guaranteed minimum cells";

    status = this->dataFieldIdGet(field_name, do_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s action_id=%d has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                do_action_id,
                field_name.c_str());
      return status;
    }
    if (q_data.hasValue(data_id)) {
      status = q_data.getValue(data_id, &field_val);
      if (status) {
        LOG_ERROR("%s:%d %s Can't get Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status = trafficMgr->bfTMQueueGuaranteedCellsSet(
          dev_tgt.dev_id, port_id, queue_nr, static_cast<uint32_t>(field_val));
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set %s for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            field_note,
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) %s %" PRIu64,
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          field_note,
          field_val);
      status_set = BF_SUCCESS;
    }
  }
  //---
  {
    bool field_val = false;
    field_name.assign("tail_drop_enable");
    const char *field_note = "queue tail drop mode";

    status = this->dataFieldIdGet(field_name, data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (q_data.hasValue(data_id)) {
      status = q_data.getValue(data_id, &field_val);
      if (status) {
        LOG_ERROR("%s:%d %s Can't get Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status = trafficMgr->bfTMQueueTailDropSet(
          dev_tgt.dev_id, port_id, queue_nr, field_val);
      if (status) {
        LOG_ERROR(
            "%s:%d %s Can't set %s for "
            "dev_id=%d pipe_id=%d "
            "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            field_note,
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_queue,
            port_id,
            queue_nr);
        return status;
      }
      LOG_DBG(
          "%s:%d %s dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d) %s %s",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr,
          field_note,
          (field_val) ? "ENABLED" : "DISABLED");
      status_set = BF_SUCCESS;
    }
  }

  //--- Apply 'buffer_only' after all its values.
  if (action_name == "buffer_only") {
    status =
        trafficMgr->bfTMQueueAppPoolDisable(dev_tgt.dev_id, port_id, queue_nr);
    if (status) {
      LOG_ERROR(
          "%s:%d %s Can't disable application pool use for "
          "dev_id=%d pipe_id=%d "
          "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          pg_queue,
          port_id,
          queue_nr);
      return status;
    }
    LOG_DBG(
        "%s:%d %s Disabled application pool use for "
        "dev_id=%d pipe_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d queue_nr=%d)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr);
    status_set = BF_SUCCESS;
  }

  return status_set;
}

bf_status_t BfRtTMQueueBufferTable::tableDefaultEntryGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status;

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *q_data = static_cast<BfRtTMTableData *>(data);
  const std::set<bf_rt_id_t> &activeDataFields = q_data->getActiveFields();
  bf_rt_id_t dft_action_id = 0;

  status = actionIdGet("shared_pool", &dft_action_id);
  if (status) {
    LOG_ERROR("%s:%d %s No 'shared_pool' action to use for default entry",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return status;
  }

  bf_rt_id_t data_action_id = 0;
  status = q_data->actionIdGet(&data_action_id);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get action id from the data given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // If the received data has some fields with the default action id
  // then return defaults only to these fields.
  if (data_action_id != dft_action_id || activeDataFields.empty()) {
    LOG_DBG("%s:%d %s set active fields for action_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dft_action_id);

    status = q_data->reset(dft_action_id);
    if (status) {
      LOG_ERROR("%s:%d %s Can't reset the data given",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
  }

  bf_tm_queue_baf_t dynamic_baf = BF_TM_Q_BAF_80_PERCENT;
  bf_tm_app_pool_t pool_id = BF_TM_EG_APP_POOL_0;
  bool tail_drop_enable = false;
  uint32_t guaranteed_cells = 0;
  uint32_t hysteresis_cells = 0;
  uint32_t pool_max_cells = 0;

  {
    auto *trafficMgr = TrafficMgrIntf::getInstance();

    status = trafficMgr->bfTMQueueBufferDefaultsGet(dev_tgt.dev_id,
                                                    &guaranteed_cells,
                                                    &hysteresis_cells,
                                                    &tail_drop_enable,
                                                    &pool_id,
                                                    &pool_max_cells,
                                                    &dynamic_baf);
    if (status) {
      LOG_ERROR("%s:%d %s Can't get TM defaults %s(%d)",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                bf_err_str(status),
                status);
      return status;
    }
  }

  std::string dynamic_baf_str("");
  std::string pool_id_str("");

  try {
    dynamic_baf_str = qbaf_to_str.at(dynamic_baf);
    pool_id_str = pool_to_str.at(pool_id);
  } catch (std::out_of_range &) {
    LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
    return BF_UNEXPECTED;
  }

  bf_status_t status_get = BF_INVALID_ARG;

  BFRT_TM_SET_Q_ACT_FIELD(
      "tail_drop_enable", dft_action_id, q_data, tail_drop_enable);
  BFRT_TM_SET_Q_ACT_FIELD("pool_id", dft_action_id, q_data, pool_id_str);
  BFRT_TM_SET_Q_ACT_FIELD(
      "dynamic_baf", dft_action_id, q_data, dynamic_baf_str);

  BFRT_TM_SET_Q_ACT_FIELD("guaranteed_cells",
                          dft_action_id,
                          q_data,
                          static_cast<uint64_t>(guaranteed_cells));
  BFRT_TM_SET_Q_ACT_FIELD("hysteresis_cells",
                          dft_action_id,
                          q_data,
                          static_cast<uint64_t>(hysteresis_cells));
  BFRT_TM_SET_Q_ACT_FIELD("pool_max_cells",
                          dft_action_id,
                          q_data,
                          static_cast<uint64_t>(pool_max_cells));
  return status_get;
}

//----------------------------------------------------------------------------
bf_status_t BfRtTMQueueSchedCfgTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    BfRtTMTableData *q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == q_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMQueueSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  //--- Get data fields from HW.

  bf_rt_id_t err_data_id = 0;

  do {
    //-- Guaranteed bandwidth.
    if (sc_hlp.f_min_priority) {
      status = trafficMgr->bfTMQueueSchedGuaranteedPriorityGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_min_priority;
        break;
      }
    }
    if (sc_hlp.f_min_rate_enable) {
      status = trafficMgr->bfTMQueueSchedGuaranteedEnableGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.min_rate_enable);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_min_rate_enable;
        break;
      }
    }

    if (sc_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTMQueueSchedDwrrWeightGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Shaping bandwidth: set priority and then enable it.
    if (sc_hlp.f_max_priority) {
      status = trafficMgr->bfTMQueueSchedRemainingBwPriorityGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_max_priority;
        break;
      }
    }
    if (sc_hlp.f_max_rate_enable) {
      status = trafficMgr->bfTMQueueSchedShapingEnableGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.max_rate_enable);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_max_rate_enable;
        break;
      }
    }
    if (sc_hlp.f_advanced_flow_control) {
      status = trafficMgr->bfTMQueueSchedAdvFcModeGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.advanced_flow_control);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_advanced_flow_control;
        break;
      }
    }

    if (sc_hlp.f_scheduling_enable) {
      status = trafficMgr->bfTMQueueSchedEnableGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.scheduling_enable);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_scheduling_enable;
        break;
      }
    }

    if (sc_hlp.f_scheduling_speed) {
      bf_port_speeds_t q_speed = BF_SPEED_NONE;
      status = trafficMgr->bfTMQueueSchedSpeedGet(
          dev_tgt.dev_id, port_id, queue_nr, &q_speed);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_scheduling_speed;
        break;
      }
      sc_hlp.scheduling_speed = static_cast<bf_port_speed_t>(q_speed);
    }

    if (sc_hlp.f_pg_l1_node) {
      status = trafficMgr->bfTMQueueL1NodeGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_pg_l1_node;
        break;
      }
    }

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field_id=%d read value from dev_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.dev_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  status = sc_hlp.setFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't set fields pg_id=%d pg_queue=%d "
        "(port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  return status;
}

bf_status_t BfRtTMQueueSchedCfgTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    const BfRtTMTableData &q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  BfRtTMQueueSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  status = sc_hlp.getFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get fields pg_id=%d pg_queue=%d "
        "(port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  //--- Set data fields to HW.
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t err_data_id = 0;

  do {
    //-- Guaranteed bandwidth.
    if (sc_hlp.f_min_priority) {
      status = trafficMgr->bfTMQueueSchedGuaranteedPrioritySet(
          dev_tgt.dev_id, port_id, queue_nr, sc_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_min_priority;
        break;
      }
    }
    if (sc_hlp.f_min_rate_enable) {
      status = (sc_hlp.min_rate_enable)
                   ? trafficMgr->bfTMQueueSchedGuaranteedEnable(
                         dev_tgt.dev_id, port_id, queue_nr)
                   : trafficMgr->bfTMQueueSchedGuaranteedDisable(
                         dev_tgt.dev_id, port_id, queue_nr);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_min_rate_enable;
        break;
      }
    }

    if (sc_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTMQueueSchedDwrrWeightSet(
          dev_tgt.dev_id, port_id, queue_nr, sc_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Shaping bandwidth: set priority and then enable it.
    if (sc_hlp.f_max_priority) {
      status = trafficMgr->bfTMQueueSchedRemainingBwPrioritySet(
          dev_tgt.dev_id, port_id, queue_nr, sc_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_max_priority;
        break;
      }
    }
    if (sc_hlp.f_max_rate_enable) {
      status = (sc_hlp.max_rate_enable)
                   ? trafficMgr->bfTMQueueSchedShapingEnable(
                         dev_tgt.dev_id, port_id, queue_nr)
                   : trafficMgr->bfTMQueueSchedShapingDisable(
                         dev_tgt.dev_id, port_id, queue_nr);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_max_rate_enable;
        break;
      }
    }

    if (sc_hlp.f_advanced_flow_control) {
      status = trafficMgr->bfTMQueueSchedAdvFcModeSet(
          dev_tgt.dev_id, port_id, queue_nr, sc_hlp.advanced_flow_control);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_advanced_flow_control;
        break;
      }
    }

    if (sc_hlp.f_scheduling_enable) {
      status = (sc_hlp.scheduling_enable)
                   ? trafficMgr->bfTMQueueSchedEnable(
                         dev_tgt.dev_id, port_id, queue_nr)
                   : trafficMgr->bfTMQueueSchedDisable(
                         dev_tgt.dev_id, port_id, queue_nr);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_scheduling_enable;
        break;
      }
    }

    if (sc_hlp.f_pg_l1_node) {
      // L1 Node assignment have to be after scheduling_enable.
      // Firstly, the scheduling_enable assigns the queue to the port
      // on its default L1 Node, and only then pg_l1_node value might
      // be assigned correctly if it is bound to the same port as the queue.
      status = trafficMgr->bfTMQueueL1NodeSet(
          dev_tgt.dev_id, port_id, queue_nr, sc_hlp.pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_pg_l1_node;
        break;
      }
    }

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field_id=%d write value from dev_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.dev_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  return status;
}

bf_status_t BfRtTMQueueSchedCfgTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    BfRtTMTableData *q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == q_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMQueueSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  //--- Get data fields from HW.

  bf_rt_id_t err_data_id = 0;

  do {
    //-- Guaranteed bandwidth.
    if (sc_hlp.f_min_priority || sc_hlp.f_min_rate_enable) {
      status = trafficMgr->bfTMQueueSchedGuaranteedEnableDefaultsGet(
          dev_tgt.dev_id,
          port_id,
          queue_nr,
          &sc_hlp.min_rate_enable,
          &sc_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = (sc_hlp.f_min_rate_enable) ? sc_hlp.f_min_rate_enable
                                                 : sc_hlp.f_min_priority;
        break;
      }
    }

    if (sc_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTMQueueSchedDwrrWeightDefaultGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Shaping bandwidth.
    if (sc_hlp.f_max_priority || sc_hlp.f_max_rate_enable) {
      status = trafficMgr->bfTMQueueSchedShapingEnableDefaultsGet(
          dev_tgt.dev_id,
          port_id,
          queue_nr,
          &sc_hlp.max_rate_enable,
          &sc_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = (sc_hlp.f_max_rate_enable) ? sc_hlp.f_max_rate_enable
                                                 : sc_hlp.f_max_priority;
        break;
      }
    }

    if (sc_hlp.f_advanced_flow_control) {
      status = trafficMgr->bfTMQueueSchedAdvFcModeDefaultGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.advanced_flow_control);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_advanced_flow_control;
        break;
      }
    }

    if (sc_hlp.f_scheduling_enable) {
      status = trafficMgr->bfTMQueueSchedEnableDefaultGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.scheduling_enable);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_scheduling_enable;
        break;
      }
    }

    // The default L1 Node depends on the Queue to Port current mapping.
    if (sc_hlp.f_pg_l1_node) {
      status = trafficMgr->bfTMQueueL1NodeDefaultGet(
          dev_tgt.dev_id, port_id, queue_nr, &sc_hlp.pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = sc_hlp.f_pg_l1_node;
        break;
      }
    }

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field_id=%d read reset value from dev_id=%d "
        "pg_id=%d pg_queue=%d (port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.dev_id,
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  status = sc_hlp.setFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't reset fields pg_id=%d pg_queue=%d "
        "(port_id=%d, queue_nr=%d), rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        pg_id,
        pg_queue,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  return status;
}

//-----
bf_status_t BfRtTMQueueSchedCfgTable::tableClear(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->tableReset(session, dev_tgt, flags);
}

//-----
bf_status_t BfRtTMQueueSchedShapingTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->tableReset(session, dev_tgt, flags);
}

bf_status_t BfRtTMQueueSchedShapingTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    BfRtTMTableData *q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (q_data == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMQueueSchedMaxRateDefaultsGet(dev_tgt.dev_id,
                                                        port_id,
                                                        queue_nr,
                                                        &s_hlp.pps,
                                                        &s_hlp.max_burst_size,
                                                        &s_hlp.max_rate,
                                                        &s_hlp.provisioning);
  if (BF_SUCCESS == status) {
    status = trafficMgr->bfTMQueueSchedMinRateDefaultsGet(dev_tgt.dev_id,
                                                          port_id,
                                                          queue_nr,
                                                          &s_hlp.pps,
                                                          &s_hlp.min_burst_size,
                                                          &s_hlp.min_rate,
                                                          &s_hlp.provisioning);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s get reset valueis failed, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't reset fields dev_id=%d port_id=%d, queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
  }
  return status;
}

bf_status_t BfRtTMQueueSchedShapingTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    BfRtTMTableData *q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == q_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bool f_min_any = (s_hlp.f_min_rate || s_hlp.f_min_burst_size);
  bool f_max_any = (s_hlp.f_max_rate || s_hlp.f_max_burst_size);
  bool f_cmn_any = (s_hlp.f_unit || s_hlp.f_provisioning);

  // Always read max with cmn for simplicity.
  if (f_max_any || f_cmn_any) {
    status = trafficMgr->bfTMQueueSchedMaxRateGet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  &s_hlp.pps,
                                                  &s_hlp.max_burst_size,
                                                  &s_hlp.max_rate,
                                                  &s_hlp.provisioning);
  }
  if (BF_SUCCESS == status && f_min_any) {
    status = trafficMgr->bfTMQueueSchedMinRateGet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  &s_hlp.pps,
                                                  &s_hlp.min_burst_size,
                                                  &s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s read valueis failed, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d, queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
  }

  return status;
}

bf_status_t BfRtTMQueueSchedShapingTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t /* pg_id */,
    uint8_t /* pg_queue */,
    bf_dev_port_t port_id,
    bf_tm_queue_t queue_nr,
    const BfRtTMTableData &q_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(q_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  bool f_min_any = (s_hlp.f_min_rate || s_hlp.f_min_burst_size);
  bool f_min_all = (s_hlp.f_min_rate && s_hlp.f_min_burst_size);
  bool f_max_any = (s_hlp.f_max_rate || s_hlp.f_max_burst_size);
  bool f_max_all = (s_hlp.f_max_rate && s_hlp.f_max_burst_size);
  bool f_cmn_any = (s_hlp.f_unit || s_hlp.f_provisioning);
  bool f_cmn_all = (s_hlp.f_unit && s_hlp.f_provisioning);

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // Read other values from HW if only one in the block is requested
  if ((f_max_any && !(f_max_all)) || (f_cmn_any && !(f_cmn_all))) {
    status = trafficMgr->bfTMQueueSchedMaxRateGet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  &s_hlp.pps,
                                                  &s_hlp.max_burst_size,
                                                  &s_hlp.max_rate,
                                                  &s_hlp.provisioning);
  }
  if (BF_SUCCESS == status && f_min_any && !(f_min_all)) {
    status = trafficMgr->bfTMQueueSchedMinRateGet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  &s_hlp.pps,
                                                  &s_hlp.min_burst_size,
                                                  &s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pre-read values failed dev_id=%d port_id=%d queue_nr=%d "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        port_id,
        queue_nr,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.getFieldValues(*this, q_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get fields dev_id=%d port_id=%d queue_nr=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              queue_nr);
    return status;
  }

  if (f_max_any || f_cmn_any) {
    status = trafficMgr->bfTMQueueSchedMaxRateSet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  s_hlp.pps,
                                                  s_hlp.max_burst_size,
                                                  s_hlp.max_rate,
                                                  s_hlp.provisioning);
  }
  if (BF_SUCCESS == status && f_min_any) {
    status = trafficMgr->bfTMQueueSchedMinRateSet(dev_tgt.dev_id,
                                                  port_id,
                                                  queue_nr,
                                                  s_hlp.pps,
                                                  s_hlp.min_burst_size,
                                                  s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s write values failed, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
    return status;
  }

  return status;
}

}  // namespace bfrt
