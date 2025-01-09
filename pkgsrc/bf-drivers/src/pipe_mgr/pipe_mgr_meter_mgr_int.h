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
 * @file pipe_mgr_meter_mgr_int.h
 * @date
 *
 * Definitions internal to meter table manager
 */

#ifndef _PIPE_MGR_METER_MGR_INT_H
#define _PIPE_MGR_METER_MGR_INT_H

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr/pipe_mgr_intf.h"
#include <target-utils/map/map.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_tbl.h"

/* Structure definition for meter ram allocation info */
typedef struct pipe_mgr_meter_ram_alloc_info_ {
  /* Number of wide-word blocks. For meters each wide-word block
   * actually a single RAM unit.
   */
  uint8_t num_wide_word_blks;
  /* Pointer to an array of tbl_word_blks */
  rmt_tbl_word_blk_t *tbl_word_blk;
  /* Number of wide-word blocks allocated for holding meter color */
  uint8_t num_color_wide_word_blks;
  rmt_tbl_word_blk_t *color_tbl_word_blk;
} pipe_mgr_meter_ram_alloc_info_t;

/* Structure definition for meter table stage info */
typedef struct pipe_mgr_meter_tbl_stage_info_ {
  /* Stage id */
  uint8_t stage_id;
  /* Number of ALU_IDS */
  uint8_t num_alu_ids;
  /* ALU Ids */
  uint8_t alu_ids[RMT_MAX_S2P_SECTIONS];
  /* Number of entries */
  uint32_t num_entries;
  /* Offset to be added to an index in this stage to get the global
   * meter index.
   */
  uint32_t ent_idx_offset;
  /* Number of entries occupied */
  uint32_t num_entries_occupied;
  /* RAM allocation info */
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info;
  /* Stage table handle (A.K.A. Logical Table Id) */
  uint8_t stage_table_handle;
  /* Number of default lower huffman bits provided by the compiler */
  uint8_t default_lower_huffman_bits;
  /* Number of spare syn2port RAMs. */
  uint8_t num_spare_rams;
  /* Spare syn2port RAMs */
  mem_id_t spare_rams[RMT_MAX_S2P_SECTIONS];
} pipe_mgr_meter_tbl_stage_info_t;

/* Structure definition for meter table instance */
typedef struct pipe_mgr_meter_tbl_instance_ {
  /* Pipe id of this instance. PIPE_ID_ALL if all pipes */
  bf_dev_pipe_t pipe_id;
  /* Number of entries in this instance */
  uint32_t num_entries;
  /* Number of entries occupied */
  uint32_t num_entries_occupied;
  /* Number of stages in which this instance is present */
  uint8_t num_stages;
  /* Array of stage level info */
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info;
  /* A bitmap representing the pipe_id(s) this managed instance belongs to */
  pipe_bitmap_t pipe_bmp;
  /* A map of index-to-spec holding the replayed specs during hitless HA. */
  bf_map_t replayed; /* LLP state. */
  /* Adjust byte count for this table instance */
  int adj_byt_cnt;
} pipe_mgr_meter_tbl_instance_t;

/* Defines for Meter MSB address type encodings */
#define TOF_METER_ADDR_TYPE_METER_TYPE_UNUSED 0
#define TOF_METER_ADDR_TYPE_METER_TYPE_LPF_COLOR_BLIND 2
#define TOF_METER_ADDR_TYPE_METER_TYPE_COLOR_AWARE 6

/* Structure definition for meter table */
typedef struct pipe_mgr_meter_tbl_ {
  /* Pointer to the device info struct */
  rmt_dev_info_t *dev_info;
  /* Meter table handle */
  pipe_meter_tbl_hdl_t meter_tbl_hdl;
  /* Meter type : Color, LPF or RED */
  pipe_meter_impl_type_e type;
  /* Meter granularity for color-based meters ONLY */
  pipe_meter_granularity_e meter_granularity;
  /* Reference type of the meter */
  pipe_tbl_ref_type_t ref_type;
  /* A flag indicating whether the table is symmetric or not */
  bool symmetric;
  /* Scope info */
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  /* Name of the table */
  char *name;
  /* Profile id with which the table is associated with */
  profile_id_t profile_id;
  /* Number of entries */
  uint32_t num_entries;
  /* Number of managed instances of the table. If symmetric, 1, else
   * as many as the number of pipes in which the table exists.
   */
  uint8_t num_tbl_instances;
  /* Array of managed instances */
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instances;
  /* A flag indicating if the meter is color-aware capable or not */
  bool enable_color_aware;
  /* A flag indiciating if per-flow is enabled on the meter table */
  bool enable_per_flow_enable;
  /* Bit position of the per-flow enable bit in the meter address */
  uint32_t per_flow_enable_bit_position;
  /* A flag indicating if the per-flow enable is there for color-aware or not */
  bool enable_color_aware_per_flow_enable;
  /* Bit position of the color-aware bit in the meter address. Valid only if
   * the above flag is valid.
   */
  uint32_t color_aware_per_flow_enable_address_type_bit_position;
  /* A flag indicating if the meter table is over allocated (replicated in
   * every stage where the match table is present.
   */
  bool over_allocated;
  /* Pipe-id of the lowest logical pipe in which the table is present.
   * Only referred to when the meter table is symmetric.
   */
  bf_dev_pipe_t lowest_pipe_id;
  pipe_tbl_dir_t direction;

} pipe_mgr_meter_tbl_t;

pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get(
    bf_dev_id_t device_id, pipe_meter_tbl_hdl_t meter_tbl_hdl);
pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get_first(bf_dev_id_t device_id);
pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get_next(pipe_mgr_meter_tbl_t *tbl);

pipe_mgr_meter_tbl_instance_t *pipe_mgr_meter_tbl_get_instance(
    pipe_mgr_meter_tbl_t *meter_tbl, bf_dev_pipe_t pipe_id);

pipe_mgr_meter_tbl_stage_info_t *pipe_mgr_meter_tbl_get_stage_info(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance, uint8_t stage_id);

uint8_t pipe_mgr_meter_mgr_get_stage(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_meter_idx_t meter_idx);

pipe_status_t pipe_mgr_meter_mgr_stage_add_entry(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    uint8_t stage_id,
    pipe_meter_idx_t meter_idx,
    rmt_virt_addr_t *ent_virt_addr);

pipe_status_t pipe_mgr_meter_mgr_lpf_stage_add_entry(
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    uint8_t stage_id,
    pipe_lpf_idx_t lpf_idx,
    rmt_virt_addr_t *ent_virt_addr);

pipe_status_t pipe_mgr_meter_mgr_wred_stage_add_entry(
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    uint8_t stage_id,
    pipe_wred_idx_t wred_idx,
    rmt_virt_addr_t *ent_virt_addr);

rmt_virt_addr_t pipe_mgr_meter_mgr_construct_virt_addr(
    pipe_mgr_meter_tbl_t *meter_tbl,
    vpn_id_t vpn,
    uint32_t ram_line_num,
    bool color_aware);

pipe_status_t pipe_mgr_meter_mgr_get_color_aware(
    pipe_mgr_meter_tbl_t *meter_tbl,
    bf_dev_pipe_t pipe_id,
    pipe_meter_idx_t meter_idx,
    bool *color_aware);

#endif  // _PIPE_MGR_METER_MGR_INT_H
