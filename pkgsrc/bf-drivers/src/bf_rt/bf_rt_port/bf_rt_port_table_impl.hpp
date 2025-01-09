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


#ifndef _BF_RT_PORT_TABLE_IMPL_HPP
#define _BF_RT_PORT_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_port_table_data_impl.hpp"
#include "bf_rt_port_table_key_impl.hpp"

namespace bfrt {

class BfRtInfo;
/*
 * BfRtPortCfgTable
 * BfRtPortStatTable
 * BfRtPortHdlInfoTable
 * BfRtPortFpIdxInfoTable
 * BfRtPortStrInfoTable
 */
class BfRtPortCfgTable : public BfRtTableObj {
 public:
  BfRtPortCfgTable(const std::string &program_name,
                   bf_rt_id_t id,
                   std::string name,
                   const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PORT_CFG,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::CLEAR,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::ADD_OR_MOD}) {
    mapInit();
  };
  bf_status_t tableEntryAdd(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryMod(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;

  bf_status_t tableEntryAddOrMod(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 const BfRtTableKey &key,
                                 const BfRtTableData &data,
                                 bool *is_added) const override;

  bf_status_t tableEntryDel(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override;

  bf_status_t tableClear(const BfRtSession & /*session*/,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t &flags) const override;

  bf_status_t tableEntryGet(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession & /*session*/,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  bf_status_t tableUsageGet(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override final;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t tableAttributesSet(
      const BfRtSession & /*session*/,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;

 private:
  bf_status_t tableEntryMod_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint32_t &dev_port,
                                     const BfRtTableData &data) const;
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint32_t &dev_port,
                                     BfRtTableData *data) const;
  std::string getStrFromSpeed(const bf_port_speed_t &speed) const {
    for (const auto &kv : speedMap) {
      if (kv.second == speed) {
        return kv.first;
      }
    }
    return "UNKNOWN";
  }
  std::string getStrFromDirection(const bf_pm_port_dir_e &dir) const {
    for (const auto &kv : portDirMap) {
      if (kv.second == dir) {
        return kv.first;
      }
    }
    return "UNKNOWN";
  }
  std::string getStrFromAN(const bf_pm_port_autoneg_policy_e &an_policy) const {
    for (const auto &kv : autonegoPolicyMap) {
      if (kv.second == an_policy) {
        return kv.first;
      }
    }
    return "UNKNOWN";
  }
  void mapInit();
  std::map<std::string, bf_port_speed_t> speedMap;
  std::map<std::string, bf_fec_type_t> fecMap;
  std::map<std::string, bf_media_type_t> mediaTypeMap;
  std::map<std::string, bf_pm_port_dir_e> portDirMap;
  std::map<std::string, bf_loopback_mode_e> loopbackModeMap;
  std::map<std::string, bf_pm_port_autoneg_policy_e> autonegoPolicyMap;
};

class BfRtPortStatTable : public BfRtTableObj {
 public:
  BfRtPortStatTable(const std::string &program_name,
                    bf_rt_id_t id,
                    std::string name,
                    const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PORT_STAT,
                     std::set<TableApi>{TableApi::MODIFY,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::CLEAR}){};

  bf_status_t tableClear(const BfRtSession & /*session*/,
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
  bf_status_t tableEntryGetFirst(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession & /*session*/,
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

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override final;

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

 private:
  bf_status_t tableEntryGet_internal(const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags,
                                     const uint32_t &dev_port,
                                     BfRtPortStatTableData *data) const;
};

class BfRtPortHdlInfoTable : public BfRtTableObj {
 public:
  BfRtPortHdlInfoTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size)
      : BfRtTableObj(
            program_name,
            id,
            name,
            size,
            TableType::PORT_HDL_INFO,
            std::set<TableApi>{
                TableApi::GET, TableApi::GET_FIRST, TableApi::GET_NEXT_N}){};
  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;
  bf_status_t tableEntryGetFirst(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;
  bf_status_t tableEntryGetNext_n(const BfRtSession & /*session*/,
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

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;
};

class BfRtPortFpIdxInfoTable : public BfRtTableObj {
 public:
  BfRtPortFpIdxInfoTable(const std::string &program_name,
                         bf_rt_id_t id,
                         std::string name,
                         const size_t &size)
      : BfRtTableObj(
            program_name,
            id,
            name,
            size,
            TableType::PORT_FRONT_PANEL_IDX_INFO,
            std::set<TableApi>{
                TableApi::GET, TableApi::GET_FIRST, TableApi::GET_NEXT_N}){};
  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;
  bf_status_t tableEntryGetFirst(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;
  bf_status_t tableEntryGetNext_n(const BfRtSession & /*session*/,
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

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;
};

class BfRtPortStrInfoTable : public BfRtTableObj {
 public:
  BfRtPortStrInfoTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size)
      : BfRtTableObj(
            program_name,
            id,
            name,
            size,
            TableType::PORT_STR_INFO,
            std::set<TableApi>{
                TableApi::GET, TableApi::GET_FIRST, TableApi::GET_NEXT_N}){};
  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override;
  bf_status_t tableEntryGetFirst(const BfRtSession & /*session*/,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;
  bf_status_t tableEntryGetNext_n(const BfRtSession & /*session*/,
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

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;
};

}  // namespace bfrt

#endif  // _BF_RT_PORT_TABLE_IMPL_HPP
