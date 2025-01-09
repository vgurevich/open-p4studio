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
 *    related to INGRESS TM POOLS
 */

#ifndef __TM_IG_POOLS_H__
#define __TM_IG_POOLS_H__

#include <stdint.h>
#include <stdbool.h>

#include <bf_types/bf_types.h>

#include "tm_ctx.h"

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

/* per app/shared pool across all pipes */
typedef struct _bf_tm_ig_shared_pool_thres {
  bf_tm_thres_t red_limit;
  bf_tm_thres_t yel_limit;
  bf_tm_thres_t green_limit;
  uint32_t pfc_limit[BF_TM_MAX_PFC_LEVELS];

} bf_tm_ig_shared_pool_thres_t;

typedef struct _bf_tm_ig_spool {
  bool color_drop_en;
  bf_tm_ig_shared_pool_thres_t threshold;
} bf_tm_ig_spool_t; /* Applies to entire TM (across all pipes) on ingress */

typedef struct _bf_tm_ig_thres {
  uint32_t dod_limit;
  uint32_t glb_min_limit;
  uint32_t skid_limit;  // Beyond this limit, PFC traffic is dropped.
  uint32_t skid_hyst;   // will be used as out-of-drop state.
  uint32_t glb_cell_limit;
  bool glb_cell_limit_enable;
} bf_tm_ig_gpool_t;

typedef struct _bf_tm_ig_pool {
  bf_tm_ig_gpool_t gpool;  // Global ingress pools

  // Resume limits apply to Shared pool
  // Single resume value for all pools for a given color
  bf_tm_thres_t red_hyst;
  bf_tm_thres_t yel_hyst;
  bf_tm_thres_t green_hyst;
  bf_tm_ig_spool_t *spool;  // #app-pools/shared pools
} bf_tm_ig_pool_t;

// Prototypes of accessor functions

#define BF_TM_IG_POOL_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ig_spool_set_##field(                 \
      bf_dev_id_t, uint8_t, bf_tm_ig_pool_t *, argtype)

#define BF_TM_IG_POOL_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ig_spool_get_##field(                 \
      bf_dev_id_t, uint8_t, bf_tm_ig_pool_t *, argtype *, argtype *)

#define BF_TM_IG_POOL_WR_ACCESSOR_PROTO(field, ...) \
  bf_tm_status_t bf_tm_ig_spool_set_##field(__VA_ARGS__)

#define BF_TM_IG_POOL_RD_ACCESSOR_PROTO(field, ...) \
  bf_tm_status_t bf_tm_ig_spool_get_##field(__VA_ARGS__)

BF_TM_IG_POOL_WR_ACCESSOR_FUNC_PROTO(red_limit, bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_FUNC_PROTO(red_limit, bf_tm_thres_t);
BF_TM_IG_POOL_WR_ACCESSOR_FUNC_PROTO(yel_limit, bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_FUNC_PROTO(yel_limit, bf_tm_thres_t);
BF_TM_IG_POOL_WR_ACCESSOR_FUNC_PROTO(green_limit, bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_FUNC_PROTO(green_limit, bf_tm_thres_t);

BF_TM_IG_POOL_WR_ACCESSOR_PROTO(red_hyst,
                                bf_dev_id_t,
                                bf_tm_ig_pool_t *,
                                bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_PROTO(
    red_hyst, bf_dev_id_t, bf_tm_ig_pool_t *, bf_tm_thres_t *, bf_tm_thres_t *);
BF_TM_IG_POOL_WR_ACCESSOR_PROTO(yel_hyst,
                                bf_dev_id_t,
                                bf_tm_ig_pool_t *,
                                bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_PROTO(
    yel_hyst, bf_dev_id_t, bf_tm_ig_pool_t *, bf_tm_thres_t *, bf_tm_thres_t *);
BF_TM_IG_POOL_WR_ACCESSOR_PROTO(green_hyst,
                                bf_dev_id_t,
                                bf_tm_ig_pool_t *,
                                bf_tm_thres_t);
BF_TM_IG_POOL_RD_ACCESSOR_PROTO(green_hyst,
                                bf_dev_id_t,
                                bf_tm_ig_pool_t *,
                                bf_tm_thres_t *,
                                bf_tm_thres_t *);

BF_TM_IG_POOL_WR_ACCESSOR_PROTO(
    color_drop, bf_dev_id_t, uint8_t, bf_tm_ig_pool_t *, bool);
BF_TM_IG_POOL_RD_ACCESSOR_PROTO(
    color_drop, bf_dev_id_t, uint8_t, bf_tm_ig_pool_t *, bool *, bool *);
bf_tm_status_t bf_tm_ig_spool_set_pfc_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            uint8_t pfc_level,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit);
bf_tm_status_t bf_tm_ig_spool_get_pfc_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            uint8_t pfc_level,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit);

#define BF_TM_IG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ig_spool_get_##field(bf_dev_id_t, uint8_t, argtype)

BF_TM_IG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(pool_usage, uint32_t *);
BF_TM_IG_SPOOL_CNTR_ACCESSOR_FUNC_PROTO(pool_wm, uint32_t *);

#define BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ig_gpool_set_##field(                  \
      bf_dev_id_t, bf_tm_ig_pool_t *, argtype)

#define BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ig_gpool_get_##field(                  \
      bf_dev_id_t, bf_tm_ig_pool_t *, argtype *, argtype *)

BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(skid_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(skid_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(skid_hyst, bf_tm_thres_t);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(skid_hyst, bf_tm_thres_t);
BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(glb_min_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(glb_min_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(dod_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(dod_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(glb_cell_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(glb_cell_limit, bf_tm_thres_t);
BF_TM_IG_GPOOL_WR_ACCESSOR_FUNC_PROTO(glb_cell_limit_state, bool);
BF_TM_IG_GPOOL_RD_ACCESSOR_FUNC_PROTO(glb_cell_limit_state, bool);

bf_tm_status_t bf_tm_uc_ct_size_set(bf_dev_id_t, uint32_t);
bf_tm_status_t bf_tm_mc_ct_size_set(bf_dev_id_t, uint32_t);
bf_tm_status_t bf_tm_uc_ct_size_get(bf_dev_id_t, uint32_t *, uint32_t *);
bf_tm_status_t bf_tm_mc_ct_size_get(bf_dev_id_t, uint32_t *, uint32_t *);

bf_tm_status_t bf_tm_ig_spool_clear_pool_wm(bf_dev_id_t devid, uint8_t poolid);
bf_tm_status_t bf_tm_ig_spool_color_drop_state_get(bf_dev_id_t,
                                                   bf_tm_color_t,
                                                   uint32_t *);
bf_tm_status_t bf_tm_ig_spool_color_drop_state_clear(bf_dev_id_t dev,
                                                     bf_tm_color_t color);
bf_tm_status_t bf_tm_pool_defaults_get(bf_dev_id_t dev,
                                       bf_tm_app_pool_t *pool,
                                       bf_tm_pool_defaults_t *def);

/* Section 2 */
/*
 * Function Pointers to program HW
 */

typedef bf_tm_status_t (*bf_tm_ig_spool_wr_fptr)(bf_dev_id_t,
                                                 uint8_t,
                                                 bf_tm_ig_spool_t *);
typedef bf_tm_status_t (*bf_tm_ig_spool_rd_fptr)(bf_dev_id_t,
                                                 uint8_t,
                                                 bf_tm_ig_spool_t *);
typedef bf_tm_status_t (*bf_tm_ig_hyst_wr_fptr)(bf_dev_id_t, bf_tm_ig_pool_t *);
typedef bf_tm_status_t (*bf_tm_ig_hyst_rd_fptr)(bf_dev_id_t, bf_tm_ig_pool_t *);

typedef bf_tm_status_t (*bf_tm_ig_spool_pfc_wr_fptr)(bf_dev_id_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     bf_tm_ig_spool_t *);
typedef bf_tm_status_t (*bf_tm_ig_spool_pfc_rd_fptr)(bf_dev_id_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     bf_tm_ig_spool_t *);

typedef bf_tm_status_t (*bf_tm_ig_gpool_wr_fptr)(bf_dev_id_t,
                                                 bf_tm_ig_gpool_t *);
typedef bf_tm_status_t (*bf_tm_ig_gpool_rd_fptr)(bf_dev_id_t,
                                                 bf_tm_ig_gpool_t *);

typedef bf_tm_status_t (*bf_tm_ig_spool_cntr_fptr)(bf_dev_id_t,
                                                   uint8_t,
                                                   uint32_t *);
typedef bf_tm_status_t (*bf_tm_ig_spool_cntr_clr_fptr)(bf_dev_id_t, uint8_t);
typedef bf_tm_status_t (*bf_tm_ct_size_wr_fptr)(bf_dev_id_t, uint32_t);
typedef bf_tm_status_t (*bf_tm_ct_size_rd_fptr)(bf_dev_id_t, uint32_t *);
typedef bf_tm_status_t (*bf_tm_ig_spool_color_drop_state_fptr)(bf_dev_id_t,
                                                               bf_tm_color_t,
                                                               uint32_t *);
typedef bf_tm_status_t (*bf_tm_ig_spool_color_drop_state_clr_fptr)(
    bf_dev_id_t, bf_tm_color_t);
typedef bf_tm_status_t (*bf_tm_pool_defaults_rd_fptr)(bf_dev_id_t,
                                                      bf_tm_app_pool_t *,
                                                      bf_tm_pool_defaults_t *);

typedef struct _bf_tm_ig_pool_hw_funcs {
  bf_tm_ig_spool_wr_fptr red_limit_wr_fptr;
  bf_tm_ig_spool_rd_fptr red_limit_rd_fptr;
  bf_tm_ig_hyst_wr_fptr red_hyst_wr_fptr;
  bf_tm_ig_hyst_rd_fptr red_hyst_rd_fptr;
  bf_tm_ig_spool_wr_fptr yel_limit_wr_fptr;
  bf_tm_ig_spool_rd_fptr yel_limit_rd_fptr;
  bf_tm_ig_hyst_wr_fptr yel_hyst_wr_fptr;
  bf_tm_ig_hyst_rd_fptr yel_hyst_rd_fptr;
  bf_tm_ig_spool_wr_fptr green_limit_wr_fptr;
  bf_tm_ig_spool_rd_fptr green_limit_rd_fptr;
  bf_tm_ig_hyst_wr_fptr green_hyst_wr_fptr;
  bf_tm_ig_hyst_rd_fptr green_hyst_rd_fptr;
  bf_tm_ig_spool_pfc_wr_fptr pfc_limit_wr_fptr;
  bf_tm_ig_spool_pfc_rd_fptr pfc_limit_rd_fptr;
  bf_tm_ig_spool_wr_fptr color_drop_en_wr_fptr;
  bf_tm_ig_spool_rd_fptr color_drop_en_rd_fptr;
  bf_tm_ig_gpool_wr_fptr glb_min_limit_wr_fptr;
  bf_tm_ig_gpool_rd_fptr glb_min_limit_rd_fptr;
  bf_tm_ig_gpool_wr_fptr dod_limit_wr_fptr;
  bf_tm_ig_gpool_rd_fptr dod_limit_rd_fptr;
  bf_tm_ig_gpool_wr_fptr skid_limit_wr_fptr;
  bf_tm_ig_gpool_rd_fptr skid_limit_rd_fptr;
  bf_tm_ig_gpool_wr_fptr skid_hyst_wr_fptr;
  bf_tm_ig_gpool_rd_fptr skid_hyst_rd_fptr;
  bf_tm_ig_spool_cntr_fptr usage_cntr_fptr;
  bf_tm_ig_spool_cntr_fptr wm_cntr_fptr;
  bf_tm_ig_spool_cntr_clr_fptr wm_clr_fptr;
  bf_tm_ct_size_wr_fptr uc_ct_size_wr_fptr;
  bf_tm_ct_size_rd_fptr uc_ct_size_rd_fptr;
  bf_tm_ct_size_wr_fptr mc_ct_size_wr_fptr;
  bf_tm_ct_size_rd_fptr mc_ct_size_rd_fptr;
  bf_tm_ig_spool_color_drop_state_fptr color_drop_state_rd_fptr;
  bf_tm_ig_spool_color_drop_state_clr_fptr color_drop_st_clr_fptr;
  bf_tm_pool_defaults_rd_fptr defaults_rd_fptr;
  bf_tm_ig_gpool_wr_fptr glb_cell_limit_wr_fptr;
  bf_tm_ig_gpool_rd_fptr glb_cell_limit_rd_fptr;
  bf_tm_ig_gpool_wr_fptr glb_cell_limit_en_wr_fptr;
  bf_tm_ig_gpool_rd_fptr glb_cell_limit_en_rd_fptr;
} bf_tm_ig_pool_hw_funcs_tbl;

#endif
