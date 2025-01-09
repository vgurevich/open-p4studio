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
 * @file pipe_mgr_tof2_interrupt.h
 * @date
 *
 * Definitions for Tofino2 interrupt handling.
 *
 */
#ifndef _PIPE_MGR_TOF2_INTERRUPT_H
#define _PIPE_MGR_TOF2_INTERRUPT_H

/* Module header includes */
#include "pipe_mgr_int.h"

/* pipe_mgr_tof2_interrupt.c */
pipe_status_t pipe_mgr_tof2_tcam_read(bf_dev_id_t dev);

pipe_status_t pipe_mgr_tof2_intr_reg_wr(rmt_dev_info_t *dev_info,
                                        pipe_bitmap_t *pbm,
                                        pipe_sess_hdl_t shdl,
                                        uint32_t addr,
                                        uint32_t data);

pipe_status_t pipe_mgr_tof2_interrupt_en_set_helper(rmt_dev_info_t *dev_info,
                                                    bool enable,
                                                    bool push_now);

pipe_status_t pipe_mgr_tof2_register_interrupt_notifs(rmt_dev_info_t *dev_info);

/* pipe_mgr_tof2_deprsr_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_deparser_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_deparser_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                      bool enable);

/* pipe_mgr_tof2_lfltr_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_lfltr_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_lfltr_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                   bool enable);

/* pipe_mgr_tof2_mau_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_mau_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_gfm_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable);

pipe_status_t pipe_mgr_tof2_mau_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable);

/* pipe_mgr_tof2_mirror_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_mirror_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_mirror_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                    bool enable);

/* pipe_mgr_tof2_parde_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_parde_misc_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_parde_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                   bool enable);

/* pipe_mgr_tof2_parser_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_parser_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_parser_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                    bool enable);

/* pipe_mgr_tof2_pgr_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_pgr_interrupt_notifs(
    rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_tof2_pgr_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable);

/* pipe_mgr_tof2_sbc_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_sbc_interrupt_notifs(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_tof2_sbc_interrupt_en_set(bf_dev_id_t dev_id,
                                                 bool enable);

/* pipe_mgr_tof2_tm_interrupt.c */
pipe_status_t pipe_mgr_tof2_register_tm_interrupt_notifs(
    rmt_dev_info_t *dev_info);

/* pipe_mgr_tof2_mau_interrupt.c */
pipe_status_t pipe_mgr_tof2_imem_rewrite(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info,
                                         bf_dev_pipe_t log_pipe,
                                         bf_dev_pipe_t phy_pipe,
                                         int stage);

pipe_status_t pipe_mgr_tof2_tm_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                bool enable);

#endif
