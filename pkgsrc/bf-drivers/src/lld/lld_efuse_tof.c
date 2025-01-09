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


#ifndef __KERNEL__
#include <inttypes.h>  // for PRIx64
#else
#include <bf_types/bf_kernel_types.h>
#endif

#include <bf_types/bf_types.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_bits.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include "lld.h"
#include "lld_log.h"
#include "lld_map.h"
#include "lld_efuse_tof.h"
#include <tofino_regs/tofino.h>

#define EFUSE_WORDS_TOF 8






















































































































































































































int lld_efuse_tof_load(bf_dev_id_t dev_id, bf_dev_init_mode_t warm_init_mode) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  int i;
  uint64_t hi64, lo64;
  uint32_t efuse_data_raw[EFUSE_WORDS_TOF];
  bool is_sw_model = false;
  (void)is_sw_model;
  (void)warm_init_mode;

  if (dev_p == NULL) return -1;











  // read efuse data
  for (i = 0; i < EFUSE_WORDS_TOF; i++) {
    int rc;

    rc = lld_read_register(
        dev_id,
        tofino_device_select_misc_regs_func_fuse_address + (4 * i),
        &efuse_data_raw[i]);
    if (rc) {
      return -1;
    }
  }

  // parse efuse bits into internal representation

  /* First parse the fields in the lower 128b */
  lo64 = efuse_data_raw[1];
  lo64 = (lo64 << 32) | efuse_data_raw[0];
  hi64 = efuse_data_raw[3];
  hi64 = (hi64 << 32) | efuse_data_raw[2];

  dev_p->efuse_data.resubmit_disable = extract_bit_fld_128(hi64, lo64, 1, 1);
  dev_p->efuse_data.mau_tcam_reduction = extract_bit_fld_128(hi64, lo64, 2, 2);
  dev_p->efuse_data.mau_sram_reduction = extract_bit_fld_128(hi64, lo64, 3, 3);
  dev_p->efuse_data.packet_generator_disable =
      extract_bit_fld_128(hi64, lo64, 4, 4);
  dev_p->efuse_data.pipe_disable = extract_bit_fld_128(hi64, lo64, 8, 5);
  dev_p->efuse_data.mau_stage_disable = extract_bit_fld_128(hi64, lo64, 20, 9);
  dev_p->efuse_data.port_disable_map_lo =
      extract_bit_fld_128(hi64, lo64, 84, 21);
  dev_p->efuse_data.port_disable_map_hi =
      extract_bit_fld_128(hi64, lo64, 85, 85);
  dev_p->efuse_data.tm_memory_disable =
      extract_bit_fld_128(hi64, lo64, 121, 86);
  dev_p->efuse_data.port_speed_reduction =
      extract_bit_fld_128(hi64, lo64, 127, 126);

  /* Nowparse the fields in the upper 128b */
  lo64 = efuse_data_raw[5];
  lo64 = (lo64 << 32) | efuse_data_raw[4];
  hi64 = efuse_data_raw[7];
  hi64 = (hi64 << 32) | efuse_data_raw[6];

  dev_p->efuse_data.cpu_port_speed_reduction =
      extract_bit_fld_128(hi64, lo64, (129 - 128), (128 - 128));
  dev_p->efuse_data.pcie_lane_reduction =
      extract_bit_fld_128(hi64, lo64, (131 - 128), (130 - 128));
  dev_p->efuse_data.baresync_disable =
      extract_bit_fld_128(hi64, lo64, (132 - 128), (132 - 128));
  dev_p->efuse_data.frequency_reduction =
      extract_bit_fld_128(hi64, lo64, (134 - 128), (133 - 128));
  dev_p->efuse_data.frequency_check_disable =
      extract_bit_fld_128(hi64, lo64, (135 - 128), (135 - 128));
  dev_p->efuse_data.versioning =
      extract_bit_fld_128(hi64, lo64, (140 - 128), (139 - 128));
  dev_p->efuse_data.chip_part_number =
      extract_bit_fld_128(hi64, lo64, (155 - 128), (151 - 128));
#ifdef DEVICE_IS_EMULATOR
  /* Force the revision number to B0 for the emulator so it doesn't need to fake
   * the efuse data. */
  dev_p->efuse_data.part_revision_number = 1;
#else
  dev_p->efuse_data.part_revision_number =
      extract_bit_fld_128(hi64, lo64, (163 - 128), (156 - 128));
#endif
  dev_p->efuse_data.package_id =
      extract_bit_fld_128(hi64, lo64, (165 - 128), (164 - 128));
  dev_p->efuse_data.silent_spin =
      extract_bit_fld_128(hi64, lo64, (167 - 128), (166 - 128));
  dev_p->efuse_data.chip_id =
      extract_bit_fld_128(hi64, lo64, (230 - 128), (168 - 128));
  dev_p->efuse_data.pmro_and_skew =
      extract_bit_fld_128(hi64, lo64, (242 - 128), (231 - 128));
  dev_p->efuse_data.voltage_scaling =
      extract_bit_fld_128(hi64, lo64, (245 - 128), (243 - 128));

  if (dev_p->efuse_data.chip_part_number == BFN_PART_NBR_BFNT10032D) {
    // T-3.2-Half
    uint64_t p = dev_p->efuse_data.port_disable_map_lo;
    uint32_t n_bits = 0;
    for (; p != 0; p >>= 1)
      if (p & 1) ++n_bits;

    if (n_bits == 46)
      dev_p->efuse_data.chip_part_number = BFN_PART_NBR_BFNT10032D_018;
    else if (n_bits == 44)
      dev_p->efuse_data.chip_part_number = BFN_PART_NBR_BFNT10032D_020;
  }
  return 0;
}

void lld_efuse_tof_wafer_str_get(bf_dev_id_t dev_id, int s_len, char *s) {
#ifndef __KERNEL__
  uint64_t chip_id = lld_efuse_get_chip_id(dev_id);
  uint64_t val, flipped = 0ull;
  uint64_t fab, lot, lotnum0, lotnum1, lotnum2, lotnum3, wafer, x, y;
  int i;

  // first, flip the 64 bits end-to-end
  for (i = 0; i < 64; i++) {
    flipped |= ((chip_id >> i) & 1) << (64 - i - 1);
  }
  // then decode
  val = flipped;
  fab = val >> 57ull;
  lot = (val >> 50ull) & 0x7full;
  lotnum0 = (val >> 43ull) & 0x7full;
  lotnum1 = (val >> 36ull) & 0x7full;
  lotnum2 = (val >> 29ull) & 0x7full;
  lotnum3 = (val >> 22ull) & 0x7full;
  wafer = (val >> 17ull) & 0x1full;
  x = (val >> 9ull) & 0xffull;
  y = (val >> 1ull) & 0xffull;
  snprintf(s,
           s_len,
           "%c%c%c%c%c%c-W%" PRIu64 "-X%" PRIu64 "-Y%" PRIu64,
           (char)fab,
           (char)lot,
           (char)lotnum0,
           (char)lotnum1,
           (char)lotnum2,
           (char)lotnum3,
           wafer,
           x,
           y);
#else
  (void)dev_id;
  (void)s;
#endif
}
