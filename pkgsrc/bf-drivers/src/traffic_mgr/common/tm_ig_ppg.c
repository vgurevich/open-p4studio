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
// to maange PPGs in Ingress TM

#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

static bf_tm_ppg_thres_hw_funcs_tbl g_ppg_thres_hw_fptr_tbl;
static bf_tm_ppg_cfg_hw_funcs_tbl g_ppg_cfg_hw_fptr_tbl;

bf_tm_status_t bf_tm_alloc_ppg(bf_dev_id_t dev,
                               bf_dev_port_t port,
                               bf_tm_ppg_hdl *ppg) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  bf_status_t rc = BF_SUCCESS;
  bf_dev_pipe_t pipe = DEV_PORT_TO_PIPE(port);
  int i;
  bf_tm_port_t *_p = NULL;
  bf_tm_ppg_t *_ppg = NULL;
  uint32_t count;
  bool free_ppg = false;
  bool defppg;
  int ppg_n, lpipe, pport;
  bf_dev_port_t lport;

  if (TM_HITLESS_WARM_INIT_IN_PROGRESS(dev)) {
    BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(*ppg, g_tm_ctx[dev]));
    bf_tm_api_hlp_get_ppg_details(dev, *ppg, &defppg, &ppg_n, &lpipe, &pport);
    if (defppg) {
      /* PPG allocate API shouldn't be called for default PPG */
      return (BF_INVALID_ARG);
    }
    _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], lpipe, ppg_n);
    BF_TM_PPG_CHECK(_ppg, ppg_n, lpipe, dev);

    lport = MAKE_DEV_PORT(lpipe, pport);
    if (lport != port) {
      LOG_ERROR(
          "HA: PPG handle 0x%x port %d not matching "
          "with passed in port %d",
          *ppg,
          lport,
          port);
      return (BF_INVALID_ARG);
    }
    _p = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
    /* Check if already maximum number of PPGs are allocated for the port */
    if (_p->ppg_count >= BF_TM_MAX_PFC_LEVELS) {
      return (BF_INVALID_ARG);
    }

    rc = bf_tm_ppg_set_cache_counters(dev, *ppg);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("HA: PPG handle 0x%x port %d Unable to allocate cache counters",
                *ppg,
                port);
      return (BF_NO_SYS_RESOURCES);
    }

    _ppg->in_use = true;
    _ppg->uport = DEV_PORT_TO_LOCAL_PORT(port);
    _ppg->port = DEV_PORT_TO_LOCAL_PORT(
        lld_sku_map_devport_from_user_to_device(dev, port));

    _p->ppg_list[_p->ppg_count] = _ppg;
    _p->ppg_count += 1;
    return (rc);
  }

  _p = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
  if (!_p) {
    LOG_ERROR("Invalid port %d passed in for device %d", port, dev);
    return (BF_INVALID_ARG);
  }
  /* Check if already maximum number of PPGs are allocated for the port */
  if (_p->ppg_count >= BF_TM_MAX_PFC_LEVELS) {
    return (BF_NO_SYS_RESOURCES);
  }

  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, 0);
  BF_TM_PPG_CHECK(_ppg, 0, pipe, dev);

  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe; i++, _ppg++) {
    // Walk over all PPGs of the pipe and find unused PPG.
    if (BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type) ||
        BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
      // Skip invalid PPGs per tof2 allocation restriction
      // MAC1-4 only uses PPG 0-63, MAC5-9 only uses 64-127. MAC0 is recirc.
      if ((i < 64 && _p->pg > 4) || (i >= 64 && _p->pg <= 4)) {
        continue;
      }
    }
    if (!_ppg->in_use) {
      /* Check if WAC usage for this PPG is zero. Until ppg cell usage drops
       * to zero, the ppg cannot be allocated; By allocating PPG that has
       * outstanding traffic, it will penalize/affect traffic flow that gets
       * mapped to the allocated PPG.
       */
      if (g_tm_ctx[dev]->target == BF_TM_TARGET_ASIC) {
        count = 0;
        free_ppg = true;
        bf_tm_ppg_get_gmin_usage_counter(dev, _ppg, &count);
        if (count) {
          continue;
        }
        bf_tm_ppg_get_shared_usage_counter(dev, _ppg, &count);
        if (count) {
          continue;
        }
        bf_tm_ppg_get_skid_usage_counter(dev, _ppg, &count);
        if (count) {
          continue;
        }
      }

      *ppg = _ppg->ppg;
      *ppg |= (pipe << 12);
      *ppg |= (DEV_PORT_TO_LOCAL_PORT(port) << 16);

      rc = bf_tm_ppg_set_cache_counters(dev, *ppg);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "HA: PPG handle 0x%x port %d Unable to allocate cache counters",
            _ppg->ppg,
            port);
        return (BF_NO_SYS_RESOURCES);
      }

      _ppg->in_use = true;
      _ppg->uport = DEV_PORT_TO_LOCAL_PORT(port);
      _ppg->port = DEV_PORT_TO_LOCAL_PORT(
          lld_sku_map_devport_from_user_to_device(dev, port));

      _p->ppg_list[_p->ppg_count] = _ppg;
      _p->ppg_count += 1;

      return (rc);
    }
  }
  if (free_ppg) {
    /* There are free PPGs but none of them have zero outstanding cell usage.
     * Otherwords, unused PPGs still have transit traffic waiting to be drained.
     * return EAGAIN so that application can call the API to allocate PPG.
     */
    return (BF_EAGAIN);
  }
  return (BF_NO_SYS_RESOURCES);
}

bf_tm_status_t bf_tm_ppg_set_min_limit(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(
          limit, ppg->thresholds.min_limit, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->thresholds.min_limit = limit;

  if (g_ppg_thres_hw_fptr_tbl.min_limit_wr_fptr) {
    rc = g_ppg_thres_hw_fptr_tbl.min_limit_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_min_limit(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       bf_tm_thres_t *sw_limit,
                                       bf_tm_thres_t *hw_limit) {
  bf_tm_ppg_t out_ppg;
  bf_tm_status_t rc = BF_TM_EOK;
  *sw_limit = ppg->thresholds.min_limit;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_ppg_thres_hw_fptr_tbl.min_limit_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ppg_thres_hw_fptr_tbl.min_limit_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_limit = out_ppg.thresholds.min_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_skid_limit(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, ppg->thresholds.skid_limit, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->thresholds.skid_limit = limit;
  if (g_ppg_thres_hw_fptr_tbl.skid_limit_wr_fptr) {
    rc = g_ppg_thres_hw_fptr_tbl.skid_limit_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_skid_limit(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        bf_tm_thres_t *sw_limit,
                                        bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;
  *sw_limit = ppg->thresholds.skid_limit;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_ppg_thres_hw_fptr_tbl.skid_limit_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ppg_thres_hw_fptr_tbl.skid_limit_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_limit = out_ppg.thresholds.skid_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_app_limit(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, ppg->thresholds.app_limit, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->thresholds.app_limit = limit;
  if (g_ppg_thres_hw_fptr_tbl.app_limit_wr_fptr) {
    rc = g_ppg_thres_hw_fptr_tbl.app_limit_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_app_limit(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       bf_tm_thres_t *sw_limit,
                                       bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;
  *sw_limit = ppg->thresholds.app_limit;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_ppg_thres_hw_fptr_tbl.app_limit_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ppg_thres_hw_fptr_tbl.app_limit_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_limit = out_ppg.thresholds.app_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_ppg_hyst(bf_dev_id_t dev_id,
                                      bf_tm_ppg_t *ppg,
                                      bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, ppg->thresholds.ppg_hyst, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->thresholds.ppg_hyst = limit;

  if (g_ppg_thres_hw_fptr_tbl.hyst_wr_fptr) {
    rc = g_ppg_thres_hw_fptr_tbl.hyst_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_ppg_hyst(bf_dev_id_t dev_id,
                                      bf_tm_ppg_t *ppg,
                                      bf_tm_thres_t *sw_limit,
                                      bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;
  *sw_limit = ppg->thresholds.ppg_hyst;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_ppg_thres_hw_fptr_tbl.hyst_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_ppg_thres_hw_fptr_tbl.hyst_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_limit = out_ppg.thresholds.ppg_hyst;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_ppg_hyst_index(bf_dev_id_t dev_id,
                                            bf_tm_ppg_t *ppg,
                                            uint8_t *sw_hyst_index,
                                            uint8_t *hw_hyst_index) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;
  *sw_hyst_index = ppg->thresholds.hyst_index;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_hyst_index &&
      g_ppg_thres_hw_fptr_tbl.hyst_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    *hw_hyst_index = 0xff;
    rc = g_ppg_thres_hw_fptr_tbl.hyst_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_hyst_index = out_ppg.thresholds.hyst_index;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_app_poolid(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        uint32_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          pool, ppg->ppg_cfg.app_poolid, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->ppg_cfg.app_poolid = pool;

  if (g_ppg_cfg_hw_fptr_tbl.app_poolid_wr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.app_poolid_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_app_poolid(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        uint32_t *sw_app_poolid,
                                        uint32_t *hw_app_poolid) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;

  *sw_app_poolid = ppg->ppg_cfg.app_poolid;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_app_poolid &&
      g_ppg_cfg_hw_fptr_tbl.app_poolid_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    // Pool Id might be read from HW only if at least one iCoS
    // is assigned to the PPG.
    if (ppg->ppg_cfg.icos_mask) {
      rc = g_ppg_cfg_hw_fptr_tbl.app_poolid_rd_fptr(dev_id, (void *)&out_ppg);
    }
    *hw_app_poolid = out_ppg.ppg_cfg.app_poolid;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_is_dynamic(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        bool mode) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          mode, ppg->ppg_cfg.is_dynamic, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->ppg_cfg.is_dynamic = mode;

  if (g_ppg_cfg_hw_fptr_tbl.is_dynamic_wr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.is_dynamic_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_is_dynamic(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        bool *sw_ppg_mode,
                                        bool *hw_ppg_mode) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;

  *sw_ppg_mode = ppg->ppg_cfg.is_dynamic;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_ppg_mode &&
      g_ppg_cfg_hw_fptr_tbl.is_dynamic_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    rc = g_ppg_cfg_hw_fptr_tbl.is_dynamic_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_ppg_mode = out_ppg.ppg_cfg.is_dynamic;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_baf(bf_dev_id_t dev_id,
                                 bf_tm_ppg_t *ppg,
                                 uint8_t baf) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(baf, ppg->ppg_cfg.baf, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->ppg_cfg.baf = baf;
  if (g_ppg_cfg_hw_fptr_tbl.baf_wr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.baf_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_baf(bf_dev_id_t dev_id,
                                 bf_tm_ppg_t *ppg,
                                 uint8_t *sw_baf,
                                 uint8_t *hw_baf) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;

  *sw_baf = ppg->ppg_cfg.baf;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_baf &&
      g_ppg_cfg_hw_fptr_tbl.baf_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    rc = g_ppg_cfg_hw_fptr_tbl.baf_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_baf = out_ppg.ppg_cfg.baf;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_icos_mask(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       uint16_t icos_mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint8_t icos_mask_for_default_ppg = 0, icos_bmap = 0;

  if (TM_HITLESS_IS_CFG_MATCH(
          icos_mask, ppg->ppg_cfg.icos_mask, g_tm_ctx[dev_id])) {
    return (rc);
  }

  // Check if any icos in icos_mask is already mapped to any other PPG.
  bf_tm_port_t *p =
      BF_TM_PORT_PTR(g_tm_ctx[dev_id], MAKE_DEV_PORT(ppg->l_pipe, ppg->uport));
  for (int i = 0; i < g_tm_ctx[dev_id]->tm_cfg.icos_count; i++) {
    if (p->ppgs[i] && p->ppgs[i] != ppg) {
      if (p->ppgs[i]->is_default_ppg) {
        continue;
      }
      icos_bmap |= (p->ppgs[i])->ppg_cfg.icos_mask;
    }
  }

  if (icos_bmap & icos_mask) {
    // There is icos that is alredy mapped to some other ppg.
    return (BF_INVALID_ARG);
  }

  // if new icos_bmap for the ppg is subset of current icos_bmap
  // move the remaining icos_bmap to default-ppg
  // Compute icos_bmap that non default PPG is dropping.
  for (int i = 0; i < g_tm_ctx[dev_id]->tm_cfg.icos_count; i++) {
    if (ppg->ppg_cfg.icos_mask & ~(icos_mask) & (1 << i)) {
      icos_mask_for_default_ppg |= (1 << i);
    }
  }

  ppg->ppg_cfg.icos_mask = icos_mask;
  // Also set icos_mask inside port structure. (A PPG can map to only one port)
  bf_tm_port_set_icos_ppg_mapping(
      dev_id, MAKE_DEV_PORT(ppg->l_pipe, ppg->uport), icos_mask, ppg);

  bf_tm_ppg_t *default_ppg =
      BF_TM_PPG_PTR(g_tm_ctx[dev_id],
                    ppg->l_pipe,
                    (g_tm_ctx[dev_id]->tm_cfg.pfc_ppg_per_pipe + ppg->port));
  BF_TM_PPG_CHECK(default_ppg,
                  (g_tm_ctx[dev_id]->tm_cfg.pfc_ppg_per_pipe + ppg->port),
                  ppg->l_pipe,
                  dev_id);

  // icos that got mapped to PPG should be removed from default PPG.
  default_ppg->ppg_cfg.icos_mask &= ~(ppg->ppg_cfg.icos_mask);
  // Or in any icos that got dropped by PPG into default PPG.
  default_ppg->ppg_cfg.icos_mask |= icos_mask_for_default_ppg;
  bf_tm_port_set_icos_ppg_mapping(dev_id,
                                  MAKE_DEV_PORT(ppg->l_pipe, ppg->uport),
                                  default_ppg->ppg_cfg.icos_mask,
                                  default_ppg);

  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_icos_mask(bf_dev_id_t dev_id,
                                       bf_tm_ppg_t *ppg,
                                       uint16_t *sw_icos_mask,
                                       uint16_t *hw_icos_mask) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;

  *sw_icos_mask = ppg->ppg_cfg.icos_mask;
  if (hw_icos_mask && g_ppg_cfg_hw_fptr_tbl.icos_mask_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    rc = g_ppg_cfg_hw_fptr_tbl.icos_mask_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_icos_mask = out_ppg.ppg_cfg.icos_mask;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_set_pfc_treatment(bf_dev_id_t dev_id,
                                           bf_tm_ppg_t *ppg,
                                           bool lossless) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          lossless, ppg->ppg_cfg.is_pfc, g_tm_ctx[dev_id])) {
    return (rc);
  }
  ppg->ppg_cfg.is_pfc = lossless;
  if (g_ppg_cfg_hw_fptr_tbl.pfc_wr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.pfc_wr_fptr(dev_id, (void *)ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_pfc_treatment(bf_dev_id_t dev_id,
                                           bf_tm_ppg_t *ppg,
                                           bool *sw_lossless,
                                           bool *hw_lossless) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;

  *sw_lossless = ppg->ppg_cfg.is_pfc;
  if (hw_lossless && g_ppg_cfg_hw_fptr_tbl.pfc_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    rc = g_ppg_cfg_hw_fptr_tbl.pfc_rd_fptr(dev_id, (void *)&out_ppg);
    *hw_lossless = out_ppg.ppg_cfg.is_pfc;
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_drop_counter(bf_dev_id_t dev_id,
                                          bf_tm_ppg_t *ppg,
                                          uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_rd_fptr(dev_id, ppg, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_drop_state(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        bool *sw_state,
                                        bool *hw_state) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_state &&
      g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_rd_fptr(dev_id, ppg, hw_state);
    if (rc) {
      *sw_state = *hw_state;
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_drop_state(bf_dev_id_t dev_id,
                                          bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_drop_counter(bf_dev_id_t dev_id,
                                            bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_gmin_usage_counter(bf_dev_id_t dev_id,
                                                bf_tm_ppg_t *ppg,
                                                uint32_t *count) {
  uint64_t cnt;
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_rd_fptr(dev_id, ppg, &cnt);
    *count = (uint32_t)cnt;  // 32b counter.. upper bits do not matter
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_gmin_usage_counter(bf_dev_id_t dev_id,
                                                  bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_shared_usage_counter(bf_dev_id_t dev_id,
                                                  bf_tm_ppg_t *ppg,
                                                  uint32_t *count) {
  uint64_t cnt;
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_rd_fptr(dev_id, ppg, &cnt);
    *count = (uint32_t)cnt;  // 32b counter.. upper bits do not matter
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_shared_usage_counter(bf_dev_id_t dev_id,
                                                    bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_skid_usage_counter(bf_dev_id_t dev_id,
                                                bf_tm_ppg_t *ppg,
                                                uint32_t *count) {
  uint64_t cnt;
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_rd_fptr(dev_id, ppg, &cnt);
    *count = (uint32_t)cnt;  // 32b counter.. upper bits do not matter
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_skid_usage_counter(bf_dev_id_t dev_id,
                                                  bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_wm_counter(bf_dev_id_t dev_id,
                                        bf_tm_ppg_t *ppg,
                                        uint32_t *count) {
  uint64_t cnt;
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_wm_cntr_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_wm_cntr_rd_fptr(dev_id, ppg, &cnt);
    *count = (uint32_t)cnt;  // 32b counter.. upper bits do not matter
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_watermark(bf_dev_id_t dev_id, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.ppg_wm_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_wm_clr_fptr(dev_id, ppg);
  }

  return (rc);
}

bf_tm_status_t bf_tm_wac_pipe_get_buffer_full_drop_counter(bf_dev_id_t dev_id,
                                                           bf_dev_pipe_t pipe,
                                                           uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_rd_fptr) {
    rc =
        g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_rd_fptr(dev_id, pipe, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_wac_pipe_clear_buffer_full_drop_counter(
    bf_dev_id_t dev_id, bf_dev_pipe_t pipe) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_clr_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_clr_fptr(dev_id, pipe);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_allocation(bf_dev_id_t dev_id, bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ppg_cfg_hw_fptr_tbl.ppg_allocation_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.ppg_allocation_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_restore_wac_offset_profile(bf_dev_id_t dev_id) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ppg_cfg_hw_fptr_tbl.restore_wac_offset_profile_fptr) {
    g_ppg_cfg_hw_fptr_tbl.restore_wac_offset_profile_fptr(dev_id);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_fast_recovery_mode(bf_dev_id_t dev_id,
                                                bf_tm_ppg_t *ppg,
                                                bool *sw_mode,
                                                bool *hw_mode) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t out_ppg;
  *sw_mode = ppg->ppg_cfg.fast_recover_mode;
  if (g_ppg_cfg_hw_fptr_tbl.fast_recovery_rd_fptr) {
    memcpy(&out_ppg, ppg, sizeof(out_ppg));
    g_ppg_cfg_hw_fptr_tbl.fast_recovery_rd_fptr(dev_id, &out_ppg);
    *hw_mode = out_ppg.ppg_cfg.fast_recover_mode;
  }

  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_resume_limit(bf_dev_id_t dev_id,
                                          bf_tm_ppg_t *ppg,
                                          uint32_t *sw_limit,
                                          uint32_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_ppg_cfg_hw_fptr_tbl.resume_lmt_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.resume_lmt_rd_fptr(dev_id, ppg, hw_limit);
    if (rc) {
      *sw_limit = *hw_limit;
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_ppg_clear_resume_limit(bf_dev_id_t dev_id,
                                            bf_tm_ppg_t *ppg) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ppg_cfg_hw_fptr_tbl.resume_lmt_clr_fptr) {
    g_ppg_cfg_hw_fptr_tbl.resume_lmt_clr_fptr(dev_id, ppg);
  }
  return (rc);
}

bf_tm_status_t bf_tm_ppg_get_defaults(bf_dev_id_t dev_id,
                                      bf_tm_ppg_t *ppg,
                                      bf_tm_ppg_defaults_t *def) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (g_ppg_cfg_hw_fptr_tbl.defaults_rd_fptr) {
    rc = g_ppg_cfg_hw_fptr_tbl.defaults_rd_fptr(dev_id, ppg, def);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

#define BF_TM_PPG_HW_FPTR_TBL_SET_LIMIT_FUNCS(                            \
    ppg_set_ml_fptr, ppg_set_sl_fptr, ppg_set_al_fptr, ppg_set_hyst_fptr) \
  {                                                                       \
    g_ppg_thres_hw_fptr_tbl.min_limit_wr_fptr = ppg_set_ml_fptr;          \
    g_ppg_thres_hw_fptr_tbl.skid_limit_wr_fptr = ppg_set_sl_fptr;         \
    g_ppg_thres_hw_fptr_tbl.app_limit_wr_fptr = ppg_set_al_fptr;          \
    g_ppg_thres_hw_fptr_tbl.hyst_wr_fptr = ppg_set_hyst_fptr;             \
  }

#define BF_TM_PPG_HW_FPTR_TBL_GET_LIMIT_FUNCS(                            \
    ppg_get_ml_fptr, ppg_get_sl_fptr, ppg_get_al_fptr, ppg_get_hyst_fptr) \
  {                                                                       \
    g_ppg_thres_hw_fptr_tbl.min_limit_rd_fptr = ppg_get_ml_fptr;          \
    g_ppg_thres_hw_fptr_tbl.skid_limit_rd_fptr = ppg_get_sl_fptr;         \
    g_ppg_thres_hw_fptr_tbl.app_limit_rd_fptr = ppg_get_al_fptr;          \
    g_ppg_thres_hw_fptr_tbl.hyst_rd_fptr = ppg_get_hyst_fptr;             \
  }

#define BF_TM_PPG_HW_FPTR_TBL_SET_FUNCS(poolid_fptr,                         \
                                        is_dyn_fptr,                         \
                                        baf_fptr,                            \
                                        pfc_fptr,                            \
                                        drop_clear_fptr,                     \
                                        wm_clear_fptrl,                      \
                                        buffer_full_cntr_clear_fptr,         \
                                        clr_ppg_drop_state,                  \
                                        clr_gmin_usage,                      \
                                        clr_shared_usage,                    \
                                        clr_skid_usage,                      \
                                        clr_resume_lmt)                      \
  {                                                                          \
    g_ppg_cfg_hw_fptr_tbl.app_poolid_wr_fptr = poolid_fptr;                  \
    g_ppg_cfg_hw_fptr_tbl.is_dynamic_wr_fptr = is_dyn_fptr;                  \
    g_ppg_cfg_hw_fptr_tbl.baf_wr_fptr = baf_fptr;                            \
    g_ppg_cfg_hw_fptr_tbl.pfc_wr_fptr = pfc_fptr;                            \
    g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_clr_fptr = drop_clear_fptr;          \
    g_ppg_cfg_hw_fptr_tbl.ppg_wm_clr_fptr = wm_clear_fptrl;                  \
    g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_clr_fptr =                    \
        buffer_full_cntr_clear_fptr;                                         \
    g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_clr_fptr = clr_ppg_drop_state;      \
    g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_clr_fptr = clr_gmin_usage;     \
    g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_clr_fptr = clr_shared_usage; \
    g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_clr_fptr = clr_skid_usage;     \
    g_ppg_cfg_hw_fptr_tbl.resume_lmt_clr_fptr = clr_resume_lmt;              \
  }

#define BF_TM_PPG_HW_FPTR_TBL_GET_FUNCS(poolid_fptr,                          \
                                        is_dyn_fptr,                          \
                                        baf_fptr,                             \
                                        icos_fptr,                            \
                                        pfc_fptr,                             \
                                        drop_cntr_fptr,                       \
                                        drop_state_fptr,                      \
                                        gmin_usage_cntr_fptr,                 \
                                        shared_usage_cntr_fptr,               \
                                        skid_usage_cntr_fptr,                 \
                                        wm_cntr_fptr,                         \
                                        buffer_full_cntr_fptr,                \
                                        _ppg_allocation_fptr,                 \
                                        wacoffset_profile_fptr,               \
                                        fast_recover_rd_fptr,                 \
                                        resume_lmt,                           \
                                        ppg_defaults)                         \
  {                                                                           \
    g_ppg_cfg_hw_fptr_tbl.app_poolid_rd_fptr = poolid_fptr;                   \
    g_ppg_cfg_hw_fptr_tbl.is_dynamic_rd_fptr = is_dyn_fptr;                   \
    g_ppg_cfg_hw_fptr_tbl.baf_rd_fptr = baf_fptr;                             \
    g_ppg_cfg_hw_fptr_tbl.icos_mask_rd_fptr = icos_fptr;                      \
    g_ppg_cfg_hw_fptr_tbl.pfc_rd_fptr = pfc_fptr;                             \
    g_ppg_cfg_hw_fptr_tbl.ppg_drop_cntr_rd_fptr = drop_cntr_fptr;             \
    g_ppg_cfg_hw_fptr_tbl.ppg_drop_state_rd_fptr = drop_state_fptr;           \
    g_ppg_cfg_hw_fptr_tbl.ppg_gmin_usage_cntr_rd_fptr = gmin_usage_cntr_fptr; \
    g_ppg_cfg_hw_fptr_tbl.ppg_shared_usage_cntr_rd_fptr =                     \
        shared_usage_cntr_fptr;                                               \
    g_ppg_cfg_hw_fptr_tbl.ppg_skid_usage_cntr_rd_fptr = skid_usage_cntr_fptr; \
    g_ppg_cfg_hw_fptr_tbl.ppg_wm_cntr_rd_fptr = wm_cntr_fptr;                 \
    g_ppg_cfg_hw_fptr_tbl.wac_buffer_full_cntr_rd_fptr =                      \
        buffer_full_cntr_fptr;                                                \
    g_ppg_cfg_hw_fptr_tbl.ppg_allocation_fptr = _ppg_allocation_fptr;         \
    g_ppg_cfg_hw_fptr_tbl.restore_wac_offset_profile_fptr =                   \
        wacoffset_profile_fptr;                                               \
    g_ppg_cfg_hw_fptr_tbl.fast_recovery_rd_fptr = fast_recover_rd_fptr;       \
    g_ppg_cfg_hw_fptr_tbl.resume_lmt_rd_fptr = resume_lmt;                    \
    g_ppg_cfg_hw_fptr_tbl.defaults_rd_fptr = ppg_defaults;                    \
  }

void bf_tm_ppg_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_PPG_HW_FPTR_TBL_SET_LIMIT_FUNCS(NULL, NULL, NULL, NULL);
  BF_TM_PPG_HW_FPTR_TBL_GET_LIMIT_FUNCS(NULL, NULL, NULL, NULL);
  BF_TM_PPG_HW_FPTR_TBL_SET_FUNCS(
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  BF_TM_PPG_HW_FPTR_TBL_GET_FUNCS(NULL,
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

static void bf_tm_ppg_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  /* Depending on HW version (Tofino, Tofino-lite, ...) setup this
   * function table with correct function pointers
   */
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    // Tofino HW APIs
    BF_TM_PPG_HW_FPTR_TBL_SET_LIMIT_FUNCS(bf_tm_tofino_ppg_set_min_limit,
                                          bf_tm_tofino_ppg_set_skid_limit,
                                          bf_tm_tofino_ppg_set_app_limit,
                                          bf_tm_tofino_ppg_set_hyst);
    BF_TM_PPG_HW_FPTR_TBL_SET_FUNCS(bf_tm_tofino_ppg_set_app_poolid,
                                    bf_tm_tofino_ppg_set_dynamic_mode,
                                    bf_tm_tofino_ppg_set_baf,
                                    bf_tm_tofino_ppg_set_pfc_treatment,
                                    bf_tm_tofino_ppg_clear_drop_counter,
                                    bf_tm_tofino_ppg_clear_watermark,
                                    bf_tm_tofino_wac_clear_buffer_full_counter,
                                    bf_tm_tofino_ppg_clear_wac_drop_state,
                                    bf_tm_tofino_ppg_clear_gmin_usage_counter,
                                    bf_tm_tofino_ppg_clear_shared_usage_counter,
                                    bf_tm_tofino_ppg_clear_skid_usage_counter,
                                    bf_tm_tofino_ppg_clear_resume_limit);
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    // Tof2 HW APIs
    BF_TM_PPG_HW_FPTR_TBL_SET_LIMIT_FUNCS(bf_tm_tof2_ppg_set_min_limit,
                                          bf_tm_tof2_ppg_set_skid_limit,
                                          bf_tm_tof2_ppg_set_app_limit,
                                          bf_tm_tof2_ppg_set_hyst);
    BF_TM_PPG_HW_FPTR_TBL_SET_FUNCS(bf_tm_tof2_ppg_set_app_poolid,
                                    bf_tm_tof2_ppg_set_dynamic_mode,
                                    bf_tm_tof2_ppg_set_baf,
                                    bf_tm_tof2_ppg_set_pfc_treatment,
                                    bf_tm_tof2_ppg_clear_drop_counter,
                                    bf_tm_tof2_ppg_clear_wm_counter,
                                    bf_tm_tof2_wac_clear_buffer_full_counter,
                                    bf_tm_tof2_ppg_clear_drop_state,
                                    bf_tm_tof2_ppg_clear_gmin_usage_counter,
                                    bf_tm_tof2_ppg_clear_shared_usage_counter,
                                    bf_tm_tof2_ppg_clear_skid_usage_counter,
                                    bf_tm_tof2_ppg_clear_resume_limit);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    // Tof2 HW APIs
    BF_TM_PPG_HW_FPTR_TBL_SET_LIMIT_FUNCS(bf_tm_tof3_ppg_set_min_limit,
                                          bf_tm_tof3_ppg_set_skid_limit,
                                          bf_tm_tof3_ppg_set_app_limit,
                                          bf_tm_tof3_ppg_set_hyst);
    BF_TM_PPG_HW_FPTR_TBL_SET_FUNCS(bf_tm_tof3_ppg_set_app_poolid,
                                    bf_tm_tof3_ppg_set_dynamic_mode,
                                    bf_tm_tof3_ppg_set_baf,
                                    bf_tm_tof3_ppg_set_pfc_treatment,
                                    bf_tm_tof3_ppg_clear_drop_counter,
                                    bf_tm_tof3_ppg_clear_wm_counter,
                                    bf_tm_tof3_wac_clear_buffer_full_counter,
                                    bf_tm_tof3_ppg_clear_drop_state,
                                    bf_tm_tof3_ppg_clear_gmin_usage_counter,
                                    bf_tm_tof3_ppg_clear_shared_usage_counter,
                                    bf_tm_tof3_ppg_clear_skid_usage_counter,
                                    bf_tm_tof3_ppg_clear_resume_limit);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition.
  }
}

static void bf_tm_ppg_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_PPG_HW_FPTR_TBL_GET_LIMIT_FUNCS(bf_tm_tofino_ppg_get_min_limit,
                                          bf_tm_tofino_ppg_get_skid_limit,
                                          bf_tm_tofino_ppg_get_app_limit,
                                          bf_tm_tofino_ppg_get_hyst);
    BF_TM_PPG_HW_FPTR_TBL_GET_FUNCS(bf_tm_tofino_ppg_get_app_poolid,
                                    bf_tm_tofino_ppg_get_dynamic_mode,
                                    bf_tm_tofino_ppg_get_baf,
                                    bf_tm_tofino_ppg_get_icos_mask,
                                    bf_tm_tofino_ppg_get_pfc_treatment,
                                    bf_tm_tofino_ppg_get_drop_counter,
                                    bf_tm_tofino_ppg_get_drop_state,
                                    bf_tm_tofino_ppg_get_gmin_usage_counter,
                                    bf_tm_tofino_ppg_get_shared_usage_counter,
                                    bf_tm_tofino_ppg_get_skid_usage_counter,
                                    bf_tm_tofino_ppg_get_wm_counter,
                                    bf_tm_tofino_wac_get_buffer_full_counter,
                                    bf_tm_tofino_ppg_get_ppg_allocation,
                                    bf_tm_tofino_restore_wac_offset_profile,
                                    bf_tm_tofino_ppg_get_fast_recover_mode,
                                    bf_tm_tofino_ppg_get_resume_limit,
                                    bf_tm_tofino_ppg_get_defaults)
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_PPG_HW_FPTR_TBL_GET_LIMIT_FUNCS(bf_tm_tof2_ppg_get_min_limit,
                                          bf_tm_tof2_ppg_get_skid_limit,
                                          bf_tm_tof2_ppg_get_app_limit,
                                          bf_tm_tof2_ppg_get_hyst);
    BF_TM_PPG_HW_FPTR_TBL_GET_FUNCS(bf_tm_tof2_ppg_get_app_poolid,
                                    bf_tm_tof2_ppg_get_dynamic_mode,
                                    bf_tm_tof2_ppg_get_baf,
                                    bf_tm_tof2_ppg_get_icos_mask,
                                    bf_tm_tof2_ppg_get_pfc_treatment,
                                    bf_tm_tof2_ppg_get_drop_counter,
                                    bf_tm_tof2_ppg_get_drop_state,
                                    bf_tm_tof2_ppg_get_gmin_usage_counter,
                                    bf_tm_tof2_ppg_get_shared_usage_counter,
                                    bf_tm_tof2_ppg_get_skid_usage_counter,
                                    bf_tm_tof2_ppg_get_wm_counter,
                                    bf_tm_tof2_wac_get_buffer_full_counter,
                                    bf_tm_tof2_ppg_get_ppg_allocation,
                                    bf_tm_tof2_restore_wac_offset_profile,
                                    bf_tm_tof2_ppg_get_fast_recover_mode,
                                    bf_tm_tof2_ppg_get_resume_limit,
                                    bf_tm_tof2_ppg_get_defaults);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_PPG_HW_FPTR_TBL_GET_LIMIT_FUNCS(bf_tm_tof3_ppg_get_min_limit,
                                          bf_tm_tof3_ppg_get_skid_limit,
                                          bf_tm_tof3_ppg_get_app_limit,
                                          bf_tm_tof3_ppg_get_hyst);
    BF_TM_PPG_HW_FPTR_TBL_GET_FUNCS(bf_tm_tof3_ppg_get_app_poolid,
                                    bf_tm_tof3_ppg_get_dynamic_mode,
                                    bf_tm_tof3_ppg_get_baf,
                                    bf_tm_tof3_ppg_get_icos_mask,
                                    bf_tm_tof3_ppg_get_pfc_treatment,
                                    bf_tm_tof3_ppg_get_drop_counter,
                                    bf_tm_tof3_ppg_get_drop_state,
                                    bf_tm_tof3_ppg_get_gmin_usage_counter,
                                    bf_tm_tof3_ppg_get_shared_usage_counter,
                                    bf_tm_tof3_ppg_get_skid_usage_counter,
                                    bf_tm_tof3_ppg_get_wm_counter,
                                    bf_tm_tof3_wac_get_buffer_full_counter,
                                    bf_tm_tof3_ppg_get_ppg_allocation,
                                    bf_tm_tof3_restore_wac_offset_profile,
                                    bf_tm_tof3_ppg_get_fast_recover_mode,
                                    bf_tm_tof3_ppg_get_resume_limit,
                                    bf_tm_tof3_ppg_get_defaults);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition.
  }
}

void bf_tm_ppg_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_ppg_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_ppg_set_hw_ftbl_rd_funcs(tm_ctx);
}

/* Setup hw push function table for read only purposes.
 * Such mode is used in restoring config from HW.
 */
void bf_tm_ppg_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_ppg_null_hw_ftbl(tm_ctx);
  bf_tm_ppg_set_hw_ftbl_rd_funcs(tm_ctx);
}

int bf_tm_ppg_get_ppglist_for_port(bf_dev_id_t dev,
                                   uint8_t port,
                                   const bf_tm_ppg_t **ppglist) {
  // Walk through entire ppg datastruc and find list of ppgs that are mapped to
  // port
  bf_tm_ppg_t *ppg = g_tm_ctx[dev]->ig_ppg;
  int i, j, hit = 0;

  for (i = 0; i < 8; i++) {
    ppglist[i] = NULL;
  }
  for (j = 0; j < g_tm_ctx[dev]->tm_cfg.total_ppg_per_pipe; j++) {
    if (ppg->port == port) {
      for (i = 0; i < 8; i++) {
        if ((1 << i) & ppg->ppg_cfg.icos_mask) {
          ppglist[i] = ppg;
          hit++;
        }
      }
      ppg++;
    }
  }
  return (hit);
}

void bf_tm_ppg_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->ig_ppg) bf_sys_free(tm_ctx->ig_ppg);
}

bf_tm_status_t bf_tm_init_ppg(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_ppg_t *ppg;
  bf_dev_pipe_t pipe = 0;
  int i, j, p;

  if (tm_ctx->ig_ppg) {
    bf_sys_free(tm_ctx->ig_ppg);
  }

  tm_ctx->ig_ppg = bf_sys_calloc(1,
                                 sizeof(bf_tm_ppg_t) * tm_ctx->tm_cfg.pipe_cnt *
                                     tm_ctx->tm_cfg.total_ppg_per_pipe);
  if (!tm_ctx->ig_ppg) {
    LOG_ERROR("Failed to initialize PPG. (status %s)",
              bf_err_str(BF_NO_SYS_RESOURCES));
    return (BF_NO_SYS_RESOURCES);
  }
  ppg = tm_ctx->ig_ppg;
  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(tm_ctx->devid, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if (lld_sku_map_pipe_id_to_phy_pipe_id(tm_ctx->devid, i, &pipe) != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          tm_ctx->devid,
          i);
    }
    bf_sys_assert(pipe < BF_TM_MAX_MAU_PIPES);
    p = 0;
    for (j = 0; j < tm_ctx->tm_cfg.total_ppg_per_pipe; j++) {
      ppg->p_pipe = pipe;
      ppg->d_pipe = (pipe % 4);
      ppg->l_pipe = i;
      ppg->ppg = j;
      ppg->in_use = false;
      ppg->port = BF_TM_INVALID_PORT;
      ppg->uport = BF_TM_INVALID_PORT;
      ppg->ppg_cfg.app_poolid = tm_ctx->tm_cfg.gmin_pool;
      ppg->ppg_cfg.icos_mask = 0;
      if (j >= tm_ctx->tm_cfg.pfc_ppg_per_pipe) {
        // set deafult ppg attributes
        ppg->is_default_ppg = true;
        ppg->port = p++;
        ppg->uport =
            DEV_PORT_TO_LOCAL_PORT(lld_sku_map_devport_from_device_to_user(
                tm_ctx->devid, MAKE_DEV_PORT(ppg->l_pipe, ppg->port)));
        // At startup / default, set default-PPG icos-mask to 0xff
        // As when PPGs of a port take over icos, default-ppg
        // icos-mask will get modified.
        ppg->ppg_cfg.icos_mask = 0xff;
      }
      ppg++;
    }
  }
  bf_tm_ppg_set_hw_ftbl(tm_ctx);

  LOG_TRACE("PPG Structures initialized");

  return (rc);
}

/*
 * Setup the Cache Counters for default PPG. Default/Non Default PPGs are setup
 * per port
 *
 * Related APIs:
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_set_cache_counters(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  if (_ppg->counter_state_list != NULL) {
    tm_free_cache_counter_node(TM_PPG, _ppg->counter_state_list, g_tm_ctx[dev]);
    _ppg->counter_state_list = NULL;
  }

  // Add the Cached Counter Node for < 64 bit counters wrap case.
  tm_counter_node_id_t node_id;
  tm_cache_counter_node_list_t *node_ptr = NULL;
  TRAFFIC_MGR_MEMSET(&node_id, 0, sizeof(tm_counter_node_id_t));
  node_id.pipe = pipe;
  node_id.ppg = ppg;
  rc = tm_allocate_and_init_cache_counter_node(
      TM_PPG, g_tm_ctx[dev], &node_id, &node_ptr);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to add Cached Counters for TM default ppg (0x%x) ", ppg);
    _ppg->counter_state_list = NULL;
    // Not sure if we can call the free here
    // bf_tm_ppg_free(dev, *ppg);
  } else {
    _ppg->counter_state_list = node_ptr;
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}
