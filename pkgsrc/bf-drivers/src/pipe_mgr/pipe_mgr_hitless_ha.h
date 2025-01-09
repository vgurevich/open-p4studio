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

#ifndef _PIPE_MGR_HITLESS_HA_H_
#define _PIPE_MGR_HITLESS_HA_H_

pipe_status_t pipe_mgr_hitless_ha_init(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_hitless_ha_complete_hw_read(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_hitless_ha_get_reconc_report(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_handle,
    pipe_tbl_ha_reconc_report_t *ha_report);

pipe_status_t pipe_mgr_hitless_ha_compute_delta_changes(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_hitless_ha_push_delta_changes(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id);

// Spec Map related functions
typedef enum pipe_mgr_hitless_ent_state_e {
  INVALID_MATCH,
  FULL_MATCH,         // Entry and it's actions match fully
  ACTION_HDL_DIRTY,   // The action handle needs to be updated from temp to real
  ACTION_DATA_DIRTY,  // The action function matches, however the action data
                      // does not match
  ACTION_DATA_MISS,   // The actions of the entry does not match including the
                      // action function
  RESOURCE_MISMATCH
} pipe_mgr_hitless_ent_state_e;

static inline const char *pipe_mgr_hitless_ent_state_name(
    pipe_mgr_hitless_ent_state_e e) {
  switch (e) {
    case FULL_MATCH:
      return "FULL";
    case ACTION_HDL_DIRTY:
      return "ACT_HDL_DIRTY";
    case ACTION_DATA_DIRTY:
      return "ACT_DATA_DIRTY";
    case ACTION_DATA_MISS:
      return "ACT_DATA_MISS";
    case RESOURCE_MISMATCH:
      return "RESOURCE_MISMATCH";
    case INVALID_MATCH:
      return "INVALID_MATCH";
  }
  return "Unknown";
}

typedef struct pipe_mgr_hitless_entry_s {
  /* n and p are to link pipe_mgr_ha_entry_t's together in the hash table. */
  struct pipe_mgr_hitless_entry_s *n;
  struct pipe_mgr_hitless_entry_s *p;
  /* np and pp are used to link entries on various lists in the spec_map_t. */
  struct pipe_mgr_hitless_entry_s *np;
  struct pipe_mgr_hitless_entry_s *pp;
  pipe_mgr_hitless_ent_state_e ha_state;
  pipe_mgr_move_list_t *mn;
  /* If mn is NULL, the parameters will be stored in the below variables */
  pipe_ent_hdl_t entry_hdl;
  pipe_tbl_match_spec_t *match_spec;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_action_spec_t *action_spec;
  uint32_t ttl;
} pipe_mgr_ha_entry_t;

typedef struct pipe_mgr_hitless_ha_ctx_ {
  bf_map_t saved_ml;
} pipe_mgr_hitless_ha_ctx_t;

typedef pipe_status_t (*pipe_mgr_entry_place_with_hdl_fn)(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_head_p);

typedef pipe_status_t (*pipe_mgr_entry_modify_fn)(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p);

typedef pipe_status_t (*pipe_mgr_entry_delete_fn)(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p);

/* Function to update the state from the API replay for fully matched entries */
typedef pipe_status_t (*pipe_mgr_entry_update_fn)(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p);

/* Hash tables for storing the spec-entry handles */
typedef struct pipe_mgr_spec_map_s {
  bf_hashtable_t *spec_htbl;
  pipe_mgr_ha_entry_t *full_match_list;
  pipe_mgr_ha_entry_t *to_add_list;
  pipe_mgr_ha_entry_t *to_modify_list;
  pipe_mgr_ha_entry_t *to_delete_list;

  dev_target_t dev_tgt;
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  void *tbl_info;

  pipe_act_fn_hdl_t def_act_fn_hdl;
  pipe_action_spec_t *def_act_spec;

  pipe_mgr_entry_place_with_hdl_fn entry_place_with_hdl_fn;
  pipe_mgr_entry_modify_fn entry_modify_fn;
  pipe_mgr_entry_delete_fn entry_delete_fn;
  pipe_mgr_entry_update_fn entry_update_fn;

} pipe_mgr_spec_map_t;

pipe_status_t pipe_mgr_hitless_ha_new_spec(pipe_mgr_spec_map_t *spec_map,
                                           pipe_mgr_move_list_t *move_node);

pipe_status_t pipe_mgr_hitless_ha_new_adt_spec(
    pipe_mgr_spec_map_t *spec_map,
    pipe_mgr_adt_move_list_t *move_node,
    uint8_t key_sz);

pipe_status_t pipe_mgr_hitless_ha_lookup_spec(pipe_mgr_spec_map_t *spec_map,
                                              pipe_tbl_match_spec_t *match_spec,
                                              pipe_action_spec_t *action_spec,
                                              pipe_act_fn_hdl_t act_fn_hdl,
                                              pipe_ent_hdl_t new_entry_hdl,
                                              pipe_ent_hdl_t *entry_hdl_p,
                                              uint32_t ttl);

pipe_status_t pipe_mgr_hitless_ha_lookup_adt_spec(
    pipe_mgr_spec_map_t *spec_map,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_ent_hdl_t new_entry_hdl,
    pipe_ent_hdl_t *entry_hdl_p,
    uint32_t key_sz);

pipe_status_t pipe_mgr_hitless_ha_reconcile(
    pipe_mgr_spec_map_t *spec_map,
    pipe_mgr_move_list_t **move_tail_p,
    pipe_mat_tbl_info_t *tbl_info,
    pipe_tbl_ha_reconc_report_t *ha_report);

void pipe_mgr_hitless_ha_delete_spec_map(pipe_mgr_spec_map_t *spec_map);
#endif
