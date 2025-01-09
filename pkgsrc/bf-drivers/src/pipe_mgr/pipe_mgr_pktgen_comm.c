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


#include <stddef.h>
#include <stdint.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_pktgen_comm.h"

int pg_log_pipe_mask(bf_dev_target_t dev_tgt) {
  int mask = 0;
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    unsigned int a = pipe_mgr_get_num_active_pipes(dev_tgt.device_id);
    mask = (1 << a) - 1;
  } else {
    mask = 1 << dev_tgt.dev_pipe_id;
  }
  return mask;
}

bf_status_t pg_write_one_pipe_reg(pipe_sess_hdl_t sid,
                                  bf_dev_id_t dev,
                                  uint8_t pm,
                                  uint32_t addr,
                                  uint32_t data) {
  pipe_bitmap_t pbm = {{0}};
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }
  uint32_t prsr_stage = ~0;
  lld_err_t lld_sts = lld_sku_get_prsr_stage(dev, &prsr_stage);
  if (LLD_OK != lld_sts) {
    LOG_ERROR("%s:%d Failed to get prsr stage for dev %d, sts %d",
              __func__,
              __LINE__,
              dev,
              lld_sts);
    return PIPE_INVALID_ARG;
  }
  uint32_t pipe_cnt = dev_info->num_active_pipes;
  for (uint32_t i = 0; i < pipe_cnt; ++i) {
    if (pm & (1 << i)) {
      PIPE_BITMAP_SET(&pbm, i);
    }
  }
  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(dev, &instr, addr, data);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(
      &sid, dev_info, &pbm, prsr_stage, (uint8_t *)(&instr), sizeof(instr));
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "Packet Generator update with ilist fails (%s) dev %d pipe-mask %#x",
        pipe_str_err(sts),
        dev,
        pm);
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}
