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
// to manage POOLS in Ingress TM

#include <string.h>
#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <target-sys/bf_sal/bf_sys_intf.h>





static bf_tm_ig_pool_hw_funcs_tbl g_ig_pool_hw_fptr_tbl;

bf_tm_status_t bf_tm_ig_spool_set_red_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.red_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->threshold.red_limit = limit;

  if (g_ig_pool_hw_fptr_tbl.red_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.red_limit_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_red_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t spool_thres;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  *sw_limit = (spool + poolid)->threshold.red_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.red_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.red_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.red_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_red_hyst(bf_dev_id_t devid,
                                           bf_tm_ig_pool_t *ig_pool,
                                           bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(limit, ig_pool->red_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  ig_pool->red_hyst = limit;

  if (g_ig_pool_hw_fptr_tbl.red_hyst_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.red_hyst_wr_fptr(devid, ig_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_red_hyst(bf_dev_id_t devid,
                                           bf_tm_ig_pool_t *ig_pool,
                                           bf_tm_thres_t *sw_limit,
                                           bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_pool_t spool_thres;

  *sw_limit = ig_pool->red_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.red_hyst_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.red_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.red_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_yel_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.yel_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->threshold.yel_limit = limit;

  if (g_ig_pool_hw_fptr_tbl.yel_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.yel_limit_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_yel_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t spool_thres;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  *sw_limit = (spool + poolid)->threshold.yel_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.yel_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.yel_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.yel_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_yel_hyst(bf_dev_id_t devid,
                                           bf_tm_ig_pool_t *ig_pool,
                                           bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(limit, ig_pool->yel_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  ig_pool->yel_hyst = limit;

  if (g_ig_pool_hw_fptr_tbl.yel_hyst_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.yel_hyst_wr_fptr(devid, ig_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_yel_hyst(bf_dev_id_t devid,
                                           bf_tm_ig_pool_t *ig_pool,
                                           bf_tm_thres_t *sw_limit,
                                           bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_pool_t spool_thres;

  *sw_limit = ig_pool->yel_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.yel_hyst_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.yel_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.yel_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_green_limit(bf_dev_id_t devid,
                                              uint8_t poolid,
                                              bf_tm_ig_pool_t *ig_pool,
                                              bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.green_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->threshold.green_limit = limit;

  if (g_ig_pool_hw_fptr_tbl.green_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.green_limit_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_green_limit(bf_dev_id_t devid,
                                              uint8_t poolid,
                                              bf_tm_ig_pool_t *ig_pool,
                                              bf_tm_thres_t *sw_limit,
                                              bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t spool_thres;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  *sw_limit = (spool + poolid)->threshold.green_limit;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.green_limit_rd_fptr) {
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ig_pool_hw_fptr_tbl.green_limit_rd_fptr(devid, poolid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.green_limit;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_green_hyst(bf_dev_id_t devid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(limit, ig_pool->green_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  ig_pool->green_hyst = limit;

  if (g_ig_pool_hw_fptr_tbl.green_hyst_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.green_hyst_wr_fptr(devid, ig_pool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_green_hyst(bf_dev_id_t devid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bf_tm_thres_t *sw_limit,
                                             bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_pool_t spool_thres;

  *sw_limit = ig_pool->green_hyst;

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.green_hyst_rd_fptr) {
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ig_pool_hw_fptr_tbl.green_hyst_rd_fptr(devid, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.green_hyst;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_color_drop(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(enable, spool->color_drop_en, g_tm_ctx[devid])) {
    return (rc);
  }
  spool->color_drop_en = enable;

  if (g_ig_pool_hw_fptr_tbl.color_drop_en_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.color_drop_en_wr_fptr(devid, poolid, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_color_drop(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bool *sw_drop_en,
                                             bool *hw_drop_en) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t shared_pool;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  *sw_drop_en = (spool + poolid)->color_drop_en;

  if (TM_IS_TARGET_ASIC(devid) && hw_drop_en &&
      g_ig_pool_hw_fptr_tbl.color_drop_en_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.color_drop_en_rd_fptr(
        devid, poolid, &shared_pool);
    if (BF_TM_IS_OK(rc)) {
      *hw_drop_en = shared_pool.color_drop_en;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_set_pfc_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            uint8_t pfc_level,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  spool += poolid;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, spool->threshold.pfc_limit[pfc_level], g_tm_ctx[devid])) {
    return (rc);
  }

  if (spool->threshold.green_limit <= limit) {
    // It not recommended to set PFC limit equal to or more than pool limit.
    // pool level PFC will not kick in.
    // Put a warning message.
    LOG_WARN(
        "Traffic Manager buffer pool %d pfc limit is higher or equal to pool "
        "size. PFC might not trigger based on pool usage",
        poolid);
  }
  spool->threshold.pfc_limit[pfc_level] = limit;

  if (g_ig_pool_hw_fptr_tbl.pfc_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.pfc_limit_wr_fptr(
        devid, poolid, pfc_level, spool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_pfc_limit(bf_dev_id_t devid,
                                            uint8_t poolid,
                                            uint8_t pfc_level,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_spool_t spool_thres;
  bf_tm_ig_spool_t *spool = ig_pool->spool;

  *sw_limit = (spool + poolid)->threshold.pfc_limit[pfc_level];

  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.pfc_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.pfc_limit_rd_fptr(
        devid, poolid, pfc_level, &spool_thres);
    if (BF_TM_IS_OK(rc)) {
      *hw_limit = spool_thres.threshold.pfc_limit[pfc_level];
    }
  }
  return (rc);
}

//   Global Pools

bf_tm_status_t bf_tm_ig_gpool_set_dod_limit(bf_dev_id_t devid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->dod_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->dod_limit = limit;
  if (g_ig_pool_hw_fptr_tbl.dod_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.dod_limit_wr_fptr(devid, gpool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_dod_limit(bf_dev_id_t devid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_limit = ig_pool->gpool.dod_limit;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.dod_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.dod_limit_rd_fptr(devid, &gpool);
    *hw_limit = gpool.dod_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_set_glb_min_limit(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_pool,
                                                bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->glb_min_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->glb_min_limit = limit;
  if (g_ig_pool_hw_fptr_tbl.glb_min_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_min_limit_wr_fptr(devid, gpool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_glb_min_limit(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_pool,
                                                bf_tm_thres_t *sw_limit,
                                                bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_limit = ig_pool->gpool.glb_min_limit;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.glb_min_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_min_limit_rd_fptr(devid, &gpool);
    *hw_limit = gpool.glb_min_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_set_skid_limit(bf_dev_id_t devid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->skid_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->skid_limit = limit;
  if (g_ig_pool_hw_fptr_tbl.skid_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.skid_limit_wr_fptr(devid, gpool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_skid_limit(bf_dev_id_t devid,
                                             bf_tm_ig_pool_t *ig_pool,
                                             bf_tm_thres_t *sw_limit,
                                             bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_limit = ig_pool->gpool.skid_limit;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.skid_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.skid_limit_rd_fptr(devid, &gpool);
    *hw_limit = gpool.skid_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_set_skid_hyst(bf_dev_id_t devid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->skid_hyst, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->skid_hyst = limit;
  if (g_ig_pool_hw_fptr_tbl.skid_hyst_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.skid_hyst_wr_fptr(devid, gpool);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_skid_hyst(bf_dev_id_t devid,
                                            bf_tm_ig_pool_t *ig_pool,
                                            bf_tm_thres_t *sw_limit,
                                            bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_limit = ig_pool->gpool.skid_hyst;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.skid_hyst_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.skid_hyst_rd_fptr(devid, &gpool);
    *hw_limit = gpool.skid_hyst;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_pool_usage(bf_dev_id_t devid,
                                             uint8_t poolid,
                                             uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ig_pool_hw_fptr_tbl.usage_cntr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.usage_cntr_fptr(devid, poolid, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_get_pool_wm(bf_dev_id_t devid,
                                          uint8_t poolid,
                                          uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ig_pool_hw_fptr_tbl.wm_cntr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.wm_cntr_fptr(devid, poolid, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_clear_pool_wm(bf_dev_id_t devid, uint8_t poolid) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ig_pool_hw_fptr_tbl.wm_clr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.wm_clr_fptr(devid, poolid);
  }
  return (rc);
}

bf_tm_status_t bf_tm_uc_ct_size_set(bf_dev_id_t dev, uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  g_tm_ctx[dev]->uc_ct_size = cells;
  if (g_ig_pool_hw_fptr_tbl.uc_ct_size_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.uc_ct_size_wr_fptr(dev, cells);
  }
  return (rc);
}

bf_tm_status_t bf_tm_mc_ct_size_set(bf_dev_id_t dev, uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  g_tm_ctx[dev]->mc_ct_size = cells;
  if (g_ig_pool_hw_fptr_tbl.mc_ct_size_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.mc_ct_size_wr_fptr(dev, cells);
  }
  return (rc);
}

bf_tm_status_t bf_tm_uc_ct_size_get(bf_dev_id_t dev,
                                    uint32_t *sw_cells,
                                    uint32_t *hw_cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  *sw_cells = g_tm_ctx[dev]->uc_ct_size;
  if (hw_cells && g_ig_pool_hw_fptr_tbl.uc_ct_size_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.uc_ct_size_rd_fptr(dev, hw_cells);
  }
  return (rc);
}

bf_tm_status_t bf_tm_mc_ct_size_get(bf_dev_id_t dev,
                                    uint32_t *sw_cells,
                                    uint32_t *hw_cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  *sw_cells = g_tm_ctx[dev]->mc_ct_size;
  if (hw_cells && g_ig_pool_hw_fptr_tbl.mc_ct_size_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.mc_ct_size_rd_fptr(dev, hw_cells);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_color_drop_state_get(bf_dev_id_t dev,
                                                   bf_tm_color_t color,
                                                   uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ig_pool_hw_fptr_tbl.color_drop_state_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.color_drop_state_rd_fptr(dev, color, state);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_spool_color_drop_state_clear(bf_dev_id_t dev,
                                                     bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ig_pool_hw_fptr_tbl.color_drop_st_clr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.color_drop_st_clr_fptr(dev, color);
  }
  return (rc);
}

bf_tm_status_t bf_tm_pool_defaults_get(bf_dev_id_t dev,
                                       bf_tm_app_pool_t *pool,
                                       bf_tm_pool_defaults_t *def) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ig_pool_hw_fptr_tbl.defaults_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.defaults_rd_fptr(dev, pool, def);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_set_glb_cell_limit(bf_dev_id_t devid,
                                                 bf_tm_ig_pool_t *ig_pool,
                                                 bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(limit, gpool->glb_cell_limit, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->glb_cell_limit = limit;
  if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_cell_limit_wr_fptr(devid, gpool);
  } else if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_wr_fptr == NULL) {
    return BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_glb_cell_limit(bf_dev_id_t devid,
                                                 bf_tm_ig_pool_t *ig_pool,
                                                 bf_tm_thres_t *sw_limit,
                                                 bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_limit = ig_pool->gpool.glb_cell_limit;
  if (TM_IS_TARGET_ASIC(devid) && hw_limit &&
      g_ig_pool_hw_fptr_tbl.glb_cell_limit_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_cell_limit_rd_fptr(devid, &gpool);
    *hw_limit = gpool.glb_cell_limit;
  } else if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_rd_fptr == NULL) {
    return BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_set_glb_cell_limit_state(bf_dev_id_t devid,
                                                       bf_tm_ig_pool_t *ig_pool,
                                                       bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t *gpool = &(ig_pool->gpool);
  if (TM_HITLESS_IS_CFG_MATCH(
          enable, gpool->glb_cell_limit_enable, g_tm_ctx[devid])) {
    return (rc);
  }
  gpool->glb_cell_limit_enable = enable;
  if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_wr_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_wr_fptr(devid, gpool);
  } else if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_wr_fptr == NULL) {
    return BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ig_gpool_get_glb_cell_limit_state(bf_dev_id_t devid,
                                                       bf_tm_ig_pool_t *ig_pool,
                                                       bool *sw_enable,
                                                       bool *hw_enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ig_gpool_t gpool;
  *sw_enable = ig_pool->gpool.glb_cell_limit_enable;
  if (TM_IS_TARGET_ASIC(devid) && hw_enable &&
      g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_rd_fptr) {
    rc = g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_rd_fptr(devid, &gpool);
    *hw_enable = gpool.glb_cell_limit_enable;
  } else if (g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_rd_fptr == NULL) {
    return BF_NOT_SUPPORTED;
  }
  return (rc);
}

#define BF_TM_IG_POOL_HW_FTBL_THRES_SET(red_limit_fptr,                       \
                                        red_hyst_fptr,                        \
                                        yel_limit_fptr,                       \
                                        yel_hyst_fptr,                        \
                                        green_limit_fptr,                     \
                                        green_hyst_fptr,                      \
                                        pfc_limit_fptr,                       \
                                        color_drop_en_fptr,                   \
                                        glb_min_limit_fptr,                   \
                                        dod_limit_fptr,                       \
                                        skid_limit_fptr,                      \
                                        skid_hyst_fptr,                       \
                                        uc_ct_fptr,                           \
                                        mc_ct_fptr,                           \
                                        wm_fptr,                              \
                                        color_drop_clr,                       \
                                        glb_cell_limit_fptr,                  \
                                        glb_cell_limit_en_fptr)               \
  {                                                                           \
    g_ig_pool_hw_fptr_tbl.red_limit_wr_fptr = red_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.red_hyst_wr_fptr = red_hyst_fptr;                   \
    g_ig_pool_hw_fptr_tbl.yel_limit_wr_fptr = yel_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.yel_hyst_wr_fptr = yel_hyst_fptr;                   \
    g_ig_pool_hw_fptr_tbl.green_limit_wr_fptr = green_limit_fptr;             \
    g_ig_pool_hw_fptr_tbl.green_hyst_wr_fptr = green_hyst_fptr;               \
    g_ig_pool_hw_fptr_tbl.pfc_limit_wr_fptr = pfc_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.color_drop_en_wr_fptr = color_drop_en_fptr;         \
    g_ig_pool_hw_fptr_tbl.glb_min_limit_wr_fptr = glb_min_limit_fptr;         \
    g_ig_pool_hw_fptr_tbl.dod_limit_wr_fptr = dod_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.skid_limit_wr_fptr = skid_limit_fptr;               \
    g_ig_pool_hw_fptr_tbl.skid_hyst_wr_fptr = skid_hyst_fptr;                 \
    g_ig_pool_hw_fptr_tbl.uc_ct_size_wr_fptr = uc_ct_fptr;                    \
    g_ig_pool_hw_fptr_tbl.mc_ct_size_wr_fptr = mc_ct_fptr;                    \
    g_ig_pool_hw_fptr_tbl.wm_clr_fptr = wm_fptr;                              \
    g_ig_pool_hw_fptr_tbl.color_drop_st_clr_fptr = color_drop_clr;            \
    g_ig_pool_hw_fptr_tbl.glb_cell_limit_wr_fptr = glb_cell_limit_fptr;       \
    g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_wr_fptr = glb_cell_limit_en_fptr; \
  }

#define BF_TM_IG_POOL_HW_FTBL_THRES_GET(red_limit_fptr,                       \
                                        red_hyst_fptr,                        \
                                        yel_limit_fptr,                       \
                                        yel_hyst_fptr,                        \
                                        green_limit_fptr,                     \
                                        green_hyst_fptr,                      \
                                        pfc_limit_fptr,                       \
                                        color_drop_en_fptr,                   \
                                        glb_min_limit_fptr,                   \
                                        dod_limit_fptr,                       \
                                        skid_limit_fptr,                      \
                                        skid_hyst_fptr,                       \
                                        usage_fptr,                           \
                                        wm_fptr,                              \
                                        uc_ct_fptr,                           \
                                        mc_ct_fptr,                           \
                                        color_drop_st_fptr,                   \
                                        def_rd_fptr,                          \
                                        glb_cell_limit_fptr,                  \
                                        glb_cell_limit_en_fptr)               \
  {                                                                           \
    g_ig_pool_hw_fptr_tbl.red_limit_rd_fptr = red_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.red_hyst_rd_fptr = red_hyst_fptr;                   \
    g_ig_pool_hw_fptr_tbl.yel_limit_rd_fptr = yel_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.yel_hyst_rd_fptr = yel_hyst_fptr;                   \
    g_ig_pool_hw_fptr_tbl.green_limit_rd_fptr = green_limit_fptr;             \
    g_ig_pool_hw_fptr_tbl.green_hyst_rd_fptr = green_hyst_fptr;               \
    g_ig_pool_hw_fptr_tbl.pfc_limit_rd_fptr = pfc_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.color_drop_en_rd_fptr = color_drop_en_fptr;         \
    g_ig_pool_hw_fptr_tbl.glb_min_limit_rd_fptr = glb_min_limit_fptr;         \
    g_ig_pool_hw_fptr_tbl.dod_limit_rd_fptr = dod_limit_fptr;                 \
    g_ig_pool_hw_fptr_tbl.skid_limit_rd_fptr = skid_limit_fptr;               \
    g_ig_pool_hw_fptr_tbl.skid_hyst_rd_fptr = skid_hyst_fptr;                 \
    g_ig_pool_hw_fptr_tbl.usage_cntr_fptr = usage_fptr;                       \
    g_ig_pool_hw_fptr_tbl.wm_cntr_fptr = wm_fptr;                             \
    g_ig_pool_hw_fptr_tbl.uc_ct_size_rd_fptr = uc_ct_fptr;                    \
    g_ig_pool_hw_fptr_tbl.mc_ct_size_rd_fptr = mc_ct_fptr;                    \
    g_ig_pool_hw_fptr_tbl.color_drop_state_rd_fptr = color_drop_st_fptr;      \
    g_ig_pool_hw_fptr_tbl.defaults_rd_fptr = def_rd_fptr;                     \
    g_ig_pool_hw_fptr_tbl.glb_cell_limit_rd_fptr = glb_cell_limit_fptr;       \
    g_ig_pool_hw_fptr_tbl.glb_cell_limit_en_rd_fptr = glb_cell_limit_en_fptr; \
  }

void bf_tm_ig_pool_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_IG_POOL_HW_FTBL_THRES_SET(NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
  BF_TM_IG_POOL_HW_FTBL_THRES_GET(NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
}

static void bf_tm_ig_pool_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_SET(
        bf_tm_tofino_ig_spool_set_red_limit,
        bf_tm_tofino_ig_spool_set_red_hyst,
        bf_tm_tofino_ig_spool_set_yel_limit,
        bf_tm_tofino_ig_spool_set_yel_hyst,
        bf_tm_tofino_ig_spool_set_green_limit,
        bf_tm_tofino_ig_spool_set_green_hyst,
        bf_tm_tofino_ig_spool_set_pfc_limit,
        bf_tm_tofino_ig_spool_set_color_drop_en,
        NULL,
        bf_tm_tofino_ig_gpool_set_dod_limit,
        bf_tm_tofino_ig_gpool_set_skid_limit,
        bf_tm_tofino_ig_gpool_set_skid_hyst,
        bf_tm_tofino_ig_gpool_set_uc_ct_size,
        bf_tm_tofino_ig_gpool_set_mc_ct_size,
        bf_tm_tofino_ig_spool_clear_wm,
        bf_tm_tofino_ig_spool_clear_color_drop_state,
        NULL,
        NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_SET(
        bf_tm_tof2_ig_spool_set_red_limit,
        bf_tm_tof2_ig_spool_set_red_hyst,
        bf_tm_tof2_ig_spool_set_yel_limit,
        bf_tm_tof2_ig_spool_set_yel_hyst,
        bf_tm_tof2_ig_spool_set_green_limit,
        bf_tm_tof2_ig_spool_set_green_hyst,
        bf_tm_tof2_ig_spool_set_pfc_limit,
        bf_tm_tof2_ig_spool_set_color_drop_en,
        bf_tm_tof2_ig_gpool_set_glb_min_limit,
        bf_tm_tof2_ig_gpool_set_dod_limit,
        bf_tm_tof2_ig_gpool_set_skid_limit,
        bf_tm_tof2_ig_gpool_set_skid_hyst,
        bf_tm_tof2_ig_gpool_set_uc_ct_size,
        bf_tm_tof2_ig_gpool_set_mc_ct_size,
        bf_tm_tof2_ig_spool_clear_wm,
        bf_tm_tof2_ig_spool_clear_color_drop_state,
        bf_tm_tof2_ig_gpool_set_glb_cell_limit,
        bf_tm_tof2_ig_gpool_set_glb_cell_limit_state);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_SET(
        bf_tm_tof3_ig_spool_set_red_limit,
        bf_tm_tof3_ig_spool_set_red_hyst,
        bf_tm_tof3_ig_spool_set_yel_limit,
        bf_tm_tof3_ig_spool_set_yel_hyst,
        bf_tm_tof3_ig_spool_set_green_limit,
        bf_tm_tof3_ig_spool_set_green_hyst,
        bf_tm_tof3_ig_spool_set_pfc_limit,
        bf_tm_tof3_ig_spool_set_color_drop_en,
        bf_tm_tof3_ig_gpool_set_glb_min_limit,
        bf_tm_tof3_ig_gpool_set_dod_limit,
        bf_tm_tof3_ig_gpool_set_skid_limit,
        bf_tm_tof3_ig_gpool_set_skid_hyst,
        bf_tm_tof3_ig_gpool_set_uc_ct_size,
        bf_tm_tof3_ig_gpool_set_mc_ct_size,
        bf_tm_tof3_ig_spool_clear_wm,
        bf_tm_tof3_ig_spool_clear_color_drop_state,
        bf_tm_tof3_ig_gpool_set_glb_cell_limit,
        bf_tm_tof3_ig_gpool_set_glb_cell_limit_state);
  }























  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

static void bf_tm_ig_pool_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_GET(bf_tm_tofino_ig_spool_get_red_limit,
                                    bf_tm_tofino_ig_spool_get_red_hyst,
                                    bf_tm_tofino_ig_spool_get_yel_limit,
                                    bf_tm_tofino_ig_spool_get_yel_hyst,
                                    bf_tm_tofino_ig_spool_get_green_limit,
                                    bf_tm_tofino_ig_spool_get_green_hyst,
                                    bf_tm_tofino_ig_spool_get_pfc_limit,
                                    bf_tm_tofino_ig_spool_get_color_drop_en,
                                    NULL,
                                    bf_tm_tofino_ig_gpool_get_dod_limit,
                                    bf_tm_tofino_ig_gpool_get_skid_limit,
                                    bf_tm_tofino_ig_gpool_get_skid_hyst,
                                    bf_tm_tofino_ig_spool_get_usage,
                                    bf_tm_tofino_ig_spool_get_wm,
                                    bf_tm_tofino_ig_gpool_get_uc_ct_size,
                                    bf_tm_tofino_ig_gpool_get_mc_ct_size,
                                    bf_tm_tofino_ig_spool_get_color_drop_state,
                                    bf_tm_tofino_pool_get_defaults,
                                    NULL,
                                    NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_GET(
        bf_tm_tof2_ig_spool_get_red_limit,
        bf_tm_tof2_ig_spool_get_red_hyst,
        bf_tm_tof2_ig_spool_get_yel_limit,
        bf_tm_tof2_ig_spool_get_yel_hyst,
        bf_tm_tof2_ig_spool_get_green_limit,
        bf_tm_tof2_ig_spool_get_green_hyst,
        bf_tm_tof2_ig_spool_get_pfc_limit,
        bf_tm_tof2_ig_spool_get_color_drop_en,
        bf_tm_tof2_ig_gpool_get_glb_min_limit,
        bf_tm_tof2_ig_gpool_get_dod_limit,
        bf_tm_tof2_ig_gpool_get_skid_limit,
        bf_tm_tof2_ig_gpool_get_skid_hyst,
        bf_tm_tof2_ig_spool_get_usage,
        bf_tm_tof2_ig_spool_get_wm,
        bf_tm_tof2_ig_gpool_get_uc_ct_size,
        bf_tm_tof2_ig_gpool_get_mc_ct_size,
        bf_tm_tof2_ig_spool_get_color_drop_state,
        bf_tm_tof2_pool_get_defaults,
        bf_tm_tof2_ig_gpool_get_glb_cell_limit,
        bf_tm_tof2_ig_gpool_get_glb_cell_limit_state);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_IG_POOL_HW_FTBL_THRES_GET(
        bf_tm_tof3_ig_spool_get_red_limit,
        bf_tm_tof3_ig_spool_get_red_hyst,
        bf_tm_tof3_ig_spool_get_yel_limit,
        bf_tm_tof3_ig_spool_get_yel_hyst,
        bf_tm_tof3_ig_spool_get_green_limit,
        bf_tm_tof3_ig_spool_get_green_hyst,
        bf_tm_tof3_ig_spool_get_pfc_limit,
        bf_tm_tof3_ig_spool_get_color_drop_en,
        bf_tm_tof3_ig_gpool_get_glb_min_limit,
        bf_tm_tof3_ig_gpool_get_dod_limit,
        bf_tm_tof3_ig_gpool_get_skid_limit,
        bf_tm_tof3_ig_gpool_get_skid_hyst,
        bf_tm_tof3_ig_spool_get_usage,
        bf_tm_tof3_ig_spool_get_wm,
        bf_tm_tof3_ig_gpool_get_uc_ct_size,
        bf_tm_tof3_ig_gpool_get_mc_ct_size,
        bf_tm_tof3_ig_spool_get_color_drop_state,
        bf_tm_tof3_pool_get_defaults,
        bf_tm_tof3_ig_gpool_get_glb_cell_limit,
        bf_tm_tof3_ig_gpool_get_glb_cell_limit_state);
  }

























  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

void bf_tm_ig_pool_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_ig_pool_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_ig_pool_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_ig_pool_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_ig_pool_null_hw_ftbl(tm_ctx);
  bf_tm_ig_pool_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_ig_pool_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->ig_pool) {
    if (tm_ctx->ig_pool->spool) {
      bf_sys_free(tm_ctx->ig_pool->spool);
    }
    bf_sys_free(tm_ctx->ig_pool);
  }
  tm_ctx->ig_pool = NULL;
}

bf_tm_status_t bf_tm_init_ig_pool(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->ig_pool) {
    bf_tm_ig_pool_delete(tm_ctx);
  }
  tm_ctx->ig_pool = bf_sys_calloc(1, sizeof(bf_tm_ig_pool_t));
  if (tm_ctx->ig_pool) {
    tm_ctx->ig_pool->spool = bf_sys_calloc(
        1, sizeof(bf_tm_ig_spool_t) * tm_ctx->tm_cfg.shared_pool_cnt);
    if (!tm_ctx->ig_pool->spool) {
      bf_tm_ig_pool_delete(tm_ctx);
      return (BF_NO_SYS_RESOURCES);
    }
  } else {
    bf_tm_ig_pool_delete(tm_ctx);
    return (BF_NO_SYS_RESOURCES);
  }

  bf_tm_ig_pool_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
