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


#ifndef _PIPE_MGR_IDLE_API_H_
#define _PIPE_MGR_IDLE_API_H_

#include "pipe_mgr_idle.h"

pipe_status_t pipe_mgr_idle_get_ttl_helper(idle_tbl_info_t *idle_tbl_info,
                                           pipe_mat_ent_hdl_t ent_hdl,
                                           bf_dev_pipe_t pipe_id,
                                           uint32_t stage_id,
                                           rmt_tbl_hdl_t stage_table_handle,
                                           uint32_t *ttl,
                                           uint32_t *init_ttl_p);

void pipe_mgr_idle_destroy_task_list(idle_task_list_t *tlist);

bool pipe_mgr_idle_is_state_active(rmt_idle_time_tbl_params_t rmt_params,
                                   uint8_t value);

idle_mgr_dev_ctx_t *idle_mgr_dev_ctx(bf_dev_id_t dev_id);

idle_tbl_stage_info_t *pipe_mgr_idle_tbl_stage_info_get(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t ipipe,
    uint32_t stage,
    rmt_tbl_hdl_t stage_table_handle);

/* Accessed by CLI thread */
idle_tbl_info_t *pipe_mgr_idle_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl);

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get_first(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p);

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get_next(bf_dev_id_t dev_id,
                                                 pipe_mat_tbl_hdl_t *tbl_hdl_p);
#endif
