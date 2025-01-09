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

#include <traffic_mgr/traffic_mgr_ecc.h>
#include <mc_mgr/mc_mgr_pipe_intf.h>

#define PIPE_MGR_TOF2_INTR_PRE_MBE_START 11
#define PIPE_MGR_TOF2_INTR_PRE_SBE_START 21
#define PIPE_MGR_TOF2_INTR_TM_WAC_NUM_ERR 17

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/*
 * Used to determine whether the ram_type of the sbe or mbe err_log value
 * matches the specified pipe_intr_pre_ram_type_t type.
 *
 * Tofino2 encodes the ram_type bits differently than Tofino1, so we use
 * this macro instead of PIPE_MGR_INTR_PRE_RAM_TYPE_SET().
 */
#define TOF2_RAM_TYPE_SET(bit_no, type) (type & (2u << (bit_no)))

/* TM Pre error */
static uint32_t pipe_mgr_tof2_tm_pre_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
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

  // device_select.tm_top.tm_pre_top.pre[].intr.stat

  /* Decode location */
  /* Read the mbe errlog reg value */
  address =
      offsetof(tof2_reg, device_select.tm_top.tm_pre_top.pre[phy_pipe].mbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  mbe_ram_type = getp_tof2_pre_mbe_log_ram(&data);
  mbe_addr = getp_tof2_pre_mbe_log_addr(&data);

#if 0
  LOG_TRACE("mbe_log address 0x%x, data 0x%x, ram_type 0x%x, mbe_addr 0x%x",
            address,
            data,
            mbe_ram_type,
            mbe_addr);
#endif

  /* Read the sbe errlog reg value */
  address =
      offsetof(tof2_reg, device_select.tm_top.tm_pre_top.pre[phy_pipe].sbe_log);
  pipe_mgr_interrupt_read_register(dev, address, &data);
  sbe_ram_type = getp_tof2_pre_sbe_log_ram(&data);
  sbe_addr = getp_tof2_pre_sbe_log_addr(&data);

#if 0
  LOG_TRACE("sbe_log address 0x%x, data 0x%x, ram_type 0x%x, sbe_addr 0x%x",
            address,
            data,
            sbe_ram_type,
            sbe_addr);
#endif

  for (bitpos = 0; bitpos < PIPE_MGR_INTR_TM_PRE_NUM_ERR; bitpos++) {
    /* Bits 14-31 are ecc errors */
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case (PIPE_INTR_PRE_RAM_FIFO +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // fifo_mbe (11)
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     mbe_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_PRE,
                     BF_ERR_LOC_TM_PRE_FIFO,
                     "TM PRE FIFO multi bit error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case (PIPE_INTR_PRE_RAM_MIT +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // mit_mbe (12)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_MIT, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_mit(dev, phy_pipe, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT0_BM +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // lit0_bm_mbe (13)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_BM, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_bm(dev, 0, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT1_BM +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // lit1_bm_mbe (14)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_BM, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_bm(dev, 1, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT0_NP +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // lit0_np_mbe (15)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_NP, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_np(dev, 0, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT1_NP +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // lit1_np_mbe (16)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_NP, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_np(dev, 1, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_PMT0 +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // pmt0_mbe (17)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT0, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_pmt(dev, 0, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_PMT1 +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // pmt1_mbe (18)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT1, mbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_pmt(dev, 1, mbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_RDM +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // rdm_mbe (19)
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case (PIPE_INTR_PRE_RAM_FIFO +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // fifo_sbe (21)
          /* Report event */
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
          /* No further action */
          break;

        case (PIPE_INTR_PRE_RAM_MIT +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // mit_sbe (22)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_MIT, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_mit(dev, phy_pipe, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT0_BM +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // lit0_bm_sbe (23)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_BM, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_bm(dev, 0, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT1_BM +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // lit1_bm_sbe (24)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_BM, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_bm(dev, 1, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT0_NP +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // lit0_np_sbe (25)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT0_NP, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_np(dev, 0, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_LIT1_NP +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // lit1_np_sbe (26)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_LIT1_NP, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_lit_np(dev, 1, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_PMT0 +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // pmt0_sbe (27)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT0, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_pmt(dev, 0, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_PMT1 +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // pmt1_sbe (28)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_PMT1, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_pmt(dev, 1, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_RDM +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // rdm_sbe (29)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_RDM, sbe_ram_type)) {
            /* Report event */
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
            /* Repair memory */
            mc_mgr_ecc_correct_rdm(dev, sbe_addr);
          }
          break;

        case (PIPE_INTR_PRE_RAM_FIFO_BANKID +
              PIPE_MGR_TOF2_INTR_PRE_MBE_START):  // fifo_mem_bankid_mbe (20)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_FIFO_BANKID, mbe_ram_type)) {
            /* Report event */
            BF_ERR_EVT(BF_ERR_SEV_FATAL,
                       dev,
                       pipe,
                       0,
                       mbe_addr,
                       BF_ERR_TYPE_MULTI_BIT_ECC,
                       BF_ERR_BLK_TM_PRE,
                       BF_ERR_LOC_TM_PRE_BANKID_MEM,
                       "TM PRE BANKID MEM multi bit error at addr 0x%x",
                       mbe_addr);
            /* Disable interrupt */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
          }
          break;

        case (PIPE_INTR_PRE_RAM_FIFO_BANKID +
              PIPE_MGR_TOF2_INTR_PRE_SBE_START):  // fifo_mem_bankid_sbe (30)
          if (TOF2_RAM_TYPE_SET(PIPE_INTR_PRE_RAM_FIFO_BANKID, sbe_ram_type)) {
            /* Report event */
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
            /* No further action */
          }
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

/* TM Wac error */
static uint32_t pipe_mgr_tof2_tm_wac_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr = 0;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Wac error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // device_select.tm_top.tm_wac_top.wac_pipe[].wac_reg.intr.stat

  for (bitpos = 0; bitpos < PIPE_MGR_TOF2_INTR_TM_WAC_NUM_ERR; bitpos++) {
    /* bits 0-9 15-16 are ecc errors */
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // ppg_mapping_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.ppg_mapping_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_reg_ppg_mapping_table_sbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_wac_ppg_map(dev, phy_pipe, err_mem_addr);
          break;

        case 1:  // ppg_mapping_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.ppg_mapping_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_reg_ppg_mapping_table_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 2:  // drop_cnt_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_reg_drop_cnt_table_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 3:  // drop_cnt_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_reg_drop_cnt_table_mbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 4:  // pfc_vis_sbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PFC_VIS,
                     "TM WAC PFC VIS single bit error");
          /* No further action */
          break;

        case 5:  // pfc_vis_mbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_PFC_VIS,
                     "TM WAC PFC VIS multi bit error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // sch_fcr_sbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_SCH_FCR,
                     "TM WAC SCH FCR single bit error");
          /* No further action */
          break;

        case 7:  // sch_fcr_mbe
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_SCH_FCR,
                     "TM WAC SCH FCR multi bit error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:  // qid_map_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.qid_map_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof2_pipe_reg_qid_map_sbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_wac_qid_map(dev, phy_pipe, err_mem_addr);
          break;

        case 9:  // qid_map_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                                 .wac_reg.qid_map_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof2_pipe_reg_qid_map_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 15:  // wac2qac merr
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_WAC2QAC,
                     "TM WAC to QAC multi bit error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 16:  // wac2qac serr
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_TM_WAC,
                     BF_ERR_LOC_TM_WAC_WAC2QAC,
                     "TM WAC to QAC single bit error");
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

/* TM Qac error */
static uint32_t pipe_mgr_tof2_tm_qac_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr = 0;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Qac error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-9
  // device_select.tm_top.tm_qac_top.qac_pipe[].qac_reg.intr.stat
  for (bitpos = 0; bitpos < 10; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // queue_drop_cnt_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.queue_drop_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_queue_drop_table_sbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 1:  // queue_drop_cnt_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.queue_drop_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_queue_drop_table_mbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 2:  // port_drop_cnt_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.port_drop_cnt_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_port_drop_cnt_table_sbe_err_log_addr(
                  &data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 3:  // port_drop_cnt_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.port_drop_cnt_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_port_drop_cnt_table_mbe_err_log_addr(
                  &data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 4:  // qid_mapping_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qid_mapping_table_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_qid_mapping_table_sbe_err_log_addr(
                  &data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_qac_qid_map(dev, phy_pipe, err_mem_addr);
          break;

        case 5:  // qid_mapping_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qid_mapping_table_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_qid_mapping_table_mbe_err_log_addr(
                  &data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // qac2prc_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qac2prc_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_qac2prc_fifo_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 7:  // qac2prc_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.qac2prc_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_qac2prc_fifo_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:  // prc2psc_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.prc2psc_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_prc2psc_fifo_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 9:  // prc2psc_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                                 .qac_reg.prc2psc_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_pipe_block_reg_prc2psc_fifo_mbe_err_log_addr(&data);
          /* Report event */
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

/* TM Clc error */
static uint32_t pipe_mgr_tof2_tm_clc_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr, err_mem_inst, err_mem_odd, err_mem_ep;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Clc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-17
  // device_select.tm_top.tm_clc_top.clc[].intr.stat
  for (bitpos = 0; bitpos < 18; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // ph_fifo_full_ep0
        case 1:  // ph_fifo_full_ep1
        case 2:  // ph_fifo_full_ep2
        case 3:  // ph_fifo_full_ep3
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 4:  // qac_ph_fifo_full_ep0
        case 5:  // qac_ph_fifo_full_ep1
        case 6:  // qac_ph_fifo_full_ep2
        case 7:  // qac_ph_fifo_full_ep3
          /* Report event */
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
          /* No further action */
          break;

        case 8:   // enfifo_serr_ep0
        case 9:   // enfifo_serr_ep1
        case 10:  // enfifo_serr_ep2
        case 11:  // enfifo_serr_ep3
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_clc_top.clc[phy_pipe]
                                 .enfifo_serr_ep0_log) +
                    (4 * (bitpos - 8));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_clc_pipe_rspec_enfifo_serr_ep0_log_addr(&data);
          err_mem_inst =
              getp_tof2_tm_clc_pipe_rspec_enfifo_serr_ep0_log_inst(&data);
          err_mem_odd =
              getp_tof2_tm_clc_pipe_rspec_enfifo_serr_ep0_log_odd(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 12:  // enfifo_merr_ep0
        case 13:  // enfifo_merr_ep1
        case 14:  // enfifo_merr_ep2
        case 15:  // enfifo_merr_ep3
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_clc_top.clc[phy_pipe]
                                 .enfifo_merr_ep0_log) +
                    (4 * (bitpos - 12));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_clc_pipe_rspec_enfifo_merr_ep0_log_addr(&data);
          err_mem_inst =
              getp_tof2_tm_clc_pipe_rspec_enfifo_merr_ep0_log_inst(&data);
          err_mem_odd =
              getp_tof2_tm_clc_pipe_rspec_enfifo_merr_ep0_log_odd(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 16:  // clc_qac_fifo_serr
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.tm_top.tm_clc_top.clc[phy_pipe].clc_qac_serr_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_clc_pipe_rspec_clc_qac_serr_log_addr(&data);
          err_mem_ep = getp_tof2_tm_clc_pipe_rspec_clc_qac_serr_log_ep(&data);
          err_mem_odd = getp_tof2_tm_clc_pipe_rspec_clc_qac_serr_log_odd(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 17:  // clc_qac_fifo_merr
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.tm_top.tm_clc_top.clc[phy_pipe].clc_qac_merr_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_clc_pipe_rspec_clc_qac_merr_log_addr(&data);
          err_mem_ep = getp_tof2_tm_clc_pipe_rspec_clc_qac_merr_log_ep(&data);
          err_mem_odd = getp_tof2_tm_clc_pipe_rspec_clc_qac_merr_log_odd(&data);
          /* Report event */
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

/* TM Pex error */
static uint32_t pipe_mgr_tof2_tm_pex_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Pex error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-11
  // device_select.tm_top.tm_pex_top.pex[].intr.stat
  for (bitpos = 0; bitpos < 12; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // clm mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_linkmem_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 1:  // clm sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_linkmem_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 2:  // dq ph fifo sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_ph_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_dq_ph_fifo_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 3:  // dq ph fifo mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_ph_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_dq_ph_fifo_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 4:  // dq meta fifo sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_meta_fifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_dq_meta_fifo_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 5:  // dq meta fifo mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .dq_meta_fifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_dq_meta_fifo_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // ph afifo sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .ph_afifo_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_ph_afifo_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 7:  // ph afifo mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .ph_afifo_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_ph_afifo_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:   // ph discard fifo sbe
        case 10:  // ph discard fifo1 sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .discard_ph_fifo0_sbe_err_log) +
                    (4 * (bitpos - 8));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_discard_ph_fifo0_sbe_err_log_addr(
                  &data);
          /* Report event */
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
          /* No further action */
          break;

        case 9:   // ph discard fifo mbe
        case 11:  // ph discard fifo1 mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_pex_top.pex[phy_pipe]
                                 .discard_ph_fifo0_mbe_err_log) +
                    (4 * (bitpos - 9));
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_pex_pipe_rspec_discard_ph_fifo0_mbe_err_log_addr(
                  &data);
          /* Report event */
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

/* TM Qlc error */
static uint32_t pipe_mgr_tof2_tm_qlc_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Qlc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-3
  // device_select.tm_top.tm_qlc_top.qlc[].intr.stat
  for (bitpos = 0; bitpos < 4; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // qlc_intr_stat_qlm_serr
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qlc_top.qlc[phy_pipe]
                                 .linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_qlc_pipe_rspec_linkmem_sbe_err_log_addr(&data);
          /* Report event */
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

        case 1:  // qlc_intr_stat_qlm_merr
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_qlc_top.qlc[phy_pipe]
                                 .linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_qlc_pipe_rspec_linkmem_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 2:  // qlc_intr_stat_sch_popempty
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.tm_top.tm_qlc_top.qlc[phy_pipe].schdeq_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr = getp_tof2_tm_qlc_pipe_rspec_schdeq_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 3:  // qlc_intr_stat_ph_fifo_full
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_TM_QLC,
                     BF_ERR_LOC_TM_QLC_PH_FIFO,
                     "TM QLC Discard FIFO Overflow");
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

/* TM Prc error */
static uint32_t pipe_mgr_tof2_tm_prc_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Prc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-1
  // device_select.tm_top.tm_prc_top.prc[].intr.stat
  for (bitpos = 0; bitpos < 2; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // t3 sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.tm_top.tm_prc_top.prc[phy_pipe].cache_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_prc_pipe_rspec_cache_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 1:  // t3 mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              device_select.tm_top.tm_prc_top.prc[phy_pipe].cache_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_prc_pipe_rspec_cache_mbe_err_log_addr(&data);
          /* Report event */
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

/* TM Psc/psc common error */
static uint32_t pipe_mgr_tof2_tm_psc_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

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
  uint32_t err_mem_addr, err_mem_bankid, err_mem_blkid;
  bool is_pcs;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  is_pcs = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("TM Psc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-1
  // device_select.tm_top.tm_psc_top.psc[].intr.stat
  // device_select.tm_top.tm_psc_top.psc_common.intr.stat
  if (is_pcs == 0) {
    // tm_psc_top.psc
    for (bitpos = 0; bitpos < 2; bitpos++) {
      if (intr_status_val & (0x1u << bitpos)) {
        switch (bitpos) {
          case 0:  // psm_sbe
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc[phy_pipe].psm_sbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof2_tm_psc_pipe_rspec_psm_sbe_err_log_addr(&data);
            err_mem_bankid =
                getp_tof2_tm_psc_pipe_rspec_psm_sbe_err_log_bankid(&data);
            err_mem_blkid =
                getp_tof2_tm_psc_pipe_rspec_psm_sbe_err_log_blkid(&data);
            /* Report event */
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
            /* No further action */
            break;

          case 1:  // psm_mbe
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc[phy_pipe].psm_mbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof2_tm_psc_pipe_rspec_psm_mbe_err_log_addr(&data);
            err_mem_bankid =
                getp_tof2_tm_psc_pipe_rspec_psm_mbe_err_log_bankid(&data);
            err_mem_blkid =
                getp_tof2_tm_psc_pipe_rspec_psm_mbe_err_log_blkid(&data);
            /* Report event */
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
            /* Disable interrupt */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;

          default:
            break;
        }
      }
    }
  } else {
    // tm_psc_top.psc_common
    for (bitpos = 0; bitpos < 5; bitpos++) {
      if (intr_status_val & (0x1u << bitpos)) {
        switch (bitpos) {
          case 1:  // linkmem_sbe
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc_common.linkmem_sbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof2_tm_psc_common_rspec_linkmem_sbe_err_log_addr(&data);
            err_mem_blkid =
                getp_tof2_tm_psc_common_rspec_linkmem_sbe_err_log_blkid(&data);
            /* Report event */
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
            /* No further action */
            break;

          case 2:  // linkmem_mbe
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc_common.linkmem_mbe_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_addr =
                getp_tof2_tm_psc_common_rspec_linkmem_mbe_err_log_addr(&data);
            err_mem_blkid =
                getp_tof2_tm_psc_common_rspec_linkmem_mbe_err_log_blkid(&data);
            /* Report event */
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
            /* Disable interrupt */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;

          case 3:  // overflow_err
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc_common.overflow_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_blkid =
                getp_tof2_tm_psc_common_rspec_overflow_err_log_blkid(&data);
            /* Report event */
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
            /* Disable interrupt */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;

          case 4:  // underflow_err
            /* Decode location */
            address = offsetof(
                tof2_reg,
                device_select.tm_top.tm_psc_top.psc_common.underflow_err_log);
            pipe_mgr_interrupt_read_register(dev, address, &data);
            err_mem_blkid =
                getp_tof2_tm_psc_common_rspec_underflow_err_log_blkid(&data);
            /* Report event */
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
            /* Disable interrupt */
            pipe_mgr_interrupt_set_enable_val(
                dev, 0, enable_hi_addr, 0x1u << bitpos);
            break;

          default:
            break;
        }
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* TM Caa error */
static uint32_t pipe_mgr_tof2_tm_caa_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
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

  int bitpos = 0;
  uint32_t address = 0, data = 0;
  uint32_t err_mem_addr, err_mem_blkid;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  LOG_TRACE("TM caa error intr (dev %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-4
  // device_select.tm_top.tm_caa_top.intr.stat
  for (bitpos = 0; bitpos < 5; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // pktdrop_err
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     0,
                     0,
                     0,
                     BF_ERR_TYPE_PKT_DROP,
                     BF_ERR_BLK_TM_CAA,
                     BF_ERR_LOC_TM_CAA,
                     "TM CAA lost packet error");
          /* No further action */
          break;

        case 1:  // linkmem_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg, device_select.tm_top.tm_caa_top.linkmem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_caa_top_rspec_linkmem_sbe_err_log_addr(&data);
          err_mem_blkid =
              getp_tof2_tm_caa_top_rspec_linkmem_sbe_err_log_blkid(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 2:  // linkmem_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg, device_select.tm_top.tm_caa_top.linkmem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_caa_top_rspec_linkmem_mbe_err_log_addr(&data);
          err_mem_blkid =
              getp_tof2_tm_caa_top_rspec_linkmem_mbe_err_log_blkid(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 3:  // overflow_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_caa_top.overflow_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_blkid =
              getp_tof2_tm_caa_top_rspec_overflow_err_log_blkid(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 4:  // underflow_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_caa_top.underflow_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_blkid =
              getp_tof2_tm_caa_top_rspec_underflow_err_log_blkid(&data);
          /* Report event */
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

/* TM Scha error */
static uint32_t pipe_mgr_tof2_tm_sch_err_handle(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t intr_address,
                                                uint32_t intr_status_val,
                                                uint32_t enable_hi_addr,
                                                uint32_t enable_lo_addr,
                                                void *userdata) {
  (void)enable_lo_addr;
  (void)subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  bf_dev_pipe_t pipe;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t address = 0, data = 0;
  int sch_num;
  bool is_b;
  char ab;
  uint32_t base_addr;
  uint32_t err_mem_addr, err_sram, err_mem_type;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  /* Get fields from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;      // phy_pipe
  sch_num = userdata_p->stage;  // 0 or 1
  is_b = userdata_p->row;       // 0 or 1

  base_addr =
      offsetof(tof2_reg,
               device_select.tm_top.tm_schb_top.sch[sch_num].intr.stat) -
      offsetof(tof2_reg,
               device_select.tm_top.tm_scha_top.sch[sch_num].intr.stat);
  base_addr *= is_b;

  ab = (!is_b) ? 'A' : 'B';

  LOG_TRACE("TM Sch%c error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            ab,
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // bit0-30
  // device_select.tm_top.tm_scha_top.sch[].intr.stat
  // device_select.tm_top.tm_schb_top.sch[].intr.stat
  for (bitpos = 0; bitpos < 31; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // tdm_table_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .tdm_table_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_tdm_table_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 1:  // tdm_table_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .tdm_table_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_tdm_table_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 2:  // upd_wac_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_wac_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_wac_sbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_wac_sbe_err_log_sram(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 3:  // upd_wac_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_wac_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_wac_mbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_wac_mbe_err_log_sram(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 4:  // upd_edprsr_advfc_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_edprsr_advfc_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_advfc_sbe_err_log_addr(
                  &data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_advfc_sbe_err_log_sram(
                  &data);
          /* Report event */
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
          /* No further action */
          break;

        case 5:  // upd_edprsr_advfc_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_edprsr_advfc_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_advfc_mbe_err_log_addr(
                  &data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_advfc_mbe_err_log_sram(
                  &data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // q_minrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_q_minrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 7:  // q_minrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:  // q_excrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_excrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_excrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_excrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_q_excrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 9:  // q_excrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_minrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_minrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 10:  // q_maxrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_maxrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_q_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 11:  // q_maxrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_q_maxrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 12:  // l1_minrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_minrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_minrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_minrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_l1_minrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 13:  // l1_minrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_minrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_minrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_minrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 14:  // l1_excrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_excrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_excrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_excrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_l1_excrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 15:  // l1_excrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_excrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_excrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_excrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 16:  // l1_maxrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_maxrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_l1_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 17:  // l1_maxrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .l1_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_l1_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_l1_maxrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 18:  // p_maxrate_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .p_maxrate_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_p_maxrate_sbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_p_maxrate_sbe_err_log_type(&data);
          /* Report event */
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
          /* Repair memory */
          bf_tm_ecc_correct_sch_p_maxrate(
              dev, pipe, err_mem_type, err_mem_addr);
          break;

        case 19:  // p_maxrate_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .p_maxrate_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_p_maxrate_mbe_err_log_addr(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_p_maxrate_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 20:  // upd_pex0_sbe
        case 22:  // upd_pex1_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_pex0_sbe_err_log) +
                    base_addr + (bitpos - 20) * 4;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_sram(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_sbe_err_log_type(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 21:  // upd_pex0_mbe
        case 23:  // upd_pex1_mbe
          /* Decode memory */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_pex0_mbe_err_log) +
                    base_addr + (bitpos - 21) * 4;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_sram(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_upd_pex0_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 24:  // upd_edprsr_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_edprsr_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_sram(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_sbe_err_log_type(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 25:  // upd_edprsr_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .upd_edprsr_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_addr(&data);
          err_sram =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_sram(&data);
          err_mem_type =
              getp_tof2_tm_sch_pipe_rspec_upd_edprsr_mbe_err_log_type(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 26:  // pex_credit_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .pex_credit_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_pex_credit_err_log_port(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 27:  // pex_mac_credit_err
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .pex_mac_credit_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_pex_mac_credit_err_log_mac_grp(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 28:  // q_watchdog_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_watchdog_sbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_watchdog_sbe_err_log_addr(&data);
          /* Report event */
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
          /* No further action */
          break;

        case 29:  // q_watchdog_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             device_select.tm_top.tm_scha_top.sch[sch_num]
                                 .q_watchdog_mbe_err_log) +
                    base_addr;
          pipe_mgr_interrupt_read_register(dev, address, &data);
          err_mem_addr =
              getp_tof2_tm_sch_pipe_rspec_q_watchdog_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 30:  // q_watchdog
          /* Report event */
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

/* Register TM interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_tm_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for TM interrupts");

  /* CAA Common */
  ret = lld_int_register_cb(
      dev,
      0,
      offsetof(tof2_reg, device_select.tm_top.tm_caa_top.intr.stat),
      &pipe_mgr_tof2_tm_caa_err_handle,
      (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 0)));
  PIPE_MGR_ASSERT(ret != -1);

  /* PSC Common */
  ret = lld_int_register_cb(
      dev,
      0,
      offsetof(tof2_reg, device_select.tm_top.tm_psc_top.psc_common.intr.stat),
      &pipe_mgr_tof2_tm_psc_err_handle,
      (void *)&(PIPE_INTR_CALLBACK_DATA(dev, 0, 0, 1)));
  PIPE_MGR_ASSERT(ret != -1);

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* WAC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                     .wac_reg.intr.stat),
        &pipe_mgr_tof2_tm_wac_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* QAC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                     .qac_reg.intr.stat),
        &pipe_mgr_tof2_tm_qac_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* SCH-A/B */
    if (phy_pipe == 0) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[0].intr.stat),
          &pipe_mgr_tof2_tm_sch_err_handle,
          // arguments are dev:dev, pipe:phy_pipe, stage:sch_num, row:is_b
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, phy_pipe, 0, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 1) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[1].intr.stat),
          &pipe_mgr_tof2_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, phy_pipe, 1, 0)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 2) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[0].intr.stat),
          &pipe_mgr_tof2_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, phy_pipe, 0, 1)));
      PIPE_MGR_ASSERT(ret != -1);
    } else if (phy_pipe == 3) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[1].intr.stat),
          &pipe_mgr_tof2_tm_sch_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, phy_pipe, 1, 1)));
      PIPE_MGR_ASSERT(ret != -1);
    }

    /* CLC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_clc_top.clc[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_clc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PEX */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pex_top.pex[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_pex_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* QLC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_qlc_top.qlc[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_qlc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PRC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_prc_top.prc[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_prc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PRE */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pre_top.pre[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_pre_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PSC */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_psc_top.psc[phy_pipe].intr.stat),
        &pipe_mgr_tof2_tm_psc_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Enable/disable TM interrupts */
pipe_status_t pipe_mgr_tof2_tm_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;

  LOG_TRACE("Setting TM Interrupt mode to %s ", enable ? "Enable" : "Disable");

  /* CAA Common */
  lld_write_register(
      dev_info->dev_id,
      offsetof(tof2_reg, device_select.tm_top.tm_caa_top.intr.en0),
      en_val);

  /* PSC Common */
  lld_write_register(
      dev_info->dev_id,
      offsetof(tof2_reg, device_select.tm_top.tm_psc_top.psc_common.intr.en0),
      en_val);

  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* WAC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_wac_top.wac_pipe[phy_pipe]
                     .wac_reg.intr.en0),
        en_val & 0xFFFFFBFF);  // disable debug_sts (10)

    /* QAC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_qac_top.qac_pipe[phy_pipe]
                     .qac_reg.intr.en0),
        en_val & 0x3FF);

    /* SCH-A/B */
    if (phy_pipe == 0) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[0].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 1) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof2_reg, device_select.tm_top.tm_scha_top.sch[1].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 2) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[0].intr.en0),
          en_val & 0x3FFFFFFF);
    } else if (phy_pipe == 3) {
      lld_write_register(
          dev_info->dev_id,
          offsetof(tof2_reg, device_select.tm_top.tm_schb_top.sch[1].intr.en0),
          en_val & 0x3FFFFFFF);
    }

    /* CLC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_clc_top.clc[phy_pipe].intr.en0),
        en_val);

    /* PEX */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pex_top.pex[phy_pipe].intr.en0),
        en_val);

    /* QLC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_qlc_top.qlc[phy_pipe].intr.en0),
        en_val);

    /* PRC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_prc_top.prc[phy_pipe].intr.en0),
        en_val);

    /* PRE */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_pre_top.pre[phy_pipe].intr.en0),
        en_val & 0xFFFFFCF0);

    /* PSC */
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg,
                 device_select.tm_top.tm_psc_top.psc[phy_pipe].intr.en0),
        en_val);
  }

  return PIPE_SUCCESS;
}
