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


#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_regs.h>
#include <lld/lld_dr_regs_tof2.h>
//#include "lld_dev.h"

/*
 * DRUs are distributed thru-out the address space based on
 * functionality. They are associated with a bus controller
 * in each functional block. Within that block they are
 * allocated in 0x40*8 segments.
 */
dru_addr_t dru_addr_tof2[] = {
    {lld_dr_fm_pkt_0, TOF2_DR_BASE_PKT_FM_0},
    {lld_dr_fm_pkt_1, TOF2_DR_BASE_PKT_FM_1},
    {lld_dr_fm_pkt_2, TOF2_DR_BASE_PKT_FM_2},
    {lld_dr_fm_pkt_3, TOF2_DR_BASE_PKT_FM_3},
    {lld_dr_fm_pkt_4, TOF2_DR_BASE_PKT_FM_4},
    {lld_dr_fm_pkt_5, TOF2_DR_BASE_PKT_FM_5},
    {lld_dr_fm_pkt_6, TOF2_DR_BASE_PKT_FM_6},
    {lld_dr_fm_pkt_7, TOF2_DR_BASE_PKT_FM_7},
    {lld_dr_fm_lrt, TOF2_DR_BASE_STAT_FM},
    {lld_dr_fm_idle, TOF2_DR_BASE_IDLE_FM},
    {lld_dr_fm_learn, TOF2_DR_BASE_LQ_FM},
    {lld_dr_fm_diag, TOF2_DR_BASE_DIAG_FM},
    {lld_dr_tx_pipe_inst_list_0, TOF2_DR_BASE_IL_TX_0},
    {lld_dr_tx_pipe_inst_list_1, TOF2_DR_BASE_IL_TX_1},
    {lld_dr_tx_pipe_inst_list_2, TOF2_DR_BASE_IL_TX_2},
    {lld_dr_tx_pipe_inst_list_3, TOF2_DR_BASE_IL_TX_3},
    {lld_dr_tx_pipe_write_block, TOF2_DR_BASE_WB_TX},
    {lld_dr_tx_pipe_read_block, TOF2_DR_BASE_RB_TX},
    {lld_dr_tx_que_write_list, TOF2_DR_BASE_WL_TX},
    {lld_dr_tx_pkt_0, TOF2_DR_BASE_PKT_TX_0},
    {lld_dr_tx_pkt_1, TOF2_DR_BASE_PKT_TX_1},
    {lld_dr_tx_pkt_2, TOF2_DR_BASE_PKT_TX_2},
    {lld_dr_tx_pkt_3, TOF2_DR_BASE_PKT_TX_3},
    {lld_dr_tx_mac_stat, TOF2_DR_BASE_MAC_TX},
    {lld_dr_rx_pkt_0, TOF2_DR_BASE_PKT_RX_0},
    {lld_dr_rx_pkt_1, TOF2_DR_BASE_PKT_RX_1},
    {lld_dr_rx_pkt_2, TOF2_DR_BASE_PKT_RX_2},
    {lld_dr_rx_pkt_3, TOF2_DR_BASE_PKT_RX_3},
    {lld_dr_rx_pkt_4, TOF2_DR_BASE_PKT_RX_4},
    {lld_dr_rx_pkt_5, TOF2_DR_BASE_PKT_RX_5},
    {lld_dr_rx_pkt_6, TOF2_DR_BASE_PKT_RX_6},
    {lld_dr_rx_pkt_7, TOF2_DR_BASE_PKT_RX_7},
    {lld_dr_rx_lrt, TOF2_DR_BASE_STAT_RX},
    {lld_dr_rx_idle, TOF2_DR_BASE_IDLE_RX},
    {lld_dr_rx_learn, TOF2_DR_BASE_LQ_RX},
    {lld_dr_rx_diag, TOF2_DR_BASE_DIAG_RX},
    {lld_dr_cmp_pipe_inst_list_0, TOF2_DR_BASE_IL_CPL_0},
    {lld_dr_cmp_pipe_inst_list_1, TOF2_DR_BASE_IL_CPL_1},
    {lld_dr_cmp_pipe_inst_list_2, TOF2_DR_BASE_IL_CPL_2},
    {lld_dr_cmp_pipe_inst_list_3, TOF2_DR_BASE_IL_CPL_3},
    {lld_dr_cmp_que_write_list, TOF2_DR_BASE_WL_CPL},
    {lld_dr_cmp_pipe_write_blk, TOF2_DR_BASE_WB_CPL},
    {lld_dr_cmp_pipe_read_blk, TOF2_DR_BASE_RB_CPL},
    {lld_dr_cmp_mac_stat, TOF2_DR_BASE_MAC_CPL},
    {lld_dr_cmp_tx_pkt_0, TOF2_DR_BASE_PKT_CPL_0},
    {lld_dr_cmp_tx_pkt_1, TOF2_DR_BASE_PKT_CPL_1},
    {lld_dr_cmp_tx_pkt_2, TOF2_DR_BASE_PKT_CPL_2},
    {lld_dr_cmp_tx_pkt_3, TOF2_DR_BASE_PKT_CPL_3},
    // new added for tof2
    {lld_dr_tx_mac_write_block, TOF2_DR_BASE_MAC_WB_TX},
    {lld_dr_tx_que_write_list_1, TOF2_DR_BASE_WL_TX_1},
    {lld_dr_tx_que_read_block_0, TOF2_DR_BASE_RB_TX_0},
    {lld_dr_tx_que_read_block_1, TOF2_DR_BASE_RB_TX_1},
    {lld_dr_cmp_mac_write_block, TOF2_DR_BASE_MAC_WB_CPL},
    {lld_dr_cmp_que_write_list_1, TOF2_DR_BASE_WL_CPL_1},
    {lld_dr_cmp_que_read_block_0, TOF2_DR_BASE_RB_CPL_0},
    {lld_dr_cmp_que_read_block_1, TOF2_DR_BASE_RB_CPL_1},
};

/********************************************************
 * lld_dr_tof2_base_get
 *
 * Return the Tof2 offset of a particular DRU.
 *******************************************************/
uint32_t lld_dr_tof2_base_get(bf_dma_dr_id_t dr) {
  return (dru_addr_tof2[dr].dru_offset);
}

/********************************************************
 * lld_dr_tof2_dr_id_get
 *
 * Return the DR that owns the register address passed
 * or BF_DMA_MAX_TOF2_DR if not a DRU reg
 *******************************************************/
bf_dma_dr_id_t lld_dr_tof2_dr_id_get(uint64_t address) {
  int dr;

  for (dr = 0; dr < (int)(sizeof(dru_addr_tof2) / sizeof(dru_addr_tof2[0]));
       dr++) {
    if ((address >= dru_addr_tof2[dr].dru_offset) &&
        (address < dru_addr_tof2[dr].dru_offset + TOF2_DR_STRIDE)) {
      return dr;
    }
  }
  return BF_DMA_MAX_DR;  // and probably crash
}
