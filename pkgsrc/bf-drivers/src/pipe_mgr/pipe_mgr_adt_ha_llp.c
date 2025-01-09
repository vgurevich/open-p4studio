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
 * @file pipe_mgr_adt_ha_llp.c
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
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hw_dump.h"

static bool pipe_mgr_adt_mgr_ent_idx_assigned_to_temp_ent_hdl(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t entry_idx,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p) {
  pipe_mgr_adt_stage_ha_llp_info_t *ha_stage_info = adt_stage_info->ha_llp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = entry_idx;
  pipe_adt_ent_hdl_t *entry_hdl = NULL;
  if (!ha_stage_info->ent_idx_to_ent_hdl) {
    return false;
  }

  map_sts =
      bf_map_get(&ha_stage_info->ent_idx_to_ent_hdl, key, (void **)&entry_hdl);
  if (map_sts == BF_MAP_NO_KEY) {
    return false;
  }
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry hdl from entry idx %d, stage id %d, err "
        "0x%x",
        __func__,
        __LINE__,
        entry_idx,
        adt_stage_info->stage_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    return false;
  }
  *adt_ent_hdl_p = *entry_hdl;
  return true;
}

static pipe_adt_ent_hdl_t pipe_mgr_adt_mgr_get_new_temp_adt_ent_hdl(
    pipe_mgr_adt_data_t *adt_tbl_data) {
  int handle = 0;
  pipe_mgr_adt_pipe_ha_llp_info_t *ha_info = adt_tbl_data->ha_llp_info;
  handle = bf_id_allocator_allocate(ha_info->ent_hdl_allocator);
  if (handle == -1) {
    return PIPE_ADT_ENT_HDL_INVALID_HDL;
  }
  if (adt_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
    handle = PIPE_SET_HDL_PIPE(handle, adt_tbl_data->pipe_id);
  }
  return (pipe_adt_ent_hdl_t)handle;
}

static void pipe_mgr_adt_mgr_release_temp_adt_ent_hdl(
    pipe_mgr_adt_data_t *adt_tbl_data, pipe_adt_ent_hdl_t adt_ent_hdl) {
  pipe_mgr_adt_pipe_ha_llp_info_t *ha_info = adt_tbl_data->ha_llp_info;
  bf_id_allocator_release(ha_info->ent_hdl_allocator,
                          PIPE_GET_HDL_VAL(adt_ent_hdl));
}

static pipe_status_t pipe_mgr_adt_mgr_assign_temp_ent_hdl_to_ent_idx(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t entry_idx,
    pipe_adt_ent_hdl_t adt_ent_hdl) {
  pipe_mgr_adt_stage_ha_llp_info_t *ha_stage_info = adt_stage_info->ha_llp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = entry_idx;
  pipe_adt_ent_hdl_t *entry_hdl =
      PIPE_MGR_CALLOC(1, sizeof(pipe_adt_ent_hdl_t));
  if (entry_hdl == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  (*entry_hdl) = adt_ent_hdl;
  map_sts =
      bf_map_add(&ha_stage_info->ent_idx_to_ent_hdl, key, (void *)entry_hdl);
  if (map_sts == BF_MAP_KEY_EXISTS) {
    LOG_ERROR(
        "%s:%d Entry idx to entry hdl mapping already exists for entry idx %d, "
        "stage id %d",
        __func__,
        __LINE__,
        entry_idx,
        adt_stage_info->stage_id);
    if (entry_hdl) {
      PIPE_MGR_FREE(entry_hdl);
    }
    return PIPE_ALREADY_EXISTS;
  }
  return PIPE_SUCCESS;
}

/* This API is intended to be used by match tbl managers during HA state restore
 * at LLP. This enables fast access to adt structures while in a loop of
 * restoring match entries and matching it with action data entries or
 * action entry handles.
 */
pipe_status_t pipe_mgr_adt_mgr_ha_get_cookie(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (adt->symmetric) {
    if (pipe_id != adt->lowest_pipe_id && pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for a symmetric ADT tbl 0x%x, "
          "device id %d",
          __func__,
          __LINE__,
          pipe_id,
          adt_tbl_hdl,
          device_id);
      return PIPE_INVALID_ARG;
    }
    adt_tbl_data = &adt->adt_tbl_data[0];
  } else {
    adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
    if (adt_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Adt tbl instance for pipe id %d, for tbl 0x%x, device id %d "
          "not found",
          __func__,
          __LINE__,
          pipe_id,
          adt_tbl_hdl,
          device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
  if (adt_stage_info == NULL) {
    /* Immediate action data might be used in this stage. */
    LOG_TRACE(
        "%s:%d Adt stage info for stage id %d, pipe id %d, tbl 0x%x, device id "
        "%d not found",
        __func__,
        __LINE__,
        stage_id,
        pipe_id,
        adt_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  adt_ha_cookie->adt = adt;
  adt_ha_cookie->adt_tbl_data = adt_tbl_data;
  adt_ha_cookie->adt_stage_info = adt_stage_info;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_mgr_decode_to_act_data_spec(
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    rmt_tbl_hdl_t stage_tbl_hdl,
    pipe_adt_ent_idx_t adt_entry_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    bool from_hw,
    pipe_sess_hdl_t *sess_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint8_t num_rams_in_wide_word = 0;
  mem_id_t mem_id = 0;
  uint8_t **shadow_ptrs = NULL;
  uint8_t **adt_word_ptrs = NULL;

  if (adt_ha_cookie) {
    adt = adt_ha_cookie->adt;
    adt_tbl_data = adt_ha_cookie->adt_tbl_data;
    adt_stage_info = adt_ha_cookie->adt_stage_info;
  } else {
    adt = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
    if (adt == NULL) {
      LOG_ERROR("%s:%d Adt tbl info for device id %d, tbl 0x%x not found",
                __func__,
                __LINE__,
                dev_tgt.device_id,
                adt_tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (adt->symmetric) {
      if (pipe_id != adt->lowest_pipe_id && pipe_id != BF_DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Invalid entry pipe id %d passed for a symmetrict ADT tbl "
            "0x%x, device id %d",
            __func__,
            __LINE__,
            pipe_id,
            adt_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }
      adt_tbl_data = &adt->adt_tbl_data[0];
    } else {
      adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
      if (adt_tbl_data == NULL) {
        LOG_ERROR(
            "%s:%d Adt tbl instance for entry pipe id %d, for tbl 0x%x, device "
            "id %d not found",
            __func__,
            __LINE__,
            pipe_id,
            adt_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
    }
    if ((dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) &&
        (!PIPE_BITMAP_GET(&adt_tbl_data->pipe_bmp, dev_tgt.dev_pipe_id))) {
      LOG_TRACE(
          "%s:%d Invalid request to access pipe %x for table %s "
          "0x%x device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          adt->name,
          adt->adt_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
    if (adt_stage_info == NULL) {
      /* It is possible for an action table to be missing in a stage if all data
       * made it to immediate fields. There is nothing to do in this case.
       */
      return PIPE_SUCCESS;
    }
  }

  adt_loc_info_t loc_info = {0};
  status = pipe_mgr_adt_get_phy_loc_info(
      adt_stage_info, stage_tbl_hdl, adt_entry_idx, &loc_info);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting adt entry location for idx %d, tbl 0x%x, "
        "device id %d, stage tbl hdl %d, stage id %d, pipe id %d, err %s",
        __func__,
        __LINE__,
        adt_entry_idx,
        adt_tbl_hdl,
        dev_tgt.device_id,
        stage_tbl_hdl,
        stage_id,
        pipe_id,
        pipe_str_err(status));
    goto err_cleanup;
  }
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  bf_dev_pipe_t pipe = 0;
  /* Read from dev_tgt.dev_pipe_id is requested.
   * The value of pipe_id is from either exact or ternary match table instance.
   */
  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    if (adt_tbl_data->pipe_id == BF_DEV_PIPE_ALL) {
      pipe = adt->lowest_pipe_id;
    } else {
      pipe = pipe_id;
    }
  } else {
    pipe = dev_tgt.dev_pipe_id;
  }

  unsigned i = 0;
  if (!from_hw) {
    shadow_ptrs = adt_stage_info->shadow_ptr_arr;
    for (i = 0; i < num_rams_in_wide_word; i++) {
      mem_id =
          ram_alloc_info->tbl_word_blk[loc_info.wide_word_blk_idx].mem_id[i];
      status = pipe_mgr_phy_mem_map_get_ref(dev_tgt.device_id,
                                            adt->direction,
                                            pipe_mem_type_unit_ram,
                                            pipe,
                                            stage_id,
                                            mem_id,
                                            loc_info.ram_line_num,
                                            &shadow_ptrs[i],
                                            true);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting shadow mem ref for adt ram %d, line %d, "
            "pipe "
            "id %d, stage id %d, for adt tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            mem_id,
            loc_info.ram_line_num,
            pipe,
            stage_id,
            adt_tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(status));
        return status;
      }
    }
  } else {
    bf_dev_pipe_t phy_pipe_id;
    uint64_t phy_addrs[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
    status = pipe_mgr_map_pipe_id_log_to_phy(adt->dev_info, pipe, &phy_pipe_id);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
          __func__,
          __LINE__,
          pipe,
          dev_tgt.device_id,
          pipe_str_err(status));
      return status;
    }
    adt_word_ptrs =
        (uint8_t **)PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(uint8_t *));
    if (adt_word_ptrs == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      status = PIPE_NO_SYS_RESOURCES;
      goto err_cleanup;
    }
    for (i = 0; i < num_rams_in_wide_word; i++) {
      phy_addrs[i] = adt->dev_info->dev_cfg.get_full_phy_addr(
          adt->direction,
          phy_pipe_id,
          stage_id,
          ram_alloc_info->tbl_word_blk[loc_info.wide_word_blk_idx].mem_id[i],
          loc_info.ram_line_num,
          pipe_mem_type_unit_ram);
      adt_word_ptrs[i] =
          (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
      if (adt_word_ptrs[i] == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        status = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
    status = pipe_mgr_dump_any_tbl_by_addr(dev_tgt.device_id,
                                           subdev,
                                           adt_tbl_hdl,
                                           0,
                                           stage_id,
                                           RMT_TBL_TYPE_ACTION_DATA,
                                           phy_addrs,
                                           num_rams_in_wide_word,
                                           loc_info.sub_entry,
                                           act_fn_hdl,
                                           NULL,
                                           NULL,
                                           0,
                                           adt_word_ptrs,
                                           NULL,
                                           sess_hdl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in dumping adt entry from hw at index %d, "
          " adt tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          adt_entry_idx,
          adt_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
  }

  status = pipe_mgr_entry_format_tof_adt_tbl_ent_decode_to_action_spec(
      adt->dev_info,
      adt->profile_id,
      stage_id,
      adt_tbl_hdl,
      act_fn_hdl,
      loc_info.sub_entry,
      act_data_spec,
      from_hw ? adt_word_ptrs : shadow_ptrs);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding adt entry at idx %d, pipe id %d, stage id %d, "
        "tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        adt_entry_idx,
        pipe_id,
        stage_id,
        adt_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(status));
    goto err_cleanup;
  }
err_cleanup:
  if (adt_word_ptrs) {
    for (i = 0; i < num_rams_in_wide_word; i++) {
      if (adt_word_ptrs[i]) {
        PIPE_MGR_FREE(adt_word_ptrs[i]);
      }
    }
    PIPE_MGR_FREE(adt_word_ptrs);
  }
  return status;
}

pipe_status_t pipe_mgr_adt_mgr_decode_virt_addr(
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    rmt_virt_addr_t virt_addr,
    pipe_adt_ent_idx_t *logical_action_idx) {
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint8_t num_entries_per_wide_word = 0;
  uint32_t num_rams_in_wide_word = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t adt_entry_width = 0;
  uint8_t wide_word_blk_idx = 0;
  uint32_t ram_line_num = 0;
  vpn_id_t vpn = 0;
  uint32_t entry_position = 0;
  uint32_t i;

  adt_stage_info = adt_cookie->adt_stage_info;
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;

  if (num_rams_in_wide_word == 1) {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH / num_entries_per_wide_word) / 8;
  } else {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH * num_rams_in_wide_word) / 8;
  }

  pipe_mgr_adt_tof_unbuild_virt_addr(
      virt_addr, adt_entry_width, &ram_line_num, &vpn, &entry_position);
  /* From VPN, figure out the wide word blk idx */
  for (i = 0; i < ram_alloc_info->num_wide_word_blks; i++) {
    if (ram_alloc_info->tbl_word_blk[i].vpn_id[0] == vpn) {
      wide_word_blk_idx = i;
      break;
    }
  }
  if (i == ram_alloc_info->num_wide_word_blks) {
    LOG_ERROR(
        "%s:%d Invalid VPN %d for action tbl 0x%x, device id %d, stage id "
        "%d, virt addr 0x%x",
        __func__,
        __LINE__,
        vpn,
        adt_cookie->adt->adt_tbl_hdl,
        adt_cookie->adt->dev_id,
        adt_stage_info->stage_id,
        virt_addr);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  /* Compute the logical action idx */
  *logical_action_idx = (ram_line_num * num_entries_per_wide_word) +
                        (num_entries_per_wide_word_blk * wide_word_blk_idx) +
                        entry_position;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_mgr_get_temp_adt_ent_hdl(
    bf_dev_id_t device_id,
    pipe_tbl_dir_t gress,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    rmt_virt_addr_t virt_addr,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_idx_t *logical_action_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_adt_ent_idx_t adt_entry_idx = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint8_t num_entries_per_wide_word = 0;
  uint8_t num_rams_in_wide_word = 0;
  uint32_t adt_entry_width = 0;
  uint8_t wide_word_blk_idx = 0;
  mem_id_t mem_id = 0;
  uint8_t **shadow_ptrs = NULL;
  uint32_t entry_position = 0;
  unsigned i = 0;
  vpn_id_t vpn = 0;
  uint32_t ram_line_num = 0;
  pipe_action_data_spec_t *act_data_spec = &(action_spec->act_data);

  if (adt_ha_cookie) {
    adt = adt_ha_cookie->adt;
    adt_tbl_data = adt_ha_cookie->adt_tbl_data;
    adt_stage_info = adt_ha_cookie->adt_stage_info;
    if (adt->symmetric) {
      pipe_id = adt->lowest_pipe_id;
    } else {
      pipe_id = adt_tbl_data->pipe_id;
    }
  } else {
    adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
    if (adt == NULL) {
      LOG_ERROR("%s:%d Adt tbl info for device id %d, tbl 0x%x not found",
                __func__,
                __LINE__,
                device_id,
                adt_tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (adt->symmetric) {
      if (pipe_id != adt->lowest_pipe_id && pipe_id != BF_DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Invalid pipe id %d passed for a symmetrict ADT tbl 0x%x, "
            "device id %d",
            __func__,
            __LINE__,
            pipe_id,
            adt_tbl_hdl,
            device_id);
        return PIPE_INVALID_ARG;
      }
      adt_tbl_data = &adt->adt_tbl_data[0];
      pipe_id = adt->lowest_pipe_id;
    } else {
      adt_tbl_data = pipe_mgr_get_adt_instance(adt, pipe_id);
      if (adt_tbl_data == NULL) {
        LOG_ERROR(
            "%s:%d Adt tbl instance for pipe id %d, for tbl 0x%x, device id %d "
            "not found",
            __func__,
            __LINE__,
            pipe_id,
            adt_tbl_hdl,
            device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
    }
    adt_stage_info = pipe_mgr_adt_get_stage_info(adt_tbl_data, stage_id);
    if (adt_stage_info == NULL) {
      LOG_ERROR(
          "%s:%d Adt stage info for stage id %d, pipe id %d, tbl 0x%x, "
          "device id "
          "%d not found",
          __func__,
          __LINE__,
          stage_id,
          pipe_id,
          adt_tbl_hdl,
          device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  shadow_ptrs = adt_stage_info->shadow_ptr_arr;
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_rams_in_wide_word = ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;

  if (num_rams_in_wide_word == 1) {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH / num_entries_per_wide_word) / 8;
  } else {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH * num_rams_in_wide_word) / 8;
  }

  pipe_mgr_adt_tof_unbuild_virt_addr(
      virt_addr, adt_entry_width, &ram_line_num, &vpn, &entry_position);
  /* From VPN, figure out the wide word blk idx */
  for (i = 0; i < ram_alloc_info->num_wide_word_blks; i++) {
    if (ram_alloc_info->tbl_word_blk[i].vpn_id[0] == vpn) {
      wide_word_blk_idx = i;
      break;
    }
  }
  if (i == ram_alloc_info->num_wide_word_blks) {
    LOG_ERROR(
        "%s:%d Invalid VPN %d for action tbl 0x%x, device id %d, stage id "
        "%d, virt addr 0x%x",
        __func__,
        __LINE__,
        vpn,
        adt->adt_tbl_hdl,
        device_id,
        stage_id,
        virt_addr);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  /* Compute the global logical action idx */
  adt_entry_idx = (ram_line_num * num_entries_per_wide_word) +
                  (num_entries_per_wide_word_blk * wide_word_blk_idx) +
                  entry_position;
  /* What is to be returned is a global logical action idx */
  (*logical_action_idx) = adt_entry_idx + adt_stage_info->stage_offset;

  /* Optimization : First, check if this entry index in this stage is associated
   * with a temp adt ent hdl. If so, return that adt ent hdl.
   */
  if (!pipe_mgr_adt_mgr_ent_idx_assigned_to_temp_ent_hdl(
          adt_stage_info, adt_entry_idx, &action_spec->adt_ent_hdl)) {
    for (i = 0; i < num_rams_in_wide_word; i++) {
      mem_id = ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id[i];
      status = pipe_mgr_phy_mem_map_get_ref(device_id,
                                            gress,
                                            pipe_mem_type_unit_ram,
                                            pipe_id,
                                            stage_id,
                                            mem_id,
                                            ram_line_num,
                                            &shadow_ptrs[i],
                                            true);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting shadow mem ref for adt ram %d, line %d, "
            "pipe "
            "id %d, stage id %d, for adt tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            mem_id,
            ram_line_num,
            pipe_id,
            stage_id,
            adt_tbl_hdl,
            device_id,
            pipe_str_err(status));
        return status;
      }
    }

    status = pipe_mgr_entry_format_tof_adt_tbl_ent_decode_to_action_spec(
        adt->dev_info,
        adt->profile_id,
        stage_id,
        adt_tbl_hdl,
        act_fn_hdl,
        entry_position,
        act_data_spec,
        shadow_ptrs);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding adt entry at idx %d, pipe id %d, stage id "
          "%d, "
          "tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          (*logical_action_idx) - adt_stage_info->stage_offset,
          pipe_id,
          stage_id,
          adt_tbl_hdl,
          device_id,
          pipe_str_err(status));
      return status;
    }
    /* Temp adt entry handle has not yet been assigned, assign one and update
     * state
     */
    action_spec->adt_ent_hdl =
        pipe_mgr_adt_mgr_get_new_temp_adt_ent_hdl(adt_tbl_data);
    if (action_spec->adt_ent_hdl == PIPE_ADT_ENT_HDL_INVALID_HDL) {
      LOG_ERROR(
          "%s:%d Error in assigning temp adt ent hdl at idx %d, pipe id %d, "
          "stage id %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          adt_entry_idx,
          pipe_id,
          stage_id,
          adt_tbl_hdl,
          device_id,
          pipe_str_err(status));
      return status;
    }

    status = pipe_mgr_adt_mgr_assign_temp_ent_hdl_to_ent_idx(
        adt_stage_info, adt_entry_idx, action_spec->adt_ent_hdl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in assigning temp adt ent hdl %d to idx %d, stage id "
          "%d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          action_spec->adt_ent_hdl,
          adt_entry_idx,
          adt_stage_info->stage_id,
          adt->adt_tbl_hdl,
          adt->dev_id,
          pipe_str_err(status));
      /* Release the temp adt ent hdl obtained */
      pipe_mgr_adt_mgr_release_temp_adt_ent_hdl(adt_tbl_data,
                                                action_spec->adt_ent_hdl);
      return status;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_stage_ha_llp_info_init(
    pipe_mgr_adt_stage_info_t *adt_stage_info) {
  adt_stage_info->ha_llp_info =
      (pipe_mgr_adt_stage_ha_llp_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_adt_stage_ha_llp_info_t));
  if (adt_stage_info->ha_llp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_adt_stage_ha_llp_info_destroy(
    pipe_mgr_adt_stage_info_t *adt_stage_info) {
  if (!adt_stage_info->ha_llp_info) {
    return;
  }
  /* Destroy the entry handle to entry index map */
  pipe_mgr_adt_stage_ha_llp_info_t *ha_llp_info = adt_stage_info->ha_llp_info;
  bf_map_sts_t map_sts;
  unsigned long key = 0;
  pipe_mat_ent_hdl_t *entry_hdl;
  if (ha_llp_info->ent_idx_to_ent_hdl) {
    while ((map_sts = bf_map_get_first_rmv(
                &ha_llp_info->ent_idx_to_ent_hdl, &key, (void **)&entry_hdl)) ==
           BF_MAP_OK) {
      PIPE_MGR_FREE(entry_hdl);
    }
    bf_map_destroy(&ha_llp_info->ent_idx_to_ent_hdl);
  }
  PIPE_MGR_FREE(adt_stage_info->ha_llp_info);
  adt_stage_info->ha_llp_info = NULL;
  return;
}

pipe_status_t pipe_mgr_adt_pipe_ha_llp_info_init(
    pipe_mgr_adt_data_t *adt_tbl_data) {
  adt_tbl_data->ha_llp_info =
      (pipe_mgr_adt_pipe_ha_llp_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_adt_pipe_ha_llp_info_t));
  if (adt_tbl_data->ha_llp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  pipe_mgr_adt_pipe_ha_llp_info_t *ha_llp_info = adt_tbl_data->ha_llp_info;

  ha_llp_info->ent_hdl_allocator =
      bf_id_allocator_new(adt_tbl_data->num_entries, false);
  return PIPE_SUCCESS;
}

void pipe_mgr_adt_pipe_ha_llp_info_destroy(pipe_mgr_adt_data_t *adt_tbl_data) {
  if (!adt_tbl_data->ha_llp_info) {
    return;
  }
  pipe_mgr_adt_pipe_ha_llp_info_t *ha_llp_info = adt_tbl_data->ha_llp_info;

  if (ha_llp_info->ent_hdl_allocator) {
    bf_id_allocator_destroy(ha_llp_info->ent_hdl_allocator);
  }
  PIPE_MGR_FREE(adt_tbl_data->ha_llp_info);
  adt_tbl_data->ha_llp_info = NULL;

  return;
}

pipe_status_t pipe_mgr_adt_mgr_update_llp_state_for_ha(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_ha_cookie_t *adt_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_idx_t logical_action_idx,
    bool sharable) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt = adt_cookie->adt;
  pipe_mgr_adt_data_t *adt_tbl_data = adt_cookie->adt_tbl_data;
  pipe_mgr_adt_stage_info_t *adt_stage_info = adt_cookie->adt_stage_info;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_adt_ent_idx_t stage_entry_idx =
      logical_action_idx - adt_stage_info->stage_offset;
  bool exists = false;
  adt_data_resources_t *resources = NULL;

  (void)device_id;
  pipe_mgr_adt_move_list_t move_list_node = {0};
  if (action_spec->resource_count) {
    resources = (adt_data_resources_t *)PIPE_MGR_CALLOC(
        action_spec->resource_count, sizeof(adt_data_resources_t));
    if (resources == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      goto err_cleanup;
    }
    int i = 0;
    for (i = 0; i < action_spec->resource_count; i++) {
      resources[i].tbl_hdl = action_spec->resources[i].tbl_hdl;
      resources[i].tbl_idx = action_spec->resources[i].tbl_idx;
    }
  }
  int is_const = 0;
  move_list_node.data = make_adt_ent_data(&action_spec->act_data,
                                          act_fn_hdl,
                                          action_spec->resource_count,
                                          is_const,
                                          resources);
  if (move_list_node.data == NULL) {
    LOG_ERROR(
        "%s:%d Error in updating adt state for entry hdl %d, tbl 0x%x, device "
        "id %d",
        __func__,
        __LINE__,
        action_spec->adt_ent_hdl,
        adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  /* Updating state is equivalent to adding a action data entry to the action
   * profile table and instantiating it at a particular physical location.
   */
  status = pipe_mgr_adt_execute_entry_add(
      adt, action_spec->adt_ent_hdl, adt_tbl_data->pipe_id, &move_list_node);

  if (status != PIPE_SUCCESS && status != PIPE_ALREADY_EXISTS) {
    LOG_ERROR(
        "%s:%d Error in updating adt state for entry hdl %d, tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        action_spec->adt_ent_hdl,
        adt_tbl_hdl,
        adt->dev_id,
        pipe_str_err(status));
    goto err_cleanup;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, action_spec->adt_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in updating adt state for entry hdl %d, tbl 0x%x, device "
        "id %d",
        __func__,
        __LINE__,
        action_spec->adt_ent_hdl,
        adt_tbl_hdl,
        adt->dev_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  if (sharable) {
    status = pipe_mgr_adt_update_state_for_entry_activate(
        entry_info, adt_stage_info->stage_id, stage_entry_idx, &exists);
  } else {
    status = pipe_mgr_adt_update_state_for_entry_install(
        entry_info, adt_stage_info->stage_id, stage_entry_idx);
  }
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for adt entry for entry hdl %d, "
        "stage id %d, idx %d, tbl 0x%x, device id %d err %s",
        __func__,
        __LINE__,
        action_spec->adt_ent_hdl,
        adt_stage_info->stage_id,
        stage_entry_idx,
        adt_tbl_hdl,
        adt->dev_id,
        pipe_str_err(status));
    goto err_cleanup;
  }

  if (!exists) {
    adt_tbl_data->num_entries_programmed++;
  }

err_cleanup:
  if (move_list_node.data) {
    free_adt_ent_data(move_list_node.data);
  }
  if (resources) {
    PIPE_MGR_FREE(resources);
  }
  return status;
}

pipe_status_t pipe_mgr_adt_llp_restore_state(
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_mgr_adt_move_list_t **move_list) {
  /* The state would have been restored as part of the match table state
   * restore. Here we just go over the state and produce move-lists to populate
   * HLP state.
   */
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_adt_move_list_t *move_list_node = NULL;

  pipe_mgr_adt_t *adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Adt tbl info for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  unsigned long key = 0;
  uint32_t pipe_idx;
  pipe_mgr_adt_data_t *adt_tbl_data;
  bf_map_t *htbl;
  struct pipe_mgr_adt_move_list_t move_head;
  move_head.next = NULL;
  pipe_mgr_adt_move_list_t *move_list_tail = &move_head;

  for (pipe_idx = 0; pipe_idx < adt->num_tbls; pipe_idx++) {
    adt_tbl_data = &adt->adt_tbl_data[pipe_idx];
    htbl = &adt_tbl_data->entry_phy_info_htbl;

    map_sts = bf_map_get_first(htbl, &key, (void **)&entry_info);
    while (map_sts == BF_MAP_OK) {
      move_list_node = alloc_adt_move_list(move_list_tail, PIPE_ADT_UPDATE_ADD);
      if (move_list_node == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      pipe_action_data_spec_t *act_data_spec =
          unpack_adt_ent_data_ad(entry_info->entry_data);
      pipe_act_fn_hdl_t act_fn_hdl =
          unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
      move_list_node->data = make_adt_ent_data(
          act_data_spec,
          act_fn_hdl,
          unpack_adt_ent_data_num_resources(entry_info->entry_data),
          unpack_adt_ent_data_const(entry_info->entry_data),
          unpack_adt_ent_data_resources(entry_info->entry_data));
      move_list_node->entry_hdl = key;
      move_list_node->pipe_id = entry_info->pipe_id;
      move_list_node->sharable = entry_info->sharable_stage_location;
      move_list_tail = move_list_node;
      map_sts = bf_map_get_next(htbl, &key, (void **)&entry_info);
    }
  }

  if (move_list) {
    *move_list = move_head.next;
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_adt_cleanup_llp_ha_state(bf_dev_id_t device_id,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    LOG_ERROR("%s:%d Action data table 0x%x for device id %d not found",
              __func__,
              __LINE__,
              adt_tbl_hdl,
              device_id);
    return;
  }
  unsigned tbl_idx = 0;
  for (tbl_idx = 0; tbl_idx < adt->num_tbls; tbl_idx++) {
    pipe_mgr_adt_data_t *adt_tbl_data = &adt->adt_tbl_data[tbl_idx];
    unsigned stage_idx = 0;
    for (stage_idx = 0; stage_idx < adt_tbl_data->num_stages; stage_idx++) {
      pipe_mgr_adt_stage_info_t *adt_stage_info =
          &adt_tbl_data->adt_stage_info[stage_idx];
      pipe_mgr_adt_stage_ha_llp_info_destroy(adt_stage_info);
    }
    pipe_mgr_adt_pipe_ha_llp_info_destroy(adt_tbl_data);
  }
}
