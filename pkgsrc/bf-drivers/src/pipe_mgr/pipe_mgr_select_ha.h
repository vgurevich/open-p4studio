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
 * @file pipe_mgr_select_ha.h
 * @date
 *
 *
 * Contains definitions for internal consumption of the selector
 * table manager for HA purposes.
 */

#ifndef _PIPE_MGR_SELECT_HA_H_
#define _PIPE_MGR_SELECT_HA_H_
/* Standard header includes */
#include <stdio.h>
#include <stdint.h>

/* Module header includes */
#include <target-utils/id/id.h>
#include <target-utils/map/map.h>
#include <target-utils/hashtbl/bf_hashtbl.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_rmt_cfg.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_adt_mgr_ha_int.h"

/* Structure definition sel mgr HA cookie, which the clients can pass in during
 * decoding of selector entries or assigning temp selector hdls. This gives
 * quick access to sel mgr structures and avoids having to look it up during
 * each API invocation.
 */
typedef struct pipe_mgr_sel_ha_cookie_t {
  sel_tbl_info_t *sel_tbl_info;
  sel_tbl_t *sel_tbl;
  sel_tbl_stage_info_t *sel_stage_info;
} pipe_mgr_sel_ha_cookie_t;

/* Structure definition for group info needed for HA */
typedef struct pipe_mgr_sel_grp_ha_info_t {
  pipe_sel_grp_hdl_t grp_hdl;
  bf_dev_pipe_t pipe_id;
  uint32_t sel_base_idx;
  uint32_t sel_len;
  pipe_adt_ent_idx_t *adt_base_idx;
} pipe_mgr_sel_grp_ha_info_t;

/* Structure definition for group info needed for config replay */
typedef struct pipe_mgr_sel_grp_replay_info_t {
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_sel_grp_id_t grp_id;
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t max_grp_size;
  bool matched;
  uint32_t num_mbrs;
  pipe_sel_grp_mbr_hdl_t *mbr_hdls;
  bool *mbr_enable;     /* Is member enabled/disabled */
  uint32_t *mbr_weight; /* Member Weight */
} pipe_mgr_sel_grp_replay_info_t;

/* Structure definition for HA info at the LLP at the selector table level */
typedef struct pipe_mgr_sel_ha_llp_info_t {
  /* A temp sel grp hdl allocator */
  Pvoid_t ha_sel_grp_array;
} pipe_mgr_sel_ha_llp_info_t;

/* Structure definition for HA info at the LLP at the pipe level */
typedef struct pipe_mgr_sel_pipe_ha_llp_info_t {
  bf_map_t idx_to_grp_info;
} pipe_mgr_sel_pipe_ha_llp_info_t;

/* Structure definition for HA info at the HLP at the pipe level */
typedef struct pipe_mgr_sel_pipe_ha_hlp_info_t {
  bf_map_t idx_to_grp_info;
  bf_map_t replay_hdl_to_info;
} pipe_mgr_sel_pipe_ha_hlp_info_t;

pipe_status_t pipe_mgr_sel_get_temp_sel_grp_hdl(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    rmt_virt_addr_t virt_addr,
    uint32_t sel_len,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_sel_ha_cookie_t *sel_ha_cookie,
    pipe_sel_grp_hdl_t *grp_hdl,
    pipe_idx_t *logical_sel_idx);

pipe_status_t pipe_mgr_sel_ha_get_cookie(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_sel_ha_cookie_t *sel_ha_cookie);

pipe_status_t pipe_mgr_sel_ha_llp_init(sel_tbl_info_t *sel_tbl_info);

pipe_status_t pipe_mgr_sel_pipe_ha_llp_init(sel_tbl_t *sel_tbl);

pipe_status_t pipe_mgr_sel_pipe_ha_hlp_init(sel_tbl_t *sel_tbl);

pipe_status_t pipe_mgr_sel_update_llp_state_for_ha(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_ha_cookie_t *sel_cookie,
    pipe_idx_t logical_sel_idx,
    uint32_t sel_len,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    rmt_virt_addr_t adt_virt_addr,
    pipe_idx_t *logical_adt_idx);

pipe_status_t pipe_mgr_sel_hlp_restore_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t *move_list,
    uint32_t *success_count,
    pd_ha_restore_cb_1 cb);

pipe_status_t pipe_mgr_sel_llp_restore_state(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t **move_list);

pipe_status_t pipe_mgr_sel_update_hlp_ref(bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_sel_tbl_hdl_t tbl_hdl,
                                          pipe_sel_grp_hdl_t grp_hdl);

pipe_status_t pipe_mgr_sel_hlp_update_state(bf_dev_id_t device_id,
                                            bf_dev_pipe_t pipe_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            pipe_sel_tbl_hdl_t tbl_hdl,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            uint32_t sel_base_idx);

pipe_status_t pipe_mgr_sel_hlp_compute_delta_changes(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t **move_head_p);

void pipe_mgr_selector_cleanup_llp_ha_state(bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            bool is_backup);

void pipe_mgr_selector_cleanup_pipe_llp_ha_state(sel_tbl_t *sel_tbl);
void pipe_mgr_selector_tbl_cleanup_llp_ha_state(sel_tbl_info_t *sel_tbl_info);

void pipe_mgr_selector_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            bool is_backup);

void pipe_mgr_selector_cleanup_pipe_hlp_ha_state(sel_tbl_t *sel_tbl);

#endif /* _PIPE_MGR_SELECT_HA_H */
