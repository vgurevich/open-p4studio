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

#include "bf_rt_tm_table_impl_portgroup.hpp"

namespace bfrt {

//----------------------------------------------------------------------------
#define BFRT_TM_SET_PG_DATA_FIELD_INT(a_field_name_, p_data_, a_val_) \
  {                                                                   \
    bf_rt_id_t a_field_id_;                                           \
    uint64_t a_field_val_ = static_cast<uint64_t>(a_val_);            \
                                                                      \
    status = this->dataFieldIdGet((a_field_name_), &a_field_id_);     \
    if (status != BF_SUCCESS) {                                       \
      LOG_ERROR("%s:%d %s has no field %s",                           \
                __func__,                                             \
                __LINE__,                                             \
                this->table_name_get().c_str(),                       \
                (a_field_name_));                                     \
      return status;                                                  \
    }                                                                 \
    if (p_data_->checkFieldActive(a_field_id_)) {                     \
      LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d pg_id=%d %s=0x%" PRIX64, \
              __func__,                                               \
              __LINE__,                                               \
              this->table_name_get().c_str(),                         \
              dev_tgt.dev_id,                                         \
              dev_tgt.pipe_id,                                        \
              pg_id,                                                  \
              (a_field_name_),                                        \
              a_field_val_);                                          \
      status = p_data_->setValue(a_field_id_, a_field_val_);          \
      if (status) {                                                   \
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",            \
                  __func__,                                           \
                  __LINE__,                                           \
                  this->table_name_get().c_str(),                     \
                  (a_field_name_));                                   \
        return status;                                                \
      }                                                               \
      status_get = BF_SUCCESS;                                        \
    }                                                                 \
  }

bf_status_t BfRtTMPortGroupCfgTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey & /* key */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  return tableDefaultEntryGet(session, dev_tgt, flags, data);
}

bf_status_t BfRtTMPortGroupCfgTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey * /* key */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  return tableDefaultEntryGet(session, dev_tgt, flags, data);
}

bf_status_t BfRtTMPortGroupCfgTable::tableDefaultEntryGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_get = BF_INVALID_ARG;
  bf_status_t status;

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given to set defaults",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *qData = static_cast<BfRtTMTableData *>(data);

  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t pg_id = pg_per_pipe;  // Dummy value for default entry logging

  BFRT_TM_SET_PG_DATA_FIELD_INT("ingress_qid_max", qData, queues_per_pg);
  BFRT_TM_SET_PG_DATA_FIELD_INT("pg_queues", qData, queues_per_pg);

  return status_get;
}

bf_status_t BfRtTMPortGroupCfgTable::tableUsageGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t & /* dev_tgt */,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  bf_status_t status = BF_SUCCESS;

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  *count = 1;  // This fixed function table has only one record

  return status;
}

bf_status_t BfRtTMPortGroupCfgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  /* This TM table has no key defined for its single read-only entry. */
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTableKeyObj(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupCfgTable::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  /* Do nothing for the table without a key. */
  return BF_SUCCESS;
}

//--------------------------------------------------------------------------

bf_status_t BfRtTMPortGroupTable::tableDefaultEntryGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_get = BF_INVALID_ARG;
  bf_status_t status;

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given to set defaults",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *qData = static_cast<BfRtTMTableData *>(data);
  bf_rt_id_t dft_action_id = 0;

  status = actionIdGet("map", &dft_action_id);
  if (status) {
    LOG_ERROR("%s:%d %s No 'map' action to use for default entry",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return status;
  }

  bf_rt_id_t data_action_id = 0;
  status = qData->actionIdGet(&data_action_id);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get action id from the data given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // If the received data has some fields with the default action id
  // then return defaults only to these fields.
  if (data_action_id != dft_action_id || qData->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s set active fields for action_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dft_action_id);

    status = qData->reset(dft_action_id);
    if (status) {
      LOG_ERROR("%s:%d %s Can't reset the data given",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
  }

  uint8_t queues_per_pg = 0;
  uint8_t ports_per_pg = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, nullptr, &queues_per_pg, &ports_per_pg);
  if (status || ports_per_pg == 0) {
    // Also if ports_per_pg happens to be zero.
    LOG_ERROR("%s:%d %s Can't get TM configuration %s(%d)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_err_str(status),
              status);
    return status;
  }

  //------------
  {
    const std::string field_name("pg_dev_ports");
    bf_rt_id_t f_pg_dev_ports = 0;

    status = this->dataFieldIdGet(field_name, dft_action_id, &f_pg_dev_ports);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (qData->checkFieldActive(f_pg_dev_ports)) {
      LOG_DBG("%s:%d %s %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_name.c_str());
      // 'pg_dev_ports' is pg_id dependent, so empty array is default.
      std::vector<bf_rt_id_t> pg_ports;
      status = qData->setValue(f_pg_dev_ports, pg_ports);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;
    }
  }

  uint8_t dft_q_per_port = queues_per_pg / ports_per_pg;
  //----
  {
    const std::string field_name("port_queue_count");
    bf_rt_id_t f_port_queue_count = 0;

    status =
        this->dataFieldIdGet(field_name, dft_action_id, &f_port_queue_count);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (qData->checkFieldActive(f_port_queue_count)) {
      LOG_DBG("%s:%d %s %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_name.c_str());
      std::vector<bf_rt_id_t> pg_counts;
      for (uint8_t p = 0; p < ports_per_pg; p++) {
        pg_counts.push_back(static_cast<bf_rt_id_t>(dft_q_per_port));
      }
      status = qData->setValue(f_port_queue_count, pg_counts);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;
    }
  }
  //----
  std::stringstream field_name_s;
  bf_rt_id_t data_id = 0;
  {
    for (uint8_t p = 0; p < ports_per_pg; p++) {
      field_name_s.str("");
      field_name_s << "ingress_qid_map_" << (static_cast<int>(p));
      status =
          this->dataFieldIdGet(field_name_s.str(), dft_action_id, &data_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s has no field %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name_s.str().c_str());
        return status;
      }
      if (qData->checkFieldActive(data_id)) {
        LOG_DBG("%s:%d %s %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name_s.str().c_str());
        std::vector<bf_rt_id_t> pg_queues;
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          pg_queues.push_back(static_cast<bf_rt_id_t>(q % dft_q_per_port));
        }
        status = qData->setValue(data_id, pg_queues);
        if (status) {
          LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    field_name_s.str().c_str());
          return status;
        }
        status_get = BF_SUCCESS;
      }
    }
  }
  //-----
  {
    std::size_t pg_q_offset = 0;

    for (uint8_t p = 0; p < ports_per_pg; p++, pg_q_offset += dft_q_per_port) {
      field_name_s.str("");
      field_name_s << "egress_qid_queues_" << (static_cast<int>(p));
      status =
          this->dataFieldIdGet(field_name_s.str(), dft_action_id, &data_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s has no field %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name_s.str().c_str());
        return status;
      }
      if (qData->checkFieldActive(data_id)) {
        LOG_DBG("%s:%d %s %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name_s.str().c_str());
        std::vector<bf_rt_id_t> pg_queues;
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          pg_queues.push_back(
              static_cast<bf_rt_id_t>(pg_q_offset + q % dft_q_per_port));
        }
        status = qData->setValue(data_id, pg_queues);
        if (status) {
          LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    field_name_s.str().c_str());
          return status;
        }
        status_get = BF_SUCCESS;
      }
    }
  }
  return status_get;
}

bf_status_t BfRtTMPortGroupTable::tableClear(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  BfRtTMTableData dft_Data(this);

  auto status = tableDefaultEntryGet(session, dev_tgt, flags, &dft_Data);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get default entry to use it for table clear",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, nullptr, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  if (BF_DEV_PIPE_ALL == dev_wrk.pipe_id) {
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

  std::lock_guard<std::mutex> lock(this->entry_lock);

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d clear with default entry",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);

    for (bf_tm_pg_t pg_id = 0; pg_id < pg_per_pipe; pg_id++) {
      // TODO: The Queue mapping state lock should be set on for each pg_id.
      status = this->tableSetEntry(dev_wrk, dft_Data, pg_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Can't clear entry dev_id=%d pipe_id=%d pg_id=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_wrk.dev_id,
                  dev_wrk.pipe_id,
                  pg_id);
        break;
      }
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // single pipe clear
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

bf_status_t BfRtTMPortGroupTable::tableUsageGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  bf_status_t status = BF_SUCCESS;

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  uint8_t mau_pipes = 0;
  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, &mau_pipes, &pg_per_pipe, nullptr, nullptr);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint32_t count_ = (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) ? mau_pipes : 1;
  count_ *= pg_per_pipe;
  *count = count_;

  return status;
}

bf_status_t BfRtTMPortGroupTable::tableSizeGet(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               size_t *size) const {
  uint32_t size_ = 0;

  auto status = this->tableUsageGet(session, dev_tgt, flags, &size_);
  *size = size_;

  return status;
}

bf_status_t BfRtTMPortGroupTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPortGroupTableKey(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortGroupTable::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtTMPortGroupTableKey *>(key))->reset();
}

//---------------------------------
bf_status_t BfRtTMPortGroupTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  if (BF_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    return BF_OBJECT_NOT_FOUND;  // Some pipe_id is required
  }

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  BfRtTMPortGroupTableKey *qKey = static_cast<BfRtTMPortGroupTableKey *>(key);
  bf_tm_pg_t pg_id = 0;

  qKey->setId(pg_id);

  return this->tableEntryGet(session, dev_tgt, flags, *qKey, data);
}

bf_status_t BfRtTMPortGroupTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
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

  const BfRtTMPortGroupTableKey &qKey =
      static_cast<const BfRtTMPortGroupTableKey &>(key);
  bf_tm_pg_t pg_id = 0;

  status = qKey.getId(pg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, nullptr, nullptr);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
    return status;
  }

  if (pg_id >= pg_per_pipe) {
    LOG_ERROR(
        "%s:%d %s Invalid key "
        " dev_id=%d pipe_id=%d pg_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id);
    return BF_INVALID_ARG;
  }

  LOG_DBG("%s:%d %s cnt=%d dev_id=%d pipe_id=%d pg_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          cnt,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id);

  // The table lock is not needed over all entries' read, but each of the
  // entries does lock the table to read all its fields consistently.

  pg_id++;  // Continue from the next entry on the same pipe.

  uint32_t i = 0;

  for (; pg_id < pg_per_pipe && i < cnt; i++) {
    auto curr_key =
        static_cast<BfRtTMPortGroupTableKey *>((*key_data_pairs)[i].first);
    auto curr_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

    curr_key->setId(pg_id);

    status = this->tableEntryGet(session, dev_tgt, flags, *curr_key, curr_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Can't get next %d entry for "
          " dev_id=%d pipe_id=%d pg_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          i + 1,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id);
      break;
    }
    pg_id++;  // TODO: make TM iterator API for PortGroup
  }

  *num_returned = i;
  return status;
}

//------------------------
bf_status_t BfRtTMPortGroupTable::tableEntryGet(const BfRtSession & /*session*/,
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

  // Get fields from the key given.
  bf_tm_pg_t pg_id = 0;
  const BfRtTMPortGroupTableKey &qKey =
      static_cast<const BfRtTMPortGroupTableKey &>(key);

  bf_status_t status = qKey.getId(pg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  BfRtTMTableData *qData = static_cast<BfRtTMTableData *>(data);

  bf_status_t status_get = BF_INVALID_ARG;
  std::string field_name = "";
  bf_rt_id_t data_id = 0;

  bf_rt_id_t data_action_id = 0;

  status = qData->actionIdGet(&data_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (data_action_id == 0 && qData->getActiveFields().empty()) {
    /* The bfrt_python makes get() request without action_id.
     * Use action 'map' in this fixed function table
     * to unfold it with data fields.
     */
    status = actionIdGet("map", &data_action_id);
    if (status) {
      LOG_ERROR("%s:%d %s No 'map' action to use for unfold",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      BF_RT_DBGCHK(0);
      return status;
    }
    LOG_DBG("%s:%d %s pg_id=%d unfold with action_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            pg_id,
            data_action_id);

    qData->actionIdSet(data_action_id);

    std::vector<bf_rt_id_t> empty_fields;
    qData->setActiveFields(empty_fields);
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

  LOG_DBG("%s:%d %s pg_id=%d action_id=%d, active_fields=%zu with_values=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          pg_id,
          data_action_id,
          qData->getActiveFields().size(),
          qData->hasValues());

  std::lock_guard<std::mutex> lock(this->entry_lock);
  // The Queue mapping state lock is not required for Get on this table
  // if the entry_lock is already taken.

  //----------------------------
  uint8_t mau_pipes = 0;
  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;
  uint8_t ports_per_pg = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, &mau_pipes, &pg_per_pipe, &queues_per_pg, &ports_per_pg);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (pg_id >= pg_per_pipe || dev_tgt.pipe_id >= mau_pipes) {
    LOG_ERROR("%s:%d %s invalid key pg_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              dev_tgt.pipe_id);
    return BF_INVALID_ARG;
  }

  bf_dev_port_t pg_base_port = 0;
  status = trafficMgr->bfTMPipeGetPortGroupBasePort(
      dev_tgt.dev_id, dev_tgt.pipe_id, pg_id, &pg_base_port);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s invalid dev_id=%d pipe_id=%d pg_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              pg_id);
    return BF_INVALID_ARG;
  }

  //-----------
  {
    field_name.assign("pg_dev_ports");

    status = this->dataFieldIdGet(field_name, data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    if (qData->checkFieldActive(data_id)) {
      std::vector<bf_rt_id_t> pg_ports;

      bf_dev_port_t pg_port = pg_base_port;

      for (uint8_t p = 0; BF_SUCCESS == status && p < ports_per_pg; p++) {
        pg_ports.push_back(static_cast<bf_rt_id_t>(pg_port));
        status =
            trafficMgr->bfTMPipeGetPortNext(dev_tgt.dev_id, pg_port, &pg_port);
        if (BF_OBJECT_NOT_FOUND == status) {
          status = BF_SUCCESS;  // Last PG's port on a pipe.
          break;
        }
      }
      if (BF_SUCCESS == status) {
        LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d pg_id=%d field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                pg_id,
                data_id);
        status = qData->setValue(data_id, pg_ports);
      }
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name.c_str());
        return status;
      }
      status_get = BF_SUCCESS;
    }
  }
  //-----------
  bf_rt_id_t f_port_queue_count = 0;

  field_name.assign("port_queue_count");

  status =
      this->dataFieldIdGet(field_name, data_action_id, &f_port_queue_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_name.c_str());
    return status;
  }
  if (!(qData->checkFieldActive(f_port_queue_count))) {
    f_port_queue_count = 0;
  }

  /* Counters and Maps for each port in this PG. */
  size_t q_buf_len = ports_per_pg + queues_per_pg * ports_per_pg;

  std::unique_ptr<uint8_t[]> q_buffer(new uint8_t[q_buf_len]);

  uint8_t *q_buf = q_buffer.get();
  if (q_buf == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate buffer",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  uint8_t *q_map = &q_buf[ports_per_pg];

  std::memset(q_buf, 0, q_buf_len);

  // All remaining data fields need the same TM data retrieval.

  if (f_port_queue_count || action_name == "map") {
    std::stringstream debug_map;
    bf_dev_port_t pg_port = pg_base_port;

    for (uint8_t p = 0; BF_SUCCESS == status && p < ports_per_pg; p++) {
      status = trafficMgr->bfTMPortQMappingGet(
          dev_tgt.dev_id, pg_port, &q_buf[p], &q_map[p * queues_per_pg]);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Get queue mappings failed for port_id:%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  pg_port);
        return status;
      }

      if (BFRT_TM_DEBUG) {
        debug_map.str("");
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          debug_map << (static_cast<int>(q_map[p * queues_per_pg + q])) << " ";
        }
        LOG_DBG("%s:%d %s pg_id=%d pg_port=%d dev_port=%d queues=%d map=[%s]",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                pg_id,
                p,
                pg_port,
                q_buf[p],
                debug_map.str().c_str());
      }
      status =
          trafficMgr->bfTMPipeGetPortNext(dev_tgt.dev_id, pg_port, &pg_port);
      if (BF_OBJECT_NOT_FOUND == status) {
        status = BF_SUCCESS;  // Last PG's port on a pipe.
        break;
      }
    }
  }  // read mapping data

  if (f_port_queue_count) {
    std::vector<bf_rt_id_t> pg_counts;
    for (uint8_t p = 0; p < ports_per_pg; p++) {
      if (q_buf[p] > queues_per_pg) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d pg_id=%d port=%d invalid %d queues",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            p,
            q_buf[p]);
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
      }
      pg_counts.push_back(static_cast<bf_rt_id_t>(q_buf[p]));
    }
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d pg_id=%d set field_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            f_port_queue_count);
    status = qData->setValue(f_port_queue_count, pg_counts);
    if (status) {
      LOG_ERROR("%s:%d %s Can't set Data Object for pg_queue_count",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
    status_get = BF_SUCCESS;
  }

  // Action specific data fields

  if (action_name == "seq" || action_name == "even" || action_name == "crop") {
    // No more Data Fields for these actions.
    return status_get;
  }

  if (action_name != "map") {
    LOG_ERROR("%s:%d %s Invalid action_id=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id,
              action_name.c_str());
    BF_RT_DBGCHK(0);
    return BF_NOT_IMPLEMENTED;
  }

  // Action 'map' ------------------------
  std::stringstream field_name_s;

  for (uint8_t p = 0; p < ports_per_pg; p++) {
    field_name_s.str("");
    field_name_s << "ingress_qid_map_" << (static_cast<int>(p));
    status = this->dataFieldIdGet(field_name_s.str(), data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name_s.str().c_str());
      return status;
    }
    if (qData->checkFieldActive(data_id)) {
      std::vector<bf_rt_id_t> pg_queues;
      if (q_buf[p]) {
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          pg_queues.push_back(
              static_cast<bf_rt_id_t>(q_map[queues_per_pg * p + q]));
        }
      }
      LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d pg_id=%d set field_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              pg_id,
              data_id);
      status = qData->setValue(data_id, pg_queues);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name_s.str().c_str());
        return status;
      }
      status_get = BF_SUCCESS;
    }
  }

  //-------------
  std::size_t pg_q_offset = 0;

  for (uint8_t p = 0; p < ports_per_pg; p++) {
    field_name_s.str("");
    field_name_s << "egress_qid_queues_" << (static_cast<int>(p));
    status = this->dataFieldIdGet(field_name_s.str(), data_action_id, &data_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s has no field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name_s.str().c_str());
      return status;
    }
    if (qData->checkFieldActive(data_id)) {
      std::vector<bf_rt_id_t> pg_queues;
      if (q_buf[p]) {
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          pg_queues.push_back(static_cast<bf_rt_id_t>(
              pg_q_offset + q_map[queues_per_pg * p + q]));
        }
      }
      LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d pg_id=%d set field_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              pg_id,
              data_id);
      status = qData->setValue(data_id, pg_queues);
      if (status) {
        LOG_ERROR("%s:%d %s Can't set Data Object for %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_name_s.str().c_str());
        return status;
      }
      status_get = BF_SUCCESS;
    }
    pg_q_offset += q_buf[p];
    if (pg_q_offset > queues_per_pg) {
      LOG_ERROR(
          "%s:%d %s dev_id=%d pipe_id=%d pg_id=%d port=%d overflow %zu queues",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          pg_id,
          p,
          pg_q_offset);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
  }

  return status_get;
}

//----
bf_status_t BfRtTMPortGroupTable::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                                const BfRtTMTableData &qData,
                                                bf_tm_pg_t pg_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_rt_id_t data_action_id = 0;

  bf_status_t status = qData.actionIdGet(&data_action_id);
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

  LOG_DBG("%s:%d %s pg_id=%d action_id=%d, active_fields=%zu with_values=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          pg_id,
          data_action_id,
          qData.getActiveFields().size(),
          qData.hasValues());

  //----------------------------
  uint8_t mau_pipes = 0;
  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;
  uint8_t ports_per_pg = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, &mau_pipes, &pg_per_pipe, &queues_per_pg, &ports_per_pg);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (pg_id >= pg_per_pipe || dev_tgt.pipe_id >= mau_pipes) {
    LOG_ERROR("%s:%d %s invalid key pg_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              dev_tgt.pipe_id);
    return BF_INVALID_ARG;
  }

  bf_dev_port_t pg_base_port = 0;
  status = trafficMgr->bfTMPipeGetPortGroupBasePort(
      dev_tgt.dev_id, dev_tgt.pipe_id, pg_id, &pg_base_port);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s invalid dev_id=%d pipe_id=%d pg_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              pg_id);
    return BF_INVALID_ARG;
  }

  // Should we raise error on read-only fields in the mod() request ?

  std::string field_name = "";

  //-----------
  // All remaining data fields depend on the same TM functions' calls.
  //-----------
  std::vector<bf_rt_id_t> port_queue_count;
  bf_rt_id_t f_port_queue_count = 0;

  size_t q_map_siz = queues_per_pg * ports_per_pg;
  size_t q_buf_siz = ports_per_pg * 2 + q_map_siz;

  std::unique_ptr<uint8_t[]> q_buffer(new uint8_t[q_buf_siz]);

  uint8_t *q_buf = q_buffer.get();
  if (q_buf == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate buffer",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  std::memset(q_buf, 0, q_buf_siz);

  uint8_t *q_counts_old = q_buf;
  uint8_t *q_counts_new = &q_buf[ports_per_pg];
  uint8_t *q_map = &q_buf[ports_per_pg * 2];

  uint8_t ports_in_data = 0;  // How many ports to update.
  std::size_t q_cnt = 0;      // Cum.sum of allocated queues in the PG.
  uint8_t p = 0;              // Port index in the PG

  status = this->dataFieldIdGet(
      "port_queue_count", data_action_id, &f_port_queue_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s has no field port_queue_count",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return status;
  }
  if (qData.hasValue(f_port_queue_count)) {
    status = qData.getValue(f_port_queue_count, &port_queue_count);
    if (status) {
      LOG_ERROR("%s:%d %s Can't get data value for port_queue_count",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
    if (port_queue_count.size() != ports_per_pg) {
      LOG_ERROR(
          "%s:%d %s pg_id=%d port_queue_count given "
          "has %zu items instead of %d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          pg_id,
          port_queue_count.size(),
          ports_per_pg);
      return BF_INVALID_ARG;
    }
    /* Even if only some of the map.ingress_qid_map are requested for change,
     * we have to make sure that number of queues remains the same; if not,
     * then we have to remap affected ports' mapping in the port group using
     * their current map settings.
     */
    for (auto const &j : port_queue_count) {
      if (p >= ports_per_pg || j > queues_per_pg || q_cnt + j > queues_per_pg) {
        LOG_ERROR("%s:%d %s pg_id=%d port=%d set %d queues count %zu overflow",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  pg_id,
                  p,
                  j,
                  q_cnt);
        return BF_INVALID_ARG;
      }
      q_counts_new[p] = static_cast<uint8_t>(j);
      q_cnt += j;
      p++;
    }
    LOG_DBG("%s:%d %s pg_id=%d requested mapping for %zu of %d physical queues",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            pg_id,
            q_cnt,
            queues_per_pg);
  } else {
    f_port_queue_count = 0;  // Mark it off when not in the data.
  }

  /* For the port group mapping consistency we always have to read current
   * carvings at least to check the number of port queues is going to change.
   */
  bf_dev_port_t pg_port = pg_base_port;
  for (p = 0; BF_SUCCESS == status && p < ports_per_pg; p++) {
    status = trafficMgr->bfTMPortQMappingGet(
        dev_tgt.dev_id, pg_port, &q_counts_old[p], &q_map[p * queues_per_pg]);
    if (BF_SUCCESS != status) {
      break;
    }
    if (!f_port_queue_count) {
      if (q_counts_old[p] > queues_per_pg) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d pg_id=%d port=%d invalid %d queues",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            p,
            q_counts_old[p]);
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
      }
      // The new carving is omitted and remains the same.
      q_counts_new[p] = q_counts_old[p];
    }
    status = trafficMgr->bfTMPipeGetPortNext(dev_tgt.dev_id, pg_port, &pg_port);
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;  // Last PG's port on a pipe.
      break;
    }
  }
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Get queue mappings failed for port_id:%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_port);
    return status;
  }

  // Set the new mapping depending on the action requested.

  bool need_remap = false;
  q_cnt = 0;

  if (action_name == "seq") {
    /* Fill the port mapping the same way as default map does using the
     * given port_queue_count, e.g. for 8 queues:
     * (0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7)
     *
     * May implement selective action with bool fields for each port.
     */
    int map_idx = 0;
    for (p = 0; p < ports_per_pg; p++) {
      for (uint8_t q = 0; q < queues_per_pg; q++) {
        q_map[map_idx++] = (q_counts_new[p]) ? q % q_counts_new[p] : 0;
      }
      q_cnt += q_counts_new[p];
    }
    /* With this action even if the queue numbers remain the same we have
     * to update maps for all the ports in the group.
     */
    ports_in_data = p;

  } else if (action_name == "even") {
    /* Fill new map with equally sized chunks using port_queue_count,
     * e.g. for 8 queues - 8 chunks of 4:
     * (0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 6,6,6,6, 7,7,7,7)
     * for 16 queues - 16 chunks of 2
     * (0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15)
     * for 17 queues - 16 chunks of 1 and 0 fills the rest of the map:
     * (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0,0,0..0)
     * for 32 queues - 32 chunks of 1 - (0,1,2,...30,31)
     * for 1 queue - fill with 0
     * for 0 queues - fill with 0
     *
     * May implement selective action with bool fields for each port.
     */
    int map_idx = 0;
    for (p = 0; p < ports_per_pg; p++) {
      if (q_counts_new[p] == 0) {
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          q_map[map_idx++] = 0;
        }
      } else {
        uint8_t chunk_size = queues_per_pg / q_counts_new[p];
        for (uint8_t q = 0; q < queues_per_pg; q++) {
          q_map[map_idx++] = (chunk_size > 1) ? (q / chunk_size) : 0;
        }
        q_cnt += q_counts_new[p];
      }
    }
    /* With this action even if the queue numbers remain the same we have
     * to update maps for all the ports in the group.
     */
    ports_in_data = p;

  } else if (action_name == "crop") {
    /* Apply new port_queue_count to the current map replacing with 0 all
     * values greater or equal to the new count allowed, e.g.
     * 1) the default map and 10 queues - will not change eventually
     *    leaving two physical queues are not used;
     * 2) the default map and 5 queues becomes:
     *    (0,1,2,3,4,0,0,0, 0,1,2,3,4,0,0,0, 0,1,2,3,4,0,0,0, 0,1,2,3,4,0,0,0)
     *
     * port_queue_count is mandatory field for this action.
     */
    if (!f_port_queue_count) {
      LOG_ERROR("%s:%d %s port_queue_count is required for the 'crop' action",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return BF_INVALID_ARG;
    }

    int map_idx = 0;
    for (p = 0; p < ports_per_pg; p++) {
      if (q_counts_new[p] >= q_counts_old[p]) {
        LOG_DBG(
            "%s:%d %s action=%s pg_id=%d port=%d skip crop "
            "from %d to %d queues",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            action_name.c_str(),
            pg_id,
            p,
            q_counts_old[p],
            q_counts_new[p]);
        if (!need_remap) {
          // Mark as no mapping update is needed for this port.
          q_counts_new[p] = queues_per_pg + 1;
        }
        map_idx += queues_per_pg;
        continue;
      }
      // The subsequent ports will need remap.
      need_remap = true;

      LOG_DBG("%s:%d %s action=%s pg_id=%d port=%d crop from %d to %d queues",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str(),
              pg_id,
              p,
              q_counts_old[p],
              q_counts_new[p]);

      for (uint8_t q = 0; q < queues_per_pg; q++) {
        if (q_map[map_idx] >= q_counts_new[p]) {
          q_map[map_idx] = 0;
        }
        map_idx++;
      }
      q_cnt += q_counts_new[p];
      ports_in_data++;
    }  // for (ports)
    /* With this action we have to update maps for all the ports in the group.
     */
  } else if (action_name == "map") {
    std::stringstream qid_data_name("");
    bf_rt_id_t f_qid_data = 0;
    std::stringstream debug_map_old;
    std::stringstream debug_map_new;
    bool f_dbg_map = BFRT_TM_DEBUG;
    ports_in_data = 0;

    for (p = 0; p < ports_per_pg; p++) {
      /* Get and validate requested mappings. */
      qid_data_name.str("");
      qid_data_name << "ingress_qid_map_" << (static_cast<int>(p));
      status = this->dataFieldIdGet(
          qid_data_name.str(), data_action_id, &f_qid_data);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s has no field %s",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  qid_data_name.str().c_str());
        return status;
      }
      if (qData.hasValue(f_qid_data)) {
        // Take new ingress_qid_map for the port from the request data.
        std::vector<bf_rt_id_t> ingress_qid_map;
        status = qData.getValue(f_qid_data, &ingress_qid_map);
        if (status) {
          LOG_ERROR("%s:%d %s Can't get %s value from the data object",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    qid_data_name.str().c_str());
          return status;
        }
        if (ingress_qid_map.size() != queues_per_pg) {
          LOG_ERROR(
              "%s:%d %s action=%s pg_id=%d port=%d "
              "has incorrect %zu of %d QID map items",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str(),
              pg_id,
              p,
              ingress_qid_map.size(),
              queues_per_pg);
          return BF_INVALID_ARG;
        }

        debug_map_old.str("");
        debug_map_new.str("");
        int map_idx = (static_cast<int>(p)) * queues_per_pg;
        uint8_t q = 0;

        for (auto const &j : ingress_qid_map) {
          if (q >= queues_per_pg || (j != 0 && j >= q_counts_new[p])) {
            LOG_ERROR(
                "%s:%d %s action=%s pg_id=%d port=%d "
                "incorrect queue number %d qiven with %d queues",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                action_name.c_str(),
                pg_id,
                p,
                j,
                q_counts_new[p]);
            return BF_INVALID_ARG;
          }

          if (f_dbg_map) {
            debug_map_old << (static_cast<int>(q_map[map_idx + q])) << " ";
          }

          q_map[map_idx + q] = static_cast<uint8_t>(j);

          if (f_dbg_map) {
            debug_map_new << (static_cast<int>(q_map[map_idx + q])) << " ";
          }

          q++;
        }  // for each queue in the map

        LOG_DBG("%s:%d %s action=%s pg_id=%d port=%d OLD queues=%d map=[%s]",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                action_name.c_str(),
                pg_id,
                p,
                q_counts_old[p],
                debug_map_old.str().c_str());
        LOG_DBG("%s:%d %s action=%s pg_id=%d port=%d NEW queues=%d map=[%s]",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                action_name.c_str(),
                pg_id,
                p,
                q_counts_new[p],
                debug_map_new.str().c_str());

        q_cnt += q_counts_new[p];
        ports_in_data++;
        if (q_counts_new[p] != q_counts_old[p]) {
          // The subsequent ports must be re-mapped either with new or old map.
          need_remap = true;
        }
      } else {
        // If the port has no new mapping value its queue count must not change.
        if (q_counts_old[p] != q_counts_new[p]) {
          LOG_ERROR(
              "%s:%d %s action=%s pg_id=%d port=%d new mapping data "
              "required to re-map %d into %d queues",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str(),
              pg_id,
              p,
              q_counts_new[p],
              q_counts_old[p]);
          return BF_INVALID_ARG;
        }
        if (!need_remap) {
          LOG_DBG(
              "%s:%d %s action=%s pg_id=%d port=%d skip change of %d queues",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str(),
              pg_id,
              p,
              q_counts_new[p]);
          // Mark as no mapping update is needed for this port.
          q_counts_new[p] = queues_per_pg + 1;
        } else {
          // Some fore port in the port group change number of queues, so this
          // port will be also remapped implicitly using the current map which
          // is already read into the q_map[]
          LOG_DBG("%s:%d %s action=%s pg_id=%d port=%d re-map same %d queues",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  action_name.c_str(),
                  pg_id,
                  p,
                  q_counts_new[p]);
          q_cnt += q_counts_new[p];
        }
      }
    }  // for each port in the group
    // End for 'map' action.
    if (ports_in_data == 0) {
      LOG_ERROR(
          "%s:%d %s action=%s pg_id=%d at least one of mandatory "
          "ingress_qid_map values is required",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_name.c_str(),
          pg_id);
      return BF_INVALID_ARG;
    }
  } else {
    LOG_ERROR("%s:%d %s Invalid action_id=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_action_id,
              action_name.c_str());
    return BF_NOT_IMPLEMENTED;
  }

  if (ports_in_data == 0) {
    /* For all actions it is ok to do nothing, except the 'map'.
     * This behavior might be changed e.g. for selective 'crop'.
     */
    LOG_DBG("%s:%d %s action=%s pg_id=%d nothing is changed",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            action_name.c_str(),
            pg_id);
    return BF_SUCCESS;
  }

  LOG_DBG(
      "%s:%d %s action=%s pg_id=%d for %d of %d ports "
      "and %zu of %d physical queues",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      action_name.c_str(),
      pg_id,
      ports_in_data,
      ports_per_pg,
      q_cnt,
      queues_per_pg);

  // Execute the TM mapping function for all required mappings.
  pg_port = pg_base_port;

  for (p = 0; BF_SUCCESS == status && p < ports_per_pg; p++) {
    if (q_counts_new[p] > queues_per_pg) {
      continue;  // No need to change this port.
    }
    LOG_DBG(
        "%s:%d %s action=%s pg_id=%d port=%d set %d queue mappings "
        "for port_id:%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        action_name.c_str(),
        pg_id,
        p,
        q_counts_new[p],
        pg_port);

    status = trafficMgr->bfTMPortQMappingSet(
        dev_tgt.dev_id, pg_port, q_counts_new[p], &q_map[p * queues_per_pg]);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s action=%s pg_id=%d pg_port=%d dev_port=%d set %d "
          "queue mappings failed",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_name.c_str(),
          pg_id,
          p,
          pg_port,
          q_counts_new[p]);
      return status;
    }
    status = trafficMgr->bfTMPipeGetPortNext(dev_tgt.dev_id, pg_port, &pg_port);
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;  // Last PG's port on a pipe.
      break;
    }
  }  // for each port apply its new queue map

  return status;
}  // tableSetEntry()

bf_status_t BfRtTMPortGroupTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const BfRtTMTableData &pgData = static_cast<const BfRtTMTableData &>(data);

  if (pgData.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const BfRtTMPortGroupTableKey &pgKey =
      static_cast<const BfRtTMPortGroupTableKey &>(key);
  bf_tm_pg_t pg_id = 0;

  bf_status_t status = pgKey.getId(pg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  std::lock_guard<std::mutex> lock(this->entry_lock);

  // TODO: The Queue mapping state lock should be set on this pg_id
  // and released before the entry_lock.

  return this->tableSetEntry(dev_tgt, pgData, pg_id);
}

}  // namespace bfrt
