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
 * This file implements TM Ingress APIs exported to client/application.
 * APIs in this file are northbound interface to TM.
 */

#include <traffic_mgr/traffic_mgr.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "tm_api_helper.h"

/*
 * Allocate an unused PPG. The new PPG can be used to implement part of QoS
 * behaviour at ingress TM. If PPGs are exhausted, the API fails.
 *
 * Related APIs: bf_tm_ppg_free(), bf_tm_ppg_defaultppg_get()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param ppg        ppg handle is allocated if available.
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_allocate(bf_dev_id_t dev,
                               bf_dev_port_t port,
                               bf_tm_ppg_hdl *ppg) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_alloc_ppg(dev, port, ppg);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Free PPG back to free pool.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle to free. PPG handle specifies
 *                   pipe to which PPG belongs to. PPG resource
 *                   will be returned to that pipe.
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_free(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, lpipe, port, i;
  bf_tm_ppg_t *_ppg;
  bf_tm_port_t *_p;
  bf_dev_port_t lport;
  uint8_t icos_mask;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &lpipe, &port);
  if (defppg) return (rc);
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], lpipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, lpipe, dev);

  lport = MAKE_DEV_PORT(lpipe, port);
  _p = BF_TM_PORT_PTR(g_tm_ctx[dev], lport);
  if (!_p) {
    LOG_ERROR(
        "%s: Invalid port %d passed in for device %d", __func__, lport, dev);
    return (BF_INVALID_ARG);
  }

  if (!_p->ppg_count) {
    // This port has no PPGs allocated... Internal error or
    // PPG is being double freed by application.
    return (rc);  // Consider it as double free and return success.
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  // before freeing up PPG, map PPG under free PFC traffic
  // that is not mapped to any other PFC PPG
  // to default PPG; also disable PFC enable bit on relevant cos
  // clear ppg<-->icos mappings (reverse mapping as well)
  // and set app poolid to default pool.

  bf_tm_ppg_t *default_ppg =
      BF_TM_PPG_PTR(g_tm_ctx[dev],
                    lpipe,
                    (g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe +
                     DEV_PORT_TO_LOCAL_PORT(
                         lld_sku_map_devport_from_user_to_device(dev, lport))));

  BF_TM_PPG_CHECK(default_ppg,
                  (g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe +
                   DEV_PORT_TO_LOCAL_PORT(
                       lld_sku_map_devport_from_user_to_device(dev, lport))),
                  lpipe,
                  dev);

  icos_mask = _ppg->ppg_cfg.icos_mask;

  // filter out icos traffic that was moved from the PPG under
  // free to a different PPG.
  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if ((1 << i) & icos_mask) {
      if (_p->ppgs[i] != _ppg) {
        // icos traffic has been moved to use different PPG.
        // do not move that icos traffic to default PPG.
        icos_mask &= ~(1 << i);
      }
    }
  }

  // Include icos_mask  of the PPG under free into default PPG.
  default_ppg->ppg_cfg.icos_mask |= icos_mask;

  // Use set_app_poolid API to program port-to-ppg mapping table.
  bf_tm_ppg_set_app_poolid(dev, default_ppg, default_ppg->ppg_cfg.app_poolid);
  // disable PFC enable bit/s on the port to which PPG is mapped to.
  bf_tm_ppg_set_pfc_treatment(dev, default_ppg, false);
  // clear reverse mapping. icos-->ppg
  bf_tm_ppg_set_icos_mask(dev, _ppg, 0);

  // Now its safe to declare PPG is free so that it can be assgined to
  // a different port,icos traffic.
  _ppg->in_use = false;
  for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
    if (_p->ppg_list[i] == _ppg) {
      _p->ppg_list[i] = NULL;
    }
  }
  _p->ppg_count -= 1;

  // Free up the Cache Counter Allocated
  if (_ppg->counter_state_list != NULL) {
    rc = tm_free_cache_counter_node(
        TM_PPG, _ppg->counter_state_list, g_tm_ctx[dev]);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to delete Cached Counters for TM PPG (0x%x) ", ppg);
    }
    _ppg->counter_state_list = NULL;
  }

  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Get default PPG associated with port. The default PPG handle can be used to
 * configure PPG limit. One or many iCoS (iCoS = ig_intr_md.ingress_cos)
 * traffic is mapped to default PPG. At the start all iCoS traffic on a port
 * is mapped to default PPG. By using API bf_tm_ppg_icos_mapping_set(), all or
 * subset of iCoS traffic can be moved to a PPG of their own.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param ppg        ppg handle associated with port
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_defaultppg_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_ppg_hdl *ppg) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(DEV_PORT_TO_PIPE(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  *ppg = g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe +
         DEV_PORT_TO_LOCAL_PORT(
             lld_sku_map_devport_from_user_to_device(dev, port));
  *ppg = (DEV_PORT_TO_PIPE(port) << 12) | *ppg;  // Fold Pipe# in the handle.
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * This API will assign iCoS (iCoS = ig_intr_md.ingress_cos) traffic on port
 * that PPG is attached to. PPG handle can be default ppg, non-default ppg.
 * If its default ppg, then all the icos mapped traffic is treated lossy. If
 * not, then depending on whether lossless treatment is attached to ppg using
 * bf_tm_ppg_lossless_treatment_enable() or not, all the icos traffic
 * specified using icos_bmap will be treated accordingly.
 *
 * Default : When PPG is allocated, no icos is mapped. User has to explicit
 *           assign one or mode iCoS to the PPG.
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_set()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle
 * param icos_bmap  Bit map of iCoS (iCoS = ig_intr_md.ingress_cos).
 *                   Bit 7 is interpreted as iCoS 7.
 * return           Status of API call.
 *
 */
bf_status_t bf_tm_ppg_icos_mapping_set(bf_dev_id_t dev,
                                       bf_tm_ppg_hdl ppg,
                                       uint8_t icos_bmap) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  // BF_TM_INVALID_ARG(TM_IS_COSBMAP_INVALID(icos_bmap));
  // icos_bmap = 0 is valid when user wants to move existing flows
  // to default PPG
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  if (defppg) {
    // Do not allow setting icos on default PPG. All iCoS that are
    // not mapped to any PPG will be automatically mapped to
    // default ppg.
    return (rc);
  }
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  rc = bf_tm_ppg_set_icos_mask(dev, _ppg, icos_bmap);

  // If port-ppg mapping table was programmed because  api
  // bf_tm_ppg_app_pool_usage_set()  was called before this API
  // then fill in the missing icos by calling the APi again.
  // Hence use the poolid from PPG structure. (If the APi was not
  // called then default app POOL which is app pool 0 will be used.
  // By programming app poolid, port-ppg mapping table entries for
  // all the cos in cos-mask is programmed.
  if (rc == BF_SUCCESS) {
    rc = bf_tm_ppg_set_app_poolid(dev, _ppg, _ppg->ppg_cfg.app_poolid);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Enable lossless treatment for PPG. The PPG handle to use is
 * obtained using bf_tm_ppg_allocate(). All traffic mapped to the PPG is
 * considered as lossless traffic.
 *
 * Default : Default property of PPG is to treat traffic as lossy.
 *
 * Related APIs: bf_tm_ppg_lossless_treatment_disable()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_lossless_treatment_enable(bf_dev_id_t dev,
                                                bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  // 1. Search wac_port_ppg_mapping table and find the port (only one port
  // per PPG) associated with PPG
  // 2. Lookup wac_ppg_icos_mapping table and find icos-mask associated with
  // ppg.
  // 3. Program wac_port_pfc_en table using port as index and icos-mas as value.
  // Above 3 steps can be achieved by walking through PPG datastructure; hence
  // avoids issuing reads to HW/ASIC
  rc = bf_tm_ppg_set_pfc_treatment(dev, _ppg, true);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Disable lossless treatment for PPG. The PPG handle that is obtained using
 * bf_tm_ppg_allocate() can be used for lossy traffic.
 *
 * Related APIs: bf_tm_ppg_lossless_treatment_enable()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_lossless_treatment_disable(bf_dev_id_t dev,
                                                 bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_set_pfc_treatment(dev, _ppg, false);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear PPG drop state register in WAC
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg handle
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_wac_drop_state_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_clear_drop_state(dev, _ppg);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * A non deafult PPG can be optionally assigned to any application pool.
 * When assigned to application pool, static or dynamic shared limit
 * can be set. This API aids to achieve that. If it is desired to not
 * assign PPG to any pool, then this API need not be invoked.
 *
 * Default: PPG is not assigned to any application pool.
 *
 * Related APIs: bf_tm_disable_ppg_app_pool_usage(),
 *               bf_tm_ppg_app_pool_usage_get()
 *
 * param dev             ASIC device identifier.
 * param ppg             ppg handle.
 * param pool            Application pool to which PPG is assigned to.
 * param base_use_limit  Limit to which PPG can grow inside application
 *                       pool. Limit is specified in cell count.
 *                       Once this limit is crossed, if PPG burst
 *                       absroption factor (BAF) is non zero, depending
 *                       availability of buffer, PPG is allowed to
 *                       use buffer upto BAF limit. If BAF limit is zero,
 *                       PPG is treated as static and no dynamic thresholding.
 * param dynamic_baf     One of the values listed in bf_tm_ppg_baf_t
 *                       When BF_TM_PPG_BAF_DISABLE is used, PPG uses static
 *                       limit.
 * param hysteresis      Hysteresis value in cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_ppg_app_pool_usage_set(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg,
                                         bf_tm_app_pool_t pool,
                                         uint32_t base_use_limit,
                                         bf_tm_ppg_baf_t dynamic_baf,
                                         uint32_t hysteresis) {
  bf_status_t rc = BF_SUCCESS;
  int dir;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(base_use_limit, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(hysteresis, g_tm_ctx[dev]));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));
  if (dir == BF_TM_DIR_EGRESS) {
    return (BF_INVALID_ARG);
  }
  bool is_dyn = bf_tm_api_hlp_is_baf_dynamic(dynamic_baf);
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  do {
    rc = bf_tm_ppg_set_app_limit(dev, _ppg, base_use_limit);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      break;
    }
    rc = bf_tm_ppg_set_is_dynamic(dev, _ppg, is_dyn);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      break;
    }
    if (is_dyn) rc = bf_tm_ppg_set_baf(dev, _ppg, dynamic_baf);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      break;
    }
    rc = bf_tm_ppg_set_ppg_hyst(dev, _ppg, hysteresis);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      break;
    }
    rc = bf_tm_ppg_set_app_poolid(dev, _ppg, id);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      break;
    }
  } while (false);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * This API can be used to move a PPG that is assigned to application
 * pool to default.
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_set(), bf_tm_ppg_app_pool_usage_get()
 *
 * param dev        ASIC device identifier.
 * param pool       Pool handle. Valid pools are BF_TM_IG_POOL_0..3
 *                   and BF_TM_SKID_POOL.
 * param ppg        ppg handle
 * return           Status of API call.
 *
 */

bf_status_t bf_tm_ppg_app_pool_usage_disable(bf_dev_id_t dev,
                                             bf_tm_app_pool_t pool,
                                             bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int dir;
  bool defppg;
  int ppg_n, pipe, port;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  (void)bf_tm_api_hlp_get_pool_details(pool, &dir);
  if (dir == BF_TM_DIR_EGRESS) {
    return (BF_INVALID_ARG);
  }
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, &port);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  // Seek clarification. there is no config that can NOT assign PPG to any app
  // pool
  // The way its done is PPG is always mapped to some shared pool (0 -- default)
  // and
  // base limit in shared pool is set to ZERO.
  rc = bf_tm_ppg_set_app_limit(dev, _ppg, 0);

  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set miminum limits of ppg. Inorder to increase min limits, that
 * many free cell should be available.
 *
 * Default : TM buffer is equally assigned to all PPGs.
 *
 * Related APIs: bf_tm_ppg_guaranteed_min_limit_get(),
 *               bf_tm_ppg_guaranteed_min_skid_hysteresis_set()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg whose limits has to be adjusted.
 * param cells      Number of cells by which minimum ppg limit
 *                   has be increased.
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_guaranteed_min_limit_set(bf_dev_id_t dev,
                                               bf_tm_ppg_hdl ppg,
                                               uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_set_min_limit(dev, _ppg, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set ppg skid limits. Cannot be increased beyond the  size of
 * Skid pool size. If decreased to zero, in transit ppg traffic
 * after asserting pause will be dropped.
 *
 * Default : Skid limits are set to zero.
 *
 * Related APIs: bf_tm_ppg_skid_limit_get(),
 *               bf_tm_ppg_guaranteed_min_skid_hysteresis_set()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg whose skid limits has to be increased.
 * param cells      Limits in terms of number of cells.
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_skid_limit_set(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  if (defppg) {
    // cannot set skid size on default ppg
    return (BF_INVALID_ARG);
  }
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_set_skid_limit(dev, _ppg, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear ppg resume limits.
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg whose resume limits has to be cleared.
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_resume_limit_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_clear_resume_limit(dev, _ppg);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set ppg hysteresis limits.
 * Hysterisis limits are numbers of cells the ppg usage should fall by
 * from its limit value. Once usage limits are below hysteresis, appropriate
 * condition is cleared. Example when PPG's skid usage limit falls
 * below its allowed limit by hysteresis value, drop condition is
 * cleared.
 *
 * Note: Upto only 32 different hysteresis limits are supported. All PPGs
 *       will have to use one of these 32 different hysteresis limits.
 *       If more than hardware supported  different hysteresis limit is
 *       requested, then the API will fail.
 *
 * Default: Skid hysteresis limit set to zero. No hysteresis.
 *
 * Related APIs: bf_tm_ppg_skid_limit_set(),
 *               bf_tm_guaranteed_min_skid_hysteresis_get()
 *
 * param dev        ASIC device identifier.
 * param ppg        ppg whose resume offset limits has to be increased.
 * param cells      Limits in terms of number of cells
 * return           Status of API call.
 */
bf_status_t bf_tm_ppg_guaranteed_min_skid_hysteresis_set(bf_dev_id_t dev,
                                                         bf_tm_ppg_hdl ppg,
                                                         uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_set_ppg_hyst(dev, _ppg, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}
