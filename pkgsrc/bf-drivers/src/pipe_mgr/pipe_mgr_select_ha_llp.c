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
 * @file pipe_mgr_select_ha_llp.c
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

static bool pipe_mgr_sel_grp_idx_assigned(sel_tbl_t *sel_tbl,
                                          pipe_idx_t grp_idx,
                                          pipe_sel_grp_hdl_t *grp_hdl) {
  pipe_mgr_sel_pipe_ha_llp_info_t *ha_llp_info = sel_tbl->llp.ha_llp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = grp_idx;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info;
  if (!ha_llp_info->idx_to_grp_info) {
    return false;
  }

  map_sts =
      bf_map_get(&ha_llp_info->idx_to_grp_info, key, (void **)&grp_ha_info);
  if (map_sts == BF_MAP_OK) {
    *grp_hdl = grp_ha_info->grp_hdl;
    return true;
  } else if (map_sts != BF_MAP_NO_KEY) {
    LOG_ERROR(
        "%s:%d Error in getting grp hdl from group idx %d, err "
        "0x%x",
        __func__,
        __LINE__,
        grp_idx,
        map_sts);
    PIPE_MGR_DBGCHK(0);
  }
  return false;
}

static pipe_sel_grp_hdl_t pipe_mgr_sel_get_new_temp_sel_grp_hdl(
    sel_tbl_info_t *sel_tbl_info, bf_dev_pipe_t pipe_id) {
  pipe_mgr_sel_ha_llp_info_t *ha_llp_info = sel_tbl_info->ha_llp_info;
  Word_t index = PIPE_SEL_GRP_HDL_BASE;
  int Rc_int;

  if (pipe_id != BF_DEV_PIPE_ALL) {
    index = PIPE_SET_HDL_PIPE(index, pipe_id);
  }
  JLFE(Rc_int, ha_llp_info->ha_sel_grp_array, index);
  if (!Rc_int) {
    LOG_ERROR("%s:%d %s(0x%x) Error allocating grp hdl",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl);
    return PIPE_INVALID_SEL_GRP_HDL;
  }

  return (pipe_sel_grp_hdl_t)index;
}

static pipe_status_t pipe_mgr_sel_llp_assign_grp_idx(
    sel_tbl_info_t *sel_tbl_info,
    sel_tbl_t *sel_tbl,
    pipe_idx_t grp_idx,
    pipe_sel_grp_hdl_t grp_hdl,
    uint32_t sel_len) {
  pipe_mgr_sel_ha_llp_info_t *ha_llp_info = sel_tbl_info->ha_llp_info;
  pipe_mgr_sel_pipe_ha_llp_info_t *pipe_ha_llp_info = sel_tbl->llp.ha_llp_info;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key;
  uint32_t stage_idx;
  PWord_t Pvalue;
  int Rc_int;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_sel_grp_ha_info_t));
  if (!grp_ha_info) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  grp_ha_info->grp_hdl = grp_hdl;
  grp_ha_info->pipe_id = sel_tbl->pipe_id;
  grp_ha_info->sel_base_idx = grp_idx;
  grp_ha_info->sel_len = sel_len;
  grp_ha_info->adt_base_idx =
      PIPE_MGR_MALLOC(sel_tbl->num_stages * sizeof(pipe_adt_ent_idx_t));

  for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
    grp_ha_info->adt_base_idx[stage_idx] = PIPE_MGR_LOGICAL_ACT_IDX_INVALID;
  }

  key = grp_hdl;
  JLI(Pvalue, ha_llp_info->ha_sel_grp_array, key);
  if (Pvalue == PJERR) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (*Pvalue) {
    LOG_ERROR("%s:%d %s(0x%x %d) Sel grp 0x%x already exists",
              __func__,
              __LINE__,
              sel_tbl_info->name,
              sel_tbl_info->tbl_hdl,
              sel_tbl_info->dev_id,
              grp_hdl);
    if (grp_ha_info) {
      PIPE_MGR_FREE(grp_ha_info);
    }
    return PIPE_ALREADY_EXISTS;
  }
  *Pvalue = (Word_t)grp_ha_info;

  key = grp_idx;
  map_sts =
      bf_map_add(&pipe_ha_llp_info->idx_to_grp_info, key, (void *)grp_ha_info);
  if (map_sts == BF_MAP_KEY_EXISTS) {
    LOG_ERROR(
        "%s:%d Group idx to info mapping already exists for idx %d "
        "pipe %d tbl 0x%x",
        __func__,
        __LINE__,
        grp_idx,
        sel_tbl->pipe_id,
        sel_tbl_info->tbl_hdl);
    if (grp_ha_info) {
      PIPE_MGR_FREE(grp_ha_info);
    }
    key = grp_hdl;
    JLD(Rc_int, ha_llp_info->ha_sel_grp_array, key);
    if (Rc_int == JERR) return PIPE_NO_SYS_RESOURCES;
    return PIPE_ALREADY_EXISTS;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_get_sel_info(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    sel_tbl_info_t **sel_tbl_info_p,
    sel_tbl_t **sel_tbl_p,
    sel_tbl_stage_info_t **sel_stage_info_p) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;
  uint32_t i;

  if (sel_tbl_info_p) {
    sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
    if (sel_tbl_info == NULL) {
      LOG_ERROR("%s:%d Selector tbl 0x%x, device id %d not found",
                __func__,
                __LINE__,
                tbl_hdl,
                device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    *sel_tbl_info_p = sel_tbl_info;
  }

  if (sel_tbl_p || sel_stage_info_p) {
    if (sel_tbl_info) {
      if (sel_tbl_info->is_symmetric) {
        if (pipe_id != sel_tbl_info->lowest_pipe_id &&
            pipe_id != BF_DEV_PIPE_ALL) {
          LOG_ERROR(
              "%s:%d Invalid pipe id %d passed for a symmetric sel tbl 0x%x, "
              "device id %d",
              __func__,
              __LINE__,
              pipe_id,
              tbl_hdl,
              device_id);
          return PIPE_INVALID_ARG;
        }
        sel_tbl = &sel_tbl_info->sel_tbl[0];
      } else {
        for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
          if (pipe_id == sel_tbl_info->sel_tbl[i].pipe_id) {
            sel_tbl = &sel_tbl_info->sel_tbl[i];
            break;
          }
        }
      }
    }
    if (sel_tbl == NULL) {
      LOG_ERROR(
          "%s:%d Sel tbl instance for pipe id %d, for tbl 0x%x, device id %d "
          "not found",
          __func__,
          __LINE__,
          pipe_id,
          tbl_hdl,
          device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (sel_tbl_p) {
      *sel_tbl_p = sel_tbl;
    }

    if (sel_stage_info_p) {
      for (i = 0; i < sel_tbl->num_stages; i++) {
        if (sel_tbl->sel_tbl_stage_info[i].stage_id == stage_id) {
          sel_stage_info = &sel_tbl->sel_tbl_stage_info[i];
          break;
        }
      }
      if (sel_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Sel stage info for stage id %d, pipe id %d, tbl 0x%x, "
            "device id "
            "%d not found",
            __func__,
            __LINE__,
            stage_id,
            pipe_id,
            tbl_hdl,
            device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      *sel_stage_info_p = sel_stage_info;
    }
  }

  return PIPE_SUCCESS;
}

/* This API is intended to be used by match tbl managers during HA state restore
 * at LLP. This enables fast access to selector structures while in a loop of
 * restoring match entries and matching it with selector groups.
 */
pipe_status_t pipe_mgr_sel_ha_get_cookie(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_sel_ha_cookie_t *sel_ha_cookie) {
  pipe_status_t sts;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;

  sts = pipe_mgr_sel_get_sel_info(device_id,
                                  tbl_hdl,
                                  pipe_id,
                                  stage_id,
                                  &sel_tbl_info,
                                  &sel_tbl,
                                  &sel_stage_info);

  if (sts == PIPE_SUCCESS) {
    sel_ha_cookie->sel_tbl_info = sel_tbl_info;
    sel_ha_cookie->sel_tbl = sel_tbl;
    sel_ha_cookie->sel_stage_info = sel_stage_info;
  }
  return sts;
}

pipe_status_t pipe_mgr_sel_get_temp_sel_grp_hdl(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    rmt_virt_addr_t virt_addr,
    uint32_t sel_len,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_mgr_sel_ha_cookie_t *sel_ha_cookie,
    pipe_sel_grp_hdl_t *grp_hdl,
    pipe_idx_t *logical_sel_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_t *sel_tbl = NULL;

  if (sel_ha_cookie) {
    sel_tbl_info = sel_ha_cookie->sel_tbl_info;
    sel_tbl = sel_ha_cookie->sel_tbl;
  } else {
    status = pipe_mgr_sel_get_sel_info(
        device_id, tbl_hdl, pipe_id, stage_id, &sel_tbl_info, &sel_tbl, NULL);
    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

  pipe_mgr_sel_vaddr_to_logical_idx(
      device_id, tbl_hdl, stage_id, virt_addr, logical_sel_idx);
  if (!pipe_mgr_sel_grp_idx_assigned(sel_tbl, *logical_sel_idx, grp_hdl)) {
    *grp_hdl =
        pipe_mgr_sel_get_new_temp_sel_grp_hdl(sel_tbl_info, sel_tbl->pipe_id);
    status = pipe_mgr_sel_llp_assign_grp_idx(
        sel_tbl_info, sel_tbl, *logical_sel_idx, *grp_hdl, sel_len);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in assigning temp sel grp hdl %d to idx %d, "
          "pipe %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          *grp_hdl,
          *logical_sel_idx,
          sel_tbl->pipe_id,
          tbl_hdl,
          device_id,
          pipe_str_err(status));
      return status;
    }
    sel_tbl->llp.num_grps++;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_ha_llp_init(sel_tbl_info_t *sel_tbl_info) {
  sel_tbl_info->ha_llp_info = (pipe_mgr_sel_ha_llp_info_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_sel_ha_llp_info_t));
  if (sel_tbl_info->ha_llp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_pipe_ha_llp_init(sel_tbl_t *sel_tbl) {
  sel_tbl->llp.ha_llp_info = (pipe_mgr_sel_pipe_ha_llp_info_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_sel_pipe_ha_llp_info_t));
  if (sel_tbl->llp.ha_llp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_update_llp_state_for_ha(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_ha_cookie_t *sel_cookie,
    pipe_idx_t logical_sel_idx,
    uint32_t sel_len,
    pipe_mgr_adt_ha_cookie_t *adt_ha_cookie,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    rmt_virt_addr_t adt_virt_addr,
    pipe_idx_t *logical_adt_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  sel_tbl_info_t *sel_tbl_info = sel_cookie->sel_tbl_info;
  sel_tbl_t *sel_tbl = sel_cookie->sel_tbl;
  sel_tbl_stage_info_t *sel_stage_info = sel_cookie->sel_stage_info;
  sel_tbl_stage_hw_info_t *stage_hw = &sel_stage_info->pv_hw;
  pipe_mgr_sel_pipe_ha_llp_info_t *ha_llp_info = sel_tbl->llp.ha_llp_info;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info;
  sel_llp_word_data_t *word_data;
  pipe_adt_ent_idx_t adt_base_idx = 0;
  pipe_full_virt_addr_t full_virt_addr;
  uint64_t ind_addr = 0;
  uint64_t data[2] = {0, 0};
  uint32_t i, j;
  uint32_t data_sz_bytes;
  uint32_t word_idx = 0, mbr_idx = 0, adt_idx = 0;
  uint32_t mbr_bit;
  uint32_t pv_subword = 0, pv_line = 0, pv_block = 0;
  uint32_t index = 0, no_subword_bits = 0;
  uint32_t no_huffman_bits = 0, huffman_bits = 0, virt_addr = 0;
  bf_dev_pipe_t pipe_id = 0, phy_pipe = 0;

  (void)tbl_hdl;

  status = pipe_mgr_adt_mgr_decode_virt_addr(
      adt_ha_cookie, adt_virt_addr, &adt_base_idx);
  PIPE_MGR_ASSERT(status == PIPE_SUCCESS);
  *logical_adt_idx = adt_base_idx;

  word_data = &sel_stage_info->llp.llp_word[logical_sel_idx];
  if (word_data->highest_mbr_idx != -1) {
    /* We've processed this group already */
    return PIPE_SUCCESS;
  }

  if (sel_tbl->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = sel_tbl_info->lowest_pipe_id;
  } else {
    pipe_id = sel_tbl->pipe_id;
  }
  status = pipe_mgr_map_pipe_id_log_to_phy(
      sel_tbl_info->dev_info, pipe_id, &phy_pipe);
  if (PIPE_SUCCESS != status) {
    LOG_ERROR("%s:%d Error in converting logical pipe %d to physical, err %s",
              __func__,
              __LINE__,
              pipe_id,
              pipe_str_err(status));
    return status;
  }
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

  bf_map_sts_t sts = bf_map_get(
      &ha_llp_info->idx_to_grp_info, logical_sel_idx, (void **)&grp_ha_info);
  if (BF_MAP_OK != sts) {
    LOG_ERROR(
        "%s:%d Error getting map info, logical_sel_idx = 0x%x, err = 0x%x",
        __func__,
        __LINE__,
        logical_sel_idx,
        sts);
  }
  // Debugging assert
  PIPE_MGR_ASSERT(grp_ha_info->adt_base_idx[sel_stage_info->stage_idx] ==
                      PIPE_MGR_LOGICAL_ACT_IDX_INVALID ||
                  grp_ha_info->adt_base_idx[sel_stage_info->stage_idx] ==
                      adt_base_idx);
  grp_ha_info->adt_base_idx[sel_stage_info->stage_idx] = adt_base_idx;

  for (word_idx = 0; word_idx < sel_len; word_idx++) {
    word_data = &sel_stage_info->llp.llp_word[logical_sel_idx + word_idx];
    word_data->adt_base_idx = adt_base_idx + word_idx * SEL_GRP_WORD_WIDTH;

    pipe_mgr_sel_tbl_get_phy_addr(stage_hw->pack_format,
                                  logical_sel_idx + word_idx,
                                  &pv_subword,
                                  &pv_line,
                                  &pv_block,
                                  TOF_SRAM_UNIT_DEPTH);

    index = (stage_hw->tbl_blk[pv_block].vpn_id[0] << log2_uint32_ceil(
                 pipe_mgr_get_sram_unit_depth(sel_tbl_info->dev_id))) |
            pv_line;
    no_subword_bits = 0;
    no_huffman_bits = TOF_SEL_SUBWORD_VPN_BITS - no_subword_bits;
    huffman_bits = (1 << (no_huffman_bits - 1)) - 1;
    virt_addr = (index << no_huffman_bits) | huffman_bits;
    construct_full_virt_addr(sel_tbl_info->dev_info,
                             &full_virt_addr,
                             sel_stage_info->pv_hw.tbl_id,
                             pipe_virt_mem_type_sel_stful,
                             virt_addr,
                             phy_pipe,
                             sel_stage_info->stage_id);
    ind_addr = full_virt_addr.addr;
    lld_subdev_ind_read(device_id, subdev, ind_addr, &data[1], &data[0]);

    PIPE_MGR_MEMCPY(word_data->data, data, sizeof(uint64_t) * 2);
    data_sz_bytes = ((sel_stage_info->ram_word_width - 1) >> 3) + 1;
    mbr_idx = 0;
    for (i = 0; i < data_sz_bytes - 1; i++) {
      for (j = 0; j < 8; j++) {
        if (SEL_TBL_IS_FAIR(sel_tbl->sel_tbl_info)) {
          if (word_data->data[data_sz_bytes - 1] <= word_data->no_bits_set) {
            break;
          }
        } else if (SEL_TBL_IS_RESILIENT(sel_tbl->sel_tbl_info)) {
          if (word_data->data[data_sz_bytes - 1] <=
              word_data->highest_mbr_idx + 1) {
            break;
          }
        }
        mbr_bit = word_data->data[i] & (1 << j);
        if (mbr_bit > 0) {
          adt_idx = adt_base_idx + word_idx * SEL_GRP_WORD_WIDTH + mbr_idx;

          status = pipe_adt_tof_generate_vaddr(
              adt_ha_cookie->adt_stage_info, adt_idx, &adt_virt_addr);
          if (PIPE_SUCCESS != status) return status;

          status =
              pipe_mgr_adt_mgr_get_temp_adt_ent_hdl(device_id,
                                                    sel_tbl_info->direction,
                                                    sel_tbl_info->adt_tbl_hdl,
                                                    adt_virt_addr,
                                                    sel_tbl->pipe_id,
                                                    sel_stage_info->stage_id,
                                                    adt_ha_cookie,
                                                    action_spec,
                                                    act_fn_hdl,
                                                    &adt_idx);
          if (PIPE_SUCCESS != status) return status;

          status = pipe_mgr_adt_mgr_update_llp_state_for_ha(
              device_id,
              sel_tbl_info->adt_tbl_hdl,
              adt_ha_cookie,
              action_spec,
              act_fn_hdl,
              adt_idx,
              false);
          if (PIPE_SUCCESS != status) return status;

          word_data->mbrs[mbr_idx] = action_spec->adt_ent_hdl;
          word_data->highest_mbr_idx = mbr_idx;
          word_data->no_bits_set++;
        }
        mbr_idx++;
      }
      if (j < 8) {
        break;
      }
    }

    status = pipe_mgr_phy_mem_map_write(
        device_id,
        sel_tbl_info->direction,
        pipe_id,
        sel_stage_info->stage_id,
        pipe_mem_type_unit_ram,
        sel_stage_info->pv_hw.tbl_blk[pv_block].mem_id[0],
        pv_line,
        word_data->data,
        NULL);
  }

  return status;
}

pipe_status_t pipe_mgr_sel_llp_restore_state(
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_mgr_sel_move_list_t **move_list) {
  /* The state would have been restored as part of the match table state
   * restore. Here we just go over the state and produce move-lists to populate
   * * HLP state.  */
  bf_map_sts_t map_sts = BF_MAP_OK;
  sel_tbl_info_t *sel_tbl_info;
  sel_tbl_t *sel_tbl;
  sel_tbl_stage_info_t *sel_stage_info;
  pipe_mgr_sel_pipe_ha_llp_info_t *pipe_ha_llp_info;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info;
  sel_llp_word_data_t *word_data = NULL;
  pipe_mgr_sel_move_list_t *move_list_node = NULL, *grp_move_list_node = NULL;
  uint32_t pipe_idx, stage_idx, adt_stage_idx;
  uint32_t i, j;
  uint32_t data_sz_bytes;
  uint32_t word_idx = 0, mbr_idx = 0;
  uint32_t adt_base_idx = 0;
  uint32_t mbr_bit;
  uint32_t num_indexes = 0;
  uint32_t loc_idx = 0;
  uint32_t max_mbrs = 0;

  if (!move_list) {
    return PIPE_SUCCESS;
  }

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(device_id, tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d Sel tbl info for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  unsigned long key = 0;
  struct pipe_mgr_sel_move_list_t move_head;
  move_head.next = NULL;
  pipe_mgr_sel_move_list_t *move_list_tail = &move_head;

  for (pipe_idx = 0; pipe_idx < sel_tbl_info->no_sel_tbls; pipe_idx++) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    pipe_ha_llp_info = sel_tbl->llp.ha_llp_info;
    map_sts = bf_map_get_first(
        &pipe_ha_llp_info->idx_to_grp_info, &key, (void **)&grp_ha_info);
    while (map_sts == BF_MAP_OK) {
      move_list_node = alloc_sel_move_list(
          move_list_tail, PIPE_SEL_UPDATE_GROUP_CREATE, sel_tbl->pipe_id);
      if (!move_list_node) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      num_indexes = sel_tbl->num_stages * grp_ha_info->sel_len;
      move_list_node->locations =
          PIPE_MGR_CALLOC(num_indexes, sizeof(struct pipe_multi_index));
      if (!move_list_node->locations) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      move_list_node->sel_grp_hdl = grp_ha_info->grp_hdl;
      move_list_node->sel_grp_size = grp_ha_info->sel_len;
      move_list_node->logical_sel_index = grp_ha_info->sel_base_idx;
      move_list_node->locations_length = num_indexes;
      move_list_tail = move_list_node;
      grp_move_list_node = move_list_node;
      int highest_mbr_cnt = 1;
      for (stage_idx = 0; stage_idx < sel_tbl->num_stages; stage_idx++) {
        sel_stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];
        data_sz_bytes = ((sel_stage_info->ram_word_width - 1) >> 3) + 1;
        adt_base_idx = grp_ha_info->adt_base_idx[stage_idx];
        for (word_idx = 0; word_idx < grp_ha_info->sel_len; word_idx++) {
          loc_idx = stage_idx * grp_ha_info->sel_len + word_idx;
          if (adt_base_idx == PIPE_MGR_LOGICAL_ACT_IDX_INVALID) {
            grp_move_list_node->locations[loc_idx].logical_index_base =
                PIPE_MGR_LOGICAL_ACT_IDX_INVALID;
          } else {
            word_data = &sel_stage_info->llp.llp_word[key + word_idx];
            if (word_data->highest_mbr_idx >= highest_mbr_cnt) {
              highest_mbr_cnt = word_data->highest_mbr_idx + 1;
            }
            mbr_idx = 0;
            for (i = 0; i < data_sz_bytes - 1; i++) {
              for (j = 0; j < 8; j++) {
                mbr_bit = word_data->data[i] & (1 << j);
                if (mbr_bit > 0) {
                  move_list_node = alloc_sel_move_list(
                      move_list_tail, PIPE_SEL_UPDATE_ADD, sel_tbl->pipe_id);
                  if (!move_list_node) {
                    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
                    return PIPE_NO_SYS_RESOURCES;
                  }
                  move_list_node->sel_grp_hdl = grp_ha_info->grp_hdl;
                  move_list_node->adt_mbr_hdl = word_data->mbrs[mbr_idx];
                  move_list_node->logical_sel_index =
                      grp_ha_info->sel_base_idx + word_idx;
                  move_list_node->logical_sel_subindex = mbr_idx;

                  pipe_idx_t adt_index_array[sel_tbl->num_stages];
                  for (adt_stage_idx = 0; adt_stage_idx < sel_tbl->num_stages;
                       adt_stage_idx++) {
                    if (adt_stage_idx == stage_idx) {
                      adt_index_array[adt_stage_idx] = adt_base_idx;
                    } else {
                      adt_index_array[adt_stage_idx] =
                          PIPE_MGR_LOGICAL_ACT_IDX_INVALID;
                    }
                  }
                  move_list_node->data =
                      make_sel_ent_data(sel_tbl->num_stages, adt_index_array);
                  move_list_tail = move_list_node;
                }
                mbr_idx++;
              }
            }
            grp_move_list_node->locations[loc_idx].logical_index_base =
                adt_base_idx + word_idx * SEL_GRP_WORD_WIDTH;
            grp_move_list_node->locations[loc_idx].logical_index_count =
                SEL_VEC_WIDTH;
          }
        }
      }
      if (grp_ha_info->sel_len > 1 && highest_mbr_cnt < 65) {
        highest_mbr_cnt = 65;
      }
      max_mbrs = highest_mbr_cnt * grp_ha_info->sel_len;
      if (max_mbrs < (grp_ha_info->sel_len - 1) * SEL_VEC_WIDTH + 1) {
        max_mbrs = (grp_ha_info->sel_len - 1) * SEL_VEC_WIDTH + 1;
      }
      grp_move_list_node->max_mbrs = max_mbrs;

      map_sts = bf_map_get_next(
          &pipe_ha_llp_info->idx_to_grp_info, &key, (void **)&grp_ha_info);
    }
  }

  *move_list = move_head.next;
  return PIPE_SUCCESS;
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

void pipe_mgr_selector_cleanup_pipe_llp_ha_state(sel_tbl_t *sel_tbl) {
  if (!sel_tbl->llp.ha_llp_info) {
    return;
  }
  pipe_mgr_sel_pipe_ha_llp_info_t *ha_llp_info = sel_tbl->llp.ha_llp_info;
  unsigned long key = 0;
  pipe_mgr_sel_grp_ha_info_t *grp_ha_info;
  bf_map_sts_t map_sts;
  while ((map_sts = bf_map_get_first_rmv(
              &ha_llp_info->idx_to_grp_info, &key, (void **)&grp_ha_info)) ==
         BF_MAP_OK) {
    destroy_grp_ha_info(grp_ha_info);
  }
  bf_map_destroy(&ha_llp_info->idx_to_grp_info);
  PIPE_MGR_FREE(sel_tbl->llp.ha_llp_info);
  sel_tbl->llp.ha_llp_info = NULL;
  return;
}

void pipe_mgr_selector_tbl_cleanup_llp_ha_state(sel_tbl_info_t *sel_tbl_info) {
  pipe_mgr_sel_ha_llp_info_t *ha_llp_info = sel_tbl_info->ha_llp_info;
  if (!ha_llp_info) {
    return;
  }
  if (ha_llp_info->ha_sel_grp_array) {
    Word_t Rc_word = 0;
    /* The ha_sel_grp_array acts as both a grp hdl allocator as well as hash
     * table. The data stored in the hash table is the same as the one stored at
     * the pipe level ha state which maps idx to the group info. Hence only free
     * the data stored once. That is done during pipe-level cleanup.
     */
    JLFA(Rc_word, ha_llp_info->ha_sel_grp_array);
    (void)Rc_word;
  }
  PIPE_MGR_FREE(sel_tbl_info->ha_llp_info);
  sel_tbl_info->ha_llp_info = NULL;
  return;
}

void pipe_mgr_selector_cleanup_llp_ha_state(bf_dev_id_t device_id,
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
    pipe_mgr_selector_cleanup_pipe_llp_ha_state(sel_tbl);
  }

  pipe_mgr_selector_tbl_cleanup_llp_ha_state(sel_tbl_info);
  return;
}
