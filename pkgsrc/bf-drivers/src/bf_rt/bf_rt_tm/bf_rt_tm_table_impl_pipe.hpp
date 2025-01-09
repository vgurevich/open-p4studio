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


#ifndef _BF_RT_TM_TABLE_IMPL_PIPE_HPP
#define _BF_RT_TM_TABLE_IMPL_PIPE_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMPipeMulticastFifoTable : public BfRtTMTable {
 public:
  BfRtTMPipeMulticastFifoTable(const std::string &program_name,
                               bf_rt_id_t id,
                               std::string name,
                               const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_PIPE_MULTICAST_FIFO,
                    std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                       TableApi::DEFAULT_ENTRY_RESET,
                                       TableApi::DEFAULT_ENTRY_GET,
                                       TableApi::CLEAR}) {}

  ~BfRtTMPipeMulticastFifoTable() = default;

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

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

 protected:
  bf_status_t getPipeBitmap(const bf_rt_target_t &dev_tgt, uint8_t *bmap) const;

 private:
  bf_status_t tableEntrySet_internal(const bf_rt_target_t &dev_tgt,
                                     const BfRtTMTableData &data) const;
};

//---- TM_PIPE_TABLE_INTF

class BfRtTMPipeTableIntf : public BfRtTMTable {
 public:
  BfRtTMPipeTableIntf(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size,
                      const TableType &type,
                      const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  ~BfRtTMPipeTableIntf() = default;

  bf_status_t tableSizeGet(const BfRtSession &session,
                           const bf_rt_target_t &dev_tgt,
                           const uint64_t &flags,
                           size_t *size) const override final;

  // Resets default entry, or entries with PIPE_ALL
  bf_status_t tableDefaultEntryReset(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags) const override final;

  // Does the same as tableDefaultEntryReset() as keyless table common behavior
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

 protected:
  // The Pipe table has Default entry which is not constant and needs a lock.
  bool isConstDefault() const override final { return false; };

  virtual bf_status_t resetDefaultEntries(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags) const;
};

//---- TM_PIPE_CFG

class BfRtTMPipeCfgTable : public BfRtTMPipeTableIntf {
 public:
  BfRtTMPipeCfgTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTMPipeTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PIPE_CFG,
                            std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                               TableApi::DEFAULT_ENTRY_RESET,
                                               TableApi::DEFAULT_ENTRY_GET,
                                               TableApi::CLEAR}) {}
  ~BfRtTMPipeCfgTable() = default;

 protected:
  bf_status_t tableGetResetValues(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableSetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

//---- TM_PIPE_SCHED_CFG

class BfRtTMPipeSchedCfgTable : public BfRtTMPipeTableIntf {
 public:
  BfRtTMPipeSchedCfgTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTMPipeTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PIPE_SCHED_CFG,
                            std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                               TableApi::DEFAULT_ENTRY_RESET,
                                               TableApi::DEFAULT_ENTRY_GET,
                                               TableApi::CLEAR}) {}
  ~BfRtTMPipeSchedCfgTable() = default;

 protected:
  bf_status_t tableGetResetValues(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableSetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};
}  // namespace bfrt
#endif
