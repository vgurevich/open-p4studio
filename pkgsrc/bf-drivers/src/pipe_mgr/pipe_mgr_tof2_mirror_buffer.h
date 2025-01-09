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


#ifndef _PIPE_MGR_TOF2_MIRROR_BUFFER_H
#define _PIPE_MGR_TOF2_MIRROR_BUFFER_H

#define PIPE_MGR_TOF2_MIRROR_SESSION_MAX 256
#define PIPE_MGR_TOF2_MIRROR_SLICE_MAX 4
#define PIPE_MGR_TOF2_MIRROR_BUFFER_USAGE_PER_SLICE_MAX 5
#define PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX 16

#define PIPE_MGR_TOF2_MIRROR_COAL_DEF_BASE_TIME 100  // usec

pipe_status_t pipe_mgr_tof2_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_tof2_mirror_buf_init_session(pipe_sess_hdl_t sess_hdl,
                                                    bf_mirror_id_t sid,
                                                    bf_dev_id_t dev_id,
                                                    pipe_bitmap_t pbm);
pipe_status_t pipe_mgr_tof2_mirror_buf_coal_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable);
pipe_status_t pipe_mgr_tof2_mirror_buf_norm_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr);
pipe_status_t pipe_mgr_tof2_session_reset(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          uint16_t sid,
                                          bf_dev_pipe_t pipe_id);
pipe_status_t pipe_mgr_tof2_mirror_buf_session_read(
    pipe_sess_hdl_t shdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t phy_pipe,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool *enable_ing,
    bool *enable_egr,
    bool *session_valid);
bool pipe_mgr_tof2_ha_mirror_buf_cfg_compare(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    bf_mirror_id_t sid,
    pipe_mgr_mirror_session_info_t *sess_info,
    pipe_mgr_mirror_session_info_t *hw_sess_info);

#endif /* _PIPE_MGR_TOF2_MIRROR_BUFFER_H */
