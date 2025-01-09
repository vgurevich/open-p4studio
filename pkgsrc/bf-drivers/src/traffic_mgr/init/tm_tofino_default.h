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


#ifndef __TM_TOFINO_DEFAULT_H__
#define __TM_TOFINO_DEFAULT_H__

#include <stddef.h>
#include <unistd.h>

#include "traffic_mgr/common/tm_error.h"
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/common/tm_hw_access.h"

#include <tofino_regs/tofino.h>
#include <lld/lld_dr_if.h>

/*
 *  This file implements initializing TM with default setting.
 *  Defaults are set at the device init time (device-add)
 */

#include "tm_tofino.h"

#define BF_TM_TOFINO_TOTAL_CELLS_FOR_DEFAULT_CARVING \
  (100 * 1024)  // 100K cells for all Tofino SKUs

#define BF_TM_PKT_SIZE_FOR_DEFAULT_CARVING \
  (1600) * (2)  // 2 pkts worth of default carving
#define BF_TM_MAX_ICOS_PER_PPG (8)
#define BF_TM_CELLS_FOR_2_PKT \
  (BF_TM_PKT_SIZE_FOR_DEFAULT_CARVING / BF_TM_TOFINO_CELL_SIZE)

#define BF_TM_PPG_ICOS_RATIO (BF_TM_MAX_ICOS_PER_PPG / (2))

/* Ingress TM Carving */
#define BF_TM_SKID_POOL_SIZE (0)

/* Gmin size of default PPG */
#define BF_TM_DEFAULT_PPG_LMT (BF_TM_CELLS_FOR_2_PKT)

// size = 1600B worth of cells for all ports
#define BF_TM_NEG_MIRROR_POOL_SIZE (20) * (BF_TM_TOFINO_PORTS_PER_PIPE)

/* Gmin pool size */
#define BF_TM_IG_DEFAULT_PPG_GMIN_POOL_SIZE                    \
  (BF_TM_DEFAULT_PPG_LMT * BF_TM_TOFINO_DEFAULT_PPG_PER_PIPE * \
   BF_TM_TOFINO_MAU_PIPES)
/* Ingress TM App pool size */
#ifdef BF_SLT
/* SLT change: Use high Ingress AP pool0 size */
#define BF_TM_IG_APP_POOL_0_SIZE (210 * 1024)
#else
#define BF_TM_IG_APP_POOL_0_SIZE                                 \
  (BF_TM_TOFINO_TOTAL_CELLS_FOR_DEFAULT_CARVING -                \
   (BF_TM_IG_DEFAULT_PPG_GMIN_POOL_SIZE + BF_TM_SKID_POOL_SIZE + \
    BF_TM_NEG_MIRROR_POOL_SIZE))
#endif

/* PPG base limit in APP pool 0 */
#define BF_TM_APP_POOL_0_PPG_BASE_LIMIT \
  (BF_TM_IG_APP_POOL_0_SIZE) /          \
      (BF_TM_TOFINO_DEFAULT_PPG_PER_PIPE * BF_TM_TOFINO_MAU_PIPES)

#define BF_TM_UC_CT_POOL_SIZE_REV_A0 (0x200)  // Power on default value
#define BF_TM_MC_CT_POOL_SIZE_REV_A0 \
  (0x1FFF)  // Power on default value. 0x1FFF
            // cells needed at minimum for MC
            // performance. jira TOFLAB-36

// Default UC CT size for rev B0 and later parts
#define BF_TM_UC_CT_POOL_SIZE (0x1000)
// Default MC CT size for rev B0 and later parts
#define BF_TM_MC_CT_POOL_SIZE (0x2000)

/* Egress TM Carving */
#define BF_TM_Q_GMIN_LMT \
  ((BF_TM_CELLS_FOR_2_PKT) / (2))  // Cells for one packet

/* Gmin pool size on egress TM */
#define BF_TM_EG_GMIN_POOL_SIZE                            \
  ((BF_TM_Q_GMIN_LMT)*BF_TM_TOFINO_TOTAL_QUEUES_PER_PIPE * \
   BF_TM_TOFINO_MAU_PIPES)

/* Egress TM App pool size for rev A0*/
#ifdef BF_SLT
/* SLT change: Use high Egress AP pool0 size */
#define BF_TM_EG_APP_POOL_0_SIZE_REV_A0 (190 * 1024)
#else
#define BF_TM_EG_APP_POOL_0_SIZE_REV_A0                           \
  (BF_TM_TOFINO_TOTAL_CELLS_FOR_DEFAULT_CARVING -                 \
   (BF_TM_EG_GMIN_POOL_SIZE + BF_TM_EG_APP_POOL_3_MC_SIZE +       \
    BF_TM_UC_CT_POOL_SIZE_REV_A0 + BF_TM_MC_CT_POOL_SIZE_REV_A0 + \
    BF_TM_NEG_MIRROR_POOL_SIZE))
#endif

/* Q base limit in APP pool 0 for rev A0 */
#define BF_TM_APP_POOL_0_Q_BASE_LIMIT_REV_A0 \
  (BF_TM_EG_APP_POOL_0_SIZE_REV_A0) /        \
      (BF_TM_TOFINO_TOTAL_QUEUES_PER_PIPE * BF_TM_TOFINO_MAU_PIPES)

/* Egress TM App pool size for B0 and later parts */
#ifdef BF_SLT
/* SLT change: Use high Egress AP pool0 size */
#define BF_TM_EG_APP_POOL_0_SIZE (190 * 1024)
#else
#define BF_TM_EG_APP_POOL_0_SIZE                            \
  (BF_TM_TOFINO_TOTAL_CELLS_FOR_DEFAULT_CARVING -           \
   (BF_TM_EG_GMIN_POOL_SIZE + BF_TM_EG_APP_POOL_3_MC_SIZE + \
    BF_TM_UC_CT_POOL_SIZE + BF_TM_MC_CT_POOL_SIZE +         \
    BF_TM_NEG_MIRROR_POOL_SIZE))
#endif

/* Q base limit in APP pool 0 for B0 and later parts */
#define BF_TM_APP_POOL_0_Q_BASE_LIMIT \
  (BF_TM_EG_APP_POOL_0_SIZE) /        \
      (BF_TM_TOFINO_TOTAL_QUEUES_PER_PIPE * BF_TM_TOFINO_MAU_PIPES)

/*
 * As PRE FIFO uses egress AP pool3 on POR and we don't change it
 * (neither drivers default nor through exposed API), set egress AP pool3
 * size to 12K (0x3000) cells by default so MC would work fine
 * even if there is burst.
 *
 * Even if MC feature is not used, minimum egress AP pool3 size should be
 * atleast 1K cells as flooded packets go through PRE.
 */
#define BF_TM_EG_APP_POOL_3_MC_SIZE (12288)     // 12K
#define BF_TM_EG_APP_POOL_3_MC_MIN_SIZE (1024)  // 1K

#define BF_TM_Q_SCH_BURST_SIZE (16384)
#define BF_TM_PORT_SCH_BURST_SIZE (9216)

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

#endif
