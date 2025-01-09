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


#ifndef _BF_RT_TM_TABLE_IMPL_COUNTERS_HPP
#define _BF_RT_TM_TABLE_IMPL_COUNTERS_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {
class BfRtTMCounterPortTable : public BfRtTMTable {
 public:
  BfRtTMCounterPortTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size,
                         const TableType &type,
                         const std::set<TableApi> table_apis,
                         const bf_dev_direction_t &dir)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {
    this->direction = dir;
  }

  ~BfRtTMCounterPortTable() = default;

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

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 private:
  bf_status_t tableEntryMod_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_dev_port_t dev_port,
                                   const BfRtTMTableData &data) const;

  bf_status_t tableEntryGet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_dev_port_t dev_port,
                                   BfRtTableData *data) const;

  bf_status_t portDropCountGet_helper(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint64_t *count) const;

  bf_status_t portUsageGet_helper(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  uint32_t *count) const;

  bf_status_t portWatermarkGet_helper(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t *count) const;

  bf_status_t portDropCountSet_helper(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      const uint64_t &drop_count_packets) const;

  bf_status_t portWatermarktClear_helper(bf_dev_id_t dev,
                                         bf_dev_port_t port) const;

  bf_dev_direction_t direction;
};

class BfRtTMCounterIgPortTable : public BfRtTMCounterPortTable {
 public:
  BfRtTMCounterIgPortTable(const std::string &program_name,
                           bf_rt_id_t id,
                           std::string name,
                           const size_t &size)
      : BfRtTMCounterPortTable(program_name,
                               id,
                               name,
                               size,
                               TableType::TM_COUNTER_IG_PORT,
                               std::set<TableApi>{TableApi::MODIFY,
                                                  TableApi::GET,
                                                  TableApi::CLEAR,
                                                  TableApi::GET_FIRST,
                                                  TableApi::GET_NEXT_N,
                                                  TableApi::USAGE_GET},
                               BF_DEV_DIR_INGRESS) {}
  ~BfRtTMCounterIgPortTable() = default;
};

class BfRtTMCounterEgPortTable : public BfRtTMCounterPortTable {
 public:
  BfRtTMCounterEgPortTable(const std::string &program_name,
                           bf_rt_id_t id,
                           std::string name,
                           const size_t &size)
      : BfRtTMCounterPortTable(program_name,
                               id,
                               name,
                               size,
                               TableType::TM_COUNTER_EG_PORT,
                               std::set<TableApi>{TableApi::MODIFY,
                                                  TableApi::GET,
                                                  TableApi::CLEAR,
                                                  TableApi::GET_FIRST,
                                                  TableApi::GET_NEXT_N,
                                                  TableApi::USAGE_GET},
                               BF_DEV_DIR_EGRESS) {}
  ~BfRtTMCounterEgPortTable() = default;
};

class BfRtTMCounterQueueTable : public BfRtTMTable {
 public:
  BfRtTMCounterQueueTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_COUNTER_QUEUE,
                    std::set<TableApi>{TableApi::MODIFY,
                                       TableApi::GET,
                                       TableApi::CLEAR,
                                       TableApi::GET_FIRST,
                                       TableApi::GET_NEXT_N,
                                       TableApi::USAGE_GET}) {}

  ~BfRtTMCounterQueueTable() = default;

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

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 private:
  bf_status_t tableEntryMod_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_tm_pg_t &pg_id,
                                   const uint8_t &pg_queue,
                                   const BfRtTMTableData &data) const;

  bf_status_t tableEntryGet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_tm_pg_t &pg_id,
                                   const uint8_t &pg_queue,
                                   BfRtTableData *data) const;

  bf_status_t queueDropCountGet_helper(bf_dev_id_t dev,
                                       const bf_dev_port_t &dev_port,
                                       const bf_tm_queue_t &queue_nr,
                                       uint64_t *count) const;
};

class BfRtTMCounterPoolTable : public BfRtTMTable {
 public:
  BfRtTMCounterPoolTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_COUNTER_POOL,
                    std::set<TableApi>{TableApi::MODIFY,
                                       TableApi::GET,
                                       TableApi::CLEAR,
                                       TableApi::GET_FIRST,
                                       TableApi::GET_NEXT_N,
                                       TableApi::USAGE_GET}) {}

  ~BfRtTMCounterPoolTable() = default;

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

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 private:
  bf_status_t tableEntryMod_helper(const bf_rt_target_t &dev_tgt,
                                   const std::string &pool,
                                   const BfRtTMTableData &data) const;

  bf_status_t tableEntryGet_helper(const bf_rt_target_t &dev_tgt,
                                   const std::string &pool,
                                   BfRtTableData *data) const;
};

class BfRtTMCounterPipeTable : public BfRtTMTable {
 public:
  BfRtTMCounterPipeTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size)
      : BfRtTMTable(program_name,
                    id,
                    name,
                    size,
                    TableType::TM_COUNTER_PIPE,
                    std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                       TableApi::DEFAULT_ENTRY_RESET,
                                       TableApi::DEFAULT_ENTRY_GET,
                                       TableApi::CLEAR}) {}

  ~BfRtTMCounterPipeTable() = default;

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

 private:
  bf_status_t tableEntrySet_helper(const bf_rt_target_t &dev_tgt,
                                   const BfRtTMTableData &data) const;
};

class BfRtTMPpgCounterIntf : public BfRtTMTable {
 public:
  BfRtTMPpgCounterIntf(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size,
                       const TableType &type,
                       const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  virtual ~BfRtTMPpgCounterIntf() = default;

  virtual bf_status_t getCounters(const bf_rt_target_t &dev_tgt,
                                  const bf_dev_pipe_t &pipe,
                                  const bf_tm_ppg_hdl &ppg_hdl,
                                  BfRtTableData *data) const;

  bf_status_t setCounters(const bf_rt_target_t &dev_tgt,
                          const bf_tm_ppg_hdl &ppg_hdl,
                          const BfRtTableData &data) const;
};

class BfRtTMCounterPortDpgTable : public BfRtTMPpgCounterIntf {
 public:
  BfRtTMCounterPortDpgTable(const std::string &program_name,
                            bf_rt_id_t id,
                            std::string name,
                            const size_t &size)
      : BfRtTMPpgCounterIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_COUNTER_PORT_DPG,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}

  ~BfRtTMCounterPortDpgTable() = default;

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

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 private:
  bf_status_t tableEntryGet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_dev_port_t &dev_port,
                                   BfRtTableData *data) const;

  bf_status_t tableEntrySet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_dev_port_t &dev_port,
                                   const BfRtTableData &data) const;
};

class BfRtTMCounterMirrorPortDpgTable : public BfRtTMPpgCounterIntf {
 public:
  BfRtTMCounterMirrorPortDpgTable(const std::string &program_name,
                                  bf_rt_id_t id,
                                  std::string name,
                                  const size_t &size)
      : BfRtTMPpgCounterIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_COUNTER_MIRROR_PORT_DPG,
                             std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                                TableApi::DEFAULT_ENTRY_RESET,
                                                TableApi::DEFAULT_ENTRY_GET,
                                                TableApi::CLEAR}) {}

  ~BfRtTMCounterMirrorPortDpgTable() = default;

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

 private:
  bf_status_t defaultEntrySet_helper(const bf_rt_target_t &dev_tgt,
                                     const BfRtTableData &data) const;
};

class BfRtTMCounterPpgTable : public BfRtTMPpgCounterIntf {
 public:
  BfRtTMCounterPpgTable(const std::string &program_name,
                        bf_rt_id_t id,
                        std::string name,
                        const size_t &size)
      : BfRtTMPpgCounterIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_COUNTER_PPG,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}

  ~BfRtTMCounterPpgTable() = default;

  bf_status_t getCounters(const bf_rt_target_t &dev_tgt,
                          const bf_dev_pipe_t &pipe,
                          const bf_tm_ppg_hdl &ppg_hdl,
                          BfRtTableData *data) const override final;

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

  bf_status_t keyReset(BfRtTableKey *key) const override final;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

 private:
  bf_status_t tableEntryGet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_tm_ppg_id_t &ppg_counter_id,
                                   BfRtTableData *data) const;

  bf_status_t tableEntrySet_helper(const bf_rt_target_t &dev_tgt,
                                   const bf_tm_ppg_id_t &ppg_counter_id,
                                   const BfRtTableData &data) const;

  bf_status_t getPpgHdl_helper(const bf_rt_target_t &dev_tgt,
                               const bf_tm_ppg_id_t &ppg_counter_id,
                               bf_tm_ppg_id_t &ppg_id,
                               bf_tm_ppg_hdl &ppg_hdl) const;
};
}  // namespace bfrt
#endif
