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


#ifndef _PIPE_MGR_IDLE_SWEEP_H_
#define _PIPE_MGR_IDLE_SWEEP_H_

#include "pipe_mgr_idle.h"

void pipe_mgr_idle_sw_timer_cb(struct bf_sys_timer_s *timer, void *data);

pipe_status_t pipe_mgr_idle_entry_get_ttl(idle_tbl_stage_info_t *stage_info,
                                          pipe_mat_ent_hdl_t ent_hdl,
                                          uint32_t *ttl_p,
                                          uint32_t *init_ttl_p);

pipe_status_t pipe_mgr_idle_entry_get_init_ttl(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t *init_ttl_p);

pipe_status_t pipe_mgr_idle_entry_get_poll_state(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e *poll_state_p);

pipe_status_t pipe_mgr_idle_entry_set_poll_state(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e poll_state);

bool pipe_mgr_idle_entry_mdata_exists(idle_tbl_stage_info_t *stage_info,
                                      pipe_mat_ent_hdl_t ent_hdl);

pipe_status_t pipe_mgr_idle_entry_add_mdata(idle_tbl_stage_info_t *stage_info,
                                            pipe_mat_ent_hdl_t ent_hdl,
                                            uint32_t index,
                                            uint32_t new_ttl,
                                            uint32_t cur_ttl);

pipe_status_t pipe_mgr_idle_entry_del_mdata(idle_tbl_stage_info_t *stage_info,
                                            pipe_mat_ent_hdl_t ent_hdl,
                                            uint32_t index);

pipe_status_t pipe_mgr_idle_entry_move_mdata(idle_tbl_stage_info_t *stage_info,
                                             pipe_mat_ent_hdl_t ent_hdl,
                                             uint32_t src_index,
                                             uint32_t dest_index);

pipe_status_t pipe_mgr_idle_entry_set_mdata_ttl_dirty(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t new_ttl);

/* Accessed by CLI thread */
idle_entry_t *pipe_mgr_idle_entry_get_by_index(
    idle_tbl_stage_info_t *stage_info, uint32_t index);

bool pipe_mgr_idle_entry_mdata_get(idle_tbl_stage_info_t *stage_info,
                                   pipe_mat_ent_hdl_t ent_hdl,
                                   uint32_t *init_ttl_p,
                                   uint32_t *cur_ttl_p,
                                   idle_entry_location_t **ils_p);
void destroy_ils(idle_entry_location_t *ils);

pipe_status_t pipe_mgr_idle_process_task_list(idle_tbl_info_t *idle_tbl_info,
                                              idle_tbl_stage_info_t *stage_info,
                                              idle_task_list_t *tlist);
#endif
