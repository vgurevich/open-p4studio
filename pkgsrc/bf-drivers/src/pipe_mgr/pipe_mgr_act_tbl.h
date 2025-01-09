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
 * @file pipe_mgr_act_tbl.h
 * @date
 *
 *
 * Contains definitions relating to pipe mgr's action data table management
 */
#ifndef _PIPE_MGR_ACT_TBL_H
#define _PIPE_MGR_ACT_TBL_H

/* Module header includes */

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_move_list.h"

#define PIPE_ADT_ENT_HDL_INVALID_HDL 0xdeadbeef
#define PIPE_ADT_ENT_IDX_INVALID_IDX 0xdeadbeef

typedef uint32_t pipe_adt_ent_idx_t;

/* Exported routines */

/* API to install an entry into an action data table to a particular RMT stage
 */
pipe_status_t rmt_adt_ent_add(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    uint32_t direct_map_tbl_index, /* Optional directmap index */
    rmt_tbl_hdl_t stage_table_handle,
    rmt_virt_addr_t *adt_virt_addr_p,
    bool update);

/* Activate an indirectly installed action data table entry in a stage */
pipe_status_t rmt_adt_ent_activate_stage(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         bf_dev_pipe_t pipe_id,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         uint8_t stage_id,
                                         pipe_idx_t entry_idx,
                                         rmt_virt_addr_t *adt_virt_addr_p,
                                         bool update);

/* De-activate an indirectly installed action data table entry in an stage */
pipe_status_t rmt_adt_ent_deactivate_stage(bf_dev_pipe_t pipe_id,
                                           bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint8_t stage_id,
                                           pipe_idx_t entry_idx);

/* API to reserve a group of contiguous action data table entries for a
 * action data table in the given stage id.
 */
pipe_status_t rmt_adt_ent_group_reserve(
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    /* The logical index of the base entry */
    uint32_t *adt_base_idx,
    /* Number of entries to be reserved for the group */
    uint32_t num_entries_per_block,
    /* If num_blocks > 1, address should always be aligned with 128 (7 trailing
       zeroes) */
    uint32_t num_blocks,
    uint32_t pipe_api_flags);

/* API to delete a group of action data table entries pointed to by
 * action data table entry handle array passed.
 */
pipe_status_t rmt_adt_ent_group_delete(
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    /* The logical index of the base entry */
    pipe_idx_t adt_base_idx,
    /* Number of entries to be reserved for the group */
    uint32_t num_entries_per_block,
    /* If num_blocks > 1, address should always be aligned with 128 (7 trailing
       zeroes) */
    uint32_t num_blocks,
    uint32_t pipe_api_flags);

/* API to install an entry with the given action data handle at a particular
 * offset in a stage
 */

pipe_status_t rmt_adt_ent_install(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    /* Offset at which to install the entry */
    uint32_t offset,
    bool hw_update);

/* API to install an entry with the given action data handle at a particular
 * offset in a stage
 */
pipe_status_t rmt_adt_ent_uninstall(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    /* Offset at which to install the entry */
    pipe_adt_ent_idx_t offset);

/* API to get a virtual address for a logical index */
pipe_status_t rmt_adt_ent_get_addr(bf_dev_id_t device_id,
                                   bf_dev_pipe_t pipe_id,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   uint8_t stage_id,
                                   pipe_idx_t entry_idx,
                                   rmt_virt_addr_t *adt_virt_addr_p);

pipe_status_t pipe_adt_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    struct pipe_mgr_adt_move_list_t *move_list,
    uint32_t *processed);

pipe_status_t pipe_mgr_adt_mgr_ent_add(
    bf_dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list);

pipe_status_t pipe_mgr_adt_get_mbr_id_hdl_int(
    bf_dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t ent_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_adt_mbr_id_t *adt_mbr_id,
    bf_dev_pipe_t *adt_mbr_pipe,
    pipe_mgr_adt_ent_data_t *adt_ent_data);

/* pipe_adt_ent_modify : */

pipe_status_t pipe_mgr_adt_mgr_ent_modify(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list);

pipe_status_t pipe_mgr_adt_mgr_ent_del(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list);

pipe_status_t pipe_mgr_adt_add_init_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    struct pipe_mgr_adt_move_list_t **move_list);

pipe_status_t pipe_mgr_adt_init_entry_hdl_get(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p);

pipe_status_t pipe_mgr_adt_txn_commit(bf_dev_id_t dev_id,
                                      pipe_adt_tbl_hdl_t tbl_hdl,
                                      bf_dev_pipe_t *pipes_list,
                                      unsigned nb_pipes);
pipe_status_t pipe_mgr_adt_txn_abort(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t tbl_hdl,
                                     bf_dev_pipe_t *pipes_list,
                                     unsigned nb_pipes);
pipe_status_t rmt_adt_ent_place(bf_dev_id_t device_id,
                                bf_dev_pipe_t pipe_id,
                                pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                pipe_adt_ent_hdl_t adt_ent_hdl,
                                uint8_t stage_id,
                                pipe_idx_t *entry_idx,
                                uint32_t pipe_api_flags);

pipe_status_t rmt_adt_ent_remove(bf_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 dev_stage_t stage_id,
                                 uint32_t pipe_api_flags);

pipe_status_t rmt_adt_ent_non_sharable_add(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           pipe_act_fn_hdl_t *act_fn_hdl_p,
                                           uint32_t pipe_api_flags);

pipe_status_t rmt_adt_ent_non_sharable_get(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           pipe_act_fn_hdl_t *act_fn_hdl_p);

pipe_status_t rmt_adt_ent_non_sharable_del(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint32_t pipe_api_flags);

pipe_status_t pipe_mgr_adt_mgr_get_phy_addrs(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_adt_ent_idx_t adt_stage_ent_idx,
    rmt_tbl_hdl_t stage_table_hdl,
    rmt_virt_addr_t virt_addr,
    uint64_t *phy_addrs,
    uint32_t *num_phy_addrs,
    uint32_t *entry_position);

pipe_status_t pipe_mgr_adt_mgr_get_ent_hdl_from_location(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t virt_addr,
    bool sharable,
    pipe_adt_ent_hdl_t *adt_ent_hdl);

pipe_status_t pipe_mgr_adt_get_entry(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw);

pipe_status_t pipe_mgr_adt_get_plcmt_data(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    struct pipe_mgr_adt_move_list_t **move_list);

#endif /* _PIPE_MGR_ACT_TBL_H */
