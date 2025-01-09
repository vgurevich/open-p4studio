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


#include <tdi_common/tdi_table_impl.hpp>
#include <tdi_dev/tdi_dev_table_impl.hpp>
#include <tdi_mirror/tdi_mirror_table_impl.hpp>
#include <tdi_p4/tdi_p4_table_impl.hpp>
#include <tdi_pktgen/tdi_pktgen_table_impl.hpp>
#include <tdi_port/tdi_port_table_impl.hpp>
#include <tdi_pre/tdi_pre_table_impl.hpp>
#include <tdi_tm/tdi_tm_table_impl.hpp>
#include <tdi_tm/tdi_tm_table_impl_counters.hpp>
#include <tdi_tm/tdi_tm_table_impl_mirror.hpp>
#include <tdi_tm/tdi_tm_table_impl_pool.hpp>
#include <tdi_tm/tdi_tm_table_impl_port.hpp>
#include <tdi_tm/tdi_tm_table_impl_portgroup.hpp>
#include <tdi_tm/tdi_tm_table_impl_ppg.hpp>
#include <tdi_tm/tdi_tm_table_impl_queue.hpp>
#include <tdi_tm/tdi_tm_table_impl_l1_node.hpp>
#include <tdi_tm/tdi_tm_table_impl_cfg.hpp>
#include <tdi_tm/tdi_tm_table_impl_pipe.hpp>
#include "tdi_table_operations_impl.hpp"

namespace tdi {
std::string tdiNullStr = "";
// Base TdiTableObj ************
std::unique_ptr<TdiTableObj> TdiTableObj::make_table(
    const std::string &prog_name,
    const TableType &table_type,
    const tdi_id_t &id,
    const std::string &name,
    const size_t &size,
    const pipe_tbl_hdl_t &pipe_hdl) {
  switch (table_type) {
    case TableType::MATCH_DIRECT:
      return std::unique_ptr<TdiTableObj>(
          new TdiMatchActionTable(prog_name, id, name, size, pipe_hdl));
    case TableType::MATCH_INDIRECT:
    case TableType::MATCH_INDIRECT_SELECTOR:
      return std::unique_ptr<TdiTableObj>(new TdiMatchActionIndirectTable(
          prog_name, id, name, size, table_type, pipe_hdl));
    case TableType::ACTION_PROFILE:
      return std::unique_ptr<TdiTableObj>(
          new TdiActionTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SELECTOR:
      return std::unique_ptr<TdiTableObj>(
          new TdiSelectorTable(prog_name, id, name, size, pipe_hdl));
    case TableType::COUNTER:
      return std::unique_ptr<TdiTableObj>(
          new TdiCounterTable(prog_name, id, name, size, pipe_hdl));
    case TableType::METER:
      return std::unique_ptr<TdiTableObj>(
          new TdiMeterTable(prog_name, id, name, size, pipe_hdl));
    case TableType::LPF:
      return std::unique_ptr<TdiTableObj>(
          new TdiLPFTable(prog_name, id, name, size, pipe_hdl));
    case TableType::WRED:
      return std::unique_ptr<TdiTableObj>(
          new TdiWREDTable(prog_name, id, name, size, pipe_hdl));
    case TableType::REGISTER:
      return std::unique_ptr<TdiTableObj>(
          new TdiRegisterTable(prog_name, id, name, size, pipe_hdl));
    case TableType::PVS:
      return std::unique_ptr<TdiTableObj>(
          new TdiPVSTable(prog_name, id, name, size, pipe_hdl));
    case TableType::PORT_METADATA:
      return std::unique_ptr<TdiTableObj>(
          new TdiPhase0Table(prog_name, id, name, size, pipe_hdl));
    case TableType::DYN_HASH_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiDynHashCfgTable(prog_name, id, name, size, pipe_hdl));
    case TableType::DYN_HASH_ALGO:
      return std::unique_ptr<TdiTableObj>(
          new TdiDynHashAlgoTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SNAPSHOT_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiSnapshotConfigTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SNAPSHOT_DATA:
      return std::unique_ptr<TdiTableObj>(
          new TdiSnapshotDataTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SNAPSHOT_TRIG:
      return std::unique_ptr<TdiTableObj>(
          new TdiSnapshotTriggerTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SNAPSHOT_LIVENESS:
      return std::unique_ptr<TdiTableObj>(
          new TdiSnapshotLivenessTable(prog_name, id, name, size, pipe_hdl));
    default:
      break;
  }
  return nullptr;
}

std::unique_ptr<TdiTableObj> TdiTableObj::make_table(
    const std::string &prog_name,
    const TableType &table_type,
    const tdi_id_t &id,
    const std::string &name,
    const size_t &size) {
  switch (table_type) {
    case TableType::PORT_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPortCfgTable(prog_name, id, name, size));
    case TableType::RECIRC_PORT_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiRecircPortCfgTable(prog_name, id, name, size));
    case TableType::PORT_STAT:
      return std::unique_ptr<TdiTableObj>(
          new TdiPortStatTable(prog_name, id, name, size));
    case TableType::PORT_HDL_INFO:
      return std::unique_ptr<TdiTableObj>(
          new TdiPortHdlInfoTable(prog_name, id, name, size));
    case TableType::PORT_FRONT_PANEL_IDX_INFO:
      return std::unique_ptr<TdiTableObj>(
          new TdiPortFpIdxInfoTable(prog_name, id, name, size));
    case TableType::PORT_STR_INFO:
      return std::unique_ptr<TdiTableObj>(
          new TdiPortStrInfoTable(prog_name, id, name, size));
    case TableType::PKTGEN_PORT_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPktgenPortTable(prog_name, id, name, size));
    case TableType::PKTGEN_APP_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPktgenAppTable(prog_name, id, name, size));
    case TableType::PKTGEN_PKT_BUFF_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPktgenPktBufferTable(prog_name, id, name, size));
    case TableType::PKTGEN_PORT_MASK_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPktgenPortMaskTable(prog_name, id, name, size));
    case TableType::PKTGEN_PORT_DOWN_REPLAY_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPktgenPortDownReplayCfgTable(prog_name, id, name, size));
    case TableType::PRE_MGID:
      return std::unique_ptr<TdiTableObj>(
          new TdiPREMGIDTable(prog_name, id, name, size));
    case TableType::PRE_NODE:
      return std::unique_ptr<TdiTableObj>(
          new TdiPREMulticastNodeTable(prog_name, id, name, size));
    case TableType::PRE_ECMP:
      return std::unique_ptr<TdiTableObj>(
          new TdiPREECMPTable(prog_name, id, name, size));
    case TableType::PRE_LAG:
      return std::unique_ptr<TdiTableObj>(
          new TdiPRELAGTable(prog_name, id, name, size));
    case TableType::PRE_PRUNE:
      return std::unique_ptr<TdiTableObj>(
          new TdiPREMulticastPruneTable(prog_name, id, name, size));
    case TableType::MIRROR_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiMirrorCfgTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueCfgTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_MAP:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueMapTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_COLOR:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueColorTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_BUFFER:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueBufferTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_SCHED_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueSchedCfgTable(prog_name, id, name, size));
    case TableType::TM_QUEUE_SCHED_SHAPING:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMQueueSchedShapingTable(prog_name, id, name, size));
    case TableType::TM_L1_NODE_SCHED_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTML1NodeSchedCfgTable(prog_name, id, name, size));
    case TableType::TM_L1_NODE_SCHED_SHAPING:
      return std::unique_ptr<TdiTableObj>(
          new TdiTML1NodeSchedShapingTable(prog_name, id, name, size));
    case TableType::TM_PIPE_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPipeCfgTable(prog_name, id, name, size));
    case TableType::TM_PIPE_SCHED_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPipeSchedCfgTable(prog_name, id, name, size));
    case TableType::TM_PORT_SCHED_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortSchedCfgTable(prog_name, id, name, size));
    case TableType::TM_PORT_SCHED_SHAPING:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortSchedShapingTable(prog_name, id, name, size));
    case TableType::TM_MIRROR_DPG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMMirrorDpgTable(prog_name, id, name, size));
    case TableType::TM_PORT_DPG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortDpgTable(prog_name, id, name, size));
    case TableType::TM_PPG_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPpgCfgTable(prog_name, id, name, size));
    case TableType::TM_PORT_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortCfgTable(prog_name, id, name, size));
    case TableType::TM_PORT_BUFFER:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortBufferTable(prog_name, id, name, size));
    case TableType::TM_PORT_FLOWCONTROL:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortFlowCtrlTable(prog_name, id, name, size));
    case TableType::TM_PORT_GROUP_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortGroupCfgTable(prog_name, id, name, size));
    case TableType::TM_PORT_GROUP:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPortGroupTable(prog_name, id, name, size));
    case TableType::PRE_PORT:
      return std::unique_ptr<TdiTableObj>(
          new TdiPREMulticastPortTable(prog_name, id, name, size));
    case TableType::TM_POOL_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPoolCfgTable(prog_name, id, name, size));
    case TableType::TM_POOL_SKID:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPoolSkidTable(prog_name, id, name, size));
    case TableType::SNAPSHOT_PHV:
      return std::unique_ptr<TdiTableObj>(
          new TdiSnapshotPhvTable(prog_name, id, name, size));
    case TableType::DEV_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiDevTable(prog_name, id, name, size));
    case TableType::DEV_WARM_INIT:
      return std::unique_ptr<TdiTableObj>(
          new TdiDevWarmInitTable(prog_name, id, name, size));
    case TableType::TM_POOL_APP:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPoolAppTable(prog_name, id, name, size));
    case TableType::TM_POOL_COLOR:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPoolColorTable(prog_name, id, name, size));
    case TableType::TM_POOL_APP_PFC:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPoolAppPfcTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_IG_PORT:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterIgPortTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_EG_PORT:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterEgPortTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_QUEUE:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterQueueTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_POOL:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterPoolTable(prog_name, id, name, size));
    case TableType::DBG_CNT:
      return std::unique_ptr<TdiTableObj>(
          new TdiTblDbgCntTable(prog_name, id, name, size));
    case TableType::LOG_DBG_CNT:
      return std::unique_ptr<TdiTableObj>(
          new TdiLogDbgCntTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_PIPE:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterPipeTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_PORT_DPG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterPortDpgTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_MIRROR_PORT_DPG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterMirrorPortDpgTable(prog_name, id, name, size));
    case TableType::TM_COUNTER_PPG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCounterPpgTable(prog_name, id, name, size));
    case TableType::TM_CFG:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMCfgTable(prog_name, id, name, size));
    case TableType::TM_PIPE_MULTICAST_FIFO:
      return std::unique_ptr<TdiTableObj>(
          new TdiTMPipeMulticastFifoTable(prog_name, id, name, size));
    case TableType::REG_PARAM:
      return std::unique_ptr<TdiTableObj>(
          new TdiRegisterParamTable(prog_name, id, name, size));
    case TableType::DYN_HASH_COMPUTE:
      return std::unique_ptr<TdiTableObj>(
          new TdiDynHashComputeTable(prog_name, id, name, size));
    case TableType::SELECTOR_GET_MEMBER:
      return std::unique_ptr<TdiTableObj>(
          new TdiSelectorGetMemberTable(prog_name, id, name, size));
    default:
      break;
  }
  return nullptr;
}

bool Annotation::operator<(const Annotation &other) const {
  return (this->full_name_ < other.full_name_);
}
bool Annotation::operator==(const Annotation &other) const {
  return (this->full_name_ == other.full_name_);
}
bool Annotation::operator==(const std::string &other_str) const {
  return (this->full_name_ == other_str);
}
tdi_status_t Annotation::fullNameGet(std::string *full_name) const {
  *full_name = full_name_;
  return TDI_SUCCESS;
}

// Beginning of old flags wrappers section.
tdi_status_t TdiTableObj::tableEntryAdd(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTableKey &key,
                                        const TdiTableData &data) const {
  return this->tableEntryAdd(session, dev_tgt, 0, key, data);
}

tdi_status_t TdiTableObj::tableEntryMod(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTableKey &key,
                                        const TdiTableData &data) const {
  return this->tableEntryMod(session, dev_tgt, 0, key, data);
}

tdi_status_t TdiTableObj::tableEntryModInc(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTableKey &key,
    const TdiTableData &data,
    const TdiTable::TdiTableModIncFlag &flag) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableModIncFlag::MOD_INC_DELETE) {
    TDI_FLAG_SET(flags, TDI_INC_DEL);
  }
  return this->tableEntryModInc(session, dev_tgt, flags, key, data);
}

tdi_status_t TdiTableObj::tableEntryDel(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTableKey &key) const {
  return this->tableEntryDel(session, dev_tgt, 0, key);
}

tdi_status_t TdiTableObj::tableClear(const tdi::Session &session,
                                     const tdi_target_t &dev_tgt) const {
  return this->tableClear(session, dev_tgt, 0);
}

tdi_status_t TdiTableObj::tableDefaultEntrySet(const tdi::Session &session,
                                               const tdi_target_t &dev_tgt,
                                               const TdiTableData &data) const {
  return this->tableDefaultEntrySet(session, dev_tgt, 0, data);
}

tdi_status_t TdiTableObj::tableDefaultEntryReset(
    const tdi::Session &session, const tdi_target_t &dev_tgt) const {
  return this->tableDefaultEntryReset(session, dev_tgt, 0);
}

tdi_status_t TdiTableObj::tableDefaultEntryGet(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTable::TdiTableGetFlag &flag,
    TdiTableData *data) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableDefaultEntryGet(session, dev_tgt, flags, data);
}

tdi_status_t TdiTableObj::tableEntryGet(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTableKey &key,
                                        const TdiTable::TdiTableGetFlag &flag,
                                        TdiTableData *data) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableEntryGet(session, dev_tgt, flags, key, data);
}

tdi_status_t TdiTableObj::tableEntryGetFirst(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTable::TdiTableGetFlag &flag,
    TdiTableKey *key,
    TdiTableData *data) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableEntryGetFirst(session, dev_tgt, flags, key, data);
}

tdi_status_t TdiTableObj::tableEntryGet(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTable::TdiTableGetFlag &flag,
                                        const tdi_handle_t &entry_handle,
                                        TdiTableKey *key,
                                        TdiTableData *data) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableEntryGet(session, dev_tgt, flags, entry_handle, key, data);
}

tdi_status_t TdiTableObj::tableEntryKeyGet(const tdi::Session &session,
                                           const tdi_target_t &dev_tgt,
                                           const tdi_handle_t &entry_handle,
                                           tdi_target_t *entry_tgt,
                                           TdiTableKey *key) const {
  return this->tableEntryKeyGet(
      session, dev_tgt, 0, entry_handle, entry_tgt, key);
}

tdi_status_t TdiTableObj::tableEntryHandleGet(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTableKey &key,
    tdi_handle_t *entry_handle) const {
  return this->tableEntryHandleGet(session, dev_tgt, 0, key, entry_handle);
}

tdi_status_t TdiTableObj::tableEntryGetNext_n(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTableKey &key,
    const uint32_t &n,
    const TdiTable::TdiTableGetFlag &flag,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableEntryGetNext_n(
      session, dev_tgt, flags, key, n, key_data_pairs, num_returned);
}

tdi_status_t TdiTableObj::tableSizeGet(const tdi::Session &session,
                                       const tdi_target_t &dev_tgt,
                                       size_t *size) const {
  return this->tableSizeGet(session, dev_tgt, 0, size);
}

tdi_status_t TdiTableObj::tableUsageGet(const tdi::Session &session,
                                        const tdi_target_t &dev_tgt,
                                        const TdiTable::TdiTableGetFlag &flag,
                                        uint32_t *count) const {
  uint64_t flags = 0;
  if (flag == TdiTable::TdiTableGetFlag::GET_FROM_HW) {
    TDI_FLAG_SET(flags, TDI_FROM_HW);
  }
  return this->tableUsageGet(session, dev_tgt, flags, count);
}
// End of old flags wrappers section.

tdi_status_t TdiTableObj::tableNameGet(std::string *name) const {
  if (name == nullptr) {
    LOG_ERROR("%s:%d %s ERROR Outparam passed null",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  *name = object_name;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableIdGet(tdi_id_t *id) const {
  if (id == nullptr) {
    LOG_ERROR("%s:%d %s ERROR Outparam passed null",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  *id = object_id;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableTypeGet(TdiTable::TableType *table_type) const {
  if (nullptr == table_type) {
    LOG_ERROR("%s:%d Outparam passed is nullptr", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  *table_type = object_type;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableEntryAdd(const tdi::Session & /*session*/,
                                        const tdi_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const TdiTableKey & /*key*/,
                                        const TdiTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Entry add not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryMod(const tdi::Session & /*session*/,
                                        const tdi_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const TdiTableKey & /*key*/,
                                        const TdiTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry mod not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryModInc(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const TdiTableKey & /*key*/,
    const TdiTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry modify incremental not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryDel(const tdi::Session & /*session*/,
                                        const tdi_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const TdiTableKey & /*key*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry Delete not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableClear(const tdi::Session & /*session*/,
                                     const tdi_target_t & /*dev_tgt*/,
                                     const uint64_t & /*flags*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Clear not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableDefaultEntrySet(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const TdiTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry set not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableDefaultEntryReset(
    const tdi::Session & /* session */,
    const tdi_target_t & /* dev_tgt */,
    const uint64_t & /*flags*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableDefaultEntryGet(
    const tdi::Session & /* session */,
    const tdi_target_t & /* dev_tgt */,
    const uint64_t & /*flags*/,
    TdiTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry get not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryGet(const tdi::Session & /*session */,
                                        const tdi_target_t & /* dev_tgt */,
                                        const uint64_t & /*flags*/,
                                        const TdiTableKey & /* &key */,
                                        TdiTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryGetFirst(const tdi::Session & /*session*/,
                                             const tdi_target_t & /*dev_tgt*/,
                                             const uint64_t & /*flags*/,
                                             TdiTableKey * /*key*/,
                                             TdiTableData * /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get first not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryGet(const tdi::Session & /*session*/,
                                        const tdi_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const tdi_handle_t & /*entry_handle*/,
                                        TdiTableKey * /*key*/,
                                        TdiTableData * /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get by handle not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryKeyGet(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const tdi_handle_t & /*entry_handle*/,
    tdi_target_t * /*entry_tgt*/,
    TdiTableKey * /*key*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get key not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryHandleGet(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const TdiTableKey & /*key*/,
    tdi_handle_t * /*entry_handle*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get handle not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableEntryGetNext_n(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const TdiTableKey & /*key*/,
    const uint32_t & /*n*/,
    keyDataPairs * /*key_data_pairs*/,
    uint32_t * /*num_returned*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get next_n not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableUsageGet(const tdi::Session & /*session*/,
                                        const tdi_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        uint32_t * /*count*/) const {
  LOG_ERROR(
      "%s:%d %s Not supported", __func__, __LINE__, table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableSizeGet(const tdi::Session & /*session*/,
                                       const tdi_target_t & /*dev_tgt*/,
                                       const uint64_t & /*flags*/,
                                       size_t *size) const {
  if (nullptr == size) {
    LOG_ERROR("%s:%d Outparam passed is nullptr", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  *size = this->_table_size;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableHasConstDefaultAction(
    bool *has_const_default_action) const {
  *has_const_default_action = this->has_const_default_action_;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableIsConst(bool *is_const) const {
  *is_const = this->is_const_table_;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableAnnotationsGet(
    AnnotationSet *annotations) const {
  for (const auto &annotation : this->annotations_) {
    (*annotations).insert(std::cref(annotation));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableApiSupportedGet(TableApiSet *tableApis) const {
  for (const auto &api : this->table_apis_) {
    tableApis->insert(std::cref(api));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyAllocate(
    std::unique_ptr<TdiTableKey> * /*key_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Key allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::keyReset(TdiTableKey * /* key */) const {
  LOG_ERROR("%s:%d %s ERROR : Table Key reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocate(
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocate(
    const tdi_id_t & /*action_id*/,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocate(
    const std::vector<tdi_id_t> & /*fields*/,
    const tdi_id_t & /* action_id */,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocate(
    const std::vector<tdi_id_t> & /* fields */,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocateContainer(
    const tdi_id_t & /*container_id*/,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocateContainer(
    const tdi_id_t & /*container_id*/,
    const tdi_id_t & /*action_id*/,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocateContainer(
    const tdi_id_t & /*container_id*/,
    const std::vector<tdi_id_t> & /*fields*/,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataAllocateContainer(
    const tdi_id_t & /*container_id*/,
    const std::vector<tdi_id_t> & /*fields*/,
    const tdi_id_t & /*action_id*/,
    std::unique_ptr<TdiTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataReset(TdiTableData * /*data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataReset(const tdi_id_t & /* action_id */,
                                    TdiTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataReset(const std::vector<tdi_id_t> & /* fields */,
                                    TdiTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataReset(const std::vector<tdi_id_t> & /* fields */,
                                    const tdi_id_t & /* action_id */,
                                    TdiTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::attributeAllocate(
    const TableAttributesType & /*type*/,
    std::unique_ptr<TdiTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::attributeAllocate(
    const TableAttributesType & /*type*/,
    const TableAttributesIdleTableMode & /*idle_table_mode*/,
    std::unique_ptr<TdiTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::attributeReset(
    const TableAttributesType & /*type*/,
    std::unique_ptr<TdiTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::attributeReset(
    const TableAttributesType & /*type*/,
    const TableAttributesIdleTableMode & /*idle_table_mode*/,
    std::unique_ptr<TdiTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::operationsAllocate(
    const TableOperationsType &op_type,
    std::unique_ptr<TdiTableOperations> *table_ops) const {
  auto op_found = operations_type_set.find(op_type);
  if (op_found == operations_type_set.end()) {
    *table_ops = nullptr;
    LOG_ERROR("%s:%d %s Operation not supported for this table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_NOT_SUPPORTED;
  }

  *table_ops = std::unique_ptr<TdiTableOperations>(
      new TdiTableOperationsImpl(this, op_type));
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldIdListGet(
    std::vector<tdi_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  for (const auto &kv : key_fields) {
    id_vec->push_back(kv.first);
  }
  std::sort(id_vec->begin(), id_vec->end());
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldTypeGet(const tdi_id_t &field_id,
                                          KeyFieldType *field_type) const {
  if (field_type == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_type = key_fields.at(field_id)->getType();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldDataTypeGet(const tdi_id_t &field_id,
                                              DataType *data_type) const {
  if (data_type == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *data_type = key_fields.at(field_id)->getDataType();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldIdGet(const std::string &name,
                                        tdi_id_t *field_id) const {
  if (field_id == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  auto found = std::find_if(
      key_fields.begin(),
      key_fields.end(),
      [&name](const std::pair<const tdi_id_t, std::unique_ptr<TdiTableKeyField>>
                  &map_item) { return (name == map_item.second->getName()); });
  if (found != key_fields.end()) {
    *field_id = (*found).second->getId();
    return TDI_SUCCESS;
  }

  LOG_ERROR("%s:%d %s Field \"%s\" not found in key field list",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            name.c_str());
  return TDI_OBJECT_NOT_FOUND;
}

tdi_status_t TdiTableObj::keyFieldSizeGet(const tdi_id_t &field_id,
                                          size_t *size) const {
  if (size == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *size = key_fields.at(field_id)->getSize();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldIsPtrGet(const tdi_id_t &field_id,
                                           bool *is_ptr) const {
  if (is_ptr == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *is_ptr = key_fields.at(field_id)->isPtr();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldNameGet(const tdi_id_t &field_id,
                                          std::string *name) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *name = key_fields.at(field_id)->getName();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::keyFieldAllowedChoicesGet(
    const tdi_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  DataType data_type;
  tdi_status_t sts = this->keyFieldDataTypeGet(field_id, &data_type);
  if (sts != TDI_SUCCESS) {
    return sts;
  }
  if (data_type != DataType::STRING) {
    LOG_ERROR("%s:%d %s This API is valid only for fields of type STRING",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  choices->clear();
  for (const auto &iter : this->key_fields.at(field_id)->getEnumChoices()) {
    choices->push_back(std::cref(iter));
  }
  return TDI_SUCCESS;
}

uint32_t TdiTableObj::dataFieldListSize() const {
  return this->dataFieldListSize(0);
}

uint32_t TdiTableObj::dataFieldListSize(const tdi_id_t &action_id) const {
  if (action_info_list.find(action_id) != action_info_list.end()) {
    return action_info_list.at(action_id)->data_fields.size() +
           common_data_fields.size();
  }
  return common_data_fields.size();
}

tdi_status_t TdiTableObj::dataFieldIdListGet(
    const tdi_id_t &action_id, std::vector<tdi_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return TDI_INVALID_ARG;
  }
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_ERROR("%s:%d %s Action Id %d Not Found",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                action_id);
      return TDI_OBJECT_NOT_FOUND;
    }
    for (const auto &kv : action_info_list.at(action_id)->data_fields) {
      id_vec->push_back(kv.first);
    }
  }
  // Also include common data fields
  for (const auto &kv : common_data_fields) {
    id_vec->push_back(kv.first);
  }
  std::sort(id_vec->begin(), id_vec->end());
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldIdListGet(
    std::vector<tdi_id_t> *id_vec) const {
  return this->dataFieldIdListGet(0, id_vec);
}

tdi_status_t TdiTableObj::containerDataFieldIdListGet(
    const tdi_id_t &field_id, std::vector<tdi_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, 0, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d not found", __func__, __LINE__, field_id);
    return status;
  }
  return field->containerFieldIdListGet(id_vec);
}

tdi_status_t TdiTableObj::dataFieldIdGet(const std::string &name,
                                         tdi_id_t *field_id) const {
  return this->dataFieldIdGet(name, 0, field_id);
}

tdi_status_t TdiTableObj::dataFieldIdGet(const std::string &name,
                                         const tdi_id_t &action_id,
                                         tdi_id_t *field_id) const {
  if (field_id == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  std::vector<tdi_id_t> empty;
  auto status = getDataField(name, action_id, empty, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field name %s Action ID %d not found",
              __func__,
              __LINE__,
              name.c_str(),
              action_id);
    return status;
  }
  *field_id = field->getId();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldSizeGet(const tdi_id_t &field_id,
                                           size_t *size) const {
  return this->dataFieldSizeGet(field_id, 0, size);
}

tdi_status_t TdiTableObj::dataFieldSizeGet(const tdi_id_t &field_id,
                                           const tdi_id_t &action_id,
                                           size_t *size) const {
  if (size == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  if (field->isContainerValid()) {
    *size = field->containerSizeGet();
  } else {
    *size = field->getSize();
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldIsPtrGet(const tdi_id_t &field_id,
                                            bool *is_ptr) const {
  return this->dataFieldIsPtrGet(field_id, 0, is_ptr);
}

tdi_status_t TdiTableObj::dataFieldIsPtrGet(const tdi_id_t &field_id,
                                            const tdi_id_t &action_id,
                                            bool *is_ptr) const {
  if (is_ptr == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_ptr = field->isPtr();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldMandatoryGet(const tdi_id_t &field_id,
                                                bool *is_mandatory) const {
  return this->dataFieldMandatoryGet(field_id, 0, is_mandatory);
}

tdi_status_t TdiTableObj::dataFieldMandatoryGet(const tdi_id_t &field_id,
                                                const tdi_id_t &action_id,
                                                bool *is_mandatory) const {
  if (is_mandatory == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_mandatory = field->isMandatory();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldReadOnlyGet(const tdi_id_t &field_id,
                                               bool *is_read_only) const {
  return this->dataFieldReadOnlyGet(field_id, 0, is_read_only);
}

tdi_status_t TdiTableObj::dataFieldReadOnlyGet(const tdi_id_t &field_id,
                                               const tdi_id_t &action_id,
                                               bool *is_read_only) const {
  if (is_read_only == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_read_only = field->isReadOnly();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldOneofSiblingsGet(
    const tdi_id_t &field_id, std::set<tdi_id_t> *oneof_siblings) const {
  return this->dataFieldOneofSiblingsGet(field_id, 0, oneof_siblings);
}

tdi_status_t TdiTableObj::dataFieldOneofSiblingsGet(
    const tdi_id_t &field_id,
    const tdi_id_t &action_id,
    std::set<tdi_id_t> *oneof_siblings) const {
  if (oneof_siblings == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *oneof_siblings = field->oneofSiblings();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldNameGet(const tdi_id_t &field_id,
                                           std::string *name) const {
  return this->dataFieldNameGet(field_id, 0, name);
}

tdi_status_t TdiTableObj::dataFieldNameGet(const tdi_id_t &field_id,
                                           const tdi_id_t &action_id,
                                           std::string *name) const {
  if (name == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *name = field->getName();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldDataTypeGet(const tdi_id_t &field_id,
                                               DataType *type) const {
  return this->dataFieldDataTypeGet(field_id, 0, type);
}

tdi_status_t TdiTableObj::dataFieldDataTypeGet(const tdi_id_t &field_id,
                                               const tdi_id_t &action_id,
                                               DataType *type) const {
  if (type == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *type = field->getDataType();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldAllowedChoicesGet(
    const tdi_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  return dataFieldAllowedChoicesGet(field_id, 0, choices);
}

tdi_status_t TdiTableObj::dataFieldAllowedChoicesGet(
    const tdi_id_t &field_id,
    const tdi_id_t &action_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  if (choices == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  if (field->getDataType() != DataType::STRING &&
      field->getDataType() != DataType::STRING_ARR) {
    LOG_ERROR("%s:%d %s API valid only for fields of type STRING field ID:%d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  choices->clear();
  for (const auto &iter : field->getEnumChoices()) {
    choices->push_back(std::cref(iter));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::dataFieldAnnotationsGet(
    const tdi_id_t &field_id, AnnotationSet *annotations) const {
  return this->dataFieldAnnotationsGet(field_id, 0, annotations);
}

tdi_status_t TdiTableObj::dataFieldAnnotationsGet(
    const tdi_id_t &field_id,
    const tdi_id_t &action_id,
    AnnotationSet *annotations) const {
  if (annotations == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  for (const auto &annotation : field->getAnnotations()) {
    (*annotations).insert(std::cref(annotation));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::defaultDataValueGet(const tdi_id_t &field_id,
                                              const tdi_id_t action_id,
                                              uint64_t *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *default_value = field->defaultValueGet();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::defaultDataValueGet(const tdi_id_t &field_id,
                                              const tdi_id_t action_id,
                                              float *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *tableDataField = nullptr;
  tdi_status_t status = TDI_SUCCESS;
  status = getDataField(field_id, action_id, &tableDataField);

  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Field id %d not found for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id,
              action_id);
    return status;
  }

  *default_value = tableDataField->defaultFlValueGet();

  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::defaultDataValueGet(
    const tdi_id_t &field_id,
    const tdi_id_t action_id,
    std::string *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *tableDataField = nullptr;
  tdi_status_t status = TDI_SUCCESS;
  status = getDataField(field_id, action_id, &tableDataField);

  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Field id %d not found for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id,
              action_id);
    return status;
  }

  *default_value = tableDataField->defaultStrValueGet();

  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::actionIdGet(const std::string &name,
                                      tdi_id_t *action_id) const {
  if (action_id == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }
  auto action_found = std::find_if(
      action_info_list.begin(),
      action_info_list.end(),
      [&name](const std::pair<const tdi_id_t,
                              std::unique_ptr<tdi_info_action_info_t>>
                  &action_map_pair) {
        return action_map_pair.second->name == name;
      });
  if (action_found != action_info_list.end()) {
    *action_id = (*action_found).second->action_id;
    return TDI_SUCCESS;
  }
  LOG_TRACE("%s:%d Action_ID for action %s not found",
            __func__,
            __LINE__,
            name.c_str());
  return TDI_OBJECT_NOT_FOUND;
}

bool TdiTableObj::actionIdApplicable() const { return false; }

tdi_status_t TdiTableObj::actionNameGet(const tdi_id_t &action_id,
                                        std::string *name) const {
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action Id %d Not Found", __func__, __LINE__, action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *name = action_info_list.at(action_id)->name;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::actionIdListGet(std::vector<tdi_id_t> *id_vec) const {
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }
  if (id_vec == nullptr) {
    LOG_TRACE("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  if (action_info_list.empty()) {
    LOG_TRACE("%s:%d Table has no action IDs", __func__, __LINE__);
    return TDI_OBJECT_NOT_FOUND;
  }
  for (const auto &kv : action_info_list) {
    id_vec->push_back(kv.first);
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::actionAnnotationsGet(
    const tdi_id_t &action_id, AnnotationSet *annotations) const {
  // For the table to support actions, it needs to only override
  // actionIdApplicable() and all these action functions will remain
  // common
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action Id %d Not Found", __func__, __LINE__, action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  for (const auto &annotation :
       this->action_info_list.at(action_id)->annotations) {
    (*annotations).insert(std::cref(annotation));
  }
  return TDI_SUCCESS;
}

// Beginning of old flags wrapper section.
tdi_status_t TdiTableObj::tableAttributesSet(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    const TdiTableAttributes &tableAttributes) const {
  return this->tableAttributesSet(session, dev_tgt, 0, tableAttributes);
}

tdi_status_t TdiTableObj::tableAttributesGet(
    const tdi::Session &session,
    const tdi_target_t &dev_tgt,
    TdiTableAttributes *tableAttributes) const {
  return this->tableAttributesGet(session, dev_tgt, 0, tableAttributes);
}
// End of old flags wrapper section.

tdi_status_t TdiTableObj::tableAttributesSet(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const TdiTableAttributes & /*tableAttributes*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::tableAttributesGet(
    const tdi::Session & /*session*/,
    const tdi_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    TdiTableAttributes * /*tableAttributes*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return TDI_NOT_SUPPORTED;
}
tdi_status_t TdiTableObj::tableAttributesSupported(
    std::set<TableAttributesType> *type_set) const {
  *type_set = attributes_type_set;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableOperationsSupported(
    std::set<TableOperationsType> *op_type_set) const {
  *op_type_set = operations_type_set;
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::tableOperationsExecute(
    const TdiTableOperations &table_ops) const {
  auto table_ops_impl = static_cast<const TdiTableOperationsImpl *>(&table_ops);
  if (table_ops_impl->tableGet()->table_id_get() != table_id_get()) {
    LOG_ERROR("%s:%d %s Table mismatch. Sent in %s expected %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              table_ops_impl->tableGet()->table_name_get().c_str(),
              table_name_get().c_str());
  }
  switch (table_ops_impl->getAllowedOp()) {
    case TableOperationsType::REGISTER_SYNC:
      return table_ops_impl->registerSyncExecute();
    case TableOperationsType::COUNTER_SYNC:
      return table_ops_impl->counterSyncExecute();
    case TableOperationsType::HIT_STATUS_UPDATE:
      return table_ops_impl->hitStateUpdateExecute();
    default:
      LOG_ERROR("%s:%d %s Table operation is not allowed",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return TDI_INVALID_ARG;
  }
  return TDI_NOT_SUPPORTED;
}

// Table virt dev APIs
// Beginning of old flags wrapper section.
tdi_status_t TdiTableObj::registerMatUpdateCb(const tdi::Session &session,
                                              const tdi_target_t &dev_tgt,
                                              const pipe_mat_update_cb &cb,
                                              const void *cookie) const {
  return this->registerMatUpdateCb(session, dev_tgt, 0, cb, cookie);
}

tdi_status_t TdiTableObj::registerAdtUpdateCb(const tdi::Session &session,
                                              const tdi_target_t &dev_tgt,
                                              const pipe_adt_update_cb &cb,
                                              const void *cookie) const {
  return this->registerAdtUpdateCb(session, dev_tgt, 0, cb, cookie);
}
tdi_status_t TdiTableObj::registerSelUpdateCb(const tdi::Session &session,
                                              const tdi_target_t &dev_tgt,
                                              const pipe_sel_update_cb &cb,
                                              const void *cookie) const {
  return this->registerSelUpdateCb(session, dev_tgt, 0, cb, cookie);
}
// End of old flags wrapper section.

tdi_status_t TdiTableObj::registerMatUpdateCb(const tdi::Session & /*session*/,
                                              const tdi_target_t & /*dev_tgt*/,
                                              const uint64_t & /*flags*/,
                                              const pipe_mat_update_cb & /*cb*/,
                                              const void * /*cookie*/) const {
  LOG_ERROR(
      "%s:%d %s Not Supported", __func__, __LINE__, table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::registerAdtUpdateCb(const tdi::Session & /*session*/,
                                              const tdi_target_t & /*dev_tgt*/,
                                              const uint64_t & /*flags*/,
                                              const pipe_adt_update_cb & /*cb*/,
                                              const void * /*cookie*/) const {
  LOG_ERROR(
      "%s:%d %s Not Supported", __func__, __LINE__, table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}
tdi_status_t TdiTableObj::registerSelUpdateCb(const tdi::Session & /*session*/,
                                              const tdi_target_t & /*dev_tgt*/,
                                              const uint64_t & /*flags*/,
                                              const pipe_sel_update_cb & /*cb*/,
                                              const void * /*cookie*/) const {
  LOG_ERROR(
      "%s:%d %s Not Supported", __func__, __LINE__, table_name_get().c_str());
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::ghostTableHandleSet(const pipe_tbl_hdl_t & /*hdl*/) {
  LOG_ERROR("%s:%d API Not supported", __func__, __LINE__);
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiTableObj::dataFieldTypeGet(
    const tdi_id_t &field_id, std::set<DataFieldType> *ret_type) const {
  return this->dataFieldTypeGet(field_id, 0, ret_type);
}

tdi_status_t TdiTableObj::dataFieldTypeGet(
    const tdi_id_t &field_id,
    const tdi_id_t &action_id,
    std::set<DataFieldType> *ret_type) const {
  if (ret_type == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *ret_type = field->getTypes();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::getResourceInternal(
    const DataFieldType &field_type, tdi_table_ref_info_t *tbl_ref) const {
  // right now we return the first ref info back only since
  // we do not have provision for multiple refs
  switch (field_type) {
    case (DataFieldType::COUNTER_INDEX):
    case (DataFieldType::COUNTER_SPEC_BYTES):
    case (DataFieldType::COUNTER_SPEC_PACKETS): {
      auto vec = tableGetRefNameVec("statistics_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::REGISTER_SPEC):
    case (DataFieldType::REGISTER_SPEC_HI):
    case (DataFieldType::REGISTER_SPEC_LO):
    case (DataFieldType::REGISTER_INDEX): {
      auto vec = tableGetRefNameVec("stateful_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::LPF_INDEX):
    case (DataFieldType::LPF_SPEC_TYPE):
    case (DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR):
    case (DataFieldType::WRED_INDEX):
    case (DataFieldType::WRED_SPEC_TIME_CONSTANT):
    case (DataFieldType::WRED_SPEC_MIN_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_PROBABILITY):
    case (DataFieldType::METER_INDEX):
    case (DataFieldType::METER_SPEC_CIR_PPS):
    case (DataFieldType::METER_SPEC_PIR_PPS):
    case (DataFieldType::METER_SPEC_CBS_PKTS):
    case (DataFieldType::METER_SPEC_PBS_PKTS):
    case (DataFieldType::METER_SPEC_CIR_KBPS):
    case (DataFieldType::METER_SPEC_PIR_KBPS):
    case (DataFieldType::METER_SPEC_CBS_KBITS):
    case (DataFieldType::METER_SPEC_PBS_KBITS): {
      auto vec = tableGetRefNameVec("meter_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }
    default:
      LOG_ERROR("%s:%d %s ERROR: Wrong dataFieldType provided",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return TDI_OBJECT_NOT_FOUND;
  }
  return TDI_OBJECT_NOT_FOUND;
}

pipe_tbl_hdl_t TdiTableObj::getResourceHdl(
    const DataFieldType &field_type) const {
  tdi_table_ref_info_t tbl_ref;
  tdi_status_t status = getResourceInternal(field_type, &tbl_ref);
  if (status != TDI_SUCCESS) {
    return 0;
  }
  return tbl_ref.tbl_hdl;
}

pipe_tbl_hdl_t TdiTableObj::getIndirectResourceHdl(
    const DataFieldType &field_type) const {
  tdi_table_ref_info_t tbl_ref;
  tdi_status_t status = getResourceInternal(field_type, &tbl_ref);
  if (status != TDI_SUCCESS) {
    return 0;
  }
  if (tbl_ref.indirect_ref) {
    return tbl_ref.tbl_hdl;
  }
  return 0;
}

tdi_status_t TdiTableObj::getKeyField(const tdi_id_t &field_id,
                                      const TdiTableKeyField **field) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d Field ID %d not found", __func__, __LINE__, field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field = key_fields.at(field_id).get();
  return TDI_SUCCESS;
}

tdi_status_t TdiTableObj::getDataField(const tdi_id_t &field_id,
                                       const TdiTableDataField **field) const {
  return this->getDataField(field_id, 0, field);
}

tdi_status_t TdiTableObj::getDataField(const tdi_id_t &field_id,
                                       const tdi_id_t &action_id,
                                       const TdiTableDataField **field) const {
  std::vector<tdi_id_t> empty;
  return this->getDataField(field_id, action_id, empty, field);
}

tdi_status_t TdiTableObj::getDataField(
    const tdi_id_t &field_id,
    const tdi_id_t &action_id,
    const std::vector<tdi_id_t> &container_id_vec,
    const TdiTableDataField **field) const {
  *field = nullptr;
  if (action_info_list.find(action_id) != action_info_list.end()) {
    *field =
        this->getDataFieldHelper(field_id,
                                 container_id_vec,
                                 0,
                                 action_info_list.at(action_id)->data_fields);
    if (*field) return TDI_SUCCESS;
  }
  // We need to search the common data too
  *field = this->getDataFieldHelper(
      field_id, container_id_vec, 0, common_data_fields);
  if (*field) return TDI_SUCCESS;
  // Logging only warning so as to not overwhelm logs. Users supposed to check
  // error code if something wrong
  LOG_WARN("%s:%d Data field ID %d actionID %d not found",
           __func__,
           __LINE__,
           field_id,
           action_id);
  return TDI_OBJECT_NOT_FOUND;
}

const TdiTableDataField *TdiTableObj::getDataFieldHelper(
    const tdi_id_t &field_id,
    const std::vector<tdi_id_t> &container_id_vec,
    const uint32_t depth,
    const std::map<tdi_id_t, std::unique_ptr<TdiTableDataField>> &field_map)
    const {
  if (field_map.find(field_id) != field_map.end()) {
    return field_map.at(field_id).get();
  }
  // iterate all fields and recursively search for the data field
  // if this field is a container field and it exists in the contaienr
  // set provided by caller.
  // If container ID vector is empty, then just try and find first
  // instance
  for (const auto &p : field_map) {
    if (((container_id_vec.size() >= depth + 1 &&
          container_id_vec[depth] == p.first) ||
         container_id_vec.empty()) &&
        p.second.get()->isContainerValid()) {
      auto field = this->getDataFieldHelper(field_id,
                                            container_id_vec,
                                            depth + 1,
                                            p.second.get()->containerMapGet());
      if (field) return field;
    }
  }
  return nullptr;
}

tdi_status_t TdiTableObj::getDataField(
    const std::string &field_name,
    const tdi_id_t &action_id,
    const std::vector<tdi_id_t> &container_id_vec,
    const TdiTableDataField **field) const {
  *field = nullptr;
  if (action_info_list.find(action_id) != action_info_list.end()) {
    *field = this->getDataFieldHelper(
        field_name,
        container_id_vec,
        0,
        action_info_list.at(action_id)->data_fields_names);
    if (*field) return TDI_SUCCESS;
  }
  // We need to search the common data too
  *field = this->getDataFieldHelper(
      field_name, container_id_vec, 0, common_data_fields_names);
  if (*field) return TDI_SUCCESS;
  // Logging only warning so as to not overwhelm logs. Users supposed to check
  // error code if something wrong
  // TODO change this to WARN
  LOG_TRACE("%s:%d Data field name %s actionID %d not found",
            __func__,
            __LINE__,
            field_name.c_str(),
            action_id);
  return TDI_OBJECT_NOT_FOUND;
}

const TdiTableDataField *TdiTableObj::getDataFieldHelper(
    const std::string &field_name,
    const std::vector<tdi_id_t> &container_id_vec,
    const uint32_t depth,
    const std::map<std::string, TdiTableDataField *> &field_map) const {
  if (field_map.find(field_name) != field_map.end()) {
    return field_map.at(field_name);
  }
  // iterate all fields and recursively search for the data field
  // if this field is a container field and it exists in the contaienr
  // vector provided by caller.
  // If container ID vector is empty, then just try and find first
  // instance
  for (const auto &p : field_map) {
    if (((container_id_vec.size() >= depth + 1 &&
          container_id_vec[depth] == p.second->getId()) ||
         container_id_vec.empty()) &&
        p.second->isContainerValid()) {
      auto field = this->getDataFieldHelper(field_name,
                                            container_id_vec,
                                            depth + 1,
                                            p.second->containerNameMapGet());
      if (field) return field;
    }
  }
  return nullptr;
}

pipe_act_fn_hdl_t TdiTableObj::getActFnHdl(const tdi_id_t &action_id) const {
  auto elem = action_info_list.find(action_id);
  if (elem == action_info_list.end()) {
    return 0;
  }
  return elem->second->act_fn_hdl;
}

tdi_id_t TdiTableObj::getActIdFromActFnHdl(
    const pipe_act_fn_hdl_t &act_fn_hdl) const {
  auto elem = act_fn_hdl_to_id.find(act_fn_hdl);
  if (elem == act_fn_hdl_to_id.end()) {
    LOG_ERROR("%s:%d %s Action hdl %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              act_fn_hdl);
    TDI_ASSERT(0);
  }
  return elem->second;
}

void TdiTableObj::addDataFieldType(const tdi_id_t &field_id,
                                   const DataFieldType &type) {
  // This method adds a dataField Type to the given field id
  auto dataField = common_data_fields.find(field_id);
  if (dataField == common_data_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    TDI_ASSERT(0);
  }
  TdiTableDataField *field = dataField->second.get();
  field->addDataFieldType(type);
  return;
}

const std::string &TdiTableObj::table_name_get() const { return object_name; }

tdi_id_t TdiTableObj::table_id_get() const {
  tdi_id_t id;
  auto bf_status = tableIdGet(&id);
  if (bf_status == TDI_SUCCESS) {
    return id;
  }
  return 0;
}
const std::string &TdiTableObj::key_field_name_get(
    const tdi_id_t &field_id) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return tdiNullStr;
  }
  return key_fields.at(field_id)->getName();
}

const std::string &TdiTableObj::data_field_name_get(
    const tdi_id_t &field_id) const {
  return this->data_field_name_get(field_id, 0);
}

const std::string &TdiTableObj::data_field_name_get(
    const tdi_id_t &field_id, const tdi_id_t &action_id) const {
  const TdiTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return tdiNullStr;
  }
  return field->getName();
}

const std::string &TdiTableObj::action_name_get(
    const tdi_id_t &action_id) const {
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_ERROR("%s:%d %s Action Id %d Not Found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              action_id);
    return tdiNullStr;
  }
  return action_info_list.at(action_id)->name;
}

size_t TdiTableObj::getMaxdataSz() const {
  size_t max_sz = 0;
  for (const auto &each_action : action_info_list) {
    size_t this_sz = each_action.second->dataSz;
    if (this_sz > max_sz) {
      max_sz = this_sz;
    }
  }
  return max_sz;
}

size_t TdiTableObj::getMaxdataSzbits() const {
  size_t max_sz = 0;
  for (const auto &each_action : action_info_list) {
    size_t this_sz = each_action.second->dataSzbits;
    if (this_sz > max_sz) {
      max_sz = this_sz;
    }
  }
  return max_sz;
}

size_t TdiTableObj::getdataSz(tdi_id_t act_id) const {
  auto elem = action_info_list.find(act_id);
  if (elem == action_info_list.end()) {
    TDI_ASSERT(0);
    return 0;
  }
  return elem->second->dataSz;
}

size_t TdiTableObj::getdataSzbits(tdi_id_t act_id) const {
  auto elem = action_info_list.find(act_id);
  if (elem == action_info_list.end()) {
    TDI_ASSERT(0);
    return 0;
  }
  return elem->second->dataSzbits;
}

bool TdiTableObj::validateTable_from_keyObj(const TdiTableKeyObj &key) const {
  tdi_id_t table_id;
  tdi_id_t table_id_of_key_obj;

  const TdiTable *table_from_key = nullptr;
  tdi_status_t bf_status = this->tableIdGet(&table_id);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  bf_status = key.tableGet(&table_from_key);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table object from key object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  bf_status = table_from_key->tableIdGet(&table_id_of_key_obj);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id from key object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  if (table_id == table_id_of_key_obj) {
    return true;
  }
  return false;
}

bool TdiTableObj::validateTable_from_dataObj(
    const TdiTableDataObj &data) const {
  tdi_id_t table_id;
  tdi_id_t table_id_of_data_obj;

  tdi_status_t bf_status = this->tableIdGet(&table_id);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  const TdiTable *table_from_data = nullptr;
  bf_status = data.getParent(&table_from_data);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table object from data object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  bf_status = table_from_data->tableIdGet(&table_id_of_data_obj);
  if (bf_status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id from data object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    TDI_DBGCHK(0);
    return false;
  }

  if (table_id == table_id_of_data_obj) {
    return true;
  }
  return false;
}

void TdiTableObj::getActionResources(const tdi_id_t action_id,
                                     bool *meter,
                                     bool *reg,
                                     bool *cntr) const {
  *cntr = *meter = *reg = false;
  // Will default to false if action_id does not exist.
  if (this->act_uses_dir_cntr.find(action_id) !=
      this->act_uses_dir_cntr.end()) {
    *cntr = this->act_uses_dir_cntr.at(action_id);
  }
  if (this->act_uses_dir_reg.find(action_id) != this->act_uses_dir_reg.end()) {
    *reg = this->act_uses_dir_reg.at(action_id);
  }
  if (this->act_uses_dir_meter.find(action_id) !=
      this->act_uses_dir_meter.end()) {
    *meter = this->act_uses_dir_meter.at(action_id);
  }
}

tdi_status_t TdiTableObj::getKeyStringChoices(
    const std::string &key_str, std::vector<std::string> &choices) const {
  tdi_id_t key_id;
  auto status = this->keyFieldIdGet(key_str, &key_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_str.c_str());
    return status;
  }

  std::vector<std::reference_wrapper<const std::string>> key_str_choices;
  status = this->keyFieldAllowedChoicesGet(key_id, &key_str_choices);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get string choices for key field Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  for (auto key_str_choice : key_str_choices) {
    choices.push_back(key_str_choice.get());
  }

  return status;
}

tdi_status_t TdiTableObj::getDataStringChoices(
    const std::string &data_str, std::vector<std::string> &choices) const {
  tdi_id_t data_id;
  auto status = this->dataFieldIdGet(data_str, &data_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_str.c_str());
    return status;
  }

  std::vector<std::reference_wrapper<const std::string>> data_str_choices;
  status = this->dataFieldAllowedChoicesGet(data_id, &data_str_choices);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get string choices for data field Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }

  for (auto data_str_choice : data_str_choices) {
    choices.push_back(data_str_choice.get());
  }

  return status;
}
}  // namespace tdi
