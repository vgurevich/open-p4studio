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
 * @file pipe_mgr_phase0_tbl_mgr.h
 * @date
 *
 * Phase0 table initialization definition.
 */

#ifndef _PIPE_MGR_PHASE0_TBL_MGR_H
#define _PIPE_MGR_PHASE0_TBL_MGR_H
/* Standard header includes */

/* Module header includes */
#include <target-utils/third-party/cJSON/cJSON.h>
#include <pipe_mgr/pipe_mgr_intf.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hitless_ha.h"

typedef struct pipe_mgr_phase0_pipe_register_spec_t {
  pipe_register_spec_t *reg;
} pipe_mgr_phase0_pipe_register_spec_t;

typedef struct pipe_mgr_phase0_pipe_hlp_ha_info_t {
  pipe_mgr_spec_map_t spec_map;
} pipe_mgr_phase0_pipe_hlp_ha_info_t;

typedef struct pipe_mgr_phase0_tbl_data_t {
  /* Lowest pipe-id to which this instance of the table belongs to.
   * BF_DEV_PIPE_ALL if it belongs to all pipes in the profile.  */
  bf_dev_pipe_t pipe_id;
  scope_pipes_t scope_pipe_bmp;
  /* Default entry handle */
  pipe_mat_ent_hdl_t default_entry_hdl;
  /* Cache of the default entry's action spec at the LLP to be used in entry
   * deletes. */
  pipe_action_spec_t *dflt_act_spec;
  /* Bitset of installed table entries. */
  bf_bitset_t installed_entries;
  /* Bitset memory. */
  uint64_t *bitset_mem;
  /* Number of pipes in the scope. */
  uint8_t num_pipes;
  /* HA related info stored at HLP */
  pipe_mgr_phase0_pipe_hlp_ha_info_t *ha_hlp_info;
  /* HA Reconciliation report (for debug purposes) */
  pipe_tbl_ha_reconc_report_t ha_reconc_report;
  /* Entry handle to pipe_mgr_mat_data pointer mapping. */
  bf_map_t ent_data;
  pipe_mgr_move_list_t *txn_data;
} pipe_mgr_phase0_tbl_data_t;

typedef struct pipe_mgr_phase0_tbl_t {
  char *name;
  /* Table handle of this phase0 match table */
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  /* Device id to which this table belongs to */
  bf_dev_id_t dev_id;
  /* Device info ptr */
  rmt_dev_info_t *dev_info;
  /* Profile ID */
  profile_id_t profile_id;
  /* Parser instance handler */
  pipe_prsr_instance_hdl_t prsr_instance_hdl;
  uint32_t num_entries;
  uint32_t *num_entries_placed;
  uint32_t *num_entries_programmed;
  bool symmetric;
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  /* Number of instances of the table which have to be managed.
   * As many as the number of pipes for a table which is asymmetrically
   * populated, or just ONE for a table which is symmetrically populated
   * across all pipes or the number of scopes for a custom scopes definition.
   */
  uint32_t num_tbls;
  /* Pointer to an array of phase0 match table data. As many as num_tbls.
   */
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;

  /* Number of match spec bytes for this table */
  uint32_t num_match_spec_bytes;
  /* Number of match spec bits for this table */
  uint32_t num_match_spec_bits;
  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;
  /* Maximum size of action data spec */
  uint32_t max_act_data_size;
  /* Shadow memory read from hardware during HA */
  pipe_mgr_phase0_pipe_register_spec_t *reg_spec;
  /* Flag to indicate if this is the first entry during cfg replay */
  bool first_entry_replayed;
  /* Shadow of encoded entries, index by logical pipe then pipe-local port
   * number */
  uint8_t **log_pipe_shadow;
} pipe_mgr_phase0_tbl_t;

pipe_mgr_phase0_tbl_t *pipe_mgr_phase0_tbl_get(bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_phase0_tbl_init(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t pipe_mat_tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp);
void pipe_mgr_phase0_tbl_delete(uint8_t device_id,
                                pipe_mat_tbl_hdl_t mat_tbl_hdl);
pipe_status_t pipe_mgr_phase0_set_symmetric_mode(bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                 bool symmetric,
                                                 scope_num_t num_scopes,
                                                 scope_pipes_t *scope_pipe_bmp);
pipe_status_t pipe_mgr_phase0_get_symmetric_mode(bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                 bool *symmetric,
                                                 scope_num_t *num_scopes,
                                                 scope_pipes_t *scope_pipe_bmp);
pipe_status_t pipe_mgr_phase0_ent_place(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *match_spec,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *action_spec,
                                        pipe_mat_ent_hdl_t *mat_ent_hdl,
                                        pipe_mgr_move_list_t **move_list);
pipe_status_t pipe_mgr_phase0_ent_del(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      pipe_mgr_move_list_t **move_list);
pipe_status_t pipe_mgr_phase0_ent_set_action(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             pipe_action_spec_t *action_spec,
                                             pipe_mgr_move_list_t **move_list);
pipe_status_t pipe_mgr_phase0_ent_program(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mgr_move_list_t *move_list,
                                          uint32_t *processed);
pipe_status_t pipe_mgr_phase0_create_dma(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_phase0_tbl_txn_commit(bf_dev_id_t dev_id,
                                             pipe_mat_ent_hdl_t mat_tbl_hdl,
                                             bf_dev_pipe_t *pipes_list,
                                             unsigned nb_pipes);
pipe_status_t pipe_mgr_phase0_tbl_txn_abort(bf_dev_id_t dev_id,
                                            pipe_mat_ent_hdl_t mat_tbl_hdl,
                                            bf_dev_pipe_t *pipes_list,
                                            unsigned nb_pipes);
void pipe_mgr_phase0_print_match_spec(bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_tbl_match_spec_t *match_spec,
                                      char *buf,
                                      size_t buf_size);
pipe_status_t pipe_mgr_phase0_get_first_entry_handle(pipe_mat_tbl_hdl_t tbl_hdl,
                                                     dev_target_t dev_tgt,
                                                     int *entry_hdl);
pipe_status_t pipe_mgr_phase0_get_next_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles);
pipe_status_t pipe_mgr_phase0_get_match_spec(
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t *pipe_id,
    pipe_tbl_match_spec_t **match_spec);
pipe_status_t pipe_mgr_phase0_get_ent_hdl(bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_tbl_match_spec_t *match_spec,
                                          pipe_mat_ent_hdl_t *entry_hdl);
pipe_status_t pipe_mgr_phase0_get_entry(pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_ent_hdl_t entry_hdl,
                                        pipe_tbl_match_spec_t *pipe_match_spec,
                                        pipe_action_spec_t *pipe_action_spec,
                                        pipe_act_fn_hdl_t *act_fn_hdl,
                                        bool from_hw);
pipe_status_t pipe_mgr_phase0_get_plcmt_data(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mgr_move_list_t **move_list);
pipe_status_t pipe_mgr_phase0_get_placed_entry_count(dev_target_t dev_tgt,
                                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                                     uint32_t *count);
pipe_status_t pipe_mgr_phase0_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, uint32_t *count);
bf_dev_port_t pipe_mgr_phase0_hdl_to_port(pipe_mat_ent_hdl_t hdl);
pipe_status_t pipe_mgr_phase0_log_state(bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        cJSON *match_tbls);
pipe_status_t pipe_mgr_phase0_restore_state(bf_dev_id_t dev_id,
                                            cJSON *match_tbl);

pipe_status_t pipe_mgr_phase0_shadow_mem_populate(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_mat_tbl_hdl_t tbl_hdl);

pipe_status_t pipe_mgr_phase0_llp_restore_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t **move_head_p);

pipe_status_t pipe_mgr_phase0_hlp_restore_state(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                pipe_mgr_move_list_t *move_list,
                                                uint32_t *success_count);

pipe_status_t pipe_mgr_phase0_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report);

pipe_status_t pipe_mgr_phase0_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t **move_head_p);

pipe_status_t pipe_mgr_phase0_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    bool is_txn,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **pipe_move_list);

pipe_status_t pipe_mgr_phase0_cleanup_default_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list);

pipe_status_t pipe_mgr_phase0_default_ent_get(dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_action_spec_t *action_spec,
                                              pipe_act_fn_hdl_t *act_fn_hdl,
                                              bool from_hw);

pipe_status_t pipe_mgr_phase0_default_ent_hdl_get(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p);

pipe_status_t pipe_mgr_phase0_tbl_raw_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint32_t tbl_index,
    bool err_correction,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_ent_hdl_t *entry_hdl,
    bool *is_default,
    uint32_t *next_index);

pipe_status_t pipe_mgr_phase0_get_last_index(dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             uint32_t *last_index);
#endif  // _PIPE_MGR_PHASE0_TBL_MGR_H
