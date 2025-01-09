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
 * @file pipe_mgr_tind.c
 * @date
 *
 * Implementation of Ternary Indirection Table Management
 */

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_tcam.h"

pipe_status_t pipe_mgr_tcam_tind_tbl_alloc(tcam_stage_info_t *stage_data,
                                           pipe_mat_tbl_info_t *mat_tbl_info) {
  uint32_t total_tind_lines = 0;
  rmt_tbl_info_t *rmt_info = NULL;
  uint32_t entries_per_mem_word = 0;
  uint32_t rmt_stage_idx = 0;

  for (rmt_stage_idx = 0; rmt_stage_idx < mat_tbl_info->num_rmt_info;
       rmt_stage_idx++) {
    rmt_info = &mat_tbl_info->rmt_info[rmt_stage_idx];
    if ((rmt_info->type == RMT_TBL_TYPE_TERN_INDIR) &&
        (rmt_info->stage_id == stage_data->stage_id) &&
        (rmt_info->handle == stage_data->stage_table_handle)) {
      break;
    }
  }

  if (rmt_stage_idx == mat_tbl_info->num_rmt_info) {
    /* Ternary indirection not used. Likely a 1 bit result */
    stage_data->total_tind_lines = 0;
    return PIPE_SUCCESS;
  }

  entries_per_mem_word = rmt_info->pack_format.entries_per_tbl_word;
  total_tind_lines = ((rmt_info->num_entries - 1) / entries_per_mem_word) + 1;

  PIPE_MGR_DBGCHK(total_tind_lines != 0);

  stage_data->tind_pack_format = rmt_info->pack_format;
  stage_data->total_tind_lines = total_tind_lines;
  stage_data->tind_num_blks = rmt_info->bank_map->num_tbl_word_blks;

  stage_data->tind_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_MALLOC(
      sizeof(rmt_tbl_word_blk_t) * stage_data->tind_num_blks);
  if (stage_data->tind_blk == NULL) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(stage_data->tind_blk,
                  rmt_info->bank_map->tbl_word_blk,
                  sizeof(rmt_tbl_word_blk_t) * stage_data->tind_num_blks);

  return PIPE_SUCCESS;
}




































bool pipe_mgr_tcam_tind_get_line_no(tcam_stage_info_t *stage_data,
                                    uint32_t physical_line_index,
                                    uint32_t *tind_line_no_p,
                                    uint32_t *tind_block,
                                    uint32_t *subword_pos) {
  uint32_t tind_line_no = 0;

  if (!stage_data->total_tind_lines) {
    return false;
  }

  tind_line_no =
      physical_line_index / stage_data->tind_pack_format.entries_per_tbl_word;
  PIPE_MGR_DBGCHK(tind_line_no < stage_data->total_tind_lines);

  if (tind_block) {
    *tind_block = tind_line_no / TOF_SRAM_UNIT_DEPTH;
  }

  if (subword_pos) {
    *subword_pos =
        physical_line_index % stage_data->tind_pack_format.entries_per_tbl_word;
  }

  if (tind_line_no_p) {
    *tind_line_no_p = tind_line_no;
  }

  return true;
}

bool pipe_mgr_tcam_tind_exists(tcam_stage_info_t *stage_data) {
  if (!stage_data->total_tind_lines) {
    return false;
  }
  return true;
}
