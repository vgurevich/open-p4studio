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

#ifndef _PIPE_MGR_EXM_TBL_MGR_INT_H
#define _PIPE_MGR_EXM_TBL_MGR_INT_H

/* Standard header includes */

/* Module header includes */
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <target-utils/map/map.h>
#include <target-utils/hashtbl/bf_hashtbl.h>

/* Local header includes */
#include "cuckoo_move.h"
#include "pipe_mgr_hitless_ha.h"

#define PIPE_MGR_EXM_TRACE(                                               \
    _device_id_, _tbl_name, _ent_hdl_, _pipe_id_, _stage_id_, _str_, ...) \
  LOG_TRACE("EXM %s:%d %d:%s:0x%x:%d:%d " _str_,                          \
            __func__,                                                     \
            __LINE__,                                                     \
            _device_id_,                                                  \
            _tbl_name,                                                    \
            _ent_hdl_,                                                    \
            _pipe_id_,                                                    \
            _stage_id_,                                                   \
            __VA_ARGS__)

typedef struct pipe_mgr_exm_tbl_htbl_key_ {
  uint8_t device_id;
  uint8_t tbl_hdl;

} pipe_mgr_exm_tbl_htbl_key_t;

typedef struct pipe_exm_txn_state_key_ {
  pipe_mat_tbl_hdl_t tbl_hdl;
  bf_dev_id_t device_id;

} pipe_exm_txn_state_key_t;

typedef struct pipe_exm_tbl_key_ {
  pipe_mat_tbl_hdl_t tbl_hdl;
  uint8_t device_id;

} pipe_exm_tbl_key_t;

/* Structure for entry handle management */
typedef struct pipe_mgr_exm_ent_hdl_mgmt_ {
  /* Entry handle allocator */
  bf_id_allocator *ent_hdl_allocator;
  /* A backup entry handle allocator for txn purposes */
  bf_id_allocator *backup_ent_hdl_allocator;

} pipe_mgr_exm_ent_hdl_mgmt_t;

typedef struct pipe_mgr_exm_phy_entry_info_ {
  pipe_mat_ent_idx_t entry_idx;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
  mem_id_t *mem_id;
  uint8_t num_ram_units;
  pipe_adt_ent_hdl_t adt_ent_hdl;
  pipe_idx_t action_idx;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mgr_indirect_ptrs_t indirect_ptrs;
  bool selector_enabled;
  uint32_t direct_stful_seq_nu;
} pipe_mgr_exm_phy_entry_info_t;

typedef struct pipe_mgr_exm_ram_alloc_info_ {
  /* Number of wide-word blocks allocated */
  uint8_t num_wide_word_blks;
  /* An array containing RAM ids per wide-word blocks */
  rmt_tbl_word_blk_t *tbl_word_blk;

} pipe_mgr_exm_ram_alloc_info_t;

typedef struct pipe_mgr_exm_hash_way_data_ {
  /* Number of entries in this hash-way */
  uint32_t num_entries;
  /* Pointer to the ram allocation info for this instance of the table.
   */
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info;
  /* Number of RAM select bits that this hash-way uses */
  uint8_t num_ram_select_bits;
  /* Number of RAM line bits */
  uint8_t num_ram_line_bits;
  /* Ram select bit offset (from the LSB) within the wide-hash */
  uint8_t ram_select_start_offset;
  /* Bit offset of the hash-bits (from the LSB) within the wide-hash */
  uint8_t ram_line_start_offset;
  /* Hash function id which this hash-way uses, can be 0 or 1 */
  uint8_t hash_function_id;
  /*Number of subword bits in the hash to selects the subentry for tf1,2,3 it
   * will be zero*/
  uint8_t num_subword_bits;
  /* Offset which needs to be subtracted from the stage level index
   * to get the hash way index.
   */
  uint32_t offset;
  /* hashway offset within all the ways in this hash_function_id */
  uint8_t hashway_within_function_id;
  uint8_t *num_ram_units;
  bool **ram_unit_present;

} pipe_mgr_exm_hash_way_data_t;

typedef struct pipe_mgr_exm_pack_format_ {
  /* Number of logical entries packed into a single wide-word */
  uint8_t num_entries_per_wide_word;
  /* Number of RAM units making up the wide-word */
  uint8_t num_rams_in_wide_word;

} pipe_mgr_exm_pack_format_t;

typedef struct pipe_mgr_exm_stash_entry_info_ {
  uint32_t stash_id;
  uint8_t stash_match_data_select;
  uint8_t stash_hashbank_select;
  uint8_t hash_function_id;
} pipe_mgr_exm_stash_entry_info_t;

typedef struct pipe_mgr_exm_stash_ {
  /* Packing format for this table's stash */
  pipe_mgr_exm_pack_format_t pack_format;
  uint32_t num_stash_entries;
  uint32_t num_rams_per_stash;
  pipe_mgr_exm_stash_entry_info_t **stash_entries;
  uint8_t **shadow_ptr_arr;
  uint8_t *wide_word_indices;
} pipe_mgr_exm_stash_t;

typedef struct pipe_mgr_exm_stage_info_ {
  /* Stage id */
  dev_stage_t stage_id;
  /* Table direction: 0 - ingress, 1 - egress */
  pipe_tbl_dir_t direction;
  /* Number of entries in this stage */
  uint32_t num_entries;
  /* Capacity of the stage */
  uint32_t capacity;
  /* Stash Capacity of the stage */
  uint32_t stash_capacity;
  /* Offset that needs to be added to get the global logical entry index */
  uint32_t stage_offset;
  /* Number of hash-ways in this stage */
  uint8_t num_hash_ways;
  /* Per-hashway data */
  pipe_mgr_exm_hash_way_data_t *hashway_data;
  /* Packing format for this table */
  pipe_mgr_exm_pack_format_t *pack_format;
  /* Pointer to the cuckoo graph for this stage */
  cuckoo_move_graph_t *cuckoo_move_graph;
  /* Number of entries placed */
  uint32_t num_entries_placed;
  /* Number of entries occupied */
  uint32_t num_entries_occupied;
  /* Array of Judy1 arrays for managing sub-entries within a ram line */
  Pvoid_t *PJ1Array;
  /* The default miss entry index for this stage */
  pipe_mat_ent_idx_t default_miss_entry_idx;
  /* The default stash id for this stage */
  uint32_t default_stash_id;

  pipe_mgr_exm_edge_container_t *edge_container;
  /* Stage table handle */
  rmt_tbl_hdl_t stage_table_handle;
  /* Proxy hash record table containing proxy hash values
   * to help in finding out if a proxy hash value already exists
   * in the stage or not.
   */
  bf_hashtable_t *proxy_hash_tbl;
  /* A flag indicating if this stage idle table is locked */
  bool idle_locked;
  /* Idle table lock id if the above flag is set */
  lock_id_t idle_tbl_lock_id;
  /* A flag indicating if this stage stat table is locked */
  bool stats_locked;
  /* Stat table lock id if the above flag is set */
  lock_id_t stats_tbl_lock_id;
  /* A pre-allocated array of shadow pointers as many elements as there are
   * number of RAM-ids which form the wide-word
   */
  uint8_t **shadow_ptr_arr;
  /* A pre-allocated array of mem ids, as many elements as there are number of
   * RAM-ids which form the wide-word
   */
  mem_id_t *mem_id_arr;
  /* An array of indices which contains a given entry, pre-allocated for use
   * during processing.
   */
  uint8_t *wide_word_indices;
  /* LLP direct logical idx(hit_addr) to entry handle mapping */
  bf_map_t log_idx_to_ent_hdl_htbl;
  /* LLP, map logical idx (0..capacity-1) to entry handle */
  bf_map_t log_idx_to_occ;
  pipe_mat_ent_idx_t stash_idx;
  /* Stash pack format and locations */
  pipe_mgr_exm_stash_t *stash;

} pipe_mgr_exm_stage_info_t;

typedef struct pipe_mgr_exm_pipe_hlp_ha_info_ {
  pipe_mgr_spec_map_t spec_map;
} pipe_mgr_exm_pipe_hlp_ha_info_t;

typedef struct pipe_mgr_exm_pipe_llp_ha_info_ {
  /* Entry handle allocator */
  bf_id_allocator *ent_hdl_allocator;
} pipe_mgr_exm_pipe_llp_ha_info_t;

typedef struct pipe_mgr_exm_stage_moves {
  uint32_t moves[CUCKOO_MAX_NUM_MOVES + 1];
} pipe_mgr_exm_stage_moves_t;
/*Struct to hold the cuckoo move stats each entry hdl*/
typedef struct pipe_mgr_exm_ent_mov_stats {
  // Captures per stage entry adds happen in X moves
  pipe_mgr_exm_stage_moves_t *stage_stats;
  // failed per stage
  uint32_t *failed;
  uint32_t total_failed;
} pipe_mgr_exm_ent_mov_stats_t;

typedef struct pipe_mgr_exm_tbl_data_ {
  /* Number of stages in which this instance of the table exists */
  uint32_t num_stages;
  /* Number of entries in this instance of the table */
  uint32_t num_entries;
  /* Lowest pipe-id to which this instance of the table belongs to.
   * BF_DEV_PIPE_ALL if it belongs to all pipes in the profile.  */
  bf_dev_pipe_t pipe_id;
  /* Bit map of all the pipes in which this instance of the table is
   * present */
  pipe_bitmap_t pipe_bmp;
  /* Num entries placed by the HLP */
  uint32_t num_entries_placed;
  /* Num entries programmed to the hardware by LLP */
  uint32_t num_entries_occupied;
  /* Pointer to an array of stage level info */
  pipe_mgr_exm_stage_info_t *exm_stage_info;
  /* An array of pointers for quick access to stage info */
  pipe_mgr_exm_stage_info_t **stage_info_ptrs;
  /* A flag indicating if the default entry has been placed */
  bool default_entry_placed;
  /* A flag indicating if the default entry is installed or not */
  bool default_entry_installed;
  /* A flag indicating if the default entry has been backed up */
  bool default_entry_backed_up;
  /* Actual default entry handle */
  pipe_mat_ent_hdl_t default_entry_hdl;
  /* Backup of the above entry handle */
  pipe_mat_ent_hdl_t backup_default_entry_hdl;
  /* A map from entry index to entry handle */
  bf_map_t ent_idx_to_ent_hdl_htbl;
  bf_map_t dirtied_ent_idx_htbl;
  /* Entry handle management */
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr;
  /* HA related info stored at HLP */
  pipe_mgr_exm_pipe_hlp_ha_info_t *ha_hlp_info;
  /* HA related info stored at LLP */
  pipe_mgr_exm_pipe_llp_ha_info_t *ha_llp_info;
  /* HA reconciliation report (for debug purposes) */
  pipe_tbl_ha_reconc_report_t ha_reconc_report;
  /* Hash action tables will cache the default entry's action spec at the LLP to
   * be used in entry deletes. */
  pipe_action_spec_t *hash_action_dflt_act_spec;
  /* API flags of the API in progress on the table */
  uint32_t api_flags;
  /* A map from entry handle to match spec for proxy hash */
  bf_map_t proxy_hash_llp_hdl_to_mspec;
  /* A map from entry handle to entry info */
  bf_map_t entry_info_htbl;
  /* A map from entry handle to physical info of the entry */
  bf_map_t entry_phy_info_htbl;
  bf_map_t dirtied_ent_hdl_htbl;
  pipe_mgr_exm_ent_mov_stats_t entry_stats;
} pipe_mgr_exm_tbl_data_t;

typedef struct pipe_mgr_exm_tbl_ {
  char *name;
  pipe_tbl_dir_t direction;
  /* Table handle of this exact match table */
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  /* Pointer to match table info structure */
  pipe_mat_tbl_info_t *mat_tbl_info;
  /* Device id to which this table belongs to */
  bf_dev_id_t dev_id;
  /* A flag indicating if this exm table is a hash-action
   * table.
   */
  bool hash_action;
  /* A flag indicating if this exm table is a proxy-hash table */
  bool proxy_hash;
  /* Device info ptr */
  rmt_dev_info_t *dev_info;
  /* A flag indicating if this table is symmetric or asymmetric */
  bool symmetric;
  /* Scope info */
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  /* Total number of entries in this exact match table */
  uint32_t num_entries;
  /* Number of instances of the table which have to be managed.
   * As many as the number of pipes for a table which is asymmetrically
   * populated, or just ONE for a table which is symmetrically populated
   * across all pipes.
   */
  uint32_t num_tbls;
  /* Pointer to an array of exact match table data. As many as num_tbls
   */
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  /* Match-key width */
  uint16_t match_key_width;
  /* Number of references to action data tables */
  uint32_t num_adt_refs;
  /* List of references to action data tables */
  pipe_tbl_ref_t *adt_tbl_refs;
  /* Number of references to selction tables */
  uint32_t num_sel_tbl_refs;
  /* List of refernces to selection tables */
  pipe_tbl_ref_t *sel_tbl_refs;
  /* Number of references to statistics tables */
  uint32_t num_stat_tbl_refs;
  /* List of references to statistic tables */
  pipe_tbl_ref_t *stat_tbl_refs;
  /* Number of references to meter tables */
  uint32_t num_meter_tbl_refs;
  /* List of references to meter tables */
  pipe_tbl_ref_t *meter_tbl_refs;
  /* Number of references to stateful tables */
  uint32_t num_stful_tbl_refs;
  /* List of references to Stateful tables */
  pipe_tbl_ref_t *stful_tbl_refs;
  /* Profile ID */
  profile_id_t profile_id;
  /* A flag indicating if a idle timeout table is associated with this
   * table.
   */
  bool idle_present;
  /* Lock type needed to manage idle-time and stat table associated */
  pipe_mgr_lock_id_type_e lock_type;
  /* Idle table lock id allocator */
  lock_id_t idle_lock_id_allocator;
  /* The idle_init value to be used when ttl is zero */
  uint32_t idle_init_val_for_ttl_0;
  /* The lowest pipe-id of all the pipes in which the table is present.
   * Used when the table is symmetric.
   */
  bf_dev_pipe_t lowest_pipe_id;
  /* A flag indicating whether a default entry has been reserved */
  bool default_entry_reserved;
  /* A flag indicating whether a stash entry is available */
  bool stash_available;
  /* Number of match spec bytes for this table */
  uint32_t num_match_spec_bytes;
  /* Number of match spec bits for this table */
  uint32_t num_match_spec_bits;
  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;
  /* Maximum size of action data spec */
  uint32_t max_act_data_size;
  /* cJSON node for the current entry during state restore */
  cJSON *restore_ent_node;



} pipe_mgr_exm_tbl_t;

typedef struct pipe_mgr_action_stat_op_entry_modify_ {
  pipe_stat_ent_idx_t old_stat_idx;
  pipe_stat_ent_idx_t new_stat_idx;
} pipe_mgr_action_stat_op_entry_modify_t;

typedef enum pipe_mgr_action_stats_op_type_ {
  ACT_SPEC_DELTA_STATS_ADD,
  ACT_SPEC_DELTA_STATS_MODIFY,
  ACT_SPEC_DELTA_STATS_DELETE
} pipe_mgr_action_stats_op_type_e;

typedef enum pipe_mgr_action_meters_op_type_ {
  ACT_SPEC_DELTA_METERS_ADD,
  ACT_SPEC_DELTA_METERS_MODIFY,
  ACT_SPEC_DELTA_METERS_DELETE
} pipe_mgr_action_meters_op_type_e;

typedef enum pipe_mgr_action_stful_op_type_ {
  ACT_SPEC_DELTA_STFUL_ADD,
  ACT_SPEC_DELTA_STFUL_MODIFY,
  ACT_SPEC_DELTA_STFUL_DELETE
} pipe_mgr_action_stful_op_type_e;

typedef struct pipe_mgr_action_stats_op_entry_add_ {
  pipe_stat_ent_idx_t stat_idx;
} pipe_mgr_action_stats_op_entry_add_t;

typedef struct pipe_mgr_action_stats_op_entry_modify_ {
  pipe_stat_ent_idx_t old_stat_idx;
  pipe_stat_ent_idx_t new_stat_idx;
} pipe_mgr_action_stats_op_entry_modify_t;

/* Structure definition for stats op within the action spec delta */
typedef struct pipe_mgr_action_stats_op_ {
  pipe_mgr_action_stats_op_type_e type;
  pipe_tbl_ref_type_t ref_type;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  union {
    pipe_mgr_action_stats_op_entry_add_t entry_add;
    pipe_mgr_action_stats_op_entry_modify_t entry_modify;
  } u;
  pipe_res_spec_t stat_spec;
} pipe_mgr_action_stats_op_t;

typedef struct pipe_mgr_action_meters_op_entry_add_ {
  /* Reference type to meters. If indirect, then the meter_spec
   * is not valid, else the meter_spec needs to be programmed
   */
  pipe_meter_spec_t *meter_spec;
  pipe_meter_idx_t meter_idx;
} pipe_mgr_action_meters_op_entry_add_t;

typedef struct pipe_mgr_action_meters_op_entry_modify_ {
  pipe_meter_spec_t *meter_spec;
  pipe_meter_idx_t old_meter_idx;
  pipe_meter_idx_t new_meter_idx;
} pipe_mgr_action_meters_op_entry_modify_t;

typedef struct pipe_mgr_action_stful_op_entry_add_ {
  pipe_stful_mem_spec_t *stful_spec;
  pipe_stful_mem_idx_t stful_idx;
} pipe_mgr_action_stful_op_entry_add_t;

typedef struct pipe_mgr_action_stful_op_entry_modify_ {
  pipe_stful_mem_spec_t *stful_spec;
  pipe_stful_mem_idx_t old_stful_idx;
  pipe_stful_mem_idx_t new_stful_idx;
} pipe_mgr_action_stful_op_entry_modify_t;

/* Structure definition for meters op within the action spec delta */
typedef struct pipe_mgr_action_meters_op_ {
  pipe_mgr_action_meters_op_type_e type;
  pipe_tbl_ref_type_t ref_type;
  pipe_meter_tbl_hdl_t meter_tbl_hdl;
  union {
    pipe_mgr_action_meters_op_entry_add_t entry_add;
    pipe_mgr_action_meters_op_entry_modify_t entry_modify;
  } u;
  pipe_res_spec_t meter_res;
} pipe_mgr_action_meters_op_t;

/* Structure definition for stful op within the action spec delta */
typedef struct pipe_mgr_action_stful_op_ {
  pipe_mgr_action_stful_op_type_e type;
  pipe_tbl_ref_type_t ref_type;
  pipe_stful_tbl_hdl_t stful_tbl_hdl;
  union {
    pipe_mgr_action_stful_op_entry_add_t entry_add;
    pipe_mgr_action_stful_op_entry_modify_t entry_modify;
  } u;
  pipe_res_spec_t stful_res;
} pipe_mgr_action_stful_op_t;

/* Structure definition for action spec delta */
typedef struct pipe_mgr_act_spec_delta_ {
  bool stats_operate;
  bool meters_operate;
  bool stful_operate;
  pipe_mgr_action_stats_op_t stats_op;
  pipe_mgr_action_meters_op_t meters_op;
  pipe_mgr_action_stful_op_t stful_op;
} pipe_mgr_act_spec_delta_t;

/* Structure definition for proxy hash record node */
typedef struct pipe_mgr_exm_proxy_hash_record_node_ {
  uint64_t proxy_hash;
} pipe_mgr_exm_proxy_hash_record_node_t;

/* Structure definition for exm entry info */
typedef struct pipe_mgr_exm_entry_info_ {
  /* Entry handle */
  pipe_mat_ent_hdl_t mat_ent_hdl;
  /* Pipe id in which the entry is present. BF_DEV_PIPE_ALL if its present in
   * all pipes
   */
  bf_dev_pipe_t pipe_id;
  /* Stage id in which the entry is present */
  dev_stage_t stage_id;
  /* Logical entry index within the stage where the entry is present */
  pipe_mat_ent_idx_t entry_idx;
  /* Logical action entry index in case of indirectly referenced action entry */
  pipe_idx_t logical_action_idx;
  /* Action entry handle to which this match entry points to. Applicable only
   * for indirect addressed action table (if any) from the match table.
   */
  pipe_adt_ent_hdl_t adt_ent_hdl;
  /* A flag to indicate the validity of the action entry handle */
  bool adt_ent_hdl_valid;
  /* Logical index to the selector table in case of this entry pointing to a
   * selector group */
  pipe_idx_t logical_sel_idx;
  /* Length of the selector group */
  uint32_t selector_len;
  /* Pointer to match entry data consisting of match spec, action spec etc */
  struct pipe_mgr_mat_data *entry_data;
} pipe_mgr_exm_entry_info_t;

typedef enum pipe_mgr_exm_operation_ {
  PIPE_MGR_EXM_OPERATION_ADD,
  PIPE_MGR_EXM_OPERATION_MODIFY,
  PIPE_MGR_EXM_OPERATION_MOVE,
  PIPE_MGR_EXM_OPERATION_DELETE
} pipe_mgr_exm_operation_e;

/* Structure definition for txn node */
typedef struct pipe_mgr_exm_txn_node_ {
  /* Entry handle */
  /* The operation that was done on this node */
  pipe_mgr_exm_operation_e operation;
  pipe_mgr_exm_entry_info_t *entry_info;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
} pipe_mgr_exm_txn_node_t;

typedef struct pipe_mgr_exm_idx_txn_node_ {
  pipe_mgr_exm_operation_e operation;
  pipe_mat_ent_hdl_t entry_hdl;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
} pipe_mgr_exm_idx_txn_node_t;

/********************************************************************
 * Function prototypes
 ********************************************************************/

pipe_mgr_exm_tbl_t *pipe_mgr_exm_tbl_get(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_exm_shadow_copy_init(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t pipe_mat_tbl_hdl,
                                            uint32_t num_entries);

uint8_t pipe_mgr_exm_get_stage_for_new_entry(pipe_mgr_exm_tbl_t *exm_tbl,
                                             bf_dev_pipe_t pipe_id);

pipe_mat_ent_hdl_t pipe_mgr_exm_get_ent_hdl_from_ent_idx(
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info);

cuckoo_move_graph_t *pipe_mgr_exm_get_cuckoo_graph(pipe_mgr_exm_tbl_t *exm_tbl,
                                                   bf_dev_pipe_t pipe_id,
                                                   uint8_t stage_id);

pipe_mgr_exm_stage_info_t *pipe_mgr_exm_tbl_get_stage_info(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id, uint8_t stage_id);

pipe_mat_ent_hdl_t pipe_mgr_exm_allocate_new_ent_hdl(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data);

void pipe_mgr_exm_set_ent_hdl(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                              pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_update_entry_loc(pipe_mgr_exm_tbl_t *exm_tbl,
                                            pipe_mat_ent_idx_t entry_idx,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            bf_dev_pipe_t pipe_id,
                                            uint8_t stage_id,
                                            bool isTxn,
                                            bool add,
                                            bool new_entry_idx);

pipe_status_t pipe_mgr_exm_add_entry_loc(pipe_mgr_exm_tbl_t *exm_tbl,
                                         pipe_mat_ent_idx_t entry_idx,
                                         pipe_mat_ent_hdl_t mat_ent_hdl,
                                         bf_dev_pipe_t pipe_id,
                                         uint8_t stage_id,
                                         bool isTxn,
                                         bool new_entry_idx);

pipe_status_t pipe_mgr_exm_entry_data_backup(pipe_mgr_exm_tbl_t *exm_tbl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_entry_idx_to_ent_hdl_backup(
    pipe_mat_ent_hdl_t **backup, pipe_mat_ent_hdl_t entry_hdl);

void pipe_mgr_exm_deep_copy_match_spec(pipe_tbl_match_spec_t *dst_spec,
                                       pipe_tbl_match_spec_t *src_spec);

void pipe_mgr_exm_deep_copy_action_spec(pipe_action_spec_t *dst_spec,
                                        pipe_action_spec_t *src_spec);

void pipe_mgr_exm_get_mem_ids_for_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    mem_id_t *wide_word_mem_id,
    bool *ram_words_updated,
    uint32_t *num_mem_ids);

uint8_t pipe_mgr_exm_get_entry_hashway(
    pipe_mgr_exm_stage_info_t *exm_stage_info, pipe_mat_ent_idx_t entry_idx);

pipe_status_t pipe_mgr_exm_compute_ram_shadow_copy_for_entry_delete(
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint8_t **shadow_ptr_arr,
    uint8_t *mem_id,
    uint8_t *wide_word_idx,
    pipe_mgr_exm_tbl_t *exm_tbl);

pipe_status_t pipe_mgr_exm_compute_ram_shadow_copy_for_entry_activate(
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint8_t **shadow_ptr_arr,
    uint8_t *mem_id,
    uint8_t *wide_word_idx,
    pipe_mgr_exm_tbl_t *exm_tbl);

pipe_status_t pipe_mgr_exm_compute_ram_shadow_copy_for_entry_deactivate(
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint8_t **shadow_ptr_arr,
    uint8_t *mem_id,
    uint8_t *wide_word_idx,
    pipe_mgr_exm_tbl_t *exm_tbl);

void pipe_mgr_exm_deallocate_entry_hdl(pipe_mgr_exm_tbl_t *exm_tbl,
                                       pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                       pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_delete_entry_loc_info(pipe_mgr_exm_tbl_t *exm_tbl,
                                                 bf_dev_pipe_t pipe_id,
                                                 uint8_t stage_id,
                                                 pipe_mat_ent_hdl_t mat_ent_hdl,
                                                 pipe_mat_ent_idx_t entry_idx,
                                                 bool isTxn);

pipe_mat_ent_idx_t pipe_mgr_exm_get_default_miss_ent_idx(
    pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_adt_ent_hdl_t pipe_mgr_exm_get_def_ent_action_hdl(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id);

bool pipe_mgr_exm_tbl_is_ent_hdl_valid(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                       pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_tbl_alloc_lock_id(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_lock_id_type_e lock_id_type,
    lock_id_t *lock_id_p);

bool pipe_mgr_exm_ent_validate_action_spec(pipe_action_spec_t *act_spec);

bool pipe_mgr_exm_entry_exists(pipe_mgr_exm_tbl_t *exm_tbl,
                               bf_dev_pipe_t pipe_id,
                               uint8_t stage_id,
                               pipe_mgr_exm_edge_container_t *edge_container,
                               pipe_tbl_match_spec_t *match_spec);

uint32_t pipe_mgr_exm_compute_ram_line_num(
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    pipe_mat_ent_idx_t entry_idx);

bool pipe_mgr_exm_proxy_hash_entry_exists(pipe_mgr_exm_tbl_t *exm_tbl,
                                          bf_dev_pipe_t pipe_id,
                                          uint8_t stage_id,
                                          uint64_t proxy_hash);

pipe_tbl_ref_t *pipe_mgr_exm_get_tbl_ref(pipe_mgr_exm_tbl_t *exm_tbl,
                                         pipe_tbl_hdl_t tbl_hdl);

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id);

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance_from_any_pipe(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id);

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance_from_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    int entry_hdl,
    const char *where,
    const int line);

pipe_status_t pipe_mgr_exm_tbl_mgr_cleanup_default_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint32_t pipe_api_flags);

pipe_mgr_exm_phy_entry_info_t *pipe_mgr_exm_get_phy_entry_info(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_mgr_exm_entry_info_t *pipe_mgr_exm_get_entry_info(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_mgr_exm_edge_container_t *pipe_mgr_exm_expand_to_logical_entries(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_exm_hash_t *hash_container,
    uint32_t num_hashes,
    uint32_t *ent_subword_loc);

bool pipe_mgr_exm_is_ent_hdl_default(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                     pipe_mat_ent_hdl_t mat_ent_hdl);

bool pipe_mgr_exm_is_default_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data);

void pipe_mgr_exm_set_def_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data);

void pipe_mgr_exm_reset_def_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data);

void pipe_mgr_exm_compute_entry_details_from_location(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_hash_way_data_t *exm_hashway_data,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_hash_info_for_decode_t *hash,
    bool **ram_unit_present,
    uint8_t *num_ram_units);

pipe_mat_ent_idx_t pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx);

pipe_status_t pipe_mgr_exm_update_state_for_new_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    cuckoo_move_list_t *move_list_node,
    pipe_mgr_move_list_t *pipe_move_list_node,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    bool populate_move_list_node);

pipe_status_t pipe_mgr_exm_update_llp_state(
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    mem_id_t *mem_id_arr,
    uint32_t num_mem_ids);

uint8_t pipe_mgr_exm_get_stage_id_from_idx(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data, pipe_mat_ent_idx_t entry_idx);

pipe_status_t pipe_mgr_exm_execute_entry_add(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_move_list_t *move_list_node,
    bool end_of_move_add,
    bool is_stash,
    bool is_recovery);

pipe_status_t pipe_mgr_hash_action_add_default_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_idx_t logical_idx,
    bool is_recovery);

pipe_status_t pipe_mgr_exm_transform_cuckoo_move_list(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    cuckoo_move_graph_t *cuckoo_move_graph,
    cuckoo_move_list_t *move_list,
    bool *edge_updated,
    uint32_t ent_subword_loc);

pipe_status_t pipe_mgr_exm_update_cuckoo_edge_state(
    pipe_mgr_exm_stage_info_t *exm_stage_info, cuckoo_move_list_t *move_list);

pipe_status_t pipe_mgr_exm_hash_action_decode_entry(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_exm_decode_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_hash_info_for_decode_t *hash_info,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    uint8_t entry_position,
    uint8_t **wide_word_ptrs,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mat_ent_idx_t stage_ent_idx,
    bool *valid,
    uint64_t *proxy_hash);

pipe_status_t pipe_mgr_exm_execute_def_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_action_spec_t *action_spec);

pipe_status_t pipe_mgr_exm_decode_act_spec_for_entry(
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mat_ent_idx_t entry_idx,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    bool from_hw,
    pipe_sess_hdl_t *sess_hdl);

pipe_status_t pipe_mgr_exm_build_indirect_resources_from_hw(
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs);

static inline bool pipe_mgr_exm_is_tbl_full(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  /* The policy for declaring an exm tbl full is as follows
   * 1. For a table with direct addressed action table, an entry is reserved
   *    from the match table for default entry. Hence, the max possible
   *    occupancy of the table is 1 less than the declared size of the table.
   *
   * 2. For a table with indirect addressed action table, no entry is reserved
   *    from the match table for default entry. Hence, the max possible
   *    occupancy of the table is the declared size of the table.
   *
   * 3. For hash-action tables, the default entry is handled differently since
   *    there is no match resources allocated and the max possible occupancy
   *    is the declared size of the table.
   */
  uint32_t num_rsvd_entries = 0;

  if (exm_tbl->default_entry_reserved) {
    num_rsvd_entries += 1;
  }
  if (exm_tbl_data->num_entries_placed >=
      (exm_tbl_data->num_entries - num_rsvd_entries)) {
    return true;
  }

  return false;
}

static inline bool pipe_mgr_exm_reserve_stash_entry(
    pipe_mat_tbl_info_t *mat_tbl_info) {
  if (!mat_tbl_info->disable_atomic_modify && mat_tbl_info->num_adt_tbl_refs) {
    /* If action is in indirect no need to use stash for atomic modify */
    if (mat_tbl_info->adt_tbl_ref[0].ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
      return true;
    }
  }
  return false;
}

bool pipe_mgr_entry_idx_is_stash(pipe_mgr_exm_stage_info_t *exm_stage_info,
                                 pipe_mat_ent_idx_t stage_ent_idx);

pipe_status_t pipe_mgr_exm_tbl_free_entry_idx_get(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t *free_entry_idx);

pipe_mat_ent_idx_t pipe_mgr_default_stash_id_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info);

pipe_status_t pipe_mgr_stash_info_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t stash_id,
    pipe_mgr_exm_stash_entry_info_t **stash_ent_info);

pipe_status_t pipe_mgr_stash_info_at_wide_word_index_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t stash_id,
    uint32_t mem_index,
    pipe_mgr_exm_stash_entry_info_t **stash_ent_info);

#define PIPE_MGR_EXM_UNUSED_ENTRY_IDX 0xaabbccdd
#define PIPE_MGR_EXM_UNUSED_STASH_ENTRY_IDX 0xaabbccde

#define PIPE_MGR_EXM_STAGE_INFO(__exm_tbl__, __pipe_id__, __stage_id__) \
  do {                                                                  \
    pipe_mgr_exm_tbl_data_t *_exm_tbl_data = NULL;                      \
    uint32_t _pipe_idx = 0;                                             \
    uint32_t _stage_idx = 0;                                            \
                                                                        \
    if (_pipe_id == BF_DEV_PIPE_ALL) {                                  \
      exm_tbl_data = &exm_tbl->exm_tbl_data[0];                         \
    } else {                                                            \
      exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_id];                   \
    }                                                                   \
                                                                        \
    _stage_info_ = exm_tbl_data->stage_info_ptrs[stage_id];             \
    while (0)

#endif /* _PIPE_MGR_EXM_TBL_MGR_INT_H */
