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


#ifndef __TM_TABLE_DISPLAY_H__
#define __TM_TABLE_DISPLAY_H__

#if TRAFFIC_MGR_CONFIG_INCLUDE_UCLI == 1
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#endif

bf_status_t tm_ucli_display_ppg_min_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
// TM_PIPE_WATERMARK_TABLES
bf_status_t tm_ucli_display_wac_ppg_watermark(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_port_watermark(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_q_wm(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_port_wm(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);

// TM_PIPE_USAGE_TABLES
bf_status_t tm_ucli_display_ppg_cell_usage_gmin(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_dpg_cell_usage_gmin(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_cell_usage_shrd_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_dpg_cell_usage_shrd_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_cell_usage_skid_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_port_usage_count(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_port_cellusage(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_q_cellusage(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
// TM_PIPE_DROPSTATUS_TABLES
bf_status_t tm_ucli_display_ppg_dropstate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_port_dropstate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_green_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_yellow_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_red_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_port_drop_state(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
// TM_PIPE_PFCSTATUS_TABLES
bf_status_t tm_ucli_display_wac_pfc_state(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_egress_port_pfc_status(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_egress_q_pfc_status(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
// TM_NONPIPE_DROPSTATUS_TABLES
bf_status_t tm_ucli_display_wac_color_drop_state(ucli_context_t *uc,
                                                 int nz,
                                                 int hex,
                                                 bf_dev_id_t devid);
bf_status_t tm_ucli_display_wac_skidpool_dropstate(ucli_context_t *uc,
                                                   int nz,
                                                   int hex,
                                                   bf_dev_id_t devid);
bf_status_t tm_ucli_display_wac_queue_shadow_state(ucli_context_t *uc,
                                                   int nz,
                                                   int hex,
                                                   bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_dropstate(ucli_context_t *uc,
                                          int nz,
                                          int hex,
                                          bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_green_dropstate(ucli_context_t *uc,
                                                int nz,
                                                int hex,
                                                bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_yel_dropstate(ucli_context_t *uc,
                                              int nz,
                                              int hex,
                                              bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_red_dropstate(ucli_context_t *uc,
                                              int nz,
                                              int hex,
                                              bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_pipe_pre_fifo_dropstate(ucli_context_t *uc,
                                                        int nz,
                                                        int hex,
                                                        bf_dev_id_t devid);
// TM_PERPIPE_ALLMODULE_COUNTERS
bf_status_t tm_ucli_display_perpipe_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_per_port_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_per_ppg_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_queue_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_stop_stats_cache_timer(ucli_context_t *uc,
                                           int nz,
                                           int hex,
                                           bf_dev_id_t devid);
bf_status_t tm_ucli_start_stats_cache_timer(ucli_context_t *uc,
                                            int nz,
                                            int hex,
                                            bf_dev_id_t devid);
bf_status_t tm_ucli_display_qac_port_drop_color_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_nonpipe_counter(ucli_context_t *uc,
                                            int nz,
                                            int hex,
                                            bf_dev_id_t devid);
// TM_PERPIPE_ALLMODULE_CLEAR_COUNTERS
bf_status_t tm_ucli_clear_qac_port_drop_counter(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_clear_qac_q_drop_counter(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_clear_wac_per_ppg_drop_counter(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_clear_wac_per_port_drop_counter(bf_dev_id_t devid,
                                                    int pipe);
bf_status_t tm_ucli_clear_nonpipe_counter(bf_dev_id_t devid);
bf_status_t tm_ucli_clear_perpipe_counter(bf_dev_id_t devid, int pipe);
// CLEAR WM COUNTERS
bf_status_t tm_ucli_reset_wac_port_watermark(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_wac_ppg_watermark(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_port_wm(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_q_wm(bf_dev_id_t devid, int pipe);
// TM_PIPE_CFG_TABLES
bf_status_t tm_ucli_display_ppg_gmin_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_hdr_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_port_ppg(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_shared_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_icos_mapping(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_offset_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_min_thrd(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_shr_thrd(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_ap(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_min_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_q_max_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_port_max_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ing_port_drop_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_port_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_qid_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_offset_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_qac_qid_map(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_qid_map(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_wac_eg_qid_map(ucli_context_t *uc,
                                           int nz,
                                           int hex,
                                           bf_dev_id_t devid);
bf_status_t tm_ucli_display_q_color_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_ppg_resume_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_display_port_q_mapping(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe);
// TM_PIPE_RSRC_CLI
bf_status_t tm_ucli_display_ppg_details(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe, int rsrc);
bf_status_t tm_ucli_display_queue_details(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe, int rsrc);
bf_status_t tm_ucli_display_port_details(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe, int rsrc);
// MISC
bf_status_t tm_ucli_display_pre_port_mask_vector(ucli_context_t *uc,
                                                 int nz,
                                                 int hex,
                                                 bf_dev_id_t devid);
bf_status_t tm_ucli_clear_pre_port_mask_vector(bf_dev_id_t devid);
bf_status_t tm_ucli_display_pre_port_down_mask(ucli_context_t *uc,
                                               int nz,
                                               int hex,
                                               bf_dev_id_t devid);
bf_status_t tm_ucli_clear_pre_port_down_mask(bf_dev_id_t devid);

bf_status_t bf_tm_power_on_reset_non_pipe_tbls(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_qac_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_qac_green_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_qac_yel_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_qac_red_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_qac_pipe_pre_fifo_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_wac_color_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_wac_skid_pool_dropstate(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_wac_queue_shadow_state(bf_dev_id_t devid);
bf_status_t tm_ucli_reset_wac_eg_qid_map(bf_dev_id_t devid);

bf_status_t tm_ucli_reset_ppg_min_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_hdr_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ing_port_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_wac_port_usage_count(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_port_dropstate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_dropstate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_usage(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_min_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_color_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_ap_cfg(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_eg_color_dropstate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_shared_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_icos_mapping(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_port_usage_count(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_usage_counter(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_min_rate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_q_max_rate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_port_max_rate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_egress_q_pfc(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_egress_port_pfc(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_port_dropstate(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_port_drop_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_ppg_resume_limit(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_port_ppg_mapping(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_wac_pfc_state(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_wac_offset_profile(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_offset_profile(bf_dev_id_t devid, int pipe);
bf_status_t tm_ucli_reset_qac_qid_profile(bf_dev_id_t devid, int pipe);
bf_status_t bf_tm_ucli_power_on_reset_per_pipe_tbls(bf_dev_id_t devid,
                                                    int pipe);

bf_status_t tm_ucli_set_ddr_train(ucli_context_t *uc,
                                  int nz,
                                  int hex,
                                  bf_dev_id_t devid);

#endif  //__TM_TABLE_DISPLAY_H__
