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
 * @file pipe_mgr_tof2_mau_interrupt.c
 * @date
 *
 * Tofino2 MAU interrupt handling.
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

#include "pipe_mgr_idle.h"
#include "pipe_mgr_phy_mem_map.h"

#define INTR_SYNTH2PORT_0 (0x1u << 0)
#define INTR_SYNTH2PORT_1 (0x1u << 1)

/* Helper function translating the amod_protocol errlog register
 * (pipes.mau.cfg_regs.amod_protocol_errlog) value to a string describing the
 * protocol error. */
static inline const char *amod_proto_log_to_string(int errlog) {
  switch (errlog) {
    case 0:
      return "End w/o start";
    case 1:
      return "Back to back start";
    case 2:
      return "Timeout waiting for bubble";
    case 3:
      return "Start/End thread mismatch";
    case 4:
      return "Non-CSR instruction used";
    case 5:
      return "FIFO overflow";
  }
  return "Unknown";
}

/* Returns pointer to table name, or NULL if handle is zero. */
static inline const char *get_tbl_name(bf_dev_id_t dev,
                                       pipe_tbl_hdl_t tbl_hdl,
                                       const char *where,
                                       const int line) {
  return (tbl_hdl) ? pipe_mgr_get_tbl_name(dev, tbl_hdl, where, line) : NULL;
}

/* Handle mau_cfg interrupt */
static uint32_t pipe_mgr_tof2_intr_mau_cfg_handle(bf_dev_id_t dev,
                                                  bf_subdev_id_t subdev_id,
                                                  uint32_t intr_address,
                                                  uint32_t intr_status_val,
                                                  uint32_t enable_hi_addr,
                                                  uint32_t enable_lo_addr,
                                                  void *userdata) {
  (void)subdev_id;
  UNUSED(enable_lo_addr);
  UNUSED(userdata);

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get the pipe and stage from the address. */
  bf_dev_pipe_t phy_pipe = dev_info->dev_cfg.dir_addr_get_pipe_id(intr_address);
  dev_stage_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(intr_address);
  bf_dev_pipe_t pipe = phy_pipe;
  pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(dev_info, phy_pipe, &pipe);

  // Status register:
  // pipes[].mau[].cfg_regs.intr_status_mau_cfg

  /* Check for SBE events in the PBus Station (pbs) request/response interface.
   * These would be transient corrected errors with no functional impact.
   * Also include SBE events in the AMOD fifo here; these would be SBEs in the
   * fifo holding CSR writes queued for an Atomic Mod CSR update. */
  uint32_t sbe_ints = intr_status_val & (0x555 | (1u << 22));

  /* Check for MBE events in the PBus Station (pbs) request/response interface.
   * These would be transient uncorrected errors leading to unknown lost events
   * such as posting a lock-ack for a stats table or receiving a configuration
   * write.
   * Also include MBE events in the AMOD fifo here; these would be MBEs in the
   * fifo holding CSR writes queued for an Atomic Mod CSR update. */
  uint32_t mbe_ints = intr_status_val & (0xAAA | (1u << 23));

  uint32_t to_disable = 0;

  /* If SBE or MBE events occurred check the error logs and raise an event. */
  if (sbe_ints || mbe_ints) {
    /* CREQ Data */
    if (intr_status_val & 0x3) {
      bool creq_data_mbe = (intr_status_val & 0x2) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_creq_errlog[creq_data_mbe ? 1 : 0]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d CREQ Data %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                creq_data_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(creq_data_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 creq_data_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU CREQ Data error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (creq_data_mbe) to_disable |= 0x2;
    }

    /* CREQ Ctrl */
    if (intr_status_val & 0xC) {
      bool creq_ctrl_mbe = (intr_status_val & 0x8) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_creq_errlog[creq_ctrl_mbe ? 3 : 2]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d CREQ Ctrl %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                creq_ctrl_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(creq_ctrl_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 creq_ctrl_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU CREQ Ctrl error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (creq_ctrl_mbe) to_disable |= 0x8;
    }

    /* CRESP Data */
    if (intr_status_val & 0x30) {
      bool crsp_data_mbe = (intr_status_val & 0x20) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_cresp_errlog[crsp_data_mbe ? 1 : 0]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d CRESP Data %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                crsp_data_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(crsp_data_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 crsp_data_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU CRESP Data error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (crsp_data_mbe) to_disable |= 0x20;
    }

    /* CRESP Ctrl */
    if (intr_status_val & 0xC0) {
      bool crsp_ctrl_mbe = (intr_status_val & 0x80) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_cresp_errlog[crsp_ctrl_mbe ? 3 : 2]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d CRESP Ctrl %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                crsp_ctrl_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(crsp_ctrl_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 crsp_ctrl_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU CRESP Ctrl error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (crsp_ctrl_mbe) to_disable |= 0x80;
    }

    /* SREQ Data */
    if (intr_status_val & 0x300) {
      bool sreq_data_mbe = (intr_status_val & 0x200) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_sreq_errlog[sreq_data_mbe ? 1 : 0]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d SREQ Data %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                sreq_data_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(sreq_data_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 sreq_data_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU SREQ Data error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (sreq_data_mbe) to_disable |= 0x200;
    }

    /* SREQ Ctrl */
    if (intr_status_val & 0xC00) {
      bool sreq_ctrl_mbe = (intr_status_val & 0x800) != 0;
      uint32_t data = 0;
      uint32_t addr =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .cfg_regs.pbs_sreq_errlog[sreq_ctrl_mbe ? 3 : 2]);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d SREQ Ctrl %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                sreq_ctrl_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(sreq_ctrl_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 0,
                 sreq_ctrl_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC
                               : BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU SREQ Ctrl error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (sreq_ctrl_mbe) to_disable |= 0x800;
    }

    /* AMOD FIFO */
    if (intr_status_val & (3u << 22)) {
      int amod_mbe_bit = 2u << 22;
      bool amod_mbe = (intr_status_val & amod_mbe_bit) != 0;
      uint32_t data = 0;
      uint32_t addr =
          amod_mbe
              ? offsetof(
                    tof2_reg,
                    pipes[phy_pipe].mau[stage].cfg_regs.amod_ecc_mbe_errlog)
              : offsetof(
                    tof2_reg,
                    pipes[phy_pipe].mau[stage].cfg_regs.amod_ecc_sbe_errlog);
      pipe_mgr_interrupt_read_register(dev, addr, &data);
      LOG_ERROR("Dev %d log-pipe %d stage %d AMOD FIFO %cBE, log 0x%x",
                dev,
                pipe,
                stage,
                amod_mbe ? 'M' : 'S',
                data);
      BF_ERR_EVT(
          amod_mbe ? BF_ERR_SEV_FATAL : BF_ERR_SEV_CORRECTABLE,
          dev,
          pipe,
          stage,
          0,
          amod_mbe ? BF_ERR_TYPE_MULTI_BIT_ECC : BF_ERR_TYPE_SINGLE_BIT_ECC,
          BF_ERR_BLK_MAU,
          BF_ERR_LOC_NONE,
          "MAU AMOD FIFO error");
      /* If we are raising a fatal event disable the interrupt so we do not keep
       * processing it. */
      if (amod_mbe) to_disable |= (2u << 22);
    }
  }

  /* Check for timeout errors on pipe commands (commands from SBC to MAU).
   * There should only be 5 queues but the field is 7 bits wide, the two msbs
   * are not used.  We check all 7 bits regardless. */
  for (int q = 0; q < 7; ++q) {
    if (intr_status_val & (0x1u << (12 + q))) {
      LOG_ERROR("Dev %d log-pipe %d stage %d pipe command timeout, queue %d",
                dev,
                pipe,
                stage,
                q);
      BF_ERR_EVT(BF_ERR_SEV_FATAL,
                 dev,
                 pipe,
                 stage,
                 q,
                 BF_ERR_TYPE_GENERIC,
                 BF_ERR_BLK_MAU,
                 BF_ERR_LOC_NONE,
                 "MAU Pipe Command Timeout");
      to_disable |= (1u << (12 + q));
    }
  }

  /* Check for writes to address holes. */
  if (intr_status_val & (1u << 19)) {
    uint32_t lo = 0, hi = 0;
    uint32_t addr_lo = offsetof(
        tof2_reg, pipes[phy_pipe].mau[stage].cfg_regs.q_hole_acc_errlog_lo);
    pipe_mgr_interrupt_read_register(dev, addr_lo, &lo);
    uint32_t addr_hi = offsetof(
        tof2_reg, pipes[phy_pipe].mau[stage].cfg_regs.q_hole_acc_errlog_hi);
    pipe_mgr_interrupt_read_register(dev, addr_hi, &hi);
    LOG_WARN("Dev %d log-pipe %d stage %d Write to addr hole, 0x%x%08x",
             dev,
             pipe,
             stage,
             hi,
             lo);
  }

  /* Check for timeouts on station requests indicating an idle message was not
   * able to be sent from the MAU to SBC.  This is fatal because SW cannot miss
   * a lock ACK or barrier ACK. */
  if (intr_status_val & (1u << 20)) {
    uint32_t data = 0;
    uint32_t addr = offsetof(
        tof2_reg, pipes[phy_pipe].mau[stage].cfg_regs.sreq_idle_timeout_errlog);
    pipe_mgr_interrupt_read_register(dev, addr, &data);
    LOG_ERROR("Dev %d log-pipe %d stage %d Idle SREQ timeout, LT %d",
              dev,
              pipe,
              stage,
              data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               data,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_NONE,
               "Idletime SREQ timeout");
    to_disable |= (1u << 20);
  }

  /* Check for timeouts on station requests indicating a stats message was not
   * able to be sent from the MAU to SBC.  This is fatal because SW cannot miss
   * a lock ACK or barrier ACK. */
  if (intr_status_val & (1u << 21)) {
    uint32_t data = 0;
    uint32_t addr =
        offsetof(tof2_reg,
                 pipes[phy_pipe].mau[stage].cfg_regs.sreq_stats_timeout_errlog);
    pipe_mgr_interrupt_read_register(dev, addr, &data);
    LOG_ERROR("Dev %d log-pipe %d stage %d Stats SREQ timeout, ALU %d",
              dev,
              pipe,
              stage,
              data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               data,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_NONE,
               "Stats SREQ timeout");
    to_disable |= (1u << 21);
  }

  /* Check for Atomic Mod protocol errors.  Fatal due to the fact that CSR
   * updates in the AMOD FIFO were not processed so HW and SW are out of sync.
   */
  if (intr_status_val & (1u << 24)) {
    uint32_t data = 0;
    uint32_t addr = offsetof(
        tof2_reg, pipes[phy_pipe].mau[stage].cfg_regs.amod_protocol_errlog);
    pipe_mgr_interrupt_read_register(dev, addr, &data);
    LOG_ERROR("Dev %d log-pipe %d stage %d AMOD protocol error %d %s",
              dev,
              pipe,
              stage,
              data,
              amod_proto_log_to_string(data));
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               data,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_NONE,
               "AMOD protocol error");
    to_disable |= (1u << 24);
  }

  /* Disable any interrupts which generated fatal events. */
  pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, to_disable);

  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

/* Handle sram ecc error */
static uint32_t pipe_mgr_tof2_ecc_sram_handle(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, col = 0;
  uint32_t mem_offset = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  row = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Sram ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  rc = pipe_mgr_api_enter(shdl);
  if (rc != PIPE_SUCCESS) return rc;

  // Status register:
  // pipes[].mau[].rams.array.row[].intr_status_mau_unit_ram_row

  const int num_cols = dev_info->dev_cfg.stage_cfg.num_sram_cols;
  const pipe_mem_type_t mem_type = pipe_mem_type_unit_ram;

  /* Outer loop executes twice: once for single-bit errors (bits 0-11)
   * and once for multi-bit errors (bits 12-23). */
  for (int sbe_or_mbe = 0; sbe_or_mbe <= num_cols; sbe_or_mbe += num_cols) {
    const bool is_sbe = (sbe_or_mbe == 0);
    const bf_error_sev_level_t sev =
        is_sbe ? BF_ERR_SEV_CORRECTABLE : BF_ERR_SEV_NON_CORRECTABLE;
    const bf_error_type_t err_type =
        is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC;

    /* Check error status of each column. */
    for (col = 0; col < num_cols; col++) {
      const uint32_t cur_status_bit = 1u << (col + sbe_or_mbe);
      if (!(intr_status_val & cur_status_bit)) continue;

      /* There's a memory error in this column. Read the line on which
       * the error occurred from the SBE or MBE error log register. */
      if (is_sbe) {
        uint32_t address = offsetof(tof2_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_sbe_errlog);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof2_unit_ram_sbe_errlog_unit_ram_sbe_errlog(&data);
      } else {
        uint32_t address = offsetof(tof2_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_mbe_errlog);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof2_unit_ram_mbe_errlog_unit_ram_mbe_errlog(&data);
      }
      mem_offset &= 0x3FF;

      /* Convert the row and column to a unit id and from that build the full
       * address of the memory. */
      const mem_id_t mem_id =
          dev_info->dev_cfg.mem_id_from_col_row(stage, col, row, mem_type);
      const uint64_t phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          0, phy_pipe, stage, mem_id, mem_offset, mem_type);

      /* Look up the table that owns the memory. Just take the first owner; we
       * really only care about the owner to distinguish between s2p tables and
       * normal tables. */
      pipe_tbl_hdl_t tbl_hdl = 0;
      rmt_tbl_type_t tbl_type = 0;
      pipe_mgr_get_mem_id_to_tbl_hdl_mapping(
          dev, pipe, stage, mem_id, mem_type, &tbl_hdl, &tbl_type);

      /* Get pointer to table name. */
      const char *tbl_name = get_tbl_name(dev, tbl_hdl, __func__, __LINE__);

      LOG_TRACE("Dev %d pipe %d stage %d unitRAM %d line %d tbl 0x%x, %cBE",
                dev,
                pipe,
                stage,
                mem_id,
                mem_offset,
                tbl_hdl,
                is_sbe ? 'S' : 'M');

      if (tbl_hdl == 0) {
        /* Memory error on an unused RAM?!  Shouldn't ever be triggered by the
         * data plane, but perhaps someone using debug commands did it.  Just
         * write the memory to zero. */
        LOG_ERROR("Dev %d pipe %d stage %d unitRAM %d %cBE but unused",
                  dev,
                  pipe,
                  stage,
                  mem_id,
                  sbe_or_mbe ? 'M' : 'S');
        lld_ind_write(dev, phy_addr, 0, 0);
        BF_ERR_EVT(sev,
                   dev,
                   pipe,
                   stage,
                   phy_addr,
                   err_type,
                   BF_ERR_BLK_SRAM,
                   BF_ERR_LOC_NONE,
                   "Sram ecc error at row %d, col %d, line %d, physical addr "
                   "0x%" PRIx64,
                   row,
                   col,
                   mem_offset,
                   phy_addr);
        continue;
      } else if (!tbl_is_s2p(tbl_hdl)) {
        pipe_mgr_intr_sram_tcam_ecc_correct(shdl, dev, tbl_hdl, phy_addr);
        BF_ERR_EVT_OBJ(sev,
                       dev,
                       pipe,
                       stage,
                       phy_addr,
                       tbl_name,
                       err_type,
                       BF_ERR_BLK_SRAM,
                       BF_ERR_LOC_NONE,
                       "Sram ecc error at row %d, col %d, line %d, "
                       "physical addr 0x%" PRIx64,
                       row,
                       col,
                       mem_offset,
                       phy_addr);
      } else if (is_sbe) {
        rc = s2p_ram_sbe_correct(shdl, dev, tbl_hdl, pipe, stage, mem_offset);
        if (rc != PIPE_SUCCESS) {
          /* For whatever reason we didn't fix it, but HW should correct this
           * on its own eventually (or faster than software can if there is
           * traffic) in a S2P table so don't upgrade the severity.
           * Note that one possible reason why we didn't fix it is because the
           * table is currently reserved by a batch or transaction. */
        }
        BF_ERR_EVT_OBJ(sev,
                       dev,
                       pipe,
                       stage,
                       phy_addr,
                       tbl_name,
                       err_type,
                       BF_ERR_BLK_SRAM,
                       BF_ERR_LOC_NONE,
                       "Sram ecc error at row %d, col %d, line %d, "
                       "physical addr 0x%" PRIx64,
                       row,
                       col,
                       mem_offset,
                       phy_addr);
      } else {
        LOG_ERROR(
            "Dev %d pipe %d stage %d unitRAM %d line %d tbl 0x%x, S2P MBE",
            dev,
            pipe,
            stage,
            mem_id,
            mem_offset,
            tbl_hdl);
        BF_ERR_EVT_OBJ(
            BF_ERR_SEV_FATAL,
            dev,
            pipe,
            stage,
            phy_addr,
            tbl_name,
            err_type,
            BF_ERR_BLK_SRAM,
            BF_ERR_LOC_NONE,
            "Uncorrectable s2p sram ecc error at row %d, col %d, line %d, "
            "physical addr 0x%" PRIx64,
            row,
            col,
            mem_offset,
            phy_addr);
        /* We are not fixing the error so turn it off so it doesn't keep firing
         * as fast as we can clear it. */
        pipe_mgr_interrupt_set_enable_val(
            dev, 0, enable_hi_addr, cur_status_bit);
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return rc;
}

/* Handle map ram ecc error */
static uint32_t pipe_mgr_tof2_ecc_map_ram_handle(bf_dev_id_t dev,
                                                 bf_subdev_id_t subdev_id,
                                                 uint32_t intr_address,
                                                 uint32_t intr_status_val,
                                                 uint32_t enable_hi_addr,
                                                 uint32_t enable_lo_addr,
                                                 void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  pipe_tbl_hdl_t tbl_hdl = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, col = 0;
  uint32_t mem_offset = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  row = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Map Ram ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val "
      "0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.map_alu.row[].adrmux.intr_status_mau_adrmux_row

  const int num_cols = dev_info->dev_cfg.stage_cfg.num_map_ram_cols;
  const pipe_mem_type_t mem_type = pipe_mem_type_map_ram;

  /* Outer loop executes twice: once for single-bit errors (bits 0-5)
   * and once for multi-bit errors (bits 6-11). */
  for (int sbe_or_mbe = 0; sbe_or_mbe <= num_cols; sbe_or_mbe += num_cols) {
    const bool is_sbe = (sbe_or_mbe == 0);
    const bf_error_type_t err_type =
        is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC;

    /* Check error status of each column. */
    for (col = 0; col < num_cols; col++) {
      const uint32_t cur_status_bit = 1u << (col + sbe_or_mbe);
      if (!(intr_status_val & cur_status_bit)) continue;

      /* There's a memory error in this column. Read the line on which
       * the error occurred from the SBE or MBE error log register. */
      if (is_sbe) {
        // single-bit error
        uint32_t address = offsetof(tof2_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_sbe_errlog[col]);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof2_mapram_sbe_errlog_mapram_sbe_errlog(&data);
      } else {
        // multi-bit error
        uint32_t address = offsetof(tof2_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_mbe_errlog[col]);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof2_mapram_mbe_errlog_mapram_mbe_errlog(&data);
      }
      mem_offset &= 0x3FF;

      /* Convert the row and col to a map ram unit id. */
      const mem_id_t mem_id =
          dev_info->dev_cfg.mem_id_from_col_row(stage, col, row, mem_type);
      const mem_id_t unit_ram_mem_id =
          dev_info->dev_cfg.unit_ram_from_map_ram(mem_id);
      const uint64_t phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          0, phy_pipe, stage, mem_id, mem_offset, mem_type);

      const rmt_tbl_type_t owner_type =
          pipe_mgr_map_ram_type_get(dev, pipe, stage, mem_id);

      /* Handle idle time map rams specially. */
      if (owner_type == RMT_TBL_TYPE_IDLE_TMO) {
        const char *tbl_name = NULL;

        /* Refresh the idle entries in the map ram row.
         * Only need to fix multibit and parity errors */
        if (!is_sbe) {
          /* Get table handle and name. */
          tbl_hdl = PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id);
          tbl_name = get_tbl_name(dev, tbl_hdl, __func__, __LINE__);

          /* For parity and multi-bit errors, we must first do a physical write
           * of the entire line to reset the error, then use virtual writes to
           * reset the entries. */
          pipe_mgr_idle_phy_write_line(dev, tbl_hdl, stage, mem_id, mem_offset);
          pipe_mgr_intr_map_ram_idle_ecc_correct(
              get_pipe_mgr_ctx()->int_ses_hndl,
              dev,
              tbl_hdl,
              pipe,
              stage,
              mem_id,
              mem_offset);
        }
        BF_ERR_EVT_OBJ(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       stage,
                       phy_addr,
                       tbl_name,
                       err_type,
                       BF_ERR_BLK_MAP_RAM,
                       BF_ERR_LOC_NONE,
                       "mapRAM ecc error fixed at physical addr 0x%" PRIx64,
                       phy_addr);
      } else {
        /* Get table handle and name. */
        tbl_hdl = PIPE_INTR_TBL_HDL(dev, pipe, stage, unit_ram_mem_id, 0);
        const char *tbl_name = get_tbl_name(dev, tbl_hdl, __func__, __LINE__);

        if (tbl_hdl == 0 || !tbl_is_s2p(tbl_hdl)) {
          /* This is unexpected...  The map RAM isn't owned, just write it to
           * zero. */
          LOG_ERROR("Dev %d pipe %d stage %d mapRAM %d %cBE but unused",
                    dev,
                    pipe,
                    stage,
                    mem_id,
                    sbe_or_mbe ? 'M' : 'S');
          lld_ind_write(dev, phy_addr, 0, 0);
          BF_ERR_EVT_OBJ(
              sbe_or_mbe ? BF_ERR_SEV_NON_CORRECTABLE : BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              stage,
              phy_addr,
              tbl_name,
              err_type,
              BF_ERR_BLK_MAP_RAM,
              BF_ERR_LOC_NONE,
              "mapRAM ecc error fixed at physical addr 0x%" PRIx64,
              phy_addr);
          continue;
        } else {
          if (is_sbe) {
            LOG_TRACE(
                "Dev %d pipe %d stage %d mapRAM %d line %d tbl 0x%x, S2P SBE",
                dev,
                pipe,
                stage,
                mem_id,
                mem_offset,
                tbl_hdl);
            /* Software does not have a way to force the mapRAM to write back
             * and correct a SBE.  The mapRAM will update when the SRAM entry
             * moves across unit rams which requires back-to-back accesses to
             * the same unit ram by traffic. */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, cur_status_bit);
            BF_ERR_EVT_OBJ(BF_ERR_SEV_CORRECTABLE,
                           dev,
                           pipe,
                           stage,
                           phy_addr,
                           tbl_name,
                           err_type,
                           BF_ERR_BLK_MAP_RAM,
                           BF_ERR_LOC_NONE,
                           "mapRAM SBE at physical addr 0x%" PRIx64,
                           phy_addr);
          } else {
            LOG_ERROR(
                "Dev %d pipe %d stage %d mapRAM %d line %d tbl 0x%x, S2P MBE",
                dev,
                pipe,
                stage,
                mem_id,
                mem_offset,
                tbl_hdl);
            BF_ERR_EVT_OBJ(
                BF_ERR_SEV_FATAL,
                dev,
                pipe,
                stage,
                phy_addr,
                tbl_name,
                err_type,
                BF_ERR_BLK_MAP_RAM,
                BF_ERR_LOC_NONE,
                "Uncorrectable S2P ECC error, stage %d mapRAM %d line %d",
                stage,
                mem_id,
                mem_offset);
            /* We are not fixing the error so turn it off so it doesn't keep
             * firing as fast as we can clear it. */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, cur_status_bit);
          }
        }
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle meter stats ecc error */
static uint32_t pipe_mgr_tof2_ecc_meter_stats_handle(bf_dev_id_t dev,
                                                     bf_subdev_id_t subdev_id,
                                                     uint32_t intr_address,
                                                     uint32_t intr_status_val,
                                                     uint32_t enable_hi_addr,
                                                     uint32_t enable_lo_addr,
                                                     void *userdata) {
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0;
  uint32_t mem_offset = 0;
  pipe_mem_type_t mem_type = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe and stage from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Meter-Stats ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val "
      "0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.match.adrdist.intr_status_mau_ad

  /* Check bits 0-9, 18-25 */
  /* bits 0-3: def_stats_sbe */
  mem_type = pipe_mem_type_stats_deferred_access_ram;
  for (row = 0; row < PIPE_MGR_INTR_MAX_STATS; row++) {
    if (intr_status_val & (0x1u << row)) {
      /* Decode location */
      address =
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .rams.match.adrdist.deferred_stats_parity_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset =
          getp_tof2_deferred_stats_parity_errlog_def_sbe_errlog_addr(&data);
      /* Report event */
      BF_ERR_EVT(
          BF_ERR_SEV_CORRECTABLE,
          dev,
          pipe,
          stage,
          dev_info->dev_cfg.get_full_phy_addr(
              0,
              phy_pipe,
              stage,
              dev_info->dev_cfg.mem_id_from_col_row(stage, 0, row, mem_type),
              mem_offset,
              mem_type),
          BF_ERR_TYPE_SINGLE_BIT_ECC,
          BF_ERR_BLK_STATS,
          BF_ERR_LOC_NONE,
          "Stats ecc error at row %d, stage %d, line %d",
          row,
          stage,
          mem_offset);
      // Deferred stats RAM single-bit (parity) error.
      LOG_TRACE("Stats ecc error at pipe %d, (row %d, stage %d, line %d) ",
                pipe,
                row,
                stage,
                mem_offset);
    }
  }

  /* bits 4-7: def_meter_sbe */
  mem_type = pipe_mem_type_meter_deferred_access_ram;
  for (row = 0; row < PIPE_MGR_INTR_MAX_METERS; row++) {
    /* Decode location */
    if (intr_status_val & (0x1u << (row + 4))) {
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.match.adrdist.def_meter_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset = getp_tof2_def_meter_sbe_errlog_def_sbe_errlog_addr(&data);
      /* Report event */
      BF_ERR_EVT(
          BF_ERR_SEV_CORRECTABLE,
          dev,
          pipe,
          stage,
          dev_info->dev_cfg.get_full_phy_addr(
              0,
              phy_pipe,
              stage,
              dev_info->dev_cfg.mem_id_from_col_row(stage, 0, row, mem_type),
              mem_offset,
              mem_type),
          BF_ERR_TYPE_SINGLE_BIT_ECC,
          BF_ERR_BLK_METERS,
          BF_ERR_LOC_NONE,
          "Meters ecc error at row %d, stage %d, line %d",
          row,
          stage,
          mem_offset);
      // Deferred meter RAM single-bit (parity) error.
      LOG_TRACE("Meters ecc error at pipe %d, (row %d, stage %d, line %d) ",
                pipe,
                row,
                stage,
                mem_offset);
    }
  }

  /* bit 8: idletime_slip */
  if (intr_status_val & (0x1u << 8)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_IDLETIME,
               "MAU idletime slip error");
    // Idletime sweep lagging beyond programmed value.
    LOG_TRACE("MAU idletime slip error");
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 8);
  }

  /* bit 9: meter_sweep */
  if (intr_status_val & (0x1u << 9)) {
    /* Decode location */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].rams.match.adrdist.meter_sweep_errlog);
    pipe_mgr_interrupt_read_register(dev, address, &data);
    /* 4 bit mem_offset */
    mem_offset = getp_tof2_meter_sweep_errlog_meter_sweep_errlog(&data);
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               mem_offset,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_METERS,
               BF_ERR_LOC_NONE,
               "Meter sweep error at offset %d",
               mem_offset);
    // Meter sweep restarted before previous sweep completed.
    LOG_TRACE("Meter sweep error in pipe %d, stage %d at addr %d ",
              pipe,
              stage,
              mem_offset);
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 9);
  }

  /* bits 18-21: stateful_log_overflow */
  if (intr_status_val & 0x3c0000) {
    /* Reporting event is disabled
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_STATEFUL_LOG,
               "MAU stateful log fifo overflow error");
    */
    LOG_TRACE("MAU stateful log fifo overflow error, 0x%x",
              (intr_status_val >> 18) & 0xf);
    /* Disable interrupt(s) */
    pipe_mgr_interrupt_set_enable_val(
        dev, 0, enable_hi_addr, (intr_status_val & 0x3c0000));
  }

  /* bits 22-25: stateful_log_underflow */
  if (intr_status_val & 0x3c00000) {
    /* Reporting event is disabled
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_UNDERFLOW,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_STATEFUL_LOG,
               "MAU stateful log fifo underflow error");
    */
    LOG_TRACE("MAU stateful log fifo underflow error, 0x%x",
              (intr_status_val >> 22) & 0xf);
    /* Disable interrupt(s) */
    pipe_mgr_interrupt_set_enable_val(
        dev, 0, enable_hi_addr, (intr_status_val & 0x3c00000));
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in synth2port */
static uint32_t pipe_mgr_tof2_synth2port_mem_err_handle(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev_id,
    uint32_t intr_address,
    uint32_t intr_status_val,
    uint32_t enable_hi_addr,
    uint32_t enable_lo_addr,
    void *userdata) {
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0;
  uint32_t mem_offset = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  row = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Synth2port intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.map_alu.row[].i2portctl.intr_status_mau_synth2port

  if (intr_status_val & INTR_SYNTH2PORT_0) {
    /* Decode location */
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.map_alu.row[row]
                           .i2portctl.mau_synth2port_errlog);
    pipe_mgr_interrupt_read_register(dev, address, &data);
    /* 16 bit mem_offset */
    mem_offset = getp_tof2_mau_synth2port_errlog_mau_synth2port_errlog(&data);
    /* Synth2port errors are benign. Log but don't report event. */
    LOG_TRACE("Synth2port error in pipe %d, stage %d at (row %d, off %d)",
              pipe,
              stage,
              row,
              mem_offset);
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(
        dev, 0, enable_hi_addr, INTR_SYNTH2PORT_0);
  }

  /* Bit 1 can be enabled and injected, but it doesn't do anything. */
  if (intr_status_val & INTR_SYNTH2PORT_1) {
    /* Quietly disable the interrupt. */
    pipe_mgr_interrupt_set_enable_val(
        dev, 0, enable_hi_addr, INTR_SYNTH2PORT_1);
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in mau sel alu */
static uint32_t pipe_mgr_tof2_sel_alu_mem_err_handle(bf_dev_id_t dev,
                                                     bf_subdev_id_t subdev_id,
                                                     uint32_t intr_address,
                                                     uint32_t intr_status_val,
                                                     uint32_t enable_hi_addr,
                                                     uint32_t enable_lo_addr,
                                                     void *userdata) {
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, col = 0;
  uint32_t mem_offset = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  row = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Sel alu intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.map_alu.meter_group[].selector
  //    .intr_status_mau_selector_alu

  /* bits 0-1: selector_alu */
  for (col = 0; col < PIPE_MGR_INTR_MAX_SEL_ALU_DIR; col++) {
    /* col 0 - ingress, col 1 - egress */
    if (intr_status_val & (0x1u << col)) {
      /* Decode location */
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.map_alu.meter_group[row]
                             .selector.mau_selector_alu_errlog);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 16 bit mem_offset */
      mem_offset =
          getp_tof2_mau_selector_alu_errlog_mau_selector_alu_errlog(&data);
      /* Nothing to be done here. The issue should clear up once software
       * updates. HW will correct set of ports. */
      /* Report event */
      BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 mem_offset,
                 BF_ERR_TYPE_GENERIC,
                 BF_ERR_BLK_SELECTOR_ALU,
                 BF_ERR_LOC_NONE,
                 "Selector ALU error at (row %d, dir %s, offset %d)",
                 row,
                 col ? "Egress" : "Ingress",
                 mem_offset);
      // No live ports in the pool.
      LOG_TRACE(
          "Sel alu error at pipe %d, stage %d, (row %d, direction %s,addr "
          "0x%x)",
          pipe,
          stage,
          row,
          col ? "Egress" : "Ingress",
          mem_offset);
      /* Disable interrupt */
      pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << col);
    }
  }

  /* bit 2: stateful_minmax */
  if (intr_status_val & (0x1u << 2)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_ST_MINMAX,
               "Selector ALU stateful min/max error");
    // No live queues in the qLAG as they've all been masked off.
    LOG_TRACE("Selector ALU stateful min/max error");
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 2);
  }

  /* bit 3: stateful_div_by_zero */
  if (intr_status_val & (0x1u << 3)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_DIV_BY0,
               "Selector ALU stateful divide by 0 error");
    LOG_TRACE("Selector ALU stateful divide by 0 error");
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 3);
  }

  // FIXME: Is this really an error?
  /* bit 4: stateful predication interrupt (salu_pred_intr) */
  if (intr_status_val & (0x1u << 4)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_SALU_PRED,
               "Selector ALU stateful predication error");
    LOG_TRACE("Selector ALU stateful predication error");
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 4);
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle stats error */
static uint32_t pipe_mgr_tof2_stats_err_handle(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  bf_dev_pipe_t log_pipe = 0;
  dev_stage_t stage = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  int alu_idx = 0;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  log_pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  alu_idx = userdata_p->row;

  LOG_TRACE(
      "Stats ALU (dev %d, pipe %d, stage %d, alu_idx %d): Addr 0x%x, "
      "Status-Val 0x%x",
      dev,
      log_pipe,
      stage,
      alu_idx,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[pipe].mau[stage].rams.map_alu.stats_wrap[alu_idx].stats
  //    .intr_status_mau_stats_alu

  /* bit 0: LRT evict FIFO overflow */
  if (intr_status_val & (0x1u)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               log_pipe,
               stage,
               alu_idx,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_STATS,
               BF_ERR_LOC_NONE,
               "Stats alu evict fifo overflow error in alu %d",
               alu_idx);
    LOG_TRACE("Stats alu evict fifo overflow error in pipe %d stage %d alu %d ",
              log_pipe,
              stage,
              alu_idx);
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u);
  }

  /* bit 1: stats entry overflow */
  if (intr_status_val & (0x1u << 1)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
               dev,
               log_pipe,
               stage,
               alu_idx,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_STATS,
               BF_ERR_LOC_NONE,
               "Stats alu stats entry overflow error in alu %d",
               alu_idx);
    LOG_TRACE(
        "Stats alu stats entry overflow error in pipe %d stage %d alu %d ",
        log_pipe,
        stage,
        alu_idx);
    /* No further action */
  }

  /* bit 2: max_value clear pending */
  if (intr_status_val & (0x1u << 2)) {
    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
               dev,
               log_pipe,
               stage,
               alu_idx,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_STATS,
               BF_ERR_LOC_NONE,
               "Stats alu clear pending error in alu %d",
               alu_idx);
    LOG_TRACE("Stats alu clear pending error in pipe %d stage %d alu %d ",
              log_pipe,
              stage,
              alu_idx);
    /* No further action */
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Helper to rewrite imem from shadow. */
pipe_status_t pipe_mgr_tof2_imem_rewrite(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info,
                                         bf_dev_pipe_t log_pipe,
                                         bf_dev_pipe_t phy_pipe,
                                         int stage) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  int bwr_size = pipe_mgr_drv_buf_size(dev_id, PIPE_MGR_DRV_BUF_BWR);

  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Failed to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }

  int entry_width = 4;
  int log_pipe_mask = 1 << log_pipe;
  uint32_t pcie_addr;
  uint8_t *data_ptr;
  int data_len;
  pipe_mgr_drv_buf_t *b = NULL;

  /* For each imem section allocate a DMA buffer, fill it with the imem data,
   * then issue a block write to correct the imem contents. */
  for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
    b = pipe_mgr_drv_buf_alloc(
        st->sid, dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      LOG_ERROR(
          "Failed to alloc DMA memory for imem correction, dev %d pipe %d "
          "stage %d",
          dev_id,
          log_pipe,
          stage);
      sts = PIPE_NO_SYS_RESOURCES;
      goto done;
    }

    pcie_addr =
        PIPE_INTR_IMEM_DATA(dev_id, phy_pipe, stage).tof2.imem[i].base_addr;
    data_ptr = PIPE_INTR_IMEM_DATA(dev_id, phy_pipe, stage).tof2.imem[i].data;
    data_len =
        PIPE_INTR_IMEM_DATA(dev_id, phy_pipe, stage).tof2.imem[i].data_len;
    if (data_ptr) {
      PIPE_MGR_MEMCPY(b->addr, data_ptr, data_len);
      int num_entries = data_len / 4;
      sts = pipe_mgr_drv_blk_wr(
          &shdl,
          entry_width,
          num_entries,
          4,
          dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(pcie_addr),
          log_pipe_mask,
          b);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to send write-block to correct imem error, dev %d log-pipe "
            "%d stage %d, sts %s",
            dev_id,
            log_pipe,
            stage,
            pipe_str_err(sts));
        goto done;
      }
    } else {
      LOG_ERROR("Failed to correct imem, no data: dev %d log-pipe %d stage %d",
                dev_id,
                log_pipe,
                stage);
      pipe_mgr_drv_buf_free(b);
      sts = PIPE_UNEXPECTED;
      goto done;
    }
  }
done:
  return sts;
}

/* Handle imem error */
static uint32_t pipe_mgr_tof2_imem_err_handle(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  dev_stage_t stage = 0;
  int col = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t mem_offset = 0;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get pipe and stage from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  log_pipe = userdata_p->pipe;
  stage = userdata_p->stage;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  LOG_TRACE("Imem intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            log_pipe,
            stage,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].dp.intr_status_mau_imem

  /* Decode location */
  address = offsetof(tof2_reg, pipes[phy_pipe].mau[stage].dp.imem_sbe_errlog);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  /* 14 bit mem_offset */
  mem_offset = getp_tof2_imem_sbe_errlog_imem_sbe_errlog(&data);
  /* intr_mem_cnt */
  mem_offset &= 0x3FFF;

  for (col = 0; col < PIPE_MGR_INTR_MAX_IMEM_DIR; col++) {
    /* col 0 - ingress, col 1 - egress */
    if (intr_status_val & (0x1u << col)) {
      /* Report event */
      BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                 dev,
                 log_pipe,
                 stage,
                 mem_offset,
                 BF_ERR_TYPE_PARITY,
                 BF_ERR_BLK_IMEM,
                 BF_ERR_LOC_NONE,
                 "Imem error in stage %s direction ",
                 col ? "Egress" : "Ingress");
      LOG_TRACE("imem error in pipe %d stage %d and %s direction, value 0x%x ",
                log_pipe,
                stage,
                col ? "Egress" : "Ingress",
                data);
    }
  }

  pipe_mgr_tof2_imem_rewrite(shdl, dev_info, log_pipe, phy_pipe, stage);

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle tcam ecc error */
static uint32_t pipe_mgr_tof2_ecc_tcam_handle(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, col = 0, subword;
  uint32_t mem_offset = 0;
  uint64_t phy_addr = 0;
  const pipe_mem_type_t mem_type = pipe_mem_type_tcam;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe and stage from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Tcam ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].tcams.intr_status_mau_tcam_array

  /* bits 0-3 tcam logical channel error (ignored) */

  /* bits 4-15 for single bit error */
  for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_tcam_rows; row++) {
    if (intr_status_val & (0x1u << (4 + row))) {
      /* Decode location */
      mem_id_t mem_id = 0;
      int index = 0;
      address = offsetof(tof2_reg,
                         pipes[phy_pipe].mau[stage].tcams.tcam_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 9 bit line no, 1 bit column and 1 bit subword */
      data = getp_tof2_tcam_sbe_errlog_tcam_sbe_errlog_addr(&data);
      subword = data & 0x1;
      mem_offset = (data >> 1) & 0x1ff;
      col = (data >> 10) & 0x1;
      mem_id = dev_info->dev_cfg.mem_id_from_col_row(stage, col, row, mem_type);
      phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          0, phy_pipe, stage, mem_id, mem_offset, mem_type);

      /* Look up the table that owns the mem_id. */
      pipe_tbl_hdl_t tbl_hdl = 0;
      rmt_tbl_type_t tbl_type = 0;
      pipe_mgr_get_mem_id_to_tbl_hdl_mapping(
          dev, pipe, stage, mem_id, mem_type, &tbl_hdl, &tbl_type);

      /* Get pointer to table name. */
      const char *tbl_name = get_tbl_name(dev, tbl_hdl, __func__, __LINE__);

      /* Report event */
      BF_ERR_EVT_OBJ(
          BF_ERR_SEV_NON_CORRECTABLE,
          dev,
          pipe,
          stage,
          phy_addr,
          tbl_name,
          BF_ERR_TYPE_PARITY,
          BF_ERR_BLK_TCAM,
          BF_ERR_LOC_NONE,
          "Tcam Ecc error at physical row %d, col %d, line %d, subword %d "
          "address 0x%" PRIx64 "",
          row,
          col,
          mem_offset,
          subword,
          phy_addr);
      // TCAM column pair single-bit (parity) error.
      LOG_TRACE(
          "Tcam ecc error at pipe %d, stage %d (row %d, col %d, line 0x%x, "
          "subword %d), phy address 0x%" PRIx64 "",
          pipe,
          stage,
          row,
          col,
          mem_offset,
          subword,
          phy_addr);

      /* Repair memory */
      bool repaired = false;
      for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
        tbl_hdl = PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index);

        if ((tbl_hdl != 0) ||
            (PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) ==
             mem_type)) {
          /* Rewrite the data at the problem address */
          pipe_mgr_intr_sram_tcam_ecc_correct(
              get_pipe_mgr_ctx()->int_ses_hndl, dev, tbl_hdl, phy_addr);
          repaired = true;
        }
      }

      /* If address is not used by any table, fix the memory directly */
      if (!repaired) {
        pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr);
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register MAU interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_mau_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, ret = 0, alu_idx = 0;

  LOG_TRACE("Pipe-mgr Registering for mau ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* MAU Config. */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe].mau[stage].cfg_regs.intr_status_mau_cfg),
          &pipe_mgr_tof2_intr_mau_cfg_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* MAU Adrdist. */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(
              tof2_reg,
              pipes[phy_pipe].mau[stage].rams.match.adrdist.intr_status_mau_ad),
          &pipe_mgr_tof2_ecc_meter_stats_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* Stats ALU. */
      for (alu_idx = 0; alu_idx < 4; alu_idx++) {
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.stats_wrap[alu_idx]
                         .stats.intr_status_mau_stats_alu),
            &pipe_mgr_tof2_stats_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, alu_idx)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* synth2port error */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        /* Synth2port interrupts are disabled by default.
         * We register the interrupt callback for debug purposes. */
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.row[row]
                         .i2portctl.intr_status_mau_synth2port),
            &pipe_mgr_tof2_synth2port_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* MAP Ram ECC/Parity. */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.row[row]
                         .adrmux.intr_status_mau_adrmux_row),
            &pipe_mgr_tof2_ecc_map_ram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* Unit RAM ECC. */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.array.row[row]
                         .intr_status_mau_unit_ram_row),
            &pipe_mgr_tof2_ecc_sram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* Selector ALU. */
      for (row = 0; row < PIPE_MGR_INTR_MAX_SEL_ALU_ROWS; row++) {
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.meter_group[row]
                         .selector.intr_status_mau_selector_alu),
            &pipe_mgr_tof2_sel_alu_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* IMEM Parity. */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe].mau[stage].dp.intr_status_mau_imem),
          &pipe_mgr_tof2_imem_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* GFM Parity. */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe].mau[stage].dp.intr_status_mau_gfm_hash),
          &pipe_mgr_gfm_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* TCAM Parity. */
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe].mau[stage].tcams.intr_status_mau_tcam_array),
          &pipe_mgr_tof2_ecc_tcam_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }
  return PIPE_SUCCESS;
}

/* Enable/disable GFM interrupts */
pipe_status_t pipe_mgr_tof2_gfm_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_status_t ret;

  LOG_TRACE("Setting GFM Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* GFM Parity is enabled on a per profile basis. */
  for (unsigned i = 0; i < dev_info->num_pipeline_profiles; ++i) {
    if (!dev_info->profile_info[i]->driver_options.hash_parity_enabled)
      continue;

    for (int s = 0; s < dev_info->num_active_mau; ++s) {
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &dev_info->profile_info[i]->pipe_bmp,
          shdl,
          offsetof(tof2_reg, pipes[0].mau[s].dp.intr_enable0_mau_gfm_hash),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }
  }

  return PIPE_SUCCESS;
}

/* Enable/disable MAU interrupts */
pipe_status_t pipe_mgr_tof2_mau_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting MAU Interrupt mode to %s ", enable ? "Enable" : "Disable");

  for (int s = 0; s < dev_info->num_active_mau; ++s) {
    /* MAU Config. */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg, pipes[0].mau[s].cfg_regs.intr_enable0_mau_cfg),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* MAU Adrdist. */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0].mau[s].rams.match.adrdist.intr_enable0_mau_ad),
        en_val & 0xFF);
    if (ret != PIPE_SUCCESS) return ret;

    /* Stats ALU and Selector ALU. */
    for (int a = 0; a < 4; ++a) {
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof2_reg,
                   pipes[0]
                       .mau[s]
                       .rams.map_alu.stats_wrap[a]
                       .stats.intr_enable0_mau_stats_alu),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;

      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof2_reg,
                   pipes[0]
                       .mau[s]
                       .rams.map_alu.meter_group[a]
                       .selector.intr_enable0_mau_selector_alu),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }

    for (int r = 0; r < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; ++r) {
      /* Synth2port not enabled. */

      /* MAP Ram ECC/Parity. */
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof2_reg,
                   pipes[0]
                       .mau[s]
                       .rams.map_alu.row[r]
                       .adrmux.intr_enable0_mau_adrmux_row),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;

      /* Unit RAM ECC. */
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof2_reg,
              pipes[0].mau[s].rams.array.row[r].intr_enable0_mau_unit_ram_row),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }

    /* IMEM Parity. */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg, pipes[0].mau[s].dp.intr_enable0_mau_imem),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* TCAM Parity. */
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg, pipes[0].mau[s].tcams.intr_enable0_mau_tcam_array),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}
