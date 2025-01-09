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


#include <stdint.h>
#include <stdbool.h>
#include <target-utils/bit_utils/bit_utils.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include "traffic_mgr/common/tm_ctx.h"
#include <traffic_mgr/traffic_mgr_counters.h>

#ifdef TM_LINEAR_COUNTERS_MAP

#define TM_COUNTER_PPG_INDEX(ctx, pipe, ppg_n) \
  ((pipe * ctx->tm_cfg.total_ppg_per_pipe) + ppg_n)

#define TM_COUNTER_PORT_INDEX(ctx, port)                    \
  ((DEV_PORT_TO_PIPE(port) *                                \
    ((ctx->tm_cfg.ports_per_pg * ctx->tm_cfg.pg_per_pipe) + \
     ctx->tm_cfg.mirror_port_cnt)) +                        \
   DEV_PORT_TO_LOCAL_PORT(port))

#define BF_TM_FIRST_Q_INDEX_IN_PG(ctx, port)                    \
  ((DEV_PORT_TO_PIPE(port) * ctx->tm_cfg.q_per_pipe) +          \
   ((DEV_PORT_TO_LOCAL_PORT(port) / ctx->tm_cfg.ports_per_pg) * \
    ctx->tm_cfg.q_per_pg))

#endif  // TM_LINEAR_COUNTERS_MAP

bf_tm_status_t tm_port_get_counter_val(tm_counter_enum_t ctr_id,
                                       tm_counter_node_id_t *node_id,
                                       struct _bf_tm_dev_cfg *tm_ctx,
                                       uint64_t *hw_val) {
  bf_tm_status_t rc = BF_INVALID_ARG;
  uint64_t ig_count = 0;
  uint64_t eg_count = 0;
  switch (ctr_id) {
    case TOTAL_PKTS_DROPPED_ON_PORT_INGRESS:
      rc = bf_tm_port_ingress_drop_get(
          tm_ctx->devid, node_id->pipe, node_id->port, &ig_count);
      *hw_val = ig_count;
      break;
    case TOTAL_PKTS_DROPPED_ON_PORT_EGRESS:
      rc = bf_tm_port_egress_drop_get(
          tm_ctx->devid, node_id->pipe, node_id->port, &eg_count);
      *hw_val = eg_count;
      break;
    default:
      break;
  }
  return rc;
}

bf_tm_status_t tm_ppg_get_counter_val(tm_counter_enum_t ctr_id,
                                      tm_counter_node_id_t *node_id,
                                      struct _bf_tm_dev_cfg *tm_ctx,
                                      uint64_t *hw_val) {
  bf_tm_status_t rc = BF_INVALID_ARG;

  switch (ctr_id) {
    case TOTAL_PKTS_DROPPED_PER_PPG:
      rc = bf_tm_ppg_drop_get(
          tm_ctx->devid, node_id->pipe, node_id->ppg, hw_val);
      break;
    default:
      break;
  }
  return rc;
}

bf_tm_status_t tm_queue_get_counter_val(tm_counter_enum_t ctr_id,
                                        tm_counter_node_id_t *node_id,
                                        struct _bf_tm_dev_cfg *tm_ctx,
                                        uint64_t *hw_val) {
  bf_tm_status_t rc = BF_INVALID_ARG;

  switch (ctr_id) {
    case TOTAL_PKTS_DROPPED_PER_Q:
      rc = bf_tm_q_drop_get(
          tm_ctx->devid, node_id->pipe, node_id->port, node_id->q, hw_val);
      break;
    default:
      break;
  }
  return rc;
}

static uint32_t tm_cache_max_counter_per_module(
    bf_tm_dev_ctx_t *tm_ctx, tm_counter_module_types_t ctr_mod) {
  if (tm_ctx == NULL) {
    LOG_ERROR("TM: %s:%d NULL Timer callback data passed", __func__, __LINE__);
    return 0;
  }

  uint32_t max_val = 0;

  switch (ctr_mod) {
    case TM_PORT:
      max_val = (tm_ctx->tm_cfg.pipe_cnt *
                 (tm_ctx->tm_cfg.mirror_port_cnt +
                  (tm_ctx->tm_cfg.ports_per_pg * tm_ctx->tm_cfg.pg_per_pipe)));
      break;
    case TM_PPG:
      max_val = (tm_ctx->tm_cfg.pipe_cnt * tm_ctx->tm_cfg.total_ppg_per_pipe);
      break;
    case TM_QUEUE:
      max_val = (tm_ctx->tm_cfg.pipe_cnt * tm_ctx->tm_cfg.q_per_pipe);
      break;
    default:
      break;
  }
  return max_val;
}

#ifdef TM_LINEAR_COUNTERS_MAP

static bf_status_t bf_tm_counter_q_index(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t port_q,
                                         uint32_t *q_index) {
  bf_status_t rc = BF_SUCCESS;
  int q_profile_index;
  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev]->q_profile;

  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof += q_profile_index;
  *q_index =
      BF_TM_FIRST_Q_INDEX_IN_PG(g_tm_ctx[dev], port) + q_prof->base_q + port_q;
  return (rc);
}

static tm_counter_module_types_t tm_get_module_from_counter_id(
    tm_counter_enum_t ctr_id, bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx == NULL) {
    LOG_ERROR("TM: %s:%d NULL Timer callback data passed", __func__, __LINE__);
    return 0;
  }

  if (ctr_id >= TM_MIN_PORT_CTR_IDX && ctr_id < TM_MIN_PPG_CTR_IDX) {
    return TM_PORT;
  } else if (ctr_id >= TM_MIN_PPG_CTR_IDX && ctr_id < TM_MIN_Q_CTR_IDX) {
    return TM_PPG;
  } else if (ctr_id >= TM_MIN_Q_CTR_IDX && ctr_id < TM_MIN_PIPE_CTR_IDX) {
    return TM_QUEUE;
  } else if (ctr_id >= TM_MIN_PIPE_CTR_IDX && ctr_id < TM_MIN_DEV_CTR_IDX) {
    return TM_PIPE;
  } else {
    return TM_DEV;
  }
}

// Should be requested under TM_LOCK. The returned pointer is only valid
// under the lock.
static bf_tm_status_t tm_node_id_to_mod_entry_index(
    tm_counter_module_types_t ctr_mod,
    tm_counter_node_id_t *node_id,
    bf_tm_dev_ctx_t *tm_ctx,
    uint32_t *index) {
  bf_tm_status_t status = BF_SUCCESS;

  switch (ctr_mod) {
    case TM_PORT:
      *index = TM_COUNTER_PORT_INDEX(tm_ctx, node_id->port);
      break;
    case TM_PPG:
      *index = TM_COUNTER_PPG_INDEX(tm_ctx, node_id->pipe, node_id->ppg);
      break;
    case TM_QUEUE:
      bf_tm_counter_q_index(tm_ctx->devid, node_id->port, node_id->q, index);
      break;
    default:
      status = BF_INVALID_ARG;
      break;
  }
  return status;
}

#endif  // #ifdef TM_LINEAR_COUNTERS_MAP

bf_tm_status_t tm_add_cache_counter(tm_counter_module_types_t mod_type,
                                    tm_counter_enum_t ctr_id,
                                    bf_tm_counter_type_enum_t ctr_type,
                                    uint16_t max_reg_bits,
                                    tm_get_counter_val_fn ctr_get_fn,
                                    bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx == NULL) {
    LOG_ERROR("TM: %s:%d NULL TM_CTX passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  if (mod_type >= TM_MAX_COUNTER_MODULE_TYPES) {
    LOG_ERROR("TM: %s:%d Invalid Mod Type %d", __func__, __LINE__, mod_type);
    return BF_INVALID_ARG;
  }

  tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
      &tm_ctx->cache_counters.cache_counters_info[mod_type];

  uint32_t rel_ctr_id = ctr_id - ctr_mod_ctx->base_ctr_id;
  if (rel_ctr_id >= TM_MAX_COUNTER_IDS_PER_MODULE) {
    // Counter ID beyond the supported value. Error
    LOG_ERROR("TM: %s:%d Counter Index passed %d rel_ctr_id %d ",
              __func__,
              __LINE__,
              ctr_id,
              rel_ctr_id);
    return BF_INVALID_ARG;
  }

  TM_LOCK(tm_ctx->devid, tm_ctx->lock);
  if (!ctr_mod_ctx->ctr_id_list[rel_ctr_id].is_valid) {
    // Only update the values if its not valid (not being used currently)
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].counter_id = ctr_id;
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].counter_type = ctr_type;
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].max_reg_bits = max_reg_bits;
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].max_reg_ctr_val =
        ((uint64_t)1 << max_reg_bits) - 1;
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].get_ctr_val_fn = ctr_get_fn;
    ctr_mod_ctx->ctr_id_list[rel_ctr_id].is_valid = true;
    ctr_mod_ctx->num_valid_ctrs_ids++;

    ctr_mod_ctx->ctr_id_list[rel_ctr_id].interval =
        ((max_reg_bits <= 32) ? 1 : 2);
  }
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  return BF_SUCCESS;
}

bf_tm_status_t tm_remove_cache_counter(tm_counter_module_types_t mod_type,
                                       tm_counter_enum_t ctr_id,
                                       bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx == NULL) {
    LOG_ERROR("TM: %s:%d NULL TM_CTX passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  if (mod_type >= TM_MAX_COUNTER_MODULE_TYPES) {
    LOG_ERROR("TM: %s:%d Invalid Mod Type %d", __func__, __LINE__, mod_type);
    return BF_INVALID_ARG;
  }

  tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
      &tm_ctx->cache_counters.cache_counters_info[mod_type];

  uint32_t rel_ctr_id = ctr_id - ctr_mod_ctx->base_ctr_id;
  if (rel_ctr_id >= TM_MAX_COUNTER_IDS_PER_MODULE) {
    // Counter ID beyond the supported value. Error
    LOG_ERROR("TM: %s:%d Counter Index passed %d rel_ctr_id %d ",
              __func__,
              __LINE__,
              ctr_id,
              rel_ctr_id);
    return BF_INVALID_ARG;
  }

  TM_LOCK(tm_ctx->devid, tm_ctx->lock);
  ctr_mod_ctx->ctr_id_list[rel_ctr_id].is_valid = false;
  ctr_mod_ctx->num_valid_ctrs_ids--;
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  return BF_SUCCESS;
}

bf_tm_status_t tm_allocate_and_init_cache_counter_node(
    tm_counter_module_types_t mod_type,
    bf_tm_dev_ctx_t *tm_ctx,
    tm_counter_node_id_t *node_id,
    tm_cache_counter_node_list_t **node_list_ptr) {
  if (tm_ctx == NULL || node_id == NULL) {
    LOG_ERROR("TM: %s:%d NULL TM_CTX or NodeId passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  tm_cache_counter_node_list_t *ctr_node = NULL;

  if (tm_ctx == NULL) {
    LOG_ERROR("TM: %s:%d NULL TM_CTX passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  if (mod_type >= TM_MAX_COUNTER_MODULE_TYPES) {
    LOG_ERROR("TM: %s:%d Invalid Mod Type %d", __func__, __LINE__, mod_type);
    return BF_INVALID_ARG;
  }

  tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
      &tm_ctx->cache_counters.cache_counters_info[mod_type];

  ctr_node = TRAFFIC_MGR_CALLOC(1, sizeof(tm_cache_counter_node_list_t));
  if (ctr_node == NULL) {
    LOG_ERROR("TM: %s:%d tm_cache_counter_node_list_t allocation failed",
              __func__,
              __LINE__);
    return BF_NO_SYS_RESOURCES;
  }

  ctr_node->is_valid = false;
  ctr_node->node_id = *node_id;
  ctr_node->module_type = mod_type;
  ctr_node->num_valid_cntrs = 0;
  *node_list_ptr = ctr_node;

  // Link this node into the Node list for the timer to work on it.
  TM_LOCK(tm_ctx->devid, tm_ctx->lock);
  BF_LIST_DLL_AP(ctr_mod_ctx->valid_ctr_node_list, ctr_node, next, prev);
  ctr_node->is_valid = true;
  ctr_mod_ctx->num_valid_ctr_nodes++;
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  return BF_SUCCESS;
}

bf_tm_status_t tm_free_cache_counter_node(
    tm_counter_module_types_t mod_type,
    tm_cache_counter_node_list_t *ctr_node,
    bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx == NULL || ctr_node == NULL) {
    LOG_ERROR("TM: %s:%d NULL TM_CTX or Ctr_Node passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
      &tm_ctx->cache_counters.cache_counters_info[mod_type];

  // Unlink the Counters Node so the Timer will not work on it.
  TM_LOCK(tm_ctx->devid, tm_ctx->lock);
  ctr_node->is_valid = false;
  BF_LIST_DLL_REM(ctr_mod_ctx->valid_ctr_node_list, ctr_node, next, prev);
  ctr_mod_ctx->num_valid_ctr_nodes--;
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  TRAFFIC_MGR_FREE(ctr_node);
  return BF_SUCCESS;
}

void tm_counter_refresh_timer_cb(bf_sys_timer_t *timer, void *data) {
  bf_dev_id_t dev_id = (bf_dev_id_t)(uintptr_t)data;
  bf_tm_status_t rc = BF_SUCCESS;
#if defined(DEVICE_IS_EMULATOR)
  return;
#endif

  (void)timer;

  static uint64_t timer_tick_count = 0;

  for (int i = 0; i < TM_MAX_COUNTER_MODULE_TYPES; i++) {
    if (!g_tm_ctx_valid[dev_id]) {
      LOG_DBG("TM: %s:%d tm_ctx Lock not initialized devid %d",
              __func__,
              __LINE__,
              dev_id);
      return;
    }

    TM_MUTEX_LOCK(&g_tm_timer_lock[dev_id]);

    bf_tm_dev_ctx_t *tm_ctx = g_tm_ctx[dev_id];

    if (tm_ctx == NULL) {
      TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev_id]);
      // The tm_ctx is deleted so we shouldnt touch any of the tm_ctx values
      // return with debug message
      LOG_DBG("TM: %s:%d tm_ctx got deleted for devid %d",
              __func__,
              __LINE__,
              dev_id);
      return;
    }

    TM_LOCK(dev_id, tm_ctx->lock);
    // Make sure that the timer_stop is not called while this timer_fn is
    // scheduled. We dont need to relaunch thus timer while its being stopped
    if (tm_ctx->cache_counters.timer_state != TM_TIMER_RUNNING) {
      TM_UNLOCK(dev_id, tm_ctx->lock);
      TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev_id]);
      LOG_DBG("TM: %s:%d Timer_CB existing since timer is stopping devid %d",
              __func__,
              __LINE__,
              dev_id);
      return;
    }

    tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
        &tm_ctx->cache_counters.cache_counters_info[i];

    // Check if any Counters are actually Added or if any Counter Nodes are
    // Present
    if (ctr_mod_ctx->num_valid_ctrs_ids > 0 &&
        ctr_mod_ctx->valid_ctr_node_list != NULL) {
      tm_cache_counter_node_list_t *iter_list =
          ctr_mod_ctx->valid_ctr_node_list;
      tm_cache_counter_node_list_t *iter_next = NULL;

      uint32_t num_nodes = 0;
      // Iterate through all the nodes and update counters
      while (iter_list != NULL) {
        iter_next = iter_list->next;

        if (iter_list->is_valid) {
          tm_cache_counter_t *ctr_id_list = ctr_mod_ctx->ctr_id_list;
          uint32_t num_ctrs = 0;

          // Iterate through all the counters for differnt modules
          for (int ctr_idx = 0; ctr_idx < TM_MAX_COUNTER_IDS_PER_MODULE;
               ctr_idx++) {
            if (ctr_id_list->is_valid) {
              TRAFFIC_MGR_DBGCHK(ctr_id_list->interval != 0);
              if (timer_tick_count % ctr_id_list->interval) {
                // we only need to get stats only in n interval. Sice the
                // counters wont rollover before that.
                continue;
              }

              uint32_t abs_ctr_id = ctr_idx + ctr_mod_ctx->base_ctr_id;

              tm_cache_counter_val_t *ctr_val =
                  &iter_list->counter_val[ctr_idx];

              rc = ctr_id_list->get_ctr_val_fn(abs_ctr_id,
                                               &iter_list->node_id,
                                               tm_ctx,
                                               &ctr_val->hw_cur_reg_ctr_val);

              if (rc == BF_SUCCESS) {
                /* If successful, lets calculate the current counter value
                 * accounting for the counter wrap.
                 * cur_reg_ctr_val holds the previous counter value that
                 * was calculated. It is used to calculate the new value
                 *
                 * if(cur_val >= prev_val) { incr_count = (cur_val - prev_val)}
                 * else { incr_count = (max_val - prev_val) + cur_val}
                 */

                uint64_t incr_count = 0;
                if (ctr_val->hw_cur_reg_ctr_val >= ctr_val->prev_reg_ctr_val) {
                  incr_count =
                      ctr_val->hw_cur_reg_ctr_val - ctr_val->prev_reg_ctr_val;
                } else {
                  incr_count = (ctr_id_list->max_reg_ctr_val -
                                ctr_val->prev_reg_ctr_val) +
                               ctr_val->hw_cur_reg_ctr_val;
                }
                ctr_val->cur_reg_ctr_val += incr_count;
                ctr_val->prev_reg_ctr_val = ctr_val->hw_cur_reg_ctr_val;
              }
              // Check for the number of valid counters already processed and
              // we are done then we can just break form this ctr_id loop
              num_ctrs++;
              if (ctr_mod_ctx->num_valid_ctrs_ids == num_ctrs) {
                // Accounted for all the valid counters we can break now
                break;
              }
            }
            // Get the next counter id
            ctr_id_list++;
          }
        }

        num_nodes++;
        if (num_nodes >= ctr_mod_ctx->num_valid_ctr_nodes ||
            num_nodes >= BF_TM_NUM_STATS_REFRESH_NODES_INTERVAL) {
          // Processed enough nodes, lets exit the timer
          break;
        }
        // Remove the Node and add it at the end once processing is done
        BF_LIST_DLL_REM(
            ctr_mod_ctx->valid_ctr_node_list, iter_list, next, prev);
        // Add the Node back into the list
        BF_LIST_DLL_AP(ctr_mod_ctx->valid_ctr_node_list, iter_list, next, prev);

        // Check for the next node
        iter_list = iter_next;
      }
    }
    TM_UNLOCK(dev_id, tm_ctx->lock);
    TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev_id]);
  }

  timer_tick_count++;
}

static void _tm_reset_cache_counters(bf_tm_dev_ctx_t *tm_ctx) {
  for (int i = 0; i < TM_MAX_COUNTER_MODULE_TYPES; i++) {
    tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
        &tm_ctx->cache_counters.cache_counters_info[i];

    // Hold the lock just incase this reset function gets called from
    // other modules to cleanup stuff.
    TM_LOCK(tm_ctx->devid, tm_ctx->lock);

    // Get the Max number of nodes that will be supported for each Module type
    TRAFFIC_MGR_MEMSET(
        ctr_mod_ctx->ctr_id_list,
        0,
        sizeof(tm_cache_counter_t) * TM_MAX_COUNTER_IDS_PER_MODULE);

    ctr_mod_ctx->num_valid_ctrs_ids = 0;
    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
  }
}

/*
 *The counters to be added at init time before the timer starts are
 */
static struct _tm_add_cache_counter_s {
  tm_counter_module_types_t module_type;
  tm_counter_enum_t counter_id;
  bf_tm_counter_type_enum_t counter_type;
  uint16_t max_reg_bits;
  tm_get_counter_val_fn get_ctr_val_fn;
} _tm_cache_counters[] = {
    // Port Counters
    {TM_PORT,
     TOTAL_PKTS_DROPPED_ON_PORT_INGRESS,
     BF_TM_COUNTER_TYPE_PACKETDROP,
     36,
     tm_port_get_counter_val},

    {TM_PORT,
     TOTAL_PKTS_DROPPED_ON_PORT_EGRESS,
     BF_TM_COUNTER_TYPE_PACKETDROP,
     47,
     tm_port_get_counter_val},
    // PPG Counters
    {TM_PPG,
     TOTAL_PKTS_DROPPED_PER_PPG,
     BF_TM_COUNTER_TYPE_PACKETDROP,
     40,
     tm_ppg_get_counter_val},
    // Queue Counters
    {TM_QUEUE,
     TOTAL_PKTS_DROPPED_PER_Q,
     BF_TM_COUNTER_TYPE_PACKETDROP,
     47,
     tm_queue_get_counter_val},
};

bf_tm_status_t tm_init_cached_counters(bf_tm_dev_ctx_t *tm_ctx) {
  if (!tm_ctx) {
    LOG_ERROR("TM: %s:%d Invalid tm_ctx passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  if (tm_ctx->cache_counters.timer_state != TM_TIMER_UNINITIALIZED) {
    // Timer already running return success
    return BF_SUCCESS;
  }
  tm_ctx->cache_counters.timer_state = TM_TIMER_UNINITIALIZED;

  bf_tm_status_t rc;

  for (int i = 0; i < TM_MAX_COUNTER_MODULE_TYPES; i++) {
    tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
        &tm_ctx->cache_counters.cache_counters_info[i];

    // Get the Max number of nodes that will be supported for each Module type
    TRAFFIC_MGR_MEMSET(
        ctr_mod_ctx->ctr_id_list,
        0,
        sizeof(tm_cache_counter_t) * TM_MAX_COUNTER_IDS_PER_MODULE);

    // Get the Max number of nodes that will be supported for each Module type
    ctr_mod_ctx->max_num_of_nodes = tm_cache_max_counter_per_module(tm_ctx, i);
    ctr_mod_ctx->valid_ctr_node_list = NULL;
    ctr_mod_ctx->num_valid_ctrs_ids = 0;
    ctr_mod_ctx->num_valid_ctr_nodes = 0;

    switch (i) {
      case TM_PORT:
        ctr_mod_ctx->base_ctr_id = TM_MIN_PORT_CTR_IDX;
        break;
      case TM_PPG:
        ctr_mod_ctx->base_ctr_id = TM_MIN_PPG_CTR_IDX;
        break;
      case TM_QUEUE:
        ctr_mod_ctx->base_ctr_id = TM_MIN_Q_CTR_IDX;
        break;
      case TM_PIPE:
        ctr_mod_ctx->base_ctr_id = TM_MIN_PIPE_CTR_IDX;
        break;
      case TM_DEV:
        ctr_mod_ctx->base_ctr_id = TM_MIN_DEV_CTR_IDX;
        break;
    }
  }

  int num_add_counters =
      sizeof(_tm_cache_counters) / sizeof(struct _tm_add_cache_counter_s);

  for (int i = 0; i < num_add_counters; i++) {
    tm_counter_module_types_t module_type = _tm_cache_counters[i].module_type;

    rc = tm_add_cache_counter(module_type,
                              _tm_cache_counters[i].counter_id,
                              _tm_cache_counters[i].counter_type,
                              _tm_cache_counters[i].max_reg_bits,
                              _tm_cache_counters[i].get_ctr_val_fn,
                              tm_ctx);

    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: %s:%d counter add failed devid %d mod_type %d"
          " counter id %d counter type %d max_bits %d status 0x%x",
          __func__,
          __LINE__,
          tm_ctx->devid,
          module_type,
          _tm_cache_counters[i].counter_id,
          _tm_cache_counters[i].counter_type,
          _tm_cache_counters[i].max_reg_bits,
          rc);
      _tm_reset_cache_counters(tm_ctx);
      return rc;
    }
  }

  rc = bf_sys_timer_create(&tm_ctx->cache_counters.ctr_timer,
                           BF_TM_STATS_REFRESH_COUNTERS_INTERVAL_SECS,
                           BF_TM_STATS_REFRESH_COUNTERS_INTERVAL_SECS,
                           tm_counter_refresh_timer_cb,
                           (void *)(uintptr_t)tm_ctx->devid);
  if (rc != BF_SYS_TIMER_OK) {
    LOG_ERROR("TM: %s:%d counter timer creation failed dev %d status 0x%x",
              __func__,
              __LINE__,
              tm_ctx->devid,
              rc);
    _tm_reset_cache_counters(tm_ctx);
    return rc;
  }
  tm_ctx->cache_counters.timer_state = TM_TIMER_INITIALIZED;

  return rc;
}

bf_tm_status_t tm_uninit_cached_counters(bf_tm_dev_ctx_t *tm_ctx) {
  if (!tm_ctx) {
    LOG_ERROR("TM: %s:%d Invalid tm_ctx passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  for (int i = 0; i < TM_MAX_COUNTER_MODULE_TYPES; i++) {
    TM_LOCK(tm_ctx->devid, tm_ctx->lock);

    tm_cache_counters_mod_ctx_t *ctr_mod_ctx =
        &tm_ctx->cache_counters.cache_counters_info[i];

    // Invalidate all the Counters that were added
    ctr_mod_ctx->num_valid_ctrs_ids = 0;

    TRAFFIC_MGR_MEMSET(
        ctr_mod_ctx->ctr_id_list,
        0,
        sizeof(tm_cache_counter_t) * TM_MAX_COUNTER_IDS_PER_MODULE);

    if (ctr_mod_ctx->valid_ctr_node_list != NULL) {
      tm_cache_counter_node_list_t *iter_list =
          ctr_mod_ctx->valid_ctr_node_list;
      tm_cache_counter_node_list_t *iter_next = NULL;

      // Iterate through all the nodes and update counters
      while (iter_list != NULL) {
        iter_next = iter_list->next;

        BF_LIST_DLL_REM(
            ctr_mod_ctx->valid_ctr_node_list, iter_list, next, prev);
        ctr_mod_ctx->num_valid_ctr_nodes--;

        TRAFFIC_MGR_FREE(iter_list);
        iter_list = iter_next;
      }
      ctr_mod_ctx->valid_ctr_node_list = NULL;
    }
    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
  }

  bf_sys_timer_del(&tm_ctx->cache_counters.ctr_timer);
  tm_ctx->cache_counters.timer_state = TM_TIMER_UNINITIALIZED;
  return BF_SUCCESS;
}

bf_tm_status_t tm_start_cached_counters_timer(bf_tm_dev_ctx_t *tm_ctx) {
  if (!tm_ctx) {
    LOG_ERROR("TM: %s:%d Invalid tm_ctx passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  TM_LOCK(tm_ctx->devid, tm_ctx->lock);

  LOG_DBG(
      "TM: %s:%d counter TIMER_START "
      "running dev %d timer state %d",
      __func__,
      __LINE__,
      tm_ctx->devid,
      tm_ctx->cache_counters.timer_state);

  if (tm_ctx->cache_counters.timer_state == TM_TIMER_RUNNING ||
      tm_ctx->cache_counters.timer_state == TM_TIMER_TRANS_TO_RUN) {
    // Already running, nothing to do
    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
    return BF_SUCCESS;
  }

  if (tm_ctx->cache_counters.timer_state != TM_TIMER_INITIALIZED) {
    LOG_ERROR(
        "TM: %s:%d counter timer not initialized or in process of "
        "running dev %d timer state %d",
        __func__,
        __LINE__,
        tm_ctx->devid,
        tm_ctx->cache_counters.timer_state);

    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
    return BF_INIT_ERROR;
  }
  tm_ctx->cache_counters.timer_state = TM_TIMER_TRANS_TO_RUN;
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  bf_sys_timer_status_t tsts = BF_SYS_TIMER_OK;

  tsts = bf_sys_timer_start(&tm_ctx->cache_counters.ctr_timer);
  if (tsts != BF_SYS_TIMER_OK) {
    LOG_ERROR("TM: %s:%d counter timer start failed dev %d status 0x%x",
              __func__,
              __LINE__,
              tm_ctx->devid,
              tsts);

    bf_sys_timer_del(&tm_ctx->cache_counters.ctr_timer);
    tm_ctx->cache_counters.timer_state = TM_TIMER_UNINITIALIZED;
    return tsts;
  }
  tm_ctx->cache_counters.timer_state = TM_TIMER_RUNNING;
  return tsts;
}

bf_tm_status_t tm_stop_cached_counters_timer(bf_tm_dev_ctx_t *tm_ctx) {
  if (!tm_ctx) {
    LOG_ERROR("TM: %s:%d Invalid tm_ctx passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  TM_LOCK(tm_ctx->devid, tm_ctx->lock);

  LOG_DBG(
      "TM: %s:%d counter TIMER_STOP "
      "running dev %d timer state %d",
      __func__,
      __LINE__,
      tm_ctx->devid,
      tm_ctx->cache_counters.timer_state);

  if ((tm_ctx->cache_counters.timer_state == TM_TIMER_UNINITIALIZED) ||
      (tm_ctx->cache_counters.timer_state == TM_TIMER_INITIALIZED)) {
    // Not initialized, or not started yet nothing to do
    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
    return BF_SUCCESS;
  }

  if (tm_ctx->cache_counters.timer_state != TM_TIMER_RUNNING) {
    LOG_ERROR(
        "TM: %s:%d counter timer not running or stopping dev %d "
        "timer state %d",
        __func__,
        __LINE__,
        tm_ctx->devid,
        tm_ctx->cache_counters.timer_state);

    TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);
    return BF_IN_USE;
  }
  tm_ctx->cache_counters.timer_state = TM_TIMER_TRANS_TO_STOP;
  TM_UNLOCK(tm_ctx->devid, tm_ctx->lock);

  bf_sys_timer_status_t tsts = BF_SYS_TIMER_OK;

  tsts = bf_sys_timer_stop(&tm_ctx->cache_counters.ctr_timer);
  if (tsts != BF_SYS_TIMER_OK) {
    LOG_ERROR("TM: %s:%d counter timer stop failed dev %d status 0x%x",
              __func__,
              __LINE__,
              tm_ctx->devid,
              tsts);
    return tsts;
  }

  tm_ctx->cache_counters.timer_state = TM_TIMER_INITIALIZED;
  return tsts;
}

bf_status_t bf_tm_stop_cache_counters_timer(bf_dev_id_t dev) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  bf_status_t rc = BF_SUCCESS;
  rc = tm_stop_cached_counters_timer(g_tm_ctx[dev]);
  return rc;
}

bf_status_t bf_tm_start_cache_counters_timer(bf_dev_id_t dev) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  bf_status_t rc = BF_SUCCESS;
  rc = tm_start_cached_counters_timer(g_tm_ctx[dev]);
  return rc;
}
