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
 * Implementation of TCAM management
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <target-utils/fbitset/fbitset.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_transaction.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tcam_ha.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_hw_dump.h"

bool global_tcam_debug;

uint32_t global_entry_hdl = 0;
uint64_t total_move_cnt = 0;
uint32_t largest_move_cnt = 0;
uint64_t total_entry_add_cnt = 0;
uint64_t total_entry_move = 0;

static pipe_status_t pipe_mgr_tcam_commit_complete(
    tcam_pipe_tbl_t *tcam_pipe_tbl);

static pipe_status_t pipe_mgr_tcam_set_tcam_index(tcam_tbl_t *tcam_tbl,
                                                  uint32_t index,
                                                  tcam_hlp_entry_t *tcam_entry,
                                                  pipe_tcam_op_e tcam_op);

static pipe_mat_ent_hdl_t pipe_mgr_tcam_entry_hdl_allocate(
    tcam_pipe_tbl_t *tcam_pipe_tbl) {
  pipe_mat_ent_hdl_t ent_hdl =
      bf_id_allocator_allocate(tcam_pipe_tbl->hlp.ent_hdl_allocator);
  if ((ent_hdl == PIPE_TCAM_INVALID_ENT_HDL) || (ent_hdl == 0)) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error allocating entry hdl on pipe %d",
        __func__,
        __LINE__,
        tcam_pipe_tbl->tcam_tbl_info_p->name,
        tcam_pipe_tbl->tcam_tbl_info_p->tbl_hdl,
        tcam_pipe_tbl->tcam_tbl_info_p->dev_id,
        tcam_pipe_tbl->pipe_id);
    return PIPE_TCAM_INVALID_ENT_HDL;
  }
  if (tcam_pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
    ent_hdl = PIPE_SET_HDL_PIPE(ent_hdl, tcam_pipe_tbl->pipe_id);
  }
  return ent_hdl;
}

static bool pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                             pipe_mat_ent_hdl_t ent_hdl) {
  if (ent_hdl == PIPE_TCAM_INVALID_ENT_HDL) {
    return false;
  }
  if (tcam_pipe_tbl->pipe_id != BF_DEV_PIPE_ALL &&
      tcam_pipe_tbl->pipe_id != PIPE_GET_HDL_PIPE(ent_hdl)) {
    return false;
  }
  ent_hdl = PIPE_GET_HDL_VAL(ent_hdl);
  if (ent_hdl == 0 || (ent_hdl == PIPE_TCAM_DEF_ENT_HDL &&
                       !tcam_pipe_tbl->hlp.default_ent_set)) {
    return false;
  }
  return bf_id_allocator_is_set(tcam_pipe_tbl->hlp.ent_hdl_allocator, ent_hdl);
}

void pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                     pipe_mat_ent_hdl_t ent_hdl) {
  if (pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, ent_hdl)) {
    ent_hdl = PIPE_GET_HDL_VAL(ent_hdl);
    bf_id_allocator_release(tcam_pipe_tbl->hlp.ent_hdl_allocator, ent_hdl);
  }
}

void pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                 pipe_mat_ent_hdl_t ent_hdl) {
  bf_id_allocator_set(tcam_pipe_tbl->hlp.ent_hdl_allocator,
                      PIPE_GET_HDL_VAL(ent_hdl));
}

static void pipe_mgr_tcam_entry_deep_copy_v2(tcam_hlp_entry_t *dest,
                                             tcam_hlp_entry_t *src) {
  PIPE_MGR_MEMCPY(dest, src, sizeof(tcam_hlp_entry_t));

  dest->next = NULL;
  dest->prev = NULL;
  dest->next_range = NULL;
  dest->prev_range = NULL;
  dest->range_list = NULL;
  dest->range_list_p = NULL;
  dest->next_atomic = NULL;
  dest->prev_atomic = NULL;
}

void pipe_mgr_tcam_entry_deep_copy(tcam_hlp_entry_t *dest,
                                   tcam_hlp_entry_t *src) {
  pipe_mgr_tcam_entry_deep_copy_v2(dest, src);
}

void pipe_mgr_tcam_hlp_entry_destroy(tcam_hlp_entry_t *tcam_entry,
                                     bool free_data) {
  if (!tcam_entry) {
    return;
  }

  if (free_data && tcam_entry->mat_data) {
    free_mat_ent_data(tcam_entry->mat_data);
  }

  PIPE_MGR_FREE(tcam_entry);
}

static void pipe_mgr_tcam_hlp_entry_destroy_all(tcam_hlp_entry_t *head_entry) {
  tcam_hlp_entry_t *tcam_entry = NULL;
  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    pipe_mgr_tcam_hlp_entry_destroy(tcam_entry, false);
  }
  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_END()
}

tcam_hlp_entry_t *pipe_mgr_tcam_entry_alloc(void) {
  size_t tcam_entry_sz = 0;
  tcam_hlp_entry_t *tcam_entry = NULL;

  tcam_entry_sz = sizeof(tcam_hlp_entry_t);

  tcam_entry = (tcam_hlp_entry_t *)PIPE_MGR_CALLOC(1, tcam_entry_sz);
  if (tcam_entry == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }

  tcam_entry->index = PIPE_MGR_TCAM_INVALID_IDX;

  return tcam_entry;
}

static void pipe_mgr_tcam_entry_copy_resource(pipe_res_spec_t *dest,
                                              pipe_res_spec_t *src) {
  PIPE_MGR_MEMCPY(dest, src, sizeof(pipe_res_spec_t));
}

/** \brief pipe_mgr_tcam_create_group
 *        API to create and assign a group_info array element to the tcam table
 *
 * Malloc a group-info data structure, initialze all the internal variables
 * and update the tcam table pointer
 *
 * \param tcam_tbl Pointer to the tcam table
 * \param group Group number/id
 * \return tcam_group_info_t* Pointer to the newly created element. NULL
 *         in case of error
 */
static tcam_group_info_t *pipe_mgr_tcam_create_group(tcam_tbl_t *tcam_tbl,
                                                     uint16_t group) {
  tcam_group_info_t *group_info = NULL;

  if (group >= tcam_tbl->hlp.max_tcam_group) {
    tcam_group_info_t **new_tcam_group_info;
    int new_group_count = group + 1;

    new_tcam_group_info = (tcam_group_info_t **)PIPE_MGR_MALLOC(
        sizeof(tcam_group_info_t *) * new_group_count);
    if (new_tcam_group_info == NULL) {
      LOG_ERROR("%s:%d Error creating tcam group info", __func__, __LINE__);
      return NULL;
    }
    PIPE_MGR_MEMSET(
        new_tcam_group_info, 0, sizeof(tcam_group_info_t *) * new_group_count);

    if (tcam_tbl->hlp.group_info) {
      /* transfer */
      PIPE_MGR_MEMCPY(
          new_tcam_group_info,
          tcam_tbl->hlp.group_info,
          sizeof(tcam_group_info_t *) * tcam_tbl->hlp.max_tcam_group);
      /* free old */
      PIPE_MGR_FREE(tcam_tbl->hlp.group_info);
    }
    tcam_tbl->hlp.max_tcam_group = new_group_count;
    tcam_tbl->hlp.group_info = new_tcam_group_info;
  }

  group_info = (tcam_group_info_t *)PIPE_MGR_MALLOC(sizeof(tcam_group_info_t));
  if (group_info == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMSET(group_info, 0, sizeof(tcam_group_info_t));

  group_info->jtcam_prio_array = (Pvoid_t)NULL;
  group_info->jtcam_index_bmp = NULL;

  tcam_tbl->hlp.group_info[group] = group_info;

  return group_info;
}

/** \brief pipe_mgr_tcam_delete_group
 *         Delete a tcam group
 *
 * Free the group_info structure and all the internal variables
 *
 * \param tcam_tbl Pointer to the tcam table
 * \param group Group number/id
 * \return void
 */
static void pipe_mgr_tcam_delete_group(tcam_tbl_t *tcam_tbl, uint16_t group) {
  tcam_group_info_t *group_info = NULL;

  if (group >= tcam_tbl->hlp.max_tcam_group) return;

  group_info = tcam_tbl->hlp.group_info[group];
  if (group_info) {
    Word_t Rc_word = 0;
    PWord_t Pvalue;

    JLF(Pvalue, group_info->jtcam_prio_array, Rc_word);
    while (Pvalue) {
      tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
      JLN(Pvalue, group_info->jtcam_prio_array, Rc_word);
      PIPE_MGR_FREE(prio_range_node);
    }

    JLFA(Rc_word, group_info->jtcam_prio_array);
    J1FA(Rc_word, group_info->jtcam_index_bmp);

    PIPE_MGR_FREE(group_info);
  }
  tcam_tbl->hlp.group_info[group] = NULL;
}

pipe_status_t pipe_mgr_tcam_update_prio_array(tcam_tbl_t *tcam_tbl,
                                              uint16_t group,
                                              uint32_t priority,
                                              uint32_t tcam_index,
                                              bool is_add) {
  Word_t index;
  int Rc_int;
  PWord_t Pvalue;

  if (group >= TCAM_MAX_GROUPS) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error. Invalid group %d exceeds max groups "
        "supported",
        __func__,
        __LINE__,
        get_tcam_tbl_info(tcam_tbl)->name,
        get_tcam_tbl_info(tcam_tbl)->tbl_hdl,
        group);
    return PIPE_INVALID_ARG;
  }

  if (group >= tcam_tbl->hlp.max_tcam_group ||
      tcam_tbl->hlp.group_info[group] == NULL) {
    pipe_mgr_tcam_create_group(tcam_tbl, group);
  }

  index = priority;
  JLI(Pvalue, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d %s(0x%x) Malloc failed",
              __func__,
              __LINE__,
              get_tcam_tbl_info(tcam_tbl)->name,
              get_tcam_tbl_info(tcam_tbl)->tbl_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }

  tcam_prio_range_t *prio_range_node = NULL;
  if (*Pvalue == 0) {
    prio_range_node = PIPE_MGR_MALLOC(sizeof(tcam_prio_range_t));
    if (!prio_range_node) {
      LOG_ERROR("%s:%d %s(0x%x) Malloc failed",
                __func__,
                __LINE__,
                get_tcam_tbl_info(tcam_tbl)->name,
                get_tcam_tbl_info(tcam_tbl)->tbl_hdl);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMSET(prio_range_node, 0, sizeof(tcam_prio_range_t));
    PIPE_MGR_DBGCHK(is_add);
    prio_range_node->start = tcam_index;
    prio_range_node->end = tcam_index;
    *Pvalue = (Word_t)prio_range_node;
  } else {
    prio_range_node = (tcam_prio_range_t *)*Pvalue;
  }

  bool del_priority_node = false;
  if (is_add) {
    index = tcam_index;
    J1S(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
    PIPE_MGR_DBGCHK(Rc_int);
    if (prio_range_node->start > tcam_index) {
      prio_range_node->start = tcam_index;
    }
    if (prio_range_node->end < tcam_index) {
      prio_range_node->end = tcam_index;
    }
  } else {
    index = tcam_index;
    J1U(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
    PIPE_MGR_DBGCHK(Rc_int);
    index = tcam_index;
    if ((prio_range_node->start == tcam_index) &&
        (prio_range_node->end == tcam_index)) {
      del_priority_node = true;
    } else if (prio_range_node->start == tcam_index) {
      J1N(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
      /* Find the new start of the prio_range_node. During warm init, entry
       * priorities may be updated out of order, and the new start may not be
       * the index immediately after.
       */
      while (tcam_tbl->hlp.tcam_entries[index]->priority != priority) {
        PIPE_MGR_DBGCHK(Rc_int);
        J1N(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
      }
      prio_range_node->start = index;
    } else if (prio_range_node->end == tcam_index) {
      J1P(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
      /* Find the new end of the prio_range_node. During warm init, entry
       * priorities may be updated out of order, and the new end may not be
       * the index immediately before.
       */
      while (tcam_tbl->hlp.tcam_entries[index]->priority != priority) {
        PIPE_MGR_DBGCHK(Rc_int);
        J1P(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_index_bmp, index);
      }
      prio_range_node->end = index;
    }
  }
  PIPE_MGR_DBGCHK(prio_range_node->end >= prio_range_node->start);
  if (del_priority_node) {
    index = priority;
    JLD(Rc_int, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
    PIPE_MGR_FREE(prio_range_node);
    PIPE_MGR_DBGCHK(Rc_int);
    if (tcam_tbl->hlp.group_info[group]->jtcam_prio_array == NULL) {
      pipe_mgr_tcam_delete_group(tcam_tbl, group);
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_pipe_init_shadow_mem(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  tcam_stage_info_t *tcam_stage_info = NULL;
  tcam_block_data_t *tcam_block_data = NULL;
  rmt_mem_pack_format_t *pack_format = NULL;
  rmt_mem_pack_format_t *tind_pack_format = NULL;
  rmt_tbl_word_blk_t *tind_blk = NULL;

  unsigned i = 0;
  unsigned block_idx = 0;

  pipe_mem_type_t pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info)
                                      ? pipe_mem_type_unit_ram
                                      : pipe_mem_type_tcam;

  for (block_idx = 0; block_idx < tcam_pipe_tbl->num_blocks; block_idx++) {
    tcam_block_data = &tcam_pipe_tbl->block_data[block_idx];
    tcam_stage_info = &tcam_pipe_tbl->stage_data[tcam_block_data->stage_index];
    pack_format = &tcam_stage_info->pack_format;
    for (i = 0; i < pack_format->mem_units_per_tbl_word; i++) {
      status = pipe_mgr_phy_mem_map_symmetric_mode_set(
          tcam_tbl_info->dev_id,
          tcam_tbl_info->direction,
          pipe_bmp,
          tcam_stage_info->stage_id,
          pipe_mem_type,
          tcam_block_data->word_blk.mem_id[i],
          tcam_tbl_info->is_symmetric);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in setting shadow mem metadata for"
            " tbl 0x%x, stage id %d, tcam id %d, err %s",
            __func__,
            __LINE__,
            tcam_tbl_info->tbl_hdl,
            tcam_stage_info->stage_id,
            tcam_block_data->word_blk.mem_id[i],
            pipe_str_err(status));
        return status;
      }
    }
  }

  unsigned j = 0;
  unsigned k = 0;
  /* Init tind shadow mem metadata */
  for (i = 0; i < tcam_pipe_tbl->num_stages; i++) {
    tcam_stage_info = &tcam_pipe_tbl->stage_data[i];
    tind_pack_format = &tcam_stage_info->tind_pack_format;
    for (j = 0; j < tcam_stage_info->tind_num_blks; j++) {
      tind_blk = &tcam_stage_info->tind_blk[j];
      for (k = 0; k < tind_pack_format->mem_units_per_tbl_word; k++) {
        status = pipe_mgr_phy_mem_map_symmetric_mode_set(
            tcam_tbl_info->dev_id,
            tcam_tbl_info->direction,
            pipe_bmp,
            tcam_stage_info->stage_id,
            pipe_mem_type_unit_ram,
            tind_blk->mem_id[k],
            tcam_tbl_info->is_symmetric);

        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in setting shadow mem metadata for"
              " tind, for tcam tbl 0x%x, stage id %d, ram id %d"
              " err %s",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              tcam_stage_info->stage_id,
              tind_blk->mem_id[k],
              pipe_str_err(status));
          return status;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_init_shadow_mem(
    tcam_tbl_info_t *tcam_tbl_info) {
  pipe_status_t status = PIPE_SUCCESS;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  uint8_t no_tcam_pipe_tbls = 0;

  no_tcam_pipe_tbls = tcam_tbl_info->no_tcam_pipe_tbls;
  unsigned i = 0;

  for (i = 0; i < no_tcam_pipe_tbls; i++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];
    status = pipe_mgr_tcam_pipe_init_shadow_mem(
        tcam_tbl_info, tcam_pipe_tbl, &tcam_pipe_tbl->pipe_bmp);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing stage shadow mem for tcam tbl"
          " 0x%x, dev id %d, pipe id %d, err %s",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          tcam_pipe_tbl->pipe_id,
          pipe_str_err(status));
      return status;
    }
  }
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tcam_get_next_priority_of_group(tcam_tbl_t *tcam_tbl,
                                                         uint16_t group,
                                                         uint32_t tcam_index) {
  tcam_group_info_t *group_info;

  group_info = tcam_tbl->hlp.group_info[group];
  if (!group_info) {
    return ~0;
  }

  Word_t index;
  int Rc_int;

  index = tcam_index;
  J1N(Rc_int, group_info->jtcam_index_bmp, index);
  if (!Rc_int) {
    return ~0;
  }
  tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[(uint32_t)index];
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }

  return tcam_entry->priority;
}

static uint32_t pipe_mgr_tcam_get_prev_priority_of_group(tcam_tbl_t *tcam_tbl,
                                                         uint16_t group,
                                                         uint32_t tcam_index) {
  tcam_group_info_t *group_info;

  if (group >= tcam_tbl->hlp.max_tcam_group) {
    return ~0;
  }

  group_info = tcam_tbl->hlp.group_info[group];
  if (!group_info) {
    return ~0;
  }

  Word_t index;
  int Rc_int;

  index = tcam_index;
  J1P(Rc_int, group_info->jtcam_index_bmp, index);
  if (!Rc_int) {
    return ~0;
  }
  tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[(uint32_t)index];
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }

  return tcam_entry->priority;
}

static bool pipe_mgr_tcam_get_prio_range(tcam_tbl_t *tcam_tbl,
                                         uint16_t group,
                                         uint32_t priority,
                                         uint32_t *start_index,
                                         uint32_t *end_index) {
  Word_t index;
  PWord_t Pvalue;

  index = priority;
  JLG(Pvalue, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
  if (!Pvalue || (Pvalue == PJERR)) {
    return false;
  }

  tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
  if (start_index) {
    *start_index = prio_range_node->start;
  }
  if (end_index) {
    *end_index = prio_range_node->end;
  }

  return true;
}

static bool pipe_mgr_tcam_get_prev_prio_start(tcam_tbl_t *tcam_tbl,
                                              uint16_t group,
                                              uint32_t priority,
                                              uint32_t *replace_index) {
  /* Returns the next tcam index to replace for moving down */
  /* This is the start index in the previous priority */

  Word_t index;
  PWord_t Pvalue;

  index = priority;
  JLP(Pvalue, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
  if (!Pvalue) {
    return false;
  }

  tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
  *replace_index = prio_range_node->start;

  return true;
}

static bool pipe_mgr_tcam_get_prev_prio_end(tcam_tbl_t *tcam_tbl,
                                            uint16_t group,
                                            uint32_t priority,
                                            uint32_t *replace_index) {
  /* Returns the next tcam index to replace for moving down */
  /* This is the start index in the previous priority */

  Word_t index;
  PWord_t Pvalue;

  index = priority;
  JLP(Pvalue, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
  if (!Pvalue) {
    return false;
  }

  tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
  *replace_index = prio_range_node->end;

  return true;
}

static bool pipe_mgr_tcam_get_next_prio_start(tcam_tbl_t *tcam_tbl,
                                              uint16_t group,
                                              uint32_t priority,
                                              uint32_t *replace_index) {
  Word_t index;
  PWord_t Pvalue;

  if (group >= tcam_tbl->hlp.max_tcam_group) {
    return false;
  }

  if (!tcam_tbl->hlp.group_info[group]) {
    return false;
  }

  index = priority;
  JLN(Pvalue, tcam_tbl->hlp.group_info[group]->jtcam_prio_array, index);
  if (!Pvalue) {
    return false;
  }

  tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
  *replace_index = prio_range_node->start;

  return true;
}

static void pipe_mgr_tcam_tbl_destroy(tcam_tbl_t *tcam_tbls, uint32_t no_ptns) {
  uint32_t i, j;

  if (tcam_tbls == NULL) {
    return;
  }

  for (i = 0; i < no_ptns; i++) {
    tcam_tbl_t *tcam_tbl = &tcam_tbls[i];

    if (tcam_tbl->hlp.tcam_entries) {
      for (j = 0; j < tcam_tbl->total_entries; j++) {
        if (tcam_tbl->hlp.tcam_entries[j]) {
          pipe_mgr_tcam_hlp_entry_destroy(tcam_tbl->hlp.tcam_entries[j], true);
          tcam_tbl->hlp.tcam_entries[j] = NULL;
        }
      }
      PIPE_MGR_FREE(tcam_tbl->hlp.tcam_entries);
    }

    if (tcam_tbl->llp.tcam_entries) {
      for (j = 0; j < tcam_tbl->total_entries; j++) {
        if (tcam_tbl->llp.tcam_entries[j]) {
          pipe_mgr_tcam_llp_entry_destroy(tcam_tbl->llp.tcam_entries[j]);
          tcam_tbl->llp.tcam_entries[j] = NULL;
        }
      }
      PIPE_MGR_FREE(tcam_tbl->llp.tcam_entries);
    }

    bf_fbs_destroy(&tcam_tbl->hlp.tcam_used_bmp);

    if (tcam_tbl->hlp.group_info) {
      for (j = 0; j < tcam_tbl->hlp.max_tcam_group; j++) {
        pipe_mgr_tcam_delete_group(tcam_tbl, j);
      }
      PIPE_MGR_FREE(tcam_tbl->hlp.group_info);
    }
  }

  if (tcam_tbls) {
    PIPE_MGR_FREE(tcam_tbls);
  }
}

/** \brief pipe_mgr_tcam_pipe_tbl_destroy
 *        Destroy the data structures created for tcam table
 *
 * \param tcam_pipe_tbl Pointer to the tcam table to destroy
 * \return void
 */
static void pipe_mgr_tcam_pipe_tbl_destroy(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                           uint8_t no_tcam_pipe_tbls) {
  uint32_t i = 0;
  uint8_t pipe_id = 0;

  if (tcam_pipe_tbl == NULL) {
    return;
  }

  for (pipe_id = 0; pipe_id < no_tcam_pipe_tbls; pipe_id++) {
    if (tcam_pipe_tbl[pipe_id].hlp.default_ent_set) {
      pipe_mgr_tcam_entry_hdl_release(
          &tcam_pipe_tbl[pipe_id], tcam_pipe_tbl[pipe_id].hlp.default_ent_hdl);
    }

    if (tcam_pipe_tbl[pipe_id].block_data) {
      PIPE_MGR_FREE(tcam_pipe_tbl[pipe_id].block_data);
    }

    if (tcam_pipe_tbl[pipe_id].stage_data) {
      for (i = 0; i < tcam_pipe_tbl[pipe_id].num_stages; i++) {
        PIPE_MGR_FREE(tcam_pipe_tbl[pipe_id].stage_data[i].tind_blk);
      }
      PIPE_MGR_FREE(tcam_pipe_tbl[pipe_id].stage_data);
    }

    bf_map_destroy(&tcam_pipe_tbl[pipe_id].hlp.tcam_entry_db);
    bf_id_allocator_destroy(tcam_pipe_tbl[pipe_id].hlp.ent_hdl_allocator);
    bf_map_destroy(&tcam_pipe_tbl[pipe_id].llp.tcam_entry_db);
    if (tcam_pipe_tbl[pipe_id].llp.ha_ent_hdl_allocator) {
      bf_id_allocator_release(tcam_pipe_tbl[pipe_id].llp.ha_ent_hdl_allocator,
                              PIPE_TCAM_DEF_ENT_HDL);
      bf_id_allocator_destroy(tcam_pipe_tbl[pipe_id].llp.ha_ent_hdl_allocator);
    }

    if (tcam_pipe_tbl[pipe_id].default_ent_type ==
        TCAM_DEFAULT_ENT_TYPE_INDIRECT) {
      // The direct entry will be destroyed when destroying all the entries
      pipe_mgr_tcam_hlp_entry_destroy(
          tcam_pipe_tbl[pipe_id].hlp.hlp_default_tcam_entry, true);
      pipe_mgr_tcam_llp_entry_destroy(
          tcam_pipe_tbl[pipe_id].llp.llp_default_tcam_entry);
    }

    pipe_mgr_tcam_tbl_destroy(tcam_pipe_tbl[pipe_id].tcam_ptn_tbls,
                              tcam_pipe_tbl[pipe_id].no_ptns);
  }

  if (tcam_pipe_tbl) {
    PIPE_MGR_FREE(tcam_pipe_tbl);
  }
}

tcam_pipe_tbl_t *pipe_mgr_tcam_tbl_get_instance_from_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    int entry_hdl,
    const char *where,
    const int line) {
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  bf_dev_pipe_t pipe_id;

  if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info)) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR("%s:%d %s(0x%x) TCAM table for pipe %d not found",
                where,
                line,
                tcam_tbl_info->name,
                tcam_tbl_info->tbl_hdl,
                pipe_id);
    }
  }
  return tcam_pipe_tbl;
}

/** \brief pipe_mgr_tcam_tbl_info_get
 *        Get the tcam tbl pointer for a given table handle
 *
 * This function returns the tcam tbl pointer stored in the tcam_tbl_htbl
 * or it's backup table
 *
 * \param dev_tgt Device target
 * \param tbl_hdl TCAM table handle
 * \param is_backup Flag to indicate if a backup table/main table is needed
 * \return tcam_tbl_t* Pointer to the tcam_tbl structure
 */
tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            bool is_backup) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_tcam_tbl_map_get(
      dev_id, tbl_hdl, (void **)&tcam_tbl_info, is_backup);
  if (msts != BF_MAP_OK) {
    return NULL;
  }
  return tcam_tbl_info;
}

tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get_first(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  unsigned long key = *tbl_hdl_p;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_tcam_tbl_map_get_first(dev_id, &key, (void **)&tcam_tbl_info);
  if (msts != BF_MAP_OK) {
    return NULL;
  }
  *tbl_hdl_p = key;
  return tcam_tbl_info;
}

tcam_tbl_info_t *pipe_mgr_tcam_tbl_info_get_next(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  unsigned long key = *tbl_hdl_p;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_tcam_tbl_map_get_next(dev_id, &key, (void **)&tcam_tbl_info);
  if (msts != BF_MAP_OK) {
    return NULL;
  }
  *tbl_hdl_p = key;
  return tcam_tbl_info;
}

static tcam_tbl_t *pipe_mgr_tcam_tbl_alloc(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                           uint32_t no_ptns,
                                           uint32_t total_entries_per_ptn) {
  tcam_tbl_t *tcam_tbls = NULL;
  uint32_t i;

  tcam_tbls = (tcam_tbl_t *)PIPE_MGR_MALLOC(sizeof(tcam_tbl_t) * no_ptns);
  if (tcam_tbls == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMSET(tcam_tbls, 0, sizeof(tcam_tbl_t) * no_ptns);

  for (i = 0; i < no_ptns; i++) {
    tcam_tbl_t *tcam_tbl;
    tcam_tbl = &tcam_tbls[i];

    tcam_tbl->tcam_pipe_tbl_p = tcam_pipe_tbl;
    tcam_tbl->ptn_index = i;
    tcam_tbl->total_entries = total_entries_per_ptn;

    tcam_tbl->hlp.max_tcam_group = 1;
    tcam_tbl->hlp.group_info = (tcam_group_info_t **)PIPE_MGR_CALLOC(
        tcam_tbl->hlp.max_tcam_group, sizeof(tcam_group_info_t *));
    if (tcam_tbl->hlp.group_info == NULL) {
      LOG_ERROR("%s:%d Error creating tcam group info", __func__, __LINE__);
      goto cleanup;
    }

    tcam_tbl->hlp.tcam_entries = (tcam_hlp_entry_t **)PIPE_MGR_CALLOC(
        total_entries_per_ptn, sizeof(tcam_hlp_entry_t *));
    if (tcam_tbl->hlp.tcam_entries == NULL) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      goto cleanup;
    }

    /* LLP state is not required on virtual devices. */
    bf_dev_id_t dev_id = tcam_pipe_tbl->tcam_tbl_info_p->dev_id;
    if (pipe_mgr_is_device_virtual(dev_id)) {
      tcam_tbl->llp.tcam_entries = NULL;
    } else {
      tcam_tbl->llp.tcam_entries = (tcam_llp_entry_t **)PIPE_MGR_CALLOC(
          total_entries_per_ptn, sizeof(tcam_llp_entry_t *));
      if (tcam_tbl->llp.tcam_entries == NULL) {
        LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
        goto cleanup;
      }
    }

    bf_fbs_init(&tcam_tbl->hlp.tcam_used_bmp, total_entries_per_ptn);
  }

  return tcam_tbls;
cleanup:
  pipe_mgr_tcam_tbl_destroy(tcam_tbls, no_ptns);
  return NULL;
}

bool is_rmt_tcam(rmt_tbl_type_t type) {
  switch (type) {
    case RMT_TBL_TYPE_TERN_MATCH:
    case RMT_TBL_TYPE_ATCAM_MATCH:
      return true;
    default:
      return false;
  }
}

static void get_group_and_priority(pipe_tbl_match_spec_t *match_spec,
                                   uint32_t *group_p,
                                   uint32_t *priority_p) {
  uint32_t priority = match_spec->priority;

  uint32_t group = (priority >> TCAM_GROUP_ID_SHIFT);
  priority = priority & ((1 << TCAM_GROUP_ID_SHIFT) - 1);

  *group_p = group;
  *priority_p = priority;
}

/* Update the local state corresponding to the stuff that is not read from
 * hardware - entry priority, ttl values based on the API replay
 */
pipe_status_t pipe_mgr_tcam_entry_update_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  bf_dev_pipe_t pipe_id;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL;
  pipe_mat_ent_hdl_t mat_ent_hdl = move_list_node->entry_hdl;
  pipe_tbl_match_spec_t *match_spec =
      unpack_mat_ent_data_ms(move_list_node->data);
  uint32_t ttl = unpack_mat_ent_data_ttl(move_list_node->data);

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          dev_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  if (!pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, mat_ent_hdl)) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id,
              mat_ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_pipe_tbl->hlp.default_ent_hdl == mat_ent_hdl) {
    LOG_ERROR("Default entry state cannot be deleted. Operation not supported");
    return PIPE_INVALID_ARG;
  }

  /* First find the tcam_index based on the mat-ent-hdl */
  tcam_entry = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, mat_ent_hdl, 0);
  if (tcam_entry == NULL) {
    LOG_ERROR("%s:%d get tcam entry failed", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d tcam entry ptn index out of bounds", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  uint32_t cur_group, cur_priority, tcam_index;
  uint32_t new_group = 0, new_priority = 0;

  cur_group = tcam_entry->group;
  cur_priority = tcam_entry->priority;
  tcam_index = tcam_entry->index;

  get_group_and_priority(match_spec, &new_group, &new_priority);

  rc = pipe_mgr_tcam_update_prio_array(
      tcam_tbl, cur_group, cur_priority, tcam_index, false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating the TCAM priority array for entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        mat_ent_hdl,
        rc);
    return rc;
  }

  tcam_entry->group = new_group;
  tcam_entry->priority = new_priority;
  set_mat_ent_data_priority(tcam_entry->mat_data, match_spec->priority);
  set_mat_ent_data_ttl(tcam_entry->mat_data, ttl);

  rc = pipe_mgr_tcam_update_prio_array(
      tcam_tbl, new_group, new_priority, tcam_index, true);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating the TCAM priority array for entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        mat_ent_hdl,
        rc);
    return rc;
  }

  if (move_head_p && tcam_tbl_info->idle_present) {
    /* Here, generate MOVE-LIST to indicate to LLP that the TTL needs to be
     * updated.
     */
    pipe_mgr_move_list_t *head = *move_head_p;
    pipe_mgr_move_list_t *move_node =
        alloc_move_list(head, PIPE_MAT_UPDATE_ADD_IDLE, move_list_node->pipe);
    move_node->entry_hdl = mat_ent_hdl;
    move_node->data = make_mat_ent_data(NULL, NULL, 0, ttl, 0, 0, 0);
    if (*move_head_p) {
      (*move_head_p)->next = move_node;
    } else {
      *move_head_p = move_node;
    }
  }
  return PIPE_SUCCESS;
}

static tcam_pipe_tbl_t *pipe_mgr_tcam_pipe_tbl_alloc(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_bitmap_t *pipe_bmp,
    uint32_t total_entries,
    uint32_t num_stages,
    uint32_t total_blocks,
    uint32_t blocks_per_bank) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  uint8_t no_tcam_pipe_tbls = 1;
  uint32_t i = 0, j = 0, k = 0;
  uint32_t block_id = 0;
  rmt_tbl_info_t *rmt_info = NULL;
  tcam_block_data_t *tcam_block_data = NULL;
  uint32_t stage_index = 0;
  uint32_t tcam_start_index = 0;
  bf_dev_pipe_t pipe_no = BF_INVALID_PIPE;
  uint32_t no_ptns = mat_tbl_info->num_partitions;
  uint32_t total_entries_per_ptn = 1 + ((total_entries - 1) / no_ptns);

  (void)pipe_bmp;
  no_tcam_pipe_tbls = tcam_tbl_info->no_tcam_pipe_tbls;

  tcam_pipe_tbl = (tcam_pipe_tbl_t *)PIPE_MGR_MALLOC(sizeof(tcam_pipe_tbl_t) *
                                                     no_tcam_pipe_tbls);
  if (tcam_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMSET(
      tcam_pipe_tbl, 0, sizeof(tcam_pipe_tbl_t) * no_tcam_pipe_tbls);

  for (i = 0; i < no_tcam_pipe_tbls; i++) {
    pipe_no =
        pipe_mgr_get_lowest_pipe_in_scope(tcam_tbl_info->scope_pipe_bmp[i]);
    if (!TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info)) {
      PIPE_MGR_DBGCHK(pipe_no != BF_INVALID_PIPE);
    }

    tcam_pipe_tbl[i].tcam_tbl_info_p = tcam_tbl_info;
    tcam_pipe_tbl[i].pipe_idx = i;
    tcam_pipe_tbl[i].pipe_id =
        TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info) ? BF_DEV_PIPE_ALL : pipe_no;

    PIPE_BITMAP_INIT(&tcam_pipe_tbl[i].pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(tcam_tbl_info->scope_pipe_bmp[i],
                                    &tcam_pipe_tbl[i].pipe_bmp);

    tcam_pipe_tbl[i].spec_map.dev_tgt.device_id = tcam_tbl_info->dev_id;
    tcam_pipe_tbl[i].spec_map.dev_tgt.dev_pipe_id = tcam_pipe_tbl[i].pipe_id;
    tcam_pipe_tbl[i].spec_map.mat_tbl_hdl = tcam_tbl_info->tbl_hdl;
    tcam_pipe_tbl[i].spec_map.tbl_info = mat_tbl_info;
    tcam_pipe_tbl[i].spec_map.entry_place_with_hdl_fn =
        pipe_mgr_tcam_entry_place_with_hdl;
    tcam_pipe_tbl[i].spec_map.entry_modify_fn = pipe_mgr_tcam_ent_set_action;
    tcam_pipe_tbl[i].spec_map.entry_delete_fn = pipe_mgr_tcam_entry_del;
    tcam_pipe_tbl[i].spec_map.entry_update_fn =
        pipe_mgr_tcam_entry_update_state;

    tcam_pipe_tbl[i].num_stages = num_stages;
    tcam_pipe_tbl[i].blocks_per_bank = blocks_per_bank;
    tcam_pipe_tbl[i].num_blocks = total_blocks;

    tcam_pipe_tbl[i].hlp.default_ent_hdl =
        PIPE_SET_HDL_PIPE(PIPE_TCAM_DEF_ENT_HDL, pipe_no);
    tcam_pipe_tbl[i].llp.default_ent_hdl =
        PIPE_SET_HDL_PIPE(PIPE_TCAM_DEF_ENT_HDL, pipe_no);
    tcam_pipe_tbl[i].default_ent_type = TCAM_DEFAULT_ENT_TYPE_DIRECT;

    tcam_pipe_tbl[i].llp.tcam_entry_db = NULL;

    tcam_pipe_tbl[i].hlp.tcam_entry_db = NULL;
    tcam_pipe_tbl[i].hlp.ent_hdl_allocator =
        bf_id_allocator_new(total_entries << 1, false);
    bf_id_allocator_set(tcam_pipe_tbl[i].hlp.ent_hdl_allocator,
                        PIPE_TCAM_DEF_ENT_HDL);

    if (!pipe_mgr_is_device_virtual(tcam_tbl_info->dev_id)) {
      tcam_pipe_tbl[i].llp.ha_ent_hdl_allocator =
          bf_id_allocator_new(total_entries << 1, false);
      bf_id_allocator_set(tcam_pipe_tbl[i].llp.ha_ent_hdl_allocator,
                          PIPE_TCAM_DEF_ENT_HDL);
    }
    tcam_pipe_tbl[i].stage_data = (tcam_stage_info_t *)PIPE_MGR_MALLOC(
        num_stages * sizeof(tcam_stage_info_t));
    if (tcam_pipe_tbl[i].stage_data == NULL) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMSET(
        tcam_pipe_tbl[i].stage_data, 0, num_stages * sizeof(tcam_stage_info_t));

    tcam_start_index = 0;
    uint8_t stage_table_idx = 0;
    uint8_t prev_stage_id = ~0;

    if (!mat_tbl_info->num_rmt_info) {
      // Keyless table
      tcam_pipe_tbl[i].stage_data[0].stage_id =
          mat_tbl_info->keyless_info->stage_id;
      tcam_pipe_tbl[i].stage_data[0].stage_table_handle =
          mat_tbl_info->keyless_info->log_tbl_id;
    } else {
      for (j = 0, stage_index = 0; j < mat_tbl_info->num_rmt_info; j++) {
        rmt_info = &mat_tbl_info->rmt_info[j];
        if (!is_rmt_tcam(rmt_info->type)) {
          continue;
        }
        PIPE_MGR_DBGCHK(stage_index < num_stages);

        if (prev_stage_id != rmt_info->stage_id) {
          stage_table_idx = 0;
        } else {
          stage_table_idx++;
        }
        prev_stage_id = rmt_info->stage_id;
        tcam_pipe_tbl[i].stage_data[stage_index].stage_id = rmt_info->stage_id;
        tcam_pipe_tbl[i].stage_data[stage_index].stage_table_handle =
            rmt_info->handle;
        tcam_pipe_tbl[i].stage_data[stage_index].stage_table_idx =
            stage_table_idx;
        tcam_pipe_tbl[i].stage_data[stage_index].tcam_start_index =
            tcam_start_index;
        tcam_pipe_tbl[i].stage_data[stage_index].no_tcam_entries =
            (rmt_info->num_entries + no_ptns - 1) / no_ptns;

        tcam_pipe_tbl[i].stage_data[stage_index].mem_depth =
            pipe_mgr_get_mem_unit_depth(tcam_tbl_info->dev_id,
                                        rmt_info->mem_type);

        tcam_pipe_tbl[i].stage_data[stage_index].movereg_src_addr =
            PIPE_MGR_INVALID_MOVEREG_ADDR;
        tcam_pipe_tbl[i].stage_data[stage_index].movereg_dest_addr =
            PIPE_MGR_INVALID_MOVEREG_ADDR;
        tcam_pipe_tbl[i].stage_data[stage_index].pack_format =
            rmt_info->pack_format;

        if (pipe_mgr_tcam_tind_tbl_alloc(
                &(tcam_pipe_tbl[i].stage_data[stage_index]), mat_tbl_info) !=
            PIPE_SUCCESS) {
          LOG_ERROR("%s:%d Ternary indirection table alloc failed",
                    __func__,
                    __LINE__);
          goto cleanup;
        }
        tcam_pipe_tbl[i].stage_data[stage_index].tind_act_fn_hdl =
            rmt_info->tind_act_fn_hdl;

        tcam_start_index += (rmt_info->num_entries + no_ptns - 1) / no_ptns;
        stage_index++;
      }
    }

    tcam_pipe_tbl[i].block_data = (tcam_block_data_t *)PIPE_MGR_MALLOC(
        total_blocks * sizeof(tcam_block_data_t));
    if (tcam_pipe_tbl[i].block_data == NULL) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      goto cleanup;
    }

    PIPE_MGR_MEMSET(tcam_pipe_tbl[i].block_data,
                    0,
                    total_blocks * sizeof(tcam_block_data_t));

    block_id = 0;
    for (j = 0, stage_index = 0; j < mat_tbl_info->num_rmt_info; j++) {
      rmt_info = &mat_tbl_info->rmt_info[j];
      if (!is_rmt_tcam(rmt_info->type)) {
        continue;
      }

      tcam_pipe_tbl[i].stage_data[stage_index].block_start_index = block_id;
      uint32_t bank = 0;
      for (bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
        for (k = 0; k < rmt_info->bank_map[bank].num_tbl_word_blks; k++) {
          PIPE_MGR_DBGCHK(block_id < total_blocks);
          tcam_block_data = &tcam_pipe_tbl[i].block_data[block_id];

          tcam_block_data->stage_index = stage_index;

          PIPE_MGR_MEMCPY(&tcam_block_data->word_blk,
                          &rmt_info->bank_map[bank].tbl_word_blk[k],
                          sizeof(rmt_tbl_word_blk_t));
          block_id++;
        }
      }

      stage_index++;
    }
    PIPE_MGR_DBGCHK(!mat_tbl_info->num_rmt_info || stage_index == num_stages);

    tcam_pipe_tbl[i].tcam_ptn_tbls = pipe_mgr_tcam_tbl_alloc(
        &tcam_pipe_tbl[i], no_ptns, total_entries_per_ptn);
    if (tcam_pipe_tbl[i].tcam_ptn_tbls == NULL) {
      goto cleanup;
    }
    tcam_pipe_tbl[i].no_ptns = no_ptns;
  }

  return tcam_pipe_tbl;
cleanup:
  pipe_mgr_tcam_pipe_tbl_destroy(tcam_pipe_tbl, no_tcam_pipe_tbls);
  return NULL;
}

static void pipe_mgr_tcam_tbl_info_destroy(tcam_tbl_info_t *tcam_tbl_info) {
  if (tcam_tbl_info == NULL) {
    return;
  }

  if (tcam_tbl_info->tcam_pipe_tbl) {
    pipe_mgr_tcam_pipe_tbl_destroy(tcam_tbl_info->tcam_pipe_tbl,
                                   tcam_tbl_info->no_tcam_pipe_tbls);
  }

  if (tcam_tbl_info->tbl_refs) {
    PIPE_MGR_FREE(tcam_tbl_info->tbl_refs);
  }

  PIPE_MGR_FREE(tcam_tbl_info->name);
  if (tcam_tbl_info->scope_pipe_bmp) {
    PIPE_MGR_FREE(tcam_tbl_info->scope_pipe_bmp);
  }

  if (tcam_tbl_info->act_fn_hdl_info) {
    PIPE_MGR_FREE(tcam_tbl_info->act_fn_hdl_info);
  }
  PIPE_MGR_LOCK_DESTROY(&tcam_tbl_info->tbl_lock);
  PIPE_MGR_FREE(tcam_tbl_info);
}

static pipe_status_t get_num_stages_and_blocks(
    pipe_mat_tbl_info_t *mat_tbl_info,
    uint32_t *total_entries_p,
    uint32_t *num_stages_p,
    uint32_t *total_blocks_p,
    uint32_t *blocks_per_bank_p,
    bool *idle_present_p) {
  uint32_t i = 0;
  rmt_tbl_info_t *rmt_info = NULL;
  uint32_t total_entries = 0, total_blocks = 0;
  bool idle_present = false;
  bool blocks_per_bank_set = false;
  uint32_t blocks_per_bank = 0;
  uint32_t num_stages = 0;

  *total_entries_p = 0;
  *num_stages_p = 0;
  *total_blocks_p = 0;
  *blocks_per_bank_p = 0;
  *idle_present_p = false;

  /* First figure out the num of blocks needed and total entries */
  for (i = 0; i < mat_tbl_info->num_rmt_info; i++) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->type == RMT_TBL_TYPE_IDLE_TMO) {
      idle_present = true;
    }
    if (!is_rmt_tcam(rmt_info->type)) {
      continue;
    }

    /* Each rmt-info with different logical table IDs are treated as
     * different stages
     */
    num_stages++;
    total_entries += rmt_info->num_entries;

    uint32_t bank;
    for (bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
      if (blocks_per_bank_set == false) {
        blocks_per_bank = rmt_info->bank_map[bank].num_tbl_word_blks;
        blocks_per_bank_set = true;
      }

      if ((mat_tbl_info->match_type == ATCAM_MATCH) &&
          (rmt_info->bank_map[bank].num_tbl_word_blks != blocks_per_bank)) {
        LOG_ERROR(
            "%s:%d Error - table with incorrect blocks per bank %d %d"
            " tbl %s(%d)",
            __func__,
            __LINE__,
            rmt_info->bank_map[bank].num_tbl_word_blks,
            blocks_per_bank,
            mat_tbl_info->name,
            mat_tbl_info->handle);
        return PIPE_INVALID_ARG;
      }
      total_blocks += rmt_info->bank_map[bank].num_tbl_word_blks;
    }
  }
  *total_entries_p = total_entries;
  *num_stages_p = num_stages;
  *total_blocks_p = total_blocks;
  *blocks_per_bank_p = blocks_per_bank;
  *idle_present_p = idle_present;

  if (mat_tbl_info->num_rmt_info == 0) {
    // Keyless case
    *total_entries_p = 1;
    *num_stages_p = 1;
    *total_blocks_p = 1;
    *blocks_per_bank_p = 1;
    *idle_present_p = false;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_install_def_entry(
    tcam_tbl_info_t *tcam_tbl_info, bool is_backup) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  int i = 0;

  for (i = 0; !is_backup && (i < tcam_tbl_info->no_tcam_pipe_tbls); i++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];

    tcam_tbl_t *tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[0];

    tcam_entry = pipe_mgr_tcam_entry_alloc();
    if (tcam_entry == NULL) {
      LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    tcam_entry->range_list_p = &tcam_entry->range_list;

    BF_LIST_DLL_AP(
        *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
    if (tcam_entry == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    tcam_entry->is_default = true;
    bool need_reserved_entry = false;

    pipe_mat_tbl_info_t *mat_info = pipe_mgr_get_tbl_info(
        tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, __func__, __LINE__);
    if (!mat_info) {
      LOG_ERROR("%s:%d Error looking up MAT info, dev %d tbl %s 0x%x",
                __func__,
                __LINE__,
                tcam_tbl_info->dev_id,
                tcam_tbl_info->name,
                tcam_tbl_info->tbl_hdl);
      return PIPE_UNEXPECTED;
    }
    rc = pipe_mgr_tbl_default_entry_needs_reserve(tcam_tbl_info->dev_info,
                                                  mat_info,
                                                  tcam_tbl_info->idle_present,
                                                  &need_reserved_entry);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error reserving default TCAM entry rc 0x%x",
                __func__,
                __LINE__,
                rc);
      return rc;
    }

    if (need_reserved_entry == true) {
      tcam_entry->priority = -1;
      tcam_entry->group = TCAM_MAX_GROUPS - 1;
      pipe_mgr_tcam_create_group(tcam_tbl, tcam_entry->group);
      rc = pipe_mgr_tcam_set_tcam_index(tcam_tbl,
                                        tcam_tbl->total_entries - 1,
                                        tcam_entry,
                                        PIPE_TCAM_OP_RESERVE_DEFAULT);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error reserving default TCAM entry rc 0x%x",
                  __func__,
                  __LINE__,
                  rc);
        return rc;
      }
      tcam_tbl->hlp.total_usage++;
      tcam_tbl->llp.total_usage++;
      tcam_tbl->llp.total_hw_usage++;

      tcam_pipe_tbl->default_ent_type = TCAM_DEFAULT_ENT_TYPE_DIRECT;
    } else {
      tcam_pipe_tbl->default_ent_type = TCAM_DEFAULT_ENT_TYPE_INDIRECT;
    }
    tcam_pipe_tbl->hlp.hlp_default_tcam_entry = tcam_entry;
  }

  return rc;
}

static pipe_status_t pipe_mgr_tcam_reserve_modify_entry(
    tcam_tbl_info_t *tcam_tbl_info, pipe_mat_tbl_info_t *mat_tbl_info) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_adt_tbl_info_t *adt_tbl_info = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  uint8_t num_adt_rams = 0;
  bool uses_imm_data = false;
  rmt_dev_info_t *pipe_mgr_dev_info;

  pipe_mgr_dev_info = pipe_mgr_get_dev_info(tcam_tbl_info->dev_id);
  if (pipe_mgr_dev_info == NULL) {
    LOG_ERROR("%s:%d get pipe mgr info failed", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
    tcam_tbl_info->reserve_modify_ent = false;
    return status;
  }

  if (mat_tbl_info->num_rmt_info == 0 || !tcam_tbl_info->adt_present ||
      tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    tcam_tbl_info->reserve_modify_ent = false;
  } else if (tcam_tbl_info->num_actions > 1) {
    tcam_tbl_info->reserve_modify_ent = true;
  } else {
    adt_tbl_info = pipe_mgr_get_adt_tbl_info(tcam_tbl_info->dev_id,
                                             tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                                             __func__,
                                             __LINE__);
    if (adt_tbl_info == NULL) {
      LOG_ERROR(
          "%s : Could not get the action data table info from the "
          " rmt cfg database for table with handle %d, device id "
          " %d",
          __func__,
          tcam_tbl_info->adt_tbl_ref.tbl_hdl,
          tcam_tbl_info->dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (adt_tbl_info->num_rmt_info) {
      num_adt_rams =
          adt_tbl_info->rmt_info[0].pack_format.mem_units_per_tbl_word;
    }

    act_fn_hdl = tcam_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
    status = pipe_mgr_entry_format_tof_tbl_uses_imm_data(
        tcam_tbl_info->dev_id,
        tcam_tbl_info->profile_id,
        mat_tbl_info->rmt_info[0].stage_id,
        tcam_tbl_info->tbl_hdl,
        act_fn_hdl,
        !TCAM_TBL_IS_ATCAM(tcam_tbl_info),
        &uses_imm_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to get entry encoding info for tcam table 0x%x in "
          "device %d",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id);
      return status;
    }

    tcam_tbl_info->reserve_modify_ent =
        (num_adt_rams > 1 || tcam_tbl_info->num_tbl_refs > 0 || uses_imm_data);
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_tbl_create_internal
 *        Internal helper function to create a tcam_tbl structure
 *
 * Function mallocs a tcam-tbl structure, updates all the relevant stuff
 * inside and also mallocs any of the sub-structures/inits the different
 * trees etc.
 *
 * \param  Same as pipe_mgr_tcam_tbl_create
 * \param is_backup Flag to specify if a backup table is being created
 * \return pipe_status_t Status of the operation
 */
static pipe_status_t pipe_mgr_tcam_tbl_create_internal(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_tbl_info_t *mat_tbl_info,
    bool is_backup,
    profile_id_t profile_id,
    pipe_bitmap_t *pipe_bmp) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t total_entries, total_blocks;
  bool idle_present;
  uint32_t blocks_per_bank;
  uint32_t num_stages, q = 0;

  rc = get_num_stages_and_blocks(mat_tbl_info,
                                 &total_entries,
                                 &num_stages,
                                 &total_blocks,
                                 &blocks_per_bank,
                                 &idle_present);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  if ((total_entries == 0) || (blocks_per_bank == 0) || (total_blocks == 0)) {
    LOG_ERROR("%s:%d Error - table with 0 size dev_id %d tbl_hdl %d",
              __func__,
              __LINE__,
              dev_id,
              tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (mat_tbl_info->match_type != ATCAM_MATCH) {
    if (!(mat_tbl_info->num_rmt_info == 0 ||
          !(total_entries % PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK))) {
      PIPE_MGR_DBGCHK(0);
    }
    PIPE_MGR_DBGCHK(((total_entries - 1) / PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK +
                     1) == total_blocks);
  }

  tcam_tbl_info = (tcam_tbl_info_t *)PIPE_MGR_MALLOC(sizeof(tcam_tbl_info_t));
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  PIPE_MGR_MEMSET(tcam_tbl_info, 0, sizeof(tcam_tbl_info_t));

  tcam_tbl_info->name = bf_sys_strdup(mat_tbl_info->name);
  tcam_tbl_info->direction = mat_tbl_info->direction;
  tcam_tbl_info->dev_id = dev_id;
  tcam_tbl_info->dev_info = pipe_mgr_get_dev_info(dev_id);
  if (tcam_tbl_info->dev_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  tcam_tbl_info->tbl_hdl = tbl_hdl;
  tcam_tbl_info->match_type = mat_tbl_info->match_type;
  tcam_tbl_info->uses_range = mat_tbl_info->uses_range;
  tcam_tbl_info->uses_alpm =
      (TCAM_TBL_IS_ATCAM(tcam_tbl_info) && mat_tbl_info->alpm_info);
  tcam_tbl_info->profile_id = profile_id;
  tcam_tbl_info->lock_type = LOCK_ID_TYPE_INVALID;
  PIPE_MGR_LOCK_INIT(tcam_tbl_info->tbl_lock);
  /* Minimum size of tcam tbl that compiler can allocate is 512.
     Store the actual tbl size specified in P4.
  */
  tcam_tbl_info->tbl_size_in_p4 = mat_tbl_info->size;
  tcam_tbl_info->tbl_size_max = mat_tbl_info->size;
  LOG_TRACE("%s: Table %s, Pipe bitmap count %d ",
            __func__,
            tcam_tbl_info->name,
            PIPE_BITMAP_COUNT(pipe_bmp));

  tcam_tbl_info->is_symmetric = mat_tbl_info->symmetric;
  tcam_tbl_info->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!tcam_tbl_info->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  /* Set the scope info */
  if (tcam_tbl_info->is_symmetric) {
    tcam_tbl_info->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      tcam_tbl_info->scope_pipe_bmp[0] |= (1 << q);
    }
  } else {
    tcam_tbl_info->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      tcam_tbl_info->scope_pipe_bmp[q] |= (1 << q);
      tcam_tbl_info->num_scopes += 1;
    }
  }

  if (mat_tbl_info->adt_tbl_ref) {
    PIPE_MGR_MEMCPY(&tcam_tbl_info->adt_tbl_ref,
                    mat_tbl_info->adt_tbl_ref,
                    sizeof(pipe_tbl_ref_t));
    tcam_tbl_info->adt_present = true;
  }
  if (mat_tbl_info->sel_tbl_ref) {
    PIPE_MGR_MEMCPY(&tcam_tbl_info->sel_tbl_ref,
                    mat_tbl_info->sel_tbl_ref,
                    sizeof(pipe_tbl_ref_t));
    tcam_tbl_info->sel_present = true;
  }
  tcam_tbl_info->idle_present = idle_present;
  if (tcam_tbl_info->is_symmetric) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->num_scopes == 1);
    tcam_tbl_info->llp.lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(tcam_tbl_info->scope_pipe_bmp[0]);
  }

  /* Store the number of match spec bits and bytes */
  tcam_tbl_info->num_match_spec_bits = mat_tbl_info->num_match_bits;
  tcam_tbl_info->num_match_spec_bytes = mat_tbl_info->num_match_bytes;
  unsigned i = 0;
  uint32_t max_bytes = 0;
  for (i = 0; i < mat_tbl_info->num_actions; i++) {
    if (mat_tbl_info->act_fn_hdl_info[i].num_bytes > max_bytes) {
      max_bytes = mat_tbl_info->act_fn_hdl_info[i].num_bytes;
    }
  }
  tcam_tbl_info->max_act_data_size = max_bytes;

  uint32_t num_tbl_refs = 0;
  num_tbl_refs = mat_tbl_info->num_stat_tbl_refs +
                 mat_tbl_info->num_meter_tbl_refs +
                 mat_tbl_info->num_stful_tbl_refs;

  pipe_tbl_ref_t *tbl_refs = NULL;
  if (num_tbl_refs) {
    tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_MALLOC(sizeof(pipe_tbl_ref_t) *
                                                 num_tbl_refs);
    if (tbl_refs == NULL) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  pipe_tbl_ref_t *tbl_ref = tbl_refs;

  uint32_t tbl_idx;
  for (tbl_idx = 0; tbl_idx < mat_tbl_info->num_stat_tbl_refs;
       tbl_idx++, tbl_ref++) {
    if (tbl_ref) {
      PIPE_MGR_MEMCPY(tbl_ref,
                      &mat_tbl_info->stat_tbl_ref[tbl_idx],
                      sizeof(pipe_tbl_ref_t));
    }
  }
  for (tbl_idx = 0; tbl_idx < mat_tbl_info->num_meter_tbl_refs;
       tbl_idx++, tbl_ref++) {
    if (tbl_ref) {
      PIPE_MGR_MEMCPY(tbl_ref,
                      &mat_tbl_info->meter_tbl_ref[tbl_idx],
                      sizeof(pipe_tbl_ref_t));
    }
  }
  for (tbl_idx = 0; tbl_idx < mat_tbl_info->num_stful_tbl_refs;
       tbl_idx++, tbl_ref++) {
    if (tbl_ref) {
      PIPE_MGR_MEMCPY(tbl_ref,
                      &mat_tbl_info->stful_tbl_ref[tbl_idx],
                      sizeof(pipe_tbl_ref_t));
    }
  }

  tcam_tbl_info->num_tbl_refs = num_tbl_refs;
  tcam_tbl_info->tbl_refs = tbl_refs;
  tcam_tbl_info->no_tcam_pipe_tbls = tcam_tbl_info->num_scopes;

  tcam_tbl_info->tcam_pipe_tbl = pipe_mgr_tcam_pipe_tbl_alloc(tcam_tbl_info,
                                                              mat_tbl_info,
                                                              pipe_bmp,
                                                              total_entries,
                                                              num_stages,
                                                              total_blocks,
                                                              blocks_per_bank);
  if (tcam_tbl_info->tcam_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d Tcam table creation failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  /* All the mallocs and data structures have been set up,
   * Add it to the hash tbl
   */

  bf_map_sts_t msts;
  msts = pipe_mgr_tcam_tbl_map_add(
      dev_id, tbl_hdl, (void *)tcam_tbl_info, is_backup);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding tcam table sts %d",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              msts);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  /* Adding to hash table should be at the end. Else, need to
   * handle removal in cleanup
   */

  /* Do any reservations that are needed */
  rc = pipe_mgr_tcam_install_def_entry(tcam_tbl_info, is_backup);
  if (rc != PIPE_SUCCESS) {
    LOG_TRACE("%s:%d Error reserving default TCAM entry rc 0x%x",
              __func__,
              __LINE__,
              rc);
    goto cleanup;
  }
  if (mat_tbl_info->match_type != ATCAM_MATCH &&
      tcam_tbl_info->tcam_pipe_tbl[0].default_ent_type ==
          TCAM_DEFAULT_ENT_TYPE_DIRECT &&
      tcam_tbl_info->tbl_size_max < total_entries) {
    /* Try to expand the tcam table by one if we need to reserve a slot for
     * default entry
     */
    tcam_tbl_info->tbl_size_max++;
  }

  /* Update the lock type for stats */
  tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
      tcam_tbl_info, PIPE_HDL_TYPE_STAT_TBL, PIPE_TBL_REF_TYPE_DIRECT);

  if (tbl_ref) {
    pipe_stat_tbl_info_t *stat_tbl_info = NULL;
    stat_tbl_info = pipe_mgr_get_stat_tbl_info(
        tcam_tbl_info->dev_id, tbl_ref->tbl_hdl, __func__, __LINE__);
    if (stat_tbl_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
    if (stat_tbl_info->lrt_enabled) {
      rc = pipe_mgr_tcam_update_lock_type(
          tcam_tbl_info->dev_id, tbl_hdl, false, true, true);
      if (rc != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d Error in updating lock type for stats, rc 0x%x",
                  __func__,
                  __LINE__,
                  rc);
        goto cleanup;
      }
    }
  }

  if (!pipe_mgr_is_device_virtual(dev_id) && mat_tbl_info->num_rmt_info) {
    /* Init shadow memory related metadata */
    rc = pipe_mgr_tcam_init_shadow_mem(tcam_tbl_info);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing shadow mem for tbl 0x%x, dev id %d"
          " err %s",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      goto cleanup;
    }
  }

  tcam_tbl_info->num_actions = mat_tbl_info->num_actions;
  tcam_tbl_info->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
      tcam_tbl_info->num_actions, sizeof(pipe_act_fn_info_t));
  if (tcam_tbl_info->act_fn_hdl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }
  PIPE_MGR_MEMCPY(tcam_tbl_info->act_fn_hdl_info,
                  mat_tbl_info->act_fn_hdl_info,
                  sizeof(pipe_act_fn_info_t) * tcam_tbl_info->num_actions);

  if (!is_backup && !mat_tbl_info->disable_atomic_modify) {
    rc = pipe_mgr_tcam_reserve_modify_entry(tcam_tbl_info, mat_tbl_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error reserving modify TCAM entry rc 0x%x",
                __func__,
                __LINE__,
                rc);
      goto cleanup;
    }
  }

  return PIPE_SUCCESS;

cleanup:

  pipe_mgr_tcam_tbl_info_destroy(tcam_tbl_info);

  return rc;
}

/** \brief pipe_mgr_tcam_tbl_create
 *         Create the local structures to hold data about a new tcam tbl
 *
 * This routine creates the local data structures needed for tcam tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the tcam tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_tcam_tbl_create(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  LOG_TRACE("Entered %s for dev %d profile %d ", __func__, dev_id, profile_id);

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table 0x%x not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false)) {
    /* Table already exists, error out */
    LOG_ERROR("%s:%d Table 0x%x on device %d already exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  rc = pipe_mgr_tcam_tbl_create_internal(
      dev_id, tbl_hdl, mat_tbl_info, false, profile_id, pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error creating tcam table for 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return rc;
  }

  rc = pipe_mgr_tcam_tbl_create_internal(
      dev_id, tbl_hdl, mat_tbl_info, true, profile_id, pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error creating backup tcam table for 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return rc;
  }

  LOG_TRACE("%s: Successfully created tcam tbl %s(0x%x)",
            __func__,
            mat_tbl_info->name,
            tbl_hdl);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_tbl_delete_internal(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t tbl_hdl, bool is_backup) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_TRACE("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Remove from the hash table */
  bf_map_sts_t msts;
  msts = pipe_mgr_tcam_tbl_map_rmv(dev_id, tbl_hdl, is_backup);

  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting tcam table sts %d",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              msts);
    return PIPE_UNEXPECTED;
  }

  pipe_mgr_tcam_tbl_info_destroy(tcam_tbl_info);

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_tbl_delete
 *         Deletes the local structures that hold data about a tcam tbl
 *
 * This routine deletes the local data structures needed for tcam tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the tcam tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_tcam_tbl_delete(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_tcam_tbl_delete_internal(dev_id, tbl_hdl, false);
  if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
    LOG_ERROR("%s:%d Error deleting table tbl_hdl %d dev_id %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id,
              rc);
    return rc;
  }

  rc = pipe_mgr_tcam_tbl_delete_internal(dev_id, tbl_hdl, true);
  if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
    LOG_ERROR("%s:%d Error deleting table tbl_hdl %d dev_id %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id,
              rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_htbl_add_delete
 *        Add/delete a tcam_entry into the tcam_entries_htbl
 *
 * \param tcam_tbl Pointer to tcam table
 * \param ent_hdl TCAM entry handle
 * \param tcam_entry Pointer to the tcam entry
 * \param is_add true for add, false for delete
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_entry_htbl_add_delete(tcam_tbl_t *tcam_tbl,
                                                  pipe_mat_ent_hdl_t ent_hdl,
                                                  tcam_hlp_entry_t *tcam_entry,
                                                  bool is_add) {
  if (tcam_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  bf_map_sts_t st = BF_MAP_OK;

  if (is_add) {
    st = bf_map_add(
        &get_tcam_pipe_tbl(tcam_tbl)->hlp.tcam_entry_db, ent_hdl, tcam_entry);
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error adding tcam entry to lookup "
          "array for entry-hdl 0x%x",
          __func__,
          __LINE__,
          get_tcam_tbl_info(tcam_tbl)->name,
          get_tcam_tbl_info(tcam_tbl)->tbl_hdl,
          ent_hdl);
      return PIPE_NO_SYS_RESOURCES;
    }
  } else {
    st = bf_map_rmv(&get_tcam_pipe_tbl(tcam_tbl)->hlp.tcam_entry_db, ent_hdl);
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error deleting tcam entry from lookup "
          "array for entry-hdl 0x%x",
          __func__,
          __LINE__,
          get_tcam_tbl_info(tcam_tbl)->name,
          get_tcam_tbl_info(tcam_tbl)->tbl_hdl,
          ent_hdl);
      return PIPE_UNEXPECTED;
    }
  }

  return PIPE_SUCCESS;
}

tcam_hlp_entry_t *pipe_mgr_tcam_entry_get(tcam_pipe_tbl_t *tcam_pipe_tbl,
                                          pipe_mat_ent_hdl_t ent_hdl,
                                          uint32_t subindex) {
  if (tcam_pipe_tbl->hlp.default_ent_set &&
      (tcam_pipe_tbl->hlp.default_ent_hdl == ent_hdl)) {
    tcam_hlp_entry_t *tcam_entry;
    tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
    return tcam_entry;
  }

  bf_map_sts_t st = BF_MAP_OK;

  tcam_hlp_entry_t *head_entry = NULL;
  st = bf_map_get(
      &tcam_pipe_tbl->hlp.tcam_entry_db, ent_hdl, (void **)&head_entry);
  if (st == BF_MAP_NO_KEY) {
    return NULL;
  }
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error looking up tcam entry"
        " 0x%x ",
        __func__,
        __LINE__,
        tcam_pipe_tbl->tcam_tbl_info_p->name,
        tcam_pipe_tbl->tcam_tbl_info_p->tbl_hdl,
        ent_hdl);
    return NULL;
  }

  tcam_hlp_entry_t *tcam_entry = NULL;
  for (tcam_entry = head_entry; tcam_entry; tcam_entry = tcam_entry->next) {
    PIPE_MGR_DBGCHK(TCAM_HLP_IS_RANGE_HEAD(tcam_entry));
    tcam_hlp_entry_t *range_entry = NULL;
    for (range_entry = tcam_entry; range_entry;
         range_entry = range_entry->next_range) {
      if (range_entry->subentry_index == subindex) {
        return range_entry;
      }
    }
  }

  return NULL;
}

/** \brief pipe_mgr_tcam_abort
 *        Abort a session for the given table handle
 *
 * This function should be called during abort to restore the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \param pipes_list points to the list of the relevant pipes
 * \param nb_pipes is the number of relevant pipes in pipes_list
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_abort(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes) {
  tcam_tbl_info_t *btcam_tbl_info;
  tcam_pipe_tbl_t *btcam_pipe_tbl;
  tcam_tbl_info_t *tcam_tbl_info;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  unsigned i, pipe;
  bool symmetric;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  btcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, true);
  if (btcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup tcam table for %d",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info)) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe);
    btcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(btcam_tbl_info, pipe);
    if (!tcam_pipe_tbl && !btcam_pipe_tbl) continue;

    if ((!tcam_pipe_tbl && btcam_pipe_tbl) ||
        (tcam_pipe_tbl && !btcam_pipe_tbl)) {
      /* That should not happen. */
      LOG_ERROR("%s:%d tcam pipe table mismatch tbl %d pipe %d",
                __func__,
                __LINE__,
                tbl_hdl,
                pipe);
      continue;
    }

    pipe_mgr_tcam_restore_all(tcam_pipe_tbl, btcam_pipe_tbl);
    tcam_pipe_tbl->cur_sess_hdl = -1;
    tcam_pipe_tbl->sess_flags = 0;
  }

  if (tcam_tbl_info->idle_present) {
    rmt_idle_abort_txn(dev_id, tbl_hdl, pipes_list, nb_pipes);
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_commit
 *        Commit the state associated with a session
 *
 * This function should be called during commit to discard the state from
 * backed up state
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \param pipes_list points to the list of the relevant pipes
 * \param nb_pipes is the number of relevant pipes in pipes_list
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_commit(bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                   bf_dev_pipe_t *pipes_list,
                                   unsigned nb_pipes) {
  tcam_tbl_info_t *btcam_tbl_info;
  tcam_pipe_tbl_t *btcam_pipe_tbl;
  tcam_tbl_info_t *tcam_tbl_info;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  unsigned i, pipe;
  bool symmetric;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  btcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, true);
  if (btcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Unable to find the backup tcam table for %d",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info)) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    btcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(btcam_tbl_info, pipe);
    if (btcam_pipe_tbl) {
      pipe_mgr_tcam_discard_all(btcam_pipe_tbl);
    }

    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe);
    if (tcam_pipe_tbl) {
      tcam_pipe_tbl->cur_sess_hdl = -1;
      tcam_pipe_tbl->sess_flags = 0;
    }
  }

  if (tcam_tbl_info->idle_present) {
    rmt_idle_commit_txn(dev_id, tbl_hdl, pipes_list, nb_pipes);
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_atom_cleanup
 *        Cleanup the state  associated with atomic transaction
 *
 * This function will cleanup the tcam state for the atomic transaction
 * including removing version matches in the tcam entries and deleting the
 * tcam entries
 *
 * \param dev_id Device id
 * \param tbl_hdl TCAM table handle
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_atom_cleanup(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         bf_dev_pipe_t *pipes_list,
                                         unsigned nb_pipes) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bool symmetric;
  unsigned pipe;
  uint8_t i = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info)) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe);
    if (!tcam_pipe_tbl) continue;

    pipe_mgr_tcam_commit_complete(tcam_pipe_tbl);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_verify_resources(
    tcam_tbl_t *tcam_tbl, pipe_action_spec_t *act_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  int i;
  pipe_tbl_ref_t *tbl_ref = NULL;
  pipe_hdl_type_t hdl_type;
  pipe_tbl_hdl_t res_hdl = 0;
  pipe_idx_t res_idx = 0;

  for (i = 0; i < act_spec->resource_count; i++) {
    res_hdl = act_spec->resources[i].tbl_hdl;
    res_idx = act_spec->resources[i].tbl_idx;
    hdl_type = PIPE_GET_HDL_TYPE(res_hdl);
    switch (hdl_type) {
      case PIPE_HDL_TYPE_STAT_TBL:
        tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
            tcam_tbl_info, PIPE_HDL_TYPE_STAT_TBL, PIPE_TBL_REF_TYPE_INDIRECT);
        if (tbl_ref) {
          if (tbl_ref->tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid stats tbl hdl 0x%x for tcam tbl 0x%x device %d, "
                "which expects stats tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                tbl_ref->tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (act_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_stat_mgr_verify_idx(tcam_tbl_info->dev_id,
                                                  tcam_pipe_tbl->pipe_id,
                                                  res_hdl,
                                                  res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying stats idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "stat tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id,
                  tcam_pipe_tbl->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
            tcam_tbl_info, PIPE_HDL_TYPE_METER_TBL, PIPE_TBL_REF_TYPE_INDIRECT);
        if (tbl_ref) {
          if (tbl_ref->tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid meter tbl hdl 0x%x for tcam tbl 0x%x device %d, "
                "which expects meter tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                tbl_ref->tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (act_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_meter_verify_idx(tcam_tbl_info->dev_id,
                                               tcam_pipe_tbl->pipe_id,
                                               res_hdl,
                                               res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying meter idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "meter tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id,
                  tcam_pipe_tbl->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
            tcam_tbl_info, PIPE_HDL_TYPE_STFUL_TBL, PIPE_TBL_REF_TYPE_INDIRECT);
        if (tbl_ref) {
          if (tbl_ref->tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid stful tbl hdl 0x%x for tcam tbl 0x%x device %d, "
                "which expects stful tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                tbl_ref->tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (act_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_stful_verify_idx(tcam_tbl_info->dev_id,
                                               tcam_pipe_tbl->pipe_id,
                                               res_hdl,
                                               res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying stful idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "stful tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id,
                  tcam_pipe_tbl->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      default:
        LOG_ERROR(
            "%s:%d Invalid resource handle type for resource tbl 0x%x "
            "for tcam tbl 0x%x device id %d",
            __func__,
            __LINE__,
            res_hdl,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id);
        return PIPE_INVALID_ARG;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_alloc_actions(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_entry,
    pipe_action_spec_t *act_spec,
    tcam_phy_loc_info_t *tcam_loc) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->adt_tbl_ref.ref_type ==
                    PIPE_TBL_REF_TYPE_INDIRECT);
    rc = rmt_adt_ent_place(tcam_tbl_info->dev_id,
                           get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
                           tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                           act_spec->adt_ent_hdl,
                           tcam_loc->stage_id,
                           &tcam_entry->logical_action_idx,
                           get_tcam_pipe_tbl(tcam_tbl)->sess_flags);
  } else if (IS_ACTION_SPEC_SEL_GRP(act_spec)) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->sel_tbl_ref.ref_type ==
                    PIPE_TBL_REF_TYPE_INDIRECT);
    rc = rmt_sel_grp_activate_stage(get_tcam_pipe_tbl(tcam_tbl)->cur_sess_hdl,
                                    tcam_tbl_info->dev_id,
                                    get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
                                    tcam_tbl_info->tbl_hdl,
                                    tcam_entry->entry_hdl,
                                    tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                    act_spec->sel_grp_hdl,
                                    tcam_loc->stage_id,
                                    &tcam_entry->logical_action_idx,
                                    &tcam_entry->selector_len,
                                    &tcam_entry->logical_sel_idx,
                                    get_tcam_pipe_tbl(tcam_tbl)->sess_flags);
  } else {
    tcam_entry->logical_action_idx = 0;
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error allocating actions for tcam entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_verify_resources(tcam_tbl, act_spec);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error verifying resources for tcam entry %d in table 0x%x "
        "device %d",
        __func__,
        __LINE__,
        tcam_entry->entry_hdl,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_dealloc_actions(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_entry,
    pipe_action_spec_t *act_spec,
    tcam_phy_loc_info_t *tcam_loc) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (IS_ACTION_SPEC_SEL_GRP(act_spec)) {
    /* If we had Selector earlier, call selector deactivate */
    rc = rmt_sel_grp_deactivate_stage(get_tcam_pipe_tbl(tcam_tbl)->cur_sess_hdl,
                                      tcam_tbl_info->dev_id,
                                      get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
                                      tcam_tbl_info->tbl_hdl,
                                      tcam_entry->entry_hdl,
                                      tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                      act_spec->sel_grp_hdl,
                                      tcam_loc->stage_id,
                                      get_tcam_pipe_tbl(tcam_tbl)->sess_flags);
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    rc = rmt_adt_ent_remove(tcam_tbl_info->dev_id,
                            tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                            act_spec->adt_ent_hdl,
                            tcam_loc->stage_id,
                            get_tcam_pipe_tbl(tcam_tbl)->sess_flags);
  }
  tcam_entry->logical_action_idx = 0;
  tcam_entry->logical_sel_idx = 0;
  tcam_entry->selector_len = 0;

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deallocating actions for tcam entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_update_actions(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_entry,
    pipe_tcam_op_e tcam_op,
    pipe_action_spec_t *act_spec,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc) {
  pipe_status_t rc;
  bool dealloc_needed = false, alloc_needed = false;
  bool cross_stage_move = false;

  if ((tcam_op == PIPE_TCAM_OP_MOVE) &&
      (dest_loc->stage_id != src_loc->stage_id)) {
    cross_stage_move = true;
  }

  if ((tcam_op == PIPE_TCAM_OP_DELETE) || (tcam_op == PIPE_TCAM_OP_MODIFY) ||
      (tcam_op == PIPE_TCAM_OP_CLEAR_DEFAULT) || cross_stage_move) {
    dealloc_needed = true;
  }

  if ((tcam_op == PIPE_TCAM_OP_ALLOCATE) || (tcam_op == PIPE_TCAM_OP_MODIFY) ||
      (tcam_op == PIPE_TCAM_OP_SET_DEFAULT) || cross_stage_move) {
    alloc_needed = true;
  }

  if (dealloc_needed) {
    rc = pipe_mgr_tcam_dealloc_actions(
        tcam_tbl,
        tcam_entry,
        unpack_mat_ent_data_as(tcam_entry->mat_data),
        cross_stage_move ? src_loc : dest_loc);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  if (alloc_needed) {
    rc = pipe_mgr_tcam_alloc_actions(tcam_tbl, tcam_entry, act_spec, dest_loc);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static inline enum pipe_mat_update_type tcam_op_to_move_node_op(
    pipe_tcam_op_e tcam_op, bool is_range) {
  switch (tcam_op) {
    case PIPE_TCAM_OP_ALLOCATE:
      if (is_range) {
        return PIPE_MAT_UPDATE_ADD_MULTI;
      } else {
        return PIPE_MAT_UPDATE_ADD;
      }
    case PIPE_TCAM_OP_MOVE:
      if (is_range) {
        return PIPE_MAT_UPDATE_MOV_MULTI;
      } else {
        return PIPE_MAT_UPDATE_MOV;
      }
    case PIPE_TCAM_OP_DELETE:
      return PIPE_MAT_UPDATE_DEL;
    case PIPE_TCAM_OP_MODIFY:
      return PIPE_MAT_UPDATE_MOD;
    case PIPE_TCAM_OP_SET_DEFAULT:
      return PIPE_MAT_UPDATE_SET_DFLT;
    case PIPE_TCAM_OP_CLEAR_DEFAULT:
      return PIPE_MAT_UPDATE_CLR_DFLT;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  return 0;
}

static void construct_entry_mat_data(tcam_hlp_entry_t *head_entry,
                                     pipe_tbl_match_spec_t *match_spec,
                                     pipe_act_fn_hdl_t act_fn_hdl,
                                     pipe_action_spec_t *act_spec,
                                     uint32_t ttl,
                                     bool new_stful_seq_nu) {
  if (!head_entry) {
    return;
  }

  uint32_t stful_seq_nu = head_entry->mat_data
                              ? unpack_mat_ent_data_stful(head_entry->mat_data)
                              : 0;
  if (new_stful_seq_nu) ++stful_seq_nu;
  head_entry->mat_data =
      make_mat_ent_data(match_spec,
                        act_spec,
                        act_fn_hdl,
                        ttl,
                        head_entry->selector_len,  // selection group len
                        stful_seq_nu,
                        0);
}

static pipe_mgr_move_list_t *pipe_mgr_tcam_construct_move_node(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *head_entry,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    bool new_stful_seq_nu) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  bool is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);
  enum pipe_mat_update_type move_op;

  move_op = tcam_op_to_move_node_op(tcam_op, is_range);

  pipe_mgr_move_list_t *move_node =
      alloc_move_list(NULL, move_op, get_tcam_pipe_tbl(tcam_tbl)->pipe_id);
  if (!move_node) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }

  move_node->entry_hdl = head_entry->entry_hdl;
  move_node->logical_sel_idx = head_entry->logical_sel_idx;
  move_node->selector_len = head_entry->selector_len;
  move_node->logical_action_idx = head_entry->logical_action_idx;

  if (tcam_tbl_info->restore_ent_node && head_entry->mat_data) {
    act_spec = unpack_mat_ent_data_as(head_entry->mat_data);
  }
  if (act_spec && IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    move_node->adt_ent_hdl = act_spec->adt_ent_hdl;
    move_node->adt_ent_hdl_valid = true;
  }

  if ((move_op == PIPE_MAT_UPDATE_ADD) || (move_op == PIPE_MAT_UPDATE_MOV)) {
    move_node->u.single.logical_idx = get_logical_index_from_index_ptn(
        get_tcam_pipe_tbl(tcam_tbl), head_entry->ptn_index, head_entry->index);
  }

  if ((move_op == PIPE_MAT_UPDATE_ADD_MULTI) ||
      (move_op == PIPE_MAT_UPDATE_MOV_MULTI)) {
    /* Fill up the array */
    tcam_hlp_entry_t *range_head;
    uint32_t count = 0;
    FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
      count++;
    }
    FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END();

    struct pipe_multi_index *locations;
    locations = (struct pipe_multi_index *)PIPE_MGR_CALLOC(
        count, sizeof(struct pipe_multi_index));
    if (locations == NULL) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return NULL;
    }

    FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
      locations[range_head->subentry_index].logical_index_base =
          get_logical_index_from_index_ptn(get_tcam_pipe_tbl(tcam_tbl),
                                           range_head->ptn_index,
                                           range_head->index);
      TCAM_HLP_GET_RANGE_ENTRY_COUNT(
          range_head,
          locations[range_head->subentry_index].logical_index_count);
    }
    FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END();

    move_node->u.multi.array_sz = count;
    move_node->u.multi.locations = locations;
  }

  if (move_op == PIPE_MAT_UPDATE_MOD) {
    move_node->old_data = head_entry->mat_data;
  }

  if ((move_op == PIPE_MAT_UPDATE_ADD) ||
      (move_op == PIPE_MAT_UPDATE_ADD_MULTI) ||
      (move_op == PIPE_MAT_UPDATE_SET_DFLT) ||
      (move_op == PIPE_MAT_UPDATE_MOD)) {
    if (tcam_tbl_info->restore_ent_node && head_entry->mat_data) {
      move_node->data = head_entry->mat_data;
    } else {
      construct_entry_mat_data(
          head_entry, match_spec, act_fn_hdl, act_spec, ttl, new_stful_seq_nu);
      move_node->data = head_entry->mat_data;
    }
  } else if ((move_op == PIPE_MAT_UPDATE_MOV) ||
             (move_op == PIPE_MAT_UPDATE_MOV_MULTI)) {
    move_node->data = head_entry->mat_data;
  } else {
    move_node->data = NULL;
  }

  if ((tcam_op == PIPE_TCAM_OP_DELETE) ||
      (tcam_op == PIPE_TCAM_OP_CLEAR_DEFAULT)) {
    move_node->data = head_entry->mat_data;
    head_entry->mat_data = NULL;
  }

  return move_node;
}

static pipe_status_t pipe_mgr_tcam_update_move_op(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_entry,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    bool new_stful_seq_nu,
    tcam_phy_loc_info_t *src_loc_p,
    pipe_mgr_move_list_t **move_tail_p,
    bool use_move_node) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc;
  pipe_mgr_move_list_t *move_node = move_tail_p ? *move_tail_p : NULL;
  tcam_phy_loc_info_t dest_loc;

  if (!tcam_entry) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, tcam_entry->index, tcam_entry->is_default, &dest_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  if (tcam_entry->subentry_index == 0) {
    if (!tcam_tbl_info->restore_ent_node) {
      if (!use_move_node) {
        rc = pipe_mgr_tcam_update_actions(
            tcam_tbl, tcam_entry, tcam_op, act_spec, src_loc_p, &dest_loc);
        if (rc != PIPE_SUCCESS) {
          return rc;
        }
      }
    } else {
      tcam_entry->logical_action_idx =
          cJSON_GetObjectItem(tcam_tbl_info->restore_ent_node, "action_idx")
              ->valuedouble;
      tcam_entry->logical_sel_idx =
          cJSON_GetObjectItem(tcam_tbl_info->restore_ent_node, "sel_idx")
              ->valuedouble;
      tcam_entry->selector_len =
          cJSON_GetObjectItem(tcam_tbl_info->restore_ent_node, "sel_len")
              ->valuedouble;
    }
  }
  tcam_hlp_entry_t *head_entry;
  TCAM_HLP_GET_TCAM_HEAD(tcam_entry, head_entry);
  if (!head_entry) return PIPE_UNEXPECTED;

  if (use_move_node && move_node) {
    PIPE_MGR_DBGCHK(head_entry->entry_hdl == move_node->entry_hdl);
    head_entry->logical_sel_idx = move_node->logical_sel_idx;
    head_entry->selector_len = move_node->selector_len;
    head_entry->logical_action_idx = move_node->logical_action_idx;
    construct_entry_mat_data(
        head_entry, match_spec, act_fn_hdl, act_spec, ttl, new_stful_seq_nu);
  } else if (move_tail_p) {
    TCAM_HLP_GET_TCAM_HEAD(tcam_entry, head_entry);

    move_node = pipe_mgr_tcam_construct_move_node(tcam_tbl,
                                                  head_entry,
                                                  tcam_op,
                                                  match_spec,
                                                  act_fn_hdl,
                                                  act_spec,
                                                  ttl,
                                                  new_stful_seq_nu);
    if (!move_node) return PIPE_NO_SYS_RESOURCES;
    if (*move_tail_p) {
      (*move_tail_p)->next = move_node;
    }
    *move_tail_p = move_node;
  } else {
    bool is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);
    enum pipe_mat_update_type move_op;
    move_op = tcam_op_to_move_node_op(tcam_op, is_range);
    if ((move_op == PIPE_MAT_UPDATE_ADD) ||
        (move_op == PIPE_MAT_UPDATE_ADD_MULTI) ||
        (move_op == PIPE_MAT_UPDATE_SET_DFLT) ||
        (move_op == PIPE_MAT_UPDATE_MOD)) {
      construct_entry_mat_data(
          head_entry, match_spec, act_fn_hdl, act_spec, ttl, new_stful_seq_nu);
    }
    if ((tcam_op == PIPE_TCAM_OP_DELETE) ||
        (tcam_op == PIPE_TCAM_OP_CLEAR_DEFAULT)) {
      if (!TCAM_SESS_IS_TXN(get_tcam_pipe_tbl(tcam_tbl))) {
        free_mat_ent_data(head_entry->mat_data);
      }
      head_entry->mat_data = NULL;
    }
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_update_version
 *        Update the version bits in the tcam entry in hardware
 *
 * This function updates the version fields in the tcam entry. Generates
 * appropriate instructions and calls driver interface APIs.
 * Called from 2 places specifically during atomic, transaction session
 *        - Delete entry during entry deletions
 *        - After the commit for each of the add entries
 *
 * \param tcam_tbl Pointer to the tcam table
 * \param tcam_entry Pointer to the tcam entry
 * \param version Version to update
 * \param disable Disable the versioning in the entry
 * \return pipe_status_t Status of the operation
 */
static pipe_status_t pipe_mgr_tcam_entry_update_version(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_entry,
    uint32_t version,
    bool disable) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;

  if (disable) {
  } else {
  }

  (void)version;

  tcam_phy_loc_info_t tcam_loc;
  rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, tcam_entry->index, tcam_entry->is_default, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for entry 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  // VK Add move op for atomic update
  return rc;
}

static uint32_t pipe_mgr_tcam_entry_priority_get(tcam_hlp_entry_t *tcam_entry) {
  return tcam_entry->priority;
}

pipe_status_t pipe_mgr_tcam_mark_index_inuse(tcam_tbl_t *tcam_tbl,
                                             uint32_t index) {
  bool old_val = bf_fbs_set(&tcam_tbl->hlp.tcam_used_bmp, index, 1);
  PIPE_MGR_DBGCHK(old_val == 0);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_mark_index_free(tcam_tbl_t *tcam_tbl,
                                            uint32_t index) {
  bool old_val = bf_fbs_set(&tcam_tbl->hlp.tcam_used_bmp, index, 0);
  PIPE_MGR_DBGCHK(old_val == 1);
  return PIPE_SUCCESS;
}

static bool pipe_mgr_tcam_index_is_free(tcam_tbl_t *tcam_tbl, uint32_t index) {
  return !bf_fbs_get(&tcam_tbl->hlp.tcam_used_bmp, index);
}

/* Find next free. index is inclusive . i.e if index is free,
 * it'll be returned
 */
static pipe_status_t pipe_mgr_tcam_find_next_free(tcam_tbl_t *tcam_tbl,
                                                  uint32_t index,
                                                  uint32_t no_entries,
                                                  uint32_t *free_index_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  int32_t free_i = 0;
  int32_t lookup_index_i = index;
  uint32_t start_block = 0, end_block = 0;

  if (index >= tcam_tbl->total_entries) {
    return PIPE_NO_SPACE;
  }

  PIPE_MGR_DBGCHK(lookup_index_i >= 0);
  do {
    free_i = bf_fbs_first_clr_contiguous(
        &tcam_tbl->hlp.tcam_used_bmp, lookup_index_i - 1, no_entries);
    if (free_i < 0) {
      return PIPE_NO_SPACE;
    }
    if (no_entries > 1) {
      rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, free_i, &start_block);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      rc = pipe_mgr_tcam_get_tcam_block(
          tcam_tbl, free_i + no_entries - 1, &end_block);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      lookup_index_i = FIRST_TCAM_ENTRY_OF_BLOCK(end_block);
    }
  } while ((no_entries > 1) && (start_block != end_block));
  PIPE_MGR_DBGCHK((free_i + no_entries) <= tcam_tbl->total_entries);

  *free_index_p = (uint32_t)free_i;
  return PIPE_SUCCESS;
}

/* Find prev free. index is exclusive . To find the last free
 * index use tcam_tbl->total_entries
 */
static pipe_status_t pipe_mgr_tcam_find_prev_free(tcam_tbl_t *tcam_tbl,
                                                  uint32_t index,
                                                  uint32_t no_entries,
                                                  uint32_t *free_index_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  PIPE_MGR_DBGCHK(index <= tcam_tbl->total_entries);
  int32_t free_i = 0;
  int32_t lookup_index_i = index;
  uint32_t start_block = 0, end_block = 0;

  PIPE_MGR_DBGCHK(lookup_index_i >= 0);
  do {
    free_i = bf_fbs_prev_clr_contiguous(
        &tcam_tbl->hlp.tcam_used_bmp, lookup_index_i, no_entries);
    if (free_i < 0) {
      return PIPE_NO_SPACE;
    }
    if (no_entries > 1) {
      rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, free_i, &start_block);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      rc = pipe_mgr_tcam_get_tcam_block(
          tcam_tbl, free_i + no_entries - 1, &end_block);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      lookup_index_i = FIRST_TCAM_ENTRY_OF_BLOCK(end_block);
    }
  } while ((no_entries > 1) && (start_block != end_block));
  PIPE_MGR_DBGCHK((free_i + no_entries) <= tcam_tbl->total_entries);

  *free_index_p = (uint32_t)free_i;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_move_helper(tcam_tbl_t *tcam_tbl,
                                               uint32_t dest_index,
                                               uint32_t src_index,
                                               tcam_hlp_entry_t **move_list_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *src_entry = NULL;
  src_entry = tcam_tbl->hlp.tcam_entries[src_index];
  if (src_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_tcam_entry_index_backup_one(tcam_tbl, dest_index);

  BF_LIST_DLL_REM(
      *(src_entry->range_list_p), src_entry, next_range, prev_range);
  if (move_list_p) {
    BF_LIST_DLL_AP(*move_list_p, src_entry, next_range, prev_range);
  }

  PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[dest_index] == NULL);

  rc = pipe_mgr_tcam_mark_index_inuse(tcam_tbl, dest_index);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

  tcam_tbl->hlp.tcam_entries[dest_index] = src_entry;
  src_entry->index = dest_index;

  rc |= pipe_mgr_tcam_mark_index_free(tcam_tbl, src_index);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  tcam_tbl->hlp.tcam_entries[src_index] = NULL;

  /* Add the tcam entry to the prio-sorted array */
  rc |= pipe_mgr_tcam_update_prio_array(
      tcam_tbl, src_entry->group, src_entry->priority, src_index, false);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

  rc |= pipe_mgr_tcam_update_prio_array(
      tcam_tbl, src_entry->group, src_entry->priority, dest_index, true);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

  return rc;
}

static pipe_status_t pipe_mgr_tcam_set_tcam_index_for_move(
    tcam_tbl_t *tcam_tbl,
    uint32_t dest_index,
    uint32_t src_index,
    pipe_mgr_move_list_t **move_tail_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL, *head_entry = NULL;
  uint32_t tail_index = 0, dest_tail_index;
  uint32_t move_start_index, move_end_index, dest_move_index;
  uint32_t move_index;

  tcam_entry = tcam_tbl->hlp.tcam_entries[src_index];
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_DBGCHK(TCAM_HLP_IS_RANGE_HEAD(tcam_entry));

  pipe_mgr_tcam_entry_backup_one(get_tcam_pipe_tbl(tcam_tbl),
                                 tcam_entry->entry_hdl);

  TCAM_HLP_GET_RANGE_TAIL_INDEX(tcam_entry, tail_index);

  dest_tail_index = dest_index + tail_index - src_index;

  tcam_phy_loc_info_t dest_loc, src_loc;
  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, dest_index, &dest_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        dest_index,
        rc);
    return rc;
  }
  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, src_index, &src_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        src_index,
        rc);
    return rc;
  }

  bool down_overlap = false, up_overlap = false;
  if ((dest_index > src_index) && (dest_index <= tail_index)) {
    /* Move down and overlap. move_end_index is non-inclusive */
    move_start_index = src_index + 1;
    move_end_index = dest_index + 1;
    dest_move_index = tail_index + 1;
    down_overlap = true;
  } else if ((dest_tail_index >= src_index) && (dest_tail_index < tail_index)) {
    /* Move up and overlap */
    move_start_index = dest_tail_index + 1;
    move_end_index = tail_index + 1;
    dest_move_index = dest_index + 1;
    up_overlap = true;
  } else {
    move_start_index = src_index;
    move_end_index = tail_index + 1;
    dest_move_index = dest_index;
  }

  tcam_hlp_entry_t *move_list = NULL;

  if (up_overlap) {
    /* Move the head entry first */
    rc = pipe_mgr_tcam_move_helper(tcam_tbl, dest_index, src_index, NULL);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }

  /* Move steps here and in tcam_hw.c need to be in sync */
  for (move_index = move_start_index; move_index < move_end_index;
       move_index++, dest_move_index++) {
    rc |= pipe_mgr_tcam_move_helper(
        tcam_tbl, dest_move_index, move_index, &move_list);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }

  if (down_overlap) {
    rc |= pipe_mgr_tcam_move_helper(tcam_tbl, dest_index, src_index, NULL);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }

  if (dest_index < src_index) {
    if (up_overlap) {
      BF_LIST_DLL_PP(move_list, tcam_entry, next_range, prev_range);
    }
    /* Move up */
    BF_LIST_DLL_CAT(
        move_list, *(tcam_entry->range_list_p), next_range, prev_range);
    *(tcam_entry->range_list_p) = move_list;
  } else {
    if (down_overlap) {
      BF_LIST_DLL_PP(
          *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
    }
    BF_LIST_DLL_CAT(
        *(tcam_entry->range_list_p), move_list, next_range, prev_range);
  }

  PIPE_MGR_DBGCHK(TCAM_HLP_IS_RANGE_HEAD(tcam_entry));
  if (!tcam_entry->is_default) {
    LOG_TRACE(
        "TCAM %s(0x%x - %d) pipe %d ptn %d "
        "Entry 0x%x with group %5d priority %5d moved from(stage-line) %5d "
        "(%2d-%5d) to %5d (%2d-%5d)",
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
        tcam_tbl->ptn_index,
        tcam_entry->entry_hdl,
        tcam_entry->group,
        tcam_entry->priority,
        src_index,
        src_loc.stage_id,
        src_loc.stage_line_no,
        dest_index,
        dest_loc.stage_id,
        dest_loc.stage_line_no);
  }

  TCAM_HLP_GET_TCAM_HEAD(tcam_entry, head_entry);
  if ((!head_entry) || (!head_entry->mat_data)) {
    return PIPE_UNEXPECTED;
  }
  rc |= pipe_mgr_tcam_update_move_op(
      tcam_tbl,
      tcam_entry,
      PIPE_TCAM_OP_MOVE,
      unpack_mat_ent_data_ms(head_entry->mat_data),
      unpack_mat_ent_data_afun_hdl(head_entry->mat_data),
      unpack_mat_ent_data_as(head_entry->mat_data),
      unpack_mat_ent_data_ttl(head_entry->mat_data),
      false,
      &src_loc,
      move_tail_p,
      false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating move operations for moving entry 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  return rc;
}

static pipe_status_t pipe_mgr_tcam_set_tcam_index(tcam_tbl_t *tcam_tbl,
                                                  uint32_t index,
                                                  tcam_hlp_entry_t *tcam_entry,
                                                  pipe_tcam_op_e tcam_op) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *src_tentry = NULL;
  uint32_t dest_index = index;
  uint32_t tcam_index = 0;
  uint32_t start_block = 0, end_block = 1;
  (void)tcam_index;

  tcam_phy_loc_info_t tcam_loc;
  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, index, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        index,
        rc);
    return rc;
  }

  pipe_mgr_tcam_get_tcam_block(tcam_tbl, index, &start_block);

  PIPE_MGR_DBGCHK((tcam_op == PIPE_TCAM_OP_ALLOCATE) ||
                  (tcam_op == PIPE_TCAM_OP_RESERVE_DEFAULT));

  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_DBGCHK(TCAM_HLP_IS_RANGE_HEAD(tcam_entry));
  FOR_ALL_TCAM_HLP_RANGE_ENTRIES_BLOCK_BEGIN(
      tcam_entry, src_tentry, tcam_index) {
    PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[dest_index] == NULL);

    pipe_mgr_tcam_entry_index_backup_one(tcam_tbl, dest_index);

    rc |= pipe_mgr_tcam_mark_index_inuse(tcam_tbl, dest_index);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

    tcam_tbl->hlp.tcam_entries[dest_index] = src_tentry;
    src_tentry->index = dest_index;

    /* Add the tcam entry to the prio-sorted array */
    rc |= pipe_mgr_tcam_update_prio_array(
        tcam_tbl, tcam_entry->group, tcam_entry->priority, dest_index, true);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &end_block);
    PIPE_MGR_DBGCHK(start_block == end_block);

    dest_index++;
  }
  FOR_ALL_TCAM_HLP_RANGE_ENTRIES_BLOCK_END()

  if (!tcam_entry->is_default) {
    LOG_TRACE(
        "TCAM %s(0x%x - %d) pipe %d ptn %d "
        "Entry 0x%x with group %5d priority %5d allocated at %5d (%2d-%5d)",
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
        tcam_tbl->ptn_index,
        tcam_entry->entry_hdl,
        tcam_entry->group,
        tcam_entry->priority,
        index,
        tcam_loc.stage_id,
        tcam_loc.stage_line_no);
  }

  return rc;
}

static bool pipe_mgr_tcam_is_prev_move_better(tcam_tbl_t *tcam_tbl,
                                              uint32_t start_index,
                                              uint32_t end_index,
                                              uint32_t up,
                                              uint32_t down) {
  uint32_t start_block = 0, end_block = 0;
  uint32_t start_stage = 0, end_stage = 0;
  uint32_t up_block = 0, down_block = 0;
  uint32_t up_stage = 0, down_stage = 0;
  uint32_t target_index = 0;

  /* Now pick a good candidate to move to out of either up/down */
  do {
    pipe_mgr_tcam_get_tcam_block(tcam_tbl, start_index, &start_block);
    start_stage = pipe_mgr_get_stage_id_for_block(tcam_tbl, start_block);
    pipe_mgr_tcam_get_tcam_block(tcam_tbl, end_index, &end_block);
    end_stage = pipe_mgr_get_stage_id_for_block(tcam_tbl, end_block);

    pipe_mgr_tcam_get_tcam_block(tcam_tbl, down, &down_block);
    down_stage = pipe_mgr_get_stage_id_for_block(tcam_tbl, down_block);
    pipe_mgr_tcam_get_tcam_block(tcam_tbl, up, &up_block);
    up_stage = pipe_mgr_get_stage_id_for_block(tcam_tbl, up_block);

    /* Target is picked based on below priority
     * - Same block
     * - Same stage - lesser block difference
     * - Same stage
     * - Across stage
     */

    if ((start_block == up_block) && (end_block == down_block)) {
      if ((start_index - up) < (down - end_index)) {
        target_index = up;
      } else {
        target_index = down;
      }
      break;
    }

    if (start_block == up_block) {
      target_index = up;
      break;
    } else if (end_block == down_block) {
      target_index = down;
      break;
    }

    if ((start_stage == down_stage) && (end_stage == up_stage)) {
      if ((start_block - up_block) < (down_block - end_block)) {
        target_index = up;
      } else {
        target_index = down;
      }
      break;
    }

    if (start_stage == up_stage) {
      target_index = up;
      break;
    } else if (end_stage == down_stage) {
      target_index = down;
      break;
    }

    /* Both the destinations are across stages. pick one */
    if ((start_stage - up_stage) < (end_stage - down_stage)) {
      target_index = up;
      break;
    } else {
      target_index = down;
      break;
    }

  } while (0);

  if (target_index == up) {
    return false;
  } else {
    return true;
  }
  return false;
}

static int compare_move_indexes(const void *a, const void *b) {
  const uint32_t p1 = *(uint32_t *)a, p2 = *(uint32_t *)b;

  if (p1 > p2) {
    return 1;
  } else if (p1 < p2) {
    return -1;
  }

  return 0;
}

/* Make space for the new TCAM entries by moving the existing entries
 * within the group
 *
 * We need space either at the start_index (i.e. current entry at start_index
 * should be displaced) or we need space at the end_index (i.e. current entry
 *at
 * end_index should be displaced)
 */
static pipe_status_t pipe_mgr_tcam_entry_make_space_within_group(
    tcam_tbl_t *tcam_tbl,
    uint16_t group,
    uint32_t start_index,
    uint32_t end_index,
    uint32_t free_spots_needed,
    uint32_t *free,
    pipe_mgr_move_list_t **move_tail_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i;
  uint32_t entries_per_block = 1;
  uint32_t move_cnt = 0;

  /* These many entries need to be free */
  uint32_t prev_free_index = 0, next_free_index = 0;
  uint32_t prev_free[free_spots_needed], next_free[free_spots_needed];
  uint32_t down_free_count = 0, up_free_count = 0;

  uint32_t lookup_index;

  PIPE_MGR_DBGCHK(entries_per_block == 1);

  lookup_index = start_index;
  for (up_free_count = 0; up_free_count < free_spots_needed; up_free_count++) {
    rc = pipe_mgr_tcam_find_prev_free(
        tcam_tbl, lookup_index, entries_per_block, &prev_free_index);
    if (rc != PIPE_SUCCESS) {
      break;
    }

    prev_free[up_free_count] = prev_free_index;
    lookup_index = prev_free_index;
  }

  lookup_index = end_index;
  for (down_free_count = 0; down_free_count < free_spots_needed;
       down_free_count++) {
    rc = pipe_mgr_tcam_find_next_free(
        tcam_tbl, lookup_index, entries_per_block, &next_free_index);
    if (rc != PIPE_SUCCESS) {
      break;
    }
    PIPE_MGR_DBGCHK(next_free_index < tcam_tbl->total_entries);

    next_free[down_free_count] = next_free_index;
    lookup_index = next_free_index + 1;
  }

  if ((up_free_count + down_free_count) < free_spots_needed) {
    return PIPE_NO_SPACE;
  }

  uint32_t p = 0, n = 0;
  uint32_t move_target[free_spots_needed];
  for (i = 0; i < free_spots_needed; i++) {
    /* Pick the best candidates from prev_free and next_free */
    if (p >= up_free_count) {
      move_target[i] = next_free[n];
      n++;
    } else if (n >= down_free_count) {
      move_target[i] = prev_free[p];
      p++;
    } else if (pipe_mgr_tcam_is_prev_move_better(tcam_tbl,
                                                 start_index,
                                                 end_index,
                                                 prev_free[p],
                                                 next_free[n])) {
      move_target[i] = prev_free[p];
      p++;
    } else {
      move_target[i] = next_free[n];
      n++;
    }
  }

  uint32_t replace_priority = 0;
  uint32_t replace_index = 0;
  while (true) {
    /* Sort the move_target based on the indexes */
    qsort(
        move_target, free_spots_needed, sizeof(uint32_t), compare_move_indexes);

    if (move_target[0] >= start_index) {
      break;
    }
    /* Move up an entry to move_target[0] */

    uint32_t free_index = move_target[0];

    /* In this group, find out the next priority set */
    replace_priority =
        pipe_mgr_tcam_get_next_priority_of_group(tcam_tbl, group, free_index);
    bool get_range = pipe_mgr_tcam_get_prio_range(
        tcam_tbl, group, replace_priority, NULL, &replace_index);
    PIPE_MGR_DBGCHK(get_range == true);

    if (replace_index > start_index) {
      break;
    }

    rc = pipe_mgr_tcam_set_tcam_index_for_move(
        tcam_tbl, free_index, replace_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error moving tcam entry from %d to %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          replace_index,
          free_index,
          rc);
      break;
    }

    /* Move the entry from replace_index to move_target[0] */
    move_target[0] = replace_index;
    move_cnt++;
  }

  while (true) {
    /* Sort the move_target based on the indexes */
    qsort(
        move_target, free_spots_needed, sizeof(uint32_t), compare_move_indexes);

    if (move_target[free_spots_needed - 1] <= end_index) {
      break;
    }

    /* Move down an entry to move_target[free_spots_needed - 1] */
    uint32_t free_index = move_target[free_spots_needed - 1];

    /* In this group, find out the prev priority set */
    replace_priority =
        pipe_mgr_tcam_get_prev_priority_of_group(tcam_tbl, group, free_index);
    bool get_range = pipe_mgr_tcam_get_prio_range(
        tcam_tbl, group, replace_priority, &replace_index, NULL);
    PIPE_MGR_DBGCHK(get_range == true);

    if (replace_index < end_index) {
      break;
    }

    rc = pipe_mgr_tcam_set_tcam_index_for_move(
        tcam_tbl, free_index, replace_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error moving tcam entry from %d to %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          replace_index,
          free_index,
          rc);
      break;
    }

    /* Move the entry from replace_index to move_target[free_spots_needed - 1]
     */
    move_target[free_spots_needed - 1] = replace_index;
    move_cnt++;
  }

  /* Make sure that all entries in the move_target are empty */
  /* Save the free indices */
  for (i = 0; i < free_spots_needed; i++) {
    PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[move_target[i]] == NULL);
    free[i] = move_target[i];
  }

  if (move_cnt > largest_move_cnt) {
    largest_move_cnt = move_cnt;
  }
  total_move_cnt += move_cnt;

  return PIPE_SUCCESS;
}

static bool fit_entry_down(tcam_tbl_t *tcam_tbl,
                           uint32_t head_index,
                           uint32_t *tail_index,
                           uint32_t count) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t dest_index = head_index;
  uint32_t dest_tail_index = dest_index + count - 1;

  if ((dest_tail_index + 1) >= tcam_tbl->total_entries) {
    return true;
  }
  uint32_t dest_block, dest_tail_block;

  rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &dest_block);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  rc =
      pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_tail_index, &dest_tail_block);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  if (dest_block != dest_tail_block) {
    dest_index = FIRST_TCAM_ENTRY_OF_BLOCK(dest_tail_block);
  }
  dest_tail_index = dest_index + count - 1;

  if ((dest_tail_index + 1) >= tcam_tbl->total_entries) {
    return true;
  }

  *tail_index = dest_tail_index;
  return false;
}

/*
 * For moving down, this function calculates how many entries need to be moved
 * and the final destination index.
 *
 * It is the exact opposite of pipe_mgr_tcam_count_blocks_to_compress_move_up()
 * See comments there.
 */
static bool pipe_mgr_tcam_count_blocks_to_compress_move_down(
    tcam_tbl_t *tcam_tbl,
    uint32_t start_index,
    uint32_t entries_per_block,
    uint32_t no_blocks,
    uint32_t *count_entries_to_move,
    uint32_t *bottom_most_index) {
  uint32_t rem_entries = entries_per_block * no_blocks;
  bool full = false;
  uint32_t move_count = 0;
  uint32_t dest_index, dest_tail_index = 0;
  uint32_t src_index;

  dest_index = start_index;
  /* rem_entries number of entries need to be placed above start-index */
  while (rem_entries && (dest_index < tcam_tbl->total_entries)) {
    full = fit_entry_down(
        tcam_tbl, dest_index, &dest_tail_index, entries_per_block);
    if (full) {
      break;
    }

    rem_entries -= entries_per_block;
    dest_index = dest_tail_index + 1;
  }

  if (rem_entries || full) {
    return true;
  }

  src_index = start_index;
  while (true) {
    PIPE_MGR_DBGCHK(dest_index <= tcam_tbl->total_entries);
    for (; src_index < dest_index; src_index++) {
      /* Just looping is probably ok, because if we've come here
       * the tcam is pretty full!
       */
      if (tcam_tbl->hlp.tcam_entries[src_index]) {
        break;
      }
    }

    if (src_index == dest_index) {
      break;
    }

    if (dest_index == tcam_tbl->total_entries) {
      /* We still have entries to move, but no space */
      full = true;
      break;
    }

    PIPE_MGR_DBGCHK(src_index < tcam_tbl->total_entries);
    tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[src_index];
    uint32_t range_count;
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);

    full = fit_entry_down(tcam_tbl, dest_index, &dest_tail_index, range_count);
    if (full) {
      break;
    }

    move_count++;

    dest_index = dest_tail_index + 1;
    src_index += range_count;
  }

  *bottom_most_index = dest_tail_index;
  *count_entries_to_move = move_count;
  return full;
}

/*
 * Move the entries from stop_index (inclusive) and below
 * to bottom_free_index and above.
 *
 * See comments at pipe_mgr_tcam_entry_compress_up() for more info
 */
static pipe_status_t pipe_mgr_tcam_entry_compress_down(
    tcam_tbl_t *tcam_tbl,
    uint32_t bottom_free_index,
    uint32_t stop_index,
    uint32_t *free_index_p,
    pipe_mgr_move_list_t **move_tail_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t dest_index = bottom_free_index;
  uint32_t src_index = bottom_free_index;

  while (true) {
    for (; src_index > stop_index; src_index--) {
      /* Just looping is probably ok, because if we've come here
       * the tcam is pretty full!
       */
      if (tcam_tbl->hlp.tcam_entries[src_index]) {
        break;
      }
      if (src_index == 0) {
        break;
      }
    }

    if (src_index < stop_index) {
      break;
    }

    tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[src_index];
    if (!tcam_entry) {
      return PIPE_UNEXPECTED;
    }
    TCAM_HLP_GET_RANGE_HEAD(tcam_entry, tcam_entry);
    uint32_t range_count;
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);

    PIPE_MGR_DBGCHK((src_index + 1) >= range_count);
    PIPE_MGR_DBGCHK((dest_index + 1) >= range_count);
    src_index = (src_index + 1) - range_count;
    dest_index = (dest_index + 1) - range_count;

    /* Update the dest-index such that all the entries will be in the same
     * tcam
     */

    uint32_t dest_block, dest_tail_block;

    rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &dest_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = pipe_mgr_tcam_get_tcam_block(
        tcam_tbl, dest_index + range_count - 1, &dest_tail_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    if (dest_block != dest_tail_block) {
      dest_index = LAST_TCAM_ENTRY_OF_BLOCK(dest_block) - range_count + 1;
    }

    rc = pipe_mgr_tcam_set_tcam_index_for_move(
        tcam_tbl, dest_index, src_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error moving tcam entry from %d to %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          dest_index,
          src_index,
          rc);
      return rc;
    }

    dest_index--;
    if (src_index == 0) {
      break;
    }
    src_index--;
  }
  *free_index_p = stop_index;
  return PIPE_SUCCESS;
}

static bool fit_entry_up(tcam_tbl_t *tcam_tbl,
                         uint32_t tail_index,
                         uint32_t *head_index,
                         uint32_t count) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t dest_tail_index = tail_index;
  uint32_t dest_index;

  if ((dest_tail_index + 1) < count) {
    return true;
  }
  dest_index = (dest_tail_index + 1) - count;
  uint32_t dest_block, dest_tail_block;

  rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &dest_block);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  rc =
      pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_tail_index, &dest_tail_block);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  if (dest_block != dest_tail_block) {
    dest_tail_index = LAST_TCAM_ENTRY_OF_BLOCK(dest_block);
  }

  if ((dest_tail_index + 1) < count) {
    return true;
  }
  dest_index = (dest_tail_index + 1) - count;

  *head_index = dest_index;
  return false;
}

/*
 * For moving up, this function calculates how many entries need to be moved
 * and the final destination index.
 *
 * It operates by doing the below:
 * We need to install entries_per_block * no_blocks number of entries
 * just above start_index (including the entry at start_index). So
 * we try to fit these many entries here (taking care of the constraint that
 * entries of same range should be in the same block).
 * Once these many entries are fit, we try to move (not real move) entries
 * at start_index and above one by one.
 * The exit condition is if we figure out no more moves are possible due to
 * up being full OR when the src for the move is above the dest (indicates a
 * move down in that case).
 *
 * At the end of this, the top_most_index is the top most index at which
 * a set of entries will be moved to (it implies that this index MUST be
 * free - there are asserts later on)
 */
static bool pipe_mgr_tcam_count_blocks_to_compress_move_up(
    tcam_tbl_t *tcam_tbl,
    uint32_t start_index,
    uint32_t entries_per_block,
    uint32_t no_blocks,
    uint32_t *count_entries_to_move,
    uint32_t *top_most_index) {
  uint32_t rem_entries = entries_per_block * no_blocks;
  bool full = false;
  uint32_t move_count = 0;
  int32_t dest_index = 0, dest_tail_index;
  int32_t src_index;

  dest_tail_index = (int32_t)start_index;
  /* rem_entries number of entries need to be placed above start-index */
  while (rem_entries && (dest_tail_index >= 0)) {
    full = fit_entry_up(tcam_tbl,
                        (uint32_t)dest_tail_index,
                        (uint32_t *)&dest_index,
                        entries_per_block);
    if (full) {
      break;
    }

    rem_entries -= entries_per_block;

    dest_tail_index = dest_index - 1;
  }

  if (rem_entries || full) {
    return true;
  }

  src_index = (int32_t)start_index;
  while (true) {
    for (; src_index > dest_tail_index; src_index--) {
      /* Just looping is probably ok, because if we've come here
       * the tcam is pretty full!
       */
      PIPE_MGR_DBGCHK(src_index >= 0);
      if (tcam_tbl->hlp.tcam_entries[src_index]) {
        break;
      }
    }

    if (src_index == dest_tail_index) {
      break;
    }

    if (dest_tail_index < 0) {
      /* We still have entries to move, but no space */
      full = true;
      break;
    }

    PIPE_MGR_DBGCHK(src_index >= 0);
    tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[src_index];
    uint32_t range_count;
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);

    full = fit_entry_up(tcam_tbl,
                        (uint32_t)dest_tail_index,
                        (uint32_t *)&dest_index,
                        range_count);
    if (full) {
      break;
    }

    move_count++;

    PIPE_MGR_DBGCHK(src_index >= (int32_t)range_count);
    src_index -= range_count;
    dest_tail_index = dest_index - 1;
  }

  *top_most_index = dest_index;
  *count_entries_to_move = move_count;
  return full;
}

/*
 * Move the entries from stop_index (inclusive) and above
 * to top_free_index and below.
 * This top_free_index would've been calculated by
 * pipe_mgr_tcam_count_blocks_to_compress_move_up() .
 * The logic in both these functions should be in sync
 * Basically pipe_mgr_tcam_count_blocks_to_compress_move_up()
 * computes the destination and does pseudo moves. This function actually
 * performs the moves
 */
static pipe_status_t pipe_mgr_tcam_entry_compress_up(
    tcam_tbl_t *tcam_tbl,
    uint32_t top_free_index,
    uint32_t stop_index,
    uint32_t *free_index_p,
    pipe_mgr_move_list_t **move_tail_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t dest_index = top_free_index;
  uint32_t src_index = top_free_index;

  PIPE_MGR_DBGCHK(stop_index < tcam_tbl->total_entries);
  while (true) {
    for (; src_index <= stop_index; src_index++) {
      /* Just looping is probably ok, because if we've come here
       * the tcam is pretty full!
       */
      if (tcam_tbl->hlp.tcam_entries[src_index]) {
        break;
      }
    }

    if (src_index > stop_index) {
      break;
    }

    tcam_hlp_entry_t *tcam_entry = tcam_tbl->hlp.tcam_entries[src_index];
    uint32_t range_count;
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);

    /* Update the dest-index such that all the entries will be in the same
     * tcam
     */

    uint32_t dest_block, dest_tail_block;

    rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &dest_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = pipe_mgr_tcam_get_tcam_block(
        tcam_tbl, dest_index + range_count - 1, &dest_tail_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    if (dest_block != dest_tail_block) {
      dest_index = FIRST_TCAM_ENTRY_OF_BLOCK(dest_tail_block);
    }

    rc = pipe_mgr_tcam_set_tcam_index_for_move(
        tcam_tbl, dest_index, src_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error moving tcam entry from %d to %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          dest_index,
          src_index,
          rc);
      return rc;
    }

    dest_index += range_count;
    src_index += range_count;
  }
  *free_index_p = dest_index;
  return PIPE_SUCCESS;
}

/*
 * Make contiguous space to install range entries.
 *
 * Do the below:
 * Check the cost associated with moving entries up and down.
 * Choose the best one and move
 */
static pipe_status_t pipe_mgr_tcam_entry_make_contiguous_space(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *head_entry,
    bool start_index_valid,
    uint32_t start_index,
    bool end_index_valid,
    uint32_t end_index,
    uint32_t entries_per_spot,
    uint32_t free_spots_needed,
    uint32_t *free,
    pipe_mgr_move_list_t **move_tail_p) {
  uint32_t move_cnt = 0;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  bool up_full = true, down_full = true;
  uint32_t up_entries_to_move = 0, down_entries_to_move = 0;
  uint32_t top_most_index = 0, bottom_most_index = 0;
  uint32_t free_index = 0;

  if (start_index_valid) {
    up_full =
        pipe_mgr_tcam_count_blocks_to_compress_move_up(tcam_tbl,
                                                       start_index,
                                                       entries_per_spot,
                                                       free_spots_needed,
                                                       &up_entries_to_move,
                                                       &top_most_index);
  } else {
    up_full = true;
  }

  if (end_index_valid) {
    down_full =
        pipe_mgr_tcam_count_blocks_to_compress_move_down(tcam_tbl,
                                                         end_index,
                                                         entries_per_spot,
                                                         free_spots_needed,
                                                         &down_entries_to_move,
                                                         &bottom_most_index);
  } else {
    down_full = true;
  }

  if (up_full && down_full) {
    return PIPE_NO_SPACE;
  }

  /* Decide to compress up/down */
  bool move_up = false;
  if (up_full) {
    move_up = false;
  } else if (down_full) {
    move_up = true;
  } else {
    if (up_entries_to_move < down_entries_to_move) {
      move_up = true;
    } else {
      move_up = false;
    }
  }

  if (move_up) {
    rc = pipe_mgr_tcam_entry_compress_up(
        tcam_tbl, top_most_index, start_index, &free_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error compressing tcam entries from %d to %d "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          top_most_index,
          start_index,
          rc);
      return rc;
    }
  } else {
    rc = pipe_mgr_tcam_entry_compress_down(
        tcam_tbl, bottom_most_index, end_index, &free_index, move_tail_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error compressing tcam entries from %d to %d "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          end_index,
          bottom_most_index,
          rc);
      return rc;
    }
  }

  uint32_t dest_index = free_index;
  uint32_t i = 0;
  tcam_hlp_entry_t *tcam_entry = head_entry;
  for (i = 0; i < free_spots_needed; i++, tcam_entry = tcam_entry->next) {
    uint32_t range_count;
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);
    uint32_t dest_block, dest_tail_block;

    rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, dest_index, &dest_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = pipe_mgr_tcam_get_tcam_block(
        tcam_tbl, dest_index + range_count - 1, &dest_tail_block);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    if (dest_block != dest_tail_block) {
      dest_index = FIRST_TCAM_ENTRY_OF_BLOCK(dest_tail_block);
    }
    free[i] = dest_index;
    dest_index += range_count;
  }

  if (move_cnt > largest_move_cnt) {
    largest_move_cnt = move_cnt;
  }
  total_move_cnt += move_cnt;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_find_free_slot(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *head_tcam_entry,
    uint16_t group,
    uint32_t priority,
    uint32_t no_blocks,
    uint32_t entries_per_block,
    uint32_t *free,
    pipe_mgr_move_list_t **move_tail_p,
    bool use_move_node) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_mgr_move_list_t *move_node = move_tail_p ? *move_tail_p : NULL;
  tcam_hlp_entry_t *tcam_entry = head_tcam_entry;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t start_index = 0, end = 0;
  uint32_t prev_prio_start = 0, prev_prio_end = 0, next_prio_start = 0;
  bool prev_prio_exists = false, next_prio_exists = false;
  uint32_t free_count = 0;
  uint32_t i = 0;

  if (!tcam_tbl_info->restore_ent_node) {
    /* If discrete indices are needed, then just search for required
     * number of discrete indices. And move entries within the group.
     *
     * If contiguous entries are needed, then search for required number of
     * contiguous entries within the given priority range.
     * If a contiguous range is not available, then search for required number
     * of discrete entries in both directions, calculate the cost to move
     * in either direction and choose the best one
     */
    if (!pipe_mgr_tcam_get_prev_prio_end(
            tcam_tbl, group, priority, &prev_prio_end)) {
      prev_prio_exists = false;
      start_index = 0;
    } else {
      prev_prio_exists = true;
      start_index = prev_prio_end;
    }

    if (!pipe_mgr_tcam_get_next_prio_start(
            tcam_tbl, group, priority, &next_prio_start)) {
      next_prio_exists = false;
      end = tcam_tbl->total_entries;
    } else {
      next_prio_exists = true;
      end = next_prio_start;
    }

    if (prev_prio_exists) {
      PIPE_MGR_DBGCHK(end > start_index);
      pipe_mgr_tcam_get_prev_prio_start(
          tcam_tbl, group, priority, &prev_prio_start);
      if (((prev_prio_start + PIPE_MGR_TCAM_ENTRY_BUFFER_SPACE) < end) &&
          ((prev_prio_start + PIPE_MGR_TCAM_ENTRY_BUFFER_SPACE) >
           start_index)) {
        start_index = prev_prio_start + PIPE_MGR_TCAM_ENTRY_BUFFER_SPACE;
      }
    }

    /* Search a free spot within the start_index (inclusive) and end
     * (excluded) range
     * If a location doesn't exist, we need to move tcam entries
     */
    uint32_t free_index = 0;
    uint32_t lookup_index;

    if (use_move_node == false || !move_node) {
      lookup_index = start_index;
      for (free_count = 0; free_count < no_blocks; free_count++) {
        rc = pipe_mgr_tcam_find_next_free(
            tcam_tbl, lookup_index, entries_per_block, &free_index);
        if ((rc != PIPE_SUCCESS) || (free_index >= end)) {
          break;
        }

        free[free_count] = free_index;
        lookup_index = free_index + entries_per_block;
      }

      /* Check for free space between prev_prio_end and start_index */
      lookup_index = start_index;
      for (; prev_prio_exists && free_count < no_blocks; free_count++) {
        rc = pipe_mgr_tcam_find_prev_free(
            tcam_tbl, lookup_index, entries_per_block, &free_index);
        if ((rc != PIPE_SUCCESS) || (free_index <= prev_prio_end)) {
          break;
        }
        free[free_count] = free_index;
        lookup_index = free_index;
      }
    } else {
      for (free_count = 0; free_count < no_blocks; free_count++) {
        if (move_node->op == PIPE_MAT_UPDATE_ADD) {
          free_index = get_index_from_logical_index(
              get_tcam_pipe_tbl(tcam_tbl), move_node->u.single.logical_idx);
        } else if (move_node->op == PIPE_MAT_UPDATE_ADD_MULTI) {
          free_index = get_index_from_logical_index(
              get_tcam_pipe_tbl(tcam_tbl),
              move_node->u.multi.locations[free_count].logical_index_base);
        }
        free[free_count] = free_index;
      }
    }
  } else {
    cJSON *ent_node = tcam_tbl_info->restore_ent_node;
    if (TCAM_TBL_USES_RANGE(tcam_tbl_info)) {
      cJSON *ranges = cJSON_GetObjectItem(ent_node, "range_heads");
      cJSON *range;
      for (range = ranges->child, free_count = 0; range;
           range = range->next, free_count++) {
        free[free_count] = cJSON_GetObjectItem(range, "entry_idx")->valuedouble;
      }
      PIPE_MGR_DBGCHK(free_count == no_blocks);
    } else {
      free[0] = cJSON_GetObjectItem(ent_node, "entry_idx")->valuedouble;
      free_count = 1;
    }
  }

  tcam_entry = head_tcam_entry;
  if (free_count < no_blocks) {
    start_index = prev_prio_exists ? prev_prio_end : 0;
    for (i = 0; i < free_count; i++) {
      tcam_entry = tcam_entry->next;
    }
    if (!TCAM_TBL_USES_RANGE(tcam_tbl_info)) {
      /* Simple move within the group */
      rc = pipe_mgr_tcam_entry_make_space_within_group(tcam_tbl,
                                                       group,
                                                       start_index,
                                                       end,
                                                       no_blocks - free_count,
                                                       &(free[free_count]),
                                                       move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x-%d) "
            "Could not make space within group for entry add rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id,
            rc);
        return rc;
      }
    } else {
      /* Moves to install contiguous entries */
      rc = pipe_mgr_tcam_entry_make_contiguous_space(tcam_tbl,
                                                     tcam_entry,
                                                     prev_prio_exists,
                                                     start_index,
                                                     next_prio_exists,
                                                     end,
                                                     entries_per_block,
                                                     no_blocks - free_count,
                                                     &(free[free_count]),
                                                     move_tail_p);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x-%d) "
            "Could not make space within group for entry add rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id,
            rc);
        return rc;
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_place_tcam_entry(
    tcam_tbl_t *tcam_tbl,
    uint16_t group,
    uint32_t priority,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    tcam_hlp_entry_t **head_tcam_entry_p,
    tcam_hlp_entry_t *orig_tcam_entry,
    pipe_mgr_move_list_t **move_tail_p,
    bool use_move_node) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_mgr_move_list_t *move_node = move_tail_p ? *move_tail_p : NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  uint32_t entries_per_block = 1;
  uint32_t no_blocks = 1;
  uint32_t i, j;

  tcam_hlp_entry_t *tcam_entry = NULL;

  rc = pipe_mgr_tcam_verify_resources(tcam_tbl, act_spec);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  rc = pipe_mgr_entry_format_tof_count_range_expand(tcam_tbl_info->dev_id,
                                                    tcam_tbl_info->profile_id,
                                                    tcam_tbl_info->tbl_hdl,
                                                    match_spec,
                                                    &no_blocks,
                                                    &entries_per_block);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error expanding the TCAM range rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  } else if (no_blocks > 1 || entries_per_block > 1) {
    LOG_TRACE(
        "%s:%d %s(0x%x-%d) range-entry %d expands to %d blocks of %d entries",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        orig_tcam_entry->entry_hdl,
        no_blocks,
        entries_per_block);
  }
  uint32_t free[no_blocks];

  if (use_move_node && move_node) {
    if (move_node->op == PIPE_MAT_UPDATE_ADD) {
      PIPE_MGR_DBGCHK(no_blocks == 1);
      PIPE_MGR_DBGCHK(entries_per_block == 1);
    } else if (move_node->op == PIPE_MAT_UPDATE_ADD_MULTI) {
      PIPE_MGR_DBGCHK(no_blocks == move_node->u.multi.array_sz);
      PIPE_MGR_DBGCHK(entries_per_block ==
                      move_node->u.multi.locations[0].logical_index_count);
    }
  }

  orig_tcam_entry->range_count = entries_per_block;

  uint32_t subentry_index = 0;
  /* Create duplicate TCAM entries */
  for (i = 0; i < no_blocks; i++, subentry_index++) {
    tcam_entry = pipe_mgr_tcam_entry_alloc();
    if (tcam_entry == NULL) {
      LOG_ERROR("%s:%d alloc tcam entry failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    pipe_mgr_tcam_entry_deep_copy_v2(tcam_entry, orig_tcam_entry);
    tcam_entry->range_list_p = &tcam_entry->range_list;

    /* The subentry-index of the range-heads should be contiguous.
     * Otherwise pipe_mgr_tcam_entry_get_location() fails
     */
    tcam_entry->subentry_index = subentry_index;

    BF_LIST_DLL_AP(
        *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
    for (j = 1; j < entries_per_block; j++) {
      tcam_hlp_entry_t *range_entry = pipe_mgr_tcam_entry_alloc();
      if (range_entry == NULL) {
        LOG_ERROR("%s:%d alloc range entry failed", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }

      pipe_mgr_tcam_entry_deep_copy_v2(range_entry, orig_tcam_entry);
      range_entry->range_list_p = tcam_entry->range_list_p;
      range_entry->subentry_index =
          no_blocks + (j - 1) + (subentry_index * (entries_per_block - 1));
      BF_LIST_DLL_AP(
          *(tcam_entry->range_list_p), range_entry, next_range, prev_range);
    }
    BF_LIST_DLL_AP(*head_tcam_entry_p, tcam_entry, next, prev);
  }

  rc = pipe_mgr_tcam_find_free_slot(tcam_tbl,
                                    *head_tcam_entry_p,
                                    group,
                                    priority,
                                    no_blocks,
                                    entries_per_block,
                                    free,
                                    move_tail_p,
                                    use_move_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error finding free space for new entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        (tcam_entry ? tcam_entry->entry_hdl : 0),
        rc);
    return rc;
  }

  tcam_entry = *head_tcam_entry_p;
  for (i = 0; i < no_blocks; i++, tcam_entry = tcam_entry->next) {
    PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[free[i]] == NULL);
    rc = pipe_mgr_tcam_set_tcam_index(
        tcam_tbl, free[i], tcam_entry, PIPE_TCAM_OP_ALLOCATE);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error allocating index %d to tcam entry handle 0x%x "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          free[i],
          tcam_entry->entry_hdl,
          rc);
      return rc;
    }
  }

  tcam_entry = *head_tcam_entry_p;
  rc = pipe_mgr_tcam_update_move_op(tcam_tbl,
                                    tcam_entry,
                                    PIPE_TCAM_OP_ALLOCATE,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ttl,
                                    false,
                                    NULL,
                                    move_tail_p,
                                    use_move_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating move operations for new entry 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  return rc;
}

pipe_status_t pipe_mgr_tcam_entry_add_internal(
    tcam_tbl_t *tcam_tbl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_tail_p,
    bool use_move_node) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_hlp_entry_t *head_tcam_entry = NULL;
  uint32_t priority = 0;
  uint32_t group = 0;
  bool group_created = false;

  /*
   * Follow the below steps:
   * - Create an entry structure
   * - Check in the tcam_prio_tree where it should go to
   * - Check for a free position within the tcam_entries array
   * - If a free pos is not found, call functions to move entries
   * - Once a position is found, update the tcam_prio_tree, tcam_entries and
   *   tcam_entries_htbl
   * - Call meters and other APIs to program
   * - Generate Instructions
   */

  /* Backup the following here:
   * - prio-tree node for the given priority
   */

  get_group_and_priority(match_spec, &group, &priority);

  tcam_hlp_entry_t tcam_entry_s;
  tcam_hlp_entry_t *tcam_entry = &tcam_entry_s;
  PIPE_MGR_MEMSET(tcam_entry, 0, sizeof(tcam_hlp_entry_t));

  tcam_entry->entry_hdl = ent_hdl;
  tcam_entry->priority = priority;
  tcam_entry->group = group;
  tcam_entry->ptn_index = tcam_tbl->ptn_index;
  tcam_entry->range_list_p = &tcam_entry->range_list;
  BF_LIST_DLL_AP(
      *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);

  if (group >= tcam_tbl->hlp.max_tcam_group ||
      tcam_tbl->hlp.group_info[group] == NULL) {
    group_created = true;
    pipe_mgr_tcam_create_group(tcam_tbl, group);
  }

  /* Place the TCAM entry */
  rc = pipe_mgr_tcam_place_tcam_entry(tcam_tbl,
                                      group,
                                      priority,
                                      match_spec,
                                      act_fn_hdl,
                                      act_spec,
                                      ttl,
                                      &head_tcam_entry,
                                      tcam_entry,
                                      move_tail_p,
                                      use_move_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error placing tcam entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        ent_hdl,
        rc);
    goto cleanup;
  }

  total_entry_add_cnt++;

  tcam_tbl->hlp.total_usage++;

  pipe_mgr_tcam_entry_htbl_add_delete(tcam_tbl, ent_hdl, head_tcam_entry, true);

  if (TCAM_SESS_IS_ATOMIC(get_tcam_pipe_tbl(tcam_tbl))) {
    BF_LIST_DLL_AP(tcam_tbl->hlp.entry_add_list,
                   head_tcam_entry,
                   next_atomic,
                   prev_atomic);
  }

  return PIPE_SUCCESS;

cleanup:
  if (group_created) {
    pipe_mgr_tcam_delete_group(tcam_tbl, group);
  }
  pipe_mgr_tcam_hlp_entry_destroy_all(head_tcam_entry);
  return rc;
}

/** \brief pipe_mgr_tcam_check_free_space
 *        Check if some free space exists in a tcam table
 *
 * This function checks if free space exists in the tcam table to
 * insert a bunch of entries. The check is very simple. Sophisticated check
 * might be needed in the future.
 *
 * \param tcam_tbl Tcam table pointer
 * \param count Number of free space to check for
 * \return bool true if free space is found, false otherwise
 */
static bool pipe_mgr_tcam_check_free_space(tcam_tbl_info_t *tcam_tbl_info,
                                           tcam_tbl_t *tcam_tbl,
                                           uint32_t count) {
  bool free_space = true;
  if (tcam_tbl_info->reserve_modify_ent && !tcam_tbl_info->uses_alpm) {
    /* Reserve an entry in the partition for atomic modify */
    free_space =
        ((tcam_tbl->hlp.total_usage + count) <= tcam_tbl->total_entries - 1);
  } else {
    free_space =
        ((tcam_tbl->hlp.total_usage + count) <= tcam_tbl->total_entries);
  }

  if (free_space) {
    free_space =
        ((tcam_tbl->hlp.total_usage + count) <= tcam_tbl_info->tbl_size_max);
  }

  return free_space;
}

static void pipe_mgr_tcam_print_action_spec(rmt_dev_info_t *dev_info,
                                            profile_id_t profile_id,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            pipe_action_spec_t *act_spec) {
  if (!act_spec) {
    return;
  }

  LOG_DBG("Action Spec:");
  pipe_mgr_entry_format_log_action_spec(
      dev_info->dev_id, BF_LOG_DBG, profile_id, act_spec, act_fn_hdl);
}

/** \brief pipe_mgr_tcam_entry_add
 *        Adds an entry to a tcam tbl
 *
 * This function finds a suitable position for the tcam entry according to
 *the
 * priority. If an entry has to be displaced to make space for the new entry,
 * it'll move the entries and associated data
 *
 * \param sess_hdl Session hdl
 * \param dev_tgt Device target
 * \param mat_tbl_hdl Table hdl of the match tbl
 */

pipe_status_t pipe_mgr_tcam_entry_place(dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *match_spec,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *act_spec,
                                        uint32_t ttl,
                                        uint32_t pipe_api_flags,
                                        pipe_mat_ent_hdl_t *ent_hdl_p,
                                        pipe_mgr_move_list_t **move_head_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_ent_hdl_t entry_hdl = -1;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);
  if (tcam_pipe_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "TCAM table for pipe %d not found",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        mat_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }

  *ent_hdl_p = 0;
  entry_hdl = pipe_mgr_tcam_entry_hdl_allocate(tcam_pipe_tbl);
  if (entry_hdl == PIPE_TCAM_INVALID_ENT_HDL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "TCAM table for pipe %d: Failed to allocate entry handle",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        mat_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_NO_SPACE;
  }
  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    pipe_mat_ent_hdl_t ha_entry_hdl = -1;

    rc = pipe_mgr_hitless_ha_lookup_spec(&tcam_pipe_tbl->spec_map,
                                         match_spec,
                                         act_spec,
                                         act_fn_hdl,
                                         entry_hdl,
                                         &ha_entry_hdl,
                                         ttl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to lookup spec during entry add to tcam table %s 0x%x "
          "pipe %d",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          dev_tgt.dev_pipe_id);
      return rc;
    }
    if (entry_hdl != ha_entry_hdl) {
      /* Free the entry-hdl */
      pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl, entry_hdl);
      entry_hdl = ha_entry_hdl;
      pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl, entry_hdl);
    }
    if (act_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE) {
      pipe_mgr_tcam_update_sel_hlp_state(
          dev_tgt, mat_tbl_hdl, entry_hdl, act_spec->sel_grp_hdl);
    }
  } else {
    rc = pipe_mgr_tcam_entry_place_with_hdl(dev_tgt,
                                            mat_tbl_hdl,
                                            match_spec,
                                            act_fn_hdl,
                                            act_spec,
                                            ttl,
                                            pipe_api_flags,
                                            entry_hdl,
                                            move_head_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d TCAM entry place failed, dev %d table %s 0x%x pipe %d rc %s",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          dev_tgt.dev_pipe_id,
          pipe_str_err(rc));
      goto cleanup;
    }
  }

  *ent_hdl_p = entry_hdl;
  return PIPE_SUCCESS;

cleanup:
  *ent_hdl_p = 0;
  pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl, entry_hdl);
  return rc;
}

pipe_status_t pipe_mgr_tcam_entry_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  /* Sanity checking */
  if (match_spec == NULL) {
    LOG_ERROR("%s:%d No match spec passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE(
      "%s Request to install TCAM entry tbl 0x%x dev %d pipe %x match spec",
      tcam_tbl_info->name,
      mat_tbl_hdl,
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id);
  pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                       BF_LOG_INFO,
                                       tcam_tbl_info->profile_id,
                                       mat_tbl_hdl,
                                       match_spec);
  pipe_mgr_tcam_print_action_spec(
      tcam_tbl_info->dev_info, tcam_tbl_info->profile_id, act_fn_hdl, act_spec);

  tcam_pipe_tbl_t *tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);

  if (!tcam_pipe_tbl) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "TCAM table for pipe %d not found",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        mat_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->sess_flags = pipe_api_flags;

  tcam_tbl_t *tcam_tbl =
      get_tcam_tbl(tcam_pipe_tbl, match_spec->partition_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Partition %d does not exist in tcam table (max partitions - %d)",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        match_spec->partition_index,
        tcam_pipe_tbl->no_ptns);
    return PIPE_INVALID_ARG;
  }

  if (!pipe_mgr_tcam_check_free_space(tcam_tbl_info, tcam_tbl, 1)) {
    LOG_ERROR(
        "Dev %d pipe %d TCAM table %s 0x%x no space, current usage %d max "
        "entries %d",
        tcam_tbl_info->dev_id,
        dev_tgt.dev_pipe_id,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl->hlp.total_usage,
        tcam_tbl->total_entries);
    return PIPE_NO_SPACE;
  }

  pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl, entry_hdl);

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_tcam_entry_add_internal(tcam_tbl,
                                        match_spec,
                                        act_fn_hdl,
                                        act_spec,
                                        ttl,
                                        entry_hdl,
                                        move_head_p ? &move_tail : NULL,
                                        false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d TCAM entry add failed for dev %d tbl_hdl %d "
        "pipe %d rc 0x%x",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        mat_tbl_hdl,
        tcam_pipe_tbl->pipe_id,
        rc);
    goto cleanup;
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  return PIPE_SUCCESS;
cleanup:

  pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl, entry_hdl);
  return rc;
}

/** \brief pipe_mgr_tcam_entry_del_internal
 *        Internal function to delete an entry
 *
 * This function deletes an entry from the tcam tbl. It also calls other
 * APIs to delete the associated action entries, meters, stats etc
 *
 * \param sess_hdl Session hdl
 * \param mat_ent_hdl tcam entry hdl to delete
 * \param pipe_api_flags
 * \return pipe_status_t  The status of the operation
 */
static pipe_status_t pipe_mgr_tcam_entry_del_internal(
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *head_entry,
    pipe_mgr_move_list_t **move_tail_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  uint32_t tcam_index = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL;

  /* Entry deletions are easy. Just need to mark the spot as free
   * and free any entry specific structures
   */

  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    tcam_phy_loc_info_t tcam_loc;
    if (!tcam_entry) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
        tcam_tbl, tcam_entry->index, tcam_entry->is_default, &tcam_loc);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the physical location info for entry 0x%x "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_entry->entry_hdl,
          rc);
      return rc;
    }

    LOG_TRACE(
        "TCAM %s(0x%x - %d) pipe %d ptn %d "
        "Entry 0x%x deleted successfully from %5d (%2d-%5d) ",
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
        tcam_tbl->ptn_index,
        tcam_entry->entry_hdl,
        tcam_entry->index,
        tcam_loc.stage_id,
        tcam_loc.stage_line_no);
  }
  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END()

  if (!tcam_entry) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  rc = pipe_mgr_tcam_update_move_op(tcam_tbl,
                                    tcam_entry,
                                    PIPE_TCAM_OP_DELETE,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    false,
                                    NULL,
                                    move_tail_p,
                                    false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating move operations for deleting entry 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    tcam_index = tcam_entry->index;
    rc = pipe_mgr_tcam_update_prio_array(
        tcam_tbl, tcam_entry->group, tcam_entry->priority, tcam_index, false);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    /* Delete the entry */
    tcam_tbl->hlp.tcam_entries[tcam_index] = NULL;

    rc = pipe_mgr_tcam_mark_index_free(tcam_tbl, tcam_index);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
  }
  FOR_ALL_TCAM_HLP_ENTRIES_BLOCK_END()

  /* Remove the entry from global hash table */
  pipe_mgr_tcam_entry_htbl_add_delete(
      tcam_tbl, head_entry->entry_hdl, NULL, false);

  PIPE_MGR_DBGCHK(tcam_tbl->hlp.total_usage);
  tcam_tbl->hlp.total_usage--;

  pipe_mgr_tcam_hlp_entry_destroy_all(head_entry);
  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_tcam_entry_delete
 *        Delets an entry from a tcam tbl
 *
 * This function deletes an entry from the tcam tbl. It also calls other
 * APIs to delete the associated action entries, meters, stats etc
 *
 * \param sess_hdl Session hdl
 * \param mat_ent_hdl tcam entry hdl to delete
 * \param pipe_api_flags
 * \return pipe_status_t  The status of the operation
 */
pipe_status_t pipe_mgr_tcam_entry_del(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      uint32_t pipe_api_flags,
                                      pipe_mgr_move_list_t **move_head_p) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL;
  uint32_t old_version = 0;
  uint8_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    rc = PIPE_OBJ_NOT_FOUND;
    goto cleanup;
  }
  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          dev_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  if (!pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, mat_ent_hdl)) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id,
              mat_ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_pipe_tbl->hlp.default_ent_hdl == mat_ent_hdl) {
    LOG_ERROR("Default entry cannot be deleted. Operation not supported");
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->sess_flags = pipe_api_flags;

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  /* First find the tcam_index based on the mat-ent-hdl */
  tcam_entry = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, mat_ent_hdl, 0);
  if (tcam_entry == NULL) {
    LOG_ERROR("%s:%d get tcam entry failed", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Entry deletions are easy. Just need to mark the spot as free
   * and free any entry specific structures
   */

  /* Backup the entry first - index backup backs up the prio_node too */
  pipe_mgr_tcam_entry_backup_one(tcam_pipe_tbl, mat_ent_hdl);

  /* check if it is a atomic transaction and if so add
   * into the entry_del list, update
   */
  bool del_entry = true;

  if (TCAM_SESS_IS_ATOMIC(tcam_pipe_tbl)) {
    del_entry = false;
  }
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Partition %d does not exist in tcam table (max partitions - %d)",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        tcam_entry->ptn_index,
        tcam_pipe_tbl->no_ptns);
    return PIPE_UNEXPECTED;
  }
  if (tcam_entry->prev_atomic) {
    /* This entry was added in this atomic session. */
    BF_LIST_DLL_REM(
        tcam_tbl->hlp.entry_add_list, tcam_entry, next_atomic, prev_atomic);
    del_entry = true;
  }

  if (del_entry) {
    rc = pipe_mgr_tcam_entry_del_internal(
        tcam_tbl, tcam_entry, move_head_p ? &move_tail : NULL);
    tcam_entry = NULL;
  } else {
    BF_LIST_DLL_AP(
        tcam_tbl->hlp.entry_del_list, tcam_entry, next_atomic, prev_atomic);
    rc = pipe_mgr_tcam_entry_update_version(
        tcam_tbl, tcam_entry, old_version, false);
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error deleting tcam entry with handle %d",
              __func__,
              __LINE__,
              mat_ent_hdl);
    goto cleanup;
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  if (del_entry) {
    pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl, mat_ent_hdl);
  }

  return PIPE_SUCCESS;

cleanup:
  return rc;
}

static void add_new_resource_to_act_spec(pipe_action_spec_t *action_spec,
                                         pipe_res_spec_t *resource) {
  PIPE_MGR_MEMCPY(&action_spec->resources[action_spec->resource_count],
                  resource,
                  sizeof(pipe_res_spec_t));
  action_spec->resource_count++;
}

static pipe_status_t pipe_mgr_tcam_ent_set_action_internal(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  struct pipe_mgr_mat_data *old_data = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint8_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  bool has_direct_stful = !!pipe_mgr_tcam_get_tbl_ref_by_type(
      tcam_tbl_info, PIPE_HDL_TYPE_STFUL_TBL, PIPE_TBL_REF_TYPE_DIRECT);
  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          dev_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  if (!pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, mat_ent_hdl)) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id,
              mat_ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (act_spec) {
    LOG_TRACE("%s Match entry set action dev %d entry handle %#x with action",
              tcam_tbl_info->name,
              dev_id,
              mat_ent_hdl);

    pipe_mgr_tcam_print_action_spec(tcam_tbl_info->dev_info,
                                    tcam_tbl_info->profile_id,
                                    act_fn_hdl,
                                    act_spec);
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->sess_flags = pipe_api_flags;

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  tcam_hlp_entry_t *head_entry = NULL;
  /* First find the tcam_index based on the mat-ent-hdl */
  head_entry = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, mat_ent_hdl, 0);
  if (head_entry == NULL) {
    LOG_ERROR("%s:%d get tcam entry failed", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (tcam_pipe_tbl->hlp.default_ent_hdl == mat_ent_hdl) {
    PIPE_MGR_DBGCHK(head_entry && head_entry->is_default);
  }
  /* Backup the entry */
  pipe_mgr_tcam_entry_backup_one(tcam_pipe_tbl, mat_ent_hdl);

  pipe_action_spec_t *cur_act_spec =
      unpack_mat_ent_data_as(head_entry->mat_data);
  pipe_res_spec_t *cur_resources = cur_act_spec->resources;
  int cur_resource_count = cur_act_spec->resource_count;

  /* Create a new action spec */
  pipe_action_spec_t *new_act_spec = NULL;
  if (!act_spec) {
    /* This is a set_resource call. Use the existing action spec */
    act_fn_hdl = unpack_mat_ent_data_afun_hdl(head_entry->mat_data);
    new_act_spec = pipe_mgr_tbl_copy_action_spec(NULL, cur_act_spec);
    if (!new_act_spec) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  } else {
    new_act_spec = pipe_mgr_tbl_copy_action_spec(NULL, act_spec);
    if (!new_act_spec) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    /* Overwrite the resources with the existing resources */
    PIPE_MGR_MEMCPY(new_act_spec->resources,
                    cur_resources,
                    sizeof(pipe_res_spec_t) * cur_resource_count);
    new_act_spec->resource_count = cur_resource_count;
  }

  /* Now update the resources */
  int i;
  bool new_stful_spec = false;
  for (i = 0; i < resource_count; i++) {
    pipe_res_spec_t *rs = &resources[i];
    switch (rs->tag) {
      case PIPE_RES_ACTION_TAG_ATTACHED:
        if (PIPE_GET_HDL_TYPE(rs->tbl_hdl) == PIPE_HDL_TYPE_STFUL_TBL) {
          new_stful_spec = true;
        }
      /* Fall through */
      case PIPE_RES_ACTION_TAG_DETACHED: {
        /* Need to delete the old resource from tcam */
        pipe_res_spec_t *trs =
            get_resource_from_act_spec(new_act_spec, rs->tbl_hdl);
        if (trs) {
          pipe_mgr_tcam_entry_copy_resource(trs, rs);
        } else {
          add_new_resource_to_act_spec(new_act_spec, rs);
        }
      } break;
      case PIPE_RES_ACTION_TAG_NO_CHANGE:
        /* Do nothing */
        break;
    }
  }
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Partition %d does not exist in tcam table (max partitions - %d)",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        head_entry->ptn_index,
        tcam_pipe_tbl->no_ptns);
    rc = PIPE_UNEXPECTED;
    goto cleanup;
  }

  rc = pipe_mgr_tcam_verify_resources(tcam_tbl, new_act_spec);
  if (rc != PIPE_SUCCESS) {
    goto cleanup;
  }

  old_data = head_entry->mat_data;
  if (!act_spec || !tcam_tbl_info->reserve_modify_ent ||
      (tcam_pipe_tbl->hlp.default_ent_set &&
       mat_ent_hdl == tcam_pipe_tbl->hlp.default_ent_hdl) ||
      pipe_mgr_is_device_virtual(dev_id)) {
    rc = pipe_mgr_tcam_update_move_op(
        tcam_tbl,
        head_entry,
        PIPE_TCAM_OP_MODIFY,
        unpack_mat_ent_data_ms(head_entry->mat_data),
        act_fn_hdl,
        new_act_spec,
        unpack_mat_ent_data_ttl(head_entry->mat_data),
        has_direct_stful && new_stful_spec,
        NULL,
        &move_tail,
        false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error updating move operations for updating entry 0x%x "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          head_entry->entry_hdl,
          rc);
      head_entry->mat_data = old_data;
      goto cleanup;
    }
  } else {
    /* Modify atomically by moving to a new location */
    pipe_tbl_match_spec_t *match_spec =
        unpack_mat_ent_data_ms(head_entry->mat_data);
    tcam_hlp_entry_t *tcam_entry = NULL;
    uint32_t entries_per_block = 1, no_blocks = 1, j = 0;

    rc = pipe_mgr_entry_format_tof_count_range_expand(tcam_tbl_info->dev_id,
                                                      tcam_tbl_info->profile_id,
                                                      tcam_tbl_info->tbl_hdl,
                                                      match_spec,
                                                      &no_blocks,
                                                      &entries_per_block);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error expanding the TCAM range rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      goto cleanup;
    } else if (no_blocks > 1 || entries_per_block > 1) {
      LOG_TRACE(
          "%s:%d %s(0x%x-%d) range-entry %d expands to %d blocks of %d entries",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          head_entry->entry_hdl,
          no_blocks,
          entries_per_block);
    }

    uint32_t free[no_blocks];

    rc = pipe_mgr_tcam_find_free_slot(tcam_tbl,
                                      head_entry,
                                      head_entry->group,
                                      head_entry->priority,
                                      no_blocks,
                                      entries_per_block,
                                      free,
                                      &move_tail,
                                      false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error finding free space to modify entry %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          head_entry->entry_hdl,
          rc);
      goto cleanup;
    }

    construct_entry_mat_data(head_entry,
                             match_spec,
                             act_fn_hdl,
                             new_act_spec,
                             unpack_mat_ent_data_ttl(head_entry->mat_data),
                             has_direct_stful && new_stful_spec);
    tcam_entry = head_entry;
    for (j = 0; j < no_blocks; j++, tcam_entry = tcam_entry->next) {
      PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[free[j]] == NULL);
      rc = pipe_mgr_tcam_set_tcam_index_for_move(
          tcam_tbl, free[j], tcam_entry->index, &move_tail);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x-%d) "
            "Error allocating index %d to tcam entry handle %d "
            "rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id,
            free[j],
            tcam_entry->entry_hdl,
            rc);
        head_entry->mat_data = old_data;
        goto cleanup;
      }
      if (TCAM_TBL_USES_RANGE(tcam_tbl_info)) {
        move_tail->op = PIPE_MAT_UPDATE_MOV_MULTI_MOD;
      } else {
        move_tail->op = PIPE_MAT_UPDATE_MOV_MOD;
      }
      if (j == 0) {
        move_tail->old_data = old_data;
      }
    }
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

cleanup:
  if (new_act_spec) {
    pipe_mgr_tbl_destroy_action_spec(&new_act_spec);
  }
  return rc;
}

pipe_status_t pipe_mgr_tcam_ent_set_action(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_act_fn_hdl_t act_fn_hdl,
                                           pipe_action_spec_t *act_spec,
                                           uint32_t pipe_api_flags,
                                           pipe_mgr_move_list_t **head_p) {
  return pipe_mgr_tcam_ent_set_action_internal(dev_id,
                                               mat_tbl_hdl,
                                               mat_ent_hdl,
                                               act_fn_hdl,
                                               act_spec,
                                               act_spec->resources,
                                               act_spec->resource_count,
                                               pipe_api_flags,
                                               head_p);
}

pipe_status_t pipe_mgr_tcam_ent_set_resource(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_res_spec_t *resources,
                                             int resource_count,
                                             uint32_t pipe_api_flags,
                                             pipe_mgr_move_list_t **head_p) {
  return pipe_mgr_tcam_ent_set_action_internal(dev_id,
                                               mat_tbl_hdl,
                                               mat_ent_hdl,
                                               0,
                                               NULL,
                                               resources,
                                               resource_count,
                                               pipe_api_flags,
                                               head_p);
}

/** \brief pipe_mgr_tcam_commit_complete
 *        Function to be called after commit has completed in atomic
 *transactions
 *
 * This function should be called (by the driver interface) once all
 * the instructions have been pushed to the hardware. It is necessary only
 *for
 * atomic transaction sessions. This function walks through all the add
 * entries and removes the versioning bits. All the delete entries
 * are actually deleted in this function
 *
 */
static pipe_status_t pipe_mgr_tcam_commit_complete(
    tcam_pipe_tbl_t *tcam_pipe_tbl) {
  pipe_mgr_move_list_t **move_head_p = NULL;
  tcam_hlp_entry_t *tcam_entry = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_t *tcam_tbl = NULL;

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  FOR_ALL_PTNS_BEGIN(tcam_pipe_tbl, tcam_tbl) {
    while (tcam_tbl->hlp.entry_add_list) {
      tcam_entry = tcam_tbl->hlp.entry_add_list;
      BF_LIST_DLL_REM(
          tcam_tbl->hlp.entry_add_list, tcam_entry, next_atomic, prev_atomic);

      rc = pipe_mgr_tcam_entry_update_version(tcam_tbl, tcam_entry, 0, true);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error updating the version of the tcam entry %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            tcam_entry->entry_hdl,
            rc);
        return rc;
      }
    }

    while (tcam_tbl->hlp.entry_del_list) {
      tcam_entry = tcam_tbl->hlp.entry_del_list;
      BF_LIST_DLL_REM(
          tcam_tbl->hlp.entry_del_list, tcam_entry, next_atomic, prev_atomic);

      pipe_mgr_tcam_entry_hdl_release(tcam_pipe_tbl, tcam_entry->entry_hdl);

      rc = pipe_mgr_tcam_entry_del_internal(
          tcam_tbl, tcam_entry, move_head_p ? &move_tail : NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error deleting the entry %d tcam tbl %#x rc 0x%x",
                  __func__,
                  __LINE__,
                  tcam_entry->entry_hdl,
                  tcam_pipe_tbl->tcam_tbl_info_p->tbl_hdl,
                  rc);
        return rc;
      }
      tcam_entry = NULL;
    }
  }
  FOR_ALL_PTNS_END();

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  return rc;
}

static void pipe_mgr_tcam_entries_htbl_assert(tcam_pipe_tbl_t *tcam_pipe_tbl) {
  tcam_hlp_entry_t *tcam_entry = NULL;
  uint32_t tcam_index = 0;

  bf_map_sts_t st;
  unsigned long key;

  st = bf_map_get_first(
      &tcam_pipe_tbl->hlp.tcam_entry_db, &key, (void **)&tcam_entry);
  while (st == BF_MAP_OK) {
    tcam_index = tcam_entry->index;

    tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return;
    }

    PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[tcam_index] == tcam_entry);
    st = bf_map_get_next(
        &tcam_pipe_tbl->hlp.tcam_entry_db, &key, (void **)&tcam_entry);
  }
}

/** \brief pipe_mgr_tcam_assert
 *        Checks the validity of tcam entries and their priorities
 *
 * This function is used for unit-testing, to make sure that priority
 * is honored after entry moves etc
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \return boolean Returns true if the priority handling is correct, false
 *         otherwise
 */
bool pipe_mgr_tcam_assert(bf_dev_id_t dev_id,
                          pipe_mat_tbl_hdl_t tbl_hdl,
                          bool debug) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint32_t i = 0, j = 0;
  tcam_hlp_entry_t *tcam_entry = NULL;
  uint8_t pipe_id = 0;
  uint32_t total_usage = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return false;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    uint32_t total_pipe_tbl_usage = 0;
    tcam_tbl_t *tcam_tbl = NULL;
    FOR_ALL_PTNS_BEGIN(tcam_pipe_tbl, tcam_tbl) {
      if (global_tcam_debug && debug) {
        LOG_TRACE("PIPE ID %d", pipe_id);
        LOG_TRACE("------------------------");
      }

      for (i = 0; i < tcam_tbl->total_entries; i++) {
        if (tcam_tbl->hlp.tcam_entries[i] == NULL) {
          if (global_tcam_debug && debug) {
            LOG_TRACE("FREE\t%10d", i);
          }
          continue;
        }

        if (debug) {
          if (tcam_tbl->hlp.tcam_entries[i]->is_default) {
            LOG_TRACE("%10d\t%10d\t%10d\t%10d", 0, -1, i, 0);
          } else {
            LOG_TRACE(
                "%10d\t%10d\t%10d\t%10d",
                tcam_tbl->hlp.tcam_entries[i]->group,
                pipe_mgr_tcam_entry_priority_get(tcam_tbl->hlp.tcam_entries[i]),
                i,
                tcam_tbl->hlp.tcam_entries[i]->entry_hdl + 1);
          }
        }
      }
      for (i = 0; i < tcam_tbl->total_entries; i++) {
        if (!tcam_tbl->hlp.tcam_entries[i]) {
          PIPE_MGR_DBGCHK(pipe_mgr_tcam_index_is_free(tcam_tbl, i));
          continue;
        }

        PIPE_MGR_DBGCHK(!pipe_mgr_tcam_index_is_free(tcam_tbl, i));
        if (tcam_tbl->hlp.tcam_entries[i]->is_default) {
          PIPE_MGR_DBGCHK((i + 1) == tcam_tbl->total_entries);
          continue;
        }

        PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[i]->index == i);

        tcam_entry = tcam_tbl->hlp.tcam_entries[i];

        /*Check that the prio-node is right */
        uint32_t start = 0, end = 0;
        PIPE_MGR_DBGCHK(pipe_mgr_tcam_get_prio_range(
            tcam_tbl, tcam_entry->group, tcam_entry->priority, &start, &end));
        PIPE_MGR_DBGCHK(start <= i);
        PIPE_MGR_DBGCHK(end >= i);

        PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[start]);
        PIPE_MGR_DBGCHK(tcam_tbl->hlp.tcam_entries[end]);

        for (j = i + 1; j < tcam_tbl->total_entries; j++) {
          if (tcam_tbl->hlp.tcam_entries[j] == NULL) {
            continue;
          }

          if (tcam_tbl->hlp.tcam_entries[j]->group !=
              tcam_tbl->hlp.tcam_entries[i]->group) {
            continue;
          }

          if (pipe_mgr_tcam_entry_priority_get(tcam_tbl->hlp.tcam_entries[i]) >
              pipe_mgr_tcam_entry_priority_get(tcam_tbl->hlp.tcam_entries[j])) {
            PIPE_MGR_DBGCHK(0);
            return false;
          }
        }
      }

      total_usage = tcam_tbl->hlp.total_usage;
      total_pipe_tbl_usage += total_usage;

      tcam_group_info_t *group_info = NULL;
      uint16_t group = 0;
      for (group = 0; group < tcam_tbl->hlp.max_tcam_group; group++) {
        group_info = tcam_tbl->hlp.group_info[group];
        if (group_info == NULL) {
          continue;
        }

        Word_t priority = 0;
        PWord_t Pvalue;
        JLF(Pvalue, group_info->jtcam_prio_array, priority);
        while (Pvalue) {
          tcam_prio_range_t *prio_range_node = (tcam_prio_range_t *)*Pvalue;
          uint32_t index;
          PIPE_MGR_DBGCHK(prio_range_node->end >= prio_range_node->start);
          for (index = prio_range_node->start; index <= prio_range_node->end;
               index++) {
            tcam_entry = tcam_tbl->hlp.tcam_entries[index];
            if (tcam_entry) {
              PIPE_MGR_DBGCHK(tcam_entry->group == group);
              PIPE_MGR_DBGCHK(tcam_entry->priority == priority);
            }
          }
          JLN(Pvalue, group_info->jtcam_prio_array, priority);
        }
      }
    }
    FOR_ALL_PTNS_END();
    if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
      total_pipe_tbl_usage--;
    }

    PIPE_MGR_DBGCHK(bf_map_count(&tcam_pipe_tbl->hlp.tcam_entry_db) ==
                    total_pipe_tbl_usage);
    pipe_mgr_tcam_entries_htbl_assert(tcam_pipe_tbl);
  }

  if (debug) {
    LOG_TRACE("LARGEST move  %5d", largest_move_cnt);
    LOG_TRACE("TOTAL adds %" PRIu64, total_entry_add_cnt);
    LOG_TRACE("      adds with moves %" PRIu64, total_entry_move);
    LOG_TRACE("      total moves %" PRIu64, total_move_cnt);
    LOG_TRACE("      avg move %f", ((double)total_move_cnt / total_entry_move));
  }

  return true;
}

/** \brief pipe_mgr_tcam_get_placed_entry_count
 *        Returns the number of valid entries
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param uint32_t* Pointer to the count of valid entries to fill
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_get_placed_entry_count(dev_target_t dev_tgt,
                                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                                   uint32_t *count_p) {
  uint32_t tcam_count = 0;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint8_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == tcam_pipe_tbl->pipe_id) {
      uint32_t ptn;
      for (ptn = 0; ptn < tcam_pipe_tbl->no_ptns; ptn++) {
        tcam_tbl_t *tcam_tbl;
        tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn];
        tcam_count += tcam_tbl->hlp.total_usage;
      }
      if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
        PIPE_MGR_DBGCHK(tcam_count);
        tcam_count--;
      }
    }
  }

  *count_p = tcam_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) &&
      (!tcam_tbl_info->is_symmetric)) {
    LOG_ERROR(
        "%s:%d Tcam table 0x%x, Invalid pipe %d specified for asymmetric tbl, "
        "dev %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);

  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table for 0x%x pipe %d on dev %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_tcam_default_ent_place_with_hdl(
      dev_tgt,
      mat_tbl_hdl,
      act_fn_hdl,
      act_spec,
      pipe_api_flags,
      tcam_pipe_tbl->hlp.default_ent_hdl,
      move_head_p,
      false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d TCAM default entry add failed for dev %d tbl_hdl %d "
        "pipe %d rc 0x%x",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        rc);
    goto cleanup;
  }

  *ent_hdl_p = tcam_pipe_tbl->hlp.default_ent_hdl;
  return PIPE_SUCCESS;

cleanup:
  *ent_hdl_p = 0;
  return rc;
}

pipe_status_t pipe_mgr_tcam_default_ent_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_head_p,
    bool use_move_node) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("Dev %d pipe %x %s Match default entry add with action",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            tcam_tbl_info->name);
  pipe_mgr_tcam_print_action_spec(
      tcam_tbl_info->dev_info, tcam_tbl_info->profile_id, act_fn_hdl, act_spec);

  tcam_pipe_tbl_t *tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);

  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table for 0x%x pipe %d on dev %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->sess_flags = pipe_api_flags;

  if (ent_hdl != tcam_pipe_tbl->hlp.default_ent_hdl) {
    LOG_ERROR(
        "%s:%d Invalid default entry handle 0x%x passed for tcam table 0x%x "
        "pipe %d device %d",
        __func__,
        __LINE__,
        ent_hdl,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  tcam_hlp_entry_t *tcam_entry = NULL;

  tcam_tbl_t *tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[0];

  pipe_mgr_tcam_default_entry_backup(tcam_pipe_tbl);

  tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  if (use_move_node) {
    move_tail = *move_head_p;
  }

  if (tcam_pipe_tbl->hlp.default_ent_set) {
    rc = pipe_mgr_tcam_update_move_op(tcam_tbl,
                                      tcam_entry,
                                      PIPE_TCAM_OP_MODIFY,
                                      NULL,
                                      act_fn_hdl,
                                      act_spec,
                                      0,
                                      true,
                                      NULL,
                                      move_head_p ? &move_tail : NULL,
                                      use_move_node);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error updating move operations for updating default entry"
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          rc);
      goto cleanup;
    }
  } else {
    tcam_entry->entry_hdl = ent_hdl;
    rc = pipe_mgr_tcam_update_move_op(tcam_tbl,
                                      tcam_entry,
                                      PIPE_TCAM_OP_SET_DEFAULT,
                                      NULL,
                                      act_fn_hdl,
                                      act_spec,
                                      0,
                                      false,
                                      NULL,
                                      move_head_p ? &move_tail : NULL,
                                      use_move_node);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "Error updating move operations for setting default entry"
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          rc);
      goto cleanup;
    }
  }

  if (move_head_p && !use_move_node) {
    *move_head_p = move_head.next;
  }

  tcam_pipe_tbl->hlp.default_ent_set = true;

  return PIPE_SUCCESS;

cleanup:
  return rc;
}

static tcam_pipe_tbl_t *get_tcam_pipe_tbl_by_any_pipe_id(
    tcam_tbl_info_t *tcam_tbl_info, bf_dev_pipe_t pipe_id) {
  uint32_t i;
  if (pipe_id == BF_DEV_PIPE_ALL) {
    if (tcam_tbl_info->is_symmetric) return &tcam_tbl_info->tcam_pipe_tbl[0];
    return NULL;
  }

  for (i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
    tcam_pipe_tbl_t *tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];
    if (PIPE_BITMAP_GET(&tcam_pipe_tbl->pipe_bmp, pipe_id)) {
      return tcam_pipe_tbl;
    }
  }
  return NULL;
}

/* Get default entry handle */
pipe_status_t pipe_mgr_tcam_default_ent_hdl_get(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t *ent_hdl_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_any_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);

  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table %s 0x%x pipe %x on dev %d not found",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  *ent_hdl_p = 0;
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    if (!tcam_pipe_tbl->hlp.default_ent_set) {
      return PIPE_OBJ_NOT_FOUND;
    }
    *ent_hdl_p = tcam_pipe_tbl->hlp.default_ent_hdl;
  } else {
    if (!tcam_pipe_tbl->llp.default_ent_set) {
      return PIPE_OBJ_NOT_FOUND;
    }
    *ent_hdl_p = tcam_pipe_tbl->llp.default_ent_hdl;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_default_ent_get(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_action_spec_t *action_spec,
                                            pipe_act_fn_hdl_t *act_fn_hdl,
                                            bool from_hw) {
  pipe_status_t sts = PIPE_SUCCESS;
  tcam_hlp_entry_t *tcam_entry = NULL;
  tcam_tbl_info_t *tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  if (from_hw)
    tcam_pipe_tbl =
        get_tcam_pipe_tbl_by_any_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);
  else
    tcam_pipe_tbl =
        get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);
  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table for 0x%x pipe %d on dev %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  tcam_pipe_tbl->cur_sess_hdl = sess_hdl;

  if (tcam_pipe_tbl->hlp.default_ent_set) {
    tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
  }

  if (from_hw) {
    tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, 0);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_TABLE_NOT_FOUND;
    }

    tcam_phy_loc_info_t tcam_loc = {0};
    sts = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
        tcam_tbl, tcam_tbl->total_entries - 1, true, &tcam_loc);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the physical location info for default entry "
          "sts 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          sts);
      return sts;
    }

    sts = tcam_get_default_entry_from_hw(
        tcam_tbl, &tcam_loc, dev_tgt.dev_pipe_id, action_spec, act_fn_hdl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting default entry info sts 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          sts);
      return sts;
    }
    if (action_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE &&
        tcam_entry) {
      action_spec->sel_grp_hdl =
          unpack_mat_ent_data_as(tcam_entry->mat_data)->sel_grp_hdl;
    }
  } else {
    if (tcam_entry) {
      pipe_action_spec_t *def_act_spec =
          unpack_mat_ent_data_as(tcam_entry->mat_data);
      if (def_act_spec) {
        pipe_mgr_tbl_copy_action_spec(action_spec, def_act_spec);
      }
      *act_fn_hdl = unpack_mat_ent_data_afun_hdl(tcam_entry->mat_data);
    } else {
      return pipe_mgr_tbl_get_init_default_entry(
          dev_tgt.device_id, mat_tbl_hdl, action_spec, act_fn_hdl);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_tbl_set_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp,
    bool is_backup) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  uint32_t total_entries = 0, total_blocks = 0, usage = 0;
  uint32_t blocks_per_bank = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  LOG_TRACE("%s: Table %s, Change to symmetric mode %d ",
            __func__,
            tcam_tbl_info->name,
            symmetric);

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       tcam_tbl_info->is_symmetric,
                                       tcam_tbl_info->num_scopes,
                                       &tcam_tbl_info->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              tcam_tbl_info->name,
              tcam_tbl_info->is_symmetric,
              tcam_tbl_info->num_scopes);
    return rc;
  }

  rc = pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, true, &usage);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to get entry count for tcam tbl 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return rc;
  }
  if (usage > 0) {
    LOG_TRACE(
        "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d ",
        __func__,
        tcam_tbl_info->name,
        tcam_tbl_info->is_symmetric,
        usage);
    return PIPE_NOT_SUPPORTED;
  }

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bool idle_present;
  uint32_t num_stages;
  rc = get_num_stages_and_blocks(mat_tbl_info,
                                 &total_entries,
                                 &num_stages,
                                 &total_blocks,
                                 &blocks_per_bank,
                                 &idle_present);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }

  // cleanup first
  pipe_mgr_tcam_pipe_tbl_destroy(tcam_tbl_info->tcam_pipe_tbl,
                                 tcam_tbl_info->no_tcam_pipe_tbls);

  tcam_tbl_info->is_symmetric = symmetric;
  mat_tbl_info->symmetric = symmetric;
  /* Copy the new scope info */
  tcam_tbl_info->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(tcam_tbl_info->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  if (tcam_tbl_info->is_symmetric) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->num_scopes == 1);
  }
  tcam_tbl_info->no_tcam_pipe_tbls = tcam_tbl_info->num_scopes;
  pipe_bitmap_t pipe_bmp;
  rc = pipe_mgr_get_pipe_bmp_for_profile(tcam_tbl_info->dev_info,
                                         tcam_tbl_info->profile_id,
                                         &pipe_bmp,
                                         __func__,
                                         __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the pipe-bmp for profile-id %d "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->profile_id,
        rc);
    PIPE_MGR_DBGCHK(0);
    return rc;
  }

  if (tcam_tbl_info->is_symmetric) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->num_scopes == 1);
    tcam_tbl_info->llp.lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(tcam_tbl_info->scope_pipe_bmp[0]);
  }

  // allocate with new setting
  tcam_tbl_info->tcam_pipe_tbl = pipe_mgr_tcam_pipe_tbl_alloc(tcam_tbl_info,
                                                              mat_tbl_info,
                                                              &pipe_bmp,
                                                              total_entries,
                                                              num_stages,
                                                              total_blocks,
                                                              blocks_per_bank);
  rc = pipe_mgr_tcam_install_def_entry(tcam_tbl_info, is_backup);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in setting up default entry for tbl 0x%x"
        " device id %d, err %s",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    /* Re-init shadow memory metadata, since the symmetricity of the table
     * has changed.
     */
    rc = pipe_mgr_tcam_init_shadow_mem(tcam_tbl_info);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing shadow memory metadata for tbl 0x%x"
          " device id %d, err %s",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }

  return rc;
}

pipe_status_t pipe_mgr_tcam_get_first_placed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  uint32_t pipe_idx;
  bool is_backup = false;

  *entry_hdl = -1;
  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls; pipe_idx++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_idx];
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        tcam_pipe_tbl->pipe_id != dev_tgt.dev_pipe_id) {
      continue;
    }
    *entry_hdl =
        bf_id_allocator_get_first(tcam_pipe_tbl->hlp.ent_hdl_allocator);
    if (*entry_hdl != -1) {
      if (tcam_pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
        *entry_hdl = PIPE_SET_HDL_PIPE(*entry_hdl, tcam_pipe_tbl->pipe_id);
      }
      if (is_entry_hdl_default(tcam_tbl_info, *entry_hdl, true)) {
        int next_hdl = -1;
        pipe_status_t rc = pipe_mgr_tcam_get_next_placed_entry_handles(
            tbl_hdl, dev_tgt, *entry_hdl, 1, &next_hdl);
        if (PIPE_SUCCESS != rc) {
          *entry_hdl = -1;
        } else {
          *entry_hdl = next_hdl;
        }
      }
      break;
    }
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_next_placed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bool is_backup = false;
  pipe_mat_tbl_hdl_t hdl = 0;
  int new_hdl = 0;
  int i = 0;
  uint32_t pipe_id;

  if (n) {
    next_entry_handles[0] = -1;
  }
  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric) {
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != pipe_id) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d for entry hdl %d passed for "
          "asymmetric tcam tbl with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  hdl = PIPE_GET_HDL_VAL(entry_hdl);
  i = 0;
  while (i < n) {
    new_hdl =
        bf_id_allocator_get_next(tcam_pipe_tbl->hlp.ent_hdl_allocator, hdl);
    if (new_hdl == -1) {
      if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
        // We've reached the end of this pipe, so exit
        break;
      }
      while (tcam_pipe_tbl->pipe_idx <
             (uint32_t)(tcam_tbl_info->no_tcam_pipe_tbls - 1)) {
        tcam_pipe_tbl =
            &tcam_tbl_info->tcam_pipe_tbl[tcam_pipe_tbl->pipe_idx + 1];
        new_hdl =
            bf_id_allocator_get_first(tcam_pipe_tbl->hlp.ent_hdl_allocator);
        if (new_hdl != -1) {
          break;
        }
      }
      if (new_hdl == -1) {
        next_entry_handles[i] = new_hdl;
        break;
      }
    }
    if (tcam_pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
      new_hdl = PIPE_SET_HDL_PIPE(new_hdl, tcam_pipe_tbl->pipe_id);
    }
    if (is_entry_hdl_default(tcam_tbl_info, new_hdl, true)) {
      hdl = PIPE_GET_HDL_VAL(new_hdl);
      continue;
    }

    next_entry_handles[i] = new_hdl;
    hdl = PIPE_GET_HDL_VAL(new_hdl);
    i++;
  }
  if (i < n) {
    next_entry_handles[i] = -1;
  }

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_ent_hdl_t entry_hdl,
                                      pipe_tbl_match_spec_t *pipe_match_spec,
                                      pipe_action_spec_t *pipe_action_spec,
                                      pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bool is_backup = false;
  pipe_tbl_match_spec_t *match_spec;
  pipe_action_spec_t *action_spec;
  tcam_hlp_entry_t *tcam_entry = NULL;
  bf_dev_pipe_t pipe_id;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      tcam_pipe_tbl->pipe_id != dev_tgt.dev_pipe_id) {
    LOG_TRACE(
        "%s : Entry with handle %d with pipe id %d does not match requested "
        "pipe id %d in ternary match table with handle %d, device_id %d",
        __func__,
        entry_hdl,
        tcam_pipe_tbl->pipe_id,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Check if the entry handle passed in a valid one */
  if (pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, entry_hdl) == false) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Entry handle 0x%x not valid in tcam table",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_entry = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, entry_hdl, 0);
  if (tcam_entry == NULL) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Entry 0x%x does not exist in tcam table",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  match_spec = unpack_mat_ent_data_ms(tcam_entry->mat_data);
  action_spec = unpack_mat_ent_data_as(tcam_entry->mat_data);

  /* Copy the match spec */
  pipe_match_spec = pipe_mgr_tbl_copy_match_spec(pipe_match_spec, match_spec);
  if (!pipe_match_spec) {
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Copy the action spec */
  pipe_action_spec =
      pipe_mgr_tbl_copy_action_spec(pipe_action_spec, action_spec);
  if (!pipe_action_spec) {
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Copy the action function hdl */
  *act_fn_hdl = unpack_mat_ent_data_afun_hdl(tcam_entry->mat_data);

  return rc;
}

static pipe_status_t pipe_mgr_tcam_get_range_plcmt_data(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_hlp_entry_t *head_entry,
    pipe_mgr_move_list_t *move_node) {
  tcam_tbl_t *tcam_tbl = tcam_pipe_tbl->tcam_ptn_tbls;
  struct pipe_multi_index *locations;
  tcam_hlp_entry_t *range_head;
  uint32_t count = 0;

  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
    count++;
  }
  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END();

  locations = (struct pipe_multi_index *)PIPE_MGR_CALLOC(
      count, sizeof(struct pipe_multi_index));

  if (locations == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
    locations[range_head->subentry_index].logical_index_base =
        get_logical_index_from_index_ptn(get_tcam_pipe_tbl(tcam_tbl),
                                         range_head->ptn_index,
                                         range_head->index);
    TCAM_HLP_GET_RANGE_ENTRY_COUNT(
        range_head, locations[range_head->subentry_index].logical_index_count);
  }
  FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END();

  move_node->u.multi.array_sz = count;
  move_node->u.multi.locations = locations;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_get_def_ent_plcmt_data(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_mgr_move_list_t **ml_head,
    pipe_mgr_move_list_t **move_list_tail) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  pipe_mgr_move_list_t *ml, *ml_tail = *move_list_tail;
  tcam_hlp_entry_t *tcam_entry;
  bf_dev_pipe_t pipe, pipe_id;

  for (pipe = 0; pipe < tcam_tbl_info->no_tcam_pipe_tbls; pipe++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe];

    if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info))
      pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
    else
      pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);

    if (tcam_pipe_tbl->hlp.default_ent_set) {
      tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
      ml = alloc_move_list(
          NULL,
          PIPE_MAT_UPDATE_SET_DFLT,
          TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info) ? BF_DEV_PIPE_ALL : pipe_id);
      if (!ml) {
        LOG_ERROR("%s:%d Error allocating move list for table 0x%x device %d",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id);
        *move_list_tail = ml_tail;
        return PIPE_NO_SYS_RESOURCES;
      }
      ml->entry_hdl = tcam_pipe_tbl->hlp.default_ent_hdl;
      ml->data = tcam_entry->mat_data;
      ml->logical_sel_idx = tcam_entry->logical_sel_idx;
      ml->logical_action_idx = tcam_entry->logical_action_idx;
      ml->u.single.logical_idx = tcam_entry->index;

      if (!(*ml_head)) {
        *ml_head = ml;
      } else {
        ml_tail->next = ml;
      }
      ml_tail = ml;
    }
  }

  *move_list_tail = ml_tail;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_plcmt_data(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mgr_move_list_t **move_list) {
  pipe_mgr_move_list_t *ml, *ml_head = NULL, *ml_tail = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  tcam_hlp_entry_t *tcam_entry;
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t pipe, pipe_id;
  unsigned long key;
  bf_map_sts_t ms;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found for device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bool is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);
  for (pipe = 0; pipe < tcam_tbl_info->no_tcam_pipe_tbls; pipe++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe];

    if (TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info))
      pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
    else
      pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);

    for (ms = bf_map_get_first(
             &tcam_pipe_tbl->hlp.tcam_entry_db, &key, (void **)&tcam_entry);
         ms == BF_MAP_OK;
         ms = bf_map_get_next(
             &tcam_pipe_tbl->hlp.tcam_entry_db, &key, (void **)&tcam_entry)) {
      mat_ent_hdl = key;
      if (is_range) {
        /* Range entry */
        ml = alloc_move_list(
            NULL,
            PIPE_MAT_UPDATE_ADD_MULTI,
            TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info) ? BF_DEV_PIPE_ALL : pipe_id);
        if (!ml) {
          LOG_ERROR("%s:%d Error allocating move list for table 0x%x device %d",
                    __func__,
                    __LINE__,
                    tcam_tbl_info->tbl_hdl,
                    tcam_tbl_info->dev_id);
          rc = PIPE_NO_SYS_RESOURCES;
          goto cleanup;
        }
        /* Add the range entry to the move list. */
        rc = pipe_mgr_tcam_get_range_plcmt_data(tcam_pipe_tbl, tcam_entry, ml);
        if (rc != PIPE_SUCCESS) goto cleanup;
      } else {
        /* Single entry */
        tcam_tbl_t *tcam_tbl = tcam_pipe_tbl->tcam_ptn_tbls;
        ml = alloc_move_list(
            NULL,
            PIPE_MAT_UPDATE_ADD,
            TCAM_TBL_IS_SYMMETRIC(tcam_tbl_info) ? BF_DEV_PIPE_ALL : pipe_id);
        if (!ml) {
          LOG_ERROR("%s:%d Error allocating move list for table 0x%x device %d",
                    __func__,
                    __LINE__,
                    tcam_tbl_info->tbl_hdl,
                    tcam_tbl_info->dev_id);
          rc = PIPE_NO_SYS_RESOURCES;
          goto cleanup;
        }
        ml->u.single.logical_idx =
            get_logical_index_from_index_ptn(get_tcam_pipe_tbl(tcam_tbl),
                                             tcam_entry->ptn_index,
                                             tcam_entry->index);
      }

      ml->entry_hdl = mat_ent_hdl;
      ml->data = tcam_entry->mat_data;
      ml->logical_sel_idx = tcam_entry->logical_sel_idx;
      ml->logical_action_idx = tcam_entry->logical_action_idx;

      if (!ml_head) {
        ml_head = ml;
      } else {
        ml_tail->next = ml;
      }
      ml_tail = ml;
    }
  }

  /* Add the default entry, if present, to the move list. */
  rc = pipe_mgr_tcam_get_def_ent_plcmt_data(tcam_tbl_info, &ml_head, &ml_tail);
  if (rc != PIPE_SUCCESS) goto cleanup;

  *move_list = ml_head;
  return PIPE_SUCCESS;

cleanup:
  /* Clean up anything already allocated. */
  while (ml_head) {
    pipe_mgr_move_list_t *x = ml_head;
    ml_head = ml_head->next;
    if (x->op == PIPE_MAT_UPDATE_ADD_MULTI && x->u.multi.locations)
      PIPE_MGR_FREE(x->u.multi.locations);
    PIPE_MGR_FREE(x);
  }
  return rc;
}

pipe_status_t pipe_mgr_tcam_get_match_spec(pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bf_dev_id_t device_id,
                                           bf_dev_pipe_t *pipe_id,
                                           pipe_tbl_match_spec_t **match_spec) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_hlp_entry_t *entry_data = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;

  bool is_backup = false;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(device_id, mat_tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
    *pipe_id = BF_DEV_PIPE_ALL;
  } else {
    *pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, *pipe_id);
    if (!tcam_pipe_tbl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          mat_tbl_hdl,
          device_id,
          *pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  /* Check if the entry handle passed in a valid one */
  if (pipe_mgr_tcam_is_valid_entry_hdl(tcam_pipe_tbl, mat_ent_hdl) == false) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Entry handle 0x%x not valid in tcam table",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        mat_ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  entry_data = pipe_mgr_tcam_entry_get(tcam_pipe_tbl, mat_ent_hdl, 0);
  if (entry_data) {
    *pipe_id = tcam_pipe_tbl->pipe_id;
    *match_spec = unpack_mat_ent_data_ms(entry_data->mat_data);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_cleanup_default_entry(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);

  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table %s 0x%x pipe %x on dev %d not found",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->sess_flags = pipe_api_flags;
  tcam_tbl_t *tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[0];
  tcam_hlp_entry_t *tcam_entry = NULL;

  pipe_mgr_tcam_default_entry_backup(tcam_pipe_tbl);

  tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;

  if (!tcam_pipe_tbl->hlp.default_ent_set) {
    return PIPE_SUCCESS;
  }

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  rc = pipe_mgr_tcam_update_move_op(tcam_tbl,
                                    tcam_entry,
                                    PIPE_TCAM_OP_CLEAR_DEFAULT,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    false,
                                    NULL,
                                    move_head_p ? &move_tail : NULL,
                                    false);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Error updating move operations for setting default entry"
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        rc);
    return rc;
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  tcam_pipe_tbl->hlp.default_ent_set = false;
  return rc;
}

pipe_status_t pipe_mgr_tcam_entry_hdl_from_stage_idx(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe,
    pipe_mat_tbl_hdl_t tbl_hdl,
    int stage_id,
    int logical_tbl_id,
    uint32_t hit_addr,
    pipe_mat_ent_hdl_t *entry_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (!tcam_tbl_info) {
    return PIPE_INVALID_ARG;
  }

  /* Confirm that this table is in the requested stage with the requested
   * logical table id. */
  unsigned int i;
  for (i = 0; i < tcam_tbl_info->tcam_pipe_tbl[0].num_stages; ++i) {
    if (tcam_tbl_info->tcam_pipe_tbl[0].stage_data[i].stage_id == stage_id &&
        tcam_tbl_info->tcam_pipe_tbl[0].stage_data[i].stage_table_handle ==
            logical_tbl_id) {
      break;
    }
  }
  if (i == tcam_tbl_info->tcam_pipe_tbl[0].num_stages) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the pipe table from the table info.  Note, we are not using
   * get_tcam_pipe_tbl_by_pipe_id here because the pipe id that we have may not
   * be the lowest pipe id in the profile. */
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  unsigned int tbl_idx;
  for (tbl_idx = 0; tbl_idx < tcam_tbl_info->no_tcam_pipe_tbls; ++tbl_idx) {
    if (PIPE_BITMAP_GET(&tcam_tbl_info->tcam_pipe_tbl[tbl_idx].pipe_bmp,
                        pipe)) {
      tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[tbl_idx];
    }
  }
  if (!tcam_pipe_tbl) return PIPE_INVALID_ARG;

  uint32_t tcam_index, ptn_index;
  rc = pipe_mgr_get_tcam_index_for_match_addr(tcam_pipe_tbl,
                                              stage_id,
                                              logical_tbl_id,
                                              hit_addr,
                                              &tcam_index,
                                              &ptn_index);
  if (PIPE_SUCCESS != rc) return rc;

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn_index);
  if (!tcam_tbl) {
    LOG_ERROR(
        "%s:%d %s(0x%x-%d) "
        "Partition %d does not exist in tcam table (max partitions - %d)",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        ptn_index,
        tcam_pipe_tbl->no_ptns);
    return PIPE_INVALID_ARG;
  }

  if (tcam_tbl_info->dev_info->virtual_device) {
    if (!tcam_tbl->hlp.tcam_entries[tcam_index]) {
      LOG_TRACE(
          "%s:%d %s(0x%x-%d) Partition %d does not have entry at index %d",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          ptn_index,
          tcam_index);
      return PIPE_OBJ_NOT_FOUND;
    }
    *entry_hdl = tcam_tbl->hlp.tcam_entries[tcam_index]->entry_hdl;
  } else {
    if (!tcam_tbl->llp.tcam_entries[tcam_index]) {
      LOG_TRACE(
          "%s:%d %s(0x%x-%d) Partition %d does not have entry at index %d",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          ptn_index,
          tcam_index);
      return PIPE_OBJ_NOT_FOUND;
    }
    *entry_hdl = tcam_tbl->llp.tcam_entries[tcam_index]->entry_hdl;
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_tcam_print_match_spec(bf_dev_id_t device_id,
                                    pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                    pipe_tbl_match_spec_t *match_spec,
                                    char *buf,
                                    size_t buf_len) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  size_t bytes_written = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    buf[0] = '\0';
    return;
  }

  pipe_mgr_entry_format_print_match_spec(device_id,
                                         tcam_tbl_info->profile_id,
                                         mat_tbl_hdl,
                                         match_spec,
                                         buf,
                                         buf_len,
                                         &bytes_written);

  return;
}

void populate_tcam_addr_node(tcam_llp_entry_t *entry, cJSON *entry_node) {
  tcam_indirect_addr_t *addr = &(entry->addr);
  cJSON *addr_node;

  cJSON_AddItemToObject(entry_node, "addr", addr_node = cJSON_CreateObject());
  cJSON_AddNumberToObject(addr_node, "action", addr->indirect_addr_action);
  cJSON_AddNumberToObject(addr_node, "sel", addr->indirect_addr_sel);
  cJSON_AddNumberToObject(addr_node, "pvl", addr->sel_grp_pvl);
  cJSON_AddNumberToObject(addr_node, "stats", addr->indirect_addr_stats);
  cJSON_AddNumberToObject(addr_node, "meter", addr->indirect_addr_meter);
  cJSON_AddNumberToObject(addr_node, "stful", addr->indirect_addr_stful);
  cJSON_AddNumberToObject(addr_node, "idle", addr->indirect_addr_idle);

  return;
}

pipe_status_t pipe_mgr_tcam_log_state(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_info_t *mat_info,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      cJSON *match_tbls) {
  bf_map_sts_t st;
  tcam_tbl_info_t *tcam_tbl_info;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  tcam_hlp_entry_t *tcam_entry;
  tcam_hlp_entry_t *range_head;
  tcam_llp_entry_t *llp_entry;
  tcam_llp_entry_t *llp_range_head;
  unsigned long ent_hdl;
  uint32_t pipe_idx;
  bool is_range;
  cJSON *match_tbl, *pipe_tbls, *pipe_tbl, *llp;
  cJSON *def_ent, *mat_ents, *mat_ent, *ranges, *range;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);

  cJSON_AddItemToArray(match_tbls, match_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(match_tbl, "name", tcam_tbl_info->name);
  cJSON_AddNumberToObject(match_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(match_tbl, "symmetric", tcam_tbl_info->is_symmetric);
  if (mat_info) {
    cJSON_AddBoolToObject(
        match_tbl, "duplicate_entry_check", mat_info->duplicate_entry_check);
  }
  cJSON_AddItemToObject(
      match_tbl, "pipe_tbls", pipe_tbls = cJSON_CreateArray());

  for (pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls; pipe_idx++) {
    tcam_pipe_tbl = &(tcam_tbl_info->tcam_pipe_tbl[pipe_idx]);
    cJSON_AddItemToArray(pipe_tbls, pipe_tbl = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe_tbl, "pipe_id", tcam_pipe_tbl->pipe_id);
    cJSON_AddItemToObject(
        pipe_tbl, "default_entry", def_ent = cJSON_CreateObject());
    if (tcam_pipe_tbl->hlp.default_ent_set) {
      tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
      cJSON_AddNumberToObject(
          def_ent, "entry_hdl", tcam_pipe_tbl->hlp.default_ent_hdl);
      cJSON_AddNumberToObject(
          def_ent, "action_idx", tcam_entry->logical_action_idx);
      cJSON_AddNumberToObject(def_ent, "sel_idx", tcam_entry->logical_sel_idx);
      cJSON_AddNumberToObject(def_ent, "sel_len", tcam_entry->selector_len);
      pipe_mgr_tbl_log_specs(dev_id,
                             tcam_tbl_info->profile_id,
                             tbl_hdl,
                             tcam_entry->mat_data,
                             def_ent,
                             true);
    }
    if (tcam_pipe_tbl->llp.default_ent_set) {
      if (!tcam_pipe_tbl->hlp.default_ent_set) {
        cJSON_AddNumberToObject(
            def_ent, "entry_hdl", tcam_pipe_tbl->llp.default_ent_hdl);
      }
      populate_tcam_addr_node(tcam_pipe_tbl->llp.llp_default_tcam_entry,
                              def_ent);
    }

    cJSON_AddItemToObject(
        pipe_tbl, "match_entries", mat_ents = cJSON_CreateArray());
    st = bf_map_get_first(
        &tcam_pipe_tbl->hlp.tcam_entry_db, &ent_hdl, (void **)&tcam_entry);
    while (st == BF_MAP_OK) {
      if (!tcam_entry->is_default) {
        cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
        cJSON_AddNumberToObject(mat_ent, "entry_hdl", tcam_entry->entry_hdl);
        cJSON_AddNumberToObject(mat_ent, "entry_idx", tcam_entry->index);
        cJSON_AddNumberToObject(
            mat_ent, "action_idx", tcam_entry->logical_action_idx);
        cJSON_AddNumberToObject(
            mat_ent, "sel_idx", tcam_entry->logical_sel_idx);
        cJSON_AddNumberToObject(mat_ent, "sel_len", tcam_entry->selector_len);
        pipe_mgr_tbl_log_specs(dev_id,
                               tcam_tbl_info->profile_id,
                               tbl_hdl,
                               tcam_entry->mat_data,
                               mat_ent,
                               false);

        if (is_range) {
          cJSON_AddItemToObject(
              mat_ent, "range_heads", ranges = cJSON_CreateArray());
          FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(tcam_entry,
                                                          range_head) {
            cJSON_AddItemToArray(ranges, range = cJSON_CreateObject());
            cJSON_AddNumberToObject(range, "entry_idx", range_head->index);
            cJSON_AddNumberToObject(
                range, "action_idx", range_head->logical_action_idx);
            cJSON_AddNumberToObject(
                range, "sel_idx", range_head->logical_sel_idx);
            cJSON_AddNumberToObject(range, "sel_len", range_head->selector_len);
          }
          FOR_ALL_TCAM_HLP_RANGE_HEAD_ENTRIES_BLOCK_END();
        }
      }
      st = bf_map_get_next(
          &tcam_pipe_tbl->hlp.tcam_entry_db, &ent_hdl, (void **)&tcam_entry);
    }
  }

  cJSON_AddItemToObject(match_tbl, "llp", llp = cJSON_CreateObject());
  cJSON_AddItemToObject(llp, "match_entries", mat_ents = cJSON_CreateArray());
  for (pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls; pipe_idx++) {
    tcam_pipe_tbl = &(tcam_tbl_info->tcam_pipe_tbl[pipe_idx]);
    st = bf_map_get_first(
        &tcam_pipe_tbl->llp.tcam_entry_db, &ent_hdl, (void **)&llp_entry);
    while (st == BF_MAP_OK) {
      if (!llp_entry->is_default) {
        cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
        cJSON_AddNumberToObject(mat_ent, "entry_hdl", ent_hdl);
        cJSON_AddNumberToObject(mat_ent, "pipe_id", llp_entry->pipe_id);
        populate_tcam_addr_node(llp_entry, mat_ent);

        if (is_range) {
          cJSON_AddItemToObject(
              mat_ent, "range_heads", ranges = cJSON_CreateArray());
          FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(llp_entry,
                                                          llp_range_head) {
            cJSON_AddItemToArray(ranges, range = cJSON_CreateObject());
            populate_tcam_addr_node(llp_range_head, range);
          }
          FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();
        }
      }
      st = bf_map_get_next(
          &tcam_pipe_tbl->llp.tcam_entry_db, &ent_hdl, (void **)&llp_entry);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  bool is_backup = false;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    return PIPE_INVALID_ARG;
  }
  LOG_TRACE("%s: Table %s, Get symmetric mode %d ",
            __func__,
            tcam_tbl_info->name,
            tcam_tbl_info->is_symmetric);
  *symmetric = tcam_tbl_info->is_symmetric;
  *num_scopes = tcam_tbl_info->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  tcam_tbl_info->scope_pipe_bmp,
                  tcam_tbl_info->num_scopes * sizeof(scope_pipes_t));

  return PIPE_SUCCESS;
}

void pipe_mgr_restore_tcam_addr_node(tcam_tbl_info_t *tbl_info,
                                     tcam_llp_entry_t *entry) {
  tcam_indirect_addr_t *addr = &(entry->addr);
  cJSON *addr_node;

  addr_node = cJSON_GetObjectItem(tbl_info->restore_ent_node, "addr");
  addr->indirect_addr_stats =
      cJSON_GetObjectItem(addr_node, "stats")->valuedouble;
  addr->indirect_addr_meter =
      cJSON_GetObjectItem(addr_node, "meter")->valuedouble;
  addr->indirect_addr_stful =
      cJSON_GetObjectItem(addr_node, "stful")->valuedouble;
  addr->indirect_addr_idle =
      cJSON_GetObjectItem(addr_node, "idle")->valuedouble;

  return;
}

pipe_status_t pipe_mgr_tcam_restore_state(bf_dev_id_t dev_id,
                                          pipe_mat_tbl_info_t *mat_info,
                                          cJSON *tcam_tbl,
                                          alpm_restore_callback_fn cb_fn) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t st = BF_MAP_OK;
  dev_target_t dev_tgt;
  tcam_tbl_info_t *tbl_info;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  tcam_tbl_t *tcam_tbl_local;
  tcam_hlp_entry_t *tcam_entry;
  pipe_mgr_move_list_t *move_node;
  pipe_tbl_match_spec_t ms = {0};
  pipe_action_spec_t as = {0};
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t ent_hdl;
  uint32_t pipe_idx;
  uint32_t pipe_id;
  bool symmetric;
  uint32_t success_count = 0;
  cJSON *pipe_tbls, *pipe_tbl, *llp;
  cJSON *def_ent, *mat_ents, *mat_ent;
  scope_pipes_t scopes = 0xf;

  dev_tgt.device_id = dev_id;
  tbl_hdl = cJSON_GetObjectItem(tcam_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  symmetric = (cJSON_GetObjectItem(tcam_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != tbl_info->is_symmetric) {
    sts = pipe_mgr_tcam_tbl_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, 1, &scopes, false);
    sts |= pipe_mgr_tcam_tbl_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, 1, &scopes, true);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, tcam tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      goto done;
    }
  }
  if (mat_info && tbl_hdl == mat_info->handle) {
    mat_info->duplicate_entry_check =
        (cJSON_GetObjectItem(tcam_tbl, "duplicate_entry_check")->type ==
         cJSON_True);
  }

  pipe_tbls = cJSON_GetObjectItem(tcam_tbl, "pipe_tbls");
  for (pipe_tbl = pipe_tbls->child, pipe_idx = 0; pipe_tbl;
       pipe_tbl = pipe_tbl->next, pipe_idx++) {
    tcam_pipe_tbl = &tbl_info->tcam_pipe_tbl[pipe_idx];
    tcam_pipe_tbl->cur_sess_hdl = pipe_mgr_get_int_sess_hdl();
    dev_tgt.dev_pipe_id =
        (uint32_t)cJSON_GetObjectItem(pipe_tbl, "pipe_id")->valueint;
    PIPE_MGR_DBGCHK(tcam_pipe_tbl->pipe_id == dev_tgt.dev_pipe_id);

    def_ent = cJSON_GetObjectItem(pipe_tbl, "default_entry");
    if (cJSON_GetObjectItem(def_ent, "entry_hdl")) {
      if (cJSON_GetObjectItem(def_ent, "act_spec")) {
        tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
        tcam_entry->entry_hdl =
            cJSON_GetObjectItem(def_ent, "entry_hdl")->valuedouble;
        tcam_entry->logical_action_idx =
            cJSON_GetObjectItem(def_ent, "action_idx")->valuedouble;
        tcam_entry->logical_sel_idx =
            cJSON_GetObjectItem(def_ent, "sel_idx")->valuedouble;
        tcam_entry->selector_len =
            cJSON_GetObjectItem(def_ent, "sel_len")->valuedouble;

        pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl, tcam_entry->entry_hdl);
        pipe_mgr_tbl_restore_specs(dev_id,
                                   tbl_info->profile_id,
                                   tbl_hdl,
                                   def_ent,
                                   &ms,
                                   &as,
                                   &act_fn_hdl);
        uint32_t ttl = 0;
        if (tbl_info->idle_present) {
          ttl = NONZERO_TTL;
        }
        tcam_entry->mat_data = make_mat_ent_data(
            NULL, &as, act_fn_hdl, ttl, tcam_entry->selector_len, 0, 0);
        PIPE_MGR_FREE(as.act_data.action_data_bits);
        tcam_pipe_tbl->hlp.default_ent_hdl = tcam_entry->entry_hdl;
        tcam_pipe_tbl->hlp.default_ent_set = true;
      }

      if (cJSON_GetObjectItem(def_ent, "addr")) {
        tcam_entry = tcam_pipe_tbl->hlp.hlp_default_tcam_entry;
        tbl_info->restore_ent_node = def_ent;
        move_node =
            pipe_mgr_tcam_construct_move_node(&tcam_pipe_tbl->tcam_ptn_tbls[0],
                                              tcam_entry,
                                              PIPE_TCAM_OP_SET_DEFAULT,
                                              NULL,
                                              0,
                                              NULL,
                                              0,
                                              false);
        success_count = 0;
        sts = pipe_mgr_tcam_process_move_list(tcam_pipe_tbl->cur_sess_hdl,
                                              dev_id,
                                              tbl_hdl,
                                              move_node,
                                              &success_count);
        free_move_list(&move_node, true);
        if (sts != PIPE_SUCCESS) {
          goto done;
        }
      }
    }

    mat_ents = cJSON_GetObjectItem(pipe_tbl, "match_entries");
    for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
      ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;
      tbl_info->restore_ent_node = mat_ent;

      pipe_mgr_tbl_restore_specs(dev_id,
                                 tbl_info->profile_id,
                                 tbl_hdl,
                                 mat_ent,
                                 &ms,
                                 &as,
                                 &act_fn_hdl);
      pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl, ent_hdl);

      tcam_tbl_local = get_tcam_tbl(tcam_pipe_tbl, ms.partition_index);
      if (tcam_tbl_local == NULL) {
        LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
        return PIPE_TABLE_NOT_FOUND;
      }

      sts = pipe_mgr_tcam_entry_add_internal(
          tcam_tbl_local, &ms, act_fn_hdl, &as, 0, ent_hdl, NULL, false);
      if (sts == PIPE_SUCCESS && cb_fn) {
        sts = cb_fn(dev_tgt, mat_info, tbl_hdl, ent_hdl, &ms, act_fn_hdl, &as);
      } else if (sts == PIPE_SUCCESS && mat_info) {
        sts = pipe_mgr_mat_tbl_key_insert(
            dev_id, mat_info, &ms, ent_hdl, tcam_pipe_tbl->pipe_id, false);
      }
      PIPE_MGR_FREE(ms.match_value_bits);
      PIPE_MGR_FREE(ms.match_mask_bits);
      PIPE_MGR_FREE(as.act_data.action_data_bits);
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
    }
  }

  llp = cJSON_GetObjectItem(tcam_tbl, "llp");
  mat_ents = cJSON_GetObjectItem(llp, "match_entries");
  for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
    ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;
    pipe_id = cJSON_GetObjectItem(mat_ent, "pipe_id")->valueint;
    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tbl_info, pipe_id);
    if (tcam_pipe_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam pipe table failed", __func__, __LINE__);
      return PIPE_TABLE_NOT_FOUND;
    }

    st = bf_map_get(
        &tcam_pipe_tbl->hlp.tcam_entry_db, ent_hdl, (void **)&tcam_entry);
    if (st != BF_MAP_OK) {
      sts = PIPE_OBJ_NOT_FOUND;
      goto done;
    }
    tbl_info->restore_ent_node = mat_ent;

    tcam_tbl_local = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
    if (tcam_tbl_local == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_TABLE_NOT_FOUND;
    }

    move_node = pipe_mgr_tcam_construct_move_node(tcam_tbl_local,
                                                  tcam_entry,
                                                  PIPE_TCAM_OP_ALLOCATE,
                                                  NULL,
                                                  0,
                                                  NULL,
                                                  0,
                                                  false);
    success_count = 0;
    sts = pipe_mgr_tcam_process_move_list(tcam_pipe_tbl->cur_sess_hdl,
                                          dev_id,
                                          tbl_hdl,
                                          move_node,
                                          &success_count);
    free_move_list(&move_node, true);
    if (sts != PIPE_SUCCESS) {
      goto done;
    }
  }

done:
  tbl_info->restore_ent_node = NULL;
  return sts;
}

static inline tcam_llp_entry_t *find_llp_entry(tcam_tbl_t *tcam_tbl,
                                               uint32_t entry_idx) {
  if (tcam_tbl->llp.tcam_entries) {
    for (uint32_t i = 0; i < tcam_tbl->total_entries; i++) {
      if (tcam_tbl->llp.tcam_entries[i]) {
        if (tcam_tbl->llp.tcam_entries[i]->index == entry_idx) {
          return tcam_tbl->llp.tcam_entries[i];
        }
      }
    }
  }
  return NULL;
}

pipe_status_t pipe_mgr_tcam_tbl_raw_entry_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              uint32_t tcam_index,
                                              bool err_correction,
                                              pipe_tbl_match_spec_t *match_spec,
                                              pipe_action_spec_t *act_spec,
                                              pipe_act_fn_hdl_t *act_fn_hdl,
                                              pipe_ent_hdl_t *entry_hdl,
                                              bool *is_default,
                                              uint32_t *next_index) {
  *act_fn_hdl = 0;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  uint32_t pipe_idx = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls; pipe_idx++) {
    if ((1 << dev_tgt.dev_pipe_id) & tcam_tbl_info->scope_pipe_bmp[pipe_idx]) {
      tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_idx];
      break;
    }
  }
  if (tcam_pipe_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  tcam_pipe_tbl->cur_sess_hdl = sess_hdl;

  // Each partition will have the same number of entries.
  uint32_t entries_per_ptn = tcam_pipe_tbl->tcam_ptn_tbls[0].total_entries;
  uint32_t ptn_entry_index = tcam_index % entries_per_ptn;
  uint32_t ptn_idx = tcam_index / entries_per_ptn;
  if (ptn_idx >= tcam_pipe_tbl->no_ptns) {
    return PIPE_INVALID_ARG;
  }

  tcam_tbl_t *tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn_idx];

  *is_default =
      (ptn_entry_index == (tcam_tbl->total_entries - 1) &&
       tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT &&
       ptn_idx == 0);

  uint32_t num_entries = 1;
  tcam_llp_entry_t *llp_entry = find_llp_entry(tcam_tbl, ptn_entry_index);
  if (llp_entry) {
    /* If entry exist, check if it is first entry in the range, if not update
     * ptn_entry_index to first one in the same range. */
    if (llp_entry->subentry_index != 0) {
      ptn_entry_index -= llp_entry->subentry_index;
      /* Update llp_entry to first one in case we need to fix missing entry. */
      llp_entry = find_llp_entry(tcam_tbl, ptn_entry_index);
      /* Update tcam_index to calculate next_index properly */
      tcam_index -= llp_entry->subentry_index;
    }
    *entry_hdl = llp_entry->entry_hdl;
    num_entries = llp_entry->range_count;
    if (num_entries > 1 && tcam_tbl_info->uses_range == false) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }

  /* Default entry has range_count 0, since there is no match, but we still want
   * to check if it is a valid entry or not */
  if (num_entries == 0) {
    if (*is_default) {
      num_entries = 1;
    } else {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }

  tcam_phy_loc_info_t tcam_loc;
  pipe_status_t rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, ptn_entry_index, *is_default, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_index,
        rc);
    return rc;
  }

  bool entry_valid = false;
  rc = tcam_hw_raw_entry_get(tcam_tbl,
                             dev_tgt.dev_pipe_id,
                             &tcam_loc,
                             ptn_entry_index,
                             match_spec,
                             act_spec,
                             act_fn_hdl,
                             num_entries,
                             &entry_valid);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  *next_index = tcam_index + num_entries;

  /* Following section checks and tries to fix the HW state in 2 cases.
   * 1. Entry exist in HW, but there is no SW state related to it.
   *    In such case entry is invalidated. Assumption here is that all action
   *    and/or resource items were invalidated since SW part is gone, hence
   *    there is no need to fix them. Default entry is invalid in TCAM.
   * 2. Entry exist in SW, but is missing in HW. In this case code will try to
   *    place complete entry at HW index, including subentries, actions, and
   *    resources in order to make sure entry is correct.
   */
  if (!entry_valid && llp_entry != NULL && !*is_default) {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) Missing entry in HW at index 0x%x, pipe %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_index,
        dev_tgt.dev_pipe_id);
    if (err_correction) {
      tcam_hlp_entry_t *hlp_entry = pipe_mgr_tcam_entry_get(
          tcam_pipe_tbl, llp_entry->entry_hdl, llp_entry->subentry_index);
      if (hlp_entry == NULL) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "HLP and LLP are out of sync, cannot repair missing entry 0x%x "
            "rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            tcam_index,
            rc);
      }
      if (hlp_entry) {
        rc = tcam_fix_missing_entry(tcam_tbl, llp_entry, hlp_entry->mat_data);
        if (rc) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error correcting HW values for index 0x%x "
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_index,
              rc);
        }
      }
    }
    // For this case return PIPE_INTERNAL_ERROR because of missmatch.
    rc = PIPE_INTERNAL_ERROR;
  } else if (entry_valid && llp_entry == NULL) {
    LOG_TRACE(
        "%s:%d - %s (%d - 0x%x) Unexpected entry in HW at index 0x%x, pipe %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_index,
        dev_tgt.dev_pipe_id);
    if (err_correction) {
      rc = tcam_fix_unexpected_entry(tcam_tbl, ptn_entry_index, num_entries);
      if (rc) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error correcting HW values for index 0x%x "
            "rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            tcam_index,
            rc);
      }
    }
    // For this case return PIPE_INTERNAL_ERROR because of missmatch.
    rc = PIPE_INTERNAL_ERROR;
  }
  /* Nothing to do in case of both valid/invalid. This code does not validate
   * action/match specs, only entry existence.
   */

  return rc;
}

pipe_status_t pipe_mgr_tcam_get_last_index(dev_target_t dev_tgt,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           uint32_t *last_index) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (uint32_t pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls;
       pipe_idx++) {
    if ((1 << dev_tgt.dev_pipe_id) & tcam_tbl_info->scope_pipe_bmp[pipe_idx]) {
      tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_idx];
      break;
    }
  }

  if (tcam_pipe_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  // Will work the same for tcam, atcam, range entries
  // Returning last index, not number of entries
  uint32_t entries_per_ptn = tcam_pipe_tbl->tcam_ptn_tbls[0].total_entries;
  *last_index = entries_per_ptn * tcam_pipe_tbl->no_ptns - 1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_tbl_update(dev_target_t dev_tgt,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_mat_ent_hdl_t ent_hdl,
                                       uint32_t pipe_api_flags,
                                       pipe_mgr_move_list_t **move_head_p) {
  return pipe_mgr_tcam_ent_set_action_internal(dev_tgt.device_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               0 /*act_fn_hdl*/,
                                               NULL /*act_spec*/,
                                               NULL /*resources*/,
                                               0 /*res_count*/,
                                               pipe_api_flags,
                                               move_head_p);
}

pipe_status_t pipe_mgr_tcam_get_reserved_entry_count(dev_target_t dev_tgt,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     size_t *count_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  *count_p = 0;

  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint32_t total_entries = 0;
  uint32_t tbl_size_in_p4 = 0;
  uint32_t tbl_size_max = 0;

  uint32_t default_ent_count = 0;
  uint32_t reserve_modify_ent = 0;

  tbl_size_max = tcam_tbl_info->tbl_size_max;
  tbl_size_in_p4 = tcam_tbl_info->tbl_size_in_p4;

  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_t *tcam_tbl = NULL;
  uint8_t pipe_id = 0;
  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == tcam_pipe_tbl->pipe_id) {
      if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
        default_ent_count++;
      }
      uint32_t ptn;
      for (ptn = 0; ptn < tcam_pipe_tbl->no_ptns; ptn++) {
        tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn];
        total_entries += tcam_tbl->total_entries;
        if (tcam_tbl_info->reserve_modify_ent && !tcam_tbl_info->uses_alpm) {
          reserve_modify_ent++;
        }
      }
    }
  }

  /* clang-format off */
    /* tbl_size_in_p4 - Table size dafined in P4
       tbl_size_max   - Max table size. Should be equal to tbl_size_in_p4 + default_ent_count if tbl_size_in_p4 < total_entries.
                        See pipe_mgr_tcam_tbl_create_internal (after call pipe_mgr_tcam_install_def_entry)
       total_entries  - Table size allocated by compiler
       reserve_modify_ent - One location reserved for atomic modify

       Desision table:
       tbl_size_in_p4 | default_ent_count | reserve_modify_ent  | tbl_size_max  | total_entries | Real size avalible for user
       1024             1                   0                     1024            1024            1023
       1023             1                   0                     1024            1024            1023
       1020             1                   0                     1021            1024            1020
       1024             1                   1                     1024            1024            1022
       1023             1                   1                     1024            1024            1022
       1020             1                   1                     1021            1024            1020
       1020             2                   2                     1022            1024            1020
       1020             3                   3                     1023            1024            1018
    */
  /* clang-format on */
  if (default_ent_count) {
    if (tbl_size_max == tbl_size_in_p4) {
      *count_p += default_ent_count;
    } else if (tbl_size_max - tbl_size_in_p4 != default_ent_count) {
      LOG_ERROR("%s: Unexpected reserved entries in tbl 0x%d device %d",
                __func__,
                tbl_hdl,
                dev_tgt.device_id);
    }
  }

  if (tbl_size_max + reserve_modify_ent > total_entries) {
    *count_p += tbl_size_max + reserve_modify_ent - total_entries;
  }

  return rc;
}

pipe_status_t pipe_mgr_tcam_get_total_entry_count(dev_target_t dev_tgt,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  size_t *count_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_t *tcam_tbl = NULL;

  uint32_t default_ent_count = 0;
  uint32_t total_entries = 0;
  for (uint8_t pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls;
       pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == tcam_pipe_tbl->pipe_id) {
      if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT)
        default_ent_count++;
      uint32_t ptn;
      for (ptn = 0; ptn < tcam_pipe_tbl->no_ptns; ptn++) {
        tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn];
        total_entries += tcam_tbl->total_entries;
      }
    }
  }

  /* Reserve count accounts default entry only if tbl_size_max == tbl_p4_sz */
  size_t reserved_count = 0;
  if (tcam_tbl_info->tbl_size_max != tcam_tbl_info->tbl_size_in_p4) {
    total_entries -= default_ent_count;
  }

  rc =
      pipe_mgr_tcam_get_reserved_entry_count(dev_tgt, tbl_hdl, &reserved_count);

  *count_p = total_entries - reserved_count;
  return rc;
}
