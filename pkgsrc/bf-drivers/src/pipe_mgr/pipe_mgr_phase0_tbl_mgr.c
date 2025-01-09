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


/*!
 * @file pipe_phase0_tbl_mgr.c
 * @date
 *
 * Phase0 table mgmt.
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_db.h"

#define PIPE_PHASE0_ENT_HDL_INVALID_HDL 0xdeadbeef

// Forward declarations
static pipe_status_t pipe_mgr_phase0_tbl_ha_init(
    pipe_mgr_phase0_tbl_t *phase0_tbl);
static void pipe_mgr_phase0_tbl_ha_deinit(pipe_mgr_phase0_tbl_t *phase0_tbl);

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_modify(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *action_spec,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_list);

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_del(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_list);

static pipe_mgr_phase0_tbl_data_t *phase0_tbl_get_instance_from_any_pipe(
    pipe_mgr_phase0_tbl_t *phase0_tbl, bf_dev_pipe_t pipe_id);

static pipe_mgr_phase0_tbl_data_t *phase0_tbl_get_instance(
    pipe_mgr_phase0_tbl_t *phase0_tbl, bf_dev_pipe_t pipe_id);

/** \brief pipe_mgr_phase0_tbl_get:
 *         Get a pointer to the shadow copy associate with the given table
 *         handle.
 */
pipe_mgr_phase0_tbl_t *pipe_mgr_phase0_tbl_get(bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  bf_map_sts_t status;
  unsigned long key = 0;

  key = mat_tbl_hdl;

  status = pipe_mgr_p0_tbl_map_get(dev_id, key, (void **)&phase0_tbl);

  if (status != BF_MAP_OK) {
    return NULL;
  }

  return phase0_tbl;
}

static pipe_status_t phase0_tbl_data_installed_init(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data) {
  uint8_t num_pipes = 0;
  uint8_t pipe_id = 0;
  for (pipe_id = 0; pipe_id < phase0_tbl->dev_info->num_active_pipes;
       pipe_id++) {
    if ((1u << pipe_id) & phase0_tbl_data->scope_pipe_bmp) num_pipes++;
  }
  phase0_tbl_data->bitset_mem = PIPE_MGR_CALLOC(
      BF_BITSET_ARRAY_SIZE(num_pipes * phase0_tbl->dev_info->dev_cfg.num_ports),
      sizeof(uint64_t));
  if (phase0_tbl_data->bitset_mem == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for phase0 match table data"
        "for table with device id %d",
        __func__,
        phase0_tbl->dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  phase0_tbl_data->num_pipes = num_pipes;

  bf_bs_init(&phase0_tbl_data->installed_entries,
             num_pipes * phase0_tbl->dev_info->dev_cfg.num_ports,
             phase0_tbl_data->bitset_mem);
  return PIPE_SUCCESS;
}

static void phase0_tbl_data_installed_cleanup(
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data) {
  if (phase0_tbl_data->bitset_mem) {
    PIPE_MGR_FREE(phase0_tbl_data->bitset_mem);
    phase0_tbl_data->bitset_mem = NULL;
  }
}

static int phase0_tbl_installed_hdl_to_idx(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_mat_ent_hdl_t entry_hdl) {
  uint32_t port = pipe_mgr_phase0_hdl_to_port(entry_hdl);
  if (!phase0_tbl->dev_info->dev_cfg.dev_port_validate(port)) {
    LOG_ERROR(
        "%s : Error in getting port from entry handle for dev %d, "
        "phase0 tbl 0x%x, handle 0x%x",
        __func__,
        phase0_tbl->dev_id,
        phase0_tbl->mat_tbl_hdl,
        entry_hdl);
    return PIPE_INVALID_ARG;
  }
  uint32_t local_port =
      phase0_tbl->dev_info->dev_cfg.dev_port_to_local_port(port);
  bf_dev_pipe_t pipeid = phase0_tbl->dev_info->dev_cfg.dev_port_to_pipe(port);
  bf_dev_pipe_t pipe;
  uint32_t pipe_idx = 0;
  for (pipe = 0; pipe < phase0_tbl->dev_info->num_active_pipes; pipe++) {
    if (((1u << pipe) & phase0_tbl_data->scope_pipe_bmp) == 0) {
      if (pipe == pipeid) return -1;
      continue;
    }
    if (pipe == pipeid)
      return pipe_idx * phase0_tbl->dev_info->dev_cfg.num_ports + local_port;
    pipe_idx++;
  }
  PIPE_MGR_DBGCHK(0);
  return -1;
}

static bool phase0_tbl_installed_set(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_mat_ent_hdl_t entry_hdl,
    int val) {
  int idx =
      phase0_tbl_installed_hdl_to_idx(phase0_tbl, phase0_tbl_data, entry_hdl);
  return bf_bs_set(&phase0_tbl_data->installed_entries, idx, val);
}

static bool phase0_tbl_installed_get(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_mat_ent_hdl_t entry_hdl) {
  int idx =
      phase0_tbl_installed_hdl_to_idx(phase0_tbl, phase0_tbl_data, entry_hdl);
  return bf_bs_get(&phase0_tbl_data->installed_entries, idx);
}

static pipe_status_t phase0_tbl_data_init(pipe_mgr_phase0_tbl_t *phase0_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t tbl_idx = 0;
  LOG_DBG("%s : num_tbls %d device id %d",
          __func__,
          phase0_tbl->num_tbls,
          phase0_tbl->dev_id);
  pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
      phase0_tbl->dev_id, phase0_tbl->mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get table info for Phase0 match table with"
        " handle %#x for device %d",
        __func__,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (phase0_tbl->num_tbls > 0) {
    phase0_tbl->phase0_tbl_data = (pipe_mgr_phase0_tbl_data_t *)PIPE_MGR_CALLOC(
        phase0_tbl->num_tbls, sizeof(pipe_mgr_phase0_tbl_data_t));

    if (phase0_tbl->phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for phase0 match table data "
          "for table with device id %d",
          __func__,
          phase0_tbl->dev_id);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  bf_dev_pipe_t pipe_id = 0;
  bf_dev_pipe_t lowest_pipe_id = 0;
  /* The num of phase0 table instances is equal to the number of scopes
    In symmetric case, num of scopes will be one.
    In non-symmetric case, num of scopes will be equal to num of
    active pipes in profile.
    In user-defined scope case, num of scopes is based on user config. User
    could add more than one pipe in one scope. In that case, the lowest pipe-id
    is the representative pipe in that scope and user is expected to pass this
    lowest pipe in all PD api calls.
  */
  for (tbl_idx = 0; tbl_idx < phase0_tbl->num_tbls; tbl_idx++) {
    lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(phase0_tbl->scope_pipe_bmp[tbl_idx]);
    /* Init default entry handle */
    phase0_tbl->phase0_tbl_data[tbl_idx].default_entry_hdl =
        phase0_tbl->dev_info->dev_cfg.make_dev_port(
            lowest_pipe_id, phase0_tbl->dev_info->dev_cfg.num_ports) +
        1;
    if (phase0_tbl->symmetric == true) {
      pipe_id = BF_DEV_PIPE_ALL;
    } else {
      pipe_id = lowest_pipe_id;
    }
    phase0_tbl->phase0_tbl_data[tbl_idx].pipe_id = pipe_id;
    phase0_tbl->phase0_tbl_data[tbl_idx].scope_pipe_bmp =
        phase0_tbl->scope_pipe_bmp[tbl_idx];
    status = phase0_tbl_data_installed_init(
        phase0_tbl, &phase0_tbl->phase0_tbl_data[tbl_idx]);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Could not initialize installed phase0 match table data "
          "for table with device id %d",
          __func__,
          phase0_tbl->dev_id);
      return status;
    }
    phase0_tbl->phase0_tbl_data[tbl_idx].ha_hlp_info =
        (pipe_mgr_phase0_pipe_hlp_ha_info_t *)PIPE_MGR_CALLOC(
            1, sizeof(pipe_mgr_phase0_pipe_hlp_ha_info_t));
    if (!phase0_tbl->phase0_tbl_data[tbl_idx].ha_hlp_info) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    pipe_mgr_spec_map_t *spec_map =
        &phase0_tbl->phase0_tbl_data[tbl_idx].ha_hlp_info->spec_map;
    spec_map->dev_tgt.device_id = phase0_tbl->dev_id;
    spec_map->dev_tgt.dev_pipe_id = pipe_id;
    spec_map->mat_tbl_hdl = phase0_tbl->mat_tbl_hdl;
    spec_map->tbl_info = mat_tbl_info;
    spec_map->entry_place_with_hdl_fn = pipe_mgr_phase0_ha_reconcile_ent_place;
    spec_map->entry_modify_fn = pipe_mgr_phase0_ha_reconcile_ent_modify;
    spec_map->entry_update_fn = NULL;
    spec_map->entry_delete_fn = pipe_mgr_phase0_ha_reconcile_ent_del;
  }
  return status;
}

/** \brief pipe_mgr_phase0_tbl_init:
 *         Performs initialization for the given table handle.
 *
 *  \param pipe_mat_tbl_hdl Table handle of the table for which the
 *         initialization is being done.
 *  \return pipe_status_t The status of the operation.
 */
pipe_status_t pipe_mgr_phase0_tbl_init(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t pipe_mat_tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp) {
  LOG_TRACE("Entering %s for dev_id %d, tbl_hdl %#x, profile %d ",
            __func__,
            dev_id,
            pipe_mat_tbl_hdl,
            profile_id);

  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  unsigned long htbl_key = 0UL;
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts;
  uint32_t num_entries = 0, q = 0;
  unsigned i = 0;
  uint32_t max_bytes = 0;

  /* First, check if the table already exists */
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, pipe_mat_tbl_hdl);
  if (phase0_tbl != NULL) {
    LOG_ERROR(
        "%s : Attempt to initialize a phase0 table which "
        "already exists, with table handle %#x, device id %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, pipe_mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get table info for phase0 match table with"
        " handle %#x for device %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  num_entries = mat_tbl_info->size;

  phase0_tbl = (pipe_mgr_phase0_tbl_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_phase0_tbl_t));

  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for phase0 match table with"
        " handle %#x for device %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  phase0_tbl->name = bf_sys_strdup(mat_tbl_info->name);
  phase0_tbl->dev_id = dev_id;
  phase0_tbl->dev_info = pipe_mgr_get_dev_info(dev_id);
  if (phase0_tbl->dev_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  phase0_tbl->num_entries = num_entries;
  phase0_tbl->mat_tbl_hdl = pipe_mat_tbl_hdl;
  phase0_tbl->profile_id = profile_id;
  phase0_tbl->prsr_instance_hdl = mat_tbl_info->prsr_instance_hdl;
  phase0_tbl->symmetric = mat_tbl_info->symmetric;
  phase0_tbl->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!phase0_tbl->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (phase0_tbl->symmetric) {
    phase0_tbl->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) { phase0_tbl->scope_pipe_bmp[0] |= (1 << q); }
  } else {
    phase0_tbl->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      phase0_tbl->scope_pipe_bmp[phase0_tbl->num_scopes] |= (1 << q);
      phase0_tbl->num_scopes += 1;
    }
  }
  phase0_tbl->num_tbls = phase0_tbl->num_scopes;
  /* Allocate and intitialize table instances */
  status = phase0_tbl_data_init(phase0_tbl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Table %s, alloc with new settings failed ",
              __func__,
              phase0_tbl->name);
    goto err_cleanup;
  }

  phase0_tbl->num_entries_placed =
      PIPE_MGR_CALLOC(phase0_tbl->dev_info->num_active_pipes, sizeof(uint32_t));
  if (!phase0_tbl->num_entries_placed) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  phase0_tbl->num_entries_programmed =
      PIPE_MGR_CALLOC(phase0_tbl->dev_info->num_active_pipes, sizeof(uint32_t));
  if (!phase0_tbl->num_entries_programmed) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Allocate the shadow of encoded entries. */
  phase0_tbl->log_pipe_shadow =
      PIPE_MGR_CALLOC(phase0_tbl->dev_info->num_active_pipes,
                      sizeof *phase0_tbl->log_pipe_shadow);
  if (!phase0_tbl->log_pipe_shadow) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  for (unsigned int log_pipe = 0;
       log_pipe < phase0_tbl->dev_info->num_active_pipes;
       ++log_pipe) {
    if (PIPE_BITMAP_GET(pipe_bmp, log_pipe) == 0) continue;
    phase0_tbl->log_pipe_shadow[log_pipe] =
        PIPE_MGR_CALLOC(phase0_tbl->dev_info->dev_cfg.num_ports,
                        phase0_tbl->dev_info->dev_cfg.p0_width * 4);
    if (!phase0_tbl->log_pipe_shadow[log_pipe]) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  /* Store the number of match spec bits and bytes */
  phase0_tbl->num_match_spec_bits = mat_tbl_info->num_match_bits;
  phase0_tbl->num_match_spec_bytes = mat_tbl_info->num_match_bytes;
  if (phase0_tbl->num_match_spec_bytes != 2) {
    PIPE_MGR_DBGCHK(phase0_tbl->num_match_spec_bytes == 2);
    return PIPE_UNEXPECTED;
  }

  phase0_tbl->num_actions = mat_tbl_info->num_actions;
  phase0_tbl->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
      phase0_tbl->num_actions, sizeof(pipe_act_fn_info_t));
  if (!phase0_tbl->act_fn_hdl_info) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(phase0_tbl->act_fn_hdl_info,
                  mat_tbl_info->act_fn_hdl_info,
                  sizeof(pipe_act_fn_info_t) * phase0_tbl->num_actions);

  for (i = 0; i < phase0_tbl->num_actions; i++) {
    if (phase0_tbl->act_fn_hdl_info[i].num_bytes > max_bytes) {
      max_bytes = phase0_tbl->act_fn_hdl_info[i].num_bytes;
    }
  }
  phase0_tbl->max_act_data_size = max_bytes;

  htbl_key = pipe_mat_tbl_hdl;
  map_sts = pipe_mgr_p0_tbl_map_add(dev_id, htbl_key, (void *)phase0_tbl);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s : Error in inserting phase0 table structure into"
        " the hash table with table handle %#x, device id %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    goto err_cleanup;
  }

  /* Initializing HA related stuff */
  status = pipe_mgr_phase0_tbl_ha_init(phase0_tbl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing ha info for phase0 table with table handle "
        "%#x, device id %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    goto err_cleanup;
  }

  LOG_TRACE("Exiting %s successfully for dev_id %d, tbl_hdl %#x",
            __func__,
            dev_id,
            pipe_mat_tbl_hdl);

  return PIPE_SUCCESS;

err_cleanup:
  if (phase0_tbl) {
    pipe_mgr_phase0_tbl_delete(dev_id, pipe_mat_tbl_hdl);
  }
  return status;
}

static void phase0_tbl_data_cleanup(pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
                                    uint32_t num_instances) {
  struct pipe_mgr_mat_data *data = NULL;
  bf_map_sts_t map_sts;
  unsigned long key;
  uint32_t i = 0;

  if (phase0_tbl_data) {
    for (i = 0; i < num_instances; i++) {
      if (phase0_tbl_data[i].dflt_act_spec)
        pipe_mgr_tbl_destroy_action_spec(&phase0_tbl_data[i].dflt_act_spec);
      phase0_tbl_data_installed_cleanup(&phase0_tbl_data[i]);
      if (phase0_tbl_data[i].ha_hlp_info) {
        PIPE_MGR_FREE(phase0_tbl_data[i].ha_hlp_info);
      }

      /* Remove all the entries from the map. */
      key = 0;
      while ((map_sts = bf_map_get_first_rmv(
                  &phase0_tbl_data[i].ent_data, &key, (void **)&data)) ==
             BF_MAP_OK) {
        free_mat_ent_data(data);
      }
    }
    PIPE_MGR_FREE(phase0_tbl_data);
  }
}

/* Phase0 DB delete */
void pipe_mgr_phase0_tbl_delete(uint8_t device_id,
                                pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  LOG_TRACE("Entering %s for table handle %#x, device id %d",
            __func__,
            mat_tbl_hdl,
            device_id);

  bf_map_sts_t map_sts;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;

  /* Walk over this structure and de-allocate heap memory allocated
   * for all the sub-structures associated with this structure and
   * ultimately free the top-level structure.
   */

  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);

  if (phase0_tbl == NULL) {
    LOG_TRACE(
        "%s : Request to delete a non-existent table with handle %#x"
        " device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    /* Nothing to free */
    return;
  }

  map_sts = pipe_mgr_p0_tbl_map_rmv(device_id, mat_tbl_hdl);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s : Error %d in removing the phase0 match table from the "
        "entry handle to table structure hash table, table handle %#x",
        __func__,
        map_sts,
        mat_tbl_hdl);
  }

  if (phase0_tbl->num_entries_placed) {
    PIPE_MGR_FREE(phase0_tbl->num_entries_placed);
  }
  if (phase0_tbl->num_entries_programmed) {
    PIPE_MGR_FREE(phase0_tbl->num_entries_programmed);
  }

  phase0_tbl_data_cleanup(phase0_tbl->phase0_tbl_data, phase0_tbl->num_tbls);
  phase0_tbl->phase0_tbl_data = NULL;

  if (phase0_tbl->scope_pipe_bmp) {
    PIPE_MGR_FREE(phase0_tbl->scope_pipe_bmp);
  }

  if (phase0_tbl->act_fn_hdl_info) {
    PIPE_MGR_FREE(phase0_tbl->act_fn_hdl_info);
  }

  if (phase0_tbl->log_pipe_shadow) {
    for (unsigned int log_pipe = 0;
         log_pipe < phase0_tbl->dev_info->num_active_pipes;
         ++log_pipe)
      if (phase0_tbl->log_pipe_shadow[log_pipe]) {
        PIPE_MGR_FREE(phase0_tbl->log_pipe_shadow[log_pipe]);
      }
    PIPE_MGR_FREE(phase0_tbl->log_pipe_shadow);
  }

  pipe_mgr_phase0_tbl_ha_deinit(phase0_tbl);

  PIPE_MGR_FREE(phase0_tbl->name);
  PIPE_MGR_FREE(phase0_tbl);

  LOG_TRACE("Exiting %s for table handle %#x, device id %d successfully",
            __func__,
            mat_tbl_hdl,
            device_id);

  return;
}

pipe_status_t pipe_mgr_phase0_set_symmetric_mode(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  /* Scope setting is not relevant for phase0 tables as the key is
     the port-id
  */
  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);

  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Request to change sym mode on non-existent table with handle %#x"
        " device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_INVALID_ARG;
  }

  phase0_tbl->symmetric = symmetric;
  phase0_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(phase0_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  /* First cleanup table data. */
  phase0_tbl_data_cleanup(phase0_tbl->phase0_tbl_data, phase0_tbl->num_tbls);
  phase0_tbl->phase0_tbl_data = NULL;

  phase0_tbl->num_tbls = phase0_tbl->num_scopes;
  /* Allocate and initialize table instances. */
  rc = phase0_tbl_data_init(phase0_tbl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s: Table %s, alloc with new settings failed ",
              __func__,
              phase0_tbl->name);
  }
  return rc;
}

pipe_status_t pipe_mgr_phase0_get_symmetric_mode(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;

  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);

  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Request to change sym mode on non-existent table with handle %#x"
        " device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_INVALID_ARG;
  }
  *symmetric = phase0_tbl->symmetric;
  *num_scopes = phase0_tbl->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  phase0_tbl->scope_pipe_bmp,
                  phase0_tbl->num_scopes * sizeof(scope_pipes_t));
  return PIPE_SUCCESS;
}

static pipe_mat_ent_hdl_t phase0_tbl_dev_port_to_entry_handle(
    bf_dev_port_t port) {
  return port + 1;
}

/* Get phase0 entry DB */
static pipe_status_t phase0_tbl_get_ent_hdl(pipe_tbl_match_spec_t *match_spec,
                                            pipe_mgr_phase0_tbl_t *phase0_tbl,
                                            pipe_mat_ent_hdl_t *entry_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t port = 0;
  status = pipe_mgr_entry_format_tof_phase0_tbl_get_match_port(
      phase0_tbl->dev_id,
      phase0_tbl->profile_id,
      phase0_tbl->mat_tbl_hdl,
      match_spec,
      &port);
  LOG_TRACE("%s: Match Port is %d (%#x)", __func__, port, port);
  /* Use port-id as entry-handle */
  *entry_hdl = phase0_tbl_dev_port_to_entry_handle(port);

  return status;
}
bf_dev_port_t pipe_mgr_phase0_hdl_to_port(pipe_mat_ent_hdl_t hdl) {
  return (PIPE_GET_HDL_VAL(hdl) - 1);
}

static bf_dev_pipe_t hdl_to_pipe(rmt_dev_info_t *dev_info,
                                 pipe_mat_ent_hdl_t hdl) {
  return dev_info->dev_cfg.dev_port_to_pipe(pipe_mgr_phase0_hdl_to_port(hdl));
}

static pipe_status_t save_ent_data(pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
                                   pipe_mat_ent_hdl_t entry_hdl,
                                   struct pipe_mgr_mat_data *data) {
  bf_map_sts_t msts =
      bf_map_add(&phase0_tbl_data->ent_data, entry_hdl, (void *)data);
  if (BF_MAP_OK != msts) {
    PIPE_MGR_DBGCHK(BF_MAP_OK == msts);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}
static pipe_status_t rmv_ent_data(pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
                                  pipe_mat_ent_hdl_t entry_hdl,
                                  struct pipe_mgr_mat_data **data) {
  bf_map_sts_t msts =
      bf_map_get_rmv(&phase0_tbl_data->ent_data, entry_hdl, (void **)data);
  if (BF_MAP_OK != msts) {
    *data = NULL;
    PIPE_MGR_DBGCHK(BF_MAP_OK == msts);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}
static pipe_status_t get_ent_data(pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
                                  pipe_mat_ent_hdl_t entry_hdl,
                                  struct pipe_mgr_mat_data **data) {
  bf_map_sts_t msts =
      bf_map_get(&phase0_tbl_data->ent_data, entry_hdl, (void **)data);
  if (BF_MAP_OK != msts) {
    *data = NULL;
    return PIPE_OBJ_NOT_FOUND;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_get_ent_hdl(bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_tbl_match_spec_t *match_spec,
                                          pipe_mat_ent_hdl_t *entry_hdl) {
  pipe_status_t status;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  pipe_mgr_phase0_tbl_t *phase0_tbl;
  bf_dev_pipe_t pipe;

  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);

  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Requesting an entry from a non-existent table with handle %#x"
        " device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  status = phase0_tbl_get_ent_hdl(match_spec, phase0_tbl, entry_hdl);
  if (status == PIPE_SUCCESS) {
    pipe = hdl_to_pipe(phase0_tbl->dev_info, *entry_hdl);
    phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
    if (!phase0_tbl_data) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          pipe);
      return PIPE_OBJ_NOT_FOUND;
    }

    struct pipe_mgr_mat_data *data = NULL;
    status = get_ent_data(phase0_tbl_data, *entry_hdl, &data);
  }

  return status;
}

static pipe_status_t log_op_for_txn(pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
                                    pipe_mgr_move_list_t *src_ml) {
  /* Allocate a new move list element. */
  pipe_mgr_move_list_t *ml = alloc_move_list(NULL, src_ml->op, BF_DEV_PIPE_ALL);
  if (!ml) return PIPE_NO_SYS_RESOURCES;

  /* Copy from the original move list element so that we can keep a record of
   * all operations done during the transaction. */
  ml->entry_hdl = src_ml->entry_hdl;
  ml->data = src_ml->data;
  if (PIPE_MAT_UPDATE_ADD == src_ml->op) {
    ml->u.single.logical_idx = src_ml->u.single.logical_idx;
  } else if (PIPE_MAT_UPDATE_MOD == src_ml->op) {
    ml->old_data = src_ml->old_data;
  } else if (PIPE_MAT_UPDATE_DEL == src_ml->op) {
    /* Nothing else to copy for deletes. */
  } else if (PIPE_MAT_UPDATE_SET_DFLT == src_ml->op) {
    /* Nothing else to copy for set default, There should be no old data since
     * this case is handled with operation PIPE_MAT_UPDATE_MOD. */
    PIPE_MGR_DBGCHK(src_ml->old_data == NULL)
  } else if (PIPE_MAT_UPDATE_CLR_DFLT == src_ml->op) {
    /* Nothing else to copy for clear default. */
  } else {
    return PIPE_UNEXPECTED;
  }

  /* prepend it to the move list stored against this table.  By prepending we
   * can walk the operations in reverse order from which they were issued. */
  ml->next = phase0_tbl_data->txn_data;
  phase0_tbl_data->txn_data = ml;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_tbl_txn_commit(bf_dev_id_t dev_id,
                                             pipe_mat_ent_hdl_t mat_tbl_hdl,
                                             bf_dev_pipe_t *pipes_list,
                                             unsigned nb_pipes) {
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  pipe_mgr_phase0_tbl_t *phase0_tbl;
  unsigned i, pipe;
  bool symmetric;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    PIPE_MGR_DBGCHK(phase0_tbl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (phase0_tbl->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          pipe);
      continue;
    }

    pipe_mgr_move_list_t *ml = phase0_tbl_data->txn_data;
    while (ml) {
      pipe_mgr_move_list_t *x = ml;
      ml = ml->next;
      PIPE_MGR_FREE(x);
    }
    phase0_tbl_data->txn_data = NULL;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_tbl_txn_abort(bf_dev_id_t dev_id,
                                            pipe_mat_ent_hdl_t mat_tbl_hdl,
                                            bf_dev_pipe_t *pipes_list,
                                            unsigned nb_pipes) {
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  pipe_mgr_phase0_tbl_t *phase0_tbl;
  unsigned i, pipe;
  bool symmetric;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    PIPE_MGR_DBGCHK(phase0_tbl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (phase0_tbl->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          pipe);
      continue;
    }

    pipe_mgr_move_list_t *ml = phase0_tbl_data->txn_data;
    while (ml) {
      struct pipe_mgr_mat_data *throw_away_data = NULL;
      pipe_status_t status = PIPE_SUCCESS;
      switch (ml->op) {
        case PIPE_MAT_UPDATE_ADD:
          /* Undo an entry add simply by removing the handle to entry data
           * mapping. */
          status =
              rmv_ent_data(phase0_tbl_data, ml->entry_hdl, &throw_away_data);
          PIPE_MGR_DBGCHK(throw_away_data == ml->data);
          break;
        case PIPE_MAT_UPDATE_MOD:
          /* Undo an entry modify by removing the new entry data and restoring
           * the old entry data for the entry handle. */
          status =
              rmv_ent_data(phase0_tbl_data, ml->entry_hdl, &throw_away_data);
          PIPE_MGR_DBGCHK(throw_away_data == ml->data);
          status |= save_ent_data(phase0_tbl_data, ml->entry_hdl, ml->old_data);
          break;
        case PIPE_MAT_UPDATE_DEL:
          /* Undo an entry delete simply by re-inserting the entry handle to
           * entry data mapping. */
          status = save_ent_data(phase0_tbl_data, ml->entry_hdl, ml->data);
          break;
        case PIPE_MAT_UPDATE_SET_DFLT:
          /* Undo a set default by removing the new entry data. */
          status =
              rmv_ent_data(phase0_tbl_data, ml->entry_hdl, &throw_away_data);
          PIPE_MGR_DBGCHK(throw_away_data == ml->data);
          break;
        case PIPE_MAT_UPDATE_CLR_DFLT:
          /* Undo a clear default. */
          status = save_ent_data(phase0_tbl_data, ml->entry_hdl, ml->data);
          break;
        default:
          status = PIPE_UNEXPECTED;
          break;
      }
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
      pipe_mgr_move_list_t *x = ml;
      ml = ml->next;
      PIPE_MGR_FREE(x);
    }
    phase0_tbl_data->txn_data = NULL;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t phase0_tbl_program_check_prsr(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mat_ent_hdl_t entry_hdl,
    bool *configure) {
  uint32_t port = pipe_mgr_phase0_hdl_to_port(entry_hdl);
  if (!phase0_tbl->dev_info->dev_cfg.dev_port_validate(port)) {
    LOG_ERROR(
        "%s : Error in getting port from entry handle for dev %d, "
        "phase0 tbl 0x%x, handle 0x%x",
        __func__,
        phase0_tbl->dev_id,
        phase0_tbl->mat_tbl_hdl,
        entry_hdl);
    return PIPE_INVALID_ARG;
  }
  uint32_t local_port =
      phase0_tbl->dev_info->dev_cfg.dev_port_to_local_port(port);
  bf_dev_pipe_t pipeid = phase0_tbl->dev_info->dev_cfg.dev_port_to_pipe(port);
  uint64_t prsr_map;
  uint8_t parser_id;
  // only on ingress
  pipe_status_t status = pipe_mgr_prsr_instance_get_profile(
      phase0_tbl->dev_id, pipeid, 0, phase0_tbl->prsr_instance_hdl, &prsr_map);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in getting parser map with parser instance handler %#x, "
        "dev %d, error %s",
        __func__,
        phase0_tbl->prsr_instance_hdl,
        phase0_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  switch (phase0_tbl->dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      parser_id = local_port / TOF_NUM_CHN_PER_PORT;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      parser_id = local_port / TOF2_NUM_CHN_PER_IPB;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      parser_id = local_port / TOF3_NUM_CHN_PER_IPB;
      break;
    default:
      LOG_ERROR(
          "%s : Invalid dev family dev %d ", __func__, phase0_tbl->dev_id);
      return PIPE_UNEXPECTED;
  }
  if (((1u << parser_id) & prsr_map) == 0) {
    *configure = false;
    return PIPE_SUCCESS;
  }
  *configure = true;
  return PIPE_SUCCESS;
}

/* Check if tbl exists on the pipe received from match spec port */
static pipe_status_t pipe_mgr_phase0_check_tbl_exists_on_pipe(
    pipe_mgr_phase0_tbl_t *phase0_tbl, bf_dev_pipe_t pipe) {
  int s_cnt = 0;
  if (pipe >= phase0_tbl->dev_info->num_active_pipes) {
    LOG_ERROR(
        "%s:%d Out of range pipeline %d for phase0 tbl 0x%x "
        "device %d",
        __func__,
        __LINE__,
        pipe,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return PIPE_INVALID_ARG;
  }
  for (s_cnt = 0; s_cnt < phase0_tbl->num_scopes; s_cnt++) {
    if (phase0_tbl->scope_pipe_bmp[s_cnt] & (1u << pipe)) {
      return PIPE_SUCCESS;
    }
  }
  LOG_ERROR(
      "%s:%d Invalid pipeline %d for phase0 tbl 0x%x "
      "device %d",
      __func__,
      __LINE__,
      pipe,
      phase0_tbl->mat_tbl_hdl,
      phase0_tbl->dev_id);
  return PIPE_INVALID_ARG;
}

static pipe_mgr_phase0_tbl_data_t *phase0_tbl_get_instance(
    pipe_mgr_phase0_tbl_t *phase0_tbl, bf_dev_pipe_t pipe_id) {
  if (!phase0_tbl->symmetric && pipe_id == BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for asymmetric phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        pipe_id,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return NULL;
  }
  if (phase0_tbl->symmetric && pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        pipe_id,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return NULL;
  }
  for (unsigned i = 0; i < phase0_tbl->num_tbls; i++) {
    if (phase0_tbl->phase0_tbl_data[i].pipe_id == pipe_id) {
      return &phase0_tbl->phase0_tbl_data[i];
    }
  }
  return NULL;
}

static pipe_mgr_phase0_tbl_data_t *phase0_tbl_get_instance_from_any_pipe(
    pipe_mgr_phase0_tbl_t *phase0_tbl, bf_dev_pipe_t pipe_id) {
  int s_cnt = 0;

  for (s_cnt = 0; s_cnt < phase0_tbl->num_scopes; s_cnt++) {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      if (phase0_tbl->phase0_tbl_data[s_cnt].pipe_id == BF_DEV_PIPE_ALL)
        return &phase0_tbl->phase0_tbl_data[s_cnt];
    } else if (phase0_tbl->scope_pipe_bmp[s_cnt] & (1u << pipe_id)) {
      return &phase0_tbl->phase0_tbl_data[s_cnt];
    }
  }
  return NULL;
}

static pipe_mgr_phase0_tbl_data_t *phase0_tbl_get_instance_from_entry(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    int entry_hdl,
    const char *where,
    const int line) {
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  bf_dev_pipe_t pipe_id;

  if (phase0_tbl->symmetric) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[0];
  } else {
    pipe_id = hdl_to_pipe(phase0_tbl->dev_info, entry_hdl);
    phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, pipe_id);
    if (!phase0_tbl_data) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          where,
          line,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          pipe_id);
    }
  }
  return phase0_tbl_data;
}

static bool phase0_tbl_is_default_ent_installed(
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data) {
  if (phase0_tbl_data == NULL || phase0_tbl_data->dflt_act_spec == NULL) {
    return false;
  }
  return true;
}

/* Program the phase0 registers */
static pipe_status_t phase0_tbl_program(pipe_sess_hdl_t sess_hdl,
                                        pipe_action_spec_t *act_data_spec,
                                        pipe_mgr_phase0_tbl_t *phase0_tbl,
                                        pipe_mat_ent_hdl_t entry_hdl,
                                        bool is_default,
                                        bool is_delete,
                                        bool is_default_clear,
                                        bool cfg_hw) {
  /* port is entry handle - 1 since, we pass an entry handle of port + 1 when
   * programming
   * an entry in the phase0 table.
   */
  rmt_dev_info_t *dev_info = phase0_tbl->dev_info;
  uint32_t port = pipe_mgr_phase0_hdl_to_port(entry_hdl);
  uint32_t p0_width = dev_info->dev_cfg.p0_width;
  pipe_register_spec_t reg_spec_list[p0_width];
  pipe_memory_spec_t mem_spec_list;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t pipe_id = 0;
  uint32_t local_port = 0;
  pipe_bitmap_t pipe_bmp;
  pipe_action_spec_t act_spec;
  pipe_mgr_action_entry_t *action_entry;
  pipe_mat_tbl_info_t *mat_tbl_info;
  bool is_default_available = true;
  bool as_created = false;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;

  /* Entry handle of 0 is invalid. Also validate port derived from the handle.
   */
  if ((entry_hdl == 0) || (!dev_info->dev_cfg.dev_port_validate(port))) {
    LOG_ERROR(
        "%s : Invalid entry handle %#x passed for phase0 table 0x%x, device id "
        "%d",
        __func__,
        entry_hdl,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_id = hdl_to_pipe(dev_info, entry_hdl);
  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_MEMSET(reg_spec_list, 0, p0_width * sizeof reg_spec_list[0]);
  PIPE_MGR_MEMSET(&mem_spec_list, 0, sizeof(mem_spec_list));

  if (is_delete || is_default_clear) {
    if (phase0_tbl_is_default_ent_installed(phase0_tbl_data) &&
        !is_default_clear) {
      act_data_spec = phase0_tbl_data->dflt_act_spec;
    } else {
      mat_tbl_info = pipe_mgr_get_tbl_info(
          phase0_tbl->dev_id, phase0_tbl->mat_tbl_hdl, __func__, __LINE__);
      if (!mat_tbl_info) {
        LOG_ERROR(
            "%s:%d Unable to find match table info for Phase0 table 0x%x "
            "device id %d",
            __func__,
            __LINE__,
            phase0_tbl->mat_tbl_hdl,
            phase0_tbl->dev_id);
        return PIPE_INVALID_ARG;
      }
      if (mat_tbl_info->default_info) {
        action_entry = &mat_tbl_info->default_info->action_entry;
        /* Populate with default action spec */
        status = pipe_mgr_create_action_spec(
            phase0_tbl->dev_id, action_entry, &act_spec);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s : Failed to delete entry handle %#x passed for phase0 table "
              "0x%x, device id %d",
              __func__,
              entry_hdl,
              phase0_tbl->mat_tbl_hdl,
              phase0_tbl->dev_id);
          return status;
        }
        as_created = true;
        act_data_spec = &act_spec;
      } else {
        is_default_available = false;
      }
    }
  }

  bf_subdev_id_t subdev;
  if (is_default_available == false) {
    status = pipe_mgr_entry_format_phase0_tbl_addr_get(
        dev_info, port, true, &subdev, reg_spec_list, &mem_spec_list);
  } else {
    status = pipe_mgr_entry_format_phase0_tbl_update(dev_info,
                                                     phase0_tbl->profile_id,
                                                     phase0_tbl->mat_tbl_hdl,
                                                     port,
                                                     &act_data_spec->act_data,
                                                     reg_spec_list,
                                                     &mem_spec_list);
  }
  if (as_created && act_spec.act_data.action_data_bits) {
    PIPE_MGR_FREE(act_spec.act_data.action_data_bits);
  }
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in encoding the phase0 tbl table with handle %#x, error %s",
        __func__,
        phase0_tbl->mat_tbl_hdl,
        pipe_str_err(status));
    return status;
  }

  local_port = dev_info->dev_cfg.dev_port_to_local_port(port);
  int shadow_offset = local_port * 4 * p0_width;
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bmp, pipe_id);

  for (unsigned int i = 0, j = 0; i < p0_width; ++i) {
    /* Save the encoded data in the shadow. */
    uint32_t x = reg_spec_list[i].reg_data;
    phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset + j++] = (x >> 0) & 255;
    phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset + j++] = (x >> 8) & 255;
    phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset + j++] = (x >> 16) & 255;
    phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset + j++] = (x >> 24) & 255;
  }
  /* If the device is locked then skip all HW programming. */
  if ((!pipe_mgr_is_device_locked(phase0_tbl->dev_id) ||
       pipe_mgr_hitless_warm_init_in_progress(phase0_tbl->dev_id)) &&
      cfg_hw) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
      case BF_DEV_FAMILY_TOFINO2: {
        /* Post instructions for updating the registers in IPB/IBUF. */
        uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(
            reg_spec_list[0].reg_addr);
        for (unsigned int i = 0; i < p0_width; ++i) {
          LOG_TRACE(
              "%s : Writing phase0 tbl hdl 0x%x, register addr"
              " 0x%x value 0x%x",
              __func__,
              phase0_tbl->mat_tbl_hdl,
              reg_spec_list[i].reg_addr,
              reg_spec_list[i].reg_data);

          pipe_instr_write_reg_t instr;
          construct_instr_reg_write(phase0_tbl->dev_id,
                                    &instr,
                                    reg_spec_list[i].reg_addr,
                                    reg_spec_list[i].reg_data);
          status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                          dev_info,
                                          &pipe_bmp,
                                          stage,
                                          (uint8_t *)&instr,
                                          sizeof(pipe_instr_write_reg_t));
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s : IL add failed for p0 table entry hdl %#x table handle "
                "%#x %s "
                "on dev %d pipe %d reg %#x, error %s",
                __func__,
                entry_hdl,
                phase0_tbl->mat_tbl_hdl,
                is_delete ? "del" : "add",
                phase0_tbl->dev_id,
                pipe_id,
                reg_spec_list[i].reg_addr,
                pipe_str_err(status));
            return status;
          }
        }
        break;
      }
      case BF_DEV_FAMILY_TOFINO3: {
        pipe_instr_set_memdata_i_only_t instr;
        /* Program ipb0 */
        uint64_t addr = mem_spec_list.mem_addr0;
        uint32_t lower_addr = addr & 0xFFFFFFFF;
        uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr);
        construct_instr_set_memdata_by_addr(dev_info, &instr, 16, lower_addr);
        status = pipe_mgr_drv_ilist_add_2(
            &sess_hdl,
            dev_info,
            &pipe_bmp,
            stage,
            (uint8_t *)&instr,
            sizeof instr,
            &phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset],
            16);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR("Failed to post ilist, dev %d log-pipe %d port %d, sts %s",
                    dev_info->dev_id,
                    pipe_id,
                    local_port,
                    pipe_str_err(status));
          return status;
        }

        /* Program ipb for the local-port */
        addr = mem_spec_list.mem_addr1;
        lower_addr = addr & 0xFFFFFFFF;
        stage = dev_info->dev_cfg.stage_id_from_addr(addr);
        construct_instr_set_memdata_by_addr(dev_info, &instr, 16, lower_addr);
        status = pipe_mgr_drv_ilist_add_2(
            &sess_hdl,
            dev_info,
            &pipe_bmp,
            stage,
            (uint8_t *)&instr,
            sizeof instr,
            &phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset],
            16);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR("Failed to post ilist, dev %d log-pipe %d port %d, sts %s",
                    dev_info->dev_id,
                    pipe_id,
                    local_port,
                    pipe_str_err(status));
          return status;
        }
        break;
      }

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
        break;
    }
    /* Post an instruction for updating memory in PGR (for Tofino2). */
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        break;
      case BF_DEV_FAMILY_TOFINO2: {
        pipe_instr_set_memdata_i_only_t instr;
        /* Ilist will take care of pipe so we only need the offset within the
         * pipe here. */
        uint64_t base =
            tof2_mem_pipes_parde_pgr_ph0_rspec_phase0_mem_word_address;
        uint64_t step =
            tof2_mem_pipes_parde_pgr_ph0_rspec_phase0_mem_word_array_element_size;
        uint64_t limit =
            tof2_mem_pipes_parde_pgr_ph0_rspec_phase0_mem_word_array_index_max;
        PIPE_MGR_DBGCHK((uint64_t)local_port <= limit);
        uint64_t addr = (base + local_port * step) >> 4;
        uint32_t lower_addr = addr & 0xFFFFFFFF;
        uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr);
        construct_instr_set_memdata_by_addr(dev_info, &instr, 16, lower_addr);
        status = pipe_mgr_drv_ilist_add_2(
            &sess_hdl,
            dev_info,
            &pipe_bmp,
            stage,
            (uint8_t *)&instr,
            sizeof instr,
            &phase0_tbl->log_pipe_shadow[pipe_id][shadow_offset],
            16);
        break;
      }
      case BF_DEV_FAMILY_TOFINO3: {
        break;
      }

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
        break;
    }
  }
  phase0_tbl_installed_set(phase0_tbl,
                           phase0_tbl_data,
                           entry_hdl,
                           (is_default || is_delete) ? 0 : 1);
  return status;
}

static void log_entry_add(pipe_mgr_phase0_tbl_t *phase0_tbl,
                          pipe_tbl_match_spec_t *match_spec,
                          pipe_act_fn_hdl_t act_fn_hdl,
                          pipe_action_spec_t *act_data_spec) {
  LOG_TRACE("%s Request to add phase0 entry on dev %d",
            phase0_tbl->name,
            phase0_tbl->dev_id);
  pipe_mgr_entry_format_log_match_spec(phase0_tbl->dev_id,
                                       BF_LOG_DBG,
                                       phase0_tbl->profile_id,
                                       phase0_tbl->mat_tbl_hdl,
                                       match_spec);
  pipe_mgr_entry_format_log_action_spec(phase0_tbl->dev_id,
                                        BF_LOG_DBG,
                                        phase0_tbl->profile_id,
                                        act_data_spec,
                                        act_fn_hdl);
}

static void log_entry_mod(pipe_mgr_phase0_tbl_t *phase0_tbl,
                          pipe_mat_ent_hdl_t ent_hdl,
                          pipe_act_fn_hdl_t act_fn_hdl,
                          pipe_action_spec_t *act_data_spec) {
  LOG_TRACE("%s Request to modify phase0 entry with handle %#x on dev %d",
            phase0_tbl->name,
            ent_hdl,
            phase0_tbl->dev_id);
  pipe_mgr_entry_format_log_action_spec(phase0_tbl->dev_id,
                                        BF_LOG_DBG,
                                        phase0_tbl->profile_id,
                                        act_data_spec,
                                        act_fn_hdl);
}

static void log_entry_set_default(pipe_mgr_phase0_tbl_t *phase0_tbl,
                                  pipe_mat_ent_hdl_t ent_hdl,
                                  pipe_act_fn_hdl_t act_fn_hdl,
                                  pipe_action_spec_t *act_data_spec) {
  LOG_TRACE(
      "%s Request to install Phase0 default entry with handle %#x on dev %d",
      phase0_tbl->name,
      ent_hdl,
      phase0_tbl->dev_id);
  LOG_DBG("%s : Phase0 entry Action spec", phase0_tbl->name);
  pipe_mgr_entry_format_log_action_spec(phase0_tbl->dev_id,
                                        BF_LOG_DBG,
                                        phase0_tbl->profile_id,
                                        act_data_spec,
                                        act_fn_hdl);
}

static pipe_status_t pipe_mgr_phase0_check_tbl_symm_and_pipe(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    bf_dev_pipe_t pipe_id,
    bf_dev_pipe_t pipe) {
  if (phase0_tbl->symmetric && pipe_id != BF_DEV_PIPE_ALL)
    return PIPE_INVALID_ARG;
  if (!phase0_tbl->symmetric) {
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data =
        phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
    if (!phase0_tbl_data || phase0_tbl_data->pipe_id != pipe_id)
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_phase0_ent_place_internal(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *action_spec,
    pipe_mat_ent_hdl_t *mat_ent_hdl,
    pipe_mgr_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  pipe_mgr_phase0_tbl_t *phase0_tbl;
  *move_list = NULL;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_dev_pipe_t pipe = hdl_to_pipe(phase0_tbl->dev_info, *mat_ent_hdl);
  if (pipe >= phase0_tbl->dev_info->num_active_pipes) {
    LOG_ERROR(
        "%s:%d Given port translated to invalid pipeline for phase0 tbl 0x%x "
        "device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  status = pipe_mgr_phase0_check_tbl_symm_and_pipe(
      phase0_tbl, dev_tgt.dev_pipe_id, pipe);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for %s phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        phase0_tbl->symmetric ? "symmetric" : "asymmetric",
        mat_tbl_hdl,
        dev_tgt.device_id);
    return status;
  }
  status = pipe_mgr_phase0_check_tbl_exists_on_pipe(phase0_tbl, pipe);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Phase0 tbl with handle 0x%x does not exists on pipe %d "
        "(derived from match spec port), device id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        pipe,
        dev_tgt.device_id);
    return status;
  }

  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
  if (!phase0_tbl_data) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Check if this entry already exists, if it does, switch to a modify. */
  struct pipe_mgr_mat_data *old_data = NULL;
  status = get_ent_data(phase0_tbl_data, *mat_ent_hdl, &old_data);
  if (PIPE_SUCCESS == status) {
    PIPE_MGR_DBGCHK(old_data);
    /* Entry already exists so modify it. */
    return pipe_mgr_phase0_ent_set_action(sess_hdl,
                                          dev_tgt.device_id,
                                          mat_tbl_hdl,
                                          *mat_ent_hdl,
                                          act_fn_hdl,
                                          action_spec,
                                          move_list);
  }

  /* Allocate and fill a move list entry with the placement decision. */
  pipe_mgr_move_list_t *ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  *move_list = ml;
  ml->entry_hdl = *mat_ent_hdl;
  ml->u.single.logical_idx = PIPE_GET_HDL_VAL(*mat_ent_hdl - 1);
  ml->data = make_mat_ent_data(match_spec, action_spec, act_fn_hdl, 0, 0, 0, 0);
  if (!ml->data) {
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_NO_SYS_RESOURCES;
  }

  status = save_ent_data(phase0_tbl_data, *mat_ent_hdl, ml->data);
  if (PIPE_SUCCESS != status) {
    free_mat_ent_data(ml->data);
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_NO_SYS_RESOURCES;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    status = log_op_for_txn(phase0_tbl_data, ml);
    if (PIPE_SUCCESS != status) {
      free_mat_ent_data(ml->data);
      PIPE_MGR_FREE(ml);
      *move_list = NULL;
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  phase0_tbl->num_entries_placed[pipe]++;

  return PIPE_SUCCESS;
}

static bool phase0_tbl_is_ent_hdl_default(pipe_mgr_phase0_tbl_t *phase0_tbl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl) {
  bf_dev_pipe_t pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data =
      phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
  return phase0_tbl_data && (mat_ent_hdl == phase0_tbl_data->default_entry_hdl);
}

static bool is_tbl_data_ent_hdl_default(
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return (mat_ent_hdl == phase0_tbl_data->default_entry_hdl);
}

pipe_status_t pipe_mgr_phase0_get_plcmt_data(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mgr_move_list_t **move_list) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_move_list_t *ml_head = NULL, *ml_tail = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  struct pipe_mgr_mat_data *data = NULL;
  unsigned long key;
  bf_map_t ent_data;
  bf_map_sts_t ms;
  uint32_t i;

  /* Walk through all table instances */
  for (i = 0; i < phase0_tbl->num_tbls; i++) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[i];
    ent_data = &phase0_tbl_data->ent_data;

    for (ms = bf_map_get_first(ent_data, &key, (void **)&data); ms == BF_MAP_OK;
         ms = bf_map_get_next(ent_data, &key, (void **)&data)) {
      pipe_mat_ent_hdl_t mat_ent_hdl = key;
      bf_dev_pipe_t pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);

      bool is_dflt = is_tbl_data_ent_hdl_default(phase0_tbl_data, mat_ent_hdl);
      if (is_dflt) pipe = phase0_tbl_data->pipe_id;
      pipe_mgr_move_list_t *ml = alloc_move_list(
          NULL, is_dflt ? PIPE_MAT_UPDATE_SET_DFLT : PIPE_MAT_UPDATE_ADD, pipe);
      if (!ml) {
        /* Clean up anything already allocated. */
        while (ml_head) {
          pipe_mgr_move_list_t *x = ml_head;
          ml_head = ml_head->next;
          PIPE_MGR_FREE(x);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      ml->entry_hdl = mat_ent_hdl;
      ml->u.single.logical_idx = PIPE_GET_HDL_VAL(mat_ent_hdl - 1);
      ml->data = data;
      if (!ml_head) {
        ml_head = ml;
      } else {
        ml_tail->next = ml;
      }
      ml_tail = ml;
    }
  }

  *move_list = ml_head;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_list) {
  (void)ttl;
  (void)pipe_api_flags;

  pipe_sess_hdl_t sess_hdl = 0;

  sess_hdl = pipe_mgr_get_int_sess_hdl();

  return pipe_mgr_phase0_ent_place_internal(sess_hdl,
                                            dev_tgt,
                                            mat_tbl_hdl,
                                            match_spec,
                                            act_fn_hdl,
                                            act_spec,
                                            &ent_hdl,
                                            move_list);
}

/** \brief pipe_mgr_phase0_ent_place:
 *         Installs a new entry into the given phase0 table.
 *
 * \param dev_tgt Device id of the device onto which this entry should be
 *installed.
 * \param mat_tbl_hdl Handle associated with the match entry table.
 * \param match_spec Pointer to the match spec associated with the entry.
 * \param act_fn_hdl Handle of the action function associated with the entry.
 * \param action_spec Pointer to the action spec associated with the entry.
 * \return pipe_status_t Status of this operation.
 */
pipe_status_t pipe_mgr_phase0_ent_place(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *match_spec,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *action_spec,
                                        pipe_mat_ent_hdl_t *mat_ent_hdl,
                                        pipe_mgr_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mat_tbl_info_t *tbl_info = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;

  tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  log_entry_add(phase0_tbl, match_spec, act_fn_hdl, action_spec);

  /* It doesn't make sense to add an entry with an action data pointer or
   * selection group since the HW doesn't support it. */
  if (!action_spec->pipe_action_datatype_bmap & PIPE_ACTION_DATA_TYPE) {
    LOG_ERROR(
        "Dev %d tbl %#x, cannot add phase0 table entry with non-action data "
        "action spec",
        dev_tgt.device_id,
        mat_tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  /* Decode the match spec to derive the entry handle. */
  status = phase0_tbl_get_ent_hdl(match_spec, phase0_tbl, mat_ent_hdl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in getting phase0 ent handle for tbl handle 0x%x error: %s",
        __func__,
        mat_tbl_hdl,
        pipe_str_err(status));
    return PIPE_INVALID_ARG;
  }

  /* Also validate the pipe and port passed in the match spec. */
  bf_dev_pipe_t pipe = hdl_to_pipe(phase0_tbl->dev_info, *mat_ent_hdl);
  bf_dev_port_t dev_port = pipe_mgr_phase0_hdl_to_port(*mat_ent_hdl);
  bf_dev_port_t local_port =
      phase0_tbl->dev_info->dev_cfg.dev_port_to_local_port(dev_port);
  if (pipe >= phase0_tbl->dev_info->num_active_pipes) {
    LOG_WARN(
        "Dev %d Table %s (0x%x) cannot add entry for port %d (pipe %d) device "
        "only has %d pipes",
        dev_tgt.device_id,
        phase0_tbl->name,
        mat_tbl_hdl,
        dev_port,
        pipe,
        phase0_tbl->dev_info->num_active_pipes);
    return PIPE_INVALID_ARG;
  }
  if (local_port >= phase0_tbl->dev_info->dev_cfg.num_ports) {
    LOG_WARN(
        "Dev %d Table %s (0x%x) cannot add entry for port %d (pipe %d, local "
        "port %d) device only has %d ports per pipe",
        dev_tgt.device_id,
        phase0_tbl->name,
        mat_tbl_hdl,
        dev_port,
        pipe,
        local_port,
        phase0_tbl->dev_info->dev_cfg.num_ports);
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    /* During warm init, set the symmetricity of the table based on the pipe id
       passed in by the user. The user must pass in the correct pipe id based
       on the symmetricity and the entry being added */
    if (!phase0_tbl->first_entry_replayed) {
      // Indicates that this is the first entry being placed in the table
      phase0_tbl->symmetric =
          dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ? true : false;
      tbl_info->symmetric = phase0_tbl->symmetric;
      phase0_tbl->first_entry_replayed = true;
    }

    status = pipe_mgr_phase0_check_tbl_symm_and_pipe(
        phase0_tbl, dev_tgt.dev_pipe_id, pipe);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for %s phase0 tbl with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          phase0_tbl->symmetric ? "symmetric" : "asymmetric",
          mat_tbl_hdl,
          dev_tgt.device_id);
      return status;
    }
    phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          pipe);
      return PIPE_OBJ_NOT_FOUND;
    }
    pipe_mat_ent_hdl_t ha_entry_hdl = -1;
    status =
        pipe_mgr_hitless_ha_lookup_spec(&phase0_tbl_data->ha_hlp_info->spec_map,
                                        match_spec,
                                        action_spec,
                                        act_fn_hdl,
                                        *mat_ent_hdl,
                                        &ha_entry_hdl,
                                        0 /*ttl*/);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in processing match spec/action spec during API replay "
          "at HLP for tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      return status;
    }

    if (*mat_ent_hdl != ha_entry_hdl) {
      /* This implies that an existing entry was matched to this new
       * entry. Hence set the entry hdl to the one that matched
       */
      *mat_ent_hdl = ha_entry_hdl;
    }
    return PIPE_SUCCESS;
  } else {
    return pipe_mgr_phase0_ent_place_internal(sess_hdl,
                                              dev_tgt,
                                              mat_tbl_hdl,
                                              match_spec,
                                              act_fn_hdl,
                                              action_spec,
                                              mat_ent_hdl,
                                              move_list);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_get_match_spec(
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t *pipe_id,
    pipe_tbl_match_spec_t **match_spec) {
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data;
  pipe_mgr_phase0_tbl_t *phase0_tbl;
  bf_dev_pipe_t pipe;

  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table with handle 0x%x, "
        "device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe = hdl_to_pipe(phase0_tbl->dev_info, entry_hdl);
  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
  if (!phase0_tbl_data) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_mat_data *data = NULL;
  pipe_status_t status = get_ent_data(phase0_tbl_data, entry_hdl, &data);
  if (PIPE_SUCCESS != status) {
    LOG_ERROR(
        "%s: Invalid entry handle %d requested for phase0 table 0x%x on dev "
        "%d",
        __func__,
        entry_hdl,
        mat_tbl_hdl,
        device_id);
    return status;
  }
  *match_spec = unpack_mat_ent_data_ms(data);
  if (phase0_tbl->symmetric) {
    *pipe_id = BF_DEV_PIPE_ALL;
  } else {
    *pipe_id = hdl_to_pipe(phase0_tbl->dev_info, entry_hdl);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_del(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_list) {
  (void)pipe_api_flags;

  pipe_sess_hdl_t sess_hdl = 0;

  sess_hdl = pipe_mgr_get_int_sess_hdl();

  return pipe_mgr_phase0_ent_del(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, move_list);
}

/* Entry delete */
pipe_status_t pipe_mgr_phase0_ent_del(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      pipe_mgr_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table with handle %#x, "
        "device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  bf_dev_pipe_t pipe;
  if (phase0_tbl->symmetric) {
    pipe = BF_DEV_PIPE_ALL;
  } else {
    pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);
  }

  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (is_tbl_data_ent_hdl_default(phase0_tbl_data, mat_ent_hdl)) {
    LOG_ERROR(
        "%s:%d ERROR : Entry delete API called on default entry handle %d "
        "for tbl 0x%x, device id %d, pipe id %d",
        __func__,
        __LINE__,
        mat_ent_hdl,
        mat_tbl_hdl,
        device_id,
        pipe);
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_move_list_t *ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_DEL, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  *move_list = ml;
  ml->entry_hdl = mat_ent_hdl;
  status = rmv_ent_data(phase0_tbl_data, mat_ent_hdl, &ml->data);
  if (PIPE_SUCCESS != status) {
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    status = log_op_for_txn(phase0_tbl_data, ml);
    if (PIPE_SUCCESS != status) {
      save_ent_data(phase0_tbl_data, mat_ent_hdl, ml->data);
      PIPE_MGR_FREE(ml);
      *move_list = NULL;
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  LOG_TRACE("%s : Deleting entry handle %#x in tbl %#x on dev %d",
            __func__,
            mat_ent_hdl,
            mat_tbl_hdl,
            device_id);

  pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);
  phase0_tbl->num_entries_placed[pipe]--;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_ha_reconcile_ent_modify(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *action_spec,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_list) {
  (void)pipe_api_flags;
  pipe_sess_hdl_t sess_hdl = 0;

  sess_hdl = pipe_mgr_get_int_sess_hdl();

  return pipe_mgr_phase0_ent_set_action(sess_hdl,
                                        device_id,
                                        mat_tbl_hdl,
                                        mat_ent_hdl,
                                        act_fn_hdl,
                                        action_spec,
                                        move_list);
}

/* Set action */
pipe_status_t pipe_mgr_phase0_ent_set_action(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             pipe_action_spec_t *action_spec,
                                             pipe_mgr_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mat_ent_hdl_t ent_hdl;
  bf_dev_pipe_t pipe;
  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s:Could not find the phase0 table info for table "
        "handle %#x",
        __func__,
        mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  log_entry_mod(phase0_tbl, mat_ent_hdl, act_fn_hdl, action_spec);

  /* It doesn't make sense to add an entry with an action data pointer or
   * selection group since the HW doesn't support it. */
  if (!action_spec->pipe_action_datatype_bmap & PIPE_ACTION_DATA_TYPE) {
    LOG_ERROR(
        "Dev %d tbl %#x entry handle %#x, cannot modify phase0 table entry "
        "with non-action data action spec",
        device_id,
        mat_tbl_hdl,
        mat_ent_hdl);
    return PIPE_INVALID_ARG;
  }
  if (phase0_tbl->symmetric)
    pipe = BF_DEV_PIPE_ALL;
  else
    pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);

  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe);
    return PIPE_OBJ_NOT_FOUND;
  }
  bool is_txn = pipe_mgr_sess_in_txn(sess_hdl);
  if (phase0_tbl_data->default_entry_hdl == mat_ent_hdl) {
    dev_target_t dev_tgt;
    dev_tgt.device_id = device_id;
    dev_tgt.dev_pipe_id = phase0_tbl_data->pipe_id;
    return pipe_mgr_phase0_default_ent_place(dev_tgt,
                                             mat_tbl_hdl,
                                             act_fn_hdl,
                                             action_spec,
                                             is_txn,
                                             &ent_hdl,
                                             move_list);
  }

  /* Look up saved entry data. */
  struct pipe_mgr_mat_data *old_data = NULL;
  status = rmv_ent_data(phase0_tbl_data, mat_ent_hdl, &old_data);
  if (PIPE_SUCCESS != status) {
    *move_list = NULL;
    return PIPE_INVALID_ARG;
  }

  /* Allocate and fill a move list entry with the placement decision. */
  pipe_mgr_move_list_t *ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOD, pipe);
  if (!ml) {
    save_ent_data(phase0_tbl_data, mat_ent_hdl, old_data);
    return PIPE_NO_SYS_RESOURCES;
  }

  *move_list = ml;
  ml->entry_hdl = mat_ent_hdl;
  ml->data = make_mat_ent_data(
      unpack_mat_ent_data_ms(old_data), action_spec, act_fn_hdl, 0, 0, 0, 0);
  if (!ml->data) {
    save_ent_data(phase0_tbl_data, mat_ent_hdl, old_data);
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_NO_SYS_RESOURCES;
  }
  ml->old_data = old_data;

  /* Save the new entry data. */
  status = save_ent_data(phase0_tbl_data, mat_ent_hdl, ml->data);
  if (PIPE_SUCCESS != status) {
    save_ent_data(phase0_tbl_data, mat_ent_hdl, old_data);
    free_mat_ent_data(ml->data);
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_UNEXPECTED;
  }

  if (is_txn) {
    status = log_op_for_txn(phase0_tbl_data, ml);
    if (PIPE_SUCCESS != status) {
      rmv_ent_data(phase0_tbl_data, mat_ent_hdl, &ml->data);
      save_ent_data(phase0_tbl_data, mat_ent_hdl, old_data);
      free_mat_ent_data(ml->data);
      PIPE_MGR_FREE(ml);
      *move_list = NULL;
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  return PIPE_SUCCESS;
}

/* Program all unoccupied table entries in the scope with the default action. */
static pipe_status_t phase0_tbl_program_default(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_move_list_t *move_list,
    bool is_clear) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_action_spec_t *action_spec = NULL, *tmp_aspec;
  bf_dev_pipe_t pipe = get_move_list_pipe(move_list);
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data =
      phase0_tbl_get_instance(phase0_tbl, pipe);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!is_clear) {
    action_spec = unpack_mat_ent_data_as(move_list->data);
    if (!action_spec) {
      LOG_ERROR("Dev %d Tbl %s 0x%x, no action spec provided to %s",
                phase0_tbl->dev_id,
                phase0_tbl->name,
                phase0_tbl->mat_tbl_hdl,
                __func__);
      return PIPE_INVALID_ARG;
    }

    /* Make a copy of the action spec to be saved against the table. */
    tmp_aspec = pipe_mgr_tbl_copy_action_spec(NULL, action_spec);
    if (!tmp_aspec) {
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  /* Go over all entries in the table, if the entry has not been programmed,
   * program it with the default action data. */
  uint32_t num_pipes = phase0_tbl->dev_info->num_active_pipes;
  uint8_t num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;
  /* Iterate through all the pipes in the scope. */
  for (unsigned int i = 0; i < num_pipes; i++) {
    if (!(phase0_tbl_data->scope_pipe_bmp & (1u << i))) continue;
    /* Iterate through all the ports. */
    for (unsigned int j = 0; j < num_ports; j++) {
      bf_dev_port_t dev_port =
          phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      pipe_mat_ent_hdl_t mat_ent_hdl =
          phase0_tbl_dev_port_to_entry_handle(dev_port);
      bool configure = true;
      if (phase0_tbl_installed_get(phase0_tbl, phase0_tbl_data, mat_ent_hdl))
        continue;
      /* If no match entry exists at this index, set the default data here */
      sts = phase0_tbl_program_check_prsr(phase0_tbl, mat_ent_hdl, &configure);
      if (sts != PIPE_SUCCESS) goto err_cleanup;
      sts = phase0_tbl_program(sess_hdl,
                               action_spec,
                               phase0_tbl,
                               mat_ent_hdl,
                               true,
                               false,
                               is_clear,
                               configure);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Failed to set default Phase0 entry for handle 0x%x "
            "tbl %s 0x%x, %s",
            __func__,
            __LINE__,
            mat_ent_hdl,
            phase0_tbl->name,
            phase0_tbl->mat_tbl_hdl,
            pipe_str_err(sts));
        goto err_cleanup;
      }
    }
  }

err_cleanup:

  if (sts != PIPE_SUCCESS) {
    if (!is_clear) pipe_mgr_tbl_destroy_action_spec(&tmp_aspec);
    return sts;
  }

  /* Save the default info for future entry delete operations. */
  if (phase0_tbl_data->dflt_act_spec)
    pipe_mgr_tbl_destroy_action_spec(&phase0_tbl_data->dflt_act_spec);
  if (is_clear)
    phase0_tbl_data->dflt_act_spec = NULL;
  else
    phase0_tbl_data->dflt_act_spec = tmp_aspec;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_ent_program(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mgr_move_list_t *move_list,
                                          uint32_t *processed) {
  *processed = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  bool configure = true;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!move_list) {
    return PIPE_SUCCESS;
  }

  for (; move_list; move_list = move_list->next) {
    bf_dev_pipe_t pipe =
        hdl_to_pipe(phase0_tbl->dev_info, move_list->entry_hdl);
    if (PIPE_MAT_UPDATE_ADD == move_list->op) {
      pipe_action_spec_t *as = unpack_mat_ent_data_as(move_list->data);
      rc = phase0_tbl_program_check_prsr(
          phase0_tbl, move_list->entry_hdl, &configure);
      if (rc != PIPE_SUCCESS) return rc;
      rc = phase0_tbl_program(sess_hdl,
                              as,
                              phase0_tbl,
                              move_list->entry_hdl,
                              false,
                              false,
                              false,
                              configure);
      if (rc == PIPE_SUCCESS) {
        phase0_tbl->num_entries_programmed[pipe]++;
      }
    } else if (PIPE_MAT_UPDATE_MOD == move_list->op &&
               phase0_tbl_is_ent_hdl_default(phase0_tbl,
                                             move_list->entry_hdl)) {
      /* This is a set default operation. */
      rc = phase0_tbl_program_default(sess_hdl, phase0_tbl, move_list, false);
    } else if (PIPE_MAT_UPDATE_MOD == move_list->op) {
      pipe_action_spec_t *as = unpack_mat_ent_data_as(move_list->data);
      rc = phase0_tbl_program_check_prsr(
          phase0_tbl, move_list->entry_hdl, &configure);
      if (rc != PIPE_SUCCESS) return rc;
      rc = phase0_tbl_program(sess_hdl,
                              as,
                              phase0_tbl,
                              move_list->entry_hdl,
                              false,
                              false,
                              false,
                              configure);
    } else if (PIPE_MAT_UPDATE_DEL == move_list->op) {
      rc = phase0_tbl_program_check_prsr(
          phase0_tbl, move_list->entry_hdl, &configure);
      if (rc != PIPE_SUCCESS) return rc;
      rc = phase0_tbl_program(sess_hdl,
                              NULL,
                              phase0_tbl,
                              move_list->entry_hdl,
                              false,
                              true,
                              false,
                              configure);
      if (rc == PIPE_SUCCESS) {
        phase0_tbl->num_entries_programmed[pipe]--;
      }
    } else if (PIPE_MAT_UPDATE_SET_DFLT == move_list->op) {
      rc = phase0_tbl_program_default(sess_hdl, phase0_tbl, move_list, false);
    } else if (PIPE_MAT_UPDATE_CLR_DFLT == move_list->op) {
      rc = phase0_tbl_program_default(sess_hdl, phase0_tbl, move_list, true);
    } else {
      LOG_ERROR("Unexpected op %d, %s", move_list->op, __func__);
      return PIPE_INVALID_ARG;
    }

    if (PIPE_SUCCESS != rc) return rc;
    ++*processed;
    if (pipe_mgr_sess_in_batch(sess_hdl)) {
      pipe_mgr_drv_ilist_chkpt(sess_hdl);
    }
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_phase0_print_match_spec(bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_tbl_match_spec_t *match_spec,
                                      char *buf,
                                      size_t buf_size) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  size_t bytes_written = 0;

  if (!buf || buf_size == 0) {
    return;
  }
  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 table info for table with"
        " handle %#x for device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return;
  }

  pipe_mgr_entry_format_print_match_spec(device_id,
                                         phase0_tbl->profile_id,
                                         mat_tbl_hdl,
                                         match_spec,
                                         buf,
                                         buf_size,
                                         &bytes_written);

  return;
}

pipe_status_t pipe_mgr_phase0_get_first_entry_handle(pipe_mat_tbl_hdl_t tbl_hdl,
                                                     dev_target_t dev_tgt,
                                                     int *entry_hdl) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  struct pipe_mgr_mat_data *data = NULL;
  bool first_entry_found = false;
  unsigned long hdl = 0;
  bf_map_sts_t msts;
  uint32_t i;

  *entry_hdl = -1;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (phase0_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < phase0_tbl->num_tbls; i++) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[i];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != phase0_tbl_data->pipe_id)
      continue;

    msts = bf_map_get_first(&phase0_tbl_data->ent_data, &hdl, (void **)&data);
    if (msts != BF_MAP_OK) {
      *entry_hdl = -1;
      if (dev_tgt.dev_pipe_id == phase0_tbl_data->pipe_id) {
        /* No entry in the requested pipe_id */
        break;
      } else {
        /* Keep looking in other table instances */
        continue;
      }
    }

    while (msts == BF_MAP_OK) {
      *entry_hdl = hdl;
      if (!is_tbl_data_ent_hdl_default(phase0_tbl_data, hdl)) {
        first_entry_found = true;
        break;
      } else {
        msts =
            bf_map_get_next(&phase0_tbl_data->ent_data, &hdl, (void **)&data);
      }
    }

    if (first_entry_found) break;

    *entry_hdl = -1;
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_get_next_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *first_tbl_data;
  pipe_mgr_phase0_tbl_data_t *cur_tbl_data;
  struct pipe_mgr_mat_data *data = NULL;
  uint32_t first_tbl_idx, idx, t;
  unsigned long hdl = 0;
  bf_map_sts_t msts;
  bool done = false;
  int i = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Start with the entry's table instance. */
  first_tbl_data = phase0_tbl_get_instance_from_entry(
      phase0_tbl, entry_hdl, __func__, __LINE__);
  if (!first_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Determine the table instance index of this first table. */
  for (t = 0; t < phase0_tbl->num_tbls; t++) {
    if (first_tbl_data == &phase0_tbl->phase0_tbl_data[t]) {
      first_tbl_idx = t;
      break;
    }
  }

  /* Sanity check. */
  if (t >= phase0_tbl->num_tbls) {
    LOG_ERROR("%s:%d Unexpected error Dev %d Phase0 tbl %s 0x%x entry hdl %d",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              entry_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  hdl = entry_hdl;
  for (idx = first_tbl_idx; idx < phase0_tbl->num_tbls && i < n && !done;
       idx++) {
    cur_tbl_data = &phase0_tbl->phase0_tbl_data[idx];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != cur_tbl_data->pipe_id)
      continue;

    if (cur_tbl_data != first_tbl_data) {
      /* Look in next table instance. */
      msts = bf_map_get_first(&cur_tbl_data->ent_data, &hdl, (void **)&data);
      if (msts == BF_MAP_OK) {
        if (!is_tbl_data_ent_hdl_default(cur_tbl_data, hdl)) {
          /* Entry present */
          next_entry_handles[i++] = hdl;
        }
      } else {
        /* Keep looking in other table instances. */
        continue;
      }
    }

    while (i < n) {
      msts = bf_map_get_next(&cur_tbl_data->ent_data, &hdl, (void **)&data);
      if (BF_MAP_OK != msts) {
        next_entry_handles[i] = -1;
        if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
          /* There is no more entry for the requested pipe. */
          done = true;
        }
        break;
      }
      if (is_tbl_data_ent_hdl_default(cur_tbl_data, hdl)) continue;
      next_entry_handles[i] = hdl;
      i++;
    }
  }

  if (i < n) next_entry_handles[i] = -1;

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_get_entry(pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_ent_hdl_t entry_hdl,
                                        pipe_tbl_match_spec_t *pipe_match_spec,
                                        pipe_action_spec_t *pipe_action_spec,
                                        pipe_act_fn_hdl_t *act_fn_hdl,
                                        bool from_hw) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_status_t status;
  size_t bytes_written = 0;
  uint8_t pipe_id = 0;
  dev_target_t dev_tgt;

  phase0_tbl = pipe_mgr_phase0_tbl_get(device_id, mat_tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s:%d Requesting an entry from a non-existent table with handle %x"
        " device id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_id = hdl_to_pipe(phase0_tbl->dev_info, entry_hdl);
  phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (from_hw && phase0_tbl->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    // Since Tofino-3 phase0 memory is write only fallback to a from-sw read
    LOG_WARN(
        "%s:%d Requested read from HW not supported, defaulting to SW read.",
        __func__,
        __LINE__);
    from_hw = false;
  }
  if (phase0_tbl_data->default_entry_hdl == entry_hdl) {
    pipe_tbl_match_spec_t match_spec = {0};
    dev_tgt.device_id = device_id;
    dev_tgt.dev_pipe_id = phase0_tbl_data->pipe_id;
    status = pipe_mgr_phase0_default_ent_get(
        dev_tgt, mat_tbl_hdl, pipe_action_spec, act_fn_hdl, from_hw);
    if (status != PIPE_SUCCESS) return status;
    if (!pipe_mgr_tbl_copy_match_spec(pipe_match_spec, &match_spec))
      return PIPE_NO_SYS_RESOURCES;
    return PIPE_SUCCESS;
  }
  if (!from_hw) {
    struct pipe_mgr_mat_data *data = NULL;
    status = get_ent_data(phase0_tbl_data, entry_hdl, &data);
    if (PIPE_SUCCESS != status) {
      return status;
    }

    if (!pipe_mgr_tbl_copy_match_spec(pipe_match_spec,
                                      unpack_mat_ent_data_ms(data))) {
      return PIPE_NO_SYS_RESOURCES;
    }
    if (!pipe_mgr_tbl_copy_action_spec(pipe_action_spec,
                                       unpack_mat_ent_data_as(data))) {
      return PIPE_NO_SYS_RESOURCES;
    }
    *act_fn_hdl = unpack_mat_ent_data_afun_hdl(data);
    return PIPE_SUCCESS;
  } else {
    pipe_sess_hdl_t sess_hdl = 0;
    sess_hdl = pipe_mgr_get_int_sess_hdl();

    /* Derive the port from the entry handle. */
    bf_dev_port_t port = pipe_mgr_phase0_hdl_to_port(entry_hdl);
    if (!phase0_tbl->dev_info->dev_cfg.dev_port_validate(port)) {
      LOG_ERROR("Invalid entry handle 0x%x for phase0 table on dev %d",
                entry_hdl,
                device_id);
      return PIPE_INVALID_ARG;
    }
    if (phase0_tbl->dev_info->dev_cfg.dev_port_to_pipe(port) >=
        phase0_tbl->dev_info->num_active_pipes) {
      LOG_ERROR("Invalid entry handle 0x%x for phase0 table on dev %d",
                entry_hdl,
                device_id);
      return PIPE_INVALID_ARG;
    }

    /* Setup an array to hold the register values. */
    uint32_t p0_width = phase0_tbl->dev_info->dev_cfg.p0_width;
    pipe_register_spec_t reg_spec_list[p0_width];
    pipe_memory_spec_t mem_spec_list;
    PIPE_MGR_MEMSET(reg_spec_list, 0, sizeof(reg_spec_list));
    PIPE_MGR_MEMSET(&mem_spec_list, 0, sizeof(mem_spec_list));

    /* Get the register addresses. */
    bf_subdev_id_t subdev;
    status = pipe_mgr_entry_format_phase0_tbl_addr_get(phase0_tbl->dev_info,
                                                       port,
                                                       false,
                                                       &subdev,
                                                       reg_spec_list,
                                                       &mem_spec_list);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("%s: Error getting phase0 tbl 0x%x for port %d dev %d",
                __func__,
                mat_tbl_hdl,
                port,
                device_id);
      return status;
    }

    /* Read the table data. */
    switch (phase0_tbl->dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
      case BF_DEV_FAMILY_TOFINO2: {
        for (unsigned int r = 0; r < p0_width; r++) {
          status = pipe_mgr_drv_reg_rd(&sess_hdl,
                                       device_id,
                                       reg_spec_list[r].reg_addr,
                                       &reg_spec_list[r].reg_data);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR("%s: Error reading phase0 tbl 0x%x for port %d dev %d",
                      __func__,
                      mat_tbl_hdl,
                      port,
                      device_id);
            return status;
          }
        }
        break;
      }
      case BF_DEV_FAMILY_TOFINO3: {
        uint64_t data_hi = 0, data_lo = 0;
        lld_subdev_ind_read(
            device_id, subdev, mem_spec_list.mem_addr0, &data_hi, &data_lo);
        reg_spec_list[0].reg_data = data_lo & 0xffffffff;
        reg_spec_list[1].reg_data = (data_lo >> 32) & 0xffffffff;
        reg_spec_list[2].reg_data = data_hi & 0xffffffff;
        reg_spec_list[3].reg_data = (data_hi >> 32) & 0xffffffff;

        break;
      }

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
        break;
    }

    /* Decode into match and action specs. */
    pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    if (!phase0_tbl->num_actions || !phase0_tbl->act_fn_hdl_info) {
      LOG_ERROR("No actions known for phase0 table 0x%x on dev %d",
                mat_tbl_hdl,
                device_id);
      return PIPE_UNEXPECTED;
    }
    if (pipe_action_spec->act_data.num_action_data_bytes <
        phase0_tbl->act_fn_hdl_info->num_bytes) {
      LOG_ERROR(
          "Action spec provided cannot store all action data for phase0 table "
          "0x%x on dev %d.  Has %d bytes, needs %d bytes",
          mat_tbl_hdl,
          device_id,
          pipe_action_spec->act_data.num_action_data_bytes,
          phase0_tbl->act_fn_hdl_info->num_bytes);
    }
    *act_fn_hdl = phase0_tbl->act_fn_hdl_info->act_fn_hdl;
    pipe_action_spec->act_data.num_valid_action_data_bits =
        phase0_tbl->act_fn_hdl_info->num_bits;
    status = pipe_mgr_entry_format_phase0_tbl_decode_reg_to_action_data(
        phase0_tbl->dev_info,
        phase0_tbl->profile_id,
        mat_tbl_hdl,
        reg_spec_list,
        &pipe_action_spec->act_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "Error decoding phase0 table 0x%x action data for port %d on dev %d, "
          "%s",
          mat_tbl_hdl,
          port,
          device_id,
          pipe_str_err(status));
      return status;
    }

    if (pipe_match_spec) {
      status = pipe_mgr_entry_format_tof_phase0_tbl_get_match_spec(
          device_id,
          phase0_tbl->profile_id,
          mat_tbl_hdl,
          port,
          pipe_match_spec);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "Error building phase0 table 0x%x match spec for port %d on dev "
            "%d, %s",
            mat_tbl_hdl,
            port,
            device_id,
            pipe_str_err(status));
        return status;
      }

      char buf[1000];
      pipe_mgr_entry_format_print_match_spec(phase0_tbl->dev_id,
                                             phase0_tbl->profile_id,
                                             phase0_tbl->mat_tbl_hdl,
                                             pipe_match_spec,
                                             buf,
                                             sizeof(buf),
                                             &bytes_written);
      LOG_TRACE("%s Request to get phase0 entry on dev %d with match spec\n%s",
                phase0_tbl->name,
                phase0_tbl->dev_id,
                buf);
    }

    char buf1[1000];
    bytes_written = 0;
    if (pipe_action_spec->pipe_action_datatype_bmap & PIPE_ACTION_DATA_TYPE) {
      pipe_mgr_entry_format_print_action_spec(phase0_tbl->dev_id,
                                              phase0_tbl->profile_id,
                                              &pipe_action_spec->act_data,
                                              *act_fn_hdl,
                                              buf1,
                                              sizeof(buf1),
                                              &bytes_written);
      LOG_TRACE("%s : phase0 entry Action spec \n%s", phase0_tbl->name, buf1);
    } else if (pipe_action_spec->pipe_action_datatype_bmap &
               PIPE_ACTION_DATA_HDL_TYPE) {
      LOG_TRACE("%s : phase0 entry action entry handle %d\n",
                phase0_tbl->name,
                pipe_action_spec->adt_ent_hdl);
    } else if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
      LOG_TRACE("%s : phase0 entry sel grp handle %#x\n",
                phase0_tbl->name,
                pipe_action_spec->sel_grp_hdl);
    }
    return PIPE_SUCCESS;
  }
}

pipe_status_t pipe_mgr_phase0_get_placed_entry_count(dev_target_t dev_tgt,
                                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                                     uint32_t *count) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  uint32_t pipe_id;
  uint32_t phase0_count = 0;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Requesting entry count from a non-existent table with handle %#x"
        " device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (phase0_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          dev_tgt.dev_pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  for (pipe_id = 0; pipe_id < phase0_tbl->dev_info->num_active_pipes;
       pipe_id++) {
    if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) ||
        ((1u << pipe_id) & phase0_tbl_data->scope_pipe_bmp)) {
      phase0_count += phase0_tbl->num_entries_placed[pipe_id];
    }
  }
  *count = phase0_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, uint32_t *count) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  uint32_t pipe_id;
  uint32_t phase0_count = 0;

  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Requesting entry count from a non-existent table with handle %#x"
        " device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (phase0_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric phase0 tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          phase0_tbl->dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          dev_tgt.dev_pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  for (pipe_id = 0; pipe_id < phase0_tbl->dev_info->num_active_pipes;
       pipe_id++) {
    if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) ||
        ((1u << pipe_id) & phase0_tbl_data->scope_pipe_bmp)) {
      phase0_count += phase0_tbl->num_entries_programmed[pipe_id];
    }
  }
  *count = phase0_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_log_state(bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        cJSON *match_tbls) {
  bf_map_sts_t st;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  unsigned long entry_hdl;
  struct pipe_mgr_mat_data *data = NULL;
  cJSON *match_tbl, *mat_ents, *mat_ent, *def_ent, *pipe_tbls, *pipe_tbl;
  uint32_t idx;
  pipe_status_t status;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Requesting an entry from a non-existent table with handle 0x%x"
        " device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  cJSON_AddItemToArray(match_tbls, match_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(match_tbl, "name", phase0_tbl->name);
  cJSON_AddNumberToObject(match_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(match_tbl, "symmetric", phase0_tbl->symmetric);

  cJSON_AddItemToObject(
      match_tbl, "pipe_tbls", pipe_tbls = cJSON_CreateArray());

  for (idx = 0; idx < phase0_tbl->num_tbls; idx++) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[idx];
    cJSON_AddItemToArray(pipe_tbls, pipe_tbl = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe_tbl, "pipe_id", phase0_tbl_data->pipe_id);
    cJSON_AddItemToObject(
        pipe_tbl, "default_entry", def_ent = cJSON_CreateObject());
    status = get_ent_data(
        phase0_tbl_data, phase0_tbl_data->default_entry_hdl, &data);
    if (status != PIPE_SUCCESS) continue;
    cJSON_AddNumberToObject(
        def_ent, "entry_hdl", phase0_tbl_data->default_entry_hdl);
    pipe_mgr_tbl_log_specs(
        dev_id, phase0_tbl->profile_id, tbl_hdl, data, def_ent, true);
  }

  cJSON_AddItemToObject(
      match_tbl, "match_entries", mat_ents = cJSON_CreateArray());
  /* Walk through all table instances */
  for (idx = 0; idx < phase0_tbl->num_tbls; idx++) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[idx];
    st = bf_map_get_first(
        &phase0_tbl_data->ent_data, &entry_hdl, (void **)&data);
    while (st == BF_MAP_OK) {
      if (!is_tbl_data_ent_hdl_default(phase0_tbl_data, entry_hdl)) {
        cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
        cJSON_AddNumberToObject(mat_ent, "entry_hdl", entry_hdl);
        pipe_mgr_tbl_log_specs(
            dev_id, phase0_tbl->profile_id, tbl_hdl, data, mat_ent, false);
      }
      st = bf_map_get_next(
          &phase0_tbl_data->ent_data, &entry_hdl, (void **)&data);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_restore_state(bf_dev_id_t dev_id,
                                            cJSON *match_tbl) {
  pipe_status_t sts;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mgr_move_list_t *move_node;
  pipe_tbl_match_spec_t ms = {0};
  pipe_action_spec_t as = {0};
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t ent_hdl;
  bool symmetric;
  uint32_t processed = 0;
  cJSON *mat_ents, *mat_ent, *def_ent, *pipe_tbls, *pipe_tbl;
  uint32_t idx;

  tbl_hdl = cJSON_GetObjectItem(match_tbl, "handle")->valueint;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  symmetric = (cJSON_GetObjectItem(match_tbl, "symmetric")->type == cJSON_True);
  phase0_tbl->symmetric = symmetric;

  pipe_tbls = cJSON_GetObjectItem(match_tbl, "pipe_tbls");
  for (pipe_tbl = pipe_tbls->child, idx = 0; pipe_tbl;
       pipe_tbl = pipe_tbl->next, idx++) {
    phase0_tbl_data = &phase0_tbl->phase0_tbl_data[idx];
    dev_tgt.dev_pipe_id =
        (uint32_t)cJSON_GetObjectItem(pipe_tbl, "pipe_id")->valueint;
    PIPE_MGR_DBGCHK(phase0_tbl_data->pipe_id == dev_tgt.dev_pipe_id);

    def_ent = cJSON_GetObjectItem(pipe_tbl, "default_entry");
    if (!cJSON_GetObjectItem(def_ent, "entry_hdl")) continue;
    ent_hdl = cJSON_GetObjectItem(def_ent, "entry_hdl")->valuedouble;
    if (!cJSON_GetObjectItem(def_ent, "act_spec")) continue;
    pipe_mgr_tbl_restore_specs(dev_id,
                               phase0_tbl->profile_id,
                               tbl_hdl,
                               def_ent,
                               &ms,
                               &as,
                               &act_fn_hdl);

    sts = pipe_mgr_phase0_default_ent_place(dev_tgt,
                                            tbl_hdl,
                                            act_fn_hdl,
                                            &as,
                                            pipe_mgr_sess_in_txn(sess_hdl),
                                            &ent_hdl,
                                            &move_node);
    PIPE_MGR_FREE(as.act_data.action_data_bits);
    PIPE_MGR_DBGCHK(ent_hdl ==
                    cJSON_GetObjectItem(def_ent, "entry_hdl")->valuedouble);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error restoring phase0 default ent handle %u for tbl "
          " handle 0x%x  error: %s",
          __func__,
          ent_hdl,
          tbl_hdl,
          pipe_str_err(sts));
      return sts;
    }

    sts = pipe_mgr_phase0_ent_program(
        sess_hdl, dev_id, tbl_hdl, move_node, &processed);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error programming phase0 default ent handle %u for tbl "
          " handle 0x%x  error: %s",
          __func__,
          ent_hdl,
          tbl_hdl,
          pipe_str_err(sts));
      return sts;
    }
  }
  mat_ents = cJSON_GetObjectItem(match_tbl, "match_entries");
  for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
    ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;
    pipe_mgr_tbl_restore_specs(dev_id,
                               phase0_tbl->profile_id,
                               tbl_hdl,
                               mat_ent,
                               &ms,
                               &as,
                               &act_fn_hdl);
    if (!symmetric) {
      dev_tgt.dev_pipe_id = hdl_to_pipe(phase0_tbl->dev_info, ent_hdl);
    }

    sts = pipe_mgr_phase0_ent_place(
        sess_hdl, dev_tgt, tbl_hdl, &ms, act_fn_hdl, &as, &ent_hdl, &move_node);
    if (ms.match_value_bits) PIPE_MGR_FREE(ms.match_value_bits);
    if (ms.match_mask_bits) PIPE_MGR_FREE(ms.match_mask_bits);
    if (as.act_data.action_data_bits)
      PIPE_MGR_FREE(as.act_data.action_data_bits);
    PIPE_MGR_DBGCHK(ent_hdl ==
                    cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error restoring phase0 ent handle %u for tbl "
          " handle 0x%x  error: %s",
          __func__,
          ent_hdl,
          tbl_hdl,
          pipe_str_err(sts));
      return sts;
    }

    sts = pipe_mgr_phase0_ent_program(
        sess_hdl, dev_id, tbl_hdl, move_node, &processed);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error programming phase0 ent handle %u for tbl "
          " handle 0x%x  error: %s",
          __func__,
          ent_hdl,
          tbl_hdl,
          pipe_str_err(sts));
      return sts;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t program_one_table(pipe_sess_hdl_t shdl,
                                       pipe_mgr_phase0_tbl_t *tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = tbl->dev_info;
  int num_pipes = tbl->dev_info->num_active_pipes;
  int p0_width = tbl->dev_info->dev_cfg.p0_width;
  uint64_t prsr_map;
  uint8_t parser_id;
  pipe_register_spec_t reg_spec_list[p0_width];
  pipe_memory_spec_t mem_spec_list;
  PIPE_MGR_MEMSET(&mem_spec_list, 0, sizeof(mem_spec_list));
  /* Loop over all ports and check the shadow for entries that need to be
   * programmed. */
  for (int log_pipe = 0; log_pipe < num_pipes; ++log_pipe) {
    /* Skip pipes that this table does not belong to. */
    if (!tbl->log_pipe_shadow[log_pipe]) continue;

    pipe_bitmap_t pbm;
    PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pbm, log_pipe);
    sts = pipe_mgr_prsr_instance_get_profile(
        tbl->dev_id, log_pipe, 0, tbl->prsr_instance_hdl, &prsr_map);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in getting parser map with parser instance handler %#x, "
          "dev %d, error %s",
          __func__,
          tbl->prsr_instance_hdl,
          tbl->dev_id,
          pipe_str_err(sts));
      return sts;
    }
    for (int local_port = 0; local_port < tbl->dev_info->dev_cfg.num_ports;
         ++local_port) {
      int shadow_offset = local_port * 4 * p0_width;
      bool non_zero = false;
      // check if cfg to hw
      switch (tbl->dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          parser_id = local_port / TOF_NUM_CHN_PER_PORT;
          break;
        case BF_DEV_FAMILY_TOFINO2:
          parser_id = local_port / TOF2_NUM_CHN_PER_IPB;
          break;
        case BF_DEV_FAMILY_TOFINO3:
          parser_id = local_port / TOF3_NUM_CHN_PER_IPB;
          break;
        default:
          LOG_ERROR("%s : Invalid dev family dev %d ", __func__, tbl->dev_id);
          return PIPE_UNEXPECTED;
      }
      if (((1u << parser_id) & prsr_map) == 0)
        continue;  // not apply to hw, ignore for now.
      /* Grab the data from the shadow incase it needs to be written.  Also
       * check if any of it is non-zero, if so it must be written. */
      for (int i = 0, j = 0; i < p0_width; ++i) {
        uint32_t x = tbl->log_pipe_shadow[log_pipe][shadow_offset + j + 3];
        x = (x << 8) | tbl->log_pipe_shadow[log_pipe][shadow_offset + j + 2];
        x = (x << 8) | tbl->log_pipe_shadow[log_pipe][shadow_offset + j + 1];
        x = (x << 8) | tbl->log_pipe_shadow[log_pipe][shadow_offset + j + 0];
        j += 4;
        reg_spec_list[i].reg_data = x;
        non_zero = non_zero || !!x;
      }

      /* In TF3 case all used ports memory must be initialized. */
      if (!non_zero && dev_info->dev_family != BF_DEV_FAMILY_TOFINO3) continue;

      /* Get the register addresses since we need to do a write. */
      int dev_port = tbl->dev_info->dev_cfg.make_dev_port(log_pipe, local_port);
      bf_subdev_id_t subdev;
      sts = pipe_mgr_entry_format_phase0_tbl_addr_get(tbl->dev_info,
                                                      dev_port,
                                                      true,
                                                      &subdev,
                                                      reg_spec_list,
                                                      &mem_spec_list);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR(
            "Failed to get P0 reg addrs, dev %d log-pipe %d port %d, sts %s",
            tbl->dev_id,
            log_pipe,
            local_port,
            pipe_str_err(sts));
        return sts;
      }

      switch (tbl->dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
        case BF_DEV_FAMILY_TOFINO2: {
          uint32_t stage = tbl->dev_info->dev_cfg.pcie_pipe_addr_get_stage(
              reg_spec_list[0].reg_addr);
          /* Post the instructions to update the registers. */
          for (int word = 0; word < p0_width; ++word) {
            pipe_instr_write_reg_t instr;
            construct_instr_reg_write(tbl->dev_id,
                                      &instr,
                                      reg_spec_list[word].reg_addr,
                                      reg_spec_list[word].reg_data);
            sts = pipe_mgr_drv_ilist_add(&shdl,
                                         tbl->dev_info,
                                         &pbm,
                                         stage,
                                         (uint8_t *)&instr,
                                         sizeof instr);
            if (PIPE_SUCCESS != sts) {
              LOG_ERROR(
                  "Failed to post instr for P0 data, dev %d log-pipe %d port "
                  "%d, "
                  "sts %s",
                  tbl->dev_id,
                  log_pipe,
                  local_port,
                  pipe_str_err(sts));
              return sts;
            }
          }
          break;
        }
        case BF_DEV_FAMILY_TOFINO3: {
          /* In TF3 case, there are 2 tables that must be initialized since
           * both are RAM memory, not CSR that is initialized to 0 on reset.
           * Both tables are addressed back to back and separated by 0x80
           * offset. Only difference is field length 16 vs 8 bytes.
           * Please refer to
           * tof3_mem.pipes.parde.i_prsr_mem.ipb_mem.meta_phase0_16byte and
           * tof3_mem.pipes.parde.i_prsr_mem.ipb_mem.meta_phase0_8byte_ver
           * for details on addressing. */
          uint8_t byte_8[8] = {0};
          for (uint32_t offset = 0; offset <= 0x80; offset += 0x80) {
            int datasz = (offset) ? 8 : 16;
            uint8_t *data =
                (offset) ? byte_8
                         : &tbl->log_pipe_shadow[log_pipe][shadow_offset];
            pipe_instr_set_memdata_i_only_t instr;
            /* Program IPB0.
             * It can be used for substitute of any ingress port,
             * when pkt gen injects a packet, so it must be programmed for
             * every entry/port. */
            uint64_t addr = mem_spec_list.mem_addr0 + offset;
            uint32_t lower_addr = addr & 0xFFFFFFFF;
            uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr);
            construct_instr_set_memdata_by_addr(
                dev_info, &instr, datasz, lower_addr);
            sts = pipe_mgr_drv_ilist_add_2(&shdl,
                                           dev_info,
                                           &pbm,
                                           stage,
                                           (uint8_t *)&instr,
                                           sizeof(instr),
                                           data,
                                           datasz);
            if (PIPE_SUCCESS != sts) {
              LOG_ERROR(
                  "Failed to post ilist, dev %d log-pipe %d port %d, sts %s",
                  tbl->dev_id,
                  log_pipe,
                  local_port,
                  pipe_str_err(sts));
              return sts;
            }

            /* Program IPB for the local-port. */
            addr = mem_spec_list.mem_addr1 + offset;
            lower_addr = addr & 0xFFFFFFFF;
            stage = dev_info->dev_cfg.stage_id_from_addr(addr);
            construct_instr_set_memdata_by_addr(
                dev_info, &instr, datasz, lower_addr);
            sts = pipe_mgr_drv_ilist_add_2(&shdl,
                                           dev_info,
                                           &pbm,
                                           stage,
                                           (uint8_t *)&instr,
                                           sizeof(instr),
                                           data,
                                           datasz);
            if (PIPE_SUCCESS != sts) {
              LOG_ERROR(
                  "Failed to post ilist, dev %d log-pipe %d port %d, sts %s",
                  tbl->dev_id,
                  log_pipe,
                  local_port,
                  pipe_str_err(sts));
              return sts;
            }
          }
          break;
        }

        case BF_DEV_FAMILY_UNKNOWN:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
      }
    }

    switch (tbl->dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        break;
      case BF_DEV_FAMILY_TOFINO2: {
        /* Post a block write per pipe for Tofino2's PGR P0 memory. */
        pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
            shdl,
            tbl->dev_id,
            pipe_mgr_drv_buf_size(tbl->dev_id, PIPE_MGR_DRV_BUF_BWR),
            PIPE_MGR_DRV_BUF_BWR,
            true);
        if (!b || !b->addr) {
          LOG_ERROR("%s:%d Failed to alocate memory for drv buffer",
                    __func__,
                    __LINE__);
          return PIPE_UNEXPECTED;
        }

        PIPE_MGR_MEMCPY(b->addr,
                        tbl->log_pipe_shadow[log_pipe],
                        tbl->dev_info->dev_cfg.num_ports * p0_width * 4);
        sts = pipe_mgr_drv_blk_wr(
            &shdl,
            p0_width * 4,
            tbl->dev_info->dev_cfg.num_ports,
            1,
            tof2_mem_pipes_parde_pgr_ph0_rspec_phase0_mem_word_address >> 4,
            1 << log_pipe,
            b);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to post blkWr for P0 data, dev %d log-pipe %d, sts %s",
              tbl->dev_id,
              log_pipe,
              pipe_str_err(sts));
          return sts;
        }
        break;
      }
      case BF_DEV_FAMILY_TOFINO3: {
        break;
      }

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_create_dma(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info) {
  struct pipe_mgr_dev_ctx *ctx;
  unsigned long key;
  bf_map_t *tbls;
  void *data;
  bf_map_sts_t s;

  pipe_status_t sts = PIPE_SUCCESS;

  ctx = pipe_mgr_dev_ctx_(dev_info->dev_id);
  if (!ctx) return PIPE_INVALID_ARG;

  PIPE_MGR_LOCK(&ctx->p0_tbl_mtx);
  tbls = &ctx->p0_tbls;

  for (s = bf_map_get_first(tbls, &key, &data);
       sts == PIPE_SUCCESS && s == BF_MAP_OK;
       s = bf_map_get_next(tbls, &key, &data))
    sts = program_one_table(shdl, (pipe_mgr_phase0_tbl_t *)data);

  PIPE_MGR_UNLOCK(&ctx->p0_tbl_mtx);
  return sts;
}

static pipe_status_t pipe_mgr_phase0_tbl_ha_init(
    pipe_mgr_phase0_tbl_t *phase0_tbl) {
  uint32_t p0_width = 0, i = 0, j = 0;
  uint32_t total_ports = 0;
  int bit_idx = 0;
  uint32_t num_ports = 0, num_pipes = 0;
  bf_dev_port_t dev_port = 0;

  num_pipes = phase0_tbl->dev_info->num_active_pipes;
  num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;

  total_ports = num_pipes * num_ports;

  /* Initialize the shadow memory */
  p0_width = phase0_tbl->dev_info->dev_cfg.p0_width;
  phase0_tbl->reg_spec =
      (pipe_mgr_phase0_pipe_register_spec_t *)PIPE_MGR_CALLOC(
          total_ports, sizeof(pipe_mgr_phase0_pipe_register_spec_t));
  if (!phase0_tbl->reg_spec) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (i = 0; i < num_pipes; i++) {
    for (j = 0; j < num_ports; j++) {
      dev_port = phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      bit_idx = phase0_tbl->dev_info->dev_cfg.dev_port_to_bit_idx(dev_port);
      phase0_tbl->reg_spec[bit_idx].reg =
          (pipe_register_spec_t *)PIPE_MGR_CALLOC(p0_width,
                                                  sizeof(pipe_register_spec_t));
      if (!phase0_tbl->reg_spec[bit_idx].reg) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
    }
  }

  phase0_tbl->first_entry_replayed = false;

  return PIPE_SUCCESS;
}

static void pipe_mgr_phase0_tbl_ha_deinit(pipe_mgr_phase0_tbl_t *phase0_tbl) {
  uint32_t i = 0, j = 0;
  int bit_idx = 0;
  uint32_t num_ports = 0, num_pipes = 0;
  bf_dev_port_t dev_port = 0;

  num_pipes = phase0_tbl->dev_info->num_active_pipes;
  num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;

  for (i = 0; i < num_pipes; i++) {
    for (j = 0; j < num_ports; j++) {
      dev_port = phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      bit_idx = phase0_tbl->dev_info->dev_cfg.dev_port_to_bit_idx(dev_port);
      if (phase0_tbl->reg_spec[bit_idx].reg) {
        PIPE_MGR_FREE(phase0_tbl->reg_spec[bit_idx].reg);
      }
    }
  }

  if (phase0_tbl->reg_spec) {
    PIPE_MGR_FREE(phase0_tbl->reg_spec);
  }
}

pipe_status_t pipe_mgr_phase0_shadow_mem_populate(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_mat_tbl_hdl_t tbl_hdl) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  uint32_t p0_width = 0;
  uint32_t num_ports = 0, num_pipes = 0;
  uint32_t i, j, m;
  bf_dev_port_t dev_port = 0;
  pipe_status_t sts;
  int bit_idx = 0;
  pipe_memory_spec_t mem_spec_list;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  p0_width = phase0_tbl->dev_info->dev_cfg.p0_width;

  num_pipes = phase0_tbl->dev_info->num_active_pipes;
  num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;

  for (i = 0; i < num_pipes; i++) {
    for (j = 0; j < num_ports; j++) {
      dev_port = phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      bit_idx = phase0_tbl->dev_info->dev_cfg.dev_port_to_bit_idx(dev_port);
      PIPE_MGR_MEMSET(&mem_spec_list, 0, sizeof(mem_spec_list));
      bf_subdev_id_t subdev;
      sts = pipe_mgr_entry_format_phase0_tbl_addr_get(
          phase0_tbl->dev_info,
          dev_port,
          false,
          &subdev,
          &phase0_tbl->reg_spec[bit_idx].reg[0],
          &mem_spec_list);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in reading the registers for port %d, entry_hdl %d for "
            "table "
            " with handle 0x%x, device id %d",
            __func__,
            dev_port,
            phase0_tbl_dev_port_to_entry_handle(dev_port),
            tbl_hdl,
            dev_id);
        return sts;
      }
      switch (phase0_tbl->dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
        case BF_DEV_FAMILY_TOFINO2: {
          for (m = 0; m < p0_width; m++) {
            sts = pipe_mgr_drv_reg_rd(
                &sess_hdl,
                dev_id,
                phase0_tbl->reg_spec[bit_idx].reg[m].reg_addr,
                &phase0_tbl->reg_spec[bit_idx].reg[m].reg_data);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s: Error in reading the registers for port %d, entry_hdl "
                  "%d "
                  "for table "
                  " with handle 0x%x, device id %d",
                  __func__,
                  dev_port,
                  phase0_tbl_dev_port_to_entry_handle(dev_port),
                  tbl_hdl,
                  dev_id);
              return sts;
            }
          }
          break;
        }
        case BF_DEV_FAMILY_TOFINO3: {
          uint64_t data_hi = 0, data_lo = 0;
          lld_subdev_ind_read(
              dev_id, subdev, mem_spec_list.mem_addr0, &data_hi, &data_lo);
          phase0_tbl->reg_spec[bit_idx].reg[0].reg_data = data_lo & 0xffffffff;
          phase0_tbl->reg_spec[bit_idx].reg[1].reg_data =
              (data_lo >> 32) & 0xffffffff;
          phase0_tbl->reg_spec[bit_idx].reg[2].reg_data = data_hi & 0xffffffff;
          phase0_tbl->reg_spec[bit_idx].reg[3].reg_data =
              (data_hi >> 32) & 0xffffffff;
          break;
        }

        case BF_DEV_FAMILY_UNKNOWN:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
          break;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_phase0_llp_populate_move_list(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_mgr_move_list_t **move_list) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_move_list_t *ml = NULL;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle %#x, device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, BF_DEV_PIPE_ALL);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = mat_ent_hdl;
  ml->u.single.logical_idx = PIPE_GET_HDL_VAL(mat_ent_hdl - 1);
  ml->data = make_mat_ent_data(match_spec, action_spec, act_fn_hdl, 0, 0, 0, 0);
  if (!ml->data) {
    PIPE_MGR_FREE(ml);
    *move_list = NULL;
    return PIPE_NO_SYS_RESOURCES;
  }

  if (move_list) {
    (*move_list)->next = ml;
    *move_list = ml;
  } else {
    free_move_list_and_data(&ml, true);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_llp_restore_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_action_spec_t action_spec = {0};
  pipe_tbl_match_spec_t match_spec = {0};
  uint32_t num_ports = 0, num_pipes = 0;
  uint32_t i, j, m;
  bf_dev_port_t dev_port = 0;
  pipe_register_spec_t *reg_spec_list = NULL;
  uint32_t p0_width = 0;
  pipe_mat_ent_hdl_t mat_ent_hdl = 0;
  struct pipe_mgr_move_list_t move_head = {0};
  struct pipe_mgr_move_list_t *move_tail;
  int bit_idx = 0;

  move_tail = &move_head;
  move_head.next = NULL;
  if (move_head_p) {
    *move_head_p = NULL;
  }

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (phase0_tbl->max_act_data_size) {
    action_spec.act_data.action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
        phase0_tbl->max_act_data_size, sizeof(uint8_t));
    if (!action_spec.act_data.action_data_bits) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      sts = PIPE_NO_SYS_RESOURCES;
      goto err_cleanup;
    }
  }

  match_spec.match_value_bits = (uint8_t *)PIPE_MGR_CALLOC(
      phase0_tbl->num_match_spec_bytes, sizeof(uint8_t));
  if (!match_spec.match_value_bits) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    sts = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }
  match_spec.match_mask_bits = (uint8_t *)PIPE_MGR_CALLOC(
      phase0_tbl->num_match_spec_bytes, sizeof(uint8_t));
  if (!match_spec.match_mask_bits) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    sts = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }
  /* Fill out the fixed fields in the match spec */
  PIPE_MGR_MEMSET(
      match_spec.match_mask_bits, 0xFF, phase0_tbl->num_match_spec_bytes);
  match_spec.num_valid_match_bits = phase0_tbl->num_match_spec_bits;
  match_spec.num_match_bytes = phase0_tbl->num_match_spec_bytes;

  /* Fill out the fixed fields in the action spec */
  action_spec.act_data.num_valid_action_data_bits =
      phase0_tbl->act_fn_hdl_info->num_bits;
  action_spec.act_data.num_action_data_bytes =
      phase0_tbl->act_fn_hdl_info->num_bytes;
  action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;

  num_pipes = phase0_tbl->dev_info->num_active_pipes;
  num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;

  p0_width = phase0_tbl->dev_info->dev_cfg.p0_width;

  reg_spec_list = (pipe_register_spec_t *)PIPE_MGR_CALLOC(
      p0_width, sizeof(pipe_register_spec_t));
  if (!reg_spec_list) {
    LOG_ERROR(
        "%s : Could not allocate space for reg_spec_list for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    sts = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }

  /* Iterate through all the ports and build the match value bits and action
   * data bits */
  for (i = 0; i < num_pipes; i++) {
    for (j = 0; j < num_ports; j++) {
      dev_port = phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      mat_ent_hdl = phase0_tbl_dev_port_to_entry_handle(dev_port);
      bit_idx = phase0_tbl->dev_info->dev_cfg.dev_port_to_bit_idx(dev_port);
      for (m = 0; m < p0_width; m++) {
        reg_spec_list[m].reg_addr =
            phase0_tbl->reg_spec[bit_idx].reg[m].reg_addr;
        reg_spec_list[m].reg_data =
            phase0_tbl->reg_spec[bit_idx].reg[m].reg_data;
      }
      sts = pipe_mgr_entry_format_phase0_tbl_decode_reg_to_action_data(
          phase0_tbl->dev_info,
          phase0_tbl->profile_id,
          tbl_hdl,
          reg_spec_list,
          &action_spec.act_data);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in recovering the action data spec from the read "
            "registers for port %d, entry_hdl %d for table "
            " with handle 0x%x, device id %d",
            __func__,
            dev_port,
            mat_ent_hdl,
            tbl_hdl,
            dev_id);
        return sts;
      }

      sts = pipe_mgr_entry_format_tof_phase0_tbl_get_match_spec(
          dev_id, phase0_tbl->profile_id, tbl_hdl, dev_port, &match_spec);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in recovering the match spec for port %d, entry_hdl %d "
            "for table "
            " with handle 0x%x, device id %d",
            __func__,
            dev_port,
            mat_ent_hdl,
            tbl_hdl,
            dev_id);
        return sts;
      }

      sts = pipe_mgr_phase0_llp_populate_move_list(
          dev_id,
          tbl_hdl,
          mat_ent_hdl,
          phase0_tbl->act_fn_hdl_info->act_fn_hdl,
          &match_spec,
          &action_spec,
          &move_tail);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in populating move list in llp restore for port %d, "
            "entry_hdl %d for table "
            " with handle 0x%x, device id %d",
            __func__,
            dev_port,
            mat_ent_hdl,
            tbl_hdl,
            dev_id);
        return sts;
      }
      phase0_tbl->num_entries_programmed[i]++;

      // Reinitialize action data and match spec
      PIPE_MGR_MEMSET(action_spec.act_data.action_data_bits,
                      0,
                      phase0_tbl->max_act_data_size);
      PIPE_MGR_MEMSET(
          match_spec.match_value_bits, 0, phase0_tbl->num_match_spec_bytes);
    }
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

err_cleanup:
  if (action_spec.act_data.action_data_bits) {
    PIPE_MGR_FREE(action_spec.act_data.action_data_bits);
  }
  if (match_spec.match_value_bits) {
    PIPE_MGR_FREE(match_spec.match_value_bits);
  }
  if (match_spec.match_mask_bits) {
    PIPE_MGR_FREE(match_spec.match_mask_bits);
  }
  if (reg_spec_list) {
    PIPE_MGR_FREE(reg_spec_list);
  }
  return sts;
}

pipe_status_t pipe_mgr_phase0_hlp_restore_state(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                pipe_mgr_move_list_t *move_list,
                                                uint32_t *success_count) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mgr_move_list_t *move_node = NULL;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_action_spec_t *action_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  bf_dev_pipe_t pipe_id, pipe;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  struct pipe_mgr_mat_data *data = NULL;
  pipe_mat_tbl_info_t *tbl_info = NULL;

  tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (move_node = move_list; move_node; move_node = move_node->next) {
    match_spec = unpack_mat_ent_data_ms(move_node->data);
    action_spec = unpack_mat_ent_data_as(move_node->data);
    act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_node->data);

    PIPE_MGR_DBGCHK(move_node->op == PIPE_MAT_UPDATE_ADD ||
                    move_node->op == PIPE_MAT_UPDATE_SET_DFLT);
    if (move_node->op != PIPE_MAT_UPDATE_ADD &&
        move_node->op != PIPE_MAT_UPDATE_SET_DFLT) {
      LOG_ERROR(
          "%s:%d Invalid move node operation %d for restoring HLP state for "
          "tbl 0x%x, device id %d",
          __func__,
          __LINE__,
          move_node->op,
          tbl_hdl,
          dev_id);
      sts = PIPE_INVALID_ARG;
      break;
    }
    pipe_id = get_move_list_pipe(move_node);
    if (move_node == move_list) {
      phase0_tbl->symmetric = pipe_id == BF_DEV_PIPE_ALL ? true : false;
      tbl_info->symmetric = phase0_tbl->symmetric;
    }

    mat_ent_hdl = move_node->entry_hdl;
    pipe = hdl_to_pipe(phase0_tbl->dev_info, mat_ent_hdl);

    sts = pipe_mgr_phase0_check_tbl_symm_and_pipe(phase0_tbl, pipe_id, pipe);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for %s phase0 tbl with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          pipe_id,
          phase0_tbl->symmetric ? "symmetric" : "asymmetric",
          tbl_hdl,
          dev_id);
      return sts;
    }

    if (move_node->op == PIPE_MAT_UPDATE_ADD)
      phase0_tbl_data = phase0_tbl_get_instance_from_any_pipe(phase0_tbl, pipe);
    else
      phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, pipe_id);
    if (phase0_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
          __func__,
          __LINE__,
          dev_id,
          phase0_tbl->name,
          phase0_tbl->mat_tbl_hdl,
          move_node->op == PIPE_MAT_UPDATE_ADD ? pipe : pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (move_node->op == PIPE_MAT_UPDATE_ADD ||
        move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
      sts = get_ent_data(phase0_tbl_data, mat_ent_hdl, &data);
      if (sts == PIPE_SUCCESS) {
        if (data) {
          // Indicates that the same mat ent hdl was populated in the move
          // list by the llp
          PIPE_MGR_DBGCHK(0);
        }
      }
      data = make_mat_ent_data(match_spec, action_spec, act_fn_hdl, 0, 0, 0, 0);
      if (!data) {
        return PIPE_NO_SYS_RESOURCES;
      }
      sts = save_ent_data(phase0_tbl_data, mat_ent_hdl, data);
      if (sts != PIPE_SUCCESS) {
        return PIPE_NO_SYS_RESOURCES;
      }
      phase0_tbl->num_entries_placed[pipe]++;
    }
    /* Add the match-spec to the hash-table keyed by the match-spec */
    if (!pipe_mgr_is_device_virtual(dev_id)) {
      if (move_node->op == PIPE_MAT_UPDATE_ADD) {
        sts = pipe_mgr_hitless_ha_new_spec(
            &phase0_tbl_data->ha_hlp_info->spec_map, move_node);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error adding phase0 entry with hdl 0x%x to entry map for "
              "tbl "
              "%s, "
              "device id %d, tbl hdl 0x%x, err %s",
              __func__,
              __LINE__,
              mat_ent_hdl,
              phase0_tbl->name,
              dev_id,
              tbl_hdl,
              pipe_str_err(sts));
          break;
        }
      } else {
        /* Save the default entry spec. */
        pipe_mgr_spec_map_t *spec_map = &phase0_tbl_data->ha_hlp_info->spec_map;
        pipe_action_spec_t *aspec_copy = NULL;
        aspec_copy = pipe_mgr_tbl_copy_action_spec(
            NULL, unpack_mat_ent_data_as(move_node->data));
        if (!aspec_copy) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
        if (spec_map->def_act_spec)
          pipe_mgr_tbl_destroy_action_spec(&spec_map->def_act_spec);
        spec_map->def_act_spec = aspec_copy;
        spec_map->def_act_fn_hdl =
            unpack_mat_ent_data_afun_hdl(move_node->data);
      }
    }
  }

  (void)sess_hdl;
  (void)success_count;

  return sts;
}

pipe_status_t pipe_mgr_phase0_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (!phase0_tbl_data) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  PIPE_MGR_MEMCPY(ha_report,
                  &phase0_tbl_data->ha_reconc_report,
                  sizeof(pipe_tbl_ha_reconc_report_t));

  return PIPE_SUCCESS;
}

static pipe_status_t phase0_tbl_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    bool is_txn,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mat_ent_hdl_t def_ent_hdl = 0;
  pipe_mat_tbl_info_t *tbl_info = NULL;

  tbl_info = pipe_mgr_get_tbl_info(
      dev_tgt.device_id, phase0_tbl->mat_tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  def_ent_hdl = phase0_tbl_data->default_entry_hdl;

  /* Check if this entry already exists. */
  struct pipe_mgr_mat_data *old_data = NULL;
  status = get_ent_data(phase0_tbl_data, def_ent_hdl, &old_data);
  if (PIPE_SUCCESS == status) {
    PIPE_MGR_DBGCHK(old_data);
    status = rmv_ent_data(phase0_tbl_data, def_ent_hdl, &old_data);
    PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
    PIPE_MGR_DBGCHK(old_data);
  }
  /* Create a set_default move node if there was no previous default configured.
   * Otherwise it will be modify move node. Both will be expanded into a series
   * of adds in the LLP to populate all unoccupied entries. */
  pipe_mgr_move_list_t *move_node =
      alloc_move_list(NULL,
                      old_data ? PIPE_MAT_UPDATE_MOD : PIPE_MAT_UPDATE_SET_DFLT,
                      dev_tgt.dev_pipe_id);
  if (move_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    if (old_data) {
      status = save_ent_data(phase0_tbl_data, def_ent_hdl, old_data);
      PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
    }
    return PIPE_NO_SYS_RESOURCES;
  }
  *pipe_move_list = move_node;

  move_node->entry_hdl = def_ent_hdl;
  move_node->old_data = old_data;
  move_node->data = make_mat_ent_data(NULL, act_spec, act_fn_hdl, 0, 0, 0, 0);
  if (!move_node->data) {
    goto err_cleanup;
  }

  status = save_ent_data(phase0_tbl_data, def_ent_hdl, move_node->data);
  if (PIPE_SUCCESS != status) goto err_cleanup;

  if (is_txn) {
    status = log_op_for_txn(phase0_tbl_data, move_node);
    if (PIPE_SUCCESS != status) {
      struct pipe_mgr_mat_data *throw_away_data = NULL;
      status = rmv_ent_data(phase0_tbl_data, def_ent_hdl, &throw_away_data);
      PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
      PIPE_MGR_DBGCHK(throw_away_data == move_node->data);
      goto err_cleanup;
    }
  }

  *ent_hdl_p = def_ent_hdl;
  return PIPE_SUCCESS;

err_cleanup:
  if (move_node->data) free_mat_ent_data(move_node->data);
  PIPE_MGR_FREE(move_node);
  *pipe_move_list = NULL;
  if (old_data) {
    status = save_ent_data(phase0_tbl_data, def_ent_hdl, old_data);
    PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
  }
  return PIPE_NO_SYS_RESOURCES;
}

pipe_status_t pipe_mgr_phase0_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mat_tbl_info_t *tbl_info = NULL;
  pipe_mgr_spec_map_t *spec_map = NULL;
  pipe_tbl_ha_reconc_report_t *ha_report = NULL;
  pipe_mgr_move_list_t *mh = NULL;
  pipe_mgr_move_list_t *move_tail = NULL;
  struct pipe_mgr_move_list_t move_head;
  unsigned int i;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  move_head.next = NULL;
  move_tail = &move_head;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (!phase0_tbl) {
    LOG_ERROR(
        "%s : Could not get the phase0 table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  for (i = 0; i < phase0_tbl->num_tbls; i++) {
    spec_map = &phase0_tbl->phase0_tbl_data[i].ha_hlp_info->spec_map;
    ha_report = &phase0_tbl->phase0_tbl_data[i].ha_reconc_report;
    sts = pipe_mgr_hitless_ha_reconcile(spec_map, &mh, tbl_info, ha_report);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error reconciling HA state, for tbl 0x%x, device id %d, "
          "err %s",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_str_err(sts));
      *move_head_p = NULL;
      return sts;
    }
    if (mh) {
      move_tail->next = mh;
      move_tail = mh;
      while (move_tail->next) move_tail = move_tail->next;
      mh = NULL;
    }
    pipe_mat_ent_hdl_t def_ent_hdl;
    if (spec_map->def_act_spec) {
      sts = phase0_tbl_default_ent_place(spec_map->dev_tgt,
                                         phase0_tbl,
                                         spec_map->def_act_fn_hdl,
                                         spec_map->def_act_spec,
                                         pipe_mgr_sess_in_txn(sess_hdl),
                                         &def_ent_hdl,
                                         &mh);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error reconciling HA state for default entry, for tbl 0x%x, "
            "device id %d, err %s",
            __func__,
            __LINE__,
            tbl_hdl,
            dev_id,
            pipe_str_err(sts));
        return sts;
      }
      if (mh) {
        move_tail->next = mh;
        move_tail = mh;
        mh = NULL;
      }
    }
  }
  *move_head_p = move_head.next;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_phase0_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    bool is_txn,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_mat_ent_hdl_t def_ent_hdl = 0;
  pipe_mat_tbl_info_t *tbl_info = NULL;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);

  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    status = PIPE_OBJ_NOT_FOUND;
    return status;
  }

  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id) &&
      !phase0_tbl->first_entry_replayed) {
    /* During warm init, set the symmetricity of the table based on the pipe id
       passed in by the user. The user must pass in the correct pipe id based
       on the symmetricity and the entry being added. */
    // This is the first entry being placed in the table.
    tbl_info = pipe_mgr_get_tbl_info(
        dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
    if (!tbl_info) {
      LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
                __func__,
                __LINE__,
                mat_tbl_hdl,
                dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    }
    phase0_tbl->symmetric =
        dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ? true : false;
    tbl_info->symmetric = phase0_tbl->symmetric;
    phase0_tbl->first_entry_replayed = true;
  }

  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  def_ent_hdl = phase0_tbl_data->default_entry_hdl;

  log_entry_set_default(phase0_tbl, def_ent_hdl, act_fn_hdl, act_spec);

  /* If this is a hitless HA replay just save the default entry in the table's
   * spec map so it can be used in the compute-delta phase later. */
  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    pipe_mgr_spec_map_t *spec_map = &phase0_tbl_data->ha_hlp_info->spec_map;
    pipe_action_spec_t *aspec_copy = NULL;
    aspec_copy = pipe_mgr_tbl_copy_action_spec(NULL, act_spec);
    if (!aspec_copy) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    if (spec_map->def_act_spec) {
      pipe_mgr_tbl_destroy_action_spec(&spec_map->def_act_spec);
    }
    spec_map->def_act_spec = aspec_copy;
    spec_map->def_act_fn_hdl = act_fn_hdl;
    *ent_hdl_p = def_ent_hdl;
    return PIPE_SUCCESS;
  }

  return phase0_tbl_default_ent_place(dev_tgt,
                                      phase0_tbl,
                                      act_fn_hdl,
                                      act_spec,
                                      is_txn,
                                      ent_hdl_p,
                                      pipe_move_list);
}

pipe_status_t pipe_mgr_phase0_cleanup_default_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mat_ent_hdl_t def_ent_hdl;
  bool null_pipe_move_list = true;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  def_ent_hdl = phase0_tbl_data->default_entry_hdl;

  /* Check if this entry already exists. */
  struct pipe_mgr_mat_data *old_data = NULL;
  status = get_ent_data(phase0_tbl_data, def_ent_hdl, &old_data);
  if (status == PIPE_OBJ_NOT_FOUND) {
    return PIPE_SUCCESS;
  }
  PIPE_MGR_DBGCHK(old_data);
  /* Create a clear_default move node. This will be expanded into a series of
   * cleans in the LLP to populate all unoccupied entries. */
  pipe_mgr_move_list_t *move_node =
      alloc_move_list(NULL, PIPE_MAT_UPDATE_CLR_DFLT, dev_tgt.dev_pipe_id);
  if (move_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    status = save_ent_data(phase0_tbl_data, def_ent_hdl, old_data);
    PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (*pipe_move_list) {
    (*pipe_move_list)->next = move_node;
    null_pipe_move_list = false;
  } else {
    *pipe_move_list = move_node;
  }
  move_node->entry_hdl = def_ent_hdl;
  move_node->data = old_data;
  status = rmv_ent_data(phase0_tbl_data, def_ent_hdl, &old_data);
  PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
  PIPE_MGR_DBGCHK(old_data == move_node->data);
  if (pipe_api_flags & PIPE_MGR_TBL_API_TXN) {
    status = log_op_for_txn(phase0_tbl_data, move_node);
    if (PIPE_SUCCESS != status) goto err_cleanup;
  }
  LOG_TRACE(
      "%s : Clearing the default entry, with handle %#x in tbl %#x on dev %d",
      __func__,
      def_ent_hdl,
      mat_tbl_hdl,
      dev_tgt.device_id);
  return PIPE_SUCCESS;

err_cleanup:
  PIPE_MGR_FREE(move_node);
  if (null_pipe_move_list)
    *pipe_move_list = NULL;
  else
    (*pipe_move_list)->next = NULL;
  status = save_ent_data(phase0_tbl_data, def_ent_hdl, old_data);
  PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
  return PIPE_NO_SYS_RESOURCES;
}

static pipe_status_t phase0_tbl_default_ent_get_from_hw(
    dev_target_t dev_tgt,
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  /* Go over all entries in the table, if the entry has not been programmed
   * retrieve the default action data from hw. */
  uint32_t num_pipes = phase0_tbl->dev_info->num_active_pipes;
  uint8_t num_ports = phase0_tbl->dev_info->dev_cfg.num_ports;

  /* Iterate through all the pipes in the scope. */
  for (unsigned int i = 0; i < num_pipes; i++) {
    if ((phase0_tbl_data->scope_pipe_bmp & (1u << i)) == 0) continue;
    /* Iterate through all the ports in the pipe. */
    for (unsigned int j = 0; j < num_ports; j++) {
      bf_dev_port_t dev_port =
          phase0_tbl->dev_info->dev_cfg.make_dev_port(i, j);
      pipe_mat_ent_hdl_t mat_ent_hdl =
          phase0_tbl_dev_port_to_entry_handle(dev_port);

      if (phase0_tbl_installed_get(phase0_tbl, phase0_tbl_data, mat_ent_hdl))
        continue;
      return pipe_mgr_phase0_get_entry(phase0_tbl->mat_tbl_hdl,
                                       dev_tgt.device_id,
                                       mat_ent_hdl,
                                       NULL,
                                       action_spec,
                                       act_fn_hdl,
                                       true);
    }
  }
  LOG_TRACE(
      "%s : Default entry cannot be read from hw for phase0 table with"
      " handle 0x%x for device id %d",
      __func__,
      phase0_tbl->mat_tbl_hdl,
      dev_tgt.device_id);
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_phase0_default_ent_get(dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_action_spec_t *action_spec,
                                              pipe_act_fn_hdl_t *act_fn_hdl,
                                              bool from_hw) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  pipe_status_t status;

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (from_hw) {
    status = phase0_tbl_default_ent_get_from_hw(
        dev_tgt, phase0_tbl, phase0_tbl_data, action_spec, act_fn_hdl);
    if (status != PIPE_OBJ_NOT_FOUND) return status;
    /* Continue and read from LLP SW state when all entries were occupied,
     * namely when PIPE_OBJ_NOT_FOUND was returned above. */
    if (phase0_tbl_data->dflt_act_spec && action_spec) {
      if (!pipe_mgr_tbl_copy_action_spec(action_spec,
                                         phase0_tbl_data->dflt_act_spec)) {
        return PIPE_NO_SYS_RESOURCES;
      }
      *act_fn_hdl = phase0_tbl->act_fn_hdl_info->act_fn_hdl;
      return PIPE_SUCCESS;
    }
    return pipe_mgr_tbl_get_init_default_entry(
        dev_tgt.device_id, mat_tbl_hdl, action_spec, act_fn_hdl);
  }
  struct pipe_mgr_mat_data *data = NULL;
  status =
      get_ent_data(phase0_tbl_data, phase0_tbl_data->default_entry_hdl, &data);
  if (PIPE_SUCCESS != status) {
    LOG_DBG(
        "%s: Default entry handle %#x requested for phase0 table %#x on dev "
        "%d not placed",
        __func__,
        phase0_tbl_data->default_entry_hdl,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return pipe_mgr_tbl_get_init_default_entry(
        dev_tgt.device_id, mat_tbl_hdl, action_spec, act_fn_hdl);
  }
  if (action_spec) {
    if (!pipe_mgr_tbl_copy_action_spec(action_spec,
                                       unpack_mat_ent_data_as(data)))
      return PIPE_NO_SYS_RESOURCES;
  }
  *act_fn_hdl = unpack_mat_ent_data_afun_hdl(data);
  return PIPE_SUCCESS;
}

/* Get default entry handle */
pipe_status_t pipe_mgr_phase0_default_ent_hdl_get(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data = NULL;
  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  phase0_tbl_data = phase0_tbl_get_instance(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  *ent_hdl_p = phase0_tbl_data->default_entry_hdl;
  return PIPE_SUCCESS;
}

static bool phase0_tbl_is_default_entry(
    pipe_mgr_phase0_tbl_t *phase0_tbl,
    pipe_mgr_phase0_tbl_data_t *phase0_tbl_data,
    pipe_mat_ent_hdl_t ent_hdl,
    bool *default_exist) {
  *default_exist = false;

  pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
      phase0_tbl->dev_id, phase0_tbl->mat_tbl_hdl, __func__, __LINE__);
  if (!mat_tbl_info) {
    LOG_ERROR(
        "%s:%d Unable to find match table info for Phase0 table 0x%x "
        "device id %d",
        __func__,
        __LINE__,
        phase0_tbl->mat_tbl_hdl,
        phase0_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  if (phase0_tbl_is_default_ent_installed(phase0_tbl_data) ||
      mat_tbl_info->default_info) {
    *default_exist = true;
    if (!phase0_tbl_installed_get(phase0_tbl, phase0_tbl_data, ent_hdl)) {
      return true;
    }
  }
  return false;
}

/* Used to validate entry if default is not available */
static bool is_zero_entry(pipe_action_spec_t *act_spec) {
  for (int i = 0; i < act_spec->act_data.num_action_data_bytes; i++) {
    if (act_spec->act_data.action_data_bits[i] != 0) {
      return false;
    }
  }

  return true;
}

pipe_status_t pipe_mgr_phase0_tbl_raw_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint32_t tbl_index,
    bool err_correction,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_ent_hdl_t *entry_hdl,
    bool *is_default,
    uint32_t *next_index) {
  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_phase0_tbl_t *phase0_tbl =
      pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (tbl_index >= phase0_tbl->dev_info->dev_cfg.num_ports) {
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_phase0_tbl_data_t *phase0_tbl_data =
      phase0_tbl_get_instance_from_any_pipe(phase0_tbl, dev_tgt.dev_pipe_id);
  if (phase0_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Phase0 tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              phase0_tbl->dev_id,
              phase0_tbl->name,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_dev_port_t dev_port = phase0_tbl->dev_info->dev_cfg.make_dev_port(
      dev_tgt.dev_pipe_id, tbl_index);

  pipe_mat_ent_hdl_t ent_hdl = phase0_tbl_dev_port_to_entry_handle(dev_port);

  if (act_spec->act_data.num_action_data_bytes != 0 &&
      act_spec->act_data.num_action_data_bytes !=
          phase0_tbl->max_act_data_size) {
    LOG_ERROR("%s:%d Provided action spec does not align with phase tbl",
              __func__,
              __LINE__);
    return PIPE_INVALID_ARG;
  }
  if (!act_spec->act_data.action_data_bits && phase0_tbl->max_act_data_size) {
    act_spec->act_data.action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
        phase0_tbl->max_act_data_size, sizeof(uint8_t));
    if (!act_spec->act_data.action_data_bits) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    act_spec->act_data.num_action_data_bytes = phase0_tbl->max_act_data_size;
  }

  pipe_tbl_match_spec_t *mspec =
      pipe_mgr_tbl_alloc_match_spec(phase0_tbl->num_match_spec_bytes);
  pipe_status_t sts = pipe_mgr_phase0_get_entry(tbl_hdl,
                                                dev_tgt.device_id,
                                                ent_hdl,
                                                mspec,
                                                act_spec,
                                                act_fn_hdl,
                                                true /*from_hw*/);
  if (sts != PIPE_SUCCESS) {
    goto done;
  }

  if (pipe_mgr_tbl_copy_match_spec(match_spec, mspec) == NULL) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto done;
  }

  bool default_exist = false;
  *is_default = phase0_tbl_is_default_entry(
      phase0_tbl, phase0_tbl_data, ent_hdl, &default_exist);

  bool exist = phase0_tbl_installed_get(phase0_tbl, phase0_tbl_data, ent_hdl);

  if (*is_default && !exist) {
    // OK - nothing to do, since default values are expected for not programmed
    // entries.
    sts = PIPE_SUCCESS;
  } else if (!*is_default && !exist) {
    // This is failure if default exists.
    if (!default_exist && is_zero_entry(act_spec)) {
      // If default does not exist entry should have 0 in act_spec
      sts = PIPE_SUCCESS;
    } else {
      if (err_correction) {
        bool configure;
        sts = phase0_tbl_program_check_prsr(phase0_tbl, ent_hdl, &configure);
        if (sts != PIPE_SUCCESS) {
          // Keep missmatch error code.
          sts = PIPE_INTERNAL_ERROR;
          goto done;
        }

        pipe_action_spec_t a_spec = {0};
        sts = phase0_tbl_program(sess_hdl,
                                 &a_spec,  // can be reused for delete
                                 phase0_tbl,
                                 ent_hdl,
                                 false /*is_default*/,
                                 true /*is_delete*/,
                                 false /*is_default_clear*/,
                                 configure);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "%s : Could not correct phase0 entry hdl 0x%x, tbl 0x%x, dev %d, "
              "sts %s",
              __func__,
              ent_hdl,
              phase0_tbl->mat_tbl_hdl,
              dev_tgt.device_id,
              pipe_str_err(sts));
        }
      }
      sts = PIPE_INTERNAL_ERROR;
    }
  } else if (exist) {
    // OK - This is ok for both default and non default HW values.
    // It is possible to program default values to non-default entry.
    // Let the caller verify against hdl.
    *entry_hdl = ent_hdl;
  }  // else - return error code

  *next_index = tbl_index + 1;
done:
  pipe_mgr_tbl_destroy_match_spec(&mspec);
  return sts;
}

pipe_status_t pipe_mgr_phase0_get_last_index(dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             uint32_t *last_index) {
  pipe_mgr_phase0_tbl_t *phase0_tbl =
      pipe_mgr_phase0_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (phase0_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the phase0 match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *last_index = phase0_tbl->dev_info->dev_cfg.num_ports - 1;

  return PIPE_SUCCESS;
}
