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
 * @file pipe_mgr_adt_tofino.c
 * @date
 *
 * Contains Tofino specific services required by the platform-independent part
 * of the action data table management.
 *
 */

/* Standard header includes */
#include <math.h>
#include <sys/types.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_adt_tofino.h"

static inline void tof_adt_vaddr_build(uint32_t ram_line_num,
                                       vpn_id_t ram_vpn,
                                       uint8_t subword_idx,
                                       uint16_t adt_entry_width,
                                       rmt_virt_addr_t *adt_virt_addr_p) {
  uint32_t num_vpn_bits = 0;
  uint32_t num_subword_bits = 0;
  uint32_t subword_huffman_encoding = 0;
  uint32_t num_subword_huffman_bits = 0;

  uint32_t num_ram_line_bits = 0;
  uint32_t vpn_shift = 0;
  uint32_t ram_line_shift = 0;
  uint32_t subword_shift = 0;

  num_ram_line_bits = log2(TOF_SRAM_UNIT_DEPTH);
  num_vpn_bits = 7;

  if (adt_entry_width <= TOF_SRAM_UNIT_WIDTH / 8) {
    /* Less than or equal to 128 bits in Tofino land */
    switch (adt_entry_width) {
      case 0:
        break;
      case 1:
        num_subword_bits = 4;
        num_subword_huffman_bits = 1;
        subword_huffman_encoding = 0;
        break;
      case 2:
        num_subword_bits = 3;
        num_subword_huffman_bits = 2;
        subword_huffman_encoding = 1;
        break;
      case 4:
        num_subword_bits = 2;
        num_subword_huffman_bits = 3;
        subword_huffman_encoding = 3;
        break;
      case 8:
        num_subword_bits = 1;
        num_subword_huffman_bits = 4;
        subword_huffman_encoding = 7;
        break;
      case 16:
        num_subword_bits = 0;
        num_subword_huffman_bits = 5;
        subword_huffman_encoding = 15;
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        break;
    }
  } else if (adt_entry_width == 2 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 256 bits in
     * Tofino land
     */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
    subword_huffman_encoding = 31;
  } else if (adt_entry_width == 3 * TOF_SRAM_UNIT_WIDTH / 8 ||
             adt_entry_width == 4 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 512 bits in Tofino land.  A width of 384 bits or 512 bits
     * should be valid, as if the entry is only 3 RAMs wide, a 512 entry
     * address just will not be consumed by a 4th word.
     */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
    subword_huffman_encoding = 31;
  } else if (adt_entry_width == 5 * TOF_SRAM_UNIT_WIDTH / 8 ||
             adt_entry_width == 6 * TOF_SRAM_UNIT_WIDTH / 8 ||
             adt_entry_width == 7 * TOF_SRAM_UNIT_WIDTH / 8 ||
             adt_entry_width == 8 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 1024 bits (which is the max) in Tofino land.  A width of 640,
     * 768, 996 and 1024 is valid, as missing words for the address will
     * simply not be consumed
     */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
    subword_huffman_encoding = 31;
  } else {
    PIPE_MGR_DBGCHK(0);
  }

  vpn_shift = num_subword_huffman_bits + num_subword_bits + num_ram_line_bits;
  ram_line_shift = vpn_shift - num_ram_line_bits;
  subword_shift = ram_line_shift - num_subword_bits;

  /***************************************************************************
   * PFE | VPN  | VPN HUFFMAN | RAM LINE NUM |NUM SUB-WORDS |SUBWORD HUFFMAN |
   **************************************************************************/
  *adt_virt_addr_p =
      ((ram_vpn & ((1 << num_vpn_bits) - 1)) << vpn_shift) |
      (ram_line_num << ram_line_shift) |
      ((subword_idx & ((1 << num_subword_bits) - 1)) << subword_shift) |
      (1 << 22) | /* Include PFE bit. */
      subword_huffman_encoding;

  return;
}

pipe_status_t pipe_adt_tof_generate_vaddr(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t adt_ent_idx,
    rmt_virt_addr_t *adt_virt_addr_p) {
  pipe_mgr_adt_ram_alloc_info_t *adt_ram_alloc_info = NULL;
  uint32_t adt_entry_width = 0;
  uint32_t num_entries_per_wide_word = 0;
  uint32_t num_rams_in_wide_word = 0;
  uint32_t ram_line_num = 0;
  uint32_t wide_word_blk_idx = 0;
  uint8_t subword_idx = 0;
  vpn_id_t ram_vpn = 0;

  /* Get the action data table info from the rmt cfg db */
  adt_ram_alloc_info = adt_stage_info->ram_alloc_info;

  /* Locate the RAM which houses the allocated entry index for the new
   * entry.
   */
  num_entries_per_wide_word = adt_ram_alloc_info->num_entries_per_wide_word;
  num_rams_in_wide_word = adt_ram_alloc_info->num_rams_in_wide_word;

  ram_line_num =
      ((adt_ent_idx) / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  subword_idx = ((adt_ent_idx) % num_entries_per_wide_word);

  wide_word_blk_idx =
      ((adt_ent_idx) / num_entries_per_wide_word) / TOF_SRAM_UNIT_DEPTH;

  /* VPN is always that of the first RAM unit in the wide-word */
  ram_vpn = (adt_ram_alloc_info->tbl_word_blk[wide_word_blk_idx].vpn_id[0]);

  if (num_rams_in_wide_word == 1) {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH / num_entries_per_wide_word) / 8;
  } else {
    adt_entry_width = (TOF_SRAM_UNIT_WIDTH * num_rams_in_wide_word) / 8;
  }
  /* Now that we have the vpn, and the line number, all in readiness to
   * for the virtual address of this action data table entry.
   */
  tof_adt_vaddr_build(
      ram_line_num, ram_vpn, subword_idx, adt_entry_width, adt_virt_addr_p);

  return PIPE_SUCCESS;
}

void pipe_mgr_adt_tof_unbuild_virt_addr(rmt_virt_addr_t virt_addr,
                                        uint32_t adt_entry_width,
                                        uint32_t *ram_line,
                                        vpn_id_t *vpn,
                                        uint32_t *entry_position) {
  uint32_t num_vpn_bits = 0;
  uint32_t num_subword_bits = 0;
  uint32_t num_subword_huffman_bits = 0;
  uint32_t num_ram_line_bits = 0;
  uint32_t vpn_shift = 0;
  uint32_t ram_line_shift = 0;
  uint32_t subword_shift = 0;

  num_ram_line_bits = TOF_SRAM_NUM_RAM_LINE_BITS;
  num_vpn_bits = 7;

  if (adt_entry_width <= TOF_SRAM_UNIT_WIDTH / 8) {
    /* Less than or equal to 128 bits in Tofino land */
    switch (adt_entry_width) {
      case 0:
        break;
      case 1:
        num_subword_bits = 4;
        num_subword_huffman_bits = 1;
        break;
      case 2:
        num_subword_bits = 3;
        num_subword_huffman_bits = 2;
        break;
      case 4:
        num_subword_bits = 2;
        num_subword_huffman_bits = 3;
        break;
      case 8:
        num_subword_bits = 1;
        num_subword_huffman_bits = 4;
        break;
      case 16:
        num_subword_bits = 0;
        num_subword_huffman_bits = 5;
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        break;
    }
  } else if (adt_entry_width == 2 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 256 bits in
     * Tofino land
     */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
  } else if (adt_entry_width == 3 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 512 bits in Tofino land */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
  } else if (adt_entry_width == 4 * TOF_SRAM_UNIT_WIDTH / 8) {
    /* Equal to 1024 bits (which is the max) in Tofino land */
    num_subword_bits = 0;
    num_subword_huffman_bits = 5;
  } else {
    PIPE_MGR_DBGCHK(0);
  }
  vpn_shift = num_subword_huffman_bits + num_subword_bits + num_ram_line_bits;
  ram_line_shift = vpn_shift - num_ram_line_bits;
  subword_shift = ram_line_shift - num_subword_bits;

  *vpn = ((virt_addr >> vpn_shift) & ((1 << num_vpn_bits) - 1));
  *ram_line =
      ((virt_addr >> ram_line_shift) & ((1 << TOF_SRAM_NUM_RAM_LINE_BITS) - 1));
  *entry_position =
      ((virt_addr >> subword_shift) & ((1 << num_subword_bits) - 1));
  return;
}

pipe_status_t pipe_mgr_adt_tof_encode_entry(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_adt_ent_idx_t adt_ent_idx,
    uint8_t **shadow_ptr_arr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint8_t entry_position = 0;
  ram_alloc_info = adt_stage_info->ram_alloc_info;

  entry_position = adt_ent_idx % (ram_alloc_info->num_entries_per_wide_word);

  /* Zero out the action entry before encoding */
  uint32_t entry_len =
      (TOF_SRAM_UNIT_WIDTH / 8) / ram_alloc_info->num_entries_per_wide_word;
  uint32_t offset = entry_len * entry_position;
  for (uint32_t i = 0; i < ram_alloc_info->num_rams_in_wide_word; i++) {
    PIPE_MGR_MEMSET(shadow_ptr_arr[i] + offset, 0, entry_len);
  }

  /* Invoke the compiler-provided encoder routine */
  status =
      pipe_mgr_entry_format_tof_adt_tbl_ent_update(adt->dev_info,
                                                   adt->profile_id,
                                                   adt_stage_info->stage_id,
                                                   adt->adt_tbl_hdl,
                                                   act_fn_hdl,
                                                   act_data_spec,
                                                   entry_position,
                                                   shadow_ptr_arr);

  return status;
}
