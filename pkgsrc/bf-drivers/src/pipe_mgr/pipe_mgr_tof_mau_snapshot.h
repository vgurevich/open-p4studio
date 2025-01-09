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
 * @file pipe_mgr_tof_mau_snapshot.h
 * @date
 *
 * Contains definitions of MAU snapshot
 *
 */
#ifndef _PIPE_MGR_TOF_MAU_SNAPSHOT_H
#define _PIPE_MGR_TOF_MAU_SNAPSHOT_H

#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_mau_snapshot.h"

pipe_status_t pipe_mgr_snapshot_timer_enable_tof(rmt_dev_info_t *dev_info,
                                                 bf_dev_pipe_t pipe,
                                                 dev_stage_t stage,
                                                 bool ing_enable,
                                                 bool egr_enable);
pipe_status_t pipe_mgr_snapshot_timer_get_enable_tof(rmt_dev_info_t *dev_info,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     bool *ing_enable,
                                                     bool *egr_enable);
pipe_status_t pipe_mgr_snapshot_timer_set_tof(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              uint64_t clocks);
pipe_status_t pipe_mgr_snapshot_timer_get_tof(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              uint64_t *clocks_now,
                                              uint64_t *clocks_trig);
pipe_status_t pipe_mgr_snapshot_capture_trigger_set_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    pipe_mgr_phv_spec_t *phv_spec);
pipe_status_t pipe_mgr_snapshot_fsm_state_set_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t fsm_state);
pipe_status_t pipe_mgr_snapshot_fsm_state_get_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t *fsm_state);
pipe_status_t pipe_mgr_get_snapshot_captured_data_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_mgr_phv_spec_t *phv_spec,
    pipe_mgr_snapshot_capture_data_t *capture);
pipe_status_t pipe_mgr_snapshot_interrupt_clear_tof(rmt_dev_info_t *dev_info,
                                                    bf_dev_pipe_t pipe,
                                                    dev_stage_t stage,
                                                    bf_snapshot_dir_t dir);
pipe_status_t pipe_mgr_snapshot_interrupt_get_tof(rmt_dev_info_t *dev_info,
                                                  bf_dev_pipe_t pipe,
                                                  dev_stage_t stage,
                                                  bf_snapshot_dir_t dir,
                                                  bool *is_set);
pipe_status_t pipe_mgr_snapshot_captured_trigger_type_get_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    int dir,
    bool *prev_stage_trig,
    bool *local_stage_trig,
    bool *timer_trig);

pipe_status_t pipe_mgr_snapshot_captured_thread_get_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    int dir,
    bool *ingress,
    bool *egress);

pipe_status_t pipe_mgr_snapshot_dp_reset_tof(rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t stage,
                                             int dir);
#endif
