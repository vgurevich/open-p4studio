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
 r @file pipe_mgr_hw_dump.c
 * @date
 *
 * Implementation of pipeline management peek/poke interface
 */

/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_hw_dump.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_table_packing.h"

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/* Dump tbl words into a buffer */
void pipe_mgr_dump_tbl_word(uint8_t *word_ptr0,
                            uint8_t *word_ptr1,
                            char *str,
                            int *c_len,
                            int max_len,
                            int index,
                            int bit_width) {
  char buf[1000];
  int j = 0;
  char *ptr = buf;
  char *end = buf + 1000;
  uint8_t *data_ptr;
  bool one_time = false;

  PIPE_MGR_MEMSET(buf, 0, sizeof(buf));

  data_ptr = (uint8_t *)word_ptr0;
  for (j = bit_width / 8 - 1; (data_ptr) && (j >= 0); j--) {
    /* Print every byte */
    ptr += snprintf(ptr, end > ptr ? (end - ptr) : 0, "%02x ", data_ptr[j]);
  }

  data_ptr = (uint8_t *)word_ptr1;
  for (j = bit_width / 8 - 1; (data_ptr) && (j >= 0); j--) {
    if (!one_time) {
      one_time = true;
      ptr += snprintf(ptr, end > ptr ? (end - ptr) : 0, "\n%-53s", " ");
    }

    /* Print every byte */
    ptr += snprintf(ptr, end > ptr ? (end - ptr) : 0, "%02x ", data_ptr[j]);
  }

  if (str) {
    int start = index * bit_width;
    int end_pos = start + bit_width - 1;
    *c_len += snprintf(str + (*c_len),
                       (*c_len < max_len) ? (max_len - (*c_len) - 1) : 0,
                       "Value(%-3d..%-3d): %s\n",
                       end_pos,
                       start,
                       buf);
  }
}

/* Print the default match entry details */
static pipe_status_t pipe_mgr_dump_def_match_entry(
    bf_dev_id_t dev_id,
    uint8_t direction,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_pipe_t log_pipe,
    rmt_dev_profile_info_t *profile,
    char *str,
    int *c_str_len,
    int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len = *c_str_len;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_VAL;
  int reg_cnt = dev_info->dev_cfg.num_dflt_reg;
  pipe_register_spec_t *reg_spec_list;
  reg_spec_list = PIPE_MGR_MALLOC(reg_cnt * sizeof *reg_spec_list);
  if (!reg_spec_list) return PIPE_NO_SYS_RESOURCES;

  /* Read the registers */
  for (bf_dev_pipe_t pipe_id = 0;
       pipe_id < pipe_mgr_get_num_active_pipes(dev_id);
       pipe_id++) {
    /* skip unwanted pipes */
    if ((log_pipe != PIPE_INVALID_VAL) && (log_pipe != pipe_id)) {
      continue;
    }

    /* Get the register addresses. */
    rc = pipe_mgr_entry_format_tbl_default_entry_prepare(dev_info,
                                                         direction,
                                                         profile->profile_id,
                                                         tbl_hdl,
                                                         pipe_id,
                                                         reg_spec_list);
    if (PIPE_SUCCESS != rc) goto the_end;

    for (int reg = 0; reg < reg_cnt; reg++)
      lld_subdev_read_register(dev_id,
                               0,
                               reg_spec_list[reg].reg_addr,
                               &(reg_spec_list[reg].reg_data));
    pipe_mgr_entry_format_tbl_default_entry_print(
        dev_info, reg_spec_list, pipe_id, str, &c_len, max_len);
  }

  *c_str_len = c_len;

the_end:
  PIPE_MGR_FREE(reg_spec_list);
  return rc;
}

/* Print exact-match tbl info from HW */
pipe_status_t pipe_mgr_print_hw_phase0_tbl_info(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                pipe_mat_ent_hdl_t ent_hdl,
                                                char *str,
                                                int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len = 0;
  int reg = 0;
  pipe_mgr_phase0_tbl_t *phase0_tbl = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_VAL;
  int p0_width = dev_info->dev_cfg.p0_width;
  pipe_register_spec_t reg_spec_list[p0_width];
  pipe_memory_spec_t mem_spec_list;
  uint32_t port = pipe_mgr_phase0_hdl_to_port(ent_hdl);

  PIPE_MGR_MEMSET(reg_spec_list, 0, sizeof(reg_spec_list));
  PIPE_MGR_MEMSET(&mem_spec_list, 0, sizeof(mem_spec_list));

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Dumping Phase0 entry with table hdl 0x%x , entry hdl: "
                    "0x%x , device %d \n",
                    tbl_hdl,
                    ent_hdl,
                    dev_id);

  phase0_tbl = pipe_mgr_phase0_tbl_get(dev_id, tbl_hdl);
  if (phase0_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find phase0 tbl info with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_subdev_id_t subdev;
  rc = pipe_mgr_entry_format_phase0_tbl_addr_get(
      dev_info, port, false, &subdev, reg_spec_list, &mem_spec_list);
  if (rc != PIPE_SUCCESS) return rc;

  /* Read the registers */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2: {
      for (reg = 0; reg < p0_width; reg++) {
        lld_subdev_read_register(dev_id,
                                 subdev,
                                 reg_spec_list[reg].reg_addr,
                                 &(reg_spec_list[reg].reg_data));
        LOG_TRACE(
            "%s : Read phase0 tbl handle 0x%x, register addr"
            " 0x%x value 0x%x",
            __func__,
            tbl_hdl,
            reg_spec_list[reg].reg_addr,
            reg_spec_list[reg].reg_data);
      }
      break;
    }
    case BF_DEV_FAMILY_TOFINO3: {
      uint64_t data_hi = 0, data_lo = 0;
      lld_subdev_ind_read(
          dev_id, subdev, mem_spec_list.mem_addr0, &data_hi, &data_lo);
      reg_spec_list[0].reg_data = data_lo & 0xffffffff;
      reg_spec_list[1].reg_data = (data_lo >> 32) & 0xffffffff;
      reg_spec_list[2].reg_data = data_hi & 0xffffffff;
      reg_spec_list[3].reg_data = (data_hi >> 32) & 0xffffffff;
      break;
    }

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
      break;
  }

  /* Print the values */
  rc = pipe_mgr_entry_format_tof_phase0_tbl_print(dev_info,
                                                  phase0_tbl->profile_id,
                                                  tbl_hdl,
                                                  reg_spec_list,
                                                  str,
                                                  &c_len,
                                                  max_len);

  return rc;
}

/* Print exact-match tbl info from HW */
pipe_status_t pipe_mgr_print_hw_exm_tbl_info(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             pipe_mat_ent_hdl_t ent_hdl,
                                             bf_dev_pipe_t log_pipe,
                                             char *str,
                                             int max_len) {
  int c_len = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  uint8_t entry_position = 0;
  uint32_t mem_addr = 0;
  uint64_t addr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  bf_dev_pipe_t pipe_id = 0;
  int ram_unit_idx = 0, j = 0;
  char pipe_str[100];
  pipe_mgr_exm_phy_entry_info_t *entry_info;
  pipe_mgr_exm_stage_info_t *exm_stage_info;
  rmt_dev_profile_info_t *profile = NULL;

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Dumping Exact-match entry with table hdl 0x%x , entry "
                    "hdl: 0x%x , device %d \n",
                    tbl_hdl,
                    ent_hdl,
                    dev_id);

  /* Get table info and location details for this entry */
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find exm tbl info with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  profile = pipe_mgr_get_profile(
      exm_tbl->dev_info, exm_tbl->profile_id, __func__, __LINE__);

  /* Get the physical entry info from the entry's table instance */
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, ent_hdl, __func__, __LINE__);
  if (exm_tbl_data == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find exm table instance for table hdl 0x%x "
                      "entry hdl %d \n",
                      tbl_hdl,
                      ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_map_sts_t map_sts = bf_map_get(
      &exm_tbl_data->entry_phy_info_htbl, ent_hdl, (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find exm entry info with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Is this default entry */
  if (exm_tbl_data->default_entry_hdl == ent_hdl) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Default entry dump \n");

    rc = pipe_mgr_dump_def_match_entry(dev_id,
                                       exm_tbl->direction,
                                       tbl_hdl,
                                       log_pipe,
                                       profile,
                                       str,
                                       &c_len,
                                       max_len);
    return rc;
  }
  /* Print pipe */
  if (log_pipe != PIPE_INVALID_VAL) {
    if ((entry_info->pipe_id != BF_DEV_PIPE_ALL) &&
        (log_pipe != entry_info->pipe_id)) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Invalid pipe %d specified, entry is on pipe %d \n",
                        log_pipe,
                        entry_info->pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    pipe_id = log_pipe;
    sprintf(pipe_str, "%d", pipe_id);
  } else if (entry_info->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = 0;
    sprintf(pipe_str, "All");
  } else {
    pipe_id = entry_info->pipe_id;
    sprintf(pipe_str, "%d", pipe_id);
  }

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);

  if (exm_stage_info == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find exm tbl 0x%x in stage %d\n",
                      tbl_hdl,
                      entry_info->stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  entry_position = entry_info->entry_idx %
                   exm_stage_info->pack_format->num_entries_per_wide_word;

  mem_addr = (entry_info->entry_idx /
              exm_stage_info->pack_format->num_entries_per_wide_word) %
             TOF_SRAM_UNIT_DEPTH;

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Pipe-id: %s, Stage-id: %d \n",
                    pipe_str,
                    entry_info->stage_id);
  if (exm_tbl->num_adt_refs != 0) {
    pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
    if (exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      dev_stage_t stage_id;
      uint32_t idx;
      bf_dev_pipe_t entry_pipe;
      rc = pipe_mgr_exm_get_dir_ent_idx(dev_id,
                                        exm_tbl->mat_tbl_hdl,
                                        ent_hdl,
                                        &entry_pipe,
                                        &stage_id,
                                        NULL,
                                        &idx);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Action: [Table handle 0x%x, idx %d, vaddr 0x%x ]\n",
                        adt_tbl_hdl,
                        idx,
                        entry_info->indirect_ptrs.adt_ptr);
    } else {
      c_len += snprintf(
          str + c_len,
          (c_len < max_len) ? (max_len - c_len - 1) : 0,
          "Action: [Table handle 0x%x , Entry handle: %d , vaddr 0x%x ]\n",
          adt_tbl_hdl,
          entry_info->adt_ent_hdl,
          entry_info->indirect_ptrs.adt_ptr);
    }
  }
  PIPE_MGR_MEMSET(&addr, 0, sizeof(addr));

  bf_dev_pipe_t phy_pipe_id = pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_VAL;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe_id);
  for (ram_unit_idx = (int)entry_info->num_ram_units - 1, j = 0;
       ram_unit_idx >= 0;
       ram_unit_idx--, j++) {
    /* Generate address for exm */
    addr[j] = exm_tbl->dev_info->dev_cfg.get_full_phy_addr(
        exm_tbl->direction,
        phy_pipe_id,
        entry_info->stage_id,
        entry_info->mem_id[ram_unit_idx],
        mem_addr,
        pipe_mem_type_unit_ram);
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(dev_id,
                                     subdev,
                                     exm_tbl->mat_tbl_hdl,
                                     exm_stage_info->stage_table_handle,
                                     entry_info->stage_id,
                                     RMT_TBL_TYPE_HASH_MATCH,
                                     &addr[0],
                                     entry_info->num_ram_units,
                                     entry_position,
                                     0,
                                     &c_len,
                                     str,
                                     max_len,
                                     NULL,
                                     NULL,
                                     NULL /*sess_hdl*/);

  return rc;
}

/* Dump action data based on entry placement info */
static pipe_status_t pipe_mgr_adt_dump_entry_details(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe_id,
    pipe_mgr_adt_t *adt_tbl,
    uint8_t stage_id,
    pipe_adt_ent_idx_t entry_idx,
    pipe_act_fn_hdl_t act_fn_hdl,
    int *c_str_len,
    char *str,
    int max_len) {
  int c_len = *c_str_len;
  pipe_status_t rc = PIPE_SUCCESS;
  uint8_t entry_position = 0;
  uint64_t addr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  pipe_mgr_adt_data_t *adt_tbl_data;
  int ram_idx = 0, j = 0;
  uint32_t num_srams = 0;
  uint32_t num_entries_per_wide_word = 0, ram_line_num = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint8_t wide_word_blk_idx = 0;
  mem_id_t *ram_id_arr = NULL;
  pipe_mgr_adt_ram_alloc_info_t *adt_ram_alloc_info = NULL;

  if (!adt_tbl->symmetric && log_pipe_id >= adt_tbl->num_tbls) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  adt_tbl_data =
      &(adt_tbl->adt_tbl_data[(adt_tbl->symmetric) ? 0 : log_pipe_id]);
  adt_ram_alloc_info = pipe_mgr_adt_get_ram_alloc_info(adt_tbl_data, stage_id);
  if (adt_ram_alloc_info == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not get adt ram alloc info with handle 0x%x \n",
                      adt_tbl->adt_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get details of HW location */
  num_srams = adt_ram_alloc_info->num_rams_in_wide_word;
  num_entries_per_wide_word_blk =
      adt_ram_alloc_info->num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;
  wide_word_blk_idx = entry_idx / num_entries_per_wide_word_blk;
  num_entries_per_wide_word = adt_ram_alloc_info->num_entries_per_wide_word;
  ram_line_num =
      ((entry_idx) / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  ram_id_arr = adt_ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id;
  entry_position = entry_idx % (num_entries_per_wide_word);
  PIPE_MGR_MEMSET(&addr, 0, sizeof(addr));

  bf_dev_pipe_t phy_pipe_id = log_pipe_id;
  pipe_mgr_map_pipe_id_log_to_phy(adt_tbl->dev_info, log_pipe_id, &phy_pipe_id);
  for (ram_idx = (int)num_srams - 1, j = 0; ram_idx >= 0; ram_idx--, j++) {
    /* Generate the address for adt */
    addr[j] =
        adt_tbl->dev_info->dev_cfg.get_full_phy_addr(adt_tbl->direction,
                                                     phy_pipe_id,
                                                     stage_id,
                                                     ram_id_arr[ram_idx],
                                                     ram_line_num,
                                                     pipe_mem_type_unit_ram);
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(dev_id,
                                     subdev,
                                     adt_tbl->adt_tbl_hdl,
                                     0,
                                     stage_id,
                                     RMT_TBL_TYPE_ACTION_DATA,
                                     &addr[0],
                                     num_srams,
                                     entry_position,
                                     act_fn_hdl,
                                     &c_len,
                                     str,
                                     max_len,
                                     NULL,
                                     NULL,
                                     NULL /*sess_hdl*/);

  *c_str_len = c_len;

  return rc;
}

/* Print action data tbl info from HW */
pipe_status_t pipe_mgr_print_hw_adt_tbl_info(bf_dev_id_t dev_id,
                                             pipe_adt_tbl_hdl_t tbl_hdl,
                                             pipe_adt_ent_hdl_t ent_hdl,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             bf_dev_pipe_t log_pipe,
                                             uint8_t stage,
                                             char *str,
                                             int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len = 0;
  pipe_mgr_adt_t *adt_tbl = NULL;
  bf_dev_pipe_t pipe_id = 0;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  pipe_act_fn_hdl_t action_fn_hdl;
  pipe_mgr_adt_stage_location_t *location = NULL;

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Dumping Action entry with table hdl 0x%x , entry hdl: "
                    "0x%x , device %d \n",
                    tbl_hdl,
                    ent_hdl,
                    dev_id);

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Pipe-id: %d, Stage-id %d \n",
                    log_pipe,
                    stage);

  /* Get table info and location details for this entry */
  adt_tbl = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (adt_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find adt tbl with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_SUCCESS;
  }
  if (adt_tbl->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Dump by entry handle not applicable for direct "
                      "addressed action tbl 0x%x",
                      tbl_hdl);
    return PIPE_SUCCESS;
  }

  entry_info = pipe_mgr_adt_get_entry_phy_info(adt_tbl, ent_hdl);
  if (entry_info == NULL) {
    snprintf(str + c_len,
             (c_len < max_len) ? (max_len - c_len - 1) : 0,
             "NOTE: Entry info for handle %d not found\n",
             ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  action_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
  /* validate action function handle */
  if (action_fn_hdl != act_fn_hdl) {
    c_len +=
        snprintf(str + c_len,
                 (c_len < max_len) ? (max_len - c_len - 1) : 0,
                 "NOTE: Action functon handle specified 0x%x, does not match"
                 " with database value 0x%x \n",
                 act_fn_hdl,
                 action_fn_hdl);
  }

  if (log_pipe != PIPE_INVALID_VAL) {
    pipe_id = log_pipe;
  }

  location = entry_info->sharable_stage_location;
  while (location) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Dumping details for entry index %d \n",
                      location->entry_idx);
    rc = pipe_mgr_adt_dump_entry_details(dev_id,
                                         pipe_id,
                                         adt_tbl,
                                         location->stage_id,
                                         location->entry_idx,
                                         action_fn_hdl,
                                         &c_len,
                                         str,
                                         max_len);
    location = location->next;
  }

  location = entry_info->non_sharable_stage_location;
  while (location) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Dumping details for entry index %d \n",
                      location->entry_idx);
    rc = pipe_mgr_adt_dump_entry_details(dev_id,
                                         pipe_id,
                                         adt_tbl,
                                         location->stage_id,
                                         location->entry_idx,
                                         action_fn_hdl,
                                         &c_len,
                                         str,
                                         max_len);
    location = location->next;
  }

  return rc;
}

/* Print action data tbl info from HW */
static pipe_status_t pipe_mgr_print_hw_adt_tbl_info_by_idx(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t tbl_hdl,
    pipe_adt_ent_idx_t entry_idx,
    pipe_act_fn_hdl_t act_fn_hdl,
    bf_dev_pipe_t log_pipe,
    uint8_t stage_id,
    char *str,
    int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id = 0;
  pipe_mgr_adt_t *adt_tbl = NULL;
  int c_len = 0;
  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Dumping details for entry index %d \n",
                    entry_idx);
  adt_tbl = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (adt_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find adt tbl with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (log_pipe != PIPE_INVALID_VAL) {
    pipe_id = log_pipe;
  }
  rc = pipe_mgr_adt_dump_entry_details(dev_id,
                                       pipe_id,
                                       adt_tbl,
                                       stage_id,
                                       entry_idx,
                                       act_fn_hdl,
                                       &c_len,
                                       str,
                                       max_len);
  return rc;
}

/* Print tcam tbl info from HW */
pipe_status_t pipe_mgr_print_hw_tcam_tbl_info(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              pipe_mat_ent_hdl_t ent_hdl,
                                              bf_dev_pipe_t pipe,
                                              uint32_t subindex,
                                              char *str,
                                              int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bool is_backup = false;
  tcam_llp_entry_t *tcam_entry;
  bf_dev_pipe_t pipe_id = 0;
  int i = 0, j = 0;
  uint32_t line_no = 0, stage_line_no = 0;
  uint8_t stage_id = 0;
  uint64_t addr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  int c_len = 0, num_tind_entries = 0;
  uint32_t tind_block = 0;
  uint32_t tind_subword_pos = 0;
  tcam_stage_info_t *stage_data = NULL;
  char pipe_str[100];
  rmt_dev_profile_info_t *profile = NULL;
  (void)subindex;

  c_len += snprintf(
      str + c_len,
      (c_len < max_len) ? (max_len - c_len - 1) : 0,
      "Dumping tcam entry with table hdl 0x%x , entry hdl: 0x%x , device %d\n",
      tbl_hdl,
      ent_hdl,
      dev_id);

  /* Get table info and location details for this entry */
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find tcam tbl with handle 0x%x \n",
                      tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_entry = pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, ent_hdl, subindex);
  if (tcam_entry == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find tcam entry with ent handle 0x%x \n",
                      ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  profile = pipe_mgr_get_profile(
      tcam_tbl_info->dev_info, tcam_tbl_info->profile_id, __func__, __LINE__);

  /* Is this default entry */
  if (tcam_entry->is_default) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Default entry dump \n");

    rc = pipe_mgr_dump_def_match_entry(dev_id,
                                       tcam_tbl_info->direction,
                                       tbl_hdl,
                                       pipe,
                                       profile,
                                       str,
                                       &c_len,
                                       max_len);

    return rc;
  }

  tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, tcam_entry->pipe_id);
  if (tcam_pipe_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find tcam table for pipe %d \n",
                      tcam_entry->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
  if (tcam_tbl == NULL) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Could not find tcam table with partition %d \n",
                      tcam_entry->ptn_index);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Print the pipe */
  if (pipe != PIPE_INVALID_VAL) {
    if ((tcam_pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) &&
        (pipe != tcam_pipe_tbl->pipe_id)) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Invalid pipe %d specified, entry is on pipe %d \n",
                        pipe,
                        tcam_pipe_tbl->pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    pipe_id = pipe;
    sprintf(pipe_str, "%d", pipe_id);
  } else if (tcam_pipe_tbl->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = 0;
    sprintf(pipe_str, "All");
  } else {
    pipe_id = tcam_pipe_tbl->pipe_id;
    sprintf(pipe_str, "%d", pipe_id);
  }

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

  tcam_block_data_t *block_data;
  line_no = tcam_loc.phy_line_no;
  block_data = get_tcam_block_data(tcam_tbl, &tcam_loc);
  stage_data = get_tcam_stage_data(tcam_tbl, &tcam_loc);
  stage_id = stage_data->stage_id;
  stage_line_no = tcam_loc.stage_line_no;

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Pipe-id: %s,  Stage-id: %d \n",
                    pipe_str,
                    stage_data->stage_id);
  if (tcam_tbl_info->adt_present) {
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->adt_tbl_ref;
    if (tbl_ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Action: [Table handle 0x%x , Entry handle: %d ]\n",
                        tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                        tcam_entry->adt_ent_hdl);
    } else {
      pipe_adt_tbl_hdl_t adt_tbl_hdl = tbl_ref->tbl_hdl;
      bf_dev_pipe_t pipe_id_ = 0;
      stage_id = 0;
      uint32_t idx;
      rc = pipe_mgr_tcam_entry_get_programmed_location(
          dev_id, tbl_hdl, ent_hdl, 0, &pipe_id_, &stage_id, NULL, &idx);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Action: [Table handle 0x%x , Entry idx: %d ]\n",
                        adt_tbl_hdl,
                        idx);
    }
  }
  PIPE_MGR_MEMSET(&addr, 0, sizeof(addr));

  bf_dev_pipe_t phy_pipe_id = pipe_id;
  pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe_id);
  bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  for (i = (int)stage_data->pack_format.mem_units_per_tbl_word - 1, j = 0;
       i >= 0;
       i--, j++) {
    pipe_mem_type_t pipe_mem_type;
    pipe_mem_type = is_atcam ? pipe_mem_type_unit_ram : pipe_mem_type_tcam;

    /* Generate the address for tcam */
    addr[j] = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
        tcam_tbl_info->direction,
        phy_pipe_id,
        stage_data->stage_id,
        block_data->word_blk.mem_id[i],
        line_no,
        pipe_mem_type);
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(
      dev_id,
      subdev,
      tcam_tbl_info->tbl_hdl,
      stage_data->stage_table_handle,
      stage_id,
      is_atcam ? RMT_TBL_TYPE_ATCAM_MATCH : RMT_TBL_TYPE_TERN_MATCH,
      &addr[0],
      stage_data->pack_format.mem_units_per_tbl_word,
      tcam_loc.subword,
      0,
      &c_len,
      str,
      max_len,
      NULL,
      NULL,
      NULL /*sess_hdl*/);
  if (rc != PIPE_SUCCESS) return rc;

  /* Start Decode of the tind entry */
  uint32_t tind_line_no = 0;
  bool tind_exists = pipe_mgr_tcam_tind_get_line_no(
      stage_data, stage_line_no, &tind_line_no, &tind_block, &tind_subword_pos);
  if (!tind_exists) {
    return PIPE_SUCCESS;
  }
  PIPE_MGR_MEMSET(&addr, 0, sizeof(addr));
  num_tind_entries = 1;  // always 1
  for (i = num_tind_entries - 1, j = 0; i >= 0; i--, j++) {
    /* Generate the address for tind */
    addr[j] = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
        tcam_tbl_info->direction,
        phy_pipe_id,
        stage_data->stage_id,
        stage_data->tind_blk[tind_block].mem_id[i],
        tind_line_no,
        pipe_mem_type_unit_ram);
  }

  rc = pipe_mgr_dump_any_tbl_by_addr(dev_id,
                                     subdev,
                                     tcam_tbl_info->tbl_hdl,
                                     0,
                                     stage_id,
                                     RMT_TBL_TYPE_TERN_INDIR,
                                     &addr[0],
                                     num_tind_entries,
                                     tind_subword_pos,
                                     0,
                                     &c_len,
                                     str,
                                     max_len,
                                     NULL,
                                     NULL,
                                     NULL /*sess_hdl*/);

  return rc;
}

/* Print match table info from HW */
pipe_status_t pipe_mgr_print_mat_hw_tbl_info(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             pipe_mat_ent_hdl_t ent_hdl,
                                             bf_dev_pipe_t log_pipe,
                                             uint32_t subindex,
                                             char *str,
                                             int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_print_hw_tcam_tbl_info(
        dev_id, tbl_hdl, ent_hdl, log_pipe, subindex, str, max_len);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_print_hw_exm_tbl_info(
        dev_id, tbl_hdl, ent_hdl, log_pipe, str, max_len);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_print_hw_phase0_tbl_info(
        dev_id, tbl_hdl, ent_hdl, str, max_len);
  } else {
    snprintf(str, max_len, "Not a match table handle 0x%x \n", tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  return rc;
}

/* Print match table info from HW */
pipe_status_t pipe_mgr_print_mat_hw_tbl_info_w_lock(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    pipe_mat_ent_hdl_t ent_hdl,
                                                    bf_dev_pipe_t log_pipe,
                                                    uint32_t subindex,
                                                    char *str,
                                                    int max_len) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  dev_target_t dev_tgt;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(ent_hdl);

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if ((owner != PIPE_MGR_TBL_OWNER_TRN) && (owner != PIPE_MGR_TBL_OWNER_EXM) &&
      (owner != PIPE_MGR_TBL_OWNER_PHASE0)) {
    snprintf(str, max_len, "Not a match table handle 0x%x \n", tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
          pipe_mgr_print_mat_hw_tbl_info(
              dev_id, tbl_hdl, ent_hdl, log_pipe, subindex, str, max_len));
}

/* Print action table info from HW */
pipe_status_t pipe_mgr_print_act_hw_tbl_info_w_lock(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t tbl_hdl,
    pipe_adt_ent_hdl_t ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    bf_dev_pipe_t log_pipe,
    uint8_t stage,
    char *str,
    int max_len) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  dev_target_t dev_tgt;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner != PIPE_MGR_TBL_OWNER_ADT) {
    snprintf(
        str, max_len, "Not an action action table handle 0x%x \n", tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(ent_hdl);

  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
      pipe_mgr_print_hw_adt_tbl_info(
          dev_id, tbl_hdl, ent_hdl, act_fn_hdl, log_pipe, stage, str, max_len));
}

/* Print action table info from HW */
pipe_status_t pipe_mgr_print_act_hw_tbl_info_by_idx_w_lock(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t tbl_hdl,
    pipe_adt_ent_idx_t entry_idx,
    pipe_act_fn_hdl_t act_fn_hdl,
    bf_dev_pipe_t log_pipe,
    uint8_t stage,
    char *str,
    int max_len) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  dev_target_t dev_tgt;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner != PIPE_MGR_TBL_OWNER_ADT) {
    snprintf(
        str, max_len, "Not an action action table handle 0x%x \n", tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe;

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
          pipe_mgr_print_hw_adt_tbl_info_by_idx(dev_id,
                                                tbl_hdl,
                                                entry_idx,
                                                act_fn_hdl,
                                                log_pipe,
                                                stage,
                                                str,
                                                max_len));
}

/* Dump from any HW address */
pipe_status_t pipe_mgr_dump_hw_memory(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev,
                                      uint64_t addr,
                                      char *str,
                                      int max_len) {
  int c_len = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t data0 = 0, data1 = 0;
  rmt_ram_line_t ram_line[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
  uint8_t *ram_ptr;
  int cntr = 0;

  rc = lld_subdev_ind_read(dev_id, subdev, addr, &data1, &data0);
  if (rc != PIPE_SUCCESS) {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "HW read failed, retcode=%d ",
                      rc);
    return rc;
  }

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Address Read: 0x%" PRIx64 "  \n",
                    addr);

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "Data0: 0x%" PRIx64 " , Data1 0x%" PRIx64 " \n",
                    data0,
                    data1);

  /* Reverse the bytes */
  ram_ptr = (uint8_t *)&ram_line[0];
  for (cntr = 0; cntr < 64 / 8; cntr++) {
    *ram_ptr = (data0 >> (8 * cntr)) & 0xff;  // right shift by 8
    ram_ptr++;
  }
  for (cntr = 0; cntr < 64 / 8; cntr++) {
    *ram_ptr = (data1 >> (8 * cntr)) & 0xff;  // right shift by 8
    ram_ptr++;
  }
  pipe_mgr_dump_tbl_word((uint8_t *)&ram_line[0],
                         NULL,
                         str,
                         &c_len,
                         max_len,
                         0,
                         TOF_SRAM_UNIT_WIDTH);

  return rc;
}

pipe_status_t pipe_mgr_dump_hw_memory_full(bf_dev_id_t dev_id,
                                           pipe_tbl_dir_t gress,
                                           bf_dev_pipe_t log_pipe_id,
                                           uint8_t stage_id,
                                           mem_id_t mem_id,
                                           uint8_t mem_type,
                                           uint16_t line_no,
                                           char *str,
                                           int max_len) {
  bf_dev_pipe_t phy_pipe_id = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t address = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (dev_info) {
    rc = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
  }
  if (!dev_info || rc != PIPE_SUCCESS) {
    snprintf(str,
             max_len - 1,
             "Error in getting physical pipe-id for logical pipe %d \n",
             log_pipe_id);
    return rc;
  }
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);

  address = dev_info->dev_cfg.get_full_phy_addr(
      gress, phy_pipe_id, stage_id, mem_id, line_no, mem_type);

  return pipe_mgr_dump_hw_memory(dev_id, subdev, address, str, max_len);
}

/* Write to any HW address */
pipe_status_t pipe_mgr_write_hw_memory(bf_dev_id_t dev_id,
                                       bf_dev_id_t subdev,
                                       uint64_t addr,
                                       uint64_t data0,
                                       uint64_t data1,
                                       char *str,
                                       int max_len) {
  int c_len = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rc = lld_subdev_ind_write(dev_id, subdev, addr, data1, data0);
  if (rc != PIPE_SUCCESS) {
    snprintf(str + c_len,
             (c_len < max_len) ? (max_len - c_len - 1) : 0,
             "HW write failed, retcode=%d ",
             rc);
    return rc;
  }

  c_len += snprintf(str + c_len,
                    (c_len < max_len) ? (max_len - c_len - 1) : 0,
                    "HW write done to address: 0x%" PRIx64 "  \n",
                    addr);
  snprintf(str + c_len,
           (c_len < max_len) ? (max_len - c_len - 1) : 0,
           "Data written, Data0 0x%" PRIx64 " , Data1 0x%" PRIx64 " \n",
           data0,
           data1);

  return rc;
}

/* Generate a mask with range bits set as zero */
uint64_t get_mask(int start_bit, int end_bit) {
  uint64_t mask = 0;
  int i = 0;

  for (i = 63; i >= 0; i--) {
    if (!((i >= start_bit) && (i <= end_bit))) {
      mask |= (uint64_t)0x1 << i;
    }
  }

  return mask;
}

/* Write to memory, start and End bit may be specified */
pipe_status_t pipe_mgr_write_hw_memory_with_start_end(bf_dev_id_t dev_id,
                                                      bf_subdev_id_t subdev,
                                                      uint64_t addr,
                                                      uint64_t data0,
                                                      uint64_t data1,
                                                      int start_bit,
                                                      int end_bit,
                                                      char *str,
                                                      int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t new_data0 = 0, new_data1 = 0;
  uint64_t read_data0 = 0, read_data1 = 0;
  int num_bytes = 0;
  int last_bit = 0;
  uint64_t mask0 = 0xffffffffffffffff;
  uint64_t mask1 = 0xffffffffffffffff;

  /* If start and end are not specified, write data without massaging */
  if ((start_bit == -1) || (end_bit == -1)) {
    rc = pipe_mgr_write_hw_memory(
        dev_id, subdev, addr, data0, data1, str, max_len);
    return rc;
  }

  /* Get num of bytes to write */
  num_bytes = (end_bit - start_bit + 1) / 8;
  if (((end_bit - start_bit + 1) % 8) > 0) num_bytes++;
  last_bit = num_bytes * 8;

  /* everything goes in data1 */
  if (start_bit >= 64) {
    new_data1 |= data1 >> (64 - (start_bit - 64) - last_bit);
    mask1 = get_mask(start_bit - 64, end_bit - 64);
    new_data1 &= (~mask1);
  } else if (end_bit < 64) {
    /* everything goes int data0 */
    new_data0 |= data1 >> (64 - start_bit - last_bit);
    mask0 = get_mask(start_bit, end_bit);
    new_data0 &= (~mask0);
  } else { /* Not supported */
    return rc;
  }

  /* Read data */
  rc = lld_subdev_ind_read(dev_id, subdev, addr, &read_data1, &read_data0);
  if (rc != PIPE_SUCCESS) {
    snprintf(str, max_len, "HW read failed, retcode=%d ", rc);
    return rc;
  }
  /* Zero out the interested bit range */
  read_data0 &= mask0;
  read_data1 &= mask1;

  /* Set the read bits */
  new_data0 |= read_data0;
  new_data1 |= read_data1;

  /* Write modified value */
  rc = pipe_mgr_write_hw_memory(
      dev_id, subdev, addr, new_data0, new_data1, str, max_len);

  return rc;
}

/* Write to TCAM */
pipe_status_t pipe_mgr_write_tcam_memory(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         uint64_t addr,
                                         uint64_t key,
                                         uint64_t mask,
                                         uint8_t payload,
                                         uint8_t mrd,
                                         uint64_t version,
                                         char *str,
                                         int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t data0 = 0, data1 = 0;
  bool is_backup = false;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    snprintf(str,
             max_len,
             "Could not find tcam tbl info with handle 0x%x \n",
             tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    snprintf(
        str,
        max_len,
        "Table 0x%x is an algorithmic table. Write cmds not yet supported\n",
        tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  uint8_t stage_id = tcam_tbl_info->dev_info->dev_cfg.stage_id_from_addr(addr);
  uint8_t mem_id = tcam_tbl_info->dev_info->dev_cfg.mem_id_from_addr(addr);
  uint64_t shifted_key = (key << 1) | (mrd & 0x1);
  uint64_t shifted_mask = (mask << 1) | (payload & 0x1);
  if (version != PYLD_INVALID_VAL) {
    shifted_key |= version << 43;
    shifted_mask |= 0x3ull << 43;
  }

  /* Convert the key/mask and payload, parity to HW (x/y) format */
  rc = pipe_mgr_entry_format_tof_tern_tbl_conv_key_mask(
      dev_id,
      tcam_tbl_info->profile_id,
      stage_id,
      mem_id,
      tbl_hdl,
      shifted_key,
      shifted_mask,
      &data0,
      &data1,
      (version != PYLD_INVALID_VAL));
  if (rc != PIPE_SUCCESS) return rc;

  /* Write to HW */
  rc = pipe_mgr_write_hw_memory(
      dev_id, subdev, addr, data0, data1, str, max_len);

  return rc;
}

/* Write to TCAM with start and end bit */
pipe_status_t pipe_mgr_write_tcam_memory_with_start_end(
    bf_dev_id_t dev_id,
    bf_subdev_id_t subdev,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint64_t addr,
    uint64_t key,
    uint64_t mask,
    uint8_t payload,
    uint8_t mrd,
    uint64_t version,
    int start_bit,
    int end_bit,
    char *str,
    int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t new_key = 0, new_mask = 0;
  uint64_t read_key = 0, read_mask = 0;
  bool is_backup = false;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  /* If start and end are not specified, write data without massaging */
  if ((start_bit == -1) || (end_bit == -1)) {
    rc = pipe_mgr_write_tcam_memory(dev_id,
                                    subdev,
                                    tbl_hdl,
                                    addr,
                                    key,
                                    mask,
                                    payload,
                                    mrd,
                                    version,
                                    str,
                                    max_len);
    return rc;
  }

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    snprintf(str,
             max_len,
             "Could not find tcam tbl info with handle 0x%x \n",
             tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    snprintf(
        str,
        max_len,
        "Table 0x%x is an algorithmic table. Write cmds not yet supported\n",
        tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  uint8_t stage_id = tcam_tbl_info->dev_info->dev_cfg.stage_id_from_addr(addr);
  uint8_t mem_id = tcam_tbl_info->dev_info->dev_cfg.mem_id_from_addr(addr);

  /* +1 for hte payload/mrd bit */
  uint64_t shifted_key = key << (start_bit + 1);
  uint64_t shifted_mask = mask << (start_bit + 1);

  if (payload != PYLD_INVALID_VAL) {
    shifted_mask |= payload & 0x1;
  }
  if (mrd != PYLD_INVALID_VAL) {
    shifted_key |= mrd & 0x1;
  }
  if (version != PYLD_INVALID_VAL) {
    shifted_key |= version << 43;
    shifted_mask |= 0x3ull << 43;
  }

  /* Create write_mask */
  /* write_mask specifies which of the bits to pick from the user provided input
   * and which of them to use from the value read from hardware
   */
  uint64_t write_mask0, write_mask1;
  write_mask0 = (1ull << (end_bit - start_bit + 1)) - 1;
  /* Shift by 1 to account for the MRD/payload bit */
  write_mask0 <<= start_bit + 1;
  write_mask1 = write_mask0;
  if (payload != PYLD_INVALID_VAL) {
    write_mask1 |= 0x1;
  }
  if (mrd != PYLD_INVALID_VAL) {
    write_mask0 |= 0x1;
  }
  if (version != PYLD_INVALID_VAL) {
    write_mask0 |= 0x3ull << 43;
    write_mask1 |= 0x3ull << 43;
  }

  /* Read the data  */
  rc = lld_subdev_ind_read(dev_id, subdev, addr, &read_mask, &read_key);
  if (rc != PIPE_SUCCESS) {
    snprintf(str, max_len, "HW read failed, retcode=%d \n", rc);
    return rc;
  }

  uint8_t read_word0[TOF_BYTES_IN_TCAM_WORD],
      read_word1[TOF_BYTES_IN_TCAM_WORD];
  unsigned i;
  for (i = 0; i < TOF_BYTES_IN_TCAM_WORD; i++) {
    read_word0[i] = read_key & 0xff;
    read_word1[i] = read_mask & 0xff;
    read_key >>= 8;
    read_mask >>= 8;
  }
  /* Convert the read-key and mask to key-mask format */
  rc = pipe_mgr_entry_format_tof_tern_decode_to_key_mask(
      dev_id,
      tcam_tbl_info->profile_id,
      stage_id,
      tbl_hdl,
      true,
      mem_id,
      false,
      0,
      read_word0,
      read_word1);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }
  read_key = 0;
  read_mask = 0;
  for (i = 0; i < TOF_BYTES_IN_TCAM_WORD; i++) {
    read_key |= (uint64_t)read_word0[i] << (8 * i);
    read_mask |= (uint64_t)read_word1[i] << (8 * i);
  }

  shifted_key = (shifted_key & write_mask0) | (read_key & ~write_mask0);
  shifted_mask = (shifted_mask & write_mask1) | (read_mask & ~write_mask1);

  /* Convert the key/mask and payload, parity to HW (x/y) format */
  rc = pipe_mgr_entry_format_tof_tern_tbl_conv_key_mask(
      dev_id,
      tcam_tbl_info->profile_id,
      stage_id,
      mem_id,
      tbl_hdl,
      shifted_key,
      shifted_mask,
      &new_key,
      &new_mask,
      (version != PYLD_INVALID_VAL));
  if (rc != PIPE_SUCCESS) return rc;

  /* Write to HW */
  rc = pipe_mgr_write_hw_memory(
      dev_id, subdev, addr, new_key, new_mask, str, max_len);

  return rc;
}

/* Write to register */
pipe_status_t pipe_mgr_dbg_write_register(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t addr,
                                          uint32_t data,
                                          char *str,
                                          int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len;

  rc = pipe_mgr_write_register(dev_id, subdev_id, addr, data);

  c_len = snprintf(str,
                   max_len - 1,
                   "Register write done to address: 0x%x on device %d \n",
                   addr,
                   dev_id);

  snprintf(str + c_len,
           (c_len < max_len) ? (max_len - c_len - 1) : 0,
           "Data written: 0x%x \n",
           data);

  return rc;
}

/* Reverse the order of addresses */
pipe_status_t pipe_mgr_reverse_addr_order(uint64_t *addr, int num_phy_addrs) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint64_t temp = 0;
  int i = 0;

  for (i = 0; i < num_phy_addrs / 2; i++) {
    temp = addr[i];
    addr[i] = addr[num_phy_addrs - i - 1];
    addr[num_phy_addrs - i - 1] = temp;
  }

  return rc;
}

/* Read from a action table using virtual address */
pipe_status_t pipe_mgr_dump_act_tbl_by_virtaddr(bf_dev_id_t dev_id,
                                                uint32_t virt_addr,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bf_dev_pipe_t log_pipe_id,
                                                uint8_t stage_id,
                                                pipe_act_fn_hdl_t act_fn_hdl,
                                                char *str,
                                                int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len = 0;
  int owner = PIPE_MGR_TBL_OWNER_NONE;
  dev_target_t dev_tgt;
  uint64_t addr[16] = {0};
  int entry_position = 0, num_phy_addrs = 0;
  rmt_tbl_type_t tbl_type = 0;

  c_len +=
      snprintf(str + c_len,
               (c_len < max_len) ? (max_len - c_len - 1) : 0,
               "Dump for action vpn-addr 0x%x on device %d, pipe %d, stage %d, "
               " tbl_hdl 0x%x, act-fn-hdl 0x%x\n",
               virt_addr,
               dev_id,
               log_pipe_id,
               stage_id,
               tbl_hdl,
               act_fn_hdl);

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    tbl_type = RMT_TBL_TYPE_ACTION_DATA;
  } else {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Not an action table handle 0x%x \n",
                      tbl_hdl);
    rc = PIPE_INVALID_ARG;
    return rc;
  }

  PIPE_MGR_MEMSET(&dev_tgt, 0, sizeof(dev_tgt));
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe_id;

  /* Get physical addr from virt addr */
  pipe_mgr_get_phy_addr_for_vpn_addr(dev_tgt,
                                     tbl_hdl,
                                     stage_id,
                                     false,
                                     0,
                                     tbl_type,
                                     virt_addr,
                                     &addr[0],
                                     &num_phy_addrs,
                                     &entry_position);

  /* Reverse addr order, decoder expects highest addr first */
  pipe_mgr_reverse_addr_order(&addr[0], num_phy_addrs);

  /* Dump the address */
  bf_dev_pipe_t phy_pipe_id = log_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_VAL;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(dev_id,
                                     subdev,
                                     tbl_hdl,
                                     0,
                                     stage_id,
                                     tbl_type,
                                     &addr[0],
                                     num_phy_addrs,
                                     entry_position,
                                     act_fn_hdl,
                                     &c_len,
                                     str,
                                     max_len,
                                     NULL,
                                     NULL,
                                     NULL /*sess_hdl*/);

  return rc;
}

/* Read from a match table using virtual address */
pipe_status_t pipe_mgr_dump_mat_tbl_by_virtaddr(bf_dev_id_t dev_id,
                                                uint32_t virt_addr,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bf_dev_pipe_t log_pipe_id,
                                                uint8_t stage_id,
                                                uint8_t stage_table_handle,
                                                int is_tind,
                                                char *str,
                                                int max_len) {
  pipe_status_t rc = PIPE_SUCCESS;
  int c_len = 0;
  int owner = PIPE_MGR_TBL_OWNER_NONE;
  dev_target_t dev_tgt;
  uint64_t addr[16] = {0};
  int entry_position = 0, num_phy_addrs = 0;
  rmt_tbl_type_t tbl_type = 0;
  bool check_stage_table_handle = false;

  c_len += snprintf(
      str + c_len,
      (c_len < max_len) ? (max_len - c_len - 1) : 0,
      "Dump for match-tbl vpn-addr 0x%x on device %d, pipe %d, stage %d,"
      " tbl_hdl 0x%x, is_tind %s\n",
      virt_addr,
      dev_id,
      log_pipe_id,
      stage_id,
      tbl_hdl,
      is_tind ? "Yes" : "No");

  /* Get profile */
  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    /* Get table info and location details for this entry */
    tcam_tbl_info_t *tcam_tbl_info = NULL;
    tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
    if (tcam_tbl_info == NULL) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Could not find tcam tbl with handle 0x%x \n",
                        tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);

    if (is_atcam && is_tind) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "TIND does not exist for this table 0x%x\n",
                        tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (is_tind) {
      tbl_type = RMT_TBL_TYPE_TERN_INDIR;
    } else if (is_atcam) {
      tbl_type = RMT_TBL_TYPE_ATCAM_MATCH;
      check_stage_table_handle = true;
    } else {
      tbl_type = RMT_TBL_TYPE_TERN_MATCH;
    }
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    tbl_type = RMT_TBL_TYPE_HASH_MATCH;

    pipe_mgr_exm_tbl_t *exm_tbl = NULL;
    exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
    if (exm_tbl == NULL) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Could not find exm tbl info with handle 0x%x \n",
                        tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    pipe_mgr_exm_stage_info_t *exm_stage_info;
    exm_stage_info =
        pipe_mgr_exm_tbl_get_stage_info(exm_tbl, log_pipe_id, stage_id);

    if (exm_stage_info == NULL) {
      c_len += snprintf(str + c_len,
                        (c_len < max_len) ? (max_len - c_len - 1) : 0,
                        "Could not find exm tbl 0x%x in stage %d\n",
                        tbl_hdl,
                        stage_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    stage_table_handle = exm_stage_info->stage_table_handle;
    check_stage_table_handle = true;
  } else {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Not a match table handle 0x%x \n",
                      tbl_hdl);
    rc = PIPE_INVALID_ARG;
    return rc;
  }

  PIPE_MGR_MEMSET(&dev_tgt, 0, sizeof(dev_tgt));
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe_id;

  /* Get physical addr from virt addr */
  pipe_mgr_get_phy_addr_for_vpn_addr(dev_tgt,
                                     tbl_hdl,
                                     stage_id,
                                     check_stage_table_handle,
                                     stage_table_handle,
                                     tbl_type,
                                     virt_addr,
                                     &addr[0],
                                     &num_phy_addrs,
                                     &entry_position);

  /* Reverse addr order, decoder expects highest addr first */
  pipe_mgr_reverse_addr_order(&addr[0], num_phy_addrs);

  /* Dump the address */
  bf_dev_pipe_t phy_pipe_id = log_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_VAL;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(dev_id,
                                     subdev,
                                     tbl_hdl,
                                     stage_table_handle,
                                     stage_id,
                                     tbl_type,
                                     &addr[0],
                                     num_phy_addrs,
                                     entry_position,
                                     0,
                                     &c_len,
                                     str,
                                     max_len,
                                     NULL,
                                     NULL,
                                     NULL /*sess_hdl*/);

  return rc;
}

#define RD_BUF_SZ 16
struct ilist_rd_cookie {
  uint8_t buf[RD_BUF_SZ];
  uint32_t buf_sz;
};

static void reg_rd_cb(void *cookie,
                      bf_dev_id_t dev,
                      bf_dev_pipe_t pipe,
                      uint32_t byte_offset,
                      uint8_t *buf,
                      uint32_t buf_sz,
                      bool all_data_complete,
                      bool user_cb_safe) {
  (void)dev;
  (void)pipe;
  (void)byte_offset;
  (void)all_data_complete;
  (void)user_cb_safe;
  struct ilist_rd_cookie *data = cookie;

  if (buf_sz && buf_sz != data->buf_sz) {
    LOG_ERROR("%s:%d Memory read out of sync with provided buffer %d != %d",
              __func__,
              __LINE__,
              buf_sz,
              data->buf_sz);
    return;
  }

  if (buf_sz) {
    PIPE_MGR_MEMCPY(&data->buf, buf, buf_sz);
  }
}

pipe_status_t pipe_mgr_dma_read_by_addr(pipe_sess_hdl_t *sess_hdl,
                                        bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev,
                                        uint64_t addr,
                                        uint64_t *data_hi,
                                        uint64_t *data_lo) {
  /* Unpack memory data for read. */

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  pipe_status_t rc = BF_INVALID_ARG;

  if (!dev_info) return rc;

  bf_dev_pipe_t pipe_id = dev_info->dev_cfg.pipe_id_from_addr(addr);
  /* Translate phy pipe to log pipe */
  pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
      dev_info, pipe_id, &pipe_id);
  pipe_id += subdev * BF_SUBDEV_PIPE_COUNT;
  uint8_t stage_id = dev_info->dev_cfg.stage_id_from_addr(addr);
  int mem_id = dev_info->dev_cfg.mem_id_from_addr(addr);
  uint32_t mem_addr = dev_info->dev_cfg.mem_addr_from_addr(addr);
  pipe_mem_type_t pipe_mem_type = dev_info->dev_cfg.mem_type_from_addr(addr);

  /* Create read instruction */
  pipe_instr_get_memdata_t instr;
  construct_instr_get_memdata(dev_id, &instr, mem_id, mem_addr, pipe_mem_type);

  rc = pipe_mgr_drv_ilist_rd_add(
      sess_hdl, dev_info, pipe_id, stage_id, (uint8_t *)&instr, sizeof(instr));
  if (rc) return rc;

  /* Return cookie */
  struct ilist_rd_cookie data = {0};
  data.buf_sz = RD_BUF_SZ;

  rc = pipe_mgr_drv_ilist_rd_push(sess_hdl, reg_rd_cb, &data);
  if (rc) return rc;

  rc = pipe_mgr_drv_ilist_rd_cmplt_all(sess_hdl);
  if (rc) return rc;
  PIPE_MGR_MEMCPY(data_lo, data.buf, sizeof(uint64_t));
  PIPE_MGR_MEMCPY(data_hi, data.buf + data.buf_sz / 2, sizeof(uint64_t));

  return PIPE_SUCCESS;
}

/* Dump any table based on physical addresses */
pipe_status_t pipe_mgr_dump_any_tbl_by_addr(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            uint8_t stage_table_handle,
                                            uint8_t stage_id,
                                            rmt_tbl_type_t tbl_type,
                                            uint64_t *addr,
                                            int num_phy_addrs,
                                            int entry_position,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            int *c_str_len,
                                            char *str,
                                            int max_len,
                                            uint8_t **read_data,
                                            uint8_t *phy_addrs_map,
                                            pipe_sess_hdl_t *sess_hdl) {
  int c_len = 0;
  if (c_str_len) {
    c_len = *c_str_len;
  }
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;
  uint64_t data0 = 0, data1 = 0;
  tind_tbl_word_t tind_tbl_word;
  exm_tbl_word_t exm_tbl_word;
  tern_tbl_word_t tbl_word0, tbl_word1;
  adt_tbl_word_t adt_tbl_word;
  int idx = 0, cntr = 0;
  int num_bytes = 0;
  uint8_t *data_ptr = NULL;
  tcam_tbl_info_t *tcam_tbl_info;
  pipe_mgr_exm_tbl_t *exm_tbl;
  pipe_mgr_adt_t *adt_tbl;
  profile_id_t prof_id = 0;
  rmt_dev_info_t *dev_info = NULL;

  /* Get profile */
  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
    if (!tcam_tbl_info) {
      return PIPE_INVALID_ARG;
    }
    prof_id = tcam_tbl_info->profile_id;
    dev_info = tcam_tbl_info->dev_info;
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
    if (!exm_tbl) {
      return PIPE_INVALID_ARG;
    }
    prof_id = exm_tbl->profile_id;
    dev_info = exm_tbl->dev_info;
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    adt_tbl = pipe_mgr_adt_get(dev_id, tbl_hdl);
    if (adt_tbl == NULL) {
      return PIPE_OBJ_NOT_FOUND;
    }
    prof_id = adt_tbl->profile_id;
    dev_info = adt_tbl->dev_info;
  } else {
    c_len += snprintf(str + c_len,
                      (c_len < max_len) ? (max_len - c_len - 1) : 0,
                      "Unsupported table handle 0x%x \n",
                      tbl_hdl);
    rc = PIPE_INVALID_ARG;
    return rc;
  }

  /* Zero out data structures */
  PIPE_MGR_MEMSET(&tbl_word0, 0, sizeof(tbl_word0));
  PIPE_MGR_MEMSET(&tbl_word1, 0, sizeof(tbl_word1));
  PIPE_MGR_MEMSET(&exm_tbl_word, 0, sizeof(exm_tbl_word));
  PIPE_MGR_MEMSET(&tind_tbl_word, 0, sizeof(tind_tbl_word));
  PIPE_MGR_MEMSET(&adt_tbl_word, 0, sizeof(adt_tbl_word));

  /* Read the data */
  for (idx = 0; idx < num_phy_addrs; idx++) {
    if (sess_hdl) {
      rc = pipe_mgr_dma_read_by_addr(
          sess_hdl, dev_id, subdev, addr[idx], &data1, &data0);
    } else {
      rc = lld_subdev_ind_read(dev_id, subdev, addr[idx], &data1, &data0);
    }
    if (rc != PIPE_SUCCESS) {
      if (str) {
        c_len += snprintf(str + c_len,
                          (c_len < max_len) ? (max_len - c_len - 1) : 0,
                          "HW read failed, retcode=%d \n",
                          rc);
        return rc;
      } else {
        LOG_ERROR("%s:%d HW read failed, err %s",
                  __func__,
                  __LINE__,
                  pipe_str_err(rc));
        return rc;
      }
    }
    if (tbl_type == RMT_TBL_TYPE_TERN_MATCH) {
      if (read_data) {
        data_ptr = read_data[idx];
        num_bytes = 8;
      } else {
        data_ptr = (uint8_t *)&tbl_word0[idx][0];
        num_bytes = TOF_BYTES_IN_TCAM_WORD;
      }
    } else if (tbl_type == RMT_TBL_TYPE_TERN_INDIR) {
      if (read_data) {
        data_ptr = read_data[idx];
      } else {
        data_ptr = (uint8_t *)&tind_tbl_word[idx];
      }
      num_bytes = TOF_BYTES_IN_RAM_WORD / 2;
    } else if ((tbl_type == RMT_TBL_TYPE_HASH_MATCH) ||
               (tbl_type == RMT_TBL_TYPE_ATCAM_MATCH)) {
      if (read_data) {
        data_ptr = read_data[phy_addrs_map[idx]];
      } else {
        data_ptr = (uint8_t *)&exm_tbl_word[idx][0];
      }
      num_bytes = 8;
    } else if (tbl_type == RMT_TBL_TYPE_ACTION_DATA) {
      if (read_data) {
        data_ptr = read_data[idx];
      } else {
        data_ptr = (uint8_t *)&adt_tbl_word[idx][0];
      }
      num_bytes = 8;
    } else {
      if (str) {
        c_len += snprintf(str + c_len,
                          (c_len < max_len) ? (max_len - c_len - 1) : 0,
                          "Unsupported table type %d \n",
                          tbl_type);
        rc = PIPE_INVALID_ARG;
        return rc;
      }
    }
    /* Populate the word array by reversing bytes */
    for (cntr = 0; cntr < num_bytes; cntr++) {
      *data_ptr = (data0 >> (8 * cntr)) & 0xff;  // right shift by 8
      data_ptr++;
    }
    if (tbl_type == RMT_TBL_TYPE_TERN_MATCH) {
      if (!read_data) {
        data_ptr = (uint8_t *)&tbl_word1[idx][0];
      }
    }
    for (cntr = 0; cntr < num_bytes; cntr++) {
      *data_ptr = (data1 >> (8 * cntr)) & 0xff;  // right shift by 8
      data_ptr++;
    }

    // pipe_mgr_dump_tbl_word((uint8_t*)&exm_tbl_word[idx], NULL, str,
    //       &c_len, max_len, ram_idx, num_bytes*8);
  }

  /* Decode the data */
  if (str) {
    if (tbl_type == RMT_TBL_TYPE_TERN_MATCH) {
      rc = pipe_mgr_entry_format_tof_tern_tbl_ent_to_str(dev_id,
                                                         prof_id,
                                                         stage_id,
                                                         tbl_hdl,
                                                         &tbl_word0,
                                                         &tbl_word1,
                                                         str,
                                                         &c_len,
                                                         max_len,
                                                         &addr[0]);
    } else if (tbl_type == RMT_TBL_TYPE_TERN_INDIR) {
      rc = pipe_mgr_entry_format_tof_tind_tbl_ent_to_str(dev_id,
                                                         prof_id,
                                                         stage_id,
                                                         tbl_hdl,
                                                         entry_position,
                                                         &tind_tbl_word,
                                                         str,
                                                         &c_len,
                                                         max_len,
                                                         &addr[0]);
    } else if ((tbl_type == RMT_TBL_TYPE_HASH_MATCH) ||
               (tbl_type == RMT_TBL_TYPE_ATCAM_MATCH)) {
      rc = pipe_mgr_entry_format_tof_exm_tbl_ent_to_str(
          dev_id,
          prof_id,
          stage_id,
          tbl_hdl,
          stage_table_handle,
          entry_position,
          (exm_tbl_word_t *)&exm_tbl_word,
          str,
          &c_len,
          max_len,
          &addr[0],
          false);

    } else if (tbl_type == RMT_TBL_TYPE_ACTION_DATA) {
      rc = pipe_mgr_entry_format_tof_adt_tbl_ent_decode(
          dev_info,
          prof_id,
          stage_id,
          tbl_hdl,
          act_fn_hdl,
          entry_position,
          (adt_tbl_word_t *)&adt_tbl_word,
          str,
          &c_len,
          max_len,
          &addr[0]);
    }
  }

  if (c_str_len) {
    *c_str_len = c_len;
  }

  return rc;
}

pipe_status_t pipe_mgr_invalidate_tbl_idx(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          uint32_t entry_index) {
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner == PIPE_MGR_TBL_OWNER_TRN)
    return pipe_mgr_invalidate_tcam_idx(
        sess_hdl, dev_id, pipe_id, tbl_hdl, entry_index);
  if (owner == PIPE_MGR_TBL_OWNER_EXM)
    return pipe_mgr_invalidate_exm_idx(
        sess_hdl, dev_id, pipe_id, tbl_hdl, entry_index);

  LOG_ERROR("%s:%d Unsupported table type table handle 0x%x",
            __func__,
            __LINE__,
            tbl_hdl);
  return PIPE_NOT_SUPPORTED;
}
