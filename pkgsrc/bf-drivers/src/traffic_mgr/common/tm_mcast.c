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
#include "tm_mcast.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>

static bf_tm_mcast_hw_funcs_tbl g_mcast_hw_fptr_tbl;

bf_status_t bf_tm_mcast_set_fifo_arb_mode(bf_dev_id_t dev,
                                          uint8_t pipe_bmap,
                                          int fifo,
                                          bool arb_mode_strict_prio) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;
  int i, j;
  uint32_t all_fifo_arbmode;
  uint32_t num_pipes;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if ((1 << i) & pipe_bmap) {
      if (arb_mode_strict_prio) {
        // Setting fifo strict prio...
        // Set all higher fifos also strict prio.
        for (j = fifo; j < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe; j++) {
          BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, j)->arb_mode =
              arb_mode_strict_prio;
        }
      } else {
        // Setting fifo wrr...
        // Set all lower fifos also wrr.
        for (j = fifo; j >= 0; j--) {
          BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, j)->arb_mode =
              arb_mode_strict_prio;
        }
      }
    }
  }
  if (g_mcast_hw_fptr_tbl.fifo_prio_wr_fptr) {
    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.pipe_cnt; i++) {
      if ((1 << i) & pipe_bmap) {
        all_fifo_arbmode = 0;
        for (j = 0; j < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe; j++) {
          all_fifo_arbmode |=
              (((BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, j)->arb_mode) ? 1 : 0)
               << j);
        }
        lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &mcast_fifo.phy_pipe),
            mcast_fifo.arb_mode = all_fifo_arbmode;
        rc = g_mcast_hw_fptr_tbl.fifo_prio_wr_fptr(dev, &mcast_fifo);
        if (BF_TM_IS_NOTOK(rc)) {
          LOG_ERROR(
              "Unable to program Mcast fifo arbitration mode. "
              "Device = %d Logical pipe = %d, fifo = %d",
              dev,
              i,
              fifo);
        }
      }
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_get_fifo_arb_mode(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          int fifo,
                                          bool *sw_arb_mode,
                                          bool *hw_arb_mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;

  *sw_arb_mode = BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], pipe, fifo)->arb_mode;

  if (hw_arb_mode && g_mcast_hw_fptr_tbl.fifo_prio_rd_fptr) {
    lld_sku_map_pipe_id_to_phy_pipe_id(dev, pipe, &mcast_fifo.phy_pipe),
        rc = g_mcast_hw_fptr_tbl.fifo_prio_rd_fptr(dev, &mcast_fifo);
    if (BF_TM_IS_OK(rc)) {
      *hw_arb_mode = (((mcast_fifo.arb_mode) >> fifo) & 0x1) ? true : false;
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_set_fifo_wrr_weight(bf_dev_id_t dev,
                                            uint8_t pipe_bmap,
                                            int fifo,
                                            uint8_t weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;
  uint32_t all_fifo_weight;
  int i, j;
  uint32_t num_pipes;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if ((1 << i) & pipe_bmap) {
      BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, fifo)->weight = weight;
    }
  }
  if (g_mcast_hw_fptr_tbl.fifo_weight_wr_fptr) {
    for (i = 0; i < (int)num_pipes; i++) {
      if ((1 << i) & pipe_bmap) {
        all_fifo_weight = 0;
        for (j = 0; j < g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe; j++) {
          all_fifo_weight |=
              ((BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, j)->weight) << (8 * j));
        }
        lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &mcast_fifo.phy_pipe),
            mcast_fifo.weight = all_fifo_weight;
        rc = g_mcast_hw_fptr_tbl.fifo_weight_wr_fptr(dev, &mcast_fifo);
        if (BF_TM_IS_NOTOK(rc)) {
          LOG_ERROR(
              "Unable to program Mcast fifo wrr weight. "
              "Device = %d Logical pipe = %d, fifo = %d",
              dev,
              i,
              fifo);
        }
      }
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_get_fifo_wrr_weight(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            int fifo,
                                            uint8_t *sw_weight,
                                            uint8_t *hw_weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;

  *sw_weight = BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], pipe, fifo)->weight;

  if (hw_weight && g_mcast_hw_fptr_tbl.fifo_weight_rd_fptr) {
    lld_sku_map_pipe_id_to_phy_pipe_id(dev, pipe, &mcast_fifo.phy_pipe),
        rc = g_mcast_hw_fptr_tbl.fifo_weight_rd_fptr(dev, &mcast_fifo);
    if (BF_TM_IS_OK(rc)) {
      *hw_weight = ((mcast_fifo.weight) >> (8 * fifo) & 0xff);
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_set_fifo_icos_mapping(bf_dev_id_t dev,
                                              uint8_t pipe_bmap,
                                              int fifo,
                                              uint8_t icos_bmap) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  bf_tm_mcast_fifo_t mcast_fifo;
  uint32_t num_pipes;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if ((1 << i) & pipe_bmap) {
      BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, fifo)->icos_bmap = icos_bmap;
    }
  }
  if (g_mcast_hw_fptr_tbl.fifo_icosmap_wr_fptr) {
    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.pipe_cnt; i++) {
      if ((1 << i) & pipe_bmap) {
        lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &mcast_fifo.phy_pipe),
            mcast_fifo.icos_bmap = icos_bmap;
        mcast_fifo.fifo = fifo;
        rc = g_mcast_hw_fptr_tbl.fifo_icosmap_wr_fptr(dev, &mcast_fifo);
        if (BF_TM_IS_NOTOK(rc)) {
          LOG_ERROR(
              "Unable to program Mcast fifo icos mapping. "
              "Device = %d Logical pipe = %d, fifo = %d",
              dev,
              i,
              fifo);
        }
      }
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_get_fifo_icos_mapping(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              int fifo,
                                              uint8_t *sw_icosbmap,
                                              uint8_t *hw_icosbmap) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;

  *sw_icosbmap = BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], pipe, fifo)->icos_bmap;

  if (hw_icosbmap && g_mcast_hw_fptr_tbl.fifo_icosmap_rd_fptr) {
    mcast_fifo.fifo = fifo;
    lld_sku_map_pipe_id_to_phy_pipe_id(dev, pipe, &mcast_fifo.phy_pipe),
        rc = g_mcast_hw_fptr_tbl.fifo_icosmap_rd_fptr(dev, &mcast_fifo);
    if (BF_TM_IS_OK(rc)) {
      *hw_icosbmap = mcast_fifo.icos_bmap;
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_set_fifo_depth(bf_dev_id_t dev,
                                       uint8_t pipe_bmap,
                                       int fifo,
                                       uint16_t depth) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;
  int i;
  uint32_t num_pipes;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if ((1 << i) & pipe_bmap) {
      BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], i, fifo)->size = depth;
    }
  }
  if (g_mcast_hw_fptr_tbl.fifo_depth_wr_fptr) {
    for (i = 0; i < (int)num_pipes; i++) {
      if ((1 << i) & pipe_bmap) {
        mcast_fifo.fifo = fifo;
        mcast_fifo.size = depth;
        lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &mcast_fifo.phy_pipe),
            rc = g_mcast_hw_fptr_tbl.fifo_depth_wr_fptr(dev, &mcast_fifo);
        if (BF_TM_IS_NOTOK(rc)) {
          LOG_ERROR(
              "Unable to program Mcast fifo depth. "
              "Device = %d Logical pipe = %d, fifo = %d",
              dev,
              i,
              fifo);
        }
      }
    }
  }
  return (rc);
}

bf_status_t bf_tm_mcast_get_fifo_depth(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       int fifo,
                                       uint16_t *sw_depth,
                                       uint16_t *hw_depth) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_fifo_t mcast_fifo;

  *sw_depth = BF_TM_MC_FIFO_PTR(g_tm_ctx[dev], pipe, fifo)->size;

  if (hw_depth && g_mcast_hw_fptr_tbl.fifo_depth_rd_fptr) {
    lld_sku_map_pipe_id_to_phy_pipe_id(dev, pipe, &mcast_fifo.phy_pipe),
        mcast_fifo.fifo = fifo;
    rc = g_mcast_hw_fptr_tbl.fifo_depth_rd_fptr(dev, &mcast_fifo);
    if (BF_TM_IS_OK(rc)) {
      *hw_depth = mcast_fifo.size;
    }
  }
  return (rc);
}

#define BF_TM_MCAST_HW_FTBL_WR_FUNCS(                          \
    fifo_arbmode, fifo_wrr_wt, fifo_icos_bmap, fifo_depth)     \
  {                                                            \
    g_mcast_hw_fptr_tbl.fifo_prio_wr_fptr = fifo_arbmode;      \
    g_mcast_hw_fptr_tbl.fifo_weight_wr_fptr = fifo_wrr_wt;     \
    g_mcast_hw_fptr_tbl.fifo_icosmap_wr_fptr = fifo_icos_bmap; \
    g_mcast_hw_fptr_tbl.fifo_depth_wr_fptr = fifo_depth;       \
  }

#define BF_TM_MCAST_HW_FTBL_RD_FUNCS(                          \
    fifo_arbmode, fifo_wrr_wt, fifo_icos_bmap, fifo_depth)     \
  {                                                            \
    g_mcast_hw_fptr_tbl.fifo_prio_rd_fptr = fifo_arbmode;      \
    g_mcast_hw_fptr_tbl.fifo_weight_rd_fptr = fifo_wrr_wt;     \
    g_mcast_hw_fptr_tbl.fifo_icosmap_rd_fptr = fifo_icos_bmap; \
    g_mcast_hw_fptr_tbl.fifo_depth_rd_fptr = fifo_depth;       \
  }

void bf_tm_mcast_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_MCAST_HW_FTBL_WR_FUNCS(NULL, NULL, NULL, NULL);
  BF_TM_MCAST_HW_FTBL_RD_FUNCS(NULL, NULL, NULL, NULL);
}

static void bf_tm_mcast_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_WR_FUNCS(bf_tm_tofino_set_mcast_fifo_arbmode,
                                 bf_tm_tofino_set_mcast_fifo_wrr_weight,
                                 bf_tm_tofino_set_mcast_fifo_icos_bmap,
                                 bf_tm_tofino_set_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_WR_FUNCS(bf_tm_tof2_set_mcast_fifo_arbmode,
                                 bf_tm_tof2_set_mcast_fifo_wrr_weight,
                                 bf_tm_tof2_set_mcast_fifo_icos_bmap,
                                 bf_tm_tof2_set_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_WR_FUNCS(bf_tm_tof3_set_mcast_fifo_arbmode,
                                 bf_tm_tof3_set_mcast_fifo_wrr_weight,
                                 bf_tm_tof3_set_mcast_fifo_icos_bmap,
                                 bf_tm_tof3_set_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

static void bf_tm_mcast_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_RD_FUNCS(bf_tm_tofino_get_mcast_fifo_arbmode,
                                 bf_tm_tofino_get_mcast_fifo_wrr_weight,
                                 bf_tm_tofino_get_mcast_fifo_icos_bmap,
                                 bf_tm_tofino_get_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_RD_FUNCS(bf_tm_tof2_get_mcast_fifo_arbmode,
                                 bf_tm_tof2_get_mcast_fifo_wrr_weight,
                                 bf_tm_tof2_get_mcast_fifo_icos_bmap,
                                 bf_tm_tof2_get_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_MCAST_HW_FTBL_RD_FUNCS(bf_tm_tof3_get_mcast_fifo_arbmode,
                                 bf_tm_tof3_get_mcast_fifo_wrr_weight,
                                 bf_tm_tof3_get_mcast_fifo_icos_bmap,
                                 bf_tm_tof3_get_mcast_fifo_depth);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

void bf_tm_mcast_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_mcast_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_mcast_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_mcast_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_mcast_null_hw_ftbl(tm_ctx);
  bf_tm_mcast_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_mcast_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->mcast_fifo) {
    bf_sys_free(tm_ctx->mcast_fifo);
    tm_ctx->mcast_fifo = NULL;
  }
}

bf_tm_status_t bf_tm_init_mcast(bf_tm_dev_ctx_t *tm_ctx) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_mcast_t *mc_fifo;
  int j, i;
  uint32_t num_pipes;
  bf_dev_pipe_t p = 0;

  tm_ctx->mcast_fifo =
      bf_sys_calloc(tm_ctx->tm_cfg.pipe_cnt * tm_ctx->tm_cfg.pre_fifo_per_pipe,
                    sizeof(bf_tm_mcast_t));

  if (!tm_ctx->mcast_fifo) {
    return (BF_NO_SYS_RESOURCES);
  }

  mc_fifo = tm_ctx->mcast_fifo;

  lld_sku_get_num_active_pipes(tm_ctx->devid, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    rc = lld_sku_map_pipe_id_to_phy_pipe_id(tm_ctx->devid, i, &p);
    if (rc != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          tm_ctx->devid,
          i);
      return (rc);
    }
    for (j = 0; j < tm_ctx->tm_cfg.pre_fifo_per_pipe; j++) {
      mc_fifo->phy_pipe = p;
      mc_fifo->fifo = j;
      mc_fifo->l_pipe = i;
      mc_fifo++;
    }
  }
  bf_tm_mcast_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
