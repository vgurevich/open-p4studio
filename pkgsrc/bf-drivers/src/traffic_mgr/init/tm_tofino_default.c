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


#include <stddef.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include "traffic_mgr/common/tm_error.h"
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/common/tm_hw_access.h"

#include <tofino_regs/tofino.h>
#include <lld/lld_err.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_sku.h>

/*
 *  This file implements initializing TM with default setting.
 *  Defaults are set at the device init time (device-add)
 */

#include "tm_tofino.h"
#include "tm_tofino_default.h"

static void bf_tm_tofino_log_defaults(bf_dev_id_t dev) {
  LOG_TRACE("TM: Cells for 2 pkts - %d", BF_TM_CELLS_FOR_2_PKT);

  /* Ingress default buffer carving */
  LOG_TRACE("TM: Ingress Default PPG GMin limit - %d", BF_TM_DEFAULT_PPG_LMT);
  LOG_TRACE("TM: Ingress PPG base limit in pool0 - %d",
            BF_TM_APP_POOL_0_PPG_BASE_LIMIT);
  LOG_TRACE("TM: Ingress PPG GMin pool size - %d",
            BF_TM_IG_DEFAULT_PPG_GMIN_POOL_SIZE);
  LOG_TRACE("TM: Ingress AP pool0 size - %d", BF_TM_IG_APP_POOL_0_SIZE);
  LOG_TRACE("TM: Ingress Skid pool size - %d", BF_TM_SKID_POOL_SIZE);
  LOG_TRACE("TM: Ingress Mirror pool size - %d", BF_TM_NEG_MIRROR_POOL_SIZE);

  /* Egress default buffer carving */
  LOG_TRACE("TM: Egress Q GMin limit - %d", BF_TM_Q_GMIN_LMT);
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    LOG_TRACE("TM: Egress Q base limit in AP pool0 - %d",
              BF_TM_APP_POOL_0_Q_BASE_LIMIT_REV_A0);
    LOG_TRACE("TM: Egress Q GMin pool size - %d", BF_TM_EG_GMIN_POOL_SIZE);
    LOG_TRACE("TM: Egress AP pool0 size - %d", BF_TM_EG_APP_POOL_0_SIZE_REV_A0);
    LOG_TRACE("TM: Egress AP pool3 size reserved for MC PRE FIFO - %d",
              BF_TM_EG_APP_POOL_3_MC_SIZE);
    LOG_TRACE("TM: Egress Mirror pool size - %d", BF_TM_NEG_MIRROR_POOL_SIZE);
    LOG_TRACE("TM: Egress UC CT size  - %d", BF_TM_UC_CT_POOL_SIZE_REV_A0);
    LOG_TRACE("TM: Egress MC CT size  - %d", BF_TM_MC_CT_POOL_SIZE_REV_A0);
  } else {
    LOG_TRACE("TM: Egress Q base limit in AP pool0 - %d",
              BF_TM_APP_POOL_0_Q_BASE_LIMIT);
    LOG_TRACE("TM: Egress Q GMin pool size - %d", BF_TM_EG_GMIN_POOL_SIZE);
    LOG_TRACE("TM: Egress AP pool0 size - %d", BF_TM_EG_APP_POOL_0_SIZE);
    LOG_TRACE("TM: Egress AP pool3 size reserved for MC PRE FIFO - %d",
              BF_TM_EG_APP_POOL_3_MC_SIZE);
    LOG_TRACE("TM: Egress Mirror pool size - %d", BF_TM_NEG_MIRROR_POOL_SIZE);
    LOG_TRACE("TM: Egress UC CT size  - %d", BF_TM_UC_CT_POOL_SIZE);
    LOG_TRACE("TM: Egress MC CT size  - %d", BF_TM_MC_CT_POOL_SIZE);
  }
}

bf_tm_status_t bf_tm_tofino_port_get_defaults(bf_dev_id_t devid,
                                              bf_tm_port_t *p,
                                              bf_tm_port_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_port_defaults_t));
  BF_TM_SET_DEFAULT_VAL(defaults, port_icos_mask, 0xff);
  BF_TM_SET_DEFAULT_VAL(defaults, port_ct_enable, false);
  // port_ig_limit: BF_TM_TOFINO_BUFFER_CELLS depends on SKU:
  //      64Q - 280*1024, others - 260*1024
  uint32_t lld_sku = lld_sku_get_sku(devid);
  if (lld_sku == BFN_SKU_BFN77110) {
    BF_TM_SET_DEFAULT_VAL(defaults, port_ig_limit, BF_TM_TOFINO_BUFFER_CELLS);
    BF_TM_SET_DEFAULT_VAL(defaults, port_eg_limit, BF_TM_TOFINO_BUFFER_CELLS);
  } else {
    BF_TM_SET_DEFAULT_VAL(
        defaults, port_ig_limit, BF_TM_TOFINOLITE_SKUS_BUFFER_CELLS);
    BF_TM_SET_DEFAULT_VAL(
        defaults, port_eg_limit, BF_TM_TOFINOLITE_SKUS_BUFFER_CELLS);
  }
  BF_TM_SET_DEFAULT_VAL(defaults, port_skid_limit, 0);
  BF_TM_SET_DEFAULT_VAL(
      defaults,
      port_ig_hysteresis,
      (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS : 0);
  BF_TM_SET_DEFAULT_VAL(
      defaults, port_eg_hysteresis, BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS);
  if (p) {
    uint32_t limit_cells = 0;
    switch (p->speed_on_add) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
        limit_cells = 4;
        break;
      case BF_SPEED_40G:
      case BF_SPEED_50G:
        limit_cells = 8;
        break;
      case BF_SPEED_100G:
        limit_cells = 0xf;
        break;
      default:
        limit_cells = 0xf;
        break;
    }
    BF_TM_SET_DEFAULT_VAL(defaults, port_uc_ct_limit, limit_cells);
  }
  BF_TM_SET_DEFAULT_VAL(defaults, port_mode_tx, BF_TM_PAUSE_NONE);
  BF_TM_SET_DEFAULT_VAL(
      defaults,
      port_mode_rx,
      (TM_IS_TARGET_ASIC(devid)) ? BF_TM_PAUSE_PFC : BF_TM_PAUSE_NONE);
  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tofino_q_get_defaults(bf_dev_id_t devid,
                                           bf_tm_q_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_q_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, q_gmin_limit, BF_TM_Q_GMIN_LMT)
  BF_TM_SET_DEFAULT_VAL(defaults, q_tail_drop, true)
  BF_TM_SET_DEFAULT_VAL(defaults, q_app_pool, BF_TM_EG_APP_POOL_0)
  BF_TM_SET_DEFAULT_VAL(defaults, q_dynamic_baf, BF_TM_Q_BAF_80_PERCENT)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_qac_hysteresis, BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS)
  if (g_tm_ctx[devid]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    BF_TM_SET_DEFAULT_VAL(
        defaults, q_base_use_limit, BF_TM_APP_POOL_0_Q_BASE_LIMIT_REV_A0)
  } else {
    BF_TM_SET_DEFAULT_VAL(
        defaults, q_base_use_limit, BF_TM_APP_POOL_0_Q_BASE_LIMIT)
  }
  BF_TM_SET_DEFAULT_VAL(defaults, q_cos, 0)
  BF_TM_SET_DEFAULT_VAL(defaults, q_color_drop_mode, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_color_yellow_limit, BF_TM_Q_COLOR_LIMIT_75_PERCENT)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_color_red_limit, BF_TM_Q_COLOR_LIMIT_75_PERCENT)
  BF_TM_SET_DEFAULT_VAL(defaults, q_color_yellow_hysteresis, 0)
  BF_TM_SET_DEFAULT_VAL(defaults, q_color_red_hysteresis, 0)
  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tofino_pool_get_defaults(bf_dev_id_t devid,
                                              bf_tm_app_pool_t *pool,
                                              bf_tm_pool_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_pool_defaults_t));
  if (pool) {
    if (*pool == BF_TM_IG_APP_POOL_0) {
      BF_TM_SET_DEFAULT_VAL(defaults, pfc_limit, BF_TM_IG_APP_POOL_0_SIZE)
    } else {
      BF_TM_SET_DEFAULT_VAL(defaults, pfc_limit, 0)
    }
  }
  BF_TM_SET_DEFAULT_VAL(defaults, color_drop_hysteresis, 0)

  if (g_tm_ctx[devid]->target == BF_TM_TARGET_MODEL) {
    BF_TM_SET_DEFAULT_VAL(defaults, skid_hysteresis, 0)
  } else {
    BF_TM_SET_DEFAULT_VAL(
        defaults, skid_hysteresis, BF_TM_TOFINO_WAC_RESET_HYSTERESIS * 8)
  }

  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tofino_pipe_get_defaults(bf_dev_id_t devid,
                                              bf_tm_eg_pipe_t *pipe,
                                              bf_tm_pipe_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_pipe_defaults_t));

  BF_TM_SET_DEFAULT_VAL(
      defaults, egress_limit_cells, g_tm_ctx[devid]->tm_cfg.total_cells)
  BF_TM_SET_DEFAULT_VAL(defaults, egress_hysteresis_cells, 0)
  if (pipe) {
    BF_TM_SET_DEFAULT_VAL(
        defaults, port_mirror_on_drop_dest, MAKE_DEV_PORT(pipe->l_pipe, 0))
    BF_TM_SET_DEFAULT_VAL(defaults, queue_mirror_on_drop_dest, 0)
  }
  BF_TM_SET_DEFAULT_VAL(defaults, pkt_ifg_compensation, BF_TM_IFG_COMPENSATION)

  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tofino_sch_q_get_defaults(
    bf_dev_id_t devid,
    bf_tm_port_t *p,
    bf_tm_eg_q_t *q,
    bf_tm_sch_q_defaults_t *defaults) {
  (void)devid;
  (void)q;
  bf_tm_status_t rc = BF_SUCCESS;
  memset(defaults, 0, sizeof(bf_tm_sch_q_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_dwrr_weight, 1023)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_enable, true)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_shaping_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_priority, BF_TM_SCH_PRIO_0)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_guaranteed_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_shaping_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_q_shaping_rate_prov_type, BF_TM_SCH_RATE_UPPER)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_q_shaping_rate_burst_size, BF_TM_Q_SCH_BURST_SIZE)
  if (p) {
    uint32_t rate = 0;
    switch (p->speed_on_add) {
      case BF_SPEED_1G:
        rate = BF_TM_RATE_1G;
        break;
      case BF_SPEED_10G:
        rate = BF_TM_RATE_10G;
        break;
      case BF_SPEED_25G:
        rate = BF_TM_RATE_25G;
        break;
      case BF_SPEED_40G:
        rate = BF_TM_RATE_40G;
        break;
      case BF_SPEED_50G:
        rate = BF_TM_RATE_50G;
        break;
      case BF_SPEED_100G:
        rate = BF_TM_RATE_100G;
        break;
      case BF_SPEED_NONE:
        // like bf_tm_tofino_set_default_for_port() does
        rate = BF_TM_RATE_100G;
        break;
      default:
        rc = BF_NOT_SUPPORTED;
        break;
    }
    if (BF_SUCCESS == rc) {
      BF_TM_SET_DEFAULT_VAL(defaults, sch_q_shaping_rate, rate)
    }
  }
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_guaranteed_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_q_guaranteed_rate_burst_size, BF_TM_Q_SCH_BURST_SIZE)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_q_guaranteed_rate_prov_type, BF_TM_SCH_RATE_UPPER)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_guaranteed_rate, 0)

  return rc;
}

bf_tm_status_t bf_tm_tofino_sch_port_get_defaults(
    bf_dev_id_t devid, bf_tm_port_t *p, bf_tm_sch_port_defaults_t *defaults) {
  (void)devid;
  bf_tm_status_t rc = BF_SUCCESS;
  memset(defaults, 0, sizeof(bf_tm_sch_port_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_port_shaping_rate_prov_type, BF_TM_SCH_RATE_UPPER)
  if (p) {
    BF_TM_SET_DEFAULT_VAL(
        defaults,
        sch_port_shaping_rate_burst_size,
        (p->offline) ? BF_TM_Q_SCH_BURST_SIZE : BF_TM_PORT_SCH_BURST_SIZE)
    uint32_t rate = 0;
    switch (p->speed_on_add) {
      case BF_SPEED_1G:
        rate = BF_TM_RATE_1G;
        break;
      case BF_SPEED_10G:
        rate = BF_TM_RATE_10G;
        break;
      case BF_SPEED_25G:
        rate = BF_TM_RATE_25G;
        break;
      case BF_SPEED_40G:
        rate = BF_TM_RATE_40G;
        break;
      case BF_SPEED_50G:
        rate = BF_TM_RATE_50G;
        break;
      case BF_SPEED_100G:
        rate = BF_TM_RATE_100G;
        break;
      case BF_SPEED_NONE:
        // like bf_tm_tofino_set_default_for_port() does
        rate = BF_TM_RATE_100G;
        break;
      default:
        rc = BF_NOT_SUPPORTED;
        break;
    }
    if (BF_SUCCESS == rc) {
      BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_rate, rate)
    }
  }

  return rc;
}

bf_tm_status_t bf_tm_tofino_ppg_get_defaults(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg,
                                             bf_tm_ppg_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_ppg_defaults_t));
  if (ppg) {
    if (ppg->is_default_ppg) {
      BF_TM_SET_DEFAULT_VAL(defaults, min_limit_cells, BF_TM_DEFAULT_PPG_LMT)
      // For dynamic sharing to work, hysteresis shouldn't be 0
      // the reason why POR value is 4 in 8 cells unit i.e 32.
      // On SW model it is zero.
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          hysteresis_cells,
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS : 0)
      bool is_mirror_dpg = BF_TM_IS_MIRROR_PORT(g_tm_ctx[devid], ppg->port);
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          pool_max_cells,
          (is_mirror_dpg) ? 0xa0 : BF_TM_APP_POOL_0_PPG_BASE_LIMIT)
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          dynamic_baf,
          (is_mirror_dpg) ? BF_TM_PPG_BAF_DISABLE : BF_TM_PPG_BAF_80_PERCENT)
    } else {
      BF_TM_SET_DEFAULT_VAL(defaults, min_limit_cells, 0)
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          hysteresis_cells,
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS : 0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool_max_cells, 0)
      BF_TM_SET_DEFAULT_VAL(defaults, dynamic_baf, BF_TM_PPG_BAF_DISABLE)
    }
  }

  return BF_SUCCESS;
}

static bf_status_t bf_tm_tofino_set_default_for_ig_pool(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;
  uint32_t uc_ct_size, mc_ct_size;

  LOG_TRACE("TM: %s: Set default values for Ingress Pools", __func__);

  // Set APP pool default
  rc = bf_tm_pool_size_set(dev, BF_TM_IG_APP_POOL_0, BF_TM_IG_APP_POOL_0_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set pool default size");
    goto cleanup;
  }
  rc = bf_tm_pool_color_drop_enable(dev, BF_TM_IG_APP_POOL_0);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not enable color drop for application pool 0");
    goto cleanup;
  }
  // Set color drop limits for all colors to full size of the pool.
  // Set color drop hyst for all colors to none
  for (i = color_start; i < color_end + 1; i++) {
    rc = bf_tm_pool_color_drop_limit_set(
        dev, BF_TM_IG_APP_POOL_0, i, BF_TM_IG_APP_POOL_0_SIZE);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not set pool color drop size");
      goto cleanup;
    }
    // color drop hysteresis -- no hyst
    rc = bf_tm_pool_color_drop_hysteresis_set(dev, i, 0);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not set pool color drop hyst");
      goto cleanup;
    }
  }

  for (i = 0; i < 8; i++) {
    rc = bf_tm_pool_pfc_limit_set(
        dev, BF_TM_IG_APP_POOL_0, i, BF_TM_IG_APP_POOL_0_SIZE);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not set pool pfc limits");
      goto cleanup;
    }
  }

  rc = bf_tm_pool_mirror_on_drop_size_set(dev, BF_TM_NEG_MIRROR_POOL_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set negative mirror pool size");
  }

  /* Set default UC/MC CT size base on chip part revision number */
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    uc_ct_size = BF_TM_UC_CT_POOL_SIZE_REV_A0;
    mc_ct_size = BF_TM_MC_CT_POOL_SIZE_REV_A0;
  } else {
    uc_ct_size = BF_TM_UC_CT_POOL_SIZE;
    mc_ct_size = BF_TM_MC_CT_POOL_SIZE;
  }
  rc = bf_tm_pool_uc_cut_through_size_set(dev, uc_ct_size);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set default UC cut through size");
  }
  rc = bf_tm_pool_mc_cut_through_size_set(dev, mc_ct_size);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set default MC cut through size");
  }
cleanup:
  return (rc);
}

static bf_status_t bf_tm_tofino_set_default_for_ppg(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_ppg_hdl ppg;
  int lport;
  bf_dev_port_t port;
  int j, k;
  uint32_t num_pipes;

  LOG_TRACE("TM: %s: Set default values for PPGs", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j /*logical_pipe*/, lport);
      // Default PPG
      rc = bf_tm_ppg_defaultppg_get(dev, port, &ppg);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: Unable to get default PPG for pipe, port = %d, %d", j, port);
        continue;
      }

      // For default PPG the Cache Counters need to be allocated here since no
      // alloc call is called later by user.

      rc = bf_tm_ppg_set_cache_counters(dev, ppg);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("PPG handle 0x%x port %d Unable to allocate cache counters",
                  ppg,
                  port);
        // Should we return error or since this is default ppg continue?
      }

      /* ppg icos mapping is only done for PFC PPG... icos that are NOT mapped
      to any PFC
       * PPGs will automatically map to default PPGs.
       * So NO need to setup ppg_icos mapping during default init.
      rc = bf_tm_ppg_icos_mapping_set(dev, ppg, 0xff); // Map all icos to
      default ppg
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg icos mapping for ppg %d", ppg);
      }
      */
      rc = bf_tm_ppg_app_pool_usage_set(dev,
                                        ppg,
                                        BF_TM_IG_APP_POOL_0,
                                        BF_TM_APP_POOL_0_PPG_BASE_LIMIT,
                                        // By default, ingress limit should
                                        // be > egress to trigger egress drops
                                        // during congestion
                                        BF_TM_PPG_BAF_80_PERCENT,
                                        BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg app pool usage for ppg %d", ppg);
      }
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, BF_TM_DEFAULT_PPG_LMT);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set gmin limit for ppg %d", ppg);
      }
      rc = bf_tm_ppg_guaranteed_min_skid_hysteresis_set(dev, ppg, 0);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set min/skid hysteresis for PPG (%d)", ppg);
      }
    }
    // Program mirror port
    for (k = 0; k < g_tm_ctx[dev]->tm_cfg.mirror_port_cnt; k++) {
      lport = MAKE_DEV_PORT(j, g_tm_ctx[dev]->tm_cfg.mirror_port_start + k);
      port = MAKE_DEV_PORT(j, lport);
      // Default PPG
      rc = bf_tm_ppg_defaultppg_get(dev, port, &ppg);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: Unable to get default PPG for pipe, mirror port = %d, %d",
            j,
            port);
        continue;
      }

      // For default PPG the Cache Counters need to be allocated here since no
      // alloc call is called later by user.

      rc = bf_tm_ppg_set_cache_counters(dev, ppg);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("PPG handle 0x%x port %d Unable to allocate cache counters",
                  ppg,
                  port);
        // Should we return error or since this is default ppg continue?
      }

      /* ppg icos mapping is only done for PFC PPG... icos that are NOT mapped
      to any PFC
       * PPGs will automatically map to default PPGs.
       * So NO need to setup ppg_icos mapping during default init.
      rc = bf_tm_ppg_icos_mapping_set(dev, ppg, 0xff); // Map all icos to
      default ppg
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg icos mapping for ppg %d", ppg);
      }
      */
      /*
       * Disable dynamic sharing and set static shared limit of
       * 160 cells (for 8 packets) so that mirror
       * port traffic doesn't affect regular traffic in that pool.
       */
      rc = bf_tm_ppg_app_pool_usage_set(
          dev,
          ppg,
          BF_TM_IG_APP_POOL_0,
          (BF_TM_CELLS_FOR_2_PKT * 4),  // Static limit
          BF_TM_PPG_BAF_DISABLE,        // Dynamic sharing is disabed
          BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg app pool usage for ppg %d", ppg);
      }
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, BF_TM_DEFAULT_PPG_LMT);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set gmin limit for ppg %d", ppg);
      }
      rc = bf_tm_ppg_guaranteed_min_skid_hysteresis_set(dev, ppg, 0);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set min/skid hysteresis for PPG (%d)", ppg);
      }
    }
  }
  // No default setting for non default PPG (0 - 127 in case of tofino).
  // They are user managed
  return (rc);
}

static bf_status_t bf_tm_tofino_set_ingress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *ports;
  bf_dev_port_t lport;
  int p;
  int j;
  uint32_t num_pipes;
  bf_dev_pipe_t pipe = 0;

  // Clear table entries that were supposed to be power on reset 0x0
  // but some how did not.
  // Tofino TM cfg space will always have 4 pipes regardless of skew
  for (j = 0; j < 4; j++) {
    for (p = 0; p < 128; p++) {
      bf_tm_write_register(dev,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_pipe[j]
                                        .csr_mem_wac_ppg_icos_mapping.entry[p]),
                           0);
    }
  }

  /* Default Ingress TM carving:
   *    - cells worth 8 pkts of 1560B as PPG Gmin limit
   *    - All icos traffic mapped to default PPG
   *    - No Skid for default PPG. No Hysteresis
   *    - Default PPG mapped to app pool 0. BAF = 80%
   *    - Color drop enabled. For all colors, drop limit is = size of pool
   */
  // Default setting for pool/s
  rc = bf_tm_tofino_set_default_for_ig_pool(dev);
  // Default setting for ppg/s
  rc |= bf_tm_tofino_set_default_for_ppg(dev);

  // Set WAC and QAC port limits
  ports = g_tm_ctx[dev]->ports;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                     g_tm_ctx[dev]->tm_cfg.pg_per_pipe);
         p++) {
      lport = MAKE_DEV_PORT(j, p);
      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s - Failed to get port descriptor for dev %d, dev_port %d",
            __func__,
            dev,
            lport);
        return (rc);
      }
      // Port limit should ideally be sum of PPG limits that are mapped to the
      // port.
      // Default port limit is set to MAX
      bf_tm_port_set_wac_drop_limit(
          dev, ports, g_tm_ctx[dev]->tm_cfg.total_cells);
      bf_tm_port_set_wac_hyst(dev, ports, BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS);
    }

    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.mirror_port_cnt); p++) {
      lport = MAKE_DEV_PORT(j, p + g_tm_ctx[dev]->tm_cfg.mirror_port_start);
      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s - Failed to get port descriptor for dev %d, dev_port %d",
            __func__,
            dev,
            lport);
        return (rc);
      }
      bf_tm_port_set_wac_drop_limit(
          dev, ports, g_tm_ctx[dev]->tm_cfg.total_cells);
      bf_tm_port_set_wac_hyst(dev, ports, BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS);
    }
  }

  /* For A0, set WAC bypass egress queue to 1 */
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    for (j = 0; j < (int)num_pipes; j++) {
      if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &pipe) != LLD_OK) {
        LOG_ERROR(
            "Unable to map logical pipe to physical pipe id. device = %d "
            "logical "
            "pipe = %d",
            dev,
            j);
        /* continue for other pipes */
        continue;
      }
      bf_tm_write_register(
          dev,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                       .wac_reg.wac_bypass_config),
          6);
    }
  }

  return (rc);
}

//////   EGRESS DEFFAULTS ///////////////////

static bf_status_t bf_tm_tofino_set_default_for_eg_pool(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;
  uint32_t eg_ap_pool_0_size;

  LOG_TRACE("TM: %s: Set default values for egress Pools", __func__);
  // Set APP pool default based on chip part revision number;
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    eg_ap_pool_0_size = BF_TM_EG_APP_POOL_0_SIZE_REV_A0;
  } else {
    eg_ap_pool_0_size = BF_TM_EG_APP_POOL_0_SIZE;
  }
  rc = bf_tm_pool_size_set(dev, BF_TM_EG_APP_POOL_0, eg_ap_pool_0_size);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set pool default size");
    goto cleanup;
  }
  rc = bf_tm_pool_color_drop_enable(dev, BF_TM_EG_APP_POOL_0);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not enable color drop for application pool 0");
    goto cleanup;
  }
  // Set color drop limits for all colors to full size of the pool.
  // Set color drop hyst for all colors to none
  for (i = color_start; i < color_end + 1; i++) {
    rc = bf_tm_pool_color_drop_limit_set(
        dev, BF_TM_EG_APP_POOL_0, i, eg_ap_pool_0_size);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not set pool color drop size");
      goto cleanup;
    }
    // color drop hysteresis -- no hyst
    rc = bf_tm_pool_color_drop_hysteresis_set(dev, i, 0);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not set pool color drop hyst");
      goto cleanup;
    }
  }

  /*
   * As PRE FIFO uses egress AP pool3 on POR and we don't change it
   * (neither drivers default nor through exposed API), set egress AP pool3
   * size to 12K (0x3000) cells by default so MC would work fine
   * even if there is burst.
   */
  rc = bf_tm_pool_size_set(
      dev, BF_TM_EG_APP_POOL_3, BF_TM_EG_APP_POOL_3_MC_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set pool default size");
    goto cleanup;
  }

  rc = bf_tm_pool_mirror_on_drop_size_set(dev, BF_TM_NEG_MIRROR_POOL_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set negative mirror pool size");
  }
cleanup:
  return (rc);
}

static bf_status_t bf_tm_tofino_set_default_for_port(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t port;
  int lport;
  int j;
  uint32_t num_pipes;

  LOG_TRACE("TM: %s: Set default values for egress ports", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      // rate = 100G
      // burst size = 16384 bytes (~2 jumbo packet)
      rc |= bf_tm_sched_port_shaping_rate_set(
          dev, port, false, BF_TM_Q_SCH_BURST_SIZE, 100000000);
      rc |= bf_tm_sched_port_enable(dev, port, BF_SPEED_10G);
    }
  }
  return (rc);
}

static bf_status_t bf_tm_tofino_set_sch_default_for_q(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_queue_t q) {
  bf_status_t rc = BF_SUCCESS;

  rc |= bf_tm_sched_q_priority_set(dev, port, q, BF_TM_SCH_PRIO_0);
  rc |= bf_tm_sched_q_dwrr_weight_set(dev, port, q, 1023);
  // rate set to 100G, burst size = 16384bytes
  rc |= bf_tm_sched_q_shaping_rate_set(
      dev, port, q, false, BF_TM_Q_SCH_BURST_SIZE, 100000000);
  // rate set to 100G, burst size = 16384bytes
  rc |= bf_tm_sched_q_guaranteed_rate_set(
      dev, port, q, false, BF_TM_Q_SCH_BURST_SIZE, 0);
  rc |= bf_tm_sched_q_remaining_bw_priority_set(dev, port, q, BF_TM_SCH_PRIO_0);
  rc |= bf_tm_sched_q_enable(dev, port, q);

  return (rc);
}

static bf_status_t bf_tm_tofino_set_default_for_q(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_queue_t q;
  int lport, i, j;
  uint32_t num_pipes;
  bf_dev_port_t port;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;
  uint32_t queue_base_limit;

  LOG_TRACE("TM: %s: Set default values for egress queues", __func__);

  /* Set the queue base limit based on chip part revision number */
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    queue_base_limit = BF_TM_APP_POOL_0_Q_BASE_LIMIT_REV_A0;
  } else {
    queue_base_limit = BF_TM_APP_POOL_0_Q_BASE_LIMIT;
  }

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      rc = bf_tm_port_q_mapping_set(
          dev, port, BF_TM_TOFINO_MIN_QUEUES_PER_PORT, NULL);
      if (rc != BF_SUCCESS) {
        // Not able to carve queues for port..
        // Running out of queue profiles ???
        LOG_ERROR("TM: %s: Could not carve queues for port %d", __func__, port);
        break;
      }
      for (q = 0; q < BF_TM_TOFINO_MIN_QUEUES_PER_PORT; q++) {
        rc = bf_tm_q_app_pool_usage_set(dev,
                                        port,
                                        q,
                                        BF_TM_EG_APP_POOL_0,
                                        queue_base_limit,
                                        BF_TM_Q_BAF_80_PERCENT,
                                        BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS);
        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "TM: %s: Could not set Q pool usage for pipe %d port %d, q %d",
              __func__,
              j,
              lport,
              q);
          return (rc);
        }
        rc = bf_tm_q_guaranteed_min_limit_set(dev, port, q, BF_TM_Q_GMIN_LMT);
        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "TM: %s: Could not set Q gmin limitfor pipe %d port %d, q %d",
              __func__,
              j,
              lport,
              q);
          return (rc);
        }

        rc = bf_tm_q_tail_drop_enable(dev, port, q);
        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "TM: %s: Could not enable Q tail drop for pipe %d port %d, q %d",
              __func__,
              j,
              lport,
              q);
          return (rc);
        }

        for (i = color_start; i < color_end + 1; i++) {
          rc = bf_tm_q_color_limit_set(
              dev, port, q, i, BF_TM_Q_COLOR_LIMIT_75_PERCENT);
          if (rc != BF_SUCCESS) {
            LOG_ERROR(
                "TM: %s: Could not set Q color limit for pipe %d port %d, q %d",
                __func__,
                j,
                lport,
                q);
            return (rc);
          }

          /*
           * Per HW team, disable color drop (tail drop for red and
           * yellow packets) by default. Since tail drop is enabled
           * by default, it covers for all packets
           */
          rc = bf_tm_q_color_drop_disable(dev, port, q);
          if (rc != BF_SUCCESS) {
            LOG_ERROR(
                "TM: %s: Could not disable Q color drop for pipe %d port %d, q "
                "%d",
                __func__,
                j,
                lport,
                q);
            return (rc);
          }
        }
        // scheduler related  defaults on queue
        rc = bf_tm_tofino_set_sch_default_for_q(dev, port, q);
        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "TM: %s: Could not set Q scheduler params for pipe %d port %d, q "
              "%d",
              __func__,
              j,
              lport,
              q);
          return (rc);
        }
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_tofino_set_egress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int j, priority;
  uint32_t num_pipes;
  bf_dev_pipe_t pipe = 0;
  uint32_t val;
  bf_tm_port_t *ports;
  bf_dev_port_t lport;
  int p;

  /* Default Egress TM carving:
   *    - cells worth 8 pkts of 1560B as Queue Gmin limit.
   *      !!!! Idea is not to trigger egress limit before ingress limit. !!!!
   *    - Color drop enabled. For all colors, drop limit is = size of queue
   */
  LOG_TRACE("TM: %s: Set default values for egress TM", __func__);

  ports = g_tm_ctx[dev]->ports;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    bf_tm_pipe_egress_limit_set(
        dev, j, g_tm_ctx[dev]->tm_cfg.total_cells);  // set to max
    bf_tm_pipe_egress_hysteresis_set(dev, j, 0);
    bf_tm_sched_pkt_ifg_compensation_set(dev, j, 20);
    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                     g_tm_ctx[dev]->tm_cfg.pg_per_pipe);
         p++) {
      lport = MAKE_DEV_PORT(j, p);
      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s - Failed to get port descriptor for dev %d, dev_port %d",
            __func__,
            dev,
            lport);
        return (rc);
      }
      rc = bf_tm_port_set_qac_drop_limit(
          dev, ports, g_tm_ctx[dev]->tm_cfg.total_cells);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s: Unable to set egress port drop limit dev(%d) pipe(%d) "
            "port(%d) status 0x%0x",
            __func__,
            dev,
            ports->l_pipe,
            ports->port,
            rc);
        return (rc);
      }
      rc = bf_tm_port_set_qac_hyst(
          dev, ports, BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s: Unable to set egress port hysterersis dev(%d) pipe(%d) "
            "port(%d) status 0x%0x",
            __func__,
            dev,
            ports->l_pipe,
            ports->port,
            rc);
        return (rc);
      }
    }
  }

  // Default setting for pool/s from egress TM prespective
  rc = bf_tm_tofino_set_default_for_eg_pool(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress pools");
    return (rc);
  }
  // Default setting for queue/s
  rc = bf_tm_tofino_set_default_for_q(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for queues");
    return (rc);
  }
  // deafult setting for port
  rc = bf_tm_tofino_set_default_for_port(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for ports");
    return (rc);
  }

  /*
   * As per HW team, set QAC Egress Buffer MC PRE Per Priority FIFO
   * Pkt Max Limit to 0xc0
   */
  for (j = 0; j < (int)num_pipes; j++) {
    if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &pipe) != LLD_OK) {
      LOG_ERROR(
          "%s: Unable to map logical pipe to physical pipe id. device = %d "
          "logical "
          "pipe = %d",
          __func__,
          dev,
          j);
      /* continue for other pipes */
      continue;
    }

    switch (pipe) {
      case 0:
        for (priority = 0; priority < 4; priority++) {
          bf_tm_write_register(
              dev,
              offsetof(Tofino,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe0[priority]),
              0xc0);
        }
        break;

      case 1:
        for (priority = 0; priority < 4; priority++) {
          bf_tm_write_register(
              dev,
              offsetof(Tofino,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe1[priority]),
              0xc0);
        }
        break;

      case 2:
        for (priority = 0; priority < 4; priority++) {
          bf_tm_write_register(
              dev,
              offsetof(Tofino,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe2[priority]),
              0xc0);
        }
        break;

      case 3:
        for (priority = 0; priority < 4; priority++) {
          bf_tm_write_register(
              dev,
              offsetof(Tofino,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe3[priority]),
              0xc0);
        }
        break;

      default:
        /* Do nothing */
        break;
    }

    /* Enable QLC counters by default */
    val = 0x3c00000c;
    bf_tm_write_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_qlc_top.qlc[pipe].control),
        val);

    /* Enable PRC counters by default */
    val = 0xf;
    bf_tm_write_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_prc_top.prc[pipe].control),
        val);
  }

  return (rc);
}

static bf_status_t bf_tm_tofino_init_seq(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val, i;
  uint32_t done;

  // Take caa block out of reset
  val = 0;
  setp_caa_block_reset_r_value(&val, 0);
  bf_tm_write_register(
      dev, offsetof(Tofino, device_select.tm_top.tm_caa.block_reset), val);

  // Take clm block out of reset
  val = 0;
  setp_clm_blk_reset_blk_reset(&val, 0);
  bf_tm_write_register(
      dev,
      offsetof(Tofino,
               device_select.tm_top.tm_clc_top.clc_common.clm_blk_reset),
      val);

  // Take qlm block out of reset
  val = 0;
  setp_qlm_blk_reset_blk_reset(&val, 0);
  bf_tm_write_register(
      dev,
      offsetof(Tofino,
               device_select.tm_top.tm_qlc_top.qlc_common.qlm_blk_reset[0]),
      val);

  bf_tm_write_register(
      dev,
      offsetof(Tofino,
               device_select.tm_top.tm_qlc_top.qlc_common.qlm_blk_reset[1]),
      val);

  // Take psc block out of reset
  val = 0;
  setp_psc_block_reset_r_block_reset_0_2_value_31_0(&val, 0);
  bf_tm_write_register(dev,
                       offsetof(Tofino,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_reset.block_reset_0_2),
                       val);
  val = 0;
  setp_psc_block_reset_r_block_reset_1_2_value_35_32(&val, 0);
  bf_tm_write_register(dev,
                       offsetof(Tofino,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_reset.block_reset_1_2),
                       val);

  // TM driver is communicating with real ASIC/Emulator/RTL-SIM
  // Hence its possible to read back chip progress.

  // check caa-ready-before  enabling caa
  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev, offsetof(Tofino, device_select.tm_top.tm_caa.block_ready), &done);
    LOG_TRACE("CAA block Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("CAA block Ready");

  // check clm-ready (clc_top.clc_common.clm_blk_rdy)
  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev,
        offsetof(Tofino,
                 device_select.tm_top.tm_clc_top.clc_common.clm_blk_rdy),
        &done);
    LOG_TRACE("CLC.CLM block Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("CLC.CLM block Ready");

  // check qlm-ready (qlc_top.qlc_common.blk_rdy)
  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy[0]),
        &done);
    LOG_TRACE("QLC block[0] Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }

  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy[1]),
        &done);
    done &= 0xf;  // Only LSB 4 bits are for QLC
    LOG_TRACE("QLC block[1] Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("QLC block Ready");

  // check psc-ready-before  enabling psc
  done = 0;
  while (!done) {
    bf_tm_read_register(dev,
                        offsetof(Tofino,
                                 device_select.tm_top.tm_psc_top.psc_common
                                     .block_ready.block_ready_0_2),
                        &done);
    LOG_TRACE("PSC block Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("PSC block Ready");
  // Enable psc block

  // Enable caa block
  val = 0;
  setp_caa_block_enable_r_value(&val, 0xfffffff);
  bf_tm_write_register(
      dev, offsetof(Tofino, device_select.tm_top.tm_caa.block_enable), val);

  val = 0;
  setp_psc_block_enable_r_block_enable_0_2_value_31_0(&val, 0xffffffff);
  bf_tm_write_register(dev,
                       offsetof(Tofino,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_enable.block_enable_0_2),
                       val);
  val = 0;
  setp_psc_block_enable_r_block_enable_1_2_value_35_32(&val, 0xf);
  bf_tm_write_register(dev,
                       offsetof(Tofino,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_enable.block_enable_1_2),
                       val);

  ////////////   Trigger mem inits of WAC, QAC, SCH, PRC/PRM /////////////
  // Trigger WAC mem init
  val = 0xf0;
  bf_tm_write_register(
      dev,
      offsetof(
          Tofino,
          device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_glb_config),
      val);
  // Trigger QAC mem init
  val = 0;
  setp_qac_mem_init_en_enable(&val, 0xf);
  bf_tm_write_register(dev,
                       offsetof(Tofino,
                                device_select.tm_top.tm_qac_top.qac_common
                                    .qac_common.qac_mem_init_en),
                       val);
  // Trigger SCH mem init
  val = 0;
  setp_sch_ctrl_r_hw_init_enb(&val, 1);
  for (i = 0; i < 4; i++) {
    bf_tm_write_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[i].ctrl),
        val);
  }

  // Trigger PRC/PRM mem init
  val = 0;
  setp_prc_control_map_init(&val, 1);
  bf_tm_write_register(
      dev,
      offsetof(Tofino, device_select.tm_top.tm_prc_top.prc_common.control),
      val);

  // TM driver is communicating with real ASIC/Emulator/RTL-SIM
  // Hence its possible to read back chip progress.
  ///////////////   Check mem init Done for WAC, QAC, SCH, PRC //////////////
  done = 0;
  while (!done) {
    bf_tm_read_register(dev,
                        offsetof(Tofino,
                                 device_select.tm_top.tm_wac_top.wac_common
                                     .wac_common.wac_mem_init_done),
                        &done);
    LOG_TRACE("Wac Mem init Done Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("Wac Mem init Complete ");
  done = 0;
  while (!done) {
    bf_tm_read_register(dev,
                        offsetof(Tofino,
                                 device_select.tm_top.tm_qac_top.qac_common
                                     .qac_common.qac_mem_init_done),
                        &done);
    LOG_TRACE("Qac Mem init Done Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("Qac Mem init Complete ");
  for (i = 0; i < 4; i++) {
    done = 0;
    while ((done & 0x3ff) != 0x3ff) {
      bf_tm_read_register(
          dev,
          offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[i].ready),
          &done);
      LOG_TRACE("SCH PIPE %d Mem init Done Status = %#x", i, done);
      if ((done & 0x3ff) != 0x3ff) bf_sys_usleep(100);
    }
  }
  LOG_TRACE("SCH Mem init Complete ");
  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_prc_top.prc_common.status),
        &done);
    done = getp_prc_status_init_done(&done);
    LOG_TRACE("PRC Map Mem init done status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("PRC Map Mem init Complete ");

  return (rc);
}

bf_status_t bf_tm_tofino_start_init_seq_during_fast_recfg(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  // Init seq to be applied before configuring TM
  // Since init sequence is interactive, (deassert block reset, check for
  // init done etc), read/writes to TM will be via normal pcie read/writes
  // no DMA at this point.
  g_tm_ctx[dev]->batch_mode = false;
  g_tm_ctx[dev]->fast_reconfig_init_seq = true;
  rc = bf_tm_tofino_init_seq(dev);
  g_tm_ctx[dev]->batch_mode = true;
  g_tm_ctx[dev]->fast_reconfig_init_seq = false;

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}

bf_status_t bf_tm_tofino_set_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  bf_dev_pipe_t pipe = 0;

  bf_tm_tofino_log_defaults(dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  // Set internal_call so that TM_LOCK is moot
  g_tm_ctx[dev]->internal_call = true;

  if (!tm_is_device_locked(dev)) {
    // Not Fast Reconfig mode; Clean boot....Apply init sequence.
    // In fast reconfig mode, init sequence is already applied in core-reset
    // phase.

    // Init seq has to be applied before configuring TM
    // Since init sequence is interactive, (deassert block reset, check for
    // init done etc), read/writes to TM will be via normal pcie read/writes
    // no DMA at this point.
    g_tm_ctx[dev]->batch_mode = false;
    g_tm_ctx[dev]->fast_reconfig_init_seq = false;
    rc = bf_tm_tofino_init_seq(dev);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("TM: Unable to apply power on init sequence on TM");
      goto cleanup;
    }

#ifdef BF_TM_HITLESS_HA_TESTING_WITH_MODEL
    // ---------  Work around for TM testing on Model --------

    // set batch_mode to true so that both power on reset of tables and
    // default configs are pushed via writelist.
    g_tm_ctx[dev]->batch_mode = true;
    // Explicitly reset all config tables. This is NOT required on real chip
    // because power on reset will ensure the table entries are zero.
    // However, explicit reset to zero is needed if testing with Model.
    // Model will NOT force table entries to zero without writing zero.
    // Table reset is needed for hitless HA Unit Testing.
    // Following 2 lines can be commented out when hit less HA testing is
    // not done on Model any more and only ASIC/chip is used. Until then
    // explict reset of tables is needed.
    // Pipe SKU is not needed. All pipes modules are always present in TM.
    LOG_TRACE("TM: Reset tables to POR value");
    for (i = 0; i < 4; i++) {
      rc |= bf_tm_ucli_power_on_reset_per_pipe_tbls(dev, i);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to Reset tables to POR value on TM");
        goto cleanup;
      }
    }
    rc |= bf_tm_power_on_reset_non_pipe_tbls(dev);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("TM: Unable to Reset tables to POR value on TM");
      goto cleanup;
    }
    bf_tm_flush_wlist(dev);  // Push any buffered writes to HW
    sleep(2);  // <---- This sleep is needed for DMA buffers to sink into model.
#endif
  }
  g_tm_ctx[dev]->batch_mode = true;

  // Ingress
  rc = bf_tm_tofino_set_ingress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Ingress TM");
    goto cleanup;
  }
  // Egress
  rc = bf_tm_tofino_set_egress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress TM");
    goto cleanup;
  }

  uint32_t hyst_threshold = 0xA0;

  bf_tm_write_register(
      dev,
      offsetof(Tofino, device_select.tm_top.tm_caa.hyst_threshold),
      hyst_threshold);

  // Enable cut through mode by default.
  // Done when port comes up. when tm_add_port() function is invoked.

  // Enable total in/out per pipe counter in CLC block.
  uint32_t val;
  if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    /* Set bit 8 (cut through) & stop-wait_default to 18 for rev A0 */
    val = 0x12100;
  } else {
    /* Set bit 8 (cut through) & stop-wait_default to 1 for rev B0 and later
     * parts */
    val = 0x1100;
  }
  for (i = 0; i < 4; i++) {
    bf_tm_write_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_clc_top.pex[i].pipe_ctrl),
        val);
    bf_tm_write_register(
        dev,
        offsetof(Tofino, device_select.tm_top.tm_clc_top.clc[i].pipe_ctrl),
        val);
  }

  /*
   * Disable non-active (unused) pipes in TM blocks so that TM buffer
   * is not reserved for any unused pipes.
   */
  for (i = 0; i < BF_SUBDEV_PIPE_COUNT; i++) {
    if (lld_sku_map_phy_pipe_id_to_pipe_id(dev, i, &pipe) != LLD_OK) {
      bf_tm_write_register(
          dev,
          offsetof(Tofino, device_select.tm_top.tm_caa.epipe[i].enable),
          0);
      bf_tm_write_register(
          dev,
          offsetof(Tofino,
                   device_select.tm_top.tm_psc_top.psc_common.epipe[i].enable),
          0);
    }
  }

  bf_tm_flush_wlist(dev);  // Push any buffered writes to HW
cleanup:
  g_tm_ctx[dev]->internal_call = false;
  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}
