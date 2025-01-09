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


// This file contains code relevant to TofinoLite version of RMT ASIC.
// When a new version of RMT ASIC need to be supported, make a copy
// of this file and change relevant hardware values.

#include "traffic_mgr/common/tm_ctx.h"

#include "tm_tofinolite.h"

void bf_tm_tofinolite_cfg(bf_tm_cfg_t *tm_cfg) {
  // common
  tm_cfg->rsvd_pool_cnt = 1;
  tm_cfg->skid_pool_cnt = 1;
  tm_cfg->shared_pool_cnt = BF_TM_TOFINOLITE_APP_POOLS;
  tm_cfg->pipe_cnt = BF_TM_TOFINOLITE_MAU_PIPES;
  tm_cfg->active_pipe_mask = 0xf;
  tm_cfg->total_cells = BF_TM_TOFINOLITE_BUFFER_CELLS;
  tm_cfg->cell_size = BF_TM_TOFINOLITE_CELL_SIZE;
  tm_cfg->pg_per_pipe = BF_TM_TOFINOLITE_PG_PER_PIPE;
  tm_cfg->ports_per_pg = BF_TM_TOFINOLITE_PORTS_PER_PG;
  tm_cfg->icos_count = BF_TM_TOFINOLITE_PFC_LEVELS;
  tm_cfg->mirror_port_start = 72;
  tm_cfg->mirror_port_cnt = 1;

  // Ingress
  tm_cfg->ig_pool_cnt = BF_TM_TOFINOLITE_APP_POOLS;
  tm_cfg->total_ppg_per_pipe = BF_TM_TOFINOLITE_TOTAL_PPG_PER_PIPE;
  tm_cfg->pfc_ppg_per_pipe = BF_TM_TOFINOLITE_PPG_PER_PIPE;
  tm_cfg->ingress_bypass_en = false;

  // Egress
  tm_cfg->eg_pool_cnt = BF_TM_TOFINOLITE_APP_POOLS;
  tm_cfg->q_per_pg = BF_TM_TOFINOLITE_QUEUES_PER_PG;
}
