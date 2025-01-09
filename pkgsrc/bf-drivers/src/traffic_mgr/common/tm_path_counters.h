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
 *    related to PIPE DROP/ERROR/DISCARD Counters
 */

#ifndef __TM_PM_COUNTERS_H__
#define __TM_PM_COUNTERS_H__

#include <stdint.h>
#include <stdbool.h>

#include <bf_types/bf_types.h>
#include "tm_ctx.h"
#include "tm_error.h"

// bf_tm_blklvl_cntrs_t*
bf_tm_status_t bf_tm_blklvl_get_drop_cntrs(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           bf_tm_blklvl_cntrs_t *blk_cntrs);
// bf_tm_pre_fifo_cntrs_t
bf_tm_status_t bf_tm_pre_fifo_get_drop_cntrs(
    bf_dev_id_t dev, bf_tm_pre_fifo_cntrs_t *fifo_drop_cntrs);

/*
 *      Clear Registers
 */
bf_tm_status_t bf_tm_blklvl_clr_drop_cntrs(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint32_t clear_mask);

// bf_tm_pre_fifo_cntrs_t
bf_tm_status_t bf_tm_pre_fifo_clr_drop_cntrs(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint32_t fifo);

/*
 * Function pointers to program HW/ASIC.
 * Depending on ASIC version/type populate correct read/write/clr fptr.
 * Blocklevel function pointers are defined with this
 */

typedef bf_tm_status_t (*bf_tm_blklvl_cntr_rd_fptr)(bf_dev_id_t,
                                                    bf_dev_pipe_t,
                                                    bf_tm_blklvl_cntrs_t *);

typedef bf_tm_status_t (*bf_tm_blklvl_cntr_clr_fptr)(bf_dev_id_t,
                                                     bf_dev_pipe_t,
                                                     uint32_t);

typedef bf_tm_status_t (*bf_tm_blklvl_cntr_rd_fptr2)(bf_dev_id_t,
                                                     bf_tm_pre_fifo_cntrs_t *);

typedef bf_tm_status_t (*bf_tm_blklvl_cntr_clr_fptr2)(bf_dev_id_t);

typedef bf_tm_status_t (*bf_tm_counter_rd_fptr)(bf_dev_id_t,
                                                bf_dev_pipe_t,
                                                uint64_t *);
typedef bf_tm_status_t (*bf_tm_counter_rd_fptr2)(bf_dev_id_t,
                                                 bf_dev_pipe_t,
                                                 uint64_t *,
                                                 uint64_t *);

typedef struct _bf_tm_path_cntr_hw_funcs_tbl {
  bf_tm_blklvl_cntr_rd_fptr blklvl_cntrs_rd_fptr;
  bf_tm_blklvl_cntr_clr_fptr blklvl_cntrs_clr_fptr;
  bf_tm_blklvl_cntr_rd_fptr2 pre_fifo_cntrs_rd_fptr;
  bf_tm_blklvl_cntr_clr_fptr pre_fifo_cntrs_clr_fptr;
  bf_tm_counter_rd_fptr valid_sop_cntr_rd_fptr;
  bf_tm_counter_rd_fptr ph_lost_cntr_rd_fptr;
  bf_tm_counter_rd_fptr cpu_copy_cntr_rd_fptr;
  bf_tm_counter_rd_fptr total_ph_processed_rd_fptr;
  bf_tm_counter_rd_fptr total_copy_cntr_rd_fptr;
  bf_tm_counter_rd_fptr total_xid_prunes_cntr_rd_fptr;
  bf_tm_counter_rd_fptr total_yid_prunes_cntr_rd_fptr;
  bf_tm_counter_rd_fptr ph_in_use_cntr_rd_fptr;
  bf_tm_counter_rd_fptr2 clc_total_cntr_rd_fptr;
  bf_tm_counter_rd_fptr2 eport_total_cntr_rd_fptr;
  bf_tm_counter_rd_fptr pex_total_pkt_cntr_rd_fptr;
  bf_tm_counter_rd_fptr total_enqueued_cntr_rd_fptr;
  bf_tm_counter_rd_fptr total_dequeued_cntr_rd_fptr;
  bf_tm_counter_rd_fptr caa_used_blocks_cntr_rd_fptr;
  bf_tm_counter_rd_fptr psc_used_blocks_cntr_rd_fptr;
  bf_tm_counter_rd_fptr qid_enqueued_cntr_rd_fptr;
  bf_tm_counter_rd_fptr qid_dequeued_cntr_rd_fptr;
  bf_tm_counter_rd_fptr prc_total_qac_query_cntr_rd_fptr;
  bf_tm_counter_rd_fptr prc_total_qac_query_zero_cntr_rd_fptr;
  bf_tm_counter_rd_fptr prc_total_pex_cntr_rd_fptr;
  bf_tm_counter_rd_fptr prc_total_pex_zero_cntr_rd_fptr;

} bf_tm_path_cntr_hw_funcs_tbl_t;

#endif
