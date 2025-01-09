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


#ifndef _BF_RT_TM_TABLE_IMPL_PORTGROUP_HPP
#define _BF_RT_TM_TABLE_IMPL_PORTGROUP_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMPortGroupCfgTable : public BfRtTMTable {
 public:
  BfRtTMPortGroupCfgTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_PORT_GROUP_CFG,
                    std::set<TableApi>{TableApi::DEFAULT_ENTRY_GET,
                                       TableApi::GET,
                                       TableApi::GET_FIRST,
                                       TableApi::USAGE_GET}) {}

  ~BfRtTMPortGroupCfgTable() = default;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flagsi,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flagsi,
                            uint32_t *count) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;
};

class BfRtTMPortGroupTable : public BfRtTMTable {
 public:
  BfRtTMPortGroupTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_PORT_GROUP,
                    std::set<TableApi>{TableApi::MODIFY,
                                       TableApi::DEFAULT_ENTRY_GET,
                                       TableApi::GET,
                                       TableApi::GET_FIRST,
                                       TableApi::GET_NEXT_N,
                                       TableApi::CLEAR,
                                       TableApi::USAGE_GET}) {}

  ~BfRtTMPortGroupTable() = default;

  bool actionIdApplicable() const override final { return true; }

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

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

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

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

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

 protected:
  bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                            const BfRtTMTableData &data,
                            bf_tm_pg_t pg_id) const;
};

}  // namespace bfrt
#endif
