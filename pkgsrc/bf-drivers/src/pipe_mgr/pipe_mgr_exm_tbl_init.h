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
 * @file pipe_exm_tbl_mgr_init.h
 * @date
 *
 * Exact-match table initialization definition.
 */

#ifndef _PIPE_MGR_EXM_TBL_INIT_H
#define _PIPE_MGR_EXM_TBL_INIT_H
/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_exm_tbl_mgr_int.h"

pipe_status_t pipe_mgr_exm_tbl_init(bf_dev_id_t dev_id,
                                    pipe_mat_tbl_hdl_t pipe_mat_tbl_hdl,
                                    profile_id_t profile_id,
                                    pipe_bitmap_t *pipe_bmp);

void pipe_mgr_exm_tbl_delete(bf_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_exm_tbl_set_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp);
pipe_status_t pipe_mgr_exm_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_exm_get_first_placed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl);

pipe_status_t pipe_mgr_exm_get_next_placed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles);

pipe_status_t pipe_mgr_exm_get_first_programmed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl);

pipe_status_t pipe_mgr_exm_get_next_programmed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles);

pipe_status_t pipe_mgr_exm_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls);

pipe_status_t pipe_mgr_exm_default_ent_get(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_action_spec_t *action_spec,
                                           pipe_act_fn_hdl_t *act_fn_hdl,
                                           bool from_hw);

pipe_status_t pipe_mgr_exm_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_mat_ent_hdl_t entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     pipe_action_spec_t *pipe_action_spec,
                                     pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_exm_get_entry_llp_from_hw(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_exm_get_proxy_hash_match_spec_by_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec);

cuckoo_move_graph_t *pipe_mgr_exm_get_or_create_cuckoo_move_graph(
    pipe_mgr_exm_stage_info_t *exm_stage_info);

uint32_t get_unit_ram_depth(pipe_mgr_exm_tbl_t *exm_tbl);
#define TOF_UNIT_RAM_DEPTH(x) get_unit_ram_depth(x)
#endif  // _PIPE_MGR_EXM_TBL_INIT_H
