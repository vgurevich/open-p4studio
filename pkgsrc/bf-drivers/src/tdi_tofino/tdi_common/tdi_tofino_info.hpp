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


/** @file tdi_tofino_info.hpp
 *
 *  @brief Contains TDI Table enums for tofino and mappers
 *
 */
#ifndef _TDI_TOFINO_INFO_HPP_
#define _TDI_TOFINO_INFO_HPP_

#include "../tdi_p4/tdi_p4_table_impl.hpp"
#include "../tdi_port/tdi_port_table_impl.hpp"
#include "../tdi_mirror/tdi_mirror_table_impl.hpp"

#include <tdi/arch/tna/tna_info.hpp>

#include <tdi_tofino/tdi_tofino_defs.h>

#include "tdi_tofino_table.hpp"
#include "tdi_state.hpp"

/**
 * @brief Namespace for TDI
 */
namespace tdi {

namespace tdi_json {
namespace values {
namespace tofino {}  // namespace tofino
}  // namespace values
}  // namespace tdi_json

namespace {
const std::map<std::string, tdi_tofino_attributes_type_e>
    tofino_attributes_type_map = {
        {"EntryScope", TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE},
        {"DynamicKeyMask", TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK},
        {"IdleTimeout", TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME},
        {"MeterByteCountAdjust",
         TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ},
        {"port_status_notif_cb", TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF},
        {"poll_intvl_ms", TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS},
        {"pre_device_config", TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG},
        {"SelectorUpdateCb",
         TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK},
};

const std::map<std::string, tdi_tofino_operations_type_e>
    tofino_operations_type_map = {
        {"SyncCounters", TDI_TOFINO_OPERATIONS_TYPE_COUNTER_SYNC},
        {"SyncRegisters", TDI_TOFINO_OPERATIONS_TYPE_REGISTER_SYNC},
        {"UpdateHitState", TDI_TOFINO_OPERATIONS_TYPE_HIT_STATUS_UPDATE},
        {"Sync", TDI_TOFINO_OPERATIONS_TYPE_SYNC},
};

const std::map<std::string, tdi_tofino_table_type_e> tofino_table_type_map = {
    {"MatchAction_Direct", TDI_TOFINO_TABLE_TYPE_MATCH_DIRECT},
    {"MatchAction_Indirect", TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT},
    {"MatchAction_Indirect_Selector",
     TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT_SELECTOR},
    {"Action", TDI_TOFINO_TABLE_TYPE_ACTION_PROFILE},
    {"Selector", TDI_TOFINO_TABLE_TYPE_SELECTOR},
    {"SelectorGetMember", TDI_TOFINO_TABLE_TYPE_SELECTOR_GET_MEMBER},
    {"Meter", TDI_TOFINO_TABLE_TYPE_METER},
    {"Counter", TDI_TOFINO_TABLE_TYPE_COUNTER},
    {"Register", TDI_TOFINO_TABLE_TYPE_REGISTER},
    {"Lpf", TDI_TOFINO_TABLE_TYPE_LPF},
    {"Wred", TDI_TOFINO_TABLE_TYPE_WRED},
    {"ParserValueSet", TDI_TOFINO_TABLE_TYPE_PVS},
    {"DynHashConfigure", TDI_TOFINO_TABLE_TYPE_DYN_HASH_CFG},
    {"DynHashAlgorithm", TDI_TOFINO_TABLE_TYPE_DYN_HASH_ALGO},
    {"DynHashCompute", TDI_TOFINO_TABLE_TYPE_DYN_HASH_COMPUTE},
    {"PortMetadata", TDI_TOFINO_TABLE_TYPE_PORT_METADATA},
    {"SnapshotCfg", TDI_TOFINO_TABLE_TYPE_SNAPSHOT_CFG},
    {"SnapshotData", TDI_TOFINO_TABLE_TYPE_SNAPSHOT_DATA},
    {"SnapshotTrigger", TDI_TOFINO_TABLE_TYPE_SNAPSHOT_TRIG},
    {"SnapshotLiveness", TDI_TOFINO_TABLE_TYPE_SNAPSHOT_LIVENESS},
    {"SnapshotPhv", TDI_TOFINO_TABLE_TYPE_SNAPSHOT_PHV},
    {"PortCfg", TDI_TOFINO_TABLE_TYPE_PORT_CFG},
    {"RecircPortCfg", TDI_TOFINO_TABLE_TYPE_RECIRC_PORT_CFG},
    {"PortStat", TDI_TOFINO_TABLE_TYPE_PORT_STAT},
    {"PortHdlInfo", TDI_TOFINO_TABLE_TYPE_PORT_HDL_INFO},
    {"PortFpIdxInfo", TDI_TOFINO_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO},
    {"PortStrInfo", TDI_TOFINO_TABLE_TYPE_PORT_STR_INFO},
    {"PktgenPortCfg", TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_CFG},
    {"PktgenAppCfg", TDI_TOFINO_TABLE_TYPE_PKTGEN_APP_CFG},
    {"PktgenPktBufferCfg", TDI_TOFINO_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG},
    {"PktgenPortMaskCfg", TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_MASK_CFG},
    {"PktgenPortDownReplyCfg",
     TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG},
    {"PreMgid", TDI_TOFINO_TABLE_TYPE_PRE_MGID},
    {"PreNode", TDI_TOFINO_TABLE_TYPE_PRE_NODE},
    {"PreEcmp", TDI_TOFINO_TABLE_TYPE_PRE_ECMP},
    {"PreLag", TDI_TOFINO_TABLE_TYPE_PRE_LAG},
    {"PrePrune", TDI_TOFINO_TABLE_TYPE_PRE_PRUNE},
    {"MirrorCfg", TDI_TOFINO_TABLE_TYPE_MIRROR_CFG},
    {"TmPpg", TDI_TOFINO_TABLE_TYPE_TM_PPG_OBSOLETE},
    {"PrePort", TDI_TOFINO_TABLE_TYPE_PRE_PORT},
    {"DevConfigure", TDI_TOFINO_TABLE_TYPE_DEV_CFG},
    {"TmPoolCfg", TDI_TOFINO_TABLE_TYPE_TM_POOL_CFG},
    {"TmPoolSkid", TDI_TOFINO_TABLE_TYPE_TM_POOL_SKID},
    {"TmPoolApp", TDI_TOFINO_TABLE_TYPE_TM_POOL_APP},
    {"TmPoolColor", TDI_TOFINO_TABLE_TYPE_TM_POOL_COLOR},
    {"TmPoolAppPfc", TDI_TOFINO_TABLE_TYPE_TM_POOL_APP_PFC},
    {"TmQueueCfg", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_CFG},
    {"TmQueueMap", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_MAP},
    {"TmQueueColor", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_COLOR},
    {"TmQueueBuffer", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_BUFFER},
    {"TmQueueSchedCfg", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_CFG},
    {"TmQueueSchedShaping", TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING},
    {"TmL1NodeSchedCfg", TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_CFG},
    {"TmL1NodeSchedShaping", TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_SHAPING},
    {"TmPipeCfg", TDI_TOFINO_TABLE_TYPE_TM_PIPE_CFG},
    {"TmPipeSchedCfg", TDI_TOFINO_TABLE_TYPE_TM_PIPE_SCHED_CFG},
    {"TmPortSchedCfg", TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_CFG},
    {"TmPortSchedShaping", TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_SHAPING},
    {"TmMirrorDpg", TDI_TOFINO_TABLE_TYPE_TM_MIRROR_DPG},
    {"TmPortDpg", TDI_TOFINO_TABLE_TYPE_TM_PORT_DPG},
    {"TmPpgCfg", TDI_TOFINO_TABLE_TYPE_TM_PPG_CFG},
    {"TmPortCfg", TDI_TOFINO_TABLE_TYPE_TM_PORT_CFG},
    {"TmPortBuffer", TDI_TOFINO_TABLE_TYPE_TM_PORT_BUFFER},
    {"TmPortFlowcontrol", TDI_TOFINO_TABLE_TYPE_TM_PORT_FLOWCONTROL},
    {"TmPortGroupCfg", TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP_CFG},
    {"TmPortGroup", TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP},
    {"TmCounterIgPort", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_IG_PORT},
    {"TmCounterEgPort", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_EG_PORT},
    {"TmCounterQueue", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_QUEUE},
    {"TmCounterPool", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_POOL},
    {"TmCounterPipe", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PIPE},
    {"RegisterParam", TDI_TOFINO_TABLE_TYPE_REG_PARAM},
    {"TmCounterPortDpg", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PORT_DPG},
    {"TmCounterMirrorPortDpg",
     TDI_TOFINO_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG},
    {"TmCounterPpg", TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PPG},
    {"TblDbgCnt", TDI_TOFINO_TABLE_TYPE_DBG_CNT},
    {"LogDbgCnt", TDI_TOFINO_TABLE_TYPE_LOG_DBG_CNT},
    {"TmCfg", TDI_TOFINO_TABLE_TYPE_TM_CFG},
    {"TmPipeMulticastFifo", TDI_TOFINO_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO}};
}  // namespace

namespace tna {
namespace tofino {

class TdiInfoMapper : public tdi::tna::TdiInfoMapper {
 public:
  TdiInfoMapper() {
    // table types
    for (const auto &kv : tofino_table_type_map) {
      tableEnumMapAdd(kv.first, static_cast<tdi_table_type_e>(kv.second));
    }
    for (const auto &kv : tofino_attributes_type_map) {
      attributesEnumMapAdd(kv.first,
                           static_cast<tdi_attributes_type_e>(kv.second));
    }
    for (const auto &kv : tofino_operations_type_map) {
      operationsEnumMapAdd(kv.first,
                           static_cast<tdi_operations_type_e>(kv.second));
    }
  }
};

/**
 * @brief Class to help create the correct Table object with the
 * help of a map. Targets should override
 */
class TableFactory : public tdi::tna::TableFactory {
 public:
  virtual std::unique_ptr<tdi::Table> makeTable(
      const TdiInfo *tdi_info,
      const tdi::TableInfo *table_info) const override {
    if (!table_info) {
      LOG_ERROR("%s:%d No table info received", __func__, __LINE__);
      return nullptr;
    }
    auto table_type =
        static_cast<tdi_tofino_table_type_e>(table_info->tableTypeGet());
    switch (table_type) {
      case TDI_TOFINO_TABLE_TYPE_MATCH_DIRECT:
        return std::unique_ptr<tdi::Table>(
            new tdi::tna::tofino::MatchActionDirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT:
      case TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT_SELECTOR:
        return std::unique_ptr<tdi::Table>(
            new MatchActionIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_ACTION_PROFILE:
        return std::unique_ptr<tdi::Table>(
            new ActionProfile(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_SELECTOR:
        return std::unique_ptr<tdi::Table>(new Selector(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_COUNTER:
        return std::unique_ptr<tdi::Table>(
            new CounterIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_REGISTER:
        return std::unique_ptr<tdi::Table>(
            new RegisterIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_METER:
        return std::unique_ptr<tdi::Table>(
            new MeterIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_LPF:
        return std::unique_ptr<tdi::Table>(
            new LpfIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_WRED:
        return std::unique_ptr<tdi::Table>(
            new WredIndirect(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PORT_CFG:
        return std::unique_ptr<tdi::Table>(
            new PortCfgTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_RECIRC_PORT_CFG:
        return std::unique_ptr<tdi::Table>(
            new RecircPortCfgTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PORT_STAT:
        return std::unique_ptr<tdi::Table>(
            new PortStatTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PORT_STR_INFO:
        return std::unique_ptr<tdi::Table>(
            new PortStrInfoTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PORT_HDL_INFO:
        return std::unique_ptr<tdi::Table>(
            new PortHdlInfoTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO:
        return std::unique_ptr<tdi::Table>(
            new PortFpIdxInfoTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_MIRROR_CFG:
        return std::unique_ptr<tdi::Table>(
            new MirrorCfgTable(tdi_info, table_info));
      case TDI_TOFINO_TABLE_TYPE_PVS:
        return std::unique_ptr<tdi::Table>(new PVS(tdi_info, table_info));
      default:
        return nullptr;
    }
    return nullptr;
  };
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif
