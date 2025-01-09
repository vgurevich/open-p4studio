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
 * @file pipe_mgr_adt_mgr_int.h
 * @date
 *
 *
 * Contains definitions for internal consumption of the action data
 * table manager.
 */

#ifndef _PIPE_MGR_ADT_MGR_INT_H
#define _PIPE_MGR_ADT_MGR_INT_H

/* Standard header includes */
#include <stdint.h>

/* Module header includes */
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <target-utils/id/id.h>
#include <target-utils/map/map.h>
#include <target-utils/power2_allocator/power2_allocator.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_hitless_ha.h"

/* Structure definitions */

typedef struct pipe_mgr_adt_txn_state_ {
  bf_map_t dirtied_ent_hdl_htbl;
} pipe_mgr_adt_txn_state_t;

/* Structure definition for RAM allocation info */
typedef struct pipe_mgr_adt_ram_alloc_info_ {
  /* Number of wide-word blocks allocated */
  uint8_t num_wide_word_blks;
  /* An array containing RAM ids per wide-word blocks */
  rmt_tbl_word_blk_t *tbl_word_blk;
  /* Number of logical entries packed into a single wide-word */
  uint8_t num_entries_per_wide_word;
  /* Number of RAM units making up the wide-word */
  uint8_t num_rams_in_wide_word;

} pipe_mgr_adt_ram_alloc_info_t;

typedef struct pipe_mgr_adt_stage_location_ {
  uint8_t stage_id;
  pipe_adt_ent_idx_t entry_idx;
  uint32_t ref_count;
  struct pipe_mgr_adt_stage_location_ *next;
  struct pipe_mgr_adt_stage_location_ *prev;
} pipe_mgr_adt_stage_location_t;

typedef struct pipe_mgr_adt_entry_info_t {
  pipe_mgr_adt_stage_location_t *sharable_stage_location;
  pipe_mgr_adt_ent_data_t *entry_data;
  bf_dev_pipe_t pipe_id;
  uint32_t num_references;
  pipe_adt_mbr_id_t mbr_id;
  pipe_adt_ent_hdl_t handle;
} pipe_mgr_adt_entry_info_t;

typedef struct pipe_mgr_adt_entry_phy_info_t {
  pipe_mgr_adt_ent_data_t *entry_data;
  pipe_mgr_adt_stage_location_t *sharable_stage_location;
  pipe_mgr_adt_stage_location_t *non_sharable_stage_location;
  bf_dev_pipe_t pipe_id;
  uint8_t num_stages;
} pipe_mgr_adt_entry_phy_info_t;

/* Structure definition for stage level info */
typedef struct pipe_mgr_adt_stage_info_ {
  /* Stage id */
  dev_stage_t stage_id;
  /* Table size within the stage as determined by the memory allocation. */
  uint32_t num_entries;
  /* Entry allocator : Either the power2_allocator or the regular id allocator
   */
  void *entry_allocator;
  /* Backup of the entry allocator */
  void *backup_entry_allocator;
  /* Pointer to the ram allocation information for this instance of
   * the table.
   */
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info;
  /* Number of entries occupied in this stage */
  uint32_t num_entries_occupied;
  /* Offset to be subtracted from the global entry index to get the stage level
   * entry index.
   */
  uint32_t stage_offset;
  struct sub_tbl_offset_t {
    uint32_t offset;
    rmt_tbl_hdl_t stage_table_handle;
  } * sub_tbl_offsets;
  uint8_t num_sub_tbls;
  uint8_t **shadow_ptr_arr;
  mem_id_t *mem_id_arr;
  /* State required for HA at LLP at the stage table level */
  void *ha_llp_info;
} pipe_mgr_adt_stage_info_t;

/* Structure definition for action data table data which is
 * per independently-managed instance of the action data table.
 */
typedef struct pipe_mgr_adt_data_ {
  /* Back pointer to pipe_mgr_adt_t */
  struct pipe_mgr_adt_ *adt;
  /* Number of stages in which this instance of the table exists */
  uint32_t num_stages;
  /* Table size as specified by the P4 program. */
  uint32_t num_entries;
  /* Pipe-id to which this instance of the table belongs to */
  bf_dev_pipe_t pipe_id;
  /* A bitmap of pipes in which this managed instance of the table is present
   * in.
   */
  pipe_bitmap_t pipe_bmp;
  /* Number of entries occupied */
  uint32_t num_entries_occupied;
  /* Number of entries occupied in physical device
   * This should return same value as num_entries_occupied
   * where HLP-LLP demarcation isnt there.*/
  uint32_t num_entries_llp_occupied;
  /* Backup number of entries occupied (before txn start) */
  uint32_t backup_num_entries_occupied;
  /* Flag to determine whether backup_num_entries_occupied is set */
  bool backup_set;
  /* Pointer to an array of stage level info */
  pipe_mgr_adt_stage_info_t *adt_stage_info;
  /* Transaction state information */
  pipe_mgr_adt_txn_state_t *txn_state;
  /* Entry allocator */
  bf_id_allocator *ent_hdl_allocator;
  /* Number of entries programmed down to the hardware at LLP */
  uint32_t num_entries_programmed;
  /* State required for HA at LLP at the pipe table level */
  void *ha_llp_info;
  /* State required for HA to HLP at the pipe table level */
  void *ha_hlp_info;
  /* Member id (pipe_adt_mbr_id_t) to entry info (pipe_mgr_adt_entry_info_t). */
  bf_map_t mbr_id_map;
  /* An map containing a mapping from entry handle to ref-count */
  bf_map_t adt_ent_refs;
  /* Entry handle (pipe_adt_ent_hdl_t) to entry info
   * (pipe_mgr_adt_entry_info_t). HLP state. */
  bf_map_t entry_info_htbl;
  /* Entry handle (pipe_adt_ent_hdl_t) to entry phy info
   * pipe_mgr_adt_entry_phy_info_t.  LLP state. */
  bf_map_t entry_phy_info_htbl;
  /* Holds pipe_adt_mbr_id_t values of the const init time entries. */
  bf_map_t const_init_entries;
} pipe_mgr_adt_data_t;

/* Structure definition for action data table. This is the center-piece of the
 * action data table manager software structures.
 */
typedef struct pipe_mgr_adt_ {
  char *name;
  pipe_tbl_dir_t direction;
  /* Action data table handle */
  pipe_adt_tbl_hdl_t adt_tbl_hdl;
  /* Device - id */
  bf_dev_id_t dev_id;
  /* Device info */
  rmt_dev_info_t *dev_info;
  /* A flag indicating if this table is symmetric or asymmetric */
  bool symmetric;
  /* Scope info */
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  /* Table size as specified by the P4 program. */
  uint32_t num_entries;
  /* Number of instances of the table which has to be managed.
   * As many as the number of pipes for a table which is asymmetrically
   * populated, or just ONE for a bel which is symmetrically populated
   * across all pipes.
   */
  uint32_t num_tbls;
  /* Pointer to an array of table data */
  pipe_mgr_adt_data_t *adt_tbl_data;
  profile_id_t profile_id;  // profile_id
  pipe_bitmap_t pipe_bmp;   // pipe bitmap
  /* How this action data table is referred to by the match table */
  pipe_tbl_ref_type_t ref_type;
  bf_dev_pipe_t lowest_pipe_id;
  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;
  /* Maximum size of action data spec */
  uint32_t max_act_data_size;
  /* Used to enable caching member id in adt. Enabled on entry add. */
  bool cache_id;
} pipe_mgr_adt_t;

typedef struct adt_loc_info_ {
  pipe_adt_ent_idx_t entry_idx;
  uint8_t wide_word_blk_idx;
  uint8_t sub_entry;
  uint32_t ram_line_num;
} adt_loc_info_t;

/************************************************************************
 * Function prototypes
 ***********************************************************************/

pipe_mgr_adt_t *pipe_mgr_adt_get(bf_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl);

/**
 * The following two functions return the number of adt entries added through
 * API calls at the hlp and llp. Used internally to check whether an action
 * table can change its symmetric property.
 */
uint32_t pipe_mgr_adt_get_num_hlp_entries(pipe_mgr_adt_t *adt);

uint32_t pipe_mgr_adt_get_num_llp_entries(pipe_mgr_adt_t *adt);

pipe_status_t pipe_mgr_adt_tbl_get_num_entries_placed(dev_target_t dev_tgt,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p);

pipe_status_t pipe_mgr_adt_tbl_get_num_entries_reserved(dev_target_t dev_tgt,
                                                        pipe_tbl_hdl_t tbl_hdl,
                                                        uint32_t *count_p);

pipe_status_t pipe_mgr_adt_tbl_get_num_entries_programmed(
    dev_target_t dev_tgt, pipe_tbl_hdl_t tbl_hdl, uint32_t *count_p);

pipe_mgr_adt_ram_alloc_info_t *pipe_mgr_adt_get_ram_alloc_info(
    pipe_mgr_adt_data_t *adt_tbl_data, uint8_t stage_id);

pipe_status_t pipe_mgr_adt_get_num_ram_units(bf_dev_id_t dev_id,
                                             pipe_adt_tbl_hdl_t tbl_hdl,
                                             bf_dev_pipe_t pipe_id,
                                             uint8_t stage_id,
                                             uint32_t *num_ram_units);

pipe_status_t pipe_mgr_adt_backup_entry_handle(pipe_mgr_adt_t *adt,
                                               pipe_adt_ent_hdl_t adt_ent_hdl);

void pipe_mgr_adt_destroy_entry_handle_backup(pipe_mgr_adt_t *adt,
                                              pipe_adt_ent_hdl_t adt_ent_hdl);

void pipe_mgr_adt_restore_entry_handle_backup(pipe_mgr_adt_t *adt,
                                              pipe_adt_ent_hdl_t adt_ent_hdl);

void pipe_mgr_adt_destroy_entry_idx_allocator_backup(
    pipe_mgr_adt_t *adt, pipe_mgr_adt_stage_info_t *adt_stage_info);

void pipe_mgr_adt_restore_entry_idx_allocator_backup(
    pipe_mgr_adt_t *adt, pipe_mgr_adt_stage_info_t *adt_stage_info);

pipe_status_t pipe_mgr_adt_backup_ram_shadow_copy(
    pipe_mgr_adt_stage_info_t *adt_stage_stage_info,
    pipe_adt_ent_idx_t entry_idx);

void pipe_mgr_adt_restore_ram_shadow_copy_backup(
    pipe_mgr_adt_stage_info_t *adt_stage_info, pipe_adt_ent_idx_t entry_idx);

void pipe_mgr_adt_destroy_ram_shadow_copy_backup(
    pipe_mgr_adt_stage_info_t *adt_stage_info, pipe_adt_ent_idx_t entry_idx);

pipe_mgr_adt_stage_info_t *pipe_mgr_adt_get_stage_info(
    pipe_mgr_adt_data_t *adt_data, uint8_t stage_id);

pipe_status_t pipe_mgr_adt_insert_ent_hdl_mapping(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_adt_ent_idx_t adt_ent_idx,
    bool isTxn);

pipe_status_t pipe_mgr_adt_add_txn_entry(pipe_mgr_adt_t *adt,
                                         pipe_adt_ent_hdl_t action_entry_hdl);

pipe_status_t pipe_mgr_adt_instance_txn_commit(
    pipe_mgr_adt_data_t *adt_tbl_data, pipe_adt_ent_hdl_t adt_ent_hdl);

pipe_status_t pipe_mgr_adt_data_txn_commit(pipe_mgr_adt_t *adt,
                                           pipe_mgr_adt_data_t *adt_data);

pipe_status_t pipe_mgr_adt_data_txn_abort(pipe_mgr_adt_t *adt,
                                          pipe_mgr_adt_data_t *adt_data);

pipe_adt_ent_idx_t pipe_mgr_adt_get_ent_idx(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    bool isTxn);

pipe_mgr_adt_data_t *pipe_mgr_get_adt_instance(pipe_mgr_adt_t *adt,
                                               bf_dev_pipe_t pipe_id);

pipe_mgr_adt_data_t *pipe_mgr_get_adt_instance_from_entry(pipe_mgr_adt_t *adt,
                                                          int entry_hdl);

pipe_status_t pipe_mgr_adt_stage_shadow_mem_init(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_bitmap_t *pipe_bmp,
    bf_dev_pipe_t pipe_id);

pipe_mgr_adt_entry_info_t *pipe_mgr_adt_get_entry_info(
    pipe_mgr_adt_t *adt, pipe_adt_ent_hdl_t adt_ent_hdl);

pipe_mgr_adt_entry_phy_info_t *pipe_mgr_adt_get_entry_phy_info(
    pipe_mgr_adt_t *adt, pipe_adt_ent_hdl_t adt_ent_hdl);

pipe_mgr_adt_entry_info_t *pipe_mgr_adt_entry_deep_copy(
    pipe_mgr_adt_entry_info_t *entry);

void pipe_mgr_adt_entry_cleanup(pipe_mgr_adt_entry_info_t *entry);

pipe_status_t pipe_mgr_adt_execute_entry_add(
    pipe_mgr_adt_t *adt,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    bf_dev_pipe_t pipe_id,
    pipe_mgr_adt_move_list_t *move_list_node);

pipe_status_t pipe_mgr_adt_update_state_for_entry_activate(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx,
    bool *exists);

pipe_status_t pipe_mgr_adt_update_state_for_entry_install(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx);

pipe_status_t pipe_mgr_adt_update_state_for_entry_placement(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_mgr_adt_entry_info_t *adt_entry_info,
    pipe_adt_ent_idx_t *entry_idx,
    bool do_placement);

pipe_status_t pipe_mgr_adt_log_state(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t tbl_hdl,
                                     cJSON *adt_tbls);

pipe_status_t pipe_mgr_adt_restore_state(bf_dev_id_t dev_id, cJSON *adt_tbl);

pipe_status_t pipe_mgr_adt_get_phy_loc_info(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    rmt_tbl_hdl_t stage_tbl_hdl,
    pipe_adt_ent_idx_t entry_idx,
    adt_loc_info_t *loc_info);

#endif  // _PIPE_MGR_ADT_MGR_INT_H
