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


#ifndef _PIPE_MGR_STFUL_TBL_MGR_H
#define _PIPE_MGR_STFUL_TBL_MGR_H

#include <target-utils/map/map.h>
#include <target-utils/third-party/cJSON/cJSON.h>

#define BITS_PER_LOG_ENTRY (128)

struct pipe_mgr_stful_tbl_stage_info {
  int stage_id;
  int logical_table;
  uint32_t num_entries;
  int num_spare_rams;
  mem_id_t spare_rams[RMT_MAX_S2P_SECTIONS];
  int num_rams;
  vpn_id_t ram_vpn[47];
  mem_id_t ram_ids[47];
  /* First dimension is by active-pipes.  Second dimension is for the rams
   * allocated to the table in the stage.  Only applies to indirect tables. */
  uint8_t **db;
  uint32_t db_sz;  // In bytes
  uint8_t num_alu_ids;
  uint8_t alu_ids[RMT_MAX_S2P_SECTIONS];
};

struct pipe_mgr_stful_tbl_inst {
  bf_dev_pipe_t pipe_id; /* lowest pipe in scope */
  /* Bit map of all the pipes in which this table instance is present */
  pipe_bitmap_t pipe_bmp;
};

enum pipe_mgr_stful_tbl_type {
  pipe_mgr_stful_tbl_type_normal,
  pipe_mgr_stful_tbl_type_log,
  pipe_mgr_stful_tbl_type_fifo,
  pipe_mgr_stful_tbl_type_stack,
};
static inline const char *pipe_mgr_stful_tbl_type_str(
    enum pipe_mgr_stful_tbl_type t) {
  switch (t) {
    case pipe_mgr_stful_tbl_type_normal:
      return "Normal";
    case pipe_mgr_stful_tbl_type_log:
      return "Logging";
    case pipe_mgr_stful_tbl_type_fifo:
      return "FIFO";
    case pipe_mgr_stful_tbl_type_stack:
      return "Stack";
  }
  return "Unknown";
}
enum pipe_mgr_stful_fifo_direction {
  pipe_mgr_stful_fifo_cpu_none,
  pipe_mgr_stful_fifo_cpu_push,
  pipe_mgr_stful_fifo_cpu_pop
};
static inline const char *pipe_mgr_stful_fifo_direction_str(
    enum pipe_mgr_stful_fifo_direction d) {
  switch (d) {
    case pipe_mgr_stful_fifo_cpu_none:
      return "None";
    case pipe_mgr_stful_fifo_cpu_push:
      return "Push";
    case pipe_mgr_stful_fifo_cpu_pop:
      return "Pop";
  }
  return "Unknown";
}

struct pipe_mgr_stful_tbl {
  pipe_mat_tbl_hdl_t hdl;
  bf_dev_id_t dev;
  rmt_dev_info_t *dev_info;
  pipe_sel_tbl_hdl_t sel_tbl_hdl;
  bool direct;
  bool symmetric;
  bool in_restore;
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  bf_map_t *hdl_to_shdw;      /* One per logical pipe for direct tables. */
  uint32_t num_tbl_instances; /* Num of tbl instances  */
  struct pipe_mgr_stful_tbl_inst *stful_tbl_inst; /* Array of table instances */
  profile_id_t profile_id;
  pipe_bitmap_t pipes;
  int num_stages;
  int width;
  bool dbl_width;
  uint32_t num_entries_p4;    // Requested by P4 program
  uint32_t num_entries_real;  // Actually allocated
  enum pipe_mgr_stful_tbl_type tbl_type;
  enum pipe_mgr_stful_fifo_direction fifo_direction;
  int set_instr;
  int set_at_instr;
  int clr_instr;
  int clr_at_instr;
  int cntr_idx;
  int *cntr_val;  // Array of size num-active-pipes
  bf_map_t action_to_instr_map;
  pipe_stful_mem_spec_t initial_value;
  bool skip_shadow;
  pipe_stful_tbl_sync_cback_fn sync_cb;
  void *sync_cookie;
  bool logging_in_progress;
  uint32_t *tbl_sync_rcvd_sz;
  int sync_row_idx;    // When syncing one entry, the ram+line it is in
  int sync_stage_idx;  // When syncing one entry the stage index it is in
  struct pipe_mgr_stful_tbl_stage_info *stages;
  uint32_t num_reg_params;
  pipe_stful_register_param_t *reg_params;
  uint32_t direction;  // Direction
};

struct pipe_mgr_stful_op_list_t;

struct pipe_mgr_stful_tbl *pipe_mgr_stful_tbl_get(bf_dev_id_t dev,
                                                  pipe_stful_tbl_hdl_t hdl);
void pipe_mgr_stful_free_ops(struct pipe_mgr_stful_op_list_t **l);
pipe_status_t pipe_mgr_stful_process_op_list(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev,
                                             pipe_tbl_hdl_t tbl_hdl,
                                             struct pipe_mgr_stful_op_list_t *l,
                                             uint32_t *num_processed);

pipe_status_t pipe_mgr_stful_get_direct_stful_hdl(
    bf_dev_id_t dev,
    pipe_mat_tbl_hdl_t mat_hdl,
    pipe_stful_tbl_hdl_t *stful_hdl);

pipe_status_t pipe_mgr_stful_direct_word_write_at(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t pipe,
    int stage,
    uint32_t idx,
    pipe_stful_mem_spec_t *spec,
    uint32_t api_flags);
pipe_status_t pipe_mgr_stful_word_write(bf_dev_target_t dev_tgt,
                                        pipe_stful_tbl_hdl_t tbl_hdl,
                                        pipe_stful_mem_idx_t index,
                                        pipe_stful_mem_spec_t *spec,
                                        struct pipe_mgr_stful_op_list_t **l);

pipe_status_t pipe_mgr_stful_tbl_sync(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t st_hdl,
                                      pipe_stful_tbl_sync_cback_fn cb,
                                      void *cookie);
pipe_status_t pipe_mgr_stful_direct_tbl_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie,
    bool *immediate_cb);
pipe_status_t pipe_mgr_stful_query_sizes(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         int *num_pipes);
pipe_status_t pipe_mgr_stful_direct_query_sizes(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                int *num_pipes);
pipe_status_t pipe_mgr_stful_ent_query(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                       pipe_stful_mem_idx_t stful_ent_idx,
                                       pipe_stful_mem_query_t *data,
                                       uint32_t pipe_api_flags);
pipe_status_t pipe_mgr_stful_ent_query_range(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                             pipe_stful_mem_idx_t stful_ent_idx,
                                             uint32_t num_indices,
                                             pipe_stful_mem_query_t *data,
                                             uint32_t *num_indices_read,
                                             uint32_t pipe_api_flags);
pipe_status_t pipe_mgr_stful_direct_ent_query(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mat_ent_hdl_t mat_ent_hdl,
                                              pipe_stful_mem_query_t *data,
                                              uint32_t pipe_api_flags);

pipe_status_t pipe_mgr_stful_table_reset(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_target_t dev_tgt,
                                         pipe_stful_tbl_hdl_t tbl_hdl,
                                         pipe_stful_mem_spec_t *stful_spec);

pipe_status_t pipe_mgr_stful_table_reset_range(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t tbl_hdl,
    pipe_stful_mem_idx_t stful_ent_idx,
    uint32_t num_indices,
    pipe_stful_mem_spec_t *stful_spec);

pipe_status_t pipe_mgr_stful_fifo_occupancy(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                            int *occupancy);
pipe_status_t pipe_mgr_stful_fifo_reset(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stful_tbl_hdl_t tbl_hdl);
pipe_status_t pipe_mgr_stful_fifo_dequeue(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                          int num_to_dequeue,
                                          pipe_stful_mem_spec_t *values,
                                          int *num_dequeued);
pipe_status_t pipe_mgr_stful_fifo_enqueue(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                          int num_to_enqueue,
                                          pipe_stful_mem_spec_t *values);

pipe_status_t pipe_mgr_stful_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_stful_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev,
    pipe_stful_tbl_hdl_t h,
    bool tf,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_stful_verify_idx(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        pipe_stful_tbl_hdl_t hdl,
                                        pipe_stful_mem_idx_t idx);

pipe_status_t pipe_mgr_stful_get_indirect_ptr(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              int stage,
                                              pipe_act_fn_hdl_t act_fn_hdl,
                                              pipe_stful_tbl_hdl_t hdl,
                                              pipe_stful_mem_idx_t idx,
                                              uint32_t *ptr);
pipe_status_t pipe_mgr_stful_get_indirect_disabled_ptr(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       int stage,
                                                       pipe_stful_tbl_hdl_t hdl,
                                                       uint32_t *ptr);
pipe_status_t pipe_mgr_stful_dir_ent_del(bf_dev_id_t dev_id,
                                         pipe_stful_tbl_hdl_t hdl,
                                         bf_dev_pipe_t pipe_id,
                                         pipe_mat_ent_hdl_t ent_hdl);
pipe_status_t pipe_mgr_stful_move(pipe_sess_hdl_t ses,
                                  bf_dev_id_t dev,
                                  pipe_stful_tbl_hdl_t h,
                                  bf_dev_pipe_t pipe,
                                  int src_stage,
                                  int dst_stage,
                                  pipe_stful_mem_idx_t src_idx,
                                  pipe_stful_mem_idx_t dst_idx);

pipe_status_t pipe_mgr_stful_wr_bit(pipe_sess_hdl_t ses,
                                    bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    pipe_stful_tbl_hdl_t hdl,
                                    int stage,
                                    int idx,
                                    bool val,
                                    bool adjust_total);

bool pipe_mgr_stful_tbl_is_direct(bf_dev_id_t dev, pipe_stful_tbl_hdl_t hdl);

void pipe_mgr_stful_spec_decode(bf_dev_id_t dev,
                                pipe_stful_tbl_hdl_t hdl,
                                pipe_stful_mem_spec_t spec,
                                bool *dual,
                                uint32_t *hi,
                                uint32_t *lo);

pipe_status_t pipe_mgr_stful_tbl_del(bf_dev_id_t dev, pipe_stful_tbl_hdl_t hdl);
pipe_status_t pipe_mgr_stful_tbl_add(pipe_sess_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     pipe_stful_tbl_hdl_t hdl,
                                     profile_id_t profile,
                                     pipe_bitmap_t *pipes);

int pipe_mgr_stful_log_tbl_info_to_buf(bf_dev_id_t dev,
                                       pipe_stful_tbl_hdl_t hdl,
                                       char *const buf,
                                       size_t s);

pipe_status_t pipe_mgr_stful_mgr_decode_virt_addr(
    bf_dev_id_t device_id,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t stful_ptr,
    bool *pfe,
    bool *pfe_defaulted,
    uint32_t *stful_idx);

pipe_status_t pipe_mgr_stful_log_state(bf_dev_id_t dev_id,
                                       pipe_stful_tbl_hdl_t tbl_hdl,
                                       cJSON *stful_tbls);

pipe_status_t pipe_mgr_stful_log_cmplt(bf_dev_id_t dev_id,
                                       pipe_stful_tbl_hdl_t tbl_hdl);

pipe_status_t pipe_mgr_stful_restore_state(bf_dev_id_t dev_id,
                                           cJSON *stful_tbl);

pipe_status_t pipe_mgr_stful_download_specs_from_shadow(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *dev_profile_info,
    pipe_stful_tbl_info_t *stful_tbl_info,
    void *arg);

pipe_status_t pipe_mgr_stful_mgr_get_stful_spec_at(
    bf_dev_id_t device_id,
    pipe_stful_tbl_hdl_t tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    uint32_t index_in_stage,
    pipe_stful_mem_spec_t *stful_spec);

pipe_status_t pipe_mgr_stful_sbe_correct(bf_dev_id_t dev_id,
                                         bf_dev_pipe_t log_pipe_id,
                                         dev_stage_t stage_id,
                                         pipe_stful_tbl_hdl_t tbl_hdl,
                                         int line);
pipe_status_t pipe_mgr_stful_param_set(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_tbl_hdl_t tbl_hdl,
                                       pipe_reg_param_hdl_t rp_hdl,
                                       int64_t value,
                                       bool reset);

pipe_status_t pipe_mgr_stful_param_get(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_tbl_hdl_t tbl_hdl,
                                       pipe_reg_param_hdl_t rp_hdl,
                                       int64_t *value);

pipe_status_t pipe_mgr_stful_param_get_hdl(bf_dev_id_t dev,
                                           const char *name,
                                           pipe_reg_param_hdl_t *hdl);

#endif  // PIPE_MGR_STFUL_TBL_MGR_H
