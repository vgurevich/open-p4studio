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
 * @file pipe_mgr_select_tbl.h
 * @date
 *
 *
 * Contains definitions relating to pipe mgr's selector table management
 */

/*
 *
 * pipe_mgr_sel_grp_mbr_enable_disable_in_stage
 *   - Handles activate/deactivate
 *   - Set/Clr bit(s) for a member
 *   - Does not transition the duplicated state
 *
 * pipe_mgr_sel_grp_mbr_add_to_stage
 *   - Decides to duplicate/not duplicate/transition on add
 *   pipe_mgr_sel_grp_spread_entries
 *     - Transitions from duplicate to !duplicate
 *   pipe_mgr_sel_grp_mbr_add_to_stage_word
 *     - Handles add mbr
 *     - Sets a bit in a stage
 *
 * pipe_mgr_sel_grp_mbr_del_from_stage
 *   pipe_mgr_sel_grp_mbr_del_and_converge
 *     - Remove one mbr
 *     - Transition from entries spread over words to mirrored over words
 *   pipe_mgr_sel_grp_mbr_del_from_stage_word
 *     - Remove one mbr
 *     - Optionally swaps in another mbr
 *
 *
 * delete_from_hw_locator
 * move_in_hw_locator
 */

#ifndef _PIPE_MGR_SELECT_TBL_H
#define _PIPE_MGR_SELECT_TBL_H

/* Module header includes */
#include <target-utils/uCli/ucli.h>
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <target-utils/power2_allocator/power2_allocator.h>
#include <target-utils/third-party/cJSON/cJSON.h>

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_act_tbl.h"

#define SEL_GROUP_ACTIVATE_ALL_STAGES

#define SEL_GRP_WORD_WIDTH 128
#define SEL_VEC_WIDTH 120

#define PIPE_SEL_GRP_HDL_BASE (PIPE_SET_HDL_TYPE(0, PIPE_HDL_TYPE_SEL_TBL))
#define PIPE_INVALID_SEL_GRP_HDL -1
#define PIPE_INVALID_SEL_GRP_IDX -1
#define PIPE_INVALID_SEL_GRP_MBR -1

#define GET_MBR_DATA_FOR_IDX(word_data, index) \
  (((word_data->data[(index) >> 3]) >> ((index)&0x7)) & 0x1)

#define SET_MBR_DATA_FOR_IDX(word_data, index, val)             \
  {                                                             \
    PIPE_MGR_DBGCHK(16 > ((index) >> 3) + 1);                   \
    if (!val) {                                                 \
      (word_data->data[(index) >> 3] &= ~(1 << ((index)&0x7))); \
    } else {                                                    \
      (word_data->data[(index) >> 3] |= (1 << ((index)&0x7)));  \
    }                                                           \
  }

#define SET_PVL_USAGE(word_data, usage) ((word_data)->data[16 - 1] = (usage));

typedef pipe_adt_ent_hdl_t pipe_sel_grp_mbr_hdl_t;

struct sel_tbl_info_s;
struct sel_tbl_stage_info_s;
struct sel_grp_stage_info_s;

typedef struct sel_mbr_pos_s {
  uint32_t pos;
  struct sel_mbr_pos_s *next;
} sel_mbr_pos_t;

typedef struct sel_grp_mbr_s {
  bool is_backup_valid : 1;
  enum pipe_mgr_grp_mbr_state_e state;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  uint32_t weight;          // weight of mbr_hdl in the selector group
  sel_mbr_pos_t *mbrs_pos;  // list of positions occupied by the mbr_hdl
} sel_grp_mbr_t;

typedef struct sel_mbr_offset_s {
  uint32_t offset;
  struct sel_mbr_offset_s *next;
} sel_mbr_offset_t;

typedef struct sel_grp_mat_backptr_s {
  struct sel_grp_mat_backptr_s *prev;
  struct sel_grp_mat_backptr_s *next;
  pipe_mat_tbl_hdl_t mat_tbl;
  /* List of entries using sel grp */
  bf_map_t ent_hdls;
} sel_grp_mat_backptr_t;

typedef struct sel_grp_info_s {
  bool is_grp_backup : 1;
  bool is_backup_valid : 1;
  bool act_fn_set : 1;
  bool is_refcount_backup : 1;
  bool backup_from_resize : 1;
  bf_dev_pipe_t pipe_id;  // Indicates in which pipe the group
  // should be created.
  pipe_sel_grp_hdl_t grp_hdl;
  uint32_t max_grp_size;
  uint32_t adt_offset;

  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t mbr_count;
  uint32_t num_references;
  uint32_t num_active_mbrs;
  uint32_t entries_per_word;
  pipe_sel_grp_id_t grp_id;

  /* Lookup array that contains info about all the places where
   * this group exists
   * It is Organized in 2 levels.
   * - The first level is indexed by pipe_idx
   * - The second level is indexed by the stage_idx
   * The value of the second level provides the sel_grp_stage_info_t *
   */
  Pvoid_t sel_grp_pipe_lookup;

  /* Map of selection members
   *  Key                 - pipe_sel_grp_mbr_hdl_t
   *  Value               - sel_grp_mbr_t *
   */
  Pvoid_t sel_grp_mbrs;

  /* Data structures needed for backup */
  Pvoid_t backedup_mbrs;

  /* List of tables that uses this group */
  sel_grp_mat_backptr_t *mat_tbl_list;

} sel_grp_info_t;

/* Data structures to keep the hardware state */

typedef struct sel_hlp_word_data_s {
  bool is_backup_valid;
  pipe_idx_t adt_base_idx;
  uint32_t word_width;  // Maximum number of members that can
                        // be present in the word.
  uint32_t usage;       // Current usage of this word, this
                        // includes the duplicate entries and
                        // deactivated entries.
  pipe_sel_grp_mbr_hdl_t *mbrs;
  // Array of size ram_word_width, values
  // assigned when the word is assigned to a group
  // Out of this, only word_width are valid
  // When usage equals word_width, the word is full
} sel_hlp_word_data_t;

typedef struct sel_llp_word_data_s {
  pipe_idx_t adt_base_idx;
  uint32_t no_bits_set;  // Current bits that are set to  1.
                         // Used in non-resilient case for
                         // programming the length
  // Array of size ram_word_width
  pipe_sel_grp_mbr_hdl_t *mbrs;
  // Array of size ram_word_width
  uint8_t *data;
  // Array of size ram_word_width/8
  int32_t highest_mbr_idx;
} sel_llp_word_data_t;

typedef struct sel_grp_stage_info_s {
  struct sel_grp_stage_info_s *next;
  struct sel_grp_stage_info_s *prev;
  struct sel_tbl_stage_info_s *stage_p;  // Parent pointer to sel_tbl_stage_info
  bool isstatic;  // Indicates that the hw group was created
  // by a profile. Don't delete
  bool inuse;  // Indicates if the group is being
  // referenced by a real software table
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t no_words;  // Also the len value to program in the match  entry
  uint32_t entries_per_word;
  // uint32_t                   word_width; //Min of ram_wod_width or the
  // max_grp_size
  uint32_t grp_size;

  // Runtime data below
  bool mbrs_duplicated;
  uint32_t cur_usage;  // Current usage in this stage
  // excluding the duplicate entries
  uint32_t sel_base_idx;
  uint32_t adt_base_idx;
  rmt_virt_addr_t sel_base_addr;  // Needed for mat entry

  /* Array which gives the member offset given a member handle */
  Pvoid_t mbr_locator;
  sel_hlp_word_data_t *sel_grp_word_data;  // Pointer to the
  // sel_word array
  // with the right offset
} sel_grp_stage_info_t;

typedef struct sel_tbl_stage_hw_info_s {
  rmt_tbl_hdl_t tbl_id;  // Logical table id
  rmt_mem_pack_format_t pack_format;
  uint32_t data_sz;
  uint32_t num_blks;
  uint32_t block_width;
  uint8_t num_spare_rams;
  mem_id_t spare_rams[RMT_MAX_S2P_SECTIONS];
  rmt_tbl_word_blk_t *tbl_blk;  // array of size num_blks
} sel_tbl_stage_hw_info_t;

typedef struct sel_tbl_stage_info_s {
  uint8_t stage_id;
  uint8_t stage_idx;
  uint32_t ram_word_width;  // Width of the RAM  word
  uint32_t no_words;        // No of RAM words allocated
  sel_tbl_stage_hw_info_t pv_hw;

  // Runtime data below
  sel_grp_stage_info_t
      *sel_grp_stage_free_list;  // Free groups allocated to this stage
  sel_grp_stage_info_t
      *sel_grp_stage_inuse_list;  // Inuse groups allocated to this stage
  sel_grp_stage_info_t
      *sel_grp_stage_backup_list;  // Groups that have been backed up

  power2_allocator_t *stage_sel_grp_allocator;
  struct {
    sel_hlp_word_data_t *hlp_word;  // Array of size no_words
  } hlp;
  struct {
    sel_llp_word_data_t *llp_word;
  } llp;

  pipe_idx_t fallback_adt_index;
  uint8_t num_alu_ids;
  uint8_t alu_ids[RMT_MAX_S2P_SECTIONS];
} sel_tbl_stage_info_t;

struct sel_tbl_info_s;

typedef struct sel_tbl_s {
  struct sel_tbl_info_s *sel_tbl_info;  // Back pointer to sel_tbl_info
  bf_dev_pipe_t pipe_id;
  uint8_t pipe_idx;  // Index of this array element in the  array
  pipe_bitmap_t inst_pipe_bmp;
  uint8_t num_stages;
  bool is_profile_set;
  bf_map_t grp_id_map;
  pipe_sess_hdl_t cur_sess_hdl;
  uint32_t sess_flags;

  /* Selection Group handle to index lookup hash table
   *      Key     - pipe_sel_grp_hdl_t
   *      Value   - sel_grp_info_t *
   */
  Pvoid_t sel_grp_array;

  /* Parsed RMT data for usage later */

  // Runtime data below
  // uint32_t                sel_grp_usage;

  sel_tbl_stage_info_t *sel_tbl_stage_info;  // Array of size num_stages

  struct {
    bool is_backup_valid;
    pipe_adt_ent_hdl_t fallback_adt_ent_hdl;
    /* State required for HA at HLP */
    void *ha_hlp_info;
  } hlp;
  struct {
    uint32_t num_grps;
    pipe_adt_ent_hdl_t fallback_adt_ent_hdl;
    /* State required for HA at LLP */
    void *ha_llp_info;
  } llp;
} sel_tbl_t;

typedef struct sel_tbl_info_s {
  char *name;
  pipe_tbl_dir_t direction;
  bf_dev_id_t dev_id;
  rmt_dev_info_t *dev_info;
  pipe_sel_tbl_hdl_t tbl_hdl;
  uint32_t max_grp_size;
  uint32_t max_grps;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;
  pipe_stful_tbl_hdl_t stful_tbl_hdl;
  pipe_sel_tbl_mode_t mode;

  bool is_symmetric;
  bool in_restore;
  bool sps_enable;
  bool sequence_order;  // Flag to keep members in selector tbl in sequence
                        // order of add/modify
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;

  pipe_sess_hdl_t cur_sess_hdl;
  uint32_t sess_flags;
  /* Callback for tracking member updates. */
  pipe_mgr_sel_tbl_update_callback cb;
  void *cb_cookie;

  uint8_t no_sel_tbls;
  sel_tbl_t *sel_tbl;  // In case of asymmetric, array of size number of pipes
  profile_id_t profile_id;  // profile-id
  pipe_bitmap_t pipe_bmp;   // pipe bitmap
  /* Lowest logical pipe id in which the table is present.
   * Used only in the symmetric cases.
   */
  bf_dev_pipe_t lowest_pipe_id;
  /* State required for HA at LLP */
  void *ha_llp_info;
  /* Used to enable cache of group id mapping at selector table level.
   * Used by bfrt. Cannot be turned off once set on. */
  bool cache_id;
} sel_tbl_info_t;

#define SEL_TBL_IS_SYMMETRIC(x) (x->is_symmetric == true)
#define SEL_SESS_IS_ATOMIC(x) ((x->sess_flags & PIPE_MGR_TBL_API_ATOM) != 0)
#define SEL_SESS_IS_TXN(x) ((x->sess_flags & PIPE_MGR_TBL_API_TXN) != 0)
#define SEL_TBL_IS_RESILIENT(x) (x->mode == RESILIENT)
#define SEL_TBL_IS_FAIR(x) (x->mode == FAIR)

/* Exported routines */

pipe_status_t pipe_sel_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    struct pipe_mgr_sel_move_list_t *move_list,
    uint32_t *processed);

pipe_status_t pipe_mgr_sel_get_plcmt_data(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    struct pipe_mgr_sel_move_list_t **move_list);

/*!
 * API function to register a callback function to track updates to groups in
 * the selection table.
 */
pipe_status_t rmt_sel_tbl_set_cb(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 pipe_mgr_sel_tbl_update_callback cb,
                                 void *cb_cookie);

/*!
 * API function to set the group profile for a selection table
 */
pipe_status_t rmt_sel_tbl_profile_set(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                      pipe_sel_tbl_profile_t *sel_tbl_profile);

/*!
 * API function to add a new group into a selection table
 */
pipe_status_t rmt_sel_grp_add(pipe_sess_hdl_t sess_hdl,
                              dev_target_t dev_tgt,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              pipe_sel_grp_id_t grp_id,
                              uint32_t max_grp_size,
                              pipe_adt_ent_idx_t adt_offset,
                              pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                              uint32_t pipe_api_flags,
                              struct pipe_mgr_sel_move_list_t **move_head_p);

/*!
 * API function to delete a group from a selection table
 */
pipe_status_t rmt_sel_grp_del(pipe_sess_hdl_t sess_hdl,
                              bf_dev_id_t device_id,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              pipe_sel_grp_hdl_t sel_grp_hdl,
                              uint32_t pipe_api_flags,
                              struct pipe_mgr_sel_move_list_t **move_list);

/*!
 * API function to delete a group from a selection table
 */
pipe_status_t rmt_sel_grp_resize(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 pipe_sel_grp_hdl_t sel_grp_hdl,
                                 pipe_sel_grp_hdl_t *tmp_grp_hdl,
                                 uint32_t max_grp_size,
                                 uint32_t pipe_api_flags,
                                 struct pipe_mgr_sel_move_list_t **move_head_p);

pipe_status_t rmt_sel_grp_notify_mat(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl);

/*!
 * API function to add a member to a group of a selection table
 */
pipe_status_t rmt_sel_grp_mbr_add(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t device_id,
                                  pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                  pipe_sel_grp_hdl_t sel_grp_hdl,
                                  pipe_act_fn_hdl_t act_fn_hdl,
                                  pipe_adt_ent_hdl_t adt_ent_hdl,
                                  uint32_t pipe_api_flags,
                                  struct pipe_mgr_sel_move_list_t **move_list);

/*!
 * API function to delete a member from a group of a selection table
 */
pipe_status_t rmt_sel_grp_mbr_del(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t device_id,
                                  pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                  pipe_sel_grp_hdl_t sel_grp_hdl,
                                  pipe_adt_ent_hdl_t adt_ent_hdl,
                                  uint32_t pipe_api_flags,
                                  struct pipe_mgr_sel_move_list_t **move_list);

/*!
 * API function to set membership of a group
 */
pipe_status_t rmt_sel_grp_mbrs_set(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   pipe_sel_grp_hdl_t sel_grp_hdl,
                                   uint32_t num_mbrs,
                                   pipe_adt_ent_hdl_t *mbrs,
                                   bool *enable,
                                   uint32_t pipe_api_flags,
                                   struct pipe_mgr_sel_move_list_t **move_list);

/*!
 * API function to get membership of a group
 */
pipe_status_t rmt_sel_grp_mbrs_get(bf_dev_id_t device_id,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   pipe_sel_grp_hdl_t sel_grp_hdl,
                                   uint32_t mbrs_size,
                                   pipe_adt_ent_hdl_t *mbrs,
                                   bool *enable,
                                   uint32_t *mbrs_populated);

/* API function to disable a group member of a selection table */
pipe_status_t rmt_sel_grp_mbr_disable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_list);

/* API function to re-enable a group member of a selection table */
pipe_status_t rmt_sel_grp_mbr_enable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_list);

/* API function to get the current state of a selection member */
pipe_status_t rmt_sel_grp_mbr_state_get(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p);

/* API function to get the member handle given a hash value */
pipe_status_t rmt_sel_grp_mbr_get_from_hash(bf_dev_id_t dev_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            uint8_t *hash,
                                            uint32_t hash_len,
                                            pipe_adt_ent_hdl_t *adt_ent_hdl_p);

/* API function to set the fallback member */
pipe_status_t rmt_sel_fallback_mbr_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_list);

/* API function to reset the fallback member */
pipe_status_t rmt_sel_fallback_mbr_reset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_list);

/* Activate a selection table group in a stage */
pipe_status_t rmt_sel_grp_activate_stage(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t pipe_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint8_t stage_id,
    rmt_virt_addr_t *grp_act_data_set_base_p,
    uint32_t *grp_port_vector_set_len_p,
    pipe_idx_t *grp_port_vector_set_base_p,
    uint32_t pipe_api_flags);

/* De-activate a selection table group in a stage */
pipe_status_t rmt_sel_grp_deactivate_stage(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           bf_dev_pipe_t pipe_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           uint8_t stage_id,
                                           uint32_t pipe_api_flags);

/* Get the action data pointer for a particular group */
pipe_action_data_spec_t *rmt_sel_grp_adt_data_get(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl);

pipe_status_t pipe_mgr_sel_grp_word_data_allocate(sel_hlp_word_data_t *sel_word,
                                                  uint32_t word_width,
                                                  uint32_t ram_word_width);

void pipe_mgr_sel_grp_word_data_deallocate(sel_hlp_word_data_t *sel_word,
                                           uint32_t ram_word_width);

pipe_stful_tbl_hdl_t pipe_mgr_sel_tbl_get_stful_ref(bf_dev_id_t dev_id,
                                                    pipe_sel_tbl_hdl_t tbl_hdl);

/* Hash Calculation APIs */
pipe_status_t pipe_mgr_hash_calc_init_defaults(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_hash_calc_input_default_set_ext(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_hash_calc_hdl_t hdl);

pipe_status_t pipe_mgr_hash_calc_input_set_ext(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t handle,
                                               pipe_fld_lst_hdl_t fl_handle);

pipe_status_t pipe_mgr_hash_calc_input_get_ext(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t handle,
                                               pipe_fld_lst_hdl_t *fl_handle);

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_set_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list);

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t max_attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list,
    uint32_t *num_attr_filled);

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_2_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    pipe_hash_calc_input_field_attribute_t **attr_list,
    uint32_t *num_attr_filled);

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_count_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t *attr_count);

pipe_status_t pipe_mgr_hash_calc_algorithm_set_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_hash_alg_hdl_t al_handle,
    const bfn_hash_algorithm_t *algorithm,
    uint64_t rotate);

pipe_status_t pipe_mgr_hash_calc_algorithm_get_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_hash_alg_hdl_t *al_handle,
    bfn_hash_algorithm_t *algorithm,
    uint64_t *rotate);

pipe_status_t pipe_mgr_hash_calc_algorithm_reset_ext(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     pipe_hash_calc_hdl_t hdl);

pipe_status_t pipe_mgr_hash_calc_seed_set_ext(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_hash_calc_hdl_t handle,
                                              pipe_hash_seed_t seed);

pipe_status_t pipe_mgr_hash_calc_seed_get_ext(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_hash_calc_hdl_t handle,
                                              pipe_hash_seed_t *seed);

pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint8_t *stream,
    uint32_t stream_len,
    uint8_t *hash,
    uint32_t hash_len);

pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_with_cfg_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    uint32_t hash_len,
    uint8_t *hash);

/** \brief pipe_mgr_sel_tbl_info_get
 *        Get the sel tbl pointer for a given table handle
 *
 * This function returns the sel tbl pointer stored in the sel_tbl_htbl
 * or it's backup table
 *
 * \param dev_tgt Device target
 * \param tbl_hdl sel table handle
 * \param is_backup Flag to indicate if a backup table/main table is needed
 * \return sel_tbl_t* Pointer to the sel_tbl structure
 */
sel_tbl_info_t *pipe_mgr_sel_tbl_info_get(bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t tbl_hdl,
                                          bool is_backup);

sel_tbl_info_t *pipe_mgr_sel_tbl_info_get_first(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t *tbl_hdl_p);

sel_tbl_info_t *pipe_mgr_sel_tbl_info_get_next(bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t *tbl_hdl_p);

sel_grp_info_t *pipe_mgr_sel_grp_get(sel_tbl_t *sel_tbl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl);

sel_grp_info_t *pipe_mgr_sel_grp_allocate(void);
void pipe_mgr_sel_grp_destroy(sel_grp_info_t *grp);

pipe_status_t pipe_mgr_sel_grp_add_to_htbl(sel_tbl_t *sel_tbl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           sel_grp_info_t *sel_grp_info);

pipe_status_t pipe_mgr_sel_grp_remove_from_htbl(sel_tbl_t *sel_tbl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl);

sel_grp_mbr_t *pipe_mgr_sel_grp_mbr_alloc(pipe_sel_grp_mbr_hdl_t mbr_hdl);

sel_grp_mbr_t *pipe_mgr_sel_grp_mbr_get(sel_grp_info_t *sel_grp_info,
                                        pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
                                        bool is_backup);

pipe_status_t pipe_mgr_sel_grp_mbr_remove_and_destroy(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    bool is_backup);

pipe_status_t pipe_mgr_sel_mbr_add_to_htbl(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    sel_grp_mbr_t *grp_mbr,
    bool is_backup);

void pipe_mgr_sel_grp_stage_list_free_all(sel_grp_stage_info_t *grp_stage_list);

pipe_status_t pipe_mgr_sel_grp_update_grp_hw_locator_list(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    sel_grp_stage_info_t *sel_grp_stage_info);

sel_grp_stage_info_t *pipe_mgr_sel_grp_stage_info_get(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_tbl_stage_info_t *sel_tbl_stage_info);

pipe_status_t pipe_mgr_sel_grp_mbr_hw_locator_update(
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t mbr_idx);

pipe_status_t pipe_mgr_sel_grp_add_internal(
    sel_tbl_t *sel_tbl,
    bf_dev_pipe_t pipe_id,
    pipe_sel_grp_id_t grp_id,
    uint32_t max_grp_size,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_idx_t adt_offset,
    struct pipe_mgr_sel_move_list_t **move_head_p);

pipe_status_t pipe_mgr_sel_grp_mbr_add_internal(
    sel_tbl_info_t *sel_tbl_info,
    sel_grp_info_t *sel_grp_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t grp_mbr_weight,
    uint32_t num_add_mbrs,
    struct pipe_mgr_sel_move_list_t **move_tail_p,
    uint32_t *rollback_weight,
    bool skip_backup);

pipe_status_t pipe_mgr_sel_grp_del_internal(
    sel_tbl_t *sel_tbl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    bool backup_restore_mode,
    bool skip_cache_id,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

pipe_status_t pipe_mgr_sel_grp_allocate_in_stage(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    uint32_t grp_size,
    int sel_base_idx,
    pipe_adt_ent_idx_t adt_base_idx,
    sel_grp_stage_info_t **sel_grp_stage_info_p);

pipe_status_t pipe_mgr_sel_grp_mbr_add_to_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t num_mbrs,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

pipe_status_t mbr_del_from_stage(sel_tbl_t *sel_tbl,
                                 sel_grp_stage_info_t *sel_grp_stage_info,
                                 sel_grp_info_t *sel_grp_info,
                                 pipe_sel_grp_mbr_hdl_t mbr_hdl,
                                 void *cookie,
                                 struct pipe_mgr_sel_move_list_t **move_tail_p);

uint32_t pipe_mgr_sel_tbl_get_stage_idx(sel_tbl_t *sel_tbl, uint32_t stage_id);

/** \brief pipe_mgr_sel_tbl_create
 *         Create the local structures to hold data about a new sel tbl
 *
 * This routine creates the local data structures needed for sel tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the sel tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_sel_tbl_create(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl,
                                      profile_id_t profile_id,
                                      pipe_bitmap_t *pipe_bmp);

/** \brief pipe_mgr_sel_tbl_delete
 *         Deletes the local structures that hold data about a sel tbl
 *
 * This routine deletes the local data structures needed for sel tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the sel tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_sel_tbl_delete(bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl);

/** \brief pipe_mgr_sel_abort
 *        Abort a session for the given table handle
 *
 * This function should be called during abort to restore the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl sel table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_sel_abort(bf_dev_id_t dev_id,
                                 pipe_sel_tbl_hdl_t tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes);

/** \brief pipe_mgr_sel_commit
 *        Commit the state associated with a session
 *
 * This function should be called during commit to discard the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl sel table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_sel_commit(bf_dev_id_t dev_id,
                                  pipe_sel_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes);

pipe_status_t pipe_mgr_sel_tbl_assert(bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl);

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1
ucli_node_t *pipe_mgr_sel_tbl_ucli_node_create(ucli_node_t *n);
#else
void *pipe_mgr_sel_tbl_ucli_node_create(void);
#endif

pipe_status_t pipe_mgr_sel_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_sel_tbl_set_symmetric_mode(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  bool symmetric,
                                                  scope_num_t num_scopes,
                                                  scope_pipes_t *scope_pipe_bmp,
                                                  bool is_backup);
pipe_status_t pipe_mgr_sel_get_first_entry_handle(pipe_sel_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  int *entry_hdl);
pipe_status_t pipe_mgr_sel_get_next_entry_handles(pipe_sel_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  pipe_sel_grp_hdl_t entry_hdl,
                                                  int n,
                                                  int *next_entry_handles);
pipe_status_t pipe_sel_get_first_group_member(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t *mbr_hdl_p);

pipe_status_t pipe_sel_get_next_group_members(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    int n,
    pipe_sel_grp_mbr_hdl_t *mbr_hdl_p);

pipe_status_t pipe_sel_get_word_llp_active_member_count(
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t *count);

pipe_status_t pipe_sel_get_word_llp_active_members(
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t count,
    pipe_adt_ent_hdl_t *mbr_hdls);

pipe_status_t pipe_mgr_sel_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_mat_ent_hdl_t entry_hdl,
                                     pipe_act_fn_hdl_t *act_fn_hdl,
                                     bool from_hw);

pipe_status_t pipe_mgr_sel_tbl_get_placed_entry_count(dev_target_t dev_tgt,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p);

pipe_status_t pipe_mgr_sel_tbl_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_tbl_hdl_t tbl_hdl, uint32_t *count_p);

pipe_status_t pipe_mgr_sel_get_group_member_count(bf_dev_id_t dev_id,
                                                  pipe_sel_tbl_hdl_t tbl_hdl,
                                                  pipe_sel_grp_hdl_t grp_hdl,
                                                  uint32_t *count_p);

pipe_status_t pipe_mgr_sel_logical_idx_to_vaddr(bf_dev_id_t dev_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                int stage,
                                                uint32_t logical_idx,
                                                rmt_virt_addr_t *vaddr);

pipe_status_t pipe_mgr_sel_vaddr_to_logical_idx(bf_dev_id_t dev_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                int stage,
                                                rmt_virt_addr_t vaddr,
                                                uint32_t *logical_idx);

void pipe_mgr_sel_tbl_get_phy_addr(rmt_mem_pack_format_t pack_format,
                                   uint32_t stage_logical_idx,
                                   uint32_t *subword_no,
                                   uint32_t *line_no,
                                   uint32_t *block,
                                   uint32_t ram_depth);

pipe_status_t pipe_mgr_sel_shadow_db_init(bf_dev_id_t dev);

void pipe_mgr_sel_shadow_db_cleanup(bf_dev_id_t dev);

pipe_status_t pipe_mgr_sel_log_state(bf_dev_id_t dev_id,
                                     pipe_sel_tbl_hdl_t tbl_hdl,
                                     cJSON *sel_tbls);

pipe_status_t pipe_mgr_sel_restore_state(bf_dev_id_t dev_id, cJSON *sel_tbl);

static inline sel_tbl_t *get_sel_tbl_by_pipe_id(sel_tbl_info_t *sel_tbl_info,
                                                bf_dev_pipe_t pipe_id) {
  uint32_t i;
  for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl_t *sel_tbl = &sel_tbl_info->sel_tbl[i];
    if (sel_tbl->pipe_id == pipe_id) {
      return sel_tbl;
    }
  }
  return NULL;
}

pipe_status_t pipe_mgr_sel_sbe_correct(bf_dev_id_t dev_id,
                                       bf_dev_pipe_t log_pipe_id,
                                       dev_stage_t stage_id,
                                       pipe_sel_tbl_hdl_t tbl_hdl,
                                       int line);
void pipe_mgr_sel_grp_mbr_set_state(sel_grp_info_t *sel_grp_info,
                                    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
                                    bool enable);

pipe_status_t pipe_mgr_sel_update_mat_refs(sel_grp_info_t *sel_grp_info,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl);

/* Replace HA member handle with a new member handle.
 * Both have the same action data and function handle.
 */
pipe_status_t pipe_mgr_mbr_update_identical_in_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    uint32_t mbr_word,
    uint32_t mbr_idx,
    pipe_adt_ent_hdl_t ha_mbr_hdl,
    pipe_adt_ent_hdl_t new_mbr_hdl,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

pipe_status_t pipe_mgr_sel_grp_get_params(bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl,
                                          uint32_t *max_size,
                                          uint32_t *adt_offset);

pipe_status_t pipe_mgr_sel_get_grp_id_hdl_int(bf_dev_target_t dev_tgt,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              pipe_sel_grp_id_t grp_id,
                                              pipe_sel_grp_hdl_t grp_hdl,
                                              pipe_sel_grp_hdl_t *sel_grp_hdl,
                                              pipe_sel_grp_id_t *sel_grp_id);

pipe_status_t pipe_mgr_sel_update_active_mbr_count(
    sel_grp_info_t *sel_grp_info);

uint32_t pipe_mgr_sel_group_count(sel_tbl_info_t *sel_tbl_info,
                                  bf_dev_pipe_t pipe_id);

void sel_stage_get_word_info(uint32_t grp_size,
                             uint32_t *no_words,
                             uint32_t *entries_per_word);

pipe_status_t pipe_mgr_selector_tbl_get_sequence_order(bool *enable);

pipe_status_t pipe_mgr_selector_tbl_set_sequence_order(bool enable);

#endif /* _PIPE_MGR_SELECT_TBL_H */
