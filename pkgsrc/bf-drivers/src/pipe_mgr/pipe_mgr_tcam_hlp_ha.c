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
 * @file pipe_mgr_tcam_hlp_ha.c
 * @date
 *
 * Implementation of TCAM HA management for HLP
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <lld/lld_inst_list_fmt.h>

#include <target-utils/fbitset/fbitset.h>

/* Local header files */
#include "pipe_mgr_log.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_transaction.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_alpm_ha.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_select_ha.h"

static pipe_status_t pipe_mgr_tcam_ha_hlp_action_restore(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_mgr_move_list_t *move_node) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_adt_ha_cookie_t adt_ha_cookie = {0};

  pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(move_node->data);

  bool is_range = move_node->op == PIPE_MAT_UPDATE_ADD_MULTI ? true : false;
  bool is_default = move_node->op == PIPE_MAT_UPDATE_SET_DFLT ? true : false;

  uint32_t index = 0;
  uint32_t ptn;
  if (move_node->op != PIPE_MAT_UPDATE_SET_DFLT) {
    if (is_range) {
      index = get_index_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
      ptn = get_ptn_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
    } else {
      index = get_index_from_logical_index(tcam_pipe_tbl,
                                           move_node->u.single.logical_idx);
      ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                       move_node->u.single.logical_idx);
    }
  } else {
    ptn = 0;
  }

  tcam_phy_loc_info_t tcam_loc;
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
    index = tcam_tbl->total_entries - 1;
  }

  rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, index, is_default, &tcam_loc);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

  if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
    PIPE_MGR_ASSERT(tcam_tbl_info->adt_tbl_ref.ref_type ==
                    PIPE_TBL_REF_TYPE_INDIRECT);
    rc = pipe_mgr_adt_mgr_ha_get_cookie(tcam_tbl_info->dev_id,
                                        tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                                        tcam_pipe_tbl->pipe_id,
                                        tcam_loc.stage_id,
                                        &adt_ha_cookie);
    if (rc == PIPE_SUCCESS) {
      rc = pipe_mgr_adt_mgr_create_hlp_state(&adt_ha_cookie,
                                             action_spec->adt_ent_hdl,
                                             move_node->logical_action_idx,
                                             true);
    }

  } else if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    PIPE_MGR_ASSERT(tcam_tbl_info->sel_tbl_ref.ref_type ==
                    PIPE_TBL_REF_TYPE_INDIRECT);
    rc = pipe_mgr_sel_update_hlp_ref(tcam_tbl_info->dev_id,
                                     tcam_tbl_info->tbl_hdl,
                                     move_node->entry_hdl,
                                     get_move_list_pipe(move_node),
                                     tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                     action_spec->sel_grp_hdl);
  }
  return rc;
}

static pipe_status_t pipe_mgr_tcam_process_placement_op_for_ha(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_mgr_move_list_t *move_node) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_INVALID_ARG;
  pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
      tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in finding the table info for tbl 0x%x"
        " device id %d",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl_match_spec_t *match_spec = unpack_mat_ent_data_ms(move_node->data);
  pipe_act_fn_hdl_t act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_node->data);
  pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(move_node->data);
  uint32_t ttl = unpack_mat_ent_data_ttl(move_node->data);
  pipe_mat_ent_hdl_t entry_hdl = move_node->entry_hdl;

  if (move_node->op == PIPE_MAT_UPDATE_SET_DFLT) {
    if (entry_hdl != tcam_pipe_tbl->hlp.default_ent_hdl) {
      LOG_ERROR(
          "%s:%d Invalid default handle passed to hlp ha for tcam table 0x%x "
          "device %d",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id);
      return PIPE_INVALID_ARG;
    }
    dev_target_t dev_tgt = {.device_id = tcam_tbl_info->dev_id,
                            .dev_pipe_id = tcam_pipe_tbl->pipe_id};
    rc = pipe_mgr_tcam_default_ent_place_with_hdl(dev_tgt,
                                                  tcam_tbl_info->tbl_hdl,
                                                  act_fn_hdl,
                                                  action_spec,
                                                  0,
                                                  entry_hdl,
                                                  &move_node,
                                                  true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d TCAM default entry add failed for dev %d tbl_hdl %d "
          "pipe %d rc 0x%x",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          tcam_tbl_info->tbl_hdl,
          dev_tgt.dev_pipe_id,
          rc);
      return rc;
    }
  } else {
    bool is_range = move_node->op == PIPE_MAT_UPDATE_ADD_MULTI ? true : false;

    uint32_t ptn;
    if (is_range) {
      ptn = get_ptn_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
    } else {
      ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                       move_node->u.single.logical_idx);
    }

    PIPE_MGR_ASSERT(ptn < tcam_pipe_tbl->no_ptns);
    tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    pipe_mgr_tcam_entry_hdl_set(tcam_pipe_tbl, entry_hdl);
    rc = pipe_mgr_tcam_entry_add_internal(tcam_tbl,
                                          match_spec,
                                          act_fn_hdl,
                                          action_spec,
                                          ttl,
                                          entry_hdl,
                                          &move_node,
                                          true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding tcam entry with hdl 0x%x to the tcam table rc 0x%x ",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          entry_hdl,
          rc);
      return rc;
    }

    /* Add it to the match-entry-map hash-table keyed by match-spec. */
    if (!pipe_mgr_is_device_virtual(tcam_tbl_info->dev_id) &&
        !mat_tbl_info->alpm_info) {
      /* The spec map is populated only for non-virtual devices, since for
       * virtual device the state restore happens through move-lists and any
       * deltas are then replayed in the form of regular APIs outside of
       * warm-init begin and warm-init end
       */
      rc = pipe_mgr_hitless_ha_new_spec(&tcam_pipe_tbl->spec_map, move_node);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error adding tcam entry with hdl 0x%x to match entry map rc 0x%x ",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            entry_hdl,
            rc);
        return rc;
      }
    }
  }

  if (mat_tbl_info->alpm_info) {
    rc = pipe_mgr_alpm_hlp_entry_restore(
        tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, move_node);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error restoring alpm entry for tcam entry hdl %d tbl 0x%x "
          "device id %d",
          __func__,
          __LINE__,
          entry_hdl,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id);
      return rc;
    }

    // Restore the priority of the preclassifier entry
    if (tcam_tbl_info->tbl_hdl == mat_tbl_info->alpm_info->preclass_handle) {
      if (!move_node) {
        LOG_ERROR(
            "%s:%d Unexpected state, move_node is NULL", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      rc = pipe_mgr_tcam_entry_update_state(
          tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, move_node, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to update state for tcam entry %d "
            "tbl 0x%x device %d",
            __func__,
            __LINE__,
            move_node->entry_hdl,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id);
        return rc;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_hlp_restore_state(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mgr_move_list_t *move_list,
                                              uint32_t *success_count) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_ASSERT(*success_count == 0);

  pipe_mgr_move_list_t *move_node = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  for (move_node = move_list; move_node; move_node = move_node->next) {
    PIPE_MGR_ASSERT((move_node->op == PIPE_MAT_UPDATE_ADD) ||
                    (move_node->op == PIPE_MAT_UPDATE_ADD_MULTI) ||
                    (move_node->op == PIPE_MAT_UPDATE_SET_DFLT));

    bf_dev_pipe_t pipe_id = 0;
    pipe_id = get_move_list_pipe(move_node);

    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (tcam_pipe_tbl == NULL) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          pipe_id);
      rc = PIPE_OBJ_NOT_FOUND;
      break;
    }

    rc = pipe_mgr_tcam_ha_hlp_action_restore(tcam_pipe_tbl, move_node);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error restoring the action for tcam entry 0x%x"
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          move_node->entry_hdl,
          rc);
      return rc;
    }

    rc = pipe_mgr_tcam_process_placement_op_for_ha(tcam_pipe_tbl, move_node);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    (*success_count)++;
  }

  if (move_node) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error processing all the placement operations rc 0x%x ",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_update_sel_hlp_state(dev_target_t dev_tgt,
                                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                                 pipe_mat_ent_hdl_t entry_hdl,
                                                 pipe_sel_grp_hdl_t grp_hdl) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_hlp_entry_t *tcam_entry = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl 0x%x not found on device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!tcam_tbl_info->sel_present) {
    return PIPE_SUCCESS;
  }

  tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);
  if (tcam_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d TCAM table 0x%x for pipe %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (bf_map_get(&tcam_pipe_tbl->hlp.tcam_entry_db,
                 entry_hdl,
                 (void **)&tcam_entry) == BF_MAP_OK) {
    pipe_mgr_sel_hlp_update_state(dev_tgt.device_id,
                                  dev_tgt.dev_pipe_id,
                                  tcam_tbl_info->tbl_hdl,
                                  tcam_entry->entry_hdl,
                                  tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                  grp_hdl,
                                  tcam_entry->logical_sel_idx);
    unpack_mat_ent_data_as(tcam_entry->mat_data)->sel_grp_hdl = grp_hdl;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_ha_reconc_report(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;

  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
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
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  if (!tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for asymmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  if (tcam_tbl_info->is_symmetric) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  } else {
    tcam_pipe_tbl =
        get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, dev_tgt.dev_pipe_id);
  }
  if (!tcam_pipe_tbl) {
    LOG_ERROR("%s:%d TCAM table for 0x%x pipe %d on dev %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMCPY(ha_report,
                  &tcam_pipe_tbl->ha_reconc_report,
                  sizeof(pipe_tbl_ha_reconc_report_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_hlp_compute_delta_changes(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
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

  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bf_dev_pipe_t pipe_id = 0;
  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];

    rc = pipe_mgr_hitless_ha_reconcile(&tcam_pipe_tbl->spec_map,
                                       &mh,
                                       tbl_info,
                                       &tcam_pipe_tbl->ha_reconc_report);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error reconciling HA state rc 0x%x ",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      *move_head_p = NULL;
      return rc;
    }
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  *move_head_p = move_head.next;

  return rc;
}

void pipe_mgr_tcam_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  (void)device_id;
  (void)mat_tbl_hdl;
  /* Nothing to cleanup */
  return;
}
