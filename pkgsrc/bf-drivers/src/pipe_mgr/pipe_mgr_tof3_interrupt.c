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
#include "pipe_mgr_tof3_interrupt.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_db.h"
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include "pipe_mgr_idle.h"
#include "pipe_mgr_learn.h"
#include "pipe_mgr_phy_mem_map.h"
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <traffic_mgr/traffic_mgr_ecc.h>
#include <mc_mgr/mc_mgr_pipe_intf.h>
#include "pipe_mgr_pktgen.h"

#define PIPE_MGR_TOF3_INTR_MAX_SYNTH2PORT_BITS 2
#define PIPE_MGR_TOF3_INTR_MIRR_NUM_ERR0 8
#define PIPE_MGR_TOF3_INTR_MIRR_NUM_ERR1 6
#define PIPE_MGR_TOF3_INTR_PRSR_NUM_ECC_ERR 20
#define PIPE_MGR_TOF3_INTR_TM_PRE_NUM_ERR 31
#define PIPE_MGR_TOF3_INTR_PRE_MBE_START 11
#define PIPE_MGR_TOF3_INTR_PRE_SBE_START 21
#define PIPE_MGR_TOF3_INTR_TM_WAC_NUM_ERR 17
#define PIPE_MGR_TOF3_INTR_MIRR_S2P_NUM_ERR 6

extern pipe_mgr_ctx_t *pipe_mgr_ctx;
/* TODO TOFINO3 */
#if 0
static uint32_t pipe_mgr_tof3_mirror_s2p_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                    uint32_t intr_address,
                                                    uint32_t intr_status_val,
                                                    uint32_t enable_hi_addr,
                                                    uint32_t enable_lo_addr,
                                                    void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t entry;

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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF3_INTR_MIRR_S2P_NUM_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT,
                     "Mirror output push error");
          LOG_TRACE("Mirror output push error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 1:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_OUT,
                     "Mirror output pop error");
          LOG_TRACE("Mirror output pop error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.mirreg.mirror.s2p_regs.s2p_session_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_s2p_regs_s2p_session_sbe_err_log_addr(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              entry,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_MIRROR,
              BF_ERR_LOC_MIRR_SESSION,
              "Mirror s2p session table single bit ecc error in session %d",
              (entry & 0xff));
          LOG_TRACE("Mirror s2p session table single bit ecc in session %d \n",
                    (data & 0x1ff));
          /*configure again*/
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;
        case 3:
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.mirreg.mirror.s2p_regs.s2p_session_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_s2p_regs_s2p_session_mbe_err_log_addr(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_NON_CORRECTABLE,
              dev,
              pipe,
              0,
              entry,
              BF_ERR_TYPE_MULTI_BIT_ECC,
              BF_ERR_BLK_MIRROR,
              BF_ERR_LOC_MIRR_SESSION,
              "Mirror s2p session table multi bit ecc error in session %d",
              (entry & 0xff));
          LOG_TRACE(
              "Mirror s2p session table multi bit ecc error in session %d \n",
              (data & 0x1ff));
          /*configure again*/
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;
        case 4:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_S2P_CREDIT,
                     "Mirror s2p credit overflow error");
          LOG_TRACE("Mirror s2p credit overflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 5:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_S2P_CREDIT,
                     "Mirror s2p credit underflow error");
          LOG_TRACE("Mirror s2p credit underflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof3_mirror_slice_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                      uint32_t intr_address,
                                                      uint32_t intr_status_val,
                                                      uint32_t enable_hi_addr,
                                                      uint32_t enable_lo_addr,
                                                      void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0, entry;
  int slice;
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);
  slice = userdata_p->row;
  LOG_TRACE(
      "Mirror ecc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < PIPE_MGR_TOF3_INTR_MIRR_NUM_ERR0; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_IDPRSR_SOPEOP_MISMATCH,
                     "Mirror idprsr input sop eop mismatch error");
          LOG_TRACE("Mirror idprsr input sop eop mismatch error");
          break;
        case 1:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_EDPRSR_SOPEOP_MISMATCH,
                     "Mirror edprsr input sop eop mismatch error");
          LOG_TRACE("Mirror edprsr input sop eop mismatch error");
          break;
        case 2:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .session_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry =
              getp_tof3_mirror_slice_regs_session_mem_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_SESSION,
                     "Mirror session table single bit ecc error in session %d",
                     (entry & 0xff));
          LOG_TRACE("Mirror session table single bit ecc in session %d \n",
                    (data & 0x1ff));
          /*configure again*/
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;
        case 3:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .session_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry =
              getp_tof3_mirror_slice_regs_session_mem_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_SESSION,
                     "Mirror session table multi bit ecc error in session %d",
                     (entry & 0xff));
          LOG_TRACE("Mirror session table multi bit ecc in session %d \n",
                    (data & 0x1ff));
          /*configure again*/
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;
        case 4:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .data_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_slice_regs_data_mem_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_MEM,
                     "Mirror data memory single bit error at addr 0x%x",
                     entry);
          LOG_TRACE("Mirror data memory single bit error at addr 0x%x", entry);
          break;
        case 5:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .data_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_slice_regs_data_mem_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_MEM,
                     "Mirror data memory multi bit error at addr 0x%x",
                     entry);
          LOG_TRACE("Mirror data memory multi bit error at addr 0x%x", entry);
          break;
        case 6:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .meta_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_slice_regs_meta_mem_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_MEM,
                     "Mirror meta data memory single bit error at addr 0x%x",
                     entry);
          LOG_TRACE("Mirror meta data memory single bit error at addr 0x%x",
                    entry);
          break;
        case 7:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .meta_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof3_mirror_slice_regs_meta_mem_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     entry,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_MIRROR,
                     BF_ERR_LOC_MIRR_DATA_MEM,
                     "Mirror meta data memory multi bit error at addr 0x%x",
                     entry);
          LOG_TRACE("Mirror meta data memory multi bit error at addr 0x%x",
                    entry);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, phy_pipe/BF_SUBDEV_PIPE_COUNT, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in tof3 deparser */
static uint32_t pipe_mgr_tof3_deparser_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                  uint32_t intr_address,
                                                  uint32_t intr_status_val,
                                                  uint32_t enable_hi_addr,
                                                  uint32_t enable_lo_addr,
                                                  void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, slice = 0, dir = 0;
  uint32_t err_addr;
  uint32_t diff_addr = offsetof(tof3_reg,
                                pipes[phy_pipe]
                                    .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                    .her.h.mirrtbl_sbe_err_log) -
                       offsetof(tof3_reg,
                                pipes[phy_pipe]
                                    .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                    .hir.h.mirrtbl_sbe_err_log);

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  dir = userdata_p->stage;
  slice = userdata_p->row;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);
  diff_addr = diff_addr * dir;
  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 17; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.mirrtbl_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xf;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_MIRRTBL,
                     "Deparser mirror tbl single bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser mirror tbl single bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_intr_mirr_tbl_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe, err_addr, dir);
          break;
        case 1:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.mirrtbl_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xf;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_MIRRTBL,
                     "Deparser mirror tbl multi bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser mirror tbl multi bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_intr_mirr_tbl_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe, err_addr, dir);
          break;
        case 2:
        case 4:
        case 6:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.ipkt_mac0_sbe_err_log) +
                    diff_addr + ((bitpos - 2) * 0x4);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_IPKT_MAC,
                     "Deparser ipkt mac%d single bit error at"
                     " addr %d, %s",
                     (bitpos - 2) / 2,
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser ipkt mac%d single bit err at addr %d, %s",
                    (bitpos - 2) / 2,
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 3:
        case 5:
        case 7:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.ipkt_mac0_mbe_err_log) +
                    diff_addr + ((bitpos - 3) * 0x4);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_IPKT_MAC,
                     "Deparser ipkt mac%d multi bit error at"
                     " addr %d, %s",
                     (bitpos - 3) / 2,
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser ipkt mac%d multi bit err at addr %d, %s",
                    (bitpos - 3) / 2,
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 8:
        case 9:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     bitpos == 8 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_CMD_FIFO,
                     "Deparser cmd fifo %s, %s",
                     (bitpos == 8 ? "overrun" : "underrun"),
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser cmd fifo %s, %s",
                    (bitpos == 8 ? "overrun" : "underrun"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 10:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_CRED_ERR,
                     "Deparser credit err, %s",
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser credit err, %s",
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     ((bitpos - 11) % 2 == 0) ? BF_ERR_TYPE_OVERFLOW
                                              : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_IPKT_MAC,
                     "Deparser ipkt mac%d %s, %s",
                     (bitpos - 11) / 2,
                     ((bitpos - 11) % 2 == 0) ? "overflow" : "underflow",
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser ipkt mac%d %s, %s",
                    (bitpos - 11) / 2,
                    ((bitpos - 11) % 2 == 0) ? "overflow" : "underflow",
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof3_deparser_err_handle1(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                   uint32_t intr_address,
                                                   uint32_t intr_status_val,
                                                   uint32_t enable_hi_addr,
                                                   uint32_t enable_lo_addr,
                                                   void *userdata) {
 // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, dir = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  dir = userdata_p->stage;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);
  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 23; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
        case 1:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     bitpos == 0 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTST,
                     "Deparser pktst %s, %s",
                     (bitpos == 0 ? "overflow" : "underflow"),
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pktst %s, %s",
                    (bitpos == 0 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:
        case 3:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     bitpos == 2 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_META_FIFO,
                     "Deparser metafifo %s, %s",
                     (bitpos == 2 ? "overflow" : "underflow"),
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser metafifo %s, %s",
                    (bitpos == 2 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:
        case 5:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     bitpos == 4 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTHDR,
                     "Deparser pkt header fifo %s, %s",
                     (bitpos == 4 ? "overflow" : "underflow"),
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt header fifo %s, %s",
                    (bitpos == 4 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:
        case 7:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     bitpos == 6 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_MIRRHDR,
                     "Deparser mirr header fifo %s, %s",
                     (bitpos == 6 ? "overflow" : "underflow"),
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser mirr header fifo %s, %s",
                    (bitpos == 6 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:
        case 9:
        case 12:
        case 13:
        case 16:
        case 17:
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              0,
              bitpos % 2 == 0 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
              BF_ERR_BLK_DEPRSR,
              BF_ERR_LOC_DEPRSR_DATAST,
              "Deparser datast%d fifo %s, %s",
              (bitpos - 8) / 4,
              (bitpos == 4 ? "overflow" : "underflow"),
              (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser datast%d fifo %s, %s",
                    (bitpos - 8) / 4,
                    (bitpos == 4 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 10:
        case 11:
        case 14:
        case 15:
        case 18:
        case 19:
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              0,
              bitpos % 2 == 0 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
              BF_ERR_BLK_DEPRSR,
              BF_ERR_LOC_DEPRSR_PKTDATA,
              "Deparser pkt data fifo%d %s, %s",
              (bitpos - 10) / 4,
              (bitpos == 4 ? "overflow" : "underflow"),
              (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt data fifo%d %s, %s",
                    (bitpos - 10) / 4,
                    (bitpos == 4 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 20:
        case 21:
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              0,
              bitpos == 20 ? BF_ERR_TYPE_OVERFLOW : BF_ERR_TYPE_UNDERFLOW,
              BF_ERR_BLK_DEPRSR,
              BF_ERR_LOC_DEPRSR_ARB_FIFO,
              "Deparser arb fifo %s, %s",
              (bitpos == 20 ? "overflow" : "underflow"),
              (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser arb fifo %s, %s",
                    (bitpos == 20 ? "overflow" : "underflow"),
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 22:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_CTL_CHAN,
                     "Deparser ctrl channel err, %s",
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser ctrl channel err, %s",
                    (dir == 0) ? "ingress" : "egress");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_deparser_err_handle2(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                   uint32_t intr_address,
                                                   uint32_t intr_status_val,
                                                   uint32_t enable_hi_addr,
                                                   uint32_t enable_lo_addr,
                                                   void *userdata) {
 // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, slice = 0, dir = 0;
  uint32_t err_addr;
  uint32_t diff_addr = offsetof(tof3_reg,
                                pipes[phy_pipe]
                                    .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                    .out_egr.meta_sbe_err_log) -
                       offsetof(tof3_reg,
                                pipes[phy_pipe]
                                    .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                    .out_ingr.meta_sbe_err_log);

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  dir = userdata_p->stage;
  slice = userdata_p->row;
  pipe_mgr_map_pipe_id_log_to_phy(pipe_mgr_get_dev_info(dev), pipe, &phy_pipe);
  diff_addr = diff_addr * dir;
  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 10; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.meta_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_META_FIFO,
                     "Deparser meta fifo single bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser meta fifo single bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 1:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.meta_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_META_FIFO,
                     "Deparser meta fifo multi bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser meta fifo multi bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 2:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pkthdr_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTHDR,
                     "Deparser pkt header single bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt header single bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 3:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pkthdr_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTHDR,
                     "Deparser pkt header multi bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt header multi bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 4:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.mirrhdr_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_MIRRHDR,
                     "Deparser mirr header single bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser mirr header single bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 5:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.mirrhdr_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_MIRRHDR,
                     "Deparser mirr header multi bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser mirr header multi bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 6:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pktdata_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0x1ff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTDATA,
                     "Deparser pkt data single bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt data single bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 7:
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pktdata_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0x1ff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_PKTDATA,
                     "Deparser pkt data multi bit error at"
                     " addr %d, %s",
                     err_addr,
                     (dir == 0) ? "ingress" : "egress");
          LOG_TRACE("Deparser pkt data multi bit err at addr %d, %s",
                    err_addr,
                    (dir == 0) ? "ingress" : "egress");
          break;
        case 8:  // only egress
          if (dir == 0) break;
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                 .out_egr.pktdata_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_TMSCH,
                     "Deparser TM sch fifo single bit error at"
                     " addr %d, egress",
                     err_addr);
          LOG_TRACE("Deparser TM sch fifo single bit err at addr %d, egress",
                    err_addr);
          break;
        case 9:  // only egress
          if (dir == 0) break;
          address = offsetof(tof3_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                 .out_egr.pktdata_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_DEPRSR,
                     BF_ERR_LOC_DEPRSR_TMSCH,
                     "Deparser TM sch fifo multi bit error at"
                     " addr %d, egress",
                     err_addr);
          LOG_TRACE("Deparser TM sch fifo multi bit err at addr %d, egress",
                    err_addr);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pgr_err_handle0(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;
  (void)enable_hi_addr;

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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // pfc0_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_mbe_log_1_2_pfc0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc0 multi bit err at addr %d ", mem_err_addr);
          break;
        case 1:  // pfc0_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_sbe_log_1_2_pfc0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc0 single bit err at addr %d ", mem_err_addr);
          break;
        case 2:  // pfc1_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_mbe_log_1_2_pfc1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc1 multi bit err at addr %d ", mem_err_addr);
          break;
        case 3:  // pfc1_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_sbe_log_1_2_pfc1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc1 single bit err at addr %d ", mem_err_addr);
          break;
        case 4:  // tbc_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_tbc_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 5:  // tbc_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_tbc_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 6:  // tbc_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_tbc_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 7:  // tbc_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_tbc_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 8:  // eth_cpu_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_eth_cpu_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 9:  // eth_cpu_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_eth_cpu_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 10:  // eth_cpu_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_eth_cpu_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 11:  // eth_cpu_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_eth_cpu_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 12:  // ebuf_p0_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_ebuf_p0_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 13:  // ebuf_p0_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_ebuf_p0_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 14:  // ebuf_p0_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_ebuf_p0_fifo0_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 15:  // ebuf_p0_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_ebuf_p0_fifo0_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 16:  // ebuf_p1_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = data & 0x7;
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr << 2) | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 17:  // ebuf_p1_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p1_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 18:  // ebuf_p1_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = data & 0x7;
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr << 2) | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 19:  // ebuf_p1_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p1_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 20:  // ebuf_p2_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p2_fifo0_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 21:  // ebuf_p2_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p2_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 22:  // ebuf_p2_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p2_fifo0_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 23:  // ebuf_p2_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p2_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 24:  // ebuf_p3_fifo0_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p3_fifo0_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 25:  // ebuf_p3_fifo0_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p3_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 26:  // ebuf_p3_fifo0_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p3_fifo0_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 27:  // ebuf_p3_fifo0_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p3_fifo0_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 28:  // tbc_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_tbc_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 29:  // tbc_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_tbc_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 30:  // tbc_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_tbc_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 31:  // tbc_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_tbc_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pgr_err_handle1(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;
  (void)enable_hi_addr;

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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // eth_cpu_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_eth_cpu_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 1:  // eth_cpu_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_eth_cpu_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 2:  // eth_cpu_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_eth_cpu_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 3:  // eth_cpu_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_eth_cpu_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 4:  // ebuf_p0_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_ebuf_p0_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p0 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 5:  // ebuf_p0_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_ebuf_p0_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p0 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 6:  // ebuf_p0_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_ebuf_p0_fifo1_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 7:  // ebuf_p0_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_ebuf_p0_fifo1_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 8:  // ebuf_p1_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr) | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p1 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 9:  // ebuf_p1_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p1_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p1 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 10:  // ebuf_p1_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr) | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 11:  // ebuf_p1_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p1_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 12:  // ebuf_p2_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p2_fifo1_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p2 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 13:  // ebuf_p2_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p2_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p2 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 14:  // ebuf_p2_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p2_fifo1_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 15:  // ebuf_p2_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p2_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 16:  // ebuf_p3_fifo1_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p3_fifo1_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 17:  // ebuf_p3_fifo1_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p3_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 18:  // ebuf_p3_fifo1_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p3_fifo1_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 19:  // ebuf_p3_fifo1_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p3_fifo1_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 20:  // tbc_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_tbc_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 21:  // tbc_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_tbc_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 22:  // tbc_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_tbc_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 23:  // tbc_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_tbc_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 24:  // eth_cpu_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_eth_cpu_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 25:  // eth_cpu_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_eth_cpu_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 26:  // eth_cpu_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_eth_cpu_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 27:  // eth_cpu_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_eth_cpu_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 28:  // ebuf_p0_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_ebuf_p0_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 29:  // ebuf_p0_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_ebuf_p0_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 30:  // ebuf_p0_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_ebuf_p0_fifo2_mem0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 31:  // ebuf_p0_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_ebuf_p0_fifo2_mem1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pgr_err_handle2(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;
  (void)enable_hi_addr;

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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // ebuf_p1_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = mem_err_addr | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 1:  // ebuf_p1_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p1_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 2:  // ebuf_p1_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (data & 0x7) << 2;
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = mem_err_addr | ((data >> 30) & 0x3);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 3:  // ebuf_p1_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p1_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 4:  // ebuf_p2_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p2_fifo2_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 5:  // ebuf_p2_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p2_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 6:  // ebuf_p2_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p2_fifo2_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 7:  // ebuf_p2_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p2_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 8:  // ebuf_p3_fifo2_mem0_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p3_fifo2_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 9:  // ebuf_p3_fifo2_mem1_mbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p3_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          break;
        case 10:  // ebuf_p3_fifo2_mem0_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p3_fifo2_mem0_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 11:  // ebuf_p3_fifo2_mem1_sbe
          address =
              offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof3_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p3_fifo2_mem1_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          break;
        case 12:  // buffer0_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_buffer0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer0 multi bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 13:  // buffer0_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_buffer0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer0 single bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 14:  // buffer1_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_buffer1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer1 multi bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 15:  // buffer1_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_buffer1_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer1 single bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 16:  // buffer2_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_buffer2_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer2 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer2 multi bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 17:  // buffer2_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_buffer2_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer2 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer2 single bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 18:  // buffer3_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_buffer3_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer3 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer3 multi bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 19:  // buffer3_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_buffer3_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer3 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer3 single bit err at addr %d ", mem_err_addr);
          pipe_mgr_pktgen_buffer_write_from_shadow(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe);
          break;
        case 20:  // phase0_mbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_mbe_log_mbe_log_1_2_phase0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PHASE0,
                     "PGR (pkt-gen) phase0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR phase0 multi bit err at addr %d ", mem_err_addr);
          // FIXME pipe_mgr_pkt_buffer_write_from_shadow
          break;
        case 21:  // phase0_sbe
          address = offsetof(
              tof3_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof3_pgr_sbe_log_sbe_log_1_2_phase0_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PHASE0,
                     "PGR (pkt-gen) phase0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR phase0 single bit err at addr %d ", mem_err_addr);
          // FIXME pipe_mgr_pkt_buffer_write_from_shadow
          break;
        case 22:  // app_evt_ovf0
        case 23:  // app_evt_ovf1
        case 24:  // app_evt_ovf2
        case 25:  // app_evt_ovf3
        case 26:  // app_evt_ovf4
        case 27:  // app_evt_ovf5
        case 28:  // app_evt_ovf6
        case 29:  // app_evt_ovf7
        case 30:  // app_evt_ovf8
        case 31:  // app_evt_ovf9
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_APP_EVT,
                     "PGR (pkt-gen) app event fifo overflow error%d",
                     (bitpos - 22));
          LOG_TRACE("PGR (pkt-gen) app event fifo overflow error%d",
                    (bitpos - 22));
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pgr_err_handle3(bf_dev_id_t dev, bf_subdev_id_t subdev_id, 
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  (void)enable_hi_addr;

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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // app_evt_ovf10
        case 1:  // app_evt_ovf11
        case 2:  // app_evt_ovf12
        case 3:  // app_evt_ovf13
        case 4:  // app_evt_ovf14
        case 5:  // app_evt_ovf15
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_APP_EVT,
                     "PGR (pkt-gen) app event fifo overflow error%d",
                     (bitpos + 10));
          LOG_TRACE("PGR (pkt-gen) app event fifo overflow error%d",
                    (bitpos + 10));
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:  // pfc_evt_ovf
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc event fifo overflow error");
          LOG_TRACE("PGR (pkt-gen) pfc event fifo overflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 7:  // ipb_chnl_seq_wrong
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_IPB_CHNL_SEQ,
                     "PGR (pkt-gen) ipb chnl sequence wrong");
          LOG_TRACE("PGR (pkt-gen) ipb chnl sequence wrong");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:  // eth_cpu_samechnl
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL,
                     "PGR (pkt-gen) eth cpu same chnl, configuration error");
          LOG_TRACE("PGR (pkt-gen) eth cpu same chnl, configuration error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 9:  // evt_tbc_samechnl
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL,
                     "PGR (pkt-gen) event tbc same chnl, configuration error");
          LOG_TRACE("PGR (pkt-gen) event tbc same chnl, configuration error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 10:  // tbc_fifo_ovf
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo overflow error");
          LOG_TRACE("PGR (pkt-gen) tbc fifo overflow error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 11:  // eth_cpu_ch0_fifo_ovf
        case 12:  // eth_cpu_ch1_fifo_ovf
        case 13:  // eth_cpu_ch2_fifo_ovf
        case 14:  // eth_cpu_ch3_fifo_ovf
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu chnl%d fifo overflow error",
                     (bitpos - 11));
          LOG_TRACE("PGR (pkt-gen) eth cpu chnl%d fifo overflow error",
                    (bitpos - 11));
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 15:  // ebuf_port0_ch0_fifo_ovf
        case 16:  // ebuf_port0_ch1_fifo_ovf
        case 17:  // ebuf_port1_ch0_fifo_ovf
        case 18:  // ebuf_port1_ch1_fifo_ovf
        case 19:  // ebuf_port2_ch0_fifo_ovf
        case 20:  // ebuf_port2_ch1_fifo_ovf
        case 21:  // ebuf_port3_ch0_fifo_ovf
        case 22:  // ebuf_port3_ch1_fifo_ovf
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_PORT_FIFO,
                     "PGR (pkt-gen) ebuf port%d chnl%d fifo overflow error",
                     (bitpos - 15) / 2,
                     (bitpos - 15) % 2);
          LOG_TRACE("PGR (pkt-gen) ebuf port%d chnl%d fifo overflow error",
                    (bitpos - 15) / 2,
                    (bitpos - 15) % 2);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof3_intr_mau_cfg_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                  uint32_t intr_address,
                                                  uint32_t intr_status_val,
                                                  uint32_t enable_hi_addr,
                                                  uint32_t enable_lo_addr,
                                                  void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_xpb_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                             uint32_t intr_address,
                                             uint32_t intr_status_val,
                                             uint32_t enable_hi_addr,
                                             uint32_t enable_lo_addr,
                                             void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_parde_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_ebuf_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_lfltr_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_mbc_intr_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_tbc0_intr_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}
static uint32_t pipe_mgr_tof3_tbc2_intr_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  lld_subdev_write_register(dev, subdev_id, intr_address, intr_status_val);
  return 0;
}

/* Handle sram ecc error */
static uint32_t pipe_mgr_tof3_ecc_sram_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;
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
  pipe_mem_type_t mem_type = 0;
  rmt_tbl_type_t tbl_type = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;

  (void)enable_hi_addr;
  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  mem_type = pipe_mem_type_unit_ram;
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;
  /* Determine row */
  row = userdata_p->row;
  rc = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  if (PIPE_SUCCESS != rc) return rc;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

  LOG_TRACE(
      "Sram ecc intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      stage,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  rc = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != rc) return rc;

  /* Check for single bit errors: bits 0 -11
     Check for multi  bit errors: bits 12:23
  */
  int num_cols = dev_info->dev_cfg.stage_cfg.num_sram_cols;
  for (int sbe_or_mbe = 0; sbe_or_mbe <= num_cols; sbe_or_mbe += num_cols) {
    bool is_sbe = sbe_or_mbe == 0;
    bf_error_sev_level_t sev =
        is_sbe ? BF_ERR_SEV_CORRECTABLE : BF_ERR_SEV_NON_CORRECTABLE;
    for (col = 0; col < num_cols; col++) {
      mem_id_t mem_id = 0;
      pipe_tbl_hdl_t tbl_hdl = 0;
      uint32_t cur_status_bit = 1u << (col + sbe_or_mbe);
      if (!(intr_status_val & cur_status_bit)) continue;
      /* Read the line within the memory on which the error occured from either
       * the SBE or MBE error log register. */
      if (is_sbe) {
        uint32_t data = 0;
        uint32_t address = offsetof(tof3_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_sbe_errlog);
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof3_unit_ram_sbe_errlog_unit_ram_sbe_errlog(&data);
      } else {
        uint32_t data = 0;
        uint32_t address = offsetof(tof3_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.array.row[row]
                                        .ram[col]
                                        .unit_ram_mbe_errlog);
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof3_unit_ram_mbe_errlog_unit_ram_mbe_errlog(&data);
      }
      mem_offset &= 0x3FF;

      /* Conver the row and column to a unit id and from that build the full
       * address of the memory. */
      mem_id = dev_info->dev_cfg.mem_id_from_col_row(col, row, mem_type);
      uint64_t phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          phy_pipe, stage, mem_id, mem_offset, mem_type);

      /* Look up the table which owns the memory.  Just take the first owner, we
       * really only care about the owner to distinguish between s2p tables and
       * normal tables. */
      pipe_mgr_get_mem_id_to_tbl_hdl_mapping(
          dev, pipe, stage, mem_id, mem_type, &tbl_hdl, &tbl_type);

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
        lld_subdev_ind_write(dev, subdev, phy_addr, 0, 0);
        BF_ERR_EVT(
            sev,
            dev,
            pipe,
            stage,
            phy_addr,
            is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
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
        pipe_mgr_intr_sram_tcam_ecc_correct(sess_hdl, dev, tbl_hdl, phy_addr);
        BF_ERR_EVT(
            sev,
            dev,
            pipe,
            stage,
            phy_addr,
            is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
            BF_ERR_BLK_SRAM,
            BF_ERR_LOC_NONE,
            "Sram ecc error at row %d, col %d, line %d, physical addr "
            "0x%" PRIx64,
            row,
            col,
            mem_offset,
            phy_addr);
      } else if (is_sbe) {
        rc = s2p_ram_sbe_correct(
            sess_hdl, dev, tbl_hdl, pipe, stage, mem_offset);
        if (PIPE_SUCCESS != rc) {
          /* For whatever reason we didn't fix it, but HW should correct this on
           * its own eventually (or faster than software can if there is
           * traffic) in a S2P table so don't upgrade the severity.
           * Note that one possible reason why we didn't fix it is because the
           * table is currently reserved by a batch or txn. */
        }
        BF_ERR_EVT(
            sev,
            dev,
            pipe,
            stage,
            phy_addr,
            is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
            BF_ERR_BLK_SRAM,
            BF_ERR_LOC_NONE,
            "Sram ecc error at row %d, col %d, line %d, physical addr "
            "0x%" PRIx64,
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
        BF_ERR_EVT(
            BF_ERR_SEV_FATAL,
            dev,
            pipe,
            stage,
            phy_addr,
            is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
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
        pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, cur_status_bit);
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return rc;
}

/* Handle map ram ecc error */
static uint32_t pipe_mgr_tof3_ecc_map_ram_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                 uint32_t intr_address,
                                                 uint32_t intr_status_val,
                                                 uint32_t enable_hi_addr,
                                                 uint32_t enable_lo_addr,
                                                 void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

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
  pipe_mgr_intr_userdata_t *userdata_p = NULL;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  (void)enable_hi_addr;
  (void)enable_lo_addr;

  pipe_mem_type_t mem_type = pipe_mem_type_map_ram;
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
  if (PIPE_SUCCESS != status) return status;

  /* Check for single bit errors: bits 0 -5
     Check for multi  bit errors: bits 6 - 11
  */
  int num_cols = dev_info->dev_cfg.stage_cfg.num_map_ram_cols;
  for (int sbe_or_mbe = 0; sbe_or_mbe <= num_cols; sbe_or_mbe += num_cols) {
    bool is_sbe = sbe_or_mbe == 0;
    for (col = 0; col < num_cols; col++) {
      mem_id_t mem_id = 0, unit_ram_mem_id = 0;
      uint32_t cur_status_bit = 1u << (col + sbe_or_mbe);
      if (!(intr_status_val & cur_status_bit)) continue;
      /* Read the line within the memory on which the error occured from either
       * the SBE or MBE error log register. */
      if (is_sbe) {
        uint32_t data = 0;
        uint32_t address = offsetof(tof3_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_sbe_errlog[col]);
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof3_mapram_sbe_errlog_mapram_sbe_errlog(&data);
      } else {
        uint32_t data = 0;
        uint32_t address = offsetof(tof3_reg,
                                    pipes[phy_pipe]
                                        .mau[stage]
                                        .rams.map_alu.row[row]
                                        .adrmux.mapram_mbe_errlog[col]);
        pipe_mgr_interrupt_read_register(dev, address, &data);
        mem_offset = getp_tof3_mapram_mbe_errlog_mapram_mbe_errlog(&data);
      }
      mem_offset &= 0x3FF;

      /* Convert the row and col to a map ram unit id. */
      mem_id = dev_info->dev_cfg.mem_id_from_col_row(col, row, mem_type);
      unit_ram_mem_id = dev_info->dev_cfg.unit_ram_from_map_ram(mem_id);
      uint64_t phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          phy_pipe, stage, mem_id, mem_offset, mem_type);

      rmt_tbl_type_t owner_type =
          pipe_mgr_map_ram_type_get(dev, pipe, stage, mem_id);
      /* Handle idle time map rams specially. */
      if (owner_type == RMT_TBL_TYPE_IDLE_TMO) {
        /* Only need to fix multibit and parity errors */
        if (!is_sbe) {
          /* Refresh the idle entries in the map ram row */
          tbl_hdl = PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id);
          /* For parity and multi-bit errors, we must first do a physical write
           * of the entire line to reset the error, then use virtual writes to
           * reset the entries.
           */
          pipe_mgr_idle_phy_write_line(dev, tbl_hdl, stage, mem_id, mem_offset);
          pipe_mgr_intr_map_ram_idle_ecc_correct(pipe_mgr_ctx->int_ses_hndl,
                                                 dev,
                                                 tbl_hdl,
                                                 pipe,
                                                 stage,
                                                 mem_id,
                                                 mem_offset);
        }
        BF_ERR_EVT(
            BF_ERR_SEV_CORRECTABLE,
            dev,
            pipe,
            stage,
            phy_addr,
            is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
            BF_ERR_BLK_MAP_RAM,
            BF_ERR_LOC_NONE,
            "mapRAM ecc error fixed at physical addr 0x%" PRIx64,
            phy_addr);
      } else {
        tbl_hdl = PIPE_INTR_TBL_HDL(dev, pipe, stage, unit_ram_mem_id, 0);
        if (tbl_hdl == 0 || !tbl_is_s2p(tbl_hdl)) {
          /* This is unexpected...  The map RAM isn't owned, just write it to
           * zero. */
          LOG_ERROR("Dev %d pipe %d stage %d mapRAM %d %cBE but unused",
                    dev,
                    pipe,
                    stage,
                    mem_id,
                    sbe_or_mbe ? 'M' : 'S');
          lld_subdev_ind_write(dev, subdev, phy_addr, 0, 0);
          BF_ERR_EVT(
              sbe_or_mbe ? BF_ERR_SEV_NON_CORRECTABLE : BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              stage,
              phy_addr,
              is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
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
                dev, enable_hi_addr, cur_status_bit);
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       stage,
                       phy_addr,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
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
            BF_ERR_EVT(
                BF_ERR_SEV_FATAL,
                dev,
                pipe,
                stage,
                phy_addr,
                is_sbe ? BF_ERR_TYPE_SINGLE_BIT_ECC : BF_ERR_TYPE_MULTI_BIT_ECC,
                BF_ERR_BLK_MAP_RAM,
                BF_ERR_LOC_NONE,
                "Uncorrectable S2P ECC error, stage %d mapRAM %d line %d",
                stage,
                mem_id,
                mem_offset);
            /* We are not fixing the error so turn it off so it doesn't keep
             * firing as fast as we can clear it. */
            pipe_mgr_interrupt_set_enable_val(
                dev, enable_hi_addr, cur_status_bit);
          }
        }
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle meter stats ecc error */
static uint32_t pipe_mgr_tof3_intr_mau_adrdist_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                      uint32_t intr_address,
                                                      uint32_t intr_status_val,
                                                      uint32_t enable_hi_addr,
                                                      uint32_t enable_lo_addr,
                                                      void *userdata) {
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  dev_stage_t stage = 0;
  int row = 0;
  uint32_t mem_offset = 0;
  pipe_mem_type_t mem_type = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);

  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  // ***TBD Tf3-fix handle subdev_id 
  (void) subdev_id;

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
  if (PIPE_SUCCESS != status) return status;

  /* Check bits 0-9, 18-25 */
  /* Check for def stats sbe bits 0-3 */
  mem_type = pipe_mem_type_stats_deferred_access_ram;
  for (row = 0; row < PIPE_MGR_INTR_MAX_STATS; row++) {
    if (intr_status_val & (0x1u << row)) {
      address =
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .mau[stage]
                       .rams.match.adrdist.deferred_stats_parity_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset =
          getp_tof3_deferred_stats_parity_errlog_def_sbe_errlog_addr(&data);
      /* meters Synth2port errors cannot be fixed, just log */
      BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 dev_info->dev_cfg.get_full_phy_addr(
                     phy_pipe,
                     stage,
                     dev_info->dev_cfg.mem_id_from_col_row(0, row, mem_type),
                     mem_offset,
                     mem_type),
                 BF_ERR_TYPE_SINGLE_BIT_ECC,
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
    }
  }
  mem_type = pipe_mem_type_meter_deferred_access_ram;
  /* Check for meters sbe: bits 4 - 7 */
  for (row = 0; row < PIPE_MGR_INTR_MAX_METERS; row++) {
    if (intr_status_val & (0x1u << (row + 4))) {
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.match.adrdist.def_meter_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 8 bit mem_offset */
      mem_offset = getp_tof3_def_meter_sbe_errlog_def_sbe_errlog_addr(&data);
      BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 dev_info->dev_cfg.get_full_phy_addr(
                     phy_pipe,
                     stage,
                     dev_info->dev_cfg.mem_id_from_col_row(0, row, mem_type),
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
    }
  }
  /* idletime slip */
  if (intr_status_val & (0x1u << 8)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_IDLETIME,
               "MAU idletime slip error");
    LOG_TRACE("MAU idletime slip error");
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << 8);
  }
  /* meter sweep */
  if (intr_status_val & (0x1u << 9)) {
    address = offsetof(
        tof3_reg,
        pipes[phy_pipe].mau[stage].rams.match.adrdist.meter_sweep_errlog);
    pipe_mgr_interrupt_read_register(dev, address, &data);
    /* 4 bit mem_offset */
    mem_offset = getp_tof3_meter_sweep_errlog_meter_sweep_errlog(&data);
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
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << 9);
  }
  /* stateful log overflow */
  if (intr_status_val & 0x3c0000) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_STATEFUL_LOG,
               "MAU stateful log fifo overflow error");
    LOG_TRACE("MAU stateful log fifo overflow error, 0x%x",
              (intr_status_val >> 18) & 0xf);
    pipe_mgr_interrupt_set_enable_val(
        dev, enable_hi_addr, (intr_status_val & 0x3c0000));
  }
  /* stateful log underflow */
  if (intr_status_val & 0x3c00000) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               stage,
               0,
               BF_ERR_TYPE_UNDERFLOW,
               BF_ERR_BLK_MAU,
               BF_ERR_LOC_MAU_STATEFUL_LOG,
               "MAU stateful log fifo underflow error");
    LOG_TRACE("MAU stateful log fifo underflow error, 0x%x",
              (intr_status_val >> 22) & 0xf);
    pipe_mgr_interrupt_set_enable_val(
        dev, enable_hi_addr, (intr_status_val & 0x3c00000));
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in synth2port */
static uint32_t pipe_mgr_tof3_synth2port_mem_err_handle(
    bf_dev_id_t dev, bf_subdev_id_t subdev_id,
    uint32_t intr_address,
    uint32_t intr_status_val,
    uint32_t enable_hi_addr,
    uint32_t enable_lo_addr,
    void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

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
  int row = 0, col = 0, col_max;
  uint32_t mem_offset = 0;
  pipe_mem_type_t mem_type = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);

  (void)enable_lo_addr;
  col_max = PIPE_MGR_TOF3_INTR_MAX_SYNTH2PORT_BITS;
  if (dev >= PIPE_MGR_NUM_DEVICES) {
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
  if (PIPE_SUCCESS != status) return status;

  mem_type = pipe_mem_type_map_ram;
  for (col = 0; col < col_max; col++) {
    if (intr_status_val & (0x1u << col)) {
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.map_alu.row[row]
                             .i2portctl.mau_synth2port_errlog);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 16 bit mem_offset */
      mem_offset = getp_tof3_mau_synth2port_errlog_mau_synth2port_errlog(&data);
      /* Synth2port errors cannot be fixed, just log */
      BF_ERR_EVT(BF_ERR_SEV_FATAL,
                 dev,
                 pipe,
                 stage,
                 dev_info->dev_cfg.get_full_phy_addr(
                     phy_pipe,
                     stage,
                     dev_info->dev_cfg.mem_id_from_col_row(col, row, mem_type),
                     mem_offset,
                     mem_type),
                 BF_ERR_TYPE_GENERIC,
                 BF_ERR_BLK_SYNTH2PORT,
                 BF_ERR_LOC_NONE,
                 "Synth2port error in (row %d, col %d, off %d)",
                 row,
                 col,
                 mem_offset);
      LOG_TRACE(
          "Synth2port error in pipe %d, stage %d at (row %d, col %d,off %d)",
          pipe,
          stage,
          row,
          col,
          mem_offset);
      pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << col);
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle error in mau sel alu */
static uint32_t pipe_mgr_tof3_sel_alu_mem_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                     uint32_t intr_address,
                                                     uint32_t intr_status_val,
                                                     uint32_t enable_hi_addr,
                                                     uint32_t enable_lo_addr,
                                                     void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

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
  pipe_mgr_intr_userdata_t *userdata_p = NULL;

  (void)enable_lo_addr;

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
  if (PIPE_SUCCESS != status) return status;

  for (col = 0; col < PIPE_MGR_INTR_MAX_SEL_ALU_DIR; col++) {
    /* col 0 - ingress, col 1 - egress */
    if (intr_status_val & (0x1u << col)) {
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .rams.map_alu.meter_group[row]
                             .selector.mau_selector_alu_errlog);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 16 bit mem_offset */
      mem_offset =
          getp_tof3_mau_selector_alu_errlog_mau_selector_alu_errlog(&data);
      /*
        Nothing to be done here, the issue should clear up once software updates
        HW will correct set of ports
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
                 col ? "Ingress" : "Egress",
                 mem_offset);
      LOG_TRACE(
          "Sel alu error at pipe %d, stage %d, (row %d, direction %s,addr "
          "0x%x)",
          pipe,
          stage,
          row,
          col ? "Ingress" : "Egress",
          mem_offset);
      pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << col);
    }
  }
  if (intr_status_val & (0x1u << 2)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_ST_MINMAX,
               "Selector ALU stateful min/max error");
    LOG_TRACE("Selector ALU stateful min/max error");
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << 2);
  }
  if (intr_status_val & (0x1u << 3)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_DEV_BY0,
               "Selector ALU stateful devide by 0 error");
    LOG_TRACE("Selector ALU stateful devide by 0 error");
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << 3);
  }
  if (intr_status_val & (0x1u << 4)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev,
               pipe,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_SELECTOR_ALU,
               BF_ERR_LOC_SELECTOR_ALU_SALU_PRED,
               "Selector ALU stateful prediction error");
    LOG_TRACE("Selector ALU stateful prediction error");
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u << 4);
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle stats error */
static uint32_t pipe_mgr_tof3_stats_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t log_pipe = 0;
  dev_stage_t stage = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  int alu_idx = 0;
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  (void)enable_hi_addr;
  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
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

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  if (intr_status_val & (0x1u)) {  // LRT evict FIFO ovf
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
    pipe_mgr_interrupt_set_enable_val(dev, enable_hi_addr, 0x1u);
  }
  if (intr_status_val & (0x1u << 1)) {  // stats entry overflow
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
  }
  if (intr_status_val & (0x1u << 2)) {  // max_value clear pending
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
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle imem error */
static uint32_t pipe_mgr_tof3_imem_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

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
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t mem_offset = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);

  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  (void)enable_hi_addr;
  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
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
  if (PIPE_SUCCESS != status) return status;

  address = offsetof(tof3_reg, pipes[phy_pipe].mau[stage].dp.imem_sbe_errlog);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  /* 14 bit mem_offset */
  mem_offset = getp_tof3_imem_sbe_errlog_imem_sbe_errlog(&data);
  /* intr_mem_cnt */
  intr_imem_cnt = PIPE_MGR_TOF3_IMEM_COUNT;
  mem_offset &= 0x3FFF;
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
                 col ? "Ingress" : "Egress");
      LOG_TRACE("imem error in pipe %d stage %d and %s direction, value 0x%x ",
                log_pipe,
                stage,
                col ? "Ingress" : "Egress",
                data);
    }
  }

  int bwr_size = pipe_mgr_drv_buf_size(dev, PIPE_MGR_DRV_BUF_BWR);
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
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
    pcie_addr =
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].base_addr;
    data_ptr = PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data;
    data_len = PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data_len;
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
      if (PIPE_SUCCESS != status) {
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
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Handle tcam ecc error */
static uint32_t pipe_mgr_tof3_ecc_tcam_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

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
  pipe_mem_type_t mem_type = pipe_mem_type_tcam;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;

  (void)enable_hi_addr;
  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
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
  if (PIPE_SUCCESS != status) return status;

  /* bits 4-15 for single bit error */
  /* bits 0-3 tcam logical channel error */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_tcam_rows; row++) {
    if (intr_status_val & (0x1u << (4 + row))) {
      mem_id_t mem_id = 0;
      pipe_tbl_hdl_t tbl_hdl = 0;
      int index = 0;
      address = offsetof(tof3_reg,
                         pipes[phy_pipe].mau[stage].tcams.tcam_sbe_errlog[row]);
      pipe_mgr_interrupt_read_register(dev, address, &data);
      /* 9 bit line no, 1 bit column and 1 bit subword */
      data = getp_tof3_tcam_sbe_errlog_tcam_sbe_errlog_addr(&data);
      subword = data & 0x1;
      mem_offset = (data >> 1) & 0x1ff;
      col = (data >> 10) & 0x1;
      mem_id = dev_info->dev_cfg.mem_id_from_col_row(col, row, mem_type);
      phy_addr = dev_info->dev_cfg.get_full_phy_addr(
          phy_pipe, stage, mem_id, mem_offset, mem_type);

      BF_ERR_EVT(
          BF_ERR_SEV_NON_CORRECTABLE,
          dev,
          pipe,
          stage,
          phy_addr,
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
          "Tcam ecc error at pipe %d, stage %d (row %d, col %d, line"
          " 0x%x, subword %d), phy address 0x%" PRIx64 "",
          pipe,
          stage,
          row,
          col,
          mem_offset,
          subword,
          phy_addr);
      for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
        tbl_hdl = PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index);
        if ((tbl_hdl == 0) ||
            (PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) !=
             mem_type)) {
          continue;
        }
        /* Rewrite the data at the problem address */
        pipe_mgr_intr_sram_tcam_ecc_correct(
            pipe_mgr_ctx->int_ses_hndl, dev, tbl_hdl, phy_addr);
      }
      /* If address is not used by any table, fix the memory directly */
      if (index == PIPE_MGR_MAX_HDL_PER_MEM_ID) {
        pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr);
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Pre error */
static uint32_t pipe_mgr_tof3_tm_pre_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id, 
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
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
  (void)enable_lo_addr;

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
  if (PIPE_SUCCESS != status) return status;

  /* Read the mbe errlog reg value */
  address =
      offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[phy_pipe].mbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  mbe_ram_type = getp_tof3_pre_mbe_log_ram(&data);
  mbe_addr = getp_tof3_pre_mbe_log_addr(&data);
  /* Read the sbe errlog reg value */
  address =
      offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[phy_pipe].sbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  sbe_ram_type = getp_tof3_pre_sbe_log_ram(&data);
  sbe_addr = getp_tof3_pre_sbe_log_addr(&data);
  for (bitpos = 0; bitpos < PIPE_MGR_INTR_TM_PRE_NUM_ERR; bitpos++) {
    /* Bits 14-31 are ecc errors */
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case (PIPE_INTR_PRE_RAM_FIFO +
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // fifo_mbe
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
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case (PIPE_INTR_PRE_RAM_MIT +
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // mit_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // lit0_bm_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // lit1_bm_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // lit0_np_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // lit1_np_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // pmt0_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // pmt1_mbe
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
              PIPE_MGR_TOF3_INTR_PRE_MBE_START):  // rdm_mbe
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
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case (PIPE_INTR_PRE_RAM_FIFO +
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // fifo_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // mit_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // lit0_bm_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // lit1_bm_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // lit0_np_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // lit1_np_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // pmt0_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // pmt1_sbe
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
              PIPE_MGR_TOF3_INTR_PRE_SBE_START):  // rdm_sbe
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
        case (PIPE_INTR_PRE_RAM_FIFO_BANKID +
              PIPE_MGR_TOF3_INTR_PRE_MBE_START): /* fifo_mem_bankid_mbe */
          if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_FIFO_BANKID,
                                             mbe_ram_type)) {
            BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       mbe_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_BANKID_MEM,
                       "TM PRE BANKID MEM multi bit error at addr 0x%x",
                       mbe_addr);
          }
          break;
        case (PIPE_INTR_PRE_RAM_FIFO_BANKID +
              PIPE_MGR_TOF3_INTR_PRE_SBE_START): /* fifo_mem_bankid_sbe */
          if (PIPE_MGR_INTR_PRE_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_FIFO_BANKID,
                                             sbe_ram_type)) {
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       sbe_addr,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_BANKID_MEM,
                       "TM PRE BANKID MEM single bit error at addr 0x%x",
                       sbe_addr);
          }
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Wac error */
static uint32_t pipe_mgr_tof3_tm_wac_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Wac error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe].wac_reg.intr.stat
  for (bitpos = 0; bitpos < PIPE_MGR_TOF3_INTR_TM_WAC_NUM_ERR; bitpos++) {
    /* bits 0-9 15-16 are ecc errors */
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // ppg_mapping_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.ppg_mapping_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_reg_ppg_mapping_table_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PPG_MAP,
                     "TM WAC PPG MAPPING single bit error at addr 0x%x",
                     err_mem_addr);
          bf_tm_ecc_correct_wac_ppg_map(dev, phy_pipe, err_mem_addr);
          break;
        case 1:  // ppg_mapping_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.ppg_mapping_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_reg_ppg_mapping_table_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PPG_MAP,
                     "TM WAC PPG MAPPING multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:  // drop_cnt_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_reg_drop_cnt_table_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_DROP_CNT,
                     "TM WAC Drop Counter single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 3:  // drop_cnt_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_reg_drop_cnt_table_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_DROP_CNT,
                     "TM WAC Drop Counter multi bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 4:  // pfc_vis_sbe
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PFC_VIS,
                     "TM WAC PFC VIS single bit error");
          break;
        case 5:  // pfc_vis_mbe
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PFC_VIS,
                     "TM WAC PFC VIS multi bit error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:  // sch_fcr_sbe
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_SCH_FCR,
                     "TM WAC SCH FCR single bit error");
          break;
        case 7:  // sch_fcr_mbe
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_SCH_FCR,
                     "TM WAC SCH FCR multi bit error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:  // qid_map_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.qid_map_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof3_pipe_reg_qid_map_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_QID_MAP,
                     "TM WAC QID MAPPING single bit error at addr 0x%x",
                     err_mem_addr);
          bf_tm_ecc_correct_wac_qid_map(dev, phy_pipe, err_mem_addr);
          break;
        case 9:  // qid_map_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.qid_map_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof3_pipe_reg_qid_map_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_QID_MAP,
                     "TM WAC QID MAPPING multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 15:  // wac2qac merr
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_WAC2QAC,
                     "TM WAC to QAC multi bit error");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 16:  // wac2qac serr
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_WAC2QAC,
                     "TM WAC to QAC single bit error");
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Qac error */
static uint32_t pipe_mgr_tof3_tm_qac_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Qac error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-9
  // device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe].qac_reg.intr.stat
  for (bitpos = 0; bitpos < 10; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // queue_drop_cnt_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.queue_drop_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_queue_drop_table_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QUE_DROP,
                     "TM QAC QUEUE DROP single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 1:  // queue_drop_cnt_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.queue_drop_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_queue_drop_table_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QUE_DROP,
                     "TM QAC QUEUE DROP multi bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 2:  // port_drop_cnt_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.port_drop_cnt_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_port_drop_cnt_table_sbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_PORT_DROP,
                     "TM QAC PORT DROP single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 3:  // port_drop_cnt_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.port_drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_port_drop_cnt_table_mbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_PORT_DROP,
                     "TM QAC PORT DROP multi bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 4:  // qid_mapping_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qid_mapping_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_qid_mapping_table_sbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QID_MAP,
                     "TM QAC QID MAPPING single bit error at addr 0x%x",
                     err_mem_addr);
          bf_tm_ecc_correct_qac_qid_map(dev, phy_pipe, err_mem_addr);
          break;
        case 5:  // qid_mapping_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qid_mapping_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_qid_mapping_table_mbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QID_MAP,
                     "TM QAC QID MAPPING multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:  // qac2prc_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qac2prc_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_qac2prc_fifo_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QAC2PRC,
                     "TM QAC QAC to PRC single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 7:  // qac2prc_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qac2prc_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_qac2prc_fifo_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_QAC2PRC,
                     "TM QAC QAC to PRC multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:  // prc2psc_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.prc2psc_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_prc2psc_fifo_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_PRC2PSC,
                     "TM QAC PRC to PSC single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 9:  // prc2psc_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.prc2psc_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_pipe_block_reg_prc2psc_fifo_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QAC,
                     BF_ERR_LOC_TM_QAC_PRC2PSC,
                     "TM QAC PRC to PSC multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Clc error */
static uint32_t pipe_mgr_tof3_tm_clc_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr, err_mem_inst, err_mem_odd, err_mem_ep;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Clc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-17
  // device_select.tm_top.tm_clc_top.clc[phy_pipe].intr.stat
  for (bitpos = 0; bitpos < 18; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
        case 1:
        case 2:
        case 3:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_TM_CLC,
                     BF_ERR_LOC_TM_CLC_PH_FIFO,
                     "TM CLC PH EP%d FIFO Full error",
                     bitpos);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_TM_CLC,
                     BF_ERR_LOC_TM_CLC_QAC_PH_FIFO,
                     "TM CLC QAC PH EP%d FIFO Full error",
                     (bitpos - 4));
          break;
        case 8:
        case 9:
        case 10:
        case 11:  // enq fifo sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_clc_top.clc[phy_pipe]
                                 .enfifo_serr_ep0_log) +
                    (4 * (bitpos - 8));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_clc_pipe_rspec_enfifo_serr_ep0_log_addr(&data);
          err_mem_inst =
              getp_tof3_tm_clc_pipe_rspec_enfifo_serr_ep0_log_inst(&data);
          err_mem_odd =
              getp_tof3_tm_clc_pipe_rspec_enfifo_serr_ep0_log_odd(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_CLC,
                     BF_ERR_LOC_TM_CLC_ENQ_FIFO,
                     "TM CLC ENQ%d FIFO single bit error at addr 0x%x, inst "
                     "0x%x, odd %d",
                     (bitpos - 8),
                     err_mem_addr,
                     err_mem_inst,
                     err_mem_odd);
          break;
        case 12:
        case 13:
        case 14:
        case 15:  // enq fifo mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_clc_top.clc[phy_pipe]
                                 .enfifo_merr_ep0_log) +
                    (4 * (bitpos - 12));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_clc_pipe_rspec_enfifo_merr_ep0_log_addr(&data);
          err_mem_inst =
              getp_tof3_tm_clc_pipe_rspec_enfifo_merr_ep0_log_inst(&data);
          err_mem_odd =
              getp_tof3_tm_clc_pipe_rspec_enfifo_merr_ep0_log_odd(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_CLC,
                     BF_ERR_LOC_TM_CLC_ENQ_FIFO,
                     "TM CLC ENQ%d FIFO multi bit error at addr 0x%x, inst "
                     "0x%x, odd %d",
                     (bitpos - 12),
                     err_mem_addr,
                     err_mem_inst,
                     err_mem_odd);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 16:  // clc qac fifo sbe
          address = offsetof(
              tof3_reg,
              device_select.tm_top.tm_clc_top.clc[phy_pipe].clc_qac_serr_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_clc_pipe_rspec_clc_qac_serr_log_addr(&data);
          err_mem_ep = getp_tof3_tm_clc_pipe_rspec_clc_qac_serr_log_ep(&data);
          err_mem_odd = getp_tof3_tm_clc_pipe_rspec_clc_qac_serr_log_odd(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_CLC,
                     BF_ERR_LOC_TM_CLC_QAC_FIFO,
                     "TM CLC QAC FIFO single bit error at addr 0x%x, ep "
                     "0x%x, odd %d",
                     err_mem_addr,
                     err_mem_ep,
                     err_mem_odd);
          break;
        case 17:  // clc qac fifo mbe
          address = offsetof(
              tof3_reg,
              device_select.tm_top.tm_clc_top.clc[phy_pipe].clc_qac_merr_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_clc_pipe_rspec_clc_qac_merr_log_addr(&data);
          err_mem_ep = getp_tof3_tm_clc_pipe_rspec_clc_qac_merr_log_ep(&data);
          err_mem_odd = getp_tof3_tm_clc_pipe_rspec_clc_qac_merr_log_odd(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_MULTI_BIT_ECC,
              BF_ERR_BLK_TM_CLC,
              BF_ERR_LOC_TM_CLC_QAC_FIFO,
              "TM CLC QAC FIFO multi bit error at addr 0x%x, ep 0x%x, odd %d",
              err_mem_addr,
              err_mem_ep,
              err_mem_odd);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Pex error */
static uint32_t pipe_mgr_tof3_tm_pex_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id, 
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Pex error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-11
  // device_select.tm_top.tm_pex_top.pex[phy_pipe].intr.stat
  for (bitpos = 0; bitpos < 12; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // clm mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_linkmem_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_CLM,
                     "TM PEX CLM multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 1:  // clm sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_linkmem_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_CLM,
                     "TM PEX CLM single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 2:  // dq ph fifo sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_ph_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_dq_ph_fifo_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_PH_FIFO,
                     "TM PEX PH FIFO single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 3:  // dq ph fifo mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_ph_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_dq_ph_fifo_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_PH_FIFO,
                     "TM PEX PH FIFO multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:  // dq meta fifo sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_meta_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_dq_meta_fifo_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_META_FIFO,
                     "TM PEX META FIFO single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 5:  // dq meta fifo mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_meta_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_dq_meta_fifo_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_META_FIFO,
                     "TM PEX META FIFO multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:  // ph afifo sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .ph_afifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_ph_afifo_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_PH_AFIFO,
                     "TM PEX PH AFIFO single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 7:  // ph afifo mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .ph_afifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_ph_afifo_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_PH_AFIFO,
                     "TM PEX PH AFIFO multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:   // ph discard fifo sbe
        case 10:  // ph discard fifo1 sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .discard_ph_fifo0_sbe_err_log) +
                    (4 * (bitpos - 8));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_discard_ph_fifo0_sbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_DISCARD_FIFO,
                     "TM PEX PH DISCARD FIFO%d single bit error at addr 0x%x",
                     (bitpos - 8) / 2,
                     err_mem_addr);
          break;
        case 9:   // ph discard fifo mbe
        case 11:  // ph discard fifo1 mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .discard_ph_fifo0_mbe_err_log) +
                    (4 * (bitpos - 9));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_pex_pipe_rspec_discard_ph_fifo0_mbe_err_log_addr(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PEX,
                     BF_ERR_LOC_TM_PEX_DISCARD_FIFO,
                     "TM PEX PH DISCARD FIFO%d multi bit error at addr 0x%x",
                     (bitpos - 9) / 2,
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Qlc error */
static uint32_t pipe_mgr_tof3_tm_qlc_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Qlc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-3
  // device_select.tm_top.tm_qlc_top.qlc[phy_pipe].intr.stat
  for (bitpos = 0; bitpos < 4; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qlc_top.qlc[phy_pipe]
                                 .linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_qlc_pipe_rspec_linkmem_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_QLC,
                     BF_ERR_LOC_TM_QLC_QLM,
                     "TM QLC QLM single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 1:
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_qlc_top.qlc[phy_pipe]
                                 .linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_qlc_pipe_rspec_linkmem_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_QLC,
                     BF_ERR_LOC_TM_QLC_QLM,
                     "TM QLC QLM multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:
          address = offsetof(
              tof3_reg,
              device_select.tm_top.tm_qlc_top.qlc[phy_pipe].schdeq_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof3_tm_qlc_pipe_rspec_schdeq_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_TM_QLC,
                     BF_ERR_LOC_TM_QLC_SCHDEQ,
                     "TM QLC SCH DEQ Pop Empty at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 3:
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_TM_QLC,
                     BF_ERR_LOC_TM_QLC_PH_FIFO,
                     "TM QLC Discard FIFO Overflow");
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Prc error */
static uint32_t pipe_mgr_tof3_tm_prc_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Prc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-1
  // device_select.tm_top.tm_prc_top.prc[phy_pipe].intr.stat
  for (bitpos = 0; bitpos < 2; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // t3 sbe
          address = offsetof(
              tof3_reg,
              device_select.tm_top.tm_prc_top.prc[phy_pipe].cache_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_prc_pipe_rspec_cache_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_PRC,
                     BF_ERR_LOC_TM_PRC_T3,
                     "TM PRC T3 Cache single bit error at addr 0x%x",
                     err_mem_addr);
          break;
        case 1:  // t3 mbe
          address = offsetof(
              tof3_reg,
              device_select.tm_top.tm_prc_top.prc[phy_pipe].cache_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_prc_pipe_rspec_cache_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PRC,
                     BF_ERR_LOC_TM_PRC_T3,
                     "TM PRC T3 Cache multi bit error at addr 0x%x",
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
/* TM Psc/psc common error */
static uint32_t pipe_mgr_tof3_tm_psc_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr, err_mem_bankid, err_mem_blkid;
  bool is_pcs;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  pipe = userdata_p->pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  is_pcs = userdata_p->row;
  LOG_TRACE("TM Psc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-1
  // device_select.tm_top.tm_psc_top.psc[phy_pipe].intr.stat
  // device_select.tm_top.tm_psc_top.psc_common.intr.stat
  if (is_pcs == 0) {  // pcs
    for (bitpos = 0; bitpos < 2; bitpos++) {
      if (intr_status_val & (0x1u << bitpos)) {
        switch (bitpos) {
          case 0:
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc[phy_pipe].psm_sbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof3_tm_psc_pipe_rspec_psm_sbe_err_log_addr(&data);
            err_mem_bankid =
                getp_tof3_tm_psc_pipe_rspec_psm_sbe_err_log_bankid(&data);
            err_mem_blkid =
                getp_tof3_tm_psc_pipe_rspec_psm_sbe_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       err_mem_addr,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_PSM,
                       "TM PSC PSM single bit error at addr 0x%x, bankid 0x%x, "
                       "blkid 0x%x",
                       err_mem_addr,
                       err_mem_bankid,
                       err_mem_blkid);
            break;
          case 1:
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc[phy_pipe].psm_mbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof3_tm_psc_pipe_rspec_psm_mbe_err_log_addr(&data);
            err_mem_bankid =
                getp_tof3_tm_psc_pipe_rspec_psm_mbe_err_log_bankid(&data);
            err_mem_blkid =
                getp_tof3_tm_psc_pipe_rspec_psm_mbe_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       err_mem_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_PSM,
                       "TM PSC PSM multi bit error at addr 0x%x, bankid 0x%x, "
                       "blkid 0x%x",
                       err_mem_addr,
                       err_mem_bankid,
                       err_mem_blkid);
            pipe_mgr_interrupt_set_enable_val(
                dev, enable_hi_addr, 0x1u << bitpos);
            break;
          default:
            break;
        }
      }
    }
  } else {  // pcs common
    for (bitpos = 0; bitpos < 5; bitpos++) {
      if (intr_status_val & (0x1u << bitpos)) {
        switch (bitpos) {
          case 1:
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc_common.linkmem_sbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof3_tm_psc_common_rspec_linkmem_sbe_err_log_addr(&data);
            err_mem_blkid =
                getp_tof3_tm_psc_common_rspec_linkmem_sbe_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                       dev,
                       pipe,
                       0,
                       err_mem_addr,
                       BF_ERR_TYPE_SINGLE_BIT_ECC,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_COMM,
                       "TM PSC COMM Linkmem single bit error at addr 0x%x, "
                       "blkid 0x%x",
                       err_mem_addr,
                       err_mem_blkid);
            break;
          case 2:
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc_common.linkmem_mbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof3_tm_psc_common_rspec_linkmem_mbe_err_log_addr(&data);
            err_mem_blkid =
                getp_tof3_tm_psc_common_rspec_linkmem_mbe_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       err_mem_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_COMM,
                       "TM PSC COMM Linkmem multi bit error at addr 0x%x, "
                       "blkid 0x%x",
                       err_mem_addr,
                       err_mem_blkid);
            pipe_mgr_interrupt_set_enable_val(
                dev, enable_hi_addr, 0x1u << bitpos);
            break;
          case 3:  // overflow
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc_common.overflow_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_blkid =
                getp_tof3_tm_psc_common_rspec_overflow_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       0,
                       BF_ERR_TYPE_OVERFLOW,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_COMM,
                       "TM PSC COMM Overflow error at blkid 0x%x",
                       err_mem_blkid);
            pipe_mgr_interrupt_set_enable_val(
                dev, enable_hi_addr, 0x1u << bitpos);
            break;
          case 4:  // underflow
            address = offsetof(
                tof3_reg,
                device_select.tm_top.tm_psc_top.psc_common.underflow_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_blkid =
                getp_tof3_tm_psc_common_rspec_underflow_err_log_blkid(&data);
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       0,
                       BF_ERR_TYPE_UNDERFLOW,
                       BF_ERR_BLK_TM_PSC,
                       BF_ERR_LOC_TM_PSC_COMM,
                       "TM PSC COMM Underflow error at blkid 0x%x",
                       err_mem_blkid);
            pipe_mgr_interrupt_set_enable_val(
                dev, enable_hi_addr, 0x1u << bitpos);
            break;
          default:
            break;
        }
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Caa error */
static uint32_t pipe_mgr_tof3_tm_caa_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr, err_mem_blkid;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;
  (void)userdata;
  LOG_TRACE("TM caa error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit1-4
  // device_select.tm_top.tm_caa_top.intr.stat
  for (bitpos = 1; bitpos < 5; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 1:
          address = offsetof(
              tof3_reg, device_select.tm_top.tm_caa_top.linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_caa_top_rspec_linkmem_sbe_err_log_addr(&data);
          err_mem_blkid =
              getp_tof3_tm_caa_top_rspec_linkmem_sbe_err_log_blkid(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     0,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_CAA,
                     BF_ERR_LOC_TM_CAA,
                     "TM CAA Linkmem single bit error at addr 0x%x, blkid 0x%x",
                     err_mem_addr,
                     err_mem_blkid);
          break;
        case 2:
          address = offsetof(
              tof3_reg, device_select.tm_top.tm_caa_top.linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_caa_top_rspec_linkmem_mbe_err_log_addr(&data);
          err_mem_blkid =
              getp_tof3_tm_caa_top_rspec_linkmem_mbe_err_log_blkid(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     0,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_CAA,
                     BF_ERR_LOC_TM_CAA,
                     "TM CAA Linkmem multi bit error at addr 0x%x, blkid 0x%x",
                     err_mem_addr,
                     err_mem_blkid);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 3:
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_caa_top.overflow_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_blkid =
              getp_tof3_tm_caa_top_rspec_overflow_err_log_blkid(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     0,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_TM_CAA,
                     BF_ERR_LOC_TM_CAA,
                     "TM CAA Overflow error at blkid 0x%x",
                     err_mem_blkid);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_caa_top.underflow_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_blkid =
              getp_tof3_tm_caa_top_rspec_underflow_err_log_blkid(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     0,
                     0,
                     0,
                     BF_ERR_TYPE_UNDERFLOW,
                     BF_ERR_BLK_TM_CAA,
                     BF_ERR_LOC_TM_CAA,
                     "TM CAA Underflow error at blkid 0x%x",
                     err_mem_blkid);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Scha error */
static uint32_t pipe_mgr_tof3_tm_sch_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t pipe;
  pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  int sch_numb;
  bool is_a;
  char ab;
  uint32_t base_addr;
  uint32_t err_mem_addr, err_sram, err_mem_type;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  (void)enable_lo_addr;

  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  /* Get pipe and stage from userdata */
  sch_numb = userdata_p->pipe;
  is_a = userdata_p->row;
  base_addr =
      offsetof(tof3_reg,
               device_select.tm_top.tm_schb_top.sch[sch_numb].intr.stat) -
      offsetof(tof3_reg,
               device_select.tm_top.tm_scha_top.sch[sch_numb].intr.stat);
  base_addr *= is_a;
  ab = (is_a == 0 ? 'A' : 'B');
  pipe = is_a * 2 + sch_numb;
  LOG_TRACE("TM Sch%c error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            ab,
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  // bit0-30
  // device_select.tm_top.tm_scha_top.sch[i].intr.stat
  // device_select.tm_top.tm_schb_top.sch[i].intr.stat
  for (bitpos = 0; bitpos < 30; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // tdm_table_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .tdm_table_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_tdm_table_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_TDM,
                     "TM SCH%c TDM single bit error at addr 0x%x",
                     ab,
                     err_mem_addr);
          break;
        case 1:  // tdm_table_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .tdm_table_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_tdm_table_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_TDM,
                     "TM SCH%c TDM multi bit error at addr 0x%x",
                     ab,
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:  // upd_wac_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_wac_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_wac_sbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_wac_sbe_err_log_sram(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_UPD_WAC,
              "TM SCH%c UPD WAC single bit error at addr 0x%x, sram 0x%x",
              ab,
              err_mem_addr,
              err_sram);
          break;
        case 3:  // upd_wac_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_wac_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_wac_mbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_wac_mbe_err_log_sram(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_WAC,
                     "TM SCH%c UPD WAC multi bit error at addr 0x%x, sram 0x%x",
                     ab,
                     err_mem_addr,
                     err_sram);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:  // upd_edprsr_advfc_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_edprsr_advfc_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_advfc_sbe_err_log_addr(
                  &data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_advfc_sbe_err_log_sram(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_EDPRSR_ADVFC,
                     "TM SCH%c UPD EDPRSR ADVFC single bit error at addr 0x%x, "
                     "sram 0x%x",
                     ab,
                     err_mem_addr,
                     err_sram);
          break;
        case 5:  // upd_edprsr_advfc_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_edprsr_advfc_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_advfc_mbe_err_log_addr(
                  &data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_advfc_mbe_err_log_sram(
                  &data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_EDPRSR_ADVFC,
                     "TM SCH%c UPD EDPRSR ADVFC multi bit error at addr 0x%x, "
                     "sram 0x%x",
                     ab,
                     err_mem_addr,
                     err_sram);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 6:  // q_minrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_Q_MINRATE,
              "TM SCH%c Q MINRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_q_minrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 7:  // q_minrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_sbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_MINRATE,
                     "TM SCH%c Q MINRATE multi bit error at addr 0x%x, type %d",
                     ab,
                     err_mem_addr,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:  // q_excrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_excrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_excrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_excrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_Q_EXCRATE,
              "TM SCH%c Q EXCRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_q_excrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 9:  // q_excrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_minrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_minrate_mbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_EXCRATE,
                     "TM SCH%c Q EXCRATE multi bit error at addr 0x%x, type %d",
                     ab,
                     err_mem_addr,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 10:  // q_maxrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_maxrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_Q_MAXRATE,
              "TM SCH%c Q MAXRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_q_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 11:  // q_maxrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_q_maxrate_mbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_MAXRATE,
                     "TM SCH%c Q MAXRATE multi bit error at addr 0x%x, type %d",
                     ab,
                     err_mem_addr,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 12:  // l1_minrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_minrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_MINRATE,
              "TM SCH%c L1 MINRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_l1_minrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 13:  // l1_minrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_minrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_minrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_minrate_mbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_MULTI_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_MINRATE,
              "TM SCH%c L1 MINRATE multi bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 14:  // l1_excrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_excrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_excrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_excrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_EXCRATE,
              "TM SCH%c L1 EXCRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_l1_excrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 15:  // l1_excrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_excrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_excrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_excrate_mbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_MULTI_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_EXCRATE,
              "TM SCH%c L1 EXCRATE multi bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 16:  // l1_maxrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_maxrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_MAXRATE,
              "TM SCH%c L1 MAXRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_l1_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 17:  // l1_maxrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .l1_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_l1_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_l1_maxrate_mbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_MULTI_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_L1_MAXRATE,
              "TM SCH%c L1 MAXRATE multi bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 18:  // p_maxrate_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .p_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_p_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_p_maxrate_sbe_err_log_type(&data);
          BF_ERR_EVT(
              BF_ERR_SEV_CORRECTABLE,
              dev,
              pipe,
              0,
              err_mem_addr,
              BF_ERR_TYPE_SINGLE_BIT_ECC,
              BF_ERR_BLK_TM_SCH,
              BF_ERR_LOC_TM_SCH_P_MAXRATE,
              "TM SCH%c P MAXRATE single bit error at addr 0x%x, type %d",
              ab,
              err_mem_addr,
              err_mem_type);
          bf_tm_ecc_correct_sch_p_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;
        case 19:  // p_maxrate_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .p_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_p_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_p_maxrate_mbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_P_MAXRATE,
                     "TM SCH%c P MAXRATE multi bit error at addr 0x%x, type %d",
                     ab,
                     err_mem_addr,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 20:  // upd_pex0_sbe
        case 22:  // upd_pex1_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_pex0_sbe_err_log) +
                    base_addr + (bitpos - 20) * 4;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_sram(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_PEX,
                     "TM SCH%c UPD PEX%d single bit error at addr 0x%x, "
                     "Bank%d, type 0x%d",
                     ab,
                     (bitpos - 20) / 2,
                     err_mem_addr,
                     err_sram,
                     err_mem_type);
          break;
        case 21:  // upd_pex0_mbe
        case 23:  // upd_pex1_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_pex0_mbe_err_log) +
                    base_addr + (bitpos - 21) * 4;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_sram(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_PEX,
                     "TM SCH%c UPD PEX%d multi bit error at addr 0x%x, Bank%d, "
                     "type 0x%d",
                     ab,
                     (bitpos - 21) / 2,
                     err_mem_addr,
                     err_sram,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 24:  // upd_edprsr_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_edprsr_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_sram(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_EDPRSR,
                     "TM SCH%c UPD EDPRSR single bit error at addr 0x%x, "
                     "Bank%d, type 0x%d",
                     ab,
                     err_mem_addr,
                     err_sram,
                     err_mem_type);
          break;
        case 25:  // upd_edprsr_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .upd_edprsr_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_addr(&data);
          err_sram =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_sram(&data);
          err_mem_type =
              getp_tof3_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_type(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_UPD_EDPRSR,
                     "TM SCH%c UPD EDPRSR multi bit error at addr 0x%x, "
                     "Bank%d, type 0x%d",
                     ab,
                     err_mem_addr,
                     err_sram,
                     err_mem_type);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 26:  // pex_credit_err
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .pex_credit_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_pex_credit_err_log_port(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_PEX_CREDIT,
                     "TM SCH%c PEX CREDIT error at port 0x%x",
                     ab,
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 27:  // pex_mac_credit_err
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .pex_mac_credit_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_pex_mac_credit_err_log_mac_grp(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_PEX_MAC_CREDIT,
                     "TM SCH%c PEX MAC CREDIT error at MAC group 0x%x",
                     ab,
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 28:  // q_watchdog_sbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_watchdog_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_watchdog_sbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_WATCHDOG,
                     "TM SCH%c Q WATCHDOG single bit error at addr 0x%x",
                     ab,
                     err_mem_addr);
          break;
        case 29:  // q_watchdog_mbe
          address = offsetof(tof3_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_numb]
                                 .q_watchdog_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof3_tm_sch_pipe_rspec_q_watchdog_mbe_err_log_addr(&data);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     err_mem_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_WATCHDOG,
                     "TM SCH%c Q WATCHDOG multi bit error at addr 0x%x",
                     ab,
                     err_mem_addr);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 30:  // q_watchdog
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_TM_SCH,
                     BF_ERR_LOC_TM_SCH_Q_WATCHDOG,
                     "TM SCH%c Q WATCHDOG error",
                     ab);
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Correct ecc error in po action ram in parser */
static pipe_status_t pipe_mgr_tof3_parser_action_ram_ecc_correct(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    int parser,
    int dir,
    uint32_t row,
    uint32_t blk_id,
    uint64_t *address) {
  (void)blk_id;
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

  if (row >= TOF3_PARSER_DEPTH) {
    LOG_ERROR("Dev %d log-pipe %d prsr %d dir %d action row %d doesn't exist",
              dev,
              pipe,
              parser,
              dir,
              row);
    return PIPE_INVALID_ARG;
  }
  data_size = PIPE_MGR_TOF3_PO_WORD_WIDTH;
  offset = row * (data_size / 16);
  status = pipe_mgr_get_prsr_instance_hdl(
      dev, pipe, dir, (parser / 4), &prsr_instance_hdl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Fail to get parser instance handler, dev %d, pipe %d, %s, parser "
        "%d, error %s",
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
  src_data = bin_cfg->tof3.po_action_data[row];
  *address = PIPE_PRSR_ADDR(dev, dir).tof3.po_action_addr +
             (tof3_mem_pipes_array_element_size * phy_pipe) +
             (PIPE_PRSR_ADDR(dev, dir).tof3.prsr_step * parser) + offset;
  st = pipe_mgr_drv_get_ses_state(
      &pipe_mgr_ctx->int_ses_hndl, __func__, __LINE__);

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
        "prsr "
        "%d",
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
static pipe_status_t pipe_mgr_tof3_parser_tcam_err_correct(bf_dev_id_t dev,
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

  if (row >= TOF3_PARSER_DEPTH) {
    LOG_ERROR("Dev %d log-pipe %d prsr %d dir %d row %d doesn't exist",
              dev,
              pipe,
              parser,
              dir,
              row);
    status = PIPE_INVALID_ARG;
    return status;
  }
  data_size = PIPE_MGR_TOF3_TCAM_WORD_WIDTH;
  address = PIPE_PRSR_ADDR(dev, dir).tof3.tcam_addr +
            (tof3_mem_pipes_array_element_size * phy_pipe) +
            (PIPE_PRSR_ADDR(dev, dir).tof3.prsr_step * parser) + row;
  src_data = PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe, parser / 4)
                 .tof3[dir]
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
  st = pipe_mgr_drv_get_ses_state(
      &pipe_mgr_ctx->int_ses_hndl, __func__, __LINE__);

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
static uint32_t pipe_mgr_tof3_parser_err_handle(bf_dev_id_t dev, bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  int parser = 0, ipb = 0;
  uint32_t err_addr = 0, blk_id = 0;
  int bitpos = 0, dir = 0, bit_max;
  uint64_t full_err_addr = 0;
  pipe_mgr_intr_userdata_t *userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  (void)enable_lo_addr;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  bit_max = PIPE_MGR_TOF3_INTR_PRSR_NUM_ECC_ERR;
  parser = userdata_p->row & 0x3;
  ipb = userdata_p->row >> 2;
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
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
  if (PIPE_SUCCESS != status) return status;

  for (bitpos = 0; bitpos < bit_max; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // no_tcam_match_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 1:  // partial_hdr_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 2:  // ctr_range_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 3:  // timeout_iter_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 4:  // timeout_cycle_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 5:  // src_ext_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 7:  // phv_owner_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 8:  // multi_wr_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 9:            // aram_sbe
          if (dir == 0) {  // ingress
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[ipb]
                                   .prsr[parser]
                                   .aram_sbe_err_log);
          } else {
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[ipb]
                                   .prsr[parser]
                                   .aram_sbe_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof3_prsr_reg_main_rspec_aram_sbe_err_log_addr(&data);
          blk_id = getp_tof3_prsr_reg_main_rspec_aram_sbe_err_log_blkid(&data);
          pipe_mgr_tof3_parser_action_ram_ecc_correct(
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
        case 10:           // aram_mbe
          if (dir == 0) {  // ingress
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[ipb]
                                   .prsr[parser]
                                   .aram_mbe_err_log);
          } else {
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[ipb]
                                   .prsr[parser]
                                   .aram_mbe_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof3_prsr_reg_main_rspec_aram_mbe_err_log_addr(&data);
          blk_id = getp_tof3_prsr_reg_main_rspec_aram_mbe_err_log_blkid(&data);
          pipe_mgr_tof3_parser_action_ram_ecc_correct(
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
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 12:  // csum_err
          pipe_mgr_interrupt_set_enable_val(
              dev, enable_hi_addr, 0x1u << bitpos);
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
              dev, enable_hi_addr, 0x1u << bitpos);
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
              dev, enable_hi_addr, 0x1u << bitpos);
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
              dev, enable_hi_addr, 0x1u << bitpos);
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
              dev, enable_hi_addr, 0x1u << bitpos);
          break;
        case 17:  // tcam_par_err
          /* Tcam memory will be refreshed periodically */
          if (dir == 0) {  // ingress
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[ipb]
                                   .prsr[parser]
                                   .tcam_par_err_log);
          } else {
            address = offsetof(tof3_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.epbprsr4reg[ipb]
                                   .prsr[parser]
                                   .tcam_par_err_log);
          }
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = getp_tof3_prsr_reg_main_rspec_tcam_par_err_log_row(&data);
          blk_id = getp_tof3_prsr_reg_main_rspec_tcam_par_err_log_word(&data);
          pipe_mgr_tof3_parser_tcam_err_correct(
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
        case 18: /* bit 18:csum_sbe */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_CSUM,
                     "Checksum RAM single bit error");
          break;
        case 19: /* bit 19:csum_mbe */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     full_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PRSR,
                     BF_ERR_LOC_PRSR_CSUM,
                     "Checksum RAM multi bit error");
          break;
        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof3_cbc0_intr_handle(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr;
  int bitpos = 0;
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  LOG_TRACE("CBC0 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;
  /* hw learning */
  if ((intr_status_val & TOFINO3_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY) &&
      pipe_mgr_flow_lrn_is_int_enabled(dev_id)) {
    // Pause the DMA from LF to DR
    for (int i = 0; i < 4; ++i) {
      uint32_t addr = offsetof(tof3_regs, device_select.pbc.pbc_pbus.arb_ctrl1[i]);
      lld_subdev_write_register(dev_id, subdev_id, addr, 0x01111);
    }

    // Read the contents of the DR
    lld_dr_service(dev_id, subdev_id, lld_dr_rx_learn, 10000);

    // Unpause the DMA from LF to DR
    for (int i = 0; i < 4; ++i) {
      uint32_t addr = offsetof(tof3_regs, device_select.pbc.pbc_pbus.arb_ctrl1[i]);
      lld_subdev_write_register(dev_id, subdev_id, addr, 0x11111);
    }
  }
  // host overflow
  if (intr_status_val & (0x1u)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_CBC,
               "CBC host overflow error");
    LOG_TRACE("CBC host overflow error");
    en_value |= 0x1u;
  }
  for (bitpos = 21; bitpos < 26; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 21:
        case 22:
          address = offsetof(
              tof3_reg,
              device_select.cbc.cbc_cbus.wl_tx_dr_rd_err_log[bitpos - 21]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_cbus_wl_tx_dr_rd_err_log_wl_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_cbus_wl_tx_dr_rd_err_log_wl_tx_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_CBC,
              "CBC WL tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              (bitpos - 21),
              err_status,
              head_ptr);
          LOG_TRACE(
              "CBC WL tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              (bitpos - 21),
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 23:
        case 24:
          address = offsetof(
              tof3_reg,
              device_select.cbc.cbc_cbus.rb_tx_dr_rd_err_log[bitpos - 23]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_cbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_cbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_CBC,
              "CBC RB tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              (bitpos - 23),
              err_status,
              head_ptr);
          LOG_TRACE(
              "CBC RB tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              (bitpos - 23),
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 25:
          address = offsetof(tof3_reg,
                             device_select.cbc.cbc_cbus.lq_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_cbus_lq_fm_dr_rd_err_log_lq_fm_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_cbus_lq_fm_dr_rd_err_log_lq_fm_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_CBC,
              "CBC LQ fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "CBC LQ fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, en_value);
  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof3_cbc1_intr_handle(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t que, que_addr, i;
  uint32_t parity_err_data[7];
  uint64_t err_addr64, err_data64[2];
  char name[7] = {'\n', '\n', '\n', '\n', '\n', '\n', '\n'};
  char *name_p = name;
  int bitpos = 0;
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  LOG_TRACE("CBC1 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;
  for (bitpos = 0; bitpos < 9; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
        case 1:
        case 5:
        case 6:
          name_p = "WL";
          break;
        case 2:
        case 3:
        case 7:
        case 8:
          name_p = "RB";
          break;
        case 4:
        case 9:
          name_p = "LQ";
          break;
        case 10:
        case 11:
        case 12:
          for (i = 0; i < 7; i++) {
            address = offsetof(tof3_reg,
                               device_select.cbc.cbc_cbus.parity_err_log[i]);
            pipe_mgr_interrupt_read_register(
                dev_id, address, (parity_err_data + i));
          }
          err_addr64 = ((parity_err_data[5] & 0x1fff) << 29) +
                       ((parity_err_data[4] >> 3) & 0x1fffffff);
          err_data64[0] =
              ((parity_err_data[0] >> 3) & 0x1fffffff) +
              (parity_err_data[1] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61);
          err_data64[1] =
              ((parity_err_data[2] >> 3) & 0x1fffffff) +
              (parity_err_data[3] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[4] & 0x7) << 61);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev_id,
                     0,
                     0,
                     0,
                     BF_ERR_TYPE_PARITY,
                     BF_ERR_BLK_DMA,
                     BF_ERR_LOC_DMA_CBC,
                     "CBC CBUS parity err: cycle %d, type %d, sid %d, qid %d, "
                     "tag %d, addr 0x%" PRIx64 ", data 0x%" PRIx64 "%016" PRIx64
                     ", parity %d",
                     (parity_err_data[5] & 0x3),
                     (parity_err_data[5] >> 27) & 0x3,
                     (parity_err_data[5] >> 23) & 0xf,
                     (parity_err_data[5] >> 21) & 0x3,
                     (parity_err_data[5] >> 13) & 0xff,
                     err_addr64,
                     err_data64[0],
                     err_data64[1],
                     (parity_err_data[0]) & 0x7);
          LOG_TRACE(
              "CBC CBUS parity err: cycle %d, type %d, sid %d, qid %d, tag "
              "%d, "
              "addr 0x%" PRIx64 ", data 0x%" PRIx64 "%016" PRIx64 ", parity %d",
              (parity_err_data[5] & 0x3),
              (parity_err_data[5] >> 27) & 0x3,
              (parity_err_data[5] >> 23) & 0xf,
              (parity_err_data[5] >> 21) & 0x3,
              (parity_err_data[5] >> 13) & 0xff,
              err_addr64,
              err_data64[0],
              err_data64[1],
              (parity_err_data[0]) & 0x7);
          en_value |= (0x1u << bitpos);
          break;
        default:
          break;
      }
      if (bitpos < 5) {  // mbe
        address =
            offsetof(tof3_reg, device_select.cbc.cbc_cbus.controller_mbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof3_cbus_controller_mbe_log_queue_addr(&data);
        que = getp_tof3_cbus_controller_mbe_log_queue(&data);
        BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                   dev_id,
                   0,
                   0,
                   que_addr,
                   BF_ERR_TYPE_MULTI_BIT_ECC,
                   BF_ERR_BLK_DMA,
                   BF_ERR_LOC_DMA_CBC,
                   "CBC %s queue multi bit ecc error at queue 0x%x, addr0x%x",
                   name_p,
                   que,
                   que_addr);
        LOG_TRACE("CBC %s queue multi bit ecc error at queue 0x%x, addr0x%x",
                  name_p,
                  que,
                  que_addr);
        en_value |= (0x1u << bitpos);
      } else if (bitpos < 10) {  // sbe
        address =
            offsetof(tof3_reg, device_select.cbc.cbc_cbus.controller_sbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof3_cbus_controller_sbe_log_queue_addr(&data);
        que = getp_tof3_cbus_controller_sbe_log_queue(&data);
        BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                   dev_id,
                   0,
                   0,
                   que_addr,
                   BF_ERR_TYPE_SINGLE_BIT_ECC,
                   BF_ERR_BLK_DMA,
                   BF_ERR_LOC_DMA_CBC,
                   "CBC %s queue single bit ecc error at queue 0x%x, addr0x%x",
                   name_p,
                   que,
                   que_addr);
        LOG_TRACE("CBC %s queue single bit ecc error at queue 0x%x, addr0x%x",
                  name_p,
                  que,
                  que_addr);
        en_value |= (0x1u << bitpos);
      }
    }
  }
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, en_value);
  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}
/*static uint32_t pipe_mgr_tof3_mbc_intr_handle(bf_dev_id_t dev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr, err_addr, i;
  uint32_t parity_err_data[4];
  uint64_t err_addr64, err_data64;
  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo
0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  LOG_TRACE("MBC error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
                dev_id,
                intr_address,
                intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;

  // host overflow
  if (intr_status_val & (0x1u)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC host overflow error");
    LOG_TRACE("MBC host overflow error");
    en_value |= 0x1u;
  }
  if (intr_status_val & (0x1u << 9)) {
    address =
        offsetof(tof3_reg, device_select.mbc.mbc_mbus.mac_0_tx_dr_rd_err_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_status =
        getp_tof3_mbus_mac_0_tx_dr_rd_err_log_mac_tx_dr_rd_err_status(&data);
    head_ptr =
        getp_tof3_mbus_mac_0_tx_dr_rd_err_log_mac_tx_dr_rd_err_head_ptr(&data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MAC 0 tx dr rd error, err_status %d, err_head_pointer
0x%x",
               err_status,
               head_ptr);
    LOG_TRACE("MBC MAC 0 tx dr rd error, err_status %d, err_head_pointer
0x%x",
              err_status,
              head_ptr);
    en_value |= (0x1u << 9);
  }
  if (intr_status_val & (0x1u << 10)) {
    address =
        offsetof(tof3_reg, device_select.mbc.mbc_mbus.wb_tx_dr_rd_err_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_status =
        getp_tof3_mbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_status(&data);
    head_ptr =
        getp_tof3_mbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_head_ptr(&data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC WB tx dr rd error, err_status %d, err_head_pointer 0x%x",
               err_status,
               head_ptr);
    LOG_TRACE("MBC WB tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
    en_value |= (0x1u << 10);
  }
  if (intr_status_val & (0x1u << 12)) {
    address = offsetof(tof3_reg,
device_select.mbc.mbc_mbus.controller_mbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof3_mbus_controller_mbe_log_queue_addr(&data);
    BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
               dev_id,
               0,
               0,
               err_addr,
               BF_ERR_TYPE_MULTI_BIT_ECC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MAC 0 queue multi bit ecc error at addr0x%x",
               err_addr);
    LOG_TRACE("MBC MAC 0 queue multi bit ecc error at addr0x%x", err_addr);
    en_value |= (0x1u << 12);
  } else if (intr_status_val & (0x1u << 13)) {
    address = offsetof(tof3_reg,
device_select.mbc.mbc_mbus.controller_mbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof3_mbus_controller_mbe_log_queue_addr(&data);
    BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
               dev_id,
               0,
               0,
               err_addr,
               BF_ERR_TYPE_MULTI_BIT_ECC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC WB iqueue multi bit ecc error at addr0x%x",
               err_addr);
    LOG_TRACE("MBC WB iqueue multi bit ecc error at addr0x%x", err_addr);
    en_value |= (0x1u << 13);
  }
  if (intr_status_val & (0x1u << 14)) {
    address = offsetof(tof3_reg,
device_select.mbc.mbc_mbus.controller_sbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof3_mbus_controller_sbe_log_queue_addr(&data);
    BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
               dev_id,
               0,
               0,
               err_addr,
               BF_ERR_TYPE_SINGLE_BIT_ECC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MAC 0 queue single bit ecc error at addr0x%x",
               err_addr);
    LOG_TRACE("MBC MAC 0 queue single bit ecc error at addr0x%x", err_addr);
  } else if (intr_status_val & (0x1u << 15)) {
    address = offsetof(tof3_reg,
device_select.mbc.mbc_mbus.controller_sbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof3_mbus_controller_sbe_log_queue_addr(&data);
    BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
               dev_id,
               0,
               0,
               err_addr,
               BF_ERR_TYPE_SINGLE_BIT_ECC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC WB iqueue single bit ecc error at addr0x%x",
               err_addr);
    LOG_TRACE("MBC WB iqueue single bit ecc error at addr0x%x", err_addr);
  }
  if (intr_status_val & (0x1u << 16)) {
    for (i = 0; i < 4; i++) {
      address =
          offsetof(tof3_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data +
i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((parity_err_data[3] & 0x1fff) << 29);
    err_data64 = (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61) +
                 (parity_err_data[1] << 29) +
                 ((parity_err_data[0] >> 3) & 0x1fffffff);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_PARITY,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MBUS parity err0: cycle %d, type %d, sid %d, qid %d, tag "
               "%d, addr 0x%lx, data 0x%lx, parity %d",
               (parity_err_data[3] >> 25) & 0x7,
               (parity_err_data[3] >> 23) & 0x3,
               (parity_err_data[3] >> 16) & 0x7f,
               (parity_err_data[3] >> 15) & 0x1,
               (parity_err_data[3] >> 13) & 0x3,
               err_addr64,
               err_data64,
               (parity_err_data[0]) & 0x7);
    LOG_TRACE(
        "MBC MBUS parity err0: cycle %d, type %d, sid %d, qid %d, tag %d, addr
"
        "0x%lx, data 0x%lx, parity %d",
        (parity_err_data[3] >> 25) & 0x7,
        (parity_err_data[3] >> 23) & 0x3,
        (parity_err_data[3] >> 16) & 0x7f,
        (parity_err_data[3] >> 15) & 0x1,
        (parity_err_data[3] >> 13) & 0x3,
        err_addr64,
        err_data64,
        (parity_err_data[0]) & 0x7);
    en_value |= (0x1u << 16);
  } else if (intr_status_val & (0x1u << 17)) {
    for (i = 0; i < 4; i++) {
      address =
          offsetof(tof3_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data +
i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((parity_err_data[3] & 0x1fff) << 29);
    err_data64 = (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61) +
                 (parity_err_data[1] << 29) +
                 ((parity_err_data[0] >> 3) & 0x1fffffff);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_PARITY,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MBUS parity err1: cycle %d, type %d, sid %d, qid %d, tag "
               "%d, addr 0x%lx, data 0x%lx, parity %d",
               (parity_err_data[3] >> 25) & 0x7,
               (parity_err_data[3] >> 23) & 0x3,
               (parity_err_data[3] >> 16) & 0x7f,
               (parity_err_data[3] >> 15) & 0x1,
               (parity_err_data[3] >> 13) & 0x3,
               err_addr64,
               err_data64,
               (parity_err_data[0]) & 0x7);
    LOG_TRACE(
        "MBC MBUS parity err1: cycle %d, type %d, sid %d, qid %d, tag %d, addr
"
        "0x%lx, data 0x%lx, parity %d",
        (parity_err_data[3] >> 25) & 0x7,
        (parity_err_data[3] >> 23) & 0x3,
        (parity_err_data[3] >> 16) & 0x7f,
        (parity_err_data[3] >> 15) & 0x1,
        (parity_err_data[3] >> 13) & 0x3,
        err_addr64,
        err_data64,
        (parity_err_data[0]) & 0x7);
    en_value |= (0x1u << 17);
  } else if (intr_status_val & (0x1u << 18)) {
    for (i = 0; i < 4; i++) {
      address =
          offsetof(tof3_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data +
i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((parity_err_data[3] & 0x1fff) << 29);
    err_data64 = (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61) +
                 (parity_err_data[1] << 29) +
                 ((parity_err_data[0] >> 3) & 0x1fffffff);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_PARITY,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MBUS parity err2: cycle %d, type %d, sid %d, qid %d, tag "
               "%d, addr 0x%lx, data 0x%lx, parity %d",
               (parity_err_data[3] >> 25) & 0x7,
               (parity_err_data[3] >> 23) & 0x3,
               (parity_err_data[3] >> 16) & 0x7f,
               (parity_err_data[3] >> 15) & 0x1,
               (parity_err_data[3] >> 13) & 0x3,
               err_addr64,
               err_data64,
               (parity_err_data[0]) & 0x7);
    LOG_TRACE(
        "MBC MBUS parity err2: cycle %d, type %d, sid %d, qid %d, tag %d, addr
"
        "0x%lx, data 0x%lx, parity %d",
        (parity_err_data[3] >> 25) & 0x7,
        (parity_err_data[3] >> 23) & 0x3,
        (parity_err_data[3] >> 16) & 0x7f,
        (parity_err_data[3] >> 15) & 0x1,
        (parity_err_data[3] >> 13) & 0x3,
        err_addr64,
        err_data64,
        (parity_err_data[0]) & 0x7);
    en_value |= (0x1u << 18);
  }
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr,
en_value);
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}*/
static uint32_t pipe_mgr_tof3_pbc0_intr_handle(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  LOG_TRACE("PBC0 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;
  if (intr_status_val & (0x1u)) {
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_OVERFLOW,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_PBC,
               "PBC host overflow error");
    LOG_TRACE("PBC host overflow error");
    pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, 0x1u);
  }
  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pbc2_intr_handle(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr;
  int bitpos = 0;
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  LOG_TRACE("PBC2 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;
  for (bitpos = 0; bitpos < 9; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // il tx dr rd err
        case 1:
        case 2:
        case 3:
          address = offsetof(
              tof3_reg, device_select.pbc.pbc_pbus.il_tx_dr_rd_err_log[bitpos]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_il_tx_dr_rd_err_log_il_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_pbus_il_tx_dr_rd_err_log_il_tx_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC IL tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              bitpos,
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC IL tx dr%d rd error, err_status %d, err_head_pointer 0x%x",
              bitpos,
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 4:
          address = offsetof(tof3_reg,
                             device_select.pbc.pbc_pbus.wb_tx_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_pbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC WB tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC WB tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 5:
          address = offsetof(tof3_reg,
                             device_select.pbc.pbc_pbus.rb_tx_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof3_pbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC RB tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC RB tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 6:
          address = offsetof(tof3_reg,
                             device_select.pbc.pbc_pbus.stat_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_stat_fm_dr_rd_err_log_stat_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof3_pbus_stat_fm_dr_rd_err_log_stat_fm_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC stat fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC stat fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 7:
          address = offsetof(tof3_reg,
                             device_select.pbc.pbc_pbus.idle_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_idle_fm_dr_rd_err_log_idle_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof3_pbus_idle_fm_dr_rd_err_log_idle_fm_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC idle fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC idle fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        case 8:
          address = offsetof(tof3_reg,
                             device_select.pbc.pbc_pbus.diag_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof3_pbus_diag_fm_dr_rd_err_log_diag_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof3_pbus_diag_fm_dr_rd_err_log_diag_fm_dr_rd_err_head_ptr(
                  &data);
          BF_ERR_EVT(
              BF_ERR_SEV_FATAL,
              dev_id,
              0,
              0,
              0,
              BF_ERR_TYPE_GENERIC,
              BF_ERR_BLK_DMA,
              BF_ERR_LOC_DMA_PBC,
              "PBC diag fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          LOG_TRACE(
              "PBC diag fm dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
          en_value |= (0x1u << bitpos);
          break;
        default:
          break;
      }
    }
  }
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, en_value);
  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof3_pbc3_intr_handle(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  // ***TBD Tf3-fix handle subdev_id
  (void) subdev_id;

  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t que, que_addr, i;
  uint32_t parity_err_data[7];
  uint64_t err_addr64, err_data64[2];
  char name[7] = {'\n', '\n', '\n', '\n', '\n', '\n', '\n'};
  char *name_p = name;
  int bitpos = 0;
  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev_id,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  LOG_TRACE("PBC3 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);
  (void)enable_lo_addr;
  (void)userdata;
  for (bitpos = 0; bitpos < 29; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:
        case 13:
          name_p = "IL i0";
          break;
        case 1:
        case 14:
          name_p = "IL i1";
          break;
        case 2:
        case 15:
          name_p = "IL i2";
          break;
        case 3:
        case 16:
          name_p = "IL i3";
          break;
        case 4:
        case 17:
          name_p = "IL o0";
          break;
        case 5:
        case 18:
          name_p = "IL o1";
          break;
        case 6:
        case 19:
          name_p = "IL o2";
          break;
        case 7:
        case 20:
          name_p = "IL o3";
          break;
        case 8:
        case 21:
          name_p = "WB i";
          break;
        case 9:
        case 22:
          name_p = "RB o";
          break;
        case 10:
        case 23:
          name_p = "stat o";
          break;
        case 11:
        case 24:
          name_p = "idle";
          break;
        case 12:
        case 25:
          name_p = "diag";
          break;
        case 26:
        case 27:
        case 28:
          for (i = 0; i < 7; i++) {
            address = offsetof(tof3_reg,
                               device_select.pbc.pbc_pbus.parity_err_log[i]);
            pipe_mgr_interrupt_read_register(
                dev_id, address, (parity_err_data + i));
          }
          err_addr64 = ((parity_err_data[5] & 0x1fff) << 29) +
                       ((parity_err_data[4] >> 3) & 0x1fffffff);
          err_data64[0] =
              ((parity_err_data[0] >> 3) & 0x1fffffff) +
              (parity_err_data[1] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61);
          err_data64[1] =
              ((parity_err_data[2] >> 3) & 0x1fffffff) +
              (parity_err_data[3] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[4] & 0x7) << 61);
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev_id,
                     0,
                     0,
                     0,
                     BF_ERR_TYPE_PARITY,
                     BF_ERR_BLK_DMA,
                     BF_ERR_LOC_DMA_PBC,
                     "PBC PBUS parity err: cycle %d, type %d, sid %d, qid %d, "
                     "tag %d, addr 0x%" PRIx64 ", data 0x%" PRIx64 "%016" PRIx64
                     ", parity %d",
                     (((parity_err_data[6] >> 31) & 0x1) +
                      ((parity_err_data[5] & 0x3) << 1)),
                     (parity_err_data[5] >> 29) & 0x3,
                     (parity_err_data[5] >> 23) & 0x3f,
                     (parity_err_data[5] >> 20) & 0x7,
                     (parity_err_data[5] >> 13) & 0x7f,
                     err_addr64,
                     err_data64[0],
                     err_data64[1],
                     (parity_err_data[0]) & 0x7);
          LOG_TRACE(
              "PBC PBUS parity err: cycle %d, type %d, sid %d, qid %d, tag "
              "%d, "
              "addr 0x%" PRIx64 ", data 0x%" PRIx64 "%016" PRIx64 ", parity %d",
              (((parity_err_data[6] >> 31) & 0x1) +
               ((parity_err_data[5] & 0x3) << 1)),
              (parity_err_data[5] >> 29) & 0x3,
              (parity_err_data[5] >> 23) & 0x3f,
              (parity_err_data[5] >> 20) & 0x7,
              (parity_err_data[5] >> 13) & 0x7f,
              err_addr64,
              err_data64[0],
              err_data64[1],
              (parity_err_data[0]) & 0x7);
          en_value |= (0x1u << bitpos);
          break;
        default:
          break;
      }
      if (bitpos < 13) {  // mbe
        address =
            offsetof(tof3_reg, device_select.pbc.pbc_pbus.controller_mbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof3_pbus_controller_mbe_log_queue_addr(&data);
        que = getp_tof3_pbus_controller_mbe_log_queue(&data);
        BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                   dev_id,
                   0,
                   0,
                   que_addr,
                   BF_ERR_TYPE_MULTI_BIT_ECC,
                   BF_ERR_BLK_DMA,
                   BF_ERR_LOC_DMA_PBC,
                   "PBC %s queue multi bit ecc error at queue 0x%x, addr0x%x",
                   name_p,
                   que,
                   que_addr);
        LOG_TRACE("PBC %s queue multi bit ecc error at queue 0x%x, addr0x%x",
                  name_p,
                  que,
                  que_addr);
        en_value |= (0x1u << bitpos);
      } else if (bitpos < 26) {  // sbe
        address =
            offsetof(tof3_reg, device_select.pbc.pbc_pbus.controller_sbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof3_pbus_controller_sbe_log_queue_addr(&data);
        que = getp_tof3_pbus_controller_sbe_log_queue(&data);
        BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                   dev_id,
                   0,
                   0,
                   que_addr,
                   BF_ERR_TYPE_SINGLE_BIT_ECC,
                   BF_ERR_BLK_DMA,
                   BF_ERR_LOC_DMA_PBC,
                   "PBC %s queue single bit ecc error at queue 0x%x, addr0x%x",
                   name_p,
                   que,
                   que_addr);
        LOG_TRACE("PBC %s queue single bit ecc error at queue 0x%x, addr0x%x",
                  name_p,
                  que,
                  que_addr);
        en_value |= (0x1u << bitpos);
      }
    }
  }
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, en_value);
  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_mau_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  dev_stage_t stage = 0;
  int row = 0, ret = 0, alu_idx = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for mau ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* Fill in the userdata for the callback */
      for (row = 0; row < PIPE_MGR_INTR_MAX_ROWS; row++) {
        PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row).pipe = pipe;
        PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row).stage = stage;
        PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row).row = row;
      }

      /* MAU Config. */
      ret = lld_int_register_cb(
          dev, subdev_id, 
          offsetof(tof3_reg,
                   pipes[phy_pipe].mau[stage].cfg_regs.intr_status_mau_cfg),
          &pipe_mgr_tof3_intr_mau_cfg_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      /* MAU Addrdist. */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(
              tof3_reg,
              pipes[phy_pipe].mau[stage].rams.match.adrdist.intr_status_mau_ad),
          &pipe_mgr_tof3_intr_mau_adrdist_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
      /* Stats ALU. */
      for (alu_idx = 0; alu_idx < 4; alu_idx++) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.stats_wrap[alu_idx]
                         .stats.intr_status_mau_stats_alu),
            &pipe_mgr_tof3_stats_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, alu_idx)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* Syn-2-Port */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.row[row]
                         .i2portctl.intr_status_mau_synth2port),
            &pipe_mgr_tof3_synth2port_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* MAP Ram ECC/Parity. */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; row++) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.row[row]
                         .adrmux.intr_status_mau_adrmux_row),
            &pipe_mgr_tof3_ecc_map_ram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* Unit RAM ECC. */
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.array.row[row]
                         .intr_status_mau_unit_ram_row),
            &pipe_mgr_tof3_ecc_sram_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* Selector ALU. */
      for (row = 0; row < PIPE_MGR_INTR_MAX_SEL_ALU_ROWS; row++) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.map_alu.meter_group[row]
                         .selector.intr_status_mau_selector_alu),
            &pipe_mgr_tof3_sel_alu_mem_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row)));
        PIPE_MGR_ASSERT(ret != -1);
      }
      /* IMEM Parity. */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe].mau[stage].dp.intr_status_mau_imem),
          &pipe_mgr_tof3_imem_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
      /* GFM Parity. */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe].mau[stage].dp.intr_status_mau_gfm_hash),
          &pipe_mgr_gfm_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
      /* TCAM Parity. */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe].mau[stage].tcams.intr_status_mau_tcam_array),
          &pipe_mgr_tof3_ecc_tcam_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_parser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0, parser = 0, xpb = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for parser ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (xpb = 0; xpb < TOF3_NUM_IPB; xpb++) {
      /* IPB */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ipbprsr4reg[xpb]
                       .ipbreg.glb_group.intr_stat.stat.stat_0_2),
          &pipe_mgr_tof3_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ipbprsr4reg[xpb]
                       .ipbreg.glb_group.intr_stat.stat.stat_1_2),
          &pipe_mgr_tof3_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);
      /* EPB */
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.epbprsr4reg[xpb]
                       .epbreg.glb_group.intr_stat.stat.stat_0_2),
          &pipe_mgr_tof3_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.epbprsr4reg[xpb]
                       .epbreg.glb_group.intr_stat.stat.stat_1_2),
          &pipe_mgr_tof3_xpb_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, xpb)));
      PIPE_MGR_ASSERT(ret != -1);

      for (parser = 0; parser < (TOF3_NUM_PARSERS / TOF3_NUM_IPB); parser++) {
        /* Ingress Parser */
        ret =
            lld_int_register_cb(dev, subdev_id,
                                offsetof(tof3_reg,
                                         pipes[phy_pipe]
                                             .pardereg.pgstnreg.ipbprsr4reg[xpb]
                                             .prsr[parser]
                                             .intr.stat),
                                &pipe_mgr_tof3_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev, pipe, 0, (xpb << 2) | parser)));
        PIPE_MGR_ASSERT(ret != -1);

        /* Egress Parser */
        ret =
            lld_int_register_cb(dev, subdev_id,
                                offsetof(tof3_reg,
                                         pipes[phy_pipe]
                                             .pardereg.pgstnreg.epbprsr4reg[xpb]
                                             .prsr[parser]
                                             .intr.stat),
                                &pipe_mgr_tof3_parser_err_handle,
                                (void *)&(PIPE_INTR_CALLBACK_DATA(
                                    dev, pipe, 1, (xpb << 2) | parser)));
        PIPE_MGR_ASSERT(ret != -1);
      }
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_deparser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  int slice;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix
  LOG_TRACE("Pipe-mgr Registering for deparser ecc interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.stat),
        &pipe_mgr_tof3_deparser_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat),
        &pipe_mgr_tof3_deparser_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat),
        &pipe_mgr_tof3_deparser_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    for (slice = 0; slice < 4; slice++) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.inpslice[slice]
                       .intr.stat),
          &pipe_mgr_tof3_deparser_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .hir.h.intr.stat),
          &pipe_mgr_tof3_deparser_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .out_ingr.intr_0.stat),
          &pipe_mgr_tof3_deparser_err_handle1,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .out_ingr.intr_1.stat),
          &pipe_mgr_tof3_deparser_err_handle2,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .her.h.intr.stat),
          &pipe_mgr_tof3_deparser_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .out_egr.intr_0.stat),
          &pipe_mgr_tof3_deparser_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .out_egr.intr_1.stat),
          &pipe_mgr_tof3_deparser_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_mirror_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  int slice;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for mirror interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    /* mirror */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.intr.stat),
        &pipe_mgr_tof3_mirror_s2p_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    for (slice = 0; slice < 4; slice++) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.mirreg.mirror.slice_regs[slice]
                       .intr.stat),
          &pipe_mgr_tof3_mirror_slice_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_parde_misc_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for parde interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* PG Station PBus */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg, pipes[phy_pipe].pardereg.pgstnreg.pbusreg.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PMERGE */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.ll0.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.lr0.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.lr1.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PARB */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.parbreg.left.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.parbreg.right.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* S2P */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg, pipes[phy_pipe].pardereg.pgstnreg.s2preg.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* P2S */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg, pipes[phy_pipe].pardereg.pgstnreg.p2sreg.intr.stat),
        &pipe_mgr_tof3_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* EBuf */
    for (int ebuf = 0; ebuf < 4; ++ebuf) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ebuf900reg[ebuf]
                       .ebuf100reg.glb_group.intr_stat),
          &pipe_mgr_tof3_ebuf_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, ebuf, 0)));
      PIPE_MGR_ASSERT(ret != -1);
      for (int which = 0; which < 2; ++which) {
        ret = lld_int_register_cb(
            dev, subdev_id,
            offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .pardereg.pgstnreg.ebuf900reg[ebuf]
                         .ebuf400reg[which]
                         .glb_group.intr_stat),
            &pipe_mgr_tof3_ebuf_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, ebuf, which)));
        PIPE_MGR_ASSERT(ret != -1);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_pgr_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for pgr interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4),
        &pipe_mgr_tof3_pgr_err_handle0,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4),
        &pipe_mgr_tof3_pgr_err_handle1,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4),
        &pipe_mgr_tof3_pgr_err_handle2,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(
            tof3_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4),
        &pipe_mgr_tof3_pgr_err_handle3,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_lfltr_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for learn filter interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg, device_select.lfltr[phy_pipe].ctrl.intr_stat),
        &pipe_mgr_tof3_lfltr_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_sbc_interrupt_notifs(bf_dev_id_t dev) {
  int ret = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix

  LOG_TRACE("Pipe-mgr Registering for SBC interrupts");

  /* MBus */
  if (0) {
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg, device_select.mbc.mbc_mbus.intr_stat),
        &pipe_mgr_tof3_mbc_intr_handle,
        NULL);
    PIPE_MGR_ASSERT(ret != -1);
  }

  /* PBus */
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat0),
      &pipe_mgr_tof3_pbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);
  /* Nothing interesting in pbc_pbus.intr_stat1 */
  ret = lld_int_register_cb(
      dev, subdev_id,  
      offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat2),
      &pipe_mgr_tof3_pbc2_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat3),
      &pipe_mgr_tof3_pbc3_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* CBus */
  ret = lld_int_register_cb(
      dev, subdev_id, 
      offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat0),
      &pipe_mgr_tof3_cbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat1),
      &pipe_mgr_tof3_cbc1_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* TBus */
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_stat0),
      &pipe_mgr_tof3_tbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);
  /* Nothing interesting in tbc.tbc_tbus.intr_stat_1 */
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_stat2),
      &pipe_mgr_tof3_tbc2_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_register_tm_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  bf_subdev_id_t subdev_id = 0; //TBD Tf3-fix
  LOG_TRACE("Pipe-mgr Registering for TM interrupts");

  /* CAA Common */
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.stat),
      &pipe_mgr_tof3_tm_caa_err_handle,
      (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
  PIPE_MGR_ASSERT(ret != -1);

  /* PSC Common */
  ret = lld_int_register_cb(
      dev, subdev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.stat),
      &pipe_mgr_tof3_tm_psc_err_handle,
      (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 1)));
  PIPE_MGR_ASSERT(ret != -1);

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* WAC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                     .wac_reg.intr.stat),
        &pipe_mgr_tof3_tm_wac_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* QAC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                     .qac_reg.intr.stat),
        &pipe_mgr_tof3_tm_qac_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* SCH-A/B */
    if (phy_pipe == 0) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_scha_top.sch[0].intr.stat),
          &pipe_mgr_tof3_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 1) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_scha_top.sch[1].intr.stat),
          &pipe_mgr_tof3_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 2) {
      ret = lld_int_register_cb(
          dev,
          offsetof(tof3_reg, device_select.tm_top.tm_schb_top.sch[0].intr.stat),
          &pipe_mgr_tof3_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 3) {
      ret = lld_int_register_cb(
          dev, subdev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_schb_top.sch[1].intr.stat),
          &pipe_mgr_tof3_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    }

    /* CLC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_clc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PEX */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_pex_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* QLC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_qlc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PRC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_prc_top.prc[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_prc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PRE */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_pre_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PSC */
    ret = lld_int_register_cb(
        dev, subdev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc[phy_pipe].intr.stat),
        &pipe_mgr_tof3_tm_psc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t tof3_pipe_intr_reg_wr(rmt_dev_info_t *dev_info,
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
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == x);
  }
  return x;
}

static pipe_status_t pipe_mgr_tof3_mau_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
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
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg, pipes[0].mau[s].cfg_regs.intr_enable0_mau_cfg),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* MAU Addrdist. */
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0].mau[s].rams.match.adrdist.intr_enable0_mau_ad),
        en_val & 0xFF);
    if (ret != PIPE_SUCCESS) return ret;

    /* Stats ALU and Selector ALU. */
    for (int a = 0; a < 4; ++a) {
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof3_reg,
                   pipes[0].mau[s].rams.map_alu.stats_wrap
                       [a].stats.intr_enable0_mau_stats_alu),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof3_reg,
                   pipes[0].mau[s].rams.map_alu.meter_group
                       [a].selector.intr_enable0_mau_selector_alu),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }

    for (int r = 0; r < dev_info->dev_cfg.stage_cfg.num_map_ram_rows; ++r) {
      /* Syn-2-Port. */
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof3_reg,
                   pipes[0].mau[s].rams.map_alu.row
                       [r].i2portctl.intr_status_mau_synth2port),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;

      /* MAP Ram ECC/Parity. */
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof3_reg,
                   pipes[0].mau[s].rams.map_alu.row
                       [r].adrmux.intr_enable0_mau_adrmux_row),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;

      /* Unit RAM ECC. */
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof3_reg,
              pipes[0].mau[s].rams.array.row[r].intr_enable0_mau_unit_ram_row),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }

    /* IMEM Parity. */
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg, pipes[0].mau[s].dp.intr_enable0_mau_imem),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* TCAM Parity. */
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg, pipes[0].mau[s].tcams.intr_enable0_mau_tcam_array),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_gfm_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_status_t ret;

  LOG_TRACE("Setting GFM Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* GFM Parity is enabled on a per profile basis. */
  for (unsigned i = 0; i < dev_info->num_pipeline_profiles; ++i) {
    if (!dev_info->profile_info[i]->driver_options.hash_parity_enabled)
      continue;

    for (int s = 0; s < dev_info->num_active_mau; ++s) {
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &dev_info->profile_info[i]->pipe_bmp,
          shdl,
          offsetof(tof3_reg, pipes[0].mau[s].dp.intr_enable0_mau_gfm_hash),
          en_val);
      if (ret != PIPE_SUCCESS) return ret;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_parser_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
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

  for (int xpb = 0; xpb < TOF3_NUM_IPB; ++xpb) {
    /* IPB */
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.pgstnreg.ipbprsr4reg[xpb]
                     .ipbreg.glb_group.intr_stat.en0.en0_0_2),
        en_val & 0xFFFFFFF2);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.pgstnreg.ipbprsr4reg[xpb]
                     .ipbreg.glb_group.intr_stat.en0.en0_1_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    /* EPB */
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.pgstnreg.epbprsr4reg[xpb]
                     .epbreg.glb_group.intr_stat.en0.en0_0_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.pgstnreg.epbprsr4reg[xpb]
                     .epbreg.glb_group.intr_stat.en0.en0_1_2),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    for (int prsr = 0; prsr < 4; ++prsr) {
      /* Ingress Parsers */
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof3_reg,
              pipes[0].pardereg.pgstnreg.ipbprsr4reg[xpb].prsr[prsr].intr.en0),
          en_val & 0xFFFFFE00);
      if (ret != PIPE_SUCCESS) return ret;

      /* Egress Parsers */
      ret = tof3_pipe_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(
              tof3_reg,
              pipes[0].pardereg.pgstnreg.epbprsr4reg[xpb].prsr[prsr].intr.en0),
          en_val & 0xFFFFFE00);
      if (ret != PIPE_SUCCESS) return ret;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_deparser_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting ig_deparser Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg,
               pipes[0].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg,
               pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  for (int slice = 0; slice < 4; ++slice) {
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0].pardereg.dprsrreg.dprsrreg.inpslice[slice].intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof3_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[slice].hir.h.intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                     .out_ingr.intr_0.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0]
                     .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                     .out_ingr.intr_1.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof3_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].her.h.intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof3_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.intr_0.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof3_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.intr_1.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_pgr_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE(" Setting PGR Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_0_4),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_1_4),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_2_4),
      en_val & 0x7FFFF);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_3_4),
      en_val & 0xFFFFFFF8);
  if (ret != PIPE_SUCCESS) return ret;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_mirror_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting Mirror Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.mirreg.mirror.s2p_regs.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  for (int slice = 0; slice < 4; ++slice) {
    ret = tof3_pipe_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof3_reg,
                 pipes[0].pardereg.mirreg.mirror.slice_regs[slice].intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_parde_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting Parde Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  /* PG Station PBus */
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pbusreg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* PMERGE */
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pmergereg.ll0.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pmergereg.lr0.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pmergereg.lr1.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* PARB */
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.parbreg.left.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.parbreg.right.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* S2P */
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.s2preg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* P2S */
  ret = tof3_pipe_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.p2sreg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* EBuf */
  for (int ebuf = 0; ebuf < 4; ++ebuf) {
    ret = tof3_pipe_intr_reg_wr(dev_info,
                                &pbm,
                                shdl,
                                offsetof(tof3_reg,
                                         pipes[0]
                                             .pardereg.pgstnreg.ebuf900reg[ebuf]
                                             .ebuf100reg.glb_group.intr_en0),
                                en_val & 0xFFFF03FF);
    if (ret != PIPE_SUCCESS) return ret;
    for (int which = 0; which < 2; ++which) {
      ret =
          tof3_pipe_intr_reg_wr(dev_info,
                                &pbm,
                                shdl,
                                offsetof(tof3_reg,
                                         pipes[0]
                                             .pardereg.pgstnreg.ebuf900reg[ebuf]
                                             .ebuf400reg[which]
                                             .glb_group.intr_en0),
                                en_val & 0xFFFF03FF);
      if (ret != PIPE_SUCCESS) return ret;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_lfltr_interrupt_en_set(
    rmt_dev_info_t *dev_info, bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;

  LOG_TRACE("Setting learn filter Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg, device_select.lfltr[phy_pipe].ctrl.intr_en0),
        en_val & 0xFFFFFFFC);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_tm_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                       bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;

  LOG_TRACE("Setting TM Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* CAA Common */
  lld_write_register(
      dev_info->dev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.en0),
      en_val);

  /* PSC Common */
  lld_write_register(
      dev_info->dev_id,
      offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.en0),
      en_val);

  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    /* WAC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                     .wac_reg.intr.en0),
        en_val & 0xFFFFFBFF);
    /* QAC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                     .qac_reg.intr.en0),
        en_val & 0x3FF);
    /* SCH-A/B */
    if (phy_pipe == 0) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_scha_top.sch[0].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 1) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_scha_top.sch[1].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 2) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_schb_top.sch[0].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 3) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof3_reg, device_select.tm_top.tm_schb_top.sch[1].intr.en0),
          en_val & 0x3FFFFFFF);
    }
    /* CLC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_clc_top.clc[phy_pipe].intr.en0),
        en_val);
    /* PEX */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pex_top.pex[phy_pipe].intr.en0),
        en_val);
    /* QLC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_qlc_top.qlc[phy_pipe].intr.en0),
        en_val);
    /* PRC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_prc_top.prc[phy_pipe].intr.en0),
        en_val);
    /* PRE */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_pre_top.pre[phy_pipe].intr.en0),
        en_val & 0xFFFFFCF0);
    /* PSC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof3_reg,
                 device_select.tm_top.tm_psc_top.psc[phy_pipe].intr.en0),
        en_val);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_tof3_sbc_interrupt_en_set(bf_dev_id_t dev_id,
                                                        bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  LOG_TRACE("Setting SBC Interrupt mode to %s ", enable ? "Enable" : "Disable");

  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.mbc.mbc_mbus.intr_en_0),
                     en_val & 0x7F601);

  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_0),
                     en_val & 0x1);
  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_0),
                     en_val & 0x1FF);
  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_0),
                     en_val);

  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en0_0),
                     en_val & 0x3E00001);
  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en1_0),
                     en_val);

  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_0),
                     en_val & 0x1FFE0001);
  lld_write_register(dev_id,
                     offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en2_0),
                     en_val);

  return PIPE_SUCCESS;
}
#endif

pipe_status_t pipe_mgr_tof3_interrupt_en_set_helper(rmt_dev_info_t *dev_info,
                                                    bool enable,
                                                    bool push_now) {
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_status_t status = PIPE_SUCCESS;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  (void)enable;
  if (push_now) {
    status = pipe_mgr_api_enter(shdl);
    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

#if 0
  /* TODO TOFINO3 */
  status |= pipe_mgr_tof3_mau_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_gfm_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_parser_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_deparser_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_pgr_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_mirror_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_parde_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_lfltr_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_tm_interrupt_en_set(dev_info, enable);
  status |= pipe_mgr_tof3_sbc_interrupt_en_set(dev, enable);
#endif

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

pipe_status_t pipe_mgr_tof3_tcam_read(bf_dev_id_t dev) {
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int parser = 0, dir = 0;
  dev_stage_t stage = 0;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);

  if (pipe_mgr_is_device_locked(dev)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_SUCCESS;
  }

  int elem_0_size[PIPE_DIR_MAX], arr_0_cnt[PIPE_DIR_MAX];
  int elem_1_size[PIPE_DIR_MAX], arr_1_cnt[PIPE_DIR_MAX];
  uint64_t address = 0;
  int prsr_cnt = 0;

  for (dir = 0; dir < PIPE_DIR_MAX; dir++) {
    elem_0_size[dir] = PIPE_MGR_TOF3_TCAM_WORD_WIDTH;
    arr_0_cnt[dir] = TOF3_PARSER_DEPTH;
    elem_1_size[dir] = 0;
    arr_1_cnt[dir] = 0;
  }
  prsr_cnt = TOF3_NUM_PARSERS;

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (dir = 0; dir < PIPE_DIR_MAX; dir++) {
      for (parser = 0; parser < prsr_cnt; parser++) {
        if (arr_0_cnt[dir]) {
          address = PIPE_PRSR_ADDR(dev, dir).tof3.tcam_addr +
                    (tof3_mem_pipes_array_element_size * phy_pipe) +
                    (PIPE_PRSR_ADDR(dev, dir).tof3.prsr_step * parser);
          if (address) {
            status = pipe_mgr_drv_blk_rd(&shdl,
                                         dev,
                                         elem_0_size[dir],
                                         arr_0_cnt[dir],
                                         1,
                                         address,
                                         NULL,
                                         NULL);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s : Read block push for tcam word0 refresh "
                  "error %s",
                  __func__,
                  pipe_str_err(status));
            }
          }
        }
        if (arr_1_cnt[dir]) {
          address = 0;  // FIXME
          if (address) {
            status = pipe_mgr_drv_blk_rd(&shdl,
                                         dev,
                                         elem_1_size[dir],
                                         arr_1_cnt[dir],
                                         1,
                                         address,
                                         NULL,
                                         NULL);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s : Read block push for tcam word1 refresh "
                  "error %s",
                  __func__,
                  pipe_str_err(status));
            }
          }
        }
      }  // parser
    }    // direction

    /* Scrub mau tcam */
    int mau_tcam_size = 0, mau_tcam_cnt = 0;
    mau_tcam_size = PIPE_MGR_TOF3_TCAM_WORD_WIDTH;
    mau_tcam_cnt = TOF3_TCAM_UNIT_DEPTH;
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      int mem_id = 0, index = 0;
      uint64_t tcam_addr = 0;
      for (mem_id = 0; mem_id < PIPE_MGR_MAX_MEM_ID; mem_id++) {
        for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
          if (PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) == 0) {
            continue;
          }
          tcam_addr =
              PIPE_INTR_TBL_TCAM_BASE_ADDR(dev, pipe, stage, mem_id, index);
          /* Make sure it is a tcam */
          if (tcam_addr == 0) {
            continue;
          }
          status = pipe_mgr_drv_blk_rd(&shdl,
                                       dev,
                                       mau_tcam_size,
                                       mau_tcam_cnt,
                                       1,
                                       tcam_addr,
                                       NULL,
                                       NULL);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s : Read block push for mau tcam refresh "
                "error %s",
                __func__,
                pipe_str_err(status));
          }
          /* Only read the TCAM once. */
          break;
        }
      }  // mem_id
    }    // stage
  }      // pipe

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}
