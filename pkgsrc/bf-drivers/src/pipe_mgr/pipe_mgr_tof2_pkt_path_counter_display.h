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


#ifndef __TOF2_PKT_PATH_DISPLAY_H__
#define __TOF2_PKT_PATH_DISPLAY_H__

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

#include "pipe_mgr_int.h"

#if DVM_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
void pipe_mgr_tof2_pkt_path_display_iprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bool non_zero,
                                                  bf_dev_id_t devid,
                                                  int pipe,
                                                  int pg_port,
                                                  int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_eprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bool non_zero,
                                                  bf_dev_id_t devid,
                                                  int pipe,
                                                  int pg_port,
                                                  int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_ipb_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_epb_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_ebuf_counter(ucli_context_t *uc,
                                                 int hex,
                                                 bool non_zero,
                                                 bf_dev_id_t devid,
                                                 int pipe,
                                                 int pg_port,
                                                 int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_s2p_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_p2s_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_display_idprsr_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe);
void pipe_mgr_tof2_pkt_path_display_edprsr_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe);
void pipe_mgr_tof2_pkt_path_display_pmarb_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe);
void pipe_mgr_tof2_pkt_path_display_pipe_counter(ucli_context_t *uc,
                                                 int hex,
                                                 bf_dev_id_t devid,
                                                 int pipe);
void pipe_mgr_tof2_nonzero_chip_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid);
void pipe_mgr_tof2_per_chip_counter_cli(ucli_context_t *uc, bf_dev_id_t devid);
void pipe_mgr_tof2_nonzero_pipe_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid,
                                            int pipe);
void pipe_mgr_tof2_pipe_counter_cli(ucli_context_t *uc,
                                    bf_dev_id_t devid,
                                    int pipe);
void pipe_mgr_tof2_nonzero_pipe_and_port_counter_cli(
    ucli_context_t *uc, bf_dev_id_t devid, int pipe, int port, int port_end);
void pipe_mgr_tof2_pipe_and_port_counter_cli(
    ucli_context_t *uc, bf_dev_id_t devid, int pipe, int port, int port_end);
#endif
void pipe_mgr_tof2_pkt_path_clear_iprsr_counter(bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_eprsr_counter(bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_ipb_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_epb_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_ebuf_counter(bf_dev_id_t devid,
                                               int pipe,
                                               int pg_port,
                                               int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_s2p_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_p2s_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end);
void pipe_mgr_tof2_pkt_path_clear_idprsr_counter(rmt_dev_info_t *dev_info,
                                                 int pipe);
void pipe_mgr_tof2_pkt_path_clear_edprsr_counter(rmt_dev_info_t *dev_info,
                                                 int pipe);
void pipe_mgr_tof2_pkt_path_clear_pmarb_counter(bf_dev_id_t devid, int pipe);
void pipe_mgr_tof2_pkt_path_clear_pipe_counter(bf_dev_id_t devid, int pipe);
void pipe_mgr_tof2_pkt_path_clear_all_counter(bf_dev_id_t devid, int pipe);

// read counter
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_iprsr_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_idprsr_read_counter(
    rmt_dev_info_t *dev_info, int p, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_epb_ebuf_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_eprsr_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_edprsr_read_counter(
    rmt_dev_info_t *dev_info, int p, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_pmarb_read_counter(
    bf_dev_id_t devid, int p, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_s2p_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_p2s_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_ipb_read_counter(
    bf_dev_id_t devid, int p, int pg_port, int port_numb, int *count);
#endif
