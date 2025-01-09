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


#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_ms.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pktgen_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_reg_if.h>

p4_pd_status_t p4_pd_init(void) { return pipe_mgr_init(); }

void p4_pd_cleanup(void) { pipe_mgr_cleanup(); }

p4_pd_status_t p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl) {
  return pipe_mgr_client_init(sess_hdl);
}

p4_pd_status_t p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl) {
  return pipe_mgr_client_cleanup(sess_hdl);
}

p4_pd_status_t p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic) {
  p4_pd_ms_begin_txn(shdl);
  return pipe_mgr_begin_txn(shdl, isAtomic);
}

p4_pd_status_t p4_pd_verify_txn(p4_pd_sess_hdl_t shdl) {
  return pipe_mgr_verify_txn(shdl);
}

p4_pd_status_t p4_pd_abort_txn(p4_pd_sess_hdl_t shdl) {
  p4_pd_ms_abort_txn(shdl);
  return pipe_mgr_abort_txn(shdl);
}

p4_pd_status_t p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous) {
  p4_pd_ms_commit_txn(shdl);
  return pipe_mgr_commit_txn(shdl, hwSynchronous);
}

p4_pd_status_t p4_pd_complete_operations(p4_pd_sess_hdl_t shdl) {
  return pipe_mgr_complete_operations(shdl);
}

p4_pd_status_t p4_pd_begin_batch(p4_pd_sess_hdl_t shdl) {
  return pipe_mgr_begin_batch(shdl);
}

p4_pd_status_t p4_pd_flush_batch(p4_pd_sess_hdl_t shdl) {
  return pipe_mgr_flush_batch(shdl);
}

p4_pd_status_t p4_pd_end_batch(p4_pd_sess_hdl_t shdl, bool hwSynchronous) {
  return pipe_mgr_end_batch(shdl, hwSynchronous);
}

p4_pd_status_t p4_pd_log_state(bf_dev_id_t dev, const char *filepath) {
  return bf_device_log(dev, filepath);
}

p4_pd_status_t p4_pd_restore_state(bf_dev_id_t dev, const char *filepath) {
  return bf_device_restore(dev, filepath);
}

p4_pd_status_t p4_pd_advance_model_time(p4_pd_sess_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        uint64_t tick_time) {
  return pipe_mgr_model_time_advance(shdl, dev, tick_time);
}

p4_pd_status_t p4_pd_recirculation_enable(p4_pd_sess_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          uint32_t port) {
  return bf_recirculation_enable(shdl, dev, port);
}

p4_pd_status_t p4_pd_recirculation_disable(p4_pd_sess_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           uint32_t port) {
  return bf_recirculation_disable(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_enable(p4_pd_sess_hdl_t shdl,
                                   bf_dev_id_t dev,
                                   uint32_t port) {
  return bf_pktgen_enable(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_disable(p4_pd_sess_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    uint32_t port) {
  return bf_pktgen_disable(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_enable_state_get(p4_pd_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             uint32_t port,
                                             bool *enabled) {
  return bf_pktgen_enable_get(shdl, dev, port, enabled);
}

p4_pd_status_t p4_pd_pktgen_enable_recirc_pattern_matching(
    p4_pd_sess_hdl_t shdl, bf_dev_id_t dev, uint32_t port) {
  return bf_pktgen_enable_recirc_pattern_matching(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_disable_recirc_pattern_matching(
    p4_pd_sess_hdl_t shdl, bf_dev_id_t dev, uint32_t port) {
  return bf_pktgen_disable_recirc_pattern_matching(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_recirc_pattern_matching_state_get(
    p4_pd_sess_hdl_t shdl, bf_dev_id_t dev, uint32_t port, bool *enabled) {
  return bf_pktgen_recirc_pattern_matching_get(shdl, dev, port, enabled);
}

p4_pd_status_t p4_pd_pktgen_clear_port_down(p4_pd_sess_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            uint32_t port) {
  return bf_pktgen_clear_port_down(shdl, dev, port);
}

p4_pd_status_t p4_pd_pktgen_port_down_get(p4_pd_sess_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          uint32_t port,
                                          bool *is_cleared) {
  return bf_pktgen_port_down_get(shdl, dev, port, is_cleared);
}

p4_pd_status_t p4_pd_pktgen_cfg_app(p4_pd_sess_hdl_t shdl,
                                    p4_pd_dev_target_t dev_tgt,
                                    uint32_t app_id,
                                    p4_pd_pktgen_app_cfg cfg) {
  bf_dev_target_t bf_dev_tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  bf_pktgen_app_cfg_t bf_cfg = {0};
  pd_cfg_to_bf_pktgen_app_cfg(&bf_cfg, &cfg);
  return bf_pktgen_cfg_app(shdl, bf_dev_tgt, app_id, &bf_cfg);
}

p4_pd_status_t p4_pd_pktgen_cfg_app_tof2(p4_pd_sess_hdl_t shdl,
                                         p4_pd_dev_target_t dev_tgt,
                                         uint32_t app_id,
                                         p4_pd_pktgen_app_cfg_tof2 cfg) {
  bf_dev_target_t bf_dev_tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  bf_pktgen_app_cfg_t bf_cfg = {0};
  pd_cfg_to_bf_pktgen_app_cfg_tof2(&bf_cfg, &cfg);
  return bf_pktgen_cfg_app(shdl, bf_dev_tgt, app_id, &bf_cfg);
}

p4_pd_status_t p4_pd_pktgen_cfg_app_get(p4_pd_sess_hdl_t shdl,
                                        p4_pd_dev_target_t dev_tgt,
                                        uint32_t app_id,
                                        p4_pd_pktgen_app_cfg *cfg) {
  bf_dev_target_t bf_dev_tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  bf_pktgen_app_cfg_t bf_cfg = {0};
  p4_pd_status_t status =
      (p4_pd_status_t)bf_pktgen_cfg_app_get(shdl, bf_dev_tgt, app_id, &bf_cfg);
  if (status == BF_SUCCESS) {
    bf_cfg_to_pd_pktgen_app_cfg(cfg, &bf_cfg);
  }
  return status;
}

p4_pd_status_t p4_pd_pktgen_cfg_app_tof2_get(p4_pd_sess_hdl_t shdl,
                                             p4_pd_dev_target_t dev_tgt,
                                             uint32_t app_id,
                                             p4_pd_pktgen_app_cfg_tof2 *cfg) {
  bf_dev_target_t bf_dev_tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  bf_pktgen_app_cfg_t bf_cfg = {0};
  p4_pd_status_t status =
      (p4_pd_status_t)bf_pktgen_cfg_app_get(shdl, bf_dev_tgt, app_id, &bf_cfg);
  if (status == BF_SUCCESS) {
    bf_cfg_to_pd_pktgen_app_cfg_tof2(cfg, &bf_cfg);
  }
  return status;
}

p4_pd_status_t p4_pd_pktgen_port_down_msk_tof2(p4_pd_sess_hdl_t shdl,
                                               p4_pd_dev_target_t dev_tgt,
                                               uint32_t port_mask_sel,
                                               p4_pd_port_down_mask_tof2 mask) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  struct bf_tof2_port_down_sel sel;
  const int size = sizeof(mask.port_mask) / sizeof(mask.port_mask[0]);
  memcpy(sel.port_mask, mask.port_mask, size);
  return bf_pktgen_cfg_port_down_mask(shdl, tgt, port_mask_sel, &sel);
}

p4_pd_status_t p4_pd_pktgen_port_down_msk_tof2_get(
    p4_pd_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    p4_pd_port_down_mask_tof2 *mask) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  struct bf_tof2_port_down_sel sel;

  p4_pd_status_t sts = bf_pktgen_port_down_mask_get(tgt, port_mask_sel, &sel);
  const int size = sizeof(sel.port_mask) / sizeof(sel.port_mask[0]);
  if (sts == BF_SUCCESS) memcpy(mask->port_mask, sel.port_mask, size);
  return sts;
}

p4_pd_status_t p4_pd_pktgen_port_down_replay_mode_set(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_pktgen_port_down_mode_t mode) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_port_down_replay_mode_set(
      shdl, tgt, (bf_pktgen_port_down_mode_t)mode);
}

p4_pd_status_t p4_pd_pktgen_port_down_replay_mode_get(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_pktgen_port_down_mode_t *mode) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_port_down_replay_mode_get(
      shdl, tgt, (bf_pktgen_port_down_mode_t *)mode);
}

p4_pd_status_t p4_pd_pktgen_app_enable(p4_pd_sess_hdl_t shdl,
                                       p4_pd_dev_target_t dev_tgt,
                                       uint32_t app_id) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_app_enable(shdl, tgt, app_id);
}

p4_pd_status_t p4_pd_pktgen_app_disable(p4_pd_sess_hdl_t shdl,
                                        p4_pd_dev_target_t dev_tgt,
                                        uint32_t app_id) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_app_disable(shdl, tgt, app_id);
}

p4_pd_status_t p4_pd_pktgen_app_enable_state_get(p4_pd_sess_hdl_t shdl,
                                                 p4_pd_dev_target_t dev_tgt,
                                                 uint32_t app_id,
                                                 bool *enabled) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_app_get(shdl, tgt, app_id, enabled);
}

p4_pd_status_t p4_pd_pktgen_write_pkt_buffer(p4_pd_sess_hdl_t shdl,
                                             p4_pd_dev_target_t dev_tgt,
                                             uint32_t offset,
                                             uint32_t size,
                                             uint8_t *buf) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_write_pkt_buffer(shdl, tgt, offset, size, buf);
}

p4_pd_status_t p4_pd_pktgen_read_pkt_buffer(p4_pd_sess_hdl_t shdl,
                                            p4_pd_dev_target_t dev_tgt,
                                            uint32_t offset,
                                            uint32_t size,
                                            uint8_t *buf) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_pkt_buffer_get(shdl, tgt, offset, size, buf);
}

p4_pd_status_t p4_pd_pktgen_get_batch_counter(p4_pd_sess_hdl_t shdl,
                                              p4_pd_dev_target_t dev_tgt,
                                              uint32_t app_id,
                                              uint64_t *count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_get_batch_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_pktgen_get_pkt_counter(p4_pd_sess_hdl_t shdl,
                                            p4_pd_dev_target_t dev_tgt,
                                            uint32_t app_id,
                                            uint64_t *count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_get_pkt_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_pktgen_get_trigger_counter(p4_pd_sess_hdl_t shdl,
                                                p4_pd_dev_target_t dev_tgt,
                                                uint32_t app_id,
                                                uint64_t *count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_get_trigger_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_pktgen_set_batch_counter(p4_pd_sess_hdl_t shdl,
                                              p4_pd_dev_target_t dev_tgt,
                                              uint32_t app_id,
                                              uint64_t count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_set_batch_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_pktgen_set_pkt_counter(p4_pd_sess_hdl_t shdl,
                                            p4_pd_dev_target_t dev_tgt,
                                            uint32_t app_id,
                                            uint64_t count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_set_pkt_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_pktgen_set_trigger_counter(p4_pd_sess_hdl_t shdl,
                                                p4_pd_dev_target_t dev_tgt,
                                                uint32_t app_id,
                                                uint64_t count) {
  bf_dev_target_t tgt = {dev_tgt.device_id, dev_tgt.dev_pipe_id};
  return bf_pktgen_set_trigger_counter(shdl, tgt, app_id, count);
}

p4_pd_status_t p4_pd_parser_id_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t port,
                                   uint8_t *parser_id) {
  return pipe_mgr_parser_id_get(dev_id, port, parser_id);
}

p4_pd_status_t p4_pd_pipe_id_get(bf_dev_id_t device_id,
                                 bf_dev_port_t port,
                                 bf_dev_pipe_t *pipe_id) {
  return pipe_mgr_pipe_id_get(device_id, port, pipe_id);
}

p4_pd_status_t p4_pd_reg_wr(bf_dev_id_t dev, uint32_t addr, uint32_t data) {
  return lld_write_register(dev, addr, data);
}
uint32_t p4_pd_reg_rd(bf_dev_id_t dev, uint32_t addr) {
  uint32_t r = 0;
  lld_read_register(dev, addr, &r);
  return r;
}
p4_pd_status_t p4_pd_ind_reg_wr(bf_dev_id_t dev,
                                uint64_t addr,
                                uint64_t data_hi,
                                uint64_t data_lo) {
  return lld_ind_write(dev, addr, data_hi, data_lo);
}
void p4_pd_ind_reg_rd(bf_dev_id_t dev,
                      uint64_t addr,
                      uint64_t *data_hi,
                      uint64_t *data_lo) {
  lld_ind_read(dev, addr, data_hi, data_lo);
}

p4_pd_status_t p4_pd_tcam_scrub_timer_set(bf_dev_id_t dev,
                                          uint32_t msec_timer) {
  return pipe_mgr_tcam_scrub_timer_set(dev, msec_timer);
}

uint32_t p4_pd_tcam_scrub_timer_get(bf_dev_id_t dev) {
  return pipe_mgr_tcam_scrub_timer_get(dev);
}

p4_pd_status_t p4_pd_flow_lrn_set_intr_mode(p4_pd_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev,
                                            bool en) {
  return pipe_mgr_flow_lrn_set_intr_mode(sess_hdl, dev, en);
}

p4_pd_status_t p4_pd_flow_lrn_get_intr_mode(p4_pd_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev,
                                            bool *en) {
  return pipe_mgr_flow_lrn_get_intr_mode(sess_hdl, dev, en);
}

p4_pd_status_t p4_pd_gfm_test_pattern_set(p4_pd_sess_hdl_t sess_hdl,
                                          p4_pd_dev_target_t dev_tgt,
                                          uint32_t pipe_api_flags,
                                          bf_dev_direction_t gress,
                                          dev_stage_t stage_id,
                                          int num_patterns,
                                          uint64_t *row_patterns,
                                          uint64_t *row_bad_parity) {
  bf_dev_target_t dt = {.device_id = dev_tgt.device_id,
                        .dev_pipe_id = dev_tgt.dev_pipe_id};
  return pipe_mgr_gfm_test_pattern_set(sess_hdl,
                                       dt,
                                       pipe_api_flags,
                                       gress,
                                       stage_id,
                                       num_patterns,
                                       row_patterns,
                                       row_bad_parity);
}

p4_pd_status_t p4_pd_gfm_test_col_set(p4_pd_sess_hdl_t sess_hdl,
                                      p4_pd_dev_target_t dev_tgt,
                                      uint32_t pipe_api_flags,
                                      bf_dev_direction_t gress,
                                      dev_stage_t stage_id,
                                      int column,
                                      uint16_t col_data[64]) {
  bf_dev_target_t dt = {.device_id = dev_tgt.device_id,
                        .dev_pipe_id = dev_tgt.dev_pipe_id};
  return pipe_mgr_gfm_test_col_set(
      sess_hdl, dt, pipe_api_flags, gress, stage_id, column, col_data);
}
