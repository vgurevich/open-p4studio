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


/**
 * @file lld_tof2_hole_test.c
 * \brief Implements the TOF2 hole test
 *
 */

/**
 * @addtogroup lld-dv-api
 * @{
 * Implements the TOF2 hole test
 */

#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#else
#define bf_sys_assert()
#endif

#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_reg_if.h>
#include "lld.h"
#include "lld_map.h"
#include "lld_log.h"
#include "lld_dev.h"
//#include <lld/lld_sku.h>
#include <tof2_regs/tof2_reg_drv.h>

#include "lld_tof2_mem_holes.h"
#include "lld_tof2_reg_holes.h"

extern bool tof2_pcie_is_pipe_stage_filtered_out(uint32_t addr);
extern bool tof2_u64_is_pipe_stage_filtered_out(uint64_t addr);

// hack
bool log_all = false;

extern void ucli_log(char *fmt, ...);

static void lld_tof2_reg_hole_test_log_err(uint32_t start_of_hole,
                                           uint32_t end_of_hole,
                                           uint32_t failing_addr,
                                           uint32_t failing_data);

static void lld_tof2_mem_hole_test_log_err(uint64_t start_of_hole,
                                           uint64_t end_of_hole,
                                           uint64_t failing_addr,
                                           uint64_t failing_data);

void lld_tof2_reg_hole_test(bf_dev_id_t dev_id,
                            chip_lvl_reg_hole_t *list,
                            uint32_t list_len) {
  uint32_t i, start, end, got;

  for (i = 0; i < list_len; i++) {
    start = list[i].start;
    end = list[i].end & ~0x3;  // mask off low order 2 bits

    if (tof2_pcie_is_pipe_stage_filtered_out(start)) continue;
    if (tof2_pcie_is_pipe_stage_filtered_out(end)) continue;

    // read at start of hole
    lld_read_register(dev_id, start, &got);
    if (got != 0x0BAD0BAD) {
      lld_tof2_reg_hole_test_log_err(start, end, start, got);
    } else if (log_all) {
      lld_tof2_reg_hole_test_log_err(start, end, start, got);
    }

    // if not the same address as start
    if (start != end) {
      // read at end of hole
      lld_read_register(dev_id, end, &got);
      if (got != 0x0BAD0BAD) {
        lld_tof2_reg_hole_test_log_err(start, end, end, got);
      } else if (log_all) {
        lld_tof2_reg_hole_test_log_err(start, end, end, got);
      }
    }
  }
}

void lld_tof2_reg_hole_test_run(bf_dev_id_t dev_id) {
  if (!lld_dev_is_tof2(dev_id)) return;
  lld_tof2_reg_hole_test(
      dev_id,
      chip_lvl_reg_hole,
      sizeof(chip_lvl_reg_hole) / sizeof(chip_lvl_reg_hole[0]));
}

static void lld_tof2_reg_hole_test_log_err(uint32_t start_of_hole,
                                           uint32_t end_of_hole,
                                           uint32_t failing_addr,
                                           uint32_t failing_data) {
  ucli_log("%08x - %08x : %08x : got=%08x : exp=0x0BAD0BAD\n",
           start_of_hole,
           end_of_hole,
           failing_addr,
           failing_data);
}

void lld_tof2_mem_hole_test(bf_dev_id_t dev_id,
                            chip_lvl_mem_hole_t *list,
                            uint32_t list_len) {
  uint32_t i;
  uint64_t start, end, got, data_hi, data_lo;

  for (i = 0; i < list_len; i++) {
    start = list[i].start;
    end = list[i].end & ~0x3ull;  // mask off low order 2 bits

    if (tof2_u64_is_pipe_stage_filtered_out(start)) continue;
    if (tof2_u64_is_pipe_stage_filtered_out(end)) continue;

    // read at start of hole
    lld_ind_read(dev_id, start, &data_hi, &data_lo);
    got = data_lo;
    if (got != 0x0BAD0BAD0BAD0BADull) {
      lld_tof2_mem_hole_test_log_err(start, end, start, got);
    } else if (log_all) {
      lld_tof2_mem_hole_test_log_err(start, end, start, got);
    }

    // if not the same address as start
    if (start != end) {
      // read at end of hole
      lld_ind_read(dev_id, end, &data_hi, &data_lo);
      got = data_lo;
      if (got != 0x0BAD0BAD0BAD0BADull) {
        lld_tof2_mem_hole_test_log_err(start, end, end, got);
      } else if (log_all) {
        lld_tof2_mem_hole_test_log_err(start, end, end, got);
      }
    }
  }
}

void lld_tof2_mem_hole_test_run(bf_dev_id_t dev_id) {
  if (!lld_dev_is_tof2(dev_id)) return;
  lld_tof2_mem_hole_test(
      dev_id,
      chip_lvl_mem_hole,
      sizeof(chip_lvl_mem_hole) / sizeof(chip_lvl_mem_hole[0]));
}

static void lld_tof2_mem_hole_test_log_err(uint64_t start_of_hole,
                                           uint64_t end_of_hole,
                                           uint64_t failing_addr,
                                           uint64_t failing_data) {
  ucli_log("%016" PRIx64 " - %016" PRIx64 " : %016" PRIx64 " : got=%016" PRIx64
           " : exp=0x0BAD0BAD0BAD0BAD\n",
           start_of_hole,
           end_of_hole,
           failing_addr,
           failing_data);
}
