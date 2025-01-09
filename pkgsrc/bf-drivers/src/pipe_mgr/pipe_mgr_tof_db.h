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


#ifndef _PIPE_MGR_TOF_DB_H
#define _PIPE_MGR_TOF_DB_H

#define PIPE_MGR_TOF_GFM_PARITY_COL 51
#define PIPE_MGR_TOF_SINGLE_GFM_ENTRY_SZ 18
#define PIPE_MGR_TOF_SEED_PARITY_COL 51
#define PIPE_MGR_TOF_OUTPUT_PARITY_GROUPS 8

#include "pipe_mgr_db.h"
pipe_status_t pipe_mgr_tof_interrupt_db_init(rmt_dev_info_t *dev_info);
void pipe_mgr_tof_prsr_db_init(bf_dev_id_t dev);
void pipe_mgr_tof_interrupt_db_cleanup(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_recalc_write_seed(pipe_sess_hdl_t sess_hdl,
                                             rmt_dev_info_t *dev_info,
                                             pipe_bitmap_t *pipe_bmp,
                                             dev_stage_t stage);
pipe_status_t pipe_mgr_tof_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
                                                   rmt_dev_info_t *dev_info,
                                                   pipe_bitmap_t *pipe_bmp,
                                                   dev_stage_t stage,
                                                   bool skip_write);
pipe_status_t pipe_mgr_tof_interrupt_cache_imem_val(rmt_dev_info_t *dev_info,
                                                    uint32_t log_pipe_mask,
                                                    dev_stage_t stage,
                                                    uint32_t base_address,
                                                    uint8_t *data,
                                                    int data_len);

pipe_status_t pipe_mgr_tof_cache_prsr_reg_val(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint32_t address,
    uint32_t data,
    bool *shadowed);

pipe_status_t pipe_mgr_tof_cache_prsr_val(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t address,
    uint8_t *data,
    int data_len,
    bool *shadowed);

pipe_status_t pipe_mgr_tof_interrupt_set_parser_tcam_shadow(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    bool ing0_egr1,
    int prsr_id,
    int tcam_index,
    uint8_t data_len,
    uint8_t *word0,
    uint8_t *word1);
pipe_status_t pipe_mgr_tof_cache_gfm(rmt_dev_info_t *dev_info,
                                     uint32_t log_pipe_mask,
                                     dev_stage_t stage,
                                     uint32_t address,
                                     uint8_t *data,
                                     int data_len);

#endif
