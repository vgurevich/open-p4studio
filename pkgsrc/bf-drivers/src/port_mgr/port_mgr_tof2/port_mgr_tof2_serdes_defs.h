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

#ifndef port_mgr_tof2_serdes_defs_h
#define port_mgr_tof2_serdes_defs_h

/********************************************************************
 * Serdes tile clock divider value. Used in 3 different blocks
 *  gpio
 *  eth400g_mac
 *  eth100g_reg
 * defines the clock of the MDIOCI interface as CORE_CLK/(CLKDIV+1).
 *
 * It should be programmed to 0011b to divide by 4 (1.35Ghz/4) as
 * we close timing at that speed.
 * All block can have different value but there is no reason for that
 * except if any timing issue.
 ********************************************************************/
typedef enum {
  BF_TOF2_MDIOCI_CLK_DIV_BY_1 = 0,
  BF_TOF2_MDIOCI_CLK_DIV_BY_2 = 1,
  BF_TOF2_MDIOCI_CLK_DIV_BY_4 = 3,
  BF_TOF2_MDIOCI_CLK_DIV_BY_8 = 7,
  BF_TOF2_MDIOCI_CLK_DIV_BY_16 = 15,

} port_mgr_tof2_mdioci_clk_div_t;
#define BF_TOF2_MDIOCI_CLK_DIV BF_TOF2_MDIOCI_CLK_DIV_BY_4

/********************************************************************
 * port_mgr_tof2_prbs_nrz_mode_t
 ********************************************************************/
typedef enum {
  PORT_MGR_PRBS_NRZ_MODE_PRBS9 = 0,
  PORT_MGR_PRBS_NRZ_MODE_PRBS15,
  PORT_MGR_PRBS_NRZ_MODE_PRBS23,
  PORT_MGR_PRBS_NRZ_MODE_PRBS31,
  PORT_MGR_PRBS_NRZ_MODE_NONE,
} port_mgr_tof2_prbs_nrz_mode_t;

/********************************************************************
 * port_mgr_tof2_prbs_pam4_mode_t
 ********************************************************************/
typedef enum {
  PORT_MGR_PRBS_PAM4_MODE_PRBS9 = 0,
  PORT_MGR_PRBS_PAM4_MODE_PRBS13,
  PORT_MGR_PRBS_PAM4_MODE_PRBS15,
  PORT_MGR_PRBS_PAM4_MODE_PRBS31,
  PORT_MGR_PRBS_PAM4_MODE_NONE,
} port_mgr_tof2_prbs_pam4_mode_t;

/* contains either
 *  port_mgr_tof2_prbs_pam4_mode_t -- or --
 *  port_mgr_tof2_prbs_nrz_mode_t
 */
typedef uint32_t port_mgr_tof2_prbs_mode_t;

/********************************************************************
 * port_mgr_serdes_nrz_speed_t
 ********************************************************************/
typedef enum {
  PORT_MGR_SERDES_NRZ_SPEED_NONE = 0,
  PORT_MGR_SERDES_NRZ_SPEED_1 = 1,
  PORT_MGR_SERDES_NRZ_SPEED_10 = 10,
  PORT_MGR_SERDES_NRZ_SPEED_20 = 20,
  PORT_MGR_SERDES_NRZ_SPEED_25 = 25,
} port_mgr_serdes_nrz_speed_t;

/********************************************************************
 * port_mgr_serdes_nrz_speed_t
 ********************************************************************/
typedef enum {
  PORT_MGR_SERDES_PAM4_SPEED_NONE = 0,
  PORT_MGR_SERDES_PAM4_SPEED_53 = 1,
} port_mgr_serdes_pam4_speed_t;

/* contains either
 *  port_mgr_serdes_nrz_speed_t -- or --
 *  port_mgr_serdes_pam4_speed_t
 */
typedef uint32_t port_mgr_tof2_serdes_rate_t;

/********************************************************************
 * credo_group8_0x13_sw_rstb_magic_get
 *
 * 12'h777=logic reset (that is, FIFO, PHY)
 * 12'h888=all reset (that is, FIFO, PHY, register map, CPU)
 * 12'h999=register map reset
 * 12'haaa=cpu reset
 *********************************************************************/
typedef enum {
  CRDO_MAGIC_VAL_NONE = 0,
  CRDO_MAGIC_VAL_LOGIC_RESET = 0x777,
  CRDO_MAGIC_VAL_SOFT_RESET = 0x888,
  CRDO_MAGIC_VAL_REG_MAP_RESET = 0x999,
  CRDO_MAGIC_VAL_CPU_RESET = 0xAAA,
} crdo_reset_magic_val_t;

/********************************************************************
 * crdo_lane_mode_t
 *
 *     00b:    NRZ
 *     01b:    PAM-4
 *     10b:    1G
 *     11b:    10G
 *********************************************************************/
typedef enum {
  CRDO_LANE_MODE_NRZ_25G = 0,
  CRDO_LANE_MODE_PAM4 = 1,
  CRDO_LANE_MODE_NRZ_1G = 2,
  CRDO_LANE_MODE_NRZ_10G = 3,
} crdo_lane_mode_t;

#endif  // port_mgr_tof2_serdes_defs_h
