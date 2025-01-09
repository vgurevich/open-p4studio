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


#ifndef _PIPE_MGR_TOF3_MIRROR_BUFFER_H
#define _PIPE_MGR_TOF3_MIRROR_BUFFER_H

#define PIPE_MGR_TOF3_MIRROR_SESSION_MAX 256
#define PIPE_MGR_TOF3_MIRROR_SLICE_MAX 4
#define PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX 16

#define PIPE_MGR_TOF3_MIRROR_COAL_DEF_BASE_TIME 100  // usec

pipe_status_t pipe_mgr_tof3_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_tof3_mirror_buf_init_session(pipe_sess_hdl_t sess_hdl,
                                                    bf_mirror_id_t sid,
                                                    bf_dev_id_t dev_id,
                                                    pipe_bitmap_t pbm);
pipe_status_t pipe_mgr_tof3_mirror_buf_coal_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable);
pipe_status_t pipe_mgr_tof3_mirror_buf_norm_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr);
pipe_status_t pipe_mgr_tof3_session_reset(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          uint16_t sid,
                                          bf_dev_pipe_t pipe_id);
#endif /* _PIPE_MGR_TOF3_MIRROR_BUFFER_H */
