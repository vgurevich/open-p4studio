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

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_parde_err_handle(bf_dev_id_t dev,
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
  // pipes[].pardereg.pgstnreg.pbusreg.intr.stat
  // pipes[].pardereg.pgstnreg.pmergereg.ll0.intr.stat
  // pipes[].pardereg.pgstnreg.pmergereg.lr0.intr.stat
  // pipes[].pardereg.pgstnreg.pmergereg.lr1.intr.stat
  // pipes[].pardereg.pgstnreg.parbreg.left.intr.stat
  // pipes[].pardereg.pgstnreg.parbreg.right.intr.stat
  // pipes[].pardereg.pgstnreg.s2preg.intr.stat
  // pipes[].pardereg.pgstnreg.p2sreg.intr.stat
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

static uint32_t pipe_mgr_tof2_ebuf_err_handle(bf_dev_id_t dev,
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
  // pipes[].pardereg.pgstnreg.ebuf900reg[].ebuf100reg.glb_group.intr_stat
  // pipes[].pardereg.pgstnreg.ebuf900reg[].ebuf400reg[].glb_group.intr_stat
  lld_write_register(dev, intr_address, intr_status_val);
  return 0;
}

pipe_status_t pipe_mgr_tof2_register_parde_misc_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for parde interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    /* PG Station PBus */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg, pipes[phy_pipe].pardereg.pgstnreg.pbusreg.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PMERGE */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.ll0.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.lr0.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.pmergereg.lr1.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* PARB */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.parbreg.left.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg,
                 pipes[phy_pipe].pardereg.pgstnreg.parbreg.right.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* S2P */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg, pipes[phy_pipe].pardereg.pgstnreg.s2preg.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* P2S */
    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(tof2_reg, pipes[phy_pipe].pardereg.pgstnreg.p2sreg.intr.stat),
        &pipe_mgr_tof2_parde_err_handle,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    /* EBuf */
    for (int ebuf = 0; ebuf < 4; ++ebuf) {
      ret = lld_int_register_cb(
          dev,
          0,
          offsetof(tof2_reg,
                   pipes[phy_pipe]
                       .pardereg.pgstnreg.ebuf900reg[ebuf]
                       .ebuf100reg.glb_group.intr_stat),
          &pipe_mgr_tof2_ebuf_err_handle,
          (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, ebuf, 0)));
      PIPE_MGR_ASSERT(ret != -1);

      for (int which = 0; which < 2; ++which) {
        ret = lld_int_register_cb(
            dev,
            0,
            offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .pardereg.pgstnreg.ebuf900reg[ebuf]
                         .ebuf400reg[which]
                         .glb_group.intr_stat),
            &pipe_mgr_tof2_ebuf_err_handle,
            (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, ebuf, which)));
        PIPE_MGR_ASSERT(ret != -1);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_parde_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                   bool enable) {
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
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pbusreg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* PMERGE */
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pmergereg.ll0.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pmergereg.lr0.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pmergereg.lr1.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* PARB */
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.left.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.right.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* S2P */
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* P2S */
  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.intr.en0),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  /* EBuf */
  for (int ebuf = 0; ebuf < 4; ++ebuf) {
    ret = pipe_mgr_tof2_intr_reg_wr(
        dev_info,
        &pbm,
        shdl,
        offsetof(tof2_reg,
                 pipes[0]
                     .pardereg.pgstnreg.ebuf900reg[ebuf]
                     .ebuf100reg.glb_group.intr_en0),
        en_val & 0xFFFF03FF);
    if (ret != PIPE_SUCCESS) return ret;

    for (int which = 0; which < 2; ++which) {
      ret = pipe_mgr_tof2_intr_reg_wr(
          dev_info,
          &pbm,
          shdl,
          offsetof(tof2_reg,
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
