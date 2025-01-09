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


// This file contains code relevant to Tofino version of RMT ASIC.
// When a new version of RMT ASIC need to be supported, make a copy
// of this file and change relevant hardware values.

#include <bf_types/bf_types.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_efuse.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "tm_tofino.h"

void bf_tm_tofino_cfg(bf_tm_cfg_t *tm_cfg, bf_dev_id_t devid) {
  lld_err_t lld_err;
  uint32_t lld_sku, num_pipes;

  /* Get number of active pipes */
  lld_err = lld_sku_get_num_active_pipes(devid, &num_pipes);
  if (lld_err != LLD_OK) {
    LOG_ERROR("%s: Not able to get num active pipes", __func__);
    /* Set num active pipes to default value and proceed */
    num_pipes = BF_TM_TOFINO_MAU_PIPES;
  }

  /* Get Tofino SKU */
  lld_sku = lld_sku_get_sku(devid);
  LOG_TRACE("%s: Tofino SKU is %x", __func__, lld_sku);

  // common
  tm_cfg->rsvd_pool_cnt = 1;
  tm_cfg->skid_pool_cnt = 1;
  tm_cfg->shared_pool_cnt = BF_TM_TOFINO_APP_POOLS;
  tm_cfg->pipe_cnt = num_pipes;
  uint32_t pipe_disabled = lld_efuse_get_pipe_disable(devid);
  tm_cfg->active_pipe_mask = (~pipe_disabled) & 0xf;

  /*
   * Tofino SKUs num active pipes and TM buffer total cells:
   *     18D: 2 active pipes and 260K total cells
   *     32D: 2 active pipes and 260K total cells
   *     32Q: 4 active pipes and 260K total cells
   *     64Q: 4 active pipes and 280K total cells
   */
  if (num_pipes == BF_TM_TOFINO_MAU_PIPES && lld_sku != BFN_SKU_BFN77120) {
    /* 64Q */
    tm_cfg->total_cells = BF_TM_TOFINO_BUFFER_CELLS;
  } else {
    /* 18D/32D/32Q */
    tm_cfg->total_cells = BF_TM_TOFINOLITE_SKUS_BUFFER_CELLS;
  }
  LOG_TRACE("%s: Tofino TM total cells - %u", __func__, tm_cfg->total_cells);

  tm_cfg->cell_size = BF_TM_TOFINO_CELL_SIZE;
  tm_cfg->pg_per_pipe = BF_TM_TOFINO_PG_PER_PIPE;
  tm_cfg->ports_per_pipe = BF_TM_TOFINO_PORTS_PER_PIPE;
  tm_cfg->ports_per_pg = BF_TM_TOFINO_PORTS_PER_PG;
  tm_cfg->icos_count = BF_TM_TOFINO_PFC_LEVELS;
  tm_cfg->mirror_port_start = 72;
  tm_cfg->mirror_port_cnt = 1;

  // Ingress
  tm_cfg->ig_pool_cnt = BF_TM_TOFINO_APP_POOLS;
  tm_cfg->total_ppg_per_pipe = BF_TM_TOFINO_TOTAL_PPG_PER_PIPE;
  tm_cfg->pfc_ppg_per_pipe = BF_TM_TOFINO_PPG_PER_PIPE;
  tm_cfg->ingress_bypass_en = false;

#ifdef TM_NO_RTL_SIM
  tm_cfg->gmin_pool = BF_TM_TOFINO_APP_POOLS + 1;
#else
  tm_cfg->gmin_pool = 0;
#endif

  // Egress
  tm_cfg->eg_pool_cnt = BF_TM_TOFINO_APP_POOLS;
  tm_cfg->q_per_pg = BF_TM_TOFINO_QUEUES_PER_PG;
  tm_cfg->q_per_pipe = BF_TM_TOFINO_TOTAL_QUEUES_PER_PIPE;
  tm_cfg->q_prof_cnt = BF_TM_TOFINO_Q_PROF_CNT;

  tm_cfg->pre_fifo_per_pipe = 4;
  tm_cfg->chnl_mult = 1;

  tm_cfg->uc_ct_max_cells = BF_TM_TOFINO_UC_CT_MAX_CELLS;
}
