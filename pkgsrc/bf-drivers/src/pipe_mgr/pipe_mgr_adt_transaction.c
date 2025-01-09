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
 * @file pipe_mgr_adt_transaction.c
 * @date
 *
 * Action data table transaction handling.
 * Contains implementation of transaction commit/abort semantics for action
 * data table.
 */

/* Standard header includes */
#include <sys/types.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <target-utils/id/id.h>

/* Local header includes */
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_transaction.h"

pipe_status_t pipe_mgr_adt_txn_commit(bf_dev_id_t dev_id,
                                      pipe_adt_tbl_hdl_t tbl_hdl,
                                      bf_dev_pipe_t *pipes_list,
                                      unsigned nb_pipes)

{
  uint8_t stage_idx = 0;
  unsigned long key = 0;
  bf_map_sts_t map_sts;
  unsigned i, pipe;
  bool symmetric;

  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_txn_state_t *txn_state = NULL;
  pipe_mgr_adt_entry_info_t *entry = NULL;

  /* From the dev_id and the table handle, get the action table state */
  adt = pipe_mgr_adt_get(dev_id, tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table state for table"
        " with handle %d and device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (adt->ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
    return PIPE_SUCCESS;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (adt->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }
    adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe);
    if (!adt_tbl_data) continue;

    txn_state = adt_tbl_data->txn_state;

    /* As part of a txn commit loop through entry handles marked dirty and blow
     * away the entry location info backup.
     */

    map_sts = bf_map_get_first_rmv(
        &txn_state->dirtied_ent_hdl_htbl, &key, (void **)&entry);

    while (map_sts == BF_MAP_OK) {
      pipe_mgr_adt_entry_cleanup(entry);

      map_sts = bf_map_get_first_rmv(
          &txn_state->dirtied_ent_hdl_htbl, &key, (void **)&entry);
    }

    bf_map_destroy(&txn_state->dirtied_ent_hdl_htbl);

    for (stage_idx = 0; stage_idx < adt_tbl_data->num_stages; stage_idx++) {
      adt_stage_info = &(adt_tbl_data->adt_stage_info[stage_idx]);
      if (adt_stage_info->backup_entry_allocator) {
        pipe_mgr_adt_destroy_entry_idx_allocator_backup(adt, adt_stage_info);
      }
    }

    adt_tbl_data->backup_set = false;
    adt_tbl_data->backup_num_entries_occupied = 0;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t rebuild_mbr_id_map(pipe_mgr_adt_t *adt) {
  pipe_mgr_adt_entry_info_t *entry = NULL;
  pipe_mgr_adt_data_t *adt_data;
  unsigned long int key;
  bf_map_sts_t sts;

  if (adt->cache_id == false) {
    return PIPE_SUCCESS;
  }

  /* First destroy current map then rebuild by iterating all entries. */
  for (uint8_t i = 0; i < adt->num_tbls; i++) {
    adt_data = &adt->adt_tbl_data[i];
    bf_map_destroy(&adt_data->mbr_id_map);
    bf_map_init(&adt_data->mbr_id_map);

    sts = bf_map_get_first(&adt_data->entry_info_htbl, &key, (void **)&entry);
    while (sts == BF_MAP_OK) {
      sts = bf_map_add(&adt_data->mbr_id_map, entry->mbr_id, (void *)entry);
      if (sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in adding entry mbr_id mapping for tbl 0x%x, "
            "pipe id %d",
            __func__,
            __LINE__,
            adt->adt_tbl_hdl,
            entry->pipe_id);
        return PIPE_UNEXPECTED;
      }
      sts = bf_map_get_next(&adt_data->entry_info_htbl, &key, (void **)&entry);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_txn_abort(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t tbl_hdl,
                                     bf_dev_pipe_t *pipes_list,
                                     unsigned nb_pipes) {
  uint8_t stage_idx = 0;
  unsigned long key = 0;
  bf_map_sts_t map_sts;
  unsigned i, pipe;
  bool symmetric;

  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_txn_state_t *txn_state = NULL;
  pipe_mgr_adt_entry_info_t *entry = NULL;
  pipe_mgr_adt_entry_info_t *backup_entry = NULL;

  /* From the dev_id and the table handle, get the action table state */
  adt = pipe_mgr_adt_get(dev_id, tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table state for table"
        " with handle %d and device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (adt->ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
    return PIPE_SUCCESS;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (adt->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }
    adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe);
    if (!adt_tbl_data) continue;

    txn_state = adt_tbl_data->txn_state;

    map_sts = bf_map_get_first_rmv(
        &txn_state->dirtied_ent_hdl_htbl, &key, (void **)&backup_entry);
    while (map_sts == BF_MAP_OK) {
      // First remove existing entry for found key
      map_sts =
          bf_map_get_rmv(&adt_tbl_data->entry_info_htbl, key, (void **)&entry);
      if (map_sts == BF_MAP_OK) {
        pipe_mgr_adt_entry_cleanup(entry);
      }

      // backup_entry not NULL means that entry was modified, so it must be
      // re-added with backup data.
      if (backup_entry) {
        bf_map_add(&adt_tbl_data->entry_info_htbl, key, (void *)backup_entry);
        // If status NO_KEY then entry was deleted, so it must be reserved
        // again.
        if (map_sts == BF_MAP_NO_KEY) {
          bf_id_allocator_set(adt_tbl_data->ent_hdl_allocator,
                              PIPE_GET_HDL_VAL(key));
        }
        // backup_entry was set to NULL and entry exist then new entry was added
      } else if (map_sts == BF_MAP_OK) {
        bf_id_allocator_release(adt_tbl_data->ent_hdl_allocator,
                                PIPE_GET_HDL_VAL(key));
      }

      map_sts = bf_map_get_first_rmv(
          &txn_state->dirtied_ent_hdl_htbl, &key, (void **)&backup_entry);
    }

    bf_map_destroy(&txn_state->dirtied_ent_hdl_htbl);

    for (stage_idx = 0; stage_idx < adt_tbl_data->num_stages; stage_idx++) {
      adt_stage_info = &(adt_tbl_data->adt_stage_info[stage_idx]);
      if (adt_stage_info->backup_entry_allocator) {
        pipe_mgr_adt_restore_entry_idx_allocator_backup(adt, adt_stage_info);
      }
    }

    if (adt_tbl_data->backup_set) {
      adt_tbl_data->num_entries_occupied =
          adt_tbl_data->backup_num_entries_occupied;
      adt_tbl_data->backup_set = false;
      adt_tbl_data->backup_num_entries_occupied = 0;
    }
  }

  return rebuild_mbr_id_map(adt);
}
