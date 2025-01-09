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
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include "traffic_mgr/common/tm_hw_access.h"

#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_mem_addr.h>
#include <lld/lld_err.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_sku.h>

/*
 *  This file implements initializing TM with default setting.
 *  Defaults are set at the device init time (device-add)
 */

#include "tm_tof3.h"
#include "tm_tof3_default.h"

static void bf_tm_tof3_log_defaults() {
  LOG_TRACE("TM: Cells for 2 pkts %d", BF_TM_CELLS_FOR_2_PKT);
  LOG_TRACE("TM: Default PPG limit %d", BF_TM_DEFAULT_PPG_LMT);
  LOG_TRACE("TM: PPG limit %d", BF_TM_PPG_LMT);
  LOG_TRACE("TM: Ingress Gmin pool size %d", BF_TM_IG_GMIN_POOL_SIZE);
  LOG_TRACE("TM: Ingress pool0 size %d", BF_TM_IG_APP_POOL_0_SIZE);
  LOG_TRACE("TM: Ppg base limit in pool0 %d", BF_TM_APP_POOL_0_PPG_BASE_LIMIT);
  LOG_TRACE("TM: Mirror pool size %d", BF_TM_NEG_MIRROR_POOL_SIZE);
  LOG_TRACE("TM: Egress Q Gmin %d", BF_TM_Q_GMIN_LMT);
  LOG_TRACE("TM: Egress Gmin pool size %d", BF_TM_EG_GMIN_POOL_SIZE);
  LOG_TRACE("TM: Egress pool0 size %d", BF_TM_EG_APP_POOL_0_SIZE);
  LOG_TRACE("TM: Egress base limit in pool0 %d", BF_TM_APP_POOL_0_Q_BASE_LIMIT);
}

bf_tm_status_t bf_tm_tof3_port_get_defaults(bf_dev_id_t devid,
                                            bf_tm_port_t *p,
                                            bf_tm_port_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_port_defaults_t));
  BF_TM_SET_DEFAULT_VAL(defaults, port_icos_mask, 0xff);
  BF_TM_SET_DEFAULT_VAL(defaults, port_ct_enable, false);
  BF_TM_SET_DEFAULT_VAL(defaults, port_ig_limit, BF_TM_TOF3_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(defaults, port_eg_limit, BF_TM_TOF3_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(defaults, port_skid_limit, BF_TM_TOF3_BUFFER_CELLS);
  BF_TM_SET_DEFAULT_VAL(
      defaults,
      port_ig_hysteresis,
      (TM_IS_TARGET_ASIC(devid)) ? (BF_TM_TOF3_WAC_DEFAULT_HYSTERESIS) : 0);
  BF_TM_SET_DEFAULT_VAL(
      defaults, port_eg_hysteresis, BF_TM_TOF3_QAC_DEFAULT_HYSTERESIS);
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
      case BF_SPEED_200G:
        limit_cells = 0x7f;
        break;
      case BF_SPEED_400G:
        limit_cells = 0x7f;
        break;
      case BF_SPEED_NONE:
        limit_cells = 8;
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

bf_tm_status_t bf_tm_tof3_q_get_defaults(bf_dev_id_t devid,
                                         bf_tm_q_defaults_t *defaults) {
  (void)devid;  // for future purposes
  memset(defaults, 0, sizeof(bf_tm_q_defaults_t));

  BF_TM_SET_DEFAULT_VAL(defaults, q_gmin_limit, BF_TM_Q_GMIN_LMT)
  BF_TM_SET_DEFAULT_VAL(defaults, q_tail_drop, true)
  BF_TM_SET_DEFAULT_VAL(defaults, q_app_pool, BF_TM_EG_APP_POOL_0)
  BF_TM_SET_DEFAULT_VAL(defaults, q_dynamic_baf, BF_TM_Q_BAF_80_PERCENT)
  BF_TM_SET_DEFAULT_VAL(
      defaults, q_qac_hysteresis, BF_TM_TOF3_QAC_DEFAULT_HYSTERESIS)
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

bf_tm_status_t bf_tm_tof3_pool_get_defaults(bf_dev_id_t devid,
                                            bf_tm_app_pool_t *pool,
                                            bf_tm_pool_defaults_t *defaults) {
  memset(defaults, 0, sizeof(bf_tm_pool_defaults_t));
  if (pool) {
    if (*pool == BF_TM_IG_APP_POOL_0) {
      // TODO: This value depends on BF_TM_TOF3_MAU_PIPES(8)
      // and it becomes unreasonable small on 4 pipe SKU.
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
        defaults, skid_hysteresis, BF_TM_TOF3_WAC_RESET_HYSTERESIS * 8)
  }
  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tof3_pipe_get_defaults(bf_dev_id_t devid,
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
  BF_TM_SET_DEFAULT_VAL(defaults, qstat_report_mode, false)
  return BF_SUCCESS;
}

bf_tm_status_t bf_tm_tof3_sch_q_get_defaults(bf_dev_id_t devid,
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
      case BF_SPEED_200G:
        rate = BF_TM_RATE_200G;
        break;
      case BF_SPEED_400G:
        rate = BF_TM_RATE_400G;
        break;
      case BF_SPEED_NONE:
        // like bf_tm_tof3_set_default_for_port() does
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

bf_tm_status_t bf_tm_tof3_sch_port_get_defaults(
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
        rate = BF_TM_RATE_40G;
        break;
      case BF_SPEED_50G:
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
        // like bf_tm_tof3_set_default_for_port() does
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

bf_tm_status_t bf_tm_tof3_sch_l1_get_defaults(
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

bf_tm_status_t bf_tm_tof3_ppg_get_defaults(bf_dev_id_t devid,
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
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOF3_WAC_DEFAULT_HYSTERESIS : 0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(
          defaults, pool_max_cells, BF_TM_APP_POOL_0_PPG_BASE_LIMIT)
      BF_TM_SET_DEFAULT_VAL(defaults, dynamic_baf, BF_TM_PPG_BAF_80_PERCENT)
    } else {
      BF_TM_SET_DEFAULT_VAL(defaults, min_limit_cells, 0)
      BF_TM_SET_DEFAULT_VAL(
          defaults,
          hysteresis_cells,
          (TM_IS_TARGET_ASIC(devid)) ? BF_TM_TOF3_WAC_DEFAULT_HYSTERESIS : 0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool, BF_TM_IG_APP_POOL_0)
      BF_TM_SET_DEFAULT_VAL(defaults, pool_max_cells, 0)
      BF_TM_SET_DEFAULT_VAL(defaults, dynamic_baf, BF_TM_PPG_BAF_DISABLE)
    }
  }

  return BF_SUCCESS;
}

static bf_status_t bf_tm_tof3_set_default_for_ig_pool(bf_dev_id_t dev) {
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

static bf_status_t bf_tm_tof3_set_default_for_ppg(bf_dev_id_t dev) {
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
      port = lld_sku_map_devport_from_device_to_user(dev, port);

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
                                        BF_TM_TOF3_WAC_DEFAULT_HYSTERESIS);
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
      port = lld_sku_map_devport_from_device_to_user(dev, port);
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
  // No default setting for non default PPG (0 - 127 in case of tof3).
  // They are user managed
  return (rc);
}

static bf_status_t bf_tm_tof3_set_ingress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc_cum = BF_SUCCESS;
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *ports;
  bf_dev_port_t lport;
  int p;
  int j;
  uint32_t num_pipes = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  // Clear table entries that were supposed to be power on reset 0x0
  // but some how did not.
  // tof3 TM cfg space will always have 4 pipes per die regardless of skew
  lld_err_t lld_rc = lld_sku_get_num_active_pipes(dev, &num_pipes);
  if (LLD_OK != lld_rc) {
    LOG_ERROR("%s:%d dev:%d, lld_rc=%d", __func__, __LINE__, dev, lld_rc);
    return (BF_INTERNAL_ERROR);
  }

  for (j = 0; j < (int)num_pipes; j++) {
    for (p = 0; p < BF_TM_TOF3_PPG_PER_PIPE; p++) {
      d_pipe = BF_TM_2DIE_D_PIPE(j, 0);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(j, 0);

      uint64_t hi = 0, lo = 0;
      setp_tof3_wac_ppg_icos_entry_icos(&lo, 0);

      uint64_t indir_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_icos(d_pipe, p);
      rc = bf_tm_subdev_write_memory(dev, subdev_id, indir_addr, 4, hi, lo);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d:%d, d_pipe:%d, ppg_nr:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  subdev_id,
                  d_pipe,
                  p,
                  rc,
                  bf_err_str(rc));
      }
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
  rc = bf_tm_tof3_set_default_for_ig_pool(dev);
  if (BF_SUCCESS != rc) {
    rc_cum |= rc;
    LOG_ERROR(
        "%s:%d dev:%d, rc=%d(%s)", __func__, __LINE__, dev, rc, bf_err_str(rc));
  }
  // Default setting for ppg/s
  rc = bf_tm_tof3_set_default_for_ppg(dev);
  if (BF_SUCCESS != rc) {
    rc_cum |= rc;
    LOG_ERROR(
        "%s:%d dev:%d, rc=%d(%s)", __func__, __LINE__, dev, rc, bf_err_str(rc));
  }

  // Set WAC and QAC port limits
  ports = g_tm_ctx[dev]->ports;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                     g_tm_ctx[dev]->tm_cfg.pg_per_pipe);
         p++) {
      lport = MAKE_DEV_PORT(j, p);
      lport = lld_sku_map_devport_from_device_to_user(dev, lport);
      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
        continue;
      }
      // Port limit should ideally be sum of PPG limits that are mapped to the
      // port.
      // Default port limit is set to MAX
      rc = bf_tm_port_set_wac_drop_limit(dev, ports, BF_TM_TOF3_BUFFER_CELLS);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
      }
      rc = bf_tm_port_set_skid_limit(dev, ports, BF_TM_TOF3_BUFFER_CELLS);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
      }
    }

    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.mirror_port_cnt); p++) {
      lport = MAKE_DEV_PORT(j, p + g_tm_ctx[dev]->tm_cfg.mirror_port_start);
      lport = lld_sku_map_devport_from_device_to_user(dev, lport);

      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
        continue;
      }
      rc = bf_tm_port_set_wac_drop_limit(dev, ports, BF_TM_TOF3_BUFFER_CELLS);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
      }
    }

    rc = bf_tm_pipe_deflection_port_enable_set(dev, j, true);
    if (BF_SUCCESS != rc) {
      rc_cum |= rc;
      LOG_ERROR("%s:%d dev:%d, pipe:%d, rc=%d(%s)",
                __func__,
                __LINE__,
                dev,
                j,
                rc,
                bf_err_str(rc));
    }
  }

  return (rc_cum == BF_SUCCESS) ? BF_SUCCESS : BF_INTERNAL_ERROR;
}

//////   EGRESS DEFFAULTS ///////////////////

static bf_status_t bf_tm_tof3_set_default_for_eg_pool(bf_dev_id_t dev) {
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

static bf_status_t bf_tm_tof3_init_pex_pipe_ctrl(bf_dev_id_t dev,
                                                 bf_subdev_id_t subdev_id,
                                                 uint32_t d_pipe) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  setp_tof3_pex_pipe_ctrl_tx_cnt_en(&val, 0x1);
  setp_tof3_pex_pipe_ctrl_tx_cnt_mode(&val, 0x2);
  setp_tof3_pex_pipe_ctrl_pack_sop_dis(&val, 0x0);
  setp_tof3_pex_pipe_ctrl_ct_sop_wait(&val, 0x1);
  setp_tof3_pex_pipe_ctrl_freeze_on_error(&val, 0x0);
  setp_tof3_pex_pipe_ctrl_ecc_dis(&val, 0x0);
  setp_tof3_pex_pipe_ctrl_afull_thrd0(&val, 0x18);
  setp_tof3_pex_pipe_ctrl_afull_thrd1(&val, 0x18);
  setp_tof3_pex_pipe_ctrl_pt_stage_prefetch_dis(&val, 0x1);
  setp_tof3_pex_pipe_ctrl_ipg_bytes(&val, 0x14);
  setp_tof3_pex_pipe_ctrl_discard_ph_error_drop_dis(&val, 0x1ul);

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[d_pipe].pipe_ctrl),
      val);

  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: %s: Could not set PEX pipe ctrl init for dev %d, "
        "subdev %d pipe %d",
        __func__,
        dev,
        subdev_id,
        d_pipe);
  }
  return rc;
}

static bf_status_t bf_tm_tof3_init_pex_shaper_ctrl(bf_dev_id_t dev,
                                                   bf_subdev_id_t subdev_id,
                                                   uint32_t d_pipe) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t tm_clk_period = 7692;
  uint32_t clk_pps_period = 7936;

  uint32_t dec_amt = 1000;
  uint32_t inc_amt =
      (uint32_t)(((double)(dec_amt * tm_clk_period) / clk_pps_period));
  uint32_t max_cred = ((inc_amt * 3) + 2 * dec_amt) / 2;

  uint32_t val = 0;
  setp_tof3_shaper_ctrl_max_cred(&val, max_cred);
  setp_tof3_shaper_ctrl_dec_amt(&val, dec_amt);
  setp_tof3_shaper_ctrl_inc_amt(&val, inc_amt);

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_pex_top.pex[d_pipe].pipe_shaper_ctrl),
      val);

  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: %s: Could not set PEX shaper ctrl init for dev %d, "
        "subdev %d pipe %d",
        __func__,
        dev,
        subdev_id,
        d_pipe);
  }

  return rc;
}

static bf_status_t bf_tm_tof3_init_edf_arb(bf_dev_id_t dev,
                                           bf_subdev_id_t subdev_id,
                                           uint32_t d_pipe) {
  bf_status_t rc = BF_SUCCESS;

  double clk_tm_period_full;
  double cycles_per_grant;
  uint32_t inc_amt;
  double mop_underrun_val;
  uint32_t pps_shaper_skid;
  uint32_t sop_scale_factor;
  double sop_headroom_cycles;
  double sop_sat_val_pre;
  uint32_t dec_amt;
  uint32_t mop_start_val;
  uint32_t sop_start_val;
  uint32_t sop_sat_val;
  uint32_t tm_clk_period = 7692;
  // EDF parameters
  clk_tm_period_full = (double)tm_clk_period / 10000;
  // Always use 200G to derive EDF settings
  cycles_per_grant = (double)(1 / (clk_tm_period_full * 200)) * 256 * 2 * 8;
  inc_amt = 32;  // always using 200g for this calculation
  pps_shaper_skid = 5;
  sop_scale_factor = 4;

  // EDF CSRs
  dec_amt = (uint32_t)inc_amt * (uint32_t)(cycles_per_grant);
  mop_start_val = dec_amt;
  // not CSRs, but need to come after dec_amt is assigned
  sop_headroom_cycles = (double)5984 / dec_amt;  // 6.23;
  mop_underrun_val = (uint32_t)dec_amt + mop_start_val;
  sop_sat_val_pre =
      (double)(uint32_t)(mop_underrun_val - (sop_headroom_cycles * inc_amt));

  sop_start_val = (uint32_t)sop_sat_val_pre - (36 * sop_scale_factor);
  sop_sat_val = (uint32_t)sop_sat_val_pre + (pps_shaper_skid * 16);

  for (uint32_t spd_idx = 0; spd_idx < 5; spd_idx++) {
    uint32_t val = 0, val1[2] = {0};
    switch (spd_idx) {
      case 0:
        setp_tof3_edf_arb_spd_ctrl0_r_inc_amt(&val, 4);
        break;
      case 1:
        setp_tof3_edf_arb_spd_ctrl0_r_inc_amt(&val, 8);
        break;
      case 2:
        setp_tof3_edf_arb_spd_ctrl0_r_inc_amt(&val, 16);
        break;
      case 3:
        setp_tof3_edf_arb_spd_ctrl0_r_inc_amt(&val, 32);
        break;
      case 4:
        setp_tof3_edf_arb_spd_ctrl0_r_inc_amt(&val, 64);
        break;
    }

    setp_tof3_edf_arb_spd_ctrl0_r_dec_amt(&val, dec_amt);
    setp_tof3_edf_arb_spd_ctrl1_r_sop_start_val(&val1, sop_start_val);
    setp_tof3_edf_arb_spd_ctrl1_r_mop_start_val(&val1, mop_start_val);
    setp_tof3_edf_arb_spd_ctrl1_r_sop_sat_val(&val1, sop_sat_val);

    rc = bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[d_pipe]
                     .edf_arb_spd_ctrl0[spd_idx]),
        val);

    rc |= bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[d_pipe]
                     .edf_arb_spd_ctrl1[spd_idx]
                     .edf_arb_spd_ctrl1_0_2),
        val1[0]);
    rc |= bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[d_pipe]
                     .edf_arb_spd_ctrl1[spd_idx]
                     .edf_arb_spd_ctrl1_1_2),
        val1[1]);

    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: %s: Could not set PEX EDF init for dev %d, "
          "subdev %d pipe %d",
          __func__,
          dev,
          subdev_id,
          d_pipe);
      return rc;
    }
  }
  return rc;
}

static bf_status_t bf_tm_tof3_init_mgd(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
    // For Port Speeds 50G, 100G, 200G, 400G
    uint32_t val[4] = {8, 16, 32, 64};
    for (int k = 0; k < 4; k++) {
      rc |= bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd
                       .mgd_cfg_max_fifo_size_per_rate[k]),
          val[k]);
    }
  }
  return rc;
}

static bf_status_t bf_tm_tof3_set_default_for_port(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t port;
  int lport;
  int j;
  uint32_t num_pipes, phy_pipe;
  bf_subdev_id_t subdev_id;
  bf_dev_pipe_t d_pipe;
  uint32_t d_inst;

  LOG_TRACE("TM: %s: Set default values for egress ports", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      port = lld_sku_map_devport_from_device_to_user(dev, port);

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

      for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
        // DIe 0 and Die 1 spex[0 - 3] rpex[4 - 7] initialized to same value.
        d_pipe = BF_TM_2DIE_D_PIPE(phy_pipe, d_inst);
        subdev_id = BF_TM_2DIE_SUBDEV_ID(phy_pipe, d_inst);

        // Initialize the PEX PFC map for each PFC priority
        uint32_t port_pex_pfc_map_base = lport * BF_TM_MAX_PFC_LEVELS;
        for (int pfc_pri = 0; pfc_pri < BF_TM_MAX_PFC_LEVELS; pfc_pri++) {
          uint32_t val = (1 << pfc_pri);

          bf_tm_subdev_write_register(
              dev,
              subdev_id,
              offsetof(tof3_reg,
                       device_select.tm_top.tm_pex_top.pex[d_pipe]
                           .pex_pfc_map_table[port_pex_pfc_map_base + pfc_pri]),
              val);
        }

        // Set the PEX Init for pipes
        rc |= bf_tm_tof3_init_pex_pipe_ctrl(dev, subdev_id, d_pipe);
        rc |= bf_tm_tof3_init_pex_shaper_ctrl(dev, subdev_id, d_pipe);
        rc |= bf_tm_tof3_init_edf_arb(dev, subdev_id, d_pipe);
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_tof3_set_buffer_default_for_q(bf_dev_id_t dev,
                                                       bf_dev_port_t port,
                                                       bf_tm_queue_t q) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q_descr;
  bool is_set = false;

  rc = bf_tm_q_get_descriptor(dev, port, q, &q_descr);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("dev:%d port:%d invalid queue:%d, rc=%d", dev, port, q, rc);
    return (rc);
  }

  BF_TM_OBJ_SET(
      g_tm_ctx[dev], q_descr, thresholds.min_limit, BF_TM_Q_GMIN_LMT, is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev],
                q_descr,
                thresholds.app_limit,
                BF_TM_APP_POOL_0_Q_BASE_LIMIT,
                is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev],
                q_descr,
                thresholds.app_hyst,
                BF_TM_TOF3_QAC_DEFAULT_HYSTERESIS,
                is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev],
                q_descr,
                thresholds.yel_limit_pcent,
                BF_TM_Q_COLOR_LIMIT_75_PERCENT,
                is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev],
                q_descr,
                thresholds.red_limit_pcent,
                BF_TM_Q_COLOR_LIMIT_75_PERCENT,
                is_set);

  BF_TM_OBJ_SET(g_tm_ctx[dev], q_descr, q_cfg.tail_drop_en, true, is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev], q_descr, q_cfg.color_drop_en, false, is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev],
                q_descr,
                q_cfg.app_poolid,
                0,
                is_set);  // BF_TM_EG_APP_POOL_0
  BF_TM_OBJ_SET(
      g_tm_ctx[dev], q_descr, q_cfg.baf, BF_TM_Q_BAF_80_PERCENT, is_set);
  BF_TM_OBJ_SET(g_tm_ctx[dev], q_descr, q_cfg.is_dynamic, true, is_set);

  if (is_set) {
    // Write the queue QAC config register once for all the settings.
    rc = bf_tm_q_set_qac_buffer(dev, q_descr);
  }
  return (rc);
}

static bf_status_t bf_tm_tof3_set_sch_default_for_q(bf_dev_id_t dev,
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

  return ((BF_SUCCESS != rc) ? BF_INTERNAL_ERROR : rc);
}

static bf_status_t bf_tm_tof3_set_default_for_q(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_queue_t q;
  int lport, j;
  uint32_t num_pipes;
  bf_dev_port_t port;
  bf_tm_eg_q_t *q_descr;

  LOG_TRACE("Set default values for Egress dev:%d", dev);

  // Low-level calls are ordered depending on what register size is
  // used on Tofino-3 to pack write operations into one DMA descriptor
  // block. Without that, the DRU write list becomes quickly exhausted
  // during warm init when the interleaved register wirites will take
  // descriptor and its DMA block for only one register value.
  // This function uses 508 of 2048 descriptors from the list.

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    LOG_TRACE(
        "Set default values for egress queues for dev:%d pipe:%d", dev, j);
    // 55 descriptors: 4-byte (of 33 items) and 8-byte (of 10 items)
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      port = lld_sku_map_devport_from_device_to_user(dev, port);
      rc = bf_tm_port_q_mapping_set(
          dev, port, BF_TM_TOF3_MIN_QUEUES_PER_PORT, NULL);
      if (BF_SUCCESS != rc) {
        // Running out of queue profiles ???
        LOG_ERROR("dev:%d pipe:%d port:%d queue mapping failed, rc=%d",
                  dev,
                  j,
                  port,
                  rc);
        return (rc);
      }
    }

    // 72 descriptors:
    // one 16-byte (of 16 items) and one 4-byte (129 items) per port.
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      port = lld_sku_map_devport_from_device_to_user(dev, port);

      // one 16-byte sized descriptor write with 16 items (one item per queue)
      for (q = 0; q < BF_TM_TOF3_MIN_QUEUES_PER_PORT; q++) {
        rc = bf_tm_tof3_set_buffer_default_for_q(dev, port, q);
        if (BF_SUCCESS != rc) {
          LOG_ERROR(
              "dev:%d pipe:%d port:%d queue:%d QAC buffer setup failed rc=%d",
              dev,
              j,
              port,
              q,
              rc);
          return (rc);
        }
      }

      // one 4-byte sized descriptor write with 129 items per port.
      for (q = 0; q < BF_TM_TOF3_MIN_QUEUES_PER_PORT; q++) {
        rc = bf_tm_q_get_descriptor(dev, port, q, &q_descr);
        if (BF_SUCCESS != rc) {
          LOG_ERROR("dev:%d pipe:%d port:%d invalid queue:%d, rc=%d",
                    dev,
                    j,
                    port,
                    q,
                    rc);
          return (rc);
        }
        // TM context is set, just write same values to WAC registers.
        // one 4B item per queue.
        rc = bf_tm_q_set_wac_buffer(dev, q_descr);
        if (BF_SUCCESS != rc) {
          LOG_ERROR("dev:%d pipe:%d port:%d queue:%d WAC setup failed rc=%d",
                    dev,
                    j,
                    port,
                    q,
                    rc);
          return (rc);
        }
        // 4B items: 7 for each queue + 1 more item for all port's queues.
        rc = bf_tm_tof3_set_sch_default_for_q(dev, port, q);
        if (BF_SUCCESS != rc) {
          LOG_ERROR(
              "dev:%d pipe:%d port:%d queue:%d scheduler setup failed rc=%d",
              dev,
              j,
              port,
              q,
              rc);
          return (rc);
        }
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_tof3_set_egress_tm_default(bf_dev_id_t dev) {
  bf_status_t rc_cum = BF_SUCCESS;
  bf_status_t rc = BF_SUCCESS;
  int j;
  uint32_t num_pipes;
  bf_subdev_id_t subdev_id;
  bf_dev_pipe_t d_pipe;

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
  lld_err_t lld_rc = lld_sku_get_num_active_pipes(dev, &num_pipes);
  if (LLD_OK != lld_rc) {
    LOG_ERROR("%s:%d dev:%d, lld_rc=%d", __func__, __LINE__, dev, lld_rc);
    return (BF_INTERNAL_ERROR);
  }

  for (j = 0; j < (int)num_pipes; j++) {
    rc = bf_tm_pipe_egress_limit_set(
        dev, j, BF_TM_TOF3_BUFFER_CELLS);  // set to max
    if (BF_SUCCESS != rc) {
      rc_cum |= rc;
      LOG_ERROR("%s:%d dev:%d, pipe:%d, rc=%d(%s)",
                __func__,
                __LINE__,
                dev,
                j,
                rc,
                bf_err_str(rc));
    }
    rc = bf_tm_pipe_egress_hysteresis_set(dev, j, 0);
    if (BF_SUCCESS != rc) {
      rc_cum |= rc;
      LOG_ERROR("%s:%d dev:%d, pipe:%d, rc=%d(%s)",
                __func__,
                __LINE__,
                dev,
                j,
                rc,
                bf_err_str(rc));
    }
    rc = bf_tm_sched_pkt_ifg_compensation_set(dev, j, 20);
    if (BF_SUCCESS != rc) {
      rc_cum |= rc;
      LOG_ERROR("%s:%d dev:%d, pipe:%d, rc=%d(%s)",
                __func__,
                __LINE__,
                dev,
                j,
                rc,
                bf_err_str(rc));
    }
    for (p = 0; p < (g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                     g_tm_ctx[dev]->tm_cfg.pg_per_pipe);
         p++) {
      lport = MAKE_DEV_PORT(j, p);
      lport = lld_sku_map_devport_from_device_to_user(dev, lport);
      rc = bf_tm_port_get_descriptor(dev, lport, &ports);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
        continue;
      }
      rc = bf_tm_port_set_qac_drop_limit(dev, ports, BF_TM_TOF3_BUFFER_CELLS);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
      }
      rc = bf_tm_port_set_qac_hyst(
          dev, ports, BF_TM_TOF3_QAC_DEFAULT_HYSTERESIS);
      if (BF_SUCCESS != rc) {
        rc_cum |= rc;
        LOG_ERROR("%s:%d dev:%d, dev_port:%d, rc=%d(%s)",
                  __func__,
                  __LINE__,
                  dev,
                  lport,
                  rc,
                  bf_err_str(rc));
      }
    }
    d_pipe = BF_TM_2DIE_D_PIPE(j, 0);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(j, 0);

    rc = bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_sch_top.sch[d_pipe].cfg_rate_clk),
        0xc4ea4a);
    if (BF_SUCCESS != rc) {
      rc_cum |= rc;
      LOG_ERROR("%s:%d dev:%d:%d, d_pipe:%d, rc=%d(%s)",
                __func__,
                __LINE__,
                dev,
                subdev_id,
                d_pipe,
                rc,
                bf_err_str(rc));
    }
  }
  if (BF_SUCCESS != rc_cum) {
    return (BF_INTERNAL_ERROR);
  }

  if (g_tm_ctx[dev]->subdev_count > 1) {
    // Init MGD Fifo Credits
    rc = bf_tm_tof3_init_mgd(dev);

    if (rc != BF_SUCCESS) {
      LOG_ERROR("TM: MGD Init FAILURE dev %d error 0x%x", dev, rc);
      return (rc);
    } else {
      LOG_TRACE("TM: MGD Init SUCCESS dev %d", dev);
    }
  }

  // Default setting for pool/s from egress TM prespective
  rc = bf_tm_tof3_set_default_for_eg_pool(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress pools");
    return (rc);
  }
  // Default setting for queue/s
  rc = bf_tm_tof3_set_default_for_q(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for queues");
    return (rc);
  }
  // deafult setting for port
  rc = bf_tm_tof3_set_default_for_port(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for ports");
    return (rc);
  }

  return (rc);
}

static int scan_caa_ready(bf_dev_id_t dev,
                          bf_subdev_id_t subdev_id,
                          uint32_t *valid_blocks) {
  uint32_t done, i;
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_0_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[0] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_1_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[1] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_2_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[2] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_3_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[3] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_4_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[4] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_caa_top.block_ready.block_ready_5_6),
      &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[5] & (0x1lu << i)) &&
        !getp_tof3_caa_block_ready_r_value(&done, i))
      return 0;
  }

  return 1;
}

static int scan_qlc_ready(bf_dev_id_t dev, bf_subdev_id_t subdev_id) {
  uint32_t done, i;
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_0_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof3_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_1_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof3_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }
  bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qlc_top.qlc_common.blk_rdy.blk_rdy_2_3),
      &done);
  for (i = 0; i < 32; i++) {
    if (!getp_tof3_qlm_blk_rdy_blk_rdy(&done, i)) return 0;
  }

  return 1;
}

static int scan_psc_ready(bf_dev_id_t dev,
                          bf_subdev_id_t subdev_id,
                          uint32_t *valid_blocks) {
  uint32_t done, i;
  bf_tm_subdev_read_register(dev,
                             subdev_id,
                             offsetof(tof3_reg,
                                      device_select.tm_top.tm_psc_top.psc_common
                                          .block_ready.block_ready_0_3),
                             &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[0] & (0x1lu << i)) &&
        !getp_tof3_psc_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(dev,
                             subdev_id,
                             offsetof(tof3_reg,
                                      device_select.tm_top.tm_psc_top.psc_common
                                          .block_ready.block_ready_1_3),
                             &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[1] & (0x1lu << i)) &&
        !getp_tof3_psc_block_ready_r_value(&done, i))
      return 0;
  }
  bf_tm_subdev_read_register(dev,
                             subdev_id,
                             offsetof(tof3_reg,
                                      device_select.tm_top.tm_psc_top.psc_common
                                          .block_ready.block_ready_2_3),
                             &done);
  for (i = 0; i < 32; i++) {
    if ((valid_blocks[2] & (0x1lu << i)) &&
        !getp_tof3_psc_block_ready_r_value(&done, i))
      return 0;
  }

  return 1;
}

static bf_status_t bf_tm_tof3_ddr_config_reset(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  // DDR Assert and Deassert
  setp_tof3_tm_fab_ctrl_r_init_all(&val, 0x1ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_fab_shim.tm_fab_ctrl),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to assert RDDR")

  // 1us sleep between write
  bf_sys_usleep(BF_TM_MICRO_SEC(1));

  setp_tof3_tm_fab_ctrl_r_init_all(&val, 0x0ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_fab_shim.tm_fab_ctrl),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to deassert RDDR")

  // Need to reset 23] : rDDR lane reset. for both dies.
  val = 0;
  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.soft_reset),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to read soft reset")

  val |= (0x1ul << 23);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.soft_reset),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to read soft reset")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_enable(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  setp_tof3_ddr_chipconfig_chipconfig_ddr_output_enable(&val, 0x1ul);
  setp_tof3_ddr_chipconfig_chipconfig_rddr_enable(&val, 0x1ul);

  if (subdev_id == 1) {
    // Only needed for the right chips for 2 or 4 subdev
    setp_tof3_ddr_chipconfig_chipconfig_southview(&val, 0x1ul);
  }

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr
                   .ddr_chipconfig),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR chip config register")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_reset(bf_dev_id_t dev,
                                             bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  setp_tof3_rddr_cmd_rddr_cmd_reset(&val, 0x1ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command register")

  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to read RDDR command register")

  val = 0;
  setp_tof3_rddr_cmd_rddr_cmd_reset(&val, 0x0ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command register")

  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to read RDDR command register")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_pll_cfg(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val, j, half;
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_A0;

  lld_err_t lld_rc = lld_sku_get_chip_part_revision_number(dev, &rev);
  if (lld_rc != LLD_OK) {
    LOG_ERROR("Failed to fetch SKU revision");
    return BF_INTERNAL_ERROR;
  }

  // Initialize Pll
  for (half = 0; half < 2; half++) {
    for (j = 0; j < 5; j++) {
      if (BF_SKU_CHIP_PART_REV_B0 == rev) {
        val = 0x1c9e;
        /*
          bit 0    : 0  -  Divide by 2
          bit 6-1  : 15 -  Feedback Clock Divider
          bit 10-7 : 9  -  Reference Clock Divider
          bit 11   : 1  -  PLL enable
          bit 12   : 1  -  Sets PLL clock output to 0.
        */
        rc = bf_tm_subdev_write_register(
            dev,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half]
                         .ddrpll_csr[j]
                         .rddr_pll_config),
            val);
        BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR PLL register")
      }

      val = 0xc9e;
      /*
        bit 0    : 0  -  Divide by 2
        bit 6-1  : 15 -  Feedback Clock Divider
        bit 10-7 : 9  -  Reference Clock Divider
        bit 11   : 1  -  PLL enable
        bit 12   : 0  -  Sets PLL clock output to 0.
      */
      rc = bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half]
                       .ddrpll_csr[j]
                       .rddr_pll_config),
          val);
      BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR PLL register")
    }
  }

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_lane_cfg(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0x27f00;
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_A0;

  lld_err_t lld_rc = lld_sku_get_chip_part_revision_number(dev, &rev);
  if (lld_rc != LLD_OK) {
    LOG_ERROR("Failed to fetch SKU revision");
    return BF_INTERNAL_ERROR;
  }

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr
                   .wr_address_mask),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDRD address mask")

  if (BF_SKU_CHIP_PART_REV_B0 == rev) {
#if !defined(DEVICE_IS_EMULATOR)
    val = 0x29ul;
    // setp_tof3_rddr_rx_config_rddr_rx_config, 10 for rxfifo_delay (10 << 2)
    // 1 for rxlane_polarity_enable (1 << 0)
#else
    val = 0x35ul;
    // setp_tof3_rddr_rx_config_rddr_rx_config, 13 for rxfifo_delay (13 << 2)
    // 1 for rxlane_polarity_enable (1 << 0)
#endif
  } else {
    val = 0xdul;
    // setp_tof3_rddr_rx_config_rddr_rx_config, 3 for rxfifo_delay (3 << 2)
    // 1 for rxlane_polarity_enable (1 << 0)
  }

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[0]
                   .rxlane[0]
                   .i_dft.rddr_rx_config),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR rx config")

  val = 0x4dul;
  // setp_tof3_rddr_tx_config_rddr_tx_lane_output_enable(1)
  // setp_tof3_rddr_tx_config_rddr_txfifo_delay(3)
  // setp_tof3_rddr_tx_config_rddr_txlane_polarity_enable(1)
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[0]
                   .txlane[0]
                   .csrs.rddr_tx_config),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR tx config")

  val = 0x0ul;
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr
                   .wr_address_mask),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR address mask")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_lane_reset(bf_dev_id_t dev,
                                                  bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  setp_tof3_rddr_cmd_rddr_cmd_reset(&val, 0x1ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  setp_tof3_rddr_cmd_rddr_cmd_reset(&val, 0x0ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_train(bf_dev_id_t dev,
                                             bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_A0;

  lld_err_t lld_rc = lld_sku_get_chip_part_revision_number(dev, &rev);
  if (lld_rc != LLD_OK) {
    LOG_ERROR("Failed to fetch SKU revision");
    return BF_INTERNAL_ERROR;
  }

  // Set the training
  setp_tof3_rddr_cmd_rddr_cmd_train(&val, 0x1ul);
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  rc = bf_tm_subdev_read_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      &val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to read RDDR command")

  if (rev == BF_SKU_CHIP_PART_REV_B0) {
    // Clear the training
    setp_tof3_rddr_cmd_rddr_cmd_train(&val, 0x0ul);
    rc = bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(
            tof3_reg,
            device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
        val);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

    rc = bf_tm_subdev_read_register(
        dev,
        subdev_id,
        offsetof(
            tof3_reg,
            device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
        &val);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")
  }

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_clear(bf_dev_id_t dev,
                                             bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr.rddr_cmd),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR command")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_rx_lane_enable(
    bf_dev_id_t dev, bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;

  val = 0x27f00;
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr
                   .wr_address_mask),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR address mask")

#if !defined(DEVICE_IS_EMULATOR)
  val = 0x8029;
  // setp_tof3_rddr_rx_config_rddr_rx_config, a for rxfifo_delay (10 << 2)
#else
  val = 0x8035;
  // setp_tof3_rddr_rx_config_rddr_rx_config, a for rxfifo_delay (13 << 2)
#endif

  // 1 for rxlane_polarity_enable (1 << 0)
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[0]
                   .rxlane[0]
                   .i_dft.rddr_rx_config),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write RDDR rx config")

  val = 0;
  rc = bf_tm_subdev_write_register(
      dev,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_ddr_top.tm_ddr.rddr.ddr_cmd_csr
                   .wr_address_mask),
      val);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to write address mask")

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_rddr_rxfsm_check(bf_dev_id_t dev,
                                                   bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t val = 0;
  uint32_t chk_cnt = 0;

  while (val != 0x7 && chk_cnt < 100) {
    // Lets verify if training is done.
    val = 0;
    rc = bf_tm_subdev_read_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[0]
                     .rxlane[0]
                     .i_dft.rddr_rxfsm_status),
        &val);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("RDDR Training Failed failed status = %#x Dev %d", rc, dev);
      return rc;
    }

    if (val != 0x7) {
      bf_sys_usleep(BF_TM_MICRO_SEC(100));
      chk_cnt++;
    } else {
      break;
    }
  }

  if (val != 0x7) {
    rc = BF_INTERNAL_ERROR;
    LOG_ERROR("RDDR Training Failed failed rddr_rxfsm_status val %d Dev %d",
              val,
              dev);
    return rc;
  } else {
    LOG_TRACE("RDDR Training SUCCESS Dev %d subdev %d", dev, subdev_id);
  }

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_init_seq_rev_A(bf_dev_id_t dev,
                                                 bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;

  // Config reset
  rc = bf_tm_tof3_ddr_config_reset(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR config")

  // Enabled RDDR
  rc = bf_tm_tof3_ddr_rddr_enable(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to enable RDDR")

  // Reset RDDR
  rc = bf_tm_tof3_ddr_rddr_reset(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to reser RDDR")

  // PLL config
  rc = bf_tm_tof3_ddr_rddr_pll_cfg(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDDR pll config")

  // Lane Config
  rc = bf_tm_tof3_ddr_rddr_lane_cfg(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDDR lane config")

  if (subdev_id == 1) {
    // We need to do the following steps in sequence between device 1 and dev 0
    // So we need to call Die 1 init first and then Die 0.
    rc = bf_tm_tof3_ddr_rddr_lane_reset(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR lane")

    // Need to train Die 0 before Die 1
    rc = bf_tm_tof3_ddr_rddr_lane_reset(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR lane")

    rc = bf_tm_tof3_ddr_rddr_train(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to train RDDR")

    rc = bf_tm_tof3_ddr_rddr_clear(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to clear RDDR")

    //  Now do Die 1 init also
    rc = bf_tm_tof3_ddr_rddr_clear(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to clear RDDR")

    if (g_tm_ctx[dev]->target == BF_TM_TARGET_ASIC) {
      // Die 0 rxfsm
      rc = bf_tm_tof3_ddr_rddr_rxfsm_check(dev, (subdev_id - 1));
      BF_TM_RETURN_ON_ERROR(rc, "Failed for RDDR rxfsm check")

      // Die 1 rxfsm
      rc = bf_tm_tof3_ddr_rddr_rxfsm_check(dev, subdev_id);
      BF_TM_RETURN_ON_ERROR(rc, "Failed for RDDR rxfsm check")
    }
  }

  return rc;
}

static bf_status_t bf_tm_tof3_ddr_init_seq_rev_B(bf_dev_id_t dev,
                                                 bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;

  rc = bf_tm_tof3_ddr_config_reset(dev, subdev_id);
  BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR config")

  if (subdev_id == 1) {
    // RDDR enable on Die 0 and Die 1
    rc = bf_tm_tof3_ddr_rddr_enable(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to enable RDDR")

    rc = bf_tm_tof3_ddr_rddr_enable(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to enable RDDR")

    // PLL config on Die 0 and Die 1
    rc = bf_tm_tof3_ddr_rddr_pll_cfg(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDDR pll config")

    rc = bf_tm_tof3_ddr_rddr_pll_cfg(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDDR pll config")

    // Reset Die 0
    rc = bf_tm_tof3_ddr_rddr_reset(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR")

    // Lane config on Die 0 and Die 1
    rc = bf_tm_tof3_ddr_rddr_lane_cfg(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDD lane config")

    rc = bf_tm_tof3_ddr_rddr_lane_cfg(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to set RDD lane config")

    // Reset Die 0
    rc = bf_tm_tof3_ddr_rddr_reset(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to reset RDDR")

    // Train Die 0
    rc = bf_tm_tof3_ddr_rddr_train(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to train RDDR")

    if (g_tm_ctx[dev]->target == BF_TM_TARGET_ASIC) {
      // Die 0 rxfsm
      rc = bf_tm_tof3_ddr_rddr_rxfsm_check(dev, (subdev_id - 1));
      BF_TM_RETURN_ON_ERROR(rc, "Failed for RDDR rxfsm check")

      // Die 1 rxfsm
      rc = bf_tm_tof3_ddr_rddr_rxfsm_check(dev, subdev_id);
      BF_TM_RETURN_ON_ERROR(rc, "Failed for RDDR rxfsm check")
    }

    // Lane enable on Die0 and Die1
    rc = bf_tm_tof3_ddr_rddr_rx_lane_enable(dev, (subdev_id - 1));
    BF_TM_RETURN_ON_ERROR(rc, "Failed to enable RDDR rx lane")

    rc = bf_tm_tof3_ddr_rddr_rx_lane_enable(dev, subdev_id);
    BF_TM_RETURN_ON_ERROR(rc, "Failed to enable RDDR rx lane")
  }

  return rc;
}

bf_status_t bf_tm_tof3_ddr_init_seq(bf_dev_id_t dev, bf_subdev_id_t subdev_id) {
  bf_status_t rc = BF_SUCCESS;
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_A0;
  if (TM_IS_TARGET_ASIC(dev)) {
#if !defined(DEVICE_IS_EMULATOR)  // Emulator
    lld_err_t lld_rc = lld_sku_get_chip_part_revision_number(dev, &rev);
    if (lld_rc != LLD_OK) {
      LOG_ERROR("Failed to fetch SKU revision");
      return BF_INTERNAL_ERROR;
    }

    if (rev == BF_SKU_CHIP_PART_REV_A0) {
      // Do not do DDR init for TF3 A0 chip version.
      return BF_SUCCESS;
    }
#else
// Use  -DEXTRA_CPPFLAGS="-DDEVICE_IS_EMULATOR=on -DEMU_TF3_REV_B=on" for REV B
// compilation
#ifdef EMU_TF3_REV_B
    rev = BF_SKU_CHIP_PART_REV_B0;
#endif
#endif
  }

  if ((BF_SKU_CHIP_PART_REV_A0 == rev) || (BF_SKU_CHIP_PART_REV_A1 == rev)) {
    rc = bf_tm_tof3_ddr_init_seq_rev_A(dev, subdev_id);
  } else if (BF_SKU_CHIP_PART_REV_B0 == rev) {
    rc = bf_tm_tof3_ddr_init_seq_rev_B(dev, subdev_id);
  } else {
    LOG_ERROR("DDR init is not supported for this chip");
    rc = BF_NOT_SUPPORTED;
  }

  return rc;
}

static bf_status_t bf_tm_tof3_init_seq(bf_dev_id_t dev) {
  uint32_t d_inst;
  bf_status_t rc = BF_SUCCESS;

  for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
    uint32_t val, i, j;
    uint32_t done;

    uint32_t caa_valid_blocks[6] = {0};
    uint32_t psc_valid_blocks[3] = {0};

    // Get CAA Valid Blocks
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_0_6),
        &caa_valid_blocks[0]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_1_6),
        &caa_valid_blocks[1]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_2_6),
        &caa_valid_blocks[2]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_3_6),
        &caa_valid_blocks[3]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_4_6),
        &caa_valid_blocks[4]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_valid.block_valid_5_6),
        &caa_valid_blocks[5]);

    // Get PSC Valid Blocks
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_valid
                     .block_valid_0_3),
        &psc_valid_blocks[0]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_valid
                     .block_valid_1_3),
        &psc_valid_blocks[1]);
    bf_tm_subdev_read_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_valid
                     .block_valid_2_3),
        &psc_valid_blocks[2]);

    // Take caa block out of reset
    uint32_t reg[6] = {0};
    for (i = 0; i < 6; i++) {
      for (j = 0; j < 32; j++) {
        if (caa_valid_blocks[i] & (0x1lu << j)) {
          setp_tof3_caa_block_reset_r_value(reg + i, j, 0);
        }
      }
    }
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_0_6),
        reg[0]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_1_6),
        reg[1]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_2_6),
        reg[2]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_3_6),
        reg[3]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_4_6),
        reg[4]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_reset.block_reset_5_6),
        reg[5]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg, device_select.tm_top.tm_caa_top.ctrl),
        0x1 | (15 << 8));

    // Take qlm block out of reset
    TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

    for (i = 0; i < 3; i++) {
      for (j = 0; j < 32; j++) {
        setp_tof3_qlm_blk_reset_blk_reset(reg + i, j, 0);
      }
    }
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc_common.qlm_blk_reset
                     .qlm_blk_reset_0_3),
        reg[0]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc_common.qlm_blk_reset
                     .qlm_blk_reset_1_3),
        reg[1]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc_common.qlm_blk_reset
                     .qlm_blk_reset_2_3),
        reg[2]);

    // Take psc block out of reset
    TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

    for (i = 0; i < 3; i++) {
      for (j = 0; j < 32; j++) {
        if (psc_valid_blocks[i] & (0x1lu << j)) {
          setp_tof3_psc_block_reset_r_value(reg + i, j, 0);
        }
      }
    }
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_reset
                     .block_reset_0_3),
        reg[0]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_reset
                     .block_reset_1_3),
        reg[1]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_reset
                     .block_reset_2_3),
        reg[2]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.ctrl),
        0x1 | (15 << 8));

    // TM driver is communicating with real ASIC/Emulator/RTL-SIM
    // Hence its possible to read back chip progress.

    // check caa-ready-before  enabling caa
    done = 0;
    while (!done) {
      done = scan_caa_ready(dev, (bf_subdev_id_t)d_inst, caa_valid_blocks);
      LOG_TRACE("CAA block Ready Status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }
    LOG_TRACE("CAA block Ready");

    // check qlc-ready (qlc_top.qlc_common.blk_rdy)
    done = 0;
    while (!done) {
      done = scan_qlc_ready(dev, (bf_subdev_id_t)d_inst);
      LOG_TRACE("QLC block[0] Ready Status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }

    // check psc-ready-before  enabling psc
    done = 0;
    while (!done) {
      done = scan_psc_ready(dev, (bf_subdev_id_t)d_inst, psc_valid_blocks);
      LOG_TRACE("PSC block Ready Status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }
    LOG_TRACE("PSC block Ready");
    // Enable psc block
    TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

    for (i = 0; i < 3; i++) {
      for (j = 0; j < 32; j++) {
        if (psc_valid_blocks[i] & (0x1lu << j)) {
          setp_tof3_psc_block_enable_r_value(reg + i, j, 1);
        }
      }
    }
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_enable
                     .block_enable_0_3),
        reg[0]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_enable
                     .block_enable_1_3),
        reg[1]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.block_enable
                     .block_enable_2_3),
        reg[2]);

    // Enable caa block
    TRAFFIC_MGR_MEMSET(reg, 0, sizeof(uint32_t) * 6);

    for (i = 0; i < 6; i++) {
      for (j = 0; j < 32; j++) {
        if (caa_valid_blocks[i] & (0x1lu << j)) {
          setp_tof3_caa_block_enable_r_value(reg + i, j, 1);
        }
      }
    }
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_0_6),
        reg[0]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_1_6),
        reg[1]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_2_6),
        reg[2]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_3_6),
        reg[3]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_4_6),
        reg[4]);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.block_enable.block_enable_5_6),
        reg[5]);

    uint64_t enbl_val = 0;
    if (g_tm_ctx[dev]->subdev_count > 1) {
      enbl_val = 0x0f0f0f0f0f0f0f0f;
    } else {
      enbl_val = 0x0f0f0f0f;
    }

    for (i = 0; i < 64; i++) {
      bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg, device_select.tm_top.tm_caa_top.epipe[i].enable),
          ((enbl_val >> i) & 0x1ul));
    }

    ////////////   Trigger mem inits of WAC, QAC, SCH, PRC/PRM /////////////
    // Trigger WAC mem init
    val = 0xf0;
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_glb_config),
        val);
    // Trigger QAC mem init
    val = 0;
    setp_tof3_qac_mem_init_en_enable(&val, 0xff);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_mem_init_en),
        val);
    // Trigger SCH mem init
    val = 0;
    for (i = 0; i < 4; i++) {
      bf_tm_subdev_read_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[i].ctrl),
          &val);
      setp_tof3_sch_ctrl_r_hw_init_enb(&val, 1);
      bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[i].ctrl),
          val);
    }

    // Trigger PRC/PRM mem init
    val = 0;
    setp_tof3_prc_control_map_init(&val, 1);
    bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.control),
        val);

    // TM driver is communicating with real ASIC/Emulator/RTL-SIM
    // Hence its possible to read back chip progress.
    ///////////////   Check mem init Done for WAC, QAC, SCH, PRC //////////////
    done = 0;
    while (!done) {
      bf_tm_subdev_read_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_wac_top.wac_common.wac_common
                       .wac_mem_init_done),
          &done);
      LOG_TRACE("Wac Mem init Done Status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }
    LOG_TRACE("Wac Mem init Complete Dev %d subdev %d", dev, d_inst);
    done = 0;
    while (!done) {
      bf_tm_subdev_read_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_mem_init_done),
          &done);
      LOG_TRACE("Qac Mem init Done Status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }
    LOG_TRACE("Qac Mem init Complete Dev %d subdev %d", dev, d_inst);
    // TODO : Check if we even need to differentiate between the SCH
    for (i = 0; i < 4; i++) {
      done = 0;
      while ((done & 0x7ff) != 0x7ff) {
        bf_tm_subdev_read_register(
            dev,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[i].ready),
            &done);
        LOG_TRACE("SCH A PIPE %d Mem init Done Status = %#x", i, done);
        if ((done & 0x7ff) != 0x7ff) bf_sys_usleep(100);
      }
    }
    LOG_TRACE("SCH Mem init Complete Dev %d subdev %d", dev, d_inst);
    done = 0;
    while (!done) {
      bf_tm_subdev_read_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.status),
          &done);
      done = getp_tof3_prc_status_init_done(&done);
      LOG_TRACE("PRC Map Mem init done status = %#x", done);
      if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
    }
    LOG_TRACE("PRC Map Mem init Complete Dev %d subdev %d", dev, d_inst);

    // Trigger QOC mem init

    for (uint32_t pipe = 0; pipe < BF_TM_TOF3_IG_PIPES; pipe++) {
      val = 0;
      setp_tof3_qoc_init_ctrl_r_init_q_cfg(&val, 1);
      bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[pipe].init_ctrl),
          val);

      done = 0;
      while (!done) {
        bf_tm_subdev_read_register(
            dev,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qoc_top.qoc[pipe].init_stat),
            &done);
        done = getp_tof3_qoc_init_status_r_cfg_queue_done(&done);
        LOG_TRACE("QOC Queue Configuration array reset status = %#x", done);
        if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
      }

      val = 0;
      setp_tof3_qoc_init_ctrl_r_init_q_ctr(&val, 1);
      bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[pipe].init_ctrl),
          val);

      done = 0;
      while (!done) {
        bf_tm_subdev_read_register(
            dev,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qoc_top.qoc[pipe].init_stat),
            &done);
        done = getp_tof3_qoc_init_status_r_ctr_rst_done(&done);
        LOG_TRACE("QOC Queue Counter arrays reset status = %#x", done);
        if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
      }

      val = 0;
      setp_tof3_qoc_init_ctrl_r_init_tsmem(&val, 1);
      bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[pipe].init_ctrl),
          val);

      done = 0;
      while (!done) {
        bf_tm_subdev_read_register(
            dev,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qoc_top.qoc[pipe].init_stat),
            &done);
        done = getp_tof3_qoc_init_status_r_tsmem_rst_done(&done);
        LOG_TRACE("QOC Timestamp arrays reset status = %#x", done);
        if (!done) bf_sys_usleep(BF_TM_MICRO_SEC(100));
      }
    }

    LOG_TRACE("QOC init Complete Dev %d subdev %d", dev, d_inst);

    // Trigger sMGC mem init
    uint32_t tm_clk_period = 7692;
    uint32_t clk_pps_period = 7936;

    for (uint32_t pipe = 0; pipe < BF_TM_TOF3_IG_PIPES; pipe++) {
      val = 0;
      uint32_t val1 = 0;
      uint32_t pps_token_bkt_cfg_decr = 0x3E8;  // 1000
      uint32_t pps_token_bkt_cfg_accum = (uint32_t)(
          ((double)(pps_token_bkt_cfg_decr * tm_clk_period) / clk_pps_period));
      uint32_t pps_token_bkt_cfg_bkt_size = 0xFA0;  // 4000
      bool pps_token_bkt_cfg_bkt_size_hi =
          !((uint64_t)pps_token_bkt_cfg_bkt_size) << 20 & (0x1ull << 32);

      setp_tof3_mgc_pps_rate_cfg_r_pps_token_bucket_cfg_0_2_accum(
          &val, pps_token_bkt_cfg_accum);
      setp_tof3_mgc_pps_rate_cfg_r_pps_token_bucket_cfg_0_2_decr(
          &val, pps_token_bkt_cfg_decr);
      setp_tof3_mgc_pps_rate_cfg_r_pps_token_bucket_cfg_0_2_bkt_size_11_0(
          &val, pps_token_bkt_cfg_bkt_size);

      setp_tof3_mgc_pps_rate_cfg_r_pps_token_bucket_cfg_1_2_bkt_size_12_12(
          &val1, (uint32_t)pps_token_bkt_cfg_bkt_size_hi);

      rc |= bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[pipe]
                       .pps_token_bucket_cfg.pps_token_bucket_cfg_0_2),
          val);

      rc |= bf_tm_subdev_write_register(
          dev,
          (bf_subdev_id_t)d_inst,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[pipe]
                       .pps_token_bucket_cfg.pps_token_bucket_cfg_1_2),
          val1);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("sMGC pps_token_bucket_cfg init failed status = %#x", rc);
      }
    }
    LOG_TRACE("sMGC init Complete Dev %d subdev %d", dev, d_inst);

    if (g_tm_ctx[dev]->subdev_count > 1) {
      rc |= bf_tm_tof3_ddr_init_seq(dev, (bf_subdev_id_t)d_inst);

      LOG_TRACE("DDR init Complete Dev %d subdev %d", dev, d_inst);
    }
  }
  return (rc);
}

bf_status_t bf_tm_tof3_start_init_seq_during_fast_recfg(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  // Init seq to be applied before configuring TM
  // Since init sequence is interactive, (deassert block reset, check for
  // init done etc), read/writes to TM will be via normal pcie read/writes
  // no DMA at this point.
  g_tm_ctx[dev]->batch_mode = false;
  g_tm_ctx[dev]->fast_reconfig_init_seq = true;
  rc = bf_tm_tof3_init_seq(dev);
  g_tm_ctx[dev]->batch_mode = true;
  g_tm_ctx[dev]->fast_reconfig_init_seq = false;

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}

static bf_status_t bf_tm_tof3_init_default_credits(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_subdev_id_t subdev_id;
  bf_dev_pipe_t d_pipe;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
    // Setup the timestamp offset for both dies if applicable
    rc |= bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.ts_offset),
        31);
  }
  // Set mem_init_en bit to 1 in CLC pipe_ctrl register for all pipes
  uint32_t num_pipes, pipe;
  uint32_t val = 0;
  uint32_t phy_pipe;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, pipe, &phy_pipe) != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. device = %d "
          "logical "
          "pipe = %d",
          dev,
          pipe);
      /* continue for other pipes */
      continue;
    }

    d_pipe = BF_TM_2DIE_D_PIPE(phy_pipe, 0);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(phy_pipe, 0);

    val = 0;
    setp_tof3_qclc_pipe_ctrl_mem_init_en(&val, 0x1ul);
    setp_tof3_qclc_pipe_ctrl_double_ph_400g(&val, 0x1ul);

    rc |= bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc[d_pipe].pipe_ctrl),
        val);

    val = 0;
    setp_tof3_sch_pex_credit_ctrl_r_queue_flush_credit_limit(&val, 18);

    rc |= bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_sch_top.sch[d_pipe].pex_credit_ctrl),
        val);

    val = 0;

    if (g_tm_ctx[dev]->subdev_count > 1) {
      setp_tof3_sch_pex_credit_ctrl_2r_pex_400G_credit(&val, 111);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_200G_credit(&val, 56);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_100G_credit(&val, 28);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_50G_credit(&val, 14);
    } else {
      setp_tof3_sch_pex_credit_ctrl_2r_pex_400G_credit(&val, 71);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_200G_credit(&val, 36);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_100G_credit(&val, 18);
      setp_tof3_sch_pex_credit_ctrl_2r_pex_50G_credit(&val, 9);
    }

    rc |= bf_tm_subdev_write_register(
        dev,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_sch_top.sch[d_pipe].pex_credit_ctrl_2),
        val);

    for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(phy_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(phy_pipe, d_inst);

      val = 0;
      setp_tof3_qlc_dis_cred_qlc_dis_cred(&val, 110);

      rc |= bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qlc_top.qlc[d_pipe].dis_cred),
          val);

      val = 0;
      if (d_inst == 0) {
        setp_tof3_pex_mgc_credit_cfg_r_cred_50G(&val, 4);
        setp_tof3_pex_mgc_credit_cfg_r_cred_100G(&val, 8);
        setp_tof3_pex_mgc_credit_cfg_r_cred_200G(&val, 16);
        setp_tof3_pex_mgc_credit_cfg_r_cred_400G(&val, 32);
      } else {
        setp_tof3_pex_mgc_credit_cfg_r_cred_50G(&val, 8);
        setp_tof3_pex_mgc_credit_cfg_r_cred_100G(&val, 16);
        setp_tof3_pex_mgc_credit_cfg_r_cred_200G(&val, 32);
        setp_tof3_pex_mgc_credit_cfg_r_cred_400G(&val, 64);
      }

      rc |= bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(
              tof3_reg,
              device_select.tm_top.tm_pex_top.pex[d_pipe].pex_mgc_credit_cfg),
          val);

      uint32_t *qac_pipe_cfg =
          &((g_tm_ctx[dev]->pipes + pipe)->qac_pipe_config);

      if (g_tm_ctx[dev]->subdev_count > 1) {
        setp_tof3_qac_pipe_config_ignore_uc_drop_st(qac_pipe_cfg, 1);
      } else {
        setp_tof3_qac_pipe_config_ignore_uc_drop_st(qac_pipe_cfg, 0);
      }
      rc |= bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.pipe_config),
          *qac_pipe_cfg);
    }
  }
  // Initialize DDR shim output
  val = 0;
  setp_tof3_tm_fab_ctrl_r_enable_shim_output(&val, 0x1ul);
  setp_tof3_tm_fab_ctrl_r_disable_feedthrough_pol(&val, 0x1ul);

  for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_fab_shim.tm_fab_ctrl),
        val);
  }
  bf_tm_flush_wlist(dev);  // Push any buffered writes to HW

  return rc;
}

bf_status_t bf_tm_tof3_set_default(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_tof3_log_defaults();

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
    rc = bf_tm_tof3_init_seq(dev);
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
      bf_tm_tof3_power_on_reset_per_pipe_tbls(dev, i);
    }
    bf_tm_tof3_power_on_reset_non_pipe_tbls(dev);
    bf_tm_flush_wlist(dev);  // Push any buffered writes to HW
    sleep(2);  // <---- This sleep is needed for DMA buffers to sink into model.
#endif
  }
  g_tm_ctx[dev]->batch_mode = true;

  // Ingress
  rc = bf_tm_tof3_set_ingress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Ingress TM");
    goto cleanup;
  }
  // Egress
  rc = bf_tm_tof3_set_egress_tm_default(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default values for Egress TM");
    goto cleanup;
  }
  rc = bf_tm_tof3_init_default_credits(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to set default credits TM");
    goto cleanup;
  }

  uint32_t val = 0;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
    // For Performance these CAA values are needed.
    val = 0;
    setp_tof3_caa_fa_full_threshold_r_value(&val, 0x3f0);
    rc = bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg, device_select.tm_top.tm_caa_top.full_threshold),
        val);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: Unable to set full_threshold dev %d subdev %d Error 0x%0x TM",
          dev,
          d_inst,
          rc);
      goto cleanup;
    }

    val = 0;
    setp_tof3_caa_fa_hyst_threshold_r_value(&val, 0x3e8);
    rc = bf_tm_subdev_write_register(
        dev,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg, device_select.tm_top.tm_caa_top.hyst_threshold),
        val);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: Unable to set hyst_threshold dev %d subdev %d Error 0x%0x TM",
          dev,
          d_inst,
          rc);
      goto cleanup;
    }
  }

cleanup:
  g_tm_ctx[dev]->internal_call = false;
  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);

  return (rc);
}
