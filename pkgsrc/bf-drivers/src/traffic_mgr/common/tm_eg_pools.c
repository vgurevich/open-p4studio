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


// This file implements abstracted (from Tofino/Tofino-lite/..) APIs
// to manage POOLS in Egress TM

#include <string.h>
#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <target-sys/bf_sal/bf_sys_intf.h>





static bf_tm_eg_pool_hw_funcs_tbl g_eg_pool_hw_fptr_tbl;

bf_tm_status_t bf_tm_eg_spool_set_red_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.red_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->threshold.red_limit = limit;

  if (g_eg_pool_hw_fptr_tbl.red_limit_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.red_limit_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_red_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t spool_thres;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  *sw_limit = (spool + poolid)->threshold.red_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.red_limit_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.red_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.red_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_red_hyst(bf_dev_id_t devid,
                                           bf_tm_eg_pool_t *eg_pool,
                                           bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(limit, eg_pool->red_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  eg_pool->red_hyst = limit;

  if (g_eg_pool_hw_fptr_tbl.red_hyst_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.red_hyst_wr_fptr(devid, eg_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_red_hyst(bf_dev_id_t devid,
                                           bf_tm_eg_pool_t *eg_pool,
                                           bf_tm_thres_t *sw_limit,
                                           bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pool_t spool_thres;

  *sw_limit = eg_pool->red_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.red_hyst_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.red_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.red_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_yel_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.yel_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->threshold.yel_limit = limit;

  if (g_eg_pool_hw_fptr_tbl.yel_limit_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.yel_limit_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_yel_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t spool_thres;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  *sw_limit = (spool + poolid)->threshold.yel_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.yel_limit_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.yel_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.yel_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_yel_hyst(bf_dev_id_t devid,
                                           bf_tm_eg_pool_t *eg_pool,
                                           bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(limit, eg_pool->yel_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  eg_pool->yel_hyst = limit;

  if (g_eg_pool_hw_fptr_tbl.yel_hyst_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.yel_hyst_wr_fptr(devid, eg_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_yel_hyst(bf_dev_id_t devid,
                                           bf_tm_eg_pool_t *eg_pool,
                                           bf_tm_thres_t *sw_limit,
                                           bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pool_t spool_thres;

  *sw_limit = eg_pool->yel_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.yel_hyst_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.yel_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.yel_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_green_limit(bf_dev_id_t devid,
                                              uint8_t poolid,
                                              bf_tm_eg_pool_t *eg_pool,
                                              bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t *spool = eg_pool->spool;
  bf_tm_thres_t current_limit;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.green_limit, g_tm_ctx[devid])) {
    return (rc);
  }

  /* Remember the current limit */
  current_limit = spool->threshold.green_limit;
  spool->threshold.green_limit = limit;

  if (g_eg_pool_hw_fptr_tbl.green_limit_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.green_limit_wr_fptr(devid, poolid, spool);

    if (rc != BF_TM_EOK) {
      /*
       * Since setting new limit in HW failed, update the SW copy
       * with old value.
       */
      spool->threshold.green_limit = current_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_green_limit(bf_dev_id_t devid,
                                              uint8_t poolid,
                                              bf_tm_eg_pool_t *eg_pool,
                                              bf_tm_thres_t *sw_limit,
                                              bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t spool_thres;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  *sw_limit = (spool + poolid)->threshold.green_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.green_limit_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.green_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.green_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_green_hyst(bf_dev_id_t devid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(limit, eg_pool->green_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  eg_pool->green_hyst = limit;

  if (g_eg_pool_hw_fptr_tbl.green_hyst_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.green_hyst_wr_fptr(devid, eg_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_green_hyst(bf_dev_id_t devid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bf_tm_thres_t *sw_limit,
                                             bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pool_t spool_thres;

  *sw_limit = eg_pool->green_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.green_hyst_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.green_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.green_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_set_color_drop(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(enable, spool->color_drop_en, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->color_drop_en = enable;

  if (g_eg_pool_hw_fptr_tbl.color_drop_en_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.color_drop_en_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_color_drop(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bool *sw_drop_en,
                                             bool *hw_drop_en) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_spool_t shared_pool;
  bf_tm_eg_spool_t *spool = eg_pool->spool;

  *sw_drop_en = (spool + poolid)->color_drop_en;

  if (TM_IS_TARGET_ASIC(devid) && hw_drop_en &&
      g_eg_pool_hw_fptr_tbl.color_drop_en_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.color_drop_en_rd_fptr(
        devid, poolid, &shared_pool);
    if (BF_TM_IS_OK(rc)) {
      *hw_drop_en = shared_pool.color_drop_en;
    }
  }
  return (rc);
}

//   Global Pools

bf_tm_status_t bf_tm_eg_gpool_set_dod_limit(bf_dev_id_t devid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_gpool_t *gpool = &(eg_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->dod_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->dod_limit = limit;
  if (g_eg_pool_hw_fptr_tbl.dod_limit_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.dod_limit_wr_fptr(devid, gpool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_gpool_get_dod_limit(bf_dev_id_t devid,
                                            bf_tm_eg_pool_t *eg_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_gpool_t gpool;
  *sw_limit = eg_pool->gpool.dod_limit;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.dod_limit_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.dod_limit_rd_fptr(devid, &gpool);
    *hw_limit = gpool.dod_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_gpool_set_fifo_limit(bf_dev_id_t devid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bf_dev_pipe_t pipe,
                                             uint8_t fifo,
                                             bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_gpool_t *gpool = &(eg_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, gpool->fifo_limit[pipe][fifo], g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->fifo_limit[pipe][fifo] = limit;
  if (g_eg_pool_hw_fptr_tbl.fifo_limit_wr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.fifo_limit_wr_fptr(devid, pipe, fifo, limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_gpool_get_fifo_limit(bf_dev_id_t devid,
                                             bf_tm_eg_pool_t *eg_pool,
                                             bf_dev_pipe_t pipe,
                                             uint8_t fifo,
                                             bf_tm_thres_t *sw_limit,
                                             bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  *sw_limit = eg_pool->gpool.fifo_limit[pipe][fifo];
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_eg_pool_hw_fptr_tbl.fifo_limit_rd_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.fifo_limit_rd_fptr(devid, pipe, fifo, hw_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_pool_usage(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_eg_pool_hw_fptr_tbl.usage_cntr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.usage_cntr_fptr(devid, poolid, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_pool_wm(bf_dev_id_t devid,
                                          uint8_t poolid,
                                          uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_eg_pool_hw_fptr_tbl.wm_cntr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.wm_cntr_fptr(devid, poolid, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_clear_pool_wm(bf_dev_id_t devid, uint8_t poolid) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_eg_pool_hw_fptr_tbl.wm_clr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.wm_clr_fptr(devid, poolid);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_get_buffer_drop_state(
    bf_dev_id_t devid,
    bf_tm_eg_buffer_drop_state_en drop_type,
    uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_eg_pool_hw_fptr_tbl.eg_buffer_dropstate_fptr) {
    rc =
        g_eg_pool_hw_fptr_tbl.eg_buffer_dropstate_fptr(devid, drop_type, state);
  }
  return (rc);
}

bf_tm_status_t bf_tm_eg_spool_clear_buffer_drop_state(
    bf_dev_id_t devid, bf_tm_eg_buffer_drop_state_en drop_type) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_eg_pool_hw_fptr_tbl.eg_buf_drop_st_clr_fptr) {
    rc = g_eg_pool_hw_fptr_tbl.eg_buf_drop_st_clr_fptr(devid, drop_type);
  }
  return (rc);
}

#define BF_TM_EG_POOL_HW_FTBL_LIMIT_SET(red_limit_fptr,                \
                                        red_hyst_fptr,                 \
                                        yel_limit_fptr,                \
                                        yel_hyst_fptr,                 \
                                        green_limit_fptr,              \
                                        green_hyst_fptr,               \
                                        color_drop_en_fptr,            \
                                        fifo_limit_fptr,               \
                                        dod_limit_fptr,                \
                                        wm_fptr,                       \
                                        eg_buffer_drop_st)             \
  {                                                                    \
    g_eg_pool_hw_fptr_tbl.red_limit_wr_fptr = red_limit_fptr;          \
    g_eg_pool_hw_fptr_tbl.red_hyst_wr_fptr = red_hyst_fptr;            \
    g_eg_pool_hw_fptr_tbl.yel_limit_wr_fptr = yel_limit_fptr;          \
    g_eg_pool_hw_fptr_tbl.yel_hyst_wr_fptr = yel_hyst_fptr;            \
    g_eg_pool_hw_fptr_tbl.green_limit_wr_fptr = green_limit_fptr;      \
    g_eg_pool_hw_fptr_tbl.green_hyst_wr_fptr = green_hyst_fptr;        \
    g_eg_pool_hw_fptr_tbl.color_drop_en_wr_fptr = color_drop_en_fptr;  \
    g_eg_pool_hw_fptr_tbl.fifo_limit_wr_fptr = fifo_limit_fptr;        \
    g_eg_pool_hw_fptr_tbl.dod_limit_wr_fptr = dod_limit_fptr;          \
    g_eg_pool_hw_fptr_tbl.wm_clr_fptr = wm_fptr;                       \
    g_eg_pool_hw_fptr_tbl.eg_buf_drop_st_clr_fptr = eg_buffer_drop_st; \
  }

#define BF_TM_EG_POOL_HW_FTBL_LIMIT_GET(red_limit_fptr,                  \
                                        red_hyst_fptr,                   \
                                        yel_limit_fptr,                  \
                                        yel_hyst_fptr,                   \
                                        green_limit_fptr,                \
                                        green_hyst_fptr,                 \
                                        color_drop_en_fptr,              \
                                        fifo_limit_fptr,                 \
                                        dod_limit_fptr,                  \
                                        usage_fptr,                      \
                                        wm_fptr,                         \
                                        eg_buf_dropst_fptr)              \
  {                                                                      \
    g_eg_pool_hw_fptr_tbl.red_limit_rd_fptr = red_limit_fptr;            \
    g_eg_pool_hw_fptr_tbl.red_hyst_rd_fptr = red_hyst_fptr;              \
    g_eg_pool_hw_fptr_tbl.yel_limit_rd_fptr = yel_limit_fptr;            \
    g_eg_pool_hw_fptr_tbl.yel_hyst_rd_fptr = yel_hyst_fptr;              \
    g_eg_pool_hw_fptr_tbl.green_limit_rd_fptr = green_limit_fptr;        \
    g_eg_pool_hw_fptr_tbl.green_hyst_rd_fptr = green_hyst_fptr;          \
    g_eg_pool_hw_fptr_tbl.color_drop_en_rd_fptr = color_drop_en_fptr;    \
    g_eg_pool_hw_fptr_tbl.fifo_limit_rd_fptr = fifo_limit_fptr;          \
    g_eg_pool_hw_fptr_tbl.dod_limit_rd_fptr = dod_limit_fptr;            \
    g_eg_pool_hw_fptr_tbl.usage_cntr_fptr = usage_fptr;                  \
    g_eg_pool_hw_fptr_tbl.wm_cntr_fptr = wm_fptr;                        \
    g_eg_pool_hw_fptr_tbl.eg_buffer_dropstate_fptr = eg_buf_dropst_fptr; \
  }

void bf_tm_eg_pool_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_EG_POOL_HW_FTBL_LIMIT_SET(
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  BF_TM_EG_POOL_HW_FTBL_LIMIT_GET(
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

static void bf_tm_eg_pool_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_SET(bf_tm_tofino_eg_spool_set_red_limit,
                                    bf_tm_tofino_eg_spool_set_red_hyst,
                                    bf_tm_tofino_eg_spool_set_yel_limit,
                                    bf_tm_tofino_eg_spool_set_yel_hyst,
                                    bf_tm_tofino_eg_spool_set_green_limit,
                                    bf_tm_tofino_eg_spool_set_green_hyst,
                                    bf_tm_tofino_eg_spool_set_color_drop_en,
                                    bf_tm_tofino_eg_gpool_set_fifo_limit,
                                    bf_tm_tofino_eg_gpool_set_dod_limit,
                                    bf_tm_tofino_eg_spool_clear_wm,
                                    bf_tm_tofino_eg_buffer_drop_state_clear);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_SET(bf_tm_tof2_eg_spool_set_red_limit,
                                    bf_tm_tof2_eg_spool_set_red_hyst,
                                    bf_tm_tof2_eg_spool_set_yel_limit,
                                    bf_tm_tof2_eg_spool_set_yel_hyst,
                                    bf_tm_tof2_eg_spool_set_green_limit,
                                    bf_tm_tof2_eg_spool_set_green_hyst,
                                    bf_tm_tof2_eg_spool_set_color_drop_en,
                                    bf_tm_tof2_eg_gpool_set_fifo_limit,
                                    bf_tm_tof2_eg_gpool_set_dod_limit,
                                    bf_tm_tof2_eg_spool_clear_wm,
                                    bf_tm_tof2_eg_buffer_drop_state_clear);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_SET(bf_tm_tof3_eg_spool_set_red_limit,
                                    bf_tm_tof3_eg_spool_set_red_hyst,
                                    bf_tm_tof3_eg_spool_set_yel_limit,
                                    bf_tm_tof3_eg_spool_set_yel_hyst,
                                    bf_tm_tof3_eg_spool_set_green_limit,
                                    bf_tm_tof3_eg_spool_set_green_hyst,
                                    bf_tm_tof3_eg_spool_set_color_drop_en,
                                    bf_tm_tof3_eg_gpool_set_fifo_limit,
                                    bf_tm_tof3_eg_gpool_set_dod_limit,
                                    bf_tm_tof3_eg_spool_clear_wm,
                                    bf_tm_tof3_eg_buffer_drop_state_clear);
  }















  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

static void bf_tm_eg_pool_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_GET(bf_tm_tofino_eg_spool_get_red_limit,
                                    bf_tm_tofino_eg_spool_get_red_hyst,
                                    bf_tm_tofino_eg_spool_get_yel_limit,
                                    bf_tm_tofino_eg_spool_get_yel_hyst,
                                    bf_tm_tofino_eg_spool_get_green_limit,
                                    bf_tm_tofino_eg_spool_get_green_hyst,
                                    bf_tm_tofino_eg_spool_get_color_drop_en,
                                    bf_tm_tofino_eg_gpool_get_fifo_limit,
                                    bf_tm_tofino_eg_gpool_get_dod_limit,
                                    bf_tm_tofino_eg_spool_get_usage,
                                    bf_tm_tofino_eg_spool_get_wm,
                                    bf_tm_tofino_eg_buffer_drop_state);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_GET(bf_tm_tof2_eg_spool_get_red_limit,
                                    bf_tm_tof2_eg_spool_get_red_hyst,
                                    bf_tm_tof2_eg_spool_get_yel_limit,
                                    bf_tm_tof2_eg_spool_get_yel_hyst,
                                    bf_tm_tof2_eg_spool_get_green_limit,
                                    bf_tm_tof2_eg_spool_get_green_hyst,
                                    bf_tm_tof2_eg_spool_get_color_drop_en,
                                    bf_tm_tof2_eg_gpool_get_fifo_limit,
                                    bf_tm_tof2_eg_gpool_get_dod_limit,
                                    bf_tm_tof2_eg_spool_get_usage,
                                    bf_tm_tof2_eg_spool_get_wm,
                                    bf_tm_tof2_eg_buffer_drop_state);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_EG_POOL_HW_FTBL_LIMIT_GET(bf_tm_tof3_eg_spool_get_red_limit,
                                    bf_tm_tof3_eg_spool_get_red_hyst,
                                    bf_tm_tof3_eg_spool_get_yel_limit,
                                    bf_tm_tof3_eg_spool_get_yel_hyst,
                                    bf_tm_tof3_eg_spool_get_green_limit,
                                    bf_tm_tof3_eg_spool_get_green_hyst,
                                    bf_tm_tof3_eg_spool_get_color_drop_en,
                                    bf_tm_tof3_eg_gpool_get_fifo_limit,
                                    bf_tm_tof3_eg_gpool_get_dod_limit,
                                    bf_tm_tof3_eg_spool_get_usage,
                                    bf_tm_tof3_eg_spool_get_wm,
                                    bf_tm_tof3_eg_buffer_get_drop_state);
  }
















  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

void bf_tm_eg_pool_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_eg_pool_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_eg_pool_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_eg_pool_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_eg_pool_null_hw_ftbl(tm_ctx);
  bf_tm_eg_pool_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_eg_pool_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->eg_pool && tm_ctx->eg_pool->spool) {
    bf_sys_free(tm_ctx->eg_pool->spool);
  }
  if (tm_ctx->eg_pool) {
    bf_sys_free(tm_ctx->eg_pool);
  }
}

bf_tm_status_t bf_tm_init_eg_pool(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->eg_pool) {
    bf_tm_eg_pool_delete(tm_ctx);
  }
  tm_ctx->eg_pool = bf_sys_calloc(1, sizeof(bf_tm_eg_pool_t));
  if (tm_ctx->eg_pool) {
    tm_ctx->eg_pool->spool = bf_sys_calloc(
        1, sizeof(bf_tm_eg_spool_t) * tm_ctx->tm_cfg.shared_pool_cnt);
    if (!tm_ctx->eg_pool->spool) {
      bf_tm_eg_pool_delete(tm_ctx);
      return (BF_NO_SYS_RESOURCES);
    }
  } else {
    bf_tm_eg_pool_delete(tm_ctx);
    return (BF_NO_SYS_RESOURCES);
  }

  bf_tm_eg_pool_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
