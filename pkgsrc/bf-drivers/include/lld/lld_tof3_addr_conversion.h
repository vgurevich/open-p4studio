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


#ifndef lld_tof3_addr_conversion_h
#define lld_tof3_addr_conversion_h

static inline uint64_t tof3_dir_to_indir_dev_sel(uint32_t daddr) {
  // Indirect Address Format for Device Select
  // 41:39 == PCIe 26:24 == 0
  // 38:34 == PCIe 23:19 == 5 bit Dev_sel type
  // 33:19 == 0 for registers, non-zero for memories
  // 18:0  == PCIe 18:0
  uint64_t x = daddr;
  x = ((x & UINT64_C(0xF80000)) << 15) | (x & UINT64_C(0x7FFFF));
  return x;
}

#endif
