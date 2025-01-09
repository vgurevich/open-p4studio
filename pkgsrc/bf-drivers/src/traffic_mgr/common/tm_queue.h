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
 *    related to queues of TM
 */

#ifndef __TM_QUEUE_H__
#define __TM_QUEUE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <bf_types/bf_types.h>

#define BF_TM_MAX_QUEUE_PER_PG (128)

typedef struct _bf_tm_q_profile {
  bool in_use;
  int base_q;  // Base q for the port within the PG
  int q_count;
  int ch_in_pg;  // channel in PG
  uint64_t q_mapping[BF_TM_MAX_QUEUE_PER_PG];
} bf_tm_q_profile_t;

typedef struct _bf_tm_q_thres {
  bf_tm_thres_t min_limit;
  bf_tm_thres_t app_limit;
  bf_tm_thres_t app_hyst;
  uint8_t app_hyst_index;   // Q hyst in shared pool is one of
                            // 32 (tofino) possible hyst
  uint8_t red_limit_pcent;  // Percentage of shared_limit
                            // when red color packets are
                            // tail dropped.
  uint8_t yel_limit_pcent;  // Percentage of shared_limit
                            // when yellow color packets are
                            // tail dropped.
  bf_tm_thres_t red_hyst;
  bf_tm_thres_t yel_hyst;
  uint8_t red_hyst_index;  // Hyst table index
  uint8_t yel_hyst_index;
} bf_tm_q_thres_t;

typedef struct _bf_tm_l1_sch {
  bool sch_enabled;
  bool pps;
  bool pri_prop;
  uint8_t cid;
  bf_tm_sched_prio_t max_rate_sch_prio;
  bf_tm_sched_prio_t min_rate_sch_prio;
  uint16_t dwrr_wt;  // Wt used in sched excess bw
  uint32_t max_rate;
  uint32_t min_rate;
  bool max_rate_enable;
  bool min_rate_enable;
  uint32_t min_burst_size;
  uint32_t max_burst_size;
} bf_tm_l1_sch_cfg_t;

typedef struct _bf_tm_eg_l1 {
  bool in_use;
  uint8_t p_pipe;
  uint8_t l_pipe;
  uint8_t pg;          // l1 belong to which PG
  uint8_t port;        // Port to which l1 belongs -- runtime.
  uint8_t uport;       // API level port which could be same as port or diferent
                       // based on the
                       // Tofino Chip Family
  uint8_t logical_l1;  // Numbered 0..# of port
  uint16_t physical_l1;  // Physical l1# within pipe
  bf_tm_l1_sch_cfg_t l1_sch_cfg;
} bf_tm_eg_l1_t;

typedef struct _bf_tm_eg_q_attrib {
  uint8_t app_poolid;  // poolid the queue belongs to
  bool is_dynamic;
  uint8_t baf;  // When Baf = 0, thres managed as static
  bool color_drop_en;
  bool tail_drop_en;
  bool fast_recover_mode;
  bool visible;
} bf_tm_q_cfg_t;

typedef struct _bf_tm_q_sch {
  bool sch_enabled;
  bool sch_pfc_enabled;
  bool pps;
  uint8_t cid;
  uint8_t pfc_prio;
  bf_tm_sched_prio_t max_rate_sch_prio;
  bf_tm_sched_prio_t min_rate_sch_prio;
  uint16_t dwrr_wt;  // Wt used in sched excess bw
  uint32_t max_rate;
  uint32_t min_rate;
  bool max_rate_enable;
  bool min_rate_enable;
  uint32_t min_burst_size;
  uint32_t max_burst_size;
  bf_tm_sched_shaper_prov_type_t sch_prov_type;
  bf_tm_sched_adv_fc_mode_t adv_fc_mode;  // Advanced Flow Control Mode.
} bf_tm_q_sch_cfg_t;

typedef struct _bf_tm_eg_q {
  bool in_use;
  bf_dev_pipe_t p_pipe;
  uint8_t l_pipe;     // 0..num_active_pipes
  uint8_t pg;         // Q belong to which PG
  uint8_t port;       // Port to which Q belongs -- runtime.
  uint8_t uport;      // API level port which could be same as port or diferent
                      // based on the
                      // Tofino Chip Family
  uint8_t logical_q;  // Numbered 0..#q within the PG
  uint16_t physical_q;  // Physical q# within pipe
  uint16_t hq_base;     // Hardware base Queue for this Virtual Queue
  uint8_t hq_per_vq;    // Hardware Queues allocated for this Virtual Queue
  bf_tm_eg_l1_t *l1;    // L1 node associated with this queue
  bf_tm_q_cfg_t q_cfg;
  bf_tm_q_thres_t thresholds;
  bf_tm_q_sch_cfg_t q_sch_cfg;

  // Cached Counters per Queue.
  tm_cache_counter_node_list_t *counter_state_list;
} bf_tm_eg_q_t;

#define BF_TM_Q_MIN_QUEUE_BLOCK (8)

// Prototypes of accessor functions

bf_status_t bf_tm_q_get_descriptor(bf_dev_id_t,
                                   bf_dev_port_t,
                                   bf_tm_queue_t,
                                   bf_tm_eg_q_t **);
bf_status_t bf_tm_q_get_qid_descriptor(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       uint8_t qid,
                                       bf_tm_eg_q_t **q);
bf_status_t bf_tm_q_get_port_queue_nr(bf_dev_id_t dev,
                                      bf_tm_eg_q_t *q,
                                      bf_tm_queue_t *queue_nr);
bf_status_t bf_tm_q_carve_queues(bf_dev_id_t,
                                 bf_dev_port_t,
                                 uint8_t,
                                 uint8_t *);
bf_status_t bf_tm_q_release_queues(bf_dev_id_t,
                                   bf_dev_port_t,
                                   int,
                                   bf_tm_q_profile_t *);

#define BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_q_set_##field(bf_dev_id_t, bf_tm_eg_q_t *, argtype)

#define BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_q_get_##field(                  \
      bf_dev_id_t, bf_tm_eg_q_t *, argtype *, argtype *)
#define BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO(field, argtype) \
  bf_tm_status_t bf_tm_q_get_##field(bf_dev_id_t, bf_tm_eg_q_t *, argtype *)
#define BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO_EXT(field, argtype) \
  bf_tm_status_t bf_tm_q_get_##field(                        \
      bf_dev_id_t, bf_subdev_id_t, bf_tm_eg_q_t *, argtype *)

#define BF_TM_Q_WR_ACCESSOR_FUNC_PROTO2(field) \
  bf_tm_status_t bf_tm_q_set_##field(bf_dev_id_t, bf_tm_eg_q_t *)

#define BF_TM_Q_RD_ACCESSOR_FUNC_PROTO2(field) \
  bf_tm_status_t bf_tm_q_get_##field(          \
      bf_dev_id_t, bf_dev_pipe_t, bf_tm_eg_q_t **, bf_tm_eg_q_t **)

#define BF_TM_Q_CLR_ACCESSOR_FUNC_PROTO(field) \
  bf_tm_status_t bf_tm_q_clear_##field(bf_dev_id_t, bf_tm_eg_q_t *)

BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(min_limit, bf_tm_thres_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(min_limit, bf_tm_thres_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(app_limit, bf_tm_thres_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(app_limit, bf_tm_thres_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(app_hyst, bf_tm_thres_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(app_hyst, bf_tm_thres_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(red_limit_pcent, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(red_limit_pcent, uint8_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(yel_limit_pcent, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(yel_limit_pcent, uint8_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(red_hyst, bf_tm_thres_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(red_hyst, bf_tm_thres_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(yel_hyst, bf_tm_thres_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(yel_hyst, bf_tm_thres_t);

BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(app_poolid, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(app_poolid, uint8_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(is_dynamic, bool);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(is_dynamic, bool);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(baf, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(baf, uint8_t);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(color_drop_en, bool);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(color_drop_en, bool);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(tail_drop_en, bool);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(tail_drop_en, bool);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO(visible, bool);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(visible, bool);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO2(mirror_on_drop_destination);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO2(mirror_on_drop_destination);

BF_TM_Q_WR_ACCESSOR_FUNC_PROTO2(qac_buffer);
BF_TM_Q_WR_ACCESSOR_FUNC_PROTO2(wac_buffer);

BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO(drop_counter, uint64_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO(usage_count, uint32_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO(wm_count, uint32_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO(drop_state_shadow, uint32_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO_EXT(drop_counter_ext, uint64_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO_EXT(usage_count_ext, uint32_t);
BF_TM_Q_CNTR_ACCESSOR_FUNC_PROTO_EXT(wm_count_ext, uint32_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(app_hyst_index, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(red_hyst_index, uint8_t);
BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(yel_hyst_index, uint8_t);

BF_TM_Q_RD_ACCESSOR_FUNC_PROTO(fast_recovery_mode, bool);

BF_TM_Q_CLR_ACCESSOR_FUNC_PROTO(watermark);
BF_TM_Q_CLR_ACCESSOR_FUNC_PROTO(drop_state_shadow);
BF_TM_Q_CLR_ACCESSOR_FUNC_PROTO(usage_count);

bf_tm_status_t bf_tm_q_get_egress_drop_state(bf_dev_id_t,
                                             bf_tm_eg_q_t *,
                                             bf_tm_color_t,
                                             bool *);
bf_tm_status_t bf_tm_q_clear_egress_drop_state(bf_dev_id_t,
                                               bf_tm_eg_q_t *,
                                               bf_tm_color_t);
bf_tm_status_t bf_tm_q_get_defaults(bf_dev_id_t, bf_tm_q_defaults_t *);
/* Q related HW read/write functions; Two functions (read, write)
 * corresponding to each ppg attribute/field.
 * The function pointers below are used in accessor Q function above.
 */
/*
 *     Function Pointers to program HW
 */
typedef bf_tm_status_t (*bf_tm_q_thres_wr_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_q_thres_rd_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_q_thres_rd_fptr2)(bf_dev_id_t);

/* Queue thresholds programming HW API table */
typedef struct _bf_tm_q_thres_hw_funcs {
  bf_tm_q_thres_wr_fptr min_limit_wr_fptr;
  bf_tm_q_thres_rd_fptr min_limit_rd_fptr;
  bf_tm_q_thres_wr_fptr app_limit_wr_fptr;
  bf_tm_q_thres_rd_fptr app_limit_rd_fptr;
  bf_tm_q_thres_wr_fptr app_hyst_wr_fptr;  // Index into offset_profile[]
  bf_tm_q_thres_rd_fptr app_hyst_rd_fptr;  // corresponding to this
                                           // value is to program
  bf_tm_q_thres_wr_fptr red_limit_pcent_wr_fptr;
  bf_tm_q_thres_rd_fptr red_limit_pcent_rd_fptr;
  bf_tm_q_thres_wr_fptr yel_limit_pcent_wr_fptr;
  bf_tm_q_thres_rd_fptr yel_limit_pcent_rd_fptr;

  bf_tm_q_thres_wr_fptr red_hyst_wr_fptr;
  bf_tm_q_thres_rd_fptr red_hyst_rd_fptr;
  bf_tm_q_thres_wr_fptr yel_hyst_wr_fptr;
  bf_tm_q_thres_rd_fptr yel_hyst_rd_fptr;

  bf_tm_q_thres_rd_fptr2 restore_qac_offset_profile_fptr;

} bf_tm_q_thres_hw_funcs_tbl;

typedef bf_tm_status_t (*bf_tm_qac_wr_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_qac_color_wr_fptr)(bf_dev_id_t,
                                                  bf_tm_eg_q_t *,
                                                  bf_tm_color_t);
typedef bf_tm_status_t (*bf_tm_qac_rd_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_qac_q_profile_fptr)(
    bf_dev_id_t, bf_dev_port_t, bf_dev_pipe_t, int, bf_tm_q_profile_t *);
typedef bf_tm_status_t (*bf_tm_qac_rd_q_profile_fptr)(bf_dev_id_t,
                                                      int,
                                                      int,
                                                      bf_tm_q_profile_t *);
typedef bf_tm_status_t (*bf_tm_qac_rd_q_profiles_fptr)(bf_dev_id_t,
                                                       bf_tm_q_profile_t *);
typedef bf_tm_status_t (*bf_tm_qac_port_q_profile_fptr)(bf_dev_id_t,
                                                        bf_dev_port_t,
                                                        bf_dev_pipe_t,
                                                        uint32_t *);
typedef bf_tm_status_t (*bf_tm_qac_cntr_fptr)(bf_dev_id_t,
                                              bf_tm_eg_q_t *,
                                              uint64_t *);
typedef bf_tm_status_t (*bf_tm_qac_cntr_fptr_ext)(bf_dev_id_t,
                                                  bf_subdev_id_t die_id,
                                                  bf_tm_eg_q_t *,
                                                  uint64_t *);
typedef bf_tm_status_t (*bf_tm_q_ing_qid_to_phys_q)(
    bf_dev_id_t, bf_dev_port_t, uint32_t, bf_dev_pipe_t *, bf_tm_queue_t *);

typedef bf_tm_status_t (*bf_tm_qac_dropstate_fptr)(bf_dev_id_t,
                                                   bf_tm_eg_q_t *,
                                                   bf_tm_color_t,
                                                   bool *);
typedef bf_tm_status_t (*bf_tm_qac_cntr_fptr2)(bf_dev_id_t,
                                               bf_tm_eg_q_t *,
                                               uint32_t *);
typedef bf_tm_status_t (*bf_tm_q_defaults_rd_fptr)(bf_dev_id_t,
                                                   bf_tm_q_defaults_t *);

typedef struct _bf_tm_q_cfg_hw_funcs {
  bf_tm_qac_wr_fptr app_poolid_wr_fptr;
  bf_tm_qac_rd_fptr app_poolid_rd_fptr;
  bf_tm_qac_wr_fptr is_dynamic_wr_fptr;
  bf_tm_qac_rd_fptr is_dynamic_rd_fptr;
  bf_tm_qac_wr_fptr baf_wr_fptr;
  bf_tm_qac_rd_fptr baf_rd_fptr;
  bf_tm_qac_rd_fptr q_get_fast_recovery;
  bf_tm_qac_wr_fptr color_drop_en_wr_fptr;
  bf_tm_qac_rd_fptr color_drop_en_rd_fptr;
  bf_tm_qac_wr_fptr tail_drop_en_wr_fptr;
  bf_tm_qac_rd_fptr tail_drop_en_rd_fptr;
  bf_tm_qac_wr_fptr q_neg_mirror_dest_wr_fptr;
  bf_tm_qac_rd_fptr q_neg_mirror_dest_rd_fptr;
  bf_tm_qac_wr_fptr q_visible_wr_fptr;
  bf_tm_qac_rd_fptr q_visible_rd_fptr;

  bf_tm_qac_q_profile_fptr q_carve_fptr;
  bf_tm_qac_q_profile_fptr q_release_fptr;
  bf_tm_q_ing_qid_to_phys_q q_ing_qid_to_phys_q;
  bf_tm_qac_cntr_fptr q_drop_cntr_fptr;
  bf_tm_qac_cntr_fptr_ext q_drop_cntr_fptr_ext;
  bf_tm_qac_dropstate_fptr q_dropstate_fptr;
  bf_tm_qac_color_wr_fptr q_dropstate_clr_fptr;
  bf_tm_qac_cntr_fptr q_usage_cntr_fptr;
  bf_tm_qac_cntr_fptr_ext q_usage_cntr_fptr_ext;
  bf_tm_qac_wr_fptr q_usage_cntr_clr_fptr;
  bf_tm_qac_cntr_fptr q_wm_cntr_fptr;
  bf_tm_qac_cntr_fptr_ext q_wm_cntr_fptr_ext;
  bf_tm_qac_wr_fptr q_wm_clr_fptr;
  bf_tm_qac_rd_q_profile_fptr q_profile_qid_map_fptr;
  bf_tm_qac_rd_q_profiles_fptr q_profiles_qid_map_fptr;
  bf_tm_qac_port_q_profile_fptr q_profile_port_fptr;
  bf_tm_qac_wr_fptr q_drop_cntr_clr_fptr;
  bf_tm_qac_cntr_fptr2 q_get_dropstate_shadow;
  bf_tm_qac_wr_fptr q_dropst_shadow_clr_fptr;
  bf_tm_q_defaults_rd_fptr q_get_defaults_fptr;
  bf_tm_qac_wr_fptr qac_buffer_wr_fptr;
  bf_tm_qac_wr_fptr wac_buffer_wr_fptr;

} bf_tm_q_cfg_hw_funcs_tbl;

bf_tm_status_t bf_tm_q_get_profiles_mapping(bf_dev_id_t dev_id,
                                            bf_tm_q_profile_t *q_profile);

bf_tm_status_t bf_tm_q_get_profile_mapping(bf_dev_id_t dev_id,
                                           int p_pipe,
                                           int profile_index,
                                           bf_tm_q_profile_t *q_profile);

bf_tm_status_t bf_tm_q_get_port_q_profile(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          int p_pipe,
                                          uint32_t *profile_index);

bf_tm_status_t bf_tm_q_get_base_queue(bf_dev_id_t dev_id,
                                      bf_dev_port_t port,
                                      int port_ch,
                                      int *base_q);

bf_tm_status_t bf_tm_q_clear_drop_counter(bf_dev_id_t dev_id, bf_tm_eg_q_t *q);
/* Function to restore QAC offset profile config from hardware */
bf_tm_status_t bf_tm_restore_qac_offset_profile(bf_dev_id_t dev_id);

bf_status_t bf_tm_q_set_cache_counters(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t port_q);
bf_tm_status_t bf_tm_q_get_pipe_physical_queue(bf_dev_id_t dev_id,
                                               bf_dev_port_t port,
                                               uint32_t ing_q,
                                               bf_dev_pipe_t *log_pipe,
                                               bf_tm_queue_t *phys_q);

#endif
