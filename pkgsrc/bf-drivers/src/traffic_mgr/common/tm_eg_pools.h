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
 *    related to EGRESS TM Pools
 */

#ifndef __TM_EG_POOLS_H__
#define __TM_EG_POOLS_H__

/*
 *                  Egress TM Pools
 */

/*
 *
 * Section 1: Ingress Pool Accessor functions exported for use by other
 *functions
 *
 *  - First section has all data structures  related to pools.
 *  - Accessor functions are provided in its corresponding .c file to
 *    read/write to data elements.
 *
 * Section 2: HW programming functions to update Ingress Pool
 *
 *  - For every Pool field/attribute that has presence in HW, implement
 *    corresponding HW read/write funciton.
 *  - Setup a table of funtion pointers so that accessor functions
 *    will use these functions to program/read from HW
 *  - These HW access function will be invoked any time accessor functions
 *    modifies fields that has presence in HW or attempts to read (with option
 *    to also read from HW)
 */

/* Section 1 */

/* per app/shared pool across all egress pipes */
typedef struct _bf_tm_eg_shared_pool_thres {
  bf_tm_thres_t green_limit;
  bf_tm_thres_t yel_limit;
  bf_tm_thres_t red_limit;
} bf_tm_eg_app_pool_thres_t;

typedef struct _bf_tm_eg_spool {
  bool color_drop_en;  // qac_glb_config
  bf_tm_eg_app_pool_thres_t threshold;
} bf_tm_eg_spool_t;

typedef struct _bf_tm_eg_cfg {
  bf_tm_thres_t dod_limit;
  bf_tm_thres_t fifo_limit[BF_PIPE_COUNT][BF_PRE_FIFO_COUNT];
  bf_tm_thres_t uc_cutthrough_limit;
  bf_tm_thres_t mc_cutthrough_limit;
} bf_tm_eg_gpool_t;

typedef struct _bf_tm_eg_pool {
  bf_tm_eg_gpool_t gpool;
  // Single resume limit for all pools for a given color
  bf_tm_thres_t green_hyst;
  bf_tm_thres_t yel_hyst;
  bf_tm_thres_t red_hyst;
  bf_tm_eg_spool_t *spool;
} bf_tm_eg_pool_t;

// Prototypes of accessor functions

#define BF_TM_EG_POOL_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_spool_set_##field(                 \
      bf_dev_id_t, uint8_t, bf_tm_eg_pool_t *, argtype)

#define BF_TM_EG_POOL_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_spool_get_##field(                 \
      bf_dev_id_t, uint8_t, bf_tm_eg_pool_t *, argtype *, argtype *)

#define BF_TM_EG_POOL_WR_ACCESSOR_RESUME_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_spool_set_##field(                        \
      bf_dev_id_t, bf_tm_eg_pool_t *, argtype)

#define BF_TM_EG_POOL_RD_ACCESSOR_RESUME_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_spool_get_##field(                        \
      bf_dev_id_t, bf_tm_eg_pool_t *, argtype *, argtype *)

BF_TM_EG_POOL_WR_ACCESSOR_FUNC_PROTO(color_drop, bool);
BF_TM_EG_POOL_RD_ACCESSOR_FUNC_PROTO(color_drop, bool);
BF_TM_EG_POOL_WR_ACCESSOR_FUNC_PROTO(red_limit, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_FUNC_PROTO(red_limit, bf_tm_thres_t);
BF_TM_EG_POOL_WR_ACCESSOR_RESUME_FUNC_PROTO(red_hyst, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_RESUME_FUNC_PROTO(red_hyst, bf_tm_thres_t);
BF_TM_EG_POOL_WR_ACCESSOR_FUNC_PROTO(yel_limit, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_FUNC_PROTO(yel_limit, bf_tm_thres_t);
BF_TM_EG_POOL_WR_ACCESSOR_RESUME_FUNC_PROTO(yel_hyst, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_RESUME_FUNC_PROTO(yel_hyst, bf_tm_thres_t);
BF_TM_EG_POOL_WR_ACCESSOR_FUNC_PROTO(green_limit, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_FUNC_PROTO(green_limit, bf_tm_thres_t);
BF_TM_EG_POOL_WR_ACCESSOR_RESUME_FUNC_PROTO(green_hyst, bf_tm_thres_t);
BF_TM_EG_POOL_RD_ACCESSOR_RESUME_FUNC_PROTO(green_hyst, bf_tm_thres_t);

#define BF_TM_EG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_spool_get_##field(bf_dev_id_t, uint8_t, argtype)

BF_TM_EG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(pool_usage, uint32_t *);
BF_TM_EG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(pool_wm, uint32_t *);

#define BF_TM_EG_GPOOL_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_gpool_set_##field(                  \
      bf_dev_id_t, bf_tm_eg_pool_t *, argtype)

#define BF_TM_EG_GPOOL_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_eg_gpool_get_##field(                  \
      bf_dev_id_t, bf_tm_eg_pool_t *, argtype *, argtype *)

bf_tm_status_t bf_tm_eg_gpool_set_fifo_limit(
    bf_dev_id_t, bf_tm_eg_pool_t *, bf_dev_pipe_t, uint8_t, bf_tm_thres_t);
bf_tm_status_t bf_tm_eg_gpool_get_fifo_limit(bf_dev_id_t,
                                             bf_tm_eg_pool_t *,
                                             bf_dev_pipe_t,
                                             uint8_t,
                                             bf_tm_thres_t *,
                                             bf_tm_thres_t *);

BF_TM_EG_GPOOL_WR_ACCESSOR_FUNC_PROTO(dod_limit, bf_tm_thres_t);
BF_TM_EG_GPOOL_RD_ACCESSOR_FUNC_PROTO(dod_limit, bf_tm_thres_t);

bf_tm_status_t bf_tm_eg_spool_clear_pool_wm(bf_dev_id_t devid, uint8_t poolid);
bf_tm_status_t bf_tm_eg_spool_get_buffer_drop_state(
    bf_dev_id_t devid,
    bf_tm_eg_buffer_drop_state_en drop_type,
    uint32_t *state);
bf_tm_status_t bf_tm_eg_spool_clear_buffer_drop_state(
    bf_dev_id_t devid, bf_tm_eg_buffer_drop_state_en drop_type);

/* Section 2 */
/*
 * Function Pointers to program HW
 */

typedef bf_tm_status_t (*bf_tm_eg_spool_wr_fptr)(bf_dev_id_t,
                                                 uint8_t,
                                                 bf_tm_eg_spool_t *);
typedef bf_tm_status_t (*bf_tm_eg_spool_rd_fptr)(bf_dev_id_t,
                                                 uint8_t,
                                                 bf_tm_eg_spool_t *);

typedef bf_tm_status_t (*bf_tm_eg_hyst_wr_fptr)(bf_dev_id_t, bf_tm_eg_pool_t *);
typedef bf_tm_status_t (*bf_tm_eg_hyst_rd_fptr)(bf_dev_id_t, bf_tm_eg_pool_t *);

typedef bf_tm_status_t (*bf_tm_eg_gpool_wr_fptr)(bf_dev_id_t,
                                                 bf_tm_eg_gpool_t *);
typedef bf_tm_status_t (*bf_tm_eg_gpool_rd_fptr)(bf_dev_id_t,
                                                 bf_tm_eg_gpool_t *);

typedef bf_tm_status_t (*bf_tm_eg_gpool_fifo_limit_wr_fptr)(bf_dev_id_t,
                                                            bf_dev_pipe_t,
                                                            uint8_t,
                                                            bf_tm_thres_t);
typedef bf_tm_status_t (*bf_tm_eg_gpool_fifo_limit_rd_fptr)(bf_dev_id_t,
                                                            bf_dev_pipe_t,
                                                            uint8_t,
                                                            bf_tm_thres_t *);

typedef bf_tm_status_t (*bf_tm_eg_spool_cntr_fptr)(bf_dev_id_t,
                                                   uint8_t,
                                                   uint32_t *);
typedef bf_tm_status_t (*bf_tm_eg_spool_cntr_fptr2)(
    bf_dev_id_t, bf_tm_eg_buffer_drop_state_en, uint32_t *);
typedef bf_tm_status_t (*bf_tm_eg_spool_clr_cntr_fptr2)(
    bf_dev_id_t, bf_tm_eg_buffer_drop_state_en);

typedef bf_tm_status_t (*bf_tm_eg_spool_cntr_clr_fptr)(bf_dev_id_t, uint8_t);

typedef struct _bf_tm_eg_pool_hw_funcs {
  bf_tm_eg_spool_wr_fptr red_limit_wr_fptr;
  bf_tm_eg_spool_rd_fptr red_limit_rd_fptr;
  bf_tm_eg_hyst_wr_fptr red_hyst_wr_fptr;
  bf_tm_eg_hyst_rd_fptr red_hyst_rd_fptr;
  bf_tm_eg_spool_wr_fptr yel_limit_wr_fptr;
  bf_tm_eg_spool_rd_fptr yel_limit_rd_fptr;
  bf_tm_eg_hyst_wr_fptr yel_hyst_wr_fptr;
  bf_tm_eg_hyst_rd_fptr yel_hyst_rd_fptr;
  bf_tm_eg_spool_wr_fptr green_limit_wr_fptr;
  bf_tm_eg_spool_rd_fptr green_limit_rd_fptr;
  bf_tm_eg_hyst_wr_fptr green_hyst_wr_fptr;
  bf_tm_eg_hyst_rd_fptr green_hyst_rd_fptr;
  bf_tm_eg_spool_wr_fptr color_drop_en_wr_fptr;
  bf_tm_eg_spool_rd_fptr color_drop_en_rd_fptr;
  bf_tm_eg_gpool_fifo_limit_wr_fptr fifo_limit_wr_fptr;
  bf_tm_eg_gpool_fifo_limit_rd_fptr fifo_limit_rd_fptr;
  bf_tm_eg_gpool_wr_fptr dod_limit_wr_fptr;
  bf_tm_eg_gpool_rd_fptr dod_limit_rd_fptr;
  bf_tm_eg_spool_cntr_fptr usage_cntr_fptr;
  bf_tm_eg_spool_cntr_fptr wm_cntr_fptr;
  bf_tm_eg_spool_cntr_clr_fptr wm_clr_fptr;
  bf_tm_eg_spool_cntr_fptr2 eg_buffer_dropstate_fptr;
  bf_tm_eg_spool_clr_cntr_fptr2 eg_buf_drop_st_clr_fptr;
} bf_tm_eg_pool_hw_funcs_tbl;

#endif
