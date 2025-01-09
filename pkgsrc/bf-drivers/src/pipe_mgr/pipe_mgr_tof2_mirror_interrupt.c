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

#include <pipe_mgr/pipe_mgr_mirror_intf.h>

#define PIPE_MGR_TOF2_INTR_MIRR_NUM_ERR0 8
#define PIPE_MGR_TOF2_INTR_MIRR_NUM_ERR1 6
#define PIPE_MGR_TOF2_INTR_MIRR_S2P_NUM_ERR 6

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_mirror_s2p_err_handle(bf_dev_id_t dev,
                                                    bf_subdev_id_t subdev_id,
                                                    uint32_t intr_address,
                                                    uint32_t intr_status_val,
                                                    uint32_t enable_hi_addr,
                                                    uint32_t enable_lo_addr,
                                                    void *userdata) {
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
  uint32_t entry;

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

  LOG_TRACE(
      "Mirror ecc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[].pardereg.mirreg.mirror.s2p_regs.intr.stat
  for (bitpos = 0; bitpos < PIPE_MGR_TOF2_INTR_MIRR_S2P_NUM_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // push_err
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 1:  // pop_err
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 2:  // session_sbe_err
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.mirreg.mirror.s2p_regs.s2p_session_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_s2p_regs_s2p_session_sbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;

        case 3:  // session_mbe_err
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.mirreg.mirror.s2p_regs.s2p_session_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_s2p_regs_s2p_session_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;

        case 4:  // s2p_credit_overflow
          /* Report event */
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
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 5:  // s2p_credit_underflow
          /* Report event */
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

static uint32_t pipe_mgr_tof2_mirror_slice_err_handle(bf_dev_id_t dev,
                                                      bf_subdev_id_t subdev_id,
                                                      uint32_t intr_address,
                                                      uint32_t intr_status_val,
                                                      uint32_t enable_hi_addr,
                                                      uint32_t enable_lo_addr,
                                                      void *userdata) {
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
  uint32_t address = 0, data = 0, entry;
  int slice;

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

  /* Get pipe and slice from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  slice = userdata_p->row;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE(
      "Mirror ecc error intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
      dev,
      pipe,
      intr_address,
      intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[].pardereg.mirreg.mirror.slice_regs[].intr.stat
  for (bitpos = 0; bitpos < PIPE_MGR_TOF2_INTR_MIRR_NUM_ERR0; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // idprs_input_sop_eop_mismatch
          /* Report event */
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
          //! @note No further action?
          break;

        case 1:  // edprs_input_sop_eop_mismatch
          /* Report event */
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
          //! @note No further action?
          break;

        case 2:  // session_mem_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .session_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry =
              getp_tof2_mirror_slice_regs_session_mem_sbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;

        case 3:  // session_mem_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .session_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry =
              getp_tof2_mirror_slice_regs_session_mem_mbe_err_log_addr(&data);
          /* Report event */
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
          /* Repair memory */
          bf_mirror_ecc_correct(
              pipe_mgr_ctx->int_ses_hndl, dev, phy_pipe, (entry & 0xff));
          break;

        case 4:  // data_mem_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .data_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_slice_regs_data_mem_sbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 5:  // data_mem_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .data_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_slice_regs_data_mem_mbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 6:  // meta_mem_sbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .meta_mem_sbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_slice_regs_meta_mem_sbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
          break;

        case 7:  // meta_mem_mbe
          /* Decode location */
          address = offsetof(tof2_reg,
                             pipes[phy_pipe]
                                 .pardereg.mirreg.mirror.slice_regs[slice]
                                 .meta_mem_mbe_err_log);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          entry = getp_tof2_mirror_slice_regs_meta_mem_mbe_err_log_addr(&data);
          /* Report event */
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
          //! @note No further action?
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

/* Register mirror interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_mirror_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;
  int slice;

  LOG_TRACE("Pipe-mgr Registering for mirror interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    /* mirror */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.intr.stat),
        &pipe_mgr_tof2_mirror_s2p_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    for (slice = 0; slice < 4; slice++) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.mirreg.mirror.slice_regs[slice]
                       .intr.stat),
          &pipe_mgr_tof2_mirror_slice_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, slice)));
      PIPE_MGR_ASSERT(ret != -1);
    }
  }

  return PIPE_SUCCESS;
}

/* Enable/disable mirror interrupts */
pipe_status_t pipe_mgr_tof2_mirror_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                    bool enable) {
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

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.mirreg.mirror.s2p_regs.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  for (int slice = 0; slice < 4; ++slice) {
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0].pardereg.mirreg.mirror.slice_regs[slice].intr.en0),
        en_val);
    if (ret != PIPE_SUCCESS) return ret;
  }

  return PIPE_SUCCESS;
}
