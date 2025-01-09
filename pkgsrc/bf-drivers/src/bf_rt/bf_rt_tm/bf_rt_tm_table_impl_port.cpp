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

#include <bf_rt_common/bf_rt_init_impl.hpp>

#include "bf_rt_tm_table_helper_ppg.hpp"
#include "bf_rt_tm_table_helper_sched.hpp"
#include "bf_rt_tm_table_impl_port.hpp"

namespace bfrt {

bf_status_t BfRtTMPortTableIntf::tableUsageGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  uint16_t count_ = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // TM Port table ignores dev_tgt.pipe_id using dev_port key.
  auto status = trafficMgr->bfTMPipeGetPortCount(
      dev_tgt.dev_id, BF_DEV_PIPE_ALL, &count_);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get number of ports on dev=%d pipe=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }
  *count = static_cast<uint32_t>(count_);

  return status;
}

bf_status_t BfRtTMPortTableIntf::tableSizeGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags,
                                              size_t *size) const {
  uint32_t size_ = 0;

  auto status = this->tableUsageGet(session, dev_tgt, flags, &size_);
  *size = size_;

  return status;
}

bf_status_t BfRtTMPortTableIntf::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMDevPortKey(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortTableIntf::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtTMDevPortKey *>(key))->reset();
}

// Resets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPortTableIntf::tableResetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_dev_port_t /* port_id */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortTableIntf::tableResetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_dev_port_t /* port_id */,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMPortTableIntf::tableResetEntry(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (p_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual reset methods will set 'default' values
  // and remove processed ids from there.
  std::set<bf_rt_id_t> activeDataFields(p_data->getActiveFields());

  // Get block of related fields at once if this mode is defined for the table.
  // In the latter case the list of field ids should shrink.
  bf_status_t status =
      this->tableResetFields(dev_tgt, port_id, p_data, activeDataFields);
  if (status == BF_EAGAIN) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return status;
  }

  LOG_DBG("%s:%d %s port_id=%d need_fields=%zu, already with_values=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id,
          activeDataFields.size(),
          p_data->hasValues());

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
    status = this->tableResetField(dev_tgt, data_id, port_id, p_data);
    if (status == BF_EAGAIN) {
      continue;  // Skip field which can't be reset at the moment
    } else if (status) {
      break;
    }
  }  // for (field_id)

  return status;
}

// Sets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPortTableIntf::tableSetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_dev_port_t /* port_id */,
    const BfRtTMTableData & /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortTableIntf::tableSetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_dev_port_t /* port_id */,
    const BfRtTMTableData & /* qData */,
    bf_rt_id_t data_id) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMPortTableIntf::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                               const BfRtTMTableData &p_data,
                                               bf_dev_port_t port_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> assignedDataFields(p_data.getAssignedFields());

  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.
  bf_status_t status =
      this->tableSetFields(dev_tgt, port_id, p_data, assignedDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return status;
  }

  LOG_DBG("%s:%d %s port_id=%d active_fields=%zu with_values=%zu to_assign=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id,
          p_data.getActiveFields().size(),
          p_data.hasValues(),
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
    status = this->tableSetField(dev_tgt, port_id, p_data, data_id);
    if (status) {
      break;
    }
  }  // for (field_id)

  return status;
}

bf_status_t BfRtTMPortTableIntf::tableGetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_dev_port_t /* port_id */,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPortTableIntf::tableGetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_dev_port_t /* port_id */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPortTableIntf::tableGetEntry(const bf_rt_target_t &dev_tgt,
                                               bf_dev_port_t port_id,
                                               BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (p_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(p_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  LOG_DBG("%s:%d %s port_id=%d got %zu of %zu values in %zu fields.",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id,
          p_data->hasValues(),
          active_fields_cnt,
          workDataFields.size());

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  bf_status_t status =
      this->tableGetFields(dev_tgt, port_id, p_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  LOG_DBG("%s:%d %s port_id=%d got %zu of %zu values; need %zu fields.",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id,
          p_data->hasValues(),
          active_fields_cnt,
          workDataFields.size());

  for (bf_rt_id_t data_id : workDataFields) {
    status = this->tableGetField(dev_tgt, data_id, port_id, p_data);
    if (status) {
      break;
    }
  }
  LOG_DBG("%s:%d %s port_id=%d got %zu of %zu values; left %zu fields.",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id,
          p_data->hasValues(),
          active_fields_cnt,
          workDataFields.size());

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s port_id=%d rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id,
              status,
              bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTMPortTableIntf::validateKey(const bf_rt_target_t &dev_tgt,
                                             bf_dev_port_t port_id) const {
  LOG_DBG("%s:%d dev_id=%d, dev_port=%d",
          __func__,
          __LINE__,
          dev_tgt.dev_id,
          port_id);
  if (!DEV_PORT_VALIDATE(port_id)) {
    // TM Port deals with regular ports only.
    return BF_INVALID_ARG;
  }
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  return trafficMgr->bfTMPortIsValid(dev_tgt.dev_id, port_id);
}

bf_status_t BfRtTMPortTableIntf::tableEntryGet(
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

  BfRtTMTableData *p_data = static_cast<BfRtTMTableData *>(data);

  // Table with actions should proceed with get using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && p_data->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s No active data fields to get",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    return BF_SUCCESS;
  }

  const BfRtTMDevPortKey &pKey = static_cast<const BfRtTMDevPortKey &>(key);
  bf_dev_port_t port_id = 0;

  bf_status_t status = pKey.getId(port_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TM API allows dev_port value out of regular bounds.
  // A special case might be the Mirror port which is #72.
  if (BF_SUCCESS != this->validateKey(dev_tgt, port_id)) {
    LOG_ERROR("%s:%d %s Incorrect dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return BF_INVALID_ARG;
  }

  // TODO: Some of the Port tables, like port.cfg, have Queue mapping fields,
  // and the queue_map state lock should be acquired at this point to keep
  // the table entry's fields consistent.
  // If the Queue mapping state lock should be acquired before the Table lock
  // then do it at the overloaded tableEntryGet() wrapper.
  if (this->isVolatile()) {
    status = this->tableGetEntry(dev_tgt, port_id, p_data);
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
    status = this->tableGetEntry(dev_tgt, port_id, p_data);
  }

  return status;
}

bf_status_t BfRtTMPortTableIntf::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const BfRtTMTableData &p_data = static_cast<const BfRtTMTableData &>(data);

  // Table with actions should proceed with mod using just action_id
  // eg. when an entry is empty, but action id value set.
  if (!(this->actionIdApplicable()) && p_data.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  const BfRtTMDevPortKey &pKey = static_cast<const BfRtTMDevPortKey &>(key);
  bf_dev_port_t port_id = 0;

  bf_status_t status = pKey.getId(port_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TM API allows dev_port out of regular bounds.
  // A special case might be the Mirror port which is #72.
  if (BF_SUCCESS != this->validateKey(dev_tgt, port_id)) {
    LOG_ERROR("%s:%d %s Incorrect dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return BF_INVALID_ARG;
  }

  // TODO: Apply the Queue mapping state lock if needed.
  // See tableEntryGet() comment for details.

  LOG_DBG("%s:%d %s Get entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> lock(this->entry_lock);
  LOG_DBG("%s:%d %s Got entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  return this->tableSetEntry(dev_tgt, p_data, port_id);
}

bf_status_t BfRtTMPortTableIntf::keyFirst(bf_rt_target_t &dev_tgt,
                                          BfRtTableKey *key) const {
  bf_dev_port_t port_id = 0;

  if (key == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMDevPortKey *pKey = static_cast<BfRtTMDevPortKey *>(key);

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  auto status =
      trafficMgr->bfTMPipeGetFirst(dev_tgt.dev_id, &(dev_tgt.pipe_id));
  if (BF_SUCCESS == status) {
    status = trafficMgr->bfTMPipeGetPortFirst(
        dev_tgt.dev_id, dev_tgt.pipe_id, &port_id);
  }
  if (BF_SUCCESS == status) {
    pKey->setId(port_id);
  } else {
    LOG_ERROR("%s:%d %s Can't get first port on dev_id=%d pipe_id=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
  }
  return status;
}

bf_status_t BfRtTMPortTableIntf::keyNext(bf_rt_target_t &dev_tgt,
                                         const BfRtTableKey &key,
                                         BfRtTableKey *key_next) const {
  if (key_next == nullptr) {
    return BF_INVALID_ARG;
  }

  const BfRtTMDevPortKey &from_key = static_cast<const BfRtTMDevPortKey &>(key);
  BfRtTMDevPortKey *next_key = static_cast<BfRtTMDevPortKey *>(key_next);
  bf_dev_port_t port_id = 0;

  bf_status_t status = from_key.getId(port_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // TM API allows dev_port out of regular port range.
  // A special case might be the Mirror port which is #72.
  if (BF_SUCCESS != this->validateKey(dev_tgt, port_id)) {
    LOG_ERROR("%s:%d %s Incorrect key port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return BF_INVALID_ARG;
  }

  LOG_DBG("%s:%d %s get next after port_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          port_id);

  dev_tgt.pipe_id = DEV_PORT_TO_PIPE(port_id);  // Ignore dev_tgt pipe value.
  bf_dev_port_t port_id_next = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  status =
      trafficMgr->bfTMPipeGetPortNext(dev_tgt.dev_id, port_id, &port_id_next);
  if (BF_OBJECT_NOT_FOUND == status) {
    // Try to continue on the next pipe.
    LOG_DBG("%s:%d %s Try next pipe after port_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            port_id);
    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt.dev_id, dev_tgt.pipe_id, &(dev_tgt.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      return status;  // No more ports on this device
    }
    status = trafficMgr->bfTMPipeGetPortFirst(
        dev_tgt.dev_id, dev_tgt.pipe_id, &port_id_next);
  }
  if (BF_SUCCESS == status) {
    next_key->setId(port_id_next);
  } else {
    LOG_ERROR(
        "%s:%d %s Can't get port on dev_id=%d next to port_id=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        port_id,
        status,
        bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTMPortTableIntf::tableClear(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  BfRtTMTableData dft_data(this);
  BfRtTMDevPortKey curr_key(this);

  bf_rt_target_t dev_wrk = dev_tgt;

  // TODO: Apply locks to related objects if needed for consistent clear.

  LOG_DBG("%s:%d %s Get entry lock once to clear all entries",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> lock(this->entry_lock);
  LOG_DBG("%s:%d %s Got entry lock to clear all entries",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  auto status = this->keyFirst(dev_wrk, &curr_key);
  if (status != BF_SUCCESS) {
    return status;
  }

  bf_dev_port_t port_id = 0;
  uint32_t cleared_cnt = 0;
  uint32_t skip_cnt = 0;

  do {
    status = curr_key.getId(port_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't get key value",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      break;
    }
    status = this->tableResetEntry(dev_wrk, port_id, &dft_data);
    if (status == BF_EAGAIN) {
      LOG_TRACE("%s:%d %s Skip reset of dev_id=%d port_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id,
                port_id);
      status = BF_SUCCESS;
      skip_cnt++;
    } else if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Can't reset entry dev_id=%d port_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id,
                port_id);
      break;
    } else {
      status = this->tableSetEntry(dev_wrk, dft_data, port_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Can't clear entry dev_id=%d port_id=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_wrk.dev_id,
                  port_id);
        break;
      }
      cleared_cnt++;
    }

    status = this->keyNext(dev_wrk, curr_key, &curr_key);
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more ports on this device
    }
  } while (BF_SUCCESS == status);

  LOG_TRACE("%s:%d %s cleared %d entries, skip %d entries, rc=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            cleared_cnt,
            skip_cnt,
            status);
  return status;
}

//----------------------------------------------------------------------------
bf_status_t BfRtTMPortCfgTable::tableGetQueueMappingField(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t data_id,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  uint8_t queues_per_pg = 0;

  status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, nullptr, &queues_per_pg, nullptr);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  size_t q_buf_len = queues_per_pg;
  std::unique_ptr<uint8_t[]> q_buffer(new uint8_t[q_buf_len]);

  uint8_t *q_map = q_buffer.get();
  if (q_map == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate buffer",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  std::memset(q_map, 0, q_buf_len);

  uint8_t q_cnt = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status =
      trafficMgr->bfTMPortQMappingGet(dev_tgt.dev_id, port_id, &q_cnt, q_map);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Get queue mappings failed for port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id);
    return status;
  }

  if (BFRT_TM_DEBUG) {
    std::stringstream dbg_m;
    dbg_m.str("");

    for (uint8_t q = 0; q < queues_per_pg; q++) {
      dbg_m << ((q == 0) ? "" : ", ") << (static_cast<int>(q_map[q]));
    }
    LOG_DBG("%s:%d %s port_id=%d queues=%d map=[%s]",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            port_id,
            q_cnt,
            dbg_m.str().c_str());
  }

  if (this->dataFieldDeclared("port_queues_count", 0, data_id)) {
    status = p_data->setValue(data_id, &q_cnt, sizeof(uint8_t));

  } else if (this->dataFieldDeclared("ingress_qid_map", 0, data_id)) {
    std::vector<bf_rt_id_t> ingress_qid_map;
    if (q_cnt) {
      for (uint8_t q = 0; q < queues_per_pg; q++) {
        if (q_map[q] >= q_cnt) {
          BF_RT_DBGCHK(0);
          return BF_UNEXPECTED;
        }
        ingress_qid_map.push_back(static_cast<bf_rt_id_t>(q_map[q]));
      }
    }
    status = p_data->setValue(data_id, ingress_qid_map);

  } else if (this->dataFieldDeclared("egress_qid_queues", 0, data_id)) {
    std::vector<bf_rt_id_t> egress_qid_queues;
    if (q_cnt) {
      uint8_t pg_base_queue = 0;
      bf_tm_pg_t pg_id = 0;
      bool is_mapped;

      status = trafficMgr->bfTMPortBaseQueueGet(
          dev_tgt.dev_id, port_id, &pg_id, &pg_base_queue, &is_mapped);

      // The base queue should be mapped if there are any queues mapped.
      if (status != BF_SUCCESS || !is_mapped) {
        LOG_ERROR("%s:%d %s Can't get base queue for port_id=%d - %s mapped",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  port_id,
                  (is_mapped) ? "is" : "not");
        return status;
      }

      for (uint8_t q = 0; q < queues_per_pg; q++) {
        if (q_map[q] >= queues_per_pg ||
            q_map[q] + pg_base_queue >= queues_per_pg) {
          BF_RT_DBGCHK(0);
          return BF_UNEXPECTED;
        }
        egress_qid_queues.push_back(
            static_cast<bf_rt_id_t>(pg_base_queue + q_map[q]));
      }
    }
    status = p_data->setValue(data_id, egress_qid_queues);
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  return status;
}

bf_status_t BfRtTMPortCfgTable::tableGetField(const bf_rt_target_t &dev_tgt,
                                              bf_rt_id_t data_id,
                                              bf_dev_port_t port_id,
                                              BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  bf_tm_pg_t pg_id = 0;
  uint8_t pg_port_nr = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status_api = trafficMgr->bfTMPortGroupGet(
      dev_tgt.dev_id, port_id, &pg_id, &pg_port_nr);
  if (status_api) {
    LOG_ERROR("%s:%d %s Can't get port group for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status_api;
  }

  if (this->dataFieldDeclared("pg_id", 0, data_id)) {
    status = p_data->setValue(
        data_id, static_cast<uint8_t *>(&pg_id), sizeof(uint8_t));

  } else if (this->dataFieldDeclared("pg_port_nr", 0, data_id)) {
    status = p_data->setValue(data_id, &pg_port_nr, sizeof(uint8_t));

  } else if (this->dataFieldDeclared("pkt_extraction_credits", 0, data_id)) {
    uint32_t pkt_credits = 0;

    status_api =
        trafficMgr->bfTMPortCreditsGet(dev_tgt.dev_id, port_id, &pkt_credits);
    if (BF_SUCCESS == status_api) {
      status = p_data->setValue(data_id, static_cast<uint64_t>(pkt_credits));
    }
  } else if (this->dataFieldDeclared("copy_to_cpu", 0, data_id)) {
    bf_dev_port_t cpu_port = 0;
    bool copy_to_cpu = false;

    auto *mcMgr = McMgrIntf::getInstance();
    status_api =
        mcMgr->mcMgrGetCopyToCPUPort(dev_tgt.dev_id, &cpu_port, &copy_to_cpu);
    if (BF_SUCCESS == status_api) {
      LOG_DBG("%s:%d %s dev_id=%d port_id=%d is %s CPU copy port %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              (cpu_port == port_id) ? "the" : "NOT a",
              (cpu_port == port_id && copy_to_cpu) ? "enabled" : "");
      copy_to_cpu = (copy_to_cpu && cpu_port == port_id);
      status = p_data->setValue(data_id, copy_to_cpu);
    }
  } else if (this->dataFieldDeclared("port_queues_count", 0, data_id) ||
             this->dataFieldDeclared("ingress_qid_map", 0, data_id) ||
             this->dataFieldDeclared("egress_qid_queues", 0, data_id)) {
    return this->tableGetQueueMappingField(dev_tgt, data_id, port_id, p_data);

  } else if (this->dataFieldDeclared("mirror_drop_destination", 0, data_id) ||
             this->dataFieldDeclared("mirror_drop_pg_queue", 0, data_id)) {
    bool mirror_drop_destination = false;
    bf_dev_port_t drop_port = 0;
    bf_tm_queue_t drop_queue = 0;
    bf_dev_pipe_t port_pipe = DEV_PORT_TO_PIPE(port_id);

    status_api = trafficMgr->bfTMPipeMirrorOnDropDestGet(
        dev_tgt.dev_id, port_pipe, &drop_port, &drop_queue);

    if (BF_SUCCESS == status_api) {
      mirror_drop_destination = (drop_port == port_id);
      LOG_DBG(
          "%s:%d %s dev_id=%d port_id=%d is %s mirror drop destination "
          "(drop_port=%d, drop_queue=%d)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          (mirror_drop_destination) ? "the" : "not a",
          drop_port,
          drop_queue);

      if (!mirror_drop_destination) {
        drop_queue = 0;  // Clean as it is for some other port.
      } else {
        uint8_t pg_base_queue = 0;
        bf_tm_pg_t pg_id_ = 0;
        bool is_mapped = false;

        status_api = trafficMgr->bfTMPortBaseQueueGet(
            dev_tgt.dev_id, port_id, &pg_id_, &pg_base_queue, &is_mapped);
        // The base queue should be mapped if there are any queues mapped.
        if (BF_SUCCESS != status_api || !is_mapped) {
          LOG_ERROR(
              "%s:%d %s Can't get base queue for dev_id=%d port_id=%d - %s "
              "mapped",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              (is_mapped) ? "is" : "not");
        }
        drop_queue += pg_base_queue;  // From port-relative to port group queue
      }

      if (this->dataFieldDeclared("mirror_drop_destination", 0, data_id)) {
        status = p_data->setValue(data_id, mirror_drop_destination);
      } else if (this->dataFieldDeclared("mirror_drop_pg_queue", 0, data_id)) {
        status = p_data->setValue(data_id, static_cast<uint64_t>(drop_queue));
      }
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d set value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  if (status_api) {
    LOG_ERROR("%s:%d %s field_id=%d get value from dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              dev_tgt.dev_id,
              port_id);
    status = status_api;
  }

  return status;
}

//----------------------------------------------------------------------------
bf_status_t BfRtTMPortBufferTable::tableResetField(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t data_id,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_INVALID_ARG;

  bool ct_enable = false;
  uint8_t uc_ct_limit_cells = 0;
  uint32_t ig_limit_cells = 0;
  uint32_t ig_hysteresis_cells = 0;
  uint32_t eg_limit_cells = 0;
  uint32_t eg_hysteresis_cells = 0;
  uint32_t skid_limit_cells = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPortBufferDefaultsGet(dev_tgt.dev_id,
                                                 port_id,
                                                 &ct_enable,
                                                 &uc_ct_limit_cells,
                                                 &ig_limit_cells,
                                                 &ig_hysteresis_cells,
                                                 &eg_limit_cells,
                                                 &eg_hysteresis_cells,
                                                 &skid_limit_cells);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM defaults dev_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id);
    return status;
  }
  if (this->dataFieldDeclared("uc_ct_limit_cells", 0, data_id)) {
    bool is_enabled = false;
    bool has_mac = false;
    status = trafficMgr->bfTMPortStatusGet(
        dev_tgt.dev_id, port_id, NULL, &is_enabled, NULL, NULL, &has_mac);
    if (BF_SUCCESS == status && (!is_enabled || !has_mac)) {
      LOG_DBG("%s:%d %s Skip uc_ct_limit_cells reset at dev_id=%d, port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
      return BF_EAGAIN;
    } else if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Invalid port: dev_id=%d, port_id=%d, rc=%d(%s)",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                port_id,
                status,
                bf_err_str(status));
    } else {
      status =
          p_data->setValue(data_id, static_cast<uint64_t>(uc_ct_limit_cells));
    }
  } else if (this->dataFieldDeclared("ig_limit_cells", 0, data_id)) {
    status = p_data->setValue(data_id, static_cast<uint64_t>(ig_limit_cells));

  } else if (this->dataFieldDeclared("ig_hysteresis_cells", 0, data_id)) {
    status =
        p_data->setValue(data_id, static_cast<uint64_t>(ig_hysteresis_cells));

  } else if (this->dataFieldDeclared("eg_limit_cells", 0, data_id)) {
    status = p_data->setValue(data_id, static_cast<uint64_t>(eg_limit_cells));

  } else if (this->dataFieldDeclared("eg_hysteresis_cells", 0, data_id)) {
    status =
        p_data->setValue(data_id, static_cast<uint64_t>(eg_hysteresis_cells));

  } else if (this->dataFieldDeclared("skid_limit_cells", 0, data_id)) {
    status = p_data->setValue(data_id, static_cast<uint64_t>(skid_limit_cells));

  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d reset value, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              status,
              bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTMPortBufferTable::tableGetField(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t data_id,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("ct_enable", 0, data_id)) {
    bool ct_enable = false;

    status_api =
        trafficMgr->bfTMPortCutThroughGet(dev_tgt.dev_id, port_id, &ct_enable);
    if (BF_SUCCESS == status_api) {
      status = p_data->setValue(data_id, ct_enable);
    }
  } else if (this->dataFieldDeclared("uc_ct_limit_cells", 0, data_id)) {
    uint8_t uc_ct_limit_cells = 0;

    status_api = trafficMgr->bfTMPortCutThroughLimitGet(
        dev_tgt.dev_id, port_id, &uc_ct_limit_cells);
    if (BF_SUCCESS == status_api) {
      status =
          p_data->setValue(data_id, static_cast<uint64_t>(uc_ct_limit_cells));
    }
  } else if (this->dataFieldDeclared("ig_limit_cells", 0, data_id)) {
    uint32_t ig_limit_cells = 0;

    status_api = trafficMgr->bfTMPortBufIgLimitGet(
        dev_tgt.dev_id, port_id, &ig_limit_cells);
    if (BF_SUCCESS == status_api) {
      status = p_data->setValue(data_id, static_cast<uint64_t>(ig_limit_cells));
    }
  } else if (this->dataFieldDeclared("ig_hysteresis_cells", 0, data_id)) {
    uint32_t ig_hysteresis_cells = 0;

    status_api = trafficMgr->bfTMPortBufIgHysteresisGet(
        dev_tgt.dev_id, port_id, &ig_hysteresis_cells);
    if (BF_SUCCESS == status_api) {
      status =
          p_data->setValue(data_id, static_cast<uint64_t>(ig_hysteresis_cells));
    }
  } else if (this->dataFieldDeclared("eg_limit_cells", 0, data_id)) {
    uint32_t eg_limit_cells = 0;

    status_api = trafficMgr->bfTMPortBufEgLimitGet(
        dev_tgt.dev_id, port_id, &eg_limit_cells);
    if (BF_SUCCESS == status_api) {
      status = p_data->setValue(data_id, static_cast<uint64_t>(eg_limit_cells));
    }
  } else if (this->dataFieldDeclared("eg_hysteresis_cells", 0, data_id)) {
    uint32_t eg_hysteresis_cells = 0;

    status_api = trafficMgr->bfTMPortBufEgHysteresisGet(
        dev_tgt.dev_id, port_id, &eg_hysteresis_cells);
    if (BF_SUCCESS == status_api) {
      status =
          p_data->setValue(data_id, static_cast<uint64_t>(eg_hysteresis_cells));
    }
  } else if (this->dataFieldDeclared("skid_limit_cells", 0, data_id)) {
    uint32_t skid_limit_cells = 0;

    status_api = trafficMgr->bfTMPortBufSkidLimitGet(
        dev_tgt.dev_id, port_id, &skid_limit_cells);
    if (BF_SUCCESS == status_api) {
      status =
          p_data->setValue(data_id, static_cast<uint64_t>(skid_limit_cells));
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d set value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  if (status_api) {
    LOG_ERROR("%s:%d %s field_id=%d get value from dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              dev_tgt.dev_id,
              port_id);
    status = status_api;
  }

  return status;
}

bf_status_t BfRtTMPortBufferTable::tableSetField(const bf_rt_target_t &dev_tgt,
                                                 bf_dev_port_t port_id,
                                                 const BfRtTMTableData &p_data,
                                                 bf_rt_id_t data_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("uc_ct_limit_cells", 0, data_id)) {
    uint64_t uc_ct_limit_cells = 0;

    status = p_data.getValue(data_id, &uc_ct_limit_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortCutThroughLimitSet(
          dev_tgt.dev_id, port_id, static_cast<uint8_t>(uc_ct_limit_cells));
    }
  } else if (this->dataFieldDeclared("ig_limit_cells", 0, data_id)) {
    uint64_t ig_limit_cells = 0;

    status = p_data.getValue(data_id, &ig_limit_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortBufIgLimitSet(
          dev_tgt.dev_id, port_id, static_cast<uint32_t>(ig_limit_cells));
    }
  } else if (this->dataFieldDeclared("ig_hysteresis_cells", 0, data_id)) {
    uint64_t ig_hysteresis_cells = 0;

    status = p_data.getValue(data_id, &ig_hysteresis_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortBufIgHysteresisSet(
          dev_tgt.dev_id, port_id, static_cast<uint32_t>(ig_hysteresis_cells));
    }
  } else if (this->dataFieldDeclared("eg_limit_cells", 0, data_id)) {
    uint64_t eg_limit_cells = 0;

    status = p_data.getValue(data_id, &eg_limit_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortBufEgLimitSet(
          dev_tgt.dev_id, port_id, static_cast<uint32_t>(eg_limit_cells));
    }
  } else if (this->dataFieldDeclared("eg_hysteresis_cells", 0, data_id)) {
    uint64_t eg_hysteresis_cells = 0;

    status = p_data.getValue(data_id, &eg_hysteresis_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortBufEgHysteresisSet(
          dev_tgt.dev_id, port_id, static_cast<uint32_t>(eg_hysteresis_cells));
    }
  } else if (this->dataFieldDeclared("skid_limit_cells", 0, data_id)) {
    uint64_t skid_limit_cells = 0;

    status = p_data.getValue(data_id, &skid_limit_cells);
    if (BF_SUCCESS == status) {
      status_api = trafficMgr->bfTMPortBufSkidLimitSet(
          dev_tgt.dev_id, port_id, static_cast<uint32_t>(skid_limit_cells));
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d get value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  if (status_api) {
    LOG_ERROR("%s:%d %s field_id=%d set value at dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              dev_tgt.dev_id,
              port_id);
    status = status_api;
  }
  return status;
}

//----------------------------------------------------------------------------
const std::map<std::string, bf_tm_flow_ctrl_type_t>
    BfRtTMPortFlowCtrlTable::str_to_flow_ctrl_type{{"NONE", BF_TM_PAUSE_NONE},
                                                   {"PFC", BF_TM_PAUSE_PFC},
                                                   {"PAUSE", BF_TM_PAUSE_PORT}};

const std::map<bf_tm_flow_ctrl_type_t, std::string>
    BfRtTMPortFlowCtrlTable::flow_ctrl_type_to_str{{BF_TM_PAUSE_NONE, "NONE"},
                                                   {BF_TM_PAUSE_PFC, "PFC"},
                                                   {BF_TM_PAUSE_PORT, "PAUSE"}};

bf_status_t BfRtTMPortFlowCtrlTable::tableResetField(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t data_id,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_INVALID_ARG;
  uint8_t icos_cnt = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPortIcosCntGet(dev_tgt.dev_id, port_id, &icos_cnt);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM configuration",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  std::unique_ptr<uint8_t[]> cos_to_icos(new uint8_t[icos_cnt]);
  uint8_t *cos_map = cos_to_icos.get();
  if (cos_map == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate buffer",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  std::memset(cos_map, 0, icos_cnt);

  bf_tm_flow_ctrl_type_t mode_tx = BF_TM_PAUSE_NONE;
  bf_tm_flow_ctrl_type_t mode_rx = BF_TM_PAUSE_NONE;

  status = trafficMgr->bfTMPortFlowCtrlDefaultsGet(
      dev_tgt.dev_id, port_id, &mode_tx, &mode_rx, cos_map);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM defaults dev_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id);
    return status;
  }

  if (this->dataFieldDeclared("mode_tx", 0, data_id)) {
    std::string mode_tx_str;
    try {
      mode_tx_str = flow_ctrl_type_to_str.at(mode_tx);
    } catch (std::out_of_range &) {
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    status = p_data->setValue(data_id, mode_tx_str);

  } else if (this->dataFieldDeclared("cos_to_icos", 0, data_id)) {
    std::vector<bf_rt_id_t> cos_list;

    for (int i = 0; i < icos_cnt; i++) {
      cos_list.push_back(static_cast<bf_rt_id_t>(cos_map[i]));
    }
    status = p_data->setValue(data_id, cos_list);

  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d reset value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  return status;
}

bf_status_t BfRtTMPortFlowCtrlTable::tableGetField(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t data_id,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_INVALID_ARG;
  bf_status_t status = BF_INVALID_ARG;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("mode_tx", 0, data_id)) {
    bf_tm_flow_ctrl_type_t mode_tx = BF_TM_PAUSE_NONE;

    status_api =
        trafficMgr->bfTMPortFlowCtrlTxGet(dev_tgt.dev_id, port_id, &mode_tx);
    if (BF_SUCCESS == status_api) {
      std::string mode_tx_str;
      try {
        mode_tx_str = flow_ctrl_type_to_str.at(mode_tx);
      } catch (std::out_of_range &) {
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
      }

      status = p_data->setValue(data_id, mode_tx_str);
    }
  } else if (this->dataFieldDeclared("mode_rx", 0, data_id)) {
    bf_tm_flow_ctrl_type_t mode_rx = BF_TM_PAUSE_NONE;

    status_api =
        trafficMgr->bfTMPortFlowCtrlRxGet(dev_tgt.dev_id, port_id, &mode_rx);
    if (BF_SUCCESS == status_api) {
      std::string mode_rx_str;
      try {
        mode_rx_str = flow_ctrl_type_to_str.at(mode_rx);
      } catch (std::out_of_range &) {
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
      }

      status = p_data->setValue(data_id, mode_rx_str);
    }
  } else if (this->dataFieldDeclared("cos_to_icos", 0, data_id)) {
    uint8_t icos_cnt = 0;

    status_api =
        trafficMgr->bfTMPortIcosCntGet(dev_tgt.dev_id, port_id, &icos_cnt);
    if (status_api) {
      LOG_ERROR("%s:%d %s Can't get TM configuration",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status_api;
    }

    std::unique_ptr<uint8_t[]> cos_to_icos(new uint8_t[icos_cnt]);
    std::vector<bf_rt_id_t> cos_list;
    uint8_t *cos_map = cos_to_icos.get();
    if (cos_map == nullptr) {
      LOG_ERROR("%s:%d %s Failed to allocate buffer",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return BF_NO_SYS_RESOURCES;
    }
    std::memset(cos_map, 0, icos_cnt);

    status_api =
        trafficMgr->bfTMPortCosMappingGet(dev_tgt.dev_id, port_id, cos_map);
    if (BF_SUCCESS == status_api) {
      bool f_dbg_m = BFRT_TM_DEBUG;
      std::stringstream dbg_m;
      dbg_m.str("");

      for (int i = 0; i < icos_cnt; i++) {
        cos_list.push_back(static_cast<bf_rt_id_t>(cos_map[i]));
        if (f_dbg_m) {
          dbg_m << ((i == 0) ? "" : ", ") << i << ":";
          dbg_m << (static_cast<int>(cos_map[i]));
        }
      }
      LOG_DBG("%s:%d %s dev_id=%d port_id=%d cos_to_icos=[%s]",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              dbg_m.str().c_str());

      status = p_data->setValue(data_id, cos_list);
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d set value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  if (status_api) {
    LOG_ERROR("%s:%d %s field_id=%d get value from dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              dev_tgt.dev_id,
              port_id);
    status = status_api;
  }

  return status;
}

bf_status_t BfRtTMPortFlowCtrlTable::tableSetField(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    const BfRtTMTableData &p_data,
    bf_rt_id_t data_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status_api = BF_INVALID_ARG;
  bf_status_t status = BF_INVALID_ARG;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (this->dataFieldDeclared("mode_tx", 0, data_id)) {
    std::string mode_tx_str("");

    status = p_data.getValue(data_id, &mode_tx_str);
    if (BF_SUCCESS == status) {
      bf_tm_flow_ctrl_type_t mode_tx = BF_TM_PAUSE_NONE;

      try {
        mode_tx = str_to_flow_ctrl_type.at(mode_tx_str);
      } catch (std::out_of_range &) {
        LOG_ERROR("%s:%d value is out of range", __func__, __LINE__);
        return BF_INVALID_ARG;
      }

      status_api =
          trafficMgr->bfTMPortFlowCtrlTxSet(dev_tgt.dev_id, port_id, mode_tx);
    }
  } else if (this->dataFieldDeclared("cos_to_icos", 0, data_id)) {
    uint8_t icos_cnt = 0;

    status_api =
        trafficMgr->bfTMPortIcosCntGet(dev_tgt.dev_id, port_id, &icos_cnt);
    if (status_api) {
      LOG_ERROR("%s:%d %s Can't get TM configuration",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status_api;
    }

    std::vector<bf_rt_id_t> cos_list;

    status = p_data.getValue(data_id, &cos_list);
    if (BF_SUCCESS == status) {
      if (cos_list.size() != icos_cnt) {
        LOG_ERROR("%s:%d %s field_id=%d must have %d items",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  data_id,
                  icos_cnt);
        return BF_INVALID_ARG;
      }

      std::unique_ptr<uint8_t[]> cos_to_icos(new uint8_t[icos_cnt]);
      uint8_t *cos_map = cos_to_icos.get();
      if (cos_map == nullptr) {
        LOG_ERROR("%s:%d %s Failed to allocate buffer",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str());
        return BF_NO_SYS_RESOURCES;
      }
      std::memset(cos_map, 0, icos_cnt);

      bool f_dbg_m = BFRT_TM_DEBUG;
      std::stringstream dbg_m;
      dbg_m.str("");

      for (int i = 0; i < icos_cnt; i++) {
        if (cos_list[i] >= icos_cnt) {
          LOG_ERROR("%s:%d %s invalid cos_to_icos[%d]=%d",
                    __func__,
                    __LINE__,
                    this->table_name_get().c_str(),
                    i,
                    cos_list[i]);
          return BF_INVALID_ARG;
        }
        cos_map[i] = cos_list[i];

        if (f_dbg_m) {
          dbg_m << ((i == 0) ? "" : ", ") << i << ":";
          dbg_m << (static_cast<int>(cos_map[i]));
        }
      }
      LOG_DBG("%s:%d %s dev_id=%d port_id=%d cos_to_icos=[%s]",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              dbg_m.str().c_str());

      status_api =
          trafficMgr->bfTMPortCosMappingSet(dev_tgt.dev_id, port_id, cos_map);
    }
  } else {
    LOG_ERROR("%s:%d %s field_id=%d is not implemented",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
    return BF_NOT_IMPLEMENTED;
  }

  if (status) {
    LOG_ERROR("%s:%d %s field_id=%d get value",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id);
  }
  if (status_api) {
    LOG_ERROR("%s:%d %s field_id=%d set value at dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              data_id,
              dev_tgt.dev_id,
              port_id);
    status = status_api;
  }

  return status;
}

//-----------
bf_status_t BfRtTMPortDpgTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status =
      trafficMgr->bfTMPPGDefaultPpgGet(dev_tgt.dev_id, port_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get DPG handle for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  //-- Buffer fields
  {
    BfRtTMPpgBufferHelper ppg_helper;

    status =
        ppg_helper.getFieldsBuffer(dev_tgt, *this, dpg_hdl, wrk_fields, p_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s can't get fields dev_id=%d port_id=%d dpg_hdl=0x%x",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                port_id,
                dpg_hdl);
      return status;
    }
  }

  //-- iCoS fields

  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_tgt.dev_id, this->programNameGet());
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d ERROR device state dev_id=%d, program='%s'",
              __func__,
              __LINE__,
              dev_tgt.dev_id,
              this->programNameGet().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto tmPpgState = device_state->tmPpgState.getStateObj();

  //-- PPG lock to fix the state
  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  // The entry lock should be already taken.
  // Lock the PPG state to read consistent iCoS mappings.
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  //-- Read DPG iCoS
  uint8_t icos_mask = 0;
  status = BfRtTMPpgIcosHelper::getFieldsIcos(
      dev_tgt, *this, dpg_hdl, wrk_fields, p_data, icos_mask);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get iCoS fields dev_id=%d port_id=%d dpg_hdl=0x%x",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        port_id,
        dpg_hdl);

    return status;
  }

  status = BfRtTMPpgIcosHelper::getFieldsPPGs(
      dev_tgt, *this, *tmPpgState, port_id, icos_mask, wrk_fields, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get PPG fields dev_id=%d port_id=%d dpg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              dpg_hdl);
  }

  return status;
}

//---
bf_status_t BfRtTMPortDpgTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  //-- iCoS fields are read-only

  //-- Buffer fields

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status =
      trafficMgr->bfTMPPGDefaultPpgGet(dev_tgt.dev_id, port_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get DPG handle for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  BfRtTMPpgBufferHelper ppg_helper;

  status =
      ppg_helper.setFieldsBuffer(dev_tgt, *this, p_data, wrk_fields, dpg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d dpg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              dpg_hdl);
  }

  return status;
}

bf_status_t BfRtTMPortDpgTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status =
      trafficMgr->bfTMPPGDefaultPpgGet(dev_tgt.dev_id, port_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get DPG handle for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  //-- Buffer fields
  {
    BfRtTMPpgBufferHelper ppg_helper;

    status = ppg_helper.resetFieldsBuffer(
        dev_tgt, *this, dpg_hdl, wrk_fields, p_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s can't reset fields dev_id=%d port_id=%d dpg_hdl=0x%x",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                port_id,
                dpg_hdl);
      return status;
    }
  }

  //--- iCoS fields are r/o do not reset.

  return status;
}

//------------------ TM_PORT_SCHED_CFG

bf_status_t BfRtTMPortSchedCfgTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPortSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  if (sc_hlp.f_max_rate_enable) {
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    status = trafficMgr->bfTMPortSchedShapingEnableDefaultGet(
        dev_tgt.dev_id, port_id, &sc_hlp.max_rate_enable);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s read reset values failed: dev_id=%d, port_id=%d, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
  }

  if (sc_hlp.f_scheduling_speed) {
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    bf_port_speeds_t port_speed_ = BF_SPEED_NONE;
    status = trafficMgr->bfTMPortSchedSpeedResetGet(
        dev_tgt.dev_id, port_id, &port_speed_);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s get Port speed failed: dev_id=%d, port_id=%d, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
    sc_hlp.scheduling_speed = static_cast<bf_port_speed_t>(port_speed_);
  }

  status = sc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
  }

  return status;
}

//-----------
bf_status_t BfRtTMPortSchedCfgTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPortSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (sc_hlp.f_scheduling_speed) {
    bf_port_speeds_t scheduling_speed_;

    status = trafficMgr->bfTMPortSchedSpeedGet(
        dev_tgt.dev_id, port_id, &scheduling_speed_);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s scheduling speed read failed: dev_id=%d, port_id=%d, "
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
    sc_hlp.scheduling_speed = static_cast<bf_port_speed_t>(scheduling_speed_);
  }

  if (sc_hlp.f_l1_nodes || sc_hlp.l1_nodes_count) {
    bf_tm_dev_cfg_t tm_cfg = {0};

    status = trafficMgr->bfTMDevCfgGet(dev_tgt.dev_id, &tm_cfg);
    if (status) {
      LOG_ERROR("%s:%d %s Can't get TM configuration",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }

    std::vector<bf_tm_l1_node_t> l1_nodes(tm_cfg.l1_per_pg, 0);
    uint8_t l1_nodes_cnt;

    status = trafficMgr->bfTMPortL1NodeAssignmentGet(
        dev_tgt.dev_id, port_id, true, &l1_nodes_cnt, l1_nodes.data());
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s L1 Node association read failed: dev_id=%d, port_id=%d, "
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
    // Set both field values consistently even if one is not requested.
    sc_hlp.l1_nodes.clear();
    std::copy_n(
        l1_nodes.begin(), l1_nodes_cnt, std::back_inserter(sc_hlp.l1_nodes));
    sc_hlp.l1_nodes_count = l1_nodes_cnt;
  }

  if (sc_hlp.f_max_rate_enable) {
    status = trafficMgr->bfTMPortSchedShapingEnableGet(
        dev_tgt.dev_id, port_id, &sc_hlp.max_rate_enable);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s shaping enable status read failed: dev_id=%d, port_id=%d, "
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
  }

  status = sc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPortSchedCfgTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  BfRtTMPortSchedCfgHelper sc_hlp;

  auto status = sc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  status = sc_hlp.getFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (sc_hlp.f_max_rate_enable) {
    status =
        (sc_hlp.max_rate_enable)
            ? trafficMgr->bfTMPortSchedShapingEnable(dev_tgt.dev_id, port_id)
            : trafficMgr->bfTMPortSchedShapingDisable(dev_tgt.dev_id, port_id);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s Port shaping mode set failed: dev_id=%d, port_id=%d, "
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
  }

  // Set the Scheduling speed as the last to allow other fields' setup before
  // enable.
  if (sc_hlp.f_scheduling_speed) {
    bf_port_speeds_t scheduling_speed_ =
        static_cast<bf_port_speeds_t>(sc_hlp.scheduling_speed);
    status = (BF_SPEED_NONE == sc_hlp.scheduling_speed)
                 ? trafficMgr->bfTMPortSchedDisable(dev_tgt.dev_id, port_id)
                 : trafficMgr->bfTMPortSchedEnable(
                       dev_tgt.dev_id, port_id, scheduling_speed_);
    if (BF_IN_USE == status) {
      // Skip TM ports which are sub-channels for the given speed >100G
      // and can't be set individually, but once with the parent port in
      // the appropriate group.
      LOG_DBG(
          "%s:%d %s Skip scheduling speed %d set on a sub channel: dev_id=%d, "
          "port_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          scheduling_speed_,
          dev_tgt.dev_id,
          port_id);
      status = BF_SUCCESS;
    } else if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s Port scheduling mode set failed: dev_id=%d, port_id=%d, "
          "rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
    }
  }

  return status;
}

//------------------ TM_PORT_SCHED_SHAPING

bf_status_t BfRtTMPortSchedShapingTable::tableResetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }
  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPortSchedMaxRateDefaultsGet(dev_tgt.dev_id,
                                                       port_id,
                                                       &s_hlp.pps,
                                                       &s_hlp.max_burst_size,
                                                       &s_hlp.max_rate,
                                                       &s_hlp.provisioning);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s read reset values failed: dev_id=%d, port_id=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        port_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
  }

  return status;
}

//-----------
bf_status_t BfRtTMPortSchedShapingTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPortSchedMaxRateGet(dev_tgt.dev_id,
                                               port_id,
                                               &s_hlp.pps,
                                               &s_hlp.max_burst_size,
                                               &s_hlp.max_rate,
                                               &s_hlp.provisioning);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s driver read failed: dev_id=%d, port_id=%d, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              status,
              bf_err_str(status));
    return status;
  }

  status = s_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPortSchedShapingTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_dev_port_t port_id,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  BfRtTMSchedShapingHelper s_hlp;

  auto status = s_hlp.initFields(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  bool f_max_any = (s_hlp.f_max_rate || s_hlp.f_max_burst_size);
  bool f_max_all = (s_hlp.f_max_rate && s_hlp.f_max_burst_size);
  bool f_cmn_any = (s_hlp.f_unit || s_hlp.f_provisioning);
  bool f_cmn_all = (s_hlp.f_unit && s_hlp.f_provisioning);

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // Read all values from HW if some of them are given for update
  if ((f_max_any && !(f_max_all)) || (f_cmn_any && !(f_cmn_all))) {
    status = trafficMgr->bfTMPortSchedMaxRateGet(dev_tgt.dev_id,
                                                 port_id,
                                                 &s_hlp.pps,
                                                 &s_hlp.max_burst_size,
                                                 &s_hlp.max_rate,
                                                 &s_hlp.provisioning);

    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s pre-read values failed dev_id=%d, port_id=%d, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          port_id,
          status,
          bf_err_str(status));
      return status;
    }
  }

  status = s_hlp.getFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get fields dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  status = trafficMgr->bfTMPortSchedMaxRateSet(dev_tgt.dev_id,
                                               port_id,
                                               s_hlp.pps,
                                               s_hlp.max_burst_size,
                                               s_hlp.max_rate,
                                               s_hlp.provisioning);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s driver write failed: dev_id=%d, port_id=%d, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id,
              status,
              bf_err_str(status));
  }

  return status;
}

}  // namespace bfrt
