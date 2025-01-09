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


/* bf_rt_includes */
#include "bf_rt_tm_intf.hpp"
#include <bf_rt/bf_rt_info.hpp>

namespace bfrt {
std::unique_ptr<ITrafficMgrIntf> ITrafficMgrIntf::instance = nullptr;
std::mutex ITrafficMgrIntf::tm_mgr_intf_mtx;

/*** TM control APIs. ***/
bf_status_t TrafficMgrIntf::bfTMCompleteOperations(bf_dev_id_t dev) {
  return bf_tm_complete_operations(dev);
}

/********Port Priority Group APIs*******/
bf_status_t TrafficMgrIntf::bfTMPPGAllocate(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_ppg_hdl *ppg) {
  return bf_tm_ppg_allocate(dev, port, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGFree(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_free(dev, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGDefaultPpgGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_ppg_hdl *ppg) {
  return bf_tm_ppg_defaultppg_get(dev, port, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGIcosMappingSet(bf_dev_id_t dev,
                                                  bf_tm_ppg_hdl ppg,
                                                  uint8_t icos_bmap) {
  return bf_tm_ppg_icos_mapping_set(dev, ppg, icos_bmap);
}

bf_status_t TrafficMgrIntf::bfTMPPGLosslessTreatmentEnable(bf_dev_id_t dev,
                                                           bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_lossless_treatment_enable(dev, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGLosslessTreatmentDisable(bf_dev_id_t dev,
                                                            bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_lossless_treatment_disable(dev, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGAppPoolUsageSet(bf_dev_id_t dev,
                                                   bf_tm_ppg_hdl ppg,
                                                   bf_tm_app_pool_t pool,
                                                   uint32_t base_use_limit,
                                                   bf_tm_ppg_baf_t dynamic_baf,
                                                   uint32_t hysteresis) {
  return bf_tm_ppg_app_pool_usage_set(
      dev, ppg, pool, base_use_limit, dynamic_baf, hysteresis);
}

bf_status_t TrafficMgrIntf::bfTMPPGAppPoolUsageDisable(bf_dev_id_t dev,
                                                       bf_tm_app_pool_t pool,
                                                       bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_app_pool_usage_disable(dev, pool, ppg);
}

bf_status_t TrafficMgrIntf::bfTMPPGGuaranteedMinLimitSet(bf_dev_id_t dev,
                                                         bf_tm_ppg_hdl ppg,
                                                         uint32_t cells) {
  return bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGSkidLimitSet(bf_dev_id_t dev,
                                                bf_tm_ppg_hdl ppg,
                                                uint32_t cells) {
  return bf_tm_ppg_skid_limit_set(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGGuaranteedMinSkidHysteresisSet(
    bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t cells) {
  return bf_tm_ppg_guaranteed_min_skid_hysteresis_set(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGAppPoolIdGet(bf_dev_id_t dev,
                                                bf_tm_ppg_hdl ppg,
                                                uint32_t *pool) {
  return bf_tm_ppg_app_pool_id_get(dev, ppg, pool);
}

bf_status_t TrafficMgrIntf::bfTMPPGAppPoolUsageGet(bf_dev_id_t dev,
                                                   bf_tm_ppg_hdl ppg,
                                                   bf_tm_app_pool_t pool,
                                                   uint32_t *base_use_limit,
                                                   bf_tm_ppg_baf_t *dynamic_baf,
                                                   uint32_t *hysteresis) {
  return bf_tm_ppg_app_pool_usage_get(
      dev, ppg, pool, base_use_limit, dynamic_baf, hysteresis);
}

bf_status_t TrafficMgrIntf::bfTMPPGuaranteedMinLimitGet(bf_dev_id_t dev,
                                                        bf_tm_ppg_hdl ppg,
                                                        uint32_t *cells) {
  return bf_tm_ppg_guaranteed_min_limit_get(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGLosslessTreatmentGet(bf_dev_id_t dev,
                                                        bf_tm_ppg_hdl ppg,
                                                        bool *pfc_val) {
  return bf_tm_ppg_lossless_treatment_get(dev, ppg, pfc_val);
}

bf_status_t TrafficMgrIntf::bfTMPPGSkidLimitGet(bf_dev_id_t dev,
                                                bf_tm_ppg_hdl ppg,
                                                uint32_t *cells) {
  return bf_tm_ppg_skid_limit_get(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGGuaranteedMinSkidHysteresisGet(
    bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t *cells) {
  return bf_tm_ppg_guaranteed_min_skid_hysteresis_get(dev, ppg, cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGIcosMappingGet(bf_dev_id_t dev,
                                                  bf_tm_ppg_hdl ppg,
                                                  uint8_t *icos_bmap) {
  return bf_tm_ppg_icos_mapping_get(dev, ppg, icos_bmap);
}

//---
bf_status_t TrafficMgrIntf::bfTMPPGTotalCntGet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint32_t *total_cnt) {
  return bf_tm_ppg_totalppg_get(dev, pipe, total_cnt);
}

bf_status_t TrafficMgrIntf::bfTMPPGUnusedCntGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t *unused_cnt) {
  return bf_tm_ppg_unusedppg_get(dev, pipe, unused_cnt);
}

bf_status_t TrafficMgrIntf::bfTMPPGPortGet(bf_dev_id_t dev,
                                           bf_tm_ppg_hdl ppg_hdl,
                                           bf_dev_port_t *port_id) {
  return bf_tm_ppg_port_get(dev, ppg_hdl, port_id);
}

bf_status_t TrafficMgrIntf::bfTMPPGNrGet(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg_hdl,
                                         bf_tm_ppg_id_t *ppg_nr) {
  return bf_tm_ppg_nr_get(dev, ppg_hdl, ppg_nr);
}

bf_status_t TrafficMgrIntf::bfTMPPGMirrorPortHandleGet(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bf_tm_ppg_hdl *ppg_hdl) {
  return bf_tm_ppg_mirror_port_handle_get(dev, pipe, ppg_hdl);
}

//----
bf_status_t TrafficMgrIntf::bfTMPPGBufferGetDefault(
    bf_dev_id_t dev,
    bf_tm_ppg_hdl ppg_hdl,
    uint32_t *min_limit_cells,
    uint32_t *hysteresis_cells) {
  return bf_tm_ppg_buffer_get_default(
      dev, ppg_hdl, min_limit_cells, hysteresis_cells);
}

bf_status_t TrafficMgrIntf::bfTMPPGAppPoolUsageGetDefault(
    bf_dev_id_t dev,
    bf_tm_ppg_hdl ppg_hdl,
    bf_tm_app_pool_t *pool,
    uint32_t *pool_max_cells,
    bf_tm_ppg_baf_t *dynamic_baf) {
  return bf_tm_ppg_app_pool_usage_get_default(
      dev, ppg_hdl, pool, pool_max_cells, dynamic_baf);
}

//--------------------- TM Device API

bf_status_t TrafficMgrIntf::bfTMDevCfgGet(bf_dev_id_t dev,
                                          bf_tm_dev_cfg_t *cfg) {
  return bf_tm_dev_config_get(dev, cfg);
}

//--------------------- TM Pipe API
bf_status_t TrafficMgrIntf::bfTMPipeIsValid(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe) {
  return bf_tm_pipe_is_valid(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetCount(bf_dev_id_t dev, uint8_t *count) {
  return bf_tm_pipe_get_count(dev, count);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetFirst(bf_dev_id_t dev,
                                             bf_dev_pipe_t *pipe) {
  return bf_tm_pipe_get_first(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetNext(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bf_dev_pipe_t *pipe_next) {
  return bf_tm_pipe_get_next(dev, pipe, pipe_next);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetPortCount(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint16_t *count) {
  return bf_tm_pipe_port_get_count(dev, pipe, true, count);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetPortFirst(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_dev_port_t *dev_port) {
  return bf_tm_pipe_port_get_first(dev, pipe, true, dev_port);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetPortNext(bf_dev_id_t dev,
                                                bf_dev_port_t dev_port,
                                                bf_dev_port_t *dev_port_next) {
  return bf_tm_pipe_port_get_next(dev, dev_port, true, dev_port_next);
}

bf_status_t TrafficMgrIntf::bfTMPipeGetPortGroupBasePort(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    bf_tm_pg_t pg_id,
    bf_dev_port_t *dev_port) {
  return bf_tm_pipe_port_group_get_first_port(dev, pipe, pg_id, dev_port);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorOnDropDestGet(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        bf_dev_port_t *port,
                                                        bf_tm_queue_t *queue) {
  return bf_tm_port_mirror_on_drop_dest_get(dev, pipe, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorOnDropDestDefaultGet(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    bf_dev_port_t *port,
    bf_tm_queue_t *queue) {
  return bf_tm_port_mirror_on_drop_dest_get_default(dev, pipe, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorOnDropDestSet(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue) {
  return bf_tm_port_mirror_on_drop_dest_set(dev, pipe, port, queue);
}

//----
bf_status_t TrafficMgrIntf::bfTMPipeSchedPktIfgCompDefaultGet(
    bf_dev_id_t dev, bf_dev_pipe_t pipe, uint8_t *adjustment) {
  return bf_tm_sched_pkt_ifg_compensation_get_default(dev, pipe, adjustment);
}

bf_status_t TrafficMgrIntf::bfTMPipeSchedPktIfgCompGet(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       uint8_t *adjustment) {
  return bf_tm_sched_pkt_ifg_compensation_get(dev, pipe, adjustment);
}

bf_status_t TrafficMgrIntf::bfTMPipeSchedPktIfgCompSet(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       uint8_t adjustment) {
  return bf_tm_sched_pkt_ifg_compensation_set(dev, pipe, adjustment);
}

//---
bf_status_t TrafficMgrIntf::bfTMPipeSchedAdvFcModeDefaultGet(bf_dev_id_t dev,
                                                             bf_dev_pipe_t pipe,
                                                             bool *enable) {
  return bf_tm_sched_adv_fc_mode_enable_get_default(dev, pipe, enable);
}

bf_status_t TrafficMgrIntf::bfTMPipeSchedAdvFcModeGet(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      bool *enable) {
  return bf_tm_sched_adv_fc_mode_enable_get(dev, pipe, enable);
}

bf_status_t TrafficMgrIntf::bfTMPipeSchedAdvFcModeSet(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      bool enable) {
  return bf_tm_sched_adv_fc_mode_enable_set(dev, pipe, enable);
}

//---
bf_status_t TrafficMgrIntf::bfTMPipeQstatReportDefaultGet(bf_dev_id_t dev,
                                                          bf_dev_pipe_t pipe,
                                                          bool *enable_any) {
  return bf_tm_qstat_report_mode_get_default(dev, pipe, enable_any);
}

bf_status_t TrafficMgrIntf::bfTMPipeQstatReportGet(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe,
                                                   bool *enable_any) {
  return bf_tm_qstat_report_mode_get(dev, pipe, enable_any);
}

bf_status_t TrafficMgrIntf::bfTMPipeQstatReportSet(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe,
                                                   bool enable_any) {
  return bf_tm_qstat_report_mode_set(dev, pipe, enable_any);
}

//---
bf_status_t TrafficMgrIntf::bfTMPipeEgHysteresisDefaultGet(bf_dev_id_t dev,
                                                           bf_dev_pipe_t pipe,
                                                           uint32_t *cells) {
  return bf_tm_pipe_egress_hysteresis_get_default(dev, pipe, cells);
}

bf_status_t TrafficMgrIntf::bfTMPipeEgHysteresisGet(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t *cells) {
  return bf_tm_pipe_egress_hysteresis_get(dev, pipe, cells);
}

bf_status_t TrafficMgrIntf::bfTMPipeEgHysteresisSet(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t cells) {
  return bf_tm_pipe_egress_hysteresis_set(dev, pipe, cells);
}

//---

bf_status_t TrafficMgrIntf::bfTMPipeEgLimitDefaultGet(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      uint32_t *cells) {
  return bf_tm_pipe_egress_limit_get_default(dev, pipe, cells);
}

bf_status_t TrafficMgrIntf::bfTMPipeEgLimitGet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint32_t *cells) {
  return bf_tm_pipe_egress_limit_get(dev, pipe, cells);
}

bf_status_t TrafficMgrIntf::bfTMPipeEgLimitSet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint32_t cells) {
  return bf_tm_pipe_egress_limit_set(dev, pipe, cells);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorDropEnableGet(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        bool *enable) {
  return bf_tm_pipe_deflection_port_enable_get(dev, pipe, enable);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorDropEnableDefaultGet(
    bf_dev_id_t dev, bf_dev_pipe_t pipe, bool *enable) {
  return bf_tm_pipe_deflection_port_enable_get_default(dev, pipe, enable);
}

bf_status_t TrafficMgrIntf::bfTMPipeMirrorDropEnableSet(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        bool enable) {
  return bf_tm_pipe_deflection_port_enable_set(dev, pipe, enable);
}

//--------------------- TM Queue API

bf_status_t TrafficMgrIntf::bfTMQueuePfcCosGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               uint8_t *cos) {
  return bf_tm_q_pfc_cos_mapping_get(dev, port, queue, cos);
}

bf_status_t TrafficMgrIntf::bfTMQueuePfcCosSet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               uint8_t cos) {
  return bf_tm_q_pfc_cos_mapping_set(dev, port, queue, cos);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueColorDropGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  bool *is_enabled) {
  return bf_tm_q_color_drop_get(dev, port, queue, is_enabled);
}

bf_status_t TrafficMgrIntf::bfTMQueueColorDropSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  bool enable) {
  return (enable) ? bf_tm_q_color_drop_enable(dev, port, queue)
                  : bf_tm_q_color_drop_disable(dev, port, queue);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueVisibleGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool *is_visible) {
  return bf_tm_q_visible_get(dev, port, queue, is_visible, is_visible);
}

bf_status_t TrafficMgrIntf::bfTMQueueVisibleSet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool is_visible) {
  return bf_tm_q_visible_set(dev, port, queue, is_visible);
}

bf_status_t TrafficMgrIntf::bfTMQueueVisibleDefaultsGet(bf_dev_id_t dev,
                                                        bool *is_visible) {
  return bf_tm_q_visible_get_default(dev, is_visible);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueColorDropLimitGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_color_t color,
    bf_tm_queue_color_limit_t *limit) {
  return bf_tm_q_color_limit_get(dev, port, queue, color, limit);
}

bf_status_t TrafficMgrIntf::bfTMQueueColorDropLimitSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_color_t color,
    bf_tm_queue_color_limit_t limit) {
  return bf_tm_q_color_limit_set(dev, port, queue, color, limit);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueColorHysteresisGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_color_t color,
    bf_tm_thres_t *hyst_cells) {
  return bf_tm_q_color_hysteresis_get(dev, port, queue, color, hyst_cells);
}

bf_status_t TrafficMgrIntf::bfTMQueueColorHysteresisSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_color_t color,
    bf_tm_thres_t hyst_cells) {
  return bf_tm_q_color_hysteresis_set(dev, port, queue, color, hyst_cells);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueGuaranteedCellsGet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue,
                                                        uint32_t *cells) {
  return bf_tm_q_guaranteed_min_limit_get(dev, port, queue, cells);
}

bf_status_t TrafficMgrIntf::bfTMQueueGuaranteedCellsSet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue,
                                                        uint32_t cells) {
  return bf_tm_q_guaranteed_min_limit_set(dev, port, queue, cells);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueTailDropGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_queue_t queue,
                                                 bool *is_enabled) {
  return bf_tm_q_tail_drop_get(dev, port, queue, is_enabled);
}

bf_status_t TrafficMgrIntf::bfTMQueueTailDropSet(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_queue_t queue,
                                                 bool enable) {
  return (enable) ? bf_tm_q_tail_drop_enable(dev, port, queue)
                  : bf_tm_q_tail_drop_disable(dev, port, queue);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueHysteresisGet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_queue_t queue,
                                                   uint32_t *hyst_cells) {
  return bf_tm_q_hysteresis_get(dev, port, queue, hyst_cells);
}

bf_status_t TrafficMgrIntf::bfTMQueueHysteresisSet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_queue_t queue,
                                                   uint32_t cells) {
  return bf_tm_q_hysteresis_set(dev, port, queue, cells);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueAppPoolGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bf_tm_app_pool_t *pool,
                                                uint32_t *base_use_limit,
                                                bf_tm_queue_baf_t *dynamic_baf,
                                                uint32_t *hyst_cells) {
  return bf_tm_q_app_pool_usage_get(
      dev, port, queue, pool, base_use_limit, dynamic_baf, hyst_cells);
}

bf_status_t TrafficMgrIntf::bfTMQueueAppPoolSet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bf_tm_app_pool_t pool,
                                                uint32_t base_use_limit,
                                                bf_tm_queue_baf_t dynamic_baf,
                                                uint32_t hyst_cells) {
  return bf_tm_q_app_pool_usage_set(
      dev, port, queue, pool, base_use_limit, dynamic_baf, hyst_cells);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueAppPoolLimitGet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue,
                                                     uint32_t *cells) {
  return bf_tm_q_app_pool_limit_get(dev, port, queue, cells);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueAppPoolDisable(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_queue_t queue) {
  return bf_tm_q_app_pool_usage_disable(dev, port, queue);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueCfgDefaultsGet(bf_dev_id_t dev,
                                                    uint8_t *pfc_cos) {
  return bf_tm_q_pfc_cos_mapping_get_default(dev, pfc_cos);
}

bf_status_t TrafficMgrIntf::bfTMQueueColorDefaultsGet(
    bf_dev_id_t dev,
    bool *drop_enable,
    bf_tm_queue_color_limit_t *yellow_drop_limit,
    bf_tm_thres_t *hyst_yellow,
    bf_tm_queue_color_limit_t *red_drop_limit,
    bf_tm_thres_t *hyst_red) {
  bf_status_t status = BF_INVALID_ARG;

  status = bf_tm_q_color_drop_get_default(dev, drop_enable);
  if (status) {
    return status;
  }

  status =
      bf_tm_q_color_limit_get_default(dev, BF_TM_COLOR_RED, red_drop_limit);
  if (status) {
    return status;
  }

  status = bf_tm_q_color_limit_get_default(
      dev, BF_TM_COLOR_YELLOW, yellow_drop_limit);
  if (status) {
    return status;
  }

  status = bf_tm_q_color_hysteresis_get_default(dev, BF_TM_COLOR_RED, hyst_red);
  if (status) {
    return status;
  }

  status = bf_tm_q_color_hysteresis_get_default(
      dev, BF_TM_COLOR_YELLOW, hyst_yellow);
  return status;
}

bf_status_t TrafficMgrIntf::bfTMQueueBufferDefaultsGet(
    bf_dev_id_t dev,
    uint32_t *guaranteed_cells,
    uint32_t *hysteresis_cells,
    bool *tail_drop_enable,
    bf_tm_app_pool_t *pool_id,
    uint32_t *pool_max_cells,
    bf_tm_queue_baf_t *dynamic_baf) {
  bf_status_t status = bf_tm_q_app_pool_usage_get_default(
      dev, pool_id, pool_max_cells, dynamic_baf, hysteresis_cells);
  if (status) {
    return status;
  }

  status = bf_tm_q_tail_drop_get_default(dev, tail_drop_enable);
  if (status) {
    return status;
  }

  status = bf_tm_q_guaranteed_min_limit_get_default(dev, guaranteed_cells);

  return status;
}

//--- Queue Scheduling
bf_status_t TrafficMgrIntf::bfTMQueueSchedEnableGet(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_queue_t queue,
                                                    bool *enable) {
  return bf_tm_sched_q_enable_get(dev, port, queue, enable);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedEnableDefaultGet(bf_dev_id_t dev,
                                                           bf_dev_port_t port,
                                                           bf_tm_queue_t queue,
                                                           bool *enable) {
  return bf_tm_sched_q_enable_get_default(dev, port, queue, enable);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedEnable(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_queue_t queue) {
  return bf_tm_sched_q_enable(dev, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedDisable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) {
  return bf_tm_sched_q_disable(dev, port, queue);
}

//
bf_status_t TrafficMgrIntf::bfTMQueueSchedSpeedGet(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_queue_t queue,
                                                   bf_port_speeds_t *speed) {
  return bf_tm_sched_q_speed_get(dev, port, queue, speed);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedEnableGet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_queue_t queue, bool *enable) {
  return bf_tm_sched_q_guaranteed_enable_get(dev, port, queue, enable);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedEnableDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_q_guaranteed_enable_get_default(
      dev, port, queue, enable, priority);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedEnable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_queue_t queue) {
  return bf_tm_sched_q_guaranteed_rate_enable(dev, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedDisable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_queue_t queue) {
  return bf_tm_sched_q_guaranteed_rate_disable(dev, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedPriorityGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_q_priority_get(dev, port, queue, priority);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedGuaranteedPrioritySet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t priority) {
  return bf_tm_sched_q_priority_set(dev, port, queue, priority);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedShapingEnableGet(bf_dev_id_t dev,
                                                           bf_dev_port_t port,
                                                           bf_tm_queue_t queue,
                                                           bool *enable) {
  return bf_tm_sched_q_shaping_enable_get(dev, port, queue, enable);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedShapingEnableDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_q_shaping_enable_get_default(
      dev, port, queue, enable, priority);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedShapingEnable(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue) {
  return bf_tm_sched_q_max_shaping_rate_enable(dev, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedShapingDisable(bf_dev_id_t dev,
                                                         bf_dev_port_t port,
                                                         bf_tm_queue_t queue) {
  return bf_tm_sched_q_max_shaping_rate_disable(dev, port, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedRemainingBwPriorityGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_q_remaining_bw_priority_get(dev, port, queue, priority);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedRemainingBwPrioritySet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t priority) {
  return bf_tm_sched_q_remaining_bw_priority_set(dev, port, queue, priority);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedDwrrWeightGet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue,
                                                        uint16_t *weight) {
  return bf_tm_sched_q_dwrr_weight_get(dev, port, queue, weight);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedDwrrWeightDefaultGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    uint16_t *weight) {
  return bf_tm_sched_q_dwrr_weight_get_default(dev, port, queue, weight);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedDwrrWeightSet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bf_tm_queue_t queue,
                                                        uint16_t weight) {
  return bf_tm_sched_q_dwrr_weight_set(dev, port, queue, weight);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedAdvFcModeGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_adv_fc_mode_t *mode) {
  return bf_tm_sched_q_adv_fc_mode_get(dev, port, queue, mode);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedAdvFcModeDefaultGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_adv_fc_mode_t *mode) {
  return bf_tm_sched_q_adv_fc_mode_get_default(dev, port, queue, mode);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedAdvFcModeSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_adv_fc_mode_t mode) {
  return bf_tm_sched_q_adv_fc_mode_set(dev, port, queue, mode);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedMaxRateDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  return bf_tm_sched_q_shaping_rate_get_default(
      dev, port, queue, pps, burst_size, rate, prov_type);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedMaxRateGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  return bf_tm_sched_q_shaping_rate_get_provisioning(
      dev, port, queue, pps, burst_size, rate, prov_type);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedMaxRateSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  return bf_tm_sched_q_shaping_rate_set_provisioning(
      dev, port, queue, pps, burst_size, rate, prov_type);
}

//---
bf_status_t TrafficMgrIntf::bfTMQueueSchedMinRateDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  return bf_tm_sched_q_guaranteed_rate_get_default(
      dev, port, queue, pps, burst_size, rate, prov_type);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedMinRateGet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue,
                                                     bool *pps,
                                                     uint32_t *burst_size,
                                                     uint32_t *rate) {
  return bf_tm_sched_q_guaranteed_rate_get(
      dev, port, queue, pps, burst_size, rate);
}

bf_status_t TrafficMgrIntf::bfTMQueueSchedMinRateSet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_queue_t queue,
                                                     bool pps,
                                                     uint32_t burst_size,
                                                     uint32_t rate) {
  return bf_tm_sched_q_guaranteed_rate_set(
      dev, port, queue, pps, burst_size, rate);
}

//--------------------- TM L1 Node API
bf_status_t TrafficMgrIntf::bfTML1NodePortAssignmentGet(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t l1_node,
    bool *in_use,
    uint8_t *pg_port_nr,
    bf_dev_port_t *port_id) {
  return bf_tm_sched_l1_port_assignment_get(
      dev, pipe, pg_id, l1_node, in_use, pg_port_nr, port_id);
}

bf_status_t TrafficMgrIntf::bfTML1NodePortAssignmentDefaultGet(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t l1_node,
    bool *in_use,
    uint8_t *pg_port_nr,
    bf_dev_port_t *port_id) {
  return bf_tm_sched_l1_port_assignment_get_default(
      dev, pipe, pg_id, l1_node, in_use, pg_port_nr, port_id);
}

bf_status_t TrafficMgrIntf::bfTML1NodeQueueAssignmentGet(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    bf_tm_pg_t pg_id,
    bf_tm_l1_node_t l1_node,
    uint8_t *l1_queues_cnt,
    bf_tm_queue_t *l1_queues) {
  return bf_tm_sched_l1_queue_assignment_get(
      dev, pipe, pg_id, l1_node, l1_queues_cnt, l1_queues);
}

bf_status_t TrafficMgrIntf::bfTMPortL1NodeAssignmentGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t port_id,
    bool in_use_only,
    uint8_t *l1_nodes_cnt,
    bf_tm_l1_node_t *l1_nodes) {
  return bf_tm_port_l1_node_assignment_get(
      dev_id, port_id, in_use_only, l1_nodes_cnt, l1_nodes);
}

bf_status_t TrafficMgrIntf::bfTMQueueL1NodeSet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_q_l1_set(dev, port, l1_node, queue);
}

bf_status_t TrafficMgrIntf::bfTMQueueL1NodeGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               bf_tm_l1_node_t *l1_node) {
  return bf_tm_sched_q_l1_get(dev, port, queue, l1_node);
}

bf_status_t TrafficMgrIntf::bfTMQueueL1NodeDefaultGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_l1_node_t *l1_node) {
  return bf_tm_sched_q_l1_get_default(dev, port, queue, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeFree(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_free(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedEnableGet(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_l1_node_t l1_node,
                                                     bool *enable) {
  return bf_tm_sched_l1_enable_get(dev, port, l1_node, enable);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedEnable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_enable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedDisable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_disable(dev, port, l1_node);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateEnableGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable) {
  return bf_tm_sched_l1_guaranteed_rate_enable_get(dev, port, l1_node, enable);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateEnableDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_l1_guaranteed_enable_get_default(
      dev, port, l1_node, enable, priority);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateEnable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_guaranteed_rate_enable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateDisable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_guaranteed_rate_disable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRatePriorityGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_l1_priority_get(dev, port, l1_node, priority);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRatePrioritySet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t priority) {
  return bf_tm_sched_l1_priority_set(dev, port, l1_node, priority);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateEnableGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable) {
  return bf_tm_sched_l1_shaping_rate_enable_get(dev, port, l1_node, enable);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateEnableDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_l1_shaping_enable_get_default(
      dev, port, l1_node, enable, priority);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateEnable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_max_shaping_rate_enable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateDisable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_max_shaping_rate_disable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRatePriorityGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t *priority) {
  return bf_tm_sched_l1_remaining_bw_priority_get(dev, port, l1_node, priority);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRatePrioritySet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t priority) {
  return bf_tm_sched_l1_remaining_bw_priority_set(dev, port, l1_node, priority);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedDwrrWeightGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    uint16_t *weight) {
  return bf_tm_sched_l1_dwrr_weight_get(dev, port, l1_node, weight);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedDwrrWeightDefaultGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    uint16_t *weight) {
  return bf_tm_sched_l1_dwrr_weight_get_default(dev, port, l1_node, weight);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedDwrrWeightSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    uint16_t weight) {
  return bf_tm_sched_l1_dwrr_weight_set(dev, port, l1_node, weight);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedPriorityPropagationGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *is_enabled) {
  return bf_tm_sched_l1_priority_prop_enable_get(
      dev, port, l1_node, is_enabled);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedPriorityPropagationDefaultGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *is_enabled) {
  return bf_tm_sched_l1_priority_prop_get_default(
      dev, port, l1_node, is_enabled);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedPriorityPropagationEnable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_priority_prop_enable(dev, port, l1_node);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedPriorityPropagationDisable(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_l1_node_t l1_node) {
  return bf_tm_sched_l1_priority_prop_disable(dev, port, l1_node);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate) {
  return bf_tm_sched_l1_shaping_rate_get_default(
      dev, port, l1_node, pps, burst_size, rate);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateGet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *pps,
                                                      uint32_t *burst_size,
                                                      uint32_t *rate) {
  return bf_tm_sched_l1_shaping_rate_get(
      dev, port, l1_node, pps, burst_size, rate);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMaxRateSet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool pps,
                                                      uint32_t burst_size,
                                                      uint32_t rate) {
  return bf_tm_sched_l1_shaping_rate_set(
      dev, port, l1_node, pps, burst_size, rate);
}

//---
bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate) {
  return bf_tm_sched_l1_guaranteed_rate_get_default(
      dev, port, l1_node, pps, burst_size, rate);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateGet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *pps,
                                                      uint32_t *burst_size,
                                                      uint32_t *rate) {
  return bf_tm_sched_l1_guaranteed_rate_get(
      dev, port, l1_node, pps, burst_size, rate);
}

bf_status_t TrafficMgrIntf::bfTML1NodeSchedMinRateSet(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool pps,
                                                      uint32_t burst_size,
                                                      uint32_t rate) {
  return bf_tm_sched_l1_guaranteed_rate_set(
      dev, port, l1_node, pps, burst_size, rate);
}

//--------------------- TM Port Group API
bf_status_t TrafficMgrIntf::bfTMPortGroupPortQueueGet(
    const bf_rt_target_t &dev_tgt,
    bf_tm_pg_t pg_id,
    uint8_t pg_queue,
    bf_dev_port_t *port,
    bf_tm_queue_t *queue_nr,
    bool *is_mapped) {
  bf_dev_target_t device_tgt;

  device_tgt.device_id = dev_tgt.dev_id;
  device_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return bf_tm_pg_port_queue_get(
      &device_tgt, pg_id, pg_queue, port, queue_nr, is_mapped);
}

//--------------------- TM Port API
bf_status_t TrafficMgrIntf::bfTMPortIsValid(bf_dev_id_t dev,
                                            bf_dev_port_t dev_port) {
  return bf_tm_port_is_valid(dev, dev_port);
}

bf_status_t TrafficMgrIntf::bfTMPortStatusGet(bf_dev_id_t dev,
                                              bf_dev_port_t port_id,
                                              bool *is_offline,
                                              bool *is_enabled,
                                              bool *qac_rx_enable,
                                              bool *recirc_enable,
                                              bool *has_mac) {
  return bf_tm_port_status_get(dev,
                               port_id,
                               is_offline,
                               is_enabled,
                               qac_rx_enable,
                               recirc_enable,
                               has_mac);
}

bf_status_t TrafficMgrIntf::bfTMPortIcosCntGet(bf_dev_id_t dev,
                                               bf_dev_port_t port_id,
                                               uint8_t *icos_count) {
  return bf_tm_port_icos_count_get(dev, port_id, icos_count);
}

bf_status_t TrafficMgrIntf::bfTMPortIcosMapDefaultsGet(bf_dev_id_t dev,
                                                       bf_dev_port_t port,
                                                       uint8_t *icos_mask) {
  return bf_tm_port_icos_map_get_default(dev, port, icos_mask);
}

bf_status_t TrafficMgrIntf::bfTMPortPPGMapGet(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint8_t *ppg_mask,
                                              bf_tm_ppg_hdl *ppg_hdlrs) {
  return bf_tm_port_ppg_map_get(dev, port, ppg_mask, ppg_hdlrs);
}

bf_status_t TrafficMgrIntf::bfTMPortGroupGet(bf_dev_id_t dev,
                                             bf_dev_port_t port_id,
                                             bf_tm_pg_t *pg_id,
                                             uint8_t *pg_port_nr) {
  return bf_tm_port_group_get(dev, port_id, pg_id, pg_port_nr);
}

bf_status_t TrafficMgrIntf::bfTMPortBaseQueueGet(bf_dev_id_t dev,
                                                 bf_dev_port_t port_id,
                                                 bf_tm_pg_t *pg_id,
                                                 uint8_t *pg_queue,
                                                 bool *is_mapped) {
  return bf_tm_port_base_queue_get(dev, port_id, pg_id, pg_queue, is_mapped);
}

bf_status_t TrafficMgrIntf::bfTMPortQMappingGet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                uint8_t *q_count,
                                                uint8_t *q_mapping) {
  return bf_tm_port_q_mapping_get(dev, port, q_count, q_mapping);
}

bf_status_t TrafficMgrIntf::bfTMPortQMappingSet(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                uint8_t q_count,
                                                uint8_t *q_mapping) {
  return bf_tm_port_q_mapping_set(dev, port, q_count, q_mapping);
}

bf_status_t TrafficMgrIntf::bfTMPortCreditsGet(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               uint32_t *pkt_credits) {
  return bf_tm_port_credits_get(dev, port, pkt_credits);
}

bf_status_t TrafficMgrIntf::bfTMPortBufferDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool *ct_enable,
    uint8_t *uc_ct_limit_cells,
    uint32_t *ig_limit_cells,
    uint32_t *ig_hysteresis_cells,
    uint32_t *eg_limit_cells,
    uint32_t *eg_hysteresis_cells,
    uint32_t *skid_limit_cells) {
  return bf_tm_port_buffer_get_default(dev,
                                       port,
                                       ct_enable,
                                       uc_ct_limit_cells,
                                       ig_limit_cells,
                                       ig_hysteresis_cells,
                                       eg_limit_cells,
                                       eg_hysteresis_cells,
                                       skid_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortCutThroughGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bool *ct_enable) {
  return bf_tm_port_cut_through_enable_status_get(
      dev, port, ct_enable, ct_enable);
}

bf_status_t TrafficMgrIntf::bfTMPortCutThroughLimitGet(
    bf_dev_id_t dev, bf_dev_port_t port, uint8_t *uc_ct_limit_cells) {
  return bf_tm_port_uc_cut_through_limit_get(dev, port, uc_ct_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufIgLimitGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint32_t *ig_limit_cells) {
  return bf_tm_port_ingress_drop_limit_get(dev, port, ig_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufIgHysteresisGet(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t *ig_hysteresis_cells) {
  return bf_tm_port_ingress_hysteresis_get(dev, port, ig_hysteresis_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufEgLimitGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint32_t *eg_limit_cells) {
  return bf_tm_port_egress_drop_limit_get(dev, port, eg_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufEgHysteresisGet(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t *eg_hysteresis_cells) {
  return bf_tm_port_egress_hysteresis_get(dev, port, eg_hysteresis_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufSkidLimitGet(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t *skid_limit_cells) {
  return bf_tm_port_skid_limit_get(dev, port, skid_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortCutThroughLimitSet(
    bf_dev_id_t dev, bf_dev_port_t port, uint8_t uc_ct_limit_cells) {
  return bf_tm_port_uc_cut_through_limit_set(dev, port, uc_ct_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufIgLimitSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint32_t ig_limit_cells) {
  return bf_tm_port_ingress_drop_limit_set(dev, port, ig_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufIgHysteresisSet(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t ig_hysteresis_cells) {
  return bf_tm_port_ingress_hysteresis_set(dev, port, ig_hysteresis_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufEgLimitSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint32_t eg_limit_cells) {
  return bf_tm_port_egress_drop_limit_set(dev, port, eg_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufEgHysteresisSet(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t eg_hysteresis_cells) {
  return bf_tm_port_egress_hysteresis_set(dev, port, eg_hysteresis_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortBufSkidLimitSet(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    uint32_t skid_limit_cells) {
  return bf_tm_port_skid_limit_set(dev, port, skid_limit_cells);
}

bf_status_t TrafficMgrIntf::bfTMPortFlowCtrlDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_flow_ctrl_type_t *mode_tx,
    bf_tm_flow_ctrl_type_t *mode_rx,
    uint8_t *cos_map) {
  return bf_tm_port_flowcontrol_get_default(
      dev, port, mode_tx, mode_rx, cos_map);
}

bf_status_t TrafficMgrIntf::bfTMPortFlowCtrlTxGet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t *mode_tx) {
  return bf_tm_port_flowcontrol_mode_get(dev, port, mode_tx);
}

bf_status_t TrafficMgrIntf::bfTMPortFlowCtrlRxGet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t *mode_rx) {
  return bf_tm_port_flowcontrol_rx_get(dev, port, mode_rx);
}

bf_status_t TrafficMgrIntf::bfTMPortCosMappingGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint8_t *cos_map) {
  return bf_tm_port_pfc_cos_mapping_get(dev, port, cos_map);
}

bf_status_t TrafficMgrIntf::bfTMPortFlowCtrlTxSet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t mode_tx) {
  return bf_tm_port_flowcontrol_mode_set(dev, port, mode_tx);
}

bf_status_t TrafficMgrIntf::bfTMPortFlowCtrlRxSet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_tm_flow_ctrl_type_t mode_rx) {
  return bf_tm_port_flowcontrol_rx_set(dev, port, mode_rx);
}

bf_status_t TrafficMgrIntf::bfTMPortCosMappingSet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  uint8_t *cos_map) {
  return bf_tm_port_pfc_cos_mapping_set(dev, port, cos_map);
}

//-------- Port Scheduling
bf_status_t TrafficMgrIntf::bfTMPortSchedEnable(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_port_speeds_t speed) {
  return bf_tm_sched_port_enable(dev, port, speed);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedDisable(bf_dev_id_t dev,
                                                 bf_dev_port_t port) {
  return bf_tm_sched_port_disable(dev, port);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedSpeedGet(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_port_speeds_t *speed) {
  return bf_tm_sched_port_speed_get(dev, port, speed);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedSpeedResetGet(
    bf_dev_id_t dev, bf_dev_port_t port, bf_port_speeds_t *speed) {
  return bf_tm_sched_port_speed_get_reset(dev, port, speed);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedShapingEnableDefaultGet(
    bf_dev_id_t dev, bf_dev_port_t port, bool *enable) {
  return bf_tm_sched_port_shaping_enable_get_default(dev, port, enable);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedShapingEnableGet(bf_dev_id_t dev,
                                                          bf_dev_port_t port,
                                                          bool *enable) {
  return bf_tm_sched_port_shaping_enable_get(dev, port, enable);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedShapingEnable(bf_dev_id_t dev,
                                                       bf_dev_port_t port) {
  return bf_tm_sched_port_shaping_enable(dev, port);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedShapingDisable(bf_dev_id_t dev,
                                                        bf_dev_port_t port) {
  return bf_tm_sched_port_shaping_disable(dev, port);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedMaxRateDefaultsGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  return bf_tm_sched_port_shaping_rate_get_default(
      dev, port, pps, burst_size, rate, prov_type);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedMaxRateGet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  return bf_tm_sched_port_shaping_rate_get_provisioning(
      dev, port, pps, burst_size, rate, prov_type);
}

bf_status_t TrafficMgrIntf::bfTMPortSchedMaxRateSet(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  return bf_tm_sched_port_shaping_rate_set_provisioning(
      dev, port, pps, burst_size, rate, prov_type);
}

/******** Pool Config APIs *******/
bf_status_t TrafficMgrIntf::bfTmAppPoolSizeSet(bf_dev_id_t dev,
                                               bf_tm_app_pool_t pool,
                                               uint32_t cells) {
  return bf_tm_pool_size_set(dev, pool, cells);
}

bf_status_t TrafficMgrIntf::bfTmAppPoolSizeGet(bf_dev_id_t dev,
                                               bf_tm_app_pool_t pool,
                                               uint32_t *cells) {
  return bf_tm_pool_size_get(dev, pool, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolSkidSizeSet(bf_dev_id_t dev,
                                                uint32_t cells) {
  return bf_tm_pool_skid_size_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolMirrorOnDropSizeSet(bf_dev_id_t dev,
                                                        uint32_t cells) {
  return bf_tm_pool_mirror_on_drop_size_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPreFifoLimitSet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint8_t fifo,
                                                uint32_t cells) {
  return bf_tm_pre_fifo_limit_set(dev, pipe, fifo, cells);
}

bf_status_t TrafficMgrIntf::bfTmGlobalMinLimitSet(bf_dev_id_t dev,
                                                  uint32_t cells) {
  return bf_tm_global_min_limit_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolUcCutThroughSizeSet(bf_dev_id_t dev,
                                                        uint32_t cells) {
  return bf_tm_pool_uc_cut_through_size_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolMcCutThroughSizeSet(bf_dev_id_t dev,
                                                        uint32_t cells) {
  return bf_tm_pool_mc_cut_through_size_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolSkidSizeGet(bf_dev_id_t dev,
                                                uint32_t *cells) {
  return bf_tm_pool_skid_size_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolMirrorOnDropSizeGet(bf_dev_id_t dev,
                                                        uint32_t *cells) {
  return bf_tm_pool_mirror_on_drop_size_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPreFifoLimitGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint8_t fifo,
                                                uint32_t *cells) {
  return bf_tm_pre_fifo_limit_get(dev, pipe, fifo, cells);
}

bf_status_t TrafficMgrIntf::bfTmGlobalMinLimitGet(bf_dev_id_t dev,
                                                  uint32_t *cells) {
  return bf_tm_global_min_limit_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolUcCutThroughSizeGet(bf_dev_id_t dev,
                                                        uint32_t *cells) {
  return bf_tm_pool_uc_cut_through_size_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmPoolMcCutThroughSizeGet(bf_dev_id_t dev,
                                                        uint32_t *cells) {
  return bf_tm_pool_mc_cut_through_size_get(dev, cells);
}

/******** Skid Pool APIs *******/
bf_status_t TrafficMgrIntf::bfTmSkidPoolHysteresisSet(bf_dev_id_t dev,
                                                      uint32_t cells) {
  return bf_tm_pool_skid_hysteresis_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmSkidPoolHysteresisGet(bf_dev_id_t dev,
                                                      uint32_t *cells) {
  return bf_tm_pool_skid_hysteresis_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmSkidPoolHysteresisGetDefault(bf_dev_id_t dev,
                                                             uint32_t *cells) {
  return bf_tm_pool_skid_hysteresis_get_default(dev, cells);
}

/***** App Pool APIs *******/
bf_status_t TrafficMgrIntf::bfTmPoolColorDropEnable(bf_dev_id_t dev,
                                                    bf_tm_app_pool_t pool) {
  return bf_tm_pool_color_drop_enable(dev, pool);
}

bf_status_t TrafficMgrIntf::bfTmPoolColorDropDisable(bf_dev_id_t dev,
                                                     bf_tm_app_pool_t pool) {
  return bf_tm_pool_color_drop_disable(dev, pool);
}

bf_status_t TrafficMgrIntf::bfTmAppPoolColorDropLimitSet(bf_dev_id_t dev,
                                                         bf_tm_app_pool_t pool,
                                                         bf_tm_color_t color,
                                                         uint32_t limit) {
  return bf_tm_pool_color_drop_limit_set(dev, pool, color, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolColorDropLimitGet(bf_dev_id_t dev,
                                                      bf_tm_app_pool_t pool,
                                                      bf_tm_color_t color,
                                                      uint32_t *limit) {
  return bf_tm_pool_color_drop_limit_get(dev, pool, color, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolColorDropStateGet(bf_dev_id_t dev,
                                                      bf_tm_app_pool_t pool,
                                                      bool *drop_state) {
  return bf_tm_pool_color_drop_state_get(dev, pool, drop_state);
}

/***** Pool color APIs *******/
bf_status_t TrafficMgrIntf::bfTmPoolColorDropHysteresisSet(bf_dev_id_t dev,
                                                           bf_tm_color_t color,
                                                           uint32_t limit) {
  return bf_tm_pool_color_drop_hysteresis_set(dev, color, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolColorDropHysteresisGet(bf_dev_id_t dev,
                                                           bf_tm_color_t color,
                                                           uint32_t *limit) {
  return bf_tm_pool_color_drop_hysteresis_get(dev, color, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolColorDropHysteresisDefaultGet(
    bf_dev_id_t dev, bf_tm_color_t color, uint32_t *limit) {
  return bf_tm_pool_color_drop_hysteresis_get_default(dev, color, limit);
}

/***** App Pool PFC APIs *******/
bf_status_t TrafficMgrIntf::bfTmPoolPfcLimitSet(bf_dev_id_t dev,
                                                bf_tm_app_pool_t pool,
                                                bf_tm_icos_t icos,
                                                uint32_t limit) {
  return bf_tm_pool_pfc_limit_set(dev, pool, icos, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolPfcLimitGet(bf_dev_id_t dev,
                                                bf_tm_app_pool_t pool,
                                                bf_tm_icos_t icos,
                                                uint32_t *limit) {
  return bf_tm_pool_pfc_limit_get(dev, pool, icos, limit);
}

bf_status_t TrafficMgrIntf::bfTmPoolPfcLimitGetDefault(bf_dev_id_t dev,
                                                       bf_tm_app_pool_t pool,
                                                       bf_tm_icos_t icos,
                                                       uint32_t *limit) {
  return bf_tm_pool_pfc_limit_get_default(dev, pool, icos, limit);
}

bf_status_t TrafficMgrIntf::bfTmMaxPfcLevelsGet(bf_dev_id_t dev,
                                                uint32_t *levels) {
  return bf_tm_max_pfc_levels_get(dev, levels);
}

/***** Port counter APIs *******/
bf_status_t TrafficMgrIntf::bfTmPortDropGetCached(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bf_dev_port_t port,
                                                  uint64_t *ig_count,
                                                  uint64_t *eg_count) {
  return bf_tm_port_drop_cache_get(dev, pipe, port, ig_count, eg_count);
}

bf_status_t TrafficMgrIntf::bfTmPortUsageGet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bf_dev_port_t port,
                                             uint32_t *ig_count,
                                             uint32_t *eg_count,
                                             uint32_t *ig_wm,
                                             uint32_t *eg_wm) {
  return bf_tm_port_usage_get(
      dev, pipe, port, ig_count, eg_count, ig_wm, eg_wm);
}

bf_status_t TrafficMgrIntf::bfTmPortIngressWatermarkClear(bf_dev_id_t dev,
                                                          bf_dev_port_t port) {
  return bf_tm_port_ingress_watermark_clear(dev, port);
}

bf_status_t TrafficMgrIntf::bfTmPortEgressWatermarkClear(bf_dev_id_t dev,
                                                         bf_dev_port_t port) {
  return bf_tm_port_egress_watermark_clear(dev, port);
}

bf_status_t TrafficMgrIntf::bfTmPortDropIngressCacheSet(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        uint64_t drop_count) {
  return bf_tm_port_drop_ingress_cache_set(dev, port, drop_count);
}

bf_status_t TrafficMgrIntf::bfTmPortDropEgressCacheSet(bf_dev_id_t dev,
                                                       bf_dev_port_t port,
                                                       uint64_t drop_count) {
  return bf_tm_port_drop_egress_cache_set(dev, port, drop_count);
}

/***** Queue counter APIs *******/
bf_status_t TrafficMgrIntf::bfTmQDropGetCached(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue,
                                               uint64_t *count) {
  return bf_tm_q_drop_cache_get(dev, pipe, port, queue, count);
}

bf_status_t TrafficMgrIntf::bfTmQDropCacheSet(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue,
                                              uint64_t count) {
  return bf_tm_q_drop_cache_set(dev, pipe, port, queue, count);
}

bf_status_t TrafficMgrIntf::bfTmQUsageGet(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint32_t *count,
                                          uint32_t *wm) {
  return bf_tm_q_usage_get(dev, pipe, port, queue, count, wm);
}

bf_status_t TrafficMgrIntf::bfTmQWatermarkClear(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue) {
  return bf_tm_q_watermark_clear(dev, port, queue);
}

/***** Pool counter APIs *******/
bf_status_t TrafficMgrIntf::bfTmPoolUsageGet(bf_dev_id_t dev,
                                             bf_tm_app_pool_t pool,
                                             uint32_t *count,
                                             uint32_t *wm) {
  return bf_tm_pool_usage_get(dev, pool, count, wm);
}

bf_status_t TrafficMgrIntf::bfTmPoolWatermarkClear(bf_dev_id_t dev,
                                                   bf_tm_app_pool_t pool) {
  return bf_tm_pool_watermark_clear(dev, pool);
}

/***** Pipe counter APIs *******/
bf_status_t TrafficMgrIntf::bfTmPipeBufferFullDropGet(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      uint64_t *count) {
  return bf_tm_pipe_buffer_full_drop_get(dev, pipe, count);
}

bf_status_t TrafficMgrIntf::bfTmQDiscardUsageGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint32_t *count,
                                                 uint32_t *wm) {
  return bf_tm_q_discard_usage_get(dev, pipe, count, wm);
}

bf_status_t TrafficMgrIntf::bfTmPipeCountersGet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint64_t *cell_count,
                                                uint64_t *pkt_count) {
  return bf_tm_pipe_counters_get(dev, pipe, cell_count, pkt_count);
}

bf_status_t TrafficMgrIntf::bfTmCutThroughCountersGet(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      uint64_t *uc_count,
                                                      uint64_t *mc_count) {
  return bf_tm_cut_through_counters_get(dev, pipe, uc_count, mc_count);
}

bf_status_t TrafficMgrIntf::bfTmBlklvlDropGet(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              bf_tm_blklvl_cntrs_t *blk_cntrs) {
  return bf_tm_blklvl_drop_get(dev, pipe, blk_cntrs);
}

bf_status_t TrafficMgrIntf::bfTmPreFifoDropGet(
    bf_dev_id_t dev, bf_tm_pre_fifo_cntrs_t *fifo_cntrs) {
  return bf_tm_pre_fifo_drop_get(dev, fifo_cntrs);
}

bf_status_t TrafficMgrIntf::bfTmPipeBufferFullDropClear(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe) {
  return bf_tm_pipe_buffer_full_drop_clear(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmQDiscardWatermarkClear(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe) {
  return bf_tm_q_discard_watermark_clear(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmPipeClearCellCounter(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe) {
  return bf_tm_pipe_clear_cell_counter(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmPipeClearPacketCounter(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe) {
  return bf_tm_pipe_clear_packet_counter(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmPipeClearUcCtPacketCounter(bf_dev_id_t dev,
                                                           bf_dev_pipe_t pipe) {
  return bf_tm_pipe_clear_uc_ct_packet_counter(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmPipeClearMcCtPacketCounter(bf_dev_id_t dev,
                                                           bf_dev_pipe_t pipe) {
  return bf_tm_pipe_clear_mc_ct_packet_counter(dev, pipe);
}

bf_status_t TrafficMgrIntf::bfTmBlklvlDropClear(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t clear_mask) {
  return bf_tm_blklvl_drop_clear(dev, pipe, clear_mask);
}

bf_status_t TrafficMgrIntf::bfTmPreFifoDropClear(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint32_t fifo) {
  return bf_tm_pre_fifo_drop_clear(dev, pipe, fifo);
}

/***** PPG Counter APIs *******/
bf_status_t TrafficMgrIntf::bfTmPpgDropGet(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_tm_ppg_hdl ppg,
                                           uint64_t *count) {
  return bf_tm_ppg_drop_get(dev, pipe, ppg, count);
}

bf_status_t TrafficMgrIntf::bfTmPpgDropGetCached(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_tm_ppg_hdl ppg,
                                                 uint64_t *count) {
  return bf_tm_ppg_drop_cache_get(dev, pipe, ppg, count);
}

bf_status_t TrafficMgrIntf::bfTmPpgDropCacheSet(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bf_tm_ppg_hdl ppg,
                                                uint64_t count) {
  return bf_tm_ppg_drop_cache_set(dev, pipe, ppg, count);
}

bf_status_t TrafficMgrIntf::bfTmPpgUsageGet(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bf_tm_ppg_hdl ppg,
                                            uint32_t *gmin_count,
                                            uint32_t *shared_count,
                                            uint32_t *skid_count,
                                            uint32_t *wm) {
  return bf_tm_ppg_usage_get(
      dev, pipe, ppg, gmin_count, shared_count, skid_count, wm);
}

bf_status_t TrafficMgrIntf::bfTmPpgWatermarkClear(bf_dev_id_t dev,
                                                  bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_watermark_clear(dev, ppg);
}

/***** TM Cfg APIs *******/
bf_status_t TrafficMgrIntf::bfTmTimestampShiftSet(bf_dev_id_t dev,
                                                  uint8_t shift) {
  return bf_tm_timestamp_shift_set(dev, shift);
}

bf_status_t TrafficMgrIntf::bfTmTimestampShiftGet(bf_dev_id_t dev,
                                                  uint8_t *shift) {
  return bf_tm_timestamp_shift_get(dev, shift);
}

bf_status_t TrafficMgrIntf::bfTmTimestampShiftGetDefault(bf_dev_id_t dev,
                                                         uint8_t *shift) {
  return bf_tm_timestamp_shift_get_default(dev, shift);
}

bf_status_t TrafficMgrIntf::bfTmCellSizeInBytesGet(bf_dev_id_t dev,
                                                   uint32_t *cell_size) {
  return bf_tm_cell_size_in_bytes_get(dev, cell_size);
}

bf_status_t TrafficMgrIntf::bfTmTotalCellCountGet(bf_dev_id_t dev,
                                                  uint32_t *total_cells) {
  return bf_tm_total_cell_count_get(dev, total_cells);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitSet(bf_dev_id_t dev,
                                                      uint32_t cells) {
  return bf_tm_global_max_limit_set(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitGet(bf_dev_id_t dev,
                                                      uint32_t *cells) {
  return bf_tm_global_max_limit_get(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitGetDefault(bf_dev_id_t dev,
                                                             uint32_t *cells) {
  return bf_tm_global_max_limit_get_default(dev, cells);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitEnable(bf_dev_id_t dev) {
  return bf_tm_global_max_limit_enable(dev);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitDisable(bf_dev_id_t dev) {
  return bf_tm_global_max_limit_disable(dev);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitStateGet(bf_dev_id_t dev,
                                                           bool *state) {
  return bf_tm_global_max_limit_state_get(dev, state);
}

bf_status_t TrafficMgrIntf::bfTmIngressBufferLimitStateGetDefault(
    bf_dev_id_t dev, bool *state) {
  return bf_tm_global_max_limit_state_get_default(dev, state);
}

/***** TM Pipe Multicast fifo APIs *******/
bf_status_t TrafficMgrIntf::bfTmMcFifoIcosMappingSet(bf_dev_id_t dev,
                                                     uint8_t pipe_bmap,
                                                     int fifo,
                                                     uint8_t icos_bmap) {
  return bf_tm_mc_fifo_icos_mapping_set(dev, pipe_bmap, fifo, icos_bmap);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoIcosMappingGet(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     int fifo,
                                                     uint8_t *icos_bmap) {
  return bf_tm_mc_fifo_icos_mapping_get(dev, pipe, fifo, icos_bmap);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoIcosMappingGetDefault(
    bf_dev_id_t dev, bf_dev_pipe_t pipe, int fifo, uint8_t *icos_bmap) {
  return bf_tm_mc_fifo_icos_mapping_get_default(dev, pipe, fifo, icos_bmap);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoArbModeSet(bf_dev_id_t dev,
                                                 uint8_t pipe_bmap,
                                                 int fifo,
                                                 bool use_strict_pri) {
  return bf_tm_mc_fifo_arb_mode_set(dev, pipe_bmap, fifo, use_strict_pri);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoArbModeGet(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 int fifo,
                                                 bool *use_strict_pri) {
  return bf_tm_mc_fifo_arb_mode_get(dev, pipe, fifo, use_strict_pri);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoArbModeGetDefault(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        int fifo,
                                                        bool *use_strict_pri) {
  return bf_tm_mc_fifo_arb_mode_get_default(dev, pipe, fifo, use_strict_pri);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoWrrWeightSet(bf_dev_id_t dev,
                                                   uint8_t pipe_bmap,
                                                   int fifo,
                                                   uint8_t weight) {
  return bf_tm_mc_fifo_wrr_weight_set(dev, pipe_bmap, fifo, weight);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoWrrWeightGet(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe,
                                                   int fifo,
                                                   uint8_t *weight) {
  return bf_tm_mc_fifo_wrr_weight_get(dev, pipe, fifo, weight);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoWrrWeightGetDefault(bf_dev_id_t dev,
                                                          bf_dev_pipe_t pipe,
                                                          int fifo,
                                                          uint8_t *weight) {
  return bf_tm_mc_fifo_wrr_weight_get_default(dev, pipe, fifo, weight);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoDepthSet(bf_dev_id_t dev,
                                               uint8_t pipe_bmap,
                                               int fifo,
                                               int size) {
  return bf_tm_mc_fifo_depth_set(dev, pipe_bmap, fifo, size);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoDepthGet(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               int fifo,
                                               int *size) {
  return bf_tm_mc_fifo_depth_get(dev, pipe, fifo, size);
}

bf_status_t TrafficMgrIntf::bfTmMcFifoDepthGetDefault(bf_dev_id_t dev,
                                                      bf_dev_pipe_t pipe,
                                                      int fifo,
                                                      int *size) {
  return bf_tm_mc_fifo_depth_get_default(dev, pipe, fifo, size);
}
}  // namespace bfrt
