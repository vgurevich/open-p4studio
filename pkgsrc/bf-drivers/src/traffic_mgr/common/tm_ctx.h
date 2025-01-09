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


/*
 *    This file contains all data strcutures
 *    related to maintaining TM resources in memory
 */

#ifndef __TM_CTX_H__
#define __TM_CTX_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include <target-utils/list/bf_list.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <traffic_mgr/traffic_mgr.h>

typedef int bf_tm_status_t;
#define BF_TM_MAX_PFC_LEVELS (8)
#define BF_TM_MAX_COS_LEVELS (8)
#define BF_TM_MAX_MAU_PIPES (8)

#define BF_TM_DEFAULT_VAL(name, type) \
  type name;                          \
  bool name##_is_valid;

#define BF_TM_SET_DEFAULT_VAL(container, name, val) \
  container->name = val;                            \
  container->name##_is_valid = true;

typedef struct _bf_tm_port_defaults {
  BF_TM_DEFAULT_VAL(port_icos_mask, uint8_t)
  BF_TM_DEFAULT_VAL(port_ct_enable, bool)
  BF_TM_DEFAULT_VAL(port_ig_limit, uint32_t)
  BF_TM_DEFAULT_VAL(port_ig_hysteresis, uint32_t)
  BF_TM_DEFAULT_VAL(port_eg_limit, uint32_t)
  BF_TM_DEFAULT_VAL(port_eg_hysteresis, uint32_t)
  BF_TM_DEFAULT_VAL(port_skid_limit, uint32_t)
  BF_TM_DEFAULT_VAL(port_uc_ct_limit, uint32_t)
  BF_TM_DEFAULT_VAL(port_mode_tx, bf_tm_flow_ctrl_type_t)
  BF_TM_DEFAULT_VAL(port_mode_rx, bf_tm_flow_ctrl_type_t)
} bf_tm_port_defaults_t;

typedef struct _bf_tm_q_defaults {
  BF_TM_DEFAULT_VAL(q_gmin_limit, uint32_t)
  BF_TM_DEFAULT_VAL(q_tail_drop, bool)
  BF_TM_DEFAULT_VAL(q_app_pool, bf_tm_app_pool_t)
  BF_TM_DEFAULT_VAL(q_base_use_limit, uint32_t)
  BF_TM_DEFAULT_VAL(q_dynamic_baf, bf_tm_queue_baf_t)
  BF_TM_DEFAULT_VAL(q_qac_hysteresis, uint32_t)
  BF_TM_DEFAULT_VAL(q_cos, uint8_t)
  BF_TM_DEFAULT_VAL(q_color_drop_mode, bool)
  BF_TM_DEFAULT_VAL(q_color_yellow_limit, bf_tm_queue_color_limit_t)
  BF_TM_DEFAULT_VAL(q_color_red_limit, bf_tm_queue_color_limit_t)
  BF_TM_DEFAULT_VAL(q_color_yellow_hysteresis, bf_tm_thres_t)
  BF_TM_DEFAULT_VAL(q_color_red_hysteresis, bf_tm_thres_t)

} bf_tm_q_defaults_t;

typedef struct _bf_tm_pool_defaults {
  BF_TM_DEFAULT_VAL(pfc_limit, uint32_t)
  BF_TM_DEFAULT_VAL(color_drop_hysteresis, uint32_t)
  BF_TM_DEFAULT_VAL(skid_hysteresis, uint32_t)
  BF_TM_DEFAULT_VAL(glb_max_cell_limit, uint32_t)
  BF_TM_DEFAULT_VAL(glb_max_cell_limit_en, bool)

} bf_tm_pool_defaults_t;

typedef struct _bf_tm_pipe_defaults {
  BF_TM_DEFAULT_VAL(mirror_drop_enable, bool)
  BF_TM_DEFAULT_VAL(egress_limit_cells, uint32_t)
  BF_TM_DEFAULT_VAL(egress_hysteresis_cells, uint32_t)
  BF_TM_DEFAULT_VAL(port_mirror_on_drop_dest, bf_dev_port_t)
  BF_TM_DEFAULT_VAL(queue_mirror_on_drop_dest, bf_tm_queue_t)
  BF_TM_DEFAULT_VAL(pkt_ifg_compensation, uint8_t)
  BF_TM_DEFAULT_VAL(qstat_report_mode, bool)

} bf_tm_pipe_defaults_t;

typedef struct _bf_tm_sch_q_defaults {
  BF_TM_DEFAULT_VAL(sch_q_dwrr_weight, uint16_t)
  BF_TM_DEFAULT_VAL(sch_q_enable, bool)
  BF_TM_DEFAULT_VAL(sch_q_shaping_enable, bool)
  BF_TM_DEFAULT_VAL(sch_q_priority, bf_tm_sched_prio_t)
  BF_TM_DEFAULT_VAL(sch_q_guaranteed_enable, bool)
  BF_TM_DEFAULT_VAL(sch_q_shaping_rate_pps, bool)
  BF_TM_DEFAULT_VAL(sch_q_shaping_rate_burst_size, uint32_t)
  BF_TM_DEFAULT_VAL(sch_q_shaping_rate, uint32_t)
  BF_TM_DEFAULT_VAL(sch_q_shaping_rate_prov_type,
                    bf_tm_sched_shaper_prov_type_t)
  BF_TM_DEFAULT_VAL(sch_q_guaranteed_rate_pps, bool)
  BF_TM_DEFAULT_VAL(sch_q_guaranteed_rate_burst_size, uint32_t)
  BF_TM_DEFAULT_VAL(sch_q_guaranteed_rate_prov_type,
                    bf_tm_sched_shaper_prov_type_t)
  BF_TM_DEFAULT_VAL(sch_q_guaranteed_rate, uint32_t)
  BF_TM_DEFAULT_VAL(sch_q_adv_fc_mode, bf_tm_sched_adv_fc_mode_t)
  BF_TM_DEFAULT_VAL(sch_adv_fc_mode_enable, bool)

} bf_tm_sch_q_defaults_t;

typedef struct _bf_tm_sch_port_defaults {
  BF_TM_DEFAULT_VAL(sch_port_shaping_enable, bool)
  BF_TM_DEFAULT_VAL(sch_port_shaping_rate_pps, bool)
  BF_TM_DEFAULT_VAL(sch_port_shaping_rate_burst_size, uint32_t)
  BF_TM_DEFAULT_VAL(sch_port_shaping_rate_prov_type,
                    bf_tm_sched_shaper_prov_type_t)
  BF_TM_DEFAULT_VAL(sch_port_shaping_rate, uint32_t)

} bf_tm_sch_port_defaults_t;

typedef struct _bf_tm_sch_l1_defaults {
  BF_TM_DEFAULT_VAL(sch_l1_dwrr_weight, uint16_t)
  BF_TM_DEFAULT_VAL(sch_l1_guaranteed_enable, bool)
  BF_TM_DEFAULT_VAL(sch_l1_guaranteed_priority, bf_tm_sched_prio_t)
  BF_TM_DEFAULT_VAL(sch_l1_shaping_enable, bool)
  BF_TM_DEFAULT_VAL(sch_l1_shaping_priority, bf_tm_sched_prio_t)
  BF_TM_DEFAULT_VAL(sch_l1_priority_prop_enable, bool)
  BF_TM_DEFAULT_VAL(sch_l1_guaranteed_rate_pps, bool)
  BF_TM_DEFAULT_VAL(sch_l1_guaranteed_rate_burst_size, uint32_t)
  BF_TM_DEFAULT_VAL(sch_l1_guaranteed_rate, uint32_t)
  BF_TM_DEFAULT_VAL(sch_l1_shaping_rate_pps, bool)
  BF_TM_DEFAULT_VAL(sch_l1_shaping_rate_burst_size, uint32_t)
  BF_TM_DEFAULT_VAL(sch_l1_shaping_rate, uint32_t)

} bf_tm_sch_l1_defaults_t;

typedef struct _bf_tm_ppg_defaults {
  BF_TM_DEFAULT_VAL(min_limit_cells, uint32_t)
  BF_TM_DEFAULT_VAL(hysteresis_cells, uint32_t)
  BF_TM_DEFAULT_VAL(pool, bf_tm_app_pool_t)
  BF_TM_DEFAULT_VAL(pool_max_cells, uint32_t)
  BF_TM_DEFAULT_VAL(dynamic_baf, bf_tm_ppg_baf_t)

} bf_tm_ppg_defaults_t;

#include <traffic_mgr/traffic_mgr_types.h>
#include "traffic_mgr_cached_counters.h"

#include "tm_port.h"
#include "tm_ig_ppg.h"
#include "tm_ig_pools.h"
#include "tm_port.h"
#include "tm_pipe.h"
#include "tm_dev.h"
#include "tm_queue.h"
#include "tm_sch.h"
#include "tm_mcast.h"
#include "tm_path_counters.h"
#include "traffic_mgr/api/tm_api_helper.h"
#include "tm_hw_access.h"
#include "traffic_mgr_log.h"

/* Invalid Identifiers */
#define BF_TM_INVALID_PPG (0xffff)
#define BF_TM_INVALID_PORT (0xff)
#define BF_TM_INVALID_PG (0xff)
#define BF_TM_INVALID_PIPE (0xff)
#define BF_TM_INVALID_ICOS (0x8)
#define BF_TM_INVALID_ICOS_MASK (0x0)
#define BF_TM_INVALID_THRES_LIMIT (0xffffff)

#define BF_TM_DIR_INGRESS (1)
#define BF_TM_DIR_EGRESS (2)

#define BF_TM_INVALID_ARG(arg)                                             \
  do {                                                                     \
    if (arg) {                                                             \
      LOG_ERROR("Invalid argument \"%s\" to API \"%s\" ", #arg, __func__); \
      return (BF_INVALID_ARG);                                             \
    }                                                                      \
  } while (0);

#define TM_IS_THRES_INVALID(cells, ctx) (cells > ctx->tm_cfg.total_cells)
#define TM_IS_DEV_INVALID(dev)                                  \
  (dev < 0 || dev >= BF_TM_NUM_ASIC || NULL == g_tm_ctx[dev] || \
   g_tm_ctx[dev]->devid != dev || !(g_tm_ctx_valid[dev]))
#define TM_IS_SUBDEV_INVALID(subdev_id) (subdev_id >= BF_TM_NUM_SUBDEV)
#define TM_IS_PIPE_INVALID(pipe, ctx) (pipe >= ctx->tm_cfg.pipe_cnt)
#define TM_IS_POOL_INVALID(poolid, ctx) (poolid >= ctx->tm_cfg.shared_pool_cnt)

#define TM_PPG_IDX(ppg_hndl) (ppg_hndl & 0xfff)
#define TM_PPG_PIPE(ppg_hndl) ((ppg_hndl >> 12) & 0xf)

// PPG handler for DPG stores its local port; PFC PPG has zero here.
#define TM_DPG_LOCAL_PORT(ppg_hndl) (DEV_PORT_TO_LOCAL_PORT(ppg_hndl >> 16))
#define TM_DPG_DEV_PORT(ppg_hndl) \
  (MAKE_DEV_PORT(TM_PPG_PIPE(ppg_hndl), TM_DPG_LOCAL_PORT(ppg_hndl)))

#define TM_IS_PPG_INVALID(ppg, ctx)                   \
  ((TM_IS_PIPE_INVALID(TM_PPG_PIPE(ppg), ctx)) ||     \
   (TM_IS_PORT_INVALID(TM_DPG_DEV_PORT(ppg), ctx)) || \
   (TM_PPG_IDX(ppg) >= ctx->tm_cfg.total_ppg_per_pipe))

#define TM_IS_PORT_INVALID(dev_port, ctx) \
  (NULL == ctx) || (BF_SUCCESS != bf_tm_port_is_valid(ctx->devid, dev_port))

#define TM_IS_COSBMAP_INVALID(icos_bmap) (!icos_bmap)
#define TM_IS_MCFIFO_INVALID(fifo, ctx) \
  ((fifo) >= (ctx->tm_cfg.pre_fifo_per_pipe))
#define TM_IS_PIPEMASK_INVALID(pipemask) (!pipemask)
#define TM_IS_ICOS_INVALID(icos) (icos >= BF_TM_MAX_COS_LEVELS)

#define TM_HITLESS_IS_CFG_MATCH(cfg1, cfg2, ctx) \
  ((ctx->current_init_mode == BF_DEV_WARM_INIT_HITLESS) && (cfg1 == cfg2))

#define TM_HITLESS_WARM_INIT_IN_PROGRESS(dev)                        \
  ((g_tm_ctx[dev]->current_init_mode == BF_DEV_WARM_INIT_HITLESS) && \
   (tm_is_device_locked(dev)))

#define BF_TM_OBJ_SET(tm_ctx, q_descr, q_prop, prop_val, is_set)     \
  if (!TM_HITLESS_IS_CFG_MATCH(prop_val, q_descr->q_prop, tm_ctx)) { \
    q_descr->q_prop = prop_val;                                      \
    is_set = true;                                                   \
  }

#define TM_IS_DROP_STATE_TYPE_INVALID(drop_type) \
  (drop_type >= BF_TM_EG_BUFFER_DROP_ST_LAST)

#define TM_CLEAR_LOW_3_BITS(val) (((val >> 3) << 3))
#define TM_CELLS_TO_8CELL_UNITS(val) (val >> 3)
#define TM_8CELL_UNITS_TO_CELLS(val) (val << 3)
#define TM_IS_8CELL_UNITS(val) ((val % 8) == 0)

typedef enum {



  BF_TM_ASIC_TOF3 = 3,
  BF_TM_ASIC_TOF2 = 2,
  BF_TM_ASIC_TOFINO = 1,
  BF_TM_ASIC_TOFINOLITE = 10,
  /* ----- Add Here : New-ASIC ------ */
} bf_tm_asic_en;

typedef enum { BF_TM_TARGET_MODEL, BF_TM_TARGET_ASIC } bf_tm_target_en;

#define TM_IS_TARGET_ASIC(dev) (g_tm_ctx[dev]->target == BF_TM_TARGET_ASIC)

typedef struct _bf_tm_pipe_cfg {
  uint8_t pipe;
  uint16_t *port_pfc_mask;  // Allocate mem of
                            // (pg_per_pipe * ports_per_pg)
                            // reg: port_pfc_en
                            // Assumption: Support of upto 16 iCoS. When
                            // more than 16, change uint16_t appropriately.
  uint32_t *port_pause_en;  // Allocate mem of
                            // (pg_per_pipe * ports_per_pg)/sizeof(uint32_t)
} bf_tm_pipe_cfg_t;

typedef struct _bf_tm_cfg {
  // Common to both ingress and egress
  uint8_t rsvd_pool_cnt;
  uint8_t skid_pool_cnt;
  uint8_t shared_pool_cnt;
  uint8_t pipe_cnt;
  uint16_t active_pipe_mask;
  uint32_t total_cells;
  uint16_t cell_size;
  uint8_t pg_per_pipe;
  uint8_t ports_per_pg;
  uint8_t ports_per_pipe;
  uint8_t icos_count;  // Supported number of iCoS
  uint8_t mirror_port_start;
  uint8_t mirror_port_cnt;
  uint8_t uc_ct_max_cells;

  // Ingress TM config
  uint8_t ig_pool_cnt;
  uint16_t total_ppg_per_pipe;
  uint16_t pfc_ppg_per_pipe;
  bool ingress_bypass_en;  // Bypass WAC/Ingress Buffer Mgmt
  uint8_t gmin_pool;       // PoolID to which deafult PPG,
                           // unmapped Q map to.

  // Egress TM config
  uint8_t eg_pool_cnt;
  uint8_t q_per_pg;      // Total queues per PG
  uint16_t q_per_pipe;   // Total queues per pipe
  uint16_t q_prof_cnt;   // Total queue profiles supported
  uint8_t l1_per_pg;     // Total L1 scheduler nodes per PG
  uint16_t l1_per_pipe;  // Total L1 scheduler nodes per pipe

  uint8_t pre_fifo_per_pipe;  // Total pre fifo per pipe
  uint8_t chnl_mult;  // To account for the even devport numbering in TOF3

} bf_tm_cfg_t;

#define BF_TM_IS_TOFINO(asic_type) ((asic_type) == (BF_TM_ASIC_TOFINO))
#define BF_TM_IS_TOFINOLITE(asic_type) ((asic_type) == (BF_TM_ASIC_TOFINOLITE))
#define BF_TM_IS_TOF2(asic_type) ((asic_type) == (BF_TM_ASIC_TOF2))
#define BF_TM_IS_TOF3(asic_type) ((asic_type) == (BF_TM_ASIC_TOF3))





/* ----- Add Here : New-ASIC ------ */

/* Maintain ppg structure for all logical pipes in logical pipe-0 to n order */
#define BF_TM_PPG_PTR(ctx, pipe, ppg_n)                                  \
  (ctx->ig_ppg                                                           \
       ? (ctx->ig_ppg + (pipe * ctx->tm_cfg.total_ppg_per_pipe) + ppg_n) \
       : NULL)

#define BF_TM_FIRST_Q_PTR_IN_PIPE(ctx, pipe) \
  (ctx->eg_q + (pipe * ctx->tm_cfg.q_per_pipe))

#define BF_TM_FIRST_Q_PTR_IN_PG(ctx, port)                            \
  (ctx->eg_q + (DEV_PORT_TO_PIPE(port) * ctx->tm_cfg.q_per_pipe) +    \
   ((DEV_PORT_TO_LOCAL_PORT(                                          \
         lld_sku_map_devport_from_user_to_device(ctx->devid, port)) / \
     ctx->tm_cfg.ports_per_pg) *                                      \
    ctx->tm_cfg.q_per_pg))

#define BF_TM_FIRST_L1_PTR_IN_PG(ctx, port)                           \
  (ctx->eg_l1 + (DEV_PORT_TO_PIPE(port) * ctx->tm_cfg.l1_per_pipe) +  \
   ((DEV_PORT_TO_LOCAL_PORT(                                          \
         lld_sku_map_devport_from_user_to_device(ctx->devid, port)) / \
     ctx->tm_cfg.ports_per_pg) *                                      \
    ctx->tm_cfg.l1_per_pg))

#define BF_TM_PORT_IDX(ctx, port) \
  DEV_PORT_TO_LOCAL_PORT(         \
      lld_sku_map_devport_from_user_to_device(ctx->devid, port))

#define BF_TM_PG_BASE_DEV_PORT(ctx, pipe, pg_n) \
  (lld_sku_map_devport_from_device_to_user(     \
      ctx->devid, MAKE_DEV_PORT(pipe, (pg_n * (ctx->tm_cfg.ports_per_pg)))))

#define BF_TM_PORTS_PER_PIPE(ctx)                         \
  ((ctx->tm_cfg.ports_per_pg * ctx->tm_cfg.pg_per_pipe) + \
   ctx->tm_cfg.mirror_port_cnt)

#define BF_TM_PORT_PTR(ctx, port)                                    \
  (ctx->ports + DEV_PORT_TO_PIPE(port) * BF_TM_PORTS_PER_PIPE(ctx) + \
   BF_TM_PORT_IDX(ctx, port))

#define BF_TM_MC_FIFO_PTR(ctx, pipe, fifo) \
  (ctx->mcast_fifo + (pipe * ctx->tm_cfg.pre_fifo_per_pipe) + fifo)

typedef enum _bf_tm_ctx_type {
  BF_TM_CTX_REGULAR = 0,  // used for regular init and working
  BF_TM_CTX_RESTORED      // used for hitless unit testing inside TM
} bf_tm_ctx_type_en;

typedef struct _bf_tm_dev_cfg {
  bool sw_inited;            // Indicates if device is ready for
                             // configuration. After all bf_tm_init_*()
                             // sucessful calls, set this value
  bool hw_inited;            // When 'sw_inited' transitions from false-->true,
                             // program HW with default config.
                             // This bit indicates successful HW configuration.
  bf_dev_id_t devid;         // Device instance
  uint8_t asic_type;         // Tofino, Tofino-lite...
  uint64_t clock_speed;      // Clock frequency in HZ */
  uint64_t bps_clock_speed;  // Clock frequency in HZ */
  bf_sku_chip_part_rev_t part_rev;  // Part revision A0/B0
  uint8_t q_profile_use_cnt;        // Keeps count of how many Q-prof
                                    // are allocated

  tm_mutex_t lock;
  bool internal_call;   // Used to hold lock or not and not flush buffers.
  bool batch_mode;      // Use write list to push bulk of device updates
  bool api_batch_mode;  // controlled by TM public API. When set to true,
                        // multiple TM APIs configs can be batched and
                        // pushed to device at the end of series of API.
  bool fast_reconfig_init_seq;  // Used to indicate if TM init seq
                                // is to be done during fast reconfig

  bf_tm_cfg_t tm_cfg;   // Based on asic-type, configuration
  bf_tm_ppg_t *ig_ppg;  // For all Pipes
  bf_tm_eg_q_t *eg_q;   // For all Pipes
  bf_tm_q_profile_t *q_profile;
  bf_tm_eg_l1_t *eg_l1;      // For all Pipes
  bf_tm_ig_pool_t *ig_pool;  // Ingress pools
  bf_tm_eg_pool_t *eg_pool;  // Egress pools
  bf_tm_port_t *ports;       // For all Pipes
  bf_tm_eg_pipe_t *pipes;
  bf_tm_mcast_t *mcast_fifo;             // For all pipes
  bf_dev_init_mode_t current_init_mode;  // init mode of TM driver
  uint32_t uc_ct_size;
  uint32_t mc_ct_size;
  uint8_t timestamp_shift;
  bf_tm_target_en target;  // asic / model. Target is set to model when
                           // running unit testing with asic model.
  tm_cache_counters_ctx_t cache_counters;
  uint32_t subdev_count;       // Tofino3 can have single die or 2 die
  bf_tm_ctx_type_en ctx_type;  // Needs to correctly delete the ctx

  // WAC/QAC hysteresis profiles
  bool read_por_wac_profile_table;  // WAC profiles are restored from HW
  bool read_por_qac_profile_table;  // QAC profiles are restored from HW
} bf_tm_dev_ctx_t;

extern bf_tm_dev_ctx_t *g_tm_ctx[BF_TM_NUM_ASIC];
extern bf_tm_dev_ctx_t *g_tm_restore_ctx[BF_TM_NUM_ASIC];

// This lock is used to protect the timer_cb from using g_tm_ctx[] that is
// deleted
extern tm_mutex_t g_tm_timer_lock[BF_TM_NUM_ASIC];
extern bool g_tm_ctx_valid[BF_TM_NUM_ASIC];

bf_tm_status_t bf_tm_init_ppg(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ppg_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ppg_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ppg_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ppg_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_q(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_q_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_q_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_q_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_q_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_ig_pool(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ig_pool_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ig_pool_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ig_pool_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ig_pool_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_eg_pool(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_eg_pool_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_eg_pool_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_eg_pool_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_eg_pool_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_ports(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_ports_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_port_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_port_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_sch(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_sch_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_sch_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_sch_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_pipe(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_pipe_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_pipe_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_pipe_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_mcast_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_mcast(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_mcast_delete(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_mcast_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_mcast_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
bf_tm_status_t bf_tm_init_counters(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_path_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_path_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);

bf_tm_status_t bf_tm_init_dev(bf_tm_dev_ctx_t *tm_ctx);
void bf_tm_dev_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx);

void bf_tm_tofino_cfg(bf_tm_cfg_t *tm_cfg, bf_dev_id_t devid);
void bf_tm_tofinolite_cfg(bf_tm_cfg_t *tm_cfg);
void bf_tm_tof2_cfg(bf_tm_cfg_t *tm_cfg, bf_dev_id_t devid);
void bf_tm_tof3_cfg(bf_tm_cfg_t *tm_cfg, bf_dev_id_t devid);





/**
 * @brief Get default values for the port.
 * bf_tm_tofino_port_get_defaults()
 * bf_tm_tof2_port_get_defaults()
 * bf_tm_tof3_port_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  p        TM Port internal data structure to get its defaults.
 *                      If NULL, then only common port-independent defaults will
 *                      be set with 'is_valid'
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tofino_port_get_defaults(bf_dev_id_t devid,
                                              bf_tm_port_t *p,
                                              bf_tm_port_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_port_get_defaults(bf_dev_id_t devid,
                                            bf_tm_port_t *p,
                                            bf_tm_port_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_port_get_defaults(bf_dev_id_t devid,
                                            bf_tm_port_t *p,
                                            bf_tm_port_defaults_t *defaults);

bf_tm_status_t bf_tm_tofino_q_get_defaults(bf_dev_id_t devid,
                                           bf_tm_q_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_q_get_defaults(bf_dev_id_t devid,
                                         bf_tm_q_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_q_get_defaults(bf_dev_id_t devid,
                                         bf_tm_q_defaults_t *defaults);

/**
 * @brief Get default values for the pool.
 * bf_tm_tofino_pool_get_defaults()
 * bf_tm_tof2_pool_get_defaults()
 * bf_tm_tof3_pool_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  pool     pool handle for which defaults are requested. If
 *                      NULL - only common values will be fetched.
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tofino_pool_get_defaults(bf_dev_id_t devid,
                                              bf_tm_app_pool_t *pool,
                                              bf_tm_pool_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_pool_get_defaults(bf_dev_id_t devid,
                                            bf_tm_app_pool_t *pool,
                                            bf_tm_pool_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_pool_get_defaults(bf_dev_id_t devid,
                                            bf_tm_app_pool_t *pool,
                                            bf_tm_pool_defaults_t *defaults);







/**
 * @brief Get default values for the pipe.
 * bf_tm_tofino_pipe_get_defaults()
 * bf_tm_tof2_pipe_get_defaults()
 * bf_tm_tof3_pipe_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  pipe     Pipe Identifier.
 *                      If only common pipe-independent defaults needs to be
 *                      fetched, this parameter can be NULL.
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tofino_pipe_get_defaults(bf_dev_id_t devid,
                                              bf_tm_eg_pipe_t *pipe,
                                              bf_tm_pipe_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_pipe_get_defaults(bf_dev_id_t devid,
                                            bf_tm_eg_pipe_t *pipe,
                                            bf_tm_pipe_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_pipe_get_defaults(bf_dev_id_t devid,
                                            bf_tm_eg_pipe_t *pipe,
                                            bf_tm_pipe_defaults_t *defaults);






/**
 * @brief Get TM scheduler defaults for Queue.
 * bf_tm_tofino_sch_q_get_defaults()
 * bf_tm_tof2_sch_q_get_defaults()
 * bf_tm_tof3_sch_q_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  p        TM Port internal data structure to get port-dependent
 *                      defaults. If NULL, then only common port-independent
 *                      defaults will be set with 'is_valid'
 * @param[in]  q        TM Queue internal data structure. If NULL - only
 *                      queue-independent constants can be fetched
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tofino_sch_q_get_defaults(
    bf_dev_id_t devid,
    bf_tm_port_t *p,
    bf_tm_eg_q_t *q,
    bf_tm_sch_q_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_sch_q_get_defaults(bf_dev_id_t devid,
                                             bf_tm_port_t *p,
                                             bf_tm_eg_q_t *q,
                                             bf_tm_sch_q_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_sch_q_get_defaults(bf_dev_id_t devid,
                                             bf_tm_port_t *p,
                                             bf_tm_eg_q_t *q,
                                             bf_tm_sch_q_defaults_t *defaults);








/**
 * @brief Get TM scheduler defaults for Port.
 * bf_tm_tofino_sch_port_get_defaults()
 * bf_tm_tof2_sch_port_get_defaults()
 * bf_tm_tof3_sch_port_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  p        TM Port internal data structure to get port-dependent
 *                      defaults. If NULL, then only common port-independent
 *                      defaults will be set with 'is_valid'
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tofino_sch_port_get_defaults(
    bf_dev_id_t devid, bf_tm_port_t *p, bf_tm_sch_port_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_sch_port_get_defaults(
    bf_dev_id_t devid, bf_tm_port_t *p, bf_tm_sch_port_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_sch_port_get_defaults(
    bf_dev_id_t devid, bf_tm_port_t *p, bf_tm_sch_port_defaults_t *defaults);

/**
 * @brief Get TM scheduler defaults for L1 Node.
 * bf_tm_tof2_sch_l1_get_defaults()
 * bf_tm_tof3_sch_l1_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  p        TM Port internal data structure. If NULL -
 *                      only port-independent constants can be fetched
 * @param[in]  l1       TM L1 id to get l1-dependent defaults. If only common
 *                      l1-independent defaults needed, then this parameter can
 *                      be NULL
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_tm_status_t bf_tm_tof2_sch_l1_get_defaults(
    bf_dev_id_t devid,
    bf_tm_port_t *p,
    bf_tm_eg_l1_t *l1,
    bf_tm_sch_l1_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_sch_l1_get_defaults(
    bf_dev_id_t devid,
    bf_tm_port_t *p,
    bf_tm_eg_l1_t *l1,
    bf_tm_sch_l1_defaults_t *defaults);

/**
 * @brief Get default values for the PPG.
 * bf_tm_tofino_ppg_get_defaults()
 * bf_tm_tof2_ppg_get_defaults()
 * bf_tm_tof3_ppg_get_defaults()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  ppg      TM PPG internal data structure to get its defaults.
 *                      If NULL, then only common ppg-independent defaults will
 *                      be set with 'is_valid'
 * @param[out] defaults Default values.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */

bf_tm_status_t bf_tm_tofino_ppg_get_defaults(bf_dev_id_t devid,
                                             bf_tm_ppg_t *ppg,
                                             bf_tm_ppg_defaults_t *defaults);
bf_tm_status_t bf_tm_tof2_ppg_get_defaults(bf_dev_id_t devid,
                                           bf_tm_ppg_t *ppg,
                                           bf_tm_ppg_defaults_t *defaults);
bf_tm_status_t bf_tm_tof3_ppg_get_defaults(bf_dev_id_t devid,
                                           bf_tm_ppg_t *ppg,
                                           bf_tm_ppg_defaults_t *defaults);

bf_tm_status_t bf_tm_write_register(bf_dev_id_t, uint32_t, uint32_t);
bf_tm_status_t bf_tm_read_register(bf_dev_id_t, uint32_t, uint32_t *);
bf_tm_status_t bf_tm_subdev_write_register(bf_dev_id_t,
                                           bf_subdev_id_t,
                                           uint32_t,
                                           uint32_t);
bf_tm_status_t bf_tm_subdev_read_register(bf_dev_id_t,
                                          bf_subdev_id_t,
                                          uint32_t,
                                          uint32_t *);
void tm_enable_all_dr(bf_dev_id_t dev_id);
void tm_disable_all_dr(bf_dev_id_t dev_id);
bf_status_t tm_lock_device(bf_dev_id_t dev_id);
bf_status_t tm_unlock_device(bf_dev_id_t dev_id);
bool tm_is_device_locked(bf_dev_id_t dev_id);
bf_status_t tm_chip_init_sequence_during_fast_reconfig(bf_dev_id_t dev_id);

/* Should be called at the end of every TM API (exposed to clients).
 * This macros will make APIs MT safe and at the end of API execution
 * all the register updates that could be buffered in DMA buffers will be
 * pushed to hardware.
 */
#define TM_UNLOCK_AND_FLUSH(dev)                                               \
  {                                                                            \
    if ((!g_tm_ctx[dev]->internal_call) && (!g_tm_ctx[dev]->api_batch_mode)) { \
      BF_TM_FLUSH_WL(dev);                                                     \
    }                                                                          \
    TM_UNLOCK(dev, g_tm_ctx[dev]->lock);                                       \
  }

#define BF_TM_RETURN_ON_ERROR(status, error_msg) \
  {                                              \
    if (status != BF_SUCCESS) {                  \
      LOG_ERROR(error_msg);                      \
      return status;                             \
    }                                            \
  }

#define BF_TM_PPG_CHECK(ppg, ppg_n_port, pipe, dev)                           \
  {                                                                           \
    if (ppg == NULL) {                                                        \
      LOG_ERROR(                                                              \
          "%s, Failed to fetch ppg data for ppg/port %d, pipe %d, device %d", \
          __func__,                                                           \
          ppg_n_port,                                                         \
          pipe,                                                               \
          dev);                                                               \
      return (BF_NO_SYS_RESOURCES);                                           \
    }                                                                         \
  }

#endif
