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
 * @file pipe_mgr_sel_tbl.c
 * @date
 *
 * Implementation of Selection table management
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_act_tbl.h"

pipe_status_t pipe_mgr_sel_backup_fallback_entry(sel_tbl_t *sel_tbl) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  sel_tbl_info_t *bsel_tbl_info = NULL;

  if (!SEL_SESS_IS_TXN(sel_tbl)) {
    return PIPE_SUCCESS;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
      sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for %d",
              __func__,
              __LINE__,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl_t *bsel_tbl;
  bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, sel_tbl->pipe_id);
  if (!bsel_tbl) {
    LOG_ERROR("%s:%d get bsel table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  if (bsel_tbl->hlp.is_backup_valid) {
    return PIPE_SUCCESS;
  }

  bsel_tbl->hlp.fallback_adt_ent_hdl = sel_tbl->hlp.fallback_adt_ent_hdl;
  bsel_tbl->hlp.is_backup_valid = true;
  return PIPE_SUCCESS;
}

static void pipe_mgr_sel_restore_fallback_entry(sel_tbl_t *sel_tbl,
                                                sel_tbl_t *bsel_tbl) {
  if (bsel_tbl->hlp.is_backup_valid) {
    sel_tbl->hlp.fallback_adt_ent_hdl = bsel_tbl->hlp.fallback_adt_ent_hdl;
  }
}

static void pipe_mgr_sel_discard_fallback_entry(sel_tbl_t *bsel_tbl) {
  bsel_tbl->hlp.is_backup_valid = false;
}

static pipe_status_t pipe_mgr_sel_word_data_backup_deep_copy(
    sel_hlp_word_data_t *dest_word_data,
    sel_hlp_word_data_t *src_word_data,
    uint32_t ram_word_width) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_sel_grp_word_data_allocate(
      dest_word_data, src_word_data->word_width, ram_word_width);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  dest_word_data->adt_base_idx = src_word_data->adt_base_idx;
  dest_word_data->word_width = src_word_data->word_width;
  dest_word_data->usage = src_word_data->usage;

  PIPE_MGR_MEMCPY(dest_word_data->mbrs,
                  src_word_data->mbrs,
                  sizeof(pipe_sel_grp_mbr_hdl_t) * src_word_data->word_width);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_stage_backup_one(
    sel_tbl_info_t *sel_tbl_info,
    uint32_t pipe_idx,
    uint32_t stage_idx,
    sel_grp_stage_info_t *grp_stage_p) {
  sel_tbl_info_t *bsel_tbl_info = NULL;
  sel_tbl_t *bsel_tbl = NULL, *sel_tbl = NULL;
  sel_tbl_stage_info_t *bsel_stage_info = NULL, *sel_stage_info = NULL;
  sel_grp_stage_info_t *bsel_stage_grp = NULL;
  uint32_t no_words = 0;
  uint32_t sel_base_idx = 0;
  uint32_t i = 0;
  sel_hlp_word_data_t *bword_data = NULL, *word_data = NULL;

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
      sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for %d",
              __func__,
              __LINE__,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(pipe_idx < bsel_tbl_info->no_sel_tbls);

  bsel_tbl = &bsel_tbl_info->sel_tbl[pipe_idx];
  sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];

  PIPE_MGR_DBGCHK(stage_idx < bsel_tbl->num_stages);

  bsel_stage_info = &bsel_tbl->sel_tbl_stage_info[stage_idx];
  sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

  /* Backup the power2_allocator) */
  if (bsel_stage_info->stage_sel_grp_allocator == NULL) {
    bsel_stage_info->stage_sel_grp_allocator =
        power2_allocator_make_copy(sel_stage_info->stage_sel_grp_allocator);
  }

  if (bsel_stage_info->stage_sel_grp_allocator == NULL) {
    LOG_ERROR("%s:%d Error making a backup of the power2 allocator",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  bsel_stage_grp =
      (sel_grp_stage_info_t *)PIPE_MGR_MALLOC(sizeof(sel_grp_stage_info_t));
  if (bsel_stage_grp == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(bsel_stage_grp, grp_stage_p, sizeof(sel_grp_stage_info_t));
  bsel_stage_grp->next = NULL;
  bsel_stage_grp->prev = NULL;

  bsel_stage_grp->sel_grp_word_data =
      &bsel_stage_info->hlp.hlp_word[bsel_stage_grp->sel_base_idx];

  /* Do not backup the mbr_locator. Derive it */
  bsel_stage_grp->mbr_locator = NULL;

  no_words = bsel_stage_grp->no_words;
  sel_base_idx = bsel_stage_grp->sel_base_idx;

  for (i = 0; i < no_words; i++) {
    /* Backup the sel_word, it has the usage info too.
     * which is used in restore to update the pvl_word
     */
    bword_data = &bsel_stage_info->hlp.hlp_word[sel_base_idx + i];

    /* Higher level functions need to make sure that this function is
     * called only once for the given group
     */
    PIPE_MGR_DBGCHK(!bword_data->is_backup_valid);

    word_data = &sel_stage_info->hlp.hlp_word[sel_base_idx + i];

    pipe_mgr_sel_grp_word_data_deallocate(bword_data,
                                          sel_stage_info->ram_word_width);

    pipe_mgr_sel_word_data_backup_deep_copy(
        bword_data, word_data, sel_stage_info->ram_word_width);
    bword_data->is_backup_valid = true;
  }

  BF_LIST_DLL_AP(
      bsel_stage_info->sel_grp_stage_backup_list, bsel_stage_grp, next, prev);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_stage_restore_one(sel_tbl_t *sel_tbl,
                                                    sel_tbl_t *bsel_tbl,
                                                    uint32_t stage_idx) {
  sel_tbl_stage_info_t *bsel_stage_info = NULL, *sel_stage_info = NULL;
  sel_grp_stage_info_t *bsel_stage_grp = NULL;
  uint32_t no_words = 0;
  uint32_t sel_base_idx = 0;
  uint32_t i = 0;
  sel_hlp_word_data_t *bword_data = NULL, *word_data = NULL;

  PIPE_MGR_DBGCHK(stage_idx < bsel_tbl->num_stages);

  bsel_stage_info = &bsel_tbl->sel_tbl_stage_info[stage_idx];
  sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

  for (bsel_stage_grp = bsel_stage_info->sel_grp_stage_backup_list;
       bsel_stage_grp;
       bsel_stage_grp = bsel_stage_grp->next) {
    no_words = bsel_stage_grp->no_words;
    sel_base_idx = bsel_stage_grp->sel_base_idx;

    for (i = 0; i < no_words; i++) {
      bword_data = &bsel_stage_info->hlp.hlp_word[sel_base_idx + i];

      PIPE_MGR_DBGCHK(bword_data->is_backup_valid);
      word_data = &sel_stage_info->hlp.hlp_word[sel_base_idx + i];

      pipe_mgr_sel_grp_word_data_deallocate(word_data,
                                            sel_stage_info->ram_word_width);

      pipe_mgr_sel_word_data_backup_deep_copy(
          word_data, bword_data, sel_stage_info->ram_word_width);
    }
  }

  if (bsel_stage_info->stage_sel_grp_allocator) {
    power2_allocator_destroy(sel_stage_info->stage_sel_grp_allocator);
    sel_stage_info->stage_sel_grp_allocator =
        bsel_stage_info->stage_sel_grp_allocator;
    bsel_stage_info->stage_sel_grp_allocator = NULL;
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_sel_stage_restore_fixup
 *        Before calling this function, all the state has to be
 *        restored
 *        Fixup the following info in the different structures:
 *             sel_grp_pipe_lookup in sel_grp_info_t
 *             mbr_locator in sel_grp_stage_info_t
 *
 */
static pipe_status_t pipe_mgr_sel_stage_restore_fixup(sel_tbl_t *sel_tbl,
                                                      sel_tbl_t *bsel_tbl,
                                                      uint32_t stage_idx) {
  sel_tbl_stage_info_t *bsel_stage_info = NULL, *sel_stage_info = NULL;
  sel_grp_stage_info_t *bsel_stage_grp = NULL, *sel_stage_grp = NULL;
  uint32_t no_words = 0;
  uint32_t i = 0, j = 0;
  sel_hlp_word_data_t *word_data = NULL, *bword_data = NULL;
  sel_grp_info_t *sel_grp = NULL;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;

  PIPE_MGR_DBGCHK(stage_idx < bsel_tbl->num_stages);

  bsel_stage_info = &bsel_tbl->sel_tbl_stage_info[stage_idx];
  sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

  sel_grp_stage_info_t *bsel_stage_grp_next = NULL;
  for (bsel_stage_grp = bsel_stage_info->sel_grp_stage_backup_list;
       bsel_stage_grp;
       bsel_stage_grp = bsel_stage_grp_next) {
    bsel_stage_grp_next = bsel_stage_grp->next;

    sel_grp = pipe_mgr_sel_grp_get(sel_tbl, bsel_stage_grp->grp_hdl);
    PIPE_MGR_DBGCHK(sel_grp);

    /*  Update the hw-locator */
    sel_stage_grp =
        pipe_mgr_sel_grp_stage_info_get(sel_tbl, sel_grp, sel_stage_info);
    if (!sel_stage_grp) {
      /* This happens in case of Group delete backups.
       * In case of member delete/add/modify backups, the stage
       * group will still be valid
       */
      sel_stage_grp = bsel_stage_grp;
      pipe_mgr_sel_grp_update_grp_hw_locator_list(
          sel_tbl, sel_grp, sel_stage_info, sel_stage_grp);
      sel_stage_grp->stage_p = sel_stage_info;

      /* Remove the node from backup list and
       * add it to the inuse list in main structure
       */
      BF_LIST_DLL_REM(bsel_stage_info->sel_grp_stage_backup_list,
                      bsel_stage_grp,
                      next,
                      prev);
      no_words = bsel_stage_grp->no_words;

      for (i = 0; i < no_words; i++) {
        bword_data = &bsel_stage_grp->sel_grp_word_data[i];
        bword_data->is_backup_valid = false;
      }

      BF_LIST_DLL_AP(
          sel_stage_info->sel_grp_stage_inuse_list, bsel_stage_grp, next, prev);
    } else {
      sel_grp_stage_info_t *next_p = sel_stage_grp->next;
      sel_grp_stage_info_t *prev_p = sel_stage_grp->prev;
      PIPE_MGR_MEMCPY(
          sel_stage_grp, bsel_stage_grp, sizeof(sel_grp_stage_info_t));
      sel_stage_grp->next = next_p;
      sel_stage_grp->prev = prev_p;
      sel_stage_grp->stage_p = sel_stage_info;
    }
    PIPE_MGR_DBGCHK(sel_stage_grp->mbr_locator == NULL);

    no_words = sel_stage_grp->no_words;

    /* Now fixup the members */
    sel_stage_grp->sel_grp_word_data =
        &sel_stage_info->hlp.hlp_word[sel_stage_grp->sel_base_idx];
    if (sel_stage_grp->mbrs_duplicated) {
      no_words = 1;
    }

    for (i = 0; i < no_words; i++) {
      word_data = &sel_stage_grp->sel_grp_word_data[i];

      for (j = 0; j < word_data->usage; j++) {
        mbr_hdl = word_data->mbrs[j];
        pipe_mgr_sel_grp_mbr_hw_locator_update(sel_stage_grp, mbr_hdl, i, j);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_backup_one_refcount(
    sel_tbl_info_t *sel_tbl_info,
    sel_tbl_t *sel_tbl,
    pipe_sel_grp_hdl_t sel_grp_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *bsel_tbl_info = NULL;
  sel_grp_info_t *bsel_grp = NULL, *sel_grp = NULL;
  bool bsel_created = false;
  sel_tbl_t *bsel_tbl;

  if (!SEL_SESS_IS_TXN(sel_tbl)) {
    return PIPE_SUCCESS;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
      sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for 0x%x",
              __func__,
              __LINE__,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, sel_tbl->pipe_id);
  if (!bsel_tbl) {
    LOG_ERROR(
        "%s:%d Unable to find the backup sel table instance for 0x%x "
        "pipe 0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->tbl_hdl,
        sel_tbl->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_grp = pipe_mgr_sel_grp_get(bsel_tbl, sel_grp_hdl);
  if (bsel_grp && (bsel_grp->is_grp_backup || bsel_grp->is_refcount_backup)) {
    return PIPE_SUCCESS;
  }

  sel_grp = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (!sel_grp) {
    LOG_ERROR("%s:%d Sel group %d not found in table 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!bsel_grp) {
    bsel_grp = pipe_mgr_sel_grp_allocate();
    if (!bsel_grp) {
      LOG_ERROR("%s:%d alloc sel group failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    bsel_grp->grp_hdl = sel_grp->grp_hdl;
    bsel_created = true;
  }

  bsel_grp->num_references = sel_grp->num_references;
  bsel_grp->is_refcount_backup = true;

  if (bsel_created) {
    /* Add it to the backup table */
    rc = pipe_mgr_sel_grp_add_to_htbl(bsel_tbl, sel_grp_hdl, bsel_grp);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error adding the backup grp %d to grp_hdl_htbl",
                __func__,
                __LINE__,
                sel_grp_hdl);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

/*
 * Backup a group. Called when adding/deleting a new group
 * NOTE: This function can only handle the case of group delete/add.
 * If any new type of operation is needed, then the way sel_grp_mbrs and
 * backedup_mbrs are handled needs change
 * NOTE: After using this API the sel_grp_hdl's sel_grp_info becomes unsuable
 * and will be copied in bsel_grp_info
 */
pipe_status_t pipe_mgr_sel_grp_backup_one(sel_tbl_t *sel_tbl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl) {
  sel_tbl_info_t *bsel_tbl_info = NULL;
  sel_grp_info_t *bsel_grp = NULL, *sel_grp = NULL;
  bool bsel_created = false;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  Pvoid_t backedup_mbrs;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  uint32_t refcount_backup = 0;
  bool is_refcount_backup = false;
  sel_tbl_t *bsel_tbl;

  if (!SEL_SESS_IS_TXN(sel_tbl)) {
    return PIPE_SUCCESS;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
      sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for %d",
              __func__,
              __LINE__,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, sel_tbl->pipe_id);
  if (!bsel_tbl) {
    LOG_ERROR(
        "%s:%d Unable to find the backup sel table instance for 0x%x "
        "pipe 0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->tbl_hdl,
        sel_tbl->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_grp = pipe_mgr_sel_grp_get(bsel_tbl, sel_grp_hdl);
  if (bsel_grp) {
    if (bsel_grp->is_grp_backup) {
      return PIPE_SUCCESS;
    } else if (bsel_grp->is_refcount_backup) {
      is_refcount_backup = true;
      refcount_backup = bsel_grp->num_references;
    }
  }

  sel_grp = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);

  if (!bsel_grp) {
    bsel_grp = pipe_mgr_sel_grp_allocate();
    if (!bsel_grp) {
      LOG_ERROR("%s:%d alloc sel group failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    bsel_created = true;
  }

  if (sel_grp) {
    /* The backedup mbrs come from the existing group in the backup */
    backedup_mbrs = bsel_grp->backedup_mbrs;

    PIPE_MGR_MEMCPY(bsel_grp, sel_grp, sizeof(sel_grp_info_t));
    /* The group is being deleted */
    bsel_grp->is_backup_valid = true;

    bsel_grp->sel_grp_pipe_lookup = NULL;
    bsel_grp->backedup_mbrs = backedup_mbrs;
    /* Use saved refcount if previously backed up */
    if (is_refcount_backup) {
      bsel_grp->num_references = refcount_backup;
    }

    /* Set the sel_grp's sel_grp_mbrs to NULL. Since it is copied
     * to the bsel_grp
     */
    sel_grp->sel_grp_mbrs = NULL;

    /* If we've created a new backup group, we need to backup all the
     * stage level info too.
     * If the backup group already exists, then we would've backed up the
     * stage level info during member backup
     */
    if (bsel_created) {
      /* Need to backup all the stages */
      JUDYL_FOREACH(
          sel_grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
        JUDYL_FOREACH2(stage_lookup,
                       stage_idx,
                       sel_grp_stage_info_t *,
                       sel_grp_stage_info,
                       2) {
          rc = pipe_mgr_sel_grp_stage_backup_one(
              sel_tbl_info, pipe_idx, stage_idx, sel_grp_stage_info);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR("%s:%d Error rc 0x%x", __func__, __LINE__, rc);
            return rc;
          }
        }
      }
    }

  } else {
    bsel_grp->is_backup_valid = false;
    bsel_grp->grp_hdl = sel_grp_hdl;
  }

  bsel_grp->is_grp_backup = true;

  if (bsel_created) {
    /* Add it to the backup table */
    rc = pipe_mgr_sel_grp_add_to_htbl(bsel_tbl, sel_grp_hdl, bsel_grp);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error adding the backup grp %d to grp_hdl_htbl",
                __func__,
                __LINE__,
                sel_grp_hdl);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_mbr_fixup_backup(sel_grp_info_t *bsel_grp,
                                                   sel_grp_info_t *sel_grp) {
  sel_grp_mbr_t *bgrp_mbr = NULL, *grp_mbr = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;

  JUDYL_FOREACH(bsel_grp->backedup_mbrs, mbr_hdl, sel_grp_mbr_t *, bgrp_mbr) {
    grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp, mbr_hdl, false);
    if (bgrp_mbr->is_backup_valid == false) {
      if (grp_mbr) {
        rc = pipe_mgr_sel_grp_mbr_remove_and_destroy(
            sel_grp, bgrp_mbr->mbr_hdl, false);
        sel_grp->mbr_count--;
      }
    } else {
      if (!grp_mbr) {
        /* Need to add a new item to the mbr htbl */
        grp_mbr = (sel_grp_mbr_t *)PIPE_MGR_MALLOC(sizeof(sel_grp_mbr_t));
        if (!grp_mbr) {
          LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
        sel_grp->mbr_count++;
      }
      PIPE_MGR_MEMCPY(grp_mbr, bgrp_mbr, sizeof(sel_grp_mbr_t));
      rc = pipe_mgr_sel_mbr_add_to_htbl(
          sel_grp, grp_mbr->mbr_hdl, grp_mbr, false);
    }

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Errorc rc 0x%x", __func__, __LINE__, rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

static sel_grp_info_t *pipe_mgr_sel_grp_restore_from_backup(
    sel_grp_info_t *bsel_grp) {
  sel_grp_info_t *sel_grp = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_grp = pipe_mgr_sel_grp_allocate();
  if (!sel_grp) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMCPY(sel_grp, bsel_grp, sizeof(sel_grp_info_t));

  sel_grp->sel_grp_pipe_lookup = NULL;
  sel_grp->backedup_mbrs = NULL;
  sel_grp->is_grp_backup = false;
  sel_grp->is_backup_valid = false;
  sel_grp->is_refcount_backup = false;
  /* Just set the htbl to NULL, since it's being referenced by sel_grp now */
  bsel_grp->sel_grp_mbrs = NULL;

  /* Now remove items from the backup_grp_mbr_list and
   * add/remove those from the htbl
   */
  rc = pipe_mgr_sel_mbr_fixup_backup(bsel_grp, sel_grp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error rc 0x%x", __func__, __LINE__, rc);
    return NULL;
  }

  return sel_grp;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_restore_all(
    sel_grp_info_t *sel_grp, sel_grp_info_t *bsel_grp) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (!sel_grp || !bsel_grp) {
    LOG_ERROR("%s:%d Invalid args", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_sel_mbr_fixup_backup(bsel_grp, sel_grp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error rc 0x%x", __func__, __LINE__, rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_restore_one(sel_tbl_t *sel_tbl,
                                                  sel_grp_info_t *bsel_grp) {
  pipe_sel_grp_hdl_t sel_grp_hdl = bsel_grp->grp_hdl;
  sel_grp_info_t *sel_grp = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_grp = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);

  if (sel_grp && !bsel_grp->is_grp_backup) {
    if (bsel_grp->is_refcount_backup) {
      sel_grp->num_references = bsel_grp->num_references;
    }
    return pipe_mgr_sel_grp_mbr_restore_all(sel_grp, bsel_grp);
  }

  if (bsel_grp->is_backup_valid) {
    /* It's a deleted group or a resized group in the transaction. */
    if (sel_grp) {
      // If the backup is created by sel resize do not delete mat_tbl_list
      // because restore from backup sel grp will use the same back pointers to
      // mat tables.
      if (bsel_grp->backup_from_resize) sel_grp->mat_tbl_list = NULL;
      rc = pipe_mgr_sel_grp_del_internal(
          sel_tbl, sel_grp_hdl, true, false, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error removing group %d from selector table htbl rc 0x%x",
            __func__,
            __LINE__,
            sel_grp_hdl,
            rc);
        return rc;
      }
    }

    sel_grp = pipe_mgr_sel_grp_restore_from_backup(bsel_grp);
    /* Readd the group back into the sel_tbl_info's htbl */
    rc = pipe_mgr_sel_grp_add_to_htbl(sel_tbl, sel_grp_hdl, sel_grp);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error adding the grp %d to grp_hdl_htbl rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_hdl,
                rc);
      return rc;
    }

  } else {
    if (sel_grp) {
      /* It's a newly added group in the transaction. Remove the group */
      rc = pipe_mgr_sel_grp_del_internal(
          sel_tbl, sel_grp_hdl, true, false, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error removing group %d from selector table htbl rc 0x%x",
            __func__,
            __LINE__,
            sel_grp_hdl,
            rc);
        return rc;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_backup_one(
    sel_tbl_info_t *sel_tbl_info,
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp,
    pipe_sel_grp_mbr_hdl_t grp_mbr_hdl) {
  sel_tbl_info_t *bsel_tbl_info = NULL;
  sel_grp_info_t *bsel_grp = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_mbr_t *sel_mbr = NULL, *bsel_mbr = NULL;
  bool new_grp = false;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  Pvoid_t stage_lookup;
  sel_tbl_t *bsel_tbl;
  uint32_t pipe_idx;
  uint32_t stage_idx;

  if (!SEL_SESS_IS_TXN(sel_tbl)) {
    return PIPE_SUCCESS;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
      sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for %d",
              __func__,
              __LINE__,
              sel_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, sel_tbl->pipe_id);
  if (!bsel_tbl) {
    LOG_ERROR(
        "%s:%d Unable to find the backup sel table instance for 0x%x "
        "pipe 0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->tbl_hdl,
        sel_tbl->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_grp = pipe_mgr_sel_grp_get(bsel_tbl, sel_grp->grp_hdl);

  if (!bsel_grp) {
    bsel_grp = pipe_mgr_sel_grp_allocate();
    if (!bsel_grp) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    bsel_grp->grp_hdl = sel_grp->grp_hdl;

    new_grp = true;
  }

  if (pipe_mgr_sel_grp_mbr_get(bsel_grp, grp_mbr_hdl, true)) {
    /* Already backed up */
    return PIPE_SUCCESS;
  }

  bsel_mbr = pipe_mgr_sel_grp_mbr_alloc(grp_mbr_hdl);
  if (bsel_mbr == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  sel_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp, grp_mbr_hdl, false);

  if (!sel_mbr) {
    bsel_mbr->is_backup_valid = false;
    bsel_mbr->mbr_hdl = grp_mbr_hdl;
  } else {
    PIPE_MGR_MEMCPY(bsel_mbr, sel_mbr, sizeof(sel_grp_mbr_t));
    bsel_mbr->is_backup_valid = true;
  }

  rc = pipe_mgr_sel_mbr_add_to_htbl(bsel_grp, grp_mbr_hdl, bsel_mbr, true);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error", __func__, __LINE__);
    return rc;
  }

  if (new_grp) {
    /* Need to add this group to the backup table */
    rc = pipe_mgr_sel_grp_add_to_htbl(bsel_tbl, sel_grp->grp_hdl, bsel_grp);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error adding the backup grp %d to grp_hdl_htbl",
                __func__,
                __LINE__,
                sel_grp->grp_hdl);
      return rc;
    }

    /* Need to backup all the stages */
    JUDYL_FOREACH(
        sel_grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
      JUDYL_FOREACH2(stage_lookup,
                     stage_idx,
                     sel_grp_stage_info_t *,
                     sel_grp_stage_info,
                     2) {
        rc = pipe_mgr_sel_grp_stage_backup_one(
            sel_tbl_info, pipe_idx, stage_idx, sel_grp_stage_info);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR("%s:%d Error rc 0x%x", __func__, __LINE__, rc);
          return rc;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_stage_info_discard_one(sel_tbl_t *bsel_tbl,
                                                         sel_tbl_t *sel_tbl,
                                                         uint32_t stage_idx) {
  sel_tbl_stage_info_t *bsel_stage_info = NULL, *sel_stage_info;
  sel_grp_stage_info_t *bsel_stage_grp = NULL;
  sel_hlp_word_data_t *bword_data = NULL;
  uint32_t no_words = 0, i = 0;
  uint32_t sel_base_idx = 0;

  PIPE_MGR_DBGCHK(stage_idx < bsel_tbl->num_stages);

  bsel_stage_info = &bsel_tbl->sel_tbl_stage_info[stage_idx];
  sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

  for (bsel_stage_grp = bsel_stage_info->sel_grp_stage_backup_list;
       bsel_stage_grp;
       bsel_stage_grp = bsel_stage_grp->next) {
    no_words = bsel_stage_grp->no_words;
    sel_base_idx = bsel_stage_grp->sel_base_idx;

    for (i = 0; i < no_words; i++) {
      bword_data = &bsel_stage_info->hlp.hlp_word[sel_base_idx + i];

      PIPE_MGR_DBGCHK(bword_data->is_backup_valid);

      pipe_mgr_sel_grp_word_data_deallocate(bword_data,
                                            sel_stage_info->ram_word_width);
      bword_data->is_backup_valid = false;
    }
  }

  pipe_mgr_sel_grp_stage_list_free_all(
      bsel_stage_info->sel_grp_stage_backup_list);
  if (bsel_stage_info->stage_sel_grp_allocator) {
    power2_allocator_destroy(bsel_stage_info->stage_sel_grp_allocator);
    bsel_stage_info->stage_sel_grp_allocator = NULL;
  }

  bsel_stage_info->sel_grp_stage_backup_list = NULL;

  return PIPE_SUCCESS;
}

static void pipe_mgr_sel_grp_discard_all(sel_tbl_t *bsel_tbl) {
  Word_t grp_hdl = 0;
  PWord_t Pvalue = NULL;
  Word_t Rc_word;
  /* Free all the groups */
  JLF(Pvalue, bsel_tbl->sel_grp_array, grp_hdl);
  while (Pvalue) {
    pipe_mgr_sel_grp_destroy((sel_grp_info_t *)*Pvalue);
    JLN(Pvalue, bsel_tbl->sel_grp_array, grp_hdl);
  }
  JLFA(Rc_word, bsel_tbl->sel_grp_array);
  (void)Rc_word;
}

pipe_status_t pipe_mgr_sel_discard_all(sel_tbl_t *sel_tbl,
                                       sel_tbl_t *bsel_tbl) {
  uint32_t j;

  pipe_mgr_sel_grp_discard_all(bsel_tbl);

  /* Discard the backed up grp_htbl */
  pipe_mgr_sel_discard_fallback_entry(bsel_tbl);
  for (j = 0; j < sel_tbl->num_stages; j++)
    pipe_mgr_sel_stage_info_discard_one(bsel_tbl, sel_tbl, j);

  return PIPE_SUCCESS;
}

static pipe_status_t rebuild_grp_id_map(sel_tbl_info_t *sel_tbl_info) {
  if (sel_tbl_info->cache_id == false) {
    return PIPE_SUCCESS;
  }
  /* Destroy existing map and create new one. Maps shadows htbl and all
   * allocs/destroy should be handled where htbl is. */
  sel_grp_info_t *sel_grp_info = NULL;
  pipe_sel_grp_hdl_t grp_hdl;
  sel_tbl_t *sel_tbl;
  for (uint8_t i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_info->sel_tbl[i];
    bf_map_destroy(&sel_tbl->grp_id_map);
    bf_map_init(&sel_tbl->grp_id_map);
    JUDYL_FOREACH(
        sel_tbl->sel_grp_array, grp_hdl, sel_grp_info_t *, sel_grp_info) {
      bf_map_sts_t sts =
          bf_map_add(&sel_tbl->grp_id_map, sel_grp_info->grp_id, sel_grp_info);
      if (BF_MAP_OK != sts) {
        LOG_ERROR("%s:%d Error adding grp %d (hdl 0x%x) to grp_id map rc %d",
                  __func__,
                  __LINE__,
                  sel_grp_info->grp_id,
                  grp_hdl,
                  sts);
        return PIPE_UNEXPECTED;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_restore_all(sel_tbl_t *sel_tbl,
                                       sel_tbl_t *bsel_tbl) {
  sel_grp_info_t *bsel_grp = NULL;
  pipe_sel_grp_hdl_t grp_hdl;
  (void)grp_hdl;
  uint32_t j;

  /* restore all the groups and members from the hash table*/
  /* This is done before restoring stages. Restoring
   * tables will also internally delete the newly created groups from
   * the stages
   */
  JUDYL_FOREACH(bsel_tbl->sel_grp_array, grp_hdl, sel_grp_info_t *, bsel_grp) {
    pipe_mgr_sel_grp_restore_one(sel_tbl, bsel_grp);
  }

  /* Restore all the stage level info */
  pipe_mgr_sel_restore_fallback_entry(sel_tbl, bsel_tbl);
  for (j = 0; j < sel_tbl->num_stages; j++)
    pipe_mgr_sel_stage_restore_one(sel_tbl, bsel_tbl, j);

  /* Go through each of the stage level info and then fixup the hw_locator
   * pointers
   */
  for (j = 0; j < sel_tbl->num_stages; j++)
    pipe_mgr_sel_stage_restore_fixup(sel_tbl, bsel_tbl, j);

  /* Free all the memory allocated for backup tables */
  pipe_mgr_sel_discard_all(sel_tbl, bsel_tbl);

  return rebuild_grp_id_map(sel_tbl->sel_tbl_info);
}
