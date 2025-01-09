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


#ifndef _TDI_PRE_TABLE_IMPL_HPP
#define _TDI_PRE_TABLE_IMPL_HPP

#include <tdi_common/tdi_table_impl.hpp>
#include "tdi_mc_mgr_intf.hpp"
#include "tdi_pre_state.hpp"
#include "tdi_pre_table_data_impl.hpp"
#include "tdi_pre_table_key_impl.hpp"

namespace tdi {

/*
 * TdiPREMGIDTable
 * TdiPREMulticastNodeTable
 * TdiPREECMPTable
 * TdiPRELAGTable
 * TdiPREMulticastPruneTable
 */

class TdiPREMGIDTable : public TdiTableObj {
 public:
  TdiPREMGIDTable(const std::string &program_name,
                  tdi_id_t id,
                  std::string name,
                  const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryModInc(const TdiSession &session,
                                const tdi_target_t &dev_tgt,
                                const uint64_t &flag,
                                const TdiTableKey &key,
                                const TdiTableData &data) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t tableEntryDel(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;

  // Attribute APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TdiTableAttributes> *attr) const override final;
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TdiTableAttributes> *attr) const override final;
  tdi_status_t tableAttributesSet(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const uint64_t &flag,
      const TdiTableAttributes &tableAttributes) const override final;

 private:
  // Helper methods for EntryAdd. EntryMod and EntryModInc
  tdi_status_t mcMGIDValidateDataMbrsSize(
      const std::vector<tdi_id_t>::size_type &nodes_size,
      const std::vector<tdi_id_t>::size_type &node_xids_size,
      const std::vector<bool>::size_type &node_xids_valid_size,
      const std::vector<tdi_id_t>::size_type &ecmps_size,
      const std::vector<tdi_id_t>::size_type &ecmp_xids_size,
      const std::vector<bool>::size_type &ecmp_xids_valid_size) const;

  tdi_status_t mcMGIDNodeMbrsAdd(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<tdi_id_t> &multicast_node_ids,
      const std::vector<bool> &node_l1_xids_valid,
      const std::vector<tdi_id_t> &node_l1_xids) const;

  tdi_status_t mcMGIDNodeMbrsDel(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<tdi_id_t> &multicast_node_ids) const;

  tdi_status_t mcMGIDEcmpMbrsAdd(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<tdi_id_t> &multicast_ecmp_ids,
      const std::vector<bool> &ecmp_l1_xids_valid,
      const std::vector<tdi_id_t> &ecmp_l1_xids) const;

  tdi_status_t mcMGIDEcmpMbrsDel(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mgid,
      const bf_mc_mgrp_hdl_t &mgrp_hdl,
      const std::vector<tdi_id_t> &multicast_ecmp_ids) const;

  // Helper methods for EntryGet

  tdi_status_t tableEntryGet_internal(const TdiSession &session,
                                      const tdi_target_t &dev_tgt,
                                      const TdiPREStateObj &pre_state,
                                      const tdi_id_t &mgid,
                                      const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                      TdiPREMGIDTableData *mgid_data) const;

  tdi_status_t mcMGIDNodeMbrsGet(const TdiSession &session,
                                 const tdi_target_t &dev_tgt,
                                 const TdiPREStateObj &pre_state,
                                 const tdi_id_t &mgid,
                                 const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                 std::vector<tdi_id_t> *multicast_node_ids,
                                 std::vector<bool> *node_l1_xids_valid,
                                 std::vector<tdi_id_t> *node_l1_xids) const;

  tdi_status_t mcMGIDECMPMbrsGet(const TdiSession &session,
                                 const tdi_target_t &dev_tgt,
                                 const TdiPREStateObj &pre_state,
                                 const tdi_id_t &mgid,
                                 const bf_mc_mgrp_hdl_t &mgrp_hdl,
                                 std::vector<tdi_id_t> *multicast_ecmp_ids,
                                 std::vector<bool> *ecmp_l1_xids_valid,
                                 std::vector<tdi_id_t> *ecmp_l1_xids) const;

  // Helper methods for EntryMod
  tdi_status_t tableEntryAdd_internal(const TdiSession &session,
                                      const tdi_target_t &dev_tgt,
                                      TdiPREStateObj &pre_state,
                                      const uint64_t &flag,
                                      const TdiTableKey &key,
                                      const TdiTableData &data,
                                      bf_mc_mgrp_hdl_t &mgrp_hdl,
                                      bf_mc_grp_id_t &mgid) const;

  tdi_status_t tableEntryDel_internal(const TdiSession &session,
                                      const tdi_target_t &dev_tgt,
                                      TdiPREStateObj &pre_state,
                                      const uint64_t &flag,
                                      const TdiTableKey &key,
                                      bf_mc_mgrp_hdl_t &mgrp_hdl,
                                      bf_mc_grp_id_t &mgid) const;
};

class TdiPREMulticastNodeTable : public TdiTableObj {
 public:
  TdiPREMulticastNodeTable(const std::string &program_name,
                           tdi_id_t id,
                           std::string name,
                           const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t tableEntryDel(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;

 private:
  // Helper methods for EntryGet
  tdi_status_t tableEntryGet_internal(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj & /*pre_state */,
      const tdi_id_t &mc_node_id,
      const bf_mc_node_hdl_t &node_hdl,
      TdiPREMulticastNodeTableData *node_data) const;
};

class TdiPREECMPTable : public TdiTableObj {
 public:
  TdiPREECMPTable(const std::string &program_name,
                  tdi_id_t id,
                  std::string name,
                  const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryModInc(const TdiSession &session,
                                const tdi_target_t &dev_tgt,
                                const uint64_t &flag,
                                const TdiTableKey &key,
                                const TdiTableData &data) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t tableEntryDel(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;

 private:
  // Helper methods for EntryAdd, EntryMod and EntryModInc
  tdi_status_t mcECMPNodeMbrsAdd(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mc_ecmp_id,
      const bf_mc_ecmp_hdl_t &ecmp_hdl,
      const std::vector<tdi_id_t> &multicast_node_ids) const;

  tdi_status_t mcECMPNodeMbrsDel(
      const TdiSession &session,
      const tdi_target_t &dev_tgt,
      const TdiPREStateObj &pre_state,
      const tdi_id_t &mc_ecmp_id,
      const bf_mc_ecmp_hdl_t &ecmp_hdl,
      const std::vector<tdi_id_t> &multicast_node_ids) const;

  // Helper methods for EntryGet
  tdi_status_t tableEntryGet_internal(const TdiSession &session,
                                      const tdi_target_t &dev_tgt,
                                      const TdiPREStateObj &pre_state,
                                      const tdi_id_t &mc_ecmp_id,
                                      const bf_mc_mgrp_hdl_t &ecmp_hdl,
                                      TdiPREECMPTableData *ecmp_data) const;
};

class TdiPRELAGTable : public TdiTableObj {
 public:
  TdiPRELAGTable(const std::string &program_name,
                 tdi_id_t id,
                 std::string name,
                 const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;
};

class TdiPREMulticastPruneTable : public TdiTableObj {
 public:
  TdiPREMulticastPruneTable(const std::string &program_name,
                            tdi_id_t id,
                            std::string name,
                            const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryDel(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;
};

class TdiPREMulticastPortTable : public TdiTableObj {
 public:
  TdiPREMulticastPortTable(const std::string &program_name,
                           tdi_id_t id,
                           std::string name,
                           const size_t &size)
      : TdiTableObj(program_name,
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

  tdi_status_t tableEntryAdd(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryMod(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             const TdiTableData &data) const override final;

  tdi_status_t tableEntryDel(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key) const override final;

  tdi_status_t tableEntryGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             const TdiTableKey &key,
                             TdiTableData *data) const override final;

  tdi_status_t tableEntryGetFirst(const TdiSession &session,
                                  const tdi_target_t &dev_tgt,
                                  const uint64_t &flag,
                                  TdiTableKey *key,
                                  TdiTableData *data) const override final;

  tdi_status_t tableEntryGetNext_n(const TdiSession &session,
                                   const tdi_target_t &dev_tgt,
                                   const uint64_t &flag,
                                   const TdiTableKey &key,
                                   const uint32_t &n,
                                   keyDataPairs *key_data_pairs,
                                   uint32_t *num_returned) const override final;

  tdi_status_t tableUsageGet(const TdiSession &session,
                             const tdi_target_t &dev_tgt,
                             const uint64_t &flag,
                             uint32_t *count) const override final;

  tdi_status_t keyAllocate(
      std::unique_ptr<TdiTableKey> *key_ret) const override final;

  tdi_status_t dataAllocate(
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<TdiTableData> *data_ret) const override final;

  tdi_status_t keyReset(TdiTableKey *key) const override final;

  tdi_status_t dataReset(TdiTableData *data) const override final;

  tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                         TdiTableData *data) const override final;
};

}  // namespace tdi

#endif  // _TDI_PRE_TABLE_IMPL_HPP
