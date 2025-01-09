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


#ifndef _PIPE_MGR_TOF3_PKTGEN_H
#define _PIPE_MGR_TOF3_PKTGEN_H

#define PIPE_MGR_TOF3_PKTGEN_APP_CNT 16

/* Tofino2 */
struct pipe_mgr_tof3_pg_app_ctx {
  uint32_t app_ctrl;
  uint32_t payload_ctrl;
  uint32_t ing_port;
  uint32_t recir_val[4];
  uint32_t recir_msk[4];
  uint32_t event_num;
  uint32_t ibg;
  uint32_t ibg_jit_max;
  uint32_t ibg_jit_scale;
  uint32_t ipg;
  uint32_t ipg_jit_max;
  uint32_t ipg_jit_scale;
  uint32_t event_timer;
};

struct pipe_mgr_tof3_pg_app_cfg {
  /* Indexed by logical pipe. */
  /* Active config. */
  struct pipe_mgr_tof3_pg_app_ctx **a;
  /* Backup config (in case of txn abort). */
  struct pipe_mgr_tof3_pg_app_ctx **b;
  bool b_valid;
};

struct pipe_mgr_tof3_pfc_ctx {
  uint32_t pfc_hdr[4];
  uint32_t pfc_timer;
  uint32_t pfc_max_pkt_size;
};

struct pipe_mgr_tof3_pg_dev_ctx {
  /* eth_cpu port ctrl */
  uint32_t port_ctrl;
  /* chnl speed. 4 pipes, each has 8 chnls, for calculate seq */
  uint32_t **ipb_chnl_sp;
  /* ipb_port_ctrl */
  uint32_t *ipb_ctrl;
  /* ebuf chnl enable/disable */
  uint8_t *ebuf_chnl_en;
  /* ebuf port ctrl */
  uint32_t **ebuf_port_ctrl;
  uint32_t *app_recirc_src;
  /* pfc */
  struct pipe_mgr_tof3_pfc_ctx *pfc;
  /* Shadow per logical pipe of port down replay config. */
  bf_pktgen_port_down_mode_t *port_down_mode;
  /* Shadow per logical pipe of port down masks.  The first two masks are the
   * two user configurable masks.  The third mask is a map of added ports and
   * is used to mask the user provided masks. */
  struct bf_tof2_port_down_sel **port_down_mask;
  struct pipe_mgr_tof3_pg_app_cfg app;
  struct pkt_buffer_shadow_t *pkt_buffer_shadow;
};

bf_status_t pipe_mgr_tof3_pktgen_add_dev(bf_session_hdl_t shdl,
                                         bf_dev_id_t dev);
bf_status_t pipe_mgr_tof3_pktgen_rmv_dev(bf_dev_id_t dev);
bf_status_t pipe_mgr_tof3_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                                 bf_dev_id_t dev);
bf_status_t pipe_mgr_tof3_pktgen_port_add(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id,
                                          bf_port_speeds_t speed);
bf_status_t pipe_mgr_tof3_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id);
bf_status_t pipe_mgr_tof3_recir_en(bf_session_hdl_t shdl,
                                   bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bool en);
bool pipe_mgr_tof3_pgr_recir_get(bf_dev_id_t dev, bf_dev_port_t port);
bf_status_t pipe_mgr_tof3_pktgen_en(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool en);
bf_status_t pipe_mgr_tof3_pktgen_get_port_en(rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port,
                                             bool *is_enabled);
bf_status_t pipe_mgr_tof3_pktgen_pgr_com_port_down_clr(pipe_sess_hdl_t sid,
                                                       int dev,
                                                       int logical_pipe,
                                                       int local_port_bit_idx);
bf_status_t pipe_mgr_tof3_pktgen_pgr_com_port_down_clr_get(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int local_port_bit_idx,
    bool *is_cleared);
void pipe_mgr_tof3_pktgen_txn_commit(int dev);
void pipe_mgr_tof3_pktgen_txn_abort(int dev, int max_app_id, int active_pipes);
void pipe_mgr_tof3_pkt_buffer_shadow_mem_update(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t offset,
                                                const uint8_t *buf,
                                                uint32_t size,
                                                bool txn);

bf_status_t pipe_mgr_tof3_pkt_buffer_shadow_mem_get(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t offset,
                                                    uint8_t *buf,
                                                    uint32_t size);
bf_status_t pipe_mgr_tof3_pkt_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                       bf_dev_id_t dev,
                                                       bf_dev_pipe_t log_pipe);
bf_status_t pipe_mgr_tof3_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int row,
                                                  int num_rows);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_batch_ctr(int dev,
                                                   int logical_pipe,
                                                   int aid,
                                                   uint64_t *val);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_pkt_ctr(int dev,
                                                 int logical_pipe,
                                                 int aid,
                                                 uint64_t *val);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_trig_ctr(int dev,
                                                  int logical_pipe,
                                                  int aid,
                                                  uint64_t *val);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_batch_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_pkt_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val);
bf_status_t pipe_mgr_tof3_pktgen_reg_app_trig_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val);
bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ctrl_en(
    pipe_sess_hdl_t sid, int dev, bf_dev_pipe_t logical_pipe, int aid, bool en);
bf_status_t pipe_mgr_tof3_pktgen_get_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                 int app_id,
                                                 bool *is_enabled);
bf_status_t pipe_mgr_tof3_pktgen_cfg_app_conf_check(rmt_dev_info_t *dev_info,
                                                    bf_dev_target_t dev_tgt,
                                                    int app_id,
                                                    bf_pktgen_app_cfg_t *cfg);
bf_status_t pipe_mgr_tof3_pktgen_cfg_app(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         int app_id,
                                         bf_pktgen_app_cfg_t *cfg);

bf_status_t pipe_mgr_tof3_pktgen_cfg_app_get(bf_dev_target_t dev_tgt,
                                             int app_id,
                                             bf_pktgen_app_cfg_t *cfg);

bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_mask(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk);

bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_mask_get(
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk);
bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_replay(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode);
bf_status_t pipe_mgr_tof3_pktgen_get_port_down_replay(
    bf_session_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode);
#endif
