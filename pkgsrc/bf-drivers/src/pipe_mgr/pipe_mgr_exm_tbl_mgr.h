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
 * @file pipe_mgr_exm_tbl_mgr.h
 * @date
 *
 *
 * Contains definitions relating to pipe mgr's exact match table
 * management.
 */

#ifndef _PIPE_MGR_EXM_TBL_MGR_H
#define _PIPE_MGR_EXM_TBL_MGR_H

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr_int.h"
#include <pipe_mgr/pipe_mgr_intf.h>
#include <target-utils/id/id.h>
#include <target-utils/third-party/cJSON/cJSON.h>

/* Local header includes */
#include "pipe_mgr_tbl.h"

#define PIPE_MAT_ENT_INVALID_ENTRY_INDEX 0xdeadbeef
#define PIPE_MAT_ENT_HDL_INVALID_HDL 0xdeadbeef
#define PIPE_MGR_EXM_DEF_MISS_ENTRY_IDX 0
#define PIPE_MGR_EXM_DEF_ENTRY_HDL 0x1

/* Types */
typedef uint32_t pipe_mat_ent_idx_t;

/* A container structure for a bunch of logical entry indices
 * of a match entry table.
 */
typedef struct pipe_mat_tbl_log_entry_container_ {
  uint32_t num_entries;        /*< Number of entries in the container */
  pipe_mat_ent_idx_t *entries; /*< Pointer to an array containing the entries */

} pipe_mat_tbl_log_entry_container_t;

typedef pipe_mat_tbl_log_entry_container_t pipe_mgr_exm_edge_container_t;

/* Some basic accessor routines defined on the logical entry container */

uint32_t pipe_mat_tbl_logical_entry_container_len(
    pipe_mat_tbl_log_entry_container_t *container);

pipe_mat_ent_idx_t pipe_mat_tbl_get_log_entry(
    pipe_mat_tbl_log_entry_container_t *container, uint32_t idx);

/********************************************************************
 * Function prototypes
 ********************************************************************/
pipe_status_t pipe_mgr_exm_init(void);

pipe_status_t pipe_mgr_exm_ent_place(dev_target_t dev_tgt,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_tbl_match_spec_t *match_spec,
                                     pipe_act_fn_hdl_t act_fn_hdl,
                                     pipe_action_spec_t *act_data_spec,
                                     uint32_t ttl,
                                     uint32_t pipe_api_flags,
                                     pipe_mat_ent_hdl_t *ent_hdl_p,
                                     pipe_mgr_move_list_t **pipe_move_list_pp);

pipe_status_t pipe_mgr_exm_ent_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t new_ent_hdl,
    pipe_mgr_move_list_t **pipe_move_list);

pipe_status_t pipe_mgr_exm_entry_del(bf_dev_id_t device_id,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_mat_ent_hdl_t mat_ent_hdl,
                                     uint32_t pipe_api_flags,
                                     pipe_mgr_move_list_t **pipe_move_list_pp);

pipe_status_t pipe_mgr_exm_ent_set_action(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list_pp);

/* pipe_mgr_exm_txn_commit : Function to commit a transaction on a
 *                           particular table on a particular device.
 */
pipe_status_t pipe_mgr_exm_tbl_txn_commit(bf_dev_id_t dev_id,
                                          pipe_mgr_sm_tbl_info_t *t,
                                          bf_dev_pipe_t *pipes_list,
                                          unsigned nb_pipes);

pipe_status_t pipe_mgr_exm_tbl_txn_abort(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         bf_dev_pipe_t *pipes_list,
                                         unsigned nb_pipes);

pipe_status_t pipe_mgr_exm_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **pipe_move_list_pp);

pipe_status_t pipe_mgr_exm_default_ent_hdl_get(dev_target_t dev_tgt,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t *ent_hdl_p);

pipe_status_t pipe_mgr_exm_get_dir_ent_idx(bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           bf_dev_pipe_t *pipe_id_p,
                                           dev_stage_t *stage_id_p,
                                           rmt_tbl_hdl_t *stage_table_hdl_p,
                                           uint32_t *index_p);

pipe_status_t pipe_mgr_exm_tbl_mgr_entry_activate(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_tbl_mgr_entry_deactivate(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_gen_lock_id(bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_mgr_lock_id_type_e lock_id_type,
                                       lock_id_t *lock_id_p);

pipe_status_t pipe_mgr_exm_update_lock_type(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            bool idle,
                                            bool stat,
                                            bool add_lock);

pipe_status_t pipe_mgr_exm_update_idle_init_val(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0);

pipe_status_t pipe_mgr_exm_reset_idle(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      bf_dev_pipe_t pipe_id,
                                      uint8_t stage_id,
                                      mem_id_t mem_id,
                                      uint32_t mem_offset);

pipe_status_t pipe_mgr_exm_tbl_get_placed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t *count_p);

pipe_status_t pipe_mgr_exm_tbl_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t *count_p);

pipe_status_t pipe_mgr_exm_get_plcmt_data(bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_exm_entry_hdl_from_stage_idx(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint8_t stage,
    uint8_t logical_tbl_id,
    pipe_mat_ent_idx_t ent_idx,
    pipe_mat_ent_hdl_t *ent_hdl);

pipe_status_t pipe_mgr_exm_ent_set_resource(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list_pp);

pipe_status_t pipe_mgr_exm_cleanup_default_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list);

pipe_status_t pipe_mgr_exm_reset_default_entry(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl);

void pipe_mgr_exm_print_match_spec(bf_dev_id_t device_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   pipe_tbl_match_spec_t *match_spec,
                                   char *buf,
                                   size_t buf_len);

pipe_status_t pipe_mgr_exm_tbl_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *pipe_move_list,
    uint32_t *num_performed);

pipe_status_t pipe_mgr_exm_get_match_spec(pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          bf_dev_id_t device_id,
                                          bf_dev_pipe_t *pipe_id,
                                          pipe_tbl_match_spec_t **match_spec);

pipe_status_t pipe_mgr_get_exm_key_with_dkm_mask(
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_tbl_match_spec_t *match_spec,
    pipe_tbl_match_spec_t *dkm_applied_ms);

pipe_status_t pipe_mgr_exm_log_state(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_info_t *mat_info,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     cJSON *match_tbls);

pipe_status_t pipe_mgr_exm_restore_state(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_info_t *mat_info,
                                         cJSON *match_tbl);

pipe_status_t pipe_mgr_exm_entry_update_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p);

pipe_status_t pipe_mgr_exm_tbl_raw_entry_get(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             uint32_t tbl_index,
                                             bool err_correction,
                                             pipe_tbl_match_spec_t *match_spec,
                                             pipe_action_spec_t *act_spec,
                                             pipe_act_fn_hdl_t *act_fn_hdl,
                                             pipe_mat_ent_hdl_t *entry_hdl,
                                             bool *is_default,
                                             uint32_t *next_index);

pipe_status_t pipe_mgr_exm_get_last_index(dev_target_t dev_tgt,
                                          pipe_mat_tbl_hdl_t tbl_hdl,
                                          uint32_t *last_index);

pipe_status_t pipe_mgr_invalidate_exm_idx(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          uint32_t tbl_index);

pipe_status_t pipe_mgr_exm_tbl_update(dev_target_t dev_tgt,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      pipe_sel_grp_hdl_t sel_grp,
                                      uint32_t pipe_api_flags,
                                      pipe_mgr_move_list_t **move_head_p);
#endif
