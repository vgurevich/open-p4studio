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


#ifndef _BF_RT_P4_TABLE_OBJ_HPP
#define _BF_RT_P4_TABLE_OBJ_HPP

#include "bf_rt_table_state.hpp"
#include "bf_rt_p4_table_data_impl.hpp"
#include "bf_rt_p4_table_key_impl.hpp"

namespace bfrt {

class BfRtMatchIdleTableState {
 public:
  BfRtMatchIdleTableState() : enabled_(false), poll_mode_(true){};
  bool isIdleTableEnabled() { return enabled_; };
  bool isIdleTableinPollMode() { return poll_mode_; };

  void setEnabled(bool enable) { enabled_ = enable; };
  void setPollMode(bool poll_mode) { poll_mode_ = poll_mode; };

 private:
  bool enabled_;
  bool poll_mode_;
};

/*
 * BfRtMatchActionTable
 * BfRtMatchActionIndirectTable
 * BfRtActionTable
 * BfRtSelectorTable
 * BfRtCounterTable
 * BfRtMeterTable
 * BfRtLPFTable
 * BfRtWREDTable
 * BfRtRegisterTable
 * BfRtPVSTable
 * BfRtPhase0Table
 * BfRtSnapshotConfigTable
 * BfRtSnapshotTriggerTable
 * BfRtSnapshotPhvTable
 * BfRtSnapshotLivenessTable
 * BfRtDynHashCfgTable
 * BfRtDynHashAlgoTable
 * BfRtDgbCntTable
 * BfRtLogDgbCntTable
 * BfRtRegisterParamTable
 */

class BfRtMatchActionTable : public BfRtTableObj {
 public:
  BfRtMatchActionTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size,
                       pipe_mat_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::MATCH_DIRECT,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::CLEAR,
                                        TableApi::DEFAULT_ENTRY_SET,
                                        TableApi::DEFAULT_ENTRY_RESET,
                                        TableApi::DEFAULT_ENTRY_GET,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET,
                                        TableApi::KEY_GET,
                                        TableApi::HANDLE_GET,
                                        TableApi::GET_BY_HANDLE,
                                        TableApi::ADD_OR_MOD},
                     pipe_hdl),
        counter_tbl_id(),
        meter_tbl_id(),
        register_tbl_id(),
        idle_table_state(new BfRtMatchIdleTableState()){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryAddOrMod(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 const BfRtTableKey &key,
                                 const BfRtTableData &data,
                                 bool *is_added) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableDefaultEntrySet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   const BfRtTableData &data) const override;

  bf_status_t tableDefaultEntryReset(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags) const override;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableSizeGet(const BfRtSession &session,
                           const bf_rt_target_t &dev_tgt,
                           const uint64_t &flags,
                           size_t *count) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  bf_status_t dataReset(const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  // Table attributes APIs
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;
  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;
  bf_status_t registerMatUpdateCb(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const pipe_mat_update_cb &cb,
                                  const void *cookie) const override;

  bool idleTableEnabled() const override {
    return idle_table_state->isIdleTableEnabled();
  }
  bool idleTablePollMode() const override {
    return idle_table_state->isIdleTableinPollMode();
  }
  BfRtThreadPool *idletimeCbThreadPoolGet() const {
    return idletime_cb_thread_pool.get();
  }
  template <class T>
  void idletimeCbThreadPoolReset(T obj) {
    this->idletime_cb_thread_pool.reset(obj);
  }
  BfRtMatchIdleTableState *idleTableStateGet() const {
    return idle_table_state.get();
  }
  void setIsTernaryTable(const bf_dev_id_t &dev_id) override final;
  void setActionResources(const bf_dev_id_t &dev_id) override final;

 private:
  bf_rt_id_t counter_tbl_id;
  bf_rt_id_t meter_tbl_id;
  bf_rt_id_t register_tbl_id;

  bf_status_t dataAllocate_internal(
      bf_rt_id_t action_id,
      std::unique_ptr<BfRtTableData> *data_ret,
      const std::vector<bf_rt_id_t> &fields) const;

  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags,
                                     const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     BfRtTableData *data) const;

  std::unique_ptr<BfRtMatchIdleTableState> idle_table_state;
  // We need this variable to be mutable because we dynamically create/destroy
  // the thread pool when the user sets (in NOTIFY_MODE)/resets idle timeout
  // for a table. This happens during tableAttributesSet API. However,
  // since all the APIs on our table objects are const, we need to make this
  // mutable. It is ok to make private data members mutable
  mutable std::unique_ptr<BfRtThreadPool> idletime_cb_thread_pool{nullptr};
};

class BfRtMatchActionIndirectTable : public BfRtTableObj {
 public:
  BfRtMatchActionIndirectTable(const std::string &program_name,
                               bf_rt_id_t id,
                               std::string name,
                               const size_t &size,
                               TableType table_type,
                               pipe_mat_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     table_type,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::CLEAR,
                                        TableApi::DEFAULT_ENTRY_SET,
                                        TableApi::DEFAULT_ENTRY_RESET,
                                        TableApi::DEFAULT_ENTRY_GET,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET,
                                        TableApi::GET_BY_HANDLE,
                                        TableApi::KEY_GET,
                                        TableApi::HANDLE_GET,
                                        TableApi::ADD_OR_MOD},
                     pipe_hdl),
        idle_table_state(new BfRtMatchIdleTableState()){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryAddOrMod(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 const BfRtTableKey &key,
                                 const BfRtTableData &data,
                                 bool *is_added) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableDefaultEntrySet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   const BfRtTableData &data) const override;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override;

  bf_status_t tableDefaultEntryReset(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableSizeGet(const BfRtSession &session,
                           const bf_rt_target_t &dev_tgt,
                           const uint64_t &flags,
                           size_t *count) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override;

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  // Table attributes APIs
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;
  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;
  bf_status_t registerMatUpdateCb(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const pipe_mat_update_cb &cb,
                                  const void *cookie) const override;
  // Hidden APIs
  bf_status_t getActionMbrIdFromHndl(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const pipe_adt_ent_hdl_t adt_ent_hdl,
                                     bf_rt_id_t *mbr_id) const;

  bf_status_t getGroupIdFromHndl(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const pipe_sel_grp_hdl_t sel_grp_hdl,
                                 bf_rt_id_t *grp_id) const;

  bf_status_t getActionState(const BfRtSession &session,
                             const bf_rt_target_t &dev_tgt,
                             const BfRtMatchActionIndirectTableData *data,
                             pipe_adt_ent_hdl_t *adt_entry_hdl,
                             pipe_sel_grp_hdl_t *sel_grp_hdl,
                             pipe_act_fn_hdl_t *act_fn_hdl,
                             pipe_mgr_adt_ent_data_t *ap_ent_data) const;

  bool idleTableEnabled() const override {
    return idle_table_state->isIdleTableEnabled();
  }
  bool idleTablePollMode() const override {
    return idle_table_state->isIdleTableinPollMode();
  }
  BfRtThreadPool *idletimeCbThreadPoolGet() const {
    return idletime_cb_thread_pool.get();
  }
  template <class T>
  void idletimeCbThreadPoolReset(T obj) {
    this->idletime_cb_thread_pool.reset(obj);
  }
  BfRtMatchIdleTableState *idleTableStateGet() const {
    return idle_table_state.get();
  }
  void setIsTernaryTable(const bf_dev_id_t &dev_id) override final;
  void setActionResources(const bf_dev_id_t &dev_id) override final;

 private:
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags,
                                     const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     BfRtTableData *data) const;

  bf_status_t populate_entry_data(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtMatchActionKey &match_key,
      const BfRtMatchActionIndirectTableData &match_data,
      pipe_tbl_match_spec_t *pipe_match_spec,
      pipe_action_spec_t *pipe_action_spec,
      pipe_act_fn_hdl_t *act_fn_hdl) const;

  void populate_indirect_resources(const pipe_mgr_adt_ent_data_t &ap_ent_data,
                                   pipe_action_spec_t *pipe_action_spec) const;

  std::unique_ptr<BfRtMatchIdleTableState> idle_table_state;
  // We need this variable to be mutable because we dynamically create/destroy
  // the thread pool when the user sets (in NOTIFY_MODE)/resets idle timeout
  // for a table. This happens during tableAttributesSet API. However,
  // since all the APIs on our table objects are const, we need to make this
  // mutable. It is ok to make private data members mutable
  mutable std::unique_ptr<BfRtThreadPool> idletime_cb_thread_pool{nullptr};
};

class BfRtActionTable : public BfRtTableObj {
 public:
  BfRtActionTable(const std::string &program_name,
                  const bf_rt_id_t &id,
                  const std::string &name,
                  const size_t &size,
                  const pipe_adt_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::ACTION_PROFILE,
                     std::set<TableApi>{TableApi::ADD,
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
                                        TableApi::ADD_OR_MOD},
                     pipe_hdl){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryAddOrMod(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 const BfRtTableKey &key,
                                 const BfRtTableData &data,
                                 bool *is_added) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t keyReset(BfRtTableKey *) const override;

  bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  bf_status_t dataReset(const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

  bf_status_t registerAdtUpdateCb(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const pipe_adt_update_cb &cb,
                                  const void *cookie) const override;

  bf_status_t getMbrIdFromHndl(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,

                               const pipe_adt_ent_hdl_t adt_ent_hdl,
                               bf_rt_id_t *mbr_id) const;

  bf_status_t getHdlFromMbrId(const BfRtSession &session,
                              const bf_rt_target_t &dev_tgt,
                              const bf_rt_id_t mbr_id,
                              pipe_adt_ent_hdl_t *adt_ent_hdl) const;

  bf_status_t getMbrState(const BfRtSession &session,
                          const bf_rt_target_t &dev_tgt,
                          bf_rt_id_t mbr_id,
                          pipe_act_fn_hdl_t *act_fn_hdl,
                          pipe_adt_ent_hdl_t *adt_ent_hdl,
                          pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  bf_status_t getFirstMbr(const bf_rt_target_t &dev_tgt,
                          bf_rt_id_t *first_mbr_id,
                          pipe_adt_ent_hdl_t *first_entry_hdl) const;

  bf_status_t getNextMbr(const bf_rt_target_t &dev_tgt,
                         bf_rt_id_t mbr_id,
                         bf_rt_id_t *next_mbr_id,
                         bf_rt_id_t *next_entry_hdl) const;
  bf_status_t getMbrList(
      uint16_t device_id,
      std::vector<std::pair<bf_rt_id_t, bf_dev_pipe_t>> *mbr_id_list) const;

 private:
  bf_status_t tableEntryGet_internal(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const pipe_adt_ent_hdl_t &entry_hdl,
      BfRtActionTableData *action_tbl_data) const;
};

class BfRtSelectorTable : public BfRtTableObj {
 public:
  BfRtSelectorTable(const std::string &program_name,
                    const bf_rt_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_sel_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR,
                     std::set<TableApi>{TableApi::ADD,
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
                                        TableApi::ADD_OR_MOD},
                     pipe_hdl){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryAddOrMod(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 const BfRtTableKey &key,
                                 const BfRtTableData &data,
                                 bool *is_added) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t getOneMbr(const BfRtSession &session,
                        const uint16_t device_id,
                        const pipe_sel_grp_hdl_t sel_grp_hdl,
                        pipe_adt_ent_hdl_t *member_hdl) const;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t registerSelUpdateCb(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const pipe_sel_update_cb &cb,
                                  const void *cookie) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;

  bf_status_t tableAttributesGet(
      const BfRtSession & /*session */,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;

  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t getGrpIdFromHndl(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const pipe_sel_grp_hdl_t &sel_grp_hdl,
                               bf_rt_id_t *sel_grp_id) const;

  bf_status_t getGrpHdl(const BfRtSession &session,
                        const bf_rt_target_t &dev_tgt,
                        const bf_rt_id_t sel_grp_id,
                        pipe_sel_grp_hdl_t *sel_grp_hdl) const;

  bf_status_t getActMbrIdFromHndl(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const pipe_adt_ent_hdl_t &adt_ent_hdl,
                                  bf_rt_id_t *act_mbr_id) const;

 private:
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const bf_rt_id_t &grp_id,
                                     BfRtSelectorTableData *sel_tbl_data,
                                     bool from_hw) const;

  bf_status_t processSelUpdateCbAttr(
      const BfRtTableAttributesImpl &tbl_attr_impl,
      const bf_rt_target_t &dev_tgt) const;

  bf_status_t getFirstGrp(const bf_rt_target_t &dev_tgt,
                          bf_rt_id_t *first_grp_id,
                          pipe_sel_grp_hdl_t *first_grp_hdl) const;

  bf_status_t getNextGrp(const bf_rt_target_t &dev_tgt,
                         bf_rt_id_t grp_id,
                         bf_rt_id_t *next_grp_id,
                         pipe_sel_grp_hdl_t *next_grp_hdl) const;
};

class BfRtCounterTable : public BfRtTableObj {
 public:
  BfRtCounterTable(const std::string &program_name,
                   const bf_rt_id_t &id,
                   const std::string &name,
                   const size_t &size,
                   const pipe_stat_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;
};

class BfRtMeterTable : public BfRtTableObj {
 public:
  BfRtMeterTable(const std::string &program_name,
                 const bf_rt_id_t &id,
                 const std::string &name,
                 const size_t &size,
                 const pipe_meter_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  // Table attributes APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;

  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;
};

class BfRtLPFTable : public BfRtTableObj {
 public:
  BfRtLPFTable(const std::string &program_name,
               const bf_rt_id_t &id,
               const std::string &name,
               const size_t &size,
               const pipe_meter_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;
};

class BfRtWREDTable : public BfRtTableObj {
 public:
  BfRtWREDTable(const std::string &program_name,
                const bf_rt_id_t &id,
                const std::string &name,
                const size_t &size,
                const pipe_meter_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;
};

class BfRtRegisterTable : public BfRtTableObj {
 public:
  BfRtRegisterTable(const std::string &program_name,
                    const bf_rt_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name,
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
  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;
  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;

  bf_status_t ghostTableHandleSet(const pipe_tbl_hdl_t &pipe_hdl) override;

 private:
  // Ghost table handle
  pipe_tbl_hdl_t ghost_pipe_tbl_hdl_;
};

class BfRtPVSTable : public BfRtTableObj {
 public:
  BfRtPVSTable(const std::string &program_name,
               bf_rt_id_t id,
               std::string name,
               const size_t &size,
               pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PVS,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::CLEAR,
                         TableApi::DELETE,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl){};
  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &pvs_key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &pvs_key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &pvs_key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  // dummy data obj for pvs
  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;
  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;
};

class BfRtPhase0Table : public BfRtTableObj {
 public:
  BfRtPhase0Table(const std::string &program_name,
                  bf_rt_id_t id,
                  std::string name,
                  const size_t &size,
                  pipe_mat_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;
  bf_status_t keyReset(BfRtTableKey *key) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override;

 private:
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags,
                                     const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     BfRtTableData *data) const;
};

class BfRtSnapshotConfigTable : public BfRtTableObj {
 public:
  BfRtSnapshotConfigTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size,
                          pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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
  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  bf_status_t setDataFields(const bf_snapshot_dir_t dir,
                            const bool timer_en,
                            const uint32_t timer_val,
                            const std::vector<int> &pipes,
                            const bf_snapshot_ig_mode_t mode,
                            BfRtSnapshotConfigTableData *data) const;

  bf_status_t tableEntryModInternal(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const pipe_snapshot_hdl_t &entry_hdl,
                                    const BfRtTableData &data) const;
};

class BfRtSnapshotTriggerTable : public BfRtTableObj {
 public:
  BfRtSnapshotTriggerTable(const std::string &program_name,
                           bf_rt_id_t id,
                           std::string name,
                           const size_t &size,
                           pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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
      this->direction = BF_SNAPSHOT_DIR_INGRESS;
    } else if (name.find("egress") != std::string::npos) {
      this->direction = BF_SNAPSHOT_DIR_EGRESS;
    } else {
      BF_RT_DBGCHK(0);
    }
  };
  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;
  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;
  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  bf_snapshot_dir_t direction;
  bf_status_t getHandle(const BfRtSession &session,
                        const bf_rt_target_t &dev_tgt,
                        const uint32_t &start_stage,
                        pipe_snapshot_hdl_t *entry_hdl) const;
  bf_status_t tableEntryModInternal(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const BfRtTableKey &key,
                                    const BfRtTableData &data) const;
};

class BfRtSnapshotDataTable : public BfRtTableObj {
 public:
  BfRtSnapshotDataTable(const std::string &program_name,
                        bf_rt_id_t id,
                        std::string name,
                        const size_t &size,
                        pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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
  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flags,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override;

  bf_status_t tableEntryHandleGet(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  bf_rt_handle_t *entry_handle) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  // Unpublished
  uint32_t getSize() const { return this->_table_size; };

 private:
  bf_status_t getHandle(const BfRtSession &session,
                        const bf_rt_target_t &dev_tgt,
                        const uint32_t &start_stage,
                        pipe_snapshot_hdl_t *entry_hdl) const;

  bf_status_t tableEntryGetFieldInfo(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const pipe_snapshot_hdl_t &entry_hdl,
                                     const uint32_t &stage,
                                     uint8_t *capture,
                                     const int num_captures,
                                     BfRtSnapshotDataTableData *data) const;

  void tableEntryGetControlInfo(
      const BfRtSession &session,
      const pipe_snapshot_hdl_t &entry_hdl,
      const uint32_t &stage,
      const bf_snapshot_capture_ctrl_info_arr_t &ctrl_info_arr,
      const int & /*num_captures*/,
      BfRtSnapshotDataTableData *data) const;

  bool isControlField(const BfRtTableDataField *df) const {
    return (df->getDataType() != DataType::BYTE_STREAM);
  };
  void tableEntryDataSetControlInfoNextTables(
      bf_dev_id_t dev_id,
      const bf_rt_id_t field_id,
      const char (&next_tbl_names)[BF_MAX_LOG_TBLS * BF_TBL_NAME_LEN],
      BfRtSnapshotDataTableData *data) const;
};

class BfRtSnapshotLivenessTable : public BfRtTableObj {
 public:
  BfRtSnapshotLivenessTable(const std::string &program_name,
                            bf_rt_id_t id,
                            std::string name,
                            const size_t &size,
                            pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_LIVENESS,
                     std::set<TableApi>{
                         TableApi::GET,
                     },
                     pipe_hdl) {
    if (name.find("ingress") != std::string::npos) {
      this->direction = BF_SNAPSHOT_DIR_INGRESS;
    } else if (name.find("egress") != std::string::npos) {
      this->direction = BF_SNAPSHOT_DIR_EGRESS;
    } else {
      BF_RT_DBGCHK(0);
    }
  };

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

 private:
  bf_snapshot_dir_t direction;
};

class BfRtSnapshotPhvTable : public BfRtTableObj {
 public:
  BfRtSnapshotPhvTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::SNAPSHOT_PHV,
                     std::set<TableApi>{TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  // Unpublished
  uint32_t getSize() const { return this->_table_size; };
};

class BfRtDynHashCfgTable : public BfRtTableObj {
 public:
  BfRtDynHashCfgTable(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size,
                      pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;
  bf_status_t tableDefaultEntrySet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableData &data) const override final;

  bf_status_t tableDefaultEntryReset(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags) const override final;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;
  bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;
  bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

 private:
  bf_status_t dataAllocate_internal(
      const bf_rt_id_t &container_id,
      std::unique_ptr<BfRtTableData> *data_ret,
      const std::vector<bf_rt_id_t> &fields) const;
};

class BfRtDynHashAlgoTable : public BfRtTableObj {
 public:
  BfRtDynHashAlgoTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size,
                       pipe_tbl_hdl_t pipe_hdl)
      : BfRtTableObj(program_name,
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
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;
  bf_status_t tableDefaultEntrySet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   const BfRtTableData &data) const override;

  bf_status_t tableDefaultEntryReset(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags) const override;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

 private:
  bf_status_t dataAllocate_internal(
      bf_rt_id_t action_id,
      std::unique_ptr<BfRtTableData> *data_ret,
      const std::vector<bf_rt_id_t> &fields) const;
};

class BfRtTblDbgCntTable : public BfRtTableObj {
 public:
  BfRtTblDbgCntTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  // Unpublished
  char **allocDataForTableNames() const;
  void freeDataForTableNames(char **tbl_names) const;
};

class BfRtLogDbgCntTable : public BfRtTableObj {
 public:
  BfRtLogDbgCntTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTableObj(program_name,
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

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  // Unpublished
  uint32_t getSize() const { return this->_table_size; };
};

class BfRtRegisterParamTable : public BfRtTableObj {
 public:
  BfRtRegisterParamTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::REG_PARAM,
                     std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                        TableApi::DEFAULT_ENTRY_RESET,
                                        TableApi::DEFAULT_ENTRY_GET}){};

  bf_status_t tableDefaultEntrySet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   const BfRtTableData &data) const override;

  bf_status_t tableDefaultEntryReset(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags) const override;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override;
  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

 private:
  bf_status_t getRef(pipe_tbl_hdl_t *hdl) const;
  bf_status_t getParamHdl(const BfRtSession &session, bf_dev_id_t dev_id) const;
};

class BfRtDynHashComputeTable : public BfRtTableObj {
 public:
  BfRtDynHashComputeTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::DYN_HASH_COMPUTE,
                     std::set<TableApi>{
                         TableApi::GET,
                     }){};

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

 private:
  bf_status_t getRef(pipe_tbl_hdl_t *hdl,
                     const BfRtTable **cfg_tbl,
                     uint32_t *hash_len) const;
};

class BfRtSelectorGetMemberTable : public BfRtTableObj {
 public:
  BfRtSelectorGetMemberTable(const std::string &program_name,
                             bf_rt_id_t id,
                             std::string name,
                             const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR_GET_MEMBER,
                     std::set<TableApi>{
                         TableApi::GET,
                     }){};

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

 private:
  bf_status_t getRef(pipe_sel_tbl_hdl_t *sel_tbl_id,
                     const BfRtSelectorTable **sel_tbl,
                     const BfRtActionTable **act_tbl) const;
};

}  // namespace bfrt

#endif  // _BF_RT_P4_TABLE_OBJ_HPP
