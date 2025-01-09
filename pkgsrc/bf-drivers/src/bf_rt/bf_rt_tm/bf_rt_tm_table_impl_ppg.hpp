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


#ifndef _BF_RT_TM_TABLE_IMPL_PPG_HPP
#define _BF_RT_TM_TABLE_IMPL_PPG_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMPpgTableIntf : public BfRtTMTable {
 public:
  BfRtTMPpgTableIntf(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size,
                     const TableType &type,
                     const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  ~BfRtTMPpgTableIntf() = default;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

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
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override final;

  bf_status_t tableSizeGet(const BfRtSession &session,
                           const bf_rt_target_t &dev_tgt,
                           const uint64_t &flags,
                           size_t *size) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 protected:
  bf_status_t tableGetSizeUsage(const BfRtSession &session,
                                const bf_rt_target_t &dev_tgt,
                                const uint64_t &flags,
                                size_t &table_size,
                                size_t &table_used) const;

  // Redefine for table-specific size calculation, if needed.
  virtual bf_status_t tableGetUsage(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    uint32_t &table_size,
                                    uint32_t &table_used) const;
  // Purge all the table entries.
  virtual bf_status_t tablePurge(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags) const;
  // Reset all the table entries.
  virtual bf_status_t tableReset(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags) const;
  //--- ppg_hdl
  virtual bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                                    const BfRtTMTableData &data,
                                    bf_tm_ppg_hdl ppg_hdl) const;

  virtual bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                                    bf_tm_ppg_hdl ppg_hdl,
                                    BfRtTMTableData *p_data) const;

  virtual bf_status_t tableResetEntry(const bf_rt_target_t &dev_tgt,
                                      bf_tm_ppg_hdl ppg_hdl,
                                      BfRtTMTableData *p_data) const;

  virtual bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                                      bf_tm_ppg_hdl ppg_hdl,
                                      bf_rt_id_t data_id,
                                      BfRtTMTableData *p_data) const;

  virtual bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                                    bf_tm_ppg_hdl ppg_hdl,
                                    bf_rt_id_t data_id,
                                    BfRtTMTableData *p_data) const;

  virtual bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                                    bf_tm_ppg_hdl ppg_hdl,
                                    const BfRtTMTableData &p_data,
                                    bf_rt_id_t data_id) const;

  // Resets a group of related fields and removes ids from the given list.
  virtual bf_status_t tableResetFields(const bf_rt_target_t &dev_tgt,
                                       bf_tm_ppg_hdl ppg_hdl,
                                       BfRtTMTableData *p_data,
                                       std::set<bf_rt_id_t> &wrk_fields) const;

  // Gets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableGetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_ppg_hdl ppg_hdl,
                                     BfRtTMTableData *p_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;

  // Sets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableSetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_ppg_hdl ppg_hdl,
                                     const BfRtTMTableData &p_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;
};

//----------------- TM_PPG_CFG

class BfRtTMPpgCfgTable : public BfRtTMPpgTableIntf {
 public:
  BfRtTMPpgCfgTable(const std::string &program_name,
                    bf_rt_id_t id,
                    std::string name,
                    const size_t &size)
      : BfRtTMPpgTableIntf(program_name,
                           id,
                           name,
                           size,
                           TableType::TM_PPG_CFG,
                           std::set<TableApi>{TableApi::MODIFY,
                                              TableApi::GET,
                                              TableApi::ADD,
                                              TableApi::DELETE,
                                              TableApi::CLEAR,
                                              TableApi::GET_FIRST,
                                              TableApi::GET_NEXT_N,
                                              TableApi::USAGE_GET}) {}
  ~BfRtTMPpgCfgTable() = default;

  bool actionIdApplicable() const override final { return true; }

  // Explicitly allocates a new PPG
  bf_status_t tableEntryAdd(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  // Deletes all PPG entries on CLEAR
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

 protected:
  // Adjust GetEntry action id depending on the PPG's port attached
  // and process appropriate data fields.
  bf_status_t tableGetActionPort(const bf_rt_target_t &dev_tgt,
                                 bf_tm_ppg_hdl ppg_hdl,
                                 const BfRtTMTableData &p_data,
                                 bf_rt_id_t &dev_port_action_id,
                                 bf_rt_id_t &mirror_port_action_id,
                                 bf_rt_id_t &do_action_id,
                                 bf_dev_port_t &ppg_port_id) const;

  bf_status_t tableAddActionPort(const bf_rt_target_t &dev_tgt,
                                 const BfRtTMTableData &p_data,
                                 bf_dev_port_t &ppg_port_id) const;

  bf_status_t tableGetActionFields(const bf_rt_target_t &dev_tgt,
                                   bf_tm_ppg_hdl ppg_hdl,
                                   BfRtTMTableData *p_data,
                                   std::set<bf_rt_id_t> &wrk_fields) const;

  bf_status_t tableSetActionFields(const bf_rt_target_t &dev_tgt,
                                   bf_tm_ppg_hdl ppg_hdl,
                                   const BfRtTMTableData &p_data,
                                   std::set<bf_rt_id_t> &wrk_fields) const;

  // Explicitly allocates a new PPG
  bf_status_t tableAddEntry(const bf_rt_target_t &dev_tgt,
                            const BfRtTMTableData &p_data,
                            bf_tm_ppg_id_t ppg_id) const;
  // Preprocess an entry to skip PPG if PFC is not configurable.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_ppg_hdl ppg_hdl,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related iCoS fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_ppg_hdl ppg_hdl,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                            bf_tm_ppg_hdl ppg_hdl,
                            bf_rt_id_t data_id,
                            BfRtTMTableData *p_data) const override final;
};

}  // namespace bfrt
#endif
