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


#ifndef BF_TOF2_SERDES_IF_H_INCLUDED
#define BF_TOF2_SERDES_IF_H_INCLUDED

/**
 * @file bf_tof2_serdes_if.h
 *
 * @brief BF Drivers APIs for Tofino2 SerDes management.
 *
 */

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct pll_info_t {
  float data_rate;
  float fvco;
  uint32_t pll_cap;
  float pll_n_float;
  uint32_t div4_en;
  uint32_t div2_bypass;
  float ref_clk;
  uint32_t pll_frac_en;
  uint32_t pll_frac_n;
} pll_info_t;

typedef enum {
  PROCESS_CORNER_UNDEF = -2,
  PROCESS_CORNER_SS = -1,
  PROCESS_CORNER_TT = 0,
  PROCESS_CORNER_FF = 1,
} bf_serdes_process_corner_t;

/** @brief  Set Tx and Rx logical to physical mappings (based on board layout)
 *
 *          Note: this function programs only the serdes, not the MAC, and
 *                does so immediately. It is intended for diagnostic purposes.
 *                For normal operation use bf_port_lane_map_set() in
 *                bf_port_if.c
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  phys_tx_ln : Tx phys -> logical for each logical lane, 0-7
 * @param[in]  phys_rx_ln : Rx phys -> logical for each logical lane, 0-7
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_lane_map_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t phys_tx_ln[8],
                                        uint32_t phys_rx_ln[8]);

/** @brief  Get Tx and Rx logical to physical mappings (based on board layout)
 *
 *          Note: this function programs only the serdes, not the MAC, and
 *                does so immediately. It is intended for diagnostic purposes.
 *                For normal operation use bf_port_lane_map_set() in
 *                bf_port_if.c
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  phys_tx_ln : Tx phys -> logical for each logical lane, 0-7
 * @param[in]  phys_rx_ln : Rx phys -> logical for each logical lane, 0-7
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_lane_map_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t phys_tx_ln[8],
                                        uint32_t phys_rx_ln[8]);

/** @brief  Cache Tx Eq settings and optionally apply to hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  pre2       : Pre2
 * @param[in]  pre        : Pre
 * @param[in]  main       : Main
 * @param[in]  post1      : Post1
 * @param[in]  post2      : Post2
 * @param[in]  apply      : true= apply to hw
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_taps_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       int32_t pre2,
                                       int32_t pre1,
                                       int32_t main,
                                       int32_t post1,
                                       int32_t post2,
                                       bool apply);

/** @brief  Retrieve cached Tx Eq settings
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  pre2       : Pre2
 * @param[in]  pre        : Pre
 * @param[in]  main       : Main
 * @param[in]  post1      : Post1
 * @param[in]  post2      : Post2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_taps_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       int32_t *pre2,
                                       int32_t *pre1,
                                       int32_t *main,
                                       int32_t *post1,
                                       int32_t *post2);

/** @brief  Retrieve currently applied (hw)  Tx Eq settings
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  pre2       : Pre2
 * @param[in]  pre        : Pre
 * @param[in]  main       : Main
 * @param[in]  post1      : Post1
 * @param[in]  post2      : Post2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_taps_hw_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          int32_t *pre2,
                                          int32_t *pre1,
                                          int32_t *main,
                                          int32_t *post1,
                                          int32_t *post2);

/** @brief Squelch Tx Output by 0'ing the Tx taps but don't update cache
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_squelch_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln);

/** @brief  Get Rx Signal information
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] sig_detect : Rx Signal detected (true)
 * @param[out] phy_ready  : CDR Lock (true)
 * @param[out] ppm        : Apparent PPM difference between local and remote
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_sig_info_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool *sig_detect,
                                           bool *phy_ready,
                                           int32_t *ppm);

/** @brief  Get AN (and HCD speed set) done
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] an_done    : AN complete (true)
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_an_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *an_done);

/** @brief  Get Rx Adaptation information
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] adapt_done : # DFE complete (true), may be NULL
 * @param[out] adapt_cnt  : # DFE attempts (total), may be NULL
 * @param[out] readapt_cnt: # DFE attempts since last read, may be NULL
 * @param[out] link_lost  : # times signal lost since last read, may be NULL
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_adapt_counts_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            bool *adapt_done,
                                            uint32_t *adapt_cnt,
                                            uint32_t *readapt_cnt,
                                            uint32_t *link_lost_cnt);

/** @brief  Get Rx Adaptation done indicationn
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] adapt_done : # DFE complete (true), may be NULL
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_adapt_done_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          bool *adapt_done);

/** @brief  Cache Tx polarity in serdes struct and optionally apply to hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 * @param[out] apply      : true=apply to hw
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_polarity_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool inv,
                                           bool apply);

/** @brief  Retreive cached Tx polarity from serdes struct
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_polarity_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool *inv);

/** @brief  Retreive programmed Tx polarity from hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_polarity_hw_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              bool *inv);

/** @brief  Cache Rx polarity in serdes struct and optionally apply to hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 * @param[out] apply      : true=apply to hw
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_polarity_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool inv,
                                           bool apply);

/** @brief  Retreive cached Rx polarity from serdes struct
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_polarity_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool *inv);

/** @brief  Retreive programmed Rx polarity from  hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] inv        : true=invert polarity
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_polarity_hw_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              bool *inv);

/** @brief  Apply a reset to an entire group of 8 (or 4) serdes.
 *
 * This function accomplishes the reset in three steps. The steps
 * are used to mask the required delays between resets.
 *
 * step 0: soft reset
 * step 1: logic reset
 * step 2: Training reset
 *
 * All steps require a 100us delay before executing the next step. You can
 * apply the group reset in parallel to all groups by executing each step
 * on all groups then waiting for a single 100us delay.
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] phase      : 0-2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_group_reset_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t phase);

/** @brief Run a simple Power-On Self-Test (POST)
 *
 * Verify accessibility of the tile and group comprising this port
 * by reading and writing a tile register that contains a known
 * reset value and is RW.
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  n_lanes  : Number of logical lanes
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_power_on_self_test(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t n_lanes);

/** @brief Run SRAM BIST on a group of 8 serdes
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_sram_bist_grp(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port);

/** @brief Run ROM BIST on a group of 8 serdes
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rom_bist_grp(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);

/** @brief Init the tile/group implementing the specified dev_port.
 *         Initialization requires,
 *           POST (optional)
 *           FW load (optional, may also be forced reload)
 *           verify requested FW is loaded
 *           un-configure any previously configured lanes
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  fw_file_name: full path to FW file
 * @param[in]  force_load : force FW reload
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_init_group(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      char *fw_file_name,
                                      bool force_load);

/** @brief Configure a lane to run autonegotiation
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  basepage   : 48b IEEE base page advertisement
 * @param[in]  consortium_np_31_16 : variable consortium advertisement bits
 * @param[in]  is_loop    : disable nonce check on loopback plugs
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_config_ln_autoneg(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint64_t basepage,
                                             uint32_t consortium_np_47_16,
                                             bool is_loop);

/** @brief Configure a serdes lane i either NRZ or PAM4 mode, based
 *         on the current port speed and n_lanes configured.
 *         If successful, also programs cached values of,
 *           Tx EQ params
 *           Tx polarity
 *           Rx polarity
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  speed      : 400, 200, 100, ...
 * @param[in]  n_lanes    : w/speed, distinguishes 25g vs 50g serdes speed
 * @param[in]  tx_pat     : NONE=mission mode
 * @param[in]  rx_pat     : NONE=mission mode
 * @param[in]  leave_squelched : exit leaving output squelched
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_config_ln(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     bf_port_speed_t speed,
                                     uint32_t n_lanes,
                                     bf_port_prbs_mode_t tx_pat,
                                     bf_port_prbs_mode_t rx_pat,
                                     bool leave_squelched);

/** @brief Return file and running hash and crc
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  fw_file_name     : FW file nale
 * @param[in]  fw_buffer_p      : FW buffer pointer
 * @param[out] fw_len           : FW length
 * @param[out] fw_hash_code     : FW hash code
 * @param[out] fw_crc           : FW CRC
 * @param[out] running_hash_code: Hash code
 * @param[out] running_crc)     : CRC
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_ver_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      char *fw_file_name,
                                      uint32_t *fw_len,
                                      uint32_t *fw_hash_code,
                                      uint32_t *fw_crc,
                                      uint32_t *running_hash_code,
                                      uint32_t *running_crc,
                                      uint32_t *fw_ver);

/** @brief Power-up/down specific serdes blocks
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  rx_off   : RX off
 * @param[in]  tx_off   : TX off
 * @param[in]  rx_bg_off: RX bg off
 * @param[in]  tx_bg_off: TX bg off
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_power_dn_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        bool rx_off,
                                        bool tx_off,
                                        bool rx_bg_off,
                                        bool tx_bg_off);

/** @brief Reset the PRBS error count
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_prbs_rst_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln);

/** @brief Return the PRBS error count
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] err_cnt  : 32b error count (note: wraps)
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_prbs_err_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *err_cnt);

/** @brief Return the TX PRBS config
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tx_cfg   : tx config
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_prbs_cfg_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *tx_cfg);

/** @brief Return the PPM offset
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ppm      : PPM offset
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_ppm_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   int32_t *ppm);

/** @brief Return eye height(s) in mv
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] eye_1    : NRZ or PAM4
 * @param[out] eye_2    : PAM4 only
 * @param[out] eye_3    : PAM4 only
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_eye_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   float *eye_1,
                                   float *eye_2,
                                   float *eye_3);

/** @brief Return OF/HF values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] of       : OF value
 * @param[out] hf       : HF value
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_of_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t *of,
                                  uint32_t *hf);

/** @brief Return Delta value
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] delta    : Delta
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_delta_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *delta);

/** @brief Return Edge1-4
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] edge1    : Edge1
 * @param[out] edge2    : Edge2
 * @param[out] edge3    : Edge3
 * @param[out] edge4    : Edge4
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_edge_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *edge1,
                                    uint32_t *edge2,
                                    uint32_t *edge3,
                                    uint32_t *edge4);

/** @brief Return NRZ DFE taps
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tap1     : Tap1
 * @param[out] tap2     : Tap2
 * @param[out] tap3     : Tap3
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_dfe_nrz_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *tap1,
                                       uint32_t *tap2,
                                       uint32_t *tap3);

/** @brief Return PAM4 DFE values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] f0       : F0
 * @param[out] f1       : F1
 * @param[out] ratio    : ratio
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_dfe_pam4_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        float *f0,
                                        float *f1,
                                        float *ratio);

/** @brief Return Skin Effect (SKEF) values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] skef_val : SKEF value
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_skef_val_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *skef_val);

/** @brief Return DAC value
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] dac_val  : DAC value
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_dac_val_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *dac_val);

/** @brief Return CTLE values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ctle_sel : Ctle select
 * @param[out] ctle_map_0: Ctle map0
 * @param[out] ctle_map_1: Ctle map1
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_ctle_val_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t ctle_sel,
                                        uint32_t *ctle_map_0,
                                        uint32_t *ctle_map_1);

/** @brief Return CTLE override value
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ctle_over_val : Override value
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_ctle_over_val_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *ctle_over_val);

/** @brief Return AGC gain values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] agc_gain_1: Agc gain 1
 * @param[out] agc_gain_2: Agc gain 2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_agcgain_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t ctle_gain_1,
                                       uint32_t ctle_gain_2);

/** @brief Set CTLE gain values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ctle_gain_1: Ctle gain 1
 * @param[out] ctle_gain_2: Ctle gain 2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_ctle_gain_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *ctle_gain_1,
                                         uint32_t *ctle_gain_2);

/** @brief Return FFE values
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] k1       : K1
 * @param[out] k2       : K2
 * @param[out] k3       : K3
 * @param[out] k4       : K4
 * @param[out] s1       : S1
 * @param[out] s2       : S2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_ffe_taps_pam4_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             int32_t *k1,
                                             int32_t *k2,
                                             int32_t *k3,
                                             int32_t *k4,
                                             int32_t *s1,
                                             int32_t *s2);

/** @brief Init the serdes tile chips (all ports)
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  fw_file_name      : PMA4+NRZ FW
 * @param[in]  fw_grp8_file_name : NRZ-only FW
 * @param[in]  skip_group_reset  : skip resetting group registers to defaults
 * @param[in]  skip_post         : skip running POST on the ports
 * @param[in]  force_fw_dnld     : force FW dnld
 * @param[in]  skip_power_dn     : skip placing ports in power-down state at
 *completion
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tile_init(bf_dev_id_t dev_id,
                                     char *fw_file_name,
                                     char *fw_grp8_file_name,
                                     bool skip_group_reset,
                                     bool skip_post,
                                     bool force_fw_dnld,
                                     bool skip_power_dn);

/** @brief Restart Rx adaptation on a lane
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_lane_reset_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int ln);

/** @brief Return lane speed and encoding mode
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out]  G       : speed in gigabits
 * @param[out]  enc_mode: NRZ or PAM4
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_lane_speed_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    int ln,
    uint32_t *G,
    bf_serdes_encoding_mode_t *enc_mode);

/** @brief Return F13 value
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] f13      : F13
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_f13_val_pam4_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *f13_val);

/** @brief Return whether FW is loaded or not.
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] loaded   : Loaded or not to be returned
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_loaded_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bool *loaded);

/** @brief Return FEC aalyzer tei
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tei      : TEI
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fec_analyzer_tei_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *tei);

/** @brief Return FEC analyzer teo
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] teo      : TEO
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fec_analyzer_teo_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *teo);

/** @brief Initialize the FEC analyzer
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in] err_type : Error type
 * @param[in] T        : Number of correctable symbols
 * @param[in] M        : Message symbol bits
 * @param[in] N        : Number of symbols per FEC block
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fec_analyzer_init_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t err_type,
                                                 uint32_t T,
                                                 uint32_t M,
                                                 uint32_t N);

/** @brief tof-2 clkobs pad config
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in] pad: BF_CLKOBS_PAD_0 or BF_CLKOBS_PAD_1
 * @param[in] clk_src: BF_SDS_NONE_CLK or BF_SDS_RX_RECOVEREDCLK or
 * BF_SDS_TX_CLK
 * @param[in] divider: 0,1,2, 3 for div by 2,4,8,16 respectively
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_clkobs_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_clkobs_pad_t pad,
                                      bf_sds_clkobs_clksel_t clk_src,
                                      int divider,
                                      int daisy_sel);

/** \brief tof-2 clkobs drive strength config
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param drive_strength: Clock observation pad drive strength. (0 ~ 15)
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG:
 */
bf_status_t bf_tof2_clkobs_drive_strength_set(bf_dev_id_t dev_id,
                                              int drive_strength);

/** @brief Retrieve AN status fields
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_an_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *lp_an_ability,
                                         uint32_t *link_status,
                                         uint32_t *an_ability,
                                         uint32_t *remote_fault,
                                         uint32_t *an_complete,
                                         uint32_t *page_rcvd,
                                         uint32_t *ext_np_status,
                                         uint32_t *parallel_detect_fault);

/** @brief Retrieve AN AN lp base page
 *
 * @param[in]  dev_id    : Device identifier
 * @param[in]  dev_port  : Port identifier
 * @param[in]  base_page : Link partner base page
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_an_lp_base_page_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint64_t *base_page);

/** @brief Retrieve AN lp base page
 *
 * @param[in]  dev_id       : Device identifier
 * @param[in]  dev_port     : Port identifier
 * @param[in]  lp_base_page : Link partner base page
 * @param[in]  lp_nxt_page1 : Link partner next page #1
 * @param[in]  lp_nxt_page2 : Link partner next page #2
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_an_lp_pages_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint64_t *lp_base_page,
                                           uint64_t *lp_nxt_page1,
                                           uint64_t *lp_nxt_page2);

/** @brief Retrieve AN HCD (highest common denomiator) speed
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] hcd       : HCD
 * @param[out] base_r_fec: FEC
 * @param[out] rs_fec    : RS_FEC
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_an_hcd_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *hcd,
                                      bool *base_r_fec,
                                      bool *rs_fec);

/** @brief Retrieve Link-training status fields
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_lt_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *readout_state,
                                         uint32_t *frame_lock,
                                         uint32_t *rx_trained,
                                         uint32_t *readout_training_state,
                                         uint32_t *training_failure,
                                         uint32_t *tx_training_data_en,
                                         uint32_t *sig_det,
                                         uint32_t *readout_txstate);

/** @brief Execute a Credo FW cmd
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  cmd      : Command
 * @param[in]  detail   : detail
 * @param[out] result   : Result
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_cmd(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int ln,
                                  uint32_t cmd,
                                  uint32_t detail,
                                  uint32_t *result);

/** @brief Execute a Credo FW debug cmd
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  section  : Section
 * @param[in]  index    : Index
 * @param[out] result   : Result
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_debug_cmd(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int ln,
                                        uint32_t section,
                                        uint32_t index,
                                        uint32_t *result);

/** @brief  Retreive programmed Tx bandgap from cache
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tx_bg      : TX bandgap
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_bandgap_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *tx_bg);

/** @brief  Retreive programmed Tx bandgap from hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tx_bg      : TX bandgap
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_bandgap_hw_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *tx_bg);

/** @brief Set TX bandgap
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  tx_bg      : TX bandgap
 * @param[in]  apply      : Flag to apply to HW or not
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tx_bandgap_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t tx_bg,
                                          bool apply);

/** @brief  Retreive programmed Rx bandgap from cache
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] rx_bg      : RX bandgap
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_bandgap_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *rx_bg);

/** @brief  Retreive programmed Rx bandgap from hw
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] rx_bg      : RX bandgap
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_bandgap_hw_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *rx_bg);

/** @brief  Set Rx bandgap
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  rx_bg      : RX bandgap
 * @param[in]  apply      : Flag to apply to HW or not
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_rx_bandgap_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t rx_bg,
                                          bool apply);

/** @brief  Set Tx and Rx logical to physical mappings (based on board layout)
 *
 *          Note: this function programs only the serdes, not the MAC, and
 *                does so immediately. It is intended for diagnostic purposes.
 *                For normal operation use bf_port_lane_map_set() in
 *                bf_port_if.c
 *
 * @param[in]  dev_id   : Device identifier
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_bandgap_init(bf_dev_id_t dev_id);

/** @brief Retrieve ISI info
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] isi_vals : ISI info
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_isi_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t isi_vals[16]);

/** @brief Inject errors
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  n_errs   : Errors
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_error_inject_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t n_errs);

/** @brief Start a temperature read operation
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_temperature_start_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln);

/** @brief Read the status if a previously started a temperature read operation
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] temp     : Temperature
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_temperature_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           float *temp);

/** @brief Read the temperature as measured by FW
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[out] temp     : Temperature
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_temperature_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              float *temp);

/** @brief Init lane prior to FW config cmd
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_init_ln(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln);

/** @brief Pre-Configure bandgap and polarity
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_pre_config_ln(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln);

/** @brief FW reg write with section
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  addr     : Address to write the data
 * @param[in]  data     : Data to be written
 * @param[in]  section  : Section to write the data
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_reg_section_wr(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t addr,
                                             uint32_t data,
                                             uint32_t section);

/** @brief FW reg read with section
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  addr     : Address to read the data
 * @param[out]  data    : Pointer to store the read data
 * @param[in]  section  : Section to read the data
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_reg_section_rd(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t addr,
                                             uint32_t *data,
                                             uint32_t section);

/** @brief Return programmed precoder settings
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] tx_en: TX enable
 * @param[out] rx_en: RX enable
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_precode_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *tx_en,
                                       bool *rx_en);

/** @brief Program precoder settings
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  tx_en: TX enable
 * @param[in]  rx_en: RX enable
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_precode_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool tx_en,
                                       bool rx_en);

/** @brief Return FW precoder config settings
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] phy_mode_tx_en: Phy mode tx enable
 * @param[out] phy_mode_rx_en: Phy mode rx enable
 * @param[out] anlt_mode_tx_en: ANLT mode tx enable
 * @param[out] anlt_mode_rx_en: ANLT mode rx enable
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_precode_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          bool *phy_mode_tx_en,
                                          bool *phy_mode_rx_en,
                                          bool *anlt_mode_tx_en,
                                          bool *anlt_mode_rx_en);

/** @brief Configure FW precoder settings
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] phy_mode_tx_en: Phy mode tx enable
 * @param[out] phy_mode_rx_en: Phy mode rx enable
 * @param[out] anlt_mode_tx_en: ANLT mode tx enable
 * @param[out] anlt_mode_rx_en: ANLT mode rx enable
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_precode_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          bool phy_mode_tx_en,
                                          bool phy_mode_rx_en,
                                          bool anlt_mode_tx_en,
                                          bool anlt_mode_rx_en);

/** @brief Retrieve PLL info
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[in]  tx_pll_info : TX PLL info
 * @param[in]  rx_pll_info : RX PLL info
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_pll_info_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        pll_info_t *tx_pll_info,
                                        pll_info_t *rx_pll_info);

/** @brief Get Eye plot from serdes FW
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out]  plot_data       : Plot data
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_fw_eye_plot_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint8_t **plot_data);

/** @brief Get termination mode
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ac_coupled: Termination mode
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_term_mode_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         bool *ac_coupled);

/** @brief Set termination mode
 *
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  ln       : Logical lane within port
 * @param[out] ac_coupled: Termination mode
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_term_mode_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         bool ac_coupled);

/** @brief Read tile Efuse value
 *
 * @param[in]  dev_id   : Device identifier
 * @param[in]  dev_port : Port identifier
 * @param[in]  bank      : "addr_pins" specifying efuse attribute to read
 * @param[out] efuse_val : Efuse value to be read
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tile_efuse_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int bank,
                                          uint32_t *efuse_val);

/** @brief Tile DRO value get
 *
 * @param[in]  dev_id   : Device identifier
 * @param[out] dro      : DRO values from up to 4 tiles
 * @param[out] max_dro  : used to determine part type
 * @param[out] num_dro_values : equal to number of pipes
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tile_dro_get(bf_dev_id_t dev_id,
                                        uint32_t dro[4],
                                        int32_t *max_dro,
                                        uint32_t *num_dro_values);

/** @brief Part type get
 *
 * @param[in]  dev_id   : Device identifier
 * @param[out] part_type: Part type
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_part_type_get(bf_dev_id_t dev_id,
                                         bf_serdes_process_corner_t *part_type);

/** @brief Read known value from tile
 *
 * @param[in]  dev_id   : Device identifier
 * @param[out] dev_port : determines tile to read
 *
 * @return Status of the API call
 *
 */
bf_status_t bf_tof2_serdes_tile_known_value_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
