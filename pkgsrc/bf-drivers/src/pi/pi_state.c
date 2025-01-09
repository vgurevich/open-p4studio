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


#include <target-sys/bf_sal/bf_sys_intf.h>
#include <target-utils/third-party/cJSON/cJSON.h>
#include <target-utils/map/map.h>
#include <stddef.h>  // needed by target-utils/third-party/cJSON/cJSON.h

#include <PI/pi.h>

#include "pi_allocators.h"
#include "pi_log.h"
#include "pi_state.h"
#include "ctx_json/ctx_json_utils.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  bf_map_t map;
  bf_map_t rev_map;
} handles_map_t;

static void handles_map_init(handles_map_t *map) {
  map->map = (bf_map_t)NULL;
  map->rev_map = (bf_map_t)NULL;
}

typedef unsigned long map_handle_t;

static void handles_map_add(handles_map_t *map,
                            map_handle_t from,
                            map_handle_t to) {
  bf_map_sts_t msts1 = bf_map_add(&map->map, from, (void *)to);
  bf_map_sts_t msts2 = bf_map_add(&map->rev_map, to, (void *)from);
  if (msts1 != BF_MAP_OK || msts2 != BF_MAP_OK)
    LOG_ERROR("%s: error when inserting into handles map", __func__);
}

static map_handle_t handles_map_lookup(handles_map_t *map, map_handle_t from) {
  void *to;
  bf_map_sts_t msts = bf_map_get(&map->map, from, &to);
  if (msts == BF_MAP_OK) {
    return (map_handle_t)to;
  } else {
    LOG_ERROR("%s: error when retrieving handle", __func__);
    return 0;
  }
}

static map_handle_t handles_map_rev_lookup(handles_map_t *map,
                                           map_handle_t from) {
  void *to;
  bf_map_sts_t msts = bf_map_get(&map->rev_map, from, &to);
  if (msts == BF_MAP_OK) {
    return (map_handle_t)to;
  } else {
    LOG_ERROR("%s: error when retrieving handle", __func__);
    return 0;
  }
}

static void handles_map_destroy(handles_map_t *map) {
  bf_map_destroy(&map->map);
  bf_map_destroy(&map->rev_map);
}

static char *read_file(const char *path) {
  char *source = NULL;
  FILE *fp = fopen(path, "r");
  if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
      /* Get the size of the file. */
      long bufsize = ftell(fp);
      if (bufsize == -1) { /* Error */
      }

      /* Allocate our buffer to that size. */
      source = bf_sys_malloc(sizeof(char) * (bufsize + 1));

      /* Go back to the start of the file. */
      if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
      }

      /* Read the entire file into memory. */
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      if (newLen == 0) {
        fputs("Error reading file", stderr);
      } else {
        source[newLen++] = '\0'; /* Just to be safe. */
      }
    }
    fclose(fp);
  }
  return source;
}

typedef struct {
  pipe_tbl_hdl_t handle;
  handles_map_t action_handles;
  // for action data tables (ADT), action_handles is not used, but
  // selector_handle may be
  uint32_t selector_handle;
  size_t num_indirect_res;
  pi_state_table_indirect_res_t indirect_res[PIPE_NUM_TBL_RESOURCES];
  // only depends on the action, so does not have to be replicated per table, I
  // am doing it for the sake of convenience
  // we map PI action ids to a heap allocated array with PIPE_NUM_TBL_RESOURCES
  // pi_state_indirect_res_access_info_t entries.
  bf_map_t action_indirect_res_access;
  // NULL if the table is not a match table, or does not support idle time.
  pi_state_idle_time_scratchspace_t *idle_time_scratch;
} table_state_t;

typedef struct {
  // The PI client should guarantee that no write or read operation is initiated
  // during warm_init calls, so the lock is not required then. However, without
  // the lock, we could have a race condition between callbacks (digest, idle
  // timeout) and device remove. More precisely, because the pipe_mgr "device
  // remove" executes after the PI "device remove", we could get a callback
  // after we have cleaned up the device state. To avoid this, every call back
  // (digest, idle timeout) must execute with the shared lock and return if the
  // device is not assigned (i.e. was removed already). "device add" and
  // "device remove" must get exclusive access to rwlock and toggle the "device
  // assigned" flag appropriately.
  // Note that to reduce the probability of getting a callback after the "device
  // remove", we could deregister our callbacks as we clean out state. However,
  // this would not simplify the code: we would still need to get the lock in
  // every callback and check the device status.
  bf_sys_rwlock_t rwlock;
  bool assigned;
  bf_map_t tables;
  bf_map_t act_prof_curr;
  // for counters and meters, it seems that all we need is a map from the PI id
  // to the pipe_mgr handle. We use the same map for all types of resources.
  handles_map_t resources;
  bf_map_t digests;
  bf_sys_mutex_t lrn_timeout_mutex;
  uint32_t lrn_timeout_us;
} device_state_t;

typedef struct {
  bf_map_t mbr_info;
  bf_map_t grp_info;
} ms_state_t;

// note that when keeping track of indirect resources, we assume that all the
// resources are set when the group is created, i.e. before the member is added
// to any group
typedef struct {
  pi_p4_id_t action_id;
  size_t num_indirect_res;
  pi_state_indirect_res_t indirect_res[PIPE_NUM_TBL_RESOURCES];
  bf_map_t grps;  // used as a bitmap
} mbr_info_t;

typedef struct grp_info_s {
  pi_p4_id_t action_id;
  size_t num_indirect_res;
  pi_state_indirect_res_t indirect_res[PIPE_NUM_TBL_RESOURCES];
  bf_map_t mbrs;  // used as a bitmap
} grp_info_t;

static device_state_t *_state;
static size_t _num_devices;
// this pipe_mgr session will be used for all devices for all learning
// operations: register learning callbacks and ack learning messages
static pipe_sess_hdl_t _lrn_session;

static table_state_t *get_table_state(pi_dev_id_t dev_id, pi_p4_id_t t_id) {
  device_state_t *dev_state = &_state[dev_id];
  void *t_state;
  bf_map_sts_t msts = bf_map_get(&dev_state->tables, t_id, &t_state);
  if (msts == BF_MAP_OK) {
    return (table_state_t *)t_state;
  } else {
    LOG_ERROR("%s: error when retrieving table state %u for device %" PRIu64,
              __func__,
              t_id,
              dev_id);
    return NULL;
  }
}

static void init_idle_time_scratchspace(
    const pi_p4info_t *p4info,
    pi_p4_id_t table_id,
    pipe_mat_tbl_hdl_t handle,
    pi_state_idle_time_scratchspace_t *idle_time_scratch) {
  idle_time_scratch->p4info = p4info;
  idle_time_scratch->table_id = table_id;
  idle_time_scratch->table_handle = handle;
  allocate_pipe_match_spec(
      table_id, p4info, &idle_time_scratch->pipe_match_spec);
  // we only need to allocate memory for the action data for direct tables
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    allocate_pipe_action_data_spec_any(
        table_id, p4info, &idle_time_scratch->pipe_action_spec.act_data);
  } else {
    idle_time_scratch->pipe_action_spec.act_data.action_data_bits = NULL;
  }
  allocate_pi_match_key(table_id, p4info, &idle_time_scratch->match_key);
}

static void destroy_idle_time_scratchspace(
    pi_state_idle_time_scratchspace_t *idle_time_scratch) {
  release_pipe_match_spec(&idle_time_scratch->pipe_match_spec);
  bf_sys_free(idle_time_scratch->pipe_action_spec.act_data.action_data_bits);
  release_pi_match_key(&idle_time_scratch->match_key);
}

// retrieve the PI id of a resource without knowing its type
static pi_p4_id_t get_res_id(const pi_p4info_t *p4info, const char *res_name) {
  pi_p4_id_t res_id;
  res_id = pi_p4info_counter_id_from_name(p4info, res_name);
  if (res_id == PI_INVALID_ID)
    res_id = pi_p4info_meter_id_from_name(p4info, res_name);
  return res_id;
}

static void retrieve_indirect_resources(cJSON *action_cjson,
                                        const pi_p4info_t *p4info,
                                        pi_p4_id_t a_id,
                                        table_state_t *t_state) {
  int err = 0;
  pi_state_indirect_res_access_info_t *access_info = bf_sys_calloc(
      PIPE_NUM_TBL_RESOURCES, sizeof(pi_state_indirect_res_access_info_t));
  bf_map_add(&t_state->action_indirect_res_access, a_id, access_info);

  cJSON *indirect_resources_cjson = NULL;
  err |= bf_cjson_get_object(action_cjson,
                             CTX_JSON_ACTION_INDIRECT_RESOURCES,
                             &indirect_resources_cjson);

  cJSON *indirect_resource_cjson = NULL;
  CTX_JSON_FOR_EACH(indirect_resource_cjson, indirect_resources_cjson) {
    char *access_mode = NULL;
    err |= bf_cjson_get_string(indirect_resource_cjson,
                               CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE,
                               &access_mode);
    char *res_name = NULL;
    err |= bf_cjson_get_string(indirect_resource_cjson,
                               CTX_JSON_INDIRECT_RESOURCE_RESOURCE_NAME,
                               &res_name);
    pi_p4_id_t res_id = get_res_id(p4info, res_name);

    if (res_id == PI_INVALID_ID) continue;
    bf_sys_assert(res_id != PI_INVALID_ID);

    // find "index" of resource
    size_t res_idx = 0;
    for (; res_idx < t_state->num_indirect_res; res_idx++) {
      if (t_state->indirect_res[res_idx].res_id == res_id) break;
    }
    if (res_idx == t_state->num_indirect_res) t_state->num_indirect_res++;

    t_state->indirect_res[res_idx].res_id = res_id;
    t_state->indirect_res[res_idx].is_match_bound = false;

    access_info[res_idx].action_id = a_id;
    access_info[res_idx].res_id = res_id;
    if (!strncmp(access_mode, "constant", sizeof "constant")) {
      access_info[res_idx].access_mode =
          PI_STATE_INDIRECT_RES_ACCESS_MODE_IMMEDIATE;
      int value = 0;
      err |= bf_cjson_get_int(
          indirect_resource_cjson, CTX_JSON_INDIRECT_RESOURCE_VALUE, &value);
      access_info[res_idx].index.immediate = value;
    } else if (!strncmp(access_mode, "index", sizeof "index")) {
      access_info[res_idx].access_mode = PI_STATE_INDIRECT_RES_ACCESS_MODE_ARG;
      char *param_name = NULL;
      err |= bf_cjson_get_string(indirect_resource_cjson,
                                 CTX_JSON_INDIRECT_RESOURCE_PARAMETER_NAME,
                                 &param_name);
      pi_p4_id_t param_id =
          pi_p4info_action_param_id_from_name(p4info, a_id, param_name);
      size_t bitwidth = pi_p4info_action_param_bitwidth(p4info, a_id, param_id);
      size_t offset = pi_p4info_action_param_offset(p4info, a_id, param_id);
      access_info[res_idx].index.action_data_pos.width = (bitwidth + 7) / 8;
      access_info[res_idx].index.action_data_pos.offset = offset;
    }
  }
}

// returns 0 if success, 1 otherwise
static int add_tables_from_json(pi_dev_id_t dev_id,
                                const pi_p4info_t *p4info,
                                const cJSON *tables_cjson) {
  int err = 0;
  device_state_t *dev_state = &_state[dev_id];

  // no need to initialize BF maps for device (to NULL); we initalize them in
  // pi_state_init and destroying them in pi_state_remove_device resets them to
  // NULL.

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);

    // skip tables we are not interested in
    if (strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      continue;
    }

    char *name = NULL;
    int handle = 0;
    err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
    err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_HANDLE, &handle);

    table_state_t *t_state = NULL;
    pi_p4_id_t t_id = pi_p4info_table_id_from_name(p4info, name);
    // tables missing from the p4info (because they are synthesized by the
    // compiler) cannot just be "skipped", because we need to inspect the
    // resources and update our maps (e.g. for hash driven stats)
    if (t_id != PI_INVALID_ID) {
      t_state = bf_sys_malloc(sizeof(table_state_t));
      bf_map_add(&dev_state->tables, t_id, t_state);

      memset(t_state, 0, sizeof(*t_state));
      t_state->handle = handle;
      handles_map_init(&t_state->action_handles);

      // do not call pi_p4info_table_supports_idle_timeout if the table doesn't
      // exist in p4info
      bool supports_idle_time =
          pi_p4info_table_supports_idle_timeout(p4info, t_id);
      if (supports_idle_time) {
        t_state->idle_time_scratch =
            bf_sys_malloc(sizeof(*t_state->idle_time_scratch));
        init_idle_time_scratchspace(
            p4info, t_id, handle, t_state->idle_time_scratch);
      }
    }

    cJSON *adts_cjson = NULL;
    err |= bf_cjson_get_object(
        table_cjson, CTX_JSON_MATCH_TABLE_ACTION_DATA_TABLE_REFS, &adts_cjson);

    table_state_t *act_prof_state = NULL;
    int num_adt = cJSON_GetArraySize(adts_cjson);
    if (num_adt > 0) {
      if (num_adt != 1) {
        LOG_ERROR("%s: in context JSON, table '%s' has more than one ADT",
                  __func__,
                  name);
      }

      cJSON *adt_cjson = adts_cjson->child;
      char *how_referenced = NULL;
      err |= bf_cjson_get_string(
          adt_cjson, CTX_JSON_TABLE_HOW_REFERENCED, &how_referenced);

      if (!strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_INDIRECT)) {
        int adt_handle = 0;
        err |= bf_cjson_get_int(
            adt_cjson, CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE, &adt_handle);

        pi_p4_id_t act_prof_id =
            pi_p4info_table_get_implementation(p4info, t_id);

        bf_map_sts_t msts = bf_map_get(
            &dev_state->tables, act_prof_id, (void **)&act_prof_state);
        if (msts == BF_MAP_NO_KEY) {
          act_prof_state = bf_sys_malloc(sizeof(table_state_t));
          memset(act_prof_state, 0, sizeof(*act_prof_state));
          bf_map_add(&dev_state->tables, act_prof_id, act_prof_state);

          act_prof_state->handle = adt_handle;

          ms_state_t *ms_state = bf_sys_malloc(sizeof(ms_state_t));
          ms_state->mbr_info = (bf_map_t)NULL;
          ms_state->grp_info = (bf_map_t)NULL;
          bf_map_add(&dev_state->act_prof_curr, act_prof_id, ms_state);
        } else {
          bf_sys_assert(msts == BF_MAP_OK);
        }
      }
    }

    cJSON *selection_tables_cjson = NULL;
    err |= bf_cjson_get_object(table_cjson,
                               CTX_JSON_MATCH_TABLE_SELECTION_TABLE_REFS,
                               &selection_tables_cjson);
    int num_sel = cJSON_GetArraySize(selection_tables_cjson);
    if (num_sel > 0) {
      if (!act_prof_state) {
        LOG_ERROR("%s: in context JSON, table '%s' has selector but no ADT",
                  __func__,
                  name);
      }
      if (num_sel != 1) {
        LOG_ERROR("%s: in context JSON, table '%s' has more than one ADT",
                  __func__,
                  name);
      }

      cJSON *selection_table_cjson = selection_tables_cjson->child;
      int selector_handle = 0;
      err |= bf_cjson_get_int(selection_table_cjson,
                              CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE,
                              &selector_handle);
      act_prof_state->selector_handle = selector_handle;
    }

    if (t_id != PI_INVALID_ID) {
      cJSON *actions_cjson = NULL;
      err |= bf_cjson_get_object(
          table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);

      cJSON *action_cjson = NULL;
      CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
        char *action_name = NULL;
        err |= bf_cjson_get_string(
            action_cjson, CTX_JSON_ACTION_NAME, &action_name);

        int action_handle = 0;
        err |= bf_cjson_get_int(
            action_cjson, CTX_JSON_ACTION_HANDLE, &action_handle);

        pi_p4_id_t a_id = pi_p4info_action_id_from_name(p4info, action_name);

        handles_map_add(&t_state->action_handles, a_id, action_handle);

        retrieve_indirect_resources(action_cjson, p4info, a_id, t_state);
      }
    }

    cJSON *statistics_tables_cjson = NULL;
    err |= bf_cjson_get_object(table_cjson,
                               CTX_JSON_MATCH_TABLE_STATISTICS_TABLE_REFS,
                               &statistics_tables_cjson);

    cJSON *statistics_table_cjson = NULL;
    CTX_JSON_FOR_EACH(statistics_table_cjson, statistics_tables_cjson) {
      char *stat_name = NULL;
      int stat_handle = 0;
      err |= bf_cjson_get_string(
          statistics_table_cjson, CTX_JSON_TABLE_NAME, &stat_name);
      err |= bf_cjson_get_int(
          statistics_table_cjson, CTX_JSON_TABLE_HANDLE, &stat_handle);

      pi_p4_id_t counter_id = pi_p4info_counter_id_from_name(p4info, stat_name);
      handles_map_add(&dev_state->resources, counter_id, stat_handle);
    }

    cJSON *meter_tables_cjson = NULL;
    err |= bf_cjson_get_object(table_cjson,
                               CTX_JSON_MATCH_TABLE_METER_TABLE_REFS,
                               &meter_tables_cjson);

    cJSON *meter_table_cjson = NULL;
    CTX_JSON_FOR_EACH(meter_table_cjson, meter_tables_cjson) {
      char *meter_name = NULL;
      int meter_handle = 0;
      err |= bf_cjson_get_string(
          meter_table_cjson, CTX_JSON_TABLE_NAME, &meter_name);
      err |= bf_cjson_get_int(
          meter_table_cjson, CTX_JSON_TABLE_HANDLE, &meter_handle);

      pi_p4_id_t meter_id = pi_p4info_meter_id_from_name(p4info, meter_name);
      handles_map_add(&dev_state->resources, meter_id, meter_handle);
    }
  }

  dev_state->assigned = true;

  return err;
}

static int add_digests_from_json(pi_dev_id_t dev_id,
                                 const pi_p4info_t *p4info,
                                 const cJSON *digests_cjson) {
  int err = 0;
  device_state_t *dev_state = &_state[dev_id];

  cJSON *digest_cjson = NULL;
  CTX_JSON_FOR_EACH(digest_cjson, digests_cjson) {
    char *name = NULL;
    int handle = 0;
    err |=
        bf_cjson_get_string(digest_cjson, CTX_JSON_LEARN_QUANTUM_NAME, &name);
    err |=
        bf_cjson_get_int(digest_cjson, CTX_JSON_LEARN_QUANTUM_HANDLE, &handle);

    pi_p4_id_t digest_id = pi_p4info_digest_id_from_name(p4info, name);
    if (digest_id == PI_INVALID_ID) {
      LOG_ERROR("%s: cannot find digest '%s' in p4info", __func__, name);
      continue;
    }

    pi_state_digest_state_t *digest_state =
        bf_sys_malloc(sizeof(pi_state_digest_state_t));
    bf_map_add(&dev_state->digests, digest_id, digest_state);

    memset(digest_state, 0, sizeof(*digest_state));
    digest_state->digest_id = digest_id;
    digest_state->digest_handle = handle;
    digest_state->entry_size = pi_p4info_digest_data_size(p4info, digest_id);
  }

  dev_state->lrn_timeout_us = 0xffffffff;

  return err;
}

// returns 0 if success, 1 otherwise
int pi_state_assign_device(pi_dev_id_t dev_id,
                           const pi_p4info_t *p4info,
                           const char *context_json_path) {
  char *json_data = read_file(context_json_path);
  if (!json_data) {
    LOG_CRIT(
        "%s: context JSON path '%s' is not valid", __func__, context_json_path);
    return 1;
  }

  cJSON *root = cJSON_Parse(json_data);

  int status = 0;
  cJSON *tables_cjson = NULL, *digests_cjson = NULL;
  status |= bf_cjson_get_object(root, CTX_JSON_TABLES_NODE, &tables_cjson);
  status |=
      bf_cjson_get_object(root, CTX_JSON_LEARN_QUANTA_NODE, &digests_cjson);

  LOG_TRACE(
      "%s: Loading table information from context JSON for device %" PRIu64,
      __func__,
      dev_id);
  status |= add_tables_from_json(dev_id, p4info, tables_cjson);

  LOG_TRACE(
      "%s: Loading digest information from context JSON for device %" PRIu64,
      __func__,
      dev_id);
  status |= add_digests_from_json(dev_id, p4info, digests_cjson);

  cJSON_Delete(root);
  bf_sys_free(json_data);

  return status;
}

typedef void (*MapApplyFn)(unsigned long key, void *data, void *aux);

void map_apply_all(bf_map_t *map, MapApplyFn fn, void *aux) {
  unsigned long key = 0;
  void *data = NULL;
  bf_map_sts_t msts = bf_map_get_first(map, &key, &data);
  while (msts == BF_MAP_OK) {
    fn(key, data, aux);
    msts = bf_map_get_next(map, &key, &data);
  }
}

static void map_generic_free(unsigned long key, void *data, void *aux) {
  (void)aux;
  (void)key;
  bf_sys_free(data);
}

static void destroy_mbr_info(unsigned long mbr_h, void *mbr_info, void *aux) {
  (void)aux;
  (void)mbr_h;
  mbr_info_t *mbr_info_ = (mbr_info_t *)mbr_info;
  bf_map_destroy(&mbr_info_->grps);
  bf_sys_free(mbr_info_);
}

static void destroy_grp_info(unsigned long grp_h, void *grp_info, void *aux) {
  (void)aux;
  (void)grp_h;
  grp_info_t *grp_info_ = (grp_info_t *)grp_info;
  bf_map_destroy(&grp_info_->mbrs);
  bf_sys_free(grp_info_);
}

static void destroy_ms_state(unsigned long id, void *ms_state, void *aux) {
  (void)aux;
  (void)id;
  ms_state_t *ms_state_ = (ms_state_t *)ms_state;
  map_apply_all(&ms_state_->mbr_info, destroy_mbr_info, NULL);
  bf_map_destroy(&ms_state_->mbr_info);
  map_apply_all(&ms_state_->grp_info, destroy_grp_info, NULL);
  bf_map_destroy(&ms_state_->grp_info);
  bf_sys_free(ms_state_);
}

static void destroy_t_state(unsigned long id, void *t_state, void *aux) {
  (void)aux;
  (void)id;
  table_state_t *t_state_ = (table_state_t *)t_state;
  handles_map_destroy(&t_state_->action_handles);
  map_apply_all(&t_state_->action_indirect_res_access, map_generic_free, NULL);
  bf_map_destroy(&t_state_->action_indirect_res_access);
  if (t_state_->idle_time_scratch) {
    destroy_idle_time_scratchspace(t_state_->idle_time_scratch);
    bf_sys_free(t_state_->idle_time_scratch);
  }
  bf_sys_free(t_state_);
}

void pi_state_remove_device(pi_dev_id_t dev_id) {
  device_state_t *dev_state = &_state[dev_id];

  // free table state
  map_apply_all(&dev_state->tables, destroy_t_state, NULL);
  bf_map_destroy(&dev_state->tables);

  // free digest state
  map_apply_all(&dev_state->digests, map_generic_free, NULL);
  bf_map_destroy(&dev_state->digests);

  // free act prof live state
  map_apply_all(&dev_state->act_prof_curr, destroy_ms_state, NULL);
  bf_map_destroy(&dev_state->act_prof_curr);

  handles_map_destroy(&dev_state->resources);

  dev_state->assigned = false;
}

bool pi_state_is_device_assigned(pi_dev_id_t dev_id) {
  device_state_t *dev_state = &_state[dev_id];
  return dev_state->assigned;
}

void pi_state_init(size_t num_devices) {
  _num_devices = num_devices;
  _state = bf_sys_calloc(num_devices, sizeof(*_state));
  for (size_t i = 0; i < num_devices; i++) {
    _state[i].tables = (bf_map_t)NULL;
    _state[i].act_prof_curr = (bf_map_t)NULL;
    handles_map_init(&_state[i].resources);
    _state[i].assigned = false;
    _state[i].digests = (bf_map_t)NULL;
    bf_sys_rwlock_init(&_state[i].rwlock, NULL);
    bf_sys_mutex_init(&_state[i].lrn_timeout_mutex);
  }
  LOG_TRACE("%s: allocating pipe_mgr session for learning", __func__);
  pipe_status_t status = pipe_mgr_client_init(&_lrn_session);
  if (status != PIPE_SUCCESS)
    LOG_CRIT("%s: failed to allocate pipe_mgr session for learning", __func__);
}

void pi_state_destroy() {
  for (size_t i = 0; i < _num_devices; i++) {
    if (_state[i].assigned) pi_state_remove_device(i);
    bf_sys_rwlock_del(&_state[i].rwlock);
    bf_sys_mutex_del(&_state[i].lrn_timeout_mutex);
  }
  bf_sys_free(_state);
  LOG_TRACE("%s: releasing pipe_mgr session for learning", __func__);
  pipe_status_t status = pipe_mgr_client_cleanup(_lrn_session);
  if (status != PIPE_SUCCESS)
    LOG_ERROR("%s: failed to release pipe_mgr session for learning", __func__);
}

uint32_t pi_state_table_id_to_handle(pi_dev_id_t dev_id, pi_p4_id_t id) {
  table_state_t *t_state = get_table_state(dev_id, id);
  return t_state->handle;
}

uint32_t pi_state_action_id_to_handle(pi_dev_id_t dev_id,
                                      pi_p4_id_t t_id,
                                      pi_p4_id_t a_id) {
  table_state_t *t_state = get_table_state(dev_id, t_id);
  return handles_map_lookup(&t_state->action_handles, a_id);
}

pi_p4_id_t pi_state_action_handle_to_id(pi_dev_id_t dev_id,
                                        pi_p4_id_t t_id,
                                        uint32_t a_handle) {
  table_state_t *t_state = get_table_state(dev_id, t_id);
  return handles_map_rev_lookup(&t_state->action_handles, a_handle);
}

uint32_t pi_state_act_prof_get_selector_handle(pi_dev_id_t dev_id,
                                               pi_p4_id_t act_prof_id) {
  table_state_t *t_state = get_table_state(dev_id, act_prof_id);
  return t_state->selector_handle;
}

uint32_t pi_state_res_id_to_handle(pi_dev_id_t dev_id, pi_p4_id_t id) {
  device_state_t *dev_state = &_state[dev_id];
  return handles_map_lookup(&dev_state->resources, id);
}

pi_p4_id_t pi_state_res_handle_to_id(pi_dev_id_t dev_id, uint32_t handle) {
  device_state_t *dev_state = &_state[dev_id];
  return handles_map_rev_lookup(&dev_state->resources, handle);
}

const pi_state_table_indirect_res_t *pi_state_table_indirect_res(
    pi_dev_id_t dev_id, pi_p4_id_t t_id, size_t *num_indirect_res) {
  table_state_t *t_state = get_table_state(dev_id, t_id);
  *num_indirect_res = t_state->num_indirect_res;
  return t_state->indirect_res;
}

const pi_state_indirect_res_access_info_t *pi_state_indirect_res_access_info(
    pi_dev_id_t dev_id, pi_p4_id_t t_id, pi_p4_id_t a_id, pi_p4_id_t res_id) {
  table_state_t *t_state = get_table_state(dev_id, t_id);
  pi_state_indirect_res_access_info_t *access_info;
  bf_map_sts_t msts = bf_map_get(
      &t_state->action_indirect_res_access, a_id, (void **)&access_info);
  bf_sys_assert(msts == BF_MAP_OK);
  for (size_t i = 0; i < PIPE_NUM_TBL_RESOURCES; i++) {
    if (access_info[i].res_id == res_id) return &access_info[i];
  }
  return NULL;
}

static ms_state_t *get_ms_state(pi_dev_id_t dev_id, pi_p4_id_t act_prof_id) {
  device_state_t *dev_state = &_state[dev_id];
  void *ms_state;
  bf_map_sts_t msts =
      bf_map_get(&dev_state->act_prof_curr, act_prof_id, &ms_state);
  if (msts == BF_MAP_OK) {
    return (ms_state_t *)ms_state;
  } else {
    LOG_ERROR(
        "%s: error when retrieving match-select state %u for device %" PRIu64,
        __func__,
        act_prof_id,
        dev_id);
    return NULL;
  }
}

static mbr_info_t *get_mbr_info(pi_dev_id_t dev_id,
                                pi_p4_id_t act_prof_id,
                                pi_indirect_handle_t mbr_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);

  mbr_info_t *mbr_info;
  bf_map_sts_t msts =
      bf_map_get(&ms_state->mbr_info, mbr_h, (void **)&mbr_info);
  bf_sys_assert(msts == BF_MAP_OK);

  return mbr_info;
}

static grp_info_t *get_grp_info(pi_dev_id_t dev_id,
                                pi_p4_id_t act_prof_id,
                                pi_indirect_handle_t grp_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);

  grp_info_t *grp_info;
  bf_map_sts_t msts =
      bf_map_get(&ms_state->grp_info, grp_h, (void **)&grp_info);
  bf_sys_assert(msts == BF_MAP_OK);

  return grp_info;
}

static bool same_indirect_res(const pi_state_indirect_res_t *res1,
                              const pi_state_indirect_res_t *res2) {
  return (res1->res_id == res2->res_id) && (res1->res_idx == res2->res_idx);
}

// in the Tofino case, we know that the pi_indirect_handle_t can safely be cast
// down to 32-bit because the upper 32-bit are unused. Therefore it is safe to
// cast them to "unsigned long", even on systems where sizeof(unsigned long) = 4

int pi_state_ms_grp_add_mbr(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h,
                            pi_indirect_handle_t mbr_h) {
  grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);

  // handle indirect reosources
  if (bf_map_count(&grp_info->mbrs) == 0) {  // first member in group
    grp_info->num_indirect_res = mbr_info->num_indirect_res;
    memcpy(grp_info->indirect_res,
           mbr_info->indirect_res,
           sizeof(grp_info->indirect_res));
  } else {  // check resource consistencies
    if (mbr_info->num_indirect_res != grp_info->num_indirect_res) {
      LOG_ERROR("%s: indirect resource inconsistency between member and group",
                __func__);
      return 1;
    }
    for (size_t i = 0; i < grp_info->num_indirect_res; i++) {
      if (!same_indirect_res(&mbr_info->indirect_res[i],
                             &grp_info->indirect_res[i])) {
        LOG_ERROR(
            "%s: indirect resource inconsistency between member and group",
            __func__);
        return 1;
      }
    }
  }

  bf_map_add(&grp_info->mbrs, mbr_h, NULL);
  bf_map_add(&mbr_info->grps, grp_h, NULL);
  return 0;
}

void pi_state_ms_grp_remove_mbr(pi_dev_id_t dev_id,
                                pi_p4_id_t act_prof_id,
                                pi_indirect_handle_t grp_h,
                                pi_indirect_handle_t mbr_h) {
  grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
  bf_map_rmv(&grp_info->mbrs, mbr_h);
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
  bf_map_rmv(&mbr_info->grps, grp_h);
}

void pi_state_ms_mbr_apply_to_grps(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_h,
                                   PIMSGrpFn grp_fn,
                                   void *aux) {
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);

  void *dummy = NULL;
  unsigned long grp_h = 0;
  bf_map_sts_t msts = bf_map_get_first(&mbr_info->grps, &grp_h, &dummy);
  while (msts == BF_MAP_OK) {
    grp_fn(dev_id, act_prof_id, mbr_h, grp_h, aux);
    msts = bf_map_get_next(&mbr_info->grps, &grp_h, &dummy);
  }
}

void pi_state_ms_mbr_create(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t mbr_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);
  mbr_info_t *mbr_info = bf_sys_malloc(sizeof(mbr_info_t));
  memset(mbr_info, 0, sizeof(*mbr_info));
  bf_map_add(&ms_state->mbr_info, mbr_h, mbr_info);
}

void pi_state_ms_mbr_delete(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t mbr_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);

  bf_map_sts_t msts;
  mbr_info_t *mbr_info;
  msts = bf_map_get_rmv(&ms_state->mbr_info, mbr_h, (void **)&mbr_info);
  bf_sys_assert(msts == BF_MAP_OK);

  // remove the mbr from all grps referencing it
  void *dummy = NULL;
  unsigned long grp_h = 0;
  msts = bf_map_get_first(&mbr_info->grps, &grp_h, &dummy);
  while (msts == BF_MAP_OK) {
    grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
    bf_sys_assert(grp_info);
    bf_map_rmv(&grp_info->mbrs, mbr_h);
    msts = bf_map_get_next(&mbr_info->grps, &grp_h, &dummy);
  }

  bf_map_destroy(&mbr_info->grps);

  bf_sys_free(mbr_info);
}

void pi_state_ms_mbr_set_act(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t mbr_h,
                             pi_p4_id_t action_id) {
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
  mbr_info->action_id = action_id;
}

void pi_state_ms_grp_set_act(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t grp_h,
                             pi_p4_id_t action_id) {
  grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
  grp_info->action_id = action_id;
}

pi_p4_id_t pi_state_ms_mbr_get_act(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_h) {
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
  return mbr_info->action_id;
}

pi_p4_id_t pi_state_ms_grp_get_act(pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t grp_h) {
  grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
  return grp_info->action_id;
}

void pi_state_ms_grp_create(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);
  grp_info_t *grp_info = bf_sys_malloc(sizeof(grp_info_t));
  memset(grp_info, 0, sizeof(*grp_info));
  bf_map_add(&ms_state->grp_info, grp_h, grp_info);
}

void pi_state_ms_grp_delete(pi_dev_id_t dev_id,
                            pi_p4_id_t act_prof_id,
                            pi_indirect_handle_t grp_h) {
  ms_state_t *ms_state = get_ms_state(dev_id, act_prof_id);

  bf_map_sts_t msts;
  grp_info_t *grp_info;
  msts = bf_map_get_rmv(&ms_state->grp_info, grp_h, (void **)&grp_info);
  bf_sys_assert(msts == BF_MAP_OK);

  // remove the grp from all mbrs referencing it
  void *dummy = NULL;
  unsigned long mbr_h = 0;
  msts = bf_map_get_first(&grp_info->mbrs, &mbr_h, &dummy);
  while (msts == BF_MAP_OK) {
    mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
    bf_sys_assert(mbr_info);
    bf_map_rmv(&mbr_info->grps, grp_h);
    msts = bf_map_get_next(&grp_info->mbrs, &mbr_h, &dummy);
  }

  bf_map_destroy(&grp_info->mbrs);

  bf_sys_free(grp_info);
}

void pi_state_ms_mbr_add_res(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t mbr_h,
                             pi_state_indirect_res_t *res) {
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
  mbr_info->indirect_res[mbr_info->num_indirect_res++] = *res;
}

const pi_state_indirect_res_t *pi_state_ms_mbr_get_res(
    pi_dev_id_t dev_id,
    pi_p4_id_t act_prof_id,
    pi_indirect_handle_t mbr_h,
    size_t *num_indirect_res) {
  mbr_info_t *mbr_info = get_mbr_info(dev_id, act_prof_id, mbr_h);
  *num_indirect_res = mbr_info->num_indirect_res;
  return mbr_info->indirect_res;
}

const pi_state_indirect_res_t *pi_state_ms_grp_get_res(
    pi_dev_id_t dev_id,
    pi_p4_id_t act_prof_id,
    pi_indirect_handle_t grp_h,
    size_t *num_indirect_res) {
  grp_info_t *grp_info = get_grp_info(dev_id, act_prof_id, grp_h);
  *num_indirect_res = grp_info->num_indirect_res;
  return grp_info->indirect_res;
}

pi_state_idle_time_scratchspace_t *pi_state_idle_time_get_scratchspace(
    pi_dev_id_t dev_id, pi_p4_id_t t_id) {
  table_state_t *t_state = get_table_state(dev_id, t_id);
  return t_state->idle_time_scratch;
}

void pi_state_shared_lock(pi_dev_id_t dev_id) {
  device_state_t *dev_state = &_state[dev_id];
  bf_sys_rwlock_rdlock(&dev_state->rwlock);
}

void pi_state_exclusive_lock(pi_dev_id_t dev_id) {
  device_state_t *dev_state = &_state[dev_id];
  bf_sys_rwlock_wrlock(&dev_state->rwlock);
}

void pi_state_unlock(pi_dev_id_t dev_id) {
  device_state_t *dev_state = &_state[dev_id];
  bf_sys_rwlock_unlock(&dev_state->rwlock);
}

pi_state_digest_state_t *pi_state_digest_get(pi_dev_id_t dev_id,
                                             pi_p4_id_t digest_id) {
  device_state_t *dev_state = &_state[dev_id];
  void *digest_state;
  bf_map_sts_t msts = bf_map_get(&dev_state->digests, digest_id, &digest_state);
  if (msts == BF_MAP_OK) {
    return (pi_state_digest_state_t *)digest_state;
  }
  LOG_ERROR("%s: error when retrieving digest state %u for device %" PRIu64,
            __func__,
            digest_id,
            dev_id);
  return NULL;
}

pipe_sess_hdl_t pi_state_digest_get_lrn_session() { return _lrn_session; }

uint32_t pi_state_digest_update_lrn_timeout(pi_dev_id_t dev_id,
                                            uint32_t lrn_timeout_us) {
  // The default pipe_mgr timeout seems somewhat too low (500us) so we increase
  // it to 50ms.
  static const uint32_t lrn_min_timeout_us = 50 * 1000;  // 50ms
  device_state_t *dev_state = &_state[dev_id];
  bf_sys_mutex_lock(&dev_state->lrn_timeout_mutex);
  if (lrn_timeout_us < dev_state->lrn_timeout_us)
    dev_state->lrn_timeout_us = lrn_timeout_us;
  else
    lrn_timeout_us = dev_state->lrn_timeout_us;
  bf_sys_mutex_unlock(&dev_state->lrn_timeout_mutex);
  return (lrn_timeout_us > 0) ? lrn_timeout_us : lrn_min_timeout_us;
}
