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
 * @file pipe_mgr_exm_tbl_dump.c
 * @date
 *
 * Exact match table manager dump definitions
 */

/* Standard header includes */

/* Module header includes */

/* Local header includes */

void pipe_mgr_exm_dump_tbl_info(ucli_context_t *uc,
                                uint8_t device_id,
                                pipe_mat_tbl_hdl_t exm_tbl_hdl);

void pipe_mgr_exm_dump_stage_info(ucli_context_t *uc,
                                  pipe_mgr_exm_stage_info_t *exm_stage_info);

void pipe_mgr_exm_dump_hashway_info(
    ucli_context_t *uc,
    pipe_mgr_exm_hash_way_data_t *exm_hash_way_data,
    uint8_t num_rams_in_wide_word,
    uint8_t num_entries_per_wide_word);

void pipe_mgr_exm_dump_entry_info(ucli_context_t *uc,
                                  uint8_t device_id,
                                  pipe_mat_tbl_hdl_t exm_tbl_hdl,
                                  pipe_mat_ent_hdl_t entry_hdl);

void pipe_mgr_exm_dump_phy_entry_info(ucli_context_t *uc,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t exm_tbl_hdl,
                                      pipe_mat_ent_hdl_t entry_hdl);

void pipe_mgr_exm_tbl_dump_hash(ucli_context_t *uc,
                                bf_dev_id_t device_id,
                                pipe_mat_tbl_hdl_t exm_tbl_hdl);

void pipe_mgr_exm_tbl_dump_entries(ucli_context_t *uc,
                                   bf_dev_id_t device_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   bool show_entry_handles);
void pipe_mgr_exm_entry_move_stats_dump(ucli_context_t *uc,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl);
void pipe_mgr_exm_entry_move_stats_clear(ucli_context_t *uc,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl);
