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



#ifndef port_mgr_tof3_physical_device_h_included
#define port_mgr_tof3_physical_device_h_included

#include <bf_types/bf_types.h>
#include "aw_if.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define TOF3_NUM_TMAC 65

#define TMAC_NUM_CH 8

// Logical serdes lane config
typedef struct port_mgr_tof3_serdes_t {
  bf_tf3_sd_t sd_cfg;
} port_mgr_tof3_serdes_t;

/** \typedef port_mgr_tmac_t:
 *
 */
typedef struct port_mgr_tmac_s {
  uint32_t ch_in_use;
  // indexed by logical lane
  port_mgr_tof3_serdes_t sd[TMAC_NUM_CH];
  // serdes lane map (i.e. swizzling)
  uint32_t phys_tx_ln[8];
  uint32_t phys_rx_ln[8];
  uint32_t max_serdes_per_mac;  // 4 or 8 depending on package
} port_mgr_tmac_t;

/** \typedef port_mgr_tof3_pdev_t:
 *
 */
typedef struct port_mgr_tof3_pdev_t {
  uint32_t pipe_log2phy[BF_PIPE_COUNT];
  port_mgr_tmac_t tmac[TOF3_NUM_TMAC];
  // note: should this be per-die?
  char *serdes_0_fw_path;
  char *serdes_1_32_fw_path;

} port_mgr_tof3_pdev_t;

#ifdef __cplusplus
}
#endif /* C++ */

#endif
