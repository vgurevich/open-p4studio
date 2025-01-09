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


#ifndef __TM_TOFINO_HW_INTF_H__
#define __TM_TOFINO_HW_INTF_H__

#include "traffic_mgr/common/tm_ctx.h"

/* HW read/write functions used by accessor functions in
 * tm_[ig_ppg.c/ig_pool.c/...
 */

/*
 *                  Ingress Tofino
 *
 * Ingress View of Tofino with 4 pipes, 128 PPGs per pipe + 72 default ppgs
 *
 * 1. 4 Shared pools. Size of shared pools = (Total size - size of skid pool
 *                                           - size of Min/Guaranteed pool)
 *
 * 2. A PFC (flow control enabled) PPG will map to 3 pools. Min Pool, Skid Pool
 *    and shared pool (configurable shared pooli; can choose one out of 4)
 *
 * 3. Sizes of Shared pools, skid pool and min pool are configurable.
 *
 * 4. Size of Min pool = SUM(size of every PPG in every PIPE)
 *
 *   Ingress View of TM memory
 *  +-----------------------+
 *  |                       |             PIPE 0 PPGs
 *  |         SKID POOL     |        +-------------------+
 *  |                       |<----+  |      PPG0         |
 *  +-----------------------+     |  +-------------------+
 *  |  Negative Mirror(DOD) |     |  |       :           |
 *  +-----------------------+     |  +-------------------+
 *  |                       |     |  |     PPG 127       |
 *  |  Shared Pool - 0      |     |  + ------------------+
 *  +-----------------------      |
 *  |  Shared Pool - 1      |     |
 *  |                       |<----+          :
 *  |                       |     |          :
 *  |                       |     |          :
 *  +-----------------------      |
 *  |  Shared Pool - 2      |     |
 *  |                       |     |
 *  +-----------------------      |        PIPE 3 PPGs
 *  |  Shared Pool - 3      |     |   +--------------------+
 *  |                       |     |   |    PPG 0           |
 *  +-----------------------      |   +--------------------+
 *                          |     |   |        :           |
 *  |  MIN/Guaranteed Pool  |     |   +--------------------+
 *  |                       <-----+---| --- PPG x          |
 *  +-----------------------+         +--------------------+
 *                                    |                    |
 *                                    |    PPG  127        |
 *                                    +--------------------+

 *  Here PPG - x is PFC enabled. Hence maps to 3 pools

 * 5. For a given packet that mapps to any PPG(say PPG y) in any PIPE
 *   (0 to 3; pipe# doesn't matter) packet is subjected to following
 *   threshold checking on ingress side....

 *    When any time check passes, further checks are NOT done and
 *    packet accepted.
 *
 *          |
 *          v
 *    +------------+
 *    | MIN POOL   |
 *    | usage check|
 *    |  Fails     |
 *    | (using ppg)|
 *    +------------+

 *          | From here on use shared Pool to which this PPG belongs
 *          v
 *    +------------+            +------------+
 *    |  SHARED    |            | If pfc     |
 *    |POOL usage  |            |enabled     |
 *    |check Fails | -------->  |(pool, icos)|
 *    |(using color)|           |level check |
 *    |            |            | fails(uses)|
 *    |            |            |(using icos)|
 *    +------------+            +------+-----+
 *                                     |
 *                                     |
 *                                     |
 *    +------------+                   |
 *    |If static   |                   |
 *    |usage level |<------------------+
 *    |or dynamic  |
 *    |level (baf) |
 *    |fails       |
 *    |(using ppg) |
 *    +-----+------+
 *          |
 *          |
 *          |  Drop packet if PFC not enabled for PPG
 *          |  else use skid space.
 *          v
 *    +------------+
 *    | If PFC     |
 *    | enabled    |
 *    | skid pool  |
 *    | usage check|
 *    +------------+
 *
 *          |
 *          v
 *      Drop Packet
 *
 */

////////////// Functions to carve default /////////////

bf_status_t bf_tm_tofino_set_default(bf_dev_id_t dev);

/////////////PPGs///////////////

#define BF_TM_TOFINO_SET_PPG_INTF(field) \
  bf_tm_status_t bf_tm_tofino_ppg_set_##field(bf_dev_id_t, const bf_tm_ppg_t *)
#define BF_TM_TOFINO_SET_PPG_INTF2(field) \
  bf_tm_status_t bf_tm_tofino_ppg_set_##field(bf_dev_id_t, bf_tm_ppg_t *)
#define BF_TM_TOFINO_GET_PPG_INTF(field) \
  bf_tm_status_t bf_tm_tofino_ppg_get_##field(bf_dev_id_t, bf_tm_ppg_t *)
#define BF_TM_TOFINO_GET_PPG_INTF2(field)      \
  bf_tm_status_t bf_tm_tofino_ppg_get_##field( \
      bf_dev_id_t, bf_tm_ppg_t *, uint64_t *)
#define BF_TM_TOFINO_GET_PPG_INTF_T(field, argtype) \
  bf_tm_status_t bf_tm_tofino_ppg_get_##field(      \
      bf_dev_id_t, bf_tm_ppg_t *, argtype *)
#define BF_TM_TOFINO_CLR_PPG_INTF(field) \
  bf_tm_status_t bf_tm_tofino_ppg_clear_##field(bf_dev_id_t, bf_tm_ppg_t *)
BF_TM_TOFINO_SET_PPG_INTF(min_limit);
BF_TM_TOFINO_GET_PPG_INTF(min_limit);
BF_TM_TOFINO_SET_PPG_INTF(skid_limit);
BF_TM_TOFINO_GET_PPG_INTF(skid_limit);
BF_TM_TOFINO_SET_PPG_INTF(app_limit);
BF_TM_TOFINO_GET_PPG_INTF(app_limit);
BF_TM_TOFINO_SET_PPG_INTF(hyst);
BF_TM_TOFINO_GET_PPG_INTF(hyst);
BF_TM_TOFINO_SET_PPG_INTF2(app_poolid);
BF_TM_TOFINO_GET_PPG_INTF(app_poolid);
BF_TM_TOFINO_SET_PPG_INTF(dynamic_mode);
BF_TM_TOFINO_GET_PPG_INTF(dynamic_mode);
BF_TM_TOFINO_SET_PPG_INTF(baf);
BF_TM_TOFINO_GET_PPG_INTF(baf);
BF_TM_TOFINO_GET_PPG_INTF(fast_recover_mode);
BF_TM_TOFINO_SET_PPG_INTF(icos_mask);
BF_TM_TOFINO_GET_PPG_INTF(icos_mask);
BF_TM_TOFINO_SET_PPG_INTF(pfc_treatment);
BF_TM_TOFINO_GET_PPG_INTF(pfc_treatment);
BF_TM_TOFINO_GET_PPG_INTF2(drop_counter);
BF_TM_TOFINO_GET_PPG_INTF2(gmin_usage_counter);
BF_TM_TOFINO_GET_PPG_INTF2(shared_usage_counter);
BF_TM_TOFINO_GET_PPG_INTF2(skid_usage_counter);
BF_TM_TOFINO_GET_PPG_INTF2(wm_counter);
BF_TM_TOFINO_GET_PPG_INTF(ppg_allocation);
bf_tm_status_t bf_tm_tofino_wac_get_buffer_full_counter(bf_dev_id_t devid,
                                                        bf_dev_pipe_t pipe,
                                                        uint64_t *count);
bf_tm_status_t bf_tm_tofino_wac_clear_buffer_full_counter(bf_dev_id_t devid,
                                                          bf_dev_pipe_t pipe);
bf_tm_status_t bf_tm_tofino_restore_wac_offset_profile(bf_dev_id_t devid);

BF_TM_TOFINO_GET_PPG_INTF_T(drop_state, bool);
BF_TM_TOFINO_GET_PPG_INTF_T(resume_limit, uint32_t);

BF_TM_TOFINO_CLR_PPG_INTF(drop_counter);
BF_TM_TOFINO_CLR_PPG_INTF(watermark);
BF_TM_TOFINO_CLR_PPG_INTF(wac_drop_state);
BF_TM_TOFINO_CLR_PPG_INTF(gmin_usage_counter);
BF_TM_TOFINO_CLR_PPG_INTF(shared_usage_counter);
BF_TM_TOFINO_CLR_PPG_INTF(skid_usage_counter);
BF_TM_TOFINO_CLR_PPG_INTF(resume_limit);

/////////////Queues///////////////

#define BF_TM_TOFINO_SET_Q_INTF(field) \
  bf_tm_status_t bf_tm_tofino_q_set_##field(bf_dev_id_t, bf_tm_eg_q_t *)
#define BF_TM_TOFINO_GET_Q_INTF(field) \
  bf_tm_status_t bf_tm_tofino_q_get_##field(bf_dev_id_t, bf_tm_eg_q_t *)
#define BF_TM_TOFINO_GET_Q_INTF2(field)      \
  bf_tm_status_t bf_tm_tofino_q_get_##field( \
      bf_dev_id_t, bf_tm_eg_q_t *, uint64_t *)
#define BF_TM_TOFINO_GET_Q_INTF_T(field, argtype) \
  bf_tm_status_t bf_tm_tofino_q_get_##field(      \
      bf_dev_id_t, bf_tm_eg_q_t *, argtype *)
#define BF_TM_TOFINO_CLR_Q_INTF(field) \
  bf_tm_status_t bf_tm_tofino_q_clear_##field(bf_dev_id_t, bf_tm_eg_q_t *)
BF_TM_TOFINO_SET_Q_INTF(min_limit);
BF_TM_TOFINO_GET_Q_INTF(min_limit);
BF_TM_TOFINO_SET_Q_INTF(app_limit);
BF_TM_TOFINO_GET_Q_INTF(app_limit);
BF_TM_TOFINO_SET_Q_INTF(app_hyst);
BF_TM_TOFINO_GET_Q_INTF(app_hyst);
BF_TM_TOFINO_SET_Q_INTF(yel_limit_pcent);
BF_TM_TOFINO_GET_Q_INTF(yel_limit_pcent);
BF_TM_TOFINO_SET_Q_INTF(red_limit_pcent);
BF_TM_TOFINO_GET_Q_INTF(red_limit_pcent);
BF_TM_TOFINO_SET_Q_INTF(yel_hyst);
BF_TM_TOFINO_GET_Q_INTF(yel_hyst);
BF_TM_TOFINO_SET_Q_INTF(red_hyst);
BF_TM_TOFINO_GET_Q_INTF(red_hyst);
BF_TM_TOFINO_SET_Q_INTF(app_poolid);
BF_TM_TOFINO_GET_Q_INTF(app_poolid);
BF_TM_TOFINO_SET_Q_INTF(is_dynamic);
BF_TM_TOFINO_GET_Q_INTF(is_dynamic);
BF_TM_TOFINO_SET_Q_INTF(baf);
BF_TM_TOFINO_GET_Q_INTF(baf);
BF_TM_TOFINO_GET_Q_INTF(fast_recover_mode);
BF_TM_TOFINO_SET_Q_INTF(color_drop_en);
BF_TM_TOFINO_GET_Q_INTF(color_drop_en);
BF_TM_TOFINO_SET_Q_INTF(tail_drop_en);
BF_TM_TOFINO_GET_Q_INTF(tail_drop_en);
BF_TM_TOFINO_SET_Q_INTF(neg_mir_dest);

BF_TM_TOFINO_GET_Q_INTF2(drop_counter);
BF_TM_TOFINO_GET_Q_INTF2(usage_counter);
BF_TM_TOFINO_GET_Q_INTF2(wm_counter);

BF_TM_TOFINO_GET_Q_INTF(neg_mir_dest);
BF_TM_TOFINO_CLR_Q_INTF(drop_counter);
BF_TM_TOFINO_CLR_Q_INTF(watermark);
BF_TM_TOFINO_GET_Q_INTF_T(skidpool_drop_state_shadow, uint32_t);
BF_TM_TOFINO_CLR_Q_INTF(skidpool_drop_state_shadow);
BF_TM_TOFINO_CLR_Q_INTF(usage_counter);

bf_tm_status_t bf_tm_tofino_q_carve_queues(
    bf_dev_id_t, bf_dev_port_t, bf_dev_pipe_t, int, bf_tm_q_profile_t *);
bf_tm_status_t bf_tm_tofino_get_q_profiles_mapping(
    bf_dev_id_t devid, bf_tm_q_profile_t *q_profile);
bf_tm_status_t bf_tm_tofino_get_q_profile_mapping(bf_dev_id_t,
                                                  int,
                                                  int,
                                                  bf_tm_q_profile_t *);
bf_tm_status_t bf_tm_tofino_get_port_q_profile(bf_dev_id_t,
                                               bf_dev_port_t,
                                               bf_dev_pipe_t,
                                               uint32_t *);
bf_tm_status_t bf_tm_tofino_restore_qac_offset_profile(bf_dev_id_t devid);
bf_tm_status_t bf_tm_tofino_get_phys_q(bf_dev_id_t devid,
                                       bf_dev_port_t devport,
                                       uint32_t ing_q,
                                       bf_dev_pipe_t *log_pipe,
                                       bf_tm_queue_t *phys_q);
bf_tm_status_t bf_tm_tofino_get_qac_qid_mapping(bf_dev_id_t devid,
                                                bf_dev_pipe_t pipe,
                                                uint32_t port,
                                                uint8_t *data);
bf_tm_status_t bf_tm_tofino_clear_qac_qid_mapping(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t entry);
bf_tm_status_t bf_tm_tofino_get_wac_eg_qid_mapping(bf_dev_id_t devid,
                                                   uint32_t entry,
                                                   uint32_t *val);
bf_tm_status_t bf_tm_tofino_clear_wac_eg_qid_mapping(bf_dev_id_t devid,
                                                     uint32_t entry);
bf_tm_status_t bf_tm_tofino_clear_qac_qid_profile(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t entry);
bf_tm_status_t bf_tm_tofino_q_get_egress_drop_state(bf_dev_id_t devid,
                                                    bf_tm_eg_q_t *q,
                                                    bf_tm_color_t color,
                                                    bool *state);
bf_tm_status_t bf_tm_tofino_q_clear_egress_drop_state(bf_dev_id_t devid,
                                                      bf_tm_eg_q_t *q,
                                                      bf_tm_color_t color);
//////////////////////IG-POOLS/////////////////

#define BF_TM_TOFINO_SET_IG_SPOOL_INTF(field)       \
  bf_tm_status_t bf_tm_tofino_ig_spool_set_##field( \
      bf_dev_id_t, uint8_t, bf_tm_ig_spool_t *)
#define BF_TM_TOFINO_GET_IG_SPOOL_INTF(field)       \
  bf_tm_status_t bf_tm_tofino_ig_spool_get_##field( \
      bf_dev_id_t, uint8_t, bf_tm_ig_spool_t *)
#define BF_TM_TOFINO_GET_IG_SPOOL_INTF_T(field, argtype) \
  bf_tm_status_t bf_tm_tofino_ig_spool_get_##field(      \
      bf_dev_id_t, uint8_t, argtype *)
#define BF_TM_TOFINO_CLR_IG_SPOOL_INTF(field) \
  bf_tm_status_t bf_tm_tofino_ig_spool_clear_##field(bf_dev_id_t, uint8_t)
#define BF_TM_TOFINO_SET_IG_RESUME_INTF(field)                  \
  bf_tm_status_t bf_tm_tofino_ig_spool_set_##field(bf_dev_id_t, \
                                                   bf_tm_ig_pool_t *)
#define BF_TM_TOFINO_GET_IG_RESUME_INTF(field)                  \
  bf_tm_status_t bf_tm_tofino_ig_spool_get_##field(bf_dev_id_t, \
                                                   bf_tm_ig_pool_t *)
BF_TM_TOFINO_SET_IG_SPOOL_INTF(red_limit);
BF_TM_TOFINO_GET_IG_SPOOL_INTF(red_limit);
BF_TM_TOFINO_SET_IG_RESUME_INTF(red_hyst);
BF_TM_TOFINO_GET_IG_RESUME_INTF(red_hyst);
BF_TM_TOFINO_SET_IG_SPOOL_INTF(yel_limit);
BF_TM_TOFINO_GET_IG_SPOOL_INTF(yel_limit);
BF_TM_TOFINO_SET_IG_RESUME_INTF(yel_hyst);
BF_TM_TOFINO_GET_IG_RESUME_INTF(yel_hyst);
BF_TM_TOFINO_SET_IG_SPOOL_INTF(green_limit);
BF_TM_TOFINO_GET_IG_SPOOL_INTF(green_limit);
BF_TM_TOFINO_SET_IG_RESUME_INTF(green_hyst);
BF_TM_TOFINO_GET_IG_RESUME_INTF(green_hyst);
BF_TM_TOFINO_SET_IG_SPOOL_INTF(color_drop_en);
BF_TM_TOFINO_GET_IG_SPOOL_INTF(color_drop_en);
BF_TM_TOFINO_GET_IG_SPOOL_INTF_T(usage, uint32_t);
BF_TM_TOFINO_GET_IG_SPOOL_INTF_T(wm, uint32_t);
BF_TM_TOFINO_CLR_IG_SPOOL_INTF(wm);
#define BF_TM_TOFINO_SET_IG_GPOOL_INTF(field)                   \
  bf_tm_status_t bf_tm_tofino_ig_gpool_set_##field(bf_dev_id_t, \
                                                   bf_tm_ig_gpool_t *)
#define BF_TM_TOFINO_GET_IG_GPOOL_INTF(field)                   \
  bf_tm_status_t bf_tm_tofino_ig_gpool_get_##field(bf_dev_id_t, \
                                                   bf_tm_ig_gpool_t *)
BF_TM_TOFINO_SET_IG_GPOOL_INTF(skid_limit);
BF_TM_TOFINO_GET_IG_GPOOL_INTF(skid_limit);
BF_TM_TOFINO_SET_IG_GPOOL_INTF(skid_hyst);
BF_TM_TOFINO_GET_IG_GPOOL_INTF(skid_hyst);
BF_TM_TOFINO_SET_IG_GPOOL_INTF(dod_limit);
BF_TM_TOFINO_GET_IG_GPOOL_INTF(dod_limit);

bf_tm_status_t bf_tm_tofino_ig_spool_set_pfc_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   uint8_t pfc,
                                                   bf_tm_ig_spool_t *ig_spool);
bf_tm_status_t bf_tm_tofino_ig_spool_get_pfc_limit(bf_dev_id_t devid,
                                                   uint8_t pool,
                                                   uint8_t pfc,
                                                   bf_tm_ig_spool_t *ig_spool);
bf_tm_status_t bf_tm_tofino_ig_gpool_set_uc_ct_size(bf_dev_id_t devid,
                                                    uint32_t cells);
bf_tm_status_t bf_tm_tofino_ig_gpool_set_mc_ct_size(bf_dev_id_t devid,
                                                    uint32_t cells);
bf_tm_status_t bf_tm_tofino_ig_gpool_get_uc_ct_size(bf_dev_id_t devid,
                                                    uint32_t *cells);
bf_tm_status_t bf_tm_tofino_ig_gpool_get_mc_ct_size(bf_dev_id_t devid,
                                                    uint32_t *cells);
bf_tm_status_t bf_tm_tofino_ig_spool_get_color_drop_state(bf_dev_id_t,
                                                          bf_tm_color_t,
                                                          uint32_t *);
bf_tm_status_t bf_tm_tofino_ig_spool_clear_color_drop_state(
    bf_dev_id_t devid, bf_tm_color_t color);

//////////////////////EG-POOLS/////////////////

#define BF_TM_TOFINO_SET_EG_SPOOL_INTF(field)       \
  bf_tm_status_t bf_tm_tofino_eg_spool_set_##field( \
      bf_dev_id_t, uint8_t, bf_tm_eg_spool_t *)
#define BF_TM_TOFINO_GET_EG_SPOOL_INTF(field)       \
  bf_tm_status_t bf_tm_tofino_eg_spool_get_##field( \
      bf_dev_id_t, uint8_t, bf_tm_eg_spool_t *)
#define BF_TM_TOFINO_SET_EG_RESUME_INTF(field)                  \
  bf_tm_status_t bf_tm_tofino_eg_spool_set_##field(bf_dev_id_t, \
                                                   bf_tm_eg_pool_t *)
#define BF_TM_TOFINO_GET_EG_RESUME_INTF(field)                  \
  bf_tm_status_t bf_tm_tofino_eg_spool_get_##field(bf_dev_id_t, \
                                                   bf_tm_eg_pool_t *)
#define BF_TM_TOFINO_GET_EG_SPOOL_INTF_T(field, argtype) \
  bf_tm_status_t bf_tm_tofino_eg_spool_get_##field(      \
      bf_dev_id_t, uint8_t, argtype *)
#define BF_TM_TOFINO_CLR_EG_SPOOL_INTF(field) \
  bf_tm_status_t bf_tm_tofino_eg_spool_clear_##field(bf_dev_id_t, uint8_t)
BF_TM_TOFINO_SET_EG_SPOOL_INTF(red_limit);
BF_TM_TOFINO_GET_EG_SPOOL_INTF(red_limit);
BF_TM_TOFINO_SET_EG_RESUME_INTF(red_hyst);
BF_TM_TOFINO_GET_EG_RESUME_INTF(red_hyst);
BF_TM_TOFINO_SET_EG_SPOOL_INTF(yel_limit);
BF_TM_TOFINO_GET_EG_SPOOL_INTF(yel_limit);
BF_TM_TOFINO_SET_EG_RESUME_INTF(yel_hyst);
BF_TM_TOFINO_GET_EG_RESUME_INTF(yel_hyst);
BF_TM_TOFINO_SET_EG_SPOOL_INTF(green_limit);
BF_TM_TOFINO_GET_EG_SPOOL_INTF(green_limit);
BF_TM_TOFINO_SET_EG_RESUME_INTF(green_hyst);
BF_TM_TOFINO_GET_EG_RESUME_INTF(green_hyst);
BF_TM_TOFINO_SET_EG_SPOOL_INTF(color_drop_en);
BF_TM_TOFINO_GET_EG_SPOOL_INTF(color_drop_en);

#define BF_TM_TOFINO_SET_EG_GPOOL_INTF(field)                   \
  bf_tm_status_t bf_tm_tofino_eg_gpool_set_##field(bf_dev_id_t, \
                                                   bf_tm_eg_gpool_t *)
#define BF_TM_TOFINO_GET_EG_GPOOL_INTF(field)                   \
  bf_tm_status_t bf_tm_tofino_eg_gpool_get_##field(bf_dev_id_t, \
                                                   bf_tm_eg_gpool_t *)
BF_TM_TOFINO_SET_EG_GPOOL_INTF(dod_limit);
BF_TM_TOFINO_GET_EG_GPOOL_INTF(dod_limit);

bf_tm_status_t bf_tm_tofino_eg_gpool_set_fifo_limit(bf_dev_id_t,
                                                    bf_dev_pipe_t,
                                                    uint8_t,
                                                    bf_tm_thres_t);
bf_tm_status_t bf_tm_tofino_eg_gpool_get_fifo_limit(bf_dev_id_t,
                                                    bf_dev_pipe_t,
                                                    uint8_t,
                                                    bf_tm_thres_t *);

BF_TM_TOFINO_GET_EG_SPOOL_INTF_T(usage, uint32_t);
BF_TM_TOFINO_GET_EG_SPOOL_INTF_T(wm, uint32_t);
BF_TM_TOFINO_CLR_EG_SPOOL_INTF(wm);
bf_tm_status_t bf_tm_tofino_eg_buffer_drop_state(bf_dev_id_t,
                                                 bf_tm_eg_buffer_drop_state_en,
                                                 uint32_t *);
bf_tm_status_t bf_tm_tofino_eg_buffer_drop_state_clear(
    bf_dev_id_t, bf_tm_eg_buffer_drop_state_en);

///////////////SCH////////////////////

#define BF_TM_TOFINO_SET_SCH_INTF(field) \
  bf_tm_status_t bf_tm_tofino_sch_set_##field(bf_dev_id_t, bf_tm_eg_q_t *)
#define BF_TM_TOFINO_GET_SCH_INTF(field) \
  bf_tm_status_t bf_tm_tofino_sch_get_##field(bf_dev_id_t, bf_tm_eg_q_t *)
#define BF_TM_TOFINO_SET_SCH_PORT_INTF(field) \
  bf_tm_status_t bf_tm_tofino_sch_set_##field(bf_dev_id_t, bf_tm_port_t *)
#define BF_TM_TOFINO_GET_SCH_PORT_INTF(field) \
  bf_tm_status_t bf_tm_tofino_sch_get_##field(bf_dev_id_t, bf_tm_port_t *)
#define BF_TM_TOFINO_SET_SCH_INTF2(field) \
  bf_tm_status_t bf_tm_tofino_sch_set_##field(bf_dev_id_t, void *)
#define BF_TM_TOFINO_GET_SCH_INTF2(field) \
  bf_tm_status_t bf_tm_tofino_sch_get_##field(bf_dev_id_t, void *)
BF_TM_TOFINO_SET_SCH_INTF(q_priority);
BF_TM_TOFINO_GET_SCH_INTF(q_priority);
BF_TM_TOFINO_SET_SCH_INTF(q_wt);
BF_TM_TOFINO_GET_SCH_INTF(q_wt);
BF_TM_TOFINO_SET_SCH_INTF(q_pfc_prio);
BF_TM_TOFINO_GET_SCH_INTF(q_pfc_prio);
BF_TM_TOFINO_SET_SCH_INTF(q_rate);
BF_TM_TOFINO_GET_SCH_INTF(q_rate);
BF_TM_TOFINO_SET_SCH_INTF(q_min_rate);
BF_TM_TOFINO_GET_SCH_INTF(q_min_rate);
BF_TM_TOFINO_SET_SCH_INTF(q_max_rate_enable_status);
BF_TM_TOFINO_GET_SCH_INTF(q_max_rate_enable_status);
BF_TM_TOFINO_SET_SCH_INTF(q_min_rate_enable_status);
BF_TM_TOFINO_GET_SCH_INTF(q_min_rate_enable_status);
BF_TM_TOFINO_SET_SCH_PORT_INTF(port_rate);
BF_TM_TOFINO_GET_SCH_PORT_INTF(port_rate);
BF_TM_TOFINO_SET_SCH_INTF2(pkt_ifg);
BF_TM_TOFINO_GET_SCH_INTF2(pkt_ifg);
BF_TM_TOFINO_SET_SCH_INTF(q_sched);
BF_TM_TOFINO_GET_SCH_INTF(q_sched);
BF_TM_TOFINO_SET_SCH_PORT_INTF(port_max_rate_enable_status);
BF_TM_TOFINO_GET_SCH_PORT_INTF(port_max_rate_enable_status);
BF_TM_TOFINO_SET_SCH_PORT_INTF(port_sched);
BF_TM_TOFINO_GET_SCH_PORT_INTF(port_sched);

bf_status_t bf_tm_tofino_sch_force_disable_port_sched(bf_dev_id_t devid,
                                                      bf_tm_port_t *p);

/* Functions to verify shaping restore config for HA unit test */
bool bf_tm_tofino_sch_verify_burst_size(bf_dev_id_t, uint32_t, uint32_t);
bool bf_tm_tofino_sch_verify_rate(bf_dev_id_t, uint32_t, uint32_t, bool);
bf_tm_status_t bf_tm_tofino_sch_get_port_egress_pfc_status(bf_dev_id_t devid,
                                                           bf_tm_port_t *port,
                                                           uint8_t *status);
bf_tm_status_t bf_tm_tofino_sch_get_q_egress_pfc_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q,
                                                        bool *status);
bf_tm_status_t bf_tm_tofino_sch_set_q_egress_pfc_status(bf_dev_id_t devid,
                                                        bf_tm_eg_q_t *q,
                                                        bool status);
bf_tm_status_t bf_tm_tofino_sch_clear_q_egress_pfc_status(bf_dev_id_t devid,
                                                          bf_tm_eg_q_t *q);

///////////////PORTS////////////////////
bf_tm_status_t bf_tm_tofino_add_new_port(bf_dev_id_t, bf_tm_port_t *);
bf_tm_status_t bf_tm_tofino_delete_port(bf_dev_id_t, bf_tm_port_t *);
bf_tm_status_t bf_tm_tofino_port_set_cpu_port(bf_dev_id_t, bf_tm_port_t *);

#define BF_TM_TOFINO_SET_PORT_INTF(field) \
  bf_tm_status_t bf_tm_tofino_port_set_##field(bf_dev_id_t, bf_tm_port_t *)
#define BF_TM_TOFINO_GET_PORT_INTF(field) \
  bf_tm_status_t bf_tm_tofino_port_get_##field(bf_dev_id_t, bf_tm_port_t *)
#define BF_TM_TOFINO_GET_PORT_INTF2(field)      \
  bf_tm_status_t bf_tm_tofino_port_get_##field( \
      bf_dev_id_t, bf_tm_port_t *, uint64_t *)
#define BF_TM_TOFINO_SET_PORT_INTF_T(field, argtype1, argtype2) \
  bf_tm_status_t bf_tm_tofino_port_set_##field(                 \
      bf_dev_id_t, bf_tm_port_t *, argtype1, argtype2)
#define BF_TM_TOFINO_GET_PORT_INTF_T(field, argtype) \
  bf_tm_status_t bf_tm_tofino_port_get_##field(      \
      bf_dev_id_t, bf_tm_port_t *, argtype *)
#define BF_TM_TOFINO_CLR_PORT_INTF(field) \
  bf_tm_status_t bf_tm_tofino_port_clear_##field(bf_dev_id_t, bf_tm_port_t *)

BF_TM_TOFINO_SET_PORT_INTF(wac_drop_limit);
BF_TM_TOFINO_SET_PORT_INTF(qac_drop_limit);
BF_TM_TOFINO_GET_PORT_INTF(wac_drop_limit);
BF_TM_TOFINO_GET_PORT_INTF(qac_drop_limit);
BF_TM_TOFINO_SET_PORT_INTF(wac_hyst_limit);
BF_TM_TOFINO_SET_PORT_INTF(qac_hyst_limit);
BF_TM_TOFINO_GET_PORT_INTF(wac_hyst_limit);
BF_TM_TOFINO_GET_PORT_INTF(qac_hyst_limit);
BF_TM_TOFINO_SET_PORT_INTF(uc_ct_limit);
BF_TM_TOFINO_GET_PORT_INTF(uc_ct_limit);
BF_TM_TOFINO_SET_PORT_INTF(flowcontrol_mode);
BF_TM_TOFINO_GET_PORT_INTF(flowcontrol_mode);
BF_TM_TOFINO_SET_PORT_INTF(flowcontrol_rx);
BF_TM_TOFINO_GET_PORT_INTF(flowcontrol_rx);
BF_TM_TOFINO_SET_PORT_INTF(pfc_cos_map);
BF_TM_TOFINO_GET_PORT_INTF(pfc_cos_mask);
BF_TM_TOFINO_SET_PORT_INTF(cut_through);
BF_TM_TOFINO_GET_PORT_INTF(cut_through);
BF_TM_TOFINO_SET_PORT_INTF(qac_rx);

BF_TM_TOFINO_GET_PORT_INTF2(ingress_drop_cntr);
BF_TM_TOFINO_GET_PORT_INTF2(egress_drop_cntr);
BF_TM_TOFINO_GET_PORT_INTF2(ingress_usage_cntr);
BF_TM_TOFINO_GET_PORT_INTF2(egress_usage_cntr);
BF_TM_TOFINO_GET_PORT_INTF2(ingress_wm_cntr);
BF_TM_TOFINO_GET_PORT_INTF2(egress_wm_cntr);
BF_TM_TOFINO_GET_PORT_INTF_T(wac_drop_state, bool);
BF_TM_TOFINO_GET_PORT_INTF_T(qac_drop_state, bool);
BF_TM_TOFINO_GET_PORT_INTF_T(pfc_state, uint8_t);
BF_TM_TOFINO_SET_PORT_INTF_T(pfc_state, uint8_t, bool);
BF_TM_TOFINO_CLR_PORT_INTF(ingress_drop_cntr);
BF_TM_TOFINO_CLR_PORT_INTF(ingress_usage_cntr);
BF_TM_TOFINO_CLR_PORT_INTF(egress_drop_cntr);
BF_TM_TOFINO_CLR_PORT_INTF(egress_usage_cntr);
BF_TM_TOFINO_CLR_PORT_INTF(qac_drop_state);
BF_TM_TOFINO_CLR_PORT_INTF(qac_drop_limit);
BF_TM_TOFINO_CLR_PORT_INTF(wac_drop_state);
BF_TM_TOFINO_CLR_PORT_INTF(pfc_state);
BF_TM_TOFINO_GET_PORT_INTF_T(pfc_enable_mask, uint8_t);

bf_tm_status_t bf_tm_tofino_port_get_egress_drop_color_cntr(bf_dev_id_t devid,
                                                            bf_tm_port_t *p,
                                                            bf_tm_color_t color,
                                                            uint64_t *count);
bf_tm_status_t bf_tm_tofino_port_get_pre_mask(bf_dev_id_t devid,
                                              uint32_t *mask_array,
                                              uint32_t size);
bf_tm_status_t bf_tm_tofino_port_clear_pre_mask(bf_dev_id_t devid);
bf_tm_status_t bf_tm_tofino_port_get_pre_down_mask(bf_dev_id_t devid,
                                                   uint32_t *mask_array,
                                                   uint32_t size);
bf_tm_status_t bf_tm_tofino_port_clear_pre_down_mask(bf_dev_id_t devid);

///////////////PIPE////////////////////
bf_tm_status_t bf_tm_tofino_set_egress_pipe_max_limit(bf_dev_id_t,
                                                      bf_dev_pipe_t,
                                                      uint32_t);
bf_tm_status_t bf_tm_tofino_get_egress_pipe_max_limit(bf_dev_id_t,
                                                      bf_dev_pipe_t,
                                                      uint32_t *);

bf_tm_status_t bf_tm_tofino_set_egress_pipe_hyst(bf_dev_id_t,
                                                 bf_dev_pipe_t,
                                                 uint32_t);
bf_tm_status_t bf_tm_tofino_get_egress_pipe_hyst(bf_dev_id_t,
                                                 bf_dev_pipe_t,
                                                 uint32_t *);
bf_tm_status_t bf_tm_tofino_pipe_get_total_in_cells(bf_dev_id_t devid,
                                                    bf_dev_pipe_t pipe,
                                                    uint64_t *count);
bf_tm_status_t bf_tm_tofino_pipe_clear_total_in_cells(bf_dev_id_t devid,
                                                      bf_dev_pipe_t pipe);

bf_tm_status_t bf_tm_tofino_pipe_get_total_in_pkts(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe,
                                                   uint64_t *count);
bf_tm_status_t bf_tm_tofino_pipe_clear_total_in_pkts(bf_dev_id_t devid,
                                                     bf_dev_pipe_t pipe);

bf_tm_status_t bf_tm_tofino_pipe_get_uc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint64_t *count);
bf_tm_status_t bf_tm_tofino_pipe_clear_uc_ct_count(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe);

bf_tm_status_t bf_tm_tofino_pipe_get_mc_ct_count(bf_dev_id_t devid,
                                                 bf_dev_pipe_t pipe,
                                                 uint64_t *count);
bf_tm_status_t bf_tm_tofino_pipe_clear_mc_ct_count(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe);

bf_tm_status_t bf_tm_tofino_set_timestamp_shift(bf_dev_id_t devid,
                                                uint8_t shift);
bf_tm_status_t bf_tm_tofino_get_timestamp_shift(bf_dev_id_t devid,
                                                uint8_t *shift);
bf_tm_status_t bf_tm_tofino_port_clear_ingress_watermark(bf_dev_id_t devid,
                                                         bf_tm_port_t *p);
bf_tm_status_t bf_tm_tofino_port_clear_egress_watermark(bf_dev_id_t devid,
                                                        bf_tm_port_t *p);

///////////////MCAST FIFO////////////////////
#define BF_TM_TOFINO_SET_MCFIFO_INTF(field)                       \
  bf_tm_status_t bf_tm_tofino_set_mcast_fifo_##field(bf_dev_id_t, \
                                                     bf_tm_mcast_fifo_t *)
#define BF_TM_TOFINO_GET_MCFIFO_INTF(field)                       \
  bf_tm_status_t bf_tm_tofino_get_mcast_fifo_##field(bf_dev_id_t, \
                                                     bf_tm_mcast_fifo_t *)

BF_TM_TOFINO_SET_MCFIFO_INTF(arbmode);
BF_TM_TOFINO_GET_MCFIFO_INTF(arbmode);
BF_TM_TOFINO_SET_MCFIFO_INTF(wrr_weight);
BF_TM_TOFINO_GET_MCFIFO_INTF(wrr_weight);
BF_TM_TOFINO_SET_MCFIFO_INTF(icos_bmap);
BF_TM_TOFINO_GET_MCFIFO_INTF(icos_bmap);
BF_TM_TOFINO_SET_MCFIFO_INTF(depth);
BF_TM_TOFINO_GET_MCFIFO_INTF(depth);

///////////////TM PM DROP ERROR DISCARD COUNTERS////////////////////

// bf_tm_blklvl_cntrs_t *s
bf_tm_status_t bf_tm_tofino_blklvl_get_drop_cntrs(
    bf_dev_id_t dev, bf_dev_pipe_t pipe, bf_tm_blklvl_cntrs_t *blk_cntrs);

// bf_tm_pre_fifo_cntrs_t
bf_tm_status_t bf_tm_tofino_pre_fifo_get_drop_cntrs(
    bf_dev_id_t dev, bf_tm_pre_fifo_cntrs_t *fifo_cntrs);

/*
 *      Clear Registers
 */
bf_tm_status_t bf_tm_tofino_blklvl_clr_drop_cntrs(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  uint32_t clear_mask);
// bf_tm_pre_fifo_cntrs_t
bf_tm_status_t bf_tm_tofino_pre_fifo_clr_drop_cntrs(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t fifo);
// Fetch the current credits for the requested port
bf_tm_status_t bf_tm_tofino_get_port_credits(bf_dev_id_t devid,
                                             bf_tm_port_t *p);
#endif
