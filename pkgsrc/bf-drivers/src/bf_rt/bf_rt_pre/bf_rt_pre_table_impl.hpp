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


#ifndef _BF_RT_PRE_TABLE_IMPL_HPP
#define _BF_RT_PRE_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_mc_mgr_intf.hpp"
#include "bf_rt_pre_state.hpp"
#include "bf_rt_pre_table_data_impl.hpp"
#include "bf_rt_pre_table_key_impl.hpp"

namespace bfrt {

/*
 * BfRtPREMGIDTable
 * BfRtPREMulticastNodeTable
 * BfRtPREECMPTable
 * BfRtPRELAGTable
 * BfRtPREMulticastPruneTable
 */

class BfRtPREMGIDTable : public BfRtTableObj {
 public:
  BfRtPREMGIDTable(const std::string &program_name,
                   bf_rt_id_t id,
                   std::string name,
                   const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_MGID,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::MODIFY_INC,
                                        TableApi::DELETE,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryModInc(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flag,
                               const BfRtTableKey &key,
                               const BfRtTableData &data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override final;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override final;
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flag,
      const BfRtTableAttributes &tableAttributes) const override final;

 private:
  // Helper methods for EntryAdd. EntryMod and EntryModInc
  bf_status_t mcMGIDValidateDataMbrsSize(
      const std::vector<bf_rt_id_t>::size_type &nodes_size,
      const std::vector<bf_rt_id_t>::size_type &node_xids_size,
      const std::vector<bool>::size_type &node_xids_valid_size,
      const std::vector<bf_rt_id_t>::size_type &ecmps_size,
      const std::vector<bf_rt_id_t>::size_type &ecmp_xids_size,
      const std::vector<bool>::size_type &ecmp_xids_valid_size) const;

  bf_status_t mcMGIDNodeMbrsAdd(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<bf_rt_id_t> &multicast_node_ids,
      const std::vector<bool> &node_l1_xids_valid,
      const std::vector<bf_rt_id_t> &node_l1_xids) const;

  bf_status_t mcMGIDNodeMbrsDel(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<bf_rt_id_t> &multicast_node_ids) const;

  bf_status_t mcMGIDEcmpMbrsAdd(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<bf_rt_id_t> &multicast_ecmp_ids,
      const std::vector<bool> &ecmp_l1_xids_valid,
      const std::vector<bf_rt_id_t> &ecmp_l1_xids) const;

  bf_status_t mcMGIDEcmpMbrsDel(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<bf_rt_id_t> &multicast_ecmp_ids) const;

  // Helper methods for EntryGet

  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const BfRtPREStateObj &pre_state,
                                     const bf_rt_id_t &mgid,
                                     const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                     BfRtPREMGIDTableData *mgid_data) const;

  bf_status_t mcMGIDNodeMbrsGet(const BfRtSession &session,
                                const bf_rt_target_t &dev_tgt,
                                const BfRtPREStateObj &pre_state,
                                const bf_rt_id_t &mgid,
                                const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                std::vector<bf_rt_id_t> *multicast_node_ids,
                                std::vector<bool> *node_l1_xids_valid,
                                std::vector<bf_rt_id_t> *node_l1_xids) const;

  bf_status_t mcMGIDECMPMbrsGet(const BfRtSession &session,
                                const bf_rt_target_t &dev_tgt,
                                const BfRtPREStateObj &pre_state,
                                const bf_rt_id_t &mgid,
                                const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                std::vector<bf_rt_id_t> *multicast_ecmp_ids,
                                std::vector<bool> *ecmp_l1_xids_valid,
                                std::vector<bf_rt_id_t> *ecmp_l1_xids) const;

  // Helper methods for EntryMod
  bf_status_t tableEntryAdd_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     BfRtPREStateObj &pre_state,
                                     const uint64_t &flag,
                                     const BfRtTableKey &key,
                                     const BfRtTableData &data,
                                     bf_mc_mgrp_hdl_t &mgrp_hdl,
                                     bf_mc_grp_id_t &mgid) const;

  bf_status_t tableEntryDel_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     BfRtPREStateObj &pre_state,
                                     const uint64_t &flag,
                                     const BfRtTableKey &key,
                                     bf_mc_mgrp_hdl_t &mgrp_hdl,
                                     bf_mc_grp_id_t &mgid) const;
};

class BfRtPREMulticastNodeTable : public BfRtTableObj {
 public:
  BfRtPREMulticastNodeTable(const std::string &program_name,
                            bf_rt_id_t id,
                            std::string name,
                            const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_NODE,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

 private:
  // Helper methods for EntryGet
  bf_status_t tableEntryGet_internal(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj & /*pre_state */,
      const bf_rt_id_t &mc_node_id,
      const bf_mc_node_hdl_t &node_hdl,
      BfRtPREMulticastNodeTableData *node_data) const;
};

class BfRtPREECMPTable : public BfRtTableObj {
 public:
  BfRtPREECMPTable(const std::string &program_name,
                   bf_rt_id_t id,
                   std::string name,
                   const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_ECMP,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::MODIFY_INC,
                                        TableApi::DELETE,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryModInc(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const uint64_t &flag,
                               const BfRtTableKey &key,
                               const BfRtTableData &data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

 private:
  // Helper methods for EntryAdd, EntryMod and EntryModInc
  bf_status_t mcECMPNodeMbrsAdd(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mc_ecmp_id,
      const bf_mc_ecmp_hdl_t &ecmp_hdl,
      const std::vector<bf_rt_id_t> &multicast_node_ids) const;

  bf_status_t mcECMPNodeMbrsDel(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtPREStateObj &pre_state,
      const bf_rt_id_t &mc_ecmp_id,
      const bf_mc_ecmp_hdl_t &ecmp_hdl,
      const std::vector<bf_rt_id_t> &multicast_node_ids) const;

  // Helper methods for EntryGet
  bf_status_t tableEntryGet_internal(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const BfRtPREStateObj &pre_state,
                                     const bf_rt_id_t &mc_ecmp_id,
                                     const bf_mc_mgrp_hdl_t &ecmp_hdl,
                                     BfRtPREECMPTableData *ecmp_data) const;
};

class BfRtPRELAGTable : public BfRtTableObj {
 public:
  BfRtPRELAGTable(const std::string &program_name,
                  bf_rt_id_t id,
                  std::string name,
                  const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_LAG,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;
};

class BfRtPREMulticastPruneTable : public BfRtTableObj {
 public:
  BfRtPREMulticastPruneTable(const std::string &program_name,
                             bf_rt_id_t id,
                             std::string name,
                             const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_PRUNE,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;
};

class BfRtPREMulticastPortTable : public BfRtTableObj {
 public:
  BfRtPREMulticastPortTable(const std::string &program_name,
                            bf_rt_id_t id,
                            std::string name,
                            const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PRE_PORT,
                     std::set<TableApi>{TableApi::ADD,
                                        TableApi::MODIFY,
                                        TableApi::DELETE,
                                        TableApi::GET,
                                        TableApi::GET_FIRST,
                                        TableApi::GET_NEXT_N,
                                        TableApi::USAGE_GET}){};

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flag,
                            uint32_t *count) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override final;
};

}  // namespace bfrt

#endif  // _BF_RT_PRE_TABLE_IMPL_HPP
