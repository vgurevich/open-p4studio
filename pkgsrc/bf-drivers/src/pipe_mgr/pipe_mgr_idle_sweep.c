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
 * @file pipe_mgr_idle_sweep.c
 * @date
 *
 * Implementation of Idle time software sweep
 */

#include <stdint.h>
#include <stdbool.h>
#include <target-utils/bit_utils/bit_utils.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <lld/bf_dma_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_idle_api.h"

/* Used in notification entry map to reference notification type. */
static const pipe_idle_time_hit_state_e g_state_idle = ENTRY_IDLE;
static const pipe_idle_time_hit_state_e g_state_active = ENTRY_ACTIVE;

static uint32_t calc_cur_ttl(uint32_t init_ttl,
                             uint32_t cur_ttl,
                             uint32_t ttl) {
  /* The new_ttl will be cur_ttl - (init_ttl - new_ttl) */
  uint32_t diff = (init_ttl > ttl) ? (init_ttl - ttl) : 0;
  uint32_t sum = (init_ttl < ttl) ? (ttl - init_ttl) : 0;
  uint32_t new_ttl = 0;
  if (cur_ttl < diff) {
    new_ttl = 0;
  } else {
    new_ttl = cur_ttl - diff + sum;
  }
  return new_ttl;
}

static idle_entry_location_t *find_il(idle_entry_metadata_t *ient_mdata,
                                      uint32_t index,
                                      bool current) {
  idle_entry_location_t *il = NULL;
  if (current) {
    for (il = ient_mdata->locations; il; il = il->n) {
      if (il->index_valid && (il->cur_index == index)) {
        break;
      }
    }
  } else {
    BF_LIST_DLL_LAST(ient_mdata->locations, il, n, p);
    idle_entry_location_t *ilast = il;
    do {
      if (!il || (il->dest_index == index)) {
        break;
      }
      il = il->p;
    } while (ilast != il);
  }
  return il;
}

static idle_entry_location_t *find_il_for_activate(
    idle_entry_metadata_t *ient_mdata, uint32_t index) {
  /* For activate find, we should not check for index_valid */
  idle_entry_location_t *il = NULL;
  for (il = ient_mdata->locations; il; il = il->n) {
    if (il->cur_index == index) {
      break;
    }
  }
  return il;
}

void destroy_ils(idle_entry_location_t *ils) {
  while (ils) {
    idle_entry_location_t *il = ils;
    BF_LIST_DLL_REM(ils, il, n, p);
    PIPE_MGR_FREE(il);
  }
}

static idle_entry_location_t *copy_ils(idle_entry_metadata_t *ient_mdata) {
  idle_entry_location_t *dils = NULL;
  idle_entry_location_t *il = NULL;
  for (il = ient_mdata->locations; il; il = il->n) {
    idle_entry_location_t *dil = NULL;
    dil =
        (idle_entry_location_t *)PIPE_MGR_MALLOC(sizeof(idle_entry_location_t));
    if (dil == NULL) {
      LOG_ERROR(
          "%s:%d"
          "Malloc Error",
          __func__,
          __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMCPY(dil, il, sizeof(idle_entry_location_t));
    dil->n = NULL;
    dil->p = NULL;
    BF_LIST_DLL_AP(dils, dil, n, p);
  }

  return dils;
cleanup:
  destroy_ils(dils);
  return NULL;
}

pipe_status_t pipe_mgr_idle_entry_del_mdata(idle_tbl_stage_info_t *stage_info,
                                            pipe_mat_ent_hdl_t ent_hdl,
                                            uint32_t index) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting mat-entry metadata for entry 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  idle_entry_location_t *il = NULL;
  il = find_il(ient_mdata, index, false);
  if (il == NULL) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  il->del_in_progress = true;

  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_add_mdata(idle_tbl_stage_info_t *stage_info,
                                            pipe_mat_ent_hdl_t ent_hdl,
                                            uint32_t index,
                                            uint32_t new_ttl,
                                            uint32_t cur_ttl) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  idle_entry_metadata_t *ient_mdata = NULL;
  bool map_entry_exists = true;
  bf_map_sts_t msts;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    ient_mdata =
        (idle_entry_metadata_t *)PIPE_MGR_MALLOC(sizeof(idle_entry_metadata_t));
    if (!ient_mdata) {
      PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMSET(ient_mdata, 0, sizeof(idle_entry_metadata_t));
    ient_mdata->locations = NULL;
    ient_mdata->refcount = 0;
    map_entry_exists = false;
  }

  ient_mdata->new_ttl = new_ttl;
  ient_mdata->cur_ttl = cur_ttl;

  idle_entry_location_t *il = NULL;
  il = (idle_entry_location_t *)PIPE_MGR_MALLOC(sizeof(idle_entry_location_t));
  if (il == NULL) {
    LOG_ERROR(
        "%s:%d "
        "Malloc Error",
        __func__,
        __LINE__);
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMSET(il, 0, sizeof(idle_entry_location_t));
  il->index_valid = false;
  il->cur_index = index;
  il->dest_index = index;

  BF_LIST_DLL_AP(ient_mdata->locations, il, n, p);

  ient_mdata->refcount++;

  if (map_entry_exists == false) {
    msts = bf_map_add(&stage_info->entry_mdata_map, ent_hdl, ient_mdata);
    if (msts != BF_MAP_OK) {
      PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding mat-entry metadata for entry 0x%x rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ent_hdl,
          msts);
      return PIPE_UNEXPECTED;
    }
  }
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_move_mdata(idle_tbl_stage_info_t *stage_info,
                                             pipe_mat_ent_hdl_t ent_hdl,
                                             uint32_t src_index,
                                             uint32_t dest_index) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting mat-entry metadata for entry 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  idle_entry_location_t *il = NULL;
  il = find_il(ient_mdata, src_index, false);
  if (il == NULL) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  if (il->del_in_progress) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  il->dest_index = dest_index;

  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_set_mdata_ttl_dirty(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t new_ttl) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting mat-entry metadata for entry 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* If this entry has not yet been swept, update both ttls */
  if (ient_mdata->cur_ttl == ient_mdata->new_ttl) {
    ient_mdata->cur_ttl = new_ttl;
  }
  ient_mdata->new_ttl = new_ttl;
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
  return PIPE_SUCCESS;
}

bool pipe_mgr_idle_entry_mdata_exists(idle_tbl_stage_info_t *stage_info,
                                      pipe_mat_ent_hdl_t ent_hdl) {
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
  if (msts != BF_MAP_OK) {
    return false;
  }
  return true;
}

static bool pipe_mgr_idle_entry_process_mdata_entry_activate(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t index,
    uint32_t *ttl_p,
    uint32_t *cur_ttl_p) {
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    return false;
  }

  idle_entry_location_t *il = NULL;
  il = find_il_for_activate(ient_mdata, index);
  if (il == NULL) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  il->index_valid = true;
  il->cur_index = index;

  *ttl_p = ient_mdata->new_ttl;
  *cur_ttl_p = ient_mdata->cur_ttl;

  bool ret_status;
  if (il->del_in_progress) {
    ret_status = false;
  } else {
    ret_status = true;
  }
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return ret_status;
}

static bool pipe_mgr_idle_entry_process_mdata_move_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t src_index,
    uint32_t dest_index) {
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;
  bool ret_status = false;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    return false;
  }

  /* Find the location corresponding to the src-index */
  idle_entry_location_t *il = NULL;
  il = find_il(ient_mdata, src_index, true);
  if (il == NULL) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  if (il->index_valid != true) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return false;
  }
  il->cur_index = dest_index;
  if (il->del_in_progress) {
    ret_status = false;
  } else {
    ret_status = true;
  }
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return ret_status;
}

static bool pipe_mgr_idle_entry_process_mdata_del_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t index) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting mat-entry metadata for entry 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return false;
  }

  /* Find the particular index in locations and delete it */
  idle_entry_location_t *il = NULL;
  il = find_il(ient_mdata, index, true);
  PIPE_MGR_DBGCHK(il && il->del_in_progress);
  if (il == NULL || !il->del_in_progress) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return false;
  }
  BF_LIST_DLL_REM(ient_mdata->locations, il, n, p);
  if (ient_mdata->refcount == 0) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    PIPE_MGR_DBGCHK(0);
    return false;
  } else {
    ient_mdata->refcount--;
  }
  PIPE_MGR_FREE(il);

  if (ient_mdata->refcount == 0) {
    msts = bf_map_rmv(&stage_info->entry_mdata_map, ent_hdl);
    if (msts != BF_MAP_OK) {
      PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error deleting mat-entry metadata for entry 0x%x rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ent_hdl,
          msts);
      return false;
    }
    PIPE_MGR_FREE(ient_mdata);
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) "
        "Remove notifications for handle 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);

    PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
    bf_map_rmv(&idle_tbl_info->notif_list, ent_hdl);
    bf_map_rmv(&idle_tbl_info->notif_list_old, ent_hdl);
    bf_map_rmv(&idle_tbl_info->notif_list_spec_copy, ent_hdl);
    PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
  }
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return true;
}

bool pipe_mgr_idle_entry_mdata_get(idle_tbl_stage_info_t *stage_info,
                                   pipe_mat_ent_hdl_t ent_hdl,
                                   uint32_t *init_ttl_p,
                                   uint32_t *cur_ttl_p,
                                   idle_entry_location_t **ils_p) {
  bf_map_sts_t msts;
  idle_entry_metadata_t *ient_mdata = NULL;
  bool ret_status = true;

  PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
  msts =
      bf_map_get(&stage_info->entry_mdata_map, ent_hdl, (void **)&ient_mdata);
  if (msts != BF_MAP_OK) {
    PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
    return false;
  }
  if (init_ttl_p) {
    *init_ttl_p = ient_mdata->new_ttl;
  }
  if (cur_ttl_p) {
    *cur_ttl_p = ient_mdata->cur_ttl;
  }
  /* We need to return a list of indexes */
  if (ils_p) {
    *ils_p = copy_ils(ient_mdata);
  }
  PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);

  return ret_status;
}

idle_entry_t *pipe_mgr_idle_entry_get_by_index(
    idle_tbl_stage_info_t *stage_info, uint32_t index) {
  /* Return an idle entry based on the given index */

  int word = index / stage_info->entries_per_word;
  int subword = index % stage_info->entries_per_word;

  if (word >= stage_info->no_words) {
    return NULL;
  }
  return &stage_info->entries[word][subword];
}

pipe_status_t pipe_mgr_idle_entry_get_init_ttl(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t *init_ttl_p) {
  uint32_t init_ttl = 0;

  if (!pipe_mgr_idle_entry_mdata_get(
          stage_info, ent_hdl, &init_ttl, NULL, NULL)) {
    return PIPE_OBJ_NOT_FOUND;
  }

  *init_ttl_p = init_ttl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_get_ttl(idle_tbl_stage_info_t *stage_info,
                                          pipe_mat_ent_hdl_t ent_hdl,
                                          uint32_t *ttl_p,
                                          uint32_t *init_ttl_p) {
  bool ttl_dirty = false;
  uint32_t index = 0;
  uint32_t init_ttl = 0, entry_ttl = 0, cur_ttl = 0;
  idle_entry_location_t *ils = NULL, *il = NULL;

  if (!pipe_mgr_idle_entry_mdata_get(
          stage_info, ent_hdl, &init_ttl, &cur_ttl, &ils)) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (init_ttl_p) {
    *init_ttl_p = init_ttl;
  }

  *ttl_p = 0;
  if (ils == NULL) {
    *ttl_p = cur_ttl;
  }

  for (il = ils; il; il = il->n) {
    if (il->del_in_progress) {
      entry_ttl = 0;
    } else if (il->index_valid == false) {
      entry_ttl = cur_ttl;
    } else {
      index = il->cur_index;

      idle_entry_t *ient = pipe_mgr_idle_entry_get_by_index(stage_info, index);
      if (!ient) {
        LOG_ERROR("%s:%d Error, `ient` is NULL", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        destroy_ils(ils);
        return PIPE_UNEXPECTED;
      }
      ttl_dirty = (ient->init_ttl != init_ttl) ? true : false;
      if (ttl_dirty) {
        entry_ttl = calc_cur_ttl(ient->init_ttl, ient->cur_ttl, init_ttl);
      } else {
        entry_ttl = ient->cur_ttl;
      }
    }

    if (entry_ttl > *ttl_p) {
      *ttl_p = entry_ttl;
    }
  }
  destroy_ils(ils);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_get_poll_state(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e *poll_state_p) {
  uint32_t index = 0;
  idle_entry_location_t *ils = NULL, *il = NULL;
  pipe_idle_time_hit_state_e entry_poll_state;

  if (!pipe_mgr_idle_entry_mdata_get(stage_info, ent_hdl, NULL, NULL, &ils)) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (ils == NULL) {
    *poll_state_p = ENTRY_IDLE;
  }

  for (il = ils; il; il = il->n) {
    if (il->del_in_progress) {
      entry_poll_state = ENTRY_IDLE;
    } else if (il->index_valid == false) {
      entry_poll_state = ENTRY_IDLE;
    } else {
      index = il->cur_index;
      idle_entry_t *ient = pipe_mgr_idle_entry_get_by_index(stage_info, index);
      if (!ient) {
        LOG_ERROR("%s:%d Error, `ient` is NULL", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      entry_poll_state = ient->poll_state;
    }

    if (entry_poll_state == ENTRY_ACTIVE) {
      *poll_state_p = ENTRY_ACTIVE;
    }
  }
  destroy_ils(ils);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_entry_set_poll_state(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e poll_state) {
  uint32_t index = 0;
  idle_entry_location_t *ils = NULL, *il = NULL;

  if (!pipe_mgr_idle_entry_mdata_get(stage_info, ent_hdl, NULL, NULL, &ils)) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (ils == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  for (il = ils; il; il = il->n) {
    if (il->del_in_progress || il->index_valid == false) {
      continue;
    } else {
      index = il->cur_index;
      idle_entry_t *ient = pipe_mgr_idle_entry_get_by_index(stage_info, index);
      if (!ient) {
        LOG_ERROR("%s:%d Error, `ient` is NULL", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      ient->poll_state = poll_state;
    }
  }
  destroy_ils(ils);
  return PIPE_SUCCESS;
}

static void pipe_mgr_idle_entry_rem_from_stage_lists(
    idle_tbl_stage_info_t *stage_info, idle_entry_t *ient) {
  switch (ient->notify_state) {
    case NOTIFY_ENTRY_SWEEP:
      BF_LIST_DLL_REM(stage_info->sweeps, ient, next_sweep, prev_sweep);
      break;
    case NOTIFY_ENTRY_SWEEP_CANDIDATE:
      BF_LIST_DLL_REM(
          stage_info->sweep_candidates, ient, next_sweep, prev_sweep);
      break;
    default:
      break;
  }
}

static void pipe_mgr_idle_entry_add_to_stage_lists(
    idle_tbl_stage_info_t *stage_info,
    idle_entry_t *ient,
    const char *where,
    const int line) {
  idle_tbl_t *idle_tbl = stage_info->idle_tbl_p;
  idle_tbl_info_t *idle_tbl_info = idle_tbl->idle_tbl_info;
  switch (ient->notify_state) {
    case NOTIFY_ENTRY_SWEEP:
      BF_LIST_DLL_AP(stage_info->sweeps, ient, next_sweep, prev_sweep);
      break;
    case NOTIFY_ENTRY_SWEEP_CANDIDATE:
      BF_LIST_DLL_AP(
          stage_info->sweep_candidates, ient, next_sweep, prev_sweep);
      break;
    default:
      break;
  }
  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) "
        "Entry hdl 0x%x going %s index %d "
        "stage %d pipe %d cur_ttl %d init_ttl %d",
        where,
        line,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ient->ent_hdl,
        ientry_notify_state_str(ient->notify_state),
        ient->index,
        stage_info->stage_id,
        idle_tbl->pipe_id,
        ient->cur_ttl,
        ient->init_ttl);
  } else {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) "
        "Entry hdl 0x%x activated at index %d "
        "stage %d pipe %d",
        where,
        line,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ient->ent_hdl,
        ient->index,
        stage_info->stage_id,
        idle_tbl->pipe_id);
  }
}

static pipe_status_t pipe_mgr_idle_copy_entry(idle_tbl_stage_info_t *stage_info,
                                              pipe_mat_ent_hdl_t ent_hdl,
                                              idle_entry_t *sent,
                                              idle_entry_t *dent) {
  /* Make sure that dest entry is not active */
  PIPE_MGR_DBGCHK(!dent->inuse);
  PIPE_MGR_DBGCHK(!dent->next_active && !dent->prev_active);
  PIPE_MGR_DBGCHK(!dent->next_sweep && !dent->prev_sweep);
  PIPE_MGR_DBGCHK(!dent->next_idle && !dent->prev_idle);

  /* Update the metadata */
  if (!pipe_mgr_idle_entry_process_mdata_move_entry(
          stage_info, ent_hdl, sent->index, dent->index)) {
    /* The copy is being done for an entry that was deleted */
    return PIPE_SUCCESS;
  }

  PIPE_MGR_DBGCHK(sent->inuse == true);
  PIPE_MGR_DBGCHK(sent->ent_hdl == ent_hdl);
  PIPE_MGR_DBGCHK(dent->inuse == false);

  int sindex = sent->index;
  int dindex = dent->index;
  PIPE_MGR_MEMCPY(dent, sent, sizeof(idle_entry_t));
  dent->index = dindex;
  sent->index = sindex;

  dent->next_active = NULL;
  dent->prev_active = NULL;
  dent->next_sweep = NULL;
  dent->prev_sweep = NULL;
  dent->next_idle = NULL;
  dent->prev_idle = NULL;

  pipe_mgr_idle_entry_add_to_stage_lists(stage_info, dent, __func__, __LINE__);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_move_entry_to_idle(
    idle_tbl_stage_info_t *stage_info, idle_entry_t *ient) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;

  if (ient->notify_state == NOTIFY_ENTRY_DISABLED) {
    return PIPE_SUCCESS;
  }
  /* Make sure that entry is active */
  PIPE_MGR_DBGCHK(ient->inuse);

  pipe_mgr_idle_entry_rem_from_stage_lists(stage_info, ient);

  PIPE_MGR_DBGCHK(!ient->next_active && !ient->prev_active);
  PIPE_MGR_DBGCHK(!ient->next_sweep && !ient->prev_sweep);
  PIPE_MGR_DBGCHK(!ient->next_idle && !ient->prev_idle);

  ient->cur_ttl = 0;
  ient->notify_state = NOTIFY_ENTRY_IDLE;
  pipe_mgr_idle_entry_add_to_stage_lists(stage_info, ient, __func__, __LINE__);
  /* Entry is going idle */
  /* In case of symmetric call the callback only
   * if the entry is idle in all the pipes
   */
  idle_tbl_t *idle_tbl = stage_info->idle_tbl_p;
  bf_dev_pipe_t pipe_id = idle_tbl->pipe_id;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info) &&
      IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_id = BF_DEV_PIPE_ALL;
  }

  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t ent_ttl = 0;
  rc = pipe_mgr_idle_get_ttl_helper(idle_tbl_info,
                                    ient->ent_hdl,
                                    pipe_id,
                                    stage_info->stage_id,
                                    stage_info->stage_table_handle,
                                    &ent_ttl,
                                    NULL);
  /* If the entry was deleted before the sweep, above function will
   * return PIPE_INVALID_ARG
   */

  if (rc == PIPE_SUCCESS && ent_ttl == 0) {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) "
        "Entry %d now idle in all pipes; called from pipe %d",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ient->ent_hdl,
        idle_tbl->pipe_id);
    PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
    bf_map_sts_t msts;
    /* If we are storing match specs to return in the age out callback check if
     * the entry handle is already in the map.  We will get here once per pipe
     * so we only want to lookup the match spec and get a copy the first time
     * through. */

    if (idle_tbl_info->tbl_params.u.notify.default_callback_choice == 1) {
      /* Check if the entry is already in the map.  If it is we are done.  If
       * not, get the match spec for the entry handle so we can save it for the
       * callback. */
      pipe_idle_time_hit_state_e *hs = NULL;
      msts = bf_map_get(&idle_tbl_info->notif_list,
                        (unsigned long)ient->ent_hdl,
                        (void **)&hs);
      if (msts == BF_MAP_OK) {
        if (*hs == ENTRY_IDLE) {
          /* Entry is already in the map, nothing else to do. */
          PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
          return PIPE_SUCCESS;
        }
        /* Replace notification with newer one */
        bf_map_rmv(&idle_tbl_info->notif_list, (unsigned long)ient->ent_hdl);
        bf_map_rmv(&idle_tbl_info->notif_list_spec_copy,
                   (unsigned long)ient->ent_hdl);
      }

      /* Lookup the entry's match spec. */
      pipe_tbl_match_spec_t const *match_spec_ref = NULL;
      rc = pipe_mgr_ent_hdl_to_match_spec_internal(idle_tbl_info->dev_id,
                                                   idle_tbl_info->tbl_hdl,
                                                   ient->ent_hdl,
                                                   NULL,
                                                   &match_spec_ref);
      if (rc == PIPE_OBJ_NOT_FOUND) {
        /* Entry was probably deleted. */
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Match spec not found for entry handle %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl);
      } else if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error getting match spec for entry %d rc %s",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            pipe_str_err(rc));
      }
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
        return rc;
      }

      /* Make a copy of the match spec since match_spec_ref is a reference to
       * the version stored by the table manager and it may be deleted by an
       * entry-delete API. */
      pipe_tbl_match_spec_t *match_spec_copy = NULL;
      rc = pipe_mgr_match_spec_duplicate(&match_spec_copy, match_spec_ref);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error duplicating entry %d match spec, %s",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            pipe_str_err(rc));
        PIPE_MGR_DBGCHK(0);
        PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
        return rc;
      }

      /* Add the entry to the notification map and the handle to match spec map.
       */
      msts = bf_map_add(
          &idle_tbl_info->notif_list, ient->ent_hdl, (void *)&g_state_idle);
      if (msts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error adding entry %d to notification list, error 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            msts);
        PIPE_MGR_DBGCHK(0);
      } else {
        msts = bf_map_add(&idle_tbl_info->notif_list_spec_copy,
                          ient->ent_hdl,
                          (void *)match_spec_copy);
        if (msts != BF_MAP_OK) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error adding entry %d to match spec list, error 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              ient->ent_hdl,
              msts);
          pipe_mgr_match_spec_free(match_spec_copy);
          PIPE_MGR_DBGCHK(0);
        }
      }
    } else {
      /* Aged entry callback does not require match specs, just add the entry
       * handle. */
      msts = bf_map_add(
          &idle_tbl_info->notif_list, ient->ent_hdl, (void *)&g_state_idle);
      if (msts == BF_MAP_KEY_EXISTS) {
        /* We will come to this function once per pipe the entry is in so
         * duplicates are okay. */
        msts = BF_MAP_OK;
      } else if (msts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error adding entry %d to notification list, error 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            msts);
        PIPE_MGR_DBGCHK(0);
      }
    }
    PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
    /* If there was an error we printed a log above, now that we released the
     * lock return an error as well. */
    if (msts != BF_MAP_OK) {
      return PIPE_UNEXPECTED;
    }
  }
  return rc;
}

static pipe_status_t pipe_mgr_idle_move_entry_to_active(
    idle_tbl_stage_info_t *stage_info, idle_entry_t *ient) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  bool needs_cb = false;
  if (ient->notify_state == NOTIFY_ENTRY_DISABLED) {
    return PIPE_SUCCESS;
  } else if (ient->notify_state == NOTIFY_ENTRY_IDLE) {
    // Only generate callbacks if entry was idle.
    needs_cb = true;
  }

  ient->cur_ttl = ient->init_ttl;
  pipe_mgr_idle_entry_rem_from_stage_lists(stage_info, ient);
  ient->notify_state = NOTIFY_ENTRY_ACTIVE;
  pipe_mgr_idle_entry_add_to_stage_lists(stage_info, ient, __func__, __LINE__);

  LOG_TRACE("%s:%d - %s (%d - 0x%x) Entry %d now active",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl);
  if (!needs_cb) return PIPE_SUCCESS;

  /* If we are storing match specs to return in the age out callback check if
   * the entry handle is already in the map.  We will get here once per pipe
   * so we only want to lookup the match spec and get a copy the first time
   * through. */
  bf_map_sts_t msts;
  pipe_status_t rc = PIPE_SUCCESS;
  PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
  if (idle_tbl_info->tbl_params.u.notify.default_callback_choice == 1) {
    /* Check if the entry is already in the map.  If it is we are done.  If
     * not, get the match spec for the entry handle so we can save it for the
     * callback. */
    pipe_idle_time_hit_state_e *hs = NULL;
    msts = bf_map_get(
        &idle_tbl_info->notif_list, (unsigned long)ient->ent_hdl, (void **)&hs);
    if (msts == BF_MAP_OK) {
      if (*hs == ENTRY_ACTIVE) {
        /* Entry is already in the map, nothing else to do. */
        PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
        return PIPE_SUCCESS;
      }
      /* Replace notification with newer one */
      bf_map_rmv(&idle_tbl_info->notif_list, (unsigned long)ient->ent_hdl);
      bf_map_rmv(&idle_tbl_info->notif_list_spec_copy,
                 (unsigned long)ient->ent_hdl);
    }

    /* Lookup the entry's match spec. */
    pipe_tbl_match_spec_t const *match_spec_ref = NULL;
    rc = pipe_mgr_ent_hdl_to_match_spec_internal(idle_tbl_info->dev_id,
                                                 idle_tbl_info->tbl_hdl,
                                                 ient->ent_hdl,
                                                 NULL,
                                                 &match_spec_ref);
    if (rc == PIPE_OBJ_NOT_FOUND) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting match spec for entry %d rc %s",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          pipe_str_err(rc));
    }
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      return rc;
    }

    /* Make a copy of the match spec since match_spec_ref is a reference to
     * the version stored by the table manager and it may be deleted by an
     * entry-delete API. */
    pipe_tbl_match_spec_t *match_spec_copy = NULL;
    rc = pipe_mgr_match_spec_duplicate(&match_spec_copy, match_spec_ref);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error duplicating entry %d match spec, %s",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          pipe_str_err(rc));
      PIPE_MGR_DBGCHK(0);
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      return rc;
    }

    /* Add the entry to the notification map and the handle to match spec map.
     */
    msts = bf_map_add(
        &idle_tbl_info->notif_list, ient->ent_hdl, (void *)&g_state_active);
    if (msts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding entry %d to notification list, error 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          msts);
      PIPE_MGR_DBGCHK(0);
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      return rc;
    }
    msts = bf_map_add(&idle_tbl_info->notif_list_spec_copy,
                      ient->ent_hdl,
                      (void *)match_spec_copy);
    if (msts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding entry %d to match spec list, error 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          msts);
      pipe_mgr_match_spec_free(match_spec_copy);
      PIPE_MGR_DBGCHK(0);
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      return rc;
    }
  } else {
    /* Active entry callback does not require match specs, just add the entry
     * handle. */
    msts = bf_map_add(
        &idle_tbl_info->notif_list, ient->ent_hdl, (void *)&g_state_active);
    if (msts == BF_MAP_KEY_EXISTS) {
      /* We will come to this function once per pipe the entry is in so
       * duplicates are okay. */
      msts = BF_MAP_OK;
    } else if (msts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding entry %d to notification list, error 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          msts);
      PIPE_MGR_DBGCHK(0);
    }
  }
  PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
  /* If there was an error we printed a log above, now that we released the
   * lock return an error as well. */
  if (msts != BF_MAP_OK) {
    return PIPE_UNEXPECTED;
  }

  return rc;
}

static pipe_status_t pipe_mgr_idle_deactivate_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    idle_entry_t *ient,
    bool is_del) {
  pipe_mgr_idle_entry_rem_from_stage_lists(stage_info, ient);

  PIPE_MGR_DBGCHK(!ient->next_active && !ient->prev_active);
  PIPE_MGR_DBGCHK(!ient->next_sweep && !ient->prev_sweep);
  PIPE_MGR_DBGCHK(!ient->next_idle && !ient->prev_idle);

  if (is_del) {
    pipe_mgr_idle_entry_process_mdata_del_entry(
        stage_info, ent_hdl, ient->index);
  }

  int index = ient->index;
  PIPE_MGR_MEMSET(ient, 0, sizeof(idle_entry_t));
  ient->index = index;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_activate_entry(
    idle_tbl_stage_info_t *stage_info,
    idle_entry_t *ient,
    idle_task_node_t *tnode) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;

  int index = ient->index;
  uint32_t new_ttl, cur_ttl;
  if (!pipe_mgr_idle_entry_process_mdata_entry_activate(
          stage_info, tnode->ent_hdl, index, &new_ttl, &cur_ttl)) {
    /* Entry deleted after adding */
    return PIPE_SUCCESS;
  }

  /* Make sure that dest entry is not active */
  PIPE_MGR_DBGCHK(!ient->inuse);
  PIPE_MGR_DBGCHK(!ient->next_active && !ient->prev_active);
  PIPE_MGR_DBGCHK(!ient->next_sweep && !ient->prev_sweep);
  PIPE_MGR_DBGCHK(!ient->next_idle && !ient->prev_idle);

  PIPE_MGR_MEMSET(ient, 0, sizeof(idle_entry_t));
  ient->index = index;

  ient->ent_hdl = tnode->ent_hdl;
  ient->poll_state = tnode->u.add.poll_state;
  ient->inuse = true;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    ient->init_ttl = new_ttl;
    ient->cur_ttl = cur_ttl;
    /* Move the entry to active */
    if (ient->init_ttl == 0) {
      PIPE_MGR_DBGCHK(ient->cur_ttl == 0);
      ient->notify_state = NOTIFY_ENTRY_DISABLED;
    } else if (ient->cur_ttl == 0) {
      ient->notify_state = NOTIFY_ENTRY_IDLE;
    } else if (ient->cur_ttl == ient->init_ttl) {
      ient->notify_state = NOTIFY_ENTRY_ACTIVE;
    } else {
      ient->notify_state = NOTIFY_ENTRY_SWEEP_CANDIDATE;
    }
    pipe_mgr_idle_entry_add_to_stage_lists(
        stage_info, ient, __func__, __LINE__);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_entry_process_update_ttl(
    idle_tbl_stage_info_t *stage_info, pipe_mat_ent_hdl_t ent_hdl, bool reset) {
  uint32_t init_ttl;
  idle_entry_location_t *ils = NULL, *il = NULL;
  if (!pipe_mgr_idle_entry_mdata_get(
          stage_info, ent_hdl, &init_ttl, NULL, &ils)) {
    /* The update is being done for an entry that was deleted
     * Or the entry hasn't been activated yet
     */
    return PIPE_SUCCESS;
  }

  for (il = ils; il; il = il->n) {
    if (il->del_in_progress || (il->index_valid == false)) {
      continue;
    }

    uint32_t index = il->cur_index;

    idle_entry_t *ient = pipe_mgr_idle_entry_get_by_index(stage_info, index);
    PIPE_MGR_DBGCHK(ient && ient->inuse);
    PIPE_MGR_DBGCHK(ient && ient->ent_hdl == ent_hdl);

    if (!ient) continue;

    uint32_t new_ttl = calc_cur_ttl(ient->init_ttl, ient->cur_ttl, init_ttl);
    if (reset) {
      new_ttl = init_ttl;
    }

    if ((new_ttl == 0) && (init_ttl != 0)) {
      /* Entry is going idle here */
      pipe_mgr_idle_move_entry_to_idle(stage_info, ient);
    } else {
      ient->init_ttl = init_ttl;
      ient->cur_ttl = new_ttl;
      pipe_mgr_idle_entry_rem_from_stage_lists(stage_info, ient);
      if (ient->init_ttl == 0) {
        PIPE_MGR_DBGCHK(ient->cur_ttl == 0);
        ient->notify_state = NOTIFY_ENTRY_DISABLED;
      } else if (ient->cur_ttl == 0) {
        ient->notify_state = NOTIFY_ENTRY_IDLE;
      } else if (ient->cur_ttl == ient->init_ttl) {
        ient->notify_state = NOTIFY_ENTRY_ACTIVE;
      } else {
        ient->notify_state = NOTIFY_ENTRY_SWEEP_CANDIDATE;
      }
      pipe_mgr_idle_entry_add_to_stage_lists(
          stage_info, ient, __func__, __LINE__);
    }
  }
  destroy_ils(ils);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_process_fsm_msg(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_dr_msg_t msg) {
  idle_entry_t *ient = NULL;
  if (IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    /* Ignore the message */
    return PIPE_SUCCESS;
  }
  int stage, pipe, stage_table_handle;
  int word, subword;
  uint32_t fsm_msg;
  extract_idle_fsm_msg(idle_tbl_info->dev_info->dev_family,
                       &msg,
                       &fsm_msg,
                       &pipe,
                       &stage,
                       &stage_table_handle,
                       &word);
  (void)stage;
  (void)pipe;
  (void)stage_table_handle;

  if (word >= stage_info->no_words) {
    PIPE_MGR_DBGCHK(word < stage_info->no_words);
    return PIPE_UNEXPECTED;
  }
  for (subword = 0; subword < stage_info->entries_per_word; subword++) {
    bool is_going_idle = fsm_msg & 0x1;
    bool is_going_active = (fsm_msg >> 1) & 0x1;
    PIPE_MGR_DBGCHK(stage_info->rmt_params.bit_width != 1);
    fsm_msg >>= next_pow2(stage_info->rmt_params.bit_width);

    /* Only one of them should be true */
    PIPE_MGR_DBGCHK(!(is_going_active && is_going_idle));

    ient = &stage_info->entries[word][subword];

    if ((ient->notify_state == NOTIFY_ENTRY_INVALID) ||
        (ient->notify_state == NOTIFY_ENTRY_DISABLED)) {
      continue;
    }

    if (is_going_active) {
      if (ient->notify_state == NOTIFY_ENTRY_ACTIVE) {
        // Old active notification. Ignore it and move on
        continue;
      }
      pipe_mgr_idle_move_entry_to_active(stage_info, ient);
    } else if (is_going_idle) {
      if (ient->notify_state == NOTIFY_ENTRY_IDLE ||
          ient->notify_state == NOTIFY_ENTRY_SWEEP_CANDIDATE) {
        /* update_ttl has already dealt with this entry earlier in the task
         * list. No need to process it here. */
        continue;
      } else if (ient->notify_state == NOTIFY_ENTRY_SWEEP) {
        /* Entry has already begun to sweep. Unexpected but harmless */
        LOG_WARN(
            "%s:%d - %s (%d - 0x%x) "
            "Entry hdl 0x%x state %s(%d) going idle index %d "
            "stage %d pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            ientry_notify_state_str(ient->notify_state),
            ient->notify_state,
            ient->index,
            stage_info->stage_id,
            pipe);
        continue;
      } else if (ient->notify_state != NOTIFY_ENTRY_ACTIVE) {
        /* Above conditions should guard against all other possibilities.
         * We've reached a bad race condition. */
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Entry hdl 0x%x state %s(%d) going idle index %d "
            "stage %d pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ient->ent_hdl,
            ientry_notify_state_str(ient->notify_state),
            ient->notify_state,
            ient->index,
            stage_info->stage_id,
            pipe);
        PIPE_MGR_DBGCHK(ient->notify_state == NOTIFY_ENTRY_ACTIVE);
      }
      PIPE_MGR_DBGCHK(ient->cur_ttl == ient->init_ttl);
      if (ient->cur_ttl <= stage_info->hw_notify_period) {
        /* Entry is going idle */
        pipe_mgr_idle_move_entry_to_idle(stage_info, ient);
      } else {
        ient->cur_ttl = ient->init_ttl - stage_info->hw_notify_period;
        /* Move the entry to sweep list
         */
        pipe_mgr_idle_entry_rem_from_stage_lists(stage_info, ient);
        ient->notify_state = NOTIFY_ENTRY_SWEEP_CANDIDATE;
        pipe_mgr_idle_entry_add_to_stage_lists(
            stage_info, ient, __func__, __LINE__);
      }
    }
  }

  /* After all the subwords are looked up, the fsm_msg should be 0 */
  PIPE_MGR_DBGCHK(fsm_msg == 0);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_process_task_list(idle_tbl_info_t *idle_tbl_info,
                                              idle_tbl_stage_info_t *stage_info,
                                              idle_task_list_t *tlist) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_task_node_t *tnode;

  for (tnode = tlist->tasks; tnode; tnode = tnode->next) {
    switch (tnode->type) {
      case IDLE_ENTRY_MOVE: {
        idle_entry_t *sent =
            pipe_mgr_idle_entry_get_by_index(stage_info, tnode->u.move.src_idx);
        idle_entry_t *dent = pipe_mgr_idle_entry_get_by_index(
            stage_info, tnode->u.move.dest_idx);

        if (sent == NULL || dent == NULL) {
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }

        /* Copy properties of src entry to dest entry */
        pipe_mgr_idle_copy_entry(stage_info, tnode->ent_hdl, sent, dent);

        /* Deactivate the src entry */
        rc = pipe_mgr_idle_deactivate_entry(stage_info, 0, sent, false);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error deactivating the src entry 0x%x for move rc %s",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              tnode->ent_hdl,
              pipe_str_err(rc));
          return rc;
        }
      }

      break;
      case IDLE_ENTRY_DEL: {
        idle_entry_t *ient =
            pipe_mgr_idle_entry_get_by_index(stage_info, tnode->u.del.del_idx);
        if (!ient) {
          LOG_ERROR("%s:%d Error, `ient` is NULL", __func__, __LINE__);
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }

        rc = pipe_mgr_idle_deactivate_entry(
            stage_info, tnode->ent_hdl, ient, true);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error deactivating the delete entry 0x%x rc %s",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              tnode->ent_hdl,
              pipe_str_err(rc));
          return rc;
        }
      }

      break;
      case IDLE_ENTRY_ADD: {
        idle_entry_t *ient =
            pipe_mgr_idle_entry_get_by_index(stage_info, tnode->u.add.add_idx);
        if (ient == NULL || ient->inuse) {
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }

        rc = pipe_mgr_idle_activate_entry(stage_info, ient, tnode);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error activating the add entry 0x%x rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              tnode->ent_hdl,
              rc);
          return rc;
        }
      } break;
      case IDLE_ENTRY_UPDATE_TTL: {
        rc = pipe_mgr_idle_entry_process_update_ttl(
            stage_info, tnode->ent_hdl, false);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error updating the ttl entry 0x%x rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              tnode->ent_hdl,
              rc);
          return rc;
        }
      } break;
      case IDLE_ENTRY_RESET_TTL: {
        rc = pipe_mgr_idle_entry_process_update_ttl(
            stage_info, tnode->ent_hdl, true);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error resetting the ttl entry 0x%x rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              tnode->ent_hdl,
              rc);
          return rc;
        }
      } break;
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_process_lock_complete(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    lock_id_t lock_id,
    bool lock_valid) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_task_list_t *tlist;
  bool lock_done = false;

  PIPE_MGR_LOCK(&stage_info->tlist_mtx);
  tlist = stage_info->task_list;
  if (!tlist) {
    if (lock_valid) {
      PIPE_MGR_DBGCHK(!lock_valid);
      PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
      return PIPE_UNEXPECTED;
    }
    PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
    return PIPE_SUCCESS;
  }
  PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);

  do {
    /* The lock id should be same as in the FIFO */
    if (tlist->lock_valid) {
      if (lock_valid) {
        if (tlist->lock_id != lock_id) {
          PIPE_MGR_DBGCHK(tlist->lock_id == lock_id);
          return PIPE_UNEXPECTED;
        }
        lock_done = true;
      } else {
        break;
      }
    }

    rc = pipe_mgr_idle_process_task_list(idle_tbl_info, stage_info, tlist);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error processing pending items for idle time"
          "pipe %d stage %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_info->idle_tbl_p->pipe_id,
          stage_info->stage_id,
          rc);
      return rc;
    }

    PIPE_MGR_LOCK(&stage_info->tlist_mtx);
    /* Dequeue the item from the move list. The deque is done
     * after processing because the add entry will
     * be checking if the move-list is empty.
     */
    BF_LIST_DLL_REM(stage_info->task_list, tlist, next, prev);

    pipe_mgr_idle_destroy_task_list(tlist);
    tlist = stage_info->task_list;
    if (!tlist) {
      PIPE_MGR_COND_SIGNAL(&stage_info->tlist_cvar);
    }
    PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
  } while (tlist && (!tlist->lock_valid || !lock_done));

  if (lock_valid) {
    PIPE_MGR_DBGCHK(lock_done);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_process_update_complete(
    idle_tbl_info_t *idle_tbl_info) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_task_list_t *tlist = NULL;
  if (!IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    return PIPE_SUCCESS;
  }

  if (!idle_tbl_info->update_in_progress) {
    PIPE_MGR_DBGCHK(idle_tbl_info->update_in_progress);
    return PIPE_UNEXPECTED;
  }
  idle_tbl_info->update_count++;

  if (idle_tbl_info->update_count == idle_tbl_info->stage_count) {
    /* Process task list once more for every table and every stage. */
    for (int j = 0; j < idle_tbl_info->no_idle_tbls; j++) {
      idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[j];
      for (int i = 0; i < idle_tbl->num_stages; i++) {
        idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];

        PIPE_MGR_LOCK(&stage_info->tlist_mtx);
        tlist = stage_info->task_list;
        while (tlist) {
          rc =
              pipe_mgr_idle_process_task_list(idle_tbl_info, stage_info, tlist);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d - %s (%d - 0x%x) "
                "Error processing pending items for idle time"
                "pipe %d stage %d rc 0x%x",
                __func__,
                __LINE__,
                idle_tbl_info->name,
                idle_tbl_info->dev_id,
                idle_tbl_info->tbl_hdl,
                stage_info->idle_tbl_p->pipe_id,
                stage_info->stage_id,
                rc);
            break;
          }
          BF_LIST_DLL_REM(stage_info->task_list, tlist, next, prev);
          pipe_mgr_idle_destroy_task_list(tlist);
          tlist = stage_info->task_list;
        }
        PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
      }
    }
    idle_tbl_info->update_in_progress = false;
    idle_tbl_info->update_count = 0;
    idle_tbl_info->update_complete_cb_fn(
        idle_tbl_info->dev_id, idle_tbl_info->update_complete_cb_data);
  }
  return rc;
}

static pipe_status_t pipe_mgr_idle_process_lock_ack_msg(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_dr_msg_t msg) {
  pipe_status_t rc = PIPE_SUCCESS;
  int stage, pipe, stage_table_handle;
  lock_id_t lock_id;
  extract_idle_lock_msg(idle_tbl_info->dev_info->dev_id,
                        &msg,
                        &lock_id,
                        &pipe,
                        &stage,
                        &stage_table_handle);
  (void)stage;
  (void)pipe;
  (void)stage_table_handle;

  /* If the lock was a barrier issued for dump-msg in poll mode,
   * the completion callback needs to be called if all the stages
   * have received the barrier back.
   */

  pipe_mgr_lock_id_type_e lock_id_type = PIPE_MGR_GET_LOCK_ID_TYPE(lock_id);

  switch (lock_id_type) {
    case LOCK_ID_TYPE_STAT_BARRIER:
      /* Ignore */
      break;
    case LOCK_ID_TYPE_IDLE_LOCK:
    case LOCK_ID_TYPE_ALL_LOCK:
    case LOCK_ID_TYPE_IDLE_BARRIER:
      if ((idle_tbl_info->update_in_progress == true) &&
          (lock_id == idle_tbl_info->update_barrier_lock_id)) {
        rc = pipe_mgr_idle_process_update_complete(idle_tbl_info);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error processing idle-time state update complete msg on "
              "pipe %d stage %d rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              stage_info->idle_tbl_p->pipe_id,
              stage_info->stage_id,
              rc);
          return rc;
        }
      } else {
        rc = pipe_mgr_idle_process_lock_complete(
            idle_tbl_info, stage_info, lock_id, true);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error processing idle-time move complete msg on pipe %d "
              "stage %d rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              stage_info->idle_tbl_p->pipe_id,
              stage_info->stage_id,
              rc);
          return rc;
        }
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Invalid lock-ack msg received with id 0x%x on pipe %d "
          "stage %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          lock_id,
          stage_info->idle_tbl_p->pipe_id,
          stage_info->stage_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_process_dump_msg(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_dr_msg_t msg) {
  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    return PIPE_SUCCESS;
  }

  int stage, pipe, stage_table_handle;
  int word, subword;
  uint32_t dump_word;
  extract_idle_dump_msg(idle_tbl_info->dev_info->dev_family,
                        &msg,
                        &dump_word,
                        &pipe,
                        &stage,
                        &stage_table_handle,
                        &word);
  (void)stage;
  (void)pipe;
  (void)stage_table_handle;

  if (!(word < stage_info->no_words)) {
    PIPE_MGR_DBGCHK(word < stage_info->no_words);
    return PIPE_UNEXPECTED;
  }
  for (subword = 0; subword < stage_info->entries_per_word; subword++) {
    uint8_t value = dump_word & ((1 << stage_info->rmt_params.bit_width) - 1);
    dump_word >>= stage_info->rmt_params.bit_width;

    idle_entry_t *ient = &stage_info->entries[word][subword];
    idle_tbl_t *idle_tbl = stage_info->idle_tbl_p;
    uint32_t pipe_count = PIPE_BITMAP_COUNT(&idle_tbl->inst_pipe_bmp);
    if (!pipe_count) {
      PIPE_MGR_DBGCHK(pipe_count);
      pipe_count = 1;
    }

    if (pipe_mgr_idle_is_state_active(stage_info->rmt_params, value)) {
      /* Active mode */
      ient->poll_state = ENTRY_ACTIVE;
    } else {
      /* update this to idle only if
       * update_count == 0
       */
      if ((ient->update_count == 0)) {
        /* Idle mode */
        ient->poll_state = ENTRY_IDLE;
      }
    }

    ient->update_count = (ient->update_count + 1) % pipe_count;
  }

  PIPE_MGR_DBGCHK(dump_word == 0);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_process_pending_msg(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_pending_msgs_t *pending_msg) {
  pipe_status_t rc = PIPE_SUCCESS;
  /* First process all the tasks with no locks held */
  rc = pipe_mgr_idle_process_lock_complete(idle_tbl_info, stage_info, 0, false);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  /* Process pending message */
  uint32_t i;
  for (i = 0; i < pending_msg->msg_count; i++) {
    idle_dr_msg_t msg = pending_msg->msgs[i];
    idle_time_msg_type_e type;
    type = extract_idle_type(idle_tbl_info->dev_info->dev_family, &msg);
    switch (type) {
      case IDLE_FSM_MSG:
        rc = pipe_mgr_idle_process_fsm_msg(idle_tbl_info, stage_info, msg);
        break;
      case IDLE_LOCK_ACK_MSG:
        rc = pipe_mgr_idle_process_lock_ack_msg(idle_tbl_info, stage_info, msg);
        break;
      case IDLE_DUMP_MSG:
        rc = pipe_mgr_idle_process_dump_msg(idle_tbl_info, stage_info, msg);
        break;
      default:
        PIPE_MGR_DBGCHK(0 && "Invalid idle msg received from hw");
        return PIPE_UNEXPECTED;
    }

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error processing idle-time msg of type %d on pipe %d stage %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          type,
          stage_info->idle_tbl_p->pipe_id,
          stage_info->stage_id);
    }
  }

  return PIPE_SUCCESS;
}

static void call_user_cb(idle_tbl_info_t *idle_tbl_info) {
  pipe_mat_ent_hdl_t ent_hdl;
  unsigned long key;
  pipe_idle_time_hit_state_e *hs = NULL;
  pipe_tbl_match_spec_t *match_spec_allocated;
  bf_map_sts_t msts;

  if (idle_tbl_info->tbl_params.u.notify.default_callback_choice == 0) {
    PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
    while ((msts = bf_map_get_first_rmv(
                &idle_tbl_info->notif_list, &key, (void **)&hs)) == BF_MAP_OK) {
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      ent_hdl = key;
      LOG_TRACE(
          "%s:%d - %s (%d - 0x%x) Entry hdl 0x%x is idle Calling user-callback",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ent_hdl);

      /* In case of restore of the table callbacks may be not set */
      if (idle_tbl_info->tbl_params.u.notify.callback_fn) {
        idle_tbl_info->tbl_params.u.notify.callback_fn(
            idle_tbl_info->dev_id,
            ent_hdl,
            *hs,
            idle_tbl_info->tbl_params.u.notify.client_data);
      }

      PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
      if (IDLE_TBL_SEND_REPEATED_NOTIF(idle_tbl_info) && *hs == ENTRY_IDLE) {
        bf_map_add(&idle_tbl_info->notif_list_old, ent_hdl, hs);
      }
    }
    idle_tbl_info->notif_list = idle_tbl_info->notif_list_old;
    bf_map_init(&idle_tbl_info->notif_list_old);
    PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
  } else {
    PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
    while ((msts = bf_map_get_first_rmv(&idle_tbl_info->notif_list_spec_copy,
                                        &key,
                                        (void **)&match_spec_allocated)) ==
           BF_MAP_OK) {
      ent_hdl = key;
      /* Remove entry from standard list if exist */
      bf_map_get_rmv(&idle_tbl_info->notif_list, ent_hdl, (void **)&hs);
      PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
      LOG_TRACE(
          "%s:%d - %s (%d - 0x%x) Entry hdl 0x%x is idle Calling "
          "user-callback_fn2",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ent_hdl);
      /* In case of restore of the table callbacks may be not set */
      if (idle_tbl_info->tbl_params.u.notify.callback_fn2) {
        idle_tbl_info->tbl_params.u.notify.callback_fn2(
            idle_tbl_info->dev_id,
            ent_hdl,
            *hs,
            match_spec_allocated,
            idle_tbl_info->tbl_params.u.notify.client_data);
      }
      PIPE_MGR_LOCK(&idle_tbl_info->notif_list_mtx);
    }
    bf_map_destroy(&idle_tbl_info->notif_list_spec_copy);
    bf_map_init(&idle_tbl_info->notif_list_spec_copy);
    PIPE_MGR_UNLOCK(&idle_tbl_info->notif_list_mtx);
  }
}

/* Timer sweep */
void pipe_mgr_idle_sw_timer_cb(bf_sys_timer_t *timer, void *data) {
  idle_tbl_stage_info_t *stage_info = data;
  pipe_status_t rc = PIPE_SUCCESS;
  (void)timer;

  if (stage_info == NULL) {
    LOG_ERROR("%s:%d NULL Timer callback data passed", __func__, __LINE__);
    return;
  }

  uint8_t stage_idx = stage_info->stage_idx;
  idle_tbl_t *idle_tbl = stage_info->idle_tbl_p;
  idle_tbl_info_t *idle_tbl_info = idle_tbl->idle_tbl_info;

  PIPE_MGR_RW_RDLOCK(&idle_tbl_info->en_dis_rw_lock);

  PIPE_MGR_DBGCHK(IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info));

  int i;
  for (i = 0; i < idle_tbl_info->no_idle_tbls; i++) {
    idle_tbl = &idle_tbl_info->idle_tbls[i];
    stage_info = &idle_tbl->stage_info[stage_idx];

    /* Find the stage with the same stage_idx */

    /* Process any tasks in this pipe and stage which do NOT require an ACK from
     * the chip. */
    rc = pipe_mgr_idle_process_lock_complete(
        idle_tbl_info, stage_info, 0, false);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

    /* First process all the pending notifications */
    PIPE_MGR_LOCK(&stage_info->pmsg_mtx);
    /* The messages that arrive during this processing
     * can be held off until next sweep
     */
    idle_pending_msgs_t *pending_msgs = stage_info->pending_msgs;
    stage_info->pending_msgs = NULL;
    PIPE_MGR_UNLOCK(&stage_info->pmsg_mtx);

    while (pending_msgs) {
      idle_pending_msgs_t *pmsg = pending_msgs;

      BF_LIST_DLL_REM(pending_msgs, pmsg, next, prev);

      rc = pipe_mgr_idle_process_pending_msg(idle_tbl_info, stage_info, pmsg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table error processing hw pending msgs on pipe %d "
            "stage %d rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            idle_tbl->pipe_id,
            stage_info->stage_id,
            rc);
        PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);
        return;
      }

      PIPE_MGR_FREE(pmsg->msgs);
      PIPE_MGR_FREE(pmsg);
    }

    /* Now decrement the TTL of all the entries in this stage */
    uint32_t sw_sweep_period = stage_info->sw_sweep_period;
    idle_entry_t *ient = stage_info->sweeps;

    while (ient) {
      idle_entry_t *ient_next = ient->next_sweep;

      PIPE_MGR_DBGCHK(ient->notify_state == NOTIFY_ENTRY_SWEEP);
      if (ient->cur_ttl <= sw_sweep_period) {
        pipe_mgr_idle_move_entry_to_idle(stage_info, ient);
      } else {
        ient->cur_ttl -= sw_sweep_period;
      }
      ient = ient_next;
    }

    /* Now move all the entries in the sweep_candidate list to the sweep list
     */
    for (ient = stage_info->sweep_candidates; ient; ient = ient->next_sweep) {
      LOG_TRACE(
          "%s:%d - %s (%d - 0x%x) "
          "Entry hdl 0x%x state %s moving to sweep index %d "
          "stage %d pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ient->ent_hdl,
          ientry_notify_state_str(ient->notify_state),
          ient->index,
          stage_info->stage_id,
          idle_tbl->pipe_id);
      ient->notify_state = NOTIFY_ENTRY_SWEEP;
    }
    BF_LIST_DLL_CAT(stage_info->sweeps,
                    stage_info->sweep_candidates,
                    next_sweep,
                    prev_sweep);
  }
  PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);

  call_user_cb(idle_tbl_info);

  return;
}

static bool pipe_mgr_idle_needs_inline_processing(
    idle_tbl_info_t *idle_tbl_info, idle_tbl_stage_info_t *stage_info) {
  if (IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    return true;
  }

  if (stage_info->sw_sweep_period) {
    return false;
  }
  return true;
}

static pipe_status_t pipe_mgr_idle_enque_msg_for_later(
    idle_tbl_stage_info_t *stage_info, idle_pending_msgs_t *pending_msg) {
  if (!stage_info->sw_sweep_period) {
    PIPE_MGR_DBGCHK(stage_info->sw_sweep_period);
    return PIPE_UNEXPECTED;
  }

  PIPE_MGR_LOCK(&stage_info->pmsg_mtx);
  BF_LIST_DLL_AP(stage_info->pending_msgs, pending_msg, next, prev);
  PIPE_MGR_UNLOCK(&stage_info->pmsg_mtx);

  return PIPE_SUCCESS;
}

/* Notification handler */
void pipe_mgr_drv_idle_cb(bf_dev_id_t logical_device,
                          bf_subdev_id_t subdev_id,
                          int size,
                          bf_dma_addr_t dma_addr) {
  bf_dev_id_t dev_id = logical_device;
  pipe_status_t ret = PIPE_SUCCESS;
  int push_cnt = 0;
  bool dr_push_err = false;
  lld_err_t x = LLD_OK;
  int buf_sz =
      pipe_mgr_drv_subdev_buf_size(dev_id, subdev_id, PIPE_MGR_DRV_BUF_IDL);
  bf_sys_dma_pool_handle_t hndl = pipe_mgr_drv_subdev_dma_pool_handle(
      logical_device, subdev_id, PIPE_MGR_DRV_BUF_IDL);
  bf_dma_addr_t addr_dma;
  /* If we don't have the device info then drop this message since the device
   * isn't around anymore. */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);

  if (!dev_info) {
    LOG_ERROR("Invalid device %d at %s:%d", logical_device, __func__, __LINE__);
    PIPE_MGR_DBGCHK(dev_info)
    return;
  }
  /* Make sure this is a buffer we are expecting. */
  idle_mgr_dev_ctx_t *ctx = idle_mgr_dev_ctx(dev_id);
  if (ctx->buf_addrs[subdev_id][ctx->buf_addr_idx[subdev_id]] != dma_addr) {
    /* First try failed, go ahead and look through all the buffers :( */
    uint32_t i;
    for (i = 0; i < ctx->buf_cnt[subdev_id]; ++i) {
      if (ctx->buf_addrs[subdev_id][i] == dma_addr) break;
    }
    if (i == ctx->buf_cnt[subdev_id]) {
      /* Didn't find the buffer's address in our list of expected addresses so
       * just ignore it. */
      LOG_ERROR("Unexpected idle notify buffer received, dev %d addr %" PRIx64,
                dev_id,
                (uint64_t)dma_addr);
      return;
    }
  }

  uint32_t num_pipes = dev_info->num_active_pipes;
  int num_stages = dev_info->num_active_mau;
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  bf_map_t needs_cb;
  bf_map_init(&needs_cb);

  /* An array on the stack to hold the counts of the number of messages in the
   * DMA buffer. */
  int entry_count[num_pipes][num_stages][num_lts];
  for (bf_dev_pipe_t p = 0; p < num_pipes; p++) {
    for (int i = 0; i < num_stages; i++) {
      for (int j = 0; j < num_lts; j++) {
        entry_count[p][i][j] = 0;
      }
    }
  }

  /* A list head data structure to hold the list of messages for a pipe/stage/
   * logical table. */
  idle_pending_msgs_t *pending_msgs[num_pipes][num_stages][num_lts];
  for (bf_dev_pipe_t p = 0; p < num_pipes; p++) {
    for (int i = 0; i < num_stages; i++) {
      for (int j = 0; j < num_lts; j++) {
        pending_msgs[p][i][j] = NULL;
      }
    }
  }

  /* Unmap the DMA address and get the virtual address of the buffer before it
     is used by the pipe manager */
  uint8_t *addr = bf_mem_dma2virt(hndl, dma_addr);
  if (addr != NULL) {
    if (bf_sys_dma_unmap(hndl, addr, buf_sz, BF_DMA_TO_CPU) != 0) {
      LOG_ERROR(
          "Unable to unmap DMA buffer %p at %s:%d", addr, __func__, __LINE__);
    }
  } else {
    LOG_ERROR("Invalid virtual address derived from %" PRIx64 " at %s:%d",
              (uint64_t)dma_addr,
              __func__,
              __LINE__);
    /* Not much to do here.  We know it is an expected buffer and has data but
     * we can't process it because we don't know the virtual address. */
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }
  /* If we don't have whole messages then this isn't a valid buffer, send it
   * back to the chip. */
  if (!size || (size % sizeof(idle_dr_msg_t))) {
    LOG_ERROR(
        "%s:%d Invalid msg addr %p size %d", __func__, __LINE__, addr, size);
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }

  /* Parse each message received and count the total messages per pipe per
   * state per logical table. */
  int count = size / sizeof(idle_dr_msg_t);
  idle_dr_msg_t *msg = (idle_dr_msg_t *)addr;
  for (int i = 0; i < count; i++, msg++) {
    idle_time_msg_type_e type;
    type = extract_idle_type(dev_info->dev_family, msg);
    int stage, phy_pipe, stage_table_handle;
    switch (type) {
      case IDLE_FSM_MSG: {
        int word;
        uint32_t idle_msg;
        extract_idle_fsm_msg(dev_info->dev_family,
                             msg,
                             &idle_msg,
                             &phy_pipe,
                             &stage,
                             &stage_table_handle,
                             &word);
        (void)word;
        (void)idle_msg;
        /*
        LOG_TRACE(
            "Idle FSM msg received 0x%02x phy_pipe %d stage %d "
            "stage_table_handle %d word %d",
            idle_msg,
            phy_pipe,
            stage,
            stage_table_handle,
            word);
        */
      } break;
      case IDLE_LOCK_ACK_MSG: {
        lock_id_t lock_id;
        extract_idle_lock_msg(dev_info->dev_family,
                              msg,
                              &lock_id,
                              &phy_pipe,
                              &stage,
                              &stage_table_handle);
        (void)lock_id;
        /*
        LOG_TRACE(
            "Idle LOCK ACK received 0x%04x phy_pipe %d stage %d "
            "stage_table_handle %d",
            lock_id,
            phy_pipe,
            stage,
            stage_table_handle);
        */
      } break;
      case IDLE_DUMP_MSG: {
        int word;
        uint32_t dump_word;
        extract_idle_dump_msg(dev_info->dev_family,
                              msg,
                              &dump_word,
                              &phy_pipe,
                              &stage,
                              &stage_table_handle,
                              &word);
        (void)word;
        (void)dump_word;
        /*
        LOG_TRACE(
            "Idle Dump msg received 0x%02x phy_pipe %d stage %d "
            "stage_table_handle %d word %d",
            dump_word,
            phy_pipe,
            stage,
            stage_table_handle,
            word);
        */
      } break;
      default:
        PIPE_MGR_DBGCHK(0 && "Invalid idle msg received from hw");
        goto cleanup;
    }
    phy_pipe += subdev_id * BF_SUBDEV_PIPE_COUNT;
    bf_dev_pipe_t pipe;
    ret = pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
        dev_info, phy_pipe, &pipe);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to get logical pipe for physical pipe %d on dev %d, "
          "%s",
          __func__,
          __LINE__,
          phy_pipe,
          dev_id,
          pipe_str_err(ret));
      PIPE_MGR_DBGCHK(0);
      continue;
    }
    /* Some sanity, if the message isn't correct then skip the entire buffer.
     */
    if (!(pipe < (unsigned int)num_pipes) || !(stage < num_stages) ||
        !(stage_table_handle < num_lts)) {
      PIPE_MGR_DBGCHK(pipe < (unsigned int)num_pipes);
      PIPE_MGR_DBGCHK(stage < num_stages);
      PIPE_MGR_DBGCHK(stage_table_handle < num_lts);
      goto cleanup;
    } else {
      entry_count[pipe][stage][stage_table_handle]++;
    }
  }

  /* Allocate space to hold the messages for each pipe/stage/logical table. */
  for (bf_dev_pipe_t p = 0; p < num_pipes; p++) {
    for (int i = 0; i < num_stages; i++) {
      for (int j = 0; j < num_lts; j++) {
        if (entry_count[p][i][j]) {
          idle_pending_msgs_t *pmsg = NULL;
          pmsg = (idle_pending_msgs_t *)PIPE_MGR_MALLOC(
              sizeof(idle_pending_msgs_t));
          if (!pmsg) {
            LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
            goto cleanup;
          }

          PIPE_MGR_MEMSET(pmsg, 0, sizeof(idle_pending_msgs_t));

          pmsg->msgs = (idle_dr_msg_t *)PIPE_MGR_MALLOC(sizeof(idle_dr_msg_t) *
                                                        entry_count[p][i][j]);
          if (!pmsg->msgs) {
            LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
            goto cleanup;
          }
          PIPE_MGR_MEMSET(
              pmsg->msgs, 0, sizeof(idle_dr_msg_t) * entry_count[p][i][j]);
          pending_msgs[p][i][j] = pmsg;
        }
      }
    }
  }

  /* Parse the messages again and copy them into lists per pipe, stage, and
   * logical table. */
  msg = (idle_dr_msg_t *)addr;
  for (int i = 0; i < count; i++, msg++) {
    idle_time_msg_type_e type;
    type = extract_idle_type(dev_info->dev_family, msg);
    int stage, phy_pipe, stage_table_handle;
    switch (type) {
      case IDLE_FSM_MSG: {
        int word;
        uint32_t idle_msg;
        extract_idle_fsm_msg(dev_info->dev_family,
                             msg,
                             &idle_msg,
                             &phy_pipe,
                             &stage,
                             &stage_table_handle,
                             &word);
        (void)word;
        (void)idle_msg;
      } break;
      case IDLE_LOCK_ACK_MSG: {
        lock_id_t lock_id;
        extract_idle_lock_msg(dev_info->dev_family,
                              msg,
                              &lock_id,
                              &phy_pipe,
                              &stage,
                              &stage_table_handle);
        (void)lock_id;
      } break;
      case IDLE_DUMP_MSG: {
        int word;
        uint32_t dump_word;
        extract_idle_dump_msg(dev_info->dev_family,
                              msg,
                              &dump_word,
                              &phy_pipe,
                              &stage,
                              &stage_table_handle,
                              &word);
        (void)word;
        (void)dump_word;
      } break;
      default:
        /* We already checked the messages once, we should never come here. */
        PIPE_MGR_DBGCHK(0 && "Invalid idle msg received from hw");
        goto cleanup;
    }
    phy_pipe += subdev_id * BF_SUBDEV_PIPE_COUNT;
    bf_dev_pipe_t pipe;
    ret = pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
        dev_info, phy_pipe, &pipe);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to get logical pipe for physical pipe %d on dev %d, "
          "%s",
          __func__,
          __LINE__,
          phy_pipe,
          dev_id,
          pipe_str_err(ret));
      PIPE_MGR_DBGCHK(0);
      continue;
    }
    /* Some sanity, if the message isn't correct then skip it. */
    if (!(pipe < (unsigned int)num_pipes) || !(stage < num_stages) ||
        !(stage_table_handle < num_lts)) {
      continue;
    } else {
      PIPE_MGR_MEMCPY(
          &pending_msgs[pipe][stage][stage_table_handle]
               ->msgs[pending_msgs[pipe][stage][stage_table_handle]->msg_count],
          msg,
          sizeof(idle_dr_msg_t));
      pending_msgs[pipe][stage][stage_table_handle]->msg_count++;
    }
  }

  /* Wait until it is our turn to process this DMA buffer.  They must be
   * processed in order and the hardware will return them in order but with
   * multiple threads processing them that order can be lost without this
   * check
   * against the DMA buffer's address. */
  PIPE_MGR_LOCK(&ctx->buf_addrs_mtx[subdev_id]);
  while (ctx->buf_addrs[subdev_id][ctx->buf_addr_idx[subdev_id]] != dma_addr) {
    PIPE_MGR_COND_WAIT(&ctx->buf_addrs_cv[subdev_id],
                       &ctx->buf_addrs_mtx[subdev_id]);
  }

  /* Now stick the pending msgs onto the individual stage info's.
   * Requires locking
   */
  for (bf_dev_pipe_t pipe = 0; pipe < num_pipes; pipe++) {
    for (int i = 0; i < num_stages; i++) {
      for (int j = 0; j < num_lts; j++) {
        if (entry_count[pipe][i][j]) {
          PIPE_MGR_DBGCHK(pending_msgs[pipe][i][j]);

          idle_tbl_info_t *idle_tbl_info = ctx->tbl_lookup[pipe][i][j];
          if (!idle_tbl_info) {
            LOG_ERROR(
                "%s:%d Idle table does not exist on pipe %d stage %d "
                "stage_table_handle %d",
                __func__,
                __LINE__,
                pipe,
                i,
                j);
            continue;
          }

          PIPE_MGR_RW_RDLOCK(&idle_tbl_info->en_dis_rw_lock);
          idle_tbl_stage_info_t *stage_info =
              pipe_mgr_idle_tbl_stage_info_get(idle_tbl_info, pipe, i, j);
          if (!stage_info) {
            LOG_ERROR(
                "%s:%d - %s (%d - 0x%x) Idle table stage %d does not exist "
                "on "
                "pipe %d",
                __func__,
                __LINE__,
                idle_tbl_info->name,
                idle_tbl_info->dev_id,
                idle_tbl_info->tbl_hdl,
                i,
                pipe);
            PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);
            continue;
          }

          if (pipe_mgr_idle_needs_inline_processing(idle_tbl_info,
                                                    stage_info)) {
            pipe_mgr_idle_process_pending_msg(
                idle_tbl_info, stage_info, pending_msgs[pipe][i][j]);
            PIPE_MGR_FREE(pending_msgs[pipe][i][j]->msgs);
            PIPE_MGR_FREE(pending_msgs[pipe][i][j]);
            bf_map_add(&needs_cb, (unsigned long)idle_tbl_info, NULL);
            // call_user_cb(idle_tbl_info);
          } else {
            pipe_mgr_idle_enque_msg_for_later(stage_info,
                                              pending_msgs[pipe][i][j]);
          }

          PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);
          pending_msgs[pipe][i][j] = NULL;
        }
        PIPE_MGR_DBGCHK(!pending_msgs[pipe][i][j]);
      }
    }
  }

  PIPE_MGR_UNLOCK(&ctx->buf_addrs_mtx[subdev_id]);

cleanup:
  /* Return this buffer to the free memory DR.  Since we don't want to reorder
   * our list of expected DMA buffers we can only push the buffer into the
   * free
   * memory DR when it is the expected buffer. */
  PIPE_MGR_LOCK(&ctx->buf_addrs_mtx[subdev_id]);
  while (ctx->buf_addrs[subdev_id][ctx->buf_addr_idx[subdev_id]] != dma_addr) {
    PIPE_MGR_COND_WAIT(&ctx->buf_addrs_cv[subdev_id],
                       &ctx->buf_addrs_mtx[subdev_id]);
  }

  do {
    /* Remap the virtual address of the buffer to the DMA address before it is
       pushed again into the free memory DR */
    if (bf_sys_dma_map(hndl,
                       addr,
                       (bf_phys_addr_t)dma_addr,
                       buf_sz,
                       &addr_dma,
                       BF_DMA_TO_CPU) != 0) {
      LOG_ERROR(
          "Unable to map DMA buffer %p at %s:%d", addr, __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      bf_map_destroy(&needs_cb);
      return;
    }
    x = lld_subdev_push_fm(dev_id, subdev_id, lld_dr_fm_idle, addr_dma, buf_sz);
    if (x != LLD_OK) {
      /* Unmap the buffer */
      if (bf_sys_dma_unmap(hndl, addr, buf_sz, BF_DMA_TO_CPU) != 0) {
        LOG_ERROR(
            "Unable to unmap DMA buffer %p at %s:%d", addr, __func__, __LINE__);
      }
      ++push_cnt;
      if (!dr_push_err) {
        LOG_ERROR(
            "%s Error pushing idle free memory to device %d rc %d addr "
            "0x%" PRIx64 " size %d",
            __func__,
            dev_id,
            x,
            addr_dma,
            buf_sz);
        dr_push_err = true;
      }
    } else if (dr_push_err) {
      LOG_ERROR(
          "%s:%d Retry pushing idle free memory to device %d successful, try "
          "%d",
          __func__,
          __LINE__,
          dev_id,
          push_cnt);
    }
  } while (x == LLD_ERR_DR_FULL && push_cnt < 1000);

  PIPE_MGR_DBGCHK(x == LLD_OK);
  /* Now that the buffer has been processed and returned to hardware advance
   * the
   * next expected index. */
  ctx->buf_addr_idx[subdev_id] =
      (ctx->buf_addr_idx[subdev_id] + 1) % ctx->buf_cnt[subdev_id];
  PIPE_MGR_COND_BROADCAST_SIGNAL(&ctx->buf_addrs_cv[subdev_id]);
  PIPE_MGR_UNLOCK(&ctx->buf_addrs_mtx[subdev_id]);

  /* Push everything */
  pipe_mgr_drv_push_idle_time_drs(dev_id);

  /* Invoke any user callbacks if needed. */
  unsigned long key;
  void *value;
  while (BF_MAP_OK == bf_map_get_first_rmv(&needs_cb, &key, &value)) {
    idle_tbl_info_t *idle_tbl_info = (idle_tbl_info_t *)key;
    call_user_cb(idle_tbl_info);
  }
  bf_map_destroy(&needs_cb);
  return;
}
