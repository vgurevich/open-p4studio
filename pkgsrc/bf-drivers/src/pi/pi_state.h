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


#ifndef _PI_STATE_H__
#define _PI_STATE_H__

#include <PI/int/pi_int.h>
#include <PI/pi.h>
#include <PI/pi_learn.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>

#include <stddef.h>

void pi_state_init(size_t num_devices);

void pi_state_shared_lock(pi_dev_id_t dev_id);

void pi_state_exclusive_lock(pi_dev_id_t dev_id);

void pi_state_unlock(pi_dev_id_t dev_id);

// returns 0 if success, 1 otherwise
int pi_state_assign_device(pi_dev_id_t dev_id,
                           const pi_p4info_t *p4info,
                           const char *context_json_path);

void pi_state_remove_device(pi_dev_id_t dev_id);

bool pi_state_is_device_assigned(pi_dev_id_t dev_id);

void pi_state_destroy();

// also works for act profiles: returns the handle of the action data table
uint32_t pi_state_table_id_to_handle(pi_dev_id_t dev_id, pi_p4_id_t id);

// only works for regular match tables
uint32_t pi_state_action_id_to_handle(pi_dev_id_t dev_id,
                                      pi_p4_id_t t_id,
                                      pi_p4_id_t a_id);

pi_p4_id_t pi_state_action_handle_to_id(pi_dev_id_t dev_id,
                                        pi_p4_id_t t_id,
                                        uint32_t a_handle);

// returns 0 if not selector
uint32_t pi_state_act_prof_get_selector_handle(pi_dev_id_t dev_id,
                                               pi_p4_id_t act_prof_id);

// to obtain resource handles
uint32_t pi_state_res_id_to_handle(pi_dev_id_t dev_id, pi_p4_id_t id);

pi_p4_id_t pi_state_res_handle_to_id(pi_dev_id_t dev_id, uint32_t handle);

typedef struct {
  pi_p4_id_t res_id;
  bool is_match_bound;
} pi_state_table_indirect_res_t;

typedef enum {
  PI_STATE_INDIRECT_RES_ACCESS_MODE_IMMEDIATE,
  PI_STATE_INDIRECT_RES_ACCESS_MODE_ARG
} pi_state_indirect_res_access_mode_t;

typedef struct {
  pi_p4_id_t action_id;
  pi_p4_id_t res_id;
  pi_state_indirect_res_access_mode_t access_mode;
  union {
    size_t immediate;
    struct {
      size_t offset;
      size_t width;
    } action_data_pos;
  } index;
} pi_state_indirect_res_access_info_t;

const pi_state_table_indirect_res_t *pi_state_table_indirect_res(
    pi_dev_id_t dev_id, pi_p4_id_t t_id, size_t *num_indirect_res);

// returns NULL if no access for this action
const pi_state_indirect_res_access_info_t *pi_state_indirect_res_access_info(
    pi_dev_id_t dev_id, pi_p4_id_t t_id, pi_p4_id_t a_id, pi_p4_id_t res_id);

// action profiles live state

// returns 0 if success, 1 otherwise
int pi_state_ms_grp_add_mbr(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h,
                            pi_indirect_handle_t mbr_h);

void pi_state_ms_grp_remove_mbr(pi_dev_id_t dev_id,
                                pi_p4_id_t act_prof_id,
                                pi_indirect_handle_t grp_h,
                                pi_indirect_handle_t mbr_h);

typedef void (*PIMSGrpFn)(pi_dev_id_t dev_id,
                          pi_p4_id_t act_prof_id,
                          pi_indirect_handle_t mbr_h,
                          pi_indirect_handle_t grp_h,
                          void *aux);

void pi_state_ms_mbr_apply_to_grps(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_h,
                                   PIMSGrpFn grp_fn,
                                   void *aux);

void pi_state_ms_mbr_create(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t mbr_h);

void pi_state_ms_mbr_delete(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t mbr_h);

void pi_state_ms_mbr_set_act(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t mbr_h,
                             pi_p4_id_t action_id);

void pi_state_ms_grp_set_act(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t grp_h,
                             pi_p4_id_t action_id);

pi_p4_id_t pi_state_ms_mbr_get_act(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_h);

pi_p4_id_t pi_state_ms_grp_get_act(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t grp_h);

void pi_state_ms_grp_create(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h);

void pi_state_ms_grp_delete(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h);

typedef struct {
  pi_p4_id_t res_id;
  size_t res_idx;
} pi_state_indirect_res_t;

void pi_state_ms_mbr_add_res(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t mbr_h,
                             pi_state_indirect_res_t *res);

const pi_state_indirect_res_t *pi_state_ms_mbr_get_res(
    pi_dev_id_t dev_id,
    pi_p4_id_t act_prof_id,
    pi_indirect_handle_t mbr_h,
    size_t *num_indirect_res);

const pi_state_indirect_res_t *pi_state_ms_grp_get_res(
    pi_dev_id_t dev_id,
    pi_p4_id_t act_prof_id,
    pi_indirect_handle_t grp_h,
    size_t *num_indirect_res);

typedef struct {
  const pi_p4info_t *p4info;
  pi_p4_id_t table_id;
  pipe_mat_tbl_hdl_t table_handle;
  pipe_tbl_match_spec_t pipe_match_spec;
  pipe_act_fn_hdl_t action_handle;
  pipe_action_spec_t pipe_action_spec;
  pi_match_key_t match_key;
} pi_state_idle_time_scratchspace_t;

pi_state_idle_time_scratchspace_t *pi_state_idle_time_get_scratchspace(
    pi_dev_id_t dev_id, pi_p4_id_t t_id);

typedef struct {
  pi_p4_id_t digest_id;
  pipe_fld_lst_hdl_t digest_handle;
  size_t entry_size;
} pi_state_digest_state_t;

pi_state_digest_state_t *pi_state_digest_get(pi_dev_id_t dev_id,
                                             pi_p4_id_t digest_id);

pipe_sess_hdl_t pi_state_digest_get_lrn_session();

uint32_t pi_state_digest_update_lrn_timeout(pi_dev_id_t dev_id,
                                            uint32_t lrn_timeout_us);

#endif  // _PI_STATE_H__
