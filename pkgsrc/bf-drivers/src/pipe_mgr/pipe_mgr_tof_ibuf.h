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


#ifndef __PIPE_MGR_TOF_IBUF_H__
#define __PIPE_MGR_TOF_IBUF_H__

#include <pipe_mgr/pipe_mgr_intf.h>

#define BF_TOFINO_IBUF_SIZE (24 * 1024)  // 24KB

pipe_status_t pipe_mgr_ibuf_tof_set_logical_port(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ibuf_tof_set_version_bits(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info,
                                                 uint8_t version);
pipe_status_t pipe_mgr_ibuf_tof_disable_all_chan(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ibuf_tof_port_set_drop_threshold(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint32_t drop_hi_thrd,
    uint32_t drop_low_thrd);
pipe_status_t pipe_mgr_ibuf_tof_port_set_afull_threshold(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint32_t afull_hi_thrd,
    uint32_t afull_low_thrd);

pipe_status_t pipe_mgr_ibuf_tof_set_port_speed_based_cfg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ibuf_tof_enable_channel(rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ibuf_tof_enable_channel_all(pipe_sess_hdl_t sess_hdl,
                                                   rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ibuf_tof_enable_congestion_notif_to_parser(
    rmt_dev_info_t *dev_info, rmt_port_info_t *port_info);
pipe_status_t pipe_mgr_ibuf_tof_parb_enable_flow_control_to_mac(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint16_t low_wm_bytes,
    uint16_t hi_wm_bytes);
pipe_status_t pipe_mgr_ibuf_tof_disable_congestion_notif_to_parser(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ibuf_tof_parb_disable_flow_control_to_mac(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ibuf_tof_disable_chnl(rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id);
pipe_status_t pipe_mgr_ibuf_tof_set_1588_timestamp_offset(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info);

pipe_status_t ibuf_set_chnl_ctrl(rmt_dev_info_t *dev_info,
                                 bf_dev_port_t port_id,
                                 bool chnl_enable,
                                 pipe_sess_hdl_t shdl,
                                 bool use_dma);
#endif
