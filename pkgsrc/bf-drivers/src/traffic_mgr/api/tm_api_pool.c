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


/* This file implements TM buffer management APIs exported to
 * client/application.
 * APIs in this file are the northbound interface to TM.
 */

#include <traffic_mgr/traffic_mgr.h>

#include "traffic_mgr/common/tm_ctx.h"

/*
 * Set application pool size. When application pool size is set, the
 * required number of cells are obtained from unassigned portion of
 * TM buffer cells. Unassigned cells = (Total buffer cells -
 * gmin size of every PPG or Queue - sizes of all applicaiton pools).
 * When required number of cells are not available, the API fails.
 * PPGs or Queues can be mapped to this new application pool.
 *
 * Default:   By default application pool size is set to zero.
 *
 * param dev             ASIC device identifier.
 * param pool            pool identifier. Any of BF_TM_IG_APP_POOL_0..3
 *                       or BF_TM_EG_APP_POOL_0..3 is valid application pool.
 * param cells           Size of pool in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_size_set(bf_dev_id_t dev,
                                bf_tm_app_pool_t pool,
                                uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (dir == BF_TM_DIR_INGRESS) {
    rc = bf_tm_ig_spool_set_green_limit(dev, id, g_tm_ctx[dev]->ig_pool, cells);
  } else {
    rc = bf_tm_eg_spool_set_green_limit(dev, id, g_tm_ctx[dev]->eg_pool, cells);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Enable Application pool's color drop condition. If color drop condition
 * is enabled, when pool color threshold limit are reached packets are
 * dropped. When color drop is not enabled, packets do not get any
 * treatment based on their color at pool level.
 *
 * Default : Default policy is to trigger drops based on color.
 *
 *
 * param dev             ASIC device identifier.
 * param pool            Application pool identifier.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_enable(bf_dev_id_t dev,
                                         bf_tm_app_pool_t pool) {
  bf_status_t rc = BF_SUCCESS;
  int dir;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (dir == BF_TM_DIR_INGRESS) {
    rc = bf_tm_ig_spool_set_color_drop(dev, id, g_tm_ctx[dev]->ig_pool, true);
  } else {
    rc = bf_tm_eg_spool_set_color_drop(dev, id, g_tm_ctx[dev]->eg_pool, true);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable Application pool's color drop condition. By disabling color drop
 * condition, packets do not get any treatment based on their color at
 * pool level.
 *
 * Default : Default policy is to trigger drops based on color.
 *
 *
 * param dev             ASIC device identifier.
 * param pool            Application pool identifier.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_disable(bf_dev_id_t dev,
                                          bf_tm_app_pool_t pool) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (dir == BF_TM_DIR_INGRESS) {
    rc = bf_tm_ig_spool_set_color_drop(dev, id, g_tm_ctx[dev]->ig_pool, false);
  } else {
    rc = bf_tm_eg_spool_set_color_drop(dev, id, g_tm_ctx[dev]->eg_pool, false);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set Application pool color drop limit.
 *
 * Default : By default, color drop limits are set to 100% of pool size
 *           for all colors.
 *
 *
 * param dev             ASIC device identifier.
 * param pool            Application pool handle.
 * param color           Color (Green, Yellow, Red)
 * param limit           Limits in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_limit_set(bf_dev_id_t dev,
                                            bf_tm_app_pool_t pool,
                                            bf_tm_color_t color,
                                            uint32_t limit) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(limit, g_tm_ctx[dev]));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_set_green_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit)
               : bf_tm_eg_spool_set_green_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit);
      break;
    case BF_TM_COLOR_YELLOW:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_set_yel_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit)
               : bf_tm_eg_spool_set_yel_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit);
      break;
    case BF_TM_COLOR_RED:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_set_red_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit)
               : bf_tm_eg_spool_set_red_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit);
      break;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set color drop hysteresis limits. The same hysteresis value is applied
 * on all application pools. Resume condition is triggered when pool usage
 * drops by hysteresis value from the limit value when color drop condition
 * was set.
 *
 * Default : By default hysteresis value is set to zero; or no hysterisis
 *
 *
 * param dev             ASIC device identifier.
 * param color           Color (Green, Yellow, Red)
 * param limit           Limit in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_hysteresis_set(bf_dev_id_t dev,
                                                 bf_tm_color_t color,
                                                 uint32_t limit) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(limit, g_tm_ctx[dev]));

  if (!TM_IS_8CELL_UNITS(limit)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        limit,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(limit)));
  }

  // Api sets same hyst in both ingress and egress TM pools.
  // Create a new API that accepts ingress or egress direction as argument.

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = bf_tm_ig_spool_set_green_hyst(dev, g_tm_ctx[dev]->ig_pool, limit);
      rc |= bf_tm_eg_spool_set_green_hyst(dev, g_tm_ctx[dev]->eg_pool, limit);
      break;
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_ig_spool_set_yel_hyst(dev, g_tm_ctx[dev]->ig_pool, limit);
      rc |= bf_tm_eg_spool_set_yel_hyst(dev, g_tm_ctx[dev]->eg_pool, limit);
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_ig_spool_set_red_hyst(dev, g_tm_ctx[dev]->ig_pool, limit);
      rc |= bf_tm_eg_spool_set_red_hyst(dev, g_tm_ctx[dev]->eg_pool, limit);
      break;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set per PFC level limit values. PFC level limits are configurable on per
 * applicaiton pool basis. When PPG usage numbers hit pfc limits, PFC is
 *triggered
 * for lossless traffic.
 *
 *
 * param dev             ASIC device identifier.
 * param pool            Application pool handle for which limits are
 *configured.
 * param icos            Internal CoS (iCoS = ig_intr_md.ingress_cos) level
 *                        on which limits are applied.
 * param limit           Limit value in terms of cell count to increase.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_pfc_limit_set(bf_dev_id_t dev,
                                     bf_tm_app_pool_t pool,
                                     bf_tm_icos_t icos,
                                     uint32_t limit) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(limit, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_ICOS_INVALID(icos));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  if (dir == BF_TM_DIR_EGRESS) {
    return (BF_INVALID_ARG);
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_spool_set_pfc_limit(
      dev, id, icos, g_tm_ctx[dev]->ig_pool, limit);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set skid pool size. Skid pool size can be oversubscribed. It need
 * not be sum of per ppg skid limits that are mapped to skid pool.
 * The reason being it is not practical scenario. Bursty behaviour could
 * be one or few PPGs at a time.
 *
 * Default:  At the start, default skid pool size is set zero.
 *
 *
 * param dev             ASIC device identifier.
 * param cells           Size of pool in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_skid_size_set(bf_dev_id_t dev, uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_gpool_set_skid_limit(dev, g_tm_ctx[dev]->ig_pool, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set global skid pool hysteresis.
 *
 * Default : By deafult hysteresis value is set to zero; or no hysterisis
 *
 *
 * param dev        ASIC device identifier.
 * param cells      Number of cells set as skid pool hysteresis. If not a
 *                  multiple of 8, gets rounded down.
 * return           Status of API call.
 */
bf_status_t bf_tm_pool_skid_hysteresis_set(bf_dev_id_t dev, uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));

  if (!TM_IS_8CELL_UNITS(cells)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        cells,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(cells)));
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_gpool_set_skid_hyst(dev, g_tm_ctx[dev]->ig_pool, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set negative mirror pool limit in units of cells.
 *
 * Default : Negative mirror pool size is zero.
 *
 *
 * param dev        ASIC device identifier.
 * param cells      New size of pool.
 * return           Status of API call.
 */
bf_status_t bf_tm_pool_mirror_on_drop_size_set(bf_dev_id_t dev,
                                               uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  // Same size is set on both ingress and egress TM.
  // Add another API to independently set .
  rc = bf_tm_ig_gpool_set_dod_limit(dev, g_tm_ctx[dev]->ig_pool, cells);
  rc |= bf_tm_eg_gpool_set_dod_limit(dev, g_tm_ctx[dev]->eg_pool, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set per pipe per FIFO limit in units of cells.
 *
 * Default : 0xc0
 *
 * Related APIs: bf_tm_pre_fifo_limit_get()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] pipe      Pipe identifier.
 * @param[in] fifo      FIFO identifier.
 * @param[in] cells     New size of pool.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pre_fifo_limit_set(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     uint8_t fifo,
                                     uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_eg_gpool_set_fifo_limit(
      dev, g_tm_ctx[dev]->eg_pool, pipe, fifo, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set Ingress total reserved buffer space in units of cells, at least larger
 * than total sum of 'ppg_min
 * Tofino2 and above support Only
 *
 * Default : 0x6000
 *
 *
 * param dev        ASIC device identifier.
 * param cells      New size of pool.
 * return           Status of API call.
 */
bf_status_t bf_tm_global_min_limit_set(bf_dev_id_t dev, uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  // Same size is set on both ingress and egress TM.
  // Add another API to independently set .
  rc |= bf_tm_ig_gpool_set_glb_min_limit(dev, g_tm_ctx[dev]->ig_pool, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set cut through size for unicast traffic. This size determines total
 * buffer set aside for cut through traffic.
 *
 * Default: All ports are in unicast cut through mode. The size is set
 *          accomodate worst case TM latency to transfer packet from ingress
 *          TM to egress TM.
 *
 *
 * param dev             ASIC device identifier.
 * param cells           Size of pool in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_uc_cut_through_size_set(bf_dev_id_t dev,
                                               uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_uc_ct_size_set(dev, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set cut through pool size for Multicast traffic. This size determines total
 * buffer set aside for cut through traffic.
 *
 * Default: Using TM latency as metric and average replications
 *          default value will be set such that multicast traffic
 *          leverages cut through performance.
 *
 *
 * param dev             ASIC device identifier.
 * param cells           Size of pool in terms of cells.
 * return                Status of API call.
 */
bf_status_t bf_tm_pool_mc_cut_through_size_set(bf_dev_id_t dev,
                                               uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mc_ct_size_set(dev, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set Ingress global max cell limit threshold
 * Tofino2 and above support Only
 *
 * Default : Global cell limit is set to 256000
 *
 * param dev        ASIC device identifier.
 * param cells      Number of cells
 * return           BF_SUCCESS on success
 *                  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_set(bf_dev_id_t dev, uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_gpool_set_glb_cell_limit(dev, g_tm_ctx[dev]->ig_pool, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enables Ingress global max cell limit.
 * Tofino2 and above support Only
 *
 * Default : Global cell limit is enabled.
 *
 * param dev        ASIC device identifier.
 * return           BF_SUCCESS on success
 *                  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_enable(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_gpool_set_glb_cell_limit_state(
      dev, g_tm_ctx[dev]->ig_pool, true);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disables Ingress global max cell limit.
 * Tofino2 and above support Only
 *
 * Default : Global cell limit is enabled.
 *
 * param dev        ASIC device identifier.
 * return           BF_SUCCESS on success
 *                  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_disable(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ig_gpool_set_glb_cell_limit_state(
      dev, g_tm_ctx[dev]->ig_pool, false);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}
