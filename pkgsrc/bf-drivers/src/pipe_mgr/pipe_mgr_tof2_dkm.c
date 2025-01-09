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
 * @file pipe_mgr_tof2_dkm.c
 * @date
 *
 * Code to implement exact match field key mask at run time.
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_tof2_dkm.h"
#include <tof2_regs/tof2_reg_drv.h>

uint32_t pipe_mgr_tof2_dkm_match_mask_addr_get(
    bf_dev_pipe_t pipe, int stage, int ram_row, int ram_col, int index) {
  uint32_t addr = offsetof(tof2_reg,
                           pipes[pipe]
                               .mau[stage]
                               .rams.array.row[ram_row]
                               .ram[ram_col]
                               .match_mask[index]);
  return addr;
}

uint32_t pipe_mgr_tof2_dkm_galios_field_matrix_addr_get(bf_dev_pipe_t pipe,
                                                        int stage,
                                                        int gfm_row,
                                                        int gfm_col) {
  uint32_t addr = offsetof(
      tof2_reg,
      pipes[pipe].mau[stage].dp.xbar_hash.hash.galois_field_matrix[gfm_row]
                                                                  [gfm_col]);
  return addr;
}
