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

#include <mc_mgr/mc_mgr_pipe_intf.h>

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/* Dummy interrupt handler */
static uint32_t pipe_mgr_tof2_dprsr_dummy_intr_handle(bf_dev_id_t dev,
                                                      bf_subdev_id_t subdev_id,
                                                      uint32_t intr_address,
                                                      uint32_t intr_status_val,
                                                      uint32_t enable_hi_addr,
                                                      uint32_t enable_lo_addr,
                                                      void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  // pipes[].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.stat
  // pipes[].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat
  // pipes[].pardereg.dprsrreg.dprsrreg.inpslice[].intr.stat
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

/*
 * Deparser Input Phase.
 *
 * Note: this is a minimal implementation that only handles the pvt_sbe
 * and pvt_mbe interrupts.
 *
 * Decodes:
 * - pipes[].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat
 */
static uint32_t pipe_mgr_tof2_dprsr_inp_intr_handle(bf_dev_id_t dev,
                                                    bf_subdev_id_t subdev_id,
                                                    uint32_t intr_address,
                                                    uint32_t intr_status_val,
                                                    uint32_t enable_hi_addr,
                                                    uint32_t enable_lo_addr,
                                                    void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)subdev_id;

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
  int row = 0;

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

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("deparser intr (pipe %d)", pipe);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // 0: pvt_sbe
  if (intr_status_val & (1u << 0)) {
    /* Decode location */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.pv_tbl_sbe_err_log);
    status = pipe_mgr_interrupt_read_register(dev, address, &data);
    row = getp_tof2_dprsr_ic_regs_pv_tbl_sbe_err_log_addr(&data);

    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
               dev,
               pipe,
               0,
               row,
               BF_ERR_TYPE_SINGLE_BIT_ECC,
               BF_ERR_BLK_DEPRSR,
               BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0,
               "Deparser Pipe Vector Tbl single bit error at"
               " row %d",
               row);
    LOG_TRACE("Dpsr Pipe Vector Tbl single bit err at row %d ", row);

    /* Repair memory */
    mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
  }

  // 1: pvt_mbe
  if (intr_status_val & (1u << 1)) {
    /* Decode location */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.pv_tbl_mbe_err_log);
    status = pipe_mgr_interrupt_read_register(dev, address, &data);
    row = getp_tof2_dprsr_ic_regs_pv_tbl_mbe_err_log_addr(&data);

    /* Report event */
    BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
               dev,
               pipe,
               0,
               row,
               BF_ERR_TYPE_MULTI_BIT_ECC,
               BF_ERR_BLK_DEPRSR,
               BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0,
               "Deparser Pipe Vector Tbl multi bit error"
               " at row %d",
               row);
    LOG_TRACE("Dpsr Pipe Vector Tbl0 multi bit err at row %d ", row);

    /* Repair memory */
    mc_mgr_ecc_correct_pvt(dev, pipe, row, false);
  }

  /* Clear interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/*
 * Deparser Header Phase (Ingress, Egress).
 *
 * Decodes:
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_i[].hir.h.intr.stat (ingress)
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_e[].her.h.intr.stat (egress)
 */
static uint32_t pipe_mgr_tof2_dprsr_hdr_intr_handle(bf_dev_id_t dev,
                                                    bf_subdev_id_t subdev_id,
                                                    uint32_t intr_address,
                                                    uint32_t intr_status_val,
                                                    uint32_t enable_hi_addr,
                                                    uint32_t enable_lo_addr,
                                                    void *userdata) {
  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  (void)subdev_id;

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, slice = 0, dir = 0;
  uint32_t err_addr;

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
  dir = userdata_p->stage;
  slice = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  /* Calculate the offset between the ingress and egress error log
   * register sets. This allows us to use the same handler for both
   * types of interrupt, with the offset being used to select the
   * correct register for the current direction. */
  uint32_t diff_addr = offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.dprsrreg.dprsrreg.ho_e[0]
                                    .her.h.mirrtbl_sbe_err_log) -
                       offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.dprsrreg.dprsrreg.ho_i[0]
                                    .hir.h.mirrtbl_sbe_err_log);
  /* Adjust for direction */
  diff_addr = diff_addr * dir;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < 17; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // mirrtbl_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.mirrtbl_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xf;
          /* Report event */
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
          /* Repair memory */
          pipe_mgr_intr_mirr_tbl_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe, err_addr, dir);
          break;

        case 1:  // mirrtbl_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.mirrtbl_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xf;
          /* Report event */
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
          /* Repair memory */
          pipe_mgr_intr_mirr_tbl_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, pipe, err_addr, dir);
          break;

        case 2:  // ipkt_mac0_sbe
        case 4:  // ipkt_mac1_sbe
        case 6:  // ipkt_mac2_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.ipkt_mac0_sbe_err_log) +
                    diff_addr + ((bitpos - 2) * 0x4);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          /* Report event */
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
          /* No further action */
          break;

        case 3:  // ipkt_mac0_mbe
        case 5:  // ipkt_mac1_mbe
        case 7:  // ipkt_mac2_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .hir.h.ipkt_mac0_mbe_err_log) +
                    diff_addr + ((bitpos - 3) * 0x4);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          /* Report event */
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
          /* No further action */
          break;

        case 8:  // cmd_fifo_overrun
        case 9:  // cmd_fifo_underrun
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 10:  // cred_err
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 11:  // ipkt_mac0_overflow
        case 12:  // ipkt_mac0_underflow
        case 13:  // ipkt_mac1_overflow
        case 14:  // ipkt_mac1_underflow
        case 15:  // ipkt_mac2_overflow
        case 16:  // ipkt_mac2_underflow
          /* Report event */
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
          /* Disable interrupt */
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

/*
 * Deparser Output Phase (Ingress, Egress) - intr_0.
 *
 * Decodes:
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_i[].out_ingr.intr_0.stat (ingress)
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_e[].out_egr.intr_0.stat (egress)
 */
static uint32_t pipe_mgr_tof2_dprsr_out_intr0_handle(bf_dev_id_t dev,
                                                     bf_subdev_id_t subdev_id,
                                                     uint32_t intr_address,
                                                     uint32_t intr_status_val,
                                                     uint32_t enable_hi_addr,
                                                     uint32_t enable_lo_addr,
                                                     void *userdata) {
  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  (void)subdev_id;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, dir = 0;

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
  dir = userdata_p->stage;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < 23; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // pktst_ovf
        case 1:  // pktst_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 2:  // metafifo_ovf
        case 3:  // metafifo_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 4:  // pkthdr_ovf
        case 5:  // pkthdr_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // mirrhdr_ovf
        case 7:  // mirrhdr_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:   // datast0_ovf
        case 9:   // datast0_und
        case 12:  // datast1_ovf
        case 13:  // datast1_und
        case 16:  // datast2_ovf
        case 17:  // datast2_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 10:  // pktdatafifo0_ovf
        case 11:  // pktdatafifo0_und
        case 14:  // pktdatafifo1_ovf
        case 15:  // pktdatafifo1_und
        case 18:  // pktdatafifo2_ovf
        case 19:  // pktdatafifo2_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 20:  // arbfifo_ovf
        case 21:  // arbfifo_und
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 22:  // ctl_chan_err
          /* Report event */
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
          /* Disable interrupt */
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

/*
 * Deparser Output Phase (Ingress, Egress) - intr_1.
 *
 * Decodes:
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_i[].out_ingr.intr_1.stat (ingress)
 * - pipes[].pardereg.dprsrreg.dprsrreg.ho_e[].out_egr.intr_1.stat (egress)
 */
static uint32_t pipe_mgr_tof2_dprsr_out_intr1_handle(bf_dev_id_t dev,
                                                     bf_subdev_id_t subdev_id,
                                                     uint32_t intr_address,
                                                     uint32_t intr_status_val,
                                                     uint32_t enable_hi_addr,
                                                     uint32_t enable_lo_addr,
                                                     void *userdata) {
  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  (void)subdev_id;

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0, slice = 0, dir = 0;
  uint32_t err_addr;

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
  dir = userdata_p->stage;
  slice = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("deparser intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  /* Calculate the offset between the ingress and egress error log
   * register sets. This allows us to use the same handler for both
   * types of interrupt, with the offset being used to select the
   * correct register for the current direction. */
  uint32_t diff_addr = offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.dprsrreg.dprsrreg.ho_e[0]
                                    .out_egr.meta_sbe_err_log) -
                       offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.dprsrreg.dprsrreg.ho_i[0]
                                    .out_ingr.meta_sbe_err_log);
  /* Adjust for direction */
  diff_addr = diff_addr * dir;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  for (bitpos = 0; bitpos < 10; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // meta_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.meta_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 1:  // meta_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.meta_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 2:  // pkthdr_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pkthdr_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 3:  // pkthdr_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pkthdr_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 4:  // mirrhdr_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.mirrhdr_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 5:  // mirrhdr_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.mirrhdr_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xff;
          /* Report event */
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
          /* No further action */
          break;

        case 6:  // pktdata_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pktdata_sbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0x1ff;
          /* Report event */
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
          /* No further action */
          break;

        case 7:  // pktdata_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                 .out_ingr.pktdata_mbe_err_log) +
                    diff_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0x1ff;
          /* Report event */
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
          /* No further action */
          break;

        case 8:  // only egress
          if (dir == 0) break;
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                 .out_egr.pktdata_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          /* Report event */
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
          /* No further action */
          break;

        case 9:  // only egress
          if (dir == 0) break;
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                 .out_egr.pktdata_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_addr = data & 0xfff;
          /* Report event */
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

/* Register deparser interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_deparser_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  int slice;

  LOG_TRACE("Pipe-mgr Registering for deparser interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    // dprsr_pbus.intr
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.stat),
        // &pipe_mgr_tof2_dprsr_pbus_intr_handle,
        &pipe_mgr_tof2_dprsr_dummy_intr_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    // inp.icr.intr
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat),
        &pipe_mgr_tof2_dprsr_inp_intr_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    // inp.icr.intr_b
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat),
        // &pipe_mgr_tof2_dprsr_inp_intrb_handle,
        &pipe_mgr_tof2_dprsr_dummy_intr_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    // inpslice[].intr
    for (slice = 0; slice < 4; slice++) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.inpslice[slice]
                       .intr.stat),
          // &pipe_mgr_tof2_dprsr_inpslice_handle,
          &pipe_mgr_tof2_dprsr_dummy_intr_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_i[].hir.h.intr
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .hir.h.intr.stat),
          &pipe_mgr_tof2_dprsr_hdr_intr_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_i[].out_ingr.intr_0
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .out_ingr.intr_0.stat),
          &pipe_mgr_tof2_dprsr_out_intr0_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_i[].out_ingr.intr_1
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                       .out_ingr.intr_1.stat),
          &pipe_mgr_tof2_dprsr_out_intr1_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_e[].her.h.intr
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .her.h.intr.stat),
          &pipe_mgr_tof2_dprsr_hdr_intr_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 1, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_e[].out_egr.intr_0
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .out_egr.intr_0.stat),
          &pipe_mgr_tof2_dprsr_out_intr0_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 1, slice)));
      PIPE_MGR_ASSERT(ret != -1);

      // ho_e[].out_egr.intr_1
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                       .out_egr.intr_1.stat),
          &pipe_mgr_tof2_dprsr_out_intr1_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 1, slice)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

/* Enable/disable deparser interrupts */
pipe_status_t pipe_mgr_tof2_deparser_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                      bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE("Setting deparser Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  // dprsr_pbus.intr
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg,
               pipes[0].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  // inp.icr.intr
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  // inp.icr.intr_b
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg,
               pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  for (int slice = 0; slice < 4; ++slice) {
    // inpslice[].intr
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0].pardereg.dprsrreg.dprsrreg.inpslice[slice].intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_i[].hir.h.intr
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof2_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[slice].hir.h.intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_i[].out_ingr.intr_0
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                     .out_ingr.intr_0.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_i[].out_ingr.intr_1
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                     .out_ingr.intr_1.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_e[].her.h.intr
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof2_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].her.h.intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_e[].out_egr.intr_0
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof2_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.intr_0.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;

    // ho_e[].out_egr.intr_1
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(
            tof2_reg,
            pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.intr_1.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}
