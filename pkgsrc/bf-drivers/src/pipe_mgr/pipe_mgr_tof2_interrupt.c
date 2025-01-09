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
 * @file pipe_mgr_tof2_interrupt.c
 * @date
 *
 * Tofino2 interrupt handling
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_tof2_interrupt.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_db.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

pipe_status_t pipe_mgr_tof2_intr_reg_wr(rmt_dev_info_t *dev_info,
                                        pipe_bitmap_t *pbm,
                                        pipe_sess_hdl_t shdl,
                                        uint32_t addr,
                                        uint32_t data) {
  pipe_instr_write_reg_t instr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  construct_instr_reg_write(dev_info->dev_id, &instr, addr, data);
  pipe_status_t x = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
  if (x != PIPE_SUCCESS) {
    LOG_ERROR(
        "Dev %d Failed to add interrupt enable write instr (%s), addr 0x%x",
        dev_info->dev_id,
        pipe_str_err(x),
        addr);
    PIPE_MGR_DBGCHK(x == PIPE_SUCCESS);
  }
  return x;
}

pipe_status_t pipe_mgr_tof2_interrupt_en_set_helper(rmt_dev_info_t *dev_info,
                                                    bool enable,
                                                    bool push_now) {
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_status_t status = PIPE_SUCCESS;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();

  if (push_now) {
    status = pipe_mgr_api_enter(shdl);
    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

  status |= pipe_mgr_tof2_mau_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_gfm_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_parser_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_deparser_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_pgr_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_mirror_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_parde_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_lfltr_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_tm_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof2_sbc_interrupt_en_set(dev, enable);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to set interrupt mode (%d)", status);
  }

  /* Push the ilist if this is not bf-driver init and app has called the API
   */
  if (push_now && (!pipe_mgr_is_device_locked(dev))) {
    status = pipe_mgr_drv_ilist_push(&pipe_mgr_ctx->int_ses_hndl, NULL, NULL);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to push interrupt mode instruction list (%d)", status);
    }
  }

  if (push_now) {
    pipe_mgr_api_exit(shdl);
  }
  return status;
}

static pipe_status_t read_phy_mem(pipe_sess_hdl_t sess_hdl,
                                  rmt_dev_info_t *dev_info,
                                  bf_dev_pipe_t log_pipe_id,
                                  rmt_tbl_info_t *rmt_tbls,
                                  uint32_t num_rmt_info) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  bool push_needed = false;
  for (uint32_t i = 0; i < num_rmt_info; i++) {
    rmt_tbl_info_t *rmt_info = &rmt_tbls[i];

    /* Skip everything that is not TCAM. */
    if (rmt_info->mem_type != RMT_MEM_TCAM) {
      continue;
    }

    uint8_t stage_id = rmt_info->stage_id;
    uint32_t mem_word_depth = dev_info->dev_cfg.stage_cfg.tcam_unit_depth;

    for (uint32_t bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
      uint32_t j;
      for (j = 0; j < rmt_info->bank_map[bank].num_tbl_word_blks; j++) {
        rmt_tbl_word_blk_t *tbl_word_blk =
            &rmt_info->bank_map[bank].tbl_word_blk[j];
        uint32_t k;
        for (k = 0; k < rmt_info->pack_format.mem_units_per_tbl_word; k++) {
          mem_id_t mem_id = tbl_word_blk->mem_id[k];

          for (uint32_t a = 0; a < mem_word_depth; ++a) {
            pipe_instr_get_memdata_t instr;
            construct_instr_get_memdata(
                dev_id, &instr, mem_id, a, pipe_mem_type_tcam);
            rc = pipe_mgr_drv_ilist_rd_add(&sess_hdl,
                                           dev_info,
                                           log_pipe_id,
                                           stage_id,
                                           (uint8_t *)&instr,
                                           sizeof(instr));
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error issuing ilist read to dev %d log pipe %d stage "
                  "%d TCAM unit mem_id %d %s",
                  __func__,
                  __LINE__,
                  dev_id,
                  log_pipe_id,
                  stage_id,
                  mem_id,
                  pipe_str_err(rc));
              goto done_looping;
            }
            push_needed = true;
          }
        }
      }
    }
  }

done_looping:
  if (rc == PIPE_SUCCESS) {
    if (push_needed) {
      rc = pipe_mgr_drv_ilist_rd_push(&sess_hdl, NULL, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error starting ilist read, dev %d log pipe %d %s",
                  __func__,
                  __LINE__,
                  dev_id,
                  log_pipe_id,
                  pipe_str_err(rc));
      }
    }
  } else {
    pipe_mgr_drv_ilist_rd_abort(&sess_hdl);
  }
  return rc;
}

static pipe_status_t read_table(pipe_sess_hdl_t sess_hdl,
                                rmt_dev_info_t *dev_info,
                                rmt_dev_profile_info_t *profile_info,
                                pipe_mat_tbl_info_t *mat_tbl_info) {
  for (int pipe_id = PIPE_BITMAP_GET_FIRST_SET(&profile_info->pipe_bmp);
       pipe_id != -1;
       pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, pipe_id)) {
    pipe_status_t rc = read_phy_mem(sess_hdl,
                                    dev_info,
                                    pipe_id,
                                    mat_tbl_info->rmt_info,
                                    mat_tbl_info->num_rmt_info);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error in reading physical memory for dev %d pipe %d tbl %s "
          "0x%x, rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_info->dev_id,
          pipe_id,
          mat_tbl_info->name,
          mat_tbl_info->handle,
          rc,
          pipe_str_err(rc));
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_tcam_read(bf_dev_id_t dev) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    pipe_mgr_api_exit(shdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_is_device_locked(dev)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_SUCCESS;
  }

  /* There is only single shadow ram register per TCAM unit.
   * If during wide data write (long match key) a concurrent read happens to
   * the same TCAM, it is possible that it will end up in the middle of
   * the write command and overwrite shadow ram register before whole write
   * operation is complete. This will result in corrupted data being written
   * to the TCAM.
   * In order to avoid such case logic is divided in two steps:
   *  1. Scrub all MATs using session table lock in order to avoid race
   *     conditions when accessing TCAM table memory (shadow ram register).
   *  2. Scrub all parsers TCAMs. Here race conditions should not happen
   *     since there are no wide writes expected in the parsers TCAMs. */
  for (uint32_t p = 0; p < dev_info->num_pipeline_profiles; p++) {
    unsigned i;
    pipe_mat_tbl_info_t *mat_tbl_info = NULL;
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    for (mat_tbl_info = dev_profile_info->tbl_info_list.mat_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_mat_tbls;
         ++mat_tbl_info, ++i) {
      if (!pipe_mgr_mat_tbl_uses_tcam(dev, mat_tbl_info->handle)) continue;
      /* Lock the table to avoid read/write race conditions. */
      status = pipe_mgr_verify_tbl_access_ignore_err(
          shdl, dev, mat_tbl_info->handle, true);
      /* If table is locked due to batch, just skip it. It will be covered
       * next time. */
      if (status == PIPE_TABLE_LOCKED)
        continue;
      else if (status != PIPE_SUCCESS)
        break;
      status = read_table(shdl, dev_info, dev_profile_info, mat_tbl_info);
      /* Release regardless of error status. */
      pipe_mgr_sm_release(shdl);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "TCAM scrub for tbl %s (0x%x) returned error (%s) on device %d",
            mat_tbl_info->name,
            mat_tbl_info->handle,
            pipe_str_err(status),
            dev_info->dev_id);
        break;
      }
    }
    if (status != PIPE_SUCCESS) {
      break;
    }
  }

  /* We need to read both word0 and word1 memory in both ingress and egress
   * parser TCAMs.  For Tofino-2 the word0 and word1 memory words are on the
   * same address so a single 16B read will cover both word0 and word1 for a
   * given TCAM line.  This means we only have two base addresses instead of the
   * four on Tofino-1. */
  uint64_t pipe_step = tof2_mem_pipes_array_element_size >> 4;
  /* Note the parser step is the same for both ingress and egress. */
  uint64_t prsr_step = tof2_mem_pipes_parde_i_prsr_mem_array_element_size >> 4;
  uint64_t prsr_tcam_base_addrs[] = {
      tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address >> 4,
      tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address >> 4};
  char *prsr_tcam_base_names[] = {"ingress", "egress"};
  int num_bases = sizeof prsr_tcam_base_addrs / sizeof prsr_tcam_base_addrs[0];

  /* For each pipe, scrub all the parser TCAMs */
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    bf_dev_pipe_t phy_pipe;
    status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    if (PIPE_SUCCESS != status) continue;

    /* Scrub the parser TCAMs. */
    for (int i = 0; i < num_bases; ++i) {
      for (int parser = 0; parser < TOF2_NUM_PARSERS; ++parser) {
        uint64_t addr = prsr_tcam_base_addrs[i];
        addr += phy_pipe * pipe_step;
        addr += parser * prsr_step;
        status = pipe_mgr_drv_blk_rd(&shdl,
                                     dev,
                                     PIPE_MGR_TOF2_TCAM_WORD_WIDTH,
                                     TOF2_PARSER_DEPTH,
                                     1,
                                     addr,
                                     NULL,
                                     NULL);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s : Error scrubbing parser TCAM, dev %d phy-pipe %d %s parser "
              "%d error %s",
              __func__,
              dev,
              phy_pipe,
              prsr_tcam_base_names[i],
              parser,
              pipe_str_err(status));
        }
      }
    }
  }

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register for interrupt notifications with lld */
pipe_status_t pipe_mgr_tof2_register_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  pipe_status_t status = PIPE_SUCCESS;

  /* Register for mau interrupt notifications */
  status |= pipe_mgr_tof2_register_mau_interrupt_notifs(dev_info);

  /* Register for parser interrupt notifications */
  status |= pipe_mgr_tof2_register_parser_interrupt_notifs(dev_info);

  /* Register for pgr interrupt notifications */
  status |= pipe_mgr_tof2_register_pgr_interrupt_notifs(dev_info);

  /* Register for ig_deparser interrupt notifications */
  status |= pipe_mgr_tof2_register_deparser_interrupt_notifs(dev_info);

  /* Register for mirror interrupt notifications */
  status |= pipe_mgr_tof2_register_mirror_interrupt_notifs(dev_info);

  /* Register for miscellaneous parde notifications */
  status |= pipe_mgr_tof2_register_parde_misc_interrupt_notifs(dev_info);

  /* Register for learn filter notifications */
  status |= pipe_mgr_tof2_register_lfltr_interrupt_notifs(dev_info);

  /* Register for tm interrupt notifications */
  status |= pipe_mgr_tof2_register_tm_interrupt_notifs(dev_info);

  /* Register for SBC interrupt notifications */
  status |= pipe_mgr_tof2_register_sbc_interrupt_notifs(dev_info->dev_id);

  return status;
}
