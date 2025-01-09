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
 * @file pipe_mgr_sel_ha_hlp.c
 * @date
 *
 */

/* Standard header includes */
#include <stdio.h>
#include <stdint.h>

/* Module header includes */
#include "pipe_mgr/pipe_mgr_intf.h"
#include "dvm/bf_dma_types.h"
#include "lld/bf_dma_if.h"
#include "lld/lld_inst_list_fmt.h"

/* Local header includes */
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_adt_tofino.h"

pipe_status_t pipe_mgr_sel_pipe_ha_hlp_init(sel_tbl_t *sel_tbl) {
  sel_tbl->hlp.ha_hlp_info = (pipe_mgr_sel_pipe_ha_hlp_info_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_sel_pipe_ha_hlp_info_t));
  if (sel_tbl->hlp.ha_hlp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_hlp_assign_grp_idx(
    sel_tbl_t *sel_tbl, pipe_mgr_sel_move_list_t *move_list_node) {
  pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key;
  uint32_t stage_idx, loc_idx;

  // Multi-device case does not need this mapping
  if (!ha_hlp_info) {
    return PIPE_SUCCESS;
  }

  pipe_mgr_sel_grp_ha_info_t *grp_ha_info =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_sel_grp_ha_info_t));
  if (!grp_ha_info) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  grp_ha_info->grp_hdl = move_list_node->sel_grp_hdl;
  grp_ha_info->pipe_id = sel_tbl->pipe_id;
  grp_ha_info->sel_base_idx = move_list_node->logical_sel_index;
  grp_ha_info->sel_len = move_list_node->sel_grp_size;
  grp_ha_info->adt_base_idx =
      PIPE_MGR_MALLOC(sel_tbl->num_stages * sizeof(pipe_adt_ent_idx_t));

  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    loc_idx = stage_idx * grp_ha_info->sel_len;
    grp_ha_info->adt_base_idx[stage_idx] =
        move_list_node->locations[loc_idx].logical_index_base;
  }

  key = grp_ha_info->sel_base_idx;
  map_sts = bf_map_add(&ha_hlp_info->idx_to_grp_info, key, (void *)grp_ha_info);
  if (map_sts == BF_MAP_KEY_EXISTS) {
    LOG_ERROR(
        "%s:%d Group idx to info mapping already exists for idx %d "
        "pipe %d tbl 0x%x",
        __func__,
        __LINE__,
        grp_ha_info->sel_base_idx,
        sel_tbl->pipe_id,
        sel_tbl->sel_tbl_info->tbl_hdl);
    if (grp_ha_info) {
      PIPE_MGR_FREE(grp_ha_info);
    }
    return PIPE_ALREADY_EXISTS;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_update_hlp_ref(bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_sel_tbl_hdl_t tbl_hdl,
                                          pipe_sel_grp_hdl_t grp_hdl) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;

  if (!pipe_mgr_is_device_virtual(device_id)) {
    // In single-process mode, the refcount is updated during cfg replay instead
    return PIPE_SUCCESS;
  }

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel tbl, with tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_tbl_t *sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d get sel table failed for tbl hdl 0x%x, pipe 0x%x, dev %d",
              __func__,
              __LINE__,
              tbl_hdl,
              pipe_id,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
  if (sel_grp_info == NULL) {
    LOG_ERROR("%s:%d sel grp %d not found for tbl 0x%x device %d",
              __func__,
              __LINE__,
              grp_hdl,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info->num_references++;
  return pipe_mgr_sel_update_mat_refs(sel_grp_info, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t pipe_mgr_sel_hlp_restore_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t *move_list,
    uint32_t *success_count,
    pd_ha_restore_cb_1 cb) {
  pipe_status_t status = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_grp_mbr_t *grp_mbr = NULL;
  sel_hlp_word_data_t *word_data;
  uint32_t pipe_idx, stage_idx;
  uint32_t word_idx = 0, mbr_idx = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  PWord_t Ppipe, Pstage;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel tbl, with tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_sel_move_list_t *move_list_node = NULL;
  for (move_list_node = move_list; move_list_node;
       move_list_node = move_list_node->next) {
    for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
      sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
      if (sel_tbl->pipe_id != move_list_node->pipe) {
        continue;
      }

      /* Set-up the session parameters */
      sel_tbl->cur_sess_hdl = pipe_mgr_get_int_sess_hdl();
      sel_tbl->sess_flags = 0;

      if (move_list_node->op == PIPE_SEL_UPDATE_GROUP_CREATE) {
        sel_grp_info = pipe_mgr_sel_grp_allocate();
        if (sel_grp_info == NULL) {
          LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
          status = PIPE_NO_SYS_RESOURCES;
          goto cleanup;
        }

        sel_grp_info->pipe_id = move_list_node->pipe;
        sel_grp_info->grp_hdl = move_list_node->sel_grp_hdl;
        sel_grp_info->max_grp_size = move_list_node->max_mbrs;

        for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
          sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
          for (sel_grp_stage_info = sel_stage_info->sel_grp_stage_free_list;
               sel_grp_stage_info;
               sel_grp_stage_info = sel_grp_stage_info->next) {
            if (sel_grp_stage_info->sel_base_idx ==
                move_list_node->logical_sel_index) {
              break;
            }
          }

          if (!sel_grp_stage_info) {
            status = pipe_mgr_sel_grp_allocate_in_stage(
                sel_tbl,
                sel_stage_info,
                sel_grp_info->max_grp_size,
                move_list_node->logical_sel_index,
                move_list_node
                    ->locations[stage_idx * move_list_node->sel_grp_size]
                    .logical_index_base,
                &sel_grp_stage_info);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR("%s:%d Unable to activate group %d in stage %d",
                        __func__,
                        __LINE__,
                        sel_grp_info->grp_hdl,
                        sel_stage_info->stage_id);
              goto cleanup;
            }
          }
          sel_grp_stage_info->inuse = true;
          sel_grp_stage_info->grp_hdl = sel_grp_info->grp_hdl;

          BF_LIST_DLL_REM(sel_stage_info->sel_grp_stage_free_list,
                          sel_grp_stage_info,
                          next,
                          prev);

          BF_LIST_DLL_AP(sel_stage_info->sel_grp_stage_inuse_list,
                         sel_grp_stage_info,
                         next,
                         prev);

          /* Update the hw_locator_list */
          status = pipe_mgr_sel_grp_update_grp_hw_locator_list(
              sel_tbl, sel_grp_info, sel_stage_info, sel_grp_stage_info);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR("Error updating hw_locator list for group %d in stage %d",
                      sel_grp_info->grp_hdl,
                      sel_stage_info->stage_id);
            goto cleanup;
          }
        }

        status = pipe_mgr_sel_grp_add_to_htbl(
            sel_tbl, move_list_node->sel_grp_hdl, sel_grp_info);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("%s:%d Error adding the grp %d to grp_hdl_htbl for pipe %d",
                    __func__,
                    __LINE__,
                    move_list_node->sel_grp_hdl,
                    move_list_node->pipe);
          goto cleanup;
        }

        status = pipe_mgr_sel_hlp_assign_grp_idx(sel_tbl, move_list_node);
        if (cb) {
          cb(sess_hdl,
             device_id,
             sel_tbl_info->adt_tbl_hdl,
             0,
             move_list_node->sel_grp_hdl,
             0,
             0,
             NULL);
        }
      } else if (move_list_node->op == PIPE_SEL_UPDATE_ADD) {
        sel_grp_info =
            pipe_mgr_sel_grp_get(sel_tbl, move_list_node->sel_grp_hdl);
        if (sel_grp_info == NULL) {
          LOG_ERROR("%s:%d %s(0x%x-%d) Selector group 0x%x not found",
                    __func__,
                    __LINE__,
                    sel_tbl_info->name,
                    sel_tbl_info->tbl_hdl,
                    sel_tbl_info->dev_id,
                    move_list_node->sel_grp_hdl);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }

        for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
          if (move_list_node->data->adt_index_array[stage_idx] !=
              PIPE_ADT_ENT_HDL_INVALID_HDL) {
            break;
          }
        }
        PIPE_MGR_ASSERT(stage_idx < sel_tbl->num_stages);
        JLG(Ppipe, sel_grp_info->sel_grp_pipe_lookup, pipe_idx);
        JLG(Pstage, *(Pvoid_t *)Ppipe, stage_idx);
        sel_grp_stage_info = (sel_grp_stage_info_t *)*Pstage;

        word_idx = move_list_node->logical_sel_index -
                   sel_grp_stage_info->sel_base_idx;
        mbr_idx = move_list_node->logical_sel_subindex;
        word_data = &sel_grp_stage_info->sel_grp_word_data[word_idx];
        PIPE_MGR_DBGCHK(word_data->mbrs[mbr_idx] == 0);
        word_data->usage++;
        word_data->mbrs[mbr_idx] = move_list_node->adt_mbr_hdl;
        sel_grp_stage_info->cur_usage++;
        pipe_mgr_sel_grp_mbr_hw_locator_update(
            sel_grp_stage_info, move_list_node->adt_mbr_hdl, word_idx, mbr_idx);

        grp_mbr = pipe_mgr_sel_grp_mbr_alloc(move_list_node->adt_mbr_hdl);
        status = pipe_mgr_sel_mbr_add_to_htbl(
            sel_grp_info, move_list_node->adt_mbr_hdl, grp_mbr, false);
        if (status == PIPE_SUCCESS) {
          sel_grp_info->mbr_count++;
        }
        status = rmt_adt_ent_non_sharable_get(device_id,
                                              sel_tbl_info->adt_tbl_hdl,
                                              move_list_node->adt_mbr_hdl,
                                              &act_fn_hdl);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Failed to get adt state for mbr %d in grp %d "
              "sel tbl 0x%x device %d",
              __func__,
              __LINE__,
              move_list_node->adt_mbr_hdl,
              move_list_node->sel_grp_hdl,
              sel_tbl_info->tbl_hdl,
              device_id);
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        if (!sel_grp_info->act_fn_set) {
          sel_grp_info->act_fn_hdl = act_fn_hdl;
          sel_grp_info->act_fn_set = true;
        } else if (sel_grp_info->act_fn_hdl != act_fn_hdl) {
          LOG_ERROR(
              "%s:%d Trying to add action member %d with incompatible action "
              "to group %d in selector table 0x%x device %d",
              __func__,
              __LINE__,
              move_list_node->adt_mbr_hdl,
              move_list_node->sel_grp_hdl,
              sel_tbl_info->tbl_hdl,
              device_id);
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        if (cb) {
          cb(sess_hdl,
             device_id,
             sel_tbl_info->adt_tbl_hdl,
             act_fn_hdl,
             move_list_node->sel_grp_hdl,
             move_list_node->adt_mbr_hdl,
             0,
             NULL);
        }
      } else if (move_list_node->op == PIPE_SEL_UPDATE_SET_FALLBACK) {
        sel_tbl->hlp.fallback_adt_ent_hdl = move_list_node->adt_mbr_hdl;
      }
    }
    (*success_count)++;
  }

cleanup:
  return status;
}

pipe_status_t pipe_mgr_sel_hlp_update_state(bf_dev_id_t device_id,
                                            bf_dev_pipe_t pipe_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            pipe_sel_tbl_hdl_t tbl_hdl,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            uint32_t sel_base_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t msts;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;
  pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = NULL;
  pipe_mgr_sel_grp_replay_info_t *grp_replay_info = NULL;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_hlp_word_data_t *word_data = NULL;
  bool resize = false;
  pipe_adt_ent_idx_t adt_base_idx;
  uint32_t stage_idx = 0;
  uint32_t no_words = 0;
  uint32_t entries_per_word = 0;
  uint32_t i;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel tbl, with tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d get sel table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }
  ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
  msts = bf_map_get(
      &ha_hlp_info->replay_hdl_to_info, grp_hdl, (void **)&grp_replay_info);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d Selector group 0x%x not found in tbl 0x%x",
              __func__,
              __LINE__,
              grp_hdl,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  msts = bf_map_get_rmv(
      &ha_hlp_info->idx_to_grp_info, sel_base_idx, (void **)&grp_ha_info);
  if (msts == BF_MAP_NO_KEY) {
    // This group was already processed
    PIPE_MGR_ASSERT(grp_replay_info->matched);
    sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_hdl);
    if (!sel_grp_info) {
      LOG_ERROR("%s:%d get sel group failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }
    sel_grp_info->num_references++;
    return pipe_mgr_sel_update_mat_refs(sel_grp_info, mat_tbl_hdl, mat_ent_hdl);
  } else if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d Unable to retrieve selector group 0x%x in tbl 0x%x",
              __func__,
              __LINE__,
              grp_hdl,
              tbl_hdl);
    return PIPE_UNEXPECTED;
  }

  grp_replay_info->matched = true;
  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_ha_info->grp_hdl);
  if (!sel_grp_info) {
    LOG_ERROR("%s:%d get sel group info failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }
  pipe_mgr_sel_grp_remove_from_htbl(sel_tbl, sel_grp_info->grp_hdl);
  pipe_mgr_sel_grp_remove_from_htbl(sel_tbl, grp_hdl);
  pipe_mgr_sel_grp_add_to_htbl(sel_tbl, grp_hdl, sel_grp_info);
  sel_grp_info->grp_hdl = grp_hdl;
  sel_grp_info->num_references = 1;
  sel_grp_info->grp_id = grp_replay_info->grp_id;

  status = pipe_mgr_sel_update_mat_refs(sel_grp_info, mat_tbl_hdl, mat_ent_hdl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d update sel group info failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }
  if (sel_grp_info->max_grp_size != grp_replay_info->max_grp_size) {
    sel_grp_info->max_grp_size = grp_replay_info->max_grp_size;
    resize = true;
  }
  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
    sel_grp_stage_info =
        pipe_mgr_sel_grp_stage_info_get(sel_tbl, sel_grp_info, sel_stage_info);
    if (!sel_grp_stage_info) {
      LOG_ERROR("%s:%d get sel group stage failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    if (resize) {
      /* For hitless HA, adt ent will not have been allocated yet */
      if (!pipe_mgr_hitless_warm_init_in_progress(device_id)) {
        status = rmt_adt_ent_group_delete(
            device_id,
            sel_tbl_info->adt_tbl_hdl,
            pipe_id,
            sel_stage_info->stage_id,
            sel_grp_stage_info->sel_grp_word_data[0].adt_base_idx,
            sel_grp_stage_info->entries_per_word,
            sel_grp_stage_info->no_words,
            sel_tbl->sess_flags);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s(0x%x - %d) Error deleting adt entry group for selector "
              "group 0x%x stage %d sts 0x%x",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              grp_hdl,
              sel_stage_info->stage_id,
              status);
          return status;
        }
      }

      sel_stage_get_word_info(
          grp_replay_info->max_grp_size, &no_words, &entries_per_word);

      // for planned restarts, the groups should be the same word length
      PIPE_MGR_ASSERT(sel_grp_stage_info->no_words == no_words);

      /* Free old allocation first */
      status = rmt_adt_ent_group_delete(
          device_id,
          sel_tbl_info->adt_tbl_hdl,
          pipe_id,
          sel_stage_info->stage_id,
          sel_grp_stage_info->sel_grp_word_data[0].adt_base_idx,
          sel_grp_stage_info->entries_per_word,
          sel_grp_stage_info->no_words,
          sel_tbl->sess_flags);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(0x%x - %d) Error deleting adt entry group for selector "
            "group stage %d status %s",
            __func__,
            __LINE__,
            sel_tbl_info->name,
            sel_tbl_info->tbl_hdl,
            sel_tbl_info->dev_id,
            sel_stage_info->stage_id,
            pipe_str_err(status));
        goto cleanup;
      }

      adt_base_idx = grp_ha_info->adt_base_idx[stage_idx];
      status = rmt_adt_ent_group_reserve(device_id,
                                         sel_tbl_info->adt_tbl_hdl,
                                         pipe_id,
                                         sel_stage_info->stage_id,
                                         &adt_base_idx,
                                         entries_per_word,
                                         no_words,
                                         sel_tbl->sess_flags);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to reserve adt group with block %d entries "
            "per block %d stage %d dev %d tbl_hdl %#x pipe %d status %s",
            __func__,
            __LINE__,
            no_words,
            entries_per_word,
            sel_stage_info->stage_id,
            device_id,
            tbl_hdl,
            pipe_id,
            pipe_str_err(status));
        goto cleanup;
      }

      power2_allocator_release(sel_stage_info->stage_sel_grp_allocator,
                               sel_base_idx);
      power2_allocator_reserve(
          sel_stage_info->stage_sel_grp_allocator, sel_base_idx, no_words);
      sel_grp_stage_info->no_words = no_words;
      sel_grp_stage_info->entries_per_word = entries_per_word;
      sel_grp_stage_info->grp_size = grp_replay_info->max_grp_size;
      for (i = 0; i < no_words; i++) {
        word_data = &sel_grp_stage_info->sel_grp_word_data[i];
        word_data->word_width = entries_per_word;
        word_data->adt_base_idx = adt_base_idx + (SEL_GRP_WORD_WIDTH * i);
      }
    }
    sel_grp_stage_info->grp_hdl = grp_hdl;
    sel_grp_stage_info->act_fn_hdl = grp_replay_info->act_fn_hdl;
  }

cleanup:
  PIPE_MGR_FREE(grp_ha_info->adt_base_idx);
  PIPE_MGR_FREE(grp_ha_info);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_hlp_update_grp_info(
    sel_tbl_t *sel_tbl,
    sel_grp_stage_info_t *sel_grp_stage_info,
    sel_grp_info_t *sel_grp_info,
    pipe_sel_grp_mbr_hdl_t mbr_hdl,
    bool mbr_enable,
    struct pipe_mgr_sel_move_list_t **move_tail_p) {
  pipe_status_t status = PIPE_SUCCESS;
  sel_grp_mbr_t *grp_mbr = NULL;

  grp_mbr = pipe_mgr_sel_grp_mbr_get(sel_grp_info, mbr_hdl, false);

  if (grp_mbr) {
    grp_mbr->weight++;
  } else {
    /* Add member to selector DB */
    grp_mbr = pipe_mgr_sel_grp_mbr_alloc(mbr_hdl);
    if (!grp_mbr) {
      return PIPE_NO_SYS_RESOURCES;
    }
    status =
        pipe_mgr_sel_mbr_add_to_htbl(sel_grp_info, mbr_hdl, grp_mbr, false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error", __func__, __LINE__);
      return PIPE_INVALID_ARG;
    }
    /* Set the member state */
    pipe_mgr_sel_grp_mbr_set_state(sel_grp_info, mbr_hdl, mbr_enable);
  }

  /* Add member to stage DB */
  status = pipe_mgr_sel_grp_mbr_add_to_stage(sel_tbl,
                                             sel_grp_stage_info,
                                             sel_grp_info,
                                             mbr_hdl,
                                             0,  // num_mbrs
                                             move_tail_p);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in adding member %d to group %d",
              __func__,
              __LINE__,
              mbr_hdl,
              sel_grp_info->grp_hdl);
    if (grp_mbr->weight == 1) {
      pipe_mgr_sel_grp_mbr_remove_and_destroy(sel_grp_info, mbr_hdl, false);
    } else {
      grp_mbr->weight--;
    }
    return status;
  }
  sel_grp_info->mbr_count++;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_hlp_compute_delta_changes(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t **move_head_p) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t msts = BF_MAP_OK;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_hlp_word_data_t *word_data = NULL;
  pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = NULL;
  pipe_mgr_sel_grp_replay_info_t *grp_replay_info = NULL;
  pipe_sel_grp_mbr_hdl_t mbr_hdl;
  uint32_t pipe_idx, stage_idx;
  unsigned long key = 0;
  uint32_t i, j, k;
  void *cookie = NULL;
  bool found_mbr = false;
  bool *mbrs_found = NULL;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel tbl, with tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_sel_move_list_t *move_tail = &move_head;

  for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
    msts = bf_map_get_first(
        &ha_hlp_info->replay_hdl_to_info, &key, (void **)&grp_replay_info);
    while (msts == BF_MAP_OK) {
      if (grp_replay_info->matched) {
        sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_replay_info->grp_hdl);
        if (!sel_grp_info) {
          LOG_ERROR("%s:%d get sel group failed", __func__, __LINE__);
          return PIPE_UNEXPECTED;
        }
        if (mbrs_found) {
          PIPE_MGR_FREE(mbrs_found);
          mbrs_found = NULL;
        }
        if (grp_replay_info->num_mbrs > 0) {
          mbrs_found = PIPE_MGR_CALLOC(grp_replay_info->num_mbrs, sizeof(bool));
          if (!mbrs_found) {
            LOG_ERROR("%s:%d allocation failed", __func__, __LINE__);
            return PIPE_NO_SYS_RESOURCES;
          }

          uint32_t num_mbrs = grp_replay_info->num_mbrs;
          pipe_adt_ent_hdl_t *mbrs =
              PIPE_MGR_CALLOC(num_mbrs, sizeof(pipe_adt_ent_hdl_t));
          bool *enable = PIPE_MGR_CALLOC(num_mbrs, sizeof(bool));
          uint32_t mbrs_populated = 0;

          status = rmt_sel_grp_mbrs_get(device_id,
                                        tbl_hdl,
                                        grp_replay_info->grp_hdl,
                                        num_mbrs,
                                        mbrs,
                                        enable,
                                        &mbrs_populated);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error getting members for sel grp %d"
                "tbl_hdl 0x%x rc 0x%x",
                __func__,
                __LINE__,
                grp_replay_info->grp_hdl,
                tbl_hdl,
                status);
            if (mbrs) PIPE_MGR_FREE(mbrs);
            if (enable) PIPE_MGR_FREE(enable);
            return status;
          }

          for (j = 0; j < mbrs_populated; j++) {
            for (k = 0; k < grp_replay_info->num_mbrs; k++) {
              mbr_hdl = grp_replay_info->mbr_hdls[k];
              if (mbr_hdl == mbrs[j] && !mbrs_found[k]) {
                mbrs_found[k] = true;
                break;
              }
            }
          }
          if (mbrs) PIPE_MGR_FREE(mbrs);
          if (enable) PIPE_MGR_FREE(enable);
        }
        for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
          sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
          sel_grp_stage_info = pipe_mgr_sel_grp_stage_info_get(
              sel_tbl, sel_grp_info, sel_stage_info);
          if (!sel_grp_stage_info) {
            LOG_ERROR("%s:%d get sel group stage failed", __func__, __LINE__);
            status = PIPE_UNEXPECTED;
            goto cleanup;
          }
          if (grp_replay_info->num_mbrs >
              sel_grp_stage_info->entries_per_word) {
            sel_grp_stage_info->mbrs_duplicated = false;

            for (i = 0; i < sel_grp_stage_info->no_words; i++) {
              word_data = &sel_grp_stage_info->sel_grp_word_data[i];
              for (j = 0; j < word_data->word_width; j++) {
                if (!word_data->mbrs[j]) {
                  continue;
                }
                found_mbr = false;
                for (k = 0; k < grp_replay_info->num_mbrs; k++) {
                  mbr_hdl = grp_replay_info->mbr_hdls[k];
                  if (mbr_hdl == word_data->mbrs[j]) {
                    found_mbr = true;
                    grp_replay_info->mbr_hdls[k] = PIPE_ADT_ENT_HDL_INVALID_HDL;
                    break;
                  }
                }
                if (found_mbr) continue;
                for (k = 0; k < grp_replay_info->num_mbrs; k++) {
                  mbr_hdl = grp_replay_info->mbr_hdls[k];
                  if (mbr_hdl == PIPE_ADT_ENT_HDL_INVALID_HDL) continue;
                  if (mbrs_found[k]) continue;
                  if (pipe_mgr_adt_cmp_entries(device_id,
                                               sel_tbl_info->adt_tbl_hdl,
                                               word_data->mbrs[j],
                                               mbr_hdl)) {
                    status = pipe_mgr_mbr_update_identical_in_stage(
                        sel_tbl,
                        sel_grp_stage_info,
                        sel_grp_info,
                        i,
                        j,
                        word_data->mbrs[j],
                        mbr_hdl,
                        &move_tail);
                    if (status != PIPE_SUCCESS) {
                      LOG_ERROR(
                          "%s:%d Error in replacing member %d  with %d in "
                          "group %d in stage %d pipe %d tbl 0x%x",
                          __func__,
                          __LINE__,
                          word_data->mbrs[j],
                          mbr_hdl,
                          sel_grp_info->grp_hdl,
                          sel_stage_info->stage_id,
                          sel_tbl->pipe_id,
                          sel_tbl_info->tbl_hdl);
                      goto cleanup;
                    }
                    found_mbr = true;
                    grp_replay_info->mbr_hdls[k] = PIPE_ADT_ENT_HDL_INVALID_HDL;
                    break;
                  }
                }
                if (!found_mbr) {
                  mbr_hdl = word_data->mbrs[j];
                  status = mbr_del_from_stage(sel_tbl,
                                              sel_grp_stage_info,
                                              sel_grp_info,
                                              mbr_hdl,
                                              cookie,
                                              &move_tail);
                  if (status != PIPE_SUCCESS) {
                    LOG_ERROR(
                        "%s:%d Error in deleting member %d from group %d "
                        "from stage %d pipe %d tbl 0x%x",
                        __func__,
                        __LINE__,
                        mbr_hdl,
                        sel_grp_info->grp_hdl,
                        sel_stage_info->stage_id,
                        sel_tbl->pipe_id,
                        sel_tbl_info->tbl_hdl);
                    goto cleanup;
                  }
                }
              }
            }
            for (k = 0; k < grp_replay_info->num_mbrs; k++) {
              mbr_hdl = grp_replay_info->mbr_hdls[k];
              if (mbr_hdl != PIPE_ADT_ENT_HDL_INVALID_HDL) {
                status = pipe_mgr_sel_hlp_update_grp_info(
                    sel_tbl,
                    sel_grp_stage_info,
                    sel_grp_info,
                    mbr_hdl,
                    grp_replay_info->mbr_enable[k],
                    &move_tail);
                if (status != PIPE_SUCCESS) {
                  LOG_ERROR("%s:%d Error", __func__, __LINE__);
                  goto cleanup;
                }
              }
            }
          } else {
            sel_grp_stage_info->mbrs_duplicated = true;
            word_data = &sel_grp_stage_info->sel_grp_word_data[0];
            for (j = 0; j < word_data->word_width; j++) {
              if (!word_data->mbrs[j]) {
                continue;
              }
              found_mbr = false;
              for (k = 0; k < grp_replay_info->num_mbrs; k++) {
                mbr_hdl = grp_replay_info->mbr_hdls[k];
                if (mbr_hdl == word_data->mbrs[j]) {
                  found_mbr = true;
                  grp_replay_info->mbr_hdls[k] = PIPE_ADT_ENT_HDL_INVALID_HDL;
                  break;
                }
              }
              if (found_mbr) continue;
              for (k = 0; k < grp_replay_info->num_mbrs; k++) {
                mbr_hdl = grp_replay_info->mbr_hdls[k];
                if (mbr_hdl == PIPE_ADT_ENT_HDL_INVALID_HDL) continue;
                if (mbrs_found[k]) continue;
                if (pipe_mgr_adt_cmp_entries(device_id,
                                             sel_tbl_info->adt_tbl_hdl,
                                             word_data->mbrs[j],
                                             mbr_hdl)) {
                  status =
                      pipe_mgr_mbr_update_identical_in_stage(sel_tbl,
                                                             sel_grp_stage_info,
                                                             sel_grp_info,
                                                             0,
                                                             j,
                                                             word_data->mbrs[j],
                                                             mbr_hdl,
                                                             &move_tail);
                  if (status != PIPE_SUCCESS) {
                    LOG_ERROR(
                        "%s:%d Error in replacing member %d  with %d in "
                        "group %d in stage %d pipe %d tbl 0x%x",
                        __func__,
                        __LINE__,
                        word_data->mbrs[j],
                        mbr_hdl,
                        sel_grp_info->grp_hdl,
                        sel_stage_info->stage_id,
                        sel_tbl->pipe_id,
                        sel_tbl_info->tbl_hdl);
                    goto cleanup;
                  }
                  found_mbr = true;
                  grp_replay_info->mbr_hdls[k] = PIPE_ADT_ENT_HDL_INVALID_HDL;
                  break;
                }
              }
              if (!found_mbr) {
                mbr_hdl = word_data->mbrs[j];
                status = mbr_del_from_stage(sel_tbl,
                                            sel_grp_stage_info,
                                            sel_grp_info,
                                            mbr_hdl,
                                            cookie,
                                            &move_tail);
                if (status != PIPE_SUCCESS) {
                  LOG_ERROR(
                      "%s:%d Error in deleting member %d from group %d "
                      "from stage %d pipe %d tbl 0x%x",
                      __func__,
                      __LINE__,
                      mbr_hdl,
                      sel_grp_info->grp_hdl,
                      sel_stage_info->stage_id,
                      sel_tbl->pipe_id,
                      sel_tbl_info->tbl_hdl);
                  goto cleanup;
                }
              }
            }
            for (k = 0; k < grp_replay_info->num_mbrs; k++) {
              mbr_hdl = grp_replay_info->mbr_hdls[k];
              if (mbr_hdl != PIPE_ADT_ENT_HDL_INVALID_HDL) {
                status = pipe_mgr_sel_hlp_update_grp_info(
                    sel_tbl,
                    sel_grp_stage_info,
                    sel_grp_info,
                    mbr_hdl,
                    grp_replay_info->mbr_enable[k],
                    &move_tail);
                if (status != PIPE_SUCCESS) {
                  LOG_ERROR("%s:%d Error", __func__, __LINE__);
                  goto cleanup;
                }
              }
            }
          }
        }
      } else {
        struct pipe_mgr_sel_move_list_t *mh = NULL;
        pipe_mgr_sel_grp_remove_from_htbl(sel_tbl, grp_replay_info->grp_hdl);
        if (sel_tbl_info->cache_id) {
          bf_map_sts_t sts =
              bf_map_rmv(&sel_tbl->grp_id_map, grp_replay_info->grp_id);
          if (sts != BF_MAP_OK) {
            LOG_ERROR(
                "%s:%d Error removing group %d from selector table map rc 0x%x",
                __func__,
                __LINE__,
                grp_replay_info->grp_id,
                sts);
          }
        }
        // add_internal will re-add new group to grp_id_map
        status = pipe_mgr_sel_grp_add_internal(sel_tbl,
                                               sel_tbl->pipe_id,
                                               grp_replay_info->grp_id,
                                               grp_replay_info->max_grp_size,
                                               grp_replay_info->grp_hdl,
                                               PIPE_MGR_LOGICAL_ACT_IDX_INVALID,
                                               &mh);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Selector table group add failed for dev %d "
              "tbl_hdl 0x%x status 0x%x",
              __func__,
              __LINE__,
              device_id,
              sel_tbl_info->tbl_hdl,
              status);
          goto cleanup;
        }
        move_tail->next = mh;
        while (move_tail->next) {
          move_tail = move_tail->next;
        }

        sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, grp_replay_info->grp_hdl);
        PIPE_MGR_ASSERT(sel_grp_info);
        /* Set the action function handle */
        if (!sel_grp_info->act_fn_set) {
          sel_grp_info->act_fn_hdl = grp_replay_info->act_fn_hdl;
          sel_grp_info->act_fn_set = true;
        }

        for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
          sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
          sel_grp_stage_info = pipe_mgr_sel_grp_stage_info_get(
              sel_tbl, sel_grp_info, sel_stage_info);
          PIPE_MGR_ASSERT(sel_grp_stage_info != NULL);
          sel_grp_stage_info->act_fn_hdl = sel_grp_info->act_fn_hdl;

          for (i = 0; i < grp_replay_info->num_mbrs; i++) {
            mbr_hdl = grp_replay_info->mbr_hdls[i];

            status =
                pipe_mgr_sel_hlp_update_grp_info(sel_tbl,
                                                 sel_grp_stage_info,
                                                 sel_grp_info,
                                                 mbr_hdl,
                                                 grp_replay_info->mbr_enable[i],
                                                 &move_tail);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR("%s:%d Error", __func__, __LINE__);
              goto cleanup;
            }
          }
        }
      }
      if (pipe_mgr_sel_update_active_mbr_count(sel_grp_info)) {
        status = PIPE_UNEXPECTED;
        goto cleanup;
      }
      msts = bf_map_get_next(
          &ha_hlp_info->replay_hdl_to_info, &key, (void **)&grp_replay_info);
    }
  }

  *move_head_p = move_head.next;

cleanup:
  if (mbrs_found) PIPE_MGR_FREE(mbrs_found);
  return status;
}

static void destroy_grp_ha_info(pipe_mgr_sel_grp_ha_info_t *grp_ha_info) {
  if (!grp_ha_info) {
    return;
  }
  if (grp_ha_info->adt_base_idx) {
    PIPE_MGR_FREE(grp_ha_info->adt_base_idx);
  }
  PIPE_MGR_FREE(grp_ha_info);
  return;
}

static void destroy_grp_replay_info(
    pipe_mgr_sel_grp_replay_info_t *grp_replay_info) {
  if (!grp_replay_info) {
    return;
  }
  if (grp_replay_info->mbr_hdls) {
    PIPE_MGR_FREE(grp_replay_info->mbr_hdls);
  }
  if (grp_replay_info->mbr_enable) {
    PIPE_MGR_FREE(grp_replay_info->mbr_enable);
  }
  if (grp_replay_info->mbr_weight) {
    PIPE_MGR_FREE(grp_replay_info->mbr_weight);
  }
  PIPE_MGR_FREE(grp_replay_info);
  return;
}

void pipe_mgr_selector_cleanup_pipe_hlp_ha_state(sel_tbl_t *sel_tbl) {
  if (!sel_tbl->hlp.ha_hlp_info) {
    return;
  }
  pipe_mgr_sel_pipe_ha_hlp_info_t *ha_hlp_info = sel_tbl->hlp.ha_hlp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info = NULL;
  unsigned long key = 0;
  /* Cleanup idx_to_grp_info map */
  while ((map_sts = bf_map_get_first_rmv(
              &ha_hlp_info->idx_to_grp_info, &key, (void **)&grp_ha_info)) ==
         BF_MAP_OK) {
    destroy_grp_ha_info(grp_ha_info);
  }
  bf_map_destroy(&ha_hlp_info->idx_to_grp_info);

  /* Cleanup replay_hdl_to_info map */
  pipe_mgr_sel_grp_replay_info_t *grp_replay_info = NULL;
  while ((map_sts = bf_map_get_first_rmv(&ha_hlp_info->replay_hdl_to_info,
                                         &key,
                                         (void **)&grp_replay_info)) ==
         BF_MAP_OK) {
    destroy_grp_replay_info(grp_replay_info);
  }
  bf_map_destroy(&ha_hlp_info->replay_hdl_to_info);

  PIPE_MGR_FREE(sel_tbl->hlp.ha_hlp_info);
  sel_tbl->hlp.ha_hlp_info = NULL;

  return;
}

void pipe_mgr_selector_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            bool is_backup) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, sel_tbl_hdl, is_backup);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Selector table 0x%x, device id %d not found",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              device_id);
    return;
  }
  unsigned tbl_idx = 0;
  for (tbl_idx = 0; tbl_idx < sel_tbl_info->no_sel_tbls; tbl_idx++) {
    sel_tbl_t *sel_tbl = &sel_tbl_info->sel_tbl[tbl_idx];
    pipe_mgr_selector_cleanup_pipe_hlp_ha_state(sel_tbl);
  }
  return;
}
