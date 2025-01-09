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
 * @file pipe_mgr_adt_mgr.c
 * @date
 *
 * Action data table manager, which includes all aspects of management such as
 * management of explicitly created action tables, exposes API's to create
 * action data table entries for the unmanaged action tables to the table
 * management entities (such as exact-match manager, TCAM manager etc).
 *
 */

/* Standard header includes */
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <target-utils/power2_allocator/power2_allocator.h>

/* Local header includes */
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_tofino.h"
#include "pipe_mgr_adt_drv_workflows.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_hw_dump.h"
#include "pipe_mgr_db.h"

/* Global hash table for action data table state */
static pipe_status_t pipe_mgr_adt_get_new_ent_idx(
    pipe_mgr_adt_stage_info_t *adt_stage_info, pipe_adt_ent_idx_t *entry_idx) {
  power2_allocator_t *power2_allocator = NULL;
  int new_entry_idx = -1;
  power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;
  new_entry_idx = power2_allocator_alloc(power2_allocator, 1);
  if (new_entry_idx == -1) {
    return PIPE_NO_SYS_RESOURCES;
  }
  *entry_idx = new_entry_idx;
  return PIPE_SUCCESS;
}

static void pipe_mgr_adt_release_ent_idx(
    pipe_mgr_adt_stage_info_t *adt_stage_info, pipe_adt_ent_idx_t adt_ent_idx) {
  power2_allocator_t *power2_allocator = NULL;
  power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;
  power2_allocator_release_multiple(power2_allocator, adt_ent_idx, 1);
  return;
}

static pipe_status_t pipe_mgr_adt_ent_place(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t *entry_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  /* Get a free entry from the stage to house the new entry */
  status = pipe_mgr_adt_get_new_ent_idx(adt_stage_info, entry_idx);
  if (status == PIPE_NO_SYS_RESOURCES) {
    /* No free space */
    LOG_ERROR(
        "%s : No free space found for a new action data table entry"
        " in stage %d for table with handle %d",
        __func__,
        adt_stage_info->stage_id,
        adt->adt_tbl_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_ent_reserve(
    pipe_mgr_adt_stage_info_t *adt_stage_info, pipe_adt_ent_idx_t entry_idx) {
  power2_allocator_t *power2_allocator = NULL;
  power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;
  int status = power2_allocator_reserve(power2_allocator, entry_idx, 1);
  if (status == 0) {
    return PIPE_SUCCESS;
  }
  LOG_ERROR(
      "%s:%d Error in reserving entry idx %d", __func__, __LINE__, entry_idx);
  PIPE_MGR_DBGCHK(0);
  return PIPE_UNEXPECTED;
}

static pipe_status_t pipe_mgr_adt_ent_reserve_contiguous(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    uint32_t num_entries_per_block,
    uint32_t num_blocks,
    pipe_adt_ent_idx_t *entry_idx_start) {
  int entry_idx = -1;
  /* Allocate a group of contiguous entry indices */
  if (*entry_idx_start == PIPE_MGR_LOGICAL_ACT_IDX_INVALID) {
    entry_idx = power2_allocator_alloc_multiple(
        adt_stage_info->entry_allocator, num_entries_per_block, num_blocks);

    if (entry_idx == -1) {
      LOG_ERROR(
          "%s : Could not allocate a group of %d contiguous entries"
          " for action data table with handle %d in stage %d",
          __func__,
          num_entries_per_block * num_blocks,
          adt->adt_tbl_hdl,
          adt_stage_info->stage_id);

      return PIPE_NO_SPACE;
    }
    *entry_idx_start = (pipe_adt_ent_idx_t)entry_idx;
  } else {
    /* State restore case. Specifically allocate the blocks starting from
     * the given entry index.
     */
    uint32_t log2;
    uint32_t block_idx;

    log2 = log2_uint32_ceil(num_entries_per_block);
    entry_idx = *entry_idx_start;
    for (block_idx = 0; block_idx < num_blocks; block_idx++) {
      if (power2_allocator_reserve(adt_stage_info->entry_allocator,
                                   entry_idx,
                                   num_entries_per_block)) {
        LOG_ERROR(
            "%s : Could not allocate a group of %d contiguous entries"
            " for action data table with handle %d in stage %d base idx %d",
            __func__,
            num_entries_per_block * num_blocks,
            adt->adt_tbl_hdl,
            adt_stage_info->stage_id,
            *entry_idx_start);

        return PIPE_NO_SPACE;
      }
      entry_idx += (1 << log2);
    }
  }
  return PIPE_SUCCESS;
}

pipe_mgr_adt_entry_info_t *pipe_mgr_adt_get_entry_info(
    pipe_mgr_adt_t *adt, pipe_adt_ent_hdl_t adt_ent_hdl) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = adt_ent_hdl;
  pipe_mgr_adt_entry_info_t *entry_info;
  pipe_mgr_adt_data_t *adt_tbl_data;

  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, adt_ent_hdl);
  if (!adt_tbl_data) {
    return NULL;
  }

  map_sts =
      bf_map_get(&adt_tbl_data->entry_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    return NULL;
  } else if (map_sts != BF_MAP_OK) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }
  return entry_info;
}

pipe_mgr_adt_entry_phy_info_t *pipe_mgr_adt_get_entry_phy_info(
    pipe_mgr_adt_t *adt, pipe_adt_ent_hdl_t adt_ent_hdl) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = adt_ent_hdl;
  pipe_mgr_adt_entry_phy_info_t *entry_info;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;

  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, adt_ent_hdl);
  if (!adt_tbl_data) {
    return NULL;
  }

  map_sts =
      bf_map_get(&adt_tbl_data->entry_phy_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    return NULL;
  } else if (map_sts != BF_MAP_OK) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }
  return entry_info;
}

static pipe_adt_ent_hdl_t pipe_mgr_adt_get_new_entry_hdl(
    pipe_mgr_adt_data_t *adt_tbl_data) {
  int id;
  pipe_adt_ent_hdl_t ent_hdl;
  if (adt_tbl_data == NULL) {
    return PIPE_ADT_ENT_HDL_INVALID_HDL;
  }
  id = bf_id_allocator_allocate(adt_tbl_data->ent_hdl_allocator);
  if (id == -1) {
    return PIPE_ADT_ENT_HDL_INVALID_HDL;
  }
  ent_hdl = id;
  if (adt_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
    ent_hdl = PIPE_SET_HDL_PIPE(ent_hdl, adt_tbl_data->pipe_id);
  }
  return ent_hdl;
}

static inline void pipe_mgr_adt_release_entry_hdl(
    pipe_mgr_adt_data_t *adt_tbl_data, pipe_adt_ent_hdl_t adt_ent_hdl) {
  bf_id_allocator_release(adt_tbl_data->ent_hdl_allocator,
                          PIPE_GET_HDL_VAL(adt_ent_hdl));
}

static inline bool adt_is_mbr_const(pipe_mgr_adt_data_t *inst,
                                    pipe_adt_mbr_id_t mbr_id) {
  unsigned long key = mbr_id;
  void *unused = NULL;
  return BF_MAP_OK == bf_map_get(&inst->const_init_entries, key, &unused);
}
static pipe_status_t pipe_mgr_adt_update_state_for_new_entry(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_adt_mbr_id_t mbr_id,
    bool is_const,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mgr_adt_move_list_t *move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_entry_info_t *adt_entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  adt_entry_info = PIPE_MGR_CALLOC(1, sizeof *adt_entry_info);
  if (adt_entry_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;
  adt_entry_info->pipe_id = adt_tbl_data->pipe_id;
  adt_entry_info->mbr_id = mbr_id;
  adt_entry_info->handle = adt_ent_hdl;
  adt_data_resources_t *resources = NULL;
  if (action_spec->resource_count) {
    resources = (adt_data_resources_t *)PIPE_MGR_CALLOC(
        action_spec->resource_count, sizeof(adt_data_resources_t));
    if (resources == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_FREE(adt_entry_info);
      return PIPE_NO_SYS_RESOURCES;
    }
    int i = 0;
    for (i = 0; i < action_spec->resource_count; i++) {
      resources[i].tbl_hdl = action_spec->resources[i].tbl_hdl;
      resources[i].tbl_idx = action_spec->resources[i].tbl_idx;
    }
  }
  adt_entry_info->entry_data = make_adt_ent_data(act_data_spec,
                                                 act_fn_hdl,
                                                 action_spec->resource_count,
                                                 is_const,
                                                 resources);
  if (adt_entry_info->entry_data == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting adt entry data for entry hdl %d, tbl 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  /* Insert into the hash tables */
  map_sts = bf_map_add(
      &adt_tbl_data->entry_info_htbl, adt_ent_hdl, (void *)adt_entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in adding action entry info for entry handle %d, table "
        "0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  if (adt->cache_id || is_const) {
    map_sts =
        bf_map_add(&adt_tbl_data->mbr_id_map, mbr_id, (void *)adt_entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in adding action entry info for entry handle %d, table "
          "0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          adt_ent_hdl,
          adt->adt_tbl_hdl,
          adt->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
      bf_map_rmv(&adt_tbl_data->entry_info_htbl, adt_ent_hdl);
      goto err_cleanup;
    }
  }
  if (move_list_node) {
    /* Populate the move list node only if a valid node is passed.
     * In HA state rebuilding case, the state is built using a move-list
     * node and hence no need to build the move-list node.
     */
    move_list_node->data = adt_entry_info->entry_data;
    move_list_node->entry_hdl = adt_ent_hdl;
    move_list_node->op = PIPE_ADT_UPDATE_ADD;
    move_list_node->pipe_id = adt_tbl_data->pipe_id;
  }
  /* Increment the number of entries occupied at the pipe level */
  adt_tbl_data->num_entries_occupied++;
  if (resources) {
    PIPE_MGR_FREE(resources);
  }
  return PIPE_SUCCESS;

err_cleanup:
  if (resources) {
    PIPE_MGR_FREE(resources);
  }
  if (adt_entry_info) {
    if (adt_entry_info->entry_data) {
      free_adt_ent_data(adt_entry_info->entry_data);
    }
    PIPE_MGR_FREE(adt_entry_info);
  }
  return status;
}

static pipe_status_t pipe_mgr_adt_update_state_for_entry_delete(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_mgr_adt_move_list_t *move_list_node,
    bool isTxn) {
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;

  unsigned long key = adt_ent_hdl;
  map_sts =
      bf_map_get_rmv(&adt_tbl_data->entry_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, action tbl 0x%x, device id %d "
        "not found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* This entry delete should be called after all references to this entry
   * have gone away. If there are match entries referring to this entry,
   * this delete will be failed.
   */
  if (entry_info->num_references > 0) {
    /* This entry handle is being referenced by any match-entries (or some
     * other kind of entry) and hence cannot be deleted.
     */
    LOG_ERROR(
        "%s : Cannot delete action data entry with handle %d, tbl_hdl"
        " %d to which  match entries are referring to",
        __func__,
        adt_ent_hdl,
        adt->adt_tbl_hdl);
    /* Re-insert the entry back into the hash table */
    map_sts =
        bf_map_add(&adt_tbl_data->entry_info_htbl, key, (void *)entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in re-inserting entry info for entry %d, tbl 0x%x, "
          "device id %d, err 0x%x",
          __func__,
          __LINE__,
          adt_ent_hdl,
          adt->adt_tbl_hdl,
          adt->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    return PIPE_ENTRY_REFERENCES_EXIST;
  }
  if (adt->cache_id) {
    map_sts = bf_map_rmv(&adt_tbl_data->mbr_id_map, entry_info->mbr_id);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Adt table (0x%x) data handle (%d) mbr_id (%d) mapping "
          "missing ",
          __func__,
          __LINE__,
          adt->adt_tbl_hdl,
          adt_ent_hdl,
          entry_info->mbr_id);
    }
  }
  PIPE_MGR_DBGCHK(entry_info->sharable_stage_location == NULL);
  move_list_node->data = entry_info->entry_data;
  PIPE_MGR_FREE(entry_info);
  move_list_node->entry_hdl = adt_ent_hdl;
  move_list_node->op = PIPE_ADT_UPDATE_DEL;
  move_list_node->pipe_id = adt_tbl_data->pipe_id;
  /* Decrement the number of entries occupied at the pipe level */
  if (isTxn && !adt_tbl_data->backup_set) {
    adt_tbl_data->backup_num_entries_occupied =
        adt_tbl_data->num_entries_occupied;
    adt_tbl_data->backup_set = true;
  }
  if (adt_tbl_data->num_entries_occupied > 0) {
    adt_tbl_data->num_entries_occupied--;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_update_state_for_entry_modify(
    pipe_mgr_adt_t *adt,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mgr_adt_move_list_t *move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_entry_info_t *adt_entry_info = NULL;
  adt_entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (adt_entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;
  adt_data_resources_t *resources = NULL;
  /* Cache the current entry data */
  move_list_node->old_data = adt_entry_info->entry_data;
  if (action_spec->resource_count) {
    resources = (adt_data_resources_t *)PIPE_MGR_CALLOC(
        action_spec->resource_count, sizeof(adt_data_resources_t));
    if (resources == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    int i = 0;
    for (i = 0; i < action_spec->resource_count; i++) {
      resources[i].tbl_hdl = action_spec->resources[i].tbl_hdl;
      resources[i].tbl_idx = action_spec->resources[i].tbl_idx;
    }
  }
  int is_const = 0;
  adt_entry_info->entry_data = make_adt_ent_data(act_data_spec,
                                                 act_fn_hdl,
                                                 action_spec->resource_count,
                                                 is_const,
                                                 resources);
  if (adt_entry_info->entry_data == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting adt entry data for entry hdl %d, tbl 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  move_list_node->entry_hdl = adt_ent_hdl;
  move_list_node->data = adt_entry_info->entry_data;
  move_list_node->op = PIPE_ADT_UPDATE_MOD;
  move_list_node->pipe_id = adt_entry_info->pipe_id;
err_cleanup:
  if (resources) {
    PIPE_MGR_FREE(resources);
  }
  return status;
}

pipe_status_t pipe_mgr_adt_update_state_for_entry_placement(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_mgr_adt_entry_info_t *adt_entry_info,
    pipe_adt_ent_idx_t *entry_idx,
    bool do_placement) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_stage_location_t *location =
      adt_entry_info->sharable_stage_location;
  while (location) {
    if (location->stage_id == adt_stage_info->stage_id) {
      if (do_placement) {
        *entry_idx = location->entry_idx;
      }
      location->ref_count++;
      adt_entry_info->num_references++;
      return PIPE_SUCCESS;
    }
    location = location->next;
  }
  location = (pipe_mgr_adt_stage_location_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_adt_stage_location_t));
  if (location == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  location->stage_id = adt_stage_info->stage_id;
  if (do_placement) {
    status = pipe_mgr_adt_ent_place(adt, adt_stage_info, &location->entry_idx);
    if (status != PIPE_SUCCESS) {
      PIPE_MGR_FREE(location);
      return status;
    }
    *entry_idx = location->entry_idx;
  } else {
    location->entry_idx = *entry_idx;
    status = pipe_mgr_adt_ent_reserve(adt_stage_info, location->entry_idx);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in reserving entry idx %d for stage id %d, tbl 0x%x, "
          "err %s",
          __func__,
          __LINE__,
          location->entry_idx,
          adt_stage_info->stage_id,
          adt->adt_tbl_hdl,
          pipe_str_err(status));
      PIPE_MGR_FREE(location);
      return status;
    }
  }
  location->ref_count = 1;
  BF_LIST_DLL_PP(adt_entry_info->sharable_stage_location, location, next, prev);
  adt_entry_info->num_references++;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_update_state_for_entry_activate(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx,
    bool *exists) {
  bool found = false;
  pipe_mgr_adt_stage_location_t *location = entry_info->sharable_stage_location;
  while (location) {
    if (location->stage_id == stage_id) {
      if (location->entry_idx != entry_idx) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      found = true;
      location->ref_count++;
      break;
    }
    location = location->next;
  }
  *exists = found;
  if (!found) {
    location = (pipe_mgr_adt_stage_location_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_adt_stage_location_t));
    if (!location) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    location->stage_id = stage_id;
    location->entry_idx = entry_idx;
    location->ref_count = 1;
    BF_LIST_DLL_PP(entry_info->sharable_stage_location, location, next, prev);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_update_state_for_entry_deactivate(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx,
    bool *location_free) {
  pipe_mgr_adt_stage_location_t *location = entry_info->sharable_stage_location;
  while (location) {
    if (location->stage_id == stage_id) {
      if (location->entry_idx != entry_idx) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      if (location->ref_count == 0) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      location->ref_count--;
      if (location->ref_count == 0) {
        BF_LIST_DLL_REM(
            entry_info->sharable_stage_location, location, next, prev);
        PIPE_MGR_FREE(location);
        (*location_free) = true;
      }
      return PIPE_SUCCESS;
    }
    location = location->next;
  }
  PIPE_MGR_DBGCHK(0);
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_adt_update_state_for_entry_install(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx) {
  pipe_mgr_adt_stage_location_t *location =
      (pipe_mgr_adt_stage_location_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_adt_stage_location_t));
  if (location == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  location->stage_id = stage_id;
  location->entry_idx = entry_idx;
  BF_LIST_DLL_PP(entry_info->non_sharable_stage_location, location, next, prev);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_update_state_for_entry_uninstall(
    pipe_mgr_adt_entry_phy_info_t *entry_info,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx) {
  pipe_mgr_adt_stage_location_t *location =
      entry_info->non_sharable_stage_location;
  while (location) {
    if (location->stage_id == stage_id && location->entry_idx == entry_idx) {
      BF_LIST_DLL_REM(
          entry_info->non_sharable_stage_location, location, next, prev);
      PIPE_MGR_FREE(location);
      return PIPE_SUCCESS;
    }
    location = location->next;
  }
  PIPE_MGR_DBGCHK(0);
  return PIPE_OBJ_NOT_FOUND;
}

static pipe_status_t pipe_mgr_adt_update_state_for_entry_remove(
    pipe_mgr_adt_entry_info_t *entry_info,
    pipe_mgr_adt_stage_info_t *adt_stage_info) {
  pipe_mgr_adt_stage_location_t *location = entry_info->sharable_stage_location;
  while (location) {
    if (location->stage_id == adt_stage_info->stage_id) {
      if (location->ref_count > 0) {
        location->ref_count--;
      } else {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      if (entry_info->num_references > 0) {
        entry_info->num_references--;
      } else {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      if (location->ref_count == 0) {
        BF_LIST_DLL_REM(
            entry_info->sharable_stage_location, location, next, prev);
        pipe_mgr_adt_release_ent_idx(adt_stage_info, location->entry_idx);
        PIPE_MGR_FREE(location);
        return PIPE_SUCCESS;
      }
    }
    location = location->next;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_compute_ram_shadow_copy(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_idx_t entry_idx,
    uint32_t *num_ram_units,
    mem_id_t **mem_id_arr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t ram_line_num = 0;
  mem_id_t mem_id = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint8_t num_entries_per_wide_word = 0;
  uint8_t num_rams_in_wide_word = 0;
  uint8_t wide_word_blk_idx = 0;
  uint8_t **shadow_ptrs = NULL;
  bf_dev_pipe_t pipe_id = 0;

  shadow_ptrs = adt_stage_info->shadow_ptr_arr;
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;
  wide_word_blk_idx = entry_idx / num_entries_per_wide_word_blk;
  ram_line_num = (entry_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  if (adt_tbl_data->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = adt->lowest_pipe_id;
  } else {
    pipe_id = adt_tbl_data->pipe_id;
  }
  unsigned i = 0;
  *num_ram_units = num_rams_in_wide_word;
  *mem_id_arr = ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id;
  for (i = 0; i < num_rams_in_wide_word; i++) {
    mem_id = ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id[i];
    status = pipe_mgr_phy_mem_map_get_ref(adt->dev_id,
                                          adt->direction,
                                          pipe_mem_type_unit_ram,
                                          pipe_id,
                                          adt_stage_info->stage_id,
                                          mem_id,
                                          ram_line_num,
                                          &shadow_ptrs[i],
                                          false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s Error in getting shadow memory ref for mem id %d, stage id %d"
          " error %s ADT %s (0x%x)",
          __func__,
          mem_id,
          adt_stage_info->stage_id,
          pipe_str_err(status),
          adt->name,
          adt->adt_tbl_hdl);
      return status;
    }
    if (!shadow_ptrs[i]) {
      LOG_ERROR(
          "%s No shadow mem ref, dev %d pipe %d stage %d unit %d line %d ADT "
          "%s (0x%x)",
          __func__,
          adt->dev_id,
          pipe_id,
          adt_stage_info->stage_id,
          mem_id,
          ram_line_num,
          adt->name,
          adt->adt_tbl_hdl);
      PIPE_MGR_DBGCHK(shadow_ptrs[i]);
      return PIPE_UNEXPECTED;
    }
  }
  status = pipe_mgr_adt_tof_encode_entry(
      adt, adt_stage_info, act_fn_hdl, act_data_spec, entry_idx, shadow_ptrs);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in encoding action data entry for entry idx %d, tbl 0x%x, "
        "device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        entry_idx,
        adt->adt_tbl_hdl,
        adt->dev_id,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id,
        pipe_str_err(status));
    return status;
  } else if (bf_sys_log_is_log_enabled(BF_MOD_PIPE, BF_LOG_DBG) == 1) {
    unsigned j = 0;
    char buf[200];
    char *end = buf + 200;
    LOG_DBG("Encoded Action data entry : ");
    for (j = 0; j < ram_alloc_info->num_rams_in_wide_word; j++) {
      char *ptr = buf;
      /* Print every ram word */
      LOG_DBG("Word %d", j);
      for (i = 0; i < TOF_SRAM_UNIT_WIDTH / 8; i++) {
        /* Print every byte */
        ptr += snprintf(
            ptr, end > ptr ? (end - ptr) : 0, "%02x ", *(shadow_ptrs[j] + i));
      }
      snprintf(ptr, end > ptr ? (end - ptr) : 0, "\n");
      LOG_DBG("%s", buf);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_execute_entry_add(
    pipe_mgr_adt_t *adt,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    bf_dev_pipe_t pipe_id,
    pipe_mgr_adt_move_list_t *move_list_node) {
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = adt_ent_hdl;
  entry_info = (pipe_mgr_adt_entry_phy_info_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_adt_entry_phy_info_t));
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting action table instance for "
        " pipe id %d",
        __func__,
        __LINE__,
        pipe_id);
    PIPE_MGR_FREE(entry_info);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info->pipe_id = pipe_id;
  entry_info->entry_data =
      make_adt_ent_data(unpack_adt_ent_data_ad(move_list_node->data),
                        unpack_adt_ent_data_afun_hdl(move_list_node->data),
                        unpack_adt_ent_data_num_resources(move_list_node->data),
                        unpack_adt_ent_data_const(move_list_node->data),
                        unpack_adt_ent_data_resources(move_list_node->data));
  map_sts =
      bf_map_add(&adt_tbl_data->entry_phy_info_htbl, key, (void *)entry_info);
  if (map_sts == BF_MAP_KEY_EXISTS) {
    free_adt_ent_data(entry_info->entry_data);
    PIPE_MGR_FREE(entry_info);
    return PIPE_ALREADY_EXISTS;
  } else if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in adding entry info for hdl %x, tbl 0x%x, device id %d, "
        "err 0x%x",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id,
        map_sts);
    PIPE_MGR_FREE(entry_info);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  adt_tbl_data->num_entries_llp_occupied++;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_write_atomic_mod_sram_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    dev_stage_t stage_id) {
  /* Only TF2 and TF3 have shadow registers per SRAM for atomic updates which
   * require a special instruction to commit their data back to the SRAM. */
  if (adt->dev_info->dev_family == BF_DEV_FAMILY_TOFINO2 ||
      adt->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    pipe_atomic_mod_sram_instr_t instr;
    construct_instr_atomic_mod_sram(adt->dev_id, &instr, adt->direction);
    return pipe_mgr_drv_ilist_add(&sess_hdl,
                                  adt->dev_info,
                                  &adt_tbl_data->pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_atomic_mod_sram_instr_t));
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_execute_entry_modify(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_adt_t *adt,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_mgr_adt_move_list_t *move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_mgr_adt_ent_data_t *old_data = NULL;
  pipe_mgr_adt_stage_location_t *location = NULL;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  uint8_t prev_stage_id = 0xff;
  bool non_sharable_done = false;
  uint32_t num_ram_units = 0;
  mem_id_t *mem_id_arr = NULL;
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt->adt_tbl_hdl,
              adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  old_data = entry_info->entry_data;
  act_data_spec = unpack_adt_ent_data_ad(move_list_node->data);
  act_fn_hdl = unpack_adt_ent_data_afun_hdl(move_list_node->data);
  entry_info->entry_data =
      make_adt_ent_data(act_data_spec,
                        act_fn_hdl,
                        unpack_adt_ent_data_num_resources(move_list_node->data),
                        unpack_adt_ent_data_const(move_list_node->data),
                        unpack_adt_ent_data_resources(move_list_node->data));
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, entry_info->pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        adt->adt_tbl_hdl,
        adt->dev_id,
        entry_info->pipe_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_OBJ_NOT_FOUND;
    goto cleanup;
  }
  /* For each location in which this entry handle is present re-program the
   * entry with the latest action data spec
   */
  location = entry_info->sharable_stage_location;
  if (!location) {
    location = entry_info->non_sharable_stage_location;
    non_sharable_done = true;
  }
  while (location) {
    if (!adt_stage_info || location->stage_id != prev_stage_id) {
      adt_stage_info =
          pipe_mgr_adt_get_stage_info(adt_tbl_data, location->stage_id);
      if (adt_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Adt stage info for tbl 0x%x, pipe id %d, stage id %d "
            "not "
            "found",
            __func__,
            __LINE__,
            adt->adt_tbl_hdl,
            entry_info->pipe_id,
            location->stage_id);
        PIPE_MGR_DBGCHK(0);
        status = PIPE_OBJ_NOT_FOUND;
        goto cleanup;
      }
    }
    prev_stage_id = location->stage_id;
    status = pipe_mgr_adt_compute_ram_shadow_copy(adt,
                                                  adt_tbl_data,
                                                  adt_stage_info,
                                                  act_data_spec,
                                                  act_fn_hdl,
                                                  location->entry_idx,
                                                  &num_ram_units,
                                                  &mem_id_arr);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in computing ram shadow copy for entry idx %d, tbl "
          "0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          location->entry_idx,
          adt->adt_tbl_hdl,
          adt->dev_id,
          entry_info->pipe_id,
          location->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto cleanup;
    }
    status = pipe_mgr_adt_program_entry(sess_hdl,
                                        adt,
                                        adt_tbl_data,
                                        adt_stage_info,
                                        location->entry_idx,
                                        adt_stage_info->shadow_ptr_arr,
                                        mem_id_arr,
                                        num_ram_units,
                                        true);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in programming action data entry for entry idx %d, "
          "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          location->entry_idx,
          adt->adt_tbl_hdl,
          adt->dev_id,
          entry_info->pipe_id,
          location->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto cleanup;
    }

    status = pipe_mgr_adt_write_atomic_mod_sram_instr(
        sess_hdl, adt, adt_tbl_data, adt_stage_info->stage_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in commiting action data entry update for entry idx "
          "%d, tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          location->entry_idx,
          adt->adt_tbl_hdl,
          adt->dev_id,
          entry_info->pipe_id,
          location->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto cleanup;
    }
    location = location->next;
    if (!location && !non_sharable_done) {
      location = entry_info->non_sharable_stage_location;
      non_sharable_done = true;
    }
  }

  if (old_data) {
    free_adt_ent_data(old_data);
  }
  return PIPE_SUCCESS;

cleanup:
  if (entry_info->entry_data) {
    free_adt_ent_data(entry_info->entry_data);
  }
  entry_info->entry_data = old_data;
  return status;
}

static pipe_status_t pipe_mgr_adt_execute_entry_del(
    pipe_mgr_adt_t *adt, pipe_adt_ent_hdl_t adt_ent_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = adt_ent_hdl;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;

  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, adt_ent_hdl);
  if (!adt_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  map_sts = bf_map_get_rmv(
      &adt_tbl_data->entry_phy_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, devce id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt->adt_tbl_hdl,
              adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  } else if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry info for entry %d, tbl 0x%x, device id "
        "%d, err %s",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  if (entry_info->sharable_stage_location ||
      entry_info->non_sharable_stage_location) {
    LOG_ERROR(
        "%s:%d Entry handle %d, for tbl 0x%x, device id %d, cannot be delete "
        "as it is still being referenced by match entries",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    /* Put the entry back into the hash table */
    map_sts =
        bf_map_add(&adt_tbl_data->entry_phy_info_htbl, key, (void *)entry_info);
    PIPE_MGR_DBGCHK(map_sts == BF_MAP_OK);
    return PIPE_ENTRY_REFERENCES_EXIST;
  }
  if (entry_info->entry_data) {
    free_adt_ent_data(entry_info->entry_data);
  }

  if (adt_tbl_data->num_entries_llp_occupied > 0) {
    adt_tbl_data->num_entries_llp_occupied--;
  }
  PIPE_MGR_FREE(entry_info);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_adt_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    struct pipe_mgr_adt_move_list_t *move_list,
    uint32_t *processed) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t num_successful = 0;
  pipe_mgr_adt_move_list_t *traverser = move_list;
  pipe_mgr_adt_t *adt = NULL;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  bf_dev_pipe_t pipe_id = 0;
  *processed = 0;
  adt = pipe_mgr_adt_get(dev_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Action data table for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  while (traverser) {
    adt_ent_hdl = traverser->entry_hdl;
    switch (traverser->op) {
      case PIPE_ADT_UPDATE_ADD:
        pipe_id = traverser->pipe_id;
        status = pipe_mgr_adt_execute_entry_add(
            adt, adt_ent_hdl, pipe_id, traverser);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in executing entry add for entry %d, tbl 0x%x, "
              "device id %d, pipe id %d, err %s",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              dev_id,
              pipe_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      case PIPE_ADT_UPDATE_MOD:
        status = pipe_mgr_adt_execute_entry_modify(
            sess_hdl, adt, adt_ent_hdl, traverser);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in executing entry modify for entry %d, tbl 0x%x, "
              "device id %d, pipe id %d, err %s",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              dev_id,
              pipe_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      case PIPE_ADT_UPDATE_DEL:
        status = pipe_mgr_adt_execute_entry_del(adt, adt_ent_hdl);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in executing entry delete for entry %d, tbl 0x%x, "
              "device id %d, err %s",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      default:
        LOG_ERROR("%s:%d Invalid operation %d for tbl 0x%x, device id %d",
                  __func__,
                  __LINE__,
                  traverser->op,
                  adt_tbl_hdl,
                  dev_id);
        PIPE_MGR_DBGCHK(0);
        break;
    }
    traverser = traverser->next;
  }
  *processed = num_successful;
  return PIPE_SUCCESS;
}

/* Supports fetching either entry index or handle in a single call - not both.
 * If ent_hdl is non-zero it is assumed to be a valid entry handle, mbr_id is
 * ignored and adt_mbr_id and adt_mbr_pipe are populated.  If ent_hdl is zero
 * then mbr_id and the pipe-id from the dev_tgt are used to lookup the handle
 * which maps to the mbr_id. */
pipe_status_t pipe_mgr_adt_get_mbr_id_hdl_int(
    bf_dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t ent_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_adt_mbr_id_t *adt_mbr_id,
    bf_dev_pipe_t *adt_mbr_pipe,
    pipe_mgr_adt_ent_data_t *adt_ent_data) {
  bool hdl_to_id = false;
  bool id_to_hdl = false;

  if (adt_mbr_id && adt_mbr_pipe && !adt_ent_hdl) {
    hdl_to_id = true;
  } else if (adt_ent_hdl && !adt_mbr_id && !adt_mbr_pipe) {
    id_to_hdl = true;
  } else {
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_adt_t *adt = NULL;
  adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the state for the action data table  with handle "
        "%d and device id %d",
        __func__,
        adt_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* If an entry handle was provided use the pipe-id from it.  Otherwise use the
   * pipe-id from the dev_tgt. */
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  if (hdl_to_id) {
    adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, ent_hdl);
  } else {
    if (adt->symmetric) {
      if (dev_tgt.dev_pipe_id != DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Invalid pipe id %d passed for a symmetric action profile "
            "table with handle 0x%x, device id %d",
            __func__,
            __LINE__,
            dev_tgt.dev_pipe_id,
            adt_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }
    } else {
      if (dev_tgt.dev_pipe_id == DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Invalid pipe id of all pipes passed for a asymmetric action "
            "profile table with handle 0x%x, device id %d",
            __func__,
            __LINE__,
            adt_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }
    }
    adt_tbl_data = pipe_mgr_get_adt_instance(adt, dev_tgt.dev_pipe_id);
  }
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting action table instance for tbl 0x%x, device id "
        "%d, pipe id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_adt_entry_info_t *entry_info;
  if (id_to_hdl) {
    bf_map_sts_t sts =
        bf_map_get(&adt_tbl_data->mbr_id_map, mbr_id, (void **)&entry_info);
    if (sts == BF_MAP_NO_KEY) return PIPE_OBJ_NOT_FOUND;
    if (sts != BF_MAP_OK) return PIPE_UNEXPECTED;
    *adt_ent_hdl = entry_info->handle;
  } else if (hdl_to_id) {
    bf_map_sts_t sts = bf_map_get(
        &adt_tbl_data->entry_info_htbl, ent_hdl, (void **)&entry_info);
    if (sts == BF_MAP_NO_KEY) return PIPE_OBJ_NOT_FOUND;
    if (sts != BF_MAP_OK) return PIPE_UNEXPECTED;
    *adt_mbr_id = entry_info->mbr_id;
    *adt_mbr_pipe = adt_tbl_data->pipe_id;
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (adt_ent_data) {
    *adt_ent_data = *entry_info->entry_data;
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_adt_ent_add:
 *         Allocates a new action data table entry and returns the entry handle
 *         for future reference.
 *
 *
 *  This function does not do a physical placement of the action data table
 *  entry in the pipe, since this action data table entry will be referenced
 *  by a match-table entry which will decide the stage of the pipe in which
 *  this action data entry needs to be placed. The table management code
 *  will call into the action data table manager to bind the action data
 *  table entry to the match-table entry at which point the entry will
 *  actually be placed.
 *
 *  \param sess_hdl The session handle associated with this call.
 *  \param dev_tgt  Device target for this call.
 *  \param adt_tbl_hdl  The action data table handle.
 *  \param action_spec  The action data spec object.
 *  \param act_fn_handle  The action function handle.
 *  \param mbr_id  The member id of added entry.
 *  \param adt_ent_hdl_p A pointer to the action data table entry handle where
 *                       the entry handle needs to be filled-in.
 *  \param pipe_api_flags Flags for this request.
 *  \param move_list list supporting table operations.
 *  \return pipe_status_t The status of this operation.
 */

pipe_status_t pipe_mgr_adt_mgr_ent_add(
    bf_dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;

  pipe_adt_ent_hdl_t action_entry_hdl;
  bool is_txn = pipe_api_flags & PIPE_MGR_TBL_API_TXN;
  bool is_const = pipe_api_flags & PIPE_MGR_TBL_API_CONST_ENT;

  adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the state for the action data table "
        " with handle %d and device id %d",
        __func__,
        adt_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (adt->symmetric) {
    if (dev_tgt.dev_pipe_id != DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for a symmetric action profile "
          "table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          adt_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  } else {
    if (dev_tgt.dev_pipe_id == DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id of all pipes passed for a asymmetric action "
          "profile table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          adt_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }

  if (adt->cache_id == false) {
    /* Once turned on, cannot be turned off. */
    adt->cache_id =
        (pipe_api_flags & PIPE_MGR_TBL_API_CACHE_ENT_ID) ? true : false;
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt, dev_tgt.dev_pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting action table instance for tbl 0x%x, device id "
        "%d, pipe id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* First, check if there is space available in the table */
  if (adt_tbl_data->num_entries_occupied == adt_tbl_data->num_entries) {
    LOG_ERROR(
        "%s:%d No space in action data tbl 0x%x, device id %d, pipe id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (adt->cache_id) {
    /* Check if member id is already in use.*/
    pipe_mgr_adt_entry_info_t *tmp;
    bf_map_sts_t sts =
        bf_map_get(&adt_tbl_data->mbr_id_map, mbr_id, (void **)&tmp);
    if (sts != BF_MAP_NO_KEY) {
      LOG_ERROR(
          "%s : Entry with member index %d already exists in %s (0x%x) on dev "
          "%d pipe %X",
          __func__,
          mbr_id,
          adt->name,
          adt_tbl_hdl,
          adt->dev_id,
          adt_tbl_data->pipe_id);
      return PIPE_ALREADY_EXISTS;
    }
  }

  action_entry_hdl = pipe_mgr_adt_get_new_entry_hdl(adt_tbl_data);
  if (action_entry_hdl == PIPE_ADT_ENT_HDL_INVALID_HDL) {
    LOG_ERROR(
        "%s : Could not allocate a new entry handle for the action "
        " data table %s (0x%x) on device id %d pipe 0x%X",
        __func__,
        adt->name,
        adt_tbl_hdl,
        dev_tgt.device_id,
        adt_tbl_data->pipe_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* If warm init is in progress - i,e.. API replay, then need to lookup if this
   * entry is already present
   */
  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    pipe_mat_ent_hdl_t ha_entry_hdl = -1;
    pipe_mgr_adt_pipe_ha_hlp_info_t *ha_hlp_info = adt_tbl_data->ha_hlp_info;
    pipe_mgr_spec_map_t *spec_map = &ha_hlp_info->spec_map;
    /* If the passed in spec is NOT of the max size create a temporary spec that
     * is of the max size to be used in the lookup.  The lookup-spec function
     * requires the action_data_bits array to be of the maximum size. */
    pipe_action_data_spec_t tmp_spec, *lkup_spec = NULL;
    uint8_t *tmp_bits = NULL;
    if (act_data_spec->num_action_data_bytes < adt->max_act_data_size) {
      tmp_bits = PIPE_MGR_CALLOC(adt->max_act_data_size, sizeof(uint8_t));
      if (!tmp_bits) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
        return PIPE_NO_SYS_RESOURCES;
      }
      PIPE_MGR_MEMCPY(
          &tmp_spec, act_data_spec, sizeof(pipe_action_data_spec_t));
      PIPE_MGR_MEMCPY(tmp_bits,
                      act_data_spec->action_data_bits,
                      act_data_spec->num_action_data_bytes);
      tmp_spec.action_data_bits = tmp_bits;
      tmp_spec.num_action_data_bytes = adt->max_act_data_size;
      lkup_spec = &tmp_spec;
    } else {
      lkup_spec = act_data_spec;
    }

    status = pipe_mgr_hitless_ha_lookup_adt_spec(spec_map,
                                                 lkup_spec,
                                                 act_fn_hdl,
                                                 action_entry_hdl,
                                                 &ha_entry_hdl,
                                                 adt->max_act_data_size);
    /* Do a little cleanup now since we are done with the temporary action spec
     * variables. */
    if (tmp_bits) {
      PIPE_MGR_FREE(tmp_bits);
      tmp_bits = NULL;
    }
    lkup_spec = NULL;
    if (status == PIPE_OBJ_NOT_FOUND) {
      /* If the spec is not found, then it is either a newly added action or
       * one that was never programmed through a match entry or selector.
       * Either way, we should add this entry. */
      LOG_TRACE(
          "Dev %d tbl %s 0x%x pipe %x Add (replay new) of member %d with "
          "act_fn %d",
          dev_tgt.device_id,
          adt->name,
          adt->adt_tbl_hdl,
          dev_tgt.dev_pipe_id,
          action_entry_hdl,
          act_fn_hdl);
      pipe_mgr_entry_format_log_action_spec(dev_tgt.device_id,
                                            BF_LOG_DBG,
                                            adt->profile_id,
                                            action_spec,
                                            act_fn_hdl);
    } else if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error looking up action data spec during API relay at HLP "
          "for adt tbl %s 0x%x device id %d pipe %x err %s",
          __func__,
          __LINE__,
          adt->name,
          adt_tbl_hdl,
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          pipe_str_err(status));
      pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
      return status;
    } else {
      LOG_DBG(
          "Dev %d tbl %s 0x%x pipe %x Add (replay) member %d with act_fn %d",
          dev_tgt.device_id,
          adt->name,
          adt->adt_tbl_hdl,
          dev_tgt.dev_pipe_id,
          ha_entry_hdl,
          act_fn_hdl);
      pipe_mgr_entry_format_log_action_spec(dev_tgt.device_id,
                                            BF_LOG_DBG,
                                            adt->profile_id,
                                            action_spec,
                                            act_fn_hdl);
    }
    if (ha_entry_hdl != action_entry_hdl) {
      /* Release the allocated entry handle */
      pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
      /* Set the matched up entry handle */
      bf_id_allocator_set(adt_tbl_data->ent_hdl_allocator,
                          PIPE_GET_HDL_VAL(ha_entry_hdl));
      *adt_ent_hdl_p = ha_entry_hdl;
      LOG_DBG("Updating member handle from %d to %d",
              action_entry_hdl,
              ha_entry_hdl);
      if (adt->cache_id) {
        pipe_mgr_adt_entry_info_t *entry_info;
        bf_map_sts_t sts = bf_map_get(
            &adt_tbl_data->entry_info_htbl, ha_entry_hdl, (void **)&entry_info);
        if (sts != BF_MAP_OK) {
          LOG_ERROR(
              "%s : Not able to get entry_info for action "
              " data table with handle 0x%x, entry hdl 0x%x and device id %d",
              __func__,
              adt_tbl_hdl,
              ha_entry_hdl,
              dev_tgt.device_id);
          return PIPE_UNEXPECTED;
        }
        entry_info->mbr_id = mbr_id;
        sts = bf_map_add(&adt_tbl_data->mbr_id_map, mbr_id, (void *)entry_info);
        if (sts != BF_MAP_OK) {
          LOG_ERROR(
              "%s : Not able to add entry_info to mbr map for action "
              " data table with handle 0x%x, mbr_id %d and device id %d",
              __func__,
              adt_tbl_hdl,
              mbr_id,
              dev_tgt.device_id);
          return PIPE_UNEXPECTED;
        }
      }
    } else {
      *adt_ent_hdl_p = action_entry_hdl;

      /* Update state for the newly allocated entry hdl */
      PIPE_MGR_ASSERT(*move_list == NULL);
      pipe_mgr_adt_move_list_t *move_list_node =
          alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_ADD);
      if (move_list_node == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
        return PIPE_NO_SYS_RESOURCES;
      }
      if (*move_list == NULL) {
        *move_list = move_list_node;
      } else {
        (*move_list)->next = move_list_node;
      }
      status = pipe_mgr_adt_update_state_for_new_entry(adt,
                                                       adt_tbl_data,
                                                       *adt_ent_hdl_p,
                                                       mbr_id,
                                                       is_const,
                                                       action_spec,
                                                       act_fn_hdl,
                                                       move_list_node);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in updating state for action data entry for table with "
            "handle %d, entry handle %d",
            __func__,
            adt->adt_tbl_hdl,
            *adt_ent_hdl_p);
        pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
        return status;
      }
    }
    return PIPE_SUCCESS;
  }

  if (is_txn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, action_entry_hdl);

    if (status != PIPE_SUCCESS) {
      pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
      return status;
    }

    if (!adt_tbl_data->backup_set) {
      adt_tbl_data->backup_num_entries_occupied =
          adt_tbl_data->num_entries_occupied;
      adt_tbl_data->backup_set = true;
    }
  }

  if (*move_list != NULL) {
    PIPE_MGR_DBGCHK(0);
    pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_adt_move_list_t *move_list_node =
      alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_ADD);
  if (move_list_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }

  status = pipe_mgr_adt_update_state_for_new_entry(adt,
                                                   adt_tbl_data,
                                                   action_entry_hdl,
                                                   mbr_id,
                                                   is_const,
                                                   action_spec,
                                                   act_fn_hdl,
                                                   move_list_node);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "Dev %d pipe %X error updating state for action data entry in table %s "
        "(0x%x), %s",
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id,
        adt->name,
        adt->adt_tbl_hdl,
        pipe_str_err(status));
    pipe_mgr_adt_release_entry_hdl(adt_tbl_data, action_entry_hdl);
    free_adt_move_list(&move_list_node);
    return status;
  }

  if (*move_list == NULL) {
    *move_list = move_list_node;
  } else {
    (*move_list)->next = move_list_node;
  }
  *adt_ent_hdl_p = action_entry_hdl;

  LOG_DBG("Dev %d tbl %s 0x%x pipe %x Add member %d with act_fn %d",
          dev_tgt.device_id,
          adt->name,
          adt->adt_tbl_hdl,
          dev_tgt.dev_pipe_id,
          action_entry_hdl,
          act_fn_hdl);
  pipe_mgr_entry_format_log_action_spec(
      dev_tgt.device_id, BF_LOG_DBG, adt->profile_id, action_spec, act_fn_hdl);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_add_init_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    struct pipe_mgr_adt_move_list_t **move_list) {
  pipe_mgr_adt_t *adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (adt == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_adt_data_t *inst;
  inst = pipe_mgr_get_adt_instance(adt, dev_tgt.dev_pipe_id);
  if (inst == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Find a high numbered id which is not yet in use.  We pick a large number
   * in an attempt to avoid conflict with user selected ids added after init. */
  pipe_adt_mbr_id_t mbr_id = 0xFFFFFFFF;
  bf_map_sts_t msts = BF_MAP_OK;
  while (mbr_id > 0) {
    unsigned long key = mbr_id;
    void *unused;
    msts = bf_map_get(&inst->mbr_id_map, key, &unused);
    if (msts == BF_MAP_NO_KEY) break;
    --mbr_id;
  }
  if (msts != BF_MAP_NO_KEY) {
    /* All ids were used in the map?!?  Unexpected... */
    PIPE_MGR_DBGCHK(msts == BF_MAP_NO_KEY);
    return PIPE_UNEXPECTED;
  }

  uint32_t pipe_api_flags = PIPE_MGR_TBL_API_CONST_ENT;
  pipe_status_t sts = pipe_mgr_adt_mgr_ent_add(dev_tgt,
                                               adt_tbl_hdl,
                                               action_spec,
                                               act_fn_hdl,
                                               mbr_id,
                                               adt_ent_hdl_p,
                                               pipe_api_flags,
                                               move_list);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }

  /* Save the mbr-id in the map of init time entries. */
  msts = bf_map_add(&inst->const_init_entries,
                    (unsigned long)mbr_id,
                    (void *)(uintptr_t)mat_tbl_hdl);
  if (msts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error adding action entry const init info for entry handle %d, "
        "id 0x%x, table %s (0x%x) pipe %X dev %d: %d",
        __func__,
        __LINE__,
        *adt_ent_hdl_p,
        mbr_id,
        adt->name,
        adt->adt_tbl_hdl,
        inst->pipe_id,
        adt->dev_id,
        msts);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  return sts;
}

pipe_status_t pipe_mgr_adt_init_entry_hdl_get(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p) {
  pipe_mgr_adt_t *adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (adt == NULL) {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_adt_data_t *inst;
  inst = pipe_mgr_get_adt_instance(adt, dev_tgt.dev_pipe_id);
  if (inst == NULL) {
    return PIPE_INVALID_ARG;
  }

  unsigned long key;
  void *data = NULL;
  for (bf_map_sts_t msts =
           bf_map_get_first(&inst->const_init_entries, &key, &data);
       msts == BF_MAP_OK;
       msts = bf_map_get_next(&inst->const_init_entries, &key, &data)) {
    pipe_mat_tbl_hdl_t h = (uintptr_t)data;
    if (h != mat_tbl_hdl) continue;

    pipe_mgr_adt_entry_info_t *entry_info = NULL;
    msts = bf_map_get(&inst->mbr_id_map, key, (void **)&entry_info);
    if (msts != BF_MAP_OK) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    *adt_ent_hdl_p = entry_info->handle;
    return PIPE_SUCCESS;
  }
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_adt_get_plcmt_data(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    struct pipe_mgr_adt_move_list_t **move_list) {
  pipe_mgr_adt_t *adt = NULL;
  adt = pipe_mgr_adt_get(dev_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the state for the action data table "
        " with handle 0x%x and device id %d",
        __func__,
        adt_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_adt_move_list_t *ml_head = NULL, *ml_tail = NULL;
  pipe_mgr_adt_entry_info_t *ent_info = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data;
  unsigned long key;
  bf_map_sts_t ms;
  bf_map_t *htbl;
  uint32_t i;

  /* Walk through all table instances */
  for (i = 0; i < adt->num_tbls; i++) {
    adt_tbl_data = &adt->adt_tbl_data[i];
    htbl = &adt_tbl_data->entry_info_htbl;

    for (ms = bf_map_get_first(htbl, &key, (void **)&ent_info); ms == BF_MAP_OK;
         ms = bf_map_get_next(htbl, &key, (void **)&ent_info)) {
      pipe_adt_ent_hdl_t entry_hdl = key;

      /* Skip the internal entries. */
      if (adt_is_mbr_const(adt_tbl_data, ent_info->mbr_id)) continue;

      pipe_mgr_adt_move_list_t *ml =
          alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_ADD);
      if (!ml) {
        /* Clean up anything already allocated. */
        while (ml_head) {
          pipe_mgr_adt_move_list_t *x = ml_head;
          ml_head = ml_head->next;
          PIPE_MGR_FREE(x);
        }
        return PIPE_NO_SYS_RESOURCES;
      }

      ml->data = ent_info->entry_data;
      ml->entry_hdl = entry_hdl;
      ml->op = PIPE_ADT_UPDATE_ADD;
      ml->pipe_id = ent_info->pipe_id;
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

/** \brief pipe_mgr_adt_mgr_ent_del:
 *         Deletes the action data table entry pointed to by the passed in
 *         entry handle.
 *
 *  \param sess_hdl The session handle associated with this call.
 *  \param pipe_api_flags  Flags associated with this call.
 *  \param adt_tbl_hdl  The action data table handle.
 *  \param adt_ent_hdl  Action data table entry handle which is to be deleted.
 *  \return pipe_status_t The status of this operation.
 */

pipe_status_t pipe_mgr_adt_mgr_ent_del(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_move_list_t *move_list_node = NULL;
  bool isTxn = false;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  if (*move_list != NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* From the entry handle get the action-data table data */
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the state for the action data table with"
        " handle 0x%x and device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, adt_ent_hdl);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Adt table data for entry handle %d (0x%x), tbl %s (0x%x), "
        "device id %d not found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_ent_hdl,
        adt->name,
        adt->adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("Dev %d tbl %s 0x%x, delete entry %d",
            device_id,
            adt->name,
            adt->adt_tbl_hdl,
            adt_ent_hdl);

  move_list_node = alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_DEL);
  if (move_list_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.  */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);
    if (status != PIPE_SUCCESS) {
      free_adt_move_list(&move_list_node);
      return status;
    }
  }

  status = pipe_mgr_adt_update_state_for_entry_delete(
      adt, adt_tbl_data, adt_ent_hdl, move_list_node, isTxn);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in updating state for entry delete for hdl %d for table"
        " with handle 0x%x",
        __func__,
        adt_ent_hdl,
        adt->adt_tbl_hdl);
    free_adt_move_list(&move_list_node);
    *move_list = NULL;
    return status;
  }

  /* This entry is not being referred to by any entry, and hence safe to
   * to delete.  */
  *move_list = move_list_node;

  pipe_mgr_adt_release_entry_hdl(adt_tbl_data, adt_ent_hdl);
  return PIPE_SUCCESS;
}

/* pipe_mgr_adt_mgr_ent_modify : */
pipe_status_t pipe_mgr_adt_mgr_ent_modify(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_adt_move_list_t **move_list) {
  (void)move_list;
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  bool isTxn = false;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info for table "
        "with handle %d for device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (*move_list != NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_adt_move_list_t *move_list_node =
      alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_MOD);
  if (move_list_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (*move_list == NULL) {
    *move_list = move_list_node;
  } else {
    (*move_list)->next = move_list_node;
  }

  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);

    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

  status = pipe_mgr_adt_update_state_for_entry_modify(
      adt, adt_ent_hdl, action_spec, act_fn_hdl, move_list_node);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry modify for entry %d, tbl 0x%x "
        "device id %d, err %s",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        device_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  LOG_DBG("Dev %d tbl %s 0x%x Mod member %d to act_fn %d",
          device_id,
          adt->name,
          adt->adt_tbl_hdl,
          adt_ent_hdl,
          act_fn_hdl);
  pipe_mgr_entry_format_log_action_spec(
      device_id, BF_LOG_DBG, adt->profile_id, action_spec, act_fn_hdl);

  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_add(pipe_sess_hdl_t sess_hdl,
                              bf_dev_id_t device_id,
                              pipe_adt_tbl_hdl_t adt_tbl_hdl,
                              bf_dev_pipe_t pipe_id,
                              uint8_t stage_id,
                              pipe_act_fn_hdl_t act_fn_hdl,
                              pipe_action_data_spec_t *act_data_spec,
                              uint32_t direct_map_tbl_idx,
                              rmt_tbl_hdl_t stage_table_handle,
                              rmt_virt_addr_t *adt_virt_addr_p,
                              bool update) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  uint32_t num_ram_units = 0;
  mem_id_t *mem_id_arr = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info for table "
        "with handle 0x%x for device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data instance for tbl 0x%x, device id %d, pipe id %d not "
        "found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        adt->dev_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Get the stage level info */
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    /* It is possible for an action table to be missing in a stage if all data
     * made it to immediate fields. There is nothing to do in this case.
     */
    return PIPE_SUCCESS;
  }
  if (direct_map_tbl_idx >= adt_stage_info->num_entries) {
    LOG_ERROR(
        "%s : Error : Invalid entry index for action data entry add "
        "for action data table with handle 0x%x, stage id %d, index 0x%x",
        __func__,
        adt->adt_tbl_hdl,
        stage_id,
        direct_map_tbl_idx);
    return PIPE_INVALID_ARG;
  }
  /* Along with the direct addressed entry index, the sub tbl within the
   * stage is passed. The direct addressed entry index calculated by the
   * caller is not the global entyr index, but the local entry index within
   * the sub table index. Most notably, algorithmic TCAM utilizes this, since
   * a single P4 table may be split into multiple logical tables iby the
   * compiler
   * which is internally represented as a single logical table.
   * Hence offset it to arrive at the global entry index.
   */
  uint8_t sub_tbl_idx;
  for (sub_tbl_idx = 0; sub_tbl_idx < adt_stage_info->num_sub_tbls;
       sub_tbl_idx++) {
    if (adt_stage_info->sub_tbl_offsets[sub_tbl_idx].stage_table_handle ==
        stage_table_handle) {
      direct_map_tbl_idx += adt_stage_info->sub_tbl_offsets[sub_tbl_idx].offset;
      break;
    }
  }
  if (sub_tbl_idx == adt_stage_info->num_sub_tbls) {
    LOG_ERROR(
        "%s:%d Invalid stage table handle %d passed for action tabl 0x%x"
        " device id %d, stage id %d, num sub_tbls %d",
        __func__,
        __LINE__,
        stage_table_handle,
        adt_tbl_hdl,
        device_id,
        stage_id,
        adt_stage_info->num_sub_tbls);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  status = pipe_mgr_adt_compute_ram_shadow_copy(adt,
                                                adt_tbl_data,
                                                adt_stage_info,
                                                act_data_spec,
                                                act_fn_hdl,
                                                direct_map_tbl_idx,
                                                &num_ram_units,
                                                &mem_id_arr);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing ram shadow copy for entry idx %d, for tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        direct_map_tbl_idx,
        adt->adt_tbl_hdl,
        adt->dev_id,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Program the entry down to the hardware */
  status = pipe_mgr_adt_program_entry(sess_hdl,
                                      adt,
                                      adt_tbl_data,
                                      adt_stage_info,
                                      direct_map_tbl_idx,
                                      adt_stage_info->shadow_ptr_arr,
                                      mem_id_arr,
                                      num_ram_units,
                                      update);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in programming action data entry for entry idx %d, tbl "
        "0x%x, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        direct_map_tbl_idx,
        adt_tbl_hdl,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  if (adt_virt_addr_p) {
    status = pipe_adt_tof_generate_vaddr(
        adt_stage_info, direct_map_tbl_idx, adt_virt_addr_p);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in generating virtual address for idx %d, tbl 0x%x, "
          "device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          direct_map_tbl_idx,
          adt_tbl_hdl,
          adt->dev_id,
          adt_tbl_data->pipe_id,
          adt_stage_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_get_addr(bf_dev_id_t device_id,
                                   bf_dev_pipe_t pipe_id,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   uint8_t stage_id,
                                   pipe_idx_t entry_idx,
                                   rmt_virt_addr_t *adt_virt_addr_p) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_adt_ent_idx_t stage_ent_idx = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table state for table "
        "with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table data for pipe id "
        "%d for table handle %d, device id %d",
        __func__,
        pipe_id,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table stage data for "
        " table with handle %d, stage id %d, device id %d",
        __func__,
        adt_tbl_hdl,
        stage_id,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_ent_idx = entry_idx - adt_stage_info->stage_offset;
  status = pipe_adt_tof_generate_vaddr(
      adt_stage_info, stage_ent_idx, adt_virt_addr_p);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in generating virtual address for idx %d, tbl 0x%x, "
        "device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        stage_ent_idx,
        adt->adt_tbl_hdl,
        adt->dev_id,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_activate_stage(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         bf_dev_pipe_t pipe_id,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         uint8_t stage_id,
                                         pipe_idx_t entry_idx,
                                         rmt_virt_addr_t *adt_virt_addr_p,
                                         bool update) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  mem_id_t *mem_id_arr = NULL;
  uint32_t num_ram_units = 0;
  bool exists = false;
  pipe_adt_ent_idx_t stage_ent_idx = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table state for table "
        "with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (entry_info->pipe_id != pipe_id) {
    LOG_ERROR(
        "%s:%d ERROR : Action entry handle %d pipe-id %d is not the same as "
        "the "
        "pipe-id %d in which it is being activated.",
        __func__,
        __LINE__,
        adt_ent_hdl,
        entry_info->pipe_id,
        pipe_id);
    return PIPE_INVALID_ARG;
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table data for pipe id "
        "%d for table handle %d, device id %d",
        __func__,
        pipe_id,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table stage data for "
        " table with handle %d, stage id %d, device id %d",
        __func__,
        adt_tbl_hdl,
        stage_id,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_ent_idx = entry_idx - adt_stage_info->stage_offset;
  status = pipe_mgr_adt_update_state_for_entry_activate(
      entry_info, stage_id, stage_ent_idx, &exists);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry activate for entry %d, tbl "
        "0x%x, device id %d, pipe id %d, stage_id %d",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        adt->dev_id,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id);
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x Activate entry %d in stage %d at idx %d (%s)",
      device_id,
      adt->name,
      adt->adt_tbl_hdl,
      pipe_id,
      adt_ent_hdl,
      stage_id,
      entry_idx,
      exists ? "exists" : "program");

  act_data_spec = unpack_adt_ent_data_ad(entry_info->entry_data);
  act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
  /* Only if the entry did not exist earlier, need to program the entry down to
   * the hardware.
   */
  if (!exists) {
    status = pipe_mgr_adt_compute_ram_shadow_copy(adt,
                                                  adt_tbl_data,
                                                  adt_stage_info,
                                                  act_data_spec,
                                                  act_fn_hdl,
                                                  stage_ent_idx,
                                                  &num_ram_units,
                                                  &mem_id_arr);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in computing ram shadow copy for entry idx %d, for tbl "
          "0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          stage_ent_idx,
          adt->adt_tbl_hdl,
          adt->dev_id,
          adt_tbl_data->pipe_id,
          adt_stage_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    status = pipe_mgr_adt_program_entry(sess_hdl,
                                        adt,
                                        adt_tbl_data,
                                        adt_stage_info,
                                        stage_ent_idx,
                                        adt_stage_info->shadow_ptr_arr,
                                        mem_id_arr,
                                        num_ram_units,
                                        update);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in programming action data entry for entry %d, idx %d, "
          "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          adt_ent_hdl,
          stage_ent_idx,
          adt_tbl_hdl,
          adt->dev_id,
          adt_tbl_data->pipe_id,
          adt_stage_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    /* A new entry location was used up, increment the number of entries
     * programmed.
     */
    adt_tbl_data->num_entries_programmed++;
  }
  status = pipe_adt_tof_generate_vaddr(
      adt_stage_info, stage_ent_idx, adt_virt_addr_p);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in generating virtual address for idx %d, tbl 0x%x, "
        "device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        stage_ent_idx,
        adt->adt_tbl_hdl,
        adt->dev_id,
        adt_tbl_data->pipe_id,
        adt_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_deactivate_stage(bf_dev_pipe_t pipe_id,
                                           bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint8_t stage_id,
                                           pipe_idx_t entry_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_adt_ent_idx_t stage_ent_idx = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table state for action"
        " data table with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_TRACE("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              adt->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table instance for table"
        " handle %d, device id %d, pipe_id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, device id %d, pipe_id %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_ent_idx = entry_idx - adt_stage_info->stage_offset;

  bool location_free = false;
  status = pipe_mgr_adt_update_state_for_entry_deactivate(
      entry_info, stage_id, stage_ent_idx, &location_free);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry deactivate for entry %d, tbl "
        "0x%x, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        stage_ent_idx,
        adt_tbl_hdl,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  if (location_free) {
    PIPE_MGR_DBGCHK(adt_tbl_data->num_entries_programmed);
    if (adt_tbl_data->num_entries_programmed) {
      adt_tbl_data->num_entries_programmed--;
    }
  }

  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x Deactivate entry %d in stage %d at idx %d "
      "(%s)",
      device_id,
      adt->name,
      adt->adt_tbl_hdl,
      pipe_id,
      adt_ent_hdl,
      stage_id,
      entry_idx,
      location_free ? "deleted" : "remains");

  return PIPE_SUCCESS;
}

/* rmt_adt_ent_group_reserve : This API is used by the caller to just reserve
 * a block of CONTIGUOUS action data entries in the given stage, given pipe.
 * No action data is provided yet, and no write to the hardware is involved.
 * All that is done is a block of entries are set aside.
 *
 * If the passed in adt_base_idx is valid, the reserved block will begin at the
 * specified index. Otherwise, this function will search for a suitable block
 * and save the found index.
 */

pipe_status_t rmt_adt_ent_group_reserve(
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    /* The logical index of the base entry */
    pipe_adt_ent_idx_t *adt_base_idx,
    /* Number of entries to be reserved for the group */
    uint32_t num_entries_per_block,
    /* If num_blocks > 1, address should always be aligned with 128 (7 trailing
       zeroes) */
    uint32_t num_blocks,
    uint32_t pipe_api_flags) {
  (void)pipe_api_flags;
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  bool isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table state for "
        "table with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* If the action data table is asymmetric and PIPE_ID passed shouldn't
   * be BF_DEV_PIPE_ALL.
   */
  if (adt->symmetric == false) {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table instance for "
        "table with handle %d, device id %d, pipe id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, device id %d, pipe id %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (isTxn && !adt_stage_info->backup_entry_allocator) {
    adt_stage_info->backup_entry_allocator = (void *)power2_allocator_make_copy(
        (power2_allocator_t *)adt_stage_info->entry_allocator);
  }

  if (*adt_base_idx != PIPE_MGR_LOGICAL_ACT_IDX_INVALID) {
    /* State restore case. Update the given base idx before allocating */
    *adt_base_idx -= adt_stage_info->stage_offset;
  }

  status = pipe_mgr_adt_ent_reserve_contiguous(
      adt, adt_stage_info, num_entries_per_block, num_blocks, adt_base_idx);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in reserving action data table entries for "
        "table handle %d, device id %d, pipe id %d, stage id %d "
        "error %s",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }
  /* A global index needs to be returned */
  *adt_base_idx += adt_stage_info->stage_offset;

  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_install(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    /* Offset at which to install the entry */
    pipe_adt_ent_idx_t offset,
    bool hw_update) {
  /* This API does not support transactional semantics. It is the responsibility
   * of the caller to make the appropriate call to adhere to transaction
   * semantics.
   */
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  mem_id_t *mem_id_arr = NULL;
  uint32_t num_ram_units = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info for table with"
        " handle %d for device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data table instance for tbl 0x%x pipe id %d"
        " device id %d not found",
        __func__,
        __LINE__,
        adt->adt_tbl_hdl,
        pipe_id,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt->adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Get the stage level info */
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);

  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Offset passed is a global idx, reduce it to a stage local idx */
  offset -= adt_stage_info->stage_offset;
  status =
      pipe_mgr_adt_update_state_for_entry_install(entry_info, stage_id, offset);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry install at idx %d, entry %d, "
        "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        offset,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  if (hw_update) {
    act_data_spec = unpack_adt_ent_data_ad(entry_info->entry_data);
    act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
    status = pipe_mgr_adt_compute_ram_shadow_copy(adt,
                                                  adt_tbl_data,
                                                  adt_stage_info,
                                                  act_data_spec,
                                                  act_fn_hdl,
                                                  offset,
                                                  &num_ram_units,
                                                  &mem_id_arr);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in computing ram shadow copy for idx %d, entry %d, tbl "
          "0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          offset,
          adt_ent_hdl,
          adt_tbl_hdl,
          device_id,
          pipe_id,
          stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    status = pipe_mgr_adt_program_entry(sess_hdl,
                                        adt,
                                        adt_tbl_data,
                                        adt_stage_info,
                                        offset,
                                        adt_stage_info->shadow_ptr_arr,
                                        mem_id_arr,
                                        num_ram_units,
                                        false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in programming action data entry for hdl %d, idx %d, "
          "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          adt_ent_hdl,
          offset,
          adt_tbl_hdl,
          device_id,
          pipe_id,
          stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x Install member %d in stage %d at "
      "local-offset %u",
      device_id,
      adt->name,
      adt_tbl_hdl,
      pipe_id,
      adt_ent_hdl,
      stage_id,
      offset);

  /* This is the case of selector mgr installing an action entry */
  adt_tbl_data->num_entries_programmed++;
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_uninstall(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    /* Offset at which to install the entry */
    pipe_adt_ent_idx_t offset) {
  /* This API does not support transactional semantics. It is the responsibility
   * of the caller to make the appropriate call to adhere to transaction
   * semantics.
   */
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info for table with"
        " handle %d for device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);

  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data table instance for tbl 0x%x pipe id %d"
        " device id %d not found",
        __func__,
        __LINE__,
        adt->adt_tbl_hdl,
        pipe_id,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);

  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, device id %d, pipe_id %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Offset passed is a global idx, reduce it to a stage local idx */
  offset -= adt_stage_info->stage_offset;
  status = pipe_mgr_adt_update_state_for_entry_uninstall(
      entry_info, stage_id, offset);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry uninstall for idx %d, entry "
        "%d, tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        offset,
        adt_ent_hdl,
        adt_tbl_hdl,
        device_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  PIPE_MGR_DBGCHK(adt_tbl_data->num_entries_programmed);
  if (adt_tbl_data->num_entries_programmed) {
    adt_tbl_data->num_entries_programmed--;
  }

  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x Install member %d in stage %d at "
      "local-offset %u",
      device_id,
      adt->name,
      adt_tbl_hdl,
      pipe_id,
      adt_ent_hdl,
      stage_id,
      offset);

  return PIPE_SUCCESS;
}

/* API to delete a group of action data table entries pointed to by
 * action data table entry handle array passed.
 * NOTES:
 *      1) This API DOES not go down to the hardware to clear the
 *         entries. Rationale being, action data entries are driven
 *         by match entries, if a delete call is made, it is assumed that the
 *         corresponding match entry "manager" is doing so in its full wisdom.
 *
 *      2) This API just clears internal state to mark the entries
 *         FREE, thereby potentally allowing new entries to take the previously
 *         occupied places.
 *
 *      3) This API does not support transaction semantics. The caller needs
 *         to call rmt_adt_ent_group_reserve to reverse
 *
 */

pipe_status_t rmt_adt_ent_group_delete(
    uint8_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id, /* BF_DEV_PIPE_ALL for all pipes */
    uint8_t stage_id,
    /* The logical index of the base entry */
    pipe_idx_t adt_base_idx,
    /* Number of entries to be reserved for the group */
    uint32_t num_entries_per_block,
    /* If num_blocks > 1, address should always be aligned with 128 (7 trailing
       zeroes) */
    uint32_t num_blocks,
    uint32_t pipe_api_flags) {
  (void)pipe_api_flags;
  (void)num_entries_per_block;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  bool isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;
  int ret = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table state for "
        "table with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* If the action data table is asymmetric and PIPE_ID passed shouldn't
   * be BF_DEV_PIPE_ALL.
   */
  if (adt->symmetric == false) {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
  }

  adt_data = pipe_mgr_get_adt_instance(adt, pipe_id);

  if (adt_data == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table instance for "
        "table with handle %d, device id %d, pipe id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_data, stage_id);

  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, device id %d, pipe id %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* The base index passed is a global index, reduce it to a stage-local idx */
  adt_base_idx -= adt_stage_info->stage_offset;

  if (isTxn && !adt_stage_info->backup_entry_allocator) {
    adt_stage_info->backup_entry_allocator = (void *)power2_allocator_make_copy(
        (power2_allocator_t *)adt_stage_info->entry_allocator);
  }

  ret = power2_allocator_release_multiple(
      adt_stage_info->entry_allocator, adt_base_idx, num_blocks);

  if (ret != 0) {
    LOG_ERROR(
        "%s : Error in releasing allocated entry indices for "
        "action data table with handle %d, device id %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        device_id,
        stage_id);
    return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_adt_destroy_entry_idx_allocator_backup(
    pipe_mgr_adt_t *adt, pipe_mgr_adt_stage_info_t *adt_stage_info) {
  power2_allocator_t *power2_allocator = NULL;

  if (adt->ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
    PIPE_MGR_DBGCHK(0);
    return;
  }

  power2_allocator =
      (power2_allocator_t *)adt_stage_info->backup_entry_allocator;

  if (power2_allocator) {
    /* Destroy the backup iff it exists */
    power2_allocator_destroy(power2_allocator);
    adt_stage_info->backup_entry_allocator = NULL;
  }

  return;
}

void pipe_mgr_adt_restore_entry_idx_allocator_backup(
    pipe_mgr_adt_t *adt, pipe_mgr_adt_stage_info_t *adt_stage_info) {
  power2_allocator_t *power2_allocator = NULL;

  if (adt->ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
    PIPE_MGR_DBGCHK(0);
    return;
  }

  if (adt->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;
    /* First destroy the existing entry index allocator */
    power2_allocator_destroy(power2_allocator);

    power2_allocator =
        (power2_allocator_t *)adt_stage_info->backup_entry_allocator;
    /* Now copy from the backup */
    adt_stage_info->entry_allocator =
        (void *)power2_allocator_make_copy(power2_allocator);

    /* Now destroy the backup */
    power2_allocator_destroy(power2_allocator);

    adt_stage_info->backup_entry_allocator = NULL;
  }

  return;
}

uint32_t pipe_mgr_adt_get_num_hlp_entries(pipe_mgr_adt_t *adt) {
  /* The total number of entries occupied in the entire table
   * is the sum total of total number of entries occupied across
   * all "instances" of the table.
   */
  unsigned i = 0;
  uint32_t num_entries_occupied = 0;
  uint32_t num_const_init = 0;

  for (i = 0; i < adt->num_tbls; i++) {
    num_entries_occupied += adt->adt_tbl_data[i].num_entries_occupied;
    num_const_init += bf_map_count(&adt->adt_tbl_data[i].const_init_entries);
  }
  PIPE_MGR_DBGCHK(num_const_init <= num_entries_occupied);
  return num_entries_occupied - num_const_init;
}

uint32_t pipe_mgr_adt_get_num_llp_entries(pipe_mgr_adt_t *adt) {
  /* The total number of entries occupied in the entire table
   * is the sum total of total number of entries occupied across
   * all "instances" of the table.
   */
  unsigned i = 0;
  uint32_t num_entries_occupied = 0;
  uint32_t num_const_init = 0;

  for (i = 0; i < adt->num_tbls; i++) {
    num_entries_occupied +=
        bf_map_count(&adt->adt_tbl_data[i].entry_phy_info_htbl);
    num_const_init += bf_map_count(&adt->adt_tbl_data[i].const_init_entries);
  }
  PIPE_MGR_DBGCHK(num_const_init <= num_entries_occupied);
  return num_entries_occupied - num_const_init;
}

static uint32_t pipe_mgr_adt_stage_get_num_entries_occupied(
    pipe_mgr_adt_stage_info_t *adt_stage_info) {
  power2_allocator_t *power2_allocator = NULL;
  power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;
  return power2_allocator_usage(power2_allocator);
}

/* This function gives the number of action entries in software which have an
 * entry handle excluding the default entries (const init time entries).
 */
pipe_status_t pipe_mgr_adt_tbl_get_num_entries_placed(dev_target_t dev_tgt,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p) {
  unsigned i = 0;
  uint32_t num_entries_occupied = 0;
  uint32_t num_const_init = 0;
  pipe_mgr_adt_t *adt = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  /* This API is only applicable for indirectly referenced action tables */
  if (adt->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    LOG_ERROR(
        "%s:%d Number of entries placed in the action data table is the "
        "same as the number of entries placed in the associated match "
        "table for directly referenced action tbl 0x%x, device id %d. This API "
        "is only applicable for indirectly referenced action tables",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;

  for (i = 0; i < adt->num_tbls; i++) {
    adt_tbl_data = &adt->adt_tbl_data[i];
    if (dev_tgt.dev_pipe_id == adt_tbl_data->pipe_id ||
        dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
      /* In case of pure physical devices, we can only get pipe
       * specific data from llp_occupied */
      if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
        num_entries_occupied += adt_tbl_data->num_entries_occupied;
      } else {
        num_entries_occupied += adt_tbl_data->num_entries_llp_occupied;
      }
      num_const_init += bf_map_count(&adt_tbl_data->const_init_entries);
    }
  }
  PIPE_MGR_DBGCHK(num_const_init <= num_entries_occupied);
  *count_p = num_entries_occupied - num_const_init;
  LOG_TRACE("%s: Dev %d pipe %X %s has %d - %d (const init) = %d placed",
            __func__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            adt->name,
            num_entries_occupied,
            num_const_init,
            *count_p);
  return PIPE_SUCCESS;
}

/* This function gives the number of action entry locations that have
 * been consumed. This is the number of action profile entries placed plus
 * the number of action profile entries reserved by the selector mgr.
 */
pipe_status_t pipe_mgr_adt_tbl_get_num_entries_reserved(dev_target_t dev_tgt,
                                                        pipe_tbl_hdl_t tbl_hdl,
                                                        uint32_t *count_p) {
  unsigned i = 0, j = 0;
  uint32_t num_entries_occupied = 0;
  pipe_mgr_adt_t *adt = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* This API is only applicable for indirectly referenced action tables */
  if (adt->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    LOG_ERROR(
        "%s:%d Number of entries placed in the action data table is the "
        "same as the number of entries placed in the associated match "
        "table for directly referenced action tbl 0x%x, device id %d. This API "
        "is only applicable for indirectly referenced action tables",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  for (i = 0; i < adt->num_tbls; i++) {
    adt_tbl_data = &adt->adt_tbl_data[i];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == adt_tbl_data->pipe_id) {
      for (j = 0; j < adt_tbl_data->num_stages; j++) {
        adt_stage_info = &adt_tbl_data->adt_stage_info[j];
        num_entries_occupied +=
            pipe_mgr_adt_stage_get_num_entries_occupied(adt_stage_info);
      }
    }
  }
  *count_p = num_entries_occupied;
  return PIPE_SUCCESS;
}

/* This function gives the number of action profile entries "programmed"
 * down to the hardware.
 */

pipe_status_t pipe_mgr_adt_tbl_get_num_entries_programmed(
    dev_target_t dev_tgt, pipe_tbl_hdl_t tbl_hdl, uint32_t *count_p) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  uint32_t num_entries = 0;
  uint32_t num_const_init = 0;

  adt = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl info for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              tbl_hdl);
    return PIPE_INVALID_ARG;
  }
  /* This API is only applicable for indirectly referenced action tables */
  if (adt->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    LOG_ERROR(
        "%s:%d Number of entries programmed in the action data table is the "
        "same as the number of entries programmed in the associated match "
        "table for directly referenced action tbl 0x%x, device id %d. This API "
        "is only applicable for indirectly referenced action tables",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  unsigned i = 0;
  for (i = 0; i < adt->num_tbls; i++) {
    adt_tbl_data = &adt->adt_tbl_data[i];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == adt_tbl_data->pipe_id) {
      num_entries += bf_map_count(&adt_tbl_data->entry_phy_info_htbl);
      num_const_init += bf_map_count(&adt_tbl_data->const_init_entries);
    }
  }
  PIPE_MGR_DBGCHK(num_const_init <= num_entries);
  *count_p = num_entries - num_const_init;
  LOG_TRACE("%s: Dev %d pipe %X %s has %d - %d (const init) = %d programmed",
            __func__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            adt->name,
            num_entries,
            num_const_init,
            *count_p);
  return PIPE_SUCCESS;
}

pipe_mgr_adt_ram_alloc_info_t *pipe_mgr_adt_get_ram_alloc_info(
    pipe_mgr_adt_data_t *adt_tbl_data, uint8_t stage_id) {
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  uint32_t stage_idx = 0;
  uint32_t num_stages = 0;

  if (adt_tbl_data == NULL) {
    return NULL;
  }

  num_stages = adt_tbl_data->num_stages;

  for (stage_idx = 0; stage_idx < num_stages; stage_idx++) {
    adt_stage_info = &(adt_tbl_data->adt_stage_info[stage_idx]);

    if (adt_stage_info->stage_id == stage_id) {
      break;
    }
  }

  if (adt_stage_info == NULL) {
    return NULL;
  }
  return (adt_stage_info->ram_alloc_info);
}

pipe_mgr_adt_stage_info_t *pipe_mgr_adt_get_stage_info(
    pipe_mgr_adt_data_t *adt_data, uint8_t stage_id) {
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  uint8_t num_stages = 0;
  uint32_t stage_idx = 0;

  num_stages = adt_data->num_stages;

  for (stage_idx = 0; stage_idx < num_stages; stage_idx++) {
    adt_stage_info = &(adt_data->adt_stage_info[stage_idx]);

    if (adt_stage_info->stage_id == stage_id) {
      break;
    } else {
      adt_stage_info = NULL;
    }
  }

  return adt_stage_info;
}

pipe_status_t pipe_mgr_adt_get_num_ram_units(bf_dev_id_t dev_id,
                                             pipe_adt_tbl_hdl_t tbl_hdl,
                                             bf_dev_pipe_t pipe_id,
                                             uint8_t stage_id,
                                             uint32_t *num_ram_units) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  *num_ram_units = 0;

  adt = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action table for device id %d, table handle 0x%x not found",
        __func__,
        __LINE__,
        dev_id,
        tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action table instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Action table stage info not found for table 0x%x, device id %d, "
        "pipe id %d, stage id %d not found",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *num_ram_units = adt_stage_info->ram_alloc_info->num_rams_in_wide_word;
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_place(bf_dev_id_t device_id,
                                bf_dev_pipe_t pipe_id,
                                pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                pipe_adt_ent_hdl_t adt_ent_hdl,
                                uint8_t stage_id,
                                pipe_idx_t *entry_idx,
                                uint32_t pipe_api_flags) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_info_t *adt_entry_info = NULL;
  pipe_adt_ent_idx_t stage_ent_idx = 0;
  bool isTxn = false;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action table for device id %d, table handle 0x%x not found",
        __func__,
        __LINE__,
        device_id,
        adt_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (adt_entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry hdl %d, tbl 0x%x, device id %d not found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action table instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Action table stage info not found for table 0x%x, device id %d, "
        "pipe id %d, stage id %d not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);

    if (status != PIPE_SUCCESS) {
      return status;
    }

    if (!adt_stage_info->backup_entry_allocator) {
      adt_stage_info->backup_entry_allocator =
          (void *)power2_allocator_make_copy(
              (power2_allocator_t *)adt_stage_info->entry_allocator);
    }
  }

  /* Update state for entry placement */
  status = pipe_mgr_adt_update_state_for_entry_placement(
      adt, adt_stage_info, adt_entry_info, &stage_ent_idx, true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry placement for entry hdl %d "
        "tbl 0x%x device id %d, stage id %d, entry idx %d, err %s",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id,
        adt_stage_info->stage_id,
        stage_ent_idx,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  *entry_idx = stage_ent_idx + adt_stage_info->stage_offset;

  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x Placed member %d at offset %u (stage %d "
      "local-offset %u), num-refs %u",
      device_id,
      adt->name,
      adt_tbl_hdl,
      pipe_id,
      adt_ent_hdl,
      *entry_idx,
      stage_id,
      stage_ent_idx,
      adt_entry_info->num_references);

  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_remove(bf_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 dev_stage_t stage_id,
                                 uint32_t pipe_api_flags) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  bool isTxn = false;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action data table for device id %d, table handle 0x%x not found",
        __func__,
        __LINE__,
        device_id,
        adt_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_TRACE(
        "%s:%d Entry info for entry %d (0x%x), table %s (0x%x), device id %d "
        "not found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_ent_hdl,
        adt->name,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, entry_info->pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action table instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        entry_info->pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Action table stage info not found for table 0x%x, device id %d, "
        "pipe id %d, stage id %d not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        entry_info->pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);

    if (status != PIPE_SUCCESS) {
      return status;
    }

    if (!adt_stage_info->backup_entry_allocator) {
      adt_stage_info->backup_entry_allocator =
          (void *)power2_allocator_make_copy(
              (power2_allocator_t *)adt_stage_info->entry_allocator);
    }
  }

  status =
      pipe_mgr_adt_update_state_for_entry_remove(entry_info, adt_stage_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry remove for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        device_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  LOG_TRACE(
      "Dev %d tbl %s 0x%x pipe %x UnPlaced member %d from stage %d, new ref "
      "cnt %u",
      device_id,
      adt->name,
      adt_tbl_hdl,
      entry_info->pipe_id,
      adt_ent_hdl,
      stage_id,
      entry_info->num_references);

  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_non_sharable_add(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           pipe_act_fn_hdl_t *act_fn_hdl_p,
                                           uint32_t pipe_api_flags) {
  pipe_status_t status;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  bool isTxn = false;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action data table info for tbl 0x%x, device id %d not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);

    if (status != PIPE_SUCCESS) {
      return status;
    }
  }
  entry_info->num_references++;
  if (act_fn_hdl_p) {
    *act_fn_hdl_p = entry_info->entry_data->act_fn_hdl;
  }

  LOG_TRACE("Dev %d tbl %s 0x%x pipe %x NonShrAdd entry %d num-refs %u",
            device_id,
            adt->name,
            adt_tbl_hdl,
            entry_info->pipe_id,
            adt_ent_hdl,
            entry_info->num_references);

  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_non_sharable_get(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           pipe_act_fn_hdl_t *act_fn_hdl_p) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action data table info for tbl 0x%x, device id %d not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (act_fn_hdl_p) {
    *act_fn_hdl_p = entry_info->entry_data->act_fn_hdl;
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_adt_ent_non_sharable_del(bf_dev_id_t device_id,
                                           pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint32_t pipe_api_flags) {
  pipe_status_t status;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  bool isTxn = false;
  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s:%d Action data table info for tbl 0x%x, device id %d not found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR("%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_ent_hdl,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (isTxn) {
    /* If this operation is done as part of a transaction, record that
     * this entry handle has been dirtied during this transaction.
     */
    status = pipe_mgr_adt_add_txn_entry(adt, adt_ent_hdl);

    if (status != PIPE_SUCCESS) {
      return status;
    }
  }
  if (entry_info->num_references > 0) {
    entry_info->num_references--;
  } else {
    LOG_ERROR("%s:%d Removing action member %d with refcount 0",
              __func__,
              __LINE__,
              adt_ent_hdl);
    return PIPE_UNEXPECTED;
  }

  LOG_TRACE("Dev %d tbl %s 0x%x pipe %x NonShrDel entry %d num-refs %u",
            device_id,
            adt->name,
            adt_tbl_hdl,
            entry_info->pipe_id,
            adt_ent_hdl,
            entry_info->num_references);

  return PIPE_SUCCESS;
}

pipe_mgr_adt_t *pipe_mgr_adt_get(bf_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  unsigned long key = 0;
  bf_map_sts_t status;
  key = adt_tbl_hdl;

  status = pipe_mgr_adt_tbl_map_get(device_id, key, (void **)&adt);

  if (status != BF_MAP_OK) {
    return NULL;
  }
  return adt;
}

pipe_mgr_adt_data_t *pipe_mgr_get_adt_instance(pipe_mgr_adt_t *adt,
                                               bf_dev_pipe_t pipe_id) {
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  uint8_t tbl_idx = 0;
  uint32_t num_tbls = 0;
  num_tbls = adt->num_tbls;
  if (pipe_id == BF_DEV_PIPE_ALL) {
    /* Only one managed instance */
    return &(adt->adt_tbl_data[0]);
  }
  for (tbl_idx = 0; tbl_idx < num_tbls; tbl_idx++) {
    adt_tbl_data = &(adt->adt_tbl_data[tbl_idx]);

    if (adt_tbl_data->pipe_id == pipe_id) {
      break;
    } else {
      adt_tbl_data = NULL;
    }
  }
  return adt_tbl_data;
}

pipe_mgr_adt_data_t *get_adt_instance_from_any_pipe(pipe_mgr_adt_t *adt,
                                                    bf_dev_pipe_t pipe_id) {
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  uint8_t tbl_idx = 0;
  uint32_t num_tbls = 0;
  num_tbls = adt->num_tbls;
  if (pipe_id == BF_DEV_PIPE_ALL) {
    /* Only one managed instance */
    return &(adt->adt_tbl_data[0]);
  }
  for (tbl_idx = 0; tbl_idx < num_tbls; tbl_idx++) {
    adt_tbl_data = &(adt->adt_tbl_data[tbl_idx]);

    if (PIPE_BITMAP_GET(&adt_tbl_data->pipe_bmp, pipe_id)) {
      return adt_tbl_data;
    }
  }
  return NULL;
}

pipe_mgr_adt_data_t *pipe_mgr_get_adt_instance_from_entry(pipe_mgr_adt_t *adt,
                                                          int entry_hdl) {
  pipe_mgr_adt_data_t *adt_tbl_data;
  bf_dev_pipe_t pipe_id;

  if (adt->symmetric) {
    adt_tbl_data = &adt->adt_tbl_data[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  }
  return adt_tbl_data;
}

pipe_mgr_adt_entry_info_t *pipe_mgr_adt_entry_deep_copy(
    pipe_mgr_adt_entry_info_t *entry) {
  pipe_mgr_adt_entry_info_t *new_entry = NULL;

  new_entry = (pipe_mgr_adt_entry_info_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_adt_entry_info_t));
  if (new_entry == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMCPY(new_entry, entry, sizeof(pipe_mgr_adt_entry_info_t));
  new_entry->sharable_stage_location = NULL;

  pipe_mgr_adt_stage_location_t *location = entry->sharable_stage_location;
  pipe_mgr_adt_stage_location_t *backup_location = NULL;
  while (location) {
    backup_location = (pipe_mgr_adt_stage_location_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_adt_stage_location_t));
    if (backup_location == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return NULL;
    }

    PIPE_MGR_MEMCPY(
        backup_location, location, sizeof(pipe_mgr_adt_stage_location_t));
    BF_LIST_DLL_PP(
        new_entry->sharable_stage_location, backup_location, next, prev);
    location = location->next;
  }

  return new_entry;
}

void pipe_mgr_adt_entry_cleanup(pipe_mgr_adt_entry_info_t *entry) {
  if (entry) {
    pipe_mgr_adt_stage_location_t *location = entry->sharable_stage_location;
    pipe_mgr_adt_stage_location_t *next = NULL;
    while (location) {
      next = location->next;
      PIPE_MGR_FREE(location);
      location = next;
    }

    PIPE_MGR_FREE(entry);
  }
}

pipe_status_t pipe_mgr_adt_add_txn_entry(pipe_mgr_adt_t *adt,
                                         pipe_adt_ent_hdl_t action_entry_hdl) {
  bf_map_sts_t map_sts;
  unsigned long key;
  pipe_mgr_adt_entry_info_t *entry = NULL;
  pipe_mgr_adt_entry_info_t *new_entry = NULL;
  pipe_mgr_adt_txn_state_t *txn_state = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data;

  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, action_entry_hdl);
  if (!adt_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  txn_state = adt_tbl_data->txn_state;
  if (txn_state == NULL) {
    LOG_ERROR(
        "%s : Could not find the transaction state for action "
        "data table for entry handle %d",
        __func__,
        action_entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  key = action_entry_hdl;

  map_sts = bf_map_get(&txn_state->dirtied_ent_hdl_htbl, key, (void **)&entry);

  if (map_sts == BF_MAP_NO_KEY) {
    map_sts = bf_map_get(&adt_tbl_data->entry_info_htbl, key, (void **)&entry);
    if (map_sts == BF_MAP_NO_KEY) {
      entry = NULL;
    }

    if (entry) {
      new_entry = pipe_mgr_adt_entry_deep_copy(entry);
      if (!new_entry) {
        return PIPE_NO_SYS_RESOURCES;
      }
    } else {
      new_entry = NULL;
    }
    map_sts =
        bf_map_add(&txn_state->dirtied_ent_hdl_htbl, key, (void *)new_entry);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error backing up entry info for hdl %x, tbl 0x%x, "
          "device id %d, err 0x%x",
          __func__,
          __LINE__,
          action_entry_hdl,
          adt->adt_tbl_hdl,
          adt->dev_id,
          map_sts);
      pipe_mgr_adt_entry_cleanup(new_entry);
      return PIPE_UNEXPECTED;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_mgr_get_phy_addrs(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_adt_ent_idx_t adt_stage_ent_idx,
    rmt_tbl_hdl_t stage_tbl_hdl,
    rmt_virt_addr_t virt_addr,
    uint64_t *phy_addrs,
    uint32_t *num_phy_addrs,
    uint32_t *entry_position) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe_id = 0;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint8_t num_rams_in_wide_word = 0;
  uint8_t wide_word_blk_idx = 0;
  uint8_t num_entries_per_wide_word = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t adt_entry_width = 0;
  uint32_t ram_line_num = 0;
  unsigned i = 0;

  (*num_phy_addrs) = 0;
  adt = pipe_mgr_adt_get(dev_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info for table "
        "with handle %d for device id %d",
        __func__,
        adt_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_tbl_data = get_adt_instance_from_any_pipe(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data instance for tbl 0x%x, device id %d, pipe id %d not "
        "found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        adt->dev_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Get the stage level info */
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the action data table stage info for "
        "table with handle %d, stage id %d",
        __func__,
        adt_tbl_hdl,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;

  if (num_rams_in_wide_word == 1) {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH / num_entries_per_wide_word) / 8;
  } else {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH * num_rams_in_wide_word) / 8;
  }

  if (virt_addr == 0xdeadbeef) {
    uint8_t sub_tbl_idx;
    for (sub_tbl_idx = 0; sub_tbl_idx < adt_stage_info->num_sub_tbls;
         sub_tbl_idx++) {
      if (adt_stage_info->sub_tbl_offsets[sub_tbl_idx].stage_table_handle ==
          stage_tbl_hdl) {
        adt_stage_ent_idx +=
            adt_stage_info->sub_tbl_offsets[sub_tbl_idx].offset;
        break;
      }
    }
    wide_word_blk_idx = adt_stage_ent_idx / num_entries_per_wide_word_blk;
    ram_line_num =
        (adt_stage_ent_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
    *entry_position = adt_stage_ent_idx % num_entries_per_wide_word;
  } else {
    vpn_id_t vpn;
    pipe_mgr_adt_tof_unbuild_virt_addr(
        virt_addr, adt_entry_width, &ram_line_num, &vpn, entry_position);
    /* From VPN, figure out the wide word blk idx */
    for (i = 0; i < ram_alloc_info->num_wide_word_blks; i++) {
      if (ram_alloc_info->tbl_word_blk[i].vpn_id[0] == vpn) {
        wide_word_blk_idx = i;
        break;
      }
    }
    if (i == ram_alloc_info->num_wide_word_blks) {
      LOG_ERROR(
          "%s:%d Invalid VPN %d for action tbl 0x%x, device id %d, stage id "
          "%d, virt addr 0x%x",
          __func__,
          __LINE__,
          vpn,
          adt->adt_tbl_hdl,
          dev_id,
          stage_id,
          virt_addr);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
  }
  if (adt_tbl_data->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = adt->lowest_pipe_id;
  } else {
    pipe_id = adt_tbl_data->pipe_id;
  }
  status =
      pipe_mgr_map_pipe_id_log_to_phy(adt->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != status) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              dev_id,
              pipe_str_err(status));
    return status;
  }
  for (i = 0; i < num_rams_in_wide_word; i++) {
    phy_addrs[i] = adt->dev_info->dev_cfg.get_full_phy_addr(
        adt->direction,
        phy_pipe_id,
        stage_id,
        ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id[i],
        ram_line_num,
        pipe_mem_type_unit_ram);
    (*num_phy_addrs)++;
    /* For action data entries, the packing of multiple entries happens only
     * within one single RAM. If the rams allocated is more than 1 per wide word
     * blk, then it implies that there is no packing
     */
    if (num_entries_per_wide_word > 1) {
      break;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_mgr_get_ent_hdl_from_location(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t virt_addr,
    bool sharable,
    pipe_adt_ent_hdl_t *adt_ent_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_adt_stage_location_t *location = NULL;
  pipe_adt_ent_idx_t adt_stage_ent_idx = 0;
  uint32_t ram_line_num = 0;
  unsigned long key = 0;
  bool found = false;
  vpn_id_t vpn = 0;
  uint32_t entry_position = 0;
  uint32_t num_rams_in_wide_word = 0;
  uint32_t num_entries_per_wide_word = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t adt_entry_width = 0;
  uint8_t wide_word_blk_idx = 0;
  unsigned i = 0;

  if (pipe_mgr_is_device_virtual(device_id)) {
    LOG_ERROR(
        "%s:%d Getting action data entry handle from location is supported "
        "only physical devices. Table 0x%x, device id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_INVALID_ARG;
  }

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Adt pipe table not found for tbl 0x%x, device id %d, pipe id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Adt tbl stage info not found for tbl 0x%x, device id %d, pipe "
        "id %d, stage id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * (TOF_SRAM_UNIT_DEPTH);

  if (num_rams_in_wide_word == 1) {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH / num_entries_per_wide_word) / 8;
  } else {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH * num_rams_in_wide_word) / 8;
  }

  pipe_mgr_adt_tof_unbuild_virt_addr(
      virt_addr, adt_entry_width, &ram_line_num, &vpn, &entry_position);
  /* From VPN, figure out the wide word blk idx */
  for (i = 0; i < ram_alloc_info->num_wide_word_blks; i++) {
    if (ram_alloc_info->tbl_word_blk[i].vpn_id[0] == vpn) {
      wide_word_blk_idx = i;
      break;
    }
  }
  if (i == ram_alloc_info->num_wide_word_blks) {
    LOG_ERROR(
        "%s:%d Invalid VPN %d for action tbl 0x%x, device id %d, stage id "
        "%d, virt addr 0x%x",
        __func__,
        __LINE__,
        vpn,
        adt->adt_tbl_hdl,
        device_id,
        stage_id,
        virt_addr);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  adt_stage_ent_idx = (ram_line_num * num_entries_per_wide_word) +
                      entry_position +
                      (wide_word_blk_idx * num_entries_per_wide_word_blk);

  map_sts = bf_map_get_first(
      &adt_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR("%s:%d Entry info not present for action tabl 0x%x, device id %d",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  while (map_sts == BF_MAP_OK) {
    if (entry_info->pipe_id == pipe_id) {
      if (sharable) {
        location = entry_info->sharable_stage_location;
      } else {
        location = entry_info->non_sharable_stage_location;
      }
      while (location) {
        if (location->stage_id == stage_id &&
            location->entry_idx == adt_stage_ent_idx) {
          found = true;
          break;
        }
        location = location->next;
      }
      if (found) {
        break;
      }
    }
    map_sts = bf_map_get_next(
        &adt_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
  }
  if (found) {
    *adt_ent_hdl = key;
    return PIPE_SUCCESS;
  }
  return PIPE_OBJ_NOT_FOUND;
}

static pipe_status_t pipe_mgr_get_adt_data_from_hw(
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_mgr_adt_stage_location_t *location = NULL;
  pipe_action_data_spec_t *local_spec = NULL;
  pipe_action_data_spec_t *local_spec1 = NULL;
  uint64_t phy_addrs[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  uint32_t num_phy_addrs = 0;
  uint32_t entry_position = 0;
  bool non_sharable_done = false;
  uint8_t **adt_word_ptrs = NULL;
  bf_dev_pipe_t pipe_id = 0;
  unsigned i = 0;

  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    LOG_ERROR(
        "%s:%d Get action entry data from hardware not supported on virtual "
        "device, id %d",
        __func__,
        __LINE__,
        dev_tgt.device_id);
    return PIPE_NOT_SUPPORTED;
  }
  adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for action entry handle %d, tbl 0x%x, device id %d "
        "not found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        dev_tgt.device_id);
    status = PIPE_OBJ_NOT_FOUND;
    goto err_cleanup;
  }
  adt_tbl_data = pipe_mgr_get_adt_instance(adt, entry_info->pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Action data instance for tbl 0x%x, device id %d, pipe id %d not "
        "found",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        adt->dev_id,
        entry_info->pipe_id);
    status = PIPE_INVALID_ARG;
    goto err_cleanup;
  }

  status = pipe_mgr_get_pipe_id(&adt_tbl_data->pipe_bmp,
                                dev_tgt.dev_pipe_id,
                                entry_info->pipe_id,
                                &pipe_id);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s:%d Action data tbl 0x%x, device id %d, Pipe id %d does not match "
        "entry pipe id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        adt->dev_id,
        dev_tgt.dev_pipe_id,
        entry_info->pipe_id);
    goto err_cleanup;
  }

  *act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
  location = entry_info->sharable_stage_location;
  if (!location) {
    non_sharable_done = true;
    location = entry_info->non_sharable_stage_location;
  }
  if (!location) {
    LOG_WARN(
        "%s:%d Action entry handle %d, for tbl 0x%x, device id %d not "
        "installed in hardware",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        dev_tgt.device_id);

    local_spec = unpack_adt_ent_data_ad(entry_info->entry_data);
    uint8_t *tmp = act_data_spec->action_data_bits;
    *act_data_spec = *local_spec;
    act_data_spec->action_data_bits = tmp;
    PIPE_MGR_MEMCPY(act_data_spec->action_data_bits,
                    local_spec->action_data_bits,
                    local_spec->num_action_data_bytes);
    return PIPE_SUCCESS;
  }
  pipe_id = (entry_info->pipe_id == BF_DEV_PIPE_ALL) ? adt->lowest_pipe_id
                                                     : entry_info->pipe_id;
  status = pipe_mgr_map_pipe_id_log_to_phy(adt->dev_info, pipe_id, &pipe_id);
  if (status) return status;

  local_spec = (pipe_action_data_spec_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_action_data_spec_t));
  if (local_spec == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  local_spec->action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
      act_data_spec->num_action_data_bytes, sizeof(uint8_t));
  if (local_spec->action_data_bits == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    goto err_cleanup;
  }

  while (location) {
    status = pipe_mgr_adt_mgr_get_phy_addrs(dev_tgt.device_id,
                                            adt_tbl_hdl,
                                            entry_info->pipe_id,
                                            location->stage_id,
                                            location->entry_idx,
                                            0xff,
                                            0xdeadbeef,
                                            phy_addrs,
                                            &num_phy_addrs,
                                            &entry_position);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting adt phy address for entry handle %d, idx %d, "
          "pipe_id %d, stage id %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          adt_ent_hdl,
          location->entry_idx,
          entry_info->pipe_id,
          location->stage_id,
          adt_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
    if (!adt_word_ptrs) {
      adt_word_ptrs =
          (uint8_t **)PIPE_MGR_CALLOC(num_phy_addrs, sizeof(uint8_t *));
      if (adt_word_ptrs == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        status = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
      for (i = 0; i < num_phy_addrs; i++) {
        adt_word_ptrs[i] =
            (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
        if (adt_word_ptrs[i] == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          status = PIPE_NO_SYS_RESOURCES;
          goto err_cleanup;
        }
      }
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(pipe_id);
    status = pipe_mgr_dump_any_tbl_by_addr(dev_tgt.device_id,
                                           subdev,
                                           adt_tbl_hdl,
                                           0xff,
                                           location->stage_id,
                                           RMT_TBL_TYPE_ACTION_DATA,
                                           phy_addrs,
                                           num_phy_addrs,
                                           entry_position,
                                           *act_fn_hdl,
                                           NULL,
                                           NULL,
                                           0,
                                           adt_word_ptrs,
                                           NULL,
                                           NULL /*sess_hdl*/);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in dumping action entry handle %d, tbl 0x%x, device id "
          "%d, err %s",
          __func__,
          __LINE__,
          adt_ent_hdl,
          adt_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
    status = pipe_mgr_entry_format_tof_adt_tbl_ent_decode_to_action_spec(
        adt->dev_info,
        adt->profile_id,
        location->stage_id,
        adt_tbl_hdl,
        *act_fn_hdl,
        entry_position,
        local_spec,
        adt_word_ptrs);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding action entry hdl %d, tbl 0x%x, device id "
          "%d, err %s",
          __func__,
          __LINE__,
          adt_ent_hdl,
          adt_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
    if (!local_spec1) {
      local_spec1 = (pipe_action_data_spec_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_action_data_spec_t));
      if (local_spec1 == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        status = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
      local_spec1->action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
          act_data_spec->num_action_data_bytes, sizeof(uint8_t));
      if (local_spec->action_data_bits == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        goto err_cleanup;
      }
      PIPE_MGR_MEMCPY(local_spec1->action_data_bits,
                      local_spec->action_data_bits,
                      act_data_spec->num_action_data_bytes);
    } else {
      if (memcmp(local_spec->action_data_bits,
                 local_spec1->action_data_bits,
                 act_data_spec->num_action_data_bytes)) {
        LOG_ERROR(
            "%s:%d Mismatching action data specs in hardware for entry handle "
            "%d, tbl 0x%x, device id %d",
            __func__,
            __LINE__,
            adt_ent_hdl,
            adt_tbl_hdl,
            dev_tgt.device_id);
        status = PIPE_UNEXPECTED;
        goto err_cleanup;
      }
    }
    location = location->next;
    if (!location && !non_sharable_done) {
      non_sharable_done = true;
      location = entry_info->non_sharable_stage_location;
    }
  }
  PIPE_MGR_MEMCPY(act_data_spec->action_data_bits,
                  local_spec1->action_data_bits,
                  act_data_spec->num_action_data_bytes);
err_cleanup:
  if (local_spec) {
    if (local_spec->action_data_bits) {
      PIPE_MGR_FREE(local_spec->action_data_bits);
    }
    PIPE_MGR_FREE(local_spec);
  }
  if (local_spec1) {
    if (local_spec1->action_data_bits) {
      PIPE_MGR_FREE(local_spec1->action_data_bits);
    }
    PIPE_MGR_FREE(local_spec1);
  }
  if (adt_word_ptrs) {
    for (i = 0; i < num_phy_addrs; i++) {
      if (adt_word_ptrs[i]) {
        PIPE_MGR_FREE(adt_word_ptrs[i]);
      }
    }
    PIPE_MGR_FREE(adt_word_ptrs);
  }
  return status;
}

bool pipe_mgr_adt_tbl_is_ent_hdl_valid(pipe_mgr_adt_data_t *adt_tbl_data,
                                       pipe_adt_ent_hdl_t mat_ent_hdl) {
  if (bf_id_allocator_is_set(adt_tbl_data->ent_hdl_allocator,
                             PIPE_GET_HDL_VAL(mat_ent_hdl))) {
    return true;
  }
  return false;
}

pipe_status_t pipe_mgr_adt_get_entry(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  if (from_hw) {
    return pipe_mgr_get_adt_data_from_hw(
        dev_tgt, tbl_hdl, entry_hdl, pipe_action_data_spec, act_fn_hdl);
  }
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  pipe_action_data_spec_t *action_data_spec = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;

  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt_tbl == NULL) {
    LOG_ERROR("%s:%d : Adt tbl info not found for table 0x%x, device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  entry_info = pipe_mgr_adt_get_entry_info(adt_tbl, entry_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Adt entry info for entry hdl %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  adt_tbl_data = pipe_mgr_get_adt_instance(adt_tbl, entry_info->pipe_id);
  if (adt_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Adt table data for pipe_id %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        entry_info->pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Check if the entry handle passed in a valid one */
  if (pipe_mgr_adt_tbl_is_ent_hdl_valid(adt_tbl_data, entry_hdl) == false) {
    LOG_ERROR("%s:%d Adt entry hdl %d not valid for tbl 0x%x device id %d",
              __func__,
              __LINE__,
              entry_hdl,
              adt_tbl->adt_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  action_data_spec = unpack_adt_ent_data_ad(entry_info->entry_data);
  *act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
  /* Copy the action spec */
  uint8_t *act_data_bits = pipe_action_data_spec->action_data_bits;
  PIPE_MGR_MEMCPY(
      pipe_action_data_spec, action_data_spec, sizeof(pipe_action_data_spec_t));
  pipe_action_data_spec->action_data_bits = act_data_bits;
  PIPE_MGR_MEMCPY(pipe_action_data_spec->action_data_bits,
                  action_data_spec->action_data_bits,
                  action_data_spec->num_action_data_bytes);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_log_state(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t tbl_hdl,
                                     cJSON *adt_tbls) {
  bf_map_sts_t st;
  pipe_mgr_adt_t *tbl_info = NULL;
  pipe_mgr_adt_data_t *tbl_data;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *phy_entry_info = NULL;
  unsigned long ent_hdl;
  pipe_action_data_spec_t *act_data;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mgr_adt_stage_location_t *stage_loc;
  cJSON *adt_tbl, *adt_ents, *adt_ent, *locs, *loc;
  cJSON *act_data_node, *indirect_resources, *indirect_resource;
  uint32_t t;

  tbl_info = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  cJSON_AddItemToArray(adt_tbls, adt_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(adt_tbl, "name", tbl_info->name);
  cJSON_AddNumberToObject(adt_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(adt_tbl, "symmetric", tbl_info->symmetric);
  cJSON_AddBoolToObject(adt_tbl, "cache_id", tbl_info->cache_id);

  cJSON_AddItemToObject(adt_tbl, "hlp_entries", adt_ents = cJSON_CreateArray());
  for (t = 0; t < tbl_info->num_tbls; t++) {
    tbl_data = &tbl_info->adt_tbl_data[t];
    st = bf_map_get_first(
        &tbl_data->entry_info_htbl, &ent_hdl, (void **)&entry_info);
    while (st == BF_MAP_OK) {
      cJSON_AddItemToArray(adt_ents, adt_ent = cJSON_CreateObject());
      cJSON_AddNumberToObject(adt_ent, "entry_hdl", ent_hdl);
      cJSON_AddNumberToObject(adt_ent, "pipe_id", entry_info->pipe_id);
      cJSON_AddNumberToObject(adt_ent, "num_refs", entry_info->num_references);
      cJSON_AddNumberToObject(adt_ent, "mbr_id", entry_info->mbr_id);

      act_data = unpack_adt_ent_data_ad(entry_info->entry_data);
      act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
      int resource_count =
          unpack_adt_ent_data_num_resources(entry_info->entry_data);
      adt_data_resources_t *resources =
          unpack_adt_ent_data_resources(entry_info->entry_data);
      cJSON_AddItemToObject(
          adt_ent, "act_data", act_data_node = cJSON_CreateObject());
      pipe_mgr_entry_format_jsonify_action_spec(
          dev_id, tbl_info->profile_id, act_data, act_fn_hdl, act_data_node);
      cJSON_AddNumberToObject(act_data_node, "act_fn_hdl", act_fn_hdl);
      cJSON_AddNumberToObject(
          adt_ent, "num_indirect_resources", resource_count);
      if (resource_count) {
        cJSON_AddItemToObject(adt_ent,
                              "indirect_resources",
                              indirect_resources = cJSON_CreateArray());
        int i = 0;
        for (i = 0; i < resource_count; i++) {
          cJSON_AddItemToArray(indirect_resources,
                               indirect_resource = cJSON_CreateObject());
          cJSON_AddNumberToObject(
              indirect_resource, "tbl_hdl", resources[i].tbl_hdl);
          cJSON_AddNumberToObject(
              indirect_resource, "tbl_idx", resources[i].tbl_idx);
        }
      }
      cJSON_AddItemToObject(
          adt_ent, "sharable_stage_locs", locs = cJSON_CreateArray());
      for (stage_loc = entry_info->sharable_stage_location; stage_loc;
           stage_loc = stage_loc->next) {
        cJSON_AddItemToArray(locs, loc = cJSON_CreateObject());
        cJSON_AddNumberToObject(loc, "stage_id", stage_loc->stage_id);
        cJSON_AddNumberToObject(loc, "entry_idx", stage_loc->entry_idx);
        cJSON_AddNumberToObject(loc, "ref_count", stage_loc->ref_count);
      }

      st = bf_map_get_next(
          &tbl_data->entry_info_htbl, &ent_hdl, (void **)&entry_info);
    }
  }

  cJSON_AddItemToObject(adt_tbl, "llp_entries", adt_ents = cJSON_CreateArray());
  for (t = 0; t < tbl_info->num_tbls; t++) {
    tbl_data = &tbl_info->adt_tbl_data[t];
    st = bf_map_get_first(
        &tbl_data->entry_phy_info_htbl, &ent_hdl, (void **)&phy_entry_info);
    while (st == BF_MAP_OK) {
      cJSON_AddItemToArray(adt_ents, adt_ent = cJSON_CreateObject());
      cJSON_AddNumberToObject(adt_ent, "entry_hdl", ent_hdl);
      cJSON_AddItemToObject(
          adt_ent, "nonsharable_stage_locs", locs = cJSON_CreateArray());
      for (stage_loc = phy_entry_info->non_sharable_stage_location; stage_loc;
           stage_loc = stage_loc->next) {
        cJSON_AddItemToArray(locs, loc = cJSON_CreateObject());
        cJSON_AddNumberToObject(loc, "stage_id", stage_loc->stage_id);
        cJSON_AddNumberToObject(loc, "entry_idx", stage_loc->entry_idx);
        cJSON_AddNumberToObject(loc, "ref_count", stage_loc->ref_count);
      }
      st = bf_map_get_next(
          &tbl_data->entry_phy_info_htbl, &ent_hdl, (void **)&phy_entry_info);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_restore_state(bf_dev_id_t dev_id, cJSON *adt_tbl) {
  pipe_status_t sts;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  pipe_adt_tbl_hdl_t tbl_hdl;
  bool symmetric = false;
  bf_dev_pipe_t pipe_id;
  pipe_mgr_adt_t *tbl_info = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  pipe_mgr_adt_entry_phy_info_t *phy_entry_info = NULL;
  pipe_mgr_adt_stage_location_t *stage_loc;
  pipe_mgr_adt_move_list_t *ml = NULL;
  pipe_adt_ent_hdl_t ent_hdl;
  pipe_adt_mbr_id_t mbr_id;
  pipe_action_spec_t action_spec = {0};
  pipe_action_data_spec_t act_data;
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t ent_idx;
  uint32_t stage_id;
  uint32_t num_ref_idx;
  rmt_virt_addr_t virt_addr = 0;
  cJSON *adt_ents, *adt_ent, *locs, *loc;
  cJSON *act_data_node, *resources, *resource;
  scope_pipes_t scopes = 0xf;

  tbl_hdl = cJSON_GetObjectItem(adt_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d Action table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  tbl_info->cache_id =
      (cJSON_GetObjectItem(adt_tbl, "cache_id")->type == cJSON_True);

  symmetric = (cJSON_GetObjectItem(adt_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != tbl_info->symmetric) {
    sts = pipe_mgr_adt_tbl_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, 1, &scopes);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, action tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      return sts;
    }
  }

  /* State restore from HLP info */
  adt_ents = cJSON_GetObjectItem(adt_tbl, "hlp_entries");
  for (adt_ent = adt_ents->child; adt_ent; adt_ent = adt_ent->next) {
    ent_hdl = cJSON_GetObjectItem(adt_ent, "entry_hdl")->valuedouble;
    pipe_id = cJSON_GetObjectItem(adt_ent, "pipe_id")->valueint;
    mbr_id = cJSON_GetObjectItem(adt_ent, "mbr_id")->valuedouble;
    act_data_node = cJSON_GetObjectItem(adt_ent, "act_data");
    act_fn_hdl = cJSON_GetObjectItem(act_data_node, "act_fn_hdl")->valuedouble;
    sts = pipe_mgr_entry_format_unjsonify_action_spec(
        dev_id, tbl_info->profile_id, &act_data, act_fn_hdl, act_data_node);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to read action data spec for adt entry %u tbl 0x%x dev %u",
          ent_hdl,
          tbl_hdl,
          dev_id);
      return sts;
    }
    action_spec.act_data = act_data;
    int num_resources =
        cJSON_GetObjectItem(adt_ent, "num_indirect_resources")->valueint;
    if (num_resources) {
      resources = cJSON_GetObjectItem(adt_ent, "indirect_resources");
      int i = 0;
      action_spec.resource_count = num_resources;
      for (i = 0; i < num_resources; i++) {
        resource = cJSON_GetArrayItem(resources, i);
        action_spec.resources[i].tbl_hdl =
            cJSON_GetObjectItem(resource, "tbl_hdl")->valueint;
        action_spec.resources[i].tbl_idx =
            cJSON_GetObjectItem(resource, "tbl_idx")->valueint;
      }
    }

    adt_tbl_data = pipe_mgr_get_adt_instance(tbl_info, pipe_id);
    if (adt_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Action table instance for tbl 0x%x, device id %d, pipe id %d "
          "not found",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* HLP restore */
    ml = alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_ADD);
    if (ml == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    sts = pipe_mgr_adt_update_state_for_new_entry(tbl_info,
                                                  adt_tbl_data,
                                                  ent_hdl,
                                                  mbr_id,
                                                  false,
                                                  &action_spec,
                                                  act_fn_hdl,
                                                  ml);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in updating state for action data entry for table with "
          "handle %d, entry handle %d",
          __func__,
          tbl_hdl,
          ent_hdl);
      return sts;
    }

    /* LLP restore */
    sts = pipe_mgr_adt_execute_entry_add(tbl_info, ent_hdl, pipe_id, ml);
    PIPE_MGR_FREE(act_data.action_data_bits);
    free_adt_move_list(&ml);
    if (sts != PIPE_SUCCESS) {
      return sts;
    }
    entry_info = pipe_mgr_adt_get_entry_info(tbl_info, ent_hdl);
    if (!entry_info) {
      LOG_ERROR("%s:%d Failed to get entry info", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }
    entry_info->num_references =
        cJSON_GetObjectItem(adt_ent, "num_refs")->valuedouble;
    locs = cJSON_GetObjectItem(adt_ent, "sharable_stage_locs");
    for (loc = locs->child; loc; loc = loc->next) {
      stage_loc = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_adt_stage_location_t));
      if (stage_loc == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      stage_loc->stage_id = cJSON_GetObjectItem(loc, "stage_id")->valueint;
      stage_loc->entry_idx = cJSON_GetObjectItem(loc, "entry_idx")->valuedouble;
      stage_loc->ref_count = cJSON_GetObjectItem(loc, "ref_count")->valuedouble;
      BF_LIST_DLL_PP(
          entry_info->sharable_stage_location, stage_loc, next, prev);

      adt_stage_info =
          pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_loc->stage_id);
      if (adt_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Action table stage info not found for table 0x%x, "
            "device id %d, pipe id %d, stage id %d not found",
            __func__,
            __LINE__,
            tbl_hdl,
            dev_id,
            pipe_id,
            stage_loc->stage_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      power2_allocator_reserve(
          (power2_allocator_t *)adt_stage_info->entry_allocator,
          stage_loc->entry_idx,
          1);

      for (num_ref_idx = 0; num_ref_idx < stage_loc->ref_count; num_ref_idx++) {
        sts = rmt_adt_ent_activate_stage(
            sess_hdl,
            dev_id,
            pipe_id,
            tbl_hdl,
            ent_hdl,
            stage_loc->stage_id,
            stage_loc->entry_idx + adt_stage_info->stage_offset,
            &virt_addr,
            false);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in activating action entry hdl %d for "
              "action tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
              __func__,
              __LINE__,
              ent_hdl,
              tbl_hdl,
              dev_id,
              pipe_id,
              stage_loc->stage_id,
              pipe_str_err(sts));
          return sts;
        }
      }
    }
  }

  /* State restore from LLP info (selector members) */
  adt_ents = cJSON_GetObjectItem(adt_tbl, "llp_entries");
  for (adt_ent = adt_ents->child; adt_ent; adt_ent = adt_ent->next) {
    ent_hdl = cJSON_GetObjectItem(adt_ent, "entry_hdl")->valuedouble;
    phy_entry_info = pipe_mgr_adt_get_entry_phy_info(tbl_info, ent_hdl);
    if (phy_entry_info == NULL) {
      LOG_ERROR(
          "%s:%d Phy entry info for tbl 0x%x, device id %d, entry handle %d "
          "not found",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    adt_tbl_data = pipe_mgr_get_adt_instance(tbl_info, phy_entry_info->pipe_id);
    if (adt_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Action table instance for tbl 0x%x, device id %d, pipe id %d "
          "not found",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          phy_entry_info->pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    locs = cJSON_GetObjectItem(adt_ent, "nonsharable_stage_locs");
    for (loc = locs->child; loc; loc = loc->next) {
      ent_idx = cJSON_GetObjectItem(loc, "entry_idx")->valuedouble;
      stage_id = cJSON_GetObjectItem(loc, "stage_id")->valueint;
      adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
      if (adt_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Action table stage info not found for table 0x%x, "
            "device id %d, pipe id %d, stage id %d not found",
            __func__,
            __LINE__,
            tbl_hdl,
            dev_id,
            phy_entry_info->pipe_id,
            stage_id);
        return PIPE_OBJ_NOT_FOUND;
      }

      /* Only need to install the action members. Corresponding selector state
       * will be independently restored by the selector manager.
       */
      sts = rmt_adt_ent_install(sess_hdl,
                                dev_id,
                                tbl_hdl,
                                phy_entry_info->pipe_id,
                                stage_id,
                                ent_hdl,
                                ent_idx + adt_stage_info->stage_offset,
                                true);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d 0x%x Error installing adt ent %u at offset %u "
            "stage %u",
            __func__,
            __LINE__,
            tbl_hdl,
            ent_hdl,
            ent_idx,
            stage_id);
        return sts;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_get_phy_loc_info(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    rmt_tbl_hdl_t stage_tbl_hdl,
    pipe_adt_ent_idx_t entry_idx,
    adt_loc_info_t *loc_info) {
  uint8_t num_entries_per_wide_word =
      adt_stage_info->ram_alloc_info->num_entries_per_wide_word;
  uint32_t num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;
  uint8_t sub_tbl_idx;

  for (sub_tbl_idx = 0; sub_tbl_idx < adt_stage_info->num_sub_tbls;
       sub_tbl_idx++) {
    if (adt_stage_info->sub_tbl_offsets[sub_tbl_idx].stage_table_handle ==
        stage_tbl_hdl) {
      entry_idx += adt_stage_info->sub_tbl_offsets[sub_tbl_idx].offset;
      break;
    }
  }
  if (sub_tbl_idx == adt_stage_info->num_sub_tbls) {
    return PIPE_OBJ_NOT_FOUND;
  }

  loc_info->entry_idx = entry_idx;
  loc_info->wide_word_blk_idx = entry_idx / num_entries_per_wide_word_blk;
  loc_info->ram_line_num =
      (entry_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  loc_info->sub_entry = entry_idx % num_entries_per_wide_word;

  return PIPE_SUCCESS;
}
