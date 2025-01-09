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
#include <math.h>
#include <inttypes.h>

#include "tm_tofino_hw_intf.h"
#include "traffic_mgr/init/tm_tofino.h"

#include <tofino_regs/tofino.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_reg_if.h>

#include "traffic_mgr/init/tm_tofino_default.h"
#include <traffic_mgr/traffic_mgr_counters.h>

/* HW read/write functions used by accessor functions in
 * tm_[ig_ppg.c/ig_pool.c/...
 */

/////////////////PPG////////////////////////////

#define BF_TM_PPG_FOLD_THRES_LIMIT(limit, baf) ((limit << 4) | baf)
#define BF_TM_PPG_THRES_LIMIT(val) (val >> 4)
#define BF_TM_PPG_BAF(val) (val & 0xf)

bf_tm_status_t bf_tm_tofino_ppg_clear_wac_drop_state(bf_dev_id_t devid,
                                                     bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_3_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_2_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_1_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_0_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_3_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_2_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_1_4),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_0_4),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_wac_drop_state(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_port_drop_state.wac_port_drop_state_2_3),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_port_drop_state.wac_port_drop_state_1_3),
      0);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_port_drop_state.wac_port_drop_state_0_3),
      0);

  return rc;
}

static bf_tm_status_t bf_tm_tofino_clear_wac_drop_state(bf_dev_id_t devid,
                                                        uint8_t p_pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_port_t p;
  p.p_pipe = p_pipe;
  rc = bf_tm_tofino_port_clear_wac_drop_state(devid, &p);

  bf_tm_ppg_t ppg;
  ppg.p_pipe = p_pipe;
  rc = bf_tm_tofino_ppg_clear_wac_drop_state(devid, &ppg);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_wac_drop_state(bf_dev_id_t devid,
                                                    bf_tm_port_t *p,
                                                    bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  if (p->port < (uint8_t)32) {
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.wac_port_drop_state.wac_port_drop_state_0_3),
        &val);
  } else if ((p->port >= 32) && (p->port < 64)) {
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.wac_port_drop_state.wac_port_drop_state_1_3),
        &val);
  } else if ((p->port >= 64) && (p->port < 72)) {
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.wac_port_drop_state.wac_port_drop_state_2_3),
        &val);
  }
  *state = (val >> (p->port % 32)) & 0x1;
  return rc;
}

bf_tm_status_t bf_tm_tofino_ppg_set_min_limit(bf_dev_id_t devid,
                                              const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  val = ppg->thresholds.min_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_min_lmt.min_lmt[ppg->ppg]),
      val);
  if (val) {
    /* If ppg limit was previously set to zero, it is required to
     * clear ppg-drop-state after setting drop-limit; otherwise  WAC will
     * continue to drop traffic.
     */
    rc |= bf_tm_tofino_clear_wac_drop_state(devid, ppg->p_pipe);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_min_limit(bf_dev_id_t devid,
                                              bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_min_lmt.min_lmt[ppg->ppg]),
      &val);
  ppg->thresholds.min_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_skid_limit(bf_dev_id_t devid,
                                               const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  val = ppg->thresholds.skid_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_hdr_lmt.hdr_lmt[ppg->ppg]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_skid_limit(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  uint32_t val;
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_hdr_lmt.hdr_lmt[ppg->ppg]),
      &val);
  ppg->thresholds.skid_limit = val;
  return (rc);
}

// Tofino implements an array of 32 entries to specify hysteresis values.
// Instead of programming actual hysteresis value in tables, index of the
// 32 entry table is used. This index is refered as offset_idx in register
// spec. WAC offset proile table is used for HYST on PPG and Port.
// bf_tm_tofino_restore_wac_offset_profile() can be used to restore
// the following 2 SW tables/variables for hitless HA case.

// TODO: At some point all the offset_profiles will be occupied with some
// values which actually might be not used by any Port, PPG.
// Some reference counting and housekeeping is needed to reuse
// this limited resource.

static uint8_t wac_offset_profile_cnt[BF_TM_NUM_ASIC][BF_TM_TOFINO_MAU_PIPES] =
    {{0}};
static uint32_t wac_offset_profile_tbl[BF_TM_NUM_ASIC][BF_TM_TOFINO_MAU_PIPES]
                                      [BF_TM_TOFINO_HYSTERESIS_PROFILES] = {
                                          {{0}}};

bf_status_t bf_tm_tofino_restore_wac_offset_profile(bf_dev_id_t dev) {
  uint32_t num_p_pipes = 0;
  uint32_t val;

  g_tm_ctx[dev]->read_por_wac_profile_table = false;

  // Clear all the array, even if some pipes ara not used on the device.
  for (int p = 0; p < BF_TM_TOFINO_MAU_PIPES; p++) {
    wac_offset_profile_cnt[dev][p] = 0;
  }

  lld_err_t lld_err = lld_sku_get_num_active_pipes(dev, &num_p_pipes);
  if ((LLD_OK != lld_err) || (num_p_pipes > BF_TM_TOFINO_MAU_PIPES)) {
    LOG_ERROR(
        "Restore WAC hyst. on dev %d can't get number of physical pipes, "
        "lld_rc=%d",
        dev,
        lld_err);
    return (BF_INTERNAL_ERROR);
  }

  for (bf_dev_pipe_t l_pipe = 0; l_pipe < num_p_pipes; l_pipe++) {
    bool got_default_value = false;
    bool got_reset_value = false;
    bf_dev_pipe_t p_pipe = 0;

    lld_err = lld_sku_map_pipe_id_to_phy_pipe_id(dev, l_pipe, &p_pipe);
    if (LLD_OK != lld_err) {
      LOG_ERROR(
          "Restore WAC hyst. on dev %d l_pipe %d can't map to physical pipe, "
          "lld_rc=%d",
          dev,
          l_pipe,
          lld_err);
      return (BF_INTERNAL_ERROR);
    }

    for (int i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
      (void)bf_tm_read_register(
          dev,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[p_pipe]
                       .wac_reg.offset_profile[i]),
          &val);
      LOG_TRACE("Restore dev %d l_pipe %d p_pipe %d WAC hyst[%d]=0x%x",
                dev,
                l_pipe,
                p_pipe,
                i,
                val);
      // Ignore SW model garbage
      val = (TM_IS_TARGET_ASIC(dev)) ? val : BF_TM_TOFINO_WAC_RESET_HYSTERESIS;
      wac_offset_profile_tbl[dev][p_pipe][i] = val;
      // Treat all values less than POR as empty items except the first one.
      // It might be at any index in the array.
      // Non-POR HW reset values are counted as empty items and will be reused
      // except the first one greate than POR.
      if (got_default_value && val <= BF_TM_TOFINO_WAC_POR_HYSTERESIS) {
        continue;
      }
      if (got_reset_value && val == BF_TM_TOFINO_WAC_RESET_HYSTERESIS) {
        continue;
      }
      wac_offset_profile_cnt[dev][p_pipe]++;
      if (val <= BF_TM_TOFINO_WAC_POR_HYSTERESIS) {
        got_default_value = true;
      }
      if (val == BF_TM_TOFINO_WAC_RESET_HYSTERESIS) {
        got_reset_value = true;
      }
    }
    LOG_TRACE("dev %d l_pipe %d p_pipe %d WAC %d hyst items",
              dev,
              l_pipe,
              p_pipe,
              wac_offset_profile_cnt[dev][p_pipe]);
  }

  g_tm_ctx[dev]->read_por_wac_profile_table = true;
  return (BF_SUCCESS);
}

static uint8_t bf_tm_tofino_get_wac_offset_profile_index(bf_dev_id_t dev,
                                                         bf_tm_thres_t cells_u,
                                                         uint8_t p_pipe) {
  bf_status_t rc = BF_SUCCESS;
  int i;

  // search from the start till end to check if cell_limit is already
  // in profile table.
  if (!g_tm_ctx[dev]->read_por_wac_profile_table) {
#ifndef BF_TM_HITLESS_HA_TESTING_WITH_MODEL
    rc = bf_tm_tofino_restore_wac_offset_profile(dev);
    if (BF_SUCCESS != rc) {
      return (BF_TM_TOFINO_HYSTERESIS_PROFILES);
    }
#else
    LOG_TRACE("clear all WAC hyst profiles dev %d", dev);
    for (int p = 0; p < BF_TM_TOFINO_MAU_PIPES; p++) {
      wac_offset_profile_cnt[dev][p] = 0;
      for (i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
        wac_offset_profile_tbl[dev][p][i] = BF_TM_TOFINO_WAC_RESET_HYSTERESIS;
      }
    }
    g_tm_ctx[dev]->read_por_wac_profile_table = true;
#endif
  }

  /* For the hysteresis values, check if it is configured already */
  for (i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
    if (wac_offset_profile_tbl[dev][p_pipe][i] == cells_u) {
      return (i);
    }
  }
  return (BF_TM_TOFINO_HYSTERESIS_PROFILES);
}

static bf_status_t bf_tm_tofino_check_wac_offset_profile(bf_dev_id_t dev,
                                                         uint8_t p_pipe,
                                                         bf_tm_thres_t cells_u,
                                                         uint8_t idx_expected) {
  int idx = bf_tm_tofino_get_wac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    // Check the expected index as well
    if ((BF_TM_TOFINO_HYSTERESIS_PROFILES != idx_expected) &&
        (idx != idx_expected)) {
      LOG_ERROR("dev %d pipe %d WAC hyst[%d] is %d instead of hyst[%d]",
                dev,
                p_pipe,
                idx,
                wac_offset_profile_tbl[dev][p_pipe][idx],
                idx_expected);
      return (BF_ALREADY_EXISTS);
    }
    return (BF_SUCCESS);
  }
  LOG_ERROR("dev %d pipe %d WAC hyst[%d]=%d is not registered",
            dev,
            p_pipe,
            idx,
            cells_u);
  return (BF_TABLE_NOT_FOUND);
}

static bf_status_t bf_tm_tofino_populate_wac_offset_profile(bf_dev_id_t dev,
                                                            uint8_t p_pipe,
                                                            bf_tm_thres_t cells,
                                                            uint8_t *index) {
  int idx;
  bf_status_t rc = BF_SUCCESS;
  uint32_t cells_u = TM_CELLS_TO_8CELL_UNITS(cells);

  // For dynamic sharing to work, hysteresis shouldn't be
  // 0 - the reason why POR value is 4 in 8 cells unit i.e 32.
  // Add default item if it is not yet in the table.
  if (cells_u < BF_TM_TOFINO_WAC_POR_HYSTERESIS) {
    cells_u = BF_TM_TOFINO_WAC_POR_HYSTERESIS;
  }

  idx = bf_tm_tofino_get_wac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    // Already exists...
    *index = idx;
    return (rc);
  }

  if (wac_offset_profile_cnt[dev][p_pipe] >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    // Find limit that is closest to the current limit
    // One possible solution is set change hysteresis
    // to another hysteresis value that is closest to previously
    // configured hysteresis.. Doing this would lead to deviating
    // from user expected behaviour...
    // Lets return error (like no system resource)
    LOG_ERROR("No WAC hysteresis profiles left on dev %d pipe %d for %d cells",
              dev,
              p_pipe,
              cells);
    rc = BF_NO_SYS_RESOURCES;
  } else {
    idx = 0;
    if (wac_offset_profile_cnt[dev][p_pipe]) {
      bool got_default_value = false;
      bool got_reset_value = false;
      // Find first empty item
      for (idx = 0; idx < BF_TM_TOFINO_HYSTERESIS_PROFILES; idx++) {
        if (wac_offset_profile_tbl[dev][p_pipe][idx] <=
            BF_TM_TOFINO_WAC_POR_HYSTERESIS) {
          if (got_default_value) {
            // no default values yet in registers or an empty one
            LOG_DBG("Reuse WAC hyst[%d]=%d of %d at dev %d pipe %d",
                    idx,
                    wac_offset_profile_tbl[dev][p_pipe][idx],
                    wac_offset_profile_cnt[dev][p_pipe],
                    dev,
                    p_pipe);
            break;
          }
          LOG_DBG("Keep WAC default hyst[%d]=%d of %d at dev %d pipe %d",
                  idx,
                  wac_offset_profile_tbl[dev][p_pipe][idx],
                  wac_offset_profile_cnt[dev][p_pipe],
                  dev,
                  p_pipe);
          got_default_value = true;  // Keep the first one.
        } else if (wac_offset_profile_tbl[dev][p_pipe][idx] ==
                   BF_TM_TOFINO_WAC_RESET_HYSTERESIS) {
          // When HW reset value is greater than POR
          if (got_reset_value) {
            // no default values yet in registers or an empty one
            LOG_DBG("Reuse WAC hyst[%d]=%d of %d at dev %d pipe %d",
                    idx,
                    wac_offset_profile_tbl[dev][p_pipe][idx],
                    wac_offset_profile_cnt[dev][p_pipe],
                    dev,
                    p_pipe);
            break;
          }
          LOG_DBG("Keep WAC reset hyst[%d]=%d of %d at dev %d pipe %d",
                  idx,
                  wac_offset_profile_tbl[dev][p_pipe][idx],
                  wac_offset_profile_cnt[dev][p_pipe],
                  dev,
                  p_pipe);
          got_reset_value = true;  // Keep the first one.
        }
      }
      if (idx >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
        LOG_ERROR("Inconsistent WAC hysteresis profiles on dev %d pipe %d",
                  dev,
                  p_pipe);
        return (BF_NO_SYS_RESOURCES);
      }
    }
    LOG_TRACE("Set WAC hyst[%d]=%d of %d at dev %d pipe %d",
              idx,
              cells_u,
              wac_offset_profile_cnt[dev][p_pipe],
              dev,
              p_pipe);
    wac_offset_profile_tbl[dev][p_pipe][idx] = cells_u;
    *index = idx;
    // Write to HW
    (void)bf_tm_write_register(
        dev,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p_pipe]
                     .wac_reg.offset_profile[*index]),
        cells_u);
    wac_offset_profile_cnt[dev][p_pipe]++;
  }
  return (rc);
}

inline static bf_status_t bf_tm_prep_ppg_shr_limit_reg_val(
    bf_dev_id_t dev, uint32_t *val, const bf_tm_ppg_t *ppg) {
  uint8_t hyst_index = 0;
  bf_status_t rc = BF_SUCCESS;

  setp_wac_ppg_shr_lmt_shr_lmt_fast_recover_mode(
      val, (ppg->ppg_cfg.fast_recover_mode) ? 1 : 0);
  setp_wac_ppg_shr_lmt_shr_lmt_dyn_en(val, (ppg->ppg_cfg.is_dynamic) ? 1 : 0);

  rc = bf_tm_tofino_populate_wac_offset_profile(
      dev, ppg->p_pipe, ppg->thresholds.ppg_hyst, &(hyst_index));
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  setp_wac_ppg_shr_lmt_shr_lmt_offset_idx(val, hyst_index);
  if (ppg->ppg_cfg.is_dynamic) {
    // Fold in BAF
    setp_wac_ppg_shr_lmt_shr_lmt_lmt(
        val,
        BF_TM_PPG_FOLD_THRES_LIMIT(ppg->thresholds.app_limit,
                                   ppg->ppg_cfg.baf));
  } else {
    setp_wac_ppg_shr_lmt_shr_lmt_lmt(val, ppg->thresholds.app_limit);
  }

  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], ppg->l_pipe, ppg->ppg);
  BF_TM_PPG_CHECK(_ppg, ppg->ppg, ppg->l_pipe, dev);
  _ppg->thresholds.hyst_index = hyst_index;

  return (rc);
}

/* Multi field register ; register set function */
inline static bf_tm_status_t bf_tm_tofino_ppg_set_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_prep_ppg_shr_limit_reg_val(devid, &val, ppg);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .csr_mem_wac_ppg_shr_lmt.shr_lmt[ppg->ppg]),
        val);
  }
  return (rc);
}

inline static bf_tm_status_t bf_tm_tofino_ppg_get_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_ppg_t *ppg, uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_shr_lmt.shr_lmt[ppg->ppg]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_app_limit(bf_dev_id_t devid,
                                              const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_app_limit(bf_dev_id_t devid,
                                              bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  bf_tm_tofino_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->thresholds.app_limit = getp_wac_ppg_shr_lmt_shr_lmt_lmt(&val);
  if (ppg->ppg_cfg.is_dynamic) {
    ppg->thresholds.app_limit =
        BF_TM_PPG_THRES_LIMIT(ppg->thresholds.app_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_hyst(bf_dev_id_t devid,
                                         const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_hyst(bf_dev_id_t devid, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cell_limit;

  rc = bf_tm_tofino_ppg_get_shared_lmt_reg(devid, ppg, &val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get PPG shared limit");
    return rc;
  }
  ppg->thresholds.hyst_index = getp_wac_ppg_shr_lmt_shr_lmt_offset_idx(&val);
  if (ppg->thresholds.hyst_index >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    LOG_ERROR("PPG(%d) threshold idx %d is invalid",
              ppg->ppg,
              ppg->thresholds.hyst_index);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.offset_profile[ppg->thresholds.hyst_index]),
      &cell_limit);
  ppg->thresholds.ppg_hyst = cell_limit << 3;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_app_poolid(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  const bf_tm_ppg_t *neighbour_ppg;
  const bf_tm_ppg_t *_ppg, *def_ppg;
  int index;
  uint32_t val;
  bf_tm_port_t *_p;
  bf_dev_port_t devport;
  uint32_t ppg_enable = 0;
  uint32_t neighbour_ppg_enable = 0;

  // get port , icos associated with ppg
  // if icos_mask is zero, still configure poolID but PPG will not be enabled.
  // Unless icos_mask is applied to PPG, traffic flow of relevant icos will
  // continue to use default PPG.

  index = ppg->port * 4;  // 2 icos per entry
  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->port);
  rc = bf_tm_port_get_descriptor(devid, devport, &_p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get port descriptor for devport %d", devport);
    return (rc);
  }

  for (int i = 0; i < BF_TM_MAX_COS_LEVELS; i++) {
    if ((1 << i) & ppg->ppg_cfg.icos_mask) {
      _p->ppgs[i] = ppg;  // Update ppglist to reflect new non default PPG
    }
  }

  // Get pointer to default-ppg
  def_ppg =
      BF_TM_PPG_PTR(g_tm_ctx[devid],
                    ppg->l_pipe,
                    (g_tm_ctx[devid]->tm_cfg.pfc_ppg_per_pipe + ppg->port));
  BF_TM_PPG_CHECK(def_ppg,
                  (g_tm_ctx[devid]->tm_cfg.pfc_ppg_per_pipe + ppg->port),
                  ppg->l_pipe,
                  devid);

  for (int i = 0; i < BF_TM_MAX_COS_LEVELS; i += 2) {
    _ppg = _p->ppgs[i];
    neighbour_ppg = _p->ppgs[i + 1];

    val = 0;
    if (neighbour_ppg && !(neighbour_ppg->is_default_ppg)) {
      if ((1 << (i + 1)) & neighbour_ppg->ppg_cfg.icos_mask) {
        neighbour_ppg_enable = 1;
      } else {
        neighbour_ppg_enable = 0;
      }
      setp_wac_port_ppg_mapping_entry_ppg1(&val, neighbour_ppg->ppg);
      setp_wac_port_ppg_mapping_entry_apid1(&val,
                                            neighbour_ppg->ppg_cfg.app_poolid);
      setp_wac_port_ppg_mapping_entry_enb1(&val, neighbour_ppg_enable);
    } else {
      // iCoS identified by cos level 'i+1' is mapped to default-ppg.
      // incase of default ppg, HW will ignore ppg#
      setp_wac_port_ppg_mapping_entry_ppg1(&val, def_ppg->ppg);
      setp_wac_port_ppg_mapping_entry_apid1(&val, def_ppg->ppg_cfg.app_poolid);
      setp_wac_port_ppg_mapping_entry_enb1(&val, 0);
    }
    if (_ppg && !(_ppg->is_default_ppg)) {
      if ((1 << i) & _ppg->ppg_cfg.icos_mask) {
        ppg_enable = 1;
      } else {
        ppg_enable = 0;
      }
      setp_wac_port_ppg_mapping_entry_ppg0(&val, _ppg->ppg);
      setp_wac_port_ppg_mapping_entry_apid0(&val, _ppg->ppg_cfg.app_poolid);
      setp_wac_port_ppg_mapping_entry_enb0(&val, ppg_enable);
    } else {
      // iCoS identified by cos level 'i' is mapped to default-ppg.
      // incase of default ppg, HW will ignore ppg#
      setp_wac_port_ppg_mapping_entry_ppg0(&val, def_ppg->ppg);
      setp_wac_port_ppg_mapping_entry_apid0(&val, def_ppg->ppg_cfg.app_poolid);
      setp_wac_port_ppg_mapping_entry_enb0(&val, 0);
    }

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .csr_mem_wac_port_ppg_mapping.entry[index + (i >> 1)]),
        val);
  }

  // If PPG icos-mask is changed/updated/added, also program reverse mapping
  /// icos-->cos for PFC generation on the port where PPG is in use.
  rc |= bf_tm_tofino_port_set_pfc_cos_map(devid, _p);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_app_poolid(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint8_t ppgid = 0, poolid = 0;
  uint32_t val;

  if (!(ppg->ppg_cfg.icos_mask)) {
    // Need to know iCoS before assigning PPG to Pool
    return (BF_INVALID_ARG);
  }

  // poolid for all iCoS traffic mapped to any PFC PPG or default PPG is same.
  // Find first iCoS that is mapped to PPG and use that to lookup table.
  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if ((1 << i) & ppg->ppg_cfg.icos_mask) {
      break;
    }
  }

  if (i >= BF_TM_MAX_PFC_LEVELS) {
    // Either deafult PPG is not assigned to any
    // app poolid and/or to any iCoS
    return (BF_INVALID_ARG);
  }

  index = ppg->port * 4;  // 2 icos per entry
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_port_ppg_mapping.entry[index + (i >> 1)]),
      &val);

  if (i & 0x1) {
    ppgid = getp_wac_port_ppg_mapping_entry_ppg1(&val);
    poolid = getp_wac_port_ppg_mapping_entry_apid1(&val);
  } else {
    ppgid = getp_wac_port_ppg_mapping_entry_ppg0(&val);
    poolid = getp_wac_port_ppg_mapping_entry_apid0(&val);
  }

  ppg->ppg = ppgid;
  ppg->ppg_cfg.app_poolid = poolid;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_dynamic_mode(bf_dev_id_t devid,
                                                 const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_dynamic_mode(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->ppg_cfg.is_dynamic = getp_wac_ppg_shr_lmt_shr_lmt_dyn_en(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_fast_recover_mode(bf_dev_id_t devid,
                                                      bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->ppg_cfg.fast_recover_mode =
      getp_wac_ppg_shr_lmt_shr_lmt_fast_recover_mode(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_baf(bf_dev_id_t devid,
                                        const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_baf(bf_dev_id_t devid, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_tofino_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->ppg_cfg.baf = BF_TM_PPG_BAF(getp_wac_ppg_shr_lmt_shr_lmt_lmt(&val));
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_icos_mask(bf_dev_id_t devid,
                                              bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint32_t val;
  uint16_t _ppg;
  uint8_t icos_mask = 0;

  index = ppg->port * 4;
  for (i = 0; i < 8; i++) {
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .csr_mem_wac_port_ppg_mapping.entry[index + i / 2]),
        &val);
    if (i & 0x1) {
      _ppg = getp_wac_port_ppg_mapping_entry_ppg1(&val);
    } else {
      _ppg = getp_wac_port_ppg_mapping_entry_ppg0(&val);
    }

    if (_ppg == ppg->ppg) {
      icos_mask |= (1 << i);
    }
  }
  ppg->ppg_cfg.icos_mask = icos_mask;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_set_pfc_treatment(bf_dev_id_t devid,
                                                  const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t k, i, port_pfc_enable = 0;
  bf_dev_port_t devport;
  bf_tm_port_t *_port;

  if (ppg->is_default_ppg) {
    // Cannot enable PFC on traffic mapped to default PPG
    return (rc);
  }

  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->port);
  _port = BF_TM_PORT_PTR(g_tm_ctx[devid], devport);

  if (!_port->icos_to_cos_mask) {
    // User has not configured port cos map. Hence it is not possible
    // to map icos to cos for generating PFC message.

    // return and when port_set_pfc_cos_map() is invoked, PFC enable
    // bit will be set if PPG is configured as PFC treatment.
    return (rc);
  }

  /*
   * - we can map multiple iCoS to single PPG.
   * - More than one PPGs (upto 8) can be assigned to a port
   * - For a given port, If a iCoS-X is assigned to PPG1, then
   *   the same iCoS-X cannot be assigned to PPG2 on the same port. That way
   *   if we take intersection of all iCoS vector across any 2 different PPGs
   *   assinged to a port, it will be NULL. There will be no intersection.
   * - The register we are updating to enable or disable is at port level.
   *   This is single register for a port and not a PPG level register. Hence,
   *   we need to collect all CoS bits across all PPGs mapped to the port and
   *   build port_pfc bit vector). For this reason there is outer loop to
   *   walk all PPGs of a port.
   * - The inner loop walks over all PFC levels and each time builds a bit
   *   vector with single icos bit set ( 1 << _port->cos_to_icosk)
   * - The bit vector built is checked for presence in PPGs icos mask.
   *   If present, reverse icos_to_cos_mask is built which gets programmed
   *   hardware.
   */
  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (_port->ppgs[i] &&
        _port->ppgs[i]->ppg_cfg.is_pfc) { /* true if PPG is PFC enabled PPG */
      for (k = 0; k < BF_TM_MAX_PFC_LEVELS; k++) {
        if (_port->cos_to_icos[k] >= BF_TM_MAX_PFC_LEVELS) {
          // Invalid mapping.
          continue;
        }
        if ((1 << _port->cos_to_icos[k]) & _port->ppgs[i]->ppg_cfg.icos_mask) {
          port_pfc_enable |= (1 << k); /* value k maps iCoS --> CoS; Index into
                                        * array cos_to_icos[] is CoS, value
                                        * maintained in the array is iCoS.
                                        */
        }
      }
    }
  }

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.port_pfc_en[ppg->port]),
      port_pfc_enable);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_pfc_treatment(bf_dev_id_t devid,
                                                  bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_tm_port_t *_port;
  uint8_t icos_to_cos_mask = 0, k;
  bf_dev_port_t devport;

  ppg->ppg_cfg.is_pfc = false;
  if (ppg->is_default_ppg) return rc;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.port_pfc_en[ppg->port]),
      &val);

  // Read pfc enable is packet-cos bmask
  // reverse map to icos and check if all icos are part of cos.
  // if true, then PPG is configured for PFC treatment.
  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->port);
  _port = BF_TM_PORT_PTR(g_tm_ctx[devid], devport);
  for (k = 0; k < BF_TM_MAX_PFC_LEVELS; k++) {
    if (_port->cos_to_icos[k] >= BF_TM_MAX_PFC_LEVELS) {
      // Invalid mapping.
      continue;
    }
    if ((1 << _port->cos_to_icos[k]) & ppg->ppg_cfg.icos_mask) {
      icos_to_cos_mask |= (1 << k);
    }
  }
  if (icos_to_cos_mask) {
    if ((val & icos_to_cos_mask) == icos_to_cos_mask) {
      ppg->ppg_cfg.is_pfc = true;
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_resume_limit(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg,
                                                 uint32_t *resume_lmt_cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_resume.ppg_resume[ppg->ppg]),
      &val);
  *resume_lmt_cells = val << 3;
  return rc;
}

bf_tm_status_t bf_tm_tofino_ppg_clear_resume_limit(bf_dev_id_t devid,
                                                   bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_resume.ppg_resume[ppg->ppg]),
      val);
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_get_pfc_state(bf_dev_id_t devid,
                                               bf_tm_port_t *port,
                                               uint8_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .wac_reg.wac_pfc_state[port->port]),
      &val);
  *state = (uint8_t)val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_pfc_state(bf_dev_id_t devid,
                                               bf_tm_port_t *port,
                                               uint8_t icos,
                                               bool state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .wac_reg.wac_pfc_state[port->port]),
      &val);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to get port_pfc_state for dev %d p_pipe %d and port %d",
        devid,
        port->p_pipe,
        port->port);
    return (rc);
  }
  val = state ? (val | (0x1u << icos)) : (val & (~(0x1u << icos)));
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .wac_reg.wac_pfc_state[port->port]),
      val);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to set port_pfc_state for dev %d p_pipe %d and port %d",
        devid,
        port->p_pipe,
        port->port);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_pfc_state(bf_dev_id_t devid,
                                                 bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .wac_reg.wac_pfc_state[port->port]),
      val);
  return (rc);
}

/* In order to recover from driver restart and rebuild PPG allocation
 * read back port_to_ppg mapping table and return reverse mapping.
 * Used when restoring PPG config.
 */
bf_tm_status_t bf_tm_tofino_ppg_get_ppg_allocation(bf_dev_id_t devid,
                                                   bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint32_t val;

  index = ppg->port * 4;
  for (i = 0; i < 8; i++) {
    if ((1 << i) & ppg->ppg_cfg.icos_mask) {
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                       .csr_mem_wac_port_ppg_mapping.entry[index + i / 2]),
          &val);
      if (i & 0x1) {
        ppg->ppg = getp_wac_port_ppg_mapping_entry_ppg1(&val);
        ppg->is_default_ppg =
            getp_wac_port_ppg_mapping_entry_enb1(&val) ? false : true;
        ppg->ppg_cfg.app_poolid = getp_wac_port_ppg_mapping_entry_apid1(&val);
      } else {
        ppg->ppg = getp_wac_port_ppg_mapping_entry_ppg0(&val);
        ppg->is_default_ppg =
            getp_wac_port_ppg_mapping_entry_enb0(&val) ? false : true;
        ppg->ppg_cfg.app_poolid = getp_wac_port_ppg_mapping_entry_apid0(&val);
      }
      break;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_drop_counter(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg,
                                                 uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt = 0;
  uint64_t cnt_1 = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_drop_count_ppg[ppg->ppg]
                   .wac_drop_count_ppg_0_2),
      &val);
  cnt = val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_drop_count_ppg[ppg->ppg]
                   .wac_drop_count_ppg_1_2),
      &val);
  cnt_1 = val;

  // 40 bit counter with 7 bit ecc
  cnt_1 = cnt_1 & 0xFF;
  *count = (cnt_1 << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_drop_state(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg,
                                               bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (ppg->ppg < 32) {  // 0-31
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_0_4),
        &val);
  } else if (ppg->ppg >= 32 && ppg->ppg < 64) {  // 32-63
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_1_4),
        &val);
  } else if (ppg->ppg >= 64 && ppg->ppg < 96) {  // 64-95
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_2_4),
        &val);
  } else if (ppg->ppg >= 96 && ppg->ppg < 128) {  // 96-127
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state0.wac_ppg_drop_state0_3_4),
        &val);
  } else if (ppg->ppg >= 128 && ppg->ppg < 160) {  // another reg for 128-200
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_0_4),
        &val);
  } else if (ppg->ppg >= 160 && ppg->ppg < 192) {  // 160-191
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_1_4),
        &val);
  } else if (ppg->ppg >= 192 && ppg->ppg < 201) {  // 192-200
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                     .wac_reg.wac_ppg_drop_state1.wac_ppg_drop_state0_2_4),
        &val);
  }

  *state = (val >> (ppg->ppg % 32)) & 0x1;
  return rc;
}

bf_tm_status_t bf_tm_tofino_ppg_get_gmin_usage_counter(bf_dev_id_t devid,
                                                       bf_tm_ppg_t *ppg,
                                                       uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_min_cnt.min_cnt[ppg->ppg]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_clear_gmin_usage_counter(bf_dev_id_t devid,
                                                         bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_min_cnt.min_cnt[ppg->ppg]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_shared_usage_counter(bf_dev_id_t devid,
                                                         bf_tm_ppg_t *ppg,
                                                         uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_shr_cnt.shr_cnt[ppg->ppg]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_clear_shared_usage_counter(bf_dev_id_t devid,
                                                           bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_shr_cnt.shr_cnt[ppg->ppg]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_skid_usage_counter(bf_dev_id_t devid,
                                                       bf_tm_ppg_t *ppg,
                                                       uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  if (ppg->is_default_ppg) {
    // For non PFC PPGs, there is no head room / skid space.
    *count = 0;
    return (rc);
  }

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_hdr_cnt.hdr_cnt[ppg->ppg]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_clear_skid_usage_counter(bf_dev_id_t devid,
                                                         bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (ppg->is_default_ppg) {
    // For non PFC PPGs, there is no head room / skid space.
    return (rc);
  }

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_hdr_cnt.hdr_cnt[ppg->ppg]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_get_wm_counter(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_wm_cnt.wm_cnt[ppg->ppg]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_clear_watermark(bf_dev_id_t devid,
                                                bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .csr_mem_wac_ppg_wm_cnt.wm_cnt[ppg->ppg]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ppg_clear_drop_counter(bf_dev_id_t devid,
                                                   bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_drop_count_ppg[ppg->ppg]
                   .wac_drop_count_ppg_1_2),
      0);

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.wac_drop_count_ppg[ppg->ppg]
                   .wac_drop_count_ppg_0_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_wac_get_buffer_full_counter(bf_dev_id_t devid,
                                                        bf_dev_pipe_t pipe,
                                                        uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cnt;
  uint64_t reg_cnt;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.wac_drop_buf_full.wac_drop_buf_full_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.wac_drop_buf_full.wac_drop_buf_full_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_wac_clear_buffer_full_counter(bf_dev_id_t devid,
                                                          bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.wac_drop_buf_full.wac_drop_buf_full_0_2),
      0);

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.wac_drop_buf_full.wac_drop_buf_full_1_2),
      0);

  return (rc);
}

/////////////////queue////////////////////////////

#define BF_TM_Q_FOLD_THRES_LIMIT(limit, baf) ((limit << 4) | baf)
#define BF_TM_Q_THRES_LIMIT(val) (val >> 4)
#define BF_TM_Q_BAF(val) (val & 0xf)
#define Q_PER_EG_MEMORY (32)
// Poolid, color_drop_en, tail_drop_en
// shr_lmt, offset_idx, dyn_en, fast_recover_mode
// red_lmt_perc, red_offset_idx, yel_lmt_perc, yel_offset_idx
// Port_thrd, offset_idx

bf_tm_status_t bf_tm_tofino_q_carve_queues(bf_dev_id_t devid,
                                           bf_dev_port_t port,
                                           bf_dev_pipe_t physical_pipe,
                                           int q_profile_indx,
                                           bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;
  int j;
  bf_dev_pipe_t pipe = physical_pipe;
  int lport = DEV_PORT_TO_LOCAL_PORT(port);
  if (!LOCAL_PORT_VALIDATE(lport)) {
    LOG_ERROR(
        "%s: Invalid local port. Pipe (%d) lport (%d)", __func__, pipe, lport);
    return BF_TM_EINT;
  }

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                   .qac_reg.qac_qid_profile_config[lport]),
      q_profile_indx);
  for (j = 0; j < 32; j++) {
    // Qac_Qid mapping
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                     .qac_reg.qac_qid_mapping[(q_profile_indx * 32) + j]),
        q_profile->q_mapping[j] + q_profile->base_q);
    // Wac_Egress_Qid mapping
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_egress_qid_mapping[(q_profile_indx * 32) + j]),
        q_profile->q_mapping[j] + q_profile->base_q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_get_q_profiles_mapping(
    bf_dev_id_t devid, bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;
  for (uint32_t i = 0; i < BF_TM_TOFINO_Q_PROF_CNT; i++) {
    rc = bf_tm_tofino_get_q_profile_mapping(devid, 0, i, &(q_profile[i]));
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "%s: Unable to restore queue profile configuration for dev %d, "
          "profile idx %d, rc %s (%d)",
          __func__,
          devid,
          i,
          bf_err_str(rc),
          rc);
      return (rc);
    }
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_get_q_profile_mapping(
    bf_dev_id_t devid,
    int physical_pipe,
    int q_profile_indx,
    bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;
  int j;
  (void)physical_pipe;
  uint32_t ret;

  /*
   * QAC has per pipe queue profile table whereas WAC has one
   * global table for all pipes. Since QAC and WAC should have the
   * same queue mapping, we use only one global table. So, return the
   * queue mapping from WAC table as QAC is per pipe table and may
   * not have the correct entries on all pipes unless any port in the pipe
   * is using the same profile index. This is important for hitless HA to
   * restore the queue mapping for a profile index at one shot.
   */
  for (j = 0; j < 32; j++) {
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_egress_qid_mapping[(q_profile_indx * 32) + j]),
        &(ret));
    q_profile->q_mapping[j] = ret;
  }
  return (rc);
}

// qid_map_ingress_mau_to_tm(port_9b, qid_7b) -> pipe_2, qid_11b
// Get mapping from devport {log_pipe_2b, port_7b} and ingress qid{qid_7b}
// to logical pipe {p_pipe_2b} and physical queue {phys_q_11b}
bf_tm_status_t bf_tm_tofino_get_phys_q(bf_dev_id_t devid,
                                       bf_dev_port_t devport,
                                       uint32_t ing_q,
                                       bf_dev_pipe_t *log_pipe,
                                       bf_tm_queue_t *phys_q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t logical_pipe = DEV_PORT_TO_PIPE(devport);

  if (TM_IS_DEV_INVALID(devid) || !(g_tm_ctx[devid]) ||
      TM_IS_PIPE_INVALID(logical_pipe, g_tm_ctx[devid])) {
    LOG_ERROR("Invalid pipe at dev_id %d dev_port=%d", devid, devport);
    return BF_TM_EINV_ARG;
  }
  uint32_t local_port = DEV_PORT_TO_LOCAL_PORT(devport);
  if (!LOCAL_PORT_VALIDATE(local_port)) {
    LOG_ERROR("Invalid pipe=%d local port %d for dev=%d dev_port=%d",
              logical_pipe,
              local_port,
              devid,
              devport);
    return BF_TM_EINV_ARG;
  }
  uint32_t qprof_idx = 0;
  bf_dev_pipe_t phys_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, logical_pipe, &phys_pipe);
  if (rc) {
    LOG_ERROR("Unable to map dev=%d pipe %d to physical pipe, rc=%d",
              devid,
              logical_pipe,
              rc);
    return BF_TM_EINT;
  }
  rc = bf_tm_tofino_get_port_q_profile(devid, devport, phys_pipe, &qprof_idx);
  if (rc) {
    LOG_ERROR(
        "Unable to get queue profile dev=%d dev_port=%d, phys_pipe=%d, rc=%d",
        devid,
        devport,
        phys_pipe,
        rc);
    return rc;
  }

  uint32_t pg_idx = ((qprof_idx & 0xf) << 5) | (ing_q & 0x1f);
  uint32_t qid = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[logical_pipe]
                   .qac_reg.qac_qid_mapping[pg_idx]),
      &qid);
  if (rc) {
    LOG_ERROR(
        "Unable to read queue mapping dev=%d dev_port=%d, pg_idx=%d, rc=%d",
        devid,
        devport,
        pg_idx,
        rc);
    return BF_TM_EINT;
  }

  uint32_t pg_num = local_port & 0xf;
  *phys_q = qid + (BF_TM_TOFINO_QUEUES_PER_PG * pg_num);
  *log_pipe = logical_pipe;
  return rc;
}
//
bf_tm_status_t bf_tm_tofino_get_qac_qid_mapping(bf_dev_id_t devid,
                                                bf_dev_pipe_t pipe,
                                                uint32_t port,
                                                uint8_t *data) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t phys_pipe = 0;
  uint32_t qprof_idx = 0;
  rc |= lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &phys_pipe);
  bf_dev_port_t devport = MAKE_DEV_PORT(pipe, port);
  rc |= bf_tm_tofino_get_port_q_profile(devid, devport, phys_pipe, &qprof_idx);

  uint32_t qid = 0;
  for (qid = 0; qid < 32; ++qid) {
    uint32_t val = 0;
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[phys_pipe]
                     .qac_reg.qac_qid_mapping[(qprof_idx << 5) + qid]),
        &val);
    data[qid] = val;
    if (rc != BF_TM_EOK) {
      rc = BF_TM_EINT;
    }
  }

  return rc;
}

bf_tm_status_t bf_tm_tofino_clear_qac_qid_profile(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t entry) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                   .qac_reg.qac_qid_profile_config[entry]),
      0);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_clear_qac_qid_mapping(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t entry) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t phys_pipe = 0;
  rc |= lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &phys_pipe);
  uint32_t qid = 0;
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[phys_pipe]
                   .qac_reg.qac_qid_mapping[entry]),
      qid);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_get_wac_eg_qid_mapping(bf_dev_id_t devid,
                                                   uint32_t entry,
                                                   uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t qid = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_egress_qid_mapping[entry]),
      &qid);
  *val = qid;
  if (BF_TM_EOK != rc) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_clear_wac_eg_qid_mapping(bf_dev_id_t devid,
                                                     uint32_t entry) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t qid = 0;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_egress_qid_mapping[entry]),
      qid);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_get_port_q_profile(bf_dev_id_t devid,
                                               bf_dev_port_t port,
                                               bf_dev_pipe_t physical_pipe,
                                               uint32_t *q_profile_indx) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t pipe = physical_pipe;
  int lport = DEV_PORT_TO_LOCAL_PORT(port);
  if (!LOCAL_PORT_VALIDATE(lport)) {
    LOG_ERROR(
        "%s: Invalid local port. Pipe (%d) lport (%d)", __func__, pipe, lport);
    return BF_TM_EINT;
  }

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                   .qac_reg.qac_qid_profile_config[lport]),
      q_profile_indx);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_min_limit(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  val = q->thresholds.min_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_min_thrd_config.entry[q->physical_q]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_min_limit(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_min_thrd_config.entry[q->physical_q]),
      &val);
  q->thresholds.min_limit = val;
  return (rc);
}

// Tofino implements an array of 32 entries to specify hysteresis values.
// Instead of programming actual hysteresis value in tables, index of the
// 32 entry table is used. This index is refered as offset_idx in register
// spec. QAC offset proile table is used for HYST on Queue and Port.
// bf_tm_tofino_restore_qac_offset_profile() can be used to restore
// the following 2 SW tables/variables for hitless HA case.

// TODO: At some point all the offset_profiles will be occupied with some
// values which actually might be not used by any Port, Queue.
// Some reference counting and housekeeping is needed to reuse
// this limited resource.

static uint8_t qac_offset_profile_cnt[BF_TM_NUM_ASIC][BF_TM_TOFINO_MAU_PIPES] =
    {{0}};
static uint16_t qac_offset_profile_tbl[BF_TM_NUM_ASIC][BF_TM_TOFINO_MAU_PIPES]
                                      [BF_TM_TOFINO_HYSTERESIS_PROFILES] = {
                                          {{0}}};

bf_status_t bf_tm_tofino_restore_qac_offset_profile(bf_dev_id_t dev) {
  uint32_t num_p_pipes = 0;
  uint32_t val;

  g_tm_ctx[dev]->read_por_qac_profile_table = false;

  // Clear all the array, even if some pipes ara not used on the device.
  for (int p = 0; p < BF_TM_TOFINO_MAU_PIPES; p++) {
    qac_offset_profile_cnt[dev][p] = 0;
  }

  lld_err_t lld_err = lld_sku_get_num_active_pipes(dev, &num_p_pipes);
  if ((LLD_OK != lld_err) || (num_p_pipes > BF_TM_TOFINO_MAU_PIPES)) {
    LOG_ERROR(
        "Restore QAC hyst. on dev %d can't get number of physical pipes, "
        "lld_rc=%d",
        dev,
        lld_err);
    return (BF_INTERNAL_ERROR);
  }

  for (bf_dev_pipe_t l_pipe = 0; l_pipe < num_p_pipes; l_pipe++) {
    bool got_default_value = false;
    bool got_reset_value = false;
    bf_dev_pipe_t p_pipe = 0;

    lld_err = lld_sku_map_pipe_id_to_phy_pipe_id(dev, l_pipe, &p_pipe);
    if (LLD_OK != lld_err) {
      LOG_ERROR(
          "Restore QAC hyst. on dev %d l_pipe %d can't map to physical pipe, "
          "lld_rc=%d",
          dev,
          l_pipe,
          lld_err);
      return (BF_INTERNAL_ERROR);
    }

    for (int i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
      (void)bf_tm_read_register(
          dev,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[p_pipe]
                       .qac_reg.offset_profile[i]),
          &val);
      LOG_TRACE("Restore dev %d l_pipe %d p_pipe %d QAC hyst[%d]=0x%x",
                dev,
                l_pipe,
                p_pipe,
                i,
                val);
      // Ignore SW model garbage
      val = (TM_IS_TARGET_ASIC(dev)) ? val : BF_TM_TOFINO_QAC_RESET_HYSTERESIS;
      qac_offset_profile_tbl[dev][p_pipe][i] = val;
      // Treat default values as empty items except the first one.
      // It might be at any index in the array.
      // Non-POR HW reset values are counted as empty items and will be reused
      // except the first one greate than POR.
      if ((got_reset_value && val == BF_TM_TOFINO_QAC_RESET_HYSTERESIS) ||
          (got_default_value && val <= BF_TM_TOFINO_QAC_POR_HYSTERESIS)) {
        continue;
      }

      qac_offset_profile_cnt[dev][p_pipe]++;
      if (val <= BF_TM_TOFINO_QAC_POR_HYSTERESIS) {
        got_default_value = true;
      }
      if (val == BF_TM_TOFINO_QAC_RESET_HYSTERESIS) {
        got_reset_value = true;
      }
    }
    LOG_TRACE("dev %d l_pipe %d p_pipe %d QAC %d hyst items",
              dev,
              l_pipe,
              p_pipe,
              qac_offset_profile_cnt[dev][p_pipe]);
  }

  g_tm_ctx[dev]->read_por_qac_profile_table = true;
  return (BF_SUCCESS);
}

static uint8_t bf_tm_tofino_get_qac_offset_profile_index(bf_dev_id_t dev,
                                                         bf_tm_thres_t cells_u,
                                                         uint8_t p_pipe) {
  bf_status_t rc = BF_SUCCESS;
  int i;

  // search from the start till end to check if cell_limit is already
  // in profile table.
  if (!g_tm_ctx[dev]->read_por_qac_profile_table) {
#ifndef BF_TM_HITLESS_HA_TESTING_WITH_MODEL
    rc = bf_tm_tofino_restore_qac_offset_profile(dev);
    if (BF_SUCCESS != rc) {
      return (BF_TM_TOFINO_HYSTERESIS_PROFILES);
    }
#else
    LOG_TRACE("clear all QAC hyst profiles dev %d", dev);
    for (int p = 0; p < BF_TM_TOFINO_MAU_PIPES; p++) {
      qac_offset_profile_cnt[dev][p] = 0;
      for (i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
        qac_offset_profile_tbl[dev][p][i] = BF_TM_TOFINO_QAC_RESET_HYSTERESIS;
      }
    }
    g_tm_ctx[dev]->read_por_qac_profile_table = true;
#endif
  }

  /* For the hysteresis values, check if it is configured already */
  for (i = 0; i < BF_TM_TOFINO_HYSTERESIS_PROFILES; i++) {
    if (qac_offset_profile_tbl[dev][p_pipe][i] == cells_u) {
      return (i);
    }
  }
  return (BF_TM_TOFINO_HYSTERESIS_PROFILES);
}

static bf_status_t bf_tm_tofino_check_qac_offset_profile(bf_dev_id_t dev,
                                                         uint8_t p_pipe,
                                                         bf_tm_thres_t cells_u,
                                                         uint8_t idx_expected) {
  int idx = bf_tm_tofino_get_qac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    // Check the expected index as well
    if ((BF_TM_TOFINO_HYSTERESIS_PROFILES != idx_expected) &&
        (idx != idx_expected)) {
      LOG_ERROR("dev %d pipe %d QAC hyst[%d] is %d instead of hyst[%d]",
                dev,
                p_pipe,
                idx,
                qac_offset_profile_tbl[dev][p_pipe][idx],
                idx_expected);
      return (BF_ALREADY_EXISTS);
    }
    return (BF_SUCCESS);
  }
  LOG_ERROR("dev %d pipe %d QAC hyst[%d]=%d is not registered",
            dev,
            p_pipe,
            idx,
            cells_u);
  return (BF_TABLE_NOT_FOUND);
}

static bf_status_t bf_tm_tofino_populate_qac_offset_profile(bf_dev_id_t dev,
                                                            uint8_t p_pipe,
                                                            bf_tm_thres_t cells,
                                                            uint8_t *index) {
  int idx;
  bf_status_t rc = BF_SUCCESS;
  uint32_t cells_u = TM_CELLS_TO_8CELL_UNITS(cells);

  // For dynamic sharing to work, hysteresis shouldn't be
  // 0 - the reason why POR value is 4 in 8 cells unit i.e 32.
  // Add default item if it is not yet in the table.
  if (cells_u < BF_TM_TOFINO_QAC_POR_HYSTERESIS) {
    cells_u = BF_TM_TOFINO_QAC_POR_HYSTERESIS;
  }

  idx = bf_tm_tofino_get_qac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    // Already exists...
    *index = idx;
    return (rc);
  }
  if (qac_offset_profile_cnt[dev][p_pipe] >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    LOG_ERROR("No QAC hysteresis profiles left on dev %d pipe %d to store %d",
              dev,
              p_pipe,
              cells);
    rc = BF_NO_SYS_RESOURCES;
  } else {
    idx = 0;
    if (qac_offset_profile_cnt[dev][p_pipe]) {
      bool got_default_value = false;
      bool got_reset_value = false;
      // Find first empty item
      for (idx = 0; idx < BF_TM_TOFINO_HYSTERESIS_PROFILES; idx++) {
        if (qac_offset_profile_tbl[dev][p_pipe][idx] <=
            BF_TM_TOFINO_QAC_POR_HYSTERESIS) {
          if (got_default_value) {
            // no default values yet in registers or an empty one
            LOG_DBG("Reuse QAC hyst[%d]=%d of %d at dev %d pipe %d",
                    idx,
                    qac_offset_profile_tbl[dev][p_pipe][idx],
                    qac_offset_profile_cnt[dev][p_pipe],
                    dev,
                    p_pipe);
            break;
          }
          LOG_DBG("Keep QAC default hyst[%d]=%d of %d at dev %d pipe %d",
                  idx,
                  qac_offset_profile_tbl[dev][p_pipe][idx],
                  qac_offset_profile_cnt[dev][p_pipe],
                  dev,
                  p_pipe);
          got_default_value = true;  // Keep the first one.
        } else if (qac_offset_profile_tbl[dev][p_pipe][idx] ==
                   BF_TM_TOFINO_QAC_RESET_HYSTERESIS) {
          // When HW reset value is greater than POR
          if (got_reset_value) {
            // no default values yet in registers or an empty one
            LOG_DBG("Reuse QAC hyst[%d]=%d of %d at dev %d pipe %d",
                    idx,
                    qac_offset_profile_tbl[dev][p_pipe][idx],
                    qac_offset_profile_cnt[dev][p_pipe],
                    dev,
                    p_pipe);
            break;
          }
          LOG_DBG("Keep QAC reset hyst[%d]=%d of %d at dev %d pipe %d",
                  idx,
                  qac_offset_profile_tbl[dev][p_pipe][idx],
                  qac_offset_profile_cnt[dev][p_pipe],
                  dev,
                  p_pipe);
          got_reset_value = true;  // Keep the first one.
        }
      }
      if (idx >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
        LOG_ERROR("Inconsistent QAC hysteresis profiles on dev %d pipe %d",
                  dev,
                  p_pipe);
        return (BF_NO_SYS_RESOURCES);
      }
    }
    LOG_TRACE("Set QAC hyst[%d]=%d of %d at dev %d pipe %d",
              idx,
              cells_u,
              qac_offset_profile_cnt[dev][p_pipe],
              dev,
              p_pipe);
    qac_offset_profile_tbl[dev][p_pipe][idx] = cells_u;
    *index = idx;
    // Write to HW
    (void)bf_tm_write_register(
        dev,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[p_pipe]
                     .qac_reg.offset_profile[*index]),
        cells_u);
    qac_offset_profile_cnt[dev][p_pipe]++;
  }
  return (rc);
}

/* Multi field register ; register value preparation function */
inline static bf_status_t bf_tm_prep_q_shr_limit_reg_val(bf_dev_id_t dev,
                                                         uint32_t *val,
                                                         bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  setp_qac_queue_shr_thrd_config_entry_fast_recover_mode(
      val, (q->q_cfg.fast_recover_mode) ? 1 : 0);
  setp_qac_queue_shr_thrd_config_entry_dyn_en(val,
                                              (q->q_cfg.is_dynamic) ? 1 : 0);

  rc = bf_tm_tofino_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.app_hyst, &(q->thresholds.app_hyst_index));
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  setp_qac_queue_shr_thrd_config_entry_offset_idx(val,
                                                  q->thresholds.app_hyst_index);
  if (q->q_cfg.is_dynamic) {
    // Fold in BAF
    setp_qac_queue_shr_thrd_config_entry_shr_lmt(
        val, BF_TM_Q_FOLD_THRES_LIMIT(q->thresholds.app_limit, q->q_cfg.baf));
  } else {
    setp_qac_queue_shr_thrd_config_entry_shr_lmt(val, q->thresholds.app_limit);
  }
  return (rc);
}

/* Multi field register ; register set function */
inline static bf_tm_status_t bf_tm_tofino_q_set_shared_lmt_reg(
    bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_prep_q_shr_limit_reg_val(devid, &val, q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                     .csr_mem_qac_queue_shr_thrd_config.entry[q->physical_q]),
        val);
  }
  return (rc);
}

inline static bf_tm_status_t bf_tm_tofino_q_get_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_eg_q_t *q, uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_shr_thrd_config.entry[q->physical_q]),
      val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_app_limit(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_app_limit(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_shared_lmt_reg(devid, q, &val);
  q->thresholds.app_limit = getp_qac_queue_shr_thrd_config_entry_shr_lmt(&val);
  if (q->q_cfg.is_dynamic) {
    q->thresholds.app_limit = BF_TM_Q_THRES_LIMIT(q->thresholds.app_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_is_dynamic(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tofino_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_is_dynamic(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_shared_lmt_reg(devid, q, &val);
  q->q_cfg.is_dynamic = getp_qac_queue_shr_thrd_config_entry_dyn_en(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_fast_recover_mode(bf_dev_id_t devid,
                                                    bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_shared_lmt_reg(devid, q, &val);
  q->q_cfg.fast_recover_mode =
      getp_qac_queue_shr_thrd_config_entry_fast_recover_mode(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_baf(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tofino_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_baf(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_shared_lmt_reg(devid, q, &val);
  q->thresholds.app_limit = getp_qac_queue_shr_thrd_config_entry_shr_lmt(&val);
  if (q->q_cfg.is_dynamic) {
    q->q_cfg.baf = BF_TM_Q_BAF(q->thresholds.app_limit);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_app_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tofino_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_app_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cell_limit;

  rc = bf_tm_tofino_q_get_shared_lmt_reg(devid, q, &val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get queue shared limit");
    return rc;
  }
  q->thresholds.app_hyst_index =
      getp_qac_queue_shr_thrd_config_entry_offset_idx(&val);
  if (q->thresholds.yel_hyst_index >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.offset_profile[q->thresholds.app_hyst_index]),
      &cell_limit);
  q->thresholds.app_hyst = (cell_limit << 3);
  return (rc);
}

inline static void bf_tm_prep_q_color_limit_reg_val(bf_dev_id_t dev,
                                                    uint32_t *val,
                                                    bf_tm_eg_q_t *q) {
  setp_qac_queue_color_limit_entry_yel_lmt_perc(val,
                                                q->thresholds.yel_limit_pcent);
  setp_qac_queue_color_limit_entry_red_lmt_perc(val,
                                                q->thresholds.red_limit_pcent);
  bf_tm_tofino_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.red_hyst, &(q->thresholds.red_hyst_index));
  bf_tm_tofino_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.yel_hyst, &(q->thresholds.yel_hyst_index));
  setp_qac_queue_color_limit_entry_yel_offset_idx(val,
                                                  q->thresholds.yel_hyst_index);
  setp_qac_queue_color_limit_entry_red_offset_idx(val,
                                                  q->thresholds.red_hyst_index);
}

inline static bf_tm_status_t bf_tm_tofino_q_set_color_lmt_reg(bf_dev_id_t devid,
                                                              bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_prep_q_color_limit_reg_val(devid, &val, q);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_color_limit.entry[q->physical_q]),
      val);
  return (rc);
}

inline static bf_tm_status_t bf_tm_tofino_q_get_color_lmt_reg(
    bf_dev_id_t devid, const bf_tm_eg_q_t *q, uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_color_limit.entry[q->physical_q]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_yel_limit_pcent(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tofino_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_yel_limit_pcent(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  bf_tm_tofino_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.yel_limit_pcent =
      getp_qac_queue_color_limit_entry_yel_lmt_perc(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_red_limit_pcent(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tofino_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_red_limit_pcent(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  bf_tm_tofino_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.red_limit_pcent =
      getp_qac_queue_color_limit_entry_red_lmt_perc(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_yel_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_yel_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cell;

  rc = bf_tm_tofino_q_get_color_lmt_reg(devid, q, &val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get queue yellow hyst");
    return rc;
  }

  q->thresholds.yel_hyst_index =
      getp_qac_queue_color_limit_entry_yel_offset_idx(&val);

  if (q->thresholds.yel_hyst_index >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.offset_profile[q->thresholds.yel_hyst_index]),
      &cell);
  q->thresholds.yel_hyst = (cell << 3);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_red_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_red_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cell;

  bf_tm_tofino_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.red_hyst_index =
      getp_qac_queue_color_limit_entry_red_offset_idx(&val);

  if (q->thresholds.yel_hyst_index >= BF_TM_TOFINO_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.offset_profile[q->thresholds.red_hyst_index]),
      &cell);
  q->thresholds.red_hyst = (cell << 3);

  return (rc);
}

inline static void bf_tm_prep_q_pool_cfg(uint32_t *val, bf_tm_eg_q_t *q) {
  setp_qac_queue_ap_config_entry_ap_id(val, q->q_cfg.app_poolid);
  setp_qac_queue_ap_config_entry_q_color_drop_en(val, q->q_cfg.color_drop_en);
  setp_qac_queue_ap_config_entry_q_drop_en(val, q->q_cfg.tail_drop_en);
}

inline static bf_tm_status_t bf_tm_tofino_q_set_pool_cfg(bf_dev_id_t devid,
                                                         bf_tm_eg_q_t *q) {
  uint32_t val = 0;
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_prep_q_pool_cfg(&val, q);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_ap_config.entry[q->physical_q]),
      val);
  return (rc);
}
inline static bf_tm_status_t bf_tm_tofino_q_get_pool_cfg(bf_dev_id_t devid,
                                                         const bf_tm_eg_q_t *q,
                                                         uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_ap_config.entry[q->physical_q]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_app_poolid(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_app_poolid(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.app_poolid = getp_qac_queue_ap_config_entry_ap_id(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_color_drop_en(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_color_drop_en(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.color_drop_en = getp_qac_queue_ap_config_entry_q_color_drop_en(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_tail_drop_en(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tofino_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_tail_drop_en(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tofino_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.tail_drop_en = getp_qac_queue_ap_config_entry_q_drop_en(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_set_neg_mir_dest(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  setp_qac_pipe_config_defd_port(&val, q->port);

  // This register keeps port_group 'physical' queue number,
  // neither a pipeline QID, nor port queue_nr.
  setp_qac_pipe_config_defd_qid(&val, q->logical_q);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.pipe_config),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_neg_mir_dest(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.pipe_config),
      &val);
  q->port = getp_qac_pipe_config_defd_port(&val);
  q->uport = DEV_PORT_TO_LOCAL_PORT(lld_sku_map_devport_from_device_to_user(
      devid, MAKE_DEV_PORT(q->l_pipe, q->port)));

  // This register keeps port_group 'physical' queue number,
  // neither a pipeline QID, nor port queue_nr.
  uint8_t qid = getp_qac_pipe_config_defd_qid(&val);
  bf_tm_eg_q_t *qid_q = NULL;

  if (!LOCAL_PORT_VALIDATE(q->uport) || qid >= BF_TM_TOFINO_QUEUES_PER_PG) {
    LOG_ERROR(
        "Read incorrect negative mirror destination dev=%d, pipe=%d, local "
        "port=%d(%d), qid=%d",
        devid,
        q->l_pipe,
        q->port,
        q->uport,
        qid);
    // Mostly as SW Model Hitless workaround
    bf_dev_port_t dft_port = 0;
    bf_tm_queue_t dft_queue = 0;
    rc = bf_tm_port_mirror_on_drop_dest_get_default(
        devid, q->l_pipe, &dft_port, &dft_queue);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
    q->port = DEV_PORT_TO_LOCAL_PORT(
        lld_sku_map_devport_from_user_to_device(devid, dft_port));
    q->uport = DEV_PORT_TO_LOCAL_PORT(dft_port);
    qid = dft_queue;
  }

  qid_q = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[devid],
                                  MAKE_DEV_PORT(q->l_pipe, q->uport)) +
          qid;

  if (NULL != qid_q && qid_q->logical_q == qid) {
    q->logical_q = qid_q->logical_q;
    q->physical_q = qid_q->physical_q;
  } else {
    rc = BF_UNEXPECTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_drop_counter(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  uint64_t reg_cnt;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.qac_queue_drop_count_addr),
      q->physical_q);

  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
              .qac_reg.qac_queue_drop_count_data.qac_queue_drop_count_data_0_2),
      &val);
  *count = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
              .qac_reg.qac_queue_drop_count_data.qac_queue_drop_count_data_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + *count;

  return (rc);
}

static bool bf_tm_tofino_q_select_drop_state_for_particular_queue(
    uint32_t val, uint8_t queue) {
  return (val >> (queue % Q_PER_EG_MEMORY)) & 0x1;
}

bf_tm_status_t bf_tm_tofino_q_get_egress_drop_state(bf_dev_id_t devid,
                                                    bf_tm_eg_q_t *q,
                                                    bf_tm_color_t color,
                                                    bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  uint32_t offset = q->physical_q / Q_PER_EG_MEMORY;
  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_state[offset]),
          &val);

      break;
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_yel_state[offset]),
          &val);
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_red_state[offset]),
          &val);
      break;
    default:
      return BF_TM_EINV_ARG;
  }
  *state =
      bf_tm_tofino_q_select_drop_state_for_particular_queue(val, q->physical_q);
  return rc;
}

static uint32_t bf_tm_tofino_q_clear_drop_state_for_particular_queue(
    uint32_t val, uint8_t queue) {
  return (val & (~(1 << (queue % Q_PER_EG_MEMORY))));
}

bf_tm_status_t bf_tm_tofino_q_clear_egress_drop_state(bf_dev_id_t devid,
                                                      bf_tm_eg_q_t *q,
                                                      bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t offset = q->physical_q / Q_PER_EG_MEMORY;
  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_state[offset]),
          &val);
      if (BF_SUCCESS != rc) return BF_TM_EINT;
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_state[offset]),
          bf_tm_tofino_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
      break;
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_yel_state[offset]),
          &val);
      if (BF_SUCCESS != rc) return BF_TM_EINT;
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_yel_state[offset]),
          bf_tm_tofino_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_red_state[offset]),
          &val);
      if (BF_SUCCESS != rc) return BF_TM_EINT;
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                       .qac_reg.queue_drop_red_state[offset]),
          bf_tm_tofino_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
      break;
    default:
      return BF_TM_EINV_ARG;
  }

  return rc;
}

bf_tm_status_t bf_tm_tofino_q_clear_drop_counter(bf_dev_id_t devid,
                                                 bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .qac_reg.qac_queue_drop_count_addr),
      q->physical_q);

  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
              .qac_reg.qac_queue_drop_count_data.qac_queue_drop_count_data_0_2),
      0);
  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
              .qac_reg.qac_queue_drop_count_data.qac_queue_drop_count_data_1_2),
      0);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_usage_counter(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q,
                                                uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_cell_count.entry[q->physical_q]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_clear_usage_counter(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_cell_count.entry[q->physical_q]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_wm_counter(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q,
                                             uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_wm_cell_count.count[q->physical_q]),
      &val);
  *count = val;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_clear_watermark(bf_dev_id_t devid,
                                              bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[q->p_pipe]
                   .csr_mem_qac_queue_wm_cell_count.count[q->physical_q]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_get_skidpool_drop_state_shadow(bf_dev_id_t devid,
                                                             bf_tm_eg_q_t *q,
                                                             uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_wac_top.wac_common.wac_common_block_drop_st
              .wac_queue_state_shadow[q->physical_q]),
      &val);
  *state = val & 0xfffful;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_q_clear_skidpool_drop_state_shadow(
    bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_wac_top.wac_common.wac_common_block_drop_st
              .wac_queue_state_shadow[q->physical_q]),
      val);
  return (rc);
}

/////////////////IG-POOLS////////////////////////////

bf_tm_status_t bf_tm_tofino_ig_spool_set_red_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.red_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_red_limit_cell[pool]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_red_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc =
      bf_tm_read_register(devid,
                          offsetof(Tofino,
                                   device_select.tm_top.tm_wac_top.wac_common
                                       .wac_common.wac_ap_red_limit_cell[pool]),
                          &val);
  ig_spool->threshold.red_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_red_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->red_hyst;
  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_ap_red_offset_cell),
                            val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_red_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_ap_red_offset_cell),
                           &val);
  ig_spool->red_hyst = val;
  ig_spool->red_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_yel_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.yel_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_yel_limit_cell[pool]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_yel_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc =
      bf_tm_read_register(devid,
                          offsetof(Tofino,
                                   device_select.tm_top.tm_wac_top.wac_common
                                       .wac_common.wac_ap_yel_limit_cell[pool]),
                          &val);
  ig_spool->threshold.yel_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_yel_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->yel_hyst;
  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_ap_yel_offset_cell),
                            val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_yel_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_ap_yel_offset_cell),
                           &val);
  ig_spool->yel_hyst = val;
  ig_spool->yel_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_green_limit(
    bf_dev_id_t devid, uint8_t pool, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.green_limit;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_ap_limit_cell[pool]),
                            val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_green_limit(
    bf_dev_id_t devid, uint8_t pool, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_ap_limit_cell[pool]),
                           &val);
  ig_spool->threshold.green_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_green_hyst(bf_dev_id_t devid,
                                                    bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->green_hyst;

  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_ap_offset_cell),
                            val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_green_hyst(bf_dev_id_t devid,
                                                    bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_ap_offset_cell),
                           &val);
  ig_spool->green_hyst = val;
  ig_spool->green_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  int i;

  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.shared_pool_cnt; i++) {
    val |= (((g_tm_ctx[devid]->ig_pool->spool[i].color_drop_en) ? 1 : 0) << i);
  }
  val &= 0xf;  // make sure remaining mem-init bits are not set
  // Color-drop-enable is in global-config register along with
  // mem_init_en, reset_floor
  rc = bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_glb_config),
      val);
  (void)poolid;
  (void)ig_spool;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_glb_config),
      &val);

  color_drop_val = getp_wac_glb_config_color_drop_en(&val);
  ig_spool->color_drop_en = (bool)(color_drop_val & (1 << poolid));

  return (rc);
}

static uint32_t bf_tm_ig_spool_pfc_reg_addr(uint8_t pool, uint8_t pfc) {
  uint32_t addr = 0x0;

  switch (pool) {
    case BF_TM_IG_APP_POOL_0:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_0_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_1:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_1_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_2:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_2_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_3:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_3_limit_cell[pfc]);
      break;
  }
  return (addr);
}

bf_tm_status_t bf_tm_tofino_ig_spool_set_pfc_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   uint8_t pfc,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  uint32_t addr = bf_tm_ig_spool_pfc_reg_addr(pool, pfc);

  val = ig_spool->threshold.pfc_limit[pfc];

  /* Convert the value to HW unit - 8-cells */
  val = (val >> 3);
  rc = bf_tm_write_register(devid, addr, val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_pfc_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   uint8_t pfc,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t addr = bf_tm_ig_spool_pfc_reg_addr(pool, pfc);

  rc = bf_tm_read_register(devid, addr, &val);

  /* Convert the value from HW unit 8-cells to cells */
  val = (val << 3);
  ig_spool->threshold.pfc_limit[pfc] = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_set_skid_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_hdr_limit_cell),
                            ig_gpool->skid_limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_get_skid_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_hdr_limit_cell),
                           &val);
  ig_gpool->skid_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_set_skid_hyst(bf_dev_id_t devid,
                                                   bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_hdr_offset_cell),
                            (ig_gpool->skid_hyst >> 3));
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_get_skid_hyst(bf_dev_id_t devid,
                                                   bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_hdr_offset_cell),
                           &val);
  ig_gpool->skid_hyst = val;
  ig_gpool->skid_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_set_dod_limit(bf_dev_id_t devid,
                                                   bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_dod_limit_cell),
                            ig_gpool->dod_limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_get_dod_limit(bf_dev_id_t devid,
                                                   bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_dod_limit_cell),
                           &val);
  ig_gpool->dod_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_usage(bf_dev_id_t devid,
                                               uint8_t pool,
                                               uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_ap_cnt_cell[pool]),
                           count);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_wm(bf_dev_id_t devid,
                                            uint8_t pool,
                                            uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_wac_top.wac_common
                                        .wac_common.wac_wm_ap_cnt_cell[pool]),
                           count);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_clear_wm(bf_dev_id_t devid, uint8_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_wac_top.wac_common
                                         .wac_common.wac_wm_ap_cnt_cell[pool]),
                            0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_spool_get_color_drop_state(bf_dev_id_t devid,
                                                          bf_tm_color_t color,
                                                          uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common
                   .wac_common_block_drop_st.drop_state_cell[color]),
      &val);
  *state = val & 0xfull;
  return rc;
}

bf_tm_status_t bf_tm_tofino_ig_spool_clear_color_drop_state(
    bf_dev_id_t devid, bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_common
                   .wac_common_block_drop_st.drop_state_cell[color]),
      val);
  return rc;
}

/////////////////EG-POOLS////////////////////////////

bf_tm_status_t bf_tm_tofino_eg_buffer_drop_state(
    bf_dev_id_t devid,
    bf_tm_eg_buffer_drop_state_en drop_type,
    uint32_t *state) {
  if (drop_type > 7) {
    return BF_TM_EINV_ARG;
  }
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_common.qac_common_block_drop_st
              .qac_glb_ap_drop_state_cell[drop_type]),
      &val);
  *state = val & 0xfull;
  return rc;
}

bf_tm_status_t bf_tm_tofino_eg_buffer_drop_state_clear(
    bf_dev_id_t devid, bf_tm_eg_buffer_drop_state_en drop_type) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_common.qac_common_block_drop_st
              .qac_glb_ap_drop_state_cell[drop_type]),
      val);
  return rc;
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_red_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.red_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_limit_cell[pool]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_red_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_limit_cell[pool]),
      &val);
  eg_spool->threshold.red_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_red_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->red_hyst;
  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_resume_offset_cell),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_red_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_resume_offset_cell),
      &val);
  eg_spool->red_hyst = val;
  eg_spool->red_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_yel_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.yel_limit;
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_limit_cell[pool]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_yel_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_limit_cell[pool]),
      &val);
  eg_spool->threshold.yel_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_yel_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->yel_hyst;
  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_resume_offset_cell),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_yel_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_resume_offset_cell),
      &val);
  eg_spool->yel_hyst = val;
  eg_spool->yel_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_green_limit(
    bf_dev_id_t devid, uint8_t pool, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.green_limit;

  /*
   * Tofino specific validation for egress AP pool3 size -
   *   -  As PRE FIFO uses egress AP pool3 on POR and we don't change it
   *      (neither drivers default nor through exposed API), egress AP pool3
   *      size has to be 12K (0x3000) cells at least so MC would work fine
   *      even if there is burst.
   *
   *   -  Even if MC feature is not used, minimum egress AP pool3 size should be
   *      atleast 1K cells as flooded packets go through PRE.
   */
  if (pool == 3) {
    if (val < BF_TM_EG_APP_POOL_3_MC_MIN_SIZE) {
      LOG_ERROR(
          "%s: invalid size (%d) for egress AP pool3, minimum should be %d "
          "cells",
          __func__,
          val,
          BF_TM_EG_APP_POOL_3_MC_MIN_SIZE);
      return (BF_INVALID_ARG);
    }

    if (val < BF_TM_EG_APP_POOL_3_MC_SIZE) {
      LOG_WARN(
          "%s: setting egress pool3 size (%d) to less than %d cells would "
          "cause traffic loss for MC traffic burst as PRE FIFO uses AP pool3",
          __func__,
          val,
          BF_TM_EG_APP_POOL_3_MC_SIZE);
    }
  }

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_limit_cell[pool]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_green_limit(
    bf_dev_id_t devid, uint8_t pool, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_limit_cell[pool]),
      &val);
  eg_spool->threshold.green_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_green_hyst(bf_dev_id_t devid,
                                                    bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->green_hyst;

  val >>= 3;  // Unit of 8 cells
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_resume_offset_cell),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_green_hyst(bf_dev_id_t devid,
                                                    bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_resume_offset_cell),
      &val);
  eg_spool->green_hyst = val;
  eg_spool->green_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_set_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  int i;
  // To avoid read modify write fill in the power on reset values.
  // As of now we do not re-program these power on reset values.
  uint32_t discp_apid = 0x3;
  uint32_t discp_apid_en = 0x1;
  uint32_t pre_pri0_apid = 0x3;
  uint32_t pre_pri1_apid = 0x3;
  uint32_t pre_pri2_apid = 0x3;
  uint32_t pre_pri3_apid = 0x3;

  uint32_t color_drop_val = 0;
  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.shared_pool_cnt; i++) {
    color_drop_val |=
        ((((g_tm_ctx[devid]->eg_pool->spool + i)->color_drop_en) ? 1 : 0) << i);
  }

  setp_qac_glb_config_ap_color_drop_en(&val, color_drop_val);
  setp_qac_glb_config_pre_pri0_apid(&val, pre_pri0_apid);
  setp_qac_glb_config_pre_pri1_apid(&val, pre_pri1_apid);
  setp_qac_glb_config_pre_pri2_apid(&val, pre_pri2_apid);
  setp_qac_glb_config_pre_pri3_apid(&val, pre_pri3_apid);
  setp_qac_glb_config_discd_apid_en(&val, discp_apid_en);
  setp_qac_glb_config_discd_apid(&val, discp_apid);

  rc = bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_glb_config),
      val);
  (void)poolid;
  (void)eg_spool;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_glb_config),
      &val);

  color_drop_val = getp_qac_glb_config_ap_color_drop_en(&val);
  eg_spool->color_drop_en = (bool)(color_drop_val & (1 << poolid));

  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_gpool_set_dod_limit(bf_dev_id_t devid,
                                                   bf_tm_eg_gpool_t *eg_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_qac_top.qac_common
                                         .qac_common.qac_dod_limit_cell),
                            eg_gpool->dod_limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_gpool_get_dod_limit(bf_dev_id_t devid,
                                                   bf_tm_eg_gpool_t *eg_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_qac_top.qac_common
                                        .qac_common.qac_dod_limit_cell),
                           &val);
  eg_gpool->dod_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_gpool_set_fifo_limit(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint8_t fifo,
                                                    bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  switch (pipe) {
    case 0:
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe0[fifo]),
          limit);
      break;
    case 1:
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe1[fifo]),
          limit);
      break;
    case 2:
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe2[fifo]),
          limit);
      break;
    case 3:
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe3[fifo]),
          limit);
      break;
    default:
      rc = BF_TM_EINV_ARG;
      break;
  }
  if (rc != BF_SUCCESS) {
    LOG_ERROR("fifo=%d, dev=%d, pipe=%d: Unable to set PRE FIFO limit=%d",
              fifo,
              devid,
              pipe,
              limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_gpool_get_fifo_limit(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint8_t fifo,
                                                    bf_tm_thres_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  switch (pipe) {
    case 0:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe0[fifo]),
          limit);
      break;
    case 1:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe1[fifo]),
          limit);
      break;
    case 2:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe2[fifo]),
          limit);
      break;
    case 3:
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe3[fifo]),
          limit);
      break;
    default:
      rc = BF_TM_EINV_ARG;
      break;
  }
  if (rc != BF_SUCCESS) {
    LOG_ERROR("fifo=%d, dev=%d, pipe=%d: Unable to get PRE FIFO limit",
              fifo,
              devid,
              pipe);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_usage(bf_dev_id_t devid,
                                               uint8_t pool,
                                               uint32_t *count)

{
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_qac_top.qac_common
                                        .qac_common.qac_glb_ap_cnt_cell[pool]),
                           count);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_get_wm(bf_dev_id_t devid,
                                            uint8_t pool,
                                            uint32_t *count)

{
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_wm_glb_ap_cnt_cell[pool]),
      count);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_eg_spool_clear_wm(bf_dev_id_t devid, uint8_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_wm_glb_ap_cnt_cell[pool]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_set_uc_ct_size(bf_dev_id_t devid,
                                                    uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  setp_qclc_ct_tot_uc_th(&val, cells);
  setp_qclc_ct_tot_mc_th(&val, g_tm_ctx[devid]->mc_ct_size);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_set_mc_ct_size(bf_dev_id_t devid,
                                                    uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t repli_fifo_lmt;

  // setting multicast CT pool size to non zero enables CT for MC
  // packets that are injected into TM by pipe with ct_enable
  // bit set in intrinsic metadata.

  setp_qac_mcct_pkt_limit_total_pkt_lmt(&val, 0x400);  // 0x400 is power
                                                       // on default value
  if (cells) {
    /* When MC CT is enabled, set replication FIFO to 0x48 (jira TOFLAB-36)
     * for rev A0. For B0 and later parts, set it to 0x200.
     */
    if (g_tm_ctx[devid]->part_rev == BF_SKU_CHIP_PART_REV_A0) {
      repli_fifo_lmt = 0x48;
    } else {
      repli_fifo_lmt = 0x200;
    }
    setp_qac_mcct_pkt_limit_repli_fifo_lmt(&val, repli_fifo_lmt);
  } else {
    setp_qac_mcct_pkt_limit_repli_fifo_lmt(&val, 0x4);  // Power on default
                                                        // value.
  }
  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_qac_top.qac_common
                                         .qac_common.qac_mcct_pkt_limit),
                            val);

  val = 0;
  setp_qclc_ct_tot_mc_th(&val, cells);
  setp_qclc_ct_tot_uc_th(&val, g_tm_ctx[devid]->uc_ct_size);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_get_uc_ct_size(bf_dev_id_t devid,
                                                    uint32_t *cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      &val);

  *cells = getp_qclc_ct_tot_uc_th(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_ig_gpool_get_mc_ct_size(bf_dev_id_t devid,
                                                    uint32_t *cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.tot_th),
      &val);
  *cells = getp_qclc_ct_tot_mc_th(&val);
  return (rc);
}

/////////////////SCH////////////////////////////

static void bf_tm_tofino_sch_prep_q_cfg_reg(uint32_t *val, bf_tm_eg_q_t *q) {
  setp_sch_queue_config_r_enb(val, q->q_sch_cfg.sch_enabled);
  /* Per HW team, pfc_upd_enb bit should be set always */
  setp_sch_queue_config_r_pfc_upd_enb(val, 1);
  setp_sch_queue_config_r_min_rate_enb(val, q->q_sch_cfg.min_rate_enable);
  setp_sch_queue_config_r_max_rate_enb(val, q->q_sch_cfg.max_rate_enable);
  setp_sch_queue_config_r_cid(val, q->q_sch_cfg.cid);
  setp_sch_queue_config_r_pfc_pri(val, q->q_sch_cfg.pfc_prio);
  setp_sch_queue_config_r_min_rate_pri(val, q->q_sch_cfg.min_rate_sch_prio);
  setp_sch_queue_config_r_max_rate_pri(val, q->q_sch_cfg.max_rate_sch_prio);
}

static void bf_tm_tofino_sch_fill_q_struct(uint32_t *val, bf_tm_eg_q_t *q) {
  q->q_sch_cfg.sch_enabled = getp_sch_queue_config_r_enb(val);
  q->q_sch_cfg.sch_pfc_enabled = getp_sch_queue_config_r_pfc_upd_enb(val);
  q->q_sch_cfg.cid = getp_sch_queue_config_r_cid(val);
  q->q_sch_cfg.pfc_prio = getp_sch_queue_config_r_pfc_pri(val);
  q->q_sch_cfg.min_rate_sch_prio = getp_sch_queue_config_r_min_rate_pri(val);
  q->q_sch_cfg.max_rate_sch_prio = getp_sch_queue_config_r_max_rate_pri(val);
  q->q_sch_cfg.min_rate_enable = getp_sch_queue_config_r_min_rate_enb(val);
  q->q_sch_cfg.max_rate_enable = getp_sch_queue_config_r_max_rate_enb(val);
}

bf_status_t bf_tm_tofino_sch_set_q_priority(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_tofino_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      val);
  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_priority(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tofino_sch_fill_q_struct(&val, q);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_q_wt(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  setp_sch_excessbucket_static_r_wt(&val, q->q_sch_cfg.dwrr_wt);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_exc_static_mem[q->physical_q]),
      val);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_wt(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_exc_static_mem[q->physical_q]),
      &val);
  val = getp_sch_excessbucket_static_r_wt(&val);

  q->q_sch_cfg.dwrr_wt = val;

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_q_pfc_prio(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_tofino_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      val);

  /* Update egress port PFC status for the new PFC priority */
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .port_pfc_status_mem[q->port]),
      (1 << q->q_sch_cfg.pfc_prio));

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_pfc_prio(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tofino_sch_fill_q_struct(&val, q);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_sch_get_q_egress_pfc_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q,
                                                        bool *status) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                                   .q_pfc_status_mem[q->physical_q]),
                      &val);

  *status = (bool)(val & 0x1u);

  return rc;
}

bf_tm_status_t bf_tm_tofino_sch_set_q_egress_pfc_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q,
                                                        bool status) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_pfc_status_mem[q->physical_q]),
      (uint32_t)status);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to set queue_egress_pfc_status for dev %d p_pipe %d and "
        "physical_q %d",
        devid,
        q->p_pipe,
        q->physical_q);
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_sch_clear_q_egress_pfc_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_pfc_status_mem[q->physical_q]),
      val);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to clear queue_egress_pfc_status for dev %d p_pipe %d and "
        "physical_q %d",
        devid,
        q->p_pipe,
        q->physical_q);
  }
  return rc;
}

static void bf_tm_burst_size_to_mant_exp_form(uint32_t burst_size,
                                              uint32_t *mant,
                                              uint32_t *exp) {
  // 4b Mant
  // 4b exp
  // B = Mant << Exp
  // Bmax = 15 << 15

  int i = 0;

  *mant = 0;
  *exp = 0;
  if (!burst_size) {
    return;
  }

  if (burst_size > (15 << 15)) burst_size = (15 << 15);

  *mant = 0;
  i = 15;
  while (!(*mant) && i > 4) {
    if (burst_size & (0xf << i)) {
      *mant = (burst_size >> (i - 3));
      *exp = i - 3;
      break;
    } else {
      i--;
    }
  }
  if (i <= 4) {
    *mant = burst_size;
    *exp = 0;
  }
  // Note : We under provision burst size. Example if user requested
  // burst size of 18000bytes, we end up setting burst size as 16384bytes
  // mantissa = 4, exponent = 12)
  // Find what should the leaky bucket burst-size should be.
  // For now do not change power on default value.
  //*mant = 0xf;
  //*exp = 7;
}

static void bf_tm_burst_size_mant_exp_form_to_bs(uint32_t mant,
                                                 uint32_t exp,
                                                 uint32_t *rate) {
  // 4b Mant
  // 4b exp
  // B = Mant << Exp
  // Bmax = 15 << 15
  *rate = mant << exp;
}

/*
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (10b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_mant_exp_form_to_rate(
    bf_dev_id_t dev_id, uint32_t mant, uint32_t exp, uint64_t *rate, bool pps) {
  // 10b Mant
  // 4b exp

  float clockspeed = g_tm_ctx[dev_id]->clock_speed;

  *rate = mant;
  if (!pps) {
    *rate *= 8;  //(convert into bps)
  }
  *rate *= clockspeed;
  *rate = *rate / 80;  // into HW rate programmed was for every 80clocks
  if (exp) {
    // If pps, multiply the exponent by 2
    if (pps) {
      exp = (exp << 1);
    }
    *rate = *rate / (1 << exp);
  }
}

/*
 * Converts rate to bytes (if input is in bps) or
 * to packets (if input is in pps) per 80 clocks as below
 *
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (10b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_to_mant_exp_form(
    bf_dev_id_t dev_id,
    uint64_t rate,
    uint32_t *mant,
    uint32_t *exp,
    bool pps,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  // 10b Mant
  // 4b exp

  float rate_per_80_clock_period, intg_part;
  uint64_t rate_for_80_clocks = rate;
  uint32_t max_exp;
  bool decimal_part = true;

  float clockspeed = g_tm_ctx[dev_id]->clock_speed;

  *mant = 0;
  *exp = 0;
  if (!rate) {
    return;
  }

  // Convert rate for 80 clocks. If bps, convert it into bytes
  rate_for_80_clocks = (rate * 80);
  if (!pps) {
    rate_for_80_clocks /= 8;
  }

  rate_per_80_clock_period = rate_for_80_clocks / clockspeed;
  if (rate_per_80_clock_period > 1023) {
    // Invalid... can't represent this. It also means that on port/queue we are
    // setting more than 100Gbps
    return;
  }

  max_exp = 15;
  if (pps) {
    max_exp *= 2;  // Before programming, exp will be divided by for pps
  }

  /* Find first valid mantissa */
  while ((rate_per_80_clock_period < 1) && (*exp <= max_exp)) {
    rate_per_80_clock_period *= 2;
    *exp += 1;
  }

  /* Too small to convert, return 0 */
  if (*exp == (max_exp + 1)) {
    *mant = 0;
    *exp = 0;
    return;
  }

  /*
   * For floating point, higher the exponent, higher the accuracy.
   * Try mantissa values for all possible exponents.
   */
  *mant = rate_per_80_clock_period;
  if (!modff(rate_per_80_clock_period, &intg_part)) {
    decimal_part = false;
  }
  while (rate_per_80_clock_period < 512 && decimal_part == true &&
         *exp < max_exp) {
    rate_per_80_clock_period *= 2;
    *mant = rate_per_80_clock_period;
    *exp += 1;

    if (!modff(rate_per_80_clock_period, &intg_part)) {
      decimal_part = false;
    }
  }

  // For pps, adjust mantissa if needed and divide the exponent by 2
  if (pps) {
    if ((*exp) % 2) {
      *mant = (*mant >> 1);
    }
    *exp = (*exp >> 1);
  }

  if (decimal_part && *mant < 1023) {
    if (prov_type == BF_TM_SCH_RATE_UPPER) {
      *mant += 1;  // Instead of losing fractional rate and under provisioning,
                   // set rate to next higher representable value;
    } else if (prov_type == BF_TM_SCH_RATE_MIN_ERROR) {
      // Calculate the rate that has minimum deviation from the specied rate
      uint64_t high_rate = 0, low_rate = 0, delta_high = 0, delta_low = 0;
      bf_tm_rate_mant_exp_form_to_rate(
          dev_id, *mant + 1, *exp, &high_rate, pps);
      bf_tm_rate_mant_exp_form_to_rate(dev_id, *mant, *exp, &low_rate, pps);

      if (high_rate >= rate) {
        delta_high = high_rate - rate;
      } else {
        delta_high = rate - high_rate;
      }

      if (low_rate >= rate) {
        delta_low = low_rate - rate;
      } else {
        delta_low = rate - low_rate;
      }

      if (delta_high < delta_low) {
        *mant += 1;
      }
    }
  }
}

bool bf_tm_tofino_sch_verify_burst_size(bf_dev_id_t devid,
                                        uint32_t restore_burst_size,
                                        uint32_t orig_burst_size) {
  uint32_t restore_mant, orig_mant;
  uint32_t restore_exp, orig_exp;
  (void)devid;  // For future use

  bf_tm_burst_size_to_mant_exp_form(
      restore_burst_size, &restore_mant, &restore_exp);
  bf_tm_burst_size_to_mant_exp_form(orig_burst_size, &orig_mant, &orig_exp);

  if (restore_mant == orig_mant && restore_exp == orig_exp) {
    return true;
  } else {
    return false;
  }
}

/*
 * Converts rate to bytes (if input is in bps) or
 * to packets (if input is in pps) per 80 clocks as below
 * Makes sure (mantissa <= bucket_size) for optimal rate
 *
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (10b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_to_mant_exp_form_adv(
    bf_dev_id_t dev_id,
    uint64_t rate,
    uint32_t bs,
    uint32_t *mant,
    uint32_t *exp,
    bool pps,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  // 10b Mant
  // 4b exp
  uint64_t new_rate = 0, old_rate = 0;
  bool bnew_rate_found = false;

  bf_tm_rate_to_mant_exp_form(dev_id, rate, mant, exp, pps, prov_type);

  if ((bs == 0) || (*exp == 0) || (*mant == 0)) {
    /* Nothing much to do, return */
    return;
  }

  while ((bs < *mant) && (*exp != 0)) {
    // For optimum rate we should have Bucket Size more than Rate Mantessa
    *mant = (*mant >> (pps ? 2 : 1));
    *exp = (*exp - 1);
  }

  new_rate = 0;
  old_rate = 0;
  bf_tm_rate_mant_exp_form_to_rate(dev_id, *mant, *exp, &new_rate, pps);

  while ((bs > *mant) && (new_rate < rate) && (*mant < 1023)) {
    old_rate = new_rate;
    *mant = *mant + 1;
    new_rate = 0;
    bf_tm_rate_mant_exp_form_to_rate(dev_id, *mant, *exp, &new_rate, pps);
    if ((new_rate >= rate)) {
      bnew_rate_found = true;
      break;
    }
  }

  if (bnew_rate_found) {
    // Over Shot lets pick the closest
    if ((rate - old_rate) < (new_rate - rate)) {
      *mant = *mant - 1;
    }
  }
}

bool bf_tm_tofino_sch_verify_rate(bf_dev_id_t devid,
                                  uint32_t restore_rate,
                                  uint32_t orig_rate,
                                  bool pps) {
  uint32_t restore_mant, orig_mant;
  uint32_t restore_exp, orig_exp;
  (void)devid;  // For future use

  bf_tm_rate_to_mant_exp_form(devid,
                              restore_rate,
                              &restore_mant,
                              &restore_exp,
                              pps,
                              BF_TM_SCH_RATE_UPPER);
  bf_tm_rate_to_mant_exp_form(
      devid, orig_rate, &orig_mant, &orig_exp, pps, BF_TM_SCH_RATE_UPPER);

  if (restore_mant == orig_mant && restore_exp == orig_exp) {
    return true;
  } else {
    return false;
  }
}

bf_status_t bf_tm_tofino_sch_set_q_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, mant, exp;
  uint64_t rate = 0, new_rate = 0;
  uint32_t bs = 0;

  // convert q->burst_size into mant, exp form
  // q bs is specified in pps or kbps.. covert to bps if in kbps
  setp_sch_leakybucket_static_r_pps(&val, q->q_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(q->q_sch_cfg.max_burst_size, &mant, &exp);
  setp_sch_leakybucket_static_r_bs_exp(&val, exp);
  setp_sch_leakybucket_static_r_bs_mant(&val, mant);

  // convert q->rate into mant, exp form.
  // q rate is specified in pps or kbps.. covert to bps in case of kbps
  rate = (uint64_t)(q->q_sch_cfg.max_rate);
  if (!q->q_sch_cfg.pps) {
    rate *= 1000;
  }

  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(devid,
                                  rate,
                                  bs,
                                  &mant,
                                  &exp,
                                  q->q_sch_cfg.pps,
                                  q->q_sch_cfg.sch_prov_type);
  bf_tm_rate_mant_exp_form_to_rate(
      devid, mant, exp, &new_rate, q->q_sch_cfg.pps);
  LOG_DBG(
      "Q_SHAPER MAX ADV dev %d p_pipe %d and p_queue %d PPS %d ORATE %" PRIu64
      " NRATE %" PRIu64
      " "
      "BS %d MANT %d EXP %d",
      devid,
      q->p_pipe,
      q->physical_q,
      q->q_sch_cfg.pps,
      rate,
      new_rate,
      bs,
      mant,
      exp);

  setp_sch_leakybucket_static_r_rate_exp(&val, exp);
  setp_sch_leakybucket_static_r_rate_mant(&val, mant);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_max_lb_static_mem[q->physical_q]),
      val);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate;
  uint32_t bs, mant, exp, val;
  bool pps;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_max_lb_static_mem[q->physical_q]),
      &val);

  pps = getp_sch_leakybucket_static_r_pps(&val);
  q->q_sch_cfg.pps = pps;

  exp = getp_sch_leakybucket_static_r_bs_exp(&val);
  mant = getp_sch_leakybucket_static_r_bs_mant(&val);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);

  exp = getp_sch_leakybucket_static_r_rate_exp(&val);
  mant = getp_sch_leakybucket_static_r_rate_mant(&val);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  q->q_sch_cfg.max_rate = (uint32_t)rate;
  if (!pps) {
    q->q_sch_cfg.max_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  q->q_sch_cfg.max_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_q_min_rate(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, mant, exp;
  uint64_t rate = 0, new_rate = 0;
  uint32_t bs = 0;

  // convert q->min_rate into mant, exp form.
  // convert q->burst_size into mant, exp form
  setp_sch_leakybucket_static_r_pps(&val, q->q_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(q->q_sch_cfg.min_burst_size, &mant, &exp);
  setp_sch_leakybucket_static_r_bs_exp(&val, exp);
  setp_sch_leakybucket_static_r_bs_mant(&val, mant);

  // q rate is specified in kbps or pps.. covert to bps in case of kbps
  rate = (uint64_t)(q->q_sch_cfg.min_rate);
  if (!q->q_sch_cfg.pps) {
    rate *= 1000;
  }

  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, q->q_sch_cfg.pps, BF_TM_SCH_RATE_UPPER);
  bf_tm_rate_mant_exp_form_to_rate(
      devid, mant, exp, &new_rate, q->q_sch_cfg.pps);
  LOG_DBG(
      "Q_SHAPER MIN ADV dev %d p_pipe %d and p_queue %d PPS %d ORATE %" PRIu64
      " NRATE %" PRIu64
      " "
      "BS %d MANT %d EXP %d",
      devid,
      q->p_pipe,
      q->physical_q,
      q->q_sch_cfg.pps,
      rate,
      new_rate,
      bs,
      mant,
      exp);

  setp_sch_leakybucket_static_r_rate_exp(&val, exp);
  setp_sch_leakybucket_static_r_rate_mant(&val, mant);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_min_lb_static_mem[q->physical_q]),
      val);
  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_min_rate(bf_dev_id_t devid,
                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate;
  uint32_t bs, mant, exp, val;
  bool pps;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .q_min_lb_static_mem[q->physical_q]),
      &val);

  pps = getp_sch_leakybucket_static_r_pps(&val);

  exp = getp_sch_leakybucket_static_r_bs_exp(&val);
  mant = getp_sch_leakybucket_static_r_bs_mant(&val);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  exp = getp_sch_leakybucket_static_r_rate_exp(&val);
  mant = getp_sch_leakybucket_static_r_rate_mant(&val);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  q->q_sch_cfg.min_rate = (uint32_t)rate;
  if (!pps) {
    q->q_sch_cfg.min_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  q->q_sch_cfg.min_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_q_max_rate_enable_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q) {
  // Because Q_sched and Q max rate enable status are shared in same register,
  // invoke q_sched function to set max-rate-enable status
  return (bf_tm_tofino_sch_set_q_sched(devid, q));
}

bf_status_t bf_tm_tofino_sch_set_q_min_rate_enable_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q) {
  // Because Q_sched and Q min rate enable status are shared in same register,
  // invoke q_sched function to set min-rate-enable status
  return (bf_tm_tofino_sch_set_q_sched(devid, q));
}

bf_status_t bf_tm_tofino_sch_get_q_max_rate_enable_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q) {
  // Because Q_sched and Q max rate enable status are shared in same register,
  // invoke q_sched function to get max-rate-enable status
  return (bf_tm_tofino_sch_get_q_sched(devid, q));
}

bf_status_t bf_tm_tofino_sch_get_q_min_rate_enable_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q) {
  // Because Q_sched and Q min rate enable status are shared in same register,
  // invoke q_sched function to get min-rate-enable status
  return (bf_tm_tofino_sch_get_q_sched(devid, q));
}

bf_status_t bf_tm_tofino_sch_set_q_sched(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_tofino_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      val);
  if (q->q_sch_cfg.sch_enabled) {
    // queue is scheduling enabled. After enabling queue, it
    // is required to clear q_pfc_status. The reason being
    // when queue is disabled from sch, TM logic self asserts
    // pfc on the queue.
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                     .q_pfc_status_mem[q->physical_q]),
        0);
  }
  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_q_sched(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[q->p_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tofino_sch_fill_q_struct(&val, q);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_port_rate(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint32_t val = 0;
  uint64_t rate = 0, new_rate = 0;
  uint32_t bs = 0;

  setp_sch_leakybucket_static_r_pps(&val, p->pps ? 1 : 0);

  bf_tm_burst_size_to_mant_exp_form(p->burst_size, &mant, &exp);
  setp_sch_leakybucket_static_r_bs_exp(&val, exp);
  setp_sch_leakybucket_static_r_bs_mant(&val, mant);

  // port rate is specified in kbps or pps.. covert to bps in cas of kbps
  rate = (uint64_t)(p->port_rate);
  if (!p->pps) {
    rate = (uint64_t)(p->port_rate) * 1000;
  }

  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, p->pps, p->sch_prov_type);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &new_rate, p->pps);
  LOG_DBG(
      "PORT_SHAPER MAX ADV dev %d p_pipe %d and p_l1 %d PPS %d ORATE %" PRIu64
      " NRATE %" PRIu64
      " "
      "BS %d MANT %d EXP %d",
      devid,
      p->p_pipe,
      p->port,
      p->pps,
      rate,
      new_rate,
      bs,
      mant,
      exp);

  setp_sch_leakybucket_static_r_rate_exp(&val, exp);
  setp_sch_leakybucket_static_r_rate_mant(&val, mant);

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[p->p_pipe]
                   .port_max_lb_static_mem[p->port]),
      val);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_port_rate(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate;
  uint32_t bs, mant, exp, val;
  bool pps;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[p->p_pipe]
                   .port_max_lb_static_mem[p->port]),
      &val);

  pps = getp_sch_leakybucket_static_r_pps(&val);
  p->pps = pps;

  exp = getp_sch_leakybucket_static_r_bs_exp(&val);
  mant = getp_sch_leakybucket_static_r_bs_mant(&val);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  exp = getp_sch_leakybucket_static_r_rate_exp(&val);
  mant = getp_sch_leakybucket_static_r_rate_mant(&val);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  p->port_rate = (uint32_t)rate;
  if (!pps) {
    p->port_rate = (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  p->burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_port_sched(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  switch (p->speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      p->credit = 0;
      break;
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      p->credit = 1;
      break;
    case BF_SPEED_100G:
      p->credit = 2;
      break;
  }

  setp_sch_port_config_r_enb(&val, p->sch_enabled ? 1 : 0);
  /* Per HW team, pfc_upd_enb bit should be set always */
  setp_sch_port_config_r_pfc_upd_enb(&val, 1);
  setp_sch_port_config_r_tdm_enb(&val, p->tdm);
  setp_sch_port_config_r_max_rate_enb(&val, p->max_rate_enabled ? 1 : 0);
  setp_sch_port_config_r_maxcr(&val, p->credit);

  rc = bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_sch_top.sch[p->p_pipe].port_config[p->port]),
      val);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_port_sched(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_sch_top.sch[p->p_pipe].port_config[p->port]),
      &val);
  p->sch_enabled =
      (getp_sch_port_config_r_enb(&val)) ? true : false;  // [0] bit
  p->max_rate_enabled =
      (getp_sch_port_config_r_max_rate_enb(&val)) ? true : false;  // [3] bit
  p->tdm = getp_sch_port_config_r_tdm_enb(&val);                   // [2] bit
  p->credit = getp_sch_port_config_r_maxcr(&val);                  // [9:8] bit
  p->fc_rx_type = getp_sch_port_config_r_pfc_upd_enb(&val);        // [1] bit

  return (rc);
}

bf_status_t bf_tm_tofino_sch_force_disable_port_sched(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  // Use direct PCIe write instead of using write lists
  rc = lld_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_sch_top.sch[p->p_pipe].port_config[p->port]),
      val);

  LOG_DBG("%s: TM force port schedule disable for pipe %d, port %d, rc %d",
          __func__,
          p->p_pipe,
          p->port,
          rc);
  return (rc);
}

bf_status_t bf_tm_tofino_sch_set_port_max_rate_enable_status(bf_dev_id_t devid,
                                                             bf_tm_port_t *p) {
  /*
   * Since port scheduling enable/disable and port shaping (max rate)
   * enable/disable are shared in same register (port_config) in TM, invoke
   * bf_tm_tofino_sch_set_port_sched() to enable/disable port shaping.
   */
  return (bf_tm_tofino_sch_set_port_sched(devid, p));
}

bf_status_t bf_tm_tofino_sch_get_port_max_rate_enable_status(bf_dev_id_t devid,
                                                             bf_tm_port_t *p) {
  /*
   * Since port scheduling enable/disable and port shaping (max rate)
   * enable/disable are shared in same register (port_config) in TM, invoke
   * bf_tm_tofino_sch_get_port_sched() to get port shaping enable status.
   */
  return (bf_tm_tofino_sch_get_port_sched(devid, p));
}

bf_status_t bf_tm_tofino_sch_set_pkt_ifg(bf_dev_id_t devid, void *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pipe_t *eg_pipe = (bf_tm_eg_pipe_t *)s;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[eg_pipe->p_pipe]
                   .global_bytecnt_adj),
      eg_pipe->ifg_compensation);

  return (rc);
}

bf_status_t bf_tm_tofino_sch_get_pkt_ifg(bf_dev_id_t devid, void *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pipe_t *eg_pipe = (bf_tm_eg_pipe_t *)s;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_sch_top.sch[eg_pipe->p_pipe]
                   .global_bytecnt_adj),
      &val);
  eg_pipe->ifg_compensation = val;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_sch_get_port_egress_pfc_status(bf_dev_id_t devid,
                                                           bf_tm_port_t *port,
                                                           uint8_t *status) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_sch_top.sch[port->p_pipe]
                                   .port_pfc_status_mem[port->port]),
                      &val);
  *status = (uint8_t)(val);
  return rc;
}

/////////////////PORTS////////////////////////////

//  ingress PORT related HW programming.
//  List of tables/regiser/memories programmed
//      csr_mem_wac_port_ppg_mapping

bf_tm_status_t bf_tm_tofino_set_egress_pipe_max_limit(bf_dev_id_t devid,
                                                      bf_dev_pipe_t pipe,
                                                      uint32_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_qac_top.qac_common
                                         .qac_common.qac_ep_limit_cell[pipe]),
                            limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_get_egress_pipe_max_limit(bf_dev_id_t devid,
                                                      bf_dev_pipe_t pipe,
                                                      uint32_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_qac_top.qac_common
                                        .qac_common.qac_ep_limit_cell[pipe]),
                           limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_set_egress_pipe_hyst(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint32_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(devid,
                            offsetof(Tofino,
                                     device_select.tm_top.tm_qac_top.qac_common
                                         .qac_common.qac_ep_resume_offset_cell),
                            limit);
  (void)pipe;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_set_timestamp_shift(bf_dev_id_t devid,
                                                uint8_t shift) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_psc_top.psc_common.timestamp_shift),
      shift);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_get_timestamp_shift(bf_dev_id_t devid,
                                                uint8_t *shift) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_psc_top.psc_common.timestamp_shift),
      &val);
  *shift = (uint8_t)val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_get_egress_pipe_hyst(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint32_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_read_register(devid,
                           offsetof(Tofino,
                                    device_select.tm_top.tm_qac_top.qac_common
                                        .qac_common.qac_ep_resume_offset_cell),
                           limit);
  (void)pipe;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_wac_drop_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[port->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  setp_wac_port_max_lmt_max_lmt_lmt(&val, port->wac_drop_limit);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[port->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[port->port]),
      val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  if (TM_CLEAR_LOW_3_BITS(port->wac_drop_limit)) {
    /* If port-drop limit was previously set to zero, it is required to
     * clear port-drop-state after setting drop-limit; otherwise  WAC will
     * continue to drop traffic.
     */
    rc = bf_tm_tofino_clear_wac_drop_state(devid, port->p_pipe);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_qac_drop_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[port->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[port->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  setp_qac_port_config_qac_port_config_port_thrd(&val, port->qac_drop_limit);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[port->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[port->port]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_wac_drop_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t limit;
  uint32_t wac_limit = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[p->port]),
      &limit);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  wac_limit = getp_wac_port_max_lmt_max_lmt_lmt(&limit);
  p->wac_drop_limit = wac_limit;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_qac_drop_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t limit;
  uint32_t qac_limit = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[p->port]),
      &limit);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  qac_limit = getp_qac_port_config_qac_port_config_port_thrd(&limit);
  p->qac_drop_limit = qac_limit;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_qac_drop_limit(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t limit = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[p->port]),
      limit);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_wac_hyst_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t hyst_index = 0;
  uint32_t val = 0;

  // Based on resume limit, find offset profile index in WAC
  rc = bf_tm_tofino_populate_wac_offset_profile(
      devid, p->p_pipe, p->wac_resume_limit, &hyst_index);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  // The port's WAC limit value is either registered in SW
  // or it was just added and written to HW.
  p->wac_hyst_index = hyst_index;

  // Update the Port's WAC offset limit index in HW
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[p->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  setp_wac_port_max_lmt_max_lmt_offset_idx(&val, p->wac_hyst_index);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[p->port]),
      val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_qac_hyst_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t hyst_index = 0;
  uint32_t val = 0;

  // Based on resume limit, find offset profile index in QAC
  rc = bf_tm_tofino_populate_qac_offset_profile(
      devid, p->p_pipe, p->qac_resume_limit, &hyst_index);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  // The port's QAC limit value is either registered in SW
  // or it was just added and written to HW.
  p->qac_hyst_index = hyst_index;

  // Update the Port's QAC offset limit index in HW
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[p->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  setp_qac_port_config_qac_port_config_offset_idx(&val, p->qac_hyst_index);
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[p->port]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_qac_hyst_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cells_u = 0;
  uint8_t hyst_index = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_config.qac_port_config[p->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  hyst_index = getp_qac_port_config_qac_port_config_offset_idx(&val);
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.offset_profile[hyst_index]),
      &cells_u);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  // Check (hyst_index, cells_u) tuple is consistent with SW profiles.
  // Restore SW hysteresis profiles from HW if not done yet.
  rc = bf_tm_tofino_check_qac_offset_profile(
      devid, p->p_pipe, cells_u, hyst_index);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  p->qac_hyst_index = hyst_index;
  p->qac_resume_limit = (TM_8CELL_UNITS_TO_CELLS(cells_u));

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_wac_hyst_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cells_u = 0;
  uint8_t hyst_index = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_max_lmt.max_lmt[p->port]),
      &val);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  hyst_index = getp_wac_port_max_lmt_max_lmt_offset_idx(&val);
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.offset_profile[hyst_index]),
      &cells_u);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  // Check (hyst_index, cells_u) tuple is consistent with SW profiles.
  // Restore SW hysteresis profiles from HW if not done yet.
  rc = bf_tm_tofino_check_wac_offset_profile(
      devid, p->p_pipe, cells_u, hyst_index);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  p->wac_hyst_index = hyst_index;
  p->wac_resume_limit = (TM_8CELL_UNITS_TO_CELLS(cells_u));
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_uc_ct_limit(bf_dev_id_t devid,
                                                 bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_port_t *pt = NULL;
  uint32_t val = 0, i, bport, bpipe, index;

  // Pointer to function that updates correct fields of register.
  void (*fptr)(uint32_t *, uint32_t) = NULL;

  // Inorder to avoid read-modify write program all 8 ports
  // that share single register.
  // Read-modify-write is avoided so that during
  // fast reconfig, there are no reads and only writes.
  bf_dev_port_t devport = MAKE_DEV_PORT(p->l_pipe, p->port - (p->port % 8));
  rc = bf_tm_port_get_descriptor(devid, devport, &pt);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  bport = pt->port;
  bpipe = pt->p_pipe;

  if (p->uc_cut_through_limit > 0xf) {
    // Tofino allows maximum of 0xf cells for CT
    p->uc_cut_through_limit = 0xf;
  }

  for (i = 0; i < 8; i++) {
    switch (pt->port % 8) {
      case 0:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th0;
        break;
      case 1:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th1;
        break;
      case 2:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th2;
        break;
      case 3:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th3;
        break;
      case 4:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th4;
        break;
      case 5:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th5;
        break;
      case 6:
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th6;
        break;
      default:  // Must be 7...
        fptr = setp_qclc_ct_uc_pt_uc_ct_pt_th7;
        break;
    }
    // Set Cut-through limit.
    fptr(&val, pt->uc_cut_through_limit);
    pt++;
  }

  index = ((bpipe * 72) + bport) / 8;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.pt_th[index]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_uc_ct_limit(bf_dev_id_t devid,
                                                 bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, bport, bpipe, index;
  bf_tm_port_t *pt = NULL;
  uint32_t (*fptr)(uint32_t *) = NULL;

  bf_dev_port_t devport = MAKE_DEV_PORT(p->l_pipe, p->port - (p->port % 8));
  rc = bf_tm_port_get_descriptor(devid, devport, &pt);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  bport = pt->port;
  bpipe = pt->p_pipe;
  index = ((bpipe * 72) + bport) / 8;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.pt_th[index]),
      &val);
  // Get fptr based on passed in port
  switch (p->port % 8) {
    case 0:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th0;
      break;
    case 1:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th1;
      break;
    case 2:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th2;
      break;
    case 3:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th3;
      break;
    case 4:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th4;
      break;
    case 5:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th5;
      break;
    case 6:
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th6;
      break;
    default:  // Must be 7...
      fptr = getp_qclc_ct_uc_pt_uc_ct_pt_th7;
      break;
  }

  // Set value for passed in port
  p->uc_cut_through_limit = fptr(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_flowcontrol_mode(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t icosmask, i, icos_to_cos_mask = 0;

  if (p->fc_type == BF_TM_PAUSE_NONE) {
    // No PFC or Port level pause.
    // clear pfc / port pause
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pfc_en[p->port]),
        0);
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pause_en[p->port]),
        0);
  }
  if (p->fc_type == BF_TM_PAUSE_PFC) {
    /*
     * Collect icos mask from all the non-default lossless PPGs (PFC enabled
     * PPGs) that are mapped to this port.
     * Default PPG is not considered. (PFC is not enabled on default PPG).
     */
    icosmask = 0;
    for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
      if (p->ppgs[i] && p->ppgs[i]->ppg_cfg.is_pfc) {
        icosmask |= p->ppgs[i]->ppg_cfg.icos_mask;
      }
    }
    for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
      if (p->cos_to_icos[i] >= BF_TM_MAX_PFC_LEVELS) {
        // Invalid mapping.
        continue;
      }
      if ((1 << p->cos_to_icos[i]) & icosmask) {
        icos_to_cos_mask |= (1 << i);
      }
    }

    /* Before enabling PFC, disable port pause first */
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pause_en[p->port]),
        0);

    /* Enable PFC */
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pfc_en[p->port]),
        icos_to_cos_mask);
  }
  if (p->fc_type == BF_TM_PAUSE_PORT) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pfc_en[p->port]),
        0);  // For port pause to kick in, should PFC bitmask be non zero ?
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                     .wac_reg.port_pause_en[p->port]),
        1);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_flowcontrol_mode(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val1, val2;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.port_pfc_en[p->port]),
      &val1);
  rc |= bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.port_pause_en[p->port]),
      &val2);

  p->fc_type = BF_TM_PAUSE_NONE;
  if (val2) {
    p->fc_type = BF_TM_PAUSE_PORT;
  }
  if (val1) {
    p->fc_type = BF_TM_PAUSE_PFC;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_pfc_enable_mask(bf_dev_id_t devid,
                                                     bf_tm_port_t *p,
                                                     uint8_t *enable_mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.port_pfc_en[p->port]),
      &val);
  *enable_mask = getp_wac_port_pfc_en_en(&val);
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_set_flowcontrol_rx(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  /*
   * For Tofino, this is a NO-OP as pfc_upd_enb bit should be
   * set always as per HW team recommendation
   */
  (void)devid;
  (void)p;
  return BF_TM_EOK;
}

bf_tm_status_t bf_tm_tofino_port_get_flowcontrol_rx(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  return (bf_tm_tofino_sch_get_port_sched(devid, p));
}

bf_tm_status_t bf_tm_tofino_port_set_pfc_cos_map(bf_dev_id_t devid,
                                                 bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  int i, j;
  uint8_t icosmask, icos_to_cos_mask, port_icos_to_cos_mask = 0;

  // collect all PPGs for port.
  // For each PPG, collect icos_mask
  // for each PPG's icos-mask, prepare reverse mask using p->cos_to_icos mapping
  // program reverse mask into wac_ppg_icos_mapping.entry[ppg->ppg]

  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (p->ppgs[i] && !(p->ppgs[i]->is_default_ppg)) {
      icosmask = p->ppgs[i]->ppg_cfg.icos_mask;
      icos_to_cos_mask = 0;
      for (j = 0; j < BF_TM_MAX_PFC_LEVELS; j++) {
        if (p->cos_to_icos[j] >= BF_TM_MAX_PFC_LEVELS) {
          // Invalid mapping.
          continue;
        }
        if ((1 << p->cos_to_icos[j]) & icosmask) {
          icos_to_cos_mask |= (1 << j);
        }
      }
      val = icos_to_cos_mask;
      port_icos_to_cos_mask |= icos_to_cos_mask;
      rc = bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[p->ppgs[i]->p_pipe]
                       .csr_mem_wac_ppg_icos_mapping.entry[p->ppgs[i]->ppg]),
          val);
    }
  }
  p->icos_to_cos_mask = port_icos_to_cos_mask;

  // Whether port_pfc_cos_map is configured before or after
  // set_ppg_pfc_treatment
  // configured, configuration has to be setup correctly. There should be no
  // enforcement of calling order of TM APIs.

  // Now, when PPG is configured as pfc (lossless), then without knowing
  // icos->cos
  // mapping it is not possible to configure enable PFC generation in
  // bf_tm_tofino_ppg_set_pfc_treatment().  Before TM generated PFC message,
  // PFC enable bit is checked. PFC enable bit is one-to-one mapping of
  // packet-cos bit. [ when PPG crosses thresholds, icos is reverse mapped to
  // cos
  // and PFC enable bit vector is checked to generate PFC message]

  // Hence whenever port PFC cos map is setup, go over PPGs of the port and for
  // PPGs that have PFC treatment, enable PFC generation bit. So call
  // bf_tm_tofino_ppg_set_pfc_treatment()

  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (p->ppgs[i]) {
      bf_tm_tofino_ppg_set_pfc_treatment(devid, p->ppgs[i]);
    }
  }

  return (rc);
}

/* Since HW only store bit mask representing icos->cos bit vector, it is
 * not possible to rebuild cos->icos array.
 * Example: If PPG x is mapped to icos 0 & 1 and
 *             icos 0 --> packet cos 7
 *             icos 1 --> packet cos 6
 *          then, for PFC generation, against PPGx a bit vector of 0x30 will
 *          be programmed in wac_ppg_icos_mapping.entry[ppg#]
 *          Since 0x30 bitmask cannot tell if ppg's icos 0 --> maps to
 *          cos 6 or cos 7, it is not possible reconstruct icos->cos mapping
 * array.
 */
bf_tm_status_t bf_tm_tofino_port_get_pfc_cos_mask(bf_dev_id_t devid,
                                                  bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t icos_to_cos_mask = 0;
  uint32_t val;
  int i;

  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (p->ppgs[i] && !(p->ppgs[i]->is_default_ppg)) {
      rc = bf_tm_read_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[p->ppgs[i]->p_pipe]
                       .csr_mem_wac_ppg_icos_mapping.entry[p->ppgs[i]->ppg]),
          &val);
      icos_to_cos_mask |= val;
    }
  }
  p->icos_to_cos_mask = icos_to_cos_mask;
  return (rc);
}

/* This function does read-modify-write. Do not invoke this
 * in fast-reconfig / hitless HA restore code.
 */
bf_tm_status_t bf_tm_tofino_port_set_cpu_port(bf_dev_id_t devid,
                                              bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, i;

  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.pipe_cnt; i++) {
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[i].ctrl),
        &val);
    setp_pre_ctrl_c2c_port(&val, p->port);
    setp_pre_ctrl_c2c_enable(&val, 1);
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[i].ctrl),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_ingress_drop_cntr(bf_dev_id_t devid,
                                                       bf_tm_port_t *p,
                                                       uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt = 0;
  uint64_t reg_cnt;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_drop_count_port[p->port]
                   .wac_drop_count_port_0_2),
      &val);
  cnt = val;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_drop_count_port[p->port]
                   .wac_drop_count_port_1_2),
      &val);
  reg_cnt = val;

  *count = (reg_cnt << 32) + cnt;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_egress_drop_color_cntr(bf_dev_id_t devid,
                                                            bf_tm_port_t *p,
                                                            bf_tm_color_t color,
                                                            uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt = 0, color_section;

  switch (color) {
    case BF_TM_COLOR_GREEN:
      color_section = p->port;
      break;
    case BF_TM_COLOR_YELLOW:
      color_section = p->port + 72;
      break;
    case BF_TM_COLOR_RED:
      color_section = p->port + 144;
      break;
    default:
      return BF_TM_EINV_ARG;
  }
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      color_section);

  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      &val);
  cnt = val;
  val = 0;
  rc = bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      &val);
  *count = ((uint64_t)val << 32) + cnt;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_egress_drop_cntr(bf_dev_id_t devid,
                                                      bf_tm_port_t *p,
                                                      uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt = 0;
  uint64_t reg_cnt;

  // Collect Green color packet drop count
  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port);
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;

  // Collect Yellow color packet drop count
  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port + 72);
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      &val);
  reg_cnt = val;
  *count += (reg_cnt << 32) + cnt;

  // Collect Red color packet drop count
  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port + 144);
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      &val);
  reg_cnt = val;
  *count += (reg_cnt << 32) + cnt;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_qac_drop_state(bf_dev_id_t devid,
                                                    bf_tm_port_t *port,
                                                    bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[port->p_pipe]
                   .qac_reg.port_drop_state[port->port]),
      &val);
  *state = val & 0x1;
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_clear_qac_drop_state(bf_dev_id_t devid,
                                                      bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[port->p_pipe]
                   .qac_reg.port_drop_state[port->port]),
      val);
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_clear_ingress_drop_cntr(bf_dev_id_t devid,
                                                         bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_drop_count_port[p->port]
                   .wac_drop_count_port_1_2),
      0);

  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .wac_reg.wac_drop_count_port[p->port]
                   .wac_drop_count_port_0_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_egress_drop_cntr(bf_dev_id_t devid,
                                                        bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  // Clear Green color packet drop count
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      0);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      0);

  // Clear Yellow color packet drop count
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port + 72);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      0);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      0);

  // Clear Red color packet drop count
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_drop_count_addr),
      p->port + 144);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_1_2),
      0);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
              .qac_reg.qac_port_drop_count_data.qac_queue_drop_count_data_0_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_ingress_usage_cntr(bf_dev_id_t devid,
                                                        bf_tm_port_t *p,
                                                        uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_cnt.cnt[p->port]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_ingress_usage_cntr(bf_dev_id_t devid,
                                                          bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_cnt.cnt[p->port]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_egress_usage_cntr(bf_dev_id_t devid,
                                                       bf_tm_port_t *p,
                                                       uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_cell_count.qac_port_cell_count[p->port]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_egress_usage_cntr(bf_dev_id_t devid,
                                                         bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_cell_count.qac_port_cell_count[p->port]),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_ingress_wm_cntr(bf_dev_id_t devid,
                                                     bf_tm_port_t *p,
                                                     uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_wm_cnt.wm_cnt[p->port]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_egress_wm_cntr(bf_dev_id_t devid,
                                                    bf_tm_port_t *p,
                                                    uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_wm_cell_count.cell_count[p->port]),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_ingress_watermark(bf_dev_id_t devid,
                                                         bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[p->p_pipe]
                   .csr_mem_wac_port_wm_cnt.wm_cnt[p->port]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_clear_egress_watermark(bf_dev_id_t devid,
                                                        bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .csr_mem_qac_port_wm_cell_count.cell_count[p->port]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_get_total_in_cells(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;

  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_clc_top.clc[pipe]
                                   .inport_cell_cnt.inport_cell_cnt_0_2),
                      &val);
  cnt = val;
  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_clc_top.clc[pipe]
                                   .inport_cell_cnt.inport_cell_cnt_1_2),
                      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_clear_total_in_cells(bf_dev_id_t devid,
                                                      bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(devid,
                       offsetof(Tofino,
                                device_select.tm_top.tm_clc_top.clc[pipe]
                                    .inport_cell_cnt.inport_cell_cnt_0_2),
                       0);

  bf_tm_write_register(devid,
                       offsetof(Tofino,
                                device_select.tm_top.tm_clc_top.clc[pipe]
                                    .inport_cell_cnt.inport_cell_cnt_1_2),
                       0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_get_total_in_pkts(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe,
                                                   uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;

  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                                   .wac_reg.ctr_vld_sop.ctr_vld_sop_0_2),
                      &val);

  cnt = val;
  bf_tm_read_register(devid,
                      offsetof(Tofino,
                               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                                   .wac_reg.ctr_vld_sop.ctr_vld_sop_1_2),
                      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;

  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_clear_total_in_pkts(bf_dev_id_t devid,
                                                     bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.ctr_vld_sop.ctr_vld_sop_0_2),
      0);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                   .wac_reg.ctr_vld_sop.ctr_vld_sop_1_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_get_uc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;

  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].uc_ct_cnt.uc_ct_cnt_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].uc_ct_cnt.uc_ct_cnt_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_clear_uc_ct_count(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].uc_ct_cnt.uc_ct_cnt_0_2),
      0);

  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].uc_ct_cnt.uc_ct_cnt_1_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_get_mc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;

  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].mc_ct_cnt.mc_ct_cnt_0_2),
      &val);
  cnt = val;
  bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].mc_ct_cnt.mc_ct_cnt_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tofino_pipe_clear_mc_ct_count(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].mc_ct_cnt.mc_ct_cnt_0_2),
      0);

  bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_clc_top.clc[pipe].mc_ct_cnt.mc_ct_cnt_1_2),
      0);

  return (rc);
}

/* Exposed through BF_PAL API. */
bf_status_t bf_tm_tofino_port_set_cut_through(bf_dev_id_t devid,
                                              bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_port_t *pt = NULL;
  uint32_t val = 0, index, pt_speed, i, bport, bpipe;

  // Also set in CLC (internal block of TM) port-speed
  // and enable cut-through mode by default which can be overridden if needed..

  // Pointer to function that updates correct fields of register.
  void (*fptr)(uint32_t *, uint32_t) = NULL;

  // Inorder to avoid read-modify write program all 8 ports
  // that share single register.
  // Read-modify-write is avoided so that during
  // fast reconfig, there are no reads and only writes.

  // Each Pt_speed register has config for 8 ports. Hence get the first
  // port in block of 8 ports
  bf_dev_port_t devport = MAKE_DEV_PORT(p->l_pipe, p->port - (p->port % 8));
  rc = bf_tm_port_get_descriptor(devid, devport, &pt);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  bport = pt->port;
  bpipe = pt->p_pipe;

  for (i = 0; i < 8; i++) {
    switch (pt->port % 8) {
      case 0:
        fptr = setp_qclc_pt_spd_pt_spd_0;
        break;
      case 1:
        fptr = setp_qclc_pt_spd_pt_spd_1;
        break;
      case 2:
        fptr = setp_qclc_pt_spd_pt_spd_2;
        break;
      case 3:
        fptr = setp_qclc_pt_spd_pt_spd_3;
        break;
      case 4:
        fptr = setp_qclc_pt_spd_pt_spd_4;
        break;
      case 5:
        fptr = setp_qclc_pt_spd_pt_spd_5;
        break;
      case 6:
        fptr = setp_qclc_pt_spd_pt_spd_6;
        break;
      default:  // Must be 7...
        fptr = setp_qclc_pt_spd_pt_spd_7;
        break;
    }
    switch (pt->speed) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
        pt_speed = 0;
        break;
      case BF_SPEED_25G:
        pt_speed = 1;
        break;
      case BF_SPEED_40G:
        pt_speed = 2;
        break;
      case BF_SPEED_50G:
        pt_speed = 3;
        break;
      case BF_SPEED_100G:
        pt_speed = 4;
        break;
      default:
        pt_speed = 4;
        break;
    }
    // Enable/Disable Cut-through.
    if (pt->ct_enabled) {
      bf_sku_chip_part_rev_t part_rev;
      rc = lld_sku_get_chip_part_revision_number(devid, &part_rev);
      if (rc != LLD_OK) {
        LOG_ERROR("%s: Could not get chip part revision number", __func__);
        return (rc);
      }
      if (part_rev != BF_SKU_CHIP_PART_REV_A0) {
        pt_speed |= 0x8;  // Set bit 3 / Enable CT
      } else {
        /* Disable CT mode; WA for A0 issue */
        // Remove following line once WA need to be removed.
        pt_speed &= 0x7;
      }
    } else {
      pt_speed &= 0x7;  // clear bit 3 / Disable CT
    }
    fptr(&val, pt_speed);
    pt++;
  }
  // clc register is array of 36 register. Each register has speed for upto 8
  // ports.
  // 72 ports per pipe.
  index = ((bpipe * 72) + bport) / 8;

  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_clc_top.clc_common.pt_speed[index]),
      val);
  return (rc);
}

bf_status_t bf_tm_tofino_port_get_cut_through(bf_dev_id_t devid,
                                              bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, index, pt_speed, bport, bpipe;

  // Pointer to function that extracts correct fields of register.
  uint32_t (*fptr)(uint32_t *) = NULL;

  // CLC register is array of 36 registers. Each register has speed & CT config
  // for upto 8 ports.
  // 72 ports per pipe. Get the index for the input port based on this.
  bport = p->port;
  bpipe = p->p_pipe;
  index = ((bpipe * 72) + bport) / 8;

  // Each Pt_speed register has config for 8 ports.
  // Get the function pointer for the 4bit block of input port
  switch (p->port % 8) {
    case 0:
      fptr = getp_qclc_pt_spd_pt_spd_0;
      break;
    case 1:
      fptr = getp_qclc_pt_spd_pt_spd_1;
      break;
    case 2:
      fptr = getp_qclc_pt_spd_pt_spd_2;
      break;
    case 3:
      fptr = getp_qclc_pt_spd_pt_spd_3;
      break;
    case 4:
      fptr = getp_qclc_pt_spd_pt_spd_4;
      break;
    case 5:
      fptr = getp_qclc_pt_spd_pt_spd_5;
      break;
    case 6:
      fptr = getp_qclc_pt_spd_pt_spd_6;
      break;
    default:  // Must be 7...
      fptr = getp_qclc_pt_spd_pt_spd_7;
      break;
  }

  rc = bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_clc_top.clc_common.pt_speed[index]),
      &val);

  pt_speed = fptr(&val);
  // Bit 3 is for CT enable/disable
  p->ct_enabled = (pt_speed & (1 << 3)) ? true : false;

  return (rc);
}

/* Not exposed as API. This is internal to TM. */
static bf_status_t bf_tm_tofino_set_pex_port_config(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_port_t *localp;
  uint32_t pex_port_cfg = 0, i, pt_lim;
  uint8_t port_speed[4];
  uint32_t pt_gap_lim0 = 0, pt_gap_lim1 = 0, pt_gap_lim2 = 0, pt_gap_lim3 = 0;

  if ((p->port / 4) >= BF_TM_TOFINO_PG_PER_PIPE) {
    LOG_ERROR("Incorrect port %d for dev %d", p->port, devid);
    return BF_INVALID_ARG;
  }

  if ((p->speed != BF_SPEED_100G) && (p->speed != BF_SPEED_40G)) {
    bf_dev_port_t devport = MAKE_DEV_PORT(p->l_pipe, p->port - (p->port % 4));
    rc = bf_tm_port_get_descriptor(devid, devport, &localp);
    if (rc != BF_SUCCESS) {
      return (rc);
    }
    for (i = 0; i < 4; i++) {
      port_speed[i] = localp->speed;
      localp++;
    }
    if (port_speed[0] == BF_SPEED_50G && port_speed[2] == BF_SPEED_50G) {
      pex_port_cfg = 3;
      pt_gap_lim0 = 0x20;
      pt_gap_lim1 = 0x20;
      pt_gap_lim2 = 0x20;
      pt_gap_lim3 = 0x20;
    }
    if (port_speed[0] == BF_SPEED_50G &&
        (port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_10G)) {
      pex_port_cfg = 1;
      pt_gap_lim0 = 0x20;
      pt_gap_lim1 = 0x20;
      pt_gap_lim2 = 0x40;
      pt_gap_lim3 = 0x40;
    }
    if (port_speed[2] == BF_SPEED_50G &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_10G)) {
      pex_port_cfg = 2;
      pt_gap_lim0 = 0x40;
      pt_gap_lim1 = 0x40;
      pt_gap_lim2 = 0x20;
      pt_gap_lim3 = 0x20;
    }
    if ((port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_10G) &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_10G)) {
      pex_port_cfg = 0;
      pt_gap_lim0 = 0x40;
      pt_gap_lim1 = 0x40;
      pt_gap_lim2 = 0x40;
      pt_gap_lim3 = 0x40;
    }
    if ((pt_gap_lim0 == 0) && (pt_gap_lim1 == 0) && (pt_gap_lim2 == 0) &&
        (pt_gap_lim3 == 0)) {
      LOG_WARN(
          "TM: Unexpected port config (%d, %d, %d, %d) for dev %d, pipe %d "
          "port %d",
          port_speed[0],
          port_speed[1],
          port_speed[2],
          port_speed[3],
          devid,
          p->p_pipe,
          p->port);
    }
  } else {
    // Config for 100G and 1x40G port
    pex_port_cfg = 4;
    pt_gap_lim0 = 0xf;
    pt_gap_lim1 = 0xf;
    pt_gap_lim2 = 0xf;
    pt_gap_lim3 = 0xf;
  }

  pt_lim = 0;
  setp_qpex_pt_gap_lim_pt_gap_lim0(&pt_lim, pt_gap_lim0);
  setp_qpex_pt_gap_lim_pt_gap_lim1(&pt_lim, pt_gap_lim1);
  setp_qpex_pt_gap_lim_pt_gap_lim2(&pt_lim, pt_gap_lim2);
  setp_qpex_pt_gap_lim_pt_gap_lim3(&pt_lim, pt_gap_lim3);

  bf_tm_write_register(devid,
                       offsetof(Tofino,
                                device_select.tm_top.tm_clc_top.pex[p->p_pipe]
                                    .pg_single[p->port / 4]),
                       pex_port_cfg);
  bf_tm_write_register(devid,
                       offsetof(Tofino,
                                device_select.tm_top.tm_clc_top.pex[p->p_pipe]
                                    .pt_gap_lim[p->port / 4]),
                       pt_lim);

  return (rc);
}

#define BF_TM_TOFINO_QAC_RX_NUM_WAITS 50
#define BF_TM_TOFINO_QAC_RX_WAIT_TIME 200
/* Disable/Enable qac_port_rx_disable bit */
static bf_tm_status_t bf_tm_tofino_set_port_rx(bf_dev_id_t devid,
                                               bf_tm_port_t *p) {
  uint32_t val_0_3 = 0, val_1_3 = 0, val_2_3 = 0, i;
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_port_t *localp;
  bf_dev_port_t devport;
  uint32_t num_waits = 0, port_usage_count;

  // The entire wide register needs to be updated even if single
  // bit needs to be changed.

  // port 64 to 71 state
  devport = MAKE_DEV_PORT(p->l_pipe, 64);
  rc = bf_tm_port_get_descriptor(devid, devport, &localp);
  if (rc != BF_SUCCESS) return (rc);
  for (i = 0; i < 8; i++) {
    val_2_3 |= ((localp->qac_rx_enable) ? 0 : 1) << i;
    localp++;
  }

  // port 32 to 63 state
  devport = MAKE_DEV_PORT(p->l_pipe, 32);
  rc = bf_tm_port_get_descriptor(devid, devport, &localp);
  if (rc != BF_SUCCESS) return (rc);
  for (i = 0; i < 32; i++) {
    val_1_3 |= ((localp->qac_rx_enable) ? 0u : 1u) << i;
    localp++;
  }

  // port 0 to 31 state
  devport = MAKE_DEV_PORT(p->l_pipe, 0);
  rc = bf_tm_port_get_descriptor(devid, devport, &localp);
  if (rc != BF_SUCCESS) return (rc);
  for (i = 0; i < 32; i++) {
    val_0_3 |= ((localp->qac_rx_enable) ? 0u : 1u) << i;
    localp++;
  }
  rc = bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_rx_disable.qac_port_rx_disable_2_3),
      val_2_3);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_rx_disable.qac_port_rx_disable_1_3),
      val_1_3);
  rc |= bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_qac_top.qac_pipe[p->p_pipe]
                   .qac_reg.qac_port_rx_disable.qac_port_rx_disable_0_3),
      val_0_3);

  /*
   * If the qac_rx is getting disabled now due to port-disable or port-delete,
   * make sure TM buffers get drained and usage count
   * goes to zero. This shouldn't be done for fast reconfig and
   * hitless scenarios. Also, skip this for model target.
   */
  if (p->qac_rx_enable == false && tm_is_device_locked(devid) == false &&
      g_tm_ctx[devid]->target == BF_TM_TARGET_ASIC) {
    while (num_waits++ <= BF_TM_TOFINO_QAC_RX_NUM_WAITS) {
      bf_tm_port_get_egress_usage_counter(devid, p, &port_usage_count);
      LOG_TRACE(
          "TM buffers egress usage count after "
          "disabling qac_rx for dev %d, dev_port %d "
          "before wait %d is %d",
          devid,
          MAKE_DEV_PORT(p->l_pipe, p->port),
          num_waits,
          port_usage_count);

      if (port_usage_count == 0) {
        break;
      }
      bf_sys_usleep(BF_TM_TOFINO_QAC_RX_WAIT_TIME);
    }

    if (num_waits > BF_TM_TOFINO_QAC_RX_NUM_WAITS) {
      /* Log a trace msg */
      LOG_TRACE(
          "TM buffers not getting drained after "
          "disabling qac_rx for dev %d, dev_port %d",
          devid,
          MAKE_DEV_PORT(p->l_pipe, p->port));
    } else {
      LOG_TRACE(
          "TM buffers got drained after "
          "disabling qac_rx for dev %d, dev_port %d "
          "after %d waits",
          devid,
          MAKE_DEV_PORT(p->l_pipe, p->port),
          num_waits - 1);
    }

    if (num_waits > 1) {
      /* Wait one more time so that packets get drained in EBUF */
      bf_sys_usleep(BF_TM_TOFINO_QAC_RX_WAIT_TIME);
    }
  }

  return (rc);
}

static bf_tm_status_t bf_tm_tofino_port_speed_based_default_cfg(
    bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  // Set default CT limits which can be modified by APIs at later point
  switch (p->speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      p->uc_cut_through_limit = 4;
      break;
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      p->uc_cut_through_limit = 8;
      break;
    case BF_SPEED_100G:
      p->uc_cut_through_limit = 0xf;
      break;
    default:
      p->uc_cut_through_limit = 0xf;
      break;
  }
  rc = bf_tm_tofino_port_set_uc_ct_limit(devid, (void *)p);
  rc |= bf_tm_tofino_port_set_cut_through(devid, p);
  rc |= bf_tm_tofino_set_pex_port_config(devid, p);
  return (rc);
}

/*
 * Neccessary config applied to ensure Egress path in TM  is setup
 * before packets from the port that is being added arrive inside TM.
 */
bf_tm_status_t bf_tm_tofino_add_new_port(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_port_t port;
  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOFINO_QUEUES_PER_PG];

  // Set default config for the port
  rc = bf_tm_tofino_port_speed_based_default_cfg(devid, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to set default port config");
  }
  if (BF_TM_IS_OK(rc)) {
    // Enable Shaper to default value
    rc = bf_tm_tofino_sch_set_port_rate(devid, (void *)p);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("Not able to enable default port shaper");
    }
  }

  if (BF_TM_IS_OK(rc)) {
    // Get the queue count for the port
    port = MAKE_DEV_PORT(p->l_pipe, p->port);
    rc = bf_tm_port_q_mapping_get(devid, port, &q_count, q_mapping);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "TM: getting queue count failed for dev %d pipe %d port %d, rc %s "
          "(%d)",
          devid,
          p->p_pipe,
          p->port,
          bf_err_str(rc),
          rc);
      return (rc);
    }

    // Map Qs of the to port to correct channel.
    for (q = 0; q < q_count; q++) {
      rc = bf_tm_q_get_descriptor(devid, port, q, &queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      // Set channel ID to which queue belongs to.
      if (p->speed == BF_SPEED_100G) {
        queue->q_sch_cfg.cid = 0;
      } else {
        queue->q_sch_cfg.cid = p->port % 4;
      }
      queue->q_sch_cfg.max_rate = p->port_rate;
      queue->q_sch_cfg.min_rate = 0;
      rc = bf_tm_tofino_sch_set_q_rate(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      rc = bf_tm_tofino_sch_set_q_min_rate(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      // Per HW team, SCH PFC has to be enabled always
      queue->q_sch_cfg.sch_pfc_enabled = true;
      rc = bf_tm_tofino_sch_set_q_sched(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }
    }
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("Could not bind queues to pipe %d port %d", p->p_pipe, p->port);
    }
  }

  // Set default Q threshold
  if (BF_TM_IS_OK(rc)) {
    for (q = 0; q < q_count; q++) {
      rc |= bf_tm_q_guaranteed_min_limit_set(devid, port, q, BF_TM_Q_GMIN_LMT);
    }
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("Could not set default Q thresholds");
    }
  }
  if (BF_TM_IS_OK(rc)) {
    // Clear qac_port_rx_disable
    rc = bf_tm_tofino_set_port_rx(devid, p);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("Could not clear port Rx disable");
    }
  }

  return (rc);
}

/*
 * Neccessary config applied to ensure no stale packets in TM
 * queued against the port that is being removed.
 */
bf_tm_status_t bf_tm_tofino_delete_port(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_port_t port;
  bf_tm_queue_t q;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOFINO_QUEUES_PER_PG];

  rc = bf_tm_tofino_port_set_cut_through(devid, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: setting cut through failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        devid,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  // Disable Shaping so that any congested egress packets against the port
  // are pushed to e-pipe.
  //                         OR
  //  Set shaper to MAX rate ?

  // Set qac_port_rx_disable
  rc = bf_tm_tofino_set_port_rx(devid, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: disabling QAC RX failed for dev %d, pipe %d port %d, rc %s (%d)",
        devid,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  // Zero default Q threshold for all queues of this port
  port = MAKE_DEV_PORT(p->l_pipe, p->port);
  // Get the queue count
  rc = bf_tm_port_q_mapping_get(devid, port, &q_count, q_mapping);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: getting queue count failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        devid,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }
  for (q = 0; q < q_count; q++) {
    rc |= bf_tm_q_guaranteed_min_limit_set(devid, port, q, 0);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_set_qac_rx(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_tofino_set_port_rx(devid, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: setting QAC RX failed for dev %d, pipe %d port %d, rc %s (%d)",
        devid,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tofino_port_get_pre_mask(bf_dev_id_t devid,
                                              uint32_t *mask_array,
                                              uint32_t size) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (size < 9 * 2) {
    return BF_TM_EINT;
  }

  for (uint32_t i = 0; i < 2; ++i) {
    uint32_t offset = 9 * i;
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_0_9),
        &val);
    mask_array[0 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_1_9),
        &val);
    mask_array[1 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_2_9),
        &val);
    mask_array[2 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_3_9),
        &val);
    mask_array[3 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_4_9),
        &val);
    mask_array[4 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_5_9),
        &val);
    mask_array[5 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_6_9),
        &val);
    mask_array[6 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_7_9),
        &val);
    mask_array[7 + offset] = val;
    val = 0;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_8_9),
        &val);
    mask_array[8 + offset] = val;
  }
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_clear_pre_mask(bf_dev_id_t devid) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  for (uint32_t i = 0; i < 2; ++i) {
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_0_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_1_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_2_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_3_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_4_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_5_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_6_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_7_9),
        val);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_8_9),
        val);
  }
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_get_pre_down_mask(bf_dev_id_t devid,
                                                   uint32_t *mask_array,
                                                   uint32_t size) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (size < 9) {
    return BF_TM_EINT;
  }

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_0_9),
      &val);
  mask_array[0] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_1_9),
      &val);
  mask_array[1] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_2_9),
      &val);
  mask_array[2] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_3_9),
      &val);
  mask_array[3] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_4_9),
      &val);
  mask_array[4] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_5_9),
      &val);
  mask_array[5] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_6_9),
      &val);
  mask_array[6] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_7_9),
      &val);
  mask_array[7] = val;
  val = 0;

  rc |= bf_tm_read_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_8_9),
      &val);
  mask_array[8] = val;
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tofino_port_clear_pre_down_mask(bf_dev_id_t devid) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_0_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_1_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_2_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_3_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_4_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_5_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_6_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_7_9),
      val);

  rc |= bf_tm_write_register(
      devid,
      offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_8_9),
      val);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}
///////////////// PRE/MCAST FIFO ////////////////////////////

bf_tm_status_t bf_tm_tofino_set_mcast_fifo_arbmode(bf_dev_id_t devid,
                                                   bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe].arb_ctrl),
      fifo->arb_mode));
}

bf_tm_status_t bf_tm_tofino_get_mcast_fifo_arbmode(bf_dev_id_t devid,
                                                   bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe].arb_ctrl),
      &(fifo->arb_mode)));
}

bf_tm_status_t bf_tm_tofino_set_mcast_fifo_wrr_weight(
    bf_dev_id_t devid, bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe].wrr_ctrl),
      fifo->weight));
}

bf_tm_status_t bf_tm_tofino_get_mcast_fifo_wrr_weight(
    bf_dev_id_t devid, bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe].wrr_ctrl),
      &(fifo->weight)));
}

bf_tm_status_t bf_tm_tofino_set_mcast_fifo_icos_bmap(bf_dev_id_t devid,
                                                     bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  int icos;

  for (icos = 0; icos < 8; icos++) {
    if ((1 << icos) & fifo->icos_bmap) {
      rc |= bf_tm_write_register(
          devid,
          offsetof(Tofino,
                   device_select.tm_top.tm_wac_top.wac_pipe[fifo->phy_pipe]
                       .wac_reg.wac_pre_fifo_mapping[icos]),
          fifo->fifo);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_get_mcast_fifo_icos_bmap(bf_dev_id_t devid,
                                                     bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  int icos;
  uint32_t val;

  fifo->icos_bmap = 0;
  for (icos = 0; icos < 8; icos++) {
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[fifo->phy_pipe]
                     .wac_reg.wac_pre_fifo_mapping[icos]),
        &val);
    if (val == fifo->fifo) {
      fifo->icos_bmap |= (1 << icos);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tofino_set_mcast_fifo_depth(bf_dev_id_t devid,
                                                 bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_write_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe]
                   .fifo_depth[fifo->fifo]),
      fifo->size));
}

bf_tm_status_t bf_tm_tofino_get_mcast_fifo_depth(bf_dev_id_t devid,
                                                 bf_tm_mcast_fifo_t *fifo) {
  return (bf_tm_read_register(
      devid,
      offsetof(Tofino,
               device_select.tm_top.tm_pre_top.pre[fifo->phy_pipe].fifo_depth),
      &(fifo->size)));
}

///////////////TM PM DROP ERROR DISCARD COUNTERS////////////////////

// bf_tm_blklvl_cntrs_t *s
bf_tm_status_t bf_tm_tofino_blklvl_get_drop_cntrs(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, bf_tm_blklvl_cntrs_t *blk_cntrs) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (blk_cntrs) {
    TRAFFIC_MGR_MEMSET(blk_cntrs, 0, sizeof(bf_tm_blklvl_cntrs_t));

    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_drop_no_dst),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->wac_no_dest_drop = (uint64_t)val;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.wac_drop_buf_full.wac_drop_buf_full_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.wac_drop_buf_full.wac_drop_buf_full_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->wac_buf_full_drop = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_caa.epipe[pipe]
                                          .pkt_dropcnt.pkt_dropcnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_caa.epipe[pipe]
                                           .pkt_dropcnt.pkt_dropcnt_1_2),
                              &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->egress_pipe_total_drop = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->psc_pkt_drop = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .pex_dis_cnt.pex_dis_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .pex_dis_cnt.pex_dis_cnt_1_2),
                              &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->pex_total_disc = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qac_dis_cnt.qac_dis_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .qac_dis_cnt.qac_dis_cnt_1_2),
                              &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->qac_total_disc = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dis_dq_cnt.tot_dis_dq_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .tot_dis_dq_cnt.tot_dis_dq_cnt_1_2),
                              &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_disc_dq = cnt;

    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                     .qac_reg.qac_ctr32_drop_no_dst),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->qac_no_dest_drop = (uint64_t)val;

    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                     .qac_reg.qac_ctr32_pre_mc_drop),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->qac_pre_mc_drop = (uint64_t)val;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .packet_drop.packet_drop_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc |= bf_tm_read_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_pre_top.pre[pipe]
                                           .packet_drop.packet_drop_1_2),
                              &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->pre_total_drop = cnt;

    // valid SOP
    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_vld_sop.ctr_vld_sop_0_2),
        &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_vld_sop.ctr_vld_sop_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->valid_sop_cntr = cnt;

    // PH lost
    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre[pipe].ph_lost.ph_lost_0_2),
        &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre[pipe].ph_lost.ph_lost_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->ph_lost_cntr = cnt;

    // CPU copy
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .cpu_copies.cpu_copies_0_2),
                             &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .cpu_copies.cpu_copies_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->cpu_copy_cntr = cnt;

    // total PH processed
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .ph_processed.ph_processed_0_2),
                             &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .ph_processed.ph_processed_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_ph_processed = cnt;

    // total copied
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .total_copies.total_copies_0_2),
                             &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .total_copies.total_copies_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_copied_cntr = cnt;

    // xid prunes
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .xid_prunes.xid_prunes_0_2),
                             &val);

    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .xid_prunes.xid_prunes_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_xid_prunes_cntr = cnt;

    // yid prunes
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .yid_prunes.yid_prunes_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .yid_prunes.yid_prunes_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_yid_prunes_cntr = cnt;

    // PH in use
    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_psc_top.psc[pipe].psc_ph_used),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->ph_in_use_cntr = val;

    // clc total
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_cell_cnt.tot_cell_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_cell_cnt.tot_cell_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->clc_total_cell_cntr = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->clc_total_pkt_cntr = cnt;

    // eport total
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_cell_cnt.port_cell_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_cell_cnt.port_cell_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->eport_total_cell_cntr = cnt;

    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_pkt_cnt.port_pkt_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_pkt_cnt.port_pkt_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->eport_total_pkt_cntr = cnt;

    // pex total pkt
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->pex_total_pkt_cntr = cnt;

    // total enq
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_eq_cnt.tot_eq_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_eq_cnt.tot_eq_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_enq_cntr = cnt;

    // total deq
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dq_cnt.tot_dq_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dq_cnt.tot_dq_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_deq_cntr = cnt;

    // CAA used blocks
    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_caa.epipe[pipe].blks_usecnt),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->caa_used_blocks = val;

    // PSC used blocks
    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(
            Tofino,
            device_select.tm_top.tm_psc_top.psc_common.epipe[pipe].blks_usecnt),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->psc_used_blocks = val;

    // qid enq
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_eq_cnt.qid_eq_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_eq_cnt.qid_eq_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->qid_enq_cntr = cnt;

    // qid deq
    cnt = 0;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_deq_cnt.qid_deq_cnt_0_2),
                             &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_read_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_deq_cnt.qid_deq_cnt_1_2),
                             &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->qid_deq_cntr = cnt;

    // The following four counters from PRC cannot be read while multicast
    // traffic is flowing on TF1.  Therefore we skip reading them all together.

    // qac query
    blk_cntrs->qac_query_cntr = 0;

    // qac query zero
    blk_cntrs->qac_query_zero_cntr = 0;

    // PRC total PEX cntr
    blk_cntrs->prc_total_pex_cntr = 0;

    // PRC total PEX zero cntr
    blk_cntrs->prc_total_pex_zero_cntr = 0;
  }
  return (rc);
}

#define UNUSED(x) (void)x;

// bf_tm_fifo_cntrs_t
bf_tm_status_t bf_tm_tofino_pre_fifo_get_drop_cntrs(
    bf_dev_id_t devid, bf_tm_pre_fifo_cntrs_t *fifo_cntrs) {
  bf_tm_status_t rc = BF_TM_EOK;

  uint32_t val = 0;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (fifo_cntrs) {
    TRAFFIC_MGR_MEMSET(fifo_cntrs, 0, sizeof(bf_tm_pre_fifo_cntrs_t));

    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[0]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[0]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre0_fifo[0] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[1]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[1]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre0_fifo[1] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[2]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[2]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre0_fifo[2] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[3]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[3]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre0_fifo[3] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[0]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[0]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre1_fifo[0] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[1]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[1]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre1_fifo[1] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[2]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[2]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre1_fifo[2] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[3]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[3]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre1_fifo[3] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[0]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[0]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre2_fifo[0] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[1]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[1]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre2_fifo[1] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[2]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[2]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre2_fifo[2] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[3]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[3]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre2_fifo[3] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[0]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[0]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre3_fifo[0] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[1]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[1]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre3_fifo[1] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[2]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[2]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre3_fifo[2] = cnt;

    cnt = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[3]
                     .wac_drop_cnt_pre0_fifo_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;

    rc |= bf_tm_read_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[3]
                     .wac_drop_cnt_pre0_fifo_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    fifo_cntrs->wac_drop_cnt_pre3_fifo[3] = cnt;
  }

  return (rc);
}

/*
 *      Clear Registers
 */
bf_tm_status_t bf_tm_tofino_blklvl_clr_drop_cntrs(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t clear_mask) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (clear_mask & WAC_NO_DEST_DROP) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_drop_no_dst),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & WAC_BUF_FULL_DROP) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.wac_drop_buf_full.wac_drop_buf_full_0_2),
        0);
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.wac_drop_buf_full.wac_drop_buf_full_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & EGRESS_PIPE_TOTAL_DROP) {
    rc = bf_tm_write_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_caa.epipe[pipe]
                                           .pkt_dropcnt.pkt_dropcnt_0_2),
                              0);
    rc |= bf_tm_write_register(devid,
                               offsetof(Tofino,
                                        device_select.tm_top.tm_caa.epipe[pipe]
                                            .pkt_dropcnt.pkt_dropcnt_1_2),
                               0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PSC_PKT_DROP) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        0);
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PEX_TOTAL_DISC) {
    rc = bf_tm_write_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .pex_dis_cnt.pex_dis_cnt_0_2),
                              0);
    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .pex_dis_cnt.pex_dis_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_TOTAL_DISC) {
    rc = bf_tm_write_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .qac_dis_cnt.qac_dis_cnt_0_2),
                              0);
    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qac_dis_cnt.qac_dis_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & TOTAL_DISC_DQ) {
    rc = bf_tm_write_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_qlc_top.qlc[pipe]
                                           .tot_dis_dq_cnt.tot_dis_dq_cnt_0_2),
                              0);
    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dis_dq_cnt.tot_dis_dq_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_NO_DEST_DROP) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                     .qac_reg.qac_ctr32_drop_no_dst),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_PRE_MC_DROP) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_qac_top.qac_pipe[pipe]
                     .qac_reg.qac_ctr32_pre_mc_drop),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PRE_TOTAL_DROP) {
    rc = bf_tm_write_register(devid,
                              offsetof(Tofino,
                                       device_select.tm_top.tm_pre_top.pre[pipe]
                                           .packet_drop.packet_drop_0_2),
                              0);
    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .packet_drop.packet_drop_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & COUNTERS_ALL) {
    // valid SOP
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_vld_sop.ctr_vld_sop_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_pipe[pipe]
                     .wac_reg.ctr_vld_sop.ctr_vld_sop_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // PH lost
    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre[pipe].ph_lost.ph_lost_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_pre_top.pre[pipe].ph_lost.ph_lost_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // CPU copy

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .cpu_copies.cpu_copies_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .cpu_copies.cpu_copies_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // total PH processed

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .ph_processed.ph_processed_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .ph_processed.ph_processed_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // total copied

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .total_copies.total_copies_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .total_copies.total_copies_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // xid prunes

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .xid_prunes.xid_prunes_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .xid_prunes.xid_prunes_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // yid prunes

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .yid_prunes.yid_prunes_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_pre_top.pre[pipe]
                                          .yid_prunes.yid_prunes_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // PH in use

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_psc_top.psc[pipe].psc_ph_used),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // clc total

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_cell_cnt.tot_cell_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_cell_cnt.tot_cell_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.clc[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // eport total

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_cell_cnt.port_cell_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_cell_cnt.port_cell_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_pkt_cnt.port_pkt_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .port_pkt_cnt.port_pkt_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // pex total pkt

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_clc_top.pex[pipe]
                                          .tot_pkt_cnt.tot_pkt_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // total enq

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_eq_cnt.tot_eq_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_eq_cnt.tot_eq_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // total deq

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dq_cnt.tot_dq_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .tot_dq_cnt.tot_dq_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // CAA used blocks

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino, device_select.tm_top.tm_caa.epipe[pipe].blks_usecnt),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // PSC used blocks

    rc |= bf_tm_write_register(
        devid,
        offsetof(
            Tofino,
            device_select.tm_top.tm_psc_top.psc_common.epipe[pipe].blks_usecnt),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // qid enq

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_eq_cnt.qid_eq_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_eq_cnt.qid_eq_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // qid deq

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_deq_cnt.qid_deq_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_qlc_top.qlc[pipe]
                                          .qid_deq_cnt.qid_deq_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // qac query

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_prc_top.prc[pipe].qac_cnt.qac_cnt_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_prc_top.prc[pipe].qac_cnt.qac_cnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // qac query zero

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_prc_top.prc[pipe]
                                          .qac_zero_cnt.qac_zero_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_prc_top.prc[pipe]
                                          .qac_zero_cnt.qac_zero_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // PRC total PEX cntr

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_prc_top.prc[pipe].pex_cnt.pex_cnt_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_prc_top.prc[pipe].pex_cnt.pex_cnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    // PRC total PEX zero cntr

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_prc_top.prc[pipe]
                                          .pex_zero_cnt.pex_zero_cnt_0_2),
                             0);

    rc |=
        bf_tm_write_register(devid,
                             offsetof(Tofino,
                                      device_select.tm_top.tm_prc_top.prc[pipe]
                                          .pex_zero_cnt.pex_zero_cnt_1_2),
                             0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  return (rc);
}

// bf_tm_fifo_cntrs_t
bf_tm_status_t bf_tm_tofino_pre_fifo_clr_drop_cntrs(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t fifo) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (pipe == 0) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre0_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (pipe == 1) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre1_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (pipe == 2) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre2_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (pipe == 3) {
    rc = bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_0_2),
        0);

    rc |= bf_tm_write_register(
        devid,
        offsetof(Tofino,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_drop_cnt_pre3_fifo[fifo]
                     .wac_drop_cnt_pre0_fifo_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  return (rc);
}

// Function to fetch the current credits for 100G ports
bf_tm_status_t bf_tm_tofino_get_port_credits(bf_dev_id_t devid,
                                             bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (p != NULL) {
    uint32_t reg_val = 0;
    rc = bf_tm_read_register(
        devid,
        offsetof(
            Tofino,
            device_select.tm_top.tm_clc_top.pex[p->p_pipe].pt_epb_cred[p->pg]),
        &reg_val);

    uint32_t channel = p->port % BF_TM_TOFINO_PORTS_PER_PG;
    if (channel == 0) {
      p->credits = (reg_val & 0xFF);
    } else if (channel == 1) {
      p->credits = (reg_val & (0xFF << 8)) >> 8;
    } else if (channel == 2) {
      p->credits = (reg_val & (0xFF << 16)) >> 16;
    } else if (channel == 3) {
      p->credits = (reg_val & (0xFF << 24)) >> 24;
    }
  }
  return (rc);
}
