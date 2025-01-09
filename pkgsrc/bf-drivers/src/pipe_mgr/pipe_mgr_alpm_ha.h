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

#ifndef _PIPE_MGR_ALPM_HA_H_
#define _PIPE_MGR_ALPM_HA_H_

pipe_status_t pipe_mgr_alpm_hlp_restore_state(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl);

pipe_status_t pipe_mgr_alpm_hlp_entry_restore(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t ll_tbl_hdl,
                                              pipe_mgr_move_list_t *move_node);

pipe_status_t pipe_mgr_alpm_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p);

void pipe_mgr_alpm_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_alpm_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report);
#endif
