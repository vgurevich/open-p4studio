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


#ifndef _PIPE_MGR_TOF_PKTGEN_H
#define _PIPE_MGR_TOF_PKTGEN_H

#define PIPE_MGR_TOF_PKTGEN_APP_CNT 8
#define PIPE_MGR_TOF_PKTGEN_PKT_LEN_MIN 21

/* Tofino */
struct pipe_mgr_pg_app_ctx {
  uint32_t app_ctrl;
  uint32_t payload_ctrl;
  uint32_t ing_port;
  uint32_t recir_val;
  uint32_t recir_msk;
  uint32_t event_num;
  uint32_t ibg;
  uint32_t ibg_jit_val;
  uint32_t ibg_jit_msk;
  uint32_t ipg;
  uint32_t ipg_jit_val;
  uint32_t ipg_jit_msk;
  uint32_t timer;
};

struct pipe_mgr_pg_app_cfg {
  /* Indexed by logical pipe. */
  /* Active config. */
  struct pipe_mgr_pg_app_ctx **a;
  /* Backup config (in case of txn abort). */
  struct pipe_mgr_pg_app_ctx **b;
  bool b_valid;
};

struct pipe_mgr_tof_pg_dev_ctx {
  /* All indexed by logical pipe. */
  int **port16_chan;
  int **port17_chan;
  uint32_t *port16_ctrl;
  uint32_t *port17_ctrl1;
  uint32_t *port17_ctrl2;
  struct pipe_mgr_pg_app_cfg app;
  struct pkt_buffer_shadow_t *pkt_buffer_shadow;
};

bf_status_t pipe_mgr_tof_pktgen_add_dev(bf_session_hdl_t shdl, bf_dev_id_t dev);
bf_status_t pipe_mgr_tof_pktgen_rmv_dev(bf_dev_id_t dev);
bf_status_t pipe_mgr_tof_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                                bf_dev_id_t dev);
bf_status_t pipe_mgr_tof_pktgen_port_add(rmt_dev_info_t *dev_info,
                                         bf_dev_port_t port_id,
                                         bf_port_speeds_t speed);
bf_status_t pipe_mgr_tof_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                         bf_dev_port_t port_id);
bf_status_t pipe_mgr_tof_set_recirc_16_en(bf_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool en);
bool pipe_mgr_tof_pktgen_reg_pgr_com_port16_get_recirc(bf_dev_target_t dev_tgt);
bf_status_t pipe_mgr_tof_pktgen_set_port_en(bf_session_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bool en);
bf_status_t pipe_mgr_tof_pktgen_get_port_en(rmt_dev_info_t *dev_info,
                                            bf_dev_port_t port,
                                            bool *en);

bf_status_t pipe_mgr_tof_pktgen_set_rpm_en(bf_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bool en);
bf_status_t pipe_mgr_tof_pktgen_get_rpm_en(rmt_dev_info_t *dev_info,
                                           bf_dev_port_t port,
                                           bool *en);

bf_status_t pipe_mgr_tof_pktgen_pgr_com_port_down_clr(pipe_sess_hdl_t sid,
                                                      bf_dev_target_t dev_tgt,
                                                      int local_port_bit_idx);
bf_status_t pipe_mgr_tof_pktgen_pgr_com_port_down_clr_get(
    bf_dev_target_t dev_tgt, int local_port_bit_idx, bool *is_cleared);
bf_status_t pipe_mgr_tof_pktgen_set_app_ctrl_en(pipe_sess_hdl_t sid,
                                                bf_dev_target_t dev_tgt,
                                                int aid,
                                                bool en);

bf_status_t pipe_mgr_tof_pktgen_get_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                int app_id,
                                                bool *is_enabled);
void pipe_mgr_tof_pktgen_txn_commit(bf_dev_id_t dev);
void pipe_mgr_tof_pktgen_txn_abort(bf_dev_id_t dev,
                                   int max_app_id,
                                   int active_pipes);
void pipe_mgr_tof_pkt_buffer_shadow_mem_update(bf_dev_target_t dev_tgt,
                                               uint32_t offset,
                                               const uint8_t *buf,
                                               uint32_t size,
                                               bool txn);

bf_status_t pipe_mgr_tof_pkt_buffer_shadow_mem_get(bf_dev_target_t dev_tgt,
                                                   uint32_t offset,
                                                   uint8_t *buf,
                                                   uint32_t size);

bf_status_t pipe_mgr_tof_pkt_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                      bf_dev_target_t dev_tgt);
bf_status_t pipe_mgr_tof_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                                 bf_dev_target_t dev_tgt,
                                                 int row,
                                                 int num_rows);
bf_status_t pipe_mgr_tof_pktgen_reg_app_batch_ctr(bf_dev_target_t dev_tgt,
                                                  int aid,
                                                  uint64_t *val);
bf_status_t pipe_mgr_tof_pktgen_reg_app_pkt_ctr(bf_dev_target_t dev_tgt,
                                                int aid,
                                                uint64_t *val);
bf_status_t pipe_mgr_tof_pktgen_reg_app_trig_ctr(bf_dev_target_t dev_tgt,
                                                 int aid,
                                                 uint64_t *val);
bf_status_t pipe_mgr_tof_pktgen_reg_app_batch_ctr_set(bf_session_hdl_t shdl,
                                                      bf_dev_target_t dev_tgt,
                                                      int aid,
                                                      uint64_t val);
bf_status_t pipe_mgr_tof_pktgen_reg_app_pkt_ctr_set(bf_session_hdl_t shdl,
                                                    bf_dev_target_t dev_tgt,
                                                    int aid,
                                                    uint64_t val);
bf_status_t pipe_mgr_tof_pktgen_reg_app_trig_ctr_set(bf_session_hdl_t shdl,
                                                     bf_dev_target_t dev_tgt,
                                                     int aid,
                                                     uint64_t val);
bf_status_t pipe_mgr_tof_pktgen_cfg_app_conf_check(rmt_dev_info_t *dev_info,
                                                   bf_dev_target_t dev_tgt,
                                                   int app_id,
                                                   bf_pktgen_app_cfg_t *cfg);
bf_status_t pipe_mgr_tof_pktgen_cfg_app(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        bf_pktgen_app_cfg_t *cfg);

bf_status_t pipe_mgr_tof_pktgen_cfg_app_get(bf_dev_target_t dev_tgt,
                                            int app_id,
                                            bf_pktgen_app_cfg_t *cfg);
#endif
