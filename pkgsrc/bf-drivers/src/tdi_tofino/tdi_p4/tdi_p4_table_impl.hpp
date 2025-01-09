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


#ifndef _TDI_P4_TABLE_OBJ_HPP
#define _TDI_P4_TABLE_OBJ_HPP

// tdi includes
#include <tdi/common/tdi_table.hpp>

//#include "tdi_table_state.hpp"
#include <tdi_common/tdi_pipe_mgr_intf.hpp>
#include <tdi_p4/tdi_p4_table_data_impl.hpp>

namespace tdi {
namespace tna {
namespace tofino {

class MatchIdleTableState {
 public:
  MatchIdleTableState() : enabled_(false), poll_mode_(true){};
  bool isIdleTableEnabled() { return enabled_; };
  bool isIdleTableinPollMode() { return poll_mode_; };

  void setEnabled(bool enable) { enabled_ = enable; };
  void setPollMode(bool poll_mode) { poll_mode_ = poll_mode; };

 private:
  bool enabled_;
  bool poll_mode_;
};

/*
 * MatchActionDirect
 * MatchActionIndirectTable
 * ActionTable
 * SelectorTable
 * CounterTable
 * MeterTable
 * LPFTable
 * WREDTable
 * RegisterTable
 * PVSTable
 * Phase0Table
 * SnapshotConfigTable
 * SnapshotTriggerTable
 * SnapshotPhvTable
 * SnapshotLivenessTable
 * DynHashCfgTable
 * DynHashAlgoTable
 * DgbCntTable
 * LogDgbCntTable
 * RegisterParamTable
 */

class MatchActionDirect : public tdi::Table {
 public:
  MatchActionDirect(const tdi::TdiInfo *tdi_info,
                    const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_SET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_RESET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_SIZE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info),
        idle_table_state(new MatchIdleTableState()){};

  virtual tdi_status_t entryAdd(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      const tdi::TableData &data) const override final;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntrySet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;

#if 0
  virtual tdi_status_t defaultEntryMod(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;
#endif

  virtual tdi_status_t defaultEntryReset(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t sizeGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               size_t *size) const override;
  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const tdi_id_t &action_id,
                                 tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 const tdi_id_t &action_id,
                                 tdi::TableData *data) const override;

  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#if 0
  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#endif
  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;
  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;
#if 0
  virtual tdi_status_t operationsAllocate(
      const tdi_operations_type_e &type,
      std::unique_ptr<tdi::TableOperations> *table_ops) const override;
  virtual tdi_status_t tableOperationsExecute(
      const tdi::TableOperations &tableOperations) const override;
#endif

  bool actionIdApplicable() const override { return true; };

  bool idleTableEnabled() const {
    return idle_table_state->isIdleTableEnabled();
  }
  bool idleTablePollMode() const {
    return idle_table_state->isIdleTableinPollMode();
  }
  TdiThreadPool *idletimeCbTdiThreadPoolGet() const {
    return idletime_cb_thread_pool.get();
  }
  template <class T>
  void idletimeCbTdiThreadPoolReset(T obj) {
    this->idletime_cb_thread_pool.reset(obj);
  }
  MatchIdleTableState *idleTableStateGet() const {
    return idle_table_state.get();
  }

 private:
  tdi_status_t dataAllocate_internal(tdi_id_t action_id,
                                     std::unique_ptr<tdi::TableData> *data_ret,
                                     const std::vector<tdi_id_t> &fields) const;

  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                 pipe_tbl_match_spec_t *pipe_match_spec,
                                 tdi::TableData *data) const;

  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter;
  std::map<tdi_id_t, bool> act_uses_dir_cntr;
  std::map<tdi_id_t, bool> act_uses_dir_reg;

  std::unique_ptr<MatchIdleTableState> idle_table_state;
  // We need this variable to be mutable because we dynamically create/destroy
  // the thread pool when the user sets (in NOTIFY_MODE)/resets idle timeout
  // for a table. This happens during tableAttributesSet API. However,
  // since all the APIs on our table objects are const, we need to make this
  // mutable. It is ok to make private data members mutable
  mutable std::unique_ptr<TdiThreadPool> idletime_cb_thread_pool{nullptr};
};

class MatchActionIndirect : public tdi::Table {
 public:
  MatchActionIndirect(const tdi::TdiInfo *tdi_info,
                      const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_SET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_RESET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_SIZE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info),
        idle_table_state(new MatchIdleTableState()){};

  virtual tdi_status_t entryAdd(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      const tdi::TableData &data) const override final;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntrySet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;

#if 0
  virtual tdi_status_t defaultEntryMod(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;
#endif

  virtual tdi_status_t defaultEntryReset(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t sizeGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               size_t *size) const override;
  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 tdi::TableData *data) const override;

  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#if 0
  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#endif
  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;
  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;

#if 0
  virtual tdi_status_t operationsAllocate(
      const tdi_operations_type_e &type,
      std::unique_ptr<tdi::TableOperations> *table_ops) const override;
  virtual tdi_status_t tableOperationsExecute(
      const tdi::TableOperations &tableOperations) const override;
#endif

  bool actionIdApplicable() const override { return false; };

  bool idleTableEnabled() const {
    return idle_table_state->isIdleTableEnabled();
  }
  bool idleTablePollMode() const {
    return idle_table_state->isIdleTableinPollMode();
  }
  TdiThreadPool *idletimeCbTdiThreadPoolGet() const {
    return idletime_cb_thread_pool.get();
  }
  template <class T>
  void idletimeCbTdiThreadPoolReset(T obj) {
    this->idletime_cb_thread_pool.reset(obj);
  }
  MatchIdleTableState *idleTableStateGet() const {
    return idle_table_state.get();
  }
  tdi_status_t getActionState(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const MatchActionIndirectTableData *data,
                              pipe_adt_ent_hdl_t *adt_entry_hdl,
                              pipe_sel_grp_hdl_t *sel_grp_hdl,
                              pipe_act_fn_hdl_t *act_fn_hdl,
                              pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getActionMbrIdFromHndl(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const pipe_adt_ent_hdl_t adt_ent_hdl,
                                      tdi_id_t *mbr_id) const;

  tdi_status_t getGroupIdFromHndl(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const pipe_sel_grp_hdl_t sel_grp_hdl,
                                  tdi_id_t *grp_id) const;

 private:
  tdi_status_t dataAllocate_internal(tdi_id_t action_id,
                                     std::unique_ptr<tdi::TableData> *data_ret,
                                     const std::vector<tdi_id_t> &fields) const;

  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                 pipe_tbl_match_spec_t *pipe_match_spec,
                                 tdi::TableData *data) const;

  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter;
  std::map<tdi_id_t, bool> act_uses_dir_cntr;
  std::map<tdi_id_t, bool> act_uses_dir_reg;

  std::unique_ptr<MatchIdleTableState> idle_table_state;
  // We need this variable to be mutable because we dynamically create/destroy
  // the thread pool when the user sets (in NOTIFY_MODE)/resets idle timeout
  // for a table. This happens during tableAttributesSet API. However,
  // since all the APIs on our table objects are const, we need to make this
  // mutable. It is ok to make private data members mutable
  mutable std::unique_ptr<TdiThreadPool> idletime_cb_thread_pool{nullptr};
  // indirect table specific
  void populate_indirect_resources(const pipe_mgr_adt_ent_data_t &ent_data,
                                   pipe_action_spec_t *pipe_action_spec) const;
};

class ActionProfile : public tdi::Table {
 public:
  ActionProfile(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info){};

  virtual tdi_status_t entryAdd(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t getMbrIdFromHndl(const tdi::Session &session,
                                const tdi::Target &dev_tgt,

                                const pipe_adt_ent_hdl_t adt_ent_hdl,
                                tdi_id_t *mbr_id) const;

  tdi_status_t getHdlFromMbrId(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi_id_t mbr_id,
                               pipe_adt_ent_hdl_t *adt_ent_hdl) const;

  tdi_status_t getMbrState(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           tdi_id_t mbr_id,
                           pipe_act_fn_hdl_t *act_fn_hdl,
                           pipe_adt_ent_hdl_t *adt_ent_hdl,
                           pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getFirstMbr(const tdi::Target &dev_tgt,
                           tdi_id_t *first_mbr_id,
                           pipe_adt_ent_hdl_t *first_entry_hdl) const;

  tdi_status_t getNextMbr(const tdi::Target &dev_tgt,
                          tdi_id_t mbr_id,
                          tdi_id_t *next_mbr_id,
                          tdi_id_t *next_entry_hdl) const;
  tdi_status_t getMbrList(
      uint16_t device_id,
      std::vector<std::pair<tdi_id_t, bf_dev_pipe_t>> *mbr_id_list) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_adt_ent_hdl_t &entry_hdl,
                                 ActionProfileData *action_tbl_data) const;
};

class Selector : public tdi::Table {
 public:
  Selector(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info){};

  virtual tdi_status_t entryAdd(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t getOneMbr(const tdi::Session &session,
                                 const uint16_t device_id,
                                 const pipe_sel_grp_hdl_t sel_grp_hdl,
                                 pipe_adt_ent_hdl_t *member_hdl) const;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;

#if 0  // TODO: Attributes
  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;

  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;

  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;

  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;

#endif

  tdi_status_t getGrpIdFromHndl(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const pipe_sel_grp_hdl_t &sel_grp_hdl,
                                tdi_id_t *sel_grp_id) const;

  tdi_status_t getGrpHdl(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi_id_t sel_grp_id,
                         pipe_sel_grp_hdl_t *sel_grp_hdl) const;

  tdi_status_t getActMbrIdFromHndl(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const pipe_adt_ent_hdl_t &adt_ent_hd,
                                   tdi_id_t *act_mbr_id) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi_id_t &grp_id,
                                 SelectorTableData *sel_tbl_data) const;

#if 0  // TODO: tableAttributesImpl not defined
  tdi_status_t processSelUpdateCbAttr(
      const BfRtTableAttributesImpl &tbl_attr_impl,
      const bf_rt_target_t &dev_tgt) const;
#endif
  tdi_status_t getFirstGrp(const tdi::Target &dev_tgt,
                           tdi_id_t *first_grp_id,
                           pipe_sel_grp_hdl_t *first_grp_hdl) const;

  tdi_status_t getNextGrp(const tdi::Target &dev_tgt,
                          tdi_id_t grp_id,
                          tdi_id_t *next_grp_id,
                          pipe_sel_grp_hdl_t *next_grp_hdl) const;
};

class CounterIndirect : public tdi::Table {
 public:
  CounterIndirect(const tdi::TdiInfo *tdi_info,
                  const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {}

  tdi_status_t entryAdd(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;
  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::Target *entry_tgt,
                           tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableKey &key,
                              tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi_handle_t &entry_handle,
                        tdi::TableKey *key,
                        tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             tdi::TableKey *key,
                             tdi::TableData *data) const override;

  tdi_status_t entryGetNextN(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             const tdi::TableKey &key,
                             const uint32_t &n,
                             keyDataPairs *key_data_pairs,
                             uint32_t *num_returned) const override;

  tdi_status_t clear(const tdi::Session &session,
                     const tdi::Target &dev_tgt,
                     const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class MeterIndirect : public tdi::Table {
 public:
  MeterIndirect(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {}

  tdi_status_t entryAdd(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::Target *entry_tgt,
                           tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableKey &key,
                              tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi_handle_t &entry_handle,
                        tdi::TableKey *key,
                        tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             tdi::TableKey *key,
                             tdi::TableData *data) const override;

  tdi_status_t entryGetNextN(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             const tdi::TableKey &key,
                             const uint32_t &n,
                             keyDataPairs *key_data_pairs,
                             uint32_t *num_returned) const override;

  tdi_status_t clear(const tdi::Session &session,
                     const tdi::Target &dev_tgt,
                     const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Table attributes APIs
  tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<TableAttributes> *attr) const override;
#if 0
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
#endif

  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;

  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;
};

class LpfIndirect : public tdi::Table {
 public:
  LpfIndirect(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {}
};

class WredIndirect : public tdi::Table {
 public:
  WredIndirect(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {}
};

// Register Indirect
class RegisterIndirect : public tdi::Table {
 public:
  RegisterIndirect(const tdi::TdiInfo *tdi_info,
                   const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {}

  tdi_status_t entryAdd(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;
  tdi_status_t entryMod(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        const tdi::TableData &data) const override;
  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &key,
                        tdi::TableData *data) const override;
  tdi_status_t entryKeyGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::Target *entry_tgt,
                           tdi::TableKey *key) const override;
  tdi_status_t entryHandleGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableKey &key,
                              tdi_handle_t *entry_handle) const override;
  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi_handle_t &entry_handle,
                        tdi::TableKey *key,
                        tdi::TableData *data) const override;
  tdi_status_t entryGetFirst(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             tdi::TableKey *key,
                             tdi::TableData *data) const override;
  tdi_status_t entryGetNextN(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             const tdi::TableKey &key,
                             const uint32_t &n,
                             keyDataPairs *key_data_pairs,
                             uint32_t *num_returned) const override;
  tdi_status_t clear(const tdi::Session &session,
                     const tdi::Target &dev_tgt,
                     const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class PVS : public tdi::Table {
 public:
  PVS(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {}

  tdi_status_t entryAdd(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &pvs_key,
                        const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &pvs_key) const override;

  tdi_status_t clear(const tdi::Session &session,
                     const tdi::Target &dev_tgt,
                     const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi::TableKey &pvs_key,
                        tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::Target *entry_tgt,
                           tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableKey &key,
                              tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi_handle_t &entry_handle,
                        tdi::TableKey *key,
                        tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             tdi::TableKey *key,
                             tdi::TableData *data) const override;

  tdi_status_t entryGetNextN(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags,
                             const tdi::TableKey &key,
                             const uint32_t &n,
                             keyDataPairs *key_data_pairs,
                             uint32_t *num_returned) const override;

  tdi_status_t usageGet(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        uint32_t *count) const override;

  tdi_status_t keyAllocate(std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  // dummy data obj for pvs
  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Attribute APIs
  tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#if 0
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
#endif
  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;
  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;
};

#if 0

class ActionTable : public tdi::Table {
 public:
  ActionTable(const std::string &program_name,
                  const tdi_id_t &id,
                  const std::string &name,
                  const size_t &size,
                  const pipe_adt_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::ACTION_PROFILE,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::CLEAR,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t keyReset(TableKey *) const override;

  tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  tdi_status_t dataReset(const tdi_id_t &action_id,
                        tdi::TableData *data) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

  tdi_status_t registerAdtUpdateCb(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const pipe_adt_update_cb &cb,
                                  const void *cookie) const override;

  tdi_status_t getMbrIdFromHndl(const tdi::Session &session,
                               const tdi::Target &dev_tgt,

                               const pipe_adt_ent_hdl_t adt_ent_hdl,
                               tdi_id_t *mbr_id) const;

  tdi_status_t getHdlFromMbrId(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi_id_t mbr_id,
                              pipe_adt_ent_hdl_t *adt_ent_hdl) const;

  tdi_status_t getMbrState(const tdi::Session &session,
                          const tdi::Target &dev_tgt,
                          tdi_id_t mbr_id,
                          pipe_act_fn_hdl_t *act_fn_hdl,
                          pipe_adt_ent_hdl_t *adt_ent_hdl,
                          pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getFirstMbr(const tdi::Target &dev_tgt,
                          tdi_id_t *first_mbr_id,
                          pipe_adt_ent_hdl_t *first_entry_hdl) const;

  tdi_status_t getNextMbr(const tdi::Target &dev_tgt,
                         tdi_id_t mbr_id,
                         tdi_id_t *next_mbr_id,
                         tdi_id_t *next_entry_hdl) const;
  tdi_status_t getMbrList(
      uint16_t device_id,
      std::vector<std::pair<tdi_id_t, bf_dev_pipe_t>> *mbr_id_list) const;

 private:
  tdi_status_t entryGet_internal(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const pipe_adt_ent_hdl_t &entry_hdl,
      ActionTableData *action_tbl_data) const;
};

class SelectorTable : public tdi::Table {
 public:
  SelectorTable(const std::string &program_name,
                    const tdi_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_sel_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::CLEAR,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;

  tdi_status_t tableAttributesGet(
      const tdi::Session & /*session */,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;

  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t getGrpIdFromHndl(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const pipe_sel_grp_hdl_t &sel_grp_hdl,
                               tdi_id_t *sel_grp_id) const;

  tdi_status_t getGrpHdl(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi_id_t sel_grp_id,
                        pipe_sel_grp_hdl_t *sel_grp_hdl) const;

  tdi_status_t getActMbrIdFromHndl(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const pipe_adt_ent_hdl_t &adt_ent_hdl,
                                  tdi_id_t *act_mbr_id) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi_id_t &grp_id,
                                     SelectorTableData *sel_tbl_data) const;

  tdi_status_t processSelUpdateCbAttr(
      const TableAttributesImpl &tbl_attr_impl,
      const tdi::Target &dev_tgt) const;

  tdi_status_t getFirstGrp(const tdi::Target &dev_tgt,
                          tdi_id_t *first_grp_id,
                          pipe_sel_grp_hdl_t *first_grp_hdl) const;

  tdi_status_t getNextGrp(const tdi::Target &dev_tgt,
                         tdi_id_t grp_id,
                         tdi_id_t *next_grp_id,
                         pipe_sel_grp_hdl_t *next_grp_hdl) const;
};

class CounterTable : public tdi::Table {
 public:
  CounterTable(const std::string &program_name,
                   const tdi_id_t &id,
                   const std::string &name,
                   const size_t &size,
                   const pipe_stat_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::COUNTER,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class MeterTable : public tdi::Table {
 public:
  MeterTable(const std::string &program_name,
                 const tdi_id_t &id,
                 const std::string &name,
                 const size_t &size,
                 const pipe_meter_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::METER,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Table attributes APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;

  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;
};

class LPFTable : public tdi::Table {
 public:
  LPFTable(const std::string &program_name,
               const tdi_id_t &id,
               const std::string &name,
               const size_t &size,
               const pipe_meter_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::LPF,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class WREDTable : public tdi::Table {
 public:
  WREDTable(const std::string &program_name,
                const tdi_id_t &id,
                const std::string &name,
                const size_t &size,
                const pipe_meter_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::WRED,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class RegisterTable : public tdi::Table {
 public:
  RegisterTable(const std::string &program_name,
                    const tdi_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::REGISTER,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl),
        ghost_pipe_tbl_hdl_(){};
  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Attribute APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;
  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;

  tdi_status_t ghostTableHandleSet(const pipe_tbl_hdl_t &pipe_hdl) override;

 private:
  // Ghost table handle
  pipe_tbl_hdl_t ghost_pipe_tbl_hdl_;
};

class Phase0Table : public tdi::Table {
 public:
  Phase0Table(const std::string &program_name,
                  tdi_id_t id,
                  std::string name,
                  const size_t &size,
                  pipe_mat_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::PORT_METADATA,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::CLEAR,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     tdi::TableData *data) const;
};

class SnapshotConfigTable : public tdi::Table {
 public:
  SnapshotConfigTable(const std::string &program_name,
                          tdi_id_t id,
                          std::string name,
                          const size_t &size,
                          pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_CFG,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::DELETE,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                         TableApi::USAGE_GET,
                     },
                     pipe_hdl){};
  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  tdi_status_t setDataFields(const bf_snapshot_dir_t dir,
                            const bool timer_en,
                            const uint32_t timer_val,
                            const std::vector<int> &pipes,
                            const bf_snapshot_ig_mode_t mode,
                            SnapshotConfigTableData *data) const;

  tdi_status_t entryModInternal(const tdi::Session &session,
                                    const tdi::Target &dev_tgt,
                                    const tdi::Flags &flags,
                                    const pipe_snapshot_hdl_t &entry_hdl,
                                    const tdi::TableData &data) const;
};

class SnapshotTriggerTable : public tdi::Table {
 public:
  SnapshotTriggerTable(const std::string &program_name,
                           tdi_id_t id,
                           std::string name,
                           const size_t &size,
                           pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_TRIG,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::DELETE,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl) {
    if (name.find("ingress") != std::string::npos) {
      this->direction = TDI_SNAPSHOT_DIR_INGRESS;
    } else if (name.find("egress") != std::string::npos) {
      this->direction = TDI_SNAPSHOT_DIR_EGRESS;
    } else {
      TDI_DBGCHK(0);
    }
  };
  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;
  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;
  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  bf_snapshot_dir_t direction;
  tdi_status_t getHandle(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const uint32_t &start_stage,
                        pipe_snapshot_hdl_t *entry_hdl) const;
  tdi_status_t entryModInternal(const tdi::Session &session,
                                    const tdi::Target &dev_tgt,
                                    const tdi::Flags &flags,
                                    const tdi::TableKey &key,
                                    const tdi::TableData &data) const;
};

class SnapshotDataTable : public tdi::Table {
 public:
  SnapshotDataTable(const std::string &program_name,
                        tdi_id_t id,
                        std::string name,
                        const size_t &size,
                        pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_DATA,
                     std::set<TableApi>{
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                         TableApi::USAGE_GET,
                     },
                     pipe_hdl){};
  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  tdi_status_t getHandle(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const uint32_t &start_stage,
                        pipe_snapshot_hdl_t *entry_hdl) const;

  tdi_status_t entryGetFieldInfo(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const pipe_snapshot_hdl_t &entry_hdl,
                                     const uint32_t &stage,
                                     uint8_t *capture,
                                     const int num_captures,
                                     SnapshotDataTableData *data) const;

  void entryGetControlInfo(
      const tdi::Session &session,
      const pipe_snapshot_hdl_t &entry_hdl,
      const uint32_t &stage,
      const bf_snapshot_capture_ctrl_info_arr_t &ctrl_info_arr,
      const int & /*num_captures*/,
      SnapshotDataTableData *data) const;

  bool isControlField(const tdi::TableDataField *df) const {
    return (df->getDataType() != DataType::BYTE_STREAM);
  };
  void entryDataSetControlInfoNextTables(
      tdi_dev_id_t dev_id,
      const tdi_id_t field_id,
      const char (&next_tbl_names)[TDI_MAX_LOG_TBLS * TDI_TBL_NAME_LEN],
      SnapshotDataTableData *data) const;
};

class SnapshotLivenessTable : public tdi::Table {
 public:
  SnapshotLivenessTable(const std::string &program_name,
                            tdi_id_t id,
                            std::string name,
                            const size_t &size,
                            pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_LIVENESS,
                     std::set<TableApi>{
                         TableApi::GET,
                     },
                     pipe_hdl) {
    if (name.find("ingress") != std::string::npos) {
      this->direction = TDI_SNAPSHOT_DIR_INGRESS;
    } else if (name.find("egress") != std::string::npos) {
      this->direction = TDI_SNAPSHOT_DIR_EGRESS;
    } else {
      TDI_DBGCHK(0);
    }
  };

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;
  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

 private:
  bf_snapshot_dir_t direction;
};

class SnapshotPhvTable : public tdi::Table {
 public:
  SnapshotPhvTable(const std::string &program_name,
                       tdi_id_t id,
                       std::string name,
                       const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_PHV,
                     std::set<TableApi>{TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override final;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  // Unpublished
  uint32_t getSize() const { return this->_table_size; };
};

class DynHashCfgTable : public tdi::Table {
 public:
  DynHashCfgTable(const std::string &program_name,
                      tdi_id_t id,
                      std::string name,
                      const size_t &size,
                      pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::DYN_HASH_CFG,
                     std::set<TableApi>{
                         TableApi::CLEAR,
                         TableApi::DEFAULT_ENTRY_SET,
                         TableApi::DEFAULT_ENTRY_RESET,
                         TableApi::DEFAULT_ENTRY_GET,
                     },
                     pipe_hdl){};
  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override final;
  tdi_status_t tableDefaultEntrySet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override final;

  tdi_status_t tableDefaultEntryReset(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags) const override final;

  tdi_status_t tableDefaultEntryGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   tdi::TableData *data) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;
  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override final;
  tdi_status_t dataAllocateContainer(
      const tdi_id_t &container_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override final;
  tdi_status_t dataAllocateContainer(
      const tdi_id_t &container_id,
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

 private:
  tdi_status_t dataAllocate_internal(
      const tdi_id_t &container_id,
      std::unique_ptr<tdi::TableData> *data_ret,
      const std::vector<tdi_id_t> &fields) const;
};

class DynHashAlgoTable : public tdi::Table {
 public:
  DynHashAlgoTable(const std::string &program_name,
                       tdi_id_t id,
                       std::string name,
                       const size_t &size,
                       pipe_tbl_hdl_t pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::DYN_HASH_ALGO,
                     std::set<TableApi>{
                         TableApi::CLEAR,
                         TableApi::DEFAULT_ENTRY_SET,
                         TableApi::DEFAULT_ENTRY_RESET,
                         TableApi::DEFAULT_ENTRY_GET,
                     },
                     pipe_hdl){};
  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;
  tdi_status_t tableDefaultEntrySet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi::TableData &data) const override;

  tdi_status_t tableDefaultEntryReset(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags) const override;

  tdi_status_t tableDefaultEntryGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   tdi::TableData *data) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

 private:
  tdi_status_t dataAllocate_internal(
      tdi_id_t action_id,
      std::unique_ptr<tdi::TableData> *data_ret,
      const std::vector<tdi_id_t> &fields) const;
};

class TblDbgCntTable : public tdi::Table {
 public:
  TblDbgCntTable(const std::string &program_name,
                     tdi_id_t id,
                     std::string name,
                     const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::DBG_CNT,
                     std::set<TableApi>{TableApi::MODIFY,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::CLEAR,
                                        TableApi::USAGE_GET}){};

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override final;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  // Unpublished
  char **allocDataForTableNames() const;
  void freeDataForTableNames(char **tbl_names) const;
};

class LogDbgCntTable : public tdi::Table {
 public:
  LogDbgCntTable(const std::string &program_name,
                     tdi_id_t id,
                     std::string name,
                     const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::DBG_CNT,
                     std::set<TableApi>{TableApi::MODIFY,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::CLEAR,
                                        TableApi::USAGE_GET}){};

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override final;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  // Unpublished
  uint32_t getSize() const { return this->_table_size; };
};

class RegisterParamTable : public tdi::Table {
 public:
  RegisterParamTable(const std::string &program_name,
                         tdi_id_t id,
                         std::string name,
                         const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::REG_PARAM,
                     std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                        TableApi::DEFAULT_ENTRY_RESET,
                                        TableApi::DEFAULT_ENTRY_GET}){};

  tdi_status_t tableDefaultEntrySet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi::TableData &data) const override;

  tdi_status_t tableDefaultEntryReset(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags) const override;

  tdi_status_t tableDefaultEntryGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   tdi::TableData *data) const override;
  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

 private:
  tdi_status_t getRef(pipe_tbl_hdl_t *hdl) const;
  tdi_status_t getParamHdl(const tdi::Session &session, tdi_dev_id_t dev_id) const;
};

class DynHashComputeTable : public tdi::Table {
 public:
  DynHashComputeTable(const std::string &program_name,
                          tdi_id_t id,
                          std::string name,
                          const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::DYN_HASH_COMPUTE,
                     std::set<TableApi>{
                         TableApi::GET,
                     }){};

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

 private:
  tdi_status_t getRef(pipe_tbl_hdl_t *hdl,
                     const Table **cfg_tbl,
                     uint32_t *hash_len) const;
};

class SelectorGetMemberTable : public tdi::Table {
 public:
  SelectorGetMemberTable(const std::string &program_name,
                             tdi_id_t id,
                             std::string name,
                             const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR_GET_MEMBER,
                     std::set<TableApi>{
                         TableApi::GET,
                     }){};

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

 private:
  tdi_status_t getRef(pipe_sel_tbl_hdl_t *sel_tbl_id,
                     const SelectorTable **sel_tbl,
                     const ActionTable **act_tbl) const;
};
#endif

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_OBJ_HPP
