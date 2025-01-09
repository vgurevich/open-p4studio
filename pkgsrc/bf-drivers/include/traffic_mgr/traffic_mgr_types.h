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


#ifndef __TRAFFIC_MGR_TYPES_H__
#define __TRAFFIC_MGR_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <bf_types/bf_types.h>
#include <tofino/pdfixed/pd_tm.h>

/**
 * @file traffic_mgr_types.h
 * \brief Details traffic manager specific types used by traffic manager APIs
 */

/**
 * @addtogroup tm-types
 * @{
 */

/** Internal CoS value used by Traffic manager. Supported range <0-7> */
typedef uint8_t bf_tm_icos_t;

/** PFC priority used by Traffic manager. Supported range <0-7> */
typedef uint8_t bf_tm_pfc_priority_t;

/** Priority flow controlled (PFC) port group used for Traffic Isolation.
 *  Upto 256 additional PPGs are supported in Tofino version
 *  The handle is encoded as 16bit port, 4bit Pipe, 12bit ppg
 */
typedef uint32_t bf_tm_ppg_hdl;

/**
 * Priority Port group (PPG) identifier in a pipe.
 * From 1 up to BF_TM_TOFINO_PPG_PER_PIPE or BF_TM_TOF2_PPG_PER_PIPE
 */
typedef uint8_t bf_tm_ppg_id_t;

/**
 * L1 nodes behind Port. Up to 32 L1 nodes per port group.
 */
typedef uint32_t bf_tm_l1_node_t;

/**
 * Queues behind L1 nodes. Up to 128 queues per port group.
 */
typedef uint32_t bf_tm_queue_t;

/**
 * Port group number in a pipe.
 * Up to BF_TM_TOFINO_PG_PER_PIPE or BF_TM_TOF2_PG_PER_PIPE
 */
typedef uint8_t bf_tm_pg_t;

/**
 * Number of cells as a threshold value.
 */
typedef uint32_t bf_tm_thres_t;

/**
 * Packet Color enumeration
 */
typedef enum {
  BF_TM_COLOR_GREEN = PD_COLOR_GREEN /** Enum to use for Green Color */,
  BF_TM_COLOR_YELLOW = PD_COLOR_YELLOW /** Enum to use for Yellow Color */,
  BF_TM_COLOR_RED = PD_COLOR_RED /** Enum to use for Red Color */,
} bf_tm_color_t;

/**
 * Enumeration of supported pool identifiers.
 * 1. Four application pools are supported in both ingress and egress
 *    direction. On ingress side, PPGs can be mapped to application pool
 *    BF_TM_IG_APP_POOL_0..3. On egress side queues can be mapped to
 *    application pools BF_TM_EG_APP_POOL_0..3
 * 2. Skid pool is a pool of PPGs that carry lossless traffic. When buffer
 *    space usage spills into skid pool, PFC is asserted.
 * 3. Negative mirror pool is used to mirror traffic that is experiencing
 *    drop. Buffers needed to accomodate traffic under drop comes from
 *    this pool. Size of this pool can be independently set for ingress
 *    and egress direction.
 */
typedef enum {
  BF_TM_IG_APP_POOL_0 /** Ingress Application Pool 0 */,
  BF_TM_IG_APP_POOL_1 /** Ingress Application Pool 1 */,
  BF_TM_IG_APP_POOL_2 /** Ingress Application Pool 2 */,
  BF_TM_IG_APP_POOL_3 /** Ingress Application Pool 3 */,














  BF_TM_EG_APP_POOL_0 /** Egress Application Pool 0 */,
  BF_TM_EG_APP_POOL_1 /** Egress Application Pool 1 */,
  BF_TM_EG_APP_POOL_2 /** Egress Application Pool 2 */,
  BF_TM_EG_APP_POOL_3 /** Egress Application Pool 3 */,














  BF_TM_APP_POOL_LAST
} bf_tm_app_pool_t;

/**
 * D(t) : Dynamic-PPG-Limit at time t.
 * Size : Size of Shared/Application Pool.
 * U(t) : In use cell count at time t. Cells used by all PPGs
 *        of the pool.
 * BAF  : One of the percentages listed in bf_tm_ppg_baf_t enum.
 *
 * D(t) = (Size - U(t)) * BAF
 */
typedef enum {
  BF_TM_PPG_BAF_1_POINT_5_PERCENT /** 1.5%  */,
  BF_TM_PPG_BAF_3_PERCENT /** 3%    */,
  BF_TM_PPG_BAF_6_PERCENT /** 6%    */,
  BF_TM_PPG_BAF_11_PERCENT /** 11%   */,
  BF_TM_PPG_BAF_20_PERCENT /** 20%   */,
  BF_TM_PPG_BAF_33_PERCENT /** 33%   */,
  BF_TM_PPG_BAF_50_PERCENT /** 50%   */,
  BF_TM_PPG_BAF_66_PERCENT /** 66%   */,
  BF_TM_PPG_BAF_80_PERCENT /** 80%   */,
  BF_TM_PPG_BAF_DISABLE /** No Burst Absorption  */,
} bf_tm_ppg_baf_t;

/**
 * D(t) : Dynamic-Q-Limit at time t.
 * Size : Size of Shared/Application Pool to which queue belongs to.
 * U(t) : In use cell count at time t. Cells used by all queues
 *        of the pool.
 * BAF  : One of the percentages listed in bf_tm_q_baf_en enum.
 *
 * D(t) = (Size - U(t)) * BAF
 */
typedef enum {
  BF_TM_Q_BAF_1_POINT_5_PERCENT /** 1.5%  */,
  BF_TM_Q_BAF_3_PERCENT /** 3%    */,
  BF_TM_Q_BAF_6_PERCENT /** 6%    */,
  BF_TM_Q_BAF_11_PERCENT /** 11%   */,
  BF_TM_Q_BAF_20_PERCENT /** 20%   */,
  BF_TM_Q_BAF_33_PERCENT /** 33%   */,
  BF_TM_Q_BAF_50_PERCENT /** 50%   */,
  BF_TM_Q_BAF_66_PERCENT /** 66%   */,
  BF_TM_Q_BAF_80_PERCENT /** 80%   */,
  BF_TM_Q_BAF_DISABLE /* If BAF is disabled, queue threshold is static. */,
} bf_tm_queue_baf_t;

typedef enum {
  /* App pool limits in cells */
  BF_TM_Q_BAF_LIMIT_128K = 128 * 1024,
  BF_TM_Q_BAF_LIMIT_256K = 256 * 1024,
  BF_TM_Q_BAF_LIMIT_384K = 384 * 1024
} bf_tm_queue_baf_limit_t;

/**
 * Color drop limits specified in terms of percentage of green color limits.
 * Green color limits by default is same as queue's minimum limit value.
 */
typedef enum {
  BF_TM_Q_COLOR_LIMIT_12_POINT_5_PERCENT /** 12.5% of green color limits */,
  BF_TM_Q_COLOR_LIMIT_25_PERCENT /** 25% of green color limits   */,
  BF_TM_Q_COLOR_LIMIT_37_POINT_5_PERCENT /** 37.5% of green color limits */,
  BF_TM_Q_COLOR_LIMIT_50_PERCENT /** 50% of green color limits   */,
  BF_TM_Q_COLOR_LIMIT_62_POINT_5_PERCENT /** 62.5% of green color limits */,
  BF_TM_Q_COLOR_LIMIT_75_PERCENT /** 75% of green color limits   */,
  BF_TM_Q_COLOR_LIMIT_87_POINT_5_PERCENT /** 87% of green color limits   */,
  BF_TM_Q_COLOR_LIMIT_100_PERCENT /** 100% of green color limits  */,
} bf_tm_queue_color_limit_t;

/**
 * Various pause mode supported on ingress TM
 */
typedef enum {
  BF_TM_PAUSE_NONE /** No FlowControl */,
  BF_TM_PAUSE_PFC /** FlowControl Type PFC */,
  BF_TM_PAUSE_PORT /** FlowControl Type, Port */
} bf_tm_flow_ctrl_type_t;

/**
 * Enum for assigning scheudling priority
 */

typedef enum {
  BF_TM_SCH_PRIO_LOW /**  */,
  BF_TM_SCH_PRIO_0 /** Scheduling Priority (Low) */ = BF_TM_SCH_PRIO_LOW,
  BF_TM_SCH_PRIO_1 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_2 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_3 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_4 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_5 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_6 /**  One of eight scheduling priority */,
  BF_TM_SCH_PRIO_7 /**  Scheduling Priority (High) */,
  BF_TM_SCH_PRIO_HIGH = BF_TM_SCH_PRIO_7,
} bf_tm_sched_prio_t;

/**
 * Enum for assigning customer shaper rate
 */

typedef enum {
  BF_TM_SCH_RATE_UPPER,     /** Upper limit for the rate, Over-provisioning*/
  BF_TM_SCH_RATE_LOWER,     /** Lower limit for the rate, Under-provisioning*/
  BF_TM_SCH_RATE_MIN_ERROR, /** Min Error betweeen the Upper/Lower rate */
} bf_tm_sched_shaper_prov_type_t;

typedef enum {
  BF_TM_SCH_ADV_FC_MODE_CRE = 0,
  BF_TM_SCH_ADV_FC_MODE_XOFF = 1,
  BF_TM_SCH_ADV_FC_MODE_MAX = BF_TM_SCH_ADV_FC_MODE_XOFF,
} bf_tm_sched_adv_fc_mode_t;

/* buffer drop state entries*/
typedef enum {
  PER_EG_PIPE_BUFF_DROP_ST,   // Per Egress PIPE Buffer drop state (4 PIPEs)
  GLB_BUFF_AP_GREEN_DROP_ST,  // Global Buffer AP Green drop state (4 Pools)
  GLB_BUFF_AP_YEL_DROP_ST,    // Global Buffer AP Yellow drop state(4 Pools)
  GLB_BUFF_AP_RED_DROP_ST,    // Global Buffer AP Red drop state (4 Pools)
  PIPE0_PRE_FIFO_DROP_ST,     // PIPE0 PRE FIFO drop state (4 FIFOs)
  PIPE1_PRE_FIFO_DROP_ST,     // PIPE1 PRE FIFO drop state (4 FIFOs)
  PIPE2_PRE_FIFO_DROP_ST,     // PIPE2 PRE FIFO drop state (4 FIFOs)
  PIPE3_PRE_FIFO_DROP_ST,     // PIPE3 PRE FIFO drop state (4 FIFOs)
  PIPE4_PRE_FIFO_DROP_ST,     // PIPE4 PRE FIFO drop state (4 FIFOs)
  PIPE5_PRE_FIFO_DROP_ST,     // PIPE5 PRE FIFO drop state (4 FIFOs)
  PIPE6_PRE_FIFO_DROP_ST,     // PIPE6 PRE FIFO drop state (4 FIFOs)
  PIPE7_PRE_FIFO_DROP_ST,     // PIPE7 PRE FIFO drop state (4 FIFOs)
  BF_TM_EG_BUFFER_DROP_ST_LAST
} bf_tm_eg_buffer_drop_state_en;

typedef struct _bf_tm_dev_cfg_t {
  uint8_t pipe_cnt;
  uint8_t pg_per_pipe;
  uint8_t q_per_pg;
  uint8_t ports_per_pg;
  uint16_t pfc_ppg_per_pipe;
  uint16_t total_ppg_per_pipe;
  uint8_t pre_fifo_per_pipe;
  uint8_t l1_per_pg;
  uint16_t l1_per_pipe;
} bf_tm_dev_cfg_t;

#define BF_TM_RATE_1G (1 * 1000 * 1000)
#define BF_TM_RATE_10G (10 * 1000 * 1000)
#define BF_TM_RATE_25G (25 * 1000 * 1000)
#define BF_TM_RATE_40G (40 * 1000 * 1000)
#define BF_TM_RATE_50G (50 * 1000 * 1000)
#define BF_TM_RATE_100G (100 * 1000 * 1000)
#define BF_TM_RATE_200G (200 * 1000 * 1000)
#define BF_TM_RATE_400G (400 * 1000 * 1000)

// In Micro secs
#define BF_TM_MICRO_SEC(us) (us)
#define BF_TM_MILLI_SEC(ms) (ms * 1000)
#define BF_TM_SEC(sec) (sec * 1000 * 1000)

/* @} */

#endif
