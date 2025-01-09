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

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

bf_status_t BfRtTMTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  LOG_DBG("%s:%d %s action_id=NONE data_fields=NONE",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtTMTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate data",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  LOG_DBG("%s:%d %s action_id=NONE data_fields_cnt=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          fields.size());
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtTMTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_ERROR("%s:%d %s Failed to allocate data",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::dataReset(BfRtTableData *data) const {
  LOG_DBG("%s:%d %s action_id=NONE",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtTMTableData *>(data))->reset();
}

bf_status_t BfRtTMTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  LOG_DBG("%s:%d %s action_id=%u data_fields=NONE",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_id);
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtTMTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    LOG_ERROR("%s:%d %s Can't allocate data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  LOG_DBG("%s:%d %s action_id=%u data_fields_cnt=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_id,
          fields.size());
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtTMTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    LOG_ERROR("%s:%d %s Can't allocate data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::keyFirst(bf_rt_target_t & /* dev_tgt */,
                                  BfRtTableKey * /* key */) const {
  LOG_TRACE("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMTable::keyNext(bf_rt_target_t & /* dev_tgt */,
                                 const BfRtTableKey & /* key */,
                                 BfRtTableKey * /* key_next */) const {
  LOG_TRACE("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMTable::tableEntryGetFirst(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            BfRtTableKey *key,
                                            BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  bf_rt_target_t dev_wrk = dev_tgt;

  auto status = this->keyFirst(dev_wrk, key);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Unable to obtain the first key value, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }

  return this->tableEntryGet(session, dev_wrk, flags, *key, data);
}

bf_status_t BfRtTMTable::tableEntryGetNext_n(const BfRtSession &session,
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

  bf_status_t status = BF_UNEXPECTED;
  const BfRtTableKey *prev_key = &key;
  bf_rt_target_t dev_wrk = dev_tgt;
  uint32_t i = 0;

  for (; i < cnt; i++) {
    auto curr_key = (*key_data_pairs)[i].first;
    auto curr_data = (*key_data_pairs)[i].second;

    status = this->keyNext(dev_wrk, *prev_key, curr_key);
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more entries.
    } else if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Unable to obtain %d next key, rc=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                i + 1,
                status);
      break;
    }

    status = this->tableEntryGet(session, dev_wrk, flags, *curr_key, curr_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get entry %d of %d, rc=%d(%s)",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                i + 1,
                cnt,
                status,
                bf_err_str(status));
      break;
    }
    prev_key = curr_key;
  }
  LOG_DBG("%s:%d %s Got %d of %d entries, rc=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          i,
          cnt,
          status);

  *num_returned = i;
  return status;
}

bf_status_t BfRtTMTable::dataReset(const bf_rt_id_t &action_id,
                                   BfRtTableData *data) const {
  LOG_DBG("%s:%d %s action_id=%u",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_id);
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtTMTableData *>(data))->reset(action_id);
}

bf_status_t BfRtTMTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                   const bf_rt_id_t &action_id,
                                   BfRtTableData *data) const {
  LOG_DBG("%s:%d %s action_id=%u data_fields_cnt=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          action_id,
          fields.size());
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtTMTableData *>(data))->reset(action_id);
}

bf_status_t BfRtTMTable::tableGetDefaultField(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    BfRtTMTableData * /* t_data */) const {
  // A fixed function table field might have no default value.
  LOG_DBG("%s:%d %s no default value for field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMTable::tableGetDefaultFields(
    const bf_rt_target_t & /* dev_tgt */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrkFields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrkFields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::tableGetDefaultEntry(const bf_rt_target_t &dev_tgt,
                                              BfRtTMTableData *t_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (t_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(t_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  auto status = this->tableGetDefaultFields(dev_tgt, t_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  LOG_DBG("%s:%d %s need fields %zu of %zu; got block of %zu values",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          workDataFields.size(),
          active_fields_cnt,
          t_data->hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
    status = this->tableGetDefaultField(dev_tgt, data_id, t_data);
    if (status) {
      break;
    }
  }
  LOG_DBG("%s:%d %s got %zu values for %zu fields",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          t_data->hasValues(),
          active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
  }

  return status;
}

bf_status_t BfRtTMTable::tableDefaultEntryGet(const BfRtSession & /* session */,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /* flags */,
                                              BfRtTableData *data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != this->singlePipe_validate(dev_tgt)) {
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *t_data = static_cast<BfRtTMTableData *>(data);

  bf_status_t status = BF_SUCCESS;

  // If a derived fixed function table has actions, it is possible
  // to get an empty entry with some action_id set as default.
  if (!(this->actionIdApplicable()) && t_data->getActiveFields().empty()) {
    LOG_DBG("%s:%d %s reset to default active fields",
            __func__,
            __LINE__,
            this->table_name_get().c_str());

    status = t_data->reset();
    if (status) {
      LOG_ERROR("%s:%d %s Can't reset the data given",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
  }

  // Table entry lock for default entry is needed if the default
  // entry in this table might be changed.
  if (this->isVolatile() || this->isConstDefault()) {
    status = this->tableGetDefaultEntry(dev_tgt, t_data);
  } else {
    LOG_DBG("%s:%d %s Get entry lock",
            __func__,
            __LINE__,
            this->table_name_get().c_str());
    std::lock_guard<std::mutex> lock(this->entry_lock);
    status = this->tableGetDefaultEntry(dev_tgt, t_data);
  }

  return status;
}

// Sets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMTable::tableSetDefaultFields(
    const bf_rt_target_t & /* dev_tgt */,
    const BfRtTMTableData & /* p_data */,
    std::set<bf_rt_id_t> & /* wrkFields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrkFields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::tableSetDefaultField(
    const bf_rt_target_t & /* dev_tgt */,
    const BfRtTMTableData & /* qData */,
    bf_rt_id_t data_id) const {
  LOG_ERROR("%s:%d %s field_id=%d is not implemented",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            data_id);
  return BF_NOT_IMPLEMENTED;
}

bf_status_t BfRtTMTable::tableSetDefaultEntry(
    const bf_rt_target_t &dev_tgt, const BfRtTMTableData &t_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> assignedDataFields(t_data.getAssignedFields());

  // Set block of related fields at once if this mode is defined for the table.
  // In latter case the list of field ids should shrink.
  auto status =
      this->tableSetDefaultFields(dev_tgt, t_data, assignedDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of default fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  LOG_DBG("%s:%d %s active_fields=%zu with_values=%zu to_assign=%zu",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          t_data.getActiveFields().size(),
          t_data.hasValues(),
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
    status = this->tableSetDefaultField(dev_tgt, t_data, data_id);
    if (status) {
      break;
    }
  }  // for (field_id)

  return status;
}

bf_status_t BfRtTMTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableData &t_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (BF_SUCCESS != this->singlePipe_validate(dev_tgt)) {
    return BF_INVALID_ARG;
  }

  const BfRtTMTableData &wData = static_cast<const BfRtTMTableData &>(t_data);

  // If a derived fixed function table has actions, it is possible
  // to set an empty entry with some action_id.
  if (!(this->actionIdApplicable()) && wData.getAssignedFields().empty()) {
    LOG_TRACE("%s:%d %s No active data fields with values are given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  LOG_DBG("%s:%d %s Get entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> lock(this->entry_lock);

  return this->tableSetDefaultEntry(dev_tgt, wData);
}

bf_status_t BfRtTMTable::tableDefaultEntryReset(
    const BfRtSession & /* session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (BF_SUCCESS != this->singlePipe_validate(dev_tgt)) {
    return BF_INVALID_ARG;
  }

  // Empty data object as a template.
  BfRtTMTableData wData(this);

  // Init the template data with table-specific HW reset values.
  // No lock is needed for this except what TM API does for itself.
  auto status = this->tableResetEntry(dev_tgt, &wData);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't reset default entry",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // Lock the table to keep the default entry change consistent.
  LOG_DBG("%s:%d %s Get entry lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Update the default entry with HW reset values.
  return this->tableSetDefaultEntry(dev_tgt, wData);
}

bf_status_t BfRtTMTable::tableGetResetValue(
    const bf_rt_target_t & /* dev_tgt */,
    bf_rt_id_t data_id,
    BfRtTMTableData * /* t_data */) const {
  // A fixed function table field might have no reset value.
  LOG_DBG("%s:%d %s no reset value for field_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          data_id);
  return BF_NOT_IMPLEMENTED;
}

// Gets a group of related fields at once and removes ids from the given list.
bf_status_t BfRtTMTable::tableGetResetValues(
    const bf_rt_target_t & /* dev_tgt */,
    BfRtTMTableData * /* p_data */,
    std::set<bf_rt_id_t> & /* wrkFields */) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  // There is no blocks of related fields in a table by default.
  // Keep the wrkFields as is.
  return BF_SUCCESS;
}

bf_status_t BfRtTMTable::tableResetEntry(const bf_rt_target_t &dev_tgt,
                                         BfRtTMTableData *t_data) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (t_data == nullptr) {
    LOG_TRACE("%s:%d %s No data object is given",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Make a copy of the data field ids to work with.
  // The virtual set methods will remove processed ids from there.
  std::set<bf_rt_id_t> workDataFields(t_data->getActiveFields());
  auto active_fields_cnt = workDataFields.size();

  // Get block of related fields at once if this mode is defined for the table.
  // In latter case the list of work field ids should shrink.
  // Also it takes care of action id if applicable.
  auto status = this->tableGetResetValues(dev_tgt, t_data, workDataFields);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s can't process block of fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  LOG_DBG("%s:%d %s need fields %zu of %zu; got block of %zu values",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          workDataFields.size(),
          active_fields_cnt,
          t_data->hasValues());

  for (bf_rt_id_t data_id : workDataFields) {
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
    status = this->tableGetResetValue(dev_tgt, data_id, t_data);
    if (status) {
      break;
    }
  }
  LOG_DBG("%s:%d %s got %zu values for %zu fields",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          t_data->hasValues(),
          active_fields_cnt);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s rc=%d(%s)",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status,
              bf_err_str(status));
  }

  return status;
}

//----------
bf_status_t BfRtTMTable::popWorkField(const std::string &field_name,
                                      const bf_rt_id_t action_id,
                                      std::set<bf_rt_id_t> &wrk_fields,
                                      bf_rt_id_t &field_id) const {
  bf_rt_id_t data_id;
  auto status = this->dataFieldIdGet(field_name, action_id, &data_id);
  if (status != BF_SUCCESS) {
    return status;
  }
  field_id = 0;

  auto f_found = wrk_fields.find(data_id);
  if (f_found != wrk_fields.end()) {
    field_id = data_id;
    wrk_fields.erase(f_found);
  }
  return BF_SUCCESS;
}

bool BfRtTMTable::dataFieldDeclared(const std::string &expected_name,
                                    const bf_rt_id_t action_id,
                                    const bf_rt_id_t expected_id) const {
  bf_rt_id_t data_id = 0;

  auto state = this->dataFieldIdGet(expected_name, action_id, &data_id);
  if (BF_OBJECT_NOT_FOUND == state) {
    return false;
  }
  if (BF_SUCCESS != state) {
    BF_RT_DBGCHK(0);
    return false;
  }
  return (expected_id == data_id);
}

bf_status_t BfRtTMTable::singlePipe_validate(
    const bf_rt_target_t &dev_tgt) const {
  LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id);

  bf_status_t status = BF_INVALID_ARG;  // PIPE_ALL is not allowed

  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    status = trafficMgr->bfTMPipeIsValid(dev_tgt.dev_id, dev_tgt.pipe_id);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Invalid dev_id=%d pipe_id=%d, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
  }
  return status;
}

bf_status_t BfRtTMTable::singleOrPipeAll_validate(
    const bf_rt_target_t &dev_tgt) const {
  LOG_DBG("%s:%d %s dev_id=%d pipe_id=%d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id);
  bf_status_t status = BF_SUCCESS;  // PIPE_ALL is allowed

  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    status = trafficMgr->bfTMPipeIsValid(dev_tgt.dev_id, dev_tgt.pipe_id);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Invalid dev_id=%d pipe_id=%d, rc=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
  }
  return status;
}

bf_status_t BfRtTMTable::tmDevCfgGet(bf_dev_id_t dev_id,
                                     uint8_t *mau_pipes,
                                     uint8_t *pg_per_pipe,
                                     uint8_t *queues_per_pg,
                                     uint8_t *ports_per_pg) const {
  bf_tm_dev_cfg_t tm_cfg = {0};

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMDevCfgGet(dev_id, &tm_cfg);
  if (status) {
    LOG_ERROR("%s:%d Can't get TM configuration dev_id=%d",
              __func__,
              __LINE__,
              dev_id);
    return status;
  }

  if (NULL != mau_pipes) {
    *mau_pipes = tm_cfg.pipe_cnt;
  }
  if (NULL != pg_per_pipe) {
    *pg_per_pipe = tm_cfg.pg_per_pipe;
  }
  if (NULL != queues_per_pg) {
    *queues_per_pg = tm_cfg.q_per_pg;
  }
  if (NULL != ports_per_pg) {
    *ports_per_pg = tm_cfg.ports_per_pg;
  }
  return status;
}

bf_status_t BfRtTMTable::workPipesGet(const bf_rt_target_t &dev_tgt,
                                      uint8_t *cnt_pipes) const {
  LOG_DBG("%s:%d", __func__, __LINE__);

  if (cnt_pipes == nullptr) {
    return BF_INVALID_ARG;
  }

  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    *cnt_pipes = 1;
    return BF_SUCCESS;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  status = trafficMgr->bfTMPipeGetCount(dev_tgt.dev_id, cnt_pipes);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d Can't get number of pipes on dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
  }
  return status;
}

bf_status_t BfRtTMTable::splitDevPort(const bf_rt_target_t &dev_tgt,
                                      bf_dev_port_t port_id,
                                      bf_tm_pg_t &pg_id,
                                      uint8_t &pg_port_nr) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id &&
      DEV_PORT_TO_PIPE(port_id) != dev_tgt.pipe_id) {
    LOG_ERROR("%s:%d %s dev_port=%d is not on pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              port_id,
              dev_tgt.pipe_id);
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  return trafficMgr->bfTMPortGroupGet(
      dev_tgt.dev_id, port_id, &pg_id, &pg_port_nr);
}
}  // namespace bfrt
