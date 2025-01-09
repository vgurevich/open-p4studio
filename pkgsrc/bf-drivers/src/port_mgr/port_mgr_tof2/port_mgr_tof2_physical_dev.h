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


#ifndef port_mgr_tof2_physical_device_h_included
#define port_mgr_tof2_physical_device_h_included

#include <bf_types/bf_types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define TOF2_NUM_UMAC4 32
#define TOF2_NUM_UMAC3 1

#define TOF2_CPU_UMAC3_STN_ID 0
#define TOF2_CPU_UMAC3_ROTATED_STN_ID 39

#define UMAC4_NUM_CH 8
#define UMAC3_NUM_CH 4

// Logical serdes lane config
typedef struct port_mgr_tof2_serdes_t {
  // Tx EQ parameters
  // (note: values are signed)
  int32_t pre2;
  int32_t pre1;
  int32_t main;
  int32_t post1;
  int32_t post2;

  // Tx polarity
  bool tx_inv;
  // Tx bandgap (calibrated)
  uint32_t tx_bg;

  // Rx polarity
  bool rx_inv;
  // Rx bandgap (calibrated)
  uint32_t rx_bg;
} port_mgr_tof2_serdes_t;

/** \typedef port_mgr_umac4_t:
 *
 */
typedef struct port_mgr_umac4_s {
  uint32_t ch_in_use;
  // indexed by logical lane
  port_mgr_tof2_serdes_t sd[UMAC4_NUM_CH];
  port_mgr_tof2_serdes_t hw_sd[UMAC4_NUM_CH];

  // serdes lane map (i.e. swizzling)
  uint32_t phys_tx_ln[8];
  uint32_t phys_rx_ln[8];

  /* Serdes lane remap info read back from the hardware */
  uint32_t hw_phys_tx_ln[8];
  uint32_t hw_phys_rx_ln[8];
} port_mgr_umac4_t;

/** \typedef port_mgr_umac3_t:
 *
 */
typedef struct port_mgr_umac3_s {
  uint32_t ch_in_use;
  port_mgr_tof2_serdes_t sd[UMAC3_NUM_CH];
  port_mgr_tof2_serdes_t hw_sd[UMAC3_NUM_CH];
  // serdes lane map (i.e. swizzling)
  uint32_t phys_tx_ln[8];
  uint32_t phys_rx_ln[8];

  /* Serdes lane remap info read back from the hardware */
  uint32_t hw_phys_tx_ln[8];
  uint32_t hw_phys_rx_ln[8];
} port_mgr_umac3_t;

/** \typedef port_mgr_tof2_pdev_t:
 *
 */
typedef struct port_mgr_tof2_pdev_t {
  uint32_t pipe_log2phy[BF_PIPE_COUNT];
  port_mgr_umac3_t umac3[TOF2_NUM_UMAC3];
  port_mgr_umac4_t umac4[TOF2_NUM_UMAC4];
  char *fw_grp_0_7_path;
  char *fw_grp_8_path;
} port_mgr_tof2_pdev_t;

#ifdef __cplusplus
}
#endif /* C++ */

#endif
