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


#ifndef _PIPE_MGR_TOF_MIRROR_BUFFER_H
#define _PIPE_MGR_TOF_MIRROR_BUFFER_H

#define PIPE_MGR_TOF_MIRROR_SESSION_MAX 1024
#define PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX 8
#define PIPE_MGR_TOF_MIRROR_COAL_BASE_SID \
  (PIPE_MGR_TOF_MIRROR_SESSION_MAX - PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX)
#define PIPE_MGR_TOF_MIRROR_NEG_SID (PIPE_MGR_TOF_MIRROR_COAL_BASE_SID - 1)

#define PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME 100  // usec

pipe_status_t pipe_mgr_tof_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_tof_mirror_buf_init_session(pipe_sess_hdl_t sess_hdl,
                                                   bf_mirror_id_t sid,
                                                   bf_dev_id_t dev_id,
                                                   pipe_bitmap_t pbm);
bool pipe_mgr_tof_mirror_buf_sid_is_coalescing(uint16_t sid);
pipe_status_t pipe_mgr_tof_mirror_buf_coal_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pipes,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable);
pipe_status_t pipe_mgr_tof_mirror_buf_coal_session_read(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool *enable,
    bool *session_valid);
pipe_status_t pipe_mgr_tof_mirror_buf_norm_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pipes,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr);
pipe_status_t pipe_mgr_tof_mirror_buf_norm_session_read(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool *enable_ing,
    bool *enable_egr,
    bool *session_valid);
bool pipe_mgr_tof_ha_mirror_buf_cfg_compare(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    bf_mirror_id_t sid,
    pipe_mgr_mirror_session_info_t *sess_info,
    pipe_mgr_mirror_session_info_t *hw_sess_info);
#endif /* _PIPE_MGR_TOF_MIRROR_BUFFER_H */
