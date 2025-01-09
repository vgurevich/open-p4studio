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
 * @file pipe_mgr_p4parser.c
 * @date 09/2016
 *
 * Code related to support parser value set feature or p4 construct
 * parser_value_set.
 */

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <endian.h>
#include <target-utils/third-party/cJSON/cJSON.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino_regs/pipe_top_level.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include "pipe_mgr_log.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_p4parser.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_ibuf.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_db.h"
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

pipemgr_p4parser_ctx_t *g_p4parser_ctx[BF_MAX_DEV_COUNT] = {NULL};
pipe_mgr_mutex_t g_pvs_mutex[PIPE_MGR_NUM_DEVICES];

#define PIPE_MGR_P4PARSER_CTX(_dev, _prof) \
  (g_p4parser_ctx[_dev]->profiles[_prof])
#define PIPEMGR_P4PARSER_PRSR_TCAM_ENTRY_INVALID (0x3ff)

static bool pipe_mgr_pvs_check_prsr_tcam_entry_index_is_allocated(
    pipemgr_p4parser_pvs_t *pvs_node,
    bf_dev_pipe_t pipe_id,
    uint8_t parser_id,
    uint16_t idx) {
  if (pvs_node->pipe_instance[0][pipe_id]
          .parser_instance[parser_id]
          .tcam_rows[idx]
          .allocated == PIPEMGR_P4PARSER_PRSR_TCAM_ALLOCATED) {
    return true;
  }
  return false;
}

static uint16_t pipe_mgr_pvs_get_unused_prsr_tcam_entry_index(
    pipemgr_p4parser_pvs_t *pvs_node,
    bf_dev_pipe_t pipe_id,
    uint8_t parser_id) {
  int i;
  for (i = pvs_node->pvs_size - 1; i >= 0; i--) {
    if (pipe_mgr_pvs_check_prsr_tcam_entry_index_is_allocated(
            pvs_node, pipe_id, parser_id, i) == false) {
      break;
    }
  }
  if (i >= 0) {
    return (i);
  } else {
    return (PIPEMGR_P4PARSER_PRSR_TCAM_ENTRY_INVALID);  // No prsr tcam entry
                                                        // available to use
  }
}

static bool pipe_mgr_pvs_inuse_check(
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node) {
  uint32_t count;

  count = bf_map_count(&gress_pvs_node->htbl);
  if (count != 0) {
    return true;
  }

  return false;
}

static pipe_pvs_hdl_t pipe_mgr_pvs_new_ent_hdl_allocate(
    pipemgr_p4parser_ent_hdl_mgmt_t *ent_hdl_mgr) {
  int new_ent_hdl = 0;

  if (ent_hdl_mgr == NULL) {
    return PIPE_PVS_ENT_HDL_INVALID_HDL;
  }
  /* Allocate the entry handle */
  new_ent_hdl = bf_id_allocator_allocate(ent_hdl_mgr->ent_hdl_allocator);
  if (new_ent_hdl == -1) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_PVS_ENT_HDL_INVALID_HDL;
  }
  return (pipe_pvs_hdl_t)new_ent_hdl;
}

static void pipe_mgr_pvs_ent_hdl_deallocate(
    pipemgr_p4parser_ent_hdl_mgmt_t *ent_hdl_mgr, pipe_pvs_hdl_t ent_hdl) {
  /* Deallocate the entry handle */
  bf_id_allocator_release(ent_hdl_mgr->ent_hdl_allocator, ent_hdl);
  return;
}

static pipe_status_t pvs_htbl_lkup(
    pipemgr_p4parser_global_gress_node_t *node,
    pipe_pvs_hdl_t entry_hdl,
    pipemgr_p4parser_key_htbl_node_t **htbl_node) {
  bf_map_sts_t s = bf_map_get(&node->htbl, entry_hdl, (void **)htbl_node);
  if (s == BF_MAP_NO_KEY) {
    LOG_ERROR("PVS entry handle 0x%x not found in PVS %s, handle 0x%x",
              entry_hdl,
              node->pvs_name,
              node->pvs_handle);
    *htbl_node = NULL;
    return PIPE_INVALID_ARG;
  }
  if (s != BF_MAP_OK) {
    LOG_ERROR(
        "Error getting entry info for entry handle 0x%x from PVS %s, handle "
        "0x%x",
        entry_hdl,
        node->pvs_name,
        node->pvs_handle);
    *htbl_node = NULL;
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_DBGCHK(*htbl_node);
  return *htbl_node ? PIPE_SUCCESS : PIPE_UNEXPECTED;
}

/* ------------------- ASIC Type Specific TCAM Abstraction --------------- */

static void pvs_prepare_trit_words(uint32_t key,
                                   uint32_t mask,
                                   uint32_t *w0,
                                   uint32_t *w1) {
  /* Tofino parser tcam Trit
   *
   *  Word0-bit  Word1-bit       Action
   *
   *   0          0             Mismatch
   *   0          1             match 1
   *   1          0             match 0
   *   1          1             Don't care
   */
  *w0 = *w1 = 0;
  for (int i = 0; i < 32; ++i) {
    uint32_t k = key & (1u << i);
    uint32_t m = mask & (1u << i);
    *w0 |= (((k) ? 0u : 1u) | ((m) ? 0u : 1u)) << i;
    *w1 |= (((k) ? 1u : 0u) | ((m) ? 0u : 1u)) << i;
  }
}
static void pvs_prepare_trit_words_for_disable(uint32_t *w0, uint32_t *w1) {
  /* Tofino parser tcam Trit
   *
   *  Word0-bit  Word1-bit       Action
   *
   *   0          0             Mismatch
   *   0          1             match 1
   *   1          0             match 0
   *   1          1             Don't care
   */
  *w0 = *w1 = 0;
}

#define PIPE_TOF_PRSR_TCAM_CONTAINER_0_START_BIT (0)
#define PIPE_TOF_PRSR_TCAM_CONTAINER_2_START_BIT (16)
#define PIPE_TOF_PRSR_TCAM_CONTAINER_3_START_BIT (24)
#define PIPE_TOF_PRSR_TCAM_PRSR_STATE_START_BIT (32)
#define PIPE_TOF_PRSR_TCAM_CNTR_ZERO_BIT (40)
#define PIPE_TOF_PRSR_TCAM_CNTR_NEG_BIT (41)
#define PIPE_TOF_PRSR_TCAM_VERSION_START_BIT (42)

static void pvs_tof_form_parser_words(pipemgr_p4parser_pvs_tcam_t *entry,
                                      uint8_t parser_state_id,
                                      uint64_t *word0,
                                      uint64_t *word1) {
  uint32_t w0_trit = 0;
  uint32_t w1_trit = 0;
  uint64_t word0_data = 0, word1_data = 0;
  /* tofino:
   * parser tcams are 44 bit wide. Prepare 44bit  bit tcam word0 and 44
   * bit tcam word1. Set lookup_16, lookup_8 x 2, curr_state, ctr_zero, ctr_neg,
   * vers bits  to trit mismatch value (= b00).
   * Parser tcam entry format.
   * bit15..0    lookup_16 match bits / container id 0
   * bit23..16   lookup_8  match bits / container id 2
   * bit31..24   lookup_8  match bits / container id 3
   * bit39..32   curr_state / parser-state
   * bit40       counter zero
   * bit41       counter negative
   * bit42..43   version
   */
  /* Tofino2:
   * lookup_8 x 4,
   * others are as same as tofino, curr_state, ctr_zero, ctr_neg, vers bits
   */
  /* Start with the match fields. */
  if (entry->allocated == PIPEMGR_P4PARSER_PRSR_TCAM_ALLOCATED) {
    pvs_prepare_trit_words(
        entry->encoded_val, entry->encoded_msk, &w0_trit, &w1_trit);
  } else {
    pvs_prepare_trit_words_for_disable(&w0_trit, &w1_trit);
  }
  word0_data = (uint64_t)w0_trit;
  word1_data = (uint64_t)w1_trit;

  /* Add the parser-state-id. */
  word0_data |= ((uint64_t)((~parser_state_id) & 0xff)
                 << PIPE_TOF_PRSR_TCAM_PRSR_STATE_START_BIT);
  word1_data |=
      ((uint64_t)(parser_state_id) << PIPE_TOF_PRSR_TCAM_PRSR_STATE_START_BIT);

  /* Add the verion bits. */
  word0_data |= ((uint64_t)0x3ull << PIPE_TOF_PRSR_TCAM_VERSION_START_BIT);
  word1_data |= ((uint64_t)0x3ull << PIPE_TOF_PRSR_TCAM_VERSION_START_BIT);

  /* Don't care counter zero and negative bits. */
  word0_data |= (0x1ull << PIPE_TOF_PRSR_TCAM_CNTR_ZERO_BIT);
  word1_data |= (0x1ull << PIPE_TOF_PRSR_TCAM_CNTR_ZERO_BIT);
  word0_data |= (0x1ull << PIPE_TOF_PRSR_TCAM_CNTR_NEG_BIT);
  word1_data |= (0x1ull << PIPE_TOF_PRSR_TCAM_CNTR_NEG_BIT);

  /* Data must be little endian. */
  word0_data = htole64(word0_data);
  word1_data = htole64(word1_data);

  *word0 = word0_data;
  *word1 = word1_data;
}

/* Take the software state and program it to the HW. */
static pipe_status_t pvs_tofino_update_parser_tcam(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_bitmap_t pbm,
    uint8_t parser_id,
    bool is_egress,
    pipemgr_p4parser_pvs_tcam_t *entry,
    uint64_t word0_data,
    uint64_t word1_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t log_pipe;
  uint64_t addr[2];
  addr[0] =
      ((is_egress
            ? (pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address +
               parser_id * pipe_top_level_pipes_e_prsr_array_element_size)
            : (pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address +
               parser_id * pipe_top_level_pipes_i_prsr_array_element_size)) >>
       4) +
      entry->tcam_row;
  addr[1] =
      ((is_egress
            ? (pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address +
               parser_id * pipe_top_level_pipes_e_prsr_array_element_size)
            : (pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address +
               parser_id * pipe_top_level_pipes_i_prsr_array_element_size)) >>
       4) +
      entry->tcam_row;
  LOG_TRACE("PVS entry dev %d pbm 0x%x %cprsr %d w0 at 0x%" PRIx64
            " data 0x%" PRIx64,
            dev_info->dev_id,
            pbm.hdr.words[0],
            is_egress ? 'e' : 'i',
            parser_id,
            addr[0],
            word0_data);
  LOG_TRACE("PVS entry dev %d pbm 0x%x %cprsr %d w1 at 0x%" PRIx64
            " data 0x%" PRIx64,
            dev_info->dev_id,
            pbm.hdr.words[0],
            is_egress ? 'e' : 'i',
            parser_id,
            addr[1],
            word1_data);
  uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr[0]);
  pipe_instr_set_memdata_i_only_t instr[2];
  construct_instr_set_memdata_by_addr(dev_info, &instr[0], 8, addr[0]);
  construct_instr_set_memdata_by_addr(dev_info, &instr[1], 8, addr[1]);
  rc = pipe_mgr_drv_ilist_add_2(&sess_hdl,
                                dev_info,
                                &pbm,
                                stage,
                                (uint8_t *)&instr[0],
                                sizeof instr[0],
                                (uint8_t *)&word0_data,
                                sizeof word0_data);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to post instruction to update parser memory on dev %d err %s",
        dev_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }
  rc = pipe_mgr_drv_ilist_add_2(&sess_hdl,
                                dev_info,
                                &pbm,
                                stage,
                                (uint8_t *)&instr[1],
                                sizeof instr[1],
                                (uint8_t *)&word1_data,
                                sizeof word1_data);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to post instruction to update parser memory on dev %d err %s",
        dev_info->dev_id,
        pipe_str_err(rc));
    return PIPE_COMM_FAIL;
  }

  /* Update the interrupt DB with the new entries. */
  PIPE_BITMAP_ITER(&pbm, log_pipe) {
    /* We only need to iterate over the pipes since this API will called only
     * once for DEV_PIPE_ALL but will be called individually for each of the
     * hardware parsers for DEV_PARSER_ALL
     */
    pipe_mgr_set_parser_tcam_shadow(dev_info,
                                    log_pipe,
                                    is_egress,
                                    parser_id,
                                    entry->tcam_row,
                                    sizeof word0_data,
                                    (uint8_t *)&word0_data,
                                    (uint8_t *)&word1_data);
  }

  return rc;
}

/* Take the software state and program it to the HW. */
static pipe_status_t pvs_tofino2_update_parser_tcam(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_bitmap_t pbm,
    uint8_t parser_id,
    bool is_egress,
    pipemgr_p4parser_pvs_tcam_t *entry,
    uint64_t word0_data,
    uint64_t word1_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t log_pipe;
  uint64_t addr;
  uint64_t data[2] = {word0_data, word1_data};

  addr =
      ((is_egress
            ? (tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address +
               parser_id * tof2_mem_pipes_parde_e_prsr_mem_array_element_size)
            : (tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address +
               parser_id *
                   tof2_mem_pipes_parde_i_prsr_mem_array_element_size)) >>
       4) +
      entry->tcam_row;
  LOG_TRACE("PVS entry dev %d pbm 0x%x %cprsr %d w0 at 0x%" PRIx64
            " data0 0x%" PRIx64 " data1 0x%" PRIx64,
            dev_info->dev_id,
            pbm.hdr.words[0],
            is_egress ? 'e' : 'i',
            parser_id,
            addr,
            word0_data,
            word1_data);
  uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr);
  pipe_instr_set_memdata_i_only_t instr;
  construct_instr_set_memdata_by_addr(dev_info, &instr, 16, addr);
  rc = pipe_mgr_drv_ilist_add_2(&sess_hdl,
                                dev_info,
                                &pbm,
                                stage,
                                (uint8_t *)&instr,
                                sizeof instr,
                                (uint8_t *)data,
                                ((sizeof data[0]) * 2));
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to post instruction to update parser memory on dev %d err %s",
        dev_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  /* Update the interrupt DB with the new entries. */
  PIPE_BITMAP_ITER(&pbm, log_pipe) {
    /* We only need to iterate over the pipes since this API will called only
     * once for DEV_PIPE_ALL but will be called individually for each of the
     * hardware parsers for DEV_PARSER_ALL
     */
    pipe_mgr_set_parser_tcam_shadow(dev_info,
                                    log_pipe,
                                    is_egress,
                                    parser_id,
                                    entry->tcam_row,
                                    sizeof word0_data,
                                    (uint8_t *)&word0_data,
                                    (uint8_t *)&word1_data);
  }

  return rc;
}
/* Take the software state and program it to the HW. */
static pipe_status_t pvs_tofino3_update_parser_tcam(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_bitmap_t pbm,
    uint8_t parser_id,
    bool is_egress,
    pipemgr_p4parser_pvs_tcam_t *entry,
    uint64_t word0_data,
    uint64_t word1_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t log_pipe;
  uint64_t addr;
  uint64_t data[2] = {word0_data, word1_data};

  addr =
      ((is_egress
            ? (tof3_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address +
               parser_id * tof3_mem_pipes_parde_e_prsr_mem_array_element_size)
            : (tof3_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address +
               parser_id *
                   tof3_mem_pipes_parde_i_prsr_mem_array_element_size)) >>
       4) +
      entry->tcam_row;
  LOG_TRACE("PVS entry dev %d pbm 0x%x %cprsr %d w0 at 0x%" PRIx64
            " data0 0x%" PRIx64 " data1 0x%" PRIx64,
            dev_info->dev_id,
            pbm.hdr.words[0],
            is_egress ? 'e' : 'i',
            parser_id,
            addr,
            word0_data,
            word1_data);
  uint32_t stage = dev_info->dev_cfg.stage_id_from_addr(addr);
  pipe_instr_set_memdata_i_only_t instr;
  construct_instr_set_memdata_by_addr(dev_info, &instr, 16, addr);
  rc = pipe_mgr_drv_ilist_add_2(&sess_hdl,
                                dev_info,
                                &pbm,
                                stage,
                                (uint8_t *)&instr,
                                sizeof instr,
                                (uint8_t *)data,
                                ((sizeof data[0]) * 2));
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to post instruction to update parser memory on dev %d err %s",
        dev_info->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  /* Update the interrupt DB with the new entries. */
  PIPE_BITMAP_ITER(&pbm, log_pipe) {
    /* We only need to iterate over the pipes since this API will called only
     * once for DEV_PIPE_ALL but will be called individually for each of the
     * hardware parsers for DEV_PARSER_ALL
     */
    pipe_mgr_set_parser_tcam_shadow(dev_info,
                                    log_pipe,
                                    is_egress,
                                    parser_id,
                                    entry->tcam_row,
                                    sizeof word0_data,
                                    (uint8_t *)&word0_data,
                                    (uint8_t *)&word1_data);
  }

  return rc;
}

static pipe_status_t pipe_mgr_user_defined_pipe_scope_get(
    rmt_dev_info_t *dev_info,
    pipemgr_p4parser_pvs_t *pvs_node,
    bf_dev_pipe_t pipeid,
    scope_pipes_t *pipe_set) {
  if (pipeid == DEV_PIPE_ALL) return PIPE_INVALID_ARG;
  if (pipeid >= dev_info->num_active_pipes) return PIPE_INVALID_ARG;
  uint32_t i;
  scope_pipes_t pipe_subset = (1u << pipeid);  // suppose pipeid < 4
  if ((pvs_node == NULL) || (pipe_set == NULL)) return PIPE_INVALID_ARG;
  if (pvs_node->pipe_scope != PIPE_MGR_PVS_SCOPE_USER_DEFINED)
    return PIPE_INVALID_ARG;
  for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
    // Check whether the pipeid is in some pipe_scope.
    if (((pipe_subset & pvs_node->user_define_pipe_scope[i]) != 0)) {
      // Check whether the pipeid is the smallest pipe_id in the pipe_scope.
      if (((pipe_subset ^ pvs_node->user_define_pipe_scope[i]) > pipe_subset) &&
          (pipe_subset != pvs_node->user_define_pipe_scope[i])) {
        *pipe_set = pvs_node->user_define_pipe_scope[i];
        return PIPE_SUCCESS;
      } else if (pipe_subset == pvs_node->user_define_pipe_scope[i]) {
        *pipe_set = pvs_node->user_define_pipe_scope[i];
        return PIPE_SUCCESS;
      }
    }
  }
  return PIPE_INVALID_ARG;
}

static pipe_status_t pipe_mgr_pvs_parser_program_prsr_tcam_this_node(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node,
    pipemgr_p4parser_pvs_t *pvs_node,
    uint32_t parser_value,
    uint32_t parser_value_mask,
    bool is_delete,
    uint16_t tcam_entry_idx,
    pipemgr_p4parser_key_htbl_node_t *htbl_node) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t log_pipe = 0;
  int log_prsr;
  pipemgr_p4parser_pvs_tcam_t entry;
  pipe_bitmap_t pbm;
  uint64_t word0 = 0, word1 = 0;
  uint64_t prsr_map = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }

  /* Determine which physical pipes to write. */
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  if (pipeid == BF_DEV_PIPE_ALL) {
    rc = pipe_mgr_get_pipe_bmp_for_profile(
        dev_info, gress_pvs_node->profile_id, &pbm, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error getting the pipe-bmp for profile-id %d, "
          " rc 0x%x",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle,
          gress_pvs_node->profile_id,
          rc);
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    // In case of pipe dev all, the software state is maintained only in the
    // 1st logical pipe index of this profile
    log_pipe = PIPE_BITMAP_GET_FIRST_SET(&pbm);
  } else {
    if (pvs_node->pipe_scope == PIPE_MGR_PVS_SCOPE_USER_DEFINED) {
      scope_pipes_t pipe_set;
      rc = pipe_mgr_user_defined_pipe_scope_get(
          dev_info, pvs_node, pipeid, &pipe_set);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d - Dev %d, pipe %d, pvs-handle 0x%x, rc 0x%x",
                  __func__,
                  __LINE__,
                  devid,
                  pipeid,
                  gress_pvs_node->pvs_handle,
                  rc);
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
      for (uint32_t i = 0; i < dev_info->num_active_pipes; i++) {
        if ((pipe_set & (1u << i)) != 0) {
          PIPE_BITMAP_SET(&pbm, i);
        }
      }
      log_pipe = pipeid;
    } else {
      PIPE_BITMAP_SET(&pbm, pipeid);
      log_pipe = pipeid;
    }
  }

  /* Update the row with the new key/mask and encode it. */
  /* Populate the entry with the encoded new parser value and mask */
  if (is_delete) {
    entry.allocated = PIPEMGR_P4PARSER_PRSR_TCAM_FREE;
    entry.value = 0;
    entry.mask = 0;
    entry.encoded_val = 0;
    entry.encoded_msk = 0;
  } else {
    entry.allocated = PIPEMGR_P4PARSER_PRSR_TCAM_ALLOCATED;
    entry.value = parser_value;
    entry.mask = parser_value_mask;
    entry.encoded_val = 0;
    entry.encoded_msk = 0;
    for (unsigned int i = 0; i < pvs_node->encoding_cnt; ++i) {
      struct pipe_mgr_pvs_encoding *enc = &pvs_node->encoding[i];
      entry.encoded_val |=
          ((parser_value & enc->src_data_mask) >> enc->masked_src_right_shift)
          << enc->masked_src_left_shift;
      entry.encoded_msk |= ((parser_value_mask & enc->src_data_mask) >>
                            enc->masked_src_right_shift)
                           << enc->masked_src_left_shift;
    }
  }

  if (pipe_mgr_prsr_instance_get_profile(devid,
                                         log_pipe,
                                         pvs_node->gress,
                                         gress_pvs_node->prsr_instance_hdl,
                                         &prsr_map) != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - Dev %d, log pipe %d, %s, invalid prsr instance handle 0x%x",
        __func__,
        __LINE__,
        devid,
        log_pipe,
        (pvs_node->gress == 0) ? "Ingress" : "Egress",
        gress_pvs_node->prsr_instance_hdl);
    rc = PIPE_UNEXPECTED;
    goto done;
  }
  /* Determine which parsers to write. */
  if (parser_id == PIPE_MGR_PVS_PARSER_ALL) {
    // In case of parser all. the software state is maintained only in the 0th
    // logical parser index
    log_prsr = 0;
  } else {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        log_prsr = parser_id;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        log_prsr = parser_id / 4;  // prsr group id
        break;
      default:
        LOG_ERROR("%s:%d Unknown dev_family Dev %d", __func__, __LINE__, devid);
        rc = PIPE_INVALID_ARG;
        goto done;
    }
    if (((prsr_map & (1 << log_prsr)) != 0)) {
      prsr_map = (1 << log_prsr);
    } else {
      prsr_map = 0;
    }
  }

  /* Post instructions to write the TCAM to all the eligible parsers in all
   * the eligible pipes
   */
  for (int state_no = 0; state_no < pvs_node->parser_state_numb; state_no++) {
    entry.tcam_row = pvs_node->pipe_instance[state_no][log_pipe]
                         .parser_instance[log_prsr]
                         .tcam_rows[tcam_entry_idx]
                         .tcam_row;

    /* Form the words that need to be programmed to the parsers */
    pvs_tof_form_parser_words(
        &entry, pvs_node->parser_state_id[state_no], &word0, &word1);

    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (prsr_map == 0) break;  // Cfg only existing in sw shadow
        for (int prsr = 0; prsr < dev_info->dev_cfg.num_prsrs; prsr++) {
          if ((prsr_map & (1 << prsr)) == 0) continue;
          /* Program the row to the parsers. */
          rc = pvs_tofino_update_parser_tcam(
              sess_hdl,
              dev_info,
              pbm,
              prsr,
              pvs_node->gress == BF_DEV_DIR_INGRESS ? false : true,
              &entry,
              word0,
              word1);
          if (PIPE_SUCCESS != rc) {
            LOG_ERROR("Dev %d failed to program pvs, status %s",
                      devid,
                      pipe_str_err(rc));
            goto done;
          }
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        if (prsr_map == 0) break;  // Cfg only existing in sw shadow
        for (int prsr = 0; prsr < (dev_info->dev_cfg.num_prsrs / 4); prsr++) {
          if ((prsr_map & (1 << prsr)) == 0) continue;
          /* Program the row to the parsers. set prsr group id to base prsr id*/
          rc = pvs_tofino2_update_parser_tcam(
              sess_hdl,
              dev_info,
              pbm,
              prsr * 4,
              pvs_node->gress == BF_DEV_DIR_INGRESS ? false : true,
              &entry,
              word0,
              word1);
          if (PIPE_SUCCESS != rc) {
            LOG_ERROR("Dev %d failed to program pvs, status %s",
                      devid,
                      pipe_str_err(rc));
            goto done;
          }
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        if (prsr_map == 0) break;  // Cfg only existing in sw shadow
        for (int prsr = 0; prsr < (dev_info->dev_cfg.num_prsrs / 4); prsr++) {
          if ((prsr_map & (1 << prsr)) == 0) continue;
          /* Program the row to the parsers. set prsr group id to base prsr id*/
          rc = pvs_tofino3_update_parser_tcam(
              sess_hdl,
              dev_info,
              pbm,
              prsr * 4,
              pvs_node->gress == BF_DEV_DIR_INGRESS ? false : true,
              &entry,
              word0,
              word1);
          if (PIPE_SUCCESS != rc) {
            LOG_ERROR("Dev %d failed to program pvs, status %s",
                      devid,
                      pipe_str_err(rc));
            goto done;
          }
        }
        break;
      default:
        LOG_ERROR(
            "%s:%d Unknown dev_family, Dev %d", __func__, __LINE__, devid);
        return PIPE_INVALID_ARG;
    }

    // Only ever update the software state if the hardware write to the parsers
    // went through
    PIPE_MGR_MEMCPY((char *)&pvs_node->pipe_instance[state_no][log_pipe]
                        .parser_instance[log_prsr]
                        .tcam_rows[tcam_entry_idx],
                    (char *)&entry,
                    sizeof(pipemgr_p4parser_pvs_tcam_t));
  }
  // Update the htbl node state
  htbl_node->parser_value = parser_value;
  htbl_node->parser_value_mask = parser_value_mask;
  int gress_idx = pvs_node->gress == BF_DEV_DIR_INGRESS ? 0 : 1;
  htbl_node->encoded_val[gress_idx] = entry.encoded_val;
  htbl_node->encoded_mask[gress_idx] = entry.encoded_msk;

done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_modify_delete_prsr_tcam(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipemgr_p4parser_global_gress_node_t *pvs_node,
    pipe_pvs_hdl_t pvs_entry_handle,
    uint32_t parser_value,
    uint32_t parser_value_mask,
    bool is_delete) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;
  uint8_t gress = 0;
  bf_dev_pipe_t pipeid = 0, log_pipe = 0;
  pipe_bitmap_t pbm;
  uint8_t parser_id = 0, log_prsr;
  pipemgr_p4parser_pvs_t *pvs = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  // Get entry information corresponding to this entry handle from the hashtable
  rc = pvs_htbl_lkup(pvs_node, pvs_entry_handle, &htbl_node);
  if (PIPE_SUCCESS != rc) goto end;
  if (!htbl_node) {
    LOG_ERROR("%s:%d Failed to get htbl node", __func__, __LINE__);
    rc = PIPE_OBJ_NOT_FOUND;
    goto end;
  }

  gress = htbl_node->gress;
  pipeid = htbl_node->pipe_id;
  parser_id = htbl_node->parser_id;

  if (pipeid == BF_DEV_PIPE_ALL) {
    // Get one pipe based on profile
    rc = pipe_mgr_get_pipe_bmp_for_profile(
        dev_info, pvs_node->profile_id, &pbm, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error getting the pipe-bmp for profile-id %d, "
          " rc 0x%x",
          __func__,
          __LINE__,
          devid,
          pvs_node->pvs_handle,
          pvs_node->profile_id,
          rc);
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    // In case of pipe dev all, the software state is maintained only in the
    // 1st logical pipe index of this profile
    log_pipe = PIPE_BITMAP_GET_FIRST_SET(&pbm);

  } else {
    log_pipe = pipeid;
  }

  if (parser_id == PIPE_MGR_PVS_PARSER_ALL) {
    // In case of parser all. the software state is maintained only in the 0th
    // logical parser index
    log_prsr = 0;
  } else {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        log_prsr = parser_id;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        log_prsr = parser_id / 4;  // prsr group id
        break;
      default:
        LOG_ERROR("%s:%d Unknown dev_family Dev %d", __func__, __LINE__, devid);
        return PIPE_INVALID_ARG;
    }
  }

  if (gress == BF_DEV_DIR_ALL) {
    // We do allow the gress scope for a pvs with instance only in ingress or
    // egress to be set as GRESS_ALL. Hence check for NULL pointer here.
    // If both instances are defined, then we only consider the ingress
    // instance
    if (pvs_node->ingress_pvs) {
      pvs = pvs_node->ingress_pvs;
    } else {
      pvs = pvs_node->egress_pvs;
    }
  } else if (gress == BF_DEV_DIR_INGRESS) {
    pvs = pvs_node->ingress_pvs;
  } else {
    pvs = pvs_node->egress_pvs;
  }
  if (pipe_mgr_pvs_check_prsr_tcam_entry_index_is_allocated(
          pvs, log_pipe, log_prsr, htbl_node->tcam_entry_idx) == false) {
    LOG_ERROR(
        "%s:%d Trying to modify an already free tcam row for entry handle "
        "0x%x, tcam entry idx %d pvs %s, handle 0x%x, gress %s, pipe 0x%x, "
        "parser 0x%x",
        __func__,
        __LINE__,
        pvs_entry_handle,
        htbl_node->tcam_entry_idx,
        pvs_node->pvs_name,
        pvs_node->pvs_handle,
        gress == BF_DEV_DIR_INGRESS
            ? "ingress"
            : gress == BF_DEV_DIR_EGRESS ? "egress" : "both gress",
        pipeid,
        parser_id);
    rc = PIPE_INVALID_ARG;
    goto end;
  }

  if (gress == BF_DEV_DIR_ALL || gress == BF_DEV_DIR_INGRESS) {
    if (pvs_node->ingress_pvs) {
      rc = pipe_mgr_pvs_parser_program_prsr_tcam_this_node(
          sess_hdl,
          devid,
          pipeid,
          parser_id,
          pvs_node,
          pvs_node->ingress_pvs,
          parser_value,
          parser_value_mask,
          is_delete,
          htbl_node->tcam_entry_idx,
          htbl_node);
      if (PIPE_SUCCESS != rc) {
        goto end;
      }
    }
  }
  if (gress == BF_DEV_DIR_ALL || gress == BF_DEV_DIR_EGRESS) {
    if (pvs_node->egress_pvs) {
      rc = pipe_mgr_pvs_parser_program_prsr_tcam_this_node(
          sess_hdl,
          devid,
          pipeid,
          parser_id,
          pvs_node,
          pvs_node->egress_pvs,
          parser_value,
          parser_value_mask,
          is_delete,
          htbl_node->tcam_entry_idx,
          htbl_node);
      if (PIPE_SUCCESS != rc) {
        goto end;
      }
    }
  }

  if (is_delete) {
    // Remove the entry from the hashtable
    bf_map_sts_t map_sts = bf_map_rmv(&pvs_node->htbl, pvs_entry_handle);
    if (map_sts == BF_MAP_NO_KEY) {
      LOG_ERROR(
          "%s:%d PVS entry with handle 0x%x not found in PVS database with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          pvs_entry_handle,
          pvs_node->pvs_handle,
          devid);
      rc = PIPE_INVALID_ARG;
      goto end;
    }
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in removing entry with handle 0x%x for PVS with handle "
          "0x%x, device id %d",
          __func__,
          __LINE__,
          pvs_entry_handle,
          pvs_node->pvs_handle,
          devid);
      rc = PIPE_UNEXPECTED;
      goto end;
    }
    if (htbl_node) {
      PIPE_MGR_FREE(htbl_node);
    }
  }
end:
  // cleanup
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_get_entry(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipemgr_p4parser_global_gress_node_t *pvs_node,
    pipe_pvs_hdl_t pvs_entry_handle,
    uint32_t *parser_value,
    uint32_t *parser_value_mask,
    uint8_t *entry_gress,
    bf_dev_pipe_t *entry_pipe,
    uint8_t *entry_parser_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;

  // Get entry information corresponding to this entry handle from the hashtable
  rc = pvs_htbl_lkup(pvs_node, pvs_entry_handle, &htbl_node);
  if (rc == PIPE_SUCCESS) {
    *parser_value = htbl_node->parser_value;
    *parser_value_mask = htbl_node->parser_value_mask;
    if (entry_gress) *entry_gress = htbl_node->gress;
    if (entry_pipe) *entry_pipe = htbl_node->pipe_id;
    if (entry_parser_id) *entry_parser_id = htbl_node->parser_id;
  }

  (void)sess_hdl;
  (void)dev_id;
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_get_hw_entry(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    uint8_t gress,
    bf_dev_pipe_t pipe_id,
    uint8_t parser_id,
    pipemgr_p4parser_global_gress_node_t *pvs_node,
    pipe_pvs_hdl_t pvs_entry_handle,
    uint32_t *parser_value,
    uint32_t *parser_value_mask) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;
  pipemgr_p4parser_pvs_t *pvs;
  unsigned long htbl_key = 0;
  bf_dev_pipe_t phy_pipe;
  uint64_t base0, base1, pipe_step, prsr_step, a0, a1, w0, w1, dont_care;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device %d not found\n", __func__, __LINE__, devid);
    return PIPE_INVALID_ARG;
  }
  // Get entry information corresponding to this entry handle from the hashtable
  htbl_key = (unsigned long)pvs_entry_handle;
  map_sts = bf_map_get(&pvs_node->htbl, htbl_key, (void **)&htbl_node);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR(
        "%s:%d PVS entry with handle 0x%x not found in PVS database with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_node->pvs_handle,
        devid);
    return PIPE_INVALID_ARG;
  }
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry info from entry handle 0x%x for PVS with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_node->pvs_handle,
        devid);
    return PIPE_UNEXPECTED;
  }
  *parser_value = 0;
  *parser_value_mask = 0;
  pvs = (gress) ? pvs_node->egress_pvs : pvs_node->ingress_pvs;
  for (int j = 0; j < pvs->parser_state_numb; j++) {
    uint32_t tcam_row = pvs->pipe_instance[j][pipe_id]
                            .parser_instance[parser_id]
                            .tcam_rows[htbl_node->tcam_entry_idx]
                            .tcam_row;
    // read from hw
    pipe_status_t sts =
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe);
    if (sts) return sts;

    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        base0 = gress ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address
                      : pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address;
        base1 = gress ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address
                      : pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address;
        pipe_step = pipe_top_level_pipes_array_element_size;
        prsr_step = gress ? pipe_top_level_pipes_e_prsr_array_element_size
                          : pipe_top_level_pipes_i_prsr_array_element_size;
        a0 = (base0 + phy_pipe * pipe_step + parser_id * prsr_step) >> 4;
        a1 = (base1 + phy_pipe * pipe_step + parser_id * prsr_step) >> 4;
        lld_subdev_ind_read(devid, subdev, a0 + tcam_row, &dont_care, &w0);
        lld_subdev_ind_read(devid, subdev, a1 + tcam_row, &dont_care, &w1);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        base0 = gress ? tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                      : tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
        pipe_step = tof2_mem_pipes_array_element_size;
        prsr_step = gress ? tof2_mem_pipes_parde_e_prsr_mem_array_element_size
                          : tof2_mem_pipes_parde_i_prsr_mem_array_element_size;
        a0 = (base0 + phy_pipe * pipe_step + parser_id * prsr_step) >> 4;
        lld_subdev_ind_read(devid, subdev, a0 + tcam_row, &w1, &w0);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        base0 = gress ? tof3_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                      : tof3_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
        pipe_step = tof3_mem_pipes_array_element_size;
        prsr_step = gress ? tof3_mem_pipes_parde_e_prsr_mem_array_element_size
                          : tof3_mem_pipes_parde_i_prsr_mem_array_element_size;
        a0 = (base0 + phy_pipe * pipe_step + parser_id * prsr_step) >> 4;
        lld_subdev_ind_read(devid, subdev, a0 + tcam_row, &w1, &w0);
        break;
      default:
        LOG_ERROR("%s:%d Chip type not implemented, dev_id %d\n",
                  __func__,
                  __LINE__,
                  devid);
        return PIPE_INVALID_ARG;
    }
    /* If a bit is zero in both W0 and W1 the entry is invalid (won't match). */
    bool invalid = ((w0 | w1) & 0xFFFFFFFFFFF) != 0xFFFFFFFFFFF;
    /* When decoding state and lookup don't worry about invalid entries (0/0)
     * since we won't display them anyways. */
    if (invalid) {
      LOG_ERROR("%s:%d TCAM entry is invalid.", __func__, __LINE__);
      return PIPE_LLD_FAILED;
    }
    // map w0 w1 back to key/mask
    uint32_t encoded_val = ~w0 & 0xFFFFFFFF;
    uint32_t encoded_msk = (w0 ^ w1) & 0xFFFFFFFF;
    uint32_t parser_value_tmp = 0;
    uint32_t parser_value_mask_tmp = 0;
    for (unsigned int i = 0; i < pvs->encoding_cnt; ++i) {
      struct pipe_mgr_pvs_encoding *enc = &pvs->encoding[i];
      parser_value_tmp |= ((encoded_val >> enc->masked_src_left_shift)
                           << enc->masked_src_right_shift) &
                          enc->src_data_mask;
      parser_value_mask_tmp |= ((encoded_msk >> enc->masked_src_left_shift)
                                << enc->masked_src_right_shift) &
                               enc->src_data_mask;
    }
    if (j == 0) {
      *parser_value = parser_value_tmp;
      *parser_value_mask = parser_value_mask_tmp;
    } else if ((*parser_value != parser_value_tmp) ||
               (*parser_value_mask != parser_value_mask_tmp)) {
      LOG_ERROR(
          "%s:%d TCAM entries in multiple states are not identical. dev id %d, "
          "%s, pipe id %d, prsr id %d, state id %d",
          __func__,
          __LINE__,
          devid,
          gress == 0 ? "ingress" : "egress",
          pipe_id,
          parser_id,
          pvs->parser_state_id[j]);
      return PIPE_UNEXPECTED;
    }
  }
  (void)sess_hdl;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_pvs_parser_program_prsr_tcam(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    uint8_t gress,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    pipemgr_p4parser_global_gress_node_t *pvs_node,
    uint32_t parser_value,
    uint32_t parser_value_mask,
    bool is_delete,
    pipe_pvs_hdl_t *pvs_entry_handle) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;
  uint8_t log_prsr;
  bf_dev_pipe_t log_pipe = 0;
  pipe_bitmap_t pbm;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long htbl_key = 0;
  uint16_t tcam_entry_idx;
  pipemgr_p4parser_pvs_t *pvs;
  *pvs_entry_handle = PIPE_PVS_ENT_HDL_INVALID_HDL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    return PIPE_INVALID_ARG;
  }

  if (pipeid == BF_DEV_PIPE_ALL) {
    // Get one pipe based on profile
    rc = pipe_mgr_get_pipe_bmp_for_profile(
        dev_info, pvs_node->profile_id, &pbm, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error getting the pipe-bmp for profile-id %d, "
          " rc 0x%x",
          __func__,
          __LINE__,
          devid,
          pvs_node->pvs_handle,
          pvs_node->profile_id,
          rc);
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    // In case of pipe dev all, the software state is maintained only in the
    // 1st logical pipe index of this profile
    log_pipe = PIPE_BITMAP_GET_FIRST_SET(&pbm);

  } else {
    // In case of user defined pipe dev, user would pass in the lowest pipeid
    // in one scope, the software state is maintained only in pipeid
    log_pipe = pipeid;
  }

  if (parser_id == PIPE_MGR_PVS_PARSER_ALL) {
    // In case of parser all. the software state is maintained only in the 0th
    // logical parser index
    log_prsr = 0;
  } else {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        log_prsr = parser_id;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        log_prsr = parser_id / 4;  // prsr group id
        break;
      default:
        LOG_ERROR("%s:%d Unknown dev_family Dev %d", __func__, __LINE__, devid);
        return PIPE_INVALID_ARG;
    }
  }

  if (gress == BF_DEV_DIR_ALL) {
    // We do allow the gress scope for a pvs with instance only in ingress or
    // egress to be set as GRESS_ALL. Hence check for NULL pointer here.
    // If both instances are defined, then we only consider the ingress
    // instance
    if (pvs_node->ingress_pvs) {
      pvs = pvs_node->ingress_pvs;
    } else {
      pvs = pvs_node->egress_pvs;
    }
  } else if (gress == BF_DEV_DIR_INGRESS) {
    pvs = pvs_node->ingress_pvs;
  } else {
    pvs = pvs_node->egress_pvs;
  }

  tcam_entry_idx =
      pipe_mgr_pvs_get_unused_prsr_tcam_entry_index(pvs, log_pipe, log_prsr);
  if (tcam_entry_idx == PIPEMGR_P4PARSER_PRSR_TCAM_ENTRY_INVALID) {
    rc = PIPE_NO_SYS_RESOURCES;
    return rc;
  }

  // Generate a new valid entry handle for this entry
  *pvs_entry_handle = pipe_mgr_pvs_new_ent_hdl_allocate(pvs_node->ent_hdl_mgr);
  if (*pvs_entry_handle == PIPE_PVS_ENT_HDL_INVALID_HDL) {
    LOG_ERROR("%s:%d PVS set with handle 0x%x, device id %d is full ",
              __func__,
              __LINE__,
              pvs_node->pvs_handle,
              devid);
    return PIPE_NO_SPACE;
  }

  // Allocate the hashtable node
  htbl_node = (pipemgr_p4parser_key_htbl_node_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipemgr_p4parser_key_htbl_node_t));
  if (!htbl_node) {
    LOG_ERROR(
        "%s:%d Failed to allocate memory for the hashtable node for PVS set "
        "with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        pvs_node->pvs_handle,
        devid);
    rc = PIPE_NO_SYS_RESOURCES;
    goto end;
  }
  // Add the node to the hashtable
  htbl_key = (unsigned long)*pvs_entry_handle;
  map_sts = bf_map_add(&pvs_node->htbl, htbl_key, (void *)htbl_node);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Unable to add node to hashtable node for PVS set with handle "
        "0x%x, device id %d gress %s, pipe 0x%x, parser 0x%x",
        __func__,
        __LINE__,
        pvs_node->pvs_handle,
        devid,
        gress == BF_DEV_DIR_INGRESS
            ? "ingress"
            : gress == BF_DEV_DIR_EGRESS ? "egress" : "both gress",
        pipeid,
        parser_id);
    rc = PIPE_UNEXPECTED;
    goto end;
  }

  if (gress == BF_DEV_DIR_ALL || gress == BF_DEV_DIR_INGRESS) {
    if (pvs_node->ingress_pvs) {
      rc =
          pipe_mgr_pvs_parser_program_prsr_tcam_this_node(sess_hdl,
                                                          devid,
                                                          pipeid,
                                                          parser_id,
                                                          pvs_node,
                                                          pvs_node->ingress_pvs,
                                                          parser_value,
                                                          parser_value_mask,
                                                          is_delete,
                                                          tcam_entry_idx,
                                                          htbl_node);
      if (PIPE_SUCCESS != rc) {
        goto end;
      }
    }
  }
  if (gress == BF_DEV_DIR_ALL || gress == BF_DEV_DIR_EGRESS) {
    if (pvs_node->egress_pvs) {
      rc = pipe_mgr_pvs_parser_program_prsr_tcam_this_node(sess_hdl,
                                                           devid,
                                                           pipeid,
                                                           parser_id,
                                                           pvs_node,
                                                           pvs_node->egress_pvs,
                                                           parser_value,
                                                           parser_value_mask,
                                                           is_delete,
                                                           tcam_entry_idx,
                                                           htbl_node);
      if (PIPE_SUCCESS != rc) {
        goto end;
      }
    }
  }

  htbl_node->gress = gress;
  htbl_node->pipe_id = pipeid;
  htbl_node->parser_id = parser_id;
  htbl_node->pvs_ent_hdl = *pvs_entry_handle;
  htbl_node->tcam_entry_idx = tcam_entry_idx;

  return PIPE_SUCCESS;
end:
  pipe_mgr_pvs_ent_hdl_deallocate(pvs_node->ent_hdl_mgr, *pvs_entry_handle);
  if (htbl_node) {
    PIPE_MGR_FREE(htbl_node);
  }
  bf_map_rmv(&pvs_node->htbl, htbl_key);
  return rc;
}

static void pipe_mgr_pvs_ent_hdl_mgr_deinit(
    pipemgr_p4parser_ent_hdl_mgmt_t *ent_hdl_mgr) {
  if (ent_hdl_mgr == NULL) {
    return;
  }

  bf_id_allocator_destroy(ent_hdl_mgr->ent_hdl_allocator);
  ent_hdl_mgr->ent_hdl_allocator = NULL;
}

static pipe_status_t pipe_mgr_pvs_ent_hdl_mgr_init(
    pipemgr_p4parser_ent_hdl_mgmt_t *ent_hdl_mgr, uint32_t num_entries) {
  bool ZERO_BASED_ALLOCATOR = false;

  if (ent_hdl_mgr == NULL) {
    return PIPE_INVALID_ARG;
  }

  /* Allocate an entry handle allocator */
  ent_hdl_mgr->ent_hdl_allocator =
      bf_id_allocator_new(num_entries, ZERO_BASED_ALLOCATOR);

  if (ent_hdl_mgr->ent_hdl_allocator == NULL) {
    LOG_ERROR("%s : Error in allocating an entry handle allocator", __func__);
    return PIPE_NO_SYS_RESOURCES;
  }

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_pvs_total_entries_get(
    bf_dev_id_t dev_id, pipemgr_p4parser_global_gress_node_t *pvs) {
  uint32_t num_entries = 0;
  rmt_dev_info_t *dev_info = NULL;

  if (pvs->ingress_pvs) {
    num_entries += pvs->ingress_pvs->pvs_size;
  }
  if (pvs->egress_pvs) {
    num_entries += pvs->egress_pvs->pvs_size;
  }

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return num_entries;
  }

  /* The max number of entries for a pvs instance will occur in the case of
   * asymmetric gress, asymmetric pipe and asymmetric parser. Since we have
   * already accounted for the egress part, now account for the pipe and the
   * parser
   */
  num_entries =
      num_entries * dev_info->num_active_pipes * dev_info->dev_cfg.num_prsrs;

  return num_entries;
}

pipe_status_t pipe_mgr_pvs_parser_exclusive_access_start(bf_dev_id_t devid) {
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "%s:%d Invalid device id %d. Unable to acquire per device pvs lock ",
        __func__,
        __LINE__,
        devid);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_LOCK(&g_pvs_mutex[devid]);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_parser_exclusive_access_end(bf_dev_id_t devid) {
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "%s:%d Invalid device id %d. Unable to release per device pvs lock ",
        __func__,
        __LINE__,
        devid);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_UNLOCK(&g_pvs_mutex[devid]);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_init_this_node(
    bf_dev_id_t devid, pipemgr_p4parser_global_gress_node_t *pvs_node) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_map_sts_t map_sts;

  // Initialize the entry hanlde manager for this PVS node
  rc = pipe_mgr_pvs_ent_hdl_mgr_init(
      pvs_node->ent_hdl_mgr, pipe_mgr_pvs_total_entries_get(devid, pvs_node));
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in initializing the pvs entry handle manager for pvs %s "
        "handle 0x%x device id %d",
        __func__,
        __LINE__,
        pvs_node->pvs_name,
        pvs_node->pvs_handle,
        devid);
    return rc;
  }

  map_sts = bf_map_init(&pvs_node->htbl);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in initializing hashtable for pvs %s handle 0x%x device "
        "id %d",
        __func__,
        __LINE__,
        pvs_node->pvs_name,
        pvs_node->pvs_handle,
        devid);
    return PIPE_UNEXPECTED;
  }

  return rc;
}

pipe_status_t pipe_mgr_pvs_init(pipe_sess_hdl_t sess_hdl, bf_dev_id_t devid) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_node;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  profile_id_t prof_id = 0;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("Invalid device id. Unable to initialize global pvs context");
    return PIPE_INVALID_ARG;
  }

  if (!g_p4parser_ctx[devid]) {
    return PIPE_SUCCESS;
  }

  for (prof_id = 0; prof_id < (int)g_p4parser_ctx[devid]->num_profiles;
       prof_id++) {
    map_sts =
        bf_map_get_first(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                         &key,
                         (void **)&gress_node);
    while (map_sts == BF_MAP_OK) {
      rc = pipe_mgr_pvs_init_this_node(devid, gress_node);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initializing the global gress node for pvs %s "
            "handle "
            "0x%x device id %d",
            __func__,
            __LINE__,
            gress_node->pvs_name,
            gress_node->pvs_handle,
            devid);
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
      map_sts =
          bf_map_get_next(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                          &key,
                          (void **)&gress_node);
    }
  }

  (void)sess_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipemgr_pvs_db_init(bf_dev_id_t devid, uint32_t num_profiles) {
  bf_map_sts_t map_sts;
  profile_id_t prof_id = 0;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("Invalid device id. Unable to initialize global pvs context");
    return PIPE_INVALID_ARG;
  }

  // Check for prior allocation.
  if (g_p4parser_ctx[devid] != NULL) {
    LOG_ERROR("%s:%d: Parser value set context previously already built.",
              __func__,
              __LINE__);
    return PIPE_UNEXPECTED;
  }

  /* If there are no profiles (possibly no P4 was given yet) then there is
   * nothing that needs to be done. */
  if (!num_profiles) return PIPE_SUCCESS;

  // Allocate the p4parser_ctx structure for this device id.
  g_p4parser_ctx[devid] = PIPE_MGR_CALLOC(1, sizeof(pipemgr_p4parser_ctx_t));
  if (g_p4parser_ctx[devid] == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate parser context structure for the current "
        "device.",
        __func__,
        __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  g_p4parser_ctx[devid]->profiles =
      PIPE_MGR_CALLOC(num_profiles, sizeof(pipemgr_p4parser_profile_ctx_t));
  if (g_p4parser_ctx[devid]->profiles == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate parser profile context structure for the "
        "current "
        "device.",
        __func__,
        __LINE__);
    PIPE_MGR_FREE(g_p4parser_ctx[devid]);
    g_p4parser_ctx[devid] = NULL;
    return PIPE_NO_SYS_RESOURCES;
  }
  g_p4parser_ctx[devid]->num_profiles = num_profiles;

  // Initialize the hash map
  for (prof_id = 0; prof_id < (int)num_profiles; prof_id++) {
    map_sts = bf_map_init(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in initializing hashtable for device "
          "id %d, prof %d",
          __func__,
          __LINE__,
          devid,
          prof_id);
      return PIPE_UNEXPECTED;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_verify_gress(
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node, uint8_t gress) {
  // Check if the passed in gress is conforms with the gress_symmetricity of
  // the pvs node
  if (gress_pvs_node->gress_scope == PIPE_MGR_PVS_SCOPE_ALL_GRESS) {
    if (gress != BF_DEV_DIR_ALL) {
      return PIPE_INVALID_ARG;
    }
  } else {
    if (gress == BF_DEV_DIR_ALL) {
      return PIPE_INVALID_ARG;
    }
    if (gress == BF_DEV_DIR_INGRESS) {
      if (!gress_pvs_node->ingress_pvs) {
        LOG_ERROR(
            "PVS set with name %s handle 0x%x, does not have a ingress "
            "instance",
            gress_pvs_node->pvs_name,
            gress_pvs_node->pvs_handle);
        return PIPE_INVALID_ARG;
      }
    } else if (gress == BF_DEV_DIR_EGRESS) {
      if (!gress_pvs_node->egress_pvs) {
        LOG_ERROR(
            "PVS set with name %s handle 0x%x, does not have a egress instance",
            gress_pvs_node->pvs_name,
            gress_pvs_node->pvs_handle);
        return PIPE_INVALID_ARG;
      }
    } else {
      return PIPE_INVALID_ARG;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_verify_pipe(
    bf_dev_id_t devid,
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node,
    uint8_t gress,
    bf_dev_pipe_t pipeid) {
  pipemgr_p4parser_pvs_t *pvs_node = NULL;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (pipeid != DEV_PIPE_ALL) {
    profile_id_t prof_id = 0;
    if (pipeid > pipe_mgr_get_num_active_pipes(devid)) {
      return PIPE_INVALID_ARG;
    }
    if (pipe_mgr_pipe_to_profile(
            dev_info, pipeid, &prof_id, __func__, __LINE__) != PIPE_SUCCESS) {
      return PIPE_INVALID_ARG;
    }
    /* Check that the profile of user specified pipe is same as
       pvs-node profile
    */
    if (prof_id != gress_pvs_node->profile_id) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "pvs not valid on pipe %d (profile-id %d), pvs belongs to profile-id "
          "%d",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle,
          pipeid,
          prof_id,
          gress_pvs_node->profile_id);
      return PIPE_INVALID_ARG;
    }
  }

  if (gress == BF_DEV_DIR_INGRESS) {
    pvs_node = gress_pvs_node->ingress_pvs;
  } else if (gress == BF_DEV_DIR_EGRESS) {
    pvs_node = gress_pvs_node->egress_pvs;
  } else {
    // Pick either ingress pvs node or egress pvs node to check pvs scope.
    // When PVS is used on both ingress and egress parser, then parser
    // scope and pipe symmetricity is same for both direction.
    // We do allow the gress scope for a pvs with instance only in ingress or
    // egress to be set as GRESS_ALL. Hence check for NULL pointer here.
    // If both instances are defined, then we only consider the ingress
    // instance as the egress instance will also have the same properties/scopes
    if (gress_pvs_node->ingress_pvs) {
      pvs_node = gress_pvs_node->ingress_pvs;
    } else {
      pvs_node = gress_pvs_node->egress_pvs;
    }
  }

  if (pvs_node->pipe_scope == PIPE_MGR_PVS_SCOPE_ALL_PIPELINES) {
    if (pipeid != DEV_PIPE_ALL) {
      return PIPE_INVALID_ARG;
    }
  } else if (pvs_node->pipe_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PIPELINE) {
    if (pipeid == DEV_PIPE_ALL) {
      return PIPE_INVALID_ARG;
    }
  } else if (pvs_node->pipe_scope == PIPE_MGR_PVS_SCOPE_USER_DEFINED) {
    scope_pipes_t pipe_set;
    if (pipe_mgr_user_defined_pipe_scope_get(
            dev_info, pvs_node, pipeid, &pipe_set) != PIPE_SUCCESS) {
      return PIPE_INVALID_ARG;
    }
  } else {
    return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_verify_parser(
    bf_dev_id_t devid,
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node,
    uint8_t gress,
    uint8_t parser_id) {
  pipemgr_p4parser_pvs_t *pvs_node = NULL;

  if (gress == BF_DEV_DIR_ALL) {
    // We do allow the gress scope for a pvs with instance only in ingress or
    // egress to be set as GRESS_ALL. Hence check for NULL pointer here.
    // If both instances are defined, then we only consider the ingress
    // instance as the egress instance will also have the same properties/scopes
    if (gress_pvs_node->ingress_pvs) {
      pvs_node = gress_pvs_node->ingress_pvs;
    } else {
      pvs_node = gress_pvs_node->egress_pvs;
    }
  } else {
    if (gress == BF_DEV_DIR_INGRESS) {
      pvs_node = gress_pvs_node->ingress_pvs;
    } else {
      pvs_node = gress_pvs_node->egress_pvs;
    }
  }

  if (pvs_node->parser_scope == PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE) {
    if (parser_id != PIPE_MGR_PVS_PARSER_ALL) {
      LOG_ERROR(
          "Dev %d, Invalid parser id. PVS scope is set to all parsers; "
          "argument "
          "specified targets single instance of parser for pvs with handle "
          "0x%x",
          devid,
          gress_pvs_node->pvs_handle);
      return PIPE_INVALID_ARG;
    }
  }
  if (pvs_node->parser_scope == PIPE_MGR_PVS_SCOPE_SINGLE_PARSER) {
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
    if (!dev_info) {
      LOG_ERROR("%s:%d Error in getting device info for device id %d",
                __func__,
                __LINE__,
                devid);
      PIPE_MGR_DBGCHK(dev_info);
      return PIPE_INVALID_ARG;
    }

    if (parser_id == PIPE_MGR_PVS_PARSER_ALL) {
      LOG_ERROR(
          "Dev %d, Invalid parser id. PVS scope is set to single parser; "
          "argument "
          "specified targets all instances of parser for pvs with handle 0x%x",
          devid,
          gress_pvs_node->pvs_handle);
      return PIPE_INVALID_ARG;
    }
    if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
        (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
      if ((parser_id % 4) != 0) {
        LOG_ERROR("Invalid parser id %d for Tofino2, dev %d, with handle 0x%x",
                  parser_id,
                  devid,
                  gress_pvs_node->pvs_handle);
        return PIPE_INVALID_ARG;
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_pvs_parser_gress_scope_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_gress_scope_en *g_scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to set pvs gress scope for handle 0x%x",
        pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(dev_id, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        dev_id);
    goto done;
  }

  LOG_TRACE(
      "Parser value gress scope get for dev :%d pvs-handle :0x%x, pvs-name :%s",
      dev_id,
      pvs_handle,
      gress_pvs_node->pvs_name);

  *g_scope = gress_pvs_node->gress_scope;
done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_gress_scope_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_gress_scope_en g_scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to set pvs gress scope for handle 0x%x",
        pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(dev_id, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        dev_id);
    goto done;
  }

  if (g_scope != PIPE_MGR_PVS_SCOPE_ALL_GRESS &&
      g_scope != PIPE_MGR_PVS_SCOPE_SINGLE_GRESS) {
    rc = PIPE_INVALID_ARG;
    LOG_ERROR(
        "%s:%d Trying to set gress scope to invalid value %d for pvs with "
        "handle 0x%x device id %d",
        __func__,
        __LINE__,
        g_scope,
        pvs_handle,
        dev_id);
  }

  if (g_scope == PIPE_MGR_PVS_SCOPE_ALL_GRESS) {
    /* If we are setting the gress scope to ALL_GRESS, then we need to ensure
     * that we the pipe and the parser scopes of each of the underlying gress
     * PVS instances are same */
    if (gress_pvs_node->ingress_pvs && gress_pvs_node->egress_pvs) {
      if (gress_pvs_node->ingress_pvs->pipe_scope !=
          gress_pvs_node->egress_pvs->pipe_scope) {
        LOG_ERROR(
            "%s:%d Trying to set the gress scope for dev %d pvs with handle "
            "0x%x to ALL GRESS when ingress PVS pipe scope (0x%x) is not equal "
            "to egress PVS pipe scope (0x%x)",
            __func__,
            __LINE__,
            dev_id,
            pvs_handle,
            gress_pvs_node->ingress_pvs->pipe_scope,
            gress_pvs_node->egress_pvs->pipe_scope);
        goto done;
      }
      if (gress_pvs_node->ingress_pvs->parser_scope !=
          gress_pvs_node->egress_pvs->parser_scope) {
        LOG_ERROR(
            "%s:%d Trying to set the gress scope for dev %d pvs with handle "
            "0x%x to ALL GRESS when ingress PVS parser scope (0x%x) is not "
            "equal to egress PVS parser scope (0x%x)",
            __func__,
            __LINE__,
            dev_id,
            pvs_handle,
            gress_pvs_node->ingress_pvs->parser_scope,
            gress_pvs_node->egress_pvs->parser_scope);
        goto done;
      }
    }
  }

  if (pipe_mgr_pvs_inuse_check(gress_pvs_node)) {
    // Attempting to set pvs gress scope when pvs is in use.
    LOG_ERROR(
        "%s:%d Unable to set gress scope for pvs (already in use) with handle "
        "0x%x, device %d. Try again after deleting all entries from this pvs",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        dev_id);
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  gress_pvs_node->gress_scope = g_scope;

  LOG_TRACE(
      "Parser value gress scope set for dev :%d pvs-handle :0x%x, pvs-name :%s"
      "scope :0x%x",
      dev_id,
      pvs_handle,
      gress_pvs_node->pvs_name,
      g_scope);

done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_pipe_scope_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    uint8_t gress,
    pipe_mgr_pvs_pipe_scope_en *p_scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to set pvs gress scope for handle 0x%x",
        pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(dev_id, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        dev_id);
    goto done;
  }

  // Check if the passed in gress is conforms with the gress_symmetricity of
  // the pvs node
  rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to set pipe symmetricity for pvs with handle 0x%x, "
        "device %d, gress 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        dev_id,
        gress);
    goto done;
  }

  if (gress == BF_DEV_DIR_ALL) {
    if (gress_pvs_node->ingress_pvs) {
      *p_scope = gress_pvs_node->ingress_pvs->pipe_scope;
    } else {
      *p_scope = gress_pvs_node->egress_pvs->pipe_scope;
    }
  } else {
    if (gress == BF_DEV_DIR_INGRESS) {
      *p_scope = gress_pvs_node->ingress_pvs->pipe_scope;
    } else {
      *p_scope = gress_pvs_node->egress_pvs->pipe_scope;
    }
  }

  LOG_TRACE(
      "Parser value pipe scope get for dev :%d pvs-handle :0x%x, pvs-name :%s",
      dev_id,
      pvs_handle,
      gress_pvs_node->pvs_name);

done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_set_user_defined_pipe_scope(
    bf_dev_id_t dev_id,
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node,
    pipe_mgr_pvs_prop_args_t args) {
  bf_dev_direction_t gress = args.user_defined.gress;
  uint32_t i, j, num_profile_pipes, zero_scope_numb = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_bitmap_t pbm;
  uint32_t pipes = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  // check
  rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to set pipe symmetricity for pvs with handle 0x%x, "
        "device %d, gress 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        dev_id,
        gress);
    return rc;
  }
  rc = pipe_mgr_get_pipe_bmp_for_profile(
      dev_info, gress_pvs_node->profile_id, &pbm, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - Dev %d, pvs-handle 0x%x, "
        "Error getting the pipe-bmp for profile-id %d, "
        " rc 0x%x",
        __func__,
        __LINE__,
        dev_id,
        gress_pvs_node->pvs_handle,
        gress_pvs_node->profile_id,
        rc);
    return rc;
  }
  num_profile_pipes = PIPE_BITMAP_COUNT(&pbm);
  if (num_profile_pipes > PIPE_MGR_MAX_USER_DEFINED_SCOPES) {
    LOG_ERROR("%s: Num of pipes in profile %d is greater than max scopes %d ",
              __func__,
              num_profile_pipes,
              PIPE_MGR_PVS_SCOPE_USER_DEFINED);
    return PIPE_INVALID_ARG;
  }
  if (sizeof(scope_pipes_t) !=
      sizeof(args.user_defined.user_defined_scope[0])) {
    LOG_ERROR(
        "%s: Size of pipe_bitmap mismatches in PD (%zd) and pipe_mgr (%zd) ",
        __func__,
        sizeof(args.user_defined.user_defined_scope[0]),
        sizeof(scope_pipes_t));
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
    if (args.user_defined.user_defined_scope[i] == 0) {
      zero_scope_numb++;
      continue;
    }
    if (args.user_defined.user_defined_scope[i] &
        ~((1u << dev_info->num_active_pipes) - 1)) {
      LOG_ERROR("%s: User defined Scope set: Invalid pipes specified ",
                __func__);
      return PIPE_INVALID_ARG;
    }
    for (j = 0; j < dev_info->num_active_pipes; j++) {
      if (args.user_defined.user_defined_scope[i] & (1u << j)) {
        if (pipes & (1u << j)) {
          LOG_ERROR(
              "%s: User defined Scope set: Pipe %d belongs to multiple "
              "scopes ",
              __func__,
              j);
          return PIPE_INVALID_ARG;
        }
        pipes |= (1u << j);
        /* Make sure pipe is in profile */
        if (!PIPE_BITMAP_GET(&pbm, j)) {
          LOG_ERROR("%s: Pipe %d not part of table profile ", __func__, j);
          return PIPE_INVALID_ARG;
        }
      }
    }
  }
  if (zero_scope_numb == PIPE_MGR_MAX_USER_DEFINED_SCOPES) {
    LOG_ERROR("%s: User defined Scope set: Num of scopes specified is zero ",
              __func__);
    return PIPE_INVALID_ARG;
  }
  // set
  if (gress == BF_DEV_DIR_ALL) {
    if (gress_pvs_node->ingress_pvs) {
      gress_pvs_node->ingress_pvs->pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
      for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
        gress_pvs_node->ingress_pvs->user_define_pipe_scope[i] =
            args.user_defined.user_defined_scope[i];
      }
    }
    if (gress_pvs_node->egress_pvs) {
      gress_pvs_node->egress_pvs->pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
      for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
        gress_pvs_node->egress_pvs->user_define_pipe_scope[i] =
            args.user_defined.user_defined_scope[i];
      }
    }
  } else {
    if (gress == BF_DEV_DIR_INGRESS) {
      gress_pvs_node->ingress_pvs->pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
      for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
        gress_pvs_node->ingress_pvs->user_define_pipe_scope[i] =
            args.user_defined.user_defined_scope[i];
      }
    } else {
      gress_pvs_node->egress_pvs->pipe_scope = PIPE_MGR_PVS_SCOPE_USER_DEFINED;
      for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
        gress_pvs_node->egress_pvs->user_define_pipe_scope[i] =
            args.user_defined.user_defined_scope[i];
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_pvs_parser_pipe_scope_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_args_t args,
    pipe_mgr_pvs_pipe_scope_en p_scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;
  bf_dev_direction_t gress = 0;
  if (dev_id >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to set pvs gress scope for handle 0x%x",
        pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(dev_id, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        dev_id);
    goto done;
  }
  if (pipe_mgr_pvs_inuse_check(gress_pvs_node)) {
    // Attempting to set pvs pipe symmetricity when pvs is in use.
    LOG_ERROR(
        "%s:%d Unable to set pipe symmetricity for pvs (already in use) with "
        "handle 0x%x, device %d, gress 0x%x. Try again after deleting all "
        "entries from this pvs",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        dev_id,
        gress_pvs_node->gress_scope);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  if (p_scope == PIPE_MGR_PVS_SCOPE_USER_DEFINED) {
    // special case, user define pipe scope
    // verify, then configure to user_define_pipe_scope and pipe_scope
    rc = pipe_mgr_pvs_set_user_defined_pipe_scope(dev_id, gress_pvs_node, args);
    if (rc == PIPE_SUCCESS) {
      LOG_TRACE(
          "Parser value pipe scope set for dev :%d pvs-handle :0x%x, pvs-name "
          ":%s"
          "PIPE_MGR_PVS_SCOPE_USER_DEFINED: 0x%x 0x%x 0x%x 0x%x",
          dev_id,
          pvs_handle,
          gress_pvs_node->pvs_name,
          args.user_defined.user_defined_scope[0],
          args.user_defined.user_defined_scope[1],
          args.user_defined.user_defined_scope[2],
          args.user_defined.user_defined_scope[3]);
    }
  } else {
    // single pipe or all pipe scope
    gress = (bf_dev_direction_t)((uint8_t)args.value);
    // Check if the passed in gress is conforms with the gress_symmetricity of
    // the pvs node
    rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to set pipe symmetricity for pvs with handle 0x%x, "
          "device %d, gress 0x%x",
          __func__,
          __LINE__,
          gress_pvs_node->pvs_handle,
          dev_id,
          gress);
      goto done;
    }
    if (gress == BF_DEV_DIR_ALL) {
      if (gress_pvs_node->ingress_pvs) {
        gress_pvs_node->ingress_pvs->pipe_scope = p_scope;
      }
      if (gress_pvs_node->egress_pvs) {
        gress_pvs_node->egress_pvs->pipe_scope = p_scope;
      }
    } else {
      if (gress == BF_DEV_DIR_INGRESS) {
        gress_pvs_node->ingress_pvs->pipe_scope = p_scope;
      } else {
        gress_pvs_node->egress_pvs->pipe_scope = p_scope;
      }
    }

    LOG_TRACE(
        "Parser value pipe scope set for dev :%d pvs-handle :0x%x, pvs-name :%s"
        "scope :0x%x",
        dev_id,
        pvs_handle,
        gress_pvs_node->pvs_name,
        p_scope);
  }

done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_parser_scope_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipe_pvs_hdl_t pvs_handle,
    uint8_t gress,
    pipe_mgr_pvs_parser_scope_en *scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("Invalid device id. Unable to set pvs scope for handle 0x%x",
              pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        devid);
    goto done;
  }

  // Check if the passed in gress conforms with the gress_symmetricity of
  // the pvs node
  rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to set parser scope for pvs with handle 0x%x, device %d, "
        "gress 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress);
    goto done;
  }

  if (gress == BF_DEV_DIR_INGRESS) {
    *scope = gress_pvs_node->ingress_pvs->parser_scope;
  } else if (gress == BF_DEV_DIR_EGRESS) {
    *scope = gress_pvs_node->egress_pvs->parser_scope;
  } else {
    if (gress_pvs_node->ingress_pvs) {
      *scope = gress_pvs_node->ingress_pvs->parser_scope;
    } else {
      *scope = gress_pvs_node->egress_pvs->parser_scope;
    }
  }

  LOG_TRACE(
      "Parser value parser scope get for dev :%d pvs-handle :0x%x, pvs-name "
      ":%s",
      devid,
      pvs_handle,
      gress_pvs_node->pvs_name);

done:
  return rc;
}

static pipe_status_t pipe_mgr_pvs_parser_parser_scope_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipe_pvs_hdl_t pvs_handle,
    uint8_t gress,
    pipe_mgr_pvs_parser_scope_en scope) {
  (void)sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node;
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("Invalid device id. Unable to set pvs scope for handle 0x%x",
              pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to set scope for pvs with handle 0x%x, "
        "device %d",
        pvs_handle,
        devid);
    goto done;
  }

  LOG_TRACE(
      "Parser value parser scope set for dev :%d pvs-handle :0x%x, pvs-name :%s"
      "scope :0x%x",
      devid,
      pvs_handle,
      gress_pvs_node->pvs_name,
      scope);

  // Check if the passed in gress conforms with the gress_symmetricity of
  // the pvs node
  rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to set parser scope for pvs with handle 0x%x, device %d, "
        "gress 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress);
    goto done;
  }

  // check if pvs is unsed before setting scope. If attempted to
  // set on an in-use PVS (in-use means that one or more set elements of
  // PVS are programmed to identify packet header bits.
  if (pipe_mgr_pvs_inuse_check(gress_pvs_node)) {
    // Attempting to set pvs parser scope when pvs is in use.
    LOG_ERROR(
        "%s:%d Unable to set parser scope for pvs (already in use) with handle "
        "0x%x, device %d, gress 0x%x. Try again after deleting all entries "
        "from this pvs",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress);
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  if (gress == BF_DEV_DIR_INGRESS) {
    gress_pvs_node->ingress_pvs->parser_scope = scope;
  } else if (gress == BF_DEV_DIR_EGRESS) {
    gress_pvs_node->egress_pvs->parser_scope = scope;
  } else {
    if (gress_pvs_node->ingress_pvs) {
      gress_pvs_node->ingress_pvs->parser_scope = scope;
    }
    if (gress_pvs_node->egress_pvs) {
      gress_pvs_node->egress_pvs->parser_scope = scope;
    }
  }
  LOG_TRACE(
      "Parser value parser scope get for dev :%d pvs-handle :0x%x, pvs-name "
      ":%s scope :0x%x",
      devid,
      pvs_handle,
      gress_pvs_node->pvs_name,
      scope);

done:
  return (rc);
}

pipe_status_t pipe_mgr_pvs_parser_tcam_add(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t devid,
                                           pipe_pvs_hdl_t pvs_handle,
                                           uint8_t gress,
                                           bf_dev_pipe_t pipeid,
                                           uint8_t parser_id,
                                           uint32_t parser_value,
                                           uint32_t parser_value_mask,
                                           pipe_pvs_hdl_t *pvs_entry_handle) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node = NULL;
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to add pvs element to pvs set with handle "
        "0x%x",
        pvs_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    goto done;
  }

  // Check if the passed in gress is conforms with the gress scope of
  // the pvs node
  rc = pipe_mgr_pvs_verify_gress(gress_pvs_node, gress);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to add element to pvs with handle 0x%x, device %d, gress "
        "0x%x, pipe 0x%x, parser 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress,
        pipeid,
        parser_id);
    goto done;
  }

  // Check if the passed in pipe conforms with the pipe symmetricity of
  // the pvs node
  rc = pipe_mgr_pvs_verify_pipe(devid, gress_pvs_node, gress, pipeid);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to add element to pvs with handle 0x%x, device %d, gress "
        "0x%x, pipe 0x%x, parser 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress,
        pipeid,
        parser_id);
    goto done;
  }

  // Check if the passed in parser conforms with the parser scope of
  // the pvs node
  rc = pipe_mgr_pvs_verify_parser(devid, gress_pvs_node, gress, parser_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to add element to pvs with handle 0x%x, device %d, gress "
        "0x%x, pipe 0x%x, parser 0x%x",
        __func__,
        __LINE__,
        gress_pvs_node->pvs_handle,
        devid,
        gress,
        pipeid,
        parser_id);
    goto done;
  }

  if (pipe_mgr_hitless_warm_init_in_progress(devid)) {
    /* During warm init, check if we can find a match for this entry add with
     * some entry in the shadow memory read from the hardware. If there is hit
     * then derive the correct pvs entry handle and return that to the user.
     * If we don't find a match, then this indicates that the user is trying
     * to add a new entry during cfg replay. Hence return the correct pvs
     * entry handle to the user and add this node to the list of nodes to be
     * added during warm init end. If we have a scenario  */
  } else {
    rc = pipe_mgr_pvs_parser_program_prsr_tcam(sess_hdl,
                                               devid,
                                               gress,
                                               pipeid,
                                               parser_id,
                                               gress_pvs_node,
                                               parser_value,
                                               parser_value_mask,
                                               false,
                                               pvs_entry_handle);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d PVS entry add failed for pvs set with handle 0x%x, hw parser "
          "id %d, device id %d, pipe id 0x%x, parser value 0x%x, parser value "
          "mask 0x%x",
          __func__,
          __LINE__,
          pvs_handle,
          parser_id,
          devid,
          pipeid,
          parser_value,
          parser_value_mask);
    }
  }

  LOG_TRACE(
      "PVS add. Dev %d pipe %x pvs %s handle 0x%x gress %s value/mask %x/%x "
      "entry handle %x",
      devid,
      pipeid,
      gress_pvs_node->pvs_name,
      pvs_handle,
      gress == BF_DEV_DIR_INGRESS
          ? "ingress"
          : gress == BF_DEV_DIR_EGRESS ? "egress" : "both gress",
      parser_value,
      parser_value_mask,
      *pvs_entry_handle);
done:
  return (rc);
}

pipe_status_t pipe_mgr_pvs_parser_tcam_modify(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle,
                                              uint32_t parser_value,
                                              uint32_t parser_value_mask) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node = NULL;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to update pvs with handle 0x%x, entry %d",
        pvs_handle,
        pvs_entry_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to update pvs with handle 0x%x, entry 0x%x",
        pvs_handle,
        pvs_entry_handle);
    goto done;
  }

  rc = pipe_mgr_pvs_parser_modify_delete_prsr_tcam(sess_hdl,
                                                   devid,
                                                   gress_pvs_node,
                                                   pvs_entry_handle,
                                                   parser_value,
                                                   parser_value_mask,
                                                   false /* is_delete */);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d PVS entry modify failed for entry 0%x pvs set handle 0x%x, "
        "device id %d, parser value 0x%x, parser value mask 0x%x",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_handle,
        devid,
        parser_value,
        parser_value_mask);
  }

  LOG_TRACE(
      "PVS Modify. Dev %d pvs %s handle 0x%x entry-handle "
      "0x%x",
      devid,
      gress_pvs_node->pvs_name,
      pvs_handle,
      pvs_entry_handle);
done:
  return (rc);
}

pipe_status_t pipe_mgr_pvs_parser_tcam_delete(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node = NULL;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to delete pvs entry with pvs handle 0x%x, "
        "entry handle %d",
        pvs_handle,
        pvs_entry_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to update pvs with handle 0x%x, entry 0x%x",
        pvs_handle,
        pvs_entry_handle);
    goto done;
  }

  rc = pipe_mgr_pvs_parser_modify_delete_prsr_tcam(sess_hdl,
                                                   devid,
                                                   gress_pvs_node,
                                                   pvs_entry_handle,
                                                   0,
                                                   0,
                                                   true /* is_delete */);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d PVS entry delete failed for entry 0%x pvs set handle 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_handle,
        devid);
    goto done;
  }

  LOG_TRACE(
      "PVS Delete. Dev %d pvs %s handle 0x%x entry-handle "
      "0x%x",
      devid,
      gress_pvs_node->pvs_name,
      pvs_handle,
      pvs_entry_handle);

  pipe_mgr_pvs_ent_hdl_deallocate(gress_pvs_node->ent_hdl_mgr,
                                  pvs_entry_handle);

done:
  return (rc);
}

pipe_status_t pipe_mgr_pvs_parser_tcam_get(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t devid,
                                           pipe_pvs_hdl_t pvs_handle,
                                           pipe_pvs_hdl_t pvs_entry_handle,
                                           uint32_t *pvs_key,
                                           uint32_t *pvs_mask,
                                           uint8_t *entry_gress,
                                           bf_dev_pipe_t *entry_pipe,
                                           uint8_t *entry_parser_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node = NULL;
  if ((pvs_key == NULL) || (pvs_mask == NULL)) {
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to get pvs entry with pvs handle 0x%x, "
        "entry handle %d",
        pvs_handle,
        pvs_entry_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to get pvs entry with handle 0x%x, entry "
        "0x%x",
        pvs_handle,
        pvs_entry_handle);
    goto done;
  }
  rc = pipe_mgr_pvs_parser_get_entry(sess_hdl,
                                     devid,
                                     gress_pvs_node,
                                     pvs_entry_handle,
                                     pvs_key,
                                     pvs_mask,
                                     entry_gress,
                                     entry_pipe,
                                     entry_parser_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d PVS entry get failed for entry 0%x pvs set handle 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_handle,
        devid);
    goto done;
  }
done:
  return (rc);
}

static pipe_status_t pipe_mgr_pvs_get_hw_verify(
    bf_dev_id_t devid,
    pipemgr_p4parser_global_gress_node_t *gress_pvs_node,
    uint8_t *gress,
    bf_dev_pipe_t *pipeid,
    uint8_t *parser_id) {
  pipe_status_t rc;
  pipe_bitmap_t pbm;
  uint64_t prsr_map = 0;
  pipemgr_p4parser_pvs_t *pvs_node = NULL;

  if (!gress_pvs_node || !gress || !pipeid || !parser_id) {
    LOG_ERROR("%s:%d - Null arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  if (*gress == BF_DEV_DIR_ALL) {
    // check whether single gress scope, if yes, return error
    if (gress_pvs_node->gress_scope != PIPE_MGR_PVS_SCOPE_ALL_GRESS) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error not support getting all gress configuration while gress scope "
          "is not ALL_GRESS",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle);
      return PIPE_INVALID_ARG;
    }
    // check whether all gress scope, if yes, take a valid gress
    *gress =
        (gress_pvs_node->ingress_pvs) ? BF_DEV_DIR_INGRESS : BF_DEV_DIR_EGRESS;
  }

  if (*gress == BF_DEV_DIR_INGRESS) {
    pvs_node = gress_pvs_node->ingress_pvs;
  } else {
    pvs_node = gress_pvs_node->egress_pvs;
  }
  if (pvs_node == NULL) {
    LOG_ERROR(
        "%s:%d - Dev %d, pvs-handle 0x%x, "
        "Error not support getting %s configuration",
        __func__,
        __LINE__,
        devid,
        gress_pvs_node->pvs_handle,
        (gress == BF_DEV_DIR_INGRESS) ? "ingress" : "egress");
    return PIPE_INVALID_ARG;
  }
  if (*pipeid == DEV_PIPE_ALL) {
    if (pvs_node->pipe_scope != PIPE_MGR_PVS_SCOPE_ALL_PIPELINES) {
      // check whether single pipe scope or user define scope, return error
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error not support getting all pipe configuration while pipe scope "
          "is not ALL_PIPE",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle);
      return PIPE_INVALID_ARG;
    }
    // check whether all pipe scope, get one pipe based on profile
    rc = pipe_mgr_get_pipe_bmp_for_profile(
        dev_info, gress_pvs_node->profile_id, &pbm, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error getting the pipe-bmp for profile-id %d, "
          " rc 0x%x",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle,
          gress_pvs_node->profile_id,
          rc);
      return rc;
    }
    *pipeid = PIPE_BITMAP_GET_FIRST_SET(&pbm);
  }
  if (*parser_id == PIPE_MGR_PVS_PARSER_ALL) {
    // check whether single prsr scope, return error
    if (pvs_node->parser_scope != PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error not support getting all parser configuration while prsr scope "
          "is not ALL_PRSR",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle);
      return PIPE_INVALID_ARG;
    }
    // check whether all prsr scope, get one prsr based on prsr instance
    rc = pipe_mgr_prsr_instance_get_profile(
        devid, *pipeid, *gress, gress_pvs_node->prsr_instance_hdl, &prsr_map);

    if ((rc != PIPE_SUCCESS) || (prsr_map == 0)) {
      LOG_ERROR(
          "%s:%d - Dev %d, pvs-handle 0x%x, "
          "Error getting valid parser map"
          " rc 0x%x",
          __func__,
          __LINE__,
          devid,
          gress_pvs_node->pvs_handle,
          rc);
      return PIPE_INVALID_ARG;
    }
    *parser_id = 0;
    while ((prsr_map >> (*parser_id) & 1) == 0) {
      (*parser_id)++;
    }
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) (*parser_id) *= 4;
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) (*parser_id) *= 4;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_parser_tcam_hw_get(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              uint8_t gress,
                                              bf_dev_pipe_t pipeid,
                                              uint8_t parser_id,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle,
                                              uint32_t *pvs_key,
                                              uint32_t *pvs_mask) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *gress_pvs_node = NULL;
  uint8_t gress_verified = gress;
  bf_dev_pipe_t pipeid_verified = pipeid;
  uint8_t parser_id_verified = parser_id;

  if ((pvs_key == NULL) || (pvs_mask == NULL)) {
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to get pvs entry with pvs handle 0x%x, "
        "entry handle %d",
        pvs_handle,
        pvs_entry_handle);
    rc = PIPE_INVALID_ARG;
    goto done;
  }
  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &gress_pvs_node);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid pvs handle. Unable to get pvs entry with handle 0x%x, entry "
        "0x%x",
        pvs_handle,
        pvs_entry_handle);
    goto done;
  }
  // verify gress pipe_id and parser_id
  rc = pipe_mgr_pvs_get_hw_verify(devid,
                                  gress_pvs_node,
                                  &gress_verified,
                                  &pipeid_verified,
                                  &parser_id_verified);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Invalid parameter. Unable to get pvs entry with handle 0x%x, entry "
        "0x%x",
        pvs_handle,
        pvs_entry_handle);
    goto done;
  }
  rc = pipe_mgr_pvs_parser_get_hw_entry(sess_hdl,
                                        devid,
                                        gress_verified,
                                        pipeid_verified,
                                        parser_id_verified,
                                        gress_pvs_node,
                                        pvs_entry_handle,
                                        pvs_key,
                                        pvs_mask);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d PVS entry get failed for entry 0%x pvs set handle 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        pvs_entry_handle,
        pvs_handle,
        devid);
    goto done;
  }
done:
  return (rc);
}

pipe_status_t pipe_mgr_pvs_parser_entry_handle_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipe_pvs_hdl_t pvs_handle,
    uint8_t gress,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    uint32_t pvs_key,
    uint32_t pvs_mask,
    pipe_pvs_hdl_t *pvs_entry_handle) {
  (void)sess_hdl;

  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_p4parser_global_gress_node_t *pvs_node = NULL;
  unsigned long htbl_key = 0;
  pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;
  uint8_t parser_id_sp;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || !pvs_entry_handle) {
    return PIPE_INVALID_ARG;
  }

  if (parser_id == PIPE_MGR_PVS_PARSER_ALL) {
    // In case of parser all. the software state is maintained only in the 0th
    // logical parser index
    parser_id_sp = parser_id;
  } else {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        parser_id_sp = parser_id;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        parser_id_sp = (parser_id / 4) * 4;
        break;
      default:
        LOG_ERROR("%s:%d Unknown dev_family Dev %d", __func__, __LINE__, devid);
        return PIPE_INVALID_ARG;
    }
  }

  rc = pipe_mgr_pvs_get_global_gress_node(devid, pvs_handle, &pvs_node);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  bf_map_sts_t map_sts =
      bf_map_get_first(&pvs_node->htbl, &htbl_key, (void **)&htbl_node);
  if (map_sts != BF_MAP_OK) {
    LOG_TRACE(
        "%s dev %d PVS %s pvs-handle 0x%x gress %d pipe %d prsr %d key 0x%08x "
        "mask 0x%08x, no entries",
        __func__,
        devid,
        pvs_node->pvs_name,
        pvs_node->pvs_handle,
        gress,
        pipeid,
        parser_id,
        pvs_key,
        pvs_mask);
    return PIPE_OBJ_NOT_FOUND;
  }

  while (map_sts == BF_MAP_OK) {
    if ((htbl_node->gress == gress) && (htbl_node->pipe_id == pipeid) &&
        (htbl_node->parser_id == parser_id_sp) &&
        (htbl_node->parser_value == pvs_key) &&
        (htbl_node->parser_value_mask == pvs_mask)) {
      *pvs_entry_handle = (pipe_pvs_hdl_t)htbl_key;
      rc = PIPE_SUCCESS;
      break;
    }
    map_sts = bf_map_get_next(&pvs_node->htbl, &htbl_key, (void **)&htbl_node);
    if (map_sts != BF_MAP_OK) {
      rc = PIPE_OBJ_NOT_FOUND;
      break;
    }
  }
  return rc;
}

pipe_status_t pipe_mgr_pvs_parser_property_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t value,
    pipe_mgr_pvs_prop_args_t args) {
  pipe_mgr_pvs_gress_scope_en gress_scope = 0;
  bf_dev_direction_t gress = 0;
  pipe_mgr_pvs_pipe_scope_en pipe_scope = 0;
  pipe_mgr_pvs_parser_scope_en parser_scope = 0;
  LOG_TRACE("%s: Invoked for sess %d, dev %d, pvs 0x%x, property %d",
            __func__,
            sess_hdl,
            dev_id,
            pvs_handle,
            property);

  switch (property) {
    case PIPE_MGR_PVS_GRESS_SCOPE:
      gress_scope = (pipe_mgr_pvs_gress_scope_en)value.value;
      return pipe_mgr_pvs_parser_gress_scope_set(
          sess_hdl, dev_id, pvs_handle, gress_scope);
    case PIPE_MGR_PVS_PIPE_SCOPE:
      pipe_scope = (pipe_mgr_pvs_pipe_scope_en)value.value;
      return pipe_mgr_pvs_parser_pipe_scope_set(
          sess_hdl, dev_id, pvs_handle, args, pipe_scope);
    case PIPE_MGR_PVS_PARSER_SCOPE:
      gress = (bf_dev_direction_t)args.value;
      parser_scope = (pipe_mgr_pvs_parser_scope_en)value.value;
      return pipe_mgr_pvs_parser_parser_scope_set(
          sess_hdl, dev_id, pvs_handle, gress, parser_scope);
    default:
      LOG_ERROR(
          "%s:%d Invalid property type passed for dev %d pvs with handle 0x%x "
          "property %d",
          __func__,
          __LINE__,
          dev_id,
          pvs_handle,
          property);
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_parser_property_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t *value,
    pipe_mgr_pvs_prop_args_t args) {
  pipe_mgr_pvs_gress_scope_en gress_scope = 0;
  bf_dev_direction_t gress = 0;
  pipe_mgr_pvs_pipe_scope_en pipe_scope = 0;
  pipe_mgr_pvs_parser_scope_en parser_scope = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  LOG_TRACE("%s: Invoked for sess %d, dev %d, pvs 0x%x, property %d",
            __func__,
            sess_hdl,
            dev_id,
            pvs_handle,
            property);
  switch (property) {
    case PIPE_MGR_PVS_GRESS_SCOPE:
      rc = pipe_mgr_pvs_parser_gress_scope_get(
          sess_hdl, dev_id, pvs_handle, &gress_scope);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to get property value for dev %d pvs with handle "
            "0x%x property %d",
            __func__,
            __LINE__,
            dev_id,
            pvs_handle,
            property);
        return rc;
      }
      value->value = (uint32_t)gress_scope;
      break;
    case PIPE_MGR_PVS_PIPE_SCOPE:
      gress = (bf_dev_direction_t)args.value;
      rc = pipe_mgr_pvs_parser_pipe_scope_get(
          sess_hdl, dev_id, pvs_handle, gress, &pipe_scope);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to get property value for dev %d pvs with handle "
            "0x%x property %d",
            __func__,
            __LINE__,
            dev_id,
            pvs_handle,
            property);
        return rc;
      }
      value->value = (uint32_t)pipe_scope;
      break;
    case PIPE_MGR_PVS_PARSER_SCOPE:
      gress = (bf_dev_direction_t)args.value;
      rc = pipe_mgr_pvs_parser_parser_scope_get(
          sess_hdl, dev_id, pvs_handle, gress, &parser_scope);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to get property value for dev %d pvs with handle "
            "0x%x property %d",
            __func__,
            __LINE__,
            dev_id,
            pvs_handle,
            property);
        return rc;
      }
      value->value = (uint32_t)parser_scope;
      break;
    default:
      LOG_ERROR(
          "%s:%d Invalid property type passed for dev %d pvs with handle 0x%x "
          "property %d",
          __func__,
          __LINE__,
          dev_id,
          pvs_handle,
          property);
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pvs_get_next_handles(bf_dev_id_t dev_id,
                                            pipe_pvs_hdl_t pvs_handle,
                                            bf_dev_direction_t gress,
                                            bf_dev_pipe_t pipe_id,
                                            uint8_t parser_id,
                                            int entry_handle,
                                            int handle_count,
                                            pipe_pvs_hdl_t *entry_handles) {
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (handle_count <= 0) {
    LOG_ERROR(
        "%s dev %d pvs handle 0x%x gress %d pipe %x prsr %x cur-hdl %d, bad "
        "cnt %d",
        __func__,
        dev_id,
        pvs_handle,
        gress,
        pipe_id,
        parser_id,
        entry_handle,
        handle_count);
    return PIPE_INVALID_ARG;
  }
  if (entry_handles == NULL) {
    LOG_ERROR(
        "%s dev %d pvs handle 0x%x gress %d pipe %x prsr %x cur-hdl %d cnt %d, "
        "entry_handles is NULL",
        __func__,
        dev_id,
        pvs_handle,
        gress,
        pipe_id,
        parser_id,
        entry_handle,
        handle_count);
    return PIPE_INVALID_ARG;
  }

  /* Initialize all returned handles to -1. */
  for (int i = 0; i < handle_count; ++i) entry_handles[i] = -1;

  pipemgr_p4parser_global_gress_node_t *node = NULL;
  rc = pipe_mgr_pvs_get_global_gress_node(dev_id, pvs_handle, &node);
  if (rc != PIPE_SUCCESS || !node) {
    LOG_ERROR(
        "%s dev %d pvs handle 0x%x gress %d pipe %x prsr %x cur-hdl %d cnt %d, "
        "unknown pvs handle",
        __func__,
        dev_id,
        pvs_handle,
        gress,
        pipe_id,
        parser_id,
        entry_handle,
        handle_count);
    return PIPE_INVALID_ARG;
  }

  /* Validate gress scope. */
  pipemgr_p4parser_pvs_t *pvs = NULL;
  if (node->gress_scope == PIPE_MGR_PVS_SCOPE_ALL_GRESS) {
    /* Configured scope is all gress, allow first only from all. */
    if (gress != BF_DEV_DIR_ALL) {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x configured gress scope is ALL, can "
          "only get from BF_DEV_DIR_ALL (%d), not %d.",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle,
          BF_DEV_DIR_ALL,
          gress);
      return PIPE_INVALID_ARG;
    }
  } else {
    /* Configured scope is per gress, allow first from either all or specific
     * gress.  However, if the PVS does not exist on the requested gress return
     * an error. */
    if (gress == BF_DEV_DIR_INGRESS) {
      if (!node->ingress_pvs) {
        LOG_ERROR(
            "%s Dev %d PVS %s handle 0x%x: Requested ingress but PVS is not "
            "present in the ingress",
            __func__,
            dev_id,
            node->pvs_name,
            pvs_handle);
        return PIPE_INVALID_ARG;
      }
    } else if (gress == BF_DEV_DIR_EGRESS) {
      if (!node->egress_pvs) {
        LOG_ERROR(
            "%s Dev %d PVS %s handle 0x%x: Requested egress but PVS is not "
            "present in the egress",
            __func__,
            dev_id,
            node->pvs_name,
            pvs_handle);
        return PIPE_INVALID_ARG;
      }
    } else if (gress == BF_DEV_DIR_ALL) {
    } else {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x, illegal value %d passed for gress",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle,
          gress);
      return PIPE_INVALID_ARG;
    }
  }
  switch (gress) {
    case BF_DEV_DIR_ALL:
      pvs = node->ingress_pvs ? node->ingress_pvs : node->egress_pvs;
      break;
    case BF_DEV_DIR_INGRESS:
      pvs = node->ingress_pvs;
      break;
    case BF_DEV_DIR_EGRESS:
      pvs = node->egress_pvs;
      break;
  }
  if (!pvs) {
    LOG_ERROR("%s Dev %d PVS %s handle 0x%x, cannot find PVS details",
              __func__,
              dev_id,
              node->pvs_name,
              pvs_handle);
    PIPE_MGR_DBGCHK(pvs);
    return PIPE_UNEXPECTED;
  }

  /* Validate the pipe_id. */
  if (pvs->pipe_scope == PIPE_MGR_PVS_SCOPE_ALL_PIPELINES) {
    /* PVS scope is all pipelines so only allow get first for all pipes. */
    if (pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x configured pipe scope is ALL, can only "
          "get from pipe-all.",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle);
      return PIPE_INVALID_ARG;
    }
  } else {
    /* Allow any legal pipe-id value. */
    if (pipe_id != BF_DEV_PIPE_ALL && pipe_id >= dev_info->num_active_pipes) {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x cannot get from pipe %d, only %d "
          "pipes are active",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle,
          pipe_id,
          dev_info->num_active_pipes);
      return PIPE_INVALID_ARG;
    }
  }

  /* Validate the parser-id. */
  if (pvs->parser_scope == PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE) {
    if (parser_id != PIPE_MGR_PVS_PARSER_ALL) {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x configured parser scope is ALL, can "
          "only get from PIPE_MGR_PVS_PARSER_ALL.",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle);
      return PIPE_INVALID_ARG;
    }
  } else {
    /* PVS is configured per parser id so allow any legal parser id. */
    if (parser_id != PIPE_MGR_PVS_PARSER_ALL &&
        parser_id >= dev_info->dev_cfg.num_prsrs) {
      LOG_ERROR(
          "%s Dev %d PVS %s handle 0x%x cannot get from parser %d, only "
          "%d parsers are present",
          __func__,
          dev_id,
          node->pvs_name,
          pvs_handle,
          parser_id,
          dev_info->dev_cfg.num_prsrs);
      return PIPE_INVALID_ARG;
    }
  }

  /* Keep getting handles until we find the first that matches the callers
   * requirements.  If no handles are found then return a not-found status. */
  rc = PIPE_OBJ_NOT_FOUND;
  int h = entry_handle;
  if (h == -1)
    h = bf_id_allocator_get_first(node->ent_hdl_mgr->ent_hdl_allocator);
  else
    h = bf_id_allocator_get_next(node->ent_hdl_mgr->ent_hdl_allocator, h);
  for (; h != -1;
       h = bf_id_allocator_get_next(node->ent_hdl_mgr->ent_hdl_allocator, h)) {
    /* Look up the entry info for this handle. */
    pipemgr_p4parser_key_htbl_node_t *htbl_node = NULL;
    pipe_status_t x = pvs_htbl_lkup(node, h, &htbl_node);
    if (x != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(x == PIPE_SUCCESS);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* If a specific gress was requested check for it. */
    if (gress == BF_DEV_DIR_INGRESS && htbl_node->gress != 0) continue;
    if (gress == BF_DEV_DIR_EGRESS && htbl_node->gress != 1) continue;

    /* If a specific pipe was requested check for it. */
    if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != htbl_node->pipe_id) continue;

    /* If a specific parser was requesed check for it. */
    if (parser_id != PIPE_MGR_PVS_PARSER_ALL &&
        parser_id != htbl_node->parser_id)
      continue;

    /* Found a handle that matched our criteria. */
    *entry_handles = h;
    ++entry_handles;
    rc = PIPE_SUCCESS;
    if (--handle_count == 0) break;
  }

  return rc;
}

pipe_status_t pipe_mgr_pvs_get_count(bf_dev_id_t dev_id,
                                     pipe_pvs_hdl_t pvs_handle,
                                     bf_dev_direction_t gress,
                                     bf_dev_pipe_t pipe_id,
                                     uint8_t parser_id,
                                     bool read_from_hw,
                                     uint32_t *count) {
  (void)read_from_hw;
  pipe_status_t rc = PIPE_SUCCESS;
  if (count == NULL) return PIPE_INVALID_ARG;

  /* Use the handle iterator to get the count. */
  pipe_pvs_hdl_t cur_hdl = 0;
  rc = pipe_mgr_pvs_get_next_handles(
      dev_id, pvs_handle, gress, pipe_id, parser_id, -1, 1, &cur_hdl);
  if (rc == PIPE_OBJ_NOT_FOUND) {
    *count = 0;
    return PIPE_SUCCESS;
  } else if (rc != PIPE_SUCCESS) {
    return rc;
  }
  *count = 1;

  while (true) {
    pipe_pvs_hdl_t next_hdl = 0;
    rc = pipe_mgr_pvs_get_next_handles(
        dev_id, pvs_handle, gress, pipe_id, parser_id, cur_hdl, 1, &next_hdl);
    if (rc != PIPE_SUCCESS) {
      return PIPE_SUCCESS;
    } else {
      cur_hdl = next_hdl;
      *count += 1;
    }
  }
  return PIPE_SUCCESS;
}

/* cleanup on device removal.
 * Free up all allocated memory to store ctx json data.
 */
void pipe_mgr_free_pvs(bf_dev_id_t devid) {
  pipemgr_p4parser_global_gress_node_t *gress_node;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipemgr_p4parser_key_htbl_node_t *htbl_node;
  unsigned long key;
  profile_id_t prof_id = 0;

  if (!g_p4parser_ctx[devid]) return;

  for (prof_id = 0; prof_id < (int)g_p4parser_ctx[devid]->num_profiles;
       prof_id++) {
    pipe_mgr_free_pvs_helper(devid, prof_id);
    while ((map_sts = bf_map_get_first_rmv(
                &PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                &key,
                (void **)&gress_node)) == BF_MAP_OK) {
      while ((map_sts = bf_map_get_first_rmv(
                  &gress_node->htbl, &key, (void **)&htbl_node)) == BF_MAP_OK) {
        PIPE_MGR_FREE(htbl_node);
      }
      bf_map_destroy(&gress_node->htbl);
      pipe_mgr_pvs_ent_hdl_mgr_deinit(gress_node->ent_hdl_mgr);
      PIPE_MGR_FREE(gress_node);
    }

    bf_map_destroy(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl);
  }
  if (g_p4parser_ctx[devid]->profiles) {
    PIPE_MGR_FREE(g_p4parser_ctx[devid]->profiles);
  }
  PIPE_MGR_FREE(g_p4parser_ctx[devid]);
  g_p4parser_ctx[devid] = NULL;
  PIPE_MGR_LOCK_DESTROY(&g_pvs_mutex[devid]);
}

pipe_status_t pipe_mgr_pvs_parser_tcam_clear(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_pvs_hdl_t pvs_handle,
                                             bf_dev_direction_t gress,
                                             bf_dev_pipe_t pipe_id,
                                             uint8_t parser_id) {
  pipe_pvs_hdl_t entry_hdl = 0;

  pipe_status_t rc = pipe_mgr_pvs_get_next_handles(
      dev_id, pvs_handle, gress, pipe_id, parser_id, -1, 1, &entry_hdl);
  while (rc == PIPE_SUCCESS) {
    rc = pipe_mgr_pvs_parser_tcam_delete(
        sess_hdl, dev_id, pvs_handle, entry_hdl);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    rc = pipe_mgr_pvs_get_next_handles(
        dev_id, pvs_handle, gress, pipe_id, parser_id, -1, 1, &entry_hdl);
  }
  if (rc == PIPE_OBJ_NOT_FOUND) rc = PIPE_SUCCESS;
  return rc;
}
