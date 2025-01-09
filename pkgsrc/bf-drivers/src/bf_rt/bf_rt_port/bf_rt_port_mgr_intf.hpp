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


#ifndef _BF_RT_PORT_MGR_INTERFACE_HPP
#define _BF_RT_PORT_MGR_INTERFACE_HPP

extern "C" {
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
}

#include <map>
#include <iostream>
#include <mutex>
#include <memory>

namespace bfrt {

class IPortMgrIntf {
 public:
  virtual ~IPortMgrIntf() = default;
  // Library init API

  virtual bf_status_t portMgrPortStatusNotifReg(port_status_chg_cb cb_fn,
                                                void *cookie) = 0;
  virtual bf_status_t portMgrPortGetFirst(bf_dev_id_t dev_id,
                                          bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t portMgrPortGetFirstAdded(bf_dev_id_t dev_id,
                                               bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t portMgrPortGetNext(bf_dev_id_t dev_id,
                                         bf_dev_port_t curr_dev_port,
                                         bf_dev_port_t *next_dev_port) = 0;
  virtual bf_status_t portMgrPortGetNextAdded(bf_dev_id_t dev_id,
                                              bf_dev_port_t curr_dev_port,
                                              bf_dev_port_t *next_dev_port) = 0;
  virtual bf_status_t portMgrPortGetDefaultLaneNum(bf_dev_id_t dev_id,
                                                   bf_port_speed_t speed,
                                                   uint32_t *lane_numb) = 0;
  virtual bool portMgrPortSpeedValidate(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_port_speed_t speed,
                                        uint32_t n_lanes,
                                        bf_fec_type_t fec) = 0;
  virtual bf_status_t portMgrPortAdd(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     bf_port_speed_t speed,
                                     bf_fec_type_t fec_type) = 0;
  virtual bf_status_t portMgrPortAddWithLanes(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bf_port_speed_t speed,
                                              uint32_t n_lanes,
                                              bf_fec_type_t fec_type) = 0;
  virtual bf_status_t portMgrPortSetLoopbackMode(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_loopback_mode_e mode) = 0;
  virtual bf_status_t portMgrPortAnPolicySet(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int an_policy) = 0;
  virtual bf_status_t portMgrPortMtuSet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t tx_mtu,
                                        uint32_t rx_mtu) = 0;
  virtual bf_status_t portMgrPortPllOvrclkSet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              float pll_ovrclk) = 0;
  virtual bf_status_t portMgrPortFlowControlPfcSet(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t tx_en_map,
                                                   uint32_t rx_en_map) = 0;
  virtual bf_status_t portMgrPortFlowControlPauseFrameSet(
      bf_dev_id_t dev_id, bf_dev_port_t dev_port, bool tx_en, bool rx_en) = 0;
  virtual bf_status_t portMgrPortFECSet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_fec_type_t fec_type) = 0;
  virtual bf_status_t portMgrPortCutThroughSet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool ct_enable) = 0;
  virtual bf_status_t portMgrPortDirSet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_pm_port_dir_e port_dir) = 0;
  virtual bf_status_t portMgrPortDirGet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_pm_port_dir_e *port_dir) = 0;
  virtual bf_status_t portMgrPortMediaTypeSet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bf_media_type_t media_type) = 0;
  virtual bf_status_t portMgrPortSerdesParamsSet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      bf_pal_serdes_params_t *serdes_param) = 0;
  virtual bf_status_t portMgrPortSpeedSet(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          bf_port_speed_t speed) = 0;
  virtual bf_status_t portMgrPortSpeedWithLanesSet(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   bf_port_speed_t speed,
                                                   uint32_t n_lanes) = 0;
  virtual bf_status_t portMgrPortEnable(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bool port_enable) = 0;
  virtual bf_status_t portMgrPortOperStateGet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool *state) = 0;
  virtual bf_status_t portMgrPortMediaTypeGet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bf_media_type_t *media_type) = 0;
  virtual bf_status_t portMgrPortCutThroughGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *ct_enabled) = 0;
  virtual bf_status_t portMgrPortFrontPortGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      bf_pal_front_port_handle_t *port_hdl) = 0;
  virtual bf_status_t portMgrPortIsInternalGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *is_internal) = 0;
  virtual bf_status_t portMgrPortIsValidGet(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port) = 0;
  virtual bf_status_t portMgrPortStrGet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        char *port_str) = 0;
  virtual bf_status_t portMgrPortStatsPollIntvlGet(bf_dev_id_t dev_id,
                                                   uint32_t *poll_intvl_ms) = 0;

  virtual bf_status_t portMgrPortLoopbackModeGet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_loopback_mode_e *mode) = 0;

  virtual bf_status_t portMgrPortAnGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      bf_pm_port_autoneg_policy_e *an_policy) = 0;

  virtual bf_status_t portMgrPortMtuGet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t *tx_mtu,
                                        uint32_t *rx_mtu) = 0;

  virtual bf_status_t portMgrPortPllOvrclkGet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              float *pll_ovrclk) = 0;

  virtual bf_status_t portMgrPortFlowControlPfcGet(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t *tx_en_map,
                                                   uint32_t *rx_en_map) = 0;

  virtual bf_status_t portMgrPortFlowContrlPauseGet(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    bool *tx_en,
                                                    bool *rx_en) = 0;

  virtual bf_status_t portMgrPortFecGet(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_fec_type_t *fec_type) = 0;
  //
  virtual bf_status_t portMgrPortDel(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port) = 0;
  virtual bf_status_t portMgrPortDelWithLanes(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bf_port_speed_t speed,
                                              uint32_t n_lanes) = 0;
  virtual bf_status_t portMgrPortDelAll(bf_dev_id_t dev_id) = 0;

  virtual bf_status_t portMgrPortThisStatGet(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             bf_rmon_counter_t ctr_type,
                                             uint64_t *stat_val) = 0;
  virtual bf_status_t portMgrPortPacketRateGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bf_pkt_rate_t *pkt_rate) = 0;
  virtual bf_status_t portMgrPortThisStatClear(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bf_rmon_counter_t ctr_type) = 0;
  virtual bf_status_t portMgrPortAllStatsGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      uint64_t stats[BF_NUM_RMON_COUNTERS]) = 0;
  virtual bf_status_t portMgrPortThisStatGetWithTimestamp(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      bf_rmon_counter_t ctr_type,
      uint64_t *stat_val,
      int64_t *timestamp_s,
      int64_t *timestamp_ns) = 0;
  virtual bf_status_t portMgrAllPureStatsGetWithTimestamp(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      uint64_t stats[BF_NUM_RMON_COUNTERS],
      int64_t *timestamp_s,
      int64_t *timestamp_ns) = 0;
  virtual bf_status_t portMgrPortAllStatsClear(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port) = 0;
  virtual bf_status_t portMgrPortStatsPollIntvlSet(bf_dev_id_t dev_id,
                                                   uint32_t poll_intvl_ms) = 0;
  virtual bf_status_t portMgrPortStatDirectGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      bf_rmon_counter_t *ctr_type_array,
      uint64_t *stat_val,
      uint32_t num_of_ctr) = 0;

  //
  virtual bf_status_t portMgrFrontPortToDevPortGet(
      bf_dev_id_t dev_id,
      bf_pal_front_port_handle_t *port_hdl,
      bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t portMgrFpIdxToDevPortGet(bf_dev_id_t dev_id,
                                               uint32_t fp_idx,
                                               bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t portMgrPortStrToDevPortGet(bf_dev_id_t dev_id,
                                                 char *port_str,
                                                 bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t portMgrPortSpeedGet(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          bf_port_speed_t *speed) = 0;
  virtual bf_status_t portMgrPortNumLanesGet(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int *num_lanes) = 0;
  virtual bf_status_t portMgrPortIsEnabled(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bool *is_enabled) = 0;
  virtual bf_status_t portMgrPortSerdesTxEqPreGet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  int *tx_pre) = 0;
  virtual bf_status_t portMgrPortSerdesTxEqPostGet(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   int *tx_post) = 0;
  virtual bf_status_t portMgrPortSerdesTxEqPre2Get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   int *tx_pre2) = 0;
  virtual bf_status_t portMgrPortSerdesTxEqPost2Get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    int *tx_post2) = 0;
  virtual bf_status_t portMgrPortSerdesTxEqAttnGet(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   int *tx_attn) = 0;
  virtual bf_status_t portMgrFpIdxGetFirst(bf_dev_id_t dev_id,
                                           uint32_t *fp_idx) = 0;
  virtual bf_status_t portMgrFpIdxGetNext(bf_dev_id_t dev_id,
                                          uint32_t curr_idx,
                                          uint32_t *next_idx) = 0;
  virtual bf_status_t portMgrNumPipesGet(bf_dev_id_t dev_id,
                                         uint32_t *num_pipes) = 0;
  virtual bf_status_t portMgrMaxPortsGet(bf_dev_id_t dev_id,
                                         uint32_t *ports) = 0;
  virtual bf_status_t portMgrNumFrontPortsGet(bf_dev_id_t dev_id,
                                              uint32_t *ports) = 0;
  virtual bf_status_t portMgrIntrLinkMonitoringGet(bf_dev_id_t dev_id,
                                                   bool *is_enabled) = 0;
  virtual bf_status_t portMgrRecircDevPortsGet(
      bf_dev_id_t dev_id, uint32_t *recirc_devport_list) = 0;

  virtual bf_status_t portMgrPtpTxTimestampGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint64_t *ts,
                                               bool *ts_valid,
                                               int *ts_id) = 0;
  virtual bf_status_t portMgrPtpTxDeltaGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint16_t *delta) = 0;
  virtual bf_status_t portMgrPtpTxDeltaSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint16_t delta) = 0;
  virtual bf_status_t portMgrPtpRxDeltaGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint16_t *delta) = 0;
  virtual bf_status_t portMgrPtpRxDeltaSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint16_t delta) = 0;
  virtual bf_status_t portMgrPortIsAddedGet(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bool *is_added) = 0;

 protected:
  static std::unique_ptr<IPortMgrIntf> instance;
  static std::once_flag m_onceFlag;
};

class PortMgrIntf : public IPortMgrIntf {
 public:
  virtual ~PortMgrIntf() = default;
  PortMgrIntf() = default;
  static IPortMgrIntf *getInstance() {
    std::call_once(m_onceFlag, [] { instance.reset(new PortMgrIntf()); });
    return IPortMgrIntf::instance.get();
  }
  bf_status_t portMgrPortStatusNotifReg(port_status_chg_cb cb_fn, void *cookie);
  bf_status_t portMgrPortGetFirst(bf_dev_id_t dev_id, bf_dev_port_t *dev_port);
  bf_status_t portMgrPortGetFirstAdded(bf_dev_id_t dev_id,
                                       bf_dev_port_t *dev_port);
  bf_status_t portMgrPortGetNext(bf_dev_id_t dev_id,
                                 bf_dev_port_t curr_dev_port,
                                 bf_dev_port_t *next_dev_port);
  bf_status_t portMgrPortGetNextAdded(bf_dev_id_t dev_id,
                                      bf_dev_port_t curr_dev_port,
                                      bf_dev_port_t *next_dev_port);

  bool portMgrPortSpeedValidate(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_port_speed_t speed,
                                uint32_t n_lanes,
                                bf_fec_type_t fec);
  bf_status_t portMgrPortGetDefaultLaneNum(bf_dev_id_t dev_id,
                                           bf_port_speed_t speed,
                                           uint32_t *lane_numb);
  bf_status_t portMgrPortAdd(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             bf_port_speed_t speed,
                             bf_fec_type_t fec_type);
  bf_status_t portMgrPortAddWithLanes(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_speed_t speed,
                                      uint32_t n_lanes,
                                      bf_fec_type_t fec_type);
  bf_status_t portMgrPortSetLoopbackMode(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bf_loopback_mode_e mode);
  bf_status_t portMgrPortAnPolicySet(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     int an_policy);
  bf_status_t portMgrPortMtuSet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                uint32_t tx_mtu,
                                uint32_t rx_mtu);
  bf_status_t portMgrPortPllOvrclkSet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      float pll_ovrclk);
  bf_status_t portMgrPortFlowControlPfcSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t tx_en_map,
                                           uint32_t rx_en_map);
  bf_status_t portMgrPortFlowControlPauseFrameSet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bool tx_en,
                                                  bool rx_en);
  bf_status_t portMgrPortFECSet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_fec_type_t fec_type);
  bf_status_t portMgrPortCutThroughSet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bool ct_enable);
  bf_status_t portMgrPortDirSet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_pm_port_dir_e port_dir);

  bf_status_t portMgrPortStatsPollIntvlGet(bf_dev_id_t dev_id,
                                           uint32_t *poll_intvl_ms);

  bf_status_t portMgrPortLoopbackModeGet(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bf_loopback_mode_e *mode);

  bf_status_t portMgrPortAnGet(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               bf_pm_port_autoneg_policy_e *an_policy);

  bf_status_t portMgrPortMtuGet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                uint32_t *tx_mtu,
                                uint32_t *rx_mtu);

  bf_status_t portMgrPortPllOvrclkGet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      float *pll_ovrclk);

  bf_status_t portMgrPortFlowControlPfcGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t *tx_en_map,
                                           uint32_t *rx_en_map);

  bf_status_t portMgrPortFlowContrlPauseGet(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bool *tx_en,
                                            bool *rx_en);

  bf_status_t portMgrPortFecGet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_fec_type_t *fec_type);

  bf_status_t portMgrPortMediaTypeSet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_media_type_t media_type);
  bf_status_t portMgrPortSerdesParamsSet(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bf_pal_serdes_params_t *serdes_param);
  bf_status_t portMgrPortSpeedSet(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  bf_port_speed_t speed);
  bf_status_t portMgrPortSpeedWithLanesSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_port_speed_t speed,
                                           uint32_t n_lanes);
  bf_status_t portMgrPortEnable(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bool port_enable);
  bf_status_t portMgrPortOperStateGet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool *state);
  bf_status_t portMgrPortMediaTypeGet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_media_type_t *media_type);
  bf_status_t portMgrPortCutThroughGet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bool *ct_enabled);
  bf_status_t portMgrPortFrontPortGet(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_pal_front_port_handle_t *port_hdl);
  bf_status_t portMgrPortIsInternalGet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bool *is_internal);
  bf_status_t portMgrPortIsValidGet(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
  bf_status_t portMgrPortStrGet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                char *port_str);
  //
  bf_status_t portMgrPortDel(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
  bf_status_t portMgrPortDelWithLanes(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_speed_t speed,
                                      uint32_t n_lanes);
  bf_status_t portMgrPortDelAll(bf_dev_id_t dev_id);
  bf_status_t portMgrPortThisStatGet(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     bf_rmon_counter_t ctr_type,
                                     uint64_t *stat_val);
  bf_status_t portMgrPortPacketRateGet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bf_pkt_rate_t *pkt_rate);
  bf_status_t portMgrAllPureStatsGetWithTimestamp(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      uint64_t stats[BF_NUM_RMON_COUNTERS],
      int64_t *timestamp_s,
      int64_t *timestamp_ns);
  bf_status_t portMgrPortThisStatGetWithTimestamp(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bf_rmon_counter_t ctr_type,
                                                  uint64_t *stat_val,
                                                  int64_t *timestamp_s,
                                                  int64_t *timestamp_ns);
  bf_status_t portMgrPortThisStatClear(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bf_rmon_counter_t ctr_type);
  bf_status_t portMgrPortAllStatsGet(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint64_t stats[BF_NUM_RMON_COUNTERS]);
  bf_status_t portMgrPortAllStatsClear(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port);
  bf_status_t portMgrPortStatsPollIntvlSet(bf_dev_id_t dev_id,
                                           uint32_t poll_intvl_ms);
  bf_status_t portMgrPortStatDirectGet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bf_rmon_counter_t *ctr_type_array,
                                       uint64_t *stat_val,
                                       uint32_t num_of_ctr);

  //
  bf_status_t portMgrFrontPortToDevPortGet(bf_dev_id_t dev_id,
                                           bf_pal_front_port_handle_t *port_hdl,
                                           bf_dev_port_t *dev_port);
  bf_status_t portMgrFpIdxToDevPortGet(bf_dev_id_t dev_id,
                                       uint32_t fp_idx,
                                       bf_dev_port_t *dev_port);
  bf_status_t portMgrPortStrToDevPortGet(bf_dev_id_t dev_id,
                                         char *port_str,
                                         bf_dev_port_t *dev_port);
  bf_status_t portMgrPortSpeedGet(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  bf_port_speed_t *speed);
  bf_status_t portMgrPortNumLanesGet(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     int *num_lanes);
  bf_status_t portMgrPortIsEnabled(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bool *is_enabled);
  bf_status_t portMgrPortDirGet(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_pm_port_dir_e *port_dir);
  bf_status_t portMgrPortSerdesTxEqPreGet(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int *tx_pre);
  bf_status_t portMgrPortSerdesTxEqPostGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int *tx_post);
  bf_status_t portMgrPortSerdesTxEqPre2Get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int *tx_pre);
  bf_status_t portMgrPortSerdesTxEqPost2Get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            int *tx_post);
  bf_status_t portMgrPortSerdesTxEqAttnGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int *tx_attn);
  bf_status_t portMgrFpIdxGetFirst(bf_dev_id_t dev_id, uint32_t *fp_idx);
  bf_status_t portMgrFpIdxGetNext(bf_dev_id_t dev_id,
                                  uint32_t curr_idx,
                                  uint32_t *next_idx);
  bf_status_t portMgrNumPipesGet(bf_dev_id_t dev_id, uint32_t *num_pipes);
  bf_status_t portMgrMaxPortsGet(bf_dev_id_t dev_id, uint32_t *ports);
  bf_status_t portMgrNumFrontPortsGet(bf_dev_id_t dev_id, uint32_t *ports);
  bf_status_t portMgrIntrLinkMonitoringGet(bf_dev_id_t dev_id,
                                           bool *is_enabled);
  bf_status_t portMgrRecircDevPortsGet(bf_dev_id_t dev_id,
                                       uint32_t *recirc_devport_list);
  bf_status_t portMgrPtpTxTimestampGet(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint64_t *ts,
                                       bool *ts_valid,
                                       int *ts_id);
  bf_status_t portMgrPtpTxDeltaGet(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint16_t *delta);
  bf_status_t portMgrPtpTxDeltaSet(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint16_t delta);
  bf_status_t portMgrPtpRxDeltaGet(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint16_t *delta);
  bf_status_t portMgrPtpRxDeltaSet(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint16_t delta);
  bf_status_t portMgrPortIsAddedGet(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    bool *is_added);
};
}  // namespace bfrt

#endif  // _BF_RT_PORT_MGR_INTERFACE_HPP
