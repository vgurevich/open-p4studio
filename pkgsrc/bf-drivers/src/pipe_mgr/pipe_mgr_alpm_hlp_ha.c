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
 * @file pipe_mgr_alpm_hlp_ha.c
 * @date
 *
 * Implementation of ALPM HA management for HLP
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <lld/lld_inst_list_fmt.h>

#include <target-utils/fbitset/fbitset.h>

/* Local header files */
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_alpm_ha.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hitless_ha.h"

uint32_t cnt_restore_add;       // add to cp list (scale-opt)
uint32_t cnt_restore_proc;      // proccess cp entry
uint32_t cnt_restore_add_spec;  // process normal entry

pipe_status_t pipe_mgr_alpm_hlp_restore_state(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl) {
  alpm_tbl_info_t *tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    tbl_info->is_cp_restore = true;
    pipe_mgr_move_list_t *move_node = tbl_info->cp_ml_head;
    while (move_node) {
      if (pipe_mgr_alpm_hlp_entry_restore(
              dev_id, tbl_info->atcam_tbl_hdl, move_node)) {
        LOG_ERROR(
            "%s:%d ALPM covering prefix restore failure for tbl 0x%x,"
            "atcam 0x%x",
            __func__,
            __LINE__,
            tbl_hdl,
            tbl_info->atcam_tbl_hdl);
      }
      pipe_mgr_move_list_t *mn = move_node;
      move_node = move_node->next;
      PIPE_MGR_FREE(mn);
    }
    tbl_info->cp_ml_head = tbl_info->cp_ml_tail = NULL;
    tbl_info->is_cp_restore = false;
  }
  LOG_DBG("ALPM hitless counters:");
  LOG_DBG("\tcov pfx add %u", cnt_restore_proc);
  LOG_DBG("\tcov pfx restore add %u", cnt_restore_add);
  LOG_DBG("\tmatch spec restore add %u", cnt_restore_add_spec);
  cnt_restore_add = cnt_restore_add_spec = cnt_restore_proc = 0;
  pipe_mgr_alpm_restore_cp(tbl_info);
  return PIPE_SUCCESS;
}

/* Function used to rebuild match_spec based on read atcam values and
 * preclassifier entry. Required in scale opt ALPM, since only partial
 * match_spec is stored in atcam.
 */
static pipe_status_t restore_full_match_spec(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_tbl_match_spec_t *match_spec,
    pipe_tbl_match_spec_t **full_match_spec) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  uint32_t subtree_root_depth = subtree->node->depth;
  if (subtree_root_depth) {
    *full_match_spec = pipe_mgr_tbl_copy_match_spec(
        *full_match_spec, pipe_tbl->match_spec_template);
    if (*full_match_spec == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x - malloc failure",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_id,
                subtree->node->entry->sram_entry_hdl);
      return PIPE_NO_SYS_RESOURCES;
    }
    pipe_action_spec_t *preclass_action_spec = NULL;
    preclass_action_spec = pipe_mgr_tbl_copy_action_spec(
        preclass_action_spec, pipe_tbl->act_spec_template);
    if (preclass_action_spec == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) malloc failure",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_id);
      return PIPE_NO_SYS_RESOURCES;
    }
    dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = pipe_id};
    pipe_act_fn_hdl_t preclass_act_fn_hdl;
    pipe_status_t sts = pipe_mgr_tcam_get_entry(tbl_info->preclass_tbl_hdl,
                                                dev_tgt,
                                                subtree->tcam_entry_hdl,
                                                *full_match_spec,
                                                preclass_action_spec,
                                                &preclass_act_fn_hdl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Failed to get entry 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_id,
                subtree->node->entry->sram_entry_hdl);
      pipe_mgr_tbl_destroy_match_spec(full_match_spec);
      return sts;
    }
    (*full_match_spec)->partition_index = 0;
    // Get atcam match spec and merge both preclass and atcam match spec
    if (subtree_root_depth >
        (tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1)) {
      subtree_root_depth =
          tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1;
    }
    /*
     * Build the full match spec from preclassifier match spec and
     * ATCAM subset key match spec. This step needs to be bypassed
     * if prefix length is 0 for ATCAM subset key width optimization as
     * ATCAM portion wouldn't have any valid bits.
     */
    if (tbl_info->atcam_subset_key_width &&
        subtree_root_depth > tbl_info->exm_fields_key_width) {
      /* ATCAM subset key width optimization case and prefix length > 0 */
      if (subtree_root_depth >
          (tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1)) {
        subtree_root_depth =
            tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1;
      }
      build_alpm_full_mspec(
          tbl_info, *full_match_spec, match_spec, subtree_root_depth);
    } else if (tbl_info->num_excluded_bits) {
      /* Exclude MSB bits optimization case */
      build_alpm_exclude_msb_bits_full_mspec(
          tbl_info, match_spec, *full_match_spec);
    }
  }
  return PIPE_SUCCESS;
}

/* Function used to store covering prefix node list. In scale opt case
 * covering prefix entries must be restored at the very end in order
 * to be always able to match them with original entry. */
static pipe_status_t add_to_cp_ml(alpm_tbl_info_t *tbl_info,
                                  pipe_mgr_move_list_t *move_node) {
  pipe_mgr_move_list_t *cp_ml_node =
      PIPE_MGR_MALLOC(sizeof(pipe_mgr_move_list_t));
  if (!cp_ml_node) return PIPE_NO_SYS_RESOURCES;
  *cp_ml_node = *move_node;
  cp_ml_node->next = NULL;
  if (tbl_info->cp_ml_tail) {
    tbl_info->cp_ml_tail->next = cp_ml_node;
  }
  tbl_info->cp_ml_tail = cp_ml_node;
  if (tbl_info->cp_ml_head == NULL) tbl_info->cp_ml_head = cp_ml_node;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_hlp_entry_restore(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t ll_tbl_hdl,
                                              pipe_mgr_move_list_t *move_node) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node = NULL;
  trie_subtree_t *subtree = NULL;
  alpm_entry_t *ent_info;
  hdl_info_t *hdl_info, *cp_hdl_info;
  cp_restore_t *cp_restore;
  pipe_tbl_match_spec_t *match_spec;
  pipe_tbl_match_spec_t *full_match_spec = NULL;
  pipe_action_spec_t *action_spec;
  pipe_act_fn_hdl_t act_fn_hdl;
  bool is_preclass = false;
  bf_dev_pipe_t pipe_id;
  uint32_t pipe_idx;
  partition_info_t *p_info;

  tbl_info = pipe_mgr_alpm_tbl_info_get_from_ll_hdl(dev_id, ll_tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR(
        "%s:%d Could not get the ALPM match table info for lower level table "
        " with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        ll_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  is_preclass = (ll_tbl_hdl == tbl_info->preclass_tbl_hdl);

  pipe_id = get_move_list_pipe(move_node);
  if (pipe_id == BF_DEV_PIPE_ALL) {
    PIPE_MGR_ASSERT(tbl_info->is_symmetric);
    pipe_tbl = tbl_info->pipe_tbls[0];
  } else {
    for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
      pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
      if (pipe_tbl->pipe_id == pipe_id) {
        break;
      }
    }
    if (pipe_idx == tbl_info->num_pipes) {
      LOG_ERROR(
          "%s:%d Could not get the ALPM pipe tbl for table 0x%x pipe %d device "
          "id %d",
          __func__,
          __LINE__,
          tbl_info->alpm_tbl_hdl,
          pipe_id,
          dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
    PIPE_MGR_DBGCHK(is_preclass == false);
    if (!is_preclass) {
      pipe_tbl->default_atcam_ent_hdl = move_node->entry_hdl;
      PIPE_MGR_ASSERT(!pipe_tbl->default_alpm_ent_hdl);
      pipe_tbl->default_alpm_ent_hdl = PIPE_ALPM_DEFAULT_ENT_HDL;
      if (pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
        pipe_tbl->default_alpm_ent_hdl = PIPE_SET_HDL_PIPE(
            pipe_tbl->default_alpm_ent_hdl, pipe_tbl->pipe_id);
      }
      hdl_info = PIPE_MGR_MALLOC(sizeof(hdl_info_t));
      hdl_info->alpm_hdl = pipe_tbl->default_alpm_ent_hdl;
      hdl_info->cp_hdl = 0;
      bf_map_add(&pipe_tbl->atcam_entry_hdl_map,
                 pipe_tbl->default_atcam_ent_hdl,
                 (void *)hdl_info);
    }
    return PIPE_SUCCESS;
  }

  PIPE_MGR_ASSERT(move_node->op == PIPE_MAT_UPDATE_ADD);

  match_spec = unpack_mat_ent_data_ms(move_node->data);
  action_spec = unpack_mat_ent_data_as(move_node->data);
  act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_node->data);

  if (is_preclass) {
    // Assuming preclass will be restored first.
    return pipe_mgr_alpm_preclass_restore_int(
        tbl_info, pipe_tbl, move_node->entry_hdl, match_spec, action_spec);
  }

  // Not a preclass table

  // Find right partition
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    p_info = &pipe_tbl->partitions[match_spec->partition_index - 1];
  } else {
    p_info = &pipe_tbl->partitions[match_spec->partition_index];
  }
  if (!p_info) {
    LOG_ERROR(
        "%s:%d Error in fetching subtree partition info", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint8_t subtree_id = 0;
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    // Scale optimization case, need to restore full match spec
    if (tbl_info->max_subtrees_per_partition > 1) {
      subtree_id = match_spec->match_value_bits[0];
      // Cp entries are marked by version bits
      if (match_spec->version_bits == tbl_info->cp_ver_bits) {
        // If not cp restore yet, postpone till all non-cp entries get processed
        if (!tbl_info->is_cp_restore) {
          if (add_to_cp_ml(tbl_info, move_node)) {
            return PIPE_NO_SYS_RESOURCES;
          }
          cnt_restore_add++;
          return PIPE_SUCCESS;
        }
      }
    }

    // Find right subtree root for current entry
    for (uint32_t i = 0; i < p_info->num_subtrees; i++) {
      if (subtree_id == p_info->subtree_nodes[i]->subtree->subtree_id) {
        subtree = p_info->subtree_nodes[i]->subtree;
        break;
      }
    }

    // No subtree mean there is no preclassifier entry present and restore
    // cannot proceed
    if (!subtree) {
      LOG_ERROR(
          "%s:%d Error getting subtree info, tbl %s (0x%x) dev %d pipe %X",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          tbl_info->dev_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    // Restore full match_spec in case of optimization enabled
    sts = restore_full_match_spec(
        dev_id, pipe_id, pipe_tbl, subtree, match_spec, &full_match_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to restore full match spec in alpm tbl %s (0x%x) "
          "device %d",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          dev_id);
      return sts;
    }

    node = find_node(pipe_tbl,
                     full_match_spec,
                     tbl_info->is_cp_restore,
                     &subtree,
                     NULL,
                     NULL);
  } else {
    /* For non-atcam subset key width handling */
    node = find_node(pipe_tbl, match_spec, false, &subtree, NULL, NULL);
  }
  if (!subtree) {
    LOG_ERROR("%s:%d Error getting subtree info, tbl %s (0x%x) dev %d pipe %X",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl,
              tbl_info->dev_id,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!node) {
    LOG_ERROR("%s:%d Error getting table node, tbl %s (0x%x) dev %d pipe %X",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl,
              tbl_info->dev_id,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  hdl_info = PIPE_MGR_CALLOC(1, sizeof(hdl_info_t));
  if (node->entry) {
    // If covering prefix was already processed from different partition/subtree
    // entry already exists. All CP handling will end up here in scale-opt case,
    // since original entries should be in place.
    if (ALPM_IS_SCALE_OPT_ENB(tbl_info) && tbl_info->is_cp_restore == false) {
      LOG_ERROR(
          "%s:%d Normal entry is processed as covering prefix, will be"
          "missed on lookup during replay",
          __func__,
          __LINE__);
      PIPE_MGR_DBGCHK(0);
    }
    ++cnt_restore_proc;
    hdl_info->alpm_hdl = node->entry->alpm_entry_hdl;
    hdl_info->cp_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);
    bf_map_add(
        &pipe_tbl->atcam_entry_hdl_map, move_node->entry_hdl, (void *)hdl_info);
    return pipe_mgr_alpm_atcam_restore_int(tbl_info,
                                           pipe_tbl,
                                           move_node->entry_hdl,
                                           node,
                                           p_info,
                                           subtree_id,
                                           match_spec,
                                           act_fn_hdl,
                                           action_spec,
                                           true);
  }

  if (tbl_info->is_cp_restore || subtree_id != subtree->subtree_id ||
      match_spec->partition_index != subtree->partition->ptn_index) {
    if (ALPM_IS_SCALE_OPT_ENB(tbl_info) && tbl_info->is_cp_restore == false) {
      LOG_ERROR(
          "%s:%d Normal entry is processed as covering prefix, will be"
          "missed on lookup during replay",
          __func__,
          __LINE__);
      PIPE_MGR_DBGCHK(0);
    }
    // Covering prefix first time found case, this happens on non-scale-opt
    // case, as cp entry is detected by different partition number.
    ++cnt_restore_proc;
    if (node->cov_pfx_arr_size > 0) {
      // We've already found a covering prefix for this node, and the alpm
      // handle has already been allocated
      cp_restore = (cp_restore_t *)node->cov_pfx_subtree_nodes[0];
      bf_map_get(&pipe_tbl->atcam_entry_hdl_map,
                 cp_restore->ent_hdl,
                 (void **)&cp_hdl_info);
      hdl_info->alpm_hdl = cp_hdl_info->alpm_hdl;
    } else {
      hdl_info->alpm_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);
    }
    hdl_info->cp_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);
    bf_map_add(
        &pipe_tbl->atcam_entry_hdl_map, move_node->entry_hdl, (void *)hdl_info);
    return pipe_mgr_alpm_atcam_restore_int(tbl_info,
                                           pipe_tbl,
                                           move_node->entry_hdl,
                                           node,
                                           p_info,
                                           subtree_id,
                                           match_spec,
                                           act_fn_hdl,
                                           action_spec,
                                           true);
  } else {
    // Actual entry case
    ent_info = PIPE_MGR_CALLOC(1, sizeof(alpm_entry_t));
    if (!ent_info) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    if (node->cov_pfx_arr_size > 0) {
      // We've already found a covering prefix for this node, and the alpm
      // handle has already been allocated
      cp_restore = (cp_restore_t *)node->cov_pfx_subtree_nodes[0];
      bf_map_get(&pipe_tbl->atcam_entry_hdl_map,
                 cp_restore->ent_hdl,
                 (void **)&cp_hdl_info);
      ent_info->alpm_entry_hdl = cp_hdl_info->alpm_hdl;
    } else {
      ent_info->alpm_entry_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);
    }
    hdl_info->alpm_hdl = ent_info->alpm_entry_hdl;
    hdl_info->cp_hdl = 0;
    bf_map_add(
        &pipe_tbl->atcam_entry_hdl_map, move_node->entry_hdl, (void *)hdl_info);

    ent_info->sram_entry_hdl = move_node->entry_hdl;
    ent_info->ttl = unpack_mat_ent_data_ttl(move_node->data);
    node->entry = ent_info;

    sts = pipe_mgr_alpm_atcam_restore_int(tbl_info,
                                          pipe_tbl,
                                          move_node->entry_hdl,
                                          node,
                                          p_info,
                                          subtree_id,
                                          match_spec,
                                          act_fn_hdl,
                                          action_spec,
                                          false);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to restore alpm entry for atcam entry %d in alpm "
          "tbl 0x%x device %d",
          __func__,
          __LINE__,
          move_node->entry_hdl,
          tbl_info->alpm_tbl_hdl,
          dev_id);
      return sts;
    }
    // Add it to the match-entry-map hash-table keyed by match-spec
    if (!pipe_mgr_is_device_virtual(dev_id)) {
      /* The spec map is populated only for non-virtual devices, since for
       * virtual device the state restore happens through move-lists and
       * any deltas are then replayed in the form of regular APIs outside
       * of warm-init begin and warm-init end
       */
      move_node->entry_hdl = ent_info->alpm_entry_hdl;
      /* For scale optimization replace match_spec with full version, so
       * replay will work. */
      if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
        match_spec->match_value_bits = match_spec->match_mask_bits = NULL;
        if (!pipe_mgr_tbl_copy_match_spec(match_spec, full_match_spec)) {
          LOG_ERROR("%s:%d Error coping mspec entry with hdl %d",
                    __func__,
                    __LINE__,
                    move_node->entry_hdl);
          return PIPE_NO_SYS_RESOURCES;
        }
      }
      move_node->data->match_spec.partition_index = 0;
      // Priority must be zero because lookup functions for most tables cannot
      // properly restore priorty just from HW read.
      move_node->data->match_spec.priority = 0;

      sts = pipe_mgr_hitless_ha_new_spec(&pipe_tbl->spec_map, move_node);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error adding alpm entry with hdl %d to match entry "
            "map "
            "in tbl 0x%x pipe %d device %d",
            __func__,
            __LINE__,
            move_node->entry_hdl,
            tbl_info->alpm_tbl_hdl,
            pipe_tbl->pipe_id,
            dev_id);
        return sts;
      }
      ++cnt_restore_add_spec;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  alpm_tbl_info_t *alpm_tbl_info = NULL;
  alpm_pipe_tbl_t *alpm_pipe_tbl = NULL;
  uint32_t i = 0;

  alpm_tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (alpm_tbl_info == NULL) {
    LOG_ERROR("%s:%d Alpm table not found for handle 0x%x device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) &&
      (!alpm_tbl_info->is_symmetric)) {
    LOG_ERROR(
        "%s:%d Alpm tbl 0x%x, Invalid pipe %d specified for asymmetric tbl, "
        "dev "
        "%d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) &&
      (alpm_tbl_info->is_symmetric)) {
    LOG_ERROR(
        "%s:%d Alpm tbl 0x%x, Invalid pipe %d specified for symmetric tbl, "
        "dev "
        "%d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (alpm_tbl_info->is_symmetric) {
    alpm_pipe_tbl = alpm_tbl_info->pipe_tbls[0];
  } else {
    for (i = 0; i < alpm_tbl_info->num_pipes; i++) {
      alpm_pipe_tbl = alpm_tbl_info->pipe_tbls[i];
      if (alpm_pipe_tbl->pipe_id == dev_tgt.dev_pipe_id) {
        break;
      }
    }
    if (i == alpm_tbl_info->num_pipes) {
      LOG_ERROR("%s:%d Pipe table with id %d not found",
                __func__,
                __LINE__,
                dev_tgt.dev_pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  PIPE_MGR_MEMCPY(ha_report,
                  &alpm_pipe_tbl->ha_reconc_report,
                  sizeof(pipe_tbl_ha_reconc_report_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  alpm_tbl_info_t *alpm_tbl_info = NULL;
  alpm_pipe_tbl_t *alpm_pipe_tbl = NULL;
  bf_dev_pipe_t pipe_id = 0;

  alpm_tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (alpm_tbl_info == NULL) {
    LOG_ERROR("%s:%d Alpm table not found for handle 0x%x device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;
  struct pipe_mgr_move_list_t *mh = NULL;

  pipe_mat_tbl_info_t *tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_id = 0; pipe_id < alpm_tbl_info->num_pipes; pipe_id++) {
    alpm_pipe_tbl = alpm_tbl_info->pipe_tbls[pipe_id];

    rc = pipe_mgr_hitless_ha_reconcile(&alpm_pipe_tbl->spec_map,
                                       &mh,
                                       tbl_info,
                                       &alpm_pipe_tbl->ha_reconc_report);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error reconciling HA state rc 0x%x ",
          __func__,
          __LINE__,
          alpm_tbl_info->name,
          alpm_tbl_info->dev_id,
          alpm_tbl_info->alpm_tbl_hdl,
          rc);
      *move_head_p = NULL;
      return rc;
    }
    move_tail->next = mh;
    mh = NULL;
    move_tail = move_tail->next;
  }

  *move_head_p = move_head.next;

  return rc;
}

void pipe_mgr_alpm_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  // TODO
  (void)device_id;
  (void)mat_tbl_hdl;
}
