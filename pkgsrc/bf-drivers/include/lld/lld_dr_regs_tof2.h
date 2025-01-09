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


#ifndef LLD_DRU_REGS_TOF2_H
#define LLD_DRU_REGS_TOF2_H

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "tof2_reg_drv_defs.h"

#define TOF2_DR_WRAP_BIT_POSITION (20 /*1MB boundary*/)
#define TOF2_DR_PTR_MASK ((1 << TOF2_DR_WRAP_BIT_POSITION) - 1)
#define TOF2_DR_WRAP_BIT_MASK (1 << TOF2_DR_WRAP_BIT_POSITION)

#define TOF2_DR_PTR_PART(x) ((x)&TOF2_DR_PTR_MASK)
#define TOF2_DR_WRP_PART(x) (((x)&TOF2_DR_WRAP_BIT_MASK) >> 20)

#define TOF2_DR_STRIDE \
  DEF_tof2_reg_device_select_pbc_pbc_il_tx_dr_array_element_size

#define TOF2_DR_BASE_MAC_TX \
  DEF_tof2_reg_device_select_mbc_mbc_mac_0_tx_dr_address
#define TOF2_DR_BASE_MAC_CPL \
  DEF_tof2_reg_device_select_mbc_mbc_mac_0_cpl_dr_address
#define TOF2_DR_BASE_MAC_WB_TX \
  DEF_tof2_reg_device_select_mbc_mbc_wb_tx_dr_address
#define TOF2_DR_BASE_MAC_WB_CPL \
  DEF_tof2_reg_device_select_mbc_mbc_wb_cpl_dr_address
#define TOF2_DR_BASE_IL_TX_0 \
  DEF_tof2_reg_device_select_pbc_pbc_il_tx_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_TX_1 \
  DEF_tof2_reg_device_select_pbc_pbc_il_tx_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_TX_2 \
  DEF_tof2_reg_device_select_pbc_pbc_il_tx_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_TX_3 \
  DEF_tof2_reg_device_select_pbc_pbc_il_tx_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_CPL_0 \
  DEF_tof2_reg_device_select_pbc_pbc_il_cpl_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_CPL_1 \
  DEF_tof2_reg_device_select_pbc_pbc_il_cpl_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_CPL_2 \
  DEF_tof2_reg_device_select_pbc_pbc_il_cpl_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_IL_CPL_3 \
  DEF_tof2_reg_device_select_pbc_pbc_il_cpl_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_WB_TX DEF_tof2_reg_device_select_pbc_pbc_wb_tx_dr_address
#define TOF2_DR_BASE_WB_CPL DEF_tof2_reg_device_select_pbc_pbc_wb_cpl_dr_address
#define TOF2_DR_BASE_RB_TX DEF_tof2_reg_device_select_pbc_pbc_rb_tx_dr_address
#define TOF2_DR_BASE_RB_CPL DEF_tof2_reg_device_select_pbc_pbc_rb_cpl_dr_address
#define TOF2_DR_BASE_STAT_FM \
  DEF_tof2_reg_device_select_pbc_pbc_stat_fm_dr_address
#define TOF2_DR_BASE_STAT_RX \
  DEF_tof2_reg_device_select_pbc_pbc_stat_rx_dr_address
#define TOF2_DR_BASE_IDLE_FM \
  DEF_tof2_reg_device_select_pbc_pbc_idle_fm_dr_address
#define TOF2_DR_BASE_IDLE_RX \
  DEF_tof2_reg_device_select_pbc_pbc_idle_rx_dr_address
#define TOF2_DR_BASE_DIAG_FM \
  DEF_tof2_reg_device_select_pbc_pbc_diag_fm_dr_address
#define TOF2_DR_BASE_DIAG_RX \
  DEF_tof2_reg_device_select_pbc_pbc_diag_rx_dr_address
#define TOF2_DR_BASE_WL_TX DEF_tof2_reg_device_select_cbc_cbc_wl0_tx_dr_address
#define TOF2_DR_BASE_WL_CPL \
  DEF_tof2_reg_device_select_cbc_cbc_wl0_cpl_dr_address
#define TOF2_DR_BASE_WL_TX_1 \
  DEF_tof2_reg_device_select_cbc_cbc_wl1_tx_dr_address
#define TOF2_DR_BASE_WL_CPL_1 \
  DEF_tof2_reg_device_select_cbc_cbc_wl1_cpl_dr_address
#define TOF2_DR_BASE_RB_TX_0 \
  DEF_tof2_reg_device_select_cbc_cbc_rb0_tx_dr_address
#define TOF2_DR_BASE_RB_CPL_0 \
  DEF_tof2_reg_device_select_cbc_cbc_rb0_cpl_dr_address
#define TOF2_DR_BASE_RB_TX_1 \
  DEF_tof2_reg_device_select_cbc_cbc_rb1_tx_dr_address
#define TOF2_DR_BASE_RB_CPL_1 \
  DEF_tof2_reg_device_select_cbc_cbc_rb1_cpl_dr_address
#define TOF2_DR_BASE_LQ_FM DEF_tof2_reg_device_select_cbc_cbc_lq_fm_dr_address
#define TOF2_DR_BASE_LQ_RX DEF_tof2_reg_device_select_cbc_cbc_lq_rx_dr_address
#define TOF2_DR_BASE_PKT_TX_0 \
  DEF_tof2_reg_device_select_tbc_tbc_tx_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_TX_1 \
  DEF_tof2_reg_device_select_tbc_tbc_tx_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_TX_2 \
  DEF_tof2_reg_device_select_tbc_tbc_tx_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_TX_3 \
  DEF_tof2_reg_device_select_tbc_tbc_tx_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_CPL_0 \
  DEF_tof2_reg_device_select_tbc_tbc_cpl_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_CPL_1 \
  DEF_tof2_reg_device_select_tbc_tbc_cpl_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_CPL_2 \
  DEF_tof2_reg_device_select_tbc_tbc_cpl_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_CPL_3 \
  DEF_tof2_reg_device_select_tbc_tbc_cpl_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_0 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_1 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_2 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_3 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_4 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (4 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_5 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (5 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_6 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (6 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_FM_7 \
  DEF_tof2_reg_device_select_tbc_tbc_fm_dr_address + (7 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_0 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (0 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_1 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (1 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_2 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (2 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_3 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (3 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_4 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (4 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_5 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (5 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_6 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (6 * TOF2_DR_STRIDE)
#define TOF2_DR_BASE_PKT_RX_7 \
  DEF_tof2_reg_device_select_tbc_tbc_rx_dr_address + (7 * TOF2_DR_STRIDE)

uint32_t lld_dr_tof2_base_get(bf_dma_dr_id_t dr);
bf_dma_dr_id_t lld_dr_tof2_dr_id_get(uint64_t address);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
