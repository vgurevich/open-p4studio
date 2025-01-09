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


/*
 *    This file contains all data strcutures and parameters
 *    related to EGRESS TM
 */

#ifndef __TM_PIPE_H__
#define __TM_PIPE_H__

#include <stdint.h>
#include <stdbool.h>

#include "tm_eg_pools.h"
#include "tm_port.h"
#include "tm_queue.h"

/*
 *                  Egress Pipe
 */

typedef struct _bf_tm_eg_pipe_thres {
  uint8_t p_pipe;
  uint8_t l_pipe;
  bf_tm_thres_t epipe_limit;
  bf_tm_thres_t epipe_resume_limit;
  uint8_t ifg_compensation;
  bf_tm_eg_q_t *neg_mirror_dest;
  uint32_t qac_pipe_config;
} bf_tm_eg_pipe_t;

typedef bf_tm_status_t (*bf_tm_pipe_wr_fptr)(bf_dev_id_t,
                                             bf_dev_pipe_t,
                                             uint32_t);
typedef bf_tm_status_t (*bf_tm_pipe_wr_fptr1)(bf_dev_id_t, bf_dev_pipe_t);
typedef bf_tm_status_t (*bf_tm_pipe_rd_fptr)(bf_dev_id_t,
                                             bf_dev_pipe_t,
                                             uint32_t *);
typedef bf_tm_status_t (*bf_tm_pipe_cntr_fptr)(bf_dev_id_t,
                                               bf_dev_pipe_t,
                                               uint64_t *);
typedef bf_tm_status_t (*bf_tm_pipe_wr_fptr2)(bf_dev_id_t,
                                              bf_tm_eg_pipe_t *,
                                              bool);
typedef bf_tm_status_t (*bf_tm_pipe_rd_fptr1)(bf_dev_id_t,
                                              bf_tm_eg_pipe_t *,
                                              bool *);

typedef bf_status_t (*bf_tm_pipe_defaults_rd_fptr)(bf_dev_id_t,
                                                   bf_tm_eg_pipe_t *,
                                                   bf_tm_pipe_defaults_t *);

typedef bf_tm_status_t (*bf_tm_drr_train_wr_fptr)(bf_dev_id_t);

typedef struct _bf_tm_pipe_hw_funcs {
  bf_tm_pipe_wr_fptr pipe_limit_wr_fptr;
  bf_tm_pipe_rd_fptr pipe_limit_rd_fptr;
  bf_tm_pipe_wr_fptr pipe_hyst_wr_fptr;
  bf_tm_pipe_rd_fptr pipe_hyst_rd_fptr;
  bf_tm_pipe_cntr_fptr pipe_cntr_get_total_cells;
  bf_tm_pipe_cntr_fptr pipe_cntr_get_total_pkts;
  bf_tm_pipe_cntr_fptr pipe_get_uc_ct_count;
  bf_tm_pipe_cntr_fptr pipe_get_mc_ct_count;
  bf_tm_pipe_wr_fptr2 qstat_report_mode_wr_fptr;
  bf_tm_pipe_rd_fptr1 qstat_report_mode_rd_fptr;
  bf_tm_pipe_wr_fptr1 pipe_cntr_total_cells_clear_fptr;
  bf_tm_pipe_wr_fptr1 pipe_cntr_total_packets_clear_fptr;
  bf_tm_pipe_wr_fptr1 pipe_cntr_uc_ct_clear_fptr;
  bf_tm_pipe_wr_fptr1 pipe_cntr_mc_ct_clear_fptr;
  bf_tm_pipe_wr_fptr2 pipe_defd_port_en_wr_fptr;
  bf_tm_pipe_rd_fptr1 pipe_defd_port_en_rd_fptr;
  bf_tm_pipe_defaults_rd_fptr pipe_get_defaults_fptr;
} bf_tm_pipe_hw_funcs_tbl;

bf_status_t bf_tm_pipe_get_descriptor(bf_dev_id_t,
                                      bf_dev_pipe_t,
                                      bf_tm_eg_pipe_t **);
bf_status_t bf_tm_pipe_set_limit(bf_dev_id_t, bf_tm_eg_pipe_t *, uint32_t);
bf_status_t bf_tm_pipe_set_hyst(bf_dev_id_t, bf_tm_eg_pipe_t *, uint32_t);
bf_status_t bf_tm_pipe_get_limit(bf_dev_id_t,
                                 bf_tm_eg_pipe_t *,
                                 uint32_t *,
                                 uint32_t *);
bf_status_t bf_tm_pipe_get_hyst(bf_dev_id_t,
                                bf_tm_eg_pipe_t *,
                                uint32_t *,
                                uint32_t *);

bf_status_t bf_tm_pipe_get_total_in_cell_count(bf_dev_id_t,
                                               bf_dev_pipe_t,
                                               uint64_t *);
bf_status_t bf_tm_pipe_clear_total_in_cell_count(bf_dev_id_t, bf_dev_pipe_t);

bf_status_t bf_tm_pipe_get_total_in_pkt_count(bf_dev_id_t,
                                              bf_dev_pipe_t,
                                              uint64_t *);
bf_status_t bf_tm_pipe_clear_total_in_packet_count(bf_dev_id_t, bf_dev_pipe_t);

bf_status_t bf_tm_pipe_get_uc_ct_count(bf_dev_id_t, bf_dev_pipe_t, uint64_t *);
bf_status_t bf_tm_pipe_clear_uc_ct_count(bf_dev_id_t, bf_dev_pipe_t);

bf_status_t bf_tm_pipe_get_mc_ct_count(bf_dev_id_t, bf_dev_pipe_t, uint64_t *);
bf_status_t bf_tm_pipe_clear_mc_ct_count(bf_dev_id_t, bf_dev_pipe_t);

bf_status_t bf_tm_set_qstat_report_mode(bf_dev_id_t dev,
                                        bf_tm_eg_pipe_t *pipe,
                                        bool mode);
bf_status_t bf_tm_get_qstat_report_mode(bf_dev_id_t dev,
                                        bf_tm_eg_pipe_t *pipe,
                                        bool *mode);

bf_status_t bf_tm_set_deflection_port_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool enable);
bf_status_t bf_tm_get_deflection_port_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool *enable);

bf_status_t bf_tm_pipe_get_defaults(bf_dev_id_t devid,
                                    bf_tm_eg_pipe_t *pipe,
                                    bf_tm_pipe_defaults_t *defaults);

#endif
