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
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/common/tm_hw_access.h"

#include <tof2_regs/tof2_reg_drv.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof2_regs/tof2_mem_addr.h>
#include <lld/lld_err.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_sku.h>

/*
 *  This file implements initializing TM with default setting.
 *  Defaults are set at the device init time (device-add)
 */

#include "traffic_mgr/init/tm_tof2.h"
#include "traffic_mgr/init/tm_tof2_default.h"

static void bf_tm_tof2_log_defaults() {
  LOG_TRACE("TM: Cells for 2 pkts %d", BF_TM_CELLS_FOR_2_PKT);
  LOG_TRACE("TM: Default PPG limit %d", BF_TM_DEFAULT_PPG_LMT);
  LOG_TRACE("TM: PPG limit %d", BF_TM_PPG_LMT);
  LOG_TRACE("TM: Ingress Gmin pool size %d", BF_TM_IG_GMIN_POOL_SIZE);
  LOG_TRACE("TM: Ingress pool0 size %d", BF_TM_IG_APP_POOL_0_SIZE);
  LOG_TRACE("TM: Ppg base limit in pool0 %d", BF_TM_APP_POOL_0_PPG_BASE_LIMIT);
  LOG_TRACE("TM: Mirror pool size %d", BF_TM_NEG_MIRROR_POOL_SIZE);
  LOG_TRACE("TM: Egress pool0 size %d", BF_TM_EG_APP_POOL_0_SIZE);
  LOG_TRACE("TM: Q base limit in pool0 %d", BF_TM_APP_POOL_0_Q_BASE_LIMIT);
  LOG_TRACE("TM: Q Gmin limit %d", BF_TM_Q_GMIN_LMT);
  LOG_TRACE("TM EG MC Size pool3 %d", BF_TM_EG_APP_POOL_3_MC_SIZE);
}

bf_tm_status_t bf_tm_tof2_port_get_defaults(bf_dev_id_t devid,
                                            bf_tm_port_t *p,
                                            bf_tm_port_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_port_defaults_t));
  BF_TM_SET_DEFAULT_VAL(defaults, port_icos_mask, 0xff);
  BF_TM_SET_DEFAULT_VAL(defaults, port_ct_enable, false);
  BF_TM_SET_DEFAULT_VAL(defaults, port_ig_limit, BF_TM_TOF2_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(defaults, port_eg_limit, BF_TM_TOF2_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(defaults, port_skid_limit, BF_TM_TOF2_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(
      defaults,
      port_ig_hysteresis,
      (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS : 0);
  BF_TM_SET_DEFAULT_VAL(
      defaults, port_eg_hysteresis, BF_TM_TOF2_QAC_DEFAULT_HYSTERESIS);
  if (p) {
    uint32_t limit_cells = 0;
    switch (p->speed_on_add) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
        limit_cells = 4;
        break;
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        limit_cells = 8;
        break;
      case BF_SPEED_100G:
        limit_cells = 0xf;
        break;
      case BF_SPEED_200G:
        limit_cells = 0x7f;
        break;
      case BF_SPEED_400G:
        limit_cells = 0x7f;
        break;
      default:
        limit_cells = 0x7f;
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

bf_tm_status_t bf_tm_tof2_q_get_defaults(bf_dev_id_t devid,
                                         bf_tm_q_defaults_t *defaults) {
  (void)devid;  // for future purposes
  memset(defaults, 0, sizeof(bf_tm_q_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, q_gmin_limit, BF_TM_Q_GMIN_LMT)
  BF_TM_SET_DEFAULT_VAL(defaults, q_tail_drop, true)
  BF_TM_SET_DEFAULT_VAL(defaults, q_app_pool, BF_TM_EG_APP_POOL_0)
  BF_TM_SET_DEFAULT_VAL(defaults, q_dynamic_baf, BF_TM_Q_BAF_80_PERCENT)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_qac_hysteresis, BF_TM_TOF2_QAC_DEFAULT_HYSTERESIS)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_base_use_limit, BF_TM_APP_POOL_0_Q_BASE_LIMIT)
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

bf_tm_status_t bf_tm_tof2_pool_get_defaults(bf_dev_id_t devid,
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
  BF_TM_SET_DEFAULT_VAL(defaults, glb_max_cell_limit, BF_TM_IG_GLB_CELL_LIMIT)
  BF_TM_SET_DEFAULT_VAL(defaults, glb_max_cell_limit_en, true)

  if (g_tm_ctx[devid]->target == BF_TM_TARGET_MODEL) {
    BF_TM_SET_DEFAULT_VAL(defaults, skid_hysteresis, 0)
  } else {
    BF_TM_SET_DEFAULT_VAL(
        defaults, skid_hysteresis, BF_TM_TOF2_WAC_RESET_HYSTERESIS * 8)
  }
  return BF_SUCCESS;
}
bf_tm_status_t bf_tm_tof2_pipe_get_defaults(bf_dev_id_t devid,
                                            bf_tm_eg_pipe_t *pipe,
                                            bf_tm_pipe_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_pipe_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, mirror_drop_enable, true)
  BF_TM_SET_DEFAULT_VAL(
      defaults, egress_limit_cells, g_tm_ctx[devid]->tm_cfg.total_cells)
  BF_TM_SET_DEFAULT_VAL(defaults, egress_hysteresis_cells, 0)
  if (pipe) {
    BF_TM_SET_DEFAULT_VAL(
        defaults, port_mirror_on_drop_dest, MAKE_DEV_PORT(pipe->l_pipe, 0))
    BF_TM_SET_DEFAULT_VAL(defaults, queue_mirror_on_drop_dest, 0)
  }
  BF_TM_SET_DEFAULT_VAL(defaults, pkt_ifg_compensation, BF_TM_IFG_COMPENSATION)
  BF_TM_SET_DEFAULT_VAL(
      defaults, qstat_report_mode, !(TM_IS_TARGET_ASIC(devid)))
  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tof2_sch_q_get_defaults(bf_dev_id_t devid,
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
      case BF_SPEED_40G_R2:
        rate = BF_TM_RATE_40G;
        break;
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        rate = BF_TM_RATE_50G;
        break;
      case BF_SPEED_100G:
        rate = BF_TM_RATE_100G;
        break;
      case BF_SPEED_200G:
        rate = BF_TM_RATE_200G;
        break;
      case BF_SPEED_400G:
        rate = BF_TM_RATE_400G;
        break;
      case BF_SPEED_NONE:
        // like bf_tm_tof2_set_default_for_port() does
        rate = BF_TM_RATE_400G;
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
  if (p) {
    BF_TM_SET_DEFAULT_VAL(
        defaults, sch_q_guaranteed_rate, (p->offline) ? BF_TM_RATE_400G : 0)
  }
  BF_TM_SET_DEFAULT_VAL(defaults, sch_q_adv_fc_mode, BF_TM_SCH_ADV_FC_MODE_CRE)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_adv_fc_mode_enable, false)

  return rc;
}

bf_tm_status_t bf_tm_tof2_sch_port_get_defaults(
    bf_dev_id_t devid, bf_tm_port_t *p, bf_tm_sch_port_defaults_t *defaults) {
  (void)devid;
  bf_tm_status_t rc = BF_SUCCESS;
  memset(defaults, 0, sizeof(bf_tm_sch_port_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_port_shaping_rate_prov_type, BF_TM_SCH_RATE_UPPER)
  if (p) {
    uint32_t rate = 0;
    uint32_t burst_size = BF_TM_PORT_SCH_BURST_SIZE;
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
      case BF_SPEED_40G_R2:
        rate = BF_TM_RATE_40G;
        break;
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        rate = BF_TM_RATE_50G;
        break;
      case BF_SPEED_100G:
        rate = BF_TM_RATE_100G;
        break;
      case BF_SPEED_200G:
        rate = BF_TM_RATE_200G;
        break;
      case BF_SPEED_400G:
        burst_size = BF_TM_Q_SCH_BURST_SIZE;
        rate = BF_TM_RATE_400G;
        break;
      case BF_SPEED_NONE:
        // like bf_tm_tof2_set_default_for_port() does
        burst_size = BF_TM_Q_SCH_BURST_SIZE;
        rate = BF_TM_RATE_400G;
        break;
      default:
        rc = BF_NOT_SUPPORTED;
        break;
    }
    if (BF_SUCCESS == rc) {
      BF_TM_SET_DEFAULT_VAL(
          defaults, sch_port_shaping_rate_burst_size, burst_size)
      BF_TM_SET_DEFAULT_VAL(defaults, sch_port_shaping_rate, rate)
    }
  }

  return rc;
}

bf_tm_status_t bf_tm_tof2_sch_l1_get_defaults(
    bf_dev_id_t devid,
    bf_tm_port_t *p,
    bf_tm_eg_l1_t *l1,
    bf_tm_sch_l1_defaults_t *defaults) {
  (void)p;
  memset(defaults, 0, sizeof(bf_tm_sch_l1_defaults_t));

  if (l1) {
    if (!(TM_IS_TARGET_ASIC(devid)) &&
        (g_tm_ctx[devid]->tm_cfg.ports_per_pg) > l1->logical_l1) {
      // on SW model 'default' L1 Nodes use settings from child queues.
      BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_dwrr_weight, 1023)
    } else {
      BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_dwrr_weight, 0)
    }
  }
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_guaranteed_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_guaranteed_priority, BF_TM_SCH_PRIO_0)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_shaping_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_shaping_priority, BF_TM_SCH_PRIO_0)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_priority_prop_enable, false)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_guaranteed_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_l1_guaranteed_rate_burst_size, BF_TM_L1_SCH_BURST_SIZE)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_l1_guaranteed_rate, BF_TM_L1_SCH_DEFAULT_RATE)
  BF_TM_SET_DEFAULT_VAL(defaults, sch_l1_shaping_rate_pps, false)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_l1_shaping_rate_burst_size, BF_TM_L1_SCH_BURST_SIZE)
  BF_TM_SET_DEFAULT_VAL(
      defaults, sch_l1_shaping_rate, BF_TM_L1_SCH_DEFAULT_RATE)

  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tof2_ppg_get_defaults(bf_dev_id_t devid,
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
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS : 0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(
          defaults, pool_max_cells, BF_TM_APP_POOL_0_PPG_BASE_LIMIT)
      BF_TM_SET_DEFAULT_VAL(defaults, dynamic_baf, BF_TM_PPG_BAF_80_PERCENT)
    } else {
      BF_TM_SET_DEFAULT_VAL(defaults, min_limit_cells, 0)
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          hysteresis_cells,
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS : 0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool_max_cells, 0)
      BF_TM_SET_DEFAULT_VAL(defaults, dynamic_baf, BF_TM_PPG_BAF_DISABLE)
    }
  }

  return BF_SUCCESS;
}

static bf_status_t bf_tm_tof2_set_default_for_ig_pool(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;

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
    goto cleanup;
  }
  rc = bf_tm_pool_uc_cut_through_size_set(dev, BF_TM_UC_CT_POOL_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set default UC cut through size");
    goto cleanup;
  }
  rc = bf_tm_pool_mc_cut_through_size_set(dev, BF_TM_MC_CT_POOL_SIZE);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set default MC cut through size");
    goto cleanup;
  }
  rc = bf_tm_global_max_limit_set(dev, BF_TM_IG_GLB_CELL_LIMIT);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set global cell limit");
    goto cleanup;
  }
  rc = bf_tm_global_max_limit_enable(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not enable global cell limit");
    goto cleanup;
  }
cleanup:
  return (rc);
}

static bf_status_t bf_tm_tof2_set_default_for_ppg(bf_dev_id_t dev) {
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
                                        BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg app pool usage for ppg %d", ppg);
      }
#if DEVICE_IS_EMULATOR
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, 32);
#else
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, BF_TM_DEFAULT_PPG_LMT);
#endif
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
      rc = bf_tm_ppg_app_pool_usage_set(dev,
                                        ppg,
                                        BF_TM_IG_APP_POOL_0,
                                        BF_TM_APP_POOL_0_PPG_BASE_LIMIT,
                                        BF_TM_PPG_BAF_80_PERCENT,
                                        0);  // No hyst, 6% BAF
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set ppg app pool usage for ppg %d", ppg);
      }
#if DEVICE_IS_EMULATOR
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, 32);
#else
      rc = bf_tm_ppg_guaranteed_min_limit_set(dev, ppg, BF_TM_DEFAULT_PPG_LMT);
#endif
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set gmin limit for ppg %d", ppg);
      }
      rc = bf_tm_ppg_guaranteed_min_skid_hysteresis_set(dev, ppg, 0);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to set min/skid hysteresis for PPG (%d)", ppg);
      }
    }
  }
  // No default setting for non default PPG (0 - 127 in case of tof2).
  // They are user managed
  return (rc);
}

static bf_tm_status_t bf_tm_tof2_caa_set_rsvd_blocks(bf_dev_id_t devid,
                                                     uint8_t blocks) {
  bf_tm_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  setp_tof2_wac_caa_rsvd_blocks_blks(&val, blocks);
  rc = bf_tm_write_register(devid,
                            offsetof(tof2_reg,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_caa_rsvd_blocks),
                            val);
  return (rc);
}

static bf_tm_status_t bf_tm_tof2_caa_set_almost_full_threshold(
    bf_dev_id_t devid, uint8_t threshold) {
  bf_tm_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  setp_tof2_caa_fa_almost_full_threshold_r_value(&val, threshold);
  rc = bf_tm_write_register(
      devid,
      offsetof(tof2_reg, device_select.tm_top.tm_caa_top.almost_full_threshold),
      val);
  return (rc);
}

static bf_tm_status_t bf_tm_tof2_set_full_threshold(bf_dev_id_t devid,
                                                    uint16_t threshold) {
  bf_tm_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  setp_tof2_psc_fa_full_threshold_r_value(&val, threshold);
  rc = bf_tm_write_register(
      devid,
      offsetof(tof2_reg,
               device_select.tm_top.tm_psc_top.psc_common.full_threshold),
      val);
  return (rc);
}

static bf_tm_status_t bf_tm_tof2_caa_set_skid_limit(bf_dev_id_t devid,
                                                    uint16_t cells) {
  bf_tm_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  setp_tof2_wac_caa_block_hdr_cell_limit(&val, cells);
  rc = bf_tm_write_register(devid,
                            offsetof(tof2_reg,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_caa_block_hdr_cell),
                            val);
  return (rc);
}

static bf_tm_status_t bf_tm_tof2_ig_set_mc_ct_cells(bf_dev_id_t devid,
                                                    uint16_t cells) {
  bf_tm_status_t rc = BF_SUCCESS;

  // Read the register first
  uint32_t val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(tof2_reg, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      &val);

  if (rc != BF_SUCCESS) {
    return (rc);
  }

  // Modify and write
  setp_tof2_qclc_ct_tot_mc_th(&val, cells);
  rc = bf_tm_write_register(
      devid,
      offsetof(tof2_reg, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      val);

  return (rc);
}

static bf_status_t bf_tm_tof2_set_ingress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *ports;
  bf_dev_port_t lport;
  int p;
  int j;
  uint32_t num_pipes;
  bf_dev_pipe_t phy_pipe = 0;

  // Clear table entries that were supposed to be power on reset 0x0
  // but some how did not.
  // tof2 TM cfg space will always have 4 pipes regardless of skew
  for (j = 0; j < 4; j++) {
    for (p = 0; p < 128; p++) {
      uint64_t hi = 0, lo = 0;
      setp_tof2_wac_ppg_icos_entry_icos(&lo, 0);

      uint64_t indir_addr =
          tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_icos(j, p);
      rc |= bf_tm_write_memory(dev, indir_addr, 4, hi, lo);
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
  rc |= bf_tm_tof2_set_default_for_ig_pool(dev);
  // Default setting for ppg/s
  rc |= bf_tm_tof2_set_default_for_ppg(dev);

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
      bf_tm_port_set_wac_drop_limit(dev, ports, BF_TM_TOF2_BUFFER_CELLS);
      bf_tm_port_set_wac_hyst(dev, ports, BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS);
      bf_tm_port_set_skid_limit(dev, ports, BF_TM_TOF2_BUFFER_CELLS);
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
      bf_tm_port_set_wac_drop_limit(dev, ports, BF_TM_TOF2_BUFFER_CELLS);
      bf_tm_port_set_wac_hyst(dev, ports, BF_TM_TOF2_WAC_DEFAULT_HYSTERESIS);
    }

    /* Set WAC bypass egress queue drop states workaround. */
    if (g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_A0 ||
        g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_B0 ||
        g_tm_ctx[dev]->part_rev == BF_SKU_CHIP_PART_REV_B1) {
      if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &phy_pipe) != LLD_OK) {
        LOG_ERROR(
            "Unable to map logical pipe to physical pipe id. dev:%d "
            "logical pipe:%d",
            dev,
            j);
      } else {
        LOG_TRACE(
            "WAC egress queue drop state workaround for TF-2 rev.%d(%s) on "
            "dev:%d logical pipe:%d",
            g_tm_ctx[dev]->part_rev,
            bf_sku_chip_part_rev_str(g_tm_ctx[dev]->part_rev),
            dev,
            j);
        rc |= bf_tm_write_register(
            dev,
            offsetof(tof2_reg,
                     device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                         .wac_reg.wac_bypass_config),
            (uint32_t)0x2);
      }
    } else {
      LOG_WARN(
          "WAC egress queue drop state workaround NOT activated for TF-2 "
          "rev.%d(%s) on dev:%d logical pipe:%d",
          g_tm_ctx[dev]->part_rev,
          bf_sku_chip_part_rev_str(g_tm_ctx[dev]->part_rev),
          dev,
          j);
    }
  }

  // As per HW team, set CAA reserved blocks to 0x10
  bf_tm_tof2_caa_set_rsvd_blocks(dev, 0x10);

  // As per HW team, set CAA almost full threshold to 0x80
  bf_tm_tof2_caa_set_almost_full_threshold(dev, 0x80);

  // As per HW team, set CAA skid limit to 0x80
  bf_tm_tof2_caa_set_skid_limit(dev, 0x80);

  // As per HW team, set mc ct size to 0x8000
  bf_tm_tof2_ig_set_mc_ct_cells(dev, 0x8000);

  return (rc);
}

//////   EGRESS DEFFAULTS ///////////////////

static bf_status_t bf_tm_tof2_set_default_for_eg_pool(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;

  LOG_TRACE("TM: %s: Set default values for egress Pools", __func__);
  // Set APP pool default
  rc = bf_tm_pool_size_set(dev, BF_TM_EG_APP_POOL_0, BF_TM_EG_APP_POOL_0_SIZE);
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
        dev, BF_TM_EG_APP_POOL_0, i, BF_TM_EG_APP_POOL_0_SIZE);
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

  // Set APP pool3 to atleast 3000 cells.
  //
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

  for (uint8_t pipe = 0; pipe < g_tm_ctx[dev]->tm_cfg.pipe_cnt; pipe++) {
    for (uint8_t fifo = 0; fifo < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
         fifo++) {
      rc = bf_tm_pre_fifo_limit_set(dev, pipe, fifo, BF_TM_EG_PRE_FIFO_LIMIT);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s:%d Could not set pre fifo limit for dev %d, pipe "
            "%d, "
            "fifo "
            "%d, rc "
            "%s (%d)",
            __func__,
            __LINE__,
            dev,
            pipe,
            fifo,
            bf_err_str(rc),
            rc);
        goto cleanup;
      }
    }
  }

  rc = bf_tm_global_min_limit_set(dev, BF_TM_IG_GLB_MIN_LIMIT);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Could not set ig glb min limit");
  }
cleanup:
  return (rc);
}

static bf_status_t bf_tm_tof2_set_default_for_port(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t port;
  int lport;
  int j;
  uint32_t num_pipes;
  bf_dev_pipe_t phy_pipe = 0;

  LOG_TRACE("TM: %s: Set default values for egress ports", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      // rate = 400G
      // burst size = 16384 bytes (~2 jumbo packet)
      rc |= bf_tm_sched_port_shaping_rate_set(
          dev, port, false, BF_TM_Q_SCH_BURST_SIZE, 400000000);
      rc |= bf_tm_sched_port_enable(dev, port, BF_SPEED_10G);

      if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &phy_pipe) != LLD_OK) {
        LOG_ERROR(
            "Unable to map logical pipe to physical pipe id. device = %d "
            "logical "
            "pipe = %d",
            dev,
            j);
        /* continue for other pipes */
        continue;
      }
      // Initialize the PEX PFC map for each PFC priority
      uint32_t port_pex_pfc_map_base = lport * BF_TM_MAX_PFC_LEVELS;
      for (int pfc_pri = 0; pfc_pri < BF_TM_MAX_PFC_LEVELS; pfc_pri++) {
        uint32_t val = (1 << pfc_pri);
        bf_tm_write_register(
            dev,
            offsetof(tof2_reg,
                     device_select.tm_top.tm_pex_top.pex[phy_pipe]
                         .pex_pfc_map_table[port_pex_pfc_map_base + pfc_pri]),
            val);
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_tof2_set_sch_default_for_q(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_queue_t q) {
  bf_status_t rc = BF_SUCCESS;

  rc |= bf_tm_sched_q_priority_set(dev, port, q, BF_TM_SCH_PRIO_0);
  rc |= bf_tm_sched_q_dwrr_weight_set(dev, port, q, 1023);
  // rate set to 400G, burst size = 16384bytes
  rc |= bf_tm_sched_q_shaping_rate_set(
      dev, port, q, false, BF_TM_Q_SCH_BURST_SIZE, 400000000);
  // rate set to 400G, burst size = 16384bytes
  rc |= bf_tm_sched_q_guaranteed_rate_set(
      dev, port, q, false, BF_TM_Q_SCH_BURST_SIZE, 400000000);
  rc |= bf_tm_sched_q_remaining_bw_priority_set(dev, port, q, BF_TM_SCH_PRIO_0);
  rc |= bf_tm_sched_q_enable(dev, port, q);

  return (rc);
}

static bf_status_t bf_tm_tof2_set_default_for_q(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_queue_t q;
  int lport, i, j;
  uint32_t num_pipes;
  bf_dev_port_t port;
  int color_start = BF_TM_COLOR_GREEN;
  int color_end = BF_TM_COLOR_RED;

  LOG_TRACE("TM: %s: Set default values for egress queues", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      rc = bf_tm_port_q_mapping_set(
          dev, port, BF_TM_TOF2_MIN_QUEUES_PER_PORT, NULL);
      if (rc != BF_SUCCESS) {
        // Not able to carve queues for port..
        // Running out of queue profiles ???
        LOG_ERROR("TM: %s: Could not carve queues for port %d", __func__, port);
        break;
      }
      for (q = 0; q < BF_TM_TOF2_MIN_QUEUES_PER_PORT; q++) {
        rc = bf_tm_q_app_pool_usage_set(dev,
                                        port,
                                        q,
                                        BF_TM_EG_APP_POOL_0,
                                        BF_TM_APP_POOL_0_Q_BASE_LIMIT,
                                        BF_TM_Q_BAF_80_PERCENT,
                                        BF_TM_TOF2_QAC_DEFAULT_HYSTERESIS);
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
          rc = bf_tm_q_color_drop_disable(dev, port, q);
          if (rc != BF_SUCCESS) {
            LOG_ERROR(
                "TM: %s: Could not disable queue color drop for pipe %d port "
                "%d, q %d",
                __func__,
                j,
                lport,
                q);
            return (rc);
          }
        }
        // scheduler related  defaults on queue
        rc = bf_tm_tof2_set_sch_default_for_q(dev, port, q);
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

static bf_status_t bf_tm_tof2_set_egress_parser_fifo_cred(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  // Need to set for both A0, B0 for TOF2
  setp_tof2_qac_queue_parser_fifo_cred_value(&val, 0x08080808);
  rc =
      bf_tm_write_register(dev,
                           offsetof(tof2_reg,
                                    device_select.tm_top.tm_qac_top.qac_common
                                        .qac_common.qac_queue_parser_fifo_cred),
                           val);
  return (rc);
}

static bf_status_t bf_tm_tof2_set_egress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  int j, priority;
  bf_dev_pipe_t pipe = 0;
  uint32_t num_pipes;
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
  if (lld_sku_get_num_active_pipes(dev, &num_pipes) != LLD_OK) {
    LOG_ERROR(
        "%s: Failed to get number of active pipes for dev %d", __func__, dev);
    return BF_INVALID_ARG;
  }

  for (j = 0; j < (int)num_pipes; j++) {
    bf_tm_pipe_egress_limit_set(dev, j, BF_TM_TOF2_BUFFER_CELLS);  // set to max
    bf_tm_pipe_egress_hysteresis_set(dev, j, 0);
    bf_tm_sched_pkt_ifg_compensation_set(dev, j, 20);  // set the same as TF1
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

      rc = bf_tm_port_set_qac_drop_limit(dev, ports, BF_TM_TOF2_BUFFER_CELLS);
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
          dev, ports, BF_TM_TOF2_QAC_DEFAULT_HYSTERESIS);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s: Unable to set egress port hysteresis dev(%d) pipe(%d) "
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

  // JIRA DRV-3401. TM Queue depth reporter per pipe. Keeping this value in sync
  // with the ingress parser FIFO queue depath.
  rc = bf_tm_tof2_set_egress_parser_fifo_cred(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: %s: Unable to set egress parser fifo cred dev(%d) status 0x%0x",
        __func__,
        dev,
        rc);
    return (rc);
  }

  // Default setting for pool/s from egress TM prespective
  rc = bf_tm_tof2_set_default_for_eg_pool(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress pools");
    return (rc);
  }
  // Default setting for queue/s
  rc = bf_tm_tof2_set_default_for_q(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for queues");
    return (rc);
  }
  // deafult setting for port
  rc = bf_tm_tof2_set_default_for_port(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for ports");
    return (rc);
  }

  /*
   * As per HW team, set QAC Egress Buffer MC PRE Per Priority FIFO
   * pkt Max Limit to 0xc0
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
        for (priority = 0; priority < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
             priority++) {
          rc |= bf_tm_write_register(
              dev,
              offsetof(tof2_reg,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe0[priority]),
              0xc0);
        }
        break;

      case 1:
        for (priority = 0; priority < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
             priority++) {
          rc |= bf_tm_write_register(
              dev,
              offsetof(tof2_reg,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe1[priority]),
              0xc0);
        }
        break;

      case 2:
        for (priority = 0; priority < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
             priority++) {
          rc |= bf_tm_write_register(
              dev,
              offsetof(tof2_reg,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe2[priority]),
              0xc0);
        }
        break;

      case 3:
        for (priority = 0; priority < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
             priority++) {
          rc |= bf_tm_write_register(
              dev,
              offsetof(tof2_reg,
                       device_select.tm_top.tm_qac_top.qac_common.qac_common
                           .qac_pre_fifo_limit_pkt_pipe3[priority]),
              0xc0);
        }
        break;

      default:
        /* Do nothing */
        break;
    }

    if (rc != BF_SUCCESS) {
      LOG_ERROR("%s: Failed to set PRE FIFO priority for dev %d, pipe %d",
                __func__,
                dev,
                pipe);
      return (rc);
    }
  }

  return (rc);
}

static int scan_caa_ready(bf_dev_id_t dev, uint32_t *valid_blocks) {
  uint32_t done, i;
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_0_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[0] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_1_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[1] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_2_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[2] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_3_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[3] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_4_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[4] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_5_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[5] & (0x1lu << i)) &&
        !getp_tof2_caa_block_ready_r_value(&done, i))
      return 0;
  }

  return 1;
}

static int scan_qlc_ready(bf_dev_id_t dev) {
  uint32_t done, i;
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_0_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof2_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_1_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof2_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_2_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof2_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }

  return 1;
}

static int scan_psc_ready(bf_dev_id_t dev, uint32_t *valid_blocks) {
  uint32_t done, i;
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_ready.block_ready_0_3),
                      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[0] & (0x1lu << i)) &&
        !getp_tof2_psc_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_ready.block_ready_1_3),
                      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[1] & (0x1lu << i)) &&
        !getp_tof2_psc_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_ready.block_ready_2_3),
                      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[2] & (0x1lu << i)) &&
        !getp_tof2_psc_block_ready_r_value(&done, i))
      return 0;
  }

  return 1;
}

static bf_status_t bf_tm_tof2_init_seq(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val, i, j;
  uint32_t done;

  uint32_t caa_valid_blocks[6] = {0};
  uint32_t psc_valid_blocks[3] = {0};

  // Get CAA Valid Blocks
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_0_6),
      &caa_valid_blocks[0]);
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_1_6),
      &caa_valid_blocks[1]);
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_2_6),
      &caa_valid_blocks[2]);
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_3_6),
      &caa_valid_blocks[3]);
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_4_6),
      &caa_valid_blocks[4]);
  bf_tm_read_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_valid.block_valid_5_6),
      &caa_valid_blocks[5]);

  // Get PSC Valid Blocks
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_valid.block_valid_0_3),
                      &psc_valid_blocks[0]);
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_valid.block_valid_1_3),
                      &psc_valid_blocks[1]);
  bf_tm_read_register(dev,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_psc_top.psc_common
                                   .block_valid.block_valid_2_3),
                      &psc_valid_blocks[2]);

  // Take caa block out of reset
  uint32_t reg[6] = {0};
  for (i = 0; i < 6; i++) {
    for (j = 0; j < 32; j++) {
      if (caa_valid_blocks[i] & (0x1lu << j)) {
        setp_tof2_caa_block_reset_r_value(reg + i, j, 0);
      }
    }
  }
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_0_6),
      reg[0]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_1_6),
      reg[1]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_2_6),
      reg[2]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_3_6),
      reg[3]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_4_6),
      reg[4]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_reset.block_reset_5_6),
      reg[5]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg, device_select.tm_top.tm_caa_top.ctrl),
                       0x1 | (15 << 8));

  // Take qlm block out of reset
  TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 32; j++) {
      setp_tof2_qlm_blk_reset_blk_reset(reg + i, j, 0);
    }
  }
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_qlc_top.qlc_common
                                    .qlm_blk_reset.qlm_blk_reset_0_3),
                       reg[0]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_qlc_top.qlc_common
                                    .qlm_blk_reset.qlm_blk_reset_1_3),
                       reg[1]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_qlc_top.qlc_common
                                    .qlm_blk_reset.qlm_blk_reset_2_3),
                       reg[2]);

  // Take psc block out of reset
  TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 32; j++) {
      if (psc_valid_blocks[i] & (0x1lu << j)) {
        setp_tof2_psc_block_reset_r_value(reg + i, j, 0);
      }
    }
  }
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_reset.block_reset_0_3),
                       reg[0]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_reset.block_reset_1_3),
                       reg[1]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_reset.block_reset_2_3),
                       reg[2]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg, device_select.tm_top.tm_psc_top.psc_common.ctrl),
      0x1 | (15 << 8));

  // TM driver is communicating with real ASIC/Emulator/RTL-SIM
  // Hence its possible to read back chip progress.

  // check caa-ready-before  enabling caa
  done = 0;
  while (!done) {
    done = scan_caa_ready(dev, caa_valid_blocks);
    LOG_TRACE("CAA block Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("CAA block Ready");

  // check qlc-ready (qlc_top.qlc_common.blk_rdy)
  done = 0;
  while (!done) {
    done = scan_qlc_ready(dev);
    LOG_TRACE("QLC block[0] Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }

  // check psc-ready-before  enabling psc
  done = 0;
  while (!done) {
    done = scan_psc_ready(dev, psc_valid_blocks);
    LOG_TRACE("PSC block Ready Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("PSC block Ready");
  // Enable psc block
  TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 32; j++) {
      if (psc_valid_blocks[i] & (0x1lu << j)) {
        setp_tof2_psc_block_enable_r_value(reg + i, j, 1);
      }
    }
  }
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_enable.block_enable_0_3),
                       reg[0]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_enable.block_enable_1_3),
                       reg[1]);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_psc_top.psc_common
                                    .block_enable.block_enable_2_3),
                       reg[2]);

  // Enable caa block
  TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

  for (i = 0; i < 6; i++) {
    for (j = 0; j < 32; j++) {
      if (caa_valid_blocks[i] & (0x1lu << j)) {
        setp_tof2_caa_block_enable_r_value(reg + i, j, 1);
      }
    }
  }
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_0_6),
      reg[0]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_1_6),
      reg[1]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_2_6),
      reg[2]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_3_6),
      reg[3]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_4_6),
      reg[4]);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg,
               device_select.tm_top.tm_caa_top.block_enable.block_enable_5_6),
      reg[5]);

  ////////////   Trigger mem inits of WAC, QAC, SCH, PRC/PRM /////////////
  // Trigger WAC mem init
  val = 0xf0;
  bf_tm_write_register(
      dev,
      offsetof(
          tof2_reg,
          device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_glb_config),
      val);
  // Trigger QAC mem init
  val = 0;
  setp_tof2_qac_mem_init_en_enable(&val, 0xf);
  bf_tm_write_register(dev,
                       offsetof(tof2_reg,
                                device_select.tm_top.tm_qac_top.qac_common
                                    .qac_common.qac_mem_init_en),
                       val);
  // Trigger SCH mem init
  val = 0;
  for (i = 0; i < 2; i++) {
    bf_tm_read_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[i].ctrl),
        &val);
    setp_tof2_sch_ctrl_r_hw_init_enb(&val, 1);
    bf_tm_write_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[i].ctrl),
        val);

    bf_tm_read_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[i].ctrl),
        &val);
    setp_tof2_sch_ctrl_r_hw_init_enb(&val, 1);
    bf_tm_write_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[i].ctrl),
        val);
  }

  // Trigger PRC/PRM mem init
  val = 0;
  setp_tof2_prc_control_map_init(&val, 1);
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg, device_select.tm_top.tm_prc_top.prc_common.control),
      val);

  // TM driver is communicating with real ASIC/Emulator/RTL-SIM
  // Hence its possible to read back chip progress.
  ///////////////   Check mem init Done for WAC, QAC, SCH, PRC //////////////
  done = 0;
  while (!done) {
    bf_tm_read_register(dev,
                        offsetof(tof2_reg,
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
                        offsetof(tof2_reg,
                                 device_select.tm_top.tm_qac_top.qac_common
                                     .qac_common.qac_mem_init_done),
                        &done);
    LOG_TRACE("Qac Mem init Done Status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("Qac Mem init Complete ");
  uint32_t doneA;
  uint32_t doneB;
  for (i = 0; i < 2; i++) {
    doneA = 0;
    doneB = 0;
    while (((doneA & 0x7ff) != 0x7ff) || ((doneB & 0x7ff) != 0x7ff)) {
      bf_tm_read_register(
          dev,
          offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[i].ready),
          &doneA);
      bf_tm_read_register(
          dev,
          offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[i].ready),
          &doneB);
      LOG_TRACE("SCH A PIPE %d Mem init Done Status = %#x", i, doneA);
      LOG_TRACE("SCH B PIPE %d Mem init Done Status = %#x", i, doneB);
      if (((doneA & 0x7ff) != 0x7ff) || ((doneB & 0x7ff) != 0x7ff))
        bf_sys_usleep(100);
    }
  }
  LOG_TRACE("SCH Mem init Complete ");
  done = 0;
  while (!done) {
    bf_tm_read_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_prc_top.prc_common.status),
        &done);
    done = getp_tof2_prc_status_init_done(&done);
    LOG_TRACE("PRC Map Mem init done status = %#x", done);
    if (!done) bf_sys_usleep(100);
  }
  LOG_TRACE("PRC Map Mem init Complete ");

  return (rc);
}

bf_status_t bf_tm_tof2_start_init_seq_during_fast_recfg(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  // Init seq to be applied before configuring TM
  // Since init sequence is interactive, (deassert block reset, check for
  // init done etc), read/writes to TM will be via normal pcie read/writes
  // no DMA at this point.
  g_tm_ctx[dev]->batch_mode = false;
  g_tm_ctx[dev]->fast_reconfig_init_seq = true;
  rc = bf_tm_tof2_init_seq(dev);
  g_tm_ctx[dev]->batch_mode = true;
  g_tm_ctx[dev]->fast_reconfig_init_seq = false;

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}

bf_status_t bf_tm_tof2_set_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_tof2_log_defaults();

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
    rc = bf_tm_tof2_init_seq(dev);
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
      bf_tm_ucli_power_on_reset_per_pipe_tbls(dev, i);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("TM: Unable to Reset tables to POR value on TM");
        goto cleanup;
      }
    }
    bf_tm_power_on_reset_non_pipe_tbls(dev);
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
  rc = bf_tm_tof2_set_ingress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Ingress TM");
    goto cleanup;
  }
  // Egress
  rc = bf_tm_tof2_set_egress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress TM");
    goto cleanup;
  }

  // Setup the timestamp offset.
  bf_tm_write_register(
      dev,
      offsetof(tof2_reg, device_select.tm_top.tm_psc_top.psc_common.ts_offset),
      31);

  // As per HW team, set global full threshold for address allocation to 0x39c
  bf_tm_tof2_set_full_threshold(dev, 0x39c);

  // As per HW team, set the number of packets inflight per MAC Group to 0x36
  for (int i = 0; i < 2; i++) {
    uint32_t val = 0;
    rc |= bf_tm_read_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_scha_top.sch[i].pex_credit_ctrl),
        &val);
    setp_tof2_sch_pex_credit_ctrl_r_pex_mac_credit(&val, 0x36);
    rc |= bf_tm_write_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_scha_top.sch[i].pex_credit_ctrl),
        val);

    rc |= bf_tm_read_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_schb_top.sch[i].pex_credit_ctrl),
        &val);
    setp_tof2_sch_pex_credit_ctrl_r_pex_mac_credit(&val, 0x36);
    rc |= bf_tm_write_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_schb_top.sch[i].pex_credit_ctrl),
        val);
  }

  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: dev %d: Failed to set the number of packets inflight per MAC "
        "Group to 0x36",
        __func__,
        dev);
    return (rc);
  }

  // Set mem_init_en bit to 1 in CLC pipe_ctrl register for all pipes
  uint32_t num_pipes, pipe;
  uint32_t val = (1 << 29);  // Bit29 is mem_init_en
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    rc |= bf_tm_write_register(
        dev,
        offsetof(tof2_reg, device_select.tm_top.tm_clc_top.clc[pipe].pipe_ctrl),
        val);
  }

  // As per HW team, set the clk tick to insert ct_clm for <=50g ports to 0x15
  for (int j = 0; j < (int)num_pipes; j++) {
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

    val = 0;
    bf_tm_read_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pex_top.pex[pipe].cfg_ct_timer),
        &val);
    setp_tof2_cfg_ct_timer_thr_50g(&val, 0x15);
    bf_tm_write_register(
        dev,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pex_top.pex[pipe].cfg_ct_timer),
        val);
  }

  bf_tm_flush_wlist(dev);  // Push any buffered writes to HW
cleanup:
  g_tm_ctx[dev]->internal_call = false;
  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}
