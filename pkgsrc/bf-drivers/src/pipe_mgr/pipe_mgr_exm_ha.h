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
 * @file pipe_mgr_exm_ha.h
 * This file contains function/type declarations for exm HA purposes.
 */
#ifndef _PIPE_MGR_EXM_HA_H_
#define _PIPE_MGR_EXM_HA_H_

pipe_status_t pipe_mgr_exm_llp_restore_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p);

pipe_status_t pipe_mgr_exm_hlp_restore_state(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mgr_move_list_t *move_list,
                                             uint32_t *success_count);

pipe_status_t pipe_mgr_exm_update_sel_hlp_state(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                pipe_mat_ent_hdl_t entry_hdl,
                                                pipe_sel_grp_hdl_t grp_hdl);

pipe_status_t pipe_mgr_exm_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_handle,
    pipe_tbl_ha_reconc_report_t *ha_report);

pipe_status_t pipe_mgr_exm_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p);

void pipe_mgr_exm_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl);

void pipe_mgr_exm_cleanup_llp_ha_state(bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl);

#endif
