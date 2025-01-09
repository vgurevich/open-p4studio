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


#ifndef _BF_RT_TM_TABLE_IMPL_QUEUE_HPP
#define _BF_RT_TM_TABLE_IMPL_QUEUE_HPP

#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMQueueTableIntf : public BfRtTMTable {
 public:
  BfRtTMQueueTableIntf(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size,
                       const TableType &type,
                       const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  ~BfRtTMQueueTableIntf() = default;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

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
  bf_status_t getDeviceTMQueuesCount(const bf_rt_target_t &dev_tgt,
                                     uint32_t *count) const;

  virtual bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                                    const BfRtTMTableData &data,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue) const;

  virtual bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue,
                                    BfRtTMTableData *q_data) const;
  // Reset all the table entries.
  virtual bf_status_t tableReset(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags) const;

  virtual bf_status_t tableResetEntry(const bf_rt_target_t &dev_tgt,
                                      bf_tm_pg_t pg_id,
                                      uint8_t pg_queue,
                                      BfRtTMTableData *q_data) const;

  virtual bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                                      bf_rt_id_t data_id,
                                      bf_tm_pg_t pg_id,
                                      uint8_t pg_queue,
                                      bf_dev_port_t port_id,
                                      bf_tm_queue_t queue_nr,
                                      BfRtTMTableData *q_data) const;

  // Resets a group of related fields and removes ids from the given list.
  virtual bf_status_t tableResetFields(const bf_rt_target_t &dev_tgt,
                                       bf_tm_pg_t pg_id,
                                       uint8_t pg_queue,
                                       bf_dev_port_t port_id,
                                       bf_tm_queue_t queue_nr,
                                       BfRtTMTableData *q_data,
                                       std::set<bf_rt_id_t> &wrk_fields) const;

  virtual bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                                    bf_rt_id_t data_id,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue,
                                    bf_dev_port_t port_id,
                                    bf_tm_queue_t queue_nr,
                                    BfRtTMTableData *q_data) const;

  virtual bf_status_t tableGetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_pg_t pg_id,
                                     uint8_t pg_queue,
                                     bf_dev_port_t port_id,
                                     bf_tm_queue_t queue_nr,
                                     BfRtTMTableData *q_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;

  virtual bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue,
                                    bf_dev_port_t port_id,
                                    bf_tm_queue_t queue_nr,
                                    const BfRtTMTableData &q_data,
                                    bf_rt_id_t data_id) const;

  virtual bf_status_t tableSetFields(const bf_rt_target_t &dev_tgt,
                                     bf_tm_pg_t pg_id,
                                     uint8_t pg_queue,
                                     bf_dev_port_t port_id,
                                     bf_tm_queue_t queue_nr,
                                     const BfRtTMTableData &q_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;
};

class BfRtTMQueueCfgTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueCfgTable(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_CFG,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::DEFAULT_ENTRY_GET,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueCfgTable() = default;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

 protected:
  bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                            bf_rt_id_t data_id,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            bf_dev_port_t port_id,
                            bf_tm_queue_t queue_nr,
                            BfRtTMTableData *q_data) const override final;

  bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            bf_dev_port_t port_id,
                            bf_tm_queue_t queue_nr,
                            const BfRtTMTableData &q_data,
                            bf_rt_id_t data_id) const override final;
};

class BfRtTMQueueMapTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueMapTable(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_MAP,
                             std::set<TableApi>{TableApi::GET,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueMapTable() = default;

 protected:
  virtual bool isVolatile() const override final { return true; };

  bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            BfRtTMTableData *data) const override final;
};

class BfRtTMQueueColorTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueColorTable(const std::string &program_name,
                        bf_rt_id_t id,
                        std::string name,
                        const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_COLOR,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::DEFAULT_ENTRY_GET,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueColorTable() = default;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

 protected:
  bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                            const BfRtTMTableData &data,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue) const override final;

  bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            BfRtTMTableData *data) const override final;

 private:
  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_queue_color_limit_t>
      str_to_color_limit;
  static const std::map<bf_tm_queue_color_limit_t, std::string>
      color_limit_to_str;
};

class BfRtTMQueueBufferTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueBufferTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_BUFFER,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::DEFAULT_ENTRY_GET,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueBufferTable() = default;

  bool actionIdApplicable() const override final { return true; }

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override final;

 protected:
  bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                            const BfRtTMTableData &data,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue) const override final;

  bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            BfRtTMTableData *data) const override final;

 private:
  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_app_pool_t> str_to_pool;
  static const std::map<bf_tm_app_pool_t, std::string> pool_to_str;

  static const std::map<std::string, bf_tm_queue_baf_t> str_to_qbaf;
  static const std::map<bf_tm_queue_baf_t, std::string> qbaf_to_str;
};

class BfRtTMQueueSchedCfgTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueSchedCfgTable(const std::string &program_name,
                           bf_rt_id_t id,
                           std::string name,
                           const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_SCHED_CFG,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueSchedCfgTable() = default;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

 protected:
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      BfRtTMTableData *q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      BfRtTMTableData *q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      const BfRtTMTableData &q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

class BfRtTMQueueSchedShapingTable : public BfRtTMQueueTableIntf {
 public:
  BfRtTMQueueSchedShapingTable(const std::string &program_name,
                               bf_rt_id_t id,
                               std::string name,
                               const size_t &size)
      : BfRtTMQueueTableIntf(program_name,
                             id,
                             name,
                             size,
                             TableType::TM_QUEUE_SCHED_SHAPING,
                             std::set<TableApi>{TableApi::MODIFY,
                                                TableApi::GET,
                                                TableApi::CLEAR,
                                                TableApi::GET_FIRST,
                                                TableApi::GET_NEXT_N,
                                                TableApi::USAGE_GET}) {}
  ~BfRtTMQueueSchedShapingTable() = default;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override final;

 protected:
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      BfRtTMTableData *q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      BfRtTMTableData *q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_tm_pg_t pg_id,
      uint8_t pg_queue,
      bf_dev_port_t port_id,
      bf_tm_queue_t queue_nr,
      const BfRtTMTableData &q_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

}  // namespace bfrt
#endif
