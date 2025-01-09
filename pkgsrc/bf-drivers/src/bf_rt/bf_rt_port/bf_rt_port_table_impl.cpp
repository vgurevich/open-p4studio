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


#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_table_attributes_impl.hpp>
#include "bf_rt_port_table_attributes_state.hpp"

#include "bf_rt_port_table_impl.hpp"
#include "bf_rt_port_table_data_impl.hpp"
#include "bf_rt_port_table_key_impl.hpp"
#include "bf_rt_port_mgr_intf.hpp"

namespace bfrt {
// Port Configuration Table
enum PortCfgDataFieldId {
  SPEED_ID = 1,
  FEC_ID = 2,
  LANES_ID = 3,
  PORT_EN_ID = 4,
  AN_ID = 5,
  LP_MOOD_ID = 6,
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
  TIMESTAMP_1588_ID = 32,
  PLL_OVRCLK_ID = 33
};

enum PktRateField { TX_PPS = 92, RX_PPS = 93, TX_RATE = 94, RX_RATE = 95 };

void BfRtPortCfgTable::mapInit() {
  speedMap["BF_SPEED_NONE"] = BF_SPEED_NONE;
  speedMap["BF_SPEED_1G"] = BF_SPEED_1G;
  speedMap["BF_SPEED_10G"] = BF_SPEED_10G;
  speedMap["BF_SPEED_25G"] = BF_SPEED_25G;
  speedMap["BF_SPEED_40G"] = BF_SPEED_40G;
  speedMap["BF_SPEED_50G"] = BF_SPEED_50G;
  speedMap["BF_SPEED_100G"] = BF_SPEED_100G;
  speedMap["BF_SPEED_200G"] = BF_SPEED_200G;
  speedMap["BF_SPEED_400G"] = BF_SPEED_400G;

  fecMap["BF_FEC_TYP_NONE"] = BF_FEC_TYP_NONE;
  fecMap["BF_FEC_TYP_FIRECODE"] = BF_FEC_TYP_FIRECODE;
  fecMap["BF_FEC_TYP_REED_SOLOMON"] = BF_FEC_TYP_REED_SOLOMON;
  fecMap["BF_FEC_TYP_FC"] = BF_FEC_TYP_FC;
  fecMap["BF_FEC_TYP_RS"] = BF_FEC_TYP_RS;

  mediaTypeMap["BF_MEDIA_TYPE_COPPER"] = BF_MEDIA_TYPE_COPPER;
  mediaTypeMap["BF_MEDIA_TYPE_OPTICAL"] = BF_MEDIA_TYPE_OPTICAL;
  mediaTypeMap["BF_MEDIA_TYPE_UNKNOWN"] = BF_MEDIA_TYPE_UNKNOWN;

  portDirMap["PM_PORT_DIR_DEFAULT"] = PM_PORT_DIR_DEFAULT;
  portDirMap["PM_PORT_DIR_TX_ONLY"] = PM_PORT_DIR_TX_ONLY;
  portDirMap["PM_PORT_DIR_RX_ONLY"] = PM_PORT_DIR_RX_ONLY;

  loopbackModeMap["BF_LPBK_NONE"] = BF_LPBK_NONE;
  loopbackModeMap["BF_LPBK_MAC_NEAR"] = BF_LPBK_MAC_NEAR;
  loopbackModeMap["BF_LPBK_MAC_FAR"] = BF_LPBK_MAC_FAR;
  loopbackModeMap["BF_LPBK_PCS_NEAR"] = BF_LPBK_PCS_NEAR;
  loopbackModeMap["BF_LPBK_SERDES_NEAR"] = BF_LPBK_SERDES_NEAR;
  loopbackModeMap["BF_LPBK_SERDES_FAR"] = BF_LPBK_SERDES_FAR;
  loopbackModeMap["BF_LPBK_PIPE"] = BF_LPBK_PIPE;

  autonegoPolicyMap["PM_AN_DEFAULT"] = PM_AN_DEFAULT;
  autonegoPolicyMap["PM_AN_FORCE_ENABLE"] = PM_AN_FORCE_ENABLE;
  autonegoPolicyMap["PM_AN_FORCE_DISABLE"] = PM_AN_FORCE_DISABLE;
}

bf_status_t BfRtPortCfgTable::tableEntryMod_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint32_t &dev_port,
    const BfRtTableData &data) const {
  const BfRtPortCfgTableData port_data =
      static_cast<const BfRtPortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  const std::unordered_map<bf_rt_id_t, bool> &boolData =
      port_data.getBoolFieldDataMap();
  const std::unordered_map<bf_rt_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  const std::unordered_map<bf_rt_id_t, int32_t> &i32Data =
      port_data.getI32FieldDataMap();
  const std::unordered_map<bf_rt_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();
  const std::unordered_map<bf_rt_id_t, float> &floatData =
      port_data.getFloatFieldDataMap();

  // step 2: cfg ports
  // AN
  if (strData.find(AN_ID) != strData.end()) {
    if (autonegoPolicyMap.find(strData.at(AN_ID)) == autonegoPolicyMap.end()) {
      LOG_ERROR("%s:%d %s ERROR : Invalid AN Policy %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(AN_ID).c_str());
      return BF_INVALID_ARG;
    }
    status = portMgr->portMgrPortAnPolicySet(
        dev_tgt.dev_id, dev_port, autonegoPolicyMap.at(strData.at(AN_ID)));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting AN Policy %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                u32Data.at(AN_ID));
    }
  }
  // Loopback
  if (strData.find(LP_MOOD_ID) != strData.end()) {
    if (loopbackModeMap.find(strData.at(LP_MOOD_ID)) == loopbackModeMap.end()) {
      LOG_TRACE("%s:%d %s ERROR : invalid loopback mode %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(LP_MOOD_ID).c_str());
      return BF_INVALID_ARG;
    }
    status = portMgr->portMgrPortSetLoopbackMode(
        dev_tgt.dev_id, dev_port, loopbackModeMap.at(strData.at(LP_MOOD_ID)));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting Loopback Mode %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                (strData.at(LP_MOOD_ID)).c_str());
    }
  }
  // mtu
  if ((u32Data.find(TX_MTU_ID) != u32Data.end()) &&
      (u32Data.find(RX_MTU_ID) != u32Data.end())) {
    status = portMgr->portMgrPortMtuSet(
        dev_tgt.dev_id, dev_port, u32Data.at(TX_MTU_ID), u32Data.at(RX_MTU_ID));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting tx_mtu %d and rx_mtu %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                u32Data.at(TX_MTU_ID),
                u32Data.at(RX_MTU_ID));
    }
  } else if (!((u32Data.find(TX_MTU_ID) == u32Data.end()) &&
               (u32Data.find(RX_MTU_ID) == u32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port mtu configuration require both tx_mtu and "
        "rx_mtu",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }
  // PLL Overclock
  if (floatData.find(PLL_OVRCLK_ID) != floatData.end()) {
    status = portMgr->portMgrPortPllOvrclkSet(
        dev_tgt.dev_id, dev_port, floatData.at(PLL_OVRCLK_ID));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting PLL Overclock %f%%",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                floatData.at(PLL_OVRCLK_ID));
    }
  }
  // pfc
  if ((u32Data.find(TX_PFC_EN_MAP_ID) != u32Data.end()) &&
      (u32Data.find(RX_PFC_EN_MAP_ID) != u32Data.end())) {
    status =
        portMgr->portMgrPortFlowControlPfcSet(dev_tgt.dev_id,
                                              dev_port,
                                              u32Data.at(TX_PFC_EN_MAP_ID),
                                              u32Data.at(RX_PFC_EN_MAP_ID));
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting tx_pfc_en_map %d and rx_pfc_en_map %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          u32Data.at(TX_PFC_EN_MAP_ID),
          u32Data.at(RX_PFC_EN_MAP_ID));
    }
  } else if (!((u32Data.find(TX_PFC_EN_MAP_ID) == u32Data.end()) &&
               (u32Data.find(RX_PFC_EN_MAP_ID) == u32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port pfc configuration require both tx_pfc_en_map "
        "and rx_pfc_en_map",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }
  // ingress parser priority threshold
  if (u32Data.find(RX_PRSR_PRI_THRESH) != u32Data.end()) {
    auto *pipeMgr = PipeMgrIntf::getInstance(session);
    status = pipeMgr->pipeMgrPortRXPrsrPriThreshSet(
        dev_tgt.dev_id, dev_port, u32Data.at(RX_PRSR_PRI_THRESH));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting rx parser priorty threshold %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                u32Data.at(RX_PRSR_PRI_THRESH));
    }
  }

  // ingress PTP timestamp
  if (u32Data.find(TIMESTAMP_1588_DELTA_RX) != u32Data.end()) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (getDataField(TIMESTAMP_1588_DELTA_RX, &tableDataField) == BF_SUCCESS) {
      status = portMgr->portMgrPtpRxDeltaSet(
          dev_tgt.dev_id, dev_port, u32Data.at(TIMESTAMP_1588_DELTA_RX));
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting 1588 PTP rx delta value:  %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  u32Data.at(TIMESTAMP_1588_DELTA_RX));
      }
    } else {
      LOG_ERROR("%s:%d %s: Error: TIMESTAMP_1588_DELTA_RX  Not supported ",
                __func__,
                __LINE__,
                table_name_get().c_str());
    }
  }

  // Egress PTP timestamp
  if (u32Data.find(TIMESTAMP_1588_DELTA_TX) != u32Data.end()) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (getDataField(TIMESTAMP_1588_DELTA_TX, &tableDataField) == BF_SUCCESS) {
      status = portMgr->portMgrPtpTxDeltaSet(
          dev_tgt.dev_id, dev_port, u32Data.at(TIMESTAMP_1588_DELTA_TX));
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting 1588 PTP tx delta value:  %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  u32Data.at(TIMESTAMP_1588_DELTA_TX));
      }
    } else {
      LOG_ERROR("%s:%d %s: Error: TIMESTAMP_1588_DELTA_TX  Not supported ",
                __func__,
                __LINE__,
                table_name_get().c_str());
    }
  }

  // port dir
  if (strData.find(PORT_DIR_ID) != strData.end()) {
    if (portDirMap.find(strData.at(PORT_DIR_ID)) == portDirMap.end()) {
      LOG_TRACE("%s:%d %s ERROR : invalid port dir %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(PORT_DIR_ID).c_str());
      return BF_INVALID_ARG;
    }
    status = portMgr->portMgrPortDirSet(dev_tgt.dev_id,
                                        static_cast<bf_dev_port_t>(dev_port),
                                        portDirMap.at(strData.at(PORT_DIR_ID)));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting port direction %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(PORT_DIR_ID).c_str());
    }
  }
  // media type
  if (strData.find(MEDIA_TP_ID) != strData.end()) {
    if (mediaTypeMap.find(strData.at(MEDIA_TP_ID)) == mediaTypeMap.end()) {
      LOG_TRACE("%s:%d %s ERROR : invalid media type %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(MEDIA_TP_ID).c_str());
      return BF_INVALID_ARG;
    }
    status = portMgr->portMgrPortMediaTypeSet(
        dev_tgt.dev_id,
        static_cast<bf_dev_port_t>(dev_port),
        mediaTypeMap.at(strData.at(MEDIA_TP_ID)));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting media type %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(MEDIA_TP_ID).c_str());
    }
  }
  // serdes params
  if ((u32Data.find(SDS_TX_ATTN_ID) != u32Data.end()) &&
      (i32Data.find(SDS_TX_PRE_ID) != i32Data.end()) &&
      (i32Data.find(SDS_TX_POST_ID) != i32Data.end())) {
    bf_pal_serdes_params_t serdes_param;
    serdes_param.tx_attn = u32Data.at(SDS_TX_ATTN_ID);
    serdes_param.tx_pre = i32Data.at(SDS_TX_PRE_ID);
    serdes_param.tx_post = i32Data.at(SDS_TX_POST_ID);
    status = portMgr->portMgrPortSerdesParamsSet(
        dev_tgt.dev_id, dev_port, &serdes_param);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in setting serdes params tx_attn %d, tx_pre %d, "
          "tx_post %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          u32Data.at(SDS_TX_ATTN_ID),
          i32Data.at(SDS_TX_PRE_ID),
          i32Data.at(SDS_TX_POST_ID));
    }
  } else if (!((u32Data.find(SDS_TX_ATTN_ID) == u32Data.end()) &&
               (i32Data.find(SDS_TX_PRE_ID) == i32Data.end()) &&
               (i32Data.find(SDS_TX_POST_ID) == i32Data.end()))) {
    LOG_ERROR(
        "%s:%d %s ERROR : port serdes params configuration require "
        "sds_tx_attn, sds_tx_pre, and sds_tx_post",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  if (!boolData.empty()) {
    // pause frame
    if ((boolData.find(TX_PAUSE_EN_ID) != boolData.end()) &&
        (boolData.find(RX_PAUSE_EN_ID) != boolData.end())) {
      status = portMgr->portMgrPortFlowControlPauseFrameSet(
          dev_tgt.dev_id,
          dev_port,
          boolData.at(TX_PAUSE_EN_ID),
          boolData.at(RX_PAUSE_EN_ID));
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s: Error in setting tx_pause_frame_en %s and "
            "rx_pause_frame_en %s",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            (boolData.at(TX_PAUSE_EN_ID) ? "enable" : "disable"),
            (boolData.at(RX_PAUSE_EN_ID) ? "enable" : "disable"));
      }
    } else if (!((boolData.find(TX_PAUSE_EN_ID) == boolData.end()) &&
                 (boolData.find(RX_PAUSE_EN_ID) == boolData.end()))) {
      LOG_ERROR(
          "%s:%d %s ERROR : port pause frame configuration require both "
          "tx_pause_frame_en and rx_pause_frame_en",
          __func__,
          __LINE__,
          table_name_get().c_str());
    }
    // cut through en
    if (boolData.find(CT_EN_ID) != boolData.end()) {
      status = portMgr->portMgrPortCutThroughSet(
          dev_tgt.dev_id, dev_port, boolData.at(CT_EN_ID));
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting port cut through enable %s",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  (boolData.at(CT_EN_ID) ? "enable" : "disable"));
      }
    }
    // step 3: enable port (u32Data[4]) at last
    if (boolData.find(PORT_EN_ID) != boolData.end()) {
      status = portMgr->portMgrPortEnable(
          dev_tgt.dev_id, dev_port, boolData.at(PORT_EN_ID));
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  ((boolData.at(PORT_EN_ID)) ? "enable" : "disable"));
      }
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::tableEntryAdd(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  const uint32_t dev_port = port_key.getId();
  const BfRtPortCfgTableData &port_data =
      static_cast<const BfRtPortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  const std::unordered_map<bf_rt_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  const std::unordered_map<bf_rt_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();

  if ((strData.empty()) || (strData.find(SPEED_ID) == strData.end()) ||
      (strData.find(FEC_ID) == strData.end())) {
    LOG_TRACE(
        "%s:%d %s ERROR : Port Cfg table entry add require both speed and fec",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // check and find right enum for speed and fec
  if ((speedMap.find(strData.at(SPEED_ID)) == speedMap.end()) ||
      (fecMap.find(strData.at(FEC_ID)) == fecMap.end())) {
    LOG_TRACE("%s:%d %s ERROR : invalid speed %s or fec %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              strData.at(SPEED_ID).c_str(),
              strData.at(FEC_ID).c_str());
    return BF_INVALID_ARG;
  }
  // step 1: add port first
  if (u32Data.find(LANES_ID) == u32Data.end()) {
    status = portMgr->portMgrPortAdd(dev_tgt.dev_id,
                                     dev_port,
                                     (speedMap.at(strData.at(SPEED_ID))),
                                     (fecMap.at(strData.at(FEC_ID))));
  } else {
    status =
        portMgr->portMgrPortAddWithLanes(dev_tgt.dev_id,
                                         dev_port,
                                         (speedMap.at(strData.at(SPEED_ID))),
                                         u32Data.at(LANES_ID),
                                         (fecMap.at(strData.at(FEC_ID))));
  }
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in adding an port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return this->tableEntryMod_internal(session, dev_tgt, dev_port, data);
}

bf_status_t BfRtPortCfgTable::tableEntryMod(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  const uint32_t dev_port = port_key.getId();
  const BfRtPortCfgTableData &port_data =
      static_cast<const BfRtPortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  const std::unordered_map<bf_rt_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();
  const std::unordered_map<bf_rt_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  uint32_t lane_numb;
  bf_fec_type_t fec_type;
  bool st;

  /* Speed case is different, it needs a port flap.
   * Changing speed also changes AN settings to default, so save the old value
   * and then reapply it. */
  auto strDataIt = strData.find(SPEED_ID);

  if (strDataIt != strData.end()) {
    auto speedMapIt = speedMap.find(strDataIt->second);
    if (speedMapIt == speedMap.end()) {
      LOG_ERROR("%s:%d %s ERROR : invalid speed type %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strDataIt->second.c_str());
      return BF_INVALID_ARG;
    }
    uint32_t n_lanes = 0;
    if (u32Data.find(LANES_ID) != u32Data.end()) {
      n_lanes = u32Data.at(LANES_ID);
    }

    bf_status_t sts =
        portMgr->portMgrPortFecGet(dev_tgt.dev_id, dev_port, &fec_type);
    status |= sts;
    if (BF_SUCCESS != sts) {
      LOG_ERROR("%s:%d %s: Error in getting FEC of dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
    }

    if (u32Data.find(LANES_ID) == u32Data.end()) {
      status = portMgr->portMgrPortGetDefaultLaneNum(
          dev_tgt.dev_id, speedMapIt->second, &lane_numb);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in getting Number of lanes",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
      st = portMgr->portMgrPortSpeedValidate(
          dev_tgt.dev_id, dev_port, speedMapIt->second, lane_numb, fec_type);
      if (st != true) {
        LOG_ERROR("%s:%d %s: Invalid Channel",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
    } else {
      st = portMgr->portMgrPortSpeedValidate(
          dev_tgt.dev_id, dev_port, speedMapIt->second, n_lanes, fec_type);
      if (st != true) {
        LOG_ERROR("%s:%d %s: Invalid Channel",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return BF_INVALID_ARG;
      }
    }

    status = portMgr->portMgrPortEnable(dev_tgt.dev_id, dev_port, false);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                "disable");
    }

    bf_pm_port_autoneg_policy_e an_policy;

    status = portMgr->portMgrPortAnGet(dev_tgt.dev_id, dev_port, &an_policy);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s: Error in getting autoneg policy",
                __func__,
                __LINE__,
                table_name_get().c_str());
    }

    status = portMgr->portMgrPortDelWithLanes(
        dev_tgt.dev_id, dev_port, speedMapIt->second, n_lanes);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in deleting Port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
    }

    if (u32Data.find(LANES_ID) == u32Data.end()) {
      status = portMgr->portMgrPortSpeedSet(
          dev_tgt.dev_id, dev_port, speedMapIt->second);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting speed type %s",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  strDataIt->second.c_str());
      }
    } else {
      status = portMgr->portMgrPortSpeedWithLanesSet(
          dev_tgt.dev_id, dev_port, speedMapIt->second, u32Data.at(LANES_ID));
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in setting speed type %s",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  strDataIt->second.c_str());
      }
    }

    status =
        portMgr->portMgrPortAnPolicySet(dev_tgt.dev_id, dev_port, an_policy);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s: Error in getting autoneg policy",
                __func__,
                __LINE__,
                table_name_get().c_str());
    }

    status = portMgr->portMgrPortEnable(dev_tgt.dev_id, dev_port, true);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in setting port enable %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                "enable");
    }
  }

  if (strData.find(FEC_ID) != strData.end()) {
    // fec
    // check fec valid
    if (fecMap.find(strData.at(FEC_ID)) == fecMap.end()) {
      LOG_TRACE("%s:%d %s ERROR : invalid fec %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                strData.at(FEC_ID).c_str());
      return BF_INVALID_ARG;
    }
    status = portMgr->portMgrPortFECSet(dev_tgt.dev_id,
                                        static_cast<bf_dev_port_t>(dev_port),
                                        fecMap.at(strData.at(FEC_ID)));
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in setting FEC",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  }
  return this->tableEntryMod_internal(session, dev_tgt, dev_port, data);
}

bf_status_t BfRtPortCfgTable::tableEntryAddOrMod(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 const BfRtTableKey &key,
                                                 const BfRtTableData &data,
                                                 bool *is_added) const {
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  const bf_dev_id_t dev_id = dev_tgt.dev_id;
  const bf_dev_port_t dev_port = port_key.getId();
  bool added = false;
  bf_status_t status = portMgr->portMgrPortIsAddedGet(dev_id, dev_port, &added);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "Unable to check whether port is added for dev : %d : front port : "
        "(%d) : %s (%d)",
        dev_id,
        dev_port,
        bf_err_str(status),
        status);
    return status;
  }
  if (added) {
    status = tableEntryMod(session, dev_tgt, 0, key, data);
  } else {
    status = tableEntryAdd(session, dev_tgt, 0, key, data);
  }
  if (is_added) *is_added = !added;
  return status;
}

bf_status_t BfRtPortCfgTable::tableEntryDel(const BfRtSession & /*session*/,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key) const {
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  auto *portMgr = PortMgrIntf::getInstance();
  return portMgr->portMgrPortDel(dev_tgt.dev_id, dev_port);
}

bf_status_t BfRtPortCfgTable::tableClear(const BfRtSession & /*session*/,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t & /*flags*/) const {
  auto *portMgr = PortMgrIntf::getInstance();
  return portMgr->portMgrPortDelAll(dev_tgt.dev_id);
}

bf_status_t BfRtPortCfgTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             table_name_get().c_str());
  }
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  return tableEntryGet_internal(session, dev_tgt, dev_port, data);
}

bf_status_t BfRtPortCfgTable::tableEntryGetFirst(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 BfRtTableKey *key,
                                                 BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             table_name_get().c_str());
  }
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirstAdded(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (BF_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  BfRtPortCfgTableKey *port_key = static_cast<BfRtPortCfgTableKey *>(key);
  port_key->setId(dev_port);
  return tableEntryGet_internal(session, dev_tgt, dev_port, data);
}

bf_status_t BfRtPortCfgTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN("%s:%d %s WARNING : Read from hardware not supported",
             __func__,
             __LINE__,
             table_name_get().c_str());
  }
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port, dev_port_n;
  dev_port = port_key.getId();
  auto *portMgr = PortMgrIntf::getInstance();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtPortCfgTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    status = portMgr->portMgrPortGetNextAdded(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully counted all the ports
      // in the system. In that case, reset the status and break out
      // of the loop
      status = BF_SUCCESS;
      break;
    }
    this_key->setId(dev_port_n);
    status = tableEntryGet_internal(session, dev_tgt, dev_port_n, this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
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

bf_status_t BfRtPortCfgTable::tableUsageGet(const BfRtSession & /* session */,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /* flags */,
                                            uint32_t *count) const {
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port, dev_port_n;
  *count = 0;
  status = portMgr->portMgrPortGetFirstAdded(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  *count = 1;
  do {
    status = portMgr->portMgrPortGetNextAdded(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      *count = 0;
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully counted all the ports
      // in the system. In that case, reset the status and break out
      // of the loop
      status = BF_SUCCESS;
      break;
    }
    dev_port = dev_port_n;
    (*count)++;
  } while (1);
  return status;
}

bf_status_t BfRtPortCfgTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint32_t &dev_port,
    BfRtTableData *data) const {
  BfRtPortCfgTableData *port_data = static_cast<BfRtPortCfgTableData *>(data);
  const auto &activeDataFields = port_data->getActiveDataFields();
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;

  bool is_valid = false;
  bool is_recirc = false;
  status = portMgr->portMgrPortIsValidGet(dev_tgt.dev_id, dev_port);
  if (BF_SUCCESS != status) {
    const int MAX_RECIRC_PORTS = 30;
    uint32_t recirc_ports[MAX_RECIRC_PORTS];
    uint32_t max_ports =
        portMgr->portMgrRecircDevPortsGet(dev_tgt.dev_id, recirc_ports);
    if (max_ports == 0) {
      LOG_TRACE("%s:%d %s: Error in getting recirc port range",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
    for (uint32_t i = 0; i < max_ports && i < MAX_RECIRC_PORTS; i++) {
      if (recirc_ports[i] == dev_port) {
        is_recirc = true;
        is_valid = true;
        status = BF_SUCCESS;
        break;
      }
      if (i == (max_ports - 1)) {
        LOG_ERROR("%s:%d %s: dev_port %d is not valid",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
        return BF_OBJECT_NOT_FOUND;
      }
    }
  } else {
    is_valid = true;
  }
  // speed
  if (activeDataFields.empty() ||
      activeDataFields.find(SPEED_ID) != activeDataFields.end()) {
    bf_port_speed_t speed = BF_SPEED_NONE;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortSpeedGet(dev_tgt.dev_id, dev_port, &speed);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port speed for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    const auto speed_str = this->getStrFromSpeed(speed);
    if (speed_str == "UNKNOWN") {
      LOG_ERROR("%s:%d %s: Error in getting port speed string for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
    }
    port_data->setValue(SPEED_ID, speed_str);
  }
  // lanes
  if (activeDataFields.empty() ||
      activeDataFields.find(LANES_ID) != activeDataFields.end()) {
    int num_lanes = 1;
    if (is_recirc) {
      num_lanes = 1;
    } else {
      bf_status_t sts =
          portMgr->portMgrPortNumLanesGet(dev_tgt.dev_id, dev_port, &num_lanes);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting num lanes for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(LANES_ID, static_cast<uint64_t>(num_lanes));
  }
  // enable
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_EN_ID) != activeDataFields.end()) {
    bool is_enabled = false;
    if (is_recirc) {
      is_enabled = true;
    } else {
      bf_status_t sts =
          portMgr->portMgrPortIsEnabled(dev_tgt.dev_id, dev_port, &is_enabled);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting admin state for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(PORT_EN_ID, is_enabled);
  }
  // direction
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_DIR_ID) != activeDataFields.end()) {
    bf_pm_port_dir_e port_dir = PM_PORT_DIR_DEFAULT;
    if (is_recirc) {
      port_dir = PM_PORT_DIR_DEFAULT;
    } else {
      bf_status_t sts =
          portMgr->portMgrPortDirGet(dev_tgt.dev_id, dev_port, &port_dir);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting direction for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    const auto dir_str = this->getStrFromDirection(port_dir);
    if (dir_str == "UNKNOWN") {
      LOG_ERROR(
          "%s:%d %s: Error in getting port direction string for dev_port %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          dev_port);
    }
    port_data->setValue(PORT_DIR_ID, dir_str);
  }
  // SDS
  if (activeDataFields.empty() ||
      activeDataFields.find(SDS_TX_ATTN_ID) != activeDataFields.end()) {
    int tx_attn = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortSerdesTxEqAttnGet(
          dev_tgt.dev_id, dev_port, &tx_attn);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx attn for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(SDS_TX_ATTN_ID, static_cast<uint64_t>(tx_attn));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(SDS_TX_PRE_ID) != activeDataFields.end()) {
    int tx_pre = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortSerdesTxEqPreGet(
          dev_tgt.dev_id, dev_port, &tx_pre);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx pre for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(SDS_TX_PRE_ID, static_cast<int64_t>(tx_pre));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(SDS_TX_POST_ID) != activeDataFields.end()) {
    int tx_post = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortSerdesTxEqPostGet(
          dev_tgt.dev_id, dev_port, &tx_post);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx post for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(SDS_TX_POST_ID, static_cast<int64_t>(tx_post));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(SDS_TX_PRE2_ID) != activeDataFields.end()) {
    int tx_pre2 = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortSerdesTxEqPre2Get(
          dev_tgt.dev_id, dev_port, &tx_pre2);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx pre for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(SDS_TX_PRE2_ID, static_cast<int64_t>(tx_pre2));
  }
  if (activeDataFields.empty() ||
      activeDataFields.find(SDS_TX_POST2_ID) != activeDataFields.end()) {
    int tx_post2 = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortSerdesTxEqPost2Get(
          dev_tgt.dev_id, dev_port, &tx_post2);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting serdes tx post for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(SDS_TX_POST2_ID, static_cast<int64_t>(tx_post2));
  }
  // op state
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_UP_ID) != activeDataFields.end()) {
    bool state = false;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortOperStateGet(dev_tgt.dev_id, dev_port, &state);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port state of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    } else {
      state = false;
    }
    port_data->setValue(PORT_UP_ID, state);
  }
  // media type
  if (activeDataFields.empty() ||
      activeDataFields.find(MEDIA_TP_ID) != activeDataFields.end()) {
    std::string media_unknown = "BF_MEDIA_TYPE_UNKNOWN";
    if (is_recirc) {
      port_data->setValue(MEDIA_TP_ID, media_unknown);
    } else {
      bool is_internal = false;
      bf_status_t sts = portMgr->portMgrPortIsInternalGet(
          dev_tgt.dev_id, static_cast<bf_dev_port_t>(dev_port), &is_internal);
      status |= sts;
      if (sts != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s: Error in getting is_internal for port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      } else {
        if (is_internal) {
          port_data->setValue(MEDIA_TP_ID, media_unknown);
          LOG_DBG(
              "%s:%d %s: Port %d is is_internal. Setting media type unknown",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_port);
        } else {
          bf_media_type_t media_type;
          sts = portMgr->portMgrPortMediaTypeGet(
              dev_tgt.dev_id,
              static_cast<bf_dev_port_t>(dev_port),
              &media_type);
          status |= sts;
          if (BF_SUCCESS != sts) {
            LOG_ERROR("%s:%d %s: Error in getting media type of dev_port %d",
                      __func__,
                      __LINE__,
                      table_name_get().c_str(),
                      dev_port);
          } else {
            // find the string of media_type
            auto it = mediaTypeMap.begin();
            for (; it != mediaTypeMap.end(); ++it) {
              if (it->second == media_type) {
                port_data->setValue(MEDIA_TP_ID, it->first);
                break;
              }
            }
            if (it == mediaTypeMap.end()) {
              LOG_ERROR(
                  "%s:%d %s: Error in converting the read media type enum into "
                  "the "
                  "corresponding string for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
            }
          }  // if sts == BF_SUCCESS
        }    // if is_internal
      }      // if sts == BF_SUCCESS
    }
  }  // if activeDataFields(MEDIA_TP_ID)
  // cut through
  if (activeDataFields.empty() ||
      activeDataFields.find(CT_EN_ID) != activeDataFields.end()) {
    bool en = false;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortCutThroughGet(dev_tgt.dev_id, dev_port, &en);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting cut through of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    port_data->setValue(CT_EN_ID, en);
  }

  // PTP ingress timestamp delta
  if (activeDataFields.empty() ||
      activeDataFields.find(TIMESTAMP_1588_DELTA_RX) !=
          activeDataFields.end()) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (getDataField(TIMESTAMP_1588_DELTA_RX, &tableDataField) == BF_SUCCESS) {
      uint16_t delta = 0;
      if (!is_recirc) {
        bf_status_t sts =
            portMgr->portMgrPtpRxDeltaGet(dev_tgt.dev_id, dev_port, &delta);
        status |= sts;
        if (BF_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Ingress Timestamp delta for "
              "dev_port %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_port);
        }
      }
      port_data->setValue(TIMESTAMP_1588_DELTA_RX,
                          static_cast<uint64_t>(delta));
    }
  }

  // PTP Egress timestamp delta
  if (activeDataFields.empty() ||
      activeDataFields.find(TIMESTAMP_1588_DELTA_TX) !=
          activeDataFields.end()) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (getDataField(TIMESTAMP_1588_DELTA_TX, &tableDataField) == BF_SUCCESS) {
      uint16_t delta = 0;
      if (!is_recirc) {
        bf_status_t sts =
            portMgr->portMgrPtpTxDeltaGet(dev_tgt.dev_id, dev_port, &delta);
        status |= sts;
        if (BF_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Egress Timestamp delta for "
              "dev_port %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_port);
        }
      }
      port_data->setValue(TIMESTAMP_1588_DELTA_TX,
                          static_cast<uint64_t>(delta));
    }
  }

  // PTP timestamp value , id , validity
  if (activeDataFields.empty() ||
      ((activeDataFields.find(TIMESTAMP_1588_VALID) !=
        activeDataFields.end()) &&
       (activeDataFields.find(TIMESTAMP_1588_VALUE) !=
        activeDataFields.end()) &&
       (activeDataFields.find(TIMESTAMP_1588_ID) != activeDataFields.end()))) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (getDataField(TIMESTAMP_1588_ID, &tableDataField) == BF_SUCCESS) {
      uint64_t ts = 0;
      bool ts_valid = false;
      int ts_id = 0;
      if (!is_recirc) {
        bf_status_t sts = portMgr->portMgrPtpTxTimestampGet(
            dev_tgt.dev_id, dev_port, &ts, &ts_valid, &ts_id);
        status |= sts;
        if (BF_SUCCESS != sts) {
          LOG_ERROR(
              "%s:%d %s: Error in getting ptp Timestamp get for dev_port %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_port);
        }
      }
      port_data->setValue(TIMESTAMP_1588_VALUE, ts);
      port_data->setValue(TIMESTAMP_1588_VALID, ts_valid);
      port_data->setValue(TIMESTAMP_1588_ID, static_cast<uint64_t>(ts_id));
    }
  } else if ((activeDataFields.find(TIMESTAMP_1588_VALUE) !=
              activeDataFields.end()) ||
             (activeDataFields.find(TIMESTAMP_1588_VALID) !=
              activeDataFields.end()) ||
             (activeDataFields.find(TIMESTAMP_1588_ID) !=
              activeDataFields.end())) {
    LOG_ERROR(
        "%s:%d %s ERROR : 1588 PTP timestamp get request require "
        "ts_id , ts_value , ts_valid",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  // front port
  if (activeDataFields.empty() ||
      activeDataFields.find(CONN_ID) != activeDataFields.end() ||
      activeDataFields.find(CHNL_ID) != activeDataFields.end()) {
    int conn_id = 0;
    int chnl_id = 0;
    if (!is_recirc) {
      bf_pal_front_port_handle_t port_hdl;
      bf_status_t sts =
          portMgr->portMgrPortFrontPortGet(dev_tgt.dev_id, dev_port, &port_hdl);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting front port of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
      if (activeDataFields.empty() ||
          activeDataFields.find(CONN_ID) != activeDataFields.end())
        port_data->setValue(CONN_ID, static_cast<uint64_t>(port_hdl.conn_id));
      if (activeDataFields.empty() ||
          activeDataFields.find(CHNL_ID) != activeDataFields.end())
        port_data->setValue(CHNL_ID, static_cast<uint64_t>(port_hdl.chnl_id));
    } else {
      port_data->setValue(CONN_ID, static_cast<uint64_t>(conn_id));
      port_data->setValue(CHNL_ID, static_cast<uint64_t>(chnl_id));
    }
  }
  // is internal
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_INTERNAL_ID) != activeDataFields.end()) {
    bool is_internal;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortIsInternalGet(
          dev_tgt.dev_id, dev_port, &is_internal);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting is_internal of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    } else {
      is_internal = false;
    }
    port_data->setValue(PORT_INTERNAL_ID, is_internal);
  }
  // is valid
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_VALID_ID) != activeDataFields.end()) {
    port_data->setValue(PORT_VALID_ID, is_valid);
  }
  // port str
  if (activeDataFields.empty() ||
      activeDataFields.find(PORT_NAME_ID) != activeDataFields.end()) {
    char name[MAX_PORT_HDL_STRING_LEN];
    std::string recirc_str = "recirc";
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortStrGet(dev_tgt.dev_id, dev_port, name);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting port str of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
      std::string name_str(name);
      port_data->setValue(PORT_NAME_ID, name_str);
    } else {
      port_data->setValue(PORT_NAME_ID, recirc_str);
    }
  }
  // AN
  if (activeDataFields.empty() ||
      activeDataFields.find(AN_ID) != activeDataFields.end()) {
    bf_pm_port_autoneg_policy_e an_policy = PM_AN_DEFAULT;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortAnGet(dev_tgt.dev_id, dev_port, &an_policy);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting AN policy of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    }
    const auto an_str = this->getStrFromAN(an_policy);
    if (an_str == "UNKNOWN") {
      LOG_ERROR(
          "c5ce7cf4 : %s:%d %s: Error in getting port direction string for "
          "dev_port %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          dev_port);
    }
    port_data->setValue(AN_ID, an_str);
  }
  // fec
  if (activeDataFields.empty() ||
      activeDataFields.find(FEC_ID) != activeDataFields.end()) {
    bf_fec_type_t fec_type;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortFecGet(dev_tgt.dev_id, dev_port, &fec_type);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting FEC of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    } else {
      fec_type = BF_FEC_TYP_NONE;
    }
    for (auto it = fecMap.begin(); it != fecMap.end(); ++it) {
      if (it->second == fec_type) {
        port_data->setValue(FEC_ID, it->first);
        break;
      }
    }
  }
  // lp mood
  if (activeDataFields.empty() ||
      activeDataFields.find(LP_MOOD_ID) != activeDataFields.end()) {
    bf_loopback_mode_e mode;
    if (!is_recirc) {
      bf_status_t sts =
          portMgr->portMgrPortLoopbackModeGet(dev_tgt.dev_id, dev_port, &mode);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting loopback mode of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    } else {
      mode = BF_LPBK_NONE;
    }
    for (auto it = loopbackModeMap.begin(); it != loopbackModeMap.end(); ++it) {
      if (it->second == mode) {
        port_data->setValue(LP_MOOD_ID, it->first);
        break;
      }
    }
  }
  // rx/tx mtu
  if (activeDataFields.empty() ||
      activeDataFields.find(TX_MTU_ID) != activeDataFields.end() ||
      activeDataFields.find(RX_MTU_ID) != activeDataFields.end()) {
    uint32_t tx_mtu = 10240, rx_mtu = 10240;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortMtuGet(
          dev_tgt.dev_id, dev_port, &tx_mtu, &rx_mtu);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s:%d %s: Error in getting rx/tx mtu of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
      }
    } else {
      tx_mtu = 0;
      rx_mtu = 0;
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(TX_MTU_ID) != activeDataFields.end())
      port_data->setValue(TX_MTU_ID, static_cast<uint64_t>(tx_mtu));
    if (activeDataFields.empty() ||
        activeDataFields.find(RX_MTU_ID) != activeDataFields.end())
      port_data->setValue(RX_MTU_ID, static_cast<uint64_t>(rx_mtu));
  }
  // PLL Overclock
  if (activeDataFields.empty() ||
      activeDataFields.find(PLL_OVRCLK_ID) != activeDataFields.end()) {
    float pll_ovrclk = 0.0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortPllOvrclkGet(
          dev_tgt.dev_id, dev_port, &pll_ovrclk);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error in getting pll overclocking config of dev_port %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_port);
      }
    }
    port_data->setValue(PLL_OVRCLK_ID, pll_ovrclk);
  }
  // rx/tx pfc
  if (activeDataFields.empty() ||
      activeDataFields.find(TX_PFC_EN_MAP_ID) != activeDataFields.end() ||
      activeDataFields.find(RX_PFC_EN_MAP_ID) != activeDataFields.end()) {
    uint32_t tx_en_map = 0, rx_en_map = 0;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortFlowControlPfcGet(
          dev_tgt.dev_id, dev_port, &tx_en_map, &rx_en_map);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error in getting rx/tx flow control PFC of dev_port %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_port);
      }
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(TX_PFC_EN_MAP_ID) != activeDataFields.end())
      port_data->setValue(TX_PFC_EN_MAP_ID, static_cast<uint64_t>(tx_en_map));
    if (activeDataFields.empty() ||
        activeDataFields.find(RX_PFC_EN_MAP_ID) != activeDataFields.end())
      port_data->setValue(RX_PFC_EN_MAP_ID, static_cast<uint64_t>(rx_en_map));
  }
  // rx/tx pause
  if (activeDataFields.empty() ||
      activeDataFields.find(TX_PAUSE_EN_ID) != activeDataFields.end() ||
      activeDataFields.find(RX_PAUSE_EN_ID) != activeDataFields.end()) {
    bool tx_en = false, rx_en = false;
    if (!is_recirc) {
      bf_status_t sts = portMgr->portMgrPortFlowContrlPauseGet(
          dev_tgt.dev_id, dev_port, &tx_en, &rx_en);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error in getting rx/tx flow control Pause of dev_port "
            "%d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_port);
      }
    }
    if (activeDataFields.empty() ||
        activeDataFields.find(TX_PAUSE_EN_ID) != activeDataFields.end())
      port_data->setValue(TX_PAUSE_EN_ID, tx_en);
    if (activeDataFields.empty() ||
        activeDataFields.find(RX_PAUSE_EN_ID) != activeDataFields.end())
      port_data->setValue(RX_PAUSE_EN_ID, rx_en);
  }
  // rx parser priority threshold
  if (activeDataFields.empty() ||
      activeDataFields.find(RX_PRSR_PRI_THRESH) != activeDataFields.end()) {
    auto *pipeMgr = PipeMgrIntf::getInstance(session);
    uint32_t threshold = 0;
    if (!is_recirc) {
      bf_status_t sts = pipeMgr->pipeMgrPortRXPrsrPriThreshGet(
          dev_tgt.dev_id, dev_port, &threshold);
      status |= sts;
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "%s:%d %s: Error getting ingress parser priority threshold of "
            "dev_port %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_port);
      }
    }
    port_data->setValue(RX_PRSR_PRI_THRESH, static_cast<uint64_t>(threshold));
  }
  return status;
}

bf_status_t BfRtPortCfgTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortCfgTableKey *>(key))->reset();
}

bf_status_t BfRtPortCfgTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return this->dataReset(emptyFields, data);
}

bf_status_t BfRtPortCfgTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                        BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortCfgTableData *>(data))->reset(fields);
}

bf_status_t BfRtPortCfgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortCfgTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::PORT_STATUS_NOTIF:
      break;
    default:
      LOG_TRACE("%s:%d Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtPortCfgTable::tableAttributesSet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::PORT_STATUS_NOTIF: {
      void *cookie;
      bool enable;
      BfRtPortStatusNotifCb callback;
      bf_rt_port_status_chg_cb callback_c;
      tbl_attr_impl->portStatusChangeNotifGet(
          &enable, &callback, &callback_c, &cookie);
      // Get the state and set enabled/cb/cookie
      auto device_state =
          BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
      if (device_state == nullptr) {
        return BF_OBJECT_NOT_FOUND;
      }
      // Update the state
      auto attributes_state =
          device_state->attributePortState.getObjState(table_id_get());
      attributes_state->stateTableAttributesPortSet(
          enable, callback, callback_c, this, cookie);

      port_status_chg_cb callback_fn =
          (enable == false) ? nullptr : bfRtPortStatusChgInternalCb;
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
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

// Port Stat Table
bf_status_t BfRtPortStatTable::tableClear(const BfRtSession & /*session*/,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /* flags */) const {
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirstAdded(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (BF_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    LOG_DBG("%s:%d %s: No ports in the system to clear stats for",
            __func__,
            __LINE__,
            table_name_get().c_str());
    return status;
  }
  status = portMgr->portMgrPortAllStatsClear(dev_tgt.dev_id, dev_port);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s: Error in clearing all stats for dev_port %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_port);
    return status;
  }
  bf_dev_port_t dev_port_n;
  while (status == BF_SUCCESS) {
    status = portMgr->portMgrPortGetNextAdded(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_TRACE("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      return status;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // We have cleared the stats for all the ports
      return BF_SUCCESS;
    }
    status = portMgr->portMgrPortAllStatsClear(dev_tgt.dev_id, dev_port_n);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d %s: Error in clearing all stats for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port_n);
      return status;
    }
    dev_port = dev_port_n;
  }
  return status;
}

bf_status_t BfRtPortStatTable::tableEntryMod(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /* flags */,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  const BfRtPortStatTableKey &port_key =
      static_cast<const BfRtPortStatTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  const BfRtPortStatTableData &port_data =
      static_cast<const BfRtPortStatTableData &>(data);
  const std::vector<bf_rt_id_t> activeDataFields =
      port_data.getActiveDataFields();
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  const uint64_t *u64Data = port_data.getU64FieldData();
  for (auto n = activeDataFields.begin(); n != activeDataFields.end(); ++n) {
    if (u64Data[(*n) - 1] != 0) {
      LOG_TRACE("%s:%d %s: Error data cannot be non-zero value",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_INVALID_ARG;
    }
  }
  if (activeDataFields.empty()) {
    // clear all
    status = portMgr->portMgrPortAllStatsClear(dev_tgt.dev_id, dev_port);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error clear all stat for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      return status;
    }
  } else {
    for (const auto &id : (activeDataFields)) {
      status = portMgr->portMgrPortThisStatClear(
          dev_tgt.dev_id, dev_port, static_cast<bf_rmon_counter_t>(id - 1));
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s: Error clear stat %d for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  (id - 1),
                  dev_port);
        continue;
      }
    }
  }
  return status;
}

bf_status_t BfRtPortStatTable::tableEntryGet(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             BfRtTableData *data) const {
  const BfRtPortStatTableKey &port_key =
      static_cast<const BfRtPortStatTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  BfRtPortStatTableData *port_data = static_cast<BfRtPortStatTableData *>(data);
  return this->tableEntryGet_internal(dev_tgt, flags, dev_port, port_data);
}

bf_status_t BfRtPortStatTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const uint32_t &dev_port,
    BfRtPortStatTableData *port_data) const {
  const std::vector<bf_rt_id_t> activeDataFields =
      port_data->getActiveDataFields();
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  if (!BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    // read from shadow
    if (activeDataFields.empty() ||
        (activeDataFields.size() >
         port_data->getAllStatsBoundry())) {  // getAllStatsBoundry is the
                                              // boundry of using get_all to
                                              // increase efficiency
      // all fields
      uint64_t stats[BF_NUM_RMON_COUNTERS];
      status = portMgr->portMgrPortAllStatsGet(dev_tgt.dev_id, dev_port, stats);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s: Error getting all stat for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
        return status;
      }
      port_data->setAllValues(stats);
    } else {
      uint64_t val;
      for (const auto &id : (activeDataFields)) {
        status = portMgr->portMgrPortThisStatGet(
            dev_tgt.dev_id,
            dev_port,
            static_cast<bf_rmon_counter_t>(id - 1),
            &val);
        if (status != BF_SUCCESS) {
          LOG_ERROR("%s:%d %s: Error getting stat for dev_port %d, ctr_type %d",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
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
      for (uint32_t i = 0; i <= num_of_ctr; i++) {
        ctr_type_array[i] = static_cast<bf_rmon_counter_t>(
            static_cast<uint32_t>(activeDataFields.at(i)) - 1);
      }
    }
    status = portMgr->portMgrPortStatDirectGet(
        dev_tgt.dev_id, dev_port, ctr_type_array, stat_val, num_of_ctr);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error getting hw stat for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      return status;
    }
    for (uint32_t i = 0; i <= num_of_ctr; i++) {
      // We need to add 1 to the field ids, as they start from 1 in bfrt json
      port_data->setValue(ctr_type_array[i] + 1, stat_val[i]);
    }
  }
  bf_pkt_rate_t pkt_rate;
  status =
      portMgr->portMgrPortPacketRateGet(dev_tgt.dev_id, dev_port, &pkt_rate);
  port_data->setValue(TX_PPS, pkt_rate.tx_pps);
  port_data->setValue(RX_PPS, pkt_rate.rx_pps);
  port_data->setValue(TX_RATE, pkt_rate.tx_rate);
  port_data->setValue(RX_RATE, pkt_rate.rx_rate);

  return status;
}

bf_status_t BfRtPortStatTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirstAdded(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (BF_OBJECT_NOT_FOUND == status) {
    // This means that we don't have any ports in the system
    return status;
  }
  BfRtPortStatTableKey *port_key = static_cast<BfRtPortStatTableKey *>(key);
  port_key->setId(dev_port);
  auto port_data = static_cast<BfRtPortStatTableData *>(data);
  return this->tableEntryGet_internal(dev_tgt, flags, dev_port, port_data);
}

bf_status_t BfRtPortStatTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtPortStatTableKey &port_key =
      static_cast<const BfRtPortStatTableKey &>(key);
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port, dev_port_n;
  dev_port = port_key.getId();
  auto *portMgr = PortMgrIntf::getInstance();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtPortStatTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPortStatTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNextAdded(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = BF_SUCCESS;
      break;
    }
    this_key->setId(dev_port_n);
    status =
        this->tableEntryGet_internal(dev_tgt, flags, dev_port_n, this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
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

bf_status_t BfRtPortStatTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortStatTableKey *>(key))->reset();
}

bf_status_t BfRtPortStatTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                         BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortStatTableData *>(data))->reset(fields);
}

bf_status_t BfRtPortStatTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return (static_cast<BfRtPortStatTableData *>(data))->reset(emptyFields);
}

bf_status_t BfRtPortStatTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortStatTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }

  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::PORT_STAT_POLL_INTVL_MS:
      break;
    default:
      LOG_TRACE("%s:%d Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtPortStatTable::tableAttributesSet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::PORT_STAT_POLL_INTVL_MS: {
      uint32_t intvl;
      bf_status_t sts = tbl_attr_impl->portStatPollIntvlMsGet(&intvl);
      if (sts != BF_SUCCESS) {
        return sts;
      }
      auto *portMgr = PortMgrIntf::getInstance();
      return portMgr->portMgrPortStatsPollIntvlSet(dev_tgt.dev_id, intvl);
    }
    default:
      LOG_TRACE(
          "%s:%d Invalid Attribute type (%d) encountered while trying to set "
          "attributes",
          __func__,
          __LINE__,
          static_cast<int>(attr_type));
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::tableAttributesGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::PORT_STAT_POLL_INTVL_MS: {
      auto *portMgr = PortMgrIntf::getInstance();
      uint32_t intvl;
      auto sts = portMgr->portMgrPortStatsPollIntvlGet(dev_tgt.dev_id, &intvl);
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get Port Stats Poll Intvl",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
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
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

// Port Hdl Info Table
bf_status_t BfRtPortHdlInfoTable::tableEntryGet(const BfRtSession & /*session*/,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortHdlInfoTableKey &port_key =
      static_cast<const BfRtPortHdlInfoTableKey &>(key);
  bf_pal_front_port_handle_t port_hdl;
  uint32_t dev_port;
  port_key.getPortHdl(&port_hdl.conn_id, &port_hdl.chnl_id);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = portMgr->portMgrFrontPortToDevPortGet(
      dev_tgt.dev_id, &port_hdl, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for port with port_hdl %d/%d err",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        port_hdl.conn_id,
        port_hdl.chnl_id);
    return status;
  }
  BfRtPortHdlInfoTableData *port_data =
      static_cast<BfRtPortHdlInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirst(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  bf_pal_front_port_handle_t port_hdl;
  status =
      portMgr->portMgrPortFrontPortGet(dev_tgt.dev_id, dev_port, &port_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting front panel port for dev_port %d with err : "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }

  BfRtPortHdlInfoTableKey *port_key =
      static_cast<BfRtPortHdlInfoTableKey *>(key);
  port_key->setPortHdl(port_hdl.conn_id, port_hdl.chnl_id);
  BfRtPortHdlInfoTableData *port_data =
      static_cast<BfRtPortHdlInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortHdlInfoTableKey &port_key =
      static_cast<const BfRtPortHdlInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port, dev_port_n;
  bf_pal_front_port_handle_t port_hdl = {0};
  port_key.getPortHdl(&port_hdl.conn_id, &port_hdl.chnl_id);
  status = portMgr->portMgrFrontPortToDevPortGet(
      dev_tgt.dev_id, &port_hdl, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for port with port_hdl %d/%d err",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        port_hdl.conn_id,
        port_hdl.chnl_id);
    return status;
  }
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtPortHdlInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPortHdlInfoTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNext(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = BF_SUCCESS;
      break;
    }
    status =
        portMgr->portMgrPortFrontPortGet(dev_tgt.dev_id, dev_port_n, &port_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error getting front panel port for dev_port %d with err : "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
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

bf_status_t BfRtPortHdlInfoTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortHdlInfoTableKey *>(key))->reset();
}

bf_status_t BfRtPortHdlInfoTable::dataReset(BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortHdlInfoTableData *>(data))->reset();
}

bf_status_t BfRtPortHdlInfoTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortHdlInfoTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortHdlInfoTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Port Fp Idx Info Table
bf_status_t BfRtPortFpIdxInfoTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortFpIdxInfoTableKey *>(key))->reset();
}

bf_status_t BfRtPortFpIdxInfoTable::dataReset(BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortFpIdxInfoTableData *>(data))->reset();
}

bf_status_t BfRtPortFpIdxInfoTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortFpIdxInfoTableKey &port_key =
      static_cast<const BfRtPortFpIdxInfoTableKey &>(key);
  uint32_t fp_idx = port_key.getId();
  uint32_t dev_port;
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = portMgr->portMgrFpIdxToDevPortGet(
      dev_tgt.dev_id, fp_idx, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting Port with fp_idx 0x%x dev_port err",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              fp_idx);
    return status;
  }
  BfRtPortFpIdxInfoTableData *port_data =
      static_cast<BfRtPortFpIdxInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t fp_idx;
  status = portMgr->portMgrFpIdxGetFirst(dev_tgt.dev_id, &fp_idx);
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first front port index",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  bf_dev_port_t dev_port;
  status = portMgr->portMgrFpIdxToDevPortGet(dev_tgt.dev_id, fp_idx, &dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting dev_port for front port index %d with err : "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        fp_idx,
        status);
    return status;
  }
  BfRtPortFpIdxInfoTableKey *port_key =
      static_cast<BfRtPortFpIdxInfoTableKey *>(key);
  port_key->setId(fp_idx);
  BfRtPortFpIdxInfoTableData *port_data =
      static_cast<BfRtPortFpIdxInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortFpIdxInfoTableKey &port_key =
      static_cast<const BfRtPortFpIdxInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t dev_port;
  uint32_t fp_idx, fp_idx_n;
  fp_idx = port_key.getId();
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtPortFpIdxInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPortFpIdxInfoTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrFpIdxGetNext(dev_tgt.dev_id, fp_idx, &fp_idx_n);
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next fp idx after %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                fp_idx);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = BF_SUCCESS;
      break;
    }
    status =
        portMgr->portMgrFpIdxToDevPortGet(dev_tgt.dev_id, fp_idx_n, &dev_port);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error getting dev port for fp idx %d with err : %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
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

bf_status_t BfRtPortFpIdxInfoTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortFpIdxInfoTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortFpIdxInfoTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Port Str Info Table
bf_status_t BfRtPortStrInfoTable::tableEntryGet(const BfRtSession & /*session*/,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortStrInfoTableKey &port_key =
      static_cast<const BfRtPortStrInfoTableKey &>(key);
  std::string port_str = port_key.getPortStr();
  uint32_t dev_port;
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = portMgr->portMgrPortStrToDevPortGet(
      dev_tgt.dev_id,
      const_cast<char *>(port_str.c_str()),
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting Port %s dev_port err",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              port_str.c_str());
    return status;
  }
  BfRtPortStrInfoTableData *port_data =
      static_cast<BfRtPortStrInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortStrInfoTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port;
  status = portMgr->portMgrPortGetFirst(
      dev_tgt.dev_id, reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
    LOG_TRACE("%s:%d %s: Error in getting first dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  char port_str[MAX_PORT_HDL_STRING_LEN] = {'\0'};
  status = portMgr->portMgrPortStrGet(dev_tgt.dev_id, dev_port, port_str);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error getting front panel port string for dev_port %d with "
        "err : %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }

  BfRtPortStrInfoTableKey *port_key =
      static_cast<BfRtPortStrInfoTableKey *>(key);
  port_key->setPortStr(std::string(port_str));
  BfRtPortStrInfoTableData *port_data =
      static_cast<BfRtPortStrInfoTableData *>(data);
  port_data->setDevPort(dev_port);
  return BF_SUCCESS;
}

bf_status_t BfRtPortStrInfoTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  const BfRtPortStrInfoTableKey &port_key =
      static_cast<const BfRtPortStrInfoTableKey &>(key);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t dev_port, dev_port_n;
  std::string port_str = port_key.getPortStr();
  status = portMgr->portMgrPortStrToDevPortGet(
      dev_tgt.dev_id,
      const_cast<char *>(port_str.data()),
      reinterpret_cast<bf_dev_port_t *>(&dev_port));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error getting dev_port for port with name %s: err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              port_str.c_str(),
              status);
    return status;
  }
  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtPortStrInfoTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPortStrInfoTableData *>((*key_data_pairs)[i].second);
    status = portMgr->portMgrPortGetNext(
        dev_tgt.dev_id,
        dev_port,
        reinterpret_cast<bf_dev_port_t *>(&dev_port_n));
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ports in the system
      status = BF_SUCCESS;
      break;
    }
    char port_name[MAX_PORT_HDL_STRING_LEN] = {'\0'};
    status = portMgr->portMgrPortStrGet(dev_tgt.dev_id, dev_port_n, port_name);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error getting front panel port name for dev_port %d with "
          "err : %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
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

bf_status_t BfRtPortStrInfoTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortStrInfoTableKey *>(key))->reset();
}

bf_status_t BfRtPortStrInfoTable::dataReset(BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortStrInfoTableData *>(data))->reset();
}

bf_status_t BfRtPortStrInfoTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortStrInfoTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStrInfoTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortStrInfoTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}
}  // namespace bfrt
