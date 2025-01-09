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
 * @file pipe_mgr_tcam.h
 * @date
 *
 * TCAM related definitions of pipeline manager
 */

#ifndef _PIPE_MGR_TCAM_H
#define _PIPE_MGR_TCAM_H

#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
/* Global header files */
#include <target-utils/uCli/ucli.h>

#include <target-utils/fbitset/fbitset.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Module header files */
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_hitless_ha.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define PIPE_TCAM_DEF_ENT_HDL 0x1
#define PIPE_TCAM_INVALID_ENT_HDL 0xffffffff

/* Total number of tcam entries in a block */
#define PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK 512

/* Buffer space to be allocated while allocating entries */
#define PIPE_MGR_TCAM_ENTRY_BUFFER_SPACE 5

#define PIPE_MGR_TCAM_INVALID_IDX 0xFFFFFFFF
#define PIPE_MGR_TCAM_INVALID_ADDR 0xFFFFFFFF
#define PIPE_INVALID_STAGE_IDX 0xFF

#define TCAM_MAX_GROUPS 256
#define TCAM_GROUP_ID_SHIFT 24

#define LAST_TCAM_ENTRY_OF_BLOCK(x) \
  (((x + 1) * PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK) - 1)

#define FIRST_TCAM_ENTRY_OF_BLOCK(x) ((x)*PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK)

#define FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_BEGIN(_head_entry_, _tentry_)          \
  {                                                                           \
    tcam_hlp_entry_t *_tnext_ = NULL;                                         \
    tcam_hlp_entry_t *_tcam_entry_ = NULL;                                    \
    for (_tcam_entry_ = _head_entry_; _tcam_entry_; _tcam_entry_ = _tnext_) { \
      _tnext_ = _tcam_entry_->next;                                           \
      tcam_hlp_entry_t *_rnext_ = NULL;                                       \
      TCAM_HLP_GET_RANGE_HEAD(_tcam_entry_, _tcam_entry_);                    \
      for (_tentry_ = _tcam_entry_; _tentry_; _tentry_ = _rnext_) {           \
        _rnext_ = _tentry_->next_range;

#define FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_END() \
  }                                          \
  }                                          \
  }

#define FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(_head_entry_,         \
                                                        _tentry_)             \
  {                                                                           \
    tcam_hlp_entry_t *_tnext_ = NULL;                                         \
    tcam_hlp_entry_t *_tcam_entry_ = NULL;                                    \
    for (_tcam_entry_ = _head_entry_; _tcam_entry_; _tcam_entry_ = _tnext_) { \
      _tnext_ = _tcam_entry_->next;                                           \
      TCAM_HLP_GET_RANGE_HEAD(_tcam_entry_, _tentry_);

#define FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END() \
  }                                                     \
  }

#define FOR_ALL_TCAM_HLP_RANGE_ENTRIES_BLOCK_BEGIN(               \
    _head_entry_, _tentry_, _index_)                              \
  {                                                               \
    tcam_hlp_entry_t *_rnext_ = NULL;                             \
    for (_tentry_ = _head_entry_; _tentry_; _tentry_ = _rnext_) { \
      _rnext_ = _tentry_->next_range;                             \
      _index_ = _tentry_->index;

#define FOR_ALL_TCAM_HLP_RANGE_ENTRIES_BLOCK_END() \
  }                                                \
  }

#define TCAM_HLP_GET_RANGE_HEAD(_tentry_, _head_) \
  (_head_ = _tentry_ ? *(_tentry_->range_list_p) : _tentry_)

#define TCAM_HLP_IS_RANGE_HEAD(tentry) (tentry == *(tentry->range_list_p))
#define TCAM_HLP_GET_RANGE_TAIL_INDEX(_head_, _tail_index_)                    \
  {                                                                            \
    tcam_hlp_entry_t *_tail_;                                                  \
    BF_LIST_DLL_LAST(*(_head_->range_list_p), _tail_, next_range, prev_range); \
    if (_tail_) _tail_index_ = _tail_->index;                                  \
  }

#define TCAM_HLP_GET_RANGE_ENTRY_COUNT(_head_, _count_) \
  { _count_ = _head_->range_count; }

#define TCAM_HLP_GET_TCAM_HEAD(_tentry_, _head_)                   \
  {                                                                \
    _head_ = _tentry_ ? *(_tentry_->range_list_p) : _tentry_;      \
    while (_head_ && _head_->prev && _head_->prev->next != NULL) { \
      _head_ = _head_->prev;                                       \
    }                                                              \
  }

typedef enum {
  PIPE_TCAM_OP_INVALID = 0,
  PIPE_TCAM_OP_ALLOCATE,
  PIPE_TCAM_OP_MOVE,
  PIPE_TCAM_OP_LOGICAL_MOVE,
  PIPE_TCAM_OP_DELETE,
  PIPE_TCAM_OP_UPDATE_VERSION,
  PIPE_TCAM_OP_UPDATE_HEAD_POINTER,
  PIPE_TCAM_OP_REFRESH_TIND,
  PIPE_TCAM_OP_MODIFY,
  PIPE_TCAM_OP_RESERVE_DEFAULT,
  PIPE_TCAM_OP_SET_DEFAULT,
  PIPE_TCAM_OP_CLEAR_DEFAULT
} pipe_tcam_op_e;

typedef struct tcam_phy_loc_info_s {
  uint32_t index;  // The logical TCAM index
  uint32_t stage_id;
  uint32_t stage_idx;      // Index into stage_data[]
  uint32_t stage_line_no;  // Line no in the stage (match address)
  uint32_t block_id;       // Index into the block_data[]
  uint32_t phy_line_no;    // Line no within the TCAM/SRAM
  uint32_t subword;        // If there's packing, the subword
} tcam_phy_loc_info_t;

typedef struct tcam_indirect_addr_s {
  uint32_t indirect_addr_action;
  uint32_t indirect_addr_sel;
  uint16_t sel_grp_pvl;
  uint32_t indirect_addr_stats;
  uint32_t indirect_addr_meter;
  uint32_t indirect_addr_stful;
  uint32_t indirect_addr_idle;
} tcam_indirect_addr_t;

typedef struct tcam_hlp_entry_s {
  struct tcam_hlp_entry_s *next;
  struct tcam_hlp_entry_s *prev;
  struct tcam_hlp_entry_s *next_range;
  struct tcam_hlp_entry_s *prev_range;
  struct tcam_hlp_entry_s *next_atomic;
  struct tcam_hlp_entry_s *prev_atomic;
  struct tcam_hlp_entry_s *range_list;
  struct tcam_hlp_entry_s **range_list_p;
  struct pipe_mgr_mat_data *mat_data;
  pipe_mat_ent_hdl_t entry_hdl;
  bool is_backup_valid : 1;
  bool is_default : 1;
  uint16_t group;
  uint32_t ptn_index;
  uint32_t priority;
  uint32_t index;
  uint32_t range_count;  // Number of contiguous entries in each block
  uint32_t subentry_index;

  pipe_idx_t logical_action_idx;
  pipe_idx_t logical_sel_idx;
  uint32_t selector_len;
} tcam_hlp_entry_t;

typedef struct tcam_llp_entry_s {
  struct tcam_llp_entry_s *next;
  struct tcam_llp_entry_s *prev;
  struct tcam_llp_entry_s *next_range;
  struct tcam_llp_entry_s *prev_range;
  struct tcam_llp_entry_s *range_list;
  struct tcam_llp_entry_s **range_list_p;
  pipe_mat_ent_hdl_t entry_hdl;
  bool is_default : 1;
  bf_dev_pipe_t pipe_id;
  uint32_t ptn_index;
  uint32_t index;
  uint32_t range_count;
  uint32_t subentry_index;

  /* The handles are cached to correctly deactivate */
  pipe_adt_ent_hdl_t adt_ent_hdl;
  pipe_sel_grp_hdl_t sel_grp_hdl;

  /* Info returned from the various tbl managers */
  tcam_indirect_addr_t addr;
  pipe_idx_t logical_action_idx;
  pipe_idx_t logical_sel_idx;

  /* The current verison of the stateful spec.  Can be used to compare against
   * the version in the mat_ent_data when deciding whether or not to write a
   * new stateful spec.  Only applies when there is a direct stateful table. */
  uint32_t direct_stful_seq_nu;
} tcam_llp_entry_t;

typedef struct tcam_stage_info_s {
  uint8_t stage_id;
  rmt_tbl_hdl_t stage_table_handle;
  /* Index of the stage logical table within the stage.
   * Used when multiple logical tables exist for the table in one stage
   */
  uint8_t stage_table_idx;
  uint32_t tcam_start_index;
  uint32_t block_start_index;
  uint32_t no_tcam_entries;
  uint32_t mem_depth;
  bool is_stat_locked;
  bool is_idle_locked;

  uint32_t movereg_src_addr;
  uint32_t movereg_dest_addr;

  // TCAM pack format
  rmt_mem_pack_format_t pack_format;

  rmt_mem_pack_format_t tind_pack_format;
  uint32_t tind_num_blks;
  rmt_tbl_word_blk_t *tind_blk;  // array of size num_blks

  /* Ternary Indirection entries are basically the shadow of what's in
   * hardware. So the total_tind_lines is equal to the
   * number of physical entries (ram lines) occupied by the table in this stage
   * This is slightly different from the total_entries for TCAM, where
   * logical entries are considered
   */
  uint32_t total_tind_lines;
  pipe_act_fn_hdl_t tind_act_fn_hdl;
} tcam_stage_info_t;

typedef struct tcam_block_data_s {
  // uint8_t             stage_id;
  uint8_t stage_index;
  rmt_tbl_word_blk_t word_blk;
} tcam_block_data_t;

typedef struct tcam_prio_range_s {
  uint32_t start;
  uint32_t end;
} tcam_prio_range_t;

typedef struct tcam_group_info_s {
  /* These are not backed up as such. Instead restored from
   * the already backed up tcam entries
   */
  // Judy array whose key is priority and value is tcam_prio_range_t
  Pvoid_t jtcam_prio_array;
  Pvoid_t jtcam_index_bmp;
} tcam_group_info_t;

typedef enum {
  TCAM_MOVE_INVALID = 0,
  TCAM_MOVE_UP,
  TCAM_MOVE_DOWN
} tcam_move_type_e;

typedef struct tcam_prev_move_s {
  tcam_move_type_e prev_move_type;
  uint32_t src_index;
  uint32_t dest_index;
} tcam_prev_move_t;

typedef enum tcam_default_ent_type_e_ {
  TCAM_DEFAULT_ENT_TYPE_INDIRECT = 1,
  TCAM_DEFAULT_ENT_TYPE_DIRECT
} tcam_default_ent_type_e;
static inline const char *tcam_default_ent_type_str(tcam_default_ent_type_e e) {
  switch (e) {
    case TCAM_DEFAULT_ENT_TYPE_INDIRECT:
      return "Indirect";
    case TCAM_DEFAULT_ENT_TYPE_DIRECT:
      return "Direct";
  }
  return "Unknown";
}

struct tcam_tbl_info_s;
struct tcam_pipe_tbl_s;

typedef struct tcam_tbl_s {
  struct tcam_pipe_tbl_s *tcam_pipe_tbl_p;
  uint32_t ptn_index;
  uint32_t total_entries;

  struct {
    // Runtime data below
    uint32_t total_usage;
    uint32_t max_tcam_group;
    /* Array sized by max_tcam_group (above), limited to TCAM_MAX_GROUPS. */
    tcam_group_info_t **group_info;

    bf_fbitset_t tcam_used_bmp;
    /* Only item that needs backup. Rest all are derived from backup */
    tcam_hlp_entry_t **tcam_entries;
    /* Array of pointers to tcam entries, upto max_entries in tbl map.
     * Elements are arranged according to their logical tcam index
     */

    /* The below 2 lists are used for handling the Transaction, atomic
     * case where we need to update the newly added entries
     * to clear the version bits in matching
     * and actually delete the delete entries
     */
    tcam_hlp_entry_t *entry_add_list; /* List of all the new entry adds */
    tcam_hlp_entry_t *entry_del_list; /* List of all the new entry deletes */
  } hlp;

  struct {
    uint32_t total_usage;
    uint32_t total_hw_usage;
    tcam_prev_move_t prev_move_cookie;
    tcam_llp_entry_t **tcam_entries;
  } llp;

} tcam_tbl_t;

typedef struct tcam_pipe_tbl_s {
  struct tcam_tbl_info_s *tcam_tbl_info_p;  // Back pointer to tcam_tbl_info
  uint32_t pipe_idx;
  bf_dev_pipe_t pipe_id;
  pipe_bitmap_t pipe_bmp;
  uint8_t num_stages;
  uint32_t num_blocks;
  uint32_t blocks_per_bank;
  pipe_sess_hdl_t cur_sess_hdl;
  uint32_t sess_flags;

  tcam_stage_info_t *stage_data;  // upto num_stages
  tcam_block_data_t *block_data;  // upto num_blocks

  bool default_backup_valid;
  tcam_default_ent_type_e default_ent_type;
  struct {
    bf_id_allocator *ent_hdl_allocator;
    bool default_ent_set;
    pipe_mat_ent_hdl_t default_ent_hdl;
    tcam_hlp_entry_t *hlp_default_tcam_entry;

    /* Tcam entries hash tbl for faster lookup for update/delete
     *       Key - pipe_mat_ent_hdl_t
     *       Value - pointer to tcam_entry
     */
    bf_map_t tcam_entry_db;  // Not backed up. Derived from backup
  } hlp;
  struct {
    bool default_ent_set;
    pipe_mat_ent_hdl_t default_ent_hdl;
    tcam_llp_entry_t *llp_default_tcam_entry;
    /* Tcam entries hash tbl for faster lookup for update/delete
     *       Key - pipe_mat_ent_hdl_t
     *       Value - pointer to tcam_entry
     */
    bf_map_t tcam_entry_db;  // Not backed up. Derived from backup
    lock_id_t cur_lock_id;
    bool terminate_op;
    /* Entry hdl allocator at LLP to be used during HW state
     * restore during Hitless HA.
     */
    bf_id_allocator *ha_ent_hdl_allocator;
  } llp;

  uint32_t no_ptns;
  tcam_tbl_t *tcam_ptn_tbls;  // Array of size no_ptns

  pipe_mgr_spec_map_t spec_map;
  pipe_tbl_ha_reconc_report_t ha_reconc_report;
} tcam_pipe_tbl_t;

typedef struct tcam_tbl_info_s {
  char *name;
  pipe_tbl_dir_t direction;
  bf_dev_id_t dev_id;
  rmt_dev_info_t *dev_info;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_match_type_t match_type;
  bool uses_range;
  bool uses_alpm;
  bool adt_present;
  bool sel_present;
  bool idle_present;
  pipe_tbl_ref_t adt_tbl_ref; /* Referece to action data table */
  pipe_tbl_ref_t sel_tbl_ref; /* Reference to selection table */
  uint32_t num_tbl_refs;
  pipe_tbl_ref_t *tbl_refs;          /* Reference to other tbls */
  pipe_mgr_lock_id_type_e lock_type; /* Indicates the type of lock that
                                      * is needed for this table
                                      */
  /* Number of match spec bytes for this table */
  uint32_t num_match_spec_bytes;
  /* Number of match spec bits for this table */
  uint32_t num_match_spec_bits;
  /* Maximum size of action data spec */
  uint32_t max_act_data_size;
  /* Maximum size of table */
  uint32_t tbl_size_max;
  /* Maximum size of table specified in P4 */
  uint32_t tbl_size_in_p4;

  bool is_symmetric;
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  bool reserve_modify_ent;
  cJSON *restore_ent_node;

  uint8_t no_tcam_pipe_tbls;
  tcam_pipe_tbl_t *tcam_pipe_tbl;  // In case of asymmetric, array of size
  // number of pipes
  profile_id_t profile_id;

  struct {
    // LLP Data structures
    lock_id_t lock_id_generator;
    /* Lowest logical pipe-id, which will be used for symmetric tables */
    bf_dev_pipe_t lowest_pipe_id;
  } llp;

  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;

  /* Global table lock */
  pipe_mgr_mutex_t tbl_lock;
} tcam_tbl_info_t;

#define TCAM_TBL_IS_SYMMETRIC(x) ((x)->is_symmetric == true)
#define TCAM_TBL_HAS_PROFILE(x) ((x)->profile_id != 0)
#define TCAM_TBL_USES_RANGE(x) ((x)->uses_range == true)
#define TCAM_TBL_IS_ATCAM(x) ((x)->match_type == ATCAM_MATCH)
#define TCAM_SESS_IS_ATOMIC(x) (((x)->sess_flags & PIPE_MGR_TBL_API_ATOM) != 0)
#define TCAM_SESS_IS_TXN(x) (((x)->sess_flags & PIPE_MGR_TBL_API_TXN) != 0)

void pipe_mgr_tcam_print_match_spec(bf_dev_id_t device_id,
                                    pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                    pipe_tbl_match_spec_t *match_spec,
                                    char *buf,
                                    size_t buf_len);

/*
 ******************************************************
 *        APIs to be used by session manager          *
 ******************************************************
 */

/** \brief pipe_mgr_tcam_tbl_create
 *         Create the local structures to hold data about a new tcam tbl
 *
 * This routine creates the local data structures needed for tcam tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the tcam tbl
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_tcam_tbl_create(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp);

/** \brief pipe_mgr_tcam_tbl_delete
 *         Deletes the local structures that hold data about a tcam tbl
 *
 * This routine deletes the local data structures needed for tcam tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the tcam tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_tcam_tbl_delete(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl);

/** \brief pipe_mgr_tcam_entry_add
 *        Adds an entry to a tcam tbl
 *
 * This function finds a suitable position for the tcam entry according to the
 * priority. If an entry has to be displaced to make space for the new entry,
 * it'll move the entries and associated data
 *
 * \param sess_hdl Session hdl
 * \param dev_tgt Device target
 * \param mat_tbl_hdl Table hdl of the match tbl
 */
pipe_status_t pipe_mgr_tcam_entry_place(dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *match_spec,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *act_data_spec,
                                        uint32_t ttl,
                                        uint32_t pipe_api_flags,
                                        pipe_mat_ent_hdl_t *ent_hdl_p,
                                        pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_entry_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl_p,
    pipe_mgr_move_list_t **head_p);

/** \brief pipe_mgr_tcam_entry_del
 *        Delets an entry from a tcam tbl
 *
 * This function deletes an entry from the tcam tbl. It also calls other
 * APIs to delete the associated action entries, meters, stats etc
 *
 * \param sess_hdl Session hdl
 * \param dev_id Device id
 * \param mat_tbl_hdl TCAM table handle
 * \param mat_ent_hdl tcam entry hdl to delete
 * \param pipe_api_flags
 * \return pipe_status_t  The status of the operation
 */
pipe_status_t pipe_mgr_tcam_entry_del(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      uint32_t pipe_api_flags,
                                      pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_get_match_spec(pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bf_dev_id_t device_id,
                                           bf_dev_pipe_t *pipe_id,
                                           pipe_tbl_match_spec_t **match_spec);

pipe_status_t pipe_mgr_tcam_default_ent_place(dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_act_fn_hdl_t act_fn_hdl,
                                              pipe_action_spec_t *act_spec,
                                              uint32_t pipe_api_flags,
                                              pipe_mat_ent_hdl_t *ent_hdl_p,
                                              pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_default_ent_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **head_p,
    bool use_move_node);

pipe_status_t pipe_mgr_tcam_default_ent_hdl_get(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t *ent_hdl_p);

pipe_status_t pipe_mgr_tcam_default_ent_get(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_action_spec_t *action_spec,
                                            pipe_act_fn_hdl_t *act_fn_hdl,
                                            bool from_hw);

pipe_status_t pipe_mgr_tcam_ent_set_action(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_act_fn_hdl_t act_fn_hdl,
                                           pipe_action_spec_t *act_spec,
                                           uint32_t pipe_api_flags,
                                           pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_ent_set_resource(bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_res_spec_t *resources,
                                             int resource_count,
                                             uint32_t pipe_api_flags,
                                             pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_entry_update_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p);

pipe_status_t pipe_mgr_tcam_cleanup_default_entry(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **head_p);

pipe_status_t pipe_mgr_tcam_process_move_list(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mgr_move_list_t *move_list,
                                              uint32_t *success_count);

/** \brief pipe_mgr_tcam_abort
 *        Abort a session for the given table handle
 *
 * This function should be called during abort to restore the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \param pipes_list points to the list of the relevant pipes
 * \param nb_pipes is the number of relevant pipes in pipes_list
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_abort(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes);

/** \brief pipe_mgr_tcam_commit
 *        Commit the state associated with a session
 *
 * This function should be called during commit to discard the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \param pipes_list points to the list of the relevant pipes
 * \param nb_pipes is the number of relevant pipes in pipes_list
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_commit(bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                   bf_dev_pipe_t *pipes_list,
                                   unsigned nb_pipes);

/** \brief pipe_mgr_tcam_atom_cleanup
 *        Cleanup the state  associated with atomic transaction
 *
 * This function will cleanup the tcam state for the atomic transaction
 * including removing version matches in the tcam entries and deleting the
 * tcam entries
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_atom_cleanup(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         bf_dev_pipe_t *pipes_list,
                                         unsigned nb_pipes);

/*
 ******************************************************
 *        APIs internal to tcam management            *
 ******************************************************
 */

/** \brief pipe_mgr_tcam_tbl_info_get
 *        Get the tcam tbl pointer for a given table handle
 *
 * This function returns the tcam tbl pointer stored in the tcam_tbl_htbl
 * or it's backup table
 *
 * \param dev_tgt Device target
 * \param tbl_hdl TCAM table handle
 * \param is_backup Flag to indicate if a backup table/main table is needed
 * \return tcam_tbl_t* Pointer to the tcam_tbl structure
 */
tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            bool is_backup);

tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get_first(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p);

tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get_next(bf_dev_id_t dev_id,
                                                 pipe_mat_tbl_hdl_t *tbl_hdl_p);

bool is_rmt_tcam(rmt_tbl_type_t type);

/** \brief pipe_mgr_tcam_entry_htbl_add_delete
 *        Add/delete a tcam_entry into the tcam_entries_htbl
 *
 * \param tcam_tbl Pointer to tcam table
 * \param ent_hdl TCAM entry handle
 * \param tcam_entry Pointer to the tcam entry
 * \param is_add true for add, false for delete
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_entry_htbl_add_delete(tcam_tbl_t *tcam_tbl,
                                                  pipe_mat_ent_hdl_t ent_hdl,
                                                  tcam_hlp_entry_t *tcam_entry,
                                                  bool is_add);

void pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                     pipe_mat_ent_hdl_t ent_hdl);

void pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                 pipe_mat_ent_hdl_t ent_hdl);

void pipe_mgr_tcam_entry_deep_copy(tcam_hlp_entry_t *dest,
                                   tcam_hlp_entry_t *src);

tcam_hlp_entry_t *pipe_mgr_tcam_entry_alloc(void);

void pipe_mgr_tcam_hlp_entry_destroy(tcam_hlp_entry_t *tcam_entry,
                                     bool free_data);

tcam_hlp_entry_t *pipe_mgr_tcam_entry_get(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                          pipe_mat_ent_hdl_t ent_hdl,
                                          uint32_t subindex);

pipe_status_t pipe_mgr_tcam_update_prio_array(tcam_tbl_t *tcam_tbl,
                                              uint16_t group,
                                              uint32_t priority,
                                              uint32_t tcam_index,
                                              bool is_add);

pipe_status_t pipe_mgr_tcam_tbl_set_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp,
    bool is_backup);
pipe_status_t pipe_mgr_tcam_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);
pipe_status_t pipe_mgr_tcam_get_first_placed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl);
pipe_status_t pipe_mgr_tcam_get_next_placed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles);

pipe_status_t pipe_mgr_tcam_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_ent_hdl_t entry_hdl,
                                      pipe_tbl_match_spec_t *pipe_match_spec,
                                      pipe_action_spec_t *pipe_action_spec,
                                      pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t tcam_get_default_entry_from_hw(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *tcam_loc,
    bf_dev_pipe_t pipe_id,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_tcam_get_entry_llp_from_hw(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl);

pipe_status_t pipe_mgr_tcam_get_plcmt_data(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    struct pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_tcam_mark_index_inuse(tcam_tbl_t *tcam_tbl,
                                             uint32_t index);

pipe_status_t pipe_mgr_tcam_mark_index_free(tcam_tbl_t *tcam_tbl,
                                            uint32_t index);

pipe_status_t pipe_mgr_tcam_entry_hdl_from_stage_idx(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe,
    pipe_mat_tbl_hdl_t tbl_hdl,
    int stage_id,
    int logical_tbl_id,
    uint32_t hit_addr,
    pipe_mat_ent_hdl_t *entry_hdl);

pipe_status_t pipe_mgr_tcam_log_state(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_info_t *mat_info,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      cJSON *match_tbls);

void pipe_mgr_restore_tcam_addr_node(tcam_tbl_info_t *tbl_info,
                                     tcam_llp_entry_t *entry);

pipe_status_t pipe_mgr_tcam_restore_state(bf_dev_id_t dev_id,
                                          pipe_mat_tbl_info_t *mat_info,
                                          cJSON *tcam_tbl,
                                          alpm_restore_callback_fn cb_fn);

/** \brief pipe_mgr_tcam_get_total_entry_count
 *        Get the total TCAM entry count
 *
 * This function is used for returning the total HW capacity
 * of a TCAM table.
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param count_p Output for parameter for count
 * \return Pipe-Mgr status
 */
pipe_status_t pipe_mgr_tcam_get_total_entry_count(dev_target_t dev_tgt,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  size_t *count_p);

/** \brief pipe_mgr_tcam_get_reserved_entry_count
 *        Get the total TCAM reserved entry count
 *
 * This function is used for returning the total reserved count
 * of a TCAM table.
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param count_p Output for parameter for count
 * \return Pipe-Mgr status
 */
pipe_status_t pipe_mgr_tcam_get_reserved_entry_count(dev_target_t dev_tgt,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     size_t *count_p);

/*
 ******************************************************
 *        APIs for sanity testing - unit test         *
 ******************************************************
 */

/** \brief pipe_mgr_tcam_assert
 *        Checks the validity of tcam entries and their priorities
 *
 * This function is used for unit-testing, to make sure that priority
 * is honored after entry moves etc
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \return bool Returns true if the priority handling is correct, false
 *         otherwise
 */
bool pipe_mgr_tcam_assert(bf_dev_id_t dev_id,
                          pipe_mat_tbl_hdl_t tbl_hdl,
                          bool debug);

/** \brief pipe_mgr_tcam_get_placed_entry_count
 *        Returns the number of valid entries
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param uint32_t* Pointer to the count of valid entries to fill
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_get_placed_entry_count(dev_target_t dev_tgt,
                                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                                   uint32_t *count_p);

static inline pipe_res_spec_t *get_resource_from_act_spec(
    pipe_action_spec_t *act_spec, pipe_res_hdl_t tbl_hdl) {
  int i;
  for (i = 0; i < act_spec->resource_count; i++) {
    pipe_res_spec_t *rs = &act_spec->resources[i];
    if (rs->tbl_hdl == tbl_hdl) {
      return rs;
    }
  }
  return NULL;
}

static inline tcam_tbl_info_t *get_tcam_tbl_info(tcam_tbl_t *tcam_tbl) {
  return tcam_tbl->tcam_pipe_tbl_p->tcam_tbl_info_p;
}

static inline tcam_pipe_tbl_t *get_tcam_pipe_tbl(tcam_tbl_t *tcam_tbl) {
  return tcam_tbl->tcam_pipe_tbl_p;
}

static inline tcam_pipe_tbl_t *get_tcam_pipe_tbl_by_pipe_id(
    tcam_tbl_info_t *tcam_tbl_info, bf_dev_pipe_t pipe_id) {
  uint32_t i;
  for (i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
    tcam_pipe_tbl_t *tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];
    if (tcam_pipe_tbl->pipe_id == pipe_id) {
      return tcam_pipe_tbl;
    }
  }
  return NULL;
}

static inline tcam_tbl_t *get_tcam_tbl(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                       uint32_t ptn_index) {
  if (ptn_index >= tcam_pipe_tbl->no_ptns) {
    return NULL;
  }
  return &tcam_pipe_tbl->tcam_ptn_tbls[ptn_index];
}

static inline tcam_stage_info_t *get_tcam_stage_data(
    tcam_tbl_t *tcam_tbl, tcam_phy_loc_info_t *tcam_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  if (tcam_loc->stage_idx >= tcam_pipe_tbl->num_stages) {
    return NULL;
  }
  return &tcam_pipe_tbl->stage_data[tcam_loc->stage_idx];
}

static inline tcam_stage_info_t *get_stage_data_for_block(
    tcam_pipe_tbl_t *tcam_pipe_tbl, tcam_block_data_t *block_data) {
  if (block_data->stage_index > tcam_pipe_tbl->num_stages) {
    return NULL;
  }
  return &tcam_pipe_tbl->stage_data[block_data->stage_index];
}

static inline tcam_block_data_t *get_tcam_block_data(
    tcam_tbl_t *tcam_tbl, tcam_phy_loc_info_t *tcam_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  if (tcam_loc->block_id >= tcam_pipe_tbl->num_blocks) {
    return NULL;
  }
  return &tcam_pipe_tbl->block_data[tcam_loc->block_id];
}

static inline bool is_entry_hdl_default(tcam_tbl_info_t *tbl_info,
                                        pipe_mat_ent_hdl_t ent_hdl,
                                        bool is_hlp) {
  int i = 0, I = tbl_info->no_tcam_pipe_tbls;
  for (; i < I; ++i) {
    if (is_hlp && tbl_info->tcam_pipe_tbl[i].hlp.default_ent_hdl == ent_hdl) {
      return true;
    }
    if (!is_hlp && tbl_info->tcam_pipe_tbl[i].llp.default_ent_hdl == ent_hdl) {
      return true;
    }
  }
  return false;
}

tcam_pipe_tbl_t *pipe_mgr_tcam_tbl_get_instance_from_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    int entry_hdl,
    const char *where,
    const int line);

pipe_status_t pipe_mgr_tcam_entry_add_internal(
    tcam_tbl_t *tcam_tbl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_tail_p,
    bool use_move_node);

pipe_status_t pipe_mgr_tcam_tbl_raw_entry_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              uint32_t tcam_index,
                                              bool err_correction,
                                              pipe_tbl_match_spec_t *match_spec,
                                              pipe_action_spec_t *act_spec,
                                              pipe_act_fn_hdl_t *act_fn_hdl,
                                              pipe_ent_hdl_t *entry_hdl,
                                              bool *is_default,
                                              uint32_t *next_index);

pipe_status_t pipe_mgr_tcam_get_last_index(dev_target_t dev_tgt,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           uint32_t *last_index);

pipe_status_t pipe_mgr_tcam_tbl_update(dev_target_t dev_tgt,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp,
                                       uint32_t pipe_api_flags,
                                       pipe_mgr_move_list_t **move_head_p);

#define FOR_ALL_PTNS_BEGIN(_pipe_tbl_, _tcam_tbl_)    \
  {                                                   \
    uint32_t _i_ = 0;                                 \
    for (_i_ = 0; _i_ < _pipe_tbl_->no_ptns; _i_++) { \
      _tcam_tbl_ = &_pipe_tbl_->tcam_ptn_tbls[_i_];

#define FOR_ALL_PTNS_END() \
  }                        \
  }

#define PIPE_MGR_TCAM_GET_ENTRY_PROP(                                  \
    _hlp_, _llp_, _index_, _is_default_, _entry_hdl_, _group_, _prio_) \
  {                                                                    \
    if (_hlp_) {                                                       \
      _index_ = _hlp_->index;                                          \
      _is_default_ = _hlp_->is_default;                                \
      _entry_hdl_ = _hlp_->entry_hdl;                                  \
      _group_ = _hlp_->group;                                          \
      _prio_ = _hlp_->priority;                                        \
    } else {                                                           \
      _index_ = _llp_->index;                                          \
      _is_default_ = _llp_->is_default;                                \
      _entry_hdl_ = _llp_->entry_hdl;                                  \
      _group_ = ~0;                                                    \
      _prio_ = ~0;                                                     \
    }                                                                  \
  }

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1
ucli_node_t *pipe_mgr_tcam_tbl_ucli_node_create(ucli_node_t *n);
#else
void *pipe_mgr_tcam_tbl_ucli_node_create(void);
#endif

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_TCAM_H */
