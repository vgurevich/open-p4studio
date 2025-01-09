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
 * @file pipe_exm_tbl_utest.c
 * @date
 *
 * Exact-match table manager, routines used over thrift for the purposes of
 * exact match unit-test.
 */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"

/* Structure definition for cached entry for txn state verification */
typedef struct pipe_mgr_exm_utest_cached_entry_ {
  pipe_mgr_exm_entry_info_t entry_data;
  // pipe_mgr_exm_entry_loc_info_t entry_loc_info;
  rmt_ram_line_t ram_shadow_copy[16];

} pipe_mgr_exm_utest_cached_entry_t;

typedef struct pipe_mgr_exm_utest_htbl_entry_ {
  bf_map_t ent_hdl_htbl;

} pipe_mgr_exm_utest_htbl_entry_t;

pipe_status_t pipe_mgr_exm_ent_get_candidate_locations(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mat_tbl_log_entry_container_t **candidate_entries);

pipe_status_t pipe_mgr_exm_compute_cuckoo_move_list(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    dev_stage_t *stage_id,
    cuckoo_move_list_t **move_list);

pipe_status_t pipe_mgr_exm_transform_cuckoo_move_list_utest(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    cuckoo_move_graph_t *cuckoo_move_graph,
    cuckoo_move_list_t *move_list);

typedef struct pipe_mgr_exm_cached_entry_ {
  pipe_tbl_match_spec_t match_spec;
  pipe_action_spec_t action_spec;
  pipe_act_fn_hdl_t act_fn_hdl;

} pipe_mgr_exm_cached_entry_t;

pipe_status_t pipe_mgr_exm_txn_compare_ent_hdl_state(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_txn_cache_entry_hdl(uint8_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_verify_txn_commit_abort(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_mgr_exm_utest_cached_entry_t *pipe_mgr_exm_utest_get_create_cached_entry(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl);
