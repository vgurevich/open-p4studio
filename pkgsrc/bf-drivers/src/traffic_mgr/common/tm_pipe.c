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


#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"





#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

static bf_tm_pipe_hw_funcs_tbl g_pipe_hw_fptr_tbl;

bf_status_t bf_tm_pipe_get_descriptor(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_tm_eg_pipe_t **p) {
  *p = g_tm_ctx[dev]->pipes + pipe;
  return (BF_SUCCESS);
}

bf_status_t bf_tm_pipe_set_limit(bf_dev_id_t dev,
                                 bf_tm_eg_pipe_t *pipe,
                                 uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, pipe->epipe_limit, g_tm_ctx[dev])) {
    return (rc);
  }

  pipe->epipe_limit = cells;
  if (g_pipe_hw_fptr_tbl.pipe_limit_wr_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_limit_wr_fptr(
        dev, pipe->p_pipe, pipe->epipe_limit);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_set_hyst(bf_dev_id_t dev,
                                bf_tm_eg_pipe_t *pipe,
                                uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, pipe->epipe_resume_limit, g_tm_ctx[dev])) {
    return (rc);
  }

  pipe->epipe_resume_limit = cells;
  if (g_pipe_hw_fptr_tbl.pipe_hyst_wr_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_hyst_wr_fptr(
        dev, pipe->p_pipe, pipe->epipe_resume_limit);
  }
  return (rc);
}

bf_status_t bf_tm_set_qstat_report_mode(bf_dev_id_t dev,
                                        bf_tm_eg_pipe_t *pipe,
                                        bool mode) {
  bf_status_t rc = BF_SUCCESS;

  if (g_pipe_hw_fptr_tbl.qstat_report_mode_wr_fptr) {
    rc = g_pipe_hw_fptr_tbl.qstat_report_mode_wr_fptr(dev, pipe, mode);
  }
  return rc;
}

bf_status_t bf_tm_get_qstat_report_mode(bf_dev_id_t dev,
                                        bf_tm_eg_pipe_t *pipe,
                                        bool *mode) {
  bf_status_t rc = BF_SUCCESS;

  if (g_pipe_hw_fptr_tbl.qstat_report_mode_rd_fptr) {
    rc = g_pipe_hw_fptr_tbl.qstat_report_mode_rd_fptr(dev, pipe, mode);
  }
  return rc;
}

bf_status_t bf_tm_set_deflection_port_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool enable) {
  bf_status_t rc = BF_SUCCESS;

  if (g_pipe_hw_fptr_tbl.pipe_defd_port_en_wr_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_defd_port_en_wr_fptr(dev, pipe, enable);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return rc;
}

bf_status_t bf_tm_get_deflection_port_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool *enable) {
  bf_status_t rc = BF_SUCCESS;

  if (g_pipe_hw_fptr_tbl.pipe_defd_port_en_rd_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_defd_port_en_rd_fptr(dev, pipe, enable);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return rc;
}

bf_status_t bf_tm_pipe_get_limit(bf_dev_id_t dev,
                                 bf_tm_eg_pipe_t *pipe,
                                 uint32_t *sw_cells,
                                 uint32_t *hw_cells) {
  bf_status_t rc = BF_SUCCESS;

  *sw_cells = pipe->epipe_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_cells &&
      g_pipe_hw_fptr_tbl.pipe_limit_rd_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_limit_rd_fptr(dev, pipe->p_pipe, hw_cells);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_hyst(bf_dev_id_t dev,
                                bf_tm_eg_pipe_t *pipe,
                                uint32_t *sw_cells,
                                uint32_t *hw_cells) {
  bf_status_t rc = BF_SUCCESS;

  *sw_cells = pipe->epipe_resume_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_cells &&
      g_pipe_hw_fptr_tbl.pipe_hyst_rd_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_hyst_rd_fptr(dev, pipe->p_pipe, hw_cells);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_total_in_cell_count(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_get_total_cells) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_get_total_cells(dev, p->p_pipe, count);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_clear_total_in_cell_count(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_total_cells_clear_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_total_cells_clear_fptr(dev, p->p_pipe);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_total_in_pkt_count(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);

  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_get_total_pkts) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_get_total_pkts(dev, p->p_pipe, count);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_clear_total_in_packet_count(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_total_packets_clear_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_total_packets_clear_fptr(dev, p->p_pipe);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_uc_ct_count(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);

  if (p && g_pipe_hw_fptr_tbl.pipe_get_uc_ct_count) {
    rc = g_pipe_hw_fptr_tbl.pipe_get_uc_ct_count(dev, p->p_pipe, count);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_clear_uc_ct_count(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);

  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_uc_ct_clear_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_uc_ct_clear_fptr(dev, p->p_pipe);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_mc_ct_count(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);

  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_get_total_pkts) {
    rc = g_pipe_hw_fptr_tbl.pipe_get_mc_ct_count(dev, p->p_pipe, count);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_clear_mc_ct_count(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p = NULL;

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);

  if (p && g_pipe_hw_fptr_tbl.pipe_cntr_mc_ct_clear_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_cntr_mc_ct_clear_fptr(dev, p->p_pipe);
  }
  return (rc);
}

bf_status_t bf_tm_pipe_get_defaults(bf_dev_id_t dev,
                                    bf_tm_eg_pipe_t *pipe,
                                    bf_tm_pipe_defaults_t *defaults) {
  bf_status_t rc = BF_SUCCESS;

  if (g_pipe_hw_fptr_tbl.pipe_get_defaults_fptr) {
    rc = g_pipe_hw_fptr_tbl.pipe_get_defaults_fptr(dev, pipe, defaults);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

#define BF_TM_PIPE_HW_FTBL_WR_FUNCS(pipe_max_limit,                 \
                                    pipe_hyst,                      \
                                    qstat_report,                   \
                                    clear_total_in_cells,           \
                                    clear_total_in_packets,         \
                                    clear_uc_ct_count,              \
                                    clear_mc_ct_count,              \
                                    defd_port_enable)               \
  g_pipe_hw_fptr_tbl = (bf_tm_pipe_hw_funcs_tbl){                   \
      .pipe_limit_wr_fptr = pipe_max_limit,                         \
      .pipe_hyst_wr_fptr = pipe_hyst,                               \
      .qstat_report_mode_wr_fptr = qstat_report,                    \
      .pipe_cntr_total_cells_clear_fptr = clear_total_in_cells,     \
      .pipe_cntr_total_packets_clear_fptr = clear_total_in_packets, \
      .pipe_cntr_uc_ct_clear_fptr = clear_uc_ct_count,              \
      .pipe_cntr_mc_ct_clear_fptr = clear_mc_ct_count,              \
      .pipe_defd_port_en_wr_fptr = defd_port_enable};

#define BF_TM_PIPE_HW_FTBL_RD_FUNCS(pipe_max_limit,                  \
                                    pipe_hyst,                       \
                                    total_cells,                     \
                                    total_in_pkts,                   \
                                    uc_ct,                           \
                                    mc_ct,                           \
                                    qstat_report,                    \
                                    defd_port_enable,                \
                                    defaults)                        \
  {                                                                  \
    g_pipe_hw_fptr_tbl.pipe_limit_rd_fptr = pipe_max_limit;          \
    g_pipe_hw_fptr_tbl.pipe_hyst_rd_fptr = pipe_hyst;                \
    g_pipe_hw_fptr_tbl.pipe_cntr_get_total_cells = total_cells;      \
    g_pipe_hw_fptr_tbl.pipe_cntr_get_total_pkts = total_in_pkts;     \
    g_pipe_hw_fptr_tbl.pipe_get_uc_ct_count = uc_ct;                 \
    g_pipe_hw_fptr_tbl.pipe_get_mc_ct_count = mc_ct;                 \
    g_pipe_hw_fptr_tbl.qstat_report_mode_rd_fptr = qstat_report;     \
    g_pipe_hw_fptr_tbl.pipe_defd_port_en_rd_fptr = defd_port_enable; \
    g_pipe_hw_fptr_tbl.pipe_get_defaults_fptr = defaults;            \
  }

static void bf_tm_pipe_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_WR_FUNCS(bf_tm_tofino_set_egress_pipe_max_limit,
                                bf_tm_tofino_set_egress_pipe_hyst,
                                NULL,
                                bf_tm_tofino_pipe_clear_total_in_cells,
                                bf_tm_tofino_pipe_clear_total_in_pkts,
                                bf_tm_tofino_pipe_clear_uc_ct_count,
                                bf_tm_tofino_pipe_clear_mc_ct_count,
                                NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_WR_FUNCS(bf_tm_tof2_set_egress_pipe_max_limit,
                                bf_tm_tof2_set_egress_pipe_hyst,
                                bf_tm_tof2_set_qac_qstat_report_mode,
                                NULL,
                                bf_tm_tof2_pipe_clear_total_in_pkts,
                                bf_tm_tof2_pipe_clear_uc_ct_count,
                                bf_tm_tof2_pipe_clear_mc_ct_count,
                                bf_tm_tof2_pipe_set_deflection_port_enable);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_WR_FUNCS(bf_tm_tof3_set_egress_pipe_max_limit,
                                bf_tm_tof3_set_egress_pipe_hyst,
                                bf_tm_tof3_set_qac_qstat_report_mode,
                                NULL,
                                bf_tm_tof3_pipe_clear_total_in_pkts,
                                bf_tm_tof3_pipe_clear_uc_ct_count,
                                bf_tm_tof3_pipe_clear_mc_ct_count,
                                bf_tm_tof3_pipe_set_deflection_port_enable);
  }












  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}
static void bf_tm_pipe_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_RD_FUNCS(bf_tm_tofino_get_egress_pipe_max_limit,
                                bf_tm_tofino_get_egress_pipe_hyst,
                                bf_tm_tofino_pipe_get_total_in_cells,
                                bf_tm_tofino_pipe_get_total_in_pkts,
                                bf_tm_tofino_pipe_get_uc_ct_count,
                                bf_tm_tofino_pipe_get_mc_ct_count,
                                NULL,
                                NULL,
                                bf_tm_tofino_pipe_get_defaults);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_RD_FUNCS(bf_tm_tof2_get_egress_pipe_max_limit,
                                bf_tm_tof2_get_egress_pipe_hyst,
                                NULL,
                                bf_tm_tof2_pipe_get_total_in_pkts,
                                bf_tm_tof2_pipe_get_uc_ct_count,
                                bf_tm_tof2_pipe_get_mc_ct_count,
                                bf_tm_tof2_get_qac_qstat_report_mode,
                                bf_tm_tof2_pipe_get_deflection_port_enable,
                                bf_tm_tof2_pipe_get_defaults);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_PIPE_HW_FTBL_RD_FUNCS(bf_tm_tof3_get_egress_pipe_max_limit,
                                bf_tm_tof3_get_egress_pipe_hyst,
                                NULL,
                                bf_tm_tof3_pipe_get_total_in_pkts,
                                bf_tm_tof3_pipe_get_uc_ct_count,
                                bf_tm_tof3_pipe_get_mc_ct_count,
                                bf_tm_tof3_get_qac_qstat_report_mode,
                                bf_tm_tof3_pipe_get_deflection_port_enable,
                                bf_tm_tof3_pipe_get_defaults);
  }













  else if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

void bf_tm_pipe_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_PIPE_HW_FTBL_WR_FUNCS(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  BF_TM_PIPE_HW_FTBL_RD_FUNCS(
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

void bf_tm_pipe_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_pipe_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_pipe_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_pipe_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_pipe_null_hw_ftbl(tm_ctx);
  bf_tm_pipe_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_pipe_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->pipes) {
    bf_sys_free(tm_ctx->pipes);
    tm_ctx->pipes = NULL;
  }
}

bf_tm_status_t bf_tm_init_pipe(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_eg_pipe_t *pipe;
  int i;
  bf_dev_pipe_t p = 0;

  // Allocate #ports * pipe
  tm_ctx->pipes =
      bf_sys_calloc(1, sizeof(bf_tm_eg_pipe_t) * tm_ctx->tm_cfg.pipe_cnt);
  if (!tm_ctx->pipes) {
    return (BF_NO_SYS_RESOURCES);
  }

  pipe = tm_ctx->pipes;
  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(tm_ctx->devid, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if (lld_sku_map_pipe_id_to_phy_pipe_id(tm_ctx->devid, i, &p) != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          tm_ctx->devid,
          i);
    }
    pipe->p_pipe = p;
    pipe->l_pipe = i;
    pipe->epipe_resume_limit = 0;  // Hysteresis is default to zero
    pipe++;
  }

  bf_tm_pipe_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
