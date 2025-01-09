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


#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_tof2_interrupt.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_db.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>

#define PIPE_MGR_TOF2_INTR_PRSR_NUM_ECC_ERR 20

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_xpb_err_handle(bf_dev_id_t dev,
                                             bf_subdev_id_t subdev,
                                             uint32_t intr_address,
                                             uint32_t intr_status_val,
                                             uint32_t enable_hi_addr,
                                             uint32_t enable_lo_addr,
                                             void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev;
  // pipes[pipe].pardereg.pgstnreg.ipbprsr4reg[xpb].ipbreg.glb_group
  //    .intr_stat.stat.stat_0_2
  // pipes[pipe].pardereg.pgstnreg.ipbprsr4reg[xpb].ipbreg.glb_group
  //    .intr_stat.stat.stat_1_2
  // pipes[pipe].pardereg.pgstnreg.epbprsr4reg[xpb].epbreg.glb_group
  //    .intr_stat.stat.stat_0_2
  // pipes[pipe].pardereg.pgstnreg.epbprsr4reg[xpb].epbreg.glb_group
  //    .intr_stat.stat.stat_1_2),
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

/* Correct ecc error in po action ram in parser */
static pipe_status_t pipe_mgr_tof2_parser_action_ram_ecc_correct(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    bf_dev_pipe_t phy_pipe,
    int dir,
    int xpb, /* IPB or EPB number, 0-8 */
    int parser,
    uint32_t row,
    uint32_t blk_id,
    uint64_t *address) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  int data_size = 0;
  uint64_t offset = 0;
  pipe_mgr_drv_ses_state_t *st = NULL;
  pipe_mgr_drv_buf_t *b = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t *src_data = NULL;
  union pipe_parser_bin_config_t *bin_cfg;

  LOG_TRACE(
      "Parser act RAM correct: dev %d pipe[%d].%s[%d].prsr[%d] row %d blk %d",
      dev_id,
      phy_pipe,
      dir ? "EPB" : "IPB",
      xpb,
      parser,
      row,
      blk_id);

  if (row >= TOF2_PARSER_DEPTH) {
    LOG_ERROR(
        "Dev %d log-pipe %d %s[%d] prsr %d dir %d action row %d doesn't exist",
        dev_id,
        log_pipe,
        dir ? "EPB" : "IPB",
        xpb,
        parser,
        dir,
        row);
    return PIPE_INVALID_ARG;
  }

  /* Action ram is 64 bytes wide and is accessed with four 16 byte accesses to
   * incrementing addresses.  Compute the offset from the base address which
   * will also be the offset into our shadow. */
  data_size = PIPE_MGR_TOF2_PO_WORD_WIDTH;
  offset = row * (data_size / 16);

  /* Get the handle for the parser instance used by this parser.  This is
   * multi-parser feature where parsers on the same pipe have different
   * configurations.  Note that the config is per port-group (IPB/EPB) which is
   * a block of four parsers hence the divide by four. */
  profile_id_t prof_id;
  status = pipe_mgr_pipe_to_profile(
      dev_info, log_pipe, &prof_id, __func__, __LINE__);
  bf_dev_pipe_t lowest_pipe = dev_info->profile_info[prof_id]->lowest_pipe;
  if (status != PIPE_SUCCESS) return status;
  pipe_prsr_instance_hdl_t prsr_instance_hdl;
  status = pipe_mgr_get_prsr_instance_hdl(
      dev_id, lowest_pipe, dir, xpb, &prsr_instance_hdl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Fail to get parser instance handle, dev %d, log-pipe %d, %s[%d], "
        "parser %d, error %s",
        __func__,
        dev_id,
        log_pipe,
        dir ? "EPB" : "IPB",
        xpb,
        parser,
        pipe_str_err(status));
    return status;
  }

  /* Bin configuration (shadow data) */
  status = pipe_mgr_prsr_instance_get_bin_cfg(
      dev_id, lowest_pipe, dir, prsr_instance_hdl, &bin_cfg);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Fail to get parser instance shadowed data, dev %d, log-pipe %d, "
        "%s[%d], parser %d, parser instance handle 0x%x, error %s",
        __func__,
        dev_id,
        log_pipe,
        dir ? "EPB" : "IPB",
        xpb,
        parser,
        prsr_instance_hdl,
        pipe_str_err(status));
    return status;
  }

  /* Location to repair. Note that this is an output parameter.  The memory
   * address map for ingress and egress parsers has all ingress parsers in an
   * array of 36 (9 IPBs with 4 parsers each) and all egress parsers in a
   * similar array.  The index of a parser in that array is 4 * xPB number +
   * parser.  However, we use the multi-write functionality of the parser group
   * where we configure the mem_ctl register so that all four parsers in a group
   * respond to the same write address.  So the address we return in the
   * out-param will be the actual address of the parser with the error but the
   * address we use to repair the memory will always be the address of the first
   * parser in the group. */
  int prsr_idx = xpb * 4;
  uint64_t addr = PIPE_PRSR_ADDR(dev_id, dir).tof2.po_action_addr +
                  ((tof2_mem_pipes_array_element_size >> 4) * phy_pipe) +
                  (PIPE_PRSR_ADDR(dev_id, dir).tof2.prsr_step * prsr_idx) +
                  offset;
  *address = addr + PIPE_PRSR_ADDR(dev_id, dir).tof2.prsr_step * parser;

  /* Pointer to shadow data */
  src_data = bin_cfg->tof2.po_action_data[row];
  if (!src_data) {
    LOG_ERROR(
        "Failed to correct prsr error, no data: dev %d log-pipe %d %s[%d] prsr "
        "%d",
        dev_id,
        log_pipe,
        dir ? "EPB" : "IPB",
        xpb,
        parser);
    PIPE_MGR_DBGCHK(src_data);
    return PIPE_UNEXPECTED;
  }

  /* Session state */
  st = pipe_mgr_drv_get_ses_state(
      &pipe_mgr_ctx->int_ses_hndl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
    PIPE_MGR_DBGCHK(st);
    return PIPE_UNEXPECTED;
  }

  /* Allocate a buffer */
  b = pipe_mgr_drv_buf_alloc(
      st->sid, dev_id, data_size, PIPE_MGR_DRV_BUF_BWR, true);
  if (!b) {
    LOG_ERROR(
        "%s : Failed to allocate write blk buffer "
        "error %s",
        __func__,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(b);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(b->addr, src_data, data_size);

  /* Rewrite the parser row again using write block (block id is not used) */
  status = pipe_mgr_drv_blk_wr(&pipe_mgr_ctx->int_ses_hndl,
                               16,
                               data_size / 16,
                               1,
                               addr,
                               1u << log_pipe,
                               b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Write block push for parser ecc correction "
        "error %s",
        __func__,
        pipe_str_err(status));

    pipe_mgr_drv_buf_free(b);
  }
  return status;
}

/* Correct ecc error in word0/word1 in parser */
static pipe_status_t pipe_mgr_tof2_parser_tcam_err_correct(bf_dev_id_t dev,
                                                           bf_dev_pipe_t pipe,
                                                           int dir,
                                                           int xpb,
                                                           int parser,
                                                           int word,
                                                           uint32_t row,
                                                           uint64_t *address) {
  bf_dev_pipe_t phy_pipe = 0;
  int data_size = 0;
  pipe_mgr_drv_ses_state_t *st = NULL;
  pipe_mgr_drv_buf_t *b = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t *src_data = NULL;

  /* Get device info */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (row >= TOF2_PARSER_DEPTH) {
    LOG_ERROR("Dev %d log-pipe %d prsr %d dir %d row %d doesn't exist",
              dev,
              pipe,
              parser,
              dir,
              row);
    return PIPE_INVALID_ARG;
  }
  data_size = PIPE_MGR_TOF2_TCAM_WORD_WIDTH;

  /* Location to repair. The memory address map for ingress and egress parsers
   * has all ingress parsers in an array of 36 (9 IPBs with 4 parsers each) and
   * all egress parsers in a similar array.  The index of a parser in that array
   * is 4 * xPB number + parser.  However, we use the multi-write functionality
   * of the parser group where we configure the mem_ctl register so that all
   * four parsers in a group respond to the same write address which is the
   * address of the first parser in the group. */
  int prsr_idx = xpb * 4 + parser;
  *address = PIPE_PRSR_ADDR(dev, dir).tof2.tcam_addr +
             ((tof2_mem_pipes_array_element_size >> 4) * phy_pipe) +
             (PIPE_PRSR_ADDR(dev, dir).tof2.prsr_step * prsr_idx) + row;
  prsr_idx = xpb * 4;
  uint64_t addr = PIPE_PRSR_ADDR(dev, dir).tof2.tcam_addr +
                  (PIPE_PRSR_ADDR(dev, dir).tof2.prsr_step * prsr_idx) + row;

  /* Pointer to shadow data */
  src_data = PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe, parser / 4)
                 .tof2[dir]
                 .tcam_data[row];
  if (!src_data) {
    LOG_ERROR(
        "Failed to correct prsr tcam, no data: dev %d log-pipe %d dir %d "
        "prsr "
        "%d",
        dev,
        pipe,
        dir,
        parser);
    PIPE_MGR_DBGCHK(src_data);
    return PIPE_UNEXPECTED;
  }

  /* Session state */
  st = pipe_mgr_drv_get_ses_state(
      &pipe_mgr_ctx->int_ses_hndl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }

  /* Allocate a buffer */
  b = pipe_mgr_drv_buf_alloc(
      st->sid, dev, data_size, PIPE_MGR_DRV_BUF_BWR, true);
  if (!b) {
    LOG_ERROR(
        "%s : Failed to allocate write blk buffer "
        "error %s",
        __func__,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(b);
    return PIPE_NO_SYS_RESOURCES;
  }

  LOG_TRACE(
      "Dev %d logPipe %d %s[%d] Prsr %d word %d row %d TCAM parity error, "
      "phyAddr 0x%" PRIx64 " len %d",
      dev,
      pipe,
      dir ? "EPB" : "IPB",
      xpb,
      parser,
      word,
      row,
      *address,
      data_size);

  PIPE_MGR_MEMCPY(b->addr, src_data, data_size);

  status = pipe_mgr_drv_blk_wr(
      &pipe_mgr_ctx->int_ses_hndl, 16, data_size / 16, 1, addr, 1u << pipe, b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Write block push for parser ecc correction "
        "error %s",
        __func__,
        pipe_str_err(status));

    pipe_mgr_drv_buf_free(b);
  }

  return status;
}

/* Handle error in parser */
static uint32_t pipe_mgr_tof2_parser_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  int parser = 0, xpb = 0;
  uint32_t err_addr = 0, blk_id = 0;
  int bitpos = 0, dir = 0;
  uint64_t full_err_addr = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bit_max = PIPE_MGR_TOF2_INTR_PRSR_NUM_ECC_ERR;

  /* Get device info */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  parser = userdata_p->row & 0x3;
  xpb = userdata_p->row >> 2;
  pipe = userdata_p->pipe;
  dir = userdata_p->stage;  // direction stored in stage

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Parser intr dev %d pipe[%d].%s[%d].prsr[%d]:"
      " Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      dir ? "EPB" : "IPB",
      xpb,
      parser,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[].pardereg.pgstnreg.ipbprsr4reg[].prsr[].intr.stat
  // pipes[].pardereg.pgstnreg.epbprsr4reg[].prsr[].intr.stat

  for (bitpos = 0; bitpos < bit_max; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // no_tcam_match_err
        case 1:  // partial_hdr_err
        case 2:  // ctr_range_err
        case 3:  // timeout_iter_err
        case 4:  // timeout_cycle_err
        case 5:  // src_ext_err
        // bit 6 is reserved
        case 7:  // phv_owner_err
        case 8:  // multi_wr_err
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 9:  // aram_sbe
          /* Decode location */
          if (dir == 0) {
            // ingress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[xpb]
                                   .prsr[parser]
                                   .aram_sbe_err_log);
          } else {
            // egress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[xpb]
                                   .prsr[parser]
                                   .aram_sbe_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof2_prsr_reg_main_rspec_aram_sbe_err_log_addr(&data);
          blk_id = getp_tof2_prsr_reg_main_rspec_aram_sbe_err_log_blkid(&data);
          /* Repair memory */
          pipe_mgr_tof2_parser_action_ram_ecc_correct(dev_info,
                                                      pipe,
                                                      phy_pipe,
                                                      dir,
                                                      xpb,
                                                      parser,
                                                      err_addr,
                                                      blk_id,
                                                      &full_err_addr);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_ACT_RAM,
                     "Action RAM single bit error");
          break;

        case 10:  // aram_mbe
          /* Decode location */
          if (dir == 0) {
            // ingress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[xpb]
                                   .prsr[parser]
                                   .aram_mbe_err_log);
          } else {
            // egress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[xpb]
                                   .prsr[parser]
                                   .aram_mbe_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof2_prsr_reg_main_rspec_aram_mbe_err_log_addr(&data);
          blk_id = getp_tof2_prsr_reg_main_rspec_aram_mbe_err_log_blkid(&data);
          /* Repair memory */
          pipe_mgr_tof2_parser_action_ram_ecc_correct(dev_info,
                                                      pipe,
                                                      phy_pipe,
                                                      dir,
                                                      xpb,
                                                      parser,
                                                      err_addr,
                                                      blk_id,
                                                      &full_err_addr);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_ACT_RAM,
                     "Action RAM multi bit error");
          break;

        case 11:  // fcs_err
        case 12:  // csum_err
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 13:  // ibuf_oflow_err
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_INP_BUFF,
                     "Parser input buffer overflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 14:  // ibuf_uflow_err
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_INP_BUFF,
                     "Parser input buffer underflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 15:  // op_fifo_oflow_err
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_OUT_FIFO,
                     "Parser output fifo overflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 16:  // op_fifo_uflow_err
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_OUT_FIFO,
                     "Parser output fifo underflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 17:  // tcam_par_err
          /* Tcam memory will be refreshed periodically */
          /* Decode location */
          if (dir == 0) {
            // ingress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[xpb]
                                   .prsr[parser]
                                   .tcam_par_err_log);
          } else {
            // egress
            address = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[xpb]
                                   .prsr[parser]
                                   .tcam_par_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof2_prsr_reg_main_rspec_tcam_par_err_log_row(&data);
          blk_id = getp_tof2_prsr_reg_main_rspec_tcam_par_err_log_word(&data);
          /* Repair memory */
          pipe_mgr_tof2_parser_tcam_err_correct(
              dev, pipe, dir, xpb, parser, blk_id, err_addr, &full_err_addr);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_PARITY,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_TCAM_PARITY,
                     "Tcam parity error");
          break;

        case 18:  // csum_sbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_CSUM,
                     "Checksum RAM single bit error");
          /* No further action */
          break;

        case 19:  // csum_mbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_CSUM,
                     "Checksum RAM multi bit error");
          /* No further action */
          break;

        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register parser interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_parser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0, parser = 0, xpb = 0;

  LOG_TRACE("Pipe-mgr Registering for parser ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (xpb = 0; xpb < TOF2_NUM_IPB; xpb++) {
      /* IPB */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ipbprsr4reg[xpb]
                       .ipbreg.glb_group.intr_stat.stat.stat_0_2),
          &pipe_mgr_tof2_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ipbprsr4reg[xpb]
                       .ipbreg.glb_group.intr_stat.stat.stat_1_2),
          &pipe_mgr_tof2_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);

      /* EPB */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.epbprsr4reg[xpb]
                       .epbreg.glb_group.intr_stat.stat.stat_0_2),
          &pipe_mgr_tof2_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.epbprsr4reg[xpb]
                       .epbreg.glb_group.intr_stat.stat.stat_1_2),
          &pipe_mgr_tof2_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);

      for (parser = 0; parser < (TOF2_NUM_PARSERS / TOF2_NUM_IPB); parser++) {
        /* Ingress Parser */
        ret =
            lld_int_register_cb(dev,
                                0,
                                offsetof(tof2_reg,
                                         pipes[phy_pipe]
                                             .pardereg.pgstnreg.ipbprsr4reg[xpb]
                                             .prsr[parser]
                                             .intr.stat),
                                &pipe_mgr_tof2_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev, pipe, 0, (xpb << 2) | parser)));
        PIPE_MGR_ASSERT(ret != -1);

        /* Egress Parser */
        ret =
            lld_int_register_cb(dev,
                                0,
                                offsetof(tof2_reg,
                                         pipes[phy_pipe]
                                             .pardereg.pgstnreg.epbprsr4reg[xpb]
                                             .prsr[parser]
                                             .intr.stat),
                                &pipe_mgr_tof2_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev, pipe, 1, (xpb << 2) | parser)));
        PIPE_MGR_ASSERT(ret != -1);
      }
    }
  }
  return PIPE_SUCCESS;
}

/* Enable/disable parser interrupts */
pipe_status_t pipe_mgr_tof2_parser_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                    bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting Parser Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  for (int xpb = 0; xpb < TOF2_NUM_IPB; ++xpb) {
    /* IPB */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.pgstnreg.ipbprsr4reg[xpb]
                     .ipbreg.glb_group.intr_stat.en0.en0_0_2),
        en_val & 0xFFFFFFF2);
    if (ret != PIPE_SUCCESS) return ret;

    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.pgstnreg.ipbprsr4reg[xpb]
                     .ipbreg.glb_group.intr_stat.en0.en0_1_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* EPB */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.pgstnreg.epbprsr4reg[xpb]
                     .epbreg.glb_group.intr_stat.en0.en0_0_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.pgstnreg.epbprsr4reg[xpb]
                     .epbreg.glb_group.intr_stat.en0.en0_1_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    for (int prsr = 0; prsr < 4; ++prsr) {
      /* Ingress Parsers */
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof2_reg,
              pipes[0].pardereg.pgstnreg.ipbprsr4reg[xpb].prsr[prsr].intr.en0),
          en_val & 0xFFFFFE00);
      if (ret != PIPE_SUCCESS) return ret;

      /* Egress Parsers */
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof2_reg,
              pipes[0].pardereg.pgstnreg.epbprsr4reg[xpb].prsr[prsr].intr.en0),
          en_val & 0xFFFFFE00);
      if (ret != PIPE_SUCCESS) return ret;
    }
  }

  return PIPE_SUCCESS;
}
