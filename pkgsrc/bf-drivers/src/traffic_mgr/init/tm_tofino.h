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

#ifndef __TM_TOFINO_H__
#define __TM_TOFINO_H__

#include "traffic_mgr/common/tm_ctx.h"

/* TOFINO Hardware Resources */
#define BF_TM_TOFINO_BUFFER_CELLS (280 * 1024)
#define BF_TM_TOFINOLITE_SKUS_BUFFER_CELLS (260 * 1024)
#define BF_TM_TOFINO_CELL_SIZE (80)
#define BF_TM_TOFINO_PG_PER_PIPE (18)
#define BF_TM_TOFINO_PORTS_PER_PG (4)
#define BF_TM_TOFINO_PORTS_PER_PIPE                         \
  ((BF_TM_TOFINO_PG_PER_PIPE * BF_TM_TOFINO_PORTS_PER_PG) + \
   1)  // +1 mirror port
#define BF_TM_TOFINO_MAU_PIPES (4)
#define BF_TM_TOFINO_PPG_PER_PIPE (128)
#define BF_TM_TOFINO_DEFAULT_PPG_PER_PIPE (BF_TM_TOFINO_PORTS_PER_PIPE)
#define BF_TM_TOFINO_TOTAL_PPG_PER_PIPE \
  (BF_TM_TOFINO_PPG_PER_PIPE + BF_TM_TOFINO_DEFAULT_PPG_PER_PIPE)

#define BF_TM_TOFINO_PFC_LEVELS BF_TM_MAX_PFC_LEVELS

#define BF_TM_TOFINO_APP_POOLS (4)
#define BF_TM_TOFINO_QUEUES_PER_PG (32)
#define BF_TM_TOFINO_MIN_QUEUES_PER_PORT (8)
#define BF_TM_TOFINO_TOTAL_QUEUES_PER_PIPE \
  (BF_TM_TOFINO_QUEUES_PER_PG * BF_TM_TOFINO_PG_PER_PIPE)  // 576

#define BF_TM_TOFINO_RESUME_PROFILES (32)

#define BF_TM_TOFINO_PERCENTAGE_LIMIT(limit) ((limit) >> 24)
#define BF_TM_TOFINO_CELL_LIMIT(limit) ((limit) & (0xffffff))

#define BF_TM_TOFINO_WAC_RESET_HYSTERESIS (0x10)  // in 8 cells unit
#define BF_TM_TOFINO_QAC_RESET_HYSTERESIS (0)     // in 8 cells unit
#define BF_TM_TOFINO_WAC_POR_HYSTERESIS (4)       // in 8 cells unit
#define BF_TM_TOFINO_QAC_POR_HYSTERESIS (4)       // in 8 cells unit
#define BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS \
  (BF_TM_TOFINO_WAC_POR_HYSTERESIS * 8)
#define BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS \
  (BF_TM_TOFINO_QAC_POR_HYSTERESIS * 8)
#define BF_TM_TOFINO_HYSTERESIS_PROFILES (32)

#define BF_TM_TOFINO_Q_PROF_CNT (16)

#define BF_TM_TOFINO_UC_CT_MAX_CELLS (15)

#endif
