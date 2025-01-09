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


#ifndef __TM_TOF3_DEFAULT_H__
#define __TM_TOF3_DEFAULT_H__

#include <stddef.h>
#include <assert.h>
#include <unistd.h>

#include "traffic_mgr/common/tm_error.h"
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include "traffic_mgr/common/tm_hw_access.h"

#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <lld/lld_dr_if.h>

/*
 *  This file implements initializing TM with default setting.
 *  Defaults are set at the device init time (device-add)
 */

#include "tm_tof3.h"

#define BF_TM_PKT_SIZE_FOR_DEFAULT_CARVING \
  (1600) * (2)  // 2 pkts worth of default carving
#define BF_TM_MAX_ICOS_PER_PPG (8)
#define BF_TM_CELLS_FOR_2_PKT \
  (BF_TM_PKT_SIZE_FOR_DEFAULT_CARVING / BF_TM_TOF3_CELL_SIZE)

#define BF_TM_PPG_ICOS_RATIO (BF_TM_MAX_ICOS_PER_PPG / (2))

/* Ingress TM Carving */
#define BF_TM_SKID_POOL_SIZE (0)

/* Gmin size of default PPG */
#define BF_TM_DEFAULT_PPG_LMT (BF_TM_CELLS_FOR_2_PKT * BF_TM_PPG_ICOS_RATIO)

/* Assign 2 pkt worth of cells for PPG 0 - 255 / non-default PPGs */
#define BF_TM_PPG_LMT (BF_TM_DEFAULT_PPG_LMT) / (BF_TM_PPG_ICOS_RATIO)

// size = 1600B worth of cells for all ports
#define BF_TM_NEG_MIRROR_POOL_SIZE (20) * (BF_TM_TOF3_PORTS_PER_PIPE)

#define BF_TM_EG_PRE_FIFO_LIMIT 0xC0

#define BF_TM_IG_GLB_MIN_LIMIT 0x6000

/* Gmin pool size */
#define BF_TM_IG_GMIN_POOL_SIZE                                              \
  ((BF_TM_DEFAULT_PPG_LMT + BF_TM_PPG_LMT) * BF_TM_TOF3_TOTAL_PPG_PER_PIPE * \
   BF_TM_TOF3_MAU_PIPES)
/* Ingress TM App pool size */
#define BF_TM_IG_APP_POOL_0_SIZE                                               \
  (BF_TM_TOF3_BUFFER_CELLS - (BF_TM_IG_GMIN_POOL_SIZE + BF_TM_SKID_POOL_SIZE - \
                              BF_TM_NEG_MIRROR_POOL_SIZE)) /                   \
      (BF_TM_TOF3_APP_POOLS)
/* PPG base limit in APP pool 0 */
#define BF_TM_APP_POOL_0_PPG_BASE_LIMIT \
  (BF_TM_IG_APP_POOL_0_SIZE) / (BF_TM_TOF3_DEFAULT_PPG_PER_PIPE)

#define BF_TM_UC_CT_POOL_SIZE (0x200)  // Power on default value
#define BF_TM_MC_CT_POOL_SIZE \
  (0x6000)  // Power on default value. 0x1FFF
            // cells needed at minimum for MC
            // performance. jira TOFLAB-36

/* Egress TM Carving */
#define BF_TM_Q_GMIN_LMT ((2) * (BF_TM_CELLS_FOR_2_PKT))

/* Gmin pool size on egress TM */
#define BF_TM_EG_GMIN_POOL_SIZE \
  ((BF_TM_Q_GMIN_LMT)*BF_TM_TOF3_TOTAL_QUEUES_PER_PIPE * BF_TM_TOF3_MAU_PIPES)
/* Egress TM App pool size */
#define BF_TM_EG_APP_POOL_0_SIZE                        \
  ((BF_TM_TOF3_BUFFER_CELLS - BF_TM_EG_GMIN_POOL_SIZE - \
    BF_TM_UC_CT_POOL_SIZE - BF_TM_MC_CT_POOL_SIZE -     \
    BF_TM_NEG_MIRROR_POOL_SIZE) /                       \
   (BF_TM_TOF3_APP_POOLS))
/* Q base limit in APP pool 0 */
#define BF_TM_APP_POOL_0_Q_BASE_LIMIT \
  (BF_TM_EG_APP_POOL_0_SIZE) / (BF_TM_TOF3_TOTAL_QUEUES_PER_PIPE)

#define BF_TM_EG_APP_POOL_3_MC_SIZE (0x3000)

#define BF_TM_Q_SCH_BURST_SIZE (16384)
#define BF_TM_PORT_SCH_BURST_SIZE (9216)

#define BF_TM_L1_SCH_BURST_SIZE (32640)
#define BF_TM_L1_SCH_DEFAULT_RATE (0x18687958)

/* Define this when testing TM hitless HA with model.
 * When testing TM hitless HA case with chip, the macro needs to be undefined.
 * For now work with model as default case.
 */

// Special build/compile time macro to unit test TM hitless cfg verification
// using MODEL. With chip/asic, unit can be done without rebuilding after
// enabling following macro.
// To trigger TM config restore and verification (unit-test), use CLI command
//  ***  module traffic_mgr ut_hitless_cfg ***

//#define BF_TM_HITLESS_HA_TESTING_WITH_MODEL

#define BF_TM_IFG_COMPENSATION (20)

#define BF_TM_IG_GLB_CELL_LIMIT 0x3E800

bf_status_t bf_tm_tof3_ddr_init_seq(bf_dev_id_t dev, bf_subdev_id_t subdev_id);

#endif
