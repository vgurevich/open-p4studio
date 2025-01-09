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


#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_init.hpp>
#include <tdi/common/tdi_attributes.hpp>
#include <tdi/common/tdi_table.hpp>

#include <tdi_common/tdi_tofino_target.hpp>
#include <tdi_common/tdi_table_field_utils.hpp>
#include <tdi_common/tdi_table_attributes_impl.hpp>
#include <tdi_tofino/tdi_tofino_init.hpp>
#include <tdi_tofino/tdi_tofino_attributes.hpp>

#include "tdi_port_mgr_intf.hpp"
#include "tdi_port_table_impl.hpp"
#include "tdi_port_table_data_impl.hpp"
#include "tdi_port_table_key_impl.hpp"
#include "tdi_port_table_attributes_state.hpp"

namespace tdi {

// Port Configuration Table
enum class PortCfgDataFieldId {
  SPEED_ID = 1,
  FEC_ID = 2,
  LANES_ID = 3,
  PORT_EN_ID = 4,
  AN_ID = 5,
  LPBK_MODE_ID = 6,
  TX_MTU_ID = 7,
  RX_MTU_ID = 8,
  TX_PFC_EN_MAP_ID = 9,
  RX_PFC_EN_MAP_ID = 10,
  TX_PAUSE_EN_ID = 11,
  RX_PAUSE_EN_ID = 12,
  CT_EN_ID = 13,
  PORT_DIR_ID = 14,
  MEDIA_TP_ID = 15,
  SDS_TX_ATTN_ID = 16,
  SDS_TX_PRE_ID = 17,
  SDS_TX_PRE2_ID = 18,
  SDS_TX_POST_ID = 19,
  SDS_TX_POST2_ID = 20,
  PORT_VALID_ID = 21,
  PORT_INTERNAL_ID = 22,
  CONN_ID = 23,
  CHNL_ID = 24,
  PORT_UP_ID = 25,
  PORT_NAME_ID = 26,
  RX_PRSR_PRI_THRESH = 27,
  TIMESTAMP_1588_DELTA_TX = 28,
  TIMESTAMP_1588_DELTA_RX = 29,
  TIMESTAMP_1588_VALID = 30,
  TIMESTAMP_1588_VALUE = 31,
  TIMESTAMP_1588_ID = 32
};

enum class PktRateField {
  TX_PPS = 92,
  RX_PPS = 93,
  TX_RATE = 94,
  RX_RATE = 95
};

void PortCfgTable::mapInit() {
  speedMap["SPEED_NONE"] = BF_SPEED_NONE;
  speedMap["SPEED_1G"] = BF_SPEED_1G;
  speedMap["SPEED_10G"] = BF_SPEED_10G;
  speedMap["SPEED_25G"] = BF_SPEED_25G;
  speedMap["SPEED_40G"] = BF_SPEED_40G;
  speedMap["SPEED_50G"] = BF_SPEED_50G;
  speedMap["SPEED_100G"] = BF_SPEED_100G;
  speedMap["SPEED_200G"] = BF_SPEED_200G;
  speedMap["SPEED_400G"] = BF_SPEED_400G;

  fecMap["FEC_TYP_NONE"] = BF_FEC_TYP_NONE;
  fecMap["FEC_TYP_FIRECODE"] = BF_FEC_TYP_FIRECODE;
  fecMap["FEC_TYP_REED_SOLOMON"] = BF_FEC_TYP_REED_SOLOMON;

  mediaTypeMap["MEDIA_TYPE_COPPER"] = BF_MEDIA_TYPE_COPPER;
  mediaTypeMap["MEDIA_TYPE_OPTICAL"] = BF_MEDIA_TYPE_OPTICAL;
  mediaTypeMap["MEDIA_TYPE_UNKNOWN"] = BF_MEDIA_TYPE_UNKNOWN;

  portDirMap["PORT_DIR_DEFAULT"] = PM_PORT_DIR_DEFAULT;
  portDirMap["PORT_DIR_TX_ONLY"] = PM_PORT_DIR_TX_ONLY;
  portDirMap["PORT_DIR_RX_ONLY"] = PM_PORT_DIR_RX_ONLY;
  portDirMap["PORT_DIR_DECOUPLED"] = PM_PORT_DIR_DECOUPLED;

  loopbackModeMap["LPBK_NONE"] = BF_LPBK_NONE;
  loopbackModeMap["LPBK_MAC_NEAR"] = BF_LPBK_MAC_NEAR;
  loopbackModeMap["LPBK_MAC_FAR"] = BF_LPBK_MAC_FAR;
  loopbackModeMap["LPBK_PCS_NEAR"] = BF_LPBK_PCS_NEAR;
  loopbackModeMap["LPBK_SERDES_NEAR"] = BF_LPBK_SERDES_NEAR;
  loopbackModeMap["LPBK_SERDES_FAR"] = BF_LPBK_SERDES_FAR;
  loopbackModeMap["LPBK_PIPE"] = BF_LPBK_PIPE;
  loopbackModeMap["LPBK_SERDES_NEAR_PARALLEL"] = BF_LPBK_SERDES_NEAR_PARALLEL;
  loopbackModeMap["LPBK_SERDES_FAR_PARALLEL"] = BF_LPBK_SERDES_FAR_PARALLEL;

  autonegoPolicyMap["AN_DEFAULT"] = PM_AN_DEFAULT;
  autonegoPolicyMap["AN_FORCE_ENABLE"] = PM_AN_FORCE_ENABLE;
  autonegoPolicyMap["AN_FORCE_DISABLE"] = PM_AN_FORCE_DISABLE;
}

void RecircPortCfgTable::mapInit() {
  speedMap["SPEED_25G"] = BF_SPEED_25G;
  speedMap["SPEED_50G"] = BF_SPEED_50G;
  speedMap["SPEED_100G"] = BF_SPEED_100G;
}

tdi_status_t PortCfgTable::entryAdd(const Session &session,
                                    const Target &dev_tgt,
                                    const Flags & /*flags*/,
                                    const TableKey &key,
                                    const TableData &data) const {
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  const uint32_t dev_port = port_key.getId();
  const PortCfgTableData &port_data =
      static_cast<const PortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  const std::unordered_map<tdi_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  const std::unordered_map<tdi_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  if ((strData.empty()) ||
      (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)) ==
       strData.end()) ||
      (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID)) ==
       strData.end())) {
    LOG_TRACE(
        "%s:%d %s ERROR : Port Cfg table entry add require both speed and fec",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // check and find right enum for speed and fec
  if ((speedMap.find(strData.at(static_cast<uint32_t>(
           PortCfgDataFieldId::SPEED_ID))) == speedMap.end()) ||
      (fecMap.find(strData.at(static_cast<uint32_t>(
           PortCfgDataFieldId::FEC_ID))) == fecMap.end())) {
    LOG_TRACE(
        "%s:%d %s ERROR : invalid speed %s or fec %s",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        strData.at(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)).c_str(),
        strData.at(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID)).c_str());
    return TDI_INVALID_ARG;
  }
  // step 1: add port first
  if (u32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID)) ==
      u32Data.end()) {
    status = portMgr->portMgrPortAdd(
        port_dev_tgt.device_id,
        dev_port,
        (speedMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)))),
        (fecMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID)))));
  } else {
    status = portMgr->portMgrPortAddWithLanes(
        port_dev_tgt.device_id,
        dev_port,
        (speedMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)))),
        u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID)),
        (fecMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID)))));
  }
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in adding an port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  return this->EntryMod_internal(session, dev_tgt, dev_port, data);
}

tdi_status_t PortCfgTable::entryMod(const Session &session,
                                    const Target &dev_tgt,
                                    const Flags & /*flags*/,
                                    const TableKey &key,
                                    const TableData &data) const {
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  const uint32_t dev_port = port_key.getId();
  const PortCfgTableData &port_data =
      static_cast<const PortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  const std::unordered_map<tdi_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();
  const std::unordered_map<tdi_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  /* Speed case is different, it needs a port flap.
   * Changing speed also changes AN settings to default, so save the old value
   * and then reapply it. */

  // speed
  auto strDataIt =
      strData.find(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID));

  if (strDataIt != strData.end()) {
    auto speedMapIt = speedMap.find(strDataIt->second);
    // Check speed is valid
    if (speedMapIt == speedMap.end()) {
      LOG_ERROR("%s:%d %s ERROR : invalid speed type %s",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                strDataIt->second.c_str());
      return TDI_INVALID_ARG;
    }

    // fec required for validation of n_lanes associated with speed.
    bf_fec_type_t fec_type;

    tdi_status_t sts =
        portMgr->portMgrPortFecGet(port_dev_tgt.device_id, dev_port, &fec_type);
    if (TDI_SUCCESS != sts) {
      LOG_ERROR("%s:%d %s: Error in getting FEC of dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      return sts;
    }

    // Check n_lanes are valid for given speed.
    uint32_t n_lanes = 0;
    bool st;
    if (u32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID)) ==
        u32Data.end()) {
      status = portMgr->portMgrPortGetDefaultLaneNum(
          port_dev_tgt.device_id, speedMapIt->second, &n_lanes);
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in getting Number of lanes",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
      st = portMgr->portMgrPortSpeedValidate(port_dev_tgt.device_id,
                                             dev_port,
                                             speedMapIt->second,
                                             n_lanes,
                                             fec_type);
      if (st != true) {
        LOG_ERROR("%s:%d %s: Invalid Channel",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
    } else {
      n_lanes = u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID));

      st = portMgr->portMgrPortSpeedValidate(port_dev_tgt.device_id,
                                             dev_port,
                                             speedMapIt->second,
                                             n_lanes,
                                             fec_type);
      if (st != true) {
        LOG_ERROR("%s:%d %s: Invalid Channel",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return TDI_INVALID_ARG;
      }
    }

    // Disable dev_port
    status =
        portMgr->portMgrPortEnable(port_dev_tgt.device_id, dev_port, false);
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                "disable");
      return status;
    }

    bf_pm_port_autoneg_policy_e an_policy;

    status =
        portMgr->portMgrPortAnGet(port_dev_tgt.device_id, dev_port, &an_policy);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s: Error in getting autoneg policy",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return status;
    }

    status = portMgr->portMgrPortDelWithLanes(
        port_dev_tgt.device_id, dev_port, speedMapIt->second, n_lanes);
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in deleting Port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      return status;
    }

    if (u32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID)) ==
        u32Data.end()) {
      status = portMgr->portMgrPortSpeedSet(
          port_dev_tgt.device_id, dev_port, speedMapIt->second);
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting speed type %s",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  strDataIt->second.c_str());
        return status;
      }
    } else {
      status = portMgr->portMgrPortSpeedWithLanesSet(
          port_dev_tgt.device_id,
          dev_port,
          speedMapIt->second,
          u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting speed type %s",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  strDataIt->second.c_str());
        return status;
      }
    }

    status = portMgr->portMgrPortAnPolicySet(
        port_dev_tgt.device_id, dev_port, an_policy);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s: Error in getting autoneg policy",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return status;
    }

    // Enable dev_port after setting the speed.
    status = portMgr->portMgrPortEnable(port_dev_tgt.device_id, dev_port, true);
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                "enable");
      return status;
    }
  }

  // fec
  if (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID)) !=
      strData.end()) {
    // check fec is valid
    if (fecMap.find(strData.at(static_cast<uint32_t>(
            PortCfgDataFieldId::FEC_ID))) == fecMap.end()) {
      LOG_TRACE("%s:%d %s ERROR : invalid fec %s",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                strData.at(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID))
                    .c_str());
      return TDI_INVALID_ARG;
    }
    status = portMgr->portMgrPortFECSet(
        port_dev_tgt.device_id,
        static_cast<bf_dev_port_t>(dev_port),
        fecMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID))));
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in setting FEC",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return status;
    }
  }
  return this->EntryMod_internal(session, dev_tgt, dev_port, data);
}

tdi_status_t PortCfgTable::entryDel(const tdi::Session & /*session*/,
                                    const tdi::Target &dev_tgt,
                                    const tdi::Flags & /*flags*/,
                                    const tdi::TableKey &key) const {
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  uint32_t dev_port = port_key.getId();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  return portMgr->portMgrPortDel(port_dev_tgt.device_id, dev_port);
}

tdi_status_t PortCfgTable::clear(const tdi::Session & /*session*/,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags & /*flags*/) const {
  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  return portMgr->portMgrPortDelAll(port_dev_tgt.device_id);
}

tdi_status_t PortCfgTable::EntryMod_internal(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint32_t &dev_port,
                                             const tdi::TableData &data) const {
  const PortCfgTableData port_data =
      static_cast<const PortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  const std::unordered_map<tdi_id_t, bool> &boolData =
      port_data.getBoolFieldDataMap();
  const std::unordered_map<tdi_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  const std::unordered_map<tdi_id_t, int32_t> &i32Data =
      port_data.getI32FieldDataMap();
  const std::unordered_map<tdi_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  // step 2: cfg ports
  // AN
  if (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID)) !=
      strData.end()) {
    if (autonegoPolicyMap.find(strData.at(static_cast<uint32_t>(
            PortCfgDataFieldId::AN_ID))) == autonegoPolicyMap.end()) {
      LOG_ERROR(
          "%s:%d %s ERROR : Invalid AN Policy %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID)).c_str());
      return TDI_INVALID_ARG;
    }
    status = portMgr->portMgrPortAnPolicySet(
        port_dev_tgt.device_id,
        dev_port,
        autonegoPolicyMap.at(
            strData.at(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID))));
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting AN Policy %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID)));
    }
  }
  // Loopback
  if (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::LPBK_MODE_ID)) !=
      strData.end()) {
    if (loopbackModeMap.find(strData.at(static_cast<uint32_t>(
            PortCfgDataFieldId::LPBK_MODE_ID))) == loopbackModeMap.end()) {
      LOG_TRACE(
          "%s:%d %s ERROR : invalid loopback mode %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::LPBK_MODE_ID))
              .c_str());
      return TDI_INVALID_ARG;
    }
    status = portMgr->portMgrPortSetLoopbackMode(
        port_dev_tgt.device_id,
        dev_port,
        loopbackModeMap.at(strData.at(
            static_cast<uint32_t>(PortCfgDataFieldId::LPBK_MODE_ID))));
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting Loopback Mode %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          (strData.at(static_cast<uint32_t>(PortCfgDataFieldId::LPBK_MODE_ID)))
              .c_str());
    }
  }
  // mtu
  if ((u32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::TX_MTU_ID)) !=
       u32Data.end()) &&
      (u32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::RX_MTU_ID)) !=
       u32Data.end())) {
    status = portMgr->portMgrPortMtuSet(
        port_dev_tgt.device_id,
        dev_port,
        u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::TX_MTU_ID)),
        u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::RX_MTU_ID)));
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting tx_mtu %d and rx_mtu %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::TX_MTU_ID)),
          u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::RX_MTU_ID)));
    }
  } else if (!((u32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::TX_MTU_ID)) == u32Data.end()) &&
               (u32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::RX_MTU_ID)) == u32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port mtu configuration require both tx_mtu and "
        "rx_mtu",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }
  // pfc
  if ((u32Data.find(static_cast<uint32_t>(
           PortCfgDataFieldId::TX_PFC_EN_MAP_ID)) != u32Data.end()) &&
      (u32Data.find(static_cast<uint32_t>(
           PortCfgDataFieldId::RX_PFC_EN_MAP_ID)) != u32Data.end())) {
    status = portMgr->portMgrPortFlowControlPfcSet(
        port_dev_tgt.device_id,
        dev_port,
        u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::TX_PFC_EN_MAP_ID)),
        u32Data.at(
            static_cast<uint32_t>(PortCfgDataFieldId::RX_PFC_EN_MAP_ID)));
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting tx_pfc_en_map %d and rx_pfc_en_map %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          u32Data.at(
              static_cast<uint32_t>(PortCfgDataFieldId::TX_PFC_EN_MAP_ID)),
          u32Data.at(
              static_cast<uint32_t>(PortCfgDataFieldId::RX_PFC_EN_MAP_ID)));
    }
  } else if (!((u32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::TX_PFC_EN_MAP_ID)) == u32Data.end()) &&
               (u32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::RX_PFC_EN_MAP_ID)) == u32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port pfc configuration require both tx_pfc_en_map "
        "and rx_pfc_en_map",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }
  // ingress parser priority threshold
  if (u32Data.find(static_cast<uint32_t>(
          PortCfgDataFieldId::RX_PRSR_PRI_THRESH)) != u32Data.end()) {
    auto *pipeMgr = tdi::tna::tofino::PipeMgrIntf::getInstance(session);
    status = pipeMgr->pipeMgrPortRXPrsrPriThreshSet(
        port_dev_tgt.device_id,
        dev_port,
        u32Data.at(
            static_cast<uint32_t>(PortCfgDataFieldId::RX_PRSR_PRI_THRESH)));
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting rx parser priorty threshold %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                u32Data.at(static_cast<uint32_t>(
                    PortCfgDataFieldId::RX_PRSR_PRI_THRESH)));
    }
  }
  // ingress PTP timestamp
  if (u32Data.find(static_cast<uint32_t>(
          PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX)) != u32Data.end()) {
    auto tableInfo = this->tableInfoGet();
    auto tableDataField = tableInfo->dataFieldGet(
        static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX));

    if (tableDataField) {
      status = portMgr->portMgrPtpRxDeltaSet(
          port_dev_tgt.device_id,
          dev_port,
          u32Data.at(static_cast<uint32_t>(
              PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting 1588 PTP rx delta value:  %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  u32Data.at(static_cast<uint32_t>(
                      PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX)));
      }
    } else {
      LOG_ERROR(
          "%s:%d %s: Error: "
          "static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX)  "
          "Not supported ",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
    }
  }

  // Egress PTP timestamp
  if (u32Data.find(static_cast<uint32_t>(
          PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX)) != u32Data.end()) {
    auto tableInfo = this->tableInfoGet();
    auto tableDataField = tableInfo->dataFieldGet(
        static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX));

    if (tableDataField) {
      status = portMgr->portMgrPtpTxDeltaSet(
          port_dev_tgt.device_id,
          dev_port,
          u32Data.at(static_cast<uint32_t>(
              PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting 1588 PTP tx delta value:  %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  u32Data.at(static_cast<uint32_t>(
                      PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX)));
      }
    } else {
      LOG_ERROR(
          "%s:%d %s: Error: "
          "static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX)  "
          "Not supported ",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
    }
  }

  // port dir
  if (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::PORT_DIR_ID)) !=
      strData.end()) {
    if (portDirMap.find(strData.at(static_cast<uint32_t>(
            PortCfgDataFieldId::PORT_DIR_ID))) == portDirMap.end()) {
      LOG_TRACE(
          "%s:%d %s ERROR : invalid port dir %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::PORT_DIR_ID))
              .c_str());
      return TDI_INVALID_ARG;
    }
    status = portMgr->portMgrPortDirSet(
        port_dev_tgt.device_id,
        static_cast<bf_dev_port_t>(dev_port),
        portDirMap.at(strData.at(
            static_cast<uint32_t>(PortCfgDataFieldId::PORT_DIR_ID))));
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting port direction %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::PORT_DIR_ID))
              .c_str());
    }
  }
  // media type
  if (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID)) !=
      strData.end()) {
    if (mediaTypeMap.find(strData.at(static_cast<uint32_t>(
            PortCfgDataFieldId::MEDIA_TP_ID))) == mediaTypeMap.end()) {
      LOG_TRACE(
          "%s:%d %s ERROR : invalid media type %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID))
              .c_str());
      return TDI_INVALID_ARG;
    }
    status = portMgr->portMgrPortMediaTypeSet(
        port_dev_tgt.device_id,
        static_cast<bf_dev_port_t>(dev_port),
        mediaTypeMap.at(strData.at(
            static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID))));
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting media type %s",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID))
              .c_str());
    }
  }
  // serdes params
  if ((u32Data.find(static_cast<uint32_t>(
           PortCfgDataFieldId::SDS_TX_ATTN_ID)) != u32Data.end()) &&
      (i32Data.find(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_PRE_ID)) !=
       i32Data.end()) &&
      (i32Data.find(static_cast<uint32_t>(
           PortCfgDataFieldId::SDS_TX_POST_ID)) != i32Data.end())) {
    bf_pal_serdes_params_t serdes_param;
    serdes_param.tx_attn =
        u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_ATTN_ID));
    serdes_param.tx_pre =
        i32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_PRE_ID));
    serdes_param.tx_post =
        i32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_POST_ID));
    status = portMgr->portMgrPortSerdesParamsSet(
        port_dev_tgt.device_id, dev_port, &serdes_param);
    if (TDI_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting serdes params tx_attn %d, tx_pre %d, "
          "tx_post %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          u32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_ATTN_ID)),
          i32Data.at(static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_PRE_ID)),
          i32Data.at(
              static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_POST_ID)));
    }
  } else if (!((u32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::SDS_TX_ATTN_ID)) == u32Data.end()) &&
               (i32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::SDS_TX_PRE_ID)) == i32Data.end()) &&
               (i32Data.find(static_cast<uint32_t>(
                    PortCfgDataFieldId::SDS_TX_POST_ID)) == i32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port serdes params configuration require "
        "sds_tx_attn, sds_tx_pre, and sds_tx_post",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }
  if (!boolData.empty()) {
    // pause frame
    if ((boolData.find(static_cast<uint32_t>(
             PortCfgDataFieldId::TX_PAUSE_EN_ID)) != boolData.end()) &&
        (boolData.find(static_cast<uint32_t>(
             PortCfgDataFieldId::RX_PAUSE_EN_ID)) != boolData.end())) {
      status = portMgr->portMgrPortFlowControlPauseFrameSet(
          port_dev_tgt.device_id,
          dev_port,
          boolData.at(
              static_cast<uint32_t>(PortCfgDataFieldId::TX_PAUSE_EN_ID)),
          boolData.at(
              static_cast<uint32_t>(PortCfgDataFieldId::RX_PAUSE_EN_ID)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s: Error in setting tx_pause_frame_en %s and "
            "rx_pause_frame_en %s",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            (boolData.at(
                 static_cast<uint32_t>(PortCfgDataFieldId::TX_PAUSE_EN_ID))
                 ? "enable"
                 : "disable"),
            (boolData.at(
                 static_cast<uint32_t>(PortCfgDataFieldId::RX_PAUSE_EN_ID))
                 ? "enable"
                 : "disable"));
      }
    } else if (!((boolData.find(static_cast<uint32_t>(
                      PortCfgDataFieldId::TX_PAUSE_EN_ID)) == boolData.end()) &&
                 (boolData.find(static_cast<uint32_t>(
                      PortCfgDataFieldId::RX_PAUSE_EN_ID)) ==
                  boolData.end()))) {
      LOG_ERROR(
          "%s:%d %s ERROR : port pause frame configuration require both "
          "tx_pause_frame_en and rx_pause_frame_en",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
    }
    // cut through en
    if (boolData.find(static_cast<uint32_t>(PortCfgDataFieldId::CT_EN_ID)) !=
        boolData.end()) {
      status = portMgr->portMgrPortCutThroughSet(
          port_dev_tgt.device_id,
          dev_port,
          boolData.at(static_cast<uint32_t>(PortCfgDataFieldId::CT_EN_ID)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s: Error in setting port cut through enable %s",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            (boolData.at(static_cast<uint32_t>(PortCfgDataFieldId::CT_EN_ID))
                 ? "enable"
                 : "disable"));
      }
    }
    // step 3: enable port (u32Data[4]) at last
    if (boolData.find(static_cast<uint32_t>(PortCfgDataFieldId::PORT_EN_ID)) !=
        boolData.end()) {
      status = portMgr->portMgrPortEnable(
          port_dev_tgt.device_id,
          dev_port,
          boolData.at(static_cast<uint32_t>(PortCfgDataFieldId::PORT_EN_ID)));
      if (TDI_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  ((boolData.at(
                       static_cast<uint32_t>(PortCfgDataFieldId::PORT_EN_ID)))
                       ? "enable"
                       : "disable"));
      }
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTable::entryGet(const tdi::Session &session,
                                    const tdi::Target &dev_tgt,
                                    const tdi::Flags &flags,
                                    const tdi::TableKey &key,
                                    tdi::TableData *data) const {
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  if (read_from_hw) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
  }

  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  return EntryGet_internal(session, dev_tgt, dev_port, data);
}

tdi_status_t PortCfgTable::entryGetFirst(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         tdi::TableKey *key,
                                         tdi::TableData *data) const {
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
  }

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirstAdded(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  if (TDI_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  PortCfgTableKey *port_key = static_cast<PortCfgTableKey *>(key);
  port_key->setId(dev_port);
  return EntryGet_internal(session, dev_tgt, dev_port, data);
}

tdi_status_t PortCfgTable::entryGetNextN(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         const uint32_t &n,
                                         keyDataPairs *key_data_pairs,
                                         uint32_t *num_returned) const {
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);
  if (read_from_hw) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
  }
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port, dev_port_n;
  dev_port = port_key.getId();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key = static_cast<PortCfgTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    status = portMgr->portMgrPortGetNextAdded(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully counted all the ports
      // in the system. In that case, reset the status and break out
      // of the loop
      status = TDI_SUCCESS;
      break;
    }
    this_key->setId(dev_port_n);
    status = EntryGet_internal(session, dev_tgt, dev_port_n, this_data);
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port_n);
      break;
    }
    dev_port = dev_port_n;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

tdi_status_t PortCfgTable::usageGet(const Session & /*session*/,
                                    const Target &dev_tgt,
                                    const Flags & /*flags*/,
                                    uint32_t *count) const {
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port, dev_port_n;
  *count = 0;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  status = portMgr->portMgrPortGetFirstAdded(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  *count = 1;
  do {
    status = portMgr->portMgrPortGetNextAdded(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      *count = 0;
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully counted all the ports
      // in the system. In that case, reset the status and break out
      // of the loop
      status = TDI_SUCCESS;
      break;
    }
    dev_port = dev_port_n;
    (*count)++;
  } while (1);
  return status;
}

tdi_status_t PortCfgTable::EntryGet_internal(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint32_t &dev_port,
                                             tdi::TableData *data) const {
  PortCfgTableData *port_data = static_cast<PortCfgTableData *>(data);
  const auto &activeDataFields = port_data->activeFieldsGet();
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  std::string table_name = tableInfoGet()->nameGet().c_str();
  auto tf_type = table_name.substr(0, 3);

  bool is_valid = false;
  bool is_recirc = false;
  status = portMgr->portMgrPortIsValidGet(port_dev_tgt.device_id, dev_port);
  if (TDI_SUCCESS != status) {
    const int MAX_RECIRC_PORTS = 30;
    uint32_t recirc_ports[MAX_RECIRC_PORTS];
    uint32_t max_ports =
        portMgr->portMgrRecircDevPortsGet(port_dev_tgt.device_id, recirc_ports);
    if (max_ports == 0) {
      LOG_TRACE("%s:%d %s: Error in getting recirc port range",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return status;
    }
    for (uint32_t i = 0; i < max_ports && i < MAX_RECIRC_PORTS; i++) {
      if (recirc_ports[i] == dev_port) {
        is_recirc = true;
        is_valid = true;
        status = TDI_SUCCESS;
        break;
      }
      if (i == (max_ports - 1)) {
        LOG_ERROR("%s:%d %s: dev_port %d is not valid",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
        return TDI_OBJECT_NOT_FOUND;
      }
    }
  } else {
    is_valid = true;
  }
  // speed
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SPEED_ID)) != activeDataFields.end()) {
    bf_port_speed_t speed = BF_SPEED_NONE;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSpeedGet(
          port_dev_tgt.device_id, dev_port, &speed);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port speed for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    const auto speed_str = utils::getStrFromDataMaps(speed, speedMap);
    if (speed_str == "UNKNOWN") {
      LOG_ERROR("%s:%d %s: Error in getting port speed string for dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID),
                        speed_str);
  }
  // lanes
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::LANES_ID)) != activeDataFields.end()) {
    int num_lanes = 1;
    if (is_recirc) {
      num_lanes = 1;
    } else {
      tdi_status_t sts = portMgr->portMgrPortNumLanesGet(
          port_dev_tgt.device_id, dev_port, &num_lanes);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting num lanes for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID),
                        static_cast<uint64_t>(num_lanes));
  }
  // enable
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_EN_ID)) != activeDataFields.end()) {
    bool is_enabled = false;
    if (is_recirc) {
      is_enabled = true;
    } else {
      tdi_status_t sts = portMgr->portMgrPortIsEnabled(
          port_dev_tgt.device_id, dev_port, &is_enabled);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting admin state for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::PORT_EN_ID),
                        is_enabled);
  }
  // direction
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_DIR_ID)) != activeDataFields.end()) {
    bf_pm_port_dir_e port_dir = PM_PORT_DIR_DEFAULT;
    if (is_recirc) {
      port_dir = PM_PORT_DIR_DEFAULT;
    } else {
      tdi_status_t sts = portMgr->portMgrPortDirGet(
          port_dev_tgt.device_id, dev_port, &port_dir);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting direction for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    const auto dir_str = utils::getStrFromDataMaps(port_dir, portDirMap);
    if (dir_str == "UNKNOWN") {
      LOG_ERROR(
          "%s:%d %s: Error in getting port direction string for dev_port %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          dev_port);
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::PORT_DIR_ID),
                        dir_str);
  }
  // SDS
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SDS_TX_ATTN_ID)) != activeDataFields.end()) {
    int tx_attn = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSerdesTxEqAttnGet(
          port_dev_tgt.device_id, dev_port, &tx_attn);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx attn for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_ATTN_ID),
        static_cast<uint64_t>(tx_attn));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SDS_TX_PRE_ID)) != activeDataFields.end()) {
    int tx_pre = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSerdesTxEqPreGet(
          port_dev_tgt.device_id, dev_port, &tx_pre);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx pre for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_PRE_ID),
        static_cast<int64_t>(tx_pre));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SDS_TX_POST_ID)) != activeDataFields.end()) {
    int tx_post = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSerdesTxEqPostGet(
          port_dev_tgt.device_id, dev_port, &tx_post);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx post for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_POST_ID),
        static_cast<int64_t>(tx_post));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SDS_TX_PRE2_ID)) != activeDataFields.end()) {
    int tx_pre2 = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSerdesTxEqPre2Get(
          port_dev_tgt.device_id, dev_port, &tx_pre2);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx pre for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_PRE2_ID),
        static_cast<int64_t>(tx_pre2));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SDS_TX_POST2_ID)) != activeDataFields.end()) {
    int tx_post2 = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortSerdesTxEqPost2Get(
          port_dev_tgt.device_id, dev_port, &tx_post2);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx post for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::SDS_TX_POST2_ID),
        static_cast<int64_t>(tx_post2));
  }
  // op state
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_UP_ID)) != activeDataFields.end()) {
    bool state = false;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortOperStateGet(
          port_dev_tgt.device_id, dev_port, &state);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port state of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    } else {
      state = false;
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::PORT_UP_ID),
                        state);
  }

  // media type
  if (tf_type != "tf2" && tf_type != "tf3") {
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::MEDIA_TP_ID)) != activeDataFields.end()) {
      std::string media_unknown = "BF_MEDIA_TYPE_UNKNOWN";
      if (is_recirc) {
        port_data->setValue(
            static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID),
            media_unknown);
      } else {
        bool is_internal = false;
        tdi_status_t sts = portMgr->portMgrPortIsInternalGet(
            port_dev_tgt.device_id,
            static_cast<bf_dev_port_t>(dev_port),
            &is_internal);
        status |= sts;
        if (sts != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s: Error in getting is_internal for port %d",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    dev_port);
        } else {
          if (is_internal) {
            port_data->setValue(
                static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID),
                media_unknown);
            LOG_DBG(
                "%s:%d %s: Port %d is is_internal. Setting media type unknown",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
          } else {
            bf_media_type_t media_type;
            sts = portMgr->portMgrPortMediaTypeGet(
                port_dev_tgt.device_id,
                static_cast<bf_dev_port_t>(dev_port),
                &media_type);
            status |= sts;
            if (TDI_SUCCESS != sts) {
              LOG_ERROR("%s:%d %s: Error in getting media type of dev_port %d",
                        __func__,
                        __LINE__,
                        tableInfoGet()->nameGet().c_str(),
                        dev_port);
            } else {
              // find the string of media_type
              auto it = mediaTypeMap.begin();
              for (; it != mediaTypeMap.end(); ++it) {
                if (it->second == media_type) {
                  port_data->setValue(
                      static_cast<uint32_t>(PortCfgDataFieldId::MEDIA_TP_ID),
                      it->first);
                  break;
                }
              }
              if (it == mediaTypeMap.end()) {
                LOG_ERROR(
                    "%s:%d %s: Error in converting the read media type enum "
                    "into "
                    "the "
                    "corresponding string for dev_port %d",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    dev_port);
              }
            }  // if sts == TDI_SUCCESS
          }    // if is_internal
        }      // if sts == TDI_SUCCESS
      }
    }
  }  // tf_type check

  // cut through
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::CT_EN_ID)) != activeDataFields.end()) {
    bool en = false;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortCutThroughGet(
          port_dev_tgt.device_id, dev_port, &en);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting cut through of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::CT_EN_ID),
                        en);
  }

  // PTP ingress timestamp delta
  if (activeDataFields.empty() ||
      activeDataFields.find(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX)) !=
          activeDataFields.end()) {
    auto tableInfo = this->tableInfoGet();
    auto tableDataField = tableInfo->dataFieldGet(
        static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX));
    if (tableDataField) {
      uint16_t delta = 0;
      if (!is_recirc) {
        tdi_status_t sts = portMgr->portMgrPtpRxDeltaGet(
            port_dev_tgt.device_id, dev_port, &delta);
        status |= sts;
        if (TDI_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Ingress Timestamp delta for "
              "dev_port %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_port);
        }
      }
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_RX),
          static_cast<uint64_t>(delta));
    }
  }

  // PTP Egress timestamp delta
  if (activeDataFields.empty() ||
      activeDataFields.find(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX)) !=
          activeDataFields.end()) {
    auto tableInfo = this->tableInfoGet();
    auto tableDataField = tableInfo->dataFieldGet(
        static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX));
    if (tableDataField) {
      uint16_t delta = 0;
      if (!is_recirc) {
        tdi_status_t sts = portMgr->portMgrPtpTxDeltaGet(
            port_dev_tgt.device_id, dev_port, &delta);
        status |= sts;
        if (TDI_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Egress Timestamp delta for "
              "dev_port %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_port);
        }
      }
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_DELTA_TX),
          static_cast<uint64_t>(delta));
    }
  }

  // PTP timestamp value , id , validity
  if (activeDataFields.empty() ||
      ((activeDataFields.find(
            static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_VALID)) !=
        activeDataFields.end()) &&
       (activeDataFields.find(
            static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_VALUE)) !=
        activeDataFields.end()) &&
       (activeDataFields.find(
            static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_ID)) !=
        activeDataFields.end()))) {
    auto tableInfo = this->tableInfoGet();
    auto tableDataField = tableInfo->dataFieldGet(
        static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_ID));
    if (tableDataField) {
      uint64_t ts = 0;
      bool ts_valid = false;
      int ts_id = 0;
      if (!is_recirc) {
        tdi_status_t sts = portMgr->portMgrPtpTxTimestampGet(
            port_dev_tgt.device_id, dev_port, &ts, &ts_valid, &ts_id);
        status |= sts;
        if (TDI_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Timestamp get for dev_port %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_port);
        }
      }
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_VALUE), ts);
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_VALID),
          ts_valid);
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TIMESTAMP_1588_ID),
          static_cast<uint64_t>(ts_id));
    }
  } else if ((activeDataFields.find(static_cast<uint32_t>(
                  PortCfgDataFieldId::TIMESTAMP_1588_VALUE)) !=
              activeDataFields.end()) ||
             (activeDataFields.find(static_cast<uint32_t>(
                  PortCfgDataFieldId::TIMESTAMP_1588_VALID)) !=
              activeDataFields.end()) ||
             (activeDataFields.find(static_cast<uint32_t>(
                  PortCfgDataFieldId::TIMESTAMP_1588_ID)) !=
              activeDataFields.end())) {
    LOG_ERROR(
        "%s:%d %s ERROR : 1588 PTP timestamp get request require "
        "ts_id , ts_value , ts_valid",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  // front port
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::CONN_ID)) != activeDataFields.end() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::CHNL_ID)) != activeDataFields.end()) {
    int conn_id = 0;
    int chnl_id = 0;
    if (!is_recirc) {
      bf_pal_front_port_handle_t port_hdl;
      tdi_status_t sts = portMgr->portMgrPortFrontPortGet(
          port_dev_tgt.device_id, dev_port, &port_hdl);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting front port of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
      if (activeDataFields.empty() ||
          activeDataFields.find(static_cast<uint32_t>(
              PortCfgDataFieldId::CONN_ID)) != activeDataFields.end())
        port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::CONN_ID),
                            static_cast<uint64_t>(port_hdl.conn_id));
      if (activeDataFields.empty() ||
          activeDataFields.find(static_cast<uint32_t>(
              PortCfgDataFieldId::CHNL_ID)) != activeDataFields.end())
        port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::CHNL_ID),
                            static_cast<uint64_t>(port_hdl.chnl_id));
    } else {
      port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::CONN_ID),
                          static_cast<uint64_t>(conn_id));
      port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::CHNL_ID),
                          static_cast<uint64_t>(chnl_id));
    }
  }
  // is internal
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_INTERNAL_ID)) != activeDataFields.end()) {
    bool is_internal;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortIsInternalGet(
          port_dev_tgt.device_id, dev_port, &is_internal);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting is_internal of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    } else {
      is_internal = false;
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::PORT_INTERNAL_ID),
        is_internal);
  }
  // is valid
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_VALID_ID)) != activeDataFields.end()) {
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::PORT_VALID_ID), is_valid);
  }
  // port str
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::PORT_NAME_ID)) != activeDataFields.end()) {
    char name[MAX_PORT_HDL_STRING_LEN];
    std::string recirc_str = "recirc";
    if (!is_recirc) {
      tdi_status_t sts =
          portMgr->portMgrPortStrGet(port_dev_tgt.device_id, dev_port, name);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port str of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
      std::string name_str(name);
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::PORT_NAME_ID), name_str);
    } else {
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::PORT_NAME_ID), recirc_str);
    }
  }
  // AN
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID)) !=
          activeDataFields.end()) {
    bf_pm_port_autoneg_policy_e an_policy = PM_AN_DEFAULT;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortAnGet(
          port_dev_tgt.device_id, dev_port, &an_policy);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting AN policy of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    }
    const auto an_str = utils::getStrFromDataMaps(an_policy, autonegoPolicyMap);
    if (an_str == "UNKNOWN") {
      LOG_ERROR(
          "c5ce7cf4 : %s:%d %s: Error in getting port direction string for "
          "dev_port %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          dev_port);
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::AN_ID),
                        an_str);
  }
  // fec
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::FEC_ID)) != activeDataFields.end()) {
    bf_fec_type_t fec_type;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortFecGet(
          port_dev_tgt.device_id, dev_port, &fec_type);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting FEC of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    } else {
      fec_type = BF_FEC_TYP_NONE;
    }
    for (auto it = fecMap.begin(); it != fecMap.end(); ++it) {
      if (it->second == fec_type) {
        port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::FEC_ID),
                            it->first);
        break;
      }
    }
  }
  // lp mood
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::LPBK_MODE_ID)) != activeDataFields.end()) {
    bf_loopback_mode_e mode;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortLoopbackModeGet(
          port_dev_tgt.device_id, dev_port, &mode);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting loopback mode of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    } else {
      mode = BF_LPBK_NONE;
    }
    for (auto it = loopbackModeMap.begin(); it != loopbackModeMap.end(); ++it) {
      if (it->second == mode) {
        port_data->setValue(
            static_cast<uint32_t>(PortCfgDataFieldId::LPBK_MODE_ID), it->first);
        break;
      }
    }
  }
  // rx/tx mtu
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::TX_MTU_ID)) != activeDataFields.end() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::RX_MTU_ID)) != activeDataFields.end()) {
    uint32_t tx_mtu = 10240, rx_mtu = 10240;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortMtuGet(
          port_dev_tgt.device_id, dev_port, &tx_mtu, &rx_mtu);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting rx/tx mtu of dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
      }
    } else {
      tx_mtu = 0;
      rx_mtu = 0;
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::TX_MTU_ID)) != activeDataFields.end())
      port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::TX_MTU_ID),
                          static_cast<uint64_t>(tx_mtu));
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::RX_MTU_ID)) != activeDataFields.end())
      port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::RX_MTU_ID),
                          static_cast<uint64_t>(rx_mtu));
  }
  // rx/tx pfc
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::TX_PFC_EN_MAP_ID)) != activeDataFields.end() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::RX_PFC_EN_MAP_ID)) != activeDataFields.end()) {
    uint32_t tx_en_map = 0, rx_en_map = 0;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortFlowControlPfcGet(
          port_dev_tgt.device_id, dev_port, &tx_en_map, &rx_en_map);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error in getting rx/tx flow control PFC of dev_port %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            dev_port);
      }
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::TX_PFC_EN_MAP_ID)) != activeDataFields.end())
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TX_PFC_EN_MAP_ID),
          static_cast<uint64_t>(tx_en_map));
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::RX_PFC_EN_MAP_ID)) != activeDataFields.end())
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::RX_PFC_EN_MAP_ID),
          static_cast<uint64_t>(rx_en_map));
  }
  // rx/tx pause
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::TX_PAUSE_EN_ID)) != activeDataFields.end() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::RX_PAUSE_EN_ID)) != activeDataFields.end()) {
    bool tx_en = false, rx_en = false;
    if (!is_recirc) {
      tdi_status_t sts = portMgr->portMgrPortFlowContrlPauseGet(
          port_dev_tgt.device_id, dev_port, &tx_en, &rx_en);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error in getting rx/tx flow control Pause of dev_port "
            "%d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            dev_port);
      }
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::TX_PAUSE_EN_ID)) != activeDataFields.end())
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::TX_PAUSE_EN_ID), tx_en);
    if (activeDataFields.empty() ||
        activeDataFields.find(static_cast<uint32_t>(
            PortCfgDataFieldId::RX_PAUSE_EN_ID)) != activeDataFields.end())
      port_data->setValue(
          static_cast<uint32_t>(PortCfgDataFieldId::RX_PAUSE_EN_ID), rx_en);
  }

  // rx parser priority threshold
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::RX_PRSR_PRI_THRESH)) != activeDataFields.end()) {
    uint32_t threshold = 0;
    auto *pipeMgr = tdi::tna::tofino::PipeMgrIntf::getInstance(session);
    if (!is_recirc) {
      tdi_status_t sts = pipeMgr->pipeMgrPortRXPrsrPriThreshGet(
          port_dev_tgt.device_id, dev_port, &threshold);
      status |= sts;
      if (TDI_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error getting ingress parser priority threshold of "
            "dev_port %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            dev_port);
      }
    }
    port_data->setValue(
        static_cast<uint32_t>(PortCfgDataFieldId::RX_PRSR_PRI_THRESH),
        static_cast<uint64_t>(threshold));
  }
  return status;
}

tdi_status_t PortCfgTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortCfgTableKey *>(key))->reset();
}

tdi_status_t PortCfgTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  return this->dataReset(emptyFields, data);
}

tdi_status_t PortCfgTable::dataReset(const std::vector<tdi_id_t> &fields,
                                     tdi::TableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortCfgTableData *>(data))->reset(0, 0, fields);
}

tdi_status_t PortCfgTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortCfgTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTable::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<tdi::TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

tdi_status_t PortCfgTable::tableAttributesSet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /* flags */,
    const tdi::TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = this->tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF: {
      void *cookie;
      bool enable;
      tdi::tna::tofino::PortStatusNotifCb callback;
      tdi_port_status_chg_cb callback_c;

      tbl_attr_impl->portStatusChangeNotifGet(
          &enable, &callback, &callback_c, &cookie);
      // Get the state and set enabled/cb/cookie
      const tdi::Device *device;
      auto status =
          DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  pipe_dev_tgt.device_id,
                  status);
        return TDI_OBJECT_NOT_FOUND;
      }

      auto device_state =
          static_cast<const tdi::tna::tofino::Device *>(device)->devStateGet(
              this->tdiInfoGet()->p4NameGet());
      // Update the state
      auto attributes_state = device_state->attributePortState.getObjState(
          this->tableInfoGet()->idGet());
      attributes_state->stateTableAttributesPortSet(
          enable, callback, callback_c, this, cookie);

      port_status_chg_cb callback_fn =
          (enable == false) ? nullptr
                            : tdi::tna::tofino::tdiPortStatusChgInternalCb;
      cookie = attributes_state.get();
      auto *portMgr = PortMgrIntf::getInstance();
      return portMgr->portMgrPortStatusNotifReg(callback_fn, cookie);
    }
    default:
      LOG_TRACE(
          "%s:%d Invalid Attribute type (%d) encountered while trying to set "
          "attributes",
          __func__,
          __LINE__,
          static_cast<int>(attr_type));
      return TDI_NOT_SUPPORTED;
  }
  return TDI_SUCCESS;
}

tdi_status_t RecircPortCfgTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortCfgTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RecircPortCfgTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RecircPortCfgTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RecircPortCfgTable::entryAdd(const Session &session,
                                          const Target &dev_tgt,
                                          const Flags & /*flags*/,
                                          const TableKey &key,
                                          const TableData &data) const {
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  bf_dev_port_t dev_port = port_key.getId();
  const PortCfgTableData &port_data =
      static_cast<const PortCfgTableData &>(data);
  tdi_status_t status = TDI_SUCCESS;
  const std::unordered_map<tdi_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  status = portMgr->portMgrPortIsSpecialPort(port_dev_tgt.device_id, dev_port);
  if (TDI_SUCCESS != status) {
    return status;
  }

  bool recirc = false;
  auto *pipeMgr = tdi::tna::tofino::PipeMgrIntf::getInstance(session);
  status =
      pipeMgr->pipeMgrRecirEnableGet(port_dev_tgt.device_id, dev_port, &recirc);
  if (TDI_SUCCESS != status) {
    return status;
  }

  bf_dev_port_t pcie_cpu = bf_pcie_cpu_port_get(port_dev_tgt.device_id);
  bf_dev_port_t pcie_cpu2 = bf_pcie_cpu_port2_get(port_dev_tgt.device_id);
  if (!recirc && (dev_port == pcie_cpu || dev_port == pcie_cpu2)) {
    return TDI_INVALID_ARG;
  }

  if ((strData.empty()) ||
      (strData.find(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)) ==
       strData.end())) {
    LOG_TRACE("%s:%d %s ERROR : Port Cfg table entry add require speed ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // check and find right enum for speed
  if ((speedMap.find(strData.at(static_cast<uint32_t>(
           PortCfgDataFieldId::SPEED_ID))) == speedMap.end())) {
    LOG_TRACE("%s:%d %s ERROR : invalid speed %s ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              strData.at(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID))
                  .c_str());
    return TDI_INVALID_ARG;
  }

  bool enable = true;
  status = pipeMgr->pipeMgrRecirEnableSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      port_dev_tgt.device_id,
      dev_port,
      enable);

  status = portMgr->portMgrNonMacPortAdd(
      port_dev_tgt.device_id,
      dev_port,
      (speedMap.at(
          strData.at(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID)))));
  return status;
}

tdi_status_t RecircPortCfgTable::entryDel(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags & /*flags*/,
                                          const tdi::TableKey &key) const {
  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  bf_dev_port_t dev_port = port_key.getId();
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  status = portMgr->portMgrPortIsSpecialPort(port_dev_tgt.device_id, dev_port);
  if (TDI_SUCCESS != status) {
    return status;
  }

  bool recirc = false;
  auto *pipeMgr = tdi::tna::tofino::PipeMgrIntf::getInstance(session);
  status =
      pipeMgr->pipeMgrRecirEnableGet(port_dev_tgt.device_id, dev_port, &recirc);
  if (TDI_SUCCESS != status) {
    return status;
  }

  bf_dev_port_t pcie_cpu = bf_pcie_cpu_port_get(port_dev_tgt.device_id);
  bf_dev_port_t pcie_cpu2 = bf_pcie_cpu_port2_get(port_dev_tgt.device_id);

  status = portMgr->portMgrNonMacPortRemove(port_dev_tgt.device_id, dev_port);
  if (status != TDI_SUCCESS) {
    return status;
  }
  if (!recirc) {
    status = pipeMgr->pipeMgrRecirEnableSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        port_dev_tgt.device_id,
        dev_port,
        true);
    if (status != TDI_SUCCESS) {
      return status;
    }
  }
  if (recirc && (dev_port == pcie_cpu || dev_port == pcie_cpu2)) {
    status = pipeMgr->pipeMgrRecirEnableSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        port_dev_tgt.device_id,
        dev_port,
        false);
    if (status != TDI_SUCCESS) {
      return status;
    }
    status = portMgr->portMgrPcieCpuPortAdd(port_dev_tgt.device_id, dev_port);
  }
  return status;
}

tdi_status_t RecircPortCfgTable::entryGet(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi::TableKey &key,
                                          tdi::TableData *data) const {
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  if (!read_from_hw) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
  }

  const PortCfgTableKey &port_key = static_cast<const PortCfgTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  return EntryGet_internal(session, dev_tgt, dev_port, data);
}

tdi_status_t RecircPortCfgTable::EntryGet_internal(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const uint32_t &dev_port,
    tdi::TableData *data) const {
  PortCfgTableData *port_data = static_cast<PortCfgTableData *>(data);
  const auto &activeDataFields = port_data->activeFieldsGet();
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  status = portMgr->portMgrPortIsSpecialPort(port_dev_tgt.device_id, dev_port);
  if (TDI_SUCCESS != status) {
    return status;
  }
  bf_port_speed_t speed;
  uint32_t n_lanes;

  status = portMgr->portMgrNonMacPortInfoGet(
      port_dev_tgt.device_id, dev_port, &speed, &n_lanes);
  if (TDI_SUCCESS != status) {
    return status;
  }

  // speed
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::SPEED_ID)) != activeDataFields.end()) {
    const auto speed_str = utils::getStrFromDataMaps(speed, speedMap);
    if (speed_str == "UNKNOWN") {
      LOG_ERROR("%s:%d %s: Error in getting port speed string for dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
    }
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::SPEED_ID),
                        speed_str);
  }
  // lanes
  if (activeDataFields.empty() ||
      activeDataFields.find(static_cast<uint32_t>(
          PortCfgDataFieldId::LANES_ID)) != activeDataFields.end()) {
    int num_lanes = n_lanes;
    port_data->setValue(static_cast<uint32_t>(PortCfgDataFieldId::LANES_ID),
                        static_cast<uint64_t>(num_lanes));
  }
  return status;
}

// Port Stat Table
tdi_status_t PortStatTable::clear(const Session & /*session*/,
                                  const Target &dev_tgt,
                                  const Flags & /*flags*/) const {
  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirstAdded(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  if (TDI_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    LOG_DBG("%s:%d %s: No ports in the system to clear stats for",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = portMgr->portMgrPortAllStatsClear(port_dev_tgt.device_id, dev_port);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in clearing all stats for dev_port %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_port);
    return status;
  }
  bf_dev_port_t dev_port_n;
  while (status == TDI_SUCCESS) {
    status = portMgr->portMgrPortGetNextAdded(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_TRACE("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      return status;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // We have cleared the stats for all the ports
      return TDI_SUCCESS;
    }
    status =
        portMgr->portMgrPortAllStatsClear(port_dev_tgt.device_id, dev_port_n);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in clearing all stats for dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port_n);
      return status;
    }
    dev_port = dev_port_n;
  }
  return status;
}

tdi_status_t PortStatTable::entryMod(const Session & /*session*/,
                                     const Target &dev_tgt,
                                     const Flags & /*flags*/,
                                     const TableKey &key,
                                     const TableData &data) const {
  const PortStatTableKey &port_key = static_cast<const PortStatTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  const PortStatTableData &port_data =
      static_cast<const PortStatTableData &>(data);
  const auto activeDataFields = port_data.activeFieldsGet();
  auto *portMgr = PortMgrIntf::getInstance();

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  tdi_status_t status = TDI_SUCCESS;
  const uint64_t *u64Data = port_data.getU64FieldData();
  for (auto n = activeDataFields.begin(); n != activeDataFields.end(); ++n) {
    if (u64Data[(*n) - 1] != 0) {
      LOG_TRACE("%s:%d %s: Error data cannot be non-zero value",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str());
      return TDI_INVALID_ARG;
    }
  }
  if (activeDataFields.empty()) {
    // clear all
    status =
        portMgr->portMgrPortAllStatsClear(port_dev_tgt.device_id, dev_port);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error clear all stat for dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      return status;
    }
  } else {
    for (const auto &id : (activeDataFields)) {
      status = portMgr->portMgrPortThisStatClear(
          port_dev_tgt.device_id,
          dev_port,
          static_cast<bf_rmon_counter_t>(id - 1));
      if (status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d %s: Error clear stat %d for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  (id - 1),
                  dev_port);
        continue;
      }
    }
  }
  return status;
}

tdi_status_t PortStatTable::entryGet(const tdi::Session & /*session*/,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     tdi::TableData *data) const {
  const PortStatTableKey &port_key = static_cast<const PortStatTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  PortStatTableData *port_data = static_cast<PortStatTableData *>(data);
  return this->EntryGet_internal(dev_tgt, flags, dev_port, port_data);
}

tdi_status_t PortStatTable::EntryGet_internal(
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const uint32_t &dev_port,
    PortStatTableData *port_data) const {
  const auto activeDataFields = port_data->activeFieldsGet();

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_TOFINO_FLAGS_FROM_HW),
                 &read_from_hw);

  if (!read_from_hw) {
    // read from shadow
    if (activeDataFields.empty() ||
        (activeDataFields.size() >
         port_data->getAllStatsBoundry())) {  // getAllStatsBoundry is the
                                              // boundry of using get_all to
                                              // increase efficiency
      // all fields
      uint64_t stats[BF_NUM_RMON_COUNTERS];
      status = portMgr->portMgrPortAllStatsGet(
          port_dev_tgt.device_id, dev_port, stats);
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s: Error getting all stat for dev_port %d",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dev_port);
        return status;
      }
      port_data->setAllValues(stats);
    } else {
      uint64_t val;
      for (const auto &id : (activeDataFields)) {
        status = portMgr->portMgrPortThisStatGet(
            port_dev_tgt.device_id,
            dev_port,
            static_cast<bf_rmon_counter_t>(id - 1),
            &val);
        if (status != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s: Error getting stat for dev_port %d, ctr_type %d",
                    __func__,
                    __LINE__,
                    tableInfoGet()->nameGet().c_str(),
                    dev_port,
                    id);
          continue;
        }
        port_data->setValue(id, val);
      }
    }
  } else {
    bf_rmon_counter_t ctr_type_array[BF_NUM_RMON_COUNTERS];
    uint64_t stat_val[BF_NUM_RMON_COUNTERS];
    uint32_t num_of_ctr;
    // read from hw
    if (activeDataFields.empty()) {
      num_of_ctr = BF_NUM_RMON_COUNTERS - 1;
      for (uint32_t i = 0; i <= num_of_ctr; i++) {
        ctr_type_array[i] = static_cast<bf_rmon_counter_t>(i);
      }
    } else {
      num_of_ctr = activeDataFields.size() - 1;
      auto it = activeDataFields.begin();
      for (uint32_t i = 0; i <= num_of_ctr; i++, it++) {
        ctr_type_array[i] =
            static_cast<bf_rmon_counter_t>(static_cast<uint32_t>(*it) - 1);
      }
    }
    status = portMgr->portMgrPortStatDirectGet(
        port_dev_tgt.device_id, dev_port, ctr_type_array, stat_val, num_of_ctr);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error getting hw stat for dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      return status;
    }
    for (uint32_t i = 0; i <= num_of_ctr; i++) {
      // We need to add 1 to the field ids, as they start from 1 in tdi json
      port_data->setValue(ctr_type_array[i] + 1, stat_val[i]);
    }
  }
  bf_pkt_rate_t pkt_rate;
  status = portMgr->portMgrPortPacketRateGet(
      port_dev_tgt.device_id, dev_port, &pkt_rate);
  port_data->setValue(static_cast<uint32_t>(PktRateField::TX_PPS),
                      pkt_rate.tx_pps);
  port_data->setValue(static_cast<uint32_t>(PktRateField::RX_PPS),
                      pkt_rate.rx_pps);
  port_data->setValue(static_cast<uint32_t>(PktRateField::TX_RATE),
                      pkt_rate.tx_rate);
  port_data->setValue(static_cast<uint32_t>(PktRateField::RX_RATE),
                      pkt_rate.rx_rate);

  return status;
}

tdi_status_t PortStatTable::entryGetFirst(const tdi::Session & /*session*/,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          tdi::TableKey *key,
                                          tdi::TableData *data) const {
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port;
  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  status = portMgr->portMgrPortGetFirstAdded(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  if (TDI_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  PortStatTableKey *port_key = static_cast<PortStatTableKey *>(key);
  port_key->setId(dev_port);
  auto port_data = static_cast<PortStatTableData *>(data);
  return this->EntryGet_internal(dev_tgt, flags, dev_port, port_data);
}

tdi_status_t PortStatTable::entryGetNextN(const tdi::Session & /*session*/,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi::TableKey &key,
                                          const uint32_t &n,
                                          keyDataPairs *key_data_pairs,
                                          uint32_t *num_returned) const {
  const PortStatTableKey &port_key = static_cast<const PortStatTableKey &>(key);
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  uint32_t dev_port, dev_port_n;
  dev_port = port_key.getId();
  auto *portMgr = PortMgrIntf::getInstance();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key = static_cast<PortStatTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<PortStatTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNextAdded(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = TDI_SUCCESS;
      break;
    }
    this_key->setId(dev_port_n);
    status = this->EntryGet_internal(dev_tgt, flags, dev_port_n, this_data);
    if (TDI_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port_n);
      break;
    }
    dev_port = dev_port_n;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

tdi_status_t PortStatTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortStatTableKey *>(key))->reset();
}

tdi_status_t PortStatTable::dataReset(const std::vector<tdi_id_t> &fields,
                                      tdi::TableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortStatTableData *>(data))->reset(0, 0, fields);
}

tdi_status_t PortStatTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  return (static_cast<PortStatTableData *>(data))->reset(0, 0, emptyFields);
}

tdi_status_t PortStatTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortStatTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStatTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStatTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new PortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStatTable::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<tdi::TableAttributes>(
      new tdi::tna::tofino::TableAttributes(this, type));
  return TDI_SUCCESS;
}

#if 0
tdi_status_t PortStatTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<tdi::TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<tdi::tna::tofino::TableAttributes &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::PORT_STAT_POLL_INTVL_MS:
      break;
    default:
      LOG_TRACE("%s:%d Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}
#endif

tdi_status_t PortStatTable::tableAttributesSet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /* flags */,
    const tdi::TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const tdi::tna::tofino::TableAttributes *>(&tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = this->tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS: {
      uint32_t intvl;
      tdi_status_t sts = tbl_attr_impl->portStatPollIntvlMsGet(&intvl);
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      auto *portMgr = PortMgrIntf::getInstance();
      return portMgr->portMgrPortStatsPollIntvlSet(pipe_dev_tgt.device_id,
                                                   intvl);
    }
    default:
      LOG_TRACE(
          "%s:%d Invalid Attribute type (%d) encountered while trying to set "
          "attributes",
          __func__,
          __LINE__,
          static_cast<int>(attr_type));
      return TDI_NOT_SUPPORTED;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStatTable::tableAttributesGet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /* flags */,
    tdi::TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl =
      static_cast<tdi::tna::tofino::TableAttributes *>(tableAttributes);
  const auto attr_type = static_cast<tdi_tofino_attributes_type_e>(
      tbl_attr_impl->attributeTypeGet());
  auto attribute_type_set = this->tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(static_cast<tdi_attributes_type_e>(attr_type)) ==
      attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);
  switch (attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS: {
      auto *portMgr = PortMgrIntf::getInstance();
      uint32_t intvl;
      auto sts =
          portMgr->portMgrPortStatsPollIntvlGet(pipe_dev_tgt.device_id, &intvl);
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get Port Stats Poll Intvl",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      return tbl_attr_impl->portStatPollIntvlMsSet(intvl);
    }
    default:
      LOG_TRACE(
          "%s:%d Invalid Attribute type (%d) encountered while trying to get "
          "attributes",
          __func__,
          __LINE__,
          static_cast<int>(attr_type));
      return TDI_NOT_SUPPORTED;
  }
  return TDI_SUCCESS;
}

// Port Str Info Table
tdi_status_t PortStrInfoTable::entryGet(const tdi::Session & /*session*/,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortStrInfoTableKey &port_key =
      static_cast<const PortStrInfoTableKey &>(key);
  std::string port_str = port_key.getPortStr();
  uint32_t dev_port;
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = portMgr->portMgrPortStrToDevPortGet(
      port_dev_tgt.device_id,
      const_cast<char *>(port_str.c_str()),
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting Port %s dev_port err",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              port_str.c_str());
    return status;
  }
  PortStrInfoTableData *port_data = static_cast<PortStrInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortStrInfoTable::entryGetFirst(const tdi::Session & /*session*/,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirst(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  char port_str[MAX_PORT_HDL_STRING_LEN] = {'\0'};
  status =
      portMgr->portMgrPortStrGet(port_dev_tgt.device_id, dev_port, port_str);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting front panel port string for dev_port %d with "
        "err : %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        dev_port,
        status);
    return status;
  }
  if (TDI_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  PortStrInfoTableKey *port_key = static_cast<PortStrInfoTableKey *>(key);
  port_key->setPortStr(std::string(port_str));
  PortStrInfoTableData *port_data = static_cast<PortStrInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortStrInfoTable::entryGetNextN(const tdi::Session & /*session*/,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             const uint32_t &n,
                                             keyDataPairs *key_data_pairs,
                                             uint32_t *num_returned) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortStrInfoTableKey &port_key =
      static_cast<const PortStrInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port, dev_port_n;
  std::string port_str = port_key.getPortStr();
  status = portMgr->portMgrPortStrToDevPortGet(
      port_dev_tgt.device_id,
      const_cast<char *>(port_str.data()),
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting dev_port for port with name %s: err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              port_str.c_str(),
              status);
    return status;
  }
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<PortStrInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<PortStrInfoTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNext(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = TDI_SUCCESS;
      break;
    }
    char port_name[MAX_PORT_HDL_STRING_LEN] = {'\0'};
    status = portMgr->portMgrPortStrGet(
        port_dev_tgt.device_id, dev_port_n, port_name);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error getting front panel port name for dev_port %d with "
          "err : %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          dev_port_n,
          status);
      return status;
    }
    this_key->setPortStr(std::string(port_name));
    this_data->setDevPort(dev_port_n);
    dev_port = dev_port_n;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

tdi_status_t PortStrInfoTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortStrInfoTableKey *>(key))->reset();
}

tdi_status_t PortStrInfoTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortStrInfoTableData *>(data))->reset(0, 0, emptyFields);
}

tdi_status_t PortStrInfoTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortStrInfoTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortStrInfoTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new PortStrInfoTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// Port Hdl Info Table
tdi_status_t PortHdlInfoTable::entryGet(const tdi::Session & /*session*/,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        const tdi::TableKey &key,
                                        tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortHdlInfoTableKey &port_key =
      static_cast<const PortHdlInfoTableKey &>(key);
  bf_pal_front_port_handle_t port_hdl;
  uint32_t dev_port;
  port_key.getPortHdl(&port_hdl.conn_id, &port_hdl.chnl_id);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = portMgr->portMgrFrontPortToDevPortGet(
      port_dev_tgt.device_id,
      &port_hdl,
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for port with port_hdl %d/%d err",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        port_hdl.conn_id,
        port_hdl.chnl_id);
    return status;
  }
  PortHdlInfoTableData *port_data = static_cast<PortHdlInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortHdlInfoTable::entryGetFirst(const tdi::Session & /*session*/,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirst(
      port_dev_tgt.device_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  bf_pal_front_port_handle_t port_hdl;
  status = portMgr->portMgrPortFrontPortGet(
      port_dev_tgt.device_id, dev_port, &port_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting front panel port for dev_port %d with err : "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        dev_port,
        status);
    return status;
  }
  if (TDI_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  PortHdlInfoTableKey *port_key = static_cast<PortHdlInfoTableKey *>(key);
  port_key->setPortHdl(port_hdl.conn_id, port_hdl.chnl_id);
  PortHdlInfoTableData *port_data = static_cast<PortHdlInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortHdlInfoTable::entryGetNextN(const tdi::Session & /*session*/,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             const uint32_t &n,
                                             keyDataPairs *key_data_pairs,
                                             uint32_t *num_returned) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortHdlInfoTableKey &port_key =
      static_cast<const PortHdlInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t dev_port, dev_port_n;
  bf_pal_front_port_handle_t port_hdl = {0};
  port_key.getPortHdl(&port_hdl.conn_id, &port_hdl.chnl_id);
  status = portMgr->portMgrFrontPortToDevPortGet(
      port_dev_tgt.device_id,
      &port_hdl,
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for port with port_hdl %d/%d err",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        port_hdl.conn_id,
        port_hdl.chnl_id);
    return status;
  }
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<PortHdlInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<PortHdlInfoTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNext(
        port_dev_tgt.device_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                dev_port);
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = TDI_SUCCESS;
      break;
    }
    status = portMgr->portMgrPortFrontPortGet(
        port_dev_tgt.device_id, dev_port_n, &port_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error getting front panel port for dev_port %d with err : "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          dev_port_n,
          status);
      return status;
    }
    this_key->setPortHdl(port_hdl.conn_id, port_hdl.chnl_id);
    this_data->setDevPort(dev_port_n);
    dev_port = dev_port_n;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

tdi_status_t PortHdlInfoTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortHdlInfoTableKey *>(key))->reset();
}

tdi_status_t PortHdlInfoTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortHdlInfoTableData *>(data))->reset(0, 0, emptyFields);
}

tdi_status_t PortHdlInfoTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortHdlInfoTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortHdlInfoTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new PortHdlInfoTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

// Port Fp Idx Info Table
tdi_status_t PortFpIdxInfoTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortFpIdxInfoTableKey *>(key))->reset();
}

tdi_status_t PortFpIdxInfoTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return (static_cast<PortFpIdxInfoTableData *>(data))
      ->reset(0, 0, emptyFields);
}

tdi_status_t PortFpIdxInfoTable::entryGet(const tdi::Session & /*session*/,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi::TableKey &key,
                                          tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortFpIdxInfoTableKey &port_key =
      static_cast<const PortFpIdxInfoTableKey &>(key);
  uint32_t fp_idx = port_key.getId();
  uint32_t dev_port;
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = portMgr->portMgrFpIdxToDevPortGet(
      port_dev_tgt.device_id,
      fp_idx,
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting Port with fp_idx 0x%x dev_port err",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              fp_idx);
    return status;
  }
  PortFpIdxInfoTableData *port_data =
      static_cast<PortFpIdxInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortFpIdxInfoTable::entryGetFirst(const tdi::Session & /*session*/,
                                               const tdi::Target &dev_tgt,
                                               const tdi::Flags &flags,
                                               tdi::TableKey *key,
                                               tdi::TableData *data) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  uint32_t fp_idx;
  status = portMgr->portMgrFpIdxGetFirst(port_dev_tgt.device_id, &fp_idx);
  if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first front port index",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  bf_dev_port_t dev_port;
  status = portMgr->portMgrFpIdxToDevPortGet(
      port_dev_tgt.device_id, fp_idx, &dev_port);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for front port index %d with err : "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        fp_idx,
        status);
    return status;
  }
  PortFpIdxInfoTableKey *port_key = static_cast<PortFpIdxInfoTableKey *>(key);
  port_key->setId(fp_idx);
  PortFpIdxInfoTableData *port_data =
      static_cast<PortFpIdxInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return TDI_SUCCESS;
}

tdi_status_t PortFpIdxInfoTable::entryGetNextN(const tdi::Session & /*session*/,
                                               const tdi::Target &dev_tgt,
                                               const tdi::Flags &flags,
                                               const tdi::TableKey &key,
                                               const uint32_t &n,
                                               keyDataPairs *key_data_pairs,
                                               uint32_t *num_returned) const {
  CHECK_AND_RETURN_HW_FLAG(flags);

  dev_target_t port_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&port_dev_tgt, nullptr, nullptr);

  const PortFpIdxInfoTableKey &port_key =
      static_cast<const PortFpIdxInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  tdi_status_t status = TDI_SUCCESS;
  bf_dev_port_t dev_port;
  uint32_t fp_idx, fp_idx_n;
  fp_idx = port_key.getId();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<PortFpIdxInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<PortFpIdxInfoTableData *>((*key_data_pairs)[i].second);
    status =
        portMgr->portMgrFpIdxGetNext(port_dev_tgt.device_id, fp_idx, &fp_idx_n);
    if (TDI_SUCCESS != status && TDI_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next fp idx after %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                fp_idx);
      break;
    }
    if (TDI_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = TDI_SUCCESS;
      break;
    }
    status = portMgr->portMgrFpIdxToDevPortGet(
        port_dev_tgt.device_id, fp_idx_n, &dev_port);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error getting dev port for fp idx %d with err : %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                fp_idx_n,
                status);
      return status;
    }
    this_key->setId(fp_idx_n);
    this_data->setDevPort(dev_port);
    fp_idx = fp_idx_n;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

tdi_status_t PortFpIdxInfoTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new PortFpIdxInfoTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t PortFpIdxInfoTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new PortFpIdxInfoTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

}  // namespace tdi
