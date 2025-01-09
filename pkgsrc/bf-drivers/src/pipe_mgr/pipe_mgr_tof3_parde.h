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


#ifndef __PIPE_MGR_TOF3_IBUF_H__
#define __PIPE_MGR_TOF3_IBUF_H__

#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_db.h"
pipe_status_t pipe_mgr_parde_tof3_device_add(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info);
void pipe_mgr_parde_tof3_device_rmv(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_parde_tof3_port_add_ing(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id);
pipe_status_t pipe_mgr_parde_tof3_port_add_egr(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id);
pipe_status_t pipe_mgr_parde_tof3_port_rmv_egr(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id);
pipe_status_t pipe_mgr_parde_tof3_port_rmv_ing(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id);
pipe_status_t pipe_mgr_parde_tof3_port_ena_ing_all(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_parde_tof3_port_dis_ing_all(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_parde_tof3_port_dis_ing_all_with_dma(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_ibuf_tof3_set_version_bits(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  uint8_t version);
pipe_status_t pipe_mgr_ibuf_tof3_config_congestion_notif_to_parser(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info, rmt_port_info_t *port_info);

pipe_status_t pipe_mgr_parde_tof3_port_ena_one(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               uint8_t logical_pipe,
                                               int ipb_num,
                                               bool ing_0_egr_1);
pipe_status_t pipe_mgr_parde_tof3_port_dis_one(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               uint8_t logical_pipe,
                                               int ipb_num,
                                               bool ing_0_egr_1);
pipe_status_t pipe_mgr_parser_config_tof3(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint8_t gress,
    pipe_bitmap_t pipe_bmp,
    uint64_t prsr_grp_map,
    struct pipe_mgr_tof3_prsr_bin_config *cfg);

pipe_status_t pipe_mgr_tof3_iprsr_get_pri_thresh(rmt_dev_info_t *dev_info,
                                                 rmt_port_info_t *port_info,
                                                 uint32_t *val);

pipe_status_t pipe_mgr_tof3_iprsr_set_pri_thresh(rmt_dev_info_t *dev_info,
                                                 rmt_port_info_t *port_info,
                                                 uint32_t val);
#endif
