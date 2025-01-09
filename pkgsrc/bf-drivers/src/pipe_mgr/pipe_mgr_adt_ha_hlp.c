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
 * @file pipe_mgr_adt_ha_hlp.c
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
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_adt_tofino.h"

static pipe_status_t pipe_mgr_adt_ha_update_state_for_new_entry(
    pipe_mgr_adt_t *adt,
    bf_dev_pipe_t pipe_id,
    pipe_mgr_adt_move_list_t *move_list_node) {
  pipe_action_data_spec_t *act_data_spec =
      unpack_adt_ent_data_ad(move_list_node->data);
  pipe_act_fn_hdl_t act_fn_hdl =
      unpack_adt_ent_data_afun_hdl(move_list_node->data);
  bool is_const = unpack_adt_ent_data_const(move_list_node->data);
  int num_resources = unpack_adt_ent_data_num_resources(move_list_node->data);
  adt_data_resources_t *resources =
      unpack_adt_ent_data_resources(move_list_node->data);
  pipe_adt_ent_hdl_t adt_ent_hdl = move_list_node->entry_hdl;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_adt_entry_info_t *adt_entry_info = NULL;

  pipe_mgr_adt_data_t *adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
  if (!adt_tbl_data) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  map_sts = bf_map_get(
      &adt_tbl_data->entry_info_htbl, adt_ent_hdl, (void **)&adt_entry_info);
  if (map_sts == BF_MAP_OK) {
    /* There should never be two move list adds for the same entry handle. */
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  if (map_sts == BF_MAP_NO_KEY) {
    adt_entry_info = (pipe_mgr_adt_entry_info_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_adt_entry_info_t));
    if (adt_entry_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    map_sts = bf_map_add(
        &adt_tbl_data->entry_info_htbl, adt_ent_hdl, (void *)adt_entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in adding action entry info for entry handle %d, table "
          "0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          adt_ent_hdl,
          adt->adt_tbl_hdl,
          adt->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    adt_entry_info->pipe_id = pipe_id;
    adt_entry_info->handle = adt_ent_hdl;
  }
  PIPE_MGR_DBGCHK(adt_entry_info->entry_data == NULL);
  adt_entry_info->entry_data = make_adt_ent_data(
      act_data_spec, act_fn_hdl, num_resources, is_const, resources);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_hlp_restore_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_move_list_t *move_list,
    uint32_t *success_count,
    pd_ha_restore_cb_1 cb) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_pipe_ha_hlp_info_t *ha_hlp_info = NULL;
  pipe_mgr_spec_map_t *spec_map;
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t curr_pipe_id = 0, prev_pipe_id = 0;
  uint8_t *action_data_bits = NULL;
  pipe_action_data_spec_t existing_act_data_spec = {0};

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl, with tbl hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_action_data_spec_t *act_data_spec = NULL;

  pipe_mgr_adt_move_list_t *move_list_node = NULL;
  for (move_list_node = move_list; move_list_node;
       move_list_node = move_list_node->next) {
    if (move_list_node->op != PIPE_ADT_UPDATE_ADD) {
      LOG_ERROR(
          "%s:%d Invalid move list node op %d passsed for adt tbl hdl 0x%x, "
          "device id %d for HLP state restore",
          __func__,
          __LINE__,
          move_list_node->op,
          adt_tbl_hdl,
          device_id);
      return PIPE_INVALID_ARG;
    }
    curr_pipe_id = move_list_node->pipe_id;
    if (prev_pipe_id != curr_pipe_id || !adt_tbl_data) {
      if (adt->symmetric) {
        if (move_list_node->pipe_id != BF_DEV_PIPE_ALL) {
          LOG_ERROR(
              "%s:%d Invalid pipe id %d passed for symmetric adt tbl 0x%x, "
              "device id %d",
              __func__,
              __LINE__,
              curr_pipe_id,
              adt_tbl_hdl,
              device_id);
          return PIPE_INVALID_ARG;
        }
        adt_tbl_data = &adt->adt_tbl_data[0];
      } else {
        adt_tbl_data = &adt->adt_tbl_data[curr_pipe_id];
      }
    }
    prev_pipe_id = curr_pipe_id;
    status = pipe_mgr_adt_ha_update_state_for_new_entry(
        adt, curr_pipe_id, move_list_node);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in updating adt HLP state for entry hdl %d, tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          adt_tbl_hdl,
          device_id,
          pipe_str_err(status));
      return status;
    }
    (*success_count)++;
    /* Set the entry handle */
    bf_id_allocator_set(adt_tbl_data->ent_hdl_allocator,
                        move_list_node->entry_hdl);
    ha_hlp_info = adt_tbl_data->ha_hlp_info;
    spec_map = &ha_hlp_info->spec_map;
    if (!pipe_mgr_is_device_virtual(device_id)) {
      /* Add the action data spec to a map to the adt entry handle */
      /* The spec map is populated only for non-virtual devices, since for
       * virtual device the state restore happens through move-lists and any
       * deltas are then replayed in the form of regular APIs outside of
       * warm-init begin and warm-init end
       */
      act_data_spec = unpack_adt_ent_data_ad(move_list_node->data);
      if (act_data_spec->num_action_data_bytes < adt->max_act_data_size) {
        if (!action_data_bits) {
          action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(adt->max_act_data_size,
                                                        sizeof(uint8_t));
        } else {
          PIPE_MGR_MEMSET(
              action_data_bits, 0, adt->max_act_data_size * sizeof(uint8_t));
        }
        /* Cache the existing action data spec */
        PIPE_MGR_MEMCPY(&existing_act_data_spec,
                        act_data_spec,
                        sizeof(pipe_action_data_spec_t));
        PIPE_MGR_MEMCPY(action_data_bits,
                        act_data_spec->action_data_bits,
                        act_data_spec->num_action_data_bytes);
        act_data_spec->action_data_bits = action_data_bits;
        act_data_spec->num_action_data_bytes = adt->max_act_data_size;
      }
      status = pipe_mgr_hitless_ha_new_adt_spec(
          spec_map, move_list_node, adt->max_act_data_size);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in updating adt spec for entry hdl %d, tbl 0x%x, "
            "device "
            "id %d, err %s",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            adt_tbl_hdl,
            device_id,
            pipe_str_err(status));
        return status;
      }
      if (existing_act_data_spec.num_action_data_bytes <
          adt->max_act_data_size) {
        PIPE_MGR_MEMCPY(act_data_spec,
                        &existing_act_data_spec,
                        sizeof(pipe_action_data_spec_t));
      }
    }
    if (cb) {
      cb(sess_hdl,
         device_id,
         adt_tbl_hdl,
         unpack_adt_ent_data_afun_hdl(move_list_node->data),
         0,
         move_list_node->entry_hdl,
         unpack_adt_ent_data_num_resources(move_list_node->data),
         unpack_adt_ent_data_resources(move_list_node->data));
    }
  }
  if (action_data_bits) {
    PIPE_MGR_FREE(action_data_bits);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_mgr_create_hlp_state(
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_idx_t logical_action_idx,
    bool place_entry) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = adt_cookie->adt;
  pipe_mgr_adt_stage_info_t *adt_stage_info = adt_cookie->adt_stage_info;
  pipe_mgr_adt_entry_info_t *adt_entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_adt_ent_idx_t stage_ent_idx =
      logical_action_idx - adt_stage_info->stage_offset;
  pipe_mgr_adt_data_t *adt_tbl_data;

  adt_tbl_data = pipe_mgr_get_adt_instance_from_entry(adt, adt_ent_hdl);
  if (!adt_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  map_sts = bf_map_get(
      &adt_tbl_data->entry_info_htbl, adt_ent_hdl, (void **)&adt_entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    /* This simply should not happen. The ADT hlp restore has already happened,
     * and we have encountered a match entry or selector group that references
     * a non-existent action entry.
     */
    LOG_ERROR(
        "%s:%d Adt entry info for entry hdl %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt->adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  if (place_entry) {
    status = pipe_mgr_adt_update_state_for_entry_placement(
        adt, adt_stage_info, adt_entry_info, &stage_ent_idx, false);

    LOG_TRACE(
        "Dev %d tbl %s 0x%x pipe %x CreateHLP entry %d idx %u (stage %d "
        "local-idx %u) num-refs %u",
        adt->dev_id,
        adt->name,
        adt->adt_tbl_hdl,
        adt_entry_info->pipe_id,
        adt_ent_hdl,
        logical_action_idx,
        adt_stage_info->stage_id,
        stage_ent_idx,
        adt_entry_info->num_references);
  }
  return status;
}

bool pipe_mgr_adt_is_loc_occupied(void) { return false; }

bool pipe_mgr_adt_is_hdl_present_at_loc(void) { return true; }

pipe_status_t pipe_mgr_adt_hlp_compute_delta_changes(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_move_list_t **ml) {
  (void)ml;
  pipe_mgr_adt_t *adt = NULL;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Action data table 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Check the reference type */
  if (adt->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    /* Nothing to be done for direct reference action data tables */
    return PIPE_SUCCESS;
  }
  return PIPE_SUCCESS;
  /*TODO */
}

void pipe_mgr_adt_cleanup_hlp_ha_state(bf_dev_id_t device_id,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Action data table for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return;
  }
  unsigned tbl_idx = 0;
  for (tbl_idx = 0; tbl_idx < adt->num_tbls; tbl_idx++) {
    pipe_mgr_adt_data_t *adt_tbl_data = &adt->adt_tbl_data[tbl_idx];
    if (adt_tbl_data->ha_hlp_info) {
      PIPE_MGR_FREE(adt_tbl_data->ha_hlp_info);
      adt_tbl_data->ha_hlp_info = NULL;
    }
  }
  return;
}

static bool compare_action_data_specs(pipe_action_data_spec_t *ad0,
                                      pipe_action_data_spec_t *ad1) {
  int d;

  if (ad0->num_action_data_bytes != ad1->num_action_data_bytes) return false;
  if (ad0->num_valid_action_data_bits != ad1->num_valid_action_data_bits)
    return false;

  d = PIPE_MGR_MEMCMP(
      ad0->action_data_bits, ad1->action_data_bits, ad0->num_action_data_bytes);
  return (d == 0);
}

/* Return true if action data spec and action function handle are identical. */
bool pipe_mgr_adt_cmp_entries(bf_dev_id_t device_id,
                              pipe_adt_tbl_hdl_t adt_tbl_hdl,
                              pipe_adt_ent_hdl_t ha_adt_ent_hdl,
                              pipe_adt_ent_hdl_t adt_ent_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_entry_info_t *ha_entry;
  pipe_mgr_adt_entry_info_t *entry;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not get the state for the action data table "
        " with handle %d and device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return false;
  }
  ha_entry = pipe_mgr_adt_get_entry_info(adt, ha_adt_ent_hdl);
  if (ha_entry == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting action table HA entry handle 0x%x for tbl "
        "0x%x, device id %d",
        __func__,
        __LINE__,
        ha_adt_ent_hdl,
        adt_tbl_hdl,
        device_id);
    return false;
  }
  entry = pipe_mgr_adt_get_entry_info(adt, adt_ent_hdl);
  if (entry == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting action table entry handle 0x%x for tbl "
        "0x%x, device id %d",
        __func__,
        __LINE__,
        adt_ent_hdl,
        adt_tbl_hdl,
        device_id);
    return false;
  }
  if (ha_entry->entry_data->act_fn_hdl != entry->entry_data->act_fn_hdl)
    return false;
  return compare_action_data_specs(&ha_entry->entry_data->action_data,
                                   &entry->entry_data->action_data);
}
