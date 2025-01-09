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


#ifndef _BF_RT_TM_INTERFACE_HPP
#define _BF_RT_TM_INTERFACE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <bf_types/bf_types.h>
#include <traffic_mgr/traffic_mgr.h>
#include <traffic_mgr/traffic_mgr_read_apis.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <memory>
#include <mutex>

namespace bfrt {

class ITrafficMgrIntf {
 public:
  virtual ~ITrafficMgrIntf() = default;

  /*** TM Cofiguration read API. ***/
  virtual bf_status_t bfTMDevCfgGet(bf_dev_id_t dev, bf_tm_dev_cfg_t *cfg) = 0;

  /*** TM control API. **/
  virtual bf_status_t bfTMCompleteOperations(bf_dev_id_t dev) = 0;

  /*****PPG APIs*******/
  virtual bf_status_t bfTMPPGAllocate(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_ppg_hdl *ppg) = 0;
  virtual bf_status_t bfTMPPGFree(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) = 0;
  virtual bf_status_t bfTMPPGDefaultPpgGet(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_ppg_hdl *ppg) = 0;
  virtual bf_status_t bfTMPPGIcosMappingSet(bf_dev_id_t dev,
                                            bf_tm_ppg_hdl ppg,
                                            uint8_t icos_bmap) = 0;
  virtual bf_status_t bfTMPPGLosslessTreatmentEnable(bf_dev_id_t dev,
                                                     bf_tm_ppg_hdl ppg) = 0;
  virtual bf_status_t bfTMPPGLosslessTreatmentDisable(bf_dev_id_t dev,
                                                      bf_tm_ppg_hdl ppg) = 0;
  virtual bf_status_t bfTMPPGAppPoolUsageSet(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bf_tm_app_pool_t pool,
                                             uint32_t base_use_limit,
                                             bf_tm_ppg_baf_t dynamic_baf,
                                             uint32_t hysteresis) = 0;
  virtual bf_status_t bfTMPPGAppPoolUsageDisable(bf_dev_id_t dev,
                                                 bf_tm_app_pool_t pool,
                                                 bf_tm_ppg_hdl ppg) = 0;

  virtual bf_status_t bfTMPPGGuaranteedMinLimitSet(bf_dev_id_t dev,
                                                   bf_tm_ppg_hdl ppg,
                                                   uint32_t cells) = 0;

  virtual bf_status_t bfTMPPGSkidLimitSet(bf_dev_id_t dev,
                                          bf_tm_ppg_hdl ppg,
                                          uint32_t cells) = 0;

  virtual bf_status_t bfTMPPGGuaranteedMinSkidHysteresisSet(bf_dev_id_t dev,
                                                            bf_tm_ppg_hdl ppg,
                                                            uint32_t cells) = 0;
  virtual bf_status_t bfTMPPGAppPoolIdGet(bf_dev_id_t dev,
                                          bf_tm_ppg_hdl ppg,
                                          uint32_t *pool) = 0;

  virtual bf_status_t bfTMPPGAppPoolUsageGet(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bf_tm_app_pool_t pool,
                                             uint32_t *base_use_limit,
                                             bf_tm_ppg_baf_t *dynamic_baf,
                                             uint32_t *hysteresis) = 0;

  virtual bf_status_t bfTMPPGuaranteedMinLimitGet(bf_dev_id_t dev,
                                                  bf_tm_ppg_hdl ppg,
                                                  uint32_t *cells) = 0;

  virtual bf_status_t bfTMPPGLosslessTreatmentGet(bf_dev_id_t dev,
                                                  bf_tm_ppg_hdl ppg,
                                                  bool *pfc_val) = 0;

  virtual bf_status_t bfTMPPGSkidLimitGet(bf_dev_id_t dev,
                                          bf_tm_ppg_hdl ppg,
                                          uint32_t *cells) = 0;

  virtual bf_status_t bfTMPPGGuaranteedMinSkidHysteresisGet(
      bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t *cells) = 0;

  virtual bf_status_t bfTMPPGIcosMappingGet(bf_dev_id_t dev,
                                            bf_tm_ppg_hdl ppg,
                                            uint8_t *icos_bmap) = 0;
  //----
  virtual bf_status_t bfTMPPGTotalCntGet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         uint32_t *total_cnt) = 0;

  virtual bf_status_t bfTMPPGUnusedCntGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t *unused_cnt) = 0;

  virtual bf_status_t bfTMPPGPortGet(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg_hdl,
                                     bf_dev_port_t *port_id) = 0;

  virtual bf_status_t bfTMPPGNrGet(bf_dev_id_t dev,
                                   bf_tm_ppg_hdl ppg_hdl,
                                   bf_tm_ppg_id_t *ppg_nr) = 0;

  virtual bf_status_t bfTMPPGMirrorPortHandleGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_tm_ppg_hdl *ppg_hdl) = 0;

  virtual bf_status_t bfTMPPGBufferGetDefault(bf_dev_id_t dev,
                                              bf_tm_ppg_hdl ppg_hdl,
                                              uint32_t *min_limit_cells,
                                              uint32_t *hysteresis_cells) = 0;

  virtual bf_status_t bfTMPPGAppPoolUsageGetDefault(
      bf_dev_id_t dev,
      bf_tm_ppg_hdl ppg_hdl,
      bf_tm_app_pool_t *pool,
      uint32_t *pool_max_cells,
      bf_tm_ppg_baf_t *dynamic_baf) = 0;

  /****** TM Pipe API *****/
  virtual bf_status_t bfTMPipeIsValid(bf_dev_id_t dev, bf_dev_pipe_t pipe) = 0;
  virtual bf_status_t bfTMPipeGetCount(bf_dev_id_t dev, uint8_t *count) = 0;
  virtual bf_status_t bfTMPipeGetFirst(bf_dev_id_t dev,
                                       bf_dev_pipe_t *pipe) = 0;
  virtual bf_status_t bfTMPipeGetNext(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_dev_pipe_t *pipe_next) = 0;

  virtual bf_status_t bfTMPipeGetPortCount(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint16_t *count) = 0;
  virtual bf_status_t bfTMPipeGetPortFirst(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_dev_port_t *dev_port) = 0;
  virtual bf_status_t bfTMPipeGetPortNext(bf_dev_id_t dev,
                                          bf_dev_port_t dev_port,
                                          bf_dev_port_t *dev_port_next) = 0;

  virtual bf_status_t bfTMPipeGetPortGroupBasePort(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe,
                                                   bf_tm_pg_t pg_id,
                                                   bf_dev_port_t *dev_port) = 0;

  virtual bf_status_t bfTMPipeMirrorOnDropDestGet(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bf_dev_port_t *port,
                                                  bf_tm_queue_t *queue) = 0;
  virtual bf_status_t bfTMPipeMirrorOnDropDestDefaultGet(
      bf_dev_id_t dev,
      bf_dev_pipe_t pipe,
      bf_dev_port_t *port,
      bf_tm_queue_t *queue) = 0;
  virtual bf_status_t bfTMPipeMirrorOnDropDestSet(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) = 0;

  virtual bf_status_t bfTMPipeSchedPktIfgCompDefaultGet(
      bf_dev_id_t dev, bf_dev_pipe_t pipe, uint8_t *adjustment) = 0;
  virtual bf_status_t bfTMPipeSchedPktIfgCompGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint8_t *adjustment) = 0;
  virtual bf_status_t bfTMPipeSchedPktIfgCompSet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint8_t adjustment) = 0;

  virtual bf_status_t bfTMPipeSchedAdvFcModeDefaultGet(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bool *enable) = 0;
  virtual bf_status_t bfTMPipeSchedAdvFcModeGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bool *enable) = 0;
  virtual bf_status_t bfTMPipeSchedAdvFcModeSet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bool enable) = 0;

  virtual bf_status_t bfTMPipeQstatReportDefaultGet(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    bool *enable_any) = 0;
  virtual bf_status_t bfTMPipeQstatReportGet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool *enable_any) = 0;
  virtual bf_status_t bfTMPipeQstatReportSet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool enable_any) = 0;

  virtual bf_status_t bfTMPipeEgHysteresisDefaultGet(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     uint32_t *cells) = 0;
  virtual bf_status_t bfTMPipeEgHysteresisGet(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              uint32_t *cells) = 0;
  virtual bf_status_t bfTMPipeEgHysteresisSet(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              uint32_t cells) = 0;

  virtual bf_status_t bfTMPipeEgLimitDefaultGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t *cells) = 0;
  virtual bf_status_t bfTMPipeEgLimitGet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         uint32_t *cells) = 0;
  virtual bf_status_t bfTMPipeEgLimitSet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         uint32_t cells) = 0;

  virtual bf_status_t bfTMPipeMirrorDropEnableGet(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bool *enable) = 0;
  virtual bf_status_t bfTMPipeMirrorDropEnableDefaultGet(bf_dev_id_t dev,
                                                         bf_dev_pipe_t pipe,
                                                         bool *enable) = 0;
  virtual bf_status_t bfTMPipeMirrorDropEnableSet(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bool enable) = 0;
  /****** TM Port APIs *****/
  virtual bf_status_t bfTMPortIsValid(bf_dev_id_t dev,
                                      bf_dev_port_t dev_port) = 0;

  virtual bf_status_t bfTMPortStatusGet(bf_dev_id_t dev,
                                        bf_dev_port_t port_id,
                                        bool *is_offline,
                                        bool *is_enabled,
                                        bool *qac_rx_enable,
                                        bool *recirc_enable,
                                        bool *has_mac) = 0;

  virtual bf_status_t bfTMPortIcosCntGet(bf_dev_id_t dev,
                                         bf_dev_port_t port_id,
                                         uint8_t *icos_count) = 0;

  virtual bf_status_t bfTMPortIcosMapDefaultsGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 uint8_t *icos_mask) = 0;

  virtual bf_status_t bfTMPortPPGMapGet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        uint8_t *ppg_mask,
                                        bf_tm_ppg_hdl *ppg_hdlrs) = 0;
  //---
  virtual bf_status_t bfTMPortBaseQueueGet(bf_dev_id_t dev,
                                           bf_dev_port_t port_id,
                                           bf_tm_pg_t *pg_id,
                                           uint8_t *pg_queue,
                                           bool *is_mapped) = 0;

  virtual bf_status_t bfTMPortQMappingGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          uint8_t *q_count,
                                          uint8_t *q_mapping) = 0;
  virtual bf_status_t bfTMPortQMappingSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          uint8_t q_count,
                                          uint8_t *q_mapping) = 0;
  //---
  virtual bf_status_t bfTMPortCreditsGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint32_t *pkt_credits) = 0;
  //---
  virtual bf_status_t bfTMPortBufferDefaultsGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bool *ct_enable,
                                                uint8_t *uc_ct_limit_cells,
                                                uint32_t *ig_limit_cells,
                                                uint32_t *ig_hysteresis_cells,
                                                uint32_t *eg_limit_cells,
                                                uint32_t *eg_hysteresis_cells,
                                                uint32_t *skid_limit_cells) = 0;

  virtual bf_status_t bfTMPortCutThroughGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bool *ct_enable) = 0;

  virtual bf_status_t bfTMPortCutThroughLimitGet(
      bf_dev_id_t dev, bf_dev_port_t port, uint8_t *uc_ct_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufIgLimitGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint32_t *ig_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufIgHysteresisGet(
      bf_dev_id_t dev, bf_dev_port_t port, uint32_t *ig_hysteresis_cells) = 0;

  virtual bf_status_t bfTMPortBufEgLimitGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint32_t *eg_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufEgHysteresisGet(
      bf_dev_id_t dev, bf_dev_port_t port, uint32_t *eg_hysteresis_cells) = 0;

  virtual bf_status_t bfTMPortBufSkidLimitGet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t *skid_limit_cells) = 0;

  virtual bf_status_t bfTMPortCutThroughLimitSet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 uint8_t uc_ct_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufIgLimitSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint32_t ig_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufIgHysteresisSet(
      bf_dev_id_t dev, bf_dev_port_t port, uint32_t ig_hysteresis_cells) = 0;

  virtual bf_status_t bfTMPortBufEgLimitSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint32_t eg_limit_cells) = 0;

  virtual bf_status_t bfTMPortBufEgHysteresisSet(
      bf_dev_id_t dev, bf_dev_port_t port, uint32_t eg_hysteresis_cells) = 0;

  virtual bf_status_t bfTMPortBufSkidLimitSet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t skid_limit_cells) = 0;
  //---
  virtual bf_status_t bfTMPortFlowCtrlDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_flow_ctrl_type_t *mode_tx,
      bf_tm_flow_ctrl_type_t *mode_rx,
      uint8_t *cos_map) = 0;

  virtual bf_status_t bfTMPortFlowCtrlTxGet(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t *mode_tx) = 0;

  virtual bf_status_t bfTMPortFlowCtrlRxGet(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t *mode_rx) = 0;

  virtual bf_status_t bfTMPortCosMappingGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint8_t *cos_map) = 0;

  virtual bf_status_t bfTMPortFlowCtrlTxSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_flow_ctrl_type_t mode_tx) = 0;

  virtual bf_status_t bfTMPortFlowCtrlRxSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_flow_ctrl_type_t mode_rx) = 0;

  virtual bf_status_t bfTMPortCosMappingSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint8_t *cos_map) = 0;

  //---- Port Scheduling
  virtual bf_status_t bfTMPortSchedEnable(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_port_speeds_t speed) = 0;
  virtual bf_status_t bfTMPortSchedDisable(bf_dev_id_t dev,
                                           bf_dev_port_t port) = 0;
  //---
  virtual bf_status_t bfTMPortSchedSpeedGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_port_speeds_t *speed) = 0;
  virtual bf_status_t bfTMPortSchedSpeedResetGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_port_speeds_t *speed) = 0;
  //---
  virtual bf_status_t bfTMPortSchedShapingEnableDefaultGet(bf_dev_id_t dev,
                                                           bf_dev_port_t port,
                                                           bool *enable) = 0;
  virtual bf_status_t bfTMPortSchedShapingEnableGet(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bool *enable) = 0;
  virtual bf_status_t bfTMPortSchedShapingEnable(bf_dev_id_t dev,
                                                 bf_dev_port_t port) = 0;
  virtual bf_status_t bfTMPortSchedShapingDisable(bf_dev_id_t dev,
                                                  bf_dev_port_t port) = 0;
  //---
  virtual bf_status_t bfTMPortSchedMaxRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) = 0;
  virtual bf_status_t bfTMPortSchedMaxRateGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) = 0;
  virtual bf_status_t bfTMPortSchedMaxRateSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool pps,
      uint32_t burst_size,
      uint32_t rate,
      bf_tm_sched_shaper_prov_type_t prov_type) = 0;

  /****** TM Port Group APIs ******/
  virtual bf_status_t bfTMPortGroupGet(bf_dev_id_t dev,
                                       bf_dev_port_t port_id,
                                       bf_tm_pg_t *pg_id,
                                       uint8_t *pg_port_nr) = 0;

  virtual bf_status_t bfTMPortGroupPortQueueGet(const bf_rt_target_t &dev_tgt,
                                                bf_tm_pg_t pg_id,
                                                uint8_t pg_queue,
                                                bf_dev_port_t *port,
                                                bf_tm_queue_t *queue_nr,
                                                bool *is_mapped) = 0;

  /****** TM Queue APIs *****/
  virtual bf_status_t bfTMQueuePfcCosGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         uint8_t *cos) = 0;
  virtual bf_status_t bfTMQueuePfcCosSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         uint8_t cos) = 0;
  virtual bf_status_t bfTMQueueColorDropGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_queue_t queue,
                                            bool *is_enabled) = 0;
  virtual bf_status_t bfTMQueueColorDropSet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_queue_t queue,
                                            bool enable) = 0;

  virtual bf_status_t bfTMQueueVisibleGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bool *is_visible) = 0;
  virtual bf_status_t bfTMQueueVisibleSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bool is_visible) = 0;
  virtual bf_status_t bfTMQueueVisibleDefaultsGet(bf_dev_id_t dev,
                                                  bool *is_visible) = 0;

  virtual bf_status_t bfTMQueueColorDropLimitGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_color_t color,
      bf_tm_queue_color_limit_t *limit) = 0;
  virtual bf_status_t bfTMQueueColorDropLimitSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_color_t color,
      bf_tm_queue_color_limit_t limit) = 0;
  virtual bf_status_t bfTMQueueColorHysteresisGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_color_t color,
      bf_tm_thres_t *hyst_cells) = 0;
  virtual bf_status_t bfTMQueueColorHysteresisSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  bf_tm_color_t color,
                                                  bf_tm_thres_t hyst_cells) = 0;

  virtual bf_status_t bfTMQueueGuaranteedCellsGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  uint32_t *cells) = 0;
  virtual bf_status_t bfTMQueueGuaranteedCellsSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  uint32_t cells) = 0;

  virtual bf_status_t bfTMQueueTailDropGet(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool *is_enabled) = 0;
  virtual bf_status_t bfTMQueueTailDropSet(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool enable) = 0;

  virtual bf_status_t bfTMQueueHysteresisGet(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             uint32_t *hyst_cells) = 0;
  virtual bf_status_t bfTMQueueHysteresisSet(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             uint32_t cells) = 0;

  virtual bf_status_t bfTMQueueAppPoolGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_app_pool_t *pool,
                                          uint32_t *base_use_limit,
                                          bf_tm_queue_baf_t *dynamic_baf,
                                          uint32_t *hyst_cells) = 0;
  virtual bf_status_t bfTMQueueAppPoolSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_app_pool_t pool,
                                          uint32_t base_use_limit,
                                          bf_tm_queue_baf_t dynamic_baf,
                                          uint32_t hyst_cells) = 0;

  virtual bf_status_t bfTMQueueAppPoolLimitGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               uint32_t *cells) = 0;

  virtual bf_status_t bfTMQueueAppPoolDisable(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue) = 0;

  virtual bf_status_t bfTMQueueCfgDefaultsGet(bf_dev_id_t dev,
                                              uint8_t *pfc_cos) = 0;

  virtual bf_status_t bfTMQueueColorDefaultsGet(
      bf_dev_id_t dev,
      bool *drop_enable,
      bf_tm_queue_color_limit_t *yellow_drop_limit,
      bf_tm_thres_t *hyst_yellow,
      bf_tm_queue_color_limit_t *red_drop_limit,
      bf_tm_thres_t *hyst_red) = 0;

  virtual bf_status_t bfTMQueueBufferDefaultsGet(
      bf_dev_id_t dev,
      uint32_t *guaranteed_cells,
      uint32_t *hysteresis_cells,
      bool *tail_drop_enable,
      bf_tm_app_pool_t *pool_id,
      uint32_t *pool_max_cells,
      bf_tm_queue_baf_t *dynamic_baf) = 0;

  //--- Queue Scheduling
  virtual bf_status_t bfTMQueueSchedEnableGet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue,
                                              bool *enable) = 0;
  virtual bf_status_t bfTMQueueSchedEnableDefaultGet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue,
                                                     bool *enable) = 0;
  virtual bf_status_t bfTMQueueSchedEnable(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue) = 0;
  virtual bf_status_t bfTMQueueSchedDisable(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_queue_t queue) = 0;

  virtual bf_status_t bfTMQueueSchedSpeedGet(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             bf_port_speeds_t *speed) = 0;

  virtual bf_status_t bfTMQueueSchedGuaranteedEnableGet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue,
                                                        bool *enable) = 0;
  virtual bf_status_t bfTMQueueSchedGuaranteedEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *enable,
      bf_tm_sched_prio_t *priority) = 0;
  virtual bf_status_t bfTMQueueSchedGuaranteedEnable(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue) = 0;
  virtual bf_status_t bfTMQueueSchedGuaranteedDisable(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_queue_t queue) = 0;
  virtual bf_status_t bfTMQueueSchedGuaranteedPriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t *priority) = 0;
  virtual bf_status_t bfTMQueueSchedGuaranteedPrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t priority) = 0;

  virtual bf_status_t bfTMQueueSchedShapingEnableGet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue,
                                                     bool *enable) = 0;
  virtual bf_status_t bfTMQueueSchedShapingEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *enable,
      bf_tm_sched_prio_t *priority) = 0;
  virtual bf_status_t bfTMQueueSchedShapingEnable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) = 0;
  virtual bf_status_t bfTMQueueSchedShapingDisable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_queue_t queue) = 0;
  virtual bf_status_t bfTMQueueSchedRemainingBwPriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t *priority) = 0;
  virtual bf_status_t bfTMQueueSchedRemainingBwPrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t priority) = 0;

  virtual bf_status_t bfTMQueueSchedDwrrWeightGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  uint16_t *weight) = 0;
  virtual bf_status_t bfTMQueueSchedDwrrWeightDefaultGet(bf_dev_id_t dev,
                                                         bf_dev_port_t port,
                                                         bf_tm_queue_t queue,
                                                         uint16_t *weight) = 0;
  virtual bf_status_t bfTMQueueSchedDwrrWeightSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  uint16_t weight) = 0;

  virtual bf_status_t bfTMQueueSchedAdvFcModeGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t *mode) = 0;
  virtual bf_status_t bfTMQueueSchedAdvFcModeDefaultGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t *mode) = 0;
  virtual bf_status_t bfTMQueueSchedAdvFcModeSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t mode) = 0;

  virtual bf_status_t bfTMQueueSchedMaxRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) = 0;
  virtual bf_status_t bfTMQueueSchedMaxRateGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) = 0;
  virtual bf_status_t bfTMQueueSchedMaxRateSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool pps,
      uint32_t burst_size,
      uint32_t rate,
      bf_tm_sched_shaper_prov_type_t prov_type) = 0;
  virtual bf_status_t bfTMQueueSchedMinRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) = 0;
  virtual bf_status_t bfTMQueueSchedMinRateGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               bool *pps,
                                               uint32_t *burst_size,
                                               uint32_t *rate) = 0;
  virtual bf_status_t bfTMQueueSchedMinRateSet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               bool pps,
                                               uint32_t burst_size,
                                               uint32_t rate) = 0;

  //--------------------- TM L1 Node API
  virtual bf_status_t bfTML1NodePortAssignmentGet(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bf_tm_pg_t pg_id,
                                                  bf_tm_l1_node_t l1_node,
                                                  bool *in_use,
                                                  uint8_t *pg_port_nr,
                                                  bf_dev_port_t *port_id) = 0;

  virtual bf_status_t bfTML1NodePortAssignmentDefaultGet(
      bf_dev_id_t dev,
      bf_dev_pipe_t pipe,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t l1_node,
      bool *in_use,
      uint8_t *pg_port_nr,
      bf_dev_port_t *port_id) = 0;

  virtual bf_status_t bfTML1NodeQueueAssignmentGet(
      bf_dev_id_t dev,
      bf_dev_pipe_t pipe,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t l1_node,
      uint8_t *l1_queues_cnt,
      bf_tm_queue_t *l1_queues) = 0;

  virtual bf_status_t bfTMPortL1NodeAssignmentGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t port_id,
      bool in_use_only,
      uint8_t *l1_nodes_cnt,
      bf_tm_l1_node_t *l1_nodes) = 0;

  virtual bf_status_t bfTMQueueL1NodeSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTMQueueL1NodeGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_l1_node_t *l1_node) = 0;

  virtual bf_status_t bfTMQueueL1NodeDefaultGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bf_tm_l1_node_t *l1_node) = 0;

  virtual bf_status_t bfTML1NodeFree(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedEnableGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_l1_node_t l1_node,
                                               bool *enable) = 0;

  virtual bf_status_t bfTML1NodeSchedEnable(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedDisable(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_l1_node_t l1_node) = 0;
  //---
  virtual bf_status_t bfTML1NodeSchedMinRateEnableGet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *enable) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRateEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *enable,
      bf_tm_sched_prio_t *priority) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRateEnable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRateDisable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRatePriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t *priority) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRatePrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t priority) = 0;

  //---
  virtual bf_status_t bfTML1NodeSchedMaxRateEnableGet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *enable) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRateEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *enable,
      bf_tm_sched_prio_t *priority) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRateEnable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRateDisable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRatePriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t *priority) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRatePrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t priority) = 0;

  virtual bf_status_t bfTML1NodeSchedDwrrWeightGet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   uint16_t *weight) = 0;

  virtual bf_status_t bfTML1NodeSchedDwrrWeightDefaultGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      uint16_t *weight) = 0;

  virtual bf_status_t bfTML1NodeSchedDwrrWeightSet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   uint16_t weight) = 0;
  //---
  virtual bf_status_t bfTML1NodeSchedPriorityPropagationGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *is_enabled) = 0;

  virtual bf_status_t bfTML1NodeSchedPriorityPropagationDefaultGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *is_enabled) = 0;

  virtual bf_status_t bfTML1NodeSchedPriorityPropagationEnable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) = 0;

  virtual bf_status_t bfTML1NodeSchedPriorityPropagationDisable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) = 0;

  //---
  virtual bf_status_t bfTML1NodeSchedMaxRateDefaultsGet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_l1_node_t l1_node,
                                                        bool *pps,
                                                        uint32_t *burst_size,
                                                        uint32_t *rate) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRateGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool *pps,
                                                uint32_t *burst_size,
                                                uint32_t *rate) = 0;

  virtual bf_status_t bfTML1NodeSchedMaxRateSet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool pps,
                                                uint32_t burst_size,
                                                uint32_t rate) = 0;

  //---
  virtual bf_status_t bfTML1NodeSchedMinRateDefaultsGet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_l1_node_t l1_node,
                                                        bool *pps,
                                                        uint32_t *burst_size,
                                                        uint32_t *rate) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRateGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool *pps,
                                                uint32_t *burst_size,
                                                uint32_t *rate) = 0;

  virtual bf_status_t bfTML1NodeSchedMinRateSet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool pps,
                                                uint32_t burst_size,
                                                uint32_t rate) = 0;

  /***** Pool Config APIs *******/
  virtual bf_status_t bfTmAppPoolSizeSet(bf_dev_id_t dev,
                                         bf_tm_app_pool_t pool,
                                         uint32_t cells) = 0;

  virtual bf_status_t bfTmAppPoolSizeGet(bf_dev_id_t dev,
                                         bf_tm_app_pool_t pool,
                                         uint32_t *cells) = 0;

  virtual bf_status_t bfTmPoolSkidSizeSet(bf_dev_id_t dev, uint32_t cells) = 0;

  virtual bf_status_t bfTmPoolMirrorOnDropSizeSet(bf_dev_id_t dev,
                                                  uint32_t cells) = 0;

  virtual bf_status_t bfTmPreFifoLimitSet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint8_t fifo,
                                          uint32_t cells) = 0;

  virtual bf_status_t bfTmGlobalMinLimitSet(bf_dev_id_t dev,
                                            uint32_t cells) = 0;

  virtual bf_status_t bfTmPoolUcCutThroughSizeSet(bf_dev_id_t dev,
                                                  uint32_t cells) = 0;

  virtual bf_status_t bfTmPoolMcCutThroughSizeSet(bf_dev_id_t dev,
                                                  uint32_t cells) = 0;

  virtual bf_status_t bfTmPoolSkidSizeGet(bf_dev_id_t dev, uint32_t *cells) = 0;

  virtual bf_status_t bfTmPoolMirrorOnDropSizeGet(bf_dev_id_t dev,
                                                  uint32_t *cells) = 0;

  virtual bf_status_t bfTmPreFifoLimitGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint8_t fifo,
                                          uint32_t *cells) = 0;

  virtual bf_status_t bfTmGlobalMinLimitGet(bf_dev_id_t dev,
                                            uint32_t *cells) = 0;

  virtual bf_status_t bfTmPoolUcCutThroughSizeGet(bf_dev_id_t dev,
                                                  uint32_t *cells) = 0;

  virtual bf_status_t bfTmPoolMcCutThroughSizeGet(bf_dev_id_t dev,
                                                  uint32_t *cells) = 0;

  /***** Skid Pool APIs *******/
  virtual bf_status_t bfTmSkidPoolHysteresisSet(bf_dev_id_t dev,
                                                uint32_t cells) = 0;

  virtual bf_status_t bfTmSkidPoolHysteresisGet(bf_dev_id_t dev,
                                                uint32_t *cells) = 0;

  virtual bf_status_t bfTmSkidPoolHysteresisGetDefault(bf_dev_id_t dev,
                                                       uint32_t *cells) = 0;

  /***** App Pool APIs *******/
  virtual bf_status_t bfTmPoolColorDropEnable(bf_dev_id_t dev,
                                              bf_tm_app_pool_t pool) = 0;

  virtual bf_status_t bfTmPoolColorDropDisable(bf_dev_id_t dev,
                                               bf_tm_app_pool_t pool) = 0;

  virtual bf_status_t bfTmAppPoolColorDropLimitSet(bf_dev_id_t dev,
                                                   bf_tm_app_pool_t pool,
                                                   bf_tm_color_t color,
                                                   uint32_t limit) = 0;

  virtual bf_status_t bfTmPoolColorDropLimitGet(bf_dev_id_t dev,
                                                bf_tm_app_pool_t pool,
                                                bf_tm_color_t color,
                                                uint32_t *limit) = 0;

  virtual bf_status_t bfTmPoolColorDropStateGet(bf_dev_id_t dev,
                                                bf_tm_app_pool_t pool,
                                                bool *drop_state) = 0;

  /***** Pool color APIs *******/
  virtual bf_status_t bfTmPoolColorDropHysteresisSet(bf_dev_id_t dev,
                                                     bf_tm_color_t color,
                                                     uint32_t limit) = 0;

  virtual bf_status_t bfTmPoolColorDropHysteresisGet(bf_dev_id_t dev,
                                                     bf_tm_color_t color,
                                                     uint32_t *limit) = 0;

  virtual bf_status_t bfTmPoolColorDropHysteresisDefaultGet(
      bf_dev_id_t dev, bf_tm_color_t color, uint32_t *limit) = 0;

  /***** App Pool PFC APIs *******/
  virtual bf_status_t bfTmPoolPfcLimitSet(bf_dev_id_t dev,
                                          bf_tm_app_pool_t pool,
                                          bf_tm_icos_t icos,
                                          uint32_t limit) = 0;

  virtual bf_status_t bfTmPoolPfcLimitGet(bf_dev_id_t dev,
                                          bf_tm_app_pool_t pool,
                                          bf_tm_icos_t icos,
                                          uint32_t *limit) = 0;

  virtual bf_status_t bfTmPoolPfcLimitGetDefault(bf_dev_id_t dev,
                                                 bf_tm_app_pool_t pool,
                                                 bf_tm_icos_t icos,
                                                 uint32_t *limit) = 0;

  virtual bf_status_t bfTmMaxPfcLevelsGet(bf_dev_id_t dev,
                                          uint32_t *levels) = 0;

  /***** Port counter APIs *******/
  virtual bf_status_t bfTmPortDropGetCached(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bf_dev_port_t port,
                                            uint64_t *ig_count,
                                            uint64_t *eg_count) = 0;

  virtual bf_status_t bfTmPortUsageGet(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       bf_dev_port_t port,
                                       uint32_t *ig_count,
                                       uint32_t *eg_count,
                                       uint32_t *ig_wm,
                                       uint32_t *eg_wm) = 0;

  virtual bf_status_t bfTmPortIngressWatermarkClear(bf_dev_id_t dev,
                                                    bf_dev_port_t port) = 0;

  virtual bf_status_t bfTmPortEgressWatermarkClear(bf_dev_id_t dev,
                                                   bf_dev_port_t port) = 0;

  virtual bf_status_t bfTmPortDropIngressCacheSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint64_t drop_count) = 0;

  virtual bf_status_t bfTmPortDropEgressCacheSet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 uint64_t drop_count) = 0;

  /***** Queue counter APIs *******/
  virtual bf_status_t bfTmQDropGetCached(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         uint64_t *count) = 0;

  virtual bf_status_t bfTmQDropCacheSet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bf_dev_port_t port,
                                        bf_tm_queue_t queue,
                                        uint64_t count) = 0;

  virtual bf_status_t bfTmQUsageGet(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    uint32_t *count,
                                    uint32_t *wm) = 0;

  virtual bf_status_t bfTmQWatermarkClear(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue) = 0;

  /***** Pool counter APIs *******/
  virtual bf_status_t bfTmPoolUsageGet(bf_dev_id_t dev,
                                       bf_tm_app_pool_t pool,
                                       uint32_t *count,
                                       uint32_t *wm) = 0;

  virtual bf_status_t bfTmPoolWatermarkClear(bf_dev_id_t dev,
                                             bf_tm_app_pool_t pool) = 0;

  /***** Pipe counter APIs *******/
  virtual bf_status_t bfTmPipeBufferFullDropGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint64_t *count) = 0;

  virtual bf_status_t bfTmQDiscardUsageGet(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint32_t *count,
                                           uint32_t *wm) = 0;

  virtual bf_status_t bfTmPipeCountersGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint64_t *cell_count,
                                          uint64_t *pkt_count) = 0;

  virtual bf_status_t bfTmCutThroughCountersGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint64_t *uc_count,
                                                uint64_t *mc_count) = 0;

  virtual bf_status_t bfTmBlklvlDropGet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bf_tm_blklvl_cntrs_t *blk_cntrs) = 0;

  virtual bf_status_t bfTmPreFifoDropGet(
      bf_dev_id_t dev, bf_tm_pre_fifo_cntrs_t *fifo_cntrs) = 0;

  virtual bf_status_t bfTmPipeBufferFullDropClear(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmQDiscardWatermarkClear(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmPipeClearCellCounter(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmPipeClearPacketCounter(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmPipeClearUcCtPacketCounter(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmPipeClearMcCtPacketCounter(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe) = 0;

  virtual bf_status_t bfTmBlklvlDropClear(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t clear_mask) = 0;

  virtual bf_status_t bfTmPreFifoDropClear(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint32_t fifo) = 0;

  /***** PPG Counter APIs *******/
  virtual bf_status_t bfTmPpgDropGet(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_tm_ppg_hdl ppg,
                                     uint64_t *count) = 0;

  virtual bf_status_t bfTmPpgDropGetCached(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_tm_ppg_hdl ppg,
                                           uint64_t *count) = 0;

  virtual bf_status_t bfTmPpgDropCacheSet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bf_tm_ppg_hdl ppg,
                                          uint64_t count) = 0;

  virtual bf_status_t bfTmPpgUsageGet(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_tm_ppg_hdl ppg,
                                      uint32_t *gmin_count,
                                      uint32_t *shared_count,
                                      uint32_t *skid_count,
                                      uint32_t *wm) = 0;

  virtual bf_status_t bfTmPpgWatermarkClear(bf_dev_id_t dev,
                                            bf_tm_ppg_hdl ppg) = 0;

  /***** TM Cfg APIs *******/
  virtual bf_status_t bfTmTimestampShiftSet(bf_dev_id_t dev, uint8_t shift) = 0;

  virtual bf_status_t bfTmTimestampShiftGet(bf_dev_id_t dev,
                                            uint8_t *shift) = 0;

  virtual bf_status_t bfTmTimestampShiftGetDefault(bf_dev_id_t dev,
                                                   uint8_t *shift) = 0;

  virtual bf_status_t bfTmCellSizeInBytesGet(bf_dev_id_t dev,
                                             uint32_t *bytes) = 0;

  virtual bf_status_t bfTmTotalCellCountGet(bf_dev_id_t dev,
                                            uint32_t *total_cells) = 0;

  virtual bf_status_t bfTmIngressBufferLimitSet(bf_dev_id_t dev,
                                                uint32_t cells) = 0;

  virtual bf_status_t bfTmIngressBufferLimitGet(bf_dev_id_t dev,
                                                uint32_t *cells) = 0;

  virtual bf_status_t bfTmIngressBufferLimitGetDefault(bf_dev_id_t dev,
                                                       uint32_t *cells) = 0;

  virtual bf_status_t bfTmIngressBufferLimitEnable(bf_dev_id_t dev) = 0;

  virtual bf_status_t bfTmIngressBufferLimitDisable(bf_dev_id_t dev) = 0;

  virtual bf_status_t bfTmIngressBufferLimitStateGet(bf_dev_id_t dev,
                                                     bool *state) = 0;

  virtual bf_status_t bfTmIngressBufferLimitStateGetDefault(bf_dev_id_t dev,
                                                            bool *state) = 0;

  /***** TM Pipe Multicast fifo APIs *******/
  virtual bf_status_t bfTmMcFifoIcosMappingSet(bf_dev_id_t dev,
                                               uint8_t pipe_bmap,
                                               int fifo,
                                               uint8_t icos_bmap) = 0;

  virtual bf_status_t bfTmMcFifoIcosMappingGet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               int fifo,
                                               uint8_t *icos_bmap) = 0;

  virtual bf_status_t bfTmMcFifoIcosMappingGetDefault(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      int fifo,
                                                      uint8_t *icos_bmap) = 0;

  virtual bf_status_t bfTmMcFifoArbModeSet(bf_dev_id_t dev,
                                           uint8_t pipe_bmap,
                                           int fifo,
                                           bool use_strict_pri) = 0;

  virtual bf_status_t bfTmMcFifoArbModeGet(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           int fifo,
                                           bool *use_strict_pri) = 0;

  virtual bf_status_t bfTmMcFifoArbModeGetDefault(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  int fifo,
                                                  bool *use_strict_pri) = 0;

  virtual bf_status_t bfTmMcFifoWrrWeightSet(bf_dev_id_t dev,
                                             uint8_t pipe_bmap,
                                             int fifo,
                                             uint8_t weight) = 0;

  virtual bf_status_t bfTmMcFifoWrrWeightGet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             int fifo,
                                             uint8_t *weight) = 0;

  virtual bf_status_t bfTmMcFifoWrrWeightGetDefault(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    int fifo,
                                                    uint8_t *weight) = 0;

  virtual bf_status_t bfTmMcFifoDepthSet(bf_dev_id_t dev,
                                         uint8_t pipe_bmap,
                                         int fifo,
                                         int size) = 0;

  virtual bf_status_t bfTmMcFifoDepthGet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         int fifo,
                                         int *size) = 0;

  virtual bf_status_t bfTmMcFifoDepthGetDefault(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                int fifo,
                                                int *size) = 0;

 protected:
  static std::unique_ptr<ITrafficMgrIntf> instance;
  static std::mutex tm_mgr_intf_mtx;
};

class TrafficMgrIntf : public ITrafficMgrIntf {
 public:
  virtual ~TrafficMgrIntf() {
    if (instance) {
      instance.release();
    }
  };

  TrafficMgrIntf() = default;

  static ITrafficMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      tm_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new TrafficMgrIntf());
      }
      tm_mgr_intf_mtx.unlock();
    }

    return ITrafficMgrIntf::instance.get();
  }

  /*** TM Cofiguration read API. ***/
  bf_status_t bfTMDevCfgGet(bf_dev_id_t dev, bf_tm_dev_cfg_t *cfg) override;

  /*** TM control API. **/
  bf_status_t bfTMCompleteOperations(bf_dev_id_t dev) override;

  /*****PPG APIs*******/
  bf_status_t bfTMPPGAllocate(bf_dev_id_t dev,
                              bf_dev_port_t port,
                              bf_tm_ppg_hdl *ppg) override;
  bf_status_t bfTMPPGFree(bf_dev_id_t dev, bf_tm_ppg_hdl ppg);
  bf_status_t bfTMPPGDefaultPpgGet(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_ppg_hdl *ppg) override;
  bf_status_t bfTMPPGIcosMappingSet(bf_dev_id_t dev,
                                    bf_tm_ppg_hdl ppg,
                                    uint8_t icos_bmap);
  bf_status_t bfTMPPGLosslessTreatmentEnable(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg) override;
  bf_status_t bfTMPPGLosslessTreatmentDisable(bf_dev_id_t dev,
                                              bf_tm_ppg_hdl ppg) override;
  bf_status_t bfTMPPGAppPoolUsageSet(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     bf_tm_app_pool_t pool,
                                     uint32_t base_use_limit,
                                     bf_tm_ppg_baf_t dynamic_baf,
                                     uint32_t hysteresis) override;
  bf_status_t bfTMPPGAppPoolUsageDisable(bf_dev_id_t dev,
                                         bf_tm_app_pool_t pool,
                                         bf_tm_ppg_hdl ppg) override;

  bf_status_t bfTMPPGGuaranteedMinLimitSet(bf_dev_id_t dev,
                                           bf_tm_ppg_hdl ppg,
                                           uint32_t cells) override;

  bf_status_t bfTMPPGSkidLimitSet(bf_dev_id_t dev,
                                  bf_tm_ppg_hdl ppg,
                                  uint32_t cells) override;

  bf_status_t bfTMPPGGuaranteedMinSkidHysteresisSet(bf_dev_id_t dev,
                                                    bf_tm_ppg_hdl ppg,
                                                    uint32_t cells) override;

  bf_status_t bfTMPPGAppPoolIdGet(bf_dev_id_t dev,
                                  bf_tm_ppg_hdl ppg,
                                  uint32_t *pool) override;

  bf_status_t bfTMPPGAppPoolUsageGet(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     bf_tm_app_pool_t pool,
                                     uint32_t *base_use_limit,
                                     bf_tm_ppg_baf_t *dynamic_baf,
                                     uint32_t *hysteresis) override;

  bf_status_t bfTMPPGuaranteedMinLimitGet(bf_dev_id_t dev,
                                          bf_tm_ppg_hdl ppg,
                                          uint32_t *cells) override;

  bf_status_t bfTMPPGLosslessTreatmentGet(bf_dev_id_t dev,
                                          bf_tm_ppg_hdl ppg,
                                          bool *pfc_val) override;

  bf_status_t bfTMPPGSkidLimitGet(bf_dev_id_t dev,
                                  bf_tm_ppg_hdl ppg,
                                  uint32_t *cells) override;

  bf_status_t bfTMPPGGuaranteedMinSkidHysteresisGet(bf_dev_id_t dev,
                                                    bf_tm_ppg_hdl ppg,
                                                    uint32_t *cells) override;

  bf_status_t bfTMPPGIcosMappingGet(bf_dev_id_t dev,
                                    bf_tm_ppg_hdl ppg,
                                    uint8_t *icos_bmap) override;
  //----
  bf_status_t bfTMPPGTotalCntGet(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 uint32_t *total_cnt) override;

  bf_status_t bfTMPPGUnusedCntGet(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  uint32_t *unused_cnt) override;

  bf_status_t bfTMPPGPortGet(bf_dev_id_t dev,
                             bf_tm_ppg_hdl ppg_hdl,
                             bf_dev_port_t *port_id) override;

  bf_status_t bfTMPPGNrGet(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg_hdl,
                           bf_tm_ppg_id_t *ppg_nr) override;

  bf_status_t bfTMPPGMirrorPortHandleGet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         bf_tm_ppg_hdl *ppg_hdl) override;

  bf_status_t bfTMPPGBufferGetDefault(bf_dev_id_t dev,
                                      bf_tm_ppg_hdl ppg_hdl,
                                      uint32_t *min_limit_cells,
                                      uint32_t *hysteresis_cells) override;

  bf_status_t bfTMPPGAppPoolUsageGetDefault(
      bf_dev_id_t dev,
      bf_tm_ppg_hdl ppg_hdl,
      bf_tm_app_pool_t *pool,
      uint32_t *pool_max_cells,
      bf_tm_ppg_baf_t *dynamic_baf) override;

  /****** TM Pipe API *****/
  bf_status_t bfTMPipeIsValid(bf_dev_id_t dev, bf_dev_pipe_t pipe) override;
  bf_status_t bfTMPipeGetCount(bf_dev_id_t dev, uint8_t *count) override;
  bf_status_t bfTMPipeGetFirst(bf_dev_id_t dev, bf_dev_pipe_t *pipe) override;
  bf_status_t bfTMPipeGetNext(bf_dev_id_t dev,
                              bf_dev_pipe_t pipe,
                              bf_dev_pipe_t *pipe_next) override;

  bf_status_t bfTMPipeGetPortCount(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint16_t *count) override;
  bf_status_t bfTMPipeGetPortFirst(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_dev_port_t *dev_port) override;
  bf_status_t bfTMPipeGetPortNext(bf_dev_id_t dev,
                                  bf_dev_port_t dev_port,
                                  bf_dev_port_t *dev_port_next) override;

  bf_status_t bfTMPipeGetPortGroupBasePort(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_tm_pg_t pg_id,
                                           bf_dev_port_t *dev_port) override;

  bf_status_t bfTMPipeMirrorOnDropDestGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bf_dev_port_t *port,
                                          bf_tm_queue_t *queue) override;
  bf_status_t bfTMPipeMirrorOnDropDestDefaultGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_dev_port_t *port,
                                                 bf_tm_queue_t *queue) override;
  bf_status_t bfTMPipeMirrorOnDropDestSet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue) override;

  bf_status_t bfTMPipeSchedPktIfgCompDefaultGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint8_t *adjustment) override;
  bf_status_t bfTMPipeSchedPktIfgCompGet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         uint8_t *adjustment) override;
  bf_status_t bfTMPipeSchedPktIfgCompSet(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         uint8_t adjustment) override;

  bf_status_t bfTMPipeSchedAdvFcModeDefaultGet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bool *enable) override;
  bf_status_t bfTMPipeSchedAdvFcModeGet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bool *enable) override;
  bf_status_t bfTMPipeSchedAdvFcModeSet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bool enable) override;

  bf_status_t bfTMPipeQstatReportDefaultGet(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool *enable_any) override;
  bf_status_t bfTMPipeQstatReportGet(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bool *enable_any) override;
  bf_status_t bfTMPipeQstatReportSet(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bool enable_any) override;

  bf_status_t bfTMPipeEgHysteresisDefaultGet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint32_t *cells) override;
  bf_status_t bfTMPipeEgHysteresisGet(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t *cells) override;
  bf_status_t bfTMPipeEgHysteresisSet(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t cells) override;

  bf_status_t bfTMPipeEgLimitDefaultGet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint32_t *cells) override;
  bf_status_t bfTMPipeEgLimitGet(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 uint32_t *cells) override;
  bf_status_t bfTMPipeEgLimitSet(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 uint32_t cells) override;

  bf_status_t bfTMPipeMirrorDropEnableGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bool *enable) override;
  bf_status_t bfTMPipeMirrorDropEnableDefaultGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bool *enable) override;
  bf_status_t bfTMPipeMirrorDropEnableSet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bool enable) override;
  /****** TM Port API *****/
  bf_status_t bfTMPortIsValid(bf_dev_id_t dev, bf_dev_port_t dev_port) override;

  bf_status_t bfTMPortStatusGet(bf_dev_id_t dev,
                                bf_dev_port_t port_id,
                                bool *is_offline,
                                bool *is_enabled,
                                bool *qac_rx_enable,
                                bool *recirc_enable,
                                bool *has_mac) override;

  bf_status_t bfTMPortIcosCntGet(bf_dev_id_t dev,
                                 bf_dev_port_t port_id,
                                 uint8_t *icos_count) override;

  bf_status_t bfTMPortIcosMapDefaultsGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint8_t *icos_mask) override;

  bf_status_t bfTMPortPPGMapGet(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                uint8_t *ppg_mask,
                                bf_tm_ppg_hdl *ppg_hdlrs) override;

  bf_status_t bfTMPortBaseQueueGet(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bf_tm_pg_t *pg_id,
                                   uint8_t *pg_queue,
                                   bool *is_mapped) override;

  bf_status_t bfTMPortQMappingGet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  uint8_t *q_count,
                                  uint8_t *q_mapping);
  bf_status_t bfTMPortQMappingSet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  uint8_t q_count,
                                  uint8_t *q_mapping) override;

  bf_status_t bfTMPortCreditsGet(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 uint32_t *pkt_credits) override;

  bf_status_t bfTMPortBufferDefaultsGet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bool *ct_enable,
                                        uint8_t *uc_ct_limit_cells,
                                        uint32_t *ig_limit_cells,
                                        uint32_t *ig_hysteresis_cells,
                                        uint32_t *eg_limit_cells,
                                        uint32_t *eg_hysteresis_cells,
                                        uint32_t *skid_limit_cells) override;

  bf_status_t bfTMPortCutThroughGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool *ct_enable) override;

  bf_status_t bfTMPortCutThroughLimitGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint8_t *uc_ct_limit_cells) override;

  bf_status_t bfTMPortBufIgLimitGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint32_t *ig_limit_cells) override;

  bf_status_t bfTMPortBufIgHysteresisGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      uint32_t *ig_hysteresis_cells) override;

  bf_status_t bfTMPortBufEgLimitGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint32_t *eg_limit_cells) override;

  bf_status_t bfTMPortBufEgHysteresisGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      uint32_t *eg_hysteresis_cells) override;

  bf_status_t bfTMPortBufSkidLimitGet(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t *skid_limit_cells) override;

  bf_status_t bfTMPortCutThroughLimitSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint8_t uc_ct_limit_cells) override;

  bf_status_t bfTMPortBufIgLimitSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint32_t ig_limit_cells) override;

  bf_status_t bfTMPortBufIgHysteresisSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint32_t ig_hysteresis_cells) override;

  bf_status_t bfTMPortBufEgLimitSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint32_t eg_limit_cells) override;

  bf_status_t bfTMPortBufEgHysteresisSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint32_t eg_hysteresis_cells) override;

  bf_status_t bfTMPortBufSkidLimitSet(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t skid_limit_cells) override;

  bf_status_t bfTMPortFlowCtrlDefaultsGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_flow_ctrl_type_t *mode_tx,
                                          bf_tm_flow_ctrl_type_t *mode_rx,
                                          uint8_t *cos_map) override;

  bf_status_t bfTMPortFlowCtrlTxGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_flow_ctrl_type_t *mode_tx) override;

  bf_status_t bfTMPortFlowCtrlRxGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_flow_ctrl_type_t *mode_rx) override;

  bf_status_t bfTMPortCosMappingGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint8_t *cos_map) override;

  bf_status_t bfTMPortFlowCtrlTxSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_flow_ctrl_type_t mode_tx) override;

  bf_status_t bfTMPortFlowCtrlRxSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_flow_ctrl_type_t mode_rx) override;

  bf_status_t bfTMPortCosMappingSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    uint8_t *cos_map) override;

  //---- Port Scheduling
  bf_status_t bfTMPortSchedEnable(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_port_speeds_t speed) override;
  bf_status_t bfTMPortSchedDisable(bf_dev_id_t dev,
                                   bf_dev_port_t port) override;
  //---
  bf_status_t bfTMPortSchedSpeedGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_port_speeds_t *speed) override;
  bf_status_t bfTMPortSchedSpeedResetGet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_port_speeds_t *speed) override;
  //---
  bf_status_t bfTMPortSchedShapingEnableDefaultGet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bool *enable) override;
  bf_status_t bfTMPortSchedShapingEnableGet(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bool *enable) override;
  bf_status_t bfTMPortSchedShapingEnable(bf_dev_id_t dev,
                                         bf_dev_port_t port) override;
  bf_status_t bfTMPortSchedShapingDisable(bf_dev_id_t dev,
                                          bf_dev_port_t port) override;
  //---
  bf_status_t bfTMPortSchedMaxRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) override;
  bf_status_t bfTMPortSchedMaxRateGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) override;
  bf_status_t bfTMPortSchedMaxRateSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bool pps,
      uint32_t burst_size,
      uint32_t rate,
      bf_tm_sched_shaper_prov_type_t prov_type) override;

  /****** TM Port Group API ******/
  bf_status_t bfTMPortGroupGet(bf_dev_id_t dev,
                               bf_dev_port_t port_id,
                               bf_tm_pg_t *pg_id,
                               uint8_t *pg_port_nr) override;

  bf_status_t bfTMPortGroupPortQueueGet(const bf_rt_target_t &dev_tgt,
                                        bf_tm_pg_t pg_id,
                                        uint8_t pg_queue,
                                        bf_dev_port_t *port,
                                        bf_tm_queue_t *queue_nr,
                                        bool *is_mapped) override;

  /****** TM Queue API *****/
  bf_status_t bfTMQueuePfcCosGet(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 uint8_t *cos) override;
  bf_status_t bfTMQueuePfcCosSet(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 uint8_t cos) override;
  bf_status_t bfTMQueueColorDropGet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bool *is_enabled) override;
  bf_status_t bfTMQueueColorDropSet(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bool enable) override;

  bf_status_t bfTMQueueVisibleGet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bool *is_visible) override;
  bf_status_t bfTMQueueVisibleSet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bool is_visible) override;
  bf_status_t bfTMQueueVisibleDefaultsGet(bf_dev_id_t dev,
                                          bool *is_visible) override;

  bf_status_t bfTMQueueColorDropLimitGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_color_t color,
      bf_tm_queue_color_limit_t *limit) override;
  bf_status_t bfTMQueueColorDropLimitSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_color_t color,
      bf_tm_queue_color_limit_t limit) override;
  bf_status_t bfTMQueueColorHysteresisGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_color_t color,
                                          bf_tm_thres_t *hyst_cells) override;
  bf_status_t bfTMQueueColorHysteresisSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_color_t color,
                                          bf_tm_thres_t hyst_cells) override;

  bf_status_t bfTMQueueGuaranteedCellsGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint32_t *cells) override;
  bf_status_t bfTMQueueGuaranteedCellsSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint32_t cells) override;

  bf_status_t bfTMQueueTailDropGet(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bool *is_enabled) override;
  bf_status_t bfTMQueueTailDropSet(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bool enable) override;

  bf_status_t bfTMQueueHysteresisGet(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue,
                                     uint32_t *hyst_cells) override;
  bf_status_t bfTMQueueHysteresisSet(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue,
                                     uint32_t cells) override;

  bf_status_t bfTMQueueAppPoolGet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bf_tm_app_pool_t *pool,
                                  uint32_t *base_use_limit,
                                  bf_tm_queue_baf_t *dynamic_baf,
                                  uint32_t *hyst_cells) override;
  bf_status_t bfTMQueueAppPoolSet(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bf_tm_app_pool_t pool,
                                  uint32_t base_use_limit,
                                  bf_tm_queue_baf_t dynamic_baf,
                                  uint32_t hyst_cells) override;

  bf_status_t bfTMQueueAppPoolLimitGet(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       uint32_t *cells) override;

  bf_status_t bfTMQueueAppPoolDisable(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_queue_t queue) override;

  bf_status_t bfTMQueueCfgDefaultsGet(bf_dev_id_t dev,
                                      uint8_t *pfc_cos) override;

  bf_status_t bfTMQueueColorDefaultsGet(
      bf_dev_id_t dev,
      bool *drop_enable,
      bf_tm_queue_color_limit_t *yellow_drop_limit,
      bf_tm_thres_t *hyst_yellow,
      bf_tm_queue_color_limit_t *red_drop_limit,
      bf_tm_thres_t *hyst_red) override;

  bf_status_t bfTMQueueBufferDefaultsGet(
      bf_dev_id_t dev,
      uint32_t *guaranteed_cells,
      uint32_t *hysteresis_cells,
      bool *tail_drop_enable,
      bf_tm_app_pool_t *pool_id,
      uint32_t *pool_max_cells,
      bf_tm_queue_baf_t *dynamic_baf) override;

  //--- Queue Scheduling
  bf_status_t bfTMQueueSchedEnableGet(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_queue_t queue,
                                      bool *enable) override;
  bf_status_t bfTMQueueSchedEnableDefaultGet(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             bool *enable) override;
  bf_status_t bfTMQueueSchedEnable(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue) override;
  bf_status_t bfTMQueueSchedDisable(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue) override;

  bf_status_t bfTMQueueSchedSpeedGet(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue,
                                     bf_port_speeds_t *speed) override;

  bf_status_t bfTMQueueSchedGuaranteedEnableGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool *enable) override;
  bf_status_t bfTMQueueSchedGuaranteedEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *enable,
      bf_tm_sched_prio_t *priority) override;
  bf_status_t bfTMQueueSchedGuaranteedEnable(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue) override;
  bf_status_t bfTMQueueSchedGuaranteedDisable(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue) override;
  bf_status_t bfTMQueueSchedGuaranteedPriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t *priority) override;
  bf_status_t bfTMQueueSchedGuaranteedPrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t priority) override;

  bf_status_t bfTMQueueSchedShapingEnableGet(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             bool *enable) override;
  bf_status_t bfTMQueueSchedShapingEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *enable,
      bf_tm_sched_prio_t *priority) override;
  bf_status_t bfTMQueueSchedShapingEnable(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue) override;
  bf_status_t bfTMQueueSchedShapingDisable(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue) override;
  bf_status_t bfTMQueueSchedRemainingBwPriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t *priority) override;
  bf_status_t bfTMQueueSchedRemainingBwPrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_prio_t priority) override;

  bf_status_t bfTMQueueSchedDwrrWeightGet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint16_t *weight) override;
  bf_status_t bfTMQueueSchedDwrrWeightDefaultGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_queue_t queue,
                                                 uint16_t *weight) override;
  bf_status_t bfTMQueueSchedDwrrWeightSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint16_t weight) override;

  bf_status_t bfTMQueueSchedAdvFcModeGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t *mode) override;
  bf_status_t bfTMQueueSchedAdvFcModeDefaultGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t *mode) override;
  bf_status_t bfTMQueueSchedAdvFcModeSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bf_tm_sched_adv_fc_mode_t mode) override;

  bf_status_t bfTMQueueSchedMaxRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) override;

  bf_status_t bfTMQueueSchedMaxRateGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) override;
  bf_status_t bfTMQueueSchedMaxRateSet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool pps,
      uint32_t burst_size,
      uint32_t rate,
      bf_tm_sched_shaper_prov_type_t prov_type) override;
  bf_status_t bfTMQueueSchedMinRateDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_queue_t queue,
      bool *pps,
      uint32_t *burst_size,
      uint32_t *rate,
      bf_tm_sched_shaper_prov_type_t *prov_type) override;
  bf_status_t bfTMQueueSchedMinRateGet(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bool *pps,
                                       uint32_t *burst_size,
                                       uint32_t *rate) override;
  bf_status_t bfTMQueueSchedMinRateSet(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bool pps,
                                       uint32_t burst_size,
                                       uint32_t rate) override;

  //--------------------- TM L1 Node API
  bf_status_t bfTML1NodePortAssignmentGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bf_tm_pg_t pg_id,
                                          bf_tm_l1_node_t l1_node,
                                          bool *in_use,
                                          uint8_t *pg_port_nr,
                                          bf_dev_port_t *port_id) override;

  bf_status_t bfTML1NodePortAssignmentDefaultGet(
      bf_dev_id_t dev,
      bf_dev_pipe_t pipe,
      bf_tm_pg_t pg_id,
      bf_tm_l1_node_t l1_node,
      bool *in_use,
      uint8_t *pg_port_nr,
      bf_dev_port_t *port_id) override;

  bf_status_t bfTML1NodeQueueAssignmentGet(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_tm_pg_t pg_id,
                                           bf_tm_l1_node_t l1_node,
                                           uint8_t *l1_queues_cnt,
                                           bf_tm_queue_t *l1_queues) override;

  bf_status_t bfTMPortL1NodeAssignmentGet(bf_dev_id_t dev_id,
                                          bf_dev_port_t port_id,
                                          bool in_use_only,
                                          uint8_t *l1_nodes_cnt,
                                          bf_tm_l1_node_t *l1_nodes) override;

  bf_status_t bfTMQueueL1NodeSet(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTMQueueL1NodeGet(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 bf_tm_l1_node_t *l1_node) override;

  bf_status_t bfTMQueueL1NodeDefaultGet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_queue_t queue,
                                        bf_tm_l1_node_t *l1_node) override;

  bf_status_t bfTML1NodeFree(bf_dev_id_t dev,
                             bf_dev_port_t port,
                             bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedEnableGet(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_l1_node_t l1_node,
                                       bool *enable) override;

  bf_status_t bfTML1NodeSchedEnable(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedDisable(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_l1_node_t l1_node) override;
  //---
  bf_status_t bfTML1NodeSchedMinRateEnableGet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_l1_node_t l1_node,
                                              bool *enable) override;

  bf_status_t bfTML1NodeSchedMinRateEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *enable,
      bf_tm_sched_prio_t *priority) override;

  bf_status_t bfTML1NodeSchedMinRateEnable(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedMinRateDisable(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedMinRatePriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t *priority) override;

  bf_status_t bfTML1NodeSchedMinRatePrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t priority) override;

  //---
  bf_status_t bfTML1NodeSchedMaxRateEnableGet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_l1_node_t l1_node,
                                              bool *enable) override;

  bf_status_t bfTML1NodeSchedMaxRateEnableDefaultsGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *enable,
      bf_tm_sched_prio_t *priority) override;

  bf_status_t bfTML1NodeSchedMaxRateEnable(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedMaxRateDisable(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedMaxRatePriorityGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t *priority) override;

  bf_status_t bfTML1NodeSchedMaxRatePrioritySet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bf_tm_sched_prio_t priority) override;

  bf_status_t bfTML1NodeSchedDwrrWeightGet(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node,
                                           uint16_t *weight) override;

  bf_status_t bfTML1NodeSchedDwrrWeightDefaultGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_l1_node_t l1_node,
                                                  uint16_t *weight) override;

  bf_status_t bfTML1NodeSchedDwrrWeightSet(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node,
                                           uint16_t weight) override;
  //---
  bf_status_t bfTML1NodeSchedPriorityPropagationGet(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *is_enabled) override;

  bf_status_t bfTML1NodeSchedPriorityPropagationDefaultGet(
      bf_dev_id_t dev,
      bf_dev_port_t port,
      bf_tm_l1_node_t l1_node,
      bool *is_enabled) override;

  bf_status_t bfTML1NodeSchedPriorityPropagationEnable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) override;

  bf_status_t bfTML1NodeSchedPriorityPropagationDisable(
      bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) override;

  //---
  bf_status_t bfTML1NodeSchedMaxRateDefaultsGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool *pps,
                                                uint32_t *burst_size,
                                                uint32_t *rate) override;

  bf_status_t bfTML1NodeSchedMaxRateGet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bool *pps,
                                        uint32_t *burst_size,
                                        uint32_t *rate) override;

  bf_status_t bfTML1NodeSchedMaxRateSet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bool pps,
                                        uint32_t burst_size,
                                        uint32_t rate) override;

  //---
  bf_status_t bfTML1NodeSchedMinRateDefaultsGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node,
                                                bool *pps,
                                                uint32_t *burst_size,
                                                uint32_t *rate) override;

  bf_status_t bfTML1NodeSchedMinRateGet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bool *pps,
                                        uint32_t *burst_size,
                                        uint32_t *rate) override;

  bf_status_t bfTML1NodeSchedMinRateSet(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bool pps,
                                        uint32_t burst_size,
                                        uint32_t rate) override;

  /***** Pool Config APIs *******/
  bf_status_t bfTmAppPoolSizeSet(bf_dev_id_t dev,
                                 bf_tm_app_pool_t pool,
                                 uint32_t cells) override;

  bf_status_t bfTmAppPoolSizeGet(bf_dev_id_t dev,
                                 bf_tm_app_pool_t pool,
                                 uint32_t *cells) override;

  bf_status_t bfTmPoolSkidSizeSet(bf_dev_id_t dev, uint32_t cells) override;

  bf_status_t bfTmPoolMirrorOnDropSizeSet(bf_dev_id_t dev,
                                          uint32_t cells) override;

  bf_status_t bfTmPreFifoLimitSet(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  uint8_t fifo,
                                  uint32_t cells) override;

  bf_status_t bfTmGlobalMinLimitSet(bf_dev_id_t dev, uint32_t cells) override;

  bf_status_t bfTmPoolUcCutThroughSizeSet(bf_dev_id_t dev,
                                          uint32_t cells) override;

  bf_status_t bfTmPoolMcCutThroughSizeSet(bf_dev_id_t dev,
                                          uint32_t cells) override;

  bf_status_t bfTmPoolSkidSizeGet(bf_dev_id_t dev, uint32_t *cells) override;

  bf_status_t bfTmPoolMirrorOnDropSizeGet(bf_dev_id_t dev,
                                          uint32_t *cells) override;

  bf_status_t bfTmPreFifoLimitGet(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  uint8_t fifo,
                                  uint32_t *cells) override;

  bf_status_t bfTmGlobalMinLimitGet(bf_dev_id_t dev, uint32_t *cells) override;

  bf_status_t bfTmPoolUcCutThroughSizeGet(bf_dev_id_t dev,
                                          uint32_t *cells) override;

  bf_status_t bfTmPoolMcCutThroughSizeGet(bf_dev_id_t dev,
                                          uint32_t *cells) override;

  /***** Skid Pool APIs *******/
  bf_status_t bfTmSkidPoolHysteresisSet(bf_dev_id_t dev,
                                        uint32_t cells) override;

  bf_status_t bfTmSkidPoolHysteresisGet(bf_dev_id_t dev,
                                        uint32_t *cells) override;

  bf_status_t bfTmSkidPoolHysteresisGetDefault(bf_dev_id_t dev,
                                               uint32_t *cells) override;

  /***** App Pool APIs *******/
  bf_status_t bfTmPoolColorDropEnable(bf_dev_id_t dev,
                                      bf_tm_app_pool_t pool) override;

  bf_status_t bfTmPoolColorDropDisable(bf_dev_id_t dev,
                                       bf_tm_app_pool_t pool) override;

  bf_status_t bfTmAppPoolColorDropLimitSet(bf_dev_id_t dev,
                                           bf_tm_app_pool_t pool,
                                           bf_tm_color_t color,
                                           uint32_t limit) override;

  bf_status_t bfTmPoolColorDropLimitGet(bf_dev_id_t dev,
                                        bf_tm_app_pool_t pool,
                                        bf_tm_color_t color,
                                        uint32_t *limit) override;

  bf_status_t bfTmPoolColorDropStateGet(bf_dev_id_t dev,
                                        bf_tm_app_pool_t pool,
                                        bool *drop_state) override;

  /***** Pool color APIs *******/
  bf_status_t bfTmPoolColorDropHysteresisSet(bf_dev_id_t dev,
                                             bf_tm_color_t color,
                                             uint32_t limit) override;

  bf_status_t bfTmPoolColorDropHysteresisGet(bf_dev_id_t dev,
                                             bf_tm_color_t color,
                                             uint32_t *limit) override;

  bf_status_t bfTmPoolColorDropHysteresisDefaultGet(bf_dev_id_t dev,
                                                    bf_tm_color_t color,
                                                    uint32_t *limit) override;

  /***** App Pool PFC APIs *******/
  bf_status_t bfTmPoolPfcLimitSet(bf_dev_id_t dev,
                                  bf_tm_app_pool_t pool,
                                  bf_tm_icos_t icos,
                                  uint32_t limit) override;

  bf_status_t bfTmPoolPfcLimitGet(bf_dev_id_t dev,
                                  bf_tm_app_pool_t pool,
                                  bf_tm_icos_t icos,
                                  uint32_t *limit) override;

  bf_status_t bfTmPoolPfcLimitGetDefault(bf_dev_id_t dev,
                                         bf_tm_app_pool_t pool,
                                         bf_tm_icos_t icos,
                                         uint32_t *limit) override;

  bf_status_t bfTmMaxPfcLevelsGet(bf_dev_id_t dev, uint32_t *levels) override;

  /***** Port counter APIs *******/
  bf_status_t bfTmPortDropGetCached(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    bf_dev_port_t port,
                                    uint64_t *ig_count,
                                    uint64_t *eg_count) override;

  bf_status_t bfTmPortUsageGet(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               bf_dev_port_t port,
                               uint32_t *ig_count,
                               uint32_t *eg_count,
                               uint32_t *ig_wm,
                               uint32_t *eg_wm) override;

  bf_status_t bfTmPortIngressWatermarkClear(bf_dev_id_t dev,
                                            bf_dev_port_t port) override;

  bf_status_t bfTmPortEgressWatermarkClear(bf_dev_id_t dev,
                                           bf_dev_port_t port) override;

  bf_status_t bfTmPortDropIngressCacheSet(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          uint64_t drop_count) override;

  bf_status_t bfTmPortDropEgressCacheSet(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint64_t drop_count) override;
  /***** Queue counter APIs *******/
  bf_status_t bfTmQDropGetCached(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 uint64_t *count) override;

  bf_status_t bfTmQDropCacheSet(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_dev_port_t port,
                                bf_tm_queue_t queue,
                                uint64_t count) override;

  bf_status_t bfTmQUsageGet(bf_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t *count,
                            uint32_t *wm) override;

  bf_status_t bfTmQWatermarkClear(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue) override;

  /***** Pool counter APIs *******/
  bf_status_t bfTmPoolUsageGet(bf_dev_id_t dev,
                               bf_tm_app_pool_t pool,
                               uint32_t *count,
                               uint32_t *wm) override;

  bf_status_t bfTmPoolWatermarkClear(bf_dev_id_t dev,
                                     bf_tm_app_pool_t pool) override;

  /***** Pipe counter APIs *******/
  bf_status_t bfTmPipeBufferFullDropGet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint64_t *count) override;

  bf_status_t bfTmQDiscardUsageGet(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint32_t *count,
                                   uint32_t *wm) override;

  bf_status_t bfTmPipeCountersGet(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  uint64_t *cell_count,
                                  uint64_t *pkt_count) override;

  bf_status_t bfTmCutThroughCountersGet(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint64_t *uc_count,
                                        uint64_t *mc_count) override;

  bf_status_t bfTmBlklvlDropGet(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_tm_blklvl_cntrs_t *blk_cntrs) override;

  bf_status_t bfTmPreFifoDropGet(bf_dev_id_t dev,
                                 bf_tm_pre_fifo_cntrs_t *fifo_cntrs) override;

  bf_status_t bfTmPipeBufferFullDropClear(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe) override;

  bf_status_t bfTmQDiscardWatermarkClear(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe) override;

  bf_status_t bfTmPipeClearCellCounter(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe) override;

  bf_status_t bfTmPipeClearPacketCounter(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe) override;

  bf_status_t bfTmPipeClearUcCtPacketCounter(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe) override;

  bf_status_t bfTmPipeClearMcCtPacketCounter(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe) override;

  bf_status_t bfTmBlklvlDropClear(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  uint32_t clear_mask) override;

  bf_status_t bfTmPreFifoDropClear(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint32_t fifo) override;

  /***** PPG Counter APIs *******/
  bf_status_t bfTmPpgDropGet(bf_dev_id_t dev,
                             bf_dev_pipe_t pipe,
                             bf_tm_ppg_hdl ppg,
                             uint64_t *count) override;

  bf_status_t bfTmPpgDropGetCached(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_tm_ppg_hdl ppg,
                                   uint64_t *count) override;

  bf_status_t bfTmPpgDropCacheSet(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  bf_tm_ppg_hdl ppg,
                                  uint64_t count) override;

  bf_status_t bfTmPpgUsageGet(bf_dev_id_t dev,
                              bf_dev_pipe_t pipe,
                              bf_tm_ppg_hdl ppg,
                              uint32_t *gmin_count,
                              uint32_t *shared_count,
                              uint32_t *skid_count,
                              uint32_t *wm) override;

  bf_status_t bfTmPpgWatermarkClear(bf_dev_id_t dev,
                                    bf_tm_ppg_hdl ppg) override;

  /***** TM Cfg APIs *******/
  bf_status_t bfTmTimestampShiftSet(bf_dev_id_t dev, uint8_t shift) override;

  bf_status_t bfTmTimestampShiftGet(bf_dev_id_t dev, uint8_t *shift) override;

  bf_status_t bfTmTimestampShiftGetDefault(bf_dev_id_t dev,
                                           uint8_t *shift) override;

  bf_status_t bfTmCellSizeInBytesGet(bf_dev_id_t dev, uint32_t *bytes) override;

  bf_status_t bfTmTotalCellCountGet(bf_dev_id_t dev,
                                    uint32_t *total_cells) override;

  bf_status_t bfTmIngressBufferLimitSet(bf_dev_id_t dev,
                                        uint32_t cells) override;

  bf_status_t bfTmIngressBufferLimitGet(bf_dev_id_t dev,
                                        uint32_t *cells) override;

  bf_status_t bfTmIngressBufferLimitGetDefault(bf_dev_id_t dev,
                                               uint32_t *cells) override;

  bf_status_t bfTmIngressBufferLimitEnable(bf_dev_id_t dev) override;

  bf_status_t bfTmIngressBufferLimitDisable(bf_dev_id_t dev) override;

  bf_status_t bfTmIngressBufferLimitStateGet(bf_dev_id_t dev,
                                             bool *state) override;

  bf_status_t bfTmIngressBufferLimitStateGetDefault(bf_dev_id_t dev,
                                                    bool *state) override;

  /***** TM Pipe Multicast fifo APIs *******/
  bf_status_t bfTmMcFifoIcosMappingSet(bf_dev_id_t dev,
                                       uint8_t pipe_bmap,
                                       int fifo,
                                       uint8_t icos_bmap) override;

  bf_status_t bfTmMcFifoIcosMappingGet(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       int fifo,
                                       uint8_t *icos_bmap) override;

  bf_status_t bfTmMcFifoIcosMappingGetDefault(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              int fifo,
                                              uint8_t *icos_bmap) override;

  bf_status_t bfTmMcFifoArbModeSet(bf_dev_id_t dev,
                                   uint8_t pipe_bmap,
                                   int fifo,
                                   bool use_strict_pri) override;

  bf_status_t bfTmMcFifoArbModeGet(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   int fifo,
                                   bool *use_strict_pri) override;

  bf_status_t bfTmMcFifoArbModeGetDefault(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          int fifo,
                                          bool *use_strict_pri) override;

  bf_status_t bfTmMcFifoWrrWeightSet(bf_dev_id_t dev,
                                     uint8_t pipe_bmap,
                                     int fifo,
                                     uint8_t weight) override;

  bf_status_t bfTmMcFifoWrrWeightGet(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     int fifo,
                                     uint8_t *weight) override;

  bf_status_t bfTmMcFifoWrrWeightGetDefault(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            int fifo,
                                            uint8_t *weight) override;

  bf_status_t bfTmMcFifoDepthSet(bf_dev_id_t dev,
                                 uint8_t pipe_bmap,
                                 int fifo,
                                 int size) override;

  bf_status_t bfTmMcFifoDepthGet(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 int fifo,
                                 int *size) override;

  bf_status_t bfTmMcFifoDepthGetDefault(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        int fifo,
                                        int *size) override;
};

}  // namespace bfrt

#endif  // _BF_RT_TRAFFIC_MGR_INTERFACE_HPP
