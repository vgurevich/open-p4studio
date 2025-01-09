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
 * This file contains APIs for client application to
 * read Traffic Manager drop, usage counters.
 */

#ifndef __TRAFFIC_MGR_CACHED_COUNTERS_H__
#define __TRAFFIC_MGR_CACHED_COUNTERS_H__

/* Timer interval for refreshing the counters */
#if DEVICE_IS_EMULATOR
// Because of slow ind_reads this is causing contention with UCLI
#define BF_TM_STATS_REFRESH_COUNTERS_INTERVAL_SECS (60 * 1000)
#else
#define BF_TM_STATS_REFRESH_COUNTERS_INTERVAL_SECS (1 * 1000)
#endif
/* Max number of counters to fresh per timer call */
#define BF_TM_NUM_STATS_REFRESH_NODES_INTERVAL 400

/**
 * Enumeration values to categorize counter
 */
typedef enum bf_tm_counter_type_enum_s {
  /* When counter value is qualified by this enum,
   * counter value is count of packets.
   */
  BF_TM_COUNTER_TYPE_PACKETCOUNT = 0,
  /* When counter value is qualified by this enum,
   * counter value is count of packet drops.
   */
  BF_TM_COUNTER_TYPE_PACKETDROP,
  /* When counter value is qualified by this enum,
   * counter value is count of packet errors occurred.
   */
  BF_TM_COUNTER_TYPE_PACKETERROR,
  /* When counter value is qualified by this enum,
   * counter value is count of internal errors.
   */
  BF_TM_COUNTER_TYPE_INTERNALERROR,
  /* When counter value is qualified by this enum,
   * counter value is count of internal resource count.
   */
  BF_TM_COUNTER_TYPE_INTERNALCOUNT,
  /* When counter value is qualified by this enum,
   * counter value is count of bytes.
   */
  BF_TM_COUNTER_TYPE_BYTECOUNT
} bf_tm_counter_type_enum_t;

typedef enum tm_counter_module_types_s {
  TM_PORT,
  TM_PPG,
  TM_QUEUE,
  TM_PIPE,
  TM_DEV,
  TM_MAX_COUNTER_MODULE_TYPES
} tm_counter_module_types_t;

#define TM_MIN_PORT_CTR_IDX 0
#define TM_MIN_PPG_CTR_IDX 100
#define TM_MIN_Q_CTR_IDX 200
#define TM_MIN_PIPE_CTR_IDX 300
#define TM_MIN_DEV_CTR_IDX 400

/*
 * The counter Id's that need Caching for Wrap around
 * Users can add more as needed per Module like PORT, PPG, Q etc..
 *
 */
typedef enum tm_counter_enum_s {
  /* Per Port Counters */
  TOTAL_PKTS_DROPPED_ON_PORT_INGRESS = TM_MIN_PORT_CTR_IDX,
  TOTAL_PKTS_DROPPED_ON_PORT_EGRESS,
  /* Per PPG Counters */
  TOTAL_PKTS_DROPPED_PER_PPG = TM_MIN_PPG_CTR_IDX,
  /* Per PPG Counters */
  TOTAL_PKTS_DROPPED_PER_Q = TM_MIN_Q_CTR_IDX,
  /* Per PIPE Counters */
  /* Per DEV Counters
   * Example FIFO Counters
   */
} tm_counter_enum_t;

#define TM_CACHE_CTR_PORT_REL_INDEX(_ctr_id_) ((_ctr_id_)-TM_MIN_PORT_CTR_IDX)

#define TM_CACHE_CTR_PPG_REL_INDEX(_ctr_id_) ((_ctr_id_)-TM_MIN_PPG_CTR_IDX)

#define TM_CACHE_CTR_Q_REL_INDEX(_ctr_id_) ((_ctr_id_)-TM_MIN_Q_CTR_IDX)

#define TM_CACHE_CTR_PIPE_REL_INDEX(_ctr_id_) ((_ctr_id_)-TM_MIN_PIPE_CTR_IDX)

#define TM_CACHE_CTR_DEV_REL_INDEX(_ctr_id_) ((_ctr_id_)-TM_MIN_DEV_CTR_IDX)

/*
 * Per TM Module 16 counters (Like Per Port 16)
 */

#define TM_MAX_COUNTER_IDS_PER_MODULE 0x10

// TM Timer States
typedef enum tm_counters_timer_state_enum_s {
  TM_TIMER_UNINITIALIZED = 0,
  TM_TIMER_INITIALIZED,
  TM_TIMER_TRANS_TO_RUN,
  TM_TIMER_RUNNING,
  TM_TIMER_TRANS_TO_STOP
} tm_counters_timer_state_enum_t;

typedef struct tm_counter_node_id_s {
  /* pipe id is logical pipe id [0 - num_active_pipes-1] inclusive */
  bf_dev_pipe_t pipe;
  /*
   * port id is logical port id relative to the pipe port is part of
   * port group with mutiple channels. chnl_id = (port_id % max_chnls_per_pg)
   */
  bf_dev_port_t port;
  /*
   * Priority Port Groups (PPG) per pipe
   */
  bf_tm_ppg_hdl ppg;
  /*
   * queue id per port
   */
  bf_tm_queue_t q;
} tm_counter_node_id_t;

struct _bf_tm_dev_cfg;

/*
 * This callback will get the current hardware value of this
 * counter requested. It is the responsibility of the caller
 * to check for wraps and make sure the current value includes
 * the accumulated counter, accounting for the wrap around
 *
 * If successful returns BF_SUCCESS and the current register value is
 * returned in the val
 * If error is returned the val is undefined
 */

typedef bf_tm_status_t (*tm_get_counter_val_fn)(tm_counter_enum_t ctr_id,
                                                tm_counter_node_id_t *node_id,
                                                struct _bf_tm_dev_cfg *tm_ctx,
                                                uint64_t *hw_val);

typedef struct tm_cache_counter_s {
  bool is_valid;
  tm_counter_enum_t counter_id;
  bf_tm_counter_type_enum_t counter_type;
  uint16_t interval;
  uint16_t max_reg_bits;
  uint64_t max_reg_ctr_val;
  tm_get_counter_val_fn get_ctr_val_fn;
} tm_cache_counter_t;

/*
 * if(cur_val > prev_val) { incr_count = (cur_val - prev_val)}
 * else { incr_count = (max_val - prev_val) + cur_val}
 */

typedef struct tm_cache_counter_val_s {
  uint64_t cur_reg_ctr_val;
  uint64_t prev_reg_ctr_val;
  uint64_t hw_cur_reg_ctr_val;
} tm_cache_counter_val_t;

typedef struct tm_cache_counter_node_list_s {
  struct tm_cache_counter_node_list_s *next;
  struct tm_cache_counter_node_list_s *prev;
  bool is_valid;
  // node details like {pipe, port, ppg, q}
  tm_counter_node_id_t node_id;
  tm_counter_module_types_t module_type;

  uint32_t num_valid_cntrs;

  // This structure is indexed by counterid per module
  tm_cache_counter_val_t counter_val[TM_MAX_COUNTER_IDS_PER_MODULE];
} tm_cache_counter_node_list_t;

typedef struct tm_cache_counters_mod_ctx_s {
  uint32_t max_num_of_nodes;
  // Number of counter nodes for this Module
  uint32_t num_valid_ctr_nodes;
  // Node list of all the Cached Counters Per Module
  tm_cache_counter_node_list_t *valid_ctr_node_list;
  // Number of counterid's valid for this Module
  uint32_t num_valid_ctrs_ids;
  // The base counter id that will be used to find the relative index
  // of the counter
  uint32_t base_ctr_id;
  // The counter id and all its related properties
  tm_cache_counter_t ctr_id_list[TM_MAX_COUNTER_IDS_PER_MODULE];
} tm_cache_counters_mod_ctx_t;

typedef struct tm_cache_counters_ctx_s {
  tm_counters_timer_state_enum_t timer_state;
  bf_sys_timer_t ctr_timer;
  tm_cache_counters_mod_ctx_t cache_counters_info[TM_MAX_COUNTER_MODULE_TYPES];

} tm_cache_counters_ctx_t;

bf_tm_status_t tm_init_cached_counters(struct _bf_tm_dev_cfg *tm_ctx);
bf_tm_status_t tm_uninit_cached_counters(struct _bf_tm_dev_cfg *tm_ctx);

bf_tm_status_t tm_start_cached_counters_timer(struct _bf_tm_dev_cfg *tm_ctx);
bf_tm_status_t tm_stop_cached_counters_timer(struct _bf_tm_dev_cfg *tm_ctx);

bf_tm_status_t tm_allocate_and_init_cache_counter_node(
    tm_counter_module_types_t mod_type,
    struct _bf_tm_dev_cfg *tm_ctx,
    tm_counter_node_id_t *node_id,
    tm_cache_counter_node_list_t **node_list_ptr);

bf_tm_status_t tm_free_cache_counter_node(
    tm_counter_module_types_t mod_type,
    tm_cache_counter_node_list_t *ctr_node,
    struct _bf_tm_dev_cfg *tm_ctx);

#endif
