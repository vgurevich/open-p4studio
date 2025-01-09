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


#include <bf_rt_common/bf_rt_init_impl.hpp>

#include "bf_rt_tm_table_helper_sched.hpp"
#include "bf_rt_tm_table_impl_l1_node.hpp"

namespace bfrt {

bf_status_t BfRtTML1NodeTableIntf::tmL1NodeCountGet(
    const bf_rt_target_t &dev_tgt,
    uint16_t *l1_per_pipe,
    uint8_t *l1_per_pg) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_tm_dev_cfg_t tm_cfg = {0};

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMDevCfgGet(dev_tgt.dev_id, &tm_cfg);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    return status;
  }

  if (NULL != l1_per_pipe) {
    *l1_per_pipe = tm_cfg.l1_per_pipe;
  }
  if (NULL != l1_per_pg) {
    *l1_per_pg = tm_cfg.l1_per_pg;
  }
  return status;
}

bf_status_t BfRtTML1NodeTableIntf::tableUsageGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_tm_dev_cfg_t tm_cfg = {0};

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMDevCfgGet(dev_tgt.dev_id, &tm_cfg);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    return status;
  }

  uint32_t count_ = tm_cfg.l1_per_pipe;

  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    count_ *= tm_cfg.pipe_cnt;
  } else {
    status = trafficMgr->bfTMPipeIsValid(dev_tgt.dev_id, dev_tgt.pipe_id);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d Invalid dev_id=%d pipe_id=%d",
                __func__,
                __LINE__,
                dev_tgt.dev_id,
                dev_tgt.pipe_id);
      return status;
    }
  }
  *count = count_;

  return status;
}

bf_status_t BfRtTML1NodeTableIntf::tableSizeGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                size_t *size) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  uint32_t usage_ = 0;

  auto status = this->tableUsageGet(session, dev_tgt, flags, &usage_);
  *size = usage_;

  return status;
}

bf_status_t BfRtTML1NodeTableIntf::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTML1NodeTableKey(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableIntf::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtTML1NodeTableKey *>(key))->reset();
}

// Resets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTML1NodeTableIntf::tableResetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    BfRtTMTableData * /* wrk_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableIntf::tableResetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTML1NodeTableIntf::tableResetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (wrk_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual reset methods will set 'default' values
  // and remove processed ids from there.
  std::set<bf_rt_id_t> activeDataFields(wrk_data->getActiveFields());

  // Get block of related fields at once if this mode is defined for the table.
  // In the latter case the list of field ids should shrink.
  bf_status_t status = this->tableResetFields(
      dev_tgt, pg_id, pg_l1_node, wrk_data, activeDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields pg_id=%d pg_l1_node=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_l1_node);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_l1_node=%d need_fields=%zu, already "
      "with_values=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_l1_node,
      activeDataFields.size(),
      wrk_data->hasValues());

  for (bf_rt_id_t data_id : activeDataFields) {
    bool f_read_only = false;
    status = this->dataFieldReadOnlyGet(data_id, &f_read_only);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get r/o attribute of field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id);
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
    status =
        this->tableResetField(dev_tgt, data_id, pg_id, pg_l1_node, wrk_data);
    if (status) {
      break;
    }
  }  // for (field_id)

  return status;
}

//----
bf_status_t BfRtTML1NodeTableIntf::tableReset(
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

  uint8_t pg_per_pipe = 0;

  status = this->tmDevCfgGet(
      dev_wrk.dev_id, nullptr, &pg_per_pipe, nullptr, nullptr);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t l1_per_pg = 0;

  status = this->tmL1NodeCountGet(dev_wrk, nullptr, &l1_per_pg);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM L1 configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  BfRtTMTableData reset_data(this);

  std::lock_guard<std::mutex> lock(this->entry_lock);

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d reset entries",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);

    for (bf_tm_pg_t pg_id = 0; pg_id < pg_per_pipe && BF_SUCCESS == status;
         pg_id++) {
      for (uint16_t pg_l1_node = 0; pg_l1_node < l1_per_pg; pg_l1_node++) {
        status = this->tableResetEntry(dev_wrk, pg_id, pg_l1_node, &reset_data);
        if (status == BF_EAGAIN) {
          LOG_TRACE(
              "%s:%d %s Skip reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_l1_node=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_l1_node);
          status = BF_SUCCESS;
          continue;
        }
        if (BF_SUCCESS != status) {
          LOG_ERROR(
              "%s:%d %s Can't prepare reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_l1_node=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_l1_node);
          break;
        }
        status = this->tableSetEntry(dev_wrk, reset_data, pg_id, pg_l1_node);
        if (BF_SUCCESS != status) {
          LOG_ERROR(
              "%s:%d %s Can't reset entry dev_id=%d pipe_id=%d"
              " pg_id=%d pg_l1_node=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              pg_id,
              pg_l1_node);
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

// Sets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTML1NodeTableIntf::tableSetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    const BfRtTMTableData & /* wrk_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableIntf::tableSetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    const BfRtTMTableData & /* wrk_data */,
    bf_rt_id_t data_id) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTML1NodeTableIntf::tableSetEntry(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTableData &wrk_data,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> assignedDataFields(wrk_data.getAssignedFields());

  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.
  bf_status_t status = this->tableSetFields(
      dev_tgt, pg_id, pg_l1_node, wrk_data, assignedDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields pg_id=%d pg_l1_node=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_l1_node);
    return status;
  }

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_l1_node=%d active_fields=%zu with_values=%zu "
      "to_assign=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_l1_node,
      wrk_data.getActiveFields().size(),
      wrk_data.hasValues(),
      assignedDataFields.size());

  for (bf_rt_id_t data_id : assignedDataFields) {
    bool f_read_only = false;
    status = this->dataFieldReadOnlyGet(data_id, &f_read_only);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s can't get r/o attribute of field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id);
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
    status = this->tableSetField(dev_tgt, pg_id, pg_l1_node, wrk_data, data_id);
    if (status) {
      break;
    }
  }  // for (field_id)

  return status;
}

bf_status_t BfRtTML1NodeTableIntf::tableGetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    BfRtTMTableData * /* wrk_data */) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTML1NodeTableIntf::tableGetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_pg_t /* pg_id */,
    bf_tm_l1_node_t /* pg_l1_node */,
    BfRtTMTableData * /* wrk_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTML1NodeTableIntf::tableGetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (wrk_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(wrk_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  LOG_DBG(
      "%s:%d %s pg_id=%d pg_l1_node=%d got %zu of %zu values in %zu fields.",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_l1_node,
      wrk_data->hasValues(),
      active_fields_cnt,
      workDataFields.size());

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  bf_status_t status = this->tableGetFields(
      dev_tgt, pg_id, pg_l1_node, wrk_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_l1_node=%d got %zu of %zu values; need %zu fields.",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_l1_node,
      wrk_data->hasValues(),
      active_fields_cnt,
      workDataFields.size());

  for (bf_rt_id_t data_id : workDataFields) {
    status = this->tableGetField(dev_tgt, data_id, pg_id, pg_l1_node, wrk_data);
    if (status) {
      break;
    }
  }
  LOG_DBG(
      "%s:%d %s pg_id=%d pg_l1_node=%d got %zu of %zu values; left %zu fields.",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      pg_id,
      pg_l1_node,
      wrk_data->hasValues(),
      active_fields_cnt,
      workDataFields.size());

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_l1_node=%d rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_l1_node,
              status,
              bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTML1NodeTableIntf::tableEntryGet(
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

  BfRtTMTableData *wrk_data = static_cast<BfRtTMTableData *>(data);

  // Table with actions should proceed with get using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && wrk_data->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s No active data fields to get",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    return BF_SUCCESS;
  }

  const BfRtTML1NodeTableKey &pKey =
      static_cast<const BfRtTML1NodeTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  bf_tm_l1_node_t pg_l1_node = 0;

  bf_status_t status = pKey.getId(pg_id, pg_l1_node);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TODO: Some of the L1 Node tables, have Queue mapping fields,
  // and the queue_map state lock should be acquired at this point to keep
  // the table entry's fields consistent.
  // If the Queue mapping state lock should be acquired before the Table lock
  // then do it at the overloaded tableEntryGet() wrapper.
  if (this->isVolatile()) {
    status = this->tableGetEntry(dev_tgt, pg_id, pg_l1_node, wrk_data);
  } else {
    LOG_DBG("%s:%d %s Get entry lock",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    std::lock_guard<std::mutex> lock(this->entry_lock);
    LOG_DBG("%s:%d %s Got entry lock",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    status = this->tableGetEntry(dev_tgt, pg_id, pg_l1_node, wrk_data);
  }

  return status;
}

bf_status_t BfRtTML1NodeTableIntf::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const BfRtTMTableData &wrk_data = static_cast<const BfRtTMTableData &>(data);

  // Table with actions should proceed with mod using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && wrk_data.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const BfRtTML1NodeTableKey &pKey =
      static_cast<const BfRtTML1NodeTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  bf_tm_l1_node_t pg_l1_node = 0;

  bf_status_t status = pKey.getId(pg_id, pg_l1_node);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TODO: Some of the L1 Node tables, have Queue mapping fields,
  // and the queue_map state lock should be acquired at this point to keep
  // the table entry's fields consistent.
  // If the Queue mapping state lock should be acquired before the Table lock
  // then do it at the overloaded tableEntryMod() wrapper.

  LOG_DBG("%s:%d %s Get entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> lock(this->entry_lock);
  LOG_DBG("%s:%d %s Got entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  return this->tableSetEntry(dev_tgt, wrk_data, pg_id, pg_l1_node);
}

bf_status_t BfRtTML1NodeTableIntf::tableEntryGetFirst(
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

  BfRtTML1NodeTableKey *pKey = static_cast<BfRtTML1NodeTableKey *>(key);
  bf_tm_pg_t pg_id = 0;
  bf_tm_l1_node_t pg_l1_node = 0;

  pKey->setId(pg_id, pg_l1_node);

  return this->tableEntryGet(session, dev_tgt, flags, *pKey, data);
}

bf_status_t BfRtTML1NodeTableIntf::tableEntryGetNext_n(
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

  const BfRtTML1NodeTableKey &pKey =
      static_cast<const BfRtTML1NodeTableKey &>(key);
  bf_tm_pg_t pg_id = 0;
  bf_tm_l1_node_t pg_l1_node = 0;

  status = pKey.getId(pg_id, pg_l1_node);
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
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint8_t l1_per_pg = 0;

  status = this->tmL1NodeCountGet(dev_tgt, nullptr, &l1_per_pg);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM L1 configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (pg_id >= pg_per_pipe || pg_l1_node >= l1_per_pg) {
    LOG_ERROR(
        "%s:%d %s Invalid key "
        " dev_id=%d pipe_id=%d pg_id=%d, pg_l1_node=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node);
    return BF_INVALID_ARG;
  }

  LOG_DBG(
      "%s:%d %s get cnt=%d dev_id=%d pipe_id=%d after pg_id=%d, pg_l1_node=%d",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      cnt,
      dev_tgt.dev_id,
      dev_tgt.pipe_id,
      pg_id,
      pg_l1_node);

  // Continue from the next entry on the same pipe.
  if (++pg_l1_node >= l1_per_pg) {
    pg_l1_node = 0;
    pg_id++;
  }

  // The table lock is not needed over all entries' read, but each of the
  // entries does lock the table to read all its fields consistently.
  uint32_t i = 0;

  for (; pg_id < pg_per_pipe && i < cnt && BF_SUCCESS == status; pg_id++) {
    for (; pg_l1_node < l1_per_pg && i < cnt && BF_SUCCESS == status;
         i++, pg_l1_node++) {
      auto curr_key =
          static_cast<BfRtTML1NodeTableKey *>((*key_data_pairs)[i].first);
      auto curr_data =
          static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

      curr_key->setId(pg_id, pg_l1_node);

      if (this->isVolatile()) {
        status = this->tableGetEntry(dev_tgt, pg_id, pg_l1_node, curr_data);
      } else {
        std::lock_guard<std::mutex> lock(this->entry_lock);
        status = this->tableGetEntry(dev_tgt, pg_id, pg_l1_node, curr_data);
      }
      // BF_OBJECT_NOT_FOUND is strange for the fixed TM table
      if (BF_OBJECT_NOT_FOUND == status) {
        BF_RT_DBGCHK(0);
        status = BF_UNEXPECTED;
      }
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s Can't get next entry for "
            " dev_id=%d pipe_id=%d pg_id=%d, pg_l1_node=%d, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            pg_id,
            pg_l1_node,
            status,
            bf_err_str(status));
      }
    }  // for (pg_l1_node)

    pg_l1_node = 0;
  }  // for (pg_id)

  *num_returned = i;
  return status;
}

//-----
bf_status_t BfRtTML1NodeTableIntf::tableClear(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->tableReset(session, dev_tgt, flags);
}

//----- TM_L1_NODE_SCHED_CFG ---------------------------

bf_status_t BfRtTML1NodeSchedCfgTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    const BfRtTMTableData &wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // wrk_data and wrk_fields might be empty, but with some action_id to apply.

  bf_rt_id_t do_action_id = 0;
  auto status = wrk_data.actionIdGet(&do_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  //--- Get L1 Node to Port current assignment status.

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  status = trafficMgr->bfTML1NodePortAssignmentGet(dev_tgt.dev_id,
                                                   dev_tgt.pipe_id,
                                                   pg_id,
                                                   pg_l1_node,
                                                   &in_use,
                                                   &pg_port_nr,
                                                   &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d port assignment rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  //--- Extract data entry into the helper.

  BfRtTML1NodeSchedCfgHelper s_hlp;

  status = s_hlp.initActions(*this);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init actions pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  bf_rt_id_t current_action_id = (in_use) ? s_hlp.a_in_use : s_hlp.a_free;

  if (do_action_id == 0) {
    // in case the data object has no action_id assigned.
    LOG_DBG(
        "%s:%d %s assume current action_id=%d on '%s' L1 Node "
        "pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        current_action_id,
        (in_use) ? "in_use" : "free",
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    do_action_id = current_action_id;
  }

  status = s_hlp.initFields(do_action_id, *this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  status = s_hlp.getFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  //--- Pre-process L1 Node assingment transitions.

  bool x_free_2_use = false;  // free-->in_use transition is requested
  bool x_use_2_free = false;  // use-->free transition is requested

  if (!(in_use) && do_action_id == s_hlp.a_free) {
    // free-->free
    LOG_DBG(
        "%s:%d %s change free L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    // continue to change fields' values except scheduling_enable and port.

  } else if (in_use && do_action_id == s_hlp.a_free) {
    // in_use-->free
    LOG_DBG(
        "%s:%d %s free L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    // release the node and continue to change fields' values if also given,
    // except scheduling_enable and port.
    x_use_2_free = true;

  } else if (in_use && do_action_id == s_hlp.a_in_use) {
    // in_use-->in_use
    if (!(s_hlp.f_dev_port)) {
      s_hlp.dev_port = port_id;  // use the current port.
    }
    if (port_id == s_hlp.dev_port) {
      LOG_DBG(
          "%s:%d %s update L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d "
          "dev_port=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.pipe_id,
          pg_id,
          pg_l1_node,
          port_id);
      // continue to change fields' values with in_use state.
    } else {
      LOG_DBG(
          "%s:%d %s move L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d "
          "from dev_port=%d to dev_port=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.pipe_id,
          pg_id,
          pg_l1_node,
          port_id,
          s_hlp.dev_port);

      // check the port move is possible
      bf_tm_pg_t pg_id_ = 0;
      uint8_t pg_port_nr_ = 0;

      status = this->splitDevPort(dev_tgt, s_hlp.dev_port, pg_id_, pg_port_nr_);
      // it also validates the new port is on the same pipe.
      if (BF_SUCCESS == status) {
        if (pg_id == pg_id_) {
          // Ok, the new port is in the same port group.
          // Move will be in two steps: free and assign to the new port.
          x_use_2_free = true;
          x_free_2_use = true;
        } else {
          status = BF_INVALID_ARG;
          LOG_ERROR(
              "%s:%d %s can't move L1 Node to different port group "
              "from pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d "
              "to dev_port=%d (pg_id=%d)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.pipe_id,
              pg_id,
              pg_l1_node,
              port_id,
              s_hlp.dev_port,
              pg_id_);
        }
      }
    }
  } else if (!(in_use) && do_action_id == s_hlp.a_in_use) {
    // free-->in_use
    LOG_DBG(
        "%s:%d %s assign L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d "
        "to dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    // change fields' values and then assign the node to the port.
    x_free_2_use = true;
  } else {
    BF_RT_DBGCHK(0);
    status = BF_UNEXPECTED;

  }  // L1 Node assgnment transition.

  if (BF_SUCCESS == status && x_use_2_free) {
    status = trafficMgr->bfTML1NodeFree(dev_tgt.dev_id, port_id, pg_l1_node);
    if (BF_SUCCESS == status) {
      in_use = false;
    }
  }
  // Common error logging for L1 Node assignment transitions.
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s failed action_id=%d on '%s' L1 Node "
        "pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        do_action_id,
        (in_use) ? "in_use" : "free",
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  //--- Change data fields.

  bf_rt_id_t err_data_id = 0;

  do {
    // Set fields in order starting from value assignments first,
    // and then do enable/disable corresponding controls.
    // The 'disable and change' sequence should be in two separate
    // set entry request, if needed.

    if (s_hlp.f_priority_propagation) {
      status = (s_hlp.priority_propagation)
                   ? trafficMgr->bfTML1NodeSchedPriorityPropagationEnable(
                         dev_tgt.dev_id, port_id, pg_l1_node)
                   : trafficMgr->bfTML1NodeSchedPriorityPropagationDisable(
                         dev_tgt.dev_id, port_id, pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_priority_propagation;
        break;
      }
    }
    if (s_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTML1NodeSchedDwrrWeightSet(
          dev_tgt.dev_id, port_id, pg_l1_node, s_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Guaranteed bandwidth: set priority and then enable it.
    if (s_hlp.f_min_priority) {
      status = trafficMgr->bfTML1NodeSchedMinRatePrioritySet(
          dev_tgt.dev_id, port_id, pg_l1_node, s_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_min_priority;
        break;
      }
    }
    if (s_hlp.f_min_rate_enable) {
      status = (s_hlp.min_rate_enable)
                   ? trafficMgr->bfTML1NodeSchedMinRateEnable(
                         dev_tgt.dev_id, port_id, pg_l1_node)
                   : trafficMgr->bfTML1NodeSchedMinRateDisable(
                         dev_tgt.dev_id, port_id, pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_min_rate_enable;
        break;
      }
    }

    //-- Shaping bandwidth: set priority and then enable it.
    if (s_hlp.f_max_priority) {
      status = trafficMgr->bfTML1NodeSchedMaxRatePrioritySet(
          dev_tgt.dev_id, port_id, pg_l1_node, s_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_max_priority;
        break;
      }
    }
    if (s_hlp.f_max_rate_enable) {
      status = (s_hlp.max_rate_enable)
                   ? trafficMgr->bfTML1NodeSchedMaxRateEnable(
                         dev_tgt.dev_id, port_id, pg_l1_node)
                   : trafficMgr->bfTML1NodeSchedMaxRateDisable(
                         dev_tgt.dev_id, port_id, pg_l1_node);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_max_rate_enable;
        break;
      }
    }

    //-- Child Queues group of fields is read only.

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s write field_id=%d failed pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d"
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  //--- Post-process L1 Node: Port assignment and enable.

  if (x_free_2_use) {
    // free-->in_use transition.
    // Might be on a differnt port.
    if (!(s_hlp.f_dev_port)) {
      s_hlp.dev_port = port_id;  // set in_use on the currently assigned port.
    }

    // When the L1 Node is 'free' it must be with scheduling_enable == false.
    // TM driver implicitly sets scheduling enabled on free --> in_use.
    // The sheduling_enable = False might be given explicitly and it will be
    // applied at the next post-process block.
    status = trafficMgr->bfTML1NodeSchedEnable(
        dev_tgt.dev_id, s_hlp.dev_port, pg_l1_node);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s enable failed pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d"
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.pipe_id,
          pg_id,
          pg_l1_node,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }

    in_use = true;  // free-->in_use is ok.
  }

  if (in_use && s_hlp.f_scheduling_enable) {
    // in_use-->in_use or free-->in_use transition with scheduling update.
    status = (s_hlp.scheduling_enable)
                 ? trafficMgr->bfTML1NodeSchedEnable(
                       dev_tgt.dev_id, s_hlp.dev_port, pg_l1_node)
                 : trafficMgr->bfTML1NodeSchedDisable(
                       dev_tgt.dev_id, s_hlp.dev_port, pg_l1_node);

    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s change scheduling to %d failed pipe_id=%d pg_id=%d "
          "pg_l1_node=%d "
          "dev_port=%d"
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          s_hlp.scheduling_enable,
          dev_tgt.pipe_id,
          pg_id,
          pg_l1_node,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
  }

  return status;
}

//-----
bf_status_t BfRtTML1NodeSchedCfgTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (wrk_data == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_rt_id_t do_action_id = 0;
  auto status = wrk_data->actionIdGet(&do_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  status = trafficMgr->bfTML1NodePortAssignmentDefaultGet(dev_tgt.dev_id,
                                                          dev_tgt.pipe_id,
                                                          pg_id,
                                                          pg_l1_node,
                                                          &in_use,
                                                          &pg_port_nr,
                                                          &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d default port assignment "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  BfRtTML1NodeSchedCfgHelper s_hlp;

  status = s_hlp.initActions(*this);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init actions pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  bf_rt_id_t default_action_id = (in_use) ? s_hlp.a_in_use : s_hlp.a_free;

  if (do_action_id != default_action_id) {
    do_action_id = default_action_id;
    wrk_data->reset(do_action_id);
    wrk_fields = wrk_data->getActiveFields();
  }

  status = s_hlp.initFields(do_action_id, *this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  //--- Get default data fields from TM.

  bf_rt_id_t err_data_id = 0;

  do {
    if (s_hlp.f_dev_port) {
      s_hlp.dev_port = port_id;
    }

    if (s_hlp.f_scheduling_enable) {
      s_hlp.scheduling_enable = in_use;
    }

    if (s_hlp.f_priority_propagation) {
      status = trafficMgr->bfTML1NodeSchedPriorityPropagationDefaultGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.priority_propagation);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_priority_propagation;
        break;
      }
    }

    //-- Guaranteed bandwidth.
    if (s_hlp.f_min_priority || s_hlp.f_min_rate_enable) {
      status = trafficMgr->bfTML1NodeSchedMinRateEnableDefaultsGet(
          dev_tgt.dev_id,
          port_id,
          pg_l1_node,
          &s_hlp.min_rate_enable,
          &s_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = (s_hlp.f_min_rate_enable) ? s_hlp.f_min_rate_enable
                                                : s_hlp.f_min_priority;
        break;
      }
    }

    if (s_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTML1NodeSchedDwrrWeightDefaultGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Shaping bandwidth and pirority.
    if (s_hlp.f_max_priority || s_hlp.f_max_rate_enable) {
      status = trafficMgr->bfTML1NodeSchedMaxRateEnableDefaultsGet(
          dev_tgt.dev_id,
          port_id,
          pg_l1_node,
          &s_hlp.max_rate_enable,
          &s_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = (s_hlp.f_max_rate_enable) ? s_hlp.f_max_rate_enable
                                                : s_hlp.f_max_priority;
        break;
      }
    }

    //-- Child Queues group of fields is R/O and does not reset from L1 Node.

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s read defaults for field_id=%d failed pipe_id=%d pg_id=%d "
        "pg_l1_node=%d "
        "dev_port=%d rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't set default fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  return status;
}

bf_status_t BfRtTML1NodeSchedCfgTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == wrk_data) {
    return BF_INVALID_ARG;
  }
  // wrk_data and wrk_fields might be empty, but with action_id to get.

  bf_rt_id_t do_action_id = 0;
  auto status = wrk_data->actionIdGet(&do_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  //--- Get L1 Node to Port current assignment status.

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  status = trafficMgr->bfTML1NodePortAssignmentGet(dev_tgt.dev_id,
                                                   dev_tgt.pipe_id,
                                                   pg_id,
                                                   pg_l1_node,
                                                   &in_use,
                                                   &pg_port_nr,
                                                   &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d port assignment rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  //---
  BfRtTML1NodeSchedCfgHelper s_hlp;

  status = s_hlp.initActions(*this);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init actions pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  bf_rt_id_t current_action_id = (in_use) ? s_hlp.a_in_use : s_hlp.a_free;

  if (do_action_id == 0) {
    // in case the data object has no action_id assigned to get from HW.
    LOG_DBG(
        "%s:%d %s imply action_id=%d on '%s' L1 Node "
        "pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        current_action_id,
        (in_use) ? "in_use" : "free",
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    do_action_id = current_action_id;
    wrk_data->reset(do_action_id);
    wrk_fields = wrk_data->getActiveFields();

  } else if (do_action_id != current_action_id) {
    LOG_ERROR(
        "%s:%d %s get action_id=%d not match current action_id=%d "
        "on '%s' L1 Node pipe_id=%d pg_id=%d pg_l1_node=%d dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        do_action_id,
        current_action_id,
        (in_use) ? "in_use" : "free",
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return BF_INVALID_ARG;
  }

  //--- Extract data entry into the helper.

  status = s_hlp.initFields(do_action_id, *this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  //--- Get data fields from HW.

  bf_rt_id_t err_data_id = 0;

  do {
    if (s_hlp.f_priority_propagation) {
      status = trafficMgr->bfTML1NodeSchedPriorityPropagationGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.priority_propagation);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_priority_propagation;
        break;
      }
    }
    if (s_hlp.f_dwrr_weight) {
      status = trafficMgr->bfTML1NodeSchedDwrrWeightGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.dwrr_weight);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_dwrr_weight;
        break;
      }
    }

    //-- Guaranteed bandwidth.
    if (s_hlp.f_min_priority) {
      status = trafficMgr->bfTML1NodeSchedMinRatePriorityGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.min_priority);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_min_priority;
        break;
      }
    }
    if (s_hlp.f_min_rate_enable) {
      status = trafficMgr->bfTML1NodeSchedMinRateEnableGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.min_rate_enable);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_min_rate_enable;
        break;
      }
    }

    //-- Shaping bandwidth: set priority and then enable it.
    if (s_hlp.f_max_priority) {
      status = trafficMgr->bfTML1NodeSchedMaxRatePriorityGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.max_priority);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_max_priority;
        break;
      }
    }
    if (s_hlp.f_max_rate_enable) {
      status = trafficMgr->bfTML1NodeSchedMaxRateEnableGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.max_rate_enable);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_max_rate_enable;
        break;
      }
    }

    //--- Post-process L1 Node: Port assignment and enable.
    if (s_hlp.f_dev_port) {
      s_hlp.dev_port = port_id;
    }

    if (s_hlp.f_scheduling_enable) {
      status = trafficMgr->bfTML1NodeSchedEnableGet(
          dev_tgt.dev_id, port_id, pg_l1_node, &s_hlp.scheduling_enable);
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_scheduling_enable;
        break;
      }
    }

    //-- Child Queues group of fields.

    if (s_hlp.f_pg_queues || s_hlp.f_queues_count) {
      uint8_t queues_per_pg = 0;

      status = this->tmDevCfgGet(
          dev_tgt.dev_id, nullptr, nullptr, &queues_per_pg, nullptr);
      if (status) {
        LOG_ERROR("%s:%d Can't get TM configuration", __func__, __LINE__);
        return status;
      }

      std::vector<bf_tm_queue_t> l1_queues(queues_per_pg, 0);
      uint8_t l1_queues_cnt;

      status = trafficMgr->bfTML1NodeQueueAssignmentGet(dev_tgt.dev_id,
                                                        dev_tgt.pipe_id,
                                                        pg_id,
                                                        pg_l1_node,
                                                        &l1_queues_cnt,
                                                        l1_queues.data());
      if (BF_SUCCESS != status) {
        err_data_id = s_hlp.f_pg_queues;
        break;
      }

      // Set both field values consistently even if one is not requested.
      s_hlp.pg_queues.clear();
      std::copy_n(l1_queues.begin(),
                  l1_queues_cnt,
                  std::back_inserter(s_hlp.pg_queues));
      s_hlp.queues_count = l1_queues_cnt;
    }

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s read field_id=%d failed pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        err_data_id,
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't set fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  return status;
}

//-----
bf_status_t BfRtTML1NodeSchedShapingTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (wrk_data == nullptr) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  auto status = trafficMgr->bfTML1NodePortAssignmentGet(dev_tgt.dev_id,
                                                        dev_tgt.pipe_id,
                                                        pg_id,
                                                        pg_l1_node,
                                                        &in_use,
                                                        &pg_port_nr,
                                                        &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d port assignment rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  BfRtTMSchedShapingHelper s_hlp;

  status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  status = trafficMgr->bfTML1NodeSchedMaxRateDefaultsGet(dev_tgt.dev_id,
                                                         port_id,
                                                         pg_l1_node,
                                                         &s_hlp.pps,
                                                         &s_hlp.max_burst_size,
                                                         &s_hlp.max_rate);
  if (BF_SUCCESS == status) {
    status =
        trafficMgr->bfTML1NodeSchedMinRateDefaultsGet(dev_tgt.dev_id,
                                                      port_id,
                                                      pg_l1_node,
                                                      &s_hlp.pps,
                                                      &s_hlp.min_burst_size,
                                                      &s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s get reset values pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't reset fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTML1NodeSchedShapingTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    BfRtTMTableData *wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == wrk_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  auto status = trafficMgr->bfTML1NodePortAssignmentGet(dev_tgt.dev_id,
                                                        dev_tgt.pipe_id,
                                                        pg_id,
                                                        pg_l1_node,
                                                        &in_use,
                                                        &pg_port_nr,
                                                        &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d port assignment rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  BfRtTMSchedShapingHelper s_hlp;

  status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  bool f_min_any = (s_hlp.f_min_rate || s_hlp.f_min_burst_size);
  bool f_max_any = (s_hlp.f_max_rate || s_hlp.f_max_burst_size);
  bool f_cmn_any = s_hlp.f_unit;

  if (f_max_any || (f_cmn_any && !(f_min_any))) {
    status = trafficMgr->bfTML1NodeSchedMaxRateGet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   &s_hlp.pps,
                                                   &s_hlp.max_burst_size,
                                                   &s_hlp.max_rate);
  }
  if (BF_SUCCESS == status && f_min_any) {
    status = trafficMgr->bfTML1NodeSchedMinRateGet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   &s_hlp.pps,
                                                   &s_hlp.min_burst_size,
                                                   &s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s read values failed pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't set fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
  }

  return status;
}

bf_status_t BfRtTML1NodeSchedShapingTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t pg_l1_node,
    const BfRtTMTableData &wrk_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(wrk_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok with this table.
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_dev_port_t port_id = 0;
  uint8_t pg_port_nr = 0;
  bool in_use = false;

  auto status = trafficMgr->bfTML1NodePortAssignmentGet(dev_tgt.dev_id,
                                                        dev_tgt.pipe_id,
                                                        pg_id,
                                                        pg_l1_node,
                                                        &in_use,
                                                        &pg_port_nr,
                                                        &port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pipe_id=%d pg_id=%d l1_node=%d port assignment rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        status,
        bf_err_str(status));
    return status;
  }

  BfRtTMSchedShapingHelper s_hlp;

  status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't init fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  bool f_min_any = (s_hlp.f_min_rate || s_hlp.f_min_burst_size);
  bool f_min_all = (s_hlp.f_min_rate && s_hlp.f_min_burst_size);
  bool f_max_any = (s_hlp.f_max_rate || s_hlp.f_max_burst_size);
  bool f_max_all = (s_hlp.f_max_rate && s_hlp.f_max_burst_size);
  bool f_cmn = s_hlp.f_unit;

  // Pre-read other values from HW if only some are given in the block
  if ((f_max_any && !(f_max_all)) || (f_cmn && !(f_min_any))) {
    status = trafficMgr->bfTML1NodeSchedMaxRateGet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   &s_hlp.pps,
                                                   &s_hlp.max_burst_size,
                                                   &s_hlp.max_rate);
  }
  if (BF_SUCCESS == status && f_min_any && !(f_min_all)) {
    status = trafficMgr->bfTML1NodeSchedMinRateGet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   &s_hlp.pps,
                                                   &s_hlp.min_burst_size,
                                                   &s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s pre-read values failed pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.getFieldValues(*this, wrk_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get fields pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id);
    return status;
  }

  if (f_max_any || (f_cmn && !(f_min_any))) {
    status = trafficMgr->bfTML1NodeSchedMaxRateSet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   s_hlp.pps,
                                                   s_hlp.max_burst_size,
                                                   s_hlp.max_rate);
  }
  if (BF_SUCCESS == status && f_min_any) {
    status = trafficMgr->bfTML1NodeSchedMinRateSet(dev_tgt.dev_id,
                                                   port_id,
                                                   pg_l1_node,
                                                   s_hlp.pps,
                                                   s_hlp.min_burst_size,
                                                   s_hlp.min_rate);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s write values failed pipe_id=%d pg_id=%d pg_l1_node=%d "
        "dev_port=%d"
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.pipe_id,
        pg_id,
        pg_l1_node,
        port_id,
        status,
        bf_err_str(status));
  }

  return status;
}

}  // namespace bfrt
