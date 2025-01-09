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
 * @file pipe_mgr_tcam_llp_ha.c
 * @date
 *
 * Implementation of TCAM hardware access, instruction post etc
 */
/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_inst_list_fmt.h>
#include <target-utils/bit_utils/bit_utils.h>

/* Local header files */
#include "pipe_mgr_log.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tcam_transaction.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_select_ha.h"

/* LLP
 * - Go through the Shadow mem and update the tcam_entries arrays with
 *   the valid entries
 * - Create a move-list for use by the HLP
 *
 * - During API replay, check for HA mode and perform the below:
 *       - If the entry in shadow-mem differs from what has been provided,
 *         mark dirty
 *       -
 */

static pipe_status_t pipe_mgr_tcam_llp_ha_update_resources(
    pipe_mat_ent_hdl_t entry_hdl,
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint8_t stage_id,
    pipe_action_spec_t *action_spec,
    uint32_t dir_entry_idx,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info) {
  uint32_t i = 0;
  pipe_tbl_ref_t *ref = NULL;
  pipe_tbl_hdl_t tbl_hdl;
  pipe_status_t status = PIPE_SUCCESS;
  action_spec->resource_count = 0;
  bool pfe = false, pfe_defaulted = false;
  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    ref = &tcam_tbl_info->tbl_refs[i];
    tbl_hdl = ref->tbl_hdl;
    int rsrc_cnt = action_spec->resource_count;
    switch (PIPE_GET_HDL_TYPE(tbl_hdl)) {
      case PIPE_HDL_TYPE_STAT_TBL:
        action_spec->resources[rsrc_cnt].tbl_hdl = tbl_hdl;
        if (ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
          /* No need to fill in the index, since its directly addressed */
          action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
          action_spec->resource_count++;
          /* Inform the stat mgr about the entry */
          dev_target_t dev_tgt;
          dev_tgt.device_id = tcam_tbl_info->dev_id;
          dev_tgt.dev_pipe_id = tcam_pipe_tbl->pipe_id;
          status = pipe_mgr_stat_mgr_add_entry(
              pipe_mgr_get_int_sess_hdl(),
              dev_tgt,
              entry_hdl,
              tbl_hdl,
              stage_id,
              dir_entry_idx,
              false,
              &action_spec->resources[rsrc_cnt].data.counter,
              ptr_info->force_stats ? NULL : &indirect_ptrs->stats_ptr);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in adding stat entry for entry hdl %d, tbl 0x%x, "
                "device id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                entry_hdl,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_id,
                pipe_str_err(status));
            return status;
          }
        } else if (ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT &&
                   !ptr_info->force_stats) {
          pipe_stat_ent_idx_t stats_idx = 0;
          status = pipe_mgr_stat_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                      tbl_hdl,
                                                      tcam_pipe_tbl->pipe_id,
                                                      stage_id,
                                                      indirect_ptrs->stats_ptr,
                                                      &pfe,
                                                      &pfe_defaulted,
                                                      &stats_idx);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding stats virt addr 0x%x tbl 0x%x, device "
                "id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->stats_ptr,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_id,
                pipe_str_err(status));
            return status;
          }
          if (pfe_defaulted || (!pfe_defaulted && pfe)) {
            action_spec->resources[rsrc_cnt].tbl_idx = stats_idx;
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
          } else {
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          action_spec->resource_count++;
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        action_spec->resources[rsrc_cnt].tbl_hdl = tbl_hdl;
        if (ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
          /* No need to fill in the index, since its directly addressed */
          action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
          action_spec->resource_count++;
        } else if (ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT &&
                   !ptr_info->force_meter) {
          pipe_idx_t meter_idx = 0;
          status = pipe_mgr_meter_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                       tbl_hdl,
                                                       tcam_pipe_tbl->pipe_id,
                                                       stage_id,
                                                       indirect_ptrs->meter_ptr,
                                                       &pfe,
                                                       &pfe_defaulted,
                                                       &meter_idx);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding meter virt addr 0x%x, tbl 0x%x, "
                "device id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->meter_ptr,
                tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_id,
                pipe_str_err(status));
            return status;
          }
          if (pfe_defaulted || (!pfe_defaulted && pfe)) {
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
            action_spec->resources[rsrc_cnt].tbl_idx = meter_idx;
          } else {
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          action_spec->resource_count++;
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        action_spec->resources[rsrc_cnt].tbl_hdl = tbl_hdl;
        if (ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
          /* No need to fill in the index, since its directly addressed */
          action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
          action_spec->resource_count++;
          /* Stful spec is not reconstructed. Stful entries are rewritten during
           * warm init end based on API replay.
           */
        } else if (ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT &&
                   !ptr_info->force_stful) {
          pipe_idx_t stful_idx = 0;
          status = pipe_mgr_stful_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                       tbl_hdl,
                                                       tcam_pipe_tbl->pipe_id,
                                                       stage_id,
                                                       indirect_ptrs->stfl_ptr,
                                                       &pfe,
                                                       &pfe_defaulted,
                                                       &stful_idx);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding stful virt addr 0x%x, tbl 0x%x, "
                "device id "
                "%d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->stfl_ptr,
                tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_id,
                pipe_str_err(status));
            return status;
          }
          if (pfe_defaulted || (!pfe_defaulted && pfe)) {
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
            action_spec->resources[rsrc_cnt].tbl_idx = stful_idx;
          } else {
            action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          action_spec->resource_count++;
        }
        break;
      default:
        PIPE_MGR_ASSERT(0);
        break;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tcam_llp_ha_update_action_spec(
    pipe_mat_ent_hdl_t entry_hdl,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_stage_info_t *stage_data,
    uint32_t stage_line_no,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    uint32_t *logical_action_idx_p,
    uint32_t *logical_sel_idx_p,
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_mgr_sel_ha_cookie_t *sel_cookie) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;

  /* DRV-1055 : Always rebuild resources first and only then inform ADT/SEL tbl
   * mgrs
   */
  rc = pipe_mgr_tcam_llp_ha_update_resources(entry_hdl,
                                             tcam_tbl_info,
                                             tcam_pipe_tbl,
                                             stage_data->stage_id,
                                             action_spec,
                                             stage_line_no,
                                             indirect_ptrs,
                                             ptr_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in populating action spec resources for tbl 0x%x device "
        "id %d, err %s",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (tcam_tbl_info->adt_present) {
    if (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      /* Based on if selector is used or not, the type should change */
      if (indirect_ptrs->sel_len) {
        action_spec->pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
      } else {
        action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
      }
    } else {
      action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    }

    switch (action_spec->pipe_action_datatype_bmap) {
      case PIPE_SEL_GRP_HDL_TYPE:
        rc = pipe_mgr_sel_get_temp_sel_grp_hdl(
            tcam_tbl_info->dev_id,
            tcam_tbl_info->sel_tbl_ref.tbl_hdl,
            indirect_ptrs->sel_ptr,
            indirect_ptrs->sel_len,
            tcam_pipe_tbl->pipe_id,
            stage_data->stage_id,
            sel_cookie,
            &action_spec->sel_grp_hdl,
            logical_sel_idx_p);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error getting a temp sel grp hdl for TCAM entry at pipe %d "
              "stage "
              "%d sel_ptr 0x%x"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_data->stage_id,
              indirect_ptrs->sel_ptr,
              rc);
          return rc;
        }

        rc = pipe_mgr_sel_update_llp_state_for_ha(
            tcam_tbl_info->dev_id,
            tcam_tbl_info->sel_tbl_ref.tbl_hdl,
            sel_cookie,
            *logical_sel_idx_p,
            indirect_ptrs->sel_len,
            adt_cookie,
            action_spec,
            act_fn_hdl,
            indirect_ptrs->adt_ptr,
            logical_action_idx_p);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error updating LLP sel state for TCAM entry at pipe %d stage "
              "%d sel_ptr 0x%x"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_data->stage_id,
              indirect_ptrs->sel_ptr,
              rc);
          return rc;
        }
        break;
      case PIPE_ACTION_DATA_HDL_TYPE:
        rc = pipe_mgr_adt_mgr_get_temp_adt_ent_hdl(
            tcam_tbl_info->dev_id,
            tcam_tbl_info->direction,
            tcam_tbl_info->adt_tbl_ref.tbl_hdl,
            indirect_ptrs->adt_ptr,
            tcam_pipe_tbl->pipe_id,
            stage_data->stage_id,
            adt_cookie,
            action_spec,
            act_fn_hdl,
            logical_action_idx_p);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error getting a temp adt ent hdl for TCAM entry at pipe %d "
              "stage "
              "%d adt_ptr 0x%x"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_data->stage_id,
              indirect_ptrs->adt_ptr,
              rc);
          return rc;
        }

        /* Ask ADT MGR to update LLP state. State is maintained at the LLP by
         * the adt mgr only for indirectly referenced action data tables.
         */
        rc = pipe_mgr_adt_mgr_update_llp_state_for_ha(
            tcam_tbl_info->dev_id,
            tcam_tbl_info->adt_tbl_ref.tbl_hdl,
            adt_cookie,
            action_spec,
            act_fn_hdl,
            *logical_action_idx_p,
            true);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error updating LLP adt state for TCAM entry at pipe %d stage "
              "%d adt_ptr 0x%x"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_data->stage_id,
              indirect_ptrs->adt_ptr,
              rc);
          return rc;
        }
        break;
      case PIPE_ACTION_DATA_TYPE:
        /* If an ADT tbl exists, check with ADT mgr for the action-data spec at
         * this location
         */
        if (tcam_tbl_info->adt_present) {
          pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;
          dev_target_t dev_tgt_all_pipes = {tcam_tbl_info->dev_id,
                                            BF_DEV_PIPE_ALL};
          rc = pipe_mgr_adt_mgr_decode_to_act_data_spec(
              dev_tgt_all_pipes,
              tcam_tbl_info->adt_tbl_ref.tbl_hdl,
              stage_data->stage_table_handle,
              stage_line_no,
              tcam_pipe_tbl->pipe_id,
              stage_data->stage_id,
              act_fn_hdl,
              act_data_spec,
              adt_cookie,
              false,
              NULL);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d - %s (%d - 0x%x) "
                "Error getting action data spec for TCAM entry at pipe %d "
                "stage "
                "%d match-addr 0x%x"
                "rc 0x%x",
                __func__,
                __LINE__,
                tcam_tbl_info->name,
                tcam_tbl_info->dev_id,
                tcam_tbl_info->tbl_hdl,
                tcam_pipe_tbl->pipe_id,
                stage_data->stage_id,
                stage_line_no,
                rc);
            return rc;
          }
        }
        break;
    }
  } else {
    /* If adt is absent, the action data has to be direct */
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }
  return PIPE_SUCCESS;
}

static pipe_mat_ent_hdl_t pipe_mgr_tcam_ha_entry_hdl_allocate(
    tcam_pipe_tbl_t *tcam_pipe_tbl) {
  pipe_mat_ent_hdl_t ent_hdl =
      bf_id_allocator_allocate(tcam_pipe_tbl->llp.ha_ent_hdl_allocator);
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

static pipe_status_t tcam_llp_read_shadow_and_restore(
    tcam_tbl_info_t *tcam_tbl_info, pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t _pipe;

  uint32_t num_valid_match_bits = tcam_tbl_info->num_match_spec_bits,
           num_match_bytes = tcam_tbl_info->num_match_spec_bytes;
  uint32_t num_action_data_bytes = tcam_tbl_info->max_act_data_size;
  PWord_t PValue = NULL;
  Pvoid_t PJLArray = NULL;
  Word_t Rc_word;
  uint32_t max_expanded_range_entry_count = 1,
           curr_expanded_range_entry_count = 0;
  uint32_t i = 0;
  pipe_tbl_match_spec_t *match_spec = NULL;
  bool range_entry_read_complete = true;

  pipe_tbl_match_spec_t **match_specs = NULL;
  pipe_mgr_sel_ha_cookie_t *sel_cookie = NULL;
  pipe_mgr_adt_ha_cookie_t *adt_cookie = NULL;
  pipe_mgr_adt_ha_cookie_t *adt_cookie_mem = NULL;

  pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
      tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Error in finding the table info for tbl 0x%x device id %d",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_action_spec_t *action_spec = NULL;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_act_fn_info_t *act_fn_info = NULL;
  act_fn_info = tcam_tbl_info->act_fn_hdl_info;

  action_spec = pipe_mgr_tbl_alloc_action_spec(num_action_data_bytes);
  if (action_spec == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  act_data_spec = &action_spec->act_data;
  for (i = 0; i < tcam_tbl_info->num_actions; i++) {
    /* Maintain a mapping from act_fn_hdl to the idx within the array of act
     * data spec for fast access.
     */
    JLI(PValue, PJLArray, act_fn_info[i].act_fn_hdl);
    *PValue = i;
  }

  if (tcam_tbl_info->adt_present) {
    adt_cookie_mem = (pipe_mgr_adt_ha_cookie_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_adt_ha_cookie_t));
    if (adt_cookie_mem == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  if (tcam_tbl_info->sel_present) {
    sel_cookie = (pipe_mgr_sel_ha_cookie_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_sel_ha_cookie_t));
    if (sel_cookie == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }
  bool isatcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  bool is_tbl_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);

  if (is_tbl_range) {
    rc = pipe_mgr_tcam_range_max_expansion_entry_count_get(
        tcam_tbl_info, &max_expanded_range_entry_count);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error getting the max number of expanded entries for tbl "
          "0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      goto cleanup;
    }
  }
  /* Allocate an additional match spec because while decoding a range entry
   * we go on reading the hardware until we encounter another range entry
   * head. As a result, we must allocate space equal to the max entries
   * a range entry can expand into AND an additional space for the next range
   * entry head
   */
  max_expanded_range_entry_count++;
  match_specs = (pipe_tbl_match_spec_t **)PIPE_MGR_CALLOC(
      max_expanded_range_entry_count, sizeof(pipe_tbl_match_spec_t *));
  if (max_expanded_range_entry_count != 0 && match_specs == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  for (i = 0; i < max_expanded_range_entry_count; i++) {
    match_specs[i] = pipe_mgr_tbl_alloc_match_spec(num_match_bytes);
    match_specs[i]->num_valid_match_bits = num_valid_match_bits;
  }

  for (_pipe = 0; _pipe < tcam_tbl_info->no_tcam_pipe_tbls; _pipe++) {
    tcam_pipe_tbl_t *tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[_pipe];

    uint32_t block;
    for (block = 0; block < tcam_pipe_tbl->num_blocks; block++) {
      tcam_block_data_t *block_data = &tcam_pipe_tbl->block_data[block];
      tcam_stage_info_t *stage_data =
          get_stage_data_for_block(tcam_pipe_tbl, block_data);

      if (!stage_data) {
        LOG_ERROR("%s:%d - Error getting stage data, device id: %d",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->dev_id);
        rc = PIPE_OBJ_NOT_FOUND;
        goto cleanup;
      }

      pipe_mem_type_t pipe_mem_type;
      pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info) ? pipe_mem_type_unit_ram
                                                       : pipe_mem_type_tcam;

      uint32_t wide_tcam_units = stage_data->pack_format.mem_units_per_tbl_word;
      uint8_t *tbl_words[wide_tcam_units];

      uint8_t word_index = 0;

      uint32_t stage_id = stage_data->stage_id;
      bf_dev_pipe_t phy_mem_map_pipe_id;

      if (adt_cookie_mem) {
        adt_cookie = adt_cookie_mem;
        rc = pipe_mgr_adt_mgr_ha_get_cookie(tcam_tbl_info->dev_id,
                                            tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                                            tcam_pipe_tbl->pipe_id,
                                            stage_id,
                                            adt_cookie);
        if (rc == PIPE_OBJ_NOT_FOUND) {
          /* Not all stages need to use ADT Table data. Immediate action data
           * could be used in some stages.
           */
          LOG_TRACE(
              "%s:%d - %s (%d - 0x%x) "
              "Not found ADT cookie for adt_tbl 0x%x pipe %d stage %d",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->adt_tbl_ref.tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_id);
          adt_cookie = NULL;
          rc = PIPE_SUCCESS;
        } else if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error getting ADT cookie for adt_tbl 0x%x pipe %d stage %d"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->adt_tbl_ref.tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_id,
              rc);
          goto cleanup;
        }
      }

      if (sel_cookie) {
        rc = pipe_mgr_sel_ha_get_cookie(tcam_tbl_info->dev_id,
                                        tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                        tcam_pipe_tbl->pipe_id,
                                        stage_id,
                                        sel_cookie);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error getting sel cookie for sel tbl 0x%x pipe %d stage %d"
              "rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->sel_tbl_ref.tbl_hdl,
              tcam_pipe_tbl->pipe_id,
              stage_id,
              rc);
          goto cleanup;
        }
      }

      if (tcam_tbl_info->is_symmetric) {
        phy_mem_map_pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
      } else {
        phy_mem_map_pipe_id =
            PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
      }

      uint32_t subword, range_head_stage_line_no = 0;
      for (subword = 0; subword < stage_data->pack_format.entries_per_tbl_word;
           subword++) {
        vpn_id_t vpn_id = block_data->word_blk.vpn_id[subword];

        uint32_t line, range_head_line = 0;
        uint32_t _l;
        for (_l = stage_data->mem_depth; _l; _l--) {
          line = _l - 1;
          for (i = 0; i < wide_tcam_units; i++) {
            word_index = wide_tcam_units - i - 1;
            rc = pipe_mgr_phy_mem_map_get_ref(tcam_tbl_info->dev_id,
                                              tcam_tbl_info->direction,
                                              pipe_mem_type,
                                              phy_mem_map_pipe_id,
                                              stage_id,
                                              block_data->word_blk.mem_id[i],
                                              line,
                                              &tbl_words[word_index],
                                              true);
            if (rc != PIPE_SUCCESS) {
              LOG_CRIT(
                  "%s:%d - %s (%d - 0x%x) "
                  "Error getting the reference to shadow data for stage %d "
                  "mem_id %d line_no %d rc %s",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->name,
                  tcam_tbl_info->dev_id,
                  tcam_tbl_info->tbl_hdl,
                  stage_id,
                  block_data->word_blk.mem_id[i],
                  line,
                  pipe_str_err(rc));
              goto cleanup;
            }
          }

          match_spec = match_specs[curr_expanded_range_entry_count];
          /* Reset some of the local variables */
          if (act_data_spec->action_data_bits) {
            PIPE_MGR_MEMSET(
                act_data_spec->action_data_bits, 0, num_action_data_bytes);
          }
          PIPE_MGR_MEMSET(
              action_spec->resources, 0, sizeof(action_spec->resources));
          action_spec->resource_count = 0;
          PIPE_MGR_MEMSET(match_spec->match_value_bits, 0, num_match_bytes);
          PIPE_MGR_MEMSET(match_spec->match_mask_bits, 0, num_match_bytes);

          uint32_t stage_line_no;

          stage_line_no = (vpn_id << log2_uint32_ceil(stage_data->mem_depth)) |
                          (line & (stage_data->mem_depth - 1));
          pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
          pipe_mgr_ent_decode_ptr_info_t ptr_info = {0};
          pipe_act_fn_hdl_t act_fn_hdl = 0;
          if (tcam_tbl_info->num_actions) {
            act_fn_hdl = tcam_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
          }

          /* Figure out the match-spec of the entry by calling decoder
           * function
           */
          PIPE_MGR_MEMSET(
              match_spec->match_value_bits, 0, match_spec->num_match_bytes);
          PIPE_MGR_MEMSET(
              match_spec->match_mask_bits, 0, match_spec->num_match_bytes);
          bool valid = false;
          bool is_range_entry_head = false;
          rc = pipe_mgr_tcam_decode_entry(tcam_tbl_info,
                                          stage_data,
                                          line,
                                          match_spec,
                                          action_spec,
                                          tbl_words,
                                          subword,
                                          &act_fn_hdl,
                                          &indirect_ptrs,
                                          &ptr_info,
                                          &valid,
                                          &is_range_entry_head);

          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding tcam entry at line %d, subword %d, "
                "stage id %d, tbl 0x%x, device id %d, err %s",
                __func__,
                __LINE__,
                line,
                subword,
                stage_data->stage_id,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id,
                pipe_str_err(rc));
            goto cleanup;
          }
          if (!valid) {
            if (is_tbl_range) {
              /* This indicates that we are done reading all the entries
               * that a given range entry had expanded into. However,
               * we are yet to process it.
               * We don't want to continue even before we process the
               * range entry under consideration.*/
              if (range_entry_read_complete) {
                continue;
              }
            } else {
              /* Entry does not exist, just continue */
              continue;
            }
          }
          if (is_tbl_range) {
            if (is_range_entry_head) {
              if (curr_expanded_range_entry_count > 0) {
                /* This means that we were already in the middle of reading
                 * a range entry and now since we have encountered another
                 * range_head, a new range entry has started and we have
                 * finished reading the old one
                 */
                range_entry_read_complete = true;
                /* Increment the line no so that we can start with decoding
                 * this new range entry in the next iteration
                 */
                _l++;
              } else {
                range_entry_read_complete = false;
                curr_expanded_range_entry_count = 1;
                /* Cache the stage line no for the range head to decode the
                 * action spec later
                 */
                range_head_stage_line_no = stage_line_no;
                range_head_line = line;
              }
            } else {
              /* Just increment the sub-entry count which a given range entry
               * had expanded into
               */
              curr_expanded_range_entry_count++;
            }

            if (!range_entry_read_complete) {
              continue;
            }
            /* Once we have read all the sub entries that the range entry had
             * expanded into, compress them into the original range entry
             */
            rc = pipe_mgr_tcam_compress_decoded_range_entries(
                tcam_tbl_info->dev_id,
                tcam_tbl_info->profile_id,
                tcam_tbl_info->tbl_hdl,
                &match_specs[0],
                match_specs[0],
                curr_expanded_range_entry_count);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in compressing decoded range entries for tbl "
                  "0x%x, device id %d, err %s",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id,
                  pipe_str_err(rc));
              goto cleanup;
            }

            /* Mark the range entry to have been read completely */
            range_entry_read_complete = true;
            /* Set the stage line no to that of the range head to decode the
             * action spec
             */
            stage_line_no = range_head_stage_line_no;
            line = range_head_line;
          }
          match_spec = match_specs[0];

          uint32_t tind_subword_pos = 0;
          uint32_t tind_line_no = 0, tind_block = 0;
          uint8_t *tind_data = NULL;
          bool tind_exists = pipe_mgr_tcam_tind_get_line_no(stage_data,
                                                            stage_line_no,
                                                            &tind_line_no,
                                                            &tind_block,
                                                            &tind_subword_pos);

          /* If not ATCAM, attempt at TIND decoding, also we only need to decode
           * TIND for the range head entry
           */
          if (!isatcam && tind_exists) {
            rc = pipe_mgr_phy_mem_map_get_ref(
                tcam_tbl_info->dev_id,
                tcam_tbl_info->direction,
                pipe_mem_type_unit_ram,
                phy_mem_map_pipe_id,
                stage_data->stage_id,
                stage_data->tind_blk[tind_block].mem_id[0],
                tind_line_no % TOF_SRAM_UNIT_DEPTH,
                &tind_data,
                true);

            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d - %s (%d - 0x%x) "
                  "Error getting the reference to tind shadow data for "
                  "tind line %d rc 0x%x",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->name,
                  tcam_tbl_info->dev_id,
                  tcam_tbl_info->tbl_hdl,
                  tind_line_no,
                  rc);
              goto cleanup;
            }
            rc = pipe_mgr_tcam_decode_tind_entry(tcam_tbl_info,
                                                 stage_data,
                                                 stage_line_no,
                                                 act_data_spec,
                                                 &act_fn_hdl,
                                                 &indirect_ptrs,
                                                 &ptr_info,
                                                 tind_data,
                                                 tind_subword_pos,
                                                 0 /*offset*/);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in decoding tind entry for line no %d, stage id "
                  "%d, tbl 0x%x, device id %d, err %s",
                  __func__,
                  __LINE__,
                  stage_line_no,
                  stage_data->stage_id,
                  tcam_tbl_info->tbl_hdl,
                  tcam_tbl_info->dev_id,
                  pipe_str_err(rc));
              goto cleanup;
            }
          } else if (!isatcam && is_range_entry_head && !tind_exists) {
            /* This is a case where the tcam table does not have a tind. So it
             * has only a single action and we do not decode the action handle
             * from the hardware.
             */
            act_fn_hdl = stage_data->tind_act_fn_hdl;
            if (act_fn_hdl == 0 && tcam_tbl_info->num_actions)
              act_fn_hdl = tcam_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
          }

          /* Clean up the indirect pointer values if they were forced. */
          if (ptr_info.force_stats) indirect_ptrs.stats_ptr = 0;
          if (ptr_info.force_meter) indirect_ptrs.meter_ptr = 0;
          if (ptr_info.force_stful) indirect_ptrs.stfl_ptr = 0;

          uint32_t ptn_index = 0, index = 0;
          pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(
              tcam_pipe_tbl, block, line, subword, &index, &ptn_index);

          /* Populate partition index for atcam */
          if (isatcam) {
            match_spec->partition_index = ptn_index;
            // Also put the partition index in the match spec too
            // if it is not an underlying tcam table of an ALPM table
            if (!mat_tbl_info->alpm_info) {
              rc = tcam_set_partition_idx_in_match_spec(
                  tcam_tbl_info, match_spec, match_spec->partition_index);
              if (rc != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in setting partition idx of %d in match_spec, "
                    "tbl 0x%x, device id %d, err %s",
                    __func__,
                    __LINE__,
                    match_spec->partition_index,
                    tcam_tbl_info->tbl_hdl,
                    tcam_tbl_info->dev_id,
                    pipe_str_err(rc));
                goto cleanup;
              }
            }
          }

          /* Create a temp-mat-ent-hdl */
          pipe_mat_ent_hdl_t entry_hdl = 0;
          entry_hdl = pipe_mgr_tcam_ha_entry_hdl_allocate(tcam_pipe_tbl);
          if (!tcam_tbl_info->is_symmetric) {
            entry_hdl = PIPE_SET_HDL_PIPE(entry_hdl, tcam_pipe_tbl->pipe_id);
          }

          /* Based on the action function handle, fill in the info into the
           * action_spec
           */
          JLG(PValue, PJLArray, act_fn_hdl);
          PIPE_MGR_ASSERT(PValue);
          action_spec->act_data.num_valid_action_data_bits =
              act_fn_info[*PValue].num_bits;
          action_spec->act_data.num_action_data_bytes =
              act_fn_info[*PValue].num_bytes;

          uint32_t logical_action_idx = 0, logical_sel_idx = 0;
          rc = tcam_llp_ha_update_action_spec(entry_hdl,
                                              tcam_pipe_tbl,
                                              stage_data,
                                              stage_line_no,
                                              &indirect_ptrs,
                                              &ptr_info,
                                              action_spec,
                                              act_fn_hdl,
                                              &logical_action_idx,
                                              &logical_sel_idx,
                                              adt_cookie,
                                              sel_cookie);
          if (rc != PIPE_SUCCESS) goto cleanup;

          pipe_mgr_move_list_t *move_node;
          enum pipe_mat_update_type op =
              is_tbl_range ? PIPE_MAT_UPDATE_ADD_MULTI : PIPE_MAT_UPDATE_ADD;
          move_node = alloc_move_list(NULL, op, tcam_pipe_tbl->pipe_id);
          if (!move_node) {
            LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
            rc = PIPE_NO_SYS_RESOURCES;
            goto cleanup;
          }

          uint32_t ttl = 0;
          if (tcam_tbl_info->idle_present) {
            ttl = NONZERO_TTL;
          }

          move_node->data =
              make_mat_ent_data(match_spec,
                                action_spec,
                                act_fn_hdl,
                                ttl,
                                indirect_ptrs.sel_len,  // selection group len
                                0,
                                0);

          move_node->entry_hdl = entry_hdl;

          if (action_spec && IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
            move_node->adt_ent_hdl = action_spec->adt_ent_hdl;
            move_node->adt_ent_hdl_valid = true;
          }

          move_node->logical_sel_idx = logical_sel_idx;
          move_node->logical_action_idx = logical_action_idx;
          move_node->selector_len = indirect_ptrs.sel_len;

          if (is_tbl_range) {
            struct pipe_multi_index *locations;
            locations = (struct pipe_multi_index *)PIPE_MGR_CALLOC(
                curr_expanded_range_entry_count,
                sizeof(struct pipe_multi_index));
            if (locations == NULL) {
              LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
              rc = PIPE_NO_SYS_RESOURCES;
              goto cleanup;
            }

            locations[0].logical_index_count = curr_expanded_range_entry_count;
            locations[0].logical_index_base = get_logical_index_from_index_ptn(
                tcam_pipe_tbl, ptn_index, index);
            move_node->u.multi.array_sz =
                (curr_expanded_range_entry_count + 7) / 8;
            move_node->u.multi.locations = locations;
          } else {
            move_node->u.single.logical_idx = get_logical_index_from_index_ptn(
                tcam_pipe_tbl, ptn_index, index);
          }

          /* Using this move-node, update the LLP without updating HW */
          /* VK update the match_specs to an array for range */
          // pipe_tbl_match_spec_t *match_specs[1];
          // match_specs[0] = unpack_mat_ent_data_ms(move_node->data);

          rc = pipe_mgr_tcam_process_allocate(tcam_pipe_tbl,
                                              &match_specs[0],
                                              move_node,
                                              &indirect_ptrs,
                                              &ptr_info);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d - %s (%d - 0x%x) "
                "Error updating the tcam entry state for entry 0x%x "
                "rc 0x%x",
                __func__,
                __LINE__,
                tcam_tbl_info->name,
                tcam_tbl_info->dev_id,
                tcam_tbl_info->tbl_hdl,
                entry_hdl,
                rc);
          } else {
#ifdef BF_HITLESS_HA_DEBUG
            LOG_DBG(
                "Dev %d Table %s (0x%x) Pipe %x, recovered match and action "
                "spec:",
                tcam_tbl_info->dev_id,
                tcam_tbl_info->name,
                tcam_tbl_info->tbl_hdl,
                tcam_pipe_tbl->pipe_id);
            pipe_mgr_entry_format_log_match_spec(tcam_tbl_info->dev_id,
                                                 BF_LOG_DBG,
                                                 tcam_tbl_info->profile_id,
                                                 tcam_tbl_info->tbl_hdl,
                                                 match_spec);
            pipe_mgr_entry_format_log_action_spec(tcam_tbl_info->dev_id,
                                                  BF_LOG_DBG,
                                                  tcam_tbl_info->profile_id,
                                                  action_spec,
                                                  act_fn_hdl);
#endif
          }

          if (move_tail_p) {
            (*move_tail_p)->next = move_node;
            *move_tail_p = move_node;
          } else {
            free_move_list_and_data(&move_node, true);
          }
          curr_expanded_range_entry_count =
              0;  // Reset the expanded range entry count
        }
      }
    }
  }

cleanup:
  if (PJLArray) {
    JLFA(Rc_word, PJLArray);
    (void)Rc_word;
  }
  if (adt_cookie_mem) {
    PIPE_MGR_FREE(adt_cookie_mem);
  }
  if (sel_cookie) {
    PIPE_MGR_FREE(sel_cookie);
  }
  for (i = 0; i < max_expanded_range_entry_count; i++) {
    if (match_specs && match_specs[i]) {
      pipe_mgr_tbl_destroy_match_spec(&match_specs[i]);
    }
  }
  if (match_specs) {
    PIPE_MGR_FREE(match_specs);
  }
  pipe_mgr_tbl_destroy_action_spec(&action_spec);
  return rc;
}

pipe_status_t pipe_mgr_tcam_llp_restore_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  if (move_head_p) {
    *move_head_p = NULL;
  }

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

  rc = tcam_llp_read_shadow_and_restore(tcam_tbl_info,
                                        move_head_p ? &move_tail : NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d - %s (%d - 0x%x) "
        "Error reading shadow mem and restore state rc 0x%x(%s)",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  if (move_head_p) {
    *move_head_p = move_head.next;
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_tcam_cleanup_llp_ha_state(bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  (void)device_id;
  (void)mat_tbl_hdl;
  /* Nothing to cleanup */
  return;
}
