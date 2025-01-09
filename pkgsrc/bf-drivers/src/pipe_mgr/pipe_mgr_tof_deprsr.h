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


/*!
 * @file pipe_mgr_tof_deprsr.h
 * @date
 *
 * Configuration of Tofino Deparser based on port speed.
 */

#ifndef __PIPE_MGR_TOF_DEPRSR_H__
#define __PIPE_MGR_TOF_DEPRSR_H__
/* Maintains a software shadow of the following three registers:
 *  pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_0_3
 *  pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_1_3
 *  pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_2_3
 * Note that these are arrays with the physical pipe as an index.
 * Note that these values are only modified when a port is added. */
struct pipe_mgr_tof_deprsr_ctx {
  uint32_t *ch_rate_0_3;
  uint32_t *ch_rate_1_3;
  uint32_t *ch_rate_2_3;
};

pipe_status_t pipe_mgr_tof_deprsr_cfg_init(pipe_sess_hdl_t sess_hdl,
                                           rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_deprsr_cfg_deinit(bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_tof_deprsr_set_port_speed_based_cfg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id);
#endif
