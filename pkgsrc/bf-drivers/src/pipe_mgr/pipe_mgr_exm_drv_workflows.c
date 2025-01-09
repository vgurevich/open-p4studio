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
 * @file pipe_mgr_exm_drv_worklows.c
 * @date
 *
 * Contains workflow implementation of all the operations done on an exact
 * match table. This includes priming the instruction lists for each of the
 *operation
 * and invoking the right set of driver interface APIs to convey the
 *instructions to the
 * device.
 */

/* Standard header includes */

/* Module header includes */

#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "cuckoo_move.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_idle.h"
#include <tofino_regs/tofino.h>
#include "pipe_mgr_exm_tbl_init.h"

static rmt_virt_addr_t pipe_mgr_exm_compute_match_addr(
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    pipe_mat_ent_idx_t entry_idx) {
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  uint8_t num_entries_per_wide_word = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t ram_line_num = 0;
  uint8_t wide_word_blk_idx = 0;
  uint8_t subword = 0;
  uint8_t hashway = 0;
  uint8_t hash_way_idx = 0;
  uint32_t offset = 0;
  vpn_id_t subword_vpn = 0;

  exm_pack_format = exm_tbl_stage_info->pack_format;

  num_entries_per_wide_word = exm_pack_format->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_SRAM_UNIT_DEPTH;

  hashway = pipe_mgr_exm_get_entry_hashway(exm_tbl_stage_info, entry_idx);
  PIPE_MGR_DBGCHK(hashway < exm_tbl_stage_info->num_hash_ways);

  /* The global entry index should be converted to the entry index within
   * this hashway for further calculations.
   */

  for (hash_way_idx = 0; hash_way_idx < hashway; hash_way_idx++) {
    exm_hashway_data = &exm_tbl_stage_info->hashway_data[hash_way_idx];
    offset += exm_hashway_data->num_entries;
    entry_idx -= exm_hashway_data->num_entries;
  }

  wide_word_blk_idx = entry_idx / num_entries_per_wide_word_blk;

  ram_alloc_info = exm_tbl_stage_info->hashway_data[hashway].ram_alloc_info;

  subword = entry_idx % num_entries_per_wide_word;
  subword_vpn = ram_alloc_info->tbl_word_blk[wide_word_blk_idx].vpn_id[subword];
  ram_line_num = (entry_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  /*                     ________________________________
   * EXM match address :| subword_vpn | ram_line_number |
   *                     --------------------------------
   */
  return (subword_vpn << TOF_SRAM_NUM_RAM_LINE_BITS) | ram_line_num;
}

static inline uint32_t pipe_mgr_stash_id_to_row(uint32_t stash_id) {
  return (stash_id / 4);
}

static inline uint32_t pipe_mgr_stash_id_to_index(uint32_t stash_id) {
  return (stash_id % 4);
}

static pipe_status_t pipe_mgr_stash_match_addr_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    uint32_t stash_id,
    struct pipe_mgr_mat_data *mat_data,
    rmt_virt_addr_t stash_entry_addr) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t reg_addr = 0, reg_data = 0, num_hashes = 0;
  uint32_t stash_row = 0, stash_index = 0, way_mask = 0;
  uint8_t hashway = 0;
  pipe_mgr_exm_stash_entry_info_t *stash_ent_info = NULL;
  pipe_exm_hash_t full_hash[2];
  bf_dev_id_t dev_id = exm_tbl->dev_id;
  pipe_mgr_exm_hash_way_data_t *hashway_data = NULL;
  uint64_t hash_value = 0;

  LOG_TRACE(
      "%s : Setting Stash match address"
      " for table with handle %d, in stage %d, stash-id %d, dev %d",
      __func__,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_id,
      stash_id,
      dev_id);

  hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, stage_ent_idx);
  hashway_data = &exm_stage_info->hashway_data[hashway];

  status = pipe_mgr_stash_info_get(exm_stage_info, stash_id, &stash_ent_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in getting stash entry info "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  stash_row = pipe_mgr_stash_id_to_row(stash_id);
  stash_index = pipe_mgr_stash_id_to_index(stash_id);

  memset(&full_hash, 0, sizeof(full_hash));
  status = pipe_mgr_exm_hash_compute(dev_id,
                                     exm_tbl->profile_id,
                                     exm_tbl->mat_tbl_hdl,
                                     unpack_mat_ent_data_ms(mat_data),
                                     exm_stage_info->stage_id,
                                     &full_hash[0],
                                     &num_hashes);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in computing the hashes for stash exact"
        " match entry for table with handle %d, in stage %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        dev_id);
    return status;
  }
  if (hashway_data->hash_function_id == 0) {
    hash_value = full_hash[0].hash_value;
  } else {
    hash_value = full_hash[1].hash_value;
  }

  /* Program stash_match_address */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_match_address[stash_index]);
  reg_data = 0;
  setp_stash_match_address_stash_match_address(&reg_data, stash_entry_addr);
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_match_address entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  /* Program stash_hashkey_data */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_hashkey_data[stash_index]);
  way_mask = ((uint32_t)((1 << hashway_data->num_ram_line_bits) - 1));
  reg_data = 0;
  setp_stash_hashkey_data_stash_hashkey_data(
      &reg_data,
      (hash_value >> hashway_data->ram_line_start_offset) & way_mask);
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_hashkey_data entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  /* Program stash_bank_enable */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_bank_enable[stash_index]);
  way_mask = ((uint32_t)((1 << hashway_data->num_ram_select_bits) - 1));
  reg_data = 0;
  /* Bank mask is based on way */
  if (hashway_data->num_ram_select_bits == 0) {
    setp_stash_bank_enable_stash_bank_enable_bank_mask(&reg_data, 0);
    setp_stash_bank_enable_stash_bank_enable_bank_id(&reg_data, 0);
  } else {
    uint32_t bank_mask = way_mask
                         << (hashway_data->ram_select_start_offset - 40);
    setp_stash_bank_enable_stash_bank_enable_bank_mask(&reg_data, bank_mask);
    /* Bits 40 to 51 are bank-id */
    setp_stash_bank_enable_stash_bank_enable_bank_id(
        &reg_data, (hash_value >> 40) & bank_mask);
  }
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_bank_data entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

#if 0
  /* Program stash_match_mask only if using DKM */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_match_mask[stash_index / 2]);
  reg_data = 0;
  setp_stash_match_mask_stash_match_mask(&reg_data, 0);
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_match_mask entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }
#endif

  /* Program stash_next_table */
  uint32_t next_table = 0;
  status = pipe_mgr_entry_format_tof_exm_get_next_tbl(
      dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_table_handle,
      unpack_mat_ent_data_afun_hdl(mat_data),
      &next_table);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Failed to get next table for stash "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  reg_addr = offsetof(
      Tofino,
      pipes[0]
          .mau[exm_stage_info->stage_id]
          .rams.match.merge.stash_next_table_lut[stash_index / 2][stash_row]);
  reg_data = 0;
  setp_stash_next_table_lut_stash_next_table_lut(
      &reg_data, next_table << ((stash_index % 2) * 8));
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_next_table entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  return status;
}

static pipe_status_t pipe_mgr_stash_match_control_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    uint32_t stash_id) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t reg_addr = 0, reg_data = 0;
  uint32_t stash_row = 0, stash_index = 0;
  uint8_t hashway = 0, hashway_within_function_id = 0;
  pipe_mgr_exm_stash_entry_info_t *stash_ent_info = NULL;
  bf_dev_id_t dev_id = exm_tbl->dev_id;

  LOG_TRACE(
      "%s : Setting Stash match control"
      " for table with handle %d, in stage %d, stash-id %d, dev %d",
      __func__,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_id,
      stash_id,
      dev_id);

  hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, stage_ent_idx);
  hashway_within_function_id =
      exm_stage_info->hashway_data[hashway].hashway_within_function_id;

  status = pipe_mgr_stash_info_get(exm_stage_info, stash_id, &stash_ent_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in getting stash entry info "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  stash_row = pipe_mgr_stash_id_to_row(stash_id);
  stash_index = pipe_mgr_stash_id_to_index(stash_id);

  /* Program stash_match_input_data_ctl */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_match_input_data_ctl[stash_index / 2]);

  /* Create the register value */
  reg_data = 0;
  setp_stash_match_input_data_ctl_stash_match_data_select(
      &reg_data, stash_ent_info->stash_match_data_select);
  /* The stash_match_input_data_ctl_stash_hash_adr_select is an offset to help
     pick which 10-bit slice of the Galois field matrix hash.

     codes 0-4 for hashword 0, codes 5-9 for hashword 1

     Let's say we have an exact match table with 7 ways (index 0 through 6).
     In this example, the first 4 ways are associated with hashword 0, the
     last 3 ways are associated with hashword 1.

     Hashword 0:
        hash way 0: bits [9:0]
        hash way 1: bits [19:10]
        hash way 2: bits [29:20]
        hash way 3: bits [39:30]

     Hashword 1:
        hash way 4: bits [9:0]
        hash way 5: bits [19:10]
        hash way 6: bits [29:20]

     For this example, when programming stashes for way 4 in hashword 1,
     the hashway_within_function_id should be 5.
  */

  setp_stash_match_input_data_ctl_stash_hash_adr_select(
      &reg_data, hashway_within_function_id);
  setp_stash_match_input_data_ctl_stash_hashbank_select(
      &reg_data, stash_ent_info->stash_hashbank_select);
  setp_stash_match_input_data_ctl_stash_enable(&reg_data, 1);
  setp_stash_match_input_data_ctl_stash_logical_table(
      &reg_data, exm_stage_info->stage_table_handle);
  setp_stash_match_input_data_ctl_stash_thread(&reg_data,
                                               exm_stage_info->direction);

  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_match_input_data_ctl entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  return status;
}

/* Program stash match address and control */
pipe_status_t pipe_mgr_stash_match_addr_control_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    uint32_t stash_id,
    struct pipe_mgr_mat_data *mat_data) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_stash_entry_info_t *stash_ent_info = NULL;
  uint32_t ram_stash_id = 0, i = 0;
  rmt_virt_addr_t stash_entry_addr = 0;

  uint8_t num_ram_units =
      exm_stage_info->stash->pack_format.num_rams_in_wide_word;
  /* Compute the stash entry addr */
  stash_entry_addr =
      pipe_mgr_exm_compute_match_addr(exm_stage_info, stage_ent_idx);

  /* Program address in all wide-words */
  for (i = 0; i < num_ram_units; i++) {
    status = pipe_mgr_stash_info_at_wide_word_index_get(
        exm_stage_info, stash_id, i, &stash_ent_info);
    if (status != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    ram_stash_id = stash_ent_info->stash_id;

    /* Program stash match address */
    status = pipe_mgr_stash_match_addr_program(sess_hdl,
                                               exm_tbl,
                                               exm_tbl_data,
                                               exm_stage_info,
                                               stage_ent_idx,
                                               ram_stash_id,
                                               mat_data,
                                               stash_entry_addr);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in programming stash match address "
          " for table with handle %d, in stage %d, stash-id %d, dev %d",
          __func__,
          exm_tbl->mat_tbl_hdl,
          exm_stage_info->stage_id,
          ram_stash_id,
          exm_tbl->dev_id);
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    /* Program stash match control */
    status = pipe_mgr_stash_match_control_program(sess_hdl,
                                                  exm_tbl,
                                                  exm_tbl_data,
                                                  exm_stage_info,
                                                  stage_ent_idx,
                                                  ram_stash_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in programming stash match control "
          " for table with handle %d, in stage %d, stash-id %d, dev %d",
          __func__,
          exm_tbl->mat_tbl_hdl,
          exm_stage_info->stage_id,
          ram_stash_id,
          exm_tbl->dev_id);
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  return status;
}

pipe_status_t pipe_mgr_stash_version_valid_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t stash_id,
    uint32_t version_valid_bits) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t reg_addr = 0, reg_data = 0;
  uint32_t stash_row = 0, stash_index = 0;
  bf_dev_id_t dev_id = exm_tbl->dev_id;

  LOG_TRACE(
      "%s : Setting Stash version valid to 0x%x"
      " for table with handle %d, in stage %d, stash-id %d, dev %d",
      __func__,
      version_valid_bits,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_id,
      stash_id,
      dev_id);

  stash_row = pipe_mgr_stash_id_to_row(stash_id);
  stash_index = pipe_mgr_stash_id_to_index(stash_id);

  /* Program stash_version_valid */
  reg_addr = offsetof(Tofino,
                      pipes[0]
                          .mau[exm_stage_info->stage_id]
                          .rams.array.row[stash_row]
                          .stash.stash_version_valid[stash_index]);
  reg_data = 0;
  setp_stash_version_valid_stash_version_valid(&reg_data, version_valid_bits);
  status =
      pipe_mgr_sess_pipes_ilist_add_register_write(sess_hdl,
                                                   dev_id,
                                                   &exm_tbl_data->pipe_bmp,
                                                   exm_stage_info->stage_id,
                                                   reg_addr,
                                                   reg_data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in instruction add for stash_version_valid entry "
        " for table with handle %d, in stage %d, stash-id %d, dev %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        stash_id,
        dev_id);
    return status;
  }

  return status;
}

pipe_status_t pipe_mgr_exm_entry_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    uint32_t num_ram_units,
    uint32_t version_valid_bits,
    uint8_t vv_word_index,
    bool is_stash,
    uint32_t stash_id,
    bool update) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mem_type_t pipe_mem_type;
  rmt_dev_info_t *dev_info = exm_tbl->dev_info;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mem_type = pipe_mem_type_unit_ram;
      break;









    default:
      return PIPE_UNEXPECTED;
  }
  pipe_instr_set_memdata_t instruction_word;
  unsigned i = 0;
  uint8_t **shadow_ptr_arr = exm_stage_info->shadow_ptr_arr;
  mem_id_t *mem_id_arr = exm_stage_info->mem_id_arr;
  uint8_t *wide_word_indices = exm_stage_info->wide_word_indices;
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }
  uint8_t num_entries_per_wide_word =
      exm_stage_info->pack_format->num_entries_per_wide_word;
  if (update && (exm_tbl->dev_info->dev_family == BF_DEV_FAMILY_TOFINO2 ||
                 exm_tbl->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    /* Write to the shadow register for updates */
    pipe_mem_type = pipe_mem_type_shadow_reg;
  }

  if (is_stash) {
    shadow_ptr_arr = exm_stage_info->stash->shadow_ptr_arr;
    wide_word_indices = exm_stage_info->stash->wide_word_indices;

    for (i = 0; i < num_ram_units; i++) {
      uint8_t *entry_data_p = shadow_ptr_arr[wide_word_indices[i]];
      uint32_t reg_addr = 0, reg_data = 0, byte_idx = 0, word_idx = 0;
      uint32_t ram_stash_id = 0, stash_row = 0, stash_index = 0;
      pipe_mgr_exm_stash_entry_info_t *stash_ent_info = NULL;

      /* Program the wide word stashes in reverse order */
      status = pipe_mgr_stash_info_at_wide_word_index_get(
          exm_stage_info,
          stash_id,
          wide_word_indices[num_ram_units - i - 1],
          &stash_ent_info);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s %d : Error in getting stash infos "
            " for table with handle %d, in stage %d, dev %d",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_stage_info->stage_id,
            exm_tbl->dev_id);
        PIPE_MGR_DBGCHK(0);
        return status;
      }
      ram_stash_id = stash_ent_info->stash_id;

      stash_row = pipe_mgr_stash_id_to_row(ram_stash_id);
      stash_index = pipe_mgr_stash_id_to_index(ram_stash_id);
      /* Each register stores 32 bit data */
      for (byte_idx = 0; byte_idx < (TOF_SRAM_UNIT_WIDTH / 8); byte_idx += 4) {
        word_idx = byte_idx / 4;
        reg_addr = offsetof(Tofino,
                            pipes[0]
                                .mau[exm_stage_info->stage_id]
                                .rams.array.row[stash_row]
                                .stash.stash_data[stash_index][word_idx]);
        reg_data = ((uint32_t)entry_data_p[byte_idx] << 0) |
                   ((uint32_t)entry_data_p[byte_idx + 1] << 8) |
                   ((uint32_t)entry_data_p[byte_idx + 2] << 16) |
                   ((uint32_t)entry_data_p[byte_idx + 3] << 24);
        status |= pipe_mgr_sess_pipes_ilist_add_register_write(
            sess_hdl,
            exm_tbl->dev_id,
            &exm_tbl_data->pipe_bmp,
            exm_stage_info->stage_id,
            reg_addr,
            reg_data);
        LOG_TRACE("%s : Setting stash_data: Stash-id %d, Data[%d] = 0x%x ",
                  __func__,
                  ram_stash_id,
                  word_idx,
                  reg_data);
      }
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in instruction add for stash_data entry "
            " for table with handle %d, in stage %d, stash-id %d, dev %d",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_stage_info->stage_id,
            ram_stash_id,
            exm_tbl->dev_id);
        return status;
      } else {
        LOG_DBG(
            "Writing 128 bits of data for stash_data entry "
            "with handle %d at stash-id %d, stash-row %d, stash-index %d "
            "stage id %d, pipe_id %x, dev %d",
            exm_tbl->mat_tbl_hdl,
            ram_stash_id,
            stash_row,
            stash_index,
            exm_stage_info->stage_id,
            exm_tbl_data->pipe_id,
            exm_tbl->dev_id);
      }
    }

    /* Program version_valid in stash. For stash entries, the version valid is
       not in the data, it is in a separate register. Always program version
       valids at the end.
     */
    for (i = 0; i < num_ram_units; i++) {
      pipe_mgr_exm_stash_entry_info_t *stash_ent_info = NULL;
      uint32_t ram_stash_id = 0;

      status = pipe_mgr_stash_info_at_wide_word_index_get(
          exm_stage_info, stash_id, wide_word_indices[i], &stash_ent_info);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s %d : Error in getting stash infos "
            " for table with handle %d, in stage %d, dev %d",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_stage_info->stage_id,
            exm_tbl->dev_id);
        PIPE_MGR_DBGCHK(0);
        return status;
      }
      ram_stash_id = stash_ent_info->stash_id;
      status = pipe_mgr_stash_version_valid_program(sess_hdl,
                                                    exm_tbl,
                                                    exm_tbl_data,
                                                    exm_stage_info,
                                                    ram_stash_id,
                                                    version_valid_bits);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in programming stash version valid "
            " for table with handle %d, in stage %d, stash-id %d, dev %d",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_stage_info->stage_id,
            ram_stash_id,
            exm_tbl->dev_id);
        PIPE_MGR_DBGCHK(0);
        return status;
      }
    }
  } else {
    uint8_t *data_ptrs[num_ram_units];
    int mem_ids[num_ram_units];
    unsigned to_write = 0;
    for (i = 0; i < num_ram_units; i++) {
      if (wide_word_indices[i] == vv_word_index) continue;
      data_ptrs[to_write] = shadow_ptr_arr[wide_word_indices[i]];
      mem_ids[to_write] = mem_id_arr[wide_word_indices[i]];
      ++to_write;
    }
    data_ptrs[to_write] = shadow_ptr_arr[vv_word_index];
    mem_ids[to_write] = mem_id_arr[vv_word_index];
    ++to_write;

    for (i = 0; i < to_write; i++) {
      uint32_t ram_line = (stage_ent_idx / num_entries_per_wide_word) %
                          TOF_UNIT_RAM_DEPTH(exm_tbl);
      construct_instr_set_memdata(exm_tbl->dev_info,
                                  &instruction_word,
                                  data_ptrs[i],
                                  TOF_SRAM_UNIT_WIDTH / 8,
                                  mem_ids[i],
                                  exm_tbl->direction,
                                  exm_stage_info->stage_id,
                                  ram_line,
                                  pipe_mem_type);
      status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                      exm_tbl->dev_info,
                                      &exm_tbl_data->pipe_bmp,
                                      exm_stage_info->stage_id,
                                      (uint8_t *)&instruction_word,
                                      sizeof(pipe_instr_set_memdata_t));

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in instruction add for exact match table 0x%x, in "
            "stage %d, pipe %x, dev %d",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_stage_info->stage_id,
            exm_tbl_data->pipe_id,
            exm_tbl->dev_id);
        return status;
      } else {
        LOG_DBG(
            "Writing data for exact match table 0x%x at ram line num %d, ram "
            "id %d stage id %d, pipe_id %x, dev %d",
            exm_tbl->mat_tbl_hdl,
            ram_line,
            mem_ids[i],
            exm_stage_info->stage_id,
            exm_tbl_data->pipe_id,
            exm_tbl->dev_id);
      }
    }
    if (!version_valid_bits) {
      /* This is an entry delete.  Send 12 NOP instructions to allow this MAU
       * stage's lookup pipeline to empty out.  If we are batching and a new
       * entry add for this location immediately follows this delete it can
       * cause an incorrect match as a packet could have matched the entry just
       * deleted here, but then pick up direct action data for the new entry
       * which is being added. */
      pipe_noop_instr_t nop;
      construct_instr_noop(exm_tbl->dev_id, &nop);
      for (i = 0; i < 12; ++i) {
        status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                        exm_tbl->dev_info,
                                        &exm_tbl_data->pipe_bmp,
                                        exm_stage_info->stage_id,
                                        (uint8_t *)&nop,
                                        sizeof nop);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s : Error in nop-instruction add for exact match table 0x%x, "
              "in stage %d, pipe %x, dev %d",
              __func__,
              exm_tbl->mat_tbl_hdl,
              exm_stage_info->stage_id,
              exm_tbl_data->pipe_id,
              exm_tbl->dev_id);
          return status;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_program_default_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_register_spec_t reg_spec_list[exm_tbl->dev_info->dev_cfg.num_dflt_reg];
  pipe_action_data_spec_t act_data_spec;
  uint32_t i = 0;
  if (act_spec == NULL) {
    /* Need to create a dummy action data spec with ZERO. This may be used by
     * the default entry encoder to form the immediate data, if any.
     */
    PIPE_MGR_MEMSET(&act_data_spec, 0, sizeof(pipe_action_data_spec_t));
    uint8_t action_data_bits[TOF_MAXIMUM_ACTION_DATA_BYTES];
    PIPE_MGR_MEMSET(action_data_bits, 0, TOF_MAXIMUM_ACTION_DATA_BYTES);
    act_data_spec.action_data_bits = action_data_bits;
  } else {
    act_data_spec = *act_spec;
  }

  uint32_t idle_time_ptr = 0;
  uint32_t stats_ptr = indirect_ptrs->stats_ptr;

  if (exm_tbl->meter_tbl_refs) {
    if (exm_tbl->meter_tbl_refs[0].color_mapram_addr_type ==
        COLOR_MAPRAM_ADDR_TYPE_IDLE) {
      // Compute color mapram address and place it in idle time address
      idle_time_ptr = indirect_ptrs->meter_ptr >> 3;
    } else if (exm_tbl->meter_tbl_refs[0].color_mapram_addr_type ==
               COLOR_MAPRAM_ADDR_TYPE_STATS) {
      stats_ptr = indirect_ptrs->meter_ptr >> 4;
    }
  }
  status =
      pipe_mgr_entry_format_tbl_default_entry_update(exm_tbl->dev_info,
                                                     exm_tbl->direction,
                                                     exm_tbl->profile_id,
                                                     exm_tbl->mat_tbl_hdl,
                                                     act_fn_hdl,
                                                     &act_data_spec,
                                                     indirect_ptrs->adt_ptr,
                                                     stats_ptr,
                                                     indirect_ptrs->meter_ptr,
                                                     indirect_ptrs->stfl_ptr,
                                                     idle_time_ptr,
                                                     indirect_ptrs->sel_ptr,
                                                     indirect_ptrs->sel_len,
                                                     reg_spec_list);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in encoding the default entry for exact match "
        "table with handle 0x%x, error %s",
        __func__,
        exm_tbl->mat_tbl_hdl,
        pipe_str_err(status));
    return status;
  }
  for (i = 0; i < exm_tbl->dev_info->dev_cfg.num_dflt_reg; i++) {
    pipe_instr_write_reg_t instr;
    LOG_TRACE(
        "%s : Exact match table with handle 0x%x, default register addr "
        "0x%x value 0x%x",
        __func__,
        exm_tbl->mat_tbl_hdl,
        reg_spec_list[i].reg_addr & 0x7ffff,
        reg_spec_list[i].reg_data);

    construct_instr_reg_write(exm_tbl->dev_id,
                              &instr,
                              reg_spec_list[i].reg_addr,
                              reg_spec_list[i].reg_data);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    exm_tbl->dev_info,
                                    &exm_tbl_data->pipe_bmp,
                                    exm_stage_info->stage_id,
                                    (uint8_t *)&instr,
                                    sizeof(pipe_instr_write_reg_t));

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Instruction list add failed for exact match default"
          " entry add for table with handle %d, error %s",
          __func__,
          exm_tbl->mat_tbl_hdl,
          pipe_str_err(status));
      return status;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_default_entry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_action_data_spec_t *act_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_register_spec_t reg_spec_list[exm_tbl->dev_info->dev_cfg.num_dflt_reg];
  uint32_t indirect_idle_ptr;
  bf_dev_pipe_t phy_pipe;
  bf_dev_pipe_t pipe_id = 0;
  bf_dev_pipe_t default_pipe_id = 0;
  (void)exm_tbl_data;
  (void)exm_stage_info;

  if (exm_tbl->symmetric)
    default_pipe_id = exm_tbl->lowest_pipe_id;
  else
    default_pipe_id = exm_tbl_data->pipe_id;

  status = pipe_mgr_get_pipe_id(
      &exm_tbl_data->pipe_bmp, dev_tgt.dev_pipe_id, default_pipe_id, &pipe_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Invalid request to access pipe %x for table %s "
        "0x%x device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id);
    return status;
  }

  status =
      pipe_mgr_map_pipe_id_log_to_phy(exm_tbl->dev_info, pipe_id, &phy_pipe);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              dev_tgt.device_id,
              pipe_str_err(status));
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

  status = pipe_mgr_entry_format_tbl_default_entry_prepare(exm_tbl->dev_info,
                                                           exm_tbl->direction,
                                                           exm_tbl->profile_id,
                                                           exm_tbl->mat_tbl_hdl,
                                                           pipe_id,
                                                           reg_spec_list);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in decoding the default entry for exact match "
        "table with handle 0x%x, error %s",
        __func__,
        exm_tbl->mat_tbl_hdl,
        pipe_str_err(status));
    return status;
  }
  for (uint32_t i = 0; i < exm_tbl->dev_info->dev_cfg.num_dflt_reg; i++) {
    status = pipe_mgr_drv_subdev_reg_rd(&sess_hdl,
                                        exm_tbl->dev_info->dev_id,
                                        subdev,
                                        reg_spec_list[i].reg_addr,
                                        &reg_spec_list[i].reg_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s : Reg read failed for default register addr 0x%x, error %s",
                __func__,
                reg_spec_list[i].reg_addr,
                pipe_str_err(status));
      return status;
    }
    LOG_TRACE(
        "%s : Exact match table with handle 0x%x, default register addr "
        "0x%x value 0x%x",
        __func__,
        exm_tbl->mat_tbl_hdl,
        reg_spec_list[i].reg_addr & 0x7ffff,
        reg_spec_list[i].reg_data);
  }
  status =
      pipe_mgr_entry_format_tbl_default_entry_get(exm_tbl->dev_info,
                                                  exm_tbl->profile_id,
                                                  exm_tbl->mat_tbl_hdl,
                                                  act_fn_hdl,
                                                  reg_spec_list,
                                                  act_spec,
                                                  &indirect_ptrs->adt_ptr,
                                                  &indirect_ptrs->stats_ptr,
                                                  &indirect_ptrs->meter_ptr,
                                                  &indirect_ptrs->stfl_ptr,
                                                  &indirect_idle_ptr,
                                                  &indirect_ptrs->sel_ptr,
                                                  &indirect_ptrs->sel_len);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in decoding the default entry for exact match "
        "table with handle 0x%x, error %s",
        __func__,
        exm_tbl->mat_tbl_hdl,
        pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_issue_push_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx) {
  pipe_status_t status = PIPE_SUCCESS;







  pipe_push_table_move_adr_instr_t push_move_adr_instr;
  rmt_virt_addr_t entry_addr =
      pipe_mgr_exm_compute_match_addr(exm_stage_info, entry_idx);
  construct_instr_move_reg_push(exm_tbl->dev_id,
                                &push_move_adr_instr,
                                exm_stage_info->stage_table_handle,
                                entry_addr);
  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  exm_tbl->dev_info,
                                  &exm_tbl_data->pipe_bmp,
                                  exm_stage_info->stage_id,
                                  (uint8_t *)&push_move_adr_instr,
                                  sizeof(pipe_push_table_move_adr_instr_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in adding push move adr instruction, error %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_issue_pop_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t ttl) {
  pipe_status_t status = PIPE_SUCCESS;







  pipe_pop_table_move_adr_instr_t pop_move_adr_instr;
  construct_instr_move_reg_pop(
      exm_tbl->dev_id,
      &pop_move_adr_instr,
      exm_stage_info->stage_table_handle,
      false,
      (ttl == 0) ? exm_tbl->idle_init_val_for_ttl_0 : 0);
  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  exm_tbl->dev_info,
                                  &exm_tbl_data->pipe_bmp,
                                  exm_stage_info->stage_id,
                                  (uint8_t *)&pop_move_adr_instr,
                                  sizeof(pipe_pop_table_move_adr_instr_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in adding push move adr instruction, erro %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_issue_lock_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    lock_id_t lock_id,
    bool *stats_locked_p,
    bool *idle_locked_p) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_barrier_lock_instr_t lock_instr;
  bool lock_idle = false;
  bool lock_stats = false;
  if (exm_tbl->lock_type == LOCK_ID_TYPE_INVALID) {
    return PIPE_SUCCESS;
  }
  switch (exm_tbl->lock_type) {
    case LOCK_ID_TYPE_ALL_LOCK:
      lock_idle = true;
      lock_stats = true;
      construct_instr_lock_all(exm_tbl->dev_id,
                               &lock_instr,
                               lock_id,
                               exm_stage_info->stage_table_handle);
      break;
    case LOCK_ID_TYPE_IDLE_LOCK:
      lock_idle = true;
      construct_instr_lock_idle(exm_tbl->dev_id,
                                &lock_instr,
                                lock_id,
                                exm_stage_info->stage_table_handle);
      break;
    case LOCK_ID_TYPE_STAT_LOCK:
      lock_stats = true;
      construct_instr_lock_stats(exm_tbl->dev_id,
                                 &lock_instr,
                                 lock_id,
                                 exm_stage_info->stage_table_handle);
      break;
    default:
      LOG_ERROR(
          "%s:%d Invalid lock type"
          " for tbl %d device id %d pipe id %d stage id %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id);
      PIPE_MGR_DBGCHK(0);
      return (PIPE_INVALID_ARG);
  }
  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  exm_tbl->dev_info,
                                  &exm_tbl_data->pipe_bmp,
                                  exm_stage_info->stage_id,
                                  (uint8_t *)&lock_instr,
                                  sizeof(pipe_barrier_lock_instr_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in adding lock instruction to ilist"
        " for tbl %d device id %d pipe id %d stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id);
    return status;
  }
  *stats_locked_p = lock_stats;
  *idle_locked_p = lock_idle;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_issue_unlock_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    lock_id_t lock_id,
    bool *stats_unlocked_p,
    bool *idle_unlocked_p) {
  if (exm_tbl->lock_type == LOCK_ID_TYPE_INVALID) {
    return PIPE_SUCCESS;
  }
  pipe_status_t status = PIPE_SUCCESS;
  pipe_barrier_lock_instr_t lock_instr;
  bool unlock_idle = false;
  bool unlock_stats = false;
  switch (exm_tbl->lock_type) {
    case LOCK_ID_TYPE_ALL_LOCK:
      unlock_idle = true;
      unlock_stats = true;
      lock_id = exm_stage_info->idle_tbl_lock_id;
      PIPE_MGR_DBGCHK(exm_stage_info->stats_tbl_lock_id == lock_id);
      construct_instr_unlock_all(exm_tbl->dev_id,
                                 &lock_instr,
                                 lock_id,
                                 exm_stage_info->stage_table_handle);
      break;
    case LOCK_ID_TYPE_IDLE_LOCK:
      unlock_idle = true;
      lock_id = exm_stage_info->idle_tbl_lock_id;
      construct_instr_unlock_idle(exm_tbl->dev_id,
                                  &lock_instr,
                                  lock_id,
                                  exm_stage_info->stage_table_handle);
      break;
    case LOCK_ID_TYPE_STAT_LOCK:
      unlock_stats = true;
      lock_id = exm_stage_info->stats_tbl_lock_id;
      construct_instr_unlock_stats(exm_tbl->dev_id,
                                   &lock_instr,
                                   lock_id,
                                   exm_stage_info->stage_table_handle);
      break;
    default:
      LOG_ERROR(
          "%s:%d Invalid lock type"
          " for tbl %d device id %d pipe id %d stage id %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id);
      PIPE_MGR_DBGCHK(0);
      return (PIPE_INVALID_ARG);
  }
  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  exm_tbl->dev_info,
                                  &exm_tbl_data->pipe_bmp,
                                  exm_stage_info->stage_id,
                                  (uint8_t *)&lock_instr,
                                  sizeof(pipe_barrier_lock_instr_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in adding lock instruction to ilist"
        " for tbl %d device id %d pipe id %d stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id);
    return status;
  }
  *idle_unlocked_p = unlock_idle;
  *stats_unlocked_p = unlock_stats;
  return PIPE_SUCCESS;
}
