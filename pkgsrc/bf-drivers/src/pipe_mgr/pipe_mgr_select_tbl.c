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
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <tofino_regs/tofino.h>

#include <target-utils/bit_utils/bit_utils.h>
#include <bfutils/dynamic_hash/dynamic_hash.h>
#include <target-utils/power2_allocator/power2_allocator.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_select_tbl_transaction.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_tbl.h"

#define PIPE_MGR_SEL_TBL_ASSERT(x, y)

typedef pipe_status_t (*pipe_mgr_sel_grp_mbr_iter_func)(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    void *cookie,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

static pipe_status_t wr_one_mbr(sel_tbl_t *sel_tbl,
                                sel_grp_stage_info_t *sel_grp_stage_info,
                                uint32_t word_offset_in_grp,
                                uint32_t mbr_offset_in_word,
                                bool new_mbr_val,
                                struct pipe_mgr_sel_move_list_t **move_tail_p);
static pipe_status_t add_one_mbr(sel_tbl_t *sel_tbl,
                                 sel_grp_info_t *sel_grp_info,
                                 sel_grp_stage_info_t *sel_grp_stage_info,
                                 pipe_sel_grp_mbr_hdl_t mbr_hdl,
                                 uint32_t word_offset_in_grp,
                                 uint32_t mbr_offset_in_word,
                                 struct pipe_mgr_sel_move_list_t **move_tail_p);
static pipe_status_t rmv_one_mbr(sel_tbl_t *sel_tbl,
                                 sel_grp_stage_info_t *sel_grp_stage_info,
                                 sel_grp_info_t *sel_grp_info,
                                 uint32_t word_offset_in_grp,
                                 uint32_t mbr_offset_in_word,
                                 uint32_t src_word_offset_in_grp,
                                 bool update_hw_locator,
                                 struct pipe_mgr_sel_move_list_t **move_tail_p);
static pipe_status_t issue_callback(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t mbr_word_idx,
    uint32_t mbr_idx,
    enum pipe_sel_update_type sel_op_type,
    bool disabled_mbr,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

static void pipe_mgr_sel_tbl_info_destroy(sel_tbl_info_t *sel_tbl_info,
                                          bool is_backup);

static void pipe_mgr_sel_grp_mbr_destroy(sel_grp_mbr_t *grp_mbr);

pipe_status_t pipe_mgr_selector_tbl_get_sequence_order(bool *enable) {
  pipe_sel_tbl_hdl_t tbl_hdl;
  sel_tbl_info_t *sel_tbl_info = NULL;
  bf_dev_id_t dev_id = 0;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get_first(dev_id, &tbl_hdl);

  if (sel_tbl_info) {
    *enable = sel_tbl_info->sequence_order;
    return PIPE_SUCCESS;
  }
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_selector_tbl_set_sequence_order(bool enable) {
  pipe_sel_tbl_hdl_t tbl_hdl;
  sel_tbl_info_t *sel_tbl_info = NULL;
  bf_dev_id_t dev_id = 0;
  Word_t count;

  /* Since sequence_order flag influence the way members are configured
   * in the group, dont allow to change the selector order when there are any
   * selector group present.
   */

  for (dev_id = 0; dev_id < PIPE_MGR_NUM_DEVICES; dev_id++) {
    sel_tbl_info = pipe_mgr_sel_tbl_info_get_first(dev_id, &tbl_hdl);
    while (sel_tbl_info) {
      count = pipe_mgr_sel_group_count(sel_tbl_info, BF_DEV_PIPE_ALL);
      if (count) {
        LOG_ERROR(
            "%s:%d Error: cant modify selector sequence when selector group "
            "is present",
            __func__,
            __LINE__);
        return PIPE_NOT_SUPPORTED;
      }
      sel_tbl_info = pipe_mgr_sel_tbl_info_get_next(dev_id, &tbl_hdl);
    }
  }

  for (dev_id = 0; dev_id < PIPE_MGR_NUM_DEVICES; dev_id++) {
    sel_tbl_info = pipe_mgr_sel_tbl_info_get_first(dev_id, &tbl_hdl);
    while (sel_tbl_info) {
      sel_tbl_info->sequence_order = enable;
      sel_tbl_info = pipe_mgr_sel_tbl_info_get_next(dev_id, &tbl_hdl);
    }
  }
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_sel_grp_get_mbr_offset(uint32_t word_idx,
                                                uint32_t mbr_index) {
  return (word_idx * SEL_GRP_WORD_WIDTH) + mbr_index;
}

static sel_mbr_offset_t *pipe_mgr_sel_mbr_offset_alloc(uint32_t offset) {
  sel_mbr_offset_t *mbr_offset = NULL;

  mbr_offset = PIPE_MGR_CALLOC(1, sizeof(sel_mbr_offset_t));
  if (mbr_offset == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }
  mbr_offset->offset = offset;
  mbr_offset->next = NULL;
  return mbr_offset;
}

static void pipe_mgr_sel_mbr_offset_destroy(sel_mbr_offset_t *mbr_offset) {
  PIPE_MGR_FREE(mbr_offset);
}

static void pipe_mgr_sel_mbr_offset_destroy_all(sel_mbr_offset_t *mbr_offset) {
  sel_mbr_offset_t *tmp = NULL;

  while (mbr_offset != NULL) {
    tmp = mbr_offset;
    mbr_offset = mbr_offset->next;
    pipe_mgr_sel_mbr_offset_destroy(tmp);
  }
}

void pipe_mgr_sel_grp_destroy(sel_grp_info_t *grp) {
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  Word_t Rc_word;
  sel_grp_mbr_t *grp_mbr = NULL;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  (void)mbr_hdl;

  if (!grp) {
    return;
  }

  (void)pipe_idx;
  {
    JUDYL_FOREACH(grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
      JLFA(Rc_word, stage_lookup);
      (void)Rc_word;
    }
  }
  JLFA(Rc_word, grp->sel_grp_pipe_lookup);
  (void)Rc_word;

  {
    JUDYL_FOREACH(grp->sel_grp_mbrs, mbr_hdl, sel_grp_mbr_t *, grp_mbr) {
      pipe_mgr_sel_grp_mbr_destroy(grp_mbr);
    }
    JLFA(Rc_word, grp->sel_grp_mbrs);
    (void)Rc_word;
  }

  {
    JUDYL_FOREACH(grp->backedup_mbrs, mbr_hdl, sel_grp_mbr_t *, grp_mbr) {
      pipe_mgr_sel_grp_mbr_destroy(grp_mbr);
    }
    JLFA(Rc_word, grp->backedup_mbrs);
    (void)Rc_word;
  }

  /* Clear mat_tbl_list only if group is not a backup one in order to avoid
   * double free,because both original and backup will point to the same list
   */
  while (grp->mat_tbl_list && !grp->is_grp_backup) {
    unsigned long key;
    void *data;
    sel_grp_mat_backptr_t *el = grp->mat_tbl_list;
    while (BF_MAP_OK ==
           bf_map_get_first_rmv(&el->ent_hdls, &key, (void **)&data))
      ;
    BF_LIST_DLL_REM(grp->mat_tbl_list, el, next, prev);
    PIPE_MGR_FREE(el);
  }

  PIPE_MGR_FREE(grp);
}

sel_grp_info_t *pipe_mgr_sel_grp_allocate(void) {
  sel_grp_info_t *sel_grp_info = NULL;

  sel_grp_info = (sel_grp_info_t *)PIPE_MGR_MALLOC(sizeof(sel_grp_info_t));
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMSET(sel_grp_info, 0, sizeof(sel_grp_info_t));

  return sel_grp_info;
}

sel_grp_info_t *pipe_mgr_sel_grp_get(sel_tbl_t *sel_tbl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl) {
  Word_t index;
  PWord_t Pvalue;

  index = sel_grp_hdl;
  JLG(Pvalue, sel_tbl->sel_grp_array, index);
  if (!Pvalue) {
    return NULL;
  }
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }
  return (sel_grp_info_t *)*Pvalue;
}

pipe_status_t pipe_mgr_sel_grp_add_to_htbl(sel_tbl_t *sel_tbl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           sel_grp_info_t *sel_grp_info) {
  Word_t index;
  PWord_t Pvalue;

  index = sel_grp_hdl;
  JLI(Pvalue, sel_tbl->sel_grp_array, index);
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (*Pvalue) {
    LOG_ERROR("%s:%d %s(0x%x %d) Sel grp 0x%x already exists",
              __func__,
              __LINE__,
              sel_tbl->sel_tbl_info->name,
              sel_tbl->sel_tbl_info->tbl_hdl,
              sel_tbl->sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_ALREADY_EXISTS;
  }

  *Pvalue = (Word_t)sel_grp_info;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_remove_from_htbl(
    sel_tbl_t *sel_tbl, pipe_sel_grp_hdl_t sel_grp_hdl) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  Word_t index;
  int Rc_int;

  index = sel_grp_hdl;
  JLD(Rc_int, sel_tbl->sel_grp_array, index);
  if (Rc_int != 1) {
    LOG_ERROR("%s:%d %s(0x%x %d) Sel grp 0x%x does not exist",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_update_mat_refs(sel_grp_info_t *sel_grp_info,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl) {
  bool found = false;
  sel_grp_mat_backptr_t *el = sel_grp_info->mat_tbl_list;
  while (el) {
    if (el->mat_tbl == mat_tbl_hdl) {
      found = true;
      break;
    }
    el = el->next;
  }
  if (!found) {
    el = (sel_grp_mat_backptr_t *)PIPE_MGR_CALLOC(
        1, sizeof(sel_grp_mat_backptr_t));
    el->mat_tbl = mat_tbl_hdl;
    BF_LIST_DLL_AP(sel_grp_info->mat_tbl_list, el, next, prev);
    bf_map_init(&el->ent_hdls);
  }
  bf_map_sts_t sts = bf_map_add(&el->ent_hdls, mat_ent_hdl, NULL);
  if (sts != BF_MAP_OK && sts != BF_MAP_KEY_EXISTS) {
    return sts;
  }
  return PIPE_SUCCESS;
}

static pipe_sel_grp_hdl_t pipe_mgr_sel_grp_hdl_allocate(sel_tbl_t *sel_tbl,
                                                        bf_dev_pipe_t pipe_id) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  Word_t index = PIPE_SEL_GRP_HDL_BASE;
  int Rc_int;

  if (pipe_id != BF_DEV_PIPE_ALL) {
    index = PIPE_SET_HDL_PIPE(index, pipe_id);
  }
  JLFE(Rc_int, sel_tbl->sel_grp_array, index);
  if (!Rc_int) {
    LOG_ERROR("%s:%d %s(0x%x) Error allocating grp hdl",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl);
    return PIPE_INVALID_SEL_GRP_HDL;
  }
  return index;
}

sel_grp_mbr_t *pipe_mgr_sel_grp_mbr_alloc(pipe_sel_grp_mbr_hdl_t mbr_hdl) {
  sel_grp_mbr_t *grp_mbr = NULL;

  grp_mbr = PIPE_MGR_MALLOC(sizeof(sel_grp_mbr_t));
  if (grp_mbr == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }
  grp_mbr->is_backup_valid = false;
  grp_mbr->state = PIPE_MGR_GRP_MBR_STATE_ACTIVE;
  grp_mbr->mbr_hdl = mbr_hdl;
  grp_mbr->weight = 1;
  grp_mbr->mbrs_pos = NULL;
  return grp_mbr;
}

static void sel_grp_pos_destroy(sel_mbr_pos_t *mbr_pos) {
  PIPE_MGR_FREE(mbr_pos);
}

static void sel_grp_pos_destroy_all(sel_mbr_pos_t *mbr_pos) {
  sel_mbr_pos_t *tmp = NULL;

  while (mbr_pos != NULL) {
    tmp = mbr_pos;
    mbr_pos = mbr_pos->next;
    sel_grp_pos_destroy(tmp);
  }

  return;
}

static void pipe_mgr_sel_grp_mbr_destroy(sel_grp_mbr_t *grp_mbr) {
  if (grp_mbr->mbrs_pos) sel_grp_pos_destroy_all(grp_mbr->mbrs_pos);
  PIPE_MGR_FREE(grp_mbr);
}

sel_grp_mbr_t *pipe_mgr_sel_grp_mbr_get(sel_grp_info_t *sel_grp_info,
                                        pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
                                        bool is_backup) {
  PWord_t Pvalue;
  Pvoid_t *mbr_array;

  if (is_backup) {
    mbr_array = &sel_grp_info->backedup_mbrs;
  } else {
    mbr_array = &sel_grp_info->sel_grp_mbrs;
  }

  JLG(Pvalue, *mbr_array, (Word_t)sel_grp_mbr_hdl);
  if (!Pvalue) {
    return NULL;
  }
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }
  return (sel_grp_mbr_t *)*Pvalue;
}

static sel_mbr_pos_t *sel_grp_pos_alloc(uint32_t pos) {
  sel_mbr_pos_t *mbr_pos = NULL;

  mbr_pos = PIPE_MGR_CALLOC(1, sizeof(sel_mbr_pos_t));
  if (mbr_pos == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }
  mbr_pos->pos = pos;
  mbr_pos->next = NULL;
  return mbr_pos;
}

/**
 * Function to add/del the position of member in selector group info
 */

static pipe_status_t sel_grp_mbr_add_del_pos(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t pos,
    bool sequence_order,
    enum pipe_sel_update_type sel_op_type) {
  sel_grp_mbr_t *grp_mbr = NULL;
  sel_mbr_pos_t *mbr_pos = NULL;
  sel_mbr_pos_t *prev = NULL, *cur = NULL;

  if (!sequence_order) {
    return PIPE_SUCCESS;
  }

  grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);

  if (!grp_mbr) {
    LOG_ERROR("%s:%d Sel group member %d not found in group %d",
              __func__,
              __LINE__,
              sel_grp_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (sel_op_type == PIPE_SEL_UPDATE_ADD) {
    mbr_pos = sel_grp_pos_alloc(pos);
    if (!mbr_pos) {
      return PIPE_NO_SYS_RESOURCES;
    }
    mbr_pos->next = grp_mbr->mbrs_pos;
    grp_mbr->mbrs_pos = mbr_pos;

    return PIPE_SUCCESS;
  } else if (sel_op_type == PIPE_SEL_UPDATE_DEL && grp_mbr->mbrs_pos != NULL) {
    cur = grp_mbr->mbrs_pos;
    if (cur->pos == pos) {
      grp_mbr->mbrs_pos = cur->next;
    } else {
      while (cur && cur->pos != pos) {
        prev = cur;
        cur = cur->next;
      }
      if (cur == NULL) {
        LOG_ERROR(
            "%s:%d Sel member pos %d not found for member handle %d in group "
            "0x%x",
            __func__,
            __LINE__,
            pos,
            sel_grp_mbr_hdl,
            sel_grp_info->grp_hdl);
        return PIPE_OBJ_NOT_FOUND;
      }
      prev->next = cur->next;
    }
    sel_grp_pos_destroy(cur);
    return PIPE_SUCCESS;

  } else {
    LOG_ERROR(
        "%s:%d Invalid selector update type %d for member handle %d in group "
        "0x%x",
        __func__,
        __LINE__,
        sel_op_type,
        sel_grp_mbr_hdl,
        sel_grp_info->grp_hdl);
    return PIPE_INVALID_ARG;
  }
}

/*
 * Function to update the member positon when members are moved within the grp
 */

static pipe_status_t pipe_mgr_sel_grp_mbr_update_pos(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t tgt_pos,
    uint32_t src_pos,
    bool sequence_order) {
  sel_grp_mbr_t *grp_mbr = NULL;
  sel_mbr_pos_t *mbr_pos = NULL;

  if (!sequence_order) {
    return PIPE_SUCCESS;
  }

  grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);

  if (!grp_mbr || !grp_mbr->mbrs_pos) {
    LOG_ERROR("%s:%d Sel group member %d not found in group %d",
              __func__,
              __LINE__,
              sel_grp_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  mbr_pos = grp_mbr->mbrs_pos;

  while (mbr_pos && mbr_pos->pos != src_pos) {
    mbr_pos = mbr_pos->next;
  }
  if (mbr_pos) {
    mbr_pos->pos = tgt_pos;
    return PIPE_SUCCESS;
  }
  LOG_ERROR("%s:%d Sel member pos %d not found for member %d in group %d",
            __func__,
            __LINE__,
            src_pos,
            sel_grp_mbr_hdl,
            sel_grp_info->grp_hdl);
  return PIPE_OBJ_NOT_FOUND;
}

void pipe_mgr_sel_grp_mbr_set_state(sel_grp_info_t *sel_grp_info,
                                    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
                                    bool enable) {
  sel_grp_mbr_t *mbr =
      pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  if (mbr) {
    mbr->state = enable ? PIPE_MGR_GRP_MBR_STATE_ACTIVE
                        : PIPE_MGR_GRP_MBR_STATE_INACTIVE;
  }
  return;
}

bool pipe_mgr_sel_grp_mbr_get_state(sel_grp_info_t *sel_grp_info,
                                    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl) {
  sel_grp_mbr_t *mbr =
      pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  return mbr && (mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE);
}

pipe_status_t pipe_mgr_sel_mbr_add_to_htbl(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    sel_grp_mbr_t *grp_mbr,
    bool is_backup) {
  PWord_t Pvalue;
  Pvoid_t *mbr_array;

  if (is_backup) {
    mbr_array = &sel_grp_info->backedup_mbrs;
  } else {
    mbr_array = &sel_grp_info->sel_grp_mbrs;
  }

  JLI(Pvalue, *mbr_array, (Word_t)sel_grp_mbr_hdl);
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (*Pvalue) {
    LOG_ERROR("%s:%d Sel grp 0x%x mbr 0x%x already exists",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl,
              sel_grp_mbr_hdl);
    return PIPE_ALREADY_EXISTS;
  }

  *Pvalue = (Word_t)grp_mbr;
  return PIPE_SUCCESS;
}

sel_grp_mbr_t *pipe_mgr_sel_grp_mbr_remove(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    bool is_backup) {
  int Rc_int;
  Pvoid_t *mbr_array;
  sel_grp_mbr_t *mbr;

  mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, is_backup);
  if (!mbr) {
    return NULL;
  }
  if (is_backup) {
    mbr_array = &sel_grp_info->backedup_mbrs;
  } else {
    mbr_array = &sel_grp_info->sel_grp_mbrs;
  }

  JLD(Rc_int, *mbr_array, (Word_t)sel_grp_mbr_hdl);
  if (!Rc_int) {
    LOG_ERROR("%s:%d Sel grp 0x%x mbr 0x%x does not exist",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl,
              sel_grp_mbr_hdl);
    return NULL;
  }

  return mbr;
}

pipe_status_t pipe_mgr_sel_grp_mbr_remove_and_destroy(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    bool is_backup) {
  int Rc_int;
  Pvoid_t *mbr_array;
  sel_grp_mbr_t *mbr;

  mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, is_backup);
  if (!mbr) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Decrement the counter incase of duplicate entries */
  if (mbr->weight > 1) {
    mbr->weight--;
    return PIPE_SUCCESS;
  }

  if (is_backup) {
    mbr_array = &sel_grp_info->backedup_mbrs;
  } else {
    mbr_array = &sel_grp_info->sel_grp_mbrs;
  }

  JLD(Rc_int, *mbr_array, (Word_t)sel_grp_mbr_hdl);
  if (!Rc_int) {
    LOG_ERROR("%s:%d Sel grp 0x%x mbr 0x%x does not exist",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl,
              sel_grp_mbr_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_sel_grp_mbr_destroy(mbr);

  return PIPE_SUCCESS;
}

static int sel_grp_profile_sort_func(const void *a, const void *b) {
  const pipe_sel_grp_profile_t *p1 = a, *p2 = b;

  /* Sort in descending order */
  if (p1->grp_size > p2->grp_size) {
    return -1;
  } else if (p1->grp_size < p2->grp_size) {
    return 1;
  }

  return 0;
}

pipe_stful_tbl_hdl_t pipe_mgr_sel_tbl_get_stful_ref(
    bf_dev_id_t dev_id, pipe_sel_tbl_hdl_t tbl_hdl) {
  sel_tbl_info_t *x = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  return x ? x->stful_tbl_hdl : 0;
}

/** \brief pipe_mgr_sel_tbl_info_get
 *        Get the sel tbl pointer for a given table handle
 *
 * This function returns the sel tbl pointer stored in the sel_tbl_htbl
 * or it's backup table
 *
 * \param dev_tgt Device target
 * \param tbl_hdl sel table handle
 * \param is_backup Flag to indicate if a backup table/main table is needed
 * \return sel_tbl_t* Pointer to the sel_tbl structure
 */
sel_tbl_info_t *pipe_mgr_sel_tbl_info_get(bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t tbl_hdl,
                                          bool is_backup) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  unsigned long key = tbl_hdl;
  bf_map_sts_t msts;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  msts =
      pipe_mgr_sel_tbl_map_get(dev_id, key, (void **)&sel_tbl_info, is_backup);

  if (msts != BF_MAP_OK) {
    return NULL;
  }
  return sel_tbl_info;
}

sel_tbl_info_t *pipe_mgr_sel_tbl_info_get_first(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  unsigned long key = *tbl_hdl_p;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_sel_tbl_map_get_first(dev_id, &key, (void **)&sel_tbl_info);

  if (msts != BF_MAP_OK) {
    return NULL;
  }

  *tbl_hdl_p = key;
  return sel_tbl_info;
}

sel_tbl_info_t *pipe_mgr_sel_tbl_info_get_next(bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  unsigned long key = *tbl_hdl_p;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_sel_tbl_map_get_next(dev_id, &key, (void **)&sel_tbl_info);

  if (msts != BF_MAP_OK) {
    return NULL;
  }
  *tbl_hdl_p = key;
  return sel_tbl_info;
}

uint32_t pipe_mgr_sel_tbl_get_stage_idx(sel_tbl_t *sel_tbl, uint32_t stage_id) {
  uint32_t i = 0;

  for (i = 0; i < sel_tbl->num_stages; i++) {
    if (sel_tbl->sel_tbl_stage_info[i].stage_id == stage_id) {
      return i;
    }
  }

  return -1;
}

static void pipe_mgr_sel_hlp_word_destroy(sel_hlp_word_data_t *sel_word,
                                          uint32_t no_words) {
  uint32_t i = 0;

  if (!sel_word) {
    return;
  }

  for (i = 0; i < no_words; i++) {
    if (sel_word[i].mbrs) {
      PIPE_MGR_FREE(sel_word[i].mbrs);
    }
  }

  PIPE_MGR_FREE(sel_word);
}

static void pipe_mgr_sel_llp_word_destroy(sel_llp_word_data_t *sel_word,
                                          uint32_t no_words) {
  uint32_t i = 0;

  if (!sel_word) {
    return;
  }

  for (i = 0; i < no_words; i++) {
    if (sel_word[i].mbrs) {
      PIPE_MGR_FREE(sel_word[i].mbrs);
    }
    if (sel_word[i].data) {
      PIPE_MGR_FREE(sel_word[i].data);
    }
  }

  PIPE_MGR_FREE(sel_word);
}

static sel_hlp_word_data_t *pipe_mgr_sel_hlp_word_alloc(
    uint32_t no_words, uint32_t ram_word_width) {
  sel_hlp_word_data_t *sel_word = NULL;
  uint32_t i = 0;

  sel_word = (sel_hlp_word_data_t *)PIPE_MGR_MALLOC(
      sizeof(sel_hlp_word_data_t) * no_words);
  if (sel_word == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    goto cleanup;
  }

  PIPE_MGR_MEMSET(sel_word, 0, sizeof(sel_hlp_word_data_t) * no_words);

  for (i = 0; i < no_words; i++) {
    sel_word[i].mbrs = (pipe_sel_grp_mbr_hdl_t *)PIPE_MGR_MALLOC(
        sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);
    if (sel_word[i].mbrs == NULL) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMSET(
        sel_word[i].mbrs, 0, sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);
  }

  return sel_word;
cleanup:
  pipe_mgr_sel_hlp_word_destroy(sel_word, no_words);
  return NULL;
}

static sel_llp_word_data_t *pipe_mgr_sel_llp_word_alloc(
    uint32_t no_words, uint32_t ram_word_width) {
  sel_llp_word_data_t *sel_word = NULL;
  uint32_t data_sz_bytes = 0;
  uint32_t i = 0;

  sel_word = (sel_llp_word_data_t *)PIPE_MGR_MALLOC(
      sizeof(sel_llp_word_data_t) * no_words);
  if (sel_word == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    goto cleanup;
  }

  PIPE_MGR_MEMSET(sel_word, 0, sizeof(sel_llp_word_data_t) * no_words);
  data_sz_bytes = ((ram_word_width - 1) >> 3) + 1;

  for (i = 0; i < no_words; i++) {
    sel_word[i].highest_mbr_idx = -1;

    sel_word[i].mbrs = (pipe_sel_grp_mbr_hdl_t *)PIPE_MGR_MALLOC(
        sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);
    if (sel_word[i].mbrs == NULL) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMSET(
        sel_word[i].mbrs, 0, sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);

    sel_word[i].data =
        (uint8_t *)PIPE_MGR_MALLOC(sizeof(uint8_t) * data_sz_bytes);
    if (sel_word[i].data == NULL) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMSET(sel_word[i].data, 0, sizeof(uint8_t) * data_sz_bytes);
  }

  return sel_word;
cleanup:
  pipe_mgr_sel_llp_word_destroy(sel_word, no_words);
  return NULL;
}

static void pipe_mgr_sel_grp_stage_info_destroy(
    sel_grp_stage_info_t *sel_grp_stage_info) {
  Word_t Rc_word;
  (void)Rc_word;
  pipe_sel_grp_mbr_hdl_t mbr_hdl = 0;
  sel_mbr_offset_t *mbr_offset = NULL;
  (void)mbr_hdl;

  if (!sel_grp_stage_info) {
    return;
  }

  JUDYL_FOREACH(sel_grp_stage_info->mbr_locator,
                mbr_hdl,
                sel_mbr_offset_t *,
                mbr_offset) {
    pipe_mgr_sel_mbr_offset_destroy_all(mbr_offset);
  }

  JLFA(Rc_word, sel_grp_stage_info->mbr_locator);

  PIPE_MGR_FREE(sel_grp_stage_info);
}

void pipe_mgr_sel_grp_stage_list_free_all(
    sel_grp_stage_info_t *grp_stage_list) {
  while (grp_stage_list) {
    sel_grp_stage_info_t *grp_stage_info = grp_stage_list;
    BF_LIST_DLL_REM(grp_stage_list, grp_stage_info, next, prev);

    pipe_mgr_sel_grp_stage_info_destroy(grp_stage_info);
  }
}

static void pipe_mgr_sel_tbl_stage_info_destroy(
    sel_tbl_stage_info_t *sel_tbl_stage_info_p, uint8_t num_stages) {
  uint32_t i = 0;
  sel_tbl_stage_info_t *sel_tbl_stage_info = NULL;

  if (!sel_tbl_stage_info_p) {
    return;
  }

  for (i = 0; i < num_stages; i++) {
    sel_tbl_stage_info = &sel_tbl_stage_info_p[i];

    pipe_mgr_sel_grp_stage_list_free_all(
        sel_tbl_stage_info->sel_grp_stage_free_list);
    pipe_mgr_sel_grp_stage_list_free_all(
        sel_tbl_stage_info->sel_grp_stage_inuse_list);

    power2_allocator_destroy(sel_tbl_stage_info->stage_sel_grp_allocator);

    pipe_mgr_sel_hlp_word_destroy(sel_tbl_stage_info->hlp.hlp_word,
                                  sel_tbl_stage_info->no_words);

    pipe_mgr_sel_llp_word_destroy(sel_tbl_stage_info->llp.llp_word,
                                  sel_tbl_stage_info->no_words);

    PIPE_MGR_FREE(sel_tbl_stage_info->pv_hw.tbl_blk);
  }

  PIPE_MGR_FREE(sel_tbl_stage_info_p);
}

static void pipe_mgr_sel_tbl_destroy(sel_tbl_t *sel_tbl_p,
                                     uint32_t no_sel_tbls,
                                     bool is_backup) {
  sel_tbl_t *sel_tbl = NULL;
  Word_t grp_hdl = 0;
  PWord_t Pvalue = NULL;
  Word_t Rc_word;
  (void)Rc_word;
  uint32_t i = 0;

  if (!sel_tbl_p) {
    return;
  }

  for (i = 0; i < no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_p[i];

    /* Free all the groups for the give table instance. */
    JLF(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
    while (Pvalue) {
      pipe_mgr_sel_grp_destroy((sel_grp_info_t *)*Pvalue);
      JLN(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
    }
    JLFA(Rc_word, sel_tbl->sel_grp_array);

    pipe_mgr_sel_tbl_stage_info_destroy(sel_tbl->sel_tbl_stage_info,
                                        sel_tbl->num_stages);
    bf_map_destroy(&sel_tbl->grp_id_map);
  }

  /* Clean up HA state */
  pipe_mgr_selector_cleanup_hlp_ha_state(sel_tbl_p->sel_tbl_info->dev_id,
                                         sel_tbl_p->sel_tbl_info->tbl_hdl,
                                         is_backup);
  pipe_mgr_selector_cleanup_llp_ha_state(sel_tbl_p->sel_tbl_info->dev_id,
                                         sel_tbl_p->sel_tbl_info->tbl_hdl,
                                         is_backup);
  PIPE_MGR_FREE(sel_tbl_p);
}

static pipe_status_t pipe_mgr_sel_update_hw_details(
    sel_tbl_stage_hw_info_t *hw_info, rmt_tbl_info_t *rmt_info) {
  hw_info->tbl_id = rmt_info->handle;
  hw_info->pack_format = rmt_info->pack_format;
  hw_info->data_sz = ((rmt_info->pack_format.mem_word_width - 1) >> 3) + 1;
  hw_info->num_blks = rmt_info->bank_map->num_tbl_word_blks;
  hw_info->block_width = rmt_info->pack_format.mem_units_per_tbl_word;
  hw_info->num_spare_rams = rmt_info->num_spare_rams;
  if (hw_info->num_spare_rams > 0)
    PIPE_MGR_MEMCPY(hw_info->spare_rams,
                    rmt_info->spare_rams,
                    sizeof(mem_id_t) * hw_info->num_spare_rams);

  hw_info->tbl_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_MALLOC(
      sizeof(rmt_tbl_word_blk_t) * hw_info->num_blks);
  if (hw_info->tbl_blk == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(hw_info->tbl_blk,
                  rmt_info->bank_map->tbl_word_blk,
                  sizeof(rmt_tbl_word_blk_t) * hw_info->num_blks);
  return PIPE_SUCCESS;
}

static pipe_status_t set_initial_value(pipe_sess_hdl_t sess_hdl,
                                       sel_tbl_info_t *sel_tbl_info,
                                       pipe_select_tbl_info_t *sel_tbl_rmt_info,
                                       bool hw_init) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_t *sel_tbl = NULL;
  bf_dev_pipe_t p = 0;
  int pipe_mask = 0;
  uint32_t j = 0;
  uint32_t i;

  if (pipe_mgr_is_device_virtual(sel_tbl_info->dev_id)) {
    return PIPE_SUCCESS;
  }

  bf_dev_id_t dev_id = sel_tbl_info->dev_id;
  rmt_dev_cfg_t *dev_cfg = &sel_tbl_info->dev_info->dev_cfg;

  uint32_t rmt_tbl;
  for (rmt_tbl = 0; rmt_tbl < sel_tbl_rmt_info->num_rmt_info; rmt_tbl++) {
    rmt_tbl_info_t *rmt_info = &sel_tbl_rmt_info->rmt_info[rmt_tbl];
    if (rmt_info->type != RMT_TBL_TYPE_SEL_PORT_VEC) {
      continue;
    }
    for (j = 0; j < sel_tbl_info->no_sel_tbls; j++) {
      sel_tbl = &sel_tbl_info->sel_tbl[j];

      /* Build a bit mask of pipes for the block write. */
      pipe_mask = 0;
      PIPE_BITMAP_ITER(&sel_tbl->inst_pipe_bmp, p) { pipe_mask |= 1 << p; }

      uint32_t bank = 0;
      for (bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
        uint32_t blk;
        for (blk = 0; blk < rmt_info->bank_map[bank].num_tbl_word_blks; blk++) {
          mem_id_t mem_id =
              rmt_info->bank_map[bank].tbl_word_blk[blk].mem_id[0];
          vpn_id_t vpn_id =
              rmt_info->bank_map[bank].tbl_word_blk[blk].vpn_id[0];
          /* Update our RAM shadow with the symmetric mode of the
           * table/memories. */
          rc = pipe_mgr_phy_mem_map_symmetric_mode_set(
              dev_id,
              sel_tbl_info->direction,
              &sel_tbl->inst_pipe_bmp,
              rmt_info->stage_id,
              pipe_mem_type_unit_ram,
              mem_id,
              sel_tbl_info->is_symmetric);
          if (rc != PIPE_SUCCESS) {
            PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
            return rc;
          }

          /* Skip map ram init during hitless HA */
          if (hw_init && !pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
            uint64_t map_phy_addr = 0;
            mem_id_t map_ram_id = dev_cfg->map_ram_from_unit_ram(mem_id);
            map_phy_addr = dev_cfg->get_full_phy_addr(rmt_info->direction,
                                                      0,
                                                      rmt_info->stage_id,
                                                      map_ram_id,
                                                      0,
                                                      pipe_mem_type_map_ram);
            uint8_t map_ram_data[4] = {~vpn_id & 0x3F, 0, 0, 0};
            rc = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                          sel_tbl_info->dev_info,
                                          4,
                                          TOF_MAP_RAM_UNIT_DEPTH,
                                          1,
                                          map_phy_addr,
                                          pipe_mask,
                                          map_ram_data);
            if (rc != PIPE_SUCCESS) {
              PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
              return rc;
            }
          }
        }
      }
      /* Block write the map RAM for the spare RAM to zeros. */
      for (i = 0; i < rmt_info->num_spare_rams; i++) {
        rc =
            pipe_mgr_phy_mem_map_symmetric_mode_set(dev_id,
                                                    sel_tbl_info->direction,
                                                    &sel_tbl->inst_pipe_bmp,
                                                    rmt_info->stage_id,
                                                    pipe_mem_type_unit_ram,
                                                    rmt_info->spare_rams[i],
                                                    sel_tbl_info->is_symmetric);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
          return rc;
        }
        /* Skip map ram init during hitless HA */
        if (hw_init && !pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
          uint64_t map_phy_addr = 0;
          mem_id_t map_ram_id =
              dev_cfg->map_ram_from_unit_ram(rmt_info->spare_rams[i]);
          map_phy_addr = dev_cfg->get_full_phy_addr(rmt_info->direction,
                                                    0,
                                                    rmt_info->stage_id,
                                                    map_ram_id,
                                                    0,
                                                    pipe_mem_type_map_ram);
          uint8_t map_ram_data[4] = {0, 0, 0, 0};
          rc = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                        sel_tbl_info->dev_info,
                                        4,
                                        TOF_MAP_RAM_UNIT_DEPTH,
                                        1,
                                        map_phy_addr,
                                        pipe_mask,
                                        map_ram_data);
          if (rc != PIPE_SUCCESS) {
            PIPE_MGR_DBGCHK(0);
            return rc;
          }
        }
      }
    }  // for no_sel_tbls
  }
  return PIPE_SUCCESS;
}

static sel_tbl_t *pipe_mgr_sel_tbl_alloc(
    pipe_sess_hdl_t sess_hdl,
    sel_tbl_info_t *sel_tbl_info,
    pipe_select_tbl_info_t *sel_tbl_rmt_info,
    uint32_t *total_entries_p,
    bool is_backup) {
  sel_tbl_t *sel_tbl = NULL;
  uint8_t no_sel_tbls = 1;
  uint32_t i = 0, j = 0, k = 0;
  rmt_tbl_info_t *rmt_info = NULL;
  uint32_t num_stages = 0, stage = 0, no_words = 0;
  sel_tbl_stage_info_t *sel_tbl_stage_info = NULL;
  sel_tbl_stage_info_t *sel_tbl_stage_info_p = NULL;
  uint32_t total_words = 0;

  (void)sess_hdl;
  no_sel_tbls = sel_tbl_info->no_sel_tbls;

  sel_tbl = (sel_tbl_t *)PIPE_MGR_MALLOC(sizeof(sel_tbl_t) * no_sel_tbls);
  if (sel_tbl == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  num_stages = sel_tbl_rmt_info->num_stages;
  PIPE_MGR_MEMSET(sel_tbl, 0, sizeof(sel_tbl_t) * no_sel_tbls);

  for (i = 0; i < no_sel_tbls; i++) {
    sel_tbl[i].sel_tbl_info = sel_tbl_info;
    bf_map_init(&sel_tbl[i].grp_id_map);

    if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) {
      sel_tbl[i].pipe_id = BF_DEV_PIPE_ALL;
    } else {
      sel_tbl[i].pipe_id =
          pipe_mgr_get_lowest_pipe_in_scope(sel_tbl_info->scope_pipe_bmp[i]);
    }
    sel_tbl[i].pipe_idx = i;
    PIPE_BITMAP_INIT(&sel_tbl[i].inst_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(sel_tbl_info->scope_pipe_bmp[i],
                                    &sel_tbl[i].inst_pipe_bmp);

    sel_tbl[i].num_stages = num_stages;

    sel_tbl_stage_info = (sel_tbl_stage_info_t *)PIPE_MGR_MALLOC(
        sizeof(sel_tbl_stage_info_t) * num_stages);
    if (sel_tbl_stage_info == NULL) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }

    PIPE_MGR_MEMSET(
        sel_tbl_stage_info, 0, sizeof(sel_tbl_stage_info_t) * num_stages);

    for (j = 0, stage = 0; j < sel_tbl_rmt_info->num_rmt_info; j++) {
      rmt_info = &sel_tbl_rmt_info->rmt_info[j];
      if (rmt_info->type != RMT_TBL_TYPE_SEL_PORT_VEC) {
        continue;
      }

      sel_tbl_stage_info_p = &sel_tbl_stage_info[stage];

      sel_tbl_stage_info_p->stage_id = rmt_info->stage_id;
      sel_tbl_stage_info_p->stage_idx = stage;
      sel_tbl_stage_info_p->num_alu_ids =
          rmt_info->params.sel_params.num_alu_indexes;
      for (k = 0; k < sel_tbl_stage_info_p->num_alu_ids; k++) {
        sel_tbl_stage_info_p->alu_ids[k] =
            rmt_info->params.sel_params.alu_indexes[k];
      }
      sel_tbl_stage_info_p->ram_word_width =
          rmt_info->pack_format.mem_word_width *
          rmt_info->pack_format.mem_units_per_tbl_word;

      sel_tbl_stage_info_p->no_words =
          1 + ((rmt_info->num_entries - 1) /
               rmt_info->pack_format.entries_per_tbl_word);

      no_words = sel_tbl_stage_info_p->no_words;
      total_words += no_words;

      if (!is_backup) {
        sel_tbl_stage_info_p->stage_sel_grp_allocator = power2_allocator_create(
            TOF_SRAM_UNIT_DEPTH, rmt_info->bank_map->num_tbl_word_blks);
        /* If there is an associated stateful table we try to keep the stateful
         * table size in sync with the selection table allocations.  A group
         * should not be allocated in a location that would be out of bounds for
         * the associated stateful table. DRV-3174. */
        if (sel_tbl_info->stful_tbl_hdl) {
          pipe_stful_tbl_info_t *stful_info =
              pipe_mgr_get_stful_tbl_info(sel_tbl_info->dev_id,
                                          sel_tbl_info->stful_tbl_hdl,
                                          __func__,
                                          __LINE__);
          if (stful_info == NULL) {
            LOG_ERROR(
                "%s:%d Cannot find stateful table 0x%x associated with select "
                "table %s (0x%x) for device %d",
                __func__,
                __LINE__,
                sel_tbl_info->stful_tbl_hdl,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id);
            goto cleanup;
          }
          uint32_t stful_sz = stful_info->size;
          stful_sz = stful_sz / 128 * 128; /* Reduce to multiple of 128 bits. */
          uint32_t sel_sz =
              TOF_SRAM_UNIT_DEPTH * rmt_info->bank_map->num_tbl_word_blks * 128;
          if (stful_sz < sel_sz) {
            uint32_t excess_bits = sel_sz - stful_sz;
            LOG_WARN(
                "Dev %d reducing selector table %s (0x%x) by %d bits from %d "
                "to %d to match stateful table %s (0x%x) of size %d.  This may "
                "reduce the number of groups allowed in the selection table.",
                sel_tbl_info->dev_id,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                excess_bits,
                sel_sz,
                sel_sz - excess_bits,
                stful_info->name,
                stful_info->handle,
                stful_info->size);

            uint32_t reserve_idx = sel_sz / 128;
            uint32_t excess_lines = excess_bits / 128;
            while (excess_lines) {
              /* Can only reserve 1024 lines at a time since the power2
               * allocator was created as groups of 1024 lines. */
              uint32_t reserve_now = excess_lines > 1024 ? 1024 : excess_lines;
              reserve_idx -= reserve_now;
              int rc = power2_allocator_reserve(
                  sel_tbl_stage_info_p->stage_sel_grp_allocator,
                  reserve_idx,
                  reserve_now);
              if (rc != 0) {
                LOG_ERROR(
                    "Dev %d selector table %s (0x%x) failed to reserve %d "
                    "excess lines from line %d: %d",
                    sel_tbl_info->dev_id,
                    sel_tbl_info->name,
                    sel_tbl_info->tbl_hdl,
                    reserve_now,
                    reserve_idx,
                    rc);
                goto cleanup;
              }
              excess_lines -= reserve_now;
            }
          }
        }
      }

      sel_tbl_stage_info_p->hlp.hlp_word = pipe_mgr_sel_hlp_word_alloc(
          no_words, sel_tbl_stage_info_p->ram_word_width);
      if (!sel_tbl_stage_info_p->hlp.hlp_word) {
        LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
        goto cleanup;
      }

      sel_tbl_stage_info_p->llp.llp_word = pipe_mgr_sel_llp_word_alloc(
          no_words, sel_tbl_stage_info_p->ram_word_width);
      if (!sel_tbl_stage_info_p->llp.llp_word) {
        LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
        goto cleanup;
      }

      pipe_mgr_sel_update_hw_details(&sel_tbl_stage_info_p->pv_hw, rmt_info);

      stage++;
    }

    sel_tbl[i].sel_tbl_stage_info = sel_tbl_stage_info;

    if (!is_backup) {
      if (pipe_mgr_sel_pipe_ha_hlp_init(&sel_tbl[i]) != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initializing pipe ha hlp info for pipe %x "
            "tbl 0x%x",
            __func__,
            __LINE__,
            sel_tbl[i].pipe_id,
            sel_tbl_info->tbl_hdl);
        goto cleanup;
      }
      if (!pipe_mgr_is_device_virtual(sel_tbl_info->dev_id)) {
        if (pipe_mgr_sel_pipe_ha_llp_init(&sel_tbl[i]) != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in initializing pipe ha llp info for pipe %x "
              "tbl 0x%x",
              __func__,
              __LINE__,
              sel_tbl[i].pipe_id,
              sel_tbl_info->tbl_hdl);
          goto cleanup;
        }
      }
    }
  }

  *total_entries_p = total_words;

  return sel_tbl;
cleanup:
  pipe_mgr_sel_tbl_destroy(sel_tbl, no_sel_tbls, is_backup);
  return NULL;
}

static void pipe_mgr_sel_tbl_info_destroy(sel_tbl_info_t *sel_tbl_info,
                                          bool is_backup) {
  if (!sel_tbl_info) {
    return;
  }

  /* Destroy all the table instances. */
  pipe_mgr_sel_tbl_destroy(
      sel_tbl_info->sel_tbl, sel_tbl_info->no_sel_tbls, is_backup);

  if (sel_tbl_info->scope_pipe_bmp) {
    PIPE_MGR_FREE(sel_tbl_info->scope_pipe_bmp);
  }

  pipe_mgr_selector_tbl_cleanup_llp_ha_state(sel_tbl_info);

  PIPE_MGR_FREE(sel_tbl_info->name);
  PIPE_MGR_FREE(sel_tbl_info);
}

/** \brief pipe_mgr_sel_tbl_create_internal
 *        Internal helper function to create a sel_tbl structure
 *
 * Function mallocs a sel-tbl structure, updates all the relevant stuff
 * inside and also mallocs any of the sub-structures/inits the different
 * trees etc.
 *
 * \param  Same as pipe_mgr_sel_tbl_create
 * \param is_backup Flag to specify if a backup table is being created
 * \return pipe_status_t Status of the operation
 */
static pipe_status_t pipe_mgr_sel_tbl_create_internal(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_select_tbl_info_t *select_tbl_rmt_info,
    bool is_backup,
    profile_id_t profile_id,
    pipe_bitmap_t *pipe_bmp) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t max_grps = 0, q = 0;

  sel_tbl_info = (sel_tbl_info_t *)PIPE_MGR_MALLOC(sizeof(sel_tbl_info_t));
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  PIPE_MGR_MEMSET(sel_tbl_info, 0, sizeof(sel_tbl_info_t));

  sel_tbl_info->name = bf_sys_strdup(select_tbl_rmt_info->name);
  sel_tbl_info->direction = select_tbl_rmt_info->direction;
  sel_tbl_info->dev_id = dev_id;
  sel_tbl_info->dev_info = pipe_mgr_get_dev_info(dev_id);
  PIPE_MGR_ASSERT(sel_tbl_info->dev_info != NULL);
  sel_tbl_info->tbl_hdl = tbl_hdl;
#ifdef UTEST
  sel_tbl_info->adt_tbl_hdl = 0;
#else
  sel_tbl_info->adt_tbl_hdl = select_tbl_rmt_info->adt_tbl_hdl;
#endif
  sel_tbl_info->stful_tbl_hdl = select_tbl_rmt_info->stful_tbl_hdl;
  sel_tbl_info->mode = select_tbl_rmt_info->mode;
  sel_tbl_info->is_symmetric = select_tbl_rmt_info->symmetric;
  sel_tbl_info->sps_enable = select_tbl_rmt_info->sps_enable;
  sel_tbl_info->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!sel_tbl_info->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  /* Set the scope info */
  if (sel_tbl_info->is_symmetric) {
    sel_tbl_info->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      sel_tbl_info->scope_pipe_bmp[0] |= (1 << q);
    }
  } else {
    sel_tbl_info->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      sel_tbl_info->scope_pipe_bmp[q] |= (1 << q);
      sel_tbl_info->num_scopes += 1;
    }
  }

  sel_tbl_info->max_grp_size = select_tbl_rmt_info->max_group_size;
  sel_tbl_info->profile_id = profile_id;
  sel_tbl_info->cache_id = false;
  PIPE_BITMAP_INIT(&sel_tbl_info->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&sel_tbl_info->pipe_bmp, pipe_bmp);
  LOG_TRACE("%s: Table %s, Pipe bitmap count %d ",
            __func__,
            sel_tbl_info->name,
            PIPE_BITMAP_COUNT(&sel_tbl_info->pipe_bmp));

  sel_tbl_info->no_sel_tbls = sel_tbl_info->num_scopes;
  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) {
    PIPE_MGR_ASSERT(sel_tbl_info->num_scopes == 1);
    sel_tbl_info->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(sel_tbl_info->scope_pipe_bmp[0]);
  }

  sel_tbl_info->sel_tbl = pipe_mgr_sel_tbl_alloc(
      sess_hdl, sel_tbl_info, select_tbl_rmt_info, &max_grps, is_backup);
  if (sel_tbl_info->sel_tbl == NULL) {
    LOG_ERROR("%s:%d Select table creation failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  if (!is_backup && !pipe_mgr_is_device_virtual(dev_id)) {
    rc = set_initial_value(sess_hdl, sel_tbl_info, select_tbl_rmt_info, true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Error setting initial value rc 0x%x",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                rc);
      pipe_mgr_sel_tbl_destroy(
          sel_tbl_info->sel_tbl, sel_tbl_info->no_sel_tbls, is_backup);
      sel_tbl_info->sel_tbl = NULL;
      goto cleanup;
    }

    rc = pipe_mgr_sel_ha_llp_init(sel_tbl_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing HA info for sel tbl 0x%x, device id %d, "
          "rc 0x%x",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          rc);
      goto cleanup;
    }
  }

  sel_tbl_info->max_grps = max_grps;

  /* Add to the hash tbl */
  unsigned long key = tbl_hdl;
  bf_map_sts_t msts;

  msts = pipe_mgr_sel_tbl_map_add(dev_id, key, (void *)sel_tbl_info, is_backup);

  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding sel table sts %d",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              msts);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  /* Adding to hash table should be at the end. Else, need to
   * handle removal in cleanup
   */

  return PIPE_SUCCESS;

cleanup:

  pipe_mgr_sel_tbl_info_destroy(sel_tbl_info, is_backup);

  return rc;
}

/** \brief pipe_mgr_sel_tbl_create
 *         Create the local structures to hold data about a new sel tbl
 *
 * This routine creates the local data structures needed for sel tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the sel tbl
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_sel_tbl_create(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl,
                                      profile_id_t profile_id,
                                      pipe_bitmap_t *pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_select_tbl_info_t *select_tbl_rmt_info = NULL;

  select_tbl_rmt_info = pipe_mgr_get_select_tbl_info(dev_id, tbl_hdl);
  if (select_tbl_rmt_info == NULL) {
    LOG_ERROR("%s:%d Select table 0x%x not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (select_tbl_rmt_info->max_groups == 0) {
    LOG_ERROR("%s: Request to create Select tbl 0x%x with 0 members",
              __func__,
              tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false)) {
    LOG_ERROR(
        "%s:%d Attempt to initialize Sel table 0x%x on device %d which already "
        "exists",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  rc = pipe_mgr_sel_tbl_create_internal(sess_hdl,
                                        dev_id,
                                        tbl_hdl,
                                        select_tbl_rmt_info,
                                        false,
                                        profile_id,
                                        pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error creating sel table for 0x%x, %s",
              __func__,
              __LINE__,
              tbl_hdl,
              pipe_str_err(rc));
    return rc;
  }

  rc = pipe_mgr_sel_tbl_create_internal(sess_hdl,
                                        dev_id,
                                        tbl_hdl,
                                        select_tbl_rmt_info,
                                        true,
                                        profile_id,
                                        pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error creating backup sel table for 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return rc;
  }

  LOG_TRACE("%s: Successfully created sel tbl 0x%x", __func__, tbl_hdl);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_tbl_delete_internal(
    bf_dev_id_t dev_id, pipe_sel_tbl_hdl_t tbl_hdl, bool is_backup) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  unsigned long key = tbl_hdl;
  bf_map_sts_t msts;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (sel_tbl_info == NULL) {
    LOG_TRACE("%s:%d Selection table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_sel_tbl_info_destroy(sel_tbl_info, is_backup);

  /* Remove from the hash table */
  msts = pipe_mgr_sel_tbl_map_rmv(dev_id, key, is_backup);

  if (msts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error deleting sel table sts %d", __func__, __LINE__, msts);
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_sel_tbl_delete
 *         Deletes the local structures that hold data about a sel tbl
 *
 * This routine deletes the local data structures needed for sel tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the sel tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_sel_tbl_delete(bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_sel_tbl_delete_internal(dev_id, tbl_hdl, false);
  if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
    LOG_ERROR("%s:%d Error deleting table tbl_hdl 0x%x dev_id %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id,
              rc);
    return rc;
  }

  rc = pipe_mgr_sel_tbl_delete_internal(dev_id, tbl_hdl, true);
  if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
    LOG_ERROR("%s:%d Error deleting table tbl_hdl 0x%x dev_id %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id,
              rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static uint32_t get_logical_index_by_word_idx(uint32_t word_idx,
                                              uint32_t mbr_idx) {
  uint32_t logical_index = word_idx * SEL_GRP_WORD_WIDTH;
  logical_index += mbr_idx;
  return logical_index;
}

void pipe_mgr_sel_tbl_get_phy_addr(rmt_mem_pack_format_t pack_format,
                                   uint32_t stage_logical_idx,
                                   uint32_t *subword_no,
                                   uint32_t *line_no,
                                   uint32_t *block,
                                   uint32_t ram_depth) {
  uint32_t virt_word_no = 0;

  /* Based on the index, return the tbl_blk */
  virt_word_no = stage_logical_idx / pack_format.entries_per_tbl_word;

  if (subword_no) {
    *subword_no = stage_logical_idx % pack_format.entries_per_tbl_word;
  }

  if (block) {
    *block = virt_word_no / ram_depth;
  }
  if (line_no) {
    *line_no = virt_word_no % ram_depth;
  }
}

static pipe_status_t pipe_mgr_sel_grp_post_instruction(
    sel_tbl_t *sel_tbl,
    uint8_t stage_id,
    sel_tbl_stage_hw_info_t *stage_hw,
    uint32_t block,
    uint32_t line,
    uint8_t *data,
    uint32_t data_sz_bytes,
    bool shadow_only) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_instr_set_memdata_v_i_only_t instr;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_bitmap_t pipe_bmp;
  bf_dev_pipe_t pipe_id;

  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  if (sel_tbl->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = sel_tbl_info->lowest_pipe_id;
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(sel_tbl->sel_tbl_info->pipe_bmp));
  } else {
    pipe_id = sel_tbl->pipe_id;
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(sel_tbl->inst_pipe_bmp));
  }

  rc = pipe_mgr_phy_mem_map_write(sel_tbl_info->dev_id,
                                  sel_tbl_info->direction,
                                  pipe_id,
                                  stage_id,
                                  pipe_mem_type_unit_ram,
                                  stage_hw->tbl_blk[block].mem_id[0],
                                  line,
                                  data,
                                  NULL);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error updating shadow memory stage %d mem-id %d line %d rc %s",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        stage_id,
        stage_hw->tbl_blk[block].mem_id[0],
        line,
        pipe_str_err(rc));
    return rc;
  }

  if (!shadow_only) {
    /* Based on the line and block, form the vpn address */
    uint32_t index = (stage_hw->tbl_blk[block].vpn_id[0] << log2_uint32_ceil(
                          pipe_mgr_get_sram_unit_depth(sel_tbl_info->dev_id))) |
                     line;
    uint32_t no_subword_bits = 0;
    uint32_t no_huffman_bits = TOF_SEL_SUBWORD_VPN_BITS - no_subword_bits;
    uint32_t huffman_bits = (1 << (no_huffman_bits - 1)) - 1;
    uint32_t virt_addr = (index << no_huffman_bits) | huffman_bits;

    construct_instr_set_v_memdata_no_data(sel_tbl_info->dev_id,
                                          &instr,
                                          data_sz_bytes,
                                          stage_hw->tbl_id,
                                          pipe_virt_mem_type_sel_stful,
                                          virt_addr);

    rc = pipe_mgr_drv_ilist_add_2(&sel_tbl->cur_sess_hdl,
                                  sel_tbl_info->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_set_memdata_v_i_only_t),
                                  data,
                                  data_sz_bytes);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_update_hw(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    uint32_t word_idx,
    bool shadow_only) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t pv_subword = 0, pv_line = 0, pv_block = 0;

  pipe_mgr_sel_tbl_get_phy_addr(stage_info->pv_hw.pack_format,
                                word_idx,
                                &pv_subword,
                                &pv_line,
                                &pv_block,
                                TOF_SRAM_UNIT_DEPTH);

  if (SEL_TBL_IS_FAIR(sel_tbl->sel_tbl_info)) {
    SET_PVL_USAGE(&stage_info->llp.llp_word[word_idx],
                  stage_info->llp.llp_word[word_idx].no_bits_set);
  } else if (SEL_TBL_IS_RESILIENT(sel_tbl->sel_tbl_info)) {
    SET_PVL_USAGE(&stage_info->llp.llp_word[word_idx],
                  stage_info->llp.llp_word[word_idx].highest_mbr_idx + 1);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  uint32_t data_sz_bytes = 0;
  data_sz_bytes = ((stage_info->ram_word_width - 1) >> 3) + 1;
  rc =
      pipe_mgr_sel_grp_post_instruction(sel_tbl,
                                        stage_info->stage_id,
                                        &stage_info->pv_hw,
                                        pv_block,
                                        pv_line,
                                        stage_info->llp.llp_word[word_idx].data,
                                        data_sz_bytes,
                                        shadow_only);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_word_data_allocate(sel_hlp_word_data_t *sel_word,
                                                  uint32_t word_width,
                                                  uint32_t ram_word_width) {
  if (sel_word->usage) {
    LOG_ERROR("%s:%d Usage is not set to 0", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  sel_word->word_width = word_width;

  PIPE_MGR_MEMSET(
      sel_word->mbrs, 0, sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);

  return PIPE_SUCCESS;
}

void pipe_mgr_sel_grp_word_data_deallocate(sel_hlp_word_data_t *sel_word,
                                           uint32_t ram_word_width) {
  if (!sel_word) {
    PIPE_MGR_DBGCHK(0);
    return;
  }

  sel_word->word_width = 0;
  sel_word->usage = 0;
  PIPE_MGR_MEMSET(
      sel_word->mbrs, 0, sizeof(pipe_sel_grp_mbr_hdl_t) * ram_word_width);
}

static uint32_t pipe_mgr_sel_base_idx_to_addr(sel_tbl_stage_info_t *stage_info,
                                              uint32_t sel_base_idx) {
  uint32_t addr = 0;
  uint32_t pv_block = 0;
  uint32_t huffman = 0;
  vpn_id_t vpn = 0;

  pipe_mgr_sel_tbl_get_phy_addr(stage_info->pv_hw.pack_format,
                                sel_base_idx,
                                NULL,
                                NULL,
                                &pv_block,
                                TOF_SRAM_UNIT_DEPTH);

  vpn = stage_info->pv_hw.tbl_blk[pv_block].vpn_id[0];

  addr = (vpn << log2_uint32_ceil(TOF_SRAM_UNIT_DEPTH)) |
         (sel_base_idx & (TOF_SRAM_UNIT_DEPTH - 1));

  /* Or in the huffman bits */
  huffman = 0;
  addr = (addr << TOF_SELECTOR_HUFFMAN_BITS) | huffman;
  return addr;
}

pipe_status_t pipe_mgr_sel_logical_idx_to_vaddr(bf_dev_id_t dev_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                int stage,
                                                uint32_t logical_idx,
                                                rmt_virt_addr_t *vaddr) {
  /* Lookup the table by device id and table handle. */
  sel_tbl_info_t *sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_INVALID_ARG;
  }
  /* Look up the stage info.  Just use the first entry in the sel_tbl array
   * since exactly which pipe table we look at doesn't matter, the index to
   * address conversion is the same for all of them. */
  int i, I = sel_tbl_info->sel_tbl[0].num_stages;
  for (i = 0; i < I; ++i) {
    if (stage == sel_tbl_info->sel_tbl[0].sel_tbl_stage_info[i].stage_id) {
      /* Use the stage info to convert the index to address. */
      sel_tbl_stage_info_t *stage_info =
          &sel_tbl_info->sel_tbl[0].sel_tbl_stage_info[i];
      *vaddr = pipe_mgr_sel_base_idx_to_addr(stage_info, logical_idx);
      return PIPE_SUCCESS;
    }
  }
  LOG_ERROR("%s:%d sel table stage info for handle 0x%x stage %d not found",
            __func__,
            __LINE__,
            sel_tbl_hdl,
            stage);
  return PIPE_INVALID_ARG;
}

static uint32_t pipe_mgr_sel_addr_to_base_idx(sel_tbl_stage_info_t *stage_info,
                                              rmt_virt_addr_t addr) {
  uint32_t vpn = 0, line_no = 0, virt_word_no = 0;
  uint32_t logical_idx = 0;
  uint32_t i = 0;

  addr >>= TOF_SELECTOR_HUFFMAN_BITS;

  vpn = (addr >> log2_uint32_ceil(TOF_SRAM_UNIT_DEPTH));
  line_no = (addr & (TOF_SRAM_UNIT_DEPTH - 1));

  for (i = 0; i < stage_info->pv_hw.num_blks; i++) {
    if (vpn == stage_info->pv_hw.tbl_blk[i].vpn_id[0]) {
      virt_word_no = i * TOF_SRAM_UNIT_DEPTH + line_no;
      break;
    }
  }

  logical_idx =
      virt_word_no * stage_info->pv_hw.pack_format.entries_per_tbl_word;
  return logical_idx;
}

pipe_status_t pipe_mgr_sel_vaddr_to_logical_idx(bf_dev_id_t dev_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                int stage,
                                                rmt_virt_addr_t vaddr,
                                                uint32_t *logical_idx) {
  /* Lookup the table by device id and table handle. */
  sel_tbl_info_t *sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_INVALID_ARG;
  }
  /* Look up the stage info.  Just use the first entry in the sel_tbl array
   * since exactly which pipe table we look at doesn't matter, the addr to
   * index conversion is the same for all of them. */
  int i, I = sel_tbl_info->sel_tbl[0].num_stages;
  for (i = 0; i < I; ++i) {
    if (stage == sel_tbl_info->sel_tbl[0].sel_tbl_stage_info[i].stage_id) {
      /* Use the stage info to convert the index to address. */
      sel_tbl_stage_info_t *stage_info =
          &sel_tbl_info->sel_tbl[0].sel_tbl_stage_info[i];
      *logical_idx = pipe_mgr_sel_addr_to_base_idx(stage_info, vaddr);
      return PIPE_SUCCESS;
    }
  }
  LOG_ERROR("%s:%d sel table stage info for handle 0x%x stage %d not found",
            __func__,
            __LINE__,
            sel_tbl_hdl,
            stage);
  return PIPE_INVALID_ARG;
}

void sel_stage_get_word_info(uint32_t grp_size,
                             uint32_t *no_words,
                             uint32_t *entries_per_word) {
  uint32_t log2;

  *no_words = ((grp_size - 1) / SEL_VEC_WIDTH) + 1;

  log2 = log2_uint32_ceil(*no_words);
  if (log2 > 5) {
    *no_words = (((*no_words - 1) >> (log2 - 5)) + 1) << (log2 - 5);
  }

  *entries_per_word = (grp_size - 1) / *no_words + 1;

  if ((*no_words > 1) && (*entries_per_word < 65)) {
    /* The power2 allocator has issues with allocating multiple words with
     * less than 65 size
     */
    *entries_per_word = 65;
  }

  return;
}

/*
 * Allocate a group in the hardware for the given size
 */
pipe_status_t pipe_mgr_sel_grp_allocate_in_stage(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    uint32_t grp_size,
    int sel_base_idx,
    pipe_adt_ent_idx_t adt_base_idx,
    sel_grp_stage_info_t **sel_grp_stage_info_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  uint32_t no_words = 0;
  uint32_t entries_per_word = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_hlp_word_data_t *sel_grp_word_data = NULL;

  bool adt_reserved = false;
  bool group_allocated = false;
  int word_allocated = -1;
  *sel_grp_stage_info_p = NULL;

  sel_stage_get_word_info(grp_size, &no_words, &entries_per_word);

  if (sel_tbl_info->adt_tbl_hdl) {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) "
        "Requesting ADT to reserve group with block %d entries per block %d "
        "pipe %x stage %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        no_words,
        entries_per_word,
        sel_tbl->pipe_id,
        sel_tbl_stage_info->stage_id);
    rc = rmt_adt_ent_group_reserve(sel_tbl_info->dev_id,
                                   sel_tbl_info->adt_tbl_hdl,
                                   sel_tbl->pipe_id,
                                   sel_tbl_stage_info->stage_id,
                                   &adt_base_idx,
                                   entries_per_word,
                                   no_words,
                                   sel_tbl->sess_flags);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to reserve adt group with block %d entries "
          "per block %d stage %d dev %d tbl_hdl %#x pipe %x rc %s",
          __func__,
          __LINE__,
          no_words,
          entries_per_word,
          sel_tbl_stage_info->stage_id,
          sel_tbl_info->dev_id,
          sel_tbl_info->tbl_hdl,
          sel_tbl->pipe_id,
          pipe_str_err(rc));
      goto cleanup;
    }
    adt_reserved = true;
  }

  if (sel_base_idx == PIPE_INVALID_SEL_GRP_IDX) {
    sel_base_idx = power2_allocator_alloc(
        sel_tbl_stage_info->stage_sel_grp_allocator, no_words);
    if (sel_base_idx == -1)
      rc = PIPE_NO_SPACE;
    else
      group_allocated = true;
  } else {
    if (power2_allocator_reserve(sel_tbl_stage_info->stage_sel_grp_allocator,
                                 sel_base_idx,
                                 no_words))
      rc = PIPE_NO_SPACE;
    else
      group_allocated = true;
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to allocate a selector group of block %d entries %d "
        "stage %d dev %d tbl_hdl 0x%x pipe %x",
        __func__,
        __LINE__,
        no_words,
        entries_per_word,
        sel_tbl_stage_info->stage_id,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        sel_tbl->pipe_id);
    goto cleanup;
  }

  sel_grp_stage_info = PIPE_MGR_MALLOC(sizeof(sel_grp_stage_info_t));
  if (sel_grp_stage_info == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  PIPE_MGR_MEMSET(sel_grp_stage_info, 0, sizeof(sel_grp_stage_info_t));
  sel_grp_stage_info->stage_p = sel_tbl_stage_info;
  sel_grp_stage_info->mbrs_duplicated = true;
  sel_grp_stage_info->no_words = no_words;
  sel_grp_stage_info->grp_size = grp_size;
  sel_grp_stage_info->entries_per_word = entries_per_word;

  sel_grp_stage_info->sel_base_addr =
      pipe_mgr_sel_base_idx_to_addr(sel_tbl_stage_info, sel_base_idx);
  sel_grp_stage_info->sel_base_idx = sel_base_idx;
  sel_grp_stage_info->adt_base_idx = adt_base_idx;

  sel_grp_stage_info->sel_grp_word_data =
      &sel_tbl_stage_info->hlp.hlp_word[sel_base_idx];

  for (uint32_t i = 0; i < no_words; i++) {
    sel_grp_word_data = &sel_grp_stage_info->sel_grp_word_data[i];
    sel_grp_word_data->adt_base_idx = adt_base_idx + (SEL_GRP_WORD_WIDTH * i);

    rc =
        pipe_mgr_sel_grp_word_data_allocate(sel_grp_word_data,
                                            entries_per_word,
                                            sel_tbl_stage_info->ram_word_width);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error allocating the word data for selector group "
          "words %d entries per word %d stage %d dev %d tbl_hdl %#x pipe %x rc "
          "%s",
          __func__,
          __LINE__,
          no_words,
          entries_per_word,
          sel_tbl_stage_info->stage_id,
          sel_tbl_info->dev_id,
          sel_tbl_info->tbl_hdl,
          sel_tbl->pipe_id,
          pipe_str_err(rc));
      goto cleanup;
    }
    word_allocated = i;
  }

  /* Add it to the sel_tbl_stage_info_t->sel_grp_stage_list */
  BF_LIST_DLL_AP(sel_tbl_stage_info->sel_grp_stage_free_list,
                 sel_grp_stage_info,
                 next,
                 prev);

  *sel_grp_stage_info_p = sel_grp_stage_info;
  return rc;

cleanup:
  if (adt_reserved) {
    /* Clean up the action table reservation. */
    rmt_adt_ent_group_delete(sel_tbl_info->dev_id,
                             sel_tbl_info->adt_tbl_hdl,
                             sel_tbl->pipe_id,
                             sel_tbl_stage_info->stage_id,
                             adt_base_idx,
                             entries_per_word,
                             no_words,
                             sel_tbl->sess_flags);
  }
  if (group_allocated) {
    power2_allocator_release(sel_tbl_stage_info->stage_sel_grp_allocator,
                             sel_base_idx);
  }
  if (sel_grp_stage_info) {
    for (int i = word_allocated; i >= 0; --i) {
      sel_grp_word_data = &sel_grp_stage_info->sel_grp_word_data[i];
      pipe_mgr_sel_grp_word_data_deallocate(sel_grp_word_data,
                                            sel_tbl_stage_info->ram_word_width);
    }
    PIPE_MGR_FREE(sel_grp_stage_info);
  }
  *sel_grp_stage_info_p = NULL;
  return rc;
}

static pipe_status_t pipe_mgr_sel_tbl_profile_set_internal(
    sel_tbl_t *sel_tbl, pipe_sel_tbl_profile_t *sel_tbl_profile) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_stage_info_t *sel_tbl_stage_info = NULL;
  uint32_t stage = 0, i = 0, j = 0;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;

  if (sel_tbl->is_profile_set) {
    LOG_ERROR(
        "%s:%d Error - duplicate request to set selector group profile "
        "on dev %d tbl_hdl 0x%x pipe %x",
        __func__,
        __LINE__,
        sel_tbl->sel_tbl_info->dev_id,
        sel_tbl->sel_tbl_info->tbl_hdl,
        sel_tbl->pipe_id);
    return PIPE_ALREADY_EXISTS;
  }

  /* Create the Group in all the stages */
  for (stage = 0; stage < sel_tbl->num_stages; stage++) {
    sel_tbl_stage_info = &sel_tbl->sel_tbl_stage_info[stage];
    for (i = 0; i < sel_tbl_profile->num_grp_profiles; i++) {
      for (j = 0; j < sel_tbl_profile->grp_profile_list[i].num_grps; j++) {
        rc = pipe_mgr_sel_grp_allocate_in_stage(
            sel_tbl,
            sel_tbl_stage_info,
            sel_tbl_profile->grp_profile_list[i].grp_size,
            PIPE_INVALID_SEL_GRP_IDX,
            PIPE_MGR_LOGICAL_ACT_IDX_INVALID,
            &sel_grp_stage_info);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error allocating grp with size %d "
              "stage %d dev %d tbl_hdl 0x%x pipe %x",
              __func__,
              __LINE__,
              sel_tbl_profile->grp_profile_list[i].grp_size,
              sel_tbl_stage_info->stage_id,
              sel_tbl->sel_tbl_info->dev_id,
              sel_tbl->sel_tbl_info->tbl_hdl,
              sel_tbl->pipe_id);
          goto cleanup;
        }

        sel_grp_stage_info->isstatic = true;
      }
    }
  }

  sel_tbl->is_profile_set = true;

  return PIPE_SUCCESS;
cleanup:
  return rc;
}

/*!
 * API function to set the group profile for a selection table
 */
pipe_status_t rmt_sel_tbl_profile_set(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                      pipe_sel_tbl_profile_t *sel_tbl_profile) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  bf_dev_pipe_t pipe_id = 0;
  sel_tbl_t *sel_tbl;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, dev_tgt.dev_pipe_id);
  if (sel_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Selector table with pipe_id %d not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = 0;

  if ((SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) &&
      (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install an asymmetric entry"
        " in a symmetric table 0x%x device %d",
        __func__,
        __LINE__,
        sel_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Sort a copy of the profile so as to avoid changing the caller's data. */
  pipe_sel_tbl_profile_t x = *sel_tbl_profile;
  x.grp_profile_list =
      PIPE_MGR_MALLOC(x.num_grp_profiles * sizeof(pipe_sel_grp_profile_t));
  if (!x.grp_profile_list) return PIPE_NO_SYS_RESOURCES;
  PIPE_MGR_MEMCPY(x.grp_profile_list,
                  sel_tbl_profile->grp_profile_list,
                  x.num_grp_profiles * sizeof(pipe_sel_grp_profile_t));

  qsort(x.grp_profile_list,
        x.num_grp_profiles,
        sizeof(pipe_sel_grp_profile_t),
        sel_grp_profile_sort_func);

  char buf[1000];
  char *end = buf + sizeof(buf);
  char *ptr = buf;
  PIPE_MGR_MEMSET(ptr, 0, end - ptr);

  uint32_t p;
  for (p = 0; p < x.num_grp_profiles; p++) {
    pipe_sel_grp_profile_t *profile = &x.grp_profile_list[p];
    ptr += snprintf(ptr,
                    end > ptr ? (end - ptr) : 0,
                    "Num grps %4d : size %4d\n",
                    profile->num_grps,
                    profile->grp_size);
  }
  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Request to set profile\n %s",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      buf);

  rc = pipe_mgr_sel_tbl_profile_set_internal(sel_tbl, &x);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Selector table profile set failed for dev %d "
        "tbl_hdl 0x%x pipe %x rc 0x%x",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        sel_tbl_hdl,
        pipe_id,
        rc);
    return rc;
  }

  PIPE_MGR_FREE(x.grp_profile_list);

  return rc;
}

pipe_status_t pipe_mgr_sel_grp_update_grp_hw_locator_list(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    sel_grp_stage_info_t *sel_grp_stage_info) {
  PWord_t Ppipe, Pstage;

  JLI(Ppipe, sel_grp_info->sel_grp_pipe_lookup, (Word_t)sel_tbl->pipe_idx);
  if (Ppipe == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  JLI(Pstage, *(Pvoid_t *)Ppipe, (Word_t)sel_tbl_stage_info->stage_idx);
  if (Pstage == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (*Pstage) {
    LOG_ERROR(
        "%s:%d %s(0x%x %d) Selector group %d already exists on pipe %x"
        "stage %d",
        __func__,
        __LINE__,
        sel_tbl->sel_tbl_info->name,
        sel_tbl->sel_tbl_info->tbl_hdl,
        sel_tbl->sel_tbl_info->dev_id,
        sel_grp_info->grp_hdl,
        sel_tbl->pipe_id,
        sel_tbl_stage_info->stage_id);
    return PIPE_ALREADY_EXISTS;
  }

  *Pstage = (Word_t)sel_grp_stage_info;
  return PIPE_SUCCESS;
}

sel_grp_stage_info_t *pipe_mgr_sel_grp_stage_info_get(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_tbl_stage_info_t *sel_tbl_stage_info) {
  PWord_t Ppipe, Pstage;

  JLI(Ppipe, sel_grp_info->sel_grp_pipe_lookup, (Word_t)sel_tbl->pipe_idx);
  if (!Ppipe) {
    return NULL;
  }

  JLI(Pstage, *(Pvoid_t *)Ppipe, (Word_t)sel_tbl_stage_info->stage_idx);
  if (!Pstage) {
    return NULL;
  }

  return (sel_grp_stage_info_t *)*Pstage;
}

/*
 * Get the number of duplicate entries in stage
 */

static pipe_status_t pipe_mgr_sel_grp_mbrs_count(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t *num_mbrs) {
  PWord_t Pmbr;
  sel_mbr_offset_t *mbr_offset = NULL;
  uint32_t count = 0;

  if (!sel_tbl || !sel_grp_stage_info) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  JLG(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);
  if (!Pmbr) {
    sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
    LOG_ERROR(
        "%s:%d %s(0x%x %d.%x) Sel grp member 0x%x not found in stage %d in grp "
        "0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_stage_info->stage_p->stage_id,
        sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (Pmbr == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pmbr;

  while (mbr_offset) {
    mbr_offset = mbr_offset->next;
    count++;
  }

  *num_mbrs = count;
  return PIPE_SUCCESS;
}

/*
 * Function to get the word and mbr index when more than one member
 * is present with same index (wcmp)
 */

static pipe_status_t pipe_mgr_sel_grp_mbr_get_offset_at_pos(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t pos,
    uint32_t *word_idx,
    uint32_t *mbr_idx) {
  PWord_t Pmbr;
  uint32_t offset;
  sel_mbr_offset_t *mbr_offset = NULL;

  if (!sel_tbl || !sel_grp_stage_info || !word_idx || !mbr_idx) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  JLG(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);
  if (!Pmbr) {
    sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
    LOG_ERROR(
        "%s:%d %s(0x%x %d.%x) Sel grp member 0x%x not found in stage %d in grp "
        "0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_stage_info->stage_p->stage_id,
        sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (Pmbr == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pmbr;

  while (mbr_offset->next && pos) {
    mbr_offset = mbr_offset->next;
    pos--;
  }

  if (pos) {
    sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
    LOG_ERROR(
        "%s:%d %s(0x%x %d.%x) Sel grp member 0x%x not found in stage %d in grp "
        "0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_stage_info->stage_p->stage_id,
        sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  offset = mbr_offset->offset;
  *word_idx = offset / SEL_GRP_WORD_WIDTH;
  *mbr_idx = offset % SEL_GRP_WORD_WIDTH;

  return PIPE_SUCCESS;
}

/*
 * Funtion to get the selector table offset of the member handle, in case of
 * duplicate entry (wcmp) function will return the last offset
 */

static pipe_status_t pipe_mgr_sel_grp_mbr_get_offset(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t *word_idx,
    uint32_t *mbr_idx) {
  PWord_t Pmbr;
  uint32_t offset;
  sel_mbr_offset_t *mbr_offset = NULL;

  if (!sel_tbl || !sel_grp_stage_info || !word_idx || !mbr_idx) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  JLG(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);
  if (!Pmbr) {
    sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
    LOG_ERROR(
        "%s:%d %s(0x%x %d.%x) Sel grp member 0x%x not found in stage %d in grp "
        "0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_stage_info->stage_p->stage_id,
        sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (Pmbr == PJERR) {
    sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
    LOG_ERROR(
        "%s:%d %s(0x%x %d.%x) Sel grp member 0x%x not found in stage %d in grp "
        "0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_stage_info->stage_p->stage_id,
        sel_grp_stage_info->grp_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pmbr;

  while (mbr_offset->next) {
    mbr_offset = mbr_offset->next;
  }

  offset = mbr_offset->offset;
  *word_idx = offset / SEL_GRP_WORD_WIDTH;
  *mbr_idx = offset % SEL_GRP_WORD_WIDTH;

  return PIPE_SUCCESS;
}

/*
 * In case of duplicate selector table entry, save the offset in sorted list
 */

pipe_status_t pipe_mgr_sel_grp_mbr_hw_locator_update(
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t mbr_idx) {
  PWord_t Pmbr;
  uint32_t offset;
  sel_mbr_offset_t *mbr_offset = NULL;
  sel_mbr_offset_t *tmp = NULL;

  offset = pipe_mgr_sel_grp_get_mbr_offset(word_idx, mbr_idx);

  mbr_offset = pipe_mgr_sel_mbr_offset_alloc(offset);
  if (!mbr_offset) {
    return PIPE_NO_SYS_RESOURCES;
  }

  JLI(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);
  if (Pmbr == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    pipe_mgr_sel_mbr_offset_destroy(mbr_offset);
    return PIPE_NO_SYS_RESOURCES;
  }

  tmp = (sel_mbr_offset_t *)*Pmbr;

  /* Keeping the offsets in sorted list */
  if (!tmp || mbr_offset->offset < tmp->offset) {
    mbr_offset->next = tmp;
    *Pmbr = (Word_t)mbr_offset;
    return PIPE_SUCCESS;
  }

  while (tmp->next != NULL && tmp->next->offset < mbr_offset->offset)
    tmp = tmp->next;

  mbr_offset->next = tmp->next;
  tmp->next = mbr_offset;

  return PIPE_SUCCESS;
}

/*
 * When the member is moved to different position in selector table update
 * new offset
 */

static pipe_status_t pipe_mgr_sel_grp_mbr_hw_locator_update_pos(
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t tgt_idx,
    uint32_t src_idx,
    uint32_t src_word_idx) {
  PWord_t Pmbr;
  uint32_t src_offset, tgt_offset;
  sel_mbr_offset_t *mbr_offset = NULL;
  sel_mbr_offset_t *cur = NULL, *prev = NULL;

  tgt_offset = pipe_mgr_sel_grp_get_mbr_offset(word_idx, tgt_idx);
  src_offset = pipe_mgr_sel_grp_get_mbr_offset(src_word_idx, src_idx);

  JLG(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);

  if (Pmbr == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (!Pmbr) {
    LOG_ERROR("%s:%d Sel grp member %d not found in stage %d in grp 0x%x",
              __func__,
              __LINE__,
              mbr_hdl,
              sel_grp_stage_info->stage_p->stage_id,
              sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pmbr;

  /* when there is no duplicate member handler in selector group */
  if (!mbr_offset->next) {
    mbr_offset->offset = tgt_offset;
    return PIPE_SUCCESS;
  }

  /* In case of duplicate member, remove member with src_offset and update with
   * tgt_offset and make sure list is sorted
   */

  if (mbr_offset->offset == src_offset) {
    *Pmbr = (Word_t)mbr_offset->next;
  } else {
    while (mbr_offset != NULL && mbr_offset->offset != src_offset) {
      prev = mbr_offset;
      mbr_offset = mbr_offset->next;
    }
    if (mbr_offset == NULL) {
      LOG_ERROR("%s:%d src idx %d is not found for mbr %d hdl",
                __func__,
                __LINE__,
                src_idx,
                mbr_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    prev->next = mbr_offset->next;
  }

  /* Update the new offset */
  mbr_offset->offset = tgt_offset;

  cur = (sel_mbr_offset_t *)*Pmbr;

  /* Keeping the offsets in sorted list */
  if (mbr_offset->offset < cur->offset) {
    mbr_offset->next = cur;
    *Pmbr = (Word_t)mbr_offset;
    return PIPE_SUCCESS;
  }

  while (cur->next != NULL && cur->next->offset < mbr_offset->offset)
    cur = cur->next;

  mbr_offset->next = cur->next;
  cur->next = mbr_offset;

  return PIPE_SUCCESS;
}

/*
 * This function handle special case when converging the selector table, in case
 * of duplicate entries, need to move from correct table.
 */

static pipe_status_t pipe_mgr_sel_grp_mbr_hw_locator_update_word(
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t src_word_idx,
    uint32_t tgt_word_idx,
    uint32_t tgt_idx) {
  PWord_t Pmbr;
  uint32_t tgt_offset;
  sel_mbr_offset_t *mbr_offset = NULL;
  sel_mbr_offset_t *cur = NULL, *prev = NULL;

  tgt_offset = pipe_mgr_sel_grp_get_mbr_offset(tgt_word_idx, tgt_idx);

  JLG(Pmbr, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);

  if (Pmbr == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (!Pmbr) {
    LOG_ERROR("%s:%d Sel grp member %d not found in stage %d in grp 0x%x",
              __func__,
              __LINE__,
              mbr_hdl,
              sel_grp_stage_info->stage_p->stage_id,
              sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pmbr;

  /* when there is no duplicate member handler in selector group */
  if (!mbr_offset->next) {
    mbr_offset->offset = tgt_offset;
    return PIPE_SUCCESS;
  }

  /* In case of duplicate member, remove member with src_word_idx and update
   * with tgt_offset and make sure list is sorted
   */

  if ((mbr_offset->offset / SEL_GRP_WORD_WIDTH) == src_word_idx) {
    *Pmbr = (Word_t)mbr_offset->next;
  } else {
    while (mbr_offset != NULL &&
           ((mbr_offset->offset / SEL_GRP_WORD_WIDTH) != src_word_idx)) {
      prev = mbr_offset;
      mbr_offset = mbr_offset->next;
    }
    if (mbr_offset == NULL) {
      LOG_ERROR("%s:%d mbr %d hdl not found in src_word_idx %d",
                __func__,
                __LINE__,
                mbr_hdl,
                src_word_idx);
      return PIPE_OBJ_NOT_FOUND;
    }
    prev->next = mbr_offset->next;
  }

  /* Update the new offset */
  mbr_offset->offset = tgt_offset;

  cur = (sel_mbr_offset_t *)*Pmbr;

  /* Keeping the offsets in sorted list */
  if (mbr_offset->offset < cur->offset) {
    mbr_offset->next = cur;
    *Pmbr = (Word_t)mbr_offset;
    return PIPE_SUCCESS;
  }

  while (cur->next != NULL && cur->next->offset < mbr_offset->offset)
    cur = cur->next;

  mbr_offset->next = cur->next;
  cur->next = mbr_offset;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_hw_locator_remove(
    sel_grp_stage_info_t *sel_grp_stage_info, pipe_sel_grp_mbr_hdl_t mbr_hdl) {
  int Rc_int;
  sel_mbr_offset_t *mbr_offset = NULL;
  sel_mbr_offset_t *prev = NULL;
  PWord_t Pvalue;

  JLG(Pvalue, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);

  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (!Pvalue) {
    LOG_ERROR("%s:%d mbr 0x%x not found in group stage info for group 0x%x",
              __func__,
              __LINE__,
              mbr_hdl,
              sel_grp_stage_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  mbr_offset = (sel_mbr_offset_t *)*Pvalue;

  /* When there are duplicate entry for member handle */
  if (mbr_offset && mbr_offset->next) {
    prev = mbr_offset;
    mbr_offset = mbr_offset->next;
    while (mbr_offset->next != NULL) {
      prev = mbr_offset;
      mbr_offset = mbr_offset->next;
    }
    prev->next = NULL;
  } else {
    JLD(Rc_int, sel_grp_stage_info->mbr_locator, (Word_t)mbr_hdl);
    if (!Rc_int) {
      LOG_ERROR("%s:%d mbr 0x%x not found in group stage info for group 0x%x",
                __func__,
                __LINE__,
                mbr_hdl,
                sel_grp_stage_info->grp_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  pipe_mgr_sel_mbr_offset_destroy(mbr_offset);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_activate_stage_internal(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    sel_grp_stage_info_t **sel_grp_stage_info_p,
    int sel_base_idx,
    pipe_adt_ent_idx_t adt_base_idx) {
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t no_words = 0;
  bool from_profile = false;

  for (sel_grp_stage_info = sel_tbl_stage_info->sel_grp_stage_free_list;
       sel_grp_stage_info;
       sel_grp_stage_info = sel_grp_stage_info->next) {
    if (sel_grp_stage_info->grp_size == sel_grp_info->max_grp_size) {
      from_profile = true;
      break;
    }
  }

  if (!sel_grp_stage_info) {
    rc = pipe_mgr_sel_grp_allocate_in_stage(sel_tbl,
                                            sel_tbl_stage_info,
                                            sel_grp_info->max_grp_size,
                                            sel_base_idx,
                                            adt_base_idx,
                                            &sel_grp_stage_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to activate group 0x%x in stage %d",
                __func__,
                __LINE__,
                sel_grp_info->grp_hdl,
                sel_tbl_stage_info->stage_id);
      return rc;
    }
  }

  sel_grp_stage_info->inuse = true;
  sel_grp_stage_info->grp_hdl = sel_grp_info->grp_hdl;

  BF_LIST_DLL_REM(sel_tbl_stage_info->sel_grp_stage_free_list,
                  sel_grp_stage_info,
                  next,
                  prev);

  BF_LIST_DLL_AP(sel_tbl_stage_info->sel_grp_stage_inuse_list,
                 sel_grp_stage_info,
                 next,
                 prev);

  /* Update the hw_locator_list */
  rc = pipe_mgr_sel_grp_update_grp_hw_locator_list(
      sel_tbl, sel_grp_info, sel_tbl_stage_info, sel_grp_stage_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("Error updating hw_locator list for group %d in stage %d",
              sel_grp_info->grp_hdl,
              sel_tbl_stage_info->stage_id);
    goto cleanup;
  }

  no_words = sel_grp_stage_info->no_words;
  sel_base_idx = sel_grp_stage_info->sel_base_idx;
  adt_base_idx = sel_grp_stage_info->adt_base_idx;
  sel_grp_info->adt_offset = adt_base_idx;

  if (sel_grp_stage_info_p) {
    *sel_grp_stage_info_p = sel_grp_stage_info;
  }

  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Activated sel grp 0x%x at stage %d base idx 0x%x words %d from_profile "
      "%d adt_offset %d",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_grp_info->grp_hdl,
      sel_tbl_stage_info->stage_id,
      sel_base_idx,
      no_words,
      from_profile,
      adt_base_idx);

  return PIPE_SUCCESS;
cleanup:
  return rc;
}

static pipe_status_t pipe_mgr_sel_grp_build_move_list(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  /* Determine how many blocks of ADT memory have been reserved. */
  int num_indexes = 0;
  uint32_t i;
  for (i = 0; i < sel_tbl->num_stages; ++i) {
    sel_grp_stage_info_t *sel_grp_stage_info = NULL;
    sel_grp_stage_info = pipe_mgr_sel_grp_stage_info_get(
        sel_tbl, sel_grp_info, &sel_tbl->sel_tbl_stage_info[i]);
    if (!sel_grp_stage_info) {
      LOG_ERROR("%s:%d get sel group stage failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }
    num_indexes += sel_grp_stage_info->no_words;
  }
  /* Allocate memory to hold information about the ADT allocations. */
  struct pipe_multi_index *locations = NULL;
  locations = PIPE_MGR_MALLOC(num_indexes * sizeof(struct pipe_multi_index));
  if (!locations) {
    LOG_ERROR("%s:%d Cannot allocate location list new sel group",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Populate the location array.  Each word in each stage has its own entry
   * in the array. */
  int k = 0;
  for (i = 0; i < sel_tbl->num_stages; ++i) {
    unsigned int j;
    sel_grp_stage_info_t *sel_grp_stage_info = NULL;
    sel_grp_stage_info = pipe_mgr_sel_grp_stage_info_get(
        sel_tbl, sel_grp_info, &sel_tbl->sel_tbl_stage_info[i]);
    if (!sel_grp_stage_info) {
      LOG_ERROR("%s:%d get sel group stage failed", __func__, __LINE__);
      PIPE_MGR_FREE(locations);
      return PIPE_UNEXPECTED;
    }
    for (j = 0; j < sel_grp_stage_info->no_words; ++j) {
      locations[k].logical_index_base =
          sel_grp_stage_info->sel_grp_word_data[j].adt_base_idx;
      locations[k].logical_index_count =
          sel_grp_stage_info->sel_grp_word_data[j].word_width;
      ++k;
    }
  }

  *move_head_p =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_GROUP_CREATE, sel_tbl->pipe_id);
  if (!*move_head_p) {
    LOG_ERROR(
        "%s:%d Cannot allocate ML node for new sel group", __func__, __LINE__);
    PIPE_MGR_FREE(locations);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Get the logical index from the first pipe/stage. */
  Pvoid_t stage_lookup = (Pvoid_t)NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  uint32_t pipe_idx = 0, stage_idx = 0;
  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    (void)pipe_idx;
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      (void)stage_idx;
      (*move_head_p)->sel_grp_hdl = sel_grp_info->grp_hdl;
      (*move_head_p)->sel_grp_size = sel_grp_stage_info->no_words;
      (*move_head_p)->max_mbrs = sel_grp_info->max_grp_size;
      (*move_head_p)->logical_sel_index = sel_grp_stage_info->sel_base_idx;
      (*move_head_p)->locations_length = num_indexes;
      (*move_head_p)->locations = locations;
      break;
    }
    break;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_add_internal(
    sel_tbl_t *sel_tbl_inst,
    bf_dev_pipe_t pipe_id,
    pipe_sel_grp_id_t grp_id,
    uint32_t max_grp_size,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_idx_t adt_offset,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl_inst->sel_tbl_info;
  sel_grp_info_t *sel_grp_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  unsigned int j = 0;
#ifdef SEL_GROUP_ACTIVATE_ALL_STAGES
  bf_dev_pipe_t iter_pipe_id = 0;
  uint32_t i = 0;
  sel_tbl_t *sel_tbl = NULL, *last_valid_sel_tbl = NULL;
#endif
  uint32_t no_words = 0;
  uint32_t entries_per_word = 0;

  sel_grp_info = pipe_mgr_sel_grp_allocate();
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  sel_grp_info->pipe_id = pipe_id;
  sel_grp_info->grp_hdl = sel_grp_hdl;
  sel_grp_info->max_grp_size = max_grp_size;
  sel_grp_info->grp_id = grp_id;

  sel_stage_get_word_info(max_grp_size, &no_words, &entries_per_word);
  sel_grp_info->entries_per_word = entries_per_word;

/* In Phase-1, activate the group in all stages and pipes */
#ifdef SEL_GROUP_ACTIVATE_ALL_STAGES

  for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_info->sel_tbl[i];
    if ((sel_tbl->pipe_id != pipe_id) && (pipe_id != BF_DEV_PIPE_ALL)) {
      continue;
    }
    last_valid_sel_tbl = sel_tbl;

    for (j = 0; j < sel_tbl->num_stages; j++) {
      rc = pipe_mgr_sel_grp_activate_stage_internal(
          sel_tbl,
          sel_grp_info,
          &sel_tbl->sel_tbl_stage_info[j],
          NULL,
          PIPE_INVALID_SEL_GRP_IDX,
          adt_offset);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error activating grp 0x%x in stage %d for pipe %x rc %s",
            __func__,
            __LINE__,
            sel_grp_hdl,
            sel_tbl->sel_tbl_stage_info[j].stage_id,
            iter_pipe_id,
            pipe_str_err(rc));
        break;
      }
    }

    if (rc != PIPE_SUCCESS) {
      break;
    } else if (sel_tbl_info->cache_id) {
      void *data;
      /* In case of resize entry already exists in the map - nothing to do. */
      bf_map_sts_t sts =
          bf_map_get(&sel_tbl->grp_id_map, grp_id, (void **)&data);
      if (sts == BF_MAP_NO_KEY) {
        sts = bf_map_add(&sel_tbl->grp_id_map, grp_id, sel_grp_info);
      }
      if (BF_MAP_OK != sts) {
        LOG_ERROR("%s:%d Error adding grp %d (hdl 0x%x) to grp_id map rc %d",
                  __func__,
                  __LINE__,
                  grp_id,
                  sel_grp_hdl,
                  sts);
        rc = PIPE_UNEXPECTED;
        break;
      }
    }
  }
  if (!last_valid_sel_tbl) {
    PIPE_MGR_FREE(sel_grp_info);
    return PIPE_INVALID_ARG;
  }
  sel_tbl = last_valid_sel_tbl;

  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_FREE(sel_grp_info);
    return rc;
  }

#endif

  /* All the allocs are done, add myself to sel_grp_hdl_to_grp_htbl */

  rc = pipe_mgr_sel_grp_add_to_htbl(sel_tbl_inst, sel_grp_hdl, sel_grp_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error adding the grp %d to grp_hdl_htbl for pipe %x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              pipe_id);
    return rc;
  }

  if (move_head_p) {
    rc = pipe_mgr_sel_grp_build_move_list(sel_tbl, sel_grp_info, move_head_p);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

/*!
 * API function to add a new group into a selection table
 */
pipe_status_t rmt_sel_grp_add(pipe_sess_hdl_t sess_hdl,
                              dev_target_t dev_tgt,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              pipe_sel_grp_id_t grp_id,
                              uint32_t max_grp_size,
                              pipe_adt_ent_idx_t adt_offset,
                              pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                              uint32_t pipe_api_flags,
                              struct pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_sel_grp_hdl_t sel_grp_hdl = -1;
  bf_dev_pipe_t pipe_id = 0;
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (sel_tbl_info->cache_id == false) {
    /* Once turned on, cannot be turned off. */
    sel_tbl_info->cache_id =
        (pipe_api_flags & PIPE_MGR_TBL_API_CACHE_ENT_ID) ? true : false;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) Request to add group with size %d to pipe %x",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      max_grp_size,
      dev_tgt.dev_pipe_id);

  if (max_grp_size > sel_tbl_info->max_grp_size || max_grp_size < 1) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Maximum group size on this table is %d, requested %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->max_grp_size,
        max_grp_size);
    return PIPE_INVALID_ARG;
  }

  if (adt_offset != PIPE_ADT_ENT_IDX_INVALID_IDX && adt_offset % 2 != 0) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Odd ADT offset value is not supported, requested %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        adt_offset);
    return PIPE_INVALID_ARG;
  }

  if ((SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) &&
      (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install an asymmetric entry"
        " in a symmetric table 0x%x device %d",
        __func__,
        __LINE__,
        sel_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = BF_DEV_PIPE_ALL;
  } else {
    pipe_id = dev_tgt.dev_pipe_id;
  }
  sel_tbl_t *sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d get sel table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  sel_grp_info_t *sel_grp_info = NULL;
  /* Check if member id is used */
  if (sel_tbl_info->cache_id) {
    if (bf_map_get(&sel_tbl->grp_id_map, grp_id, (void **)&sel_grp_info) !=
        BF_MAP_NO_KEY) {
      LOG_ERROR("%s:%d grp_id %d already exists.", __func__, __LINE__, grp_id);
      return PIPE_ALREADY_EXISTS;
    }
  }

  sel_grp_hdl = pipe_mgr_sel_grp_hdl_allocate(sel_tbl, pipe_id);

  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    pipe_mgr_sel_grp_replay_info_t *grp_replay_info =
        PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_sel_grp_replay_info_t));
    if (!grp_replay_info) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    grp_replay_info->grp_hdl = sel_grp_hdl;
    grp_replay_info->grp_id = grp_id;
    grp_replay_info->max_grp_size = max_grp_size;
    grp_replay_info->mbr_hdls =
        PIPE_MGR_CALLOC(max_grp_size, sizeof(pipe_sel_grp_mbr_hdl_t));
    if (!grp_replay_info->mbr_hdls) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    grp_replay_info->mbr_enable =
        PIPE_MGR_CALLOC(max_grp_size, sizeof(pipe_sel_grp_mbr_hdl_t));
    if (!grp_replay_info->mbr_enable) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    grp_replay_info->mbr_weight =
        PIPE_MGR_CALLOC(max_grp_size, sizeof(uint32_t));
    if (!grp_replay_info->mbr_weight) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
    bf_map_add(
        &ha_hlp_info->replay_hdl_to_info, sel_grp_hdl, (void *)grp_replay_info);

    sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
    if (!sel_grp_info) {
      sel_grp_info = pipe_mgr_sel_grp_allocate();
      if (sel_grp_info == NULL) {
        LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }

      sel_grp_info->pipe_id = pipe_id;
      sel_grp_info->grp_hdl = sel_grp_hdl;
      sel_grp_info->grp_id = grp_id;
      sel_grp_info->max_grp_size = max_grp_size;
      if (sel_tbl_info->cache_id) {
        bf_map_sts_t sts =
            bf_map_add(&sel_tbl->grp_id_map, grp_id, sel_grp_info);
        if (BF_MAP_OK != sts) {
          LOG_ERROR("%s:%d Error adding grp %d (hdl 0x%x) to grp_id map rc %d",
                    __func__,
                    __LINE__,
                    grp_id,
                    sel_grp_hdl,
                    sts);
          rc = PIPE_UNEXPECTED;
        }
      }
      pipe_mgr_sel_grp_add_to_htbl(sel_tbl, sel_grp_hdl, sel_grp_info);
    }
  } else {
    rc = pipe_mgr_sel_grp_backup_one(sel_tbl, sel_grp_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error backing up group %d rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_hdl,
                rc);
      goto cleanup;
    }

    rc = pipe_mgr_sel_grp_add_internal(sel_tbl,
                                       pipe_id,
                                       grp_id,
                                       max_grp_size,
                                       sel_grp_hdl,
                                       adt_offset,
                                       move_head_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Selector table profile set failed for dev %d "
          "tbl_hdl 0x%x rc 0x%x",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          sel_tbl_hdl,
          rc);
      goto cleanup;
    }
  }

  *sel_grp_hdl_p = sel_grp_hdl;

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Successfully added group 0x%x with size %d to pipe %x",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_grp_hdl,
      max_grp_size,
      dev_tgt.dev_pipe_id);

  PIPE_MGR_SEL_TBL_ASSERT(dev_tgt.device_id, sel_tbl_hdl);
  return PIPE_SUCCESS;

cleanup:
  return rc;
}

static pipe_status_t pipe_mgr_sel_grp_deallocate_in_stage(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    sel_grp_stage_info_t *grp_stage_info,
    bool backup_restore_mode,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t no_words = 0;
  sel_hlp_word_data_t *word_data = NULL;
  uint32_t sel_base_idx = 0;
  uint32_t i = 0, j = 0;
  Word_t Rc_word;
  pipe_adt_ent_hdl_t mbr_hdl;
  pipe_sel_grp_mbr_hdl_t sel_hdl = 0;
  sel_mbr_offset_t *mbr_offset = NULL;
  (void)sel_hdl;

  no_words = grp_stage_info->no_words;
  sel_base_idx = grp_stage_info->sel_base_idx;

  for (i = 0; i < no_words; i++) {
    word_data = &grp_stage_info->sel_grp_word_data[i];

    for (j = 0; j < word_data->usage; j++) {
      mbr_hdl = word_data->mbrs[j];
      rc = issue_callback(sel_tbl,
                          grp_stage_info,
                          mbr_hdl,
                          i,
                          j,
                          PIPE_SEL_UPDATE_DEL,
                          false,
                          move_tail_p);
      if (rc != PIPE_SUCCESS) return rc;
    }

    pipe_mgr_sel_grp_word_data_deallocate(word_data,
                                          stage_info->ram_word_width);
  }

  if (!backup_restore_mode) {
    /* These modules do their own backup. So no need to clear them up */

    if (!grp_stage_info->isstatic) {
      power2_allocator_release(stage_info->stage_sel_grp_allocator,
                               sel_base_idx);

      sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
      if (sel_tbl_info->adt_tbl_hdl) {
        LOG_TRACE(
            "%s:%d - %s (%d - 0x%x) "
            "Requesting ADT to delete group with base 0x%x pipe %x stage %d",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->dev_id,
            sel_tbl_info->tbl_hdl,
            grp_stage_info->sel_grp_word_data[0].adt_base_idx,
            sel_tbl->pipe_id,
            stage_info->stage_id);
        rc = rmt_adt_ent_group_delete(
            sel_tbl_info->dev_id,
            sel_tbl_info->adt_tbl_hdl,
            sel_tbl->pipe_id,
            stage_info->stage_id,
            grp_stage_info->sel_grp_word_data[0].adt_base_idx,
            grp_stage_info->entries_per_word,
            grp_stage_info->no_words,
            sel_tbl->sess_flags);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s(0x%x - %d) Error deleting adt entry group for selector "
              "group stage %d rc 0x%x",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              stage_info->stage_id,
              rc);
          return rc;
        }
        for (i = 0; i < no_words; i++) {
          word_data = &grp_stage_info->sel_grp_word_data[i];
          word_data->adt_base_idx = 0;
        }
      }
    }
  }

  JUDYL_FOREACH(
      grp_stage_info->mbr_locator, sel_hdl, sel_mbr_offset_t *, mbr_offset) {
    pipe_mgr_sel_mbr_offset_destroy_all(mbr_offset);
  }
  // Remove the member locator
  JLFA(Rc_word, grp_stage_info->mbr_locator);
  (void)Rc_word;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_deactivate_stage_internal(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *sel_tbl_stage_info,
    sel_grp_stage_info_t *sel_grp_stage_info,
    bool backup_restore_mode,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_sel_grp_deallocate_in_stage(sel_tbl,
                                            sel_tbl_stage_info,
                                            sel_grp_stage_info,
                                            backup_restore_mode,
                                            move_tail_p);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error deallocating group in stage %d",
              __func__,
              __LINE__,
              sel_tbl_stage_info->stage_id);
    return rc;
  }

  sel_grp_stage_info->inuse = false;

  BF_LIST_DLL_REM(sel_tbl_stage_info->sel_grp_stage_inuse_list,
                  sel_grp_stage_info,
                  next,
                  prev);

  if (!sel_grp_stage_info->isstatic) {
    pipe_mgr_sel_grp_stage_info_destroy(sel_grp_stage_info);
    sel_grp_stage_info = NULL;
  } else {
    BF_LIST_DLL_AP(sel_tbl_stage_info->sel_grp_stage_free_list,
                   sel_grp_stage_info,
                   next,
                   prev);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_del_internal(
    sel_tbl_t *sel_tbl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    bool backup_restore_mode,
    bool skip_cache_id,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_grp_info_t *sel_grp_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  (void)stage_idx;

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sel_move_list_t *grp_del = alloc_sel_move_list(
      NULL, PIPE_SEL_UPDATE_GROUP_DESTROY, sel_grp_info->pipe_id);
  if (!grp_del) {
    LOG_ERROR(
        "%s:%d Delete of group %#x fails", __func__, __LINE__, sel_grp_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }
  grp_del->sel_grp_hdl = sel_grp_hdl;

  /* Walk through all the hw instances and deactivate from each of the
   * stages
   */
  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      rc = pipe_mgr_sel_grp_deactivate_stage_internal(
          &sel_tbl_info->sel_tbl[pipe_idx],
          sel_grp_stage_info->stage_p,
          sel_grp_stage_info,
          backup_restore_mode,
          move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Deactivate of the group %d failed "
            "rc 0x%x",
            __func__,
            __LINE__,
            sel_grp_hdl,
            rc);
        goto cleanup;
      }
    }
  }

  if (sel_tbl_info->cache_id && skip_cache_id == false) {
    bf_map_sts_t sts = bf_map_rmv(&sel_tbl->grp_id_map, sel_grp_info->grp_id);
    if (sts != BF_MAP_OK) {
      LOG_ERROR("%s:%d Error removing group %d from selector table map rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_info->grp_id,
                sts);
      // This is not critical, can be ignored
    }
  }

  /* Remove the handle from the sel_grp_hdl_to_grp_htbl */
  rc = pipe_mgr_sel_grp_remove_from_htbl(sel_tbl, sel_grp_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error removing group %d from selector table htbl rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              rc);
    goto cleanup;
  }
  /*Do not destroy the group yet because we might need it for rollback*/
  if (skip_cache_id == false) {
    pipe_mgr_sel_grp_destroy(sel_grp_info);
    LOG_TRACE("%s:%d - %s (%d - 0x%x) Successfully deleted group 0x%x",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              sel_grp_hdl);
  }
  if (move_tail_p) {
    (*move_tail_p)->next = grp_del;
    *move_tail_p = grp_del;
  }
  return PIPE_SUCCESS;
cleanup:
  free_sel_move_list_and_data(&grp_del);
  return rc;
}

/*!
 * API function to delete a group from a selection table
 */
pipe_status_t rmt_sel_grp_del(pipe_sess_hdl_t sess_hdl,
                              bf_dev_id_t dev_id,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              pipe_sel_grp_hdl_t sel_grp_hdl,
                              uint32_t pipe_api_flags,
                              struct pipe_mgr_sel_move_list_t **move_head_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_mbr_t *grp_mbr = NULL;
  pipe_sel_grp_mbr_hdl_t mbr_hdl = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t mbr_weight = 0;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Sanity check */
  if (sel_grp_info->num_references) {
    LOG_ERROR(
        "%s:%d Unable to delete sel grp %d which is still referenced by %d "
        "match entries",
        __func__,
        __LINE__,
        sel_grp_hdl,
        sel_grp_info->num_references);
    return PIPE_ENTRY_REFERENCES_EXIST;
  }

  LOG_TRACE("%s:%d - %s (%d - 0x%x) Request to delete group 0x%x",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->dev_id,
            sel_tbl_info->tbl_hdl,
            sel_grp_hdl);

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  /* Update mbr reference counts */
  JUDYL_FOREACH(sel_grp_info->sel_grp_mbrs, mbr_hdl, sel_grp_mbr_t *, grp_mbr) {
    mbr_weight = grp_mbr->weight;
    while (mbr_weight--) {
      rmt_adt_ent_non_sharable_del(
          dev_id, sel_tbl_info->adt_tbl_hdl, mbr_hdl, pipe_api_flags);
    }
  }
  /*check to skip_backup because the selector resize caller does not require
   * backup*/
  if (!(pipe_api_flags & PIPE_FLAG_SKIP_BACKUP)) {
    rc = pipe_mgr_sel_grp_backup_one(sel_tbl, sel_grp_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error backing up group %d rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_hdl,
                rc);
      goto cleanup;
    }
  }
  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  // Required for group resizing, when temporary group info is being removed
  bool skip_cache_id = pipe_api_flags & PIPE_MGR_TBL_API_CACHE_ENT_ID;
  rc = pipe_mgr_sel_grp_del_internal(sel_tbl,
                                     sel_grp_hdl,
                                     false,
                                     skip_cache_id,
                                     move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Selector group delete failed for dev %d tbl_hdl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        dev_id,
        sel_tbl_hdl,
        rc);
    goto cleanup;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  PIPE_MGR_SEL_TBL_ASSERT(dev_id, sel_tbl_hdl);
  return PIPE_SUCCESS;
cleanup:
  return rc;
}

static pipe_status_t update_handle_in_grp_stage_info(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_hdl_t new_hdl,
    pipe_sel_grp_hdl_t old_hdl) {
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  (void)pipe_idx;
  uint32_t stage_idx;
  (void)stage_idx;

  /* Go over all selector group stage info to update handle (used in callback)*/
  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      PIPE_MGR_DBGCHK(sel_grp_stage_info->grp_hdl == old_hdl);
      sel_grp_stage_info->grp_hdl = new_hdl;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_sel_grp_notify_mat(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  bf_dev_pipe_t pipe_id;

  /* Get required structs and validate inputs */
  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_mat_backptr_t *el = sel_grp_info->mat_tbl_list;
  unsigned long key;
  void *data;
  bf_map_sts_t sts;
  pipe_status_t ret = PIPE_SUCCESS;
  while (el) {
    struct pipe_mgr_move_list_t *iter = NULL;
    struct pipe_mgr_move_list_t *move_head_p = NULL;
    sts = bf_map_get_first(&el->ent_hdls, &key, (void **)&data);
    while (sts == BF_MAP_OK) {
      struct pipe_mgr_move_list_t *ml_node = NULL;
      ret = pipe_mgr_mat_tbl_update_action(
          sess_hdl, dev_tgt, el->mat_tbl, (pipe_mat_ent_hdl_t)key, &ml_node);
      if (ret != PIPE_SUCCESS) break;
      if (iter == NULL) {
        move_head_p = ml_node;
      } else {
        iter->next = ml_node;
      }
      iter = ml_node;
      sts = bf_map_get_next(&el->ent_hdls, &key, (void **)&data);
    }
    if (ret != PIPE_SUCCESS) break;
    if (move_head_p != NULL) {
      ret = pipe_mgr_sm_save_ml_prep(sess_hdl, dev_tgt.device_id, el->mat_tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Fail saving ml %d", __func__, __LINE__, ret);
        el = NULL;
        break;
      }
      pipe_mgr_sm_save_ml(
          sess_hdl, dev_tgt.device_id, el->mat_tbl, move_head_p);
    }

    el = el->next;
  }
  return ret;
}

/*!
 * API function to resize group from a selection table
 */
pipe_status_t rmt_sel_grp_resize(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_hdl_t *tmp_grp_hdl,
    uint32_t max_grp_size,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_id_t dev_id = dev_tgt.device_id;
  bf_dev_pipe_t pipe_id = dev_tgt.dev_pipe_id;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_t *sel_tbl;

  /* Get required structs and validate inputs */
  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) && (pipe_id != BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install an asymmetric entry"
        " in a symmetric table 0x%x device %d",
        __func__,
        __LINE__,
        sel_tbl_hdl,
        dev_id);
    return PIPE_INVALID_ARG;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (max_grp_size > sel_tbl_info->max_grp_size) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Maximum group size on this table is %d, requested %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->max_grp_size,
        max_grp_size);
    return PIPE_INVALID_ARG;
  } else if (max_grp_size <= 0) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Maximum group size cannot be 0 or less",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  /* Nothing to do if same size */
  if (sel_grp_info->max_grp_size == max_grp_size) {
    return PIPE_SUCCESS;
  }

  /* Check if existing members will fit in new size */
  if (sel_grp_info->mbr_count > max_grp_size) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Cannot resize group table member count %d will not fit requested %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        sel_grp_info->mbr_count,
        max_grp_size);
    return PIPE_INVALID_ARG;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  /* Allocate temporary handle to create new group with new max size */
  *tmp_grp_hdl = pipe_mgr_sel_grp_hdl_allocate(sel_tbl, pipe_id);

  struct pipe_mgr_sel_move_list_t *move_tail;

  if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
    pipe_mgr_sel_grp_replay_info_t *grp_replay_info = NULL;
    bf_map_sts_t sts = bf_map_get(&ha_hlp_info->replay_hdl_to_info,
                                  sel_grp_hdl,
                                  (void **)&grp_replay_info);
    if (sts != BF_MAP_OK || !grp_replay_info) {
      LOG_ERROR(
          "%s:%d Replay info not found, err = %d.", __func__, __LINE__, sts);
      return PIPE_INVALID_ARG;
    }
    grp_replay_info->max_grp_size = max_grp_size;
    return PIPE_SUCCESS;
  }

  /* add_internal will allocate the list */
  rc = pipe_mgr_sel_grp_add_internal(sel_tbl,
                                     pipe_id,
                                     sel_grp_info->grp_id,
                                     max_grp_size,
                                     *tmp_grp_hdl,
                                     PIPE_MGR_LOGICAL_ACT_IDX_INVALID,
                                     move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Selector table group creation failed dev %d"
        "tbl_hdl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        dev_id,
        sel_tbl_hdl,
        rc);
    return rc;
  }

  if (move_head_p) {
    *move_head_p = move_tail;
  }

  /* Copy members to new group */
  uint32_t num_mbrs = sel_grp_info->mbr_count;
  pipe_adt_ent_hdl_t *mbrs =
      PIPE_MGR_CALLOC(num_mbrs, sizeof(pipe_adt_ent_hdl_t));
  bool *enable = PIPE_MGR_CALLOC(num_mbrs, sizeof(bool));
  uint32_t mbrs_populated = 0;
  rc = rmt_sel_grp_mbrs_get(dev_id,
                            sel_tbl_hdl,
                            sel_grp_hdl,
                            num_mbrs,
                            mbrs,
                            enable,
                            &mbrs_populated);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error getting members for sel grp %d"
        "tbl_hdl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        sel_grp_hdl,
        sel_tbl_hdl,
        rc);
    if (mbrs) PIPE_MGR_FREE(mbrs);
    if (enable) PIPE_MGR_FREE(enable);
    return rc;
  }

  struct pipe_mgr_sel_move_list_t *move_list_mbrs;
  // Enable skip backup bit in flag because the backup tmp_grp_hdl is not
  // required
  pipe_api_flags |= PIPE_FLAG_SKIP_BACKUP;
  rc = rmt_sel_grp_mbrs_set(sess_hdl,
                            dev_id,
                            sel_tbl_hdl,
                            *tmp_grp_hdl,
                            mbrs_populated,
                            mbrs,
                            enable,
                            pipe_api_flags,
                            move_head_p ? &move_list_mbrs : NULL);
  pipe_api_flags ^= PIPE_FLAG_SKIP_BACKUP;
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error setting members for sel grp %d"
        "tbl_hdl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        *tmp_grp_hdl,
        sel_tbl_hdl,
        rc);
    if (mbrs) PIPE_MGR_FREE(mbrs);
    if (enable) PIPE_MGR_FREE(enable);
    return rc;
  }

  if (mbrs) PIPE_MGR_FREE(mbrs);
  if (enable) PIPE_MGR_FREE(enable);

  /* Merge lists if members were set. After add there should be only 1 element*/
  move_tail->next = move_list_mbrs;
  while (move_tail->next) move_tail = move_tail->next;
  sel_grp_info_t *new_sel_grp_info =
      pipe_mgr_sel_grp_get(sel_tbl, *tmp_grp_hdl);
  if (new_sel_grp_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  /* Need to copy new allocated resources to old group and old resource to new
   * group */

  Pvoid_t old_pipe_lookup = sel_grp_info->sel_grp_pipe_lookup;
  Pvoid_t old_grp_mbrs = sel_grp_info->sel_grp_mbrs;
  rc = pipe_mgr_sel_grp_backup_one(sel_tbl, sel_grp_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error backing up group %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              rc);
    return rc;
  }
  if ((pipe_api_flags & PIPE_MGR_TBL_API_TXN) && rc == PIPE_SUCCESS) {
    /*Set the flag to indicate this backup is from resizing*/
    sel_tbl_info_t *bsel_tbl_info = pipe_mgr_sel_tbl_info_get(
        sel_tbl_info->dev_id, sel_tbl_info->tbl_hdl, true);
    if (!bsel_tbl_info) {
      PIPE_MGR_ASSERT(0);
      return PIPE_UNEXPECTED;
    }
    sel_tbl_t *bsel_tbl =
        get_sel_tbl_by_pipe_id(bsel_tbl_info, sel_tbl->pipe_id);
    if (!bsel_tbl) {
      PIPE_MGR_ASSERT(0);
      return PIPE_UNEXPECTED;
    }
    sel_grp_info_t *bsel_grp = pipe_mgr_sel_grp_get(bsel_tbl, sel_grp_hdl);
    if (!bsel_grp) {
      PIPE_MGR_ASSERT(0);
      return PIPE_UNEXPECTED;
    }
    bsel_grp->backup_from_resize = true;
  }
  sel_grp_info->max_grp_size = new_sel_grp_info->max_grp_size;
  sel_grp_info->sel_grp_pipe_lookup = new_sel_grp_info->sel_grp_pipe_lookup;
  sel_grp_info->sel_grp_mbrs = new_sel_grp_info->sel_grp_mbrs;
  new_sel_grp_info->sel_grp_pipe_lookup = old_pipe_lookup;
  new_sel_grp_info->sel_grp_mbrs = old_grp_mbrs;

  /* Update handle in selector group stage info */
  rc = update_handle_in_grp_stage_info(sel_grp_info, sel_grp_hdl, *tmp_grp_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error updating grp %d handle", __func__, __LINE__, *tmp_grp_hdl);
    return rc;
  }
  PIPE_MGR_SEL_TBL_ASSERT(dev_id, sel_tbl_hdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_sel_grp_get_idx_count(sel_hlp_word_data_t *word_data,
                                               uint32_t no_words) {
  uint32_t idx = 0;
  uint32_t i = 0;

  for (i = 0; i < no_words; i++) {
    idx += word_data[i].usage;
  }

  return idx;
}

static uint32_t pipe_mgr_sel_grp_get_word_with_lowest_usage(
    sel_hlp_word_data_t *word_data, uint32_t no_words) {
  uint32_t low_idx = 0, low_usage = 0;
  uint32_t i = 0;

  low_usage = word_data->usage;
  for (i = 1; i < no_words; i++) {
    if (low_usage > word_data[i].usage) {
      low_idx = i;
      low_usage = word_data[i].usage;
    }
  }

  return low_idx;
}

static uint32_t pipe_mgr_sel_grp_get_word_with_highest_usage(
    sel_hlp_word_data_t *word_data, uint32_t no_words) {
  uint32_t high_idx = 0, high_usage = 0;
  uint32_t i = 0;

  high_usage = word_data->usage;
  for (i = 1; i < no_words; i++) {
    if (high_usage < word_data[i].usage) {
      high_idx = i;
      high_usage = word_data[i].usage;
    }
  }

  return high_idx;
}

/**
 * Function used in case of sequence_order to get the word and index when
 * members cant be fitted inside single word.
 * Members are split across words equally while maintaining the order based on
 * total number of members configured.
 */

static pipe_status_t pipe_mgr_sel_get_word_idx(
    uint32_t max_usage,  // Total number of members configured in the group
    uint32_t cur_usage,  // Current element in the group
    uint32_t no_words,   // No of RAM words allocated
    uint32_t *word_idx,
    uint32_t *mbr_index) {
  uint32_t mbrs_per_word = 0;

  mbrs_per_word = max_usage / no_words;
  if (max_usage % no_words) {
    mbrs_per_word++;
  }

  *word_idx = cur_usage / mbrs_per_word;
  *mbr_index = cur_usage % mbrs_per_word;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_copy(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    sel_hlp_word_data_t *src_word,
    uint32_t sword_idx,
    uint32_t src_start_idx,
    uint32_t src_end_idx,
    uint32_t dest_word_idx,
    uint32_t dest_start_idx,
    bool clear_src,
    bool update_hw_locator,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  uint32_t i = 0;
  uint32_t dest_idx = dest_start_idx;
  sel_hlp_word_data_t *dest_word = NULL;
  pipe_adt_ent_hdl_t src_adt_ent_hdl;
  pipe_status_t rc = PIPE_SUCCESS;

  dest_word = &sel_grp_stage_info->sel_grp_word_data[dest_word_idx];

  for (i = src_start_idx, dest_idx = dest_start_idx; i <= src_end_idx;
       i++, dest_idx++) {
    src_adt_ent_hdl = src_word->mbrs[i];
    PIPE_MGR_DBGCHK(dest_word->mbrs[dest_idx] == 0);

    dest_word->mbrs[dest_idx] = src_adt_ent_hdl;

    if (update_hw_locator) {
      rc = pipe_mgr_sel_grp_mbr_hw_locator_update_word(sel_grp_stage_info,
                                                       src_adt_ent_hdl,
                                                       sword_idx,
                                                       dest_word_idx,
                                                       dest_idx);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x %d) Error updating mbr hw locator for mbr 0x%x "
            "grp 0x%x",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id,
            src_adt_ent_hdl,
            sel_grp_stage_info->grp_hdl);
        return rc;
      }
    }

    if (clear_src) {
      /* Issue callback to allow client to track updates. */
      uint32_t src_word_idx =
          (ptrdiff_t)(src_word - sel_grp_stage_info->sel_grp_word_data);
      rc = issue_callback(sel_tbl,
                          sel_grp_stage_info,
                          src_word->mbrs[i],
                          src_word_idx,
                          i,
                          PIPE_SEL_UPDATE_DEL,
                          false,
                          move_tail_p);
      if (PIPE_SUCCESS != rc) return rc;

      src_word->mbrs[i] = 0;
    }

    /* Issue callback to allow client to track updates. */
    rc = issue_callback(sel_tbl,
                        sel_grp_stage_info,
                        dest_word->mbrs[dest_idx],
                        dest_word_idx,
                        dest_idx,
                        PIPE_SEL_UPDATE_ADD,
                        false,
                        move_tail_p);
    if (PIPE_SUCCESS != rc) return rc;
    /* Deactivate copied entry if necessary */
    if (!pipe_mgr_sel_grp_mbr_get_state(sel_grp_info, src_adt_ent_hdl)) {
      rc = issue_callback(sel_tbl,
                          sel_grp_stage_info,
                          dest_word->mbrs[dest_idx],
                          dest_word_idx,
                          dest_idx,
                          PIPE_SEL_UPDATE_DEACTIVATE,
                          false,
                          move_tail_p);
      if (PIPE_SUCCESS != rc) return rc;
    }

    if (update_hw_locator && sel_tbl_info->sequence_order) {
      uint32_t dest_word_pos = 0, src_word_pos = 0;
      uint32_t dest_pos = 0, src_pos = 0;

      dest_word_pos = pipe_mgr_sel_grp_get_idx_count(
          sel_grp_stage_info->sel_grp_word_data, dest_word_idx);
      dest_pos = dest_word_pos + dest_idx;

      src_word_pos = pipe_mgr_sel_grp_get_idx_count(
          sel_grp_stage_info->sel_grp_word_data, sword_idx);
      src_pos = src_word_pos + i;

      rc = pipe_mgr_sel_grp_mbr_update_pos(sel_grp_info,
                                           src_adt_ent_hdl,
                                           dest_pos,
                                           src_pos,
                                           sel_tbl_info->sequence_order);
      if (PIPE_SUCCESS != rc) return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_spread_entries(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  uint32_t no_words = 0;
  sel_hlp_word_data_t *word_0 = NULL, *word_data = NULL;
  uint32_t no_entries_per_word;
  uint32_t word_0_idx = 0, word_0_end_idx = 0;
  uint32_t i = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t excess_entries = 0, usage = 0;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  uint32_t j = 0;

  no_words = grp_stage_info->no_words;
  word_0 = grp_stage_info->sel_grp_word_data;

  no_entries_per_word = grp_stage_info->entries_per_word / no_words;
  excess_entries = grp_stage_info->entries_per_word % no_words;

  sel_tbl_stage_info_t *stage_info = grp_stage_info->stage_p;

  /* All members, starting with this index, through the end of the word will
   * be removed from word 0 and evenly spread across the other words. */
  word_0_idx = no_entries_per_word;

  /* First empty all members in all words except the first word. */
  for (i = 1; i < no_words; i++) {
    word_data = &grp_stage_info->sel_grp_word_data[i];

    usage = no_entries_per_word;

    for (j = 0; j < word_data->usage; j++) {
      adt_ent_hdl = word_data->mbrs[j];
      if (adt_ent_hdl == 0) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      /* Issue callback to allow client to track updates. */
      rc = issue_callback(sel_tbl,
                          grp_stage_info,
                          word_data->mbrs[j],
                          i,
                          j,
                          PIPE_SEL_UPDATE_DEL,
                          false,
                          move_tail_p);
      if (PIPE_SUCCESS != rc) {
        return rc;
      }
    }

    /* If the members don't divide evenly then put one extra in each word
     * until the surplus is gone. */
    if (excess_entries) {
      usage++;
      excess_entries--;
    }

    word_0_end_idx = word_0_idx + usage - 1;

    /* Clear out the entry handles for all the members in the word since
     * they have just been deleted. */
    PIPE_MGR_MEMSET(
        word_data->mbrs,
        0,
        sizeof(pipe_sel_grp_mbr_hdl_t) * stage_info->ram_word_width);
    rc = pipe_mgr_sel_grp_mbr_copy(sel_tbl,
                                   grp_stage_info,
                                   sel_grp_info,
                                   word_0,
                                   0,
                                   word_0_idx,
                                   word_0_end_idx,
                                   i,
                                   0,
                                   true,
                                   true,
                                   move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return rc;
    }

    /* Update usage */
    word_data = &grp_stage_info->sel_grp_word_data[i];
    word_data->usage = usage;

    word_0_idx += usage;
  }

  word_0->usage = no_entries_per_word;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_del_from_stage_word(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    uint32_t mbr_word_idx,
    uint32_t mbr_idx,
    bool replace_from_other_word,
    bool update_hw_locator,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_hlp_word_data_t *word_data = NULL, *replace_word_data = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t no_words = 0;
  uint32_t replace_word_idx = 0;
  bool replace_entry = true;

  word_data = &sel_grp_stage_info->sel_grp_word_data[mbr_word_idx];
  replace_word_idx = mbr_word_idx;
  no_words = sel_grp_stage_info->no_words;

  pipe_adt_ent_hdl_t adt_ent_hdl = word_data->mbrs[mbr_idx];

  word_data->mbrs[mbr_idx] = 0;

  if (replace_from_other_word) {
    replace_word_idx = pipe_mgr_sel_grp_get_word_with_highest_usage(
        sel_grp_stage_info->sel_grp_word_data, no_words);
  }

  if ((replace_word_idx == mbr_word_idx) && (mbr_idx == word_data->usage - 1)) {
    /* Do not replace if this is the last member */
    replace_entry = false;
  }

  if (adt_ent_hdl == 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  rc = issue_callback(sel_tbl,
                      sel_grp_stage_info,
                      adt_ent_hdl,
                      mbr_word_idx,
                      mbr_idx,
                      PIPE_SEL_UPDATE_DEL,
                      false,
                      move_tail_p);
  if (PIPE_SUCCESS != rc) return rc;

  if (replace_entry) {
    /* Move the highest entry of the word to
     * this location
     */
    replace_word_data =
        &sel_grp_stage_info->sel_grp_word_data[replace_word_idx];
    rc = pipe_mgr_sel_grp_mbr_copy(sel_tbl,
                                   sel_grp_stage_info,
                                   sel_grp_info,
                                   replace_word_data,
                                   replace_word_idx,
                                   replace_word_data->usage - 1,
                                   replace_word_data->usage - 1,
                                   mbr_word_idx,
                                   mbr_idx,
                                   true,
                                   update_hw_locator,
                                   move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return rc;
    }
    word_data->usage++;
    replace_word_data->usage--;
  }

  word_data->usage--;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_converge(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_hlp_word_data_t *word_0 = NULL, *word_data = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;
  uint32_t no_words = 0;
  uint32_t word_0_idx = 0;
  uint32_t word_0_usage = 0;

  no_words = sel_grp_stage_info->no_words;
  word_0 = sel_grp_stage_info->sel_grp_word_data;

  word_0_idx = word_0->usage;
  for (i = 1; i < no_words; i++) {
    word_data = &sel_grp_stage_info->sel_grp_word_data[i];
    if (word_data->usage) {
      rc = pipe_mgr_sel_grp_mbr_copy(sel_tbl,
                                     sel_grp_stage_info,
                                     sel_grp_info,
                                     word_data,
                                     i,
                                     0,
                                     word_data->usage - 1,
                                     0,
                                     word_0_idx,
                                     true,
                                     true,
                                     move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error", __func__, __LINE__);
        return rc;
      }
      word_0_idx += word_data->usage;
    }
  }

  word_0->usage = word_0_idx;
  word_0_usage = word_0->usage;
  for (i = 1; i < no_words; i++) {
    word_data = &sel_grp_stage_info->sel_grp_word_data[i];
    rc = pipe_mgr_sel_grp_mbr_copy(sel_tbl,
                                   sel_grp_stage_info,
                                   sel_grp_info,
                                   word_0,
                                   0,
                                   0,
                                   word_0_usage - 1,
                                   i,
                                   0,
                                   false,
                                   false,
                                   move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return rc;
    }

    word_data->usage = word_0_usage;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_del_and_converge(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    uint32_t mbr_word,
    uint32_t mbr_idx,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_sel_grp_mbr_del_from_stage_word(sel_tbl,
                                                sel_grp_stage_info,
                                                sel_grp_info,
                                                mbr_word,
                                                mbr_idx,
                                                false,
                                                true,
                                                move_tail_p);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error", __func__, __LINE__);
    return rc;
  }

  rc = pipe_mgr_sel_grp_converge(
      sel_tbl, sel_grp_stage_info, sel_grp_info, move_tail_p);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error", __func__, __LINE__);
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_add_to_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t total_conf_mbrs,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  uint32_t cur_usage = 0;
  uint32_t mbr_idx = 0, mbr_word = 0;
  uint32_t no_words = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0, index = 0;
  bool index_set = false;
  uint32_t word_idx_to_insert = 0;
  uint32_t cur_pos = 0, cur_word_pos = 0;
  sel_tbl_stage_info_t *sel_tbl_stage_info = sel_grp_stage_info->stage_p;

  cur_usage = sel_grp_stage_info->cur_usage;
  no_words = sel_grp_stage_info->no_words;

  /* Members in the selector group will be programmed differently based on
   * if sequence order is enabled or not.
   */
  if (sel_tbl_info->sequence_order && total_conf_mbrs) {
    /* When mbrs needs to be spread across the word, transitioning from
     * duplicated to !duplicated
     */
    if (total_conf_mbrs > sel_grp_stage_info->entries_per_word &&
        sel_grp_stage_info->mbrs_duplicated) {
      sel_hlp_word_data_t *word_data = NULL;
      uint32_t j = 0;
      pipe_adt_ent_hdl_t adt_ent_hdl = 0;

      /* First empty all members in all words except the first word. */
      for (i = 1; i < no_words; i++) {
        word_data = &sel_grp_stage_info->sel_grp_word_data[i];

        for (j = 0; j < word_data->usage; j++) {
          adt_ent_hdl = word_data->mbrs[j];
          if (adt_ent_hdl == 0) {
            PIPE_MGR_DBGCHK(0);
            return PIPE_UNEXPECTED;
          }

          /* Issue callback to allow client to track updates. */
          rc = issue_callback(sel_tbl,
                              sel_grp_stage_info,
                              word_data->mbrs[j],
                              i,
                              j,
                              PIPE_SEL_UPDATE_DEL,
                              false,
                              move_tail_p);
          if (PIPE_SUCCESS != rc) {
            return rc;
          }
        }

        word_data->usage = 0;
        /* Clear out the entry handles for all the members in the word since
         * they have just been deleted. */
        PIPE_MGR_MEMSET(word_data->mbrs,
                        0,
                        sizeof(pipe_sel_grp_mbr_hdl_t) *
                            sel_tbl_stage_info->ram_word_width);
      }
      sel_grp_stage_info->mbrs_duplicated = false;
    }
    if (!sel_grp_stage_info->mbrs_duplicated) {
      /* Incase of sequence ordering, divide the members among the words */
      pipe_mgr_sel_get_word_idx(
          total_conf_mbrs, cur_usage, no_words, &word_idx_to_insert, &index);

      PIPE_MGR_DBGCHK(word_idx_to_insert < no_words);
      index_set = true;
    }

    /* Check if the group is transitioning from duplicated to !duplicated. */
  } else if (sel_grp_stage_info->mbrs_duplicated &&
             cur_usage == sel_grp_stage_info->entries_per_word &&
             1 != sel_grp_stage_info->no_words) {
    LOG_TRACE("%s:%d - %s (%d - 0x%x) Spreading sel grp 0x%x at stage %d",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              sel_grp_info->grp_hdl,
              sel_tbl_stage_info->stage_id);

    /* Need to spread the entries across all the available
     * no_words and then do a simple add
     */
    rc = pipe_mgr_sel_grp_spread_entries(
        sel_tbl, sel_grp_stage_info, sel_grp_info, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%#x %d.%x) Error \"%s\" spreading while adding mbr %#x to "
          "grp %#x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id,
          sel_tbl->pipe_id,
          pipe_str_err(rc),
          mbr_hdl,
          sel_grp_info->grp_hdl);
      goto cleanup;
    }
    sel_grp_stage_info->mbrs_duplicated = false;
  }

  if (sel_grp_stage_info->mbrs_duplicated) {
    /* The members are duplicated in each word of the group so loop over
     * all words. */
    uint32_t first_free_idx_0 = 0;
    for (i = 0; i < no_words; i++) {
      uint32_t m = 0, first_free_idx = 0;
      bool free_idx_found = false;
      if (pipe_mgr_hitless_warm_init_in_progress(
              sel_tbl->sel_tbl_info->dev_id)) {
        sel_hlp_word_data_t *word_data;
        word_data = &sel_grp_stage_info->sel_grp_word_data[i];
        /* Check if there is a free index between 0 and usage,
           These free indexes are memebrs that were in de-activated state
           before HA hitless
        */
        for (m = 0; m < sel_grp_stage_info->sel_grp_word_data[i].usage; m++) {
          if (word_data->mbrs[m] == 0) {
            free_idx_found = true;
            first_free_idx = m;
            break;
          }
        }
        if (free_idx_found == true) {
          mbr_idx = first_free_idx;
          if (i == 0) {
            first_free_idx_0 = first_free_idx;
          }
        } else {
          mbr_idx = sel_grp_stage_info->sel_grp_word_data[i].usage;
        }
      } else {
        mbr_idx = sel_grp_stage_info->sel_grp_word_data[i].usage;
      }
      rc = add_one_mbr(sel_tbl,
                       sel_grp_info,
                       sel_grp_stage_info,
                       mbr_hdl,
                       i,
                       mbr_idx,
                       move_tail_p);
      if (PIPE_SUCCESS != rc) break;
      /* Ensure the member is added at the same index in all words. */
      if (pipe_mgr_hitless_warm_init_in_progress(
              sel_tbl->sel_tbl_info->dev_id)) {
        if (free_idx_found) {
          PIPE_MGR_DBGCHK(mbr_idx == first_free_idx_0);
        } else {
          PIPE_MGR_DBGCHK(mbr_idx ==
                          sel_grp_stage_info->sel_grp_word_data[0].usage - 1);
        }
      } else {
        PIPE_MGR_DBGCHK(mbr_idx ==
                        sel_grp_stage_info->sel_grp_word_data[0].usage - 1);
      }
      index_set = true;
    }
    /* The hardware locator only tracks the first word when the members are
     * duplicated. */
    mbr_word = 0;
    cur_pos = mbr_idx;
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

  } else {
    if (!index_set) {
      /* Simple add into the word with lowest usage*/
      word_idx_to_insert = pipe_mgr_sel_grp_get_word_with_lowest_usage(
          sel_grp_stage_info->sel_grp_word_data, no_words);
      PIPE_MGR_DBGCHK(word_idx_to_insert < no_words);
      index = sel_grp_stage_info->sel_grp_word_data[word_idx_to_insert].usage;
    }
    rc = add_one_mbr(sel_tbl,
                     sel_grp_info,
                     sel_grp_stage_info,
                     mbr_hdl,
                     word_idx_to_insert,
                     index,
                     move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(%#x %d.%x) Error \"%s\" adding mbr %#x to grp %#x",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                sel_tbl->pipe_id,
                pipe_str_err(rc),
                mbr_hdl,
                sel_grp_info->grp_hdl);
      goto cleanup;
    }
    mbr_idx = index;
    mbr_word = word_idx_to_insert;
    index_set = true;
    cur_word_pos = pipe_mgr_sel_grp_get_idx_count(
        sel_grp_stage_info->sel_grp_word_data, mbr_word);
    cur_pos = cur_word_pos + mbr_idx;
  }

  if (!index_set) {
    LOG_ERROR("%s:%d %s(%#x %d.%x) No space to add mbr %#x to grp %#x",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_tbl->pipe_id,
              mbr_hdl,
              sel_grp_info->grp_hdl);
    goto cleanup;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Added mbr 0x%x to sel grp 0x%x at stage %d word %d index %d",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      mbr_hdl,
      sel_grp_info->grp_hdl,
      sel_tbl_stage_info->stage_id,
      mbr_word,
      mbr_idx);

  rc = pipe_mgr_sel_grp_mbr_hw_locator_update(
      sel_grp_stage_info, mbr_hdl, mbr_word, mbr_idx);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%#x %d.%x) Error updating mbr hw locator for mbr %#x grp %#x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_info->grp_hdl);
    goto cleanup;
  }

  rc = sel_grp_mbr_add_del_pos(sel_grp_info,
                               mbr_hdl,
                               cur_pos,
                               sel_tbl_info->sequence_order,
                               PIPE_SEL_UPDATE_ADD);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%#x %d.%x) Error updating mbr hw locator for mbr %#x grp %#x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_tbl->pipe_id,
        mbr_hdl,
        sel_grp_info->grp_hdl);
    goto cleanup;
  }
  sel_grp_stage_info->cur_usage++;

  return PIPE_SUCCESS;
cleanup:
  return rc;
}

pipe_status_t mbr_del_from_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    void *cookie,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  (void)cookie;
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  uint32_t cur_usage = 0;
  uint32_t no_words = 0;
  uint32_t i = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t mbr_word = 0, mbr_idx = 0;

  /* Find the member to be deleted in the group. */
  rc = pipe_mgr_sel_grp_mbr_get_offset(
      sel_tbl, sel_grp_stage_info, mbr_hdl, &mbr_word, &mbr_idx);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Get offset failed for hdl %d", __func__, __LINE__, mbr_hdl);
    return rc;
  }

  no_words = sel_grp_stage_info->no_words;
  cur_usage = sel_grp_stage_info->cur_usage;

  /* If decrementing the current usage brings us below the entries-per-word
   * threshold then we transition from spreading the members across the words
   * of the group to replicating all members in each word. */
  if (!sel_tbl_info->sequence_order && !sel_grp_stage_info->mbrs_duplicated &&
      (cur_usage <= sel_grp_stage_info->entries_per_word + 1) &&
      1 != sel_grp_stage_info->no_words) {
    /* Need to reconverge the entries */
    rc = pipe_mgr_sel_grp_mbr_del_and_converge(sel_tbl,
                                               sel_grp_stage_info,
                                               sel_grp_info,
                                               mbr_word,
                                               mbr_idx,
                                               move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return rc;
    }

    sel_grp_stage_info->mbrs_duplicated = true;
    sel_grp_stage_info->cur_usage--;

    rc = pipe_mgr_sel_grp_mbr_hw_locator_remove(sel_grp_stage_info, mbr_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x %d) Error deleting mbr hw locator for mbr 0x%x "
          "grp 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id,
          mbr_hdl,
          sel_grp_info->grp_hdl);
      return rc;
    }
  } else if (sel_grp_stage_info->mbrs_duplicated) {
    /* Need to delete the member from all words */
    bool update_hw_locator = true;
    for (i = 0; i < no_words; i++) {
      rc = rmv_one_mbr(sel_tbl,
                       sel_grp_stage_info,
                       sel_grp_info,
                       i,
                       mbr_idx,
                       i,
                       update_hw_locator,
                       move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d %s(0x%x %d) Error removing mbr 0x%x grp 0x%x word %d",
                  __func__,
                  __LINE__,
                  sel_tbl_info->name,
                  sel_tbl_info->tbl_hdl,
                  sel_tbl_info->dev_id,
                  mbr_hdl,
                  sel_grp_info->grp_hdl,
                  i);
        return rc;
      }
      /* Update the hw locator only for word 0 */
      update_hw_locator = false;
    }
    sel_grp_stage_info->cur_usage--;

  } else {
    uint32_t src_word_idx = 0;

    if (sel_tbl_info->sequence_order) {
      /* Incase of sequence order, we need to maintain order so dont change
         to different word */
      src_word_idx = mbr_word;
    } else {
      /* Move the msb member from the word with the highest usage to the
       * position of the member being deleted. */
      src_word_idx = pipe_mgr_sel_grp_get_word_with_highest_usage(
          sel_grp_stage_info->sel_grp_word_data, sel_grp_stage_info->no_words);
    }

    rc = rmv_one_mbr(sel_tbl,
                     sel_grp_stage_info,
                     sel_grp_info,
                     mbr_word,
                     mbr_idx,
                     src_word_idx,
                     true,
                     move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x %d) Error removing mbr 0x%x grp 0x%x",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                mbr_hdl,
                sel_grp_info->grp_hdl);
      return rc;
    }

    sel_grp_stage_info->cur_usage--;

    /* Reconverge the entries, since while modification selector group has one
     * active member (which is deleted at last), so converge when cur_usage is 1
     */
    if (sel_tbl_info->sequence_order && !sel_grp_stage_info->mbrs_duplicated &&
        1 != sel_grp_stage_info->no_words &&
        (sel_grp_stage_info->cur_usage == 1)) {
      rc = pipe_mgr_sel_grp_converge(
          sel_tbl, sel_grp_stage_info, sel_grp_info, move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error", __func__, __LINE__);
        return rc;
      }
      sel_grp_stage_info->mbrs_duplicated = true;
    }
  }

  return PIPE_SUCCESS;
}

static uint32_t get_adt_index(uint32_t adt_base_index, uint32_t mbr_idx) {
  return adt_base_index + mbr_idx;
}

static pipe_status_t issue_operation(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t mbr_word_idx,
    uint32_t mbr_idx,
    enum pipe_sel_update_type sel_op_type,
    bool disabled_mbr,
    bool replace_mbr_hdl,
    struct pipe_mgr_sel_move_list_t **move_tail_p);

/* Replace HA member handle with a new member handle.
 * Both have the same action data and function handle.
 */
pipe_status_t pipe_mgr_mbr_update_identical_in_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    uint32_t word_offset_in_grp,
    uint32_t mbr_offset_in_word,
    pipe_adt_ent_hdl_t ha_mbr_hdl,
    pipe_adt_ent_hdl_t new_mbr_hdl,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_mbr_t *mbr, *grp_mbr;

  /* Indicies must be valid. */
  PIPE_MGR_DBGCHK(word_offset_in_grp < sel_grp_stage_info->no_words);
  PIPE_MGR_DBGCHK(mbr_offset_in_word < sel_grp_stage_info->entries_per_word);
  sel_hlp_word_data_t *word_data;
  word_data = &sel_grp_stage_info->sel_grp_word_data[word_offset_in_grp];
  /* Index in word must be occupied with ha_mbr_hdl. */
  PIPE_MGR_DBGCHK(ha_mbr_hdl == word_data->mbrs[mbr_offset_in_word]);
  PIPE_MGR_DBGCHK(0 != word_data->usage);
  /* Update the handle. */
  word_data->mbrs[mbr_offset_in_word] = new_mbr_hdl;

  /* Issue callback to allow client to track updates. */
  rc = issue_operation(sel_tbl,
                       sel_grp_stage_info,
                       ha_mbr_hdl,
                       word_offset_in_grp,
                       mbr_offset_in_word,
                       PIPE_SEL_UPDATE_DEL,
                       false,
                       true,
                       move_tail_p);
  if (PIPE_SUCCESS != rc) return rc;
  rc = issue_operation(sel_tbl,
                       sel_grp_stage_info,
                       new_mbr_hdl,
                       word_offset_in_grp,
                       mbr_offset_in_word,
                       PIPE_SEL_UPDATE_ADD,
                       false,
                       true,
                       move_tail_p);
  if (PIPE_SUCCESS != rc) return rc;
  /* Remove the deleted member. */
  rc = pipe_mgr_sel_grp_mbr_hw_locator_remove(sel_grp_stage_info, ha_mbr_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x %d) Error deleting mbr hw locator for mbr 0x%x "
        "grp 0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        ha_mbr_hdl,
        sel_grp_info->grp_hdl);
    return rc;
  }
  /* Update the moved member. */
  rc = pipe_mgr_sel_grp_mbr_hw_locator_update(
      sel_grp_stage_info, new_mbr_hdl, word_offset_in_grp, mbr_offset_in_word);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR(
        "%s:%d %s(0x%x %d) Error adding mbr hw locator for mbr 0x%x "
        "grp 0x%x",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        new_mbr_hdl,
        sel_grp_info->grp_hdl);
    return rc;
  }

  mbr = pipe_mgr_sel_grp_mbr_remove(sel_grp_info, ha_mbr_hdl, false);
  if (mbr == NULL) {
    LOG_ERROR("%s:%d %s(0x%x %d) Error removing mbr 0x%x grp 0x%x",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              ha_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, new_mbr_hdl, false);
  if (grp_mbr) {
    grp_mbr->weight++;
  } else {
    mbr->is_backup_valid = false;
    mbr->mbr_hdl = new_mbr_hdl;
    rc = pipe_mgr_sel_mbr_add_to_htbl(sel_grp_info, new_mbr_hdl, mbr, false);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return PIPE_INVALID_ARG;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_add_internal(
    sel_tbl_info_t *sel_tbl_info,
    sel_grp_info_t *sel_grp_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t grp_mbr_weight,
    uint32_t total_conf_mbrs,
    struct pipe_mgr_sel_move_list_t **move_tail_p,
    uint32_t *rollback_weight,
    bool skip_backup) {
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_t *grp_sel_tbl = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_act_fn_hdl_t mbr_act_fn_hdl = 0;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_grp_mbr_t *grp_mbr = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  (void)stage_idx;
  bool act_fn_changed = false, adt_state_updated = false;
  bool grp_mbr_updated = false;
  uint32_t mbr_weight = grp_mbr_weight;

  if (sel_grp_info->mbr_count == sel_grp_info->max_grp_size) {
    LOG_ERROR("%s:%d Group %d is already full",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl);
    return PIPE_NO_SPACE;
  }

  grp_sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, sel_grp_info->pipe_id);
  if (grp_sel_tbl == NULL) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              sel_grp_info->pipe_id);
    return PIPE_UNEXPECTED;
  }

  if (!sel_grp_info->act_fn_set) {
    sel_grp_info->act_fn_hdl = act_fn_hdl;
    sel_grp_info->act_fn_set = true;
    act_fn_changed = true;
  }

  if (sel_grp_info->act_fn_hdl != act_fn_hdl) {
    LOG_ERROR(
        "%s:%d Adding member %d with incompatible action with group %d "
        "in sel tbl 0x%x device %d",
        __func__,
        __LINE__,
        sel_grp_mbr_hdl,
        sel_grp_info->grp_hdl,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  /* Check if the member already exists */
  grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  if (grp_mbr) {
    LOG_DBG(
        "%s:%d Adding duplicate Selector group member 0x%x on group 0x%x on "
        "tbl "
        "0x%x with weight = %d",
        __func__,
        __LINE__,
        sel_grp_mbr_hdl,
        sel_grp_info->grp_hdl,
        sel_tbl_info->tbl_hdl,
        grp_mbr->weight);
  }

  while (mbr_weight--) {
    /* Update adt member state while checking if the member is valid */
    rc = rmt_adt_ent_non_sharable_add(sel_tbl_info->dev_id,
                                      sel_tbl_info->adt_tbl_hdl,
                                      sel_grp_mbr_hdl,
                                      &mbr_act_fn_hdl,
                                      grp_sel_tbl->sess_flags);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to update adt state for mbr %d in grp %d "
          "sel tbl 0x%x device %d",
          __func__,
          __LINE__,
          sel_grp_mbr_hdl,
          sel_grp_info->grp_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      goto cleanup;
    }
    adt_state_updated = true;

    if (mbr_act_fn_hdl != act_fn_hdl) {
      LOG_ERROR(
          "%s:%d Adding member %d with action function %d to group %d with "
          "action function %d in tbl 0x%x device %d",
          __func__,
          __LINE__,
          sel_grp_mbr_hdl,
          mbr_act_fn_hdl,
          sel_grp_info->grp_hdl,
          act_fn_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
    if (!skip_backup) {
      rc = pipe_mgr_sel_grp_mbr_backup_one(
          sel_tbl_info, grp_sel_tbl, sel_grp_info, sel_grp_mbr_hdl);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error backing up group %d mbr %d rc 0x%x",
                  __func__,
                  __LINE__,
                  sel_grp_info->grp_hdl,
                  sel_grp_mbr_hdl,
                  rc);
        goto cleanup;
      }
    }
    if (grp_mbr) {
      grp_mbr->weight++;
    } else {
      grp_mbr = pipe_mgr_sel_grp_mbr_alloc(sel_grp_mbr_hdl);
      if (!grp_mbr) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }

      rc = pipe_mgr_sel_mbr_add_to_htbl(
          sel_grp_info, sel_grp_mbr_hdl, grp_mbr, false);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error adding mbr %d to selector group %d",
                  __func__,
                  __LINE__,
                  sel_grp_mbr_hdl,
                  sel_grp_info->grp_hdl);
        pipe_mgr_sel_grp_mbr_destroy(grp_mbr);
        grp_mbr = NULL;
        goto cleanup;
      }
    }
    grp_mbr_updated = true;
    /* Now add this grp member into all the hardware stages where this
     * group is active
     */
    JUDYL_FOREACH(
        sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
      sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
      JUDYL_FOREACH2(stage_lookup,
                     stage_idx,
                     sel_grp_stage_info_t *,
                     sel_grp_stage_info,
                     2) {
        sel_grp_stage_info->act_fn_hdl = sel_grp_info->act_fn_hdl;
        rc = pipe_mgr_sel_grp_mbr_add_to_stage(sel_tbl,
                                               sel_grp_stage_info,
                                               sel_grp_info,
                                               sel_grp_mbr_hdl,
                                               total_conf_mbrs,
                                               move_tail_p);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in adding a member to stage", __func__, __LINE__);
          goto cleanup;
        }
      }
    }

    sel_grp_info->mbr_count++;
    adt_state_updated = false;
  }

  return PIPE_SUCCESS;
cleanup:
  if (act_fn_changed) {
    sel_grp_info->act_fn_set = false;
  }
  if (grp_mbr && grp_mbr_updated) {
    /* Failed during the staging */
    if (grp_mbr->weight == 1) {
      pipe_mgr_sel_grp_mbr_remove_and_destroy(
          sel_grp_info, sel_grp_mbr_hdl, false);
    } else {
      /* Weight is used for rollback, so just revert the incremented value */
      grp_mbr->weight--;
    }
  }
  if (adt_state_updated) {
    rmt_adt_ent_non_sharable_del(sel_tbl_info->dev_id,
                                 sel_tbl_info->adt_tbl_hdl,
                                 sel_grp_mbr_hdl,
                                 grp_sel_tbl->sess_flags);
  }

  /* when weight is more than one and one of the member is added to stage then
   * we need to rollback the installed member, so get the rollback members
   * weight caller function will roll back the partial installed group members
   */

  mbr_weight = grp_mbr_weight - mbr_weight;

  if (rollback_weight && mbr_weight > 1) {
    *rollback_weight = mbr_weight - 1;
  }
  return rc;
}

/*!
 * API function to register a callback function to track updates to groups in
 * the selection table.
 */
pipe_status_t rmt_sel_tbl_set_cb(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 pipe_mgr_sel_tbl_update_callback cb,
                                 void *cb_cookie) {
  (void)sess_hdl;
  sel_tbl_info_t *sel_tbl_info = NULL;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl_info->cb = cb;
  sel_tbl_info->cb_cookie = cb_cookie;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_update_active_mbr_count(
    sel_grp_info_t *sel_grp_info) {
  uint32_t num_active_mbrs = 0;
  sel_grp_mbr_t *mbr;
  Word_t mbr_hdl = 0;
  PWord_t Pvalue = NULL;

  if (!sel_grp_info) return PIPE_INVALID_ARG;

  JLF(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  while (Pvalue && Pvalue != PJERR) {
    mbr = (sel_grp_mbr_t *)*Pvalue;
    if (mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) num_active_mbrs++;
    JLN(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  }
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Error updating active member count for sel grp 0x%x",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl);
    return PIPE_UNEXPECTED;
  }

  sel_grp_info->num_active_mbrs = num_active_mbrs;
  return PIPE_SUCCESS;
}

/*!
 * API function to add a member to a group of a selection table
 */
pipe_status_t rmt_sel_grp_mbr_add(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  uint32_t pipe_idx;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Request to add mbr 0x%x on group 0x%x",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_grp_mbr_hdl,
      sel_grp_hdl);

  if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    bf_map_sts_t msts;
    for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
      sel_tbl_t *sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
      pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
      pipe_mgr_sel_grp_replay_info_t *grp_replay_info;
      msts = bf_map_get(&ha_hlp_info->replay_hdl_to_info,
                        sel_grp_hdl,
                        (void **)&grp_replay_info);
      if (msts != BF_MAP_OK) {
        continue;
      }
      if (!grp_replay_info->act_fn_hdl) {
        grp_replay_info->act_fn_hdl = act_fn_hdl;
      } else {
        PIPE_MGR_DBGCHK(grp_replay_info->act_fn_hdl == act_fn_hdl);
      }
      if (grp_replay_info->num_mbrs == grp_replay_info->max_grp_size) {
        LOG_ERROR(
            "%s:%d Group %d is already full", __func__, __LINE__, sel_grp_hdl);
        return PIPE_NO_SPACE;
      }
      grp_replay_info->mbr_hdls[grp_replay_info->num_mbrs] = sel_grp_mbr_hdl;
      grp_replay_info->mbr_enable[grp_replay_info->num_mbrs] = true;
      grp_replay_info->mbr_weight[grp_replay_info->num_mbrs] = 1;
      grp_replay_info->num_mbrs++;
    }
  } else {
    bf_dev_pipe_t pipe_id;
    if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
      pipe_id = BF_DEV_PIPE_ALL;
    else
      pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

    sel_tbl_t *sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
    if (!sel_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Selector table with pipe %x (grp_hdl 0x%x) not found",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->dev_id,
          sel_tbl_info->tbl_hdl,
          pipe_id,
          sel_grp_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
    if (sel_grp_info == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                sel_grp_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* Set-up the session parameters */
    sel_tbl->cur_sess_hdl = sess_hdl;
    sel_tbl->sess_flags = pipe_api_flags;

    struct pipe_mgr_sel_move_list_t move_head;
    move_head.next = NULL;
    struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

    rc = pipe_mgr_sel_grp_mbr_add_internal(sel_tbl_info,
                                           sel_grp_info,
                                           act_fn_hdl,
                                           sel_grp_mbr_hdl,
                                           1,  // mbr_weight
                                           0,  // num_add_mbrs
                                           move_head_p ? &move_tail : NULL,
                                           NULL,
                                           false);
    if (move_head_p) {
      *move_head_p = move_head.next;
    }
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x %d.%x) Error adding member 0x%x to group 0x%x, \"%s\"",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id,
          sel_grp_info->pipe_id,
          sel_grp_mbr_hdl,
          sel_grp_info->grp_hdl,
          pipe_str_err(rc));
      return rc;
    }
  }

  if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

static void update_adt_base_indexes(sel_tbl_t *sel_tbl,
                                    int update_stage_idx,
                                    uint32_t word_idx,
                                    pipe_idx_t *adt_index_array) {
  int stage_idx;
  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    if (update_stage_idx == -1 || update_stage_idx == stage_idx) {
      sel_tbl_stage_info_t *stage_info =
          &sel_tbl->sel_tbl_stage_info[stage_idx];
      sel_hlp_word_data_t *word_data = NULL;
      word_data = &stage_info->hlp.hlp_word[word_idx];

      pipe_idx_t adt_index = word_data->adt_base_idx;

      adt_index_array[stage_idx] = adt_index;
    } else {
      adt_index_array[stage_idx] = PIPE_MGR_LOGICAL_ACT_IDX_INVALID;
    }
  }
}

static pipe_status_t issue_operation(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t mbr_word_idx,
    uint32_t mbr_idx,
    enum pipe_sel_update_type sel_op_type,
    bool disabled_mbr,
    bool replace_mbr_hdl,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_sel_grp_hdl_t grp_hdl = sel_grp_stage_info->grp_hdl;
  sel_grp_info_t *grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
  if (grp_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bool warm_init =
      pipe_mgr_hitless_warm_init_in_progress(sel_tbl->sel_tbl_info->dev_id);

  uint32_t word_idx;
  word_idx = sel_grp_stage_info->sel_base_idx + mbr_word_idx;
  uint32_t logical_index = get_logical_index_by_word_idx(word_idx, mbr_idx);
  if ((!sel_grp_stage_info->stage_p->stage_idx || warm_init) && move_tail_p) {
    struct pipe_mgr_sel_move_list_t *move_node;
    move_node =
        alloc_sel_move_list(*move_tail_p, sel_op_type, sel_tbl->pipe_id);
    if (!move_node) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    if (sel_op_type == PIPE_SEL_UPDATE_ADD) {
      pipe_idx_t adt_index_array[sel_tbl->num_stages];
      int stage_idx = -1;
      if (warm_init) {
        stage_idx = sel_grp_stage_info->stage_p->stage_idx;
      }
      update_adt_base_indexes(sel_tbl, stage_idx, word_idx, adt_index_array);

      move_node->data = make_sel_ent_data(sel_tbl->num_stages, adt_index_array);
      if (!move_node->data) {
        PIPE_MGR_FREE(move_node);
        LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
    }

    move_node->sel_grp_hdl = grp_hdl;
    move_node->adt_mbr_hdl = mbr_hdl;
    move_node->logical_sel_index = word_idx;
    move_node->logical_sel_subindex = mbr_idx;
    move_node->disabled_mbr = disabled_mbr;
    move_node->replace_mbr_hdl = replace_mbr_hdl;

    *move_tail_p = move_node;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Operation %d sel mbr 0x%x for grp 0x%x in stage %d index %d",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_op_type,
      mbr_hdl,
      grp_hdl,
      sel_grp_stage_info->stage_p->stage_id,
      logical_index);

  return rc;
}

static pipe_status_t issue_callback(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t mbr_word_idx,
    uint32_t mbr_idx,
    enum pipe_sel_update_type sel_op_type,
    bool disabled_mbr,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_sel_grp_hdl_t grp_hdl = sel_grp_stage_info->grp_hdl;
  sel_grp_info_t *grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
  if (grp_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  /* Issue callback when updating the first stage only. */
  pipe_mgr_sel_tbl_update_callback cb = sel_tbl->sel_tbl_info->cb;
  void *cookie = sel_tbl->sel_tbl_info->cb_cookie;

  uint32_t word_idx;
  word_idx = sel_grp_stage_info->sel_base_idx + mbr_word_idx;
  uint32_t logical_index = get_logical_index_by_word_idx(word_idx, mbr_idx);

  rc = issue_operation(sel_tbl,
                       sel_grp_stage_info,
                       mbr_hdl,
                       mbr_word_idx,
                       mbr_idx,
                       sel_op_type,
                       disabled_mbr,
                       false,
                       move_tail_p);
  if (rc) {
    LOG_ERROR("%s:%d Failed to issue operation", __func__, __LINE__);
    return rc;
  }
  if (!sel_grp_stage_info->stage_p->stage_idx) {
    if (cb && ((sel_op_type == PIPE_SEL_UPDATE_ADD) ||
               (sel_op_type == PIPE_SEL_UPDATE_DEL))) {
      dev_target_t x = {0};
      x.device_id = sel_tbl->sel_tbl_info->dev_id;
      x.dev_pipe_id =
          sel_tbl->sel_tbl_info->is_symmetric ? DEV_PIPE_ALL : sel_tbl->pipe_id;
      rc = cb(
          sel_tbl->cur_sess_hdl,  // Session Handle
          x,                      // Dev_tgt
          cookie,                 //
          grp_hdl,                // Group Handle
          mbr_hdl,                // Member Handle (action data handle)
          logical_index,          // Logical Index
          sel_op_type == PIPE_SEL_UPDATE_ADD ? true : false);  // Add or Delete
    }
  }

  return rc;
}

static pipe_status_t wr_one_mbr(sel_tbl_t *sel_tbl,
                                sel_grp_stage_info_t *sel_grp_stage_info,
                                uint32_t word_offset_in_grp,
                                uint32_t mbr_offset_in_word,
                                bool new_mbr_val,
                                struct pipe_mgr_sel_move_list_t **move_tail_p) {
  PIPE_MGR_DBGCHK(word_offset_in_grp < sel_grp_stage_info->no_words);
  PIPE_MGR_DBGCHK(mbr_offset_in_word < sel_grp_stage_info->entries_per_word);

  sel_hlp_word_data_t *word_data;
  word_data = &sel_grp_stage_info->sel_grp_word_data[word_offset_in_grp];

  pipe_sel_grp_mbr_hdl_t mbr_hdl = word_data->mbrs[mbr_offset_in_word];
  if (mbr_hdl == 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  // Call issue_callback to update about the activate/deactivate
  pipe_status_t rc;
  rc = issue_callback(
      sel_tbl,
      sel_grp_stage_info,
      mbr_hdl,
      word_offset_in_grp,
      mbr_offset_in_word,
      new_mbr_val ? PIPE_SEL_UPDATE_ACTIVATE : PIPE_SEL_UPDATE_DEACTIVATE,
      false,
      move_tail_p);
  if (PIPE_SUCCESS != rc) {
    return rc;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t add_one_mbr(
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp_info,
    sel_grp_stage_info_t *sel_grp_stage_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    uint32_t word_offset_in_grp,
    uint32_t mbr_offset_in_word,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool disabled_mbr = false;

  /* Get the member state in hitless HA,
     It could be a de-activated member.
  */
  if (pipe_mgr_hitless_warm_init_in_progress(sel_tbl->sel_tbl_info->dev_id)) {
    if (!pipe_mgr_sel_grp_mbr_get_state(sel_grp_info, mbr_hdl)) {
      disabled_mbr = true;
    } else {
      disabled_mbr = false;
    }
  }

  /* Indicies must be valid. */
  PIPE_MGR_DBGCHK(word_offset_in_grp < sel_grp_stage_info->no_words);
  PIPE_MGR_DBGCHK(mbr_offset_in_word < sel_grp_stage_info->entries_per_word);

  sel_hlp_word_data_t *word_data;
  word_data = &sel_grp_stage_info->sel_grp_word_data[word_offset_in_grp];

  /* Index in word must not be occupied. */
  PIPE_MGR_DBGCHK(0 == word_data->mbrs[mbr_offset_in_word]);

  /* Issue callback to allow client to track updates. */
  rc = issue_callback(sel_tbl,
                      sel_grp_stage_info,
                      mbr_hdl,
                      word_offset_in_grp,
                      mbr_offset_in_word,
                      PIPE_SEL_UPDATE_ADD,
                      disabled_mbr,
                      move_tail_p);
  if (PIPE_SUCCESS != rc) {
    goto cleanup;
  }

  /* Increment usage */
  word_data->usage++;

  /* Save the new member's entry handle. */
  word_data->mbrs[mbr_offset_in_word] = mbr_hdl;

  return PIPE_SUCCESS;
cleanup:
  return rc;
}

static pipe_status_t rmv_one_mbr(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    uint32_t word_offset_in_grp,
    uint32_t mbr_offset_in_word,
    uint32_t src_word_offset_in_grp,
    bool update_hw_locator,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  uint32_t cur_word_pos = 0, src_word_pos = 0;
  uint32_t cur_pos = 0, src_pos = 0;

  /* Indicies must be valid. */
  PIPE_MGR_DBGCHK(word_offset_in_grp < sel_grp_stage_info->no_words);
  PIPE_MGR_DBGCHK(src_word_offset_in_grp < sel_grp_stage_info->no_words);
  PIPE_MGR_DBGCHK(mbr_offset_in_word < sel_grp_stage_info->entries_per_word);

  sel_hlp_word_data_t *word_data, *src_word_data;
  word_data = &sel_grp_stage_info->sel_grp_word_data[word_offset_in_grp];
  src_word_data =
      &sel_grp_stage_info->sel_grp_word_data[src_word_offset_in_grp];

  /* Index in word must be occupied, but it can be either active or
   * inactive. */
  PIPE_MGR_DBGCHK(0 != word_data->mbrs[mbr_offset_in_word]);
  PIPE_MGR_DBGCHK(0 != src_word_data->usage);
  PIPE_MGR_DBGCHK(0 != src_word_data->mbrs[src_word_data->usage - 1]);

  /* When removing a member compact the remaining members to the lsb.
   * Special case the logic for removing the msb member where nothing needs
   * to be done to compact the remaining entries. */
  if (mbr_offset_in_word + 1 == word_data->usage &&
      src_word_offset_in_grp == word_offset_in_grp) {
    pipe_sel_grp_mbr_hdl_t mbr_hdl = word_data->mbrs[mbr_offset_in_word];
    /* Issue callback to allow client to track updates. */
    rc = issue_callback(sel_tbl,
                        sel_grp_stage_info,
                        word_data->mbrs[mbr_offset_in_word],
                        word_offset_in_grp,
                        mbr_offset_in_word,
                        PIPE_SEL_UPDATE_DEL,
                        false,
                        move_tail_p);
    if (PIPE_SUCCESS != rc) return rc;

    /* Decrement the usage since the member is removed. */
    word_data->usage--;

    /* Clear the member's entry handle. */
    word_data->mbrs[mbr_offset_in_word] = 0;

    /* Remove the entry from the hw locator. */
    if (update_hw_locator) {
      rc = pipe_mgr_sel_grp_mbr_hw_locator_remove(sel_grp_stage_info, mbr_hdl);
      if (PIPE_SUCCESS != rc) return rc;

      cur_word_pos = pipe_mgr_sel_grp_get_idx_count(
          sel_grp_stage_info->sel_grp_word_data, word_offset_in_grp);
      cur_pos = cur_word_pos + mbr_offset_in_word;

      rc = sel_grp_mbr_add_del_pos(sel_grp_info,
                                   mbr_hdl,
                                   cur_pos,
                                   sel_tbl_info->sequence_order,
                                   PIPE_SEL_UPDATE_DEL);
      if (PIPE_SUCCESS != rc) return rc;
    }
  } else {
    /* Note this case could be operating on two separate words in the group
     * or the same word (tgt_word == src_word). */
    sel_hlp_word_data_t *tgt_word, *src_word;
    tgt_word = &sel_grp_stage_info->sel_grp_word_data[word_offset_in_grp];
    src_word = &sel_grp_stage_info->sel_grp_word_data[src_word_offset_in_grp];

    /* Get the handle and logical index (in the table) for the entry being
     * removed. */
    pipe_sel_grp_mbr_hdl_t tgt_hdl = tgt_word->mbrs[mbr_offset_in_word];
    /* Get the handle for the entry being moved. */
    uint32_t src_offset = src_word->usage - 1;
    pipe_sel_grp_mbr_hdl_t src_hdl = src_word->mbrs[src_offset];

    /* Update the handle since the action data is now moved. */
    tgt_word->mbrs[mbr_offset_in_word] = src_word->mbrs[src_offset];

    /* Issue callback to allow client to track updates. */
    rc = issue_callback(sel_tbl,
                        sel_grp_stage_info,
                        tgt_hdl,
                        word_offset_in_grp,
                        mbr_offset_in_word,
                        PIPE_SEL_UPDATE_DEL,
                        false,
                        move_tail_p);
    if (PIPE_SUCCESS != rc) return rc;
    rc = issue_callback(sel_tbl,
                        sel_grp_stage_info,
                        src_hdl,
                        word_offset_in_grp,
                        mbr_offset_in_word,
                        PIPE_SEL_UPDATE_ADD,
                        false,
                        move_tail_p);
    if (PIPE_SUCCESS != rc) return rc;
    /* Deactivate moved entry if necessary */
    if (!pipe_mgr_sel_grp_mbr_get_state(sel_grp_info, src_hdl)) {
      rc = issue_callback(sel_tbl,
                          sel_grp_stage_info,
                          src_hdl,
                          word_offset_in_grp,
                          mbr_offset_in_word,
                          PIPE_SEL_UPDATE_DEACTIVATE,
                          false,
                          move_tail_p);
      if (PIPE_SUCCESS != rc) return rc;
    }

    if (update_hw_locator) {
      /* Remove the deleted member. */
      rc = pipe_mgr_sel_grp_mbr_hw_locator_remove(sel_grp_stage_info, tgt_hdl);
      if (PIPE_SUCCESS != rc) return rc;

      cur_word_pos = pipe_mgr_sel_grp_get_idx_count(
          sel_grp_stage_info->sel_grp_word_data, word_offset_in_grp);
      cur_pos = cur_word_pos + mbr_offset_in_word;

      rc = sel_grp_mbr_add_del_pos(sel_grp_info,
                                   tgt_hdl,
                                   cur_pos,
                                   sel_tbl_info->sequence_order,
                                   PIPE_SEL_UPDATE_DEL);
      if (PIPE_SUCCESS != rc) return rc;

      /* Update the moved member. */
      rc = pipe_mgr_sel_grp_mbr_hw_locator_update_pos(sel_grp_stage_info,
                                                      src_hdl,
                                                      word_offset_in_grp,
                                                      mbr_offset_in_word,
                                                      src_offset,
                                                      src_word_offset_in_grp);
      if (PIPE_SUCCESS != rc) return rc;

      src_word_pos = pipe_mgr_sel_grp_get_idx_count(
          sel_grp_stage_info->sel_grp_word_data, src_word_offset_in_grp);
      src_pos = src_word_pos + src_offset;

      rc = pipe_mgr_sel_grp_mbr_update_pos(sel_grp_info,
                                           src_hdl,
                                           cur_pos,
                                           src_pos,
                                           sel_tbl_info->sequence_order);
      if (PIPE_SUCCESS != rc) return rc;
    }
    /* Delete msb from source word now that it has been copied to the new
     * location and has replaced the deleted entry. */
    rc = rmv_one_mbr(sel_tbl,
                     sel_grp_stage_info,
                     sel_grp_info,
                     src_word_offset_in_grp,
                     src_offset,
                     src_word_offset_in_grp,
                     false,
                     move_tail_p);
    if (PIPE_SUCCESS != rc) return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t mbr_enable_disable_in_stage(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    void *data_p,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  (void)sel_grp_info;
  bool data = *(uint8_t *)data_p;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t mbr_word_idx = 0, mbr_idx = 0;
  uint32_t num_mbrs = 0, mbrs = 0;

  /* Find the member in the group.  Note that mbr_word_idx will not be valid
   * if the members are duplicated. */

  rc = pipe_mgr_sel_grp_mbrs_count(
      sel_tbl, sel_grp_stage_info, mbr_hdl, &num_mbrs);
  if (rc != PIPE_SUCCESS) return rc;

  for (mbrs = 0; mbrs < num_mbrs; mbrs++) {
    rc = pipe_mgr_sel_grp_mbr_get_offset_at_pos(
        sel_tbl, sel_grp_stage_info, mbr_hdl, mbrs, &mbr_word_idx, &mbr_idx);
    if (rc != PIPE_SUCCESS) return rc;

    if (sel_grp_stage_info->mbrs_duplicated) {
      /* The members are duplicated in each word of the group so loop over
       * all words. */
      uint32_t i = 0;
      for (; i < sel_grp_stage_info->no_words && PIPE_SUCCESS == rc; i++)
        rc = wr_one_mbr(
            sel_tbl, sel_grp_stage_info, i, mbr_idx, data, move_tail_p);
    } else {
      /* The members are not duplicated so the member is in exactly one
       * place. */
      rc = wr_one_mbr(sel_tbl,
                      sel_grp_stage_info,
                      mbr_word_idx,
                      mbr_idx,
                      data,
                      move_tail_p);
    }
  }

  return rc;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_iterate(
    sel_tbl_info_t *sel_tbl_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    const char *what,
    pipe_mgr_sel_grp_mbr_iter_func iter_func,
    void *cookie,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_t *sel_tbl = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  (void)stage_idx;

  /* Check that the member is in the group. */
  if (!pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false)) {
    LOG_ERROR("%s:%d Sel group member %d not found in group %d",
              __func__,
              __LINE__,
              sel_grp_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      rc = iter_func(sel_tbl,
                     sel_grp_stage_info,
                     sel_grp_info,
                     sel_grp_mbr_hdl,
                     cookie,
                     move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x %d.%x) stage %d selection grp %#x mbr %#x error "
            "\"%s\" in %s",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id,
            sel_tbl->pipe_id,
            sel_tbl->sel_tbl_stage_info[stage_idx].stage_id,
            sel_grp_info->grp_hdl,
            sel_grp_mbr_hdl,
            pipe_str_err(rc),
            what);
        return rc;
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_mbr_del_internal(
    sel_tbl_info_t *sel_tbl_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    pipe_sel_grp_mbr_hdl_t grp_mbr_weight,
    struct pipe_mgr_sel_move_list_t **move_tail_p,
    uint32_t *rollback_weight) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool adt_state_updated = false;
  uint32_t mbr_weight = grp_mbr_weight;
  sel_tbl_t *sel_tbl;

  /* In case of sequence order and when first member in modified list not
   * enabled net effect function will add active member from newly configured
   * group before deleting last active member, ignore the check here.
   */
  if ((!sel_tbl_info->sequence_order) && sel_grp_info->mbr_count == 1 &&
      sel_grp_info->num_references) {
    LOG_ERROR(
        "%s:%d Unable to delete last member from sel grp %d which is "
        "still referenced by match entries",
        __func__,
        __LINE__,
        sel_grp_info->grp_hdl);
    return PIPE_ENTRY_REFERENCES_EXIST;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, sel_grp_info->pipe_id);
  if (sel_tbl == NULL) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              sel_grp_info->pipe_id);
    return PIPE_UNEXPECTED;
  }

  while (mbr_weight--) {
    /* Update adt member state while checking if the member is valid */
    rc = rmt_adt_ent_non_sharable_del(sel_tbl_info->dev_id,
                                      sel_tbl_info->adt_tbl_hdl,
                                      sel_grp_mbr_hdl,
                                      sel_tbl->sess_flags);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to update adt state for mbr %d in grp %d "
          "sel tbl 0x%x device %d",
          __func__,
          __LINE__,
          sel_grp_mbr_hdl,
          sel_grp_info->grp_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      goto cleanup;
    }
    adt_state_updated = true;

    rc = pipe_mgr_sel_grp_mbr_backup_one(
        sel_tbl_info, sel_tbl, sel_grp_info, sel_grp_mbr_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error backing up group %d mbr %d rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_info->grp_hdl,
                sel_grp_mbr_hdl,
                rc);
      goto cleanup;
    }

    /* Stateful selection tables require updates per bit change while non-
     * stateful tables can update on RAM word boundaries.  Use different worker
     * functions for the two cases. */
    rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                      sel_grp_info,
                                      sel_grp_mbr_hdl,
                                      __func__,
                                      mbr_del_from_stage,
                                      NULL,
                                      move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in deleting a member from stage", __func__, __LINE__);
      goto cleanup;
    }

    rc = pipe_mgr_sel_grp_mbr_remove_and_destroy(
        sel_grp_info, sel_grp_mbr_hdl, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error removing group member %d from group htbl rc 0x%x",
                __func__,
                __LINE__,
                sel_grp_mbr_hdl,
                rc);
      goto cleanup;
    }

    sel_grp_info->mbr_count--;
    if (!sel_grp_info->mbr_count) {
      // Clear the group action if the last member was removed
      sel_grp_info->act_fn_hdl = 0;
      sel_grp_info->act_fn_set = false;
    }

    adt_state_updated = false;
  }

  return PIPE_SUCCESS;
cleanup:
  if (adt_state_updated) {
    rmt_adt_ent_non_sharable_add(sel_tbl_info->dev_id,
                                 sel_tbl_info->adt_tbl_hdl,
                                 sel_grp_mbr_hdl,
                                 NULL,
                                 sel_tbl->sess_flags);
  }

  /* when weight is more than one and one of the member is deleted from stage
   * then we need to rollback the deleted member, so get the rollback weight
   * caller function will roll back the partial deleted group members
   */
  mbr_weight = grp_mbr_weight - mbr_weight;
  if (rollback_weight && mbr_weight > 1) {
    *rollback_weight = mbr_weight - 1;
  }
  return rc;
}

/*!
 * API function to delete a member from a group of a selection table
 */
pipe_status_t rmt_sel_grp_mbr_del(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Request to delete mbr 0x%x on group 0x%x",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_grp_mbr_hdl,
      sel_grp_hdl);

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_sel_grp_mbr_del_internal(sel_tbl_info,
                                         sel_grp_info,
                                         sel_grp_mbr_hdl,
                                         1,
                                         move_head_p ? &move_tail : NULL,
                                         NULL);
  if (move_head_p) {
    *move_head_p = move_head.next;
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error deleting selector group member %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              rc);
    goto cleanup;
  }

  if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;

cleanup:
  return rc;
}

static pipe_status_t pipe_mgr_sel_validate_mbrs(
    sel_tbl_info_t *sel_tbl_info,
    sel_grp_info_t *sel_grp_info,
    pipe_adt_ent_hdl_t *mbrs,
    bool *enable,
    uint32_t num_mbrs,
    pipe_act_fn_hdl_t *act_fn_hdl_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_act_fn_hdl_t act_fn_hdl = 0, curr_act_fn_hdl = 0;
  uint32_t num_enable = 0;

  if (num_mbrs > sel_grp_info->max_grp_size) {
    LOG_ERROR(
        "%s:%d Error trying to set %d members for group %d which has "
        "max group size of %d",
        __func__,
        __LINE__,
        num_mbrs,
        sel_grp_info->grp_hdl,
        sel_grp_info->max_grp_size);
    return PIPE_NO_SPACE;
  }

  if (num_mbrs == 0) {
    if (sel_grp_info->num_references) {
      LOG_ERROR(
          "%s:%d Unable to clear sel grp %d which is still referenced "
          "by match entries",
          __func__,
          __LINE__,
          sel_grp_info->grp_hdl);
      return PIPE_INVALID_ARG;
    } else {
      return PIPE_SUCCESS;
    }
  }

  for (uint32_t i = 0; i < num_mbrs; i++) {
    if (enable[i]) {
      num_enable++;
    }
    rc = rmt_adt_ent_non_sharable_get(sel_tbl_info->dev_id,
                                      sel_tbl_info->adt_tbl_hdl,
                                      mbrs[i],
                                      &curr_act_fn_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error getting member info for handle %d to add to sel grp %d "
          "tbl 0x%x device %d",
          __func__,
          __LINE__,
          mbrs[i],
          sel_grp_info->grp_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      return rc;
    }

    if (i == 0) {
      act_fn_hdl = curr_act_fn_hdl;
    } else if (act_fn_hdl != curr_act_fn_hdl) {
      LOG_ERROR(
          "%s:%d Unexpected action function %d for member %d in membership "
          "set with action function %d for sel grp %d table 0x%x device %d",
          __func__,
          __LINE__,
          curr_act_fn_hdl,
          mbrs[i],
          act_fn_hdl,
          sel_grp_info->grp_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      return PIPE_INVALID_ARG;
    }
  }

  if (num_enable == 0 && sel_grp_info->num_references) {
    LOG_ERROR(
        "%s:%d Cannot fully disable sel grp %d which is still referenced by "
        "match entries",
        __func__,
        __LINE__,
        sel_grp_info->grp_hdl);
    return PIPE_INVALID_ARG;
  }

  if (sel_grp_info->act_fn_hdl != act_fn_hdl && sel_grp_info->num_references) {
    LOG_ERROR(
        "%s:%d Unable to change action of sel grp %d which is still referenced "
        "by match entries",
        __func__,
        __LINE__,
        sel_grp_info->grp_hdl);
    return PIPE_INVALID_ARG;
  }

  *act_fn_hdl_p = act_fn_hdl;
  return PIPE_SUCCESS;
}

/**
 * Function to get the weight of the input array of member
 */
static uint32_t get_mbr_weight(pipe_adt_ent_hdl_t *mbrs,
                               pipe_adt_ent_hdl_t mbr,
                               int count) {
  int i = 0, weight = 0;
  for (i = 0; i < count; i++) {
    if (mbrs[i] == mbr) weight++;
  }
  return weight;
}

/**
 * Function to check if the mbr hdl is already present in selector grp
 */

static bool mbr_hdl_check_dup(sel_grp_mbr_t *mbrs,
                              pipe_adt_ent_hdl_t hdl,
                              int count,
                              bool update_weight) {
  int i;

  for (i = 0; i < count; i++) {
    if (mbrs[i].mbr_hdl == hdl) {
      if (update_weight) mbrs[i].weight++;
      return true;
    }
  }

  return false;
}

/**
 * Function to get the difference of members which needs to be configured in
 * selector group, it will compare the existing members with the new members
 * to make sure that the members are in order.
 */

static pipe_status_t pipe_mgr_sel_get_mbr_diff_inorder(
    sel_grp_info_t *sel_grp_info,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    bf_dev_id_t dev_id,
    pipe_adt_ent_hdl_t *mbrs,
    bool *enable,
    uint32_t num_mbrs,
    sel_grp_mbr_t *del_mbrs,
    uint32_t *num_del_mbrs,
    sel_grp_mbr_t *add_mbrs,
    uint32_t *num_add_mbrs,
    uint32_t *num_old_enb) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;
  uint32_t index = 0;
  uint32_t num_del_enb = 0;
  uint32_t last_del_enb = 0;
  pipe_adt_ent_hdl_t *conf_mbrs = NULL;
  bool *conf_enable = NULL;
  uint32_t mbrs_populated = 0;

  sel_grp_mbr_t temp_mbr;
  *num_del_mbrs = 0;
  *num_add_mbrs = 0;

  /* Get existing members in sequence */
  uint32_t conf_mbr_count = sel_grp_info->mbr_count;

  if (conf_mbr_count) {
    conf_mbrs = PIPE_MGR_CALLOC(conf_mbr_count, sizeof(pipe_adt_ent_hdl_t));
    conf_enable = PIPE_MGR_CALLOC(conf_mbr_count, sizeof(bool));
    rc = rmt_sel_grp_mbrs_get(dev_id,
                              sel_tbl_hdl,
                              sel_grp_hdl,
                              conf_mbr_count,
                              conf_mbrs,
                              conf_enable,
                              &mbrs_populated);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error getting members for sel grp %d"
          "tbl_hdl 0x%x rc 0x%x",
          __func__,
          __LINE__,
          sel_grp_hdl,
          sel_tbl_hdl,
          rc);
      if (conf_mbrs) PIPE_MGR_FREE(conf_mbrs);
      if (conf_enable) PIPE_MGR_FREE(conf_enable);
      return rc;
    }
  }

  for (index = 0; index < num_mbrs && index < conf_mbr_count &&
                  (conf_mbr_count < sel_grp_info->entries_per_word &&
                   num_mbrs < sel_grp_info->entries_per_word);
       index++) {
    if (conf_mbrs[index] != mbrs[index]) break;

    if (conf_enable[index] == true) (*num_old_enb)++;

    if (conf_enable[index] != enable[index]) {
      add_mbrs[*num_add_mbrs].mbr_hdl = mbrs[index];
      add_mbrs[*num_add_mbrs].state = enable[index]
                                          ? PIPE_MGR_GRP_MBR_STATE_ACTIVE
                                          : PIPE_MGR_GRP_MBR_STATE_INACTIVE;
      add_mbrs[*num_add_mbrs].is_backup_valid = true;
      add_mbrs[*num_add_mbrs].weight++;
      (*num_add_mbrs)++;
    }
  }

  /* Delete the members from last, so that it will not impact the sequence */
  for (i = conf_mbr_count; conf_mbr_count && i > index; i--) {
    if (conf_enable[i - 1] == true) {
      (*num_old_enb)++;
      num_del_enb++;
      last_del_enb = *num_del_mbrs;
    }
    del_mbrs[*num_del_mbrs].mbr_hdl = conf_mbrs[i - 1];
    del_mbrs[*num_del_mbrs].state = conf_enable[i - 1]
                                        ? PIPE_MGR_GRP_MBR_STATE_ACTIVE
                                        : PIPE_MGR_GRP_MBR_STATE_INACTIVE;
    del_mbrs[*num_del_mbrs].weight++;
    (*num_del_mbrs)++;
  }

  /* Group should have atleast one active members, so delete final active
     members at last */
  if (num_del_enb && num_del_enb == *num_old_enb &&
      last_del_enb != (*num_del_mbrs - 1)) {
    memcpy(&temp_mbr, &del_mbrs[*num_del_mbrs - 1], sizeof(sel_grp_mbr_t));
    memcpy(&del_mbrs[*num_del_mbrs - 1],
           &del_mbrs[last_del_enb],
           sizeof(sel_grp_mbr_t));
    memcpy(&del_mbrs[last_del_enb], &temp_mbr, sizeof(sel_grp_mbr_t));
  }

  for (; index < num_mbrs; index++) {
    add_mbrs[*num_add_mbrs].mbr_hdl = mbrs[index];
    add_mbrs[*num_add_mbrs].state = enable[index]
                                        ? PIPE_MGR_GRP_MBR_STATE_ACTIVE
                                        : PIPE_MGR_GRP_MBR_STATE_INACTIVE;
    add_mbrs[*num_add_mbrs].is_backup_valid = false;
    add_mbrs[*num_add_mbrs].weight++;
    (*num_add_mbrs)++;
  }

  if (conf_mbrs) PIPE_MGR_FREE(conf_mbrs);
  if (conf_enable) PIPE_MGR_FREE(conf_enable);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_get_mbr_diff(sel_grp_info_t *sel_grp_info,
                                               pipe_adt_ent_hdl_t *mbrs,
                                               bool *enable,
                                               uint32_t num_mbrs,
                                               sel_grp_mbr_t *del_mbrs,
                                               uint32_t *num_del_mbrs,
                                               sel_grp_mbr_t *add_mbrs,
                                               uint32_t *num_add_mbrs,
                                               uint32_t *num_old_enb) {
  Word_t mbr_hdl = 0;
  PWord_t Pvalue = NULL;
  sel_grp_mbr_t *mbr = NULL;
  uint32_t i = 0;
  enum pipe_mgr_grp_mbr_state_e state;
  *num_del_mbrs = 0;
  *num_add_mbrs = 0;
  uint32_t weight;
  uint32_t tmp;

  /* Delete all members in group but not in membership list */
  JLF(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  while (Pvalue && Pvalue != PJERR) {
    mbr = (sel_grp_mbr_t *)*Pvalue;
    if (!mbr) {
      PIPE_MGR_ASSERT(0);
      return PIPE_UNEXPECTED;
    }
    for (i = 0; i < num_mbrs; i++) {
      if (mbrs[i] == (pipe_adt_ent_hdl_t)mbr_hdl) {
        /* Check if there is (decrese) change in weight */
        weight = get_mbr_weight(mbrs, mbrs[i], num_mbrs);
        if (mbr->weight > weight) {
          tmp = mbr->weight - weight;
          if (tmp) {
            del_mbrs[*num_del_mbrs] = *mbr;
            del_mbrs[*num_del_mbrs].weight = tmp;
            (*num_del_mbrs)++;
          }
        }
        break;
      }
    }
    if (i == num_mbrs) {
      del_mbrs[*num_del_mbrs] = *mbr;
      (*num_del_mbrs)++;
    }
    if (mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      (*num_old_enb) += mbr->weight;
    }
    JLN(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  }
  if (Pvalue == PJERR) {
    return PIPE_UNEXPECTED;
  }

  /* Add all members in membership list but not in group, or in group with
   * a different activation state
   */
  for (i = 0; i < num_mbrs; i++) {
    JLG(Pvalue, sel_grp_info->sel_grp_mbrs, (Word_t)mbrs[i]);
    if (Pvalue == PJERR) {
      return PIPE_UNEXPECTED;
    }
    state = enable[i] ? PIPE_MGR_GRP_MBR_STATE_ACTIVE
                      : PIPE_MGR_GRP_MBR_STATE_INACTIVE;
    mbr = Pvalue ? (sel_grp_mbr_t *)*Pvalue : NULL;

    if (!mbr || mbr->state != state) {
      if (mbr_hdl_check_dup(add_mbrs, mbrs[i], i, 1)) {
        continue;
      }

      add_mbrs[*num_add_mbrs].mbr_hdl = mbrs[i];
      add_mbrs[*num_add_mbrs].state = state;
      add_mbrs[*num_add_mbrs].is_backup_valid = (mbr != NULL);
      add_mbrs[*num_add_mbrs].weight++;
      (*num_add_mbrs)++;
    } else if (mbr) {
      weight = get_mbr_weight(mbrs, mbrs[i], num_mbrs);
      if ((weight > mbr->weight) &&
          !(mbr_hdl_check_dup(add_mbrs, mbrs[i], num_mbrs, 0))) {
        add_mbrs[*num_add_mbrs].mbr_hdl = mbrs[i];
        add_mbrs[*num_add_mbrs].state = mbr->state;
        add_mbrs[*num_add_mbrs].is_backup_valid = false;
        add_mbrs[*num_add_mbrs].weight = (weight - mbr->weight);
        (*num_add_mbrs)++;
      }
    }
  }

  return PIPE_SUCCESS;
}

static void pipe_mgr_sel_grp_rollback_mbrs_set(sel_tbl_info_t *sel_tbl_info,
                                               sel_grp_info_t *sel_grp_info,
                                               sel_grp_mbr_t *del_mbrs,
                                               uint32_t num_del,
                                               sel_grp_mbr_t *add_mbrs,
                                               uint32_t num_add,
                                               int enable_add_idx) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint8_t data = 0;
  uint32_t total_mbrs = 0;

  /* First rollback any new/edited members */
  for (; num_add > 0; num_add--) {
    if ((uint32_t)enable_add_idx == num_add) {
      /* Skip the special member for now */
      continue;
    }
    if (!add_mbrs[num_add - 1].is_backup_valid) {
      rc = pipe_mgr_sel_grp_mbr_del_internal(sel_tbl_info,
                                             sel_grp_info,
                                             add_mbrs[num_add - 1].mbr_hdl,
                                             add_mbrs[num_add - 1].weight,
                                             NULL,
                                             NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error deleting new member %d from sel grp %d during "
            "membership rollback in table 0x%x device %d",
            __func__,
            __LINE__,
            add_mbrs[num_add - 1].mbr_hdl,
            sel_grp_info->grp_hdl,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id);
        PIPE_MGR_DBGCHK(0);
        return;
      }
    } else {
      data = (add_mbrs[num_add - 1].state == PIPE_MGR_GRP_MBR_STATE_INACTIVE);
      rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                        sel_grp_info,
                                        add_mbrs[num_add - 1].mbr_hdl,
                                        __func__,
                                        mbr_enable_disable_in_stage,
                                        &data,
                                        NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error rolling back member %d in sel grp %d in table "
            "0x%x device %d",
            __func__,
            __LINE__,
            add_mbrs[num_add - 1].mbr_hdl,
            sel_grp_info->grp_hdl,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id);
        PIPE_MGR_DBGCHK(0);
        return;
      }
      pipe_mgr_sel_grp_mbr_set_state(
          sel_grp_info, add_mbrs[num_add - 1].mbr_hdl, data);
    }
  }

  total_mbrs = sel_grp_info->mbr_count + num_del;
  for (; num_del > 0; num_del--) {
    rc = pipe_mgr_sel_grp_mbr_add_internal(sel_tbl_info,
                                           sel_grp_info,
                                           sel_grp_info->act_fn_hdl,
                                           del_mbrs[num_del - 1].mbr_hdl,
                                           del_mbrs[num_del - 1].weight,
                                           total_mbrs,
                                           NULL,
                                           NULL,
                                           false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error adding back member %d to sel grp %d during "
          "membership rollback in table 0x%x device %d",
          __func__,
          __LINE__,
          del_mbrs[num_del - 1].mbr_hdl,
          sel_grp_info->grp_hdl,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return;
    }
    if (del_mbrs[num_del - 1].state == PIPE_MGR_GRP_MBR_STATE_INACTIVE) {
      data = 0;
      rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                        sel_grp_info,
                                        del_mbrs[num_del - 1].mbr_hdl,
                                        __func__,
                                        mbr_enable_disable_in_stage,
                                        &data,
                                        NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error rolling back member %d in sel grp %d in table "
            "0x%x device %d",
            __func__,
            __LINE__,
            del_mbrs[num_del - 1].mbr_hdl,
            sel_grp_info->grp_hdl,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id);
        PIPE_MGR_DBGCHK(0);
        return;
      }
      pipe_mgr_sel_grp_mbr_set_state(
          sel_grp_info, del_mbrs[num_del - 1].mbr_hdl, data);
    } else if (enable_add_idx != -1) {
      /* Roll back the special member now that we have an active member */
      if (!add_mbrs[enable_add_idx].is_backup_valid) {
        rc = pipe_mgr_sel_grp_mbr_del_internal(sel_tbl_info,
                                               sel_grp_info,
                                               add_mbrs[enable_add_idx].mbr_hdl,
                                               add_mbrs[enable_add_idx].weight,
                                               NULL,
                                               NULL);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error deleting new member %d from sel grp %d during "
              "membership rollback in table 0x%x device %d",
              __func__,
              __LINE__,
              add_mbrs[enable_add_idx].mbr_hdl,
              sel_grp_info->grp_hdl,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id);
          PIPE_MGR_DBGCHK(0);
          return;
        }
      } else {
        data = 0;
        rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                          sel_grp_info,
                                          add_mbrs[enable_add_idx].mbr_hdl,
                                          __func__,
                                          mbr_enable_disable_in_stage,
                                          &data,
                                          NULL);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error rolling back member %d in sel grp %d in table "
              "0x%x device %d",
              __func__,
              __LINE__,
              add_mbrs[enable_add_idx].mbr_hdl,
              sel_grp_info->grp_hdl,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id);
          PIPE_MGR_DBGCHK(0);
          return;
        }
        pipe_mgr_sel_grp_mbr_set_state(
            sel_grp_info, add_mbrs[enable_add_idx].mbr_hdl, data);
      }
      enable_add_idx = -1;
    }
  }

  return;
}

/**
 * Function will reorder the move list to make sure we add new active member
 * (add_hdl) from the modified group before remove last active (last_del)
 * member from existing group.
 */

void pipe_mgr_sel_move_list_net_effect(
    pipe_sel_grp_mbr_hdl_t last_del,
    pipe_sel_grp_mbr_hdl_t add_hdl,
    struct pipe_mgr_sel_move_list_t *move_head) {
  struct pipe_mgr_sel_move_list_t *ml = move_head->next;
  struct pipe_mgr_sel_move_list_t *add_node = NULL, *del_node = NULL;
  ;
  struct pipe_mgr_sel_move_list_t *prev = NULL, *del_prev = NULL;

  /* Find last instanance of del handle (wcmp case) in the list */
  if (ml->adt_mbr_hdl == last_del && ml->op == PIPE_SEL_UPDATE_DEL) {
    del_prev = move_head;
  }

  prev = move_head;
  while (ml) {
    if (ml->adt_mbr_hdl == last_del && ml->op == PIPE_SEL_UPDATE_DEL) {
      del_prev = prev;
      del_node = ml;
    }
    prev = ml;
    ml = ml->next;
  }
  if (!del_node) {
    LOG_ERROR("%s:%d Failed to find selector group member %d",
              __func__,
              __LINE__,
              last_del);
    return;
  }
  add_node = del_node;
  prev = add_node;

  while (add_node) {
    if (add_node->adt_mbr_hdl == add_hdl && add_node->op == PIPE_SEL_UPDATE_ADD)
      break;
    prev = add_node;
    add_node = add_node->next;
  }

  if (!add_node) {
    LOG_ERROR("%s:%d Failed to add selector group member %d",
              __func__,
              __LINE__,
              add_hdl);
    return;
  }

  prev->next = add_node->next;
  add_node->next = del_node;
  del_prev->next = add_node;

  return;
}

/*!
 * API function to set membership of a group
 */
pipe_status_t rmt_sel_grp_mbrs_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint32_t num_mbrs,
    pipe_adt_ent_hdl_t *mbrs,
    bool *enable,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_list) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_mbr_t *del_mbrs = NULL, *add_mbrs = NULL;
  uint32_t num_del_mbrs = 0, num_add_mbrs = 0, num_old_enb = 0;
  uint32_t del_idx = 0, add_idx = 0;
  int enable_add_idx = -1;
  uint32_t i = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  uint8_t data = 0;
  uint32_t rollback_weight = 0;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;
  pipe_sel_grp_mbr_hdl_t last_del_hdl = PIPE_ADT_ENT_HDL_INVALID_HDL;
  pipe_sel_grp_mbr_hdl_t add_mbr_hdl = PIPE_ADT_ENT_HDL_INVALID_HDL;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_sel_validate_mbrs(
      sel_tbl_info, sel_grp_info, mbrs, enable, num_mbrs, &act_fn_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Invalid membership list for sel grp %d in table 0x%x device %d",
        __func__,
        __LINE__,
        sel_grp_hdl,
        sel_tbl_hdl,
        device_id);
    return rc;
  }

  del_mbrs = PIPE_MGR_CALLOC(sel_grp_info->mbr_count, sizeof(sel_grp_mbr_t));
  if (del_mbrs == NULL) {
    LOG_ERROR("%s:%d Failed to allocate memory", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  add_mbrs = PIPE_MGR_CALLOC(num_mbrs, sizeof(sel_grp_mbr_t));
  if (add_mbrs == NULL) {
    LOG_ERROR("%s:%d Failed to allocate memory", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  if (sel_tbl_info->sequence_order) {
    rc = pipe_mgr_sel_get_mbr_diff_inorder(sel_grp_info,
                                           sel_tbl_hdl,
                                           sel_grp_hdl,
                                           device_id,
                                           mbrs,
                                           enable,
                                           num_mbrs,
                                           del_mbrs,
                                           &num_del_mbrs,
                                           add_mbrs,
                                           &num_add_mbrs,
                                           &num_old_enb);
  } else {
    rc = pipe_mgr_sel_get_mbr_diff(sel_grp_info,
                                   mbrs,
                                   enable,
                                   num_mbrs,
                                   del_mbrs,
                                   &num_del_mbrs,
                                   add_mbrs,
                                   &num_add_mbrs,
                                   &num_old_enb);
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Failed to calculate the membership diff for sel grp %d in table "
        "0x%x device %d",
        __func__,
        __LINE__,
        sel_grp_hdl,
        sel_tbl_hdl,
        device_id);
    goto cleanup;
  }

  for (; del_idx < num_del_mbrs; del_idx++) {
    PIPE_MGR_DBGCHK(num_old_enb > 0 ||
                    del_mbrs[del_idx].state == PIPE_MGR_GRP_MBR_STATE_INACTIVE);
    /* Ensure we are not left with an empty group if there are match refs */
    if (sel_grp_info->num_references && num_old_enb == 1 &&
        del_mbrs[del_idx].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      if (sel_tbl_info->sequence_order &&
          add_mbrs[0].mbr_hdl == del_mbrs[del_idx].mbr_hdl &&
          add_mbrs[0].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
        /* Avoid adding and deleting the same member in group */
        enable_add_idx = 0;
        continue;
      }

      for (i = 0; i < num_add_mbrs; i++) {
        if (add_mbrs[i].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
          if (add_mbrs[i].is_backup_valid) {
            data = 1;
            rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                              sel_grp_info,
                                              add_mbrs[i].mbr_hdl,
                                              __func__,
                                              mbr_enable_disable_in_stage,
                                              &data,
                                              move_list ? &move_tail : NULL);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error activating member %d in sel grp %d in table "
                  "0x%x device %d",
                  __func__,
                  __LINE__,
                  add_mbrs[i].mbr_hdl,
                  sel_grp_hdl,
                  sel_tbl_hdl,
                  device_id);
              goto rollback;
            }
            pipe_mgr_sel_grp_mbr_set_state(
                sel_grp_info, add_mbrs[i].mbr_hdl, data);
          } else {
            if (sel_tbl_info->sequence_order && i != 0) {
              /* Dont add the member here, it will change the sequence */
              last_del_hdl = del_mbrs[del_idx].mbr_hdl;
              add_mbr_hdl = add_mbrs[i].mbr_hdl;
              break;
            }
            /*some callers like grp_resize does not require backup
             * so check if this call requires backup.
             */
            bool skip_backup = pipe_api_flags & PIPE_FLAG_SKIP_BACKUP;
            rc =
                pipe_mgr_sel_grp_mbr_add_internal(sel_tbl_info,
                                                  sel_grp_info,
                                                  act_fn_hdl,
                                                  add_mbrs[i].mbr_hdl,
                                                  add_mbrs[i].weight,
                                                  num_mbrs,
                                                  move_list ? &move_tail : NULL,
                                                  &rollback_weight,
                                                  skip_backup);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error adding member %d to sel grp %d in table 0x%x "
                  "device %d",
                  __func__,
                  __LINE__,
                  add_mbrs[i].mbr_hdl,
                  sel_grp_hdl,
                  sel_tbl_hdl,
                  device_id);
              /* In case of duplicate grp member (weight > 1) if any of the
               * member is added then it need to be rolled back, so get the
               * weight of the members which need to be rolled back.
               */
              if (rollback_weight) {
                add_mbrs[i].weight = rollback_weight;
                enable_add_idx = i;
              }
              goto rollback;
            }
          }
          enable_add_idx = i;
          break;
        }
      }
    }

    rc = pipe_mgr_sel_grp_mbr_del_internal(sel_tbl_info,
                                           sel_grp_info,
                                           del_mbrs[del_idx].mbr_hdl,
                                           del_mbrs[del_idx].weight,
                                           move_list ? &move_tail : NULL,
                                           &rollback_weight);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error deleting member %d from sel grp %d in table 0x%x "
          "device %d",
          __func__,
          __LINE__,
          del_mbrs[del_idx].mbr_hdl,
          sel_grp_hdl,
          sel_tbl_hdl,
          device_id);
      if (rollback_weight) {
        del_mbrs[del_idx].weight = rollback_weight;
        del_idx++;
      }
      goto rollback;
    }

    if (del_mbrs[del_idx].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      num_old_enb--;
    }
  }

  // After removing old members there are 3 possibilities:
  // 1. Group is empty, act_fn is not set
  // 2. Group is not empty and act_fn did not change
  // 3. Group is not empty and act_fn did change - in this case old members
  //    must be used, but their action was updated in the action table,
  //    otherwise validate function will throw an error. In this case there are
  //    no members to add, but act_fn is still set to old value, hence update
  //    it.
  sel_grp_info->act_fn_hdl = act_fn_hdl;
  for (; add_idx < num_add_mbrs; add_idx++) {
    if ((uint32_t)enable_add_idx == add_idx) {
      continue;
    }
    if (!add_mbrs[add_idx].is_backup_valid) {
      if (pipe_mgr_hitless_warm_init_in_progress(device_id)) {
        bf_map_sts_t msts;
        uint32_t pipe_idx = 0;
        uint32_t weight = 0;
        for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
          sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
          pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info =
              sel_tbl->hlp.ha_hlp_info;
          pipe_mgr_sel_grp_replay_info_t *grp_replay_info;
          msts = bf_map_get(&ha_hlp_info->replay_hdl_to_info,
                            sel_grp_hdl,
                            (void **)&grp_replay_info);
          if (msts != BF_MAP_OK) {
            continue;
          }
          if (!grp_replay_info->act_fn_hdl) {
            grp_replay_info->act_fn_hdl = act_fn_hdl;
          } else {
            PIPE_MGR_ASSERT(grp_replay_info->act_fn_hdl == act_fn_hdl);
          }
          if (grp_replay_info->num_mbrs == grp_replay_info->max_grp_size) {
            LOG_ERROR("%s:%d Group %d is already full",
                      __func__,
                      __LINE__,
                      sel_grp_hdl);
            return PIPE_NO_SPACE;
          }
          weight = add_mbrs[add_idx].weight;
          while (weight--) {
            grp_replay_info->mbr_hdls[grp_replay_info->num_mbrs] =
                add_mbrs[add_idx].mbr_hdl;
            grp_replay_info->mbr_enable[grp_replay_info->num_mbrs] =
                (add_mbrs[add_idx].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE)
                    ? true
                    : false;
            grp_replay_info->mbr_weight[grp_replay_info->num_mbrs] = 1;
            grp_replay_info->num_mbrs++;
          }
        }
      }
      /*some callers like grp_resize does not require backup
       * so check if this call requires backup.
       */
      bool skip_backup = pipe_api_flags & PIPE_FLAG_SKIP_BACKUP;
      /* Add new member to group. */
      rc = pipe_mgr_sel_grp_mbr_add_internal(sel_tbl_info,
                                             sel_grp_info,
                                             act_fn_hdl,
                                             add_mbrs[add_idx].mbr_hdl,
                                             add_mbrs[add_idx].weight,
                                             num_mbrs,
                                             move_list ? &move_tail : NULL,
                                             &rollback_weight,
                                             skip_backup);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error adding member %d to sel grp %d in table 0x%x "
            "device %d",
            __func__,
            __LINE__,
            add_mbrs[add_idx].mbr_hdl,
            sel_grp_hdl,
            sel_tbl_hdl,
            device_id);
        /* In case of duplicate grp member (weight > 1) if any of the
         * member is added then it need to be rolled back, so get the
         * weight of the members which need to be rolled back.
         */

        if (rollback_weight) {
          add_mbrs[add_idx].weight = rollback_weight;
          add_idx++;
        }
        goto rollback;
      }
    } else if (add_mbrs[add_idx].state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      /* Activate instead if member already exists in group. */
      data = 1;
      rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                        sel_grp_info,
                                        add_mbrs[add_idx].mbr_hdl,
                                        __func__,
                                        mbr_enable_disable_in_stage,
                                        &data,
                                        move_list ? &move_tail : NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error activating member %d in sel grp %d in table 0x%x "
            "device %d",
            __func__,
            __LINE__,
            add_mbrs[add_idx].mbr_hdl,
            sel_grp_hdl,
            sel_tbl_hdl,
            device_id);
        goto rollback;
      }
      pipe_mgr_sel_grp_mbr_set_state(
          sel_grp_info, add_mbrs[add_idx].mbr_hdl, data);
    }
    /* Whether or not the member did exist in the group, deactivate
     * if needed.
     */
    if (add_mbrs[add_idx].state == PIPE_MGR_GRP_MBR_STATE_INACTIVE) {
      data = 0;
      rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                        sel_grp_info,
                                        add_mbrs[add_idx].mbr_hdl,
                                        __func__,
                                        mbr_enable_disable_in_stage,
                                        &data,
                                        move_list ? &move_tail : NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error activating member %d in sel grp %d in table 0x%x "
            "device %d",
            __func__,
            __LINE__,
            add_mbrs[add_idx].mbr_hdl,
            sel_grp_hdl,
            sel_tbl_hdl,
            device_id);
        goto rollback;
      }
      pipe_mgr_sel_grp_mbr_set_state(
          sel_grp_info, add_mbrs[add_idx].mbr_hdl, data);
    }
  }

  if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
    rc = PIPE_UNEXPECTED;
    goto rollback;
  }

  rc = PIPE_SUCCESS;
  goto cleanup;

rollback:
  pipe_mgr_sel_grp_rollback_mbrs_set(sel_tbl_info,
                                     sel_grp_info,
                                     del_mbrs,
                                     del_idx,
                                     add_mbrs,
                                     add_idx,
                                     enable_add_idx);
cleanup:
  if (move_list) {
    if (sel_tbl_info->sequence_order &&
        last_del_hdl != PIPE_ADT_ENT_HDL_INVALID_HDL) {
      /* Modify move_head to make sure Add before delete for last active member
       */
      pipe_mgr_sel_move_list_net_effect(last_del_hdl, add_mbr_hdl, &move_head);
    }
    *move_list = move_head.next;
  }
  if (del_mbrs) {
    PIPE_MGR_FREE(del_mbrs);
  }
  if (add_mbrs) {
    PIPE_MGR_FREE(add_mbrs);
  }
  return rc;
}

static pipe_status_t update_duplicate_mbrs(sel_grp_mbr_t *mbr,
                                           pipe_adt_ent_hdl_t *mbrs,
                                           bool *enable,
                                           uint32_t *mbrs_populated,
                                           uint32_t mbrs_size,
                                           bool sequence_order) {
  uint32_t count;
  sel_mbr_pos_t *temp = NULL;

  if ((!mbr) || (!mbrs) || (!enable) || (!mbrs_populated)) {
    LOG_ERROR("%s:%d Invalid arguments", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (sequence_order) {
    temp = mbr->mbrs_pos->next;
    for (count = 2; count <= mbr->weight && *mbrs_populated < mbrs_size && temp;
         count++) {
      mbrs[temp->pos] = mbr->mbr_hdl;
      enable[temp->pos] = mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE;
      (*mbrs_populated)++;
      temp = temp->next;
    }
  } else {
    for (count = 2; count <= mbr->weight && *mbrs_populated < mbrs_size;
         count++) {
      mbrs[*mbrs_populated] = mbr->mbr_hdl;
      enable[*mbrs_populated] = mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE;
      (*mbrs_populated)++;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_sel_grp_mbrs_get(bf_dev_id_t device_id,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   pipe_sel_grp_hdl_t sel_grp_hdl,
                                   uint32_t mbrs_size,
                                   pipe_adt_ent_hdl_t *mbrs,
                                   bool *enable,
                                   uint32_t *mbrs_populated) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  Word_t mbr_hdl = 0;
  PWord_t Pvalue = NULL;
  sel_grp_mbr_t *mbr = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *mbrs_populated = 0;
  JLF(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  while (Pvalue && Pvalue != PJERR && *mbrs_populated < mbrs_size) {
    if (!sel_tbl_info->sequence_order) {
      mbr = (sel_grp_mbr_t *)*Pvalue;
      mbrs[*mbrs_populated] = mbr->mbr_hdl;
      enable[*mbrs_populated] = mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE;
      (*mbrs_populated)++;
      if (mbr->weight > 1) {
        rc = update_duplicate_mbrs(mbr,
                                   mbrs,
                                   enable,
                                   mbrs_populated,
                                   mbrs_size,
                                   sel_tbl_info->sequence_order);
        if (rc != PIPE_SUCCESS) {
          return rc;
        }
      }
    } else {
      mbr = (sel_grp_mbr_t *)*Pvalue;
      if (!mbr->mbrs_pos) {
        LOG_ERROR(
            "%s:%d %s(0x%x-%d) Selector group 0x%x handle %d position not "
            "found",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id,
            sel_grp_hdl,
            mbr->mbr_hdl);
        return PIPE_UNEXPECTED;
      }
      mbrs[mbr->mbrs_pos->pos] = mbr->mbr_hdl;
      enable[mbr->mbrs_pos->pos] = mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE;
      (*mbrs_populated)++;
      if (mbr->weight > 1) {
        rc = update_duplicate_mbrs(mbr,
                                   mbrs,
                                   enable,
                                   mbrs_populated,
                                   mbrs_size,
                                   sel_tbl_info->sequence_order);
        if (rc != PIPE_SUCCESS) {
          return rc;
        }
      }
    }
    JLN(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  }
  if (Pvalue == PJERR) {
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

/* API function to disable a group member of a selection table */
pipe_status_t rmt_sel_grp_mbr_disable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;
  uint8_t data = 0;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE(
      "%s:%d - %s (%d - 0x%x) "
      "Request to disable mbr 0x%x on group 0x%x",
      __func__,
      __LINE__,
      sel_tbl_info->name,
      sel_tbl_info->dev_id,
      sel_tbl_info->tbl_hdl,
      sel_grp_mbr_hdl,
      sel_grp_hdl);

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  rc = pipe_mgr_sel_grp_mbr_backup_one(
      sel_tbl_info, sel_tbl, sel_grp_info, sel_grp_mbr_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error backing up group %d mbr %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl,
              sel_grp_mbr_hdl,
              rc);
    return rc;
  }

  sel_grp_mbr_t *mbr =
      pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  if (!mbr) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Sel group member %d not found in group %d",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_grp_mbr_hdl,
        sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  /* Stateful selection tables require updates per bit change while non-
   * stateful tables can update on RAM word boundaries.  Use different worker
   * functions for the two cases. */
  rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                    sel_grp_info,
                                    sel_grp_mbr_hdl,
                                    __func__,
                                    mbr_enable_disable_in_stage,
                                    &data,
                                    move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  mbr->state = PIPE_MGR_GRP_MBR_STATE_INACTIVE;
  if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
    return PIPE_UNEXPECTED;
  }

  PIPE_MGR_SEL_TBL_ASSERT(dev_id, sel_tbl_hdl);
  return PIPE_SUCCESS;
}

/* API function to re-enable a group member of a selection table */
pipe_status_t rmt_sel_grp_mbr_enable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t sel_grp_mbr_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;
  uint8_t data = 1;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("%s:%d - %s (%d - 0x%x) Request to enable mbr 0x%x on group 0x%x",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->dev_id,
            sel_tbl_info->tbl_hdl,
            sel_grp_mbr_hdl,
            sel_grp_hdl);

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  rc = pipe_mgr_sel_grp_mbr_backup_one(
      sel_tbl_info, sel_tbl, sel_grp_info, sel_grp_mbr_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error backing up group %d mbr %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_info->grp_hdl,
              sel_grp_mbr_hdl,
              rc);
    return rc;
  }

  sel_grp_mbr_t *mbr =
      pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  if (!mbr) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Sel group member %d not found in group %d",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  /* Stateful selection tables require updates per bit change while non-
   * stateful tables can update on RAM word boundaries.  Use different worker
   * functions for the two cases. */
  rc = pipe_mgr_sel_grp_mbr_iterate(sel_tbl_info,
                                    sel_grp_info,
                                    sel_grp_mbr_hdl,
                                    __func__,
                                    mbr_enable_disable_in_stage,
                                    &data,
                                    move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  mbr->state = PIPE_MGR_GRP_MBR_STATE_ACTIVE;
  if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
    return PIPE_UNEXPECTED;
  }

  PIPE_MGR_SEL_TBL_ASSERT(dev_id, sel_tbl_hdl);
  return PIPE_SUCCESS;
}

/* Activate a selection table group in a stage */
pipe_status_t rmt_sel_grp_activate_stage(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint8_t stage_id,
    rmt_virt_addr_t *grp_act_data_set_base_p,
    uint32_t *grp_port_vector_set_len_p,
    pipe_idx_t *grp_port_vector_set_base_p,
    uint32_t pipe_api_flags) {
  uint32_t stage_idx = 0, i = 0;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_t *sel_tbl_inst = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_stage_info_t *sel_stage = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl_inst = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl_inst) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl_inst->cur_sess_hdl = sess_hdl;
  sel_tbl_inst->sess_flags = pipe_api_flags;

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl_inst, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (sel_grp_info->num_active_mbrs == 0) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) Unable to activate selector group 0x%x, "
        "there are no active members.",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        sel_grp_hdl);
    return PIPE_INVALID_ARG;
  }

  /* Check if it has already been activated earlier */
  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      sel_stage = &sel_tbl->sel_tbl_stage_info[stage_idx];
      if (sel_stage->stage_id == stage_id) {
        break;
      }
    }
  }

  if (!sel_grp_stage_info) {
    for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
      sel_tbl = &sel_tbl_info->sel_tbl[i];

      stage_idx = pipe_mgr_sel_tbl_get_stage_idx(sel_tbl, stage_id);
      if (stage_idx == (uint32_t)-1) {
        rc = PIPE_INVALID_ARG;
        break;
      }

      rc = pipe_mgr_sel_grp_activate_stage_internal(
          sel_tbl,
          sel_grp_info,
          &sel_tbl->sel_tbl_stage_info[stage_idx],
          &sel_grp_stage_info,
          PIPE_INVALID_SEL_GRP_IDX,
          PIPE_MGR_LOGICAL_ACT_IDX_INVALID);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error activating grp %d in stage %d for "
            "pipe %x rc 0x%x",
            __func__,
            __LINE__,
            sel_grp_hdl,
            stage_id,
            sel_tbl->pipe_id,
            rc);
        break;
      }
    }

    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }
    if (!sel_grp_stage_info) {
      LOG_ERROR(
          "%s:%d Error activating grp %d in stage %d for "
          "rc 0x%x",
          __func__,
          __LINE__,
          sel_grp_hdl,
          stage_id,
          rc);
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
  }

  rc = pipe_mgr_sel_grp_backup_one_refcount(
      sel_tbl_info, sel_tbl_inst, sel_grp_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error backing up group %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              rc);
    goto cleanup;
  }

  *grp_act_data_set_base_p =
      sel_grp_stage_info->sel_grp_word_data[0].adt_base_idx;
  *grp_port_vector_set_len_p = sel_grp_stage_info->no_words;
  *grp_port_vector_set_base_p = sel_grp_stage_info->sel_base_idx;

  sel_grp_info->num_references++;

  return pipe_mgr_sel_update_mat_refs(sel_grp_info, mat_tbl_hdl, mat_ent_hdl);

cleanup:
  return rc;
}

/* De-activate a selection table group in a stage */
pipe_status_t rmt_sel_grp_deactivate_stage(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           bf_dev_pipe_t pipe_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           uint8_t stage_id,
                                           uint32_t pipe_api_flags) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  // no references mean resizing
  if (!sel_grp_info->num_references) return PIPE_SUCCESS;

  rc = pipe_mgr_sel_grp_backup_one_refcount(sel_tbl_info, sel_tbl, sel_grp_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error backing up group %d rc 0x%x",
              __func__,
              __LINE__,
              sel_grp_hdl,
              rc);
    return rc;
  }

  sel_grp_info->num_references--;
  bool found = false;
  sel_grp_mat_backptr_t *el = sel_grp_info->mat_tbl_list;
  while (el) {
    if (el->mat_tbl == mat_tbl_hdl) {
      found = true;
      bf_map_sts_t sts = bf_map_rmv(&el->ent_hdls, mat_ent_hdl);
      if (sts != BF_MAP_OK) {
        return sts;
      }
      break;
    }
    el = el->next;
  }
  if (!found) {
    LOG_ERROR(
        "%s:%d Missing match tbl reference sel grp %d table 0x%x hdl 0x%x",
        __func__,
        __LINE__,
        sel_grp_hdl,
        sel_tbl_info->tbl_hdl,
        mat_tbl_hdl);
    PIPE_MGR_DBGCHK(0);
  }

#ifdef SEL_GROUP_ACTIVATE_ALL_STAGES
  (void)sess_hdl;
  (void)stage_id;
  (void)pipe_api_flags;
#else
  sel_tbl_t *sel_tbl = NULL;
  sel_grp_hw_locator_t *sel_grp_hw_locator = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;

  /* Set-up the session parameters */
  sel_tbl_info->cur_sess_hdl = sess_hdl;
  sel_tbl_info->sess_flags = pipe_api_flags;

  ble = sel_grp_info->sel_grp_hw_locator_list;
  while (ble) {
    bl_next = ble->next;
    sel_grp_hw_locator = (sel_grp_hw_locator_t *)ble->data;

    if (sel_grp_hw_locator->stage_id == stage_id) {
      if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) {
        sel_tbl = sel_tbl_info->sel_tbl;
      } else {
        sel_tbl = &sel_tbl_info->sel_tbl[sel_grp_hw_locator->pipe_id];
      }

      sel_grp_stage_info = sel_grp_hw_locator->grp_stage_p;

      rc = pipe_mgr_sel_grp_deactivate_stage_internal(
          sel_tbl,
          sel_grp_stage_info->stage_p,
          sel_grp_stage_info,
          false,
          move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Deactivate of the group %d failed "
            "rc 0x%x",
            __func__,
            __LINE__,
            sel_grp_hdl,
            rc);
        return rc;
      }

      /* Remove the hw_locator */
      BF_LIST_DLL_REM(
          sel_grp_info->sel_grp_hw_locator_list, sel_grp_info, next, prev);
      PIPE_MGR_FREE(sel_grp_hw_locator);
    }
    ble = bl_next;
  }
#endif

  return PIPE_SUCCESS;
}

pipe_action_data_spec_t *rmt_sel_grp_adt_data_get(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return NULL;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return NULL;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return NULL;
  }

  if (!sel_grp_info->sel_grp_pipe_lookup) {
    LOG_ERROR("%s:%d %s Selector table not active on any stage",
              __func__,
              __LINE__,
              sel_tbl_info->name);
    return NULL;
  }

  return NULL;
}

/** \brief pipe_mgr_sel_abort
 *        Abort a session for the given table handle
 *
 * This function should be called during abort to restore the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl sel table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_sel_abort(bf_dev_id_t dev_id,
                                 pipe_sel_tbl_hdl_t tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_info_t *bsel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_t *bsel_tbl;
  sel_tbl_t *sel_tbl;
  unsigned i, pipe;
  bool symmetric;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for %d",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (sel_tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe);
    if (!sel_tbl) {
      LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->dev_id,
                sel_tbl_info->tbl_hdl,
                pipe);
      continue;
    }

    sel_tbl->cur_sess_hdl = -1;
    sel_tbl->sess_flags = 0;

    bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, pipe);
    if (!bsel_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) Backup selector table with pipe %x "
          "not found",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->dev_id,
          sel_tbl_info->tbl_hdl,
          pipe);
      continue;
    }

    rc = pipe_mgr_sel_restore_all(sel_tbl, bsel_tbl);
  }

  return rc;
}

/** \brief pipe_mgr_sel_commit
 *        Commit the state associated with a session
 *
 * This function should be called during commit to discard the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl sel table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_sel_commit(bf_dev_id_t dev_id,
                                  pipe_sel_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_info_t *bsel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_t *bsel_tbl;
  sel_tbl_t *sel_tbl;
  unsigned i, pipe;
  bool symmetric;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bsel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, true);
  if (bsel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup sel table for 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (sel_tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe);
    if (!sel_tbl) {
      LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->dev_id,
                sel_tbl_info->tbl_hdl,
                pipe);
      continue;
    }

    sel_tbl->cur_sess_hdl = -1;
    sel_tbl->sess_flags = 0;

    bsel_tbl = get_sel_tbl_by_pipe_id(bsel_tbl_info, pipe);
    if (!bsel_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) Backup selector table with pipe %x "
          "not found",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->dev_id,
          sel_tbl_info->tbl_hdl,
          pipe);
      continue;
    }

    rc = pipe_mgr_sel_discard_all(sel_tbl, bsel_tbl);
  }

  return rc;
}

static pipe_status_t pipe_mgr_sel_mbr_assert(sel_tbl_t *sel_tbl,
                                             sel_grp_info_t *sel_grp) {
  uint32_t no_words = 0;
  sel_hlp_word_data_t *word_data = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  uint32_t word_index = 0, index = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  (void)stage_idx;
  (void)pipe_idx;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  sel_grp_mbr_t *grp_mbr = NULL;
  (void)mbr_hdl;
  (void)grp_mbr;

  JUDYL_FOREACH(sel_grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      JUDYL_FOREACH2(
          sel_grp->sel_grp_mbrs, mbr_hdl, sel_grp_mbr_t *, grp_mbr, 3) {
        rc = pipe_mgr_sel_grp_mbr_get_offset(
            sel_tbl, sel_grp_stage_info, mbr_hdl, &word_index, &index);
        PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

        no_words = sel_grp_stage_info->no_words;

        if (sel_grp_stage_info->mbrs_duplicated) {
          PIPE_MGR_DBGCHK(word_index == 0);
          for (i = 0; i < no_words; i++) {
            word_data = &sel_grp_stage_info->sel_grp_word_data[i];
            PIPE_MGR_DBGCHK(word_data->mbrs[index] == grp_mbr->mbr_hdl);
          }
        } else {
          word_data = &sel_grp_stage_info->sel_grp_word_data[word_index];
          PIPE_MGR_DBGCHK(word_data->mbrs[index] == grp_mbr->mbr_hdl);
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_grp_assert(sel_tbl_info_t *sel_tbl_info,
                                             sel_grp_info_t *sel_grp) {
  uint32_t no_pipes = 0, no_stages = 0;
  uint32_t iter_pipe_id = 0;
  uint32_t i = 0, j = 0, no_ops = 0;
  sel_tbl_t *sel_tbl = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  uint32_t no_words = 0;
  sel_hlp_word_data_t *word_data = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  sel_grp_mbr_t *grp_mbr = NULL;
  (void)mbr_hdl;
  (void)grp_mbr;

  no_pipes = sel_tbl_info->no_sel_tbls;
  no_stages = sel_tbl_info->sel_tbl->num_stages;

  uint8_t stage_check[no_pipes][no_stages];

  PIPE_MGR_MEMSET(stage_check, 0, sizeof(uint8_t) * no_pipes * no_stages);

  uint32_t mbr_count;
  JLC(mbr_count, sel_grp->sel_grp_mbrs, 0, -1);

  PIPE_MGR_DBGCHK(sel_grp->mbr_count == mbr_count);
  PIPE_MGR_DBGCHK(sel_grp->backedup_mbrs == NULL);

  {
    JUDYL_FOREACH(
        sel_grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
      sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
      JUDYL_FOREACH2(stage_lookup,
                     stage_idx,
                     sel_grp_stage_info_t *,
                     sel_grp_stage_info,
                     2) {
        stage_check[pipe_idx][stage_idx] = 1;
      }
    }
  }

#ifdef SEL_GROUP_ACTIVATE_ALL_STAGES
  /* Verify that the grp is activated in all the stages */
  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info)) {
    iter_pipe_id = 0;
    no_ops = 1;
  } else if (sel_grp->pipe_id == BF_DEV_PIPE_ALL) {
    iter_pipe_id = 0;
    no_ops = sel_tbl_info->no_sel_tbls;
  } else {
    iter_pipe_id = sel_grp->pipe_id;
    no_ops = 1;
  }

  for (i = 0; i < no_ops; i++, iter_pipe_id++) {
    sel_tbl = &sel_tbl_info->sel_tbl[iter_pipe_id];
    for (j = 0; j < sel_tbl->num_stages; j++) {
      PIPE_MGR_DBGCHK(stage_check[iter_pipe_id][j] == 1);
    }
  }
#endif

  if (!SEL_TBL_IS_SYMMETRIC(sel_tbl_info) &&
      (sel_grp->pipe_id != BF_DEV_PIPE_ALL)) {
    /* Make sure the grp is not activated in pipes to which it is not intended
     */
    for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
      if (i == sel_grp->pipe_id) {
        continue;
      }
      sel_tbl = &sel_tbl_info->sel_tbl[i];
      for (j = 0; j < sel_tbl->num_stages; j++) {
        PIPE_MGR_DBGCHK(stage_check[i][j] == 0);
      }
    }
  }

  {
    JUDYL_FOREACH(
        sel_grp->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
      sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
      JUDYL_FOREACH2(stage_lookup,
                     stage_idx,
                     sel_grp_stage_info_t *,
                     sel_grp_stage_info,
                     2) {
        PIPE_MGR_DBGCHK(sel_grp->mbr_count == sel_grp_stage_info->cur_usage);
        no_words = sel_grp_stage_info->no_words;

        if (sel_grp_stage_info->mbrs_duplicated) {
          for (i = 0; i < no_words; i++) {
            word_data = &sel_grp_stage_info->sel_grp_word_data[i];
            PIPE_MGR_DBGCHK(word_data->usage == sel_grp->mbr_count);
          }
        } else {
          for (i = 0; i < no_words; i++) {
            word_data = &sel_grp_stage_info->sel_grp_word_data[i];
            PIPE_MGR_DBGCHK(
                (word_data->usage >= sel_grp->mbr_count / no_words) &&
                (word_data->usage <= (sel_grp->mbr_count / no_words) + 1));
          }
        }
      }
    }
  }

  pipe_mgr_sel_mbr_assert(sel_tbl, sel_grp);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_sel_tbl_stage_assert(
    sel_tbl_t *sel_tbl, sel_tbl_stage_info_t *stage_info) {
  uint8_t word_alloc[stage_info->no_words];
  uint32_t i = 0;
  sel_grp_stage_info_t *grp_stage = NULL;
  uint32_t sel_base_idx = 0;

  PIPE_MGR_MEMSET(word_alloc, 0, sizeof(uint8_t) * stage_info->no_words);

  /* In each stage, make sure that inuse and free groups
   * do not overlap with each other
   * Free list should be present only if profile is set
   * Make sure the word usage and pvl_word are in sync
   */

  if (!sel_tbl->is_profile_set) {
    PIPE_MGR_DBGCHK(stage_info->sel_grp_stage_free_list == NULL);
  }
  PIPE_MGR_DBGCHK(stage_info->sel_grp_stage_backup_list == NULL);

  for (grp_stage = stage_info->sel_grp_stage_free_list; grp_stage;
       grp_stage = grp_stage->next) {
    PIPE_MGR_DBGCHK(grp_stage->stage_p == stage_info);
    sel_base_idx = grp_stage->sel_base_idx;

    PIPE_MGR_DBGCHK(sel_base_idx + grp_stage->no_words <= stage_info->no_words);
    for (i = 0; i < grp_stage->no_words; i++) {
      PIPE_MGR_DBGCHK(word_alloc[i + sel_base_idx] == 0);
      word_alloc[i + sel_base_idx] = 1;
    }
  }

  for (grp_stage = stage_info->sel_grp_stage_inuse_list; grp_stage;
       grp_stage = grp_stage->next) {
    PIPE_MGR_DBGCHK(grp_stage->stage_p == stage_info);
    sel_base_idx = grp_stage->sel_base_idx;

    PIPE_MGR_DBGCHK(sel_base_idx + grp_stage->no_words <= stage_info->no_words);
    for (i = 0; i < grp_stage->no_words; i++) {
      PIPE_MGR_DBGCHK(word_alloc[i + sel_base_idx] == 0);
      word_alloc[i + sel_base_idx] = 1;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_assert(bf_dev_id_t dev_id,
                                      pipe_sel_tbl_hdl_t tbl_hdl) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp = NULL;
  pipe_sel_grp_hdl_t grp_hdl;
  sel_tbl_t *sel_tbl;
  (void)grp_hdl;
  uint32_t i, j;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_info->sel_tbl[i];
    JUDYL_FOREACH(sel_tbl->sel_grp_array, grp_hdl, sel_grp_info_t *, sel_grp) {
      pipe_mgr_sel_grp_assert(sel_tbl_info, sel_grp);
    }
    for (j = 0; j < sel_tbl->num_stages; j++) {
      pipe_sel_tbl_stage_assert(sel_tbl, &sel_tbl->sel_tbl_stage_info[j]);
    }
  }

  return PIPE_SUCCESS;
}

uint32_t pipe_mgr_sel_group_count(sel_tbl_info_t *sel_tbl_info,
                                  bf_dev_pipe_t pipe_id) {
  sel_tbl_t *sel_tbl;
  Word_t Rc_word = 0;
  uint32_t count = 0;
  int i;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    /* Walk through all table instances. */
    for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
      sel_tbl = &sel_tbl_info->sel_tbl[i];
      JLC(Rc_word, sel_tbl->sel_grp_array, 0, -1);
      count += (uint32_t)Rc_word;
    }
  } else {
    sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
    if (!sel_tbl) {
      LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->dev_id,
                sel_tbl_info->tbl_hdl,
                pipe_id);
      return 0;
    }
    JLC(Rc_word, sel_tbl->sel_grp_array, 0, -1);
    count = (uint32_t)Rc_word;
  }

  return count;
}

static uint32_t pipe_mgr_sel_hw_group_count(sel_tbl_info_t *sel_tbl_info,
                                            bf_dev_pipe_t pipe_id) {
  sel_tbl_t *sel_tbl;
  uint32_t pipe_idx;
  uint32_t count = 0;

  for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    if (pipe_id == BF_DEV_PIPE_ALL || pipe_id == sel_tbl->pipe_id) {
      count += sel_tbl->llp.num_grps;
    }
  }

  return count;
}

pipe_status_t pipe_mgr_sel_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  sel_tbl_info_t *tbl = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (!tbl) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  *symmetric = tbl->is_symmetric;
  *num_scopes = tbl->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  tbl->scope_pipe_bmp,
                  tbl->num_scopes * sizeof tbl->scope_pipe_bmp[0]);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_set_symmetric_mode(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  bool symmetric,
                                                  scope_num_t num_scopes,
                                                  scope_pipes_t *scope_pipe_bmp,
                                                  bool is_backup) {
  pipe_status_t status = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl = NULL;
  pipe_select_tbl_info_t *sel_tbl_rmt_info = NULL;
  uint32_t usage = 0;
  uint32_t max_grps = 0;

  sel_tbl = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (sel_tbl == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  LOG_TRACE("%s: Table %s, Change to symmetric mode %d ",
            __func__,
            sel_tbl->name,
            symmetric);

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       sel_tbl->is_symmetric,
                                       sel_tbl->num_scopes,
                                       &sel_tbl->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              sel_tbl->name,
              sel_tbl->is_symmetric,
              sel_tbl->num_scopes);
    return status;
  }

  usage = pipe_mgr_sel_group_count(sel_tbl, BF_DEV_PIPE_ALL);
  if (usage > 0) {
    LOG_ERROR(
        "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d ",
        __func__,
        sel_tbl->name,
        sel_tbl->is_symmetric,
        usage);
    return PIPE_NOT_SUPPORTED;
  }

  sel_tbl_rmt_info = pipe_mgr_get_select_tbl_info(dev_id, tbl_hdl);
  if (sel_tbl_rmt_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  // cleanup first
  pipe_mgr_sel_tbl_destroy(sel_tbl->sel_tbl, sel_tbl->no_sel_tbls, is_backup);

  sel_tbl->is_symmetric = symmetric;
  /* Copy the new scope info */
  sel_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(sel_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));
  sel_tbl->no_sel_tbls = sel_tbl->num_scopes;
  if (sel_tbl->is_symmetric) {
    PIPE_MGR_DBGCHK(sel_tbl->num_scopes == 1);
    sel_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(sel_tbl->scope_pipe_bmp[0]);
  }
  // allocate with new setting
  sel_tbl->sel_tbl = pipe_mgr_sel_tbl_alloc(
      sess_hdl, sel_tbl, sel_tbl_rmt_info, &max_grps, is_backup);
  sel_tbl->max_grps = max_grps;

  if (!is_backup) {
    status = set_initial_value(sess_hdl, sel_tbl, sel_tbl_rmt_info, false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Error setting initial value rc 0x%x",
                __func__,
                __LINE__,
                sel_tbl->name,
                sel_tbl->tbl_hdl,
                sel_tbl->dev_id,
                status);
      pipe_mgr_sel_tbl_destroy(
          sel_tbl->sel_tbl, sel_tbl->no_sel_tbls, is_backup);
      sel_tbl->sel_tbl = NULL;
    }
  }

  return status;
}

pipe_status_t pipe_mgr_sel_get_first_entry_handle(pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  int *entry_hdl) {
  sel_tbl_info_t *sel_tbl_info;
  sel_tbl_t *sel_tbl = NULL;
  bool is_backup = false;
  Word_t grp_hdl;
  PWord_t Pvalue;
  uint32_t i;

  *entry_hdl = -1;
  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (!sel_tbl_info) {
    LOG_ERROR(
        "%s : Could not get the selection table info for table  handle 0x%x, "
        "device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (sel_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric sel tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_info->sel_tbl[i];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != sel_tbl->pipe_id)
      continue;

    grp_hdl = 0;
    JLF(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
    if (Pvalue) {
      /* First entry found. */
      *entry_hdl = grp_hdl;
      break;
    } else {
      *entry_hdl = -1;
      if (dev_tgt.dev_pipe_id == sel_tbl->pipe_id) {
        /* No entry in the requested pipe_id */
        break;
      }
    }
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_get_next_entry_handles(pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  pipe_sel_grp_hdl_t entry_hdl,
                                                  int n,
                                                  int *next_entry_handles) {
  sel_tbl_info_t *sel_tbl_info;
  sel_tbl_t *first_sel_tbl = NULL;
  sel_tbl_t *sel_tbl = NULL;
  Word_t grp_hdl = 0;
  uint32_t first_tbl_idx, idx;
  PWord_t Pvalue;
  bool is_backup = false;
  bool done = false;
  int i = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (!sel_tbl_info) {
    LOG_ERROR(
        "%s : Could not get the selection table info for table handle 0x%x "
        "device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (sel_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric sel tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      dev_tgt.dev_pipe_id != PIPE_GET_HDL_PIPE(entry_hdl)) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d for entry hdl %d passed for "
        "asymmetric sel tbl with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  uint32_t j = 0;
  if (sel_tbl_info->is_symmetric) {
    first_sel_tbl = &sel_tbl_info->sel_tbl[0];
  } else {
    bf_dev_pipe_t pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    for (j = 0; j < sel_tbl_info->no_sel_tbls; j++) {
      sel_tbl = &sel_tbl_info->sel_tbl[j];
      if (sel_tbl->pipe_id == pipe_id) {
        first_sel_tbl = sel_tbl;
        break;
      }
    }
  }

  /* Sanity check. */
  if (j >= sel_tbl_info->no_sel_tbls) {
    LOG_ERROR("%s:%d Unexpected error Dev %d Selector tbl %s 0x%x entry hdl %d",
              __func__,
              __LINE__,
              sel_tbl_info->dev_id,
              sel_tbl_info->name,
              tbl_hdl,
              entry_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  first_tbl_idx = j;

  grp_hdl = entry_hdl;
  for (idx = first_tbl_idx; idx < sel_tbl_info->no_sel_tbls && i < n && !done;
       idx++) {
    sel_tbl = &sel_tbl_info->sel_tbl[idx];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != sel_tbl->pipe_id)
      continue;

    if (sel_tbl != first_sel_tbl) {
      /* Look in next table instance. */
      grp_hdl = 0;
      JLF(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
      if (Pvalue) {
        /* Entry present */
        next_entry_handles[i++] = grp_hdl;
      } else {
        /* Keep looking in other table instances. */
        continue;
      }
    }

    while (i < n) {
      JLN(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
      if (!Pvalue) {
        next_entry_handles[i] = -1;
        if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
          /* There is no more entry for the requested pipe. */
          done = true;
        }
        break;
      }
      next_entry_handles[i] = grp_hdl;
      i++;
    }
  }

  if (i < n) next_entry_handles[i] = -1;

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_mat_ent_hdl_t entry_hdl,
                                     pipe_act_fn_hdl_t *act_fn_hdl,
                                     bool from_hw) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  bool is_backup = false;
  sel_grp_info_t *entry_data;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (!sel_tbl_info) {
    LOG_ERROR(
        "%s : Could not get the selection table info for table handle 0x%x "
        "device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Check if the entry handle passed in a valid one */
  entry_data = pipe_mgr_sel_grp_get(sel_tbl, entry_hdl);
  if (!entry_data) {
    LOG_ERROR(
        "%s : Sel Grp info get failed for table handle 0x%x entry handle %d",
        __func__,
        sel_tbl_info->tbl_hdl,
        entry_hdl);
    return PIPE_INVALID_ARG;
  }

  /* Copy the action function hdl */
  *act_fn_hdl = entry_data->act_fn_hdl;

  if (from_hw) {
  }

  return rc;
}

pipe_status_t pipe_mgr_sel_tbl_get_placed_entry_count(dev_target_t dev_tgt,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p) {
  sel_tbl_info_t *sel_tbl = NULL;
  uint32_t usage;

  sel_tbl = pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (sel_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (sel_tbl->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric sel tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  usage = pipe_mgr_sel_group_count(sel_tbl, dev_tgt.dev_pipe_id);

  *count_p = usage;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_tbl_hdl_t tbl_hdl, uint32_t *count_p) {
  sel_tbl_info_t *sel_tbl = NULL;
  uint32_t usage;

  sel_tbl = pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (sel_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (sel_tbl->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric sel tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  usage = pipe_mgr_sel_hw_group_count(sel_tbl, dev_tgt.dev_pipe_id);

  *count_p = usage;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_get_group_member_count(bf_dev_id_t dev_id,
                                                  pipe_sel_tbl_hdl_t tbl_hdl,
                                                  pipe_sel_grp_hdl_t grp_hdl,
                                                  uint32_t *count_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Selector table with handle 0x%x not found in device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *count_p = sel_grp_info->mbr_count;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_fallback_mbr_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_sel_update_type sel_op_type,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl_t *sel_tbl;
  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, dev_tgt.dev_pipe_id);
  if (sel_tbl == NULL) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Selector table with pipe %x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->dev_id,
              sel_tbl_info->tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }

  if (!sel_tbl->hlp.fallback_adt_ent_hdl && !adt_ent_hdl) {
    /* Fallback entry is not set for resetting*/
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table with pipe %x fallback entry is not set for reset",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;
  sel_tbl->sess_flags = pipe_api_flags;

  /* Backup the fallback entry */
  pipe_mgr_sel_backup_fallback_entry(sel_tbl);

  /* Loop through all the stages and provide callbacks */
  pipe_idx_t adt_index_array[sel_tbl->num_stages];

  uint32_t stage_idx;
  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    sel_tbl_stage_info_t *sel_tbl_stage_info;
    sel_tbl_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

    if (sel_op_type == PIPE_SEL_UPDATE_SET_FALLBACK) {
      rc = rmt_adt_ent_place(sel_tbl_info->dev_id,
                             sel_tbl->pipe_id,
                             sel_tbl_info->adt_tbl_hdl,
                             adt_ent_hdl,
                             sel_tbl_stage_info->stage_id,
                             &adt_index_array[stage_idx],
                             sel_tbl->sess_flags);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error placing adt entry 0x%x in stage %d for fallback entry",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->dev_id,
            sel_tbl_info->tbl_hdl,
            adt_ent_hdl,
            sel_tbl_stage_info->stage_id);
        return rc;
      }
    } else {
      rc = rmt_adt_ent_remove(sel_tbl_info->dev_id,
                              sel_tbl_info->adt_tbl_hdl,
                              sel_tbl->hlp.fallback_adt_ent_hdl,
                              sel_tbl_stage_info->stage_id,
                              sel_tbl->sess_flags);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error removing adt entry 0x%x in stage %d for fallback entry",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->dev_id,
            sel_tbl_info->tbl_hdl,
            sel_tbl->hlp.fallback_adt_ent_hdl,
            sel_tbl_stage_info->stage_id);
        return rc;
      }
    }
  }

  if (move_tail_p) {
    struct pipe_mgr_sel_move_list_t *move_node;
    move_node =
        alloc_sel_move_list(*move_tail_p, sel_op_type, sel_tbl->pipe_id);
    if (!move_node) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    move_node->adt_mbr_hdl = adt_ent_hdl;
    if (sel_op_type == PIPE_SEL_UPDATE_SET_FALLBACK) {
      move_node->data = make_sel_ent_data(sel_tbl->num_stages, adt_index_array);
      if (!move_node->data) {
        PIPE_MGR_FREE(move_node);
        if (*move_tail_p) {
          (*move_tail_p)->next = NULL;
        }
        LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }

      sel_tbl->hlp.fallback_adt_ent_hdl = adt_ent_hdl;
    } else {
      sel_tbl->hlp.fallback_adt_ent_hdl = 0;
    }
    *move_tail_p = move_node;
  }

  return PIPE_SUCCESS;
}

/* API function to set the fallback member */
pipe_status_t rmt_sel_fallback_mbr_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_status_t rc;
  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_sel_fallback_mbr_update(sess_hdl,
                                        dev_tgt,
                                        sel_tbl_hdl,
                                        adt_ent_hdl,
                                        PIPE_SEL_UPDATE_SET_FALLBACK,
                                        pipe_api_flags,
                                        move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %d - 0x%x "
        "SEL TBL: Error setting fallback member 0x%x rc(0x%x)",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        sel_tbl_hdl,
        adt_ent_hdl,
        rc);
    return rc;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  return PIPE_SUCCESS;
}

/* API function to reset the fallback member */
pipe_status_t rmt_sel_fallback_mbr_reset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    uint32_t pipe_api_flags,
    struct pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_status_t rc;
  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_sel_fallback_mbr_update(sess_hdl,
                                        dev_tgt,
                                        sel_tbl_hdl,
                                        0,
                                        PIPE_SEL_UPDATE_CLR_FALLBACK,
                                        pipe_api_flags,
                                        move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %d - 0x%x "
        "SEL TBL: Error resetting fallback member rc(0x%x)",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        sel_tbl_hdl,
        rc);
    return rc;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_get_plcmt_data(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    struct pipe_mgr_sel_move_list_t **move_list) {
  pipe_status_t rc = PIPE_SUCCESS;
  sel_tbl_info_t *tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sel_move_list_t *ml_head = NULL, *ml_tail = NULL;

  /* Go over all table instances, if symmetric there will be a single instance
   * but if asymmetric there can be multiple. */
  for (uint8_t i = 0; i < tbl_info->no_sel_tbls; ++i) {
    sel_tbl_t *inst_info = tbl_info->sel_tbl + i;
    /* Just use the first stage for generating the replay since we only issue
     * callbacks for updates to the first stage in the normal path. */
    sel_tbl_stage_info_t *stg_info = inst_info->sel_tbl_stage_info;
    /* Loop over each group in the selector table and issue a create group
     * callback. */
    for (sel_grp_stage_info_t *grp = stg_info->sel_grp_stage_inuse_list;
         grp != NULL;
         grp = grp->next) {
      struct pipe_mgr_sel_move_list_t *ml = NULL;
      sel_grp_info_t *grp_info = pipe_mgr_sel_grp_get(inst_info, grp->grp_hdl);
      if (grp_info) {
        rc = pipe_mgr_sel_grp_build_move_list(inst_info, grp_info, &ml);
      }
      if (PIPE_SUCCESS != rc || !ml) {
        goto free_mem_and_return;
      }

      /* First time through we'll set our list head, otherwise just append to
       * the end and update our tail pointer. */
      if (!ml_head) {
        ml_head = ml;
      } else {
        ml_tail->next = ml;
      }
      ml_tail = ml;

      /* No go over all members in the group and generate an ADD callback for
       * each member. */
      for (uint32_t n = 0; n < grp->no_words; ++n) {
        sel_hlp_word_data_t *wrd = grp->sel_grp_word_data + n;
        for (uint32_t e = 0; e < wrd->word_width; ++e) {
          if (0 == wrd->mbrs[e]) continue;
          ml = alloc_sel_move_list(
              ml_tail, PIPE_SEL_UPDATE_ADD, inst_info->pipe_id);
          if (!ml) {
            goto free_mem_and_return;
          }
          ml->sel_grp_hdl = grp->grp_hdl;
          ml->adt_mbr_hdl = wrd->mbrs[e];
          ml->logical_sel_index = grp->sel_base_idx + n;
          ml->logical_sel_subindex = e;
          pipe_idx_t adt_index_array[inst_info->num_stages];
          update_adt_base_indexes(
              inst_info, -1, grp->sel_base_idx + n, adt_index_array);
          ml->data = make_sel_ent_data(inst_info->num_stages, adt_index_array);

          /* Since ml_tail was passed into the alloc function ml has already
           * been appended to it so we only need to update our tail pointer. */
          ml_tail = ml;

          /* If the member is disabled a deactivate callback needs to be
           * generated. */
          if (!pipe_mgr_sel_grp_mbr_get_state(grp_info, wrd->mbrs[e])) {
            ml = alloc_sel_move_list(
                ml_tail, PIPE_SEL_UPDATE_DEACTIVATE, inst_info->pipe_id);
            if (!ml) {
              goto free_mem_and_return;
            }
            ml->sel_grp_hdl = grp->grp_hdl;
            ml->adt_mbr_hdl = wrd->mbrs[e];
            ml->logical_sel_index = grp->sel_base_idx + n;
            ml->logical_sel_subindex = e;
            ml_tail = ml;
          }
        }
      }
    }
  }
  *move_list = ml_head;
  return PIPE_SUCCESS;

free_mem_and_return:
  while (ml_head) {
    struct pipe_mgr_sel_move_list_t *x = ml_head;
    ml_head = ml_head->next;
    if (x->locations) PIPE_MGR_FREE(x->locations);
    PIPE_MGR_FREE(x);
  }
  return PIPE_NO_SYS_RESOURCES;
}

static pipe_status_t pipe_mgr_sel_process_mbr_add(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    pipe_adt_ent_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t mbr_idx,
    pipe_idx_t adt_base_index,
    bool disable_mbr,
    bool replace_mbr_hdl) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  sel_llp_word_data_t *word_data;
  pipe_status_t rc = PIPE_SUCCESS;

  uint32_t adt_index;

  adt_index = get_adt_index(adt_base_index, mbr_idx);
  /* Call ADT to add the adt member at this offset */
  if (sel_tbl_info->adt_tbl_hdl && !sel_tbl_info->in_restore) {
    rc = rmt_adt_ent_install(sel_tbl->cur_sess_hdl,
                             sel_tbl_info->dev_id,
                             sel_tbl_info->adt_tbl_hdl,
                             sel_tbl->pipe_id,
                             stage_info->stage_id,
                             mbr_hdl,
                             adt_index,
                             !replace_mbr_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error installing adt ent 0x%x at offset %d "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          mbr_hdl,
          adt_index,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }

  word_data = &stage_info->llp.llp_word[word_idx];

  PIPE_MGR_DBGCHK(word_data->mbrs[mbr_idx] == 0);

  if (word_data->adt_base_idx != adt_base_index) {
    word_data->adt_base_idx = adt_base_index;
  }

  if (disable_mbr) {
    SET_MBR_DATA_FOR_IDX(word_data, mbr_idx, 0);
  } else {
    SET_MBR_DATA_FOR_IDX(word_data, mbr_idx, 1);
    word_data->no_bits_set++;
  }
  word_data->mbrs[mbr_idx] = mbr_hdl;

  if ((word_data->highest_mbr_idx == -1) ||
      (mbr_idx > (uint32_t)word_data->highest_mbr_idx)) {
    word_data->highest_mbr_idx = mbr_idx;
  }
  PIPE_MGR_DBGCHK(word_data->no_bits_set <=
                  ((unsigned)word_data->highest_mbr_idx + 1));

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_process_mbr_delete(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    pipe_adt_ent_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t mbr_idx) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  sel_llp_word_data_t *word_data;
  pipe_status_t rc;
  bool is_active;

  word_data = &stage_info->llp.llp_word[word_idx];
  PIPE_MGR_DBGCHK(mbr_hdl == word_data->mbrs[mbr_idx]);

  uint32_t adt_base_index;
  adt_base_index = word_data->adt_base_idx;

  uint32_t adt_index;
  adt_index = get_adt_index(adt_base_index, mbr_idx);
  /* Call ADT to delete the adt member at this offset */
  if (sel_tbl_info->adt_tbl_hdl) {
    rc = rmt_adt_ent_uninstall(sel_tbl_info->dev_id,
                               sel_tbl_info->adt_tbl_hdl,
                               sel_tbl->pipe_id,
                               stage_info->stage_id,
                               mbr_hdl,
                               adt_index);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error installing adt ent 0x%x at offset %d "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          mbr_hdl,
          adt_index,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }

  is_active = GET_MBR_DATA_FOR_IDX(word_data, mbr_idx);
  if (is_active) {
    PIPE_MGR_DBGCHK(word_data->no_bits_set);
    word_data->no_bits_set--;
  }
  SET_MBR_DATA_FOR_IDX(word_data, mbr_idx, 0);
  word_data->mbrs[mbr_idx] = 0;

  if (mbr_idx == (uint32_t)word_data->highest_mbr_idx) {
    int32_t i;
    for (i = mbr_idx; i >= 0; i--) {
      if (word_data->mbrs[i] != 0) {
        break;
      }
    }
    word_data->highest_mbr_idx = i;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_process_mbr_activate_deactivate(
    sel_tbl_stage_info_t *stage_info,
    pipe_adt_ent_hdl_t mbr_hdl,
    uint32_t word_idx,
    uint32_t mbr_idx,
    bool activate) {
  (void)mbr_hdl;
  /* State update */
  sel_llp_word_data_t *word_data;
  bool is_active;

  word_data = &stage_info->llp.llp_word[word_idx];
  is_active = GET_MBR_DATA_FOR_IDX(word_data, mbr_idx);

  if (is_active && !activate) {
    PIPE_MGR_DBGCHK(word_data->no_bits_set);
    word_data->no_bits_set--;
  } else if (!is_active && activate) {
    word_data->no_bits_set++;
    PIPE_MGR_DBGCHK(word_data->no_bits_set <=
                    ((unsigned)word_data->highest_mbr_idx + 1));
  }
  SET_MBR_DATA_FOR_IDX(word_data, mbr_idx, activate ? 1 : 0);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_write_fallback_reg(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    rmt_virt_addr_t fallback_adt_ent_addr) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;

  pipe_bitmap_t pipe_bmp;
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  if (sel_tbl->pipe_id == BF_DEV_PIPE_ALL) {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(sel_tbl->sel_tbl_info->pipe_bmp));
  } else {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(sel_tbl->inst_pipe_bmp));
  }

  if (!PIPE_BITMAP_COUNT(&pipe_bmp)) {
    PIPE_MGR_DBGCHK(0 && "No pipes to write");
    return PIPE_UNEXPECTED;
  }
  uint32_t stage_id = stage_info->stage_id;
  uint32_t reg_data = 0, reg_addr = 0;

  setp_selector_action_adr_fallback_selector_action_adr_fallback(
      &reg_data, fallback_adt_ent_addr);

  sel_tbl_stage_hw_info_t *hw_info = &stage_info->pv_hw;

  /* Figure out the home row */
  uint32_t home_row = 0;
  uint32_t blk;

  for (blk = 0; blk < hw_info->num_blks + hw_info->num_spare_rams; blk++) {
    mem_id_t mem_id;
    if (blk >= hw_info->num_blks) {
      mem_id = hw_info->spare_rams[blk - hw_info->num_blks];
    } else {
      rmt_tbl_word_blk_t *tbl_blk = &hw_info->tbl_blk[blk];
      mem_id = tbl_blk->mem_id[0];
    }

    uint32_t phy_row = sel_tbl_info->dev_info->dev_cfg.mem_id_to_row(
        mem_id, pipe_mem_type_unit_ram);
    if (phy_row > home_row) {
      home_row = phy_row;
    }
  }

  /* Write reg_data on all rows the selector exists */
  for (blk = 0; blk < hw_info->num_blks + hw_info->num_spare_rams; blk++) {
    mem_id_t mem_id;
    if (blk >= hw_info->num_blks) {
      mem_id = hw_info->spare_rams[blk - hw_info->num_blks];
    } else {
      rmt_tbl_word_blk_t *tbl_blk = &hw_info->tbl_blk[blk];
      mem_id = tbl_blk->mem_id[0];
    }

    uint32_t phy_row = sel_tbl_info->dev_info->dev_cfg.mem_id_to_row(
        mem_id, pipe_mem_type_unit_ram);
    uint32_t oflo = (phy_row == home_row) ? 0 : 1;
    (void)oflo;

    pipe_instr_write_reg_i_only_t instr;

    switch (sel_tbl_info->dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        reg_addr = offsetof(
            Tofino,
            pipes[0]
                .mau[stage_id]
                .rams.map_alu.selector_action_adr_fallback[phy_row][oflo]);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        reg_addr = offsetof(
            tof2_reg,
            pipes[0]
                .mau[stage_id]
                .rams.map_alu.selector_action_adr_fallback[phy_row][oflo]);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        reg_addr = offsetof(
            tof3_reg,
            pipes[0]
                .mau[stage_id]
                .rams.map_alu.selector_action_adr_fallback[phy_row][oflo]);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    construct_instr_reg_write_no_data(sel_tbl_info->dev_id, &instr, reg_addr);
    rc = pipe_mgr_drv_ilist_add_2(&sel_tbl->cur_sess_hdl,
                                  sel_tbl_info->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_write_reg_i_only_t),
                                  (uint8_t *)&reg_data,
                                  sizeof(uint32_t));
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_process_fallback_mbr_activate(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    pipe_adt_ent_hdl_t mbr_hdl,
    pipe_idx_t adt_index,
    rmt_virt_addr_t *fallback_adt_ent_addr_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;

  /* Call ADT to add the adt member at this offset */
  if (sel_tbl_info->adt_tbl_hdl) {
    rc = rmt_adt_ent_activate_stage(sel_tbl->cur_sess_hdl,
                                    sel_tbl_info->dev_id,
                                    sel_tbl->pipe_id,
                                    sel_tbl_info->adt_tbl_hdl,
                                    mbr_hdl,
                                    stage_info->stage_id,
                                    adt_index,
                                    fallback_adt_ent_addr_p,
                                    false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error activating adt ent 0x%x at offset %d "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          mbr_hdl,
          adt_index,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }

  sel_tbl->llp.fallback_adt_ent_hdl = mbr_hdl;
  stage_info->fallback_adt_index = adt_index;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_process_fallback_mbr_deactivate(
    sel_tbl_t *sel_tbl,
    sel_tbl_stage_info_t *stage_info,
    rmt_virt_addr_t *fallback_adt_ent_addr_p) {
  sel_tbl_info_t *sel_tbl_info = sel_tbl->sel_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;

  pipe_adt_ent_hdl_t mbr_hdl = sel_tbl->llp.fallback_adt_ent_hdl;

  uint32_t adt_index;
  adt_index = stage_info->fallback_adt_index;
  if (sel_tbl_info->adt_tbl_hdl) {
    rc = rmt_adt_ent_deactivate_stage(sel_tbl->pipe_id,
                                      sel_tbl_info->dev_id,
                                      sel_tbl_info->adt_tbl_hdl,
                                      mbr_hdl,
                                      stage_info->stage_id,
                                      adt_index);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error deactivating adt ent 0x%x at offset %d "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          mbr_hdl,
          adt_index,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }
  stage_info->fallback_adt_index = ~0;
  sel_tbl->llp.fallback_adt_ent_hdl = 0;
  *fallback_adt_ent_addr_p = 0;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_process_placement_op(
    pipe_sess_hdl_t sess_hdl,
    sel_tbl_info_t *sel_tbl_info,
    struct pipe_mgr_sel_move_list_t *move_node) {
  sel_tbl_t *sel_tbl;

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, move_node->pipe);
  if (sel_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Selector table with pipe_id %d not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->tbl_hdl,
        sel_tbl_info->dev_id,
        move_node->pipe);
    return PIPE_UNEXPECTED;
  }

  /* Set-up the session parameters */
  sel_tbl->cur_sess_hdl = sess_hdl;

  // Loop through all the stages
  uint32_t stage_idx;
  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    sel_tbl_stage_info_t *stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
    pipe_adt_ent_hdl_t mbr_hdl = move_node->adt_mbr_hdl;
    uint32_t word_idx = move_node->logical_sel_index;
    uint32_t mbr_idx = move_node->logical_sel_subindex;
    bool disabled_mbr = move_node->disabled_mbr;
    uint32_t adt_idx = 0;

    bool new_mbr_val;
    bool adjust_total;
    bool fallback_update = false;
    rmt_virt_addr_t fallback_adt_ent_addr = 0;

    pipe_status_t rc = PIPE_SUCCESS;
    switch (move_node->op) {
      case PIPE_SEL_UPDATE_GROUP_CREATE:
        sel_tbl->llp.num_grps++;
        return rc;
      case PIPE_SEL_UPDATE_GROUP_DESTROY:
        PIPE_MGR_DBGCHK(sel_tbl->llp.num_grps);
        if (sel_tbl->llp.num_grps) {
          sel_tbl->llp.num_grps--;
        }
        return rc;
      case PIPE_SEL_UPDATE_ADD:
        if (disabled_mbr) {
          new_mbr_val = false;
        } else {
          new_mbr_val = true;
        }
        adjust_total = true;
        adt_idx = unpack_sel_ent_data_adt_index(move_node->data, stage_idx);
        if (adt_idx != PIPE_MGR_LOGICAL_ACT_IDX_INVALID) {
          rc = pipe_mgr_sel_process_mbr_add(sel_tbl,
                                            stage_info,
                                            mbr_hdl,
                                            word_idx,
                                            mbr_idx,
                                            adt_idx,
                                            disabled_mbr,
                                            move_node->replace_mbr_hdl);
        }
        break;
      case PIPE_SEL_UPDATE_DEL:
        new_mbr_val = false;
        adjust_total = true;
        rc = pipe_mgr_sel_process_mbr_delete(
            sel_tbl, stage_info, mbr_hdl, word_idx, mbr_idx);
        break;
      case PIPE_SEL_UPDATE_ACTIVATE:
        new_mbr_val = true;
        adjust_total = SEL_TBL_IS_FAIR(sel_tbl_info);
        rc = pipe_mgr_sel_process_mbr_activate_deactivate(
            stage_info, mbr_hdl, word_idx, mbr_idx, true);
        break;
      case PIPE_SEL_UPDATE_DEACTIVATE:
        new_mbr_val = false;
        adjust_total = SEL_TBL_IS_FAIR(sel_tbl_info);
        rc = pipe_mgr_sel_process_mbr_activate_deactivate(
            stage_info, mbr_hdl, word_idx, mbr_idx, false);
        break;
      case PIPE_SEL_UPDATE_SET_FALLBACK:
        fallback_update = true;
        rc = pipe_mgr_sel_process_fallback_mbr_activate(
            sel_tbl,
            stage_info,
            mbr_hdl,
            unpack_sel_ent_data_adt_index(move_node->data, stage_idx),
            &fallback_adt_ent_addr);
        break;
      case PIPE_SEL_UPDATE_CLR_FALLBACK:
        fallback_update = true;
        rc = pipe_mgr_sel_process_fallback_mbr_deactivate(
            sel_tbl, stage_info, &fallback_adt_ent_addr);
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_INVALID_ARG;
    }

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error processing selector move list node op %d mbr_hdl 0x%x rc 0x%x",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id,
          move_node->op,
          mbr_hdl,
          rc);
      return rc;
    }
    if (fallback_update) {
      /* Update the fallback register */
      rc = pipe_mgr_sel_write_fallback_reg(
          sel_tbl, stage_info, fallback_adt_ent_addr);
    } else if (move_node->replace_mbr_hdl) {
      /* No need to update HW. Only the member handle is updated in SW. */
    } else if (sel_tbl_info->stful_tbl_hdl) {
      /* Issue an instruction to write the bit and adjust the count if needed.
       */
      uint32_t tgt_offset = get_logical_index_by_word_idx(word_idx, mbr_idx);
      if ((move_node->op == PIPE_SEL_UPDATE_DEL) &&
          !SEL_TBL_IS_FAIR(sel_tbl_info)) {
        /* In resilient mode set the bit first in case the data plane had
         * cleared it and then clear it with an "adjust total" instruction.
         * This is required because the "adjust total" instruction only
         * decrements the count when transitioning from one to zero. */
        rc = pipe_mgr_stful_wr_bit(sel_tbl->cur_sess_hdl,
                                   sel_tbl_info->dev_id,
                                   sel_tbl->pipe_id,
                                   sel_tbl_info->stful_tbl_hdl,
                                   stage_info->stage_id,
                                   tgt_offset,
                                   1,
                                   false);
        if (PIPE_SUCCESS != rc) return rc;
      }
      rc = pipe_mgr_stful_wr_bit(sel_tbl->cur_sess_hdl,
                                 sel_tbl_info->dev_id,
                                 sel_tbl->pipe_id,
                                 sel_tbl_info->stful_tbl_hdl,
                                 stage_info->stage_id,
                                 tgt_offset,
                                 new_mbr_val,
                                 adjust_total);
      if (PIPE_SUCCESS != rc) return rc;
      rc = pipe_mgr_sel_grp_update_hw(sel_tbl, stage_info, word_idx, true);
    } else {
      // Update the HW
      rc = pipe_mgr_sel_grp_update_hw(sel_tbl, stage_info, word_idx, false);
    }

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Error updating hardware for word %d rc 0x%x",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                word_idx,
                rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_sel_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    struct pipe_mgr_sel_move_list_t *move_list,
    uint32_t *processed) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found on dev %d",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(*processed == 0);

  struct pipe_mgr_sel_move_list_t *move_node;

  for (move_node = move_list; move_node; move_node = move_node->next) {
    rc = pipe_mgr_sel_process_placement_op(sess_hdl, sel_tbl_info, move_node);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    (*processed)++;
    if (pipe_mgr_sess_in_batch(sess_hdl)) {
      pipe_mgr_drv_ilist_chkpt(sess_hdl);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_sel_get_first_group_member(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t *mbr_hdl_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  Word_t mbr_hdl = 0;
  PWord_t Pvalue = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found on dev %d",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  JLF(Pvalue, sel_grp_info->sel_grp_mbrs, mbr_hdl);
  if (Pvalue) {
    *mbr_hdl_p = mbr_hdl;
    return PIPE_SUCCESS;
  }
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_sel_get_next_group_members(
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    int n,
    pipe_sel_grp_mbr_hdl_t *mbr_hdl_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  Word_t curr_mbr_hdl;
  PWord_t Pvalue = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found on dev %d",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  curr_mbr_hdl = mbr_hdl;
  uint32_t i = 0;
  while (n) {
    Pvalue = NULL;
    JLN(Pvalue, sel_grp_info->sel_grp_mbrs, curr_mbr_hdl);
    if (Pvalue) {
      mbr_hdl_p[i++] = curr_mbr_hdl;
    } else {
      if (i > 0) {
        return PIPE_SUCCESS;
      } else {
        return PIPE_OBJ_NOT_FOUND;
      }
    }
    n--;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_sel_get_word_llp_active_member_count(
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t *count) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *stage_info = NULL;
  sel_llp_word_data_t *word_data = NULL;
  uint32_t mbr_idx = 0;

  /* Virtual devices do not have llp state */
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    LOG_ERROR("%s:%d LLP state does not exist on virtual device %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found on dev %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, dev_tgt.dev_pipe_id);
  if (sel_tbl == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x pipe_id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  stage_info = &sel_tbl->sel_tbl_stage_info[0];
  if (word_index >= stage_info->no_words) {
    LOG_ERROR("%s:%d Word index %d out of range in sel tbl 0x%x with %d words",
              __func__,
              __LINE__,
              word_index,
              tbl_hdl,
              stage_info->no_words);
    return PIPE_INVALID_ARG;
  }
  word_data = &stage_info->llp.llp_word[word_index];

  for (; mbr_idx < SEL_VEC_WIDTH; mbr_idx++) {
    if (GET_MBR_DATA_FOR_IDX(word_data, mbr_idx)) {
      (*count)++;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_sel_get_word_llp_active_members(
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t count,
    pipe_adt_ent_hdl_t *mbr_hdls) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *stage_info = NULL;
  sel_llp_word_data_t *word_data = NULL;
  uint32_t mbr_idx = 0, mbr_hdls_idx = 0;

  /* Virtual devices do not have llp state */
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    LOG_ERROR("%s:%d LLP state does not exist on virtual device %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x not found on dev %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, dev_tgt.dev_pipe_id);
  if (sel_tbl == NULL) {
    LOG_ERROR("%s:%d sel table with handle 0x%x pipe_id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  stage_info = &sel_tbl->sel_tbl_stage_info[0];
  if (word_index >= stage_info->no_words) {
    LOG_ERROR("%s:%d Word index %d out of range in sel tbl 0x%x with %d words",
              __func__,
              __LINE__,
              word_index,
              tbl_hdl,
              stage_info->no_words);
    return PIPE_INVALID_ARG;
  }
  word_data = &stage_info->llp.llp_word[word_index];

  for (; mbr_idx < SEL_VEC_WIDTH && mbr_hdls_idx < count; mbr_idx++) {
    if (GET_MBR_DATA_FOR_IDX(word_data, mbr_idx)) {
      mbr_hdls[mbr_hdls_idx] = word_data->mbrs[mbr_idx];
      mbr_hdls_idx++;
    }
  }

  if (mbr_hdls_idx < count) {
    mbr_hdls[mbr_hdls_idx] = PIPE_INVALID_SEL_GRP_MBR;
  }

  return PIPE_SUCCESS;
}

/* API function to get the current state of a selection member */
pipe_status_t rmt_sel_grp_mbr_state_get(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t sel_grp_mbr_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  if (mbr_state_p == NULL) {
    LOG_ERROR("%s:%d mbr_state_p is NULL", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_mbr_t *mbr =
      pipe_mgr_sel_grp_mbr_get(sel_grp_info, sel_grp_mbr_hdl, false);
  if (!mbr) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Sel group member %d not found in group %d",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_mbr_hdl,
              sel_grp_info->grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *mbr_state_p = mbr->state;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_get_params(bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl,
                                          uint32_t *max_size,
                                          uint32_t *adt_offset) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *max_size = sel_grp_info->max_grp_size;
  *adt_offset = sel_grp_info->adt_offset;

  return PIPE_SUCCESS;
}

/* Supports fetching either grp id or handle in a single call - not both.
 */
pipe_status_t pipe_mgr_sel_get_grp_id_hdl_int(bf_dev_target_t dev_tgt,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              pipe_sel_grp_id_t grp_id,
                                              pipe_sel_grp_hdl_t grp_hdl,
                                              pipe_sel_grp_hdl_t *sel_grp_hdl,
                                              pipe_sel_grp_id_t *sel_grp_id) {
  if (sel_grp_hdl != NULL && sel_grp_id != NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_info =
      pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (sel_tbl_info->is_symmetric) {
    if (dev_tgt.dev_pipe_id != DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for a symmetric selector "
          "table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          sel_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  } else {
    if (dev_tgt.dev_pipe_id == DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id of all pipes passed for a asymmetric "
          "selector table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          sel_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }

  sel_tbl_t *sel_tbl =
      get_sel_tbl_by_pipe_id(sel_tbl_info, dev_tgt.dev_pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d get sel table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  sel_grp_info_t *sel_grp_info = NULL;
  if (sel_grp_hdl) {
    bf_map_sts_t sts =
        bf_map_get(&sel_tbl->grp_id_map, grp_id, (void **)&sel_grp_info);
    if (sts == BF_MAP_NO_KEY) return PIPE_OBJ_NOT_FOUND;
    if (sts != BF_MAP_OK) return PIPE_UNEXPECTED;
    *sel_grp_hdl = sel_grp_info->grp_hdl;
  } else if (sel_grp_id) {
    sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
    if (sel_grp_info == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
                __func__,
                __LINE__,
                sel_tbl_info->name,
                sel_tbl_info->tbl_hdl,
                sel_tbl_info->dev_id,
                grp_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    *sel_grp_id = sel_grp_info->grp_id;
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

static uint8_t calc_sbox_3x3(uint8_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;

  uint8_t o0 =
      (((~i2 & 1) & i1) | ((~i2 & 1) & (~i0 & 1)) | (i1 & (~i0 & 1))) & 1;
  uint8_t o1 = ((i2 & i1) | (i2 & (~i0 & 1)) | (i1 & (~i0 & 1))) & 1;
  uint8_t o2 = (((~i2 & 1) & i1) | ((~i2 & 1) & i0) | (i1 & i0)) & 1;

  return ((o2 << 2) + (o1 << 1) + o0) & 0x7;
}

static uint8_t calc_sbox_4x4(uint8_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;

  uint8_t o0 = ((i3 & (~i2 & 1) & i1) | ((~i3 & 1) & i2 & (~i1 & 1)) |
                (i3 & i2 & (~i0 & 1)) | ((~i3 & 1) & i1 & i0)) &
               1;
  uint8_t o1 = ((i3 & (~i2 & 1) & i1) | (i3 & (~i2 & 1) & (~i0 & 1)) |
                ((~i3 & 1) & i2 & (~i0 & 1)) | ((~i3 & 1) & i1 & (~i0 & 1)) |
                (i2 & i1 & i0)) &
               1;
  uint8_t o2 = ((i3 & (~i2 & 1) & i0) | ((~i3 & 1) & i1 & (~i0 & 1)) |
                (i2 & (~i1 & 1))) &
               1;
  uint8_t o3 = ((i3 & (~i2 & 1) & (~i1 & 1) & i0) | ((~i3 & 1) & i2) |
                ((~i3 & 1) & (~i1 & 1) & (~i0 & 1)) | (i2 & i1)) &
               1;

  return ((o3 << 3) + (o2 << 2) + (o1 << 1) + o0) & 0xf;
}

static uint16_t calc_swizzler_14_1(uint16_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;

  return ((i13 << 13) + (i8 << 12) + (i5 << 11) + (i1 << 10) + (i12 << 9) +
          (i7 << 8) + (i4 << 7) + (i0 << 6) + (i11 << 5) + (i6 << 4) +
          (i3 << 3) + (i10 << 2) + (i9 << 1) + i2) &
         0x3fff;
}

static uint16_t calc_swizzler_14_2(uint16_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;

  return ((i13 << 13) + (i9 << 12) + (i5 << 11) + (i2 << 10) + (i12 << 9) +
          (i8 << 8) + (i4 << 7) + (i1 << 6) + (i11 << 5) + (i7 << 4) +
          (i3 << 3) + (i0 << 2) + (i10 << 1) + i6) &
         0x3fff;
}

static uint16_t sps_14_scramble(uint16_t hash_value) {
  uint16_t input = hash_value & 0x3fff;
  uint16_t output = 0;

  uint8_t sps0 = input & 0x7;
  uint8_t sps3 = (input >> 3) & 0x7;
  uint8_t sps6 = (input >> 6) & 0xf;
  uint8_t sps10 = (input >> 10) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_3x3(sps3);
  sps6 = calc_sbox_4x4(sps6);
  sps10 = calc_sbox_4x4(sps10);

  input = ((sps10 << 10) + (sps6 << 6) + (sps3 << 3) + sps0) & 0x3fff;
  output = calc_swizzler_14_1(input);

  sps0 = output & 0x7;
  sps3 = (output >> 3) & 0x7;
  sps6 = (output >> 6) & 0xf;
  sps10 = (output >> 10) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_3x3(sps3);
  sps6 = calc_sbox_4x4(sps6);
  sps10 = calc_sbox_4x4(sps10);

  input = ((sps10 << 10) + (sps6 << 6) + (sps3 << 3) + sps0) & 0x3fff;
  output = calc_swizzler_14_2(input);

  return output;
}

static uint16_t calc_swizzler_15_1(uint16_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;
  uint8_t i14 = (input >> 14) & 1;

  return ((i14 << 14) + (i9 << 13) + (i4 << 12) + (i1 << 11) + (i13 << 10) +
          (i8 << 9) + (i3 << 8) + (i0 << 7) + (i12 << 6) + (i7 << 5) +
          (i6 << 4) + (i2 << 3) + (i11 << 2) + (i10 << 1) + i5) &
         0x7fff;
}

static uint16_t calc_swizzler_15_2(uint16_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;
  uint8_t i14 = (input >> 14) & 1;

  return ((i14 << 14) + (i10 << 13) + (i6 << 12) + (i2 << 11) + (i13 << 10) +
          (i9 << 9) + (i5 << 8) + (i1 << 7) + (i12 << 6) + (i8 << 5) +
          (i4 << 4) + (i0 << 3) + (i11 << 2) + (i7 << 1) + i3) &
         0x7fff;
}

static uint16_t sps_15_scramble(uint16_t hash_value) {
  uint16_t input = hash_value & 0x7fff;
  uint16_t output = 0;

  uint8_t sps0 = input & 0x7;
  uint8_t sps3 = (input >> 3) & 0xf;
  uint8_t sps7 = (input >> 7) & 0xf;
  uint8_t sps11 = (input >> 11) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_4x4(sps3);
  sps7 = calc_sbox_4x4(sps7);
  sps11 = calc_sbox_4x4(sps11);

  input = ((sps11 << 11) + (sps7 << 7) + (sps3 << 3) + sps0) & 0x7fff;
  output = calc_swizzler_15_1(input);

  sps0 = output & 0x7;
  sps3 = (output >> 3) & 0xf;
  sps7 = (output >> 7) & 0xf;
  sps11 = (output >> 11) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_4x4(sps3);
  sps7 = calc_sbox_4x4(sps7);
  sps11 = calc_sbox_4x4(sps11);

  input = ((sps11 << 11) + (sps7 << 7) + (sps3 << 3) + sps0) & 0x7fff;
  output = calc_swizzler_15_2(input);

  return output;
}

static uint32_t calc_swizzler_18_1(uint32_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;
  uint8_t i14 = (input >> 14) & 1;
  uint8_t i15 = (input >> 15) & 1;
  uint8_t i16 = (input >> 16) & 1;
  uint8_t i17 = (input >> 17) & 1;

  return ((i17 << 17) + (i12 << 16) + (i9 << 15) + (i4 << 14) + (i2 << 13) +
          (i16 << 12) + (i11 << 11) + (i8 << 10) + (i3 << 9) + (i1 << 8) +
          (i15 << 7) + (i10 << 6) + (i7 << 5) + (i5 << 4) + (i0 << 3) +
          (i14 << 2) + (i13 << 1) + i6) &
         0x3ffff;
}

static uint32_t calc_swizzler_18_2(uint32_t input) {
  uint8_t i0 = input & 1;
  uint8_t i1 = (input >> 1) & 1;
  uint8_t i2 = (input >> 2) & 1;
  uint8_t i3 = (input >> 3) & 1;
  uint8_t i4 = (input >> 4) & 1;
  uint8_t i5 = (input >> 5) & 1;
  uint8_t i6 = (input >> 6) & 1;
  uint8_t i7 = (input >> 7) & 1;
  uint8_t i8 = (input >> 8) & 1;
  uint8_t i9 = (input >> 9) & 1;
  uint8_t i10 = (input >> 10) & 1;
  uint8_t i11 = (input >> 11) & 1;
  uint8_t i12 = (input >> 12) & 1;
  uint8_t i13 = (input >> 13) & 1;
  uint8_t i14 = (input >> 14) & 1;
  uint8_t i15 = (input >> 15) & 1;
  uint8_t i16 = (input >> 16) & 1;
  uint8_t i17 = (input >> 17) & 1;

  return ((i17 << 17) + (i13 << 16) + (i9 << 15) + (i5 << 14) + (i2 << 13) +
          (i16 << 12) + (i12 << 11) + (i8 << 10) + (i4 << 9) + (i1 << 8) +
          (i15 << 7) + (i11 << 6) + (i7 << 5) + (i3 << 4) + (i0 << 3) +
          (i14 << 2) + (i10 << 1) + i6) &
         0x3ffff;
}

static uint32_t sps_18_scramble(uint32_t hash_value) {
  uint32_t input = hash_value & 0x3ffff;
  uint32_t output = 0;

  uint8_t sps0 = input & 0x7;
  uint8_t sps3 = (input >> 3) & 0x7;
  uint8_t sps6 = (input >> 6) & 0xf;
  uint8_t sps10 = (input >> 10) & 0xf;
  uint8_t sps14 = (input >> 14) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_3x3(sps3);
  sps6 = calc_sbox_4x4(sps6);
  sps10 = calc_sbox_4x4(sps10);
  sps14 = calc_sbox_4x4(sps14);

  input = ((sps14 << 14) + (sps10 << 10) + (sps6 << 6) + (sps3 << 3) + sps0) &
          0x3ffff;
  output = calc_swizzler_18_1(input);

  sps0 = output & 0x7;
  sps3 = (output >> 3) & 0x7;
  sps6 = (output >> 6) & 0xf;
  sps10 = (output >> 10) & 0xf;
  sps14 = (output >> 14) & 0xf;

  sps0 = calc_sbox_3x3(sps0);
  sps3 = calc_sbox_3x3(sps3);
  sps6 = calc_sbox_4x4(sps6);
  sps10 = calc_sbox_4x4(sps10);
  sps14 = calc_sbox_4x4(sps14);

  input = ((sps14 << 14) + (sps10 << 10) + (sps6 << 6) + (sps3 << 3) + sps0) &
          0x3ffff;
  output = calc_swizzler_18_2(input);

  return output;
}

uint32_t calculate_plan_b(uint32_t input, bool *active_mbrs) {
  bool priority_encoder[SEL_GRP_WORD_WIDTH] = {0};
  uint8_t prio_xor[2] = {0};
  prio_xor[0] = (input & 0x3f) << 1;
  prio_xor[1] = ((input >> 6) & 0x3f) << 1;

  /* Construct circular priority encoder */
  uint8_t i = 0;
  for (; i < SEL_GRP_WORD_WIDTH; i++) {
    if (active_mbrs[i]) {
      priority_encoder[i ^ prio_xor[i & 1]] = true;
    }
  }

  uint8_t head = (input >> 12) & 0x3f;
  uint8_t idx = 0;
  for (i = head + SEL_GRP_WORD_WIDTH; i > head; i--) {
    idx = i % SEL_GRP_WORD_WIDTH;
    if (priority_encoder[idx]) {
      return idx ^ prio_xor[idx & 1];
    }
  }

  return SEL_GRP_WORD_WIDTH;
}

static uint32_t get_hash_value(uint8_t *hash,
                               uint8_t hash_len,
                               uint8_t offset,
                               int width) {
  uint8_t byte_offset = 0, bit_offset = 0, mask = 0, ret_shift = 0;
  uint32_t ret = 0;

  while (width > 0) {
    byte_offset = hash_len - offset / 8 - 1;
    bit_offset = offset % 8;
    mask = width >= 8 ? 0xff : (1 << width) - 1;
    ret |= (uint32_t)((hash[byte_offset] >> bit_offset) & mask) << ret_shift;
    width -= 8 - bit_offset;
    offset += 8 - bit_offset;
    ret_shift += 8 - bit_offset;
  }

  return ret;
}

pipe_status_t rmt_sel_grp_mbr_get_from_hash(bf_dev_id_t dev_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            uint8_t *hash,
                                            uint32_t hash_len,
                                            pipe_adt_ent_hdl_t *adt_ent_hdl_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  PWord_t Pvalue = NULL;
  Pvoid_t stage_lookup = NULL;
  Word_t pipe_idx = 0, stage_idx = 0;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_hlp_word_data_t *word_data = NULL;
  sel_grp_mbr_t *mbr = NULL;
  uint32_t num_words = 0, word_idx = 0, mbr_idx = 0;
  uint32_t num_active_mbrs = 0, i = 0, hash_value = 0;
  bool active_mbrs[SEL_GRP_WORD_WIDTH] = {0};
  bf_dev_pipe_t pipe_id;
  sel_tbl_t *sel_tbl;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  JLF(Pvalue, sel_grp_info->sel_grp_pipe_lookup, pipe_idx);
  stage_lookup = (Pvoid_t)(*Pvalue);
  JLF(Pvalue, stage_lookup, stage_idx);
  sel_grp_stage_info = (sel_grp_stage_info_t *)(*Pvalue);
  num_words = sel_grp_stage_info->no_words;

  if (num_words > 1) {
    if (hash_len * 8 < 66) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) Hash len needs to be at least 66 bits. Sent in %d",
          __func__,
          __LINE__,
          sel_tbl_info->name,
          sel_tbl_info->tbl_hdl,
          sel_tbl_info->dev_id,
          hash_len * 8);
      return PIPE_INVALID_ARG;
    }
    hash_value = get_hash_value(hash, hash_len, 51, 15);
    if (sel_tbl_info->sps_enable) {
      hash_value = sps_15_scramble(hash_value);
    }
    uint32_t sel_shift = 0;
    while ((num_words >> sel_shift) > 31) {
      sel_shift++;
    }
    word_idx = (hash_value & 0x3ff) % (num_words >> sel_shift);
    word_idx =
        (word_idx << sel_shift) | ((hash_value >> 10) & ((1 << sel_shift) - 1));
  }
  word_data = &sel_grp_stage_info->sel_grp_word_data[word_idx];

  for (i = 0; i < word_data->usage; i++) {
    mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, word_data->mbrs[i], false);
    if (mbr && mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      active_mbrs[i] = true;
      num_active_mbrs++;
    }
  }
  PIPE_MGR_DBGCHK(num_active_mbrs == sel_grp_info->num_active_mbrs);
  if (num_active_mbrs == 0) {
    LOG_ERROR(
        "%s:%d No active members in sel grp 0x%x word %d table 0x%x device %d",
        __func__,
        __LINE__,
        sel_grp_hdl,
        word_idx,
        sel_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (uint32_t hash_idx = 0; hash_idx < 3; hash_idx++) {
    hash_value = get_hash_value(hash, hash_len, hash_idx * 14, 14);
    if (sel_tbl_info->sps_enable) {
      hash_value = sps_14_scramble(hash_value & 0x3fff);
    }
    if (SEL_TBL_IS_RESILIENT(sel_tbl_info)) {
      mbr_idx = hash_value % word_data->usage;
      mbr = pipe_mgr_sel_grp_mbr_get(
          sel_grp_info, word_data->mbrs[mbr_idx], false);
      if (mbr && mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
        *adt_ent_hdl_p = mbr->mbr_hdl;
        return PIPE_SUCCESS;
      }
    } else {
      mbr_idx = hash_value % num_active_mbrs;
      for (i = 0; i < word_data->usage; i++) {
        mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, word_data->mbrs[i], false);
        if (mbr && mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
          if (mbr_idx == 0) {
            *adt_ent_hdl_p = mbr->mbr_hdl;
            return PIPE_SUCCESS;
          }
          mbr_idx--;
        }
      }
      /* Fair selection should only ever need one iteration */
      LOG_ERROR(
          "%s:%d Error finding hit member for fair sel tbl 0x%x device %d",
          __func__,
          __LINE__,
          sel_tbl_hdl,
          dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  /* 3 failed attempts at resilient hashing. Fall back to plan B */
  uint32_t input = ((get_hash_value(hash, hash_len, 42, 9) & 0x1ff) << 9) |
                   (get_hash_value(hash, hash_len, 5, 9) & 0x1ff);
  if (sel_tbl_info->sps_enable) {
    input = sps_18_scramble(input);
  }
  mbr_idx = calculate_plan_b(input, active_mbrs);
  if (mbr_idx < SEL_VEC_WIDTH) {
    mbr =
        pipe_mgr_sel_grp_mbr_get(sel_grp_info, word_data->mbrs[mbr_idx], false);
    if (mbr && mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE) {
      *adt_ent_hdl_p = mbr->mbr_hdl;
      return PIPE_SUCCESS;
    }
  }

  LOG_ERROR(
      "%s:%d Error finding hit member for resilient sel tbl 0x%x device %d",
      __func__,
      __LINE__,
      sel_tbl_hdl,
      dev_id);
  return PIPE_OBJ_NOT_FOUND;
}

void pipe_mgr_sel_shadow_db_cleanup(bf_dev_id_t dev) {
  int p;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev);
    return;
  }

  int dev_pipes = dev_info->num_active_pipes;

  if (PIPE_SEL_SHADOW_DB(dev)) {
    for (p = 0; p < dev_pipes; p++) {
      if (PIPE_SEL_SHADOW_DB(dev)->power_db[p])
        PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->power_db[p]);
      if (PIPE_SEL_SHADOW_DB(dev)->seed_db[p])
        PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->seed_db[p]);
      if (PIPE_SEL_SHADOW_DB(dev)->pgm_db[p])
        PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->pgm_db[p]);
    }
    if (PIPE_SEL_SHADOW_DB(dev)->power_db)
      PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->power_db);
    if (PIPE_SEL_SHADOW_DB(dev)->seed_db)
      PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->seed_db);
    if (PIPE_SEL_SHADOW_DB(dev)->pgm_db)
      PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->pgm_db);
    PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev));
    PIPE_SEL_SHADOW_DB(dev) = NULL;
  }
}

/* DB to cache the dynamic selector register values */
pipe_status_t pipe_mgr_sel_shadow_db_init(bf_dev_id_t dev) {
  dev_stage_t s_idx = 0;
  uint32_t p = 0, dev_pipes = 0, dev_stages = 0;
  pipe_status_t sts = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  dev_pipes = dev_info->num_active_pipes;
  dev_stages = dev_info->num_active_mau;

  PIPE_SEL_SHADOW_DB(dev) =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_sel_shadow_db_t));
  if (!PIPE_SEL_SHADOW_DB(dev)) return PIPE_NO_SYS_RESOURCES;
  PIPE_SEL_SHADOW_DB(dev)->dev = dev;

  /* Alocate memory for all pipes */
  PIPE_SEL_SHADOW_DB(dev)->power_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*PIPE_SEL_SHADOW_DB(dev)->power_db));
  PIPE_SEL_SHADOW_DB(dev)->seed_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*PIPE_SEL_SHADOW_DB(dev)->seed_db));
  PIPE_SEL_SHADOW_DB(dev)->pgm_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*PIPE_SEL_SHADOW_DB(dev)->pgm_db));
  if (!PIPE_SEL_SHADOW_DB(dev)->seed_db || !PIPE_SEL_SHADOW_DB(dev)->power_db ||
      !PIPE_SEL_SHADOW_DB(dev)->pgm_db) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  /* Alocate memory for all stages in a pipe */
  for (p = 0; p < dev_pipes; p++) {
    PIPE_SEL_SHADOW_DB(dev)->power_db[p] = PIPE_MGR_CALLOC(
        dev_stages, sizeof(*PIPE_SEL_SHADOW_DB(dev)->power_db[p]));
    if (!PIPE_SEL_SHADOW_DB(dev)->power_db[p]) goto cleanup;
    PIPE_SEL_SHADOW_DB(dev)->seed_db[p] = PIPE_MGR_CALLOC(
        dev_stages, sizeof(*PIPE_SEL_SHADOW_DB(dev)->seed_db[p]));
    if (!PIPE_SEL_SHADOW_DB(dev)->seed_db[p]) goto cleanup;
    PIPE_SEL_SHADOW_DB(dev)->pgm_db[p] = PIPE_MGR_CALLOC(
        dev_stages, sizeof(*PIPE_SEL_SHADOW_DB(dev)->pgm_db[p]));
    if (!PIPE_SEL_SHADOW_DB(dev)->pgm_db[p]) goto cleanup;
  }

  /* Set the error code incase of a failure. */
  sts = PIPE_INIT_ERROR;
  /* For each profile on the chip. */
  for (int prof_id = 0; prof_id < (int)dev_info->num_pipeline_profiles;
       ++prof_id) {
    /* Get the shadow of our config registers from the parsed context json
     * data. */
    struct pipe_config_cache_2d_byte_array_t *power_ctrl;
    struct pipe_config_cache_2d_byte_array_t *hash_seed;
    struct pipe_config_cache_2d_byte_array_t *hash_mask;
    if (BF_MAP_OK != bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                                pipe_cck_xbar_din_power_ctrl,
                                (void *)&power_ctrl)) {
      LOG_ERROR("Dev %d Failed to get input-xbar-power-ctrl from profile %d",
                dev,
                prof_id);
      goto cleanup;
    }
    if (BF_MAP_OK != bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                                pipe_cck_hash_seed,
                                (void *)&hash_seed)) {
      LOG_ERROR("Dev %d Failed to get dyn-sel hash seed from profile %d",
                dev,
                prof_id);
      goto cleanup;
    }
    if (BF_MAP_OK != bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                                pipe_cck_hash_parity_group_mask,
                                (void *)&hash_mask)) {
      LOG_ERROR("Dev %d Failed to get dyn-sel hash mask from profile %d",
                dev,
                prof_id);
      goto cleanup;
    }

    /* Populate our shadows with the data. */
    unsigned int pipe_id;
    PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp, pipe_id) {
      for (s_idx = 0; s_idx < dev_stages; s_idx++) {
        for (int row = 0; row < PIPE_SEL_POWER_CTL_ROWS; ++row) {
          for (int col = 0; col < PIPE_SEL_POWER_CTL_COLS; ++col) {
            int idx = (row * PIPE_SEL_POWER_CTL_COLS + col) * 4;
            uint32_t val = 0;
            for (int byte = 0; byte < 4; ++byte) {
              val = (val << 8) | power_ctrl->val[s_idx][idx + byte];
            }
            PIPE_SEL_SHADOW_DB(dev)
                ->power_db[pipe_id][s_idx]
                .power_ctl[row][col] = val;
          }
        }
        for (int seed = 0; seed < PIPE_SEL_HASH_SEEDS_NUM; ++seed) {
          int idx = seed * 4;
          uint32_t val = 0;
          for (int byte = 0; byte < 4; ++byte) {
            val = (val << 8) | hash_seed->val[s_idx][idx + byte];
          }
          PIPE_SEL_SHADOW_DB(dev)->seed_db[pipe_id][s_idx].hash_seed[seed] =
              val;
        }
        for (int row = 0; row < PIPE_SEL_PGM_ROWS; ++row) {
          for (int col = 0; col < PIPE_SEL_PGM_COLS; ++col) {
            int idx = (row * PIPE_SEL_PGM_COLS + col) * 4;
            uint32_t val = 0;
            for (int byte = 0; byte < 4; ++byte) {
              val = (val << 8) | hash_mask->val[s_idx][idx + byte];
            }
            PIPE_SEL_SHADOW_DB(dev)->pgm_db[pipe_id][s_idx].pgm[row][col] = val;
          }
        }
      }
    }
  }

  return PIPE_SUCCESS;
cleanup:
  if (PIPE_SEL_SHADOW_DB(dev)->power_db) {
    PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->power_db);
  }
  if (PIPE_SEL_SHADOW_DB(dev)->seed_db) {
    PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->seed_db);
  }
  if (PIPE_SEL_SHADOW_DB(dev)->pgm_db) {
    PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev)->pgm_db);
  }

  if (PIPE_SEL_SHADOW_DB(dev)) {
    PIPE_MGR_FREE(PIPE_SEL_SHADOW_DB(dev));
    PIPE_SEL_SHADOW_DB(dev) = NULL;
  }
  return sts;
}

static void pipe_mgr_hash_calc_seed(pipe_dhash_info_t *dhash) {
  if (!dhash || !PIPE_SEL_SHADOW_DB(dhash->dev_id) ||
      dhash->num_hash_configs == 0) {
    return;
  }

  dhash->curr_seed_value = 0;
  uint32_t i, j, k;
  uint32_t val = 0, bit_val = 0;
  uint32_t pipe = PIPE_BITMAP_GET_FIRST_SET(&dhash->pipe_bmp);
  pipe_dhash_hash_config_t *hash_config = NULL;
  for (k = 0; k < dhash->num_hash_configs; k++) {
    hash_config = &(dhash->hash_configs[k]);
    for (i = 0; i < PIPE_SEL_HASH_SEEDS_NUM; i++) {
      val = PIPE_SEL_SHADOW_DB(dhash->dev_id)
                ->seed_db[pipe][hash_config->stage_id]
                .hash_seed[i];

      for (j = 0; j < hash_config->hash.num_hash_bits; j++) {
        if (hash_config->hash.hash_bits[j].gfm_start_bit == i) {
          bit_val = (val >> hash_config->hash.hash_id) & 0x1;
          dhash->curr_seed_value |=
              (bit_val << hash_config->hash.hash_bits[j].p4_hash_output_bit);
        }
      }

      for (j = 0; j < hash_config->hash_mod.num_hash_bits; j++) {
        if (hash_config->hash_mod.hash_bits[j].gfm_start_bit == i) {
          bit_val = (val >> hash_config->hash_mod.hash_id) & 0x1;
          dhash->curr_seed_value |=
              (bit_val
               << hash_config->hash_mod.hash_bits[j].p4_hash_output_bit);
        }
      }
    }
  }
}

/* Set the defaults for field-list, algo and seed during init */
pipe_status_t pipe_mgr_hash_calc_init_defaults(bf_dev_id_t dev_id) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_field_list_t *field_list = NULL;
  uint32_t attr_size = 0;
  uint32_t p = 0, i = 0;
  unsigned long key = 0;
  pipe_hash_calc_hdl_t hdl;
  pipe_bitmap_t *pipe_bmp;
  bf_map_sts_t msts = BF_MAP_OK;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    return PIPE_UNEXPECTED;
  }

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    pipe_bmp = &dev_info->profile_info[p]->pipe_bmp;
    msts = bf_map_get_first(
        &dev_info->profile_info[p]->dhash_info, &key, (void **)&dhash);

    while (msts == BF_MAP_OK) {
      hdl = key;
      LOG_TRACE("%s: dev %d, Dyn hash calc hdl 0x%x, init defaults \n",
                __func__,
                dev_id,
                hdl);
      /* Populate the dev-id and pipe_bmp */
      dhash->dev_id = dev_id;
      PIPE_BITMAP_INIT(&dhash->pipe_bmp, PIPE_BMP_SIZE);
      PIPE_BITMAP_ASSIGN(&dhash->pipe_bmp, pipe_bmp);

      /* Populate default attributes */
      for (i = 0; i < dhash->num_field_lists; i++) {
        if (dhash->field_lists[i].handle == dhash->def_field_list_hdl) {
          field_list = &(dhash->field_lists[i]);
          break;
        }
      }
      if (!field_list) {
        LOG_ERROR(
            "%s:%d Invalid init field list handle 0x%x for hash calculation "
            "0x%x",
            __func__,
            __LINE__,
            dhash->def_field_list_hdl,
            hdl);
        return PIPE_INVALID_ARG;
      }
      if (field_list->num_fields > 0) {
        /* Build the default field list by including all fields in p4 order */
        dhash->num_curr_fields = field_list->num_fields;
        attr_size = field_list->num_fields *
                    sizeof(pipe_hash_calc_input_field_attribute_t);
        dhash->curr_field_attrs =
            PIPE_MGR_REALLOC(dhash->curr_field_attrs, attr_size);
        for (i = 0; i < field_list->num_fields; i++) {
          dhash->curr_field_attrs[i].input_field = i;
          dhash->curr_field_attrs[i].slice_start_bit = 0;
          dhash->curr_field_attrs[i].slice_length = 0;
          dhash->curr_field_attrs[i].order = i + 1;
          dhash->curr_field_attrs[i].type = INPUT_FIELD_ATTR_TYPE_MASK;
          dhash->curr_field_attrs[i].value.mask = INPUT_FIELD_INCLUDED;
        }
      }

      /* Populate default seed */
      pipe_mgr_hash_calc_seed(dhash);
      dhash->def_seed_value = dhash->curr_seed_value;

      msts = bf_map_get_next(
          &dev_info->profile_info[p]->dhash_info, &key, (void **)&dhash);
    }
  }

  return PIPE_SUCCESS;
}

/* Write GFM */
static pipe_status_t pipe_mgr_hash_calc_hash_gfm_write(pipe_sess_hdl_t sess_hdl,
                                                       rmt_dev_info_t *dev_info,
                                                       uint32_t stage,
                                                       pipe_dhash_info_t *dhash,
                                                       hash_regs_t *hash_regs) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t i = 0, addr = 0, value = 0;
  galois_field_matrix_delta_t *gfm = NULL;
  pipe_instr_write_reg_t instr;

  /* Go over GFM */
  for (i = 0; i < hash_regs->gfm_sz; i++) {
    gfm = &(hash_regs->galois_field_matrix_regs[i]);
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        addr = offsetof(
            Tofino,
            pipes[0]
                .mau[stage]
                .dp.xbar_hash.hash
                .galois_field_matrix[gfm->byte_pair_index][gfm->hash_bit]);
        value = (gfm->byte0 & 0xff) | ((gfm->valid0 & 0x1u) << 8) |
                ((gfm->byte1 & 0xff) << 9) | ((gfm->valid1 & 0x1u) << 17);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        addr = offsetof(
            tof2_reg,
            pipes[0]
                .mau[stage]
                .dp.xbar_hash.hash
                .galois_field_matrix[gfm->byte_pair_index][gfm->hash_bit]);
        value = (gfm->byte0 & 0xff) | ((gfm->byte1 & 0xff) << 8);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        addr = offsetof(
            tof3_reg,
            pipes[0]
                .mau[stage]
                .dp.xbar_hash.hash
                .galois_field_matrix[gfm->byte_pair_index][gfm->hash_bit]);
        value = (gfm->byte0 & 0xff) | ((gfm->byte1 & 0xff) << 8);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    construct_instr_reg_write(dev_id, &instr, addr, value);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    dev_info,
                                    &dhash->pipe_bmp,
                                    stage,
                                    (uint8_t *)&instr,
                                    sizeof instr);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to add Hash GFM to instruction list, stage %d, rc = (%d), "
          "dev_id %d",
          stage,
          status,
          dev_id);
      return status;
    }
  }

  /* Update GFM shadow for all entries */
  for (i = 0; i < hash_regs->gfm_sz; i++) {
    gfm = &(hash_regs->galois_field_matrix_regs[i]);

    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        value = (gfm->byte0 & 0xff) | ((gfm->valid0 & 0x1u) << 8) |
                ((gfm->byte1 & 0xff) << 9) | ((gfm->valid1 & 0x1u) << 17);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        value = (gfm->byte0 & 0xff) | ((gfm->byte1 & 0xff) << 8);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        value = (gfm->byte0 & 0xff) | ((gfm->byte1 & 0xff) << 8);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    pipe_mgr_update_gfm_shadow(dev_info,
                               &dhash->pipe_bmp,
                               stage,
                               gfm->byte_pair_index,
                               gfm->hash_bit,
                               value);
  }

  status = pipe_mgr_recalc_write_gfm_parity(
      sess_hdl, dev_info, &dhash->pipe_bmp, stage, false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to add rewrite GFM parity, stage %d, rc = (%d), "
        "dev_id %d",
        stage,
        status,
        dev_id);
  }

  return status;
}

/* Write hash seed */
static pipe_status_t pipe_mgr_hash_calc_hash_seed_write(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint32_t stage,
    pipe_dhash_info_t *dhash,
    hash_regs_t *hash_regs) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t i = 0, pipe = 0, addr = 0;
  uint32_t old_value = 0, new_value = 0;
  hash_seed_delta_t *seed = NULL;
  pipe_instr_write_reg_t instr;

  /* Get lowest pipe of profile to update shadow DB */
  pipe = PIPE_BITMAP_GET_FIRST_SET(&dhash->pipe_bmp);

  /* Go over all the bits of the seed */
  for (i = 0; i < hash_regs->hs_sz; i++) {
    seed = &(hash_regs->hash_seed_regs[i]);

    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        addr = offsetof(
            Tofino,
            pipes[0].mau[stage].dp.xbar_hash.hash.hash_seed[seed->hash_bit]);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        addr = offsetof(
            tof2_reg,
            pipes[0].mau[stage].dp.xbar_hash.hash.hash_seed[seed->hash_bit]);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        addr = offsetof(
            tof3_reg,
            pipes[0].mau[stage].dp.xbar_hash.hash.hash_seed[seed->hash_bit]);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
    old_value = PIPE_SEL_SHADOW_DB(dev_id)
                    ->seed_db[pipe][stage]
                    .hash_seed[seed->hash_bit];

    /* Bits 0-7 are the hash_seed */
    new_value =
        (old_value & seed->hash_seed_and_value) | seed->hash_seed_or_value;

    construct_instr_reg_write(dev_id, &instr, addr, new_value);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    dev_info,
                                    &dhash->pipe_bmp,
                                    stage,
                                    (uint8_t *)&instr,
                                    sizeof instr);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to add Hash seed to instruction list, stage %d, rc = "
          "(%d)",
          stage,
          status);
    }

    PIPE_SEL_SHADOW_DB(dev_id)->seed_db[pipe][stage].hash_seed[seed->hash_bit] =
        new_value;
  }

  return status;
}

static pipe_status_t pipe_mgr_hash_calc_ixbar_input_set(
    const pipe_dhash_info_t *dhash,
    const pipe_dhash_field_list_t *field_list,
    const pipe_dhash_crossbar_config_t *crossbar_config,
    bool is_crossbar_mod,
    ixbar_input_t *ixbar_input,
    uint32_t *input_sz) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_dhash_field_t *field = NULL;
  pipe_dhash_crossbar_t *crossbar = NULL;
  uint32_t j = 0, field_bit = 0;
  int slice_start_bit = 0, slice_length = 0;
  uint32_t num_crossbars = is_crossbar_mod == true
                               ? crossbar_config->num_crossbar_mods
                               : crossbar_config->num_crossbars;
  uint32_t max_sib_group = 1;
  uint32_t *groups = NULL, *sib_groups = NULL;

  groups = PIPE_MGR_CALLOC(dhash->num_curr_fields, sizeof(uint32_t));
  if (!groups) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    status = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  sib_groups = PIPE_MGR_CALLOC(dhash->num_curr_fields, sizeof(uint32_t));
  if (!sib_groups) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    status = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  *input_sz = 0;
  /* Construct the ixbar_input list (lsb to msb) */
  for (int k = dhash->num_curr_fields - 1; k >= 0; k--) {
    if (dhash->curr_field_attrs[k].value.mask == INPUT_FIELD_EXCLUDED) {
      continue;
    }
    /* For this field attr, figure out if it is a symmetric field. If
     * yes, then find its corresponding symmetric brother and mark
     */
    bool is_symmetric = false;
    for (int i = dhash->num_curr_fields - 1; i >= 0; i--) {
      if (i == k) continue;
      // if same order was found for another attr
      if (dhash->curr_field_attrs[k].order ==
          dhash->curr_field_attrs[i].order) {
        is_symmetric = true;
        // If first time encountering this pair
        if (groups[i] == 0) {
          groups[k] = max_sib_group;
          sib_groups[k] = max_sib_group + 1;
          max_sib_group += 2;
        } else {
          groups[k] = sib_groups[i];
          sib_groups[k] = groups[i];
        }
        LOG_DBG(
            "%s:%d Setting field %s and %s as symmetric. "
            "group:%d sib_group:%d for first",
            __func__,
            __LINE__,
            field_list->fields[dhash->curr_field_attrs[k].input_field].name,
            field_list->fields[dhash->curr_field_attrs[i].input_field].name,
            groups[k],
            sib_groups[k]);
        break;
      }
    }
    /*
     * field_start_bit(fs), field_length(fl)(field->bit_width),
     * field_width(fw)
     * slice_start_bit(ss), slice_length(sl)
     *
     * Let fs = 9, fl = 23 (For lets say src_ip fw = 32; hdr.ipv4.sip[31:9])
     * Then ss can range from [0, fl-1], sl can range from [1, fl-ss]
     * or ss= [0, 22], sl=[1,23-ss].
     * If sl is 0, then no need to worry about ss/sl
     * If the conditions aren't met, then log error and cleanup
     *
     * Let ss = 5 and sl = 12
     * the field_bit loop should run from [9+5, 9+5+12-1] = [14,25]
     * or [fs+ss, fs+ss+sl)
     */

    field = &(field_list->fields[dhash->curr_field_attrs[k].input_field]);
    slice_start_bit = 0;
    slice_length = 0;
    if (dhash->curr_field_attrs[k].slice_start_bit != 0) {
      if (dhash->curr_field_attrs[k].slice_start_bit >= field->bit_width) {
        LOG_ERROR(
            "%s:%d Error processing slice start bit %d for field %s"
            " field_start_bit %d field_bit_width %d hash calc 0x%x",
            __func__,
            __LINE__,
            dhash->curr_field_attrs[k].slice_start_bit,
            field->name,
            field->start_bit,
            field->bit_width,
            dhash->handle);
        status = PIPE_INVALID_ARG;
        goto cleanup;
      }
    }
    slice_start_bit = dhash->curr_field_attrs[k].slice_start_bit;
    if (dhash->curr_field_attrs[k].slice_length != 0) {
      if (dhash->curr_field_attrs[k].slice_length >
          field->bit_width - slice_start_bit) {
        LOG_ERROR(
            "%s:%d Error processing slice start_bit %d slice length %d "
            "for field %s field_start_bit %d field_bit_width %d hash calc 0x%x",
            __func__,
            __LINE__,
            dhash->curr_field_attrs[k].slice_start_bit,
            dhash->curr_field_attrs[k].slice_length,
            field->name,
            field->start_bit,
            field->bit_width,
            dhash->handle);
        status = PIPE_INVALID_ARG;
        goto cleanup;
      }
      slice_length = dhash->curr_field_attrs[k].slice_length;
    } else {
      slice_length = field->bit_width;
    }
    for (field_bit = field->start_bit + slice_start_bit;
         field_bit < field->start_bit + slice_start_bit + slice_length;
         ++field_bit) {
      for (j = 0; j < num_crossbars; j++) {
        crossbar = is_crossbar_mod == true
                       ? &(crossbar_config->crossbar_mods[j])
                       : &(crossbar_config->crossbars[j]);
        /* Add to input if it's the right field bit */
        if (crossbar->field_bit == field_bit &&
            !strcmp(field->name, crossbar->name)) {
          ixbar_input[*input_sz].type = tPHV;
          ixbar_input[*input_sz].ixbar_bit_position =
              crossbar->byte_number * 8 + crossbar->bit_in_byte;
          ixbar_input[*input_sz].bit_size = 1;
          ixbar_input[*input_sz].symmetric_info.is_symmetric = is_symmetric;
          ixbar_input[*input_sz].symmetric_info.sym_group = groups[k];
          ixbar_input[*input_sz].symmetric_info.sib_sym_group = sib_groups[k];
          ixbar_input[*input_sz].u.valid = true;
          (*input_sz)++;
          LOG_DBG(
              "%s:%d Setting field %s field_bit %d ss %d sl %d "
              " fs %d fl %d ixbar_pos %d hash calc 0x%x",
              __func__,
              __LINE__,
              field->name,
              field_bit,
              dhash->curr_field_attrs[k].slice_start_bit,
              dhash->curr_field_attrs[k].slice_length,
              field->start_bit,
              field->bit_width,
              *input_sz - 1,
              dhash->handle);
          break;
        }
      }
    }
  }
cleanup:
  if (groups) PIPE_MGR_FREE(groups);
  if (sib_groups) PIPE_MGR_FREE(sib_groups);
  return status;
}

/* Populate the bit positions of the gfm and p4 hash bit */
static void pipe_mgr_hash_calc_populate_gfm_p4out_bit_map(
    pipe_dhash_hash_t hash_info, hash_calc_rotate_info_t *rot_info) {
  hash_matrix_output_t *hash_bit = NULL;
  for (uint32_t j = 0; j < hash_info.num_hash_bits; j++) {
    hash_bit = &(hash_info.hash_bits[j]);
    rot_info->p4_hash_output_bit_posn[j] = hash_bit->p4_hash_output_bit;
    rot_info->gfm_bit_posn[hash_bit->p4_hash_output_bit] =
        hash_bit->gfm_start_bit;
  }
  return;
}

/* Common API to set new config based on field_list and algo handle */
static pipe_status_t pipe_mgr_hash_calc_input_algorithm_set(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_dhash_info_t *dhash) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_dhash_field_list_t *field_list = NULL;
  pipe_dhash_crossbar_config_t *crossbar_config = NULL;
  ixbar_input_t *ixbar_input = NULL;
  pipe_dhash_alg_t *algorithm = NULL;
  hash_regs_t hash_regs = {0};
  uint32_t input_sz = 0;
  uint32_t i = 0, idx = 0;
  pipe_hash_alg_hdl_t non_p4_algo_hdl = -1;
  pipe_dhash_hash_config_t *hash_config = NULL;
  hash_calc_rotate_info_t *rot_info = NULL;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < dhash->num_field_lists; i++) {
    if (dhash->field_lists[i].handle == dhash->curr_field_list_hdl) {
      field_list = &(dhash->field_lists[i]);
      break;
    }
  }
  if (!field_list) {
    PIPE_MGR_DBGCHK(field_list);
    return PIPE_UNEXPECTED;
  }

  if (dhash->curr_algo_hdl == non_p4_algo_hdl) {
    /* This means that the last element in the algo array
     * which contains a non-context.json algo needs to be used
     */
    algorithm = &(dhash->algorithms[dhash->num_algorithms - 1]);
  } else {
    for (i = 0; i < dhash->num_algorithms; i++) {
      if (dhash->algorithms[i].handle == dhash->curr_algo_hdl) {
        algorithm = &(dhash->algorithms[i]);
        break;
      }
    }
  }
  if (!algorithm) {
    PIPE_MGR_DBGCHK(algorithm);
    return PIPE_UNEXPECTED;
  }
  rot_info = PIPE_MGR_MALLOC(sizeof(hash_calc_rotate_info_t));
  if (!rot_info) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Hash info for rotation */
  rot_info->rotate = dhash->rotate;
  rot_info->gfm_bit_posn =
      PIPE_MGR_CALLOC(dhash->hash_bit_width, sizeof(uint32_t));
  if (!rot_info->gfm_bit_posn) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    PIPE_MGR_FREE(rot_info);
    return PIPE_NO_SYS_RESOURCES;
  }
  rot_info->p4_hash_output_bit_posn =
      PIPE_MGR_CALLOC(dhash->hash_bit_width, sizeof(uint32_t));
  if (!rot_info->p4_hash_output_bit_posn) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    PIPE_MGR_FREE(rot_info->gfm_bit_posn);
    PIPE_MGR_FREE(rot_info);
    return PIPE_NO_SYS_RESOURCES;
  }
  for (idx = 0; idx < field_list->num_crossbar_configs; idx++) {
    crossbar_config = &(field_list->crossbar_configs[idx]);
    /* Crossbar and hash */
    ixbar_input = PIPE_MGR_REALLOC(
        ixbar_input, crossbar_config->num_crossbars * sizeof(ixbar_input_t));
    pipe_mgr_hash_calc_ixbar_input_set(
        dhash, field_list, crossbar_config, false, ixbar_input, &input_sz);
    hash_config = &(dhash->hash_configs[idx]);
    pipe_mgr_hash_calc_populate_gfm_p4out_bit_map(hash_config->hash, rot_info);
    rot_info->num_hash_bits = hash_config->hash.num_hash_bits;
    /* Get gfm and seed programming */
    determine_tofino_regs(&(crossbar_config->ixbar_init),
                          ixbar_input,
                          input_sz,
                          &(algorithm->hash_alg),
                          rot_info,
                          &hash_regs);

    /* Write GFM */
    status = pipe_mgr_hash_calc_hash_gfm_write(
        sess_hdl, dev_info, crossbar_config->stage_id, dhash, &hash_regs);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error writing gfm for stage %d hash calc 0x%x dev %d",
                __func__,
                __LINE__,
                crossbar_config->stage_id,
                dhash->handle,
                dev_id);
      goto cleanup;
    }

    /* Write the seed. Note that the seed which is written is the
     * one which is determined by determine_tofino_regs() and not the
     * one which is present in dhash. In fact the dhash value is
     * overwritten by pipe_mgr_hash_calc_seed() which reads back from
     * the registers and puts back in dhash.
     */

    status = pipe_mgr_hash_calc_hash_seed_write(
        sess_hdl, dev_info, crossbar_config->stage_id, dhash, &hash_regs);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error writing seed for stage %d hash calc 0x%x dev %d",
                __func__,
                __LINE__,
                crossbar_config->stage_id,
                dhash->handle,
                dev_id);
      goto cleanup;
    }

    /* Free the struct lists */
    if (hash_regs.galois_field_matrix_regs) {
      PIPE_MGR_FREE(hash_regs.galois_field_matrix_regs);
      hash_regs.galois_field_matrix_regs = NULL;
    }
    if (hash_regs.hash_seed_regs) {
      PIPE_MGR_FREE(hash_regs.hash_seed_regs);
      hash_regs.hash_seed_regs = NULL;
    }

    /* Crossbar mod and hash mod */
    ixbar_input = PIPE_MGR_REALLOC(
        ixbar_input,
        crossbar_config->num_crossbar_mods * sizeof(ixbar_input_t));

    pipe_mgr_hash_calc_ixbar_input_set(
        dhash, field_list, crossbar_config, true, ixbar_input, &input_sz);

    /* Get gfm and seed programming */
    determine_tofino_regs(&(crossbar_config->ixbar_mod_init),
                          ixbar_input,
                          input_sz,
                          &(algorithm->hash_alg),
                          rot_info,
                          &hash_regs);

    /* Write GFM */
    status = pipe_mgr_hash_calc_hash_gfm_write(
        sess_hdl, dev_info, crossbar_config->stage_id, dhash, &hash_regs);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error writing gfm for stage %d hash calc 0x%x dev %d",
                __func__,
                __LINE__,
                crossbar_config->stage_id,
                dhash->handle,
                dev_id);
      goto cleanup;
    }

    /* Write the seed */
    status = pipe_mgr_hash_calc_hash_seed_write(
        sess_hdl, dev_info, crossbar_config->stage_id, dhash, &hash_regs);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error writing seed for stage %d hash calc 0x%x dev %d",
                __func__,
                __LINE__,
                crossbar_config->stage_id,
                dhash->handle,
                dev_id);
      goto cleanup;
    }

    /* Free the struct lists */
    if (hash_regs.galois_field_matrix_regs) {
      PIPE_MGR_FREE(hash_regs.galois_field_matrix_regs);
      hash_regs.galois_field_matrix_regs = NULL;
    }
    if (hash_regs.hash_seed_regs) {
      PIPE_MGR_FREE(hash_regs.hash_seed_regs);
      hash_regs.hash_seed_regs = NULL;
    }
    status = pipe_mgr_recalc_write_seed_parity(
        sess_hdl, dev_info, &dhash->pipe_bmp, crossbar_config->stage_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to adjust seed for even parity, stage %d, rc = (%d)",
                crossbar_config->stage_id,
                status);
      if (ixbar_input) {
        PIPE_MGR_FREE(ixbar_input);
        ixbar_input = NULL;
      }
      return status;
    }
  }
  pipe_mgr_hash_calc_seed(dhash);

cleanup:
  if (ixbar_input) {
    PIPE_MGR_FREE(ixbar_input);
  }
  if (hash_regs.galois_field_matrix_regs) {
    PIPE_MGR_FREE(hash_regs.galois_field_matrix_regs);
  }
  if (hash_regs.hash_seed_regs) {
    PIPE_MGR_FREE(hash_regs.hash_seed_regs);
  }
  if (rot_info->gfm_bit_posn) {
    PIPE_MGR_FREE(rot_info->gfm_bit_posn);
  }
  if (rot_info->p4_hash_output_bit_posn) {
    PIPE_MGR_FREE(rot_info->p4_hash_output_bit_posn);
  }
  if (rot_info) {
    PIPE_MGR_FREE(rot_info);
  }
  return status;
}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_set_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list) {
  pipe_dhash_info_t *dhash = NULL;

  dhash = pipe_mgr_get_hash_calc_info(dev_id, handle);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (fl_handle != dhash->curr_field_list_hdl) {
    LOG_ERROR(
        "%s:%d Cannot set fields from list 0x%x for current list 0x%x in "
        "dynamic hash handle 0x%x device %d",
        __func__,
        __LINE__,
        fl_handle,
        dhash->curr_field_list_hdl,
        handle,
        dev_id);
    return PIPE_INVALID_ARG;
  }

  dhash->num_curr_fields = attr_count;
  if (attr_count) {
    uint32_t attr_size =
        attr_count * sizeof(pipe_hash_calc_input_field_attribute_t);
    dhash->curr_field_attrs =
        PIPE_MGR_REALLOC(dhash->curr_field_attrs, attr_size);
    PIPE_MGR_MEMCPY(dhash->curr_field_attrs, attr_list, attr_size);
  }

  return pipe_mgr_hash_calc_input_algorithm_set(sess_hdl, dev_id, dhash);
}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_2_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    pipe_hash_calc_input_field_attribute_t **attr_list,
    uint32_t *num_attr_filled) {
  uint32_t curr_attr_count = 0;
  pipe_status_t sts = pipe_mgr_hash_calc_input_field_attribute_count_get_ext(
      dev_id, handle, fl_handle, &curr_attr_count);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Dynamic hash attr count not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return sts;
  }
  if (curr_attr_count == 0) {
    LOG_DBG("%s:%d Dynamic hash attr count found as 0 for hdl 0x%x",
            __func__,
            __LINE__,
            handle);
    return sts;
  }
  *attr_list = PIPE_MGR_MALLOC(curr_attr_count *
                               sizeof(pipe_hash_calc_input_field_attribute_t));
  if (*attr_list == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  return pipe_mgr_hash_calc_input_field_attribute_get_ext(
      dev_id, handle, fl_handle, curr_attr_count, *attr_list, num_attr_filled);
}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t max_attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list,
    uint32_t *num_attr_filled) {
  pipe_dhash_info_t *dhash = NULL;

  dhash = pipe_mgr_get_hash_calc_info(dev_id, handle);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (fl_handle != dhash->curr_field_list_hdl) {
    LOG_ERROR(
        "%s:%d Cannot set fields from list 0x%x for current list 0x%x in "
        "dynamic hash handle 0x%x device %d",
        __func__,
        __LINE__,
        fl_handle,
        dhash->curr_field_list_hdl,
        handle,
        dev_id);
    return PIPE_INVALID_ARG;
  }

  *num_attr_filled = (dhash->num_curr_fields > max_attr_count)
                         ? max_attr_count
                         : dhash->num_curr_fields;
  PIPE_MGR_MEMCPY(
      attr_list,
      dhash->curr_field_attrs,
      *num_attr_filled * sizeof(pipe_hash_calc_input_field_attribute_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_count_get_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t *attr_count) {
  pipe_dhash_info_t *dhash = NULL;

  dhash = pipe_mgr_get_hash_calc_info(dev_id, handle);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (fl_handle != dhash->curr_field_list_hdl) {
    LOG_ERROR(
        "%s:%d Cannot set fields from list 0x%x for current list 0x%x in "
        "dynamic hash handle 0x%x device %d",
        __func__,
        __LINE__,
        fl_handle,
        dhash->curr_field_list_hdl,
        handle,
        dev_id);
    return PIPE_INVALID_ARG;
  }

  *attr_count = dhash->num_curr_fields;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hash_calc_input_default_set_ext(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_hash_calc_hdl_t hdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_field_list_t *field_list = NULL;
  uint32_t i = 0;

  LOG_TRACE("%s: dev %d, Dynamic Hash calc hdl 0x%x set to default attributes ",
            __func__,
            dev_id,
            hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < dhash->num_field_lists; i++) {
    if (dhash->field_lists[i].handle == dhash->def_field_list_hdl) {
      field_list = &(dhash->field_lists[i]);
      break;
    }
  }
  if (!field_list) {
    LOG_ERROR(
        "%s:%d Default field list handle 0x%x for hash_calc 0x%x not found",
        __func__,
        __LINE__,
        dhash->def_field_list_hdl,
        hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  dhash->curr_field_list_hdl = dhash->def_field_list_hdl;

  if (field_list->num_fields > 0) {
    /* Build the default field list by including all fields in the p4 order */
    pipe_hash_calc_input_field_attribute_t *attr_list = PIPE_MGR_CALLOC(
        field_list->num_fields, sizeof(pipe_hash_calc_input_field_attribute_t));
    if (!attr_list) {
      LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    for (i = 0; i < field_list->num_fields; i++) {
      attr_list[i].input_field = i;
      attr_list[i].slice_start_bit = 0;
      attr_list[i].slice_length = 0;
      attr_list[i].order = i + 1;
      attr_list[i].type = INPUT_FIELD_ATTR_TYPE_MASK;
      attr_list[i].value.mask = INPUT_FIELD_INCLUDED;
    }

    sts = pipe_mgr_hash_calc_input_field_attribute_set_ext(
        sess_hdl,
        dev_id,
        hdl,
        dhash->def_field_list_hdl,
        field_list->num_fields,
        attr_list);
    PIPE_MGR_FREE(attr_list);
  }

  return sts;
}

pipe_status_t pipe_mgr_hash_calc_input_set_ext(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t hdl,
                                               pipe_fld_lst_hdl_t fl_hdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_field_list_t *field_list = NULL;
  uint32_t i = 0;

  LOG_TRACE(
      "%s: dev %d, Dynamic Hash calculation hdl 0x%x Change field-list to hdl "
      "%u ",
      __func__,
      dev_id,
      hdl,
      fl_hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (fl_hdl == dhash->curr_field_list_hdl) {
    LOG_TRACE(
        "%s:%d Dynamic hash calculation 0x%x, No change in field list hdl "
        "0x%x ",
        __func__,
        __LINE__,
        hdl,
        fl_hdl);
    return PIPE_SUCCESS;
  }

  for (i = 0; i < dhash->num_field_lists; i++) {
    if (dhash->field_lists[i].handle == fl_hdl) {
      field_list = &(dhash->field_lists[i]);
      break;
    }
  }
  if (!field_list) {
    LOG_ERROR("%s:%d Invalid field list handle 0x%x for hash calculation 0x%x",
              __func__,
              __LINE__,
              fl_hdl,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  dhash->curr_field_list_hdl = fl_hdl;

  if (field_list->num_fields > 0) {
    /* Build the default field list by including all fields in the p4 order */
    pipe_hash_calc_input_field_attribute_t *attr_list = PIPE_MGR_CALLOC(
        field_list->num_fields, sizeof(pipe_hash_calc_input_field_attribute_t));
    if (!attr_list) {
      LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    for (i = 0; i < field_list->num_fields; i++) {
      attr_list[i].input_field = i;
      attr_list[i].slice_start_bit = 0;
      attr_list[i].slice_length = 0;
      attr_list[i].order = i + 1;
      attr_list[i].type = INPUT_FIELD_ATTR_TYPE_MASK;
      attr_list[i].value.mask = INPUT_FIELD_INCLUDED;
    }

    sts = pipe_mgr_hash_calc_input_field_attribute_set_ext(
        sess_hdl, dev_id, hdl, fl_hdl, field_list->num_fields, attr_list);
    PIPE_MGR_FREE(attr_list);
  }

  return sts;
}

pipe_status_t pipe_mgr_hash_calc_input_get_ext(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t hdl,
                                               pipe_fld_lst_hdl_t *fl_hdl) {
  pipe_dhash_info_t *dhash = NULL;

  (void)sess_hdl;

  LOG_TRACE(
      "%s: dev %d, Dynamic hash calculation hdl 0x%x Get field-list hdl \n",
      __func__,
      dev_id,
      hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *fl_hdl = dhash->curr_field_list_hdl;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_hash_calc_algorithm_nonp4_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t hdl,
    const bfn_hash_algorithm_t *algorithm_in,
    uint64_t rotate) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  int j = 0;

  LOG_TRACE(
      "%s: dev %d, Dynamic hash calculation hdl 0x%x Algo to %d crc to %d",
      __func__,
      dev_id,
      hdl,
      algorithm_in->hash_alg,
      algorithm_in->crc_type);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  dhash->curr_algo_hdl = -1;
  dhash->rotate = rotate;
  bfn_hash_algorithm_t *algorithm =
      &dhash->algorithms[dhash->num_algorithms - 1].hash_alg;
  /* If crc_matrix already exists then free it */
  if (algorithm->crc_matrix) {
    for (j = 0; j < 256; j++) {
      if (algorithm->crc_matrix[j]) {
        PIPE_MGR_FREE(algorithm->crc_matrix[j]);
      }
    }
    PIPE_MGR_FREE(algorithm->crc_matrix);
    algorithm->crc_matrix = NULL;
  }
  /* Copy the alg in pipe_mgr data structure */
  PIPE_MGR_MEMCPY(algorithm, algorithm_in, sizeof(bfn_hash_algorithm_t));
  algorithm->crc_matrix = PIPE_MGR_CALLOC(256, sizeof(uint8_t *));
  if (!algorithm->crc_matrix) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    status = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (j = 0; j < 256; j++) {
    algorithm->crc_matrix[j] =
        PIPE_MGR_CALLOC((algorithm->hash_bit_width + 7) / 8, sizeof(uint8_t));
    if (!algorithm->crc_matrix[j]) {
      LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
      status = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }
  initialize_crc_matrix(algorithm);

  status = pipe_mgr_hash_calc_input_algorithm_set(sess_hdl, dev_id, dhash);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Uable to set algorithm for hdl 0x%x", __func__, __LINE__, hdl);
    goto cleanup;
  }
  return status;

cleanup:
  if (algorithm->crc_matrix) {
    for (j = 0; j < 256; j++) {
      if (algorithm->crc_matrix[j]) {
        PIPE_MGR_FREE(algorithm->crc_matrix[j]);
      }
    }
    PIPE_MGR_FREE(algorithm->crc_matrix);
    algorithm->crc_matrix = NULL;
  }
  return status;
}

pipe_status_t pipe_mgr_hash_calc_algorithm_reset_ext(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     pipe_hash_calc_hdl_t hdl) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  status = pipe_mgr_hash_calc_algorithm_set_ext(
      sess_hdl, dev_id, hdl, dhash->def_algo_hdl, NULL, 0);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Unable to set algo to default hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return status;
  }
  return pipe_mgr_hash_calc_seed_set_ext(
      sess_hdl, dev_id, hdl, dhash->def_seed_value);
}

pipe_status_t pipe_mgr_hash_calc_algorithm_set_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t hdl,
    pipe_hash_alg_hdl_t algo_hdl,
    const bfn_hash_algorithm_t *algorithm_out,
    uint64_t rotate) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_alg_t *algorithm = NULL;
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_hash_alg_hdl_t non_p4_algo_hdl = -1;

  LOG_TRACE(
      "%s: dev %d, Dynamic hash calculation hdl 0x%x Change algo to hdl %u \n",
      __func__,
      dev_id,
      hdl,
      algo_hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (algo_hdl == dhash->curr_algo_hdl && algo_hdl != non_p4_algo_hdl) {
    LOG_TRACE(
        "%s:%d Dynamic hash calculation 0x%x, No change in algo hdl 0x%x ",
        __func__,
        __LINE__,
        hdl,
        algo_hdl);
    return PIPE_SUCCESS;
  }

  if (algo_hdl == non_p4_algo_hdl) {
    if (algorithm_out) {
      sts = pipe_mgr_hash_calc_algorithm_nonp4_set(
          sess_hdl, dev_id, hdl, algorithm_out, rotate);
    } else {
      LOG_ERROR(
          "%s:%d Algorithm handle 0x%x requires a bfn_hash_algorithm_t  0x%x",
          __func__,
          __LINE__,
          algo_hdl,
          hdl);
      return PIPE_INVALID_ARG;
    }
  } else {
    for (uint32_t i = 0; i < dhash->num_algorithms; i++) {
      if (dhash->algorithms[i].handle == algo_hdl) {
        algorithm = &(dhash->algorithms[i]);
        break;
      }
    }
    if (!algorithm) {
      LOG_ERROR("%s:%d Invalid algorithm handle 0x%x for hash calculation 0x%x",
                __func__,
                __LINE__,
                algo_hdl,
                hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    dhash->curr_algo_hdl = algo_hdl;
    dhash->rotate = rotate;
    sts = pipe_mgr_hash_calc_input_algorithm_set(sess_hdl, dev_id, dhash);
  }
  return sts;
}

pipe_status_t pipe_mgr_hash_calc_algorithm_get_ext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t hdl,
    pipe_hash_alg_hdl_t *algo_hdl,
    bfn_hash_algorithm_t *algorithm,
    uint64_t *rotate) {
  pipe_dhash_info_t *dhash = NULL;
  bfn_hash_algorithm_t *curr_alg = NULL;
  uint32_t i = 0;

  (void)sess_hdl;

  LOG_TRACE("%s: dev %d, Dynamic hash calculation hdl 0x%x Get algo hdl \n",
            __func__,
            dev_id,
            hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *algo_hdl = dhash->curr_algo_hdl;
  if (algorithm) {
    for (i = 0; i < dhash->num_algorithms; i++) {
      if (dhash->algorithms[i].handle == dhash->curr_algo_hdl) {
        curr_alg = &dhash->algorithms[i].hash_alg;
        break;
      }
    }
    if (!curr_alg) {
      LOG_ERROR("%s:%d Error finding current algorithm 0x%x",
                __func__,
                __LINE__,
                dhash->curr_algo_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* Copy the alg from pipe_mgr data structure */
    PIPE_MGR_MEMCPY(algorithm, curr_alg, sizeof(bfn_hash_algorithm_t));
  }
  if (rotate) {
    *rotate = dhash->rotate;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_hash_calc_seed_set_int(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     pipe_dhash_info_t *dhash) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_dhash_hash_config_t *hash_config = NULL;
  hash_matrix_output_t *hash_bit = NULL;
  pipe_instr_write_reg_t instr;
  uint32_t i = 0, j = 0;
  uint32_t pipe = 0, addr = 0, value = 0;
  uint64_t seed = dhash->curr_seed_value, seed_bit = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get lowest pipe of profile to update shadow DB */
  pipe = PIPE_BITMAP_GET_FIRST_SET(&dhash->pipe_bmp);

  for (i = 0; i < dhash->num_hash_configs; i++) {
    hash_config = &(dhash->hash_configs[i]);
    uint32_t num_bits = hash_config->hash.num_hash_bits;
    for (j = 0; j < num_bits; j++) {
      int idx = (j + dhash->rotate) % num_bits;
      hash_bit = &(hash_config->hash.hash_bits[j]);
      int result_bit = hash_bit->p4_hash_output_bit;
      hash_bit = &(hash_config->hash.hash_bits[idx]);
      switch (dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          addr = offsetof(
              Tofino,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          addr = offsetof(
              tof2_reg,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;
        case BF_DEV_FAMILY_TOFINO3:
          addr = offsetof(
              tof3_reg,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;

        case BF_DEV_FAMILY_UNKNOWN:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
      }
      value = PIPE_SEL_SHADOW_DB(dev_id)
                  ->seed_db[pipe][hash_config->stage_id]
                  .hash_seed[hash_bit->gfm_start_bit];
      seed_bit = (seed >> result_bit) & 0x1;

      value &= ~(1 << hash_config->hash.hash_id);
      value |= seed_bit << hash_config->hash.hash_id;

      construct_instr_reg_write(dev_id, &instr, addr, value);

      status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                      dev_info,
                                      &dhash->pipe_bmp,
                                      hash_config->stage_id,
                                      (uint8_t *)&instr,
                                      sizeof instr);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to add Hash seed to instruction list, stage %d, rc = (%d)",
            hash_config->stage_id,
            status);
        return status;
      }

      PIPE_SEL_SHADOW_DB(dev_id)
          ->seed_db[pipe][hash_config->stage_id]
          .hash_seed[hash_bit->gfm_start_bit] = value;
    }

    for (j = 0; j < hash_config->hash_mod.num_hash_bits; j++) {
      hash_bit = &(hash_config->hash_mod.hash_bits[j]);
      switch (dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          addr = offsetof(
              Tofino,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          addr = offsetof(
              tof2_reg,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;
        case BF_DEV_FAMILY_TOFINO3:
          addr = offsetof(
              tof3_reg,
              pipes[0]
                  .mau[hash_config->stage_id]
                  .dp.xbar_hash.hash.hash_seed[hash_bit->gfm_start_bit]);
          break;

        case BF_DEV_FAMILY_UNKNOWN:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
      }
      value = PIPE_SEL_SHADOW_DB(dev_id)
                  ->seed_db[pipe][hash_config->stage_id]
                  .hash_seed[hash_bit->gfm_start_bit];
      seed_bit = (seed >> hash_bit->p4_hash_output_bit) & 0x1;

      value &= ~(1 << hash_config->hash_mod.hash_id);
      value |= seed_bit << hash_config->hash_mod.hash_id;

      construct_instr_reg_write(dev_id, &instr, addr, value);

      status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                      dev_info,
                                      &dhash->pipe_bmp,
                                      hash_config->stage_id,
                                      (uint8_t *)&instr,
                                      sizeof instr);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to add Hash seed to instruction list, stage %d, rc = (%d)",
            hash_config->stage_id,
            status);
        return status;
      }

      PIPE_SEL_SHADOW_DB(dev_id)
          ->seed_db[pipe][hash_config->stage_id]
          .hash_seed[hash_bit->gfm_start_bit] = value;
    }
    status = pipe_mgr_recalc_write_seed_parity(
        sess_hdl, dev_info, &dhash->pipe_bmp, hash_config->stage_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to adjust seed for even parity, stage %d, rc = (%d)",
                hash_config->stage_id,
                status);
      return status;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hash_calc_seed_set_ext(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_hash_calc_hdl_t hdl,
                                              pipe_hash_seed_t seed) {
  pipe_dhash_info_t *dhash = NULL;

  LOG_TRACE(
      "%s: dev %d, Dynamic Hash calculation hdl 0x%x Change seed value to "
      "%" PRIu64,
      __func__,
      dev_id,
      hdl,
      seed);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (seed == dhash->curr_seed_value) {
    LOG_TRACE(
        "%s:%d Dynamic hash calculation 0x%x, No change in seed value %" PRIu64,
        __func__,
        __LINE__,
        hdl,
        seed);
    return PIPE_SUCCESS;
  }

  dhash->curr_seed_value = seed;
  return pipe_mgr_hash_calc_seed_set_int(sess_hdl, dev_id, dhash);
}

pipe_status_t pipe_mgr_hash_calc_seed_get_ext(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_hash_calc_hdl_t hdl,
                                              pipe_hash_seed_t *seed) {
  pipe_dhash_info_t *dhash = NULL;

  (void)sess_hdl;

  LOG_TRACE("%s: dev %d, Dynamic Hash Calculation hdl 0x%x Get seed value  \n",
            __func__,
            dev_id,
            hdl);

  dhash = pipe_mgr_get_hash_calc_info(dev_id, hdl);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_hash_calc_seed(dhash);

  *seed = dhash->curr_seed_value;

  return PIPE_SUCCESS;
}

static pipe_status_t populate_crossbar_data(
    pipe_dhash_field_list_t *field_list,
    uint32_t stage_id,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    bool is_mod,
    pipe_dhash_crossbar_data_t *input_xbar_bytes) {
  pipe_dhash_field_t *field = NULL;
  pipe_dhash_crossbar_config_t *crossbar_config = NULL;
  pipe_dhash_crossbar_t *crossbar = NULL;
  uint32_t k = 0, j = 0, field_bit = 0;
  uint32_t num_crossbars = 0, byte_pair_index, index_in_pair;
  uint8_t attr_bit = 0;

  crossbar_config = &(field_list->crossbar_configs[stage_id]);
  num_crossbars = is_mod ? crossbar_config->num_crossbar_mods
                         : crossbar_config->num_crossbars;
  /* Loop for each attr field */
  for (k = 0; k < attr_count; k++) {
    field = &(field_list->fields[attrs[k].input_field - 1]);
    for (field_bit = field->start_bit;
         field_bit < field->start_bit + field->bit_width;
         ++field_bit) {
      for (j = 0; j < num_crossbars; j++) {
        crossbar = is_mod ? &(crossbar_config->crossbar_mods[j])
                          : &(crossbar_config->crossbars[j]);
        if (crossbar->field_bit == field_bit &&
            !strcmp(field->name, crossbar->name)) {
          byte_pair_index =
              (crossbar->byte_number * 8 + crossbar->bit_in_byte) / 16;
          index_in_pair =
              (crossbar->byte_number * 8 + crossbar->bit_in_byte) % 16;
          if (attrs[k].type == INPUT_FIELD_ATTR_TYPE_STREAM) {
            attr_bit =
                (attrs[k].value.stream[(field_bit - field->start_bit) / 8] >>
                 ((field_bit - field->start_bit) % 8)) &
                1ULL;
          } else if (attrs[k].type == INPUT_FIELD_ATTR_TYPE_VALUE) {
            if (field_bit >= 32) {
              LOG_ERROR(
                  "%s:%d Field bit index %s:%d cannot be > 31"
                  " if uint32 value",
                  __func__,
                  __LINE__,
                  field->name,
                  field_bit);
              PIPE_MGR_DBGCHK(0);
              return PIPE_INVALID_ARG;
            }
            attr_bit = (attrs[k].value.val >> (field_bit)) & 1UL;
          } else {
            PIPE_MGR_DBGCHK(0);
            LOG_ERROR("%s:%d Only stream or value types expected",
                      __func__,
                      __LINE__);
            return PIPE_UNEXPECTED;
          }
          if (index_in_pair < 8) {
            input_xbar_bytes[byte_pair_index].byte0 |=
                ((attr_bit & 1) << index_in_pair);
            input_xbar_bytes[byte_pair_index].valid0 = 1;
          } else {
            input_xbar_bytes[byte_pair_index].byte1 |=
                ((attr_bit & 1) << (index_in_pair - 8));
            input_xbar_bytes[byte_pair_index].valid1 = 1;
          }
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_int(
    bf_dev_id_t dev_id,
    pipe_dhash_info_t *dhash,
    pipe_dhash_field_list_t *field_list,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    uint32_t hash_len,
    bool is_mod,
    uint8_t *hash) {
  uint32_t i = 0, byte_pair_index = 0, parity = 0;
  uint32_t gfm_val = 0, hg = 0;
  uint8_t vector0 = 0, vector1 = 0;
  bf_dev_pipe_t pipe = PIPE_BITMAP_GET_FIRST_SET(&dhash->pipe_bmp);
  pipe_dhash_hash_config_t *hash_config = NULL;
  pipe_dhash_crossbar_data_t *input_xbar_bytes = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  hash_matrix_output_t *hash_matrix_output = NULL;
  uint32_t num_hash_bits = 0, hash_id = 0;

  uint8_t num_input_pgs = PIPE_MGR_TOF_HASH_INPUT_PARITY_GROUPS,
          bytepairs_per_pg = PIPE_MGR_TOF_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      num_input_pgs = PIPE_MGR_TOF_HASH_INPUT_PARITY_GROUPS;
      bytepairs_per_pg = PIPE_MGR_TOF_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      num_input_pgs = PIPE_MGR_TOF2_HASH_INPUT_PARITY_GROUPS;
      bytepairs_per_pg = PIPE_MGR_TOF2_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      num_input_pgs = PIPE_MGR_TOF2_HASH_INPUT_PARITY_GROUPS;
      bytepairs_per_pg = PIPE_MGR_TOF2_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  input_xbar_bytes = PIPE_MGR_CALLOC(num_input_pgs * bytepairs_per_pg,
                                     sizeof(pipe_dhash_crossbar_data_t));
  if (!input_xbar_bytes) {
    LOG_ERROR("%s:%d Calloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (hg = 0; hg < dhash->num_hash_configs; hg++) {
    hash_config = &(dhash->hash_configs[hg]);
    num_hash_bits = is_mod ? hash_config->hash_mod.num_hash_bits
                           : hash_config->hash.num_hash_bits;
    if (num_hash_bits == 0) continue;

    /* construct input_xbar_bytes for this stage in the
     * the hash config
     */
    PIPE_MGR_MEMSET(
        input_xbar_bytes,
        0,
        num_input_pgs * bytepairs_per_pg * sizeof(pipe_dhash_crossbar_data_t));

    status = populate_crossbar_data(
        field_list, hg, attr_count, attrs, is_mod, input_xbar_bytes);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d failed to populate crossbar for dev:%d pipe%d hash_config %d",
          __func__,
          __LINE__,
          dev_id,
          pipe,
          hg);
      PIPE_MGR_FREE(input_xbar_bytes);
      return status;
    }
    /* Calculate every hash bit*/
    for (i = 0; i < num_hash_bits; i++) {
      vector0 = 0;
      vector1 = 0;
      hash_matrix_output = is_mod ? &hash_config->hash_mod.hash_bits[i]
                                  : &hash_config->hash.hash_bits[i];

      for (uint32_t pg = 0; pg < num_input_pgs; pg++) {
        parity = 0;
        /* For each of 16 groups - 72b of crossbar_data and 72b GFM
         * are ANDed and then parity is calculated for these 72 bits to
         * create a 16 bit vector, 1 bit for each group.
         */
        for (uint32_t byte_pair = 0; byte_pair < bytepairs_per_pg;
             byte_pair++) {
          byte_pair_index = pg * bytepairs_per_pg + byte_pair;
          gfm_val = 0;
          status =
              pipe_mgr_gfm_shadow_entry_get(dev_info,
                                            pipe,
                                            hash_config->stage_id,
                                            byte_pair_index,
                                            hash_matrix_output->gfm_start_bit,
                                            &gfm_val);
          if (status) {
            LOG_ERROR(
                "%s:%d failed to get GFM value for GFM out bit:%d "
                "dev:%d pipe%d hash_config %d",
                __func__,
                __LINE__,
                hash_matrix_output->gfm_start_bit,
                dev_id,
                pipe,
                hg);
            PIPE_MGR_FREE(input_xbar_bytes);
            return status;
          }
          switch (dev_info->dev_family) {
            case BF_DEV_FAMILY_TOFINO:
              gfm_val &=
                  ((input_xbar_bytes[byte_pair_index].byte0 & 0xff) |
                   ((input_xbar_bytes[byte_pair_index].valid0 & 0x1u) << 8) |
                   ((input_xbar_bytes[byte_pair_index].byte1 & 0xff) << 9) |
                   ((input_xbar_bytes[byte_pair_index].valid1 & 0x1u) << 17));
              break;
            case BF_DEV_FAMILY_TOFINO2:
              gfm_val &=
                  ((input_xbar_bytes[byte_pair_index].byte0 & 0xff) |
                   ((input_xbar_bytes[byte_pair_index].byte1 & 0xff) << 8));
              break;
            case BF_DEV_FAMILY_TOFINO3:
              gfm_val &=
                  ((input_xbar_bytes[byte_pair_index].byte0 & 0xff) |
                   ((input_xbar_bytes[byte_pair_index].byte1 & 0xff) << 8));
              break;

            case BF_DEV_FAMILY_UNKNOWN:
              PIPE_MGR_DBGCHK(0);
              PIPE_MGR_FREE(input_xbar_bytes);
              return PIPE_UNEXPECTED;
          }
          parity ^= __builtin_popcount(gfm_val) % 2 == 0;
        }
        if (pg < 8)
          vector0 |= (parity << pg);
        else
          vector1 |= (parity << (pg - 8));
      }
      /* The 16b parity vector has been calculated.
       * AND it with the parity_group_mask and then calculate parity.
       * After seed inclusion, this parity is one of the 52 output bits.
       * Populate it in the correct p4_hash_bit in hash
       */
      parity = 0;

      hash_id =
          is_mod ? hash_config->hash_mod.hash_id : hash_config->hash.hash_id;
      vector0 &= (PIPE_SEL_SHADOW_DB(dev_id)
                      ->pgm_db[pipe][hash_config->stage_id]
                      .pgm[hash_id][0] &
                  0xff);
      vector1 &= (PIPE_SEL_SHADOW_DB(dev_id)
                      ->pgm_db[pipe][hash_config->stage_id]
                      .pgm[hash_id][1] &
                  0xff);

      parity ^= (__builtin_popcount(vector0) % 2 == 0) ^
                (__builtin_popcount(vector1) % 2 == 0);
      /* Need to xor correct bit of seed register with this parity val */
      parity ^= ((PIPE_SEL_SHADOW_DB(dev_id)
                      ->seed_db[pipe][hash_config->stage_id]
                      .hash_seed[hash_matrix_output->gfm_start_bit] >>
                  hash_id) &
                 1UL);
      uint32_t out_bytes = hash_matrix_output->p4_hash_output_bit / 8;
      uint32_t out_bit_in_byte = hash_matrix_output->p4_hash_output_bit % 8;
      if (hash_matrix_output->p4_hash_output_bit > hash_len * 8) {
        LOG_ERROR(
            "%s:%d p4_hash_out_bit %d incurred which is greater "
            "than hash_len %d",
            __func__,
            __LINE__,
            hash_matrix_output->p4_hash_output_bit,
            hash_len);
        PIPE_MGR_FREE(input_xbar_bytes);
        return PIPE_INVALID_ARG;
      }
      hash[hash_len - out_bytes - 1] |= (parity << out_bit_in_byte);
    }
  }
  PIPE_MGR_FREE(input_xbar_bytes);
  return status;
}

pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_with_cfg_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    uint32_t hash_len,
    uint8_t *hash) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_field_list_t *field_list = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t i = 0;

  dhash = pipe_mgr_get_hash_calc_info(dev_id, handle);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (hash_len < (dhash->hash_bit_width + 7) / 8) {
    LOG_ERROR(
        "%s:%d %d bits required for hash calculation of dyn hash calc 0x%x"
        " which is > requested:%d bits len",
        __func__,
        __LINE__,
        (dhash->hash_bit_width + 7) / 8,
        handle,
        hash_len);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < dhash->num_field_lists; i++) {
    if (dhash->field_lists[i].handle == dhash->curr_field_list_hdl) {
      field_list = &(dhash->field_lists[i]);
      break;
    }
  }
  if (!field_list) {
    PIPE_MGR_DBGCHK(field_list);
    return PIPE_UNEXPECTED;
  }
  status = pipe_mgr_hash_calc_calculate_hash_value_int(
      dev_id, dhash, field_list, attr_count, attrs, hash_len, false, hash);
  if (status) {
    LOG_ERROR("%s:%d Failed to calculate hash value for handle %x",
              __func__,
              __LINE__,
              handle);
    return status;
  }

  status = pipe_mgr_hash_calc_calculate_hash_value_int(
      dev_id, dhash, field_list, attr_count, attrs, hash_len, true, hash);
  if (status) {
    LOG_ERROR("%s:%d Failed to calculate hash_mod value for handle %x",
              __func__,
              __LINE__,
              handle);
    return status;
  }
  return status;
}

pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_ext(
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint8_t *stream,
    uint32_t stream_len,
    uint8_t *hash,
    uint32_t hash_len) {
  pipe_dhash_info_t *dhash = NULL;
  pipe_dhash_alg_t *curr_alg = NULL;
  uint32_t i = 0;

  dhash = pipe_mgr_get_hash_calc_info(dev_id, handle);
  if (!dhash) {
    LOG_ERROR("%s:%d Dynamic hash calculation info not found for hdl 0x%x",
              __func__,
              __LINE__,
              handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < dhash->num_algorithms; i++) {
    if (dhash->algorithms[i].handle == dhash->curr_algo_hdl) {
      curr_alg = &dhash->algorithms[i];
      break;
    }
  }
  if (!curr_alg) {
    LOG_ERROR(
        "%s:%d Error finding current algorithm 0x%x for dyn hash calc 0x%x",
        __func__,
        __LINE__,
        dhash->curr_algo_hdl,
        handle);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (hash_len < (dhash->hash_bit_width + 7) / 8) {
    LOG_ERROR(
        "%s:%d %d bits required for hash calculation of dyn hash calc 0x%x",
        __func__,
        __LINE__,
        (dhash->hash_bit_width + 7) / 8,
        handle);
    return PIPE_INVALID_ARG;
  }

  calculate_crc(
      &curr_alg->hash_alg, dhash->hash_bit_width, stream, stream_len, hash);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_log_state(bf_dev_id_t dev_id,
                                     pipe_sel_tbl_hdl_t tbl_hdl,
                                     cJSON *sel_tbls) {
  sel_tbl_info_t *tbl_info = NULL;
  sel_tbl_t *pipe_tbl = NULL;
  sel_grp_info_t *grp_info = NULL;
  sel_grp_mbr_t *grp_mbr = NULL;
  PWord_t Pvalue = NULL;
  Pvoid_t stage_lookup = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_hlp_word_data_t *word_data = NULL;
  Word_t grp_hdl = 0;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  uint32_t pipe_idx = 0, stage_idx = 0;
  uint32_t word_idx = 0, mbr_idx = 0;
  cJSON *sel_tbl, *sel_grps, *sel_grp, *mat_tbls, *mat_entries;
  cJSON *pipes, *pipe, *stages, *stage, *mat_tbl, *mat_entry;
  cJSON *words, *word, *mbrs, *mbr;

  tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (tbl_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  cJSON_AddItemToArray(sel_tbls, sel_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(sel_tbl, "name", tbl_info->name);
  cJSON_AddNumberToObject(sel_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(sel_tbl, "symmetric", tbl_info->is_symmetric);
  cJSON_AddBoolToObject(sel_tbl, "cache_id", tbl_info->cache_id);
  cJSON_AddBoolToObject(sel_tbl, "sequence_order", tbl_info->sequence_order);

  cJSON_AddItemToObject(sel_tbl, "fallback", pipes = cJSON_CreateArray());
  for (pipe_idx = 0; pipe_idx < tbl_info->no_sel_tbls; pipe_idx++) {
    pipe_tbl = &tbl_info->sel_tbl[pipe_idx];
    cJSON_AddItemToArray(pipes, pipe = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe, "pipe_id", pipe_tbl->pipe_id);
    cJSON_AddNumberToObject(
        pipe, "hlp_hdl", pipe_tbl->hlp.fallback_adt_ent_hdl);
    cJSON_AddNumberToObject(
        pipe, "llp_hdl", pipe_tbl->llp.fallback_adt_ent_hdl);
    if (pipe_tbl->llp.fallback_adt_ent_hdl) {
      cJSON_AddItemToObject(pipe, "stages", stages = cJSON_CreateArray());
      for (stage_idx = 0; stage_idx < pipe_tbl->num_stages; stage_idx++) {
        cJSON_AddItemToArray(stages, stage = cJSON_CreateObject());
        cJSON_AddNumberToObject(
            stage,
            "adt_idx",
            pipe_tbl->sel_tbl_stage_info[stage_idx].fallback_adt_index);
      }
    }
  }

  cJSON_AddItemToObject(sel_tbl, "sel_grps", sel_grps = cJSON_CreateArray());
  for (pipe_idx = 0; pipe_idx < tbl_info->no_sel_tbls; pipe_idx++) {
    pipe_tbl = &tbl_info->sel_tbl[pipe_idx];
    JLF(Pvalue, pipe_tbl->sel_grp_array, grp_hdl);
    while (Pvalue) {
      grp_info = (sel_grp_info_t *)*Pvalue;
      cJSON_AddItemToArray(sel_grps, sel_grp = cJSON_CreateObject());
      cJSON_AddNumberToObject(sel_grp, "grp_hdl", grp_hdl);
      cJSON_AddNumberToObject(sel_grp, "grp_id", grp_info->grp_id);
      cJSON_AddNumberToObject(sel_grp, "pipe_id", grp_info->pipe_id);
      cJSON_AddNumberToObject(sel_grp, "act_fn_hdl", grp_info->act_fn_hdl);
      cJSON_AddNumberToObject(sel_grp, "max_grp_size", grp_info->max_grp_size);
      cJSON_AddNumberToObject(sel_grp, "mbr_count", grp_info->mbr_count);
      cJSON_AddNumberToObject(
          sel_grp, "num_references", grp_info->num_references);
      cJSON_AddNumberToObject(
          sel_grp, "num_active_mbrs", grp_info->num_active_mbrs);
      cJSON_AddNumberToObject(
          sel_grp, "entries_per_word", grp_info->entries_per_word);

      cJSON_AddItemToObject(
          sel_grp, "mat_tbl_list", mat_tbls = cJSON_CreateArray());
      unsigned long key;
      void *data;
      sel_grp_mat_backptr_t *el = grp_info->mat_tbl_list;
      while (el) {
        cJSON_AddItemToArray(mat_tbls, mat_tbl = cJSON_CreateObject());
        cJSON_AddNumberToObject(mat_tbl, "mat_tbl_hdl", el->mat_tbl);
        cJSON_AddItemToObject(
            mat_tbl, "mat_entries", mat_entries = cJSON_CreateArray());
        bf_map_sts_t sts =
            bf_map_get_first(&el->ent_hdls, &key, (void **)&data);
        while (sts == BF_MAP_OK) {
          cJSON_AddItemToArray(mat_entries, mat_entry = cJSON_CreateObject());
          cJSON_AddNumberToObject(mat_entry, "entry_hdl", key);
          sts = bf_map_get_next(&el->ent_hdls, &key, (void **)&data);
        }
        el = el->next;
      }

      cJSON_AddItemToObject(sel_grp, "pipes", pipes = cJSON_CreateArray());
      JUDYL_FOREACH(
          grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
        cJSON_AddItemToArray(pipes, pipe = cJSON_CreateObject());
        cJSON_AddNumberToObject(
            pipe, "pipe_id", tbl_info->sel_tbl[pipe_idx].pipe_id);
        cJSON_AddItemToObject(pipe, "stages", stages = cJSON_CreateArray());
        JUDYL_FOREACH2(stage_lookup,
                       stage_idx,
                       sel_grp_stage_info_t *,
                       sel_grp_stage_info,
                       2) {
          (void)stage_idx;
          cJSON_AddItemToArray(stages, stage = cJSON_CreateObject());
          cJSON_AddNumberToObject(
              stage, "stage_id", sel_grp_stage_info->stage_p->stage_id);
          cJSON_AddNumberToObject(
              stage, "sel_base_idx", sel_grp_stage_info->sel_base_idx);
          cJSON_AddBoolToObject(
              stage, "duplicated", sel_grp_stage_info->mbrs_duplicated);
          cJSON_AddItemToObject(stage, "words", words = cJSON_CreateArray());
          for (word_idx = 0; word_idx < sel_grp_stage_info->no_words;
               word_idx++) {
            word_data = &(sel_grp_stage_info->sel_grp_word_data[word_idx]);
            cJSON_AddItemToArray(words, word = cJSON_CreateObject());
            cJSON_AddNumberToObject(
                word, "adt_base_idx", word_data->adt_base_idx);
            cJSON_AddItemToObject(word, "mbrs", mbrs = cJSON_CreateArray());
            for (mbr_idx = 0; mbr_idx < word_data->word_width; mbr_idx++) {
              mbr_hdl = word_data->mbrs[mbr_idx];
              if (mbr_hdl) {
                Pvalue = NULL;
                JLG(Pvalue, grp_info->sel_grp_mbrs, (Word_t)mbr_hdl);
                grp_mbr = (sel_grp_mbr_t *)*Pvalue;
                cJSON_AddItemToArray(mbrs, mbr = cJSON_CreateObject());
                cJSON_AddNumberToObject(mbr, "mbr_hdl", mbr_hdl);
                cJSON_AddNumberToObject(mbr, "mbr_idx", mbr_idx);
                cJSON_AddBoolToObject(
                    mbr,
                    "active",
                    (grp_mbr->state == PIPE_MGR_GRP_MBR_STATE_ACTIVE));
              }
            }
            if (sel_grp_stage_info->mbrs_duplicated) {
              break;
            }
          }
        }
        if (grp_info->pipe_id == BF_DEV_PIPE_ALL) {
          break;
        }
      }
      JLN(Pvalue, pipe_tbl->sel_grp_array, grp_hdl);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_restore_state(bf_dev_id_t dev_id, cJSON *sel_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  sel_tbl_info_t *tbl_info = NULL;
  sel_tbl_t *pipe_tbl = NULL;
  sel_grp_info_t *grp_info = NULL;
  sel_grp_mbr_t *grp_mbr = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  pipe_sel_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t ent_hdl;
  bool symmetric;
  uint32_t pipe_idx, stage_idx;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  struct pipe_mgr_sel_move_list_t *move_head, *move_tail, *move_node;
  uint32_t sel_base_idx, adt_base_idx;
  uint32_t word_idx = 0, mbr_idx = 0;
  rmt_virt_addr_t fallback_adt_ent_addr = 0;
  cJSON *sel_grps, *sel_grp;
  cJSON *pipes, *pipe, *stages, *stage, *mat_tbls, *mat_entries;
  cJSON *words, *word, *mbrs, *mbr, *mat_tbl, *mat_entry;
  scope_pipes_t scopes = 0xf;
  uint32_t cur_pos = 0;
  sel_mbr_pos_t *mbr_pos = NULL;

  tbl_hdl = cJSON_GetObjectItem(sel_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d Selector table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  tbl_info->in_restore = true;
  tbl_info->cache_id =
      (cJSON_GetObjectItem(sel_tbl, "cache_id")->type == cJSON_True);

  symmetric = (cJSON_GetObjectItem(sel_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != tbl_info->is_symmetric) {
    sts = pipe_mgr_sel_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, 1, &scopes, false);
    sts |= pipe_mgr_sel_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, 1, &scopes, true);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, selector tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      goto done;
    }
  }

  pipes = cJSON_GetObjectItem(sel_tbl, "fallback");
  for (pipe = pipes->child, pipe_idx = 0; pipe; pipe = pipe->next, pipe_idx++) {
    pipe_tbl = &tbl_info->sel_tbl[pipe_idx];
    PIPE_MGR_DBGCHK(pipe_tbl->pipe_id ==
                    (uint32_t)cJSON_GetObjectItem(pipe, "pipe_id")->valueint);
    pipe_tbl->cur_sess_hdl = pipe_mgr_get_int_sess_hdl();
    pipe_tbl->hlp.fallback_adt_ent_hdl =
        cJSON_GetObjectItem(pipe, "hlp_hdl")->valuedouble;
    pipe_tbl->llp.fallback_adt_ent_hdl =
        cJSON_GetObjectItem(pipe, "llp_hdl")->valuedouble;
    if (pipe_tbl->llp.fallback_adt_ent_hdl) {
      stages = cJSON_GetObjectItem(pipe, "stages");
      for (stage = stages->child, stage_idx = 0; stage;
           stage = stage->next, stage_idx++) {
        adt_base_idx = cJSON_GetObjectItem(stage, "adt_idx")->valuedouble;
        pipe_tbl->sel_tbl_stage_info[stage_idx].fallback_adt_index =
            adt_base_idx;

        sts = rmt_adt_ent_get_addr(
            dev_id,
            pipe_tbl->pipe_id,
            tbl_info->adt_tbl_hdl,
            pipe_tbl->sel_tbl_stage_info[stage_idx].stage_id,
            adt_base_idx,
            &fallback_adt_ent_addr);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error getting virtual address for action logical index %d "
              "for selector table 0x%x device %d",
              __func__,
              __LINE__,
              adt_base_idx,
              tbl_info->tbl_hdl,
              tbl_info->dev_id);
          goto done;
        }
        sts = pipe_mgr_sel_write_fallback_reg(
            pipe_tbl,
            &pipe_tbl->sel_tbl_stage_info[stage_idx],
            fallback_adt_ent_addr);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error updating hardware for fallback index %d "
              "in selector table 0x%x device %d",
              __func__,
              __LINE__,
              adt_base_idx,
              tbl_info->tbl_hdl,
              tbl_info->dev_id);
          goto done;
        }
      }
    }
  }

  sel_grps = cJSON_GetObjectItem(sel_tbl, "sel_grps");
  for (sel_grp = sel_grps->child; sel_grp; sel_grp = sel_grp->next) {
    move_head = NULL;
    grp_info = pipe_mgr_sel_grp_allocate();
    if (grp_info == NULL) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      sts = PIPE_NO_SYS_RESOURCES;
      goto done;
    }

    grp_info->grp_hdl = cJSON_GetObjectItem(sel_grp, "grp_hdl")->valuedouble;
    grp_info->grp_id = cJSON_GetObjectItem(sel_grp, "grp_id")->valuedouble;
    grp_info->pipe_id = cJSON_GetObjectItem(sel_grp, "pipe_id")->valueint;
    grp_info->act_fn_hdl =
        cJSON_GetObjectItem(sel_grp, "act_fn_hdl")->valuedouble;
    grp_info->max_grp_size =
        cJSON_GetObjectItem(sel_grp, "max_grp_size")->valuedouble;
    grp_info->num_references =
        cJSON_GetObjectItem(sel_grp, "num_references")->valuedouble;
    grp_info->num_active_mbrs =
        cJSON_GetObjectItem(sel_grp, "num_active_mbrs")->valuedouble;
    grp_info->entries_per_word =
        cJSON_GetObjectItem(sel_grp, "entries_per_word")->valuedouble;
    mat_tbls = cJSON_GetObjectItem(sel_grp, "mat_tbl_list");
    for (mat_tbl = mat_tbls->child; mat_tbl; mat_tbl = mat_tbl->next) {
      sel_grp_mat_backptr_t *el = (sel_grp_mat_backptr_t *)PIPE_MGR_CALLOC(
          1, sizeof(sel_grp_mat_backptr_t));
      if (el == NULL) {
        LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
        sts = PIPE_NO_SYS_RESOURCES;
        goto done;
      }
      el->mat_tbl = cJSON_GetObjectItem(mat_tbl, "mat_tbl_hdl")->valuedouble;
      BF_LIST_DLL_AP(grp_info->mat_tbl_list, el, next, prev);
      bf_map_init(&el->ent_hdls);

      mat_entries = cJSON_GetObjectItem(mat_tbl, "mat_entries");
      for (mat_entry = mat_entries->child; mat_entry;
           mat_entry = mat_entries->next) {
        ent_hdl = cJSON_GetObjectItem(mat_entry, "entry_hdl")->valuedouble;
        bf_map_sts_t msts = bf_map_add(&el->ent_hdls, ent_hdl, NULL);
        if (msts != BF_MAP_OK) {
          LOG_ERROR("%s:%d Map add operation error", __func__, __LINE__);
          sts = PIPE_UNEXPECTED;
          PIPE_MGR_DBGCHK(0);
          goto done;
        }
      }
    }

    pipes = cJSON_GetObjectItem(sel_grp, "pipes");
    for (pipe = pipes->child; pipe; pipe = pipe->next) {
      pipe_tbl = get_sel_tbl_by_pipe_id(
          tbl_info, cJSON_GetObjectItem(pipe, "pipe_id")->valueint);
      if (!pipe_tbl) {
        LOG_ERROR("%s:%d get sel table failed", __func__, __LINE__);
        return PIPE_UNEXPECTED;
      }
      stages = cJSON_GetObjectItem(pipe, "stages");
      for (stage = stages->child, stage_idx = 0; stage;
           stage = stage->next, stage_idx++) {
        words = cJSON_GetObjectItem(stage, "words");
        sel_base_idx = cJSON_GetObjectItem(stage, "sel_base_idx")->valuedouble;
        adt_base_idx =
            cJSON_GetObjectItem(words->child, "adt_base_idx")->valuedouble;
        sts = pipe_mgr_sel_grp_activate_stage_internal(
            pipe_tbl,
            grp_info,
            &pipe_tbl->sel_tbl_stage_info[stage_idx],
            &sel_grp_stage_info,
            sel_base_idx,
            adt_base_idx);
        if (!move_head) {
          sts = pipe_mgr_sel_grp_add_to_htbl(
              pipe_tbl, grp_info->grp_hdl, grp_info);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error adding the grp %d to grp_hdl_htbl for pipe %x",
                __func__,
                __LINE__,
                grp_info->grp_hdl,
                grp_info->pipe_id);
            goto done;
          }
          sts =
              pipe_mgr_sel_grp_build_move_list(pipe_tbl, grp_info, &move_head);
          if (sts != PIPE_SUCCESS) {
            goto done;
          }
          move_tail = move_head;
        }
        sel_grp_stage_info->act_fn_hdl = grp_info->act_fn_hdl;
        sel_grp_stage_info->mbrs_duplicated =
            (cJSON_GetObjectItem(stage, "duplicated")->type == cJSON_True);
        sel_grp_stage_info->cur_usage = 0;
        word = words->child;
        for (word_idx = 0; word_idx < sel_grp_stage_info->no_words;
             word_idx++) {
          if (!sel_grp_stage_info->mbrs_duplicated || word_idx == 0) {
            PIPE_MGR_DBGCHK(
                cJSON_GetObjectItem(word, "adt_base_idx")->valuedouble ==
                (double)(adt_base_idx + (SEL_GRP_WORD_WIDTH * word_idx)));
          }
          mbrs = cJSON_GetObjectItem(word, "mbrs");
          for (mbr = mbrs->child; mbr; mbr = mbr->next) {
            mbr_hdl = cJSON_GetObjectItem(mbr, "mbr_hdl")->valuedouble;
            mbr_idx = cJSON_GetObjectItem(mbr, "mbr_idx")->valuedouble;
            sts = add_one_mbr(pipe_tbl,
                              grp_info,
                              sel_grp_stage_info,
                              mbr_hdl,
                              word_idx,
                              mbr_idx,
                              &move_tail);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR("%s:%d Error adding mbr %d to grp %d tbl 0x%x dev %d",
                        __func__,
                        __LINE__,
                        mbr_hdl,
                        grp_info->grp_hdl,
                        tbl_hdl,
                        dev_id);
              goto done;
            }
            if (!sel_grp_stage_info->mbrs_duplicated || word_idx == 0) {
              sts = pipe_mgr_sel_grp_mbr_hw_locator_update(
                  sel_grp_stage_info, mbr_hdl, word_idx, mbr_idx);
              if (sts != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d %s(%#x %d.%x) Error updating mbr hw locator for "
                    "mbr %#x grp %#x",
                    __func__,
                    __LINE__,
                    tbl_info->name,
                    tbl_hdl,
                    dev_id,
                    pipe_tbl->pipe_id,
                    mbr_hdl,
                    grp_info->grp_hdl);
                goto done;
              }

              cur_pos = sel_grp_stage_info->cur_usage;
              sts = sel_grp_mbr_add_del_pos(grp_info,
                                            tbl_hdl,
                                            sel_grp_stage_info->cur_usage,
                                            tbl_info->sequence_order,
                                            PIPE_SEL_UPDATE_ADD);
              if (sts != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d %s(%#x %d.%x) Error updating mbr hw locator for "
                    "mbr %#x grp %#x",
                    __func__,
                    __LINE__,
                    tbl_info->name,
                    tbl_hdl,
                    dev_id,
                    pipe_tbl->pipe_id,
                    mbr_hdl,
                    grp_info->grp_hdl);
                goto done;
              }
              sel_grp_stage_info->cur_usage++;
            }

            if (!pipe_mgr_sel_grp_mbr_get(grp_info, mbr_hdl, false)) {
              /* Generate using, word_idx, mbr_idx */
              grp_mbr = pipe_mgr_sel_grp_mbr_alloc(mbr_hdl);
              if (!grp_mbr) {
                sts = PIPE_NO_SYS_RESOURCES;
                goto done;
              }
              mbr_pos = sel_grp_pos_alloc(cur_pos);
              if (!mbr_pos) {
                return PIPE_NO_SYS_RESOURCES;
              }
              grp_mbr->mbrs_pos = mbr_pos;
              sts = pipe_mgr_sel_mbr_add_to_htbl(
                  grp_info, mbr_hdl, grp_mbr, false);
              if (sts != PIPE_SUCCESS) {
                pipe_mgr_sel_grp_mbr_destroy(grp_mbr);
                goto done;
              }
              grp_info->mbr_count++;
            }
          }
          if (!sel_grp_stage_info->mbrs_duplicated) {
            word = word->next;
          }
        }
      }
    }

    for (move_node = move_head; move_node; move_node = move_node->next) {
      sts = pipe_mgr_sel_process_placement_op(sess_hdl, tbl_info, move_node);
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
    }
    free_sel_move_list(&move_head);
  }

done:
  tbl_info->in_restore = false;
  return sts;
}

pipe_status_t pipe_mgr_sel_sbe_correct(bf_dev_id_t dev_id,
                                       bf_dev_pipe_t log_pipe_id,
                                       dev_stage_t stage_id,
                                       pipe_sel_tbl_hdl_t tbl_hdl,
                                       int line) {
  /* If the device is locked we cannot correct anything since the software state
   * may not agree with the hardware state. */
  if (pipe_mgr_is_device_locked(dev_id)) return PIPE_SUCCESS;

  sel_tbl_info_t *tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, tbl_hdl, false);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d Dev %d sel table with handle 0x%x not found",
              __func__,
              __LINE__,
              dev_id,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Take the first pipe table, RAM allocation doesn't change across pipes. */
  sel_tbl_t *pipe_tbl = tbl_info->sel_tbl;

  sel_tbl_stage_info_t *stage_info = NULL;
  for (unsigned int i = 0; i < pipe_tbl->num_stages; ++i) {
    if (pipe_tbl->sel_tbl_stage_info[i].stage_id == stage_id) {
      stage_info = &pipe_tbl->sel_tbl_stage_info[i];
      break;
    }
  }
  if (!stage_info) return PIPE_INVALID_ARG;

  bf_dev_pipe_t phy_pipe_id;
  pipe_status_t sts = pipe_mgr_map_pipe_id_log_to_phy(
      tbl_info->dev_info, log_pipe_id, &phy_pipe_id);
  if (sts) return sts;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);

  for (unsigned int i = 0; i < stage_info->pv_hw.num_blks; ++i) {
    vpn_id_t vpn = stage_info->pv_hw.tbl_blk[i].vpn_id[0];
    uint32_t low_vir_addr =
        (((vpn << 10) | line) << TOF_SEL_SUBWORD_VPN_BITS) | 0xF;

    pipe_full_virt_addr_t vaddr;
    vaddr.addr = 0;
    construct_full_virt_addr(tbl_info->dev_info,
                             &vaddr,
                             stage_info->pv_hw.tbl_id,
                             pipe_virt_mem_type_sel_stful,
                             low_vir_addr,
                             phy_pipe_id,
                             stage_id);
    LOG_DBG(
        "Dev %d pipe %x stage %d lt %d sel tbl 0x%x SBE correct vpn %d line "
        "%d virt 0x%" PRIx64,
        dev_id,
        log_pipe_id,
        stage_id,
        stage_info->pv_hw.tbl_id,
        tbl_hdl,
        vpn,
        line,
        vaddr.addr);
    uint64_t dont, care;
    lld_subdev_ind_read(dev_id, subdev, vaddr.addr, &dont, &care);
  }
  return PIPE_SUCCESS;
}

bool pipe_mgr_sel_is_mode_fair(bf_dev_id_t dev_id, pipe_sel_tbl_hdl_t hdl) {
  pipe_select_tbl_info_t *sel_tbl_info = NULL;

  sel_tbl_info = pipe_mgr_get_select_tbl_info(dev_id, hdl);

  if (sel_tbl_info && SEL_TBL_IS_FAIR(sel_tbl_info)) {
    return true;
  }
  return false;
}
