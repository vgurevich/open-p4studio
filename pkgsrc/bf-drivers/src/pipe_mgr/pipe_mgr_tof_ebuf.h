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


#ifndef __PIPE_MGR_TOF_EBUF_H__
#define __PIPE_MGR_TOF_EBUF_H__

#include <pipe_mgr/pipe_mgr_intf.h>

#define BF_TOF_EBUF_DISPATCH_FIFO_NUM_ENTRIES 24

pipe_status_t pipe_mgr_ebuf_tof_dev_add(rmt_dev_info_t *dev_info);
void pipe_mgr_ebuf_tof_dev_rmv(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ebuf_tof_set_port_cut_through(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port,
                                                     bool cut_through_enabled);
pipe_status_t pipe_mgr_ebuf_tof_set_port_chnl_ctrl(rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port);
pipe_status_t pipe_mgr_ebuf_tof_disable_port_chnl(rmt_dev_info_t *dev_info,
                                                  bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ebuf_tof_set_1588_timestamp_offset(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ebuf_tof_epb_set_100g_credits(pipe_sess_hdl_t sess_hdl,
                                                     rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ebuf_tof_wait_for_flush_all_chan(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info);

pipe_status_t ebuf_set_epb_prsr_port_chnl_ctrl_en_reg(rmt_dev_info_t *dev_info,
                                                      uint8_t logical_pipe,
                                                      bf_dev_port_t local_port,
                                                      bool enable);

pipe_status_t pipe_mgr_ebuf_tof_complete_port_mode_transition_wa(
    rmt_dev_info_t *dev_info, bf_dev_port_t port);

pipe_status_t pipe_mgr_ebuf_tof_get_port_counter(rmt_dev_info_t *dev_info,
                                                 bf_dev_port_t port_id,
                                                 uint64_t *value);

pipe_status_t pipe_mgr_ebuf_tof_get_port_bypass_counter(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id, uint64_t *value);

pipe_status_t pipe_mgr_ebuf_tof_get_port_100g_credits(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id,
                                                      uint64_t *value);

#endif
