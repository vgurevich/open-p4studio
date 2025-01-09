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
/* ------- Generated code. Any mofications will be lost ------- */
#ifndef LLD_TOF3_INTERRUPT_STRUCT_INCLUDED
#define LLD_TOF3_INTERRUPT_STRUCT_INCLUDED

#ifndef __KERNEL__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <bf_types/bf_types.h>
#endif
#include <tof3_regs/tof3_reg_drv.h>

  typedef struct lld_blk_lvl_int_s {
    uint32_t status_reg;
    uint32_t enable_hi_reg;
    uint32_t enable_lo_reg;
    uint32_t status_inject_reg;
    uint32_t shadow_mask;
    lld_int_cb cb_fn[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
    void *userdata[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
    uint32_t count[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT][LLD_TOF3_COUNT_NUMB];
    uint32_t count_shown[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT][LLD_TOF3_COUNT_NUMB];
  } lld_tof3_blk_lvl_int_t;

  typedef struct lld_tof3_blk_lvl_int_list_s {
    uint32_t reg_top;
    uint32_t mask;//mask of the input value if reg_top is -1, otherwise mask for reg_top
    bool is_leaf; // true: u.blk_lvl_int, false: u.blk_lvl_list
    union next_lvl_p{
      lld_tof3_blk_lvl_int_t *blk_lvl_int;
      struct lld_tof3_blk_lvl_int_list_s *blk_lvl_list;
    } u;
    uint32_t n;
  } lld_tof3_blk_lvl_int_list_t;

#define STRUCT_LEN(x) (sizeof(x)/sizeof(x[0]))

/* ---2020-08-19--- */
lld_tof3_blk_lvl_int_t HOST__0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.pcie_bar01_regs.pcie_intr.stat),
   offsetof(tof3_reg, device_select.pcie_bar01_regs.pcie_intr.en0),
   offsetof(tof3_reg, device_select.pcie_bar01_regs.pcie_intr.en1),
   offsetof(tof3_reg, device_select.pcie_bar01_regs.pcie_intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.misc_intr.stat),
   offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.misc_intr.en0),
   offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.misc_intr.en1),
   offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.misc_intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.misc_all_regs.gpio_regs.gpio_common_regs.eth_gpio_intr.stat),
   offsetof(tof3_reg, device_select.misc_all_regs.gpio_regs.gpio_common_regs.eth_gpio_intr.en0),
   offsetof(tof3_reg, device_select.misc_all_regs.gpio_regs.gpio_common_regs.eth_gpio_intr.en1),
   offsetof(tof3_reg, device_select.misc_all_regs.gpio_regs.gpio_common_regs.eth_gpio_intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t TBUS_TBC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_stat0),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_0),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_stat1),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en1_0),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en1_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_stat2),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en2_0),
   offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en2_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};
#if 1
lld_tof3_blk_lvl_int_t CBUS_WAC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[0].wac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[0].wac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[0].wac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[0].wac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[1].wac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[1].wac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[1].wac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[1].wac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[2].wac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[2].wac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[2].wac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[2].wac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[3].wac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[3].wac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[3].wac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_wac_top.wac_pipe[3].wac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_CAA_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[0].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[0].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[0].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[0].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[1].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[1].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[1].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[1].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[2].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[2].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[2].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[2].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[3].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[3].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[3].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[3].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[4].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[4].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[4].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[4].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[5].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[5].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[5].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[5].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[6].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[6].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[6].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[6].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[7].qac_reg.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[7].qac_reg.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[7].qac_reg.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_pipe[7].qac_reg.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QAC_8_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_SCH_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_SCH_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_SCH_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_SCH_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_CLC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.stat.stat_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en0.en0_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en1.en1_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.inj.inj_0_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.stat.stat_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en0.en0_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en1.en1_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.inj.inj_1_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.stat.stat_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en0.en0_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.en1.en1_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr.inj.inj_2_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_CLC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.stat.stat_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en0.en0_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en1.en1_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.inj.inj_0_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.stat.stat_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en0.en0_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en1.en1_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.inj.inj_1_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.stat.stat_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en0.en0_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.en1.en1_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[1].intr.inj.inj_2_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_CLC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.stat.stat_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en0.en0_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en1.en1_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.inj.inj_0_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.stat.stat_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en0.en0_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en1.en1_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.inj.inj_1_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.stat.stat_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en0.en0_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.en1.en1_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[2].intr.inj.inj_2_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_CLC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.stat.stat_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en0.en0_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en1.en1_0_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.inj.inj_0_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.stat.stat_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en0.en0_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en1.en1_1_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.inj.inj_1_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.stat.stat_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en0.en0_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.en1.en1_2_3),
   offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[3].intr.inj.inj_2_3),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[4].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[4].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[4].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[4].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[5].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[5].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[5].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[5].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[6].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[6].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[6].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[6].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PEX_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[7].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[7].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[7].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[7].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[4].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[4].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[4].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[4].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[5].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[5].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[5].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[5].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[6].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[6].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[6].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[6].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QLC_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[7].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[7].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[7].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[7].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[4].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[4].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[4].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[4].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[5].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[5].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[5].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[5].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[6].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[6].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[6].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[6].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRC_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[7].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[7].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[7].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc[7].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[4].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[4].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[4].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[4].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[5].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[5].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[5].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[5].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[6].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[6].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[6].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[6].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PRE_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[7].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[7].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[7].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[7].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[4].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[4].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[4].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[4].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[5].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[5].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[5].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[5].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[6].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[6].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[6].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[6].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[7].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[7].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[7].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc[7].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_PSC_8_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QOC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[0].intr.stat),
   -1,
   -1,
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QOC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[1].intr.stat),
   -1,
   -1,
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QOC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[2].intr.stat),
   -1,
   -1,
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_QOC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[3].intr.stat),
   -1,
   -1,
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.stat.stat_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.en0.en0_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.en1.en1_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.inj.inj_0_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.stat.stat_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.en0.en0_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.en1.en1_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].intr.inj.inj_1_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.stat.stat_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.en0.en0_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.en1.en1_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.inj.inj_0_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.stat.stat_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.en0.en0_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.en1.en1_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[1].intr.inj.inj_1_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.stat.stat_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.en0.en0_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.en1.en1_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.inj.inj_0_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.stat.stat_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.en0.en0_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.en1.en1_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[2].intr.inj.inj_1_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.stat.stat_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.en0.en0_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.en1.en1_0_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.inj.inj_0_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.stat.stat_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.en0.en0_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.en1.en1_1_2),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[3].intr.inj.inj_1_2),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_4_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[0].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[0].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[0].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[0].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_5_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[1].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[1].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[1].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[1].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_6_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[2].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[2].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[2].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[2].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t CBUS_MGC_7_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[3].intr.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[3].intr.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[3].intr.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.rmgc[3].intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

#endif // Skip TM_TOP

lld_tof3_blk_lvl_int_t CBUS_CBC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat0),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en0_0),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en0_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat1),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en1_0),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en1_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat0),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en0_2),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en0_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_stat1),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en1_2),
   offsetof(tof3_reg, device_select.cbc.cbc_cbus.intr_en1_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

#define ETH400G_BLK(mac)\
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_app_chnl_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.chnl_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.chnl_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.chnl_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.chnl_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_app_mem_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.mem_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.mem_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.mem_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.mem_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_app_uctrl_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.uctrl_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.uctrl_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.uctrl_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.uctrl_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_app_tv80_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.tv80_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.tv80_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.tv80_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.tv80_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_app_icrc_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.err_icrc_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.err_icrc_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.err_icrc_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_app.err_icrc_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_sys_tx_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_sys.tx_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.tx_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.tx_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.tx_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_sys_rx_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_sys.rx_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.rx_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.rx_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.rx_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_sys_cts_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_sys.cts_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.cts_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.cts_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.cts_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_sys_ptp_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_sys.err_ptp_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.err_ptp_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.err_ptp_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_sys.err_ptp_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_mac_mem_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_mac.mem_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.mem_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.mem_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.mem_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_MAC_##mac##_mac_link_blk_lvl_int[] = { \
  {offsetof(tof3_reg, eth400g[mac].eth400g_mac.link_intr.stat), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.link_intr.en0), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.link_intr.en1), \
   offsetof(tof3_reg, eth400g[mac].eth400g_mac.link_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
};
#define SES_REG_NM(x) serdes##x
#define SERDES_BLK(sds)\
lld_tof3_blk_lvl_int_t MBUS_SERDES_##sds##_anlt_blk_lvl_int[] = { \
  {offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.anlt_intr.stat), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.anlt_intr.en0), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.anlt_intr.en1), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.anlt_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
}; \
lld_tof3_blk_lvl_int_t MBUS_SERDES_##sds##_sds_blk_lvl_int[] = { \
  {offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.sds_intr.stat), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.sds_intr.en0), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.sds_intr.en1), \
   offsetof(tof3_reg, serdes.SES_REG_NM(sds).serdes_glue_regs.sds_intr.inj), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
};
ETH400G_BLK(0)
lld_tof3_blk_lvl_int_t MBUS_SERDES_0_anlt_blk_lvl_int[] = {
  {offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.anlt_intr.stat),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.anlt_intr.en0),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.anlt_intr.en1),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.anlt_intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};
lld_tof3_blk_lvl_int_t MBUS_SERDES_0_sds_blk_lvl_int[] = {
  {offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.sds_intr.stat),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.sds_intr.en0),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.sds_intr.en1),
   offsetof(tof3_reg, serdes.serdes0.serdes4ln_glue_regs.sds_intr.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};
ETH400G_BLK(1)   SERDES_BLK(1)
ETH400G_BLK(2)   SERDES_BLK(2)
ETH400G_BLK(3)   SERDES_BLK(3)
ETH400G_BLK(4)   SERDES_BLK(4)
ETH400G_BLK(5)   SERDES_BLK(5)
ETH400G_BLK(6)   SERDES_BLK(6)
ETH400G_BLK(7)   SERDES_BLK(7)
ETH400G_BLK(8)   SERDES_BLK(8)
ETH400G_BLK(9)   SERDES_BLK(9)
ETH400G_BLK(10) SERDES_BLK(10)
ETH400G_BLK(11) SERDES_BLK(11)
ETH400G_BLK(12) SERDES_BLK(12)
ETH400G_BLK(13) SERDES_BLK(13)
ETH400G_BLK(14) SERDES_BLK(14)
ETH400G_BLK(15) SERDES_BLK(15)
ETH400G_BLK(16) SERDES_BLK(16)
ETH400G_BLK(17) SERDES_BLK(17)
ETH400G_BLK(18) SERDES_BLK(18)
ETH400G_BLK(19) SERDES_BLK(19)
ETH400G_BLK(20) SERDES_BLK(20)
ETH400G_BLK(21) SERDES_BLK(21)
ETH400G_BLK(22) SERDES_BLK(22)
ETH400G_BLK(23) SERDES_BLK(23)
ETH400G_BLK(24) SERDES_BLK(24)
ETH400G_BLK(25) SERDES_BLK(25)
ETH400G_BLK(26) SERDES_BLK(26)
ETH400G_BLK(27) SERDES_BLK(27)
ETH400G_BLK(28) SERDES_BLK(28)
ETH400G_BLK(29) SERDES_BLK(29)
ETH400G_BLK(30) SERDES_BLK(30)
ETH400G_BLK(31) SERDES_BLK(31)
ETH400G_BLK(32) SERDES_BLK(32)

lld_tof3_blk_lvl_int_t MBUS_MBC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.mbc.mbc_mbus.intr_stat),
   offsetof(tof3_reg, device_select.mbc.mbc_mbus.intr_en_0),
   offsetof(tof3_reg, device_select.mbc.mbc_mbus.intr_en_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};


#define PIPE_MAU_BLK_INT(pp,mm)\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_status_mau_cfg),\
   offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_enable0_mau_cfg),\
   offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_enable1_mau_cfg),\
   offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_inject_mau_cfg),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.match.adrdist.intr_status_mau_ad),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.match.adrdist.intr_enable0_mau_ad),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.match.adrdist.intr_enable1_mau_ad),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.match.adrdist.intr_inject_mau_ad),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_2_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[0].stats.intr_status_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[0].stats.intr_enable0_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[0].stats.intr_enable1_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[0].stats.intr_inject_mau_stats_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_3_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_4_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[0].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_5_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[0].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[0].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[0].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[0].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_6_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[0].selector.intr_status_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[0].selector.intr_enable0_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[0].selector.intr_enable1_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[0].selector.intr_inject_mau_selector_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_7_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_8_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[1].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_9_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[1].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[1].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[1].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[1].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_10_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[1].stats.intr_status_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[1].stats.intr_enable0_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[1].stats.intr_enable1_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[1].stats.intr_inject_mau_stats_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_11_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_12_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[2].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_13_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[2].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[2].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[2].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[2].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_14_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[1].selector.intr_status_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[1].selector.intr_enable0_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[1].selector.intr_enable1_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[1].selector.intr_inject_mau_selector_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_15_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_16_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[3].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_17_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[3].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[3].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[3].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[3].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_18_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[2].stats.intr_status_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[2].stats.intr_enable0_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[2].stats.intr_enable1_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[2].stats.intr_inject_mau_stats_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_19_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_20_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[4].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_21_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[4].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[4].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[4].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[4].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_22_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[2].selector.intr_status_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[2].selector.intr_enable0_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[2].selector.intr_enable1_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[2].selector.intr_inject_mau_selector_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_23_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_24_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[5].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_25_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[5].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[5].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[5].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[5].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_26_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[3].stats.intr_status_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[3].stats.intr_enable0_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[3].stats.intr_enable1_mau_stats_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.stats_wrap[3].stats.intr_inject_mau_stats_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_27_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_28_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[6].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_29_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[6].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[6].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[6].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[6].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_30_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[3].selector.intr_status_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[3].selector.intr_enable0_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[3].selector.intr_enable1_mau_selector_alu),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.meter_group[3].selector.intr_inject_mau_selector_alu),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_31_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].i2portctl.intr_status_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].i2portctl.intr_enable0_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].i2portctl.intr_enable1_mau_synth2port),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].i2portctl.intr_inject_mau_synth2port),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_32_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].adrmux.intr_status_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].adrmux.intr_enable0_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].adrmux.intr_enable1_mau_adrmux_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.row[7].adrmux.intr_inject_mau_adrmux_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_33_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[7].intr_status_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[7].intr_enable0_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[7].intr_enable1_mau_unit_ram_row),\
   offsetof(tof3_reg, pipes[pp].mau[mm].rams.array.row[7].intr_inject_mau_unit_ram_row),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_34_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_status_mau_snapshot),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable0_mau_snapshot),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable1_mau_snapshot),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_inject_mau_snapshot),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_35_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_status_mau_imem),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable0_mau_imem),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable1_mau_imem),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_inject_mau_imem),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_36_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_status_mau_gfm_hash),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable0_mau_gfm_hash),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_enable1_mau_gfm_hash),\
   offsetof(tof3_reg, pipes[pp].mau[mm].dp.intr_inject_mau_gfm_hash),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_MAU##mm##_37_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].tcams.intr_status_mau_tcam_array),\
   offsetof(tof3_reg, pipes[pp].mau[mm].tcams.intr_enable0_mau_tcam_array),\
   offsetof(tof3_reg, pipes[pp].mau[mm].tcams.intr_enable1_mau_tcam_array),\
   offsetof(tof3_reg, pipes[pp].mau[mm].tcams.intr_inject_mau_tcam_array),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\

#define PIPE_PRSR_BLK_INT(pp)\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_0_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_1_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_2_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_3_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_4_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_5_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_6_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_7_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_8_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_9_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en0.en0_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en0.en0_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en0.en0_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_10_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_11_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_12_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_13_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_14_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_15_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_16_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_17_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_18_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_19_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_20_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.en0.en0_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.en0.en0_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_21_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_0_4),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_1_4),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_2_4),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_3_4),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_22_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[0].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[0].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[1].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[1].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf100reg.glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf100reg.glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_23_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[0].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[0].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[1].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[1].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf100reg.glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf100reg.glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_24_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[0].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[0].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[1].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[1].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf100reg.glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf100reg.glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_25_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[0].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[0].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[1].glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[1].glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf100reg.glb_group.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf100reg.glb_group.intr_en0),\
   -1,\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_26_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_27_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_28_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pbusreg.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_29_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[0].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_30_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[1].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_31_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[2].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_32_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[3].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_33_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[4].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_34_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[5].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_35_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[6].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_36_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[7].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_37_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_0_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en1.en1_0_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_0_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_1_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en1.en1_1_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_1_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.stat.stat_2_3),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.en1.en1_2_3),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].ipbreg.glb_group.intr_stat.inj.inj_2_3),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ipbprsr4reg[8].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_38_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.ll0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pmergereg.lr0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_39_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.left.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.parbreg.right.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_40_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[0].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_41_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[1].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_42_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[2].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_43_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[3].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_44_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[4].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_45_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[5].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_46_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[6].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_47_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[7].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_48_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.stat.stat_0_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.en1.en1_0_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.inj.inj_0_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.stat.stat_1_2),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.en1.en1_1_2),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].epbreg.glb_group.intr_stat.inj.inj_1_2),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.epbprsr4reg[8].prsr[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_49_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en1.intr_en1_0_4),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en1.intr_en1_1_4),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en1.intr_en1_2_4),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgrreg.pgr_common.intr_en1.intr_en1_3_4),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_50_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[0].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[0].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[1].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf400reg[1].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf100reg.glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[0].ebuf100reg.glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_51_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[0].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[0].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[1].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf400reg[1].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf100reg.glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[1].ebuf100reg.glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_52_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[0].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[0].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[1].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf400reg[1].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf100reg.glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[2].ebuf100reg.glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_53_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[0].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[0].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[1].glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf400reg[1].glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf100reg.glb_group.intr_stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.ebuf900reg[3].ebuf100reg.glb_group.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_54_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.s2preg.reg_1.intr_2.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_PRSR_55_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_0.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.p2sreg.reg_1.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\

#define PIPE_DPRSR_BLK_INT(pp)\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_0_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.stat),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.en1),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_1_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_2_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_3_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_4_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_5_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_6_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_7_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_8_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_9_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_10_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_11_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_12_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_13_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_14_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.en0),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_15_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_pbus.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_16_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_17_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_18_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_19_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_20_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.inpslice[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_21_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_22_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_23_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_24_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_25_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_26_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_27_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_28_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_0.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.intr_1.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_DPRSR_29_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.s2p_regs.intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[0].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[1].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[2].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.en1),\
   -1,\
   offsetof(tof3_reg, pipes[pp].pardereg.mirreg.mirror.slice_regs[3].intr.inj),\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};
#define PIPE_LFLTR_BLK_SH_INTS(pp)\
lld_tof3_blk_lvl_int_t PBUS_pipe##pp##_LFLTR_blk_lvl_int[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.intr_en0),\
   offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
  {offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.pbs_intr_stat),\
   offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.pbs_intr_en0),\
   offsetof(tof3_reg, pipes[pp].pardereg.lfltr_reg.ctrl.pbs_intr_en1),\
   -1,\
   0xffffffff,\
   {{0}},\
   {{0}},\
   {{{0}}},\
   {{{0}}}},\
};

lld_tof3_blk_lvl_int_t PBUS_PBC_0_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat1),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat3),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_1),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t PBUS_PBC_1_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat1),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat3),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_3),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t PBUS_PBC_2_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_4),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_5),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat1),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_4),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_5),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_4),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_5),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat3),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_4),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_5),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_t PBUS_PBC_3_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat0),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_6),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en0_7),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat1),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_6),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en1_7),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat2),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_6),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en2_7),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_stat3),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_6),
   offsetof(tof3_reg, device_select.pbc.pbc_pbus.intr_en3_7),
   -1,
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_list_t CBUS_QAC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x101,
   true,
   .u.blk_lvl_int = CBUS_QAC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x202,
   true,
   .u.blk_lvl_int = CBUS_QAC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x404,
   true,
   .u.blk_lvl_int = CBUS_QAC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x808,
   true,
   .u.blk_lvl_int = CBUS_QAC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x1010,
   true,
   .u.blk_lvl_int = CBUS_QAC_4_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x2020,
   true,
   .u.blk_lvl_int = CBUS_QAC_5_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x4040,
   true,
   .u.blk_lvl_int = CBUS_QAC_6_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x8080,
   true,
   .u.blk_lvl_int = CBUS_QAC_7_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_7_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_intr_stat),
   0x30000,
   true,
   .u.blk_lvl_int = CBUS_QAC_8_blk_lvl_int,
   STRUCT_LEN(CBUS_QAC_8_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_SCH_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr_all),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_SCH_0_blk_lvl_int,
   STRUCT_LEN(CBUS_SCH_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr_all),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_SCH_1_blk_lvl_int,
   STRUCT_LEN(CBUS_SCH_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr_all),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_SCH_2_blk_lvl_int,
   STRUCT_LEN(CBUS_SCH_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_sch_top.sch[0].intr_all),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_SCH_3_blk_lvl_int,
   STRUCT_LEN(CBUS_SCH_3_blk_lvl_int)},
};

#define CBUS_DDRC_Tx_blk(half, grp)\
lld_tof3_blk_lvl_int_t CBUS_DDRC_tx_##half##_##grp##_blk_lvl[] = { \
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][0].rintr_status_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][0].rintr_enable0_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][0].rintr_enable1_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][0].rintr_inject_ddr_lanegrp), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
};
CBUS_DDRC_Tx_blk(0,0)
CBUS_DDRC_Tx_blk(0,1)
CBUS_DDRC_Tx_blk(0,2)
CBUS_DDRC_Tx_blk(0,3)
CBUS_DDRC_Tx_blk(0,4)
CBUS_DDRC_Tx_blk(1,0)
CBUS_DDRC_Tx_blk(1,1)
CBUS_DDRC_Tx_blk(1,2)
CBUS_DDRC_Tx_blk(1,3)
CBUS_DDRC_Tx_blk(1,4)

lld_tof3_blk_lvl_int_list_t CBUS_DDRC_tx_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x00003,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_0_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_0_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x0000c,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_0_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_0_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x00030,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_0_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_0_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x000c0,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_0_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_0_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x00300,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_0_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_0_4_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x00c00,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_1_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_1_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x03000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_1_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_1_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x0c000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_1_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_1_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0x30000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_1_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_1_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[0]),
   0xc0000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_tx_1_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_tx_1_4_blk_lvl)},
};
#define CBUS_DDRC_Rx_blk(half, grp)\
lld_tof3_blk_lvl_int_t CBUS_DDRC_rx_##half##_##grp##_blk_lvl[] = { \
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][1].rintr_status_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][1].rintr_enable0_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][1].rintr_enable1_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][1].rintr_inject_ddr_lanegrp), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
};
CBUS_DDRC_Rx_blk(0,0)
CBUS_DDRC_Rx_blk(0,1)
CBUS_DDRC_Rx_blk(0,2)
CBUS_DDRC_Rx_blk(0,3)
CBUS_DDRC_Rx_blk(0,4)
CBUS_DDRC_Rx_blk(1,0)
CBUS_DDRC_Rx_blk(1,1)
CBUS_DDRC_Rx_blk(1,2)
CBUS_DDRC_Rx_blk(1,3)
CBUS_DDRC_Rx_blk(1,4)
lld_tof3_blk_lvl_int_list_t CBUS_DDRC_rx_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x00003,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_0_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_0_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x0000c,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_0_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_0_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x00030,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_0_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_0_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x000c0,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_0_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_0_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x00300,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_0_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_0_4_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x00c00,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_1_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_1_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x03000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_1_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_1_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x0c000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_1_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_1_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0x30000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_1_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_1_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[1]),
   0xc0000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_rx_1_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_rx_1_4_blk_lvl)},
};
lld_tof3_blk_lvl_int_t CBUS_DDRC_mgd_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd.intr_status_tm_mgd),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd.intr_enable0_tm_mgd),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd.intr_enable1_tm_mgd),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_mgd.intr_inject_tm_mgd),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};
#define CBUS_DDRC_eye_blk(half, grp)\
lld_tof3_blk_lvl_int_t CBUS_DDRC_eye_##half##_##grp##_blk_lvl[] = { \
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][2].rintr_status_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][2].rintr_enable0_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][2].rintr_enable1_ddr_lanegrp), \
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.i_half[half].lanegrp_err[grp][2].rintr_inject_ddr_lanegrp), \
   0xffffffff, \
   {{0}}, \
   {{0}}, \
   {{{0}}}, \
   {{{0}}}}, \
};
CBUS_DDRC_eye_blk(0,0)
CBUS_DDRC_eye_blk(0,1)
CBUS_DDRC_eye_blk(0,2)
CBUS_DDRC_eye_blk(0,3)
CBUS_DDRC_eye_blk(0,4)
CBUS_DDRC_eye_blk(1,0)
CBUS_DDRC_eye_blk(1,1)
CBUS_DDRC_eye_blk(1,2)
CBUS_DDRC_eye_blk(1,3)
CBUS_DDRC_eye_blk(1,4)
lld_tof3_blk_lvl_int_list_t CBUS_DDRC_eye_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x00003,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_0_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_0_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x0000c,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_0_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_0_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x00030,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_0_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_0_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x000c0,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_0_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_0_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x00300,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_0_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_0_4_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x00c00,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_1_0_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_1_0_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x03000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_1_1_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_1_1_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x0c000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_1_2_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_1_2_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0x30000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_1_3_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_1_3_blk_lvl)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_side.intr_decode_rddr_side[2]),
   0xc0000,
   true,
   .u.blk_lvl_int = CBUS_DDRC_eye_1_4_blk_lvl,
   STRUCT_LEN(CBUS_DDRC_eye_1_4_blk_lvl)},
};

lld_tof3_blk_lvl_int_list_t CBUS_DDRC_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_top.intr_decode_ddr_top),
   0x03,
   false,
   .u.blk_lvl_list = CBUS_DDRC_tx_blk_lvl_list,
   STRUCT_LEN(CBUS_DDRC_tx_blk_lvl_list)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_top.intr_decode_ddr_top),
   0x0c,
   false,
   .u.blk_lvl_list = CBUS_DDRC_rx_blk_lvl_list,
   STRUCT_LEN(CBUS_DDRC_rx_blk_lvl_list)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_top.intr_decode_ddr_top),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_DDRC_mgd_blk_lvl_int,
   STRUCT_LEN(CBUS_DDRC_mgd_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_ddr.rddr.intr_top.intr_decode_ddr_top),
   0xc0,
   false,
   .u.blk_lvl_list = CBUS_DDRC_eye_blk_lvl_list,
   STRUCT_LEN(CBUS_DDRC_eye_blk_lvl_list)},
};

lld_tof3_blk_lvl_int_t CBUS_FAB_SHIM_blk_lvl_int[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_ecc.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_ecc.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_ecc.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_ecc.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_parity.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_parity.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_parity.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_ddr_parity.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2wac_ecc.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2wac_ecc.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2wac_ecc.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2wac_ecc.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2qoc_ecc.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2qoc_ecc.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2qoc_ecc.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_s2p2qoc_ecc.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmcrr_ecc.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmcrr_ecc.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmcrr_ecc.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmcrr_ecc.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmpfc_ecc.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmpfc_ecc.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmpfc_ecc.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_p2s2tmpfc_ecc.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
  {offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_afifo_overflow.stat),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_afifo_overflow.en0),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_afifo_overflow.en1),
   offsetof(tof3_reg, device_select.tm_top.tm_ddr_top.tm_fab_shim.intr_afifo_overflow.inj),
   0xffffffff,
   {{0}},
   {{0}},
   {{{0}}},
   {{{0}}}},
};

lld_tof3_blk_lvl_int_list_t CBUS_CLC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr_all),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_CLC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_CLC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr_all),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_CLC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_CLC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr_all),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_CLC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_CLC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_clc_top.clc[0].intr_all),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_CLC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_CLC_3_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_PEX_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_PEX_0_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_PEX_1_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_PEX_2_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_PEX_3_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0x300,
   true,
   .u.blk_lvl_int = CBUS_PEX_4_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0xc00,
   true,
   .u.blk_lvl_int = CBUS_PEX_5_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0x3000,
   true,
   .u.blk_lvl_int = CBUS_PEX_6_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pex_top.pex[0].intr_all),
   0xc000,
   true,
   .u.blk_lvl_int = CBUS_PEX_7_blk_lvl_int,
   STRUCT_LEN(CBUS_PEX_7_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_QLC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_QLC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_QLC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_QLC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_QLC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0x300,
   true,
   .u.blk_lvl_int = CBUS_QLC_4_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0xc00,
   true,
   .u.blk_lvl_int = CBUS_QLC_5_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0x3000,
   true,
   .u.blk_lvl_int = CBUS_QLC_6_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qlc_top.qlc[0].dc_int_stat),
   0xc000,
   true,
   .u.blk_lvl_int = CBUS_QLC_7_blk_lvl_int,
   STRUCT_LEN(CBUS_QLC_7_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_PRC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x10001,
   true,
   .u.blk_lvl_int = CBUS_PRC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x20002,
   true,
   .u.blk_lvl_int = CBUS_PRC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x40004,
   true,
   .u.blk_lvl_int = CBUS_PRC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x80008,
   true,
   .u.blk_lvl_int = CBUS_PRC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x100010,
   true,
   .u.blk_lvl_int = CBUS_PRC_4_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x200020,
   true,
   .u.blk_lvl_int = CBUS_PRC_5_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x400040,
   true,
   .u.blk_lvl_int = CBUS_PRC_6_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_prc_top.prc_common.pipe_int_status),
   0x800080,
   true,
   .u.blk_lvl_int = CBUS_PRC_7_blk_lvl_int,
   STRUCT_LEN(CBUS_PRC_7_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_PRE_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x10001,
   true,
   .u.blk_lvl_int = CBUS_PRE_0_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x20002,
   true,
   .u.blk_lvl_int = CBUS_PRE_1_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x40004,
   true,
   .u.blk_lvl_int = CBUS_PRE_2_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x80008,
   true,
   .u.blk_lvl_int = CBUS_PRE_3_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x100010,
   true,
   .u.blk_lvl_int = CBUS_PRE_4_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x200020,
   true,
   .u.blk_lvl_int = CBUS_PRE_5_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x400040,
   true,
   .u.blk_lvl_int = CBUS_PRE_6_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre_common.pipe_int_status),
   0x800080,
   true,
   .u.blk_lvl_int = CBUS_PRE_7_blk_lvl_int,
   STRUCT_LEN(CBUS_PRE_7_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_PSC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_PSC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_PSC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_PSC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_PSC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0x300,
   true,
   .u.blk_lvl_int = CBUS_PSC_4_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0xc00,
   true,
   .u.blk_lvl_int = CBUS_PSC_5_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0x3000,
   true,
   .u.blk_lvl_int = CBUS_PSC_6_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0xc000,
   true,
   .u.blk_lvl_int = CBUS_PSC_7_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_7_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.dc_int_stat),
   0x30000,
   true,
   .u.blk_lvl_int = CBUS_PSC_8_blk_lvl_int,
   STRUCT_LEN(CBUS_PSC_8_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_QOC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[0].intr_all),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_QOC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_QOC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[0].intr_all),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_QOC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_QOC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[0].intr_all),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_QOC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_QOC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_qoc_top.qoc[0].intr_all),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_QOC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_QOC_3_blk_lvl_int)},
};

lld_tof3_blk_lvl_int_list_t CBUS_MGC_0_blk_lvl_list[] = {
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0x3,
   true,
   .u.blk_lvl_int = CBUS_MGC_0_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_0_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0xc,
   true,
   .u.blk_lvl_int = CBUS_MGC_1_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_1_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0x30,
   true,
   .u.blk_lvl_int = CBUS_MGC_2_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_2_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0xc0,
   true,
   .u.blk_lvl_int = CBUS_MGC_3_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_3_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0x300,
   true,
   .u.blk_lvl_int = CBUS_MGC_4_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_4_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0xc00,
   true,
   .u.blk_lvl_int = CBUS_MGC_5_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_5_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0x3000,
   true,
   .u.blk_lvl_int = CBUS_MGC_6_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_6_blk_lvl_int)},
  {offsetof(tof3_reg, device_select.tm_top.tm_mgc_top.smgc[0].all_rmgc_smgc_ints),
   0xc000,
   true,
   .u.blk_lvl_int = CBUS_MGC_7_blk_lvl_int,
   STRUCT_LEN(CBUS_MGC_7_blk_lvl_int)},
};


#define ETH400G_BLK_LIST(mac)                                     \
lld_tof3_blk_lvl_int_list_t MBUS_MAC_##mac##_blk_lvl_list[] = {   \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x10001,                                                       \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_app_chnl_blk_lvl_int,        \
   STRUCT_LEN(MBUS_MAC_##mac##_app_chnl_blk_lvl_int)},            \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x20002,                                                       \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_app_mem_blk_lvl_int,         \
   STRUCT_LEN(MBUS_MAC_##mac##_app_mem_blk_lvl_int)},             \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x40004,                                                       \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_sys_cts_blk_lvl_int,         \
   STRUCT_LEN(MBUS_MAC_##mac##_sys_cts_blk_lvl_int)},             \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x80008,                                                       \
   true,                                                          \
   .u.blk_lvl_int = MBUS_SERDES_##mac##_anlt_blk_lvl_int,         \
   STRUCT_LEN(MBUS_SERDES_##mac##_anlt_blk_lvl_int)},             \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x100010,                                                      \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_mac_mem_blk_lvl_int,         \
   STRUCT_LEN(MBUS_MAC_##mac##_mac_mem_blk_lvl_int)},             \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x200020,                                                      \
   true,                                                          \
   .u.blk_lvl_int = MBUS_SERDES_##mac##_sds_blk_lvl_int,          \
   STRUCT_LEN(MBUS_SERDES_##mac##_sds_blk_lvl_int)},              \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x400040,                                                      \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_app_tv80_blk_lvl_int,        \
   STRUCT_LEN(MBUS_MAC_##mac##_app_tv80_blk_lvl_int)},            \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x800080,                                                      \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_sys_rx_blk_lvl_int,          \
   STRUCT_LEN(MBUS_MAC_##mac##_sys_rx_blk_lvl_int)},              \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x1000100,                                                     \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_sys_tx_blk_lvl_int,          \
   STRUCT_LEN(MBUS_MAC_##mac##_sys_tx_blk_lvl_int)},              \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x2000200,                                                     \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_app_icrc_blk_lvl_int,        \
   STRUCT_LEN(MBUS_MAC_##mac##_app_icrc_blk_lvl_int)},            \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x4000400,                                                     \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_sys_ptp_blk_lvl_int,         \
   STRUCT_LEN(MBUS_MAC_##mac##_sys_ptp_blk_lvl_int)},             \
  {offsetof(tof3_reg, eth400g[mac].eth400g_app.global_intr_stat), \
   0x8000800,                                                     \
   true,                                                          \
   .u.blk_lvl_int = MBUS_MAC_##mac##_mac_link_blk_lvl_int,        \
   STRUCT_LEN(MBUS_MAC_##mac##_mac_link_blk_lvl_int)},            \
};
ETH400G_BLK_LIST(0)
ETH400G_BLK_LIST(1)
ETH400G_BLK_LIST(2)
ETH400G_BLK_LIST(3)
ETH400G_BLK_LIST(4)
ETH400G_BLK_LIST(5)
ETH400G_BLK_LIST(6)
ETH400G_BLK_LIST(7)
ETH400G_BLK_LIST(8)
ETH400G_BLK_LIST(9)
ETH400G_BLK_LIST(10)
ETH400G_BLK_LIST(11)
ETH400G_BLK_LIST(12)
ETH400G_BLK_LIST(13)
ETH400G_BLK_LIST(14)
ETH400G_BLK_LIST(15)
ETH400G_BLK_LIST(16)
ETH400G_BLK_LIST(17)
ETH400G_BLK_LIST(18)
ETH400G_BLK_LIST(19)
ETH400G_BLK_LIST(20)
ETH400G_BLK_LIST(21)
ETH400G_BLK_LIST(22)
ETH400G_BLK_LIST(23)
ETH400G_BLK_LIST(24)
ETH400G_BLK_LIST(25)
ETH400G_BLK_LIST(26)
ETH400G_BLK_LIST(27)
ETH400G_BLK_LIST(28)
ETH400G_BLK_LIST(29)
ETH400G_BLK_LIST(30)
ETH400G_BLK_LIST(31)
ETH400G_BLK_LIST(32)


#define PIPE_MAU_BLK_INT_LIST(pp,mm)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x3,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_2_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_2_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_3_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_3_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x30,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_4_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_4_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc0,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_5_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_5_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x300,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_6_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_6_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc00,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_7_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_7_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x3000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_8_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_8_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_9_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_9_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x30000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_10_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_10_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc0000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_11_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_11_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x300000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_12_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_12_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc00000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_13_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_13_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x3000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_14_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_14_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_15_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_15_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0x30000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_16_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_16_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[0]),\
   0xc0000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_17_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_17_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x3,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_18_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_18_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_19_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_19_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x30,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_20_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_20_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc0,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_21_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_21_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x300,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_22_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_22_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc00,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_23_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_23_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x3000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_24_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_24_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_25_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_25_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x30000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_26_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_26_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc0000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_27_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_27_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x300000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_28_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_28_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc00000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_29_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_29_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x3000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_30_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_30_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_31_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_31_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0x30000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_32_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_32_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].rams.map_alu.intr_mau_decode_memory_core[1]),\
   0xc0000000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_33_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_33_blk_lvl_int)},\
};\

#define PIPE_PRSR_BLK_INT_LIST(pp)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_0_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_1_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_1_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_2_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_2_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_3_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_3_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_4_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_4_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_5_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_5_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_6_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_6_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x40,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_7_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_7_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x80,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_8_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_8_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x100,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_9_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_9_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x200,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_10_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_10_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status0),\
   0x400,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_11_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_11_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_1_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_12_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_12_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_13_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_13_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_14_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_14_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_15_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_15_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_16_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_16_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_17_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_17_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x40,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_18_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_18_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x80,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_19_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_19_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x100,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_20_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_20_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status0),\
   0x200,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_21_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_21_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_2_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_22_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_22_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_23_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_23_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_24_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_24_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_25_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_25_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_26_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_26_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status0),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_27_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_27_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_3_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_29_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_29_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_30_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_30_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_31_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_31_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_32_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_32_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_33_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_33_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_34_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_34_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x40,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_35_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_35_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x80,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_36_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_36_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x100,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_37_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_37_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x200,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_38_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_38_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring0intr.status1),\
   0x400,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_39_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_39_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_4_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_40_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_40_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_41_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_41_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_42_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_42_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_43_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_43_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_44_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_44_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_45_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_45_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x40,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_46_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_46_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x80,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_47_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_47_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x100,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_48_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_48_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring1intr.status1),\
   0x200,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_49_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_49_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_5_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_50_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_50_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_51_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_51_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_52_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_52_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_53_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_53_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_54_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_54_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.csr_ring2intr.status1),\
   0x20,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_55_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_55_blk_lvl_int)},\
};\

#define PIPE_DPRSR_BLK_INT_LIST(pp)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_0_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_1_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_1_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_2_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_2_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_3_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_3_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_4_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_4_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status0),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_5_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_5_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_1_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_6_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_6_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_7_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_7_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_8_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_8_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_9_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_9_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_2_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_10_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_10_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status0),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_11_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_11_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status0),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_12_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_12_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status0),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_13_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_13_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_3_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring3intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_14_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_14_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_4_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_16_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_16_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_17_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_17_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_18_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_18_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_19_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_19_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring0intr.status1),\
   0x10,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_20_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_20_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_5_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_21_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_21_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_22_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_22_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_23_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_23_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring1intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_24_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_24_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_6_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_25_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_25_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status1),\
   0x2,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_26_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_26_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status1),\
   0x4,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_27_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_27_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring2intr.status1),\
   0x8,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_28_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_28_blk_lvl_int)},\
};\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_7_blk_lvl_list[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.csr_ring3intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_29_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_29_blk_lvl_int)},\
};\

#define PIPE_MAU_BLK_SH_INTS(pp,mm)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_MAU##mm##_sh_ints[] = {\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0x3,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0xc,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0x30,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_0_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0xc0,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_1_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0x300,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_34_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_34_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0xc00,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_35_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_35_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0x3000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_36_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_36_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].mau[mm].cfg_regs.intr_decode_top),\
   0xc000,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_MAU##mm##_37_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_MAU##mm##_37_blk_lvl_int)},\
};\

#define PIPE_PRSR_BLK_SH_INTS(pp)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_PRSR_sh_ints[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_0_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_0_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status0),\
   0x4,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_0_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_0_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status0),\
   0x8,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_1_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_1_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status0),\
   0x10,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_2_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_2_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_PRSR_28_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_28_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status1),\
   0x4,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_3_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_3_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status1),\
   0x8,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_4_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_4_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.pgstnreg.pgluereg.parde_intr.status1),\
   0x10,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_PRSR_5_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_PRSR_5_blk_lvl_list)},\
};\

#define PIPE_DPRSR_BLK_SH_INTS(pp)\
lld_tof3_blk_lvl_int_list_t PBUS_pipe##pp##_DPRSR_sh_ints[] = {\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status0),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_0_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_0_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status0),\
   0x4,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_0_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_0_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status0),\
   0x8,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_1_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_1_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status0),\
   0x10,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_2_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_2_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status0),\
   0x20,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_3_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_3_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status1),\
   0x1,\
   true,\
   .u.blk_lvl_int = PBUS_pipe##pp##_DPRSR_15_blk_lvl_int,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_15_blk_lvl_int)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status1),\
   0x4,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_4_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_4_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status1),\
   0x8,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_5_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_5_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status1),\
   0x10,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_6_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_6_blk_lvl_list)},\
  {offsetof(tof3_reg, pipes[pp].pardereg.dprsrreg.dprsrreg.dprsr_csr_ring.parde_intr.status1),\
   0x20,\
   false,\
   .u.blk_lvl_list = PBUS_pipe##pp##_DPRSR_7_blk_lvl_list,\
   STRUCT_LEN(PBUS_pipe##pp##_DPRSR_7_blk_lvl_list)},\
};\

PIPE_PRSR_BLK_INT(0)

PIPE_PRSR_BLK_INT_LIST(0)

PIPE_PRSR_BLK_SH_INTS(0)

PIPE_DPRSR_BLK_INT(0)

PIPE_DPRSR_BLK_INT_LIST(0)

PIPE_DPRSR_BLK_SH_INTS(0)

PIPE_LFLTR_BLK_SH_INTS(0)

PIPE_MAU_BLK_INT(0,0)

PIPE_MAU_BLK_INT_LIST(0,0)

PIPE_MAU_BLK_SH_INTS(0,0)

PIPE_MAU_BLK_INT(0,1)

PIPE_MAU_BLK_INT_LIST(0,1)

PIPE_MAU_BLK_SH_INTS(0,1)

PIPE_MAU_BLK_INT(0,2)

PIPE_MAU_BLK_INT_LIST(0,2)

PIPE_MAU_BLK_SH_INTS(0,2)

PIPE_MAU_BLK_INT(0,3)

PIPE_MAU_BLK_INT_LIST(0,3)

PIPE_MAU_BLK_SH_INTS(0,3)

PIPE_MAU_BLK_INT(0,4)

PIPE_MAU_BLK_INT_LIST(0,4)

PIPE_MAU_BLK_SH_INTS(0,4)

PIPE_MAU_BLK_INT(0,5)

PIPE_MAU_BLK_INT_LIST(0,5)

PIPE_MAU_BLK_SH_INTS(0,5)

PIPE_MAU_BLK_INT(0,6)

PIPE_MAU_BLK_INT_LIST(0,6)

PIPE_MAU_BLK_SH_INTS(0,6)

PIPE_MAU_BLK_INT(0,7)

PIPE_MAU_BLK_INT_LIST(0,7)

PIPE_MAU_BLK_SH_INTS(0,7)

PIPE_MAU_BLK_INT(0,8)

PIPE_MAU_BLK_INT_LIST(0,8)

PIPE_MAU_BLK_SH_INTS(0,8)

PIPE_MAU_BLK_INT(0,9)

PIPE_MAU_BLK_INT_LIST(0,9)

PIPE_MAU_BLK_SH_INTS(0,9)

PIPE_MAU_BLK_INT(0,10)

PIPE_MAU_BLK_INT_LIST(0,10)

PIPE_MAU_BLK_SH_INTS(0,10)

PIPE_MAU_BLK_INT(0,11)

PIPE_MAU_BLK_INT_LIST(0,11)

PIPE_MAU_BLK_SH_INTS(0,11)

PIPE_MAU_BLK_INT(0,12)

PIPE_MAU_BLK_INT_LIST(0,12)

PIPE_MAU_BLK_SH_INTS(0,12)

PIPE_MAU_BLK_INT(0,13)

PIPE_MAU_BLK_INT_LIST(0,13)

PIPE_MAU_BLK_SH_INTS(0,13)

PIPE_MAU_BLK_INT(0,14)

PIPE_MAU_BLK_INT_LIST(0,14)

PIPE_MAU_BLK_SH_INTS(0,14)

PIPE_MAU_BLK_INT(0,15)

PIPE_MAU_BLK_INT_LIST(0,15)

PIPE_MAU_BLK_SH_INTS(0,15)

PIPE_MAU_BLK_INT(0,16)

PIPE_MAU_BLK_INT_LIST(0,16)

PIPE_MAU_BLK_SH_INTS(0,16)

PIPE_MAU_BLK_INT(0,17)

PIPE_MAU_BLK_INT_LIST(0,17)

PIPE_MAU_BLK_SH_INTS(0,17)

PIPE_MAU_BLK_INT(0,18)

PIPE_MAU_BLK_INT_LIST(0,18)

PIPE_MAU_BLK_SH_INTS(0,18)

PIPE_MAU_BLK_INT(0,19)

PIPE_MAU_BLK_INT_LIST(0,19)

PIPE_MAU_BLK_SH_INTS(0,19)

PIPE_PRSR_BLK_INT(1)

PIPE_PRSR_BLK_INT_LIST(1)

PIPE_PRSR_BLK_SH_INTS(1)

PIPE_DPRSR_BLK_INT(1)

PIPE_DPRSR_BLK_INT_LIST(1)

PIPE_DPRSR_BLK_SH_INTS(1)

PIPE_LFLTR_BLK_SH_INTS(1)

PIPE_MAU_BLK_INT(1,0)

PIPE_MAU_BLK_INT_LIST(1,0)

PIPE_MAU_BLK_SH_INTS(1,0)

PIPE_MAU_BLK_INT(1,1)

PIPE_MAU_BLK_INT_LIST(1,1)

PIPE_MAU_BLK_SH_INTS(1,1)

PIPE_MAU_BLK_INT(1,2)

PIPE_MAU_BLK_INT_LIST(1,2)

PIPE_MAU_BLK_SH_INTS(1,2)

PIPE_MAU_BLK_INT(1,3)

PIPE_MAU_BLK_INT_LIST(1,3)

PIPE_MAU_BLK_SH_INTS(1,3)

PIPE_MAU_BLK_INT(1,4)

PIPE_MAU_BLK_INT_LIST(1,4)

PIPE_MAU_BLK_SH_INTS(1,4)

PIPE_MAU_BLK_INT(1,5)

PIPE_MAU_BLK_INT_LIST(1,5)

PIPE_MAU_BLK_SH_INTS(1,5)

PIPE_MAU_BLK_INT(1,6)

PIPE_MAU_BLK_INT_LIST(1,6)

PIPE_MAU_BLK_SH_INTS(1,6)

PIPE_MAU_BLK_INT(1,7)

PIPE_MAU_BLK_INT_LIST(1,7)

PIPE_MAU_BLK_SH_INTS(1,7)

PIPE_MAU_BLK_INT(1,8)

PIPE_MAU_BLK_INT_LIST(1,8)

PIPE_MAU_BLK_SH_INTS(1,8)

PIPE_MAU_BLK_INT(1,9)

PIPE_MAU_BLK_INT_LIST(1,9)

PIPE_MAU_BLK_SH_INTS(1,9)

PIPE_MAU_BLK_INT(1,10)

PIPE_MAU_BLK_INT_LIST(1,10)

PIPE_MAU_BLK_SH_INTS(1,10)

PIPE_MAU_BLK_INT(1,11)

PIPE_MAU_BLK_INT_LIST(1,11)

PIPE_MAU_BLK_SH_INTS(1,11)

PIPE_MAU_BLK_INT(1,12)

PIPE_MAU_BLK_INT_LIST(1,12)

PIPE_MAU_BLK_SH_INTS(1,12)

PIPE_MAU_BLK_INT(1,13)

PIPE_MAU_BLK_INT_LIST(1,13)

PIPE_MAU_BLK_SH_INTS(1,13)

PIPE_MAU_BLK_INT(1,14)

PIPE_MAU_BLK_INT_LIST(1,14)

PIPE_MAU_BLK_SH_INTS(1,14)

PIPE_MAU_BLK_INT(1,15)

PIPE_MAU_BLK_INT_LIST(1,15)

PIPE_MAU_BLK_SH_INTS(1,15)

PIPE_MAU_BLK_INT(1,16)

PIPE_MAU_BLK_INT_LIST(1,16)

PIPE_MAU_BLK_SH_INTS(1,16)

PIPE_MAU_BLK_INT(1,17)

PIPE_MAU_BLK_INT_LIST(1,17)

PIPE_MAU_BLK_SH_INTS(1,17)

PIPE_MAU_BLK_INT(1,18)

PIPE_MAU_BLK_INT_LIST(1,18)

PIPE_MAU_BLK_SH_INTS(1,18)

PIPE_MAU_BLK_INT(1,19)

PIPE_MAU_BLK_INT_LIST(1,19)

PIPE_MAU_BLK_SH_INTS(1,19)

PIPE_PRSR_BLK_INT(2)

PIPE_PRSR_BLK_INT_LIST(2)

PIPE_PRSR_BLK_SH_INTS(2)

PIPE_DPRSR_BLK_INT(2)

PIPE_DPRSR_BLK_INT_LIST(2)

PIPE_DPRSR_BLK_SH_INTS(2)

PIPE_LFLTR_BLK_SH_INTS(2)

PIPE_MAU_BLK_INT(2,0)

PIPE_MAU_BLK_INT_LIST(2,0)

PIPE_MAU_BLK_SH_INTS(2,0)

PIPE_MAU_BLK_INT(2,1)

PIPE_MAU_BLK_INT_LIST(2,1)

PIPE_MAU_BLK_SH_INTS(2,1)

PIPE_MAU_BLK_INT(2,2)

PIPE_MAU_BLK_INT_LIST(2,2)

PIPE_MAU_BLK_SH_INTS(2,2)

PIPE_MAU_BLK_INT(2,3)

PIPE_MAU_BLK_INT_LIST(2,3)

PIPE_MAU_BLK_SH_INTS(2,3)

PIPE_MAU_BLK_INT(2,4)

PIPE_MAU_BLK_INT_LIST(2,4)

PIPE_MAU_BLK_SH_INTS(2,4)

PIPE_MAU_BLK_INT(2,5)

PIPE_MAU_BLK_INT_LIST(2,5)

PIPE_MAU_BLK_SH_INTS(2,5)

PIPE_MAU_BLK_INT(2,6)

PIPE_MAU_BLK_INT_LIST(2,6)

PIPE_MAU_BLK_SH_INTS(2,6)

PIPE_MAU_BLK_INT(2,7)

PIPE_MAU_BLK_INT_LIST(2,7)

PIPE_MAU_BLK_SH_INTS(2,7)

PIPE_MAU_BLK_INT(2,8)

PIPE_MAU_BLK_INT_LIST(2,8)

PIPE_MAU_BLK_SH_INTS(2,8)

PIPE_MAU_BLK_INT(2,9)

PIPE_MAU_BLK_INT_LIST(2,9)

PIPE_MAU_BLK_SH_INTS(2,9)

PIPE_MAU_BLK_INT(2,10)

PIPE_MAU_BLK_INT_LIST(2,10)

PIPE_MAU_BLK_SH_INTS(2,10)

PIPE_MAU_BLK_INT(2,11)

PIPE_MAU_BLK_INT_LIST(2,11)

PIPE_MAU_BLK_SH_INTS(2,11)

PIPE_MAU_BLK_INT(2,12)

PIPE_MAU_BLK_INT_LIST(2,12)

PIPE_MAU_BLK_SH_INTS(2,12)

PIPE_MAU_BLK_INT(2,13)

PIPE_MAU_BLK_INT_LIST(2,13)

PIPE_MAU_BLK_SH_INTS(2,13)

PIPE_MAU_BLK_INT(2,14)

PIPE_MAU_BLK_INT_LIST(2,14)

PIPE_MAU_BLK_SH_INTS(2,14)

PIPE_MAU_BLK_INT(2,15)

PIPE_MAU_BLK_INT_LIST(2,15)

PIPE_MAU_BLK_SH_INTS(2,15)

PIPE_MAU_BLK_INT(2,16)

PIPE_MAU_BLK_INT_LIST(2,16)

PIPE_MAU_BLK_SH_INTS(2,16)

PIPE_MAU_BLK_INT(2,17)

PIPE_MAU_BLK_INT_LIST(2,17)

PIPE_MAU_BLK_SH_INTS(2,17)

PIPE_MAU_BLK_INT(2,18)

PIPE_MAU_BLK_INT_LIST(2,18)

PIPE_MAU_BLK_SH_INTS(2,18)

PIPE_MAU_BLK_INT(2,19)

PIPE_MAU_BLK_INT_LIST(2,19)

PIPE_MAU_BLK_SH_INTS(2,19)

PIPE_PRSR_BLK_INT(3)

PIPE_PRSR_BLK_INT_LIST(3)

PIPE_PRSR_BLK_SH_INTS(3)

PIPE_DPRSR_BLK_INT(3)

PIPE_DPRSR_BLK_INT_LIST(3)

PIPE_DPRSR_BLK_SH_INTS(3)

PIPE_LFLTR_BLK_SH_INTS(3)

PIPE_MAU_BLK_INT(3,0)

PIPE_MAU_BLK_INT_LIST(3,0)

PIPE_MAU_BLK_SH_INTS(3,0)

PIPE_MAU_BLK_INT(3,1)

PIPE_MAU_BLK_INT_LIST(3,1)

PIPE_MAU_BLK_SH_INTS(3,1)

PIPE_MAU_BLK_INT(3,2)

PIPE_MAU_BLK_INT_LIST(3,2)

PIPE_MAU_BLK_SH_INTS(3,2)

PIPE_MAU_BLK_INT(3,3)

PIPE_MAU_BLK_INT_LIST(3,3)

PIPE_MAU_BLK_SH_INTS(3,3)

PIPE_MAU_BLK_INT(3,4)

PIPE_MAU_BLK_INT_LIST(3,4)

PIPE_MAU_BLK_SH_INTS(3,4)

PIPE_MAU_BLK_INT(3,5)

PIPE_MAU_BLK_INT_LIST(3,5)

PIPE_MAU_BLK_SH_INTS(3,5)

PIPE_MAU_BLK_INT(3,6)

PIPE_MAU_BLK_INT_LIST(3,6)

PIPE_MAU_BLK_SH_INTS(3,6)

PIPE_MAU_BLK_INT(3,7)

PIPE_MAU_BLK_INT_LIST(3,7)

PIPE_MAU_BLK_SH_INTS(3,7)

PIPE_MAU_BLK_INT(3,8)

PIPE_MAU_BLK_INT_LIST(3,8)

PIPE_MAU_BLK_SH_INTS(3,8)

PIPE_MAU_BLK_INT(3,9)

PIPE_MAU_BLK_INT_LIST(3,9)

PIPE_MAU_BLK_SH_INTS(3,9)

PIPE_MAU_BLK_INT(3,10)

PIPE_MAU_BLK_INT_LIST(3,10)

PIPE_MAU_BLK_SH_INTS(3,10)

PIPE_MAU_BLK_INT(3,11)

PIPE_MAU_BLK_INT_LIST(3,11)

PIPE_MAU_BLK_SH_INTS(3,11)

PIPE_MAU_BLK_INT(3,12)

PIPE_MAU_BLK_INT_LIST(3,12)

PIPE_MAU_BLK_SH_INTS(3,12)

PIPE_MAU_BLK_INT(3,13)

PIPE_MAU_BLK_INT_LIST(3,13)

PIPE_MAU_BLK_SH_INTS(3,13)

PIPE_MAU_BLK_INT(3,14)

PIPE_MAU_BLK_INT_LIST(3,14)

PIPE_MAU_BLK_SH_INTS(3,14)

PIPE_MAU_BLK_INT(3,15)

PIPE_MAU_BLK_INT_LIST(3,15)

PIPE_MAU_BLK_SH_INTS(3,15)

PIPE_MAU_BLK_INT(3,16)

PIPE_MAU_BLK_INT_LIST(3,16)

PIPE_MAU_BLK_SH_INTS(3,16)

PIPE_MAU_BLK_INT(3,17)

PIPE_MAU_BLK_INT_LIST(3,17)

PIPE_MAU_BLK_SH_INTS(3,17)

PIPE_MAU_BLK_INT(3,18)

PIPE_MAU_BLK_INT_LIST(3,18)

PIPE_MAU_BLK_SH_INTS(3,18)

PIPE_MAU_BLK_INT(3,19)

PIPE_MAU_BLK_INT_LIST(3,19)

PIPE_MAU_BLK_SH_INTS(3,19)

lld_tof3_blk_lvl_int_list_t Tof3_int_top[16][20] = {
  { /* 0..31 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[0]),
     0x0f3fffff,
     true,
     .u.blk_lvl_int = HOST__0_blk_lvl_int,
     STRUCT_LEN(HOST__0_blk_lvl_int),},
  },

  { /* 32..63 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[1]),
     0x3,
     true,
     .u.blk_lvl_int = TBUS_TBC_0_blk_lvl_int,
     STRUCT_LEN(TBUS_TBC_0_blk_lvl_int),},
  },

  { /* 64..95 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x3,
     true,
     .u.blk_lvl_int = CBUS_WAC_0_blk_lvl_int,
     STRUCT_LEN(CBUS_WAC_0_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc,
     true,
     .u.blk_lvl_int = CBUS_CAA_0_blk_lvl_int,
     STRUCT_LEN(CBUS_CAA_0_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x30,
     false,
     .u.blk_lvl_list = CBUS_QAC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_QAC_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc0,
     false,
     .u.blk_lvl_list = CBUS_SCH_0_blk_lvl_list,
     STRUCT_LEN(CBUS_SCH_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x300,
     false,
     .u.blk_lvl_list = CBUS_DDRC_blk_lvl_list,
     STRUCT_LEN(CBUS_DDRC_blk_lvl_list),},
    /* Note both these TM Fab Shim interrupts, below, and the DDR interrupts,
     * above, use the same mask for top-level status bits 72 and 73. */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x300,
     true,
     .u.blk_lvl_int = CBUS_FAB_SHIM_blk_lvl_int,
     STRUCT_LEN(CBUS_FAB_SHIM_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc00,
     false,
     .u.blk_lvl_list = CBUS_CLC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_CLC_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x3000,
     false,
     .u.blk_lvl_list = CBUS_PEX_0_blk_lvl_list,
     STRUCT_LEN(CBUS_PEX_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc000,
     false,
     .u.blk_lvl_list = CBUS_QLC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_QLC_0_blk_lvl_list),},
#if !defined(DEVICE_IS_EMULATOR)
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x30000,
     false,
     .u.blk_lvl_list = CBUS_PRC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_PRC_0_blk_lvl_list),},
#endif
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc0000,
     false,
     .u.blk_lvl_list = CBUS_PRE_0_blk_lvl_list,
     STRUCT_LEN(CBUS_PRE_0_blk_lvl_list),},
#if !defined(DEVICE_IS_EMULATOR)
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x300000,
     false,
     .u.blk_lvl_list = CBUS_PSC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_PSC_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc00000,
     false,
     .u.blk_lvl_list = CBUS_QOC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_QOC_0_blk_lvl_list),},
#endif
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0x3000000,
     false,
     .u.blk_lvl_list = CBUS_MGC_0_blk_lvl_list,
     STRUCT_LEN(CBUS_MGC_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[2]),
     0xc0000000,
     true,
     .u.blk_lvl_int = CBUS_CBC_0_blk_lvl_int,
     STRUCT_LEN(CBUS_CBC_0_blk_lvl_int),},
  },

  { /* 96..127 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[3]),
     0x3,
     true,
     .u.blk_lvl_int = CBUS_CBC_0_blk_lvl_int,
     STRUCT_LEN(CBUS_CBC_0_blk_lvl_int),},
  },

#if !defined(DEVICE_IS_EMULATOR)
  { /* 128..159 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00000003,
     false,
     .u.blk_lvl_list = MBUS_MAC_0_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_0_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x0000000c,
     false,
     .u.blk_lvl_list = MBUS_MAC_1_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_1_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00000030,
     false,
     .u.blk_lvl_list = MBUS_MAC_2_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_2_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x000000c0,
     false,
     .u.blk_lvl_list = MBUS_MAC_3_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_3_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00000300,
     false,
     .u.blk_lvl_list = MBUS_MAC_4_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_4_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00000c00,
     false,
     .u.blk_lvl_list = MBUS_MAC_5_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_5_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00003000,
     false,
     .u.blk_lvl_list = MBUS_MAC_6_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_6_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x0000c000,
     false,
     .u.blk_lvl_list = MBUS_MAC_7_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_7_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00030000,
     false,
     .u.blk_lvl_list = MBUS_MAC_8_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_8_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x000c0000,
     false,
     .u.blk_lvl_list = MBUS_MAC_9_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_9_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00300000,
     false,
     .u.blk_lvl_list = MBUS_MAC_10_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_10_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x00c00000,
     false,
     .u.blk_lvl_list = MBUS_MAC_11_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_11_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x03000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_12_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_12_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x0c000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_13_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_13_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0x30000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_14_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_14_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[4]),
     0xc0000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_15_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_15_blk_lvl_list),},
  },
  { /* 160..191 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00000003,
     false,
     .u.blk_lvl_list = MBUS_MAC_16_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_16_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x0000000c,
     false,
     .u.blk_lvl_list = MBUS_MAC_17_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_17_blk_lvl_list),},

    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00000030,
     false,
     .u.blk_lvl_list = MBUS_MAC_18_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_18_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x000000c0,
     false,
     .u.blk_lvl_list = MBUS_MAC_19_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_19_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00000300,
     false,
     .u.blk_lvl_list = MBUS_MAC_20_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_20_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00000c00,
     false,
     .u.blk_lvl_list = MBUS_MAC_21_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_21_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00003000,
     false,
     .u.blk_lvl_list = MBUS_MAC_22_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_22_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x0000c000,
     false,
     .u.blk_lvl_list = MBUS_MAC_23_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_23_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00030000,
     false,
     .u.blk_lvl_list = MBUS_MAC_24_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_24_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x000c0000,
     false,
     .u.blk_lvl_list = MBUS_MAC_25_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_25_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00300000,
     false,
     .u.blk_lvl_list = MBUS_MAC_26_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_26_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x00c00000,
     false,
     .u.blk_lvl_list = MBUS_MAC_27_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_27_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x03000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_28_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_28_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x0c000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_29_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_29_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0x30000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_30_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_30_blk_lvl_list),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[5]),
     0xc0000000,
     false,
     .u.blk_lvl_list = MBUS_MAC_31_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_31_blk_lvl_list),},
  },
  { /* 192..223 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[6]),
     0x00000003,
     false,
     .u.blk_lvl_list = MBUS_MAC_32_blk_lvl_list,
     STRUCT_LEN(MBUS_MAC_32_blk_lvl_list),},

    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[6]),
     0x30000,
     true,
     .u.blk_lvl_int = MBUS_MBC_0_blk_lvl_int,
     STRUCT_LEN(MBUS_MBC_0_blk_lvl_int),},
  },
#endif
  { /* 224..255 */
    {0}
  },

  { /* 256..287 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU0_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU0_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU1_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU1_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU2_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU2_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU3_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU3_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x300,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU4_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU4_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc00,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU5_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU5_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x3000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU6_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU6_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU7_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU7_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x30000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU8_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU8_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc0000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU9_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU9_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x300000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU10_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU10_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc00000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU11_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU11_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x3000000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU12_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU12_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc000000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU13_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU13_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0x30000000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU14_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU14_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[8]),
     0xc0000000,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU15_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU15_sh_ints),},
  },

  { /* 288..319 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU16_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU16_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU17_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU17_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU18_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU18_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe0_MAU19_sh_ints,
     STRUCT_LEN(PBUS_pipe0_MAU19_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x100,
     false,
     .u.blk_lvl_list = PBUS_pipe0_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe0_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x200,
     false,
     .u.blk_lvl_list = PBUS_pipe0_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe0_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x400,
     false,
     .u.blk_lvl_list = PBUS_pipe0_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe0_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x800,
     false,
     .u.blk_lvl_list = PBUS_pipe0_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe0_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[9]),
     0x3000,
     true,
     .u.blk_lvl_int = PBUS_pipe0_LFLTR_blk_lvl_int,
     STRUCT_LEN(PBUS_pipe0_LFLTR_blk_lvl_int),},
  },

  { /* 320..351 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU0_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU0_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU1_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU1_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU2_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU2_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU3_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU3_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x300,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU4_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU4_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc00,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU5_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU5_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x3000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU6_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU6_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU7_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU7_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x30000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU8_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU8_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc0000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU9_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU9_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x300000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU10_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU10_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc00000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU11_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU11_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x3000000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU12_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU12_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc000000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU13_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU13_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0x30000000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU14_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU14_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[10]),
     0xc0000000,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU15_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU15_sh_ints),},
  },

  { /* 352..383 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU16_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU16_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU17_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU17_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU18_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU18_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe1_MAU19_sh_ints,
     STRUCT_LEN(PBUS_pipe1_MAU19_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x100,
     false,
     .u.blk_lvl_list = PBUS_pipe1_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe1_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x200,
     false,
     .u.blk_lvl_list = PBUS_pipe1_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe1_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x400,
     false,
     .u.blk_lvl_list = PBUS_pipe1_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe1_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x800,
     false,
     .u.blk_lvl_list = PBUS_pipe1_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe1_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[11]),
     0x3000,
     true,
     .u.blk_lvl_int = PBUS_pipe1_LFLTR_blk_lvl_int,
     STRUCT_LEN(PBUS_pipe1_LFLTR_blk_lvl_int),},
  },

  { /* 384..415 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU0_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU0_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU1_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU1_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU2_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU2_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU3_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU3_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x300,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU4_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU4_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc00,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU5_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU5_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x3000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU6_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU6_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU7_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU7_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x30000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU8_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU8_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc0000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU9_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU9_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x300000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU10_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU10_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc00000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU11_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU11_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x3000000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU12_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU12_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc000000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU13_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU13_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0x30000000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU14_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU14_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[12]),
     0xc0000000,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU15_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU15_sh_ints),},
  },

  { /* 416..447 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU16_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU16_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU17_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU17_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU18_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU18_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe2_MAU19_sh_ints,
     STRUCT_LEN(PBUS_pipe2_MAU19_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x100,
     false,
     .u.blk_lvl_list = PBUS_pipe2_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe2_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x200,
     false,
     .u.blk_lvl_list = PBUS_pipe2_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe2_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x400,
     false,
     .u.blk_lvl_list = PBUS_pipe2_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe2_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x800,
     false,
     .u.blk_lvl_list = PBUS_pipe2_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe2_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[13]),
     0x3000,
     true,
     .u.blk_lvl_int = PBUS_pipe2_LFLTR_blk_lvl_int,
     STRUCT_LEN(PBUS_pipe2_LFLTR_blk_lvl_int),},
  },

  { /* 448..479 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU0_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU0_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU1_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU1_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU2_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU2_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU3_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU3_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x300,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU4_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU4_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc00,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU5_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU5_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x3000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU6_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU6_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU7_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU7_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x30000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU8_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU8_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc0000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU9_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU9_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x300000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU10_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU10_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc00000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU11_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU11_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x3000000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU12_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU12_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc000000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU13_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU13_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0x30000000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU14_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU14_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[14]),
     0xc0000000,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU15_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU15_sh_ints),},
  },

  { /* 480..511 */
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x3,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU16_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU16_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0xc,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU17_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU17_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x30,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU18_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU18_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0xc0,
     false,
     .u.blk_lvl_list = PBUS_pipe3_MAU19_sh_ints,
     STRUCT_LEN(PBUS_pipe3_MAU19_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x100,
     false,
     .u.blk_lvl_list = PBUS_pipe3_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe3_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x200,
     false,
     .u.blk_lvl_list = PBUS_pipe3_PRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe3_PRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x400,
     false,
     .u.blk_lvl_list = PBUS_pipe3_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe3_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x800,
     false,
     .u.blk_lvl_list = PBUS_pipe3_DPRSR_sh_ints,
     STRUCT_LEN(PBUS_pipe3_DPRSR_sh_ints),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x3000,
     true,
     .u.blk_lvl_int = PBUS_pipe3_LFLTR_blk_lvl_int,
     STRUCT_LEN(PBUS_pipe3_LFLTR_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x3000000,
     true,
     .u.blk_lvl_int = PBUS_PBC_0_blk_lvl_int,
     STRUCT_LEN(PBUS_PBC_0_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0xc000000,
     true,
     .u.blk_lvl_int = PBUS_PBC_1_blk_lvl_int,
     STRUCT_LEN(PBUS_PBC_1_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0x30000000,
     true,
     .u.blk_lvl_int = PBUS_PBC_2_blk_lvl_int,
     STRUCT_LEN(PBUS_PBC_2_blk_lvl_int),},
    {offsetof(tof3_reg, device_select.pcie_bar01_regs.shadow_int[15]),
     0xc0000000,
     true,
     .u.blk_lvl_int = PBUS_PBC_3_blk_lvl_int,
     STRUCT_LEN(PBUS_PBC_3_blk_lvl_int),},
  },
};
#endif // LLD_TOF3_INTERRUPT_STRUCT_INCLUDED/* clang-format on */
