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


#ifndef PIPE_MGR_STATS_MGR_TRACE_H
#define PIPE_MGR_STATS_MGR_TRACE_H

#include "pipe_mgr_stat_mgr_int.h"

#define PIPE_MGR_STAT_DBGCHK(inst, x)    \
  {                                      \
    if (!(x)) {                          \
      pipe_mgr_stat_trace_err_log(inst); \
      PIPE_MGR_DBGCHK(x);                \
    }                                    \
  }

void pipe_mgr_stat_mgr_trace_add(pipe_mgr_stat_tbl_t *stat_tbl,
                                 pipe_mgr_stat_tbl_instance_t *inst,
                                 pipe_mat_ent_hdl_t ent_hdl,
                                 dev_stage_t stage_id,
                                 pipe_stat_stage_ent_idx_t stage_idx,
                                 uint32_t deferred);
void pipe_mgr_stat_mgr_trace_def_add(pipe_mgr_stat_tbl_t *stat_tbl,
                                     pipe_mgr_stat_tbl_instance_t *inst,
                                     pipe_mat_ent_hdl_t ent_hdl,
                                     dev_stage_t stage_id,
                                     pipe_stat_stage_ent_idx_t stage_idx,
                                     bf_dev_pipe_t pipe_id);
void pipe_mgr_stat_mgr_trace_del(pipe_mgr_stat_tbl_t *stat_tbl,
                                 pipe_mgr_stat_tbl_instance_t *inst,
                                 pipe_mat_ent_hdl_t ent_hdl,
                                 dev_stage_t stage_id,
                                 pipe_stat_stage_ent_idx_t stage_idx,
                                 uint32_t deferred);
void pipe_mgr_stat_mgr_trace_def_del(pipe_mgr_stat_tbl_t *stat_tbl,
                                     pipe_mgr_stat_tbl_instance_t *inst,
                                     pipe_mat_ent_hdl_t ent_hdl,
                                     dev_stage_t stage_id,
                                     pipe_stat_stage_ent_idx_t stage_idx,
                                     bf_dev_pipe_t pipe_id);
void pipe_mgr_stat_mgr_trace_mov(pipe_mgr_stat_tbl_t *stat_tbl,
                                 pipe_mgr_stat_tbl_instance_t *inst,
                                 pipe_mat_ent_hdl_t ent_hdl,
                                 dev_stage_t dst_stage_id,
                                 pipe_stat_stage_ent_idx_t dst_stage_idx,
                                 dev_stage_t src_stage_id,
                                 pipe_stat_stage_ent_idx_t src_stage_idx,
                                 uint32_t deferred);
void pipe_mgr_stat_mgr_trace_def_mov(pipe_mgr_stat_tbl_t *stat_tbl,
                                     pipe_mgr_stat_tbl_instance_t *inst,
                                     pipe_mat_ent_hdl_t ent_hdl,
                                     dev_stage_t dst_stage_id,
                                     pipe_stat_stage_ent_idx_t dst_stage_idx,
                                     dev_stage_t src_stage_id,
                                     pipe_stat_stage_ent_idx_t src_stage_idx,
                                     bf_dev_pipe_t pipe_id);
void pipe_mgr_stat_mgr_trace_bar(pipe_mgr_stat_tbl_t *stat_tbl,
                                 pipe_mgr_stat_tbl_instance_t *inst,
                                 pipe_mgr_stat_barrier_state_t *bs);
void pipe_mgr_stat_mgr_trace_bar_ack(pipe_mgr_stat_tbl_t *stat_tbl,
                                     pipe_mgr_stat_tbl_instance_t *inst,
                                     bf_dev_pipe_t pipe_id,
                                     dev_stage_t stage_id,
                                     lock_id_t lock_id,
                                     bool deferred);
pipe_status_t pipe_mgr_stat_trace_str(pipe_mgr_stat_tbl_trace_entry_t *trace,
                                      char *buf,
                                      int buf_len);
void pipe_mgr_stat_trace_err_log(pipe_mgr_stat_tbl_instance_t *inst);
#endif
