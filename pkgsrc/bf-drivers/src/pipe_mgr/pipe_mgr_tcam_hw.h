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
 * @file pipe_mgr_tcam_hw.h
 * @date
 *
 * TCAM related definitions of pipeline manager
 */

#ifndef _PIPE_MGR_TCAM_HW_H
#define _PIPE_MGR_TCAM_HW_H

#define FOR_ALL_TCAM_LLP_ENTRIES_BLOCK_BEGIN(_head_entry_, _tentry_)          \
  {                                                                           \
    tcam_llp_entry_t *_tnext_ = NULL;                                         \
    tcam_llp_entry_t *_tcam_entry_ = NULL;                                    \
    for (_tcam_entry_ = _head_entry_; _tcam_entry_; _tcam_entry_ = _tnext_) { \
      _tnext_ = _tcam_entry_->next;                                           \
      tcam_llp_entry_t *_rnext_ = NULL;                                       \
      TCAM_LLP_GET_RANGE_HEAD(_tcam_entry_, _tcam_entry_);                    \
      for (_tentry_ = _tcam_entry_; _tentry_; _tentry_ = _rnext_) {           \
        _rnext_ = _tentry_->next_range;

#define FOR_ALL_TCAM_LLP_ENTRIES_BLOCK_END() \
  }                                          \
  }                                          \
  }

#define FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(_head_entry_,         \
                                                        _tentry_)             \
  {                                                                           \
    tcam_llp_entry_t *_tnext_ = NULL;                                         \
    tcam_llp_entry_t *_tcam_entry_ = NULL;                                    \
    for (_tcam_entry_ = _head_entry_; _tcam_entry_; _tcam_entry_ = _tnext_) { \
      _tnext_ = _tcam_entry_->next;                                           \
      TCAM_LLP_GET_RANGE_HEAD(_tcam_entry_, _tentry_);

#define FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END() \
  }                                                     \
  }

#define FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_BEGIN(               \
    _head_entry_, _tentry_, _index_)                              \
  {                                                               \
    tcam_llp_entry_t *_rnext_ = NULL;                             \
    for (_tentry_ = _head_entry_; _tentry_; _tentry_ = _rnext_) { \
      _rnext_ = _tentry_->next_range;                             \
      _index_ = _tentry_->index;

#define FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_END() \
  }                                                \
  }

#define TCAM_LLP_GET_RANGE_HEAD(_tentry_, _head_) \
  (_head_ = _tentry_ ? *(_tentry_->range_list_p) : _tentry_)

#define TCAM_LLP_IS_RANGE_HEAD(tentry) (tentry == *(tentry->range_list_p))
#define TCAM_LLP_GET_RANGE_TAIL_INDEX(_head_, _tail_index_)                    \
  {                                                                            \
    tcam_llp_entry_t *_tail_;                                                  \
    BF_LIST_DLL_LAST(*(_head_->range_list_p), _tail_, next_range, prev_range); \
    if (_tail_) _tail_index_ = _tail_->index;                                  \
  }

#define TCAM_LLP_GET_RANGE_ENTRY_COUNT(_head_, _count_) \
  { _count_ = _head_->range_count; }

#define TCAM_LLP_GET_TCAM_HEAD(_tentry_, _head_)                   \
  {                                                                \
    _head_ = _tentry_ ? *(_tentry_->range_list_p) : _tentry_;      \
    while (_head_ && _head_->prev && _head_->prev->next != NULL) { \
      _head_ = _head_->prev;                                       \
    }                                                              \
  }

#define TCAM_LLP_GET_RANGE_ENTRY_COUNT(_head_, _count_) \
  { _count_ = _head_->range_count; }

void pipe_mgr_tcam_llp_entry_destroy(tcam_llp_entry_t *tcam_entry);

void pipe_mgr_tcam_get_instruction_addr(tcam_tbl_t *tcam_tbl,
                                        uint64_t *tcam_addr,
                                        uint64_t *tind_addr,
                                        uint32_t index);

pipe_status_t pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
    tcam_tbl_t *tcam_tbl,
    uint32_t index,
    bool is_default,
    tcam_phy_loc_info_t *tcam_loc);

pipe_status_t pipe_mgr_tcam_get_phy_loc_info(tcam_tbl_t *tcam_tbl,
                                             uint32_t index,
                                             tcam_phy_loc_info_t *tcam_phy_loc);

void pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t block,
    uint32_t physical_line_no,
    uint32_t subword,
    uint32_t *index_p,
    uint32_t *ptn_index_p);

pipe_status_t tcam_set_partition_idx_in_match_spec(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_tbl_match_spec_t *match_spec,
    uint32_t partition_idx);

pipe_status_t pipe_mgr_tcam_get_tcam_block(tcam_tbl_t *tcam_tbl,
                                           uint32_t index,
                                           uint32_t *block_p);

uint32_t pipe_mgr_get_stage_id_for_block(tcam_tbl_t *tcam_tbl, uint32_t block);

pipe_tbl_ref_t *pipe_mgr_tcam_get_tbl_ref_by_type(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_hdl_type_t hdl_type,
    pipe_tbl_ref_type_t ref_type);
bool pipe_mgr_tcam_has_direct_resource(tcam_tbl_info_t *tcam_tbl_info);

static inline uint32_t get_ptn_from_logical_index(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_idx_t logical_idx) {
  return logical_idx % tcam_pipe_tbl->no_ptns;
}

static inline uint32_t get_index_from_logical_index(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_idx_t logical_idx) {
  return logical_idx / tcam_pipe_tbl->no_ptns;
}

static inline pipe_idx_t get_logical_index_from_index_ptn(
    tcam_pipe_tbl_t *tcam_pipe_tbl, uint32_t ptn_index, uint32_t index) {
  return (index * tcam_pipe_tbl->no_ptns) + ptn_index;
}

tcam_llp_entry_t *pipe_mgr_tcam_llp_entry_get(tcam_tbl_info_t *tcam_tbl_info,
                                              pipe_mat_ent_hdl_t ent_hdl,
                                              uint32_t subindex);

/** \brief pipe_mgr_tcam_get_programmed_entry_count
 *        Returns the number of programmed entries
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param uint32_t* Pointer to the count of valid entries to fill
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, uint32_t *count_p);

pipe_status_t pipe_mgr_tcam_entry_get_programmed_location(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t subindex,
    bf_dev_pipe_t *pipe_id_p,
    uint8_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_hdl_p,
    uint32_t *index_p);

pipe_status_t pipe_mgr_tcam_gen_lock_id(bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mgr_lock_id_type_e lock_id_type,
                                        lock_id_t *lock_id_p);

pipe_status_t pipe_mgr_tcam_update_lock_type(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             bool idle,
                                             bool stat,
                                             bool add_lock);

pipe_status_t pipe_mgr_tcam_update_idle_init_val(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0);

pipe_status_t pipe_mgr_tcam_reset_idle(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       bf_dev_pipe_t pipe_id,
                                       uint8_t stage_id,
                                       mem_id_t mem_id,
                                       uint32_t mem_offset);

pipe_status_t pipe_mgr_tcam_get_first_programmed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl);

pipe_status_t pipe_mgr_tcam_get_next_programmed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles);

/* Used for HA */
pipe_status_t pipe_mgr_tcam_process_allocate(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    pipe_tbl_match_spec_t **match_specs,
    pipe_mgr_move_list_t *move_node,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info);

pipe_status_t pipe_mgr_tcam_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls);

pipe_status_t pipe_mgr_get_tcam_index_for_match_addr(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    uint32_t stage_line_no,
    uint32_t *index_p,
    uint32_t *ptn_index_p);

pipe_status_t pipe_mgr_tcam_decode_tind_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_stage_info_t *stage_data,
    uint32_t stage_line_no,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    uint8_t *tind_word_ptr,
    uint32_t tind_subword_pos,
    uint16_t offset);

pipe_status_t pipe_mgr_tcam_range_max_expansion_entry_count_get(
    tcam_tbl_info_t *tcam_tbl_info, uint32_t *max_expanded_range_entries);

pipe_status_t pipe_mgr_tcam_decode_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_stage_info_t *stage_data,
    uint32_t line_no,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    uint8_t **tcam_word_ptrs,
    uint8_t subword,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    bool *entry_valid,
    bool *is_range_entry_head);

pipe_status_t pipe_mgr_tcam_reset_default_entry(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t tbl_hdl);

pipe_status_t pipe_mgr_tcam_get_entry_from_hw_raw(
    tcam_tbl_t *tcam_tbl,
    bf_dev_pipe_t pipe_id,
    tcam_stage_info_t *stage_data,
    tcam_block_data_t *block_data,
    uint32_t tcam_index,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    uint8_t **tcam_word_ptrs,
    bool *entry_valid);

pipe_status_t tcam_fix_unexpected_entry(tcam_tbl_t *tcam_tbl,
                                        uint32_t entry_idx,
                                        uint32_t num_entries);

pipe_status_t tcam_fix_missing_entry(tcam_tbl_t *tcam_tbl,
                                     tcam_llp_entry_t *tcam_entry,
                                     struct pipe_mgr_mat_data *mat_data);

pipe_status_t pipe_mgr_invalidate_tcam_idx(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           bf_dev_pipe_t pipe_id,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           uint32_t entry_index);

pipe_status_t tcam_hw_raw_entry_get(tcam_tbl_t *tcam_tbl,
                                    bf_dev_pipe_t pipe_id,
                                    tcam_phy_loc_info_t *tcam_loc,
                                    uint32_t ptn_entry_index,
                                    pipe_tbl_match_spec_t *match_spec,
                                    pipe_action_spec_t *act_spec,
                                    pipe_act_fn_hdl_t *act_fn_hdl,
                                    uint32_t num_entries,
                                    bool *entry_valid);
#endif
