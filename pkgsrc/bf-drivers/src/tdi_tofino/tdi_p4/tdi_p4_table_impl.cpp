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

// tdi include
#include <tdi/common/tdi_init.hpp>

// local tofino includes
#include <tdi_common/tdi_tofino_target.hpp>
#include <tdi_common/tdi_state.hpp>
#include <tdi_common/tdi_tofino_target.hpp>
#include <tdi_common/tdi_table_attributes_impl.hpp>
#include <tdi_p4/tdi_p4_table_impl.hpp>
#include <tdi_p4/tdi_p4_table_key_impl.hpp>
#include <tdi_p4/tdi_table_attributes_state.hpp>

#define CHECK_PRINT_RETURN(sts, str)                  \
  do {                                                \
    if (sts != TDI_SUCCESS) {                         \
      LOG_ERROR("%s:%d %s", __func__, __LINE__, str); \
      return TDI_INVALID_ARG;                         \
    }                                                 \
  } while (0)

namespace tdi {
namespace tna {
namespace tofino {
namespace {

// getActionSpec fetches both resources and action_spec from Pipe Mgr.
// If some resources are unsupported it will filter them out.
tdi_status_t getActionSpec(const tdi::Session &session,
                           const dev_target_t &dev_tgt,
                           const tdi::Flags &flags,
                           const pipe_tbl_hdl_t &pipe_tbl_hdl,
                           const pipe_mat_ent_hdl_t &mat_ent_hdl,
                           uint32_t res_get_flags,
                           pipe_tbl_match_spec_t *pipe_match_spec,
                           pipe_action_spec_t *pipe_action_spec,
                           pipe_act_fn_hdl_t *act_fn_hdl,
                           pipe_res_get_data_t *res_data) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = TDI_SUCCESS;
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (pipe_match_spec) {
    status = pipeMgr->pipeMgrGetEntry(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
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
    status = pipeMgr->pipeMgrTableGetDefaultEntry(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
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

template <class T>
tdi_status_t setIdleTable(T &table,
                          const tdi::Session &session,
                          const dev_target_t &pipe_dev_tgt,
                          const TableAttributes &tbl_attr) {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes &>(tbl_attr);
  // Use callback option with match_spec pointer
  auto ttl_params = tbl_attr_impl.getIdleTableTtlParamsInternal();
  ttl_params.u.notify.default_callback_choice = 1;
  ttl_params.u.notify.callback_fn2 =
      tdi::tna::tofino::tdiIdleTmoExpiryInternalCb;
  // Save this callback separately and register the internal cb
  auto callback_cpp = tbl_attr_impl.getIdleTableCallback();
  auto callback_c = tbl_attr_impl.getIdleTableCallbackC();
  auto enabled = tbl_attr_impl.getIdleTableEnabled();

  // Get the state and set enabled/cb/cookie
  const tdi::Device *device;
  auto status =
      DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          table.tdiInfoGet()->p4NameGet());
  // Update the state
  auto attr_state =
      device_state->attributesState.getObjState(table.tableInfoGet()->idGet());
  StateTableAttributesAging aging(enabled,
                                  callback_cpp,
                                  callback_c,
                                  &table,
                                  ttl_params.u.notify.client_data);
  attr_state->setAgingAttributeObj(aging);

  // Now all modification to the ttl_params to suit our needs will go here
  // That includes setting mode
  ttl_params.u.notify.client_data = &attr_state->getAgingAttributeObj();

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());

  // Apply new config in pipe_mgr
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrIdleParamsSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      ttl_params);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to set params in pipe_mgr",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = pipeMgr->pipeMgrIdleTmoEnableSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      enabled);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to %s idle table in pipe_mgr",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
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
  table.idletimeCbTdiThreadPoolReset(nullptr);
  table.idleTableStateGet()->setPollMode(ttl_params.mode == POLL_MODE);
  // Create a thread pool only for the enabled notify case
  if (enabled && table.idleTableStateGet()->isIdleTableinPollMode() == false) {
    table.idletimeCbTdiThreadPoolReset(new tdi::TdiThreadPool());
  }
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t getIdleTable(const T &table,
                          const tdi::Session &session,
                          const tdi::Target &dev_tgt,
                          tdi::TableAttributes *tbl_attr) {
  auto tbl_attr_impl =
      static_cast<tdi::tna::tofino::TableAttributes *>(tbl_attr);
  // Get the info from tableAttributes
  pipe_idle_time_params_t ttl_params;
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto status = pipeMgr->pipeMgrIdleParamsGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      &ttl_params);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d %s Failed to get params info from pipe_mgr",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return status;
  }

  tdi_tofino_attributes_idle_table_mode_e tdi_mode =
      static_cast<tdi_tofino_attributes_idle_table_mode_e>(ttl_params.mode);
  if (tdi_mode > TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_INVALID_MODE) {
    LOG_ERROR("%s:%d %s Invalid Idle Table Mode found programmed in pipe mgr",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  // Set the idle table mode in the attributes obj
  tbl_attr_impl->idleTableModeSet(tdi_mode);

  // Get the state and get enabled/cb/cookie
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          table.tdiInfoGet()->p4NameGet());
  // Get the state
  auto attr_state =
      device_state->attributesState.getObjState(table.tableInfoGet()->idGet());
  auto aging_state = attr_state->getAgingAttributeObj();
  auto t = aging_state.stateTableAttributesAgingGet();

  if (tdi_mode == TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE) {
    return tbl_attr_impl->idleTablePollModeSet(std::get<0>(t));
  } else if (tdi_mode == TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE) {
    // enabled, cb, ttl_query_interval, max_ttl, min_ttl, cookie
    return tbl_attr_impl->idleTableNotifyModeSet(
        std::get<0>(t),
        std::get<1>(t),
        ttl_params.u.notify.ttl_query_interval,
        ttl_params.u.notify.max_ttl,
        ttl_params.u.notify.min_ttl,
        ttl_params.u.notify.client_data);
  }
  return TDI_SUCCESS;
}
// used by dynamic key mask attribute, from key field mask to match spec
tdi_status_t fieldMasksToMatchSpec(
    const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask,
    pipe_tbl_match_spec_t *mat_spec,
    const tdi::Table *table) {
  for (const auto &field : field_mask) {
    const tdi::KeyFieldInfo *key_field =
        table->tableInfoGet()->keyFieldGet(field.first);
    if (key_field == nullptr) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field.first);
      return TDI_OBJECT_NOT_FOUND;
    }
    auto &sz = key_field->sizeGet();
    if (((sz + 7) / 8) != field.second.size()) {
      LOG_TRACE("%s:%d %s ERROR Invalid key field mask size, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field.first);
      return TDI_INVALID_ARG;
    }
    auto key_field_context_info =
        static_cast<const TofinoKeyFieldContextInfo *>(
            key_field->keyFieldContextInfoGet());
    if (key_field_context_info->isFieldSlice()) {
      tdi::tna::tofino::MatchActionKey::packFieldIntoMatchSpecByteBuffer(
          *key_field,
          field.second.size(),
          true, /* Must be true for DKM */
          field.second.data(),
          field.second.data(),
          mat_spec->match_value_bits + key_field_context_info->fieldOffsetGet(),
          mat_spec->match_mask_bits + key_field_context_info->fieldOffsetGet(),
          table);
    } else {
      std::memcpy(
          mat_spec->match_mask_bits + key_field_context_info->fieldOffsetGet(),
          field.second.data(),
          field.second.size());
    }
  }
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());
  mat_spec->num_match_bytes = table_context_info->keySizeGet().bytes;
  mat_spec->num_valid_match_bits = table_context_info->keySizeGet().bits;
  return TDI_SUCCESS;
}
// used by dynamic key mask attribute, from match spec to key field mask
tdi_status_t matchSpecToFieldMasks(
    const pipe_tbl_match_spec_t &mat_spec,
    std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask,
    const tdi::Table *table) {
  if (mat_spec.match_mask_bits == nullptr) return TDI_INVALID_ARG;
  std::vector<tdi_id_t> id_vec = table->tableInfoGet()->keyFieldIdListGet();
  int cnt = 0;
  for (auto field_id : id_vec) {
    const tdi::KeyFieldInfo *key_field =
        table->tableInfoGet()->keyFieldGet(field_id);
    if (key_field == nullptr) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_OBJECT_NOT_FOUND;
    }
    auto key_field_context_info =
        static_cast<const TofinoKeyFieldContextInfo *>(
            key_field->keyFieldContextInfoGet());
    cnt = key_field_context_info->fieldOffsetGet();
    auto sz = key_field->sizeGet();
    sz = (sz + 7) / 8;
    std::vector<uint8_t> mask(sz);
    for (uint32_t i = 0; i < sz; i++) {
      mask[i] = (mat_spec.match_mask_bits[cnt + i]);
    }
    if (mask.empty()) {
      LOG_TRACE("%s:%d %s Invalid mask for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_INVALID_ARG;
    }
    if (field_mask->find(field_id) != field_mask->end()) {
      LOG_WARN(
          "%s:%d %s Field id %d has been configured duplicatedly, use the "
          "latest configuration",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str(),
          field_id);
      field_mask->at(field_id) = mask;
    } else {
      field_mask->emplace(std::make_pair(field_id, mask));
    }
  }
  return TDI_SUCCESS;
}
template <class T>
tdi_status_t commonAttributesSet(const T *table,
                                 const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::TableAttributes &tableAttributes) {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = table->tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  auto table_context_info = static_cast<const MatchActionTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      tdi_tofino_attributes_entry_scope_e entry_scope;
      TableEntryScopeArguments scope_args(0);
      tdi_status_t sts = tbl_attr_impl->entry_scope_.entryScopeParamsGet(
          &entry_scope, static_cast<TableEntryScopeArguments *>(&scope_args));
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from TDI entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<32> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ulong();

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrTblSetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          prop_type,
          prop_val,
          args_val);
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      const tdi::Device *device;
      auto status =
          DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  pipe_dev_tgt.device_id,
                  status);
        return TDI_OBJECT_NOT_FOUND;
      }
      auto device_state =
          static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
              table->tdiInfoGet()->p4NameGet());

      // Set ENTRY_SCOPE of the table
      auto attributes_state = device_state->attributesState.getObjState(
          table->tableInfoGet()->idGet());
      attributes_state->setEntryScope(entry_scope);
      return sts;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      tdi_status_t sts = tbl_attr_impl->dynKeyMaskGet(&field_mask);
      pipe_tbl_match_spec_t match_spec;
      const int sz = table_context_info->keySizeGet().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      match_spec.match_mask_bits = match_mask_bits.data();
      // For this case value_bits will be modified the same way as mask, but
      // cannot be left uninitialized.
      match_spec.match_value_bits = match_mask_bits.data();
      // translate from map to match_spec
      sts = fieldMasksToMatchSpec(field_mask, &match_spec, table);
      if (sts != TDI_SUCCESS) return sts;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrMatchKeyMaskSpecSet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          &match_spec);
      return sts;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      int byte_count;
      tdi_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != TDI_SUCCESS) {
        return sts;
      }

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          table_context_info->resourceHdlGet(DataFieldType::METER_SPEC_CIR_PPS);
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          res_hdl,
          byte_count);
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
      return setIdleTable(
          const_cast<T &>(*table), session, pipe_dev_tgt, *tbl_attr_impl);
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t commonAttributesGet(const T *table,
                                 const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 tdi::TableAttributes *tableAttributes) {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl =
      static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = table->tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  auto table_context_info = static_cast<const MatchActionTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      auto sts = pipeMgr->pipeMgrTblGetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          prop_type,
          &prop_val,
          &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str());
        return sts;
      }

      tdi_tofino_attributes_entry_scope_e entry_scope;
      TableEntryScopeArguments scope_args(args_val.value);

      // Derive TDI entry scope from pipe mgr entry scope
      entry_scope =
          prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              ? TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES
              : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    ? TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE
                    : TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entry_scope_.entryScopeParamsSet(
          entry_scope, static_cast<TableEntryScopeArguments &>(scope_args));
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
      pipe_tbl_match_spec_t mat_spec;
      const int sz = table_context_info->keySizeGet().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      mat_spec.match_mask_bits = match_mask_bits.data();
      mat_spec.num_match_bytes = sz;
      mat_spec.num_valid_match_bits = table_context_info->keySizeGet().bits;

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMatchKeyMaskSpecGet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          &mat_spec);
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      matchSpecToFieldMasks(mat_spec, &field_mask, table);
      return tbl_attr_impl->dynKeyMaskSet(field_mask);
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
      return getIdleTable<T>(*table, session, dev_tgt, tbl_attr_impl);
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      int byte_count;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          table_context_info->resourceHdlGet(DataFieldType::METER_SPEC_CIR_PPS);
      auto sts = pipeMgr->pipeMgrMeterByteCountGet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          res_hdl,
          &byte_count);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get meter bytecount adjust from pipe_mgr",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str());
        return sts;
      }
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t entryModInternal(const tdi::Table &table,
                              const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableData &data,
                              const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto &match_data = static_cast<const MatchActionTableData &>(data);
  auto &match_table = static_cast<const MatchActionDirect &>(table);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_id_t action_id = match_data.actionIdGet();

  std::vector<tdi_id_t> dataFields;
  if (match_data.allFieldsSetGet()) {
    dataFields = table.tableInfoGet()->dataFieldIdListGet(action_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else {
    dataFields.assign(match_data.activeFieldsGet().begin(),
                      match_data.activeFieldsGet().end());
  }

  pipe_action_spec_t pipe_action_spec = {0};
  match_data.copy_pipe_action_spec(&pipe_action_spec);

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
    auto tableDataField =
        table.tableInfoGet()->dataFieldGet(dataFieldId, action_id);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                          tableDataField->dataFieldContextInfoGet())
                          ->typesGet();
    fieldDestination field_destination =
        TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
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
  auto table_context_info =
      static_cast<const MatchActionDirectTableContextInfo *>(
          table.tableInfoGet()->tableContextInfoGet());

  if (action_spec_found) {
    auto table_type = static_cast<tdi_tofino_table_type_e>(
        table.tableInfoGet()->tableTypeGet());
    pipe_act_fn_hdl_t act_fn_hdl = 0;
    if (table_type == TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT ||
        table_type == TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
      // If we are modifying the action spec for a match action indirect or
      // match action selector table, we need to verify the member ID or the
      // selector group id referenced here is legit.

      const MatchActionIndirectTableData &match_indir_data =
          static_cast<const MatchActionIndirectTableData &>(match_data);

      const MatchActionIndirect &mat_indir_table =
          static_cast<const MatchActionIndirect &>(table);
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

      if (status != TDI_SUCCESS) {
        if (match_indir_data.isGroup()) {
          if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not exist in the group table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getGroupId());
            return TDI_OBJECT_NOT_FOUND;
          } else if (adt_ent_hdl ==
                     MatchActionIndirectTableData::invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not have any members in the group table "
                "associated with the table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getGroupId());
            return TDI_OBJECT_NOT_FOUND;
          }
        } else {
          if (adt_ent_hdl ==
              MatchActionIndirectTableData::invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a action "
                "member id %d which does not exist in the action profile table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getActionMbrId());
            return TDI_OBJECT_NOT_FOUND;
          }
        }
      }
      if (match_indir_data.isGroup()) {
        pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
      } else {
        pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
      }
    } else {
      // This is a direct table. The action fn hdl needs to be
      // retrieved from the MAT data and not the action profile state
      act_fn_hdl = match_data.getActFnHdl();
    }
    status = pipeMgr->pipeMgrMatEntSetAction(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        act_fn_hdl,
        &pipe_action_spec,
        0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else if (direct_resource_found) {
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        pipe_action_spec.resources,
        pipe_action_spec.resource_count,
        0 /* Pipe API flags */);

    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in modifying resources part of table data, err %d",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          status);
      return status;
    }
  }

  if (direct_counter_found) {
    const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                            .getCounterSpecObj()
                                            .getPipeCounterSpec();
    status = pipeMgr->pipeMgrMatEntDirectStatSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying counter, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }

  if (ttl_found) {
    if (match_table.idleTablePollMode()) {
      status = pipeMgr->pipeMgrIdleTimeSetHitState(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          mat_ent_hdl,
          match_data.get_entry_hit_state());
    } else {
      bool reset = true;
      flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_SKIP_TTL_RESET),
                     &reset);

      status = pipeMgr->pipeMgrMatEntSetIdleTtl(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          mat_ent_hdl,
          match_data.get_ttl(),
          0 /* Pipe API flags */,
          reset);
    }
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying entry idle value, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }

  return TDI_SUCCESS;
}

template <typename T>
tdi_status_t getTableUsage(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const T &table,
                           uint32_t *count) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  tdi_status_t status = pipeMgr->pipeMgrGetEntryCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      read_from_hw,
      count);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting to usage for table, err %d (%s)",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              status,
              bf_err_str(status));
  }
  return status;
}

template <typename T>
tdi_status_t getReservedEntries(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const T &table,
                                size_t *size) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrGetReservedEntryCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting reserved entries count for table, err %d "
        "(%s)",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        status,
        bf_err_str(status));
  }
  return status;
}

// Template function for getFirst for Indirect meters, LPF, WRED and register
// tables
template <typename Tbl, typename Key>
tdi_status_t getFirst_for_resource_tbls(const Tbl &table,
                                        const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        Key *key,
                                        tdi::TableData *data) {
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != table.tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  // First entry in a index based table is idx 0
  key->setIdxKey(0);

  return table.entryGet(session, dev_tgt, flags, *key, data);
}

// Template function for getNext_n for Indirect meters, LPF, WRED and register
// tables
template <typename Tbl, typename Key>
tdi_status_t getNext_n_for_resource_tbls(
    const Tbl &table,
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const Key &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) {
  tdi_status_t status = TDI_SUCCESS;
  size_t table_size = 0;
  status = table.sizeGet(session, dev_tgt, flags, &table_size);
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

    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != table.tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }

    const Table *table_from_key;
    this_key->tableGet(&table_from_key);
    auto table_id_from_key = table_from_key->tableInfoGet()->idGet();
    if (table_id_from_key != table.tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table key object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          table_id_from_key);
      return TDI_INVALID_ARG;
    }

    status = table.entryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting counter index %d, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                i,
                status);
      // Make the data object null if error
      (*key_data_pairs)[j].second = nullptr;
    }

    (*num_returned)++;
  }
  return TDI_SUCCESS;
}

// This function checks if the key idx (applicable for Action profile, selector,
// Indirect meter, Counter, LPF, WRED, Register tables) is within the bounds of
// the size of the table

bool verify_key_for_idx_tbls(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Table &table,
                             uint32_t idx) {
  size_t table_size;
  tdi::Flags flags(0);
  table.sizeGet(session, dev_tgt, flags, &table_size);
  if (idx < table_size) {
    return true;
  }
  LOG_ERROR("%s:%d %s : ERROR Idx %d for key exceeds the size of the table %zd",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            idx,
            table_size);
  return false;
}

template <class Table, class Key>
tdi_status_t key_reset(const Table & /*table*/, Key *match_key) {
// TODO(sayanb)
#if 0
  if (!table.validateTable_from_keyObj(*match_key)) {
    LOG_TRACE("%s:%d %s ERROR : Key object is not associated with the table",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return match_key->reset();
}

tdi_status_t clearMatCommon(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const bool &&reset_default_entry,
                            const tdi::Table *table) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());

  // Clear the table
  tdi_status_t status = pipeMgr->pipeMgrMatTblClear(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0 /* pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Failed to clear table %s, err %d",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  if (reset_default_entry) {
    tdi::Flags flags(0);
    status = table->defaultEntryReset(session, dev_tgt, flags);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in resetting default entry , err %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
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
tdi_status_t resourceCheckAndInitialize(const tdi::Table &tbl,
                                        const T &tbl_data,
                                        const bool is_default) {
  // Get the pipe action spec from the data
  T &data = const_cast<T &>(tbl_data);
  // auto &match_table = static_cast<const MatchActionDirect &>(tbl);
  auto table_context_info =
      static_cast<const MatchActionDirectTableContextInfo *>(
          tbl.tableInfoGet()->tableContextInfoGet());
  PipeActionSpec &pipe_action_spec_obj = data.getPipeActionSpecObj();
  pipe_action_spec_t *pipe_action_spec =
      pipe_action_spec_obj.getPipeActionSpec();
  bool meter = false, reg = false, stat = false;
  const auto &action_id = data.actionIdGet();
  table_context_info->actionResourcesGet(action_id, &meter, &reg, &stat);

  // We can get the indirect count from the data object directly by
  // counting the number of resource index field types set
  // However, for direct count, that is not possible since one
  // resource can have many different fields. We count direct
  // resources set by going through table_ref_map.
  // const auto &actual_indirect_count = tbl_data.indirectResCountGet();
  uint32_t actual_direct_count = 0;

  // Get a map of all the resources(direct and indirect) for this table
  auto &table_ref_map = table_context_info->tableRefMapGet();
  if (table_ref_map.size() == 0) {
    // Nothing to be done. Just return
    return TDI_SUCCESS;
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
  //      tbl.tableInfoGet()->nameGet().c_str());
  //  return TDI_INVALID_ARG;
  //}
  if (programmed_direct_count == actual_direct_count) {
    // If the direct resource count is equal to the resource count in the
    // formed pipe action spec, we don't need to do anything. as it means that
    // the user has explicitly set the values of all the resources attached
    // to this table already
    return TDI_SUCCESS;
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
        tbl.tableInfoGet()->nameGet().c_str(),
        programmed_direct_count,
        actual_direct_count);
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
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
            tbl.tableInfoGet()->nameGet().c_str(),
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
  return TDI_SUCCESS;
}

bool checkDefaultOnly(const Table *table, const tdi::TableData &data) {
  const auto &action_id = data.actionIdGet();

  auto &annotations =
      table->tableInfoGet()->actionGet(action_id)->annotationsGet();

  auto def_an = Annotation("@defaultonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

bool checkTableOnly(const Table *table, const tdi_id_t &action_id) {
  if (!action_id) return false;

  auto &annotations =
      table->tableInfoGet()->actionGet(action_id)->annotationsGet();
  auto def_an = Annotation("@tableonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

#if 0
{
tdi_status_t setHashAlgoFromDataObject(const tdi::Table *table,
                                      const DynHashAlgoTableData &data,
                                      const std::vector<tdi_id_t> &dataFields,
                                      bfn_hash_algorithm_t *algorithm,
                                      uint64_t *seed,
                                      bool *is_algo_set,
                                      bool *is_seed_set) {
  bfn_crc_alg_t crc_type = CRC_INVALID;
  bfn_hash_alg_type_t alg_type = INVALID_DYN;

  const auto &action_id = data.actionIdGet();

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
    const tdi::DataFieldInfo *tableDataField = nullptr;
    if (action_id) {
      status = table->dataFieldGet(field_id, action_id, &tableDataField);
    } else {
      status = table->dataFieldGet(field_id, &tableDataField);
    }

    if (tableDataField->getName() == "seed") {
      status = data.getValue(field_id, seed);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_seed_set = true;
    } else if (tableDataField->getName() == "msb") {
      status = data.getValue(field_id, &msb);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "extend") {
      status = data.getValue(field_id, &extend);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "algorithm_name") {
      status = data.getValue(field_id, &algo_name);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
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
        return TDI_INVALID_ARG;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "hash_bit_width") {
      // reinterpret_cast is dangerous but we are fairly certain that
      // hash_bit_width will always be a positive value within bounds.
      status = data.getValue(field_id, &hash_bit_width);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "reverse") {
      status = data.getValue(field_id, &algorithm->reverse);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "polynomial") {
      status = data.getValue(field_id, &algorithm->poly);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "init") {
      status = data.getValue(field_id, &algorithm->init);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
      *is_algo_set = true;
    } else if (tableDataField->getName() == "final_xor") {
      status = data.getValue(field_id, &algorithm->final_xor);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str(),
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

tdi_status_t getHashAlgoInDataObject(const tdi::Table *table,
                                    const bfn_hash_algorithm_t &algorithm,
                                    const uint64_t &seed,
                                    DynHashAlgoTableData *data) {
  // If alg_type == CRC_DYN, crc_type == CRC_INVALID, then must be user_defined
  // If alg_type == CRC_DYN, crc_type != CRC_INVALID, then must be predefined
  // If alg_type == either of RANDOM_DYN, IDENTITY_DYN, XOR_DYN, then also must
  // be predefined (non-CRC)
  tdi_id_t action_id;
  std::string action_name = "pre_defined";
  if (algorithm.hash_alg == CRC_DYN && algorithm.crc_type == CRC_INVALID) {
    // user defined crc
    action_name = "user_defined";
  } else if (algorithm.hash_alg == INVALID_DYN &&
             algorithm.crc_type == CRC_INVALID) {
    LOG_TRACE("%s:%d %s : Error : received invalid config",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str());
    return TDI_UNEXPECTED;
  }
  auto status = table->actionIdGet(action_name, &action_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the action ID for action %s",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              action_name.c_str());
    return status;
  }
  std::vector<tdi_id_t> empty;
  data->actionIdSet(action_id);
  data->setActiveFields(empty);

  tdi_id_t field_id;

  // Get the seed
  status = table->dataFieldIdGet("seed", action_id, &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for seed",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = data->setValue(field_id, seed);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  // Get the msb and extend
  status = table->dataFieldIdGet("msb", action_id, &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for msb",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = data->setValue(field_id, algorithm.msb);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  status = table->dataFieldIdGet("extend", action_id, &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for extend",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = data->setValue(field_id, algorithm.extend);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  if (action_name == "pre_defined") {
    // Set the algo name
    status = table->dataFieldIdGet("algorithm_name", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for algorithm_name",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    std::string algo_str;
    if (algorithm.hash_alg == CRC_DYN) {
      char crc_name[TDI_UTILS_ALGO_NAME_LEN];
      crc_alg_type_to_str(algorithm.crc_type, crc_name);
      algo_str = crc_name;
    } else if (algorithm.hash_alg == INVALID_DYN) {
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error while getting alg type of current algo",
                  __func__,
                  __LINE__,
                  table->tableInfoGet()->nameGet().c_str());
      }
      return TDI_INVALID_ARG;
    } else {
      // algo type is RANDOM/XOR/IDENTITY
      char alg_name[TDI_UTILS_ALGO_NAME_LEN];
      hash_alg_type_to_str(algorithm.hash_alg, alg_name);
      algo_str = alg_name;
    }

    status = data->setValue(field_id, algo_str);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
  } else if (action_name == "user_defined") {
    // get hash_bit_width
    status = table->dataFieldIdGet("hash_bit_width", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for hash_bit_width",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    status = data->setValue(field_id,
                            static_cast<uint64_t>(algorithm.hash_bit_width));
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
    // get reverse
    status = table->dataFieldIdGet("reverse", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for reverse",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.reverse);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
    // get polynomial
    status = table->dataFieldIdGet("polynomial", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for polynomial",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.poly);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
    // get init
    status = table->dataFieldIdGet("init", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for init",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.init);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
    // get final_xor
    status = table->dataFieldIdGet("final_xor", action_id, &field_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for final_xor",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str());
      return status;
    }
    status = data->setValue(field_id, algorithm.final_xor);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
  }
  return status;
}

// Must be in line with json file
const std::map<std::string, bf_snapshot_ig_mode_t> snapIgMode = {
    {"INGRESS", TDI_SNAPSHOT_IGM_INGRESS},
    {"GHOST", TDI_SNAPSHOT_IGM_GHOST},
    {"ANY", TDI_SNAPSHOT_IGM_ANY},
    {"INGRESS_GHOST", TDI_SNAPSHOT_IGM_BOTH}};

// Function used to convert pipe_mgr format (which does not use "." and uses "_"
// instead) to naming used in tdi.json. Function will return input string if
// related table is not found.
const std::string getQualifiedTableName(const tdi_dev_id_t &dev_id,
                                        const std::string &p4_name,
                                        const std::string &tbl_name) {
  const Info *info;
  std::vector<const Table *> tables;
  PipelineProfInfoVec pipe_info;
  DevMgr::getInstance().tdiInfoGet(dev_id, p4_name, &info);
  info->tdiInfoPipelineInfoGet(&pipe_info);
  info->tdiInfoGetTables(&tables);
  for (auto const table : tables) {
    // For comparison convert name to pipe_mgr format. No pipe name prefix and
    // "_" instead of ".".
    std::string name;
    table->tableNameGet(&name);
    std::string name_cmp = name;
    for (auto pinfo : pipe_info) {
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
                                          "GW_TBL_INHIBIT"};
}

#endif

}  // anonymous namespace

// MatchActionTable ******************

tdi_status_t MatchActionDirect::entryAdd(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags & /*flags*/,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  const MatchActionTableData &match_data =
      static_cast<const MatchActionTableData &>(data);

  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Initialize the direct resources if they were not provided by the caller.
  auto status = resourceCheckAndInitialize<MatchActionTableData>(
      *this, match_data, false);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  if (checkDefaultOnly(this, data)) {
    LOG_TRACE("%s:%d %s Error adding action because it is defaultOnly",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();
  if (idle_table_state && idle_table_state->isIdleTableinPollMode()) {
    /* In poll mode non-zero ttl means that entry should be marked as active */
    ttl = (match_data.get_entry_hit_state() == ENTRY_ACTIVE) ? 1 : 0;
  }
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrMatEntAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      act_fn_hdl,
      pipe_action_spec,
      ttl,
      0 /* Pipe API flags */,
      &pipe_entry_hdl);
}

tdi_status_t MatchActionDirect::entryMod(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryModInternal(*this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

tdi_status_t MatchActionDirect::entryDel(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags & /*flags*/,
                                         const tdi::TableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMatEntDelByMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      0 /* Pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionDirect::clear(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/) const {
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return clearMatCommon(session, dev_tgt, true, this);
}

tdi_status_t MatchActionDirect::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionTableData &match_data =
      static_cast<const MatchActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  const auto &action_id = match_data.actionIdGet();
  if (checkTableOnly(this, action_id)) {
    LOG_TRACE("%s:%d %s Error adding action because it is tableOnly",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Check which direct resources were provided.
  bool direct_reg = false, direct_cntr = false, direct_mtr = false;
  if (action_id) {
    auto table_context_info =
        static_cast<const MatchActionDirectTableContextInfo *>(
            this->tableInfoGet()->tableContextInfoGet());
    table_context_info->actionResourcesGet(
        action_id, &direct_mtr, &direct_reg, &direct_cntr);
  } else {
    std::vector<tdi_id_t> dataFields;
    if (match_data.allFieldsSetGet()) {
      dataFields = this->tableInfoGet()->dataFieldIdListGet();
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  status);
        return status;
      }
    } else {
      dataFields.assign(match_data.activeFieldsGet().begin(),
                        match_data.activeFieldsGet().end());
    }
    for (const auto &dataFieldId : dataFields) {
      const tdi::DataFieldInfo *tableDataField =
          this->tableInfoGet()->dataFieldGet(dataFieldId, action_id);
      if (!tableDataField) {
        return TDI_OBJECT_NOT_FOUND;
      }
      auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                            tableDataField->dataFieldContextInfoGet())
                            ->typesGet();
      auto dest =
          TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
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
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  if (action_id) {
    // Initialize the direct resources if they were not provided by the
    // caller.
    status = resourceCheckAndInitialize<MatchActionTableData>(
        *this, match_data, true);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Failed to initialize direct resources",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
      return status;
    }

    pipe_action_spec = match_data.get_pipe_action_spec();
    pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

    status = pipeMgr->pipeMgrMatDefaultEntrySet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        act_fn_hdl,
        pipe_action_spec,
        0 /* Pipe API flags */,
        &entry_hdl);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting default entry, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else {
    // Get the handle of the default entry.
    status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        &entry_hdl);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error getting entry handle, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }

  // Program direct counters if requested.
  if (direct_cntr) {
    const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                            .getCounterSpecObj()
                                            .getPipeCounterSpec();
    status = pipeMgr->pipeMgrMatEntDirectStatSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        entry_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct stats, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }
  // Program direct meter/wred/lpf and registers only if we did not do a set
  // above.
  if (!action_id && (direct_reg || direct_mtr)) {
    pipe_action_spec = match_data.get_pipe_action_spec();
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        entry_hdl,
        const_cast<pipe_res_spec_t *>(pipe_action_spec->resources),
        pipe_action_spec->resource_count,
        0 /* Pipe API flags */);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct resources, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }
  return status;
}

tdi_status_t MatchActionDirect::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0);
}

tdi_status_t MatchActionDirect::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      true /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

tdi_status_t MatchActionDirect::defaultEntryGet(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_entry_hdl);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              pipe_dev_tgt.dev_pipe_id,
              status);
    return status;
  }
  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

tdi_status_t MatchActionDirect::entryKeyGet(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags & /*flags*/,
                                            const tdi_handle_t &entry_handle,
                                            tdi::Target *entry_tgt,
                                            tdi::TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TNA_TARGET_PIPE_ID),
                      entry_pipe);
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionDirect::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      entry_handle,
      false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionDirect::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi_handle_t &entry_handle,
                                         tdi::TableKey *key,
                                         tdi::TableData *data) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = entryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionDirect::entryGetFirst(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags &flags,
                                              tdi::TableKey *key,
                                              tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  tdi_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      &first_entry_handle);

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = entryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          this->tdiInfoGet()->p4NameGet());
  auto nextRef =
      device_state->nextRefState.getObjState(this->tableInfoGet()->idGet());
  nextRef->setRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                      TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                  pipe_dev_tgt.dev_pipe_id,
                  first_entry_handle);
  return status;
}

tdi_status_t MatchActionDirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      true /* light_pipe_validation */);
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == TDI_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());

    const tdi::Device *device;
    status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                pipe_dev_tgt.device_id,
                status);
      return TDI_OBJECT_NOT_FOUND;
    }

    auto device_state =
        static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
            this->tdiInfoGet()->p4NameGet());
    auto nextRef =
        device_state->nextRefState.getObjState(this->tableInfoGet()->idGet());
    status = nextRef->getRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                                 TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                             pipe_dev_tgt.dev_pipe_id,
                             &pipe_entry_hdl);
  }

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  std::vector<int> next_entry_handles(n, 0);

  status = pipeMgr->pipeMgrGetNextEntryHandles(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      pipe_entry_hdl,
      n,
      next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }
  pipe_tbl_match_spec_t match_spec;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
    auto this_key = static_cast<MatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    if (next_entry_handles[i] == -1) {
      break;
    }
    this_key->populate_match_spec(&match_spec);
    status = entryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(match_spec.priority);
    this_key->setPartitionIndex(match_spec.partition_index);
  }
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          this->tdiInfoGet()->p4NameGet());
  auto nextRef =
      device_state->nextRefState.getObjState(this->tableInfoGet()->idGet());
  // Even if entryGet failed, since pipe_mgr can fetch next n handles
  // starting from non-existing handle, just store last one that was used.
  nextRef->setRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                      TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                  pipe_dev_tgt.dev_pipe_id,
                  next_entry_handles[i - 1]);

  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::sizeGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        size_t *count) const {
  tdi_status_t status = TDI_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->tableInfoGet()->sizeGet() - reserved;
  return status;
}

tdi_status_t MatchActionDirect::usageGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t MatchActionDirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MatchActionKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t MatchActionDirect::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
tdi_status_t MatchActionDirect::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t MatchActionDirect::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

tdi_status_t dataReset_internal(const tdi::Table & /*table*/,
                                const tdi_id_t & /*action_id*/,
                                const std::vector<tdi_id_t> & /*fields*/,
                                tdi::TableData * /*data*/) {
#if 0
  tdi::TableData *data_obj = static_cast<tdi::TableData *>(data);
  if (!table.validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return data_obj->reset(action_id, fields);
#endif
  // TODO(sayanb)
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::keyReset(TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  return key_reset<MatchActionDirect, MatchActionKey>(*this, match_key);
}

tdi_status_t MatchActionDirect::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const tdi_id_t &action_id,
                                          tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const std::vector<tdi_id_t> &fields,
                                          tdi::TableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const std::vector<tdi_id_t> &fields,
                                          const tdi_id_t &action_id,
                                          tdi::TableData *data) const {
  return dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t MatchActionDirect::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

#if 0
tdi_status_t MatchActionDirect::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  switch (type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE:
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}
#endif

tdi_status_t MatchActionDirect::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableAttributes &tableAttributes) const {
  return commonAttributesSet<MatchActionDirect>(
      this, session, dev_tgt, tableAttributes);
}

tdi_status_t MatchActionDirect::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableAttributes *tableAttributes) const {
  return commonAttributesGet<MatchActionDirect>(
      this, session, dev_tgt, tableAttributes);
}

#if 0
tdi_status_t MatchActionDirect::registerMatUpdateCb(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const pipe_mat_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterMatUpdateCb(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}
#endif

tdi_status_t MatchActionDirect::dataAllocate_internal(
    tdi_id_t action_id,
    std::unique_ptr<tdi::TableData> *data_ret,
    const std::vector<tdi_id_t> &fields) const {
  if (action_id && tableInfoGet()->actionGet(action_id) == nullptr) {
    LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const MatchActionTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  bool stful_fetched = false;

  tdi_id_t ttl_field_id = 0;
  tdi_id_t hs_field_id = 0;

  uint32_t res_get_flags = 0;
  pipe_res_get_data_t res_data;
  res_data.stful.data = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  bool all_fields_set = false;

  MatchActionTableData *match_data = static_cast<MatchActionTableData *>(data);
  std::vector<tdi_id_t> dataFields;
  const auto &req_action_id = match_data->actionIdGet();

  all_fields_set = match_data->allFieldsSetGet();
  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
    // do not assign dataFields in this case because we might not know
    // what the actual_action_id is yet
    // Based upon the actual action id, we will be filling in the data fields
    // later anyway
  } else {
    dataFields.assign(match_data->activeFieldsGet().begin(),
                      match_data->activeFieldsGet().end());
    for (const auto &dataFieldId : dataFields) {
      const tdi::DataFieldInfo *tableDataField =
          this->tableInfoGet()->dataFieldGet(dataFieldId, req_action_id);
      if (!tableDataField) {
        return TDI_OBJECT_NOT_FOUND;
      }
      auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                            tableDataField->dataFieldContextInfoGet())
                            ->typesGet();
      auto dest =
          TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
      switch (dest) {
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
  match_data->TableData::reset(0);
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         table_context_info->tableHdlGet(),
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return status;
  }

  auto &action_id = table_context_info->actFnHdlToIdGet().at(pipe_act_fn_hdl);
  if (req_action_id && req_action_id != action_id) {
    // Keeping this log as warning for iteration purposes.
    // Caller can decide to throw an error if required
    LOG_TRACE("%s:%d %s ERROR expecting action ID to be %d but recvd %d ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              req_action_id,
              action_id);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return TDI_INVALID_ARG;
  }

  match_data->actionIdSet(action_id);
  // Get the list of dataFields for action_id. The list of active fields needs
  // to be set
  if (all_fields_set) {
    dataFields = tableInfoGet()->dataFieldIdListGet(action_id);
    if (dataFields.empty()) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                status);
      // Must free stful related memory
      if (res_data.stful.data != nullptr) {
        bf_sys_free(res_data.stful.data);
      }
      return status;
    }
    match_data->activeFieldsSet({});
  } else {
    // dataFields has already been populated
    // with the correct fields since the requested action and actual
    // action have also been verified. Only active fields need to be
    // corrected because all fields must have been set now
    match_data->activeFieldsSet(dataFields);
  }

  for (const auto &dataFieldId : dataFields) {
    const tdi::DataFieldInfo *tableDataField =
        this->tableInfoGet()->dataFieldGet(dataFieldId, action_id);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                          tableDataField->dataFieldContextInfoGet())
                          ->typesGet();
    auto dest = TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
    switch (dest) {
      case fieldDestination::DIRECT_LPF:
        if (res_data.has_lpf) {
          match_data->getPipeActionSpecObj().setValueLPFSpec(res_data.mtr.lpf);
        }
        break;
      case fieldDestination::DIRECT_WRED:
        if (res_data.has_red) {
          match_data->getPipeActionSpecObj().setValueWREDSpec(res_data.mtr.red);
        }
        break;
      case fieldDestination::DIRECT_METER:
        if (res_data.has_meter) {
          match_data->getPipeActionSpecObj().setValueMeterSpec(
              res_data.mtr.meter);
        }
        break;
      case fieldDestination::DIRECT_COUNTER:
        if (res_data.has_counter) {
          match_data->getPipeActionSpecObj().setValueCounterSpec(
              res_data.counter);
        }
        break;
      case fieldDestination::DIRECT_REGISTER:
        if (res_data.has_stful && !stful_fetched) {
          std::vector<pipe_stful_mem_spec_t> register_pipe_data(
              res_data.stful.data,
              res_data.stful.data + res_data.stful.pipe_count);
          match_data->getPipeActionSpecObj().setValueRegisterSpec(
              register_pipe_data);
          stful_fetched = true;
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
        // We have already processed this. Ignore
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dataFieldId);
        if (res_data.stful.data != nullptr) {
          bf_sys_free(res_data.stful.data);
        }
        return TDI_NOT_SUPPORTED;
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
    match_data->removeActiveField(ttl_field_id);
  }
  if (!res_data.has_hit_state) {
    match_data->removeActiveField(hs_field_id);
  }
  return TDI_SUCCESS;
}

// MatchActionIndirect **************
namespace {
const std::vector<DataFieldType> indirectResourceDataFields = {
    DataFieldType::COUNTER_INDEX,
    DataFieldType::METER_INDEX,
    DataFieldType::REGISTER_INDEX};
}

void MatchActionIndirect::populate_indirect_resources(
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

tdi_status_t MatchActionIndirect::entryAdd(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  const MatchActionIndirectTableData &match_data =
      static_cast<const MatchActionIndirectTableData &>(data);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Check if all indirect resource indices are supplied or not.
  // Entry Add mandates that all indirect indices be given
  // Initialize the direct resources if applicable
  status = resourceCheckAndInitialize<MatchActionIndirectTableData>(
      *this, match_data, false);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  pipe_action_spec_t pipe_action_spec = {0};
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;

  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  match_key.populate_match_spec(&pipe_match_spec);

  uint32_t ttl = match_data.get_ttl();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  // Fill in state from action member id or selector group id in the action spec
  pipe_mgr_adt_ent_data_t ap_ent_data;
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &ap_ent_data);

  if (status != TDI_SUCCESS) {
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
      if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not have any members in the group table associated with the "
            "table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a action member id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getActionMbrId());
        return TDI_OBJECT_NOT_FOUND;
      }
    }
    return TDI_UNEXPECTED;
  }

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, &pipe_action_spec);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Ready to add the entry
  return pipeMgr->pipeMgrMatEntAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      act_fn_hdl,
      &pipe_action_spec,
      ttl,
      0 /* Pipe API flags */,
      &pipe_entry_hdl);
}

tdi_status_t MatchActionIndirect::entryMod(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryModInternal(*this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

tdi_status_t MatchActionIndirect::entryDel(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrMatEntDelByMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      0 /* Pipe api flags */);
}

tdi_status_t MatchActionIndirect::clear(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/) const {
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return clearMatCommon(session, dev_tgt, true, this);
}

tdi_status_t MatchActionIndirect::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi::TableKey &key,
                                           tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      true /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

tdi_status_t MatchActionIndirect::entryKeyGet(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags & /*flags*/,
                                              const tdi_handle_t &entry_handle,
                                              tdi::Target *entry_tgt,
                                              tdi::TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TNA_TARGET_PIPE_ID),
                      entry_pipe);
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionIndirect::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      entry_handle,
      false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionIndirect::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi_handle_t &entry_handle,
                                           tdi::TableKey *key,
                                           tdi::TableData *data) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = entryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionIndirect::entryGetFirst(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                tdi::TableKey *key,
                                                tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  tdi_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      &first_entry_handle);

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = entryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);

  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          this->tdiInfoGet()->p4NameGet());
  auto nextRef =
      device_state->nextRefState.getObjState(this->tableInfoGet()->idGet());
  nextRef->setRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                      TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                  pipe_dev_tgt.dev_pipe_id,
                  first_entry_handle);

  return status;
}

tdi_status_t MatchActionIndirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl,
      true /* light_pipe_validation */);
  // Store ref point for GetNext_n to follow.
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto device_state =
      static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
          this->tdiInfoGet()->p4NameGet());
  auto nextRef =
      device_state->nextRefState.getObjState(this->tableInfoGet()->idGet());
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == TDI_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
    status = nextRef->getRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                                 TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                             pipe_dev_tgt.dev_pipe_id,
                             &pipe_entry_hdl);
  }

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  std::vector<int> next_entry_handles(n, 0);

  status = pipeMgr->pipeMgrGetNextEntryHandles(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      pipe_entry_hdl,
      n,
      next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }
  pipe_tbl_match_spec_t match_spec;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
    auto this_key = static_cast<MatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    tdi_id_t table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    if (next_entry_handles[i] == -1) {
      break;
    }
    this_key->populate_match_spec(&match_spec);
    status = entryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(match_spec.priority);
    this_key->setPartitionIndex(match_spec.partition_index);
  }
  // Even if entryGet failed, since pipe_mgr can fetch next n handles
  // starting from non-existing handle, just store last one that was used.
  nextRef->setRef(session.handleGet(static_cast<tdi_mgr_type_e>(
                      TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                  pipe_dev_tgt.dev_pipe_id,
                  next_entry_handles[i - 1]);

  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirect::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionIndirectTableData &match_data =
      static_cast<const MatchActionIndirectTableData &>(data);
  pipe_action_spec_t pipe_action_spec = {0};

  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;

  if (this->tableInfoGet()->hasConstDefaultAction()) {
    LOG_TRACE(
        "%s:%d %s Cannot set Default action because the table has a const "
        "default action",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  pipe_mgr_adt_ent_data_t ap_ent_data;
  // Fill in state from action member id or selector group id in the action spec
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &ap_ent_data);

  if (status != TDI_SUCCESS) {
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
      if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table associated with the table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a action member "
            "id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getActionMbrId());
        return TDI_OBJECT_NOT_FOUND;
      }
    }
    return TDI_UNEXPECTED;
  }

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, &pipe_action_spec);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrMatDefaultEntrySet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      act_fn_hdl,
      &pipe_action_spec,
      0 /* Pipe API flags */,
      &pipe_entry_hdl);
}

tdi_status_t MatchActionIndirect::defaultEntryGet(const tdi::Session &session,
                                                  const tdi::Target &dev_tgt,
                                                  const tdi::Flags &flags,
                                                  tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_entry_hdl);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              pipe_dev_tgt.dev_pipe_id,
              status);
    return status;
  }
  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

tdi_status_t MatchActionIndirect::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  if (this->tableInfoGet()->hasConstDefaultAction()) {
    // If default action is const, then this API is a no-op
    LOG_DBG(
        "%s:%d %s Calling reset on a table with const "
        "default action",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0);
}

tdi_status_t MatchActionIndirect::getActionState(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const MatchActionIndirectTableData *data,
    pipe_adt_ent_hdl_t *adt_entry_hdl,
    pipe_sel_grp_hdl_t *sel_grp_hdl,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto table_context_info =
      static_cast<const MatchActionIndirectTableContextInfo *>(
          this->tableInfoGet()->tableContextInfoGet());
  auto actTbl =
      static_cast<const ActionProfile *>(table_context_info->actProfGet());
  if (!data->isGroup()) {
    // Safe to do a static cast here since all table objects are constructed by
    // our factory and the right kind of sub-object is constructed by the
    // factory depending on the table type. Here the actProfTbl member of the
    // table object is a pointer to the action profile table associated with the
    // match-action table. It is guaranteed to be  of type ActionProfile

    status = actTbl->getMbrState(session,
                                 dev_tgt,
                                 data->getActionMbrId(),
                                 act_fn_hdl,
                                 adt_entry_hdl,
                                 ap_ent_data);
    if (status != TDI_SUCCESS) {
      *adt_entry_hdl = MatchActionIndirectTableData::invalid_action_entry_hdl;
      return status;
    }
  } else {
    auto selTbl =
        static_cast<const Selector *>(table_context_info->selectorGet());
    status =
        selTbl->getGrpHdl(session, dev_tgt, data->getGroupId(), sel_grp_hdl);
    if (status != TDI_SUCCESS) {
      *sel_grp_hdl = MatchActionIndirectTableData::invalid_group;
      return TDI_OBJECT_NOT_FOUND;
    }
    dev_target_t pipe_dev_tgt;
    auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
    tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
    status = selTbl->getOneMbr(
        session, pipe_dev_tgt.device_id, *sel_grp_hdl, adt_entry_hdl);
    if (status != TDI_SUCCESS) {
      *adt_entry_hdl = MatchActionIndirectTableData::invalid_action_entry_hdl;
      return TDI_OBJECT_NOT_FOUND;
    }

    tdi_id_t a_member_id = 0;
    status =
        getActionMbrIdFromHndl(session, dev_tgt, *adt_entry_hdl, &a_member_id);
    if (status != TDI_SUCCESS) {
      return TDI_OBJECT_NOT_FOUND;
    }

    status = actTbl->getMbrState(
        session, dev_tgt, a_member_id, act_fn_hdl, adt_entry_hdl, ap_ent_data);
  }
  return status;
}

tdi_status_t MatchActionIndirect::getActionMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    tdi_id_t *mbr_id) const {
  auto table_context_info =
      static_cast<const MatchActionIndirectTableContextInfo *>(
          this->tableInfoGet()->tableContextInfoGet());
  auto actTbl =
      static_cast<const ActionProfile *>(table_context_info->actProfGet());
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, mbr_id);
}

tdi_status_t MatchActionIndirect::getGroupIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_sel_grp_hdl_t sel_grp_hdl,
    tdi_id_t *grp_id) const {
  auto table_context_info =
      static_cast<const MatchActionIndirectTableContextInfo *>(
          this->tableInfoGet()->tableContextInfoGet());
  auto selTbl =
      static_cast<const Selector *>(table_context_info->selectorGet());
  return selTbl->getGrpIdFromHndl(session, dev_tgt, sel_grp_hdl, grp_id);
}

tdi_status_t MatchActionIndirect::sizeGet(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags & /*flags*/,
                                          size_t *count) const {
  tdi_status_t status = TDI_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->tableInfoGet()->sizeGet() - reserved;
  return status;
}

tdi_status_t MatchActionIndirect::usageGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t MatchActionIndirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MatchActionKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirect::keyReset(TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  return key_reset<MatchActionIndirect, MatchActionKey>(*this, match_key);
}

tdi_status_t MatchActionIndirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirect::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirect::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionIndirect::dataReset(const std::vector<tdi_id_t> &fields,
                                            tdi::TableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionIndirect::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

#if 0
tdi_status_t MatchActionIndirect::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  switch (type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE:
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t MatchActionIndirect::attributeReset(
    const TableAttributesType &type,
    const tdi_tofino_attributes_idle_table_mode_e &idle_table_mode,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  switch (type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE:
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}
#endif

tdi_status_t MatchActionIndirect::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableAttributes &tableAttributes) const {
  return commonAttributesSet<MatchActionIndirect>(
      this, session, dev_tgt, tableAttributes);
}

tdi_status_t MatchActionIndirect::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableAttributes *tableAttributes) const {
  return commonAttributesGet<MatchActionIndirect>(
      this, session, dev_tgt, tableAttributes);
}

// Unexposed functions
tdi_status_t MatchActionIndirect::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_res_get_data_t res_data;

  bool stful_fetched = false;
  tdi_id_t ttl_field_id = 0;
  tdi_id_t hs_field_id = 0;
  uint32_t res_get_flags = 0;
  res_data.stful.data = nullptr;

  MatchActionIndirectTableData *match_data =
      static_cast<MatchActionIndirectTableData *>(data);
  std::vector<tdi_id_t> dataFields;
  bool all_fields_set = match_data->allFieldsSetGet();

  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
  } else {
    dataFields.assign(match_data->activeFieldsGet().begin(),
                      match_data->activeFieldsGet().end());
    for (const auto &dataFieldId : match_data->activeFieldsGet()) {
      const tdi::DataFieldInfo *tableDataField =
          this->tableInfoGet()->dataFieldGet(dataFieldId);
      if (!tableDataField) {
        return TDI_OBJECT_NOT_FOUND;
      }
      auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                            tableDataField->dataFieldContextInfoGet())
                            ->typesGet();
      auto dest =
          TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
      switch (dest) {
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
  match_data->TableData::reset();
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         table_context_info->tableHdlGet(),
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handle %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return status;
  }

  match_data->actionIdSet(0);
  // There is no direct action in indirect flow, hence always fill in
  // only requested fields.
  if (all_fields_set) {
    dataFields = tableInfoGet()->dataFieldIdListGet();
    if (dataFields.empty()) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                status);
      // Must free stful related memory
      if (res_data.stful.data != nullptr) {
        bf_sys_free(res_data.stful.data);
      }
      return status;
    }
    match_data->activeFieldsSet({});
  } else {
    // dataFields has already been populated
    // with the correct fields since the requested action and actual
    // action have also been verified. Only active fields need to be
    // corrected because all fields must have been set now
    match_data->activeFieldsSet(dataFields);
  }

  for (const auto &dataFieldId : dataFields) {
    const tdi::DataFieldInfo *tableDataField =
        this->tableInfoGet()->dataFieldGet(dataFieldId);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    auto fieldTypes = static_cast<const TofinoDataFieldContextInfo *>(
                          tableDataField->dataFieldContextInfoGet())
                          ->typesGet();
    auto dest = TofinoDataFieldContextInfo::getDataFieldDestination(fieldTypes);
    std::set<tdi_id_t> oneof_siblings = tableDataField->oneofSiblingsGet();

    switch (dest) {
      case fieldDestination::DIRECT_METER:
        if (res_data.has_meter) {
          match_data->getPipeActionSpecObj().setValueMeterSpec(
              res_data.mtr.meter);
        }
        break;
      case fieldDestination::DIRECT_LPF:
        if (res_data.has_lpf) {
          match_data->getPipeActionSpecObj().setValueLPFSpec(res_data.mtr.lpf);
        }
        break;
      case fieldDestination::DIRECT_WRED:
        if (res_data.has_red) {
          match_data->getPipeActionSpecObj().setValueWREDSpec(res_data.mtr.red);
        }
        break;
      case fieldDestination::DIRECT_COUNTER:
        if (res_data.has_counter) {
          match_data->getPipeActionSpecObj().setValueCounterSpec(
              res_data.counter);
        }
        break;
      case fieldDestination::DIRECT_REGISTER:
        if (res_data.has_stful && !stful_fetched) {
          std::vector<pipe_stful_mem_spec_t> register_pipe_data(
              res_data.stful.data,
              res_data.stful.data + res_data.stful.pipe_count);
          match_data->getPipeActionSpecObj().setValueRegisterSpec(
              register_pipe_data);
          stful_fetched = true;
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
        // If its the action member ID or the selector group ID, populate the
        // right member of the data object
        if (fieldTypes.find(DataFieldType::ACTION_MEMBER_ID) !=
            fieldTypes.end()) {
          if (IS_ACTION_SPEC_ACT_DATA_HDL(pipe_action_spec)) {
            tdi_id_t act_mbr_id;
            status = this->getActionMbrIdFromHndl(
                session, dev_tgt, pipe_action_spec->adt_ent_hdl, &act_mbr_id);
            // Default entries will not have an action member handle if they
            // were installed automatically.  In this case return a member id
            // of zero.
            if (status == TDI_OBJECT_NOT_FOUND &&
                !pipe_match_spec &&  // Default entries won't have a match
                                     // spec
                pipe_action_spec->adt_ent_hdl == 0) {
              status = TDI_SUCCESS;
              act_mbr_id = 0;
            }
            TDI_ASSERT(status == TDI_SUCCESS);
            match_data->setActionMbrId(act_mbr_id);
            // Remove oneof sibling from active fields
            for (const auto &sib : oneof_siblings)
              match_data->removeActiveField(sib);
          }
        } else if (fieldTypes.find(DataFieldType::SELECTOR_GROUP_ID) !=
                   fieldTypes.end()) {
          if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
            tdi_id_t sel_grp_id;
            status = getGroupIdFromHndl(
                session, dev_tgt, pipe_action_spec->sel_grp_hdl, &sel_grp_id);
            TDI_ASSERT(status == TDI_SUCCESS);
            match_data->setGroupId(sel_grp_id);
            // Remove oneof sibling from active fields
            for (const auto &sib : oneof_siblings)
              match_data->removeActiveField(sib);
          }
        } else {
          TDI_ASSERT(0);
        }
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dataFieldId);
        if (res_data.stful.data != nullptr) {
          bf_sys_free(res_data.stful.data);
        }
        return TDI_NOT_SUPPORTED;
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
    match_data->removeActiveField({ttl_field_id});
  }
  if (!res_data.has_hit_state) {
    match_data->removeActiveField({hs_field_id});
  }
  return TDI_SUCCESS;
}

// ActionProfile

tdi_status_t ActionProfile::entryAdd(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/,
                                     const tdi::TableKey &key,
                                     const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);
  const ActionProfileData &action_tbl_data =
      static_cast<const ActionProfileData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_adt_ent_hdl_t adt_entry_hdl;
  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrAdtEntAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      act_fn_hdl,
      mbr_id,
      pipe_action_spec,
      &adt_entry_hdl,
      pipe_flags);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Error adding new ADT %d entry with mbr_id %d.",
              __func__,
              __LINE__,
              tableInfoGet()->idGet(),
              mbr_id);
  }
  return status;
}

tdi_status_t ActionProfile::entryMod(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/,
                                     const tdi::TableKey &key,
                                     const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);
  const ActionProfileData &action_tbl_data =
      static_cast<const ActionProfileData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();

  // Action entry handle doesn't change during a modify
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      adt_ent_hdl,
      act_fn_hdl,
      pipe_action_spec,
      0 /* Pipe API flags */);

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in modifying action profile member for tbl id %d "
        "member id %d, err %s",
        __func__,
        __LINE__,
        tableInfoGet()->idGet(),
        mbr_id,
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

tdi_status_t ActionProfile::entryDel(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/,
                                     const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntDel(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      adt_ent_hdl,
      0 /* Pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in deletion of action profile member %d for tbl id %d "
        ", err %s",
        __func__,
        __LINE__,
        mbr_id,
        tableInfoGet()->idGet(),
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

tdi_status_t ActionProfile::clear(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags & /*flags*/) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  while (TDI_SUCCESS == pipeMgr->pipeMgrGetFirstEntryHandle(
                            session.handleGet(static_cast<tdi_mgr_type_e>(
                                TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                            table_context_info->tableHdlGet(),
                            pipe_dev_tgt,
                            (int *)&adt_ent_hdl)) {
    status = pipeMgr->pipeMgrAdtEntDel(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        adt_ent_hdl,
        0 /* Pipe api flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d Error deleting action profile member for tbl id 0x%x "
          "handle %d, err %s",
          __func__,
          __LINE__,
          tableInfoGet()->idGet(),
          adt_ent_hdl,
          pipe_str_err((pipe_status_t)status));
      return status;
    }
  }
  return status;
}

tdi_status_t ActionProfile::entryGet(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);
  ActionProfileData *action_tbl_data = static_cast<ActionProfileData *>(data);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  pipe_adt_ent_hdl_t adt_ent_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Action member Id %d does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              mbr_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  return entryGet_internal(
      session, dev_tgt, flags, adt_ent_hdl, action_tbl_data);
}

tdi_status_t ActionProfile::entryKeyGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        const tdi_handle_t &entry_handle,
                                        tdi::Target *entry_tgt,
                                        tdi::TableKey *key) const {
  ActionProfileKey *action_tbl_key = static_cast<ActionProfileKey *>(key);
  tdi_id_t mbr_id;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntMbrIdGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      entry_handle,
      &mbr_id,
      &pipe_dev_tgt.dev_pipe_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  action_tbl_key->setMemberId(mbr_id);
  *entry_tgt = dev_tgt;
  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TNA_TARGET_PIPE_ID),
                      pipe_dev_tgt.dev_pipe_id);
  return status;
}

tdi_status_t ActionProfile::entryHandleGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           tdi_handle_t *entry_handle) const {
  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);
  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t ActionProfile::entryGet(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi_handle_t &entry_handle,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const {
  // Doesn't matter what we are initializing with. It will be
  // set to the right thing
  tdi::tna::tofino::Target entry_tgt(
      0, TNA_DEV_PIPE_ALL, TNA_DIRECTION_INGRESS, 0);
  tdi_status_t status =
      this->entryKeyGet(session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != TDI_SUCCESS) {
    return status;
  }
  return this->entryGet(session, entry_tgt, flags, *key, data);
}

tdi_status_t ActionProfile::entryGetFirst(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          tdi::TableKey *key,
                                          tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  ActionProfileKey *action_tbl_key = static_cast<ActionProfileKey *>(key);
  ActionProfileData *action_tbl_data = static_cast<ActionProfileData *>(data);

  tdi_id_t first_mbr_id;
  int first_entry_hdl;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      &first_entry_hdl);

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  status = pipeMgr->pipeMgrAdtEntMbrIdGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      first_entry_hdl,
      &first_mbr_id,
      &pipe_dev_tgt.dev_pipe_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first entry member id, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  tdi::tna::tofino::Target ent_dev_tgt(
      pipe_dev_tgt.device_id, pipe_dev_tgt.dev_pipe_id, TNA_DIRECTION_ALL, 0);

  status = entryGet_internal(
      session, ent_dev_tgt, flags, first_entry_hdl, action_tbl_data);
  if (status != TDI_SUCCESS) {
    return status;
  }
  action_tbl_key->setMemberId(first_mbr_id);
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const ActionProfileKey &action_tbl_key =
      static_cast<const ActionProfileKey &>(key);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  tdi_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      pipe_entry_hdl,
      n,
      next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    tdi_id_t next_mbr_id;
    bf_dev_pipe_t next_mbr_pipe;
    // Get the action entry handle used by pipe-mgr from the member id
    status = pipeMgr->pipeMgrAdtEntMbrIdGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        next_entry_handles[i],
        &next_mbr_id,
        &next_mbr_pipe);
    if (status) break;
    auto this_key = static_cast<ActionProfileKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<ActionProfileData *>((*key_data_pairs)[i].second);
    const tdi::Table *table_from_data;
    this_data->getParent(&table_from_data);
    auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    tdi::tna::tofino::Target mbr_dev_tgt(
        pipe_dev_tgt.device_id, next_mbr_pipe, TNA_DIRECTION_ALL, 0);
    status = entryGet_internal(
        session, mbr_dev_tgt, flags, next_entry_handles[i], this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with entry "
          "handle %d, mbr id %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
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
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::usageGet(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t ActionProfile::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new ActionProfileKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new ActionProfileData(this, action_id));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // This dataAllocate is mainly used for entry gets from the action table
  // wherein  the action id of the entry is not known and will be filled in by
  // the entry get
  *data_ret = std::unique_ptr<tdi::TableData>(new ActionProfileData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::getMbrState(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    tdi_id_t mbr_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto status = pipeMgr->pipeMgrAdtEntDataGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      adt_ent_hdl,
      ap_ent_data);
  *act_fn_hdl = ap_ent_data->act_fn_hdl;

  return status;
}

tdi_status_t ActionProfile::getMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    tdi_id_t *mbr_id) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrAdtEntMbrIdGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      adt_ent_hdl,
      mbr_id,
      &pipe_dev_tgt.dev_pipe_id);
}

tdi_status_t ActionProfile::getHdlFromMbrId(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      mbr_id,
      adt_ent_hdl);
}

tdi_status_t ActionProfile::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_adt_ent_hdl_t &entry_hdl,
    ActionProfileData *action_tbl_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  pipe_action_spec_t *action_spec = action_tbl_data->mutable_pipe_action_spec();
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  auto table_context_info = static_cast<const ActionProfileContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_act_fn_hdl_t act_fn_hdl;
  tdi_status_t status =
      pipeMgr->pipeMgrGetActionDataEntry(table_context_info->tableHdlGet(),
                                         pipe_dev_tgt,
                                         entry_hdl,
                                         &action_spec->act_data,
                                         &act_fn_hdl,
                                         read_from_hw);
  // At this point, if a member wasn't found, there is a high chance
  // that the action data wasn't programmed in the hw itself because by
  // this time TDI sw state check has passed. So try it once again with
  // with read_from_hw = False
  if (status == TDI_OBJECT_NOT_FOUND && read_from_hw) {
    status =
        pipeMgr->pipeMgrGetActionDataEntry(table_context_info->tableHdlGet(),
                                           pipe_dev_tgt,
                                           entry_hdl,
                                           &action_spec->act_data,
                                           &act_fn_hdl,
                                           false);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting action data from pipe-mgr, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  tdi_id_t action_id = table_context_info->actFnHdlToIdGet().at(act_fn_hdl);

  action_tbl_data->actionIdSet(action_id);
  return TDI_SUCCESS;
}

tdi_status_t ActionProfile::keyReset(TableKey *key) const {
  ActionProfileKey *action_key = static_cast<ActionProfileKey *>(key);
  return key_reset<ActionProfile, ActionProfileKey>(*this, action_key);
}

#if 0
tdi_status_t ActionProfile::registerAdtUpdateCb(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const tdi::Flags & /*flags*/,
                                                 const pipe_adt_update_cb &cb,
                                                 const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterAdtUpdateCb(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}
#endif

// Selector **************
tdi_status_t Selector::getActMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t &adt_ent_hdl,
    tdi_id_t *act_mbr_id) const {
  auto selector_context_info = static_cast<const SelectorTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  ActionProfile *actTbl =
      static_cast<ActionProfile *>(selector_context_info->actProfTbl_);
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, act_mbr_id);
  return TDI_SUCCESS;
}

tdi_status_t Selector::entryAdd(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags & /*flags*/,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key = static_cast<const SelectorTableKey &>(key);
  const SelectorTableData &sel_data =
      static_cast<const SelectorTableData &>(data);
  // Make a call to pipe-mgr to first create the group
  pipe_sel_grp_hdl_t sel_grp_hdl;
  uint32_t max_grp_size = sel_data.get_max_grp_size();
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  std::vector<tdi_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();

  if (members.size() != member_status.size()) {
    LOG_TRACE("%s:%d MemberId size %zu and member status size %zu do not match",
              __func__,
              __LINE__,
              members.size(),
              member_status.size());
    return TDI_INVALID_ARG;
  }

  if (members.size() > max_grp_size) {
    LOG_TRACE(
        "%s:%d %s Number of members provided %zd exceeds the maximum group "
        "size %d for group id %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        members.size(),
        max_grp_size,
        sel_grp_id);
    return TDI_INVALID_ARG;
  }

  // Before we add the group, we first check the validity of the members to be
  // added if any and build up a vector of action entry handles and action
  // function handles to be used to pass to the pipe-mgr API
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  auto selector_context_info = static_cast<const SelectorTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  for (unsigned i = 0; i < members.size(); ++i) {
    // For each member verify if the member ID specified exists. If so, get the
    // action function handle
    pipe_adt_ent_hdl_t adt_ent_hdl = 0;

    ActionProfile *actTbl =
        static_cast<ActionProfile *>(selector_context_info->actProfTbl_);
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which does not exist into "
          "group id %d on pipe %x",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          members[i],
          sel_grp_id,
          pipe_dev_tgt.dev_pipe_id);
      return TDI_INVALID_ARG;
    }

    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  // Now, attempt to add the group;
  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrSelGrpAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      selector_context_info->tableHdlGet(),
      sel_grp_id,
      max_grp_size,
      &sel_grp_hdl,
      pipe_flags);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding group id %d pipe %x, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              sel_grp_id,
              pipe_dev_tgt.dev_pipe_id,
              status);
    return status;
  }

  // Set the membership of the group
  status = pipeMgr->pipeMgrSelGrpMbrsSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      selector_context_info->tableHdlGet(),
      sel_grp_hdl,
      members.size(),
      action_entry_hdls.data(),
      (bool *)(pipe_member_status.data()),
      0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in setting membership for group id %d pipe %x, err "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        sel_grp_id,
        pipe_dev_tgt.dev_pipe_id,
        status);
    pipeMgr->pipeMgrSelGrpDel(session.handleGet(static_cast<tdi_mgr_type_e>(
                                  TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                              pipe_dev_tgt.device_id,
                              selector_context_info->tableHdlGet(),
                              sel_grp_hdl,
                              0 /* Pipe API flags */);
  }

  return status;
}

tdi_status_t Selector::entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags & /*flags*/,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key = static_cast<const SelectorTableKey &>(key);
  const SelectorTableData &sel_data =
      static_cast<const SelectorTableData &>(data);

  std::vector<tdi_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  // Get the mapping from selector group id to selector group handle

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto selector_context_info = static_cast<const SelectorTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  status = pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      selector_context_info->tableHdlGet(),
      sel_grp_id,
      &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  uint32_t curr_size;
  status = pipeMgr->pipeMgrSelGrpMaxSizeGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      selector_context_info->tableHdlGet(),
      sel_grp_hdl,
      &curr_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting max grp size, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  // Next, validate the member IDs
  ActionProfile *actTbl =
      static_cast<ActionProfile *>(selector_context_info->actProfTbl_);
  for (unsigned i = 0; i < members.size(); ++i) {
    pipe_adt_ent_hdl_t adt_ent_hdl;
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which not exist into group "
          "id %d pipe %x",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          members[i],
          sel_grp_id,
          pipe_dev_tgt.dev_pipe_id);
      return TDI_INVALID_ARG;
    }
    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }
  bool membrs_set = false;
  // If new members will fit current size, set members first to support
  // downsizing of the group.
  if (curr_size >= members.size()) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        selector_context_info->tableHdlGet(),
        sel_grp_hdl,
        members.size(),
        action_entry_hdls.data(),
        (bool *)(pipe_member_status.data()),
        0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          pipe_dev_tgt.dev_pipe_id,
          status);
      return status;
    }
    membrs_set = true;
  }
  const auto max_grp_size = sel_data.get_max_grp_size();
  // Size of 0 is ignored, means no change in size.
  if (max_grp_size != 0 && curr_size != max_grp_size) {
    status = pipeMgr->pipeMgrSelGrpSizeSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        selector_context_info->tableHdlGet(),
        sel_grp_hdl,
        max_grp_size);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting group size for id %d pipe %x, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          pipe_dev_tgt.dev_pipe_id,
          status);
      return status;
    }
  }
  if (membrs_set == false) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        selector_context_info->tableHdlGet(),
        sel_grp_hdl,
        members.size(),
        action_entry_hdls.data(),
        (bool *)(pipe_member_status.data()),
        0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          pipe_dev_tgt.dev_pipe_id,
          status);
      return status;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags & /*flags*/,
                                const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key = static_cast<const SelectorTableKey &>(key);
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  status = pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      sel_grp_id,
      &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  status = pipeMgr->pipeMgrSelGrpDel(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      sel_grp_hdl,
      0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error deleting selector group %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              sel_grp_id,
              status);
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags & /*flags*/
                             ) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  while (TDI_SUCCESS == pipeMgr->pipeMgrGetFirstEntryHandle(
                            session.handleGet(static_cast<tdi_mgr_type_e>(
                                TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                            table_context_info->tableHdlGet(),
                            pipe_dev_tgt,
                            (int *)&sel_grp_hdl)) {
    status = pipeMgr->pipeMgrSelGrpDel(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        sel_grp_hdl,
        0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s Error deleting selector group %d pipe %x, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                sel_grp_hdl,
                pipe_dev_tgt.dev_pipe_id,
                status);
      return status;
    }
  }
  return status;
}

tdi_status_t Selector::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi_id_t &grp_id,
    SelectorTableData *sel_tbl_data) const {
  tdi_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      grp_id,
      &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }

  // Get the max size configured for the group
  uint32_t max_grp_size = 0;
  status = pipeMgr->pipeMgrSelGrpMaxSizeGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      sel_grp_hdl,
      &max_grp_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get size for Grp Id %d on pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              pipe_dev_tgt.dev_pipe_id);
    return status;
  }

  // Query pipe mgr for member and status list
  uint32_t count = 0;
  status = pipeMgr->pipeMgrGetSelGrpMbrCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      sel_grp_hdl,
      &count);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get info for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              pipe_dev_tgt.dev_pipe_id);
    return status;
  }

  std::vector<pipe_adt_ent_hdl_t> pipe_members(count, 0);
  std::vector<char> pipe_member_status(count, 0);
  uint32_t mbrs_populated = 0;
  bool from_hw = false;
  status = pipeMgr->pipeMgrSelGrpMbrsGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      sel_grp_hdl,
      count,
      pipe_members.data(),
      (bool *)(pipe_member_status.data()),
      &mbrs_populated,
      from_hw);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get membership for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              pipe_dev_tgt.dev_pipe_id);
    return status;
  }

  std::vector<tdi_id_t> member_ids;
  std::vector<bool> member_id_status;
  for (unsigned i = 0; i < mbrs_populated; i++) {
    tdi_id_t member_id = 0;
    status = getActMbrIdFromHndl(session, dev_tgt, pipe_members[i], &member_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting member id for member hdl %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                pipe_members[i]);
      return TDI_INVALID_ARG;
    }

    member_ids.push_back(member_id);
    member_id_status.push_back(pipe_member_status[i]);
  }
  sel_tbl_data->setMembers(member_ids);
  sel_tbl_data->setMemberStatus(member_id_status);
  sel_tbl_data->setMaxGrpSize(max_grp_size);
  return TDI_SUCCESS;
}

tdi_status_t Selector::entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const {
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);
  SelectorTableData *sel_tbl_data = static_cast<SelectorTableData *>(data);
  tdi_id_t grp_id = sel_tbl_key.getGroupId();

  return entryGet_internal(session, dev_tgt, grp_id, sel_tbl_data);
}

tdi_status_t Selector::entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags & /*flags*/,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const {
  SelectorTableKey *sel_tbl_key = static_cast<SelectorTableKey *>(key);
  tdi_id_t grp_id;
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrSelGrpIdGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      entry_handle,
      &grp_id);
  if (status != TDI_SUCCESS) return status;
  sel_tbl_key->setGroupId(grp_id);
  // TODO(sayanb): Get correct target
  *entry_tgt = dev_tgt;
  return status;
}

tdi_status_t Selector::entryHandleGet(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      const tdi::TableKey &key,
                                      tdi_handle_t *entry_handle) const {
  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);
  tdi_id_t sel_grp_id = sel_tbl_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      sel_grp_id,
      entry_handle);
}

tdi_status_t Selector::entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const {
  tdi::tna::tofino::Target entry_tgt(
      0, TNA_DEV_PIPE_ALL, TNA_DIRECTION_INGRESS, 0);
  tdi_status_t status =
      this->entryKeyGet(session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != TDI_SUCCESS) {
    return status;
  }
  return this->entryGet(session, entry_tgt, flags, *key, data);
}

tdi_status_t Selector::entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  if (read_from_hw) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported."
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      (int *)&sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first handle, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  return this->entryGet(session, dev_tgt, flags, sel_grp_hdl, key, data);
}

tdi_status_t Selector::entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     tdi::Table::keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_id_t sel_grp_id = sel_tbl_key.getGroupId();
  tdi_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      sel_grp_id,
      &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Grp Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              sel_grp_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      pipe_entry_hdl,
      n,
      next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    if (next_entry_handles[i] == -1) {
      break;
    }
    auto this_key = static_cast<TableKey *>((*key_data_pairs)[i].first);
    auto this_data = static_cast<tdi::TableData *>((*key_data_pairs)[i].second);

    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d does not match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    status = entryGet(
        session, dev_tgt, flags, next_entry_handles[i], this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with group "
          "handle %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
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
  return TDI_SUCCESS;
}

tdi_status_t Selector::getOneMbr(const tdi::Session &session,
                                 const uint16_t device_id,
                                 const pipe_sel_grp_hdl_t sel_grp_hdl,
                                 pipe_adt_ent_hdl_t *member_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  return pipeMgr->pipeMgrGetFirstGroupMember(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      device_id,
      sel_grp_hdl,
      member_hdl);
}

tdi_status_t Selector::getGrpIdFromHndl(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const pipe_sel_grp_hdl_t &sel_grp_hdl,
                                        tdi_id_t *sel_grp_id) const {
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpIdGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      sel_grp_hdl,
      sel_grp_id);
}

tdi_status_t Selector::getGrpHdl(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi_id_t sel_grp_id,
                                 pipe_sel_grp_hdl_t *sel_grp_hdl) const {
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      sel_grp_id,
      sel_grp_hdl);
}

tdi_status_t Selector::usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t Selector::keyAllocate(std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new SelectorTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::keyReset(TableKey *key) const {
  SelectorTableKey *sel_key = static_cast<SelectorTableKey *>(key);
  return key_reset<Selector, SelectorTableKey>(*this, sel_key);
}

tdi_status_t Selector::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  const std::vector<tdi_id_t> fields{};
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str())
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str())
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::dataReset(tdi::TableData *data) const {
  SelectorTableData *sel_data = static_cast<SelectorTableData *>(data);

#if 0  // TODO
  if (!this->validateTable_from_dataObj(*sel_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return sel_data->reset();
}

#if 0  // TODO: Attributes
tdi_status_t Selector::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  if (type != TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

tdi_status_t Selector::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  if (type != TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t Selector::processSelUpdateCbAttr(
    const tdi::tna::tofino::TableAttributes &tbl_attr_impl,
    const tdi::Target &dev_tgt) const {
  // 1. From the table attribute object, get the selector update parameters.
  // 2. Make a table attribute state object to store the parameters that are
  // required for when the callback is invoked.
  // 3. Invoke pipe-mgr callback registration function to register TDI
  // internal callback.
  auto t = tbl_attr_impl.selectorUpdateCbInternalGet();

  auto enable = std::get<0>(t);
  auto session = std::get<1>(t);
  auto cpp_callback_fn = std::get<2>(t);
  auto c_callback_fn = std::get<3>(t);
  auto cookie = std::get<4>(t);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto device_state =
      DevMgrImpl::tdiDeviceStateGet(pipe_dev_tgt.device_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id);
    TDI_ASSERT(0);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto session_obj = session.lock();

  if (session_obj == nullptr) {
    LOG_TRACE("%s:%d %s ERROR Session object passed no longer exists",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(tableInfoGet()->idGet());

  StateSelUpdateCb sel_update_cb(
      enable, this, session, cpp_callback_fn, c_callback_fn, cookie);

  attributes_state->setSelUpdateCbObj(sel_update_cb);

  auto *pipeMgr = PipeMgrIntf::getInstance(*session_obj);
  tdi_status_t status = pipeMgr->pipeMgrSelTblRegisterCb(
      session_obj->handleGet(),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      selUpdatePipeMgrInternalCb,
      &(attributes_state->getSelUpdateCbObj()));

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in registering selector update callback with pipe-mgr, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    // Reset the selector update callback object, since pipeMgr registration did
    // not succeed
    attributes_state->resetSelUpdateCbObj();
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t Selector::tableAttributesSet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(tbl_attr_impl->attributeTypeGet());

  if (attr_type != TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(attr_type));
    return TDI_INVALID_ARG;
  }
  return this->processSelUpdateCbAttr(*tbl_attr_impl, dev_tgt);
}

tdi_status_t Selector::tableAttributesGet(
    const tdi::Session & /*session */,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    TableAttributes *tableAttributes) const {
  // 1. From the table attribute state, retrieve all the params that were
  // registered by the user
  // 2. Set the params in the passed in tableAttributes obj

  auto tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(tbl_attr_impl->attributeTypeGet());
  if (attr_type != TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(attr_type));
    return TDI_INVALID_ARG;
  }

  auto device_state =
      DevMgrImpl::tdiDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_tgt.dev_id);
    TDI_ASSERT(0);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(tableInfoGet()->idGet());
  StateSelUpdateCb sel_update_cb;
  attributes_state->getSelUpdateCbObj(&sel_update_cb);

  // Set the state in the attribute object
  auto state_param = sel_update_cb.stateGet();
  tbl_attr_impl->selectorUpdateCbInternalSet(
      std::make_tuple(std::get<0>(state_param),
                      std::get<2>(state_param),
                      std::get<3>(state_param),
                      std::get<4>(state_param),
                      std::get<5>(state_param)));

  return TDI_SUCCESS;
}
#endif

// COUNTER TABLE APIS

tdi_status_t CounterIndirect::entryAdd(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags & /*flags*/,
                                       const tdi::TableKey &key,
                                       const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const CounterIndirectTableKey &cntr_key =
      static_cast<const CounterIndirectTableKey &>(key);
  const CounterIndirectTableData &cntr_data =
      static_cast<const CounterIndirectTableData &>(data);

  uint32_t counter_id = cntr_key.getCounterId();
  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  const pipe_stat_data_t *stat_data =
      cntr_data.getCounterSpecObj().getPipeCounterSpec();
  status = pipeMgr->pipeMgrStatEntSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      counter_id,
      const_cast<pipe_stat_data_t *>(stat_data));

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying counter index %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              counter_id,
              status);
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::entryMod(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       const tdi::TableKey &key,
                                       const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t CounterIndirect::entryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       const tdi::TableKey &key,
                                       tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const CounterIndirectTableKey &cntr_key =
      static_cast<const CounterIndirectTableKey &>(key);
  CounterIndirectTableData *cntr_data =
      static_cast<CounterIndirectTableData *>(data);

  uint32_t counter_id = cntr_key.getCounterId();
  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    status = pipeMgr->pipeMgrStatEntDatabaseSync(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        counter_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter "
          "idx %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          counter_id,
          status);
      return status;
    }
  }

  pipe_stat_data_t stat_data = {0};
  pipe_stat_data_t *ptr = &stat_data;
  status = pipeMgr->pipeMgrStatEntQuery(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &counter_id,
      1,
      &ptr);

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        counter_id,
        status);
    return status;
  }

  cntr_data->getCounterSpecObj().setCounterDataFromCounterSpec(stat_data);

  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::entryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       const tdi_handle_t &entry_handle,
                                       tdi::TableKey *key,
                                       tdi::TableData *data) const {
  CounterIndirectTableKey *cntr_key =
      static_cast<CounterIndirectTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t CounterIndirect::entryKeyGet(const tdi::Session & /*session*/,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags & /*flags*/,
                                          const tdi_handle_t &entry_handle,
                                          tdi::Target *entry_tgt,
                                          tdi::TableKey *key) const {
  CounterIndirectTableKey *cntr_key =
      static_cast<CounterIndirectTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::entryHandleGet(const tdi::Session & /*session*/,
                                             const tdi::Target & /*dev_tgt*/,
                                             const tdi::Flags & /*flags*/,
                                             const tdi::TableKey &key,
                                             tdi_handle_t *entry_handle) const {
  const CounterIndirectTableKey &cntr_key =
      static_cast<const CounterIndirectTableKey &>(key);
  *entry_handle = cntr_key.getCounterId();
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::entryGetFirst(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags &flags,
                                            tdi::TableKey *key,
                                            tdi::TableData *data) const {
  CounterIndirectTableKey *cntr_key =
      static_cast<CounterIndirectTableKey *>(key);
  return getFirst_for_resource_tbls<CounterIndirect, CounterIndirectTableKey>(
      *this, session, dev_tgt, flags, cntr_key, data);
}

tdi_status_t CounterIndirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const CounterIndirectTableKey &cntr_key =
      static_cast<const CounterIndirectTableKey &>(key);

  bf_status_t status = BF_SUCCESS;
  size_t table_size = this->tableInfoGet()->sizeGet();
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
        static_cast<CounterIndirectTableKey *>((*key_data_pairs)[j].first);
    this_key->setIdxKey(i);
    stat_idx[j] = i;
    auto this_data =
        static_cast<CounterIndirectTableData *>((*key_data_pairs)[j].second);
    stat_data[j] = this_data->getCounterSpecObj().getPipeCounterSpec();
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  if (read_from_hw) {
    status = pipeMgr->pipeMgrStatDatabaseSync(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        nullptr,
        nullptr);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter"
          ", err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          status);
      return status;
    }
  }

  status = pipeMgr->pipeMgrStatEntQuery(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      stat_idx.data(),
      j,
      stat_data.data());

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        1,  // counter_id,
        status);
    return status;
  }
  *num_returned = j;
  return BF_SUCCESS;
}

tdi_status_t CounterIndirect::clear(const tdi::Session &session,
                                    const tdi::Target &dev_tgt,
                                    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrStatTableReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing counter table err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t CounterIndirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new CounterIndirectTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::keyReset(TableKey *key) const {
  CounterIndirectTableKey *counter_key =
      static_cast<CounterIndirectTableKey *>(key);
  return key_reset<CounterIndirect, CounterIndirectTableKey>(*this,
                                                             counter_key);
}

tdi_status_t CounterIndirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new CounterIndirectTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirect::dataReset(tdi::TableData *data) const {
  CounterIndirectTableData *counter_data =
      static_cast<CounterIndirectTableData *>(data);

#if 0  // TODO
  if (!this->validateTable_from_dataObj(*counter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return counter_data->reset();
}

// METER TABLE

tdi_status_t MeterIndirect::entryAdd(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/,
                                     const tdi::TableKey &key,
                                     const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MeterIndirectTableKey &meter_key =
      static_cast<const MeterIndirectTableKey &>(key);
  const MeterIndirectTableData &meter_data =
      static_cast<const MeterIndirectTableData &>(data);

  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  const pipe_meter_spec_t *meter_spec =
      meter_data.getMeterSpecObj().getPipeMeterSpec();
  status = pipeMgr->pipeMgrMeterEntSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      meter_idx,
      (pipe_meter_spec_t *)meter_spec,
      0 /* Pipe API flags */);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying meter index %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              meter_idx,
              status);
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::entryMod(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t MeterIndirect::entryGet(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MeterIndirectTableKey &meter_key =
      static_cast<const MeterIndirectTableKey &>(key);
  MeterIndirectTableData *meter_data =
      static_cast<MeterIndirectTableData *>(data);

  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();
  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  pipe_meter_spec_t meter_spec;
  std::memset(&meter_spec, 0, sizeof(meter_spec));

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  status = pipeMgr->pipeMgrMeterReadEntryIdx(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      meter_idx,
      &meter_spec,
      read_from_hw);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading meter entry idx %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              meter_idx,
              status);
    return status;
  }

  // Populate data elements right here
  meter_data->getMeterSpecObj().setMeterDataFromMeterSpec(meter_spec);
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::entryGet(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi_handle_t &entry_handle,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const {
  MeterIndirectTableKey *mtr_key = static_cast<MeterIndirectTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t MeterIndirect::entryKeyGet(const tdi::Session & /*session*/,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        const tdi_handle_t &entry_handle,
                                        tdi::Target *entry_tgt,
                                        tdi::TableKey *key) const {
  MeterIndirectTableKey *mtr_key = static_cast<MeterIndirectTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::entryHandleGet(const tdi::Session & /*session*/,
                                           const tdi::Target & /*dev_tgt*/,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           tdi_handle_t *entry_handle) const {
  const MeterIndirectTableKey &mtr_key =
      static_cast<const MeterIndirectTableKey &>(key);
  *entry_handle = mtr_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::entryGetFirst(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          tdi::TableKey *key,
                                          tdi::TableData *data) const {
  MeterIndirectTableKey *meter_key = static_cast<MeterIndirectTableKey *>(key);
  return getFirst_for_resource_tbls<MeterIndirect, MeterIndirectTableKey>(
      *this, session, dev_tgt, flags, meter_key, data);
}

tdi_status_t MeterIndirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const MeterIndirectTableKey &meter_key =
      static_cast<const MeterIndirectTableKey &>(key);
  return getNext_n_for_resource_tbls<MeterIndirect, MeterIndirectTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      meter_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t MeterIndirect::clear(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMeterReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing Meter table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t MeterIndirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MeterIndirectTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::keyReset(TableKey *key) const {
  MeterIndirectTableKey *meter_key = static_cast<MeterIndirectTableKey *>(key);
  return key_reset<MeterIndirect, MeterIndirectTableKey>(*this, meter_key);
}

tdi_status_t MeterIndirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new MeterIndirectTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::dataReset(tdi::TableData *data) const {
  MeterIndirectTableData *meter_data =
      static_cast<MeterIndirectTableData *>(data);

#if 0  // TODO
  if (!this->validateTable_from_dataObj(*meter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return meter_data->reset();
}

tdi_status_t MeterIndirect::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}
#if 0
tdi_status_t MeterIndirect::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  return tbl_attr_impl.resetAttributeType(type);
}
#endif
tdi_status_t MeterIndirect::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      int byte_count;
      tdi_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          table_context_info->tableHdlGet(),
          byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterIndirect::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl =
      static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      int byte_count;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMeterByteCountGet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          table_context_info->tableHdlGet(),
          &byte_count);
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

// Register Indirect
tdi_status_t RegisterIndirect::entryAdd(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        const tdi::TableKey &key,
                                        const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterIndirectTableKey &register_key =
      static_cast<const RegisterIndirectTableKey &>(key);
  const RegisterIndirectTableData &register_data =
      static_cast<const RegisterIndirectTableData &>(data);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  std::vector<tdi_id_t> dataFields;
  dataFields = this->tableInfoGet()->dataFieldIdListGet();

  const auto &register_spec_data = register_data.getRegisterSpecObj();
  const auto &action_id = register_data.actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->tableInfoGet()->dataFieldGet(dataFields[0], action_id);
  if (!tableDataField) {
    return TDI_OBJECT_NOT_FOUND;
  }

  register_spec_data.populateStfulSpecFromData(&stful_spec);

  return pipeMgr->pipeStfulEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(
                                      TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                  pipe_dev_tgt,
                                  table_context_info->tableHdlGet(),
                                  register_idx,
                                  &stful_spec,
                                  0 /* Pipe API flags */);
}

tdi_status_t RegisterIndirect::entryMod(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t RegisterIndirect::entryGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterIndirectTableKey &register_key =
      static_cast<const RegisterIndirectTableKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  // Query number of pipes to get possible number of results.
  int num_pipes = 0;
  status = pipeMgr->pipeStfulQueryGetSizes(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      &num_pipes);

  // Use vectors to populate pipe mgr stful query data structure.
  // One vector to hold all possible pipe data.
  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;

  uint32_t pipe_api_flags = 0;
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->pipeStfulEntQuery(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      register_idx,
      &stful_query,
      pipe_api_flags);
  if (status == TDI_SUCCESS) {
    std::vector<tdi_id_t> dataFields;
    dataFields = this->tableInfoGet()->dataFieldIdListGet();
    // Down cast to RegisterIndirectTableData
    RegisterIndirectTableData *register_data =
        static_cast<RegisterIndirectTableData *>(data);
    auto &register_spec_data = register_data->getRegisterSpecObj();
    const auto &action_id = register_data->actionIdGet();
    const tdi::DataFieldInfo *tableDataField =
        this->tableInfoGet()->dataFieldGet(dataFields[0], action_id);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    // pipe_count is returned upon successful query,
    // hence use it instead of vector size.
    register_spec_data.populateDataFromStfulSpec(
        register_pipe_data, static_cast<uint32_t>(stful_query.pipe_count));
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirect::entryGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi_handle_t &entry_handle,
                                        tdi::TableKey *key,
                                        tdi::TableData *data) const {
  RegisterIndirectTableKey *reg_key =
      static_cast<RegisterIndirectTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t RegisterIndirect::entryKeyGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi_handle_t &entry_handle,
                                           tdi::Target *entry_tgt,
                                           tdi::TableKey *key) const {
  RegisterIndirectTableKey *reg_key =
      static_cast<RegisterIndirectTableKey *>(key);
  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, entry_handle)) {
    return TDI_INVALID_ARG;
  }
  reg_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirect::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const RegisterIndirectTableKey &reg_key =
      static_cast<const RegisterIndirectTableKey &>(key);
  pipe_stful_mem_idx_t register_idx = reg_key.getIdxKey();
  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }
  *entry_handle = register_idx;
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirect::entryGetFirst(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  RegisterIndirectTableKey *register_key =
      static_cast<RegisterIndirectTableKey *>(key);
  return getFirst_for_resource_tbls<RegisterIndirect, RegisterIndirectTableKey>(
      *this, session, dev_tgt, flags, register_key, data);
}

tdi_status_t RegisterIndirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const RegisterIndirectTableKey &register_key =
      static_cast<const RegisterIndirectTableKey &>(key);
  return getNext_n_for_resource_tbls<RegisterIndirect,
                                     RegisterIndirectTableKey>(*this,
                                                               session,
                                                               dev_tgt,
                                                               flags,
                                                               register_key,
                                                               n,
                                                               key_data_pairs,
                                                               num_returned);
}

tdi_status_t RegisterIndirect::clear(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeStfulTableReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing register table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t RegisterIndirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new RegisterIndirectTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirect::keyReset(TableKey *key) const {
  RegisterIndirectTableKey *register_key =
      static_cast<RegisterIndirectTableKey *>(key);
  return key_reset<RegisterIndirect, RegisterIndirectTableKey>(*this,
                                                               register_key);
}

tdi_status_t RegisterIndirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new RegisterIndirectTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterIndirect::dataReset(tdi::TableData *data) const {
  RegisterIndirectTableData *register_data =
      static_cast<RegisterIndirectTableData *>(data);

#if 0  // TODO
  if (!this->validateTable_from_dataObj(*register_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return register_data->reset();
}

// PVSTable****************
tdi_status_t PVS::entryAdd(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags & /*flags*/,
                           const tdi::TableKey &pvs_key,
                           const tdi::TableData & /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const PVSTableKey &match_key = static_cast<const PVSTableKey &>(pvs_key);
  uint32_t entry_id;
  uint32_t key = 0, mask = 0;
  match_key.populate_match_spec(&key, &mask);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  auto sts = pipeMgr->pipeMgrPvsEntryAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),  // pvs_hdl
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      key,
      mask,
      &entry_id);
  if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
  }
  return sts;
}

tdi_status_t PVS::entryDel(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags & /*flags*/,
                           const tdi::TableKey &pvs_key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const PVSTableKey &match_key = static_cast<const PVSTableKey &>(pvs_key);
  pipe_pvs_hdl_t pvs_entry_handle;
  uint32_t key, mask;
  match_key.populate_match_spec(&key, &mask);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrPvsEntryHandleGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      key,
      mask,
      &pvs_entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }
  status = pipeMgr->pipeMgrPvsEntryDelete(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      pvs_entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %s",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_str_err(pipe_status_t(status)));
  }
  return status;
}

tdi_status_t PVS::clear(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrPvsClear(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Table clear failed for table %s, err %s",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_str_err(pipe_status_t(status)));
  }
  return status;
}

tdi_status_t PVS::entryGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi::TableKey &pvs_key,
                           tdi::TableData * /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const PVSTableKey &match_key = static_cast<const PVSTableKey &>(pvs_key);
  // PvsTableData *pvs_data = static_cast<PvsTableData *>(data);
  pipe_pvs_hdl_t pvs_entry_handle;
  uint32_t key, mask, key1, mask1;
  match_key.populate_match_spec(&key, &mask);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrPvsEntryHandleGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      key,
      mask,
      &pvs_entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  if (read_from_hw) {
    status = pipeMgr->pipeMgrPvsEntryGetHw(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        direction,
        pipe_dev_tgt.dev_pipe_id,
        prsr_id,
        table_context_info->tableHdlGet(),
        pvs_entry_handle,
        &key1,
        &mask1);
  } else {
    status = pipeMgr->pipeMgrPvsEntryGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        pvs_entry_handle,
        &key1,
        &mask1,
        NULL,
        NULL,
        NULL);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading pvs entry err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  // Check whether the key/mask pair that's read back from pipe_mgr matches the
  // one in tdi
  if ((key != key1) || (mask != mask1)) {
    status = TDI_UNEXPECTED;
    LOG_ERROR(
        "%s:%d %s ERROR reading pvs entry err %d, fetched value do not match "
        "provided key",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
  }
  return status;
}

tdi_status_t PVS::entryKeyGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags & /*flags*/,
                              const tdi_handle_t &entry_handle,
                              tdi::Target *entry_tgt,
                              tdi::TableKey *key) const {
  PVSTableKey *match_key = static_cast<PVSTableKey *>(key);
  uint32_t pkey, pmask;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  bf_dev_pipe_t pipe_id;
  uint8_t prsr_id;

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  uint8_t gress;
  tdi_status_t status = pipeMgr->pipeMgrPvsEntryGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      entry_handle,
      &pkey,
      &pmask,
      &gress,
      &pipe_id,
      &prsr_id);

  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TNA_TARGET_DIRECTION),
                      static_cast<bf_dev_direction_t>(gress));
  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TNA_TARGET_PIPE_ID),
                      pipe_id);
  entry_tgt->setValue(static_cast<tdi_target_e>(TDI_TOFINO_TARGET_PARSER_ID),
                      prsr_id);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pkey, pmask);
  return status;
}

tdi_status_t PVS::entryHandleGet(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags & /*flags*/,
                                 const tdi::TableKey &key,
                                 tdi_handle_t *entry_handle) const {
  const PVSTableKey &match_key = static_cast<const PVSTableKey &>(key);
  uint32_t pkey, pmask;
  match_key.populate_match_spec(&pkey, &pmask);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrPvsEntryHandleGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      pkey,
      pmask,
      static_cast<pipe_pvs_hdl_t *>(entry_handle));

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t PVS::entryGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::TableKey *key,
                           tdi::TableData * /*data*/) const {
  PVSTableKey *match_key = static_cast<PVSTableKey *>(key);
  uint32_t pkey, pmask;
  tdi_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  if (read_from_hw) {
    status = pipeMgr->pipeMgrPvsEntryGetHw(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        direction,
        pipe_dev_tgt.dev_pipe_id,
        prsr_id,
        table_context_info->tableHdlGet(),
        entry_handle,
        &pkey,
        &pmask);
  } else {
    status = pipeMgr->pipeMgrPvsEntryGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        entry_handle,
        &pkey,
        &pmask,
        NULL,
        NULL,
        NULL);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading pvs entry err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pkey, pmask);
  return status;
}

tdi_status_t PVS::entryGetFirst(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags & /*flags*/,
                                tdi::TableKey *key,
                                tdi::TableData * /*data*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_pvs_hdl_t pvs_entry_handle;
  tdi_status_t status = pipeMgr->pipeMgrPvsEntryGetFirst(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      &pvs_entry_handle);
  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d getting first pvs entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  uint32_t pipe_key = 0, pipe_msk = 0;
  status = pipeMgr->pipeMgrPvsEntryGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      pvs_entry_handle,
      &pipe_key,
      &pipe_msk,
      NULL,
      NULL,
      NULL);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d mapping pvs entry %d to key and mask",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status,
              pvs_entry_handle);
    return status;
  }

  PVSTableKey *pvs_key = static_cast<PVSTableKey *>(key);
  pvs_key->populate_key_from_match_spec(pipe_key, pipe_msk);
  return TDI_SUCCESS;
}

tdi_status_t PVS::entryGetNextN(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags & /*flags*/,
                                const tdi::TableKey &cur_key,
                                const uint32_t &n,
                                tdi::Table::keyDataPairs *key_data_pairs,
                                uint32_t *num_returned) const {
  if (!key_data_pairs) return TDI_INVALID_ARG;

  // Make sure there is room to return results.
  if (key_data_pairs->capacity() < n) {
    LOG_TRACE(
        "%s:%d %s ERROR getting next pvs entries, requested %d entries but "
        "only provided storage for %zu",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        n,
        key_data_pairs->capacity());
    return TDI_INVALID_ARG;
  }

  if (num_returned != nullptr) *num_returned = 0;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Map the passed in key to an entry handle.
  const PVSTableKey &cur_entry = static_cast<const PVSTableKey &>(cur_key);
  pipe_pvs_hdl_t entry_handle;
  uint32_t key, mask;
  cur_entry.populate_match_spec(&key, &mask);
  tdi_status_t status = pipeMgr->pipeMgrPvsEntryHandleGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      key,
      mask,
      &entry_handle);
  if (status != TDI_SUCCESS) return status;

  // Use pipe_mgr's get next to get the set of next entry handles.
  std::vector<pipe_pvs_hdl_t> next_entries(n);
  status = pipeMgr->pipeMgrPvsEntryGetNext(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      entry_handle,
      n,
      next_entries.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR %d getting next pvs entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  // Map each entry handle to its key and mask to populate the return data.
  for (unsigned i = 0; i < n; ++i) {
    int hdl = next_entries[i];
    if (hdl == -1) break;

    uint32_t pipe_key = 0, pipe_msk = 0;
    status = pipeMgr->pipeMgrPvsEntryGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        hdl,
        &pipe_key,
        &pipe_msk,
        NULL,
        NULL,
        NULL);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR %d mapping pvs entry %d to key and mask",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                status,
                hdl);
      return status;
    }
    auto k = static_cast<PVSTableKey *>(key_data_pairs->at(i).first);
    k->populate_key_from_match_spec(pipe_key, pipe_msk);
    if (num_returned) ++(*num_returned);
  }
  return TDI_SUCCESS;
}

tdi_status_t PVS::usageGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           uint32_t *count) const {
  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return PipeMgrIntf::getInstance(session)->pipeMgrPvsEntryGetCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      table_context_info->tableHdlGet(),
      direction,
      pipe_dev_tgt.dev_pipe_id,
      prsr_id,
      read_from_hw,
      count);
}

tdi_status_t PVS::keyAllocate(std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new PVSTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PVS::keyReset(TableKey *key) const {
  PVSTableKey *pvs_key = static_cast<PVSTableKey *>(key);
  return key_reset<PVS, PVSTableKey>(*this, pvs_key);
}

// PVS table has no data, this is a dummy data node
tdi_status_t PVS::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new tdi::TableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PVS::dataReset(tdi::TableData * /*data*/) const {
  // Nothing to do for PVS Table data reset
  return TDI_SUCCESS;
}

tdi_status_t PVS::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  // const auto att_type = static_cast<tdi_tofino_attributes_type_e>(type);
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  if (static_cast<tdi_tofino_attributes_type_e>(type) !=
      TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE) {
    LOG_TRACE(
        "%s:%d %s PVS Table Runtime Attributes only support setting"
        "entry scope: gress scope, pipe scope, and parser scope",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

#if 0
tdi_status_t PVS::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  switch (type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE:
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}
#endif

tdi_status_t PVS::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableAttributes &tableAttributes) const {
  const auto &tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes &>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl.attributeTypeGet());
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipe_mgr_pvs_prop_args_t args;
      pipe_mgr_pvs_prop_type_t property;
      pipe_mgr_pvs_prop_value_t value;

      tdi_tofino_attributes_gress_scope_e gress;
      tdi_tofino_attributes_entry_scope_e pipe;
      tdi_tofino_attributes_parser_scope_e prsr;
      TableEntryScopeArguments pipe_args(0);
      tdi_tofino_attributes_gress_target_e prsr_gress;

      tdi_status_t sts = tbl_attr_impl.entry_scope_.entryScopeParamsGet(
          &pipe, static_cast<TableEntryScopeArguments *>(&pipe_args));
      CHECK_PRINT_RETURN(sts, "Failed to get entry scope params");

      sts = tbl_attr_impl.entry_scope_.gressScopeParamsGet(&gress);
      CHECK_PRINT_RETURN(sts, "Failed to get gress scope params");

      sts = tbl_attr_impl.entry_scope_.prsrScopeParamsGet(&prsr, &prsr_gress);
      CHECK_PRINT_RETURN(sts, "Failed to get prsr scope params");

      // set gress scope
      args.value = 0;
      property = PIPE_MGR_PVS_GRESS_SCOPE;
      if (gress == TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_ALL_GRESS) {
        value.gress_scope = PIPE_MGR_PVS_SCOPE_ALL_GRESS;
      } else if (gress == TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_SINGLE_GRESS) {
        value.gress_scope = PIPE_MGR_PVS_SCOPE_SINGLE_GRESS;
      } else {
        LOG_TRACE("%s:%d %s Invalid gress scope (%d)",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  static_cast<int>(gress));
        return TDI_NOT_SUPPORTED;
      }
      sts = pipeMgr->pipeMgrPvsSetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Gress scope setting fails",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
      // set pipe scope
      property = PIPE_MGR_PVS_PIPE_SCOPE;
      std::bitset<32> bitval;
      pipe_args.getValue(&bitval);
      std::cout << "bitval: " << bitval.to_ulong() << std::endl;
      switch (pipe) {
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_ALL_PIPELINES;
          args.value = bitval.to_ulong();
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_SINGLE_PIPELINE;
          args.value = bitval.to_ulong();
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED:
          value.pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
          std::memcpy(args.user_defined.user_defined_scope,
                      &bitval,
                      sizeof(scope_pipes_t) * PIPE_MGR_MAX_USER_DEFINED_SCOPES);
          args.user_defined.gress = direction;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid pipe scope (%d)",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    static_cast<int>(pipe));
          return TDI_NOT_SUPPORTED;
      }
      sts = pipeMgr->pipeMgrPvsSetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Pipe scope setting fails",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
      // set prsr scope
      switch (prsr_gress) {
        case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_INGRESS:
          args.value = (uint32_t)BF_DEV_DIR_INGRESS;
          break;
        case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_EGRESS:
          args.value = (uint32_t)BF_DEV_DIR_EGRESS;
          break;
        case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_ALL:
          args.value = (uint32_t)BF_DEV_DIR_ALL;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid parser scope args (%d)",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    static_cast<int>(prsr_gress));
          return TDI_NOT_SUPPORTED;
      }
      switch (prsr) {
        case TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_ALL_PARSERS_IN_PIPE:
          value.parser_scope = PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE;
          break;
        case TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_SINGLE_PARSER:
          value.parser_scope = PIPE_MGR_PVS_SCOPE_SINGLE_PARSER;
          break;
        default:
          LOG_TRACE("%s:%d %s Invalid prsr scope (%d)",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    static_cast<int>(prsr));
          return TDI_NOT_SUPPORTED;
      }
      property = PIPE_MGR_PVS_PARSER_SCOPE;
      sts = pipeMgr->pipeMgrPvsSetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Parser scope setting fails",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
      break;
    }
    default:
      LOG_TRACE("%s:%d %s PVS table only support ENTRY_SCOPE",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return TDI_NOT_SUPPORTED;
  }
  return TDI_SUCCESS;
}

tdi_status_t PVS::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl =
      static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }

  dev_target_t pipe_dev_tgt;
  bf_dev_direction_t direction;
  uint8_t prsr_id;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, &direction, &prsr_id);

  auto table_context_info = static_cast<const TofinoTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      tdi_tofino_attributes_gress_scope_e gress_val;
      tdi_tofino_attributes_entry_scope_e pipe_val;
      TableEntryScopeArguments pipe_args(0);
      tdi_tofino_attributes_parser_scope_e prsr_val;
      tdi_tofino_attributes_gress_target_e gress;

      pipe_mgr_pvs_prop_args_t args;
      pipe_mgr_pvs_prop_type_t property;
      pipe_mgr_pvs_prop_value_t value;

      args.value = static_cast<uint32_t>(direction);
      // read gress scope value
      property = PIPE_MGR_PVS_GRESS_SCOPE;
      tdi_status_t sts = pipeMgr->pipeMgrPvsGetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          &value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      if (value.gress_scope == PIPE_MGR_PVS_SCOPE_ALL_GRESS)
        gress_val = TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_ALL_GRESS;
      else if (value.gress_scope == PIPE_MGR_PVS_SCOPE_SINGLE_GRESS)
        gress_val = TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_SINGLE_GRESS;
      else {
        LOG_TRACE("%s:%d %s Get Invalid gress scope (%d)",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  static_cast<int>(value.gress_scope));
        return TDI_INVALID_ARG;
      }
      // read pipe scope value
      property = PIPE_MGR_PVS_PIPE_SCOPE;
      sts = pipeMgr->pipeMgrPvsGetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          &value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_ALL_PIPELINES)
        pipe_val = TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES;
      else if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PIPELINE)
        pipe_val = TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE;
      else if (value.pipe_scope == PIPE_MGR_PVS_SCOPE_USER_DEFINED)
        pipe_val = TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED;
      else {
        LOG_TRACE("%s:%d %s Get Invalid pipe scope (%d)",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  static_cast<int>(value.pipe_scope));
        return TDI_INVALID_ARG;
      }
      std::bitset<32> bitval(args.value);
      pipe_args.setValue(bitval);
      // read prsr scope value
      property = PIPE_MGR_PVS_PARSER_SCOPE;
      sts = pipeMgr->pipeMgrPvsGetProperty(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt.device_id,
          table_context_info->tableHdlGet(),
          property,
          &value,
          args);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Get PVS property fail",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      if (value.parser_scope == PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE)
        prsr_val = TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_ALL_PARSERS_IN_PIPE;
      else if (value.parser_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PARSER)
        prsr_val = TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_SINGLE_PARSER;
      else {
        LOG_TRACE("%s:%d %s Get Invalid parser scope (%d)",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  static_cast<int>(value.parser_scope));
        return TDI_INVALID_ARG;
      }
      if (direction == BF_DEV_DIR_INGRESS) {
        gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_INGRESS;
      } else if (direction == BF_DEV_DIR_EGRESS) {
        gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_EGRESS;
      } else if (direction == BF_DEV_DIR_ALL) {
        gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_ALL;
      } else {
        LOG_TRACE("%s:%d %s Invalid target direction (%d)",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  static_cast<int>(direction));
        return TDI_INVALID_ARG;
      }
      // set attributes back to obj
      sts = tbl_attr_impl->entry_scope_.entryScopeParamsSet(
          pipe_val, static_cast<TableEntryScopeArguments &>(pipe_args));
      CHECK_PRINT_RETURN(sts, "Failed to set entry scope params");

      sts = tbl_attr_impl->entry_scope_.gressScopeParamsSet(gress_val);
      CHECK_PRINT_RETURN(sts, "Failed to set gress scope params");

      sts = tbl_attr_impl->entry_scope_.prsrScopeParamsSet(prsr_val, gress);
      CHECK_PRINT_RETURN(sts, "Failed to set prsr scope params");

      return sts;
    }
    default:
      LOG_TRACE("%s:%d %s PVS table only support ENTRY_SCOPE",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return TDI_NOT_SUPPORTED;
  }
  return TDI_SUCCESS;
}

#if 0
// LPF TABLE

tdi_status_t LPFTable::entryAdd(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        const tdi::TableKey &key,
                                        const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const LPFTableKey &lpf_key = static_cast<const LPFTableKey &>(key);
  const LPFTableData &lpf_data =
      static_cast<const LPFTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_lpf_idx_t lpf_idx = lpf_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, lpf_idx)) {
    return TDI_INVALID_ARG;
  }

  return pipeMgr->pipeMgrLpfEntSet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      lpf_idx,
      const_cast<pipe_lpf_spec_t *>(lpf_data.getLPFSpecObj().getPipeLPFSpec()),
      0 /* Pipe API flags */);
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::entryMod(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t LPFTable::entryGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        tdi::TableData *data) const {
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const LPFTableKey &lpf_key = static_cast<const LPFTableKey &>(key);
  LPFTableData *lpf_data = static_cast<LPFTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_lpf_spec_t lpf_spec;
  std::memset(&lpf_spec, 0, sizeof(lpf_spec));

  pipe_lpf_idx_t lpf_idx = lpf_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, lpf_idx)) {
    return TDI_INVALID_ARG;
  }
  status = pipeMgr->pipeMgrLpfReadEntryIdx(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, table_context_info->tableHdlGet(), lpf_idx, &lpf_spec);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading lpf entry idx %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              lpf_idx,
              status);
    return status;
  }

  // Populate data elements right here
  lpf_data->getLPFSpecObj().setLPFDataFromLPFSpec(&lpf_spec);
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::entryGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi_handle_t &entry_handle,
                                        tdi::TableKey *key,
                                        tdi::TableData *data) const {
  LPFTableKey *lpf_key = static_cast<LPFTableKey *>(key);
  lpf_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t LPFTable::entryKeyGet(const tdi::Session & /*session*/,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi_handle_t &entry_handle,
                                           tdi::Target *entry_tgt,
                                           tdi::TableKey *key) const {
  LPFTableKey *lpf_key = static_cast<LPFTableKey *>(key);
  lpf_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const LPFTableKey &lpf_key = static_cast<const LPFTableKey &>(key);
  *entry_handle = lpf_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::entryGetFirst(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  LPFTableKey *lpf_key = static_cast<LPFTableKey *>(key);
  return getFirst_for_resource_tbls<LPFTable, LPFTableKey>(
      *this, session, dev_tgt, flags, lpf_key, data);
}

tdi_status_t LPFTable::entryGetNext_n(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags &flags,
                                              const tdi::TableKey &key,
                                              const uint32_t &n,
                                              tdi::Table::keyDataPairs *key_data_pairs,
                                              uint32_t *num_returned) const {
  const LPFTableKey &lpf_key = static_cast<const LPFTableKey &>(key);
  return getNext_n_for_resource_tbls<LPFTable, LPFTableKey>(
      *this, session, dev_tgt, flags, lpf_key, n, key_data_pairs, num_returned);
}

tdi_status_t LPFTable::clear(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeMgrLpfReset(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                                pipe_dev_tgt,
                                                this->table_context_info->tableHdlGet(),
                                                0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing MeterLPF table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t LPFTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new LPFTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::keyReset(TableKey *key) const {
  LPFTableKey *lpf_key = static_cast<LPFTableKey *>(key);
  return key_reset<LPFTable, LPFTableKey>(*this, lpf_key);
}

tdi_status_t LPFTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new LPFTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t LPFTable::dataReset(tdi::TableData *data) const {
  LPFTableData *lpf_data = static_cast<LPFTableData *>(data);
  if (!this->validateTable_from_dataObj(*lpf_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return lpf_data->reset();
}

// WRED TABLE

tdi_status_t WREDTable::entryAdd(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags & /*flags*/,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const WREDTableKey &wred_key = static_cast<const WREDTableKey &>(key);
  const WREDTableData &wred_data =
      static_cast<const WREDTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_wred_idx_t wred_idx = wred_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, wred_idx)) {
    return TDI_INVALID_ARG;
  }

  return pipeMgr->pipeMgrWredEntSet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      wred_idx,
      const_cast<pipe_wred_spec_t *>(
          wred_data.getWREDSpecObj().getPipeWREDSpec()),
      0 /* Pipe API flags */);
}

tdi_status_t WREDTable::entryMod(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t WREDTable::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         tdi::TableData *data) const {
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const WREDTableKey &wred_key = static_cast<const WREDTableKey &>(key);
  WREDTableData *wred_data = static_cast<WREDTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_wred_spec_t wred_spec;
  std::memset(&wred_spec, 0, sizeof(wred_spec));

  pipe_wred_idx_t wred_idx = wred_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, wred_idx)) {
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrWredReadEntryIdx(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                            pipe_dev_tgt,
                                            table_context_info->tableHdlGet(),
                                            wred_idx,
                                            &wred_spec);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading WRED entry idx %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              wred_idx,
              status);
    return status;
  }

  // Populate data elements right here
  wred_data->getWREDSpecObj().setWREDDataFromWREDSpec(&wred_spec);
  return TDI_SUCCESS;
}

tdi_status_t WREDTable::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi_handle_t &entry_handle,
                                         tdi::TableKey *key,
                                         tdi::TableData *data) const {
  WREDTableKey *wred_key = static_cast<WREDTableKey *>(key);
  wred_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t WREDTable::entryKeyGet(const tdi::Session & /*session*/,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags & /*flags*/,
                                            const tdi_handle_t &entry_handle,
                                            tdi::Target *entry_tgt,
                                            tdi::TableKey *key) const {
  WREDTableKey *wred_key = static_cast<WREDTableKey *>(key);
  wred_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t WREDTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const WREDTableKey &wred_key = static_cast<const WREDTableKey &>(key);
  *entry_handle = wred_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t WREDTable::entryGetFirst(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags &flags,
                                              tdi::TableKey *key,
                                              tdi::TableData *data) const {
  WREDTableKey *wred_key = static_cast<WREDTableKey *>(key);
  return getFirst_for_resource_tbls<WREDTable, WREDTableKey>(
      *this, session, dev_tgt, flags, wred_key, data);
}

tdi_status_t WREDTable::entryGetNext_n(const tdi::Session &session,
                                               const tdi::Target &dev_tgt,
                                               const tdi::Flags &flags,
                                               const tdi::TableKey &key,
                                               const uint32_t &n,
                                               tdi::Table::keyDataPairs *key_data_pairs,
                                               uint32_t *num_returned) const {
  const WREDTableKey &wred_key = static_cast<const WREDTableKey &>(key);
  return getNext_n_for_resource_tbls<WREDTable, WREDTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      wred_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t WREDTable::clear(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeMgrWredReset(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                                 pipe_dev_tgt,
                                                 this->table_context_info->tableHdlGet(),
                                                 0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing WRED table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t WREDTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new WREDTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t WREDTable::keyReset(TableKey *key) const {
  WREDTableKey *wred_key = static_cast<WREDTableKey *>(key);
  return key_reset<WREDTable, WREDTableKey>(*this, wred_key);
}

tdi_status_t WREDTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new WREDTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t WREDTable::dataReset(tdi::TableData *data) const {
  WREDTableData *wred_data = static_cast<WREDTableData *>(data);
  if (!this->validateTable_from_dataObj(*wred_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return wred_data->reset();
}

// REGISTER TABLE
tdi_status_t RegisterTable::entryAdd(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags & /*flags*/,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  const RegisterTableData &register_data =
      static_cast<const RegisterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  std::vector<tdi_id_t> dataFields;
  tdi_status_t status = dataFieldIdListGet(&dataFields);
  TDI_ASSERT(status == TDI_SUCCESS);
  const auto &register_spec_data = register_data.getRegisterSpecObj();
  const tdi::DataFieldInfo *tableDataField;
  status = dataFieldGet(dataFields[0], &tableDataField);
  TDI_ASSERT(status == TDI_SUCCESS);
  register_spec_data.populateStfulSpecFromData(&stful_spec);

  return pipeMgr->pipeStfulEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                  pipe_dev_tgt,
                                  table_context_info->tableHdlGet(),
                                  register_idx,
                                  &stful_spec,
                                  0 /* Pipe API flags */);
}

tdi_status_t RegisterTable::entryMod(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t RegisterTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  // Query number of pipes to get possible number of results.
  int num_pipes = 0;
  status = pipeMgr->pipeStfulQueryGetSizes(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet(), &num_pipes);

  // Use vectors to populate pipe mgr stful query data structure.
  // One vector to hold all possible pipe data.
  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;

  uint32_t pipe_api_flags = 0;
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->pipeStfulEntQuery(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                      pipe_dev_tgt,
                                      table_context_info->tableHdlGet(),
                                      register_idx,
                                      &stful_query,
                                      pipe_api_flags);

  if (status == TDI_SUCCESS) {
    std::vector<tdi_id_t> dataFields;
    status = dataFieldIdListGet(&dataFields);
    TDI_ASSERT(status == TDI_SUCCESS);
    // Down cast to RegisterTableData
    RegisterTableData *register_data =
        static_cast<RegisterTableData *>(data);
    auto &register_spec_data = register_data->getRegisterSpecObj();
    const tdi::DataFieldInfo *tableDataField;
    status = dataFieldGet(dataFields[0], &tableDataField);
    TDI_ASSERT(status == TDI_SUCCESS);
    // pipe_count is returned upon successful query,
    // hence use it instead of vector size.
    register_spec_data.populateDataFromStfulSpec(
        register_pipe_data, static_cast<uint32_t>(stful_query.pipe_count));
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi_handle_t &entry_handle,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  RegisterTableKey *reg_key = static_cast<RegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t RegisterTable::entryKeyGet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  RegisterTableKey *reg_key = static_cast<RegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const RegisterTableKey &reg_key =
      static_cast<const RegisterTableKey &>(key);
  *entry_handle = reg_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryGetFirst(const tdi::Session &session,
                                                  const tdi::Target &dev_tgt,
                                                  const tdi::Flags &flags,
                                                  tdi::TableKey *key,
                                                  tdi::TableData *data) const {
  RegisterTableKey *register_key = static_cast<RegisterTableKey *>(key);
  return getFirst_for_resource_tbls<RegisterTable, RegisterTableKey>(
      *this, session, dev_tgt, flags, register_key, data);
}

tdi_status_t RegisterTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  return getNext_n_for_resource_tbls<RegisterTable, RegisterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      register_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t RegisterTable::clear(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeStfulTableReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, table_context_info->tableHdlGet(), nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing register table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t RegisterTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new RegisterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::keyReset(TableKey *key) const {
  RegisterTableKey *register_key = static_cast<RegisterTableKey *>(key);
  return key_reset<RegisterTable, RegisterTableKey>(*this,
                                                            register_key);
}

tdi_status_t RegisterTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new RegisterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::dataReset(tdi::TableData *data) const {
  RegisterTableData *register_data =
      static_cast<RegisterTableData *>(data);
  if (!this->validateTable_from_dataObj(*register_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return register_data->reset();
}

tdi_status_t RegisterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  auto attribute_type_set = tableInfoGet()->attributesSupported();
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t RegisterTable::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(tbl_attr_impl->attributeTypeGet());
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      tdi_tofino_attributes_entry_scope_e entry_scope;
      TableEntryScopeArguments scope_args(0);
      tdi_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<TableEntryScopeArguments *>(&scope_args));
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Unable to get the entry scope params",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from TDI entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<32> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ulong();
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      return pipeMgr->pipeMgrTblSetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                            dev_tgt.dev_id,
                                            ghost_pipe_tbl_hdl_,
                                            prop_type,
                                            prop_val,
                                            args_val);
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(tbl_attr_impl->attributeTypeGet());
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
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
            tableInfoGet()->nameGet().c_str(),
            tableInfoGet()->nameGet().c_str());
        return sts;
      }

      tdi_tofino_attributes_entry_scope_e entry_scope;
      TableEntryScopeArguments scope_args(args_val.value);

      // Derive TDI entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE
                              : TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<TableEntryScopeArguments &>(scope_args));
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::ghostTableHandleSet(
    const pipe_tbl_hdl_t &pipe_hdl) {
  ghost_pipe_tbl_hdl_ = pipe_hdl;
  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MatchActionKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::keyReset(TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  return key_reset<Phase0Table, MatchActionKey>(*this, match_key);
}

tdi_status_t Phase0Table::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  *data_ret =
      std::unique_ptr<tdi::TableData>(new Phase0TableData(this, action_id));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s : ERROR Unable to allocate data. Out of Memory",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }

  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::entryAdd(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  const Phase0TableData &match_data =
      static_cast<const Phase0TableData &>(data);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntAdd(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                   pipe_dev_tgt,
                                   table_context_info->tableHdlGet(),
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   pipe_action_spec,
                                   0 /* ttl */,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

tdi_status_t Phase0Table::entryMod(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        table_context_info->tableHdlGet(),
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
              false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  const Phase0TableData &match_data =
      static_cast<const Phase0TableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  status = pipeMgr->pipeMgrMatEntSetAction(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt.device_id,
      tablePipeHandleGet(),
      pipe_entry_hdl,
      act_fn_hdl,
      const_cast<pipe_action_spec_t *>(pipe_action_spec),
      0 /* Pipe API flags */);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::entryDel(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status =
      pipeMgr->pipeMgrMatEntDelByMatchSpec(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                           pipe_dev_tgt,
                                           table_context_info->tableHdlGet(),
                                           &pipe_match_spec,
                                           0 /* Pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t Phase0Table::clear(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/) const {
  return clearMatCommon(session, dev_tgt, false, this);
}

tdi_status_t Phase0Table::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    tdi::TableData *data) const {
  Phase0TableData &match_data = static_cast<Phase0TableData &>(*data);
  pipe_action_spec_t *pipe_action_spec = match_data.get_pipe_action_spec();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;

  tdi_status_t status = getActionSpec(session,
                                     pipe_dev_tgt,
                                     flags,
                                     table_context_info->tableHdlGet(),
                                     pipe_entry_hdl,
                                     PIPE_RES_GET_FLAG_ENTRY,
                                     pipe_match_spec,
                                     pipe_action_spec,
                                     &pipe_act_fn_hdl,
                                     nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  tdi_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data.actionIdSet(action_id);
  std::vector<tdi_id_t> empty;
  match_data.setActiveFields(empty);
  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi::TableKey &key,
                                           tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        table_context_info->tableHdlGet(),
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
      true /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return this->entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

tdi_status_t Phase0Table::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

tdi_status_t Phase0Table::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        table_context_info->tableHdlGet(),
                                        &pipe_match_spec,
                                        entry_handle,
      false /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t Phase0Table::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi_handle_t &entry_handle,
                                           tdi::TableKey *key,
                                           tdi::TableData *data) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = entryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t Phase0Table::entryGetFirst(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                tdi::TableKey *key,
                                                tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  int pipe_entry_hdl = 0;
  tdi_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), table_context_info->tableHdlGet(), pipe_dev_tgt, &pipe_entry_hdl);
  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  match_key->populate_match_spec(&pipe_match_spec);

  Phase0TableData *match_data = static_cast<Phase0TableData *>(data);
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data->get_pipe_action_spec();
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         table_context_info->tableHdlGet(),
                         pipe_entry_hdl,
                         PIPE_RES_GET_FLAG_ENTRY,
                         &pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  tdi_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data->actionIdSet(action_id);
  std::vector<tdi_id_t> empty;
  match_data->setActiveFields(empty);

  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::entryGetNext_n(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const tdi::Flags &flags,
                                                 const tdi::TableKey &key,
                                                 const uint32_t &n,
                                                 tdi::Table::keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        table_context_info->tableHdlGet(),
                                        &pipe_match_spec,
                                        &pipe_entry_hdl,
      true /* light_pipe_validation */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                               table_context_info->tableHdlGet(),
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }

  uint32_t i = 0;
  std::vector<tdi_id_t> empty;
  for (i = 0; i < n; i++) {
    memset(&pipe_match_spec, 0, sizeof(pipe_match_spec));
    auto this_key =
        static_cast<MatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<Phase0TableData *>((*key_data_pairs)[i].second);
    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
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
                           table_context_info->tableHdlGet(),
                           next_entry_handles[i],
                           PIPE_RES_GET_FLAG_ENTRY,
                           &pipe_match_spec,
                           pipe_action_spec,
                           &pipe_act_fn_hdl,
                           nullptr);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
          "err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          pipe_entry_hdl,
          status);
      return status;
    }
    tdi_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
    this_data->actionIdSet(action_id);
    this_data->setActiveFields(empty);
  }

  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t Phase0Table::usageGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t Phase0Table::dataReset(tdi::TableData *data) const {
  Phase0TableData *data_obj = static_cast<Phase0TableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  return data_obj->reset(action_id);
}

// SnapshotConfigTable
tdi_status_t SnapshotConfigTable::entryModInternal(
    const tdi::Session &session,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    const pipe_snapshot_hdl_t &entry_hdl,
    const tdi::TableData &data) const {
  tdi_status_t sts = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotConfigTableData &sc_data =
      static_cast<const SnapshotConfigTableData &>(data);
  tdi_id_t field_id;

  // Set the snapshot timer setting
  sts = this->dataFieldIdGet("timer_enable", &field_id);
  if (sts != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return sts;
  }

  bool timer_en;
  sts = sc_data.getValue(field_id, &timer_en);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  sts = this->dataFieldIdGet("ingress_trigger_mode", &field_id);
  if (sts != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return sts;
  }
  std::string ig_mode;
  sts = sc_data.getValue(field_id, &ig_mode);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  // For timer disable - true, enable - false.
  // Mode input validation done on data object level
  sts = pipeMgr->bfSnapshotCfgSet(entry_hdl, !timer_en, snapIgMode.at(ig_mode));
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in snapshot config setting",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }

  // Set the snapshot timer timestamp value
  sts = this->dataFieldIdGet("timer_value_usecs", &field_id);
  if (sts) {
    TDI_DBGCHK(0);
    return sts;
  }
  uint64_t timer_val;
  sts = sc_data.getValue(field_id, &timer_val);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  sts = pipeMgr->bfSnapshotStateSet(
      entry_hdl, TDI_SNAPSHOT_ST_DISABLED, timer_val);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in configuring snapshot timer value",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }
  return sts;
}

tdi_status_t SnapshotConfigTable::entryAdd(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  tdi_status_t sts = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotConfigTableKey &sc_key =
      static_cast<const SnapshotConfigTableKey &>(key);
  const SnapshotConfigTableData &sc_data =
      static_cast<const SnapshotConfigTableData &>(data);
  uint32_t start_stage, end_stage;
  sc_key.get_stage_id(&start_stage, &end_stage);
  std::string thread;
  bf_snapshot_dir_t dir;
  tdi_id_t field_id;

  sts = this->dataFieldIdGet("thread", &field_id);
  if (sts) {
    TDI_DBGCHK(0);
    return sts;
  }
  sts = sc_data.getValue(field_id, &thread);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  if (!thread.compare("INGRESS")) {
    dir = TDI_SNAPSHOT_DIR_INGRESS;
  } else if (!thread.compare("EGRESS")) {
    dir = TDI_SNAPSHOT_DIR_EGRESS;
  } else {
    LOG_TRACE("Invalid thread string provided");
    return TDI_INVALID_ARG;
  }

  // Create the snapshot
  pipe_snapshot_hdl_t entry_hdl;
  sts = pipeMgr->bfSnapshotCreate(
      dev_tgt.dev_id, dev_tgt.pipe_id, start_stage, end_stage, dir, &entry_hdl);
  if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }

  sts = this->entryModInternal(session, dev_tgt, flags, entry_hdl, data);
  if (sts != TDI_SUCCESS) {
    pipeMgr->bfSnapshotDelete(entry_hdl);
    return sts;
  }

  return sts;
}

tdi_status_t SnapshotConfigTable::entryDel(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotConfigTableKey &match_key =
      static_cast<const SnapshotConfigTableKey &>(key);
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }

  int num_trig = 0;
  status = pipeMgr->bfSnapshotNumTrigFieldsGet(entry_hdl, &num_trig);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting entry info",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }

  if (num_trig != 0) {
    LOG_ERROR(
        "Triggers related to this entry still exist and must be removed, "
        "before snapshot instance is deleted. Number of triggers %d",
        num_trig);
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->bfSnapshotDelete(entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %s",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_str_err(pipe_status_t(status)));
    return status;
  }
  return status;
}

tdi_status_t SnapshotConfigTable::setDataFields(
    const bf_snapshot_dir_t dir,
    const bool timer_en,
    const uint32_t timer_val,
    const std::vector<int> &pipes,
    const bf_snapshot_ig_mode_t ig_mode,
    SnapshotConfigTableData *data) const {
  // Set thread
  tdi_id_t field_id;
  auto status = this->dataFieldIdGet("thread", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  switch (dir) {
    case TDI_SNAPSHOT_DIR_INGRESS:
      status = data->setValue(field_id, std::string("INGRESS"));
      break;
    case TDI_SNAPSHOT_DIR_EGRESS:
      status = data->setValue(field_id, std::string("EGRESS"));
      break;
    default:
      LOG_ERROR("Unsupported direction configured in snapshot");
      TDI_DBGCHK(0);
      return TDI_UNEXPECTED;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  // Set timer fields
  status = this->dataFieldIdGet("timer_enable", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = data->setValue(field_id, timer_en);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  status = this->dataFieldIdGet("timer_value_usecs", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = data->setValue(field_id, static_cast<uint64_t>(timer_val));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  // Set captured pipes per thread, iterator "i" overlaps with
  // bf_snapshot_thread_t and field names are in the same order.
  std::vector<std::string> fields = {
      "ingress_capture", "egress_capture", "ghost_capture"};
  for (uint32_t i = 0; i < fields.size(); i++) {
    status = this->dataFieldIdGet(fields[i], &field_id);
    if (status != TDI_SUCCESS) {
      TDI_DBGCHK(0);
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
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
    } else {
      data->removeActiveField({field_id});
    }
  }

  status = this->dataFieldIdGet("ingress_trigger_mode", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  for (const auto &ig_mode_pair : snapIgMode) {
    if (ig_mode_pair.second == ig_mode) {
      status = data->setValue(field_id, ig_mode_pair.first);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  field_id);
        return status;
      }
    }
  }
  return status;
}

tdi_status_t SnapshotConfigTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  pipe_snapshot_hdl_t entry_hdl;
  uint32_t start_stage, end_stage;
  tdi_status_t status;
  uint32_t num_pipes;

  const SnapshotConfigTableKey &sc_key =
      static_cast<const SnapshotConfigTableKey &>(key);
  SnapshotConfigTableData *sc_data =
      static_cast<SnapshotConfigTableData *>(data);
  sc_key.get_stage_id(&start_stage, &end_stage);

  status = this->entryHandleGet(session, dev_tgt, flags, key, &entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot config values",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get number of pipes",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  std::vector<int> thread_mask(num_pipes);
  status = pipeMgr->bfSnapshotThreadGet(
      entry_hdl, thread_mask.size(), thread_mask.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get captured thread values",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  bf_snapshot_dir_t dir;
  status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_hdl, NULL, NULL, NULL, NULL, &dir);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry params, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  status =
      this->setDataFields(dir, timer_en, timer_val, thread_mask, mode, sc_data);

  return status;
}

tdi_status_t SnapshotConfigTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  SnapshotConfigTableKey *match_key =
      static_cast<SnapshotConfigTableKey *>(key);

  uint8_t start_stage, end_stage = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_snapshot_dir_t gress;
  auto status = pipeMgr->bfSnapshotEntryParamsGet(entry_handle,
                                                  &entry_tgt->dev_id,
                                                  &entry_tgt->pipe_id,
                                                  &start_stage,
                                                  &end_stage,
                                                  &gress);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  match_key->set_stage_id(start_stage, end_stage);
  return status;
}

tdi_status_t SnapshotConfigTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const SnapshotConfigTableKey &match_key =
      static_cast<const SnapshotConfigTableKey &>(key);
  uint32_t start_stage, end_stage;
  match_key.get_stage_id(&start_stage, &end_stage);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  bf_snapshot_dir_t dir;
  tdi_status_t status = pipeMgr->bfSnapshotHandleGet(dev_tgt.dev_id,
                                                    dev_tgt.pipe_id,
                                                    start_stage,
                                                    end_stage,
                                                    &dir,
                                                    entry_handle);
  if (status != TDI_SUCCESS && dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
    /* The snapshot might have been created for all pipes */
    status = pipeMgr->bfSnapshotHandleGet(dev_tgt.dev_id,
                                          TDI_DEV_PIPE_ALL,
                                          start_stage,
                                          end_stage,
                                          &dir,
                                          entry_handle);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
    }
  }
  return status;
}

tdi_status_t SnapshotConfigTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi_handle_t &entry_handle,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  uint8_t start_stage, end_stage = 0;
  SnapshotConfigTableKey *sc_key =
      static_cast<SnapshotConfigTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_handle,
      static_cast<tdi_dev_id_t *>(nullptr),
      static_cast<bf_dev_pipe_t *>(nullptr),
      &start_stage,
      &end_stage,
      static_cast<bf_snapshot_dir_t *>(nullptr));
  if (status == TDI_SUCCESS) {
    sc_key->set_stage_id(start_stage, end_stage);

    status = this->entryGet(session, dev_tgt, flags, *key, data);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t SnapshotConfigTable::clear(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  tdi_status_t status = pipeMgr->bfSnapshotClear(dev_tgt.dev_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing snapshot table err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t SnapshotConfigTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<TableKey>(new SnapshotConfigTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotConfigTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret = std::unique_ptr<tdi::TableData>(
      new SnapshotConfigTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotConfigTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  tdi_status_t status;
  uint32_t start_stage = 0, end_stage = this->getSize();
  *num_returned = 0;

  const SnapshotConfigTableKey &sc_key =
      static_cast<const SnapshotConfigTableKey &>(key);
  sc_key.get_stage_id(&start_stage, &end_stage);

  pipe_snapshot_hdl_t entry_handle;
  status =
      this->entryHandleGet(session, dev_tgt, flags, key, &entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get snapshot entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  std::vector<int> next_hdls(n, 0);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotNextEntryHandlesGet(
      dev_tgt.dev_id, entry_handle, n, next_hdls.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot handles, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  for (uint32_t i = 0; i < n; i++) {
    auto this_key =
        static_cast<SnapshotConfigTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    if (next_hdls[i] == 0) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    status = this->entryGet(
        session, dev_tgt, flags, next_hdls[i], this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
  }

  return status;
}

tdi_status_t SnapshotConfigTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  pipe_snapshot_hdl_t entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  status = pipeMgr->bfSnapshotFirstHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot stages, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  return this->entryGet(session, dev_tgt, flags, entry_hdl, key, data);
}

tdi_status_t SnapshotConfigTable::usageGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->bfSnapshotUsageGet(dev_tgt.dev_id, dev_tgt.pipe_id, count);
}

// SnapshotTrigTable
tdi_status_t SnapshotTriggerTable::getHandle(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint32_t &start_stage,
    pipe_snapshot_hdl_t *entry_hdl) const {
  tdi_status_t status;
  pipe_snapshot_hdl_t hdl;
  bf_snapshot_dir_t dir;
  uint32_t end_stage = start_stage;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  do {
    status = pipeMgr->bfSnapshotHandleGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, start_stage, end_stage, &dir, &hdl);
    end_stage++;
  } while (status != TDI_SUCCESS && end_stage < this->getSize());
  if (status != TDI_SUCCESS || dir != this->direction) {
    // This is expected to happen when iterating over table entries
    return TDI_OBJECT_NOT_FOUND;
  }
  *entry_hdl = hdl;
  return TDI_SUCCESS;
}

tdi_status_t SnapshotTriggerTable::entryModInternal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotTriggerTableKey &st_key =
      static_cast<const SnapshotTriggerTableKey &>(key);
  const SnapshotTriggerTableData &st_data =
      static_cast<const SnapshotTriggerTableData &>(data);
  uint32_t start_stage;
  status = st_key.get_stage_id(&start_stage);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in configuring snapshot timer value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  pipe_snapshot_hdl_t entry_hdl;
  status = this->getHandle(session, dev_tgt, start_stage, &entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot find related snapshot instance",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  std::vector<tdi_id_t> fields;
  status = this->dataFieldIdListGet(&fields);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot fetch field ID list for snapshot trigger table",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_UNEXPECTED;
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
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field name for field %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }

    // Skip fixed fields
    if (!field_name.compare("enable") || !field_name.compare("trigger_state") ||
        field_name.find(sufix) == field_name.length() - sufix.length()) {
      continue;
    }

    tdi_id_t field_mask_id;
    status =
        this->dataFieldIdGet(std::string(field_name + sufix), &field_mask_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    uint64_t mask = 0, value = 0;
    size_t field_size = 0;
    status = this->dataFieldSizeGet(field_id, &field_size);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field size for %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_name.c_str());
      return status;
    }
    field_size = (field_size + 7) / 8;

    std::vector<uint8_t> v(field_size);
    std::vector<uint8_t> m(field_size);
    // If there is no data for specific filed continue
    status = st_data.getValue(field_mask_id, m.size(), m.data());
    if (status == TDI_OBJECT_NOT_FOUND) continue;
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting value for field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    status = st_data.getValue(field_id, v.size(), v.data());
    if (status == TDI_OBJECT_NOT_FOUND) continue;
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting value for field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name).c_str());
      return status;
    }

    // Convert vector to uint64 as triggers do not support fields > 64
    auto offset = (field_size > sizeof(value)) ? v.size() - sizeof(value) : 0;
    auto size = (field_size > sizeof(value)) ? sizeof(value) : field_size;
    EndiannessHandler::toHostOrder(size, v.data() + offset, &value);
    EndiannessHandler::toHostOrder(size, m.data() + offset, &mask);

    // Delete all $
    std::string::size_type i = field_name.find('$');
    while (i != std::string::npos) {
      field_name.erase(i, 1);
      i = field_name.find('$');
    }
    // replace all '.' to '_'
    std::replace(field_name.begin(), field_name.end(), '.', '_');
    // remove "trig." prefix
    field_name.erase(0, 5);
    status = pipeMgr->bfSnapshotCaptureTriggerFieldAdd(
        entry_hdl, (char *)(field_name).c_str(), value, mask);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in adding field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name).c_str());
      return status;
    }
  }

  // Process fixed fields
  tdi_id_t field_id;
  bool enable = false;
  status = this->dataFieldIdGet("enable", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = st_data.getValue(field_id, &enable);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  bf_snapshot_state_t state =
      (enable) ? TDI_SNAPSHOT_ST_ENABLED : TDI_SNAPSHOT_ST_DISABLED;
  status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  return status;
}

tdi_status_t SnapshotTriggerTable::entryAdd(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  return this->entryModInternal(session, dev_tgt, flags, key, data);
}

tdi_status_t SnapshotTriggerTable::entryMod(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  return this->entryModInternal(session, dev_tgt, flags, key, data);
}

tdi_status_t SnapshotTriggerTable::entryDel(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotTriggerTableKey &st_key =
      static_cast<const SnapshotTriggerTableKey &>(key);
  uint32_t start_stage;
  st_key.get_stage_id(&start_stage);
  pipe_snapshot_hdl_t entry_hdl;
  status = this->getHandle(session, dev_tgt, start_stage, &entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }

  // Disable snapshot
  bool timer_en = false;
  uint32_t timer_val = 0;
  bf_snapshot_ig_mode_t mode;
  status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  bf_snapshot_state_t state = TDI_SNAPSHOT_ST_DISABLED;
  status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  // Clear trigger fields
  status = pipeMgr->bfSnapshotCaptureTriggerFieldsClr(entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing snapshot trigger table err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t SnapshotTriggerTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  const SnapshotTriggerTableKey &st_key =
      static_cast<const SnapshotTriggerTableKey &>(key);
  SnapshotTriggerTableData *st_data =
      static_cast<SnapshotTriggerTableData *>(data);
  uint32_t stage;
  st_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
  if (status != TDI_SUCCESS && dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
    tdi::Target dev_tgt_all = dev_tgt;
    dev_tgt_all.pipe_id = TDI_DEV_PIPE_ALL;
    // The snapshot might have been created for all pipes
    status = this->getHandle(session, dev_tgt_all, stage, &entry_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
    }
  }

  // Populate fields to the snapshot data
  std::vector<tdi_id_t> fields;
  status = this->dataFieldIdListGet(&fields);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Cannot fetch field ID list for snapshot trigger table",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_UNEXPECTED;
  }
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  std::string sufix("_mask");
  // Process trigger fields
  for (auto it = fields.begin(); it != fields.end(); ++it) {
    auto field_id = *it;
    std::string field_name;
    status = this->dataFieldNameGet(field_id, &field_name);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field name for field %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }

    // Skip fixed fields and masks
    if (!field_name.compare("enable") || !field_name.compare("trigger_state") ||
        field_name.find(sufix) == field_name.length() - sufix.length()) {
      continue;
    }

    tdi_id_t field_mask_id;
    status =
        this->dataFieldIdGet(std::string(field_name + sufix), &field_mask_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field id for %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    if (!st_data->checkFieldActive(field_id)) {
      // Disable mask if field is not active
      st_data->removeActiveField({field_mask_id});
      continue;
    }

    uint64_t mask = 0, value = 0;
    size_t field_size = 0;
    status = this->dataFieldSizeGet(field_id, &field_size);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get the field size for %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_name.c_str());
      return status;
    }
    field_size = (field_size + 7) / 8;

    // Delete all $
    std::string::size_type i = field_name.find('$');
    while (i != std::string::npos) {
      field_name.erase(i, 1);
      i = field_name.find('$');
    }
    // replace all '.' to '_'
    std::replace(field_name.begin(), field_name.end(), '.', '_');
    // remove "trig." prefix
    field_name.erase(0, 5);
    status = pipeMgr->bfSnapshotCaptureTriggerFieldGet(
        entry_hdl, (char *)(field_name).c_str(), &value, &mask);
    if (TDI_OBJECT_NOT_FOUND == status) {
      st_data->removeActiveField({field_mask_id, field_id});
      continue;
    }
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in getting field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name).c_str());
      return status;
    }

    // Convert vector to uint64 as triggers do not support fields > 64
    std::vector<uint8_t> v(field_size);
    std::vector<uint8_t> m(field_size);
    auto offset = (field_size > sizeof(value)) ? v.size() - sizeof(value) : 0;
    auto size = (field_size > sizeof(value)) ? sizeof(value) : field_size;
    EndiannessHandler::toNetworkOrder(size, value, v.data() + offset);
    EndiannessHandler::toNetworkOrder(size, mask, m.data() + offset);

    status = st_data->setValue(field_mask_id, m.data(), size);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error setting value for field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name + sufix).c_str());
      return status;
    }

    status = st_data->setValue(field_id, v.data(), size);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error setting value for field %s",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                (field_name).c_str());
      return status;
    }
  }

  // Process fixed fields
  bool state;
  uint32_t num_pipes = 0;
  status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get numper of active pipes",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  std::vector<pipe_snapshot_fsm_state_t> trig_state(num_pipes);
  status = pipeMgr->bfSnapshotStateGet(
      entry_hdl, trig_state.size(), trig_state.data(), &state);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  // If only one pipe was requested, trim down the result vector
  if (dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
    trig_state.erase(trig_state.begin(), trig_state.begin() + dev_tgt.pipe_id);
    trig_state.erase(trig_state.begin() + 1, trig_state.end());
  }

  tdi_id_t field_id;
  status = this->dataFieldIdGet("enable", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }

  status = st_data->setValue(field_id, state);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
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
                  this->tableInfoGet()->nameGet().c_str(),
                  *it);
        TDI_DBGCHK(0);
        return TDI_UNEXPECTED;
    }
  }
  status = this->dataFieldIdGet("trigger_state", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = st_data->setValue(field_id, trig_states);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }

  return status;
}

tdi_status_t SnapshotTriggerTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  SnapshotTriggerTableKey *match_key =
      static_cast<SnapshotTriggerTableKey *>(key);

  uint8_t start_stage, end_stage = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_snapshot_dir_t gress;
  auto status = pipeMgr->bfSnapshotEntryParamsGet(entry_handle,
                                                  &entry_tgt->dev_id,
                                                  &entry_tgt->pipe_id,
                                                  &start_stage,
                                                  &end_stage,
                                                  &gress);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  match_key->set_stage_id(start_stage);
  return status;
}

tdi_status_t SnapshotTriggerTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const SnapshotTriggerTableKey &st_key =
      static_cast<const SnapshotTriggerTableKey &>(key);
  uint32_t stage = 0;
  st_key.get_stage_id(&stage);
  auto status = this->getHandle(session, dev_tgt, stage, entry_handle);

  if (status != TDI_SUCCESS && dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
    tdi::Target dev_tgt_all = dev_tgt;
    dev_tgt_all.pipe_id = TDI_DEV_PIPE_ALL;
    // The snapshot might have been created for all pipes
    status = this->getHandle(session, dev_tgt_all, stage, entry_handle);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Entry does not exist",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
    }
  }
  return status;
}

tdi_status_t SnapshotTriggerTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi_handle_t &entry_handle,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  uint8_t start_stage, end_stage = 0;
  SnapshotTriggerTableKey *st_key =
      static_cast<SnapshotTriggerTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status = pipeMgr->bfSnapshotEntryParamsGet(
      entry_handle,
      static_cast<tdi_dev_id_t *>(nullptr),
      static_cast<bf_dev_pipe_t *>(nullptr),
      &start_stage,
      &end_stage,
      static_cast<bf_snapshot_dir_t *>(nullptr));
  if (status == TDI_SUCCESS) {
    st_key->set_stage_id(start_stage);

    status = this->entryGet(session, dev_tgt, flags, *key, data);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t SnapshotTriggerTable::clear(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_snapshot_hdl_t entry_hdl = 0;

  for (uint32_t st_stg = 0; st_stg < this->getSize(); st_stg++) {
    auto status = this->getHandle(session, dev_tgt, st_stg, &entry_hdl);
    if (status != TDI_SUCCESS) {
      // Expected to happen when iterating over all possible entries.
      continue;
    }

    // Disable snapshot
    bool timer_en = false;
    uint32_t timer_val = 0;
    bf_snapshot_ig_mode_t mode;
    status = pipeMgr->bfSnapshotCfgGet(entry_hdl, &timer_en, &timer_val, &mode);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get snapshot timer values",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
      return status;
    }
    bf_snapshot_state_t state = TDI_SNAPSHOT_ST_DISABLED;
    status = pipeMgr->bfSnapshotStateSet(entry_hdl, state, timer_val);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to update snapshot state",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
      return status;
    }

    status = pipeMgr->bfSnapshotCaptureTriggerFieldsClr(entry_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in Clearing snapshot trigger table err %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                status);
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotTriggerTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<TableKey>(new SnapshotTriggerTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotTriggerTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SnapshotTriggerTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}
tdi_status_t SnapshotTriggerTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  tdi_status_t status = TDI_SUCCESS;
  uint32_t stage = 0;
  *num_returned = 0;

  const SnapshotTriggerTableKey &sc_key =
      static_cast<const SnapshotTriggerTableKey &>(key);
  sc_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;

  for (uint32_t i = 0; i < n; i++) {
    // Find next valid stage
    stage++;
    status = TDI_OBJECT_NOT_FOUND;
    for (; stage < this->getSize(); stage++) {
      status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
      if (status != TDI_SUCCESS && dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
        tdi::Target wrk_tgt = dev_tgt;
        wrk_tgt.pipe_id = TDI_DEV_PIPE_ALL;
        status = this->getHandle(session, wrk_tgt, stage, &entry_hdl);
      }
      if (status == TDI_SUCCESS) break;
    }
    auto this_key =
        static_cast<SnapshotTriggerTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          this->tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }

    if (status != TDI_SUCCESS) {
      (*key_data_pairs)[i].second = nullptr;
      if (status == TDI_OBJECT_NOT_FOUND) status = TDI_SUCCESS;
      continue;
    }

    status = this->entryGet(
        session, dev_tgt, flags, entry_hdl, this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
  }

  return status;
}

tdi_status_t SnapshotTriggerTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  pipe_snapshot_hdl_t entry_hdl;
  for (uint32_t stage = 0; stage < this->getSize(); stage++) {
    status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
    if (status == TDI_SUCCESS) break;
    if (dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
      tdi::Target wrk_tgt = dev_tgt;
      wrk_tgt.pipe_id = TDI_DEV_PIPE_ALL;
      status = this->getHandle(session, wrk_tgt, stage, &entry_hdl);
      if (status == TDI_SUCCESS) break;
    }
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Snapshot entry not found, err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return this->entryGet(session, dev_tgt, flags, entry_hdl, key, data);
}

// SnapshotData

void SnapshotDataTable::entryDataSetControlInfoNextTables(
    tdi_dev_id_t dev_id,
    const tdi_id_t field_id,
    const char (&next_tbl_names)[TDI_MAX_LOG_TBLS * TDI_TBL_NAME_LEN],
    SnapshotDataTableData *data) const {
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

void SnapshotDataTable::entryGetControlInfo(
    const tdi::Session &session,
    const pipe_snapshot_hdl_t &entry_hdl,
    const uint32_t &stage,
    const bf_snapshot_capture_ctrl_info_arr_t &ctrl_info_arr,
    const int & /*num_captures*/,
    SnapshotDataTableData *data) const {
  tdi_status_t status = TDI_SUCCESS;

  bf_snapshot_dir_t thread;
  dev_stage_t first_stage;
  tdi_dev_id_t dev_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status =
      pipeMgr->bfSnapshotEntryParamsGet(entry_hdl,
                                        &dev_id,
                                        static_cast<bf_dev_pipe_t *>(nullptr),
                                        &first_stage,
                                        static_cast<dev_stage_t *>(nullptr),
                                        &thread);

  std::vector<tdi_id_t> dataFields;
  // Get the container fields for this field-id
  status = dataFieldIdListGet(&dataFields);
  if (TDI_SUCCESS != status) {
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
    tdi_id_t field_id = *it;
    const tdi::DataFieldInfo *dataField;
    status = dataFieldGet(field_id, &dataField);
    if (TDI_SUCCESS != status) {
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
      std::vector<tdi_id_t> c_dataFields;
      // Get the container fields for meter_alu_info
      status = containerDataFieldIdListGet(field_id, &c_dataFields);
      if (TDI_SUCCESS != status) {
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
        std::unique_ptr<SnapshotDataTableData> tbl_info_obj(
            new SnapshotDataTableData(this));
        // Go over all fields for meter_alu_info
        for (auto c_it = c_dataFields.begin(); c_it != c_dataFields.end();
             ++c_it) {
          tdi_id_t c_field_id = *c_it;
          const tdi::DataFieldInfo *c_dataField;
          status = dataFieldGet(c_field_id, &c_dataField);
          if (TDI_SUCCESS != status) {
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
      std::vector<tdi_id_t> c_dataFields;
      // Get the container fields for tbl_info
      status = containerDataFieldIdListGet(field_id, &c_dataFields);
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d Error in getting data field id list for id %u",
                  __func__,
                  __LINE__,
                  field_id);
        continue;
      }

      for (uint32_t tbl_idx = 0; tbl_idx < TDI_MAX_LOG_TBLS; tbl_idx++) {
        // Skip empty entries. Set at least one value for tbl_info so
        // that grpc does not complain
        if ((tbl_idx != 0) && ctrl->tables_info[tbl_idx].table_handle == 0) {
          continue;
        }
        std::unique_ptr<SnapshotDataTableData> tbl_info_obj(
            new SnapshotDataTableData(this));
        // Go over all fields for tbl_info
        for (auto c_it = c_dataFields.begin(); c_it != c_dataFields.end();
             ++c_it) {
          tdi_id_t c_field_id = *c_it;
          const tdi::DataFieldInfo *c_dataField;
          status = dataFieldGet(c_field_id, &c_dataField);
          if (TDI_SUCCESS != status) {
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
                                        TDI_TBL_NAME_LEN));
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
      this->entryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_next_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_GBL_EXECUTE_TABLES) !=
               types.end()) {
      this->entryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->gbl_exec_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_ENABLED_GBL_EXECUTE_TABLES) !=
               types.end()) {
      this->entryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_gbl_exec_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_LONG_BRANCH_TABLES) !=
               types.end()) {
      this->entryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->long_branch_tbl_names, data);
    } else if (types.find(DataFieldType::SNAPSHOT_ENABLED_LONG_BRANCH_TABLES) !=
               types.end()) {
      this->entryDataSetControlInfoNextTables(
          dev_id, field_id, ctrl->enabled_long_branch_tbl_names, data);
    } else {
      // unknown field
      field_value = 0;
      data->setValue(field_id, field_value);
    }
  }  // For all fields

  return;
}

tdi_status_t SnapshotDataTable::entryGetFieldInfo(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_snapshot_hdl_t &entry_hdl,
    const uint32_t &stage,
    uint8_t *capture,
    const int num_captures,
    SnapshotDataTableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  std::vector<tdi_id_t> dataFields;
  // Get the container fields for this field-id
  status = this->dataFieldIdListGet(&dataFields);
  if (TDI_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in getting data field id list", __func__, __LINE__);
    return status;
  }
  bf_snapshot_dir_t thread;
  status =
      pipeMgr->bfSnapshotEntryParamsGet(entry_hdl,
                                        static_cast<tdi_dev_id_t *>(nullptr),
                                        static_cast<bf_dev_pipe_t *>(nullptr),
                                        static_cast<dev_stage_t *>(nullptr),
                                        static_cast<dev_stage_t *>(nullptr),
                                        &thread);
  if (TDI_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in getting snapshot params", __func__, __LINE__);
    return status;
  }
  // Go over all fields for this stage
  for (auto it = dataFields.begin(); it != dataFields.end(); ++it) {
    tdi_id_t field_id = *it;
    const tdi::DataFieldInfo *dataField;
    status = dataFieldGet(field_id, &dataField);
    if (TDI_SUCCESS != status) {
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
    // Delete all $
    std::string::size_type i = field_name.find('$');
    while (i != std::string::npos) {
      field_name.erase(i, 1);
      i = field_name.find('$');
    }
    // replace all '.' to '_'
    std::replace(field_name.begin(), field_name.end(), '.', '_');

    bool exists = false;
    status =
        pipeMgr->bfSnapshotFieldInScope(dev_tgt.dev_id,
                                        dev_tgt.pipe_id,
                                        stage,
                                        thread,
                                        const_cast<char *>(field_name.c_str()),
                                        &exists);
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in checking field scope for field %s in "
          "stage %d",
          __func__,
          __LINE__,
          this->tableInfoGet()->nameGet().c_str(),
          field_name.c_str(),
          stage);
      return status;
    }
    if (!exists) {
      data->removeActiveField({field_id});
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

    if (TDI_SUCCESS != status) {
      // This may happen in case when there is slight difference between field
      // dictionary and actual PHV values in complex P4 programs.
      // Warn only for now.
      LOG_WARN("%s:%d %s: Error in decoding captured field %s in stage %d",
               __func__,
               __LINE__,
               this->tableInfoGet()->nameGet().c_str(),
               field_name.c_str(),
               stage);
      field_valid = false;
    }

    if (!field_valid) {
      data->removeActiveField({field_id});
      continue;
    }

    // For byte streams change byte order since value fetched is uint64_t
    // All fields here are expected to be byte streams
    std::vector<uint8_t> v(field_size);
    auto offset =
        (field_size > sizeof(field_value)) ? v.size() - sizeof(field_value) : 0;
    auto size =
        (field_size > sizeof(field_value)) ? sizeof(field_value) : field_size;
    EndiannessHandler::toNetworkOrder(size, field_value, v.data() + offset);
    status = data->setValue(field_id, v.data(), field_size);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
  }  // For all fields

  return status;
}

tdi_status_t SnapshotDataTable::getHandle(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint32_t &stage,
    pipe_snapshot_hdl_t *entry_hdl) const {
  tdi_status_t status;
  pipe_snapshot_hdl_t hdl;
  uint32_t num_pipes;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  if (dev_tgt.pipe_id != TDI_DEV_PIPE_ALL) {
    // Verify pipe numer to avoid exccesive errors
    status = pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipes);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get number of pipes",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
      return status;
    }
    if (num_pipes <= dev_tgt.pipe_id) {
      LOG_TRACE("%s:%d %s : Error : Invalid pipe numer : %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                dev_tgt.pipe_id);
      return TDI_INVALID_ARG;
    }
  }

  // Get next with handle '0' will return all instances created on all pipes
  std::vector<int> hdl_list(this->_table_size * num_pipes, 0);
  status = pipeMgr->bfSnapshotNextEntryHandlesGet(
      dev_tgt.dev_id, 0, hdl_list.size(), hdl_list.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Cannot get list of snapshot handles, err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  status = TDI_OBJECT_NOT_FOUND;
  int i = 0;
  hdl = hdl_list[i++];
  while (hdl != 0) {
    dev_stage_t start_stage, end_stage;
    bf_dev_pipe_t pipe_id;
    tdi_dev_id_t dev_id;
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
        (pipe_id == dev_tgt.pipe_id || pipe_id == TDI_DEV_PIPE_ALL) &&
        dev_id == dev_tgt.dev_id) {
      status = TDI_SUCCESS;
      break;
    }
    hdl = hdl_list[i++];
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  *entry_hdl = hdl;
  return TDI_SUCCESS;
}

tdi_status_t SnapshotDataTable::entryGet(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const tdi::Flags & /*flags*/,
                                                 const tdi::TableKey &key,
                                                 tdi::TableData *data) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  const SnapshotDataTableKey &sd_key =
      static_cast<const SnapshotDataTableKey &>(key);
  SnapshotDataTableData *sd_data =
      static_cast<SnapshotDataTableData *>(data);
  uint32_t stage;
  sd_key.get_stage_id(&stage);

  pipe_snapshot_hdl_t entry_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  uint32_t total_size = 0, stage_size = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfSnapshotCapturePhvFieldsDictSize(
      entry_hdl, &total_size, &stage_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Failed to get phv dict size",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading snapshot capture for entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  status = this->entryGetFieldInfo(session,
                                        dev_tgt,
                                        entry_hdl,
                                        stage,
                                        capture.data(),
                                        num_captures,
                                        sd_data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error when setting snapshot data fields",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  this->entryGetControlInfo(
      session, entry_hdl, stage, ctrl_info_arr, num_captures, sd_data);
  return status;
}

tdi_status_t SnapshotDataTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  SnapshotDataTableKey *sd_key =
      static_cast<SnapshotDataTableKey *>(key);

  // This is indexed table, so handle is the same as stage id in the key
  uint32_t stage = entry_handle;
  // Check if snapshot instance covering this handle/stage exist
  pipe_snapshot_hdl_t snap_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  sd_key->set_stage_id(stage);
  return status;
}

tdi_status_t SnapshotDataTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  const SnapshotDataTableKey &sd_key =
      static_cast<const SnapshotDataTableKey &>(key);
  uint32_t stage = 0;
  sd_key.get_stage_id(&stage);
  // Check if snapshot instance covering this handle/stage exist
  pipe_snapshot_hdl_t snap_hdl;
  auto status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Entry does not exist",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
  }

  // This is indexed table, so handle is the same as stage id in the key
  *entry_handle = stage;

  return status;
}

tdi_status_t SnapshotDataTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi_handle_t &entry_handle,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  SnapshotDataTableKey *sd_key =
      static_cast<SnapshotDataTableKey *>(key);
  // This is indexed table, so handle is the same as stage id in the key
  sd_key->set_stage_id(entry_handle);

  return this->entryGet(session, dev_tgt, flags, *key, data);
}

tdi_status_t SnapshotDataTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new SnapshotDataTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotDataTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SnapshotDataTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}
tdi_status_t SnapshotDataTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  tdi_status_t status = TDI_OBJECT_NOT_FOUND;
  *num_returned = 0;

  auto first_key = static_cast<const SnapshotDataTableKey &>(key);
  uint32_t start_stage;
  uint8_t end_stage = this->_table_size;
  first_key.get_stage_id(&start_stage);
  pipe_snapshot_hdl_t snap_hdl;
  uint32_t i = 0;
  for (uint32_t stage = start_stage + 1; stage < end_stage && i < n; stage++) {
    // Check if stage is valid
    status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
    if (status != TDI_SUCCESS) {
      continue;
    }
    auto this_key =
        static_cast<SnapshotDataTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    this_key->set_stage_id(stage);

    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match the table",
          __func__,
          __LINE__,
          this->tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }

    status = this->entryGet(
        session, dev_tgt, flags, *(*key_data_pairs)[i].first, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
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

tdi_status_t SnapshotDataTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  if (dev_tgt.pipe_id == TDI_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data table do not support calls"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  tdi_status_t status = TDI_SUCCESS;

  // Find first valid stage
  pipe_snapshot_hdl_t snap_hdl;
  uint32_t stage;
  for (stage = 0; stage < this->_table_size; stage++) {
    status = this->getHandle(session, dev_tgt, stage, &snap_hdl);
    if (status == TDI_SUCCESS) break;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR : Snapshot data doesn't have any entries"
        " for all pipes",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  auto sd_key = static_cast<SnapshotDataTableKey *>(key);
  sd_key->set_stage_id(stage);

  return this->entryGet(session, dev_tgt, flags, *key, data);
}

tdi_status_t SnapshotDataTable::usageGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  // This is the only call that is allowed without pipe argument
  // Fixed table, return table size
  *count = this->_table_size;
  return TDI_SUCCESS;
}

// Snapshot Liveness
tdi_status_t SnapshotLivenessTable::usageGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  /* Table usage do not apply to snapshot. */
  *count = 0;
  return TDI_SUCCESS;
}

tdi_status_t SnapshotLivenessTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  /* TODO: Hardcoded end_stage */
  uint32_t start_stage = 0, end_stage = 20, stage_val = 0;
  tdi_status_t sts = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SnapshotLivenessTableKey &match_key =
      static_cast<const SnapshotLivenessTableKey &>(key);
  SnapshotLivenessTableData &match_data =
      static_cast<SnapshotLivenessTableData &>(*data);

  /* Get the field name */
  std::string f_name;
  sts = match_key.getFieldName(&f_name);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR getting field name",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }

  // Delete all $
  std::string::size_type i = f_name.find('$');
  while (i != std::string::npos) {
    f_name.erase(i, 1);
    i = f_name.find('$');
  }
  // replace all '.' to '_'
  std::replace(f_name.begin(), f_name.end(), '.', '_');

  tdi_id_t field_id;
  sts = this->dataFieldIdGet("valid_stages", &field_id);
  if (sts) {
    TDI_DBGCHK(0);
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
    if ((TDI_SUCCESS == sts) && exists) {
      /* Field exists in stage */
      match_data.setValue(field_id, static_cast<uint64_t>(stage_val));
    }
  }

  return TDI_SUCCESS;
}

tdi_status_t SnapshotLivenessTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<TableKey>(new SnapshotLivenessTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotLivenessTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SnapshotLivenessTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// Snapshot PHV
tdi_status_t SnapshotPhvTable::entryGet(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags & /*flags*/,
                                                const tdi::TableKey &key,
                                                tdi::TableData *data) const {
  if (TDI_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  const SnapshotPhvTableKey &phv_key =
      static_cast<const SnapshotPhvTableKey &>(key);
  SnapshotPhvTableData &phv_data =
      static_cast<SnapshotPhvTableData &>(*data);

  dev_stage_t stage = phv_key.getStageId();
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  uint32_t total_size = 0;

  std::vector<int> stages(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stages.size(), stages.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  auto it = find(stages.begin(), stages.end(), stage);
  if (it == stages.end()) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->bfSnapshotTotalPhvCountGet(dev_tgt.dev_id, &total_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting snapshot PHV number from pipe-mgr,"
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting snapshot PHV capture from pipe-mgr,"
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }

  // Only valid containers should be marked as active.
  // Field id is equal to PHV number. This must be in kept line with JSON file.
  for (uint32_t i = 0; i < total_size; i++) {
    if (static_cast<bool>(fields_v[i]) && phv_data.checkFieldActive(i)) {
      status = phv_data.setValue(i, fields[i]);
      // Errors logged by setter function.
      if (status != TDI_SUCCESS) return status;
    } else {
      phv_data.removeActiveField({i});
    }
  }

  return status;
}

tdi_status_t SnapshotPhvTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (TDI_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  *num_returned = 0;
  const SnapshotPhvTableKey &phv_key =
      static_cast<const SnapshotPhvTableKey &>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  std::vector<int> stages(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stages.size(), stages.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  int start_stage = static_cast<int>(phv_key.getStageId());
  auto it = find(stages.begin(), stages.end(), start_stage);
  if (it == stages.end()) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }

  /* Grab next stage */
  it++;

  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<SnapshotPhvTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;

    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }

    /* Stage not used, set data to null, must be done for gRPC server. */
    if (i >= stages.size() || *it == -1 || it == stages.end()) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    this_key->setStageId(*it);
    status = this->entryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
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

tdi_status_t SnapshotPhvTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  if (TDI_DEV_PIPE_ALL == dev_tgt.pipe_id) {
    LOG_TRACE("%s:%d %s ERROR Getting entry on all pipes not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  SnapshotPhvTableKey *phv_key =
      static_cast<SnapshotPhvTableKey *>(key);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  std::vector<int> stage_list(this->getSize());
  auto status = pipeMgr->bfSnapshotStagesGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, stage_list.size(), stage_list.data());
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting used snapshot stages, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  /* -1 on first index means that no stages were programmed */
  if (stage_list[0] == -1) {
    LOG_TRACE("%s:%d %s ERROR Entry not found",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }

  phv_key->setStageId(stage_list[0]);
  return this->entryGet(session, dev_tgt, flags, *key, data);
}

tdi_status_t SnapshotPhvTable::usageGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return TDI_SUCCESS;
}

tdi_status_t SnapshotPhvTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new SnapshotPhvTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SnapshotPhvTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

tdi_status_t SnapshotPhvTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(
      new SnapshotPhvTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// DynHashCfgTable****************

tdi_status_t DynHashCfgTable::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const DynHashCfgTableData &match_data =
      static_cast<const DynHashCfgTableData &>(data);
  tdi_status_t sts = TDI_SUCCESS;

  std::vector<pipe_hash_calc_input_field_attribute_t> attr_list;
  sts = match_data.attrListGet(&attr_list);

  // If attr list is empty, then we want to reset the entry instead
  if (attr_list.empty()) {
    LOG_DBG("%s:%d %s: Received empty list. Resetting to default entry",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str());
    return this->defaultEntryReset(session, dev_tgt, flags);
  }

  pipe_fld_lst_hdl_t fl_hdl;
  sts = pipeMgr->pipeMgrHashCalcInputGet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet(), &fl_hdl);
  if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in getting dyn hashing field list handle",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }
  sts = pipeMgr->pipeMgrHashCalcInputFieldAttrSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                                  dev_tgt.dev_id,
                                                  table_context_info->tableHdlGet(),  // hdl
                                                  fl_hdl,
                                                  attr_list.size(),
                                                  attr_list.data());
  if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in adding an entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
  }
  return sts;
}

tdi_status_t DynHashCfgTable::defaultEntryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableData *data) const {
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s ERROR : Read from hardware not supported. "
        "Reading from sw instead",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    // Continuing as if read from sw
  }
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  DynHashCfgTableData *match_data =
      static_cast<DynHashCfgTableData *>(data);
  tdi_status_t sts;

  // Get list of top field IDs and their corresponding names
  // and create a temp map
  std::map<tdi_id_t, std::string> name_map;
  std::vector<tdi_id_t> field_id_vec;
  sts = this->dataFieldIdListGet(&field_id_vec);
  if (sts != TDI_SUCCESS) return sts;
  for (const auto &field_id : field_id_vec) {
    std::string field_name;
    sts = this->dataFieldNameGet(field_id, &field_name);
    if (sts != TDI_SUCCESS) return sts;

    name_map[field_id] = field_name;
  }

  tdi_id_t order_field_id, start_bit_field_id, length_field_id;
  sts = dataFieldIdGet("order", &order_field_id);
  if (sts != TDI_SUCCESS) return sts;

  sts = dataFieldIdGet("start_bit", &start_bit_field_id);
  if (sts != TDI_SUCCESS) return sts;

  sts = dataFieldIdGet("length", &length_field_id);
  if (sts != TDI_SUCCESS) return sts;

  // Get fl_hdl and then attr_list from pipe_mgr
  pipe_fld_lst_hdl_t fl_hdl;
  sts = pipeMgr->pipeMgrHashCalcInputGet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet(), &fl_hdl);
  if (sts != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in get field list handle",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }
  uint32_t num_attr_filled = 0;
  pipe_hash_calc_input_field_attribute_t *attr_list = nullptr;
  sts = pipeMgr->pipeMgrHashCalcInputFieldAttribute2Get(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                                        dev_tgt.dev_id,
                                                        table_context_info->tableHdlGet(),
                                                        fl_hdl,
                                                        &attr_list,
                                                        &num_attr_filled);
  if (sts != TDI_SUCCESS) return sts;

  // temp map to keep data objects in before final top setValue
  std::unordered_map<tdi_id_t, std::vector<std::unique_ptr<tdi::TableData>>>
      c_field_to_data_map;

  for (uint32_t i = 0; i < num_attr_filled; i++) {
    pipe_hash_calc_input_field_attribute_t *attr = &(attr_list[i]);

    // Figure out which top-field this attr slice belongs to
    tdi_id_t container_field_id = attr->input_field + 1;
    auto field_name = name_map.at(container_field_id);

    // Allocate a data object and set Value for all container fields
    std::unique_ptr<tdi::TableData> inner_data;
    sts = this->dataAllocateContainer(container_field_id, &inner_data);
    if (sts != TDI_SUCCESS) goto cleanup;

    sts = inner_data->setValue(order_field_id,
                               static_cast<uint64_t>(attr->order));
    if (sts != TDI_SUCCESS) goto cleanup;

    sts = inner_data->setValue(start_bit_field_id,
                               static_cast<uint64_t>(attr->slice_start_bit));
    if (sts != TDI_SUCCESS) goto cleanup;

    // if pipe_mgr length is 0, don't set in TDI. It will be defaulted to
    // end of slice. Setting 0 explicitly is not to be done because
    // that makes the order 0
    if (attr->slice_length != 0) {
      sts = inner_data->setValue(length_field_id,
                                 static_cast<uint64_t>(attr->slice_length));
      if (sts != TDI_SUCCESS) goto cleanup;
    }

    if (c_field_to_data_map.find(container_field_id) ==
        c_field_to_data_map.end()) {
      c_field_to_data_map[container_field_id] =
          std::vector<std::unique_ptr<tdi::TableData>>{};
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
      std::unique_ptr<tdi::TableData> inner_data;
      sts = this->dataAllocateContainer(container_field_id, &inner_data);
      if (sts != TDI_SUCCESS) goto cleanup;

      sts = inner_data->setValue(order_field_id, static_cast<uint64_t>(0));
      if (sts != TDI_SUCCESS) goto cleanup;

      sts = inner_data->setValue(start_bit_field_id, static_cast<uint64_t>(0));
      if (sts != TDI_SUCCESS) goto cleanup;
      // skip setting length to default to full field
      // set it in the map
      c_field_to_data_map[container_field_id].push_back(std::move(inner_data));
    }
  }
  // From the temp map, setValue per con_field_id
  for (auto &cf_dl_pair : c_field_to_data_map) {
    sts = match_data->setValue(cf_dl_pair.first, std::move(cf_dl_pair.second));
    if (sts != TDI_SUCCESS) goto cleanup;
  }

cleanup:
  auto d_sts = pipeMgr->pipeMgrHashCalcAttributeListDestroy(attr_list);
  if (d_sts) {
    LOG_ERROR("%s:%d %s ERROR while destroying attr list ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return d_sts;
  }
  if (sts) {
    LOG_ERROR("%s:%d %s ERROR while trying to Get default entry ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return d_sts;
  }
  return sts;
}

tdi_status_t DynHashCfgTable::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  tdi_status_t sts = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  sts = pipeMgr->pipeMgrHashCalcInputDefaultSet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet());
  if (TDI_SUCCESS != sts) {
    LOG_TRACE("%s:%d %s: Error in resetting to default hash field list",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return sts;
  }
  return sts;
}

tdi_status_t DynHashCfgTable::clear(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags &flags) const {
  return this->defaultEntryReset(session, dev_tgt, flags);
}

tdi_status_t DynHashCfgTable::dataAllocate_internal(
    const tdi_id_t &container_id,
    std::unique_ptr<tdi::TableData> *data_ret,
    const std::vector<tdi_id_t> &fields) const {
  std::string field_name = "";
  if (container_id) {
    // container_id should exist
    auto sts = this->dataFieldNameGet(container_id, 0, &field_name);
    if (sts != TDI_SUCCESS) {
      LOG_WARN("%s:%d:%s : Error while getting Container ID %d ",
               __func__,
               __LINE__,
               tableInfoGet()->nameGet().c_str(),
               container_id);
      return TDI_INVALID_ARG;
    }
  }
  *data_ret = std::unique_ptr<tdi::TableData>(
      new DynHashCfgTableData(this, container_id, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  if (container_id) {
    auto data = static_cast<DynHashCfgTableData *>(data_ret->get());
    data->setDynHashFieldName(field_name);
  }
  return TDI_SUCCESS;
}

tdi_status_t DynHashCfgTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return this->dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t DynHashCfgTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return this->dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t DynHashCfgTable::dataAllocateContainer(
    const tdi_id_t &container_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> empty;
  return this->dataAllocate_internal(container_id, data_ret, empty);
}

tdi_status_t DynHashCfgTable::dataAllocateContainer(
    const tdi_id_t &container_id,
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return this->dataAllocate_internal(container_id, data_ret, fields);
}

// DynHashAlgoTable****************

tdi_status_t DynHashAlgoTable::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const DynHashAlgoTableData &match_data =
      static_cast<const DynHashAlgoTableData &>(data);
  tdi_status_t status = TDI_SUCCESS;

  // Figure out which action if given
  tdi_id_t action_id = match_data.actionIdGet(&action_id);

  std::string action_name = "";
  if (action_id) {
    this->actionNameGet(action_id, &action_name);
  }

  std::vector<tdi_id_t> dataFields;
  /* 3 cases
   * Not all fields are set
   * All fields are set but action_id is 0
   * All fields are set but action_id is non-zero.
   */
  bool all_fields_set = match_data.allFieldsSetGet();
  if (!all_fields_set) {
    dataFields.assign(match_data.activeFieldsGet().begin(),
                      match_data.activeFieldsGet().end());
  } else {
    if (action_id == 0) {
      LOG_TRACE(
          "%s:%d %s :Error: Need to provide an action ID for all fields for "
          "entryMod",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return TDI_INVALID_ARG;
    } else {
      status = this->dataFieldIdListGet(action_id, &dataFields);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  status);
        return status;
      }
    }
  }

  bool is_algo_set = false;
  bool is_seed_set = false;
  uint64_t seed = 0;

  bfn_hash_algorithm_t algorithm = {};
  status = setHashAlgoFromDataObject(this,
                                     match_data,
                                     dataFields,
                                     &algorithm,
                                     &seed,
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
                  tableInfoGet()->nameGet().c_str(),
                  err);
        return TDI_INVALID_ARG;
      }
    }
    // First set the algorithm. An initial default seed is also set based upon
    // the algorithm as part of pipeMgrHashCalcAlgorithmSet
    // Then go ahead and set custom seed
    status = pipeMgr->pipeMgrHashCalcAlgorithmSet(
        session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet(), -1, &algorithm);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set dyn hash algorithm",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return status;
    }
  }
  if (is_seed_set) {
    status = pipeMgr->pipeMgrHashCalcSeedSet(
        session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, table_context_info->tableHdlGet(), seed);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set seed %" PRIu64,
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                seed);
      return status;
    }
  }

  return status;
}

tdi_status_t DynHashAlgoTable::defaultEntryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  DynHashAlgoTableData *algo_data =
      static_cast<DynHashAlgoTableData *>(data);
  tdi_status_t status = TDI_SUCCESS;
  uint64_t seed = 0;
  status = pipeMgr->pipeMgrHashCalcSeedGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                           dev_tgt.dev_id,
                                           table_context_info->tableHdlGet(),
                                           (pipe_hash_seed_t *)&seed);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Get Dyn Hashing Algorithm and Seed fail",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  bfn_hash_algorithm_t algorithm;
  pipe_hash_alg_hdl_t alg_hdl;
  status = pipeMgr->pipeMgrHashCalcAlgorithmGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                                dev_tgt.dev_id,
                                                table_context_info->tableHdlGet(),
                                                &alg_hdl,
                                                &algorithm);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set dyn hash algorithm",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = getHashAlgoInDataObject(this, algorithm, seed, algo_data);
  return status;
}

tdi_status_t DynHashAlgoTable::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeMgrHashCalcAlgorithmReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, this->table_context_info->tableHdlGet());
}

tdi_status_t DynHashAlgoTable::clear(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags) const {
  return this->defaultEntryReset(session, dev_tgt, flags);
}

tdi_status_t DynHashAlgoTable::dataAllocate_internal(
    tdi_id_t action_id,
    std::unique_ptr<tdi::TableData> *data_ret,
    const std::vector<tdi_id_t> &fields) const {
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
      return TDI_OBJECT_NOT_FOUND;
    }
    *data_ret = std::unique_ptr<tdi::TableData>(
        new DynHashAlgoTableData(this, action_id, fields));
  } else {
    *data_ret = std::unique_ptr<tdi::TableData>(
        new DynHashAlgoTableData(this, fields));
  }

  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t DynHashAlgoTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t DynHashAlgoTable::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
tdi_status_t DynHashAlgoTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t DynHashAlgoTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

// DynHashComputeTable****************
tdi_status_t DynHashComputeTable::getRef(pipe_tbl_hdl_t *hdl,
                                            const Table **cfg_tbl,
                                            uint32_t *hash_len) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find ref. Missing required DynHashConfig table reference"
        "in tdi.json");
    return TDI_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid tdi.json configuration. compute should be part"
        " of exactly one config table.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  *hdl = it->second.back().tbl_hdl;
  auto tbl_id = it->second.back().id;

  // Get the cfg table
  auto status = this->tdiInfoGet()->tdiTableFromIdGet(tbl_id, cfg_tbl);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  auto cfg_tbl_obj = static_cast<const tdi::Table *>(*cfg_tbl);
  *hash_len = (cfg_tbl_obj->hash_bit_width_get() + 7) / 8;
  return TDI_SUCCESS;
}

tdi_status_t DynHashComputeTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<TableKey>(new DynHashComputeTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t DynHashComputeTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields{};
  if (data_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  *data_ret = std::unique_ptr<tdi::TableData>(
      new DynHashComputeTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t DynHashComputeTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = TDI_SUCCESS;
  if (data == nullptr) {
    return TDI_INVALID_ARG;
  }

  const DynHashComputeTableKey &hash_key =
      static_cast<const DynHashComputeTableKey &>(key);
  DynHashComputeTableData *hash_data =
      static_cast<DynHashComputeTableData *>(data);

  uint32_t hash_len = 0;

  pipe_tbl_hdl_t cfg_tbl_hdl;
  const Table *cfg_table;
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
                this->tableInfoGet()->nameGet().c_str());
      break;
    }
    status = pipeMgr->pipeMgrHashCalcCalculateHashValueWithCfg(
        session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
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

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get hash value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  tdi_id_t hash_value_id;
  status = this->dataFieldIdGet("hash_value", 0, &hash_value_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  status = hash_data->setValue(hash_value_id, hash.data(), hash_len);
  return status;
}

// SelectorGetMemberTable****************
tdi_status_t SelectorGetMemberTable::getRef(
    pipe_sel_tbl_hdl_t *sel_tbl_hdl,
    const SelectorTable **sel_tbl,
    const ActionProfile **act_tbl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find sel ref. Missing required sel table reference "
        "in tdi.json");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid tdi.json configuration. SelGetMem should be part"
        " of exactly one Selector table.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  *sel_tbl_hdl = it->second.back().tbl_hdl;
  auto sel_tbl_id = it->second.back().id;

  // Get the Sel table
  const Table *tbl;
  auto status = this->tdiInfoGet()->tdiTableFromIdGet(sel_tbl_id, &tbl);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  auto tbl_obj = static_cast<const tdi::Table *>(tbl);
  *sel_tbl = static_cast<const SelectorTable *>(tbl_obj);

  // Get action profile table
  status =
      this->tdiInfoGet()->tdiTableFromIdGet(tbl_obj->getActProfId(), &tbl);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  tbl_obj = static_cast<const tdi::Table *>(tbl);
  *act_tbl = static_cast<const ActionProfile *>(tbl_obj);
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<TableKey>(new SelectorGetMemberTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  if (data_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  std::vector<tdi_id_t> fields{};
  *data_ret = std::unique_ptr<tdi::TableData>(
      new SelectorGetMemberTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = TDI_SUCCESS;
  if (data == nullptr) {
    return TDI_INVALID_ARG;
  }

  const SelectorGetMemberTableKey &sel_key =
      static_cast<const SelectorGetMemberTableKey &>(key);
  SelectorGetMemberTableData *sel_data =
      static_cast<SelectorGetMemberTableData *>(data);

  uint64_t grp_id = 0;
  tdi_id_t field_id;
  status = this->keyFieldIdGet("$SELECTOR_GROUP_ID", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get field ID for grp ID",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &grp_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to grp_id",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  uint64_t hash = 0;
  status = this->keyFieldIdGet("hash_value", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &hash);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get hash",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  const SelectorTable *sel_tbl;
  const ActionProfile *act_tbl;
  pipe_sel_tbl_hdl_t sel_tbl_hdl;
  status = this->getRef(&sel_tbl_hdl, &sel_tbl, &act_tbl);
  if (status) {
    return status;
  }

  pipe_sel_grp_hdl_t grp_hdl;
  status = sel_tbl->getGrpHdl(session, dev_tgt, grp_id, &grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  uint32_t num_bytes = sizeof(uint64_t);
  std::vector<uint8_t> hash_arr(num_bytes, 0);
  EndiannessHandler::toNetworkOrder(num_bytes, hash, hash_arr.data());

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  status = pipeMgr->pipeMgrSelGrpMbrGetFromHash(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
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
        this->tableInfoGet()->nameGet().c_str(),
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
              this->tableInfoGet()->nameGet().c_str(),
              adt_ent_hdl);
    return status;
  }

  status = this->dataFieldIdGet("$ACTION_MEMBER_ID", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_data->setValue(field_id, act_mbr_id);
  return status;
}

// Debug counter

// Caller must do null pointer check.
char **TblDbgCntTable::allocDataForTableNames() const {
  char **tbl_names = new char *[this->_table_size];
  if (tbl_names != NULL) {
    for (uint32_t i = 0; i < this->_table_size; i++) {
      tbl_names[i] = new char[PIPE_MGR_TBL_NAME_LEN];
      std::memset(tbl_names[i], 0, PIPE_MGR_TBL_NAME_LEN);
    }
  }
  return tbl_names;
}

void TblDbgCntTable::freeDataForTableNames(char **tbl_names) const {
  for (uint32_t i = 0; i < this->_table_size; i++) {
    delete[] tbl_names[i];
  }
  delete[] tbl_names;
}

tdi_status_t TblDbgCntTable::entryMod(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags & /*flags*/,
                                              const tdi::TableKey &key,
                                              const tdi::TableData &data) const {
  const TblDbgCntTableKey &dbg_key =
      static_cast<const TblDbgCntTableKey &>(key);
  const TblDbgCntTableData &dbg_data =
      static_cast<const TblDbgCntTableData &>(data);
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_id_t field_id;
  auto status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    TDI_DBGCHK(0);
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
              tableInfoGet()->nameGet().c_str(),
              status);
    return TDI_INVALID_ARG;
  }

  bf_tbl_dbg_counter_type_t type =
      static_cast<bf_tbl_dbg_counter_type_t>(it - cntTypeStr.begin());
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  // Convert name to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  status =
      pipeMgr->bfDbgCounterSet(tgt, const_cast<char *>(tbl_name.c_str()), type);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }
  return status;
}

tdi_status_t TblDbgCntTable::entryGet(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags & /*flags*/,
                                              const tdi::TableKey &key,
                                              tdi::TableData *data) const {
  const TblDbgCntTableKey &dbg_key =
      static_cast<const TblDbgCntTableKey &>(key);
  TblDbgCntTableData &dbg_data =
      static_cast<TblDbgCntTableData &>(*data);
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }

  tdi_id_t field_id;
  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, cntTypeStr.at(type));
  if (status) return status;

  status = this->dataFieldIdGet("value", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, static_cast<uint64_t>(value));
  if (status) return status;

  return status;
}

tdi_status_t TblDbgCntTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  *num_returned = 0;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
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
  const TblDbgCntTableKey &dbg_key =
      static_cast<const TblDbgCntTableKey &>(key);
  // Convert key to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  auto it = std::find(tbl_names.begin(), tbl_names.end(), tbl_name);
  if (it == tbl_names.end()) {
    return TDI_OBJECT_NOT_FOUND;
  }
  // Move to next element
  it++;

  for (uint32_t i = 0; i < n; i++) {
    auto this_key =
        static_cast<TblDbgCntTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    // If run out of entries, mark remaning data as empty.
    if (it == tbl_names.end()) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    tbl_name = getQualifiedTableName(dev_tgt.dev_id, this->prog_name, *it);
    this_key->setTblName(tbl_name);
    status = this->entryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
    it++;
  }

  return TDI_SUCCESS;
}

tdi_status_t TblDbgCntTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  TblDbgCntTableKey *dbg_key = static_cast<TblDbgCntTableKey *>(key);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
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
  return this->entryGet(session, dev_tgt, flags, *key, data);
}

tdi_status_t TblDbgCntTable::clear(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }

  for (int i = 0; i < num_tbls; ++i) {
    status = pipeMgr->bfDbgCounterClear(tgt, tbl_list[i]);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s Error while clearing debug counter for table %s, %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                tbl_list[i],
                status);
      break;
    }
  }
  this->freeDataForTableNames(tbl_list);
  return status;
}

tdi_status_t TblDbgCntTable::usageGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return TDI_SUCCESS;
}

tdi_status_t TblDbgCntTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new TblDbgCntTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t TblDbgCntTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

tdi_status_t TblDbgCntTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new TblDbgCntTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// Log table debug counter
tdi_status_t LogDbgCntTable::entryMod(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags & /*flags*/,
                                              const tdi::TableKey &key,
                                              const tdi::TableData &data) const {
  const LogDbgCntTableKey &dbg_key =
      static_cast<const LogDbgCntTableKey &>(key);
  uint64_t stage, log_tbl;
  tdi_id_t field_id;
  auto status = this->keyFieldIdGet("stage", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &stage);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &log_tbl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  const TblDbgCntTableData &dbg_data =
      static_cast<const TblDbgCntTableData &>(data);
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    TDI_DBGCHK(0);
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
              tableInfoGet()->nameGet().c_str(),
              status);
    return TDI_INVALID_ARG;
  }

  bf_tbl_dbg_counter_type_t type =
      static_cast<bf_tbl_dbg_counter_type_t>(it - cntTypeStr.begin());
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->bfDbgCounterSet(tgt, stage, log_tbl, type);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting debug counter for pipe %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return status;
}

tdi_status_t LogDbgCntTable::entryGet(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags & /*flags*/,
                                              const tdi::TableKey &key,
                                              tdi::TableData *data) const {
  const LogDbgCntTableKey &dbg_key =
      static_cast<const LogDbgCntTableKey &>(key);
  TblDbgCntTableData &dbg_data =
      static_cast<TblDbgCntTableData &>(*data);
  uint64_t stage, log_tbl;
  tdi_id_t field_id;
  auto status = this->keyFieldIdGet("stage", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &stage);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(field_id, &log_tbl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting debug counter for pipe %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_tgt.pipe_id,
              status);
    return status;
  }

  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, cntTypeStr.at(type));
  if (status) return status;

  status = this->dataFieldIdGet("value", &field_id);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, static_cast<uint64_t>(value));
  if (status) return status;

  return status;
}

tdi_status_t LogDbgCntTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  *num_returned = 0;
  // 16 logical tables per stage
  uint64_t num_tbls = 16;
  // Total table size is nuber of logical tables per stage multiplied by number
  // of stages.
  uint64_t last_stage = this->_table_size / num_tbls;
  const LogDbgCntTableKey &dbg_key =
      static_cast<const LogDbgCntTableKey &>(key);
  tdi_id_t stage_field_id, log_tbl_field_id;
  uint64_t stage, log_tbl;
  auto status = this->keyFieldIdGet("stage", &stage_field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(stage_field_id, &stage);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              stage_field_id);
    return status;
  }
  status = this->keyFieldIdGet("log_tbl", &log_tbl_field_id);
  if (status != TDI_SUCCESS) {
    TDI_DBGCHK(0);
    return status;
  }
  status = dbg_key.getValue(log_tbl_field_id, &log_tbl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              log_tbl_field_id);
    return status;
  }

  // Grab next entry
  if (stage >= last_stage || log_tbl >= num_tbls) {
    return TDI_INVALID_ARG;
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
          static_cast<LogDbgCntTableKey *>((*key_data_pairs)[i].first);
      auto this_data = (*key_data_pairs)[i].second;

      status = this_key->setValue(log_tbl_field_id, tbl_id);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  log_tbl_field_id);
        return status;
      }
      status = this_key->setValue(stage_field_id, stage_id);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  stage_field_id);
        return status;
      }
      status =
          this->entryGet(session, dev_tgt, flags, *this_key, this_data);
      if (status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  i + 1,
                  status);
        // Make the data object null if error
        (*key_data_pairs)[i].second = nullptr;
      }
      (*num_returned)++;
    }
    log_tbl = 0;
  }

  return TDI_SUCCESS;
}

tdi_status_t LogDbgCntTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  LogDbgCntTableKey *dbg_key = static_cast<LogDbgCntTableKey *>(key);
  std::string field_names[2] = {"stage", "log_tbl"};
  for (auto field_name : field_names) {
    tdi_id_t field_id;
    auto status = this->keyFieldIdGet(field_name, &field_id);
    if (status != TDI_SUCCESS) {
      TDI_DBGCHK(0);
      return status;
    }
    status = dbg_key->setValue(field_id, 0);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to set the value for field id %d",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
  }

  return this->entryGet(session, dev_tgt, flags, *key, data);
}

tdi_status_t LogDbgCntTable::clear(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/) const {
  tdi_status_t status = TDI_SUCCESS;
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
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Error while clearing debug counter for stage %d, log_tbl "
            "%d, err=%d",
            __func__,
            __LINE__,
            this->tableInfoGet()->nameGet().c_str(),
            stage,
            log_tbl,
            status);
        break;
      }
    }
  }
  return status;
}

tdi_status_t LogDbgCntTable::usageGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const tdi::Flags & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return TDI_SUCCESS;
}

tdi_status_t LogDbgCntTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new LogDbgCntTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t LogDbgCntTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

tdi_status_t LogDbgCntTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // This table reuses TblDbgCnt data object.
  *data_ret =
      std::unique_ptr<tdi::TableData>(new TblDbgCntTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// Register param table
// Table reuses debug counter table data object.
tdi_status_t RegisterParamTable::getRef(pipe_tbl_hdl_t *hdl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot set register param. Missing required register table reference "
        "in tdi.json.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid tdi.json configuration. Register param should be part"
        " of exactly one register table.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  *hdl = it->second[0].tbl_hdl;
  return TDI_SUCCESS;
}

tdi_status_t RegisterParamTable::getParamHdl(const tdi::Session &session,
                                                tdi_dev_id_t dev_id) const {
  pipe_reg_param_hdl_t reg_param_hdl;
  // Get param name from table name. Have to remove pipe name.
  std::string param_name = this->tableInfoGet()->nameGet();
  param_name = param_name.erase(0, param_name.find(".") + 1);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status =
      pipeMgr->pipeStfulParamGetHdl(dev_id, param_name.c_str(), &reg_param_hdl);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to get register param handle from pipe mgr",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    TDI_DBGCHK(0);
    return 0;
  }

  return reg_param_hdl;
}

tdi_status_t RegisterParamTable::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableData &data) const {
  const RegisterParamTableData &mdata =
      static_cast<const RegisterParamTableData &>(data);
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    mdata.value);
}

tdi_status_t RegisterParamTable::defaultEntryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    tdi::TableData *data) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }
  RegisterParamTableData *mdata =
      static_cast<RegisterParamTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    &mdata->value);
}

tdi_status_t RegisterParamTable::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
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
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      tbl_hdl,
      this->getParamHdl(session, dev_tgt.dev_id));
}

tdi_status_t RegisterParamTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new RegisterParamTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}
#endif

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
