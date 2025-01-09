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
#include "pipe_mgr_learn.h"

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_mbc_intr_handle(bf_dev_id_t dev,
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
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

#if 0
static uint32_t pipe_mgr_tof2_mbc_intr_handle(bf_dev_id_t dev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;

  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr, err_addr, i;
  uint32_t parity_err_data[4];
  uint64_t err_addr64, err_data64;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
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
        offsetof(tof2_reg, device_select.mbc.mbc_mbus.mac_0_tx_dr_rd_err_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_status =
        getp_tof2_mbus_mac_0_tx_dr_rd_err_log_mac_tx_dr_rd_err_status(&data);
    head_ptr =
        getp_tof2_mbus_mac_0_tx_dr_rd_err_log_mac_tx_dr_rd_err_head_ptr(&data);
    BF_ERR_EVT(BF_ERR_SEV_FATAL,
               dev_id,
               0,
               0,
               0,
               BF_ERR_TYPE_GENERIC,
               BF_ERR_BLK_DMA,
               BF_ERR_LOC_DMA_MBC,
               "MBC MAC 0 tx dr rd error, err_status %d, err_head_pointer 0x%x",
               err_status,
               head_ptr);
    LOG_TRACE("MBC MAC 0 tx dr rd error, err_status %d, err_head_pointer 0x%x",
              err_status,
              head_ptr);
    en_value |= (0x1u << 9);
  }

  if (intr_status_val & (0x1u << 10)) {
    address =
        offsetof(tof2_reg, device_select.mbc.mbc_mbus.wb_tx_dr_rd_err_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_status =
        getp_tof2_mbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_status(&data);
    head_ptr =
        getp_tof2_mbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_head_ptr(&data);
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
    address = offsetof(tof2_reg, device_select.mbc.mbc_mbus.controller_mbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof2_mbus_controller_mbe_log_queue_addr(&data);
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
    address = offsetof(tof2_reg, device_select.mbc.mbc_mbus.controller_mbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof2_mbus_controller_mbe_log_queue_addr(&data);
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
    address = offsetof(tof2_reg, device_select.mbc.mbc_mbus.controller_sbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof2_mbus_controller_sbe_log_queue_addr(&data);
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
    address = offsetof(tof2_reg, device_select.mbc.mbc_mbus.controller_sbe_log);
    pipe_mgr_interrupt_read_register(dev_id, address, &data);
    err_addr = getp_tof2_mbus_controller_sbe_log_queue_addr(&data);
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
          offsetof(tof2_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data + i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((uint64_t)(parity_err_data[3] & 0x1fff) << 29);
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
        "MBC MBUS parity err0: cycle %d, type %d, sid %d, qid %d, tag %d, addr "
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
          offsetof(tof2_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data + i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((uint64_t)(parity_err_data[3] & 0x1fff) << 29);
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
        "MBC MBUS parity err1: cycle %d, type %d, sid %d, qid %d, tag %d, addr "
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
          offsetof(tof2_reg, device_select.mbc.mbc_mbus.parity_err_log[i]);
      pipe_mgr_interrupt_read_register(dev_id, address, (parity_err_data + i));
    }
    err_addr64 = ((parity_err_data[2] >> 3) & 0x1fffffff) +
                 ((uint64_t)(parity_err_data[3] & 0x1fff) << 29);
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
        "MBC MBUS parity err2: cycle %d, type %d, sid %d, qid %d, tag %d, addr "
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
  pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id,0, enable_hi_addr, en_value);
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}
#endif /* 0 */

static uint32_t pipe_mgr_tof2_tbc0_intr_handle(bf_dev_id_t dev,
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
  // device_select.tbc.tbc_tbus.intr_stat0
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

static uint32_t pipe_mgr_tof2_tbc2_intr_handle(bf_dev_id_t dev,
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
  // device_select.tbc.tbc_tbus.intr_stat2
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

static uint32_t pipe_mgr_tof2_cbc0_intr_handle(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;

  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr;
  int bitpos = 0;

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

  // device_select.cbc.cbc_cbus.intr_stat0

  /* bit 20: lq_rx_dr_full */
  if ((intr_status_val & TOFINO2_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY) &&
      pipe_mgr_flow_lrn_is_int_enabled(dev_id)) {
    /* For TF2LAB-139 keep the host CREQ response weight as 15 and the others as
     * one. */

    /* Disable LQ slots on CBUS which will prevent any filter evictions to SBC
     * and stop any new DMAs.  The filters must be paused because the interrupt
     * is only for empty-to-not-empty transitions in the DR and not the DR being
     * non-empty.  We cannot ensure the DR is left empty, to generate the empty-
     * to-non-empty interrupt on the next learn, without pausing learning and
     * servicing the DR completely. */
    lld_dr_cbus_arb_ctrl_set(dev_id, subdev, 0xF1011111);

    /* Empty the DR. */
    lld_dr_service(dev_id, subdev, lld_dr_rx_learn, 10000);

    /* Since lld_dr_service will only update the view once when first starting
     * processing it is possible that the earlier write to to arb_ctrl will take
     * a moment to apply since it is just posted and there may be DMAs in
     * progress resulting in the DR pointer being updated by the chip just after
     * lld_dr_service updates the view, in this case we'd miss processing those
     * later DR entries.  To avoid this, service the DR one more time which will
     * update the view and process the last few DMAs. */
    lld_dr_service(dev_id, subdev, lld_dr_rx_learn, 10000);

    /* Clear the interrupt status before enabling LQs on the CBUS. */
    lld_write_register(
        dev_id, intr_address, TOFINO2_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY);

    /* Remove this status bit from status_value since we've handled it and
     * cleared the status register.  We don't want to clear this bit again if
     * there are other status bits set and we execute the final
     * lld_write_register call at the end of the function. */
    intr_status_val &= ~TOFINO2_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY;

    /* Restore the default weights. */
    lld_dr_cbus_arb_ctrl_set(dev_id, subdev, 0xF1111111);

    /* If this was the only bit set we are done. */
    if (intr_status_val == TOFINO2_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY)
      return PIPE_SUCCESS;
  }

  // bit 0: host_overflow
  if (intr_status_val & (0x1u)) {
    /* Report event */
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
    /* Update mask */
    en_value |= 0x1u;
  }

  for (bitpos = 21; bitpos < 26; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 21:  // wl0_tx_dr_rd_err
        case 22:  // wl1_tx_dr_rd_err
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.cbc.cbc_cbus.wl_tx_dr_rd_err_log[bitpos - 21]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_cbus_wl_tx_dr_rd_err_log_wl_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_cbus_wl_tx_dr_rd_err_log_wl_tx_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 23:  // rb0_tx_dr_rd_err
        case 24:  // rb1_tx_dr_rd_err
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.cbc.cbc_cbus.rb_tx_dr_rd_err_log[bitpos - 23]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_cbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_cbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 25:  // lq_fm_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.cbc.cbc_cbus.lq_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_cbus_lq_fm_dr_rd_err_log_lq_fm_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_cbus_lq_fm_dr_rd_err_log_lq_fm_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        default:
          break;
      }
    }
  }

  /* Disable specified interrupts */
  if (en_value) {
    pipe_mgr_non_pipe_interrupt_set_enable_val(
        dev_id, 0, enable_hi_addr, en_value);
  }

  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_cbc1_intr_handle(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev;

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
  LOG_TRACE("CBC1 error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev_id,
            intr_address,
            intr_status_val);

  // device_select.cbc.cbc_cbus.intr_stat1
  for (bitpos = 0; bitpos < 13; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // wl_iqueue0_mbe
        case 1:  // wl_iqueue1_mbe
        case 5:  // wl_iqueue0_sbe
        case 6:  // wl_iqueue1_sbe
          name_p = "WL";
          break;

        case 2:  // rb_oqueue0_mbe
        case 3:  // rb_oqueue1_mbe
        case 7:  // rb_oqueue0_sbe
        case 8:  // rb_oqueue1_sbe
          name_p = "RB";
          break;

        case 4:  // lq_oqueue_mbe
        case 9:  // lq_oqueue_sbe
          name_p = "LQ";
          break;

        case 10:  // cbus_parity_err[0]
        case 11:  // cbus_parity_err[1]
        case 12:  // cbus_parity_err[2]
          /* Decode location */
          for (i = 0; i < 7; i++) {
            address = offsetof(tof2_reg,
                               device_select.cbc.cbc_cbus.parity_err_log[i]);
            pipe_mgr_interrupt_read_register(
                dev_id, address, (parity_err_data + i));
          }
          err_addr64 = ((uint64_t)(parity_err_data[5] & 0x1fff) << 29) +
                       ((parity_err_data[4] >> 3) & 0x1fffffff);
          err_data64[0] =
              ((parity_err_data[0] >> 3) & 0x1fffffff) +
              (parity_err_data[1] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61);
          err_data64[1] =
              ((parity_err_data[2] >> 3) & 0x1fffffff) +
              (parity_err_data[3] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[4] & 0x7) << 61);
          /* Report event */
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
          /* Update mask  */
          en_value |= (0x1u << bitpos);
          break;

        default:
          break;
      }

      if (bitpos < 5) {
        /* Multi-bit errors */
        /* Decode location */
        address =
            offsetof(tof2_reg, device_select.cbc.cbc_cbus.controller_mbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof2_cbus_controller_mbe_log_queue_addr(&data);
        que = getp_tof2_cbus_controller_mbe_log_queue(&data);
        /* Report event */
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
        /* Update mask */
        en_value |= (0x1u << bitpos);
      } else if (bitpos < 10) {
        /* Single-bit errors */
        /* Decode location */
        address =
            offsetof(tof2_reg, device_select.cbc.cbc_cbus.controller_sbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof2_cbus_controller_sbe_log_queue_addr(&data);
        que = getp_tof2_cbus_controller_sbe_log_queue(&data);
        /* Report event */
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
        /* Update mask */
        en_value |= (0x1u << bitpos);
      }
    }
  }

  /* Disable specified interrupts */
  pipe_mgr_non_pipe_interrupt_set_enable_val(
      dev_id, 0, enable_hi_addr, en_value);

  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_pbc0_intr_handle(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev;

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

  // device_select.pbc.pbc_pbus.intr_stat0
  // bit 0: host_overflow
  if (intr_status_val & (0x1u)) {
    /* Report event */
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
    /* Disable interrupt */
    pipe_mgr_non_pipe_interrupt_set_enable_val(dev_id, 0, enable_hi_addr, 0x1u);
  }

  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}
static uint32_t pipe_mgr_tof2_pbc2_intr_handle(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev;

  uint32_t en_value = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_status, head_ptr;
  int bitpos = 0;

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

  // device_select.pbc.pbc_pbus.intr_stat2
  for (bitpos = 0; bitpos < 9; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // il_tx_dr_0_rd_err
        case 1:  // il_tx_dr_1_rd_err
        case 2:  // il_tx_dr_2_rd_err
        case 3:  // il_tx_dr_3_rd_err
          /* Decode location */
          address = offsetof(
              tof2_reg, device_select.pbc.pbc_pbus.il_tx_dr_rd_err_log[bitpos]);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_il_tx_dr_rd_err_log_il_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_pbus_il_tx_dr_rd_err_log_il_tx_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 4:  // wb_tx_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.pbc.pbc_pbus.wb_tx_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_pbus_wb_tx_dr_rd_err_log_wb_tx_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 5:  // rb_tx_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.pbc.pbc_pbus.rb_tx_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_status(&data);
          head_ptr =
              getp_tof2_pbus_rb_tx_dr_rd_err_log_rb_tx_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 6:  // stat_fm_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.pbc.pbc_pbus.stat_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_stat_fm_dr_rd_err_log_stat_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof2_pbus_stat_fm_dr_rd_err_log_stat_fm_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 7:  // idle_fm_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.pbc.pbc_pbus.idle_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_idle_fm_dr_rd_err_log_idle_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof2_pbus_idle_fm_dr_rd_err_log_idle_fm_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        case 8:  // diag_fm_dr_rd_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.pbc.pbc_pbus.diag_fm_dr_rd_err_log);
          pipe_mgr_interrupt_read_register(dev_id, address, &data);
          err_status =
              getp_tof2_pbus_diag_fm_dr_rd_err_log_diag_fm_dr_rd_err_status(
                  &data);
          head_ptr =
              getp_tof2_pbus_diag_fm_dr_rd_err_log_diag_fm_dr_rd_err_head_ptr(
                  &data);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        default:
          break;
      }
    }
  }

  /* Disable specified interrupts */
  pipe_mgr_non_pipe_interrupt_set_enable_val(
      dev_id, 0, enable_hi_addr, en_value);

  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_pbc3_intr_handle(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_lo_addr;
  (void)userdata;
  (void)subdev;

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

  // device_select.pbc.pbc_pbus.intr_stat3
  for (bitpos = 0; bitpos < 29; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:   // il_iqueue_0_mbe
        case 13:  // il_iqueue_0_sbe
          name_p = "IL i0";
          break;

        case 1:   // il_iqueue_1_mbe
        case 14:  // il_iqueue_1_sbe
          name_p = "IL i1";
          break;

        case 2:   // il_iqueue_2_mbe
        case 15:  // il_iqueue_2_sbe
          name_p = "IL i2";
          break;

        case 3:   // il_iqueue_3_mbe
        case 16:  // il_iqueue_3_sbe
          name_p = "IL i3";
          break;

        case 4:   // il_oqueue_0_mbe
        case 17:  // il_oqueue_0_sbe
          name_p = "IL o0";
          break;

        case 5:   // il_oqueue_1_mbe
        case 18:  // il_oqueue_1_sbe
          name_p = "IL o1";
          break;

        case 6:   // il_oqueue_2_mbe
        case 19:  // il_oqueue_2_sbe
          name_p = "IL o2";
          break;

        case 7:   // il_oqueue_3_mbe
        case 20:  // il_oqueue_3_sbe
          name_p = "IL o3";
          break;

        case 8:   // wb_iqueue_mbe
        case 21:  // wb_iqueue_sbe
          name_p = "WB i";
          break;

        case 9:   // rb_oqueue_mbe
        case 22:  // rb_oqueue_sbe
          name_p = "RB o";
          break;

        case 10:  // stat_oqueue_mbe
        case 23:  // stat_oqueue_sbe
          name_p = "stat o";
          break;

        case 11:  // idle_oqueue_mbe
        case 24:  // idle_oqueue_sbe
          name_p = "idle";
          break;

        case 12:  // diag_oqueue_mbe
        case 25:  // diag_oqueue_sbe
          name_p = "diag";
          break;

        case 26:  // pbus_parity_err[0]
        case 27:  // pbus_parity_err[1]
        case 28:  // pbus_parity_err[2]
          /* Decode location */
          for (i = 0; i < 7; i++) {
            address = offsetof(tof2_reg,
                               device_select.pbc.pbc_pbus.parity_err_log[i]);
            pipe_mgr_interrupt_read_register(
                dev_id, address, (parity_err_data + i));
          }
          err_addr64 = ((uint64_t)(parity_err_data[5] & 0x1fff) << 29) +
                       ((parity_err_data[4] >> 3) & 0x1fffffff);
          err_data64[0] =
              ((parity_err_data[0] >> 3) & 0x1fffffff) +
              (parity_err_data[1] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[2] & 0x7) << 61);
          err_data64[1] =
              ((parity_err_data[2] >> 3) & 0x1fffffff) +
              (parity_err_data[3] << 29) +
              (uint64_t)((uint64_t)(parity_err_data[4] & 0x7) << 61);
          /* Report event */
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
          /* Update mask */
          en_value |= (0x1u << bitpos);
          break;

        default:
          break;
      }

      if (bitpos < 13) {
        /* Multi-bit errors */
        /* Decode location */
        address =
            offsetof(tof2_reg, device_select.pbc.pbc_pbus.controller_mbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof2_pbus_controller_mbe_log_queue_addr(&data);
        que = getp_tof2_pbus_controller_mbe_log_queue(&data);
        /* Report event */
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
        /* Update mask */
        en_value |= (0x1u << bitpos);
      } else if (bitpos < 26) {
        /* Single-bit errors */
        /* Decode location */
        address =
            offsetof(tof2_reg, device_select.pbc.pbc_pbus.controller_sbe_log);
        pipe_mgr_interrupt_read_register(dev_id, address, &data);
        que_addr = getp_tof2_pbus_controller_sbe_log_queue_addr(&data);
        que = getp_tof2_pbus_controller_sbe_log_queue(&data);
        /* Report event */
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
        /* Update mask */
        en_value |= (0x1u << bitpos);
      }
    }
  }

  /* Disable specified interrupts */
  pipe_mgr_non_pipe_interrupt_set_enable_val(
      dev_id, 0, enable_hi_addr, en_value);

  /* Clear the interrupt status */
  lld_write_register(dev_id, intr_address, intr_status_val);

  return PIPE_SUCCESS;
}

/* Register sbc interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_sbc_interrupt_notifs(bf_dev_id_t dev_id) {
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for SBC interrupts");

  /* MBus */
  if (0) {
    //! @note Currently disabled.
    ret = lld_int_register_cb(
        dev_id,
        0,
        offsetof(tof2_reg, device_select.mbc.mbc_mbus.intr_stat),
        &pipe_mgr_tof2_mbc_intr_handle,
        NULL);
    PIPE_MGR_ASSERT(ret != -1);
  }

  /* PBus */
  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_stat0),
      &pipe_mgr_tof2_pbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* Nothing interesting in pbc_pbus.intr_stat1 */

  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_stat2),
      &pipe_mgr_tof2_pbc2_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_stat3),
      &pipe_mgr_tof2_pbc3_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* CBus */
  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.cbc.cbc_cbus.intr_stat0),
      &pipe_mgr_tof2_cbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.cbc.cbc_cbus.intr_stat1),
      &pipe_mgr_tof2_cbc1_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* TBus */
  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_stat0),
      &pipe_mgr_tof2_tbc0_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  /* Nothing interesting in tbc.tbc_tbus.intr_stat1 */

  ret = lld_int_register_cb(
      dev_id,
      0,
      offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_stat2),
      &pipe_mgr_tof2_tbc2_intr_handle,
      NULL);
  PIPE_MGR_ASSERT(ret != -1);

  return PIPE_SUCCESS;
}

/* Enable/disable sbc interrupts */
pipe_status_t pipe_mgr_tof2_sbc_interrupt_en_set(bf_dev_id_t dev_id,
                                                 bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  LOG_TRACE("Setting SBC Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* MBus */
  //! @note Notification currently disabled.
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.mbc.mbc_mbus.intr_en_0),
                     en_val & 0x7F601);

  /* PBus */
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_en0_0),
                     en_val & 0x1);
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_en2_0),
                     en_val & 0x1FF);
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.pbc.pbc_pbus.intr_en3_0),
                     en_val);

  /* CBus */
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.cbc.cbc_cbus.intr_en0_0),
                     en_val & 0x3E00001);
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.cbc.cbc_cbus.intr_en1_0),
                     en_val);

  /* TBus */
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en0_0),
                     en_val & 0x1FFE0001);
  lld_write_register(dev_id,
                     offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en2_0),
                     en_val);

  return PIPE_SUCCESS;
}
