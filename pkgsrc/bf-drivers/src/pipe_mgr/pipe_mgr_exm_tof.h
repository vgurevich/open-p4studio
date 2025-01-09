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
 * @file pipe_mgr_exm_tof.h
 * @date
 *
 * Code/Definitions relating to Tofino Exact match.
 * Sort of a PD layer to the Exact match driver.
 */

/* Standard header includes */
#include <math.h>

/* Module header includes */

/* Local header includes  */
#include "pipe_mgr_int.h"

static inline vpn_id_t pipe_mgr_exm_tof_get_subword_vpn(
    rmt_virt_addr_t virt_addr) {
  return ((virt_addr >> (int)(log2(TOF_SRAM_UNIT_DEPTH))) &
          ((1 << TOF_EXM_SUBWORD_VPN_BITS) - 1));
}

static inline uint32_t pipe_mgr_exm_tof_get_ram_line(
    rmt_virt_addr_t virt_addr) {
  return (virt_addr & ((1 << (int)log2(TOF_SRAM_UNIT_DEPTH)) - 1));
}
