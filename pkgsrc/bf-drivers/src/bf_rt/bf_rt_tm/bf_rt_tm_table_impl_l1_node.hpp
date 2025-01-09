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


#ifndef _BF_RT_TM_TABLE_IMPL_L1_NODE_HPP
#define _BF_RT_TM_TABLE_IMPL_L1_NODE_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTML1NodeTableIntf : public BfRtTMTable {
 public:
  BfRtTML1NodeTableIntf(const std::string &program_name,
                        bf_rt_id_t id,
                        std::string name,
                        const size_t &size,
                        const TableType &type,
                        const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  ~BfRtTML1NodeTableIntf() = default;

  // Resets all table entries
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

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
                                 BfRtTableData *data) const override final;

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
  // Reset all the table entries.
  virtual bf_status_t tableReset(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags) const;

  virtual bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                                    const BfRtTMTableData &data,
                                    bf_tm_pg_t pg_id,
                                    bf_tm_l1_node_t pg_l1_node) const;

  virtual bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    bf_tm_l1_node_t pg_l1_node,
                                    BfRtTMTableData *wrk_data) const;

  virtual bf_status_t tableResetEntry(const bf_rt_target_t &dev_tgt,
                                      bf_tm_pg_t pg_id,
                                      bf_tm_l1_node_t pg_l1_node,
                                      BfRtTMTableData *wrk_data) const;

  virtual bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                                      bf_rt_id_t data_id,
                                      bf_tm_pg_t pg_id,
                                      bf_tm_l1_node_t pg_l1_node,
                                      BfRtTMTableData *wrk_data) const;

  virtual bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                                    bf_rt_id_t data_id,
                                    bf_tm_pg_t pg_id,
                                    bf_tm_l1_node_t pg_l1_node,
                                    BfRtTMTableData *wrk_data) const;

  virtual bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    bf_tm_l1_node_t pg_l1_node,
                                    const BfRtTMTableData &wrk_data,
                                    bf_rt_id_t data_id) const;

  // Resets a group of related fields and removes ids from the given list.
  virtual bf_status_t tableResetFields(const bf_rt_target_t &dev_tgt,
                                       bf_tm_pg_t pg_id,
                                       bf_tm_l1_node_t pg_l1_node,
                                       BfRtTMTableData *wrk_data,
                                       std::set<bf_rt_id_t> &wrk_fields) const;

  // Gets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableGetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_pg_t pg_id,
                                     bf_tm_l1_node_t pg_l1_node,
                                     BfRtTMTableData *wrk_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;

  // Sets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableSetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_pg_t pg_id,
                                     bf_tm_l1_node_t pg_l1_node,
                                     const BfRtTMTableData &wrk_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;

 protected:
  bf_status_t tmL1NodeCountGet(const bf_rt_target_t &dev_tgt,
                               uint16_t *l1_per_pipe,
                               uint8_t *l1_per_pg) const;
};

class BfRtTML1NodeSchedCfgTable : public BfRtTML1NodeTableIntf {
 public:
  BfRtTML1NodeSchedCfgTable(const std::string &program_name,
                            bf_rt_id_t id,
                            std::string name,
                            const size_t &size)
      : BfRtTML1NodeTableIntf(program_name,
                              id,
                              name,
                              size,
                              TableType::TM_L1_NODE_SCHED_CFG,
                              std::set<TableApi>{TableApi::MODIFY,
                                                 TableApi::GET,
                                                 TableApi::CLEAR,
                                                 TableApi::GET_FIRST,
                                                 TableApi::GET_NEXT_N,
                                                 TableApi::USAGE_GET}) {}
  ~BfRtTML1NodeSchedCfgTable() = default;

  bool actionIdApplicable() const override final { return true; }

 protected:
  // Resets the group of related fields with HW reset values.
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      BfRtTMTableData *wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      BfRtTMTableData *wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Sets the group of related fields at once.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      const BfRtTMTableData &wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

 private:
  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_sched_prio_t> str_to_prio;
  static const std::map<bf_tm_sched_prio_t, std::string> prio_to_str;
};

class BfRtTML1NodeSchedShapingTable : public BfRtTML1NodeTableIntf {
 public:
  BfRtTML1NodeSchedShapingTable(const std::string &program_name,
                                bf_rt_id_t id,
                                std::string name,
                                const size_t &size)
      : BfRtTML1NodeTableIntf(program_name,
                              id,
                              name,
                              size,
                              TableType::TM_L1_NODE_SCHED_SHAPING,
                              std::set<TableApi>{TableApi::MODIFY,
                                                 TableApi::GET,
                                                 TableApi::CLEAR,
                                                 TableApi::GET_FIRST,
                                                 TableApi::GET_NEXT_N,
                                                 TableApi::USAGE_GET}) {}
  ~BfRtTML1NodeSchedShapingTable() = default;

 protected:
  // Resets the group of related fields with HW reset values.
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      BfRtTMTableData *wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      BfRtTMTableData *wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Sets the group of related fields at once.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t pg_l1_node,
      const BfRtTMTableData &wrk_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};
}  // namespace bfrt
#endif
