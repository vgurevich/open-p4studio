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


#ifndef __PIPE_MGR_PARDE_H__
#define __PIPE_MGR_PARDE_H__

#include "pipe_mgr_tof_ibuf.h"
#include "pipe_mgr_tof_ebuf.h"
#include "pipe_mgr_tof_parb.h"
#include "pipe_mgr_tof_prsr.h"
#include "pipe_mgr_tof_deprsr.h"

struct pipe_mgr_tof_ebuf_pipe_reg_ctx;
struct pipe_mgr_tof2_ebuf_pipe_reg_ctx;
struct pipe_mgr_tof3_ebuf_pipe_reg_ctx;

struct pipe_mgr_tof_ebuf_ctx {
  /* Allocated based on number of pipes. */
  struct pipe_mgr_tof_ebuf_pipe_reg_ctx *reg;
};
struct pipe_mgr_tof2_ebuf_ctx {
  /* Allocated based on number of pipes. */
  struct pipe_mgr_tof2_ebuf_pipe_reg_ctx *reg;
};
struct pipe_mgr_tof3_ebuf_ctx {
  /* Allocated based on number of pipes. */
  struct pipe_mgr_tof3_ebuf_pipe_reg_ctx *reg;
};
union pipe_mgr_ebuf_ctx {
  struct pipe_mgr_tof_ebuf_ctx tof;
  struct pipe_mgr_tof2_ebuf_ctx tof2;
  struct pipe_mgr_tof3_ebuf_ctx tof3;
};
union pipe_mgr_deprsr_ctx {
  struct pipe_mgr_tof_deprsr_ctx tof;
};

pipe_status_t pipe_mgr_parde_device_add(pipe_sess_hdl_t shdl,
                                        rmt_dev_info_t *dev_info);

void pipe_mgr_parde_device_rmv(rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_parde_port_add(pipe_sess_hdl_t shdl,
                                      rmt_dev_info_t *dev_info,
                                      bf_port_cb_direction_t direction,
                                      bf_dev_port_t port_id);

pipe_status_t pipe_mgr_parde_port_rmv(pipe_sess_hdl_t shdl,
                                      rmt_dev_info_t *dev_info,
                                      bf_port_cb_direction_t direction,
                                      bf_dev_port_t port_id);

pipe_status_t pipe_mgr_parde_complete_port_mode_transition_wa(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info, bf_dev_port_t port_id);

pipe_status_t pipe_mgr_parde_traffic_disable(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_parde_traffic_enable(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_parde_wait_for_traffic_flush(pipe_sess_hdl_t shdl,
                                                    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_parde_set_port_cut_through(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  bf_dev_port_t port_id,
                                                  bool cut_through_enabled);

pipe_status_t pipe_mgr_parde_port_set_drop_threshold(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     bf_dev_port_t port_id,
                                                     uint32_t drop_hi_thrd,
                                                     uint32_t drop_low_thrd);

pipe_status_t pipe_mgr_parde_port_set_afull_threshold(pipe_sess_hdl_t sess_hdl,
                                                      bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id,
                                                      uint32_t afull_hi_thrd,
                                                      uint32_t afull_low_thrd);

pipe_status_t pipe_mgr_parser_config_create_dma(pipe_sess_hdl_t sess_hdl,
                                                rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_parde_port_ebuf_counter_get(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id,
                                                   uint64_t *value);

pipe_status_t pipe_mgr_parde_port_ebuf_bypass_counter_get(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint64_t *value);

pipe_status_t pipe_mgr_parde_port_ebuf_100g_credits_get(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint64_t *value);

pipe_status_t pipe_mgr_parde_iprsr_pri_threshold_set(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id,
                                                     uint32_t threshold);

pipe_status_t pipe_mgr_parde_iprsr_pri_threshold_get(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id,
                                                     uint32_t *threshold);
#endif
