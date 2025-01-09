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
 * @file pipe_mgr_exm_hlp_ha.c
 * @date
 * This file handles the task of restoring HLP state by consuming LLP generated
 * move-lists. Only ENTRY ADD type of move-list is expected. The state is
 * rebuilt by calling the standard state update function for entry add.
 */

/* Standard header includes */
#include <stdio.h>
#include <stdint.h>

/* Module header includes */
#include "target-utils/third-party/judy-1.0.5/src/Judy.h"
#include "pipe_mgr/pipe_mgr_intf.h"
#include "dvm/bf_dma_types.h"
#include "lld/bf_dma_if.h"
#include "lld/bf_dma_if.h"

/* Local header includes */
#include "pipe_mgr_log.h"
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_move_list.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_select_ha.h"

pipe_status_t pipe_mgr_exm_hlp_restore_state(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mgr_move_list_t *move_list,
                                             uint32_t *success_count) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  bool get_adt_cookie = true;
  pipe_mgr_adt_ha_cookie_t adt_ha_cookie = {0};
  pipe_adt_tbl_hdl_t adt_tbl_hdl;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_move_list_t *move_node = NULL;
  bf_dev_pipe_t prev_pipe_id = 0;
  for (move_node = move_list; move_node; move_node = move_node->next) {
    bf_dev_pipe_t pipe_id;
    dev_stage_t stage_id = 0;
    pipe_tbl_match_spec_t *match_spec = unpack_mat_ent_data_ms(move_node->data);
    pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(move_node->data);
    pipe_act_fn_hdl_t act_fn_hdl =
        unpack_mat_ent_data_afun_hdl(move_node->data);
    pipe_idx_t logical_idx = move_node->u.single.logical_idx;

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
          mat_tbl_hdl,
          dev_id);
      status = PIPE_INVALID_ARG;
      break;
    }
    pipe_id = get_move_list_pipe(move_node);
    if (exm_tbl->symmetric) {
      if (pipe_id != BF_DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Incorrect pipe id %d passed for a symmetric table 0x%x, "
            "device id %d",
            __func__,
            __LINE__,
            pipe_id,
            mat_tbl_hdl,
            dev_id);
        status = PIPE_INVALID_ARG;
        break;
      } else {
        if ((prev_pipe_id != pipe_id) || !exm_tbl_data) {
          exm_tbl_data = &exm_tbl->exm_tbl_data[0];
          exm_stage_info = NULL;
          prev_pipe_id = pipe_id;
        }
      }
    } else {
      /* Do some pipe-id validation */
      if ((prev_pipe_id != pipe_id) || !exm_tbl_data) {
        exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
        if (!exm_tbl_data) {
          LOG_ERROR("%s:%d Invalid pipe id %d for tbl 0x%x, device id %d",
                    __func__,
                    __LINE__,
                    pipe_id,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id);
          status = PIPE_INVALID_ARG;
          break;
        }
        exm_stage_info = NULL;
        prev_pipe_id = pipe_id;
      }
    }
    if (!exm_stage_info) {
      if (move_node->op == PIPE_MAT_UPDATE_ADD) {
        stage_id = pipe_mgr_exm_get_stage_id_from_idx(
            exm_tbl_data, move_node->u.single.logical_idx);
      } else if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
        /* For default entry add, the stage id is the last stage in which the
         * table is present.
         */
        stage_id =
            exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1].stage_id;
      }
      exm_stage_info =
          pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
      if (!exm_stage_info) {
        LOG_ERROR(
            "%s:%d Exm tbl stage info for tbl 0x%x, pipe id %d, stage id %d "
            "not found",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            pipe_id,
            stage_id);
        status = PIPE_OBJ_NOT_FOUND;
        break;
      }
      get_adt_cookie = true;
    } else {
      if (move_node->op == PIPE_MAT_UPDATE_ADD) {
        if (!(logical_idx >= exm_stage_info->stage_offset &&
              logical_idx < (exm_stage_info->stage_offset +
                             exm_stage_info->num_entries))) {
          /* The logical index is not within the stage, hence a new stage id
           * and info are looked up.
           */
          stage_id =
              pipe_mgr_exm_get_stage_id_from_idx(exm_tbl_data, logical_idx);
          exm_stage_info =
              pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
          get_adt_cookie = true;
        } else {
          stage_id = exm_stage_info->stage_id;
        }
      } else if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
        /* For default entry add, the stage id is the last stage in which the
         * table is present.
         */
        stage_id =
            exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1].stage_id;
        exm_stage_info =
            pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
        get_adt_cookie = true;
      }
      if (!exm_stage_info) {
        LOG_ERROR(
            "%s:%d Exm tbl stage info for tbl 0x%x pipe id %d stage id %d "
            "not found",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            pipe_id,
            stage_id);
        break;
      }
    }

    cuckoo_move_list_t cuckoo_move_node;
    cuckoo_move_node.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
    cuckoo_move_node.logical_action_idx = move_node->logical_action_idx;
    cuckoo_move_node.logical_sel_idx = move_node->logical_sel_idx;
    cuckoo_move_node.selector_len = move_node->selector_len;
    cuckoo_move_node.next = NULL;
    cuckoo_move_node.ttl = 0;
    cuckoo_move_node.proxy_hash = 0;

    if (move_node->op == PIPE_MAT_UPDATE_ADD) {
      pipe_mat_ent_idx_t stage_entry_idx =
          logical_idx - exm_stage_info->stage_offset;
      cuckoo_move_node.dst_entry = stage_entry_idx;
      if (!exm_tbl->hash_action) {
        /* Cuckoo edge state update not applicable for hash-action tables */
        status = pipe_mgr_exm_update_cuckoo_edge_state(exm_stage_info,
                                                       &cuckoo_move_node);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in updating cuckoo edge state for entry hdl %d, tbl "
              "0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              move_node->entry_hdl,
              mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          break;
        }
      }
    } else if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
      cuckoo_move_node.dst_entry = exm_stage_info->default_miss_entry_idx;
    }

    status = pipe_mgr_exm_update_state_for_new_entry(exm_tbl,
                                                     move_node->entry_hdl,
                                                     match_spec,
                                                     action_spec,
                                                     act_fn_hdl,
                                                     &cuckoo_move_node,
                                                     move_node,
                                                     cuckoo_move_node.dst_entry,
                                                     exm_tbl_data,
                                                     exm_stage_info,
                                                     false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error updating state for entry hdl %d, tbl 0x%x, pipe id %d, "
          "stage id %d, err %s",
          __func__,
          __LINE__,
          move_node->entry_hdl,
          mat_tbl_hdl,
          pipe_id,
          stage_id,
          pipe_str_err(status));
      break;
    }
    /* Set the entry handle */
    pipe_mgr_exm_set_ent_hdl(exm_tbl_data, move_node->entry_hdl);
    if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
      pipe_mgr_exm_set_def_ent_placed(exm_tbl_data);
    }
    /* If the match table refers to an action table indirectly, need to
     * update ADT mgr state about this entry placement.
     */
    if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
      adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;

      if (get_adt_cookie) {
        status = pipe_mgr_adt_mgr_ha_get_cookie(
            dev_id, adt_tbl_hdl, pipe_id, stage_id, &adt_ha_cookie);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Failed to get adt cookie for adt table 0x%x attached to "
              "exm table 0x%x, dev %d pipe %x stage %d",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              mat_tbl_hdl,
              dev_id,
              pipe_id,
              stage_id);
          break;
        }
        get_adt_cookie = false;
      }
      status = pipe_mgr_adt_mgr_create_hlp_state(&adt_ha_cookie,
                                                 action_spec->adt_ent_hdl,
                                                 move_node->logical_action_idx,
                                                 true);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in creating hlp state for adt entry hdl %d, adt tbl "
            "0x%x, exm tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            action_spec->adt_ent_hdl,
            adt_tbl_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        break;
      }
    } else if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
      status = pipe_mgr_sel_update_hlp_ref(dev_id,
                                           exm_tbl->mat_tbl_hdl,
                                           move_node->entry_hdl,
                                           pipe_id,
                                           exm_tbl->sel_tbl_refs[0].tbl_hdl,
                                           action_spec->sel_grp_hdl);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in updating hlp refcount for sel grp hdl %d, sel tbl "
            "0x%x, exm tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            action_spec->sel_grp_hdl,
            exm_tbl->sel_tbl_refs[0].tbl_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        break;
      }
    }

    pipe_mat_tbl_info_t *mat_tbl_info =
        pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
    if (mat_tbl_info == NULL) {
      LOG_ERROR(
          "%s:%d Error in finding the table info for tbl 0x%x"
          " device id %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* Add it to the match-entry-map hash-table keyed by match-spec. */
    if (!pipe_mgr_is_device_virtual(dev_id)) {
      /* The spec map is populated only for non-virtual devices, since for
       * virtual device the state restore happens through move-lists and any
       * deltas are then replayed in the form of regular APIs outside of
       * warm-init begin and warm-init end
       */
      status = pipe_mgr_hitless_ha_new_spec(
          &exm_tbl_data->ha_hlp_info->spec_map, move_node);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error adding exm entry with hdl 0x%x to entry map for tbl "
            "%s, "
            "device id %d, tbl hdl 0x%x, err %s",
            __func__,
            __LINE__,
            move_node->entry_hdl,
            exm_tbl->name,
            dev_id,
            mat_tbl_hdl,
            pipe_str_err(status));
        break;
      }
    }
    (*success_count)++;
  }
  if (move_node) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) Error processing all placement operations "
        "during HA state restore at HLP, err %s",
        __func__,
        __LINE__,
        exm_tbl->name,
        dev_id,
        mat_tbl_hdl,
        pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_update_sel_hlp_state(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                pipe_mat_ent_hdl_t entry_hdl,
                                                pipe_sel_grp_hdl_t grp_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d EXM tbl 0x%x device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!exm_tbl->num_sel_tbl_refs) {
    return PIPE_SUCCESS;
  }

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, entry_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_sel_hlp_update_state(dev_tgt.device_id,
                                dev_tgt.dev_pipe_id,
                                exm_tbl->mat_tbl_hdl,
                                entry_info->mat_ent_hdl,
                                exm_tbl->sel_tbl_refs[0].tbl_hdl,
                                grp_hdl,
                                entry_info->logical_sel_idx);
  unpack_mat_ent_data_as(entry_info->entry_data)->sel_grp_hdl = grp_hdl;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d EXM tbl 0x%x device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) && (!exm_tbl->symmetric)) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, Invalid pipe %d specified for asymmetric tbl, dev "
        "%d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) && (exm_tbl->symmetric)) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, Invalid pipe %d specified for symmetric tbl, dev "
        "%d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, dev_tgt.dev_pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl info for tbl 0x%x pipe id %d, device id %d not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_MEMCPY(ha_report,
                  &exm_tbl_data->ha_reconc_report,
                  sizeof(pipe_tbl_ha_reconc_report_t));

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_hash_action_hitless_ha_reconcile(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_spec_map_t *spec_map = &exm_tbl_data->ha_hlp_info->spec_map;
  pipe_tbl_ha_reconc_report_t *ha_report = &exm_tbl_data->ha_reconc_report;

  /* We will build a list of move-nodes from all the add/mod/del calls below,
   * move_head will track the head of the list and move_tail will track the end
   * of the list so we can append each new move-node to it. */
  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;
  struct pipe_mgr_move_list_t *mh = NULL;

  /* Initialize the report stats. */
  ha_report->num_entries_added = 0;
  ha_report->num_entries_deleted = 0;
  ha_report->num_entries_modified = 0;

  bool has_idle =
      pipe_mgr_mat_tbl_has_idle(exm_tbl->dev_id, exm_tbl->mat_tbl_hdl);

  /* Handle the fully matched entries first.  This is only needed for idletime
   * cases since the TLL cannot be properly recovered from HW and therefore is
   * not correct in the LLP state and since the HLP state was generated from
   * that the HLP state is not correct. */
  if (has_idle) {
    for (pipe_mgr_ha_entry_t *entry = spec_map->full_match_list;
         entry && (rc == PIPE_SUCCESS);
         entry = entry->np) {
      /* All the entries in here need to update the state in HLP */
      rc = spec_map->entry_update_fn(
          spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, entry->mn, &mh);
      if (mh) {
        move_tail->next = mh;
        mh = NULL;
        move_tail = move_tail->next;
      }
    }
  }

  /* Go through the list of new entries and add then.  We are using the modify
   * function since all table entries are already present (the hash-action HW
   * read and restore creates all entries since technically all entries are
   * valid in the table at all times. */
  for (pipe_mgr_ha_entry_t *entry = spec_map->to_add_list;
       entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    ++ha_report->num_entries_added;
    rc = spec_map->entry_place_with_hdl_fn(spec_map->dev_tgt,
                                           spec_map->mat_tbl_hdl,
                                           entry->match_spec,
                                           entry->act_fn_hdl,
                                           entry->action_spec,
                                           entry->ttl,
                                           0,
                                           entry->entry_hdl,
                                           &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  /* Go through the "to modify" list next.  These are entries which have been
   * replayed with different action data or resource information. */
  for (pipe_mgr_ha_entry_t *entry = spec_map->to_modify_list;
       entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    /* Again, just as above, update HLP state if there is a TTL. */
    if (has_idle) {
      rc = spec_map->entry_update_fn(
          spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, entry->mn, &mh);
      if (mh) {
        move_tail->next = mh;
        mh = NULL;
        move_tail = move_tail->next;
      }
      if (rc != PIPE_SUCCESS) break;
    }

    ++ha_report->num_entries_modified;
    rc = spec_map->entry_modify_fn(spec_map->dev_tgt.device_id,
                                   spec_map->mat_tbl_hdl,
                                   entry->mn->entry_hdl,
                                   entry->act_fn_hdl,
                                   entry->action_spec,
                                   0,
                                   &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  /* There can be many entries on the "to delete" list since hash action tables
   * restore all entries.  Entries will be on this list for one of two reasons,
   * either they were originally added before the HA event but not replayed or
   * the entry was never added and is equal to the default entry.  In either
   * case the entries must be deleted from the HLP and LLP, we will write the
   * entry back to HW using the replayed default in both cases.  Before doing
   * this, if a default entry was replayed, make sure it is set.  If it was not
   * replayed then set the P4 specified default. */
  pipe_mat_ent_hdl_t def_ent_hdl;
  pipe_act_fn_hdl_t def_act_fn_hdl;
  pipe_action_spec_t *def_act_spec, tmp_act_spec;
  if (spec_map->def_act_spec) {
    def_act_fn_hdl = spec_map->def_act_fn_hdl;
    def_act_spec = spec_map->def_act_spec;
  } else {
    def_act_fn_hdl =
        exm_tbl->mat_tbl_info->default_info->action_entry.act_fn_hdl;
    def_act_spec = &tmp_act_spec;
    rc = pipe_mgr_create_action_spec(
        exm_tbl->dev_id,
        &exm_tbl->mat_tbl_info->default_info->action_entry,
        def_act_spec);
    if (rc != PIPE_SUCCESS) return rc;
  }
  rc = pipe_mgr_exm_default_ent_place(spec_map->dev_tgt,
                                      spec_map->mat_tbl_hdl,
                                      def_act_fn_hdl,
                                      def_act_spec,
                                      0,
                                      &def_ent_hdl,
                                      &mh);
  /* Clean up temp action spec if we allocated it. */
  if (def_act_spec == &tmp_act_spec) {
    if (tmp_act_spec.act_data.action_data_bits)
      PIPE_MGR_FREE(tmp_act_spec.act_data.action_data_bits);
  }
  if (mh) {
    move_tail->next = mh;
    mh = NULL;
    move_tail = move_tail->next;
  }
  for (pipe_mgr_ha_entry_t *entry = spec_map->to_delete_list;
       entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    ++ha_report->num_entries_deleted;
    rc = spec_map->entry_delete_fn(spec_map->dev_tgt.device_id,
                                   spec_map->mat_tbl_hdl,
                                   entry->mn->entry_hdl,
                                   0,
                                   &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  /* Pass the list of move-nodes collected here to the caller. */
  *move_head_p = move_head.next;

  /* Now cleanup all the state for this spec-map */
  pipe_mgr_hitless_ha_delete_spec_map(spec_map);

  return rc;
}

pipe_status_t pipe_mgr_exm_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_status_t status = PIPE_SUCCESS;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d EXM tbl 0x%x device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;
  struct pipe_mgr_move_list_t *mh = NULL;

  pipe_mat_tbl_info_t *tbl_info = exm_tbl->mat_tbl_info;
  if (!tbl_info) {
    LOG_ERROR("%s:%d Mat tbl info not found for tbl 0x%x, device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  unsigned i = 0;
  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    pipe_mgr_spec_map_t *spec_map = &exm_tbl_data->ha_hlp_info->spec_map;
    pipe_tbl_ha_reconc_report_t *ha_report = &exm_tbl_data->ha_reconc_report;
    if (exm_tbl->hash_action) {
      /* Clear the "HA mode" so that any deltas from the reconcile will be
       * applied.  */
      pipe_mgr_init_mode_reset(dev_id);
      status = pipe_mgr_exm_hash_action_hitless_ha_reconcile(
          exm_tbl, exm_tbl_data, &mh);
      /* Reset the HA mode. */
      pipe_mgr_init_mode_set(dev_id, BF_DEV_WARM_INIT_HITLESS);
    } else {
      status =
          pipe_mgr_hitless_ha_reconcile(spec_map, &mh, tbl_info, ha_report);
    }
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error reconciling HA state, for tbl 0x%x, device id %d, "
          "pipe_id %d, err %s",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_id,
          exm_tbl_data->pipe_id,
          pipe_str_err(status));
      *move_head_p = NULL;
      return status;
    }
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }
  *move_head_p = move_head.next;

  return PIPE_SUCCESS;
}

void pipe_mgr_exm_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return;
  }
  unsigned tbl_idx = 0;
  for (tbl_idx = 0; tbl_idx < exm_tbl->num_tbls; tbl_idx++) {
    pipe_mgr_exm_tbl_data_t *exm_tbl_data = &exm_tbl->exm_tbl_data[tbl_idx];
    if (!exm_tbl_data->ha_hlp_info) {
      return;
    }
    /* Spec map within the hlp info will be cleaned up as part of hitless HA
     * reconcile
     */
    PIPE_MGR_FREE(exm_tbl_data->ha_hlp_info);
    exm_tbl_data->ha_hlp_info = NULL;
  }
  return;
}
