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


#ifndef _BF_RT_TM_TABLE_IMPL_PORT_HPP
#define _BF_RT_TM_TABLE_IMPL_PORT_HPP

#include "bf_rt_pre/bf_rt_mc_mgr_intf.hpp"
#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMPortTableIntf : public BfRtTMTable {
 public:
  BfRtTMPortTableIntf(const std::string &program_name,
                      bf_rt_id_t id,
                      std::string name,
                      const size_t &size,
                      const TableType &type,
                      const std::set<TableApi> table_apis)
      : BfRtTMTable(program_name, id, name, size, type, table_apis) {}

  ~BfRtTMPortTableIntf() = default;

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

  bf_status_t validateKey(const bf_rt_target_t &dev_tgt,
                          bf_dev_port_t port_id) const;

  bf_status_t keyFirst(bf_rt_target_t &dev_tgt,
                       BfRtTableKey *key) const override final;

  bf_status_t keyNext(bf_rt_target_t &dev_tgt,
                      const BfRtTableKey &key,
                      BfRtTableKey *next_key) const override final;

 protected:
  virtual bf_status_t tableSetEntry(const bf_rt_target_t &dev_tgt,
                                    const BfRtTMTableData &data,
                                    bf_dev_port_t port_id) const;

  virtual bf_status_t tableGetEntry(const bf_rt_target_t &dev_tgt,
                                    bf_dev_port_t port_id,
                                    BfRtTMTableData *p_data) const;

  virtual bf_status_t tableResetEntry(const bf_rt_target_t &dev_tgt,
                                      bf_dev_port_t port_id,
                                      BfRtTMTableData *p_data) const;

  virtual bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                                      bf_rt_id_t data_id,
                                      bf_dev_port_t port_id,
                                      BfRtTMTableData *p_data) const;

  virtual bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                                    bf_rt_id_t data_id,
                                    bf_dev_port_t port_id,
                                    BfRtTMTableData *p_data) const;

  virtual bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                                    bf_dev_port_t port_id,
                                    const BfRtTMTableData &p_data,
                                    bf_rt_id_t data_id) const;

  // Resets a group of related fields and removes ids from the given list.
  virtual bf_status_t tableResetFields(const bf_rt_target_t &dev_tgt,
                                       bf_dev_port_t port_id,
                                       BfRtTMTableData *p_data,
                                       std::set<bf_rt_id_t> &wrk_fields) const;

  // Gets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableGetFields(const bf_rt_target_t &dev_tgt,
                                     bf_dev_port_t port_id,
                                     BfRtTMTableData *p_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;

  // Sets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableSetFields(const bf_rt_target_t &dev_tgt,
                                     bf_dev_port_t port_id,
                                     const BfRtTMTableData &p_data,
                                     std::set<bf_rt_id_t> &wrk_fields) const;
};

class BfRtTMPortCfgTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortCfgTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_CFG,
                            std::set<TableApi>{TableApi::GET,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortCfgTable() = default;

 protected:
  bool isVolatile() const override final { return true; };

  bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                            bf_rt_id_t data_id,
                            bf_dev_port_t port_id,
                            BfRtTMTableData *p_data) const override final;

 private:
  bf_status_t tableGetQueueMappingField(const bf_rt_target_t &dev_tgt,
                                        bf_rt_id_t data_id,
                                        bf_dev_port_t port_id,
                                        BfRtTMTableData *p_data) const;
};

class BfRtTMPortBufferTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortBufferTable(const std::string &program_name,
                        bf_rt_id_t id,
                        std::string name,
                        const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_BUFFER,
                            std::set<TableApi>{TableApi::MODIFY,
                                               TableApi::GET,
                                               TableApi::CLEAR,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortBufferTable() = default;

 protected:
  bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                              bf_rt_id_t data_id,
                              bf_dev_port_t port_id,
                              BfRtTMTableData *p_data) const override final;

  bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                            bf_rt_id_t data_id,
                            bf_dev_port_t port_id,
                            BfRtTMTableData *p_data) const override final;

  bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                            bf_dev_port_t port_id,
                            const BfRtTMTableData &p_data,
                            bf_rt_id_t data_id) const override final;
};

class BfRtTMPortFlowCtrlTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortFlowCtrlTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_FLOWCONTROL,
                            std::set<TableApi>{TableApi::MODIFY,
                                               TableApi::GET,
                                               TableApi::CLEAR,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortFlowCtrlTable() = default;

 protected:
  bf_status_t tableResetField(const bf_rt_target_t &dev_tgt,
                              bf_rt_id_t data_id,
                              bf_dev_port_t port_id,
                              BfRtTMTableData *p_data) const override final;

  bf_status_t tableGetField(const bf_rt_target_t &dev_tgt,
                            bf_rt_id_t data_id,
                            bf_dev_port_t port_id,
                            BfRtTMTableData *p_data) const override final;

  bf_status_t tableSetField(const bf_rt_target_t &dev_tgt,
                            bf_dev_port_t port_id,
                            const BfRtTMTableData &p_data,
                            bf_rt_id_t data_id) const override final;

 private:
  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_flow_ctrl_type_t>
      str_to_flow_ctrl_type;
  static const std::map<bf_tm_flow_ctrl_type_t, std::string>
      flow_ctrl_type_to_str;
};

//----------------- TM_PORT_DPG

class BfRtTMPortDpgTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortDpgTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_DPG,
                            std::set<TableApi>{TableApi::MODIFY,
                                               TableApi::GET,
                                               TableApi::CLEAR,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortDpgTable() = default;

 protected:
  // Resets the group of related fields with HW reset values.
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Sets the group of related fields at once.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

//------------------ TM_PORT_SCHED_CFG

class BfRtTMPortSchedCfgTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortSchedCfgTable(const std::string &program_name,
                          bf_rt_id_t id,
                          std::string name,
                          const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_SCHED_CFG,
                            std::set<TableApi>{TableApi::MODIFY,
                                               TableApi::GET,
                                               TableApi::CLEAR,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortSchedCfgTable() = default;

 protected:
  // Resets the group of related fields with HW reset values.
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Sets the group of related fields at once.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

//------------------ TM_PORT_SCHED_SHAPING

class BfRtTMPortSchedShapingTable : public BfRtTMPortTableIntf {
 public:
  BfRtTMPortSchedShapingTable(const std::string &program_name,
                              bf_rt_id_t id,
                              std::string name,
                              const size_t &size)
      : BfRtTMPortTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_PORT_SCHED_SHAPING,
                            std::set<TableApi>{TableApi::MODIFY,
                                               TableApi::GET,
                                               TableApi::CLEAR,
                                               TableApi::GET_FIRST,
                                               TableApi::GET_NEXT_N,
                                               TableApi::USAGE_GET}) {}
  ~BfRtTMPortSchedShapingTable() = default;

 protected:
  // Resets the group of related fields with HW reset values.
  bf_status_t tableResetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Gets the group of related fields at once.
  bf_status_t tableGetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  // Sets the group of related fields at once.
  bf_status_t tableSetFields(
      const bf_rt_target_t &dev_tgt,
      bf_dev_port_t port_id,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

}  // namespace bfrt
#endif
