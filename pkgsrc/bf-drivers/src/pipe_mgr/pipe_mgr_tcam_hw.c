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
 * @file pipe_mgr_tcam_hw.c
 * @date
 *
 * Implementation of TCAM hardware access, instruction post etc
 */
/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <lld/bf_dma_if.h>
#include <target-utils/bit_utils/bit_utils.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_transaction.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_hw_dump.h"
#include "pipe_mgr_adt_mgr_ha_int.h"

static pipe_status_t pipe_mgr_tcam_update_hw(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool is_recovery);
static pipe_status_t pipe_mgr_tcam_process_modify(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *head_entry,
    pipe_tbl_match_spec_t **match_specs,
    pipe_mgr_move_list_t *move_node);
static pipe_status_t resources_update(tcam_tbl_t *tcam_tbl,
                                      tcam_llp_entry_t *tcam_entry,
                                      tcam_phy_loc_info_t *tcam_loc,
                                      pipe_tcam_op_e tcam_op,
                                      struct pipe_mgr_mat_data *mat_data);

static bool move_across_log_tables(tcam_stage_info_t *src_stage_info,
                                   tcam_stage_info_t *dest_stage_info) {
  return (src_stage_info != dest_stage_info);
}

static bool move_across_stages(tcam_stage_info_t *src_stage_info,
                               tcam_stage_info_t *dest_stage_info) {
  return (src_stage_info->stage_id != dest_stage_info->stage_id);
}

static tcam_llp_entry_t *pipe_mgr_tcam_llp_entry_allocate(void) {
  tcam_llp_entry_t *tcam_entry;

  tcam_entry = (tcam_llp_entry_t *)PIPE_MGR_CALLOC(1, sizeof(tcam_llp_entry_t));
  if (tcam_entry == NULL) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }
  tcam_entry->index = PIPE_MGR_TCAM_INVALID_IDX;
  return tcam_entry;
}

void pipe_mgr_tcam_llp_entry_destroy(tcam_llp_entry_t *tcam_entry) {
  if (!tcam_entry) {
    return;
  }

  PIPE_MGR_FREE(tcam_entry);
}

tcam_llp_entry_t *pipe_mgr_tcam_llp_entry_get(tcam_tbl_info_t *tcam_tbl_info,
                                              pipe_mat_ent_hdl_t ent_hdl,
                                              uint32_t subindex) {
  tcam_llp_entry_t *head_entry = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  bf_map_sts_t st = BF_MAP_OK;

  tcam_pipe_tbl = pipe_mgr_tcam_tbl_get_instance_from_entry(
      tcam_tbl_info, ent_hdl, __func__, __LINE__);
  if (!tcam_pipe_tbl) {
    return NULL;
  }

  st = bf_map_get(
      &tcam_pipe_tbl->llp.tcam_entry_db, ent_hdl, (void **)&head_entry);
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error looking up tcam entry"
        " 0x%x ",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        ent_hdl);
    return NULL;
  }

  tcam_llp_entry_t *tcam_entry = NULL;
  for (tcam_entry = head_entry; tcam_entry; tcam_entry = tcam_entry->next) {
    PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));
    tcam_llp_entry_t *range_entry = NULL;
    for (range_entry = tcam_entry; range_entry;
         range_entry = range_entry->next_range) {
      if (range_entry->subentry_index == subindex) {
        return range_entry;
      }
    }
  }

  return NULL;
}

static pipe_status_t pipe_mgr_tcam_llp_entry_htbl_add(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    pipe_mat_ent_hdl_t ent_hdl,
    tcam_llp_entry_t *tcam_entry) {
  if (tcam_pipe_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  if (tcam_tbl_info == NULL) {
    return PIPE_INVALID_ARG;
  }

  bf_map_sts_t st = BF_MAP_OK;

  st = bf_map_add(&tcam_pipe_tbl->llp.tcam_entry_db, ent_hdl, tcam_entry);
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error adding tcam entry to lookup "
        "array for entry-hdl 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_NO_SYS_RESOURCES;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_llp_entry_htbl_delete(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_mat_ent_hdl_t ent_hdl) {
  if (tcam_pipe_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  if (tcam_tbl_info == NULL) {
    return PIPE_INVALID_ARG;
  }

  bf_map_sts_t st = BF_MAP_OK;

  st = bf_map_rmv(&tcam_pipe_tbl->llp.tcam_entry_db, ent_hdl);
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error deleting tcam entry from lookup "
        "array for entry-hdl 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

static tcam_stage_info_t *get_stage_data_for_atcam(tcam_tbl_t *tcam_tbl,
                                                   uint32_t index) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  int stage_idx = 0;
  tcam_stage_info_t *stage_data = NULL;

  for (stage_idx = 0; stage_idx < tcam_pipe_tbl->num_stages; stage_idx++) {
    stage_data = &tcam_pipe_tbl->stage_data[stage_idx];
    if (index < (stage_data->tcam_start_index + stage_data->no_tcam_entries)) {
      return stage_data;
    }
  }
  return NULL;
}

pipe_status_t pipe_mgr_tcam_get_tcam_block(tcam_tbl_t *tcam_tbl,
                                           uint32_t index,
                                           uint32_t *block_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  uint32_t block = 0;

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    tcam_stage_info_t *stage_data = NULL;
    stage_data = get_stage_data_for_atcam(tcam_tbl, index);
    if (stage_data == NULL) {
      LOG_ERROR("%s:%d get stage data for atcam failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    // We need to first find the stage_id

    block = stage_data->block_start_index;
    PIPE_MGR_DBGCHK(index >= stage_data->tcam_start_index);
    uint32_t index_in_stage = index - stage_data->tcam_start_index;

    block += ((index_in_stage / stage_data->pack_format.entries_per_tbl_word) *
              tcam_pipe_tbl->blocks_per_bank) +
             (tcam_tbl->ptn_index / stage_data->mem_depth);
  } else {
    block = index / PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK;
  }

  if (block >= tcam_pipe_tbl->num_blocks) {
    return PIPE_INVALID_ARG;
  }

  *block_p = block;
  return PIPE_SUCCESS;
}

uint32_t pipe_mgr_get_stage_id_for_block(tcam_tbl_t *tcam_tbl, uint32_t block) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  uint32_t stage_idx = 0;

  if (block >= get_tcam_pipe_tbl(tcam_tbl)->num_blocks) {
    return PIPE_INVALID_ARG;
  }

  stage_idx = tcam_pipe_tbl->block_data[block].stage_index;
  return tcam_pipe_tbl->stage_data[stage_idx].stage_id;
}

pipe_status_t pipe_mgr_tcam_get_phy_loc_info(tcam_tbl_t *tcam_tbl,
                                             uint32_t index,
                                             tcam_phy_loc_info_t *tcam_loc) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t physical_line_no = 0;
  uint32_t stage_idx = 0;
  uint32_t index_in_block = 0;
  uint32_t block = 0;
  vpn_id_t vpn = 0;
  tcam_stage_info_t *stage_data;
  tcam_block_data_t *block_data;
  uint32_t subword = 0;

  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);

  if (tcam_pipe_tbl->tcam_tbl_info_p->match_type != ATCAM_MATCH &&
      tcam_tbl->total_entries == 1) {
    // Keyless table
    tcam_loc->index = index;
    tcam_loc->stage_id = tcam_pipe_tbl->stage_data[0].stage_id;
    tcam_loc->stage_idx = 0;
    tcam_loc->stage_line_no = 0;
    tcam_loc->block_id = 0;
    tcam_loc->phy_line_no = 0;
    tcam_loc->subword = 0;
    return PIPE_SUCCESS;
  }

  rc = pipe_mgr_tcam_get_tcam_block(tcam_tbl, index, &block);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  block_data = &tcam_pipe_tbl->block_data[block];
  stage_idx = block_data->stage_index;
  stage_data = &tcam_pipe_tbl->stage_data[stage_idx];

  if (TCAM_TBL_IS_ATCAM(get_tcam_tbl_info(tcam_tbl))) {
    physical_line_no = tcam_tbl->ptn_index % stage_data->mem_depth;
    uint32_t index_in_stage = index - stage_data->tcam_start_index;
    subword = index_in_stage % stage_data->pack_format.entries_per_tbl_word;
  } else {
    index_in_block = index % PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK;
    physical_line_no = PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK - 1 - index_in_block;
    subword = 0;
  }

  vpn = block_data->word_blk.vpn_id[subword];

  PIPE_MGR_MEMSET(tcam_loc, 0, sizeof(tcam_phy_loc_info_t));

  tcam_loc->index = index;
  tcam_loc->stage_id = stage_data->stage_id;
  tcam_loc->stage_idx = stage_idx;
  tcam_loc->stage_line_no = (vpn << log2_uint32_ceil(stage_data->mem_depth)) |
                            (physical_line_no & (stage_data->mem_depth - 1));
  tcam_loc->block_id = block;
  tcam_loc->phy_line_no = physical_line_no;
  tcam_loc->subword = subword;
  return PIPE_SUCCESS;
}

void pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t block,
    uint32_t physical_line_no,
    uint32_t subword,
    uint32_t *index_p,
    uint32_t *ptn_index_p) {
  uint32_t index, ptn_index;
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    /* blocks_per_bank - indicates how many partitions there are.
     *                   For 3k partitions, the blocks_per_bank will be 3 (since
     *                   mem depth is 1k)
     *
     * any index calculation is related to stage. Because in each stage, the
     * number of entries packed and the number of blocks allocated can be
     * different. However blocks_per_bank remains the same for all stages
     *
     * The blocks are numbered as below:
     *
     * Assume blocks_per_bank = 3
     *        entries_per_tbl_word = 2
     *
     *        0       3       6       10
     *        1       4       7       11
     *        2       5       9       12 etc
     *
     * Points to note:
     *        Block 0 has 2k entries
     *        All the entries in block 0 have index either 0 or 1
     *        The partition index for all the entries in block 0 is 0-1023
     *
     * The below math essentially tries to figure out where in the above
     * matrix the entry is and calculates the partition index and the
     * index within the stage
     */

    uint32_t stage_idx = 0;
    tcam_stage_info_t *stage_data;
    tcam_block_data_t *block_data;

    block_data = &tcam_pipe_tbl->block_data[block];
    stage_idx = block_data->stage_index;
    stage_data = &tcam_pipe_tbl->stage_data[stage_idx];

    uint32_t stage_block, index_in_stage;
    stage_block = block - stage_data->block_start_index;

    ptn_index = ((stage_block % tcam_pipe_tbl->blocks_per_bank) *
                 stage_data->mem_depth) +
                physical_line_no;

    index_in_stage = ((stage_block / tcam_pipe_tbl->blocks_per_bank) *
                      stage_data->pack_format.entries_per_tbl_word) +
                     subword;
    index = index_in_stage + stage_data->tcam_start_index;
  } else {
    PIPE_MGR_DBGCHK(physical_line_no < PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK);
    index = ((block + 1) * PIPE_TOTAL_TCAM_ENTRIES_PER_BLOCK) -
            physical_line_no - 1;
    ptn_index = 0;
  }

  *ptn_index_p = ptn_index;
  *index_p = index;
}

pipe_status_t pipe_mgr_get_tcam_index_for_match_addr(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    uint32_t stage_line_no,
    uint32_t *index_p,
    uint32_t *ptn_index_p) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  uint32_t block, phy_line_no, subword = 0;
  tcam_stage_info_t *stage_data = NULL, *next_stage_data;
  vpn_id_t vpn_id;
  uint32_t start_block, end_block;
  unsigned int stage_idx = 0;

  for (stage_idx = 0; stage_idx < tcam_pipe_tbl->num_stages; ++stage_idx) {
    if (tcam_pipe_tbl->stage_data[stage_idx].stage_id == stage_id &&
        tcam_pipe_tbl->stage_data[stage_idx].stage_table_handle ==
            stage_table_handle) {
      stage_data = &tcam_pipe_tbl->stage_data[stage_idx];
      break;
    }
  }
  if (!stage_data) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Invalid stage %d and logical table id %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        stage_id,
        stage_table_handle);
    return PIPE_INVALID_ARG;
  }
  next_stage_data = (stage_idx + 1) < tcam_pipe_tbl->num_stages
                        ? &tcam_pipe_tbl->stage_data[stage_idx + 1]
                        : NULL;

  vpn_id = stage_line_no >> log2_uint32_ceil(stage_data->mem_depth);
  phy_line_no = stage_line_no & (stage_data->mem_depth - 1);

  start_block = stage_data->block_start_index;
  end_block = next_stage_data ? next_stage_data->block_start_index - 1
                              : (tcam_pipe_tbl->num_blocks - 1);
  for (block = start_block; block <= end_block; block++) {
    tcam_block_data_t *block_data = &tcam_pipe_tbl->block_data[block];
    for (subword = 0; subword < stage_data->pack_format.entries_per_tbl_word;
         subword++) {
      if (vpn_id == block_data->word_blk.vpn_id[subword]) {
        break;
      }
    }
    if (subword != stage_data->pack_format.entries_per_tbl_word) {
      break;
    }
  }

  if (block > end_block) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Match address 0x%x is invalid in stage %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        stage_line_no,
        stage_id);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(
      tcam_pipe_tbl, block, phy_line_no, subword, index_p, ptn_index_p);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
    tcam_tbl_t *tcam_tbl,
    uint32_t index,
    bool is_default,
    tcam_phy_loc_info_t *tcam_loc) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;

  PIPE_MGR_MEMSET(tcam_loc, 0, sizeof(tcam_phy_loc_info_t));

  if (!is_default || (get_tcam_pipe_tbl(tcam_tbl)->default_ent_type ==
                      TCAM_DEFAULT_ENT_TYPE_DIRECT)) {
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, index, tcam_loc);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the physical location info for index %d "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          index,
          rc);
      return rc;
    }
  } else {
    tcam_loc->index = PIPE_MGR_TCAM_INVALID_IDX;
    tcam_loc->stage_id =
        get_tcam_pipe_tbl(tcam_tbl)
            ->stage_data[get_tcam_pipe_tbl(tcam_tbl)->num_stages - 1]
            .stage_id;
    tcam_loc->stage_idx = get_tcam_pipe_tbl(tcam_tbl)->num_stages - 1;
    tcam_loc->stage_line_no = ~0;
    tcam_loc->block_id = 0;
    tcam_loc->phy_line_no = ~0;
    tcam_loc->subword = 0;
  }

  return PIPE_SUCCESS;
}

static pipe_tbl_ref_t *get_tbl_ref(tcam_tbl_info_t *tcam_tbl_info,
                                   pipe_tbl_hdl_t tbl_hdl) {
  uint32_t i;

  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    if (tcam_tbl_info->tbl_refs[i].tbl_hdl == tbl_hdl) {
      return &tcam_tbl_info->tbl_refs[i];
    }
  }
  return NULL;
}

pipe_tbl_ref_t *pipe_mgr_tcam_get_tbl_ref_by_type(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_hdl_type_t hdl_type,
    pipe_tbl_ref_type_t ref_type) {
  uint32_t i;

  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->tbl_refs[i];
    if ((PIPE_GET_HDL_TYPE(tbl_ref->tbl_hdl) == hdl_type) &&
        (tbl_ref->ref_type == ref_type)) {
      return tbl_ref;
    }
  }
  return NULL;
}

pipe_tbl_ref_t *pipe_mgr_tcam_get_tbl_ref(tcam_tbl_info_t *tcam_tbl_info,
                                          pipe_hdl_type_t hdl_type) {
  uint32_t i;

  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->tbl_refs[i];
    if (PIPE_GET_HDL_TYPE(tbl_ref->tbl_hdl) == hdl_type) {
      return tbl_ref;
    }
  }
  return NULL;
}

bool pipe_mgr_tcam_has_direct_resource(tcam_tbl_info_t *tcam_tbl_info) {
  uint32_t i;

  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->tbl_refs[i];
    if (tbl_ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      return true;
    }
  }

  return false;
}

static pipe_status_t direct_resource_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    pipe_res_spec_t *rs,
    uint32_t stage_id,
    uint32_t stage_line_no,
    bool end_of_move_add,
    pipe_tcam_op_e tcam_op,
    struct pipe_mgr_mat_data *mat_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_act_fn_hdl_t act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);

  pipe_tbl_hdl_t tbl_hdl = rs->tbl_hdl;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  pipe_meter_impl_type_e meter_type;

  pipe_sess_hdl_t sh = get_tcam_pipe_tbl(tcam_tbl)->cur_sess_hdl;
  bf_dev_id_t dev = tcam_tbl_info->dev_id;
  uint32_t flags = get_tcam_pipe_tbl(tcam_tbl)->sess_flags;
  bf_dev_pipe_t pipe_id = get_tcam_pipe_tbl(tcam_tbl)->pipe_id;
  pipe_mat_tbl_hdl_t tcam_tbl_hdl = tcam_tbl_info->tbl_hdl;
  uint32_t x = 0;
  bool new_add = false;

  if ((tcam_op == PIPE_TCAM_OP_ALLOCATE) || (tcam_op == PIPE_TCAM_OP_MOVE) ||
      (tcam_op == PIPE_TCAM_OP_SET_DEFAULT)) {
    new_add = true;
  } else if (tcam_op == PIPE_TCAM_OP_MODIFY) {
    new_add = false;
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  switch (tbl_type) {
    case PIPE_HDL_TYPE_STAT_TBL: {
      bf_dev_target_t dev_tgt;
      dev_tgt.dev_pipe_id = pipe_id;
      dev_tgt.device_id = dev;
      bool hw_stats_init = end_of_move_add ? false : true;
      /* During an entry modify from an action without direct stats to an action
       * with direct stats the entry must be added to stats_mgr.  This can only
       * happen as part of a default entry modify/set/reset. */
      if (new_add || !tcam_entry->addr.indirect_addr_stats) {
        /* New entry : Write Zero to the index */
        rc = pipe_mgr_stat_mgr_add_entry(sh,
                                         dev_tgt,
                                         tcam_entry->entry_hdl,
                                         tbl_hdl,
                                         stage_id,
                                         stage_line_no,
                                         hw_stats_init,
                                         &rs->data.counter,
                                         &x);
        if (rc == PIPE_SUCCESS) {
          tcam_entry->addr.indirect_addr_stats = x;
        }
      }
    } break;
    case PIPE_HDL_TYPE_METER_TBL:
      if (!pipe_mgr_get_meter_impl_type(tbl_hdl, dev, &meter_type)) {
        PIPE_MGR_DBGCHK(0);
        rc = PIPE_UNEXPECTED;
        break;
      }
      switch (meter_type) {
        case PIPE_METER_TYPE_STANDARD:
          rc = rmt_meter_mgr_direct_meter_attach(sh,
                                                 dev,
                                                 tbl_hdl,
                                                 stage_line_no,
                                                 pipe_id,
                                                 stage_id,
                                                 &rs->data.meter,
                                                 &x);
          break;
        case PIPE_METER_TYPE_LPF:
          rc = rmt_meter_mgr_direct_lpf_attach(sh,
                                               dev,
                                               tbl_hdl,
                                               stage_line_no,
                                               pipe_id,
                                               stage_id,
                                               &rs->data.lpf,
                                               &x);
          break;
        case PIPE_METER_TYPE_WRED:
          rc = rmt_meter_mgr_direct_wred_attach(sh,
                                                dev,
                                                tbl_hdl,
                                                stage_line_no,
                                                pipe_id,
                                                stage_id,
                                                &rs->data.red,
                                                &x);
          break;
        default:
          rc = PIPE_UNEXPECTED;
          PIPE_MGR_DBGCHK(0);
          break;
      }
      if (rc == PIPE_SUCCESS) {
        tcam_entry->addr.indirect_addr_meter = x;
      }
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      /* Call the stateful manager's write function to update the HW. */
      if (new_add || tcam_entry->direct_stful_seq_nu !=
                         unpack_mat_ent_data_stful(mat_data)) {
        tcam_entry->direct_stful_seq_nu = unpack_mat_ent_data_stful(mat_data);
        rc = pipe_mgr_stful_direct_word_write_at(sh,
                                                 dev,
                                                 tcam_tbl_hdl,
                                                 tcam_entry->entry_hdl,
                                                 pipe_id,
                                                 stage_id,
                                                 stage_line_no,
                                                 &rs->data.stful,
                                                 flags);
      }
      /* Also get an indirect pointer to the same stateful location in
       * case this is the default entry. */
      if (PIPE_SUCCESS == rc) {
        rc = pipe_mgr_stful_get_indirect_ptr(
            dev, pipe_id, stage_id, act_fn_hdl, tbl_hdl, stage_line_no, &x);
        if (PIPE_SUCCESS == rc) {
          tcam_entry->addr.indirect_addr_stful = x;
        }
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error allocating direct resource (tbl 0x%x) for tcam entry 0x%x "
        "stage_id %d index %d rc \"%s\" 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        stage_id,
        stage_line_no,
        pipe_str_err(rc),
        rc);
    return rc;
  }
  return rc;
}

static pipe_status_t direct_resource_deallocate(tcam_tbl_t *tcam_tbl,
                                                tcam_llp_entry_t *tcam_entry,
                                                pipe_tbl_hdl_t tbl_hdl,
                                                uint32_t stage_id,
                                                uint32_t stage_line_no) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  bf_dev_target_t dev_tgt;
  bf_dev_id_t dev = tcam_tbl_info->dev_id;
  bf_dev_pipe_t pipe_id = get_tcam_pipe_tbl(tcam_tbl)->pipe_id;

  dev_tgt.dev_pipe_id = pipe_id;
  dev_tgt.device_id = dev;

  switch (tbl_type) {
    case PIPE_HDL_TYPE_STAT_TBL:
      /* Only pass the entry handle to stats_mgr for deletion if it was added
       * to stats_mgr.  Default entries are not required to use direct stats
       * tables so stats_mgr may not have been informed of this entry handle. */
      if (!tcam_entry->is_default || tcam_entry->addr.indirect_addr_stats) {
        rc = pipe_mgr_stat_mgr_delete_entry(
            dev_tgt, stage_id, tcam_entry->entry_hdl, stage_line_no, tbl_hdl);
      }
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      /* Do nothing */
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      /* Do nothing */
      rc = pipe_mgr_stful_dir_ent_del(
          dev, tbl_hdl, pipe_id, tcam_entry->entry_hdl);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  return rc;
}

static pipe_status_t direct_resource_move(tcam_tbl_t *tcam_tbl,
                                          tcam_llp_entry_t *tcam_entry,
                                          pipe_res_spec_t *rs,
                                          tcam_phy_loc_info_t *src_loc,
                                          tcam_phy_loc_info_t *dest_loc,
                                          pipe_act_fn_hdl_t act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_tbl_hdl_t tbl_hdl = rs->tbl_hdl;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);

  pipe_sess_hdl_t sh = get_tcam_pipe_tbl(tcam_tbl)->cur_sess_hdl;
  bf_dev_id_t dev = tcam_tbl_info->dev_id;
  bf_dev_pipe_t pipe_id = get_tcam_pipe_tbl(tcam_tbl)->pipe_id;
  uint32_t x = 0;

  uint32_t dest_stage_id = 0, src_stage_id = 0;
  uint32_t dest_stage_line_no = 0, src_stage_line_no = 0;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_stage_id = src_loc->stage_id;
  src_stage_line_no = src_loc->stage_line_no;
  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_stage_id = dest_loc->stage_id;
  dest_stage_line_no = dest_loc->stage_line_no;

  switch (tbl_type) {
    case PIPE_HDL_TYPE_STAT_TBL: {
      if (move_across_log_tables(src_stage_info, dest_stage_info)) {
        /* In case of cross stage_id move, just reset the dest now
         * Later, after removing src entry, and doing a
         * special dump, inform stat mgr of the move
         */
        rc = pipe_mgr_stat_mgr_reset_entry(
            sh, dev, pipe_id, tbl_hdl, dest_stage_id, dest_stage_line_no);
      } else {
        rc = rmt_stat_mgr_stat_ent_move(dev,
                                        tbl_hdl,
                                        pipe_id,
                                        tcam_entry->entry_hdl,
                                        src_stage_id,
                                        dest_stage_id,
                                        src_stage_line_no,
                                        dest_stage_line_no);
      }
    } break;
    case PIPE_HDL_TYPE_METER_TBL:
      rc = pipe_mgr_meter_mgr_move_meter_spec(sh,
                                              dev,
                                              pipe_id,
                                              src_stage_id,
                                              dest_stage_id,
                                              tbl_hdl,
                                              src_stage_line_no,
                                              dest_stage_line_no);

      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      rc = pipe_mgr_stful_move(sh,
                               dev,
                               tbl_hdl,
                               pipe_id,
                               src_stage_id,
                               dest_stage_id,
                               src_stage_line_no,
                               dest_stage_line_no);
      /* Get a new indirect pointer in case this is the default entry
       * that is moving. */
      if (PIPE_SUCCESS == rc) {
        rc = pipe_mgr_stful_get_indirect_ptr(dev,
                                             pipe_id,
                                             dest_stage_id,
                                             act_fn_hdl,
                                             tbl_hdl,
                                             dest_stage_line_no,
                                             &x);
        if (PIPE_SUCCESS == rc) {
          tcam_entry->addr.indirect_addr_stful = x;
        }
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error moving direct resource (tbl 0x%x) for tcam entry 0x%x "
        "src_stage_id %d src_index %d dest_stage_id %d dest_index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        src_stage_id,
        src_stage_line_no,
        dest_stage_id,
        dest_stage_line_no,
        rc);
    return rc;
  }
  return rc;
}

static pipe_status_t indirect_resource_allocate(tcam_tbl_t *tcam_tbl,
                                                tcam_llp_entry_t *tcam_entry,
                                                pipe_res_spec_t *rs,
                                                uint32_t stage_id,
                                                uint32_t stage_line_no,
                                                pipe_act_fn_hdl_t act_fn_hdl) {
  (void)stage_line_no;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_tbl_hdl_t tbl_hdl = rs->tbl_hdl;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);

  bf_dev_id_t dev = tcam_tbl_info->dev_id;
  bf_dev_pipe_t pipe_id = get_tcam_pipe_tbl(tcam_tbl)->pipe_id;

  uint32_t x;
  switch (tbl_type) {
    case PIPE_HDL_TYPE_STAT_TBL:
      rc = rmt_stat_mgr_stat_ent_attach(
          dev, pipe_id, stage_id, tbl_hdl, rs->tbl_idx, &x);
      if (rc == PIPE_SUCCESS) {
        tcam_entry->addr.indirect_addr_stats = x;
      }
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      rc = rmt_meter_mgr_meter_attach(
          dev, pipe_id, stage_id, tbl_hdl, rs->tbl_idx, &x);
      if (rc == PIPE_SUCCESS) {
        tcam_entry->addr.indirect_addr_meter = x;
      }
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      /* Populate the TCAM entry's indirect pointer. */
      rc = pipe_mgr_stful_get_indirect_ptr(
          dev, pipe_id, stage_id, act_fn_hdl, tbl_hdl, rs->tbl_idx, &x);
      if (PIPE_SUCCESS == rc) {
        tcam_entry->addr.indirect_addr_stful = x;
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error allocating indirect resource (tbl 0x%x) for tcam entry 0x%x "
        "stage_id %d index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        stage_id,
        stage_line_no,
        rc);
    return rc;
  }
  return rc;
}

static pipe_status_t indirect_resource_deallocate(tcam_tbl_t *tcam_tbl,
                                                  tcam_llp_entry_t *tcam_entry,
                                                  pipe_tbl_hdl_t tbl_hdl) {
  (void)tcam_tbl;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);

  switch (tbl_type) {
    case PIPE_HDL_TYPE_STAT_TBL:
      tcam_entry->addr.indirect_addr_stats = 0;
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      tcam_entry->addr.indirect_addr_meter = 0;
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      tcam_entry->addr.indirect_addr_stful = 0;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t resource_allocate(tcam_tbl_t *tcam_tbl,
                                       tcam_llp_entry_t *tcam_entry,
                                       pipe_res_spec_t *rs,
                                       uint32_t stage_id,
                                       uint32_t stage_line_no,
                                       pipe_tcam_op_e tcam_op,
                                       struct pipe_mgr_mat_data *mat_data) {
  pipe_act_fn_hdl_t act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_tbl_hdl_t tbl_hdl = rs->tbl_hdl;
  pipe_tbl_ref_t *tbl_ref = get_tbl_ref(tcam_tbl_info, tbl_hdl);

  if (!tbl_ref) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Resource tbl 0x%x not found. Unable to allocate resource "
        "for tcam_entry 0x%x "
        "stage_id %d index %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        stage_id,
        stage_line_no);
    return PIPE_INVALID_ARG;
  }

  if (rs->tag != PIPE_RES_ACTION_TAG_ATTACHED) return PIPE_SUCCESS;

  pipe_tbl_ref_type_t ref_type = tbl_ref->ref_type;

  if (ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    tcam_prev_move_t *prev_move = NULL;
    bool end_of_move_add = false;
    prev_move = &tcam_tbl->llp.prev_move_cookie;
    if (prev_move->prev_move_type != TCAM_MOVE_INVALID) {
      end_of_move_add = true;
    }
    return direct_resource_allocate(tcam_tbl,
                                    tcam_entry,
                                    rs,
                                    stage_id,
                                    stage_line_no,
                                    end_of_move_add,
                                    tcam_op,
                                    mat_data);
  } else {
    return indirect_resource_allocate(
        tcam_tbl, tcam_entry, rs, stage_id, stage_line_no, act_fn_hdl);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t resource_deallocate(tcam_tbl_t *tcam_tbl,
                                         tcam_llp_entry_t *tcam_entry,
                                         pipe_tbl_hdl_t tbl_hdl,
                                         pipe_tcam_op_e tcam_op,
                                         uint32_t stage_id,
                                         uint32_t stage_line_no) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_tbl_ref_t *tbl_ref = get_tbl_ref(tcam_tbl_info, tbl_hdl);

  if (!tbl_ref) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Resource tbl 0x%x not found. Unable to deallocate resource "
        "for tcam_entry 0x%x "
        "stage_id %d index %d ",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        stage_id,
        stage_line_no);
    return PIPE_INVALID_ARG;
  }

  pipe_tbl_ref_type_t ref_type = tbl_ref->ref_type;

  /* In case of modification, the direct entries need not be deallocated. They
   * are just overwritten
   */
  if (ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    if (tcam_op != PIPE_TCAM_OP_MODIFY) {
      return direct_resource_deallocate(
          tcam_tbl, tcam_entry, tbl_hdl, stage_id, stage_line_no);
    }
  } else {
    return indirect_resource_deallocate(tcam_tbl, tcam_entry, tbl_hdl);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t resource_move(tcam_tbl_t *tcam_tbl,
                                   tcam_llp_entry_t *tcam_entry,
                                   pipe_res_spec_t *rs,
                                   tcam_phy_loc_info_t *src_loc,
                                   tcam_phy_loc_info_t *dest_loc,
                                   pipe_tcam_op_e tcam_op,
                                   struct pipe_mgr_mat_data *mat_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_act_fn_hdl_t act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_tbl_hdl_t tbl_hdl = rs->tbl_hdl;

  pipe_tbl_ref_t *tbl_ref = get_tbl_ref(tcam_tbl_info, tbl_hdl);

  uint32_t dest_stage_id = 0, src_stage_id = 0;
  uint32_t dest_stage_line_no = 0, src_stage_line_no = 0;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_stage_id = src_loc->stage_id;
  src_stage_line_no = src_loc->stage_line_no;
  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_stage_id = dest_loc->stage_id;
  dest_stage_line_no = dest_loc->stage_line_no;

  if (!tbl_ref) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Resource tbl 0x%x not found. Unable to deallocate resource "
        "for tcam_entry 0x%x "
        "src_stage_id %d src_index %d dest_stage_id %d dest_index %d",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tbl_hdl,
        tcam_entry->entry_hdl,
        src_stage_id,
        src_stage_line_no,
        dest_stage_id,
        dest_stage_line_no);
    return PIPE_INVALID_ARG;
  }

  if (rs->tag != PIPE_RES_ACTION_TAG_ATTACHED) return PIPE_SUCCESS;

  pipe_tbl_ref_type_t ref_type = tbl_ref->ref_type;

  if (ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    return direct_resource_move(
        tcam_tbl, tcam_entry, rs, src_loc, dest_loc, act_fn_hdl);
  } else {
    if (move_across_stages(src_stage_info, dest_stage_info)) {
      rc = resource_deallocate(tcam_tbl,
                               tcam_entry,
                               rs->tbl_hdl,
                               tcam_op,
                               src_stage_id,
                               src_stage_line_no);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      rc = resource_allocate(tcam_tbl,
                             tcam_entry,
                             rs,
                             dest_stage_id,
                             dest_stage_line_no,
                             tcam_op,
                             mat_data);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }

    return PIPE_SUCCESS;
  }

  return rc;
}

static pipe_status_t pipe_mgr_tcam_post_instruction(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_sess_hdl_t *cur_sess_hdl,
    pipe_bitmap_t *pipe_bmp,
    uint8_t stage_id,
    uint8_t *instr,
    uint8_t instr_len,
    uint8_t *data,
    uint8_t data_len) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_drv_ilist_add_2(cur_sess_hdl,
                                tcam_tbl_info->dev_info,
                                pipe_bmp,
                                stage_id,
                                instr,
                                instr_len,
                                data,
                                data_len);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d - %s (%d - 0x%x) Instruction add failed rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static bool pipe_mgr_tcam_is_movereg_needed(tcam_tbl_info_t *tcam_tbl_info) {
  (void)tcam_tbl_info;
  /* Move reg will always be enabled since it is the move efficient way to move
   * entries.  A pop will block for a number of cycles when the previous
   * operation was a tcam copy or write.  If we do not use a pop instruction a
   * delay of up to 25 nops would need to be inserted to allow the lookup
   * pipeline to cycle through.  In a MAU's pipeline the TCAM lookups happen at
   * the very begining while a direct ADT lookup using the match address will be
   * done 10s of cycles later.  So, during a move, when we copy an existing
   * entry, then invalidate it at the original location and begin to rewrite the
   * TIND, ADT, etc. to add a new entry at that location we could see a match at
   * that location (just before the invalidate) use the updated SRAM entries
   * (TIND, ADT, ...). */
  return true;
}

static pipe_status_t pipe_mgr_tcam_execute_push_move_adr(
    tcam_tbl_t *tcam_tbl, tcam_stage_info_t *stage_info, uint32_t stage_line_no)

{
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_push_table_move_adr_instr_t instr;

  construct_instr_move_reg_push(tcam_tbl_info->dev_id,
                                &instr,
                                stage_info->stage_table_handle,
                                stage_line_no);

  rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                      &tcam_pipe_tbl->cur_sess_hdl,
                                      &tcam_pipe_tbl->pipe_bmp,
                                      stage_info->stage_id,
                                      (uint8_t *)&instr,
                                      sizeof(instr),
                                      NULL,
                                      0);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Instruction add failed 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  stage_info->movereg_dest_addr = stage_info->movereg_src_addr;
  stage_info->movereg_src_addr = stage_line_no;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_pop_move_adr(
    tcam_pipe_tbl_t *tcam_pipe_tbl, tcam_stage_info_t *stage_info) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_pop_table_move_adr_instr_t instr;
  bool stat_init = false;

  /* See the NOTE above in pipe_mgr_tcam_idle_time_allocate()
   * With tcams, the idle time entry will be initialized explicitly
   * so writing disabled value with pop instruction is ok
   */
  construct_instr_move_reg_pop(tcam_tbl_info->dev_id,
                               &instr,
                               stage_info->stage_table_handle,
                               stat_init,
                               1);

  rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                      &tcam_pipe_tbl->cur_sess_hdl,
                                      &tcam_pipe_tbl->pipe_bmp,
                                      stage_info->stage_id,
                                      (uint8_t *)&instr,
                                      sizeof(instr),
                                      NULL,
                                      0);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Instruction add failed 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  stage_info->movereg_dest_addr = stage_info->movereg_src_addr;
  stage_info->movereg_src_addr = PIPE_MGR_INVALID_MOVEREG_ADDR;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_setup_movereg_for_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_stage_info_t *dest_stage_info,
    uint32_t dest_stage_line_no) {
  pipe_status_t rc = PIPE_SUCCESS;

  /* The sequence to follow for moveregs is as follows:
   * While moving down (higher to lower priority)
   *         - Push dest
   *         - Push src
   *         - Copy TCAM
   *         - Write ADT for dest
   *         - Write TIND for dest
   *         - Invalidate the src
   *         - Pop
   *         - Write ADT for new entry
   *         - Write S2P resources for new entry
   *         - Pop
   *         - Write TIND for new entry
   *         - Write TCAM for new entry
   *
   * While moving up (lower to higher priority)
   *         - Push dest
   *         - Push src
   *         - Write ADT for dest
   *         - Write TIND for dest
   *         - Write TCAM for dest
   *         - Pop
   *         - Invalidate the src
   *         - Pop
   *         - Write ADT for new entry
   *         - Write S2P resources for new entry
   *         - Write TIND for new entry
   *         - Write TCAM for new entry
   *
   * As seen above, while moving up the entries themselves take
   * care of inhibiting the movereg matches. So invalidation is not
   * necessary */

  if (dest_stage_info->movereg_src_addr != dest_stage_line_no) {
    if (dest_stage_info->movereg_src_addr == PIPE_MGR_INVALID_MOVEREG_ADDR) {
      /* Nothing to do here */
      PIPE_MGR_DBGCHK(dest_stage_info->movereg_dest_addr ==
                      PIPE_MGR_INVALID_MOVEREG_ADDR);
    } else {
      /* We need to flush out the existing entries */
      rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                              dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
      rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                              dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  } else {
    /* Do one POP. Another is done after adding related actions,
     * before adding TCAM entry
     */
    rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                            dest_stage_info);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_setup_movereg_for_move(
    tcam_tbl_t *tcam_tbl,
    tcam_stage_info_t *src_stage_info,
    tcam_stage_info_t *dest_stage_info,
    uint32_t src_stage_line_no,
    uint32_t dest_stage_line_no) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (dest_stage_info->movereg_src_addr != dest_stage_line_no) {
    if (dest_stage_info->movereg_src_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      /* We need to flush out the existing entries */
      rc |= pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                               dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
      rc |= pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                               dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
    if (dest_stage_info->movereg_src_addr == PIPE_MGR_INVALID_MOVEREG_ADDR) {
      PIPE_MGR_DBGCHK(dest_stage_info->movereg_dest_addr ==
                      PIPE_MGR_INVALID_MOVEREG_ADDR);
      rc |= pipe_mgr_tcam_execute_push_move_adr(
          tcam_tbl, dest_stage_info, dest_stage_line_no);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  }

  rc |= pipe_mgr_tcam_execute_push_move_adr(
      tcam_tbl, src_stage_info, src_stage_line_no);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);

  PIPE_MGR_DBGCHK(dest_stage_info->movereg_src_addr == src_stage_line_no);
  PIPE_MGR_DBGCHK(dest_stage_info->movereg_dest_addr == dest_stage_line_no);
  return rc;
}

static pipe_status_t pipe_mgr_tcam_setup_moveregs(tcam_tbl_t *tcam_tbl,
                                                  tcam_phy_loc_info_t *src_loc,
                                                  tcam_phy_loc_info_t *dest_loc,
                                                  pipe_tcam_op_e tcam_op) {
  /*
   * Setting up moveregs involves the below logic:
   * Moveregs are only needed if the table has a directly addressed
   * stateful table (idle, stats, meter, stful) etc
   *
   * Moveregs are needed in the below situations:
   *  - Moves within a stage_id
   *  - When a new entry getting allocated evicts an existing entry (i.e.
   *    the new allocate is happening after moves)
   *
   * For inter-stage_id moves, movereg has to be set-up as below:
   *  - In the destination stage_id, the entry will be added as a new entry.
   *    So setup the destination stage_id like a new entry add
   *
   */
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;
  bool movereg_needed = false;
  uint32_t src_stage_line_no = 0, dest_stage_line_no = 0;

  if (!pipe_mgr_tcam_is_movereg_needed(tcam_tbl_info)) {
    return PIPE_SUCCESS;
  }

  /* Setting up moveregs are needed only for moving and new add
   * after a move
   */
  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_stage_line_no = dest_loc->stage_line_no;

  if (tcam_op == PIPE_TCAM_OP_MOVE) {
    movereg_needed = true;

    src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
    src_stage_line_no = src_loc->stage_line_no;
  }

  tcam_prev_move_t *prev_move = NULL;
  prev_move = &tcam_tbl->llp.prev_move_cookie;
  if ((tcam_op == PIPE_TCAM_OP_ALLOCATE) &&
      (prev_move->prev_move_type != TCAM_MOVE_INVALID)) {
    movereg_needed = true;
  }

  if (movereg_needed == false) {
    return PIPE_SUCCESS;
  }

  switch (tcam_op) {
    case PIPE_TCAM_OP_MOVE:
      if (!move_across_log_tables(src_stage_info, dest_stage_info)) {
        rc = pipe_mgr_tcam_setup_movereg_for_move(tcam_tbl,
                                                  src_stage_info,
                                                  dest_stage_info,
                                                  src_stage_line_no,
                                                  dest_stage_line_no);
      } else {
        rc = pipe_mgr_tcam_setup_movereg_for_allocate(
            tcam_tbl, dest_stage_info, dest_stage_line_no);
      }
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Setting up moveregs failed for move from %d to %d 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            src_loc->index,
            dest_loc->index,
            rc);
        return rc;
      }

      break;
    case PIPE_TCAM_OP_ALLOCATE:
      rc = pipe_mgr_tcam_setup_movereg_for_allocate(
          tcam_tbl, dest_stage_info, dest_stage_line_no);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Setting up moveregs failed for allocate after move at %d 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            dest_loc->index,
            rc);
        return rc;
      }
      break;
    default:
      break;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_teardown_moveregs(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_stage_info_t *dest_stage_info = NULL;
  tcam_stage_info_t *src_stage_info = NULL;
  bool movereg_needed = false;
  uint32_t dest_stage_line_no = 0;

  if (!pipe_mgr_tcam_is_movereg_needed(tcam_tbl_info)) {
    return PIPE_SUCCESS;
  }

  /* Tearing down moveregs are needed for new add
   * after a move or a inter-stage_id move (which is essentially a new add)
   */
  dest_stage_line_no = dest_loc->stage_line_no;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);

  if (tcam_op == PIPE_TCAM_OP_MOVE) {
    src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
    if (move_across_log_tables(src_stage_info, dest_stage_info)) {
      movereg_needed = true;
    }
  }

  tcam_prev_move_t *prev_move = NULL;
  prev_move = &tcam_tbl->llp.prev_move_cookie;
  if ((tcam_op == PIPE_TCAM_OP_ALLOCATE) &&
      (prev_move->prev_move_type != TCAM_MOVE_INVALID)) {
    movereg_needed = true;
  }

  if (movereg_needed == false) {
    return PIPE_SUCCESS;
  }

  PIPE_MGR_DBGCHK(dest_stage_info->movereg_src_addr ==
                  PIPE_MGR_INVALID_MOVEREG_ADDR);
  if (dest_stage_info->movereg_dest_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
    PIPE_MGR_DBGCHK(dest_stage_info->movereg_dest_addr == dest_stage_line_no);
    rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                            dest_stage_info);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Tearing down moveregs failed for at %d 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        dest_loc->index,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_nops(tcam_tbl_t *tcam_tbl,
                                                uint32_t stage) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  bool is_sram_based = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  int nop_count = is_sram_based ? 12 : 25;

  pipe_noop_instr_t nop;
  construct_instr_noop(tcam_tbl_info->dev_id, &nop);
  for (int n = 0; n < nop_count && rc == PIPE_SUCCESS; ++n) {
    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage,
                                        (uint8_t *)&nop,
                                        sizeof nop,
                                        NULL,
                                        0);
  }
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) Error posting tcam delay instruction rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
  }
  return rc;
}

static pipe_status_t pipe_mgr_tcam_execute_invalidate_entry(
    tcam_tbl_t *tcam_tbl, tcam_phy_loc_info_t *tcam_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  uint32_t i = 0;
  pipe_tcam_invalidate_instr_t tcam_invalidate_instr;
  pipe_instr_set_memdata_i_only_t tcam_set_memdata_instr;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t line_no = 0;
  tcam_stage_info_t *stage_data = NULL;
  tcam_block_data_t *block_data = NULL;
  uint8_t *instr_p = NULL;
  uint8_t instr_sz = 0, data_sz = 0;
  uint32_t stage_id;
  bool tcam_words_updated[TOF_MAX_TCAM_WORDS_IN_TERN_TBL_WORD];
  uint8_t version_valid_bits = 0;
  bf_dev_pipe_t pipe_id;
  bool is_stash = false;

  PIPE_MGR_MEMSET(tcam_words_updated, 0, sizeof(tcam_words_updated));

  line_no = tcam_loc->phy_line_no;
  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);
  stage_id = stage_data->stage_id;
  block_data = get_tcam_block_data(tcam_tbl, tcam_loc);

  pipe_mem_type_t pipe_mem_type;
  pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info) ? pipe_mem_type_unit_ram
                                                   : pipe_mem_type_tcam;

  uint32_t wide_tcam_units = stage_data->pack_format.mem_units_per_tbl_word;
  uint8_t *tbl_words[wide_tcam_units];

  uint8_t word_index = 0;

  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
  }

  for (i = 0; i < wide_tcam_units; i++) {
    word_index = wide_tcam_units - i - 1;
    rc = pipe_mgr_phy_mem_map_get_ref(tcam_tbl_info->dev_id,
                                      tcam_tbl_info->direction,
                                      pipe_mem_type,
                                      pipe_id,
                                      stage_id,
                                      block_data->word_blk.mem_id[i],
                                      line_no,
                                      &tbl_words[word_index],
                                      false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the reference to shadow data for index %d "
          " rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_loc->index,
          rc);
      return rc;
    }
  }

  switch (pipe_mem_type) {
    case pipe_mem_type_tcam:
      for (i = 0; i < wide_tcam_units; i++) {
        word_index = wide_tcam_units - i - 1;
        PIPE_MGR_MEMSET(tbl_words[word_index], 0, PIPE_MGR_MAU_WORD_WIDTH);







          // Set the MRD Bit to 1 for power saving
          tbl_words[word_index][0] = 0x1;
          tcam_words_updated[word_index] = true;

      }
      break;
    case pipe_mem_type_unit_ram:
      RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(RMT_EXM_ENTRY_VERSION_INVALID,
                                           RMT_EXM_ENTRY_VERSION_INVALID,
                                           version_valid_bits);
      rc = pipe_mgr_entry_format_tof_exm_tbl_ent_set_vv(
          tcam_tbl_info->dev_id,
          tcam_tbl_info->profile_id,
          stage_id,
          tcam_tbl_info->tbl_hdl,
          stage_data->stage_table_handle,
          tcam_loc->subword,
          version_valid_bits,
          tbl_words,
          tcam_words_updated,
          is_stash);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error calling tcam encoder routine"
            " rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
      break;
    default:
      return PIPE_INVALID_ARG;
  }

  for (i = 0; i < wide_tcam_units; i++) {
    /* Tcam words are in the reverse order of the word_blks */
    word_index = wide_tcam_units - i - 1;
    if (!tcam_words_updated[word_index]) {
      continue;
    }

    switch (pipe_mem_type) {
      case pipe_mem_type_tcam:













          construct_instr_tcam_invalidate(tcam_tbl_info->dev_id,
                                          &tcam_invalidate_instr,
                                          block_data->word_blk.mem_id[i],
                                          line_no,
                                          (i == wide_tcam_units - 1) ? 1 : 0);
          instr_p = (uint8_t *)&tcam_invalidate_instr;
          instr_sz = sizeof(pipe_tcam_invalidate_instr_t);

        data_sz = TOF_BYTES_IN_TCAM_WHOLE_WORD;
        break;
      case pipe_mem_type_unit_ram:
        construct_instr_set_memdata_no_data(tcam_tbl_info->dev_id,
                                            &tcam_set_memdata_instr,
                                            TOF_BYTES_IN_RAM_WORD,
                                            block_data->word_blk.mem_id[i],
                                            tcam_tbl_info->direction,
                                            stage_id,
                                            line_no,
                                            pipe_mem_type);
        instr_p = (uint8_t *)&tcam_set_memdata_instr;
        instr_sz = sizeof(pipe_instr_set_memdata_i_only_t);
        data_sz = TOF_BYTES_IN_RAM_WORD;
        break;
      default:
        return PIPE_INVALID_ARG;
    }

    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_id,
                                        instr_p,
                                        instr_sz,
                                        tbl_words[word_index],
                                        data_sz);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Invalidation of tcam entry at %d failed "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_loc->index,
          rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_update_tcam_shadow_data(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint32_t index = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t stage_id = 0;
  uint8_t version_valid_bits = 0;
  uint32_t i = 0;
  bool mrd_terminate = true;
  bool *mrd_terminate_indices = NULL;
  uint32_t line_no = 0;
  bool tcam_words_updated[TOF_MAX_TCAM_WORDS_IN_TERN_TBL_WORD];
  tcam_stage_info_t *stage_data = NULL;
  tcam_block_data_t *block_data = NULL;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_set_tcam_write_reg_instr_t tcam_wr_instr;
  pipe_instr_set_memdata_i_only_t tcam_set_memdata_instr;
  uint8_t *instr_p = NULL;
  uint8_t instr_sz = 0, data_sz = 0;
  bf_dev_pipe_t pipe_id;
  bool is_stash = false;

  PIPE_MGR_MEMSET(tcam_words_updated,
                  0,
                  sizeof(bool) * TOF_MAX_TCAM_WORDS_IN_TERN_TBL_WORD);

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  index = tcam_entry->index;

  line_no = tcam_loc->phy_line_no;

  block_data = get_tcam_block_data(tcam_tbl, tcam_loc);
  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);

  stage_id = stage_data->stage_id;

  if (!TCAM_LLP_IS_RANGE_HEAD(tcam_entry)) {
    mrd_terminate = false;
  }

  uint32_t wide_tcam_units = stage_data->pack_format.mem_units_per_tbl_word;
  int version_index = wide_tcam_units;
  uint8_t *tbl_words[wide_tcam_units];
  int word_write_order[wide_tcam_units];
  int num_words_to_write = 0;

  uint8_t word_index = 0;

  pipe_mem_type_t pipe_mem_type, unit_ram_mem_type;
  pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info) ? pipe_mem_type_unit_ram
                                                   : pipe_mem_type_tcam;

  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
  }

  // Mark the tcam mem indices that must have the mrd terminate bit turned on.
  mrd_terminate_indices = PIPE_MGR_CALLOC(wide_tcam_units, sizeof(bool));
  for (i = 0; i < wide_tcam_units; i++) {
    word_index = wide_tcam_units - i - 1;
    rc = pipe_mgr_phy_mem_map_get_ref(tcam_tbl_info->dev_id,
                                      tcam_tbl_info->direction,
                                      pipe_mem_type,
                                      pipe_id,
                                      stage_id,
                                      block_data->word_blk.mem_id[i],
                                      line_no,
                                      &tbl_words[word_index],
                                      false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the reference to shadow data for index %d "
          " rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          index,
          rc);
      PIPE_MGR_FREE(mrd_terminate_indices);
      return rc;
    }
    mrd_terminate_indices[word_index] = block_data->word_blk.mrd_terminate[i];
  }

  pipe_tbl_match_spec_t *match_spec;
  match_spec = match_specs[tcam_entry->subentry_index];
  act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
  if (IS_ACTION_SPEC_SEL_GRP(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->sel_present);
    act_data_spec = NULL;
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(
        tcam_tbl_info->adt_present &&
        (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_INDIRECT));
  } else {
    act_data_spec = &unpack_mat_ent_data_as(mat_data)->act_data;
  }

  // VK If tcam op is for UPDATE_VERSION, then call different function

  switch (pipe_mem_type) {
    case pipe_mem_type_tcam:
      version_valid_bits = 0;
      rc = pipe_mgr_entry_format_tof_tern_tbl_ent_update(
          tcam_tbl_info->dev_id,
          tcam_tbl_info->profile_id,
          stage_id,
          tcam_tbl_info->tbl_hdl,
          version_valid_bits,
          match_spec,
          mrd_terminate,
          mrd_terminate_indices,
          tbl_words,
          wide_tcam_units,
          tcam_words_updated);
      break;
    case pipe_mem_type_unit_ram:
      if (match_spec->version_bits == 0xD) {
        // Special case for ALPM covering prefix handling.
        version_valid_bits = match_spec->version_bits & 0xF;
      } else {
        RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                             RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                             version_valid_bits);
      }

      rc = pipe_mgr_entry_format_tof_exm_tbl_ent_update(
          tcam_tbl_info->dev_id,
          tcam_tbl_info->profile_id,
          stage_id,
          tcam_tbl_info->tbl_hdl,
          stage_data->stage_table_handle,
          version_valid_bits,
          match_spec,
          act_fn_hdl,
          act_data_spec,
          tcam_entry->addr.indirect_addr_action,
          tcam_entry->addr.indirect_addr_stats,
          tcam_entry->addr.indirect_addr_meter,
          tcam_entry->addr.indirect_addr_stful,
          tcam_entry->addr.indirect_addr_sel,
          tcam_entry->addr.sel_grp_pvl,
          tcam_loc->subword,
          0, /* Proxy hash value of ZERO always */
          tbl_words,
          tcam_words_updated,
          &version_index,
          is_stash);
      break;
    default:
      rc = PIPE_INVALID_ARG;
  }
  PIPE_MGR_FREE(mrd_terminate_indices);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error calling tcam encoder routine"
        " rc 0x%x",
        __func__,
        __LINE__,
        rc);
    return rc;
  }

  /* Determine the order to write the memory units.  For TCAM based tables just
   * write in reverse order of the word blocks, but for SRAM based tables save
   * the word containing the version/valid bits for last. */
  for (i = 0; i < wide_tcam_units; i++) {
    word_index = wide_tcam_units - i - 1;
    /* If the word wasn't updated, don't write it. */
    if (!tcam_words_updated[word_index]) continue;
    /* If the word has the version/valid write it last.  This only applies to
     * SRAM based tables. */
    if (word_index == version_index) continue;
    word_write_order[num_words_to_write++] = word_index;
  }
  /* Now add the version/valid word last. */
  if (pipe_mem_type == pipe_mem_type_unit_ram)
    word_write_order[num_words_to_write++] = version_index;

  for (i = 0; (int)i < num_words_to_write; i++) {
    /* Tcam words are in the reverse order of the word_blks */
    word_index = word_write_order[i];
    int mem_idx = wide_tcam_units - word_index - 1;
    switch (pipe_mem_type) {
      case pipe_mem_type_tcam:














          construct_instr_tcam_write(
              tcam_tbl_info->dev_id,
              &tcam_wr_instr,
              block_data->word_blk.mem_id[mem_idx],
              line_no,
              ((int)i == num_words_to_write - 1) ? 1 : 0);
          instr_p = (uint8_t *)&tcam_wr_instr;
          instr_sz = sizeof(uint32_t);

        data_sz = TOF_BYTES_IN_TCAM_WHOLE_WORD;
        break;
      case pipe_mem_type_unit_ram:
        unit_ram_mem_type = pipe_mem_type;
        if (tcam_op == PIPE_TCAM_OP_REFRESH_TIND &&
            (tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO2 ||
             tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
          /* Write to the shadow register for updates */
          unit_ram_mem_type = pipe_mem_type_shadow_reg;





        }
        construct_instr_set_memdata_no_data(
            tcam_tbl_info->dev_id,
            &tcam_set_memdata_instr,
            TOF_BYTES_IN_RAM_WORD,
            block_data->word_blk.mem_id[mem_idx],
            tcam_tbl_info->direction,
            stage_id,
            line_no,
            unit_ram_mem_type);
        instr_p = (uint8_t *)&tcam_set_memdata_instr;
        instr_sz = sizeof(pipe_instr_set_memdata_i_only_t);
        data_sz = TOF_BYTES_IN_RAM_WORD;
        break;
      default:
        return PIPE_INVALID_ARG;
    }

    rc = pipe_mgr_tcam_post_instruction(get_tcam_tbl_info(tcam_tbl),
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_id,
                                        instr_p,
                                        instr_sz,
                                        tbl_words[word_index],
                                        data_sz);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

#define IS_INDR_PTR_SET(res_ptr) (res_ptr != 0 && res_ptr != (uint32_t)-1)

static pipe_status_t pipe_mgr_tcam_act_data_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    bool uses_direct_adt,
    bool update,
    bool is_recovery) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_action_spec_t *act_spec = unpack_mat_ent_data_as(mat_data);

  uint32_t stage_id = tcam_loc->stage_id;
  uint32_t stage_line_no = tcam_loc->stage_line_no;
  tcam_stage_info_t *stage_info;
  stage_info = get_tcam_stage_data(tcam_tbl, tcam_loc);

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (!tcam_tbl_info->adt_present) {
    return PIPE_SUCCESS;
  }

  if (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_DIRECT &&
      uses_direct_adt) {
    rc = rmt_adt_ent_add(tcam_pipe_tbl->cur_sess_hdl,
                         tcam_tbl_info->dev_id,
                         tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                         tcam_pipe_tbl->pipe_id,
                         stage_id,
                         unpack_mat_ent_data_afun_hdl(mat_data),
                         &act_spec->act_data,
                         stage_line_no,
                         stage_info->stage_table_handle,
                         &tcam_entry->addr.indirect_addr_action,
                         update);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Dev %d pipe %d tbl %s 0x%x entry %u: Error adding an entry to "
          "action table 0x%x %s",
          __func__,
          __LINE__,
          tcam_tbl_info->dev_id,
          tcam_pipe_tbl->pipe_id,
          tcam_tbl_info->name,
          tcam_tbl_info->tbl_hdl,
          tcam_entry->entry_hdl,
          tcam_tbl_info->adt_tbl_ref.tbl_hdl,
          pipe_str_err(rc));
      return rc;
    }
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    if (!tcam_tbl_info->restore_ent_node) {
      // If action is already set this is repair entry flow. ADT should be
      //  already activated, so skip that.
      if (!is_recovery ||
          !IS_INDR_PTR_SET(tcam_entry->addr.indirect_addr_action)) {
        rc = rmt_adt_ent_activate_stage(tcam_pipe_tbl->cur_sess_hdl,
                                        tcam_tbl_info->dev_id,
                                        tcam_pipe_tbl->pipe_id,
                                        tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                                        act_spec->adt_ent_hdl,
                                        stage_id,
                                        logical_action_idx,
                                        &tcam_entry->addr.indirect_addr_action,
                                        update);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s(%x) Error activating action data hdl 0x%x in stage_id"
              " %d rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->tbl_hdl,
              act_spec->adt_ent_hdl,
              stage_id,
              rc);
          return rc;
        }
      }
    } else {
      cJSON *addr =
          cJSON_GetObjectItem(tcam_tbl_info->restore_ent_node, "addr");
      tcam_entry->addr.indirect_addr_action =
          cJSON_GetObjectItem(addr, "action")->valuedouble;
    }
    tcam_entry->adt_ent_hdl = act_spec->adt_ent_hdl;
    tcam_entry->logical_action_idx = logical_action_idx;
  } else {
    tcam_entry->addr.indirect_addr_action = 0;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_act_data_deallocate(
    tcam_tbl_t *tcam_tbl, tcam_llp_entry_t *tcam_entry, uint32_t stage_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (!tcam_tbl_info->adt_present) {
    return PIPE_SUCCESS;
  }

  if (tcam_entry->adt_ent_hdl) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->adt_tbl_ref.ref_type ==
                    PIPE_TBL_REF_TYPE_INDIRECT);
    rc = rmt_adt_ent_deactivate_stage(get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
                                      tcam_tbl_info->dev_id,
                                      tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                                      tcam_entry->adt_ent_hdl,
                                      stage_id,
                                      tcam_entry->logical_action_idx);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(%x) Error deactivating action data entry rc 0x%x",
                __func__,
                __LINE__,
                tcam_tbl_info->name,
                tcam_tbl_info->tbl_hdl,
                rc);
      return rc;
    }
  }

  tcam_entry->addr.indirect_addr_action = -1;
  tcam_entry->adt_ent_hdl = 0;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_act_data_move(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_action_spec_t *act_spec = NULL;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  uint32_t src_stage_id = src_loc->stage_id;
  uint32_t src_stage_line_no = src_loc->stage_line_no;
  uint32_t dest_stage_id = dest_loc->stage_id;
  uint32_t dest_stage_line_no = dest_loc->stage_line_no;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bool uses_direct_adt = false;
  if (!tcam_tbl_info->adt_present) {
    return PIPE_SUCCESS;
  } else if (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    uses_direct_adt = true;
  }

  act_spec = unpack_mat_ent_data_as(mat_data);

  pipe_adt_ent_hdl_t adt_ent_hdl;
  if (IS_ACTION_SPEC_ACT_DATA(act_spec)) {
    adt_ent_hdl = tcam_entry->adt_ent_hdl;
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    adt_ent_hdl = act_spec->adt_ent_hdl;
  } else {
    return PIPE_SUCCESS;
  }
  (void)adt_ent_hdl;

  if (move_across_stages(src_stage_info, dest_stage_info)) {
    rc = pipe_mgr_tcam_act_data_deallocate(tcam_tbl, tcam_entry, src_stage_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error deallocating action data for move from stage_id %d "
          "stage_id-idx %d to dest stage_id %d stage_id-idx %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          src_stage_id,
          src_stage_line_no,
          dest_stage_id,
          dest_stage_line_no,
          rc);
      return rc;
    }

    rc = pipe_mgr_tcam_act_data_allocate(tcam_tbl,
                                         tcam_entry,
                                         dest_loc,
                                         mat_data,
                                         logical_action_idx,
                                         uses_direct_adt,
                                         false /*update*/,
                                         false /*is_recovery*/);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error allocating action data for move from stage_id %d "
          "stage_id-idx %d to dest stage_id %d stage_id-idx %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          src_stage_id,
          src_stage_line_no,
          dest_stage_id,
          dest_stage_line_no,
          rc);
      return rc;
    }
  } else {
    if (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      rc = rmt_adt_ent_add(tcam_pipe_tbl->cur_sess_hdl,
                           tcam_tbl_info->dev_id,
                           tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                           tcam_pipe_tbl->pipe_id,
                           dest_stage_id,
                           unpack_mat_ent_data_afun_hdl(mat_data),
                           &act_spec->act_data,
                           dest_stage_line_no,
                           dest_stage_info->stage_table_handle,
                           NULL,
                           false);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error moving action data for move from stage_id %d "
            "stage_id-idx %d to dest stage_id %d stage_id-idx %d rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            src_stage_id,
            src_stage_line_no,
            dest_stage_id,
            dest_stage_line_no,
            rc);
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_sel_grp_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    uint32_t stage_id,
    uint32_t stage_line_no,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_sel_idx,
    pipe_idx_t logical_action_idx,
    bool is_recovery) {
  (void)stage_line_no;
  pipe_status_t rc;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_action_spec_t *act_spec = unpack_mat_ent_data_as(mat_data);

  if (!tcam_tbl_info->sel_present) {
    return PIPE_SUCCESS;
  }

  if (!IS_ACTION_SPEC_SEL_GRP(act_spec)) {
    return PIPE_SUCCESS;
  }

  if (!is_recovery || !IS_INDR_PTR_SET(tcam_entry->addr.indirect_addr_action)) {
    uint32_t indirect_addr_action;
    rc = rmt_adt_ent_get_addr(tcam_tbl_info->dev_id,
                              get_tcam_pipe_tbl(tcam_tbl)->pipe_id,
                              tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                              stage_id,
                              logical_action_idx,
                              &indirect_addr_action);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting virtual address for action logical index %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          logical_action_idx,
          rc);
      return rc;
    }
    tcam_entry->addr.indirect_addr_action = indirect_addr_action;
  }

  if (!is_recovery || !IS_INDR_PTR_SET(tcam_entry->addr.indirect_addr_sel)) {
    rmt_virt_addr_t indirect_sel_addr = 0;
    rc = pipe_mgr_sel_logical_idx_to_vaddr(tcam_tbl_info->dev_id,
                                           tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                           stage_id,
                                           logical_sel_idx,
                                           &indirect_sel_addr);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting virtual address for selection logical index %d rc "
          "0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          logical_sel_idx,
          rc);
      return rc;
    }
    tcam_entry->addr.indirect_addr_sel = indirect_sel_addr;
  }

  tcam_entry->sel_grp_hdl = act_spec->sel_grp_hdl;
  tcam_entry->addr.sel_grp_pvl = unpack_mat_ent_data_sel(mat_data);
  tcam_entry->logical_action_idx = logical_action_idx;
  tcam_entry->logical_sel_idx = logical_sel_idx;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_sel_grp_deallocate(
    tcam_tbl_t *tcam_tbl, tcam_llp_entry_t *tcam_entry, uint32_t stage_id) {
  (void)tcam_tbl;
  (void)stage_id;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (!tcam_tbl_info->sel_present) {
    return PIPE_SUCCESS;
  }

  if (tcam_entry->sel_grp_hdl == 0) {
    return PIPE_SUCCESS;
  }

  tcam_entry->sel_grp_hdl = 0;
  tcam_entry->addr.indirect_addr_sel = -1;
  tcam_entry->addr.indirect_addr_action = -1;
  tcam_entry->addr.sel_grp_pvl = 0;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_sel_grp_move(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_sel_idx,
    pipe_idx_t logical_action_idx) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_action_spec_t *act_spec = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  uint32_t dest_stage_id = 0, src_stage_id = 0;
  uint32_t dest_stage_line_no = 0, src_stage_line_no = 0;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_stage_id = src_loc->stage_id;
  src_stage_line_no = src_loc->stage_line_no;
  dest_stage_id = dest_loc->stage_id;
  dest_stage_line_no = dest_loc->stage_line_no;

  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (!tcam_tbl_info->sel_present) {
    return PIPE_SUCCESS;
  }

  act_spec = unpack_mat_ent_data_as(mat_data);
  if (!IS_ACTION_SPEC_SEL_GRP(act_spec)) {
    return PIPE_SUCCESS;
  }

  if (!move_across_stages(src_stage_info, dest_stage_info)) {
    return PIPE_SUCCESS;
  }

  rc = pipe_mgr_tcam_sel_grp_deallocate(tcam_tbl, tcam_entry, src_stage_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deallocating sel-grp for move from stage_id %d "
        "stage_id-idx %d to dest stage_id %d stage_id-idx %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        src_stage_id,
        src_stage_line_no,
        dest_stage_id,
        dest_stage_line_no,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_sel_grp_allocate(tcam_tbl,
                                      tcam_entry,
                                      dest_stage_id,
                                      dest_stage_line_no,
                                      mat_data,
                                      logical_sel_idx,
                                      logical_action_idx,
                                      false /*is_recovery*/);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error allocating sel-grp for move from stage_id %d "
        "stage_id-idx %d to dest stage_id %d stage_id-idx %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        src_stage_id,
        src_stage_line_no,
        dest_stage_id,
        dest_stage_line_no,
        rc);
    return rc;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_stats_accumulate_for_move(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint32_t src_stage_id;
  uint32_t src_stage_line_no;
  uint32_t dest_stage_id;
  uint32_t dest_stage_line_no;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_stage_id = dest_loc->stage_id;
  dest_stage_line_no = dest_loc->stage_line_no;
  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_stage_id = src_loc->stage_id;
  src_stage_line_no = src_loc->stage_line_no;

  pipe_tbl_ref_t *tbl_ref;
  tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
      tcam_tbl_info, PIPE_HDL_TYPE_STAT_TBL, PIPE_TBL_REF_TYPE_DIRECT);
  if (!tbl_ref) {
    return PIPE_SUCCESS;
  }

  if (!move_across_log_tables(src_stage_info, dest_stage_info)) {
    return PIPE_SUCCESS;
  }

  rc = rmt_stat_mgr_direct_stat_ent_database_sync(tcam_pipe_tbl->cur_sess_hdl,
                                                  tcam_tbl_info->dev_id,
                                                  tcam_tbl_info->tbl_hdl,
                                                  tcam_entry->entry_hdl,
                                                  tbl_ref->tbl_hdl,
                                                  tcam_pipe_tbl->pipe_id,
                                                  src_stage_id,
                                                  src_stage_line_no);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error dumping direct stats for accumulation entry 0x%x "
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

  bf_dev_id_t dev = tcam_tbl_info->dev_id;
  bf_dev_pipe_t pipe_id = get_tcam_pipe_tbl(tcam_tbl)->pipe_id;
  /* Now inform the stat tbl of the move - this piece is moved down from
   * direct_resource_move to here.
   */
  rc = rmt_stat_mgr_stat_ent_move(dev,
                                  tbl_ref->tbl_hdl,
                                  pipe_id,
                                  tcam_entry->entry_hdl,
                                  src_stage_id,
                                  dest_stage_id,
                                  src_stage_line_no,
                                  dest_stage_line_no);

  return rc;
}

static pipe_status_t pipe_mgr_tcam_idle_time_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    struct pipe_mgr_mat_data *mat_data) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t stage_line_no = 0;
  uint32_t stage_id = 0;

  if (!tcam_tbl_info->idle_present || tcam_entry->is_default) {
    return PIPE_SUCCESS;
  }

  stage_id = tcam_loc->stage_id;
  stage_line_no = tcam_loc->stage_line_no;

  /* NOTE:
   * end_of_move_add is not used with TCAM. The movereg_idle_pop_ctl
   * register usage with tcam results in multiple code-path.
   * i.e. we need to handle all the below cases differently
   * - Add with no moves
   * - Add with moves only within stage_id
   * - Add in stage_id x, with a move however the moves are in other stages
   * - Add in stage_id x, with some moves in stage_id x and some moves in other
   *stages
   *
   * This results in higher probability for bugs.
   * The benefits of simplified code far outweighs the benefits of using
   * movereg_idle_pop_ctl and the POP instruction idle_disable_val
   * bit to reduce 1 instruction.
   * Hence, for tcams we explicitly initialze the idletime entry
   * for every add
   */

  rc = rmt_idle_add_entry(
      tcam_pipe_tbl->cur_sess_hdl,
      tcam_tbl_info->dev_id,
      tcam_tbl_info->tbl_hdl,
      tcam_entry->entry_hdl,
      tcam_pipe_tbl->pipe_id,
      stage_id,
      get_tcam_stage_data(tcam_tbl, tcam_loc)->stage_table_handle,
      stage_line_no,
      tcam_entry->is_default ? 0 : unpack_mat_ent_data_ttl(mat_data),
      false,
      tcam_pipe_tbl->sess_flags);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error allocating idle table entry for tcam entry 0x%x at index %d "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_idle_time_deallocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t stage_line_no = 0;
  uint32_t stage_id = 0;

  if (!tcam_tbl_info->idle_present || tcam_entry->is_default) {
    return PIPE_SUCCESS;
  }
  stage_id = tcam_loc->stage_id;
  stage_line_no = tcam_loc->stage_line_no;

  rc = rmt_idle_delete_entry(
      tcam_pipe_tbl->cur_sess_hdl,
      tcam_tbl_info->dev_id,
      tcam_tbl_info->tbl_hdl,
      tcam_entry->entry_hdl,
      tcam_pipe_tbl->pipe_id,
      stage_id,
      get_tcam_stage_data(tcam_tbl, tcam_loc)->stage_table_handle,
      stage_line_no,
      tcam_pipe_tbl->sess_flags);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deleting idle table entry for tcam entry 0x%x at index %d "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_idle_time_move(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (!tcam_tbl_info->idle_present) {
    return PIPE_SUCCESS;
  }

  rc = rmt_idle_move_entry(
      tcam_pipe_tbl->cur_sess_hdl,
      tcam_tbl_info->dev_id,
      tcam_tbl_info->tbl_hdl,
      tcam_entry->entry_hdl,
      tcam_pipe_tbl->pipe_id,
      src_loc->stage_id,
      get_tcam_stage_data(tcam_tbl, src_loc)->stage_table_handle,
      dest_loc->stage_id,
      get_tcam_stage_data(tcam_tbl, dest_loc)->stage_table_handle,
      src_loc->stage_line_no,
      dest_loc->stage_line_no,
      tcam_pipe_tbl->sess_flags);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error moving idle table entry for tcam entry 0x%x from "
        " src stage_id %d stage_id-index %d "
        " to dest stage_id %d stage_id-index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        src_loc->stage_id,
        src_loc->stage_line_no,
        dest_loc->stage_id,
        dest_loc->stage_line_no,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_action_allocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool uses_direct_adt,
    bool update,
    bool is_recovery) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t stage_id = 0, stage_line_no = 0;

  stage_id = tcam_loc->stage_id;
  stage_line_no = tcam_loc->stage_line_no;

  rc = pipe_mgr_tcam_act_data_allocate(tcam_tbl,
                                       tcam_entry,
                                       tcam_loc,
                                       mat_data,
                                       logical_action_idx,
                                       uses_direct_adt,
                                       update,
                                       is_recovery);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error allocating action data "
        "rc 0x%x",
        __func__,
        __LINE__,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_sel_grp_allocate(tcam_tbl,
                                      tcam_entry,
                                      stage_id,
                                      stage_line_no,
                                      mat_data,
                                      logical_sel_idx,
                                      logical_action_idx,
                                      is_recovery);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error activating sel grp "
        "rc 0x%x",
        __func__,
        __LINE__,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_action_deallocate(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t stage_id = 0;

  stage_id = tcam_loc->stage_id;

  rc = pipe_mgr_tcam_act_data_deallocate(tcam_tbl, tcam_entry, stage_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error deallocating action data for "
        "adt_ent_hdl %d tcam entry 0x%x, rc 0x%x",
        __func__,
        __LINE__,
        tcam_entry->adt_ent_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_sel_grp_deallocate(tcam_tbl, tcam_entry, stage_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error de-activating the selection grp for tcam 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_action_move(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool mov_mod) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Move the idle entry first */
  rc = pipe_mgr_tcam_idle_time_move(tcam_tbl, tcam_entry, src_loc, dest_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error moving idle table entry for tcam entry 0x%x from index %d "
        " to index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        src_loc->index,
        dest_loc->index,
        rc);
    return rc;
  }

  if (mov_mod) {
    rc = resources_update(
        tcam_tbl, tcam_entry, src_loc, PIPE_TCAM_OP_MODIFY, mat_data);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  int i;
  for (i = 0; i < unpack_mat_ent_data_as(mat_data)->resource_count; i++) {
    pipe_res_spec_t *rs = &unpack_mat_ent_data_as(mat_data)->resources[i];
    rc = resource_move(
        tcam_tbl, tcam_entry, rs, src_loc, dest_loc, tcam_op, mat_data);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  rc = pipe_mgr_tcam_act_data_move(
      tcam_tbl, tcam_entry, src_loc, dest_loc, mat_data, logical_action_idx);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error moving action data entry for tcam entry 0x%x from index %d "
        " to index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        src_loc->index,
        dest_loc->index,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_sel_grp_move(tcam_tbl,
                                  tcam_entry,
                                  src_loc,
                                  dest_loc,
                                  mat_data,
                                  logical_sel_idx,
                                  logical_action_idx);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error moving selector group for tcam entry 0x%x from index %d "
        " to index %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        src_loc->index,
        dest_loc->index,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_delete_entry(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    pipe_tcam_op_e tcam_op,
    tcam_phy_loc_info_t *tcam_loc) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_TRACE(
      "TCAM %s execute del entry %d index %d block %d line_no %d "
      "index %d",
      tcam_tbl->tcam_pipe_tbl_p->tcam_tbl_info_p->name,
      tcam_entry->entry_hdl,
      tcam_entry->index,
      tcam_loc->block_id,
      tcam_loc->phy_line_no,
      tcam_loc->index);
  rc = pipe_mgr_tcam_idle_time_deallocate(tcam_tbl, tcam_entry, tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deallocating idle entry for entry delete "
        "at %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_action_deallocate(tcam_tbl, tcam_entry, tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error deallocating action data for tcam index %d rc 0x%x",
              __func__,
              __LINE__,
              tcam_entry->index,
              rc);
    return rc;
  }

  uint32_t i;
  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    rc = resource_deallocate(tcam_tbl,
                             tcam_entry,
                             tcam_tbl_info->tbl_refs[i].tbl_hdl,
                             tcam_op,
                             tcam_loc->stage_id,
                             tcam_loc->stage_line_no);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  uint32_t tcam_index = 0;
  tcam_llp_entry_t *range_entry = NULL;
  (void)range_entry;
  (void)tcam_index;
  tcam_phy_loc_info_t range_loc = *tcam_loc;

  FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_BEGIN(
      tcam_entry, range_entry, tcam_index) {
    if (tcam_index != range_loc.index) {
      rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_index, &range_loc);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    rc = pipe_mgr_tcam_execute_invalidate_entry(tcam_tbl, &range_loc);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error invalidating entry at %d rc 0x%x",
                __func__,
                __LINE__,
                tcam_index,
                rc);
      return rc;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_END()

  /* In case we are batching and the next operation is a new entry add at any of
   * the locations invalidated just above we need to add some delay to allow the
   * MAU's lookup pipeline to move forward and any packets which which may have
   * matched the invalidated addresses above to exit the stage's pipeline. */
  rc = pipe_mgr_tcam_execute_nops(tcam_tbl, tcam_loc->stage_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error invalidating entry at %d rc 0x%x",
              __func__,
              __LINE__,
              tcam_index,
              rc);
    return rc;
  }

  return rc;
}

static pipe_status_t pipe_mgr_tind_execute_add_entry(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    pipe_tcam_op_e tcam_op,
    struct pipe_mgr_mat_data *mat_data) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_instr_set_memdata_i_only_t tind_set_instr;
  uint32_t index = 0;
  uint32_t stage_id = 0;
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t tind_subword_pos = 0;
  pipe_action_data_spec_t *act_data_spec = NULL;
  uint32_t stage_line_no = 0;
  uint32_t tind_line_no = 0, tind_block = 0;
  tcam_stage_info_t *stage_data;
  bf_dev_pipe_t pipe_id;

  index = tcam_entry->index;
  stage_id = tcam_loc->stage_id;
  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);
  stage_line_no = tcam_loc->stage_line_no;

  bool tind_exists;








    tind_exists = pipe_mgr_tcam_tind_get_line_no(stage_data,
                                                 stage_line_no,
                                                 &tind_line_no,
                                                 &tind_block,
                                                 &tind_subword_pos);


  if (!tind_exists) {
    return PIPE_SUCCESS;
  }

  act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
  if (IS_ACTION_SPEC_SEL_GRP(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->sel_present);
    act_data_spec = NULL;
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(
        tcam_tbl_info->adt_present &&
        (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_INDIRECT));
  } else {
    act_data_spec = &unpack_mat_ent_data_as(mat_data)->act_data;
  }

  uint8_t *tind_data = NULL;

  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
  }
  uint8_t formatting_index = tind_subword_pos;
  pipe_mem_type_t mem_type = pipe_mem_type_unit_ram;
  uint16_t mem_unit_depth =
      pipe_mgr_get_mem_unit_depth(tcam_tbl_info->dev_id, RMT_MEM_SRAM);
  uint32_t offset = 0;













  rc = pipe_mgr_phy_mem_map_get_ref(tcam_tbl_info->dev_id,
                                    tcam_tbl_info->direction,
                                    mem_type,
                                    pipe_id,
                                    stage_id,
                                    stage_data->tind_blk[tind_block].mem_id[0],
                                    tind_line_no % mem_unit_depth,
                                    &tind_data,
                                    false);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the reference to tind shadow data for index %d "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        index,
        rc);
    return rc;
  }

  rc = pipe_mgr_entry_format_tof_tind_tbl_ent_update(
      tcam_tbl_info->dev_id,
      tcam_tbl_info->profile_id,
      stage_id,
      tcam_tbl_info->tbl_hdl,
      act_fn_hdl,
      act_data_spec,
      tcam_entry->addr.indirect_addr_action,
      tcam_entry->addr.indirect_addr_stats,
      tcam_entry->addr.indirect_addr_meter,
      tcam_entry->addr.indirect_addr_stful,
      tcam_entry->addr.indirect_addr_sel,
      tcam_entry->addr.sel_grp_pvl,
      formatting_index,
      offset,
      (tind_tbl_word_t *)tind_data);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error calling ternary indirection encoder routine"
        " rc 0x%x",
        __func__,
        __LINE__,
        rc);
    return rc;
  }

  PIPE_MGR_MEMSET(&tind_set_instr, 0, sizeof(pipe_instr_set_memdata_i_only_t));
  uint32_t tind_data_len = TOF_BYTES_IN_RAM_WORD;

  pipe_mem_type_t pipe_mem_type = pipe_mem_type_unit_ram;
  if (tcam_op == PIPE_TCAM_OP_REFRESH_TIND &&
      (tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO2 ||
       tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    /* Write to the shadow register for updates */
    pipe_mem_type = pipe_mem_type_shadow_reg;




  }
  construct_instr_set_memdata_no_data(
      tcam_tbl_info->dev_id,
      &tind_set_instr,
      tind_data_len,
      stage_data->tind_blk[tind_block].mem_id[0],
      tcam_tbl_info->direction,
      stage_id,
      tind_line_no % mem_unit_depth,
      pipe_mem_type);

  rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                      &tcam_pipe_tbl->cur_sess_hdl,
                                      &tcam_pipe_tbl->pipe_bmp,
                                      stage_id,
                                      (uint8_t *)&tind_set_instr,
                                      sizeof(pipe_instr_set_memdata_i_only_t),
                                      tind_data,
                                      tind_data_len);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_add_update_entry(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    pipe_tcam_op_e tcam_op,
    bool special_move,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool skip_tind = false, skip_tcam = false;

  PIPE_MGR_DBGCHK(!tcam_entry->is_default);

  /* Skip programming tind in case of update of version/action data */
  if (tcam_op == PIPE_TCAM_OP_UPDATE_VERSION) {
    skip_tind = true;
  }

  if (special_move) {
    skip_tind = true;
  }

  if (!TCAM_LLP_IS_RANGE_HEAD(tcam_entry)) {
    skip_tind = true;
  }

  if (tcam_op == PIPE_TCAM_OP_REFRESH_TIND) {
    skip_tcam = true;
  }

  tcam_stage_info_t *stage_data;
  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);
  if (!pipe_mgr_tcam_tind_exists(stage_data)) {
    /* If the Tind does not exist, then the overhead if any is in the
     * tcam itself (like the 1 bit payload or ATCAM)
     * So reprogram tcam
     */
    skip_tcam = false;
  }

  if (!skip_tind) {
    rc = pipe_mgr_tind_execute_add_entry(
        tcam_tbl, tcam_entry, tcam_loc, tcam_op, mat_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error updating tind shadow data for new entry add "
          "at %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_entry->index,
          rc);
      return rc;
    }
  }

  if (!skip_tcam) {
    rc = pipe_mgr_tcam_update_tcam_shadow_data(
        tcam_tbl, tcam_entry, tcam_loc, tcam_op, match_specs, mat_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error updating shadow data for new entry add "
          "at %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_entry->index,
          rc);
      return rc;
    }
  }

  return rc;
}

static pipe_status_t pipe_mgr_tcam_execute_add_entry(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *tcam_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool is_recovery) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  PIPE_MGR_DBGCHK(!tcam_entry->is_default);

  PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));
  LOG_TRACE(
      "TCAM %s execute add entry %d index %d dest_block %d dest_line_no %d "
      "dest_index %d",
      tcam_tbl_info->name,
      tcam_entry->entry_hdl,
      tcam_entry->index,
      tcam_loc->block_id,
      tcam_loc->phy_line_no,
      tcam_loc->index);

  rc = pipe_mgr_tcam_action_allocate(tcam_tbl,
                                     tcam_entry,
                                     tcam_loc,
                                     mat_data,
                                     logical_action_idx,
                                     logical_sel_idx,
                                     true,
                                     false /*update*/,
                                     is_recovery);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error allocating action data for new entry add "
        "at %d rc 0x%x",
        __func__,
        __LINE__,
        tcam_entry->index,
        rc);
    return rc;
  }

  if (!tcam_tbl_info->restore_ent_node && !is_recovery) {
    int i;
    for (i = 0; i < unpack_mat_ent_data_as(mat_data)->resource_count; i++) {
      pipe_res_spec_t *rs = &unpack_mat_ent_data_as(mat_data)->resources[i];
      rc = resource_allocate(tcam_tbl,
                             tcam_entry,
                             rs,
                             tcam_loc->stage_id,
                             tcam_loc->stage_line_no,
                             tcam_op,
                             mat_data);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }
  }

  if (tcam_tbl_info->restore_ent_node) {
    pipe_mgr_restore_tcam_addr_node(tcam_tbl_info, tcam_entry);
  }

  if (!TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    /* Tear down the moveregs here, before adding new entry
     */
    rc = pipe_mgr_tcam_teardown_moveregs(tcam_tbl, NULL, tcam_loc, tcam_op);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error tearing down moveregs rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  uint32_t tcam_index = 0;
  tcam_llp_entry_t *range_entry = NULL;
  tcam_phy_loc_info_t range_loc = *tcam_loc;

  FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_BEGIN(
      tcam_entry, range_entry, tcam_index) {
    if (tcam_index != range_loc.index) {
      rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_index, &range_loc);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    rc = pipe_mgr_tcam_execute_add_update_entry(tcam_tbl,
                                                range_entry,
                                                &range_loc,
                                                tcam_op,
                                                false,
                                                match_specs,
                                                mat_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error updating hardware for new entry add at %d rc 0x%x",
                __func__,
                __LINE__,
                tcam_entry->index,
                rc);
      return rc;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_END()

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    /* Tear down the moveregs here, after adding new entry
     */
    rc = pipe_mgr_tcam_teardown_moveregs(tcam_tbl, NULL, tcam_loc, tcam_op);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error tearing down moveregs rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  if (!tcam_tbl_info->restore_ent_node) {
    /* Allocate the idle time entry after
     * allocating the tcam entry. Otherwise the entry may timeout prematurely
     */
    rc = pipe_mgr_tcam_idle_time_allocate(
        tcam_tbl, tcam_entry, tcam_loc, mat_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error allocating idle entry for new entry add "
          "at %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_entry->index,
          rc);
      return rc;
    }
  }

  return rc;
}

static bool pipe_mgr_tcam_can_use_copy_instr(tcam_tbl_t *tcam_tbl,
                                             tcam_phy_loc_info_t *src_loc,
                                             tcam_phy_loc_info_t *dest_loc,
                                             uint32_t *top_row_p,
                                             uint32_t *bottom_row_p,
                                             uint32_t *src_col_p,
                                             uint32_t *dest_col_p) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  uint32_t src_start_row, src_end_row, src_top_row, src_bottom_row;
  uint32_t dest_start_row, dest_end_row, dest_top_row, dest_bottom_row;
  uint32_t src_col, dest_col;
  uint32_t src_start_mem_id, src_end_mem_id;
  uint32_t dest_start_mem_id, dest_end_mem_id;

  tcam_stage_info_t *src_stage_info, *dest_stage_info;
  tcam_block_data_t *src_block_data, *dest_block_data;

  if (TCAM_TBL_IS_ATCAM(get_tcam_tbl_info(tcam_tbl))) {
    return false;
  }

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_block_data = get_tcam_block_data(tcam_tbl, dest_loc);

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_block_data = get_tcam_block_data(tcam_tbl, src_loc);

  if (!dest_stage_info || !dest_block_data || !src_stage_info ||
      !src_block_data) {
    LOG_ERROR("%s:%d Invalid TCAM data", __func__, __LINE__);
    return false;
  }

  if (move_across_log_tables(src_stage_info, dest_stage_info)) {
    return false;
  }

  // Find the start and end mem-ids (can be out of order)
  uint32_t i = 0;
  src_start_mem_id = src_block_data->word_blk.mem_id[0];
  src_end_mem_id = src_block_data->word_blk.mem_id[0];
  for (i = 1; i < src_stage_info->pack_format.mem_units_per_tbl_word; i++) {
    if (src_block_data->word_blk.mem_id[i] < src_start_mem_id) {
      src_start_mem_id = src_block_data->word_blk.mem_id[i];
    }
    if (src_block_data->word_blk.mem_id[i] > src_end_mem_id) {
      src_end_mem_id = src_block_data->word_blk.mem_id[i];
    }
  }

  dest_start_mem_id = dest_block_data->word_blk.mem_id[0];
  dest_end_mem_id = dest_block_data->word_blk.mem_id[0];
  for (i = 1; i < dest_stage_info->pack_format.mem_units_per_tbl_word; i++) {
    if (dest_block_data->word_blk.mem_id[i] < dest_start_mem_id) {
      dest_start_mem_id = dest_block_data->word_blk.mem_id[i];
    }
    if (dest_block_data->word_blk.mem_id[i] > dest_end_mem_id) {
      dest_end_mem_id = dest_block_data->word_blk.mem_id[i];
    }
  }

  rmt_dev_cfg_t *dev_cfg = &tcam_tbl_info->dev_info->dev_cfg;
  src_start_row = dev_cfg->mem_id_to_row(src_start_mem_id, pipe_mem_type_tcam);
  src_end_row = dev_cfg->mem_id_to_row(src_end_mem_id, pipe_mem_type_tcam);
  dest_start_row =
      dev_cfg->mem_id_to_row(dest_start_mem_id, pipe_mem_type_tcam);
  dest_end_row = dev_cfg->mem_id_to_row(dest_end_mem_id, pipe_mem_type_tcam);
  src_col = dev_cfg->mem_id_to_col(src_start_mem_id, pipe_mem_type_tcam);
  dest_col = dev_cfg->mem_id_to_col(dest_start_mem_id, pipe_mem_type_tcam);

  if (src_start_row < src_end_row) {
    src_bottom_row = src_start_row;
    src_top_row = src_end_row;
  } else {
    src_bottom_row = src_end_row;
    src_top_row = src_start_row;
  }
  if (dest_start_row < dest_end_row) {
    dest_bottom_row = dest_start_row;
    dest_top_row = dest_end_row;
  } else {
    dest_bottom_row = dest_end_row;
    dest_top_row = dest_start_row;
  }

  if (src_bottom_row != dest_bottom_row) {
    return false;
  }

  if (src_top_row != dest_top_row) {
    return false;
  }

  *top_row_p = src_top_row;
  *bottom_row_p = src_bottom_row;
  *src_col_p = src_col;
  *dest_col_p = dest_col;
  return true;
}

static pipe_status_t pipe_mgr_tcam_execute_copy_entry(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  bool copy_allowed = false;
  uint32_t top_row = 0, bottom_row = 0, src_col = 0, dest_col = 0;
  uint32_t src_stage_id = 0, dest_stage_id = 0;
  uint32_t dest_line_no = 0;
  uint32_t src_line_no = 0;
  uint64_t src_phy_addr, dest_phy_addr;

  copy_allowed = pipe_mgr_tcam_can_use_copy_instr(
      tcam_tbl, src_loc, dest_loc, &top_row, &bottom_row, &src_col, &dest_col);
  if (!copy_allowed) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  tcam_stage_info_t *src_stage_info, *dest_stage_info;
  tcam_block_data_t *src_block_data, *dest_block_data;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_stage_id = dest_stage_info->stage_id;
  dest_block_data = get_tcam_block_data(tcam_tbl, dest_loc);
  dest_line_no = dest_loc->phy_line_no;

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_stage_id = src_stage_info->stage_id;
  src_block_data = get_tcam_block_data(tcam_tbl, src_loc);
  src_line_no = src_loc->phy_line_no;

  pipe_tcam_copy_instr_t tcam_copy_instr;
  pipe_tcam_copy_instr_data_t tcam_copy_data;
  PIPE_MGR_MEMSET(&tcam_copy_data, 0, sizeof(pipe_tcam_copy_instr_data_t));
  PIPE_MGR_MEMSET(&tcam_copy_instr, 0, sizeof(pipe_tcam_copy_instr_t));

  construct_instr_tcam_copy(tcam_tbl_info->dev_id, &tcam_copy_instr);
  construct_instr_tcam_copy_data(tcam_tbl_info->dev_id,
                                 &tcam_copy_data,
                                 dest_line_no,
                                 src_line_no,
                                 top_row,
                                 bottom_row,
                                 dest_col,
                                 src_col);

  uint32_t *buf = (uint32_t *)&tcam_copy_data;
  *buf = htole32(*buf);
  rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                      &tcam_pipe_tbl->cur_sess_hdl,
                                      &tcam_pipe_tbl->pipe_bmp,
                                      src_stage_id,
                                      (uint8_t *)&tcam_copy_instr,
                                      sizeof(pipe_tcam_copy_instr_t),
                                      (uint8_t *)&tcam_copy_data,
                                      sizeof(pipe_tcam_copy_instr_data_t));

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
    return rc;
  }

  PIPE_MGR_DBGCHK(!TCAM_TBL_IS_ATCAM(get_tcam_tbl_info(tcam_tbl)));

  uint32_t i;
  uint32_t wide_tcam_units = src_stage_info->pack_format.mem_units_per_tbl_word;

  bf_dev_pipe_t pipe_id;
  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
  }

  for (i = 0; i < wide_tcam_units; i++) {
    src_phy_addr = tcam_tbl_info->dev_info->dev_cfg.get_relative_phy_addr(
        src_block_data->word_blk.mem_id[i], src_line_no, pipe_mem_type_tcam);
    dest_phy_addr = tcam_tbl_info->dev_info->dev_cfg.get_relative_phy_addr(
        dest_block_data->word_blk.mem_id[i], dest_line_no, pipe_mem_type_tcam);
    /* Instruct the phy-mem-map to copy the entry */
    rc = pipe_mgr_phy_mem_map_copy(tcam_tbl_info->dev_id,
                                   tcam_tbl_info->direction,
                                   pipe_id,
                                   src_stage_id,
                                   dest_stage_id,
                                   src_phy_addr,
                                   dest_phy_addr,
                                   false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error copying the shadow memory contents from %d to %d "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          src_loc->index,
          dest_loc->index,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_move_entry_down(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool mov_mod) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  uint32_t dest_block = 0, src_block = 0;
  uint32_t dest_line_no = 0;
  uint32_t src_line_no = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t no_entries = 0;
  uint32_t move_index, move_src_index, move_dest_index;
  uint32_t src_index, dest_index;
  tcam_stage_info_t *dest_stage_info;
  tcam_stage_info_t *src_stage_info;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_index = dest_loc->index;
  dest_line_no = dest_loc->phy_line_no;
  dest_block = dest_loc->block_id;

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_index = src_loc->index;
  src_line_no = src_loc->phy_line_no;
  src_block = src_loc->block_id;
  uint32_t src_stage_id = src_stage_info->stage_id;

  PIPE_MGR_DBGCHK(dest_index > src_index);
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));

  uint32_t dest_tail_index = 0;
  TCAM_LLP_GET_RANGE_TAIL_INDEX(tcam_entry, dest_tail_index);

  PIPE_MGR_DBGCHK(dest_tail_index >= dest_index);
  TCAM_LLP_GET_RANGE_ENTRY_COUNT(tcam_entry, no_entries);

  LOG_TRACE(
      "TCAM %s move entry down src block %d src line_no %d "
      "dest_block %d dest_line_no %d",
      tcam_tbl_info->name,
      src_block,
      src_line_no,
      dest_block,
      dest_line_no);

  uint32_t top_row = 0, bottom_row = 0, src_col = 0, dest_col = 0;

  if (!pipe_mgr_tcam_can_use_copy_instr(tcam_tbl,
                                        src_loc,
                                        dest_loc,
                                        &top_row,
                                        &bottom_row,
                                        &src_col,
                                        &dest_col)) {
    tcam_phy_loc_info_t range_loc = *dest_loc;

    /* Tear down the moveregs here, so that idle entry gets initialized
     * by explicit write. See NOTE in pipe_mgr_tcam_idle_time_allocate()
     */
    rc = pipe_mgr_tcam_teardown_moveregs(tcam_tbl, src_loc, dest_loc, tcam_op);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error tearing down moveregs rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }

    if (!pipe_mgr_tcam_tind_exists(dest_stage_info)) {
      /* If TIND is not present, then move the action
       * stuff here itself, so that pointers
       * are programmed correctly during
       * pipe_mgr_tcam_execute_add_update_entry()
       */
      rc = pipe_mgr_tcam_action_move(tcam_tbl,
                                     tcam_entry,
                                     src_loc,
                                     dest_loc,
                                     tcam_op,
                                     mat_data,
                                     logical_action_idx,
                                     logical_sel_idx,
                                     mov_mod);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error moving action data rc 0x%x", __func__, __LINE__, rc);
        return rc;
      }
    }

    for (move_index = dest_index; move_index < (dest_index + no_entries);
         move_index++) {
      if (move_index != range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, move_index, &range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }

      rc = pipe_mgr_tcam_execute_add_update_entry(
          tcam_tbl,
          tcam_tbl->llp.tcam_entries[move_index],
          &range_loc,
          tcam_op,
          true,
          match_specs,
          mat_data);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
        return rc;
      }
    }

    if (pipe_mgr_tcam_tind_exists(dest_stage_info)) {
      rc = pipe_mgr_tcam_action_move(tcam_tbl,
                                     tcam_entry,
                                     src_loc,
                                     dest_loc,
                                     tcam_op,
                                     mat_data,
                                     logical_action_idx,
                                     logical_sel_idx,
                                     mov_mod);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error moving action data rc 0x%x", __func__, __LINE__, rc);
        return rc;
      }

      rc = pipe_mgr_tind_execute_add_entry(
          tcam_tbl, tcam_entry, dest_loc, tcam_op, mat_data);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }

  } else {
    /* Move all entries except the head first */
    move_src_index = src_index + 1;
    if (dest_index < (src_index + no_entries)) {
      /* There's overlap */
      move_dest_index = src_index + no_entries;
    } else {
      move_dest_index = dest_index + 1;
    }

    tcam_phy_loc_info_t src_range_loc = *src_loc;
    tcam_phy_loc_info_t dest_range_loc = *dest_loc;
    for (; move_dest_index < (dest_index + no_entries);
         move_src_index++, move_dest_index++) {
      if (move_src_index != src_range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_src_index, &src_range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      if (move_dest_index != dest_range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_dest_index, &dest_range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      rc = pipe_mgr_tcam_execute_copy_entry(
          tcam_tbl, &src_range_loc, &dest_range_loc);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    /* Now move the head entry */
    rc = pipe_mgr_tcam_execute_copy_entry(tcam_tbl, src_loc, dest_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }

    rc = pipe_mgr_tcam_action_move(tcam_tbl,
                                   tcam_entry,
                                   src_loc,
                                   dest_loc,
                                   tcam_op,
                                   mat_data,
                                   logical_action_idx,
                                   logical_sel_idx,
                                   mov_mod);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error moving action data rc 0x%x", __func__, __LINE__, rc);
      return rc;
    }

    rc = pipe_mgr_tind_execute_add_entry(
        tcam_tbl, tcam_entry, dest_loc, tcam_op, mat_data);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  /* At this point the move-reg has the src and dest(inhibited) locations.
    If this is a terminate operation and table is atcam, go ahead and issue a
    pop now to make the destination matchable else pop after invalidate because
    tcam require explicit invalidate to make it unmatchable otherwise there
    would be two matching entries which is incorrect*/
  if (tcam_pipe_tbl->llp.terminate_op && TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    if (dest_stage_info->movereg_dest_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      /*pop makes the dest matchable and will have all the resources moved
        from the src*/
      rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                              dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  }
  /* For normal TCAM tables, the entries need to be invalidated when moving
   * down because the movereg logic does not inhibit TCAM matches.  For ATCAM,
   * the match happens in SRAM and the movereg logic will inhibit the match.
   * If we are modifying the entry through a move or if this operation needs
   * termination then we will also invalidate the source now. */
  if (!TCAM_TBL_IS_ATCAM(tcam_tbl_info) || mov_mod ||
      tcam_pipe_tbl->llp.terminate_op) {
    /* While moving down, invalidate the entries from the bottom so that the MRD
     * entry gets cleared in the end */
    if (dest_index > (src_index + no_entries)) {
      move_src_index = src_index + no_entries;
    } else {
      move_src_index = dest_index;
    }
    move_dest_index = dest_index;

    tcam_phy_loc_info_t range_loc = *src_loc;
    for (; move_src_index > src_index; move_src_index--, move_dest_index--) {
      if ((move_src_index - 1) != range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_src_index - 1, &range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      rc = pipe_mgr_tcam_execute_invalidate_entry(tcam_tbl, &range_loc);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error invalidating entry at %d for move up "
            " rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            move_src_index - 1,
            rc);
        return rc;
      }
    }
    if (move_across_stages(src_stage_info, dest_stage_info)) {
      rc = pipe_mgr_tcam_execute_nops(tcam_tbl, src_stage_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) Error adding cross stage move delay rc "
            "0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
    }
  }
  /* We are moving the entry down and have now posted instructions to copy the
   * source entry to the lower priority destination.  If the movereg
   * functionality is being used we can now safely shift the src to dst address
   * in the movereg logic because the destination is ready.  However, if this is
   * a multi-entry move (B->C move in a sequence of B->C, A->B, add at A) then
   * we need to shift the movereg addresses by issuing a push for the next
   * source address so that will be delayed until we process the next operation
   * and have that address.  However, if we are terminating this operation there
   * will be no "next" address so issue a pop now. */
  if (tcam_pipe_tbl->llp.terminate_op && !TCAM_TBL_IS_ATCAM(tcam_tbl_info)) {
    if (dest_stage_info->movereg_dest_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                              dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  }

  /* After making sure that the src entry is not matching, do a special
   * accumulate to accumulate stats for the src entry into dest location when we
   * are moving across logical tables within a stage or across stages. */
  rc = pipe_mgr_tcam_stats_accumulate_for_move(
      tcam_tbl, tcam_entry, src_loc, dest_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error accumulating stats for entry at %d for move down "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        dest_index,
        rc);
    return rc;
  }

  return rc;
}

static pipe_status_t pipe_mgr_tcam_execute_move_entry_up(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool mov_mod) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  uint32_t dest_block = 0, src_block = 0;
  uint32_t dest_line_no = 0;
  uint32_t src_line_no = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t no_entries = 0;
  uint32_t move_index, move_src_index, move_dest_index;
  bool overlap = false;
  uint32_t src_index, dest_index;
  tcam_stage_info_t *dest_stage_info = NULL, *src_stage_info = NULL;

  dest_stage_info = get_tcam_stage_data(tcam_tbl, dest_loc);
  dest_index = dest_loc->index;
  dest_line_no = dest_loc->phy_line_no;
  dest_block = dest_loc->block_id;

  src_stage_info = get_tcam_stage_data(tcam_tbl, src_loc);
  src_index = src_loc->index;
  src_line_no = src_loc->phy_line_no;
  src_block = src_loc->block_id;
  uint32_t src_stage_id = src_stage_info->stage_id;

  PIPE_MGR_DBGCHK(dest_index < src_index);
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));

  uint32_t dest_tail_index = 0;
  TCAM_LLP_GET_RANGE_TAIL_INDEX(tcam_entry, dest_tail_index);

  PIPE_MGR_DBGCHK(dest_tail_index >= dest_index);
  TCAM_LLP_GET_RANGE_ENTRY_COUNT(tcam_entry, no_entries);

  LOG_TRACE(
      "TCAM %s move entry up src block %d src line_no %d "
      "dest_block %d dest_line_no %d",
      tcam_tbl_info->name,
      src_block,
      src_line_no,
      dest_block,
      dest_line_no);
  uint32_t top_row = 0, bottom_row = 0, src_col = 0, dest_col = 0;

  if (!pipe_mgr_tcam_can_use_copy_instr(tcam_tbl,
                                        src_loc,
                                        dest_loc,
                                        &top_row,
                                        &bottom_row,
                                        &src_col,
                                        &dest_col)) {
    /* Tear down the moveregs here, so that idle entry gets initialized
     * by explicit write. See NOTE in pipe_mgr_tcam_idle_time_allocate()
     */
    rc = pipe_mgr_tcam_teardown_moveregs(tcam_tbl, src_loc, dest_loc, tcam_op);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error tearing down moveregs rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    rc = pipe_mgr_tcam_action_move(tcam_tbl,
                                   tcam_entry,
                                   src_loc,
                                   dest_loc,
                                   tcam_op,
                                   mat_data,
                                   logical_action_idx,
                                   logical_sel_idx,
                                   mov_mod);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error moving action data rc 0x%x", __func__, __LINE__, rc);
      return rc;
    }

    tcam_phy_loc_info_t range_loc = *dest_loc;
    for (move_index = dest_index; move_index < (dest_index + no_entries);
         move_index++) {
      if (move_index != range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, move_index, &range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }

      rc = pipe_mgr_tcam_execute_add_update_entry(
          tcam_tbl,
          tcam_tbl->llp.tcam_entries[move_index],
          &range_loc,
          tcam_op,
          false,
          match_specs,
          mat_data);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
        return rc;
      }
    }

  } else {
    rc = pipe_mgr_tcam_action_move(tcam_tbl,
                                   tcam_entry,
                                   src_loc,
                                   dest_loc,
                                   tcam_op,
                                   mat_data,
                                   logical_action_idx,
                                   logical_sel_idx,
                                   mov_mod);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error moving action data rc 0x%x", __func__, __LINE__, rc);
      return rc;
    }

    rc = pipe_mgr_tind_execute_add_entry(
        tcam_tbl, tcam_entry, dest_loc, tcam_op, mat_data);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }

    /* 2 cases to handle for ranges:
     * - move all range entries
     * - move some of the range entries up.
     *   This step needs the old mrd_terminate to be refreshed
     */

    move_dest_index = dest_index + 1;
    if (dest_tail_index >= src_index) {
      /* There's overlap */
      /* The entry at dest_index is the one at
       * dest_tail_index + 1
       */
      move_src_index = dest_tail_index + 1;
      overlap = true;
    } else {
      move_src_index = src_index + 1;
    }

    /* First move the head entry */
    rc = pipe_mgr_tcam_execute_copy_entry(tcam_tbl, src_loc, dest_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }

    tcam_phy_loc_info_t src_range_loc = *src_loc;
    tcam_phy_loc_info_t dest_range_loc = *dest_loc;
    for (; move_src_index < (src_index + no_entries);
         move_src_index++, move_dest_index++) {
      if (move_src_index != src_range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_src_index, &src_range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      if (move_dest_index != dest_range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_dest_index, &dest_range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }
      rc = pipe_mgr_tcam_execute_copy_entry(
          tcam_tbl, &src_range_loc, &dest_range_loc);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }
  }

  if (TCAM_TBL_USES_RANGE(tcam_tbl_info) ||
      move_across_log_tables(src_stage_info, dest_stage_info) || mov_mod ||
      tcam_pipe_tbl->llp.terminate_op) {
    if (overlap) {
      move_src_index = dest_tail_index + 1;
    } else {
      move_src_index = src_index;
    }

    move_dest_index = dest_index;

    tcam_phy_loc_info_t range_loc = *src_loc;

    /* Before invalidating the move source, issue a move-reg pop to allow the
     * move destination to become matchable. */
    if (dest_stage_info->movereg_dest_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      rc = pipe_mgr_tcam_execute_pop_move_adr(get_tcam_pipe_tbl(tcam_tbl),
                                              dest_stage_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }

    for (; move_src_index < (src_index + no_entries);
         move_src_index++, move_dest_index++) {
      if (move_src_index != range_loc.index) {
        rc = pipe_mgr_tcam_get_phy_loc_info(
            tcam_tbl, move_src_index, &range_loc);
        if (rc != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return rc;
        }
      }

      rc = pipe_mgr_tcam_execute_invalidate_entry(tcam_tbl, &range_loc);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error invalidating entry at %d for move up "
            " rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            move_src_index,
            rc);
        return rc;
      }
    }
    if (move_across_stages(src_stage_info, dest_stage_info)) {
      rc = pipe_mgr_tcam_execute_nops(tcam_tbl, src_stage_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) Error adding cross stage move delay rc "
            "0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
    }
  }

  /* After making sure that the src entry is not matching, do a special
   * accumulate to accumulate stats for the src entry into dest location when we
   * are moving across logical tables within a stage or across stages. */
  rc = pipe_mgr_tcam_stats_accumulate_for_move(
      tcam_tbl, tcam_entry, src_loc, dest_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error accumulating stats for entry at %d for move up "
        " rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        dest_index,
        rc);
    return rc;
  }
  return rc;
}

void pipe_mgr_tcam_get_instruction_addr(tcam_tbl_t *tcam_tbl,
                                        uint64_t *tcam_addr,
                                        uint64_t *tind_addr,
                                        uint32_t index) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  bf_dev_pipe_t pipe_id = 0, phy_pipe_id = 0;
  uint32_t line_no = 0, stage_line_no = 0;
  uint32_t tind_line_no = 0;
  uint32_t tind_block = 0;
  tcam_stage_info_t *stage_data = NULL;
  tcam_block_data_t *block_data = NULL;

  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = tcam_pipe_tbl->pipe_id;
  }

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              tcam_tbl_info->dev_id,
              pipe_str_err(rc));
    return;
  }

  tcam_phy_loc_info_t tcam_loc = {0};
  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, index, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        index,
        rc);
    return;
  }

  line_no = tcam_loc.phy_line_no;
  stage_line_no = tcam_loc.stage_line_no;
  stage_data = get_tcam_stage_data(tcam_tbl, &tcam_loc);
  block_data = get_tcam_block_data(tcam_tbl, &tcam_loc);

  pipe_mem_type_t pipe_mem_type;
  pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info) ? pipe_mem_type_unit_ram
                                                   : pipe_mem_type_tcam;

  *tcam_addr = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
      tcam_tbl_info->direction,
      phy_pipe_id,
      stage_data->stage_id,
      block_data->word_blk.mem_id[0],
      line_no,
      pipe_mem_type);

  bool tind_exists = pipe_mgr_tcam_tind_get_line_no(
      stage_data, stage_line_no, &tind_line_no, &tind_block, NULL);
  if (tind_exists) {
    *tind_addr = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
        tcam_tbl_info->direction,
        phy_pipe_id,
        stage_data->stage_id,
        stage_data->tind_blk[tind_block].mem_id[0],
        tind_line_no,
        pipe_mem_type_unit_ram);
  }
}

static bool pipe_mgr_tcam_op_needs_locking(pipe_tcam_op_e tcam_op,
                                           tcam_move_type_e prev_move_type) {
  switch (tcam_op) {
    case PIPE_TCAM_OP_ALLOCATE:
      if (prev_move_type != TCAM_MOVE_INVALID) {
        return true;
      }
      return false;
    case PIPE_TCAM_OP_UPDATE_VERSION:
    case PIPE_TCAM_OP_REFRESH_TIND:
      return false;
    case PIPE_TCAM_OP_DELETE:
    case PIPE_TCAM_OP_MOVE:
      return true;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  return false;
}

static pipe_status_t pipe_mgr_tcam_alloc_lock_id(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_mgr_lock_id_type_e lock_id_type,
    lock_id_t *lock_id_p) {
  PIPE_MGR_LOCK(&tcam_tbl_info->tbl_lock);
  lock_id_t lid = tcam_tbl_info->llp.lock_id_generator++;
  PIPE_MGR_UNLOCK(&tcam_tbl_info->tbl_lock);

  /* Encode the lock id with the type */
  PIPE_MGR_FORM_LOCK_ID(*lock_id_p, lock_id_type, lid);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_lock_stage(tcam_tbl_t *tcam_tbl,
                                              tcam_phy_loc_info_t *tcam_loc,
                                              bool lock_stat_move) {
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  tcam_tbl_info_t *tcam_tbl_info;
  tcam_stage_info_t *stage_info;
  lock_id_t lock_id = PIPE_MGR_INVALID_LOCK_ID;
  pipe_status_t rc = PIPE_SUCCESS;

  if (!tcam_tbl || !tcam_loc) {
    return PIPE_INVALID_ARG;
  }

  tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  stage_info = get_tcam_stage_data(tcam_tbl, tcam_loc);

  if (stage_info->is_idle_locked || stage_info->is_stat_locked) {
    return PIPE_SUCCESS;
  }

  pipe_mgr_lock_id_type_e l_type = tcam_tbl_info->lock_type;
  if (l_type == LOCK_ID_TYPE_INVALID ||
      (l_type == LOCK_ID_TYPE_STAT_LOCK && !lock_stat_move)) {
    tcam_pipe_tbl->llp.cur_lock_id = PIPE_MGR_INVALID_LOCK_ID;
    return PIPE_SUCCESS;
  }

  if (tcam_pipe_tbl->llp.cur_lock_id == PIPE_MGR_INVALID_LOCK_ID) {
    rc = pipe_mgr_tcam_alloc_lock_id(tcam_tbl_info,
                                     tcam_tbl_info->lock_type,
                                     &tcam_pipe_tbl->llp.cur_lock_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error generating lock-id rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  lock_id = tcam_pipe_tbl->llp.cur_lock_id;
  PIPE_MGR_DBGCHK(lock_id != PIPE_MGR_INVALID_LOCK_ID);

  /* Issue an instruction to actually lock this stage_id */

  pipe_barrier_lock_instr_t lock_instr;
  bool lock_idle = false, lock_stat = false;
  (void)lock_stat;
  switch (l_type) {
    case LOCK_ID_TYPE_ALL_LOCK:
      /* Only lock both resources if we are not doing an interstage move.
       * If we are, fall through and only lock idle table.
       */
      if (lock_stat_move) {
        lock_idle = true;
        lock_stat = true;
        construct_instr_lock_all(tcam_tbl_info->dev_id,
                                 &lock_instr,
                                 lock_id,
                                 stage_info->stage_table_handle);
        break;
      }
    /* Fall through */
    case LOCK_ID_TYPE_IDLE_LOCK:
      lock_idle = true;
      construct_instr_lock_idle(tcam_tbl_info->dev_id,
                                &lock_instr,
                                lock_id,
                                stage_info->stage_table_handle);
      break;
    case LOCK_ID_TYPE_STAT_LOCK:
      if (lock_stat_move) {
        lock_stat = true;
        construct_instr_lock_stats(tcam_tbl_info->dev_id,
                                   &lock_instr,
                                   lock_id,
                                   stage_info->stage_table_handle);
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  if (lock_stat || lock_idle) {
    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_info->stage_id,
                                        (uint8_t *)&lock_instr,
                                        sizeof(lock_instr),
                                        NULL,
                                        0);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Instruction add failed 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  stage_info->is_stat_locked = lock_stat;
  stage_info->is_idle_locked = lock_idle;

  /* Inform the other tables of the lock */
  if (lock_idle) {
    rc = rmt_idle_tbl_lock(tcam_pipe_tbl->cur_sess_hdl,
                           tcam_tbl_info->dev_id,
                           tcam_tbl_info->tbl_hdl,
                           lock_id,
                           tcam_pipe_tbl->pipe_id,
                           stage_info->stage_id,
                           stage_info->stage_table_handle,
                           tcam_pipe_tbl->sess_flags);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Failed to lock the idle table pipe %d stage_id %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }

  if (lock_stat) {
    bf_dev_target_t dev_tgt;
    dev_tgt.device_id = tcam_tbl_info->dev_id;
    dev_tgt.dev_pipe_id = tcam_pipe_tbl->pipe_id;
    pipe_tbl_ref_t *tbl_ref;

    tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
        tcam_tbl_info, PIPE_HDL_TYPE_STAT_TBL, PIPE_TBL_REF_TYPE_DIRECT);
    if (!tbl_ref) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    rc = rmt_stat_mgr_tbl_lock(
        dev_tgt, tbl_ref->tbl_hdl, stage_info->stage_id, lock_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Failed to lock the stat table pipe %d stage_id %d rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          stage_info->stage_id,
          rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_unlock_all_stages(
    tcam_pipe_tbl_t *tcam_pipe_tbl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  lock_id_t lock_id = PIPE_MGR_INVALID_LOCK_ID;

  lock_id = tcam_pipe_tbl->llp.cur_lock_id;

  /* Issue an instruction to unlock this stage_id */
  uint32_t stage_idx = 0;

  tcam_stage_info_t *stage_info = NULL;

  for (stage_idx = 0; stage_idx < tcam_pipe_tbl->num_stages; stage_idx++) {
    stage_info = &tcam_pipe_tbl->stage_data[stage_idx];

    if (stage_info->movereg_src_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      rc = pipe_mgr_tcam_execute_pop_move_adr(tcam_pipe_tbl, stage_info);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }
    if (stage_info->movereg_dest_addr != PIPE_MGR_INVALID_MOVEREG_ADDR) {
      rc = pipe_mgr_tcam_execute_pop_move_adr(tcam_pipe_tbl, stage_info);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    PIPE_MGR_DBGCHK(stage_info->movereg_src_addr ==
                    PIPE_MGR_INVALID_MOVEREG_ADDR);
    PIPE_MGR_DBGCHK(stage_info->movereg_dest_addr ==
                    PIPE_MGR_INVALID_MOVEREG_ADDR);

    if (!stage_info->is_stat_locked && !stage_info->is_idle_locked) {
      continue;
    }

    pipe_barrier_lock_instr_t lock_instr;
    if (stage_info->is_stat_locked && stage_info->is_idle_locked) {
      construct_instr_unlock_all(tcam_tbl_info->dev_id,
                                 &lock_instr,
                                 lock_id,
                                 stage_info->stage_table_handle);
    } else if (stage_info->is_idle_locked) {
      construct_instr_unlock_idle(tcam_tbl_info->dev_id,
                                  &lock_instr,
                                  lock_id,
                                  stage_info->stage_table_handle);
    } else if (stage_info->is_stat_locked) {
      construct_instr_unlock_stats(tcam_tbl_info->dev_id,
                                   &lock_instr,
                                   lock_id,
                                   stage_info->stage_table_handle);
    }

    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_info->stage_id,
                                        (uint8_t *)&lock_instr,
                                        sizeof(lock_instr),
                                        NULL,
                                        0);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Instruction add failed 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }

    /* Inform the other tables of the unlock */
    if (stage_info->is_idle_locked) {
      rc = rmt_idle_tbl_unlock(tcam_pipe_tbl->cur_sess_hdl,
                               tcam_tbl_info->dev_id,
                               tcam_tbl_info->tbl_hdl,
                               lock_id,
                               tcam_pipe_tbl->pipe_id,
                               stage_info->stage_id,
                               stage_info->stage_table_handle,
                               tcam_pipe_tbl->sess_flags);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Failed to unlock the idle table pipe %d stage_id %d rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            tcam_pipe_tbl->pipe_id,
            stage_info->stage_id,
            rc);
        return rc;
      }
    }

    if (stage_info->is_stat_locked) {
      bf_dev_target_t dev_tgt;
      dev_tgt.device_id = tcam_tbl_info->dev_id;
      dev_tgt.dev_pipe_id = tcam_pipe_tbl->pipe_id;
      pipe_tbl_ref_t *tbl_ref;

      tbl_ref = pipe_mgr_tcam_get_tbl_ref_by_type(
          tcam_tbl_info, PIPE_HDL_TYPE_STAT_TBL, PIPE_TBL_REF_TYPE_DIRECT);
      if (!tbl_ref) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      rc = rmt_stat_mgr_tbl_unlock(
          dev_tgt, tbl_ref->tbl_hdl, stage_info->stage_id, lock_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Failed to unlock the stat table pipe %d stage_id %d rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            tcam_pipe_tbl->pipe_id,
            stage_info->stage_id,
            rc);
        return rc;
      }
    }

    stage_info->is_stat_locked = false;
    stage_info->is_idle_locked = false;
  }
  tcam_pipe_tbl->llp.cur_lock_id = PIPE_MGR_INVALID_LOCK_ID;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_default_ent_update(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    struct pipe_mgr_mat_data *mat_data) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t j = 0;
  uint32_t stage_id = 0;
  pipe_instr_write_reg_i_only_t instr;
  pipe_action_data_spec_t *act_data_spec = NULL;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_register_spec_t *reg_spec_list =
      PIPE_MGR_CALLOC(tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg,
                      sizeof(pipe_register_spec_t));

  stage_id = get_tcam_pipe_tbl(tcam_tbl)
                 ->stage_data[get_tcam_pipe_tbl(tcam_tbl)->num_stages - 1]
                 .stage_id;

  if (IS_ACTION_SPEC_SEL_GRP(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(tcam_tbl_info->sel_present);
    act_data_spec = NULL;
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(unpack_mat_ent_data_as(mat_data))) {
    PIPE_MGR_DBGCHK(
        tcam_tbl_info->adt_present &&
        (tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_INDIRECT));
  } else {
    act_data_spec = &unpack_mat_ent_data_as(mat_data)->act_data;
  }

  pipe_tbl_ref_t *tbl_ref;
  tbl_ref = pipe_mgr_tcam_get_tbl_ref(tcam_tbl_info, PIPE_HDL_TYPE_METER_TBL);

  uint32_t indirect_addr_idle = tcam_entry->addr.indirect_addr_idle;
  uint32_t indirect_addr_stats = tcam_entry->addr.indirect_addr_stats;

  if (tbl_ref) {
    if (tbl_ref->color_mapram_addr_type == COLOR_MAPRAM_ADDR_TYPE_IDLE) {
      indirect_addr_idle = tcam_entry->addr.indirect_addr_meter >> 3;
    } else if (tbl_ref->color_mapram_addr_type ==
               COLOR_MAPRAM_ADDR_TYPE_STATS) {
      indirect_addr_stats = tcam_entry->addr.indirect_addr_meter >> 4;
    }
  }

  rc = pipe_mgr_entry_format_tbl_default_entry_update(
      tcam_tbl_info->dev_info,
      tcam_tbl_info->direction,
      tcam_tbl_info->profile_id,
      tcam_tbl_info->tbl_hdl,
      unpack_mat_ent_data_afun_hdl(mat_data),
      act_data_spec,
      tcam_entry->addr.indirect_addr_action,
      indirect_addr_stats,
      tcam_entry->addr.indirect_addr_meter,
      tcam_entry->addr.indirect_addr_stful,
      indirect_addr_idle,
      tcam_entry->addr.indirect_addr_sel,
      tcam_entry->addr.sel_grp_pvl,
      reg_spec_list);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error formatting the default entry for tbl 0x%x rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              rc);
    goto cleanup;
  }

  for (j = 0; j < tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg; j++) {
    LOG_TRACE("TCAM %s Default register addr 0x%x value 0x%x",
              tcam_tbl_info->name,
              reg_spec_list[j].reg_addr & 0x7ffff,
              reg_spec_list[j].reg_data);
    construct_instr_reg_write_no_data(
        tcam_tbl_info->dev_id, &instr, reg_spec_list[j].reg_addr);
    reg_spec_list[j].reg_data = htole32(reg_spec_list[j].reg_data);
    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_id,
                                        (uint8_t *)&instr,
                                        sizeof(pipe_instr_write_reg_i_only_t),
                                        (uint8_t *)&reg_spec_list[j].reg_data,
                                        sizeof(uint32_t));
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
      goto cleanup;
    }
  }
cleanup:
  PIPE_MGR_FREE(reg_spec_list);
  return rc;
}

static pipe_status_t pipe_mgr_tcam_execute_default_ent_get(
    tcam_tbl_t *tcam_tbl,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    bf_dev_pipe_t dev_pipe_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs) {
  (void)tcam_pipe_tbl;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t j = 0;
  uint32_t indirect_idle_ptr;
  bf_dev_pipe_t default_pipe_id = 0;
  bf_dev_pipe_t phy_pipe;
  bf_dev_pipe_t pipe_id = 0;

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  pipe_register_spec_t
      reg_spec_list[tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg];

  if (tcam_tbl_info->is_symmetric)
    default_pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  else
    default_pipe_id = tcam_pipe_tbl->pipe_id;

  rc = pipe_mgr_get_pipe_id(
      &tcam_pipe_tbl->pipe_bmp, dev_pipe_id, default_pipe_id, &pipe_id);
  if (rc != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s:%d Invalid request to access pipe %x for table %s "
        "0x%x device id %d",
        __func__,
        __LINE__,
        dev_pipe_id,
        tcam_tbl_info->name,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_info->dev_id);
    return rc;
  }

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              tcam_tbl_info->dev_info->dev_id,
              pipe_str_err(rc));
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

  rc =
      pipe_mgr_entry_format_tbl_default_entry_prepare(tcam_tbl_info->dev_info,
                                                      tcam_tbl_info->direction,
                                                      tcam_tbl_info->profile_id,
                                                      tcam_tbl_info->tbl_hdl,
                                                      pipe_id,
                                                      reg_spec_list);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in decoding the default entry for tbl 0x%x rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              rc);
    return rc;
  }

  for (j = 0; j < tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg; j++) {
    rc = pipe_mgr_drv_subdev_reg_rd(&tcam_pipe_tbl->cur_sess_hdl,
                                    tcam_tbl_info->dev_info->dev_id,
                                    subdev,
                                    reg_spec_list[j].reg_addr,
                                    &reg_spec_list[j].reg_data);
    LOG_TRACE("TCAM %s Default register addr 0x%x value 0x%x",
              tcam_tbl_info->name,
              reg_spec_list[j].reg_addr & 0x7ffff,
              reg_spec_list[j].reg_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Reg read failed for default register addr 0x%x rc 0x%x",
                __func__,
                __LINE__,
                reg_spec_list[j].reg_addr,
                rc);
      return rc;
    }
  }

  rc = pipe_mgr_entry_format_tbl_default_entry_get(tcam_tbl_info->dev_info,
                                                   tcam_tbl_info->profile_id,
                                                   tcam_tbl_info->tbl_hdl,
                                                   act_fn_hdl,
                                                   reg_spec_list,
                                                   act_data_spec,
                                                   &indirect_ptrs->adt_ptr,
                                                   &indirect_ptrs->stats_ptr,
                                                   &indirect_ptrs->meter_ptr,
                                                   &indirect_ptrs->stfl_ptr,
                                                   &indirect_idle_ptr,
                                                   &indirect_ptrs->sel_ptr,
                                                   &indirect_ptrs->sel_len);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in decoding the default entry for tbl 0x%x rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              rc);
    return rc;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_default_ent_add(
    tcam_pipe_tbl_t *tcam_pipe_tbl, pipe_mgr_move_list_t *move_node) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, 0);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_mat_ent_hdl_t entry_hdl = move_node->entry_hdl;
  struct pipe_mgr_mat_data *mat_data = move_node->data;
  pipe_idx_t logical_action_idx = move_node->logical_action_idx;
  pipe_idx_t logical_sel_idx = move_node->logical_sel_idx;

  LOG_TRACE("Dev %d pipe %x default entry program",
            tcam_tbl_info->dev_id,
            tcam_pipe_tbl->pipe_id);

  /* If the default entry already exists then do a modify instead of an add. */
  if (tcam_pipe_tbl->llp.default_ent_set) {
    rc = pipe_mgr_tcam_process_modify(tcam_pipe_tbl,
                                      tcam_pipe_tbl->llp.llp_default_tcam_entry,
                                      NULL,
                                      move_node);
    return rc;
  }

  tcam_llp_entry_t *tcam_entry;

  tcam_entry = pipe_mgr_tcam_llp_entry_allocate();
  if (!tcam_entry) {
    return PIPE_NO_SYS_RESOURCES;
  }
  tcam_entry->range_list_p = &tcam_entry->range_list;

  BF_LIST_DLL_AP(
      *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
  tcam_entry->pipe_id = tcam_pipe_tbl->pipe_id;
  tcam_entry->is_default = true;
  tcam_entry->ptn_index = 0;
  tcam_entry->entry_hdl = entry_hdl;

  if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
    uint32_t index = tcam_tbl->total_entries - 1;
    tcam_entry->index = index;
    PIPE_MGR_DBGCHK(tcam_tbl->llp.tcam_entries[index] == NULL);
    tcam_tbl->llp.tcam_entries[index] = tcam_entry;
  }

  if (tcam_entry == NULL || !tcam_entry->is_default) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
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

  /* The pipe_mgr_tcam_action_allocate function will handle direct action,
   * indirect action, and selection group cases.  If this table has direct
   * action but this default entry does not use it then we don't want to
   * notify the action manager.  Check the default entry type here to identify
   * such cases. */
  bool uses_direct_adt =
      tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT;
  rc = pipe_mgr_tcam_action_allocate(tcam_tbl,
                                     tcam_entry,
                                     &tcam_loc,
                                     mat_data,
                                     logical_action_idx,
                                     logical_sel_idx,
                                     uses_direct_adt,
                                     false /*update*/,
                                     false /*is_recovery*/);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error allocating action data for default entry"
        " tbl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  if (!tcam_tbl_info->restore_ent_node) {
    int i;
    for (i = 0; i < unpack_mat_ent_data_as(mat_data)->resource_count; i++) {
      pipe_res_spec_t *rs = &unpack_mat_ent_data_as(mat_data)->resources[i];
      rc = resource_allocate(tcam_tbl,
                             tcam_entry,
                             rs,
                             tcam_loc.stage_id,
                             tcam_loc.stage_line_no,
                             PIPE_TCAM_OP_SET_DEFAULT,
                             mat_data);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }
  } else {
    pipe_mgr_restore_tcam_addr_node(tcam_tbl_info, tcam_entry);
  }

  rc = pipe_mgr_tcam_execute_default_ent_update(tcam_tbl, tcam_entry, mat_data);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error updating hardware for default entry"
        " tbl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  if (!tcam_tbl_info->restore_ent_node) {
    rc = pipe_mgr_tcam_idle_time_allocate(
        tcam_tbl, tcam_entry, &tcam_loc, mat_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error allocating idle entry for default entry add "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  tcam_pipe_tbl->llp.default_ent_set = true;
  tcam_pipe_tbl->llp.llp_default_tcam_entry = tcam_entry;

  return pipe_mgr_tcam_llp_entry_htbl_add(tcam_pipe_tbl, entry_hdl, tcam_entry);
}

static pipe_status_t pipe_mgr_tcam_write_atomic_mod_csr(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t stage_id,
    bool start) {
  if (tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Tofino doesn't have atomic mod registers, return */
    return PIPE_SUCCESS;
  }

  pipe_atomic_mod_csr_instr_t instr;
  construct_instr_atomic_mod_csr(
      tcam_tbl_info->dev_id, &instr, tcam_tbl_info->direction, start, true);
  return pipe_mgr_drv_ilist_add(&tcam_pipe_tbl->cur_sess_hdl,
                                tcam_tbl_info->dev_info,
                                &tcam_pipe_tbl->pipe_bmp,
                                stage_id,
                                (uint8_t *)&instr,
                                sizeof(pipe_atomic_mod_csr_instr_t));
}

static pipe_status_t pipe_mgr_tcam_write_atomic_mod_sram_reg(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t stage_id) {
  if (tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Tofino doesn't have atomic mod registers, return */
    return PIPE_SUCCESS;
  }

  uint32_t reg_addr = 0, reg_data = 0;
  pipe_instr_write_reg_i_only_t instr;

  switch (tcam_tbl_info->dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      reg_addr =
          offsetof(tof2_reg,
                   pipes[0]
                       .mau[stage_id]
                       .rams.match.adrdist
                       .atomic_mod_sram_go_pending[tcam_tbl_info->direction]);
      setp_tof2_atomic_mod_sram_go_pending_atomic_mod_sram_go_pending(&reg_data,
                                                                      1);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      reg_addr =
          offsetof(tof3_reg,
                   pipes[0]
                       .mau[stage_id]
                       .rams.match.adrdist
                       .atomic_mod_sram_go_pending[tcam_tbl_info->direction]);
      setp_tof3_atomic_mod_sram_go_pending_atomic_mod_sram_go_pending(&reg_data,
                                                                      1);
      break;



    default:
      return PIPE_SUCCESS;
  }

  construct_instr_reg_write_no_data(tcam_tbl_info->dev_id, &instr, reg_addr);
  return pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        stage_id,
                                        (uint8_t *)&instr,
                                        sizeof(pipe_instr_write_reg_i_only_t),
                                        (uint8_t *)&reg_data,
                                        sizeof(uint32_t));
}

static pipe_status_t pipe_mgr_tcam_reset_default_entry_int(
    tcam_pipe_tbl_t *tcam_pipe_tbl, tcam_phy_loc_info_t *tcam_loc, bool init) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  tcam_stage_info_t *stage_info =
      &tcam_pipe_tbl->stage_data[tcam_pipe_tbl->num_stages - 1];
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_mgr_action_entry_t *action_entry = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_hdl_type_t tbl_type;
  uint32_t indirect_action_ptr = 0, indirect_stats_ptr = 0;
  uint32_t indirect_meter_ptr = 0, indirect_stful_ptr = 0;
  uint32_t indirect_idle_ptr = 0;
  uint32_t i = 0;
  pipe_register_spec_t
      reg_spec_list[tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg];

  mat_tbl_info = pipe_mgr_get_tbl_info(
      tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Cannot find match table info for tcam tbl 0x%x device %d",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (mat_tbl_info->default_info) {
    action_entry = &mat_tbl_info->default_info->action_entry;
    act_fn_hdl = action_entry->act_fn_hdl;
    if (action_entry->num_act_data) {
      /* Create action spec */
      act_data_spec = PIPE_MGR_CALLOC(1, sizeof(pipe_action_data_spec_t));
      if (act_data_spec == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      rc = pipe_mgr_create_action_data_spec(
          action_entry->act_data, action_entry->num_act_data, act_data_spec);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Failed to create action data spec for default entry for tbl "
            "0x%x ",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        PIPE_MGR_FREE(act_data_spec);
        return rc;
      }
    }

    if (tcam_tbl_info->adt_present &&
        tcam_tbl_info->adt_tbl_ref.ref_type == PIPE_TBL_REF_TYPE_DIRECT &&
        tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
      rc = rmt_adt_ent_add(tcam_pipe_tbl->cur_sess_hdl,
                           tcam_tbl_info->dev_id,
                           tcam_tbl_info->adt_tbl_ref.tbl_hdl,
                           tcam_pipe_tbl->pipe_id,
                           tcam_loc->stage_id,
                           act_fn_hdl,
                           act_data_spec,
                           tcam_loc->stage_line_no,
                           stage_info->stage_table_handle,
                           &indirect_action_ptr,
                           !init);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error writing default action for tcam table 0x%x "
            "rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->tbl_hdl,
            rc);
        goto cleanup;
      }
    }

    /* Attach indirect resources */
    for (i = 0; i < mat_tbl_info->default_info->action_entry.num_ind_res; i++) {
      pipe_mgr_ind_res_info_t *ind_res = NULL;
      ind_res = &mat_tbl_info->default_info->action_entry.ind_res[i];
      tbl_type = PIPE_GET_HDL_TYPE(ind_res->handle);
      switch (tbl_type) {
        case PIPE_HDL_TYPE_STAT_TBL:
          rc = rmt_stat_mgr_stat_ent_attach(tcam_tbl_info->dev_id,
                                            tcam_pipe_tbl->pipe_id,
                                            tcam_loc->stage_id,
                                            ind_res->handle,
                                            ind_res->idx,
                                            &indirect_stats_ptr);
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          rc = rmt_meter_mgr_meter_attach(tcam_tbl_info->dev_id,
                                          tcam_pipe_tbl->pipe_id,
                                          tcam_loc->stage_id,
                                          ind_res->handle,
                                          ind_res->idx,
                                          &indirect_meter_ptr);
          break;
        case PIPE_HDL_TYPE_STFUL_TBL:
          rc = pipe_mgr_stful_get_indirect_ptr(tcam_tbl_info->dev_id,
                                               tcam_pipe_tbl->pipe_id,
                                               tcam_loc->stage_id,
                                               act_fn_hdl,
                                               ind_res->handle,
                                               ind_res->idx,
                                               &indirect_stful_ptr);
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          return PIPE_INVALID_ARG;
      }
    }
  }

  pipe_tbl_ref_t *tbl_ref =
      pipe_mgr_tcam_get_tbl_ref(tcam_tbl_info, PIPE_HDL_TYPE_METER_TBL);
  if (tbl_ref &&
      tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
    /* Add default meter spec if the table uses a direct meter */
    if (tbl_ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      pipe_meter_tbl_hdl_t meter_tbl_hdl = tbl_ref->tbl_hdl;
      pipe_meter_tbl_info_t *meter_tbl_info = pipe_mgr_get_meter_tbl_info(
          tcam_tbl_info->dev_id, meter_tbl_hdl, __func__, __LINE__);
      if (!meter_tbl_info) {
        LOG_ERROR(
            "%s:%d Failed to find meter info for direct meter 0x%x "
            "attached to tcam table 0x%x device %d",
            __func__,
            __LINE__,
            meter_tbl_hdl,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      if (meter_tbl_info->meter_type == PIPE_METER_TYPE_STANDARD) {
        pipe_meter_spec_t meter = {0};
        meter.meter_type = meter_tbl_info->enable_color_aware
                               ? METER_TYPE_COLOR_AWARE
                               : METER_TYPE_COLOR_UNAWARE;
        switch (meter_tbl_info->meter_granularity) {
          case PIPE_METER_GRANULARITY_BYTES:
            meter.cir.type = METER_RATE_TYPE_KBPS;
            meter.cir.value.kbps = ~0ull;
            meter.pir.type = METER_RATE_TYPE_KBPS;
            meter.pir.value.kbps = ~0ull;
            break;
          case PIPE_METER_GRANULARITY_PACKETS:
            meter.cir.type = METER_RATE_TYPE_PPS;
            meter.cir.value.pps = ~0ull;
            meter.pir.type = METER_RATE_TYPE_PPS;
            meter.pir.value.pps = ~0ull;
            break;
          default:
            LOG_ERROR(
                "%s:%d Invalid meter granularity type %d for meter tbl 0x%x "
                "attached to tcam tbl 0x%x device %d",
                __func__,
                __LINE__,
                meter_tbl_info->meter_granularity,
                meter_tbl_hdl,
                tcam_tbl_info->tbl_hdl,
                tcam_tbl_info->dev_id);
            return PIPE_UNEXPECTED;
        }
        meter.cburst = ~0ull;
        meter.pburst = ~0ull;
        rmt_meter_mgr_direct_meter_attach(tcam_pipe_tbl->cur_sess_hdl,
                                          tcam_tbl_info->dev_id,
                                          meter_tbl_hdl,
                                          tcam_loc->stage_line_no,
                                          tcam_pipe_tbl->pipe_id,
                                          tcam_loc->stage_id,
                                          &meter,
                                          &indirect_meter_ptr);
      }
    }

    if (tbl_ref->color_mapram_addr_type == COLOR_MAPRAM_ADDR_TYPE_IDLE) {
      indirect_idle_ptr = indirect_meter_ptr >> 3;
    } else if (tbl_ref->color_mapram_addr_type ==
               COLOR_MAPRAM_ADDR_TYPE_STATS) {
      indirect_stats_ptr = indirect_meter_ptr >> 4;
    }
  }

  pipe_mgr_entry_format_tbl_default_entry_update(tcam_tbl_info->dev_info,
                                                 tcam_tbl_info->direction,
                                                 tcam_tbl_info->profile_id,
                                                 tcam_tbl_info->tbl_hdl,
                                                 act_fn_hdl,
                                                 act_data_spec,
                                                 indirect_action_ptr,
                                                 indirect_stats_ptr,
                                                 indirect_meter_ptr,
                                                 indirect_stful_ptr,
                                                 indirect_idle_ptr,
                                                 0,
                                                 0,
                                                 reg_spec_list);

  /* Update registers atomically if we are not in any sort of init phase */
  if (!init) {
    rc = pipe_mgr_tcam_write_atomic_mod_csr(
        tcam_tbl_info, tcam_pipe_tbl, tcam_loc->stage_id, true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod csr start for tcam tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          tcam_tbl_info->dev_id);
      goto cleanup;
    }
    rc = pipe_mgr_tcam_write_atomic_mod_sram_reg(
        tcam_tbl_info, tcam_pipe_tbl, tcam_loc->stage_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod sram reg for tcam tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          tcam_tbl_info->dev_id);
      goto cleanup;
    }
  }
  for (i = 0; i < tcam_tbl_info->dev_info->dev_cfg.num_dflt_reg; i++) {
    pipe_instr_write_reg_i_only_t instr;
    LOG_TRACE("TCAM %s Default register addr 0x%x value 0x%x",
              tcam_tbl_info->name,
              reg_spec_list[i].reg_addr & 0x7ffff,
              reg_spec_list[i].reg_data);
    construct_instr_reg_write_no_data(
        tcam_tbl_info->dev_id, &instr, reg_spec_list[i].reg_addr);
    reg_spec_list[i].reg_data = htole32(reg_spec_list[i].reg_data);
    rc = pipe_mgr_tcam_post_instruction(tcam_tbl_info,
                                        &tcam_pipe_tbl->cur_sess_hdl,
                                        &tcam_pipe_tbl->pipe_bmp,
                                        tcam_loc->stage_id,
                                        (uint8_t *)&instr,
                                        sizeof(pipe_instr_write_reg_i_only_t),
                                        (uint8_t *)&reg_spec_list[i].reg_data,
                                        sizeof(uint32_t));
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Instruction add failed 0x%x", __func__, __LINE__, rc);
      goto cleanup;
    }
  }
  if (!init) {
    rc = pipe_mgr_tcam_write_atomic_mod_csr(
        tcam_tbl_info, tcam_pipe_tbl, tcam_loc->stage_id, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod csr end for tcam tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          tcam_tbl_info->dev_id);
      goto cleanup;
    }
  }

cleanup:
  if (act_data_spec) {
    if (act_data_spec->action_data_bits) {
      PIPE_MGR_FREE(act_data_spec->action_data_bits);
    }
    PIPE_MGR_FREE(act_data_spec);
  }
  return rc;
}

pipe_status_t pipe_mgr_tcam_reset_default_entry(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t tbl_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_t *tcam_tbl = NULL;
  tcam_phy_loc_info_t tcam_loc;
  uint32_t i = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Could not get the TCAM match table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];
    tcam_pipe_tbl->cur_sess_hdl = sess_hdl;
    tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, 0);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
        tcam_tbl, tcam_tbl->total_entries - 1, true, &tcam_loc);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the physical location info for default entry "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    rc = pipe_mgr_tcam_reset_default_entry_int(tcam_pipe_tbl, &tcam_loc, true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing default reset for table 0x%x pipe %d device "
          "%d\n",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_pipe_tbl->pipe_id,
          tcam_tbl_info->dev_id);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_default_ent_reset(
    tcam_tbl_t *tcam_tbl, tcam_llp_entry_t *tcam_entry) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);

  tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);

  if (tcam_entry == NULL || !tcam_entry->is_default) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
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

  rc = pipe_mgr_tcam_lock_stage(tcam_tbl, &tcam_loc, true);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error locking the table rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_idle_time_deallocate(tcam_tbl, tcam_entry, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deallocating idle entry for default entry delete "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  rc = pipe_mgr_tcam_action_deallocate(tcam_tbl, tcam_entry, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error deallocating action data for default entry"
        " tbl 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  uint32_t i;
  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    rc = resource_deallocate(tcam_tbl,
                             tcam_entry,
                             tcam_tbl_info->tbl_refs[i].tbl_hdl,
                             PIPE_TCAM_OP_CLEAR_DEFAULT,
                             tcam_loc.stage_id,
                             tcam_loc.stage_line_no);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  rc = pipe_mgr_tcam_unlock_all_stages(tcam_pipe_tbl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error unlocking the table rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
  }

  rc = pipe_mgr_tcam_reset_default_entry_int(
      tcam_pipe_tbl,
      &tcam_loc,
      pipe_mgr_is_device_locked(tcam_tbl_info->dev_id));
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error writing default reset for table 0x%x pipe %d device %d\n",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_pipe_tbl->pipe_id,
        tcam_tbl_info->dev_id);
    return rc;
  }

  if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
    uint32_t index = tcam_tbl->total_entries - 1;
    PIPE_MGR_DBGCHK(tcam_tbl->llp.tcam_entries[index] == tcam_entry);
    tcam_tbl->llp.tcam_entries[index] = NULL;
  }

  tcam_pipe_tbl->llp.default_ent_set = false;
  tcam_pipe_tbl->llp.llp_default_tcam_entry = NULL;

  pipe_mgr_tcam_llp_entry_htbl_delete(tcam_pipe_tbl, tcam_entry->entry_hdl);
  pipe_mgr_tcam_llp_entry_destroy(tcam_entry);

  return PIPE_SUCCESS;
}

static pipe_status_t action_update(tcam_tbl_t *tcam_tbl,
                                   tcam_llp_entry_t *tcam_entry,
                                   tcam_phy_loc_info_t *tcam_loc,
                                   struct pipe_mgr_mat_data *mat_data,
                                   pipe_idx_t logical_action_idx,
                                   pipe_idx_t logical_sel_idx) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_tcam_action_deallocate(tcam_tbl, tcam_entry, tcam_loc);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  bool uses_direct_adt = true;

  /* It is possible that table uses direct ADT, but default entry does not.
     Good example is const default entry, which can be updated, but action
     itself will not change and can be empty. In such case we need to correct
     adt usage variable, because default entry is using the same flow as any
     entry modify call. */
  if (tcam_entry->is_default && tcam_tbl->tcam_pipe_tbl_p->default_ent_type ==
                                    TCAM_DEFAULT_ENT_TYPE_INDIRECT) {
    uses_direct_adt = false;
  }

  rc = pipe_mgr_tcam_action_allocate(tcam_tbl,
                                     tcam_entry,
                                     tcam_loc,
                                     mat_data,
                                     logical_action_idx,
                                     logical_sel_idx,
                                     uses_direct_adt,
                                     true /*update*/,
                                     false /*is_recovery*/);
  return rc;
}

static pipe_status_t resources_update(tcam_tbl_t *tcam_tbl,
                                      tcam_llp_entry_t *tcam_entry,
                                      tcam_phy_loc_info_t *tcam_loc,
                                      pipe_tcam_op_e tcam_op,
                                      struct pipe_mgr_mat_data *mat_data) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_res_spec_t *resources = unpack_mat_ent_data_as(mat_data)->resources;
  int resource_count = unpack_mat_ent_data_as(mat_data)->resource_count;

  uint32_t tbl;
  for (tbl = 0; tbl < tcam_tbl_info->num_tbl_refs; tbl++) {
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->tbl_refs[tbl];
    rc = resource_deallocate(tcam_tbl,
                             tcam_entry,
                             tbl_ref->tbl_hdl,
                             tcam_op,
                             tcam_loc->stage_id,
                             tcam_loc->stage_line_no);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  int i;
  for (i = 0; i < resource_count; i++) {
    pipe_res_spec_t *rs = &resources[i];
    if (rs->tag == PIPE_RES_ACTION_TAG_ATTACHED) {
      rc = resource_allocate(tcam_tbl,
                             tcam_entry,
                             rs,
                             tcam_loc->stage_id,
                             tcam_loc->stage_line_no,
                             tcam_op,
                             mat_data);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_execute_action_update(
    tcam_tbl_t *tcam_tbl,
    tcam_llp_entry_t *tcam_entry,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  bool tind_update_needed = true;
  tcam_indirect_addr_t old_addr;
  pipe_status_t rc = PIPE_SUCCESS;

  old_addr = tcam_entry->addr;

  tcam_phy_loc_info_t tcam_loc;
  rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, tcam_entry->index, tcam_entry->is_default, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  rc = action_update(tcam_tbl,
                     tcam_entry,
                     &tcam_loc,
                     mat_data,
                     logical_action_idx,
                     logical_sel_idx);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error updating action for entry 0x%x index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  /* Now update the resources */
  rc = resources_update(tcam_tbl, tcam_entry, &tcam_loc, tcam_op, mat_data);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error updating resources for entry 0x%x index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        tcam_entry->index,
        rc);
    return rc;
  }

  if (PIPE_MGR_MEMCMP(
          &old_addr, &tcam_entry->addr, sizeof(tcam_indirect_addr_t))) {
    tind_update_needed = true;
  }

  if (tind_update_needed) {
    if (!tcam_entry->is_default) {
      rc = pipe_mgr_tcam_update_hw(tcam_tbl,
                                   NULL /*src_loc*/,
                                   &tcam_loc,
                                   PIPE_TCAM_OP_REFRESH_TIND,
                                   match_specs,
                                   mat_data,
                                   logical_action_idx,
                                   logical_sel_idx,
                                   false /*is_recovery*/);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(%x)Error updating action data for tcam entry 0x%x rc "
            "0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->tbl_hdl,
            tcam_entry->index,
            rc);
        return rc;
      }
    } else {
      /* Default action modify, requires atomic csr modify */
      rc = pipe_mgr_tcam_write_atomic_mod_csr(
          tcam_tbl_info, tcam_pipe_tbl, tcam_loc.stage_id, true);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      /* Csr atomic modify requires atomic sram updates through a register
       * write. Note that this must be the first csr to be written after
       * the atomic start.
       */
      rc = pipe_mgr_tcam_write_atomic_mod_sram_reg(
          tcam_tbl_info, tcam_pipe_tbl, tcam_loc.stage_id);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      rc = pipe_mgr_tcam_execute_default_ent_update(
          tcam_tbl, tcam_entry, mat_data);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error updating default entry rc 0x%x",
                  __func__,
                  __LINE__,
                  rc);
        return rc;
      }
      rc = pipe_mgr_tcam_write_atomic_mod_csr(
          tcam_tbl_info, tcam_pipe_tbl, tcam_loc.stage_id, false);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}

/* dest_index is the valid index in case when only one index is present */
static pipe_status_t pipe_mgr_tcam_update_hw(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *src_loc,
    tcam_phy_loc_info_t *dest_loc,
    pipe_tcam_op_e tcam_op,
    pipe_tbl_match_spec_t **match_specs,
    struct pipe_mgr_mat_data *mat_data,
    pipe_idx_t logical_action_idx,
    pipe_idx_t logical_sel_idx,
    bool is_recovery) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_move_type_e current_move_type = TCAM_MOVE_INVALID;
  tcam_prev_move_t *prev_move = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_llp_entry_t *tcam_entry = NULL;
  uint32_t src_index, dest_index;
  bool mov_mod = false;

  if (!dest_loc) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (tcam_op == PIPE_TCAM_OP_MODIFY && src_loc) {
    mov_mod = true;
    tcam_op = PIPE_TCAM_OP_MOVE;
  }

  if (tcam_op == PIPE_TCAM_OP_MOVE && !src_loc) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  dest_index = dest_loc->index;

  src_index = src_loc ? src_loc->index : PIPE_MGR_TCAM_INVALID_IDX;

  prev_move = &tcam_tbl->llp.prev_move_cookie;
  tcam_entry = tcam_tbl->llp.tcam_entries[dest_index];
  if (tcam_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  switch (tcam_op) {
    case PIPE_TCAM_OP_ALLOCATE:
    case PIPE_TCAM_OP_DELETE:
    case PIPE_TCAM_OP_UPDATE_VERSION:
    case PIPE_TCAM_OP_REFRESH_TIND:
      current_move_type = TCAM_MOVE_INVALID;
      break;
    case PIPE_TCAM_OP_MOVE:
      PIPE_MGR_DBGCHK(src_index != PIPE_MGR_TCAM_INVALID_IDX);
      current_move_type =
          (src_index > dest_index) ? TCAM_MOVE_UP : TCAM_MOVE_DOWN;
      break;
    default:
      return PIPE_SUCCESS;
  }

  /* If the currently locked stage_id is not the stage_id we're acting
   * on and if this operation needs locking, do an unlock on the
   * currently locked stage_id and lock the stage_id we're acting on
   *
   * Don't lock stats table if we are doing an interstage move, as we need
   * to move the counter through software by doing a dump.
   */
  bool lock_stats = (!src_loc || (src_loc->stage_id == dest_loc->stage_id));
  if ((tcam_tbl_info->lock_type != LOCK_ID_TYPE_INVALID) &&
      pipe_mgr_tcam_op_needs_locking(tcam_op, prev_move->prev_move_type)) {
    /* Lock the dest stage_id */
    rc = pipe_mgr_tcam_lock_stage(tcam_tbl, dest_loc, lock_stats);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error locking the table rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    if (src_index != PIPE_MGR_TCAM_INVALID_IDX) {
      /* Lock the source stage_id */
      rc = pipe_mgr_tcam_lock_stage(tcam_tbl, src_loc, lock_stats);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error locking the table rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
    }
  }

  if (mov_mod) {
    /* For move-modify cases we modify the entry during a move to atomically
     * update the TIND and ADT together.  However a direct stateful resource
     * cannot be written during the move, the move-reg logic will atomically
     * copy the current stateful spec from the source to the destination so we
     * will first update the stateful spec and then perform the move so that the
     * new spec is copied to the entries new location. */
    for (int i = 0; i < unpack_mat_ent_data_as(mat_data)->resource_count; i++) {
      pipe_res_spec_t *rs = &unpack_mat_ent_data_as(mat_data)->resources[i];
      /* Look for an attached, direct, stateful resource. */
      if (rs->tag != PIPE_RES_ACTION_TAG_ATTACHED) continue;
      pipe_tbl_ref_t *tbl_ref = get_tbl_ref(tcam_tbl_info, rs->tbl_hdl);
      if (!tbl_ref) {
        LOG_ERROR("%s:%d -  %s (%d - 0x%x) Error in getting table ref",
                  __func__,
                  __LINE__,
                  tcam_tbl_info->name,
                  tcam_tbl_info->dev_id,
                  tcam_tbl_info->tbl_hdl);
        return PIPE_UNEXPECTED;
      }

      if (tbl_ref->ref_type != PIPE_TBL_REF_TYPE_DIRECT) continue;
      pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(rs->tbl_hdl);
      if (tbl_type != PIPE_HDL_TYPE_STFUL_TBL) continue;
      /* If the sequence number has changed issue the write and update our
       * current sequence number. */
      if (tcam_entry->direct_stful_seq_nu !=
          unpack_mat_ent_data_stful(mat_data)) {
        tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
        rc = pipe_mgr_stful_direct_word_write_at(tcam_pipe_tbl->cur_sess_hdl,
                                                 tcam_tbl_info->dev_id,
                                                 tcam_tbl_info->tbl_hdl,
                                                 tcam_entry->entry_hdl,
                                                 tcam_pipe_tbl->pipe_id,
                                                 src_loc->stage_id,
                                                 src_loc->stage_line_no,
                                                 &rs->data.stful,
                                                 tcam_pipe_tbl->sess_flags);
        if (PIPE_SUCCESS == rc) {
          pipe_act_fn_hdl_t act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
          uint32_t x = 0;
          rc = pipe_mgr_stful_get_indirect_ptr(tcam_tbl_info->dev_id,
                                               tcam_pipe_tbl->pipe_id,
                                               src_loc->stage_id,
                                               act_fn_hdl,
                                               rs->tbl_hdl,
                                               src_loc->stage_line_no,
                                               &x);
          if (PIPE_SUCCESS == rc) {
            tcam_entry->addr.indirect_addr_stful = x;
          }
        }
        if (PIPE_SUCCESS != rc) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) Error updating direct stateful, rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              rc);
          return rc;
        }
      }
    }
  }
  rc = pipe_mgr_tcam_setup_moveregs(tcam_tbl, src_loc, dest_loc, tcam_op);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error setting up moveregs rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  switch (tcam_op) {
    case PIPE_TCAM_OP_ALLOCATE:
      rc = pipe_mgr_tcam_execute_add_entry(tcam_tbl,
                                           tcam_entry,
                                           dest_loc,
                                           tcam_op,
                                           match_specs,
                                           mat_data,
                                           logical_action_idx,
                                           logical_sel_idx,
                                           is_recovery);
      break;
    case PIPE_TCAM_OP_REFRESH_TIND:
    case PIPE_TCAM_OP_UPDATE_VERSION:
      /* Update entry */
      rc = pipe_mgr_tcam_execute_add_update_entry(tcam_tbl,
                                                  tcam_entry,
                                                  dest_loc,
                                                  tcam_op,
                                                  false,
                                                  match_specs,
                                                  mat_data);
      break;
    case PIPE_TCAM_OP_DELETE:
      rc = pipe_mgr_tcam_execute_delete_entry(
          tcam_tbl, tcam_entry, tcam_op, dest_loc);
      break;
    case PIPE_TCAM_OP_MOVE:
      if (tcam_entry->is_default) {
        // Default entries never move in TCAM.
        LOG_ERROR(
            "%s:%d Error updating hardware - default entry move not supported.",
            __func__,
            __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      if (current_move_type == TCAM_MOVE_UP) {
        rc = pipe_mgr_tcam_execute_move_entry_up(tcam_tbl,
                                                 tcam_entry,
                                                 src_loc,
                                                 dest_loc,
                                                 tcam_op,
                                                 match_specs,
                                                 mat_data,
                                                 logical_action_idx,
                                                 logical_sel_idx,
                                                 mov_mod);
      } else {
        rc = pipe_mgr_tcam_execute_move_entry_down(tcam_tbl,
                                                   tcam_entry,
                                                   src_loc,
                                                   dest_loc,
                                                   tcam_op,
                                                   match_specs,
                                                   mat_data,
                                                   logical_action_idx,
                                                   logical_sel_idx,
                                                   mov_mod);
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error updating hardware for %s dest_index %d rc 0x%x",
              __func__,
              __LINE__,
              (tcam_op == PIPE_TCAM_OP_ALLOCATE) ? "add" : "move",
              dest_index,
              rc);
    return rc;
  }

  rc = pipe_mgr_tcam_teardown_moveregs(tcam_tbl, src_loc, dest_loc, tcam_op);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error tearing down moveregs rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  prev_move->prev_move_type = current_move_type;
  prev_move->src_index = src_index;
  prev_move->dest_index = dest_index;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_validate_placement_op(
    pipe_mgr_move_list_t *move_node) {
  (void)move_node;
  return PIPE_SUCCESS;
}

static tcam_llp_entry_t *tcam_llp_entry_create(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    pipe_mgr_move_list_t *move_node,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info) {
  uint32_t entries_per_block, no_blocks;

  bool is_range = move_node->op == PIPE_MAT_UPDATE_ADD_MULTI ? true : false;

  if (is_range) {
    no_blocks = move_node->u.multi.array_sz;
    entries_per_block = move_node->u.multi.locations[0].logical_index_count;
  } else {
    no_blocks = 1;
    entries_per_block = 1;
  }

  pipe_ent_hdl_t entry_hdl = move_node->entry_hdl;
  tcam_llp_entry_t *head_entry = NULL;

  uint32_t ptn;
  if (is_range) {
    ptn = get_ptn_from_logical_index(
        tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
  } else {
    ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                     move_node->u.single.logical_idx);
  }

  PIPE_MGR_DBGCHK(ptn < tcam_pipe_tbl->no_ptns);
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return NULL;
  }

  uint32_t subentry_index = 0;
  /* Create duplicate TCAM entries */
  uint32_t i;
  for (i = 0; i < no_blocks; i++, subentry_index++) {
    uint32_t index;
    if (is_range) {
      index = get_index_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[i].logical_index_base);
      ptn = get_ptn_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[i].logical_index_base);
    } else {
      index = get_index_from_logical_index(tcam_pipe_tbl,
                                           move_node->u.single.logical_idx);
      ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                       move_node->u.single.logical_idx);
    }

    PIPE_MGR_DBGCHK(ptn == tcam_tbl->ptn_index);

    tcam_llp_entry_t *tcam_entry;
    tcam_entry = pipe_mgr_tcam_llp_entry_allocate();
    if (!tcam_entry) {
      return NULL;
    }

    tcam_entry->range_list_p = &tcam_entry->range_list;
    tcam_entry->subentry_index = subentry_index;

    tcam_entry->pipe_id = tcam_pipe_tbl->pipe_id;
    tcam_entry->ptn_index = tcam_tbl->ptn_index;
    tcam_entry->index = index;
    tcam_entry->range_count = entries_per_block;
    tcam_entry->entry_hdl = entry_hdl;

    BF_LIST_DLL_AP(
        *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);

    tcam_tbl->llp.tcam_entries[index] = tcam_entry;
    tcam_tbl->llp.total_hw_usage++;

    uint32_t j;
    for (j = 1; j < entries_per_block; j++) {
      tcam_llp_entry_t *range_entry = pipe_mgr_tcam_llp_entry_allocate();
      if (!range_entry) {
        return NULL;
      }

      range_entry->range_list_p = tcam_entry->range_list_p;
      range_entry->subentry_index =
          no_blocks + (j - 1) + (subentry_index * (entries_per_block - 1));

      range_entry->pipe_id = tcam_pipe_tbl->pipe_id;
      range_entry->ptn_index = tcam_tbl->ptn_index;
      range_entry->index = index + j;
      range_entry->range_count = entries_per_block;
      range_entry->entry_hdl = entry_hdl;

      BF_LIST_DLL_AP(
          *(range_entry->range_list_p), range_entry, next_range, prev_range);

      tcam_tbl->llp.tcam_entries[index + j] = range_entry;
      tcam_tbl->llp.total_hw_usage++;
    }
    BF_LIST_DLL_AP(head_entry, tcam_entry, next, prev);
  }

  if (head_entry && indirect_ptrs) {
    // HA case: restore state saved during pipe_mgr_tcam_update_hw

    pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(move_node->data);
    head_entry->addr.indirect_addr_action = indirect_ptrs->adt_ptr;
    if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
      head_entry->adt_ent_hdl = action_spec->adt_ent_hdl;
      head_entry->logical_action_idx = move_node->logical_action_idx;
    } else if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
      head_entry->sel_grp_hdl = action_spec->sel_grp_hdl;
      head_entry->addr.indirect_addr_sel = indirect_ptrs->sel_ptr;
      head_entry->addr.sel_grp_pvl = indirect_ptrs->sel_len;
      head_entry->logical_action_idx = move_node->logical_action_idx;
      head_entry->logical_sel_idx = move_node->logical_sel_idx;
    }

    head_entry->addr.indirect_addr_stats =
        ptr_info->force_stats ? 0 : indirect_ptrs->stats_ptr;
    head_entry->addr.indirect_addr_meter =
        ptr_info->force_meter ? 0 : indirect_ptrs->meter_ptr;
    head_entry->addr.indirect_addr_stful =
        ptr_info->force_stful ? 0 : indirect_ptrs->stfl_ptr;
  }

  return head_entry;
}

pipe_status_t pipe_mgr_tcam_process_allocate(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    pipe_tbl_match_spec_t **match_specs,
    pipe_mgr_move_list_t *move_node,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;

  pipe_ent_hdl_t entry_hdl = move_node->entry_hdl;
  tcam_llp_entry_t *head_entry =
      tcam_llp_entry_create(tcam_pipe_tbl, move_node, indirect_ptrs, ptr_info);
  if (!head_entry) {
    return PIPE_NO_SYS_RESOURCES;
  }

  bool is_range = move_node->op == PIPE_MAT_UPDATE_ADD_MULTI ? true : false;
  uint32_t ptn;
  if (is_range) {
    ptn = get_ptn_from_logical_index(
        tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
  } else {
    ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                     move_node->u.single.logical_idx);
  }

  PIPE_MGR_DBGCHK(ptn < tcam_pipe_tbl->no_ptns);
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  if (!indirect_ptrs) {
    // No indirect ptrs passed in, so we need to update hardware

    tcam_llp_entry_t *range_head;
    FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
      tcam_phy_loc_info_t dest_loc;
      rc = pipe_mgr_tcam_get_phy_loc_info(
          tcam_tbl, range_head->index, &dest_loc);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }

      rc = pipe_mgr_tcam_update_hw(tcam_tbl,
                                   NULL /*src_loc*/,
                                   &dest_loc,
                                   PIPE_TCAM_OP_ALLOCATE,
                                   match_specs,
                                   move_node->data,
                                   move_node->logical_action_idx,
                                   move_node->logical_sel_idx,
                                   false /*is_recovery*/);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error processing the placement add for entry 0x%x rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            entry_hdl,
            rc);
        return rc;
      }
    }
    FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();
  }

  tcam_tbl->llp.total_usage++;
  return pipe_mgr_tcam_llp_entry_htbl_add(tcam_pipe_tbl, entry_hdl, head_entry);
}

static pipe_status_t pipe_mgr_tcam_process_delete(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *head_entry,
    pipe_mgr_move_list_t *move_node) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_ent_hdl_t entry_hdl = move_node->entry_hdl;

  tcam_llp_entry_t *tcam_entry;
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    uint32_t index = tcam_entry->index;

    PIPE_MGR_DBGCHK(tcam_entry->ptn_index == tcam_tbl->ptn_index);

    tcam_phy_loc_info_t dest_loc;
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, index, &dest_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }

    rc = pipe_mgr_tcam_update_hw(tcam_tbl,
                                 NULL /*src_loc*/,
                                 &dest_loc,
                                 PIPE_TCAM_OP_DELETE,
                                 NULL /*m_specs*/,
                                 NULL /*m_data*/,
                                 0 /*act_idx*/,
                                 0 /*sel_idx*/,
                                 false /*is_recovery*/);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error processing the placement add for entry 0x%x rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          entry_hdl,
          rc);
      return rc;
    }
    tcam_tbl->llp.tcam_entries[index] = NULL;
  }
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();

  pipe_mgr_tcam_llp_entry_htbl_delete(tcam_pipe_tbl, entry_hdl);

  FOR_ALL_TCAM_LLP_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    uint32_t index = tcam_entry->index;
    tcam_tbl->llp.tcam_entries[index] = NULL;
    pipe_mgr_tcam_llp_entry_destroy(tcam_entry);
    tcam_tbl->llp.total_hw_usage--;
  }
  FOR_ALL_TCAM_LLP_ENTRIES_BLOCK_END();

  PIPE_MGR_DBGCHK(tcam_tbl->llp.total_usage);
  tcam_tbl->llp.total_usage--;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_llp_move_helper(
    tcam_tbl_t *tcam_tbl,
    uint32_t dest_index,
    uint32_t src_index,
    tcam_llp_entry_t **move_list_p) {
  tcam_llp_entry_t *src_entry = NULL;
  src_entry = tcam_tbl->llp.tcam_entries[src_index];
  if (src_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_tcam_entry_index_backup_one(tcam_tbl, dest_index);

  BF_LIST_DLL_REM(
      *(src_entry->range_list_p), src_entry, next_range, prev_range);
  if (move_list_p) {
    BF_LIST_DLL_AP(*move_list_p, src_entry, next_range, prev_range);
  }

  PIPE_MGR_DBGCHK(tcam_tbl->llp.tcam_entries[dest_index] == NULL);

  tcam_tbl->llp.tcam_entries[dest_index] = src_entry;
  src_entry->index = dest_index;

  tcam_tbl->llp.tcam_entries[src_index] = NULL;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_process_move(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *head_entry,
    pipe_tbl_match_spec_t **match_specs,
    pipe_mgr_move_list_t *move_node) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_ent_hdl_t entry_hdl = move_node->entry_hdl;

  bool is_range;
  is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info) ? true : false;
  pipe_tcam_op_e tcam_op;
  if (move_node->op == PIPE_MAT_UPDATE_MOV ||
      move_node->op == PIPE_MAT_UPDATE_MOV_MULTI) {
    tcam_op = PIPE_TCAM_OP_MOVE;
  } else {
    PIPE_MGR_DBGCHK(PIPE_MAT_UPDATE_MOV_MOD || PIPE_MAT_UPDATE_MOV_MULTI_MOD);
    tcam_op = PIPE_TCAM_OP_MODIFY;
  }

  uint32_t ptn;
  if (is_range) {
    ptn = get_ptn_from_logical_index(
        tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
  } else {
    ptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                     move_node->u.single.logical_idx);
  }

  PIPE_MGR_DBGCHK(ptn < tcam_pipe_tbl->no_ptns);
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  tcam_llp_entry_t *tcam_entry;
  uint32_t i = 0;
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    uint32_t src_index;

    uint32_t dest_index, dptn;
    if (is_range) {
      dest_index = get_index_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[i].logical_index_base);
      dptn = get_ptn_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[i].logical_index_base);
    } else {
      dest_index = get_index_from_logical_index(
          tcam_pipe_tbl, move_node->u.single.logical_idx);
      dptn = get_ptn_from_logical_index(tcam_pipe_tbl,
                                        move_node->u.single.logical_idx);
    }
    i++;

    PIPE_MGR_DBGCHK(dptn == tcam_tbl->ptn_index);

    src_index = tcam_entry->index;

    if (src_index == dest_index) {
      /* This entry has not moved */
      continue;
    }

    uint32_t tail_index = 0, dest_tail_index;
    uint32_t move_start_index, move_end_index, dest_move_index;
    uint32_t move_index;

    PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));

    TCAM_LLP_GET_RANGE_TAIL_INDEX(tcam_entry, tail_index);

    dest_tail_index = dest_index + tail_index - src_index;

    bool down_overlap = false, up_overlap = false;
    if ((dest_index > src_index) && (dest_index <= tail_index)) {
      /* Move down and overlap. move_end_index is non-inclusive */
      move_start_index = src_index + 1;
      move_end_index = dest_index + 1;
      dest_move_index = tail_index + 1;
      down_overlap = true;
    } else if ((dest_tail_index >= src_index) &&
               (dest_tail_index < tail_index)) {
      /* Move up and overlap */
      move_start_index = dest_tail_index + 1;
      move_end_index = tail_index + 1;
      dest_move_index = dest_index + 1;
      up_overlap = true;
    } else {
      move_start_index = src_index;
      move_end_index = tail_index + 1;
      dest_move_index = dest_index;
    }

    tcam_llp_entry_t *move_list = NULL;

    if (up_overlap) {
      /* Move the head entry first */
      rc = pipe_mgr_tcam_llp_move_helper(tcam_tbl, dest_index, src_index, NULL);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    /* Move steps here and in tcam.c need to be in sync */
    for (move_index = move_start_index; move_index < move_end_index;
         move_index++, dest_move_index++) {
      rc = pipe_mgr_tcam_llp_move_helper(
          tcam_tbl, dest_move_index, move_index, &move_list);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    if (down_overlap) {
      rc = pipe_mgr_tcam_llp_move_helper(tcam_tbl, dest_index, src_index, NULL);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }

    if (dest_index < src_index) {
      if (up_overlap) {
        BF_LIST_DLL_PP(move_list, tcam_entry, next_range, prev_range);
      }
      /* Move up */
      BF_LIST_DLL_CAT(
          move_list, *(tcam_entry->range_list_p), next_range, prev_range);
      *(tcam_entry->range_list_p) = move_list;
    } else {
      if (down_overlap) {
        BF_LIST_DLL_PP(
            *(tcam_entry->range_list_p), tcam_entry, next_range, prev_range);
      }
      BF_LIST_DLL_CAT(
          *(tcam_entry->range_list_p), move_list, next_range, prev_range);
    }

    PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));

    tcam_phy_loc_info_t dest_loc;
    tcam_phy_loc_info_t src_loc;
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, src_index, &src_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, dest_index, &dest_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }

    rc = pipe_mgr_tcam_update_hw(tcam_tbl,
                                 &src_loc,
                                 &dest_loc,
                                 tcam_op,
                                 match_specs,
                                 move_node->data,
                                 move_node->logical_action_idx,
                                 move_node->logical_sel_idx,
                                 false /*is_recovery*/);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error processing the placement add for entry 0x%x rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          entry_hdl,
          rc);
      return rc;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();

  /* Clear the prev move cookie if the move chain ends here */
  if (move_node->next == NULL) {
    PIPE_MGR_MEMSET(
        &tcam_tbl->llp.prev_move_cookie, 0, sizeof(tcam_prev_move_t));
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tcam_write_atomic_mod_sram_instr(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    uint32_t stage_id) {
  if (tcam_tbl_info->dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Tofino doesn't have atomic mod registers, return */
    return PIPE_SUCCESS;
  }

  pipe_atomic_mod_sram_instr_t instr;
  construct_instr_atomic_mod_sram(
      tcam_tbl_info->dev_id, &instr, tcam_tbl_info->direction);
  return pipe_mgr_drv_ilist_add(&tcam_pipe_tbl->cur_sess_hdl,
                                tcam_tbl_info->dev_info,
                                &tcam_pipe_tbl->pipe_bmp,
                                stage_id,
                                (uint8_t *)&instr,
                                sizeof(pipe_atomic_mod_sram_instr_t));
}

static pipe_status_t pipe_mgr_tcam_process_modify(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *head_entry,
    pipe_tbl_match_spec_t **match_specs,
    pipe_mgr_move_list_t *move_node) {
  pipe_status_t rc = PIPE_SUCCESS;

  struct pipe_mgr_mat_data *mat_data = move_node->data;
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_llp_entry_t *tcam_entry;
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    rc = pipe_mgr_tcam_execute_action_update(tcam_tbl,
                                             tcam_entry,
                                             PIPE_TCAM_OP_MODIFY,
                                             match_specs,
                                             mat_data,
                                             move_node->logical_action_idx,
                                             move_node->logical_sel_idx);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();

  if (!head_entry->is_default) {
    /* If this is a match entry modify, use the instr to modify sram.
     * Default entry modify would have used the csr instead.
     */
    tcam_phy_loc_info_t dest_loc;
    rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
        tcam_tbl, head_entry->index, head_entry->is_default, &dest_loc);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = pipe_mgr_tcam_write_atomic_mod_sram_instr(
        tcam_tbl_info, tcam_pipe_tbl, dest_loc.stage_id);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  return rc;
}

static pipe_status_t pipe_mgr_tcam_execute_add_idle(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *head_entry,
    pipe_mgr_move_list_t *move_node) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_SUCCESS;

  tcam_llp_entry_t *tcam_entry;
  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    tcam_phy_loc_info_t dest_loc;
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_entry->index, &dest_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    rc = pipe_mgr_tcam_idle_time_allocate(
        tcam_tbl, tcam_entry, &dest_loc, move_node->data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error allocating idle table entry for tcam entry 0x%x at index %d "
          " rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_entry->entry_hdl,
          tcam_entry->index,
          rc);
      return rc;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_END()
  return rc;
}

static pipe_status_t pipe_mgr_tcam_process_placement_op(
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_llp_entry_t *tcam_entry,
    pipe_mgr_move_list_t *move_node) {
  tcam_tbl_t *tcam_tbl;
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  pipe_status_t rc = PIPE_INVALID_ARG;

  bool use_match_spec = false, is_range;
  uint32_t entries_per_block, no_blocks;
  is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info) ? true : false;
  no_blocks = 1;
  entries_per_block = 1;
  if ((move_node->op == PIPE_MAT_UPDATE_ADD) ||
      (move_node->op == PIPE_MAT_UPDATE_MOV) ||
      (move_node->op == PIPE_MAT_UPDATE_MOV_MOD) ||
      (move_node->op == PIPE_MAT_UPDATE_ADD_MULTI) ||
      (move_node->op == PIPE_MAT_UPDATE_MOV_MULTI) ||
      (move_node->op == PIPE_MAT_UPDATE_MOV_MULTI_MOD)) {
    use_match_spec = true;
    if (is_range) {
      no_blocks = move_node->u.multi.array_sz;
      entries_per_block = move_node->u.multi.locations[0].logical_index_count;
    }
  } else if ((move_node->op == PIPE_MAT_UPDATE_MOD) &&
             !tcam_entry->is_default) {
    use_match_spec = true;
    /* Figure out the no_blocks and entries_per_block from existing entry */
    if (is_range) {
      tcam_llp_entry_t *head_entry;
      TCAM_LLP_GET_TCAM_HEAD(tcam_entry, head_entry);
      tcam_llp_entry_t *range_head;
      (void)range_head;
      no_blocks = 0;
      FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, range_head) {
        no_blocks++;
      }
      FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END();
      TCAM_LLP_GET_RANGE_ENTRY_COUNT(head_entry, entries_per_block);
    }
  }

  /* NOTE: VLA usage. C99 allows it. */
  pipe_tbl_match_spec_t *match_specs[no_blocks * entries_per_block];

  pipe_tbl_match_spec_t *orig_match_spec =
      use_match_spec ? unpack_mat_ent_data_ms(move_node->data) : NULL;
  pipe_tbl_match_spec_t espec[no_blocks][entries_per_block];
  uint8_t value_bits[no_blocks][entries_per_block]
                    [use_match_spec && orig_match_spec->num_match_bytes
                         ? orig_match_spec->num_match_bytes
                         : 1];
  uint8_t mask_bits[no_blocks][entries_per_block]
                   [use_match_spec && orig_match_spec->num_match_bytes
                        ? orig_match_spec->num_match_bytes
                        : 1];

  pipe_tbl_match_spec_t *expand_specs[no_blocks];

  if (use_match_spec) {
    if (is_range) {
      /* Expand the Match spec first */

      uint32_t i, j;
      for (i = 0; i < no_blocks; i++) {
        for (j = 0; j < entries_per_block; j++) {
          espec[i][j].match_value_bits = value_bits[i][j];
          espec[i][j].match_mask_bits = mask_bits[i][j];
          pipe_mgr_tbl_copy_match_spec(&espec[i][j], orig_match_spec);
        }
      }

      for (i = 0; i < no_blocks; i++) {
        expand_specs[i] = espec[i];
      }

      rc = pipe_mgr_entry_format_tof_range_expand(tcam_tbl_info->dev_id,
                                                  tcam_tbl_info->profile_id,
                                                  tcam_tbl_info->tbl_hdl,
                                                  orig_match_spec,
                                                  expand_specs,
                                                  no_blocks,
                                                  entries_per_block);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error expanding the TCAM range rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            rc);
        return rc;
      }

      for (i = 0; i < no_blocks; i++) {
        for (j = 0; j < entries_per_block; j++) {
          match_specs[(i * entries_per_block) + j] = &expand_specs[i][j];
          LOG_DBG("Expanded match spec for block %d entry %d", i, j);
          pipe_mgr_entry_format_log_match_spec(tcam_tbl_info->dev_id,
                                               BF_LOG_DBG,
                                               tcam_tbl_info->profile_id,
                                               tcam_tbl_info->tbl_hdl,
                                               &expand_specs[i][j]);
        }
      }
    } else {
      match_specs[0] = unpack_mat_ent_data_ms(move_node->data);
    }
  }

  switch (move_node->op) {
    case PIPE_MAT_UPDATE_ADD:
    /* Fall through */
    case PIPE_MAT_UPDATE_ADD_MULTI:
      rc = pipe_mgr_tcam_process_allocate(
          tcam_pipe_tbl, match_specs, move_node, NULL, NULL);
      break;
    case PIPE_MAT_UPDATE_SET_DFLT:
      rc = pipe_mgr_tcam_execute_default_ent_add(tcam_pipe_tbl, move_node);
      break;
    case PIPE_MAT_UPDATE_CLR_DFLT:
      tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, 0);
      if (tcam_tbl == NULL) {
        LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
        return PIPE_UNEXPECTED;
      }
      rc = pipe_mgr_tcam_execute_default_ent_reset(tcam_tbl, tcam_entry);
      break;
    case PIPE_MAT_UPDATE_DEL:
      rc = pipe_mgr_tcam_process_delete(tcam_pipe_tbl, tcam_entry, move_node);
      break;
    case PIPE_MAT_UPDATE_MOD:
      rc = pipe_mgr_tcam_process_modify(
          tcam_pipe_tbl, tcam_entry, match_specs, move_node);
      break;
    case PIPE_MAT_UPDATE_MOV:
    /* Fall through */
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      rc = pipe_mgr_tcam_process_move(
          tcam_pipe_tbl, tcam_entry, match_specs, move_node);
      break;
    case PIPE_MAT_UPDATE_ADD_IDLE:
      rc = pipe_mgr_tcam_execute_add_idle(tcam_pipe_tbl, tcam_entry, move_node);
      break;
  }
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }
  return PIPE_SUCCESS;
}

static bool pipe_mgr_op_needs_termination(tcam_tbl_info_t *tcam_tbl_info,
                                          pipe_mgr_move_list_t *move_node) {
  /* If the 2 ops are for different pipes, then terminate */

  /* If the current op is any op other than move, terminate */

  /* If the current op is a move, check the next op */
  if (move_node == NULL) {
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  if (move_node->next == NULL) {
    return true;
  }

  if (move_node->op != PIPE_MAT_UPDATE_MOV) {
    return true;
  }

  /* Current op is a move. Check the next op */
  if ((move_node->next->op != PIPE_MAT_UPDATE_MOV) &&
      (move_node->next->op != PIPE_MAT_UPDATE_ADD)) {
    return true;
  }

  pipe_idx_t src_index;

  tcam_llp_entry_t *src_entry;
  src_entry =
      pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, move_node->entry_hdl, 0);
  if (src_entry == NULL) {
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  tcam_pipe_tbl_t *src_pipe_tbl;
  src_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, src_entry->pipe_id);
  if (src_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam pipe table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  src_index = get_logical_index_from_index_ptn(
      src_pipe_tbl, src_entry->ptn_index, src_entry->index);

  if (src_index != move_node->next->u.single.logical_idx) {
    return true;
  }

  bf_dev_pipe_t dest_pipe_id;
  /* Check the pipes */
  if (move_node->next->op == PIPE_MAT_UPDATE_ADD) {
    dest_pipe_id = get_move_list_pipe(move_node->next);
  } else {
    PIPE_MGR_DBGCHK(move_node->next->op == PIPE_MAT_UPDATE_MOV);
    tcam_llp_entry_t *dest_entry;
    dest_entry = pipe_mgr_tcam_llp_entry_get(
        tcam_tbl_info, move_node->next->entry_hdl, 0);
    if (dest_entry == NULL) {
      PIPE_MGR_DBGCHK(0);
      return false;
    }
    dest_pipe_id = dest_entry->pipe_id;
  }

  if (src_entry->pipe_id != dest_pipe_id) {
    return true;
  }

  return false;
}

static bool is_tcam_interstage_move(tcam_tbl_info_t *tcam_tbl_info,
                                    pipe_mgr_move_list_t *move_node) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_t *tcam_tbl = NULL;
  tcam_llp_entry_t *tcam_entry = NULL;
  uint32_t dest_idx = 0, src_idx = 0;
  tcam_phy_loc_info_t dest_loc, src_loc;
  bool is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);

  if (TCAM_TBL_IS_ATCAM(tcam_tbl_info) ||
      (move_node->op != PIPE_MAT_UPDATE_MOV &&
       move_node->op != PIPE_MAT_UPDATE_MOV_MULTI &&
       move_node->op != PIPE_MAT_UPDATE_MOV_MOD &&
       move_node->op != PIPE_MAT_UPDATE_MOV_MULTI_MOD)) {
    return false;
  }

  tcam_entry =
      pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, move_node->entry_hdl, 0);
  if (tcam_entry) {
    tcam_pipe_tbl =
        get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, tcam_entry->pipe_id);
    if (!tcam_pipe_tbl) {
      return false;
    }
    tcam_tbl = tcam_pipe_tbl->tcam_ptn_tbls;

    src_idx = tcam_entry->index;
    if (is_range) {
      dest_idx = get_index_from_logical_index(
          tcam_pipe_tbl, move_node->u.multi.locations[0].logical_index_base);
    } else {
      dest_idx = get_index_from_logical_index(tcam_pipe_tbl,
                                              move_node->u.single.logical_idx);
    }

    if (pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, src_idx, &src_loc) !=
        PIPE_SUCCESS) {
      return false;
    }
    if (pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, dest_idx, &dest_loc) !=
        PIPE_SUCCESS) {
      return false;
    }

    return src_loc.stage_id != dest_loc.stage_id;
  }

  return false;
}

pipe_status_t pipe_mgr_tcam_process_move_list(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mgr_move_list_t *move_list,
                                              uint32_t *success_count) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_lock_id_type_e l_type = LOCK_ID_TYPE_INVALID;
  uint32_t i = 0, last_available = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found on device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(*success_count == 0);

  pipe_mgr_move_list_t *move_node = NULL, *next_move_node = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  l_type = tcam_tbl_info->lock_type;

  /*
   * Get the number of wide tcam units. This is used later on to
   * estimate the entry add instructions' size which is needed to
   * terminate the move chain if all instructions of the move chain
   * wouldn't fit within a single DMA buffer.
   *
   * Any pipe table can be used for this.
   */
  tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[0];
  if (tcam_pipe_tbl == NULL) {
    /* This should never be hit */
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "TCAM table for pipe 0 not found",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Set-up the session parameters */
  tcam_pipe_tbl->cur_sess_hdl = sess_hdl;
  tcam_pipe_tbl->llp.terminate_op = false;

  uint32_t wide_tcam_units = 0;
  tcam_stage_info_t *stage_data = NULL;
  for (int stage_idx = 0; stage_idx < tcam_pipe_tbl->num_stages; stage_idx++) {
    stage_data = &tcam_pipe_tbl->stage_data[stage_idx];
    if (wide_tcam_units < stage_data->pack_format.mem_units_per_tbl_word) {
      wide_tcam_units = stage_data->pack_format.mem_units_per_tbl_word;
    }
  }

  for (move_node = move_list; move_node; move_node = move_node->next) {
    /* Do a validation of the ops */
    rc = pipe_mgr_tcam_validate_placement_op(move_node);
    if (rc != PIPE_SUCCESS) {
      break;
    }

    i++;
    /* Check if the current op needs termination */
    bool terminate_op;
    terminate_op = pipe_mgr_op_needs_termination(tcam_tbl_info, move_node);

    /* If this is a move chain under a stats or idle resource lock we must be
     * sure all instructions for the move chain fit within a single DMA buffer.
     * This may require us to terminate a long move chain and restart it as a
     * new move chain if there is not enough space in the DMA buffer. */
    if (l_type != LOCK_ID_TYPE_INVALID && i > 1 && !terminate_op) {
      uint32_t current_available =
          pipe_mgr_drv_ilist_locked_size_remaining(sess_hdl, dev_id);
      if (last_available) {
        /* last_available has the value from the last time through, compare to
         * the current usage to see the decrease in space. */
        uint32_t decrease = last_available - current_available;
        uint32_t final_add = decrease + (20 * wide_tcam_units);

        /* The amount of space required is:
         *  - the number of bytes required to post the current move and
         *    final add, but in case the previous move used copy instruction
         *    and the current move wouldn't use copy instruction (due to move
         *    across TCAM units), then space required for current move
         *    would be same as add, so account for 2 times of add
         *  - Note that the direct resources are idle, stats, stful, and meters
         *    but we cannot have all at once, stats can be initialized through
         *    the pop but stful, meters and idle may need to be written.  The
         *    worst case one virtual write for stful/meter and one for idle.
         *  - space for the final two pop insructions
         *  - space for the final unlock
         */
        uint32_t virt_wr_sz = sizeof(pipe_instr_set_memdata_v_t);
        uint32_t idle_wr_sz = sizeof(pipe_instr_set_memdata_v_i_only_t);
        idle_wr_sz += sizeof(uint32_t);
        uint32_t pop_sz = 2 * sizeof(pipe_pop_table_move_adr_instr_t);
        uint32_t unlock_sz = sizeof(pipe_barrier_lock_instr_t);
        uint32_t required =
            2 * final_add + pop_sz + unlock_sz + virt_wr_sz + idle_wr_sz;
        if (required > current_available) {
          /* Instructions for this op will fit, but those for the next op will
           * not so terminate after this one. */
          terminate_op = true;
        }
      }

      last_available = current_available;
    }

    /* Check if the move chain should be split to ensure we can fit a
     * lock/unlock pair in the same DMA buffer.
     */
    if (!terminate_op &&
        (is_tcam_interstage_move(tcam_tbl_info, move_node) ||
         is_tcam_interstage_move(tcam_tbl_info, move_node->next))) {
      terminate_op = true;
      next_move_node = move_node->next;
      move_node->next = NULL;
    }

    tcam_llp_entry_t *tcam_entry = NULL;
    bf_dev_pipe_t pipe_id = 0;
    switch (move_node->op) {
      case PIPE_MAT_UPDATE_ADD:
      case PIPE_MAT_UPDATE_ADD_MULTI:
      case PIPE_MAT_UPDATE_SET_DFLT:
        tcam_entry = NULL;
        pipe_id = get_move_list_pipe(move_node);
        break;
      case PIPE_MAT_UPDATE_CLR_DFLT:
      case PIPE_MAT_UPDATE_DEL:
      case PIPE_MAT_UPDATE_MOD:
      case PIPE_MAT_UPDATE_MOV:
      case PIPE_MAT_UPDATE_MOV_MULTI:
      case PIPE_MAT_UPDATE_MOV_MOD:
      case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      case PIPE_MAT_UPDATE_ADD_IDLE:
        tcam_entry =
            pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, move_node->entry_hdl, 0);
        if (!tcam_entry) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "TCAM entry 0x%x not found for op %s (%d)",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              move_node->entry_hdl,
              pipe_mgr_move_list_op_str(move_node->op),
              move_node->op);
          rc = PIPE_OBJ_NOT_FOUND;
          break;
        }

        pipe_id = tcam_entry->pipe_id;
        break;
    }

    if (rc != PIPE_SUCCESS) {
      break;
    }

    tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
    if (tcam_pipe_tbl == NULL) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "TCAM table for pipe %d not found",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          pipe_id);
      rc = PIPE_UNEXPECTED;
      break;
    }

    /* Set-up the session parameters */
    tcam_pipe_tbl->cur_sess_hdl = sess_hdl;
    tcam_pipe_tbl->llp.terminate_op = terminate_op;

    rc = pipe_mgr_tcam_process_placement_op(
        tcam_pipe_tbl, tcam_entry, move_node);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d tcam_process_placement_op failed for entry %d op %s (%d) tbl "
          "0x%x",
          __func__,
          __LINE__,
          move_node->entry_hdl,
          pipe_mgr_move_list_op_str(move_node->op),
          move_node->op,
          tcam_tbl_info->tbl_hdl);
      return rc;
    }

    if (terminate_op) {
      /* Now do a terminating call */
      rc = pipe_mgr_tcam_unlock_all_stages(tcam_pipe_tbl);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
      i = 0;
      last_available = 0;
      if (next_move_node) {
        move_node->next = next_move_node;
        next_move_node = NULL;
      }
    }

    (*success_count)++;
    if (pipe_mgr_sess_in_batch(sess_hdl)) {
      pipe_mgr_drv_ilist_chkpt(sess_hdl);
    }

    tcam_pipe_tbl->llp.terminate_op = false;
  }

  if (tcam_pipe_tbl) {
    /* Now do a terminating call */
    rc = pipe_mgr_tcam_unlock_all_stages(tcam_pipe_tbl);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Tcam llp op failed for tbl 0x%x rc %d",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              rc);
  }
  return rc;
}

/** \brief pipe_mgr_tcam_get_programmed_entry_count
 *        Returns the number of programmed entries
 *
 * \param dev_tgt Device target
 * \param tbl_hdl tcam table handle
 * \param uint32_t* Pointer to the count of valid entries to fill
 * \return pipe_status_t Status of the operation
 */
pipe_status_t pipe_mgr_tcam_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, uint32_t *count_p) {
  uint32_t tcam_count = 0;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint8_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == tcam_pipe_tbl->pipe_id) {
      uint32_t ptn;
      for (ptn = 0; ptn < tcam_pipe_tbl->no_ptns; ptn++) {
        tcam_tbl_t *tcam_tbl;
        tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn];
        tcam_count += tcam_tbl->llp.total_hw_usage;
      }
      if (tcam_pipe_tbl->default_ent_type == TCAM_DEFAULT_ENT_TYPE_DIRECT) {
        PIPE_MGR_DBGCHK(tcam_count);
        tcam_count--;
      }
    }
  }

  *count_p = tcam_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_entry_get_programmed_location(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t subindex,
    bf_dev_pipe_t *pipe_id_p,
    uint8_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_hdl_p,
    uint32_t *index_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_llp_entry_t *tcam_entry = NULL;
  bf_dev_pipe_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_entry =
      pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, mat_ent_hdl, subindex);
  if (tcam_entry == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!TCAM_HLP_IS_RANGE_HEAD(tcam_entry)) {
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_id = tcam_entry->pipe_id;

  tcam_pipe_tbl = get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, pipe_id);
  if (tcam_pipe_tbl == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, tcam_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
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

  *pipe_id_p = tcam_pipe_tbl->pipe_id;
  *stage_id_p = (uint8_t)tcam_loc.stage_id;
  if (stage_table_hdl_p) {
    tcam_stage_info_t *stage_info;
    stage_info = get_tcam_stage_data(tcam_tbl, &tcam_loc);
    *stage_table_hdl_p = stage_info->stage_table_handle;
  }
  if (index_p) {
    *index_p = tcam_loc.stage_line_no;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_gen_lock_id(bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mgr_lock_id_type_e lock_id_type,
                                        lock_id_t *lock_id_p) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  return pipe_mgr_tcam_alloc_lock_id(tcam_tbl_info, lock_id_type, lock_id_p);
}

pipe_status_t pipe_mgr_tcam_update_lock_type(bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             bool idle,
                                             bool stat,
                                             bool add_lock) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl %d not found", __func__, __LINE__, mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(idle || stat);

  pipe_mgr_lock_id_type_e ltype = tcam_tbl_info->lock_type;
  bool idle_locked = false, stat_locked = false;
  switch (ltype) {
    case LOCK_ID_TYPE_IDLE_LOCK:
      idle_locked = true;
      break;
    case LOCK_ID_TYPE_STAT_LOCK:
      stat_locked = true;
      break;
    case LOCK_ID_TYPE_ALL_LOCK:
      idle_locked = true;
      stat_locked = true;
      break;
    default:
      idle_locked = false;
      stat_locked = false;
      break;
  }

  bool lock_stat = false, lock_idle = false;
  if (add_lock) {
    lock_stat = (stat || stat_locked) ? true : false;
    lock_idle = (idle || idle_locked) ? true : false;
  } else {
    lock_stat = (stat ? false : (stat_locked ? true : false));
    lock_idle = (idle ? false : (idle_locked ? true : false));
  }

  if (lock_idle && lock_stat) {
    tcam_tbl_info->lock_type = LOCK_ID_TYPE_ALL_LOCK;
  } else if (lock_idle) {
    tcam_tbl_info->lock_type = LOCK_ID_TYPE_IDLE_LOCK;
  } else if (lock_stat) {
    tcam_tbl_info->lock_type = LOCK_ID_TYPE_STAT_LOCK;
  } else {
    tcam_tbl_info->lock_type = LOCK_ID_TYPE_INVALID;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_update_idle_init_val(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0) {
  (void)dev_id;
  (void)mat_tbl_hdl;
  (void)idle_init_val_for_ttl_0;
  // nothing to do
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_reset_idle(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       bf_dev_pipe_t pipe_id,
                                       uint8_t stage_id,
                                       mem_id_t mem_id,
                                       uint32_t mem_offset) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_t *tcam_tbl = NULL;
  rmt_tbl_info_t *rmt_info = NULL;
  tcam_phy_loc_info_t tcam_loc;
  pipe_mat_ent_hdl_t ent_hdl = 0;
  uint32_t ent_idx = 0;
  uint32_t index = 0, ptn_index = 0;
  uint32_t i = 0, j = 0;
  uint32_t entries_per_word = 0;
  uint32_t ttl;
  bool found_mem_id = false;

  mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(device_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d TCAM tbl 0x%x not found on device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!tcam_tbl_info->idle_present) {
    return PIPE_SUCCESS;
  }
  for (i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
    if (PIPE_BITMAP_GET(&tcam_tbl_info->tcam_pipe_tbl[i].pipe_bmp, pipe_id)) {
      tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];
    }
  }
  if (tcam_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d TCAM pipe tbl not found for pipe %d tbl 0x%x device %d",
              __func__,
              __LINE__,
              pipe_id,
              mat_tbl_hdl,
              device_id);
    return PIPE_UNEXPECTED;
  }

  for (i = 0; !found_mem_id && i < mat_tbl_info->num_rmt_info; ++i) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->stage_id == stage_id &&
        rmt_info->type == RMT_TBL_TYPE_IDLE_TMO) {
      entries_per_word = rmt_info->pack_format.entries_per_tbl_word;
      for (ent_idx = 0, j = 0; j < rmt_info->bank_map->num_tbl_word_blks; j++) {
        if (mem_id == rmt_info->bank_map->tbl_word_blk[j].mem_id[0]) {
          ent_idx += mem_offset * entries_per_word;
          found_mem_id = true;
          break;
        } else {
          ent_idx += TOF_MAP_RAM_UNIT_DEPTH * entries_per_word;
        }
      }
    }
  }
  if (!found_mem_id) {
    LOG_ERROR("%s:%d Idletime rmt info not found for tcam tbl 0x%x dev %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < entries_per_word; i++) {
    sts = pipe_mgr_tcam_entry_hdl_from_stage_idx(device_id,
                                                 pipe_id,
                                                 mat_tbl_hdl,
                                                 stage_id,
                                                 rmt_info->handle,
                                                 ent_idx + i,
                                                 &ent_hdl);
    if (sts == PIPE_OBJ_NOT_FOUND) {
      // No valid entry at this index, move onto the next one
      continue;
    } else if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error retrieving entry at idx %d for tcam tbl 0x%x pipe %d "
          "stage %d device %d",
          __func__,
          __LINE__,
          ent_idx + i,
          mat_tbl_hdl,
          pipe_id,
          stage_id,
          device_id);
      return sts;
    }

    if (ent_hdl == tcam_pipe_tbl->llp.default_ent_hdl) {
      // Default entries don't use idletime
      continue;
    }

    sts = pipe_mgr_idle_get_init_ttl(device_id,
                                     mat_tbl_hdl,
                                     ent_hdl,
                                     pipe_id,
                                     stage_id,
                                     rmt_info->handle,
                                     &ttl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error retrieving ttl for entry %d in tcam tbl 0x%x device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }

    sts = pipe_mgr_get_tcam_index_for_match_addr(tcam_pipe_tbl,
                                                 stage_id,
                                                 rmt_info->handle,
                                                 ent_idx + i,
                                                 &index,
                                                 &ptn_index);
    if (PIPE_SUCCESS != sts) return sts;
    tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn_index);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    sts = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, index, &tcam_loc);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the physical location info for entry 0x%x "
          "rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          ent_hdl,
          sts);
      return sts;
    }

    sts = pipe_mgr_tcam_lock_stage(tcam_tbl, &tcam_loc, false);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to lock stage %d for tcam tbl 0x%x device %d",
                __func__,
                __LINE__,
                stage_id,
                mat_tbl_hdl,
                device_id);
      return sts;
    }

    sts = rmt_idle_delete_entry(sess_hdl,
                                device_id,
                                mat_tbl_hdl,
                                ent_hdl,
                                tcam_pipe_tbl->pipe_id,
                                stage_id,
                                rmt_info->handle,
                                ent_idx + i,
                                0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error deleting idletime state for entry %d in tcam tbl 0x%x "
          "device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }

    sts = rmt_idle_add_entry(sess_hdl,
                             device_id,
                             mat_tbl_hdl,
                             ent_hdl,
                             tcam_pipe_tbl->pipe_id,
                             stage_id,
                             rmt_info->handle,
                             ent_idx + i,
                             ttl,
                             true,
                             0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error re-adding idletime state for entry %d in tcam tbl 0x%x "
          "device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }
    pipe_mgr_tcam_unlock_all_stages(tcam_pipe_tbl);
  }
  pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);

  return PIPE_SUCCESS;
}

static bool is_entry_in_pipe(pipe_mat_ent_hdl_t ent_hdl,
                             bf_dev_pipe_t pipe_id) {
  return (pipe_id == BF_DEV_PIPE_ALL) ||
         (PIPE_GET_HDL_PIPE(ent_hdl) == pipe_id);
}

pipe_status_t pipe_mgr_tcam_get_first_programmed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl;
  bool first_entry_found = false;
  bf_map_sts_t msts;
  uint32_t i;

  *entry_hdl = -1;
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  tcam_llp_entry_t *head_entry;
  (void)head_entry;
  unsigned long key;

  for (i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[i];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != tcam_pipe_tbl->pipe_id)
      continue;

    msts = bf_map_get_first(
        &tcam_pipe_tbl->llp.tcam_entry_db, &key, (void **)&head_entry);
    if (msts != BF_MAP_OK) {
      *entry_hdl = -1;
      if (dev_tgt.dev_pipe_id == tcam_pipe_tbl->pipe_id) {
        /* No entry in the requested pipe_id */
        break;
      } else {
        /* Keep looking in other table instances */
        continue;
      }
    }

    while (msts == BF_MAP_OK) {
      *entry_hdl = key;
      if (!is_entry_hdl_default(tcam_tbl_info, *entry_hdl, false) &&
          is_entry_in_pipe(*entry_hdl, dev_tgt.dev_pipe_id)) {
        first_entry_found = true;
        break;
      } else {
        msts = bf_map_get_next(
            &tcam_pipe_tbl->llp.tcam_entry_db, &key, (void **)&head_entry);
      }
    }

    if (first_entry_found) break;

    *entry_hdl = -1;
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_next_programmed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_llp_entry_t *head_entry = NULL;
  tcam_pipe_tbl_t *first_tcam_tbl;
  tcam_pipe_tbl_t *cur_tcam_tbl;
  unsigned long key;
  bf_map_sts_t msts;
  uint32_t first_tcam_idx, idx, t;
  bool done = false;
  int i = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the TCAM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (tcam_tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric tcam tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      dev_tgt.dev_pipe_id != PIPE_GET_HDL_PIPE(entry_hdl)) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d for entry hdl %d passed for "
        "asymmetric tcam tbl with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Start with the entry's table instance. */
  first_tcam_tbl = pipe_mgr_tcam_tbl_get_instance_from_entry(
      tcam_tbl_info, entry_hdl, __func__, __LINE__);
  if (!first_tcam_tbl) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Determine the table instance index of this first table. */
  for (t = 0; t < tcam_tbl_info->no_tcam_pipe_tbls; t++) {
    if (first_tcam_tbl == &tcam_tbl_info->tcam_pipe_tbl[t]) {
      first_tcam_idx = t;
      break;
    }
  }

  /* Sanity check. */
  if (t >= tcam_tbl_info->no_tcam_pipe_tbls) {
    LOG_ERROR("%s:%d Unexpected error for tcam tbl 0x%x, entry hdl %d",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              entry_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  key = entry_hdl;
  for (idx = first_tcam_idx;
       idx < tcam_tbl_info->no_tcam_pipe_tbls && i < n && !done;
       idx++) {
    cur_tcam_tbl = &tcam_tbl_info->tcam_pipe_tbl[idx];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != cur_tcam_tbl->pipe_id)
      continue;

    if (cur_tcam_tbl != first_tcam_tbl) {
      /* Look in next table instance. */
      msts = bf_map_get_first(
          &cur_tcam_tbl->llp.tcam_entry_db, &key, (void **)&head_entry);
      if (msts == BF_MAP_OK) {
        if (!is_entry_hdl_default(tcam_tbl_info, key, false) &&
            is_entry_in_pipe(key, dev_tgt.dev_pipe_id)) {
          /* Entry present */
          next_entry_handles[i++] = key;
        }
      } else {
        /* Keep looking in other table instances. */
        continue;
      }
    }

    while (i < n) {
      msts = bf_map_get_next(
          &cur_tcam_tbl->llp.tcam_entry_db, &key, (void **)&head_entry);
      if (msts != BF_MAP_OK) {
        next_entry_handles[i] = -1;
        if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
          /* There is no more entry for the requested pipe. */
          done = true;
        }
        break;
      }

      if (is_entry_hdl_default(tcam_tbl_info, key, false) ||
          !is_entry_in_pipe(key, dev_tgt.dev_pipe_id))
        continue;

      next_entry_handles[i] = key;
      i++;
    }
  }

  if (i < n) next_entry_handles[i] = -1;

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_decode_tind_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_stage_info_t *stage_data,
    uint32_t stage_line_no,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    uint8_t *tind_word_ptr,
    uint32_t tind_subword_pos,
    uint16_t offset) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_entry_format_tof_tind_tbl_ent_decode_to_components(
      tcam_tbl_info->dev_id,
      tcam_tbl_info->profile_id,
      stage_data->stage_id,
      tcam_tbl_info->tbl_hdl,
      tind_subword_pos,
      offset,
      tind_word_ptr,
      act_data_spec,
      act_fn_hdl,
      indirect_ptrs,
      ptr_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding TIND entry at line %d, sub entry "
        "%d, "
        "tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        stage_line_no,
        tind_subword_pos,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_range_max_expansion_entry_count_get(
    tcam_tbl_info_t *tcam_tbl_info, uint32_t *max_expanded_range_entries) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool is_range = TCAM_TBL_USES_RANGE(tcam_tbl_info);
  *max_expanded_range_entries = 1;
  if (is_range) {
    rc = pipe_mgr_entry_format_tof_range_max_expansion_entry_count_get(
        tcam_tbl_info->dev_id,
        tcam_tbl_info->profile_id,
        tcam_tbl_info->tbl_hdl,
        max_expanded_range_entries);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting the max expanded range entries"
          " for tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_decode_entry(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_stage_info_t *stage_data,
    uint32_t line_no,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    uint8_t **tcam_word_ptrs,
    uint8_t subword,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    bool *entry_valid,
    bool *is_range_entry_head) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool is_stash = false;
  bool isatcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  if (!isatcam) {
    uint8_t version_key = 0, version_mask = 0;
    rc = pipe_mgr_entry_format_tof_tern_tbl_ent_decode_to_match_spec(
        tcam_tbl_info->dev_id,
        tcam_tbl_info->profile_id,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        match_spec,
        tcam_word_ptrs,
        &version_key,
        &version_mask,
        is_range_entry_head);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding read tcam entry to match spec at idx %d "
          " for tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          line_no,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      return rc;
    }
    if (!version_mask && version_key) {
      *entry_valid = false;
    } else {
      *entry_valid = true;
    }
  } else {
    uint8_t version_valid_bits = 0;
    rc = pipe_mgr_entry_format_tof_exm_tbl_ent_decode_to_components(
        tcam_tbl_info->dev_id,
        tcam_tbl_info->profile_id,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        stage_data->stage_table_handle,
        subword,
        &version_valid_bits,
        match_spec,
        &action_spec->act_data,
        tcam_word_ptrs,
        NULL,
        indirect_ptrs,
        ptr_info,
        act_fn_hdl,
        NULL,
        is_stash,
        NULL /* proxy_hash */);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding atcam entry to match spec at idx %d, for "
          "tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          line_no,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      return rc;
    }
    if (version_valid_bits == RMT_EXM_ENTRY_VERSION_INVALID) {
      *entry_valid = false;
    } else {
      *entry_valid = true;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t get_tind_entry_from_hw(
    tcam_tbl_info_t *tcam_tbl_info,
    bf_dev_pipe_t pipe_id,
    tcam_stage_info_t *stage_data,
    uint32_t stage_line_no,
    uint8_t *tind_word_ptr,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_ent_decode_ptr_info_t *ptr_info,
    pipe_sess_hdl_t *sess_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t tind_block = 0;
  uint32_t tind_subword_pos = 0;
  uint32_t tind_line_no = 0;
  bool tind_exists;








    tind_exists = pipe_mgr_tcam_tind_get_line_no(stage_data,
                                                 stage_line_no,
                                                 &tind_line_no,
                                                 &tind_block,
                                                 &tind_subword_pos);


  if (!tind_exists) {
    *act_fn_hdl = stage_data->tind_act_fn_hdl;
    if (*act_fn_hdl == 0 && tcam_tbl_info->num_actions) {
      *act_fn_hdl = tcam_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
    }
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_MEMSET(tind_word_ptr, 0, TOF_BYTES_IN_RAM_WORD * sizeof(uint8_t));
  uint32_t num_tind_entries = 1;  // always 1
  bf_dev_pipe_t phy_pipe_id;

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              tcam_tbl_info->dev_id,
              pipe_str_err(rc));
    return rc;
  }
  uint8_t formatting_index = tind_subword_pos;
  pipe_mem_type_t mem_type = pipe_mem_type_unit_ram;
  uint16_t mem_unit_depth =
      pipe_mgr_get_mem_unit_depth(tcam_tbl_info->dev_id, RMT_MEM_SRAM);
  uint32_t offset = 0;
  if (mem_unit_depth == 0) {
    LOG_ERROR("%s:%d Invalid memory depth for dev %d mem type SRAM",
              __func__,
              __LINE__,
              tcam_tbl_info->dev_id);
    return PIPE_UNEXPECTED;
  }

  /* Generate the address for tind */
  uint64_t addr = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
      tcam_tbl_info->direction,
      phy_pipe_id,
      stage_data->stage_id,
      stage_data->tind_blk[tind_block].mem_id[0],
      tind_line_no % mem_unit_depth,
      mem_type);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);

  rc = pipe_mgr_dump_any_tbl_by_addr(tcam_tbl_info->dev_id,
                                     subdev,
                                     tcam_tbl_info->tbl_hdl,
                                     0,
                                     stage_data->stage_id,
                                     RMT_TBL_TYPE_TERN_INDIR,
                                     &addr,
                                     num_tind_entries,
                                     tind_subword_pos,
                                     0,
                                     NULL,
                                     NULL,
                                     0,
                                     &tind_word_ptr,
                                     NULL,
                                     sess_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in dumping TIND entry at line %d, sub entry %d, "
        "for "
        "tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        tind_line_no,
        tind_subword_pos,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  return pipe_mgr_tcam_decode_tind_entry(tcam_tbl_info,
                                         stage_data,
                                         stage_line_no,
                                         act_data_spec,
                                         act_fn_hdl,
                                         indirect_ptrs,
                                         ptr_info,
                                         tind_word_ptr,
                                         formatting_index,
                                         offset);
}

static pipe_status_t tcam_build_indirect_resources_from_hw(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_stage_info_t *stage_data,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs) {
  pipe_status_t rc = PIPE_SUCCESS;
  unsigned i = 0;
  bool pfe = false, pfe_defaulted = false;
  for (i = 0; i < tcam_tbl_info->num_tbl_refs; i++) {
    pfe = false;
    pfe_defaulted = false;
    pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->tbl_refs[i];
    pipe_res_spec_t *res_spec =
        &action_spec->resources[action_spec->resource_count];
    res_spec->tbl_hdl = tbl_ref->tbl_hdl;
    if (tbl_ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      action_spec->resource_count++;
      switch (PIPE_GET_HDL_TYPE(tbl_ref->tbl_hdl)) {
        case PIPE_HDL_TYPE_STFUL_TBL:
          rc = pipe_mgr_stful_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                   tbl_ref->tbl_hdl,
                                                   tcam_pipe_tbl->pipe_id,
                                                   stage_data->stage_id,
                                                   indirect_ptrs->stfl_ptr,
                                                   &pfe,
                                                   &pfe_defaulted,
                                                   &res_spec->tbl_idx);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding stful addr 0x%x, for tbl 0x%x, device "
                "id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->stfl_ptr,
                tbl_ref->tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_data->stage_id,
                pipe_str_err(rc));
            return rc;
          }
          if (pfe || pfe_defaulted) {
            /* Stfl is enabled */
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
          } else {
            res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          // Restore the stful table handle
          res_spec->tbl_hdl = tbl_ref->tbl_hdl;
          break;
        case PIPE_HDL_TYPE_STAT_TBL:
          rc = pipe_mgr_stat_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                  tbl_ref->tbl_hdl,
                                                  tcam_pipe_tbl->pipe_id,
                                                  stage_data->stage_id,
                                                  indirect_ptrs->stats_ptr,
                                                  &pfe,
                                                  &pfe_defaulted,
                                                  &res_spec->tbl_idx);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding stats addr 0x%x, for tbl 0x%x, device "
                "id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->stats_ptr,
                tbl_ref->tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_data->stage_id,
                pipe_str_err(rc));
            return rc;
          }
          if (pfe || pfe_defaulted) {
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
          } else {
            res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          // Restore the stat table handle
          res_spec->tbl_hdl = tbl_ref->tbl_hdl;
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          rc = pipe_mgr_meter_mgr_decode_virt_addr(tcam_tbl_info->dev_id,
                                                   tbl_ref->tbl_hdl,
                                                   tcam_pipe_tbl->pipe_id,
                                                   stage_data->stage_id,
                                                   indirect_ptrs->meter_ptr,
                                                   &pfe,
                                                   &pfe_defaulted,
                                                   &res_spec->tbl_idx);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in decoding meter addr 0x%x, for tbl 0x%x, device "
                "id %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                indirect_ptrs->meter_ptr,
                tbl_ref->tbl_hdl,
                tcam_tbl_info->dev_id,
                tcam_pipe_tbl->pipe_id,
                stage_data->stage_id,
                pipe_str_err(rc));
            return rc;
          }
          if (pfe || pfe_defaulted) {
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
          } else {
            res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
          }
          // Restore the meter table handle
          res_spec->tbl_hdl = tbl_ref->tbl_hdl;
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          return PIPE_INVALID_ARG;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t get_action_spec_for_tcam_entry_from_hw(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    bf_dev_pipe_t pipe_id,
    tcam_stage_info_t *stage_data,
    uint32_t stage_line_no,
    bool skip_direct_adt,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_act_fn_hdl_t act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_tbl_ref_t *tbl_ref = &tcam_tbl_info->adt_tbl_ref;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = tbl_ref->tbl_hdl;
  rmt_virt_addr_t virt_addr = 0xdeadbeef;
  dev_target_t dev_tgt = {tcam_tbl_info->dev_id, pipe_id};

  if (tcam_tbl_info->sel_present && indirect_ptrs->sel_len > 0) {
    /* Since group handle is not saved in LLP state, populate the field
     * with the selector logical index instead. If we are in single-process
     * mode and HLP state exists, this value will be later replaced with
     * the actual group handle.
     */
    virt_addr = indirect_ptrs->sel_ptr;
    pipe_mgr_sel_vaddr_to_logical_idx(tcam_tbl_info->dev_id,
                                      tcam_tbl_info->sel_tbl_ref.tbl_hdl,
                                      stage_data->stage_id,
                                      virt_addr,
                                      &action_spec->sel_grp_hdl);
    action_spec->pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
  } else if (tcam_tbl_info->adt_present) {
    if (tbl_ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      /* For indirectly referenced action data table, the location in the
       * action RAM from which the entry is read is derived from the action
       * data pointer in the match overhead.
       */
      action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
      virt_addr = indirect_ptrs->adt_ptr;
      rc =
          pipe_mgr_adt_mgr_get_ent_hdl_from_location(tcam_tbl_info->dev_id,
                                                     adt_tbl_hdl,
                                                     tcam_pipe_tbl->pipe_id,
                                                     stage_data->stage_id,
                                                     virt_addr,
                                                     true,
                                                     &action_spec->adt_ent_hdl);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    } else {
      action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
      if (!skip_direct_adt) {
        rc = pipe_mgr_adt_mgr_decode_to_act_data_spec(
            dev_tgt,
            adt_tbl_hdl,
            stage_data->stage_table_handle,
            stage_line_no,
            tcam_pipe_tbl->pipe_id,
            stage_data->stage_id,
            act_fn_hdl,
            &action_spec->act_data,
            NULL,
            true,
            NULL);
        if (rc != PIPE_SUCCESS) {
          return rc;
        }
      }
    }
  } else {
    /* If there is no action table, set it to action data type */
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }

  /* Next, recover the indirect index by decoding the virtual addresses read
   * from hw (in the indirect_ptrs) and populate them in the resource specs. */
  rc = tcam_build_indirect_resources_from_hw(
      tcam_tbl_info, tcam_pipe_tbl, stage_data, action_spec, indirect_ptrs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in building resource spec from hw for tcam entry at idx "
        "%d, tbl 0x%x, device id %d, pipe_id %d, stage_id %d, err %s",
        __func__,
        __LINE__,
        stage_line_no,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        tcam_pipe_tbl->pipe_id,
        stage_data->stage_id,
        pipe_str_err(rc));
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_entry_from_hw_raw(
    tcam_tbl_t *tcam_tbl,
    bf_dev_pipe_t pipe_id,
    tcam_stage_info_t *stage_data,
    tcam_block_data_t *block_data,
    uint32_t tcam_index,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    uint8_t **tcam_word_ptrs,
    bool *entry_valid) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_phy_loc_info_t dest_loc;
  bool is_range_entry_head = false;

  tcam_pipe_tbl_t *tcam_pipe_tbl = tcam_tbl->tcam_pipe_tbl_p;
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;

  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_index, &dest_loc);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }
  uint32_t line_no = dest_loc.phy_line_no;
  uint8_t stage_id = stage_data->stage_id;
  uint32_t stage_line_no = dest_loc.stage_line_no;
  bf_dev_pipe_t phy_pipe_id;

  if (!PIPE_BITMAP_GET(&tcam_pipe_tbl->pipe_bmp, pipe_id)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              tcam_tbl_info->dev_id,
              pipe_str_err(rc));
    return rc;
  }
  uint32_t mem_units_num = stage_data->pack_format.mem_units_per_tbl_word;
  uint8_t phy_addrs_map[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  uint64_t addrs[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  pipe_mem_type_t pipe_mem_type =
      is_atcam ? pipe_mem_type_unit_ram : pipe_mem_type_tcam;
  unsigned i = 0;

  for (i = 0; i < mem_units_num; i++) {
    /* Generate the address for tcam */
    if (pipe_mem_type == pipe_mem_type_tcam) {
      addrs[mem_units_num - 1 - i] =
          tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
              tcam_tbl_info->direction,
              phy_pipe_id,
              stage_id,
              block_data->word_blk.mem_id[i],
              line_no,
              pipe_mem_type);
    } else {
      addrs[i] = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
          tcam_tbl_info->direction,
          phy_pipe_id,
          stage_id,
          block_data->word_blk.mem_id[i],
          line_no,
          pipe_mem_type);
    }
    PIPE_MGR_MEMSET(
        tcam_word_ptrs[i], 0, sizeof(uint8_t) * TOF_BYTES_IN_RAM_WORD);
  }

  if (is_atcam) {
    for (i = 0; i < mem_units_num; i++) {
      phy_addrs_map[i] = mem_units_num - i - 1;
    }
  }

  pipe_sess_hdl_t sess_hdl = tcam_pipe_tbl->cur_sess_hdl;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(
      tcam_tbl_info->dev_id,
      subdev,
      tcam_tbl_info->tbl_hdl,
      stage_data->stage_table_handle,
      stage_id,
      is_atcam ? RMT_TBL_TYPE_ATCAM_MATCH : RMT_TBL_TYPE_TERN_MATCH,
      addrs,
      stage_data->pack_format.mem_units_per_tbl_word,
      dest_loc.subword,
      0,
      NULL,
      NULL,
      0,
      tcam_word_ptrs,
      phy_addrs_map,
      &sess_hdl);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in reading TCAM entry at idx %d, stage id %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        tcam_index,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  /* First, decode the TCAM entry */
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_mgr_ent_decode_ptr_info_t ptr_info = {0};
  rc = pipe_mgr_tcam_decode_entry(tcam_tbl_info,
                                  stage_data,
                                  line_no,
                                  match_spec,
                                  action_spec,
                                  tcam_word_ptrs,
                                  dest_loc.subword,
                                  act_fn_hdl,
                                  &indirect_ptrs,
                                  &ptr_info,
                                  entry_valid,
                                  &is_range_entry_head);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d TCAM entry at idx %d, stage id %d, tbl 0x%x, device id %d "
        "invalidated, decode status %s",
        __func__,
        __LINE__,
        tcam_index,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Next decode TIND entry, if any */
  if (is_range_entry_head && *entry_valid) {
    if (!is_atcam) {
      uint8_t tind_word_ptr[TOF_BYTES_IN_RAM_WORD] = {0};
      PIPE_MGR_MEMSET(tind_word_ptr, 0, sizeof(tind_word_ptr));
      rc = get_tind_entry_from_hw(tcam_tbl_info,
                                  pipe_id,
                                  stage_data,
                                  dest_loc.stage_line_no,
                                  tind_word_ptr,
                                  &action_spec->act_data,
                                  act_fn_hdl,
                                  &indirect_ptrs,
                                  &ptr_info,
                                  &sess_hdl);
      if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
        LOG_ERROR(
            "%s:%d Error in reading TIND entry from hardware for line no %d, "
            "stage %d, tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            dest_loc.stage_line_no,
            stage_data->stage_id,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id,
            pipe_str_err(rc));
        /* In this case entry is valid, but action spec is not. Log error and
         * return PIPE_SUCCESS. Caller should fix the entry. */
        return PIPE_SUCCESS;
      }
    }
    rc = get_action_spec_for_tcam_entry_from_hw(tcam_tbl_info,
                                                tcam_pipe_tbl,
                                                pipe_id,
                                                stage_data,
                                                stage_line_no,
                                                false,
                                                action_spec,
                                                &indirect_ptrs,
                                                *act_fn_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting action spec for tcam entry at line no %d, "
          "stage id %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          stage_line_no,
          stage_data->stage_id,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      /* In this case entry is valid, but action spec is not. Log error and
       * return PIPE_SUCCESS. Caller should fix the entry. */
      return PIPE_SUCCESS;
    }
  }

  /* Populate the partition index for atcam tables */
  if (is_atcam) {
    uint32_t index = 0;
    pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(tcam_pipe_tbl,
                                                  dest_loc.block_id,
                                                  line_no,
                                                  dest_loc.subword,
                                                  &index,
                                                  &match_spec->partition_index);
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
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tcam_get_each_entry_from_hw(
    tcam_tbl_info_t *tcam_tbl_info,
    tcam_tbl_t *tcam_tbl,
    tcam_pipe_tbl_t *tcam_pipe_tbl,
    tcam_stage_info_t *stage_data,
    tcam_block_data_t *block_data,
    tcam_llp_entry_t *tcam_entry,
    bf_dev_pipe_t pipe_id,
    uint32_t tcam_index,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    uint8_t **tcam_word_ptrs) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_phy_loc_info_t dest_loc;
  bool is_range_entry_head = false;

  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_index, &dest_loc);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }
  uint32_t line_no = dest_loc.phy_line_no;
  uint8_t stage_id = stage_data->stage_id;
  uint32_t stage_line_no = dest_loc.stage_line_no;
  bf_dev_pipe_t phy_pipe_id;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    if (tcam_entry->pipe_id == BF_DEV_PIPE_ALL)
      pipe_id = PIPE_BITMAP_GET_FIRST_SET(&tcam_pipe_tbl->pipe_bmp);
    else
      pipe_id = tcam_entry->pipe_id;
  }

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      tcam_tbl_info->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              tcam_tbl_info->dev_id,
              pipe_str_err(rc));
    return rc;
  }

  uint8_t phy_addrs_map[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  uint64_t addrs[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK] = {0};
  bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  pipe_mem_type_t pipe_mem_type =
      is_atcam ? pipe_mem_type_unit_ram : pipe_mem_type_tcam;
  unsigned i = 0;

  for (i = 0; i < stage_data->pack_format.mem_units_per_tbl_word; i++) {
    /* Generate the address for tcam */
    if (pipe_mem_type == pipe_mem_type_tcam) {
      addrs[stage_data->pack_format.mem_units_per_tbl_word - 1 - i] =
          tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
              tcam_tbl_info->direction,
              phy_pipe_id,
              stage_id,
              block_data->word_blk.mem_id[i],
              line_no,
              pipe_mem_type);
    } else {
      addrs[i] = tcam_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
          tcam_tbl_info->direction,
          phy_pipe_id,
          stage_id,
          block_data->word_blk.mem_id[i],
          line_no,
          pipe_mem_type);
    }
    PIPE_MGR_MEMSET(
        tcam_word_ptrs[i], 0, sizeof(uint8_t) * TOF_BYTES_IN_RAM_WORD);
  }

  if (is_atcam) {
    for (i = 0; i < stage_data->pack_format.mem_units_per_tbl_word; i++) {
      phy_addrs_map[i] = stage_data->pack_format.mem_units_per_tbl_word - i - 1;
    }
  }

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(
      tcam_tbl_info->dev_id,
      subdev,
      tcam_tbl_info->tbl_hdl,
      stage_data->stage_table_handle,
      stage_id,
      is_atcam ? RMT_TBL_TYPE_ATCAM_MATCH : RMT_TBL_TYPE_TERN_MATCH,
      addrs,
      stage_data->pack_format.mem_units_per_tbl_word,
      dest_loc.subword,
      0,
      NULL,
      NULL,
      0,
      tcam_word_ptrs,
      phy_addrs_map,
      NULL /*sess_hdl*/);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in reading TCAM entry at idx %d, stage id %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        tcam_index,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  /* First, decode the TCAM entry */
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_mgr_ent_decode_ptr_info_t ptr_info = {0};
  bool entry_valid = false;
  rc = pipe_mgr_tcam_decode_entry(tcam_tbl_info,
                                  stage_data,
                                  line_no,
                                  match_spec,
                                  action_spec,
                                  tcam_word_ptrs,
                                  dest_loc.subword,
                                  act_fn_hdl,
                                  &indirect_ptrs,
                                  &ptr_info,
                                  &entry_valid,
                                  &is_range_entry_head);
  if (rc != PIPE_SUCCESS || !entry_valid) {
    LOG_ERROR(
        "%s:%d TCAM entry at idx %d, stage id %d, tbl 0x%x, device id %d "
        "invalidated, decode status %s",
        __func__,
        __LINE__,
        tcam_index,
        stage_data->stage_id,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Next decode TIND entry, if any */
  if (TCAM_LLP_IS_RANGE_HEAD(tcam_entry)) {
    if (!is_atcam) {
      uint8_t *tind_word_ptr = tcam_word_ptrs[0];
      PIPE_MGR_MEMSET(
          tind_word_ptr, 0, sizeof(uint8_t) * TOF_BYTES_IN_RAM_WORD);
      rc = get_tind_entry_from_hw(tcam_tbl_info,
                                  pipe_id,
                                  stage_data,
                                  dest_loc.stage_line_no,
                                  tind_word_ptr,
                                  &action_spec->act_data,
                                  act_fn_hdl,
                                  &indirect_ptrs,
                                  &ptr_info,
                                  NULL /*sess_hdl*/);
      if (rc != PIPE_SUCCESS && rc != PIPE_OBJ_NOT_FOUND) {
        LOG_ERROR(
            "%s:%d Error in reading TIND entry from hardware for line no %d, "
            "stage %d, tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            dest_loc.stage_line_no,
            stage_data->stage_id,
            tcam_tbl_info->tbl_hdl,
            tcam_tbl_info->dev_id,
            pipe_str_err(rc));
        return rc;
      }
    }
    rc = get_action_spec_for_tcam_entry_from_hw(tcam_tbl_info,
                                                tcam_pipe_tbl,
                                                pipe_id,
                                                stage_data,
                                                stage_line_no,
                                                false,
                                                action_spec,
                                                &indirect_ptrs,
                                                *act_fn_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting action spec for tcam entry at line no %d, "
          "stage id %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          stage_line_no,
          stage_data->stage_id,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }

  /* Populate the partition index for atcam tables */
  if (is_atcam) {
    uint32_t index = 0;
    pipe_mgr_tcam_get_ptn_index_from_phy_loc_info(tcam_pipe_tbl,
                                                  dest_loc.block_id,
                                                  line_no,
                                                  dest_loc.subword,
                                                  &index,
                                                  &match_spec->partition_index);
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
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t tcam_set_partition_idx_in_match_spec(
    tcam_tbl_info_t *tcam_tbl_info,
    pipe_tbl_match_spec_t *match_spec,
    uint32_t partition_idx) {
  // Get partition idx info from mat tbl info
  pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
      tcam_tbl_info->dev_id, tcam_tbl_info->tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table 0x%x not found in RMT database for device %d",
              __func__,
              __LINE__,
              tcam_tbl_info->tbl_hdl,
              tcam_tbl_info->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint32_t partition_idx_start_bit;
  uint32_t partition_idx_field_width;

  if (mat_tbl_info->partition_idx_info) {
    pipe_partition_idx_info_t *p = mat_tbl_info->partition_idx_info;
    partition_idx_start_bit = p->partition_idx_start_bit;
    partition_idx_field_width = p->partition_idx_field_width;
  } else if (mat_tbl_info->alpm_info) {
    alpm_mat_tbl_info_t *p = mat_tbl_info->alpm_info;
    partition_idx_start_bit = p->partition_idx_start_bit;
    partition_idx_field_width = p->partition_idx_field_width;
  } else {
    LOG_ERROR("%s:%d get atcam partition info", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }
  // Put partition idx in match_spec too and make its mask
  // from start bit to width size all 1s
  // Copied from pipe_mgr_entry_format_tof_exm_tbl_ent_decode_to_components
  uint16_t start_offset = partition_idx_start_bit / 8;
  uint16_t end_offset =
      (partition_idx_start_bit + partition_idx_field_width) / 8;
  uint8_t bit_offset = partition_idx_start_bit % 8;
  for (uint16_t j = start_offset; j < end_offset; j++) {
    match_spec->match_mask_bits[j] |= ((1 << (8 - bit_offset)) - 1);
    bit_offset = 0;
  }
  // value bits
  uint32_t nval = htobe32(partition_idx);
  uint8_t *val_p = (uint8_t *)&nval;
  size_t field_size_bytes = (partition_idx_field_width + 7) / 8;
  memcpy(match_spec->match_value_bits + start_offset,
         val_p + 4 - field_size_bytes,
         end_offset - start_offset);
  return PIPE_SUCCESS;
}

pipe_status_t tcam_get_default_entry_from_hw(
    tcam_tbl_t *tcam_tbl,
    tcam_phy_loc_info_t *tcam_loc,
    bf_dev_pipe_t pipe_id,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  tcam_pipe_tbl_t *tcam_pipe_tbl = get_tcam_pipe_tbl(tcam_tbl);
  tcam_stage_info_t *stage_data = NULL;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_tcam_execute_default_ent_get(tcam_tbl,
                                             tcam_pipe_tbl,
                                             pipe_id,
                                             act_fn_hdl,
                                             &pipe_action_spec->act_data,
                                             &indirect_ptrs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting the default entry for tbl 0x%x, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }
  if (*act_fn_hdl == 0) {
    return PIPE_OBJ_NOT_FOUND;
  }

  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);

  bool skip_direct_adt = tcam_tbl->tcam_pipe_tbl_p->default_ent_type ==
                         TCAM_DEFAULT_ENT_TYPE_INDIRECT;
  rc = get_action_spec_for_tcam_entry_from_hw(tcam_tbl_info,
                                              tcam_pipe_tbl,
                                              pipe_id,
                                              stage_data,
                                              tcam_loc->stage_line_no,
                                              skip_direct_adt,
                                              pipe_action_spec,
                                              &indirect_ptrs,
                                              *act_fn_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting the default entry for tbl 0x%x, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        tcam_tbl_info->tbl_hdl,
        tcam_tbl_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_get_entry_llp_from_hw(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  bool is_backup = false;
  tcam_llp_entry_t *tcam_entry = NULL, *range_entry = NULL;
  unsigned i = 0;
  tcam_stage_info_t *stage_data = NULL;
  tcam_block_data_t *block_data = NULL;
  uint32_t subindex = 0;
  uint32_t range_count = 0;
  pipe_tbl_match_spec_t **match_spec_arr = NULL;
  bf_dev_pipe_t pipe_id = 0;

  /* Get table info and location details for this entry */
  tcam_tbl_info =
      pipe_mgr_tcam_tbl_info_get(dev_tgt.device_id, tbl_hdl, is_backup);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d could not find tcam tbl with handle 0x%x device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  bool range = tcam_tbl_info->uses_range;

  if (!tcam_tbl_info->is_symmetric) {
    /* Make sure the table has been locked for the proper pipe. For a symmetric
     * table, the table has been locked for all pipes. */
    tcam_pipe_tbl = pipe_mgr_tcam_tbl_get_instance_from_entry(
        tcam_tbl_info, entry_hdl, __func__, __LINE__);
    if (!tcam_pipe_tbl) {
      return PIPE_OBJ_NOT_FOUND;
    }

    rc = pipe_mgr_is_pipe_in_bmp(&tcam_pipe_tbl->pipe_bmp, dev_tgt.dev_pipe_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Entry with handle %d with pipe id %d does not match requested "
          "pipe id %d in ternary match table with handle %d, device_id %d",
          __func__,
          entry_hdl,
          PIPE_GET_HDL_PIPE(entry_hdl),
          dev_tgt.dev_pipe_id,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }

  tcam_llp_entry_t *head_entry =
      pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, entry_hdl, subindex);
  if (head_entry == NULL) {
    LOG_ERROR(
        "%s:%d could not find tcam entry with ent handle 0x%x for tbl 0x%x "
        "device id %d \n",
        __func__,
        __LINE__,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  tcam_pipe_tbl =
      get_tcam_pipe_tbl_by_pipe_id(tcam_tbl_info, head_entry->pipe_id);
  if (tcam_pipe_tbl == NULL) {
    LOG_ERROR("%s:%d get tcam pipe table failed", __func__, __LINE__);
    return PIPE_UNEXPECTED;
  }

  rc = pipe_mgr_get_pipe_id(&tcam_pipe_tbl->pipe_bmp,
                            dev_tgt.dev_pipe_id,
                            head_entry->pipe_id,
                            &pipe_id);
  if (rc != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s:%d Tbl hdl 0x%x, device id %d, Pipe id %d does not match "
        "entry pipe id %d",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id,
        head_entry->pipe_id);
    return rc;
  }

  tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, head_entry->ptn_index);
  if (tcam_tbl == NULL) {
    LOG_ERROR(
        "%s:%d could not find tcam table with partition %d for entry hdl %d, "
        "tbl 0x%x, device id %d",
        __func__,
        __LINE__,
        head_entry->ptn_index,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_UNEXPECTED;
  }

  /* Is this default entry */
  if (head_entry->is_default) {
    tcam_phy_loc_info_t tcam_loc = {0};
    rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
        tcam_tbl, head_entry->index, true, &tcam_loc);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = tcam_get_default_entry_from_hw(
        tcam_tbl, &tcam_loc, pipe_id, pipe_action_spec, act_fn_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d could not read the default entry with handle %d from the "
          "hardware for tbl "
          "0x%x, device id %d",
          __func__,
          __LINE__,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return rc;
    }

    return PIPE_SUCCESS;
  }

  uint8_t **tcam_word_ptrs = NULL;
  uint64_t *addrs = NULL;
  uint8_t *phy_addrs_map = NULL;

  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_BEGIN(head_entry, tcam_entry) {
    PIPE_MGR_DBGCHK(TCAM_LLP_IS_RANGE_HEAD(tcam_entry));
    TCAM_LLP_GET_RANGE_ENTRY_COUNT(tcam_entry, range_count);
    match_spec_arr = (pipe_tbl_match_spec_t **)PIPE_MGR_CALLOC(
        range_count, sizeof(pipe_tbl_match_spec_t *));
    if (match_spec_arr == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    tcam_phy_loc_info_t tcam_loc;
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_entry->index, &tcam_loc);
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
      goto err_cleanup;
    }
    block_data = get_tcam_block_data(tcam_tbl, &tcam_loc);
    stage_data = get_tcam_stage_data(tcam_tbl, &tcam_loc);

    if (!tcam_word_ptrs) {
      tcam_word_ptrs = (uint8_t **)PIPE_MGR_CALLOC(
          stage_data->pack_format.mem_units_per_tbl_word, sizeof(uint8_t *));
      if (tcam_word_ptrs == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        rc = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
      for (i = 0; i < stage_data->pack_format.mem_units_per_tbl_word; i++) {
        tcam_word_ptrs[i] =
            (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
        if (tcam_word_ptrs[i] == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          rc = PIPE_NO_SYS_RESOURCES;
          goto err_cleanup;
        }
      }
    }
    if (!addrs) {
      addrs = (uint64_t *)PIPE_MGR_CALLOC(
          stage_data->pack_format.mem_units_per_tbl_word, sizeof(uint64_t));
      if (addrs == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        rc = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
    }
    bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
    if (!phy_addrs_map && is_atcam) {
      phy_addrs_map = (uint8_t *)PIPE_MGR_CALLOC(
          stage_data->pack_format.mem_units_per_tbl_word, sizeof(uint8_t));
      for (i = 0; i < stage_data->pack_format.mem_units_per_tbl_word; i++) {
        phy_addrs_map[i] = i;
      }
    }
    uint32_t tcam_index = 0;
    unsigned iter = 0;
    FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_BEGIN(
        tcam_entry, range_entry, tcam_index) {
      match_spec_arr[iter] =
          pipe_mgr_tbl_alloc_match_spec(pipe_match_spec->num_match_bytes);
      if (match_spec_arr[iter] == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        rc = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
      rc = tcam_get_each_entry_from_hw(tcam_tbl_info,
                                       tcam_tbl,
                                       tcam_pipe_tbl,
                                       stage_data,
                                       block_data,
                                       range_entry,
                                       pipe_id,
                                       tcam_index,
                                       match_spec_arr[iter],
                                       pipe_action_spec,
                                       act_fn_hdl,
                                       tcam_word_ptrs);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting tcam entry at idx %d, for tbl 0x%x, device "
            "id %d, err %s",
            __func__,
            __LINE__,
            tcam_index,
            tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(rc));
        goto err_cleanup;
      }
      iter++;
    }
    FOR_ALL_TCAM_LLP_RANGE_ENTRIES_BLOCK_END()
    /* Now for range entries, compress the ranges of individual entries,
     * starting from the head entry
     */
    if (range) {
      rc = pipe_mgr_tcam_compress_decoded_range_entries(
          dev_tgt.device_id,
          tcam_tbl_info->profile_id,
          tbl_hdl,
          match_spec_arr,
          match_spec_arr[0],
          range_count);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in compressing decoded range entries for entry hdl "
            "%d, "
            "tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            entry_hdl,
            tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(rc));
        goto err_cleanup;
      }
    }
    pipe_match_spec =
        pipe_mgr_tbl_copy_match_spec(pipe_match_spec, match_spec_arr[0]);
    if (pipe_match_spec == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    if (match_spec_arr) {
      for (i = 0; i < range_count; i++) {
        if (match_spec_arr[i]) {
          pipe_mgr_tbl_destroy_match_spec(&match_spec_arr[i]);
        }
      }
      PIPE_MGR_FREE(match_spec_arr);
      match_spec_arr = NULL;
    }
  }
  FOR_ALL_TCAM_LLP_RANGE_HEAD_ENTRIES_BLOCK_END()

  /* Try to extract the entry priority and sel grp hdl from hlp state if
   * applicable.
   */
  if (tcam_pipe_tbl->hlp.tcam_entry_db) {
    tcam_hlp_entry_t *hlp_entry =
        pipe_mgr_tcam_entry_get(tcam_pipe_tbl, entry_hdl, 0);
    if (hlp_entry) {
      pipe_match_spec->priority =
          (hlp_entry->group << TCAM_GROUP_ID_SHIFT) | hlp_entry->priority;
      if (pipe_action_spec->pipe_action_datatype_bmap ==
          PIPE_SEL_GRP_HDL_TYPE) {
        pipe_action_spec->sel_grp_hdl =
            unpack_mat_ent_data_as(hlp_entry->mat_data)->sel_grp_hdl;
      }
    }
  } else {
    /* No hlp state, give back the logical entry index instead */
    pipe_match_spec->priority = head_entry->index;
  }

err_cleanup:
  if (tcam_word_ptrs) {
    for (i = 0; i < stage_data->pack_format.mem_units_per_tbl_word; i++) {
      if (tcam_word_ptrs[i]) {
        PIPE_MGR_FREE(tcam_word_ptrs[i]);
      }
    }
    PIPE_MGR_FREE(tcam_word_ptrs);
  }
  if (addrs) {
    PIPE_MGR_FREE(addrs);
  }
  if (match_spec_arr) {
    for (i = 0; i < range_count; i++) {
      if (match_spec_arr[i]) {
        pipe_mgr_tbl_destroy_match_spec(&match_spec_arr[i]);
      }
    }
    PIPE_MGR_FREE(match_spec_arr);
  }
  if (phy_addrs_map) {
    PIPE_MGR_FREE(phy_addrs_map);
  }
  return rc;
}

pipe_status_t pipe_mgr_tcam_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls) {
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  uint8_t pipe_id = 0;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *num_def_hdls = 0;

  for (pipe_id = 0; pipe_id < tcam_tbl_info->no_tcam_pipe_tbls; pipe_id++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_id];
    if (tcam_pipe_tbl->llp.default_ent_set) {
      default_hdls[pipe_id] = tcam_pipe_tbl->llp.default_ent_hdl;
      (*num_def_hdls)++;
    } else {
      default_hdls[pipe_id] = 0;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_invalidate_tcam_idx(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           bf_dev_pipe_t pipe_id,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           uint32_t entry_index) {
  if (pipe_id == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    LOG_ERROR("%s:%d Tcam table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (uint32_t pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls;
       pipe_idx++) {
    if ((1 << pipe_id) & tcam_tbl_info->scope_pipe_bmp[pipe_idx]) {
      tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_idx];
      break;
    }
  }
  if (tcam_pipe_tbl == NULL) {
    return PIPE_INVALID_ARG;
  }

  tcam_pipe_tbl->cur_sess_hdl = sess_hdl;

  // Each partition will have the same number of entries.
  uint32_t entries_per_ptn = tcam_pipe_tbl->tcam_ptn_tbls[0].total_entries;
  uint32_t ptn_entry_index = entry_index % entries_per_ptn;
  uint32_t ptn_idx = entry_index / entries_per_ptn;
  if (ptn_idx > tcam_pipe_tbl->no_ptns - 1) {
    return PIPE_INVALID_ARG;
  }

  tcam_tbl_t *tcam_tbl = &tcam_pipe_tbl->tcam_ptn_tbls[ptn_idx];

  /* Reuse following function as it does exactly what we need. */
  return tcam_fix_unexpected_entry(tcam_tbl, ptn_entry_index, 1);
}

pipe_status_t tcam_fix_unexpected_entry(tcam_tbl_t *tcam_tbl,
                                        uint32_t entry_idx,
                                        uint32_t num_entries) {
  pipe_status_t rc;

  if (num_entries == 0) return PIPE_INVALID_ARG;

  tcam_phy_loc_info_t range_loc;
  for (uint32_t i = 0; i < num_entries; i++) {
    rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, entry_idx + i, &range_loc);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      LOG_ERROR("%s:%d Error getting entry location at %d rc 0x%x",
                __func__,
                __LINE__,
                entry_idx + i,
                rc);
      return rc;
    }

    rc = pipe_mgr_tcam_execute_invalidate_entry(tcam_tbl, &range_loc);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error invalidating entry at %d rc 0x%x",
                __func__,
                __LINE__,
                entry_idx + i,
                rc);
      return rc;
    }
  }
  /* Allow the MAU's lookup pipeline to move forward and any packets which
   * which may have matched the invalidated addresses above to exit
   * the stage's pipeline. */
  rc = pipe_mgr_tcam_execute_nops(tcam_tbl, range_loc.stage_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error adding nops for entry at %d rc 0x%x",
              __func__,
              __LINE__,
              entry_idx,
              rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t tcam_fix_missing_entry(tcam_tbl_t *tcam_tbl,
                                     tcam_llp_entry_t *tcam_entry,
                                     struct pipe_mgr_mat_data *mat_data) {
  pipe_status_t rc;

  tcam_tbl_info_t *tcam_tbl_info = tcam_tbl->tcam_pipe_tbl_p->tcam_tbl_info_p;
  bool is_range = tcam_tbl_info->uses_range;

  pipe_tbl_match_spec_t *orig_match_spec = &mat_data->match_spec;

  /* NOTE: VLA usage. C99 allows it. */
  pipe_tbl_match_spec_t *match_specs[tcam_entry->range_count];

  pipe_tbl_match_spec_t espec[1][tcam_entry->range_count];
  uint8_t value_bits[1][tcam_entry->range_count]
                    [orig_match_spec->num_match_bytes
                         ? orig_match_spec->num_match_bytes
                         : 1];
  uint8_t mask_bits[1][tcam_entry->range_count]
                   [orig_match_spec->num_match_bytes
                        ? orig_match_spec->num_match_bytes
                        : 1];

  pipe_tbl_match_spec_t *expand_specs[1];

  if (is_range) {
    /* Expand the Match spec first */

    uint32_t i, j;
    for (i = 0; i < 1; i++) {
      for (j = 0; j < tcam_entry->range_count; j++) {
        espec[i][j].match_value_bits = value_bits[i][j];
        espec[i][j].match_mask_bits = mask_bits[i][j];
        pipe_mgr_tbl_copy_match_spec(&espec[i][j], orig_match_spec);
      }
    }

    for (i = 0; i < 1; i++) {
      expand_specs[i] = espec[i];
    }

    rc = pipe_mgr_entry_format_tof_range_expand(tcam_tbl_info->dev_id,
                                                tcam_tbl_info->profile_id,
                                                tcam_tbl_info->tbl_hdl,
                                                orig_match_spec,
                                                expand_specs,
                                                1,
                                                tcam_entry->range_count);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error expanding the TCAM range rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          rc);
      return rc;
    }

    for (i = 0; i < 1; i++) {
      for (j = 0; j < tcam_entry->range_count; j++) {
        match_specs[(i * tcam_entry->range_count) + j] = &expand_specs[i][j];
        LOG_DBG("Expanded match spec for block %d entry %d", i, j);
        pipe_mgr_entry_format_log_match_spec(tcam_tbl_info->dev_id,
                                             BF_LOG_DBG,
                                             tcam_tbl_info->profile_id,
                                             tcam_tbl_info->tbl_hdl,
                                             &expand_specs[i][j]);
      }
    }
  } else {
    match_specs[0] = orig_match_spec;
  }

  tcam_phy_loc_info_t dest_loc;
  rc = pipe_mgr_tcam_get_phy_loc_info(tcam_tbl, tcam_entry->index, &dest_loc);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return rc;
  }

  rc = pipe_mgr_tcam_update_hw(tcam_tbl,
                               NULL /*src_loc*/,
                               &dest_loc,
                               PIPE_TCAM_OP_ALLOCATE,
                               match_specs,
                               mat_data,
                               tcam_entry->logical_action_idx,
                               tcam_entry->logical_sel_idx,
                               true /*is_recovery*/);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error processing the placement add for entry 0x%x rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        tcam_entry->entry_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static uint8_t **alloc_word_ptrs(int mem_units_per_tbl_word) {
  uint8_t **tcam_word_ptrs = NULL;
  tcam_word_ptrs =
      (uint8_t **)PIPE_MGR_CALLOC(mem_units_per_tbl_word, sizeof(uint8_t *));
  if (tcam_word_ptrs == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return NULL;
  }
  for (int j = 0; j < mem_units_per_tbl_word; j++) {
    tcam_word_ptrs[j] =
        (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
    if (tcam_word_ptrs[j] == NULL) {
      while (j--) {
        PIPE_MGR_FREE(tcam_word_ptrs[j - 1]);
      }
      PIPE_MGR_FREE(tcam_word_ptrs);
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return NULL;
    }
  }
  return tcam_word_ptrs;
}

static void free_word_ptrs(uint8_t **tcam_word_ptrs,
                           int mem_units_per_tbl_word) {
  if (tcam_word_ptrs != NULL) {
    for (int j = 0; j < mem_units_per_tbl_word; j++) {
      if (tcam_word_ptrs[j]) {
        PIPE_MGR_FREE(tcam_word_ptrs[j]);
      }
    }
    PIPE_MGR_FREE(tcam_word_ptrs);
  }
}

/* This function reads entry from HW for specified tcam index.
 *
 * Function allocates and frees all needed memory for this operation,
 * but internal parts of provided match and action specs must be freed by
 * the caller.
 */
pipe_status_t tcam_hw_raw_entry_get(tcam_tbl_t *tcam_tbl,
                                    bf_dev_pipe_t pipe_id,
                                    tcam_phy_loc_info_t *tcam_loc,
                                    uint32_t ptn_entry_index,
                                    pipe_tbl_match_spec_t *match_spec,
                                    pipe_action_spec_t *act_spec,
                                    pipe_act_fn_hdl_t *act_fn_hdl,
                                    uint32_t num_entries,
                                    bool *entry_valid) {
  pipe_status_t rc = PIPE_UNEXPECTED;

  tcam_tbl_info_t *tcam_tbl_info = tcam_tbl->tcam_pipe_tbl_p->tcam_tbl_info_p;

  tcam_stage_info_t *stage_data;
  tcam_block_data_t *block_data;
  stage_data = get_tcam_stage_data(tcam_tbl, tcam_loc);
  block_data = get_tcam_block_data(tcam_tbl, tcam_loc);

  /* Following need to be declared before any cleanup calls. */
  pipe_tbl_match_spec_t **match_spec_arr = NULL;
  uint8_t arr_size = num_entries;

  uint8_t **tcam_word_ptrs = NULL;
  tcam_word_ptrs =
      alloc_word_ptrs(stage_data->pack_format.mem_units_per_tbl_word);
  if (!tcam_word_ptrs) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  if (act_spec->act_data.num_action_data_bytes != 0 &&
      act_spec->act_data.num_action_data_bytes !=
          tcam_tbl_info->max_act_data_size) {
    LOG_ERROR("%s:%d Provided action spec does not align with match tbl",
              __func__,
              __LINE__);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  act_spec->act_data.num_action_data_bytes = tcam_tbl_info->max_act_data_size;

  if (!act_spec->act_data.action_data_bits &&
      tcam_tbl_info->max_act_data_size) {
    act_spec->act_data.action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
        act_spec->act_data.num_action_data_bytes, sizeof(uint8_t));
  }

  if (!act_spec->act_data.action_data_bits) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  match_spec_arr = (pipe_tbl_match_spec_t **)PIPE_MGR_CALLOC(
      arr_size, sizeof(pipe_tbl_match_spec_t *));
  if (match_spec_arr == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  unsigned rd_ent_num = 0;
  bool is_valid;
  *entry_valid = true;
  while (num_entries--) {
    if (rd_ent_num >= arr_size) {
      PIPE_MGR_DBGCHK(0);
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
    match_spec_arr[rd_ent_num] =
        pipe_mgr_tbl_alloc_match_spec(tcam_tbl_info->num_match_spec_bytes);
    if (match_spec_arr[rd_ent_num] == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    rc = pipe_mgr_tcam_get_entry_from_hw_raw(tcam_tbl,
                                             pipe_id,
                                             stage_data,
                                             block_data,
                                             ptn_entry_index,
                                             match_spec_arr[rd_ent_num],
                                             act_spec,
                                             act_fn_hdl,
                                             tcam_word_ptrs,
                                             &is_valid);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting tcam entry at idx %d, for tbl 0x%x, device "
          "id %d, err %s",
          __func__,
          __LINE__,
          ptn_entry_index,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      goto cleanup;
    }
    rd_ent_num++;
    *entry_valid &= is_valid;
    ptn_entry_index++;
  }

  // For range entries match spect must be created from all related reads.
  if (rd_ent_num > 1) {
    rc = pipe_mgr_tcam_compress_decoded_range_entries(tcam_tbl_info->dev_id,
                                                      tcam_tbl_info->profile_id,
                                                      tcam_tbl_info->tbl_hdl,
                                                      match_spec_arr,
                                                      match_spec_arr[0],
                                                      rd_ent_num);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in compressing decoded range entries for start index %d,"
          "tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          ptn_entry_index,
          tcam_tbl_info->tbl_hdl,
          tcam_tbl_info->dev_id,
          pipe_str_err(rc));
      goto cleanup;
    }
  }

  // Copy will alloc missing internal structs.
  if (pipe_mgr_tbl_copy_match_spec(match_spec, match_spec_arr[0]) == NULL) {
    rc = PIPE_NO_SYS_RESOURCES;
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    goto cleanup;
  }

cleanup:
  free_word_ptrs(tcam_word_ptrs,
                 stage_data->pack_format.mem_units_per_tbl_word);

  if (match_spec_arr) {
    for (uint32_t i = 0; i < arr_size; i++) {
      if (match_spec_arr[i] != NULL) {
        pipe_mgr_tbl_destroy_match_spec(&match_spec_arr[i]);
      }
    }
    PIPE_MGR_FREE(match_spec_arr);
  }

  // Free match/action spec memory only if failed, set to NULL, so caller won't
  // try to double free it.
  if (rc != PIPE_SUCCESS) {
    if (match_spec->match_value_bits) {
      PIPE_MGR_FREE(match_spec->match_value_bits);
      match_spec->match_value_bits = NULL;
    }
    if (match_spec->match_mask_bits) {
      PIPE_MGR_FREE(match_spec->match_mask_bits);
      match_spec->match_mask_bits = NULL;
    }
    if (act_spec->act_data.action_data_bits) {
      PIPE_MGR_FREE(act_spec->act_data.action_data_bits);
      act_spec->act_data.action_data_bits = NULL;
    }
    *entry_valid = false;
  }

  return rc;
}
