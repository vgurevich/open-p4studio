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
 * @file pipe_mgr_adt_mgr_ha_int.h
 * @date
 *
 *
 * Contains definitions for internal consumption of the action data
 * table manager for HA purposes.
 */

#ifndef _PIPE_MGR_ADT_MGR_HA_INT_H
#define _PIPE_MGR_ADT_MGR_HA_INT_H

/* Standard header includes */
#include <stdio.h>
#include <stdint.h>

/* Module header includes */
#include <target-utils/id/id.h>
#include <target-utils/map/map.h>

/* Local header includes */
#include "pipe_mgr_rmt_cfg.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_hitless_ha.h"

/* Structure definition adt mgr HA cookie, which the clients can pass in during
 * decoding of action data entries or assigning temp adt ent hdls. This gives
 * quick access to ADT mgr structures and avoids having to look it up during
 * each API invocation.
 */
typedef struct pipe_mgr_adt_ha_cookie_ {
  pipe_mgr_adt_t *adt;
  pipe_mgr_adt_data_t *adt_tbl_data;
  pipe_mgr_adt_stage_info_t *adt_stage_info;
} pipe_mgr_adt_ha_cookie_t;

/* Structure definition for HA info at the LLP at the pipe table level */
typedef struct pipe_mgr_adt_pipe_ha_llp_info_ {
  /* A temp adt ent hdl allocator */
  bf_id_allocator *ent_hdl_allocator;
} pipe_mgr_adt_pipe_ha_llp_info_t;

/* Structure definition for HA info at the HLP at the pipe table level */
typedef struct pipe_mgr_adt_pipe_ha_hlp_info_ {
  pipe_mgr_spec_map_t spec_map;
} pipe_mgr_adt_pipe_ha_hlp_info_t;

/* Structure definition for HA info at the LLP at the stage level */
typedef struct pipe_mgr_adt_stage_ha_llp_info_ {
  /* A map from stage entry idx to assigned temp adt ent hdl */
  bf_map_t ent_idx_to_ent_hdl;
} pipe_mgr_adt_stage_ha_llp_info_t;

typedef struct pipe_mgr_adt_ha_llp_htbl_node_ {
  pipe_adt_ent_hdl_t adt_ent_hdl;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_action_data_spec_t *act_data_spec;
} pipe_mgr_adt_ha_llp_htbl_node_t;

pipe_status_t pipe_mgr_adt_mgr_decode_to_act_data_spec(
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    rmt_tbl_hdl_t stage_table_handle,
    pipe_adt_ent_idx_t adt_entry_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    bool from_hw,
    pipe_sess_hdl_t *sess_hdl);

pipe_status_t pipe_mgr_adt_mgr_decode_virt_addr(
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    rmt_virt_addr_t virt_addr,
    pipe_adt_ent_idx_t *logical_action_idx);

pipe_status_t pipe_mgr_adt_mgr_get_temp_adt_ent_hdl(
    bf_dev_id_t device_id,
    pipe_tbl_dir_t gress,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    rmt_virt_addr_t virt_addr,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_idx_t *logical_action_idx);

pipe_status_t pipe_mgr_adt_mgr_ha_get_cookie(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie);

pipe_status_t pipe_mgr_adt_pipe_ha_llp_info_init(
    pipe_mgr_adt_data_t *adt_tbl_data);

void pipe_mgr_adt_pipe_ha_llp_info_destroy(pipe_mgr_adt_data_t *adt_tbl_data);

pipe_status_t pipe_mgr_adt_stage_ha_llp_info_init(
    pipe_mgr_adt_stage_info_t *adt_stage_info);

void pipe_mgr_adt_stage_ha_llp_info_destroy(
    pipe_mgr_adt_stage_info_t *adt_stage_info);

pipe_status_t pipe_mgr_adt_mgr_update_llp_state_for_ha(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_idx_t logical_action_idx,
    bool sharable);

pipe_status_t pipe_mgr_adt_hlp_restore_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_move_list_t *move_list,
    uint32_t *success_count,
    pd_ha_restore_cb_1 cb);

pipe_status_t pipe_mgr_adt_llp_restore_state(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_move_list_t **move_list);

pipe_status_t pipe_mgr_adt_mgr_create_hlp_state(
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_idx_t logical_action_idx,
    bool place_entry);

bool pipe_mgr_adt_is_loc_occupied(void);

bool pipe_mgr_adt_is_hdl_present_at_loc(void);

pipe_status_t pipe_mgr_adt_hlp_compute_delta_changes(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl,
    pipe_mgr_adt_move_list_t **ml);

void pipe_mgr_adt_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl);

void pipe_mgr_adt_cleanup_llp_ha_state(bf_dev_id_t device_id,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl);

/* Return true if action data spec and action function handle are identical. */
bool pipe_mgr_adt_cmp_entries(bf_dev_id_t device_id,
                              pipe_adt_tbl_hdl_t adt_tbl_hdl,
                              pipe_adt_ent_hdl_t ha_adt_ent_hdl,
                              pipe_adt_ent_hdl_t adt_ent_hdl);

#endif  // _PIPE_MGR_ADT_MGR_HA_INT_H
