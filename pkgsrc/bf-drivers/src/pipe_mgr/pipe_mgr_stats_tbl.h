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
 * @file pipe_mgr_stats_tbl.h
 * @date
 *
 *
 * Contains definitions relating to pipe mgr's statistics table management
 */

#ifndef _PIPE_MGR_STATS_TBL_H
#define _PIPE_MGR_STATS_TBL_H
/* Global header includes */

/* Module header includes */
#include <target-utils/third-party/cJSON/cJSON.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"

#define PIPE_STAT_ENT_INVALID_ENTRY_INDEX 0xdeadbeef

typedef uint32_t pipe_stat_stage_ent_idx_t;

typedef struct pipe_mgr_stat_move_list_ {
  struct pipe_mgr_stat_move_list_ *next;
  struct pipe_mgr_stat_move_list_ *prev;
  pipe_stat_stage_ent_idx_t src_ent_idx;
  uint8_t src_stage_id;
  uint8_t dst_stage_id;
  pipe_stat_stage_ent_idx_t dst_ent_idx;

} pipe_mgr_stat_move_list_t;

pipe_status_t pipe_mgr_stat_mgr_verify_idx(bf_dev_id_t device_id,
                                           bf_dev_pipe_t pipe_id,
                                           pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                           pipe_stat_ent_idx_t stat_ent_idx);

/* API function to attach a stats entry to a match table entry */
pipe_status_t rmt_stat_mgr_stat_ent_attach(uint8_t device_id,
                                           bf_dev_pipe_t pipe_id,
                                           uint8_t stage_id,
                                           pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                           pipe_stat_ent_idx_t stat_ent_idx,
                                           rmt_virt_addr_t *stat_ent_virt_addr);

/* API to compute a statistics virtual address with the per-flow enable
 * bit set to ZERO.
 */
pipe_status_t pipe_mgr_stat_mgr_compute_disabled_address(
    bf_dev_id_t device_id,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    rmt_virt_addr_t *virt_addr);

pipe_status_t pipe_mgr_stat_mgr_ent_query(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                          pipe_stat_ent_idx_t *stat_ent_idx,
                                          size_t num_entries,
                                          pipe_stat_data_t **stat_data);

pipe_status_t pipe_mgr_stat_tbl_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie);

pipe_status_t pipe_mgr_stat_tbl_log_database_sync(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_stat_tbl_hdl_t stat_tbl_hdl);

pipe_status_t rmt_stat_mgr_query_direct_stats(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              bf_dev_pipe_t pipe_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mat_ent_hdl_t mat_ent_hdl,
                                              pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                              pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_direct_stat_ent_sync(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl);

pipe_status_t rmt_stat_mgr_direct_stat_ent_database_sync(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx);

pipe_status_t rmt_stat_mgr_stat_ent_move(bf_dev_id_t device_id,
                                         pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                         bf_dev_pipe_t pipe_id,
                                         pipe_mat_ent_hdl_t mat_ent_hdl,
                                         dev_stage_t src_stage_id,
                                         dev_stage_t dst_stage_id,
                                         pipe_stat_stage_ent_idx_t src_ent_idx,
                                         pipe_stat_stage_ent_idx_t dst_ent_idx);

pipe_status_t pipe_mgr_stat_mgr_add_entry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    bool hw_init,
    pipe_stat_data_t *stat_data,
    rmt_virt_addr_t *stat_ent_virt_addr);

pipe_status_t pipe_mgr_stat_mgr_delete_entry(
    dev_target_t dev_tgt,
    dev_stage_t stage_id,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    pipe_stat_tbl_hdl_t stat_tbl_hdl);

pipe_status_t pipe_mgr_stat_mgr_ent_set(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                        pipe_stat_ent_idx_t stat_ent_idx,
                                        pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_ent_load(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                         pipe_stat_ent_idx_t stat_ent_idx,
                                         pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_stage_ent_load(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    uint8_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_direct_ent_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_reset_entry(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            bf_dev_pipe_t pipe_id,
                                            pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                            dev_stage_t stage_id,
                                            pipe_stat_stage_ent_idx_t ent_idx);

pipe_status_t pipe_mgr_stat_tbl_init(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                     profile_id_t profile_id,
                                     pipe_bitmap_t *pipe_bmp);

pipe_status_t rmt_stat_mgr_tbl_lock(dev_target_t dev_tgt,
                                    pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                    dev_stage_t stage_id,
                                    lock_id_t lock_id);

pipe_status_t rmt_stat_mgr_tbl_unlock(dev_target_t dev_tgt,
                                      pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                      dev_stage_t stage_id,
                                      lock_id_t lock_id);

pipe_status_t pipe_mgr_stat_mgr_decode_virt_addr(
    bf_dev_id_t device_id,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t virt_addr,
    bool *pfe,
    bool *pfe_defaulted,
    pipe_stat_ent_idx_t *stats_idx);

pipe_status_t pipe_mgr_stat_log_state(bf_dev_id_t dev_id,
                                      pipe_stat_tbl_hdl_t tbl_hdl,
                                      cJSON *stat_tbls);

pipe_status_t pipe_mgr_stat_restore_state(bf_dev_id_t dev_id, cJSON *stat_tbl);

pipe_status_t pipe_mgr_stat_mgr_reset_table(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_stat_tbl_hdl_t stat_tbl_hdl);

bool pipe_mgr_stat_tbl_is_indirect(bf_dev_id_t device_id,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl);

pipe_status_t pipe_mgr_stat_tbl_sbe_correct(bf_dev_id_t dev_id,
                                            bf_dev_pipe_t log_pipe_id,
                                            dev_stage_t stage_id,
                                            pipe_stat_tbl_hdl_t tbl_hdl,
                                            int line);
#endif
