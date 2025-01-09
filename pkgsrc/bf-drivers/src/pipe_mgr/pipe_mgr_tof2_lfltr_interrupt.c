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
 * @file pipe_mgr_tof2_lfltr_interrupt.c
 * @date
 *
 * Tofino2 Learn Filter interrupt handling
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_tof2_interrupt.h"
//#include "pipe_mgr_drv.h"
//#include "pipe_mgr_db.h"
#include <tof2_regs/tof2_reg_drv.h>
//#include <tof2_regs/tof2_mem_drv.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>

// extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_lfltr_err_handle(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev_id,
                                               uint32_t intr_address,
                                               uint32_t intr_status_val,
                                               uint32_t enable_hi_addr,
                                               uint32_t enable_lo_addr,
                                               void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)subdev_id;
  /* Each learn filter block consists of two learn filters.
   * Each learn filter has a bloom filter (BFT) and each bloom filter is
   * implemented with four memories with parity protection.
   * Each learn filter has storage for the learn digests/quanta (LQT) in three
   * memories with ECC protection.
   * There is also one buffer (LBUF), made up of three memories, which pulls the
   * digests from the LQT and passes them over the CBus to SBC. */
  const int num_bfts = 2;
  const int num_bft_mems = 4;
  const int num_lqt = 2;
  const int num_lqt_mems = 3;
  const int num_lbuf_mems = 3;

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

  /* Bits [24:22] Learn Buf Multi-Bit ECC Error */
  for (int i = 0; i < num_lbuf_mems; ++i) {
    int intr_bit = 22 + i;
    if (intr_status_val & (1u << intr_bit)) {
      LOG_ERROR(
          "Learn Filter %d LBuf Memory %d multi-bit ECC error", log_pipe, i);
      BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                 dev,
                 log_pipe,
                 0,
                 i,
                 BF_ERR_TYPE_MULTI_BIT_ECC,
                 BF_ERR_BLK_LFLTR,
                 BF_ERR_LOC_LFLTR_LBUF,
                 "Learn Filter %d LBuf Memory %d multi-bit ECC error",
                 log_pipe,
                 i);
    }
  }

  /* Bits [27:25] Learn Buf Single-Bit ECC Error */
  for (int i = 0; i < num_lbuf_mems; ++i) {
    int intr_bit = 25 + i;
    if (intr_status_val & (1u << intr_bit)) {
      LOG_ERROR(
          "Learn Filter %d LBuf Memory %d single-bit ECC error", log_pipe, i);
      BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                 dev,
                 log_pipe,
                 0,
                 i,
                 BF_ERR_TYPE_SINGLE_BIT_ECC,
                 BF_ERR_BLK_LFLTR,
                 BF_ERR_LOC_LFLTR_LBUF,
                 "Learn Filter %d LBuf Memory %d single-bit ECC error",
                 log_pipe,
                 i);
    }
  }

  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

/* Register Learn Filter interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_lfltr_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for learn filter interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg, device_select.lfltr[phy_pipe].ctrl.intr_stat),
        &pipe_mgr_tof2_lfltr_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Enable/disable Learn Filter interrupts */
pipe_status_t pipe_mgr_tof2_lfltr_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                   bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;

  LOG_TRACE("Setting learn filter Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_status_t rc =
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    if (rc != PIPE_SUCCESS) return rc;
    lld_write_register(
        dev_info->dev_id,
        offsetof(tof2_reg, device_select.lfltr[phy_pipe].ctrl.intr_en0),
        en_val & 0x0FFFFFFC);
  }

  return PIPE_SUCCESS;
}
