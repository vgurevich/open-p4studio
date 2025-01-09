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


#include <unordered_set>

#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_table_attributes_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_table_attributes_state.hpp"

#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>
#include "bf_rt_p4_table_data_impl.hpp"
#include "bf_rt_p4_table_impl.hpp"
#include "bf_rt_p4_table_key_impl.hpp"

namespace bfrt {
namespace {

// getActionSpec fetches both resources and action_spec from Pipe Mgr.
// If some resources are unsupported it will filter them out.
bf_status_t getActionSpec(const BfRtSession &session,
                          const dev_target_t &dev_tgt,
                          const uint64_t &flags,
                          const pipe_tbl_hdl_t &pipe_tbl_hdl,
                          const pipe_mat_ent_hdl_t &mat_ent_hdl,
                          uint32_t res_get_flags,
                          pipe_tbl_match_spec_t *pipe_match_spec,
                          pipe_action_spec_t *pipe_action_spec,
                          pipe_act_fn_hdl_t *act_fn_hdl,
                          pipe_res_get_data_t *res_data) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = BF_SUCCESS;
  bool read_from_hw = false;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    read_from_hw = true;
  }
  if (pipe_match_spec) {
    status = pipeMgr->pipeMgrGetEntry(session.sessHandleGet(),
                                      pipe_tbl_hdl,
                                      dev_tgt,
                                      mat_ent_hdl,
                                      pipe_match_spec,
                                      pipe_action_spec,
                                      act_fn_hdl,
                                      read_from_hw,
                                      res_get_flags,
                                      res_data);
  } else {
    // Idle resource is not supported on default entries.
    res_get_flags &= ~PIPE_RES_GET_FLAG_IDLE;
    status = pipeMgr->pipeMgrTableGetDefaultEntry(session.sessHandleGet(),
                                                  dev_tgt,
                                                  pipe_tbl_hdl,
                                                  pipe_action_spec,
                                                  act_fn_hdl,
                                                  read_from_hw,
                                                  res_get_flags,
                                                  res_data);
  }
  return status;
}

static bf_status_t populate_data_fields(const BfRtTableObj &table,
                                        const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        pipe_res_get_data_t &res_data,
                                        pipe_act_fn_hdl_t pipe_act_fn_hdl,
                                        BfRtTableData *data) {
  bf_status_t status = BF_SUCCESS;

  bf_rt_id_t ttl_field_id = 0;
  bf_rt_id_t hs_field_id = 0;
  BfRtTable::TableType table_type;
  table.tableTypeGet(&table_type);

  BfRtMatchActionTableData *match_data =
      static_cast<BfRtMatchActionTableData *>(data);
  std::vector<bf_rt_id_t> dataFields;

  bool all_fields_set = match_data->allFieldsSet();
  bf_rt_id_t req_action_id = 0;
  status = match_data->actionIdGet(&req_action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);
  bf_rt_id_t action_id = 0;

  if (table_type == BfRtTable::TableType::MATCH_DIRECT) {
    action_id = table.getActIdFromActFnHdl(pipe_act_fn_hdl);
    if (req_action_id && req_action_id != action_id) {
      // Keeping this log as warning for iteration purposes.
      // Caller can decide to throw an error if required
      LOG_TRACE("%s:%d %s ERROR expecting action ID to be %d but recvd %d ",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                req_action_id,
                action_id);
      // Must free stful related memory
      if (res_data.stful.data != nullptr) {
        bf_sys_free(res_data.stful.data);
      }
      return BF_INVALID_ARG;
    }
  }

  match_data->actionIdSet(action_id);
  // Get the list of dataFields for action_id.
  if (all_fields_set) {
    status = table.dataFieldIdListGet(action_id, &dataFields);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      // Must free stful related memory
      if (res_data.stful.data != nullptr) {
        bf_sys_free(res_data.stful.data);
      }
      return status;
    }
    // Reset object with proper action id, but empty vector so all action
    // related fields will be marked as active.
    std::vector<bf_rt_id_t> empty;
    match_data->setActiveFields(empty);
  } else {
    // Set action id to proper value, but use data fields already present,
    // so only data fields requested will be returned.
    dataFields.assign(match_data->getActiveFields().begin(),
                      match_data->getActiveFields().end());
  }

  for (const auto &dataFieldId : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    status = table.getDataField(dataFieldId, action_id, &tableDataField);
    BF_RT_ASSERT(status == BF_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(fieldTypes);
    switch (field_destination) {
      case fieldDestination::DIRECT_LPF:
        if (res_data.has_lpf) {
          match_data->getPipeActionSpecObj().setValueLPFSpec(res_data.mtr.lpf);
          // There may be multiple lpf fields, it is enough to set it once
          res_data.has_counter = false;
        }
        break;
      case fieldDestination::DIRECT_WRED:
        if (res_data.has_red) {
          match_data->getPipeActionSpecObj().setValueWREDSpec(res_data.mtr.red);
          // There may be multiple red fields, it is enough to set it once
          res_data.has_red = false;
        }
        break;
      case fieldDestination::DIRECT_METER:
        if (res_data.has_meter) {
          match_data->getPipeActionSpecObj().setValueMeterSpec(
              res_data.mtr.meter);
          // There may be multiple meter fields, it is enough to set it once
          res_data.has_meter = false;
        }
        break;
      case fieldDestination::DIRECT_COUNTER:
        if (res_data.has_counter) {
          match_data->getPipeActionSpecObj().setValueCounterSpec(
              res_data.counter);
          // There may be multiple counter fields, it is enough to set it once
          res_data.has_counter = false;
        }
        break;
      case fieldDestination::DIRECT_REGISTER:
        if (res_data.has_stful) {
          std::vector<pipe_stful_mem_spec_t> register_pipe_data(
              res_data.stful.data,
              res_data.stful.data + res_data.stful.pipe_count);
          status = match_data->getPipeActionSpecObj().setValueRegisterSpec(
              register_pipe_data);
          if (status != BF_SUCCESS) {
            LOG_TRACE("%s:%d %s ERROR in setting register spec, err %d",
                      __func__,
                      __LINE__,
                      table.table_name_get().c_str(),
                      status);
            return status;
          }
          // There may be multiple register fields, it is enough to set it once
          res_data.has_stful = false;
        }
        break;
      case fieldDestination::TTL:
        ttl_field_id = dataFieldId;
        if (res_data.has_ttl) {
          match_data->set_ttl_from_read(res_data.idle.ttl);
        }
        break;
      case fieldDestination::ENTRY_HIT_STATE:
        hs_field_id = dataFieldId;
        if (res_data.has_hit_state) {
          match_data->set_entry_hit_state(res_data.idle.hit_state);
        }
        break;
      case fieldDestination::ACTION_SPEC: {
        if (table_type == BfRtTable::TableType::MATCH_DIRECT) {
          // Direct action data are populated by pipe_mgr in act_spec.
          break;
        }
        // If its the action member ID or the selector group ID, populate the
        // right member of the data object
        auto ind_tbl =
            static_cast<const BfRtMatchActionIndirectTable *>(&table);
        auto match_indir_data =
            static_cast<BfRtMatchActionIndirectTableData *>(match_data);
        pipe_action_spec_t *pipe_action_spec =
            match_indir_data->get_pipe_action_spec();
        std::set<bf_rt_id_t> oneof_siblings;
        status =
            ind_tbl->dataFieldOneofSiblingsGet(dataFieldId, &oneof_siblings);
        BF_RT_ASSERT(status == BF_SUCCESS);

        if (fieldTypes.find(DataFieldType::ACTION_MEMBER_ID) !=
            fieldTypes.end()) {
          if (IS_ACTION_SPEC_ACT_DATA_HDL(pipe_action_spec)) {
            bf_rt_id_t act_mbr_id;
            status = ind_tbl->getActionMbrIdFromHndl(
                session, dev_tgt, pipe_action_spec->adt_ent_hdl, &act_mbr_id);
            // Default entries will not have an action member handle if they
            // were installed automatically.  In this case return a member id
            // of zero.
            if (status == BF_OBJECT_NOT_FOUND &&
                //! pipe_match_spec &&  // Default entries won't have a match
                //! spec
                pipe_action_spec->adt_ent_hdl == 0) {
              status = BF_SUCCESS;
              act_mbr_id = 0;
            }
            BF_RT_ASSERT(status == BF_SUCCESS);
            match_indir_data->setActionMbrId(act_mbr_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else if (fieldTypes.find(DataFieldType::SELECTOR_GROUP_ID) !=
                   fieldTypes.end()) {
          if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
            bf_rt_id_t sel_grp_id;
            status = ind_tbl->getGroupIdFromHndl(
                session, dev_tgt, pipe_action_spec->sel_grp_hdl, &sel_grp_id);
            BF_RT_ASSERT(status == BF_SUCCESS);
            match_indir_data->setGroupId(sel_grp_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else {
          BF_RT_ASSERT(0);
        }
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  table.table_name_get().c_str(),
                  dataFieldId);
        if (res_data.stful.data != nullptr) {
          bf_sys_free(res_data.stful.data);
        }
        return BF_NOT_SUPPORTED;
        break;
    }
  }

  // Must free stful related memory
  if (res_data.stful.data != nullptr) {
    bf_sys_free(res_data.stful.data);
  }
  // After going over all the data fields, check whether either one
  // of entry_ttl or hit_state was set, remove if not.
  if (!res_data.has_ttl) {
    match_data->removeActiveFields({ttl_field_id});
  }
  if (!res_data.has_hit_state) {
    match_data->removeActiveFields({hs_field_id});
  }
  return BF_SUCCESS;
}

bf_status_t get_next_n_entries(const BfRtTableObj &table,
                               const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const BfRtTableKey &key,
                               const uint32_t &n,
                               BfRtTable::keyDataPairs *key_data_pairs,
                               uint32_t *num_returned) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        table.tablePipeHandleGet(),
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        false /* light_pipe_validation */);
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == BF_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             table.table_name_get().c_str());
    auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
        dev_tgt.dev_id, table.programNameGet());
    if (nullptr == device_state) {
      LOG_ERROR("%s:%d Failed to get device state for dev_id=%d",
                __func__,
                __LINE__,
                dev_tgt.dev_id);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }

    auto nextRef = device_state->nextRefState.getObjState(table.table_id_get());
    status = nextRef->getRef(
        session.sessHandleGet(), dev_tgt.pipe_id, &pipe_entry_hdl);
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return status;
  }

  std::vector<pipe_action_spec_t *> pipe_action_specs(n, nullptr);
  std::vector<pipe_tbl_match_spec_t> pipe_match_specs(n, {0});
  std::vector<uint32_t> res_get_flags(n, 0);
  // Validate data array
  unsigned i = 0;
  BfRtMatchActionKey *this_key = nullptr;
  for (i = 0; i < n; i++) {
    this_key = static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtMatchActionTableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != table.table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    if (this_data->allFieldsSet()) {
      res_get_flags[i] = PIPE_RES_GET_FLAG_ALL;
    } else {
      bf_rt_id_t req_action_id;
      status = this_data->actionIdGet(&req_action_id);
      for (const auto &dataFieldId : this_data->getActiveFields()) {
        const BfRtTableDataField *tableDataField = nullptr;
        status =
            table.getDataField(dataFieldId, req_action_id, &tableDataField);
        BF_RT_ASSERT(status == BF_SUCCESS);
        auto fieldTypes = tableDataField->getTypes();
        fieldDestination field_destination =
            BfRtTableDataField::getDataFieldDestination(fieldTypes);
        switch (field_destination) {
          case fieldDestination::DIRECT_LPF:
          case fieldDestination::DIRECT_METER:
          case fieldDestination::DIRECT_WRED:
            res_get_flags[i] |= PIPE_RES_GET_FLAG_METER;
            break;
          case fieldDestination::DIRECT_REGISTER:
            res_get_flags[i] |= PIPE_RES_GET_FLAG_STFUL;
            break;
          case fieldDestination::ACTION_SPEC:
            res_get_flags[i] |= PIPE_RES_GET_FLAG_ENTRY;
            break;
          case fieldDestination::DIRECT_COUNTER:
            res_get_flags[i] |= PIPE_RES_GET_FLAG_CNTR;
            break;
          case fieldDestination::ENTRY_HIT_STATE:
          case fieldDestination::TTL:
            res_get_flags[i] |= PIPE_RES_GET_FLAG_IDLE;
            break;
          default:
            break;
        }
      }
    }
    // Use table data object spec to save memory
    pipe_action_specs[i] = this_data->get_pipe_action_spec();
    // Initialize match spec fields, actual data will get overwritten
    // later by pipe_mgr
    this_key->populate_match_spec(&pipe_match_specs[i]);
  }

  std::vector<pipe_act_fn_hdl_t> act_fn_hdls(n, 0);
  std::vector<pipe_res_get_data_t> res_data(n, {0});
  pipe_action_spec_t **aspecs = pipe_action_specs.data();
  pipe_mat_ent_hdl_t last_ent_hdl = 0;

  status =
      pipeMgr->pipeMgrGetNextEntries(session.sessHandleGet(),
                                     table.tablePipeHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_entry_hdl,
                                     n,
                                     BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW),
                                     res_get_flags.data(),
                                     pipe_match_specs.data(),
                                     aspecs,
                                     act_fn_hdls.data(),
                                     res_data.data(),
                                     &last_ent_hdl,
                                     num_returned);
  if (PIPE_SUCCESS != status) {
    LOG_TRACE("%s:%d %s ERROR : Error fetching %d next entries.",
              __func__,
              __LINE__,
              table.table_name_get().c_str(),
              n);
    return status;
  }

  for (i = 0; i < *num_returned; i++) {
    this_key = static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    if (populate_data_fields(
            table, session, dev_tgt, res_data[i], act_fn_hdls[i], this_data)) {
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(pipe_match_specs[i].priority);
    this_key->setPartitionIndex(pipe_match_specs[i].partition_index);
  }

  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_tgt.dev_id, table.programNameGet());
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d Failed to get device state for dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto nextRef = device_state->nextRefState.getObjState(table.table_id_get());
  // Since pipe_mgr can fetch next n handles starting from non-existing handle,
  // store last one that was used in order to be able to fetch entries even if
  // related entry was deleted.
  nextRef->setRef(session.sessHandleGet(), dev_tgt.pipe_id, last_ent_hdl);

  return BF_SUCCESS;
}

template <class T>
bf_status_t getStfulSpecFromPipeMgr(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const pipe_tbl_hdl_t &pipe_tbl_hdl,
                                    const pipe_tbl_hdl_t &pipe_entry_hdl,
                                    const BfRtTable::BfRtTableGetFlag &flag,
                                    T *match_data) {
  int num_pipes = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = pipeMgr->pipeStfulDirectQueryGetSizes(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &num_pipes);

  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;
  uint32_t pipe_api_flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  status = pipeMgr->pipeStfulDirectEntQuery(session.sessHandleGet(),
                                            dev_tgt.dev_id,
                                            pipe_tbl_hdl,
                                            pipe_entry_hdl,
                                            &stful_query,
                                            pipe_api_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Failed to get Query info from pipe_mgr, err %d",
              __func__,
              __LINE__,
              status);
    return status;
  }
  status = match_data->getPipeActionSpecObj().setValueRegisterSpec(
      register_pipe_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d ERROR in setting register spec, err %d",
              __func__,
              __LINE__,
              status);
    return status;
  }
  return BF_SUCCESS;
}

template <class T>
bf_status_t getIdleTable(const T &table,
                         const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         BfRtTableAttributesImpl *tbl_attr_impl) {
  // Get the info from tableAttributes
  pipe_idle_time_params_t ttl_params;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->pipeMgrIdleParamsGet(session.sessHandleGet(),
                                              dev_tgt.dev_id,
                                              table.tablePipeHandleGet(),
                                              &ttl_params);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to get params info from pipe_mgr",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return status;
  }

  TableAttributesIdleTableMode bfrt_mode =
      static_cast<TableAttributesIdleTableMode>(ttl_params.mode);
  if (bfrt_mode > TableAttributesIdleTableMode::INVALID_MODE) {
    LOG_ERROR("%s:%d %s Invalid Idle Table Mode found programmed in pipe mgr",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  // Set the idle table mode in the attributes obj
  tbl_attr_impl->idleTableModeSet(bfrt_mode);

  // Get the state and get enabled/cb/cookie
  auto dev_state = BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id,
                                                      table.programNameGet());
  if (dev_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  // Get the state
  auto attr_state =
      dev_state->attributesState.getObjState(table.table_id_get());
  auto aging_state = attr_state->getAgingAttributeObj();
  auto t = aging_state.stateTableAttributesAgingGet();

  if (bfrt_mode == TableAttributesIdleTableMode::POLL_MODE) {
    return tbl_attr_impl->idleTablePollModeSet(std::get<0>(t));
  } else if (bfrt_mode == TableAttributesIdleTableMode::NOTIFY_MODE) {
    // enabled, cb, ttl_query_interval, max_ttl, min_ttl, cookie
    return tbl_attr_impl->idleTableNotifyModeSet(
        std::get<0>(t),
        std::get<1>(t),
        ttl_params.u.notify.ttl_query_interval,
        ttl_params.u.notify.max_ttl,
        ttl_params.u.notify.min_ttl,
        ttl_params.u.notify.client_data);
  }
  return BF_SUCCESS;
}

template <class T>
bf_status_t setIdleTable(T &table,
                         const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const BfRtTableAttributesImpl &tbl_attr_impl,
                         const uint64_t &flags) {
  // Save this callback separately and register the internal cb
  auto idle_cb_cpp = tbl_attr_impl.getIdleTableCallback();
  auto idle_cb_c = tbl_attr_impl.getIdleTableCallbackC();
  auto active_cb_cpp = tbl_attr_impl.getIdleTableActivateCallback();
  auto active_cb_c = tbl_attr_impl.getIdleTableActivateCallbackC();
  auto enabled = tbl_attr_impl.getIdleTableEnabled();

  // Use callback option with match_spec pointer
  auto ttl_params = tbl_attr_impl.getIdleTableTtlParamsInternal();

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FORCE_SIMPLE_IDLE_TIME_CALLBACK)) {
    ttl_params.u.notify.default_callback_choice = 0;
    ttl_params.u.notify.callback_fn = bfRtIdleTmoExpiryInternalSimpleCb;
  } else {
    ttl_params.u.notify.default_callback_choice = 1;
    ttl_params.u.notify.callback_fn2 = bfRtIdleTmoExpiryInternalCb;
  }

  // Get the state and set enabled/cb/cookie
  auto dev_state = BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id,
                                                      table.programNameGet());
  if (dev_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  // Update the state
  auto attr_state =
      dev_state->attributesState.getObjState(table.table_id_get());
  BfRtStateTableAttributesAging aging(enabled,
                                      idle_cb_cpp,
                                      active_cb_cpp,
                                      idle_cb_c,
                                      active_cb_c,
                                      &table,
                                      ttl_params.u.notify.client_data);
  attr_state->setAgingAttributeObj(aging);

  // Now all modification to the ttl_params to suit our needs will go here
  // That includes setting mode
  ttl_params.u.notify.client_data = &attr_state->getAgingAttributeObj();

  // Apply new config in pipe_mgr
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->pipeMgrIdleParamsSet(session.sessHandleGet(),
                                              dev_tgt.dev_id,
                                              table.tablePipeHandleGet(),
                                              ttl_params);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to set params in pipe_mgr",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return status;
  }
  status = pipeMgr->pipeMgrIdleTmoEnableSet(session.sessHandleGet(),
                                            dev_tgt.dev_id,
                                            table.tablePipeHandleGet(),
                                            enabled);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to %s idle table in pipe_mgr",
              __func__,
              __LINE__,
              table.table_name_get().c_str(),
              enabled ? "enable" : "disable");
    return status;
  }
  // Maintain state of the mode of idle table and the enable status.
  // This is required to validate field gets on TTL or ENTRY_HIT_STATE
  // fields. When idle table is in POLL mode only ENTRY_HIT_STATE is
  // applicable. For NOTIFY mode, TTL is applicable but not
  // ENTRY_HIT_STATE. This state is used to validate those gets/sets.
  // Also, this state is used to call the right pipe-mgr API to either
  // get TTL or ENTRY_HIT_STATE
  table.idleTableStateGet()->setEnabled(enabled);
  // Destroy the thread pool if exists
  table.idletimeCbThreadPoolReset(nullptr);
  table.idleTableStateGet()->setPollMode(ttl_params.mode == POLL_MODE);
  // Create a thread pool only for the enabled notify case
  if (enabled && table.idleTableStateGet()->isIdleTableinPollMode() == false) {
    table.idletimeCbThreadPoolReset(new BfRtThreadPool());
  }
  return BF_SUCCESS;
}

bf_status_t tableEntryModInternal(const BfRtTableObj &table,
                                  const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableData &data,
                                  const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_rt_id_t action_id = 0;
  status = match_data.actionIdGet(&action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  std::vector<bf_rt_id_t> dataFields;
  if (match_data.allFieldsSet()) {
    // This function is used for both match action direct and match action
    // indirect tables. Match action direct tables have a non-zero action id and
    // match action indirect tables have a zero action id. Zero action id
    // implies action id is not applicable and appropriate APIs need to be used.
    if (action_id) {
      status = table.dataFieldIdListGet(action_id, &dataFields);
    } else {
      status = table.dataFieldIdListGet(&dataFields);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  } else {
    dataFields.assign(match_data.getActiveFields().begin(),
                      match_data.getActiveFields().end());
  }

  pipe_action_spec_t pipe_action_spec = {0};
  match_data.copy_pipe_action_spec(&pipe_action_spec);

  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  bool direct_resource_found = false;
  bool action_spec_found = false;
  bool ttl_found = false;
  bool direct_counter_found = false;

  // Pipe-mgr exposes different APIs to modify different parts of the data
  // 1. To modify any part of the action spec, pipe_mgr_mat_ent_set_action is
  // the API to use
  //    As part of this following direct resources can be modified
  //      a. LPF
  //      b. WRED
  //      c. METER
  //      d. REGISTER
  // 2. So, if any of the data fields that are to be modified is part of the
  // action spec
  //    the above mentioned direct resources get a free ride.
  // 3. If there are no action parameters to be modified, the resources need to
  // be modified using
  //     the set_resource API.
  // 4. For direct counter resource, pipe_mgr_mat_ent_direct_stat_set is the API
  // to be used.
  // 5. For modifying TTL, a separate API to set the ttl is used.

  for (const auto &dataFieldId : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (action_id) {
      status = table.getDataField(dataFieldId, action_id, &tableDataField);
    } else {
      status = table.getDataField(dataFieldId, &tableDataField);
    }
    BF_RT_ASSERT(status == BF_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(fieldTypes);
    switch (field_destination) {
      case fieldDestination::DIRECT_LPF:
      case fieldDestination::DIRECT_METER:
      case fieldDestination::DIRECT_WRED:
      case fieldDestination::DIRECT_REGISTER:
        direct_resource_found = true;
        break;

      case fieldDestination::ACTION_SPEC:
        action_spec_found = true;
        break;

      case fieldDestination::TTL:
      case fieldDestination::ENTRY_HIT_STATE:
        ttl_found = true;
        break;

      case fieldDestination::DIRECT_COUNTER:
        direct_counter_found = true;
        break;

      default:
        break;
    }
  }

  if (action_id) {
    // If the caller specified an action id then we need to program the entry to
    // use that action id.  Set action_spec_found to true so that we call
    // pipeMgrMatEntSetAction down below.
    // Note that if the action did not have any action parameters the for loop
    // over data fields would not have found any "action-spec" fields.
    action_spec_found = true;
  }

  if (action_spec_found) {
    BfRtTable::TableType table_type;
    table.tableTypeGet(&table_type);
    if (table_type == BfRtTable::TableType::MATCH_INDIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
      // If we are modifying the action spec for a match action indirect or
      // match action selector table, we need to verify the member ID or the
      // selector group id referenced here is legit.

      const BfRtMatchActionIndirectTableData &match_indir_data =
          static_cast<const BfRtMatchActionIndirectTableData &>(match_data);

      const BfRtMatchActionIndirectTable &mat_indir_table =
          static_cast<const BfRtMatchActionIndirectTable &>(table);
      pipe_mgr_adt_ent_data_t ap_ent_data;

      pipe_adt_ent_hdl_t adt_ent_hdl = 0;
      pipe_sel_grp_hdl_t sel_grp_hdl = 0;
      status = mat_indir_table.getActionState(session,
                                              dev_tgt,
                                              &match_indir_data,
                                              &adt_ent_hdl,
                                              &sel_grp_hdl,
                                              &act_fn_hdl,
                                              &ap_ent_data);

      if (status != BF_SUCCESS) {
        if (match_indir_data.isGroup()) {
          if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not exist in the group table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getGroupId());
            return BF_OBJECT_NOT_FOUND;
          } else if (adt_ent_hdl == BfRtMatchActionIndirectTableData::
                                        invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not have any members in the group table "
                "associated with the table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getGroupId());
            return BF_OBJECT_NOT_FOUND;
          }
        } else {
          if (adt_ent_hdl ==
              BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a action "
                "member id %d which does not exist in the action profile table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getActionMbrId());
            return BF_OBJECT_NOT_FOUND;
          }
        }
      }
      if (match_indir_data.isGroup()) {
        pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
      } else {
        pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
      }
    }
    status = pipeMgr->pipeMgrMatEntSetAction(session.sessHandleGet(),
                                             pipe_dev_tgt.device_id,
                                             table.tablePipeHandleGet(),
                                             mat_ent_hdl,
                                             act_fn_hdl,
                                             &pipe_action_spec,
                                             0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  } else if (direct_resource_found) {
    status = pipeMgr->pipeMgrMatEntSetResource(session.sessHandleGet(),
                                               pipe_dev_tgt.device_id,
                                               table.tablePipeHandleGet(),
                                               mat_ent_hdl,
                                               pipe_action_spec.resources,
                                               pipe_action_spec.resource_count,
                                               0 /* Pipe API flags */);

    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in modifying resources part of table data, err %d",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          status);
      return status;
    }
  }

  if (direct_counter_found) {
    if (!BF_RT_FLAG_IS_SET(flags, BF_RT_SKIP_STAT_RESET)) {
      const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                              .getCounterSpecObj()
                                              .getPipeCounterSpec();

      status = pipeMgr->pipeMgrMatEntDirectStatSet(
          session.sessHandleGet(),
          pipe_dev_tgt.device_id,
          table.tablePipeHandleGet(),
          mat_ent_hdl,
          const_cast<pipe_stat_data_t *>(stat_data));
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in modifying counter, err %d",
                  __func__,
                  __LINE__,
                  table.table_name_get().c_str(),
                  status);
        return status;
      }
    }
  }

  if (ttl_found) {
    if (table.idleTablePollMode()) {
      status =
          pipeMgr->pipeMgrIdleTimeSetHitState(session.sessHandleGet(),
                                              pipe_dev_tgt.device_id,
                                              table.tablePipeHandleGet(),
                                              mat_ent_hdl,
                                              match_data.get_entry_hit_state());
    } else {
      bool reset = true;
      if (BF_RT_FLAG_IS_SET(flags, BF_RT_SKIP_TTL_RESET)) {
        reset = false;
      }
      status = pipeMgr->pipeMgrMatEntSetIdleTtl(session.sessHandleGet(),
                                                pipe_dev_tgt.device_id,
                                                table.tablePipeHandleGet(),
                                                mat_ent_hdl,
                                                match_data.get_ttl(),
                                                0 /* Pipe API flags */,
                                                reset);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying entry idle value, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  }

  return BF_SUCCESS;
}

template <typename T>
bf_status_t getTableUsage(const BfRtSession &session,
                          const bf_rt_target_t &dev_tgt,
                          const uint64_t &flags,
                          const T &table,
                          uint32_t *count) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrGetEntryCount(
      session.sessHandleGet(),
      pipe_dev_tgt,
      table.tablePipeHandleGet(),
      BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false,
      count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting to usage for table, err %d (%s)",
              __func__,
              __LINE__,
              table.table_name_get().c_str(),
              status,
              bf_err_str(status));
  }
  return status;
}

template <typename T>
bf_status_t getReservedEntries(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const T &table,
                               size_t *size) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;

  bf_status_t status = pipeMgr->pipeMgrGetReservedEntryCount(
      session.sessHandleGet(), pipe_dev_tgt, table.tablePipeHandleGet(), size);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting reserved entries count for table, err %d "
        "(%s)",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        status,
        bf_err_str(status));
  }
  return status;
}

// Template function for getFirst for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
bf_status_t getFirst_for_resource_tbls(const Table &table,
                                       const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t &flags,
                                       Key *key,
                                       BfRtTableData *data) {
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != table.table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  // First entry in a index based table is idx 0
  key->setIdxKey(0);

  return table.tableEntryGet(session, dev_tgt, flags, *key, data);
}

// Template function for getNext_n for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
bf_status_t getNext_n_for_resource_tbls(const Table &table,
                                        const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const Key &key,
                                        const uint32_t &n,
                                        BfRtTable::keyDataPairs *key_data_pairs,
                                        uint32_t *num_returned) {
  bf_status_t status = BF_SUCCESS;
  size_t table_size = 0;
  status = table.tableSizeGet(session, dev_tgt, flags, &table_size);
  uint32_t start_key = key.getIdxKey();

  *num_returned = 0;
  uint32_t i = 0;
  uint32_t j = 0;
  for (i = start_key + 1, j = 0; i <= start_key + n; i++, j++) {
    if (i >= table_size) {
      break;
    }
    auto this_key = static_cast<Key *>((*key_data_pairs)[j].first);
    this_key->setIdxKey(i);
    auto this_data = (*key_data_pairs)[j].second;

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != table.table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }

    bf_rt_id_t table_id_from_key;
    const BfRtTable *table_from_key;
    this_key->tableGet(&table_from_key);
    table_from_key->tableIdGet(&table_id_from_key);

    if (table_id_from_key != table.table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table key object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          table_id_from_key);
      return BF_INVALID_ARG;
    }

    status = table.tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting counter index %d, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                i,
                status);
      // Make the data object null if error
      (*key_data_pairs)[j].second = nullptr;
    }

    (*num_returned)++;
  }
  return BF_SUCCESS;
}

// This function checks if the key idx (applicable for Action profile, selector,
// Indirect meter, Counter, LPF, WRED, Register tables) is within the bounds of
// the size of the table

bool verify_key_for_idx_tbls(const BfRtSession &session,
                             const bf_rt_target_t &dev_tgt,
                             const BfRtTableObj &table,
                             uint32_t idx) {
  size_t table_size;
  table.tableSizeGet(session, dev_tgt, 0, &table_size);
  if (idx < table_size) {
    return true;
  }
  LOG_ERROR("%s:%d %s : ERROR Idx %d for key exceeds the size of the table %zd",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            idx,
            table_size);
  return false;
}

template <class Table, class Key>
bf_status_t key_reset(const Table &table, Key *match_key) {
  if (!table.validateTable_from_keyObj(*match_key)) {
    LOG_TRACE("%s:%d %s ERROR : Key object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return match_key->reset();
}

bf_status_t tableClearMatCommon(const BfRtSession &session,
                                const bf_rt_target_t &dev_tgt,
                                const bool &&reset_default_entry,
                                const BfRtTableObj *table) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Clear the table
  bf_status_t status = pipeMgr->pipeMgrMatTblClear(session.sessHandleGet(),
                                                   pipe_dev_tgt,
                                                   table->tablePipeHandleGet(),
                                                   0 /* pipe api flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Failed to clear table %s, err %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              status);
    return status;
  }
  if (reset_default_entry) {
    status = table->tableDefaultEntryReset(session, dev_tgt, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in resetting default entry , err %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                status);
    }
  }
  return status;
}

/* @brief
 * This function does the following
 * 1. Get the actual counts of the direct and the indirect resources needed by
 * this data object.
 * 2. Get the programmed counts of resources.
 * 3. Error out if there is a difference between actual indirect res
 * and the programmed indirect res count.
 * 4. Initialize the direct resources if there is a difference between the
 * actual and programmed direct res count.
 */
template <typename T>
bf_status_t resourceCheckAndInitialize(const BfRtTableObj &tbl,
                                       const T &tbl_data,
                                       const bool is_default) {
  // Get the pipe action spec from the data
  T &data = const_cast<T &>(tbl_data);
  PipeActionSpec &pipe_action_spec_obj = data.getPipeActionSpecObj();
  pipe_action_spec_t *pipe_action_spec =
      pipe_action_spec_obj.getPipeActionSpec();
  bf_rt_id_t action_id = 0;
  bool meter = false, reg = false, stat = false;
  auto status = data.actionIdGet(&action_id);
  if (BF_SUCCESS != status) {
    return status;
  }
  tbl.getActionResources(action_id, &meter, &reg, &stat);

  // We can get the indirect count from the data object directly by
  // counting the number of resource index field types set
  // However, for direct count, that is not possible since one
  // resource can have many different fields. We count direct
  // resources set by going through table_ref_map.
  // const auto &actual_indirect_count = tbl_data.indirectResCountGet();
  uint32_t actual_direct_count = 0;

  // Get a map of all the resources(direct and indirect) for this table
  auto &table_ref_map = tbl.getTableRefMap();
  if (table_ref_map.size() == 0) {
    // Nothing to be done. Just return
    return BF_SUCCESS;
  }
  // Iterate over the map to get all the direct resources attached to this
  // table
  std::unordered_set<pipe_tbl_hdl_t> direct_resources;
  for (const auto &ref : table_ref_map) {
    for (const auto &iter : ref.second) {
      if (iter.indirect_ref) {
        continue;
      }
      pipe_hdl_type_t hdl_type =
          static_cast<pipe_hdl_type_t>(PIPE_GET_HDL_TYPE(iter.tbl_hdl));
      switch (hdl_type) {
        case PIPE_HDL_TYPE_STAT_TBL:
          if (stat || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          if (meter || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        case PIPE_HDL_TYPE_STFUL_TBL:
          if (reg || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        default:
          break;
      }
    }
  }
  actual_direct_count = direct_resources.size();

  // Get the number of indirect and direct resources associated with the
  // action_spec. It can be different from the actual counts depending
  // upon whether the user has set it in the action_spec or not
  const auto &programmed_direct_count =
      pipe_action_spec_obj.directResCountGet();
  // const auto &programmed_indirect_count =
  //    pipe_action_spec_obj.indirectResCountGet();

  // if (programmed_indirect_count != actual_indirect_count) {
  //  LOG_ERROR(
  //      "%s:%d %s ERROR Indirect resource should always be set for this table
  //      ",
  //      __func__,
  //      __LINE__,
  //      tbl.table_name_get().c_str());
  //  return BF_INVALID_ARG;
  //}
  if (programmed_direct_count == actual_direct_count) {
    // If the direct resource count is equal to the resource count in the
    // formed pipe action spec, we don't need to do anything. as it means that
    // the user has explicitly set the values of all the resources attached
    // to this table already
    return BF_SUCCESS;
  } else if (is_default && programmed_direct_count > actual_direct_count) {
    // For default entries program only resources that are applicable for
    // the specified action.
    for (int i = 0; i < pipe_action_spec->resource_count; i++) {
      if (direct_resources.find(pipe_action_spec->resources[i].tbl_hdl) ==
          direct_resources.end()) {
        // This means that resource from action_spec is not applicable for
        // this action. Remove it by placing last resource in it's place and
        // decrementing resource count. If resource is present on both don't do
        // anything.
        int last_idx = pipe_action_spec->resource_count - 1;
        pipe_action_spec->resources[i] = pipe_action_spec->resources[last_idx];
        pipe_action_spec->resources[last_idx].tag =
            PIPE_RES_ACTION_TAG_NO_CHANGE;
        pipe_action_spec->resources[last_idx].tbl_hdl = 0;
        pipe_action_spec->resource_count--;
        // Iterate over same entry again, since it was updated.
        i--;
      }
    }
  } else if (programmed_direct_count > actual_direct_count) {
    // This cannot happen. If this happens then it means that we somehow
    // ended up programming more direct resources in the pipe action spec
    // than what actually exist
    LOG_ERROR(
        "%s:%d %s ERROR Pipe action spec has more direct resources "
        "programmed (%d) than the actual direct resources (%d) attached to "
        "that table",
        __func__,
        __LINE__,
        tbl.table_name_get().c_str(),
        programmed_direct_count,
        actual_direct_count);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  } else {
    // This means that the user has not intialized (via setValue) all the
    // direct resources that are attached to the table. Thus initialize
    // the remaining resources in the pipe action spec, so that the entry
    // gets programmed in all the respective resource managers
    // Remove the resources from the set that have already been
    // initialized
    for (int i = 0; i < pipe_action_spec->resource_count; i++) {
      if (direct_resources.find(pipe_action_spec->resources[i].tbl_hdl) ==
          direct_resources.end()) {
        // This means that we have an indirect resource in the action_spec
        // which we don't care about here. just log debug and continue
        LOG_DBG(
            "%s:%d %s Indirect resource with tbl hdl (%d) found "
            "programmed in the action spec",
            __func__,
            __LINE__,
            tbl.table_name_get().c_str(),
            pipe_action_spec->resources[i].tbl_hdl);
      } else {
        direct_resources.erase(pipe_action_spec->resources[i].tbl_hdl);
      }
    }

    // Now the set will have the remaining direct resoures which need to be
    // initialized in the action spec
    int i = pipe_action_spec->resource_count;
    for (const auto &iter : direct_resources) {
      // Since the pipe action spec already contains zeros for the resources
      // that have not been initialized, we just need to set the tbl_hdl
      // and the tag. No need to set again explicitly set the resource_data
      // to zero
      pipe_action_spec->resources[i].tbl_hdl = iter;
      pipe_action_spec->resources[i].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      i++;
    }
    pipe_action_spec->resource_count = i;
  }
  return BF_SUCCESS;
}

bool checkDefaultOnly(const BfRtTable *table, const BfRtTableData &data) {
  bf_rt_id_t action_id = 0;
  auto bf_status = data.actionIdGet(&action_id);
  if (bf_status != BF_SUCCESS) {
    return false;
  }

  AnnotationSet annotations;
  bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  auto def_an = Annotation("@defaultonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

bool checkTableOnly(const BfRtTable *table, const bf_rt_id_t &action_id) {
  if (!action_id) return false;
  AnnotationSet annotations;
  bf_status_t bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  auto def_an = Annotation("@tableonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

// used by dynamic key mask attribute, from key field mask to match spec
bf_status_t fieldMasksToMatchSpec(
    const std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> &field_mask,
    pipe_tbl_match_spec_t *mat_spec,
    const BfRtTableObj *table) {
  for (const auto &field : field_mask) {
    const BfRtTableKeyField *key_field;
    auto status = table->getKeyField(field.first, &key_field);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field.first);
      return status;
    }
    size_t sz;
    status = table->keyFieldSizeGet(field.first, &sz);
    if ((status != BF_SUCCESS) || (((sz + 7) / 8) != field.second.size())) {
      LOG_TRACE("%s:%d %s ERROR Invalid key field mask size, field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field.first);
      return status;
    }

    if (key_field->isFieldSlice()) {
      /* For field slice key, we store the full width field in the match spec.
       */
      bfrt::BfRtMatchActionKey::packFieldIntoMatchSpecByteBuffer(
          *key_field,
          field.second.size(),
          true, /* Must be true for DKM */
          field.second.data(),
          field.second.data(),
          mat_spec->match_value_bits + key_field->getOffset(),
          mat_spec->match_mask_bits + key_field->getOffset(),
          table);
    } else {
      std::memcpy(mat_spec->match_mask_bits + key_field->getOffset(),
                  field.second.data(),
                  field.second.size());
    }
  }
  mat_spec->num_match_bytes = table->getKeySize().bytes;
  mat_spec->num_valid_match_bits = table->getKeySize().bits;
  return BF_SUCCESS;
}
// used by dynamic key mask attribute, from match spec to key field mask
bf_status_t matchSpecToFieldMasks(
    const pipe_tbl_match_spec_t &mat_spec,
    std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> *field_mask,
    const BfRtTableObj *table) {
  if (mat_spec.match_mask_bits == nullptr) return BF_INVALID_ARG;
  std::vector<bf_rt_id_t> id_vec;
  table->keyFieldIdListGet(&id_vec);
  int cnt = 0;
  for (auto field_id : id_vec) {
    const BfRtTableKeyField *key_field;
    auto status = table->getKeyField(field_id, &key_field);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
    cnt = key_field->getOffset();
    size_t sz;
    status = table->keyFieldSizeGet(field_id, &sz);
    sz = (sz + 7) / 8;
    if (key_field->isFieldSlice()) {
      /* For field slice key, convert from full width field to key field size */
      size_t full_sz;
      full_sz = key_field->getParentFieldFullByteSize();
      cnt += full_sz - sz;
    }
    std::vector<uint8_t> mask(sz);
    for (uint32_t i = 0; i < sz; i++) {
      mask[i] = (mat_spec.match_mask_bits[cnt + i]);
    }
    if (mask.empty()) {
      LOG_TRACE("%s:%d %s Invalid mask for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
    if (field_mask->find(field_id) != field_mask->end()) {
      LOG_WARN(
          "%s:%d %s Field id %d has been configured duplicatedly, use the "
          "latest configuration",
          __func__,
          __LINE__,
          table->table_name_get().c_str(),
          field_id);
      field_mask->at(field_id) = mask;
    } else {
      field_mask->emplace(std::make_pair(field_id, mask));
    }
  }
  return BF_SUCCESS;
}

bf_status_t setHashAlgoFromDataObject(const BfRtTableObj *table,
                                      const BfRtDynHashAlgoTableData &data,
                                      const std::vector<bf_rt_id_t> &dataFields,
                                      bfn_hash_algorithm_t *algorithm,
                                      uint64_t *seed,
                                      uint64_t *rotate,
                                      bool *is_algo_set,
                                      bool *is_seed_set) {
  bfn_crc_alg_t crc_type = CRC_INVALID;
  bfn_hash_alg_type_t alg_type = INVALID_DYN;

  bf_rt_id_t action_id;
  auto status = data.actionIdGet(&action_id);

  std::string action_name = "";
  if (action_id) {
    table->actionNameGet(action_id, &action_name);
  }

  bool msb = false;
  bool extend = false;
  // The default value for hash_bit_width needs to be
  // the width defined in the hash object. This default
  // width is used for some non-crc algorithms like identity.
  // Some algorithms which do not care about this like
  // random, will ignore it.
  uint64_t hash_bit_width = table->hash_bit_width_get();
  std::string algo_name;

  for (const auto &field_id : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (action_id) {
      status = table->getDataField(field_id, action_id, &tableDataField);
    } else {
      status = table->getDataField(field_id, &tableDataField);
    }

    if (tableDataField->getName() == "seed") {
      status = data.getValue(field_id, seed);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_seed_set = true;
    } else if (tableDataField->getName() == "rotate") {
      status = data.getValue(field_id, rotate);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "msb") {
      status = data.getValue(field_id, &msb);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "extend") {
      status = data.getValue(field_id, &extend);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "algorithm_name") {
      status = data.getValue(field_id, &algo_name);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      // convert the algo name to a bfn_hash_algorithm_t struct
      crc_type = crc_alg_str_to_type(algo_name.c_str());
      alg_type = hash_alg_str_to_type(algo_name.c_str());
      if (crc_type == CRC_INVALID) {
        LOG_WARN(
            "%s:%d %s is Not a CRC algorithm. Will try to use another "
            "algorithm type",
            __func__,
            __LINE__,
            algo_name.c_str());
      } else {
        // Some CRC type was found. Set Algo type as CRC
        alg_type = CRC_DYN;
      }
      if (alg_type == INVALID_DYN) {
        // By this time, algo_type should have something. Either
        // CRC_DYN, RANDOM_DYN, XOR_DYN, IDENTITY_DYN
        LOG_TRACE("%s:%d %s is not a valid alg or crc type",
                  __func__,
                  __LINE__,
                  algo_name.c_str());
        return BF_INVALID_ARG;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "hash_bit_width") {
      // reinterpret_cast is dangerous but we are fairly certain that
      // hash_bit_width will always be a positive value within bounds.
      status = data.getValue(field_id, &hash_bit_width);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "reverse") {
      status = data.getValue(field_id, &algorithm->reverse);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "polynomial") {
      status = data.getValue(field_id, &algorithm->poly);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "init") {
      status = data.getValue(field_id, &algorithm->init);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "final_xor") {
      status = data.getValue(field_id, &algorithm->final_xor);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    }
  }

  // Set the bit width. For predefined CRC algorithms,
  // initialize_algorithm() will anyway overwrite it with the
  // right values. Identity and XOR will not be touched by
  // initialize_algorithm()
  algorithm->hash_bit_width = hash_bit_width;
  // This helps figure out if algorithm needs to be set
  if (action_name == "pre_defined") {
    initialize_algorithm(algorithm, alg_type, msb, extend, crc_type);
  } else if (action_name == "user_defined") {
    // convert these values to a bfn_hash_algorithm_t struct
    algorithm->hash_alg = CRC_DYN;
    algorithm->msb = msb;
    algorithm->extend = extend;
    algorithm->crc_type = CRC_INVALID;
    algorithm->crc_matrix = nullptr;
  }
  return status;
}

bf_status_t getHashAlgoInDataObject(const BfRtTableObj *table,
                                    const bfn_hash_algorithm_t &algorithm,
                                    const uint64_t &rotate,
                                    const uint64_t &seed,
                                    BfRtDynHashAlgoTableData *data) {
  // If alg_type == CRC_DYN, crc_type == CRC_INVALID, then must be user_defined
  // If alg_type == CRC_DYN, crc_type != CRC_INVALID, then must be predefined
  // If alg_type == either of RANDOM_DYN, IDENTITY_DYN, XOR_DYN, then also must
  // be predefined (non-CRC)
  bf_rt_id_t action_id;
  std::string action_name = "pre_defined";
  if (algorithm.hash_alg == CRC_DYN && algorithm.crc_type == CRC_INVALID) {
    // user defined crc
    action_name = "user_defined";
  } else if (algorithm.hash_alg == INVALID_DYN &&
             algorithm.crc_type == CRC_INVALID) {
    LOG_TRACE("%s:%d %s : Error : received invalid config",
              __func__,
              __LINE__,
              table->table_name_get().c_str());
    return BF_UNEXPECTED;
  }
  auto status = table->actionIdGet(action_name, &action_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the action ID for action %s",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              action_name.c_str());
    return status;
  }
  std::vector<bf_rt_id_t> empty;
  data->actionIdSet(action_id);
  data->setActiveFields(empty);

  bf_rt_id_t field_id;

  // Get the rotate
  status = table->dataFieldIdGet("rotate", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for rotate",
              __func__,
              __LINE__,
              table->table_name_get().c_str());
    return status;
  }
  status = data->setValue(field_id, rotate);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Get the seed
  status = table->dataFieldIdGet("seed", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for seed",
              __func__,
              __LINE__,
              table->table_name_get().c_str());
    return status;
  }
  status = data->setValue(field_id, seed);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Get the msb and extend
  status = table->dataFieldIdGet("msb", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for msb",
              __func__,
              __LINE__,
              table->table_name_get().c_str());
    return status;
  }
  status = data->setValue(field_id, algorithm.msb);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              field_id);
    return status;
  }

  status = table->dataFieldIdGet("extend", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for extend",
              __func__,
              __LINE__,
              table->table_name_get().c_str());
    return status;
  }
  status = data->setValue(field_id, algorithm.extend);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              field_id);
    return status;
  }

  if (action_name == "pre_defined") {
    // Set the algo name
    status = table->dataFieldIdGet("algorithm_name", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for algorithm_name",
          __func__,
          __LINE__,
          table->table_name_get().c_str());
      return status;
    }
    std::string algo_str;
    if (algorithm.hash_alg == CRC_DYN) {
      char crc_name[BF_UTILS_ALGO_NAME_LEN];
      crc_alg_type_to_str(algorithm.crc_type, crc_name);
      algo_str = crc_name;
    } else if (algorithm.hash_alg == INVALID_DYN) {
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error while getting alg type of current algo",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str());
      }
      return BF_INVALID_ARG;
    } else {
      // algo type is RANDOM/XOR/IDENTITY
      char alg_name[BF_UTILS_ALGO_NAME_LEN];
      hash_alg_type_to_str(algorithm.hash_alg, alg_name);
      algo_str = alg_name;
    }

    status = data->setValue(field_id, algo_str);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
  } else if (action_name == "user_defined") {
    // get hash_bit_width
    status = table->dataFieldIdGet("hash_bit_width", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for hash_bit_width",
          __func__,
          __LINE__,
          table->table_name_get().c_str());
      return status;
    }
    status = data->setValue(field_id,
                            static_cast<uint64_t>(algorithm.hash_bit_width));
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
    // get reverse
    status = table->dataFieldIdGet("reverse", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for reverse",
                __func__,
                __LINE__,
                table->table_name_get().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.reverse);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
    // get polynomial
    status = table->dataFieldIdGet("polynomial", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for polynomial",
                __func__,
                __LINE__,
                table->table_name_get().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.poly);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
    // get init
    status = table->dataFieldIdGet("init", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for init",
                __func__,
                __LINE__,
                table->table_name_get().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.init);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
    // get final_xor
    status = table->dataFieldIdGet("final_xor", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for final_xor",
                __func__,
                __LINE__,
                table->table_name_get().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.final_xor);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                field_id);
      return status;
    }
  }
  return status;
}

// Must be in line with json file
const std::map<std::string, bf_snapshot_ig_mode_t> snapIgMode = {
    {"INGRESS", BF_SNAPSHOT_IGM_INGRESS},
    {"GHOST", BF_SNAPSHOT_IGM_GHOST},
    {"ANY", BF_SNAPSHOT_IGM_ANY},
    {"INGRESS_GHOST", BF_SNAPSHOT_IGM_BOTH}};

// Function used to convert pipe_mgr format (which does not use "." and uses "_"
// instead) to naming used in bf-rt.json. Function will return input string if
// related table is not found.
const std::string getQualifiedTableName(const bf_dev_id_t &dev_id,
                                        const std::string &p4_name,
                                        const std::string &tbl_name) {
  const BfRtInfo *info;
  std::vector<const BfRtTable *> tables;
  PipelineProfInfoVec pipe_info;
  BfRtDevMgr::getInstance().bfRtInfoGet(dev_id, p4_name, &info);
  info->bfRtInfoPipelineInfoGet(&pipe_info);
  info->bfrtInfoGetTables(&tables);
  for (auto const table : tables) {
    // For comparison convert name to pipe_mgr format. No pipe name prefix and
    // "_" instead of ".".
    std::string name;
    table->tableNameGet(&name);
    std::string name_cmp = name;
    for (auto &pinfo : pipe_info) {
      std::string prefix = pinfo.first;
      // Remove pipe name and trailing "." if present.
      if (name_cmp.rfind(prefix, 0) == 0)
        name_cmp.erase(0, prefix.length() + 1);
      std::replace(name_cmp.begin(), name_cmp.end(), '.', '_');
      if (!tbl_name.compare(name_cmp)) {
        return name;
      }
    }
  }
  // Return same name if not found
  return tbl_name;
}

std::string getPipeMgrTblName(const std::string &orig_name) {
  std::string tbl_name = orig_name;
  // Remove till first dot. Internal tables do not have dots in the name.
  tbl_name.erase(0, tbl_name.find(".") + 1);
  std::replace(tbl_name.begin(), tbl_name.end(), '.', '_');
  return tbl_name;
}

// Must be in line with bf_tbl_dbg_counter_type_t
const std::vector<std::string> cntTypeStr{"DISABLED",
                                          "TBL_MISS",
                                          "TBL_HIT",
                                          "GW_TBL_MISS",
                                          "GW_TBL_HIT",
                                          "GW_TBL_INHIBIT",
                                          "UNUSED"};

}  // anonymous namespace

// BfRtMatchActionTable ******************
void BfRtMatchActionTable::setActionResources(const bf_dev_id_t &dev_id) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  for (auto const &i : this->act_fn_hdl_to_id) {
    bool has_cntr, has_meter, has_lpf, has_wred, has_reg;
    has_cntr = has_meter = has_lpf = has_wred = has_reg = false;
    bf_status_t sts =
        pipeMgr->pipeMgrGetActionDirectResUsage(dev_id,
                                                this->pipe_tbl_hdl,
                                                i.first,
                                                &has_cntr,
                                                &has_meter,
                                                &has_lpf,
                                                &has_wred,
                                                &has_reg);
    if (sts) {
      LOG_ERROR("%s:%d %s Cannot fetch action resource information, %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                sts);
      continue;
    }
    this->act_uses_dir_cntr[i.second] = has_cntr;
    this->act_uses_dir_meter[i.second] = has_meter || has_lpf || has_wred;
    this->act_uses_dir_reg[i.second] = has_reg;
  }
}

bf_status_t BfRtMatchActionTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);

  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Initialize the direct resources if they were not provided by the caller.
  auto status = resourceCheckAndInitialize<BfRtMatchActionTableData>(
      *this, match_data, false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  if (checkDefaultOnly(this, data)) {
    LOG_TRACE("%s:%d %s Error adding action because it is defaultOnly",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();
  if (idle_table_state->isIdleTableinPollMode()) {
    /* In poll mode non-zero ttl means that entry should be marked as active */
    ttl = (match_data.get_entry_hit_state() == ENTRY_ACTIVE) ? 1 : 0;
  }
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   pipe_action_spec,
                                   ttl,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryModInternal(
      *this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

bf_status_t BfRtMatchActionTable::tableEntryAddOrMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data,
    bool *is_added) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);

  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Initialize the direct resources if they were not provided by the caller.
  auto status = resourceCheckAndInitialize<BfRtMatchActionTableData>(
      *this, match_data, false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  if (checkDefaultOnly(this, data)) {
    LOG_TRACE("%s:%d %s Error adding action because it is defaultOnly",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();
  if (idle_table_state->isIdleTableinPollMode()) {
    /* In poll mode non-zero ttl means that entry should be marked as active */
    ttl = (match_data.get_entry_hit_state() == ENTRY_ACTIVE) ? 1 : 0;
  }
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  return pipeMgr->pipeMgrMatEntAddOrMod(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        act_fn_hdl,
                                        pipe_action_spec,
                                        ttl,
                                        0 /* Pipe API flags */,
                                        &pipe_entry_hdl,
                                        is_added);
}

bf_status_t BfRtMatchActionTable::tableEntryDel(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  uint32_t pipe_flags = 0;
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);

  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_IGNORE_NOT_FOUND)) {
    pipe_flags = PIPE_FLAG_IGNORE_NOT_FOUND;
  }

  bf_status_t status =
      pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           &pipe_match_spec,
                                           pipe_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableClear(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/) const {
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

bf_status_t BfRtMatchActionTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  bf_rt_id_t action_id;
  status = match_data.actionIdGet(&action_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(status == BF_SUCCESS);
    return status;
  }
  if (checkTableOnly(this, action_id)) {
    LOG_TRACE("%s:%d %s Error adding action because it is tableOnly",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Check which direct resources were provided.
  bool direct_reg = false, direct_cntr = false, direct_mtr = false;
  if (action_id) {
    this->getActionResources(action_id, &direct_mtr, &direct_reg, &direct_cntr);
  } else {
    std::vector<bf_rt_id_t> dataFields;
    if (match_data.allFieldsSet()) {
      status = this->dataFieldIdListGet(&dataFields);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  status);
        return status;
      }
    } else {
      dataFields.assign(match_data.getActiveFields().begin(),
                        match_data.getActiveFields().end());
    }
    for (const auto &dataFieldId : dataFields) {
      const BfRtTableDataField *field = nullptr;
      status = this->getDataField(dataFieldId, &field);
      BF_RT_DBGCHK(status == BF_SUCCESS);
      fieldDestination dest =
          BfRtTableDataField::getDataFieldDestination(field->getTypes());
      switch (dest) {
        case fieldDestination::DIRECT_REGISTER:
          direct_reg = true;
          break;
        case fieldDestination::DIRECT_COUNTER:
          direct_cntr = true;
          break;
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
        case fieldDestination::DIRECT_LPF:
          direct_mtr = true;
          break;
        default:
          break;
      }
    }
  }

  pipe_mat_ent_hdl_t entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  if (action_id) {
    // Initialize the direct resources if they were not provided by the
    // caller.
    status = resourceCheckAndInitialize<BfRtMatchActionTableData>(
        *this, match_data, true);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Failed to initialize direct resources",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }

    pipe_action_spec = match_data.get_pipe_action_spec();
    pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

    status = pipeMgr->pipeMgrMatDefaultEntrySet(session.sessHandleGet(),
                                                pipe_dev_tgt,
                                                pipe_tbl_hdl,
                                                act_fn_hdl,
                                                pipe_action_spec,
                                                0 /* Pipe API flags */,
                                                &entry_hdl);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting default entry, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  } else {
    // Get the handle of the default entry.
    status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
        session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &entry_hdl);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error getting entry handle, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  }

  // Program direct counters if requested.
  if (direct_cntr) {
    if (!BF_RT_FLAG_IS_SET(flags, BF_RT_SKIP_STAT_RESET)) {
      const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                              .getCounterSpecObj()
                                              .getPipeCounterSpec();
      status = pipeMgr->pipeMgrMatEntDirectStatSet(
          session.sessHandleGet(),
          pipe_dev_tgt.device_id,
          pipe_tbl_hdl,
          entry_hdl,
          const_cast<pipe_stat_data_t *>(stat_data));
      if (BF_SUCCESS != status) {
        LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct stats, %d",
                  __func__,
                  __LINE__,
                  pipe_dev_tgt.device_id,
                  pipe_dev_tgt.dev_pipe_id,
                  this->table_name_get().c_str(),
                  status);
        return status;
      }
    }
  }
  // Program direct meter/wred/lpf and registers only if we did not do a set
  // above.
  if (!action_id && (direct_reg || direct_mtr)) {
    pipe_action_spec = match_data.get_pipe_action_spec();
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.sessHandleGet(),
        pipe_dev_tgt.device_id,
        pipe_tbl_hdl,
        entry_hdl,
        const_cast<pipe_res_spec_t *>(pipe_action_spec->resources),
        pipe_action_spec->resource_count,
        0 /* Pipe API flags */);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct resources, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, 0);
}

bf_status_t BfRtMatchActionTable::tableEntryGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        true /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtMatchActionTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &pipe_entry_hdl);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

bf_status_t BfRtMatchActionTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  bf_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_handle);

  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = tableEntryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d Failed to get device state for dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
  nextRef->setRef(session.sessHandleGet(), dev_tgt.pipe_id, first_entry_handle);

  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  return get_next_n_entries(
      *this, session, dev_tgt, flags, key, n, key_data_pairs, num_returned);
}

bf_status_t BfRtMatchActionTable::tableSizeGet(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               size_t *count) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;

  *count = 0;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrGetTotalHwEntryCount(session.sessHandleGet(),
                                                  pipe_dev_tgt,
                                                  this->tablePipeHandleGet(),
                                                  count);
    // Fallback to sw count in case HW count is not supported.
    if (status != BF_NOT_SUPPORTED) goto exit;
  }

  size_t reserved;
  status = pipeMgr->pipeMgrGetReservedEntryCount(session.sessHandleGet(),
                                                 pipe_dev_tgt,
                                                 this->tablePipeHandleGet(),
                                                 &reserved);
  *count = this->_table_size - reserved;
exit:
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting total entries count for table, err %d "
        "(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        status,
        bf_err_str(status));
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableUsageGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtMatchActionTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
bf_status_t BfRtMatchActionTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

bf_status_t dataReset_internal(const BfRtTableObj &table,
                               const bf_rt_id_t &action_id,
                               const std::vector<bf_rt_id_t> &fields,
                               BfRtTableData *data) {
  BfRtTableDataObj *data_obj = static_cast<BfRtTableDataObj *>(data);
  if (!table.validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(action_id, fields);
}

bf_status_t BfRtMatchActionTable::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtMatchActionTable, BfRtMatchActionKey>(*this, match_key);
}

bf_status_t BfRtMatchActionTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(const bf_rt_id_t &action_id,
                                            BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, action_id, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(
    const std::vector<bf_rt_id_t> &fields, BfRtTableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    BfRtTableData *data) const {
  return dataReset_internal(*this, action_id, fields, data);
}

bf_status_t BfRtMatchActionTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type == TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE(
        "%s:%d %s Idle Table Runtime Attribute requires a Mode, please use the "
        "appropriate API to include the idle table mode",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type, idle_table_mode));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMatchActionTable::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

bf_status_t BfRtMatchActionTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<64> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ullong();
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      if (sts == BF_SUCCESS) {
        auto device_state =
            BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
        if (device_state == nullptr) {
          BF_RT_ASSERT(0);
          return BF_OBJECT_NOT_FOUND;
        }

        // Set ENTRY_SCOPE of the table
        auto attributes_state =
            device_state->attributesState.getObjState(table_id_get());
        attributes_state->setEntryScope(entry_scope);
      }
      return sts;
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> field_mask;
      bf_status_t sts = tbl_attr_impl->dynKeyMaskGet(&field_mask);
      pipe_tbl_match_spec_t match_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      match_spec.match_mask_bits = match_mask_bits.data();
      // For this case value_bits will be modified the same way as mask, but
      // cannot be left uninitialized.
      match_spec.match_value_bits = match_mask_bits.data();
      // translate from map to match_spec
      sts = fieldMasksToMatchSpec(field_mask, &match_spec, this);
      if (sts != BF_SUCCESS) return sts;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrMatchKeyMaskSpecSet(
          session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &match_spec);
      return sts;
    }
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      bf_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != BF_SUCCESS) {
        return sts;
      }
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.sessHandleGet(), pipe_dev_tgt, res_hdl, byte_count);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return setIdleTable(const_cast<BfRtMatchActionTable &>(*this),
                          session,
                          dev_tgt,
                          *tbl_attr_impl,
                          flags);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      pipe_tbl_match_spec_t mat_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      mat_spec.match_mask_bits = match_mask_bits.data();
      mat_spec.num_match_bytes = sz;
      mat_spec.num_valid_match_bits = this->getKeySize().bits;

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMatchKeyMaskSpecGet(
          session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &mat_spec);
      std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> field_mask;
      matchSpecToFieldMasks(mat_spec, &field_mask, this);
      return tbl_attr_impl->dynKeyMaskSet(field_mask);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return getIdleTable(*this, session, dev_tgt, tbl_attr_impl);
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      auto sts = pipeMgr->pipeMgrMeterByteCountGet(
          session.sessHandleGet(), pipe_dev_tgt, res_hdl, &byte_count);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get meter bytecount adjust from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::registerMatUpdateCb(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const pipe_mat_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterMatUpdateCb(session.sessHandleGet(),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}

// Unexposed functions

void BfRtMatchActionTable::setIsTernaryTable(const bf_dev_id_t &dev_id) {
  auto status = PipeMgrIntf::getInstance()->pipeMgrTblIsTern(
      dev_id, pipe_tbl_hdl, &this->is_ternary_table_);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to find whether table is ternary or not",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
}

bf_status_t BfRtMatchActionTable::dataAllocate_internal(
    bf_rt_id_t action_id,
    std::unique_ptr<BfRtTableData> *data_ret,
    const std::vector<bf_rt_id_t> &fields) const {
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
      return BF_OBJECT_NOT_FOUND;
    }
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtMatchActionTableData(this, action_id, fields));
  } else {
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtMatchActionTableData(this, fields));
  }

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t req_action_id = 0;

  uint32_t res_get_flags = 0;
  pipe_res_get_data_t res_data = {0};
  res_data.stful.data = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  bool all_fields_set = false;

  BfRtMatchActionTableData *match_data =
      static_cast<BfRtMatchActionTableData *>(data);
  std::vector<bf_rt_id_t> dataFields;
  status = match_data->actionIdGet(&req_action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  all_fields_set = match_data->allFieldsSet();
  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
    // do not assign dataFields in this case because we might not know
    // what the actual_action_id is yet
    // Based upon the actual action id, we will be filling in the data fields
    // later anyway
  } else {
    dataFields.assign(match_data->getActiveFields().begin(),
                      match_data->getActiveFields().end());
    for (const auto &dataFieldId : dataFields) {
      const BfRtTableDataField *tableDataField = nullptr;
      if (req_action_id) {
        status = getDataField(dataFieldId, req_action_id, &tableDataField);
      } else {
        status = getDataField(dataFieldId, &tableDataField);
      }
      BF_RT_ASSERT(status == BF_SUCCESS);
      auto fieldTypes = tableDataField->getTypes();
      fieldDestination field_destination =
          BfRtTableDataField::getDataFieldDestination(fieldTypes);
      switch (field_destination) {
        case fieldDestination::DIRECT_LPF:
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
          res_get_flags |= PIPE_RES_GET_FLAG_METER;
          break;
        case fieldDestination::DIRECT_REGISTER:
          res_get_flags |= PIPE_RES_GET_FLAG_STFUL;
          break;
        case fieldDestination::ACTION_SPEC:
          res_get_flags |= PIPE_RES_GET_FLAG_ENTRY;
          break;
        case fieldDestination::DIRECT_COUNTER:
          res_get_flags |= PIPE_RES_GET_FLAG_CNTR;
          break;
        case fieldDestination::ENTRY_HIT_STATE:
        case fieldDestination::TTL:
          res_get_flags |= PIPE_RES_GET_FLAG_IDLE;
          break;
        default:
          break;
      }
    }
  }
  // All inputs from the data object have been processed. Now reset it
  // for out data purpose
  // We reset the data object with act_id 0 and all fields
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return status;
  }
  return populate_data_fields(
      *this, session, dev_tgt, res_data, pipe_act_fn_hdl, data);
}

// BfRtMatchActionIndirectTable **************
namespace {
const std::vector<DataFieldType> indirectResourceDataFields = {
    DataFieldType::COUNTER_INDEX,
    DataFieldType::METER_INDEX,
    DataFieldType::REGISTER_INDEX};
}

void BfRtMatchActionIndirectTable::setActionResources(
    const bf_dev_id_t &dev_id) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  for (auto const &i : this->act_fn_hdl_to_id) {
    bool has_cntr, has_meter, has_lpf, has_wred, has_reg;
    has_cntr = has_meter = has_lpf = has_wred = has_reg = false;
    bf_status_t sts =
        pipeMgr->pipeMgrGetActionDirectResUsage(dev_id,
                                                this->pipe_tbl_hdl,
                                                i.first,
                                                &has_cntr,
                                                &has_meter,
                                                &has_lpf,
                                                &has_wred,
                                                &has_reg);
    if (sts) {
      LOG_ERROR("%s:%d %s Cannot fetch action resource information, %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                sts);
      continue;
    }
    this->act_uses_dir_cntr[i.second] = has_cntr;
    this->act_uses_dir_meter[i.second] = has_meter || has_lpf || has_wred;
    this->act_uses_dir_reg[i.second] = has_reg;
  }
}

void BfRtMatchActionIndirectTable::populate_indirect_resources(
    const pipe_mgr_adt_ent_data_t &ent_data,
    pipe_action_spec_t *pipe_action_spec) const {
  /* Append indirect action resources after already set resources in
   * pipe_action_spec. */
  for (int j = 0; j < ent_data.num_resources; j++) {
    int free_idx = pipe_action_spec->resource_count;
    pipe_res_spec_t *res_spec = &pipe_action_spec->resources[free_idx];
    res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
    res_spec->tbl_idx = (pipe_res_idx_t)ent_data.adt_data_resources[j].tbl_idx;
    res_spec->tbl_hdl = ent_data.adt_data_resources[j].tbl_hdl;
    pipe_action_spec->resource_count++;
  }
  return;
}

bf_status_t BfRtMatchActionIndirectTable::populate_entry_data(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtMatchActionKey &match_key,
    const BfRtMatchActionIndirectTableData &match_data,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) const {
  if (!pipe_match_spec || !pipe_action_spec || !act_fn_hdl)
    return BF_INVALID_ARG;

  // Check if all indirect resource indices are supplied or not.
  // Entry Add mandates that all indirect indices be given
  // Initialize the direct resources if applicable
  bf_status_t status =
      resourceCheckAndInitialize<BfRtMatchActionIndirectTableData>(
          *this, match_data, false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  match_key.populate_match_spec(pipe_match_spec);
  match_data.copy_pipe_action_spec(pipe_action_spec);

  // Fill in state from action member id or selector group id in the action spec
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  pipe_mgr_adt_ent_data_t ap_ent_data;
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          act_fn_hdl,
                          &ap_ent_data);

  if (status != BF_SUCCESS) {
    // This implies the action member ID or the selector group ID to which this
    // match entry wants to point to doesn't exist. In the selector group case,
    // the group might be empty, which cannot have a match entry pointing to
    // Its important we output the right error message. Hence check the
    // pipe_action_spec's action entry handle values to discern the exact error
    // 1. If the match entry wants to point to a group ID, if the grp handle in
    // the pipe_action_spec is of an INVALID value, then it implies group does
    // not exist
    // 2. If the group handle is valid and the action entry handle is of an
    // INVALID value, then it implies group exists, but the group is empty
    // 3. If the match entry wants to point to a action member ID, then an
    // invalid value of action entry handle indicates that the member ID does
    // not exist
    if (match_data.isGroup()) {
      if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not have any members in the group table associated with the "
            "table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a action member id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getActionMbrId());
        return BF_OBJECT_NOT_FOUND;
      }
    }
    return BF_UNEXPECTED;
  }

  if (match_data.isGroup()) {
    pipe_action_spec->sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec->adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, pipe_action_spec);

  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionIndirectTableData &match_data =
      static_cast<const BfRtMatchActionIndirectTableData &>(data);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  pipe_action_spec_t pipe_action_spec = {0};
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();

  bf_status_t status = populate_entry_data(session,
                                           dev_tgt,
                                           match_key,
                                           match_data,
                                           &pipe_match_spec,
                                           &pipe_action_spec,
                                           &act_fn_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize entry data",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // Ready to add the entry
  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   &pipe_action_spec,
                                   ttl,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             &pipe_match_spec,
                                             &pipe_entry_hdl,
                                             false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryModInternal(
      *this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryAddOrMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data,
    bool *is_added) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionIndirectTableData &match_data =
      static_cast<const BfRtMatchActionIndirectTableData &>(data);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  pipe_action_spec_t pipe_action_spec = {0};
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();

  bf_status_t status = populate_entry_data(session,
                                           dev_tgt,
                                           match_key,
                                           match_data,
                                           &pipe_match_spec,
                                           &pipe_action_spec,
                                           &act_fn_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize entry data",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // Ready to add the entry
  return pipeMgr->pipeMgrMatEntAddOrMod(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        act_fn_hdl,
                                        &pipe_action_spec,
                                        ttl,
                                        0 /* Pipe API flags */,
                                        &pipe_entry_hdl,
                                        is_added);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  uint32_t pipe_flags = 0;
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_IGNORE_NOT_FOUND)) {
    pipe_flags = PIPE_FLAG_IGNORE_NOT_FOUND;
  }

  return pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                              pipe_dev_tgt,
                                              pipe_tbl_hdl,
                                              &pipe_match_spec,
                                              pipe_flags);
}

bf_status_t BfRtMatchActionIndirectTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        true /*light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  bf_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_handle);

  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = tableEntryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d Failed to get device state for dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
  nextRef->setRef(session.sessHandleGet(), dev_tgt.pipe_id, first_entry_handle);

  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  return get_next_n_entries(
      *this, session, dev_tgt, flags, key, n, key_data_pairs, num_returned);
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionIndirectTableData &match_data =
      static_cast<const BfRtMatchActionIndirectTableData &>(data);
  pipe_action_spec_t pipe_action_spec = {0};

  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;

  pipe_mgr_adt_ent_data_t ap_ent_data;
  // Fill in state from action member id or selector group id in the action spec
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &ap_ent_data);

  if (status != BF_SUCCESS) {
    if (status == BF_TABLE_LOCKED) return status;
    // This implies the action member ID or the selector group ID to which this
    // match entry wants to point to doesn't exist. In the selector group case,
    // the group might be empty, which cannot have a match entry pointing to
    // Its important we output the right error message. Hence check the
    // pipe_action_spec's action entry handle values to discern the exact error
    // 1. If the match entry wants to point to a group ID, if the grp handle in
    // the pipe_action_spec is of an INVALID value, then it implies group does
    // not exist
    // 2. If the group handle is valid and the action entry handle is of an
    // INVALID value, then it implies group exists, but the group is empty
    // 3. If the match entry wants to point to a action member ID, then an
    // invalid value of action entry handle indicates that the member ID does
    // not exist
    if (match_data.isGroup()) {
      if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which does not exist in the group table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which does not exist in the group table associated with the table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a action member "
            "id %d which does not exist in the action profile table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getActionMbrId());
        return BF_OBJECT_NOT_FOUND;
      }
    }
    return BF_UNEXPECTED;
  }

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, &pipe_action_spec);

  return pipeMgr->pipeMgrMatDefaultEntrySet(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            act_fn_hdl,
                                            &pipe_action_spec,
                                            0 /* Pipe API flags */,
                                            &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &pipe_entry_hdl);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  if (this->has_const_default_action_) {
    // If default action is const, then this API is a no-op
    LOG_DBG(
        "%s:%d %s Calling reset on a table with const "
        "default action",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_SUCCESS;
  }
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, 0);
}

bf_status_t BfRtMatchActionIndirectTable::getActionState(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtMatchActionIndirectTableData *data,
    pipe_adt_ent_hdl_t *adt_entry_hdl,
    pipe_sel_grp_hdl_t *sel_grp_hdl,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  if (!data->isGroup()) {
    // Safe to do a static cast here since all table objects are constructed by
    // our factory and the right kind of sub-object is constructed by the
    // factory depending on the table type. Here the actProfTbl member of the
    // table object is a pointer to the action profile table associated with the
    // match-action table. It is guaranteed to be  of type BfRtActionTable

    status = actTbl->getMbrState(session,
                                 dev_tgt,
                                 data->getActionMbrId(),
                                 act_fn_hdl,
                                 adt_entry_hdl,
                                 ap_ent_data);
    if (status != BF_SUCCESS) {
      *adt_entry_hdl =
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl;
      return status;
    }
  } else {
    BfRtSelectorTable *selTbl = static_cast<BfRtSelectorTable *>(selectorTbl);
    status =
        selTbl->getGrpHdl(session, dev_tgt, data->getGroupId(), sel_grp_hdl);
    if (status != BF_SUCCESS) {
      *sel_grp_hdl = BfRtMatchActionIndirectTableData::invalid_group;
      return BF_OBJECT_NOT_FOUND;
    }
    status =
        selTbl->getOneMbr(session, dev_tgt.dev_id, *sel_grp_hdl, adt_entry_hdl);
    if (status != BF_SUCCESS) {
      *adt_entry_hdl =
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl;
      return BF_OBJECT_NOT_FOUND;
    }

    bf_rt_id_t a_member_id = 0;
    status =
        getActionMbrIdFromHndl(session, dev_tgt, *adt_entry_hdl, &a_member_id);
    if (status != BF_SUCCESS) {
      return BF_OBJECT_NOT_FOUND;
    }

    status = actTbl->getMbrState(
        session, dev_tgt, a_member_id, act_fn_hdl, adt_entry_hdl, ap_ent_data);
  }
  return status;
}

void BfRtMatchActionIndirectTable::setIsTernaryTable(
    const bf_dev_id_t &dev_id) {
  auto status = PipeMgrIntf::getInstance()->pipeMgrTblIsTern(
      dev_id, pipe_tbl_hdl, &this->is_ternary_table_);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to find whether table is ternary or not",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
}

bf_status_t BfRtMatchActionIndirectTable::getActionMbrIdFromHndl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    bf_rt_id_t *mbr_id) const {
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, mbr_id);
}

bf_status_t BfRtMatchActionIndirectTable::getGroupIdFromHndl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_sel_grp_hdl_t sel_grp_hdl,
    bf_rt_id_t *grp_id) const {
  BfRtSelectorTable *selTbl = static_cast<BfRtSelectorTable *>(selectorTbl);
  return selTbl->getGrpIdFromHndl(session, dev_tgt, sel_grp_hdl, grp_id);
}

bf_status_t BfRtMatchActionIndirectTable::tableSizeGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    size_t *count) const {
  bf_status_t status = BF_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->_table_size - reserved;
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtMatchActionIndirectTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtMatchActionIndirectTable, BfRtMatchActionKey>(*this,
                                                                     match_key);
}

bf_status_t BfRtMatchActionIndirectTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionIndirectTable::dataReset(
    const std::vector<bf_rt_id_t> &fields, BfRtTableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type == TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE(
        "%s:%d %s Idle Table Runtime Attribute requires a Mode, please use the "
        "appropriate API to include the idle table mode",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type, idle_table_mode));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

bf_status_t BfRtMatchActionIndirectTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<64> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ullong();
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      if (sts == BF_SUCCESS) {
        auto device_state =
            BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
        if (device_state == nullptr) {
          BF_RT_ASSERT(0);
          return BF_OBJECT_NOT_FOUND;
        }

        // Set ENTRY_SCOPE of the table
        auto attributes_state =
            device_state->attributesState.getObjState(table_id_get());
        attributes_state->setEntryScope(entry_scope);
      }
      return sts;
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> field_mask;
      bf_status_t sts = tbl_attr_impl->dynKeyMaskGet(&field_mask);
      pipe_tbl_match_spec_t match_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      match_spec.match_mask_bits = match_mask_bits.data();
      // For this case value_bits will be modified the same way as mask, but
      // cannot be left uninitialized.
      match_spec.match_value_bits = match_mask_bits.data();
      // translate from map to match_spec
      sts = fieldMasksToMatchSpec(field_mask, &match_spec, this);
      if (sts != BF_SUCCESS) return sts;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrMatchKeyMaskSpecSet(
          session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &match_spec);
      return sts;
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return setIdleTable(const_cast<BfRtMatchActionIndirectTable &>(*this),
                          session,
                          dev_tgt,
                          *tbl_attr_impl,
                          flags);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      pipe_tbl_match_spec_t mat_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      mat_spec.match_mask_bits = match_mask_bits.data();
      mat_spec.num_match_bytes = sz;
      mat_spec.num_valid_match_bits = this->getKeySize().bits;

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMatchKeyMaskSpecGet(
          session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &mat_spec);
      std::unordered_map<bf_rt_id_t, std::vector<uint8_t>> field_mask;
      matchSpecToFieldMasks(mat_spec, &field_mask, this);
      return tbl_attr_impl->dynKeyMaskSet(field_mask);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return getIdleTable(*this, session, dev_tgt, tbl_attr_impl);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "get "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

// Unexposed functions
bf_status_t BfRtMatchActionIndirectTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;

  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_res_get_data_t res_data = {0};

  uint32_t res_get_flags = 0;
  res_data.stful.data = nullptr;

  BfRtMatchActionIndirectTableData *match_data =
      static_cast<BfRtMatchActionIndirectTableData *>(data);
  std::vector<bf_rt_id_t> dataFields;
  bool all_fields_set = match_data->allFieldsSet();

  bf_rt_id_t req_action_id = 0;
  status = match_data->actionIdGet(&req_action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
  } else {
    dataFields.assign(match_data->getActiveFields().begin(),
                      match_data->getActiveFields().end());
    for (const auto &dataFieldId : match_data->getActiveFields()) {
      const BfRtTableDataField *tableDataField = nullptr;
      status = getDataField(dataFieldId, &tableDataField);
      BF_RT_ASSERT(status == BF_SUCCESS);
      auto fieldTypes = tableDataField->getTypes();
      fieldDestination field_destination =
          BfRtTableDataField::getDataFieldDestination(fieldTypes);
      switch (field_destination) {
        case fieldDestination::DIRECT_LPF:
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
          res_get_flags |= PIPE_RES_GET_FLAG_METER;
          break;
        case fieldDestination::DIRECT_REGISTER:
          res_get_flags |= PIPE_RES_GET_FLAG_STFUL;
          break;
        case fieldDestination::ACTION_SPEC:
          res_get_flags |= PIPE_RES_GET_FLAG_ENTRY;
          break;
        case fieldDestination::DIRECT_COUNTER:
          res_get_flags |= PIPE_RES_GET_FLAG_CNTR;
          break;
        case fieldDestination::ENTRY_HIT_STATE:
        case fieldDestination::TTL:
          res_get_flags |= PIPE_RES_GET_FLAG_IDLE;
          break;
        default:
          break;
      }
    }
  }
  // All inputs from the data object have been processed. Now reset it
  // for out data purpose
  // We reset the data object with act_id 0 and all fields
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handle %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return status;
  }
  return populate_data_fields(
      *this, session, dev_tgt, res_data, pipe_act_fn_hdl, data);
}

bf_status_t BfRtMatchActionIndirectTable::registerMatUpdateCb(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const pipe_mat_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterMatUpdateCb(session.sessHandleGet(),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}

// BfRtActionTable

bf_status_t BfRtActionTable::tableEntryAdd(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  const BfRtActionTableData &action_tbl_data =
      static_cast<const BfRtActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_adt_ent_hdl_t adt_entry_hdl;
  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrAdtEntAdd(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     act_fn_hdl,
                                     mbr_id,
                                     pipe_action_spec,
                                     &adt_entry_hdl,
                                     pipe_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Error adding new ADT %d entry with mbr_id %d.",
              __func__,
              __LINE__,
              table_id_get(),
              mbr_id);
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryMod(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  const BfRtActionTableData &action_tbl_data =
      static_cast<const BfRtActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();

  // Action entry handle doesn't change during a modify
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl,
                                        false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntSet(session.sessHandleGet(),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     act_fn_hdl,
                                     pipe_action_spec,
                                     0 /* Pipe API flags */);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in modifying action profile member for tbl id %d "
        "member id %d, err %s",
        __func__,
        __LINE__,
        table_id_get(),
        mbr_id,
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryAddOrMod(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t & /*flags*/,
                                                const BfRtTableKey &key,
                                                const BfRtTableData &data,
                                                bool *is_added) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  const BfRtActionTableData &action_tbl_data =
      static_cast<const BfRtActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();

  // Action entry handle doesn't change during a modify
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl,
                                        true);
  if (status == BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d already exist for tbl 0x%x",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());

    status = pipeMgr->pipeMgrAdtEntSet(session.sessHandleGet(),
                                       pipe_dev_tgt.device_id,
                                       pipe_tbl_hdl,
                                       adt_ent_hdl,
                                       act_fn_hdl,
                                       pipe_action_spec,
                                       0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d Error in modifying action profile member for tbl id %d "
          "member id %d, err %s",
          __func__,
          __LINE__,
          table_id_get(),
          mbr_id,
          pipe_str_err((pipe_status_t)status));
    }

    if (is_added) *is_added = false;
    return status;
  }

  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrAdtEntAdd(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     act_fn_hdl,
                                     mbr_id,
                                     pipe_action_spec,
                                     &adt_ent_hdl,
                                     pipe_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Error adding new ADT %d entry with mbr_id %d.",
              __func__,
              __LINE__,
              table_id_get(),
              mbr_id);
  }
  if (is_added) *is_added = true;
  return status;
}

bf_status_t BfRtActionTable::tableEntryDel(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  bool check_only = false;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_IGNORE_NOT_FOUND)) {
    check_only = true;
  }
  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl,
                                        check_only);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntDel(session.sessHandleGet(),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     0 /* Pipe api flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in deletion of action profile member %d for tbl id %d "
        ", err %s",
        __func__,
        __LINE__,
        mbr_id,
        table_id_get(),
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

bf_status_t BfRtActionTable::tableClear(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  while (BF_SUCCESS ==
         pipeMgr->pipeMgrGetFirstEntryHandle(session.sessHandleGet(),
                                             pipe_tbl_hdl,
                                             pipe_dev_tgt,
                                             (int *)&adt_ent_hdl)) {
    status = pipeMgr->pipeMgrAdtEntDel(session.sessHandleGet(),
                                       pipe_dev_tgt.device_id,
                                       pipe_tbl_hdl,
                                       adt_ent_hdl,
                                       0 /* Pipe api flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d Error deleting action profile member for tbl id 0x%x "
          "handle %d, err %s",
          __func__,
          __LINE__,
          table_id_get(),
          adt_ent_hdl,
          pipe_str_err((pipe_status_t)status));
      return status;
    }
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key,
                                           BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  BfRtActionTableData *action_tbl_data =
      static_cast<BfRtActionTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  pipe_adt_ent_hdl_t adt_ent_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl,
                                        false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Action member Id %d does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mbr_id);
    return BF_OBJECT_NOT_FOUND;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, adt_ent_hdl, action_tbl_data);
}

bf_status_t BfRtActionTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtActionTableKey *action_tbl_key = static_cast<BfRtActionTableKey *>(key);
  bf_rt_id_t mbr_id;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.sessHandleGet(),
                                               pipe_dev_tgt.device_id,
                                               pipe_tbl_hdl,
                                               entry_handle,
                                               &mbr_id,
                                               &pipe_dev_tgt.dev_pipe_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  action_tbl_key->setMemberId(mbr_id);
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = pipe_dev_tgt.dev_pipe_id;
  return status;
}

bf_status_t BfRtActionTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             mbr_id,
                                             entry_handle,
                                             false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const bf_rt_handle_t &entry_handle,
                                           BfRtTableKey *key,
                                           BfRtTableData *data) const {
  bf_rt_target_t entry_tgt;
  bf_status_t status = this->tableEntryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != BF_SUCCESS) {
    return status;
  }
  return this->tableEntryGet(session, entry_tgt, flags, *key, data);
}

bf_status_t BfRtActionTable::tableEntryGetFirst(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                BfRtTableKey *key,
                                                BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtActionTableKey *action_tbl_key = static_cast<BfRtActionTableKey *>(key);
  BfRtActionTableData *action_tbl_data =
      static_cast<BfRtActionTableData *>(data);

  bf_rt_id_t first_mbr_id;
  int first_entry_hdl;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_hdl);

  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.sessHandleGet(),
                                          pipe_dev_tgt.device_id,
                                          pipe_tbl_hdl,
                                          first_entry_hdl,
                                          &first_mbr_id,
                                          &pipe_dev_tgt.dev_pipe_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first entry member id, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  bf_rt_target_t ent_dev_tgt = dev_tgt;
  ent_dev_tgt.pipe_id = pipe_dev_tgt.dev_pipe_id;

  status = tableEntryGet_internal(
      session, ent_dev_tgt, flags, first_entry_hdl, action_tbl_data);
  if (status != BF_SUCCESS) {
    return status;
  }
  action_tbl_key->setMemberId(first_mbr_id);
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::tableEntryGetNext_n(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 const BfRtTableKey &key,
                                                 const uint32_t &n,
                                                 keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  bf_rt_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &pipe_entry_hdl,
                                        false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return BF_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    bf_rt_id_t next_mbr_id;
    bf_dev_pipe_t next_mbr_pipe;
    // Get the action entry handle used by pipe-mgr from the member id
    status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.sessHandleGet(),
                                            pipe_dev_tgt.device_id,
                                            pipe_tbl_hdl,
                                            next_entry_handles[i],
                                            &next_mbr_id,
                                            &next_mbr_pipe);
    if (status) break;
    auto this_key =
        static_cast<BfRtActionTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtActionTableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    bf_rt_target_t mbr_dev_tgt = dev_tgt;
    mbr_dev_tgt.pipe_id = next_mbr_pipe;
    status = tableEntryGet_internal(
        session, mbr_dev_tgt, flags, next_entry_handles[i], this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with entry "
          "handle %d, mbr id %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          i + 1,
          next_entry_handles[i],
          next_mbr_id,
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setMemberId(next_mbr_id);
  }
  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::tableUsageGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtActionTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtActionTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtActionTableData(this, action_id));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // This dataAllocate is mainly used for entry gets from the action table
  // wherein  the action id of the entry is not known and will be filled in by
  // the entry get
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtActionTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::getMbrState(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t mbr_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto status = pipeMgr->pipeMgrAdtEntDataGet(session.sessHandleGet(),
                                              pipe_dev_tgt,
                                              pipe_tbl_hdl,
                                              mbr_id,
                                              adt_ent_hdl,
                                              ap_ent_data);
  *act_fn_hdl = ap_ent_data->act_fn_hdl;

  return status;
}

bf_status_t BfRtActionTable::getMbrIdFromHndl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    bf_rt_id_t *mbr_id) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrAdtEntMbrIdGet(session.sessHandleGet(),
                                        pipe_dev_tgt.device_id,
                                        pipe_tbl_hdl,
                                        adt_ent_hdl,
                                        mbr_id,
                                        &pipe_dev_tgt.dev_pipe_id);
}

bf_status_t BfRtActionTable::getHdlFromMbrId(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrAdtEntHdlGet(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      mbr_id,
                                      adt_ent_hdl,
                                      false);
}

bf_status_t BfRtActionTable::registerAdtUpdateCb(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 const pipe_adt_update_cb &cb,
                                                 const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterAdtUpdateCb(session.sessHandleGet(),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}

bf_status_t BfRtActionTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_adt_ent_hdl_t &entry_hdl,
    BfRtActionTableData *action_tbl_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_action_spec_t *action_spec = action_tbl_data->mutable_pipe_action_spec();
  bool read_from_hw = false;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    read_from_hw = true;
  }
  pipe_act_fn_hdl_t act_fn_hdl;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  bf_status_t status =
      pipeMgr->pipeMgrGetActionDataEntry(pipe_tbl_hdl,
                                         pipe_dev_tgt,
                                         entry_hdl,
                                         &action_spec->act_data,
                                         &act_fn_hdl,
                                         read_from_hw);
  // At this point, if a member wasn't found, there is a high chance
  // that the action data wasn't programmed in the hw itself because by
  // this time BF-RT sw state check has passed. So try it once again with
  // with read_from_hw = False
  if (status == BF_OBJECT_NOT_FOUND && read_from_hw) {
    status = pipeMgr->pipeMgrGetActionDataEntry(pipe_tbl_hdl,
                                                pipe_dev_tgt,
                                                entry_hdl,
                                                &action_spec->act_data,
                                                &act_fn_hdl,
                                                false);

    // Able to fetch Action data with read_from_hw = False indicate members are
    // not progarmmed to HW, send error message to indicate this state
    if (status == BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting action data HW entry from pipemgr",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_NO_HW_ENTRY;
    }
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting action data from pipe-mgr, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(act_fn_hdl);

  action_tbl_data->actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  action_tbl_data->setActiveFields(empty);
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::keyReset(BfRtTableKey *key) const {
  BfRtActionTableKey *action_key = static_cast<BfRtActionTableKey *>(key);
  return key_reset<BfRtActionTable, BfRtActionTableKey>(*this, action_key);
}

bf_status_t BfRtActionTable::dataReset(BfRtTableData *data) const {
  BfRtActionTableData *data_obj = static_cast<BfRtActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(0);
}

bf_status_t BfRtActionTable::dataReset(const bf_rt_id_t &action_id,
                                       BfRtTableData *data) const {
  BfRtActionTableData *data_obj = static_cast<BfRtActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(action_id);
}

// BfRtSelectorTable **************
bf_status_t BfRtSelectorTable::registerSelUpdateCb(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const pipe_sel_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeRegisterSelUpdateCb(session.sessHandleGet(),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}
bf_status_t BfRtSelectorTable::getActMbrIdFromHndl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_adt_ent_hdl_t &adt_ent_hdl,
    bf_rt_id_t *act_mbr_id) const {
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, act_mbr_id);
}

bf_status_t BfRtSelectorTable::tableEntryAdd(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  const BfRtSelectorTableData &sel_data =
      static_cast<const BfRtSelectorTableData &>(data);
  // Make a call to pipe-mgr to first create the group
  pipe_sel_grp_hdl_t sel_grp_hdl;
  uint32_t max_grp_size = sel_data.get_max_grp_size();
  uint32_t adt_offset = sel_data.get_adt_offset();
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  std::vector<bf_rt_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();

  if (members.size() != member_status.size()) {
    LOG_TRACE("%s:%d MemberId size %zu and member status size %zu do not match",
              __func__,
              __LINE__,
              members.size(),
              member_status.size());
    return BF_INVALID_ARG;
  }

  // Before we add the group, we first check the validity of the members to be
  // added if any and build up a vector of action entry handles and action
  // function handles to be used to pass to the pipe-mgr API
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  for (unsigned i = 0; i < members.size(); ++i) {
    // For each member verify if the member ID specified exists. If so, get the
    // action function handle
    pipe_adt_ent_hdl_t adt_ent_hdl = 0;

    BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which does not exist into "
          "group id %d on pipe %x",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          members[i],
          sel_grp_id,
          dev_tgt.pipe_id);
      return BF_INVALID_ARG;
    }

    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  // Now, attempt to add the group;
  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrSelGrpAdd(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     sel_grp_id,
                                     max_grp_size,
                                     adt_offset,
                                     &sel_grp_hdl,
                                     pipe_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding group id %d pipe %x, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sel_grp_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }

  // Set the membership of the group
  status = pipeMgr->pipeMgrSelGrpMbrsSet(session.sessHandleGet(),
                                         pipe_dev_tgt.device_id,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         members.size(),
                                         action_entry_hdls.data(),
                                         (bool *)(pipe_member_status.data()),
                                         0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in setting membership for group id %d pipe %x, err "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                              pipe_dev_tgt.device_id,
                              pipe_tbl_hdl,
                              sel_grp_hdl,
                              0 /* Pipe API flags */);
  }

  return status;
}

bf_status_t BfRtSelectorTable::tableEntryMod(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  const BfRtSelectorTableData &sel_data =
      static_cast<const BfRtSelectorTableData &>(data);

  std::vector<bf_rt_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  if (members.size() != member_status.size()) {
    LOG_TRACE("%s:%d MemberId size %zu and member status size %zu do not match",
              __func__,
              __LINE__,
              members.size(),
              member_status.size());
    return BF_INVALID_ARG;
  }

  // Get the mapping from selector group id to selector group handle

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &sel_grp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  uint32_t curr_size;
  uint32_t adt_offset;
  status = pipeMgr->pipeMgrSelGrpParamsGet(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           &curr_size,
                                           &adt_offset);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting max grp size, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  // Next, validate the member IDs
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  for (unsigned i = 0; i < members.size(); ++i) {
    pipe_adt_ent_hdl_t adt_ent_hdl;
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which not exist into group "
          "id %d pipe %x",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          members[i],
          sel_grp_id,
          dev_tgt.pipe_id);
      return BF_INVALID_ARG;
    }
    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  bool membrs_set = false;
  // If new members will fit current size, set members first to support
  // downsizing of the group.
  if (curr_size >= members.size()) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(session.sessHandleGet(),
                                           pipe_dev_tgt.device_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           members.size(),
                                           action_entry_hdls.data(),
                                           (bool *)(pipe_member_status.data()),
                                           0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
    membrs_set = true;
  }
  const auto max_grp_size = sel_data.get_max_grp_size();
  bool size_changed = false;
  // Size of 0 is ignored, means no change in size.
  if (max_grp_size != 0 && curr_size != max_grp_size) {
    status = pipeMgr->pipeMgrSelGrpSizeSet(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           max_grp_size);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting group size for id %d pipe %x, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
    size_changed = true;
  }
  if (membrs_set == false) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(session.sessHandleGet(),
                                           pipe_dev_tgt.device_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           members.size(),
                                           action_entry_hdls.data(),
                                           (bool *)(pipe_member_status.data()),
                                           0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      if (size_changed) {
        // Restore group size
        auto sts = pipeMgr->pipeMgrSelGrpSizeSet(session.sessHandleGet(),
                                                 pipe_dev_tgt,
                                                 pipe_tbl_hdl,
                                                 sel_grp_hdl,
                                                 curr_size);
        if (sts != PIPE_SUCCESS) {
          LOG_TRACE(
              "%s:%d %s Unable to restore original size for group id %d pipe "
              "%x, err "
              "%d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sel_grp_id,
              dev_tgt.pipe_id,
              sts);
        }
      }
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryAddOrMod(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t & /*flags*/,
                                                  const BfRtTableKey &key,
                                                  const BfRtTableData &data,
                                                  bool *is_added) const {
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);

  bf_rt_id_t sel_grp_id = sel_key.getGroupId();
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  bf_status_t status = getGrpHdl(session, dev_tgt, sel_grp_id, &sel_grp_hdl);
  if (is_added) *is_added = false;
  if (status != BF_SUCCESS) {
    if (is_added) *is_added = true;
    return tableEntryAdd(session, dev_tgt, 0, key, data);
  }
  return tableEntryMod(session, dev_tgt, 0, key, data);
}

bf_status_t BfRtSelectorTable::tableEntryDel(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &sel_grp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  status = pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     sel_grp_hdl,
                                     0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error deleting selector group %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sel_grp_id,
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableClear(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/
                                          ) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  while (BF_SUCCESS ==
         pipeMgr->pipeMgrGetFirstEntryHandle(session.sessHandleGet(),
                                             pipe_tbl_hdl,
                                             pipe_dev_tgt,
                                             (int *)&sel_grp_hdl)) {
    status = pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                                       pipe_dev_tgt.device_id,
                                       pipe_tbl_hdl,
                                       sel_grp_hdl,
                                       0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error deleting selector group %d pipe %x, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                sel_grp_hdl,
                dev_tgt.pipe_id,
                status);
      return status;
    }
  }
  return status;
}

bf_status_t BfRtSelectorTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_id_t &grp_id,
    BfRtSelectorTableData *sel_tbl_data,
    bool from_hw) const {
  bf_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        grp_id,
                                        &sel_grp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  // Get the max size configured for the group
  uint32_t max_grp_size = 0;
  uint32_t adt_offset = 0;
  status = pipeMgr->pipeMgrSelGrpParamsGet(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           &max_grp_size,
                                           &adt_offset);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get size for Grp Id %d on pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  // Query pipe mgr for member and status list
  uint32_t count = 0;
  status = pipeMgr->pipeMgrGetSelGrpMbrCount(session.sessHandleGet(),
                                             dev_tgt.dev_id,
                                             pipe_tbl_hdl,
                                             sel_grp_hdl,
                                             &count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get info for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<pipe_adt_ent_hdl_t> pipe_members(count, 0);
  std::vector<char> pipe_member_status(count, 0);
  uint32_t mbrs_populated = 0;
  status = pipeMgr->pipeMgrSelGrpMbrsGet(session.sessHandleGet(),
                                         dev_tgt.dev_id,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         count,
                                         pipe_members.data(),
                                         (bool *)(pipe_member_status.data()),
                                         &mbrs_populated,
                                         from_hw);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get membership for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<bf_rt_id_t> member_ids;
  std::vector<bool> member_id_status;
  for (unsigned i = 0; i < mbrs_populated; i++) {
    bf_rt_id_t member_id = 0;
    status = getActMbrIdFromHndl(session, dev_tgt, pipe_members[i], &member_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting member id for member hdl %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pipe_members[i]);
      return BF_INVALID_ARG;
    }

    member_ids.push_back(member_id);
    member_id_status.push_back(pipe_member_status[i]);
  }
  sel_tbl_data->setMembers(member_ids);
  sel_tbl_data->setMemberStatus(member_id_status);
  sel_tbl_data->setMaxGrpSize(max_grp_size);
  sel_tbl_data->setAdtOffset(adt_offset);
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             BfRtTableData *data) const {
  bool from_hw = false;
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    from_hw = true;
  }

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  BfRtSelectorTableData *sel_tbl_data =
      static_cast<BfRtSelectorTableData *>(data);
  bf_rt_id_t grp_id = sel_tbl_key.getGroupId();

  return tableEntryGet_internal(
      session, dev_tgt, grp_id, sel_tbl_data, from_hw);
}

bf_status_t BfRtSelectorTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtSelectorTableKey *sel_tbl_key = static_cast<BfRtSelectorTableKey *>(key);
  bf_rt_id_t grp_id;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrSelGrpIdGet(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            entry_handle,
                                            &grp_id);
  if (status != BF_SUCCESS) return status;
  sel_tbl_key->setGroupId(grp_id);
  *entry_tgt = dev_tgt;
  return status;
}

bf_status_t BfRtSelectorTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  bf_rt_id_t sel_grp_id = sel_tbl_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      sel_grp_id,
                                      entry_handle);
}

bf_status_t BfRtSelectorTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const bf_rt_handle_t &entry_handle,
                                             BfRtTableKey *key,
                                             BfRtTableData *data) const {
  bf_rt_target_t entry_tgt;
  bf_status_t status = this->tableEntryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != BF_SUCCESS) {
    return status;
  }
  return this->tableEntryGet(session, entry_tgt, flags, *key, data);
}

bf_status_t BfRtSelectorTable::tableEntryGetFirst(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t &flags,
                                                  BfRtTableKey *key,
                                                  BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported."
        " Defaulting to sw read",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, (int *)&sel_grp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first handle, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return this->tableEntryGet(session, dev_tgt, flags, sel_grp_hdl, key, data);
}

bf_status_t BfRtSelectorTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t sel_grp_id = sel_tbl_key.getGroupId();
  bf_rt_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Grp Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              sel_grp_id,
              table_id_get());
    return status;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return BF_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    if (next_entry_handles[i] == -1) {
      break;
    }
    auto this_key = static_cast<BfRtTableKey *>((*key_data_pairs)[i].first);
    auto this_data = static_cast<BfRtTableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d does not match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    status = tableEntryGet(
        session, dev_tgt, flags, next_entry_handles[i], this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with group "
          "handle %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          i + 1,
          next_entry_handles[i],
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
  }
  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::getOneMbr(const BfRtSession &session,
                                         const uint16_t device_id,
                                         const pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t *member_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeMgrGetFirstGroupMember(session.sessHandleGet(),
                                             pipe_tbl_hdl,
                                             device_id,
                                             sel_grp_hdl,
                                             member_hdl);
}

bf_status_t BfRtSelectorTable::getGrpIdFromHndl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_sel_grp_hdl_t &sel_grp_hdl,
    bf_rt_id_t *sel_grp_id) const {
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpIdGet(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     sel_grp_hdl,
                                     sel_grp_id);
}

bf_status_t BfRtSelectorTable::getGrpHdl(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_id_t sel_grp_id,
    pipe_sel_grp_hdl_t *sel_grp_hdl) const {
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      sel_grp_id,
                                      sel_grp_hdl);
}

bf_status_t BfRtSelectorTable::tableUsageGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtSelectorTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtSelectorTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::keyReset(BfRtTableKey *key) const {
  BfRtSelectorTableKey *sel_key = static_cast<BfRtSelectorTableKey *>(key);
  return key_reset<BfRtSelectorTable, BfRtSelectorTableKey>(*this, sel_key);
}

bf_status_t BfRtSelectorTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  const std::vector<bf_rt_id_t> fields{};
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              table_name_get().c_str())
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              table_name_get().c_str())
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::dataReset(BfRtTableData *data) const {
  BfRtSelectorTableData *sel_data = static_cast<BfRtSelectorTableData *>(data);
  if (!this->validateTable_from_dataObj(*sel_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return sel_data->reset();
}

bf_status_t BfRtSelectorTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtSelectorTable::processSelUpdateCbAttr(
    const BfRtTableAttributesImpl &tbl_attr_impl,
    const bf_rt_target_t &dev_tgt) const {
  // 1. From the table attribute object, get the selector update parameters.
  // 2. Make a table attribute state object to store the parameters that are
  // required for when the callback is invoked.
  // 3. Invoke pipe-mgr callback registration function to register BF-RT
  // internal callback.
  auto t = tbl_attr_impl.selectorUpdateCbInternalGet();

  auto enable = std::get<0>(t);
  auto session = std::get<1>(t);
  auto cpp_callback_fn = std::get<2>(t);
  auto c_callback_fn = std::get<3>(t);
  auto cookie = std::get<4>(t);

  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto session_obj = session.lock();

  if (session_obj == nullptr) {
    LOG_TRACE("%s:%d %s ERROR Session object passed no longer exists",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(table_id_get());

  BfRtStateSelUpdateCb sel_update_cb(
      enable, this, session, cpp_callback_fn, c_callback_fn, cookie);

  attributes_state->setSelUpdateCbObj(sel_update_cb);

  auto *pipeMgr = PipeMgrIntf::getInstance(*session_obj);
  bf_status_t status = pipeMgr->pipeMgrSelTblRegisterCb(
      session_obj->sessHandleGet(),
      dev_tgt.dev_id,
      pipe_tbl_hdl,
      selUpdatePipeMgrInternalCb,
      &(attributes_state->getSelUpdateCbObj()));

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in registering selector update callback with pipe-mgr, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    // Reset the selector update callback object, since pipeMgr registration did
    // not succeed
    attributes_state->resetSelUpdateCbObj();
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableAttributesSet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();

  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    return BF_INVALID_ARG;
  }
  return this->processSelUpdateCbAttr(*tbl_attr_impl, dev_tgt);
}

bf_status_t BfRtSelectorTable::tableAttributesGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // 1. From the table attribute state, retrieve all the params that were
  // registered by the user
  // 2. Set the params in the passed in tableAttributes obj

  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    return BF_INVALID_ARG;
  }

  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(table_id_get());
  BfRtStateSelUpdateCb sel_update_cb;
  attributes_state->getSelUpdateCbObj(&sel_update_cb);

  // Set the state in the attribute object
  auto state_param = sel_update_cb.stateGet();
  tbl_attr_impl->selectorUpdateCbInternalSet(
      std::make_tuple(std::get<0>(state_param),
                      std::get<2>(state_param),
                      std::get<3>(state_param),
                      std::get<4>(state_param),
                      std::get<5>(state_param)));

  return BF_SUCCESS;
}
// COUNTER TABLE APIS

bf_status_t BfRtCounterTable::tableEntryAdd(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  const BfRtCounterTableData &cntr_data =
      static_cast<const BfRtCounterTableData &>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  const pipe_stat_data_t *stat_data =
      cntr_data.getCounterSpecObj().getPipeCounterSpec();
  status =
      pipeMgr->pipeMgrStatEntSet(session.sessHandleGet(),
                                 pipe_dev_tgt,
                                 pipe_tbl_hdl,
                                 counter_id,
                                 const_cast<pipe_stat_data_t *>(stat_data));

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying counter index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              counter_id,
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryMod(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtCounterTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  BfRtCounterTableData *cntr_data = static_cast<BfRtCounterTableData *>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrStatEntDatabaseSync(
        session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, counter_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter "
          "idx %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          counter_id,
          status);
      return status;
    }
  }

  pipe_stat_data_t stat_data = {0};
  pipe_stat_data_t *ptr = &stat_data;
  status = pipeMgr->pipeMgrStatEntQuery(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &counter_id,
                                        1,
                                        &ptr);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        counter_id,
        status);
    return status;
  }

  cntr_data->getCounterSpecObj().setCounterDataFromCounterSpec(stat_data);

  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const bf_rt_handle_t &entry_handle,
                                            BfRtTableKey *key,
                                            BfRtTableData *data) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtCounterTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  *entry_handle = cntr_key.getCounterId();
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryGetFirst(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 BfRtTableKey *key,
                                                 BfRtTableData *data) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtCounterTable, BfRtCounterTableKey>(
      *this, session, dev_tgt, flags, cntr_key, data);
}

bf_status_t BfRtCounterTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);

  bf_status_t status = BF_SUCCESS;
  size_t table_size = 0;
  status = this->tableSizeGet(session, dev_tgt, flags, &table_size);
  uint32_t start_key = cntr_key.getIdxKey();

  *num_returned = 0;
  uint32_t i = 0;
  uint32_t j = 0;
  std::vector<pipe_stat_data_t *> stat_data(n);
  std::vector<uint32_t> stat_idx(n);
  for (i = start_key + 1, j = 0; i <= start_key + n; i++, j++) {
    if (i >= table_size) {
      break;
    }
    auto this_key =
        static_cast<BfRtCounterTableKey *>((*key_data_pairs)[j].first);
    this_key->setIdxKey(i);
    stat_idx[j] = i;
    auto this_data =
        static_cast<BfRtCounterTableData *>((*key_data_pairs)[j].second);
    stat_data[j] = this_data->getCounterSpecObj().getPipeCounterSpec();
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrStatDatabaseSync(
        session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, nullptr, nullptr);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter"
          ", err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  status = pipeMgr->pipeMgrStatEntQuery(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        stat_idx.data(),
                                        j,
                                        stat_data.data());

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        1,  // counter_id,
        status);
    return status;
  }
  *num_returned = j;
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableClear(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrStatTableReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing counter table err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtCounterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtCounterTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::keyReset(BfRtTableKey *key) const {
  BfRtCounterTableKey *counter_key = static_cast<BfRtCounterTableKey *>(key);
  return key_reset<BfRtCounterTable, BfRtCounterTableKey>(*this, counter_key);
}

bf_status_t BfRtCounterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtCounterTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::dataReset(BfRtTableData *data) const {
  BfRtCounterTableData *counter_data =
      static_cast<BfRtCounterTableData *>(data);
  if (!this->validateTable_from_dataObj(*counter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return counter_data->reset();
}

// METER TABLE

bf_status_t BfRtMeterTable::tableEntryAdd(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/,
                                          const BfRtTableKey &key,
                                          const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  const BfRtMeterTableData &meter_data =
      static_cast<const BfRtMeterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  const pipe_meter_spec_t *meter_spec =
      meter_data.getMeterSpecObj().getPipeMeterSpec();
  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return BF_INVALID_ARG;
  }

  return pipeMgr->pipeMgrMeterEntSet(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     meter_idx,
                                     (pipe_meter_spec_t *)meter_spec,
                                     0 /* Pipe API flags */);
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryMod(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const BfRtTableKey &key,
                                          const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtMeterTable::tableEntryGet(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const BfRtTableKey &key,
                                          BfRtTableData *data) const {
  bool from_hw = false;

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    from_hw = true;
  }

  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  BfRtMeterTableData *meter_data = static_cast<BfRtMeterTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_meter_spec_t meter_spec;
  std::memset(&meter_spec, 0, sizeof(meter_spec));

  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrMeterReadEntryIdx(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             meter_idx,
                                             &meter_spec,
                                             from_hw);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading meter entry idx %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              meter_idx,
              status);
    return status;
  }

  // Populate data elements right here
  status = meter_data->getMeterSpecObj().setMeterDataFromMeterSpec(meter_spec);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR while populating data elements : err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryGet(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const bf_rt_handle_t &entry_handle,
                                          BfRtTableKey *key,
                                          BfRtTableData *data) const {
  BfRtMeterTableKey *mtr_key = static_cast<BfRtMeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtMeterTable::tableEntryKeyGet(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const bf_rt_handle_t &entry_handle,
                                             bf_rt_target_t *entry_tgt,
                                             BfRtTableKey *key) const {
  BfRtMeterTableKey *mtr_key = static_cast<BfRtMeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMeterTableKey &mtr_key =
      static_cast<const BfRtMeterTableKey &>(key);
  *entry_handle = mtr_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryGetFirst(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               BfRtTableKey *key,
                                               BfRtTableData *data) const {
  BfRtMeterTableKey *meter_key = static_cast<BfRtMeterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtMeterTable, BfRtMeterTableKey>(
      *this, session, dev_tgt, flags, meter_key, data);
}

bf_status_t BfRtMeterTable::tableEntryGetNext_n(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                const uint32_t &n,
                                                keyDataPairs *key_data_pairs,
                                                uint32_t *num_returned) const {
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtMeterTable, BfRtMeterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      meter_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtMeterTable::tableClear(const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrMeterReset(session.sessHandleGet(),
                                                  pipe_dev_tgt,
                                                  this->pipe_tbl_hdl,
                                                  0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing Meter table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtMeterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMeterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::keyReset(BfRtTableKey *key) const {
  BfRtMeterTableKey *meter_key = static_cast<BfRtMeterTableKey *>(key);
  return key_reset<BfRtMeterTable, BfRtMeterTableKey>(*this, meter_key);
}

bf_status_t BfRtMeterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtMeterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::dataReset(BfRtTableData *data) const {
  BfRtMeterTableData *meter_data = static_cast<BfRtMeterTableData *>(data);
  if (!this->validateTable_from_dataObj(*meter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return meter_data->reset();
}

bf_status_t BfRtMeterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMeterTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      bf_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != BF_SUCCESS) {
        return sts;
      }
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      pipeMgr->pipeMgrMeterByteCountGet(
          session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &byte_count);
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

// LPF TABLE

bf_status_t BfRtLPFTable::tableEntryAdd(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey &key,
                                        const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtLPFTableKey &lpf_key = static_cast<const BfRtLPFTableKey &>(key);
  const BfRtLPFTableData &lpf_data =
      static_cast<const BfRtLPFTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_lpf_idx_t lpf_idx = lpf_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, lpf_idx)) {
    return BF_INVALID_ARG;
  }

  return pipeMgr->pipeMgrLpfEntSet(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      lpf_idx,
      const_cast<pipe_lpf_spec_t *>(lpf_data.getLPFSpecObj().getPipeLPFSpec()),
      0 /* Pipe API flags */);
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::tableEntryMod(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const BfRtTableKey &key,
                                        const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtLPFTable::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const BfRtTableKey &key,
                                        BfRtTableData *data) const {
  bool from_hw = false;

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    from_hw = true;
  }

  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtLPFTableKey &lpf_key = static_cast<const BfRtLPFTableKey &>(key);
  BfRtLPFTableData *lpf_data = static_cast<BfRtLPFTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_lpf_spec_t lpf_spec;
  std::memset(&lpf_spec, 0, sizeof(lpf_spec));

  pipe_lpf_idx_t lpf_idx = lpf_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, lpf_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrLpfReadEntryIdx(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           lpf_idx,
                                           &lpf_spec,
                                           from_hw);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading lpf entry idx %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              lpf_idx,
              status);
    return status;
  }

  // Populate data elements right here
  status = lpf_data->getLPFSpecObj().setLPFDataFromLPFSpec(&lpf_spec);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR while populating data elements : err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const bf_rt_handle_t &entry_handle,
                                        BfRtTableKey *key,
                                        BfRtTableData *data) const {
  BfRtLPFTableKey *lpf_key = static_cast<BfRtLPFTableKey *>(key);
  lpf_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtLPFTable::tableEntryKeyGet(const BfRtSession & /*session*/,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const bf_rt_handle_t &entry_handle,
                                           bf_rt_target_t *entry_tgt,
                                           BfRtTableKey *key) const {
  BfRtLPFTableKey *lpf_key = static_cast<BfRtLPFTableKey *>(key);
  lpf_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtLPFTableKey &lpf_key = static_cast<const BfRtLPFTableKey &>(key);
  *entry_handle = lpf_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::tableEntryGetFirst(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             BfRtTableKey *key,
                                             BfRtTableData *data) const {
  BfRtLPFTableKey *lpf_key = static_cast<BfRtLPFTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtLPFTable, BfRtLPFTableKey>(
      *this, session, dev_tgt, flags, lpf_key, data);
}

bf_status_t BfRtLPFTable::tableEntryGetNext_n(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags,
                                              const BfRtTableKey &key,
                                              const uint32_t &n,
                                              keyDataPairs *key_data_pairs,
                                              uint32_t *num_returned) const {
  const BfRtLPFTableKey &lpf_key = static_cast<const BfRtLPFTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtLPFTable, BfRtLPFTableKey>(
      *this, session, dev_tgt, flags, lpf_key, n, key_data_pairs, num_returned);
}

bf_status_t BfRtLPFTable::tableClear(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrLpfReset(session.sessHandleGet(),
                                                pipe_dev_tgt,
                                                this->pipe_tbl_hdl,
                                                0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing MeterLPF table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtLPFTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtLPFTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::keyReset(BfRtTableKey *key) const {
  BfRtLPFTableKey *lpf_key = static_cast<BfRtLPFTableKey *>(key);
  return key_reset<BfRtLPFTable, BfRtLPFTableKey>(*this, lpf_key);
}

bf_status_t BfRtLPFTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtLPFTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtLPFTable::dataReset(BfRtTableData *data) const {
  BfRtLPFTableData *lpf_data = static_cast<BfRtLPFTableData *>(data);
  if (!this->validateTable_from_dataObj(*lpf_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return lpf_data->reset();
}

// WRED TABLE

bf_status_t BfRtWREDTable::tableEntryAdd(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t & /*flags*/,
                                         const BfRtTableKey &key,
                                         const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtWREDTableKey &wred_key = static_cast<const BfRtWREDTableKey &>(key);
  const BfRtWREDTableData &wred_data =
      static_cast<const BfRtWREDTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_wred_idx_t wred_idx = wred_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, wred_idx)) {
    return BF_INVALID_ARG;
  }

  return pipeMgr->pipeMgrWredEntSet(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      wred_idx,
      const_cast<pipe_wred_spec_t *>(
          wred_data.getWREDSpecObj().getPipeWREDSpec()),
      0 /* Pipe API flags */);
}

bf_status_t BfRtWREDTable::tableEntryMod(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t &flags,
                                         const BfRtTableKey &key,
                                         const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtWREDTable::tableEntryGet(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t &flags,
                                         const BfRtTableKey &key,
                                         BfRtTableData *data) const {
  bool from_hw = false;

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    from_hw = true;
  }

  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtWREDTableKey &wred_key = static_cast<const BfRtWREDTableKey &>(key);
  BfRtWREDTableData *wred_data = static_cast<BfRtWREDTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_wred_spec_t wred_spec;
  std::memset(&wred_spec, 0, sizeof(wred_spec));

  pipe_wred_idx_t wred_idx = wred_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, wred_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrWredReadEntryIdx(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            wred_idx,
                                            &wred_spec,
                                            from_hw);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading WRED entry idx %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              wred_idx,
              status);
    return status;
  }

  // Populate data elements right here
  status = wred_data->getWREDSpecObj().setWREDDataFromWREDSpec(&wred_spec);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR while populating data elements : err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtWREDTable::tableEntryGet(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t &flags,
                                         const bf_rt_handle_t &entry_handle,
                                         BfRtTableKey *key,
                                         BfRtTableData *data) const {
  BfRtWREDTableKey *wred_key = static_cast<BfRtWREDTableKey *>(key);
  wred_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtWREDTable::tableEntryKeyGet(const BfRtSession & /*session*/,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const bf_rt_handle_t &entry_handle,
                                            bf_rt_target_t *entry_tgt,
                                            BfRtTableKey *key) const {
  BfRtWREDTableKey *wred_key = static_cast<BfRtWREDTableKey *>(key);
  wred_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtWREDTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtWREDTableKey &wred_key = static_cast<const BfRtWREDTableKey &>(key);
  *entry_handle = wred_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtWREDTable::tableEntryGetFirst(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags,
                                              BfRtTableKey *key,
                                              BfRtTableData *data) const {
  BfRtWREDTableKey *wred_key = static_cast<BfRtWREDTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtWREDTable, BfRtWREDTableKey>(
      *this, session, dev_tgt, flags, wred_key, data);
}

bf_status_t BfRtWREDTable::tableEntryGetNext_n(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               const BfRtTableKey &key,
                                               const uint32_t &n,
                                               keyDataPairs *key_data_pairs,
                                               uint32_t *num_returned) const {
  const BfRtWREDTableKey &wred_key = static_cast<const BfRtWREDTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtWREDTable, BfRtWREDTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      wred_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtWREDTable::tableClear(const BfRtSession &session,
                                      const bf_rt_target_t &dev_tgt,
                                      const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrWredReset(session.sessHandleGet(),
                                                 pipe_dev_tgt,
                                                 this->pipe_tbl_hdl,
                                                 0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing WRED table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtWREDTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtWREDTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtWREDTable::keyReset(BfRtTableKey *key) const {
  BfRtWREDTableKey *wred_key = static_cast<BfRtWREDTableKey *>(key);
  return key_reset<BfRtWREDTable, BfRtWREDTableKey>(*this, wred_key);
}

bf_status_t BfRtWREDTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtWREDTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtWREDTable::dataReset(BfRtTableData *data) const {
  BfRtWREDTableData *wred_data = static_cast<BfRtWREDTableData *>(data);
  if (!this->validateTable_from_dataObj(*wred_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return wred_data->reset();
}

// REGISTER TABLE
bf_status_t BfRtRegisterTable::tableEntryAdd(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  const BfRtRegisterTableData &register_data =
      static_cast<const BfRtRegisterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return BF_INVALID_ARG;
  }

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  std::vector<bf_rt_id_t> dataFields;
  bf_status_t status = dataFieldIdListGet(&dataFields);
  BF_RT_ASSERT(status == BF_SUCCESS);
  const auto &register_spec_data = register_data.getRegisterSpecObj();
  const BfRtTableDataField *tableDataField;
  status = getDataField(dataFields[0], &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : cannot get the data field, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  status = register_spec_data.populateStfulSpecFromData(&stful_spec);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : cannot populate spec, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return pipeMgr->pipeStfulEntSet(session.sessHandleGet(),
                                  pipe_dev_tgt,
                                  pipe_tbl_hdl,
                                  register_idx,
                                  &stful_spec,
                                  0 /* Pipe API flags */);
}

bf_status_t BfRtRegisterTable::tableEntryMod(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtRegisterTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  // Query number of pipes to get possible number of results.
  int num_pipes = 0;
  status = pipeMgr->pipeStfulQueryGetSizes(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &num_pipes);

  // Use vectors to populate pipe mgr stful query data structure.
  // One vector to hold all possible pipe data.
  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;

  uint32_t pipe_api_flags = 0;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeStfulEntQuery(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      register_idx,
                                      &stful_query,
                                      pipe_api_flags);

  if (status == BF_SUCCESS) {
    // Down cast to BfRtRegisterTableData
    BfRtRegisterTableData *register_data =
        static_cast<BfRtRegisterTableData *>(data);
    auto &register_spec_data = register_data->getRegisterSpecObj();
    // pipe_count is returned upon successful query,
    // hence use it instead of vector size.
    register_spec_data.populateDataFromStfulSpec(
        register_pipe_data, static_cast<uint32_t>(stful_query.pipe_count));
  }
  return status;
}

bf_status_t BfRtRegisterTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const bf_rt_handle_t &entry_handle,
                                             BfRtTableKey *key,
                                             BfRtTableData *data) const {
  BfRtRegisterTableKey *reg_key = static_cast<BfRtRegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtRegisterTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtRegisterTableKey *reg_key = static_cast<BfRtRegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtRegisterTableKey &reg_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  *entry_handle = reg_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableEntryGetFirst(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t &flags,
                                                  BfRtTableKey *key,
                                                  BfRtTableData *data) const {
  BfRtRegisterTableKey *register_key = static_cast<BfRtRegisterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtRegisterTable, BfRtRegisterTableKey>(
      *this, session, dev_tgt, flags, register_key, data);
}

bf_status_t BfRtRegisterTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtRegisterTable, BfRtRegisterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      register_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtRegisterTable::tableClear(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeStfulTableReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing register table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtRegisterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtRegisterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::keyReset(BfRtTableKey *key) const {
  BfRtRegisterTableKey *register_key = static_cast<BfRtRegisterTableKey *>(key);
  return key_reset<BfRtRegisterTable, BfRtRegisterTableKey>(*this,
                                                            register_key);
}

bf_status_t BfRtRegisterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtRegisterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::dataReset(BfRtTableData *data) const {
  BfRtRegisterTableData *register_data =
      static_cast<BfRtRegisterTableData *>(data);
  if (!this->validateTable_from_dataObj(*register_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return register_data->reset();
}

bf_status_t BfRtRegisterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtRegisterTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Unable to get the entry scope params",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<64> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ullong();
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      return pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                            dev_tgt.dev_id,
                                            ghost_pipe_tbl_hdl_,
                                            prop_type,
                                            prop_val,
                                            args_val);
    }
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                ghost_pipe_tbl_hdl_,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Failed to get entry scope from pipe_mgr for table %s",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::ghostTableHandleSet(
    const pipe_tbl_hdl_t &pipe_hdl) {
  ghost_pipe_tbl_hdl_ = pipe_hdl;
  return BF_SUCCESS;
}

// BfRtPVSTable****************
bf_status_t BfRtPVSTable::tableEntryAdd(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey &pvs_key,
                                        const BfRtTableData & /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPVSTableKey &match_key =
      static_cast<const BfRtPVSTableKey &>(pvs_key);
  bf_status_t sts;
  pipe_pvs_hdl_t pvs_entry_handle;
  uint32_t key = 0, mask = 0;
  match_key.populate_match_spec(&key, &mask);
  sts = pipeMgr->pipeMgrPvsEntryHandleGet(session.sessHandleGet(),
                                          dev_tgt.dev_id,
                                          pipe_tbl_hdl,
                                          dev_tgt.direction,
                                          dev_tgt.pipe_id,
                                          dev_tgt.prsr_id,
                                          key,
                                          mask,
                                          &pvs_entry_handle);
  if (sts == BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error entry already exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_ALREADY_EXISTS;
  } else if (sts != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE("%s:%d %s: Unexpected error (%d)",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sts);
    return sts;
  }

  sts = pipeMgr->pipeMgrPvsEntryAdd(session.sessHandleGet(),
                                    dev_tgt.dev_id,
                                    pipe_tbl_hdl,  // pvs_hdl
                                    dev_tgt.direction,
                                    dev_tgt.pipe_id,
                                    dev_tgt.prsr_id,
                                    key,
                                    mask,
                                    &pvs_entry_handle);
  if (BF_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }
  return sts;
}

bf_status_t BfRtPVSTable::tableEntryDel(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey &pvs_key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPVSTableKey &match_key =
      static_cast<const BfRtPVSTableKey &>(pvs_key);
  pipe_pvs_hdl_t pvs_entry_handle;
  uint32_t key, mask;
  match_key.populate_match_spec(&key, &mask);
  bf_status_t status =
      pipeMgr->pipeMgrPvsEntryHandleGet(session.sessHandleGet(),
                                        dev_tgt.dev_id,
                                        pipe_tbl_hdl,
                                        dev_tgt.direction,
                                        dev_tgt.pipe_id,
                                        dev_tgt.prsr_id,
                                        key,
                                        mask,
                                        &pvs_entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_SUCCESS;
  }
  status = pipeMgr->pipeMgrPvsEntryDelete(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, pvs_entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pipe_str_err(pipe_status_t(status)));
  }
  return status;
}

bf_status_t BfRtPVSTable::tableClear(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_status_t status = pipeMgr->pipeMgrPvsClear(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                this->pipe_tbl_hdl,
                                                dev_tgt.direction,
                                                dev_tgt.pipe_id,
                                                dev_tgt.prsr_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Table clear failed for table %s, err %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pipe_str_err(pipe_status_t(status)));
  }
  return status;
}

bf_status_t BfRtPVSTable::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const BfRtTableKey &pvs_key,
                                        BfRtTableData * /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPVSTableKey &match_key =
      static_cast<const BfRtPVSTableKey &>(pvs_key);
  // BfRtPvsTableData *pvs_data = static_cast<BfRtPvsTableData *>(data);
  pipe_pvs_hdl_t pvs_entry_handle;
  uint32_t key, mask, key1, mask1;
  match_key.populate_match_spec(&key, &mask);
  bf_status_t status =
      pipeMgr->pipeMgrPvsEntryHandleGet(session.sessHandleGet(),
                                        dev_tgt.dev_id,
                                        pipe_tbl_hdl,
                                        dev_tgt.direction,
                                        dev_tgt.pipe_id,
                                        dev_tgt.prsr_id,
                                        key,
                                        mask,
                                        &pvs_entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrPvsEntryGetHw(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           dev_tgt.direction,
                                           dev_tgt.pipe_id,
                                           dev_tgt.prsr_id,
                                           pipe_tbl_hdl,
                                           pvs_entry_handle,
                                           &key1,
                                           &mask1);
  } else {
    status = pipeMgr->pipeMgrPvsEntryGet(session.sessHandleGet(),
                                         dev_tgt.dev_id,
                                         pipe_tbl_hdl,
                                         pvs_entry_handle,
                                         &key1,
                                         &mask1,
                                         NULL,
                                         NULL,
                                         NULL);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading pvs entry err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  // Check whether the key/mask pair that's read back from pipe_mgr matches the
  // one in bf_rt
  if ((key != key1) || (mask != mask1)) {
    status = BF_UNEXPECTED;
    LOG_ERROR(
        "%s:%d %s ERROR reading pvs entry err %d, fetched value do not match "
        "provided key",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
  }
  return status;
}

bf_status_t BfRtPVSTable::tableEntryKeyGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const bf_rt_handle_t &entry_handle,
                                           bf_rt_target_t *entry_tgt,
                                           BfRtTableKey *key) const {
  BfRtPVSTableKey *match_key = static_cast<BfRtPVSTableKey *>(key);
  uint32_t pkey, pmask;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  uint8_t gress;
  bf_status_t status = pipeMgr->pipeMgrPvsEntryGet(session.sessHandleGet(),
                                                   dev_tgt.dev_id,
                                                   pipe_tbl_hdl,
                                                   entry_handle,
                                                   &pkey,
                                                   &pmask,
                                                   &gress,
                                                   &entry_tgt->pipe_id,
                                                   &entry_tgt->prsr_id);
  entry_tgt->direction = static_cast<bf_dev_direction_t>(gress);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pkey, pmask);
  return status;
}

bf_status_t BfRtPVSTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtPVSTableKey &match_key = static_cast<const BfRtPVSTableKey &>(key);
  uint32_t pkey, pmask;
  match_key.populate_match_spec(&pkey, &pmask);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_status_t status = pipeMgr->pipeMgrPvsEntryHandleGet(
      session.sessHandleGet(),
      dev_tgt.dev_id,
      pipe_tbl_hdl,
      dev_tgt.direction,
      dev_tgt.pipe_id,
      dev_tgt.prsr_id,
      pkey,
      pmask,
      static_cast<pipe_pvs_hdl_t *>(entry_handle));

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtPVSTable::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const bf_rt_handle_t &entry_handle,
                                        BfRtTableKey *key,
                                        BfRtTableData * /*data*/) const {
  BfRtPVSTableKey *match_key = static_cast<BfRtPVSTableKey *>(key);
  uint32_t pkey, pmask;
  bf_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrPvsEntryGetHw(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           dev_tgt.direction,
                                           dev_tgt.pipe_id,
                                           dev_tgt.prsr_id,
                                           pipe_tbl_hdl,
                                           entry_handle,
                                           &pkey,
                                           &pmask);
  } else {
    status = pipeMgr->pipeMgrPvsEntryGet(session.sessHandleGet(),
                                         dev_tgt.dev_id,
                                         pipe_tbl_hdl,
                                         entry_handle,
                                         &pkey,
                                         &pmask,
                                         NULL,
                                         NULL,
                                         NULL);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading pvs entry err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pkey, pmask);
  return status;
}

bf_status_t BfRtPVSTable::tableEntryGetFirst(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             BfRtTableKey *key,
                                             BfRtTableData * /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  pipe_pvs_hdl_t pvs_entry_handle;
  bf_status_t status = pipeMgr->pipeMgrPvsEntryGetFirst(session.sessHandleGet(),
                                                        dev_tgt.dev_id,
                                                        pipe_tbl_hdl,
                                                        dev_tgt.direction,
                                                        dev_tgt.pipe_id,
                                                        dev_tgt.prsr_id,
                                                        &pvs_entry_handle);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d getting first pvs entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  uint32_t pipe_key = 0, pipe_msk = 0;
  status = pipeMgr->pipeMgrPvsEntryGet(session.sessHandleGet(),
                                       dev_tgt.dev_id,
                                       pipe_tbl_hdl,
                                       pvs_entry_handle,
                                       &pipe_key,
                                       &pipe_msk,
                                       NULL,
                                       NULL,
                                       NULL);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d mapping pvs entry %d to key and mask",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status,
              pvs_entry_handle);
    return status;
  }

  BfRtPVSTableKey *pvs_key = static_cast<BfRtPVSTableKey *>(key);
  pvs_key->populate_key_from_match_spec(pipe_key, pipe_msk);
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::tableEntryGetNext_n(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &cur_key,
                                              const uint32_t &n,
                                              keyDataPairs *key_data_pairs,
                                              uint32_t *num_returned) const {
  if (!key_data_pairs) return BF_INVALID_ARG;

  // Make sure there is room to return results.
  if (key_data_pairs->capacity() < n) {
    LOG_TRACE(
        "%s:%d %s ERROR getting next pvs entries, requested %d entries but "
        "only provided storage for %zu",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        n,
        key_data_pairs->capacity());
    return BF_INVALID_ARG;
  }

  if (num_returned != nullptr) *num_returned = 0;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  // Map the passed in key to an entry handle.
  const BfRtPVSTableKey &cur_entry =
      static_cast<const BfRtPVSTableKey &>(cur_key);
  pipe_pvs_hdl_t entry_handle;
  uint32_t key, mask;
  cur_entry.populate_match_spec(&key, &mask);
  bf_status_t status =
      pipeMgr->pipeMgrPvsEntryHandleGet(session.sessHandleGet(),
                                        dev_tgt.dev_id,
                                        pipe_tbl_hdl,
                                        dev_tgt.direction,
                                        dev_tgt.pipe_id,
                                        dev_tgt.prsr_id,
                                        key,
                                        mask,
                                        &entry_handle);
  if (status != BF_SUCCESS) return status;

  // Use pipe_mgr's get next to get the set of next entry handles.
  std::vector<pipe_pvs_hdl_t> next_entries(n);
  status = pipeMgr->pipeMgrPvsEntryGetNext(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           dev_tgt.direction,
                                           dev_tgt.pipe_id,
                                           dev_tgt.prsr_id,
                                           entry_handle,
                                           n,
                                           next_entries.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d getting next pvs entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  // Map each entry handle to its key and mask to populate the return data.
  for (unsigned i = 0; i < n; ++i) {
    int hdl = next_entries[i];
    if (hdl == -1) break;

    uint32_t pipe_key = 0, pipe_msk = 0;
    status = pipeMgr->pipeMgrPvsEntryGet(session.sessHandleGet(),
                                         dev_tgt.dev_id,
                                         pipe_tbl_hdl,
                                         hdl,
                                         &pipe_key,
                                         &pipe_msk,
                                         NULL,
                                         NULL,
                                         NULL);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR %d mapping pvs entry %d to key and mask",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                status,
                hdl);
      return status;
    }
    auto k = static_cast<BfRtPVSTableKey *>(key_data_pairs->at(i).first);
    k->populate_key_from_match_spec(pipe_key, pipe_msk);
    if (num_returned) ++(*num_returned);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::tableUsageGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        uint32_t *count) const {
  return PipeMgrIntf::getInstance(session)->pipeMgrPvsEntryGetCount(
      session.sessHandleGet(),
      dev_tgt.dev_id,
      pipe_tbl_hdl,
      dev_tgt.direction,
      dev_tgt.pipe_id,
      dev_tgt.prsr_id,
      BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW),
      count);
}

bf_status_t BfRtPVSTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPVSTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::keyReset(BfRtTableKey *key) const {
  BfRtPVSTableKey *pvs_key = static_cast<BfRtPVSTableKey *>(key);
  return key_reset<BfRtPVSTable, BfRtPVSTableKey>(*this, pvs_key);
}

// PVS table has no data, this is a dummy data node
bf_status_t BfRtPVSTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtEmptyTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::dataReset(BfRtTableData *data) const {
  BfRtTableDataObj *data_obj = static_cast<BfRtTableDataObj *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Nothing to do for PVS Table data reset
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s PVS Table Runtime Attributes only support setting"
        "entry scope: gress scope, pipe scope, and parser scope",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::ENTRY_SCOPE:
      break;
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtPVSTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  const auto &tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl &>(tableAttributes);
  const auto attr_type = tbl_attr_impl.getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      bf_status_t sts;
      pipe_mgr_pvs_prop_args_t args;
      pipe_mgr_pvs_prop_type_t property;
      pipe_mgr_pvs_prop_value_t value;

      TableGressScope gress;
      TableEntryScope pipe;
      TablePrsrScope prsr;
      BfRtTableEntryScopeArgumentsImpl pipe_args(0);
      GressTarget prsr_gress;

      sts = tbl_attr_impl.entryScopeParamsGet(
          &gress, &pipe, &pipe_args, &prsr, &prsr_gress);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Attribute is not set",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_NOT_SUPPORTED;
      }
      // set gress scope
      args.value = 0;
      property = PIPE_MGR_PVS_GRESS_SCOPE;
      if (gress == TableGressScope::GRESS_SCOPE_ALL_GRESS) {
        value.gress_scope = PIPE_MGR_PVS_SCOPE_ALL_GRESS;
      } else if (gress == TableGressScope::GRESS_SCOPE_SINGLE_GRESS) {
        value.gress_scope = PIPE_MGR_PVS_SCOPE_SINGLE_GRESS;
      } else {
        LOG_TRACE("%s:%d %s Invalid gress scope (%d)",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  static_cast<int>(gress));
        return BF_NOT_SUPPORTED;
      }
      sts = pipeMgr->pipeMgrPvsSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Gress scope setting fails",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
      // set pipe scope
      property = PIPE_MGR_PVS_PIPE_SCOPE;
      std::bitset<64> bitval;
      pipe_args.getValue(&bitval);
      switch (pipe) {
        case TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_ALL_PIPELINES;
          args.value = bitval.to_ullong();
          break;
        case TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_SINGLE_PIPELINE;
          args.value = bitval.to_ullong();
          break;
        case TableEntryScope::ENTRY_SCOPE_USER_DEFINED:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
          std::memcpy(args.user_defined.user_defined_scope,
                      &bitval,
                      sizeof(scope_pipes_t) * PIPE_MGR_MAX_USER_DEFINED_SCOPES);
          args.user_defined.gress = dev_tgt.direction;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid pipe scope (%d)",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    static_cast<int>(pipe));
          return BF_NOT_SUPPORTED;
      }
      sts = pipeMgr->pipeMgrPvsSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Pipe scope setting fails",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
      // set prsr scope
      switch (prsr_gress) {
        case GressTarget::GRESS_TARGET_INGRESS:
          args.value = (uint32_t)BF_DEV_DIR_INGRESS;
          break;
        case GressTarget::GRESS_TARGET_EGRESS:
          args.value = (uint32_t)BF_DEV_DIR_EGRESS;
          break;
        case GressTarget::GRESS_TARGET_ALL:
          args.value = (uint32_t)BF_DEV_DIR_ALL;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid parser scope args (%d)",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    static_cast<int>(prsr_gress));
          return BF_NOT_SUPPORTED;
      }
      switch (prsr) {
        case TablePrsrScope::PRSR_SCOPE_ALL_PRSRS_IN_PIPE:
          value.parser_scope = PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE;
          break;
        case TablePrsrScope::PRSR_SCOPE_SINGLE_PRSR:
          value.parser_scope = PIPE_MGR_PVS_SCOPE_SINGLE_PARSER;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid prsr scope (%d)",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    static_cast<int>(prsr));
          return BF_NOT_SUPPORTED;
      }
      property = PIPE_MGR_PVS_PARSER_SCOPE;
      sts = pipeMgr->pipeMgrPvsSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Parser scope setting fails",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
      break;
    }
    default:
      LOG_TRACE("%s:%d %s PVS table only support ENTRY_SCOPE",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPVSTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      bf_status_t sts;
      TableGressScope gress_val;
      TableEntryScope pipe_val;
      BfRtTableEntryScopeArgumentsImpl pipe_args(0);
      TablePrsrScope prsr_val;
      GressTarget gress;

      pipe_mgr_pvs_prop_args_t args;
      pipe_mgr_pvs_prop_type_t property;
      pipe_mgr_pvs_prop_value_t value;

      args.value = static_cast<uint32_t>(dev_tgt.direction);
      // read gress scope value
      property = PIPE_MGR_PVS_GRESS_SCOPE;
      sts = pipeMgr->pipeMgrPvsGetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           &value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      if (value.gress_scope == PIPE_MGR_PVS_SCOPE_ALL_GRESS)
        gress_val = TableGressScope::GRESS_SCOPE_ALL_GRESS;
      else if (value.gress_scope == PIPE_MGR_PVS_SCOPE_SINGLE_GRESS)
        gress_val = TableGressScope::GRESS_SCOPE_SINGLE_GRESS;
      else {
        LOG_TRACE("%s:%d %s Get Invalid gress scope (%d)",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  static_cast<int>(value.gress_scope));
        return BF_INVALID_ARG;
      }
      // read pipe scope value
      property = PIPE_MGR_PVS_PIPE_SCOPE;
      sts = pipeMgr->pipeMgrPvsGetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           &value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_ALL_PIPELINES)
        pipe_val = TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES;
      else if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PIPELINE)
        pipe_val = TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE;
      else if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_USER_DEFINED)
        pipe_val = TableEntryScope::ENTRY_SCOPE_USER_DEFINED;
      else {
        LOG_TRACE("%s:%d %s Get Invalid pipe scope (%d)",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  static_cast<int>(value.pipe_scope));
        return BF_INVALID_ARG;
      }
      std::bitset<64> bitval(args.value);
      pipe_args.setValue(bitval);
      // read prsr scope value
      property = PIPE_MGR_PVS_PARSER_SCOPE;
      sts = pipeMgr->pipeMgrPvsGetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           property,
                                           &value,
                                           args);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      if (value.parser_scope == PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE)
        prsr_val = TablePrsrScope::PRSR_SCOPE_ALL_PRSRS_IN_PIPE;
      else if (value.parser_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PARSER)
        prsr_val = TablePrsrScope::PRSR_SCOPE_SINGLE_PRSR;
      else {
        LOG_TRACE("%s:%d %s Get Invalid parser scope (%d)",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  static_cast<int>(value.parser_scope));
        return BF_INVALID_ARG;
      }
      if (dev_tgt.direction == BF_DEV_DIR_INGRESS) {
        gress = GressTarget::GRESS_TARGET_INGRESS;
      } else if (dev_tgt.direction == BF_DEV_DIR_EGRESS) {
        gress = GressTarget::GRESS_TARGET_EGRESS;
      } else if (dev_tgt.direction == BF_DEV_DIR_ALL) {
        gress = GressTarget::GRESS_TARGET_ALL;
      } else {
        LOG_TRACE("%s:%d %s Invalid target direction (%d)",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  static_cast<int>(dev_tgt.direction));
        return BF_INVALID_ARG;
      }
      // set attributes back to obj
      sts = tbl_attr_impl->entryScopeParamsSet(
          gress_val, pipe_val, pipe_args, prsr_val, gress);
      return sts;
    }
    default:
      LOG_TRACE("%s:%d %s PVS table only support ENTRY_SCOPE",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtPhase0Table, BfRtMatchActionKey>(*this, match_key);
}

bf_status_t BfRtPhase0Table::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPhase0TableData(this, action_id));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s : ERROR Unable to allocate data. Out of Memory",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryAdd(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtPhase0TableData &match_data =
      static_cast<const BfRtPhase0TableData &>(data);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   pipe_action_spec,
                                   0 /* ttl */,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtPhase0Table::tableEntryMod(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  const BfRtPhase0TableData &match_data =
      static_cast<const BfRtPhase0TableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  status = pipeMgr->pipeMgrMatEntSetAction(
      session.sessHandleGet(),
      pipe_dev_tgt.device_id,
      tablePipeHandleGet(),
      pipe_entry_hdl,
      act_fn_hdl,
      const_cast<pipe_action_spec_t *>(pipe_action_spec),
      0 /* Pipe API flags */);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryDel(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  uint32_t pipe_flags = 0;

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_IGNORE_NOT_FOUND)) {
    pipe_flags = PIPE_FLAG_IGNORE_NOT_FOUND;
  }

  bf_status_t status =
      pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           &pipe_match_spec,
                                           pipe_flags);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtPhase0Table::tableClear(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/) const {
  return tableClearMatCommon(session, dev_tgt, false, this);
}

bf_status_t BfRtPhase0Table::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  BfRtPhase0TableData &match_data = static_cast<BfRtPhase0TableData &>(*data);
  pipe_action_spec_t *pipe_action_spec = match_data.get_pipe_action_spec();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;

  bf_status_t status = getActionSpec(session,
                                     pipe_dev_tgt,
                                     flags,
                                     pipe_tbl_hdl,
                                     pipe_entry_hdl,
                                     PIPE_RES_GET_FLAG_ENTRY,
                                     pipe_match_spec,
                                     pipe_action_spec,
                                     &pipe_act_fn_hdl,
                                     nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data.actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  match_data.setActiveFields(empty);
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key,
                                           BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return this->tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtPhase0Table::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const bf_rt_handle_t &entry_handle,
                                           BfRtTableKey *key,
                                           BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryGetFirst(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                BfRtTableKey *key,
                                                BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  int pipe_entry_hdl = 0;
  bf_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &pipe_entry_hdl);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  match_key->populate_match_spec(&pipe_match_spec);

  BfRtPhase0TableData *match_data = static_cast<BfRtPhase0TableData *>(data);
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data->get_pipe_action_spec();
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         PIPE_RES_GET_FLAG_ENTRY,
                         &pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data->actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  match_data->setActiveFields(empty);

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryGetNext_n(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 const BfRtTableKey &key,
                                                 const uint32_t &n,
                                                 keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
                                        false /* light_pipe_validation */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return BF_SUCCESS;
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  uint32_t i = 0;
  std::vector<bf_rt_id_t> empty;
  for (i = 0; i < n; i++) {
    memset(&pipe_match_spec, 0, sizeof(pipe_match_spec));
    auto this_key =
        static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPhase0TableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    if (next_entry_handles[i] == -1) {
      break;
    }

    this_key->populate_match_spec(&pipe_match_spec);
    pipe_action_spec_t *pipe_action_spec = nullptr;
    pipe_action_spec = this_data->get_pipe_action_spec();
    pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
    status = getActionSpec(session,
                           pipe_dev_tgt,
                           flags,
                           pipe_tbl_hdl,
                           next_entry_handles[i],
                           PIPE_RES_GET_FLAG_ENTRY,
                           &pipe_match_spec,
                           pipe_action_spec,
                           &pipe_act_fn_hdl,
                           nullptr);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          pipe_entry_hdl,
          status);
      return status;
    }
    bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
    this_data->actionIdSet(action_id);
    this_data->setActiveFields(empty);
  }

  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableUsageGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtPhase0Table::dataReset(BfRtTableData *data) const {
  BfRtPhase0TableData *data_obj = static_cast<BfRtPhase0TableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  return data_obj->reset(action_id);
}

// BfRtSnapshotConfigTable
bf_status_t BfRtSnapshotConfigTable::tableEntryModInternal(
    const BfRtSession &session,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const pipe_snapshot_hdl_t &entry_hdl,
    const BfRtTableData &data) const {
  bf_status_t sts = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotConfigTableData &sc_data =
      static_cast<const BfRtSnapshotConfigTableData &>(data);
  bf_rt_id_t field_id;

  // Set the snapshot timer setting
  sts = this->dataFieldIdGet("timer_enable", &field_id);
  if (sts != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return sts;
  }

  bool timer_en;
  sts = sc_data.getValue(field_id, &timer_en);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return sts;
  }
  sts = this->dataFieldIdGet("ingress_trigger_mode", &field_id);
  if (sts != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return sts;
  }
  std::string ig_mode;
  sts = sc_data.getValue(field_id, &ig_mode);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return sts;
  }

  // For timer disable - true, enable - false.
  // Mode input validation done on data object level
  sts = pipeMgr->bfSnapshotCfgSet(entry_hdl, !timer_en, snapIgMode.at(ig_mode));
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in snapshot config setting",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }

  // Set the snapshot timer timestamp value
  sts = this->dataFieldIdGet("timer_value_usecs", &field_id);
  if (sts) {
    BF_RT_DBGCHK(0);
    return sts;
  }
  uint64_t timer_val;
  sts = sc_data.getValue(field_id, &timer_val);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return sts;
  }
  sts = pipeMgr->bfSnapshotStateSet(
      entry_hdl, BF_SNAPSHOT_ST_DISABLED, timer_val);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in configuring snapshot timer value",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }
  return sts;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t sts = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotConfigTableKey &sc_key =
      static_cast<const BfRtSnapshotConfigTableKey &>(key);
  const BfRtSnapshotConfigTableData &sc_data =
      static_cast<const BfRtSnapshotConfigTableData &>(data);
  uint32_t start_stage, end_stage;
  sc_key.get_stage_id(&start_stage, &end_stage);
  std::string thread;
  bf_snapshot_dir_t dir;
  bf_rt_id_t field_id;

  sts = this->dataFieldIdGet("thread", &field_id);
  if (sts) {
    BF_RT_DBGCHK(0);
    return sts;
  }
  sts = sc_data.getValue(field_id, &thread);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return sts;
  }
  if (!thread.compare("INGRESS")) {
    dir = BF_SNAPSHOT_DIR_INGRESS;
  } else if (!thread.compare("EGRESS")) {
    dir = BF_SNAPSHOT_DIR_EGRESS;
  } else {
    LOG_TRACE("Invalid thread string provided");
    return BF_INVALID_ARG;
  }

  // Create the snapshot
  pipe_snapshot_hdl_t entry_hdl;
  sts = pipeMgr->bfSnapshotCreate(
      dev_tgt.dev_id, dev_tgt.pipe_id, start_stage, end_stage, dir, &entry_hdl);
  if (BF_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }

  sts = this->tableEntryModInternal(session, dev_tgt, flags, entry_hdl, data);
  if (sts != BF_SUCCESS) {
    pipeMgr->bfSnapshotDelete(entry_hdl);
    return sts;
  }

  return sts;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotConfigTableKey &match_key =
      static_cast<const BfRtSnapshotConfigTableKey &>(key);
  uint32_t start_stage, end_stage;
  bf_snapshot_dir_t dir;
  match_key.get_stage_id(&start_stage, &end_stage);
  pipe_snapshot_hdl_t entry_hdl;
  status = pipeMgr->bfSnapshotHandleGet(dev_tgt.dev_id,
                                        dev_tgt.pipe_id,
                                        start_stage,
                                        end_stage,
                                        &dir,
                                        &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_SUCCESS;
  }

  int num_trig = 0;
  status = pipeMgr->bfSnapshotNumTrigFieldsGet(entry_hdl, &num_trig);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting entry info",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_SUCCESS;
  }

  if (num_trig != 0) {
    LOG_ERROR(
        "Triggers related to this entry still exist and must be removed, "
        "before snapshot instance is deleted. Number of triggers %d",
        num_trig);
    return BF_INVALID_ARG;
  }

  status = pipeMgr->bfSnapshotDelete(entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pipe_str_err(pipe_status_t(status)));
    return status;
  }
  return status;
}

bf_status_t BfRtSnapshotConfigTable::setDataFields(
    const bf_snapshot_dir_t dir,
    const bool timer_en,
    const uint32_t timer_val,
    const std::vector<int> &pipes,
    const bf_snapshot_ig_mode_t ig_mode,
    BfRtSnapshotConfigTableData *data) const {
  // Set thread
  bf_rt_id_t field_id;
  auto status = this->dataFieldIdGet("thread", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  switch (dir) {
    case BF_SNAPSHOT_DIR_INGRESS:
      status = data->setValue(field_id, std::string("INGRESS"));
      break;
    case BF_SNAPSHOT_DIR_EGRESS:
      status = data->setValue(field_id, std::string("EGRESS"));
      break;
    default:
      LOG_ERROR("Unsupported direction configured in snapshot");
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Set timer fields
  status = this->dataFieldIdGet("timer_enable", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = data->setValue(field_id, timer_en);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  status = this->dataFieldIdGet("timer_value_usecs", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = data->setValue(field_id, static_cast<uint64_t>(timer_val));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Set captured pipes per thread, iterator "i" overlaps with
  // bf_snapshot_thread_t and field names are in the same order.
  std::vector<std::string> fields = {
      "ingress_capture", "egress_capture", "ghost_capture"};
  for (uint32_t i = 0; i < fields.size(); i++) {
    status = this->dataFieldIdGet(fields[i], &field_id);
    if (status != BF_SUCCESS) {
      BF_RT_DBGCHK(0);
      return status;
    }
    // Get all pipes per thread
    std::vector<uint32_t> pipe_list;
    for (auto it = pipes.begin(); it != pipes.end(); it++) {
      if (*it & (1 << i)) {
        pipe_list.push_back(it - pipes.begin());
      }
    }
    // Set thread that had no pipes as not active
    if (!pipe_list.empty()) {
      status = data->setValue(field_id, pipe_list);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_id);
        return status;
      }
    } else {
      data->removeActiveFields({field_id});
    }
  }

  status = this->dataFieldIdGet("ingress_trigger_mode", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  for (const auto &ig_mode_pair : snapIgMode) {
    if (ig_mode_pair.second == ig_mode) {
      status = data->setValue(field_id, ig_mode_pair.first);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  field_id);
        return status;
      }
    }
  }
  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  pipe_snapshot_hdl_t entry_hdl;
  uint32_t start_stage, end_stage;
  bf_status_t status;
  uint32_t num_pipes;

  const BfRtSnapshotConfigTableKey &sc_key =
      static_cast<const BfRtSnapshotConfigTableKey &>(key);
  BfRtSnapshotConfigTableData *sc_data =
      static_cast<BfRtSnapshotConfigTableData *>(data);
  sc_key.get_stage_id(&start_stage, &end_stage);

  status = this->tableEntryHandleGet(session, dev_tgt, flags, key, &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot config values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get number of pipes",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  std::vector<int> thread_mask(num_pipes);
  status = pipeMgr->bfSnapshotThreadGet(
      entry_hdl, thread_mask.size(), thread_mask.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get captured thread values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  bf_snapshot_dir_t dir;
  status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_hdl, NULL, NULL, NULL, NULL, &dir);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry params, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  status =
      this->setDataFields(dir, timer_en, timer_val, thread_mask, mode, sc_data);

  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtSnapshotConfigTableKey *match_key =
      static_cast<BfRtSnapshotConfigTableKey *>(key);

  uint8_t start_stage, end_stage = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_snapshot_dir_t gress;
  auto status = pipeMgr->bfSnapshotEntryParamsGet(entry_handle,
                                                  &entry_tgt->dev_id,
                                                  &entry_tgt->pipe_id,
                                                  &start_stage,
                                                  &end_stage,
                                                  &gress);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  match_key->set_stage_id(start_stage, end_stage);
  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtSnapshotConfigTableKey &match_key =
      static_cast<const BfRtSnapshotConfigTableKey &>(key);
  uint32_t start_stage, end_stage;
  match_key.get_stage_id(&start_stage, &end_stage);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_snapshot_dir_t dir;
  bf_status_t status = pipeMgr->bfSnapshotHandleGet(dev_tgt.dev_id,
                                                    dev_tgt.pipe_id,
                                                    start_stage,
                                                    end_stage,
                                                    &dir,
                                                    entry_handle);
  if (status != BF_SUCCESS && dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    /* The snapshot might have been created for all pipes */
    status = pipeMgr->bfSnapshotHandleGet(dev_tgt.dev_id,
                                          BF_DEV_PIPE_ALL,
                                          start_stage,
                                          end_stage,
                                          &dir,
                                          entry_handle);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                table_name_get().c_str());
    }
  }
  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  uint8_t start_stage, end_stage = 0;
  BfRtSnapshotConfigTableKey *sc_key =
      static_cast<BfRtSnapshotConfigTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_handle,
      static_cast<bf_dev_id_t *>(nullptr),
      static_cast<bf_dev_pipe_t *>(nullptr),
      &start_stage,
      &end_stage,
      static_cast<bf_snapshot_dir_t *>(nullptr));
  if (status == BF_SUCCESS) {
    sc_key->set_stage_id(start_stage, end_stage);

    status = this->tableEntryGet(session, dev_tgt, flags, *key, data);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_status_t status = pipeMgr->bfSnapshotClear(dev_tgt.dev_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing snapshot table err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtSnapshotConfigTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtSnapshotConfigTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotConfigTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtSnapshotConfigTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status;
  uint32_t start_stage = 0, end_stage = this->getSize();
  *num_returned = 0;

  const BfRtSnapshotConfigTableKey &sc_key =
      static_cast<const BfRtSnapshotConfigTableKey &>(key);
  sc_key.get_stage_id(&start_stage, &end_stage);

  pipe_snapshot_hdl_t entry_handle;
  status =
      this->tableEntryHandleGet(session, dev_tgt, flags, key, &entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get snapshot entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  std::vector<int> next_hdls(n, 0);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotNextEntryHandlesGet(
      dev_tgt.dev_id, entry_handle, n, next_hdls.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot handles, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  for (uint32_t i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtSnapshotConfigTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    if (next_hdls[i] == 0) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    status = this->tableEntryGet(
        session, dev_tgt, flags, next_hdls[i], this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
  }

  return status;
}

bf_status_t BfRtSnapshotConfigTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  pipe_snapshot_hdl_t entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  status = pipeMgr->bfSnapshotFirstHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot stages, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return this->tableEntryGet(session, dev_tgt, flags, entry_hdl, key, data);
}

bf_status_t BfRtSnapshotConfigTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->bfSnapshotUsageGet(dev_tgt.dev_id, dev_tgt.pipe_id, count);
}

// BfRtSnapshotTrigTable
bf_status_t BfRtSnapshotTriggerTable::getHandle(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint32_t &start_stage,
    pipe_snapshot_hdl_t *entry_hdl) const {
  bf_status_t status;
  pipe_snapshot_hdl_t hdl;
  bf_snapshot_dir_t dir;
  uint32_t end_stage = start_stage;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  do {
    status = pipeMgr->bfSnapshotHandleGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, start_stage, end_stage, &dir, &hdl);
    end_stage++;
  } while (status != BF_SUCCESS && end_stage < this->getSize());
  if (status != BF_SUCCESS || dir != this->direction) {
    // This is expected to happen when iterating over table entries
    return BF_OBJECT_NOT_FOUND;
  }
  *entry_hdl = hdl;
  return BF_SUCCESS;
}

/* Modifies trigger field name to be in sync with pipe_mgr dictionary. */
static void updateSnapshotFieldName(std::string *field_name, bool is_trig) {
  // Delete all '$'. If a number is present after '$' then stacked are used
  // and '$' should be replaced by '_'.
  std::string::size_type i = field_name->find('$');
  while (i != std::string::npos) {
    if (field_name->at(i + 1) > '9' || field_name->at(i + 1) < '0') {
      field_name->erase(i, 1);
      i--;
    } else {
      field_name->replace(i, 1, "_");
    }
    i = field_name->find('$', i + 1);
  }
  // replace all '.' to '_'
  std::replace(field_name->begin(), field_name->end(), '.', '_');

  if (is_trig) {
    // remove "trig." prefix
    field_name->erase(0, 5);
  }
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryModInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotTriggerTableKey &st_key =
      static_cast<const BfRtSnapshotTriggerTableKey &>(key);
  const BfRtSnapshotTriggerTableData &st_data =
      static_cast<const BfRtSnapshotTriggerTableData &>(data);
  uint32_t start_stage;
  status = st_key.get_stage_id(&start_stage);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in configuring snapshot timer value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_snapshot_hdl_t entry_hdl;
  status = this->getHandle(session, dev_tgt, start_stage, &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot find related snapshot instance",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  std::vector<bf_rt_id_t> fields;
  status = this->dataFieldIdListGet(&fields);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot fetch field ID list for snapshot trigger table",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_UNEXPECTED;
  }
  std::string sufix("_mask");
  // Process trigger fields
  for (auto it = fields.begin(); it != fields.end(); ++it) {
    auto field_id = *it;
    if (!st_data.checkFieldActive(field_id)) {
      continue;
    }

    std::string field_name;
    status = this->dataFieldNameGet(field_id, &field_name);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field name for field %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_id);
      return status;
    }

    // Skip fixed fields
    if (!field_name.compare("enable") || !field_name.compare("trigger_state") ||
        field_name.find(sufix) == field_name.length() - sufix.length()) {
      continue;
    }

    bf_rt_id_t field_mask_id;
    status =
        this->dataFieldIdGet(std::string(field_name + sufix), &field_mask_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    uint64_t mask = 0, value = 0;
    size_t field_size = 0;
    status = this->dataFieldSizeGet(field_id, &field_size);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field size for %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    field_size = (field_size + 7) / 8;

    std::vector<uint8_t> v(field_size);
    std::vector<uint8_t> m(field_size);
    // If there is no data for specific filed continue
    status = st_data.getValue(field_mask_id, m.size(), m.data());
    if (status == BF_OBJECT_NOT_FOUND) continue;
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting value for field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    status = st_data.getValue(field_id, v.size(), v.data());
    if (status == BF_OBJECT_NOT_FOUND) continue;
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting value for field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name).c_str());
      return status;
    }

    // Convert vector to uint64 as triggers do not support fields > 64
    auto offset = (field_size > sizeof(value)) ? v.size() - sizeof(value) : 0;
    auto size = (field_size > sizeof(value)) ? sizeof(value) : field_size;
    BfRtEndiannessHandler::toHostOrder(size, v.data() + offset, &value);
    BfRtEndiannessHandler::toHostOrder(size, m.data() + offset, &mask);

    updateSnapshotFieldName(&field_name, true);
    status = pipeMgr->bfSnapshotCaptureTriggerFieldAdd(
        entry_hdl, (char *)(field_name).c_str(), value, mask);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in adding field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name).c_str());
      return status;
    }
  }

  // Process fixed fields
  bf_rt_id_t field_id;
  bool enable = false;
  status = this->dataFieldIdGet("enable", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = st_data.getValue(field_id, &enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  bf_snapshot_state_t state =
      (enable) ? BF_SNAPSHOT_ST_ENABLED : BF_SNAPSHOT_ST_DISABLED;
  status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  return this->tableEntryModInternal(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  return this->tableEntryModInternal(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotTriggerTableKey &st_key =
      static_cast<const BfRtSnapshotTriggerTableKey &>(key);
  uint32_t start_stage;
  st_key.get_stage_id(&start_stage);
  pipe_snapshot_hdl_t entry_hdl;
  status = this->getHandle(session, dev_tgt, start_stage, &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_SUCCESS;
  }

  // Disable snapshot
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  bf_snapshot_state_t state = BF_SNAPSHOT_ST_DISABLED;
  status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // Clear trigger fields
  status = pipeMgr->bfSnapshotCaptureTriggerFieldsClr(entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing snapshot trigger table err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  const BfRtSnapshotTriggerTableKey &st_key =
      static_cast<const BfRtSnapshotTriggerTableKey &>(key);
  BfRtSnapshotTriggerTableData *st_data =
      static_cast<BfRtSnapshotTriggerTableData *>(data);
  uint32_t stage;
  st_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
  if (status != BF_SUCCESS && dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    bf_rt_target_t dev_tgt_all = dev_tgt;
    dev_tgt_all.pipe_id = BF_DEV_PIPE_ALL;
    // The snapshot might have been created for all pipes
    status = this->getHandle(session, dev_tgt_all, stage, &entry_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
    }
  }

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  uint32_t num_pipes = 0;
  status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get numper of active pipes",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL && dev_tgt.pipe_id >= num_pipes) {
    LOG_TRACE("%s:%d %s : Error : Invalid pipe_id",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Populate fields to the snapshot data
  std::vector<bf_rt_id_t> fields;
  status = this->dataFieldIdListGet(&fields);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot fetch field ID list for snapshot trigger table",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_UNEXPECTED;
  }

  std::string sufix("_mask");
  // Process trigger fields
  for (auto it = fields.begin(); it != fields.end(); ++it) {
    auto field_id = *it;
    std::string field_name;
    status = this->dataFieldNameGet(field_id, &field_name);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field name for field %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_id);
      return status;
    }

    // Skip fixed fields and masks
    if (!field_name.compare("enable") || !field_name.compare("trigger_state") ||
        field_name.find(sufix) == field_name.length() - sufix.length()) {
      continue;
    }

    bf_rt_id_t field_mask_id;
    status =
        this->dataFieldIdGet(std::string(field_name + sufix), &field_mask_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    if (!st_data->checkFieldActive(field_id)) {
      // Disable mask if field is not active
      st_data->removeActiveFields({field_mask_id});
      continue;
    }

    uint64_t mask = 0, value = 0;
    size_t field_size = 0;
    status = this->dataFieldSizeGet(field_id, &field_size);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field size for %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_name.c_str());
      return status;
    }
    field_size = (field_size + 7) / 8;

    updateSnapshotFieldName(&field_name, true);
    status = pipeMgr->bfSnapshotCaptureTriggerFieldGet(
        entry_hdl, (char *)(field_name).c_str(), &value, &mask);
    if (BF_OBJECT_NOT_FOUND == status) {
      st_data->removeActiveFields({field_mask_id, field_id});
      continue;
    }
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name).c_str());
      return status;
    }

    // Convert vector to uint64 as triggers do not support fields > 64
    std::vector<uint8_t> v(field_size);
    std::vector<uint8_t> m(field_size);
    auto offset = (field_size > sizeof(value)) ? v.size() - sizeof(value) : 0;
    auto size = (field_size > sizeof(value)) ? sizeof(value) : field_size;
    BfRtEndiannessHandler::toNetworkOrder(size, value, v.data() + offset);
    BfRtEndiannessHandler::toNetworkOrder(size, mask, m.data() + offset);

    status = st_data->setValue(field_mask_id, m.data(), size);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error setting value for field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    status = st_data->setValue(field_id, v.data(), size);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error setting value for field %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                (field_name).c_str());
      return status;
    }
  }

  // Process fixed fields
  bool state;

  std::vector<pipe_snapshot_fsm_state_t> trig_state(num_pipes);
  status = pipeMgr->bfSnapshotStateGet(
      entry_hdl, trig_state.size(), trig_state.data(), &state);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // If only one pipe was requested, trim down the result vector
  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    trig_state.erase(trig_state.begin(), trig_state.begin() + dev_tgt.pipe_id);
    trig_state.erase(trig_state.begin() + 1, trig_state.end());
  }

  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("enable", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }

  status = st_data->setValue(field_id, state);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  std::vector<std::string> trig_states;
  for (auto it = trig_state.begin(); it != trig_state.end(); it++) {
    switch (*it) {
      case PIPE_SNAPSHOT_FSM_ST_PASSIVE:
        trig_states.push_back("PASSIVE");
        break;
      case PIPE_SNAPSHOT_FSM_ST_ARMED:
      // Fallthrough
      case PIPE_SNAPSHOT_FSM_ST_TRIGGER_HAPPY:
        trig_states.push_back("ARMED");
        break;
      case PIPE_SNAPSHOT_FSM_ST_FULL:
        trig_states.push_back("FULL");
        break;
      case PIPE_SNAPSHOT_FSM_ST_MAX:
        // Not used index
        break;
      default:
        LOG_ERROR("%s:%d %s : Error unrecognized trigger state value %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  *it);
        BF_RT_DBGCHK(0);
        return BF_UNEXPECTED;
    }
  }
  status = this->dataFieldIdGet("trigger_state", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = st_data->setValue(field_id, trig_states);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }

  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtSnapshotTriggerTableKey *match_key =
      static_cast<BfRtSnapshotTriggerTableKey *>(key);

  uint8_t start_stage, end_stage = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_snapshot_dir_t gress;
  auto status = pipeMgr->bfSnapshotEntryParamsGet(entry_handle,
                                                  &entry_tgt->dev_id,
                                                  &entry_tgt->pipe_id,
                                                  &start_stage,
                                                  &end_stage,
                                                  &gress);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  match_key->set_stage_id(start_stage);
  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtSnapshotTriggerTableKey &st_key =
      static_cast<const BfRtSnapshotTriggerTableKey &>(key);
  uint32_t stage = 0;
  st_key.get_stage_id(&stage);
  auto status = this->getHandle(session, dev_tgt, stage, entry_handle);

  if (status != BF_SUCCESS && dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    bf_rt_target_t dev_tgt_all = dev_tgt;
    dev_tgt_all.pipe_id = BF_DEV_PIPE_ALL;
    // The snapshot might have been created for all pipes
    status = this->getHandle(session, dev_tgt_all, stage, entry_handle);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
    }
  }
  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  uint8_t start_stage, end_stage = 0;
  BfRtSnapshotTriggerTableKey *st_key =
      static_cast<BfRtSnapshotTriggerTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_handle,
      static_cast<bf_dev_id_t *>(nullptr),
      static_cast<bf_dev_pipe_t *>(nullptr),
      &start_stage,
      &end_stage,
      static_cast<bf_snapshot_dir_t *>(nullptr));
  if (status == BF_SUCCESS) {
    st_key->set_stage_id(start_stage);

    status = this->tableEntryGet(session, dev_tgt, flags, *key, data);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_snapshot_hdl_t entry_hdl = 0;

  for (uint32_t st_stg = 0; st_stg < this->getSize(); st_stg++) {
    auto status = this->getHandle(session, dev_tgt, st_stg, &entry_hdl);
    if (status != BF_SUCCESS) {
      // Expected to happen when iterating over all possible entries.
      continue;
    }

    // Disable snapshot
    bool timer_en = false;
    uint32_t timer_val = 0;
    bf_snapshot_ig_mode_t mode;
    status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }
    bf_snapshot_state_t state = BF_SNAPSHOT_ST_DISABLED;
    status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }

    status = pipeMgr->bfSnapshotCaptureTriggerFieldsClr(entry_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in Clearing snapshot trigger table err %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                status);
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotTriggerTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtSnapshotTriggerTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotTriggerTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSnapshotTriggerTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}
bf_status_t BfRtSnapshotTriggerTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  uint32_t stage = 0;
  *num_returned = 0;

  const BfRtSnapshotTriggerTableKey &sc_key =
      static_cast<const BfRtSnapshotTriggerTableKey &>(key);
  sc_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;

  for (uint32_t i = 0; i < n; i++) {
    // Find next valid stage
    stage++;
    status = BF_OBJECT_NOT_FOUND;
    for (; stage < this->getSize(); stage++) {
      status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
      if (status != BF_SUCCESS && dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
        bf_rt_target_t wrk_tgt = dev_tgt;
        wrk_tgt.pipe_id = BF_DEV_PIPE_ALL;
        status = this->getHandle(session, wrk_tgt, stage, &entry_hdl);
      }
      if (status == BF_SUCCESS) break;
    }
    auto this_key =
        static_cast<BfRtSnapshotTriggerTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }

    if (status != BF_SUCCESS) {
      (*key_data_pairs)[i].second = nullptr;
      if (status == BF_OBJECT_NOT_FOUND) status = BF_SUCCESS;
      continue;
    }

    status = this->tableEntryGet(
        session, dev_tgt, flags, entry_hdl, this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
  }

  return status;
}

bf_status_t BfRtSnapshotTriggerTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  pipe_snapshot_hdl_t entry_hdl;
  for (uint32_t stage = 0; stage < this->getSize(); stage++) {
    status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
    if (status == BF_SUCCESS) break;
    if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
      bf_rt_target_t wrk_tgt = dev_tgt;
      wrk_tgt.pipe_id = BF_DEV_PIPE_ALL;
      status = this->getHandle(session, wrk_tgt, stage, &entry_hdl);
      if (status == BF_SUCCESS) break;
    }
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Snapshot entry not found, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }
  return this->tableEntryGet(session, dev_tgt, flags, entry_hdl, key, data);
}

// SnapshotData

void BfRtSnapshotDataTable::tableEntryDataSetControlInfoNextTables(
    bf_dev_id_t dev_id,
    const bf_rt_id_t field_id,
    const char (&next_tbl_names)[BF_MAX_LOG_TBLS * BF_TBL_NAME_LEN],
    BfRtSnapshotDataTableData *data) const {
  size_t pos = 0;
  size_t start_pos = 0;
  std::string s(next_tbl_names);
  std::vector<std::string> tables_qualified;
  while ((pos = s.find(" ", start_pos)) != std::string::npos) {
    std::string tempStr = getQualifiedTableName(
        dev_id, this->prog_name, s.substr(start_pos, pos - start_pos));
    tables_qualified.push_back(tempStr);
    start_pos = pos + 1;
  }
  data->setValue(field_id, tables_qualified);
}

void BfRtSnapshotDataTable::tableEntryGetControlInfo(
    const BfRtSession &session,
    const pipe_snapshot_hdl_t &entry_hdl,
    const uint32_t &stage,
    const bf_snapshot_capture_ctrl_info_arr_t &ctrl_info_arr,
    const int & /*num_captures*/,
    BfRtSnapshotDataTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  bf_snapshot_dir_t thread;
  dev_stage_t first_stage;
  bf_dev_id_t dev_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status =
      pipeMgr->bfSnapshotEntryParamsGet(entry_hdl,
                                        &dev_id,
                                        static_cast<bf_dev_pipe_t *>(nullptr),
                                        &first_stage,
                                        static_cast<dev_stage_t *>(nullptr),
                                        &thread);

  std::vector<bf_rt_id_t> dataFields;
  // Get the container fields for this field-id
  status = dataFieldIdListGet(&dataFields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in getting data field id list", __func__, __LINE__);
    return;
  }

  const bf_snapshot_capture_ctrl_info_t *ctrl =
      &ctrl_info_arr.ctrl[stage - first_stage];
  if (ctrl->stage_id != stage) {
    LOG_ERROR("%s:%d Error in control data, stage do not match %d != %d",
              __func__,
              __LINE__,
              ctrl->stage_id,
              stage);
    return;
  }

  // Go over all fields
  for (auto it = dataFields.begin(); it != dataFields.end(); ++it) {
    bf_rt_id_t field_id = *it;
    const BfRtTableDataField *dataField;
    status = getDataField(field_id, &dataField);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d Error in getting data field for id %u",
                __func__,
                __LINE__,
                field_id);
      continue;
    }
    // Only control fields are handled here
    if (!this->isControlField(dataField)) {
      continue;
    }

    std::string field_name = dataField->getName();
    std::set<DataFieldType> types;
    types = dataField->getTypes();

    uint64_t field_value = 0;
    if (types.find(DataFieldType::SNAPSHOT_PREV_STAGE_TRIGGER) != types.end()) {
      field_value = ctrl->prev_stage_trigger;
      data->setValue(field_id, field_value);
    } else if (types.find(DataFieldType::SNAPSHOT_TIMER_TRIGGER) !=
               types.end()) {
      field_value = ctrl->timer_trigger;
      data->setValue(field_id, field_value);
    } else if (types.find(DataFieldType::SNAPSHOT_LOCAL_STAGE_TRIGGER) !=
               types.end()) {
      field_value = ctrl->local_stage_trigger;
      data->setValue(field_id, field_value);
    } else if (types.find(DataFieldType::SNAPSHOT_NEXT_TABLE_NAME) !=
               types.end()) {
      std::string tempStr(ctrl->next_table, strlen(ctrl->next_table));
      if (tempStr.empty()) tempStr = "NO_NEXT_TABLE";
      tempStr = getQualifiedTableName(dev_id, this->prog_name, tempStr);
      data->setValue(field_id, tempStr);
    } else if (types.find(DataFieldType::SNAPSHOT_METER_ALU_INFO) !=
               types.end()) {
      std::vector<bf_rt_id_t> c_dataFields;
      // Get the container fields for meter_alu_info
      status = containerDataFieldIdListGet(field_id, &c_dataFields);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d Error in getting data field id list for id %u",
                  __func__,
                  __LINE__,
                  field_id);
        continue;
      }
      for (uint32_t alu_idx = 0; alu_idx < 4; alu_idx++) {
        // Skip empty entries. Set at least one value for meter_alu_info so
        // that grpc does not complain
        if ((alu_idx != 0) && ctrl->meter_alu_info[alu_idx].table_handle == 0) {
          continue;
        }
        std::unique_ptr<BfRtSnapshotDataTableData> tbl_info_obj(
            new BfRtSnapshotDataTableData(this));
        // Go over all fields for meter_alu_info
        for (auto c_it = c_dataFields.begin(); c_it != c_dataFields.end();
             ++c_it) {
          bf_rt_id_t c_field_id = *c_it;
          const BfRtTableDataField *c_dataField;
          status = getDataField(c_field_id, &c_dataField);
          if (BF_SUCCESS != status) {
            LOG_ERROR("%s:%d Error in getting data field for id %u",
                      __func__,
                      __LINE__,
                      c_field_id);
            continue;
          }

          std::string c_field_name = c_dataField->getName();
          std::set<DataFieldType> c_types;
          c_types = c_dataField->getTypes();

          uint64_t c_field_value = 0;
          if (c_types.find(DataFieldType::SNAPSHOT_TABLE_NAME) !=
              c_types.end()) {
            // Table name is currently not available from pipe-mgr
            std::string tempStr("NO_TABLE");
            tempStr = getQualifiedTableName(dev_id, this->prog_name, tempStr);
            tbl_info_obj->setValue(c_field_id, tempStr);
          } else if (c_types.find(
                         DataFieldType::SNAPSHOT_METER_ALU_OPERATION_TYPE) !=
                         c_types.end() &&
                     ctrl->meter_alu_info[alu_idx].ctrl_info_p) {
            std::string tempStr(
                ctrl->meter_alu_info[alu_idx].ctrl_info_p,
                strlen(ctrl->meter_alu_info[alu_idx].ctrl_info_p));
            if (tempStr.empty()) tempStr = "NO_METER_ALU_OPER_TYPE";
            tbl_info_obj->setValue(c_field_id, tempStr);
          } else if (c_types.find(
                         DataFieldType::SNAPSHOT_METER_ALU_OPERATION_TYPE) !=
                         c_types.end() &&
                     ctrl->meter_alu_info[alu_idx].ctrl_info_p == nullptr &&
                     alu_idx == 0) {
            std::string tempStr("NO_METER_ALU_OPER_TYPE");
            tbl_info_obj->setValue(c_field_id, tempStr);
          } else {
            // unknown field
            c_field_value = 0;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          }
        }  // c_dataFields
        data->setValue(field_id, std::move(tbl_info_obj));
      }
    } else if (types.find(DataFieldType::SNAPSHOT_TABLE_INFO) != types.end()) {
      std::vector<bf_rt_id_t> c_dataFields;
      // Get the container fields for tbl_info
      status = containerDataFieldIdListGet(field_id, &c_dataFields);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d Error in getting data field id list for id %u",
                  __func__,
                  __LINE__,
                  field_id);
        continue;
      }

      for (uint32_t tbl_idx = 0; tbl_idx < BF_MAX_LOG_TBLS; tbl_idx++) {
        // Skip empty entries. Set at least one value for tbl_info so
        // that grpc does not complain
        if ((tbl_idx != 0) && ctrl->tables_info[tbl_idx].table_handle == 0) {
          continue;
        }
        std::unique_ptr<BfRtSnapshotDataTableData> tbl_info_obj(
            new BfRtSnapshotDataTableData(this));
        // Go over all fields for tbl_info
        for (auto c_it = c_dataFields.begin(); c_it != c_dataFields.end();
             ++c_it) {
          bf_rt_id_t c_field_id = *c_it;
          const BfRtTableDataField *c_dataField;
          status = getDataField(c_field_id, &c_dataField);
          if (BF_SUCCESS != status) {
            LOG_ERROR("%s:%d Error in getting data field for id %u",
                      __func__,
                      __LINE__,
                      c_field_id);
            continue;
          }

          std::string c_field_name = c_dataField->getName();
          std::set<DataFieldType> c_types;
          c_types = c_dataField->getTypes();

          uint64_t c_field_value = 0;
          if (c_types.find(DataFieldType::SNAPSHOT_MATCH_HIT_ADDRESS) !=
              c_types.end()) {
            c_field_value = ctrl->tables_info[tbl_idx].match_hit_address;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          } else if (c_types.find(DataFieldType::SNAPSHOT_TABLE_HIT) !=
                     c_types.end()) {
            c_field_value = ctrl->tables_info[tbl_idx].table_hit;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          } else if (c_types.find(DataFieldType::SNAPSHOT_TABLE_INHIBITED) !=
                     c_types.end()) {
            c_field_value = ctrl->tables_info[tbl_idx].table_inhibited;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          } else if (c_types.find(DataFieldType::SNAPSHOT_TABLE_EXECUTED) !=
                     c_types.end()) {
            c_field_value = ctrl->tables_info[tbl_idx].table_executed;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          } else if (c_types.find(DataFieldType::SNAPSHOT_TABLE_NAME) !=
                     c_types.end()) {
            std::string tempStr(ctrl->tables_info[tbl_idx].table_name,
                                strnlen(ctrl->tables_info[tbl_idx].table_name,
                                        BF_TBL_NAME_LEN));
            if (tempStr.empty()) tempStr = "NO_TABLE";
            tempStr = getQualifiedTableName(dev_id, this->prog_name, tempStr);
            tbl_info_obj->setValue(c_field_id, tempStr);
          } else if (c_types.find(DataFieldType::SNAPSHOT_MATCH_HIT_HANDLE) !=
                     c_types.end()) {
            c_field_value = ctrl->tables_info[tbl_idx].hit_entry_handle;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          } else {
            // unknown field
            c_field_value = 0;
            tbl_info_obj->setValue(c_field_id, c_field_value);
          }
        }  // c_dataFields
        data->setValue(field_id, std::move(tbl_info_obj));
      }  // tbl_idx
    } else if (types.find(DataFieldType::SNAPSHOT_ENABLED_NEXT_TABLES) !=
               types.end()) {
      this->tableEntryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_next_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_GBL_EXECUTE_TABLES) !=
               types.end()) {
      this->tableEntryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->gbl_exec_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_ENABLED_GBL_EXECUTE_TABLES) !=
               types.end()) {
      this->tableEntryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_gbl_exec_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_LONG_BRANCH_TABLES) !=
               types.end()) {
      this->tableEntryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->long_branch_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_ENABLED_LONG_BRANCH_TABLES) !=
               types.end()) {
      this->tableEntryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_long_branch_tbl_names, data);
    } else {
      // unknown field
      field_value = 0;
      data->setValue(field_id, field_value);
    }
  }  // For all fields

  return;
}

bf_status_t BfRtSnapshotDataTable::tableEntryGetFieldInfo(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const pipe_snapshot_hdl_t &entry_hdl,
    const uint32_t &stage,
    uint8_t *capture,
    const int num_captures,
    BfRtSnapshotDataTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  std::vector<bf_rt_id_t> dataFields;
  // Get the container fields for this field-id
  status = this->dataFieldIdListGet(&dataFields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in getting data field id list", __func__, __LINE__);
    return status;
  }
  bf_snapshot_dir_t thread;
  status =
      pipeMgr->bfSnapshotEntryParamsGet(entry_hdl,
                                        static_cast<bf_dev_id_t *>(nullptr),
                                        static_cast<bf_dev_pipe_t *>(nullptr),
                                        static_cast<dev_stage_t *>(nullptr),
                                        static_cast<dev_stage_t *>(nullptr),
                                        &thread);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in getting snapshot params", __func__, __LINE__);
    return status;
  }
  // Go over all fields for this stage
  for (auto it = dataFields.begin(); it != dataFields.end(); ++it) {
    bf_rt_id_t field_id = *it;
    const BfRtTableDataField *dataField;
    status = getDataField(field_id, &dataField);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d Error in getting data field for id %u",
                __func__,
                __LINE__,
                field_id);
      return status;
    }

    // Control fields are set in different function.
    if (this->isControlField(dataField)) {
      continue;
    }
    std::string field_name = dataField->getName();
    size_t field_size = (dataField->getSize() + 7) / 8;

    // Skip Field if not present in this stage
    uint64_t field_value = 0;

    updateSnapshotFieldName(&field_name, false);
    bool exists = false;
    status =
        pipeMgr->bfSnapshotFieldInScope(dev_tgt.dev_id,
                                        dev_tgt.pipe_id,
                                        stage,
                                        thread,
                                        const_cast<char *>(field_name.c_str()),
                                        &exists);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in checking field scope for field %s in "
          "stage %d",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          field_name.c_str(),
          stage);
      return status;
    }
    if (!exists) {
      data->removeActiveFields({field_id});
      continue;
    }
    // On Tofino 1 there may be invalid fields populated with data.
    // Skip if that is the case.
    bool field_valid;
    status =
        pipeMgr->bfSnapshotCaptureDecodeFieldValue(entry_hdl,
                                                   dev_tgt.pipe_id,
                                                   stage,
                                                   capture,
                                                   num_captures,
                                                   (char *)field_name.c_str(),
                                                   &field_value,
                                                   &field_valid);

    if (BF_SUCCESS != status) {
      // This may happen in case when there is slight difference between field
      // dictionary and actual PHV values in complex P4 programs.
      // Warn only for now.
      LOG_WARN("%s:%d %s: Error in decoding captured field %s in stage %d",
               __func__,
               __LINE__,
               this->table_name_get().c_str(),
               field_name.c_str(),
               stage);
      field_valid = false;
    }

    if (!field_valid) {
      data->removeActiveFields({field_id});
      continue;
    }

    // For byte streams change byte order since value fetched is uint64_t
    // All fields here are expected to be byte streams
    std::vector<uint8_t> v(field_size);
    auto offset =
        (field_size > sizeof(field_value)) ? v.size() - sizeof(field_value) : 0;
    auto size =
        (field_size > sizeof(field_value)) ? sizeof(field_value) : field_size;
    BfRtEndiannessHandler::toNetworkOrder(size, field_value, v.data() + offset);
    status = data->setValue(field_id, v.data(), field_size);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_id);
      return status;
    }
  }  // For all fields

  return status;
}

bf_status_t BfRtSnapshotDataTable::getHandle(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint32_t &stage,
    pipe_snapshot_hdl_t *entry_hdl) const {
  bf_status_t status;
  pipe_snapshot_hdl_t hdl;
  uint32_t num_pipes = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get number of pipes",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  if (dev_tgt.pipe_id != BF_DEV_PIPE_ALL) {
    // Verify pipe number to avoid exccesive errors
    if (num_pipes <= dev_tgt.pipe_id) {
      LOG_TRACE("%s:%d %s : Error : Invalid pipe numer : %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.pipe_id);
      return BF_INVALID_ARG;
    }
  }

  // Get next with handle '0' will return all instances created on all pipes
  std::vector<int> hdl_list(this->_table_size * num_pipes, 0);
  status = pipeMgr->bfSnapshotNextEntryHandlesGet(
      dev_tgt.dev_id, 0, hdl_list.size(), hdl_list.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot handles, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }
  status = BF_OBJECT_NOT_FOUND;
  int i = 0;
  hdl = hdl_list[i++];
  while (hdl != 0) {
    dev_stage_t start_stage, end_stage;
    bf_dev_pipe_t pipe_id;
    bf_dev_id_t dev_id;
    auto sts = pipeMgr->bfSnapshotEntryParamsGet(
        hdl,
        &dev_id,
        &pipe_id,
        &start_stage,
        &end_stage,
        static_cast<bf_snapshot_dir_t *>(nullptr));
    if (sts) break;
    // Check if stage belongs to this entry
    if (stage >= start_stage && stage <= end_stage &&
        (pipe_id == dev_tgt.pipe_id || pipe_id == BF_DEV_PIPE_ALL) &&
        dev_id == dev_tgt.dev_id) {
      status = BF_SUCCESS;
      break;
    }
    hdl = hdl_list[i++];
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  *entry_hdl = hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotDataTable::tableEntryGet(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 const BfRtTableKey &key,
                                                 BfRtTableData *data) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtSnapshotDataTableKey &sd_key =
      static_cast<const BfRtSnapshotDataTableKey &>(key);
  BfRtSnapshotDataTableData *sd_data =
      static_cast<BfRtSnapshotDataTableData *>(data);
  uint32_t stage;
  sd_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint32_t total_size = 0, stage_size = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotCapturePhvFieldsDictSize(
      entry_hdl, &total_size, &stage_size);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Failed to get phv dict size",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  int num_captures = 0;
  bf_snapshot_capture_ctrl_info_arr_t ctrl_info_arr;
  memset(&ctrl_info_arr, 0, sizeof(ctrl_info_arr));
  std::vector<uint8_t> capture(total_size);

  status = pipeMgr->bfSnapshotCaptureGet(entry_hdl,
                                         dev_tgt.pipe_id,
                                         capture.data(),
                                         &ctrl_info_arr,
                                         &num_captures);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading snapshot capture for entry",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  status = this->tableEntryGetFieldInfo(session,
                                        dev_tgt,
                                        entry_hdl,
                                        stage,
                                        capture.data(),
                                        num_captures,
                                        sd_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error when setting snapshot data fields",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  this->tableEntryGetControlInfo(
      session, entry_hdl, stage, ctrl_info_arr, num_captures, sd_data);
  return status;
}

bf_status_t BfRtSnapshotDataTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtSnapshotDataTableKey *sd_key =
      static_cast<BfRtSnapshotDataTableKey *>(key);

  // This is indexed table, so handle is the same as stage id in the key
  uint32_t stage = entry_handle;
  // Check if snapshot instance covering this handle/stage exist
  pipe_snapshot_hdl_t snap_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  sd_key->set_stage_id(stage);
  return status;
}

bf_status_t BfRtSnapshotDataTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtSnapshotDataTableKey &sd_key =
      static_cast<const BfRtSnapshotDataTableKey &>(key);
  uint32_t stage = 0;
  sd_key.get_stage_id(&stage);
  // Check if snapshot instance covering this handle/stage exist
  pipe_snapshot_hdl_t snap_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
  }

  // This is indexed table, so handle is the same as stage id in the key
  *entry_handle = stage;

  return status;
}

bf_status_t BfRtSnapshotDataTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtSnapshotDataTableKey *sd_key =
      static_cast<BfRtSnapshotDataTableKey *>(key);
  // This is indexed table, so handle is the same as stage id in the key
  sd_key->set_stage_id(entry_handle);

  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtSnapshotDataTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  if (type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s Snapshot Data Table Runtime Attributes only support "
        "ENTRY_SCOPE setting",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotDataTable::tableAttributesGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    BF_RT_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  return tbl_attr_impl->entryScopeParamsSet(
      TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE);
}

bf_status_t BfRtSnapshotDataTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtSnapshotDataTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotDataTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSnapshotDataTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}
bf_status_t BfRtSnapshotDataTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  bf_status_t status = BF_OBJECT_NOT_FOUND;
  *num_returned = 0;

  auto first_key = static_cast<const BfRtSnapshotDataTableKey &>(key);
  uint32_t start_stage;
  uint8_t end_stage = this->_table_size;
  first_key.get_stage_id(&start_stage);
  pipe_snapshot_hdl_t snap_hdl;
  uint32_t i = 0;
  for (uint32_t stage = start_stage + 1; stage < end_stage && i < n; stage++) {
    // Check if stage is valid
    status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
    if (status != BF_SUCCESS) {
      continue;
    }
    auto this_key =
        static_cast<BfRtSnapshotDataTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    this_key->set_stage_id(stage);

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }

    status = this->tableEntryGet(
        session, dev_tgt, flags, *(*key_data_pairs)[i].first, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
    i++;
  }

  return status;
}

bf_status_t BfRtSnapshotDataTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (dev_tgt.pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  bf_status_t status = BF_SUCCESS;

  // Find first valid stage
  pipe_snapshot_hdl_t snap_hdl;
  uint32_t stage;
  for (stage = 0; stage < this->_table_size; stage++) {
    status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
    if (status == BF_SUCCESS) break;
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data doesn't have any entries"
        " for all pipes",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return status;
  }

  auto sd_key = static_cast<BfRtSnapshotDataTableKey *>(key);
  sd_key->set_stage_id(stage);

  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtSnapshotDataTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  // This is the only call that is allowed without pipe argument
  // Fixed table, return table size
  *count = this->_table_size;
  return BF_SUCCESS;
}

// Snapshot Liveness
bf_status_t BfRtSnapshotLivenessTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  /* Table usage do not apply to snapshot. */
  *count = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotLivenessTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  /* TODO: Hardcoded end_stage */
  uint32_t start_stage = 0, end_stage = 20, stage_val = 0;
  bf_status_t sts = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtSnapshotLivenessTableKey &match_key =
      static_cast<const BfRtSnapshotLivenessTableKey &>(key);
  BfRtSnapshotLivenessTableData &match_data =
      static_cast<BfRtSnapshotLivenessTableData &>(*data);

  /* Get the field name */
  std::string f_name;
  sts = match_key.getFieldName(&f_name);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR getting field name",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }

  updateSnapshotFieldName(&f_name, false);
  bf_rt_id_t field_id;
  sts = this->dataFieldIdGet("valid_stages", &field_id);
  if (sts) {
    BF_RT_DBGCHK(0);
    return sts;
  }

  /* Check for field validity in all stages */
  for (stage_val = start_stage; stage_val <= end_stage; stage_val++) {
    bool exists = false;
    sts = pipeMgr->bfSnapshotTriggerFieldInScope(
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        stage_val,
        this->direction,
        const_cast<char *>(f_name.c_str()),
        &exists);
    if ((BF_SUCCESS == sts) && exists) {
      /* Field exists in stage */
      match_data.setValue(field_id, static_cast<uint64_t>(stage_val));
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotLivenessTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtSnapshotLivenessTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotLivenessTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSnapshotLivenessTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Snapshot PHV
bf_status_t BfRtSnapshotPhvTable::tableEntryGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t & /*flags*/,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  if (BF_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtSnapshotPhvTableKey &phv_key =
      static_cast<const BfRtSnapshotPhvTableKey &>(key);
  BfRtSnapshotPhvTableData &phv_data =
      static_cast<BfRtSnapshotPhvTableData &>(*data);

  dev_stage_t stage = phv_key.getStageId();
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  uint32_t total_size = 0;

  std::vector<int> stages(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stages.size(), stages.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  auto it = find(stages.begin(), stages.end(), stage);
  if (it == stages.end()) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->bfSnapshotTotalPhvCountGet(dev_tgt.dev_id, &total_size);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting snapshot PHV number from pipe-mgr,"
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  std::vector<uint32_t> fields(total_size);
  std::vector<char> fields_v(total_size);  // Using char for boolean values
  status = pipeMgr->bfSnapshotRawCaptureGet(
      dev_tgt.dev_id,
      dev_tgt.pipe_id,
      stage,
      total_size,
      fields.data(),
      reinterpret_cast<bool *>(fields_v.data()));
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting snapshot PHV capture from pipe-mgr,"
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  // Only valid containers should be marked as active.
  // Field id is equal to PHV number. This must be in kept line with JSON file.
  for (uint32_t i = 0; i < total_size; i++) {
    if (static_cast<bool>(fields_v[i]) && phv_data.checkFieldActive(i)) {
      status = phv_data.setValue(i, fields[i]);
      // Errors logged by setter function.
      if (status != BF_SUCCESS) return status;
    } else {
      phv_data.removeActiveFields({i});
    }
  }

  return status;
}

bf_status_t BfRtSnapshotPhvTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (BF_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *num_returned = 0;
  const BfRtSnapshotPhvTableKey &phv_key =
      static_cast<const BfRtSnapshotPhvTableKey &>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  std::vector<int> stages(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stages.size(), stages.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  int start_stage = static_cast<int>(phv_key.getStageId());
  auto it = find(stages.begin(), stages.end(), start_stage);
  if (it == stages.end()) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }

  /* Grab next stage */
  it++;

  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtSnapshotPhvTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }

    /* Stage not used, set data to null, must be done for gRPC server. */
    if (i >= stages.size() || *it == -1 || it == stages.end()) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    this_key->setStageId(*it);
    status = this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
    it++;
  }

  return status;
}

bf_status_t BfRtSnapshotPhvTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (BF_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtSnapshotPhvTableKey *phv_key =
      static_cast<BfRtSnapshotPhvTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  std::vector<int> stage_list(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stage_list.size(), stage_list.data());
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  /* -1 on first index means that no stages were programmed */
  if (stage_list[0] == -1) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }

  phv_key->setStageId(stage_list[0]);
  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtSnapshotPhvTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotPhvTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  if (type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s This Table Attributes only support "
        "ENTRY_SCOPE setting",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotPhvTable::tableAttributesGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    BF_RT_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  return tbl_attr_impl->entryScopeParamsSet(
      TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE);
}

bf_status_t BfRtSnapshotPhvTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtSnapshotPhvTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSnapshotPhvTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

bf_status_t BfRtSnapshotPhvTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtSnapshotPhvTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// BfRtDynHashCfgTable****************

bf_status_t BfRtDynHashCfgTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtDynHashCfgTableData &match_data =
      static_cast<const BfRtDynHashCfgTableData &>(data);
  bf_status_t sts = BF_SUCCESS;

  std::vector<pipe_hash_calc_input_field_attribute_t> attr_list;
  sts = match_data.attrListGet(&attr_list);

  // If attr list is empty, then we want to reset the entry instead
  if (attr_list.empty()) {
    LOG_DBG("%s:%d %s: Received empty list. Resetting to default entry",
            __func__,
            __LINE__,
            table_name_get().c_str());
    return this->tableDefaultEntryReset(session, dev_tgt, flags);
  }

  pipe_fld_lst_hdl_t fl_hdl;
  sts = pipeMgr->pipeMgrHashCalcInputGet(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &fl_hdl);
  if (BF_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in getting dyn hashing field list handle",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }
  sts = pipeMgr->pipeMgrHashCalcInputFieldAttrSet(session.sessHandleGet(),
                                                  dev_tgt.dev_id,
                                                  pipe_tbl_hdl,  // hdl
                                                  fl_hdl,
                                                  attr_list.size(),
                                                  attr_list.data());
  if (BF_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
  return sts;
}

bf_status_t BfRtDynHashCfgTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s ERROR : Read from hardware not supported. "
        "Reading from sw instead",
        __func__,
        __LINE__,
        table_name_get().c_str());
    // Continuing as if read from sw
  }
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  BfRtDynHashCfgTableData *match_data =
      static_cast<BfRtDynHashCfgTableData *>(data);
  bf_status_t sts;

  // Get list of top field IDs and their corresponding names
  // and create a temp map
  std::map<bf_rt_id_t, std::string> name_map;
  std::vector<bf_rt_id_t> field_id_vec;
  sts = this->dataFieldIdListGet(&field_id_vec);
  if (sts != BF_SUCCESS) return sts;
  for (const auto &field_id : field_id_vec) {
    std::string field_name;
    sts = this->dataFieldNameGet(field_id, &field_name);
    if (sts != BF_SUCCESS) return sts;

    name_map[field_id] = field_name;
  }

  bf_rt_id_t order_field_id, start_bit_field_id, length_field_id;
  sts = dataFieldIdGet("order", &order_field_id);
  if (sts != BF_SUCCESS) return sts;

  sts = dataFieldIdGet("start_bit", &start_bit_field_id);
  if (sts != BF_SUCCESS) return sts;

  sts = dataFieldIdGet("length", &length_field_id);
  if (sts != BF_SUCCESS) return sts;

  // Get fl_hdl and then attr_list from pipe_mgr
  pipe_fld_lst_hdl_t fl_hdl;
  sts = pipeMgr->pipeMgrHashCalcInputGet(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &fl_hdl);
  if (sts != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in get field list handle",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }
  uint32_t num_attr_filled = 0;
  pipe_hash_calc_input_field_attribute_t *attr_list = nullptr;
  sts = pipeMgr->pipeMgrHashCalcInputFieldAttribute2Get(session.sessHandleGet(),
                                                        dev_tgt.dev_id,
                                                        pipe_tbl_hdl,
                                                        fl_hdl,
                                                        &attr_list,
                                                        &num_attr_filled);
  if (sts != BF_SUCCESS) return sts;

  // temp map to keep data objects in before final top setValue
  std::unordered_map<bf_rt_id_t, std::vector<std::unique_ptr<BfRtTableData>>>
      c_field_to_data_map;

  for (uint32_t i = 0; i < num_attr_filled; i++) {
    pipe_hash_calc_input_field_attribute_t *attr = &(attr_list[i]);

    // Figure out which top-field this attr slice belongs to
    bf_rt_id_t container_field_id = attr->input_field + 1;
    auto field_name = name_map.at(container_field_id);

    // Allocate a data object and set Value for all container fields
    std::unique_ptr<BfRtTableData> inner_data;
    sts = this->dataAllocateContainer(container_field_id, &inner_data);
    if (sts != BF_SUCCESS) goto cleanup;

    sts = inner_data->setValue(order_field_id,
                               static_cast<uint64_t>(attr->order));
    if (sts != BF_SUCCESS) goto cleanup;

    sts = inner_data->setValue(start_bit_field_id,
                               static_cast<uint64_t>(attr->slice_start_bit));
    if (sts != BF_SUCCESS) goto cleanup;

    // if pipe_mgr length is 0, don't set in BFRT. It will be defaulted to
    // end of slice. Setting 0 explicitly is not to be done because
    // that makes the order 0
    if (attr->slice_length != 0) {
      sts = inner_data->setValue(length_field_id,
                                 static_cast<uint64_t>(attr->slice_length));
      if (sts != BF_SUCCESS) goto cleanup;
    }

    if (c_field_to_data_map.find(container_field_id) ==
        c_field_to_data_map.end()) {
      c_field_to_data_map[container_field_id] =
          std::vector<std::unique_ptr<BfRtTableData>>{};
    }
    c_field_to_data_map[container_field_id].push_back(std::move(inner_data));
  }
  // for any con_field_id that wasn't received back from pipe_mgr, we need to
  // populate the data object with order 0 field slices
  for (const auto &id_name : name_map) {
    const auto &container_field_id = id_name.first;
    if (c_field_to_data_map.find(container_field_id) ==
        c_field_to_data_map.end()) {
      // Allocate a data object and set Value for all container fields
      std::unique_ptr<BfRtTableData> inner_data;
      sts = this->dataAllocateContainer(container_field_id, &inner_data);
      if (sts != BF_SUCCESS) goto cleanup;

      sts = inner_data->setValue(order_field_id, static_cast<uint64_t>(0));
      if (sts != BF_SUCCESS) goto cleanup;

      sts = inner_data->setValue(start_bit_field_id, static_cast<uint64_t>(0));
      if (sts != BF_SUCCESS) goto cleanup;
      // skip setting length to default to full field
      // set it in the map
      c_field_to_data_map[container_field_id].push_back(std::move(inner_data));
    }
  }
  // From the temp map, setValue per con_field_id
  for (auto &cf_dl_pair : c_field_to_data_map) {
    sts = match_data->setValue(cf_dl_pair.first, std::move(cf_dl_pair.second));
    if (sts != BF_SUCCESS) goto cleanup;
  }

cleanup:
  auto d_sts = pipeMgr->pipeMgrHashCalcAttributeListDestroy(attr_list);
  if (d_sts) {
    LOG_ERROR("%s:%d %s ERROR while destroying attr list ",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return d_sts;
  }
  if (sts) {
    LOG_ERROR("%s:%d %s ERROR while trying to Get default entry ",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return d_sts;
  }
  return sts;
}

bf_status_t BfRtDynHashCfgTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  bf_status_t sts = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  sts = pipeMgr->pipeMgrHashCalcInputDefaultSet(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl);
  if (BF_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in resetting to default hash field list",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return sts;
  }
  return sts;
}

bf_status_t BfRtDynHashCfgTable::tableClear(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

bf_status_t BfRtDynHashCfgTable::dataAllocate_internal(
    const bf_rt_id_t &container_id,
    std::unique_ptr<BfRtTableData> *data_ret,
    const std::vector<bf_rt_id_t> &fields) const {
  std::string field_name = "";
  if (container_id) {
    // container_id should exist
    auto sts = this->dataFieldNameGet(container_id, 0, &field_name);
    if (sts != BF_SUCCESS) {
      LOG_WARN("%s:%d:%s : Error while getting Container ID %d ",
               __func__,
               __LINE__,
               table_name_get().c_str(),
               container_id);
      return BF_INVALID_ARG;
    }
  }
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtDynHashCfgTableData(this, container_id, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  if (container_id) {
    auto data = static_cast<BfRtDynHashCfgTableData *>(data_ret->get());
    data->setDynHashFieldName(field_name);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDynHashCfgTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return this->dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtDynHashCfgTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return this->dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtDynHashCfgTable::dataAllocateContainer(
    const bf_rt_id_t &container_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> empty;
  return this->dataAllocate_internal(container_id, data_ret, empty);
}

bf_status_t BfRtDynHashCfgTable::dataAllocateContainer(
    const bf_rt_id_t &container_id,
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return this->dataAllocate_internal(container_id, data_ret, fields);
}

// BfRtDynHashAlgoTable****************

bf_status_t BfRtDynHashAlgoTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtDynHashAlgoTableData &match_data =
      static_cast<const BfRtDynHashAlgoTableData &>(data);
  bf_status_t status = BF_SUCCESS;

  // Figure out which action if given
  bf_rt_id_t action_id;
  status = match_data.actionIdGet(&action_id);

  std::string action_name = "";
  if (action_id) {
    this->actionNameGet(action_id, &action_name);
  }

  std::vector<bf_rt_id_t> dataFields;
  /* 3 cases
   * Not all fields are set
   * All fields are set but action_id is 0
   * All fields are set but action_id is non-zero.
   */
  bool all_fields_set = match_data.allFieldsSet();
  if (!all_fields_set) {
    dataFields.assign(match_data.getActiveFields().begin(),
                      match_data.getActiveFields().end());
  } else {
    if (action_id == 0) {
      LOG_TRACE(
          "%s:%d %s :Error: Need to provide an action ID for all fields for "
          "entryMod",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return BF_INVALID_ARG;
    } else {
      status = this->dataFieldIdListGet(action_id, &dataFields);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  status);
        return status;
      }
    }
  }

  bool is_algo_set = false;
  bool is_seed_set = false;
  uint64_t seed = 0;
  uint64_t rotate = 0;

  bfn_hash_algorithm_t algorithm = {};
  status = setHashAlgoFromDataObject(this,
                                     match_data,
                                     dataFields,
                                     &algorithm,
                                     &seed,
                                     &rotate,
                                     &is_algo_set,
                                     &is_seed_set);
  if (is_algo_set) {
    // Verify if the bfn_hash_algorithm_t struct can be realized or not
    // if this is a CRC algo
    if (algorithm.hash_alg == CRC_DYN) {
      char *err;
      auto alg_sts = verify_algorithm(&algorithm, &err);
      if (!alg_sts) {
        LOG_TRACE("%s:%d %s : Error : Failed to verify algorithm. Err: %s",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  err);
        return BF_INVALID_ARG;
      }
    }
    // First set the algorithm. An initial default seed is also set based upon
    // the algorithm as part of pipeMgrHashCalcAlgorithmSet
    // Then go ahead and set custom seed
    status = pipeMgr->pipeMgrHashCalcAlgorithmSet(session.sessHandleGet(),
                                                  dev_tgt.dev_id,
                                                  pipe_tbl_hdl,
                                                  -1,
                                                  &algorithm,
                                                  rotate);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set dyn hash algorithm",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  }
  if (is_seed_set) {
    status = pipeMgr->pipeMgrHashCalcSeedSet(
        session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, seed);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set seed %" PRIu64,
                __func__,
                __LINE__,
                table_name_get().c_str(),
                seed);
      return status;
    }
  }

  return status;
}

bf_status_t BfRtDynHashAlgoTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  BfRtDynHashAlgoTableData *algo_data =
      static_cast<BfRtDynHashAlgoTableData *>(data);
  bf_status_t status = BF_SUCCESS;
  uint64_t seed = 0;
  uint64_t rotate = 0;
  status = pipeMgr->pipeMgrHashCalcSeedGet(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           (pipe_hash_seed_t *)&seed);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Get Dyn Hashing Algorithm and Seed fail",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  bfn_hash_algorithm_t algorithm;
  pipe_hash_alg_hdl_t alg_hdl;
  status = pipeMgr->pipeMgrHashCalcAlgorithmGet(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                &alg_hdl,
                                                &algorithm,
                                                &rotate);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set dyn hash algorithm",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = getHashAlgoInDataObject(this, algorithm, rotate, seed, algo_data);
  return status;
}

bf_status_t BfRtDynHashAlgoTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeMgrHashCalcAlgorithmReset(
      session.sessHandleGet(), dev_tgt.dev_id, this->pipe_tbl_hdl);
}

bf_status_t BfRtDynHashAlgoTable::tableClear(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

bf_status_t BfRtDynHashAlgoTable::dataAllocate_internal(
    bf_rt_id_t action_id,
    std::unique_ptr<BfRtTableData> *data_ret,
    const std::vector<bf_rt_id_t> &fields) const {
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
      return BF_OBJECT_NOT_FOUND;
    }
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtDynHashAlgoTableData(this, action_id, fields));
  } else {
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtDynHashAlgoTableData(this, fields));
  }

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDynHashAlgoTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtDynHashAlgoTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
bf_status_t BfRtDynHashAlgoTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtDynHashAlgoTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

// BfRtDynHashComputeTable****************
bf_status_t BfRtDynHashComputeTable::getRef(pipe_tbl_hdl_t *hdl,
                                            const BfRtTable **cfg_tbl,
                                            uint32_t *hash_len) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find ref. Missing required DynHashConfig table reference"
        "in bf-rt.json");
    return BF_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid bf-rt.json configuration. compute should be part"
        " of exactly one config table.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  *hdl = it->second.back().tbl_hdl;
  auto tbl_id = it->second.back().id;

  // Get the cfg table
  auto status = this->bfRtInfoGet()->bfrtTableFromIdGet(tbl_id, cfg_tbl);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  auto cfg_tbl_obj = static_cast<const BfRtTableObj *>(*cfg_tbl);
  *hash_len = (cfg_tbl_obj->hash_bit_width_get() + 7) / 8;
  return BF_SUCCESS;
}

bf_status_t BfRtDynHashComputeTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtDynHashComputeTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDynHashComputeTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields{};
  if (data_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtDynHashComputeTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDynHashComputeTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = BF_SUCCESS;
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  const BfRtDynHashComputeTableKey &hash_key =
      static_cast<const BfRtDynHashComputeTableKey &>(key);
  BfRtDynHashComputeTableData *hash_data =
      static_cast<BfRtDynHashComputeTableData *>(data);

  uint32_t hash_len = 0;

  pipe_tbl_hdl_t cfg_tbl_hdl;
  const BfRtTable *cfg_table;
  status = this->getRef(&cfg_tbl_hdl, &cfg_table, &hash_len);
  std::vector<uint8_t> hash(hash_len, 0);
  if (status) {
    return status;
  }
  std::vector<pipe_hash_calc_input_field_attribute_t> attrs;
  status = hash_key.attrListGet(cfg_table, &attrs);

  do {
    if (status) {
      LOG_TRACE("%s:%d %s ERROR fail to get attrs from key",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      break;
    }
    status = pipeMgr->pipeMgrHashCalcCalculateHashValueWithCfg(
        session.sessHandleGet(),
        dev_tgt.dev_id,
        cfg_tbl_hdl,
        attrs.size(),
        attrs.data(),
        hash_len,
        hash.data());
  } while (0);

  for (auto &attr : attrs) {
    if (attr.value.stream && attr.type == INPUT_FIELD_ATTR_TYPE_STREAM)
      delete[] attr.value.stream;
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get hash value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  bf_rt_id_t hash_value_id;
  status = this->dataFieldIdGet("hash_value", 0, &hash_value_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  status = hash_data->setValue(hash_value_id, hash.data(), hash_len);
  return status;
}

// BfRtSelectorGetMemberTable****************
bf_status_t BfRtSelectorGetMemberTable::getRef(
    pipe_sel_tbl_hdl_t *sel_tbl_hdl,
    const BfRtSelectorTable **sel_tbl,
    const BfRtActionTable **act_tbl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find sel ref. Missing required sel table reference "
        "in bf-rt.json");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid bf-rt.json configuration. SelGetMem should be part"
        " of exactly one Selector table.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  *sel_tbl_hdl = it->second.back().tbl_hdl;
  auto sel_tbl_id = it->second.back().id;

  // Get the Sel table
  const BfRtTable *tbl;
  auto status = this->bfRtInfoGet()->bfrtTableFromIdGet(sel_tbl_id, &tbl);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  auto tbl_obj = static_cast<const BfRtTableObj *>(tbl);
  *sel_tbl = static_cast<const BfRtSelectorTable *>(tbl_obj);

  // Get action profile table
  status =
      this->bfRtInfoGet()->bfrtTableFromIdGet(tbl_obj->getActProfId(), &tbl);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  tbl_obj = static_cast<const BfRtTableObj *>(tbl);
  *act_tbl = static_cast<const BfRtActionTable *>(tbl_obj);
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtSelectorGetMemberTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  if (data_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  std::vector<bf_rt_id_t> fields{};
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtSelectorGetMemberTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status = BF_SUCCESS;
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  const BfRtSelectorGetMemberTableKey &sel_key =
      static_cast<const BfRtSelectorGetMemberTableKey &>(key);
  BfRtSelectorGetMemberTableData *sel_data =
      static_cast<BfRtSelectorGetMemberTableData *>(data);

  uint64_t grp_id = 0;
  bf_rt_id_t field_id;
  status = this->keyFieldIdGet("$SELECTOR_GROUP_ID", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get field ID for grp ID",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &grp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to grp_id",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint64_t hash = 0;
  status = this->keyFieldIdGet("hash_value", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &hash);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get hash",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  const BfRtSelectorTable *sel_tbl;
  const BfRtActionTable *act_tbl;
  pipe_sel_tbl_hdl_t sel_tbl_hdl;
  status = this->getRef(&sel_tbl_hdl, &sel_tbl, &act_tbl);
  if (status) {
    return status;
  }

  pipe_sel_grp_hdl_t grp_hdl;
  status = sel_tbl->getGrpHdl(session, dev_tgt, grp_id, &grp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  uint32_t num_bytes = sizeof(uint64_t);
  std::vector<uint8_t> hash_arr(num_bytes, 0);
  BfRtEndiannessHandler::toNetworkOrder(num_bytes, hash, hash_arr.data());

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  status = pipeMgr->pipeMgrSelGrpMbrGetFromHash(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                sel_tbl_hdl,
                                                grp_hdl,
                                                hash_arr.data(),
                                                num_bytes,
                                                &adt_ent_hdl);
  if (status) {
    LOG_ERROR(
        "%s:%d %s : Error : Unable to convert (hash,grp_hdl) to adt_mbr_hdl"
        " for hash %" PRIu64 " grp hdl %d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        hash,
        grp_hdl);
    return status;
  }
  uint32_t act_mbr_id = 0;
  status =
      act_tbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, &act_mbr_id);
  if (status) {
    LOG_ERROR("%s:%d %s : Error : Unable to get mbr ID for hdl %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              adt_ent_hdl);
    return status;
  }

  status = this->dataFieldIdGet("$ACTION_MEMBER_ID", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_data->setValue(field_id, act_mbr_id);
  return status;
}

// Debug counter

// Caller must do null pointer check.
char **BfRtTblDbgCntTable::allocDataForTableNames() const {
  char **tbl_names = new char *[this->_table_size];
  if (tbl_names != NULL) {
    for (uint32_t i = 0; i < this->_table_size; i++) {
      tbl_names[i] = new char[PIPE_MGR_TBL_NAME_LEN];
      std::memset(tbl_names[i], 0, PIPE_MGR_TBL_NAME_LEN);
    }
  }
  return tbl_names;
}

void BfRtTblDbgCntTable::freeDataForTableNames(char **tbl_names) const {
  for (uint32_t i = 0; i < this->_table_size; i++) {
    delete[] tbl_names[i];
  }
  delete[] tbl_names;
}

bf_status_t BfRtTblDbgCntTable::tableEntryMod(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  const BfRtTblDbgCntTableData &dbg_data =
      static_cast<const BfRtTblDbgCntTableData &>(data);
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t field_id;
  auto status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  std::string type_str;
  status = dbg_data.getValue(field_id, &type_str);
  if (status) return status;
  auto it = std::find(cntTypeStr.begin(), cntTypeStr.end(), type_str);
  if (it == cntTypeStr.end()) {
    LOG_TRACE("%s:%d %s ERROR invalid value in debug counter table key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return BF_INVALID_ARG;
  }

  bf_tbl_dbg_counter_type_t type =
      static_cast<bf_tbl_dbg_counter_type_t>(it - cntTypeStr.begin());
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  // Convert name to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  status =
      pipeMgr->bfDbgCounterSet(tgt, const_cast<char *>(tbl_name.c_str()), type);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }
  return status;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  BfRtTblDbgCntTableData &dbg_data =
      static_cast<BfRtTblDbgCntTableData &>(*data);
  bf_tbl_dbg_counter_type_t type;
  uint32_t value;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  // Convert name to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  auto status = pipeMgr->bfDbgCounterGet(
      tgt, const_cast<char *>(tbl_name.c_str()), &type, &value);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }

  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, cntTypeStr.at(type));
  if (status) return status;

  status = this->dataFieldIdGet("value", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, static_cast<uint64_t>(value));
  if (status) return status;

  return status;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  *num_returned = 0;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }
  // Translate to vector for easier operations
  std::vector<std::string> tbl_names;
  for (int i = 0; i < num_tbls; i++) {
    tbl_names.push_back(tbl_list[i]);
  }
  this->freeDataForTableNames(tbl_list);

  // Sort and remove duplicates
  tbl_names.erase(std::unique(tbl_names.begin(), tbl_names.end()),
                  tbl_names.end());
  std::sort(tbl_names.begin(), tbl_names.end());
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  // Convert key to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  auto it = std::find(tbl_names.begin(), tbl_names.end(), tbl_name);
  if (it == tbl_names.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Move to next element
  it++;

  for (uint32_t i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtTblDbgCntTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    // If run out of entries, mark remaning data as empty.
    if (it == tbl_names.end()) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    tbl_name = getQualifiedTableName(dev_tgt.dev_id, this->prog_name, *it);
    this_key->setTblName(tbl_name);
    status = this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
    it++;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  BfRtTblDbgCntTableKey *dbg_key = static_cast<BfRtTblDbgCntTableKey *>(key);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }
  // Translate to vector for easier operations
  std::vector<std::string> tbl_names;
  for (int i = 0; i < num_tbls; i++) {
    tbl_names.push_back(tbl_list[i]);
  }
  this->freeDataForTableNames(tbl_list);
  // Sort and remove duplicates
  tbl_names.erase(std::unique(tbl_names.begin(), tbl_names.end()),
                  tbl_names.end());
  std::sort(tbl_names.begin(), tbl_names.end());

  std::string new_name =
      getQualifiedTableName(dev_tgt.dev_id, this->prog_name, tbl_names[0]);
  // compare returns 0 on equal
  dbg_key->setTblName(new_name);
  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtTblDbgCntTable::tableClear(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }

  for (int i = 0; i < num_tbls; ++i) {
    status = pipeMgr->bfDbgCounterClear(tgt, tbl_list[i]);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Error while clearing debug counter for table %s, %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                tbl_list[i],
                status);
      break;
    }
  }
  this->freeDataForTableNames(tbl_list);
  return status;
}

bf_status_t BfRtTblDbgCntTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  if (type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s This Table Attributes only support "
        "ENTRY_SCOPE setting",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::tableAttributesGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    BF_RT_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  return tbl_attr_impl->entryScopeParamsSet(
      TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE);
}

bf_status_t BfRtTblDbgCntTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTblDbgCntTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

bf_status_t BfRtTblDbgCntTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtTblDbgCntTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Log table debug counter
bf_status_t BfRtLogDbgCntTable::tableEntryMod(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  const BfRtLogDbgCntTableKey &dbg_key =
      static_cast<const BfRtLogDbgCntTableKey &>(key);
  uint64_t stage, log_tbl;
  bf_rt_id_t field_id;
  auto status = this->keyFieldIdGet("stage", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &stage);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &log_tbl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  const BfRtTblDbgCntTableData &dbg_data =
      static_cast<const BfRtTblDbgCntTableData &>(data);
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  std::string type_str;
  status = dbg_data.getValue(field_id, &type_str);
  if (status) return status;
  auto it = std::find(cntTypeStr.begin(), cntTypeStr.end(), type_str);
  if (it == cntTypeStr.end()) {
    LOG_TRACE("%s:%d %s ERROR invalid value in debug counter table key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return BF_INVALID_ARG;
  }

  bf_tbl_dbg_counter_type_t type =
      static_cast<bf_tbl_dbg_counter_type_t>(it - cntTypeStr.begin());
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfDbgCounterSet(tgt, stage, log_tbl, type);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting debug counter for pipe %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtLogDbgCntTable::tableEntryGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  const BfRtLogDbgCntTableKey &dbg_key =
      static_cast<const BfRtLogDbgCntTableKey &>(key);
  BfRtTblDbgCntTableData &dbg_data =
      static_cast<BfRtTblDbgCntTableData &>(*data);
  uint64_t stage, log_tbl;
  bf_rt_id_t field_id;
  auto status = this->keyFieldIdGet("stage", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &stage);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &log_tbl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              field_id);
    return status;
  }

  bf_tbl_dbg_counter_type_t type;
  uint32_t value;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfDbgCounterGet(tgt, stage, log_tbl, &type, &value);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting debug counter for pipe %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id,
              status);
    return status;
  }

  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, cntTypeStr.at(type));
  if (status) return status;

  status = this->dataFieldIdGet("value", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, static_cast<uint64_t>(value));
  if (status) return status;

  return status;
}

bf_status_t BfRtLogDbgCntTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  *num_returned = 0;
  // 16 logical tables per stage
  uint64_t num_tbls = 16;
  // Total table size is nuber of logical tables per stage multiplied by number
  // of stages.
  uint64_t last_stage = this->_table_size / num_tbls;
  const BfRtLogDbgCntTableKey &dbg_key =
      static_cast<const BfRtLogDbgCntTableKey &>(key);
  bf_rt_id_t stage_field_id, log_tbl_field_id;
  uint64_t stage, log_tbl;
  auto status = this->keyFieldIdGet("stage", &stage_field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(stage_field_id, &stage);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              stage_field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &log_tbl_field_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(log_tbl_field_id, &log_tbl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              log_tbl_field_id);
    return status;
  }

  // Grab next entry
  if (stage >= last_stage || log_tbl >= num_tbls) {
    return BF_INVALID_ARG;
  } else if (log_tbl == num_tbls - 1) {
    log_tbl = 0;
    stage++;
  } else {
    log_tbl++;
  }
  uint32_t i = 0;
  for (uint64_t stage_id = stage; stage_id < last_stage; stage_id++) {
    for (uint64_t tbl_id = log_tbl; tbl_id < num_tbls && i < n; tbl_id++, i++) {
      auto this_key =
          static_cast<BfRtLogDbgCntTableKey *>((*key_data_pairs)[i].first);
      auto this_data = (*key_data_pairs)[i].second;

      status = this_key->setValue(log_tbl_field_id, tbl_id);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  log_tbl_field_id);
        return status;
      }
      status = this_key->setValue(stage_field_id, stage_id);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  stage_field_id);
        return status;
      }
      status =
          this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  i + 1,
                  status);
        // Make the data object null if error
        (*key_data_pairs)[i].second = nullptr;
      }
      (*num_returned)++;
    }
    log_tbl = 0;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtLogDbgCntTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtLogDbgCntTableKey *dbg_key = static_cast<BfRtLogDbgCntTableKey *>(key);
  std::string field_names[2] = {"stage", "log_tbl"};
  for (auto &field_name : field_names) {
    bf_rt_id_t field_id;
    auto status = this->keyFieldIdGet(field_name, &field_id);
    if (status != BF_SUCCESS) {
      BF_RT_DBGCHK(0);
      return status;
    }
    status = dbg_key->setValue(field_id, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                field_id);
      return status;
    }
  }

  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtLogDbgCntTable::tableClear(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;
  // 16 logical tables per stage
  uint32_t num_tbls = 16;
  // Total table size is nuber of logical tables per stage multiplied by number
  // of stages.
  uint32_t last_stage = this->_table_size / num_tbls;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  for (uint32_t stage = 0; stage < last_stage; stage++) {
    for (uint32_t log_tbl = 0; log_tbl < num_tbls; ++log_tbl) {
      status = pipeMgr->bfDbgCounterClear(tgt, stage, log_tbl);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Error while clearing debug counter for stage %d, log_tbl "
            "%d, err=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            stage,
            log_tbl,
            status);
        break;
      }
    }
  }
  return status;
}

bf_status_t BfRtLogDbgCntTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtLogDbgCntTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtLogDbgCntTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtLogDbgCntTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

bf_status_t BfRtLogDbgCntTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // This table reuses TblDbgCnt data object.
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtTblDbgCntTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Register param table
// Table reuses debug counter table data object.
bf_status_t BfRtRegisterParamTable::getRef(pipe_tbl_hdl_t *hdl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot set register param. Missing required register table reference "
        "in bf-rt.json.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid bf-rt.json configuration. Register param should be part"
        " of exactly one register table.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  *hdl = it->second[0].tbl_hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterParamTable::getParamHdl(const BfRtSession &session,
                                                bf_dev_id_t dev_id) const {
  pipe_reg_param_hdl_t reg_param_hdl;
  // Get param name from table name. Have to remove pipe name.
  std::string param_name = this->table_name_get();
  param_name = param_name.erase(0, param_name.find(".") + 1);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status =
      pipeMgr->pipeStfulParamGetHdl(dev_id, param_name.c_str(), &reg_param_hdl);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to get register param handle from pipe mgr",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return 0;
  }

  return reg_param_hdl;
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  const BfRtRegisterParamTableData &mdata =
      static_cast<const BfRtRegisterParamTableData &>(data);
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamSet(session.sessHandleGet(),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    mdata.value);
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }
  BfRtRegisterParamTableData *mdata =
      static_cast<BfRtRegisterParamTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamGet(session.sessHandleGet(),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    &mdata->value);
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamReset(
      session.sessHandleGet(),
      pipe_dev_tgt,
      tbl_hdl,
      this->getParamHdl(session, dev_tgt.dev_id));
}

bf_status_t BfRtRegisterParamTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtRegisterParamTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

}  // namespace bfrt
