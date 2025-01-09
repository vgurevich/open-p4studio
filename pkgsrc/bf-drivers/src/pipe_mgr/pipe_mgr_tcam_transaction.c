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
 * @file pipe_mgr_tcam.c
 * @date
 *
 * Implementation of transaction related routines for TCAMs - backup/restore etc
 * For each of the runtime data-structures, 3 routines are needed.
 * Backup, Restore and Discard.
 * Backup needs to be called before doing any modification to the current state
 * Restore is called during session abort
 * Discard is called during session commit to discard the backed up state
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_transaction.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tind.h"

pipe_status_t pipe_mgr_tcam_default_entry_backup(
    tcam_pipe_tbl_t *tcam_pipe_tbl) {
  tcam_hlp_entry_t *tcam_entry = NULL;
  tcam_tbl_info_t *btcam_tbl_info = NULL;
  tcam_pipe_tbl_t *btcam_pipe_tbl = NULL;
  tcam_hlp_entry_t *btcam_entry = NULL;
  tcam_tbl_t *tcam_tbl = NULL;

  tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, 0);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_TABLE_NOT_FOUND;
  }

  if (!TCAM_SESS_IS_TXN(tcam_pipe_tbl)) {
    return PIPE_SUCCESS;
  }

  btcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(get_tcam_tbl_info(tcam_tbl)->dev_id,
                                 get_tcam_tbl_info(tcam_tbl)->tbl_hdl,
                                 true);
  if (btcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup tcam table for %d",
              __func__,
              __LINE__,
              get_tcam_tbl_info(tcam_tbl)->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  btcam_pipe_tbl = &btcam_tbl_info->tcam_pipe_tbl[tcam_pipe_tbl->pipe_idx];

  if (btcam_pipe_tbl->default_backup_valid) {
    return PIPE_SUCCESS;
  }

  btcam_pipe_tbl->hlp.default_ent_set = tcam_pipe_tbl->hlp.default_ent_set;
  btcam_pipe_tbl->default_ent_type = tcam_pipe_tbl->default_ent_type;
  btcam_pipe_tbl->hlp.default_ent_hdl = tcam_pipe_tbl->hlp.default_ent_hdl;

  tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
  btcam_entry = pipe_mgr_tcam_entry_alloc();
  if (!btcam_entry) {
    LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_mgr_tcam_entry_deep_copy(btcam_entry, tcam_entry);
  btcam_pipe_tbl->hlp.hlp_default_tcam_entry = btcam_entry;

  btcam_pipe_tbl->default_backup_valid = true;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_default_entry_restore(
    tcam_pipe_tbl_t *tcam_pipe_tbl, tcam_pipe_tbl_t *btcam_pipe_tbl) {
  tcam_tbl_t *tcam_tbl = NULL;
  tcam_hlp_entry_t *tcam_entry = NULL;
  uint32_t dir_dflt_index = 0;

  if (btcam_pipe_tbl->default_backup_valid == false) {
    return PIPE_SUCCESS;
  }

  PIPE_MGR_DBGCHK(tcam_pipe_tbl->hlp.hlp_default_tcam_entry != NULL);
  tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[0];
  dir_dflt_index = tcam_tbl->total_entries - 1;

  /* Release all the state about the current entry */
  pipe_mgr_tcam_hlp_entry_destroy(tcam_pipe_tbl->hlp.hlp_default_tcam_entry,
                                  false);
  tcam_pipe_tbl->hlp.hlp_default_tcam_entry = NULL;
  if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
    tcam_tbl->hlp.tcam_entries[dir_dflt_index] = NULL;
  }

  /* Restore the state from the backed up entry */
  tcam_pipe_tbl->hlp.default_ent_set = btcam_pipe_tbl->hlp.default_ent_set;
  tcam_pipe_tbl->default_ent_type = btcam_pipe_tbl->default_ent_type;
  tcam_pipe_tbl->hlp.default_ent_hdl = btcam_pipe_tbl->hlp.default_ent_hdl;

  tcam_entry = btcam_pipe_tbl->hlp.hlp_default_tcam_entry;
  btcam_pipe_tbl->hlp.hlp_default_tcam_entry = NULL;
  tcam_entry->range_list_p = &(tcam_entry->range_list);
  BF_LIST_DLL_AP(
      *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);

  tcam_pipe_tbl->hlp.hlp_default_tcam_entry = tcam_entry;

  if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
    tcam_tbl->hlp.tcam_entries[dir_dflt_index] = tcam_entry;
  }

  btcam_pipe_tbl->default_backup_valid = false;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_default_entry_backup_discard(
    tcam_pipe_tbl_t *btcam_pipe_tbl) {
  if ((btcam_pipe_tbl->hlp.default_ent_set == true) &&
      (btcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_INDIRECT)) {
    pipe_mgr_tcam_hlp_entry_destroy(btcam_pipe_tbl->hlp.hlp_default_tcam_entry,
                                    false);
    btcam_pipe_tbl->hlp.hlp_default_tcam_entry = NULL;
  }

  btcam_pipe_tbl->default_backup_valid = false;
  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_index_backup_one
 *        Backup a tcam entry in the backup tables
 *
 * \param tcam_tbl Pointer to the tcam table structure
 * \param index TCAM index to backup
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_entry_index_backup_one(tcam_tbl_t *tcam_tbl,
                                                   uint32_t index) {
  tcam_hlp_entry_t *tcam_entry = NULL;
  tcam_tbl_info_t *btcam_tbl_info = NULL;
  tcam_tbl_t *btcam_tbl = NULL;
  tcam_pipe_tbl_t *btcam_pipe_tbl = NULL;
  tcam_hlp_entry_t *btcam_entry = NULL;
  /* The backup has to backup all the below:
   * The tcam_entries array
   * The tcam_prio_tree
   * Global tcam_entries_htbl hash table
   */

  if (!TCAM_SESS_IS_TXN(get_tcam_pipe_tbl(tcam_tbl))) {
    return PIPE_SUCCESS;
  }

  btcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(get_tcam_tbl_info(tcam_tbl)->dev_id,
                                 get_tcam_tbl_info(tcam_tbl)->tbl_hdl,
                                 true);
  if (btcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup tcam table for %d",
              __func__,
              __LINE__,
              get_tcam_tbl_info(tcam_tbl)->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  btcam_pipe_tbl =
      &btcam_tbl_info->tcam_pipe_tbl[get_tcam_pipe_tbl(tcam_tbl)->pipe_idx];

  btcam_tbl = get_tcam_tbl(btcam_pipe_tbl, tcam_tbl->ptn_index);
  if (btcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_TABLE_NOT_FOUND;
  }
  if (btcam_tbl->hlp.tcam_entries[index]) {
    /* Shadowing has already been completed */
    return PIPE_SUCCESS;
  }

  tcam_entry = tcam_tbl->hlp.tcam_entries[index];

  if (tcam_entry) {
    btcam_entry = pipe_mgr_tcam_entry_alloc();
    if (btcam_entry == NULL) {
      LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    pipe_mgr_tcam_entry_deep_copy(btcam_entry, tcam_entry);
    btcam_entry->is_backup_valid = true;
  } else {
    btcam_entry = pipe_mgr_tcam_entry_alloc();
    if (btcam_entry == NULL) {
      LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMSET(btcam_entry, 0, sizeof(tcam_hlp_entry_t));
    btcam_entry->is_backup_valid = false;
  }

  btcam_tbl->hlp.tcam_entries[index] = btcam_entry;

  return PIPE_SUCCESS;
}

/* Backup all the instances of a ent-hdl based TCAM entry */
pipe_status_t pipe_mgr_tcam_entry_backup_one(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                             pipe_mat_ent_hdl_t ent_hdl) {
  tcam_hlp_entry_t *tcam_entry = NULL, *head_entry = NULL;

  if (!TCAM_SESS_IS_TXN(tcam_pipe_tbl)) {
    return PIPE_SUCCESS;
  }

  head_entry = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, ent_hdl, 0);
  if (head_entry == NULL) {
    return PIPE_SUCCESS;
  }

  if (head_entry->is_default) {
    pipe_mgr_tcam_default_entry_backup(tcam_pipe_tbl);
    return PIPE_SUCCESS;
  }

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_TABLE_NOT_FOUND;
  }

  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    if (tcam_entry->index != PIPE_MGR_TCAM_INVALID_IDX) {
      pipe_mgr_tcam_entry_index_backup_one(tcam_tbl, tcam_entry->index);
    }
  }
  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_END()

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_restore_all
 *        Restore the tcam entries from backup
 *
 * Function to restore the tcam_entries and revert the state. This function
 * also helps to restore some derived data structures which are not
 * backed up directly - particularly the tcam_entry_list and the
 * tcam_entries_htbl
 *
 * \param tcam_tbl Pointer to the tcam table
 * \param btcam_tbl Pointer to the backup tcam table
 * \return pipe_status_t Status of the operation
 */
static pipe_status_t pipe_mgr_tcam_entry_restore_all(tcam_tbl_t *tcam_tbl,
                                                     tcam_tbl_t *btcam_tbl) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;
  tcam_hlp_entry_t *btcam_entry = NULL, *tcam_entry = NULL;

  /* Step 1 - Remove all the tcam metadata that are in the
   * locations populated in backup table
   *
   * Step 2 - Copy from the backup table to the main table
   *
   * Step 3 - For all the entries that are valid in the backup table,
   * update the tcam metadata
   *
   */

  for (i = 0; i < btcam_tbl->total_entries; i++) {
    if (btcam_tbl->hlp.tcam_entries[i] == NULL) {
      continue;
    }

    tcam_entry = tcam_tbl->hlp.tcam_entries[i];
    if (tcam_entry) {
      if (!tcam_entry->is_default) {
        pipe_mgr_tcam_update_prio_array(
            tcam_tbl, tcam_entry->group, tcam_entry->priority, i, false);
        if ((tcam_entry->range_list_p == NULL) ||
            ((*(tcam_entry->range_list_p)) == NULL)) {
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }
        if (!TCAM_HLP_IS_RANGE_HEAD(tcam_entry)) {
          BF_LIST_DLL_REM(
              *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
        } else {
          tcam_hlp_entry_t *entry_in_htbl = pipe_mgr_tcam_entry_get(
              get_tcam_pipe_tbl(tcam_tbl), tcam_entry->entry_hdl, 0);
          if (entry_in_htbl == NULL) {
            PIPE_MGR_DBGCHK(0);
            return PIPE_UNEXPECTED;
          }

          BF_LIST_DLL_REM(
              *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
          tcam_hlp_entry_t *new_head = *(tcam_entry->range_list_p);
          tcam_hlp_entry_t *tentry = NULL;

          if (new_head) {
            new_head->range_list = new_head;
            new_head->range_list_p = &new_head->range_list;
          }

          for (tentry = new_head; tentry; tentry = tentry->next_range) {
            tentry->range_list_p = new_head->range_list_p;
          }

          if (entry_in_htbl == tcam_entry) {
            pipe_mgr_tcam_entry_htbl_add_delete(
                tcam_tbl, tcam_entry->entry_hdl, NULL, false);
            pipe_mgr_tcam_entry_hdl_release(get_tcam_pipe_tbl(tcam_tbl),
                                            tcam_entry->entry_hdl);
          }

          BF_LIST_DLL_REM(entry_in_htbl, tcam_entry, next, prev);
          if (new_head) {
            BF_LIST_DLL_AP(entry_in_htbl, new_head, next, prev);
          }

          if (entry_in_htbl) {
            pipe_mgr_tcam_entry_htbl_add_delete(
                tcam_tbl, entry_in_htbl->entry_hdl, entry_in_htbl, true);
            pipe_mgr_tcam_entry_hdl_set(get_tcam_pipe_tbl(tcam_tbl),
                                        entry_in_htbl->entry_hdl);
          }
        }
      }

      PIPE_MGR_DBGCHK(tcam_tbl->hlp.total_usage);
      tcam_tbl->hlp.total_usage--;
    }
  }

  for (i = 0; i < btcam_tbl->total_entries; i++) {
    if (btcam_tbl->hlp.tcam_entries[i] == NULL) {
      continue;
    }

    btcam_entry = btcam_tbl->hlp.tcam_entries[i];

    if (btcam_entry->is_backup_valid) {
      if (tcam_tbl->hlp.tcam_entries[i]) {
        /* TCAM entry got modified */
        /* Existing entry needs to be freed and removed from
         * the tcam_entries_htbl
         */
        tcam_entry = tcam_tbl->hlp.tcam_entries[i];

        pipe_mgr_tcam_hlp_entry_destroy(tcam_entry, false);
      } else {
        rc = pipe_mgr_tcam_mark_index_inuse(tcam_tbl, i);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      tcam_entry = pipe_mgr_tcam_entry_alloc();
      if (tcam_entry == NULL) {
        LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      pipe_mgr_tcam_entry_deep_copy(tcam_entry, btcam_entry);

      tcam_tbl->hlp.tcam_entries[i] = tcam_entry;

    } else {
      /* Need to remove the existing entry */

      if (tcam_tbl->hlp.tcam_entries[i]) {
        tcam_entry = tcam_tbl->hlp.tcam_entries[i];

        pipe_mgr_tcam_hlp_entry_destroy(tcam_entry, false);
        tcam_tbl->hlp.tcam_entries[i] = NULL;

        rc = pipe_mgr_tcam_mark_index_free(tcam_tbl, i);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
    }
  }

  tcam_hlp_entry_t *prev_head = NULL;
  uint32_t range_count = 0;
  for (i = 0; i < btcam_tbl->total_entries; i++) {
    if (btcam_tbl->hlp.tcam_entries[i] == NULL) {
      continue;
    }

    btcam_entry = btcam_tbl->hlp.tcam_entries[i];
    tcam_entry = tcam_tbl->hlp.tcam_entries[i];

    if ((range_count != 0) && (btcam_entry->is_backup_valid == false)) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    if (btcam_entry->is_backup_valid) {
      if (!btcam_entry->is_default) {
        pipe_mgr_tcam_update_prio_array(
            tcam_tbl, tcam_entry->group, tcam_entry->priority, i, true);

        tcam_hlp_entry_t *entry_in_htbl = pipe_mgr_tcam_entry_get(
            get_tcam_pipe_tbl(tcam_tbl), tcam_entry->entry_hdl, 0);

        if (!entry_in_htbl) {
          /* Fixup the tcam entry into the various metadata */
          pipe_mgr_tcam_entry_htbl_add_delete(
              tcam_tbl, tcam_entry->entry_hdl, tcam_entry, true);
          pipe_mgr_tcam_entry_hdl_set(get_tcam_pipe_tbl(tcam_tbl),
                                      tcam_entry->entry_hdl);
        }

        if (range_count == 0) {
          range_count = tcam_entry->range_count;
          /* This will be the head entry */
          BF_LIST_DLL_AP(entry_in_htbl, tcam_entry, next, prev);
          prev_head = tcam_entry;
          tcam_entry->range_list_p = &tcam_entry->range_list;
        }

        BF_LIST_DLL_AP(
            *(prev_head->range_list_p), tcam_entry, next_range, prev_range);
        range_count--;
        if (range_count == 0) {
          prev_head = NULL;
        }
      }
      tcam_tbl->hlp.total_usage++;
    }

    pipe_mgr_tcam_hlp_entry_destroy(btcam_entry, false);
    btcam_tbl->hlp.tcam_entries[i] = NULL;
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_discard_all
 *        Discard the tcam entry backup
 *
 * \param btcam_tbl Pointer to the backup tcam table
 * \return void
 */
static void pipe_mgr_tcam_entry_discard_all(tcam_pipe_tbl_t *btcam_pipe_tbl) {
  uint32_t i = 0;
  tcam_hlp_entry_t *btcam_entry = NULL;
  tcam_tbl_t *btcam_tbl = NULL;

  FOR_ALL_PTNS_BEGIN(btcam_pipe_tbl, btcam_tbl) {
    for (i = 0; i < btcam_tbl->total_entries; i++) {
      if (btcam_tbl->hlp.tcam_entries[i] == NULL) {
        continue;
      }
      btcam_entry = btcam_tbl->hlp.tcam_entries[i];
      pipe_mgr_tcam_hlp_entry_destroy(btcam_entry, false);
      btcam_tbl->hlp.tcam_entries[i] = NULL;
    }
  }
  FOR_ALL_PTNS_END();
}

/** \brief pipe_mgr_tcam_restore_all
 *        Restore the state from the backup copies
 *
 * This function should be called during abort to restore the state from
 * backed up state
 *
 * \param tcam_pipe_tbl Pointer to the tcam table pipe info
 * \param btcam_pipe_tbl Pointer to the backup tcam table pipe info
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_restore_all(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                        tcam_pipe_tbl_t *btcam_pipe_tbl) {
  tcam_hlp_entry_t *tcam_entry = NULL;
  tcam_tbl_t *tcam_tbl, *btcam_tbl;

  FOR_ALL_PTNS_BEGIN(tcam_pipe_tbl, tcam_tbl) {
    while (tcam_tbl->hlp.entry_add_list) {
      tcam_entry = tcam_tbl->hlp.entry_add_list;
      BF_LIST_DLL_REM(
          tcam_tbl->hlp.entry_add_list, tcam_entry, next_atomic, prev_atomic);
    }

    while (tcam_tbl->hlp.entry_del_list) {
      tcam_entry = tcam_tbl->hlp.entry_del_list;
      BF_LIST_DLL_REM(
          tcam_tbl->hlp.entry_del_list, tcam_entry, next_atomic, prev_atomic);
    }

    btcam_tbl = get_tcam_tbl(btcam_pipe_tbl, tcam_tbl->ptn_index);
    if (btcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_TABLE_NOT_FOUND;
    }
    pipe_mgr_tcam_entry_restore_all(tcam_tbl, btcam_tbl);
  }
  FOR_ALL_PTNS_END();
  pipe_mgr_tcam_default_entry_restore(tcam_pipe_tbl, btcam_pipe_tbl);

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_discard_all
 *        Restore the state from the backup copies
 *
 * This function should be called during commit to discard the state from
 * backed up state
 *
 * \param btcam_pipe_tbl Pointer to the backup tcam table pipe instance
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_discard_all(tcam_pipe_tbl_t *btcam_pipe_tbl) {
  pipe_mgr_tcam_entry_discard_all(btcam_pipe_tbl);
  pipe_mgr_tcam_default_entry_backup_discard(btcam_pipe_tbl);
  return PIPE_SUCCESS;
}
