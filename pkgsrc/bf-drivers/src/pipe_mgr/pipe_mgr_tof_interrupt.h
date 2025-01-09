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
 * @file pipe_mgr_tof_interrupt.h
 * @date
 *
 * Definitions for Tofino1 interrupt handling
 *
 */
#ifndef _PIPE_MGR_TOF_INTERRUPT_H
#define _PIPE_MGR_TOF_INTERRUPT_H

/* Module header includes */
#include "pipe_mgr_int.h"

pipe_status_t pipe_mgr_tof_tcam_read(bf_dev_id_t dev);

/* register interrupt notification */
pipe_status_t pipe_mgr_tof_register_interrupt_notifs(rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof_register_mau_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_mirror_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_tm_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_parser_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_ig_deparser_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_pgr_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_gfm_interrupt_notifs(
    rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_tof_register_sbc_interrupt_notifs(bf_dev_id_t dev);

/* interrupt mode set helper */
pipe_status_t pipe_mgr_tof_interrupt_en_set_helper(rmt_dev_info_t *dev_info,
                                                   bool enable,
                                                   bool push_now);
#endif
