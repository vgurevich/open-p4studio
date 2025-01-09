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


#ifndef LLD_SKU_H_INCLUDED
#define LLD_SKU_H_INCLUDED

#include <lld/lld_err.h>
#include <bf_types/bf_types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BFN_PART_NBR_BFNT10064Q = 0x0,       // T-6.4-Full
  BFN_PART_NBR_BFNT10032Q = 0x10,      // T-3.2-Full
  BFN_PART_NBR_BFNT10032D = 0x08,      // T-3.2-Half
  BFN_PART_NBR_BFNT10032D_018 = 0x0c,  // T-3.2-18-Half
  BFN_PART_NBR_BFNT10032D_020 = 0x1a,  // T-3.2-20-Half

  /*
   * For Tofino 2 the CHIP_PART_NUMBER in EFUSE is encodeded as
   * [4:0] - IO BANDWIDTH 00000 = 128, 00001 = 96, 00011 = 80, 00111 = 64,
   *                      01111 = 56, 11111 = 48
   * [6:5] - PIPE 00 = 4, 01 = 3, 11 = 2
   * [8:7] - MAU STAGES 00 = U, 01 = M, 11 = H
   */

  BFN_PART_NBR_BFNT20128Q = 0x0,    // T2-12.8-Full
  BFN_PART_NBR_BFNT20128QM = 0x80,  // T2-12.8-Full-12MAU
  BFN_PART_NBR_BFNT20080T = 0x23,   // T2-8.0-Triple
  BFN_PART_NBR_BFNT20080TM = 0xA3,  // T2-8.0-Triple-12MAU
  BFN_PART_NBR_BFNT20064Q = 0x7,    // T2-6.4-Full
  BFN_PART_NBR_BFNT20064D = 0x67,   // T2-6.4-Half

  /* Tofino 3 */
  /*
   * For Tofino 3 the CHIP_PART_NUMBER in EFUSE is encodeded as
   * [1:0] - Serdes mode 00 = 56G, 01 = 112G,  10 = Hybrid
   * [4:2] - MAU STAGES 00 = U, 01 = M
   */

  BFN_PART_NBR_BFNT3_56G = 0x0,     // T3-12.8-4-256, Native: 256x53)
  BFN_PART_NBR_BFNT3_112G = 0x1,    // T3-12.8-4-512
  BFN_PART_NBR_BFNT3_HYBRID = 0x2,  // T3-12.8-4-192-32, Hybrid: 192x53 + 32x106



} bfn_part_nbr_e;

typedef enum {
  BFN_SKU_BFN_T1_64Q = 0,
  BFN_SKU_BFN77110 = BFN_SKU_BFN_T1_64Q,
  BFN_SKU_BFN_T1_32Q,
  BFN_SKU_BFN77120 = BFN_SKU_BFN_T1_32Q,
  BFN_SKU_BFN_T1_32D,
  BFN_SKU_BFN77121 = BFN_SKU_BFN_T1_32D,
  BFN_SKU_BFN_T1_32D_018,
  BFN_SKU_BFN77131 = BFN_SKU_BFN_T1_32D_018,
  BFN_SKU_BFN_T1_32D_020,
  BFN_SKU_BFN77140 = BFN_SKU_BFN_T1_32D_020,
  BFN_SKU_BFN0128Q,        // T2-12.8-Qual
  BFN_SKU_BFN0128QM,       // T2-12.8-Qual-12MAU
  BFN_SKU_BFN0080T,        // T2-8.0-Triple
  BFN_SKU_BFN0080TM,       // T2-8.0-Triple-12MAU
  BFN_SKU_BFN0064Q,        // T2-6.4-Quad
  BFN_SKU_BFN0064D,        // T2-6.4-Dual
  BFN_SKU_BFNT31_12Q,      // T3-12.8-4-256
  BFN_SKU_BFNT31_112_12Q,  // T3-12.8-4-512
  BFN_SKU_BFNT31_12QH,     // T3-12.8-4-192-32
  BFN_SKU_BFNT32_25Q,      // T3-25.6-8-256
  BFN_SKU_BFNT32_112_25Q,  // T3-25.6-8-512
  BFN_SKU_BFNT32_25QH,     // T3-25.6-8-192-32


} bfn_sku_e;

typedef enum {
  BF_SKU_PORT_SPEED_INVALID = 0,
  BF_SKU_PORT_SPEED_100G,
  BF_SKU_PORT_SPEED_50G,
  BF_SKU_PORT_SPEED_25G
} bf_sku_port_speed_t;

typedef enum {
  BF_SKU_CORE_CLK_UNDEF_GHZ = 0,
  BF_SKU_CORE_CLK_1_5_GHZ,
  BF_SKU_CORE_CLK_1_35_GHZ,
  BF_SKU_CORE_CLK_1_3_GHZ,
  BF_SKU_CORE_CLK_1_2625_GHZ,
  BF_SKU_CORE_CLK_1_25_GHZ,
  BF_SKU_CORE_CLK_1_2_GHZ,
  BF_SKU_CORE_CLK_1_1_GHZ,
  BF_SKU_CORE_CLK_1_0_GHZ,
  BF_SKU_CORE_CLK_1_05_GHZ,
} bf_sku_core_clk_freq_t;

typedef enum {
  BF_SKU_CHIP_PART_REV_A0 = 0,
  BF_SKU_CHIP_PART_REV_B0,
  BF_SKU_CHIP_PART_REV_B1,
  BF_SKU_CHIP_PART_REV_A1,
} bf_sku_chip_part_rev_t;

static inline const char *bf_sku_chip_part_rev_str(bf_sku_chip_part_rev_t rev) {
  switch (rev) {
    case BF_SKU_CHIP_PART_REV_A0:
      return "A0";
    case BF_SKU_CHIP_PART_REV_B0:
      return "B0";
    case BF_SKU_CHIP_PART_REV_B1:
      return "B1";
    case BF_SKU_CHIP_PART_REV_A1:
      return "A1";
  }
  return "Unknown";
}

uint32_t lld_sku_get_sku(bf_dev_id_t dev_id);
bf_dev_type_t lld_sku_get_dev_type(bf_dev_id_t dev_id);
bf_dev_type_t lld_sku_get_subdev_type(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id);
lld_err_t lld_sku_get_num_subdev(bf_dev_id_t dev_id,
                                 uint32_t *num_subdev,
                                 uint32_t *subdev_msk);
lld_err_t lld_sku_map_pipe_id_to_phy_pipe_id(bf_dev_id_t dev_id,
                                             bf_dev_pipe_t pipe_id,
                                             bf_dev_pipe_t *phy_pipe_id);
lld_err_t lld_sku_map_phy_pipe_id_to_pipe_id(bf_dev_id_t dev_id,
                                             bf_dev_pipe_t phy_pipe_id,
                                             bf_dev_pipe_t *pipe_id);
lld_err_t lld_sku_map_mac_ch_to_dev_port_id(bf_dev_id_t dev_id,
                                            uint32_t mac_blk,
                                            uint32_t ch,
                                            bf_dev_port_t *dev_port_id);
lld_err_t lld_sku_map_dev_port_id_to_mac_ch(bf_dev_id_t dev_id,
                                            uint32_t dev_port_id,
                                            uint32_t *mac_blk,
                                            uint32_t *ch);
lld_err_t lld_sku_get_num_active_pipes(bf_dev_id_t dev_id, uint32_t *num_pipes);
lld_err_t lld_sku_get_num_active_mau_stages(bf_dev_id_t dev_id,
                                            uint32_t *num_stages,
                                            uint32_t phy_pipe_id);
lld_err_t lld_sku_get_prsr_stage(bf_dev_id_t dev_id, uint32_t *stage);
lld_err_t lld_sku_get_dprsr_stage(bf_dev_id_t dev_id, uint32_t *stage);
lld_err_t lld_sku_check_efuse_consistency(bf_dev_id_t dev_id);
lld_err_t lld_sku_get_pcie_lanes(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 int *num_lanes);
lld_err_t lld_sku_get_port_speed(bf_dev_id_t dev_id, bf_sku_port_speed_t *sp);
lld_err_t lld_sku_get_baresync_en(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  int *en);
lld_err_t lld_sku_get_resubmit_en(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  int *en);
lld_err_t lld_sku_get_pkt_generator_en(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       int *en);
lld_err_t lld_sku_pipe_id_map_get(bf_dev_id_t dev_id,
                                  bf_dev_pipe_t pipe_log2phy[BF_PIPE_COUNT]);
lld_err_t lld_sku_get_core_clk_freq(bf_dev_id_t dev_id,
                                    bf_sku_core_clk_freq_t *bps_freq,
                                    bf_sku_core_clk_freq_t *pps_freq);
lld_err_t lld_sku_map_log2phy_mac_block(bf_dev_id_t dev_id,
                                        uint32_t log_mac_block,
                                        uint32_t *phy_mac_block);
lld_err_t lld_sku_map_phy2log_mac_block(bf_dev_id_t dev_id,
                                        uint32_t phy_mac_block,
                                        uint32_t *log_mac_block);
lld_err_t lld_sku_get_chip_part_revision_number(bf_dev_id_t dev_id,
                                                bf_sku_chip_part_rev_t *rev_no);

lld_err_t lld_sku_is_dev_port_internal(bf_dev_id_t dev_id,
                                       uint32_t dev_port_id,
                                       bool *is_internal);
lld_err_t lld_sku_get_chip_id(bf_dev_id_t dev_id, uint64_t *chip_id);
lld_err_t lld_sku_get_vmin(bf_dev_id_t dev_id, int *vmin);
bf_dev_port_t lld_get_pcie_cpu_port(bf_dev_id_t dev_id);
bf_dev_port_t lld_get_pcie_cpu_port2(bf_dev_id_t dev_id);
bf_dev_port_t lld_get_min_cpu_port(bf_dev_id_t dev_id);
bf_dev_port_t lld_get_min_cpu_port2(bf_dev_id_t dev_id);
bf_dev_port_t lld_get_max_cpu_port(bf_dev_id_t dev_id);
bf_status_t lld_get_next_cpu_port(bf_dev_id_t dev_id, bf_dev_port_t *port);
int lld_get_max_frontport_mac_per_pipe(bf_dev_id_t dev_id);
int lld_get_max_mac_blocks(bf_dev_id_t dev_id);
int lld_get_chnls_per_mac(bf_dev_id_t dev_id);
int lld_get_chnls_dev_port(bf_dev_id_t dev_id, bf_dev_port_t dev_port_id);
int lld_get_internal_pipe_numb(bf_dev_id_t dev_id);
int lld_get_min_fp_port(bf_dev_id_t dev_id);
int lld_get_max_fp_port(bf_dev_id_t dev_id);
lld_err_t lld_sku_map_mac_stn_id_to_tile_and_group(bf_dev_id_t dev_id,
                                                   uint32_t mac_stn_id,
                                                   uint32_t *tile,
                                                   uint32_t *grp);
lld_err_t lld_sku_map_tile_and_group_to_mac_stn_id(bf_dev_id_t dev_id,
                                                   uint32_t tile,
                                                   uint32_t grp,
                                                   uint32_t *mac_stn_id);
char *lld_sku_get_sku_name(bf_dev_id_t dev_id);
lld_err_t lld_sku_map_devport_from_user_to_device_safe(bf_dev_id_t dev_id,
                                                       bf_dev_port_t user_port,
                                                       bf_dev_port_t *dev_port);
bf_dev_port_t lld_sku_map_devport_from_user_to_device(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port);
bf_dev_port_t lld_sku_map_devport_from_device_to_user(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port);
int lld_get_num_serdes_per_mac(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // LLD_SKU_H_INCLUDED
