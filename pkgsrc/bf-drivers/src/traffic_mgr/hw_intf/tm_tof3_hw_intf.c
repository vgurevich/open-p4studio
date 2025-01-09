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
#include <assert.h>
#include <math.h>
#include <inttypes.h>

#include "tm_tof3_hw_intf.h"
#include "traffic_mgr/init/tm_tof3.h"

#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_mem_addr.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld.h>
#include <lld/lld_sku.h>

#include "traffic_mgr/init/tm_tof3_default.h"

/* HW read/write functions used by accessor functions in
 * tm_[ig_ppg.c/ig_pool.c/...
 */

/////////////////PPG////////////////////////////

#define BF_TM_PPG_FOLD_THRES_LIMIT(limit, fast_recover_mode, baf) \
  ((limit << 7) | (fast_recover_mode << 6) | (baf << 2))
#define BF_TM_PPG_THRES_LIMIT(val) (val >> 7)
#define BF_TM_PPG_BAF(val) ((val >> 2) & 0xf)
#define BF_TM_PPG_FAST_RECOVER(val) ((val >> 6) & 0x1ull)

bf_tm_status_t bf_tm_tof3_ppg_set_min_limit(bf_dev_id_t devid,
                                            const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);
  setp_tof3_wac_ppg_cnt_entry_cnt(&lo, ppg->thresholds.min_limit);

  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_th(
        ppg->d_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_th(
        ppg->d_pipe, ppg->port);
  }
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_min_limit(bf_dev_id_t devid,
                                            bf_tm_ppg_t *ppg) {
  uint64_t hi = 0, lo = 0;
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_th(
        ppg->d_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_th(
        ppg->d_pipe, ppg->port);
  }
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  ppg->thresholds.min_limit = getp_tof3_wac_ppg_cnt_entry_cnt(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_skid_limit(bf_dev_id_t devid,
                                             const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  setp_tof3_wac_ppg_cnt_entry_cnt(&lo, ppg->thresholds.skid_limit);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_th(ppg->d_pipe,
                                                                ppg->ppg);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_skid_limit(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg) {
  uint64_t hi = 0, lo = 0;
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_th(ppg->d_pipe,
                                                                ppg->ppg);

  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  ppg->thresholds.skid_limit = getp_tof3_wac_ppg_cnt_entry_cnt(&lo);
  return (rc);
}

// tof3_reg implements an array of 32 entries to specify hysteresis values.
// Instead of programming actual hysteresis value in tables, index of the
// 32 entry table is used. This index is refered as offset_idx in register
// spec. WAC offset proile table is used for HYST on PPG and Port.
// bf_tm_tof3_restore_wac_offset_profile() can be used to restore
// the following 2 SW tables/variables for hitless HA case.

// TODO: At some point all the offset_profiles will be occupied with some
// values which actually might be not used by any Port, PPG.
// Some reference counting and housekeeping is needed to reuse
// this limited resource.

static uint8_t wac_offset_profile_cnt[BF_TM_NUM_ASIC][BF_TM_TOF3_MAU_PIPES] = {
    {0}};
static uint32_t wac_offset_profile_tbl[BF_TM_NUM_ASIC][BF_TM_TOF3_MAU_PIPES]
                                      [BF_TM_TOF3_HYSTERESIS_PROFILES] = {
                                          {{0}}};

bf_status_t bf_tm_tof3_restore_wac_offset_profile(bf_dev_id_t dev) {
  uint32_t val;
  uint32_t num_p_pipes = 0;
  bf_subdev_id_t subdev_id;
  bf_dev_pipe_t d_pipe;

  g_tm_ctx[dev]->read_por_wac_profile_table = false;

  // Clear all the array, even if some pipes ara not used on the device.
  for (int p = 0; p < BF_TM_TOF3_MAU_PIPES; p++) {
    wac_offset_profile_cnt[dev][p] = 0;
  }

  lld_err_t lld_err = lld_sku_get_num_active_pipes(dev, &num_p_pipes);
  if ((LLD_OK != lld_err) || (num_p_pipes > BF_TM_TOF3_MAU_PIPES)) {
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
    subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, 0);
    d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, 0);

    for (int i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
      (void)bf_tm_subdev_read_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                       .wac_reg.offset_profile[i]),
          &val);
      LOG_TRACE("Restore dev %d:%d l_pipe %d p_pipe %d WAC hyst[%d]=0x%x",
                dev,
                subdev_id,
                l_pipe,
                p_pipe,
                i,
                val);
      // Ignore SW model garbage
      val = (TM_IS_TARGET_ASIC(dev)) ? val : BF_TM_TOF3_WAC_RESET_HYSTERESIS;
      wac_offset_profile_tbl[dev][p_pipe][i] = val;
      // Treat all values less than POR as empty items except the first one.
      // It might be at any index in the array.
      // Non-POR HW reset values are counted as empty items and will be reused
      // except the first one greate than POR.
      if ((got_reset_value && val == BF_TM_TOF3_WAC_RESET_HYSTERESIS) ||
          (got_default_value && val <= BF_TM_TOF3_WAC_POR_HYSTERESIS)) {
        continue;
      }

      wac_offset_profile_cnt[dev][p_pipe]++;
      if (val <= BF_TM_TOF3_WAC_POR_HYSTERESIS) {
        got_default_value = true;
      }
      if (val == BF_TM_TOF3_WAC_RESET_HYSTERESIS) {
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

static uint8_t bf_tm_tof3_get_wac_offset_profile_index(bf_dev_id_t dev,
                                                       bf_tm_thres_t cells_u,
                                                       uint8_t p_pipe) {
  bf_status_t rc = BF_SUCCESS;
  int i;

  // search from the start till end to check if cell_limit is already
  // in profile table.
  if (!g_tm_ctx[dev]->read_por_wac_profile_table) {
#ifndef BF_TM_HITLESS_HA_TESTING_WITH_MODEL
    rc = bf_tm_tof3_restore_wac_offset_profile(dev);
    if (BF_SUCCESS != rc) {
      return (BF_TM_TOF3_HYSTERESIS_PROFILES);
    }
#else
    LOG_TRACE("clear all WAC hyst profiles dev %d", dev);
    for (int p = 0; p < BF_TM_TOF3_MAU_PIPES; p++) {
      wac_offset_profile_cnt[dev][p] = 0;
      for (i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
        wac_offset_profile_tbl[dev][p][i] = BF_TM_TOF3_WAC_RESET_HYSTERESIS;
      }
    }
    g_tm_ctx[dev]->read_por_wac_profile_table = true;
#endif
  }

  /* For the hysteresis values, check if it is configured already */
  for (i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
    if (wac_offset_profile_tbl[dev][p_pipe][i] == cells_u) {
      return (i);
    }
  }
  return (BF_TM_TOF3_HYSTERESIS_PROFILES);
}

static bf_status_t bf_tm_tof3_populate_wac_offset_profile(bf_dev_id_t dev,
                                                          uint8_t p_pipe,
                                                          bf_tm_thres_t cells,
                                                          uint8_t *index) {
  int idx;
  bf_status_t rc = BF_SUCCESS;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, 0);
  uint32_t d_inst;
  uint32_t cells_u = TM_CELLS_TO_8CELL_UNITS(cells);

  // For dynamic sharing to work, hysteresis shouldn't be
  // 0 - the reason why POR value is 4 in 8 cells unit i.e 32.
  // Add default item if it is not yet in the table.
  if (cells_u < BF_TM_TOF3_WAC_POR_HYSTERESIS) {
    cells_u = BF_TM_TOF3_WAC_POR_HYSTERESIS;
  }

  idx = bf_tm_tof3_get_wac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOF3_HYSTERESIS_PROFILES) {
    // Already exists...
    *index = idx;
    return (rc);
  }

  if (wac_offset_profile_cnt[dev][p_pipe] >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
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
      for (idx = 0; idx < BF_TM_TOF3_HYSTERESIS_PROFILES; idx++) {
        if (wac_offset_profile_tbl[dev][p_pipe][idx] <=
            BF_TM_TOF3_WAC_POR_HYSTERESIS) {
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
                   BF_TM_TOF3_WAC_RESET_HYSTERESIS) {
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
      if (idx >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
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

    for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, d_inst);

      // Write to HW
      (void)bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                       .wac_reg.offset_profile[*index]),
          cells_u);
    }
    wac_offset_profile_cnt[dev][p_pipe]++;
  }
  return (rc);
}

inline static bf_status_t bf_tm_prep_ppg_shr_limit_reg_val(
    bf_dev_id_t dev, uint64_t *val, const bf_tm_ppg_t *ppg) {
  bf_status_t rc = BF_SUCCESS;
  (void)dev;

  if (ppg->ppg_cfg.is_dynamic) {
    // Fold in BAF for dynamic mode
    // Bits 0:1 are reserved
    // Bits 2:5 are for BAF
    // Bit 6 is for fast recover mode
    *val = BF_TM_PPG_FOLD_THRES_LIMIT(ppg->thresholds.app_limit,
                                      ppg->ppg_cfg.fast_recover_mode,
                                      ppg->ppg_cfg.baf);
  } else {
    *val = ppg->thresholds.app_limit;
  }

  return (rc);
}

/* Multi field register ; register set function */
inline static bf_tm_status_t bf_tm_tof3_ppg_set_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0, indir_addr;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  // First program the off_idx memory
  // Bits 0:4 correspond to hysteresis index
  // Bit 5 corresponds to dynamic mode enable/disable
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(
        ppg->d_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(
        ppg->d_pipe, ppg->port);
  }

  // Set the hysteresis index
  uint8_t hyst_index = 0;
  rc |= bf_tm_tof3_populate_wac_offset_profile(
      devid, ppg->p_pipe, ppg->thresholds.ppg_hyst, &(hyst_index));
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[devid], ppg->l_pipe, ppg->ppg);
  BF_TM_PPG_CHECK(_ppg, ppg->ppg, ppg->l_pipe, devid);
  _ppg->thresholds.hyst_index = hyst_index;

  setp_tof3_wac_ppg_off_idx_entry_off_idx(&lo, hyst_index);

  // Set the dynamic mode enable/disable
  setp_tof3_wac_ppg_off_idx_entry_dyn(&lo, ppg->ppg_cfg.is_dynamic ? 1 : 0);

  // Write the off_idx memory
  rc |= bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  // Next, program the shr_th memory
  lo = 0;
  rc |= bf_tm_prep_ppg_shr_limit_reg_val(devid, &lo, ppg);
  if (rc == BF_SUCCESS) {
    if (!ppg->is_default_ppg) {
      indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_th(
          ppg->d_pipe, ppg->ppg);
    } else {
      indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_shr_th(
          ppg->d_pipe, ppg->port);
    }
    rc |= bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  }

  return (rc);
}

inline static bf_tm_status_t bf_tm_tof3_ppg_get_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_ppg_t *ppg, uint32_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0, indir_addr;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_th(
        ppg->d_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_shr_th(
        ppg->d_pipe, ppg->port);
  }
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *val = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_app_limit(bf_dev_id_t devid,
                                            const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_app_limit(bf_dev_id_t devid,
                                            bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  bf_tm_tof3_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->thresholds.app_limit = val;
  if (ppg->ppg_cfg.is_dynamic) {
    ppg->thresholds.app_limit =
        BF_TM_PPG_THRES_LIMIT(ppg->thresholds.app_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_hyst(bf_dev_id_t devid,
                                       const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_hyst(bf_dev_id_t devid, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t cell_limit = 0;
  uint64_t hi = 0, lo = 0, indir_addr;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("PPG(%d) threshold idx read error %d", ppg->ppg, rc);
    return rc;
  }

  if (!ppg->is_default_ppg) {
    ppg->thresholds.hyst_index = getp_tof3_wac_ppg_off_idx_entry_off_idx(&lo);
  } else {
    ppg->thresholds.hyst_index = getp_tof3_wac_pg_off_idx_entry_off_idx(&lo);
  }

  if (ppg->thresholds.hyst_index >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
    LOG_ERROR("PPG(%d) threshold idx is invalid", ppg->ppg);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->d_pipe]
                   .wac_reg.offset_profile[ppg->thresholds.hyst_index]),
      &cell_limit);
  ppg->thresholds.ppg_hyst = cell_limit << 3;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_app_poolid(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  const bf_tm_ppg_t *neighbour_ppg;
  const bf_tm_ppg_t *_ppg, *def_ppg;
  int index;
  uint64_t hi = 0, lo = 0;
  bf_tm_port_t *_p;
  bf_dev_port_t devport;
  uint32_t ppg_enable = 0;
  uint32_t neighbour_ppg_enable = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  // get port , icos associated with ppg
  // if icos_mask is zero, still configure poolID but PPG will not be enabled.
  // Unless icos_mask is applied to PPG, traffic flow of relevant icos will
  // continue to use default PPG.

  index = ppg->port * 4;  // 2 icos per entry
  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->uport);
  // devport = lld_sku_map_devport_from_device_to_user(devid, devport);

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

    if (neighbour_ppg && !(neighbour_ppg->is_default_ppg)) {
      if ((1 << (i + 1)) & neighbour_ppg->ppg_cfg.icos_mask) {
        neighbour_ppg_enable = 1;
      } else {
        neighbour_ppg_enable = 0;
      }
      setp_tof3_wac_port_ppg_mapping_entry_ppg1(&lo, neighbour_ppg->ppg);
      setp_tof3_wac_port_ppg_mapping_entry_apid1(
          &lo, neighbour_ppg->ppg_cfg.app_poolid);
      setp_tof3_wac_port_ppg_mapping_entry_enb1(&lo, neighbour_ppg_enable);
    } else {
      // iCoS identified by cos level 'i+1' is mapped to default-ppg.
      // incase of default ppg, HW will ignore ppg#
      setp_tof3_wac_port_ppg_mapping_entry_ppg1(&lo, def_ppg->ppg);
      setp_tof3_wac_port_ppg_mapping_entry_apid1(&lo,
                                                 def_ppg->ppg_cfg.app_poolid);
      setp_tof3_wac_port_ppg_mapping_entry_enb1(&lo, 0);
    }
    if (_ppg && !(_ppg->is_default_ppg)) {
      if ((1 << i) & _ppg->ppg_cfg.icos_mask) {
        ppg_enable = 1;
      } else {
        ppg_enable = 0;
      }
      setp_tof3_wac_port_ppg_mapping_entry_ppg0(&lo, _ppg->ppg);
      setp_tof3_wac_port_ppg_mapping_entry_apid0(&lo, _ppg->ppg_cfg.app_poolid);
      setp_tof3_wac_port_ppg_mapping_entry_enb0(&lo, ppg_enable);
    } else {
      // iCoS identified by cos level 'i' is mapped to default-ppg.
      // incase of default ppg, HW will ignore ppg#
      setp_tof3_wac_port_ppg_mapping_entry_ppg0(&lo, def_ppg->ppg);
      setp_tof3_wac_port_ppg_mapping_entry_apid0(&lo,
                                                 def_ppg->ppg_cfg.app_poolid);
      setp_tof3_wac_port_ppg_mapping_entry_enb0(&lo, 0);
    }

    uint64_t indir_addr =
        tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(
            ppg->d_pipe, index + (i >> 1));
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  }

  // If PPG icos-mask is changed/updated/added, also program reverse mapping
  /// icos-->cos for PFC generation on the port where PPG is in use.
  rc |= bf_tm_tof3_port_set_pfc_cos_map(devid, _p);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_app_poolid(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint8_t ppgid = 0, poolid = 0;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

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
  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(
          ppg->d_pipe, index + (i >> 1));
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  if (i & 0x1) {
    ppgid = getp_tof3_wac_port_ppg_mapping_entry_ppg1(&lo);
    poolid = getp_tof3_wac_port_ppg_mapping_entry_apid1(&lo);
  } else {
    ppgid = getp_tof3_wac_port_ppg_mapping_entry_ppg0(&lo);
    poolid = getp_tof3_wac_port_ppg_mapping_entry_apid0(&lo);
  }

  ppg->ppg = ppgid;
  ppg->ppg_cfg.app_poolid = poolid;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_dynamic_mode(bf_dev_id_t devid,
                                               const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_dynamic_mode(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0, indir_addr;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("PPG(%d) threshold idx read error %d", ppg->ppg, rc);
    return rc;
  }

  if (!ppg->is_default_ppg) {
    ppg->ppg_cfg.is_dynamic = getp_tof3_wac_ppg_off_idx_entry_dyn(&lo);
  } else {
    ppg->ppg_cfg.is_dynamic = getp_tof3_wac_pg_off_idx_entry_dyn(&lo);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_baf(bf_dev_id_t devid, const bf_tm_ppg_t *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_ppg_set_shared_lmt_reg(devid, s);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_baf(bf_dev_id_t devid, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_tof3_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->ppg_cfg.baf = BF_TM_PPG_BAF(val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_icos_mask(bf_dev_id_t devid,
                                            bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint64_t hi = 0, lo = 0;
  uint16_t _ppg;
  uint8_t icos_mask = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  index = ppg->port * 4;
  for (i = 0; i < 8; i++) {
    uint64_t indir_addr =
        tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(
            ppg->d_pipe, index + i / 2);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (i & 0x1) {
      _ppg = getp_tof3_wac_port_ppg_mapping_entry_ppg1(&lo);
    } else {
      _ppg = getp_tof3_wac_port_ppg_mapping_entry_ppg0(&lo);
    }

    if (_ppg == ppg->ppg) {
      icos_mask |= (1 << i);
    }
  }
  ppg->ppg_cfg.icos_mask = icos_mask;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_set_pfc_treatment(bf_dev_id_t devid,
                                                const bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_port_t devport;
  bf_tm_port_t *_port;

  if (ppg->is_default_ppg) {
    // Cannot enable PFC on traffic mapped to default PPG
    return (rc);
  }

  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->uport);
  // devport = lld_sku_map_devport_from_device_to_user(devid, devport);

  _port = BF_TM_PORT_PTR(g_tm_ctx[devid], devport);

  /*
   * For Tofino2 (unlike Tofino), link pause enable and PFC enable config are
   * part of the same register (port_config). So, use
   * bf_tm_tof3_port_set_flowcontrol_mode function to configure the PFC
   * enable correctly based on the flowcontrol mode config.
   */
  rc = bf_tm_tof3_port_set_flowcontrol_mode(devid, _port);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_pfc_treatment(bf_dev_id_t devid,
                                                bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_tm_port_t *_port;
  uint8_t icos_to_cos_mask = 0, k;
  bf_dev_port_t devport;

  ppg->ppg_cfg.is_pfc = false;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  // Read PFC CoS currently enabled on the port.
  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.port_config[ppg->port]),
      &val);

  // Read pfc enable is packet-cos bmask
  // reverse map to icos and check if all icos are part of cos.
  // if true, then PPG is configured for PFC treatment.
  devport = MAKE_DEV_PORT(ppg->l_pipe, ppg->uport);
  // devport = lld_sku_map_devport_from_device_to_user(devid, devport);

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

/* In order to recover from driver restart and rebuild PPG allocation
 * read back port_to_ppg mapping table and return reverse mapping.
 * Used when restoring PPG config.
 */
bf_tm_status_t bf_tm_tof3_ppg_get_ppg_allocation(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  int index, i;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  index = ppg->port * 4;
  for (i = 0; i < 8; i++) {
    if ((1 << i) & ppg->ppg_cfg.icos_mask) {
      uint64_t indir_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(
              ppg->d_pipe, index + i / 2);
      rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
      if (i & 0x1) {
        ppg->ppg = getp_tof3_wac_port_ppg_mapping_entry_ppg1(&lo);
        ppg->is_default_ppg =
            getp_tof3_wac_port_ppg_mapping_entry_enb1(&lo) ? false : true;
        ppg->ppg_cfg.app_poolid =
            getp_tof3_wac_port_ppg_mapping_entry_apid1(&lo);
      } else {
        ppg->ppg = getp_tof3_wac_port_ppg_mapping_entry_ppg0(&lo);
        ppg->is_default_ppg =
            getp_tof3_wac_port_ppg_mapping_entry_enb0(&lo) ? false : true;
        ppg->ppg_cfg.app_poolid =
            getp_tof3_wac_port_ppg_mapping_entry_apid0(&lo);
      }
      break;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_drop_counter(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_ppg(ppg->d_pipe,
                                                                    ppg->ppg);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_clear_drop_counter(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_ppg(ppg->p_pipe,
                                                                    ppg->ppg);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_gmin_usage_counter(bf_dev_id_t devid,
                                                     bf_tm_ppg_t *ppg,
                                                     uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_cnt(
        ppg->d_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_cnt(
        ppg->d_pipe, ppg->port);
  }

  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_shared_usage_counter(bf_dev_id_t devid,
                                                       bf_tm_ppg_t *ppg,
                                                       uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_cnt(ppg->d_pipe,
                                                                 ppg->ppg);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_skid_usage_counter(bf_dev_id_t devid,
                                                     bf_tm_ppg_t *ppg,
                                                     uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  if (ppg->is_default_ppg) {
    // For non PFC PPGs, there is no head room / skid space.
    *count = 0;
    return (rc);
  }

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_cnt(ppg->d_pipe,
                                                                 ppg->ppg);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_wm_counter(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg,
                                             uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_wm_cnt(ppg->d_pipe,
                                                               ppg->ppg);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_clear_wm_counter(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(ppg->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_wm_cnt(ppg->p_pipe,
                                                               ppg->ppg);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_wac_get_buffer_full_counter(bf_dev_id_t devid,
                                                      bf_dev_pipe_t pipe,
                                                      uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val, cnt;
  uint64_t reg_cnt;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_0_2),
      &val);
  cnt = val;
  bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_1_2),
      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_wac_clear_buffer_full_counter(bf_dev_id_t devid,
                                                        bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_0_2),
      0);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_1_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_drop_state(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg,
                                             bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi, lo;
  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_drop_st(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_drop_st(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_read_memory(devid, 0, indir_addr, &hi, &lo);
  *state = getp_tof3_wac_ppg_drop_st_entry_drop_st(&lo);
  return rc;
}

bf_tm_status_t bf_tm_tof3_ppg_clear_drop_state(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_drop_st(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_drop_st(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);
  return rc;
}

bf_tm_status_t bf_tm_tof3_ppg_clear_gmin_usage_counter(bf_dev_id_t devid,
                                                       bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_cnt(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_cnt(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_clear_shared_usage_counter(bf_dev_id_t devid,
                                                         bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_cnt(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_shr_cnt(
        ppg->p_pipe, ppg->port);
  }

  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_clear_skid_usage_counter(bf_dev_id_t devid,
                                                       bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  if (ppg->is_default_ppg) {
    // For non PFC PPGs, there is no head room / skid space.
    return (BF_INVALID_ARG);
  }

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_cnt(ppg->p_pipe,
                                                                 ppg->ppg);
  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ppg_get_resume_limit(bf_dev_id_t devid,
                                               bf_tm_ppg_t *ppg,
                                               uint32_t *resume_lmt_cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint64_t hi = 0, lo = 0;
  uint8_t off_idx = 0;

  uint64_t indir_addr = 0;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(
        ppg->p_pipe, ppg->port);
  }
  rc = bf_tm_subdev_read_memory(devid, 0, indir_addr, &hi, &lo);
  if (BF_SUCCESS != rc) return BF_TM_EINT;
  off_idx = getp_tof3_wac_ppg_off_idx_entry_off_idx(&lo);

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.offset_profile[off_idx]),
      &val);
  if (BF_SUCCESS != rc) return BF_TM_EINT;
  val = getp_tof3_wac_offset_offset(&val);
  *resume_lmt_cells = TM_8CELL_UNITS_TO_CELLS(val);
  return rc;
}

bf_tm_status_t bf_tm_tof3_ppg_clear_resume_limit(bf_dev_id_t devid,
                                                 bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint64_t hi = 0, lo = 0;
  uint8_t off_idx = 0;

  uint64_t indir_addr = 0;
  if (!ppg->is_default_ppg) {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(
        ppg->p_pipe, ppg->ppg);
  } else {
    indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(
        ppg->p_pipe, ppg->port);
  }
  rc = bf_tm_subdev_read_memory(devid, indir_addr, 0, &hi, &lo);
  if (BF_SUCCESS != rc) return BF_TM_EINT;
  off_idx = getp_tof3_wac_ppg_off_idx_entry_off_idx(&lo);

  rc = bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[ppg->p_pipe]
                   .wac_reg.offset_profile[off_idx]),
      val);
  return rc;
}

bf_tm_status_t bf_tm_tof3_ppg_get_fast_recover_mode(bf_dev_id_t devid,
                                                    bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_tof3_ppg_get_shared_lmt_reg(devid, ppg, &val);
  ppg->ppg_cfg.fast_recover_mode = BF_TM_PPG_FAST_RECOVER(val);

  return (rc);
}

/////////////////queue////////////////////////////

#define BF_TM_Q_FOLD_THRES_LIMIT(limit, baf) ((limit << 4) | baf)
#define BF_TM_Q_THRES_LIMIT(val) (val >> 4)
#define BF_TM_Q_BAF(val) (val & 0xf)
#define Q_PER_EG_MEMORY (32)
#define Q_PER_IG_MEMORY (64)

// Poolid, color_drop_en, tail_drop_en
// shr_lmt, offset_idx, dyn_en, fast_recover_mode
// red_lmt_perc, red_offset_idx, yel_lmt_perc, yel_offset_idx
// Port_thrd, offset_idx

bf_tm_status_t bf_tm_tof3_sch_get_q_speed(bf_dev_id_t devid,
                                          bf_tm_eg_q_t *q,
                                          bf_port_speeds_t *speed) {
  (void)devid;

  bf_tm_status_t rc = BF_SUCCESS;

  switch (q->hq_per_vq) {
    case (0):
      *speed = BF_SPEED_NONE;
      break;
    case (2):
      *speed = BF_SPEED_50G;
      break;
    case (4):
      *speed = BF_SPEED_100G;
      break;
    case (8):
      *speed = BF_SPEED_200G;
      break;
    case (16):
      *speed = BF_SPEED_400G;
      break;
    default:
      *speed = BF_SPEED_NONE;
      rc = BF_INVALID_ARG;
      break;
  }

  return (rc);
}

// Build a map of current HQ to VQ assignments adjusted to PG scope
// and find there a chunk to fit the VQ.
// HQs are per-pipe resource and we split it into per-port-group
// fixed size chunks for further flexible carving.
static bf_tm_status_t bf_tm_tof3_alloc_hq(bf_dev_id_t devid,
                                          bf_tm_port_t *p,
                                          bf_tm_eg_q_t *q,
                                          bf_tm_queue_t q_num,
                                          uint8_t hq_per_vq) {
  uint16_t hq_map[BF_TM_TOF3_HW_QUEUES_PER_PG];
  uint32_t base_hq = 0;
  uint16_t hq_q = 0;
  uint16_t pg_q = 0;

  uint8_t hq_per_vq_old = q->hq_per_vq;  // keep to recover on error

  bf_tm_eg_q_t *pg_queues = BF_TM_FIRST_Q_PTR_IN_PG(
      g_tm_ctx[devid], MAKE_DEV_PORT(p->l_pipe, p->uport));

  for (hq_q = 0; hq_q < BF_TM_TOF3_HW_QUEUES_PER_PG; hq_q++) {
    hq_map[hq_q] = BF_TM_TOF3_HW_QUEUES_PER_PG;  // N/A
  }

  q->hq_per_vq = 0;  // re-use its queues for allocation

  // Mark each HQ with what VQ it serves.
  for (pg_q = 0; pg_q < BF_TM_TOF3_QUEUES_PER_PG; pg_q++) {
    for (hq_q = 0;
         hq_q < BF_TM_TOF3_HW_QUEUES_PER_PG && hq_q < pg_queues[pg_q].hq_per_vq;
         hq_q++) {
      if (pg_queues[pg_q].hq_base < (q->pg * BF_TM_TOF3_HW_QUEUES_PER_PG)) {
        q->hq_per_vq = hq_per_vq_old;  // recover the shadow value
        return (BF_INTERNAL_ERROR);
      }
      hq_map[pg_queues[pg_q].hq_base - (q->pg * BF_TM_TOF3_HW_QUEUES_PER_PG) +
             hq_q] = pg_q;
    }
  }

  // Search the first chunk with enough unallocated HQs
  base_hq = BF_TM_TOF3_HW_QUEUES_PER_PG;  // N/A value
  uint16_t hq_cnt = 0;
  uint16_t hq_free = 0;

  for (hq_q = 0; hq_q < BF_TM_TOF3_HW_QUEUES_PER_PG && hq_cnt < hq_per_vq;
       hq_q++) {
    if (hq_map[hq_q] == BF_TM_TOF3_HW_QUEUES_PER_PG) {  // got unassigned HQ
      if (base_hq == BF_TM_TOF3_HW_QUEUES_PER_PG) {     // start of the chunk
        base_hq = hq_q;
      }
      hq_cnt++;
      hq_free++;
    } else {  // got an allocated chunk, continue searching
      base_hq = BF_TM_TOF3_HW_QUEUES_PER_PG;  // N/A
      hq_cnt = 0;
    }
  }

  if (hq_cnt != hq_per_vq) {
    LOG_ERROR(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
        "l_port:%d as queue_nr:%d at speed:%d - can't allocate %d HQs having "
        "%d unassigned.",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->physical_q,
        p->port,
        q_num,
        p->speed_on_add,
        hq_per_vq,
        hq_free);
    q->hq_per_vq = hq_per_vq_old;  // recover the shadow value
    return (BF_NO_SYS_RESOURCES);
  }
  LOG_DBG(
      "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
      "l_port:%d as queue_nr:%d at speed:%d new pg_base_hq:%d:%d",
      devid,
      q->l_pipe,
      q->pg,
      q->logical_q,
      q->physical_q,
      p->port,
      q_num,
      p->speed_on_add,
      base_hq,
      hq_per_vq);

  base_hq += (q->pg * BF_TM_TOF3_HW_QUEUES_PER_PG);  // to the pipe scope

  q->hq_base = base_hq;
  q->hq_per_vq = hq_per_vq;

  return (BF_SUCCESS);
}

bf_tm_status_t bf_tm_tof3_vq_hq_map(bf_dev_id_t devid,
                                    bf_tm_port_t *p,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_queue_t q_num,
                                    bool reset) {
  uint8_t chn_cnt = BF_TM_TOF3_PORTS_PER_PG;  // 4
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t hq_per_vq = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  // On TF3 each HQ is capable to serve at max 25G speed.
  // Each VQ must have enough HQs to serve at the port scheduling speed.
  switch (p->speed_on_add) {
    case BF_SPEED_NONE:
      chn_cnt = 1;
      reset = true;
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      chn_cnt = 1;
      hq_per_vq = 2;
      break;
    case BF_SPEED_100G:
      chn_cnt = 1;
      hq_per_vq = 4;
      break;
    case BF_SPEED_200G:
      chn_cnt = 2;
      hq_per_vq = 8;
      break;
    case BF_SPEED_400G:
      chn_cnt = BF_TM_TOF3_PORTS_PER_PG;
      hq_per_vq = 16;  // each VQ must serve at 400G (16 * 25G)
      break;
    default:
      return (BF_INVALID_ARG);
  }

  if ((p->l_pipe != q->l_pipe) || (q->pg >= BF_TM_TOF3_PG_PER_PIPE)) {
    rc = BF_UNEXPECTED;
  }

  if (reset && (q->port < p->port)) {
    /* This might happen when port group remapping is ongoing with
       port N already carved, its Q profile changed, but the next port N+1 still
       has its old profile with queues now assigned to port N, so we don't want
       to reset these taken the remapping is going as advised: ascending from
       lowest port to highest in the group.
    */
    LOG_TRACE(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d l_port:%d already carved, "
        "skip reset for l_pipe:%d l_port:%d",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->port,
        p->l_pipe,
        p->port);
    return (BF_SUCCESS);
  }

  if (p->port != q->port) {
    LOG_ERROR(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d l_port:%d is not carved to "
        "l_pipe:%d l_port:%d",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->port,
        p->l_pipe,
        p->port);
    return (BF_UNEXPECTED);
  }

  if (p->port % chn_cnt != 0) {
    LOG_DBG(
        "devid:%d l_pipe:%d l_port:%d is a subchannel at speed:%d so detach "
        "its queue_nr:%d carved to pg_id:%d pg_queue:%d from HW",
        devid,
        p->l_pipe,
        p->port,
        p->speed_on_add,
        q_num,
        q->pg,
        q->logical_q);
    reset = true;
  }

  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = 0;

  if (reset) {
    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
      indir_addr =
          tof3_mem_tm_tm_qlc_qlc_mem_csr_memory_qlc_vq(d_pipe, q->physical_q);
      rc |= bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
    }
    if (BF_TM_IS_NOTOK(rc) || q->hq_per_vq > 0) {
      LOG_TRACE(
          "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
          "l_port:%d as queue_nr:%d detached %d HQs, rc=%d",
          devid,
          q->l_pipe,
          q->pg,
          q->logical_q,
          q->physical_q,
          p->port,
          q_num,
          q->hq_per_vq,
          rc);
    }
    q->hq_base = q->physical_q;
    q->hq_per_vq = 0;

    return ((BF_TM_IS_NOTOK(rc)) ? BF_HW_UPDATE_FAILED : rc);
  }

  uint8_t hq_per_vq_old = q->hq_per_vq;

  if (hq_per_vq_old == hq_per_vq) {
    LOG_DBG(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
        "l_port:%d as queue_nr:%d has already allocated HQ:%d:%d",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->physical_q,
        p->port,
        q_num,
        q->hq_base,
        q->hq_per_vq);
    return (BF_SUCCESS);
  } else if (hq_per_vq_old < hq_per_vq) {
    // Need to allocate with more HQs.
    LOG_DBG(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
        "l_port:%d as queue_nr:%d re-allocate %d from HQ:%d:%d",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->physical_q,
        p->port,
        q_num,
        hq_per_vq,
        q->hq_base,
        q->hq_per_vq);
    rc = bf_tm_tof3_alloc_hq(devid, p, q, q_num, hq_per_vq);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
  } else {
    // Keep base HQ with less HQs
    LOG_DBG(
        "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
        "l_port:%d as queue_nr:%d same base less HQ:%d:%d(-%d)",
        devid,
        q->l_pipe,
        q->pg,
        q->logical_q,
        q->physical_q,
        p->port,
        q_num,
        q->hq_base,
        q->hq_per_vq,
        hq_per_vq_old - hq_per_vq);
    q->hq_per_vq = hq_per_vq;
  }

  setp_tof3_qlc_vq_entry_base(&lo, q->hq_base);
  // Encode cap in register: hq_per_vq == 2 ^ (hq_per_vq_enc + 1)
  setp_tof3_qlc_vq_entry_cap(&lo, (hq_per_vq >> 1) - 1);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    indir_addr =
        tof3_mem_tm_tm_qlc_qlc_mem_csr_memory_qlc_vq(d_pipe, q->physical_q);
    rc |= bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  }
  LOG_TRACE(
      "devid:%d l_pipe:%d pg_id:%d pg_queue:%d ph_queue:%d carved to "
      "l_port:%d as queue_nr:%d at speed:%d %s HQ:%d:%d, rc=%d",
      devid,
      q->l_pipe,
      q->pg,
      q->logical_q,
      q->physical_q,
      p->port,
      q_num,
      p->speed_on_add,
      (hq_per_vq_old < q->hq_per_vq) ? "allocate" : "reuse",
      q->hq_base,
      q->hq_per_vq,
      rc);
  if (BF_TM_IS_NOTOK(rc)) {
    return (BF_HW_UPDATE_FAILED);
  }

  return (rc);
}

static uint8_t bf_tm_get_qoc_queue_cfg(uint8_t num_hq_per_vq) {
  switch (num_hq_per_vq) {
    case 0:
      return (0);
    case 2:
      return (0);
    case 4:
      return (1);
    case 8:
      return (2);
    case 16:
      return (3);
    default:
      LOG_ERROR("bf_tm_get_qoc_queue_cfg::passed Invalid num_hq_per_vq %d",
                num_hq_per_vq);
  }
  return (0);
}

bf_tm_status_t bf_tm_init_qoc_for_port(bf_dev_id_t devid,
                                       bf_tm_port_t *p,
                                       uint8_t q_count) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  bf_dev_port_t port;

  if (q_count == 0) {
    return rc;
  }

  uint32_t queue_cfg = 0;
  uint8_t q_cfg = 0;
  uint32_t q_cfg_bits = 0;
  uint32_t q_cfg_bit_mask = 0;
  uint32_t queue_base = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  // port = lld_sku_map_devport_from_device_to_user(devid, port);

  for (q = 0; q < q_count; q++) {
    rc = bf_tm_q_get_descriptor(devid, port, q, &queue);
    if (rc != BF_SUCCESS) {
      return rc;
    }

    if (q == 0) {
      queue_base = queue->physical_q / 16;
      queue_cfg = 0;

      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[d_pipe]
                       .cfg_queue[queue_base]),
          &queue_cfg);

      if (rc != BF_SUCCESS) {
        return rc;
      }

    } else if (queue->physical_q % 16 == 0) {
      rc = bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[d_pipe]
                       .cfg_queue[queue_base]),
          queue_cfg);

      if (rc != BF_SUCCESS) {
        return rc;
      }

      queue_base = queue->physical_q / 16;
      queue_cfg = 0;

      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qoc_top.qoc[d_pipe]
                       .cfg_queue[queue_base]),
          &queue_cfg);

      if (rc != BF_SUCCESS) {
        return rc;
      }
    }

    q_cfg = bf_tm_get_qoc_queue_cfg(queue->hq_per_vq);
    q_cfg_bits = (q_cfg << ((queue->physical_q % 16) * 2));
    q_cfg_bit_mask = (0x3 << ((queue->physical_q % 16) * 2));
    queue_cfg = ((queue_cfg & ~q_cfg_bit_mask) | q_cfg_bits);
  }

  // Write the remaining config.
  if (queue_base != 0) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(
            tof3_reg,
            device_select.tm_top.tm_qoc_top.qoc[d_pipe].cfg_queue[queue_base]),
        queue_cfg);

    if (rc != BF_SUCCESS) {
      return rc;
    }
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_q_carve_queues(bf_dev_id_t devid,
                                         bf_dev_port_t port,
                                         bf_dev_pipe_t physical_pipe,
                                         int q_profile_indx,
                                         bf_tm_q_profile_t *q_profile) {
  // void this to keep the function signature the same
  (void)q_profile_indx;

  bf_tm_status_t rc = BF_TM_EOK;
  int j;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  bf_dev_pipe_t pipe = physical_pipe;
  int lport = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_user_to_device(devid, port));
  uint32_t mem_idx;
  uint8_t mem_entry_idx, lohi_idx;
  // 4 queues per entry, 8 entries per memory
  uint64_t lo[BF_TM_TOF3_QUEUES_PER_PG / Q_PER_EG_MEMORY];
  uint64_t indir_addr;

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG / Q_PER_EG_MEMORY; j++) {
    lo[j] = 0;
  }

  // fill the map in blocks of 4 queues
  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG / 4; j++) {
    mem_entry_idx = j % 8;
    lohi_idx = j / 8;
    lo[lohi_idx] |= ((q_profile->q_mapping[j * 4] + q_profile->base_q) >> 2)
                    << (mem_entry_idx * 4);
  }
  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG; j += Q_PER_EG_MEMORY) {
    // SRAM address {eport[5:0], ing_qid[5]}
    mem_idx = (lport << 1) + (j >> 5);

    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, d_inst);

      indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_qid_mapping(
          d_pipe, mem_idx);
      rc |= bf_tm_subdev_write_memory(
          devid, subdev_id, indir_addr, 8, 0, lo[j / Q_PER_EG_MEMORY]);
    }
  }

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG / Q_PER_EG_MEMORY; j++) {
    lo[j] = 0;
  }

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG / 4; j++) {
    mem_entry_idx = j % 16;
    lohi_idx = j / 16;
    lo[lohi_idx] |= ((q_profile->q_mapping[j * 4] + q_profile->base_q) >> 2)
                    << (mem_entry_idx * 4);
  }

  for (uint32_t egress_pipe = 0; egress_pipe < BF_TM_TOF3_EG_PIPES;
       egress_pipe++) {
    // SRAM address {eport[5:0], epipe[2:0]}
    mem_idx = (lport << 3) + egress_pipe;
    indir_addr =
        tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qid_map(0, mem_idx);
    rc |= bf_tm_subdev_write_memory(
        devid, (bf_subdev_id_t)(pipe > 3), indir_addr, 8, 0, lo[0]);
  }

  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("dev:%d dev_port:%d failed to set queue map", devid, port);
    return (BF_HW_UPDATE_FAILED);
  }

  // Assign hw queues
  bf_tm_eg_q_t *q;
  bf_tm_port_t *p;
  rc = bf_tm_port_get_descriptor(devid, port, &p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "%s: TM: Failed to get port descriptor for port %d", __func__, port);
    return rc;
  }

  for (int qid = 0; qid < q_profile->q_count; qid++) {
    rc = bf_tm_q_get_descriptor(devid, port, qid, &q);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("%s: TM: Failed to get queue descriptor for port %d, queue %d",
                __func__,
                port,
                qid);
      return rc;
    }

    rc = bf_tm_tof3_vq_hq_map(devid, p, q, qid, false);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "Failed to set VQ to HQ map on dev:%d, port:%d, queue:%d, rc=%d",
          devid,
          port,
          qid,
          rc);
      return rc;
    }
  }

  // Update qoc for the port
  rc = bf_tm_init_qoc_for_port(devid, p, q_profile->q_count);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("%s: TM: Failed to update qoc for port %d", __func__, port);
    return rc;
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_release_queues(bf_dev_id_t devid,
                                           bf_dev_port_t port,
                                           bf_dev_pipe_t physical_pipe,
                                           int profile_idx,
                                           bf_tm_q_profile_t *q_profile) {
  bf_tm_eg_q_t *q = NULL;
  bf_tm_port_t *p = NULL;
  (void)physical_pipe;

  bf_tm_status_t rc = bf_tm_port_get_descriptor(devid, port, &p);
  if (BF_TM_IS_NOTOK(rc)) {
    return rc;
  }

  for (int qid = 0; qid < q_profile->q_count; qid++) {
    // Do not use bf_tm_q_get_descriptor() as it takes old profile from the
    // port.
    q = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[devid], port) + q_profile->base_q +
        qid;

    rc = bf_tm_tof3_vq_hq_map(devid, p, q, qid, true);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "Failed to reset VQ to HQ map on dev:%d, dev_port:%d, "
          "q_profile:%d(ch:%d,base:%d,cnt:%d), queue_nr:%d, rc=%d",
          devid,
          port,
          profile_idx,
          q_profile->ch_in_pg,
          q_profile->base_q,
          q_profile->q_count,
          qid,
          rc);
      return rc;
    }
  }
  return (rc);
}

// qid_map_ingress_mau_to_tm(port_9b, qid_6b) -> pipe_2, qid_10b
// Get mapping from devport {log_pipe_2b, port_7b} and ingress qid{qid_6b}
// to physical pipe {p_pipe_2b} and physical queue {phys_q_10b}
bf_tm_status_t bf_tm_tof3_get_physical_q(bf_dev_id_t devid,
                                         bf_dev_port_t devport,
                                         uint32_t ing_q,
                                         bf_dev_pipe_t *log_pipe,
                                         bf_tm_queue_t *phys_q) {
  bf_tm_status_t rc = BF_TM_EOK;

  // determine phys/log pipe numbers
  *log_pipe = DEV_PORT_TO_PIPE(devport);
  bf_dev_pipe_t phys_pipe = 0;
  rc |= lld_sku_map_pipe_id_to_phy_pipe_id(devid, *log_pipe, &phys_pipe);
  uint32_t port = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_user_to_device(devid, devport));

  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(phys_pipe, 0);

  // determine phys_q number

  // SRAM address {eport[5:0], epipe[2:0]}
  uint32_t mem_idx = (port << 3) + phys_pipe;

  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qid_map(0, mem_idx);
  rc |= bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  // calculate q number
  // q_mid_idx inside one row q_mid[qid[5:2]]
  uint8_t q_mid_idx = ((ing_q >> 2) & 0xfu);
  // offset iside 64 bits row (qid_mid[4 bits])
  uint8_t word_shift = q_mid_idx * 4;
  // find and extract 4-bit value qid_mid
  uint8_t q_mid = 0;
  q_mid = (lo >> word_shift) & 0xf;

  // the final qid :{mac[3:0],      qid_mid[3:0],   ing_qid[1:0]}
  *phys_q = ((port & 0xf) << 6) + (q_mid << 2) + (ing_q & 0x3);

  return rc;
}

bf_tm_status_t bf_tm_tof3_get_q_profiles_mapping(bf_dev_id_t devid,
                                                 bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t num_p_pipes = 0;
  bf_dev_pipe_t p_pipe = 0;
  lld_err_t lld_err = lld_sku_get_num_active_pipes(devid, &num_p_pipes);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Dev %d: Can't get the number of physical pipes, lld_rc=%d",
              devid,
              lld_err);
    return (BF_INTERNAL_ERROR);
  }

  for (bf_dev_pipe_t l_pipe = 0; l_pipe < num_p_pipes; l_pipe++) {
    lld_err = lld_sku_map_pipe_id_to_phy_pipe_id(devid, l_pipe, &p_pipe);
    if (LLD_OK != lld_err) {
      LOG_ERROR("Dev %d: Cannot map l_pipe %d to a physical pipe, lld_rc=%d",
                devid,
                l_pipe,
                lld_err);
      return (BF_INTERNAL_ERROR);
    }

    for (uint32_t i = 0; i < BF_TM_TOF3_Q_PROF_CNT / BF_TM_TOF3_MAU_PIPES;
         i++) {
      uint32_t ports_per_pipe =
          BF_TM_TOF3_PG_PER_PIPE * BF_TM_TOF3_PORTS_PER_PG;
      uint32_t q_prof_idx = (ports_per_pipe * p_pipe) + i;
      rc = bf_tm_tof3_get_q_profile_mapping(
          devid, p_pipe, i, &(q_profile[q_prof_idx]));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "%s: Unable to restore queue profile mapping for dev %d pipe "
            "%d q profile idx %d, rc %s "
            "(%d)",
            __func__,
            devid,
            p_pipe,
            q_prof_idx,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }

  return rc;
}

bf_tm_status_t bf_tm_tof3_get_q_profile_mapping(bf_dev_id_t devid,
                                                int physical_pipe,
                                                int q_profile_indx,
                                                bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mem_idx;
  uint8_t mem_entry_idx, lohi_idx;
  int j;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(physical_pipe, devid);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(physical_pipe, devid);

  // 4 queues per entry, 8 entries per memory
  uint64_t hi;
  uint64_t lo[BF_TM_TOF3_QUEUES_PER_PG / Q_PER_EG_MEMORY];
  uint64_t indir_addr;

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG / Q_PER_EG_MEMORY; j++) {
    lo[j] = 0;
  }

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG; j += Q_PER_EG_MEMORY) {
    // SRAM address {eport[5:0], ing_qid[5]}
    mem_idx = (q_profile_indx << 1) + (j >> 4);
    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_qid_mapping(
        d_pipe, mem_idx);
    rc = bf_tm_subdev_read_memory(
        devid, subdev_id, indir_addr, &hi, &lo[j / Q_PER_EG_MEMORY]);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "%s: Qid mapping read error for dev %d pipe %d memory idx %d, rc %s "
          "(%d)",
          __func__,
          devid,
          d_pipe,
          mem_idx,
          bf_err_str(rc),
          rc);
      return rc;
    }
  }

  for (j = 0; j < BF_TM_TOF3_QUEUES_PER_PG; j++) {
    lohi_idx = j / 32;
    mem_entry_idx = (j >> 2) & 7;
    q_profile->q_mapping[j] =
        ((((lo[lohi_idx] >> (mem_entry_idx * 4)) & 0xF) % 4) << 2) + (j % 4);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_get_port_q_profile(bf_dev_id_t devid,
                                             bf_dev_port_t port,
                                             bf_dev_pipe_t physical_pipe,
                                             uint32_t *q_profile_indx) {
  (void)devid;
  bf_tm_status_t rc = BF_TM_EOK;
  int lport = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_user_to_device(devid, port));

  // On Tofino-3, there is no concept of queue profiles, so each port can have
  // it's own mapping which needs to be restored.
  uint32_t ports_per_pipe = BF_TM_TOF3_PG_PER_PIPE * BF_TM_TOF3_PORTS_PER_PG;
  *q_profile_indx = (ports_per_pipe * physical_pipe) + lport;

  return (rc);
}

// QAC qac_queue_config_entry_shr_cfg entries
static inline uint8_t
getp_tof3_qac_queue_config_entry_shr_cfg_fast_recover_mode(void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 25) & 0x1ull));
}
static inline void setp_tof3_qac_queue_config_entry_shr_cfg_fast_recover_mode(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0x1ull) << 25) | (*((uint64_t *)csr) & 0xfffffffffdffffffull);
}
/* Field member: qac_queue_shr_thrd_config_entry_dyn_en                    */
static inline uint8_t getp_tof3_qac_queue_config_entry_shr_cfg_dyn_en(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 24) & 0x1ull));
}
static inline void setp_tof3_qac_queue_config_entry_shr_cfg_dyn_en(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0x1ull) << 24) | (*((uint64_t *)csr) & 0xfffffffffeffffffull);
}
/* Field member: qac_queue_shr_thrd_config_entry_offset_idx                */
static inline uint8_t getp_tof3_qac_queue_config_entry_shr_cfg_offset_idx(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 19) & 0x1full));
}
static inline void setp_tof3_qac_queue_config_entry_shr_cfg_offset_idx(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0x1full) << 19) | (*((uint64_t *)csr) & 0xffffffffff07ffffull);
}
/* Field member: qac_queue_shr_thrd_config_entry_shr_lmt                   */
static inline uint32_t getp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(
    void *csr) {
  return ((uint32_t)((*((uint64_t *)csr)) & 0x7ffffull));
}
static inline void setp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      (value & 0x7ffffull) | (*((uint64_t *)csr) & 0xfffffffffff80000ull);
}

bf_tm_status_t bf_tm_tof3_q_set_min_limit(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t data[2] = {0};
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);

  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  setp_tof3_qac_queue_config_entry_min_lmt(&data[0], q->thresholds.min_limit);

#if DEVICE_IS_EMULATOR
  /* lock min threshold to 500 for emulation tests */
  setp_tof3_qac_queue_config_entry_min_lmt(&data[0], 500);
#endif

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
        d_pipe, q->physical_q);

    rc |= bf_tm_subdev_write_memory(
        devid, subdev_id, indir_addr, 16, data[1], data[0]);
  }

  return (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
}

bf_tm_status_t bf_tm_tof3_q_get_min_limit(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t data[2] = {0};

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);

  q->thresholds.min_limit = getp_tof3_qac_queue_config_entry_min_lmt(&data[0]);
  return (rc);
}

// tof3_reg implements an array of 32 entries to specify hysteresis values.
// Instead of programming actual hysteresis value in tables, index of the
// 32 entry table is used. This index is refered as offset_idx in register
// spec. QAC offset proile table is used for HYST on Queue and Port.
// bf_tm_tof3_restore_qac_offset_profile() can be used to restore
// the following 2 SW tables/variables for hitless HA case.

// TODO: At some point all the offset_profiles will be occupied with some
// values which actually might be not used by any Port, Queue.
// Some reference counting and housekeeping is needed to reuse
// this limited resource.

static uint8_t qac_offset_profile_cnt[BF_TM_NUM_ASIC][BF_TM_TOF3_MAU_PIPES] = {
    {0}};
static uint16_t qac_offset_profile_tbl[BF_TM_NUM_ASIC][BF_TM_TOF3_MAU_PIPES]
                                      [BF_TM_TOF3_HYSTERESIS_PROFILES] = {
                                          {{0}}};

bf_status_t bf_tm_tof3_restore_qac_offset_profile(bf_dev_id_t dev) {
  uint32_t val;
  uint32_t num_p_pipes = 0;
  bf_subdev_id_t subdev_id;
  bf_dev_pipe_t d_pipe;

  g_tm_ctx[dev]->read_por_qac_profile_table = false;

  // Clear all the array, even if some pipes ara not used on the device.
  for (int p = 0; p < BF_TM_TOF3_MAU_PIPES; p++) {
    qac_offset_profile_cnt[dev][p] = 0;
  }

  lld_err_t lld_err = lld_sku_get_num_active_pipes(dev, &num_p_pipes);
  if ((LLD_OK != lld_err) || (num_p_pipes > BF_TM_TOF3_MAU_PIPES)) {
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
    subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, 0);
    d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, 0);

    for (int i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
      (void)bf_tm_subdev_read_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.offset_profile[i]),
          &val);
      LOG_TRACE("Restore dev %d:%d l_pipe %d p_pipe %d QAC hyst[%d]=0x%x",
                dev,
                subdev_id,
                l_pipe,
                p_pipe,
                i,
                val);
      // Ignore SW model garbage
      val = (TM_IS_TARGET_ASIC(dev)) ? val : BF_TM_TOF3_QAC_RESET_HYSTERESIS;
      qac_offset_profile_tbl[dev][p_pipe][i] = val;
      // Treat default values as empty items except the first one.
      // It might be at any index in the array.
      // Non-POR HW reset values are counted as empty items and will be reused
      // except the first one greate than POR.
      if ((got_reset_value && val == BF_TM_TOF3_QAC_RESET_HYSTERESIS) ||
          (got_default_value && val <= BF_TM_TOF3_QAC_POR_HYSTERESIS)) {
        continue;
      }

      qac_offset_profile_cnt[dev][p_pipe]++;
      if (val <= BF_TM_TOF3_QAC_POR_HYSTERESIS) {
        got_default_value = true;
      }
      if (val == BF_TM_TOF3_QAC_RESET_HYSTERESIS) {
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

static uint8_t bf_tm_tof3_get_qac_offset_profile_index(bf_dev_id_t dev,
                                                       bf_tm_thres_t cells_u,
                                                       uint8_t p_pipe) {
  bf_status_t rc = BF_SUCCESS;
  int i;

  // search from the start till end to check if cell_limit is already
  // in profile table.
  if (!g_tm_ctx[dev]->read_por_qac_profile_table) {
#ifndef BF_TM_HITLESS_HA_TESTING_WITH_MODEL
    rc = bf_tm_tof3_restore_qac_offset_profile(dev);
    if (BF_SUCCESS != rc) {
      return (BF_TM_TOF3_HYSTERESIS_PROFILES);
    }
#else
    LOG_TRACE("clear all QAC hyst profiles dev %d", dev);
    for (int p = 0; p < BF_TM_TOF3_MAU_PIPES; p++) {
      qac_offset_profile_cnt[dev][p] = 0;
      for (i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
        qac_offset_profile_tbl[dev][p][i] = BF_TM_TOF3_QAC_RESET_HYSTERESIS;
      }
    }
    g_tm_ctx[dev]->read_por_qac_profile_table = true;
#endif
  }

  /* For the hysteresis values, check if it is configured already */
  for (i = 0; i < BF_TM_TOF3_HYSTERESIS_PROFILES; i++) {
    if (qac_offset_profile_tbl[dev][p_pipe][i] == cells_u) {
      return (i);
    }
  }
  return (BF_TM_TOF3_HYSTERESIS_PROFILES);
}

static bf_status_t bf_tm_tof3_check_qac_offset_profile(bf_dev_id_t dev,
                                                       uint8_t p_pipe,
                                                       bf_tm_thres_t cells_u,
                                                       uint8_t idx_expected) {
  int idx = bf_tm_tof3_get_qac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOF3_HYSTERESIS_PROFILES) {
    // Check the expected index as well
    if ((BF_TM_TOF3_HYSTERESIS_PROFILES != idx_expected) &&
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

static bf_status_t bf_tm_tof3_populate_qac_offset_profile(bf_dev_id_t dev,
                                                          uint8_t p_pipe,
                                                          bf_tm_thres_t cells,
                                                          uint8_t *index) {
  int idx;
  bf_status_t rc = BF_SUCCESS;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, 0);
  uint32_t d_inst;
  uint32_t cells_u = TM_CELLS_TO_8CELL_UNITS(cells);

  // For dynamic sharing to work, hysteresis shouldn't be
  // 0 - the reason why POR value is 4 in 8 cells unit i.e 32.
  // Add default item if it is not yet in the table.
  if (cells_u < BF_TM_TOF3_QAC_POR_HYSTERESIS) {
    cells_u = BF_TM_TOF3_QAC_POR_HYSTERESIS;
  }

  idx = bf_tm_tof3_get_qac_offset_profile_index(dev, cells_u, p_pipe);
  if (idx < BF_TM_TOF3_HYSTERESIS_PROFILES) {
    *index = idx;
    return (rc);
  }
  if (qac_offset_profile_cnt[dev][p_pipe] >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
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
      for (idx = 0; idx < BF_TM_TOF3_HYSTERESIS_PROFILES; idx++) {
        if (qac_offset_profile_tbl[dev][p_pipe][idx] <=
            BF_TM_TOF3_QAC_POR_HYSTERESIS) {
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
                   BF_TM_TOF3_QAC_RESET_HYSTERESIS) {
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
      if (idx >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
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

    for (d_inst = 0; d_inst < g_tm_ctx[dev]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(p_pipe, d_inst);

      // Write to HW
      (void)bf_tm_subdev_write_register(
          dev,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.offset_profile[*index]),
          cells_u);
    }
    qac_offset_profile_cnt[dev][p_pipe]++;
  }
  return (rc);
}

/* Multi field register ; register value preparation function */
inline static bf_status_t bf_tm_prep_q_shr_limit_reg_val(bf_dev_id_t dev,
                                                         uint64_t *val,
                                                         bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  setp_tof3_qac_queue_config_entry_shr_cfg_fast_recover_mode(
      val, (q->q_cfg.fast_recover_mode) ? 1 : 0);
  setp_tof3_qac_queue_config_entry_shr_cfg_dyn_en(
      val, (q->q_cfg.is_dynamic) ? 1 : 0);

  rc = bf_tm_tof3_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.app_hyst, &(q->thresholds.app_hyst_index));
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  setp_tof3_qac_queue_config_entry_shr_cfg_offset_idx(
      val, q->thresholds.app_hyst_index);
  if (q->q_cfg.is_dynamic) {
    // Fold in BAF
    setp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(
        val, BF_TM_Q_FOLD_THRES_LIMIT(q->thresholds.app_limit, q->q_cfg.baf));
  } else {
    setp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(val,
                                                     q->thresholds.app_limit);
  }
  return (rc);
}

/* Multi field register ; register set function */
inline static bf_tm_status_t bf_tm_tof3_q_set_shared_lmt_reg(bf_dev_id_t devid,
                                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0, data[2] = {0};
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);

  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  rc = bf_tm_prep_q_shr_limit_reg_val(devid, &val, q);
  if (rc == BF_SUCCESS) {
    setp_tof3_qac_queue_config_entry_shr_cfg(&data[0], val);

    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

      indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);

      rc |= bf_tm_subdev_write_memory(
          devid, subdev_id, indir_addr, 16, data[1], data[0]);
    }
    rc = (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
  }
  return (rc);
}

inline static bf_tm_status_t bf_tm_tof3_q_get_shared_lmt_reg(
    bf_dev_id_t devid, const bf_tm_eg_q_t *q, uint64_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t data[2] = {0};
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);

  *val = getp_tof3_qac_queue_config_entry_shr_cfg(&data[0]);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_app_limit(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_app_limit(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_shared_lmt_reg(devid, q, &val);
  q->thresholds.app_limit =
      getp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(&val);
  if (q->q_cfg.is_dynamic) {
    q->thresholds.app_limit = BF_TM_Q_THRES_LIMIT(q->thresholds.app_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_is_dynamic(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tof3_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_is_dynamic(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_shared_lmt_reg(devid, q, &val);
  q->q_cfg.is_dynamic = getp_tof3_qac_queue_config_entry_shr_cfg_dyn_en(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_baf(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tof3_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_baf(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_shared_lmt_reg(devid, q, &val);
  q->thresholds.app_limit =
      getp_tof3_qac_queue_config_entry_shr_cfg_shr_lmt(&val);
  if (q->q_cfg.is_dynamic) {
    q->q_cfg.baf = BF_TM_Q_BAF(q->thresholds.app_limit);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_app_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_tof3_q_set_shared_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_app_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;
  uint32_t cell_limit;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_tof3_q_get_shared_lmt_reg(devid, q, &val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get queue shared limit");
    return rc;
  }
  q->thresholds.app_hyst_index =
      getp_tof3_qac_queue_config_entry_shr_cfg_offset_idx(&val);
  if (q->thresholds.yel_hyst_index >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.offset_profile[q->thresholds.app_hyst_index]),
      &cell_limit);
  q->thresholds.app_hyst = (cell_limit << 3);
  return (rc);
}

// QAC qac_queue_config_entry_color_cfg entries
static inline uint8_t getp_tof3_qac_queue_config_entry_color_cfg_yel_offset_idx(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 12) & 0xfull));
}
static inline void setp_tof3_qac_queue_config_entry_color_cfg_yel_offset_idx(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0xfull) << 12) | (*((uint64_t *)csr) & 0xffffffffffff0fffull);
}
/* Field member: qac_queue_color_limit_entry_yel_lmt_perc                  */
static inline uint8_t getp_tof3_qac_queue_config_entry_color_cfg_yel_lmt_perc(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 8) & 0xfull));
}
static inline void setp_tof3_qac_queue_config_entry_color_cfg_yel_lmt_perc(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0xfull) << 8) | (*((uint64_t *)csr) & 0xfffffffffffff0ffull);
}
/* Field member: qac_queue_color_limit_entry_red_offset_idx                */
static inline uint8_t getp_tof3_qac_queue_config_entry_color_cfg_red_offset_idx(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 4) & 0xfull));
}
static inline void setp_tof3_qac_queue_config_entry_color_cfg_red_offset_idx(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0xfull) << 4) | (*((uint64_t *)csr) & 0xffffffffffffff0full);
}
/* Field member: qac_queue_color_limit_entry_red_lmt_perc                  */
static inline uint8_t getp_tof3_qac_queue_config_entry_color_cfg_red_lmt_perc(
    void *csr) {
  return ((uint8_t)((*((uint64_t *)csr)) & 0xfull));
}
static inline void setp_tof3_qac_queue_config_entry_color_cfg_red_lmt_perc(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      (value & 0xfull) | (*((uint64_t *)csr) & 0xfffffffffffffff0ull);
}

inline static void bf_tm_prep_q_color_limit_reg_val(bf_dev_id_t dev,
                                                    uint64_t *val,
                                                    bf_tm_eg_q_t *q) {
  setp_tof3_qac_queue_config_entry_color_cfg_yel_lmt_perc(
      val, q->thresholds.yel_limit_pcent);
  setp_tof3_qac_queue_config_entry_color_cfg_red_lmt_perc(
      val, q->thresholds.red_limit_pcent);
  bf_tm_tof3_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.red_hyst, &(q->thresholds.red_hyst_index));
  bf_tm_tof3_populate_qac_offset_profile(
      dev, q->p_pipe, q->thresholds.yel_hyst, &(q->thresholds.yel_hyst_index));
  setp_tof3_qac_queue_config_entry_color_cfg_yel_offset_idx(
      val, q->thresholds.yel_hyst_index);
  setp_tof3_qac_queue_config_entry_color_cfg_red_offset_idx(
      val, q->thresholds.red_hyst_index);
}

inline static bf_tm_status_t bf_tm_tof3_q_set_color_lmt_reg(bf_dev_id_t devid,
                                                            bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0, data[2] = {0};
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);

  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);

  if (rc != BF_SUCCESS) {
    return rc;
  }

  bf_tm_prep_q_color_limit_reg_val(devid, &val, q);
  setp_tof3_qac_queue_config_entry_color_cfg(&data[0], val);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
        d_pipe, q->physical_q);

    rc |= bf_tm_subdev_write_memory(
        devid, subdev_id, indir_addr, 16, data[1], data[0]);
  }

  return (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
}

inline static bf_tm_status_t bf_tm_tof3_q_get_color_lmt_reg(
    bf_dev_id_t devid, const bf_tm_eg_q_t *q, uint64_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t data[2] = {0};

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);
  *val = getp_tof3_qac_queue_config_entry_color_cfg(&data[0]);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_yel_limit_pcent(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_yel_limit_pcent(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  bf_tm_tof3_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.yel_limit_pcent =
      getp_tof3_qac_queue_config_entry_color_cfg_yel_lmt_perc(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_red_limit_pcent(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_red_limit_pcent(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  bf_tm_tof3_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.red_limit_pcent =
      getp_tof3_qac_queue_config_entry_color_cfg_red_lmt_perc(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_yel_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_yel_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;
  uint32_t cell;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_tof3_q_get_color_lmt_reg(devid, q, &val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("Not able to get queue yellow hyst");
    return rc;
  }
  q->thresholds.yel_hyst_index =
      getp_tof3_qac_queue_config_entry_color_cfg_yel_offset_idx(&val);

  if (q->thresholds.yel_hyst_index >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.offset_profile[q->thresholds.yel_hyst_index]),
      &cell);
  q->thresholds.yel_hyst = (cell << 3);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_red_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_color_lmt_reg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_red_hyst(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;
  uint32_t cell;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  bf_tm_tof3_q_get_color_lmt_reg(devid, q, &val);
  q->thresholds.red_hyst_index =
      getp_tof3_qac_queue_config_entry_color_cfg_red_offset_idx(&val);

  if (q->thresholds.yel_hyst_index >= BF_TM_TOF3_HYSTERESIS_PROFILES) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.offset_profile[q->thresholds.red_hyst_index]),
      &cell);
  q->thresholds.red_hyst = (cell << 3);

  return (rc);
}

// QAC qac_queue_config_entry_ap_config entries
static inline uint8_t getp_tof3_qac_queue_config_entry_ap_config_q_drop_en(
    void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 3) & 0x1ull));
}
static inline void setp_tof3_qac_queue_config_entry_ap_config_q_drop_en(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0x1ull) << 3) | (*((uint64_t *)csr) & 0xfffffffffffffff7ull);
}
/* Field member: qac_queue_ap_config_entry_q_color_drop_en                 */
static inline uint8_t
getp_tof3_qac_queue_config_entry_ap_config_q_color_drop_en(void *csr) {
  return ((uint8_t)(((*((uint64_t *)csr)) >> 2) & 0x1ull));
}
static inline void setp_tof3_qac_queue_config_entry_ap_config_q_color_drop_en(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      ((value & 0x1ull) << 2) | (*((uint64_t *)csr) & 0xfffffffffffffffbull);
}
/* Field member: qac_queue_ap_config_entry_ap_id                           */
static inline uint8_t getp_tof3_qac_queue_config_entry_ap_config_ap_id(
    void *csr) {
  return ((uint8_t)((*((uint64_t *)csr)) & 0x3ull));
}
static inline void setp_tof3_qac_queue_config_entry_ap_config_ap_id(
    void *csr, uint64_t value) {
  *((uint64_t *)csr) =
      (value & 0x3ull) | (*((uint64_t *)csr) & 0xfffffffffffffffcull);
}

inline static void bf_tm_prep_q_pool_cfg(uint64_t *val, bf_tm_eg_q_t *q) {
  setp_tof3_qac_queue_config_entry_ap_config_ap_id(val, q->q_cfg.app_poolid);
  setp_tof3_qac_queue_config_entry_ap_config_q_color_drop_en(
      val, q->q_cfg.color_drop_en);
  setp_tof3_qac_queue_config_entry_ap_config_q_drop_en(val,
                                                       q->q_cfg.tail_drop_en);
}

bf_tm_status_t bf_tm_tof3_q_set_wac_buffer(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  uint64_t indir_addr = 0;

  bf_tm_prep_q_pool_cfg(&val, q);

  // In Tofino3, qacq_ap_config in WAC also needs to be updated accordingly.
  // This is same replica of what should be in QAC.

  // Need to initialize both Die0 and Die1 with same value.
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    indir_addr =
        tof3_mem_tm_tm_wac_wac_common_mem_wac_common_mem_qacq_ap_config(
            d_pipe, q->physical_q);

    rc |= bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, val);
  }

  return (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
}

bf_tm_status_t bf_tm_tof3_q_set_qac_buffer(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0, data[2] = {0};
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);

  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("%s:%d, rc=%d", __func__, __LINE__, rc);
    return rc;
  }

  rc = bf_tm_prep_q_shr_limit_reg_val(devid, &val, q);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("%s:%d, rc=%d", __func__, __LINE__, rc);
    return rc;
  }

  bf_tm_prep_q_pool_cfg(&val, q);
  setp_tof3_qac_queue_config_entry_ap_config(&data[0], val);

  setp_tof3_qac_queue_config_entry_min_lmt(&data[0], q->thresholds.min_limit);

  bf_tm_prep_q_color_limit_reg_val(devid, &val, q);
  setp_tof3_qac_queue_config_entry_color_cfg(&data[0], val);

  rc = bf_tm_prep_q_shr_limit_reg_val(devid, &val, q);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Failed to prepare shared config register for pipe %d queue %d",
        __func__,
        q->p_pipe,
        q->physical_q);
    return rc;
  }
  setp_tof3_qac_queue_config_entry_shr_cfg(&data[0], val);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
        d_pipe, q->physical_q);

    rc |= bf_tm_subdev_write_memory(
        devid, subdev_id, indir_addr, 16, data[1], data[0]);
  }

  return (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
}

inline static bf_tm_status_t bf_tm_tof3_q_set_pool_cfg(bf_dev_id_t devid,
                                                       bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_tof3_q_set_qac_buffer(devid, q);
  rc |= bf_tm_tof3_q_set_wac_buffer(devid, q);

  return (BF_TM_EOK != rc) ? BF_INTERNAL_ERROR : rc;
}

inline static bf_tm_status_t bf_tm_tof3_q_get_pool_cfg(bf_dev_id_t devid,
                                                       const bf_tm_eg_q_t *q,
                                                       uint64_t *val) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t data[2] = {0};
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_config(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(
      devid, subdev_id, indir_addr, &data[1], &data[0]);
  *val = getp_tof3_qac_queue_config_entry_ap_config(&data[0]);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_app_poolid(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_app_poolid(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.app_poolid = getp_tof3_qac_queue_config_entry_ap_config_ap_id(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_color_drop_en(bf_dev_id_t devid,
                                              bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_color_drop_en(bf_dev_id_t devid,
                                              bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.color_drop_en =
      getp_tof3_qac_queue_config_entry_ap_config_q_color_drop_en(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_tail_drop_en(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_tof3_q_set_pool_cfg(devid, q);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_tail_drop_en(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_pool_cfg(devid, q, &val);
  q->q_cfg.tail_drop_en =
      getp_tof3_qac_queue_config_entry_ap_config_q_drop_en(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_visible(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t *q_0 = q - (q->physical_q % g_tm_ctx[devid]->tm_cfg.q_per_pg);
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  uint32_t val_0 = 0;
  uint32_t val_1 = 0;
  for (int i = 0; i < g_tm_ctx[devid]->tm_cfg.q_per_pg; i++) {
    if ((q_0 + i)->q_cfg.visible) {
      uint8_t reg_no = i / 32;
      if (reg_no == 0) {
        val_0 |= 1 << i % 32;
      } else {
        val_1 |= 1 << i % 32;
      }
    }
  }

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_queue_state_visible_bmp[q->pg]
                     .qac_queue_state_visible_bmp_0_2),
        val_0);
    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_queue_state_visible_bmp[q->pg]
                     .qac_queue_state_visible_bmp_1_2),
        val_1);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_visible(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  uint8_t reg_no = (q->physical_q % g_tm_ctx[devid]->tm_cfg.q_per_pg) / 32;
  bf_tm_eg_q_t *q_0 = q - (q->physical_q % 32);
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.qac_queue_state_visible_bmp[q->pg]
                   .qac_queue_state_visible_bmp_0_2),
      &val);
  if (reg_no != 0) {
    if (reg_no == 1) {
      rc |= bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.qac_queue_state_visible_bmp[q->pg]
                       .qac_queue_state_visible_bmp_1_2),
          &val);
    }
  }

  if (rc != BF_TM_EOK) {
    return rc;
  }

  for (int i = 0; i < 32; i++) {
    (q_0 + i)->q_cfg.visible = (val & (1 << i)) > 0;
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_set_neg_mir_dest(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  // use shadow memory as value buffer
  uint32_t *val = &((g_tm_ctx[devid]->pipes + q->l_pipe)->qac_pipe_config);

  setp_tof3_qac_pipe_config_defd_fixed_port(val, q->port);

  // This register keeps port_group 'physical' queue number,
  // neither a pipeline QID, nor port queue_nr.
  setp_tof3_qac_pipe_config_defd_fixed_port_mapped_qid(val, q->logical_q);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.pipe_config),
        *val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_neg_mir_dest(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  // use shadow memory as value buffer
  uint32_t *val = &((g_tm_ctx[devid]->pipes + q->l_pipe)->qac_pipe_config);
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_qac_top.qac_pipe[d_pipe].qac_reg.pipe_config),
      val);
  if (BF_SUCCESS != rc) {
    return (rc);
  }
  q->port = getp_tof3_qac_pipe_config_defd_fixed_port(val);
  q->uport = DEV_PORT_TO_LOCAL_PORT(lld_sku_map_devport_from_device_to_user(
      devid, MAKE_DEV_PORT(q->l_pipe, q->port)));

  // This register keeps port_group 'physical' queue number,
  // neither a pipeline QID, nor port queue_nr.
  uint8_t qid = getp_tof3_qac_pipe_config_defd_fixed_port_mapped_qid(val);
  bf_tm_eg_q_t *qid_q = NULL;

  if (!LOCAL_PORT_VALIDATE(q->uport) || qid >= BF_TM_TOF3_QUEUES_PER_PG) {
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

bf_tm_status_t bf_tm_tof3_q_get_drop_counter(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q,
                                             uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);
  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_queue(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  *count = getp_tof3_qac_drop_count_queue_entry_count(&lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_drop_counter_ext(bf_dev_id_t devid,
                                                 bf_subdev_id_t die_id,
                                                 bf_tm_eg_q_t *q,
                                                 uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_queue(
          q->p_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, die_id, indir_addr, &hi, &lo);
  *count = getp_tof3_qac_drop_count_queue_entry_count(&lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_clear_drop_counter(bf_dev_id_t devid,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_queue(
            d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_usage_counter(bf_dev_id_t devid,
                                              bf_tm_eg_q_t *q,
                                              uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);
  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = getp_tof3_qac_queue_cell_count_entry_queue_cell_count(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_usage_counter_ext(bf_dev_id_t devid,
                                                  bf_subdev_id_t die_id,
                                                  bf_tm_eg_q_t *q,
                                                  uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(
          q->p_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, die_id, indir_addr, &hi, &lo);

  *count = getp_tof3_qac_queue_cell_count_entry_queue_cell_count(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_wm_counter(bf_dev_id_t devid,
                                           bf_tm_eg_q_t *q,
                                           uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);
  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_wm_cell_count(
          d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  *count = getp_tof3_qac_queue_wm_cell_count_entry_cell_count(&lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_wm_counter_ext(bf_dev_id_t devid,
                                               bf_subdev_id_t die_id,
                                               bf_tm_eg_q_t *q,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_wm_cell_count(
          q->p_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, die_id, indir_addr, &hi, &lo);

  *count = getp_tof3_qac_queue_wm_cell_count_entry_cell_count(&lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_clear_wm_counter(bf_dev_id_t devid,
                                             bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);

    setp_tof3_qac_queue_wm_cell_count_entry_cell_count(&lo, 0);
    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_wm_cell_count(
            d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, lo);
  }
  return rc;
}

static bool bf_tm_tof3_q_select_drop_state_for_particular_queue(uint32_t val,
                                                                uint8_t queue) {
  return (val >> (queue % Q_PER_EG_MEMORY)) & 0x1;
}

bf_tm_status_t bf_tm_tof3_q_get_egress_drop_state(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q,
                                                  bf_tm_color_t color,
                                                  bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  uint32_t offset = q->physical_q / Q_PER_EG_MEMORY;
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    switch (color) {
      case BF_TM_COLOR_GREEN:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_state[offset]),
            &val);
        break;
      case BF_TM_COLOR_YELLOW:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_yel_state[offset]),
            &val);
        break;
      case BF_TM_COLOR_RED:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_red_state[offset]),
            &val);
        break;
      default:
        return BF_TM_EINV_ARG;
    }
  }
  *state =
      bf_tm_tof3_q_select_drop_state_for_particular_queue(val, q->physical_q);
  return rc;
}

static uint32_t bf_tm_tof3_q_clear_drop_state_for_particular_queue(
    uint32_t val, uint8_t queue) {
  return (val & (~(1 << (queue % Q_PER_EG_MEMORY))));
}

bf_tm_status_t bf_tm_tof3_q_clear_egress_drop_state(bf_dev_id_t devid,
                                                    bf_tm_eg_q_t *q,
                                                    bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  uint32_t offset = q->physical_q / Q_PER_EG_MEMORY;
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    switch (color) {
      case BF_TM_COLOR_GREEN:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_state[offset]),
            &val);
        if (BF_SUCCESS != rc) return BF_TM_EINT;
        rc = bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_state[offset]),
            bf_tm_tof3_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
        break;
      case BF_TM_COLOR_YELLOW:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_yel_state[offset]),
            &val);
        if (BF_SUCCESS != rc) return BF_TM_EINT;
        rc = bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_yel_state[offset]),
            bf_tm_tof3_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
        break;
      case BF_TM_COLOR_RED:
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_red_state[offset]),
            &val);
        if (BF_SUCCESS != rc) return BF_TM_EINT;
        rc = bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.queue_drop_red_state[offset]),
            bf_tm_tof3_q_clear_drop_state_for_particular_queue(val,
                                                               q->physical_q));
        break;
      default:
        return BF_TM_EINV_ARG;
    }
    if (BF_SUCCESS != rc) return BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_q_clear_usage_counter(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(
            d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
    if (BF_SUCCESS != rc) return BF_TM_EINT;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_q_get_fast_recover_mode(bf_dev_id_t devid,
                                                  bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val;

  rc = bf_tm_tof3_q_get_shared_lmt_reg(devid, q, &val);
  q->q_cfg.fast_recover_mode =
      getp_tof3_qac_queue_config_entry_shr_cfg_fast_recover_mode(&val);
  return (rc);
}

/////////////////IG-POOLS////////////////////////////

bf_tm_status_t bf_tm_tof3_ig_spool_set_red_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.red_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_red_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_red_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_red_limit_cell[pool]),
      &val);
  ig_spool->threshold.red_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_red_hyst(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->red_hyst;
  val >>= 3;  // Unit of 8 cells

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_red_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_red_hyst(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_red_offset_cell),
      &val);
  ig_spool->red_hyst = val;
  ig_spool->red_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_yel_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.yel_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_yel_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_yel_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_yel_limit_cell[pool]),
      &val);
  ig_spool->threshold.yel_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_yel_hyst(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->yel_hyst;
  val >>= 3;  // Unit of 8 cells
  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_yel_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_yel_hyst(bf_dev_id_t devid,
                                                bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_yel_offset_cell),
      &val);
  ig_spool->yel_hyst = val;
  ig_spool->yel_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_green_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->threshold.green_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_green_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_limit_cell[pool]),
      &val);
  ig_spool->threshold.green_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_green_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = ig_spool->green_hyst;

  val >>= 3;  // Unit of 8 cells
  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_green_hyst(bf_dev_id_t devid,
                                                  bf_tm_ig_pool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_ap_offset_cell),
      &val);
  ig_spool->green_hyst = val;
  ig_spool->green_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_en = 0;
  int i;

  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.shared_pool_cnt; i++) {
    color_drop_en |=
        (((g_tm_ctx[devid]->ig_pool->spool[i].color_drop_en) ? 1 : 0) << i);
  }

  setp_tof3_wac_glb_config_color_drop_en(&val, color_drop_en);

  val &= 0xf;  // make sure remaining mem-init bits are not set
  // Color-drop-enable is in global-config register along with
  // mem_init_en, reset_floor
  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_glb_config),
        val);
  }
  (void)poolid;
  (void)ig_spool;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_val = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_glb_config),
      &val);

  color_drop_val = getp_tof3_wac_glb_config_color_drop_en(&val);
  ig_spool->color_drop_en = (bool)(color_drop_val & (1 << poolid));

  return (rc);
}

static uint32_t bf_tm_ig_spool_pfc_reg_addr(uint8_t pool, uint8_t pfc) {
  uint32_t addr = 0x0;

  switch (pool) {
    case BF_TM_IG_APP_POOL_0:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_0_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_1:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_1_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_2:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_2_limit_cell[pfc]);
      break;
    case BF_TM_IG_APP_POOL_3:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_wac_top.wac_common.wac_common
                          .wac_pfc_pool_3_limit_cell[pfc]);
      break;
  }
  return (addr);
}

bf_tm_status_t bf_tm_tof3_ig_spool_set_pfc_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 uint8_t pfc,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  uint32_t addr = bf_tm_ig_spool_pfc_reg_addr(pool, pfc);

  val = ig_spool->threshold.pfc_limit[pfc];

  /* Convert the value to HW unit - 8-cells*/
  val = (val >> 3);

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(devid, (bf_subdev_id_t)d_inst, addr, val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_pfc_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 uint8_t pfc,
                                                 bf_tm_ig_spool_t *ig_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  uint32_t addr = bf_tm_ig_spool_pfc_reg_addr(pool, pfc);

  rc = bf_tm_subdev_read_register(devid, 0, addr, &val);

  /* Convert the value from HW uint 8-cells to cells */
  val = (val << 3);

  ig_spool->threshold.pfc_limit[pfc] = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_skid_limit(bf_dev_id_t devid,
                                                  bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_hdr_limit_cell),
        ig_gpool->skid_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_skid_limit(bf_dev_id_t devid,
                                                  bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_hdr_limit_cell),
      &val);
  ig_gpool->skid_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_skid_hyst(bf_dev_id_t devid,
                                                 bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_hdr_offset_cell),
        (ig_gpool->skid_hyst >> 3));
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_skid_hyst(bf_dev_id_t devid,
                                                 bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_hdr_offset_cell),
      &val);
  ig_gpool->skid_hyst = val;
  ig_gpool->skid_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_dod_limit(bf_dev_id_t devid,
                                                 bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_dod_limit_cell),
        ig_gpool->dod_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_dod_limit(bf_dev_id_t devid,
                                                 bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_dod_limit_cell),
      &val);
  ig_gpool->dod_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_glb_min_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_glb_min_limit_cell),
        ig_gpool->glb_min_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_glb_min_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_min_limit_cell),
      &val);
  ig_gpool->glb_min_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_usage(bf_dev_id_t devid,
                                             uint8_t pool,
                                             uint32_t *ig_spool_count) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_read_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_common.wac_common
                     .wac_ap_cnt_cell[pool]),
        ig_spool_count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_wm(bf_dev_id_t devid,
                                          uint8_t pool,
                                          uint32_t *ig_spool_count) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_wm_ap_cnt_cell[pool]),
      ig_spool_count);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_clear_wm(bf_dev_id_t devid, uint8_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_wm_ap_cnt_cell[pool]),
      0);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_spool_get_color_drop_state(bf_dev_id_t devid,
                                                        bf_tm_color_t color,
                                                        uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common
                   .wac_common_block_drop_st.drop_state_cell[color]),
      &val);
  *state = getp_tof3_wac_ap_drop_state_cell_drop_state(&val);
  return rc;
}

bf_tm_status_t bf_tm_tof3_ig_spool_clear_color_drop_state(bf_dev_id_t devid,
                                                          bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common
                   .wac_common_block_drop_st.drop_state_cell[color]),
      0);
  return rc;
}

/////////////////EG-POOLS////////////////////////////

bf_tm_status_t bf_tm_tof3_eg_spool_set_red_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.red_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_red_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_buffer_get_drop_state(
    bf_dev_id_t devid,
    bf_tm_eg_buffer_drop_state_en drop_type,
    uint32_t *state) {
  if (TM_IS_DROP_STATE_TYPE_INVALID(drop_type)) {
    return BF_TM_EINV_ARG;
  }
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_qac_top.qac_common.qac_common_block_drop_st
              .qac_glb_ap_drop_state_cell[drop_type]),
      &val);
  *state = getp_tof3_qac_glb_ap_drop_state_cell_drop_state(&val);
  return rc;
}

bf_tm_status_t bf_tm_tof3_eg_buffer_drop_state_clear(
    bf_dev_id_t devid, bf_tm_eg_buffer_drop_state_en drop_type) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_IS_DROP_STATE_TYPE_INVALID(drop_type)) {
    return BF_TM_EINV_ARG;
  }
  uint32_t val = 0;
  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc = bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(
            tof3_reg,
            device_select.tm_top.tm_qac_top.qac_common.qac_common_block_drop_st
                .qac_glb_ap_drop_state_cell[drop_type]),
        val);
    if (BF_SUCCESS != rc) return rc;
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_red_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_limit_cell[pool]),
      &val);
  eg_spool->threshold.red_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_red_hyst(bf_dev_id_t devid,
                                                bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->red_hyst;
  val >>= 3;  // Unit of 8 cells

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_red_resume_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_red_hyst(bf_dev_id_t devid,
                                                bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_red_resume_offset_cell),
      &val);
  eg_spool->red_hyst = val;
  eg_spool->red_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_yel_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.yel_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_yel_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_yel_limit(bf_dev_id_t devid,
                                                 uint8_t pool,
                                                 bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_limit_cell[pool]),
      &val);
  eg_spool->threshold.yel_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_yel_hyst(bf_dev_id_t devid,
                                                bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->yel_hyst;
  val >>= 3;  // Unit of 8 cells

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_yel_resume_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_yel_hyst(bf_dev_id_t devid,
                                                bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_yel_resume_offset_cell),
      &val);
  eg_spool->yel_hyst = val;
  eg_spool->yel_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_green_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->threshold.green_limit;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_gre_limit_cell[pool]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_green_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_limit_cell[pool]),
      &val);
  eg_spool->threshold.green_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_green_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = eg_spool->green_hyst;

  val >>= 3;  // Unit of 8 cells

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_ap_gre_resume_offset_cell),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_green_hyst(bf_dev_id_t devid,
                                                  bf_tm_eg_pool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_gre_resume_offset_cell),
      &val);
  eg_spool->green_hyst = val;
  eg_spool->green_hyst <<= 3;  // Unit of 8 cells
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_set_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_en = 0;
  int i;
  // To avoid read modify write fill in the power on reset values.
  // As of now we do not re-program these power on reset values.
  uint32_t discp_apid = 0x3;
  uint32_t discp_apid_en = 0x1;
  uint32_t pre_pri0_apid = 0x3;
  uint32_t pre_pri1_apid = 0x3;
  uint32_t pre_pri2_apid = 0x3;
  uint32_t pre_pri3_apid = 0x3;

  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.shared_pool_cnt; i++) {
    color_drop_en |=
        ((((g_tm_ctx[devid]->eg_pool->spool + i)->color_drop_en) ? 1 : 0) << i);
  }

  setp_tof3_qac_glb_config_ap_color_drop_en(&val, color_drop_en);
  setp_tof3_qac_glb_config_pre_pri0_apid(&val, pre_pri0_apid);
  setp_tof3_qac_glb_config_pre_pri1_apid(&val, pre_pri1_apid);
  setp_tof3_qac_glb_config_pre_pri2_apid(&val, pre_pri2_apid);
  setp_tof3_qac_glb_config_pre_pri3_apid(&val, pre_pri3_apid);
  setp_tof3_qac_glb_config_discd_apid_en(&val, discp_apid_en);
  setp_tof3_qac_glb_config_discd_apid(&val, discp_apid);

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_glb_config),
        val);
  }
  (void)poolid;
  (void)eg_spool;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_color_drop_en(
    bf_dev_id_t devid, uint8_t poolid, bf_tm_eg_spool_t *eg_spool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t color_drop_val = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_glb_config),
      &val);

  color_drop_val = getp_tof3_qac_glb_config_ap_color_drop_en(&val);
  eg_spool->color_drop_en = (bool)(color_drop_val & (1 << poolid));

  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_gpool_set_dod_limit(bf_dev_id_t devid,
                                                 bf_tm_eg_gpool_t *eg_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_dod_limit_cell),
        eg_gpool->dod_limit);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_gpool_get_dod_limit(bf_dev_id_t devid,
                                                 bf_tm_eg_gpool_t *eg_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_dod_limit_cell),
      &val);
  eg_gpool->dod_limit = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_gpool_set_fifo_limit(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint8_t fifo,
                                                  bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    switch (pipe) {
      case 0:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe0[fifo]),
            limit);
        break;
      case 1:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe1[fifo]),
            limit);
        break;
      case 2:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe2[fifo]),
            limit);
        break;
      case 3:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe3[fifo]),
            limit);
        break;
      case 4:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe4[fifo]),
            limit);
        break;
      case 5:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe5[fifo]),
            limit);
        break;
      case 6:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe6[fifo]),
            limit);
        break;
      case 7:
        rc = bf_tm_subdev_write_register(
            devid,
            (bf_subdev_id_t)d_inst,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_common.qac_common
                         .qac_pre_fifo_limit_pkt_pipe7[fifo]),
            limit);
        break;
      default:
        rc = BF_TM_EINV_ARG;
        break;
    }
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "fifo=%d, dev=%d, subdev=%d, pipe=%d: Unable to set PRE FIFO "
          "limit=%d",
          fifo,
          devid,
          d_inst,
          pipe,
          limit);
      return (rc);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_gpool_get_fifo_limit(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint8_t fifo,
                                                  bf_tm_thres_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  switch (pipe) {
    case 0:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe0[fifo]),
          limit);
      break;
    case 1:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe1[fifo]),
          limit);
      break;
    case 2:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe2[fifo]),
          limit);
      break;
    case 3:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe3[fifo]),
          limit);
      break;
    case 5:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe5[fifo]),
          limit);
      break;
    case 6:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe6[fifo]),
          limit);
      break;
    case 7:
      rc = bf_tm_subdev_read_register(
          devid,
          0,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_common.qac_common
                       .qac_pre_fifo_limit_pkt_pipe7[fifo]),
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

bf_tm_status_t bf_tm_tof3_eg_spool_get_usage(bf_dev_id_t devid,
                                             uint8_t pool,
                                             uint32_t *eg_spool_count)

{
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_glb_ap_cnt_cell[pool]),
      eg_spool_count);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_get_wm(bf_dev_id_t devid,
                                          uint8_t pool,
                                          uint32_t *eg_spool_count)

{
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_wm_glb_ap_cnt_cell[pool]),
      eg_spool_count);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_eg_spool_clear_wm(bf_dev_id_t devid, uint8_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_wm_glb_ap_cnt_cell[pool]),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_uc_ct_size(bf_dev_id_t devid,
                                                  uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  setp_tof3_qclc_ct_tot_uc_th(&val, cells);
  setp_tof3_qclc_ct_tot_mc_th(&val, g_tm_ctx[devid]->mc_ct_size);

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc_common[0].tot_th),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_mc_ct_size(bf_dev_id_t devid,
                                                  uint32_t cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    val = 0;
    // setting multicast CT pool size to non zero enables CT for MC
    // packets that are injected into TM by pipe with ct_enable
    // bit set in intrinsic metadata.

    setp_tof3_qac_mcct_total_pkt_limit_total_pkt_lmt(&val,
                                                     0x400);  // 0x400 is power

    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_mcct_total_pkt_limit),
        val);

    val = 0;
    // on default value
    if (cells) {
      // When MC CT is enabled, set replication FIFO to 0x48 (jira TOFLAB-36)
      setp_tof3_qac_mcct_repli_fifo_pkt_limit_repli_fifo_lmt(&val, 0x48);
    } else {
      setp_tof3_qac_mcct_repli_fifo_pkt_limit_repli_fifo_lmt(
          &val, 0x4);  // Power on default
                       // value.
    }
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_common.qac_common
                     .qac_mcct_repli_fifo_pkt_limit),
        val);

    val = 0;
    setp_tof3_qclc_ct_tot_mc_th(&val, cells);
    setp_tof3_qclc_ct_tot_uc_th(&val, g_tm_ctx[devid]->uc_ct_size);
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc_common[0].tot_th),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_uc_ct_size(bf_dev_id_t devid,
                                                  uint32_t *cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc_common[0].tot_th),
      &val);

  *cells = getp_tof3_qclc_ct_tot_uc_th(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_mc_ct_size(bf_dev_id_t devid,
                                                  uint32_t *cells) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc_common[0].tot_th),
      &val);
  *cells = getp_tof3_qclc_ct_tot_mc_th(&val);
  return (rc);
}

/////////////////SCH////////////////////////////

static void bf_tm_tof3_sch_prep_q_cfg_reg(uint32_t *val, bf_tm_eg_q_t *q) {
  setp_tof3_sch_queue_config_r_enb(val, q->q_sch_cfg.sch_enabled);
  /* Per HW team, pfc_upd_enb bit should be set always */
  setp_tof3_sch_queue_config_r_pfc_upd_enb(val, 1);
  setp_tof3_sch_queue_config_r_min_rate_enb(val, q->q_sch_cfg.min_rate_enable);
  setp_tof3_sch_queue_config_r_max_rate_enb(val, q->q_sch_cfg.max_rate_enable);
  setp_tof3_sch_queue_config_r_l1_id(val, q->q_sch_cfg.cid);
  setp_tof3_sch_queue_config_r_pfc_pri(val, q->q_sch_cfg.pfc_prio);
  setp_tof3_sch_queue_config_r_min_rate_pri(val,
                                            q->q_sch_cfg.min_rate_sch_prio);
  setp_tof3_sch_queue_config_r_max_rate_pri(val,
                                            q->q_sch_cfg.max_rate_sch_prio);
  setp_tof3_sch_queue_config_r_adv_fc_mode(val, q->q_sch_cfg.adv_fc_mode);
}

static void bf_tm_tof3_sch_fill_q_struct(uint32_t *val, bf_tm_eg_q_t *q) {
  q->q_sch_cfg.sch_enabled = getp_tof3_sch_queue_config_r_enb(val);
  q->q_sch_cfg.sch_pfc_enabled = getp_tof3_sch_queue_config_r_pfc_upd_enb(val);
  q->q_sch_cfg.cid = getp_tof3_sch_queue_config_r_l1_id(val);
  q->q_sch_cfg.pfc_prio = getp_tof3_sch_queue_config_r_pfc_pri(val);
  q->q_sch_cfg.min_rate_sch_prio =
      getp_tof3_sch_queue_config_r_min_rate_pri(val);
  q->q_sch_cfg.max_rate_sch_prio =
      getp_tof3_sch_queue_config_r_max_rate_pri(val);
  q->q_sch_cfg.min_rate_enable = getp_tof3_sch_queue_config_r_min_rate_enb(val);
  q->q_sch_cfg.max_rate_enable = getp_tof3_sch_queue_config_r_max_rate_enb(val);
  q->q_sch_cfg.adv_fc_mode = getp_tof3_sch_queue_config_r_adv_fc_mode(val);
}

bf_status_t bf_tm_tof3_sch_set_q_priority(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  bf_tm_tof3_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      val);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_priority(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tof3_sch_fill_q_struct(&val, q);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_q_wt(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  setp_tof3_tm_sch_pipe_mem_rspec_q_exc_static_mem_entry_wt(
      &lo, q->q_sch_cfg.dwrr_wt);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_exc_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_wt(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_exc_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  q->q_sch_cfg.dwrr_wt =
      getp_tof3_tm_sch_pipe_mem_rspec_q_exc_static_mem_entry_wt(&lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_q_pfc_prio(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  bf_tm_tof3_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      val);
  if (q->q_sch_cfg.sch_enabled) {
    // queue is scheduling enabled. After enabling queue, it
    // is required to clear q_pfc_status. The reason being
    // when queue is disabled from sch, TM logic self asserts
    // pfc on the queue.
    uint64_t indir_addr =
        tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, 0);
  }

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_pfc_prio(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tof3_sch_fill_q_struct(&val, q);

  return (rc);
}

static void bf_tm_burst_size_to_mant_exp_form(uint32_t burst_size,
                                              uint32_t *mant,
                                              uint32_t *exp) {
  // 8b Mant
  // 4b exp
  // B = Mant << Exp
  // Bmax = 255 << 15

  int i = 0;

  *mant = 0;
  *exp = 0;
  if (!burst_size) {
    return;
  }

  if (burst_size > (255 << 15)) burst_size = (255 << 15);

  *mant = 0;
  i = 22;
  while (!(*mant) && i >= 8) {
    if (burst_size & (0xff << i)) {
      *mant = (burst_size >> (i - 7));
      *exp = i - 7;
      break;
    } else {
      i--;
    }
  }
  if (i < 8) {
    *mant = burst_size;
    *exp = 0;
  }
}

static void bf_tm_burst_size_mant_exp_form_to_bs(uint32_t mant,
                                                 uint32_t exp,
                                                 uint32_t *rate) {
  // 8b Mant
  // 4b exp
  // B = Mant << Exp
  // Bmax = 255 << 15
  *rate = mant << exp;
}

/*
 * Converts rate to bytes (if input is in bps) or
 * to packets (if input is in pps) per 80 clocks as below
 *
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (12b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_to_mant_exp_form(bf_dev_id_t dev_id,
                                        uint64_t rate,
                                        uint32_t *mant,
                                        uint32_t *exp,
                                        bool pps) {
  (void)dev_id;
  // 12b Mant
  // 4b exp

  float rate_per_80_clock_period, intg_part;
  uint64_t rate_for_80_clocks = rate;
  uint32_t max_exp;
  bool decimal_part = true;

  float clockspeed = BF_TM_TOF3_SHAPER_FREQ;

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
  if (rate_per_80_clock_period > 4095) {
    // Invalid... can't represent this. It also means that on port/queue we
    // are setting more than 400Gbps
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
  while (rate_per_80_clock_period < 2048 && decimal_part == true &&
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

  if (decimal_part && *mant < 4095) {
    *mant += 1;  // Instead of losing fractional rate and under provisioning,
                 // set rate to next higher representable value;
  }
}

/*
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (12b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_mant_exp_form_to_rate(
    bf_dev_id_t dev_id, uint32_t mant, uint32_t exp, uint64_t *rate, bool pps) {
  (void)dev_id;
  // 12b Mant
  // 4b exp

  float clockspeed = BF_TM_TOF3_SHAPER_FREQ;

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
 * Makes sure (mantissa <= bucket_size) for optimal rate
 *
 * Rate in Bytes or Packets per 80-core clocks. It is represented as
 * floating point number (12b Mantissa, 4b Exponent).
 * [Rate = Mantissa / (2**Exponent x 80ns) for 1ns clock].
 * When 'pps' is set, Exponent is multiplied by 2 for packet rate calculation.
 */
static void bf_tm_rate_to_mant_exp_form_adv(bf_dev_id_t dev_id,
                                            uint64_t rate,
                                            uint32_t bs,
                                            uint32_t *mant,
                                            uint32_t *exp,
                                            bool pps) {
  // 12b Mant
  // 4b exp
  uint64_t new_rate = 0, old_rate = 0;
  bool bnew_rate_found = false;

  bf_tm_rate_to_mant_exp_form(dev_id, rate, mant, exp, pps);

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

  while ((bs > *mant) && (new_rate < rate) && (*mant < 4095)) {
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

bool bf_tm_tof3_sch_verify_burst_size(bf_dev_id_t devid,
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

bool bf_tm_tof3_sch_verify_rate(bf_dev_id_t devid,
                                uint32_t restore_rate,
                                uint32_t orig_rate,
                                bool pps) {
  uint32_t restore_mant, orig_mant;
  uint32_t restore_exp, orig_exp;
  (void)devid;  // For future use

  bf_tm_rate_to_mant_exp_form(
      devid, restore_rate, &restore_mant, &restore_exp, pps);
  bf_tm_rate_to_mant_exp_form(devid, orig_rate, &orig_mant, &orig_exp, pps);

  if (restore_mant == orig_mant && restore_exp == orig_exp) {
    return true;
  } else {
    return false;
  }
}

bf_status_t bf_tm_tof3_sch_set_q_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint64_t rate = 0, hi = 0, lo = 0, new_rate = 0;
  uint32_t bs = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  // convert q->burst_size into mant, exp form
  // q bs is specified in pps or kbps.. covert to bps if in kbps
  setp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_pps(
      &lo, q->q_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(q->q_sch_cfg.max_burst_size, &mant, &exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_bs_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_bs_mant(&lo, mant);

  // convert q->rate into mant, exp form.
  // q rate is specified in pps or kbps.. covert to bps in case of kbps
  rate = (uint64_t)(q->q_sch_cfg.max_rate);
  if (!q->q_sch_cfg.pps) {
    rate *= 1000;
  }
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, q->q_sch_cfg.pps);
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

  setp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_rate_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_rate_mant(&lo,
                                                                      mant);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_max_lb_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate, hi, lo;
  uint32_t bs, mant, exp;
  bool pps;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_max_lb_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  pps = getp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_pps(&lo);
  q->q_sch_cfg.pps = pps;

  exp = getp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_bs_exp(&lo);
  mant = getp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_bs_mant(&lo);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);

  exp = getp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_rate_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_q_max_lb_static_mem_entry_rate_mant(&lo);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  q->q_sch_cfg.max_rate = (uint32_t)rate;
  if (!pps) {
    q->q_sch_cfg.max_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  q->q_sch_cfg.max_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_q_min_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint64_t rate = 0, hi = 0, lo = 0, new_rate = 0;
  uint32_t bs = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  // convert q->min_rate into mant, exp form.
  // convert q->burst_size into mant, exp form
  setp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_pps(
      &lo, q->q_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(q->q_sch_cfg.min_burst_size, &mant, &exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_bs_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_bs_mant(&lo, mant);

  // q rate is specified in kbps or pps.. covert to bps in case of kbps
  rate = (uint64_t)(q->q_sch_cfg.min_rate);
  if (!q->q_sch_cfg.pps) {
    rate *= 1000;
  }
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, q->q_sch_cfg.pps);
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

  setp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_rate_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_rate_mant(&lo,
                                                                      mant);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_min_lb_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_min_rate(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate, hi, lo;
  uint32_t bs, mant, exp;
  bool pps;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_min_lb_static_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  pps = getp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_pps(&lo);

  exp = getp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_bs_exp(&lo);
  mant = getp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_bs_mant(&lo);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  exp = getp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_rate_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_q_min_lb_static_mem_entry_rate_mant(&lo);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  q->q_sch_cfg.min_rate = (uint32_t)rate;
  if (!pps) {
    q->q_sch_cfg.min_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  q->q_sch_cfg.min_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_q_max_rate_enable_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q) {
  // Because Q_sched and Q max rate enable status are shared in same register,
  // invoke q_sched function to set max-rate-enable status
  return (bf_tm_tof3_sch_set_q_sched(devid, q));
}

bf_status_t bf_tm_tof3_sch_set_q_min_rate_enable_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q) {
  // Because Q_sched and Q min rate enable status are shared in same register,
  // invoke q_sched function to set min-rate-enable status
  return (bf_tm_tof3_sch_set_q_sched(devid, q));
}

bf_status_t bf_tm_tof3_sch_get_q_max_rate_enable_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q) {
  // Because Q_sched and Q max rate enable status are shared in same register,
  // invoke q_sched function to get max-rate-enable status
  return (bf_tm_tof3_sch_get_q_sched(devid, q));
}

bf_status_t bf_tm_tof3_sch_get_q_min_rate_enable_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q) {
  // Because Q_sched and Q min rate enable status are shared in same register,
  // invoke q_sched function to get min-rate-enable status
  return (bf_tm_tof3_sch_get_q_sched(devid, q));
}

bf_status_t bf_tm_tof3_sch_set_q_sched(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  bf_tm_tof3_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      val);
  if (q->q_sch_cfg.sch_enabled) {
    // queue is scheduling enabled. After enabling queue, it
    // is required to clear q_pfc_status. The reason being
    // when queue is disabled from sch, TM logic self asserts
    // pfc on the queue.
    uint64_t indir_addr =
        tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, 0);
  }
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_q_sched(bf_dev_id_t devid, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tof3_sch_fill_q_struct(&val, q);

  return (rc);
}

static bf_status_t bf_tm_tof3_sch_set_port_flush(bf_dev_id_t devid,
                                                 bf_tm_port_t *p,
                                                 uint8_t *mask,
                                                 bool enb) {
  if (g_tm_ctx[devid]->subdev_count > 1) {
    // For Multi-Die device no need to do the flush.
    return BF_TM_EOK;
  }

  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t valarr[4] = {0};
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  for (int i = 0; i < BF_TM_TOF3_QUEUES_PER_PG; i++) {
    setp_tof3_sch_queue_flush_mask_r_queue_flush_mask(
        valarr, i, (mask[i / 8] >> (i % 8)) & 0x1u);
  }
  rc |= bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_flush_mask.queue_flush_mask_0_2),
      valarr[0]);
  rc |= bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_flush_mask.queue_flush_mask_1_2),
      valarr[1]);

  setp_tof3_sch_queue_flush_ctrl_r_queue_flush_port(&val, p->port);
  setp_tof3_sch_queue_flush_ctrl_r_queue_flush_enb(&val, enb);
  rc |= bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe].queue_flush_ctrl),
      val);

  return (rc);
}

static bool queues_are_empty(bf_dev_id_t devid,
                             bf_tm_eg_q_t **q_list,
                             uint8_t q_count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = 0;
  bf_tm_eg_q_t *q = NULL;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  for (uint8_t i = 0; i < q_count; i++) {
    hi = 0;
    lo = 0;
    q = q_list[i];
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

    indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(
            d_pipe, q->physical_q);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("TM: queue occupancy read failed for dev %d, pipe %d q %d",
                devid,
                q->p_pipe,
                q->physical_q);
      return false;
    }
    if (getp_tof3_qac_queue_cell_count_entry_queue_cell_count(&lo)) {
      return false;
    }
  }

  return true;
}

static void log_queue_state(bf_dev_id_t devid,
                            bf_tm_eg_q_t **q_list,
                            uint8_t q_count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = 0;
  bf_tm_eg_q_t *q = NULL;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  for (uint8_t i = 0; i < q_count; i++) {
    hi = 0;
    lo = 0;
    q = q_list[i];
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

    indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(
            d_pipe, q->physical_q);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("TM: queue occupancy read failed for dev %d, pipe %d q %d",
                devid,
                q->p_pipe,
                q->physical_q);
      return;
    }
    LOG_ERROR("Dev %d Q-index %d logQ %d phyQ %d occupancy %d",
              devid,
              i,
              q->logical_q,
              q->physical_q,
              getp_tof3_qac_queue_cell_count_entry_queue_cell_count(&lo));
  }
}

static bf_status_t bf_tm_tof3_sch_wait_port_flush_q(bf_dev_id_t devid,
                                                    bf_tm_port_t *p,
                                                    bf_tm_eg_q_t **q_list,
                                                    uint8_t q_count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t done = 0;
  uint32_t addr = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  addr =
      offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[d_pipe].intr.stat);

  // clear intr status before polling
  setp_tof3_tm_sch_pipe_rspec_intr_stat_q_flush_done(&val, 1);
  rc = bf_tm_subdev_write_register(devid, subdev_id, addr, val);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: queue flush start failed for dev %d, pipe %d port %d",
              devid,
              p->p_pipe,
              p->port);
    return (rc);
  }

  val = 0;
  int num_poll = 10;
  useconds_t poll_interval = 40;
#if DEVICE_IS_EMULATOR
  num_poll = 4;
  poll_interval = 250000;
#endif
  for (int i = 0; i < num_poll; i++) {
    if (queues_are_empty(devid, q_list, q_count)) {
      return rc;
    }

    val = 0;
    rc = bf_tm_subdev_read_register(devid, subdev_id, addr, &val);
    done = getp_tof3_tm_sch_pipe_rspec_intr_stat_q_flush_done(&val);
    if (done) {
      return rc;
    }
    bf_sys_usleep(poll_interval);
  }

  // TODO: The Queues are not flushed properly we should log and return an
  // error.
  LOG_ERROR("TM: queue flush wait timed out for dev %d, pipe %d port %d",
            devid,
            p->p_pipe,
            p->port);
  log_queue_state(devid, q_list, q_count);
  return (rc);
}

static bf_status_t bf_tm_tof3_sch_wait_port_q_empty(bf_dev_id_t devid,
                                                    bf_tm_port_t *p,
                                                    bf_tm_eg_q_t **q_list,
                                                    uint8_t q_count) {
  bf_tm_status_t rc = BF_TM_EOK;

  int num_poll = 100;
  useconds_t poll_interval = 40;
#if DEVICE_IS_EMULATOR
  num_poll = 4;
  poll_interval = 250000;
#endif

  for (int i = 0; i < num_poll; i++) {
    if (queues_are_empty(devid, q_list, q_count)) {
      return rc;
    }
    bf_sys_usleep(poll_interval);
  }

  // The Queues are not flushed properly we should log and return an error.
  rc = BF_TM_EINT;
  LOG_ERROR("TM: queue empty wait timed out for dev %d, l_pipe %d l_port %d",
            devid,
            p->l_pipe,
            p->uport);
  log_queue_state(devid, q_list, q_count);
  return (rc);
}

static bf_status_t bf_tm_tof3_sch_wait_port_flush(bf_dev_id_t devid,
                                                  bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOF3_QUEUES_PER_PG];
  bf_dev_port_t port;

  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  // port = lld_sku_map_devport_from_device_to_user(devid, port);

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

  bf_tm_eg_q_t *q_list[BF_TM_TOF3_QUEUES_PER_PG] = {0};
  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  for (q = 0; q < q_count; q++) {
    rc |= bf_tm_q_get_descriptor(devid, port, q, &queue);
    if (rc != BF_TM_EOK) {
      return rc;
    }
    q_list[q] = queue;
  }

  // For TF3.2 ASIC we cannot force flush the Queues, we have to wait for the
  // Queues to drain normally. On Model the Queue usage is all 0BAD so will
  // never goes to zero only flush ctrl can be used to check the state and
  // complete the process
  if ((g_tm_ctx[devid]->subdev_count > 1) && TM_IS_TARGET_ASIC(devid)) {
    rc = bf_tm_tof3_sch_wait_port_q_empty(devid, p, q_list, q_count);
  } else {
    rc = bf_tm_tof3_sch_wait_port_flush_q(devid, p, q_list, q_count);
  }
  return (rc);
}

static void bf_tm_tof3_sch_prep_l1_cfg_reg(uint32_t *val, bf_tm_eg_l1_t *l1) {
  setp_tof3_sch_l1_config_r_enb(val, l1->l1_sch_cfg.sch_enabled);
  setp_tof3_sch_l1_config_r_min_rate_enb(val, l1->l1_sch_cfg.min_rate_enable);
  setp_tof3_sch_l1_config_r_max_rate_enb(val, l1->l1_sch_cfg.max_rate_enable);
  setp_tof3_sch_l1_config_r_l1_port(val, l1->l1_sch_cfg.cid);
  setp_tof3_sch_l1_config_r_min_rate_pri(val, l1->l1_sch_cfg.min_rate_sch_prio);
  setp_tof3_sch_l1_config_r_max_rate_pri(val, l1->l1_sch_cfg.max_rate_sch_prio);
  setp_tof3_sch_l1_config_r_pri_prop(val, l1->l1_sch_cfg.pri_prop);
}

static void bf_tm_tof3_sch_fill_l1_struct(uint32_t *val, bf_tm_eg_l1_t *l1) {
  l1->l1_sch_cfg.sch_enabled = getp_tof3_sch_l1_config_r_enb(val);
  l1->l1_sch_cfg.cid = getp_tof3_sch_l1_config_r_l1_port(val);
  l1->l1_sch_cfg.min_rate_sch_prio =
      getp_tof3_sch_l1_config_r_min_rate_pri(val);
  l1->l1_sch_cfg.max_rate_sch_prio =
      getp_tof3_sch_l1_config_r_max_rate_pri(val);
  l1->l1_sch_cfg.min_rate_enable = getp_tof3_sch_l1_config_r_min_rate_enb(val);
  l1->l1_sch_cfg.max_rate_enable = getp_tof3_sch_l1_config_r_max_rate_enb(val);
}

bf_status_t bf_tm_tof3_sch_set_l1_priority(bf_dev_id_t devid,
                                           bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  bf_tm_tof3_sch_prep_l1_cfg_reg(&val, l1);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .l1_config[l1->physical_l1]),
      val);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_l1_priority(bf_dev_id_t devid,
                                           bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .l1_config[l1->physical_l1]),
      &val);
  bf_tm_tof3_sch_fill_l1_struct(&val, l1);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_l1_wt(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  setp_tof3_tm_sch_pipe_mem_rspec_l1_exc_static_mem_entry_wt(
      &lo, l1->l1_sch_cfg.dwrr_wt);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_exc_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_l1_wt(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_exc_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  l1->l1_sch_cfg.dwrr_wt =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_exc_static_mem_entry_wt(&lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_l1_rate(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint64_t rate = 0, hi = 0, lo = 0, new_rate = 0;
  uint32_t bs = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  // convert l1->burst_size into mant, exp form
  // l1 bs is specified in pps or kbps.. covert to bps if in kbps
  setp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_pps(
      &lo, l1->l1_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(l1->l1_sch_cfg.max_burst_size, &mant, &exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_bs_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_bs_mant(&lo, mant);

  // convert l1->rate into mant, exp form.
  // l1 rate is specified in pps or kbps.. covert to bps in case of kbps
  rate = (uint64_t)(l1->l1_sch_cfg.max_rate);
  if (!l1->l1_sch_cfg.pps) {
    rate *= 1000;
  }
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, l1->l1_sch_cfg.pps);
  bf_tm_rate_mant_exp_form_to_rate(
      devid, mant, exp, &new_rate, l1->l1_sch_cfg.pps);
  LOG_DBG("L1_SHAPER MAX ADV dev %d p_pipe %d and p_l1 %d PPS %d ORATE %" PRIu64
          " NRATE %" PRIu64
          " "
          "BS %d MANT %d EXP %d",
          devid,
          l1->p_pipe,
          l1->physical_l1,
          l1->l1_sch_cfg.pps,
          rate,
          new_rate,
          bs,
          mant,
          exp);

  setp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_rate_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_rate_mant(&lo,
                                                                       mant);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_max_lb_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_l1_rate(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate, hi, lo;
  uint32_t bs, mant, exp;
  bool pps;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_max_lb_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  pps = getp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_pps(&lo);
  l1->l1_sch_cfg.pps = pps;

  exp = getp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_bs_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_bs_mant(&lo);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);

  exp =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_rate_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_max_lb_static_mem_entry_rate_mant(&lo);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  l1->l1_sch_cfg.max_rate = (uint32_t)rate;
  if (!pps) {
    l1->l1_sch_cfg.max_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  l1->l1_sch_cfg.max_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_l1_min_rate(bf_dev_id_t devid,
                                           bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint64_t rate = 0, hi = 0, lo = 0, new_rate = 0;
  uint32_t bs = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  // convert l1->min_rate into mant, exp form.
  // convert l1->burst_size into mant, exp form
  setp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_pps(
      &lo, l1->l1_sch_cfg.pps ? 1 : 0);
  bf_tm_burst_size_to_mant_exp_form(l1->l1_sch_cfg.min_burst_size, &mant, &exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_bs_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_bs_mant(&lo, mant);

  // l1 rate is specified in kbps or pps.. covert to bps in case of kbps
  rate = (uint64_t)(l1->l1_sch_cfg.min_rate);
  if (!l1->l1_sch_cfg.pps) {
    rate *= 1000;
  }
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(
      devid, rate, bs, &mant, &exp, l1->l1_sch_cfg.pps);
  bf_tm_rate_mant_exp_form_to_rate(
      devid, mant, exp, &new_rate, l1->l1_sch_cfg.pps);
  LOG_DBG("L1_SHAPER MIN ADV dev %d p_pipe %d and p_l1 %d PPS %d ORATE %" PRIu64
          " NRATE %" PRIu64
          " "
          "BS %d MANT %d EXP %d",
          devid,
          l1->p_pipe,
          l1->physical_l1,
          l1->l1_sch_cfg.pps,
          rate,
          new_rate,
          bs,
          mant,
          exp);

  setp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_rate_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_rate_mant(&lo,
                                                                       mant);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_min_lb_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_l1_min_rate(bf_dev_id_t devid,
                                           bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate, hi, lo;
  uint32_t bs, mant, exp;
  bool pps;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_l1_min_lb_static_mem(d_pipe, l1->physical_l1);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  pps = getp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_pps(&lo);

  exp = getp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_bs_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_bs_mant(&lo);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  exp =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_rate_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_l1_min_lb_static_mem_entry_rate_mant(&lo);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  l1->l1_sch_cfg.min_rate = (uint32_t)rate;
  if (!pps) {
    l1->l1_sch_cfg.min_rate =
        (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  l1->l1_sch_cfg.min_burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_l1_max_rate_enable_status(bf_dev_id_t devid,
                                                         bf_tm_eg_l1_t *l1) {
  // Because l1_sched and l1 max rate enable status are shared in same
  // register, invoke l1_sched function to set max-rate-enable status
  return (bf_tm_tof3_sch_set_l1_sched(devid, l1));
}

bf_status_t bf_tm_tof3_sch_set_l1_min_rate_enable_status(bf_dev_id_t devid,
                                                         bf_tm_eg_l1_t *l1) {
  // Because l1_sched and l1 min rate enable status are shared in same
  // register, invoke l1_sched function to set min-rate-enable status
  return (bf_tm_tof3_sch_set_l1_sched(devid, l1));
}

bf_status_t bf_tm_tof3_sch_get_l1_max_rate_enable_status(bf_dev_id_t devid,
                                                         bf_tm_eg_l1_t *l1) {
  // Because l1_sched and l1 max rate enable status are shared in same
  // register, invoke l1_sched function to get max-rate-enable status
  return (bf_tm_tof3_sch_get_l1_sched(devid, l1));
}

bf_status_t bf_tm_tof3_sch_get_l1_min_rate_enable_status(bf_dev_id_t devid,
                                                         bf_tm_eg_l1_t *l1) {
  // Because l1_sched and l1 min rate enable status are shared in same
  // register, invoke l1_sched function to get min-rate-enable status
  return (bf_tm_tof3_sch_get_l1_sched(devid, l1));
}

bf_status_t bf_tm_tof3_sch_set_l1_sched(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  bf_tm_tof3_sch_prep_l1_cfg_reg(&val, l1);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .l1_config[l1->physical_l1]),
      val);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_l1_sched(bf_dev_id_t devid, bf_tm_eg_l1_t *l1) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(l1->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(l1->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .l1_config[l1->physical_l1]),
      &val);
  bf_tm_tof3_sch_fill_l1_struct(&val, l1);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_port_rate(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t mant, exp;
  uint64_t rate = 0, hi = 0, lo = 0, new_rate = 0;
  uint32_t bs = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_port_max_lb_static_mem(d_pipe, p->port);

  setp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_pps(
      &lo, p->pps ? 1 : 0);

  bf_tm_burst_size_to_mant_exp_form(p->burst_size, &mant, &exp);
  setp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_bs_exp(&lo, exp);
  setp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_bs_mant(&lo,
                                                                       mant);

  // port rate is specified in kbps or pps.. covert to bps in cas of kbps
  rate = (uint64_t)(p->port_rate);
  if (!p->pps) {
    rate = (uint64_t)(p->port_rate) * 1000;
  }

  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  bf_tm_rate_to_mant_exp_form_adv(devid, rate, bs, &mant, &exp, p->pps);
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

  setp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_rate_exp(&lo,
                                                                        exp);
  setp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_rate_mant(&lo,
                                                                         mant);

  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_port_rate(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t rate, hi = 0, lo = 0;
  uint32_t bs, mant, exp;
  bool pps;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_port_max_lb_static_mem(d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);

  pps = getp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_pps(&lo);
  p->pps = pps;

  exp =
      getp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_bs_exp(&lo);
  mant =
      getp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_bs_mant(&lo);
  bf_tm_burst_size_mant_exp_form_to_bs(mant, exp, &bs);
  exp = getp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_rate_exp(
      &lo);
  mant = getp_tof3_tm_sch_pipe_mem_rspec_port_max_lb_static_mem_entry_rate_mant(
      &lo);
  bf_tm_rate_mant_exp_form_to_rate(devid, mant, exp, &rate, pps);
  p->port_rate = (uint32_t)rate;
  if (!pps) {
    p->port_rate = (uint32_t)(rate / 1000);  // Rate is mentioned in KBPS
  }
  p->burst_size = bs;

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_port_sched(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, pt_speed;
  uint32_t reg_offset = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, d_inst);

    if (p->port < 32) {
      reg_offset = offsetof(tof3_reg,
                            device_select.tm_top.tm_pex_top.pex[d_pipe]
                                .port_tdm_enb_map.port_tdm_enb_map_0_2);

    } else if (p->port < 64) {
      reg_offset = offsetof(tof3_reg,
                            device_select.tm_top.tm_pex_top.pex[d_pipe]
                                .port_tdm_enb_map.port_tdm_enb_map_1_2);
    }

    rc = bf_tm_subdev_read_register(devid, subdev_id, reg_offset, &val);

    if (rc != BF_SUCCESS) {
      return rc;
    }

    if (p->port < 32) {
      setp_tof3_pex_port_tdm_enb_map_port_tdm_enb_map_0_2_enb_map0_31_0(
          &val, (p->tdm << p->port));

    } else if (p->port < 64) {
      if (p->port < 36) {
        setp_tof3_pex_port_tdm_enb_map_port_tdm_enb_map_1_2_enb_map0_35_32(
            &val, (p->tdm << (p->port - 32)));
      }
    }

    rc = bf_tm_subdev_write_register(
        devid, (bf_subdev_id_t)d_inst, reg_offset, val);

    if (rc != BF_SUCCESS) {
      return rc;
    }
  }
  val = 0;

  setp_tof3_sch_port_config_r_enb(&val, p->sch_enabled ? 1 : 0);
  /* Per HW team, pfc_upd_enb bit should be set always */
  setp_tof3_sch_port_config_r_pfc_upd_enb(&val, 1);

  if (p->max_rate_enabled) {
    setp_tof3_sch_port_config_r_max_rate_enb(&val, 1);
  } else {
    setp_tof3_sch_port_config_r_max_rate_enb(&val, 0);
  }

  switch (p->speed) {
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
      pt_speed = 5;
      break;
    case BF_SPEED_200G:
      pt_speed = 6;
      break;
    case BF_SPEED_400G:
      pt_speed = 7;
      break;
    case BF_SPEED_NONE:
      // When disabling port, set channel speed to 50G
      pt_speed = 3;
      break;
    default:
      pt_speed = 7;
      break;
  }
  setp_tof3_sch_port_config_r_port_speed_mode(&val, pt_speed);
  d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  rc |= bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_sch_top.sch[d_pipe].port_config[p->port]),
      val);
  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_port_sched(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_sch_top.sch[d_pipe].port_config[p->port]),
      &val);
  p->sch_enabled = (getp_tof3_sch_port_config_r_enb(&val)) ? true : false;
  switch (getp_tof3_sch_port_config_r_port_speed_mode(&val)) {
    case 0:
      p->speed = BF_SPEED_10G;
      break;
    case 1:
      p->speed = BF_SPEED_25G;
      break;
    case 2:
      p->speed = BF_SPEED_40G;
      break;
    case 3:
      p->speed = BF_SPEED_50G;
      break;
    case 5:
      p->speed = BF_SPEED_100G;
      break;
    case 6:
      p->speed = BF_SPEED_200G;
      break;
    case 7:
      p->speed = BF_SPEED_400G;
      break;
    default:
      p->speed = BF_SPEED_400G;
      break;
  }
  p->fc_rx_type = getp_tof3_sch_port_config_r_pfc_upd_enb(&val);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_set_pkt_ifg(bf_dev_id_t devid, void *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pipe_t *eg_pipe = (bf_tm_eg_pipe_t *)s;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(eg_pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(eg_pipe->p_pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe].global_bytecnt_adj),
      eg_pipe->ifg_compensation);

  return (rc);
}

bf_status_t bf_tm_tof3_sch_get_pkt_ifg(bf_dev_id_t devid, void *s) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_pipe_t *eg_pipe = (bf_tm_eg_pipe_t *)s;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(eg_pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(eg_pipe->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe].global_bytecnt_adj),
      &val);
  eg_pipe->ifg_compensation = val;

  return (rc);
}

/////////////////PORTS////////////////////////////

//  ingress PORT related HW programming.
//  List of tables/regiser/memories programmed
//      csr_mem_wac_port_ppg_mapping

bf_tm_status_t bf_tm_tof3_set_egress_pipe_max_limit(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_ep_limit_cell[d_pipe]),
      limit);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_get_egress_pipe_max_limit(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_ep_limit_cell[d_pipe]),
      limit);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_set_egress_pipe_hyst(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               uint32_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_ep_resume_offset_cell),
      limit);
  (void)pipe;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_set_timestamp_shift(bf_dev_id_t devid,
                                              uint8_t shift) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.timestamp_shift),
        shift);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_get_timestamp_shift(bf_dev_id_t devid,
                                              uint8_t *shift) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_psc_top.psc_common.timestamp_shift),
      &val);
  *shift = (uint8_t)val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_get_egress_pipe_hyst(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               uint32_t *limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_common.qac_common
                   .qac_ep_resume_offset_cell),
      limit);
  (void)pipe;
  return (rc);
}

bf_status_t bf_tm_tof3_set_qac_qstat_report_mode(bf_dev_id_t devid,
                                                 bf_tm_eg_pipe_t *pipe,
                                                 bool mode) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t *val = &(pipe->qac_pipe_config);
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  setp_tof3_qac_pipe_config_qstat_report_mode(val, mode);
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.pipe_config),
        *val);
  }

  return rc;
}

bf_status_t bf_tm_tof3_get_qac_qstat_report_mode(bf_dev_id_t devid,
                                                 bf_tm_eg_pipe_t *pipe,
                                                 bool *mode) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t *val = &(pipe->qac_pipe_config);
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_qac_top.qac_pipe[d_pipe].qac_reg.pipe_config),
      val);
  *mode = getp_tof3_qac_pipe_config_qstat_report_mode(val);

  return rc;
}

bf_tm_status_t bf_tm_tof3_port_set_wac_drop_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, 0);
  d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, 0);

  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_th(
      d_pipe, port->port);
  /*
   * HW use it in unit of 8, so limit[18:3] is used for limit check, while bit
   * 2 (limit[2]) is used as ?enable?. When limit[2] = 1, then limit[18:3]
   * takes effect, else port_limit will not be enforced. Example : Limit[18:3]
   * = 0x100, limit[2] = 1:  ingress port share limit is 256*8 = 2048.
   * Limit[18:3] = 0x100, limit[2] = 0:  ingress port share limit is ?NO
   * Limit?. Also: Limit[18:3] = TOTAL_BUFF_CELLS, limit[2] = 1:  ingress port
   * share limit is "No limit".
   */
  lo = (TM_CLEAR_LOW_3_BITS(port->wac_drop_limit) | (0x1 << 2)) & 0x7ffffull;
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_qac_drop_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(
        d_pipe, port->port);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
    setp_tof3_qac_port_config_entry_port_thrd(&lo, port->qac_drop_limit);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_wac_drop_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  uint32_t wac_limit;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, 0);
  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_th(
      d_pipe, port->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (BF_SUCCESS != rc) {
    return (rc);
  }
  wac_limit = TM_CLEAR_LOW_3_BITS(lo);
  port->wac_drop_limit = wac_limit;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_qac_drop_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  uint32_t qac_limit = 0;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(
        d_pipe, port->port);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
  }

  qac_limit = getp_tof3_qac_port_config_entry_port_thrd(&lo);
  port->qac_drop_limit = qac_limit;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_qac_hyst_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr;
  uint8_t hyst_index = 0;
  uint32_t d_inst;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  // Based on resume limit, find offset profile index in QAC
  rc = bf_tm_tof3_populate_qac_offset_profile(
      devid, port->p_pipe, port->qac_resume_limit, &hyst_index);
  if (BF_SUCCESS != rc) return rc;

  // The port's QAC limit value is either registered in SW
  // or it was just added and written to HW.
  port->qac_hyst_index = hyst_index;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);

    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(
        d_pipe, port->port);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
    setp_tof3_qac_port_config_entry_offset_idx(&lo, port->qac_hyst_index);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
    if (BF_SUCCESS != rc) {
      return (rc);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_qac_hyst_limit(bf_dev_id_t devid,
                                                  bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint32_t cells_u = 0;
  uint8_t hyst_index = 0;
  uint32_t d_inst;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(d_pipe,
                                                                   port->port);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (rc != BF_SUCCESS) {
      return (rc);
    }
    hyst_index = getp_tof3_qac_port_config_entry_offset_idx(&lo);

    rc = bf_tm_read_register(
        devid,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.offset_profile[hyst_index]),
        &cells_u);
    if (rc != BF_SUCCESS) {
      return (rc);
    }
    // Check (hyst_index, cells_u) tuple is consistent with SW profiles.
    // Restore SW hysteresis profiles from HW if not done yet.
    rc = bf_tm_tof3_check_qac_offset_profile(
        devid, port->p_pipe, cells_u, hyst_index);
    if (rc != BF_SUCCESS) {
      return (rc);
    }
  }
  port->qac_hyst_index = hyst_index;
  port->qac_resume_limit = TM_8CELL_UNITS_TO_CELLS(cells_u);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_uc_ct_limit(bf_dev_id_t devid,
                                               bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  int num_ch = 1;
  uint32_t spd = p->speed;
  bool ct_enb = p->ct_enabled;
  uint8_t ct_lim = p->uc_cut_through_limit;

  // Set default CT limits which can be modified by APIs at later point
  switch (p->speed) {
    case BF_SPEED_100G:
      num_ch = 1;
      break;
    case BF_SPEED_200G:
      num_ch = 2;
      break;
    case BF_SPEED_400G:
      num_ch = 4;
      break;
    default:
      num_ch = 1;
      break;
  }

  for (int i = 0; i < num_ch; i++) {
    p->speed = spd;
    p->uc_cut_through_limit = ct_lim;
    p->ct_enabled = ct_enb;
    rc |= bf_tm_tof3_port_set_cut_through(devid, p);
    p++;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_uc_ct_limit(bf_dev_id_t devid,
                                               bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_clc_top.clc_common[0]
                   .qclc_pt_spd[p->p_pipe][p->port]),
      &val);
  // Set value for passed in port
  p->uc_cut_through_limit = getp_tof3_qclc_pt_spd_pt_ct_th(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_skid_limit(bf_dev_id_t devid,
                                              bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, 0);

  // Bit2 should be set for the config to take effect
  val = (port->skid_limit) | (1 << 2);
  setp_tof3_wac_port_th_entry_cnt(&lo, val);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_hdr_th(d_pipe,
                                                                 port->port);

  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_flowcontrol_mode(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t icosmask, i, icos_to_cos_mask = 0;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  if (p->fc_type == BF_TM_PAUSE_NONE) {
    // No PFC or Port level pause.
    // clear pfc / port pause
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.port_config[p->port]),
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

    // Before enabling PFC, disable port pause first, enable PFC
    setp_tof3_wac_port_config_pause_en(&val, 0);
    setp_tof3_wac_port_config_pfc_en(&val, icos_to_cos_mask);
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.port_config[p->port]),
        val);
  }
  if (p->fc_type == BF_TM_PAUSE_PORT) {
    setp_tof3_wac_port_config_pause_en(&val, 1);
    setp_tof3_wac_port_config_pfc_en(&val, 0);
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.port_config[p->port]),
        val);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_flowcontrol_mode(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.port_config[p->port]),
      &val);

  p->fc_type = BF_TM_PAUSE_NONE;
  if (getp_tof3_wac_port_config_pause_en(&val)) {
    p->fc_type = BF_TM_PAUSE_PORT;
  }
  if (getp_tof3_wac_port_config_pfc_en(&val)) {
    p->fc_type = BF_TM_PAUSE_PFC;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_pfc_enable_mask(bf_dev_id_t devid,
                                                   bf_tm_port_t *p,
                                                   uint8_t *enable_mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.port_config[p->port]),
      &val);

  *enable_mask = getp_tof3_wac_port_config_pfc_en(&val);
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_set_flowcontrol_rx(bf_dev_id_t devid,
                                                  bf_tm_port_t *p) {
  /*
   * For tof3_reg, this is a NO-OP as pfc_upd_enb bit should be
   * set always as per HW team recommendation
   */
  (void)devid;
  (void)p;
  return BF_TM_EOK;
}

bf_tm_status_t bf_tm_tof3_port_get_flowcontrol_rx(bf_dev_id_t devid,
                                                  bf_tm_port_t *p) {
  return (bf_tm_tof3_sch_get_port_sched(devid, p));
}

bf_tm_status_t bf_tm_tof3_port_set_pfc_cos_map(bf_dev_id_t devid,
                                               bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  int i, j;
  uint8_t icosmask, icos_to_cos_mask, port_icos_to_cos_mask = 0;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  // collect all PPGs for port.
  // For each PPG, collect icos_mask
  // for each PPG's icos-mask, prepare reverse mask using p->cos_to_icos
  // mapping program reverse mask into wac_ppg_icos_mapping.entry[ppg->ppg]

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
      setp_tof3_wac_ppg_icos_entry_icos(&lo, icos_to_cos_mask);
      port_icos_to_cos_mask |= icos_to_cos_mask;
      uint64_t indir_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_icos(
              p->ppgs[i]->d_pipe, p->ppgs[i]->ppg);
      rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
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
  // bf_tm_tof3_ppg_set_pfc_treatment().  Before TM generated PFC message,
  // PFC enable bit is checked. PFC enable bit is one-to-one mapping of
  // packet-cos bit. [ when PPG crosses thresholds, icos is reverse mapped to
  // cos
  // and PFC enable bit vector is checked to generate PFC message]

  // Hence whenever port PFC cos map is setup, go over PPGs of the port and
  // for PPGs that have PFC treatment, enable PFC generation bit. So call
  // bf_tm_tof3_port_set_flowcontrol_mode() for Tofino2 as link pause enable
  // and PFC enable config are part of the same register - port_config.

  rc |= bf_tm_tof3_port_set_flowcontrol_mode(devid, p);

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
bf_tm_status_t bf_tm_tof3_port_get_pfc_cos_mask(bf_dev_id_t devid,
                                                bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t icos_to_cos_mask = 0;
  uint64_t hi;
  uint64_t lo;
  int i;
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (p->ppgs[i] && !(p->ppgs[i]->is_default_ppg)) {
      uint64_t indir_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_icos(
              p->ppgs[i]->d_pipe, p->ppgs[i]->ppg);
      rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
      icos_to_cos_mask |= getp_tof3_wac_ppg_icos_entry_icos(&lo);
    }
  }
  p->icos_to_cos_mask = icos_to_cos_mask;
  return (rc);
}

/* This function does read-modify-write. Do not invoke this
 * in fast-reconfig / hitless HA restore code.
 */
bf_tm_status_t bf_tm_tof3_port_set_cpu_port(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, i;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (i = 0; i < g_tm_ctx[devid]->tm_cfg.pipe_cnt; i++) {
    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(i, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(i, d_inst);

      rc |= bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[d_pipe].ctrl),
          &val);
      setp_tof3_pre_ctrl_c2c_port(&val, p->port);
      setp_tof3_pre_ctrl_c2c_enable(&val, 1);
      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[d_pipe].ctrl),
          val);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_ingress_drop_cntr(bf_dev_id_t devid,
                                                     bf_tm_port_t *p,
                                                     uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_port(d_pipe,
                                                                     p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  *count = getp_tof3_wac_drop_count_port_drop_cnt_cnt(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_clear_ingress_drop_cntr(bf_dev_id_t devid,
                                                       bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_port(d_pipe,
                                                                     p->port);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_egress_drop_cntr(bf_dev_id_t devid,
                                                    bf_tm_port_t *p,
                                                    uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);
  uint64_t hi = 0;
  uint64_t lo = 0;
  uint64_t indir_addr;

  // Green drop count
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_SUCCESS) {
    return rc;
  }
  *count = getp_tof3_qac_drop_count_port_entry_count(&lo);

  // Yellow drop count
  lo = 0;
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port + 36);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_SUCCESS) {
    return rc;
  }
  *count += getp_tof3_qac_drop_count_port_entry_count(&lo);

  // Red drop count
  lo = 0;
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port + 72);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_SUCCESS) {
    return rc;
  }
  *count += getp_tof3_qac_drop_count_port_entry_count(&lo);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_clear_egress_drop_cntr(bf_dev_id_t devid,
                                                      bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);
  uint64_t indir_addr;

  // Green drop count
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  // Yellow drop count
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port + 36);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  // Red drop count
  indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
      d_pipe, p->port + 72);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, 0, 0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_ingress_usage_cntr(bf_dev_id_t devid,
                                                      bf_tm_port_t *p,
                                                      uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_min_cnt(
      d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_TM_EOK) {
    return rc;
  }
  *count = lo;

  lo = 0;
  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_cnt(
      d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_TM_EOK) {
    return rc;
  }
  *count += lo;

  lo = 0;
  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_hdr_cnt(
      d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_TM_EOK) {
    return rc;
  }
  *count += lo;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_egress_usage_cntr(bf_dev_id_t devid,
                                                     bf_tm_port_t *p,
                                                     uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_cell_count(d_pipe,
                                                                     p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  *count = getp_tof3_qac_port_cell_count_entry_port_cell_count(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_ingress_wm_cntr(bf_dev_id_t devid,
                                                   bf_tm_port_t *p,
                                                   uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_wm_cnt(d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  *count = getp_tof3_wac_pg_wm_cnt_wm_cnt_cnt(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_clear_ingress_wm_cntr(bf_dev_id_t devid,
                                                     bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_wm_cnt(d_pipe, p->port);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, 0);

  return rc;
}

bf_tm_status_t bf_tm_tof3_port_get_egress_wm_cntr(bf_dev_id_t devid,
                                                  bf_tm_port_t *p,
                                                  uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_wm_cell_count(
          d_pipe, p->port);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  *count = getp_tof3_qac_port_wm_cell_count_entry_cell_count(&lo);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_clear_egress_wm_cntr(bf_dev_id_t devid,
                                                    bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_wm_cell_count(
          d_pipe, p->port);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, 0);

  return rc;
}

/* marked as missing with hw team
bf_tm_status_t bf_tm_tof2_pipe_get_total_in_cells(bf_dev_id_t devid,
                                                  int pipe,
                                                  uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;

  bf_tm_subdev_read_register(devid,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_clc_top.clc[pipe]
                                   .inport_cell_cnt.inport_cell_cnt_0_2),
                      &val);
  cnt = val;
  bf_tm_subdev_read_register(devid,
                      offsetof(tof2_reg,
                               device_select.tm_top.tm_clc_top.clc[pipe]
                                   .inport_cell_cnt.inport_cell_cnt_1_2),
                      &val);
  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;
  return (rc);
}
*/

bf_tm_status_t bf_tm_tof3_pipe_get_total_in_pkts(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, cnt;
  uint64_t reg_cnt;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.ctr_vld_sop.ctr_ctrl_pkt_0_2),
      &val);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  cnt = val;
  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.ctr_vld_sop.ctr_ctrl_pkt_1_2),
      &val);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  reg_cnt = val;
  *count = (reg_cnt << 32) + cnt;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_clear_total_in_pkts(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.ctr_vld_sop.ctr_ctrl_pkt_0_2),
      0);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                   .wac_reg.ctr_vld_sop.ctr_ctrl_pkt_1_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_get_uc_ct_count(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[d_pipe].uc_ct_cnt),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_clear_uc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[d_pipe].uc_ct_cnt),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_get_mc_ct_count(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[d_pipe].mc_ct_cnt),
      &val);
  *count = val;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_clear_mc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[d_pipe].mc_ct_cnt),
      0);

  return (rc);
}

#define PORT_SPEED_MODE_10G 0
#define PORT_SPEED_MODE_25G 1
#define PORT_SPEED_MODE_40G 2
#define PORT_SPEED_MODE_50G 3
#define PORT_SPEED_MODE_UNUSED 4
#define PORT_SPEED_MODE_100G 5
#define PORT_SPEED_MODE_200G 6
#define PORT_SPEED_MODE_400G 7

bf_status_t bf_tm_tof3_port_mgd_pt_spd(bf_dev_id_t devid,
                                       bf_tm_port_t *p,
                                       uint8_t enbl) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t num_chnl = 1;
  uint32_t i;
  uint32_t mgd_spd = 0;  // 2 bit value used for fifo values for MGD for
                         // different speeds. {0, 1, 2, 3}

  if (!p || p->port >= (BF_TM_TOF3_PG_PER_PIPE * BF_TM_TOF3_PORTS_PER_PG)) {
    return BF_INVALID_ARG;
  }

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  switch (p->speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      mgd_spd = 0;
      num_chnl = 1;
      break;
    case BF_SPEED_100G:
      mgd_spd = 1;
      num_chnl = 1;
      break;
    case BF_SPEED_200G:
      if (p->port % 2 != 0) {
        return BF_INVALID_ARG;
      }
      mgd_spd = 2;
      num_chnl = 2;
      break;
    case BF_SPEED_400G:
      if (p->port % 4 != 0) {
        return BF_INVALID_ARG;
      }
      mgd_spd = 3;
      num_chnl = 4;
      break;
    default:
      mgd_spd = 0;
      num_chnl = 1;
      break;
  }

  do {
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd
                     .mgd_cfg_chan_rate[d_pipe][p->port / 4]),
        &val);
    if (rc != BF_TM_EOK) {
      LOG_ERROR(
          "bf_tm_tof3_port_mgd_pt_spd: chan_rate read failed pipe %d"
          " port %d status 0x%0x",
          p->p_pipe,
          p->port,
          rc);
      break;
    }

    for (i = 0; i < num_chnl; i++) {
      val &= ~(3 << (((p->port % 4) + i) * 2));
      val |= (mgd_spd << (((p->port % 4) + i) * 2));
    }

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd
                     .mgd_cfg_chan_rate[d_pipe][p->port / 4]),
        val);
    if (rc != BF_TM_EOK) {
      LOG_ERROR(
          "bf_tm_tof3_port_mgd_pt_spd: chan_rate write failed pipe %d"
          " port %d status 0x%0x",
          p->p_pipe,
          p->port,
          rc);
      break;
    }

    val = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd
                     .mgd_cfg_chan_enbl[d_pipe][p->port / 4]),
        &val);
    if (rc != BF_TM_EOK) {
      LOG_ERROR(
          "bf_tm_tof3_port_mgd_pt_spd: chan_enbl read failed pipe %d"
          " port %d status 0x%0x",
          p->p_pipe,
          p->port,
          rc);
      break;
    }

    for (i = 0; i < num_chnl; i++) {
      val &= ~(1 << (((p->port % 4) + i) * 1));
      val |= ((enbl & 1) << (((p->port % 4) + i) * 1));
    }

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd
                     .mgd_cfg_chan_enbl[d_pipe][p->port / 4]),
        val);
    if (rc != BF_TM_EOK) {
      LOG_ERROR(
          "bf_tm_tof3_port_mgd_pt_spd: chan_enbl write failed pipe %d"
          " port %d status 0x%0x",
          p->p_pipe,
          p->port,
          rc);
      break;
    }
  } while (0);
  return rc;
}

bf_status_t bf_tm_tof3_port_smgc_pt_spd(bf_dev_id_t devid,
                                        uint8_t pipe_num,
                                        uint8_t port,
                                        uint8_t pt_spd,
                                        uint8_t enbl) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe_num, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe_num, 0);

  switch (port / 4) {
    case 0: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac0_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac0_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac0_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac0_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac0_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac0_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac0_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac0_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac0_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 1: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac1_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac1_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac1_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac1_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac1_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac1_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac1_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac1_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac1_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 2: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac2_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac2_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac2_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac2_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac2_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac2_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac2_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac2_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac2_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 3: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac3_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac3_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac3_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac3_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac3_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac3_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac3_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac3_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac3_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 4: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac4_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac4_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac4_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac4_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac4_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac4_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac4_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac4_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac4_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 5: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac5_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac5_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac5_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac5_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac5_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac5_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac5_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac5_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac5_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 6: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac6_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac6_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac6_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac6_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac6_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac6_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac6_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac6_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac6_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 7: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac7_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac7_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac7_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac7_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac7_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac7_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac7_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac7_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac7_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 8: {
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac8_rates),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac8_rates read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
      setp_tof3_tm_smgc_rspec_mac8_rates_chnl_rate(&val, port % 4, pt_spd);

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac8_rates),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac8_rates write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac8_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac8_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.smgc[d_pipe].mac8_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_smgc_pt_spd: mac8_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    default: {
      LOG_ERROR("bf_tm_tof3_port_smgc_pt_spd: pipe %d invalid port %d",
                pipe_num,
                port);
      return BF_INVALID_ARG;
    } break;
  }
  return rc;
}

bf_status_t bf_tm_tof3_port_rmgc_pt_spd(bf_dev_id_t devid,
                                        uint8_t pipe_num,
                                        uint8_t port,
                                        uint8_t pt_spd,
                                        uint8_t enbl) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe_num, 0);
  // Get the remote Subdev_id
  bf_subdev_id_t rsubdev_id = BF_TM_2DIE_SUBDEV_ID(pipe_num, 1);

  (void)pt_spd;

  switch (port / 4) {
    case 0: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac0_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac0_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac0_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac0_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 1: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac1_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac1_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac1_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac1_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 2: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac2_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac2_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac2_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac2_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 3: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac3_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac3_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac3_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac3_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 4: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac4_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac4_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac4_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac4_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 5: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac5_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac5_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac5_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac5_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 6: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac6_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac6_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac6_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac6_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 7: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac7_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac7_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac7_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac7_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    case 8: {
      val = 0;
      rc = bf_tm_subdev_read_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac8_en),
          &val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac8_en read failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }

      val &= ~(1 << (port % 4));
      val |= ((enbl & 1) << (port % 4));

      rc |= bf_tm_subdev_write_register(
          devid,
          rsubdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_mgc_top.rmgc[d_pipe].mac8_en),
          val);
      if (rc != BF_TM_EOK) {
        LOG_ERROR(
            "bf_tm_tof3_port_rmgc_pt_spd: mac8_en write failed pipe %d"
            " port %d status 0x%0x",
            pipe_num,
            port,
            rc);
        break;
      }
    } break;
    default: {
      LOG_ERROR("bf_tm_tof3_port_rmgc_pt_spd: pipe %d invalid port %d",
                pipe_num,
                port);
      return BF_INVALID_ARG;
    } break;
  }
  return rc;
}

bf_status_t bf_tm_tof3_port_pex_pt_spd(bf_dev_id_t devid,
                                       uint8_t pipe_num,
                                       uint8_t port,
                                       uint8_t pt_spd,
                                       uint8_t enbl) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    val = 0;
    setp_tof3_pex_pt_spd_pt_spd(&val, pt_spd);

    d_pipe = BF_TM_2DIE_D_PIPE(pipe_num, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe_num, d_inst);

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[d_pipe].pex_pt_spd[port]),
        val);
    if (rc != BF_TM_EOK) {
      LOG_ERROR(
          "bf_tm_tof3_port_pex_pt_spd: pex_pt_spd write failed pipe %d"
          " port %d status 0x%0x",
          pipe_num,
          port,
          rc);
      return rc;
    }

    switch (port / 4) {
      case 0: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac0_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac0_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac0_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac0_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 1: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac1_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac1_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac1_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac1_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 2: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac2_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac2_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac2_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac2_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 3: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac3_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac3_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac3_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac3_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 4: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac4_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac4_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac4_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac4_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 5: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac5_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac5_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac5_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac5_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 6: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac6_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac6_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac6_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac6_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 7: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac7_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac7_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac7_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac7_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      case 8: {
        val = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac8_en),
            &val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac8_en read failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }

        val &= ~(1 << (port % 4));
        val |= ((enbl & 1) << (port % 4));

        rc |= bf_tm_subdev_write_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_pex_top.pex[d_pipe].mac8_en),
            val);
        if (rc != BF_TM_EOK) {
          LOG_ERROR(
              "bf_tm_tof3_port_pex_pt_spd: mac8_en write failed pipe %d"
              " port %d status 0x%0x",
              pipe_num,
              port,
              rc);
          break;
        }
      } break;
      default: {
        LOG_ERROR("bf_tm_tof3_port_pex_pt_spd: pipe %d invalid port %d",
                  pipe_num,
                  port);
        return BF_INVALID_ARG;
      } break;
    }
  }
  return rc;
}

/* Not exposed as API. This is internal to TM. */
bf_status_t bf_tm_tof3_port_set_cut_through(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0, pt_speed;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  // Also set in CLC (internal block of TM) port-speed
  // and enable cut-through mode by default which can be overridden if
  // needed..

  switch (p->speed) {
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
      pt_speed = 5;
      break;
    case BF_SPEED_200G:
      pt_speed = 6;
      break;
    case BF_SPEED_400G:
      pt_speed = 7;
      break;
    case BF_SPEED_NONE:
      pt_speed = 3;
      break;
    default:
      pt_speed = 7;
      break;
  }
  setp_tof3_qclc_pt_spd_pt_spd(&val, pt_speed);

  // Enable/Disable Cut-through.
  if (p->ct_enabled) {
    setp_tof3_qclc_pt_spd_pt_ct_en(&val, 1);
  } else {
    setp_tof3_qclc_pt_spd_pt_ct_en(&val, 0);
  }

  // Set cut-through limit.
  setp_tof3_qclc_pt_spd_pt_ct_th(&val, p->uc_cut_through_limit);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    rc |= bf_tm_subdev_write_register(
        devid,
        (bf_subdev_id_t)d_inst,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc_common[0]
                     .qclc_pt_spd[p->p_pipe][p->port]),
        val);
  }

  val = 0;
  unsigned int sub_reg = p->port / 32;
  bf_tm_port_t *cur;
  for (int i = 0; i < 32; i++) {
    cur = g_tm_ctx[devid]->ports +
          (p->p_pipe * ((g_tm_ctx[devid]->tm_cfg.ports_per_pg *
                         g_tm_ctx[devid]->tm_cfg.pg_per_pipe) +
                        g_tm_ctx[devid]->tm_cfg.mirror_port_cnt)) +
          (sub_reg * 32) + i;
    if (cur->ct_enabled) {
      setp_tof3_wac_port_ct_dis_wac_ct_dis(&val, i, 0);
    } else {
      setp_tof3_wac_port_ct_dis_wac_ct_dis(&val, i, 1);
    }
    if (sub_reg == 1 && i >= 4) {
      break;
    }
  }

  d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0);
  subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

  size_t offs = 0;
  if (sub_reg == 0) {
    offs = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                        .wac_reg.wac_port_ct_dis.wac_port_ct_dis_0_2);
  } else {
    offs = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                        .wac_reg.wac_port_ct_dis.wac_port_ct_dis_1_2);
  }
  rc |= bf_tm_subdev_write_register(devid, subdev_id, offs, val);

  return (rc);
}

bf_status_t bf_tm_tof3_port_get_cut_through(bf_dev_id_t devid,
                                            bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_clc_top.clc_common[0]
                   .qclc_pt_spd[p->p_pipe][p->port]),
      &val);

  switch (getp_tof3_qclc_pt_spd_pt_spd(&val)) {
    case 0:
      p->speed = BF_SPEED_10G;
      break;
    case 1:
      p->speed = BF_SPEED_25G;
      break;
    case 2:
      p->speed = BF_SPEED_40G;
      break;
    case 3:
      p->speed = BF_SPEED_50G;
      break;
    case 5:
      p->speed = BF_SPEED_100G;
      break;
    case 6:
      p->speed = BF_SPEED_200G;
      break;
    case 7:
      p->speed = BF_SPEED_400G;
      break;
    default:
      p->speed = BF_SPEED_400G;
      break;
  }

  p->ct_enabled = getp_tof3_qclc_pt_spd_pt_ct_en(&val);

  return (rc);
}

static bf_tm_status_t bf_tm_tof3_set_queue_state(bf_dev_id_t devid,
                                                 bf_tm_port_t *p,
                                                 bool reset) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOF3_QUEUES_PER_PG];
  bf_dev_port_t port;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  rc = bf_tm_port_q_mapping_get(devid, port, &q_count, q_mapping);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: set_qacq getting queue count failed for dev %d, l_pipe %d l_port "
        "%d, rc %s (%d)",
        devid,
        p->l_pipe,
        p->uport,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  bf_tm_eg_q_t *queue;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = 0;
  bf_tm_queue_t q;

  for (q = 0; q < q_count; q++) {
    rc = bf_tm_q_get_descriptor(devid, port, q, &queue);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "TM: set_qacq failed to get q desc for dev %d, l_pipe %d l_port %d, "
          "queue %d rc %s (%d)",
          devid,
          p->l_pipe,
          p->uport,
          q,
          bf_err_str(rc),
          rc);
      return (rc);
    }

    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      // We need to update both source pipe and pipe + 4 in a 2 Die System
      // Example
      // Die0 Pipe 0 Die0 Pipe 4
      // Die1 Pipe 2 Die1 Pipe 6 etc...
      d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, 0) + d_inst * BF_SUBDEV_PIPE_COUNT;
      subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, 0);

      hi = 0, lo = 0;
      indir_addr = tof3_mem_tm_tm_wac_wac_common_mem_wac_common_mem_lcl_qstate(
          d_pipe, queue->physical_q);

      if (!reset) {
        lo = 0xf;
      }
      rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, hi, lo);
      if (BF_TM_IS_NOTOK(rc)) {
        LOG_ERROR(
            "TM: set qstate failed to ind wr for dev %d, subdev_id %d l_pipe "
            "%d l_port %d, "
            "queue %d rc %s (%d)",
            devid,
            subdev_id,
            p->l_pipe,
            p->uport,
            q,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }
  return rc;
}

/* Disable/Enable qac_port_rx_disable bit */
static bf_tm_status_t bf_tm_tof3_set_port_rx(bf_dev_id_t devid,
                                             bf_tm_port_t *p) {
  uint32_t val_0_3 = 0, val_1_3 = 0, val_2_3 = 0, i;
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_port_t *localp;
  bf_dev_port_t devport;
  uint32_t *val;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  // use shadow memory as value buffer

  // The entire wide register needs to be updated even if single
  // bit needs to be changed.

  // port 32 to 35 state
  devport = MAKE_DEV_PORT(p->l_pipe, 32);
  devport = lld_sku_map_devport_from_device_to_user(devid, devport);

  rc = bf_tm_port_get_descriptor(devid, devport, &localp);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: set_port_rx port port descriptor get failed for dev %d, l_pipe %d "
        "devport "
        "%d, rc %s (%d)",
        devid,
        p->l_pipe,
        devport,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  if (p->qac_rx_enable == false) {
    // Need ignore_uc_drop_st to be set to 0 for 12T and during dynamic
    // reconfig for 25T
    val = &((g_tm_ctx[devid]->pipes + p->l_pipe)->qac_pipe_config);
    setp_tof3_qac_pipe_config_ignore_uc_drop_st(val, 0);

    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, d_inst);

      rc = bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.pipe_config),
          *val);

      if (BF_TM_IS_NOTOK(rc)) {
        LOG_ERROR(
            "TM: set_port_rx port pipe_config failed for dev %d, subdev %d "
            "l_pipe %d "
            "l_port "
            "%d, rc %s (%d)",
            devid,
            subdev_id,
            p->l_pipe,
            p->uport,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }

  for (i = 0; i < 4; i++) {
    val_1_3 |= ((localp->qac_rx_enable) ? 0u : 1u) << i;
    localp++;
  }

  // port 0 to 31 state
  devport = MAKE_DEV_PORT(p->l_pipe, 0);
  devport = lld_sku_map_devport_from_device_to_user(devid, devport);

  rc = bf_tm_port_get_descriptor(devid, devport, &localp);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: set_port_rx port port descriptor get failed for dev %d, l_pipe %d "
        "devport "
        "%d, rc %s (%d)",
        devid,
        p->l_pipe,
        devport,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  for (i = 0; i < 32; i++) {
    val_0_3 |= ((localp->qac_rx_enable) ? 0u : 1u) << i;
    localp++;
  }

  LOG_DBG(
      "dev:%d l_pipe:%d l_port:%d QAC Rx set to %d (0x%08x, 0x%08x, 0x%08x)",
      devid,
      p->l_pipe,
      p->uport,
      p->qac_rx_enable,
      val_0_3,
      val_1_3,
      val_2_3);

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, d_inst);

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_0_3),
        val_0_3);
    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_1_3),
        val_1_3);

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_2_3),
        val_2_3);

    rc |= bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_0_3),
        &val_0_3);

    rc |= bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_1_3),
        &val_1_3);

    rc |= bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_port_rx_disable.qac_port_rx_disable_2_3),
        &val_2_3);

    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "TM: set_port_rx port port_rx_disable failed for dev %d, subdev %d "
          "l_pipe %d "
          "l_port "
          "%d, rc %s (%d)",
          devid,
          subdev_id,
          p->l_pipe,
          p->uport,
          bf_err_str(rc),
          rc);
      return (rc);
    }

    LOG_DBG(
        "dev:%d/%d d_pipe:%d l_pipe:%d l_port:%d QAC Rx check (0x%08x, "
        "0x%08x, 0x%08x)",
        devid,
        subdev_id,
        d_pipe,
        p->l_pipe,
        p->uport,
        val_0_3,
        val_1_3,
        val_2_3);
  }

  /*
   * If qac_rx is getting disabled now due to port-disable or port-delete,
   * make sure TM buffers get flushed and usage d_inst
   * goes to zero. This shouldn't be done for fast reconfig and
   * hitless scenarios. Also, skip this for model target.
   */
  if (p->qac_rx_enable == false && tm_is_device_locked(devid) == false &&
      TM_IS_TARGET_ASIC(devid)) {
    bf_dev_port_t port = MAKE_DEV_PORT(p->l_pipe, p->uport);
    // port = lld_sku_map_devport_from_device_to_user(devid, port);

    // Flush all queues
    LOG_TRACE(
        "%s: flushing queues for dev %d devport %d....", __func__, devid, port);
    rc = bf_tm_port_all_queues_flush(devid, port);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "TM: set_port_rx port port flush failed for dev %d, l_pipe %d "
          "l_port "
          "%d, rc %s (%d)",
          devid,
          p->l_pipe,
          p->uport,
          bf_err_str(rc),
          rc);
      return (rc);
    }
  }

  // Need to set back the uc_drop_st to 1 on Multi-Die Port Enable.
  if (p->qac_rx_enable == true) {
    val = &((g_tm_ctx[devid]->pipes + p->l_pipe)->qac_pipe_config);
    // Set the ignore_uc_drop_st back to 1 after port add complete for 2 Die

    if (g_tm_ctx[devid]->subdev_count > 1) {
      setp_tof3_qac_pipe_config_ignore_uc_drop_st(val, 1);
    } else {
      setp_tof3_qac_pipe_config_ignore_uc_drop_st(val, 0);
    }

    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      d_pipe = BF_TM_2DIE_D_PIPE(p->p_pipe, d_inst);
      subdev_id = BF_TM_2DIE_SUBDEV_ID(p->p_pipe, d_inst);

      rc = bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                       .qac_reg.pipe_config),
          *val);
      if (BF_TM_IS_NOTOK(rc)) {
        LOG_ERROR(
            "TM: set_port_rx port pipe_config failed for dev %d, subdev %d "
            "l_pipe %d "
            "l_port "
            "%d, rc %s (%d)",
            devid,
            subdev_id,
            p->l_pipe,
            p->uport,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }

  rc = bf_tm_tof3_set_queue_state(devid, p, p->qac_rx_enable);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: set_port_rx port enable/add failed for dev %d, l_pipe %d "
        "l_port "
        "%d, rc %s (%d)",
        devid,
        p->l_pipe,
        p->uport,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  return (rc);
}

static bf_tm_status_t bf_tm_tof3_port_speed_based_default_cfg(bf_dev_id_t devid,
                                                              bf_tm_port_t *p) {
  switch (p->speed) {
    case PORT_SPEED_MODE_200G: {
      if (p->port & 1) return BF_INVALID_ARG;
    } break;
    case PORT_SPEED_MODE_400G: {
      if (p->port & 3) return BF_INVALID_ARG;
    } break;
  }

  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_thres_t ct_lim = 0;
  uint32_t pt_speed, num_ch = 1;

  // Set default CT limits which can be modified by APIs at later point
  // Set Port Speed for CLC, PEX, SMGC
  switch (p->speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
      pt_speed = 0;
      // ct_lim = 4;
      num_ch = 1;
      break;
    case BF_SPEED_25G:
      pt_speed = 1;
      // ct_lim = 4;
      num_ch = 1;
      break;
    case BF_SPEED_40G:
      pt_speed = 2;
      // ct_lim = 8;
      num_ch = 1;
      break;
    case BF_SPEED_50G:
      pt_speed = 3;
      // ct_lim = 8;
      num_ch = 1;
      break;
    case BF_SPEED_100G:
      pt_speed = 5;
      // ct_lim = 0xf;
      num_ch = 1;
      break;
    case BF_SPEED_200G:
      pt_speed = 6;
      // ct_lim = 0x7f;
      num_ch = 2;
      break;
    case BF_SPEED_400G:
      pt_speed = 7;
      // ct_lim = 0x7f;
      num_ch = 4;
      break;
    case BF_SPEED_NONE:
      pt_speed = 3;
      // ct_lim = 8;
      num_ch = 1;
      break;
    default:
      pt_speed = 7;
      // ct_lim = 0x7f;
      num_ch = 4;
      break;
  }

  // TODO : This value is used in Emulation. Need to check if this
  // is the value for all speeds
  ct_lim = 0x30;

  p->uc_cut_through_limit = ct_lim;
  rc = bf_tm_tof3_port_set_uc_ct_limit(devid, p);

  // TODO : Set Port PEX Speed.

  uint32_t i = 0;
  bf_tm_port_t *p_cur = p;
  for (i = 0; i < num_ch; i++, p_cur++) {
    rc |= bf_tm_tof3_port_pex_pt_spd(
        devid, p_cur->p_pipe, p_cur->port, pt_speed, 1);
    rc |= bf_tm_tof3_port_smgc_pt_spd(
        devid, p_cur->p_pipe, p_cur->port, pt_speed, 1);

    if (g_tm_ctx[devid]->subdev_count > 1) {
      rc |= bf_tm_tof3_port_rmgc_pt_spd(
          devid, p_cur->p_pipe, p_cur->port, pt_speed, 1);
    }
  }

  if (g_tm_ctx[devid]->subdev_count > 1) {
    rc |= bf_tm_tof3_port_mgd_pt_spd(devid, p, 1);
  }
  return (rc);
}

/*
 * Neccessary config applied to ensure Egress path in TM  is setup
 * before packets from the port that is being added arrive inside TM.
 */
bf_tm_status_t bf_tm_tof3_add_new_port(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_port_t port;
  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  uint8_t q_count = 0;
  uint8_t q_mapping[BF_TM_TOF3_QUEUES_PER_PG];

  // Set default config for the port
  rc = bf_tm_tof3_port_speed_based_default_cfg(devid, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "Not able to set default port config for dev %d pipe %d port %d, rc "
        "%s "
        "(%d)",
        devid,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
  }
  if (BF_TM_IS_OK(rc)) {
    // Enable Shaper to default value
    rc = bf_tm_tof3_sch_set_port_rate(devid, (void *)p);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "Not able to enable default port shaper for dev %d pipe %d port "
          "%d, "
          "rc %s "
          "(%d)",
          devid,
          p->p_pipe,
          p->port,
          bf_err_str(rc),
          rc);
    }
  }

  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  // port = lld_sku_map_devport_from_device_to_user(devid, port);

  if (BF_TM_IS_OK(rc)) {
    // Get the queue count for the port
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
    // Minimum q_count is 16 per port
    for (q = 0; q < q_count; q++) {
      rc = bf_tm_q_get_descriptor(devid, port, q, &queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      // Set channel ID to which queue belongs to.
      int port_ch = DEV_PORT_TO_LOCAL_PORT(p->port) %
                    g_tm_ctx[devid]->tm_cfg.ports_per_pg;
      queue->q_sch_cfg.cid = port_ch;
      queue->q_sch_cfg.max_rate = p->port_rate;
      queue->q_sch_cfg.min_rate = 0;
      rc = bf_tm_tof3_sch_set_q_rate(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      rc = bf_tm_tof3_sch_set_q_min_rate(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }
      // Per HW team, SCH PFC has to be enabled always
      queue->q_sch_cfg.sch_pfc_enabled = true;
      rc = bf_tm_tof3_sch_set_q_sched(devid, queue);
      if (rc != BF_SUCCESS) {
        break;
      }

      // Assign HW queues according to the port's current queue carving
      // and scheduling speed.
      rc = bf_tm_tof3_vq_hq_map(devid, p, queue, q, false);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to allocate queues for pipe %d port %d speed %d. Consider "
            "to reduce number of queues carved in the port's group, rc=%d",
            p->p_pipe,
            p->port,
            p->speed,
            rc);
        break;
      }
    }
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("Could not bind queues to pipe %d port %d", p->p_pipe, p->port);
      return (rc);
    }
  }

  rc = bf_tm_init_qoc_for_port(devid, p, q_count);
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
    rc = bf_tm_tof3_set_port_rx(devid, p);
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
bf_tm_status_t bf_tm_tof3_delete_port(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_port_t port;
  uint q;
  bf_tm_port_t *base_port = p;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOF3_QUEUES_PER_PG];

  // We duplicate bf_tm_tof3_port_set_uc_ct_limit for now
  // to avoid overwriting the speed - sch.port_config needs to know it.
  bf_port_speeds_t old_speed = p->speed;
  int num_ch = 1;
  switch (p->speed) {
    case BF_SPEED_100G:
      num_ch = 1;
      break;
    case BF_SPEED_200G:
      num_ch = 2;
      break;
    case BF_SPEED_400G:
      num_ch = 4;
      break;
    default:
      num_ch = 1;
      break;
  }

  for (int i = 0; i < num_ch; i++) {
    p->speed = BF_SPEED_NONE;
    p->ct_enabled = false;
    rc |= bf_tm_tof3_port_set_cut_through(devid, p);
    p->speed = old_speed;  // We don't want to overwrite port speed yet.
    p++;
  }
  p = base_port;
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
  rc = bf_tm_tof3_set_port_rx(devid, p);
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
  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  // port = lld_sku_map_devport_from_device_to_user(devid, port);

  // flush all queues
  rc = bf_tm_port_all_queues_flush(devid, port);
  if (BF_TM_IS_NOTOK(rc)) {
    // error logging implemented in callee
    return rc;
  }
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

  int i = 0;
  bf_tm_port_t *p_cur = p;
  for (i = 0; i < num_ch; i++, p_cur++) {
    rc |= bf_tm_tof3_port_pex_pt_spd(
        devid, p_cur->p_pipe, p_cur->port, BF_SPEED_NONE, 0);
    rc |= bf_tm_tof3_port_smgc_pt_spd(
        devid, p_cur->p_pipe, p_cur->port, BF_SPEED_NONE, 0);
    if (g_tm_ctx[devid]->subdev_count > 1) {
      rc |= bf_tm_tof3_port_rmgc_pt_spd(
          devid, p_cur->p_pipe, p_cur->port, BF_SPEED_NONE, 0);
    }
  }

  if (g_tm_ctx[devid]->subdev_count > 1) {
    rc |= bf_tm_tof3_port_mgd_pt_spd(devid, p, 0);
  }

  bf_tm_status_t rc_ = BF_TM_EOK;
  bf_tm_eg_q_t *queue;
  for (q = 0; q < q_count; q++) {
    rc_ = bf_tm_q_guaranteed_min_limit_set(devid, port, q, 0);
    if (BF_TM_IS_NOTOK(rc_)) {
      LOG_ERROR(
          "reset gmin failed for dev:%d, pipe:%d, port:%d, queue:%d, rc=%d(%s)",
          devid,
          p->p_pipe,
          p->port,
          q,
          rc,
          bf_err_str(rc));
      rc = BF_INTERNAL_ERROR;
    }
    rc_ = bf_tm_q_get_descriptor(devid, port, q, &queue);
    if (BF_TM_IS_OK(rc_)) {
      rc_ = bf_tm_tof3_vq_hq_map(devid, p, queue, q, true);
    }
    if (BF_TM_IS_NOTOK(rc_)) {
      LOG_ERROR(
          "reset HW allocation failed for dev:%d, pipe:%d, port:%d, queue:%d, "
          "rc=%d(%s)",
          devid,
          p->p_pipe,
          p->port,
          q,
          rc,
          bf_err_str(rc));
      rc = BF_INTERNAL_ERROR;
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_set_qac_rx(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_tof3_set_port_rx(devid, p);
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

// create n-byte mask for q flush register
// q_count is the number of queues assigned to the port in question.
// len is the number of queues available per MAC
static bf_tm_status_t build_q_flush_bitmask(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint8_t q_count,
                                            uint8_t len,
                                            uint8_t *mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  for (q = 0; q < q_count; q++) {
    rc |= bf_tm_q_get_descriptor(dev, port, q, &queue);
    if (rc != BF_TM_EOK) {
      break;
    }
    int qid = queue->physical_q % BF_TM_TOF3_QUEUES_PER_PG;
    if (qid >= len) {
      return BF_INVALID_ARG;
    }
    mask[qid / 8] |= 0x1u << (qid % 8);
  }

  return rc;
}

static bf_tm_status_t flush_queues_for_port(bf_dev_id_t dev,
                                            bf_tm_port_t *p,
                                            uint8_t *mask) {
  bf_tm_status_t rc = BF_TM_EOK;

  bool old_batch_mode = g_tm_ctx[dev]->batch_mode;
  g_tm_ctx[dev]->batch_mode = false;

  rc = bf_tm_tof3_sch_set_port_flush(dev, p, mask, true);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: queue flush setup failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    g_tm_ctx[dev]->batch_mode = old_batch_mode;
    return (rc);
  }

  rc = bf_tm_tof3_sch_wait_port_flush(dev, p);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: queue flush wait failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    g_tm_ctx[dev]->batch_mode = old_batch_mode;
    return (rc);
  }

  uint8_t zeroes[BF_TM_TOF3_QUEUES_PER_PG / 8];
  memset(zeroes, 0x0, BF_TM_TOF3_QUEUES_PER_PG / 8);
  rc = bf_tm_tof3_sch_set_port_flush(dev, p, zeroes, false);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: queue flush disable failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    g_tm_ctx[dev]->batch_mode = old_batch_mode;
    return (rc);
  }

  g_tm_ctx[dev]->batch_mode = old_batch_mode;
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_set_all_queue_flush(bf_dev_id_t dev,
                                                   bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t q_count;
  uint8_t q_mapping[BF_TM_TOF3_QUEUES_PER_PG];
  bf_dev_port_t port;

  port = MAKE_DEV_PORT(p->l_pipe, p->uport);
  // port = lld_sku_map_devport_from_device_to_user(dev, port);

  // Get the queue count
  rc = bf_tm_port_q_mapping_get(dev, port, &q_count, q_mapping);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: getting queue count failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  uint8_t mask[BF_TM_TOF3_QUEUES_PER_PG / 8];
  memset(mask, 0x0, BF_TM_TOF3_QUEUES_PER_PG / 8);
  rc =
      build_q_flush_bitmask(dev, port, q_count, BF_TM_TOF3_QUEUES_PER_PG, mask);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: creating flush bitmask failed for dev %d, pipe %d port %d, rc "
        "%s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  rc = flush_queues_for_port(dev, p, mask);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR(
        "TM: port queue flush failed for dev %d, pipe %d port %d, rc %s "
        "(%d)",
        dev,
        p->p_pipe,
        p->port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_ingress_usage_cntr(bf_dev_id_t devid,
                                                        bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  uint64_t indir_addr;

  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_min_cnt(
      port->p_pipe, port->port);
  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);

  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_cnt(
      port->p_pipe, port->port);
  rc |= bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);

  indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_hdr_cnt(
      port->p_pipe, port->port);
  rc |= bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_wac_drop_state_ext(bf_dev_id_t devid,
                                                      bf_tm_port_t *port,
                                                      bool *shr_lmt_state,
                                                      bool *hdr_lmt_state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_st(
      port->p_pipe, port->port);
  rc = bf_tm_subdev_read_memory(devid, 0, indir_addr, &hi, &lo);
  *shr_lmt_state = lo & 0x1;
  *hdr_lmt_state = (lo >> 1) & 0x1;
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_wac_drop_state(bf_dev_id_t devid,
                                                    bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  uint64_t indir_addr = tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_st(
      port->p_pipe, port->port);
  rc = bf_tm_subdev_write_memory(devid, 0, indir_addr, 8, hi, lo);

  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_egress_usage_cntr(bf_dev_id_t devid,
                                                       bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0;
  uint64_t lo = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_cell_count(
            d_pipe, port->port);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
    if (rc != BF_TM_EOK) return BF_TM_EINT;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_qac_drop_state(bf_dev_id_t devid,
                                                  bf_tm_port_t *port,
                                                  bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.port_drop_state[port->port]),
        &val);
    if (rc != BF_TM_EOK) return BF_TM_EINT;
  }
  *state = val & 0x1;
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_qac_drop_state(bf_dev_id_t devid,
                                                    bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.port_drop_state[port->port]),
        val);
    if (rc != BF_TM_EOK) return BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_qac_drop_limit(bf_dev_id_t devid,
                                                    bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    uint64_t indir_addr =
        tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(d_pipe,
                                                                   port->port);
    rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
    if (rc != BF_TM_EOK) return BF_TM_EINT;
    lo = getp_tof3_qac_port_config_entry_port_thrd(&lo);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
    if (rc != BF_TM_EOK) return BF_TM_EINT;
  }
  return (rc);
}

static bf_tm_status_t bf_tm_tof3_port_get_pfc_state_reg(bf_dev_id_t devid,
                                                        bf_tm_port_t *port,
                                                        uint64_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pfc_state(port->p_pipe,
                                                               port->port);
  rc = bf_tm_subdev_read_memory(devid, 0, indir_addr, &hi, &lo);
  *state = lo;

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_pfc_state(bf_dev_id_t devid,
                                             bf_tm_port_t *port,
                                             uint8_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0;
  rc = bf_tm_tof3_port_get_pfc_state_reg(devid, port, &val);
  *state = getp_tof3_wac_pfc_state_pfc_state_port_ppg_state(&val);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_pfc_state_ext(bf_dev_id_t devid,
                                                 bf_tm_port_t *port,
                                                 uint8_t *port_ppg_state,
                                                 uint8_t *rm_pfc_state,
                                                 uint8_t *mac_pfc_out,
                                                 bool *mac_pause_out) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t val = 0;
  rc = bf_tm_tof3_port_get_pfc_state_reg(devid, port, &val);
  if (rc == BF_TM_EOK) {
    if (port_ppg_state) {
      *port_ppg_state = getp_tof3_wac_pfc_state_pfc_state_port_ppg_state(&val);
    }
    if (rm_pfc_state) {
      *rm_pfc_state = getp_tof3_wac_pfc_state_pfc_state_rm_pfc_state(&val);
    }
    if (mac_pfc_out) {
      *mac_pfc_out = getp_tof3_wac_pfc_state_pfc_state_mac_pfc_out(&val);
    }
    if (mac_pause_out) {
      *mac_pause_out =
          (bool)getp_tof3_wac_pfc_state_pfc_state_mac_pause_out(&val);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_clear_pfc_state(bf_dev_id_t devid,
                                               bf_tm_port_t *port) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t hi = 0, lo = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pfc_state(d_pipe,
                                                               port->port);
  rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to clear port_pfc_state for dev %d "
        "p_pipe %d and "
        "port %d",
        devid,
        port->p_pipe,
        port->port);
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_egress_drop_color_cntr(bf_dev_id_t devid,
                                                          bf_tm_port_t *port,
                                                          bf_tm_color_t color,
                                                          uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t lo = 0;
  uint64_t indir_addr;
  uint32_t color_section;

  switch (color) {
    case BF_TM_COLOR_GREEN:
      color_section = port->port;
      break;
    case BF_TM_COLOR_YELLOW:
      color_section = port->port + 36;
      break;
    case BF_TM_COLOR_RED:
      color_section = port->port + 72;
      break;
    default:
      return BF_TM_EINV_ARG;
  }

  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    uint64_t tmp_hi = 0;
    uint64_t tmp_lo = 0;
    d_pipe = BF_TM_2DIE_D_PIPE(port->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(port->p_pipe, d_inst);
    indir_addr = tof3_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(
        d_pipe, color_section);
    rc = bf_tm_subdev_read_memory(
        devid, subdev_id, indir_addr, &tmp_hi, &tmp_lo);

    if (rc != BF_TM_EOK) return BF_TM_EINT;
    lo += getp_tof3_qac_drop_count_port_entry_count(&tmp_lo);
  }
  *count = lo;
  return (rc);
}

bf_tm_status_t bf_tm_tof3_sch_get_port_egress_pfc_status(bf_dev_id_t devid,
                                                         bf_tm_port_t *port,
                                                         uint8_t *status) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  rc = bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[port->p_pipe]
                   .port_pfc_status_mem[port->port]),
      &val);

  *status = (uint8_t)(getp_tof3_sch_port_pfc_status_r_pfc_pri_pause(&val));
  return rc;
}

bf_tm_status_t bf_tm_tof3_sch_get_q_egress_pfc_status(bf_dev_id_t devid,
                                                      bf_tm_eg_q_t *q,
                                                      bool *status) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t lo = 0;
  uint64_t hi = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  uint64_t indir_addr =
      tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
  rc = bf_tm_subdev_read_memory(devid, subdev_id, indir_addr, &hi, &lo);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: Unable to get queue_egress_pfc_status for subdev %d dev %d p_pipe "
        "%d and "
        "physical_q %d",
        subdev_id,
        devid,
        q->p_pipe,
        q->physical_q);
    return rc;
  }
  *status = (bool)(lo & 0x1ul);

  return rc;
}

bf_tm_status_t bf_tm_tof3_sch_set_q_egress_pfc_status(bf_dev_id_t devid,
                                                      bf_tm_eg_q_t *q,
                                                      bool status) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t lo = (uint64_t)status;
  uint64_t hi = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    indir_addr = tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: Unable to set queue_egress_pfc_status for subdev %d dev %d "
          "p_pipe %d and "
          "physical_q %d",
          subdev_id,
          devid,
          q->p_pipe,
          q->physical_q);
      return rc;
    }
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_sch_clear_q_egress_pfc_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t lo = 0;
  uint64_t hi = 0;
  uint64_t indir_addr;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;

  for (uint32_t d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, d_inst);
    indir_addr = tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 8, hi, lo);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: Unable to clear queue_egress_pfc_status for subdev %d dev %d "
          "p_pipe %d and "
          "physical_q %d",
          subdev_id,
          devid,
          q->p_pipe,
          q->physical_q);
      return rc;
    }
  }
  return rc;
}

///////////////// PRE/MCAST FIFO ////////////////////////////

bf_tm_status_t bf_tm_tof3_set_mcast_fifo_arbmode(bf_dev_id_t devid,
                                                 bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, d_inst);

    rc |= bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe].arb_ctrl),
        fifo->arb_mode);
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_get_mcast_fifo_arbmode(bf_dev_id_t devid,
                                                 bf_tm_mcast_fifo_t *fifo) {
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, 0);

  return (bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[d_pipe].arb_ctrl),
      &(fifo->arb_mode)));
}

bf_tm_status_t bf_tm_tof3_set_mcast_fifo_wrr_weight(bf_dev_id_t devid,
                                                    bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe].wrr_ctrl),
        fifo->weight);
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_get_mcast_fifo_wrr_weight(bf_dev_id_t devid,
                                                    bf_tm_mcast_fifo_t *fifo) {
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, 0);

  return (bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[d_pipe].wrr_ctrl),
      &(fifo->weight)));
}

bf_tm_status_t bf_tm_tof3_set_mcast_fifo_icos_bmap(bf_dev_id_t devid,
                                                   bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  int icos;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, 0);

  // TODO : Provide Configurations for mapping
  // from iCOS[2:0] to 4 MC Replication FIFOs.
  for (icos = 0; icos < 8; icos++) {
    if ((1 << icos) & fifo->icos_bmap) {
      rc |= bf_tm_subdev_write_register(
          devid,
          subdev_id,
          offsetof(tof3_reg,
                   device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                       .wac_reg.wac_pre_fifo_mapping[icos]),
          fifo->fifo);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_get_mcast_fifo_icos_bmap(bf_dev_id_t devid,
                                                   bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  int icos;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, 0);

  fifo->icos_bmap = 0;
  for (icos = 0; icos < 8; icos++) {
    rc |= bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.wac_pre_fifo_mapping[icos]),
        &val);
    if (val == fifo->fifo) {
      fifo->icos_bmap |= (1 << icos);
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_set_mcast_fifo_depth(bf_dev_id_t devid,
                                               bf_tm_mcast_fifo_t *fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(
            tof3_reg,
            device_select.tm_top.tm_pre_top.pre[d_pipe].fifo_depth[fifo->fifo]),
        fifo->size);
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_get_mcast_fifo_depth(bf_dev_id_t devid,
                                               bf_tm_mcast_fifo_t *fifo) {
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(fifo->phy_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(fifo->phy_pipe, 0);

  bf_tm_status_t rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_pre_top.pre[d_pipe].fifo_depth),
      &(fifo->size));
  return (rc);
}

bf_tm_status_t bf_tm_tof3_port_get_pre_mask(bf_dev_id_t devid,
                                            uint32_t *mask_array,
                                            uint32_t size) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (size < 9 * 2) {
    return BF_TM_EINT;
  }

  for (uint32_t i = 0; i < 2; ++i) {
    uint32_t offset = 9 * i;
    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_0_9),
        &val);
    mask_array[0 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_1_9),
        &val);
    mask_array[1 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_2_9),
        &val);
    mask_array[2 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_3_9),
        &val);
    mask_array[3 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_4_9),
        &val);
    mask_array[4 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_5_9),
        &val);
    mask_array[5 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_6_9),
        &val);
    mask_array[6 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_7_9),
        &val);
    mask_array[7 + offset] = val;
    val = 0;

    rc |= bf_tm_subdev_read_register(
        devid,
        0,
        offsetof(tof3_reg,
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

bf_tm_status_t bf_tm_tof3_port_clear_pre_mask(bf_dev_id_t devid) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  for (uint32_t i = 0; i < 2; ++i) {
    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_0_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_1_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_2_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_3_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_4_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_5_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_6_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_7_9),
        val);

    rc |= bf_tm_subdev_write_register(
        devid,
        0,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre_common.port_mask[i]
                     .port_mask_8_9),
        val);
  }
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_get_pre_down_mask(bf_dev_id_t devid,
                                                 uint32_t *mask_array,
                                                 uint32_t size) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  if (size < 9) {
    return BF_TM_EINT;
  }

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_0_9),
      &val);
  mask_array[0] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_1_9),
      &val);
  mask_array[1] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_2_9),
      &val);
  mask_array[2] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_3_9),
      &val);
  mask_array[3] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_4_9),
      &val);
  mask_array[4] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_5_9),
      &val);
  mask_array[5] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_6_9),
      &val);
  mask_array[6] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_7_9),
      &val);
  mask_array[7] = val;
  val = 0;

  rc |= bf_tm_subdev_read_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_8_9),
      &val);
  mask_array[8] = val;
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_tof3_port_clear_pre_down_mask(bf_dev_id_t devid) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_0_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_1_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_2_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_3_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_4_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_5_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_6_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_7_9),
      val);

  rc |= bf_tm_subdev_write_register(
      devid,
      0,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.port_down.port_down_8_9),
      val);
  if (rc != BF_TM_EOK) {
    rc = BF_TM_EINT;
  }
  return rc;
}

///////////////TM PM DROP ERROR DISCARD COUNTERS////////////////////

// bf_tm_blklvl_cntrs_t *s
bf_tm_status_t bf_tm_tof3_blklvl_get_drop_cntrs(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, bf_tm_blklvl_cntrs_t *blk_cntrs) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  if (blk_cntrs) {
    TRAFFIC_MGR_MEMSET(blk_cntrs, 0, sizeof(bf_tm_blklvl_cntrs_t));

    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.ctr_drop_no_dst),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->wac_no_dest_drop = (uint64_t)val;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->wac_buf_full_drop = cnt;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->egress_pipe_total_drop = cnt;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->psc_pkt_drop = cnt;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .pex_dis_cnt.pex_dis_cnt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .pex_dis_cnt.pex_dis_cnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->pex_total_disc = cnt;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .qac_dis_cnt.qac_dis_cnt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .qac_dis_cnt.qac_dis_cnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->qac_total_disc = cnt;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .tot_dis_dq_cnt.tot_dis_dq_cnt_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .tot_dis_dq_cnt.tot_dis_dq_cnt_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->total_disc_dq = cnt;

    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_ctr32_drop_no_dst),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->qac_no_dest_drop = (uint64_t)val;

    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_ctr32_pre_mc_drop),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    blk_cntrs->qac_pre_mc_drop = (uint64_t)val;

    cnt = 0;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe]
                     .packet_drop.packet_drop_0_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    rc = bf_tm_subdev_read_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe]
                     .packet_drop.packet_drop_1_2),
        &val);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }

    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    blk_cntrs->pre_total_drop = cnt;
  }
  return (rc);
}

// bf_tm_fifo_cntrs_t
bf_tm_status_t bf_tm_tof3_pre_fifo_get_drop_cntrs(
    bf_dev_id_t devid, bf_tm_pre_fifo_cntrs_t *fifo_cntrs) {
  bf_tm_status_t rc = BF_TM_EOK;

  uint32_t val = 0;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (fifo_cntrs) {
    TRAFFIC_MGR_MEMSET(fifo_cntrs, 0, sizeof(bf_tm_pre_fifo_cntrs_t));

    for (int pipe = 0; pipe < g_tm_ctx[devid]->tm_cfg.pipe_cnt; pipe++) {
      for (int fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
        bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
        bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

        val = 0;
        cnt = 0;
        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.qac_ctr48_drop_pre_fifo[fifo]
                         .qac_ctr48_drop_pre_fifo_0_2),
            &val);
        reg_cnt = val;
        cnt = (reg_cnt << 0) + cnt;

        rc = bf_tm_subdev_read_register(
            devid,
            subdev_id,
            offsetof(tof3_reg,
                     device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                         .qac_reg.qac_ctr48_drop_pre_fifo[fifo]
                         .qac_ctr48_drop_pre_fifo_1_2),
            &val);
        if (BF_TM_IS_NOTOK(rc)) {
          return rc;
        }

        reg_cnt = val;
        cnt = (reg_cnt << 32) + cnt;

        switch (pipe) {
          case 0:
            fifo_cntrs->wac_drop_cnt_pre0_fifo[fifo] = cnt;
            break;
          case 1:
            fifo_cntrs->wac_drop_cnt_pre1_fifo[fifo] = cnt;
            break;
          case 2:
            fifo_cntrs->wac_drop_cnt_pre2_fifo[fifo] = cnt;
            break;
          case 3:
            fifo_cntrs->wac_drop_cnt_pre3_fifo[fifo] = cnt;
            break;
          case 4:
            fifo_cntrs->wac_drop_cnt_pre4_fifo[fifo] = cnt;
            break;
          case 5:
            fifo_cntrs->wac_drop_cnt_pre5_fifo[fifo] = cnt;
            break;
          case 6:
            fifo_cntrs->wac_drop_cnt_pre6_fifo[fifo] = cnt;
            break;
          case 7:
            fifo_cntrs->wac_drop_cnt_pre7_fifo[fifo] = cnt;
            break;
          default:
            LOG_ERROR("TM: invalid pipe id %d for device %d", d_pipe, devid);
            return BF_INVALID_ARG;
            ;
        }
      }
    }
  }

  return (rc);
}

/*
 *      Clear Registers
 */
bf_tm_status_t bf_tm_tof3_blklvl_clr_drop_cntrs(bf_dev_id_t devid,
                                                bf_dev_pipe_t pipe,
                                                uint32_t clear_mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  if (clear_mask & WAC_NO_DEST_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.ctr_drop_no_dst),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & WAC_BUF_FULL_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[d_pipe]
                     .wac_reg.wac_drop_buf_full.ctr_ctrl_pkt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & EGRESS_PIPE_TOTAL_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_caa_top.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PSC_PKT_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc_common.epipe[d_pipe]
                     .pkt_dropcnt.pkt_dropcnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PEX_TOTAL_DISC) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .pex_dis_cnt.pex_dis_cnt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .pex_dis_cnt.pex_dis_cnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_TOTAL_DISC) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .qac_dis_cnt.qac_dis_cnt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .qac_dis_cnt.qac_dis_cnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & TOTAL_DISC_DQ) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .tot_dis_dq_cnt.tot_dis_dq_cnt_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[d_pipe]
                     .tot_dis_dq_cnt.tot_dis_dq_cnt_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_NO_DEST_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_ctr32_drop_no_dst),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & QAC_PRE_MC_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.qac_ctr32_pre_mc_drop),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  if (clear_mask & PRE_TOTAL_DROP) {
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe]
                     .packet_drop.packet_drop_0_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[d_pipe]
                     .packet_drop.packet_drop_1_2),
        0);
    if (BF_TM_IS_NOTOK(rc)) {
      return rc;
    }
  }

  return (rc);
}

// bf_tm_fifo_cntrs_t
bf_tm_status_t bf_tm_tof3_pre_fifo_clr_drop_cntrs(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t fifo) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe, 0);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.qac_ctr48_drop_pre_fifo[fifo]
                   .qac_ctr48_drop_pre_fifo_0_2),
      0);
  if (BF_TM_IS_NOTOK(rc)) {
    return rc;
  }

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                   .qac_reg.qac_ctr48_drop_pre_fifo[fifo]
                   .qac_ctr48_drop_pre_fifo_1_2),
      0);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_sch_set_q_adv_fc_mode(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  bf_tm_tof3_sch_prep_q_cfg_reg(&val, q);
  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      val);
  if (q->q_sch_cfg.sch_enabled) {
    // queue is scheduling enabled. After enabling queue, it
    // is required to clear q_pfc_status. The reason being
    // when queue is disabled from sch, TM logic self asserts
    // pfc on the queue.
    uint64_t indir_addr =
        tof3_mem_tm_tm_sch_sch_q_pfc_status_mem(d_pipe, q->physical_q);
    rc = bf_tm_subdev_write_memory(devid, subdev_id, indir_addr, 4, 0, 0);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_sch_get_q_adv_fc_mode(bf_dev_id_t devid,
                                                bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(q->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(q->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_sch_top.sch[d_pipe]
                   .queue_config[q->physical_q]),
      &val);
  bf_tm_tof3_sch_fill_q_struct(&val, q);

  return (rc);
}

bf_tm_status_t bf_tm_tof3_sch_set_adv_fc_mode_enable(bf_dev_id_t devid,
                                                     bf_tm_eg_pipe_t *pipe,
                                                     bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t addr;

  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, 0);

  if (pipe->p_pipe < 8) {
    addr = offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[d_pipe].ctrl);
  } else {
    LOG_ERROR("TM: invalid pipe id %d for device %d", pipe->p_pipe, devid);
    return BF_INVALID_ARG;
  }
  rc = bf_tm_subdev_read_register(devid, subdev_id, addr, &val);

  setp_tof3_sch_ctrl_r_adv_fc_mode_en(&val, enable);

  rc |= bf_tm_subdev_write_register(devid, subdev_id, addr, val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_sch_get_adv_fc_mode_enable(bf_dev_id_t devid,
                                                     bf_tm_eg_pipe_t *pipe,
                                                     bool *enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t addr;
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, 0);

  if (pipe->p_pipe < 8) {
    addr = offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[d_pipe].ctrl);
  } else {
    LOG_ERROR("TM: invalid pipe id %d for device %d", pipe->p_pipe, devid);
    return BF_INVALID_ARG;
  }

  rc = bf_tm_subdev_read_register(devid, subdev_id, addr, &val);

  val = getp_tof3_sch_ctrl_r_adv_fc_mode_en(&val);

  if (enable) {
    *enable = !!val;
  }
  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_set_deflection_port_enable(bf_dev_id_t devid,
                                                          bf_tm_eg_pipe_t *pipe,
                                                          bool enable) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t *val = &(pipe->qac_pipe_config);
  bf_dev_pipe_t d_pipe;
  bf_subdev_id_t subdev_id;
  uint32_t d_inst;

  setp_tof3_qac_pipe_config_defd_en(val, enable);
  for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
    d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, d_inst);
    subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, d_inst);

    rc = bf_tm_subdev_write_register(
        devid,
        subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[d_pipe]
                     .qac_reg.pipe_config),
        *val);
  }

  return (rc);
}

bf_tm_status_t bf_tm_tof3_pipe_get_deflection_port_enable(bf_dev_id_t devid,
                                                          bf_tm_eg_pipe_t *pipe,
                                                          bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t *val = &(pipe->qac_pipe_config);
  bf_dev_pipe_t d_pipe = BF_TM_2DIE_D_PIPE(pipe->p_pipe, 0);
  bf_subdev_id_t subdev_id = BF_TM_2DIE_SUBDEV_ID(pipe->p_pipe, 0);

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(
          tof3_reg,
          device_select.tm_top.tm_qac_top.qac_pipe[d_pipe].qac_reg.pipe_config),
      val);

  *enable = getp_tof3_qac_pipe_config_defd_en(val);

  return rc;
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_glb_cell_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  bf_subdev_id_t subdev_id = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      &val);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  if (getp_tof3_wac_glb_cell_limit_limit(&val) == ig_gpool->glb_cell_limit) {
    return rc;
  }

  setp_tof3_wac_glb_cell_limit_limit(&val, ig_gpool->glb_cell_limit);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_glb_cell_limit(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_subdev_id_t subdev_id = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      &val);
  ig_gpool->glb_cell_limit = getp_tof3_wac_glb_cell_limit_limit(&val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_set_glb_cell_limit_state(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val = 0;
  uint32_t state = (ig_gpool->glb_cell_limit_enable) ? 1 : 0;
  bf_subdev_id_t subdev_id = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      &val);
  if (rc != BF_TM_EOK) {
    return rc;
  }

  if (getp_tof3_wac_glb_cell_limit_en(&val) == state) {
    return rc;
  }

  setp_tof3_wac_glb_cell_limit_en(&val, state);

  rc = bf_tm_subdev_write_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      val);
  return (rc);
}

bf_tm_status_t bf_tm_tof3_ig_gpool_get_glb_cell_limit_state(
    bf_dev_id_t devid, bf_tm_ig_gpool_t *ig_gpool) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t val;
  bf_subdev_id_t subdev_id = 0;

  rc = bf_tm_subdev_read_register(
      devid,
      subdev_id,
      offsetof(tof3_reg,
               device_select.tm_top.tm_wac_top.wac_common.wac_common
                   .wac_glb_cell_limit),
      &val);
  ig_gpool->glb_cell_limit_enable =
      getp_tof3_wac_glb_cell_limit_en(&val) ? true : false;
  return (rc);
}

///////////////////////// DDR ///////////////////////////////////

bf_tm_status_t bf_tm_tof3_ddr_training(bf_dev_id_t devid) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t d_inst;

  // Do not do DDR train for single die chips
  if (g_tm_ctx[devid]->subdev_count > 1) {
    for (d_inst = 0; d_inst < g_tm_ctx[devid]->subdev_count; d_inst++) {
      rc = bf_tm_tof3_ddr_init_seq(devid, d_inst);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "DDR train failed for Device %d, Sub device %d", devid, d_inst);
        return rc;
      }
    }
  }

  return (rc);
}

///////////////// QOC ////////////////////////////
