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


#ifndef lld_tof_addr_conversion_h
#define lld_tof_addr_conversion_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t tof_dir_to_indir_dev_sel(uint32_t daddr) {
  // Indirect Address Format for Device Select
  // 41:39 == PCIe 25:23 == 0
  // 38:34 == PCIe 22:18 == 5 bit Dev_sel type
  // 33:18 == 0 for registers, non-zero for memories
  // 17:0  == PCIe 17:0
  uint64_t x = daddr;
  x = ((x & UINT64_C(0x7C0000)) << 16) | (x & UINT64_C(0x3FFFF));
  return x;
}

static inline uint64_t tof_dir_to_indir_mac(uint32_t daddr) {
  // Indirect Address Format for MAC
  // 41:39 == 010b
  // 38:32 == MAC [6:0]
  // 31:17 == 0 for registers, non-zero for memories
  // 16:0  == PCIe [16:0]
  uint64_t x = daddr;
  x = ((x & UINT64_C(0xFE0000)) << 15) | (x & UINT64_C(0x1FFFF)) |
      0x10000000000;
  return x;
}

static inline uint64_t tof_dir_to_indir_pipe(uint32_t daddr) {
  // Indirect Address Format for pipe
  // 41:40 == 10b (pipe)
  // 39:37 == PipeID
  // 36:33 == StageID
  // 32:19 == 0
  // 18:0  == PCIe 18:0
  uint64_t x = daddr;
  x = ((x & UINT64_C(0x1F80000)) << 15) | (x & UINT64_C(0x7FFFF)) |
      0x20000000000;
  return x;
}

static inline uint32_t tof_indir_to_dir_dev_sel(uint64_t iaddr) {
  // Indirect Address Format for Device Select
  // 41:39 == PCIe 25:23 == 0
  // 38:34 == PCIe 22:18 == 5 bit Dev_sel type
  // 33:18 == 0 for registers, non-zero for memories
  // 17:0  == PCIe 17:0
  uint64_t x = iaddr;
  x = ((x >> 16) & 0x7C0000) | (x & 0x3FFFF);
  return x & 0xFFFFFFFF;
}

#ifdef __cplusplus
}
#endif /* C++ */

#endif
