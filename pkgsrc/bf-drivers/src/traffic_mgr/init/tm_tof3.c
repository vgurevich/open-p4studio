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


// This file contains code relevant to Tof2 version of RMT ASIC.
// When a new version of RMT ASIC need to be supported, make a copy
// of this file and change relevant hardware values.

#include <dvm/bf_drv_intf.h>
#include <lld/lld_efuse.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "tm_tof3.h"

void bf_tm_tof3_cfg(bf_tm_cfg_t *tm_cfg, bf_dev_id_t devid) {
  // common
  tm_cfg->rsvd_pool_cnt = 1;
  tm_cfg->skid_pool_cnt = 1;
  tm_cfg->shared_pool_cnt = BF_TM_TOF3_APP_POOLS;
  lld_err_t lld_err;
  uint32_t num_pipes;
  lld_err = lld_sku_get_num_active_pipes(devid, &num_pipes);
  if (lld_err != LLD_OK) {
    LOG_ERROR("%s: Not able to get num active pipes", __func__);
    /* Set num active pipes to default value and proceed */
    num_pipes = BF_TM_TOF3_MAU_PIPES;
  }
  tm_cfg->pipe_cnt = num_pipes;
  uint32_t pipe_disabled = lld_efuse_get_pipe_disable(devid);
  tm_cfg->active_pipe_mask = (~pipe_disabled) & 0xff;
  tm_cfg->total_cells = BF_TM_TOF3_BUFFER_CELLS;
  tm_cfg->cell_size = BF_TM_TOF3_CELL_SIZE;
  tm_cfg->pg_per_pipe = BF_TM_TOF3_PG_PER_PIPE;
  tm_cfg->ports_per_pipe = BF_TM_TOF3_PORTS_PER_PIPE;
  tm_cfg->ports_per_pg = BF_TM_TOF3_PORTS_PER_PG;
  tm_cfg->icos_count = BF_TM_TOF3_PFC_LEVELS;
  tm_cfg->mirror_port_start = BF_TM_TOF3_MIRROR_PORT_START;
  tm_cfg->mirror_port_cnt = BF_TM_TOF3_MIRROR_PORT_COUNT;

  // Ingress
  tm_cfg->ig_pool_cnt = BF_TM_TOF3_APP_POOLS;
  tm_cfg->total_ppg_per_pipe = BF_TM_TOF3_TOTAL_PPG_PER_PIPE;
  tm_cfg->pfc_ppg_per_pipe = BF_TM_TOF3_PPG_PER_PIPE;
  tm_cfg->ingress_bypass_en = false;

#ifdef TM_NO_RTL_SIM
  tm_cfg->gmin_pool = BF_TM_TOF3_APP_POOLS + 1;
#else
  tm_cfg->gmin_pool = 0;
#endif

  // Egress
  tm_cfg->eg_pool_cnt = BF_TM_TOF3_APP_POOLS;
  tm_cfg->q_per_pg = BF_TM_TOF3_QUEUES_PER_PG;
  tm_cfg->q_per_pipe = BF_TM_TOF3_TOTAL_QUEUES_PER_PIPE;
  tm_cfg->l1_per_pg = BF_TM_TOF3_SCH_L1_PER_PG;
  tm_cfg->l1_per_pipe = BF_TM_TOF3_TOTAL_SCH_L1_PER_PIPE;
  tm_cfg->q_prof_cnt = BF_TM_TOF3_Q_PROF_CNT;

  tm_cfg->pre_fifo_per_pipe = 4;
  tm_cfg->chnl_mult = 2;

  tm_cfg->uc_ct_max_cells = BF_TM_TOF3_UC_CT_MAX_CELLS;
}
