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

// This file contains all functions related to ucli display tables functionality

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stddef.h>
#include "traffic_mgr/common/tm_error.h"
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr_ucli_apis.h"
#include <target-utils/uCli/ucli_common.h>
#include <traffic_mgr/traffic_mgr_types.h>
#include <traffic_mgr/traffic_mgr_read_apis.h>

#define HLINE                                                                  \
  "__________________________________________________________________________" \
  "__________________________________________________________________________"

#define TM_UCLI_CHECK_AND_PRINT_ERROR(err_code)                             \
  if (BF_SUCCESS != err_code) {                                             \
    LOG_ERROR(                                                              \
        "UCLI: Unexpected error at %s:%d code %d", __func__, __LINE__, rc); \
    if (BF_NOT_SUPPORTED != err_code) {                                     \
      aim_printf(&uc->pvs, "Error occured. BF_STATUS: %d\n", err_code);     \
      return (err_code);                                                    \
    }                                                                       \
  }

#define TM_UCLI_CHECK_ERROR(err_code) \
  if (BF_SUCCESS != err_code) {       \
    return (err_code);                \
  }

#define TM_UCLI_PRINT_BOOLEAN(value, chars_num) \
  aim_printf(&uc->pvs, "%-" #chars_num "s", value ? "True" : "False");

inline static float bf_tm_get_ig_ppg_baf_percentage(bf_tm_ppg_baf_t baf) {
  switch (baf) {
    case BF_TM_PPG_BAF_1_POINT_5_PERCENT:
      return 1.5;
    case BF_TM_PPG_BAF_3_PERCENT:
      return 3.0;
    case BF_TM_PPG_BAF_6_PERCENT:
      return 5.0;
    case BF_TM_PPG_BAF_11_PERCENT:
      return 10.0;
    case BF_TM_PPG_BAF_20_PERCENT:
      return 20.0;
    case BF_TM_PPG_BAF_33_PERCENT:
      return 33.0;
    case BF_TM_PPG_BAF_50_PERCENT:
      return 50.0;
    case BF_TM_PPG_BAF_66_PERCENT:
      return 66.0;
    case BF_TM_PPG_BAF_80_PERCENT:
      return 80.0;
    default:
      return 0.0;
  }
}

inline static float bf_tm_get_eg_q_baf_percentage(bf_tm_queue_baf_t baf) {
  switch (baf) {
    case BF_TM_Q_BAF_1_POINT_5_PERCENT:
      return 1.5;
    case BF_TM_Q_BAF_3_PERCENT:
      return 3.0;
    case BF_TM_Q_BAF_6_PERCENT:
      return 6.0;
    case BF_TM_Q_BAF_11_PERCENT:
      return 11.0;
    case BF_TM_Q_BAF_20_PERCENT:
      return 20.0;
    case BF_TM_Q_BAF_33_PERCENT:
      return 33.0;
    case BF_TM_Q_BAF_50_PERCENT:
      return 50.0;
    case BF_TM_Q_BAF_66_PERCENT:
      return 66.0;
    case BF_TM_Q_BAF_80_PERCENT:
      return 80.0;
    default:
      return 0.0;
  }
}

inline static float bf_tm_get_q_color_limit_percentage(
    bf_tm_queue_color_limit_t limit) {
  switch (limit) {
    case BF_TM_Q_COLOR_LIMIT_12_POINT_5_PERCENT:
      return 12.5;
    case BF_TM_Q_COLOR_LIMIT_25_PERCENT:
      return 25.0;
    case BF_TM_Q_COLOR_LIMIT_37_POINT_5_PERCENT:
      return 37.5;
    case BF_TM_Q_COLOR_LIMIT_50_PERCENT:
      return 50.0;
    case BF_TM_Q_COLOR_LIMIT_62_POINT_5_PERCENT:
      return 62.5;
    case BF_TM_Q_COLOR_LIMIT_75_PERCENT:
      return 75.0;
    case BF_TM_Q_COLOR_LIMIT_87_POINT_5_PERCENT:
      return 87.5;
    case BF_TM_Q_COLOR_LIMIT_100_PERCENT:
      return 100.0;
    default:
      return 0.0;
  }
}

inline static int bf_tm_get_port_speed_mode_g(bf_port_speed_t speed) {
  switch (speed) {
    case BF_SPEED_10G:
      return 10;
    case BF_SPEED_25G:
      return 25;
    case BF_SPEED_40G:
      return 40;
    case BF_SPEED_50G:
      return 50;
    case BF_SPEED_100G:
      return 100;
    case BF_SPEED_200G:
      return 200;
    case BF_SPEED_400G:
      return 400;
    default:
      return 0;
  }
}

bf_status_t tm_ucli_display_ppg_min_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-19s", "MinLimit ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 26, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ++ppg) {
    uint32_t limit = 0;
    bf_tm_ppg_hdl ppg_hndl = 0;
    // we dont need full filled ppg type, only pipe and ppg#, so skip port field
    ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);

    rc |= bf_tm_ppg_guaranteed_min_limit_get(devid, ppg_hndl, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    // we don`t print anything only if
    // print-non-zero parameter is false and
    // value is 0 (false)
    const bool print = (nz || limit);
    if (!print) continue;

    aim_printf(&uc->pvs, "%-7u", ppg);
    if (!hex) {
      aim_printf(&uc->pvs, "%-19u", limit);
    } else {
      aim_printf(&uc->pvs, "%-19x", limit);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}
// TM_PIPE_WATERMARK_TABLES
bf_status_t tm_ucli_display_wac_ppg_watermark(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-7s", "PerPPGtWaterMark");
  aim_printf(&uc->pvs, "%-19s", "Buffer Usage Watermark ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 26, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hdl = 0;
    // we dont need full filled ppg type, only pipe and ppg#, so skip port field
    ppg_hdl = BF_TM_PPG_HANDLE(pipe, 0, ppg);

    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc |= bf_tm_ppg_usage_get(
        devid, pipe, ppg_hdl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    // we don`t print anything only if
    // print-non-zero parameter is false and
    // value is 0 (false)
    const bool print = (nz || wm);
    if (!print) continue;

    aim_printf(&uc->pvs, "%-7u", ppg);
    if (!hex) {
      aim_printf(&uc->pvs, "%-19u", wm);
    } else {
      aim_printf(&uc->pvs, "%-19x", wm);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_wac_port_watermark(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  aim_printf(&uc->pvs, "%-20s", "PerPortWaterMark");
  aim_printf(&uc->pvs, "%-19s", "Buffer Usage Watermark ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 53, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t ig_count = 0, eg_count = 0, ig_wm = 0, eg_wm = 0;
    rc = bf_tm_port_usage_get(
        devid, pipe, devport, &ig_count, &eg_count, &ig_wm, &eg_wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz || ig_wm);

    if (print) {
      aim_printf(&uc->pvs, "%-20d", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-23u", ig_wm);
      } else {
        aim_printf(&uc->pvs, "%-23x", ig_wm);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_q_wm(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-19s", "WaterMark cells");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 63, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  for (uint32_t die_id = 0; die_id < cfg.pipe_cnt / BF_SUBDEV_PIPE_COUNT;
       die_id++) {
    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(
          &uc->pvs, "---------------SUBDEV %d START------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }

    bf_dev_port_t devport = 0;
    rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    do {
      uint8_t q_count;
      uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
      rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      uint32_t watermark[BF_TM_MAX_QUEUE_PER_PG];
      uint32_t print_port = 0;
      for (uint8_t portq = 0; portq < q_count; ++portq) {
        uint32_t count = 0, wm = 0;
        if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
          rc = bf_tm_q_usage_get_ext(
              devid, die_id, pipe, devport, portq, &count, &wm);
        } else {
          rc = bf_tm_q_usage_get(devid, pipe, devport, portq, &count, &wm);
        }
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        watermark[portq] = wm;
        print_port |= wm;
      }

      if (print_port) {
        aim_printf(&uc->pvs,
                   "%1d/%-7d",
                   DEV_PORT_TO_PIPE(devport),
                   DEV_PORT_TO_LOCAL_PORT(devport));
        for (uint8_t portq = 0; portq < q_count; ++portq) {
          const bool print = (nz || watermark[portq]);
          if (!print) continue;
          bf_tm_eg_q_t *q_hdl;
          rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
          TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

          aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
          aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
          aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
          if (!hex) {
            aim_printf(&uc->pvs, "%-19u", watermark[portq]);
          } else {
            aim_printf(&uc->pvs, "%-19x", watermark[portq]);
          }
          aim_printf(&uc->pvs, "\n");
          aim_printf(&uc->pvs, "%9s", "");
        }
        aim_printf(&uc->pvs, "\n");
      }

      rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
    } while (BF_SUCCESS == rc);
    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(&uc->pvs, "---------------SUBDEV %d END-------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }
  }  // for
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_port_wm(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-20s", "PerPortWaterMark");
  aim_printf(&uc->pvs, "%-19s", "CellCount");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 39, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t ig_count = 0, eg_count = 0, ig_wm = 0, eg_wm = 0;
    rc = bf_tm_port_usage_get(
        devid, pipe, devport, &ig_count, &eg_count, &ig_wm, &eg_wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | eg_wm);

    if (print) {
      aim_printf(&uc->pvs, "%-20d", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", eg_wm);
      } else {
        aim_printf(&uc->pvs, "%-19x", eg_wm);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

// TM_PIPE_USAGE_TABLES

bf_status_t tm_ucli_display_ppg_cell_usage_gmin(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-16s", "PPGCellUsage");
  aim_printf(&uc->pvs, "%-19s", "GMinPpgCellUsage");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 42, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc |= bf_tm_ppg_usage_get(
        devid, pipe, ppg_hndl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | gmin_count);
    if (!print) continue;
    aim_printf(&uc->pvs, "%-16d", ppg);
    if (!hex) {
      aim_printf(&uc->pvs, "%-19u", gmin_count);
    } else {
      aim_printf(&uc->pvs, "%-19x", gmin_count);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_dpg_cell_usage_gmin(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-16s", "DPGCellUsage");
  aim_printf(&uc->pvs, "%-19s", "GMinPpgCellUsage");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 42, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  bf_tm_ppg_hdl dpg_hndl;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    rc = bf_tm_ppg_defaultppg_get(devid, devport, &dpg_hndl);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc = bf_tm_ppg_usage_get(
        devid, pipe, dpg_hndl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | gmin_count);
    if (print) {
      bf_tm_ppg_id_t dpg_nr;
      bf_tm_ppg_nr_get(devid, dpg_hndl, &dpg_nr);
      aim_printf(&uc->pvs, "%-16d", dpg_nr);
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", gmin_count);
      } else {
        aim_printf(&uc->pvs, "%-19x", gmin_count);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_ppg_cell_usage_shrd_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-16s", "PPGCellUsage");
  aim_printf(&uc->pvs, "%-19s", "SharedPoolPpgCellUsage");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 48, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc |= bf_tm_ppg_usage_get(
        devid, pipe, ppg_hndl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | shared_count);
    if (!print) continue;
    aim_printf(&uc->pvs, "%-16d", ppg);
    if (!hex) {
      aim_printf(&uc->pvs, "%-22u", shared_count);
    } else {
      aim_printf(&uc->pvs, "%-22x", shared_count);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_dpg_cell_usage_shrd_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-16s", "DPGCellUsage");
  aim_printf(&uc->pvs, "%-19s", "SharedPoolPpgCellUsage");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 48, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  bf_tm_ppg_hdl dpg_hndl;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    rc = bf_tm_ppg_defaultppg_get(devid, devport, &dpg_hndl);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc = bf_tm_ppg_usage_get(
        devid, pipe, dpg_hndl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | shared_count);
    if (print) {
      bf_tm_ppg_id_t dpg_nr;
      bf_tm_ppg_nr_get(devid, dpg_hndl, &dpg_nr);
      aim_printf(&uc->pvs, "%-16d", dpg_nr);
      if (!hex) {
        aim_printf(&uc->pvs, "%-22u", shared_count);
      } else {
        aim_printf(&uc->pvs, "%-22x", shared_count);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_ppg_cell_usage_skid_pool(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-16s", "PPGCellUsage");
  aim_printf(&uc->pvs, "%-19s", "SkidPpgCellUsage");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 42, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint32_t gmin_count = 0, shared_count = 0, skid_count = 0, wm = 0;
    rc |= bf_tm_ppg_usage_get(
        devid, pipe, ppg_hndl, &gmin_count, &shared_count, &skid_count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | skid_count);
    if (!print) continue;
    aim_printf(&uc->pvs, "%-16d", ppg);
    if (!hex) {
      aim_printf(&uc->pvs, "%-19u", skid_count);
    } else {
      aim_printf(&uc->pvs, "%-19x", skid_count);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_wac_port_usage_count(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-22s", "PerPortBufferUsage");
  aim_printf(&uc->pvs, "%-19s", "Buffer in Use ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 46, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t ig_count = 0, eg_count = 0, ig_wm = 0, eg_wm = 0;
    rc = bf_tm_port_usage_get(
        devid, pipe, devport, &ig_count, &eg_count, &ig_wm, &eg_wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | ig_count);

    if (print) {
      aim_printf(&uc->pvs, "%-22u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", ig_count);
      } else {
        aim_printf(&uc->pvs, "%-19x", ig_count);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_port_cellusage(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-20s", "PerPortCellUsage");
  aim_printf(&uc->pvs, "%-19s", "CellCount");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 39, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t ig_count = 0, eg_count = 0, ig_wm = 0, eg_wm = 0;
    rc = bf_tm_port_usage_get(
        devid, pipe, devport, &ig_count, &eg_count, &ig_wm, &eg_wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | eg_count);

    if (print) {
      aim_printf(&uc->pvs, "%-20d", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", eg_count);
      } else {
        aim_printf(&uc->pvs, "%-19x", eg_count);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_q_cellusage(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-19s", "Usage Cells");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 63, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  for (uint32_t die_id = 0; die_id < cfg.pipe_cnt / BF_SUBDEV_PIPE_COUNT;
       die_id++) {
    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(
          &uc->pvs, "---------------SUBDEV %d START------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }

    bf_dev_port_t devport = 0;
    rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    do {
      uint8_t q_count;
      uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
      rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      uint32_t counts[BF_TM_MAX_QUEUE_PER_PG];
      uint32_t print_port = 0;

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        uint32_t count = 0, wm = 0;
        if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
          rc = bf_tm_q_usage_get_ext(
              devid, die_id, pipe, devport, portq, &count, &wm);
        } else {
          rc = bf_tm_q_usage_get(devid, pipe, devport, portq, &count, &wm);
        }
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        counts[portq] = count;
        print_port |= count;
      }

      if (print_port) {
        aim_printf(&uc->pvs,
                   "%1d/%-7d",
                   DEV_PORT_TO_PIPE(devport),
                   DEV_PORT_TO_LOCAL_PORT(devport));
        for (uint8_t portq = 0; portq < q_count; ++portq) {
          const bool print = (nz || counts[portq]);
          if (!print) continue;
          bf_tm_eg_q_t *q_hdl;
          rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
          TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
          aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
          aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
          aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
          if (!hex) {
            aim_printf(&uc->pvs, "%-19u", counts[portq]);
          } else {
            aim_printf(&uc->pvs, "%-19x", counts[portq]);
          }
          aim_printf(&uc->pvs, "\n");
          aim_printf(&uc->pvs, "%9s", "");
        }
        aim_printf(&uc->pvs, "\n");
      }

      rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
    } while (BF_SUCCESS == rc);

    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(&uc->pvs, "---------------SUBDEV %d END-------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }
  }  // for

  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}
// TM_PIPE_DROPSTATUS_TABLES
bf_status_t tm_ucli_display_ppg_dropstate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-4s", "PPG");
  aim_printf(&uc->pvs, "%-32s", "Dropstate");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 31, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bool state;
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_drop_state_get(devid, ppg_hndl, &state);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-4u", ppg);
    TM_UCLI_PRINT_BOOLEAN(state, 32)
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_port_dropstate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-4s", "Port");
  aim_printf(&uc->pvs, "%-32s", "DropState Shared");
  aim_printf(&uc->pvs, "%-32s", "DropState Headroom");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 32, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
      bool state;
      rc = bf_tm_port_wac_drop_state_get(devid, devport, &state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      aim_printf(&uc->pvs, "%-4u", DEV_PORT_TO_LOCAL_PORT(devport));
      TM_UCLI_PRINT_BOOLEAN(state, 32)
      aim_printf(&uc->pvs, "\n");
    } else {
      bool shr_state, hdr_state;

      rc = bf_tm_port_wac_drop_state_get_ext(
          devid, devport, &shr_state, &hdr_state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      aim_printf(&uc->pvs, "%-4u", DEV_PORT_TO_LOCAL_PORT(devport));
      TM_UCLI_PRINT_BOOLEAN(shr_state, 32)
      TM_UCLI_PRINT_BOOLEAN(hdr_state, 32)
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_green_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-20s", "DropState ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 64, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    bool states[BF_TM_MAX_QUEUE_PER_PG];
    bool print_port = false;

    for (uint8_t portq = 0; portq < q_count; portq++) {
      bool state = false;
      rc = bf_tm_q_drop_state_get(
          devid, devport, portq, BF_TM_COLOR_GREEN, &state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      states[portq] = state;
      print_port |= state;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; portq++) {
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        TM_UCLI_PRINT_BOOLEAN(states[portq], 20)
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_yellow_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-20s", "DropState ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 64, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    bool states[BF_TM_MAX_QUEUE_PER_PG];
    bool print_port = false;

    for (uint8_t portq = 0; portq < q_count; portq++) {
      bool state;
      rc = bf_tm_q_drop_state_get(
          devid, devport, portq, BF_TM_COLOR_YELLOW, &state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      states[portq] = state;
      print_port |= state;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; portq++) {
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        TM_UCLI_PRINT_BOOLEAN(states[portq], 20)
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_red_drop(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-20s", "DropState ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 64, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    bool states[BF_TM_MAX_QUEUE_PER_PG];
    bool print_port = false;

    for (uint8_t portq = 0; portq < q_count; portq++) {
      bool state;
      rc = bf_tm_q_drop_state_get(
          devid, devport, portq, BF_TM_COLOR_RED, &state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      states[portq] = state;
      print_port |= state;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; portq++) {
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        TM_UCLI_PRINT_BOOLEAN(states[portq], 20)
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_port_drop_state(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-8s", "Port");
  aim_printf(&uc->pvs, "%-1s", "Dropstate ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 29, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    bool val = 0;
    rc = bf_tm_port_qac_drop_state_get(devid, devport, &val);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-8u", DEV_PORT_TO_LOCAL_PORT(devport));
    TM_UCLI_PRINT_BOOLEAN(val, 11)
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_wac_pfc_state(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-19s", "PerPortPFCState");
  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    aim_printf(&uc->pvs, "%-23s", "PFC State(8 iCoS mask) ");
  } else {
    aim_printf(&uc->pvs, "%-31s", "Port PPG State (8 iCoS mask) | ");
    aim_printf(&uc->pvs, "%-32s", "Remote PFC State(8 iCoS mask) | ");
    aim_printf(&uc->pvs, "%-33s", "MAC PFC Out State(8 iCoS mask) | ");
    aim_printf(&uc->pvs, "%-21s", "MAC PAUSE Out State |");
  }

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 136, HLINE);
  aim_printf(&uc->pvs, "\n");

  bool regular;
  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    regular = false;
  } else {
    regular = true;
  }

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, regular, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t port_ppg_state = 0;
    uint8_t rm_pfc_state = 0;
    uint8_t mac_pfc_out = 0;
    bool mac_pause_out = false;

    if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
      rc = bf_tm_port_pfc_state_get(devid, devport, &port_ppg_state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    } else {
      rc = bf_tm_port_pfc_state_get_ext(devid,
                                        devport,
                                        &port_ppg_state,
                                        &rm_pfc_state,
                                        &mac_pfc_out,
                                        &mac_pause_out);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    }

    const bool print = (nz | port_ppg_state);

    if (print) {
      aim_printf(&uc->pvs, "%-19u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-31u", port_ppg_state);
      } else {
        aim_printf(&uc->pvs, "%-31x", port_ppg_state);
      }
      if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
        aim_printf(&uc->pvs, "\n");
        rc = bf_tm_pipe_port_get_next(devid, devport, regular, &devport);
        continue;
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-32u", rm_pfc_state);
      } else {
        aim_printf(&uc->pvs, "%-32x", rm_pfc_state);
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-33u", mac_pfc_out);
      } else {
        aim_printf(&uc->pvs, "%-33x", mac_pfc_out);
      }
      TM_UCLI_PRINT_BOOLEAN(mac_pause_out, 21)
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, regular, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_egress_port_pfc_status(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-17s", "Physical-Port");
  aim_printf(&uc->pvs, "%-32s", "PfcStatus ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 37, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t status = 0;

    rc |= bf_tm_port_skid_limit_get(devid, devport, &status);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | status);

    if (print) {
      aim_printf(&uc->pvs, "%-17u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-32u", status);
      } else {
        aim_printf(&uc->pvs, "%-32x", status);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_egress_q_pfc_status(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-1s", "PfcStatus ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 46, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    bool statuses[BF_TM_MAX_QUEUE_PER_PG];
    bool print_port = false;

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      bool status = false;
      rc = bf_tm_sched_q_egress_pfc_status_get(devid, devport, portq, &status);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      statuses[portq] = status;
      print_port |= status;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        TM_UCLI_PRINT_BOOLEAN(statuses[portq], 10)
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

// TM_NONPIPE_DROPSTATUS_TABLES

bf_status_t tm_ucli_display_wac_color_drop_state(ucli_context_t *uc,
                                                 int nz,
                                                 int hex,
                                                 bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-43s", "Entry0: Green, Entry1: Yel, Entry2: Red");
  aim_printf(&uc->pvs, "%-32s", "DropMask(AP3..AP0)");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 71, HLINE);
  aim_printf(&uc->pvs, "\n");

  for (int e = 0; e < 3; e++) {
    uint32_t state = 0;
    rc |= bf_tm_pools_color_drop_state_get(devid, (bf_tm_color_t)e, &state);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | state);
    if (!print) continue;

    aim_printf(&uc->pvs, "%-43u", e);
    if (!hex) {
      aim_printf(&uc->pvs, "%-32u", state);
    } else {
      aim_printf(&uc->pvs, "%-32x", state);
    }
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_wac_skidpool_dropstate(ucli_context_t *uc,
                                                   int nz,
                                                   int hex,
                                                   bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-12s", "SkidPool");
  aim_printf(&uc->pvs, "%-32s", "Dropped");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 29, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc = bf_tm_pools_color_drop_state_get(devid, 3, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  const bool print = (nz | state);
  if (!print) return rc;

  aim_printf(&uc->pvs, "%-15s", "AP3..0");
  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  if (print) aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_wac_queue_shadow_state(ucli_context_t *uc,
                                                   int nz,
                                                   int hex,
                                                   bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  if (BF_TM_IS_TOF2(g_tm_ctx[devid]->asic_type) ||
      BF_TM_IS_TOF3(g_tm_ctx[devid]->asic_type)) {
    (void)nz;
    (void)hex;
    (void)devid;
    aim_printf(&uc->pvs, "This operation not supported currently\n");
    return BF_NOT_SUPPORTED;
  }

  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-16s", "Pipe0(G,Y,R,Min)");
  aim_printf(&uc->pvs, "%-16s", "Pipe1(G,Y,R,Min)");
  aim_printf(&uc->pvs, "%-16s", "Pipe2(G,Y,R,Min)");
  aim_printf(&uc->pvs, "%-16s", "Pipe3(G,Y,R,Min)");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 109, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, 0, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t states[BF_TM_MAX_QUEUE_PER_PG];
    uint32_t print_port = 0;

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      uint32_t state = 0;
      rc = bf_tm_q_drop_state_shadow_get(devid, devport, portq, &state);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      states[portq] = state;
      print_port |= state;
    }

    if (print_port) {
      aim_printf(&uc->pvs, "%1s/%-7d", "-", DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        const bool print = (nz || states[portq]);
        if (!print) continue;
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1s/%-11u", "-", q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        if (!hex) {
          aim_printf(&uc->pvs, "%-16u", states[portq] & 0xfu);
        } else {
          aim_printf(&uc->pvs, "%-16x", states[portq] & 0xfu);
        }

        if (!hex) {
          aim_printf(&uc->pvs, "%-16u", (states[portq] >> 4) & 0xfu);
        } else {
          aim_printf(&uc->pvs, "%-16x", (states[portq] >> 4) & 0xfu);
        }
        if (!hex) {
          aim_printf(&uc->pvs, "%-16u", (states[portq] >> 8) & 0xfu);
        } else {
          aim_printf(&uc->pvs, "%-16x", (states[portq] >> 8) & 0xfu);
        }
        if (!hex) {
          aim_printf(&uc->pvs, "%-16u", (states[portq] >> 12) & 0xfu);
        } else {
          aim_printf(&uc->pvs, "%-16x", (states[portq] >> 12) & 0xfu);
        }
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_dropstate(ucli_context_t *uc,
                                          int nz,
                                          int hex,
                                          bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-31s", "Per Pipe Egress Buffer Drop");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask for PIPE3..0 (8-bit for TOF3)");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 59, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc = bf_tm_pool_egress_buffer_drop_state_get(
      devid, PER_EG_PIPE_BUFF_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  const bool print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_qac_green_dropstate(ucli_context_t *uc,
                                                int nz,
                                                int hex,
                                                bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-47s", "Per Pool Egress Drop for green colored pkts");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask AP3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 75, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc = bf_tm_pool_egress_buffer_drop_state_get(
      devid, GLB_BUFF_AP_GREEN_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  const bool print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_qac_yel_dropstate(ucli_context_t *uc,
                                              int nz,
                                              int hex,
                                              bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-48s", "Per Pool Egress Drop for yellow colored pkts");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask AP3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 76, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc = bf_tm_pool_egress_buffer_drop_state_get(
      devid, GLB_BUFF_AP_YEL_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  const bool print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_qac_red_dropstate(ucli_context_t *uc,
                                              int nz,
                                              int hex,
                                              bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-45s", "Per Pool Egress Drop for red colored pkts");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask AP3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 73, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc = bf_tm_pool_egress_buffer_drop_state_get(
      devid, GLB_BUFF_AP_RED_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  const bool print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_qac_pipe_pre_fifo_dropstate(ucli_context_t *uc,
                                                        int nz,
                                                        int hex,
                                                        bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-30s", "Pipe 0 PRE FIFO drop state");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask for FIFO3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 62, HLINE);
  aim_printf(&uc->pvs, "\n");

  uint32_t state;
  rc |= bf_tm_pool_egress_buffer_drop_state_get(
      devid, PIPE0_PRE_FIFO_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bool print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%-30s", "Pipe 1 PRE FIFO drop state");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask for FIFO3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 62, HLINE);
  aim_printf(&uc->pvs, "\n");

  state = 0;
  rc |= bf_tm_pool_egress_buffer_drop_state_get(
      devid, PIPE1_PRE_FIFO_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%-30s", "Pipe 2 PRE FIFO drop state");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask for FIFO3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 62, HLINE);
  aim_printf(&uc->pvs, "\n");

  state = 0;
  rc |= bf_tm_pool_egress_buffer_drop_state_get(
      devid, PIPE2_PRE_FIFO_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%-30s", "Pipe 3 PRE FIFO drop state");
  aim_printf(&uc->pvs, "%-32s", "4-bit DropMask for FIFO3..0");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 62, HLINE);
  aim_printf(&uc->pvs, "\n");

  state = 0;
  rc |= bf_tm_pool_egress_buffer_drop_state_get(
      devid, PIPE3_PRE_FIFO_DROP_ST, &state);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  print = (nz | state);
  if (!print) return rc;

  if (!hex) {
    aim_printf(&uc->pvs, "%-32u", state);
  } else {
    aim_printf(&uc->pvs, "%-32x", state);
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

// TM_PERPIPE_ALLMODULE_COUNTERS
bf_status_t tm_ucli_display_perpipe_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(
      &uc->pvs, "%-49s", "perpipe-Counter                              ");
  aim_printf(&uc->pvs, "%-30s", "Count/State");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 87, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_blklvl_cntrs_t cntrs;
  rc = bf_tm_blklvl_drop_get(devid, pipe, &cntrs);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bool print = (nz | cntrs.valid_sop_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Valid SOP");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.valid_sop_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.valid_sop_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "wac_reg.ctr_vld_sop");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.wac_no_dest_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "No dest drop");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.wac_no_dest_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.wac_no_dest_drop);
    }
    aim_printf(&uc->pvs, "%-30s", "wac_reg.ctr_drop_no_dst");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.wac_buf_full_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Buffer Full");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.wac_buf_full_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.wac_buf_full_drop);
    }
    aim_printf(&uc->pvs, "%-30s", "wac_reg.wac_drop_buf_full");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.egress_pipe_total_drop);
  if (print) {
    aim_printf(
        &uc->pvs, "%-45s", "Packets dropped due to buffer full condition");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.egress_pipe_total_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.egress_pipe_total_drop);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_caa.epipe.pkt_dropcnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.psc_pkt_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Pkts dropped in egress pipe[PSC]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.psc_pkt_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.psc_pkt_drop);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_psc_top.psc_common.epipe.pkt_dropcnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.pex_total_disc);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Pex to Qlc discard Pkts");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.pex_total_disc);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.pex_total_disc);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.pex_dis_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qac_total_disc);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Discard Pkts");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qac_total_disc);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qac_total_disc);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.qac_dis_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_disc_dq);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Discard Dequeue Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_disc_dq);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_disc_dq);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.tot_dis_dq_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qac_no_dest_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total No dest drop in Qac");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qac_no_dest_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qac_no_dest_drop);
    }
    aim_printf(
        &uc->pvs, "%-30s", "tm_qac_top.qac_pipe.qac_reg.qac_ctr32_drop_no_dst");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qac_pre_mc_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total PRE Multicast drop in Qac");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qac_pre_mc_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qac_pre_mc_drop);
    }
    aim_printf(
        &uc->pvs, "%-30s", "tm_qac_top.qac_pipe.qac_reg.qac_ctr32_pre_mc_drop");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.pre_total_drop);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total packet dropped (PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.pre_total_drop);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.pre_total_drop);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.packet_drop");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.ph_lost_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Packet handles lost due to FIFO full (PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.ph_lost_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.ph_lost_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.ph_lost");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.cpu_copy_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total copies to cpu");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.cpu_copy_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.cpu_copy_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.cpu_copies");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_ph_processed);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total packet header processed (PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_ph_processed);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_ph_processed);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.ph_processed");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_copied_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total copies made (in PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_copied_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_copied_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.total_copies");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_xid_prunes_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total XID prunes (in PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_xid_prunes_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_xid_prunes_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.xid_prunes");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_yid_prunes_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total YID prunes (in PRE)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_yid_prunes_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_yid_prunes_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_pre_top.pre.yid_prunes");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.ph_in_use_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Current Packet Handles in use(PSC)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.ph_in_use_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.ph_in_use_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_psc_top.psc.psc_ph_used");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.clc_total_cell_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Cell Count [in Pipe]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.clc_total_cell_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.clc_total_cell_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.clc.tot_cell_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.clc_total_pkt_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "CLC Total Packet Count [in Pipe]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.clc_total_pkt_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.clc_total_pkt_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tot_pkt_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.eport_total_cell_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Eport Cell Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.eport_total_cell_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.eport_total_cell_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.pex.port_cell_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.eport_total_pkt_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Eport Pkt Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.eport_total_pkt_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.eport_total_pkt_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.pex.port_pkt_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.pex_total_pkt_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "PEX Total Packet Count [in Pipe]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.pex_total_pkt_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.pex_total_pkt_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "pex.tot_pkt_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_enq_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Enqueue Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_enq_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_enq_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.tot_eq_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.total_deq_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Dequeue Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.total_deq_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.total_deq_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.tot_dq_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.caa_used_blocks);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Current in use blocks [CAA]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.caa_used_blocks);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.caa_used_blocks);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_caa.epipe.blks_usecnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.psc_used_blocks);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Current in use blocks [PSC]");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.psc_used_blocks);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.psc_used_blocks);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_psc_top.psc_common.epipe.blks_usecnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qid_enq_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total selected qid enqueue count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qid_enq_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qid_enq_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.qid_eq_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qid_deq_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total selected qid dequeue count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qid_deq_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qid_deq_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_qlc_top.qlc.qid_deq_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qac_query_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total qac query (prc)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qac_query_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qac_query_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_prc_top.prc.qac_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.qac_query_zero_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total qac query zero (prc)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.qac_query_zero_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.qac_query_zero_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_prc_top.prc.qac_zero_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.prc_total_pex_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total qac query (prc)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.prc_total_pex_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.prc_total_pex_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_prc_top.prc.pex_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | cntrs.prc_total_pex_zero_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total qac query (prc)");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cntrs.prc_total_pex_zero_cntr);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cntrs.prc_total_pex_zero_cntr);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_prc_top.prc.pex_zero_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  uint64_t c_count = 0, p_count = 0;
  rc |= bf_tm_pipe_counters_get(devid, pipe, &c_count, &p_count);
  TM_UCLI_CHECK_ERROR(rc)
  print = (nz | cntrs.clc_total_cell_cntr);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Inport Cell Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, c_count);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, c_count);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.clc.inport_cell_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | p_count);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total Inport Packet Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, p_count);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, p_count);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.clc.inport_pkt_cnt");
    aim_printf(&uc->pvs, "\n");
  }
  // UC
  c_count = 0;
  // MC
  p_count = 0;
  rc |= bf_tm_cut_through_counters_get(devid, pipe, &c_count, &p_count);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  print = (nz | c_count);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total UCast Cut Through Packet Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, c_count);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, c_count);
    }
    aim_printf(&uc->pvs, "%-30s", "tm_clc_top.clc.uc_ct_cnt");
    aim_printf(&uc->pvs, "\n");
  }

  print = (nz | p_count);
  if (print) {
    aim_printf(&uc->pvs, "%-45s", "Total MCcast Cut Through Packet Count");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, p_count);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, p_count);
    }
    aim_printf(&uc->pvs, "%-30s", "mc_ct_cnt");
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_wac_per_port_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-19s", "Port           ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 51, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint64_t cnt = 0;
    rc = bf_tm_port_ingress_drop_get(devid, pipe, devport, &cnt);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | cnt);
    if (print) {
      aim_printf(&uc->pvs, "%-15u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
      } else {
        aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
      }
      aim_printf(&uc->pvs, "%-30s", "wac_reg.wac_drop_count_port");
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_wac_per_ppg_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-19s", "PPG            ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 51, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    uint64_t cnt = 0;
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_drop_get(devid, pipe, ppg_hndl, &cnt);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | cnt);
    if (print) {
      aim_printf(&uc->pvs, "%-15u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
      } else {
        aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
      }
      aim_printf(&uc->pvs, "%-30s", "wac_reg.wac_drop_count_ppg");
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_qac_queue_drop_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-20s", "Drop count ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 66, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  for (uint32_t die_id = 0; die_id < cfg.pipe_cnt / BF_SUBDEV_PIPE_COUNT;
       die_id++) {
    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(
          &uc->pvs, "---------------SUBDEV %d START------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }
    bf_dev_port_t devport = 0;
    rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    do {
      uint8_t q_count;
      uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
      rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      uint64_t counts[BF_TM_MAX_QUEUE_PER_PG];
      uint64_t print_port = 0;

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        uint64_t cnt = 0;
        if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
          rc = bf_tm_q_drop_get_ext(devid, die_id, pipe, devport, portq, &cnt);
        } else {
          rc = bf_tm_q_drop_get(devid, pipe, devport, portq, &cnt);
        }
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        counts[portq] = cnt;
        print_port |= cnt;
      }

      if (print_port) {
        aim_printf(&uc->pvs,
                   "%1d/%-7d",
                   DEV_PORT_TO_PIPE(devport),
                   DEV_PORT_TO_LOCAL_PORT(devport));

        for (uint8_t portq = 0; portq < q_count; ++portq) {
          const bool print = (nz || counts[portq]);
          if (print) {
            bf_tm_eg_q_t *q_hdl;
            rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
            TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
            aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
            aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
            aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
            if (!hex) {
              aim_printf(&uc->pvs, "%-40" PRIu64, counts[portq]);
            } else {
              aim_printf(&uc->pvs, "%-40" PRIx64, counts[portq]);
            }
            aim_printf(&uc->pvs, "\n");
            aim_printf(&uc->pvs, "%9s", "");
          }
        }
        aim_printf(&uc->pvs, "\n");
      }

      rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
    } while (BF_SUCCESS == rc);

    if (cfg.pipe_cnt > BF_SUBDEV_PIPE_COUNT) {
      aim_printf(&uc->pvs, "---------------SUBDEV %d END-------------", die_id);
      aim_printf(&uc->pvs, "\n");
    }
  }  // for

  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_stop_stats_cache_timer(ucli_context_t *uc,
                                           int nz,
                                           int hex,
                                           bf_dev_id_t devid) {
  (void)nz;
  (void)hex;
  bf_status_t rc;
  aim_printf(&uc->pvs, "%.*s", 87, HLINE);
  aim_printf(&uc->pvs, "\n");

  rc = bf_tm_stop_cache_counters_timer(devid);
  if (rc != BF_SUCCESS) {
    aim_printf(&uc->pvs, "%-20s", "TM cache counters stop timer FALIED");
  } else {
    aim_printf(&uc->pvs, "%-20s", "TM cache counters stop timer SUCCESS");
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_start_stats_cache_timer(ucli_context_t *uc,
                                            int nz,
                                            int hex,
                                            bf_dev_id_t devid) {
  (void)nz;
  (void)hex;
  bf_status_t rc;
  aim_printf(&uc->pvs, "%.*s", 87, HLINE);
  aim_printf(&uc->pvs, "\n");

  rc = bf_tm_start_cache_counters_timer(devid);
  if (rc != BF_SUCCESS) {
    aim_printf(&uc->pvs, "%-20s", "TM cache counters start timer FALIED");
  } else {
    aim_printf(&uc->pvs, "%-20s", "TM cache counters start timer SUCCESS");
  }
  aim_printf(&uc->pvs, "\n");
  return rc;
}

bf_status_t tm_ucli_display_qac_port_drop_color_counter(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)pipe;
  aim_printf(&uc->pvs, "%-10s", "Port 0-71");
  aim_printf(&uc->pvs, "%-20s", "Green dropped");
  aim_printf(&uc->pvs, "%-20s", "Yellow dropped");
  aim_printf(&uc->pvs, "%-20s", "Red dropped");
  aim_printf(&uc->pvs, "%-30s", "Total");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 92, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint64_t total = 0, green = 0, yellow = 0, red = 0;

    rc = bf_tm_port_egress_color_drop_get(
        devid, devport, BF_TM_COLOR_GREEN, &green);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    rc = bf_tm_port_egress_color_drop_get(
        devid, devport, BF_TM_COLOR_YELLOW, &yellow);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    rc =
        bf_tm_port_egress_color_drop_get(devid, devport, BF_TM_COLOR_RED, &red);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    total = green + yellow + red;

    const bool print = (nz | total);
    if (print) {
      aim_printf(&uc->pvs, "%-10u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-20" PRIu64, green);
        aim_printf(&uc->pvs, "%-20" PRIu64, yellow);
        aim_printf(&uc->pvs, "%-20" PRIu64, red);
        aim_printf(&uc->pvs, "%-30" PRIu64, total);
      } else {
        aim_printf(&uc->pvs, "%-20" PRIx64, green);
        aim_printf(&uc->pvs, "%-20" PRIx64, yellow);
        aim_printf(&uc->pvs, "%-20" PRIx64, red);
        aim_printf(&uc->pvs, "%-30" PRIx64, total);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_nonpipe_counter(ucli_context_t *uc,
                                            int nz,
                                            int hex,
                                            bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(
      &uc->pvs, "%-49s", "nonpipe-Counter                              ");
  aim_printf(&uc->pvs, "%-30s", "Count/State");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 87, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_pre_fifo_cntrs_t prefifo_cntrs;
  rc = bf_tm_pre_fifo_drop_get(devid, &prefifo_cntrs);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_fifo = cfg.pre_fifo_per_pipe;
  bf_dev_pipe_t pipe = 0;
  rc = bf_tm_pipe_get_first(devid, &pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    for (uint32_t f = 0; f < max_fifo; ++f) {
      uint64_t cnt = 0;
      switch (pipe) {
        case 0:
          cnt = prefifo_cntrs.wac_drop_cnt_pre0_fifo[f];
          break;
        case 1:
          cnt = prefifo_cntrs.wac_drop_cnt_pre1_fifo[f];
          break;
        case 2:
          cnt = prefifo_cntrs.wac_drop_cnt_pre2_fifo[f];
          break;
        case 3:
          cnt = prefifo_cntrs.wac_drop_cnt_pre3_fifo[f];
          break;
        default:
          break;
      }
      const bool print = (nz | cnt);
      if (print) {
        aim_printf(&uc->pvs,
                   "Pipe%d PRE FIFO[%d] drop count                     ",
                   pipe,
                   f);
        if (!hex) {
          aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
        } else {
          aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
        }
        aim_printf(&uc->pvs, "%-30s", "qac_ctr48_drop_pre_fifo");
        aim_printf(&uc->pvs, "\n");
      }
    }
    rc = bf_tm_pipe_get_next(devid, pipe, &pipe);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

// CLR WM
bf_status_t tm_ucli_reset_wac_port_watermark(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_ingress_watermark_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_wac_ppg_watermark(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_watermark_clear(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_qac_port_wm(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_egress_watermark_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_qac_q_wm(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_watermark_clear(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_clear_qac_port_drop_counter(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_egress_drop_count_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_clear_qac_q_drop_counter(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_drop_count_clear(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_clear_wac_per_ppg_drop_counter(bf_dev_id_t devid,
                                                   int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_drop_count_clear(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_clear_wac_per_port_drop_counter(bf_dev_id_t devid,
                                                    int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_ingress_drop_count_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_clear_nonpipe_counter(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  uint32_t max_fifo = cfg.pre_fifo_per_pipe;

  bf_dev_pipe_t pipe = 0;
  rc = bf_tm_pipe_get_first(devid, &pipe);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    for (uint32_t fifo = 0; fifo < max_fifo; ++fifo) {
      rc |= bf_tm_pre_fifo_drop_clear(devid, pipe, fifo);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_get_next(devid, pipe, &pipe);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_clear_perpipe_counter(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t clr_mask = COUNTERS_ALL;
  rc = bf_tm_blklvl_drop_clear(devid, pipe, clr_mask);
  return rc;
}

// TM_PIPE_CFG_TABLES

bf_status_t tm_ucli_display_ppg_gmin_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-19s", "GMinLimit ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 26, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  for (uint32_t ppg = 0; ppg < cfg.total_ppg_per_pipe; ppg++) {
    uint32_t limit = 0;
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_guaranteed_min_limit_get(devid, ppg_hndl, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      aim_printf(&uc->pvs, "%-7u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", limit);
      } else {
        aim_printf(&uc->pvs, "%-19x", limit);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  return rc;
}

bf_status_t tm_ucli_display_ppg_hdr_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-19s", "HeadRoomLimit ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 31, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  for (uint32_t ppg = 0; ppg < cfg.pfc_ppg_per_pipe; ppg++) {
    uint32_t limit = 0;
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_skid_limit_get(devid, ppg_hndl, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      aim_printf(&uc->pvs, "%-7u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", limit);
      } else {
        aim_printf(&uc->pvs, "%-19x", limit);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_port_ppg(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "%-27s", "Port (per pfc lvl [0..7]):");
  aim_printf(&uc->pvs, "%-8s", "ppg ");
  aim_printf(&uc->pvs, "%-12s", "app-pool ");
  aim_printf(&uc->pvs, "%-12s", "Non-default ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 89, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t ppg_mask = 0;
    bf_tm_ppg_hdl ppg_hndlrs[BF_TM_MAX_PFC_LEVELS];
    rc = bf_tm_port_ppg_map_get(devid, devport, &ppg_mask, ppg_hndlrs);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-27u", DEV_PORT_TO_LOCAL_PORT(devport));

    for (uint16_t cos = 0; cos < BF_TM_MAX_PFC_LEVELS; cos++) {
      uint32_t ppg_id = 0, pool_id = 0;
      bool default_ppg = false;
      rc = bf_tm_ppg_allocation_cfg_get(
          devid, ppg_hndlrs[cos], &ppg_id, &pool_id, &default_ppg);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      if (!hex) {
        aim_printf(&uc->pvs, "%-8u", ppg_id);
      } else {
        aim_printf(&uc->pvs, "%-8x", ppg_id);
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-12u", pool_id);
      } else {
        aim_printf(&uc->pvs, "%-12x", pool_id);
      }
      TM_UCLI_PRINT_BOOLEAN(!default_ppg, 12)
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%-27s", "");
    }

    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_ppg_shared_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-10s", "Limit ");
  aim_printf(&uc->pvs, "%-5s", "Offset ");
  aim_printf(&uc->pvs, "%-1s", "Dynamic ");
  aim_printf(&uc->pvs, "%-9s", "BAF ");
  aim_printf(&uc->pvs, "%-1s", "FastRecovery ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 62, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;

  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    bf_tm_app_pool_t pool = 0;
    uint32_t limit = 0;
    bf_tm_ppg_baf_t dynamic_baf;
    uint32_t hysteresis;
    bool fast_recovery;
    rc |= bf_tm_ppg_app_pool_usage_get(
        devid, ppg_hndl, pool, &limit, &dynamic_baf, &hysteresis);
    rc |= bf_tm_ppg_fast_recovery_mode_get(devid, ppg_hndl, &fast_recovery);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-7u", ppg);
    if (dynamic_baf != BF_TM_PPG_BAF_DISABLE) {
      aim_printf(&uc->pvs, "%-10s", "N/A");
    } else {
      if (!hex) {
        aim_printf(&uc->pvs, "%-10u", limit);
      } else {
        aim_printf(&uc->pvs, "%-10x", limit);
      }
    }
    if (!hex) {
      aim_printf(&uc->pvs, "%-7u", hysteresis);
    } else {
      aim_printf(&uc->pvs, "%-7x", hysteresis);
    }
    TM_UCLI_PRINT_BOOLEAN(dynamic_baf != BF_TM_PPG_BAF_DISABLE, 8)
    if (dynamic_baf != BF_TM_PPG_BAF_DISABLE) {
      aim_printf(&uc->pvs,
                 "%-4.1f%-5c",
                 bf_tm_get_ig_ppg_baf_percentage(dynamic_baf),
                 '%');
    } else {
      aim_printf(&uc->pvs, "%-9s", "N/A");
    }
    TM_UCLI_PRINT_BOOLEAN(fast_recovery, 12)
    aim_printf(&uc->pvs, "\n");
  }
  return rc;
}

bf_status_t tm_ucli_display_ppg_icos_mapping(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-8s", "icos-mask ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 27, HLINE);
  aim_printf(&uc->pvs, "\n");
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint8_t icos_mask;
    rc = bf_tm_ppg_icos_mapping_get(devid, ppg_hndl, &icos_mask);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | icos_mask);

    if (print) {
      aim_printf(&uc->pvs, "%-7u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-10u", icos_mask);
      } else {
        aim_printf(&uc->pvs, "%-10x", icos_mask);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_wac_offset_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-17s", "OffsetProfile");
  aim_printf(&uc->pvs, "%-10s", "Limit(Cells) ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 46, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    uint32_t hysteresis = 0;
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc = bf_tm_ppg_guaranteed_min_skid_hysteresis_get(
        devid, ppg_hndl, &hysteresis);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    const bool print = (nz | hysteresis);
    if (print) {
      aim_printf(&uc->pvs, "%-17u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-19u", hysteresis);
      } else {
        aim_printf(&uc->pvs, "%-19x", hysteresis);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_q_min_thrd(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-19s", "MinLimit ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 63, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t limits[BF_TM_MAX_QUEUE_PER_PG];
    uint32_t print_port = 0;

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      uint32_t limit = 0;
      rc = bf_tm_q_guaranteed_min_limit_get(devid, devport, portq, &limit);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      limits[portq] = limit;
      print_port |= limit;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        const bool print = (nz || limits[portq]);
        if (!print) continue;
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        if (!hex) {
          aim_printf(&uc->pvs, "%-19u", limits[portq]);
        } else {
          aim_printf(&uc->pvs, "%-19x", limits[portq]);
        }
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_shr_thrd(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-19s", "Limit ");
  aim_printf(&uc->pvs, "%-8s", "Offset ");
  aim_printf(&uc->pvs, "%-8s", "Dynamic ");
  aim_printf(&uc->pvs, "%-8s", "BAF");
  aim_printf(&uc->pvs, "%-8s", "FastRecover ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 99, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%1d/%-7d",
               DEV_PORT_TO_PIPE(devport),
               DEV_PORT_TO_LOCAL_PORT(devport));

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      uint32_t limit = 0;
      bf_tm_app_pool_t pool = 0;
      bf_tm_queue_baf_t dynamic_baf = 0;
      uint32_t hysteresis = 0;
      rc = bf_tm_q_app_pool_usage_get(
          devid, devport, portq, &pool, &limit, &dynamic_baf, &hysteresis);
      bool fast_recovery = false;
      rc =
          bf_tm_q_fast_recovery_mode_get(devid, devport, portq, &fast_recovery);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      bf_tm_eg_q_t *q_hdl;
      rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%3u/%-8u", devport, portq);
      aim_printf(&uc->pvs, "%1u/%-10u", q_hdl->p_pipe, q_hdl->physical_q);
      aim_printf(&uc->pvs, "%2u/%-9u", q_hdl->pg, q_hdl->logical_q);
      if (dynamic_baf != BF_TM_Q_BAF_DISABLE) {
        aim_printf(&uc->pvs, "%-19s", "N/A");
      } else {
        if (!hex) {
          aim_printf(&uc->pvs, "%-19u", limit);
        } else {
          aim_printf(&uc->pvs, "%-19x", limit);
        }
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-8u", hysteresis);
      } else {
        aim_printf(&uc->pvs, "%-8x", hysteresis);
      }
      if (dynamic_baf != BF_TM_Q_BAF_DISABLE) {
        aim_printf(&uc->pvs, "%-8s", "");
        aim_printf(&uc->pvs,
                   "%-4.1f%-4c",
                   bf_tm_get_eg_q_baf_percentage(dynamic_baf),
                   '%');
      } else {
        aim_printf(&uc->pvs, "%-16s", "Disabled");
      }
      TM_UCLI_PRINT_BOOLEAN(fast_recovery, 8)
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%9s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_ap(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-2s", "SharedPool ");
  aim_printf(&uc->pvs, "%-1s", "Q-Color-Drop-Enable ");
  aim_printf(&uc->pvs, "%-1s", "Q-Tail-Drop-Enable ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 94, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%1d/%-7d",
               DEV_PORT_TO_PIPE(devport),
               DEV_PORT_TO_LOCAL_PORT(devport));

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      bf_tm_app_pool_t pool = 0;
      rc = bf_tm_q_app_poolid_get(devid, devport, portq, &pool);
      bool color_drop_en;
      rc = bf_tm_q_color_drop_get(devid, devport, portq, &color_drop_en);
      bool tail_drop_en;
      rc = bf_tm_q_tail_drop_get(devid, devport, portq, &tail_drop_en);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      bf_tm_eg_q_t *q_hdl;
      rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
      aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
      aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
      if (!hex) {
        aim_printf(&uc->pvs, "%-11u", pool);
      } else {
        aim_printf(&uc->pvs, "%-11x", pool);
      }
      TM_UCLI_PRINT_BOOLEAN(color_drop_en, 20)
      TM_UCLI_PRINT_BOOLEAN(tail_drop_en, 14)
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%9s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_min_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-3s", "pps/bps ");
  aim_printf(&uc->pvs, "%-7s", "Burst size ");
  aim_printf(&uc->pvs, "%-7s", "Min rate ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 96, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%1d/%-7d",
               DEV_PORT_TO_PIPE(devport),
               DEV_PORT_TO_LOCAL_PORT(devport));

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      bool is_pps;
      uint32_t burst_size;
      uint32_t min_rate;
      rc = bf_tm_sched_q_guaranteed_rate_get(
          devid, devport, portq, &is_pps, &burst_size, &min_rate);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      bf_tm_eg_q_t *q_hdl;
      rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
      aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
      aim_printf(&uc->pvs, "%2u/%-7u", q_hdl->pg, q_hdl->logical_q);
      TM_UCLI_PRINT_BOOLEAN(is_pps, 8)
      if (!hex) {
        aim_printf(&uc->pvs, "%-11u", burst_size);
      } else {
        aim_printf(&uc->pvs, "%-11x", burst_size);
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-7u", min_rate);
      } else {
        aim_printf(&uc->pvs, "%-7x", min_rate);
      }

      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%9s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_max_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-3s", "pps/bps ");
  aim_printf(&uc->pvs, "%-7s", "Burst size ");
  aim_printf(&uc->pvs, "%-7s", "Max rate ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 96, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%1d/%-7d",
               DEV_PORT_TO_PIPE(devport),
               DEV_PORT_TO_LOCAL_PORT(devport));
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      bool is_pps;
      uint32_t burst_size;
      uint32_t max_rate;
      rc = bf_tm_sched_q_shaping_rate_get(
          devid, devport, portq, &is_pps, &burst_size, &max_rate);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      bf_tm_eg_q_t *q_hdl;
      rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
      aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
      aim_printf(&uc->pvs, "%2u/%-7u", q_hdl->pg, q_hdl->logical_q);
      TM_UCLI_PRINT_BOOLEAN(is_pps, 8)
      if (!hex) {
        aim_printf(&uc->pvs, "%-11u", burst_size);
      } else {
        aim_printf(&uc->pvs, "%-11x", burst_size);
      }
      if (!hex) {
        aim_printf(&uc->pvs, "%-11u", max_rate);
      } else {
        aim_printf(&uc->pvs, "%-11x", max_rate);
      }

      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%9s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_port_max_rate(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "%-7s", "Port");
  aim_printf(&uc->pvs, "%-1s", "pps/bps ");
  aim_printf(&uc->pvs, "%-4s", "Burst size ");
  aim_printf(&uc->pvs, "%-4s", "Max rate ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 96, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    bool is_pps;
    uint32_t burst_size;
    uint32_t min_rate;
    rc = bf_tm_sched_port_shaping_rate_get(
        devid, devport, &is_pps, &burst_size, &min_rate);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs, "%-7u", DEV_PORT_TO_LOCAL_PORT(devport));
    TM_UCLI_PRINT_BOOLEAN(is_pps, 8)
    if (!hex) {
      aim_printf(&uc->pvs, "%-11u", burst_size);
    } else {
      aim_printf(&uc->pvs, "%-11x", burst_size);
    }
    if (!hex) {
      aim_printf(&uc->pvs, "%-11u", min_rate);
    } else {
      aim_printf(&uc->pvs, "%-11x", min_rate);
    }

    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_ing_port_drop_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-8s", "PORT");
  aim_printf(&uc->pvs, "%-30s", "Ingress Port Limit (in cells)");
  aim_printf(&uc->pvs, "%-30s", "Ingress Hysteresis (in cells)");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 67, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t limit = 0;
    rc = bf_tm_port_ingress_drop_limit_get(devid, devport, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t hysteresis = 0;
    rc = bf_tm_port_ingress_hysteresis_get(devid, devport, &hysteresis);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      aim_printf(&uc->pvs, "%-8u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        aim_printf(&uc->pvs, "%-30u", limit);
      } else {
        aim_printf(&uc->pvs, "%-30x", limit);
      }
      if (BF_NOT_SUPPORTED == rc) {
        aim_printf(&uc->pvs, "N/A");
      } else if (!hex) {
        aim_printf(&uc->pvs, "%-29u", hysteresis);
      } else {
        aim_printf(&uc->pvs, "%-29x", hysteresis);
      }
      aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);

  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_port_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-8s", "Port");
  aim_printf(&uc->pvs, "%-19s", "PortLImit ");
  aim_printf(&uc->pvs, "%-5s", "OffsetIndex ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 44, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t limit = 0;
    rc = bf_tm_port_egress_drop_limit_get(devid, devport, &limit);
    uint32_t hysteresis = 0;
    rc = bf_tm_port_egress_hysteresis_get(devid, devport, &hysteresis);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      if (print) aim_printf(&uc->pvs, "%-8u", DEV_PORT_TO_LOCAL_PORT(devport));
      if (!hex) {
        if (print) aim_printf(&uc->pvs, "%-19u", limit);
      } else {
        if (print) aim_printf(&uc->pvs, "%-19x", limit);
      }
      if (!hex) {
        if (print) aim_printf(&uc->pvs, "%-12u", hysteresis);
      } else {
        if (print) aim_printf(&uc->pvs, "%-12x", hysteresis);
      }
      if (print) aim_printf(&uc->pvs, "\n");
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);

  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_qid_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "%-8s", "Port");
  aim_printf(&uc->pvs, "%-4s", "ProfileIndex ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 31, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint32_t index = 0;
    bf_dev_pipe_t p_pipe;
    rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
    rc = bf_tm_q_get_port_q_profile(devid, devport, p_pipe, &index);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-8u", DEV_PORT_TO_LOCAL_PORT(devport));
    if (!hex) {
      aim_printf(&uc->pvs, "%-13u", index);
    } else {
      aim_printf(&uc->pvs, "%-13x", index);
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_qac_offset_profile(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-10s", "Limit(8 Cell Unit) ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 64, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    uint32_t hysteresises[BF_TM_MAX_QUEUE_PER_PG];
    uint32_t print_port = 0;

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      uint32_t hysteresis = 0;
      rc |= bf_tm_q_hysteresis_get(devid, devport, portq, &hysteresis);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      hysteresises[portq] = hysteresis;
      print_port |= hysteresis;
    }

    if (print_port) {
      aim_printf(&uc->pvs,
                 "%1d/%-7d",
                 DEV_PORT_TO_PIPE(devport),
                 DEV_PORT_TO_LOCAL_PORT(devport));

      for (uint8_t portq = 0; portq < q_count; ++portq) {
        const bool print = (nz || hysteresises[portq]);
        if (!print) continue;
        bf_tm_eg_q_t *q_hdl;
        rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
        TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
        aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
        aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
        aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);
        if (!hex) {
          aim_printf(&uc->pvs, "%-10u", hysteresises[portq]);
        } else {
          aim_printf(&uc->pvs, "%-10x", hysteresises[portq]);
        }
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%9s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

// it doesn`t make sence make this platform independent
bf_status_t tm_ucli_display_qac_qid_map(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    aim_printf(&uc->pvs, "%-10s", "Port");
    aim_printf(&uc->pvs, "%-10s", "qid#");
    aim_printf(&uc->pvs, "%-10s", "Local-Qid");
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%.*s", 145, HLINE);
    aim_printf(&uc->pvs, "\n");

    uint32_t max_port = cfg.ports_per_pg * cfg.pg_per_pipe;
    for (uint32_t port = 0; port < max_port; port++) {
      aim_printf(&uc->pvs, "Port %-5d", port);

      uint8_t data[32] = {0};  // queues per PG

      rc |= bf_tm_tofino_get_qac_qid_mapping(devid, pipe, port, data);

      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      for (uint32_t qid = 0; qid < 32; ++qid) {
        aim_printf(&uc->pvs, "%-10u", qid);
        if (!hex) {
          aim_printf(&uc->pvs, "%-4u", data[qid]);
        } else {
          aim_printf(&uc->pvs, "%-4x", data[qid]);
        }
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "%-10s", "");
      }
      aim_printf(&uc->pvs, "\n");
    }
  } else if (BF_TM_IS_TOF2(g_tm_ctx[devid]->asic_type)) {
    uint32_t max_port = cfg.ports_per_pg * cfg.pg_per_pipe;
    for (uint32_t port = 0; port < max_port; port++) {
      aim_printf(&uc->pvs, "Port %-5d", port);

      uint8_t data[BF_TM_TOF2_QGRP_ID_PER_PORT] = {0};
      rc |= bf_tm_tof2_get_qac_qid_map(devid, pipe, port, data);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%-10s", "qid:");
      for (int i = 0; i < BF_TM_TOF2_QGRP_ID_PER_PORT; i++) {
        if (!hex) {
          aim_printf(&uc->pvs, "%-4u", data[i]);
        } else {
          aim_printf(&uc->pvs, "%-4x", data[i]);
        }
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%.*s", 117, HLINE);
      aim_printf(&uc->pvs, "\n");
    }
  }

  return rc;
}
// it doesn`t make sence make this platform independent
bf_status_t tm_ucli_display_wac_qid_map(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  if (BF_TM_IS_TOF2(g_tm_ctx[devid]->asic_type)) {
    uint32_t max_port = cfg.pg_per_pipe * cfg.ports_per_pg;
    for (uint32_t port = 0; port < max_port; port++) {
      aim_printf(&uc->pvs, "Port %-13d", port);
      uint8_t data[BF_TM_TOF2_QGRP_ID_PER_PORT] = {0};
      rc |= bf_tm_tof2_get_wac_qid_map(devid, pipe, port, data);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%-13s", "qid:");
      for (int i = 0; i < BF_TM_TOF2_QGRP_ID_PER_PORT; i++) {
        if (!hex) {
          aim_printf(&uc->pvs, "%-4u", data[i]);
        } else {
          aim_printf(&uc->pvs, "%-4x", data[i]);
        }
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%.*s", 117, HLINE);
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}
// it doesn`t make sence make this platform independent
bf_status_t tm_ucli_display_wac_eg_qid_map(ucli_context_t *uc,
                                           int nz,
                                           int hex,
                                           bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    aim_printf(
        &uc->pvs, "%-53s", "Index Derived as [4bProfile#][5bIngressQid[0..31]");
    aim_printf(&uc->pvs,
               "%-5s",
               "Local-Qid (Phys-Q = GetLocalQid([4bPort's "
               "Profile#][IngressQ0..31]) + (32 * PG#)) ");
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%.*s", 145, HLINE);
    aim_printf(&uc->pvs, "\n");

    for (uint32_t e = 0; e < 512; e++) {
      uint32_t val = 0;
      rc |= bf_tm_tofino_get_wac_eg_qid_mapping(devid, e, &val);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%-53u", e);
      if (!hex) {
        aim_printf(&uc->pvs, "%-82u", val);
      } else {
        aim_printf(&uc->pvs, "%-82x", val);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  return rc;
}

bf_status_t tm_ucli_display_port_q_mapping(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "%-7s", "Port");
  aim_printf(&uc->pvs, "%-7s", "QID");
  aim_printf(&uc->pvs, "%-10s", "Physical Q");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 145, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    aim_printf(&uc->pvs, "%-7u", DEV_PORT_TO_LOCAL_PORT(devport));
    for (uint8_t qid = 0; qid < cfg.q_per_pg; ++qid) {
      uint32_t phys_q = 0;
      bf_dev_pipe_t l_pipe = 0;
      rc = bf_tm_port_pipe_physical_queue_get(
          devid, devport, qid, &l_pipe, &phys_q);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%-7u", qid);
      if (!hex) {
        aim_printf(&uc->pvs, "%-10u", phys_q);
      } else {
        aim_printf(&uc->pvs, "%-10x", phys_q);
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%-7s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_q_color_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  aim_printf(&uc->pvs, "LP/Port - Logical Pipe / Local Port\n");
  aim_printf(&uc->pvs,
             "D_P/Q    - Queue mapping: Device Port (dev_port) / Port queue "
             "number (queue_nr)\n");
  aim_printf(&uc->pvs, "P/PhQ    - QSTAT: Physical Pipe / Physical Queue\n");
  aim_printf(&uc->pvs,
             "PgN/PgQ  - Port Group (pg_id) / Port Group Queue (pg_queue)\n\n");

  aim_printf(&uc->pvs, "%-9s", "LP/Port");
  aim_printf(&uc->pvs, "%-12s", "D_P/Q");
  aim_printf(&uc->pvs, "%-12s", "P/PhQ");
  aim_printf(&uc->pvs, "%-12s", "PgN/PgQ");
  aim_printf(&uc->pvs, "%-3s", "YelLimit ");
  aim_printf(&uc->pvs, "%-5s", "YelLimitHysteresis ");
  aim_printf(&uc->pvs, "%-3s", "RedLimit ");
  aim_printf(&uc->pvs, "%-5s", "RedLimitHysteresis ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 100, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_dev_pipe_t p_pipe = 0;
  rc = lld_sku_map_pipe_id_to_phy_pipe_id(devid, pipe, &p_pipe);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc |= bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%1d/%-7d",
               DEV_PORT_TO_PIPE(devport),
               DEV_PORT_TO_LOCAL_PORT(devport));

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      bf_tm_queue_color_limit_t y_limit, r_limit;
      rc |= bf_tm_q_color_limit_get(
          devid, devport, portq, BF_TM_COLOR_YELLOW, &y_limit);
      rc |= bf_tm_q_color_limit_get(
          devid, devport, portq, BF_TM_COLOR_RED, &r_limit);
      uint32_t y_hysteresis = 0, r_hysteresis = 0;
      rc |= bf_tm_q_color_hysteresis_get(
          devid, devport, portq, BF_TM_COLOR_YELLOW, &y_hysteresis);
      rc |= bf_tm_q_color_hysteresis_get(
          devid, devport, portq, BF_TM_COLOR_RED, &r_hysteresis);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      bf_tm_eg_q_t *q_hdl;
      rc = bf_tm_q_get_descriptor(devid, devport, portq, &q_hdl);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%3u/%-9u", devport, portq);
      aim_printf(&uc->pvs, "%1u/%-11u", q_hdl->p_pipe, q_hdl->physical_q);
      aim_printf(&uc->pvs, "%2u/%-10u", q_hdl->pg, q_hdl->logical_q);

      aim_printf(&uc->pvs,
                 "%-4.1f%-5c",
                 bf_tm_get_q_color_limit_percentage(y_limit),
                 '%');

      if (!hex) {
        aim_printf(&uc->pvs, "%-16u", y_hysteresis);
      } else {
        aim_printf(&uc->pvs, "%-16x", y_hysteresis);
      }
      aim_printf(&uc->pvs,
                 "%-4.1f%-5c",
                 bf_tm_get_q_color_limit_percentage(r_limit),
                 '%');
      if (!hex) {
        aim_printf(&uc->pvs, "%-15u", r_hysteresis);
      } else {
        aim_printf(&uc->pvs, "%-15x", r_hysteresis);
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs, "%-9s", "");
    }
    aim_printf(&uc->pvs, "\n");
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);

  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_display_ppg_resume_limit(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "%-7s", "PPG");
  aim_printf(&uc->pvs, "%-16s", "ResumeLimit ");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 29, HLINE);
  aim_printf(&uc->pvs, "\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;

  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint32_t limit = 0;
    rc |= bf_tm_ppg_resume_limit_get(devid, ppg_hndl, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    const bool print = (nz | limit);
    if (print) {
      aim_printf(&uc->pvs, "%-7u", ppg);
      if (!hex) {
        aim_printf(&uc->pvs, "%-16u", limit);
      } else {
        aim_printf(&uc->pvs, "%-16x", limit);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_ppg_details(ucli_context_t *uc,
                                        int nz,
                                        int hex,
                                        bf_dev_id_t devid,
                                        int pipe,
                                        int rsrc) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "Ppg Details (Threshold Config, Usage, Watermark)\n");
  aim_printf(&uc->pvs, "________________________________________________\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, rsrc);

  if ((uint32_t)rsrc < cfg.total_ppg_per_pipe) {
    uint32_t min_limit = 0;
    rc |= bf_tm_ppg_guaranteed_min_limit_get(devid, ppg_hndl, &min_limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | min_limit);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Min Limit cells",
                   min_limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Min Limit cells",
                   min_limit);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  if ((uint32_t)rsrc < cfg.pfc_ppg_per_pipe) {
    uint32_t skid_lmt = 0;
    rc |= bf_tm_ppg_skid_limit_get(devid, ppg_hndl, &skid_lmt);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | skid_lmt);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "HeadRoom Limit cells",
                   skid_lmt);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "HeadRoom Limit cells",
                   skid_lmt);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  if ((uint32_t)rsrc < cfg.total_ppg_per_pipe) {
    uint32_t limit = 0;
    rc |= bf_tm_ppg_resume_limit_get(devid, ppg_hndl, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-16u",
                   "Resume Limit ",
                   limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-16x",
                   "Resume Limit ",
                   limit);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  if ((uint32_t)rsrc < cfg.total_ppg_per_pipe) {
    uint32_t shr_lmt = 0;
    bf_tm_ppg_baf_t baf = 0;
    uint32_t hyst = 0;
    bool fast_recovery;
    rc |= bf_tm_ppg_app_pool_usage_get(
        devid, ppg_hndl, BF_TM_IG_APP_POOL_0, &shr_lmt, &baf, &hyst);
    rc |= bf_tm_ppg_fast_recovery_mode_get(devid, ppg_hndl, &fast_recovery);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    if (baf == BF_TM_PPG_BAF_DISABLE) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Base use Limit ",
                   shr_lmt);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Base use Limit ",
                   shr_lmt);
      }
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-19s",
                 "Base use Limit ",
                 "N/A");
    }

    aim_printf(&uc->pvs, "\n");
    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-11u",
                 "HystOffset ",
                 hyst);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-11x",
                 "HystOffset ",
                 hyst);
    }
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Dynamic ");
    TM_UCLI_PRINT_BOOLEAN(baf != BF_TM_PPG_BAF_DISABLE, 8)
    aim_printf(&uc->pvs, "\n");

    if (baf != BF_TM_PPG_BAF_DISABLE) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-4.1f%-5c",
                 "BAF",
                 bf_tm_get_ig_ppg_baf_percentage(baf),
                 '%');
      aim_printf(&uc->pvs, "\n");
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-8s",
                 "BAF",
                 "N/A");
      aim_printf(&uc->pvs, "\n");
    }

    aim_printf(&uc->pvs, "%-40s ", "FastRecover ");
    TM_UCLI_PRINT_BOOLEAN(fast_recovery, 12)
    aim_printf(&uc->pvs, "\n");
  }

  if ((uint32_t)rsrc < cfg.pfc_ppg_per_pipe) {
    uint8_t icos_mask;
    rc |= bf_tm_ppg_icos_mask_get(devid, ppg_hndl, &icos_mask);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-10u",
                 "icos-mask ",
                 icos_mask);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-10x",
                 "icos-mask ",
                 icos_mask);
    }
    aim_printf(&uc->pvs, "\n");
  }

  if ((uint32_t)rsrc < cfg.total_ppg_per_pipe) {
    uint32_t gmin = 0, shr = 0, skid = 0, wm = 0;
    rc |= bf_tm_ppg_usage_get(devid, pipe, ppg_hndl, &gmin, &shr, &skid, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    bool print = (nz | wm);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-23u",
                   "Buffer Usage Watermark ",
                   wm);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-23x",
                   "Buffer Usage Watermark ",
                   wm);
      }
      aim_printf(&uc->pvs, "\n");
    }

    print = (nz | gmin);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32u",
                   "GMin Ppg Cell Usage",
                   gmin);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32x",
                   "GMin Ppg Cell Usage",
                   gmin);
      }
      aim_printf(&uc->pvs, "\n");
    }

    print = (nz | shr);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32u",
                   "Shared PoolPpg Cell Usage",
                   shr);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32x",
                   "Shared PoolPpg Cell Usage",
                   shr);
      }
      aim_printf(&uc->pvs, "\n");
    }

    if ((uint32_t)rsrc < cfg.pfc_ppg_per_pipe) {
      print = (nz | skid);
      if (print) {
        if (!hex) {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32u",
                     "Skid Ppg Cell Usage",
                     skid);
        } else {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32x",
                     "Skid Ppg Cell Usage",
                     skid);
        }
        aim_printf(&uc->pvs, "\n");
      }
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_queue_details(ucli_context_t *uc,
                                          int nz,
                                          int hex,
                                          bf_dev_id_t devid,
                                          int pipe,
                                          int rsrc) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  if ((uint32_t)rsrc >= cfg.q_per_pg * cfg.pg_per_pipe) {
    return BF_UNEXPECTED;
  }

  aim_printf(&uc->pvs, "Queue Details (Threshold Config, Usage, Watermark)\n");
  aim_printf(&uc->pvs, "________________________________________________\n");

  bf_tm_pg_t pg_id = rsrc / cfg.q_per_pg;
  uint8_t pg_q = rsrc % cfg.q_per_pg;
  bf_dev_port_t devport = 0;
  bf_tm_queue_t portq = 0;
  bool mapped;
  bf_dev_target_t tgt;
  tgt.dev_pipe_id = pipe;
  tgt.device_id = devid;
  rc |= bf_tm_pg_port_queue_get(&tgt, pg_id, pg_q, &devport, &portq, &mapped);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  {
    aim_printf(&uc->pvs,
               "%-40s "
               "%-19u",
               "Port (D_P)",
               DEV_PORT_TO_LOCAL_PORT(devport));
    aim_printf(&uc->pvs, "\n");
    aim_printf(&uc->pvs,
               "%-40s "
               "%-19u"
               "%s",
               "Port queue number (queue_nr)",
               portq,
               mapped ? "" : " (not currently mapped)");
    aim_printf(&uc->pvs, "\n");
  }

  uint32_t l1_node = 0;

  {
    uint32_t limit;
    rc |= bf_tm_q_guaranteed_min_limit_get(devid, devport, portq, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    const bool print = (nz | limit);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Min Limit ",
                   limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Min Limit ",
                   limit);
      }
      if (print) aim_printf(&uc->pvs, "\n");
    }
  }

  {
    bf_tm_app_pool_t poolid = 0;
    uint32_t limit = 0;
    bf_tm_queue_baf_t baf;
    uint32_t hysteresis;
    rc |= bf_tm_q_app_pool_usage_get(
        devid, devport, portq, &poolid, &limit, &baf, &hysteresis);
    bool fast_recovery;
    rc |= bf_tm_q_fast_recovery_mode_get(devid, devport, portq, &fast_recovery);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    if (baf == BF_TM_Q_BAF_DISABLE) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Shared Limit ",
                   limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Shared Limit ",
                   limit);
      }
      aim_printf(&uc->pvs, "\n");
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-19s",
                 "Shared Limit ",
                 "N/A");
      aim_printf(&uc->pvs, "\n");
    }

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-22u",
                 "Hysteresis Table Index",
                 hysteresis);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-22x",
                 "Hysteresis Table Index",
                 hysteresis);
    }
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Dynamic ");
    TM_UCLI_PRINT_BOOLEAN(baf != BF_TM_Q_BAF_DISABLE, 8)
    aim_printf(&uc->pvs, "\n");

    if (baf != BF_TM_Q_BAF_DISABLE) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-4.1f%-5c",
                 "BAF",
                 bf_tm_get_eg_q_baf_percentage(baf),
                 '%');
    } else {
      aim_printf(&uc->pvs, "%-8s", "N/A");
    }
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "FastRecover ");
    TM_UCLI_PRINT_BOOLEAN(fast_recovery, 12)
    aim_printf(&uc->pvs, "\n");

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-14u",
                 "Shared PoolId ",
                 poolid);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-14x",
                 "Shared PoolId ",
                 poolid);
    }
    aim_printf(&uc->pvs, "\n");
  }

  {
    bool color_drop;
    rc |= bf_tm_q_color_drop_get(devid, devport, portq, &color_drop);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Q-Color-Drop-Enable ");
    TM_UCLI_PRINT_BOOLEAN(color_drop, 20)
    aim_printf(&uc->pvs, "\n");
  }

  {
    bool tail_drop;
    rc |= bf_tm_q_tail_drop_get(devid, devport, portq, &tail_drop);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs, "%-40s ", "Q-Drop-Enable ");
    TM_UCLI_PRINT_BOOLEAN(tail_drop, 14)
    aim_printf(&uc->pvs, "\n");
  }

  {
    bf_tm_queue_color_limit_t limit;
    rc |=
        bf_tm_q_color_limit_get(devid, devport, portq, BF_TM_COLOR_RED, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%-40s "
               "%-4.1f%-5c",
               "Red Limit Pcent",
               bf_tm_get_q_color_limit_percentage(limit),
               '%');
    aim_printf(&uc->pvs, "\n");
  }

  {
    uint32_t hysteresis = 0;
    rc |= bf_tm_q_color_hysteresis_get(
        devid, devport, portq, BF_TM_COLOR_RED, &hysteresis);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-27u",
                 "Red Limit Hyst Table Index ",
                 hysteresis);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-27x",
                 "Red Limit Hyst Table Index ",
                 hysteresis);
    }
    aim_printf(&uc->pvs, "\n");
  }

  {
    bf_tm_queue_color_limit_t limit;
    rc |= bf_tm_q_color_limit_get(
        devid, devport, portq, BF_TM_COLOR_YELLOW, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%-40s "
               "%-4.1f%-5c",
               "Yellow Limit Pcent",
               bf_tm_get_q_color_limit_percentage(limit),
               '%');
    aim_printf(&uc->pvs, "\n");
  }

  {
    uint32_t hysteresis = 0;
    rc |= bf_tm_q_color_hysteresis_get(
        devid, devport, portq, BF_TM_COLOR_YELLOW, &hysteresis);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-27u",
                 "Yellow Limit Hyst Table Index ",
                 hysteresis);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-27x",
                 "Yellow Limit Hyst Table Index ",
                 hysteresis);
    }
    aim_printf(&uc->pvs, "\n");
  }

  {
    uint32_t count = 0, wm = 0;
    rc |= bf_tm_q_usage_get(devid, pipe, devport, portq, &count, &wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    bool print = (nz | count);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32u",
                   "Queue Usage",
                   count);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32x",
                   "Queue Usage",
                   count);
      }
      aim_printf(&uc->pvs, "\n");
    }

    print = (nz | wm);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32u",
                   "Queue watermark",
                   wm);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32x",
                   "Queue watermark",
                   wm);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  {
    bool status = false;
    rc |= bf_tm_sched_q_egress_pfc_status_get(devid, devport, portq, &status);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs, "%-40s ", "Pfc Status ");
    TM_UCLI_PRINT_BOOLEAN(status, 11)
    aim_printf(&uc->pvs, "\n");
  }

  { /*Too much low lvl values to create API for everyone. TM_SCH API used*/
    bf_tm_eg_q_t *q;
    rc |= bf_tm_q_get_descriptor(devid, devport, portq, &q);
    bool status;
    rc |= bf_tm_sch_get_q_sched(devid, q, &status, &status);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    l1_node = q->q_sch_cfg.cid;
    aim_printf(&uc->pvs, "%-40s ", "Queue Enabled");
    TM_UCLI_PRINT_BOOLEAN(q->q_sch_cfg.sch_enabled, 13)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Pfc Update Enabled");
    TM_UCLI_PRINT_BOOLEAN(q->q_sch_cfg.sch_pfc_enabled, 18)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Min Rate Enabled");
    TM_UCLI_PRINT_BOOLEAN(q->q_sch_cfg.min_rate_enable, 16)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Max Rate Enabled");
    TM_UCLI_PRINT_BOOLEAN(q->q_sch_cfg.max_rate_enable, 16)
    aim_printf(&uc->pvs, "\n");

    aim_printf(
        &uc->pvs,
        "%-40s "
        "%-10u",
        BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type) ? "Channel Id" : "L1 ID",
        q->q_sch_cfg.cid);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-12u",
               "PFC Priority",
               q->q_sch_cfg.pfc_prio);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-17u",
               "Min Rate Sch Prio",
               q->q_sch_cfg.min_rate_sch_prio);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-16u",
               "Max Rate Sch Pri",
               q->q_sch_cfg.max_rate_sch_prio);

    aim_printf(&uc->pvs, "\n");
  }

  {
    bool pps;
    uint32_t bss = 0, rate = 0;
    rc |= bf_tm_sched_q_guaranteed_rate_get(
        devid, devport, portq, &pps, &bss, &rate);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Min Rate pps/bps ");
    TM_UCLI_PRINT_BOOLEAN(pps, 17)
    aim_printf(&uc->pvs, "\n");

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23u",
                 "Min Rate Burst size ",
                 bss);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23x",
                 "Min Rate Burst size ",
                 bss);
    }
    aim_printf(&uc->pvs, "\n");

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17u",
                 "Min Rate ",
                 rate);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17x",
                 "Min Rate ",
                 rate);
    }

    aim_printf(&uc->pvs, "\n");
  }

  {
    bool pps;
    uint32_t bss = 0, rate = 0;
    rc |= bf_tm_sched_q_shaping_rate_get(
        devid, devport, portq, &pps, &bss, &rate);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Max Rate pps/bps ");
    TM_UCLI_PRINT_BOOLEAN(pps, 17)
    aim_printf(&uc->pvs, "\n");

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23u",
                 "Max Rate Burst size ",
                 bss);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23x",
                 "Max Rate Burst size ",
                 bss);
    }
    aim_printf(&uc->pvs, "\n");

    if (!hex) {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17u",
                 "Max Rate ",
                 rate);
    } else {
      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17x",
                 "Max Rate ",
                 rate);
    }

    aim_printf(&uc->pvs, "\n");
  }

  {
    bf_tm_sched_adv_fc_mode_t afc = 0;
    rc |= bf_tm_sched_q_adv_fc_mode_get(devid, devport, portq, &afc);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs,
               "%-40s "
               "%-16u",
               "AFC (0 - credit, 1 - xoff)",
               afc);
    aim_printf(&uc->pvs, "\n");
  }

  if (!(BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type))) {
    // Dump L1 Configuration
    aim_printf(&uc->pvs, "\n");
    aim_printf(&uc->pvs, "L1 node dump");
    aim_printf(&uc->pvs, "________________________________________________\n");

    bf_tm_eg_l1_t *l1 = NULL;
    rc = bf_tm_sch_l1_get_descriptor(devid, devport, l1_node, &l1);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc) {
      rc |= bf_tm_sch_get_l1_sch(devid, l1);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
      aim_printf(&uc->pvs, "%-40s ", "L1 Enabled");
      TM_UCLI_PRINT_BOOLEAN(l1->l1_sch_cfg.sch_enabled, 13)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs, "%-40s ", "L1 Min Rate Enable");
      TM_UCLI_PRINT_BOOLEAN(l1->l1_sch_cfg.min_rate_enable, 13)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs, "%-40s ", "L1 Max Rate Enable");
      TM_UCLI_PRINT_BOOLEAN(l1->l1_sch_cfg.max_rate_enable, 13)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-13u",
                 "L1 l1_port",
                 l1->l1_sch_cfg.cid);
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-13u",
                 "L1 Min Rate Priority",
                 l1->l1_sch_cfg.min_rate_sch_prio);
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-13u",
                 "L1 Max Rate Priority",
                 l1->l1_sch_cfg.max_rate_sch_prio);
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs, "%-40s ", "L1 Priority Propagated");
      TM_UCLI_PRINT_BOOLEAN(l1->l1_sch_cfg.pri_prop, 13)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs, "%-40s ", "L1 pps");
      TM_UCLI_PRINT_BOOLEAN(l1->l1_sch_cfg.pps, 13)
      aim_printf(&uc->pvs, "\n");

      bool pps = false;
      uint32_t bss = 0, rate = 0;
      rc |= bf_tm_sched_l1_guaranteed_rate_get(
          devid, devport, l1_node, &pps, &bss, &rate);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      aim_printf(&uc->pvs, "%-40s ", "L1 Min Rate pps/bps ");
      TM_UCLI_PRINT_BOOLEAN(pps, 17)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23u",
                 "L1 Min Rate Burst size ",
                 bss);
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17u",
                 "L1 Min Rate ",
                 rate);
      aim_printf(&uc->pvs, "\n");

      pps = false;
      bss = 0;
      rate = 0;
      rc |= bf_tm_sched_l1_shaping_rate_get(
          devid, devport, l1_node, &pps, &bss, &rate);
      TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

      aim_printf(&uc->pvs, "%-40s ", "L1 Max Rate pps/bps ");
      TM_UCLI_PRINT_BOOLEAN(pps, 17)
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-23u",
                 "L1 Max Rate Burst size ",
                 bss);
      aim_printf(&uc->pvs, "\n");

      aim_printf(&uc->pvs,
                 "%-40s "
                 "%-17u",
                 "L1 Max Rate ",
                 rate);
      aim_printf(&uc->pvs, "\n");
    }
  }
  return rc;
}

bf_status_t tm_ucli_display_port_details(ucli_context_t *uc,
                                         int nz,
                                         int hex,
                                         bf_dev_id_t devid,
                                         int pipe,
                                         int rsrc) {
  bf_status_t rc = BF_SUCCESS;
  aim_printf(&uc->pvs, "Port Details (Threshold Config, Usage, Watermark)\n");
  aim_printf(&uc->pvs, "________________________________________________\n");

  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  uint32_t max_ing_port = cfg.ports_per_pg * cfg.pg_per_pipe + 1;
  uint32_t max_eg_port = cfg.ports_per_pg * cfg.pg_per_pipe;
  bf_dev_port_t devport = MAKE_DEV_PORT(pipe, rsrc);

  uint32_t lport = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_user_to_device(devid, devport));

  aim_printf(&uc->pvs,
             "%-40s "
             "%-19u",
             "Devport ",
             devport);
  aim_printf(&uc->pvs, "\n");

  if (lport < max_ing_port) {
    uint32_t limit = 0, hyst = 0;
    rc = bf_tm_port_ingress_drop_limit_get(devid, devport, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    bool print = (nz | limit);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Ingress Port Limit ",
                   limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Ingress Port Limit ",
                   limit);
      }
      aim_printf(&uc->pvs, "\n");
    }

    rc = bf_tm_port_ingress_hysteresis_get(devid, devport, &hyst);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    print = (nz | hyst);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Ingress Port Hysteresis ",
                   hyst);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Ingress Port Hysteresis ",
                   hyst);
      }
      aim_printf(&uc->pvs, "\n");
    }

    uint32_t ig_usage = 0, eg_usage = 0, ig_wm = 0, eg_wm = 0;
    rc = bf_tm_port_usage_get(
        devid, pipe, devport, &ig_usage, &eg_usage, &ig_wm, &eg_wm);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    print = (nz | ig_wm);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-22u",
                   "Port Watermark in WAC ",
                   ig_wm);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-22x",
                   "Port Watermark in WAC ",
                   ig_wm);
      }
      aim_printf(&uc->pvs, "\n");
    }

    print = (nz | ig_usage);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Cell usage in WAC",
                   ig_usage);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Cell usage in WAC",
                   ig_usage);
      }
      aim_printf(&uc->pvs, "\n");
    }

    if (lport < max_eg_port) {
      print = (nz | eg_wm);
      if (print) {
        if (!hex) {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32u",
                     "Port Watermark in QAC",
                     eg_wm);
        } else {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32x",
                     "Port Watermark in QAC",
                     eg_wm);
        }
        aim_printf(&uc->pvs, "\n");
      }

      print = (nz | eg_usage);
      if (print) {
        if (!hex) {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32u",
                     "Cell Usage in QAC",
                     eg_usage);
        } else {
          aim_printf(&uc->pvs,
                     "%-40s "
                     "%-32x",
                     "Cell Usage in QAC",
                     eg_usage);
        }
        aim_printf(&uc->pvs, "\n");
      }
    }
  }

  if (lport < max_eg_port) {
    bf_tm_flow_ctrl_type_t type;
    rc = bf_tm_port_flowcontrol_mode_get(devid, devport, &type);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs,
               "%-40s "
               "%-24s",
               "WAC Port PFC / Port Pause Enable",
               type == BF_TM_PAUSE_NONE
                   ? "none"
                   : (type == BF_TM_PAUSE_PFC ? "pfc" : "port"));
    aim_printf(&uc->pvs, "\n");

    /* Ingress and egress drop limits are in sync
     * Check bf_tm_tofino_port_get_drop_limit() and
     * bf_tm_tof2_port_get_drop_limit() for details
     */
    uint32_t limit = 0, hyst = 0;
    rc = bf_tm_port_ingress_drop_limit_get(devid, devport, &limit);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    rc = bf_tm_port_egress_hysteresis_get(devid, devport, &hyst);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    bool print = (nz | limit);

    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19u",
                   "Egress Port Limit ",
                   limit);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-19x",
                   "Egress Port Limit ",
                   limit);
      }
      aim_printf(&uc->pvs, "\n");
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-21u",
                   "Egress Port Hystersis ",
                   hyst);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-21x",
                   "Egress Port Hystersis ",
                   hyst);
      }
      aim_printf(&uc->pvs, "\n");
    }

    bool drop_st = 0;
    rc = bf_tm_port_qac_drop_state_get(devid, devport, &drop_st);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Drop Status in QAC ");
    TM_UCLI_PRINT_BOOLEAN(drop_st, 19)
    aim_printf(&uc->pvs, "\n");

    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Queues mapped (QID count) ");
    aim_printf(&uc->pvs, "%-5u ", q_count);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "QID mapping ");
    aim_printf(&uc->pvs, "%-1s ", "[");
    for (uint8_t i = 0; i < cfg.q_per_pg && i < BF_TM_MAX_QUEUE_PER_PG; ++i) {
      aim_printf(&uc->pvs, "%-1d", q_mapping[i]);
      if (i < cfg.q_per_pg - 1) aim_printf(&uc->pvs, "%-1s ", ",");
    }
    aim_printf(&uc->pvs, "%-1s ", " ]");

    aim_printf(&uc->pvs, "\n");

    bool enbl;
    bf_tm_port_t *p;
    rc = bf_tm_port_get_descriptor(devid, devport, &p);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    rc = bf_tm_sch_get_port_sched(devid, p, &enbl, &enbl);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "Sch Port Enabled");
    TM_UCLI_PRINT_BOOLEAN(enbl, 16)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-22u",
               "Sch Pfc Update Enabled",
               p->fc_rx_type);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "TDM Enabled");
    TM_UCLI_PRINT_BOOLEAN(p->tdm, 11)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "%-40s ", "Max Rate Enabled");
    TM_UCLI_PRINT_BOOLEAN(p->max_rate_enabled, 16)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-17u",
               "Max Packet Credit",
               p->credit);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-1d %1s",
               "Port Speed Mode",
               bf_tm_get_port_speed_mode_g(p->speed),
               "G");
    aim_printf(&uc->pvs, "\n");

    uint8_t status = 0;
    rc = bf_tm_sched_port_egress_pfc_status_get(devid, devport, &status);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    print = (nz | status);
    if (print) {
      if (!hex) {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32u",
                   "Sch Pfc Status",
                   status);
      } else {
        aim_printf(&uc->pvs,
                   "%-40s "
                   "%-32x",
                   "Sch Pfc Status",
                   status);
      }
      aim_printf(&uc->pvs, "\n");
    }

    bool pps;
    uint32_t bss = 0, rate = 0;
    rc = bf_tm_sched_port_shaping_rate_get(devid, devport, &pps, &bss, &rate);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

    aim_printf(&uc->pvs, "%-40s ", "pps/bps ");
    TM_UCLI_PRINT_BOOLEAN(pps, 8)
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-19u",
               "Port Burst size ",
               bss);
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs,
               "%-40s "
               "%-19u",
               "Port Rate ",
               rate);
    aim_printf(&uc->pvs, "\n");

    bool ct_en = false;
    rc = bf_tm_port_cut_through_enable_status_get(
        devid, devport, &ct_en, &ct_en);
    TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
    aim_printf(&uc->pvs, "%-40s ", "cut through enable ");
    TM_UCLI_PRINT_BOOLEAN(ct_en, 8)
    aim_printf(&uc->pvs, "\n");
  }

  return rc;
}

bf_status_t tm_ucli_display_pre_port_mask_vector(ucli_context_t *uc,
                                                 int nz,
                                                 int hex,
                                                 bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  uint32_t size = (cfg.pipe_cnt * cfg.ports_per_pg * cfg.pg_per_pipe * 2) / 32;
  uint32_t *array = bf_sys_malloc(size * sizeof(uint32_t));
  rc |= bf_tm_port_get_pre_port_mask(devid, array, size);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  for (uint32_t e = 0; e < 2; ++e) {
    uint32_t offset = e * (size / 2);
    for (uint32_t i = 0; i < size / 2; ++i) {
      if (i == 0) {
        aim_printf(&uc->pvs, "Entry %d\n", e);
      }
      aim_printf(&uc->pvs, "Port%3u..%3u", i * 32 + 31, i * 32);
      if (!hex) {
        aim_printf(&uc->pvs, "%-32u", array[i + offset]);
      } else {
        aim_printf(&uc->pvs, "%-32x", array[i + offset]);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  return rc;
}

bf_status_t tm_ucli_clear_pre_port_mask_vector(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc = bf_tm_port_clear_pre_port_mask(devid);
  return rc;
}

bf_status_t tm_ucli_display_pre_port_down_mask(ucli_context_t *uc,
                                               int nz,
                                               int hex,
                                               bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  (void)nz;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)

  uint32_t size = (cfg.pipe_cnt * cfg.ports_per_pg * cfg.pg_per_pipe) / 32;
  uint32_t *array = bf_sys_malloc(size * sizeof(uint32_t));
  rc |= bf_tm_port_get_pre_port_down_mask(devid, array, size);
  TM_UCLI_CHECK_AND_PRINT_ERROR(rc)
  for (uint32_t i = 0; i < size; ++i) {
    aim_printf(&uc->pvs, "Port%3u..%3u", i * 32 + 31, i * 32);
    if (!hex) {
      aim_printf(&uc->pvs, "%-32u", array[i]);
    } else {
      aim_printf(&uc->pvs, "%-32x", array[i]);
    }
    aim_printf(&uc->pvs, "\n");
  }

  return rc;
}

bf_status_t tm_ucli_clear_pre_port_down_mask(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc = bf_tm_port_clear_pre_port_down_mask(devid);
  return rc;
}

bf_status_t tm_ucli_reset_qac_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc = bf_tm_pool_egress_buffer_drop_state_clear(devid,
                                                 PER_EG_PIPE_BUFF_DROP_ST);
  return rc;
}

bf_status_t tm_ucli_reset_qac_green_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc = bf_tm_pool_egress_buffer_drop_state_clear(devid,
                                                 GLB_BUFF_AP_GREEN_DROP_ST);
  return rc;
}

bf_status_t tm_ucli_reset_qac_yel_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc =
      bf_tm_pool_egress_buffer_drop_state_clear(devid, GLB_BUFF_AP_YEL_DROP_ST);
  return rc;
}

bf_status_t tm_ucli_reset_qac_red_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc =
      bf_tm_pool_egress_buffer_drop_state_clear(devid, GLB_BUFF_AP_RED_DROP_ST);
  return rc;
}

bf_status_t tm_ucli_reset_qac_pipe_pre_fifo_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc |=
      bf_tm_pool_egress_buffer_drop_state_clear(devid, PIPE0_PRE_FIFO_DROP_ST);
  rc |=
      bf_tm_pool_egress_buffer_drop_state_clear(devid, PIPE1_PRE_FIFO_DROP_ST);
  rc |=
      bf_tm_pool_egress_buffer_drop_state_clear(devid, PIPE2_PRE_FIFO_DROP_ST);
  rc |=
      bf_tm_pool_egress_buffer_drop_state_clear(devid, PIPE3_PRE_FIFO_DROP_ST);
  return rc;
}

bf_status_t tm_ucli_reset_wac_color_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc |= bf_tm_pool_color_drop_state_clear(devid, BF_TM_COLOR_GREEN);
  rc |= bf_tm_pool_color_drop_state_clear(devid, BF_TM_COLOR_YELLOW);
  rc |= bf_tm_pool_color_drop_state_clear(devid, BF_TM_COLOR_RED);
  return rc;
}

bf_status_t tm_ucli_reset_wac_skid_pool_dropstate(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc |= bf_tm_pool_color_drop_state_clear(devid, 4);
  return rc;
}

bf_status_t tm_ucli_reset_wac_queue_shadow_state(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, 0, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc |= bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)

    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_shadow_drop_state_clear(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_wac_eg_qid_map(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    for (uint32_t e = 0; e < 512; e++) {
      rc |= bf_tm_tofino_clear_wac_eg_qid_mapping(devid, e);
    }
  }
  return rc;
}

bf_status_t bf_tm_power_on_reset_non_pipe_tbls(bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  rc |= tm_ucli_reset_wac_eg_qid_map(devid);
  rc |= tm_ucli_reset_wac_color_dropstate(devid);
  rc |= tm_ucli_reset_wac_skid_pool_dropstate(devid);
  rc |= tm_ucli_reset_wac_queue_shadow_state(devid);
  rc |= tm_ucli_reset_qac_dropstate(devid);
  rc |= tm_ucli_reset_qac_green_dropstate(devid);
  rc |= tm_ucli_reset_qac_yel_dropstate(devid);
  rc |= tm_ucli_reset_qac_red_dropstate(devid);
  rc |= tm_ucli_reset_qac_pipe_pre_fifo_dropstate(devid);
  rc |= tm_ucli_clear_pre_port_mask_vector(devid);
  rc |= tm_ucli_clear_pre_port_down_mask(devid);
  return rc;
}

bf_status_t tm_ucli_reset_ppg_min_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_guaranteed_min_limit_set(devid, ppg_hndl, 0);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_ppg_hdr_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_skid_limit_set(devid, ppg_hndl, 0);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_ing_port_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_ingress_drop_limit_set(devid, devport, 0);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_qac_qid_map(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t e;
  for (e = 0; e < 512; e++) {
    rc |= bf_tm_tofino_clear_qac_qid_mapping(devid, pipe, e);
  }
  return rc;
}

bf_status_t tm_ucli_reset_wac_port_usage_count(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_ingress_usage_count_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_port_dropstate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_wac_drop_state_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_ppg_dropstate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_wac_drop_state_clear(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_ppg_usage(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_usage_clear(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_q_min_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_guaranteed_min_limit_set(devid, devport, portq, 0);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_color_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc |=
          bf_tm_q_color_limit_set(devid, devport, portq, BF_TM_COLOR_GREEN, 0);
      rc |=
          bf_tm_q_color_limit_set(devid, devport, portq, BF_TM_COLOR_YELLOW, 0);
      rc |= bf_tm_q_color_limit_set(devid, devport, portq, BF_TM_COLOR_RED, 0);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_ap_cfg(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc |= bf_tm_q_app_pool_usage_set(devid, devport, portq, 0, 0, 0, 0);
      rc |= bf_tm_q_tail_drop_disable(devid, devport, portq);
      rc |= bf_tm_q_color_drop_disable(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_eg_color_dropstate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc |= bf_tm_q_color_drop_state_clear(
          devid, devport, portq, BF_TM_COLOR_GREEN);
      rc |= bf_tm_q_color_drop_state_clear(
          devid, devport, portq, BF_TM_COLOR_YELLOW);
      rc |= bf_tm_q_color_drop_state_clear(
          devid, devport, portq, BF_TM_COLOR_RED);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_ppg_shared_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_app_pool_usage_set(devid, ppg_hndl, 0, 0, 0, 0);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_ppg_icos_mapping(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.pfc_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    uint8_t cos = 0;
    rc |= bf_tm_port_pfc_cos_mapping_set(devid, ppg_hndl, &cos);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_qac_port_usage_count(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_egress_usage_count_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_usage_counter(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_usage_clear(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_min_rate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_sched_q_guaranteed_rate_set(devid, devport, portq, 0, 0, 0);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_q_max_rate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_sched_q_shaping_rate_set(devid, devport, portq, 0, 0, 0);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_port_max_rate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_sched_port_shaping_rate_set(devid, devport, 0, 0, 0);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_egress_q_pfc(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_sched_q_enable(devid, devport, portq);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_egress_port_pfc(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    uint8_t q_count;
    uint8_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
    rc = bf_tm_port_q_mapping_get(devid, devport, &q_count, q_mapping);
    TM_UCLI_CHECK_ERROR(rc)
    for (uint8_t portq = 0; portq < q_count; ++portq) {
      rc = bf_tm_q_pfc_cos_mapping_set(devid, devport, portq, 0);
      TM_UCLI_CHECK_ERROR(rc)
    }
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_qac_port_dropstate(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_qac_drop_state_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_qac_port_drop_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;

  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_qac_drop_limit_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_ppg_resume_limit(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_resume_limit_clear(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_port_ppg_mapping(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_dev_cfg_t cfg;
  rc = bf_tm_dev_config_get(devid, &cfg);
  TM_UCLI_CHECK_ERROR(rc)
  uint32_t max_ppg = cfg.total_ppg_per_pipe;
  for (uint32_t ppg = 0; ppg < max_ppg; ppg++) {
    bf_tm_ppg_hdl ppg_hndl = BF_TM_PPG_HANDLE(pipe, 0, ppg);
    rc |= bf_tm_ppg_free(devid, ppg_hndl);
    TM_UCLI_CHECK_ERROR(rc)
  }
  return rc;
}

bf_status_t tm_ucli_reset_wac_pfc_state(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t devport = 0;
  rc = bf_tm_pipe_port_get_first(devid, pipe, false, &devport);
  TM_UCLI_CHECK_ERROR(rc)
  do {
    rc = bf_tm_port_pfc_state_clear(devid, devport);
    TM_UCLI_CHECK_ERROR(rc)
    rc = bf_tm_pipe_port_get_next(devid, devport, false, &devport);
  } while (BF_SUCCESS == rc);
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t tm_ucli_reset_wac_offset_profile(bf_dev_id_t devid, int pipe) {
  (void)pipe;  // restore works for all pipes
  bf_status_t rc = BF_SUCCESS;
  rc |= bf_tm_restore_wac_offset_profile(devid);
  return rc;
}

bf_status_t tm_ucli_reset_qac_offset_profile(bf_dev_id_t devid, int pipe) {
  (void)pipe;  // restore works for all pipes
  bf_status_t rc = BF_SUCCESS;
  rc |= bf_tm_restore_qac_offset_profile(devid);
  return rc;
}

bf_status_t tm_ucli_reset_qac_qid_profile(bf_dev_id_t devid, int pipe) {
  bf_status_t rc = BF_SUCCESS;
  if (BF_TM_IS_TOFINO(g_tm_ctx[devid]->asic_type)) {
    bf_dev_port_t devport = 0;
    rc = bf_tm_pipe_port_get_first(devid, pipe, true, &devport);
    TM_UCLI_CHECK_ERROR(rc)
    do {
      rc = bf_tm_tofino_clear_qac_qid_profile(
          devid, pipe, DEV_PORT_TO_LOCAL_PORT(devport));
      TM_UCLI_CHECK_ERROR(rc)
      rc = bf_tm_pipe_port_get_next(devid, devport, true, &devport);
    } while (BF_SUCCESS == rc);
  }
  return ((BF_OBJECT_NOT_FOUND == rc) ? BF_SUCCESS : rc);
}

bf_status_t bf_tm_ucli_power_on_reset_per_pipe_tbls(bf_dev_id_t devid,
                                                    int pipe) {
  bf_status_t rc = BF_SUCCESS;
  rc |= tm_ucli_reset_ppg_min_limit(devid, pipe);
  rc |= tm_ucli_reset_ppg_hdr_limit(devid, pipe);
  rc |= tm_ucli_reset_ing_port_limit(devid, pipe);
  rc |= tm_ucli_reset_ppg_resume_limit(devid, pipe);
  rc |= tm_ucli_reset_port_ppg_mapping(devid, pipe);
  rc |= tm_ucli_reset_ppg_shared_limit(devid, pipe);
  rc |= tm_ucli_reset_ppg_icos_mapping(devid, pipe);
  rc |= tm_ucli_reset_wac_offset_profile(devid, pipe);
  rc |= tm_ucli_reset_wac_pfc_state(devid, pipe);
  rc |= tm_ucli_reset_wac_ppg_watermark(devid, pipe);
  rc |= tm_ucli_reset_wac_port_watermark(devid, pipe);
  rc |= tm_ucli_reset_wac_port_usage_count(devid, pipe);
  rc |= tm_ucli_reset_ppg_usage(devid, pipe);
  rc |= tm_ucli_reset_ppg_dropstate(devid, pipe);
  rc |= tm_ucli_reset_port_dropstate(devid, pipe);
  rc |= tm_ucli_reset_q_min_limit(devid, pipe);
  rc |= tm_ucli_reset_q_ap_cfg(devid, pipe);
  rc |= tm_ucli_reset_q_color_limit(devid, pipe);
  rc |= tm_ucli_reset_qac_port_drop_limit(devid, pipe);
  rc |= tm_ucli_reset_qac_offset_profile(devid, pipe);
  rc |= tm_ucli_reset_qac_qid_profile(devid, pipe);
  rc |= tm_ucli_reset_qac_qid_map(devid, pipe);
  rc |= tm_ucli_reset_qac_q_wm(devid, pipe);
  rc |= tm_ucli_reset_qac_port_wm(devid, pipe);
  rc |= tm_ucli_reset_qac_port_usage_count(devid, pipe);
  rc |= tm_ucli_reset_q_usage_counter(devid, pipe);
  rc |= tm_ucli_reset_q_eg_color_dropstate(devid, pipe);
  rc |= tm_ucli_reset_qac_port_dropstate(devid, pipe);
  rc |= tm_ucli_reset_egress_port_pfc(devid, pipe);
  rc |= tm_ucli_reset_egress_q_pfc(devid, pipe);
  rc |= tm_ucli_reset_q_min_rate(devid, pipe);
  rc |= tm_ucli_reset_q_max_rate(devid, pipe);
  rc |= tm_ucli_reset_port_max_rate(devid, pipe);
  return rc;
}

bf_status_t tm_ucli_set_ddr_train(ucli_context_t *uc,
                                  int nz,
                                  int hex,
                                  bf_dev_id_t devid) {
  bf_status_t rc = BF_SUCCESS;
  (void)uc;
  (void)nz;
  (void)hex;

  rc = bf_tm_set_ddr_train(devid);
  return rc;
}
