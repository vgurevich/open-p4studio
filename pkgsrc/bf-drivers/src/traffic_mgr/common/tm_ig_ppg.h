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
 *    related to INGRESS TM PPG
 */

#ifndef __TM_PPG_H__
#define __TM_PPG_H__

#include <stdint.h>
#include <stdbool.h>

#include <bf_types/bf_types.h>
#include "tm_ctx.h"
#include "tm_error.h"

/*
 *
 *             This file has 2 sections.
 *            ___________________________
 *
 *     Section 1: PPG Accessor functions exported for use by other functions
 *
 *  - First section has all data structures
 *     related to PPG.
 *  - Any data element from these structure are NOT directly accessed any
 *     where in driver.
 *  - Accessor functions are provided in its corresponding .c file to
 *     read/write to data elements. Function declarations of these accessror
 *     functions are also in this file.
 *
 *     Section 2: PPG HW programming functions
 *
 *  - For every PPG field/attribute that has presence in HW, implement
 *    corresponding HW read/write funciton.
 *  - Setup a table of funtion pointers so that accessor functions
 *    will use these functions to program/read from HW
 *  - These HW access function will be invoked any time accessor functions
 *    modifies fields that has presence in HW or attempts to read (with option
 *    to also read from HW)
 */

/* Section 1 */

/* PPG related data strcutres */

/* ppg thresholds */
typedef struct _bf_tm_ppg_thres {
  /* Any time a new field (that has presence in HW)
   * is added, also add accessor function tm_ig_ppg.c
   */
  bf_tm_thres_t min_limit;   // Min limit. Usage beyond this will spill
                             // into shared resources
  bf_tm_thres_t skid_limit;  // Triggers drops even for PFC traffic above
                             // this limit. Applies only to non default PPGs
  bf_tm_thres_t app_limit;   // Triggers Xoff/Drop beyond this limit
                             // Xoff for PFC enabled PPGs, Drop for
                             // default ppgs and non pfc enabled ppgs.
  bf_tm_thres_t ppg_hyst;    // Will be converted to hysterisis value
                             // Index in table offset_profile[]
                             // corresponding to this value is used
                             // to program csr_mem_wac_ppg_shr_lmt
  uint8_t hyst_index;        // PPG hysteresis in shared pool is
                             // one of the 32 (tofino) possible hyst
} bf_tm_ppg_thres_t;

typedef struct _bf_tm_ppg_cfg {
  /* Any time a new field (that has presence in HW)
   * is added, also add accessor function tm_ig_ppg.c
   */
  uint8_t app_poolid;  // Shared poolid to which the ppg maps
  bool is_dynamic;     // Dynamic thresholds or static
  uint8_t baf;         // If dynamic, burst factor
  bool fast_recover_mode;
  uint16_t icos_mask;  // Multiple iCoS from the 'port'
  bool is_pfc;
} bf_tm_ppg_cfg_t;

typedef struct _bf_tm_ppg {
  bool in_use;     // When true, PPG is assigned to (port, icos)
  uint8_t p_pipe;  // physical Pipe#
  uint8_t l_pipe;  // logical Pipe#
  uint8_t
      d_pipe;  // Die local pipe_id since wac_pipe per die is only 4 dimension
  bool is_default_ppg;
  uint16_t ppg;   // PPG#
  uint8_t port;   // Only (1:1) relation ppg <-->port mapping
                  // or multiple ports cannot use single PPG
  uint8_t uport;  // API level port which could be same as port or diferent
                  // based on the
                  // Tofino Chip Family

  bf_tm_ppg_cfg_t ppg_cfg;
  bf_tm_ppg_thres_t thresholds;

  // Cached Counters per PPG.
  tm_cache_counter_node_list_t *counter_state_list;
} bf_tm_ppg_t;
// Memory : sizeof(bf_tm_ppg_t) * per_pipe_ppg_cnt * active_pipes

// Compose a PPG handler from its pipe_id, local port, and PPG#
// l_port is an 'user' device local port, i.e. 0..71 + 72 mirror port on TF-3.
#define BF_TM_PPG_HANDLE(l_pipe, l_port, ppg_id) \
  ((((bf_tm_ppg_hdl)(l_port)) << 16) |           \
   (((((bf_tm_ppg_hdl)(l_pipe)) & 0xf)) << 12) | \
   (((bf_tm_ppg_hdl)(ppg_id)) & 0xfff))

// Use this macro while bf_tm_port_t has no its mirror_port value set.
// The uport should be after TM_IS_PORT_INVALID(uport, ctx) check.
// The uport is an 'user device port' with pipe id.
#define BF_TM_IS_MIRROR_PORT(ctx, uport) \
  (BF_TM_IS_MIRROR_LOCALPORT(            \
      ctx,                               \
      DEV_PORT_TO_LOCAL_PORT(            \
          lld_sku_map_devport_from_user_to_device(ctx->devid, uport))))

// lport is a local 'device port', e.g. on TF3 it is 0..35 and 36 for mirror.
#define BF_TM_IS_MIRROR_LOCALPORT(ctx, lport)    \
  ((lport >= (ctx->tm_cfg.mirror_port_start)) && \
   (lport < (ctx->tm_cfg.mirror_port_start + ctx->tm_cfg.mirror_port_cnt)))

// Prototypes of accessor functions
#define BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ppg_set_##field(bf_dev_id_t, bf_tm_ppg_t *, argtype)

#define BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ppg_get_##field(                  \
      bf_dev_id_t, bf_tm_ppg_t *, argtype *, argtype *)

#define BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_ppg_get_##field(bf_dev_id_t, bf_tm_ppg_t *, argtype *)

#define BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(field) \
  bf_tm_status_t bf_tm_ppg_clear_##field(bf_dev_id_t, bf_tm_ppg_t *)
bf_tm_status_t bf_tm_alloc_ppg(bf_dev_id_t, bf_dev_port_t, bf_tm_ppg_hdl *);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(min_limit, bf_tm_thres_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(min_limit, bf_tm_thres_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(skid_limit, bf_tm_thres_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(skid_limit, bf_tm_thres_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(app_limit, bf_tm_thres_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(app_limit, bf_tm_thres_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(ppg_hyst, bf_tm_thres_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(ppg_hyst, bf_tm_thres_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(ppg_hyst_index, uint8_t);

BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(app_poolid, uint32_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(app_poolid, uint32_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(is_dynamic, bool);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(is_dynamic, bool);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(baf, uint8_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(baf, uint8_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(icos_mask, uint16_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(icos_mask, uint16_t);
BF_TM_PPG_WR_ACCESSOR_FUNC_PROTO(pfc_treatment, bool);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(pfc_treatment, bool);

BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(drop_counter, uint64_t);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(drop_state, bool);
BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(gmin_usage_counter, uint32_t);
BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(shared_usage_counter, uint32_t);
BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(skid_usage_counter, uint32_t);
BF_TM_PPG_CNTR_ACCESSOR_FUNC_PROTO(wm_counter, uint32_t);

BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(drop_counter);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(watermark);
bf_tm_status_t bf_tm_wac_pipe_get_buffer_full_drop_counter(bf_dev_id_t,
                                                           bf_dev_pipe_t,
                                                           uint64_t *);
bf_tm_status_t bf_tm_wac_pipe_clear_buffer_full_drop_counter(bf_dev_id_t,
                                                             bf_dev_pipe_t);
bf_status_t bf_tm_ppg_set_cache_counters(bf_dev_id_t dev, bf_tm_ppg_hdl ppg);

BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(fast_recovery_mode, bool);
BF_TM_PPG_RD_ACCESSOR_FUNC_PROTO(resume_limit, uint32_t);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(drop_state);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(gmin_usage_counter);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(shared_usage_counter);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(skid_usage_counter);
BF_TM_PPG_CLR_CNTR_ACCESSOR_FUNC_PROTO(resume_limit);

bf_tm_status_t bf_tm_ppg_get_defaults(bf_dev_id_t dev_id,
                                      bf_tm_ppg_t *ppg,
                                      bf_tm_ppg_defaults_t *def);
/* Section 2 */

/* PPG related HW read/write functions; Two functions (read, write)
 * corresponding to each ppg attribute/field.
 * The function pointers below are used in accessor ppg function above.
 */

/*
 *     Function Pointers to program HW
 */
typedef bf_tm_status_t (*bf_tm_ppg_thres_wr_fptr)(bf_dev_id_t,
                                                  const bf_tm_ppg_t *);
typedef bf_tm_status_t (*bf_tm_ppg_thres_rd_fptr)(bf_dev_id_t, bf_tm_ppg_t *);

/* ppg thresholds programming HW API table */
typedef struct _bf_tm_ppg_thres_hw_funcs {
  bf_tm_ppg_thres_wr_fptr min_limit_wr_fptr;  // csr_mem_wac_ppg_min_lmt
  bf_tm_ppg_thres_rd_fptr min_limit_rd_fptr;
  bf_tm_ppg_thres_wr_fptr skid_limit_wr_fptr;  // csr_mem_wac_ppg_hdr_lmt
  bf_tm_ppg_thres_rd_fptr skid_limit_rd_fptr;
  bf_tm_ppg_thres_wr_fptr app_limit_wr_fptr;  // csr_mem_wac_ppg_shr_lmt
  bf_tm_ppg_thres_rd_fptr app_limit_rd_fptr;
  bf_tm_ppg_thres_wr_fptr hyst_wr_fptr;  // Index into offset_profile[]
  bf_tm_ppg_thres_rd_fptr hyst_rd_fptr;  // corresponding to this
                                         // value is to program
                                         // csr_mem_wac_ppg_shr_lmt
} bf_tm_ppg_thres_hw_funcs_tbl;

typedef bf_tm_status_t (*bf_tm_wac_wr_fptr)(bf_dev_id_t, const bf_tm_ppg_t *);
typedef bf_tm_status_t (*bf_tm_wac_wr_fptr2)(bf_dev_id_t, bf_tm_ppg_t *);
typedef bf_tm_status_t (*bf_tm_wac_rd_fptr)(bf_dev_id_t, bf_tm_ppg_t *);
typedef bf_tm_status_t (*bf_tm_wac_rd_fptr2)(bf_dev_id_t);
typedef bf_tm_status_t (*bf_tm_wac_rd_fptr3)(bf_dev_id_t,
                                             bf_tm_ppg_t *,
                                             uint32_t *);

typedef bf_tm_status_t (*bf_tm_wac_cntr_fptr)(bf_dev_id_t,
                                              bf_tm_ppg_t *,
                                              uint64_t *);
typedef bf_tm_status_t (*bf_tm_wac_state_fptr)(bf_dev_id_t,
                                               bf_tm_ppg_t *,
                                               bool *);
typedef bf_tm_status_t (*bf_tm_wac_cntr_fptr2)(bf_dev_id_t,
                                               bf_dev_pipe_t,
                                               uint64_t *);
typedef bf_tm_status_t (*bf_tm_wac_cntr_fptr3)(bf_dev_id_t, bf_dev_pipe_t);

typedef bf_tm_status_t (*bf_tm_ppg_defaults_rd_fptr)(bf_dev_id_t,
                                                     bf_tm_ppg_t *,
                                                     bf_tm_ppg_defaults_t *);

typedef struct _bf_tm_ppg_cfg_hw_funcs {
  bf_tm_wac_wr_fptr2 app_poolid_wr_fptr;  // csr_mem_wac_port_ppg_mapping
  bf_tm_wac_rd_fptr app_poolid_rd_fptr;
  bf_tm_wac_wr_fptr is_dynamic_wr_fptr;  // csr_mem_wac_ppg_shr_lmt
  bf_tm_wac_rd_fptr is_dynamic_rd_fptr;
  bf_tm_wac_wr_fptr baf_wr_fptr;  // csr_mem_wac_ppg_shr_lmt
  bf_tm_wac_rd_fptr baf_rd_fptr;
  bf_tm_wac_rd_fptr icos_mask_rd_fptr;
  bf_tm_wac_wr_fptr pfc_wr_fptr;  // csr_mem_wac_port_pfc_en
  bf_tm_wac_rd_fptr pfc_rd_fptr;
  bf_tm_wac_rd_fptr fast_recovery_rd_fptr;
  bf_tm_wac_rd_fptr3 resume_lmt_rd_fptr;
  bf_tm_wac_wr_fptr2 resume_lmt_clr_fptr;
  bf_tm_wac_cntr_fptr ppg_drop_cntr_rd_fptr;
  bf_tm_wac_state_fptr ppg_drop_state_rd_fptr;
  bf_tm_wac_wr_fptr2 ppg_drop_state_clr_fptr;
  bf_tm_wac_cntr_fptr ppg_gmin_usage_cntr_rd_fptr;
  bf_tm_wac_wr_fptr2 ppg_gmin_usage_cntr_clr_fptr;
  bf_tm_wac_cntr_fptr ppg_shared_usage_cntr_rd_fptr;
  bf_tm_wac_wr_fptr2 ppg_shared_usage_cntr_clr_fptr;
  bf_tm_wac_cntr_fptr ppg_skid_usage_cntr_rd_fptr;
  bf_tm_wac_wr_fptr2 ppg_skid_usage_cntr_clr_fptr;
  bf_tm_wac_cntr_fptr ppg_wm_cntr_rd_fptr;
  bf_tm_wac_wr_fptr2 ppg_wm_clr_fptr;
  bf_tm_wac_cntr_fptr2 wac_buffer_full_cntr_rd_fptr;
  bf_tm_wac_cntr_fptr3 wac_buffer_full_cntr_clr_fptr;

  bf_tm_wac_rd_fptr ppg_allocation_fptr;
  bf_tm_wac_rd_fptr2 restore_wac_offset_profile_fptr;
  bf_tm_wac_wr_fptr2 ppg_drop_cntr_clr_fptr;

  bf_tm_ppg_defaults_rd_fptr defaults_rd_fptr;
} bf_tm_ppg_cfg_hw_funcs_tbl;

/* Carving values / macros / APIs */

/* Default Thresholds values */

/* Default PPG Assignment */

/* Useful to restore config from hardware */
bf_tm_status_t bf_tm_ppg_get_allocation(bf_dev_id_t dev_id, bf_tm_ppg_t *ppg);
bf_tm_status_t bf_tm_restore_wac_offset_profile(bf_dev_id_t dev_id);

#endif
