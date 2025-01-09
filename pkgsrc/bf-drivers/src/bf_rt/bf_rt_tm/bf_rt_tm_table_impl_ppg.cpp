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

#include "bf_rt_tm_table_helper_ppg.hpp"
#include "bf_rt_tm_table_impl_ppg.hpp"

namespace bfrt {

bf_status_t BfRtTMPpgTableIntf::tableSizeGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             size_t *size) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  size_t r_size = 0;
  size_t r_usage = 0;

  if (size == nullptr) {
    return BF_INVALID_ARG;
  }

  auto status =
      this->tableGetSizeUsage(session, dev_tgt, flags, r_size, r_usage);
  if (BF_SUCCESS == status) {
    *size = r_size;
  }

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableUsageGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags,
                                              uint32_t *count) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  size_t r_size = 0;
  size_t r_usage = 0;

  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  auto status =
      this->tableGetSizeUsage(session, dev_tgt, flags, r_size, r_usage);
  if (BF_SUCCESS == status) {
    *count = r_usage;
  }

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableGetSizeUsage(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t & /*flags*/,
                                                  size_t &table_size,
                                                  size_t &table_used) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // bfrt_python info() uses only PIPE_ALL when it is called for a parent node,
  // e.g. tm.ppg.info()
  // With a single table it also allows pipe_id, e.g. tm ppg.icos.info(pipe=2)

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t wrk_tgt = dev_tgt;

  bf_status_t status = BF_UNEXPECTED;

  if (wrk_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(wrk_tgt.dev_id, &(wrk_tgt.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id);
      return status;
    }
  }

  status = this->singlePipe_validate(wrk_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  //----
  // Lock the state for add/del changes. No need in the entry lock.
  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
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
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  uint32_t u_ppg_sum = 0;
  uint32_t t_ppg_sum = 0;

  do {
    uint32_t u_ppg_cnt = 0;
    uint32_t t_ppg_cnt = 0;

    std::size_t ppg_state_size = 0;
    status = tmPpgState->stateSizeGet(wrk_tgt.pipe_id, ppg_state_size);
    if (BF_OBJECT_NOT_FOUND == status) {
      ppg_state_size = 0;
    } else if (status != BF_SUCCESS) {
      break;
    }
    // The PPG table size is common HW resource for all types of PPGs.
    status = this->tableGetUsage(session, wrk_tgt, t_ppg_cnt, u_ppg_cnt);
    if (status != BF_SUCCESS) {
      break;
    }
    if ((u_ppg_sum + u_ppg_cnt < u_ppg_sum) ||
        (t_ppg_sum + t_ppg_cnt < t_ppg_sum)) {
      status = BF_UNEXPECTED;
      break;
    }
    u_ppg_sum += u_ppg_cnt;
    t_ppg_sum += t_ppg_cnt;

    if (ppg_state_size != u_ppg_cnt) {
      LOG_TRACE(
          "%s:%d %s dev_id=%d pipe_id=%d PPG state size %zu != %u usage "
          "- some PPGs are possibly managed behind BF-RT TM",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          wrk_tgt.dev_id,
          wrk_tgt.pipe_id,
          ppg_state_size,
          u_ppg_cnt);
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // Is called for a single pipe.
    }
    status = trafficMgr->bfTMPipeGetNext(
        wrk_tgt.dev_id, wrk_tgt.pipe_id, &(wrk_tgt.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more pipes on this device
    }
  } while (BF_SUCCESS == status);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't get PPG size dev_id=%d pipe_id=%d, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              wrk_tgt.dev_id,
              wrk_tgt.pipe_id,
              status,
              bf_err_str(status));
    return status;
  }

  if ((BF_DEV_PIPE_ALL == dev_tgt.pipe_id) && (this->_table_size < t_ppg_sum)) {
    LOG_WARN(
        "%s:%d %s dev_id=%d pipe_id=%x "
        "declared size(%zu) < actual size(%u)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        wrk_tgt.dev_id,
        wrk_tgt.pipe_id,
        this->_table_size,
        t_ppg_sum);
  }

  if (u_ppg_sum > t_ppg_sum) {
    LOG_WARN("%s:%d %s dev_id=%d pipe_id=%x table usage (%u) > table size(%u)",
             __func__,
             __LINE__,
             this->table_name_get().c_str(),
             wrk_tgt.dev_id,
             wrk_tgt.pipe_id,
             u_ppg_sum,
             t_ppg_sum);
  }

  table_size = t_ppg_sum;
  table_used = u_ppg_sum;

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableGetUsage(const BfRtSession & /* session */,
                                              const bf_rt_target_t &dev_tgt,
                                              uint32_t &table_size,
                                              uint32_t &table_used) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t t_ppg_cnt = 0;
  uint32_t u_ppg_cnt = 0;

  auto status = trafficMgr->bfTMPPGTotalCntGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &t_ppg_cnt);
  if (BF_SUCCESS != status) {
    return status;
  }
  status = trafficMgr->bfTMPPGUnusedCntGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &u_ppg_cnt);
  if (BF_SUCCESS != status) {
    return status;
  }
  if (u_ppg_cnt > t_ppg_cnt) {
    return BF_UNEXPECTED;
  }
  table_size = t_ppg_cnt;
  table_used = t_ppg_cnt - u_ppg_cnt;

  return status;
}

bf_status_t BfRtTMPpgTableIntf::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPpgTableKey(this));
  if (*key_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate key",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableIntf::keyReset(BfRtTableKey *key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtTMPpgTableKey *>(key))->reset();
}

// Resets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPpgTableIntf::tableResetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_ppg_hdl /* ppg_hdl */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableIntf::tableResetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_ppg_hdl ppg_hdl,
    bf_rt_id_t data_id,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s ppg_hdl=0x%x not implemented field_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            ppg_hdl,
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMPpgTableIntf::tableResetEntry(const bf_rt_target_t &dev_tgt,
                                                bf_tm_ppg_hdl ppg_hdl,
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
  auto status =
      this->tableResetFields(dev_tgt, ppg_hdl, p_data, activeDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields for ppg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl);
    return status;
  }

  LOG_DBG("%s:%d %s ppg_hdl=0x%x need_fields=%zu, already with_values=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          ppg_hdl,
          activeDataFields.size(),
          p_data->hasValues());

  for (bf_rt_id_t data_id : activeDataFields) {
    status = this->tableResetField(dev_tgt, ppg_hdl, data_id, p_data);
    if (status != BF_SUCCESS) {
      break;
    }
  }  // for (field_id)

  return status;
}

// Sets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPpgTableIntf::tableSetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_ppg_hdl /* ppg_hdl */,
    const BfRtTMTableData & /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableIntf::tableSetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_ppg_hdl ppg_hdl,
    const BfRtTMTableData & /* qData */,
    bf_rt_id_t data_id) const {
  LOG_ERROR("%s:%d %s ppg_hdl=0x%x not implemented field_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            ppg_hdl,
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMPpgTableIntf::tableSetEntry(const bf_rt_target_t &dev_tgt,
                                              const BfRtTMTableData &p_data,
                                              bf_tm_ppg_hdl ppg_hdl) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> assignedDataFields(p_data.getAssignedFields());

  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.
  auto status =
      this->tableSetFields(dev_tgt, ppg_hdl, p_data, assignedDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields ppg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl);
    return status;
  }

  LOG_DBG(
      "%s:%d %s ppg_hdl=0x%x active_fields=%zu with_values=%zu to_assign=%zu",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      ppg_hdl,
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
      LOG_ERROR("%s:%d %s attempt to set r/o field_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                data_id);
      status = BF_INVALID_ARG;
      break;
    }
    status = this->tableSetField(dev_tgt, ppg_hdl, p_data, data_id);
    if (status != BF_SUCCESS) {
      break;
    }
  }  // for (field_id)

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableGetField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    bf_tm_ppg_hdl ppg_hdl,
    BfRtTMTableData * /* p_data */) const {
  LOG_ERROR("%s:%d %s ppg_hdl=0x%x not implemented field_id=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            ppg_hdl,
            data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMPpgTableIntf::tableGetFields(
    const bf_rt_target_t & /* dev_tgt */,
    bf_tm_ppg_hdl /* ppg_hdl */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrk_fields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrk_fields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMPpgTableIntf::tableGetEntry(const bf_rt_target_t &dev_tgt,
                                              bf_tm_ppg_hdl ppg_hdl,
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

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  auto status = this->tableGetFields(dev_tgt, ppg_hdl, p_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields ppg_hdl=0x%x, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
    return status;
  }
  LOG_DBG(
      "%s:%d %s ppg_hdl=0x%x need fields %zu of %zu; got block of %zu values",
      __func__,
      __LINE__,
      this->table_name_get().c_str(),
      ppg_hdl,
      workDataFields.size(),
      active_fields_cnt,
      p_data->hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
    status = this->tableGetField(dev_tgt, data_id, ppg_hdl, p_data);
    if (status != BF_SUCCESS) {
      break;
    }
  }
  LOG_DBG("%s:%d %s ppg_hdl=0x%x got %zu values for %zu fields",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          ppg_hdl,
          p_data->hasValues(),
          active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ppg_hdl=0x%x rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableEntryGet(const BfRtSession & /* session */,
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

  BfRtTMTableData *p_data = static_cast<BfRtTMTableData *>(data);

  // Table with actions might respond with only action id value set.
  if (!(this->actionIdApplicable()) && p_data->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s No active data fields to get",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    return BF_SUCCESS;
  }

  //---  Check both the implicit pipe_id key and the ppg_id from the key.

  const BfRtTMPpgTableKey &pKey = static_cast<const BfRtTMPpgTableKey &>(key);
  bf_tm_ppg_id_t ppg_id = 0;

  status = pKey.getId(ppg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (0 == ppg_id) {
    LOG_ERROR("%s:%d %s ppg_id=%d is not allowed",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_id);
    return BF_INVALID_ARG;
  }

  //-------------- TM PPG State
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
  bf_tm_ppg_hdl ppg_hdl = 0;

  if (this->isVolatile()) {
    // This table does not require its entry fields to be in mutually
    // consistent state. It needs only the valid ppg handler from the state.
    // This also implies the entries not deleted from this table.
    status = tmPpgState->atomicTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d no handler rc=%d(%s)",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                ppg_id,
                status,
                bf_err_str(status));
      return status;
    }
    status = this->tableGetEntry(dev_tgt, ppg_hdl, p_data);

  } else {
    LOG_DBG("%s:%d %s Get PPG state lock",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    // Both locks to ensure the entry will not change neither from this table
    // nor from other table. The state lock is optional for read if the same
    // table is responsible for delete what requires the entry_lock first.
    std::lock_guard<std::mutex> entry_mux(this->entry_lock);
    std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
    LOG_DBG("%s:%d %s Got PPG state lock",
            __func__,
            __LINE__,
            this->table_name_get().c_str());

    status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d no state, rc=%d(%s)",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                ppg_id,
                status,
                bf_err_str(status));
      return status;
    }
    status = this->tableGetEntry(dev_tgt, ppg_hdl, p_data);
  }

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableEntryMod(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /* flags */,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  const BfRtTMTableData &p_data = static_cast<const BfRtTMTableData &>(data);

  // Some actions may be applied without data fields.
  // If it is not possible for the table it will raise error later.
  if (!(this->actionIdApplicable()) && p_data.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  //---  Check both the implicit pipe_id key and the ppg_id from the key.

  const BfRtTMPpgTableKey &pKey = static_cast<const BfRtTMPpgTableKey &>(key);
  bf_tm_ppg_id_t ppg_id = 0;

  status = pKey.getId(ppg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (0 == ppg_id) {
    LOG_ERROR("%s:%d %s ppg_id=%d is not allowed",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_id);
    return BF_INVALID_ARG;
  }

  //-------------- TM PPG State
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
  bf_tm_ppg_hdl ppg_hdl = 0;

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> entry_mux(this->entry_lock);
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              ppg_id,
              status,
              bf_err_str(status));
    return status;
  }
  return this->tableSetEntry(dev_tgt, p_data, ppg_hdl);
}

bf_status_t BfRtTMPpgTableIntf::tableEntryDel(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /* flags */,
                                              const BfRtTableKey &key) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  const BfRtTMPpgTableKey &pKey = static_cast<const BfRtTMPpgTableKey &>(key);
  bf_tm_ppg_id_t ppg_id = 0;

  status = pKey.getId(ppg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (0 == ppg_id) {
    LOG_ERROR("%s:%d %s ppg_id=%d is not allowed",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_id);
    return BF_INVALID_ARG;
  }

  //-------------- TM PPG State
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
  bf_tm_ppg_hdl ppg_hdl = 0;

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> entry_mux(this->entry_lock);
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d, get state rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              ppg_id,
              status,
              bf_err_str(status));
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPPGFree(dev_tgt.dev_id, ppg_hdl);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x "
        "free PPG failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_id,
        ppg_hdl,
        status,
        bf_err_str(status));
    return status;
  }

  status = tmPpgState->stateTMPpgDel(dev_tgt.pipe_id, ppg_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x, "
        "delete state failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_id,
        ppg_hdl,
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x deleted",
          __func__,
          __LINE__,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_id,
          ppg_hdl);

  // Delete the PPG counter state
  bf_tm_ppg_id_t ppg_cnt_id = 0;
  status = trafficMgr->bfTMPPGNrGet(dev_tgt.dev_id, ppg_hdl, &ppg_cnt_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_hdl=0x%x "
        "failed to fetch internal ppg id, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_hdl,
        status,
        bf_err_str(status));
    return status;
  }

  status = tmPpgState->stateTMPpgCntDel(dev_tgt.pipe_id, ppg_cnt_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_cnt_id=%d "
        "delete counter state failed, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_cnt_id,
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_counter_id=%d deleted",
          __func__,
          __LINE__,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_cnt_id);

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableEntryGetFirst(
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
  bf_tm_ppg_id_t ppg_id = 0;
  bf_tm_ppg_hdl ppg_hdl = 0;

  status = tmPpgState->atomicTMPpgGetFirst(dev_tgt.pipe_id, ppg_id, ppg_hdl);
  if (BF_SUCCESS != status) {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d can't get first PPG rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            status,
            bf_err_str(status));
    return status;
  }

  BfRtTMPpgTableKey *pKey = static_cast<BfRtTMPpgTableKey *>(key);
  pKey->setId(ppg_id);

  // TM PPG state and entry lock are inside, so if the first ppg_id
  // disappears due to race then the error will be reported.
  return this->tableEntryGet(session, dev_tgt, flags, *pKey, data);
}

bf_status_t BfRtTMPpgTableIntf::tableEntryGetNext_n(
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

  auto status = this->singlePipe_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  if (key_data_pairs == nullptr || num_returned == nullptr) {
    return BF_INVALID_ARG;
  }

  const BfRtTMPpgTableKey &pKey = static_cast<const BfRtTMPpgTableKey &>(key);
  bf_tm_ppg_id_t ppg_id = 0;

  status = pKey.getId(ppg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (0 == ppg_id) {
    LOG_ERROR("%s:%d %s ppg_id=%d is not allowed",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_id);
    return BF_INVALID_ARG;
  }

  LOG_DBG("%s:%d %s getting %d entries after ppg_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          cnt,
          ppg_id);

  // The table and state lock are not set over all entries' read,
  // but each of the entries does lock the table to read all its fields
  // as one consistent entry.

  //-------------- TM PPG State
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

  bf_tm_ppg_id_t from_ppg_id = 0;
  bf_tm_ppg_hdl ppg_hdl = 0;

  uint32_t i = 0;

  for (; i < cnt; i++) {
    from_ppg_id = ppg_id;
    status = tmPpgState->atomicTMPpgGetNext(
        dev_tgt.pipe_id, from_ppg_id, ppg_id, ppg_hdl);

    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;  // No more entries
      break;
    }
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d dev_id=%d pipe_id=%d can't get PPG next to ppg_id=%d",
                __func__,
                __LINE__,
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                from_ppg_id);
      break;
    }
    LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_id=%d next is ppg_id=%d",
            __func__,
            __LINE__,
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            from_ppg_id,
            ppg_id);

    auto curr_key =
        static_cast<BfRtTMPpgTableKey *>((*key_data_pairs)[i].first);
    auto curr_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

    curr_key->setId(ppg_id);

    if (this->isVolatile()) {
      status = tmPpgState->atomicTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d "
            "ppg_hdl=0x%x, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            ppg_id,
            ppg_hdl,
            status,
            bf_err_str(status));
        return status;
      }
      status = this->tableGetEntry(dev_tgt, ppg_hdl, curr_data);
    } else {
      LOG_DBG("%s:%d %s Get PPG state lock",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
      std::lock_guard<std::mutex> entry_mux(this->entry_lock);
      std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
      LOG_DBG("%s:%d %s Got PPG state lock",
              __func__,
              __LINE__,
              this->table_name_get().c_str());

      status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d "
            "ppg_hdl=0x%x, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            ppg_id,
            ppg_hdl,
            status,
            bf_err_str(status));
        return status;
      }
      status = this->tableGetEntry(dev_tgt, ppg_hdl, curr_data);
    }
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d can't get next entry ppg_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                ppg_id);
      break;
    }
    LOG_DBG(
        "%s:%d dev_id=%d pipe_id=%d after ppg_id=%d "
        "got ppg_id=%d (%d of %d)",
        __func__,
        __LINE__,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        from_ppg_id,
        ppg_id,
        i + 1,
        cnt);
  }  // for cnt

  LOG_DBG("%s:%d %s got %d of %d entries",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          i,
          cnt);

  *num_returned = i;
  return status;
}

bf_status_t BfRtTMPpgTableIntf::tablePurge(const BfRtSession & /*session*/,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  bf_status_t status = BF_UNEXPECTED;

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

  //-------------- TM PPG State
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_wrk.dev_id, this->programNameGet());
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d ERROR device state dev_id=%d, program='%s'",
              __func__,
              __LINE__,
              dev_wrk.dev_id,
              this->programNameGet().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto tmPpgState = device_state->tmPpgState.getStateObj();

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> entry_mux(this->entry_lock);
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  bf_tm_ppg_id_t ppg_id = 0;
  bf_tm_ppg_hdl ppg_hdl = 0;

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d delete all entries",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);
    status = tmPpgState->stateTMPpgGetFirst(dev_wrk.pipe_id, ppg_id, ppg_hdl);
    if (BF_OBJECT_NOT_FOUND == status) {
      LOG_TRACE("%s:%d %s dev_id=%d pipe_id=%d no entries to delete",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id,
                dev_wrk.pipe_id);
    } else if (BF_SUCCESS != status) {
      LOG_TRACE(
          "%s:%d dev_id=%d pipe_id=%d can't get first PPG to delete, rc=%d",
          __func__,
          __LINE__,
          dev_wrk.dev_id,
          dev_wrk.pipe_id,
          status);
      return status;
    }

    bf_tm_ppg_id_t next_ppg_id = 0;
    uint32_t cnt_del = 0;

    while (BF_SUCCESS == status) {
      status = trafficMgr->bfTMPPGFree(dev_wrk.dev_id, ppg_hdl);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x, "
            "delete failed, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id,
            ppg_id,
            ppg_hdl,
            status,
            bf_err_str(status));
        return status;
      }
      status = tmPpgState->stateTMPpgDel(dev_wrk.pipe_id, ppg_id);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x, "
            "delete state failed, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id,
            ppg_id,
            ppg_hdl,
            status,
            bf_err_str(status));
        return status;
      }
      LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_id=%d deleted (%d)",
              __func__,
              __LINE__,
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              ppg_id,
              cnt_del);

      // Delete the PPG counter state
      bf_tm_ppg_id_t ppg_cnt_id = 0;
      status = trafficMgr->bfTMPPGNrGet(dev_wrk.dev_id, ppg_hdl, &ppg_cnt_id);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_hdl=0x%x "
            "failed to fetch internal ppg id, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id,
            ppg_hdl,
            status,
            bf_err_str(status));
        return status;
      }

      status = tmPpgState->stateTMPpgCntDel(dev_wrk.pipe_id, ppg_cnt_id);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s dev_id=%d pipe_id=%d ppg_cnt_id=%d "
            "delete counter state failed, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id,
            ppg_cnt_id,
            status,
            bf_err_str(status));
        return status;
      }
      LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_counter_id=%d deleted",
              __func__,
              __LINE__,
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              ppg_cnt_id);

      cnt_del++;

      status = tmPpgState->stateTMPpgGetNext(
          dev_wrk.pipe_id, ppg_id, next_ppg_id, ppg_hdl);
      if (BF_OBJECT_NOT_FOUND == status) {
        status = BF_SUCCESS;  // No more entries
        break;
      }
      if (BF_SUCCESS != status) {
        LOG_TRACE("%s:%d dev_id=%d pipe_id=%d can't get PPG next to %d, rc=%d",
                  __func__,
                  __LINE__,
                  dev_wrk.dev_id,
                  dev_wrk.pipe_id,
                  ppg_id,
                  status);
      }
      ppg_id = next_ppg_id;
    }  // loop over ppgs on pipe_id

    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;  // no ppgs on the pipe
    }
    LOG_TRACE("%s:%d %s dev_id=%d pipe_id=%d deleted %d entries, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              cnt_del,
              status);
    if (BF_SUCCESS != status) {
      break;
    }
    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // Is called for a single pipe reset.
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));

  } while (BF_SUCCESS == status);  // loop over pipes

  if (BF_OBJECT_NOT_FOUND == status) {
    status = BF_SUCCESS;  // no more pipes
  }

  return status;
}

bf_status_t BfRtTMPpgTableIntf::tableReset(const BfRtSession & /*session*/,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  bf_status_t status = BF_UNEXPECTED;

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

  //-------------- TM PPG State
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

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> entry_mux(this->entry_lock);
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  bf_tm_ppg_id_t ppg_id = 0;
  bf_tm_ppg_hdl ppg_hdl = 0;

  BfRtTMTableData entry_Data(this);

  do {
    LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d reset entries",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_wrk.dev_id,
            dev_wrk.pipe_id);
    status = tmPpgState->stateTMPpgGetFirst(dev_tgt.pipe_id, ppg_id, ppg_hdl);
    if (BF_OBJECT_NOT_FOUND == status) {
      LOG_TRACE("%s:%d %s dev_id=%d pipe_id=%d no entries to reset",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id,
                dev_wrk.pipe_id);
    } else if (BF_SUCCESS != status) {
      LOG_TRACE(
          "%s:%d dev_id=%d pipe_id=%d can't get first PPG to reset, rc=%d",
          __func__,
          __LINE__,
          dev_wrk.dev_id,
          dev_wrk.pipe_id,
          status);
      return status;
    }

    bf_tm_ppg_id_t next_ppg_id = 0;
    uint32_t cnt_reset = 0;

    while (BF_SUCCESS == status) {
      status = this->tableResetEntry(dev_wrk, ppg_hdl, &entry_Data);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d can't reset ppg_id=%d, rc=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_wrk.dev_id,
                  dev_wrk.pipe_id,
                  ppg_id,
                  status);
        return status;
      }
      status = this->tableSetEntry(dev_wrk, entry_Data, ppg_hdl);
      if (BF_SUCCESS == status) {
        cnt_reset++;
        LOG_DBG("%s:%d dev_id=%d pipe_id=%d ppg_id=%d ppg_hdl=0x%x reset (%d)",
                __func__,
                __LINE__,
                dev_wrk.dev_id,
                dev_wrk.pipe_id,
                ppg_id,
                ppg_hdl,
                cnt_reset);
      } else if (BF_OBJECT_NOT_FOUND != status) {
        // Should't happen, unless some other thread besides BFRT.
        LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d can't update ppg_id=%d, rc=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_wrk.dev_id,
                  dev_wrk.pipe_id,
                  ppg_id,
                  status);
        return status;
      }

      status = tmPpgState->stateTMPpgGetNext(
          dev_tgt.pipe_id, ppg_id, next_ppg_id, ppg_hdl);
      if (BF_OBJECT_NOT_FOUND == status) {
        status = BF_SUCCESS;  // No more entries
        break;
      }
      if (BF_SUCCESS != status) {
        LOG_TRACE("%s:%d dev_id=%d pipe_id=%d can't get PPG next to %d, rc=%d",
                  __func__,
                  __LINE__,
                  dev_wrk.dev_id,
                  dev_wrk.pipe_id,
                  ppg_id,
                  status);
      }
      ppg_id = next_ppg_id;
    }  // loop over ppgs on pipe_id

    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;  // no ppgs on the pipe
    }
    LOG_TRACE("%s:%d %s dev_id=%d pipe_id=%d reset %d entriesi, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id,
              cnt_reset,
              status);
    if (BF_SUCCESS != status) {
      break;
    }
    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // Is called for a single pipe reset.
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));
  } while (BF_SUCCESS == status);  // loop over pipes

  if (BF_OBJECT_NOT_FOUND == status) {
    status = BF_SUCCESS;  // no more pipes
  }

  return status;
}

//----------- TM_PPG_CFG table

bf_status_t BfRtTMPpgCfgTable::tableGetActionFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_ppg_hdl ppg_hdl,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  bf_rt_id_t do_action_id = 0;
  bf_rt_id_t mirror_port_action_id = 0;
  bf_rt_id_t dev_port_action_id = 0;
  bf_dev_port_t ppg_port_id = 0;

  bf_status_t status = this->tableGetActionPort(dev_tgt,
                                                ppg_hdl,
                                                *p_data,
                                                dev_port_action_id,
                                                mirror_port_action_id,
                                                do_action_id,
                                                ppg_port_id);
  if (BF_SUCCESS != status) {
    return status;
  }

  // TM Mirror port is not a valid 'local' port.
  bool is_mirror_port =
      !(LOCAL_PORT_VALIDATE(DEV_PORT_TO_LOCAL_PORT(ppg_port_id)));

  bf_rt_id_t new_action_id =
      (is_mirror_port) ? mirror_port_action_id : dev_port_action_id;

  if ((do_action_id == 0 && wrk_fields.empty()) ||
      (do_action_id != new_action_id)) {
    LOG_DBG("%s:%d %s reset Get entry action %d to action %d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            do_action_id,
            new_action_id);
    p_data->reset(new_action_id);
    wrk_fields = p_data->getActiveFields();
  }

  if (new_action_id == mirror_port_action_id) {
    return status;
  }

  //--- Process 'dev_port' action data fields.
  bf_rt_id_t f_dev_port = 0;
  status = BfRtTMPpgHelper::popWorkField(
      *this, "dev_port", dev_port_action_id, wrk_fields, f_dev_port);
  if (BF_SUCCESS == status && f_dev_port) {
    status = p_data->setValue(f_dev_port, static_cast<uint64_t>(ppg_port_id));
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s get dev_port of PPG ppg_hdl=0x%x falied, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
    return status;
  }
  //---
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bool pfc_enable = false;
  bf_rt_id_t f_pfc_enable = 0;

  status = BfRtTMPpgHelper::popWorkField(
      *this, "pfc_enable", dev_port_action_id, wrk_fields, f_pfc_enable);
  if (BF_SUCCESS == status && f_pfc_enable) {
    status = trafficMgr->bfTMPPGLosslessTreatmentGet(
        dev_tgt.dev_id, ppg_hdl, &pfc_enable);
    if (BF_SUCCESS == status) {
      status = p_data->setValue(f_pfc_enable, pfc_enable);
    }
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s get PFC lossless status of PPG ppg_hdl=0x%x falied, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ppg_hdl,
        status,
        bf_err_str(status));
    return status;
  }
  //---
  uint32_t pfc_skid_max_cells = 0;
  bf_rt_id_t f_pfc_skid_max_cells = 0;
  status = BfRtTMPpgHelper::popWorkField(*this,
                                         "pfc_skid_max_cells",
                                         dev_port_action_id,
                                         wrk_fields,
                                         f_pfc_skid_max_cells);
  if (BF_SUCCESS == status && f_pfc_skid_max_cells) {
    status = trafficMgr->bfTMPPGSkidLimitGet(
        dev_tgt.dev_id, ppg_hdl, &pfc_skid_max_cells);
    if (BF_SUCCESS == status) {
      status = p_data->setValue(f_pfc_skid_max_cells,
                                static_cast<uint64_t>(pfc_skid_max_cells));
    }
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s get pfc_skid_max_cells for PPG ppg_hdl=0x%x falied, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ppg_hdl,
        status,
        bf_err_str(status));
    return status;
  }

  return status;
}

//------
bf_status_t BfRtTMPpgCfgTable::tableAddActionPort(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTableData &p_data,
    bf_dev_port_t &ppg_port_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  //--- Prepare action ids

  bf_rt_id_t do_action_id = 0;
  status = p_data.actionIdGet(&do_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  bf_rt_id_t dev_port_action_id = 0;
  std::string action_name("dev_port");
  status = this->actionIdGet(action_name, &dev_port_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s no action %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str());
    return status;
  }

  bf_rt_id_t mirror_port_action_id = 0;
  action_name = "mirror_port";
  status = this->actionIdGet(action_name, &mirror_port_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s no action %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str());
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (do_action_id == mirror_port_action_id) {
    // Get the Mirror port through its DPG handle
    bf_dev_port_t mirror_port_id = 0;
    bf_tm_ppg_hdl dpg_hdl = 0;

    status = trafficMgr->bfTMPPGMirrorPortHandleGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, &dpg_hdl);
    if (BF_SUCCESS == status) {
      status =
          trafficMgr->bfTMPPGPortGet(dev_tgt.dev_id, dpg_hdl, &mirror_port_id);
      if (BF_SUCCESS == status) {
        ppg_port_id = mirror_port_id;
        return status;
      }
    }
  } else if (do_action_id == dev_port_action_id) {
    bf_rt_id_t f_dev_port = 0;
    uint64_t external_port_id = 0;

    status = this->dataFieldIdGet("dev_port", dev_port_action_id, &f_dev_port);
    if (BF_SUCCESS == status) {
      status = p_data.getValue(f_dev_port, &external_port_id);
    }
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s can't get mandatory dev_port value, rc=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                status);
      return status;
    }
    bf_dev_port_t port_id = static_cast<bf_dev_port_t>(external_port_id);
    if (!(DEV_PORT_VALIDATE(port_id)) ||
        (DEV_PORT_TO_PIPE(port_id) != dev_tgt.pipe_id)) {
      LOG_ERROR("%s:%d %s dev_port=%d is not on pipe_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                port_id,
                dev_tgt.pipe_id);
      return BF_INVALID_ARG;
    }
    ppg_port_id = static_cast<bf_dev_port_t>(external_port_id);
    return BF_SUCCESS;
  } else {
    LOG_ERROR("%s:%d %s invalid action_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              do_action_id);
    status = BF_INVALID_ARG;
  }

  return status;
}

//----
bf_status_t BfRtTMPpgCfgTable::tableGetActionPort(
    const bf_rt_target_t &dev_tgt,
    bf_tm_ppg_hdl ppg_hdl,
    const BfRtTMTableData &p_data,
    bf_rt_id_t &dev_port_action_id,
    bf_rt_id_t &mirror_port_action_id,
    bf_rt_id_t &do_action_id,
    bf_dev_port_t &ppg_port_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  //--- Prepare action ids

  do_action_id = 0;
  status = p_data.actionIdGet(&do_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  dev_port_action_id = 0;
  std::string action_name("dev_port");
  status = this->actionIdGet(action_name, &dev_port_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s no action %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str());
    return status;
  }

  mirror_port_action_id = 0;
  action_name = "mirror_port";
  status = this->actionIdGet(action_name, &mirror_port_action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s no action %s",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              action_name.c_str());
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  ppg_port_id = 0;

  status = trafficMgr->bfTMPPGPortGet(dev_tgt.dev_id, ppg_hdl, &ppg_port_id);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't read ppg_hdl=0x%x, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
    return status;
  }

  // The ppg_hdl should be obtained from state map by <pipe_id, ppg_id>
  // Also the ppg_hdl itself must have the same pipe_id encoded.
  // Anyway, let's double check that the current dev_port
  // is at the pipe_id requested.
  if (dev_tgt.pipe_id != (DEV_PORT_TO_PIPE(ppg_port_id))) {
    LOG_ERROR("%s:%d %s ppg_hdl=0x%x dev_port=%d not on pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              ppg_port_id,
              dev_tgt.pipe_id);
    return BF_UNEXPECTED;
  }

  return status;
}

//---
bf_status_t BfRtTMPpgCfgTable::tableSetActionFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_ppg_hdl ppg_hdl,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_rt_id_t do_action_id = 0;
  bf_rt_id_t mirror_port_action_id = 0;
  bf_rt_id_t dev_port_action_id = 0;
  bf_dev_port_t ppg_port_id = 0;

  bf_status_t status = this->tableGetActionPort(dev_tgt,
                                                ppg_hdl,
                                                p_data,
                                                dev_port_action_id,
                                                mirror_port_action_id,
                                                do_action_id,
                                                ppg_port_id);
  if (BF_SUCCESS != status) {
    return status;
  }

  // TM Mirror port is not a valid 'local' port.
  bool is_mirror_port =
      !(LOCAL_PORT_VALIDATE(DEV_PORT_TO_LOCAL_PORT(ppg_port_id)));

  bf_rt_id_t new_action_id =
      (is_mirror_port) ? mirror_port_action_id : dev_port_action_id;

  if (do_action_id != new_action_id) {
    LOG_ERROR("%s:%d %s ppg_hdl=0x%x action %d can't be set",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              do_action_id);
    return BF_INVALID_ARG;
  }

  if (do_action_id == mirror_port_action_id) {
    return status;  // Done for a Mirror Port
  }

  // Check the expected dev_port, if given, against the current one.
  bf_rt_id_t f_dev_port = 0;

  status = BfRtTMPpgHelper::popWorkField(
      *this, "dev_port", dev_port_action_id, wrk_fields, f_dev_port);
  if (BF_SUCCESS == status && f_dev_port) {
    uint64_t expected_port_id = 0;
    status = p_data.getValue(f_dev_port, &expected_port_id);
    if (BF_SUCCESS != status) {
      return status;
    }
    if (!(DEV_PORT_VALIDATE(expected_port_id)) ||
        (DEV_PORT_TO_PIPE(expected_port_id) != dev_tgt.pipe_id)) {
      LOG_ERROR("%s:%d %s pipe_id=%d incorrect dev_port=%" PRIu64,
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.pipe_id,
                expected_port_id);
      return BF_INVALID_ARG;
    }
    if (ppg_port_id != static_cast<bf_dev_port_t>(expected_port_id)) {
      LOG_ERROR(
          "%s:%d %s pipe_id=%d ppg_hdl=0x%x is at dev_port=%d "
          "instead of %" PRIu64,
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.pipe_id,
          ppg_hdl,
          ppg_port_id,
          expected_port_id);
      return BF_INVALID_ARG;
    }
  }

  //---
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  bf_rt_id_t f_pfc_enable = 0;
  status = BfRtTMPpgHelper::popWorkField(
      *this, "pfc_enable", dev_port_action_id, wrk_fields, f_pfc_enable);
  if (BF_SUCCESS == status && f_pfc_enable) {
    bool new_lossless = false;
    bool old_lossless = false;
    status = p_data.getValue(f_pfc_enable, &new_lossless);
    if (BF_SUCCESS == status) {
      status = trafficMgr->bfTMPPGLosslessTreatmentGet(
          dev_tgt.dev_id, ppg_hdl, &old_lossless);
      if (BF_SUCCESS == status && old_lossless != new_lossless) {
        if (new_lossless) {
          status = trafficMgr->bfTMPPGLosslessTreatmentEnable(dev_tgt.dev_id,
                                                              ppg_hdl);
        } else {
          status = trafficMgr->bfTMPPGLosslessTreatmentDisable(dev_tgt.dev_id,
                                                               ppg_hdl);
        }
      }
    }
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s ppg_hdl=0x%x PFC mode set falied, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
    return status;
  }

  //---
  bf_rt_id_t f_pfc_skid_max_cells = 0;
  status = BfRtTMPpgHelper::popWorkField(*this,
                                         "pfc_skid_max_cells",
                                         dev_port_action_id,
                                         wrk_fields,
                                         f_pfc_skid_max_cells);
  if (BF_SUCCESS == status && f_pfc_skid_max_cells) {
    uint64_t pfc_skid_max_cells = 0;
    status = p_data.getValue(f_pfc_skid_max_cells, &pfc_skid_max_cells);
    if (BF_SUCCESS == status) {
      status = trafficMgr->bfTMPPGSkidLimitSet(
          dev_tgt.dev_id, ppg_hdl, pfc_skid_max_cells);
    }
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s ppg_hdl=0x%x pfc_skid_max_cells set falied, rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              ppg_hdl,
              status,
              bf_err_str(status));
    return status;
  }

  return status;
}

//---
bf_status_t BfRtTMPpgCfgTable::tableGetField(const bf_rt_target_t &dev_tgt,
                                             bf_rt_id_t data_id,
                                             bf_tm_ppg_hdl ppg_hdl,
                                             BfRtTMTableData *p_data) const {
  bf_status_t status = BF_SUCCESS;

  //---- PPG counter id
  bf_rt_id_t ctr_field_id = 0;
  status = this->dataFieldIdGet("ppg_counter_id", &ctr_field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Failed to get the field id for ppg_counter_id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  if (data_id == ctr_field_id) {
    bf_tm_ppg_id_t ppg_cnt_id = 0;
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    status = trafficMgr->bfTMPPGNrGet(dev_tgt.dev_id, ppg_hdl, &ppg_cnt_id);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s dev_id=%d pipe_id=%d ppg_hdl=0x%x "
          "failed to fetch internal ppg id, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_hdl,
          status,
          bf_err_str(status));
      return status;
    }
    status = p_data->setValue(data_id, static_cast<uint64_t>(ppg_cnt_id));
  }
  return status;
}

//---
bf_status_t BfRtTMPpgCfgTable::tableGetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_ppg_hdl ppg_hdl,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_UNEXPECTED;

  do {
    //---- Action id and dependent fields: dev_port, PPC.
    status = this->tableGetActionFields(dev_tgt, ppg_hdl, p_data, wrk_fields);
    if (BF_SUCCESS != status) {
      break;
    }

    //---- iCoS fields
    uint8_t icos_mask = 0;
    status = BfRtTMPpgIcosHelper::getFieldsIcos(
        dev_tgt, *this, ppg_hdl, wrk_fields, p_data, icos_mask);
    if (BF_SUCCESS != status) {
      break;
    }

    //---- Shared Pool fields
    BfRtTMPpgBufferHelper ppg_helper;
    status =
        ppg_helper.getFieldsBuffer(dev_tgt, *this, ppg_hdl, wrk_fields, p_data);

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get PPG fields dev_id=%d pipe_id=%d ppg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              ppg_hdl);
  }
  return status;
}

//---
bf_status_t BfRtTMPpgCfgTable::tableSetFields(
    const bf_rt_target_t &dev_tgt,
    bf_tm_ppg_hdl ppg_hdl,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  //---- Action id and dependent fields: dev_port, PFC.
  auto status =
      this->tableSetActionFields(dev_tgt, ppg_hdl, p_data, wrk_fields);
  if (BF_SUCCESS == status) {
    //---- iCoS fields
    uint8_t new_icos = 0;  // iCoS bitmask resulting value from the data object.
    uint8_t new_mask = 0;  // iCoS bits changed in the bitmask.

    status = BfRtTMPpgIcosHelper::getValuesIcos(
        *this, p_data, wrk_fields, new_icos, new_mask);
    if (BF_SUCCESS == status) {
      status = BfRtTMPpgIcosHelper::writeValuesIcos(
          dev_tgt, *this, ppg_hdl, new_icos, new_mask);
    }
  }

  if (BF_SUCCESS == status) {
    //-- Buffer fields
    BfRtTMPpgBufferHelper ppg_helper;
    status =
        ppg_helper.setFieldsBuffer(dev_tgt, *this, p_data, wrk_fields, ppg_hdl);
  }

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set PPG fields dev_id=%d pipe_id=%d ppg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              ppg_hdl);
  }
  return status;
}

//---
bf_status_t BfRtTMPpgCfgTable::tableClear(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->tablePurge(session, dev_tgt, flags);
}

//---
bf_status_t BfRtTMPpgCfgTable::tableEntryAdd(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /* flags */,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  const BfRtTMTableData &p_data = static_cast<const BfRtTMTableData &>(data);

  // The ppg.cfg table allows empty data object with 'mirror_port' ADD action.
  // This check will be done later by tableAddEntry().

  const BfRtTMPpgTableKey &pKey = static_cast<const BfRtTMPpgTableKey &>(key);
  bf_tm_ppg_id_t ppg_id = 0;

  bf_status_t status = pKey.getId(ppg_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get key values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  if (0 == ppg_id || dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_ERROR("%s:%d %s dev_id=%d pipe_id=%d or ppg_id=%d incorrect",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              ppg_id);
    return BF_INVALID_ARG;
  }

  // TM PPG locks with state change are inside:
  return this->tableAddEntry(dev_tgt, p_data, ppg_id);
}

bf_status_t BfRtTMPpgCfgTable::tableAddEntry(const bf_rt_target_t &dev_tgt,
                                             const BfRtTMTableData &p_data,
                                             bf_tm_ppg_id_t ppg_id) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  bf_status_t status = BF_UNEXPECTED;

  // The port_id is mandatory for AddEntry to allocate a new PPG.
  // It is either an explicit external port with 'dev_port' action,
  // or it is made from pipe_id with 'mirror_port' action.
  bf_dev_port_t port_id = 0;

  status = this->tableAddActionPort(dev_tgt, p_data, port_id);
  if (BF_SUCCESS != status) {
    return status;
  }

  //-------------- TM PPG State lock.
  // The entry_lock should be done by the caller.
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
  bf_tm_ppg_hdl ppg_hdl = 0;

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> entry_mux(this->entry_lock);
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  // Check there is no ppg_id yet.
  status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
  if (BF_OBJECT_NOT_FOUND != status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d "
        "%s ppg_hdl=0x%x, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_id,
        (BF_SUCCESS == status) ? "already exists" : "lookup error",
        ppg_hdl,
        status,
        bf_err_str(status));
    return ((BF_SUCCESS == status) ? BF_ALREADY_EXISTS : status);
  }

  // Allocate new PPG on the port.
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  status = trafficMgr->bfTMPPGAllocate(dev_tgt.dev_id, port_id, &ppg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s Can't allocate a new PPG for "
        "dev_id=%d pipe_id=%d dev_port=%d ppg_id=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        port_id,
        ppg_id,
        status,
        bf_err_str(status));
    return status;
  }
  LOG_DBG(
      "%s:%d dev_id=%d pipe_id=%d port_id=%d ppg_hdl=0x%x ppg_id=%d allocated",
      __func__,
      __LINE__,
      dev_tgt.dev_id,
      dev_tgt.pipe_id,
      port_id,
      ppg_hdl,
      ppg_id);

  // The new PPG might be left without fields assigned
  // with default TM settings and empty iCoS mask.
  //
  // The dev_port data field is already processed and removed from wrk_fields.
  //
  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.

  status = this->tableSetEntry(dev_tgt, p_data, ppg_hdl);

  // Keep new (ppg_id <-> ppg_hdl) in the state object.
  if (BF_SUCCESS == status) {
    status = tmPpgState->stateTMPpgAdd(dev_tgt.pipe_id, ppg_id, ppg_hdl);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d "
          "ppg_hdl=0x%x state set failed, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_id,
          ppg_hdl,
          status,
          bf_err_str(status));
    }
  }
  if (BF_SUCCESS != status) {
    // Roll back on error deleting the new entry.
    auto status_2 = trafficMgr->bfTMPPGFree(dev_tgt.dev_id, ppg_hdl);
    if (BF_SUCCESS != status_2) {
      LOG_ERROR(
          "%s:%d %s Can't roll back the new PPG allocation for "
          "dev_id=%d pipe_id=%d dev_port=%d "
          "ppg_hdl=0x%x ppg_id=%d, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          port_id,
          ppg_hdl,
          ppg_id,
          status_2,
          bf_err_str(status_2));
    } else {
      LOG_TRACE(
          "%s:%d dev_id=%d pipe_id=%d port_id=%d ppg_hdl=0x%x ppg_id=%d "
          "deleted to clean after its new setup failed.",
          __func__,
          __LINE__,
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          port_id,
          ppg_hdl,
          ppg_id);
    }
    return status;
  }

  // Fetch the internal ppg number allocated by the TM driver
  // This is exposed as ppg counter id for users in order
  // for them to access an entry from tm.counter.ppg table
  bf_tm_ppg_id_t ppg_cnt_id = 0;
  auto cnt_status =
      trafficMgr->bfTMPPGNrGet(dev_tgt.dev_id, ppg_hdl, &ppg_cnt_id);
  if (BF_SUCCESS != cnt_status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d pipe_id=%d ppg_id=%d "
        "ppg_hdl=0x%x failed to fetch internal ppg id, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        ppg_id,
        ppg_hdl,
        cnt_status,
        bf_err_str(cnt_status));
  } else {
    cnt_status =
        tmPpgState->stateTMPpgCntAdd(dev_tgt.pipe_id, ppg_cnt_id, ppg_id);
    if (BF_SUCCESS != cnt_status) {
      LOG_ERROR(
          "%s:%d %s dev_id=%d pipe_id=%d ppg_counter_id=%d "
          "ppg_id=%d state set failed, rc=%d(%s)",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_cnt_id,
          ppg_id,
          cnt_status,
          bf_err_str(cnt_status));
    }
  }

  return status;
}

}  // namespace bfrt
