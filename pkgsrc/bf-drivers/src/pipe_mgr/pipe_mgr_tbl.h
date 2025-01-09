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
 * @file pipe_mgr_tbl.h
 * @date
 *
 * Contains definitions for action data table manager related initialization.
 *
 */
#ifndef _PIPE_MGR_TBL_H
#define _PIPE_MGR_TBL_H

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_move_list.h"
#include <target-utils/third-party/cJSON/cJSON.h>

#define RMT_API(sess_hdl, pipe_api_flags, verify_fn, fn)             \
  {                                                                  \
    pipe_status_t ret = PIPE_SUCCESS;                                \
    if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {      \
      return ret;                                                    \
    }                                                                \
    do {                                                             \
      if (pipe_mgr_sess_in_batch(sess_hdl)) {                        \
        pipe_mgr_drv_ilist_chkpt(sess_hdl);                          \
      }                                                              \
                                                                     \
      ret = (verify_fn);                                             \
      if (ret != PIPE_SUCCESS) {                                     \
        break;                                                       \
      }                                                              \
                                                                     \
      bool isAtom = pipe_mgr_sess_in_atomic_txn(sess_hdl);           \
      uint32_t flags =                                               \
          pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0; \
      flags |= isAtom ? PIPE_MGR_TBL_API_ATOM : 0;                   \
                                                                     \
      ret = fn;                                                      \
    } while (0);                                                     \
    ret = handleTableApiRsp(sess_hdl,                                \
                            ret,                                     \
                            pipe_api_flags & PIPE_FLAG_SYNC_REQ,     \
                            __func__,                                \
                            __LINE__);                               \
    pipe_mgr_api_exit(sess_hdl);                                     \
    return ret;                                                      \
  }

pipe_status_t handleTableApiRsp(pipe_sess_hdl_t sess_hdl,
                                pipe_status_t sts,
                                bool isSync,
                                const char *where,
                                const int line);

pipe_status_t pipe_mgr_tbl_set_scope(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool symmetric,
                                     scope_num_t num_scopes,
                                     scope_pipes_t *scope_pipe_bmp,
                                     bool update_dflt);

pipe_status_t pipe_mgr_tbl_get_scope(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool *symmetric,
                                     scope_num_t *num_scopes,
                                     scope_pipes_t *scope_pipe_bmp);

bool pipe_mgr_tbl_is_scope_different(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool symmetric_a,
                                     scope_num_t num_scopes_a,
                                     scope_pipes_t *scope_pipe_bmp_a,
                                     bool symmetric_b,
                                     scope_num_t num_scopes_b,
                                     scope_pipes_t *scope_pipe_bmp_b);

bf_dev_pipe_t pipe_mgr_get_lowest_pipe_in_scope(scope_pipes_t scope_pipe_bmp);
pipe_status_t pipe_mgr_get_all_pipes_in_scope(bf_dev_pipe_t pipe,
                                              scope_num_t num_scopes,
                                              scope_pipes_t *scope_pipe_bmp,
                                              pipe_bitmap_t *pipe_bmp);
pipe_status_t pipe_mgr_convert_scope_pipe_bmp(scope_pipes_t scope_pipe_bmp,
                                              pipe_bitmap_t *pipe_bmp);
pipe_status_t pipe_mgr_is_pipe_in_bmp(pipe_bitmap_t *pipe_bmp,
                                      bf_dev_pipe_t pipe_id);

pipe_status_t pipe_mgr_tbl_set_symmetric_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool symmetric,
                                              scope_num_t num_scopes,
                                              scope_pipes_t *scope_pipe_bmp,
                                              bool update_dflt);

pipe_status_t pipe_mgr_tbl_get_property_scope(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *symmetric,
                                              uint32_t *args);

pipe_status_t pipe_mgr_tbl_set_placement_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool placement_app);

pipe_status_t pipe_mgr_tbl_get_placement_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *placement_app);

pipe_status_t pipe_mgr_tbl_set_repeated_notify(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               bool repeated_notify);

pipe_status_t pipe_mgr_tbl_get_repeated_notify(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               bool *repeated_notify);

pipe_status_t pipe_mgr_tbl_set_duplicate_entry_check(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool duplicate_check_enable);

pipe_status_t pipe_mgr_tbl_get_duplicate_entry_check(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *duplicate_check_enable);

pipe_status_t pipe_mgr_tbl_get_first_entry_handle(pipe_sess_hdl_t sess_hdl,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  int *entry_hdl);

pipe_status_t pipe_mgr_tbl_get_next_entry_handles(pipe_sess_hdl_t sess_hdl,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  pipe_mat_ent_hdl_t entry_hdl,
                                                  int n,
                                                  int *next_entry_handles);

pipe_status_t pipe_mgr_tbl_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls);

pipe_status_t pipe_mgr_tbl_get_init_default_entry(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_tbl_get_entry_hdlr(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_target_t,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw);

pipe_status_t pipe_mgr_tbl_get_action_data_hdlr(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw);

pipe_status_t pipe_mgr_tbl_get_entry_count(dev_target_t dev_tgt,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           bool read_from_hw,
                                           uint32_t *count_p);

pipe_status_t pipe_mgr_tbl_get_reserved_entry_count(dev_target_t dev_tgt,
                                                    pipe_tbl_hdl_t tbl_hdl,
                                                    size_t *count_p);

pipe_status_t pipe_mgr_tbl_get_total_hw_entry_count(dev_target_t dev_tgt,
                                                    pipe_tbl_hdl_t tbl_hdl,
                                                    size_t *count_p);

pipe_status_t pipe_mgr_tbl_default_entry_needs_reserve(
    rmt_dev_info_t *dev_info,
    pipe_mat_tbl_info_t *tbl_info,
    bool idle_tbl_present,
    bool *reservation_required);

pipe_status_t pipe_mgr_tbl_stage_idx_to_hdl(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            bf_dev_pipe_t pipe,
                                            uint8_t stage,
                                            uint8_t logical_tbl_id,
                                            uint32_t hit_addr,
                                            pipe_mat_ent_hdl_t *entry_hdl);

typedef struct pipe_mgr_mat_key_htbl_node_ {
  pipe_mat_ent_hdl_t mat_ent_hdl;
  pipe_tbl_match_spec_t *match_spec;
} pipe_mgr_mat_key_htbl_node_t;

typedef enum pipe_mgr_mat_operation_ {
  PIPE_MGR_MAT_OPERATION_ADD,
  PIPE_MGR_MAT_OPERATION_DELETE,
} pipe_mgr_mat_operation_e;

typedef struct pipe_mgr_mat_txn_log_htbl_node_ {
  pipe_mgr_mat_key_htbl_node_t *htbl_node;
  /* Index of the array of hashtable from which the entry was deleted.
   * Only used when an entry gets deleted and has to be inserted back
   * owing to a transaction abort. Since we need to know the pipe id into
   * which the entry needs to be inserted.
   */
  bf_dev_pipe_t pipe_idx;
  pipe_mgr_mat_operation_e operation;
} pipe_mgr_mat_txn_log_htbl_node_t;

typedef struct pipe_mgr_mat_tbl_foreach_arg_ {
  bf_dev_id_t device_id;
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  pipe_mat_tbl_info_t *mat_tbl_info;
} pipe_mgr_mat_tbl_foreach_arg_t;

void pipe_mgr_free_key_htbl_node(void *node);

pipe_status_t pipe_mgr_mat_tbl_key_exists(pipe_mat_tbl_info_t *mat_tbl_info,
                                          pipe_tbl_match_spec_t *match_spec,
                                          bf_dev_pipe_t pipe_id,
                                          bool *exists,
                                          pipe_mat_ent_hdl_t *mat_ent_hdl);

pipe_status_t pipe_mgr_mat_tbl_key_insert(bf_dev_id_t device_id,
                                          pipe_mat_tbl_info_t *mat_tbl_info,
                                          pipe_tbl_match_spec_t *match_spec,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          bool isTxn);

pipe_status_t pipe_mgr_mat_tbl_key_delete(bf_dev_id_t device_id,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bool isTxn,
                                          pipe_mat_tbl_info_t *mat_tbl_info,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_match_spec_t *match_spec);

void pipe_mgr_mat_tbl_txn_commit(bf_dev_id_t device_id,
                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes);

void pipe_mgr_mat_tbl_txn_abort(bf_dev_id_t device_id,
                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                bf_dev_pipe_t *pipes_list,
                                unsigned nb_pipes);

void pipe_mgr_match_spec_htbl_cleanup(bf_dev_id_t device_id,
                                      pipe_mat_tbl_info_t *mat_tbl_info);

pipe_tbl_match_spec_t *pipe_mgr_tbl_copy_match_spec(pipe_tbl_match_spec_t *dst,
                                                    pipe_tbl_match_spec_t *src);
void pipe_mgr_tbl_destroy_match_spec(pipe_tbl_match_spec_t **match_spec_p);
pipe_tbl_match_spec_t *pipe_mgr_tbl_alloc_match_spec(size_t num_match_bytes);

int pipe_mgr_tbl_compare_match_specs(pipe_tbl_match_spec_t *m1,
                                     pipe_tbl_match_spec_t *m2);

pipe_status_t pipe_mgr_create_match_spec(
    uint8_t *key,
    uint8_t *msk,
    int len_bytes,
    int len_bits,
    uint32_t priority,
    pipe_tbl_match_spec_t *pipe_match_spec);

pipe_status_t pipe_mgr_create_action_data_spec(
    pipe_mgr_field_info_t *field_info,
    uint32_t num_fields,
    pipe_action_data_spec_t *act_data_spec);

pipe_status_t pipe_mgr_create_action_spec(bf_dev_id_t dev_id,
                                          pipe_mgr_action_entry_t *ae,
                                          pipe_action_spec_t *act_spec);

pipe_action_spec_t *pipe_mgr_tbl_copy_action_spec(pipe_action_spec_t *dst,
                                                  pipe_action_spec_t *src);

void pipe_mgr_tbl_destroy_action_spec(pipe_action_spec_t **action_spec_p);
pipe_action_spec_t *pipe_mgr_tbl_alloc_action_spec(
    size_t num_action_data_bytes);

int pipe_mgr_tbl_compare_action_data_specs(pipe_action_data_spec_t *ad1,
                                           pipe_action_data_spec_t *ad2);

int pipe_mgr_tbl_compare_action_specs(pipe_action_spec_t *a1,
                                      pipe_action_spec_t *a2);

pipe_status_t pipe_mgr_tbl_set_symmetric_mode_wrapper(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    bool update_hw,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp,
    bool update_dflt);

pipe_status_t pipe_mgr_tbl_get_symmetric_mode(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *symmetric,
                                              scope_num_t *num_scopes,
                                              scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_mat_tbl_get_first_stage_table(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t h,
    profile_id_t *profile_id_p,
    uint32_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_handle_p);

void pipe_mgr_tbl_log_specs(bf_dev_id_t dev_id,
                            profile_id_t prof_id,
                            pipe_mat_tbl_hdl_t tbl_hdl,
                            struct pipe_mgr_mat_data *data,
                            cJSON *mat_ent,
                            bool is_default);

void pipe_mgr_tbl_restore_specs(bf_dev_id_t dev_id,
                                profile_id_t prof_id,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                cJSON *mat_ent,
                                pipe_tbl_match_spec_t *ms,
                                pipe_action_spec_t *as,
                                pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_restore_mat_tbl_key_state(
    bf_dev_id_t device_id,
    pipe_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t *move_list);

pipe_status_t pipe_mgr_cleanup_default_entry(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_target_t dev_tgt,
                                             pipe_mat_tbl_info_t *mat_tbl_info,
                                             pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_place_default_entry(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_target_t dev_tgt,
                                           pipe_mat_tbl_info_t *mat_tbl_info,
                                           pipe_act_fn_hdl_t act_fn_hdl,
                                           pipe_action_spec_t *act_spec,
                                           pipe_mat_ent_hdl_t *ent_hdl_p,
                                           pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_get_default_entry(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_mat_tbl_info_t *mat_tbl_info,
                                         pipe_action_spec_t *pipe_action_spec,
                                         pipe_act_fn_hdl_t *act_fn_hdl,
                                         bool from_hw);

pipe_status_t pipe_mgr_tbl_process_move_list(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             enum pipe_mgr_table_owner_t owner,
                                             pipe_mgr_move_list_t *move_list,
                                             uint32_t *processed,
                                             bool free_when_done);

void pipe_mgr_tbl_free_move_list(enum pipe_mgr_table_owner_t owner,
                                 pipe_mgr_move_list_t *move_list,
                                 bool free_data);
#endif
