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
 * @file pipe_mgr_tof_interrupt.c
 * @date
 *
 * Tofino1 interrupt handling
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_tof_interrupt.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_db.h"
#include <tofino_regs/tofino.h>
#include <tofino_regs/pipe_top_level.h>
#include "pipe_mgr_idle.h"
#include "pipe_mgr_learn.h"
#include "pipe_mgr_phy_mem_map.h"
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include <mc_mgr/mc_mgr_pipe_intf.h>
#include "pipe_mgr_pktgen.h"

#define PIPE_MGR_TOF_INTR_PRE_MBE_START 14
#define PIPE_MGR_TOF_INTR_PRE_SBE_START 23
#define PIPE_MGR_TOF_INTR_PGR_NUM_ECC_ERR 10
#define PIPE_MGR_TOF_INTR_DEPRSR_NUM_ECC_ERR 6
#define PIPE_MGR_TOF_INTR_PRSR_NUM_ECC_ERR 18
#define PIPE_MGR_TOF_INTR_MIRR_NUM_ERR 17
#define PIPE_MGR_TOF_INTR_EBUF_BYPASS_NUM_ERR 6

#define INTR_SYNTH2PORT (0x1u << 0)

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/* Returns pointer to table name, or NULL if handle is zero. */
static inline const char *get_tbl_name(bf_dev_id_t dev,
                                       pipe_tbl_hdl_t tbl_hdl,
                                       const char *where,
                                       const int line) {
  return (tbl_hdl) ? pipe_mgr_get_tbl_name(dev, tbl_hdl, where, line) : NULL;
}

/* Handle error in pgr */
static uint32_t pipe_mgr_tof_pgr_err_handle(bf_dev_id_t dev,
                                            bf_subdev_id_t subdev_id,
                                            uint32_t intr_address,
                                            uint32_t intr_status_val,
                                            uint32_t enable_hi_addr,
                                            uint32_t enable_lo_addr,
                                            void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0, mem_addr = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;

  (void)subdev_id;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);

  LOG_TRACE("pgr intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF_INTR_PGR_NUM_ECC_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      bf_dev_target_t dev_tgt = {dev, pipe};
      switch (bitpos) {
        case 8:  // buffer_mbe
          address = offsetof(Tofino,
                             pipes[phy_pipe].pmarb.pgr_reg.pgr_common.mbe_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_addr = data & 0x3ff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer multi bit error at"
                     " addr %d",
                     mem_addr);
          LOG_TRACE("PGR multi bit err at addr %d ", mem_addr);
          /* Write the entire pkt buffer memory, mem-addr is not used */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;
        case 9:  // buffer_sbe
          address = offsetof(Tofino,
                             pipes[phy_pipe].pmarb.pgr_reg.pgr_common.sbe_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_addr = data & 0x3ff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer single bit error at"
                     " addr %d",
                     mem_addr);
          LOG_TRACE("PGR single bit err at addr %d ", mem_addr);
          /* Write the entire pkt buffer memory, mem-addr is not used */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
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

/* Handle sram ecc error */
static uint32_t pipe_mgr_tof_ecc_sram_handle(bf_dev_id_t dev,
                                             bf_subdev_id_t subdev_id,
                                             uint32_t intr_address,
                                             uint32_t intr_status_val,
                                             uint32_t enable_hi_addr,
                                             uint32_t enable_lo_addr,
                                             void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);

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
  (void)subdev_id;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  /* Determine row */
  row = userdata_p->row;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Sram ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
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
        uint32_t address = offsetof(Tofino,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_sbe_errlog);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_unit_ram_sbe_errlog_unit_ram_sbe_errlog(&data);
      } else {
        uint32_t address = offsetof(Tofino,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_mbe_errlog);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_unit_ram_mbe_errlog_unit_ram_mbe_errlog(&data);
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
static uint32_t pipe_mgr_tof_ecc_map_ram_handle(bf_dev_id_t dev,
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

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  /* Determine row */
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

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
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
        uint32_t address = offsetof(Tofino,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_sbe_errlog[col]);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_mapram_sbe_errlog_mapram_sbe_errlog(&data);
      } else {
        // multi-bit error
        uint32_t address = offsetof(Tofino,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_mbe_errlog[col]);
        uint32_t data = 0;
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_mapram_mbe_errlog_mapram_mbe_errlog(&data);
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
         * Only need to fix multibit and parity errors. */
        if (!is_sbe) {
          /* Get table handle and name. */
          tbl_hdl = PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id);
          tbl_name = get_tbl_name(dev, tbl_hdl, __func__, __LINE__);

          /* For parity and multi-bit errors, we must first do a physical write
           * of the entire line to reset the error, then use virtual writes to
           * reset the entries. */
          pipe_mgr_idle_phy_write_line(dev, tbl_hdl, stage, mem_id, mem_offset);
          pipe_mgr_intr_map_ram_idle_ecc_correct(pipe_mgr_ctx->int_ses_hndl,
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
             * move across unit rams which requires back-to-back accesses to
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
static uint32_t pipe_mgr_tof_ecc_meter_stats_handle(bf_dev_id_t dev,
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
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);

  if (dev >= PIPE_MGR_NUM_DEVICES || !dev_info) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
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

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.match.adrdist.intr_status_mau_ad

  mem_type = pipe_mem_type_stats_deferred_access_ram;

  /* Check for stats parity error: bits 0 -3 */
  for (row = 0; row < PIPE_MGR_INTR_MAX_STATS; row++) {
    if (intr_status_val & (0x1u << row)) {
      address =
          offsetof(Tofino,
                   pipes[phy_pipe]
                       .mau[stage]
                       .rams.match.adrdist.deferred_stats_parity_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset = getp_deferred_stats_parity_errlog_def_sbe_errlog_addr(&data);
      /* meters Synth2port errors cannot be fixed, just log */
      BF_ERR_EVT(
          BF_ERR_SEV_FATAL,
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
          BF_ERR_TYPE_PARITY,
          BF_ERR_BLK_STATS,
          BF_ERR_LOC_NONE,
          "Stats ecc error at row %d, stage %d, line %d",
          row,
          stage,
          mem_offset);
      LOG_TRACE("Stats ecc error at pipe %d, (row %d, stage %d, line %d) ",
                pipe,
                row,
                stage,
                mem_offset);
      /* turn off this interrupt as we do not want to see it it again*/
      pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << row);
    }
  }

  mem_type = pipe_mem_type_meter_deferred_access_ram;

  /* Check for meters sbe: bits 4 - 7 */
  for (row = 0; row < PIPE_MGR_INTR_MAX_METERS; row++) {
    if (intr_status_val & (0x1u << (row + 4))) {
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.match.adrdist.def_meter_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset = getp_def_meter_sbe_errlog_def_sbe_errlog_addr(&data);
      /* stats Synth2port errors cannot be fixed, just log */
      BF_ERR_EVT(
          BF_ERR_SEV_FATAL,
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
      LOG_TRACE("Meters ecc error at pipe %d, (row %d, stage %d, line %d) ",
                pipe,
                row,
                stage,
                mem_offset);
      pipe_mgr_interrupt_set_enable_val(
          dev, 0, enable_hi_addr, 0x1u << (row + 4));
    }
  }

  /* meter sweep */
  if (intr_status_val & (0x1u << 9)) {
    address = offsetof(
        Tofino,
        pipes[phy_pipe].mau[stage].rams.match.adrdist.meter_sweep_errlog);
    pipe_mgr_interrupt_read_register(dev, address, &data);
    /* 4 bit mem_offset */
    mem_offset = getp_meter_sweep_errlog_meter_sweep_errlog(&data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               mem_offset,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_METERS,
               BF_ERR_LOC_NONE,
               "Meters sweep error at offset %d",
               mem_offset);
    LOG_TRACE("Meter sweep error in pipe %d, stage %d at addr %d ",
              pipe,
              stage,
              mem_offset);
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << 9);
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in synth2port */
static uint32_t pipe_mgr_tof_synth2port_mem_err_handle(bf_dev_id_t dev,
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
  rmt_dev_info_t *dev_info;

  if (dev >= PIPE_MGR_NUM_DEVICES || !(dev_info = pipe_mgr_get_dev_info(dev))) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  /* Determine row */
  row = userdata_p->row;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Synth2port intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.map_alu.row[].i2portctl.intr_status_mau_synth2port

  if (intr_status_val & INTR_SYNTH2PORT) {
    /* Decode location */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.map_alu.row[row]
                           .i2portctl.mau_synth2port_errlog);
    pipe_mgr_interrupt_read_register(dev, address, &data);
    /* 16 bit mem_offset */
    mem_offset = getp_mau_synth2port_errlog_mau_synth2port_errlog(&data);
    /* Synth2port errors are benign. Log event but don't report it. */
    LOG_TRACE("Synth2port error in pipe %d, stage %d at (row %d, off %d)",
              pipe,
              stage,
              row,
              mem_offset);
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, INTR_SYNTH2PORT);
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in mau sel alu */
static uint32_t pipe_mgr_tof_sel_alu_mem_err_handle(bf_dev_id_t dev,
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

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  /* Determine row */
  row = userdata_p->row;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);

  LOG_TRACE(
      "Sel alu intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].rams.map_alu.meter_group[].selector
  //    .intr_status_mau_selector_alu

  for (col = 0; col < PIPE_MGR_INTR_MAX_SEL_ALU_DIR; col++) {
    /* col 0 - ingress, col 1 - egress */
    if (intr_status_val & (0x1u << col)) {
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.map_alu.meter_group[row]
                             .selector.mau_selector_alu_errlog);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 16 bit mem_offset */
      mem_offset = getp_mau_selector_alu_errlog_mau_selector_alu_errlog(&data);
      /*
       * Nothing to be done here. The issue should clear up once software
       * updates. HW will correct set of ports.
       */
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
      LOG_TRACE(
          "Sel alu error at pipe %d, stage %d, (row %d, direction %s,addr "
          "0x%x)",
          pipe,
          stage,
          row,
          col ? "Egress" : "Ingress",
          mem_offset);
      pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u << col);
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle imem error */
static uint32_t pipe_mgr_tof_imem_err_handle(bf_dev_id_t dev,
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
  int col = 0, intr_imem_cnt;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t mem_offset = 0;

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

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  log_pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  LOG_TRACE("Imem intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            log_pipe,
            stage,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].dp.intr_status_mau_imem

  address = offsetof(Tofino, pipes[phy_pipe].mau[stage].dp.imem_sbe_errlog);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  /* 14 bit mem_offset */
  mem_offset = getp_imem_sbe_errlog_imem_sbe_errlog(&data);
  /* intr_mem_cnt */
  intr_imem_cnt = PIPE_MGR_TOF_IMEM_COUNT;
  mem_offset &= 0x3FFF;
#ifdef PIPE_MGR_INTR_IMEM_TEST
  mem_offset = 0x3fff;
#endif

  for (col = 0; col < PIPE_MGR_INTR_MAX_IMEM_DIR; col++) {
    /* col 0 - ingress, col 1 - egress */
    if (intr_status_val & (0x1u << col)) {
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

  int bwr_size = pipe_mgr_drv_buf_size(dev, PIPE_MGR_DRV_BUF_BWR);
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
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
  for (int i = 0; i < intr_imem_cnt; ++i) {
    b = pipe_mgr_drv_buf_alloc(
        st->sid, dev, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      pipe_mgr_api_exit(shdl);
      LOG_ERROR("Failed to correct imem error, dev %d pipe %d stage %d",
                dev,
                log_pipe,
                stage);
      return PIPE_NO_SYS_RESOURCES;
    }
    pcie_addr = PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].base_addr;
    data_ptr = PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data;
    data_len = PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data_len;
    if (data_ptr) {
      PIPE_MGR_MEMCPY(b->addr, data_ptr, data_len);
      int num_entries = data_len / 4;
      status = pipe_mgr_drv_blk_wr(
          &shdl,
          entry_width,
          num_entries,
          4,
          dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(pcie_addr),
          log_pipe_mask,
          b);
      if (status != PIPE_SUCCESS) {
        pipe_mgr_api_exit(shdl);
        LOG_ERROR(
            "Failed to correct imem error, dev %d log-pipe %d stage %d, sts %s",
            dev,
            log_pipe,
            stage,
            pipe_str_err(status));
        return PIPE_NO_SYS_RESOURCES;
      }
    } else {
      LOG_ERROR("Failed to correct imem, no data: dev %d log-pipe %d stage %d",
                dev,
                log_pipe,
                stage);
      pipe_mgr_drv_buf_free(b);
      pipe_mgr_api_exit(shdl);
      PIPE_MGR_DBGCHK(data_ptr);
      return PIPE_UNEXPECTED;
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle tcam ecc error */
static uint32_t pipe_mgr_tof_ecc_tcam_handle(bf_dev_id_t dev,
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
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe and stage from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);

  LOG_TRACE(
      "Tcam ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].tcams.intr_status_mau_tcam_array

  /* bits 4-15 for single bit error */
  for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_tcam_rows; row++) {
    if (intr_status_val & (0x1u << (4 + row))) {
      /* Decode location */
      mem_id_t mem_id = 0;
      int index = 0;
      address = offsetof(Tofino,
                         pipes[phy_pipe].mau[stage].tcams.tcam_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 9 bit line no, 1 bit column and 1 bit subword */
      data = getp_tcam_sbe_errlog_tcam_sbe_errlog_addr(&data);
      subword = data & 0x1;
      mem_offset = (data >> 1) & 0x1ff;
      col = (data >> 10) & 0x1;
      mem_id = dev_info->dev_cfg.mem_id_from_col_row(stage, col, row, mem_type);
#ifdef PIPE_MGR_INTR_MAU_TCAM_TEST
      for (col = 0; col <= 1; col++) {
        for (mem_offset = 0; mem_offset < 0x1f; mem_offset++) {
#endif
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
              "Tcam Ecc error at physical row %d, col %d, line %d, subword %d"
              "address 0x%" PRIx64 "",
              row,
              col,
              mem_offset,
              subword,
              phy_addr);
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

            if ((tbl_hdl != 0) &&
                (PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) ==
                 mem_type)) {
              /* Rewrite the data at the problem address */
              pipe_mgr_intr_sram_tcam_ecc_correct(
                  pipe_mgr_ctx->int_ses_hndl, dev, tbl_hdl, phy_addr);
              repaired = true;
            }
          }

          /* If address is not used by any table, fix the memory directly */
          if (!repaired) {
            pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr);
          }
#ifdef PIPE_MGR_INTR_MAU_TCAM_TEST
        }
      }
#endif
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register for mau interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_mau_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, ret = 0;

  LOG_TRACE("Pipe-mgr Registering for mau ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* Tcam ecc */
      address = offsetof(
          Tofino, pipes[phy_pipe].mau[stage].tcams.intr_status_mau_tcam_array);
      ret = lld_int_register_cb(
          dev,
          0,
          address,
          &pipe_mgr_tof_ecc_tcam_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
      /* Sram ecc */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.array.row[row]
                               .intr_status_mau_unit_ram_row);
        ret = lld_int_register_cb(
            dev,
            0,
            address,
            &pipe_mgr_tof_ecc_sram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* map ram ecc */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.map_alu.row[row]
                               .adrmux.intr_status_mau_adrmux_row);
        ret = lld_int_register_cb(
            dev,
            0,
            address,
            &pipe_mgr_tof_ecc_map_ram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      /* stats meters ecc */
      address = offsetof(
          Tofino,
          pipes[phy_pipe].mau[stage].rams.match.adrdist.intr_status_mau_ad);
      ret = lld_int_register_cb(
          dev,
          0,
          address,
          &pipe_mgr_tof_ecc_meter_stats_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* synth2port error */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        /* Synth2port interrupts are disabled by default.
         * We register the interrupt callback for debug purposes. */
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.map_alu.row[row]
                               .i2portctl.intr_status_mau_synth2port);
        ret = lld_int_register_cb(
            dev,
            0,
            address,
            &pipe_mgr_tof_synth2port_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      for (row = 0; row < PIPE_MGR_INTR_MAX_SEL_ALU_ROWS; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.map_alu.meter_group[row]
                               .selector.intr_status_mau_selector_alu);
        ret = lld_int_register_cb(
            dev,
            0,
            address,
            &pipe_mgr_tof_sel_alu_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }

      address =
          offsetof(Tofino, pipes[phy_pipe].mau[stage].dp.intr_status_mau_imem);
      ret = lld_int_register_cb(
          dev,
          0,
          address,
          &pipe_mgr_tof_imem_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

/* Handle mirror error */
uint32_t pipe_mgr_tof_mirror_err_handle(bf_dev_id_t dev,
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
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t entry_0 = 0, entry_1 = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);

  LOG_TRACE(
      "Mirror ecc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF_INTR_MIRR_NUM_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_POINTER_FIFO,
                     "Dual ecc error for pointer fifo");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, (0x1u << bitpos));
          break;
        case 1:
          address = offsetof(Tofino,
                             pipes[phy_pipe]
                                 .deparser.mirror.mir_buf_regs.mir_glb_group
                                 .mir_addr_err_idesc);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          /* asic has 2 memories with same info, correct both */
          entry_0 = getp_mir_buf_regs_mir_addr_err_idesc_addr_err_i0desc(&data);
          entry_1 = getp_mir_buf_regs_mir_addr_err_idesc_addr_err_i1desc(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry_0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_IG,
                     "Mirroring dual ecc error in ingress");
#ifdef PIPE_MGR_INTR_MIRROR_TEST
          for (entry_0 = 0, entry_1 = 0; entry_0 < 100; entry_0++, entry_1++) {
            LOG_TRACE("Fixing mirror dual ecc entry (ing) at index %d",
                      entry_0);
#endif
            bf_mirror_ecc_correct(
                pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, entry_0);
            bf_mirror_ecc_correct(
                pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, entry_1);
#ifdef PIPE_MGR_INTR_MIRROR_TEST
          }
#endif
          break;
        case 2:
          address = offsetof(Tofino,
                             pipes[phy_pipe]
                                 .deparser.mirror.mir_buf_regs.mir_glb_group
                                 .mir_addr_err_edesc);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          /* asic has 2 memories with same info, correct both */
          entry_0 = getp_mir_buf_regs_mir_addr_err_edesc_addr_err_e0desc(&data);
          entry_1 = getp_mir_buf_regs_mir_addr_err_edesc_addr_err_e1desc(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry_0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_EG,
                     "Mirroring dual ecc error for egress");
#ifdef PIPE_MGR_INTR_MIRROR_TEST
          for (entry_0 = 0, entry_1 = 0; entry_0 < 100; entry_0++, entry_1++) {
            LOG_TRACE("Fixing mirror dual ecc entry (eg) at index %d", entry_0);
#endif
            bf_mirror_ecc_correct(
                pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, entry_0);
            bf_mirror_ecc_correct(
                pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, entry_1);
#ifdef PIPE_MGR_INTR_MIRROR_TEST
          }
#endif
          break;
        case 3:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT_DESC,
                     "Mirror Dual ecc error for output descriptor");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_BUFFER,
                     "Mirror Dual ecc error for data buffer");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 5:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DROP_NEG,
                     "Mirror Drop negative Mirroring packet");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DROP_COAL,
                     "Mirror Drop coalescing packet");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 7:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_IG_DIS_SESS,
                     "Mirror packet received on ingress for disable session");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_EG_DIS_SESS,
                     "Mirror packet received on egress for disable session");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 10:
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_IG,
                     "Mirroring single bit ecc error in ingress");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          /* mem addr for single bit err not available, just log error */
          break;
        case 11:
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_EG,
                     "Mirroring single bit ecc error for egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          /* mem addr for single bit err not available, just log error */
          break;
        case 12:
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT_DESC,
                     "Mirroring Single ecc error for output descriptor");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 13:
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_BUFFER,
                     "Mirroring Single ecc error for data buffer");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 14:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT,
                     "mirror output overflow");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 15:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_CRC12,
                     "Mirror CRC12 error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 16:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT,
                     "Mirror output pop error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
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

/* Register for mirror interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_mirror_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for mirror ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* mirror */
    address =
        offsetof(Tofino,
                 pipes[phy_pipe]
                     .deparser.mirror.mir_buf_regs.mir_glb_group.mir_int_stat);
    ret = lld_int_register_cb(
        dev_info->dev_id,
        0,
        address,
        &pipe_mgr_tof_mirror_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* TM PRE error */
uint32_t pipe_mgr_tof_tm_pre_err_handle(bf_dev_id_t dev,
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
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t sbe_ram_type = 0, sbe_addr = 0;
  uint32_t mbe_ram_type = 0, mbe_addr = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM pre error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Read the mbe errlog reg value */
  address =
      offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[phy_pipe].mbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  mbe_ram_type = getp_pre_mbe_log_ram(&data);
  mbe_addr = getp_pre_mbe_log_addr(&data);
  /* Read the sbe errlog reg value */
  address =
      offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[phy_pipe].sbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  sbe_ram_type = getp_pre_sbe_log_ram(&data);
  sbe_addr = getp_pre_sbe_log_addr(&data);
  for (bitpos = 0; bitpos < PIPE_MGR_INTR_TM_PRE_NUM_ERR; bitpos++) {
    /* Bits 14-31 are ecc errors */
    if (intr_status_val & (0x1u << bitpos)) {
#ifdef PIPE_MGR_INTR_TM_PRE_TEST
      int test_addr = 0;
      for (test_addr = 0; test_addr < 256; test_addr++) {
        sbe_addr = test_addr;
        mbe_addr = test_addr;
        sbe_ram_type = 0xffff;
        mbe_ram_type = 0xffff;
        LOG_TRACE("TM pre error pipe %d, bitpos %d, offset 0x%x",
                  pipe,
                  bitpos,
                  test_addr);
#endif
        switch (bitpos) {
          case (PIPE_INTR_PRE_RAM_FIFO +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // fifo_mbe
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       mbe_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_FIFO,
                       "TM PRE FIFO multi bit error");
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;
          case (PIPE_INTR_PRE_RAM_MIT +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // mit_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_MIT,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_MIT,
                         "TM PRE MIT multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_mit(dev, phy_pipe, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT0_BM +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // lit0_bm_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_BM,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT0_BM,
                         "TM PRE LIT0 BM multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_lit_bm(dev, 0, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT1_BM +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // lit1_bm_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_BM,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT1_BM,
                         "TM PRE LIT1 BM multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_lit_bm(dev, 1, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT0_NP +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // lit0_np_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_NP,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT0_NP,
                         "TM PRE LIT0 NP multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_lit_np(dev, 0, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT1_NP +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // lit1_np_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_NP,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT1_NP,
                         "TM PRE LIT1 NP multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_lit_np(dev, 1, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_PMT0 +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // pmt0_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT0,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_PMT0,
                         "TM PRE PMT0 multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_pmt(dev, 0, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_PMT1 +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // pmt1_mbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT1,
                                               mbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         mbe_addr,
                         BF_ERR_TYPE_MULTI_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_PMT1,
                         "TM PRE PMT1 multi bit error at addr 0x%x",
                         mbe_addr);
              mc_mgr_ecc_correct_pmt(dev, 1, mbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_RDM +
                PIPE_MGR_TOF_INTR_PRE_MBE_START):  // rdm_mbe
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       mbe_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_RDM,
                       "TM PRE RDM multi bit error at addr 0x%x",
                       mbe_addr);
            /* Disable the interrupt as we do not want it to keep coming */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;
          case (PIPE_INTR_PRE_RAM_FIFO +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // fifo_sbe
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       sbe_addr,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_FIFO,
                       "TM PRE FIFO single bit error at addr 0x%x",
                       sbe_addr);
            break;
          case (PIPE_INTR_PRE_RAM_MIT +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // mit_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_MIT,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_MIT,
                         "TM PRE MIT single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_mit(dev, phy_pipe, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT0_BM +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // lit0_bm_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_BM,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT0_BM,
                         "TM PRE LIT0 BM single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_lit_bm(dev, 0, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT1_BM +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // lit1_bm_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_BM,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT1_BM,
                         "TM PRE LIT1 BM single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_lit_bm(dev, 1, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT0_NP +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // lit0_np_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_NP,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT0_NP,
                         "TM PRE LIT0 NP single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_lit_np(dev, 0, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_LIT1_NP +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // lit1_np_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_NP,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_LIT1_NP,
                         "TM PRE LIT1 NP single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_lit_np(dev, 1, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_PMT0 +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // pmt0_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT0,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_PMT0,
                         "TM PRE PMT0 single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_pmt(dev, 0, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_PMT1 +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // pmt1_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT1,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_PMT1,
                         "TM PRE PMT1 single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_pmt(dev, 1, sbe_addr);
            }
            break;
          case (PIPE_INTR_PRE_RAM_RDM +
                PIPE_MGR_TOF_INTR_PRE_SBE_START):  // rdm_sbe
            if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_RDM,
                                               sbe_ram_type)) {
              BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                         dev,
                         pipe,
                         0,
                         sbe_addr,
                         BF_ERR_TYPE_SINGLE_BIT_ECC,
                         BF_ERR_BLK_TM_PRE,
                         BF_ERR_LOC_TM_PRE_RDM,
                         "TM PRE RDM single bit error at addr 0x%x",
                         sbe_addr);
              mc_mgr_ecc_correct_rdm(dev, sbe_addr);
            }
            break;
          default:
            break;
        }
#ifdef PIPE_MGR_INTR_TM_PRE_TEST
      }
#endif
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM CAA error */
static uint32_t pipe_mgr_tof_tm_caa_err_handle(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(userdata);
  UNUSED(subdev_id);

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // device_select.tm_top.tm_caa.intr.status

  // bit 0: pktdrop_err
  if (intr_status_val & 0x01u) {
    /* Note that this condition is considered fatal on Tofino1
     * due to a hardware resource leak. */
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               0,
               0,
               0,
               BF_ERR_TYPE_PKT_DROP,
               BF_ERR_BLK_TM_CAA,
               BF_ERR_LOC_TM_CAA,
               "TM CAA lost packet error");
    LOG_ERROR("Dev %d TM CAA lost packet error", dev);
    /* No further action.*/
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register for tm interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_tm_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for TM interrupts");

  /* CAA Common */
  ret = lld_int_register_cb(
      dev_info->dev_id,
      0,
      offsetof(Tofino, device_select.tm_top.tm_caa.intr.status),
      &pipe_mgr_tof_tm_caa_err_handle,
      (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, 0, 0, 0)));
  PIPE_MGR_ASSERT(ret != -1);

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* TM PRE */
    address = offsetof(Tofino,
                       device_select.tm_top.tm_pre_top.pre[phy_pipe].int_stat);
    ret = lld_int_register_cb(
        dev_info->dev_id,
        0,
        address,
        &pipe_mgr_tof_tm_pre_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Correct ecc error in po action ram in parser */
static pipe_status_t pipe_mgr_tof_parser_action_ram_ecc_correct(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    int parser,
    int dir,
    uint32_t row,
    uint32_t blk_id,
    uint64_t *address) {
  UNUSED(blk_id);

  bf_dev_pipe_t phy_pipe = 0;
  int data_size = 0;
  uint64_t offset = 0;
  pipe_mgr_drv_ses_state_t *st = NULL;
  pipe_mgr_drv_buf_t *b = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t *src_data = NULL;
  pipe_prsr_instance_hdl_t prsr_instance_hdl;
  union pipe_parser_bin_config_t *bin_cfg;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (row >= TOF_PARSER_DEPTH) {
    LOG_ERROR("Dev %d log-pipe %d prsr %d dir %d action row %d doesn't exist",
              dev,
              pipe,
              parser,
              dir,
              row);
    return PIPE_INVALID_ARG;
  }
  status = pipe_mgr_get_prsr_instance_hdl(
      dev, pipe, dir, parser, &prsr_instance_hdl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Fail to get parser instance handler, dev %d, pipe %d, %s, "
        "parser %d, error %s",
        __func__,
        dev,
        pipe,
        dir ? "ingress" : "egress",
        parser,
        pipe_str_err(status));
    return status;
  }
  status = pipe_mgr_prsr_instance_get_bin_cfg(
      dev, pipe, dir, prsr_instance_hdl, &bin_cfg);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Fail to get parser instance shadowed data, dev %d, pipe %d, %s, "
        "parser %d, parser instance handle 0x%x, error %s",
        __func__,
        dev,
        pipe,
        dir ? "ingress" : "egress",
        parser,
        prsr_instance_hdl,
        pipe_str_err(status));
    return status;
  }
  data_size = PIPE_MGR_TOF_PO_WORD_WIDTH;
  offset = row * (data_size / 16);
  src_data = bin_cfg->tof.po_action_data[row];
  *address = PIPE_PRSR_ADDR(dev, dir).tof.po_action_addr +
             (pipe_top_level_pipes_array_element_size * phy_pipe) +
             (PIPE_PRSR_ADDR(dev, dir).tof.prsr_step * parser) + offset;
  st = pipe_mgr_drv_get_ses_state(
      &pipe_mgr_ctx->int_ses_hndl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }

  if (src_data) {
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
      status = PIPE_NO_SYS_RESOURCES;
      goto done;
    }

    PIPE_MGR_MEMCPY(b->addr, src_data, data_size);

    /* Rewite the parser row again using write block (block id is not used) */
    status = pipe_mgr_drv_blk_wr(&pipe_mgr_ctx->int_ses_hndl,
                                 16,
                                 data_size / 16,
                                 1,
                                 *address,
                                 1u << pipe,
                                 b);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Write block push for parser ecc correction "
          "error %s",
          __func__,
          pipe_str_err(status));
    }
  } else {
    LOG_ERROR(
        "Failed to correct prsr error, no data: dev %d log-pipe %d dir %d "
        "prsr %d",
        dev,
        pipe,
        dir,
        parser);
    status = PIPE_UNEXPECTED;
    PIPE_MGR_DBGCHK(src_data);
    goto done;
  }

done:
  return status;
}

/* Correct ecc error in word0/word1 in parser */
static pipe_status_t pipe_mgr_tof_parser_tcam_err_correct(bf_dev_id_t dev,
                                                          bf_dev_pipe_t pipe,
                                                          int parser,
                                                          int dir,
                                                          int word,
                                                          uint32_t row) {
  bf_dev_pipe_t phy_pipe = 0;
  int data_size = 0;
  uint64_t address = 0;
  pipe_mgr_drv_ses_state_t *st = NULL;
  pipe_mgr_drv_buf_t *b = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t *src_data = NULL;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (row >= TOF_PARSER_DEPTH) {
    LOG_ERROR("Dev %d log-pipe %d prsr %d dir %d row %d doesn't exist",
              dev,
              pipe,
              parser,
              dir,
              row);
    status = PIPE_INVALID_ARG;
    return status;
  }
  data_size = PIPE_MGR_TOF_TCAM_WORD_WIDTH;
  if (word == 0) {
    address = PIPE_PRSR_ADDR(dev, dir).tof.word0_addr +
              (pipe_top_level_pipes_array_element_size * phy_pipe) +
              (PIPE_PRSR_ADDR(dev, dir).tof.prsr_step * parser) + row;
    src_data = PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe, parser)
                   .tof[dir]
                   .word0_data[row];
  } else {
    address = PIPE_PRSR_ADDR(dev, dir).tof.word1_addr +
              (pipe_top_level_pipes_array_element_size * phy_pipe) +
              (PIPE_PRSR_ADDR(dev, dir).tof.prsr_step * parser) + row;
    src_data = PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe, parser)
                   .tof[dir]
                   .word1_data[row];
  }

  if (!src_data) {
    LOG_ERROR(
        "Failed to correct prsr tcam, no data: dev %d log-pipe %d dir %d "
        "prsr %d",
        dev,
        pipe,
        dir,
        parser);
    PIPE_MGR_DBGCHK(src_data);
    return PIPE_UNEXPECTED;
  }
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
    status = PIPE_NO_SYS_RESOURCES;
    goto done;
  }

  LOG_TRACE(
      "Dev %d logPipe %d %cPrsr %d word%d row %d parity error, phyAddr "
      "0x%" PRIx64 " len %d",
      dev,
      pipe,
      dir ? 'e' : 'i',
      parser,
      word,
      row,
      address,
      data_size);

  PIPE_MGR_MEMCPY(b->addr, src_data, data_size);

  status = pipe_mgr_drv_blk_wr(&pipe_mgr_ctx->int_ses_hndl,
                               16,
                               data_size / 16,
                               1,
                               address,
                               1u << pipe,
                               b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Write block push for parser ecc correction "
        "error %s",
        __func__,
        pipe_str_err(status));
  }

done:
  return status;
}

/* Handle error in parser */
static uint32_t pipe_mgr_tof_parser_err_handle(bf_dev_id_t dev,
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
  int parser = 0;
  uint32_t err_addr = 0, blk_id = 0;
  int bitpos = 0, dir = 0, bit_max;
  uint64_t full_err_addr = 0;
  const pipe_mgr_intr_userdata_t *userdata_p =
      (pipe_mgr_intr_userdata_t *)userdata;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  bit_max = PIPE_MGR_TOF_INTR_PRSR_NUM_ECC_ERR;
  parser = userdata_p->row;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  dir = userdata_p->stage;  // direction stored in stage
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Parser intr (dev %d, pipe %d, dir %d, parser %d):"
      " Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      dir,
      parser,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < bit_max; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // no_tcam_match_err
        case 1:  // partial_hdr_err
        case 2:  // ctr_range_err
        case 3:  // timeout_iter_err
        case 4:  // timeout_cycle_err
        case 5:  // src_ext_err
        case 6:  // dst_cont_err
        case 7:  // phv_owner_err
        case 8:  // multi_wr_err
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 9:  // aram_sbe
          if (dir == 0) {
            // ingress
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ibp18_reg.ibp_reg[parser]
                                   .prsr_reg.aram_sbe_errlog);
          } else {
            // egress
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ebp18_reg.ebp_reg[parser]
                                   .prsr_reg.aram_sbe_errlog);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_prsr_reg_main_rspec_aram_sbe_errlog_addr(&data);
          blk_id = getp_prsr_reg_main_rspec_aram_sbe_errlog_blkid(&data);
          pipe_mgr_tof_parser_action_ram_ecc_correct(
              dev, pipe, parser, dir, err_addr, blk_id, &full_err_addr);
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
          if (dir == 0) {
            // ingress
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ibp18_reg.ibp_reg[parser]
                                   .prsr_reg.aram_mbe_errlog);
          } else {
            // egress
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ebp18_reg.ebp_reg[parser]
                                   .prsr_reg.aram_mbe_errlog);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_prsr_reg_main_rspec_aram_mbe_errlog_addr(&data);
          blk_id = getp_prsr_reg_main_rspec_aram_mbe_errlog_blkid(&data);
          pipe_mgr_tof_parser_action_ram_ecc_correct(
              dev, pipe, parser, dir, err_addr, blk_id, &full_err_addr);
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
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 13:  // ibuf_oflow_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_INP_BUFF,
                     "Parser input buffer overflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 14:  // ibuf_uflow_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_INP_BUFF,
                     "Parser input buffer underflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 15:  // op_fifo_oflow_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_OUT_FIFO,
                     "Parser output fifo overflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 16:  // op_fifo_uflow_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_OUT_FIFO,
                     "Parser output fifo underflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;
        case 17:  // tcam_par_err
          /* Tcam memory will be refreshed periodically */
          if (dir == 0) {  // ingress
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ibp18_reg.ibp_reg[parser]
                                   .prsr_reg.tcam_par_errlog);
          } else {
            address = offsetof(Tofino,
                               pipes[phy_pipe]
                                   .pmarb.ebp18_reg.ebp_reg[parser]
                                   .prsr_reg.tcam_par_errlog);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_prsr_reg_main_rspec_tcam_par_errlog_addr(&data);
          blk_id = err_addr >> 8;
          err_addr &= 0xFF;
          pipe_mgr_tof_parser_tcam_err_correct(
              dev, pipe, parser, dir, blk_id, err_addr);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_PARITY,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_TCAM_PARITY,
                     "Tcam parity error");
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

/* Register for parser interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_parser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0, parser;

  LOG_TRACE("Pipe-mgr Registering for parser ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (parser = 0; parser < TOF_NUM_PARSERS; parser++) {
      /* ingress */
      address = offsetof(
          Tofino,
          pipes[phy_pipe].pmarb.ibp18_reg.ibp_reg[parser].prsr_reg.intr.status);
      ret = lld_int_register_cb(dev_info->dev_id,
                                0,
                                address,
                                &pipe_mgr_tof_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev_info->dev_id, pipe, 0, parser)));
      PIPE_MGR_DBGCHK(ret != -1);
      /* egress */
      address = offsetof(
          Tofino,
          pipes[phy_pipe].pmarb.ebp18_reg.ebp_reg[parser].prsr_reg.intr.status);
      ret = lld_int_register_cb(dev_info->dev_id,
                                0,
                                address,
                                &pipe_mgr_tof_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev_info->dev_id, pipe, 1, parser)));
      PIPE_MGR_DBGCHK(ret != -1);
    }
  }
  return PIPE_SUCCESS;
}

/* Handle error in Egress Buffer port */
static uint32_t pipe_mgr_tof_ebuf_disp_err_handle(bf_dev_id_t dev,
                                                  bf_subdev_id_t subdev_id,
                                                  uint32_t intr_address,
                                                  uint32_t intr_status_val,
                                                  uint32_t enable_hi_addr,
                                                  uint32_t enable_lo_addr,
                                                  void *userdata) {
  UNUSED(subdev_id);

  bf_dev_pipe_t pipe = 0;
  int parser = 0;
  const pipe_mgr_intr_userdata_t *userdata_p =
      (pipe_mgr_intr_userdata_t *)userdata;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Get pipe and parser info from userdata */
  pipe = userdata_p->pipe;
  parser = userdata_p->row;

  // bit 0: ebuf_ovf
  if (intr_status_val & 0x01u) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               parser,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_EBUF,
               BF_ERR_LOC_EBUF,
               "Egress Buffer Dispatch fifo overflow");
    /* Disable interrupt */
    pipe_mgr_interrupt_set_enable_val(dev, 0, enable_hi_addr, 0x1u);
    LOG_ERROR(
        "Egress Buffer Dispatch fifo overflow pipe %d parser %d", pipe, parser);
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in Egress Buffer bypass fifo */
static uint32_t pipe_mgr_tof_ebuf_fifo_err_handle(bf_dev_id_t dev,
                                                  bf_subdev_id_t subdev_id,
                                                  uint32_t intr_address,
                                                  uint32_t intr_status_val,
                                                  uint32_t enable_hi_addr,
                                                  uint32_t enable_lo_addr,
                                                  void *userdata) {
  UNUSED(subdev_id);
  bf_dev_pipe_t pipe = 0;
  int bitpos = 0;
  int parser = 0;
  const pipe_mgr_intr_userdata_t *userdata_p =
      (pipe_mgr_intr_userdata_t *)userdata;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Get pipe and parser info from userdata */
  pipe = userdata_p->pipe;
  parser = userdata_p->row;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF_INTR_EBUF_BYPASS_NUM_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // egrbypff_ecc_dual_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     parser,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_EBUF,
                     BF_ERR_LOC_EBUF,
                     "Egress Bypass fifo ECC uncorrectable error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, (0x1u << bitpos));
          LOG_ERROR(
              "Egress Bypass fifo ECC uncorrectable error pipe %d parser %d",
              pipe,
              parser);
          break;
        case 1:  // egrbypff_ecc_sngl_err
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     parser,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_EBUF,
                     BF_ERR_LOC_EBUF,
                     "Egress Bypass fifo cell ECC correctable error");
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, (0x1u << bitpos));
          LOG_ERROR(
              "Egress Bypass fifo cell ECC correctable error pipe %d parser %d",
              pipe,
              parser);
          break;

        case 2:  // egrbypff_ovf0_err
        case 3:  // egrbypff_ovf1_err
        case 4:  // egrbypff_ovf2_err
        case 5:  // egrbypff_ovf3_err
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     parser,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_OUT_FIFO,
                     "Egress Bypass fifo overflow error on channel %d",
                     (bitpos - 2));
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          LOG_ERROR(
              "Egress Bypass fifo overflow error on pipe %d parser %d channel "
              "%d",
              pipe,
              parser,
              (bitpos - 2));
          break;

        default:
          LOG_ERROR("ebuf bypass fifo interrupt ID undefined");
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
/* Register for Egress buffer interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_ebuf_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0, parser;

  LOG_TRACE("Pipe-mgr Registering for egress buffer interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (parser = 0; parser < TOF_NUM_PARSERS; parser++) {
      /* ebuf_disp_regs : Egress Buffer Port Registers */
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .pmarb.ebp18_reg.egrNx_reg[parser]
                             .ebuf_disp_regs.int_stat);
      ret = lld_int_register_cb(dev_info->dev_id,
                                0,
                                address,
                                &pipe_mgr_tof_ebuf_disp_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev_info->dev_id, pipe, 1, parser)));
      PIPE_MGR_DBGCHK(ret != -1);

      /* ebuf_fifo_regs: Egress bypass fifo */
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .pmarb.ebp18_reg.egrNx_reg[parser]
                             .ebuf_fifo_regs.int_stat);
      ret = lld_int_register_cb(dev_info->dev_id,
                                0,
                                address,
                                &pipe_mgr_tof_ebuf_fifo_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev_info->dev_id, pipe, 1, parser)));
      PIPE_MGR_DBGCHK(ret != -1);
    }
  }
  return PIPE_SUCCESS;
}
/* Handle error in ig_deparser */
static uint32_t pipe_mgr_tof_ig_deparser_err_handle(bf_dev_id_t dev,
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
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, row = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);

  LOG_TRACE("ig_deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF_INTR_DEPRSR_NUM_ECC_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // pv_tbl0_sbe
        case 4:  // pv_csr_sbe, CPU reads triggers this
          address = offsetof(
              Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.pv_tbl0_sbe_errlog);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          row = getp_dprsr_header_ingress_only_g_pv_tbl0_sbe_errlog_addr(&data);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          for (row = 0; row < 1000; row++) {
            LOG_TRACE("Ig-Dpsr PV Tbl0 sbe at row %d, pipe %d", row, pipe);
#endif
            /* Use only 13 bits, msb indicates tbl 0 or 1 */
            row &= 0x1fff;
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       row,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_DEPRSR,
                       BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0,
                       "Ig-Deparser Pipe Vector Tbl0 single bit error at"
                       " row %d",
                       row);
            LOG_TRACE("Ig-Dpsr Pipe Vector Tbl0 single bit err at row %d ",
                      row);
            /* Fix error */
            mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          }
#endif
          break;
        case 1:  // pv_tbl0_mbe
        case 5:  // pv_csr_mbe, CPU reads triggers this
          address = offsetof(
              Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.pv_tbl0_mbe_errlog);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          row = getp_dprsr_header_ingress_only_g_pv_tbl0_mbe_errlog_addr(&data);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          for (row = 0; row < 1000; row++) {
            LOG_TRACE("Ig-Dpsr PV Tbl0 mbe at row %d, pipe %d", row, pipe);
#endif
            /* Use only 13 bits, msb indicates tbl 0 or 1 */
            row &= 0x1fff;
            BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       row,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_DEPRSR,
                       BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0,
                       "Ig-Deparser Pipe Vector Tbl0 multi bit error"
                       " at row %d",
                       row);
            LOG_TRACE("Ig-Dpsr Pipe Vector Tbl0 multi bit err at row %d ", row);
            mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          }
#endif
          break;
        case 2:  // pv_tbl1_sbe
          address = offsetof(
              Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.pv_tbl1_sbe_errlog);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          row = getp_dprsr_header_ingress_only_g_pv_tbl1_sbe_errlog_addr(&data);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          for (row = 0; row < 1000; row++) {
            LOG_TRACE("Ig-Dpsr PV Tbl1 sbe at row %d, pipe %d", row, pipe);
#endif
            /* Use only 13 bits, msb indicates tbl 0 or 1 */
            row &= 0x1fff;
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       row,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_DEPRSR,
                       BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL1,
                       "Ig-Deparser Pipe Vector Tbl1 single bit error"
                       " at row %d",
                       row);
            LOG_TRACE("Ig-Dpsr Pipe Vector Tbl1 single bit err at row %d ",
                      row);
            mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          }
#endif
          break;
        case 3:  // pv_tbl1_mbe
          address = offsetof(
              Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.pv_tbl1_mbe_errlog);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          row = getp_dprsr_header_ingress_only_g_pv_tbl1_mbe_errlog_addr(&data);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          for (row = 0; row < 1000; row++) {
            LOG_TRACE("Ig-Dpsr PV Tbl1 mbe at row %d, pipe %d", row, pipe);
#endif
            /* Use only 13 bits, msb indicates tbl 0 or 1 */
            row &= 0x1fff;
            BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       row,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_DEPRSR,
                       BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL1,
                       "Ig-Deparser Pipe Vector Tbl1 multi bit error"
                       "at row %d",
                       row);
            LOG_TRACE("Ig-Dpsr Pipe Vector Tbl1 multi bit err at row %d ", row);
            mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
#ifdef PIPE_MGR_INTR_IG_DEPARSER_TEST
          }
#endif
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

/* Register for ig_deparser interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_ig_deparser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for ig_deparser ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    address =
        offsetof(Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.intr.status);
    ret = lld_int_register_cb(
        dev_info->dev_id,
        0,
        address,
        &pipe_mgr_tof_ig_deparser_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Register for pgr interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_pgr_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for pgr ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    address =
        offsetof(Tofino, pipes[phy_pipe].pmarb.pgr_reg.pgr_common.int_stat);
    ret = lld_int_register_cb(
        dev_info->dev_id,
        0,
        address,
        &pipe_mgr_tof_pgr_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Register for gfm interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_gfm_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  dev_stage_t stage = 0;

  LOG_TRACE("Pipe-mgr Registering for GFM ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      address = offsetof(
          Tofino, pipes[phy_pipe].mau[stage].dp.intr_status_mau_gfm_hash);
      ret = lld_int_register_cb(
          dev_info->dev_id,
          0,
          address,
          &pipe_mgr_gfm_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev_info->dev_id, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_pbc2_intr_handle(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t status_reg,
                                          uint32_t status_value,
                                          uint32_t enable_hi_reg,
                                          uint32_t enable_lo_reg,
                                          void *userdata) {
  UNUSED(enable_hi_reg);
  UNUSED(enable_lo_reg);
  UNUSED(userdata);
  UNUSED(subdev_id);

  if (status_value & 0x1FF) {
    LOG_ERROR("Dev %d PBus2 DMA error 0x%x", dev_id, status_value);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_PBC,
               "PBus2 DMA Error 0x%x",
               status_value);

    pipe_mgr_non_pipe_interrupt_set_enable_val(
        dev_id, 0, enable_hi_reg, (status_value & 0x1FF));
  }

  // default is to just clear them
  lld_write_register(dev_id, status_reg, status_value);
  return 0;
}

static uint32_t pipe_mgr_pbc3_intr_handle(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t status_reg,
                                          uint32_t status_value,
                                          uint32_t enable_hi_reg,
                                          uint32_t enable_lo_reg,
                                          void *userdata) {
  UNUSED(enable_hi_reg);
  UNUSED(enable_lo_reg);
  UNUSED(userdata);
  UNUSED(subdev_id);

  if (status_value & ((0x7 << 26) | 0x1FFF)) {
    LOG_ERROR("Dev %d PBus3 DMA error 0x%x", dev_id, status_value);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_PBC,
               "PBus3 DMA Error 0x%x",
               status_value);
    pipe_mgr_non_pipe_interrupt_set_enable_val(
        dev_id, 0, enable_hi_reg, (status_value & ((0x7 << 26) | 0x1FFF)));
  }

  // default is to just clear them
  lld_write_register(dev_id, status_reg, status_value);
  return 0;
}

static uint32_t pipe_mgr_mbc_intr_handle(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         uint32_t status_reg,
                                         uint32_t status_value,
                                         uint32_t enable_hi_reg,
                                         uint32_t enable_lo_reg,
                                         void *userdata) {
  UNUSED(enable_hi_reg);
  UNUSED(enable_lo_reg);
  UNUSED(userdata);
  UNUSED(subdev_id);

  if (status_value & 0xEA0) {
    LOG_ERROR("Dev %d MBus DMA error 0x%x", dev_id, status_value);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBus DMA Error 0x%x",
               status_value);
    pipe_mgr_non_pipe_interrupt_set_enable_val(
        dev_id, 0, enable_hi_reg, (status_value & 0xEA0));
  }

  // default is to just clear them
  lld_write_register(dev_id, status_reg, status_value);
  return 0;
}

static uint32_t pipe_mgr_cbc_intr_handle(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         uint32_t status_reg,
                                         uint32_t status_value,
                                         uint32_t enable_hi_reg,
                                         uint32_t enable_lo_reg,
                                         void *userdata) {
  UNUSED(enable_hi_reg);
  UNUSED(enable_lo_reg);
  UNUSED(userdata);

  if ((status_value & TOFINO_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY) &&
      pipe_mgr_flow_lrn_is_int_enabled(dev_id)) {
    /* Disable LQ slots on CBUS which will prevent any filter evictions to SBC
     * and stop any new DMAs.  The filters must be paused because the interrupt
     * is only for empty-to-not-empty transitions in the DR and not the DR being
     * non-empty.  We cannot ensure the DR is left empty, to generate the empty-
     * to-non-empty interrupt on the next learn, without pausing learning and
     * servicing the DR completely. */
    lld_dr_cbus_arb_ctrl_set(dev_id, subdev_id, 0x11111011);

    /* Empty the DR. */
    lld_dr_service(dev_id, subdev_id, lld_dr_rx_learn, 10000);
    /* Since lld_dr_service will only update the view once when first starting
     * processing it is possible that the earlier write to to arb_ctrl will take
     * a moment to apply since it is just posted and there may be DMAs in
     * progress resulting in the DR pointer being updated by the chip just after
     * lld_dr_service updates the view, in this case we'd miss processing those
     * later DR entries.  To avoid this, service the DR one more time which will
     * update the view and process the last few DMAs. */
    lld_dr_service(dev_id, subdev_id, lld_dr_rx_learn, 10000);

    /* Clear the interrupt status before enabling LQs on the CBUS. */
    lld_write_register(
        dev_id, status_reg, TOFINO_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY);

    /* Remove this status bit from status_value since we've handled it and
     * cleared the status register.  We don't want to clear this bit again if
     * there are other status bits set and we execute the final
     * lld_write_register call at the end of the function. */
    status_value &= ~TOFINO_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY;

    /* Restore the default weights. */
    lld_dr_cbus_arb_ctrl_set(dev_id, subdev_id, 0x11111111);

    /* If this was the only bit set we are done. */
    if (status_value == TOFINO_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY)
      return PIPE_SUCCESS;
  }
  if (status_value & 0x73600) {
    LOG_ERROR("Dev %d CBus DMA error 0x%x", dev_id, status_value);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_CBC,
               "CBus DMA Error 0x%x",
               status_value);
    pipe_mgr_non_pipe_interrupt_set_enable_val(
        dev_id, 0, enable_hi_reg, (status_value & 0x73600));
  }

  // default is to just clear them
  lld_write_register(dev_id, status_reg, status_value);
  return 0;
}

/* Register for sbc interrupt notifications with lld */
pipe_status_t pipe_mgr_tof_register_sbc_interrupt_notifs(bf_dev_id_t dev) {
  uint32_t address = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for SBC interrupts");

  address = offsetof(Tofino, device_select.pbc.pbc_pbus.int_stat2);
  ret = lld_int_register_cb(dev, 0, address, &pipe_mgr_pbc2_intr_handle, NULL);
  PIPE_MGR_ASSERT(ret != -1);

  address = offsetof(Tofino, device_select.pbc.pbc_pbus.int_stat3);
  ret = lld_int_register_cb(dev, 0, address, &pipe_mgr_pbc3_intr_handle, NULL);
  PIPE_MGR_ASSERT(ret != -1);

  address = offsetof(Tofino, device_select.mbc.mbc_mbus.int_stat);
  ret = lld_int_register_cb(dev, 0, address, &pipe_mgr_mbc_intr_handle, NULL);
  PIPE_MGR_ASSERT(ret != -1);

  address = offsetof(Tofino, device_select.cbc.cbc_cbus.int_stat);
  ret = lld_int_register_cb(dev, 0, address, &pipe_mgr_cbc_intr_handle, NULL);
  PIPE_MGR_ASSERT(ret != -1);

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof_lfltr_err_handle(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  UNUSED(enable_hi_addr);
  UNUSED(enable_lo_addr);
  UNUSED(subdev_id);
  /* Each learn filter block consists of two learn filters.
   * Each learn filter has a bloom filter (BFT) and each bloom filter is
   * implemented with four memories with parity protection.
   * Each learn filter has storage for the learn digests/quanta (LQT) in three
   * memories with ECC protection. */
  const int num_bfts = 2;
  const int num_bft_mems = 4;
  const int num_lqt = 2;
  const int num_lqt_mems = 3;

  const pipe_mgr_intr_userdata_t *userdata_p =
      (pipe_mgr_intr_userdata_t *)userdata;
  bf_dev_pipe_t log_pipe = userdata_p->pipe;

  // device_select.lfltr[].ctrl.intr_stat

  /* Bits [9:2] Bloom Filter Parity Error */
  for (int bft = 0; bft < num_bfts; ++bft) {
    for (int i = 0; i < num_bft_mems; ++i) {
      int intr_bit = 2 + num_bft_mems * bft + i;
      if (intr_status_val & (1u << intr_bit)) {
        LOG_ERROR(
            "Learn Filter %d BFT%d Memory %d parity error", log_pipe, bft, i);
        BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                   dev,
                   log_pipe,
                   0,
                   i,
                   BF_ERR_TYPE_PARITY,
                   BF_ERR_BLK_LFLTR,
                   bft ? BF_ERR_LOC_LFLTR_BFT1 : BF_ERR_LOC_LFLTR_BFT0,
                   "Learn Filter %d BFT%d Memory %d parity error",
                   log_pipe,
                   bft,
                   i);
      }
    }
  }

  /* Bits [15:10] Learn Quanta Multi-Bit ECC Error */
  for (int lqt = 0; lqt < num_lqt; ++lqt) {
    for (int i = 0; i < num_lqt_mems; ++i) {
      int intr_bit = 10 + num_lqt_mems * lqt + i;
      if (intr_status_val & (1u << intr_bit)) {
        LOG_ERROR("Learn Filter %d LQT%d Memory %d multi-bit ECC error",
                  log_pipe,
                  lqt,
                  i);
        BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                   dev,
                   log_pipe,
                   0,
                   i,
                   BF_ERR_TYPE_MULTI_BIT_ECC,
                   BF_ERR_BLK_LFLTR,
                   lqt ? BF_ERR_LOC_LFLTR_LQT1 : BF_ERR_LOC_LFLTR_LQT0,
                   "Learn Filter %d LQT%d Memory %d multi-bit ECC error",
                   log_pipe,
                   lqt,
                   i);
      }
    }
  }

  /* Bits [21:16] Learn Quanta Single-Bit ECC Error */
  for (int lqt = 0; lqt < num_lqt; ++lqt) {
    for (int i = 0; i < num_lqt_mems; ++i) {
      int intr_bit = 16 + num_lqt_mems * lqt + i;
      if (intr_status_val & (1u << intr_bit)) {
        LOG_ERROR("Learn Filter %d LQT%d Memory %d single-bit ECC error",
                  log_pipe,
                  lqt,
                  i);
        BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                   dev,
                   log_pipe,
                   0,
                   i,
                   BF_ERR_TYPE_SINGLE_BIT_ECC,
                   BF_ERR_BLK_LFLTR,
                   lqt ? BF_ERR_LOC_LFLTR_LQT1 : BF_ERR_LOC_LFLTR_LQT0,
                   "Learn Filter %d LQT%d Memory %d single-bit ECC error",
                   log_pipe,
                   lqt,
                   i);
      }
    }
  }

  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

pipe_status_t pipe_mgr_tof_register_lfltr_interrupt_notifs(bf_dev_id_t dev) {
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for Learn Filter interrupts");

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  uint32_t pipe_step = offsetof(Tofino, device_select.lfltr1) -
                       offsetof(Tofino, device_select.lfltr0);

  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_status_t rc =
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    if (rc != PIPE_SUCCESS) return rc;

    uint32_t addr = offsetof(Tofino, device_select.lfltr0.ctrl.int_stat);
    addr += phy_pipe * pipe_step;
    ret = lld_int_register_cb(
        dev,
        0,
        addr,
        &pipe_mgr_tof_lfltr_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, log_pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof_mau_interrupt_mode_set(bf_dev_id_t dev,
                                                         bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  dev_stage_t stage = 0;
  uint32_t tcam_data = 0, sram_data = 0, map_ram_data = 0;
  uint32_t meter_stats_data = 0, synth2port_data = 0;
  uint32_t selector_alu_data = 0, imem_data = 0;
  int idx = 0, row = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  rmt_dev_cfg_t *cfg = &(dev_info->dev_cfg);
  LOG_TRACE(" Setting MAU Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting MAU Interrupt mode to %s", enable ? "Enable" : "Disable");

  if (enable) {
    /* sbe */
    for (idx = 0; idx < cfg->stage_cfg.num_tcam_rows; idx++) {
      tcam_data |= 0x1u << (4 + idx);
    }
    /* sbe and mbe */
    for (idx = 0; idx < (2 * cfg->stage_cfg.num_sram_cols); idx++) {
      sram_data |= 0x1u << idx;
    }
    /* sbe and mbe */
    for (idx = 0; idx < (2 * cfg->stage_cfg.num_map_ram_cols); idx++) {
      map_ram_data |= 0x1u << idx;
    }
    for (idx = 0; idx < (PIPE_MGR_INTR_MAX_METERS + PIPE_MGR_INTR_MAX_STATS);
         idx++) {
      meter_stats_data |= 0x1u << idx;
    }
    meter_stats_data |= 0x1u << 9;  // meter_sweep
    synth2port_data |= INTR_SYNTH2PORT;
    for (idx = 0; idx < PIPE_MGR_INTR_MAX_SEL_ALU_DIR; idx++) {
      selector_alu_data |= 0x1u << idx;
    }
    for (idx = 0; idx < PIPE_MGR_INTR_MAX_IMEM_DIR; idx++) {
      imem_data |= 0x1u << idx;
    }

  } else {
    tcam_data = 0;
    sram_data = 0;
    map_ram_data = 0;
    meter_stats_data = 0;
    synth2port_data = 0;
    selector_alu_data = 0;
    imem_data = 0;
  }

  /* Always enable interrupt hi */
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* tcam */
      address = offsetof(
          Tofino, pipes[phy_pipe].mau[stage].tcams.intr_enable0_mau_tcam_array);
      pipe_mgr_ilist_add_register_write(dev, 0, address, tcam_data);

      /* Sram */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.array.row[row]
                               .intr_enable0_mau_unit_ram_row);
        pipe_mgr_ilist_add_register_write(dev, 0, address, sram_data);
      }

      /* map ram */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.map_alu.row[row]
                               .adrmux.intr_enable0_mau_adrmux_row);
        pipe_mgr_ilist_add_register_write(dev, 0, address, map_ram_data);
      }

      /* meter, stats, meter sweep */
      address = offsetof(
          Tofino,
          pipes[phy_pipe].mau[stage].rams.match.adrdist.intr_enable0_mau_ad);
      pipe_mgr_ilist_add_register_write(dev, 0, address, meter_stats_data);

      /* selector alu */
      for (row = 0; row < PIPE_MGR_INTR_MAX_SEL_ALU_ROWS; row++) {
        address = offsetof(Tofino,
                           pipes[phy_pipe]
                               .mau[stage]
                               .rams.map_alu.meter_group[row]
                               .selector.intr_enable0_mau_selector_alu);
        pipe_mgr_ilist_add_register_write(dev, 0, address, selector_alu_data);
      }

      /* imem */
      address =
          offsetof(Tofino, pipes[phy_pipe].mau[stage].dp.intr_enable0_mau_imem);
      pipe_mgr_ilist_add_register_write(dev, 0, address, imem_data);
    }
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for mirror */
static pipe_status_t pipe_mgr_tof_mirror_interrupt_mode_set(bf_dev_id_t dev,
                                                            bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  uint32_t mirror_data = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }

  LOG_TRACE(" Setting Mirror Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting Mirror Interrupt mode to %s",
            enable ? "Enable" : "Disable");

  if (enable) {
    int idx = 0;
    /* Bits 0-16 */
    for (idx = 0; idx < PIPE_MGR_TOF_INTR_MIRR_NUM_ERR; idx++) {
      mirror_data |= 0x1u << idx;
    }
  } else {
    mirror_data = 0;
  }

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    address = offsetof(
        Tofino,
        pipes[phy_pipe].deparser.mirror.mir_buf_regs.mir_glb_group.mir_int_en);
    pipe_mgr_ilist_add_register_write(dev, 0, address, mirror_data);
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for TM */
static pipe_status_t pipe_mgr_tof_tm_interrupt_mode_set(bf_dev_id_t dev,
                                                        bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  uint32_t tm_pre_data = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }

  LOG_TRACE(" Setting TM Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* CAA Common */
  lld_write_register(dev,
                     offsetof(Tofino, device_select.tm_top.tm_caa.intr.enable0),
                     enable ? 0x1 : 0);

  if (enable) {
    int idx = 0;
    /* Bits 14-31 are ecc errors, do not enable other errors */
    for (idx = (PIPE_INTR_PRE_RAM_FIFO + PIPE_MGR_TOF_INTR_PRE_MBE_START);
         idx <= (PIPE_INTR_PRE_RAM_RDM + PIPE_MGR_TOF_INTR_PRE_SBE_START);
         idx++) {
      tm_pre_data |= 0x1u << idx;
    }
  }
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* Always enable interrupt hi */
    address =
        offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[phy_pipe].int_en0);
    lld_write_register(dev, address, tm_pre_data);
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for parser */
static pipe_status_t pipe_mgr_tof_parser_interrupt_mode_set(bf_dev_id_t dev,
                                                            bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int parser = 0;
  uint32_t parser_data = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  LOG_TRACE(" Setting Parser Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting Parser Interrupt mode to %s",
            enable ? "Enable" : "Disable");
  if (enable) {
    int idx = 0;
    /* Bits 0-17 */
    for (idx = 0; idx < PIPE_MGR_TOF_INTR_PRSR_NUM_ECC_ERR; idx++) {
      parser_data |= 0x1u << idx;
    }
  } else {
    parser_data = 0;
  }

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (parser = 0; parser < TOF_NUM_PARSERS; parser++) {
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .pmarb.ibp18_reg.ibp_reg[parser]
                             .prsr_reg.intr.enable0);
      pipe_mgr_ilist_add_register_write(dev, 0, address, parser_data);

      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .pmarb.ebp18_reg.ebp_reg[parser]
                             .prsr_reg.intr.enable0);
      pipe_mgr_ilist_add_register_write(dev, 0, address, parser_data);
    }
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for ig_deparser */
static pipe_status_t pipe_mgr_tof_deparser_interrupt_mode_set(bf_dev_id_t dev,
                                                              bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  uint32_t ig_deparser_data = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  LOG_TRACE(" Setting ig_deparser Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting ig_deparser Interrupt mode to %s",
            enable ? "Enable" : "Disable");
  if (enable) {
    int idx = 0;
    /* Bits 0-5 are ecc errors */
    for (idx = 0; idx < PIPE_MGR_TOF_INTR_DEPRSR_NUM_ECC_ERR; idx++) {
      ig_deparser_data |= 0x1u << idx;
    }
  }
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    address =
        offsetof(Tofino, pipes[phy_pipe].deparser.hdr.hir.ingr.intr.enable0);
    pipe_mgr_ilist_add_register_write(dev, 0, address, ig_deparser_data);
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for PGR (pktgen) */
static pipe_status_t pipe_mgr_tof_pgr_interrupt_mode_set(bf_dev_id_t dev,
                                                         bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  uint32_t pgr_data = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  LOG_TRACE(" Setting PGR Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting PGR Interrupt mode to %s", enable ? "Enable" : "Disable");
  if (enable) {
    /* Bit 8 - buffer mbe, Bit 9 - buffer sbe */
    pgr_data |= 0x1u << 8;
    pgr_data |= 0x1u << 9;
  }
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    address =
        offsetof(Tofino, pipes[phy_pipe].pmarb.pgr_reg.pgr_common.int_en0);
    pipe_mgr_ilist_add_register_write(dev, 0, address, pgr_data);
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set for GFM (Galois Field Matrix) */
static pipe_status_t pipe_mgr_tof_gfm_interrupt_mode_set(bf_dev_id_t dev,
                                                         bool enable) {
  uint32_t address = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  uint32_t gfm_data = 0;
  dev_stage_t stage = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  LOG_TRACE(" Setting GFM Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting GFM Interrupt mode to %s", enable ? "Enable" : "Disable");
  if (enable) {
    int idx = 0;
    /* Bits 0-7 - intr_gfm_parity_error_ingress
       Bit 8-15 - intr_gfm_parity_error_egress
    */
    for (idx = 0; idx < PIPE_MGR_INTR_GFM_NUM_ECC_ERR; idx++) {
      gfm_data |= 0x1u << idx;
    }
  }
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      address = offsetof(
          Tofino, pipes[phy_pipe].mau[stage].dp.intr_enable0_mau_gfm_hash);
      pipe_mgr_ilist_add_register_write(dev, 0, address, gfm_data);
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof_sbc_interrupt_mode_set(bf_dev_id_t dev,
                                                         bool enable) {
  uint32_t addr_cbc = offsetof(Tofino, device_select.cbc.cbc_cbus.int_en_0);
  uint32_t addr_mbc = offsetof(Tofino, device_select.mbc.mbc_mbus.int_en_0);
  uint32_t addr_pbc_2 = offsetof(Tofino, device_select.pbc.pbc_pbus.int_en2_0);
  uint32_t addr_pbc_3 = offsetof(Tofino, device_select.pbc.pbc_pbus.int_en3_0);

  uint32_t cbc_data = 0x73600;
  uint32_t mbc_data = 0xEA0;
  uint32_t pbc_data_2 = 0x1FF;
  uint32_t pbc_data_3 = (0x7 << 26) | 0x1FFF;
  uint32_t data = 0;
  LOG_TRACE(" Setting SBC Interrupt mode to %s ",
            enable ? "Enable" : "Disable");
  LOG_TRACE(" Setting SBC Interrupt mode to %s", enable ? "Enable" : "Disable");
  lld_subdev_read_register(dev, 0, addr_cbc, &data);
  if (enable) {
    data |= cbc_data;
  } else {
    data &= ~cbc_data;
  }
  lld_write_register(dev, addr_cbc, data);

  if (enable) {
    lld_write_register(dev, addr_mbc, mbc_data);
    lld_write_register(dev, addr_pbc_2, pbc_data_2);
    lld_write_register(dev, addr_pbc_3, pbc_data_3);
  } else {
    lld_write_register(dev, addr_mbc, 0);
    lld_write_register(dev, addr_pbc_2, 0);
    lld_write_register(dev, addr_pbc_3, 0);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof_lfltr_interrupt_mode_set(bf_dev_id_t dev,
                                                           bool enable) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  uint32_t pipe_step = offsetof(Tofino, device_select.lfltr1) -
                       offsetof(Tofino, device_select.lfltr0);
  uint32_t data = enable ? 0x3FFFFC : 0;

  LOG_TRACE("Setting learn filter Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_status_t rc =
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    if (PIPE_SUCCESS != rc) return rc;

    uint32_t addr = offsetof(Tofino, device_select.lfltr0.ctrl.int_en0);
    addr += phy_pipe * pipe_step;
    lld_write_register(dev, addr, data);
  }
  return PIPE_SUCCESS;
}

/* Interrupt mode set helper */
pipe_status_t pipe_mgr_tof_interrupt_en_set_helper(rmt_dev_info_t *dev_info,
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

  status |= pipe_mgr_tof_mau_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_mirror_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_tm_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_parser_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_deparser_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_pgr_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_gfm_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_sbc_interrupt_mode_set(dev, enable);
  status |= pipe_mgr_tof_lfltr_interrupt_mode_set(dev, enable);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to set interrupt mode (%d)", status);
  }

  /* Push the ilist if this is not bf-driver init and app has called the API. */
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
                  "%s:%d Error issuing ilist read to dev %d log pipe %d "
                  "stage %d TCAM unit mem_id %d %s",
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

pipe_status_t pipe_mgr_tof_tcam_read(bf_dev_id_t dev) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

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

  /* We need to read both word0 memory and word1 memory in both ingress and
   * egress parser TCAMs.  Get the four base addresses to use as well as the
   * address steps to go between pipes and parser tcams. */
  uint64_t pipe_step = pipe_top_level_pipes_array_element_size >> 4;
  /* Note the parser step is the same for both ingress and egress. */
  uint64_t prsr_step = pipe_top_level_pipes_i_prsr_array_element_size >> 4;
  uint64_t prsr_tcam_base_addrs[] = {
      pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address >> 4,
      pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address >> 4,
      pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address >> 4,
      pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address >> 4};
  char *prsr_tcam_base_names[] = {
      "ingress word0", "ingress word1", "egress word0", "egress word1"};
  int num_bases = sizeof prsr_tcam_base_addrs / sizeof prsr_tcam_base_addrs[0];

  /* For each pipe, scrub all the parser TCAMs */
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    bf_dev_pipe_t phy_pipe;
    status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    if (status != PIPE_SUCCESS) continue;

    /* Scrub the parser TCAMs. */
    for (int i = 0; i < num_bases; ++i) {
      for (int parser = 0; parser < TOF_NUM_PARSERS; ++parser) {
        uint64_t addr = prsr_tcam_base_addrs[i];
        addr += phy_pipe * pipe_step;
        addr += parser * prsr_step;
        status = pipe_mgr_drv_blk_rd(&shdl,
                                     dev,
                                     PIPE_MGR_TOF_TCAM_WORD_WIDTH,
                                     TOF_PARSER_DEPTH,
                                     1,
                                     addr,
                                     NULL,
                                     NULL);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s : Error scrubbing parser TCAM, dev %d phy-pipe %d %s "
              "parser %d error %s",
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
pipe_status_t pipe_mgr_tof_register_interrupt_notifs(rmt_dev_info_t *dev_info) {
  pipe_status_t status = PIPE_SUCCESS;

  /* Register for mau interrupt notifications */
  status |= pipe_mgr_tof_register_mau_interrupt_notifs(dev_info);

  /* Register for mirror interrupt notifications */
  status |= pipe_mgr_tof_register_mirror_interrupt_notifs(dev_info);

  /* Register for tm interrupt notifications */
  status |= pipe_mgr_tof_register_tm_interrupt_notifs(dev_info);

  /* Register for parser interrupt notifications */
  status |= pipe_mgr_tof_register_parser_interrupt_notifs(dev_info);

  /* Register for Egress buffer interrupt notifications */
  status |= pipe_mgr_tof_register_ebuf_interrupt_notifs(dev_info);

  /* Register for ig_deparser interrupt notifications */
  status |= pipe_mgr_tof_register_ig_deparser_interrupt_notifs(dev_info);

  /* Register for pgr interrupt notifications */
  status |= pipe_mgr_tof_register_pgr_interrupt_notifs(dev_info);

  /* Register for gfm interrupt notifications */
  status |= pipe_mgr_tof_register_gfm_interrupt_notifs(dev_info);

  /* Register for SBC interrupt notifications */
  status |= pipe_mgr_tof_register_sbc_interrupt_notifs(dev_info->dev_id);

  /* Register for Learn Filter interrupt notifications */
  status |= pipe_mgr_tof_register_lfltr_interrupt_notifs(dev_info->dev_id);

  return status;
}
