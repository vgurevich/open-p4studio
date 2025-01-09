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
#include <tofino_regs/tofino.h>

pipe_status_t pipe_mgr_tof_mc_mgid_grp_addr_get(int mgid_grp,
                                                uint32_t *tbl0_addr,
                                                uint32_t *tbl1_addr) {
  /* The two PV tables are each of size 4k by 32bits.  One entry holds the 4 bit
   * mask for 8 MGIDs.  The 4k entries only cover 32k MGIDs, not the entire 64k
   * space.  The msb of the MGID is ignored when accessing this table.
   * Note that the base address and size of table 1 is incorrect in the
   * generated register files so just use the base of the first table and offset
   * by the size. */
  uint32_t tbl0_base_addr =
      offsetof(Tofino, pipes[0].deparser.hdr.him.hi_pv_table.tbl0[0]);
  uint32_t tbl1_base_addr = tbl0_base_addr + 4 * 4096;
  *tbl0_addr = tbl0_base_addr + 4 * mgid_grp;
  *tbl1_addr = tbl1_base_addr + 4 * mgid_grp;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_mc_copy_to_cpu_pv_addr_get(bf_dev_pipe_t pipe,
                                                      uint32_t *addr) {
  *addr = offsetof(Tofino, pipes[pipe].deparser.hdr.hir.ingr.copy_to_cpu_pv);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_mc_pv_table0_addr_get(bf_dev_pipe_t pipe,
                                                 int mgid_grp,
                                                 uint32_t *addr) {
  *addr =
      offsetof(Tofino, pipes[pipe].deparser.hdr.him.hi_pv_table.tbl0[mgid_grp]);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_mc_yid_tbl_addr_get(uint32_t *addr) {
  *addr = offsetof(Tofino, pipes[0].deparser.hdr.hir.ingr.yid_tbl);
  return PIPE_SUCCESS;
}
