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


// This file contains code relevant to Tof3 version of RMT ASIC.
// When a new version of RMT ASIC need to be supported, make a copy
// of this file and change relevant hardware values.

#ifndef __TM_TOF3_H__
#define __TM_TOF3_H__

#include "traffic_mgr/common/tm_ctx.h"

/* TOF3 Hardware Resources */
#define BF_TM_TOF3_BUFFER_CELLS (384 * 1024)
#define BF_TM_TOF3_CELL_SIZE (256)
#define BF_TM_TOF3_PG_PER_PIPE (9)
#define BF_TM_TOF3_PORTS_PER_PG (4)
#define BF_TM_TOF3_PORTS_PER_PIPE                       \
  ((BF_TM_TOF3_PG_PER_PIPE * BF_TM_TOF3_PORTS_PER_PG) + \
   1)  // 36 + 1 mirror (ingress, but not egress) port
#define BF_TM_TOF3_MIRROR_PORT_START \
  (BF_TM_TOF3_PG_PER_PIPE * BF_TM_TOF3_PORTS_PER_PG)
#define BF_TM_TOF3_MIRROR_PORT_COUNT (1)
#define BF_TM_TOF3_MAU_PIPES (8)
#define BF_TM_TOF3_IG_PIPES (4)
#define BF_TM_TOF3_EG_PIPES (8)
#define BF_TM_TOF3_PPG_PER_PIPE (128)
#define BF_TM_TOF3_DEFAULT_PPG_PER_PIPE (BF_TM_TOF3_PORTS_PER_PIPE)
#define BF_TM_TOF3_TOTAL_PPG_PER_PIPE \
  (BF_TM_TOF3_PPG_PER_PIPE + BF_TM_TOF3_DEFAULT_PPG_PER_PIPE)

#define BF_TM_TOF3_PFC_LEVELS BF_TM_MAX_PFC_LEVELS

#define BF_TM_TOF3_APP_POOLS (4)
#define BF_TM_TOF3_QUEUES_PER_PG (64)
#define BF_TM_TOF3_HW_QUEUES_PER_PG (256)
#define BF_TM_TOF3_MIN_QUEUES_PER_PORT (16)
#define BF_TM_TOF3_TOTAL_QUEUES_PER_PIPE \
  (BF_TM_TOF3_QUEUES_PER_PG * BF_TM_TOF3_PG_PER_PIPE)  // 576
#define BF_TM_TOF3_SCH_L1_PER_PG (16)
#define BF_TM_TOF3_TOTAL_SCH_L1_PER_PIPE \
  (BF_TM_TOF3_SCH_L1_PER_PG * BF_TM_TOF3_PG_PER_PIPE)  // 144

#define BF_TM_TOF3_RESUME_PROFILES (32)

#define BF_TM_TOF3_PERCENTAGE_LIMIT(limit) ((limit) >> 24)
#define BF_TM_TOF3_CELL_LIMIT(limit) ((limit) & (0xffffff))

#define BF_TM_TOF3_WAC_RESET_HYSTERESIS (0x10)  // in 8 cells unit
#define BF_TM_TOF3_QAC_RESET_HYSTERESIS (0)     // in 8 cells unit
#define BF_TM_TOF3_WAC_POR_HYSTERESIS (4)       // in 8 cells unit
#define BF_TM_TOF3_QAC_POR_HYSTERESIS (4)       // in 8 cells unit
#define BF_TM_TOF3_WAC_DEFAULT_HYSTERESIS (BF_TM_TOF3_WAC_POR_HYSTERESIS * 8)
#define BF_TM_TOF3_QAC_DEFAULT_HYSTERESIS (BF_TM_TOF3_QAC_POR_HYSTERESIS * 8)
#define BF_TM_TOF3_HYSTERESIS_PROFILES (32)

#define BF_TM_TOF3_Q_PROF_CNT (288)

#define BF_TM_TOF3_UC_CT_MAX_CELLS (127)

#endif
