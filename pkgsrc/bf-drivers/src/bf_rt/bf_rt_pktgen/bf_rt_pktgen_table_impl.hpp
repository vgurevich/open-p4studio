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


#ifndef _BF_RT_PKTGEN_TABLE_IMPL_HPP
#define _BF_RT_PKTGEN_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_pktgen_table_data_impl.hpp"
#include "bf_rt_pktgen_table_key_impl.hpp"

namespace bfrt {

class BfRtInfo;

class PgenDataId {
 public:
  virtual bf_status_t get(const BfRtSession &session,
                          bf_dev_id_t dev,
                          bf_dev_port_t port,
                          bool *enbl) const = 0;
  virtual bf_status_t set(const BfRtSession &session,
                          bf_dev_id_t dev,
                          bf_dev_port_t port,
                          bool enbl) const = 0;
  virtual bf_status_t clear(const BfRtSession &session,
                            bf_dev_id_t dev,
                            bf_dev_port_t port) const = 0;
  virtual bf_rt_id_t id() const = 0;
};

class RecircEnableId : public PgenDataId {
 public:
  bf_status_t get(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool *enable) const override;
  bf_status_t set(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool enable) const override;
  bf_status_t clear(const BfRtSession &session,
                    bf_dev_id_t dev,
                    bf_dev_port_t port) const override;
  bf_rt_id_t id() const override;
};

class PktgenEnableId : public PgenDataId {
 public:
  bf_status_t get(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool *enable) const override;
  bf_status_t set(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool enable) const override;
  bf_status_t clear(const BfRtSession &session,
                    bf_dev_id_t dev,
                    bf_dev_port_t port) const override;
  bf_rt_id_t id() const override;
};

class RecircPatternMatchingEnableId : public PgenDataId {
 public:
  bf_status_t get(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool *enable) const override;
  bf_status_t set(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool enable) const override;
  bf_status_t clear(const BfRtSession &session,
                    bf_dev_id_t dev,
                    bf_dev_port_t port) const override;
  bf_rt_id_t id() const override;
};

class ClearPortDownEnableId : public PgenDataId {
 public:
  bf_status_t get(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool *cleared) const override;
  bf_status_t set(const BfRtSession &session,
                  bf_dev_id_t dev,
                  bf_dev_port_t port,
                  bool cleared) const override;
  bf_status_t clear(const BfRtSession &session,
                    bf_dev_id_t dev,
                    bf_dev_port_t port) const override;
  bf_rt_id_t id() const override;
};

class PgenDataIdFactory {
 public:
  static PgenDataId *create(bf_rt_id_t id);
};

/*
 * BfRtPktgenPortTable
 * BfRtPktgenAppTable
 * BfRtPktgenPortMaskTable
 * BfRtPktgenPktBufferTable
 * BfRtPktgenPortDownReplayCfgTable
 */

/* Pktgen port configuration table */
class BfRtPktgenPortTable : public BfRtTableObj {
 public:
  BfRtPktgenPortTable(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PKTGEN_PORT_CFG,
                     std::set<TableApi>{
                         TableApi::MODIFY, TableApi::GET, TableApi::CLEAR}){};

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

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
};

/* Pktgen Application configuration table */
class BfRtPktgenAppTable : public BfRtTableObj {
 public:
  BfRtPktgenAppTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PKTGEN_APP_CFG,
                     std::set<TableApi>{TableApi::MODIFY,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET,
                                        TableApi::CLEAR}){};
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

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
                            uint32_t *count) const;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

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
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags,
                                     const uint32_t &app_id,
                                     BfRtTableData *data) const;

 private:
  // patter value/mask size is fixed in a device
  static const uint32_t pattern_size_tof = 32;
  static const uint32_t pattern_size_tof2 = 128;
};

/* Pktgen Port Mask Table, Only for TOFINO2 Port down trigger Pktgen */
class BfRtPktgenPortMaskTable : public BfRtTableObj {
 public:
  BfRtPktgenPortMaskTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PKTGEN_PORT_MASK_CFG,
                     std::set<TableApi>{
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::CLEAR,
                     }){};

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

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
};

/* Pktgen packet buffer configuration table, configure the payload of generated
 * packets */
class BfRtPktgenPktBufferTable : public BfRtTableObj {
 public:
  BfRtPktgenPktBufferTable(const std::string &program_name,
                           bf_rt_id_t id,
                           std::string name,
                           const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PKTGEN_PKT_BUFF_CFG,
                     std::set<TableApi>{
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::CLEAR,
                     }){};

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

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
};

/* Pktgen port down replay configuration table, Only for port down trigger
 * Pktgen, for configuring replay mode */
class BfRtPktgenPortDownReplayCfgTable : public BfRtTableObj {
 public:
  BfRtPktgenPortDownReplayCfgTable(const std::string &program_name,
                                   bf_rt_id_t id,
                                   std::string name,
                                   const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PKTGEN_PORT_DOWN_REPLAY_CFG,
                     std::set<TableApi>{
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::CLEAR,
                     }){};

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey & /*key*/,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey & /*key*/,
                            BfRtTableData *data) const override;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;
};

}  // namespace bfrt
#endif  //_BF_RT_PKTGEN_TABLE_IMPL_HPP
