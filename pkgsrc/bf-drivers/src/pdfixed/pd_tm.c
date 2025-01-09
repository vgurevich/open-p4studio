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


#include <tofino/pdfixed/pd_tm.h>
#include <traffic_mgr/traffic_mgr.h>

p4_pd_status_t p4_pd_tm_allocate_ppg(p4_pd_tm_dev_t dev,
                                     p4_pd_tm_port_t port,
                                     p4_pd_tm_ppg_t *ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_allocate(dev, port, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_free_ppg(p4_pd_tm_dev_t dev, p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_free(dev, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_get_default_ppg(p4_pd_tm_dev_t dev,
                                        p4_pd_tm_port_t port,
                                        p4_pd_tm_ppg_t *ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_defaultppg_get(dev, port, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ppg_icos_mapping(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_ppg_t ppg,
                                             p4_pd_tm_icos_map_t icos_map) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_icos_mapping_set(dev, ppg, icos_map);
  return status;
}

p4_pd_status_t p4_pd_tm_ppg_icos_mapping_get(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_ppg_t ppg,
                                             uint8_t *icos_map) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_icos_mapping_get(dev, ppg, icos_map);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_lossless_treatment(p4_pd_tm_dev_t dev,
                                                  p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_lossless_treatment_enable(dev, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_lossless_treatment(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_lossless_treatment_disable(dev, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ppg_app_pool_usage(p4_pd_tm_dev_t dev,
                                               p4_pd_tm_ppg_t ppg,
                                               p4_pd_pool_id_t pool_id,
                                               uint32_t base_use_limit,
                                               p4_pd_tm_ppg_baf_t dynamic_baf,
                                               uint32_t hysteresis) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_app_pool_usage_set(dev,
                                        ppg,
                                        (bf_tm_app_pool_t)pool_id,
                                        base_use_limit,
                                        (bf_tm_ppg_baf_t)dynamic_baf,
                                        hysteresis);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_ppg_app_pool_usage(p4_pd_tm_dev_t dev,
                                                   p4_pd_pool_id_t pool_id,
                                                   p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_ppg_app_pool_usage_disable(dev, (bf_tm_app_pool_t)pool_id, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ppg_guaranteed_min_limit(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_ppg_t ppg,
                                                     uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ppg_skid_limit(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_ppg_t ppg,
                                           uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_skid_limit_set(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_guaranteed_min_skid_hysteresis(p4_pd_tm_dev_t dev,
                                                           p4_pd_tm_ppg_t ppg,
                                                           uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_guaranteed_min_skid_hysteresis_set(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_q_mapping(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           int16_t q_count,
                                           uint8_t *q_mapping) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_q_mapping_set(dev, port, q_count, q_mapping);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_app_pool_usage(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port,
                                             p4_pd_tm_queue_t queue,
                                             p4_pd_pool_id_t pool,
                                             uint32_t base_use_limit,
                                             p4_pd_tm_queue_baf_t dynamic_baf,
                                             uint32_t hysteresis) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_app_pool_usage_set(dev,
                                      port,
                                      queue,
                                      (bf_tm_app_pool_t)pool,
                                      base_use_limit,
                                      (bf_tm_queue_baf_t)dynamic_baf,
                                      hysteresis);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_q_app_pool_usage(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_app_pool_usage_disable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_guaranteed_min_limit(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   p4_pd_tm_queue_t queue,
                                                   uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_guaranteed_min_limit_set(dev, port, queue, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_color_limit(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          p4_pd_tm_queue_t queue,
                                          p4_pd_color_t color,
                                          p4_pd_color_limit_t limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_limit_set(
      dev, port, queue, (bf_tm_color_t)color, (bf_tm_queue_color_limit_t)limit);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_color_hysteresis(p4_pd_tm_dev_t dev,
                                               p4_pd_tm_port_t port,
                                               p4_pd_tm_queue_t queue,
                                               p4_pd_color_t color,
                                               p4_pd_tm_thres_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_hysteresis_set(
      dev, port, queue, (bf_tm_color_t)color, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_hysteresis(p4_pd_tm_dev_t dev,
                                         p4_pd_tm_port_t port,
                                         p4_pd_tm_queue_t queue,
                                         uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_hysteresis_set(dev, port, queue, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_q_tail_drop(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_tail_drop_enable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_q_tail_drop(p4_pd_tm_dev_t dev,
                                            p4_pd_tm_port_t port,
                                            p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_tail_drop_disable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_q_color_drop(p4_pd_tm_dev_t dev,
                                            p4_pd_tm_port_t port,
                                            p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_drop_enable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_q_color_drop(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port,
                                             p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_drop_disable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_set_negative_mirror_dest(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_pipe_t pipe,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_mirror_on_drop_dest_set(dev, pipe, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_get_negative_mirror_dest(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_pipe_t pipe,
                                                 p4_pd_tm_port_t *port,
                                                 p4_pd_tm_queue_t *queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_mirror_on_drop_dest_get(
      dev, pipe, (bf_dev_port_t *)port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_pfc_cos_mapping(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              p4_pd_tm_queue_t queue,
                                              p4_pd_tm_icos_t cos) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_pfc_cos_mapping_set(dev, port, queue, cos);
  return status;
}

p4_pd_status_t p4_pd_tm_set_app_pool_size(p4_pd_tm_dev_t dev,
                                          p4_pd_pool_id_t pool,
                                          uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_size_set(dev, (bf_tm_app_pool_t)pool, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_app_pool_color_drop(p4_pd_tm_dev_t dev,
                                                   p4_pd_pool_id_t pool) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_color_drop_enable(dev, (bf_tm_app_pool_t)pool);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_app_pool_color_drop(p4_pd_tm_dev_t dev,
                                                    p4_pd_pool_id_t pool) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_color_drop_disable(dev, (bf_tm_app_pool_t)pool);
  return status;
}

p4_pd_status_t p4_pd_tm_set_app_pool_color_drop_limit(p4_pd_tm_dev_t dev,
                                                      p4_pd_pool_id_t pool,
                                                      p4_pd_color_t color,
                                                      uint32_t limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_color_drop_limit_set(
      dev, (bf_tm_app_pool_t)pool, (bf_tm_color_t)color, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_set_app_pool_color_drop_hysteresis(p4_pd_tm_dev_t dev,
                                                           p4_pd_color_t color,
                                                           uint32_t limit) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_pool_color_drop_hysteresis_set(dev, (bf_tm_color_t)color, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_set_app_pool_pfc_limit(p4_pd_tm_dev_t dev,
                                               p4_pd_pool_id_t pool,
                                               p4_pd_tm_icos_t icos,
                                               uint32_t limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_pfc_limit_set(dev, (bf_tm_app_pool_t)pool, icos, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_set_skid_pool_size(p4_pd_tm_dev_t dev, uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_skid_size_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_skid_pool_hysteresis(p4_pd_tm_dev_t dev,
                                                 uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_skid_hysteresis_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_negative_mirror_pool_size(p4_pd_tm_dev_t dev,
                                                      uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_mirror_on_drop_size_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_uc_cut_through_pool_size(p4_pd_tm_dev_t dev,
                                                     uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_uc_cut_through_size_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_mc_cut_through_pool_size(p4_pd_tm_dev_t dev,
                                                     uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_mc_cut_through_size_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ingress_buffer_limit(p4_pd_tm_dev_t dev,
                                                 uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_global_max_limit_set(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_ingress_buffer_limit(p4_pd_tm_dev_t dev) {
  p4_pd_status_t status = 0;
  status = bf_tm_global_max_limit_enable(dev);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_ingress_buffer_limit(p4_pd_tm_dev_t dev) {
  p4_pd_status_t status = 0;
  status = bf_tm_global_max_limit_disable(dev);
  return status;
}

p4_pd_status_t p4_pd_tm_set_egress_pipe_limit(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_pipe_t pipe,
                                              uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pipe_egress_limit_set(dev, pipe, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_egress_pipe_hysteresis(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_pipe_t pipe,
                                                   uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pipe_egress_hysteresis_set(dev, pipe, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ingress_port_drop_limit(p4_pd_tm_dev_t dev,
                                                    p4_pd_tm_port_t port,
                                                    uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_ingress_drop_limit_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_egress_port_drop_limit(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_egress_drop_limit_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_ingress_port_hysteresis(p4_pd_tm_dev_t dev,
                                                    p4_pd_tm_port_t port,
                                                    uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_ingress_hysteresis_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_egress_port_hysteresis(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_egress_hysteresis_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_uc_cut_through_limit(p4_pd_tm_dev_t dev,
                                                      p4_pd_tm_port_t port,
                                                      uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_uc_cut_through_limit_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_flowcontrol_mode(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_flow_ctrl_type_t fctype) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_flowcontrol_mode_set(
      dev, port, (bf_tm_flow_ctrl_type_t)fctype);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_pfc_cos_mapping(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 uint8_t *cos_map) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_pfc_cos_mapping_set(dev, port, cos_map);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_skid_limit(p4_pd_tm_dev_t dev,
                                            p4_pd_tm_port_t port,
                                            uint32_t cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_skid_limit_set(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_set_cpuport(p4_pd_tm_dev_t dev, p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;

  status = bf_tm_port_cpuport_set(dev, port);

  return status;
}

p4_pd_status_t p4_pd_tm_reset_cpuport(p4_pd_tm_dev_t dev) {
  p4_pd_status_t status = 0;

  status = bf_tm_port_cpuport_reset(dev);

  return status;
}

p4_pd_status_t p4_pd_tm_get_total_buffer_size(p4_pd_tm_dev_t dev,
                                              uint64_t *buffer_size) {
  uint32_t total_cells = 0;
  uint32_t cell_size = 0;

  p4_pd_status_t status = 0;

  status = bf_tm_total_cell_count_get(dev, &total_cells);
  if (status != BF_SUCCESS) {
    return status;
  }
  status = bf_tm_cell_size_in_bytes_get(dev, &cell_size);
  if (status != BF_SUCCESS) {
    return status;
  }

  *buffer_size = (total_cells * cell_size);
  return status;
}

p4_pd_status_t p4_pd_tm_get_cell_size_in_bytes(p4_pd_tm_dev_t dev,
                                               uint32_t *cell_size) {
  p4_pd_status_t status = 0;

  status = bf_tm_cell_size_in_bytes_get(dev, cell_size);

  return status;
}

p4_pd_status_t p4_pd_tm_get_cell_count(p4_pd_tm_dev_t dev,
                                       uint32_t *total_cells) {
  p4_pd_status_t status = 0;

  status = bf_tm_total_cell_count_get(dev, total_cells);

  return status;
}

///////////////// SCH APIS //////////////////

p4_pd_status_t p4_pd_tm_set_q_sched_priority(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port,
                                             p4_pd_tm_queue_t queue,
                                             uint16_t priority) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_priority_set(
      dev, port, queue, (bf_tm_sched_prio_t)priority);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_dwrr_weight(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          p4_pd_tm_queue_t queue,
                                          uint16_t weight) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_dwrr_weight_set(dev, port, queue, weight);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_shaping_rate(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           p4_pd_tm_queue_t queue,
                                           bool pps,
                                           uint32_t burstsize,
                                           uint32_t rate) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_sched_q_shaping_rate_set(dev, port, queue, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_shaping_rate_provisioning(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    bool pps,
    uint32_t burstsize,
    uint32_t rate,
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type) {
  p4_pd_status_t status = 0;
  bf_tm_sched_shaper_prov_type_t bf_prov_type =
      (bf_tm_sched_shaper_prov_type_t)pd_prov_type;
  status = bf_tm_sched_q_shaping_rate_set_provisioning(
      dev, port, queue, pps, burstsize, rate, bf_prov_type);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_guaranteed_rate(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              p4_pd_tm_queue_t queue,
                                              bool pps,
                                              uint32_t burstsize,
                                              uint32_t rate) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_sched_q_guaranteed_rate_set(dev, port, queue, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_set_q_remaining_bw_sched_priority(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    uint16_t priority) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_remaining_bw_priority_set(
      dev, port, queue, (uint16_t)priority);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_q_l1_set(p4_pd_tm_dev_t dev,
                                       p4_pd_tm_port_t port,
                                       p4_pd_tm_l1_node_t l1_node,
                                       p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_l1_set(dev, port, l1_node, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_q_l1_reset(p4_pd_tm_dev_t dev,
                                         p4_pd_tm_port_t port,
                                         p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_l1_reset(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_priority_set(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              p4_pd_tm_l1_node_t l1_node,
                                              uint16_t priority) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_priority_set(dev, port, l1_node, priority);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_dwrr_weight_set(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_l1_node_t l1_node,
                                                 uint16_t weight) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_dwrr_weight_set(dev, port, l1_node, weight);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_dwrr_weight_get(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_l1_node_t l1_node,
                                                 uint16_t *weight) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_dwrr_weight_get(dev, port, l1_node, weight);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_shaping_rate_set(p4_pd_tm_dev_t dev,
                                                  p4_pd_tm_port_t port,
                                                  p4_pd_tm_l1_node_t l1_node,
                                                  bool pps,
                                                  uint32_t burst_size,
                                                  uint32_t rate) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_shaping_rate_set(
      dev, port, l1_node, pps, burst_size, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_max_shaping_rate_enable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_max_shaping_rate_enable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_max_shaping_rate_disable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_max_shaping_rate_disable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_priority_prop_enable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_priority_prop_enable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_priority_prop_disable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_priority_prop_disable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_guaranteed_rate_set(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_port_t port,
                                                     p4_pd_tm_l1_node_t l1_node,
                                                     bool pps,
                                                     uint32_t burst_size,
                                                     uint32_t rate) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_guaranteed_rate_set(
      dev, port, l1_node, pps, burst_size, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_remaining_bw_priority_set(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_l1_node_t l1_node,
    uint16_t priority) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_sched_l1_remaining_bw_priority_set(dev, port, l1_node, priority);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_guaranteed_rate_enable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_guaranteed_rate_enable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_guaranteed_rate_disable(
    p4_pd_tm_dev_t dev, p4_pd_tm_port_t port, p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_guaranteed_rate_disable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_enable(p4_pd_tm_dev_t dev,
                                        p4_pd_tm_port_t port,
                                        p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_enable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_disable(p4_pd_tm_dev_t dev,
                                         p4_pd_tm_port_t port,
                                         p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_disable(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_l1_free(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_port_t port,
                                      p4_pd_tm_l1_node_t l1_node) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_l1_free(dev, port, l1_node);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_shaping_rate(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              bool pps,
                                              uint32_t burstsize,
                                              uint32_t rate) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_shaping_rate_set(dev, port, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_set_port_shaping_rate_provisioning(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    bool pps,
    uint32_t burstsize,
    uint32_t rate,
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type) {
  p4_pd_status_t status = 0;

  bf_tm_sched_shaper_prov_type_t bf_prov_type =
      (bf_tm_sched_shaper_prov_type_t)pd_prov_type;

  status = bf_tm_sched_port_shaping_rate_set_provisioning(
      dev, port, pps, burstsize, rate, bf_prov_type);
  return status;
}

p4_pd_status_t p4_pd_tm_set_shaper_pkt_ifg_compensation(p4_pd_tm_dev_t dev,
                                                        p4_pd_tm_pipe_t pipe,
                                                        uint8_t adjustment) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_pkt_ifg_compensation_set(dev, pipe, adjustment);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_q_sched(p4_pd_tm_dev_t dev,
                                       p4_pd_tm_port_t port,
                                       p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_enable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_q_sched(p4_pd_tm_dev_t dev,
                                        p4_pd_tm_port_t port,
                                        p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_disable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_port_shaping(p4_pd_tm_dev_t dev,
                                            p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_shaping_enable(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_port_shaping(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_shaping_disable(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_enable_port_sched(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          uint16_t port_speed) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_enable(dev, port, port_speed);
  return status;
}

p4_pd_status_t p4_pd_tm_disable_port_sched(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_disable(dev, port);
  return status;
}

void p4_pd_tm_complete_operations(p4_pd_tm_dev_t dev) {
  bf_tm_complete_operations(dev);
}

p4_pd_status_t p4_pd_tm_q_max_rate_shaper_enable(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_max_shaping_rate_enable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_q_max_rate_shaper_disable(p4_pd_tm_dev_t dev,
                                                  p4_pd_tm_port_t port,
                                                  p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_max_shaping_rate_disable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_q_min_rate_shaper_enable(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_guaranteed_rate_enable(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_q_min_rate_shaper_disable(p4_pd_tm_dev_t dev,
                                                  p4_pd_tm_port_t port,
                                                  p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_guaranteed_rate_disable(dev, port, queue);
  return status;
}

// CFG Fetch APIs

p4_pd_status_t p4_pd_tm_get_q_sched_priority(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port,
                                             p4_pd_tm_queue_t queue,
                                             p4_pd_tm_sched_prio_t *priority) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_priority_get(
      dev, port, queue, (bf_tm_sched_prio_t *)priority);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_dwrr_weight(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          p4_pd_tm_queue_t queue,
                                          uint16_t *weight) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_dwrr_weight_get(dev, port, queue, weight);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_shaping_rate(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           p4_pd_tm_queue_t queue,
                                           bool *pps,
                                           uint32_t *burstsize,
                                           uint32_t *rate) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_sched_q_shaping_rate_get(dev, port, queue, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_shaping_rate_provisioning(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    bool *pps,
    uint32_t *burstsize,
    uint32_t *rate,
    p4_pd_tm_sched_shaper_prov_type_t *pd_prov_type) {
  p4_pd_status_t status = 0;
  bf_tm_sched_shaper_prov_type_t bf_prov_type;
  status = bf_tm_sched_q_shaping_rate_get_provisioning(
      dev, port, queue, pps, burstsize, rate, &bf_prov_type);
  if (pd_prov_type) {
    *pd_prov_type = (p4_pd_tm_sched_shaper_prov_type_t)bf_prov_type;
  }
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_guaranteed_rate(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              p4_pd_tm_queue_t queue,
                                              bool *pps,
                                              uint32_t *burstsize,
                                              uint32_t *rate) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_sched_q_guaranteed_rate_get(dev, port, queue, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_remaining_bw_sched_priority(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    p4_pd_tm_sched_prio_t *priority) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_q_remaining_bw_priority_get(
      dev, port, queue, (bf_tm_sched_prio_t *)priority);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_shaping_rate(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port,
                                              bool *pps,
                                              uint32_t *burstsize,
                                              uint32_t *rate) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_port_shaping_rate_get(dev, port, pps, burstsize, rate);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_shaping_rate_provisioning(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    bool *pps,
    uint32_t *burstsize,
    uint32_t *rate,
    p4_pd_tm_sched_shaper_prov_type_t *pd_prov_type) {
  p4_pd_status_t status = 0;
  bf_tm_sched_shaper_prov_type_t bf_prov_type;

  status = bf_tm_sched_port_shaping_rate_get_provisioning(
      dev, port, pps, burstsize, rate, &bf_prov_type);

  if (pd_prov_type) {
    *pd_prov_type = (p4_pd_tm_sched_shaper_prov_type_t)bf_prov_type;
  }
  return status;
}

p4_pd_status_t p4_pd_tm_get_shaper_pkt_ifg_compensation(p4_pd_tm_dev_t dev,
                                                        p4_pd_tm_pipe_t pipe,
                                                        uint8_t *adjust) {
  p4_pd_status_t status = 0;
  status = bf_tm_sched_pkt_ifg_compensation_get(dev, pipe, adjust);
  return status;
}

p4_pd_status_t p4_pd_tm_get_egress_pipe_limit(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_pipe_t pipe,
                                              uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pipe_egress_limit_get(dev, pipe, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_egress_pipe_hysteresis(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_pipe_t pipe,
                                                   uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pipe_egress_hysteresis_get(dev, pipe, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_q_mapping(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           uint8_t *q_count,
                                           uint8_t *q_map) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_q_mapping_get(dev, port, q_count, q_map);
  return status;
}

p4_pd_status_t p4_pd_tm_get_pipe_queue_qid_list(p4_pd_tm_dev_t dev,
                                                p4_pd_tm_pipe_t log_pipe,
                                                p4_pd_tm_queue_t pipe_queue,
                                                p4_pd_tm_port_t *port,
                                                uint32_t *qid_count,
                                                p4_pd_tm_queue_t *qid_list) {
  p4_pd_status_t status = 0;
  status = bf_tm_pipe_queue_qid_list_get(
      dev, log_pipe, pipe_queue, (bf_dev_port_t *)port, qid_count, qid_list);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_pipe_physical_queue(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_port_t port,
                                                     uint32_t ingress_qid,
                                                     p4_pd_tm_pipe_t *log_pipe,
                                                     p4_pd_tm_queue_t *phys_q) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_pipe_physical_queue_get(
      dev, (bf_dev_port_t)port, ingress_qid, log_pipe, phys_q);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_app_pool_usage(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port,
                                             p4_pd_tm_queue_t queue,
                                             p4_pd_pool_id_t *pool,
                                             uint32_t *base_use_limit,
                                             p4_pd_tm_queue_baf_t *dynamic_baf,
                                             uint32_t *hysteresis) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_app_pool_usage_get(dev,
                                      port,
                                      queue,
                                      (bf_tm_app_pool_t *)pool,
                                      base_use_limit,
                                      (bf_tm_queue_baf_t *)dynamic_baf,
                                      hysteresis);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_guaranteed_min_limit(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   p4_pd_tm_queue_t queue,
                                                   uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_guaranteed_min_limit_get(dev, port, queue, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_color_limit(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          p4_pd_tm_queue_t queue,
                                          p4_pd_color_t color,
                                          p4_pd_color_limit_t *limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_limit_get(dev,
                                   port,
                                   queue,
                                   (bf_tm_color_t)color,
                                   (bf_tm_queue_color_limit_t *)limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_q_color_hysteresis(p4_pd_tm_dev_t dev,
                                               p4_pd_tm_port_t port,
                                               p4_pd_tm_queue_t queue,
                                               p4_pd_color_t color,
                                               p4_pd_tm_thres_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_color_hysteresis_get(
      dev, port, queue, (bf_tm_color_t)color, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_uc_cut_through_limit(p4_pd_tm_dev_t dev,
                                                      p4_pd_tm_port_t port,
                                                      uint8_t *limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_uc_cut_through_limit_get(dev, port, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_total_ppg(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_pipe_t pipe,
                                      uint32_t *total_ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_totalppg_get(dev, pipe, total_ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_get_unused_ppg_count(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_pipe_t pipe,
                                             uint32_t *unused_ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_unusedppg_get(dev, pipe, unused_ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ppg_app_pool_usage(p4_pd_tm_dev_t dev,
                                               p4_pd_tm_ppg_t ppg,
                                               p4_pd_pool_id_t pool_id,
                                               uint32_t *base_use_limit,
                                               p4_pd_tm_ppg_baf_t *dynamic_baf,
                                               uint32_t *hysteresis) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_app_pool_usage_get(dev,
                                        ppg,
                                        (bf_tm_app_pool_t)pool_id,
                                        base_use_limit,
                                        (bf_tm_ppg_baf_t *)dynamic_baf,
                                        hysteresis);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ppg_guaranteed_min_limit(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_ppg_t ppg,
                                                     uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_guaranteed_min_limit_get(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ppg_skid_limit(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_ppg_t ppg,
                                           uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_skid_limit_get(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ppg_guaranteed_min_skid_hysteresis(
    p4_pd_tm_dev_t dev, p4_pd_tm_ppg_t ppg, uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_guaranteed_min_skid_hysteresis_get(dev, ppg, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_app_pool_size(p4_pd_tm_dev_t dev,
                                          p4_pd_pool_id_t pool_id,
                                          uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_size_get(dev, (bf_tm_app_pool_t)pool_id, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_app_pool_color_drop_limit(p4_pd_tm_dev_t dev,
                                                      p4_pd_pool_id_t pool_id,
                                                      p4_pd_color_t color,
                                                      uint32_t *limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_color_drop_limit_get(
      dev, (bf_tm_app_pool_t)pool_id, (bf_tm_color_t)color, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_app_pool_color_drop_hysteresis(p4_pd_tm_dev_t dev,
                                                           p4_pd_color_t color,
                                                           uint32_t *limit) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_pool_color_drop_hysteresis_get(dev, (bf_tm_color_t)color, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_app_pool_pfc_limit(p4_pd_tm_dev_t dev,
                                               p4_pd_pool_id_t pool_id,
                                               p4_pd_tm_icos_t icos,
                                               uint32_t *limit) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_pool_pfc_limit_get(dev, (bf_tm_app_pool_t)pool_id, icos, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_skid_pool_size(p4_pd_tm_dev_t dev,
                                           uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_skid_size_get(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_skid_pool_hysteresis(p4_pd_tm_dev_t dev,
                                                 uint32_t *limit) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_skid_hysteresis_get(dev, limit);
  return status;
}

p4_pd_status_t p4_pd_tm_get_negative_mirror_pool_size(p4_pd_tm_dev_t dev,
                                                      uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_mirror_on_drop_size_get(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_uc_cut_through_pool_size(p4_pd_tm_dev_t dev,
                                                     p4_pd_pool_id_t pool_id,
                                                     uint32_t *cells) {
  (void)pool_id;
  p4_pd_status_t status = 0;
  status = bf_tm_pool_uc_cut_through_size_get(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_mc_cut_through_pool_size(p4_pd_tm_dev_t dev,
                                                     p4_pd_pool_id_t pool_id,
                                                     uint32_t *cells) {
  p4_pd_status_t status = 0;
  (void)pool_id;
  status = bf_tm_pool_mc_cut_through_size_get(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ingress_buffer_limit(p4_pd_tm_dev_t dev,
                                                 uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_global_max_limit_get(dev, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ingress_buffer_limit_state(p4_pd_tm_dev_t dev,
                                                       bool *state) {
  p4_pd_status_t status = 0;
  status = bf_tm_global_max_limit_state_get(dev, state);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ingress_port_drop_limit(p4_pd_tm_dev_t dev,
                                                    p4_pd_tm_port_t port,
                                                    uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_ingress_drop_limit_get(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_egress_port_drop_limit(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_egress_drop_limit_get(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_ingress_port_hysteresis(p4_pd_tm_dev_t dev,
                                                    p4_pd_tm_port_t port,
                                                    uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_ingress_hysteresis_get(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_egress_port_hysteresis(p4_pd_tm_dev_t dev,
                                                   p4_pd_tm_port_t port,
                                                   uint32_t *cells) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_egress_hysteresis_get(dev, port, cells);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_flowcontrol_mode(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_flow_ctrl_type_t *fctype) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_flowcontrol_mode_get(
      dev, port, (bf_tm_flow_ctrl_type_t *)fctype);
  return status;
}

p4_pd_status_t p4_pd_tm_get_port_pfc_cos_mapping(p4_pd_tm_dev_t dev,
                                                 p4_pd_tm_port_t port,
                                                 uint8_t *cos_map) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_pfc_cos_mapping_get(dev, port, cos_map);
  return status;
}

p4_pd_status_t p4_pd_tm_set_timestamp_shift(p4_pd_tm_dev_t dev,
                                            uint8_t ts_shift) {
  p4_pd_status_t status = 0;
  status = bf_tm_timestamp_shift_set(dev, ts_shift);
  return status;
}

p4_pd_status_t p4_pd_tm_get_timestamp_shift(p4_pd_tm_dev_t dev,
                                            uint8_t *ts_shift) {
  p4_pd_status_t status = 0;
  status = bf_tm_timestamp_shift_get(dev, ts_shift);
  return status;
}

p4_pd_status_t p4_pd_tm_ppg_drop_get(p4_pd_tm_dev_t dev,
                                     p4_pd_tm_pipe_t pipe,
                                     p4_pd_tm_ppg_t ppg,
                                     uint64_t *count) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_drop_cache_get(dev, pipe, ppg, count);
  return status;
}

p4_pd_status_t p4_pd_tm_ppg_drop_count_clear(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_drop_count_clear(dev, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_q_drop_get(p4_pd_tm_dev_t dev,
                                   p4_pd_tm_pipe_t pipe,
                                   p4_pd_tm_port_t port,
                                   p4_pd_tm_queue_t queue,
                                   uint64_t *count) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_drop_cache_get(dev, pipe, port, queue, count);
  return status;
}

p4_pd_status_t p4_pd_tm_q_drop_count_clear(p4_pd_tm_dev_t dev,
                                           p4_pd_tm_port_t port,
                                           p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_drop_count_clear(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_pool_usage_get(p4_pd_tm_dev_t dev,
                                       p4_pd_pool_id_t pool_id,
                                       uint32_t *count,
                                       uint32_t *wm) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_usage_get(dev, (bf_tm_app_pool_t)pool_id, count, wm);
  return status;
}

p4_pd_status_t p4_pd_tm_pool_watermark_clear(p4_pd_tm_dev_t dev,
                                             p4_pd_pool_id_t pool_id) {
  p4_pd_status_t status = 0;
  status = bf_tm_pool_watermark_clear(dev, (bf_tm_app_pool_t)pool_id);
  return status;
}

p4_pd_status_t p4_pd_tm_q_usage_get(p4_pd_tm_dev_t dev,
                                    p4_pd_tm_pipe_t pipe,
                                    p4_pd_tm_port_t port,
                                    p4_pd_tm_queue_t queue,
                                    uint32_t *count,
                                    uint32_t *wm) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_usage_get(dev, pipe, port, queue, count, wm);
  return status;
}

p4_pd_status_t p4_pd_tm_q_watermark_clear(p4_pd_tm_dev_t dev,
                                          p4_pd_tm_port_t port,
                                          p4_pd_tm_queue_t queue) {
  p4_pd_status_t status = 0;
  status = bf_tm_q_watermark_clear(dev, port, queue);
  return status;
}

p4_pd_status_t p4_pd_tm_ppg_usage_get(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_pipe_t pipe,
                                      p4_pd_tm_ppg_t ppg,
                                      uint32_t *gmin_count,
                                      uint32_t *shared_count,
                                      uint32_t *skid_count,
                                      uint32_t *wm) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_usage_get(
      dev, pipe, ppg, gmin_count, shared_count, skid_count, wm);
  return status;
}

p4_pd_status_t p4_pd_tm_ppg_watermark_clear(p4_pd_tm_dev_t dev,
                                            p4_pd_tm_ppg_t ppg) {
  p4_pd_status_t status = 0;
  status = bf_tm_ppg_watermark_clear(dev, ppg);
  return status;
}

p4_pd_status_t p4_pd_tm_port_drop_get(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_pipe_t pipe,
                                      p4_pd_tm_port_t port,
                                      uint64_t *ig_count,
                                      uint64_t *eg_count) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_drop_cache_get(dev, pipe, port, ig_count, eg_count);
  return status;
}

p4_pd_status_t p4_pd_tm_port_drop_count_clear(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_drop_count_clear(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_port_ingress_drop_count_clear(p4_pd_tm_dev_t dev,
                                                      p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_ingress_drop_count_clear(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_port_egress_drop_count_clear(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_egress_drop_count_clear(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_port_usage_get(p4_pd_tm_dev_t dev,
                                       p4_pd_tm_pipe_t pipe,
                                       p4_pd_tm_port_t port,
                                       uint32_t *ig_count,
                                       uint32_t *eg_count,
                                       uint32_t *ig_wm,
                                       uint32_t *eg_wm) {
  p4_pd_status_t status = 0;
  status =
      bf_tm_port_usage_get(dev, pipe, port, ig_count, eg_count, ig_wm, eg_wm);
  return status;
}

p4_pd_status_t p4_pd_tm_port_watermark_clear(p4_pd_tm_dev_t dev,
                                             p4_pd_tm_port_t port) {
  p4_pd_status_t status = 0;
  status = bf_tm_port_watermark_clear(dev, port);
  return status;
}

p4_pd_status_t p4_pd_tm_stop_cache_counters_timer(p4_pd_tm_dev_t dev) {
  p4_pd_status_t status = 0;
  status = bf_tm_stop_cache_counters_timer(dev);
  return status;
}

p4_pd_status_t p4_pd_tm_start_cache_counters_timer(p4_pd_tm_dev_t dev) {
  p4_pd_status_t status = 0;
  status = bf_tm_start_cache_counters_timer(dev);
  return status;
}

static void copy_bf_to_pd_blklvl_cntrs(bf_tm_blklvl_cntrs_t *blk_cntrs,
                                       p4_pd_tm_blklvl_cntrs_t *pd_blk_cntrs) {
  pd_blk_cntrs->wac_no_dest_drop = blk_cntrs->wac_no_dest_drop;
  pd_blk_cntrs->qac_no_dest_drop = blk_cntrs->qac_no_dest_drop;
  pd_blk_cntrs->wac_buf_full_drop = blk_cntrs->wac_buf_full_drop;
  pd_blk_cntrs->egress_pipe_total_drop = blk_cntrs->egress_pipe_total_drop;
  pd_blk_cntrs->psc_pkt_drop = blk_cntrs->psc_pkt_drop;
  pd_blk_cntrs->pex_total_disc = blk_cntrs->pex_total_disc;
  pd_blk_cntrs->qac_total_disc = blk_cntrs->qac_total_disc;
  pd_blk_cntrs->total_disc_dq = blk_cntrs->total_disc_dq;
  pd_blk_cntrs->pre_total_drop = blk_cntrs->pre_total_drop;
  pd_blk_cntrs->qac_pre_mc_drop = blk_cntrs->qac_pre_mc_drop;
}

static void copy_bf_to_pd_pre_fifo_cntrs(
    bf_tm_pre_fifo_cntrs_t *fifo_cntrs,
    p4_pd_tm_pre_fifo_cntrs_t *pd_fifo_cntrs) {
  for (int i = 0; i < BF_PRE_FIFO_COUNT; i++) {
    pd_fifo_cntrs->wac_drop_cnt_pre0_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre0_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre1_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre1_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre2_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre2_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre3_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre3_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre4_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre4_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre5_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre5_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre6_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre6_fifo[i];
    pd_fifo_cntrs->wac_drop_cnt_pre7_fifo[i] =
        fifo_cntrs->wac_drop_cnt_pre7_fifo[i];
  }
}

p4_pd_status_t p4_pd_tm_blklvl_drop_get(p4_pd_tm_dev_t dev,
                                        p4_pd_tm_pipe_t pipe,
                                        p4_pd_tm_blklvl_cntrs_t *pd_blk_cntrs) {
  (void)pd_blk_cntrs;
  p4_pd_status_t status = 0;
  bf_tm_blklvl_cntrs_t bf_blk_cntrs;

  status = bf_tm_blklvl_drop_get(dev, pipe, &bf_blk_cntrs);
  if (status == 0) {
    copy_bf_to_pd_blklvl_cntrs(&bf_blk_cntrs, pd_blk_cntrs);
  }
  return status;
}

p4_pd_status_t p4_pd_tm_pre_fifo_drop_get(
    p4_pd_tm_dev_t dev, p4_pd_tm_pre_fifo_cntrs_t *fifo_cntrs) {
  (void)fifo_cntrs;
  p4_pd_status_t status = 0;
  bf_tm_pre_fifo_cntrs_t bf_pre_fifo_cntrs;

  status = bf_tm_pre_fifo_drop_get(dev, &bf_pre_fifo_cntrs);
  if (status == 0) {
    copy_bf_to_pd_pre_fifo_cntrs(&bf_pre_fifo_cntrs, fifo_cntrs);
  }
  return status;
}

p4_pd_status_t p4_pd_tm_q_visible_set(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_port_t port,
                                      p4_pd_tm_queue_t queue,
                                      bool visible) {
  p4_pd_status_t status = 0;

  status = bf_tm_q_visible_set(dev, port, queue, visible);
  return status;
}

p4_pd_status_t p4_pd_tm_q_visible_get(p4_pd_tm_dev_t dev,
                                      p4_pd_tm_port_t port,
                                      p4_pd_tm_queue_t queue,
                                      bool *visible) {
  p4_pd_status_t status = 0;

  status = bf_tm_q_visible_get(dev, port, queue, visible, visible);
  return status;
}

p4_pd_status_t p4_pd_tm_qstat_report_mode_set(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_pipe_t pipe,
                                              bool mode) {
  p4_pd_status_t status = 0;

  status = bf_tm_qstat_report_mode_set(dev, pipe, mode);
  return status;
}
p4_pd_status_t p4_pd_tm_qstat_report_mode_get(p4_pd_tm_dev_t dev,
                                              p4_pd_tm_pipe_t pipe,
                                              bool *mode) {
  p4_pd_status_t status = 0;

  status = bf_tm_qstat_report_mode_get(dev, pipe, mode);
  return status;
}

p4_pd_status_t pd_tm_pipe_deflection_port_enable_set(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_pipe_t pipe,
                                                     bool enable) {
  p4_pd_status_t status = 0;

  status = bf_tm_pipe_deflection_port_enable_set(dev, pipe, enable);
  return status;
}
p4_pd_status_t pd_tm_pipe_deflection_port_enable_get(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_pipe_t pipe,
                                                     bool *enable) {
  p4_pd_status_t status = 0;

  status = bf_tm_pipe_deflection_port_enable_get(dev, pipe, enable);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_q_adv_fc_mode_set(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    p4_pd_tm_sched_adv_fc_mode_t mode) {
  p4_pd_status_t status = 0;
  bf_tm_sched_adv_fc_mode_t bf_mode = (bf_tm_sched_adv_fc_mode_t)mode;

  status = bf_tm_sched_q_adv_fc_mode_set(dev, port, queue, bf_mode);
  return status;
}
p4_pd_status_t p4_pd_tm_sched_q_adv_fc_mode_get(
    p4_pd_tm_dev_t dev,
    p4_pd_tm_port_t port,
    p4_pd_tm_queue_t queue,
    p4_pd_tm_sched_adv_fc_mode_t *mode) {
  p4_pd_status_t status = 0;
  bf_tm_sched_adv_fc_mode_t bf_mode;

  status = bf_tm_sched_q_adv_fc_mode_get(dev, port, queue, &bf_mode);
  if (mode) {
    *mode = (p4_pd_tm_sched_adv_fc_mode_t)bf_mode;
  }
  return status;
}

p4_pd_status_t p4_pd_tm_sched_adv_fc_mode_enable_set(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_pipe_t pipe,
                                                     bool enable) {
  p4_pd_status_t status = 0;

  status = bf_tm_sched_adv_fc_mode_enable_set(dev, pipe, enable);
  return status;
}

p4_pd_status_t p4_pd_tm_sched_adv_fc_mode_enable_get(p4_pd_tm_dev_t dev,
                                                     p4_pd_tm_pipe_t pipe,
                                                     bool *enable) {
  p4_pd_status_t status = 0;

  status = bf_tm_sched_adv_fc_mode_enable_get(dev, pipe, enable);
  return status;
}

p4_pd_status_t p4_pd_tm_dev_config_get(p4_pd_tm_dev_t dev,
                                       p4_pd_tm_dev_cfg_t *cfg) {
  p4_pd_status_t status = 0;
  bf_tm_dev_cfg_t tm_cfg;
  status = bf_tm_dev_config_get(dev, &tm_cfg);

  cfg->l1_per_pg = tm_cfg.l1_per_pg;
  cfg->l1_per_pipe = tm_cfg.l1_per_pipe;
  cfg->pfc_ppg_per_pipe = tm_cfg.pfc_ppg_per_pipe;
  cfg->pg_per_pipe = tm_cfg.pg_per_pipe;
  cfg->pipe_cnt = tm_cfg.pipe_cnt;
  cfg->ports_per_pg = tm_cfg.ports_per_pg;
  cfg->q_per_pg = tm_cfg.q_per_pg;
  cfg->total_ppg_per_pipe = tm_cfg.total_ppg_per_pipe;
  cfg->pre_fifo_per_pipe = tm_cfg.pre_fifo_per_pipe;
  return status;
}
