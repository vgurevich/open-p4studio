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
 * @file pipe_mgr_exm_llp_ha.c
 * @date
 * This file deals with the task of decoding the read EXM entries from hardware
 * and decoding them and preparing the standard move-list, which is then
 * consumed by HLP to rebuild state. Note that no LLP state is reconstructed
 * and that is done, no different than how it is done during regular API
 * handling, i,e.. HLP publishes move-list and LLP executes the move-list.
 * Match entry decoding also involves decoding all the associated resoruces
 * a match entry can refer to either directly or indirectly. Logic to do this
 * is present as well.
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
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_move_list.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_idle.h"

static pipe_status_t pipe_mgr_exm_rebuild_mat_ent_resources(
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_mgr_sel_ha_cookie_t *sel_cookie,
    pipe_mat_ent_idx_t *logical_action_idx,
    pipe_mat_ent_idx_t *logical_sel_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;
  bf_dev_pipe_t pipe_id = exm_tbl_data->pipe_id;
  uint8_t stage_id = exm_stage_info->stage_id;
  dev_target_t dev_tgt_all_pipes = {exm_tbl->dev_id, BF_DEV_PIPE_ALL};

  /* Compute direct resource entry index */
  pipe_mat_ent_idx_t dir_ent_idx =
      pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
          exm_tbl, exm_stage_info, entry_idx);

  /* Now, based on how the match table references a resource table, rebuild
   * the resources it is pointing to. For a directly referenced resource table
   * the resource is attached by default.
   * For indirectly referenced resource tables
   *   1. If the overhead pointers' PFE is disabled AND the compiler has not
   *      defaulted the PFE, resource is detached.
   *   2. If the overhead pointers' PFE is enabled AND the compiler has not
   *      defaulted the PFE, resource is attached.
   *   3. If the compiler has defaulted the PFE, then the resource is attached.
   *   4. If the compiler has forced the full address it is ignored.
   */

  /* DRV-1055 : Always rebuild resources first and only then inform ADT/SEL tbl
   * mgrs
   */

  /* Stats first */
  if (exm_tbl->num_stat_tbl_refs) {
    pipe_stat_tbl_hdl_t stat_tbl_hdl = exm_tbl->stat_tbl_refs[0].tbl_hdl;
    int rsrc_cnt = action_spec->resource_count;
    pipe_stat_ent_idx_t stats_idx = 0;
    bool pfe = false, pfe_defaulted = false;
    action_spec->resources[rsrc_cnt].tbl_hdl = stat_tbl_hdl;
    if (exm_tbl->stat_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      /* No need to fill in the index, since its directly addressed */
      action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      /* Inform stat mgr about this entry */
      dev_target_t dev_tgt;
      dev_tgt.device_id = exm_tbl->dev_id;
      dev_tgt.dev_pipe_id = pipe_id;
      status = pipe_mgr_stat_mgr_add_entry(
          pipe_mgr_get_int_sess_hdl(),
          dev_tgt,
          entry_hdl,
          stat_tbl_hdl,
          stage_id,
          dir_ent_idx,
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
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
            stage_id,
            pipe_str_err(status));
        return status;
      }
      action_spec->resource_count++;
    } else if (exm_tbl->stat_tbl_refs[0].ref_type ==
                   PIPE_TBL_REF_TYPE_INDIRECT &&
               !ptr_info->force_stats) {
      status = pipe_mgr_stat_mgr_decode_virt_addr(exm_tbl->dev_id,
                                                  stat_tbl_hdl,
                                                  pipe_id,
                                                  stage_id,
                                                  indirect_ptrs->stats_ptr,
                                                  &pfe,
                                                  &pfe_defaulted,
                                                  &stats_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding stats virt addr 0x%x, tbl 0x%x, device id "
            "%d, pipe id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->stats_ptr,
            stat_tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
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
  }
  /* Meters next */
  if (exm_tbl->num_meter_tbl_refs) {
    pipe_meter_tbl_hdl_t meter_tbl_hdl = exm_tbl->meter_tbl_refs[0].tbl_hdl;
    int rsrc_cnt = action_spec->resource_count;
    bool pfe = false, pfe_defaulted = false;
    pipe_meter_idx_t meter_idx = 0;

    action_spec->resources[rsrc_cnt].tbl_hdl = meter_tbl_hdl;
    if (exm_tbl->meter_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      /* The meter spec is not reconstructed. Meter RAM entries are just
       * re-written during warm-init end based on API replay.
       */
      action_spec->resource_count++;
    } else if (exm_tbl->meter_tbl_refs[0].ref_type ==
                   PIPE_TBL_REF_TYPE_INDIRECT &&
               !ptr_info->force_meter) {
      status = pipe_mgr_meter_mgr_decode_virt_addr(exm_tbl->dev_id,
                                                   meter_tbl_hdl,
                                                   pipe_id,
                                                   stage_id,
                                                   indirect_ptrs->meter_ptr,
                                                   &pfe,
                                                   &pfe_defaulted,
                                                   &meter_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding meter virt addr 0x%x, tbl 0x%x, device id "
            "%d, pipe id %d stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->meter_ptr,
            meter_tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
            stage_id,
            pipe_str_err(status));
        return status;
      }
      if (pfe_defaulted || (!pfe_defaulted && pfe)) {
        action_spec->resources[rsrc_cnt].tbl_idx = meter_idx;
        action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      } else {
        action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_DETACHED;
      }
      action_spec->resource_count++;
    }
  }
  /* Stateful next */
  if (exm_tbl->num_stful_tbl_refs) {
    pipe_stful_tbl_hdl_t stful_tbl_hdl = exm_tbl->stful_tbl_refs[0].tbl_hdl;
    int rsrc_cnt = action_spec->resource_count;
    bool pfe = false, pfe_defaulted = false;

    action_spec->resources[rsrc_cnt].tbl_hdl = stful_tbl_hdl;
    if (exm_tbl->stful_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      action_spec->resources[rsrc_cnt].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      /* Stful spec is not reconstructed. Stful entries are rewritten during
       * warm init end based on API replay.
       */
      action_spec->resource_count++;
    } else if (exm_tbl->stful_tbl_refs[0].ref_type ==
                   PIPE_TBL_REF_TYPE_INDIRECT &&
               !ptr_info->force_stful) {
      pipe_idx_t stful_idx = 0;
      status = pipe_mgr_stful_mgr_decode_virt_addr(exm_tbl->dev_id,
                                                   stful_tbl_hdl,
                                                   pipe_id,
                                                   stage_id,
                                                   indirect_ptrs->stfl_ptr,
                                                   &pfe,
                                                   &pfe_defaulted,
                                                   &stful_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding stful virt addr 0x%x, tbl 0x%x, device id "
            "%d, pipe id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->stfl_ptr,
            stful_tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
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
  }

  if (exm_tbl->num_sel_tbl_refs && indirect_ptrs->sel_len) {
    pipe_sel_grp_hdl_t sel_grp_hdl;
    pipe_sel_tbl_hdl_t sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;

    status = pipe_mgr_sel_get_temp_sel_grp_hdl(exm_tbl->dev_id,
                                               sel_tbl_hdl,
                                               indirect_ptrs->sel_ptr,
                                               indirect_ptrs->sel_len,
                                               pipe_id,
                                               stage_id,
                                               sel_cookie,
                                               &sel_grp_hdl,
                                               logical_sel_idx);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting temp sel grp hdl for tbl 0x%x, device id %d, "
          "pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          sel_tbl_hdl,
          exm_tbl->dev_id,
          pipe_id,
          stage_id,
          pipe_str_err(status));
      return status;
    }

    status = pipe_mgr_sel_update_llp_state_for_ha(exm_tbl->dev_id,
                                                  sel_tbl_hdl,
                                                  sel_cookie,
                                                  *logical_sel_idx,
                                                  indirect_ptrs->sel_len,
                                                  adt_cookie,
                                                  action_spec,
                                                  act_fn_hdl,
                                                  indirect_ptrs->adt_ptr,
                                                  logical_action_idx);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in updating llp state for sel grp hdl %d, tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          sel_grp_hdl,
          sel_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
    action_spec->pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
    action_spec->sel_grp_hdl = sel_grp_hdl;
  } else if (exm_tbl->num_adt_refs) {
    pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
    if (exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      /* Ask ADT mgr to decode the contents at this entry idx */
      status = pipe_mgr_adt_mgr_decode_to_act_data_spec(
          dev_tgt_all_pipes,
          adt_tbl_hdl,
          exm_stage_info->stage_table_handle,
          dir_ent_idx,
          pipe_id,
          exm_stage_info->stage_id,
          act_fn_hdl,
          act_data_spec,
          adt_cookie,
          false,
          NULL);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding to act data spec for match "
            "entry at idx %d, stage id %d, pipe id %d, tbl 0x%x, "
            "adt entry idx %d, adt tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            entry_idx,
            exm_stage_info->stage_id,
            pipe_id,
            exm_tbl->mat_tbl_hdl,
            dir_ent_idx,
            adt_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        return status;
      }
      action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    } else if (exm_tbl->adt_tbl_refs[0].ref_type ==
               PIPE_TBL_REF_TYPE_INDIRECT) {
      /* For indirectly addressed action table, ask ADT mgr for
       * an action entry handle. This is a temporary entry handle
       * used during this stage of state restore, and will be
       * reconciled with the actual action entry handle during
       * API replay.
       */
      status = pipe_mgr_adt_mgr_get_temp_adt_ent_hdl(exm_tbl->dev_id,
                                                     exm_tbl->direction,
                                                     adt_tbl_hdl,
                                                     indirect_ptrs->adt_ptr,
                                                     pipe_id,
                                                     exm_stage_info->stage_id,
                                                     adt_cookie,
                                                     action_spec,
                                                     act_fn_hdl,
                                                     logical_action_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting adt entry hdl for match entry "
            "at idx %d, stage id %d, pipe id %d, tbl 0x%x, adt "
            "virt add 0x%x, adt tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            entry_idx,
            exm_stage_info->stage_id,
            pipe_id,
            exm_tbl->mat_tbl_hdl,
            indirect_ptrs->adt_ptr,
            adt_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        return status;
      }
      action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;

      /* Ask ADT MGR to update LLP state. State is maintained at the LLP by the
       * adt mgr only for indirectly referenced action data tables.
       */
      status = pipe_mgr_adt_mgr_update_llp_state_for_ha(exm_tbl->dev_id,
                                                        adt_tbl_hdl,
                                                        adt_cookie,
                                                        action_spec,
                                                        act_fn_hdl,
                                                        *logical_action_idx,
                                                        true);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in updating llp state for adt ent hdl %d, tbl 0x%x, "
            "device id %d, err %s",
            __func__,
            __LINE__,
            action_spec->adt_ent_hdl,
            adt_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        return status;
      }
    }
  } else {
    /* If selector table refs and adt refs are both zero, then action
     * has to be direct */
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }
  return PIPE_SUCCESS;
}

static pipe_mat_ent_hdl_t pipe_mgr_exm_ha_alloc_mat_ent_hdl(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  int entry_hdl =
      bf_id_allocator_allocate(exm_tbl_data->ha_llp_info->ent_hdl_allocator);
  if (entry_hdl == -1) {
    return PIPE_MAT_ENT_HDL_INVALID_HDL;
  }
  if (exm_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
    entry_hdl = PIPE_SET_HDL_PIPE(entry_hdl, exm_tbl_data->pipe_id);
  }
  return (pipe_mat_ent_hdl_t)entry_hdl;
}

static pipe_status_t pipe_mgr_exm_llp_populate_move_list(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    pipe_mat_ent_idx_t entry_idx,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_mgr_move_list_t **move_tail_p,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    mem_id_t *mem_id_arr,
    uint32_t num_mem_ids) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_move_list_t *move_list_node = NULL;
  uint32_t ttl = 0;

  /* Allocate move list node */
  move_list_node =
      alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, exm_tbl_data->pipe_id);
  if (move_list_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Populate the move-list */
  if (exm_tbl->idle_present) {
    ttl = NONZERO_TTL;
  }
  move_list_node->data = make_mat_ent_data(match_spec,
                                           action_spec,
                                           act_fn_hdl,
                                           ttl, /* TTL */
                                           indirect_ptrs->sel_len,
                                           0,
                                           0 /* Proxy hash */);
  if (move_list_node->data == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  move_list_node->entry_hdl = mat_ent_hdl;
  if (IS_ACTION_SPEC_ACT_DATA_HDL((action_spec))) {
    move_list_node->adt_ent_hdl = action_spec->adt_ent_hdl;
    move_list_node->adt_ent_hdl_valid = true;
  }
  move_list_node->logical_sel_idx = logical_sel_idx;
  move_list_node->logical_action_idx = logical_action_idx;
  move_list_node->selector_len = indirect_ptrs->sel_len;
  move_list_node->u.single.logical_idx =
      entry_idx + exm_stage_info->stage_offset;

  /* Populate LLP state */
  status = pipe_mgr_exm_update_llp_state(move_list_node,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         indirect_ptrs,
                                         mem_id_arr,
                                         num_mem_ids);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating LLP state for entry hdl %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  if (move_tail_p) {
    (*move_tail_p)->next = move_list_node;
    *move_tail_p = move_list_node;
  } else {
    free_move_list_and_data(&move_list_node, true);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_ha_hash_action_tbl_decode(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_idx_t entry_idx = 0;
  uint32_t num_entries = 0;
  bf_dev_pipe_t pipe_id = 0;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_mat_ent_idx_t dir_ent_idx = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t mat_ent_hdl = 0;
  uintptr_t ent_hdl_p = 0;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_mgr_ent_decode_ptr_info_t ptr_info = {0};
  pipe_mgr_sel_ha_cookie_t *sel_cookie = NULL;
  pipe_idx_t logical_action_idx = 0, logical_sel_idx = 0;

  act_data_spec = &action_spec->act_data;
  num_entries = exm_tbl_data->num_entries;
  exm_stage_info = exm_tbl_data->exm_stage_info;
  act_fn_hdl = exm_tbl->act_fn_hdl_info[0].act_fn_hdl;
  pipe_id = exm_tbl_data->pipe_id;

  if (!exm_tbl->num_adt_refs) return PIPE_SUCCESS;

  /* Based on the action function handle, fill in the info into the
   * action_spec
   */
  act_data_spec->num_valid_action_data_bits =
      exm_tbl->act_fn_hdl_info[0].num_bits;
  act_data_spec->num_action_data_bytes = exm_tbl->act_fn_hdl_info[0].num_bytes;

  PIPE_MGR_MEMSET(action_spec->resources, 0, sizeof(action_spec->resources));
  action_spec->resource_count = 0;
  action_spec->adt_ent_hdl = 0;
  action_spec->sel_grp_hdl = 0;
  action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;

  LOG_DBG("%s Recovered EXM entries tbl 0x%x dev %d pipe %x",
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id);
  for (entry_idx = 0; entry_idx < num_entries; entry_idx++) {
    dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, entry_idx);
    /* Allocate a mat-ent hdl*/
    mat_ent_hdl = pipe_mgr_exm_ha_alloc_mat_ent_hdl(exm_tbl_data);

    /* Now, deal with resources which includes, action, stats, meters,
     * stateful, idle etc.
     */
    status = pipe_mgr_exm_rebuild_mat_ent_resources(mat_ent_hdl,
                                                    exm_tbl,
                                                    exm_tbl_data,
                                                    exm_stage_info,
                                                    dir_ent_idx,
                                                    &indirect_ptrs,
                                                    &ptr_info,
                                                    action_spec,
                                                    act_fn_hdl,
                                                    adt_cookie,
                                                    sel_cookie,
                                                    &logical_action_idx,
                                                    &logical_sel_idx);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in rebuilding mat ent resources for entry at "
          "idx %d, tbl 0x%x, pipe id %d, stage id %d, device id %d, "
          "err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          pipe_id,
          exm_stage_info->stage_id,
          exm_tbl->dev_id,
          pipe_str_err(status));
      goto err_cleanup;
    }

    /* Re-initialize match spec */
    PIPE_MGR_MEMSET(match_spec->match_value_bits,
                    0,
                    sizeof(uint8_t) * exm_tbl->num_match_spec_bytes);
    /* Recover the match spec from the entry index */
    status = bf_hash_mat_entry_hash_action_match_spec_decode_from_hash(
        exm_tbl->dev_id,
        exm_tbl->profile_id,
        exm_stage_info->stage_id,
        exm_tbl->mat_tbl_hdl,
        dir_ent_idx,
        false /*proxy hash*/,
        match_spec);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding match spec from entry idx %d, tbl 0x%x, "
          "stage id %d, device id %d, "
          "err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_stage_info->stage_id,
          exm_tbl->dev_id,
          pipe_str_err(status));
      goto err_cleanup;
    }

    /* In case of field slices whole field is part of match spec, but
     * only slice is part of hash, hence encode will return less bits.
     * For single slice it is ok, because rest is 0 anyways, but need
     * to correct mspec. */
    match_spec->num_valid_match_bits = exm_tbl->num_match_spec_bits;

    status = pipe_mgr_exm_llp_populate_move_list(exm_tbl,
                                                 exm_tbl_data,
                                                 exm_stage_info,
                                                 mat_ent_hdl,
                                                 logical_action_idx,
                                                 logical_sel_idx,
                                                 dir_ent_idx,
                                                 act_fn_hdl,
                                                 match_spec,
                                                 action_spec,
                                                 move_tail_p,
                                                 &indirect_ptrs,
                                                 NULL,
                                                 0);
    if (status != PIPE_SUCCESS) {
      goto err_cleanup;
    }
    ent_hdl_p = (uintptr_t)mat_ent_hdl;
    map_sts = bf_map_add(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                         dir_ent_idx,
                         (void *)ent_hdl_p);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in inserting log dir idx to entry hdl mapping for idx "
          "%d, hdl 0x%x, tbl 0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          entry_idx,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      status = PIPE_UNEXPECTED;
      goto err_cleanup;
    }

    LOG_DBG("Entry Index %d:", entry_idx);
    pipe_mgr_entry_format_log_match_spec(exm_tbl->dev_id,
                                         BF_LOG_DBG,
                                         exm_tbl->profile_id,
                                         exm_tbl->mat_tbl_hdl,
                                         match_spec);
    pipe_mgr_entry_format_log_action_spec(exm_tbl->dev_id,
                                          BF_LOG_DBG,
                                          exm_tbl->profile_id,
                                          action_spec,
                                          act_fn_hdl);
  }

err_cleanup:

  return status;
}

static pipe_status_t pipe_mgr_exm_llp_read_shadow_and_restore(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_mat_ent_idx_t entry_idx = 0, dir_ent_idx = 0;
  pipe_mgr_exm_hash_info_for_decode_t hash_info = {0};
  dev_stage_t stage_id = 0;
  unsigned i = 0, j = 0, k = 0;
  bf_dev_pipe_t pipe_id = 0;
  uint32_t ram_line = 0;
  uint32_t num_sub_entries = 0;
  uint32_t num_ram_units = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  pipe_mgr_adt_ha_cookie_t *adt_cookie_mem = NULL;
  pipe_mgr_adt_ha_cookie_t *adt_cookie = NULL;
  pipe_mgr_sel_ha_cookie_t *sel_cookie = NULL;
  pipe_tbl_match_spec_t match_spec = {0};
  pipe_action_spec_t action_spec = {0};
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_mgr_ent_decode_ptr_info_t ptr_info = {0};
  pipe_adt_tbl_hdl_t adt_tbl_hdl = 0;
  pipe_sel_tbl_hdl_t sel_tbl_hdl = 0;
  uint8_t **exm_word = NULL;
  PWord_t PValue = NULL;
  Pvoid_t PJLArray = NULL;
  Word_t Rc_word;
  pipe_mgr_move_list_t *move_list_head = NULL;
  pipe_act_fn_info_t *act_fn_info = NULL;
  pipe_idx_t logical_action_idx = 0, logical_sel_idx = 0;
  unsigned subentry = 0;
  uint8_t version_valid_bits = 0;
  bool *ram_ids = NULL;
  mem_id_t *mem_id_arr = NULL;

  act_fn_info = exm_tbl->act_fn_hdl_info;
  if (exm_tbl->num_adt_refs) {
    adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
    adt_cookie_mem = (pipe_mgr_adt_ha_cookie_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_adt_ha_cookie_t));
    if (adt_cookie_mem == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }
  if (exm_tbl->num_sel_tbl_refs) {
    sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;
    sel_cookie = (pipe_mgr_sel_ha_cookie_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_sel_ha_cookie_t));
    if (sel_cookie == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }
  if (exm_tbl->max_act_data_size) {
    action_spec.act_data.action_data_bits =
        (uint8_t *)PIPE_MGR_CALLOC(exm_tbl->max_act_data_size, sizeof(uint8_t));
    if (action_spec.act_data.action_data_bits == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      status = PIPE_NO_SYS_RESOURCES;
      goto err_cleanup;
    }
  }
  for (i = 0; i < exm_tbl->num_actions; i++) {
    /* Maintain a mapping from act_fn_hdl to the idx within the array of act
     * data spec for fast access.
     */
    JLI(PValue, PJLArray, act_fn_info[i].act_fn_hdl);
    *PValue = i;
  }
  match_spec.match_value_bits = (uint8_t *)PIPE_MGR_CALLOC(
      exm_tbl->num_match_spec_bytes, sizeof(uint8_t));
  if (match_spec.match_value_bits == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    status = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }
  match_spec.match_mask_bits = (uint8_t *)PIPE_MGR_CALLOC(
      exm_tbl->num_match_spec_bytes, sizeof(uint8_t));
  if (match_spec.match_mask_bits == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    status = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }
  PIPE_MGR_MEMSET(
      match_spec.match_mask_bits, 0xFF, exm_tbl->num_match_spec_bytes);

  match_spec.num_valid_match_bits = exm_tbl->num_match_spec_bits;
  match_spec.num_match_bytes = exm_tbl->num_match_spec_bytes;

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    pipe_id = exm_tbl_data->pipe_id;
    if (exm_tbl->symmetric) {
      pipe_id = exm_tbl->lowest_pipe_id;
    }
    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      exm_stage_info = &exm_tbl_data->exm_stage_info[j];
      stage_id = exm_stage_info->stage_id;
      num_sub_entries = exm_stage_info->pack_format->num_entries_per_wide_word;
      if (exm_word) {
        PIPE_MGR_FREE(exm_word);
        exm_word = NULL;
      }
      num_entries_per_wide_word_blk = (TOF_SRAM_UNIT_DEPTH * num_sub_entries);
      num_ram_units = exm_stage_info->pack_format->num_rams_in_wide_word;
      ram_ids = (bool *)PIPE_MGR_CALLOC(num_ram_units, sizeof(bool));
      if (ram_ids == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        goto err_cleanup;
      }
      mem_id_arr = (mem_id_t *)PIPE_MGR_CALLOC(num_ram_units, sizeof(mem_id_t));
      if (mem_id_arr == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        goto err_cleanup;
      }

      exm_word = (uint8_t **)PIPE_MGR_CALLOC(num_ram_units, sizeof(uint8_t *));
      if (exm_word == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        goto err_cleanup;
      }
      if (exm_tbl->num_adt_refs) {
        adt_cookie = adt_cookie_mem;
        status = pipe_mgr_adt_mgr_ha_get_cookie(
            exm_tbl->dev_id, adt_tbl_hdl, pipe_id, stage_id, adt_cookie);
        if (status == PIPE_OBJ_NOT_FOUND) {
          /* Not all stages need to use ADT Table data. Immediate action data
           * could be used in some stages.
           */
          LOG_TRACE(
              "%s:%d Not found adt mgr ha cookie for tbl 0x%x device id "
              "%d, pipe id %d, stage id %d",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              exm_tbl->dev_id,
              pipe_id,
              stage_id);
          adt_cookie = NULL;
          status = PIPE_SUCCESS;
        } else if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in getting adt mgr ha cookie for tbl 0x%x device id "
              "%d, pipe id %d, stage id %d, err %s",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              exm_tbl->dev_id,
              pipe_id,
              stage_id,
              pipe_str_err(status));
          goto err_cleanup;
        }
      }
      if (exm_tbl->num_sel_tbl_refs) {
        status = pipe_mgr_sel_ha_get_cookie(
            exm_tbl->dev_id, sel_tbl_hdl, pipe_id, stage_id, sel_cookie);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in getting sel mgr ha cookie for tbl 0x%x device id "
              "%d, pipe id %d, stage id %d, err %s",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              exm_tbl->dev_id,
              pipe_id,
              stage_id,
              pipe_str_err(status));
          goto err_cleanup;
        }
      }
      if (exm_tbl->hash_action) {
        // Special handling for hash action tables
        status = pipe_mgr_exm_ha_hash_action_tbl_decode(exm_tbl,
                                                        exm_tbl_data,
                                                        adt_cookie,
                                                        &match_spec,
                                                        &action_spec,
                                                        move_tail_p);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in decoding for hash action for tbl 0x%x device id "
              "%d, pipe id %d, stage id %d, err %s",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              exm_tbl->dev_id,
              pipe_id,
              stage_id,
              pipe_str_err(status));
          goto err_cleanup;
        }
        continue;
      }
      for (k = 0; k < exm_stage_info->num_hash_ways; k++) {
        exm_hashway_data = &exm_stage_info->hashway_data[k];
        ram_alloc_info = exm_hashway_data->ram_alloc_info;
        /* Now read each line of this hash-way and decode all possible
         * sub-entries.
         */
        uint32_t wide_word_blk_idx = 0;
        for (wide_word_blk_idx = 0;
             wide_word_blk_idx < ram_alloc_info->num_wide_word_blks;
             wide_word_blk_idx++) {
          uint32_t mem_id_idx = 0;
          for (ram_line = 0; ram_line < TOF_SRAM_UNIT_DEPTH; ram_line++) {
            /* For each ram-line get the wide-word */
            for (mem_id_idx = 0; mem_id_idx < num_ram_units; mem_id_idx++) {
              mem_id_t mem_id = ram_alloc_info->tbl_word_blk[wide_word_blk_idx]
                                    .mem_id[mem_id_idx];
              status = pipe_mgr_phy_mem_map_get_ref(exm_tbl->dev_id,
                                                    exm_tbl->direction,
                                                    pipe_mem_type_unit_ram,
                                                    pipe_id,
                                                    stage_id,
                                                    mem_id,
                                                    ram_line,
                                                    &exm_word[mem_id_idx],
                                                    true);

              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in getting shadow memory ref for ram id %d, "
                    "stage id %d, pipe id %d, for exm tbl 0x%x, device id %d, "
                    "err %s",
                    __func__,
                    __LINE__,
                    mem_id,
                    stage_id,
                    pipe_id,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    pipe_str_err(status));
                goto err_cleanup;
              }
            }
            /* For each wide-word, decode all possible sub-entries */
            for (subentry = 0; subentry < num_sub_entries; subentry++) {
              /* Re-initialize match spec and action spec */
              PIPE_MGR_MEMSET(match_spec.match_value_bits,
                              0,
                              sizeof(uint8_t) * exm_tbl->num_match_spec_bytes);
              /* Re-initialize action spec */
              if (action_spec.act_data.action_data_bits) {
                PIPE_MGR_MEMSET(action_spec.act_data.action_data_bits,
                                0,
                                exm_tbl->max_act_data_size);
              }
              PIPE_MGR_MEMSET(
                  action_spec.resources, 0, sizeof(action_spec.resources));
              action_spec.resource_count = 0;
              action_spec.adt_ent_hdl = 0;
              action_spec.sel_grp_hdl = 0;
              action_spec.resource_count = 0;

              entry_idx = (ram_line * num_sub_entries) +
                          (wide_word_blk_idx * num_entries_per_wide_word_blk) +
                          subentry + exm_hashway_data->offset;
              pipe_mgr_exm_compute_entry_details_from_location(exm_tbl,
                                                               exm_stage_info,
                                                               exm_hashway_data,
                                                               entry_idx,
                                                               &hash_info,
                                                               NULL,
                                                               NULL);

              /* Re-initialze the ram_id boolean array */
              PIPE_MGR_MEMSET(ram_ids, 0, sizeof(bool) * num_ram_units);
              PIPE_MGR_MEMSET(&indirect_ptrs, 0, sizeof(indirect_ptrs));

              status =
                  pipe_mgr_entry_format_tof_exm_tbl_ent_decode_to_components(
                      exm_tbl->dev_id,
                      exm_tbl->profile_id,
                      stage_id,
                      exm_tbl->mat_tbl_hdl,
                      exm_stage_info->stage_table_handle,
                      subentry,
                      &version_valid_bits,
                      &match_spec,
                      &action_spec.act_data,
                      exm_word,
                      &hash_info,
                      &indirect_ptrs,
                      &ptr_info,
                      &act_fn_hdl,
                      ram_ids,
                      false,
                      NULL /* proxy_hash */);
              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in decoding to match spec & action spec for "
                    "entry idx %d, stage id %d, pipe id %d, tbl 0x%x, device "
                    "id %d, err %s",
                    __func__,
                    __LINE__,
                    entry_idx,
                    stage_id,
                    pipe_id,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    pipe_str_err(status));
                goto err_cleanup;
              }
              if (version_valid_bits == RMT_EXM_ENTRY_VERSION_INVALID) {
                /* The entry does not exist. */
                continue;
              }
              /* Clean up the indirect pointer values if they were forced. */
              if (ptr_info.force_stats) indirect_ptrs.stats_ptr = 0;
              if (ptr_info.force_meter) indirect_ptrs.meter_ptr = 0;
              if (ptr_info.force_stful) indirect_ptrs.stfl_ptr = 0;

              /* Based on the action function handle, fill in the info into the
               * action_spec
               */
              JLG(PValue, PJLArray, act_fn_hdl);
              PIPE_MGR_ASSERT(PValue);
              action_spec.act_data.num_valid_action_data_bits =
                  act_fn_info[*PValue].num_bits;
              action_spec.act_data.num_action_data_bytes =
                  act_fn_info[*PValue].num_bytes;

              /* Allocate a mat-ent hdl */
              pipe_mat_ent_hdl_t mat_ent_hdl = 0;
              mat_ent_hdl = pipe_mgr_exm_ha_alloc_mat_ent_hdl(exm_tbl_data);

              /* Now, deal with resources which includes, action, stats, meters,
               * stateful, idle etc.
               */
              status =
                  pipe_mgr_exm_rebuild_mat_ent_resources(mat_ent_hdl,
                                                         exm_tbl,
                                                         exm_tbl_data,
                                                         exm_stage_info,
                                                         entry_idx,
                                                         &indirect_ptrs,
                                                         &ptr_info,
                                                         &action_spec,
                                                         act_fn_hdl,
                                                         adt_cookie,
                                                         sel_cookie,
                                                         &logical_action_idx,
                                                         &logical_sel_idx);
              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in rebuilding mat ent resources for entry at "
                    "idx %d, tbl 0x%x, pipe id %d, stage id %d, device id %d, "
                    "err %s",
                    __func__,
                    __LINE__,
                    entry_idx,
                    exm_tbl->mat_tbl_hdl,
                    pipe_id,
                    stage_id,
                    exm_tbl->dev_id,
                    pipe_str_err(status));
                goto err_cleanup;
              }

              unsigned idx = 0, ii = 0;
              for (ii = 0; ii < num_ram_units; ii++) {
                if (ram_ids[ii]) {
                  mem_id_arr[idx++] =
                      ram_alloc_info->tbl_word_blk[wide_word_blk_idx]
                          .mem_id[ii];
                }
              }
              uint32_t num_mem_ids = idx;

              status = pipe_mgr_exm_llp_populate_move_list(exm_tbl,
                                                           exm_tbl_data,
                                                           exm_stage_info,
                                                           mat_ent_hdl,
                                                           logical_action_idx,
                                                           logical_sel_idx,
                                                           entry_idx,
                                                           act_fn_hdl,
                                                           &match_spec,
                                                           &action_spec,
                                                           move_tail_p,
                                                           &indirect_ptrs,
                                                           mem_id_arr,
                                                           num_mem_ids);
              if (status != PIPE_SUCCESS) {
                goto err_cleanup;
              }

#ifdef BF_HITLESS_HA_DEBUG
              LOG_DBG(
                  "Dev %d Table %s (0x%x) Pipe %x, recovered match and action "
                  "spec:",
                  exm_tbl->dev_id,
                  exm_tbl->name,
                  exm_tbl->mat_tbl_hdl,
                  exm_tbl_data->pipe_id);
              pipe_mgr_entry_format_log_match_spec(exm_tbl->dev_id,
                                                   BF_LOG_DBG,
                                                   exm_tbl->profile_id,
                                                   exm_tbl->mat_tbl_hdl,
                                                   &match_spec);
              pipe_mgr_entry_format_log_action_spec(exm_tbl->dev_id,
                                                    BF_LOG_DBG,
                                                    exm_tbl->profile_id,
                                                    &action_spec,
                                                    act_fn_hdl);
#endif

              dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
                  exm_tbl, exm_stage_info, entry_idx);
              uintptr_t ent_hdl_p = mat_ent_hdl;
              map_sts = bf_map_add(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                                   dir_ent_idx,
                                   (void *)ent_hdl_p);
              if (map_sts != BF_MAP_OK) {
                LOG_ERROR(
                    "%s:%d Error in inserting log dir idx to entry hdl mapping "
                    "for idx %d, hdl 0x%x, tbl 0x%x, device id %d, err 0x%x",
                    __func__,
                    __LINE__,
                    entry_idx,
                    mat_ent_hdl,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    map_sts);
                status = PIPE_UNEXPECTED;
                goto err_cleanup;
              }
            }
          }
        }
      }
      if (ram_ids) {
        PIPE_MGR_FREE(ram_ids);
        ram_ids = NULL;
      }
      if (mem_id_arr) {
        PIPE_MGR_FREE(mem_id_arr);
        mem_id_arr = NULL;
      }
    }
  }
err_cleanup:
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
  if (action_spec.act_data.action_data_bits) {
    PIPE_MGR_FREE(action_spec.act_data.action_data_bits);
  }
  if (match_spec.match_value_bits) {
    PIPE_MGR_FREE(match_spec.match_value_bits);
  }
  if (match_spec.match_mask_bits) {
    PIPE_MGR_FREE(match_spec.match_mask_bits);
  }
  if (exm_word) {
    PIPE_MGR_FREE(exm_word);
  }
  if (status != PIPE_SUCCESS) {
    /* Need to cleanup the move-list nodes produced */
    free_move_list_and_data(&move_list_head, true);
  }
  return status;
}

pipe_status_t pipe_mgr_exm_llp_restore_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl for tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  if (move_head_p) {
    *move_head_p = NULL;
  }
  status = pipe_mgr_exm_llp_read_shadow_and_restore(
      exm_tbl, move_head_p ? &move_tail : NULL);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in reading shadow memory and restoring state at LLP for "
        "tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_id,
        pipe_str_err(status));
    return status;
  }
  if (move_head_p) {
    *move_head_p = move_head.next;
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_exm_cleanup_llp_ha_state(bf_dev_id_t device_id,
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
    if (!exm_tbl_data->ha_llp_info) {
      return;
    }
    bf_id_allocator_destroy(exm_tbl_data->ha_llp_info->ent_hdl_allocator);
    PIPE_MGR_FREE(exm_tbl_data->ha_llp_info);
    exm_tbl_data->ha_llp_info = NULL;
  }
  return;
}
