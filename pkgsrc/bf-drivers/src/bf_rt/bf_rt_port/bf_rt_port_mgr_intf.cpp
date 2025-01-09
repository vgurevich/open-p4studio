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


extern "C" {
#include <bf_rt/bf_rt_common.h>
}

/* bf_rt_includes */
#include <bf_rt/bf_rt_info.hpp>
#include "bf_rt_port_mgr_intf.hpp"

namespace bfrt {
std::unique_ptr<IPortMgrIntf> IPortMgrIntf::instance = nullptr;
std::once_flag IPortMgrIntf::m_onceFlag;

// Port configuration
bf_status_t PortMgrIntf::portMgrPortStatusNotifReg(port_status_chg_cb cb_fn,
                                                   void *cookie) {
  return bf_pal_port_status_notif_reg(cb_fn, cookie);
}

bf_status_t PortMgrIntf::portMgrPortGetFirst(bf_dev_id_t dev_id,
                                             bf_dev_port_t *dev_port) {
  return bf_pal_port_get_first(dev_id, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortGetFirstAdded(bf_dev_id_t dev_id,
                                                  bf_dev_port_t *dev_port) {
  return bf_pal_port_get_first_added(dev_id, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortGetNext(bf_dev_id_t dev_id,
                                            bf_dev_port_t curr_dev_port,
                                            bf_dev_port_t *next_dev_port) {
  return bf_pal_port_get_next(dev_id, curr_dev_port, next_dev_port);
}

bf_status_t PortMgrIntf::portMgrPortGetNextAdded(bf_dev_id_t dev_id,
                                                 bf_dev_port_t curr_dev_port,
                                                 bf_dev_port_t *next_dev_port) {
  return bf_pal_port_get_next_added(dev_id, curr_dev_port, next_dev_port);
}

bf_status_t PortMgrIntf::portMgrPortGetDefaultLaneNum(bf_dev_id_t dev_id,
                                                      bf_port_speed_t speed,
                                                      uint32_t *lane_numb) {
  return bf_pal_port_get_default_lane_numb(dev_id, speed, lane_numb);
}

bf_status_t PortMgrIntf::portMgrPortAdd(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_port_speed_t speed,
                                        bf_fec_type_t fec_type) {
  return bf_pal_port_add(dev_id, dev_port, speed, fec_type);
}

bf_status_t PortMgrIntf::portMgrPortAddWithLanes(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_port_speed_t speed,
                                                 uint32_t n_lanes,
                                                 bf_fec_type_t fec_type) {
  return bf_pal_port_add_with_lanes(dev_id, dev_port, speed, n_lanes, fec_type);
}

bf_status_t PortMgrIntf::portMgrPortSetLoopbackMode(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    bf_loopback_mode_e mode) {
  return bf_pal_port_loopback_mode_set(dev_id, dev_port, mode);
}

bf_status_t PortMgrIntf::portMgrPortAnPolicySet(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                int an_policy) {
  return bf_pal_port_autoneg_policy_set(dev_id, dev_port, an_policy);
}

bf_status_t PortMgrIntf::portMgrPortMtuSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t tx_mtu,
                                           uint32_t rx_mtu) {
  return bf_pal_port_mtu_set(dev_id, dev_port, tx_mtu, rx_mtu);
}

bf_status_t PortMgrIntf::portMgrPortPllOvrclkSet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 float pll_ovrclk) {
  return bf_pal_port_pll_ovrclk_set(dev_id, dev_port, pll_ovrclk);
}

bf_status_t PortMgrIntf::portMgrPortFlowControlPfcSet(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t tx_en_map,
                                                      uint32_t rx_en_map) {
  return bf_pal_port_flow_control_pfc_set(
      dev_id, dev_port, tx_en_map, rx_en_map);
}

bf_status_t PortMgrIntf::portMgrPortFlowControlPauseFrameSet(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, bool tx_en, bool rx_en) {
  return bf_pal_port_flow_control_link_pause_set(
      dev_id, dev_port, tx_en, rx_en);
}

bf_status_t PortMgrIntf::portMgrPortFECSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_fec_type_t fec_type) {
  return bf_pal_port_fec_set(dev_id, dev_port, fec_type);
}

bf_status_t PortMgrIntf::portMgrPortCutThroughSet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bool ct_enable) {
  if (ct_enable) {
    return bf_pal_port_cut_through_enable(dev_id, dev_port);
  } else {
    return bf_pal_port_cut_through_disable(dev_id, dev_port);
  }
}

bf_status_t PortMgrIntf::portMgrPortDirSet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_pm_port_dir_e port_dir) {
  return bf_pal_port_direction_set(dev_id, dev_port, port_dir);
}

bf_status_t PortMgrIntf::portMgrPortMediaTypeSet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_media_type_t media_type) {
  return bf_pal_port_media_type_set(dev_id, dev_port, media_type);
}

bf_status_t PortMgrIntf::portMgrPortSerdesParamsSet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_pal_serdes_params_t *serdes_param) {
  return bf_pal_port_serdes_params_set(dev_id, dev_port, serdes_param);
}

bf_status_t PortMgrIntf::portMgrPortEnable(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bool port_enable) {
  if (port_enable) {
    return bf_pal_port_enable(dev_id, dev_port);
  } else {
    return bf_pal_port_disable(dev_id, dev_port);
  }
}

bf_status_t PortMgrIntf::portMgrPortOperStateGet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bool *state) {
  return bf_pal_port_oper_state_get(dev_id, dev_port, state);
}

bf_status_t PortMgrIntf::portMgrPortMediaTypeGet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_media_type_t *media_type) {
  return bf_pal_port_media_type_get(dev_id, dev_port, media_type);
}

bf_status_t PortMgrIntf::portMgrPortCutThroughGet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bool *ct_enabled) {
  return bf_pal_port_cut_through_enable_status_get(
      dev_id, dev_port, ct_enabled);
}

bf_status_t PortMgrIntf::portMgrPortFrontPortGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_pal_front_port_handle_t *port_hdl) {
  return bf_pal_dev_port_to_front_port_get(dev_id, dev_port, port_hdl);
}

bool PortMgrIntf::portMgrPortSpeedValidate(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_port_speed_t speed,
                                           uint32_t n_lanes,
                                           bf_fec_type_t fec) {
  return bf_pal_dev_port_speed_validate(dev_id, dev_port, speed, n_lanes, fec);
}

bf_status_t PortMgrIntf::portMgrPortIsInternalGet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bool *is_internal) {
  return bf_pal_is_port_internal(dev_id, dev_port, is_internal);
}

bf_status_t PortMgrIntf::portMgrPortIsValidGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port) {
  return bf_pal_port_is_valid(dev_id, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortDel(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port) {
  return bf_pal_port_del(dev_id, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortDelWithLanes(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_port_speed_t speed,
                                                 uint32_t n_lanes) {
  return bf_pal_port_del_with_lanes(dev_id, dev_port, speed, n_lanes);
}

bf_status_t PortMgrIntf::portMgrPortIsAddedGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *is_added) {
  return bf_pal_is_port_added(dev_id, dev_port, is_added);
}

bf_status_t PortMgrIntf::portMgrPortDelAll(bf_dev_id_t dev_id) {
  return bf_pal_port_del_all(dev_id);
}

bf_status_t PortMgrIntf::portMgrPortStrGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           char *port_str) {
  return bf_pal_dev_port_to_port_str_map(dev_id, dev_port, port_str);
}

bf_status_t PortMgrIntf::portMgrPortStatsPollIntvlGet(bf_dev_id_t dev_id,
                                                      uint32_t *poll_intvl_ms) {
  return bf_pal_port_stats_poll_intvl_get(dev_id, poll_intvl_ms);
}

bf_status_t PortMgrIntf::portMgrPortLoopbackModeGet(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    bf_loopback_mode_e *mode) {
  return bf_pal_port_loopback_mode_get(dev_id, dev_port, mode);
}

bf_status_t PortMgrIntf::portMgrPortAnGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_pm_port_autoneg_policy_e *an_policy) {
  return bf_pal_port_autoneg_policy_get(dev_id, dev_port, an_policy);
}

bf_status_t PortMgrIntf::portMgrPortMtuGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t *tx_mtu,
                                           uint32_t *rx_mtu) {
  return bf_pal_port_mtu_get(dev_id, dev_port, tx_mtu, rx_mtu);
}

bf_status_t PortMgrIntf::portMgrPortPllOvrclkGet(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 float *pll_ovrclk) {
  return bf_pal_port_pll_ovrclk_get(dev_id, dev_port, pll_ovrclk);
}

bf_status_t PortMgrIntf::portMgrPortFlowControlPfcGet(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t *tx_en_map,
                                                      uint32_t *rx_en_map) {
  return bf_pal_port_flow_control_pfc_get(
      dev_id, dev_port, tx_en_map, rx_en_map);
}

bf_status_t PortMgrIntf::portMgrPortFlowContrlPauseGet(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool *tx_en,
                                                       bool *rx_en) {
  return bf_pal_port_flow_control_link_pause_get(
      dev_id, dev_port, tx_en, rx_en);
}

bf_status_t PortMgrIntf::portMgrPortFecGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_fec_type_t *fec_type) {
  return bf_pal_port_fec_get(dev_id, dev_port, fec_type);
}

// Port Stats
bf_status_t PortMgrIntf::portMgrPortThisStatGet(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                bf_rmon_counter_t ctr_type,
                                                uint64_t *stat_val) {
  return bf_pal_port_this_stat_get(dev_id, dev_port, ctr_type, stat_val);
}

bf_status_t PortMgrIntf::portMgrPortPacketRateGet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bf_pkt_rate_t *pkt_rate) {
  return bf_pal_port_pkt_rate_get(dev_id, dev_port, pkt_rate);
}

bf_status_t PortMgrIntf::portMgrPortThisStatClear(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  bf_rmon_counter_t ctr_type) {
  return bf_pal_port_this_stat_clear(dev_id, dev_port, ctr_type);
}

bf_status_t PortMgrIntf::portMgrPortAllStatsGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint64_t stats[BF_NUM_RMON_COUNTERS]) {
  return bf_pal_port_all_stats_get(dev_id, dev_port, stats);
}

bf_status_t PortMgrIntf::portMgrPortThisStatGetWithTimestamp(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_rmon_counter_t ctr_type,
    uint64_t *stat_val,
    int64_t *timestamp_s,
    int64_t *timestamp_ns) {
  return bf_pal_port_this_stat_get_with_timestamp(
      dev_id, dev_port, ctr_type, stat_val, timestamp_s, timestamp_ns);
}

bf_status_t PortMgrIntf::portMgrAllPureStatsGetWithTimestamp(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint64_t stats[BF_NUM_RMON_COUNTERS],
    int64_t *timestamp_s,
    int64_t *timestamp_ns) {
  return bf_pal_port_all_pure_stats_get_with_timestamp(
      dev_id, dev_port, stats, timestamp_s, timestamp_ns);
}

bf_status_t PortMgrIntf::portMgrPortAllStatsClear(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port) {
  return bf_pal_port_all_stats_clear(dev_id, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortStatsPollIntvlSet(bf_dev_id_t dev_id,
                                                      uint32_t poll_intvl_ms) {
  return bf_pal_port_stats_poll_intvl_set(dev_id, poll_intvl_ms);
}

bf_status_t PortMgrIntf::portMgrPortStatDirectGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_rmon_counter_t *ctr_type_array,
    uint64_t *stat_val,
    uint32_t num_of_ctr) {
  return bf_pal_port_stat_direct_get(
      dev_id, dev_port, ctr_type_array, stat_val, num_of_ctr);
}

// Port Info Conversion
bf_status_t PortMgrIntf::portMgrFrontPortToDevPortGet(
    bf_dev_id_t dev_id,
    bf_pal_front_port_handle_t *port_hdl,
    bf_dev_port_t *dev_port) {
  return bf_pal_front_port_to_dev_port_get(dev_id, port_hdl, dev_port);
}

bf_status_t PortMgrIntf::portMgrFpIdxToDevPortGet(bf_dev_id_t dev_id,
                                                  uint32_t fp_idx,
                                                  bf_dev_port_t *dev_port) {
  return bf_pal_fp_idx_to_dev_port_map(dev_id, fp_idx, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortStrToDevPortGet(bf_dev_id_t dev_id,
                                                    char *port_str,
                                                    bf_dev_port_t *dev_port) {
  return bf_pal_port_str_to_dev_port_map(dev_id, port_str, dev_port);
}

bf_status_t PortMgrIntf::portMgrPortSpeedGet(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             bf_port_speed_t *speed) {
  return bf_pal_port_speed_get(dev_id, dev_port, speed);
}

bf_status_t PortMgrIntf::portMgrPortSpeedSet(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             bf_port_speed_t speed) {
  return bf_pal_port_speed_set(dev_id, dev_port, speed);
}

bf_status_t PortMgrIntf::portMgrPortSpeedWithLanesSet(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      bf_port_speed_t speed,
                                                      uint32_t n_lanes) {
  return bf_pal_port_speed_with_lanes_set(dev_id, dev_port, speed, n_lanes);
}

bf_status_t PortMgrIntf::portMgrPortNumLanesGet(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                int *num_lanes) {
  return bf_pal_port_num_lanes_get(dev_id, dev_port, num_lanes);
}

bf_status_t PortMgrIntf::portMgrPortIsEnabled(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool *is_enabled) {
  return bf_pal_port_is_enabled(dev_id, dev_port, is_enabled);
}
bf_status_t PortMgrIntf::portMgrPortDirGet(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bf_pm_port_dir_e *port_dir) {
  return bf_pal_port_direction_get(dev_id, dev_port, port_dir);
}
bf_status_t PortMgrIntf::portMgrPortSerdesTxEqPreGet(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     int *tx_pre) {
  return bf_pal_port_serdes_tx_eq_pre_get(dev_id, dev_port, tx_pre);
}
bf_status_t PortMgrIntf::portMgrPortSerdesTxEqPostGet(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      int *tx_post) {
  return bf_pal_port_serdes_tx_eq_post_get(dev_id, dev_port, tx_post);
}
bf_status_t PortMgrIntf::portMgrPortSerdesTxEqPre2Get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      int *tx_pre2) {
  return bf_pal_port_serdes_tx_eq_pre2_get(dev_id, dev_port, tx_pre2);
}
bf_status_t PortMgrIntf::portMgrPortSerdesTxEqPost2Get(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       int *tx_post2) {
  return bf_pal_port_serdes_tx_eq_post2_get(dev_id, dev_port, tx_post2);
}
bf_status_t PortMgrIntf::portMgrPortSerdesTxEqAttnGet(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      int *tx_attn) {
  return bf_pal_port_serdes_tx_eq_attn_get(dev_id, dev_port, tx_attn);
}
bf_status_t PortMgrIntf::portMgrFpIdxGetFirst(bf_dev_id_t dev_id,
                                              uint32_t *fp_idx) {
  return bf_pal_fp_idx_get_first(dev_id, fp_idx);
}
bf_status_t PortMgrIntf::portMgrFpIdxGetNext(bf_dev_id_t dev_id,
                                             uint32_t curr_idx,
                                             uint32_t *next_idx) {
  return bf_pal_fp_idx_get_next(dev_id, curr_idx, next_idx);
}
bf_status_t PortMgrIntf::portMgrNumPipesGet(bf_dev_id_t dev_id,
                                            uint32_t *num_pipes) {
  return bf_pal_num_pipes_get(dev_id, num_pipes);
}
bf_status_t PortMgrIntf::portMgrMaxPortsGet(bf_dev_id_t dev_id,
                                            uint32_t *ports) {
  return bf_pal_max_ports_get(dev_id, ports);
}
bf_status_t PortMgrIntf::portMgrNumFrontPortsGet(bf_dev_id_t dev_id,
                                                 uint32_t *ports) {
  return bf_pal_num_front_ports_get(dev_id, ports);
}
bf_status_t PortMgrIntf::portMgrIntrLinkMonitoringGet(bf_dev_id_t dev_id,
                                                      bool *is_enabled) {
  return bf_pal_interrupt_based_link_monitoring_get(dev_id, is_enabled);
}
bf_status_t PortMgrIntf::portMgrRecircDevPortsGet(
    bf_dev_id_t dev_id, uint32_t *recirc_devport_list) {
  return bf_pal_recirc_devports_get(dev_id, recirc_devport_list);
}

bf_status_t PortMgrIntf::portMgrPtpTxTimestampGet(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint64_t *ts,
                                                  bool *ts_valid,
                                                  int *ts_id) {
  return bf_pal_port_1588_timestamp_get(dev_id, dev_port, ts, ts_valid, ts_id);
}
bf_status_t PortMgrIntf::portMgrPtpTxDeltaGet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint16_t *delta) {
  return bf_pal_port_1588_timestamp_delta_tx_get(dev_id, dev_port, delta);
}
bf_status_t PortMgrIntf::portMgrPtpTxDeltaSet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint16_t delta) {
  return bf_pal_port_1588_timestamp_delta_tx_set(dev_id, dev_port, delta);
}
bf_status_t PortMgrIntf::portMgrPtpRxDeltaGet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint16_t *delta) {
  return bf_pal_port_1588_timestamp_delta_rx_get(dev_id, dev_port, delta);
}

bf_status_t PortMgrIntf::portMgrPtpRxDeltaSet(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint16_t delta) {
  return bf_pal_port_1588_timestamp_delta_rx_set(dev_id, dev_port, delta);
}
}  // namespace bfrt
