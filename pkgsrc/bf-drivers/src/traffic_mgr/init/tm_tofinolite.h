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

#ifndef __TM_TOFINOLITE_H__
#define __TM_TOFINOLITE_H__

#include "traffic_mgr/common/tm_ctx.h"

/* TOFINOLITE Hardware Resources */
#define BF_TM_TOFINOLITE_BUFFER_CELLS (280 * 1024)
#define BF_TM_TOFINOLITE_CELL_SIZE (80)
#define BF_TM_TOFINOLITE_PG_PER_PIPE (18)
#define BF_TM_TOFINOLITE_PORTS_PER_PG (4)
#define BF_TM_TOFINOLITE_PORTS_PER_PIPE                           \
  (BF_TM_TOFINOLITE_PG_PER_PIPE * BF_TM_TOFINOLITE_PORTS_PER_PG + \
   1)  // +! mirror port
#define BF_TM_TOFINOLITE_MAU_PIPES (4)
#define BF_TM_TOFINOLITE_PPG_PER_PIPE (256)
#define BF_TM_TOFINOLITE_DEFAULT_PPG_PER_PIPE (BF_TM_TOFINOLITE_PORTS_PER_PIPE)
#define BF_TM_TOFINOLITE_TOTAL_PPG_PER_PIPE \
  (BF_TM_TOFINOLITE_PPG_PER_PIPE + BF_TM_TOFINOLITE_DEFAULT_PPG_PER_PIPE)

#define BF_TM_TOFINOLITE_PFC_LEVELS BF_TM_MAX_PFC_LEVELS

#define BF_TM_TOFINOLITE_APP_POOLS (4)
#define BF_TM_TOFINOLITE_QUEUES_PER_PG (32)
#define BF_TM_TOFINOLITE_MIN_QUEUES_PER_PORT (8)
#define BF_TM_TOFINOLITE_TOTAL_QUEUES_PER_PIPE \
  (BF_TM_TOFINOLITE_QUEUES_PER_PG * BF_TM_TOFINOLITE_PORTGROUP)  // 576
#define BF_TM_TOFINOLITE_RESUME_PROFILES (32)

#define BF_TM_TOFINOLITE_PERCENTAGE_LIMIT(limit) ((limit) >> 24)
#define BF_TM_TOFINOLITE_CELL_LIMIT(limit) ((limit) & (0xffffff))

#endif
