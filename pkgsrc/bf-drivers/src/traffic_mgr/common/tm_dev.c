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


#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"





static bf_tm_dev_hw_funcs_tbl g_dev_hw_fptr_tbl;

bf_status_t bf_tm_set_timestamp_shift(bf_dev_id_t dev, uint8_t shift) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          shift, g_tm_ctx[dev]->timestamp_shift, g_tm_ctx[dev])) {
    return (rc);
  }

  if (g_dev_hw_fptr_tbl.timestamp_shift_wr_fptr) {
    rc = g_dev_hw_fptr_tbl.timestamp_shift_wr_fptr(dev, shift);
  }
  return (rc);
}

bf_status_t bf_tm_get_timestamp_shift(bf_dev_id_t dev, uint8_t *shift) {
  bf_status_t rc = BF_SUCCESS;

  if (g_dev_hw_fptr_tbl.timestamp_shift_rd_fptr) {
    rc = g_dev_hw_fptr_tbl.timestamp_shift_rd_fptr(dev, shift);
  }
  return (rc);
}

bf_status_t bf_tm_set_ddr_train(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  if (g_dev_hw_fptr_tbl.ddr_train_wr_fptr) {
    rc = g_dev_hw_fptr_tbl.ddr_train_wr_fptr(dev);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

#define BF_TM_DEV_HW_FTBL_WR_FUNCS(timestamp_shift, ddr_train_set)         \
  g_dev_hw_fptr_tbl =                                                      \
      (bf_tm_dev_hw_funcs_tbl){.timestamp_shift_wr_fptr = timestamp_shift, \
                               .ddr_train_wr_fptr = ddr_train_set};

#define BF_TM_DEV_HW_FTBL_RD_FUNCS(timestamp_shift) \
  { g_dev_hw_fptr_tbl.timestamp_shift_rd_fptr = timestamp_shift; }

static void bf_tm_dev_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_WR_FUNCS(bf_tm_tofino_set_timestamp_shift, NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_WR_FUNCS(bf_tm_tof2_set_timestamp_shift, NULL);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_WR_FUNCS(bf_tm_tof3_set_timestamp_shift,
                               bf_tm_tof3_ddr_training);
  }





}

static void bf_tm_dev_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_RD_FUNCS(bf_tm_tofino_get_timestamp_shift);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_RD_FUNCS(bf_tm_tof2_get_timestamp_shift);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_DEV_HW_FTBL_RD_FUNCS(bf_tm_tof3_get_timestamp_shift);
  }





}

void bf_tm_dev_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_DEV_HW_FTBL_WR_FUNCS(NULL, NULL);
  BF_TM_DEV_HW_FTBL_RD_FUNCS(NULL);
}

void bf_tm_dev_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_dev_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_dev_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_dev_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_dev_null_hw_ftbl(tm_ctx);
  bf_tm_dev_set_hw_ftbl_rd_funcs(tm_ctx);
}

bf_tm_status_t bf_tm_init_dev(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_dev_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
